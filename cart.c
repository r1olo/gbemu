#include <stdio.h>
#include <string.h>

#include "gbemu.h"

/* === No MBC === */
typedef struct nombc {
    byte sbank[0x4000];
    byte *ram;
} nombc_t;

static byte
nombc_read(cart_t *cart, ushort addr)
{
    nombc_t *data = (nombc_t *)cart->mbc_data;
    byte ret = 0xff;

    /* 0x0000 - 0x3FFF (ROM bank 0) */
    if (addr < 0x4000) {
        ret = cart->bank[addr];
    }

    /* 0x4000 - 0x7FFF (ROM bank 1) */
    else if (addr < 0x8000) {
        ret = data->sbank[addr - 0x4000];
    }

    /* 0xA000 - 0xBFFF (RAM if exists) */
    else if (addr >= 0xA000 && addr < 0xC000) {
        if (data->ram)
            ret = data->ram[addr - 0xA000];
    }

    /* out of bounds address */
    else {
        die("[nombc_read] address out of bounds");
    }

    return ret;
}

static void
nombc_write(cart_t *cart, ushort addr, byte val)
{
    /* out of bounds address */
    if (addr >= 0xC000)
        die("[nombc_write] address out of bounds");
}

static void
nombc_init(cart_t *cart, FILE *file)
{
    /* TODO: implement RAM (is it needed?) */
    cart->read = nombc_read;
    cart->write = nombc_write;
    cart->mbc_data = malloc_or_die(sizeof(nombc_t), "nombc_init", "mbc_data");

    nombc_t *data = (nombc_t *)cart->mbc_data;
    data->ram = NULL;

    if (!fread(data->sbank, 0x4000, 1, file))
        die("[nombc_init] can't read second bank from file:");
}

/* === MBC1 === */
typedef struct mbc1 {
    /* the bank arrays */
    byte **roms;
    byte **rams;

    /* how many rom and ram banks we have */
    uint nroms;
    uint nrams;

    /* indexes in the banks */
    uint currom;
    uint curram;

    /* out of currom's 5 bits, only this number of bits is actually used for
     * calculating the current bank. this is needed because if the top discarded
     * bits are different than 0 and the rest of the bits is 0, bank number 0 is
     * actually duplicated in the 4000 - BFFF zone */
    uint bits_for_rom;

    /* if this is set, curram bits are used as supplement for currom's bits */
    BOOL morerom;

    /* simple vs advanced banking mode */
    BOOL advanced;
} mbc1_t;

static byte
mbc1_read(cart_t *cart, ushort addr)
{
    return 0xff;
}

static void
mbc1_write(cart_t *cart, ushort addr, byte val)
{
    return;
}

static void
mbc1_init(cart_t *cart, FILE *file)
{
    /* TODO: implement a saving system */
    cart->read = mbc1_read;
    cart->write = mbc1_write;
    cart->mbc_data = malloc_or_die(sizeof(mbc1_t), "mbc1_init", "mbc_data");

    mbc1_t *data = (mbc1_t *)cart->mbc_data;
    data->currom = 0;
    data->curram = 0;
    data->advanced = FALSE;

    /* 0x148 -> ROM size */
    switch(cart->bank[0x148]) {
        case 0x00:
            data->nroms = 1;
            data->bits_for_rom = 1;
            data->morerom = FALSE;
            break;
        case 0x01:
            data->nroms = 3;
            data->bits_for_rom = 2;
            data->morerom = FALSE;
            break;
        case 0x02:
            data->nroms = 7;
            data->bits_for_rom = 3;
            data->morerom = FALSE;
            break;
        case 0x03:
            data->nroms = 15;
            data->bits_for_rom = 4;
            data->morerom = FALSE;
            break;
        case 0x04:
            data->nroms = 31;
            data->bits_for_rom = 5;
            data->morerom = FALSE;
            break;
        case 0x05:
            data->nroms = 63;
            data->bits_for_rom = 5;
            data->morerom = TRUE;
            break;
        case 0x06:
            data->nroms = 127;
            data->bits_for_rom = 5;
            data->morerom = TRUE;
            break;
        default:
            die("[mbc1_init] wrong ROM size");
    }

    /* create the ROM banks */
    data->roms = malloc_or_die(data->nroms * sizeof(void *), "mbc1_init", "rom array");
    for (int i = 0; i < data->nroms; i++) {
        data->roms[i] = malloc_or_die(0x4000, "mbc1_init", "rom");
        if (!fread(data->roms[i], 0x4000, 1, file))
            die("[mbc1_init] can't read bank from file");
    }

    /* 0x149 -> RAM size */
    switch (cart->bank[0x149]) {
        case 0x00:
            data->nrams = 0;
            break;
        case 0x02:
            data->nrams = 1;
            break;
        case 0x03:
            if (data->morerom)
                die("[mbc1_init] 32 KiB RAM with >512 KiB ROM");
            data->nrams = 4;
            break;
        default:
            die("[mbc1_init] wrong RAM size");
    }

    /* create the RAM banks */
    if (data->nrams) {
        data->rams = malloc_or_die(data->nrams * sizeof(void *), "mbc1_init", "ram array");
        for (int i = 0; i < data->nrams; i++)
            data->rams[i] = malloc_or_die(0x4000, "mbc1_init", "ram");
    } else {
        data->rams = NULL;
    }
}

static void
select_mbc(cart_t *cart, FILE *file)
{
    /* 0x147 -> MBC type */
    switch (cart->bank[0x147]) {
        case 0x00: /* ROM ONLY */
            nombc_init(cart, file);
            break;
        case 0x01: /* MBC1 */
        case 0x02: /* MBC1+RAM */
        case 0x03: /* MBC1+RAM+BATTERY */
            mbc1_init(cart, file);
            break;
        default:
            die("[select_mbc] unknown MBC type");
    }
}

void
cart_init(cart_t *cart, gb_t *bus, FILE *file)
{
    cart->bus = bus;

    if (!fread(cart->bank, 0x4000, 1, file))
        die("[cart_init] can't read first bank from file:");

    select_mbc(cart, file);
}

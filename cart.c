#include <stdio.h>
#include <string.h>

#include "gbemu.h"

/* === No MBC === */
typedef struct nombc {
    byte **banks;
    byte *ram;
} nombc_t;

static byte
nombc_read(cart_t *cart, ushort addr)
{
    nombc_t *data = (nombc_t *)cart->mbc_data;
    byte ret = 0xff;

    /* 0x0000 - 0x3FFF (ROM bank 0) */
    if (IS_IN_RANGE(addr, 0x0000, 0x3FFF)) {
        ret = data->banks[0][addr];
    }

    /* 0x4000 - 0x7FFF (ROM bank 1) */
    else if (IS_IN_RANGE(addr, 0x4000, 0x7FFF)) {
        ret = data->banks[1][addr - 0x4000];
    }

    /* 0xA000 - 0xBFFF (RAM if exists) */
    else if (IS_IN_RANGE(addr, 0xA000, 0xBFFF)) {
        if (data->ram)
            ret = data->ram[addr - 0xA000];
    }

    /* out of bounds address */
    else
        die("[nombc_read] address out of bounds");

    return ret;
}

static void
nombc_write(cart_t *cart, ushort addr, byte val)
{
    /* out of bounds address */
    if (!IS_IN_RANGE(addr, 0x0000, 0x7FFF) || !IS_IN_RANGE(addr, 0xA000, 0xBFFF))
        die("[nombc_write] address out of bounds");
}

static void
nombc_init(cart_t *cart, FILE *file)
{
    cart->read = nombc_read;
    cart->write = nombc_write;
    cart->mbc_data = malloc_or_die(sizeof(nombc_t), "nombc_init", "mbc_data");

    nombc_t *data = (nombc_t *)cart->mbc_data;
    data->banks = malloc_or_die(2 * sizeof(void *), "nombc_init", "rom array");
    data->banks[0] = cart->bank;
    data->banks[1] = malloc_or_die(0x4000, "nombc_init", "rom bank");

    /* 0x149 -> RAM size */
    switch (cart->bank[0x149]) {
        case 0x00:
            data->ram = NULL;
            break;
        case 0x02:
            data->ram = malloc_or_die(0x4000, "nombc_init", "ram bank");
            break;
        default:
            die("[nombc_init] wrong RAM size");
    }
}

/* === MBC1 === */
typedef struct mbc1 {
    /* the bank arrays */
    byte **roms;
    byte **rams;

    /* how many rom and ram banks we have */
    uint nroms;
    uint nrams;

    /* registers */
    byte currom; /* only last 5 bits are used */
    byte curram; /* only last 2 bits are used */
    byte mode; /* only last bit is used */

    /* ram enable */
    BOOL ram_enable;

    /* out of currom's 5 bits, only this number of bits is actually used for
     * calculating the current bank. this is needed because if the top discarded
     * bits are different than 0 and the rest of the bits is 0, bank number 0 is
     * actually duplicated in the 4000 - BFFF zone */
    uint bits_for_rom;

    /* if this is set, curram bits are used as supplement for currom's bits */
    BOOL morerom;
} mbc1_t;

static uint
mbc1_getfirstrombanknumber(mbc1_t *data)
{
    /* get bank index for 0000 - 3FFF zone */
    uint ret = 0;

    /* if we're in advanced banking, we need to read the curram bits, otherwise
     * it's always 0 */
    if (data->mode & MASK(1))
        ret = (data->curram & MASK(2)) << 5;

    return ret;
}

static uint
mbc1_getsecondrombanknumber(mbc1_t *data)
{
    /* get bank index for 4000 - 7FFF zone */
    /* first get the 5 bits: if they're 0, treat it as 1 */
    uint ret = data->currom & MASK(5);
    if (ret == 0)
        ret = 1;

    /* then only get the bits that are really needed */
    ret &= MASK(data->bits_for_rom);

    /* if we wired the RAM bits for ROM, add them now */
    if (data->morerom)
        ret |= ((data->curram & MASK(2)) << 5);

    return ret;
}

static uint
mbc1_getrambanknumber(mbc1_t *data)
{
    uint ret = (data->curram & MASK(2));

    /* if we wired the RAM bits to ROM or we're in simple banking mode, bank
     * number is always 0 */
    if (data->morerom || !(data->mode & MASK(1)))
        ret = 0;

    return ret;
}

static byte
mbc1_read(cart_t *cart, ushort addr)
{
    mbc1_t *data = (mbc1_t *)cart->mbc_data;
    byte ret = 0xff;
    uint idx;

    /* 0x0000 - 0x3FFF (ROM bank X0) */
    if (IS_IN_RANGE(addr, 0x0000, 0x3FFF)) {
        idx = mbc1_getfirstrombanknumber(data);
        if (idx >= data->nroms)
            die("[mbc1_read] tried to read invalid rom bank");
        ret = data->roms[idx][addr];
    }

    /* 0x4000 - 0x7FFF (ROM bank 01-7F) */
    else if (IS_IN_RANGE(addr, 0x4000, 0x7FFF)) {
        idx = mbc1_getsecondrombanknumber(data);
        if (idx >= data->nroms)
            die("[mbc1_read] tried to read invalid rom bank");
        ret = data->roms[idx][addr - 0x4000];
    }

    /* 0xA000 - 0xBFFF (RAM bank 00-03) */
    else if (IS_IN_RANGE(addr, 0xA000, 0xBFFF)) {
        idx = mbc1_getrambanknumber(data);
        if (idx >= data->nrams)
            die("[mbc1_read] tried to read invalid ram bank");
        if (data->ram_enable)
            ret = data->rams[idx][addr - 0xA000];
    }

    /* out of bounds address */
    else
        die("[mbc1_read] address out of bounds");

    return ret;
}

static void
mbc1_write(cart_t *cart, ushort addr, byte val)
{
    mbc1_t *data = (mbc1_t *)cart->mbc_data;

    /* 0x0000 - 0x1FFF (RAM Enable) */
    if (IS_IN_RANGE(addr, 0x0000, 0x1FFF))
        data->ram_enable = val == 0xA;

    /* 0x2000 - 0x3FFF (ROM Bank Number) */
    else if (IS_IN_RANGE(addr, 0x2000, 0x3FFF))
        data->currom = val;

    /* 0x4000 - 0x5FFF (RAM Bank Number or Upper Bits */
    else if (IS_IN_RANGE(addr, 0x4000, 0x5FFF))
        data->curram = val;

    /* 0x6000 - 0x7FFF (Banking Mode Select) */
    else if (IS_IN_RANGE(addr, 0x6000, 0x7FFF))
        data->mode = val;

    else if (!IS_IN_RANGE(addr, 0x0000, 0x7FFF) || !IS_IN_RANGE(addr, 0xA000, 0xBFFF))
        die("[mbc1_write] address out of bounds");
}

static void
mbc1_init(cart_t *cart, FILE *file)
{
    /* TODO: implement a saving system */
    cart->read = mbc1_read;
    cart->write = mbc1_write;
    cart->mbc_data = malloc_or_die(sizeof(mbc1_t), "mbc1_init", "mbc_data");

    mbc1_t *data = (mbc1_t *)cart->mbc_data;
    data->currom = data->curram = data->mode = 0;
    data->ram_enable = FALSE;

    /* 0x148 -> ROM size */
    switch(cart->bank[0x148]) {
        case 0x00:
            data->nroms = 2;
            data->bits_for_rom = 1;
            data->morerom = FALSE;
            break;
        case 0x01:
            data->nroms = 4;
            data->bits_for_rom = 2;
            data->morerom = FALSE;
            break;
        case 0x02:
            data->nroms = 8;
            data->bits_for_rom = 3;
            data->morerom = FALSE;
            break;
        case 0x03:
            data->nroms = 16;
            data->bits_for_rom = 4;
            data->morerom = FALSE;
            break;
        case 0x04:
            data->nroms = 32;
            data->bits_for_rom = 5;
            data->morerom = FALSE;
            break;
        case 0x05:
            data->nroms = 64;
            data->bits_for_rom = 5;
            data->morerom = TRUE;
            break;
        case 0x06:
            data->nroms = 128;
            data->bits_for_rom = 5;
            data->morerom = TRUE;
            break;
        default:
            die("[mbc1_init] wrong ROM size");
    }

    /* create the ROM banks */
    data->roms = malloc_or_die(data->nroms * sizeof(void *), "mbc1_init", "rom array");
    data->roms[0] = cart->bank;
    for (int i = 1; i < data->nroms; i++) {
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
    data->rams = NULL;
    if (data->nrams) {
        data->rams = malloc_or_die(data->nrams * sizeof(void *), "mbc1_init", "ram array");
        for (int i = 0; i < data->nrams; i++)
            data->rams[i] = malloc_or_die(0x4000, "mbc1_init", "ram");
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
    cart->bank = malloc_or_die(0x4000, "cart_init", "first bank");

    if (!fread(cart->bank, 0x4000, 1, file))
        die("[cart_init] can't read first bank from file:");

    select_mbc(cart, file);
}

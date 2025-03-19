#include "cart.h"
#include "log.h"
#include "types.h"

static uint8_t
_fake_read(cart_t *cart, uint16_t addr)
{
    return 0xFF;
}

static void
_fake_write(cart_t *cart, uint16_t addr, uint8_t val)
{
    return;
}

struct nombc {
    uint8_t bank1[0x4000];
    uint8_t *ram;
};

static enum gb_err
nombc_init(cart_t *cart, FILE *file)
{
    enum gb_err err;

    /* allocate the mbc data */
    cart->mbc_data = malloc(sizeof(struct nombc));
    if (!cart->mbc_data)
        gb_die(errno);

    /* fill the struct with empty data */
    memset(cart->mbc_data, 0, sizeof(struct nombc));
    struct nombc *nombc_data = (struct nombc *)cart->mbc_data;

    /* try reading the second bank */
    if (!fread(nombc_data->bank1, 0x4000, 1, file)) {
        LOG(LOG_ERR, "can't read second ROM bank from file");
        err = GBEMU_BAD_FILE;
        goto free;
    }

    return GBEMU_SUCCESS;

free:
    free(cart->mbc_data);
    return err;
}

static uint8_t
nombc_read_rom(cart_t *cart, uint16_t addr)
{
    /* access first bank (A14 is low) */
    if (!(addr & 0x4000))
        return cart->bank0[addr & 0x3FFF];

    /* return second bank (A14 is high) */
    struct nombc *nombc_data = (struct nombc *)cart->mbc_data;
    return nombc_data->bank1[addr & 0x3FFF];
}

static void
nombc_write_rom(cart_t *cart, uint16_t addr, uint8_t val)
{
    /* writing to a read only memory is generally not a good idea */
    return;
}

static uint8_t
nombc_read_ram(cart_t *cart, uint16_t addr)
{
    /* if RAM exists, read from it */
    struct nombc *nombc_data = (struct nombc *)cart->mbc_data;
    if (nombc_data->ram)
        return nombc_data->ram[addr & 0x1FFF];
    return 0xFF;
}

static void
nombc_write_ram(cart_t *cart, uint16_t addr, uint8_t val)
{
    /* if RAM exists, write to it */
    struct nombc *nombc_data = (struct nombc *)cart->mbc_data;
    if (nombc_data->ram) {
        nombc_data->ram[addr & 0x1FFF] = val;
        return;
    }
}

struct mbc1 {
    /* the banks and rams arrays */
    uint8_t (*banks)[0x4000];
    uint8_t (*rams)[0x2000];

    /* the number of banks and rams */
    unsigned nbanks;
    unsigned nrams;

    /* the current bank register. this has to be masked with all the possible
     * ROMS (if it's 3, then mask it with 0x03). note that for the 00 -> 01
     * conversion, all the 5 bits are checked (mask is 0x1F always) */
    uint8_t cur_bank;
};

static enum gb_err
mbc1_init(cart_t *cart, FILE *file)
{
    /* TODO */
    enum gb_err err;

    /* try allocating the MBC1 data */
    cart->mbc_data = malloc(sizeof(struct mbc1));
    if (!cart->mbc_data)
        gb_die(errno);

    /* get MBC1 data */
    struct mbc1 *mbc1_data = (struct mbc1 *)cart->mbc_data;

    /* read ROMS */
    switch (cart->bank0[0x148]) {
        case 0x01:
            mbc1_data->nbanks = 3;
            break;
        default:
            LOG(LOG_ERR, "invalid number of ROM banks");
            err = GBEMU_BAD_CART;
            goto free_data;
    }

    /* read RAMS */
    switch (cart->bank0[0x149]) {
        case 0x00:
            mbc1_data->nrams = 0;
            break;
        case 0x02:
            mbc1_data->nrams = 1;
            break;
        default:
            LOG(LOG_ERR, "invalid number of RAM banks");
            err = GBEMU_BAD_CART;
            goto free_data;
    }

    /* allocate roms */
    mbc1_data->banks = malloc(0x4000 * mbc1_data->nbanks);
    if (!mbc1_data->banks)
        gb_die(errno);

    /* fill roms from file */
    for (size_t i = 0; i < mbc1_data->nbanks; ++i) {
        if (!fread(mbc1_data->banks[i], 0x4000, 1, file)) {
            LOG(LOG_ERR, "can't read ROM banks from file");
            err = GBEMU_BAD_FILE;
            goto free_banks;
        }
    }

    /* allocate rams */
    if (mbc1_data->nrams) {
        mbc1_data->rams = malloc(0x2000 * mbc1_data->nrams);
        if (mbc1_data->rams)
            gb_die(errno);

        /* fill rams with all zeroes */
        for (size_t i = 0; i < mbc1_data->nrams; ++i)
            memset(mbc1_data->rams[i], 0, 0x2000);
    }

    /* set current (external) bank to 1 */
    mbc1_data->cur_bank = 1;

    return GBEMU_SUCCESS;

free_banks:
    free(mbc1_data->banks);
free_data:
    free(cart->mbc_data);
    return err;
}

static inline unsigned
_mbc1_get_cur_rom(struct mbc1 *mbc1_data)
{
    /* current ROM bank */
    unsigned cur_rom = mbc1_data->cur_bank & mbc1_data->nbanks;
    if ((mbc1_data->cur_bank & 0x1F) == 0)
        cur_rom = 1;

    return cur_rom;
}

static uint8_t
mbc1_read_rom(cart_t *cart, uint16_t addr)
{
    /* TODO */

    /* if A14 is low read first bank */
    if (!(addr & 0x4000))
        return cart->bank0[addr & 0x3FFF];

    /* get our mbc data */
    struct mbc1 *mbc1_data = (struct mbc1 *)cart->mbc_data;

    /* get current rom */
    unsigned cur_rom = _mbc1_get_cur_rom(mbc1_data);

    /* if current rom is 0, read from first bank */
    if (cur_rom == 0)
        return cart->bank0[addr & 0x3FFF];
    
    /* else, read from the rom's external banks */
    assert(cur_rom <= mbc1_data->nbanks);
    return mbc1_data->banks[cur_rom - 1][addr & 0x3FFF];
}

static void
mbc1_write_rom(cart_t *cart, uint16_t addr, uint8_t val)
{
    /* TODO (do NOT write to a ROM) */

    /* get our mbc data */
    struct mbc1 *mbc1_data = (struct mbc1 *)cart->mbc_data;

    /* 0x2000 - 0x3FFF - ROM Bank Number */
    if (addr >= 0x2000 && addr < 0x4000)
        mbc1_data->cur_bank = val;
}

static enum gb_err
select_mbc(cart_t *cart, FILE *file)
{
    /* TODO */
    enum gb_err ret;
    switch (cart->bank0[0x147]) {
        case 0x00:
            ret = nombc_init(cart, file);
            cart->read_rom = nombc_read_rom;
            cart->write_rom = nombc_write_rom;
            cart->read_ram = nombc_read_ram;
            cart->write_ram = nombc_write_ram;
            break;
        case 0x01:
            ret = mbc1_init(cart, file);
            cart->read_rom = mbc1_read_rom;
            cart->write_rom = mbc1_write_rom;
            cart->read_ram = _fake_read;
            cart->write_ram = _fake_write;
            break;
        default:
            LOG(LOG_ERR, "invalid MBC type");
            ret = GBEMU_BAD_CART;
            break;
    }

    return ret;
}

enum gb_err
cart_init(cart_t *cart, FILE *file)
{
    /* try reading from file */
    memset(cart, 0, sizeof(cart_t));
    if (!fread(cart->bank0, 0x4000, 1, file)) {
        LOG(LOG_ERR, "can't read first ROM bank from file");
        return GBEMU_BAD_FILE;
    }

    /* select MBC */
    return select_mbc(cart, file);
}

enum gb_err
cart_create(cart_t **pcart, const char *filename)
{
    /* try mallocing */
    cart_t *cart = malloc(sizeof(cart_t));
    if (!cart)
        gb_die(errno);

    /* open file and init cart */
    FILE *file = fopen(filename, "rb");
    if (!file) {
        LOG(LOG_ERR, "failed to open file");
        free(cart);
        return GBEMU_BAD_FILE;
    }

    enum gb_err ret = cart_init(cart, file);
    fclose(file);
    if (ret != GBEMU_SUCCESS) {
        LOG(LOG_ERR, "failed to initialize cart");
        free(cart);
        return ret;
    }

    /* close file and return cart */
    *pcart = cart;
    return GBEMU_SUCCESS;
}

void
cart_destroy(cart_t *cart)
{
    /* first nuke the mbc data (if any) (wait why shouldn't it be) */
    if (cart->mbc_data)
        free(cart->mbc_data);

    /* nuke the cart */
    free(cart);
}

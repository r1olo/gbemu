#include "bus.h"
#include "log.h"
#include "types.h"

/* this represents an external bus */
typedef struct ext_bus {
    uint8_t (*read)(struct ext_bus *bus, uint16_t addr, bool cs);
    void    (*write)(struct ext_bus *bus, uint16_t addr, bool cs, uint8_t val);
    mem64_t *ext_ram;
    cart_t  *cart;
} ext_bus_t;

/* quick shortcuts */
#define A15(addr) (addr & 0x8000)
#define A14(addr) (addr & 0x4000)
#define A13(addr) (addr & 0x2000)

static uint8_t
_ext_bus_read(ext_bus_t *bus, uint16_t addr, bool cs)
{
    /* if A15 is low (ROM) */
    if (cs && !A15(addr))
        return bus->cart->read_rom(bus->cart, addr);

    /* if #cs is low and A14 is up (ext RAM). by placing this before cart RAM,
     * we make Echo RAM possible. notice however that if A14 and A15 are up at
     * the same time, the PC should explode */
    if (!cs && A14(addr))
        return bus->ext_ram->data[addr];

    /* if #cs is low and A13 is up (cart RAM) */
    if (!cs && A13(addr))
        return bus->cart->read_ram(bus->cart, addr);

    /* this is not supposed to happen */
    LOG(LOG_ERR, "bad read issued to external bus");
    return 0xFF;
}

static void
_ext_bus_write(ext_bus_t *bus, uint16_t addr, bool cs, uint8_t val)
{
    /* if A15 is low (ROM) */
    if (cs && !A15(addr)) {
        bus->cart->write_rom(bus->cart, addr, val);
        return;
    }

    /* if #cs is low and A14 is up (ext RAM). by placing this before cart RAM,
     * we make Echo RAM possible. notice however that if A14 and A15 are up at
     * the same time, the PC should explode */
    if (!cs && A14(addr)) {
        bus->ext_ram->data[addr] = val;
        return;
    }

    /* if #cs is low and A13 is up (cart RAM) */
    if (!cs && A13(addr)) {
        bus->cart->write_ram(bus->cart, addr, val);
        return;
    }

    /* not supposed to happen */
    LOG(LOG_ERR, "bad write issued to external bus");
}

bus_t *
ext_bus_create(mem64_t *ext_ram, cart_t *cart)
{
    /* try to allocate */
    ext_bus_t *ext_bus = malloc(sizeof(ext_bus_t));
    if (!ext_bus)
        gb_die(errno);

    /* fill struct */
    ext_bus->read = _ext_bus_read;
    ext_bus->write = _ext_bus_write;
    ext_bus->ext_ram = ext_ram;
    ext_bus->cart = cart;

    return (bus_t *)ext_bus;
}

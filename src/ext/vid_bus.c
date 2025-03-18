#include "bus.h"
#include "log.h"
#include "types.h"

/* this represents a video bus */
typedef struct vid_bus {
    uint8_t (*read)(struct vid_bus *bus, uint16_t addr, bool cs);
    void    (*write)(struct vid_bus *bus, uint16_t addr, bool cs, uint8_t val);
    mem64_t *vram;
} vid_bus_t;

static uint8_t
_vid_bus_read(vid_bus_t *bus, uint16_t addr, bool cs)
{
    /* if the only chip is not selected (VRAM), error */
    if (cs) {
        LOG(LOG_ERR, "bad read issued to video bus");
        return 0xFF;
    }

    /* just return the vram data */
    return bus->vram->data[addr];
}

static void
_vid_bus_write(vid_bus_t *bus, uint16_t addr, bool cs, uint8_t val)
{
    /* if the only chip is not selected (VRAM), error */
    if (cs) {
        LOG(LOG_ERR, "bad write issued to video bus");
        return;
    }

    /* set value */
    bus->vram->data[addr] = val;
}

bus_t *
vid_bus_create(mem64_t *vram)
{
    /* try to allocate bus */
    vid_bus_t *vid_bus = malloc(sizeof(vid_bus_t));
    if (!vid_bus)
        gb_die(errno);

    /* fill struct */
    vid_bus->read = _vid_bus_read;
    vid_bus->write = _vid_bus_write;
    vid_bus->vram = vram;

    return (bus_t *)vid_bus;
}

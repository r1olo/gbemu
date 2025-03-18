#ifndef __BUS_H
#define __BUS_H

#include "cart.h"
#include "mem.h"

#include <stdbool.h>
#include <stdint.h>

/* bus_t struct:
 * this represents a generic 16-bit addressable bus with an 8-bit word. it
 * offers the methods read() and write(), which depend on the implementation.
 * 'cs' is the chip select parameter, which is sent alongside the read/write
 * signals */
typedef struct bus {
    uint8_t (*read)(struct bus *bus, uint16_t addr, bool cs);
    void    (*write)(struct bus *bus, uint16_t addr, bool cs, uint8_t val);
} bus_t;

/* ext_bus.c */
bus_t *ext_bus_create(mem64_t *ext_ram, cart_t *cart);

/* vid_bus.c */
bus_t *vid_bus_create(mem64_t *vram);

#endif /* __BUS_H */

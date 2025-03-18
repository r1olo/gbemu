#ifndef __MEM_H
#define __MEM_H

#include <stdint.h>

/* very simple representation of a memory array of 0x10000 bytes */
typedef struct mem64 {
    uint8_t data[0x10000];
} mem64_t;

static inline uint8_t
mem64_read(mem64_t *mem, uint16_t addr)
{
    /* very simple... */
    return mem->data[addr];
}

static inline void
mem64_write(mem64_t *mem, uint16_t addr, uint8_t val)
{
    /* also simple... */
    mem->data[addr] = val;
}

#endif /* __MEM_H */

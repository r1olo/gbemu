#include "gbemu.h"

byte
mem_readb(mem_t *mem, ushort addr)
{
    byte ret = 0xff;

    /* 0xC000 - 0xFDFF (Work RAM and Echo RAM) */
    if (IS_IN_RANGE(addr, 0xC000, 0xFDFF)) {
        /* if we're in Echo RAM (0xE000 and beyond), adjust address to normal
         * work RAM area */
        if (addr >= 0xE000)
            addr -= 0x2000;

        ret = mem->ram[addr - 0xC000];
    }

    /* 0xFF80 - 0xFFFE (High RAM) */
    else if (IS_IN_RANGE(addr, 0xFF80, 0xFFFE))
        ret = mem->hram[addr - 0xFF80];

    /* address out of bounds */
    else
        die("[mem_readb] address out of bounds");

    return ret;
}

void
mem_writeb(mem_t *mem, ushort addr, byte val)
{
    /* 0xC000 - 0xFDFF (Work RAM and Echo RAM) */
    if (IS_IN_RANGE(addr, 0xC000, 0xFDFF)) {
        /* same adjust */
        if (addr >= 0xE000)
            addr -= 0x2000;

        mem->ram[addr - 0xC000] = val;
    }

    /* 0xFF80 - 0xFFFE (High RAM) */
    else if (IS_IN_RANGE(addr, 0xFF80, 0xFFFE))
        mem->hram[addr - 0xFF80] = val;

    else
        die("[mem_writeb] address out of bounds");
}

void
mem_init(mem_t *mem, gb_t *bus)
{
    mem->bus = bus;
    mem->ram = malloc_or_die(0x2000, "mem_init", "ram");
    mem->hram = malloc_or_die(0x7F, "mem_init", "hram");
}

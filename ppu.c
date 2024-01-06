#include "gbemu.h"

byte
ppu_readb(ppu_t *ppu, ushort addr)
{
    /* disallow vram/oam access when in specific modes */
    return 0xff;
}

void
ppu_writeb(ppu_t *ppu, ushort addr, byte val)
{
    /* same thing */
}

void
ppu_init(ppu_t *ppu, gb_t *bus)
{
    /* TODO */
    ppu->bus = bus;
    ppu->vblank_intr = ppu->stat_intr = FALSE;
    ppu->vram = malloc_or_die(0x2000, "ppu_init", "vram");
    ppu->oam = malloc_or_die(0xA0, "ppu_init", "oam");
}

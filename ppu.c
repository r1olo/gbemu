#include "gbemu.h"

static byte
indirect_readb(ppu_t *ppu, ushort addr, BOOL conflict)
{
    /* disallow vram/oam access when in specific modes */
    return 0xff;
}

static void
indirect_writeb(ppu_t *ppu, ushort addr, byte val, BOOL conflict)
{
    /* same thing */
}

byte
ppu_readb(ppu_t *ppu, ushort addr)
{
    return indirect_readb(ppu, addr, TRUE);
}

void
ppu_writeb(ppu_t *ppu, ushort addr, byte val)
{
    indirect_writeb(ppu, addr, val, TRUE);
}

byte
ppu_dma_readb(ppu_t *ppu, ushort addr)
{
    /* DMA has full access */
    return indirect_readb(ppu, addr, FALSE);
}

void
ppu_dma_writeb(ppu_t *ppu, ushort addr, byte val)
{
    /* DMA has full access */
    indirect_writeb(ppu, addr, val, FALSE);
}

void
ppu_cycle(ppu_t *ppu)
{
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

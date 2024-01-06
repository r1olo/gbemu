#include "gbemu.h"

static byte
dma_mmu_readb(dma_t *dma, ushort addr)
{
    byte ret = 0xff;

    /* 0x0000 - 0x7FFF (cart) */
    if (IS_IN_RANGE(addr, 0x0000, 0x7FFF))
        ret = dma->bus->cart->read(dma->bus->cart, addr);

    /* 0x8000 - 0x9FFF (ppu) */
    else if (IS_IN_RANGE(addr, 0x8000, 0x9FFF)) {
        /* let's assume DMA has full access to PPU's VRAM */
        ret = ppu_dma_readb(dma->bus->ppu, addr);
    }

    /* 0xA000 - 0xBFFF (cart) */
    else if (IS_IN_RANGE(addr, 0xA000, 0xBFFF))
        ret = dma->bus->cart->read(dma->bus->cart, addr);

    /* 0xC000 - 0xDF96 (mem) */
    else if (IS_IN_RANGE(addr, 0xC000, 0xDFFF))
        ret = mem_readb(dma->bus->mem, addr);

    /* address out of bounds */
    else
        die("[dma_mmu_readb] address out of bounds");

    return ret;
}

static void
dma_mmu_writeb(dma_t *dma, ushort addr, byte val)
{
    /* 0xFE00 - 0xFE9F (oam) */
    if (IS_IN_RANGE(addr, 0xFE00, 0xFE9F)) {
        /* DMA has full access to PPU's OAM */
        ppu_dma_writeb(dma->bus->ppu, addr, val);
    }

    /* address out of bounds */
    else
        die("[dma_mmu_writeb] address out of bounds");
}

BOOL
dma_running(dma_t *dma)
{
    return dma->cycles < 160;
}

void
dma_start(dma_t *dma, byte src)
{
    if (dma_running(dma) || src > 0xDF)
        return;

    dma->cycles = 0;
    dma->src = src << 8;
}

void
dma_cycle(dma_t *dma)
{
    if (!dma_running(dma))
        return;

    byte val = dma_mmu_readb(dma, dma->src + dma->cycles);
    dma_mmu_writeb(dma, 0xFE00 + dma->cycles, val);
    dma->cycles++;
}

void
dma_init(dma_t *dma, gb_t *gb)
{
    dma->bus = gb;
    dma->cycles = 160;
    dma->src = 0;
}

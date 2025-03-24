#include "soc/soc.h"

static uint8_t
_dma_mem_read(dma_t *dma, uint16_t addr)
{
    /* 0x0000 - 0x7FFF - External bus, CS=1 (cart ROM) */
    if (addr < 0x8000)
        return soc_ext_bus_read(dma->soc, PRIO_DMA, addr, true);

    /* 0x8000 - 0x9FFF - Video bus, CS=0 */
    if (addr < 0xA000)
        return soc_vid_bus_read(dma->soc, PRIO_DMA, addr, false);

    /* 0xA000 - 0xFFFF - External bus, CS=0 (rams) */
    return soc_ext_bus_read(dma->soc, PRIO_DMA, addr, false);
}

void
dma_cycle(dma_t *dma)
{
    /* if the request cycles have executed, actually start the DMA. this
     * emulates the behavior where the DMA actually starts at the second machine
     * cycle after the instruction that issued it (8 clock cycles) */
    if (dma->requested && !(--dma->requested)) {
        dma->pending = 160;
        return;
    }

    /* return if not running */
    if (!dma->pending)
        return;

    /* waste the cycles first */
    WASTE_CYCLES(dma);

    /* calculate addresses */
    uint16_t mem_addr = (dma->high_addr << 8) + (160 - dma->pending);
    uint8_t oam_addr = 160 - dma->pending;

    /* read from mem */
    uint8_t val = _dma_mem_read(dma, mem_addr);

    /* write to OAM */
    soc_oam_write(dma->soc, PRIO_DMA, oam_addr, val);

    /* decrease pending cycles */
    --dma->pending;

    /* get ready for a new round of cycle wasting */
    dma->cycles_to_waste = 3;
}

void
dma_init(dma_t *dma, soc_t *soc)
{
    /* assign the SoC */
    dma->soc = soc;

    /* whoever left this variable uninitialized deserves... */
    dma->requested = 0;

    /* misc */
    dma->high_addr = 0;
    dma->pending = 0;
    dma->cycles_to_waste = 0;
}

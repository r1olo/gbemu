#include "soc/soc.h"
#include "log.h"

static inline bus_prio_t
_get_ext_bus_priority(soc_t *soc)
{
    /* DMA is running and is using external bus */
    if (soc->dma->pending && (soc->dma->high_addr < 0x80 ||
            soc->dma->high_addr > 0x9F))
        return PRIO_DMA;

    /* dma is using video bus */
    return PRIO_CPU;
}

static inline bus_prio_t
_get_vid_bus_priority(soc_t *soc)
{
    /* DMA is running and is using video bus */
    if (soc->dma->pending && soc->dma->high_addr >= 0x80 &&
            soc->dma->high_addr <= 0x9F)
        return PRIO_DMA;

    /* PPU is running and is using video bus */
    if (LCDC_PPU_ENABLE(soc->ppu->lcdc) && soc->ppu->mode == PPU_RENDER)
        return PRIO_PPU;

    /* video bus is free */
    return PRIO_CPU;
}

static inline bus_prio_t
_get_oam_bus_priority(soc_t *soc)
{
    /* dma is running, OAM is busy */
    if (soc->dma->pending)
        return PRIO_DMA;

    /* PPU is running and is using OAM */
    if (LCDC_PPU_ENABLE(soc->ppu->lcdc) &&
            (soc->ppu->mode == PPU_OAMSCAN || soc->ppu->mode == PPU_RENDER))
        return PRIO_PPU;

    /* OAM is free */
    return PRIO_CPU;
}

static inline void
_calculate_bus_priorities(soc_t *soc)
{
    soc->ext_prio = _get_ext_bus_priority(soc);
    soc->vid_prio = _get_vid_bus_priority(soc);
    soc->oam_prio = _get_oam_bus_priority(soc);
}

uint8_t
soc_ext_bus_read(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs)
{
    /* check priority */
    if (prio != soc->ext_prio) {
        LOG(LOG_ERR, "ext bus priority mismatch");
        return 0xFF;
    }

    /* invoke external bus */
    return soc->ext_bus->read(soc->ext_bus, addr, cs);
}

void
soc_ext_bus_write(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs, uint8_t val)
{
    /* check priority */
    if (prio != soc->ext_prio) {
        LOG(LOG_ERR, "ext bus priority mismatch");
        return;
    }

    /* invoke external bus */
    soc->ext_bus->write(soc->ext_bus, addr, cs, val);
}

uint8_t
soc_vid_bus_read(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs)
{
    /* check priority */
    if (prio != soc->vid_prio) {
        LOG(LOG_ERR, "vid bus priority mismatch");
        return 0xFF;
    }

    /* invoke video bus */
    return soc->video_bus->read(soc->video_bus, addr, cs);
}

void
soc_vid_bus_write(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs, uint8_t val)
{
    /* check priority */
    if (prio != soc->vid_prio) {
        LOG(LOG_ERR, "vid bus priority mismatch");
        return;
    }

    /* invoke video bus */
    soc->video_bus->write(soc->video_bus, addr, cs, val);
}

uint8_t
soc_oam_read(soc_t *soc, bus_prio_t prio, uint8_t addr)
{
    /* TODO: check for OAM bug */
    /* check priority */
    if (prio != soc->oam_prio) {
        LOG(LOG_ERR, "oam priority mismatch");
        return 0xFF;
    }

    /* read OAM */
    return soc->oam[addr];
}

void
soc_oam_write(soc_t *soc, bus_prio_t prio, uint8_t addr, uint8_t val)
{
    /* TODO: check for OAM bug */
    /* check priority */
    if (prio != soc->oam_prio) {
        LOG(LOG_ERR, "oam priority mismatch");
        return;
    }

    /* write OAM */
    soc->oam[addr] = val;
}

static inline uint8_t
_soc_iomem_read(soc_t *soc, uint8_t addr)
{
    /* do something based on address */
    uint8_t ret = 0xFF;
    switch (addr) {
        case 0x00:
            ret = jp_read(soc->jp);
            break;
        case 0x04:
            ret = (soc->tim->sys >> 8) & 0xFF;
            break;
        case 0x05:
            ret = soc->tim->tima;
            break;
        case 0x06:
            ret = soc->tim->tma;
            break;
        case 0x07:
            ret = soc->tim->tac;
            break;
        case 0x0F:
            ret = soc->cpu->iflag;
            break;
        case 0x40:
            ret = soc->ppu->lcdc;
            break;
        case 0x41:
            ret = ppu_get_stat(soc->ppu);
            break;
        case 0x42:
            ret = soc->ppu->scy;
            break;
        case 0x43:
            ret = soc->ppu->scx;
            break;
        case 0x44:
            ret = soc->ppu->ly;
            break;
        case 0x45:
            ret = soc->ppu->lyc;
            break;
        case 0x46:
            ret = soc->dma->high_addr;
            break;
        case 0x47:
            ret = soc->ppu->bgp;
            break;
        case 0x48:
            ret = soc->ppu->obp0;
            break;
        case 0x49:
            ret = soc->ppu->obp1;
            break;
        case 0xFF:
            ret = soc->cpu->ie;
            break;
        default:
            ret = 0xFF;
            LOG(LOG_ERR, "unhandled IO reg: 0x%X", addr);
            break;
    }
    
    return ret;
}

static inline void
_soc_iomem_write(soc_t *soc, uint8_t addr, uint8_t val)
{
    static char serial = 0;
    /* do something based on address */
    switch (addr) {
        case 0x00:
            jp_write(soc->jp, val);
            break;
        case 0x01:
            serial = val;
            break;
        case 0x02:
            if (val == 0x81)
                LOG(LOG_ERR, "serial output: %c", serial);
            break;
        case 0x04:
            tim_write_div(soc->tim);
            break;
        case 0x05:
            tim_write_tima(soc->tim, val);
            break;
        case 0x06:
            tim_write_tma(soc->tim, val);
            break;
        case 0x07:
            tim_write_tac(soc->tim, val);
            break;
        case 0x0F:
            soc->cpu->iflag = val | 0xE0;
            break;
        case 0x40:
            ppu_write_lcdc(soc->ppu, val);
            break;
        case 0x41:
            ppu_set_stat(soc->ppu, val);
            break;
        case 0x42:
            soc->ppu->scy = val;
            break;
        case 0x43:
            soc->ppu->scx = val;
            break;
        case 0x44:
            break;
        case 0x45:
            soc->ppu->lyc = val;
            break;
        case 0x46:
            dma_start(soc->dma, val);
            break;
        case 0x47:
            soc->ppu->bgp = val;
            break;
        case 0x48:
            soc->ppu->obp0 = val;
            break;
        case 0x49:
            soc->ppu->obp1 = val;
            break;
        case 0xFF:
            soc->cpu->ie = val;
            break;
        default:
            LOG(LOG_ERR, "unhandled IO reg: 0x%x", addr);
            break;
    }
}

static inline uint8_t
_soc_hram_read(soc_t *soc, uint8_t addr)
{
    /* TODO: is HRAM actually 128 bytes? */
    return soc->hram[addr];
}

static inline void
_soc_hram_write(soc_t *soc, uint8_t addr, uint8_t val)
{
    /* TODO: is HRAM actually 128 bytes? */
    soc->hram[addr] = val;
}

bool
soc_internal_read(soc_t *soc, uint8_t *dst, uint8_t addr)
{
    /* HRAM is FF80 and up, while IOMEM is FF00 to FF7F */
    if (addr & 0x80) {
        /* if we're reading from HRAM, we don't need to delay the read. we
         * return true */
        *dst = _soc_hram_read(soc, addr);
        return true;
    }

    /* make sure this is the only read in the cycle (not elegant) (don't care)
     * */
    assert(!soc->pending_io_read);
    soc->pending_io_read = true;
    soc->pending_dst = dst;
    soc->pending_addr = addr;
    return false;
}

void
soc_internal_write(soc_t *soc, uint8_t addr, uint8_t val)
{
    /* HRAM is FF80 and up, while IOMEM is FF00 to FF7F */
    if (addr & 0x80)
        _soc_hram_write(soc, addr, val);
    else
        _soc_iomem_write(soc, addr, val);
}

void
soc_cycle(soc_t *soc)
{
    /* TODO: at the end of the machine cycle, we must check the OAM bug. if the
     * CPU wrote to the bus and/or used the IDU, it will set the appropriate
     * flags here */

    /* first, calculate bus priorities for the current cycle */
    _calculate_bus_priorities(soc);

    /* there's a dependency problem. the CPU's read and write signals, along
     * with the bidirectional data bus, are connected to the other components.
     * all of these (CPU included) run in *parallel*, which makes it impossible
     * to pick a specific order in which to cycle the components.
     *
     * if the CPU reads a value from a chip, that chip must first do its clock
     * activities and then return the updated value to the CPU through the data
     * bus. likewise, if the CPU writes a value to a chip, the chip will first
     * read the CPU's value from the bus and then use that in its calculations.
     *
     * even if not all the I/O registers are wired this way, we can solve this
     * dependency problem with this method, given that (perhaps because of this
     * internal wiring) the CPU can only execute either a read or a write in a
     * single cycle and can't use the read value until next cycle:
     *
     *  - cycle the CPU and execute the I/O writes, but enqueue the I/O read for
     *    later
     *  - cycle the components
     *  - execute the enqueued I/O read from before
     *
     * in essence, this just delays the reads until the other stuff has run.
     */

    /* cycle the CPU and possibly enqueue a read */
    cpu_cycle(soc->cpu);

    /* cycle the other components */
    dma_cycle(soc->dma);
    ppu_cycle(soc->ppu);
    tim_cycle(soc->tim);
    jp_cycle(soc->jp);

    /* if there's a pending read from the CPU, fulfill it now */
    if (soc->pending_io_read) {
        assert(!(soc->pending_addr & 0x80));
        *soc->pending_dst = _soc_iomem_read(soc, soc->pending_addr);
        soc->pending_io_read = false;
    }
}

unsigned
soc_step(soc_t *soc)
{
    unsigned c = 4;
    for (size_t i = 0; i < 4; ++i) {
        soc_cycle(soc);
        ++c;
    }
    while (soc->cpu->state != FETCH) {
        for (size_t i = 0; i < 4; ++i) {
            soc_cycle(soc);
            ++c;
        }
    }
    return c;
}

unsigned
soc_run_until_vblank(soc_t *soc)
{
    /* total cycles */
    unsigned c = 0;

    /* if PPU is disabled, just step once (TODO: think) */
    if (!LCDC_PPU_ENABLE(soc->ppu->lcdc)) {
        for (size_t i = 0; i < 4560; ++i)
            c += soc_step(soc);

        return c;
    }

    /* otherwise, wait for LY = 144 */
    while (LCDC_PPU_ENABLE(soc->ppu->lcdc) && soc->ppu->ly == 144)
        c += soc_step(soc);
    while (LCDC_PPU_ENABLE(soc->ppu->lcdc) && soc->ppu->ly != 144)
        c += soc_step(soc);

    return c;
}

void
soc_run_one_frame(soc_t *soc)
{
    /* run for 70224 cycles */
    for (size_t i = 0; i < 70224; ++i)
        soc_step(soc);
}

soc_t *
soc_create(bus_t *ext_bus, bus_t *video_bus)
{
    /* main SoC */
    soc_t *soc = malloc(sizeof(soc_t));
    if (!soc)
        goto failure;

    /* DMA controller */
    soc->dma = malloc(sizeof(dma_t));
    if (!soc->dma)
        goto soc_free;
    dma_init(soc->dma, soc);

    /* SM83 core */
    soc->cpu = malloc(sizeof(cpu_t));
    if (!soc->cpu)
        goto dma_free;
    cpu_init(soc->cpu, soc);

    /* the PPU */
    soc->ppu = malloc(sizeof(ppu_t));
    if (!soc->ppu)
        goto cpu_free;
    ppu_init(soc->ppu, soc);

    /* the timer */
    soc->tim = malloc(sizeof(tim_t));
    if (!soc->tim)
        goto ppu_free;
    tim_init(soc->tim, soc);

    /* the joypad */
    soc->jp = malloc(sizeof(jp_t));
    if (!soc->jp)
        goto tim_free;
    jp_init(soc->jp, soc);

    /* init variables */
    soc->ext_bus = ext_bus;
    soc->video_bus = video_bus;
    soc->ext_prio = soc->vid_prio = soc->oam_prio = PRIO_CPU;
    memset(soc->oam, 0, sizeof(soc->oam));
    memset(soc->hram, 0, sizeof(soc->hram));
    soc->pending_io_read = false;

    return soc;

tim_free:
    free(soc->tim);

ppu_free:
    free(soc->ppu);

cpu_free:
    free(soc->cpu);

dma_free:
    free(soc->dma);

soc_free:
    free(soc);

failure:
    return NULL;
}

void
soc_destroy(soc_t *soc)
{
    /* free components one by one */
    free(soc->jp);
    free(soc->tim);
    free(soc->ppu);
    free(soc->cpu);
    free(soc->dma);
    free(soc);
}

#include "soc/soc.h"
#include "log.h"

#define WASTE_CYCLES() \
    if (ppu->cycles_to_waste) { \
        --ppu->cycles_to_waste; \
        return; \
    }

#ifndef GREEN
#define WHITE       0xFFFFFFFF
#define LIGHT_GRAY  0xD3D3D3FF
#define DARK_GRAY   0xA9A9A9FF
#define BLACK       0x000000FF
#else
#define WHITE       0x9BBC0FFF
#define LIGHT_GRAY  0x8BAC0FFF
#define DARK_GRAY   0x306230FF
#define BLACK       0x0F380FFF
#endif

static inline void
_check_lyc_stat_interrupt(ppu_t *ppu)
{
    if (ppu->lyc_int && ppu->ly == ppu->lyc)
        ppu->stat_lyc = true;
    else
        ppu->stat_lyc = false;
}

static inline pixel_t
_get_pixel_with_palette_and_color_id(uint8_t palette, uint8_t color_id)
{
    assert(color_id < 4);
    pixel_t ret;
    uint8_t color = (palette >> (color_id * 2)) & 0x03;
    switch (color) {
        case 0x00:
            ret.val = WHITE;
            break;
        case 0x01:
            ret.val = LIGHT_GRAY;
            break;
        case 0x02:
            ret.val = DARK_GRAY;
            break;
        case 0x03:
            ret.val = BLACK;
            break;
        default:
            assert(false);
    }

    return ret;
}

static void
extract_color_ids_from_bitplane(uint8_t *row, uint8_t low, uint8_t high)
{
    for (size_t i = 0; i < 8; ++i) {
        size_t bit_idx = 7 - i;
        uint8_t low_bit = (low >> bit_idx) & 0x01;
        uint8_t high_bit = (high >> bit_idx) & 0x01;
        row[i] = (high_bit << 1) | low_bit;
        LOG(LOG_VERBOSE, "extracted color ID: 0x%02X2", row[i]);
    }
}

static inline bool
_is_bg_queue_empty(ppu_t *ppu)
{
    return ppu->bg_queue_idx >= 8;
}

static inline void
_shift_obj_queues(ppu_t *ppu)
{
    /* shift all pixels to the left and put 0x00 as the last byte */
    for (size_t i = 1; i < 8; ++i)
        ppu->obj_queue[i - 1] = ppu->obj_queue[i];
    ppu->obj_queue[7] = 0x00;

    /* shift the obj attributes also */
    for (size_t i = 1; i < 8; ++i)
        ppu->obj_attrs[i - 1] = ppu->obj_attrs[i];
    ppu->obj_attrs[7] = 0x00;
}

static inline void
_go_to_oamscan(ppu_t *ppu)
{
    /* get ready for OAMSCAN */
    ppu->mode = PPU_OAMSCAN;
    ppu->cycles_to_waste = 1;
    ppu->cur_oam_idx = 0;
    ppu->cur_objs = 0;

    /* fire the STAT interrupt if enabled */
    if (ppu->oam_int)
        ppu->stat_mode = true;
    else
        ppu->stat_mode = false;
}

static inline void
_go_to_render(ppu_t *ppu)
{
    /* get ready for RENDER */
    ppu->mode = PPU_RENDER;
    ppu->cycles_to_waste = 1;
    ppu->fetcher_mode = PPU_FETCHER_FETCH;
    ppu->sprite_fetch = false;
    ppu->cur_fetched_obj = 0;
    ppu->next_obj_to_check = 0;
    ppu->bg_queue_idx = 8;
    ppu->tmp_reg_full = false;
    ppu->lx = 0;
    ppu->fetcher_x = -1;
    ppu->sprite_hit = false;
    ppu->render_cycles = 0;

    /* init obj queue with transparent object pixels */
    for (size_t i = 0; i < 8; ++i)
        ppu->obj_queue[i] = 0x00;

    /* init obj attribute queue with null attributes */
    for (size_t i = 0; i < 8; ++i)
        ppu->obj_attrs[i] = 0x00;
}

static inline void
_go_to_hblank(ppu_t *ppu)
{
    /* get ready for HBLANK */
    ppu->mode = PPU_HBLANK;
    ppu->cycles_to_waste = 375 - ppu->render_cycles;

    /* fire the STAT interrupt if enabled */
    if (ppu->hblank_int)
        ppu->stat_mode = true;
    else
        ppu->stat_mode = false;
}

static inline void
_go_to_vblank(ppu_t *ppu)
{
    /* get ready for VBLANK */
    ppu->mode = PPU_VBLANK;
    ppu->cycles_to_waste = 455;

    /* fire the normal VBLANK interrupt */
    soc_interrupt(ppu->soc, INT_VBLANK);

    /* fire the STAT interrupt if it's been enabled */
    if (ppu->vblank_int)
        ppu->stat_mode = true;
    else
        ppu->stat_mode = false;
}

static void
_ppu_vblank(ppu_t *ppu)
{
    /* if there are more cycles to waste, do it and return */
    WASTE_CYCLES();

    /* if LY goes to 154 go to OAMSCAN, otherwise wait another round */
    if (++ppu->ly > 153) {
        ppu->ly = 0;
        _check_lyc_stat_interrupt(ppu);
        _go_to_oamscan(ppu);
    } else {
        ppu->cycles_to_waste = 455;
    }
}

static void
_ppu_hblank(ppu_t *ppu)
{
    /* if there are more cycles to waste, do it and return */
    WASTE_CYCLES();

    /* get ready for either another round of OAMSCAN or VBLANK */
    if (++ppu->ly > 143) {
        _check_lyc_stat_interrupt(ppu);
        _go_to_vblank(ppu);
    } else {
        _go_to_oamscan(ppu);
    }
}

static void
_ppu_oamscan(ppu_t *ppu)
{
    /* if there are more cycles to waste, do it and return */
    WASTE_CYCLES();

    /* TODO: two bus reads in one cycle. this should be split in two different
     * cycles */

    /* current OAM address */
    uint8_t cur_addr = ppu->cur_oam_idx * 4;
    assert(cur_addr < 0xA0);

    /* object's Y position (sprite Y - 16), must be signed */
    int obj_y_pos = soc_oam_read(ppu->soc, PRIO_PPU, cur_addr) - 16;

    /* get current obj size (LCDC.2) */
    int obj_size = LCDC_OBJ_SIZE(ppu->lcdc) ? 16 : 8;

    /* if we can add another object AND the object is within our reach, add it
     * */
    if (ppu->cur_objs < 10 && ppu->ly >= obj_y_pos &&
            ppu->ly < obj_y_pos + obj_size) {
        /* the X coordinate is the obj's X coord - 8 */
        ppu->objs[ppu->cur_objs].x_pos =
            soc_oam_read(ppu->soc, PRIO_PPU, cur_addr + 1);

        /* the object's OAM index */
        ppu->objs[ppu->cur_objs].obj_idx = ppu->cur_oam_idx;

        /* the object's tile row (current scanline - first obj Y line) */
        ppu->objs[ppu->cur_objs].tile_row = ppu->ly - obj_y_pos;

        ++ppu->cur_objs;
    }

    /* if cur_oam_idx gets to 40 exit OAMSCAN and enter RENDER, else reiterate
     * process but waste 1 dot */
    if (++ppu->cur_oam_idx > 39) {
        _go_to_render(ppu);
    } else {
        ppu->cycles_to_waste = 1;
    }
}

static bool
_try_fill_bg_queue(ppu_t *ppu)
{
    /* if tmp register is not full OR queue is not empty, return */
    if (!_is_bg_queue_empty(ppu) || !ppu->tmp_reg_full)
        return false;

    /* set BG queue */
    for (size_t i = 0; i < 8; ++i)
        ppu->bg_queue[i] = ppu->tmp_reg[i];

    /* BG queue is now not empty and tmp reg is not full */
    ppu->bg_queue_idx = 0;
    ppu->tmp_reg_full = false;

    return true;
}

static inline void
_try_restart_bg_fetch(ppu_t *ppu)
{
    /* if we can fill BG queue, increase fetcher X and go to fetch mode */
    ppu->fetcher_mode = PPU_FETCHER_SLEEP;
    if (_try_fill_bg_queue(ppu)) {
        LOG(LOG_VERBOSE, "bg queue refilled, going back to fetch mode");
        ++ppu->fetcher_x;
        ppu->fetcher_mode = PPU_FETCHER_FETCH;
        ppu->cycles_to_waste = 1;
    } else {
        LOG(LOG_VERBOSE, "bg queue is full, sleeping before pushing");
    }
}

static inline void
_fetch_bg_tile_id(ppu_t *ppu)
{
    /* discard the first 8 tiles */
    uint fetcher_x = ppu->fetcher_x < 0 ? 0 : ppu->fetcher_x;

    /* calculate tile map address */
    uint16_t addr = LCDC_BG_TILEMAP(ppu->lcdc) ? 0x9C00 : 0x9800;
    addr |= (((ppu->ly + ppu->scy) / 8) & 0x1F) << 5;
    addr |= ((ppu->scx / 8) + fetcher_x) & 0x1F;

    /* extract tile ID */
    ppu->cur_tile_id = soc_vid_bus_read(ppu->soc, PRIO_PPU, addr, false);
    LOG(LOG_VERBOSE, "reading tile ID from addr: 0x%04X (tile ID %d)", addr,
            ppu->cur_tile_id);
}

static void
_ppu_fetcher_fetch(ppu_t *ppu)
{
    /* waste cycles if needed */
    WASTE_CYCLES();

    /* fetch tile ID */
    if (ppu->sprite_fetch) {
        /* fetch sprite tile ID and attributes (TODO: 2 mem reads!!) */
        obj_store_entry_t *obj = &ppu->objs[ppu->cur_fetched_obj];
        ppu->cur_tile_id = soc_oam_read(ppu->soc, PRIO_PPU,
                obj->obj_idx * 4 + 2);
        ppu->cur_fetched_obj_attrs = soc_oam_read(ppu->soc, PRIO_PPU,
                obj->obj_idx * 4 + 3);
    } else {
        /* fetch BG tile ID */
        _fetch_bg_tile_id(ppu);
    }

    /* advance to next step */
    ppu->cycles_to_waste = 1;
    ppu->fetcher_mode = PPU_FETCHER_TILE_LOW;
}

static inline uint16_t
_get_bg_tile_address(ppu_t *ppu)
{
    /* formulate the tile address */
    uint16_t addr = 0x8000;

    /* bit 12 depends on the addressing mode. if we are in 0x8000 mode, it is
     * always set. otherwise, if we are in 0x8800 mode, it is the negation of
     * tile ID's bit 7 */
    addr |=
        (!((LCDC_BGWIN_TILES(ppu->lcdc) || (ppu->cur_tile_id & 0x80)))) << 12;

    /* put the tile ID in the address */
    addr |= ppu->cur_tile_id << 4;

    /* put the tile's row (Y coord) in the address */
    addr |= ((ppu->ly + ppu->scy) & 0x07) << 1;

    return addr;
}

static inline uint16_t
_get_obj_tile_address(ppu_t *ppu)
{
    /* formulate the tile address */
    uint16_t addr = 0x8000;

    /* put the tile ID in the address */
    addr |= ppu->cur_tile_id << 4;

    /* put the obj's Y row in the address */
    addr |= (ppu->objs[ppu->cur_fetched_obj].tile_row & 0x07) << 1;

    return addr;
}

static void
_ppu_fetcher_tile_low(ppu_t *ppu)
{
    /* waste cycles if needed */
    WASTE_CYCLES();

    /* formulate the tile address */
    uint16_t addr = _get_bg_tile_address(ppu);

    /* get low tile */
    ppu->cur_tile_low = soc_vid_bus_read(ppu->soc, PRIO_PPU, addr, false);
    LOG(LOG_VERBOSE, "getting low tile from addr 0x%04X (tile data: 0x%02X)",
            addr, ppu->cur_tile_low);

    /* advance to next step */
    ppu->cycles_to_waste = 1;
    ppu->fetcher_mode = PPU_FETCHER_TILE_HIGH;
}

static void
_merge_into_obj_queue(ppu_t *ppu)
{
    uint8_t cur_obj_pixels[8];
    extract_color_ids_from_bitplane(cur_obj_pixels, ppu->cur_tile_low,
            ppu->cur_tile_high);

    /* merge only if the currently queued pixel is transparent (color ID: 0) */
    for (size_t i = 0; i < 8; ++i) {
        if (ppu->obj_queue[i] == 0) {
            ppu->obj_queue[i] = cur_obj_pixels[i];
            ppu->obj_attrs[i] = ppu->cur_fetched_obj_attrs;
        }
    }
}

static void
_ppu_fetcher_tile_high_and_push(ppu_t *ppu)
{
    /* waste cycles if needed */
    WASTE_CYCLES();

    /* formulate the tile address */
    uint16_t addr = _get_bg_tile_address(ppu);

    /* this is the high tile, so bit 0 must be 1 */
    addr |= 0x01;

    /* get low tile */
    ppu->cur_tile_high = soc_vid_bus_read(ppu->soc, PRIO_PPU, addr, false);
    LOG(LOG_VERBOSE, "getting high tile from addr 0x%04X (tile data: 0x%02X)",
            addr, ppu->cur_tile_high);

    /* try to push */
    if (ppu->sprite_fetch) {
        /* MERGE SPRITE INTO OBJ QUEUE */
        _merge_into_obj_queue(ppu);

        /* sprite fetch is over, we can keep going */
        ppu->sprite_fetch = ppu->sprite_hit = false;
        ppu->fetcher_mode = PPU_FETCHER_SLEEP;
    } else {
        /* put the pixels in the tmp register */
        extract_color_ids_from_bitplane(ppu->tmp_reg, ppu->cur_tile_low,
                ppu->cur_tile_high);
        ppu->tmp_reg_full = true;

        /* for a succesful fill, fetcher goes back immediately to FETCH after
         * increasing its current tile number, otherwise it will stay stuck in
         * SLEEP */
        _try_restart_bg_fetch(ppu);
    }
}

static void
_ppu_fetcher_sleep(ppu_t *ppu)
{
    /* if we end up here in a sprite fetch state, there was a bug */
    assert(!ppu->sprite_fetch);

    /* try filling bg queue, otherwise keep sleep mode */
    _try_restart_bg_fetch(ppu);
}

static void
_ppu_fetcher(ppu_t *ppu)
{
    switch (ppu->fetcher_mode) {
        case PPU_FETCHER_FETCH:
            _ppu_fetcher_fetch(ppu);
            break;
        case PPU_FETCHER_TILE_LOW:
            _ppu_fetcher_tile_low(ppu);
            break;
        case PPU_FETCHER_TILE_HIGH:
            _ppu_fetcher_tile_high_and_push(ppu);
            break;
        case PPU_FETCHER_SLEEP:
            _ppu_fetcher_sleep(ppu);
            break;
        default:
            assert(false);
    }
}

static void
_ppu_pusher(ppu_t *ppu)
{
    /* white default pixel */
    pixel_t px;
    px.val = 0xFFFFFFFF;

    /* try to get BG pixel (queue is always clocked forward) */
    uint8_t bg_color = ppu->bg_queue[ppu->bg_queue_idx++];
    if (LCDC_BGWIN_ENABLE(ppu->lcdc)) {
        px = _get_pixel_with_palette_and_color_id(ppu->bgp, bg_color);
    }

    /* extract pixel from obj queue if sprites are enabled. TODO: is obj queue
     * shifted even if disabled? */
    if (LCDC_OBJ_ENABLE(ppu->lcdc)) {
        uint8_t obj_color = ppu->obj_queue[0];
        uint8_t obj_attrs = ppu->obj_attrs[0];
        _shift_obj_queues(ppu);
        if (!(OBJ_ATTR_PRIORITY(obj_attrs) && bg_color == 0) && obj_color != 0) {
            uint8_t obj_palette = OBJ_ATTR_PALETTE(obj_attrs) ? ppu->obp1 : ppu->obp0;
            px = _get_pixel_with_palette_and_color_id(obj_palette, obj_color);
        }
    }

    /* actually push pixel if lx >= 8 */
    if (ppu->lx >= 8)
        ppu->screen[ppu->ly][ppu->lx - 8] = px;

    /* increase LX */
    ++ppu->lx;

    /* reset the next object to fetch, since we are in a new X coord */
    ppu->next_obj_to_check = 0;
}

static void
_ppu_render(ppu_t *ppu)
{
    /* Pusher/LCD is clocked when:
     *      - BG queue is not empty
     *      - Sprite is not hit
     *      - LX is greater or equal than 8 (actually the pusher is clocked if
     *        LX < 8, except the LCD is not - so called offscreen pixel push)
     */
    if (!_is_bg_queue_empty(ppu) && !ppu->sprite_hit)
        _ppu_pusher(ppu);

    /* call the fetcher */
    _ppu_fetcher(ppu);

    /* check for sprites in the current X value */
    for (size_t i = ppu->next_obj_to_check; i < ppu->cur_objs; ++i) {
        if (ppu->objs[i].x_pos == ppu->lx) {
            ppu->cur_fetched_obj = i;
            ppu->next_obj_to_check = i + 1;
            ppu->sprite_hit = true;
            break;
        }
    }

    /* if we hit a sprite and the pixel fetcher is free, we occupy it */
    if (ppu->sprite_hit && ppu->fetcher_mode == PPU_FETCHER_SLEEP) {
        ppu->fetcher_mode = PPU_FETCHER_FETCH;
        ppu->sprite_fetch = true;
    }

    /* increase render cycles */
    ++ppu->render_cycles;

    /* if LX goes to 168, we can start the HBLANK phase */
    if (ppu->lx > 167)
        _go_to_hblank(ppu);
}

void
ppu_cycle(ppu_t *ppu)
{
    /* if LCD is off, prepare for when it is turned on by setting LY = 0 and
     * OAMSCAN. exit immediately (don't use _go_to_oamscan since we don't want
     * to fire interrupts) */
    if (!LCDC_PPU_ENABLE(ppu->lcdc)) {
        ppu->ly = 0;
        ppu->mode = PPU_OAMSCAN;
        ppu->cycles_to_waste = 1;
        ppu->cur_oam_idx = 0;
        ppu->cur_objs = 0;
        return;
    }

    /* sample the current STAT line status (before stepping through the PPU) */
    bool old_stat = ppu->stat_mode || ppu->stat_lyc;

    /* see the status */
    switch (ppu->mode) {
        case PPU_HBLANK:
            _ppu_hblank(ppu);
            break;
        case PPU_VBLANK:
            _ppu_vblank(ppu);
            break;
        case PPU_OAMSCAN:
            _ppu_oamscan(ppu);
            break;
        case PPU_RENDER:
            _ppu_render(ppu);
            break;
        default:
            assert(false);
    }

    /* check if STAT went high (we only interrupt on the rising edge) */
    bool stat = ppu->stat_mode || ppu->stat_lyc;

    /* fire a STAT interrupt if needed */
    if (!old_stat && stat)
        soc_interrupt(ppu->soc, INT_STAT);
}

void
ppu_init(ppu_t *ppu, soc_t *soc)
{
    /* setup SoC */
    ppu->soc = soc;

    /* first value of LCDC */
    ppu->lcdc = 0x91;

    /* PPU starts operating at VBLANK (1 line after) */
    ppu->mode = PPU_VBLANK;
    ppu->cycles_to_waste = 455;
    ppu->ly = 145;

    /* set comparison register */
    ppu->lyc = 0;

    /* the initial viewport is 0x0 */
    ppu->scx = ppu->scy = 0;

    /* initial palette is 0xFC */
    ppu->bgp = 0xFC;

    /* initial objects' palettes are uninitialized, but we're setting them to
     * 0xFF anyway */
    ppu->obp0 = ppu->obp1 = 0xFF;

    /* initial window's position is 0 */
    ppu->wx = ppu->wy = 0;

    /* set initial interrupt sources */
    ppu->lyc_int = ppu->oam_int = ppu->vblank_int = ppu->hblank_int = false;

    /* fill the screen with white pixels */
    memset(ppu->screen, 0xFF, SCREEN_HEIGHT * SCREEN_WIDTH);

    /* initially, the two STAT sources are off */
    ppu->stat_mode = ppu->stat_lyc = false;
}

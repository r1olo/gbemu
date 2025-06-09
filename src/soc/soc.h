#ifndef __SOC_H
#define __SOC_H

#include "ext/bus.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* the Game Boy's screen */
#define SCREEN_WIDTH    160
#define SCREEN_HEIGHT   144

/* interrupt masks for the CPU */
#define INT_VBLANK  (BIT(0))
#define INT_STAT    (BIT(1))
#define INT_TIMER   (BIT(2))
#define INT_SERIAL  (BIT(3))
#define INT_JP      (BIT(4))

/* quick macro to waste cycles */
#define WASTE_CYCLES(x)         \
    if ((x)->cycles_to_waste) { \
        --(x)->cycles_to_waste; \
        return;                 \
    }

/* this represents the bus's status for the current cycle */
typedef enum bus_prio {
    PRIO_DMA,
    PRIO_PPU,
    PRIO_CPU,
} bus_prio_t;

/* pre-define structs because of circular dependencies */
struct dma;
struct cpu;
struct ppu;
struct tim;
struct soc;
struct jp;

/* some joypad definitions */
#define JP_ACTION(x)        (((x) & 0x20) >> 5)
#define JP_DPAD(x)          (((x) & 0x10) >> 4)
#define JP_START_DOWN(x)    (((x) & 0x08) >> 3)
#define JP_SELECT_UP(x)     (((x) & 0x04) >> 2)
#define JP_B_LEFT(x)        (((x) & 0x02) >> 1)
#define JP_A_RIGHT(x)       ((x) & 0x01)

/* the joypad. this is relatively simple */
typedef struct jp {
    /* the controlling SoC */
    struct soc *soc;

    /* what is currently selected */
    bool action;
    bool dpad;

    /* the buttons' current status */
    bool start, select, a, b;
    bool down, up, left, right;

    /* whether the buttons have been pressed asyncrhonously */
    bool start_pressed, select_pressed, a_pressed, b_pressed;
    bool down_pressed, up_pressed, left_pressed, right_pressed;
} jp_t;

/* TAC (Timer Control) fields */
#define TAC_CLOCK(x)        ((x) & 0x03)
#define TAC_ENABLE(x)       (((x) & 0x04) >> 2)

/* the timer. as useless as it sounds, this component is actually very
 * important and is relied on by the majority of games. it's called tim and not
 * timer because timer_t apparently already exists in C somewhere */
typedef struct tim {
    /* pointer to the controlling SoC */
    struct soc *soc;

    /* the global sys timer. this is only 14 bits and always increments every
     * M-cycle. the DIV register starts from bit 6 of the SYS timer */
    uint16_t sys;

    /* TIMA, TMA and TAC registers */
    uint8_t tima, tma, tac;

    /* the remaining cycles of "overflow status", where TIMA is supposedly 0
     * before it is loaded with the value from TMA */
    unsigned overflow;

    /* whether the CPU tried to write to the DIV register */
    bool div_write;

    /* whether the CPU wrote something to TIMA (and what) */
    bool tima_write;
    uint8_t tima_write_data;

    /* whether TIMA writes are being ignored. this occurs for a entire machine
     * cycle after TIMA was reset to TMA after an overflow */
    unsigned tima_writes_ignored;

    /* this is the old TAC register, to allow for increasing TIMA when changing
     * the TAC clock in the same cycle */
    uint8_t old_tac;
} tim_t;

/* the DMA controller. the address translation mechanism for this controller is
 * different from the CPU. it can only read from either the external bus or the
 * video bus, even if the highest addresses should not be mapped externally.
 * (e.g. 0xFF00 is a CPU register, but the DMA reads it from the external bus
 * nevertheless) */
typedef struct dma {
    /* pointer to the controlling SoC */
    struct soc *soc;

    /* whether a DMA transfer has been requested by CPU. this is the amount of
     * cycles before the DMA is actually engaged */
    unsigned requested;

    /* the DMA pending cycles */
    unsigned pending;

    /* the cycles to waste */
    unsigned cycles_to_waste;

    /* higher byte address */
    uint8_t high_addr;
} dma_t;

/* quickly get the OAM entry flags */
#define OAM_ENTRY_PALETTE(x)    (((x) & 0x10) >> 4)
#define OAM_ENTRY_X_FLIP(x)     (((x) & 0x20) >> 5)
#define OAM_ENTRY_Y_FLIP(x)     (((x) & 0x40) >> 6)
#define OAM_ENTRY_PRIORITY(x)   (((x) & 0x80) >> 7)

/* this represents an object entry in the sprite store PLUS the X coordinate. in
 * the Game Boy we have a 100 bit long store which has 10 bits per object.
 * within these 10 bits, the first 6 are used for the object's OAM index (0-39),
 * while the remaining 4 are used for the tile row (0-15).
 *
 * the (signed) X coordinate is not in the sprite store, but in some other
 * hardware matchers. we can simplify here as it doesn't make any difference */
typedef struct obj_store_entry {
    /* object's X pos */
    unsigned x_pos;

    /* object's tile row */
    unsigned tile_row;

    /* object's index (0-39) */
    unsigned obj_idx;
} obj_store_entry_t;

/* the various PPU modes */
enum ppu_mode {
    PPU_HBLANK,
    PPU_VBLANK,
    PPU_OAMSCAN,
    PPU_RENDER,
};

/* the different PPU fetcher modes
 *      - BG Fetch / Sprite Fetch
 *      - Get tile low
 *      - Get tile high
 *      - Sleep while result is not pushed into FIFO */
enum ppu_fetcher_mode {
    PPU_FETCHER_FETCH,
    PPU_FETCHER_TILE_LOW,
    PPU_FETCHER_TILE_HIGH_PUSH,
    PPU_FETCHER_SLEEP,
};

/* access various fields of the LCDC register */
#define LCDC_BGWIN_ENABLE(x)    ((x) & 0x01)
#define LCDC_OBJ_ENABLE(x)      (((x) & 0x02) >> 1)
#define LCDC_OBJ_SIZE(x)        (((x) & 0x04) >> 2)
#define LCDC_BG_TILEMAP(x)      (((x) & 0x08) >> 3)
#define LCDC_BGWIN_TILES(x)     (((x) & 0x10) >> 4)
#define LCDC_WIN_ENABLE(X)      (((x) & 0x20) >> 5)
#define LCDC_WIN_TILEMAP(x)     (((x) & 0x40) >> 6)
#define LCDC_PPU_ENABLE(x)      (((x) & 0x80) >> 7)

/* various object attributes */
#define OBJ_ATTR_PALETTE(x)     (((x) & 0x10) >> 4)
#define OBJ_ATTR_X_FLIP(x)      (((x) & 0x20) >> 5)
#define OBJ_ATTR_Y_FLIP(x)      (((x) & 0x40) >> 6)
#define OBJ_ATTR_PRIORITY(x)    (((x) & 0x80) >> 7)

/* the Pixel Processing Unit. perhaps the most complicated component of them
 * all, the PPU is repsonsible for displaying pixels to the screen! */
typedef struct ppu {
    /* pointer to the controlling SoC */
    struct soc *soc;

    /* the current PPU mode */
    enum ppu_mode mode;

    /* the control register */
    uint8_t lcdc;

    /* remaining dots to waste */
    unsigned cycles_to_waste;

    /* the current LY value (for the current scanline, read next_ly below) */
    uint8_t ly;

    /* the LYC comparison register */
    uint8_t lyc;

    /* the viewport */
    uint8_t scx, scy;

    /* the background palette */
    uint8_t bgp;

    /* the objects' palettes */
    uint8_t obp0, obp1;

    /* the window's position */
    uint8_t wx, wy;

    /* sources of interrupts */
    bool lyc_int_enabled, oam_int_enabled,
         vblank_int_enabled, hblank_int_enabled;

    /* sprite store with X coords (matchers) */
    obj_store_entry_t objs[10];

    /* current index in the OAM */
    size_t cur_oam_idx;

    /* how many objects are in the current scanline */
    size_t cur_objs;

    /* fetcher mode */
    enum ppu_fetcher_mode fetcher_mode;

    /* whether the fetcher is busy fetching a sprite */
    bool sprite_fetch;

    /* the current object index being fetched */
    size_t cur_fetched_obj;

    /* the current object pixel attributes */
    uint8_t cur_fetched_obj_attrs;

    /* next object index to check (in case of multiple sprites in the same X
     * coordinate) */
    size_t next_obj_to_check;

    /* the BG queue */
    uint8_t bg_queue[8];

    /* the object queues with attributes */
    uint8_t obj_queue[8], obj_attrs[8];

    /* the current head in the BG queue (8 means it's empty). the OBJ queue has
     * no index because it works by forever "shifting" 0x00 bytes if it is empty
     */
    size_t bg_queue_idx;

    /* whether the temp queue register is full and ready to fill BG queue */
    bool tmp_reg_full;

    /* the temporary BG queue register */
    uint8_t tmp_reg[8];

    /* current pusher X value (which pixel is being pushed) */
    unsigned lx;

    /* the BG fetcher's current tile */
    int fetcher_x;

    /* currently fetched tile ID */
    uint8_t cur_tile_id;

    /* currently fetched tile low and high bytes */
    uint8_t cur_tile_low, cur_tile_high;

    /* whether a sprite has been hit in this LX value */
    bool sprite_hit;

    /* how many cycles the RENDER phase took */
    unsigned render_cycles;

    /* the actual screen */
    pixel_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];

    /* the STAT sources. these two are ORed together into a single line which
     * interrupts the CPU if it goes high from low (STAT blocking) */
    bool stat_mode, stat_lyc;

    /* whether the STAT register has been written to. this means that the STAT
     * line automagically goes ON for one cycle, then resets back to what it
     * should actually be */
    bool stat_written;
} ppu_t;

/* the possible CPU states */
enum cpu_state {
    FETCH, M2, M3, M4, M5, M6, M7
};

/* the possible EI instruction states */
enum ei_state {
    EI_NOT_CALLED,
    EI_CALLED,
    EI_SET
};

/* single instruction step function */
typedef void (*instr_fn)(struct cpu *cpu);

/* include the name for debugging purposes */
typedef struct fn_row {
    const char *name;
    instr_fn fn;
} fn_row_t;

/* have an array of rows (to be declared statically) */
typedef fn_row_t fn_row_array[];

/* have an array of lists (to be declared statically) */
typedef fn_row_t (*fn_row_array_ptrs[])[256];

/* accessible types */
typedef fn_row_t *fn_list_t;
typedef fn_list_t *instructions_t;

/* the SoC's pseudo-SM83 core. it is "pseudo" because it is probably modified by
 * Nintendo, but the inner workings seem to match the original Sharp's SM83 */
typedef struct cpu {
    /* pointer to the controlling SoC */
    struct soc *soc;

    /* instruction register */
    uint8_t ir;

    /* interrupt enable */
    uint8_t ie;

    /* interrupt flag */
    uint8_t iflag;

    /* interrupt master enable */
    bool ime;

    /* the current EI instruction state */
    enum ei_state ei_state;

    /* register set */
    reg_t af, bc, de, hl;

    /* program counter and stack pointer */
    reg_t pc, sp;

    /* the wz register (temporary values) */
    reg_t wz;

    /* the current instruction list */
    fn_list_t curlist;

    /* the current state */
    enum cpu_state state;

    /* the current instruction PC for debugging purposes */
    reg_t curpc;

    /* pending dots. the memory accesses are located in the two middle 4MHz
     * dots, but that's kind of irrelevant */
    unsigned cycles_to_waste;

    /* the HALT logic */
    bool halt, halt_bug;
} cpu_t;

/* the main system-on-chip structure. the DMG-CPU-B can roughly be divided in
 * the following components:
 *      - CPU (SM83 core)
 *      - DMA controller
 *      - PPU
 *      - Audio controller
 *      - Timer
 *      - Input controller
 *      - Serial controller
 *      - Internal bus with priority (OAM)
 *      - External and video buses with priority
 */
typedef struct soc {
    /* the DMA controller */
    struct dma *dma;

    /* the SM83 core */
    struct cpu *cpu;

    /* the PPU */
    struct ppu *ppu;

    /* the timer */
    struct tim *tim;

    /* the joypad */
    struct jp *jp;

    /* the priorities for the current cycle */
    bus_prio_t ext_prio, vid_prio, oam_prio;

    /* the external bus */
    bus_t *ext_bus;

    /* the video bus */
    bus_t *video_bus;

    /* OAM memory (it's actually 2*128B SRAM chips, even if only the first 160
     * bytes are accessible by the Game Boy) */
    uint8_t oam[0x100];

    /* HRAM area (assuming it's a standard 128B SRAM chip) (TODO) */
    uint8_t hram[0x100];

    /* whether there's an enqueued I/O read from the CPU */
    bool pending_io_read;
    uint8_t *pending_dst;
    uint8_t pending_addr;
} soc_t;

/*
 *      ** DMA **
 */

static inline void
dma_start(dma_t *dma, uint8_t high_addr)
{
    /* request a transfer and update high address. as explained below, setting
     * this value to 8 means that we will wait 2 machine cycles before actually
     * starting DMA. this mechanism is slightly different from
     * dma->cycles_to_waste, as it allows for a transfer to be restarted
     * (mooneye's oam_dma_restart) */
    dma->requested = 8;
    dma->high_addr = high_addr;
}

void dma_cycle(dma_t *dma);
void dma_init(dma_t *dma, soc_t *soc);

/*
 *      ** CPU **
 */

void cpu_cycle(cpu_t *cpu);
void cpu_init(cpu_t *cpu, soc_t *soc);

/*
 *      ** SOC **
 */

/* external bus */
uint8_t soc_ext_bus_read(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs);
void soc_ext_bus_write(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs, uint8_t val);

/* video bus */
uint8_t soc_vid_bus_read(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs);
void soc_vid_bus_write(soc_t *soc, bus_prio_t prio, uint16_t addr, bool cs, uint8_t val);

/* OAM */
uint8_t soc_oam_read(soc_t *soc, bus_prio_t prio, uint8_t addr);
void soc_oam_write(soc_t *soc, bus_prio_t prio, uint8_t addr, uint8_t val);

/* registers (iomem) and HRAM */
bool soc_internal_read(soc_t *soc, uint8_t *dst, uint8_t addr);
void soc_internal_write(soc_t *soc, uint8_t addr, uint8_t val);

/* core soc */
void soc_cycle(soc_t *soc);
unsigned soc_step(soc_t *soc);
unsigned soc_run_until_vblank(soc_t *soc);
void soc_run_one_frame(soc_t *soc);
soc_t *soc_create(bus_t *ext_bus, bus_t *video_bus);
void soc_destroy(soc_t *soc);

/* inline soc functions */
static inline void
soc_interrupt(soc_t *soc, uint8_t int_mask)
{
    /* we simply OR in the interrupt */
    soc->cpu->iflag |= int_mask;
}

/* this is for a component to "cancel" its own interrupt. imagine the PPU is
 * turned off; if it doesn't clear its interrupt it may be problematic as it may
 * trigger a VBlank interrupt when it is in a different mode */
static inline void
soc_uninterrupt(soc_t *soc, uint8_t int_mask)
{
    /* we clear the interrupt */
    soc->cpu->iflag &= ~int_mask;
}

/*
 *      ** PPU **
 */

static inline uint8_t
ppu_get_stat(ppu_t *ppu)
{
    uint8_t ret = 0x80;
    ret |= LCDC_PPU_ENABLE(ppu->lcdc) ? ppu->mode & 0x03 : 0;
    ret |= (ppu->lyc == ppu->ly) << 2;
    ret |= ppu->hblank_int_enabled << 3;
    ret |= ppu->vblank_int_enabled << 4;
    ret |= ppu->oam_int_enabled << 5;
    ret |= ppu->lyc_int_enabled << 6;
    return ret;
}

static inline void
ppu_set_stat(ppu_t *ppu, uint8_t val)
{
    ppu->hblank_int_enabled = val & 0x08;
    ppu->vblank_int_enabled = val & 0x10;
    ppu->oam_int_enabled = val & 0x20;
    ppu->lyc_int_enabled = val & 0x40;

    /* spurious interrupts */
    ppu->stat_written = true;
}

static inline void
ppu_write_lcdc(ppu_t *ppu, uint8_t val)
{
    /* if LCD is turned off, reset it for good */
    if (!LCDC_PPU_ENABLE(val)) {
        ppu->ly = 0;
        ppu->mode = PPU_HBLANK;
        ppu->cycles_to_waste = 1;
        ppu->stat_mode = ppu->stat_lyc = false;
    }

    /* set normal LCDC */
    ppu->lcdc = val;
}

void ppu_cycle(ppu_t *ppu);
void ppu_init(ppu_t *ppu, soc_t *soc);

/*
 *      ** TIMER **
 */

static inline void
tim_write_div(tim_t *tim)
{
    /* this will just reset the system timer */
    tim->div_write = true;
}

static inline void
tim_write_tma(tim_t *tim, uint8_t val)
{
    /* set next value of TMA */
    tim->tma = val;

    /* during the period in which TIMA writes are ignored, TIMA is kind of
     * hard-wired to TMA. therefore, we need to update it as well */
    if (tim->tima_writes_ignored)
        tim->tima = val;
}

static inline void
tim_write_tima(tim_t *tim, uint8_t val)
{
    /* when writing to TIMA, we're more of "making a request" */
    tim->tima_write = true;
    tim->tima_write_data = val;
}

static inline void
tim_write_tac(tim_t *tim, uint8_t val)
{
    /* when writing to TAC, something funny may happen. if the clock is changed
     * and the selected bit switches from a set one to an unset one, the falling
     * edge detector will capture that and increment TIMA. */
    tim->old_tac = tim->tac;
    tim->tac = val | 0xF8;
}

void tim_cycle(tim_t *tim);
void tim_init(tim_t *tim, soc_t *soc);

/*
 *      ** JOYPAD **
 */
uint8_t jp_read(jp_t *jp);
void jp_write(jp_t *jp, uint8_t val);
void jp_cycle(jp_t *jp);
void jp_init(jp_t *jp, soc_t *soc);

#endif /* __SOC_H */

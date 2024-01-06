#ifndef H_GBEMU
#define H_GBEMU

#include <stdlib.h>
#include <stdio.h>

typedef unsigned char byte;
typedef signed char offset;
typedef unsigned short ushort;
typedef unsigned int uint;

#define BOOL unsigned int
#define TRUE (1 == 1)
#define FALSE (!TRUE)

#define BIT(n) (1 << n)
#define MASK(bits) (BIT(bits) - 1)

#define IS_IN_RANGE(addr, start, end) \
    (addr >= start && addr <= end)

typedef union reg reg_t;
typedef struct gb gb_t;
typedef struct cpu cpu_t;
typedef struct mem mem_t;
typedef struct dma dma_t;
typedef struct ppu ppu_t;
typedef struct cart cart_t;
typedef struct input input_t;
typedef struct tim tim_t;
typedef struct serial serial_t;

union reg {
    struct {
        byte lo;
        byte hi;
    };
    ushort val;
};

struct gb {
    /* main gb struct, can be considered
     * the main bus */
    cpu_t *cpu;
    mem_t *mem;
    dma_t *dma;
    ppu_t *ppu;
    cart_t *cart;
    input_t *input;
    tim_t *tim;
    serial_t *serial;
};

struct cpu {
    /* gameboy's heart */
    gb_t *bus;
    reg_t af, bc, de, hl, sp, pc;
    byte ie;
    BOOL ei_called, halt, ime;
};

struct mem {
    /* memory chip */
    gb_t *bus;
    byte *ram, *hram;
};

struct dma {
    /* dma controller */
    gb_t *bus;
    uint cycles;
    ushort src;
};

struct ppu {
    /* graphics controller */
    gb_t *bus;
    BOOL vblank_intr, stat_intr;
    byte *vram, *oam;
};

struct cart {
    /* cart controller */
    gb_t *bus;
    byte (*read)(cart_t *cart, ushort addr);
    void (*write)(cart_t *cart, ushort addr, byte val);
    byte *bank; /* first bank is always present */
    void *mbc_data;    /* various mbc types have specific data */
};

struct input {
    /* input controller */
    gb_t *bus;
    BOOL intr;
};

struct tim {
    /* timer controller */
    gb_t *bus;
    BOOL intr;
    byte tima, tma, tac;
    ushort div;
    uint cycles;
    BOOL phase1, phase2;
};

struct serial {
    /* serial controller */
    gb_t *bus;
    BOOL intr;
};

/* gb.c */
void gb_run_once(gb_t *gb);
void gb_run_until_vsync(gb_t *gb);
gb_t *gb_create(FILE *file);
gb_t *gb_open(const char *path);

/* cpu.c */
uint cpu_step(cpu_t *cpu);
void cpu_init(cpu_t *cpu, gb_t *bus);

/* mem.c */
byte mem_readb(mem_t *mem, ushort addr);
void mem_writeb(mem_t *mem, ushort addr, byte val);
void mem_init(mem_t *mem, gb_t *bus);

/* dma.c */
BOOL dma_running(dma_t *dma);
void dma_start(dma_t *dma, byte src);
void dma_cycle(dma_t *dma);
void dma_init(dma_t *dma, gb_t *bus);

/* ppu.c */
byte ppu_readb(ppu_t *ppu, ushort addr);
void ppu_writeb(ppu_t *ppu, ushort addr, byte val);
void ppu_init(ppu_t *ppu, gb_t *bus);

/* cart.c */
void cart_init(cart_t *cart, gb_t *bus, FILE *file);

/* input.c */
void input_init(input_t *input, gb_t *bus);

/* tim.c */
void tim_reset_div(tim_t *tim);
void tim_write_tma(tim_t *tim, byte val);
void tim_write_tima(tim_t *tim, byte val);
void tim_write_tac(tim_t *tim, byte val);
void tim_cycle(tim_t *tim);
void tim_init(tim_t *tim, gb_t *bus);

/* serial.c */
void serial_init(serial_t *serial, gb_t *bus);

/* utils.c */
void __attribute__((noreturn)) die(const char *fmt, ...);
void *malloc_or_die(uint size, const char *fname, const char *name);

#endif /* H_GBEMU */

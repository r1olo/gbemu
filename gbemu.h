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

#define MASK(bits) ((1 << bits) - 1)

typedef union reg reg_t;
typedef struct gb gb_t;
typedef struct cpu cpu_t;
typedef struct mem mem_t;
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
    BOOL ei, halt, _ime;
};

struct mem {
    /* memory chip */
    gb_t *bus;
    byte ram[0x2000];
    byte hram[0x7F];
};

struct ppu {
    /* graphics controller */
    gb_t *bus;
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
};

struct tim {
    /* timer controller */
    gb_t *bus;
};

struct serial {
    /* serial controller */
    gb_t *bus;
};

/* gb.c */
gb_t *gb_create(FILE *file);
void gb_destroy(gb_t *gb);
gb_t *gb_open(const char *path);

/* cpu.c */
void cpu_init(cpu_t *cpu, gb_t *bus);

/* mem.c */
void mem_init(mem_t *mem, gb_t *bus);

/* ppu.c */
void ppu_init(ppu_t *ppu, gb_t *bus);

/* cart.c */
void cart_init(cart_t *cart, gb_t *bus, FILE *file);

/* input.c */
void input_init(input_t *input, gb_t *bus);

/* tim.c */
void tim_init(tim_t *tim, gb_t *bus);

/* serial.c */
void serial_init(serial_t *serial, gb_t *bus);

/* utils.c */
void __attribute__((noreturn)) die(const char *fmt, ...);
void *malloc_or_die(uint size, const char *fname, const char *name);

#endif /* H_GBEMU */
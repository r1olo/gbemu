#include <stdio.h>

#include "gbemu.h"

void
gb_run_once(gb_t *gb)
{
    int c = cpu_step(gb->cpu);
    for (int i = 0; i < c; i++) {
        dma_cycle(gb->dma);
    }
}

void
gb_run_until_vsync(gb_t *gb)
{
    /* TODO */
}

gb_t *
gb_create(FILE *file)
{
    gb_t *gb = malloc_or_die(sizeof(gb_t), "gb_create", "gb");
    gb->cpu = malloc_or_die(sizeof(cpu_t), "gb_create", "cpu");
    gb->mem = malloc_or_die(sizeof(mem_t), "gb_create", "mem");
    gb->dma = malloc_or_die(sizeof(dma_t), "gb_create", "dma");
    gb->ppu = malloc_or_die(sizeof(ppu_t), "gb_create", "ppu");
    gb->cart = malloc_or_die(sizeof(cart_t), "gb_create", "cart");
    gb->input = malloc_or_die(sizeof(input_t), "gb_create", "input");
    gb->tim = malloc_or_die(sizeof(tim_t), "gb_create", "tim");
    gb->serial = malloc_or_die(sizeof(serial_t), "gb_create", "serial");

    cpu_init(gb->cpu, gb);
    mem_init(gb->mem, gb);
    dma_init(gb->dma, gb);
    ppu_init(gb->ppu, gb);
    cart_init(gb->cart, gb, file);
    input_init(gb->input, gb);
    tim_init(gb->tim, gb);
    serial_init(gb->serial, gb);

    return gb;
}

gb_t *
gb_open(const char *path)
{
    gb_t *gb;
    FILE *file = fopen(path, "rb");
    
    if (!file)
        die("[gb_open] can't open cart file");

    gb = gb_create(file);
    fclose(file);
    return gb;
}

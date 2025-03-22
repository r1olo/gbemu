#include "ext/bus.h"
#include "ext/cart.h"
#include "ext/mem.h"
#include "soc/soc.h"
#include "types.h"
#include "log.h"

#include <string.h>
#include <stdio.h>
#include <soc/cpu_common.h>
#include <soc/instr/instrs.h>

static void
print_cpu_state(cpu_t *cpu)
{
    printf("==========================\n");
    printf("Flags: Z=%d, N=%d, H=%d, C=%d\n",
            FLAG(7), FLAG(6), FLAG(5), FLAG(4));
    printf("AF: 0x%04X\n", cpu->af.val);
    printf("BC: 0x%04X\n", cpu->bc.val);
    printf("DE: 0x%04X\n", cpu->de.val);
    printf("HL: 0x%04X\n", cpu->hl.val);
    printf("SP: 0x%04X\n", cpu->sp.val);
    printf("PC: 0x%04X\n", cpu->curpc.val);
    printf("IE: 0x%02X\n", cpu->ie);
    printf("IF: 0x%02X\n", cpu->iflag);
    printf("Interrupts: %d\n", cpu->ime);
    printf("==========================\n");
}

static void
print_tim_state(tim_t *tim)
{
    printf("TIMER SYS: 0x%02X - TIMER DIV: 0x%02X - TIMA: 0x%02X - TAC: 0x%02X - OVERFLOW: %d\n",
            tim->sys, (tim->sys >> 8) & 0xFF, tim->tima, tim->tac, tim->overflow);
}

static void
print_ppu_state(ppu_t *ppu)
{
    printf("PPU MODE: %d - LY: 0x%02X - STAT: 0x%02X - LCDC: 0x%02X - ON/OFF: %d\n",
            ppu->mode, ppu->ly, ppu_get_stat(ppu), ppu->lcdc,
            LCDC_PPU_ENABLE(ppu->lcdc));
}

// static void
// print_tilemap(vid_bus_t *vbus)
// {
//     printf("0x9800 TILE MAP\n");
//     for (size_t i = 0; i < 32; ++i) {
//         for (size_t j = 0; j < 32; ++j) {
//             printf("0x%02X ", vbus->vram->data[0x9800 + 32 * i + j]);
//         }
//         printf("\n");
//     }
// 
//     printf("0x9C00 TILE MAP\n");
//     for (size_t i = 0; i < 32; ++i) {
//         for (size_t j = 0; j < 32; ++j) {
//             printf("0x%02X ", vbus->vram->data[0x9C00 + 32 * i + j]);
//         }
//         printf("\n");
//     }
// }
// 
// static void
// print_vram(vid_bus_t *vbus)
// {
//     for (size_t i = 0; i < 0x800 / 16; ++i) {
//         printf("TILE %lu: ", i);
//         for (size_t j = 0; j < 16; ++j) {
//             printf("0x%02X ", vbus->vram->data[0x8000 + 16 * i + j]);
//         }
//         printf("\n");
//     }
// }

int main(int argc, char *argv[])
{
    if (argc < 2)
        exit(1);

#ifndef NDEBUG
    if (argc > 2 && !strcmp(argv[2], "-v"))
        _cur_log_lvl = LOG_VERBOSE;
#endif /* NDEBUG */

    cart_t *cart;
    enum gb_err res = cart_create(&cart, argv[1]);
    if (res != GBEMU_SUCCESS) {
        fprintf(stderr, "can't open cart!\n");
        return 1;
    }
    mem64_t *mem = malloc(sizeof(mem64_t));
    mem64_t *vram = malloc(sizeof(mem64_t));

    bus_t *bus = ext_bus_create(mem, cart);
    bus_t *vid_bus = vid_bus_create(vram);
    soc_t *soc = soc_create((bus_t *)bus, (bus_t *)vid_bus);

    while (true) {
        while (soc->cpu->curpc.val != 0x01E1) {
            soc_step(soc);
            print_cpu_state(soc->cpu);
            print_tim_state(soc->tim);
            print_ppu_state(soc->ppu);
        }

        printf("bp reached\n");
        getchar();

        //while (soc->cpu->curpc.val != 0x0268) {
        //    soc_step(soc);
        //    print_cpu_state(soc->cpu);
        //    print_tim_state(soc->tim);
        //    print_ppu_state(soc->ppu);
        //}

        //printf("bp reached\n");
        //getchar();
        break;
    }

    while (true) {
        soc_step(soc);
        print_cpu_state(soc->cpu);
        print_tim_state(soc->tim);
        print_ppu_state(soc->ppu);
        getchar();
    }

    free(mem);
    free(vram);
    free(bus);
    free(vid_bus);
    cart_destroy(cart);
    soc_destroy(soc);
}

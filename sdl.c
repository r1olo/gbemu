#include <SDL2/SDL_keycode.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <unistd.h>

#include "ext/bus.h"
#include "ext/cart.h"
#include "ext/mem.h"
#include "soc/soc.h"
#include "soc/cpu_common.h"
#include "log.h"

#define WIN_WIDTH   160
#define WIN_HEIGHT  144

static void
print_cpu_state(cpu_t *cpu)
{
    printf("==========================\n");
    printf("AF: 0x%04X\n", cpu->af.val);
    printf("Flags: Z=%d, N=%d, H=%d, C=%d\n",
            FLAG(7), FLAG(6), FLAG(5), FLAG(4));
    printf("BC: 0x%04X\n", cpu->bc.val);
    printf("DE: 0x%04X\n", cpu->de.val);
    printf("HL: 0x%04X\n", cpu->hl.val);
    printf("SP: 0x%04X\n", cpu->sp.val);
    printf("PC: 0x%04X\n", cpu->curpc.val);
    printf("IE: 0x%04X\n", cpu->ie);
    printf("IF: 0x%04X\n", cpu->iflag);
    printf("Interrupts: %d\n", cpu->ime);
    printf("==========================\n");
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
    printf("PPU MODE: %d - LY: %d - ON/OFF: %d\n",
            ppu->mode, ppu->ly, LCDC_PPU_ENABLE(ppu->lcdc));
}

/*
static void
print_tilemap(vid_bus_t *vbus)
{
    printf("0x9800 TILE MAP\n");
    for (size_t i = 0; i < 32; ++i) {
        for (size_t j = 0; j < 32; ++j) {
            printf("0x%02X ", vbus->vram->data[0x9800 + 32 * i + j]);
        }
        printf("\n");
    }

    printf("0x9C00 TILE MAP\n");
    for (size_t i = 0; i < 32; ++i) {
        for (size_t j = 0; j < 32; ++j) {
            printf("0x%02X ", vbus->vram->data[0x9C00 + 32 * i + j]);
        }
        printf("\n");
    }
}
*/

int main(int argc, char *argv[])
{
    if (argc < 2)
        exit(1);

#ifndef NDEBUG
    if (argc >= 3 && !strcmp(argv[2], "-v"))
        _cur_log_lvl = LOG_VERBOSE;
#endif /* NDEBUG */

    cart_t *cart;
    enum gb_err err = cart_create(&cart, argv[1]);
    if (err != GBEMU_SUCCESS) {
        fprintf(stderr, "error opening cart\n");
        exit(1);
    }

    mem64_t *mem = malloc(sizeof(mem64_t));
    mem64_t *vram = malloc(sizeof(mem64_t));

    bus_t *bus = ext_bus_create(mem, cart);
    bus_t *vid_bus = vid_bus_create(vram);
    soc_t *soc = soc_create((bus_t *)bus, (bus_t *)vid_bus);

    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("GBEmu",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WIN_WIDTH * 4,
            WIN_HEIGHT * 4,
            SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("unable to create window: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("unable to create renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, WIN_WIDTH, WIN_HEIGHT);

    SDL_Texture *texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            WIN_WIDTH,
            WIN_HEIGHT);
    if (!texture) {
        SDL_Log("unable to create texture: %s", SDL_GetError());
        return 1;
    }

    pixel_t pixels[WIN_HEIGHT][WIN_WIDTH] = {0};
    pixels[0][80].val = 0x00FF0000;

    int pitch;
    void *texture_pixels = NULL;
    if (SDL_LockTexture(texture, NULL, &texture_pixels, &pitch)) {
        SDL_Log("unable to lock texture: %s", SDL_GetError());
        return 1;
    }
    assert(pitch * WIN_HEIGHT == sizeof(pixels));
    memcpy(texture_pixels, pixels, pitch * WIN_HEIGHT);
    SDL_UnlockTexture(texture);

    print_cpu_state(soc->cpu);
    print_ppu_state(soc->ppu);
    print_tim_state(soc->tim);

    /* the keys array */
    bool keys[256] = { false };

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    assert(e.key.keysym.scancode < 256);
                    keys[e.key.keysym.scancode] = true;
                    break;
                case SDL_KEYUP:
                    assert(e.key.keysym.scancode < 256);
                    keys[e.key.keysym.scancode] = false;
                    break;
                default:
                    break;
            }
        }

        soc->jp->start_pressed = keys[SDL_SCANCODE_RETURN];
        soc->jp->select_pressed = keys[SDL_SCANCODE_S];

        //soc_run_one_frame(soc);
        soc_run_until_vblank(soc);

        if (SDL_LockTexture(texture, NULL, &texture_pixels, &pitch)) {
            SDL_Log("unable to lock texture: %s", SDL_GetError());
            return 1;
        }
        assert(pitch * WIN_HEIGHT == sizeof(pixels));
        memcpy(texture_pixels, soc->ppu->screen, pitch * WIN_HEIGHT);
        SDL_UnlockTexture(texture);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        /* sleep (vblank delay) */
        //usleep(16949);
    }

    free(mem);
    free(vram);
    free(bus);
    free(vid_bus);
    cart_destroy(cart);
    soc_destroy(soc);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

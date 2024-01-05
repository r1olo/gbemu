#include <stdio.h>

#include "gbemu.h"

#define FLAG(n) \
    ((gb->cpu->af.lo & (1 << n)) >> n)

int
main(int argc, char **argv)
{
    if (argc < 2)
        die("I need a ROM!");

    gb_t *gb = gb_open(argv[1]);
    while (TRUE) {
        printf("==========================\n");
        printf("AF: 0x%x\n", gb->cpu->af.val);
        printf("Flags: Z=%d, N=%d, H=%d, C=%d\n",
                FLAG(7), FLAG(6), FLAG(5), FLAG(4));
        printf("BC: 0x%x\n", gb->cpu->bc.val);
        printf("DE: 0x%x\n", gb->cpu->de.val);
        printf("HL: 0x%x\n", gb->cpu->hl.val);
        printf("SP: 0x%x\n", gb->cpu->sp.val);
        printf("PC: 0x%x\n", gb->cpu->pc.val);
        printf("Interrupts: %d\n\n", gb->cpu->ime);
        gb_run_once(gb);
        getchar();
    }
}

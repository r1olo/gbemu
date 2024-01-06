#include "gbemu.h"

void
input_init(input_t *input, gb_t *bus)
{
    /* TODO */
    input->bus = bus;
    input->intr = FALSE;
}

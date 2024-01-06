#include "gbemu.h"

void
serial_init(serial_t *serial, gb_t *bus)
{
    /* TODO */
    serial->bus = bus;
    serial->intr = FALSE;
}

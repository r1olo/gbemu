#include "gbemu.h"

int main()
{
    gb_t *gb = gb_open("../rom.gb");
    gb_destroy(gb);
}

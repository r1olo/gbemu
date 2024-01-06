#include "gbemu.h"

static int
get_freq_bit(byte tac)
{
    uint ret, freq = tac & 3;
    
    switch (freq) {
        case 0:
            ret = 9;
            break;
        case 1:
            ret = 3;
            break;
        case 2:
            ret = 5;
            break;
        case 3:
            ret = 7;
            break;
        default:
            die("[get_freq_bit] invalid tac value");
    }

    return ret;
}

static void
increase_tima(tim_t *tim)
{
    if (tim->tima + 1 < tim->tima) {
        /* overflow */
        tim->phase1 = TRUE;
    } else {
        tim->tima++;
    }
}

void
tim_reset_div(tim_t *tim)
{
    /* if timer is disabled do nothing */
    if (!(tim->tma & BIT(2)))
        return;

    ushort old_div = tim->div;
    tim->div = 0;

    /* increase TIMA if falling edge is detected */
    uint bit = get_freq_bit(tim->tac);
    if ((old_div & BIT(bit)) && !(tim->div & BIT(bit)))
        increase_tima(tim);
}

void
tim_write_tma(tim_t *tim, byte val)
{
    if (tim->phase2) {
        /* also load TIMA with new value */
        tim->tima = val;
    }

    tim->tma = val;
}

void
tim_write_tima(tim_t *tim, byte val)
{
    if (tim->phase1) {
        /* cancel TIMA reload and interrupt */
        tim->phase1 = FALSE;
    } else if (tim->phase2) {
        /* ignore the write completely */
        return;
    }

    tim->tima = val;
}

void
tim_write_tac(tim_t *tim, byte val)
{
    byte old_tac = tim->tac;
    tim->tac = val;
    uint freq = tim->tac & 3, old_freq = old_tac & 3;

    /* timer was already disabled, do nothing */
    if (!(old_tac & BIT(2)))
        return;

    uint bit = get_freq_bit(freq);
    uint old_bit = get_freq_bit(old_freq);

    /* if timer is disabled */
    if (!(tim->tac & BIT(2))) {
        /* if DIV's mux was 1 */
        if (tim->div & BIT(old_bit))
            increase_tima(tim);
    } else if (freq != old_freq) {
        /* if freq is changed and DIV's mux goes to 0 */
        if ((tim->div & BIT(old_bit)) && !(tim->div & BIT(bit)))
            increase_tima(tim);
    }
}

void
tim_cycle(tim_t *tim)
{
    if (tim->cycles < 3) {
        tim->cycles++;
        return;
    }

    /* reset cycles and increase DIV */
    tim->cycles = 0;
    ushort old_div = tim->div++;

    /* this is needed for obscure timer behavior */
    if (tim->phase1) {
        tim->phase1 = FALSE;
        tim->phase2 = TRUE;
        tim->tima = tim->tma;
        tim->intr = TRUE;
    } else if (tim->phase2) {
        tim->phase2 = FALSE;
    }

    /* if timer is disable, don't care */
    if (!(tim->tma & BIT(2)))
        return;

    /* increase TIMA if falling edge is detected */
    int bit = get_freq_bit(tim->tac);
    if ((old_div & BIT(bit)) && !(tim->div & BIT(bit)))
        increase_tima(tim);
}

void
tim_init(tim_t *tim, gb_t *bus)
{
    tim->bus = bus;
    tim->intr = FALSE;
    tim->tima = tim->tma = 0;
    tim->tac = 0xf8;
    tim->div = 0x18;
    tim->cycles = 0;
    tim->phase1 = tim->phase2 = FALSE;
}

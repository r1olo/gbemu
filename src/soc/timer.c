#include "soc/soc.h"
#include "types.h"

static inline unsigned
_get_freq_bit(uint8_t clock)
{
    assert(clock <= 3);
    switch (clock) {
        case 0x00:
            return 9;
        case 0x01:
            return 3;
        case 0x02:
            return 5;
        case 0x03:
            return 7;
        default:
            unreachable();
    }
}

void
tim_cycle(tim_t *tim)
{
    /* sample the previous SYS timer to detect any falling edge */
    uint16_t old_sys = tim->sys;

    /* if the CPU wrote to DIV, reset the SYS timer */
    if (tim->div_write) {
        tim->sys = 0;
        tim->div_write = false;
    }

    /* increment SYS. this is *always* incremented, even if it has just been
     * reset by the CPU */
    ++tim->sys;

    /* sample TIMA to detect falling edge on bit 7 */
    uint8_t old_tima = tim->tima;

    /* if tima writes are ignored, we "block" the CPU from messing with it */
    if (tim->tima_writes_ignored) {
        --tim->tima_writes_ignored;
        tim->tima_write = false;
    }

    /* either increase TIMA or set it because of overflow (with interrupt). if
     * we're in overflow, this also cancels any attempt from the CPU to write
     * TIMA. moreover, after TIMA has been reset to TMA, there's a period of
     * cycles (testing gives 1 machine cycle) where writes to it are ignored all
     * the time. */
    if (tim->overflow && !(--tim->overflow)) {
        tim->tima = tim->tma;
        tim->tima_writes_ignored = 3;
        soc_interrupt(tim->soc, INT_TIMER);
    } else {
        /* currently selected bit */
        unsigned bit = _get_freq_bit(TAC_CLOCK(tim->tac));

        /* if timer is not enabled, the SYS falling edge doesn't work. if we
         * *just* disabled the timer, however, it will cause a TIMA tick if the
         * currently selected bit is set (DMG stuff) */
        if (TAC_ENABLE(tim->tac)) {
            unsigned old_bit = _get_freq_bit(TAC_CLOCK(tim->old_tac));
            if (old_sys & BIT(old_bit) && !(tim->sys & BIT(bit)))
                ++tim->tima;
        } else {
            /* we are disabled, but previously we were enabled */
            if (TAC_ENABLE(tim->old_tac) && (tim->sys & BIT(bit)))
                ++tim->tima;
        }
    }

    /* check for tima overflow (this always works even if timer is disabled) */
    if (tim->tima_write) {
        /* if CPU issued a write, replace TIMA with CPU's value and don't check
         * for falling edge */
        tim->tima = tim->tima_write_data;
        tim->tima_write = false;

        /* writing to TIMA during an overflow cancels the overflow */
        tim->overflow = 0;
    } else {
        /* if CPU didn't write, check TIMA for falling edge (overflow) and set
         * the variable for the next cycle */
        if (old_tima & BIT(7) && !(tim->tima & BIT(7))) {
            /* there's a delay between the timer tick and the actual TIMA
             * setting/interrupt, and by testing it is 3 cycles. this would
             * adhere to the fact that the timer is properly reset 1 machine
             * cycle after it has overflown */
            tim->overflow = 3;
            assert(tim->tima == 0);
        }
    }

    /* set the old TAC to normal TAC */
    tim->old_tac = tim->tac;
}

void
tim_init(tim_t *tim, soc_t *soc)
{
    /* set up SoC */
    tim->soc = soc;

    /* initial system timer is 0x18 shifted 8 times to the left (which
     * corresponds to DIV, set to 0x18) */
    tim->sys = 0x1800;

    /* initial registers */
    tim->tima = tim->tma = 0x00;
    tim->tac = 0xF8;

    /* initial status */
    tim->overflow = 0;
    tim->div_write = false;
    tim->tima_write = false;
    tim->tima_writes_ignored = 0;
    tim->old_tac = tim->tac;
}

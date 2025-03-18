#include "soc/cpu_common.h"
#include "soc/instr/instrs.h"
#include "types.h"

static void
_dec_pc(cpu_t *cpu)
{
    /* decrement PC */
    _idu_write(cpu, cpu->pc.val--);
}

static void
_dec_sp(cpu_t *cpu)
{
    /* decrement PC */
    _idu_write(cpu, cpu->sp.val--);
}

static void
_push_high_pc(cpu_t *cpu)
{
    /* push high PC to stack and decrease SP */
    _cpu_write_byte(cpu, cpu->sp.val, cpu->pc.hi);
    _idu_write(cpu, cpu->sp.val--);
}

static void
_push_low_pc_ack_interrupt(cpu_t *cpu)
{
    /* push low PC to stack and set PC to one of the IRQ addresses.
     * fun fact: this is the fourth cycle in the instruction. if a higher
     * priority interrupt was fired since the beginning of the ISR, *that*
     * interrupt will be served instead of the original one */
    uint8_t ints = _pending_interrupts(cpu);

    /* just to remember, I forgot to put the following line when first
     * debugging the emulator, and I couldn't find out why it wasn't working.
     * an interrupt was NOT pushing the return address to the stack!
     *
     * to fix it, I copied the write from _push_high_pc, pushing the *HIGH* PC
     * to the stack instead of the low. will it end? */
    _cpu_write_byte(cpu, cpu->sp.val, cpu->pc.lo);

    /* PC is always written, no matter the case below */
    _idu_write(cpu, cpu->pc.val);

    /* something might have overwritten IF during this very instruction, meaning
     * that we now have no valid interrupts. in such case, we jump to address
     * 0x0000 and don't clear anything */
    if (!ints) {
        cpu->pc.val = 0x0000;
        return;
    }

    /* check for interrupt in order */
    for (uint i = 0; i < 5; ++i) {
        /* if we find an interrupt, break out of the loop */
        if (ints & BIT(i)) {
            cpu->pc.val = 0x0040 + 8 * i;
            cpu->iflag &= ~(BIT(i));
            break;
        }
    }
}

fn_row_array isr = {
    { "ISR [M2]",           _dec_pc },
    { "ISR [M3]",           _dec_sp },
    { "ISR [M4]",           _push_high_pc },
    { "ISR [M5]",           _push_low_pc_ack_interrupt },
    { "ISR [M6]",           _nothing },
    { NULL,                 NULL }
};

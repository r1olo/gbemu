#include "soc/cpu_common.h"
#include "soc/instr/instrs.h"
#include "soc/soc.h"
#include "log.h"
#include "types.h"

bool
_cpu_read_byte(cpu_t *cpu, uint8_t *dst, uint16_t addr)
{
    /* 0x0000 - 0x7FFF External bus, CS=1 (cart ROM) */
    if (addr < 0x8000) {
        *dst = soc_ext_bus_read(cpu->soc, PRIO_CPU, addr, true);
        return true;
    }

    /* 0x8000 - 0x9FFF Video bus, CS=0 */
    if (addr < 0xA000) {
        *dst = soc_vid_bus_read(cpu->soc, PRIO_CPU, addr, false);
        return true;
    }

    /* 0xA000 - 0xFDFF External bus, CS=0 (rams) */
    if (addr < 0xFE00) {
        *dst = soc_ext_bus_read(cpu->soc, PRIO_CPU, addr, false);
        return true;
    }

    /* 0xFE00 - 0xFEFF OAM + unused area */
    if (addr < 0xFF00) {
        *dst = soc_oam_read(cpu->soc, PRIO_CPU, addr & 0xFF);
        return true;
    }

    /* 0xFF00 - 0xFFFE High area (HRAM and registers) */
    if (addr < 0xFFFF)
        return soc_internal_read(cpu->soc, dst, addr & 0xFF);

    /* 0xFFFF - IE register */
    *dst = cpu->ie;
    return true;
}

void
_cpu_write_byte(cpu_t *cpu, uint16_t addr, uint8_t val)
{
    /* 0x0000 - 0x7FFF External bus, CS=1 (cart ROM) */
    if (addr < 0x8000)
        return soc_ext_bus_write(cpu->soc, PRIO_CPU, addr, true, val);

    /* 0x8000 - 0x9FFF Video bus, CS=0 */
    if (addr < 0xA000)
        return soc_vid_bus_write(cpu->soc, PRIO_CPU, addr, false, val);

    /* 0xA000 - 0xFDFF External bus, CS=0 (rams) */
    if (addr < 0xFE00)
        return soc_ext_bus_write(cpu->soc, PRIO_CPU, addr, false, val);

    /* 0xFE00 - 0xFEFF OAM + unused area */
    if (addr < 0xFF00)
        return soc_oam_write(cpu->soc, PRIO_CPU, addr & 0xFF, val);

    /* 0xFF00 - 0xFFFE High area (HRAM and registers) */
    if (addr < 0xFFFF)
        return soc_internal_write(cpu->soc, addr & 0xFF, val);

    /* 0xFFFF - IE register */
    cpu->ie = val;
}

bool
_cpu_read_imm8(cpu_t *cpu, uint8_t *dst)
{
    /* read from (PC) and increment PC (we don't want delayed reads here) */
    bool ret = _cpu_read_byte(cpu, dst, cpu->pc.val);
    _idu_write(cpu, cpu->pc.val++);
    return ret;
}

void
cpu_cycle(cpu_t *cpu)
{
    /* if we have pending cycles exhaust them */
    WASTE_CYCLES(cpu);

    /* make sure we have a valid instruction list and a valid function */
    assert(cpu->curlist && cpu->curlist[cpu->state].fn);

    /* execute che current function */
    LOG(LOG_VERBOSE, "executing %s in PC 0x%04X",
            cpu->curlist[cpu->state].name, cpu->curpc.val);
    cpu->curlist[cpu->state++].fn(cpu);

    /* if the next function in the list is NULL, we overlap the currently
     * executed function with a fetch */
    if (!cpu->curlist[cpu->state].fn)
        cpu->state = FETCH;

    /* check for EI instruction */
    switch (cpu->ei_state) {
        case EI_CALLED:
            cpu->ei_state = EI_SET;
            break;
        case EI_SET:
            cpu->ime = true;
            cpu->ei_state = EI_NOT_CALLED;
            break;
        default:
            break;
    }

    /* if we're in fetch mode, proceed with routine work */
    if (cpu->state == FETCH) {
        /* this is mostly for debug */
        cpu->curpc = cpu->pc;

        /* get the current interrupts */
        uint8_t ints = _pending_interrupts(cpu);

        /* the actual instruction we fetch depends on whether we're halted. note
         * that we're just filling the instruction register - we might jump to
         * ISR directly later on if IME is enabled */
        if (cpu->halt) {
            if (ints) {
                /* any pending interrupt, whether handled or not (see IME check
                 * later), causes HALT to stop */
                cpu->halt = false;

                /* if it's the first cycle, the CPU does not increment the PC
                 * (HALT bug). otherwise, it normally does */
                if (cpu->halt_bug)
                    _cpu_read_byte(cpu, &cpu->ir, cpu->pc.val);
                else
                    _cpu_read_imm8(cpu, &cpu->ir);
            } else {
                /* setting up a NOP instruction */
                cpu->ir = 0x00;
            }

            /* always disable the halt bug after the first cycle */
            cpu->halt_bug = false;
        } else {
            /* if not halted, normal fetch is executed */
            _cpu_read_imm8(cpu, &cpu->ir);
        }

        /* the next step depends on whether we're interrupted. if we are, we
         * jump directly to ISR. otherwise, we execute the currently fetched
         * instruction */
        if (cpu->ime && ints) {
            cpu->ime = false;
            cpu->curlist = isr;
        } else {
            /* set up the new instruction */
            cpu->curlist = *instructions[cpu->ir];
        }
    }

    /* get ready for a new round of cycle wasting */
    cpu->cycles_to_waste = 3;
}

void
cpu_init(cpu_t *cpu, soc_t *soc)
{
    /* assign SoC */
    cpu->soc = soc;

    /* first instruction is NOP */
    cpu->ir = 0;

    /* set up interrupt logic */
    cpu->ie = 0;
    cpu->iflag = 0xE1;
    cpu->ime = false;
    cpu->ei_state = EI_NOT_CALLED;

    /* default reg values */
    cpu->af.hi = 0x01;
    cpu->af.lo = 0x00;
    cpu->bc.hi = 0xFF;
    cpu->bc.lo = 0x13;
    cpu->de.hi = 0x00;
    cpu->de.lo = 0xC1;
    cpu->hl.hi = 0x84;
    cpu->hl.lo = 0x03;

    /* default PC and SP */
    cpu->pc.val = 0x100;
    cpu->sp.val = 0xFFFE;

    /* we start with the fetch state */
    cpu->curlist = nop;
    cpu->state = FETCH;

    /* misc */
    cpu->curpc = cpu->pc;

    /* pending dots */
    cpu->cycles_to_waste = 3;

    /* the HALT logic */
    cpu->halt = cpu->halt_bug = false;
}

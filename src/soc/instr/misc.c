#include "soc/cpu_common.h"
#include "soc/instr/instrs.h"
#include "log.h"

static void
_panic(cpu_t *cpu)
{
    /* TODO */
    LOG(LOG_ERR, "unknown instruction");
}

static void
_halt(cpu_t *cpu)
{
    /* halt the CPU */
    cpu->halt = true;

    /* this sets up the HALT bug, which only happens in the first cycle */
    cpu->halt_bug = true;
}

static void
_ei(cpu_t *cpu)
{
    /* the EI instruction was called. it will not be set immediately, but after
     * 1 M-Cycle. to avoid "bricking" interrupts by chanining EI instructions,
     * we first check if EI has already been invoked (see mooneye's ei_sequence
     * test) */
    if (cpu->ei_state == EI_NOT_CALLED)
        cpu->ei_state = EI_CALLED;
}

static void
_di(cpu_t *cpu)
{
    /* the DI instruction immediately disables instructions and cancels any
     * previously called EI */
    cpu->ime = false;
    cpu->ei_state = EI_NOT_CALLED;
}

static void
_cb_prefix(cpu_t *cpu)
{
    /* simply set the list to the current CB instruction. the CB function lists
     * have a NULL element on top (the FETCH cycle) so that the cpu state
     * matches with the correct index. no adjustment is needed */
    _cpu_read_imm8(cpu, &cpu->ir);
    cpu->curlist = *cb_instructions[cpu->ir];
}

fn_row_array nop = {
    { "NOP [M2]",       _nothing },
    { NULL,             NULL }
};

fn_row_array stop = {
    { "STOP [M2]",      _panic },
    { NULL,             NULL }
};

fn_row_array halt = {
    { "HALT [M2]",      _halt },
    { "HALT [M3]",      _nothing },
    { NULL,             NULL }
};

fn_row_array ei = {
    { "EI [M2]",        _ei },
    { NULL,             NULL }
};

fn_row_array di = {
    { "DI [M2]",        _di },
    { NULL,             NULL }
};

fn_row_array prefix = {
    { "PREFIX CB [M2]", _cb_prefix },
    { NULL,             NULL }
};

fn_row_array not_impl = {
    { "NOT IMPLEMENTED",_panic },
    { NULL,             NULL }
};

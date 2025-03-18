#include "soc/cpu_common.h"
#include "soc/instr/instrs.h"
#include "types.h"

static inline uint8_t *
_ld8_get_r(cpu_t *cpu, uint8_t val)
{
    /* based on value, choose a register */
    assert(val <= 7 && val != 6);
    switch (val) {
        case 0x00:
            return &cpu->bc.hi;
        case 0x01:
            return &cpu->bc.lo;
        case 0x02:
            return &cpu->de.hi;
        case 0x03:
            return &cpu->de.lo;
        case 0x04:
            return &cpu->hl.hi;
        case 0x05:
            return &cpu->hl.lo;
        case 0x07:
            return &cpu->af.hi;
        default:
            unreachable();
    }
}

static inline reg_t *
_ld8_get_rr(cpu_t *cpu, uint8_t val)
{
    assert(val <= 3);
    switch (val) {
        case 0x00:
            return &cpu->bc;
        case 0x01:
            return &cpu->de;
        case 0x02:
        case 0x03:
            return &cpu->hl;
        default:
            unreachable();
    }
}

static inline reg_t *
_ld16_get_rr(cpu_t *cpu, uint8_t val)
{
    assert(val <= 3);
    switch (val) {
        case 0x00:
            return &cpu->bc;
        case 0x01:
            return &cpu->de;
        case 0x02:
            return &cpu->hl;
        case 0x03:
            return &cpu->sp;
        default:
            unreachable();
    }
}

static inline reg_t *
_push_pop_get_rr(cpu_t *cpu, uint8_t val)
{
    assert(val <= 3);
    switch (val) {
        case 0x00:
            return &cpu->bc;
        case 0x01:
            return &cpu->de;
        case 0x02:
            return &cpu->hl;
        case 0x03:
            return &cpu->af;
        default:
            unreachable();
    }
}

static void
_ld_r_r(cpu_t *cpu)
{
    /* get the two registers and move them */
    uint8_t *src = _ld8_get_r(cpu, cpu->ir & 0x07);
    uint8_t *dst = _ld8_get_r(cpu, (cpu->ir & 0x38) >> 3);
    *dst = *src;
}

static void
_ld_ihl_r_write_mem(cpu_t *cpu)
{
    /* get src reg and write it to mem */
    uint8_t *src = _ld8_get_r(cpu, cpu->ir & 0x07);
    _cpu_write_byte(cpu, cpu->hl.val, *src);
}

static void
_read_ihl_to_z(cpu_t *cpu)
{
    /* simply read mem to Z */
    _cpu_read_byte(cpu, &cpu->wz.lo, cpu->hl.val);
}

static void
_ld_r_ihl_set_mem(cpu_t *cpu)
{
    /* set the read memory from Z to register */
    uint8_t *dst = _ld8_get_r(cpu, (cpu->ir & 0x38) >> 3);
    *dst = cpu->wz.lo;
}

static void
_ld_irr_a(cpu_t *cpu)
{
    /* get the dst and write mem into it */
    reg_t *dst = _ld8_get_rr(cpu, (cpu->ir & 0x30) >> 4);
    _cpu_write_byte(cpu, dst->val, cpu->af.hi);

    /* increment or decrement HL if we need to */
    switch ((cpu->ir & 0x30) >> 4) {
        case 0x02:
            _idu_write(cpu, (cpu->hl.val)++);
            break;
        case 0x03:
            _idu_write(cpu, cpu->hl.val--);
            break;
        default:
            break;
    }
}

static void
_ld_a_irr(cpu_t *cpu)
{
    /* get the src and read mem into it */
    reg_t *src = _ld8_get_rr(cpu, (cpu->ir & 0x30) >> 4);
    _cpu_read_byte(cpu, &cpu->af.hi, src->val);

    /* increment or decrement HL if we need to */
    switch ((cpu->ir & 0x30) >> 4) {
        case 0x02:
            _idu_write(cpu, cpu->hl.val++);
            break;
        case 0x03:
            _idu_write(cpu, cpu->hl.val--);
            break;
        default:
            break;
    }
}

static void
_read_imm8_into_z(cpu_t *cpu)
{
    /* simply read the immediate */
    _cpu_read_imm8(cpu, &cpu->wz.lo);
}

static void
_read_imm8_into_w(cpu_t *cpu)
{
    /* simply read the immediate */
    _cpu_read_imm8(cpu, &cpu->wz.hi);
}

static void
_write_a_to_iwz(cpu_t *cpu)
{
    /* write A to (WZ) */
    _cpu_write_byte(cpu, cpu->wz.val, cpu->af.hi);
}

static void
_read_iwz_into_z(cpu_t *cpu)
{
    /* read (WZ) into Z */
    _cpu_read_byte(cpu, &cpu->wz.lo, cpu->wz.val);
}

static void
_ld_r_n_set_imm(cpu_t *cpu)
{
    /* set the immediate */
    uint8_t *dst = _ld8_get_r(cpu, (cpu->ir & 0x38) >> 3);
    *dst = cpu->wz.lo;
}

static void
_write_z_to_ihl(cpu_t *cpu)
{
    /* write immediate to mem */
    _cpu_write_byte(cpu, cpu->hl.val, cpu->wz.lo);
}

static void
_write_a_to_hic(cpu_t *cpu)
{
    /* write A register to 0xFF00 + C */
    _cpu_write_byte(cpu, 0xFF00 + cpu->bc.lo, cpu->af.hi);
}

static void
_read_hic_into_z(cpu_t *cpu)
{
    /* read memory into Z */
    _cpu_read_byte(cpu, &cpu->wz.lo, 0xFF00 + cpu->bc.lo);
}

static void
_write_z_to_a(cpu_t *cpu)
{
    /* put Z into A */
    cpu->af.hi = cpu->wz.lo;
}

static void
_write_a_to_hiz(cpu_t *cpu)
{
    /* write A to 0xFF00 + Z */
    _cpu_write_byte(cpu, 0xFF00 + cpu->wz.lo, cpu->af.hi);
}

static void
_read_hiz_into_z(cpu_t *cpu)
{
    /* read 0xFF00 + Z into Z */
    _cpu_read_byte(cpu, &cpu->wz.lo, 0xFF00 + cpu->wz.lo);
}

static void
_ld_rr_nn(cpu_t *cpu)
{
    /* put WZ into one of the dst registers */
    reg_t *dst = _ld16_get_rr(cpu, (cpu->ir & 0x30) >> 4);
    dst->val = cpu->wz.val;
}

static void
_write_p_to_iwz_inc(cpu_t *cpu)
{
    /* write P (low SP) to (WZ) and increment WZ */
    _cpu_write_byte(cpu, cpu->wz.val++, cpu->sp.lo);
}

static void
_write_s_to_iwz(cpu_t *cpu)
{
    /* write S (high SP) to (WZ) */
    _cpu_write_byte(cpu, cpu->wz.val, cpu->sp.hi);
}

static void
_dec_sp(cpu_t *cpu)
{
    /* decrement SP */
    _idu_write(cpu, cpu->sp.val--);
}

static void
_push_hi(cpu_t *cpu)
{
    /* write hi reg to mem and dec SP again */
    reg_t *reg = _push_pop_get_rr(cpu, (cpu->ir & 0x30) >> 4);
    _cpu_write_byte(cpu, cpu->sp.val, reg->hi);
    _idu_write(cpu, cpu->sp.val--);
}

static void
_push_lo(cpu_t *cpu)
{
    /* just write lo reg to mem */
    reg_t *reg = _push_pop_get_rr(cpu, (cpu->ir & 0x30) >> 4);
    _cpu_write_byte(cpu, cpu->sp.val, reg->lo);
}

static void
_pop_lo_into_z(cpu_t *cpu)
{
    /* pop lo value and increment SP */
    _cpu_read_byte(cpu, &cpu->wz.lo, cpu->sp.val);
    _idu_write(cpu, cpu->sp.val++);
}

static void
_pop_hi_into_w(cpu_t *cpu)
{
    /* pop hi value and increment SP */
    _cpu_read_byte(cpu, &cpu->wz.hi, cpu->sp.val);
    _idu_write(cpu, cpu->sp.val++);
}

static void
_pop(cpu_t *cpu)
{
    /* write WZ into reg */
    reg_t *reg = _push_pop_get_rr(cpu, (cpu->ir & 0x30) >> 4);
    reg->val = cpu->wz.val;

    /* this is kinda hacky but the concept is the same. when popping the AF
     * register, we should only change the flag bits of the F register, and NOT
     * the whole 8 bits. doing this will suffice: */
    cpu->af.lo &= 0xF0;
}

static void
_write_hl_to_sp(cpu_t *cpu)
{
    /* TODO: HL is put on the address bus. it is unknown whether the game boy
     * asserts a write or read, but the latter is more likely. we just discard
     * the read. maybe make a _idu_read() counterpart? */

    /* move hl to sp */
    cpu->sp.val = cpu->hl.val;
}

static void
_write_adjusted_pe_to_l(cpu_t *cpu)
{
    /* write adjusted P (low SP) to L (SPL + Z) */
    cpu->hl.lo = cpu->sp.lo + cpu->wz.lo;

    /* set flags */
    if (((cpu->sp.lo & 0xf) + (cpu->wz.lo & 0xf)) & 0x10)
        SET_HALF();
    else
        UNSET_HALF();

    if (((cpu->sp.lo & 0xff) + (cpu->wz.lo & 0xff)) & 0x100)
        SET_CARRY();
    else
        UNSET_CARRY();

    /* reset these two */
    UNSET_SUB();
    UNSET_ZERO();
}

static void
_write_adjusted_se_to_h(cpu_t *cpu)
{
    /* write adjusted S (high SP) to H (SPH + adjustment + carry) */

    /* adjustment depends on sign of Z */
    uint8_t adj = 0;
    if (cpu->wz.lo & 0x80)
        adj = -1;

    /* add with previous carry but don't set flags */
    cpu->hl.hi = cpu->sp.hi + adj + FLAG_CARRY();
}

/* 01xxxyyy - LD r, r */
fn_row_array ld_r_r = {
    { "LD r, r [M2]",           _ld_r_r },
    { NULL,                     NULL },
};

/* 01110yyy - LD (HL), r */
fn_row_array ld_ihl_r = {
    { "LD (HL), r [M2]",        _ld_ihl_r_write_mem },
    { "LD (HL), r [M3]",        _nothing },
    { NULL,                     NULL },
};

/* 01xxx110 - LD r, (HL) */
fn_row_array ld_r_ihl = {
    { "LD r, (HL) [M2]",        _read_ihl_to_z },
    { "LD r, (HL) [M3]",        _ld_r_ihl_set_mem },
    { NULL,                     NULL },
};

/* 00xx0010 - LD (rr), A */
fn_row_array ld_irr_a = {
    { "LD (rr), A [M2]",        _ld_irr_a },
    { "LD (rr), A [M3]",        _nothing },
    { NULL,                     NULL },
};

/* 00xx1010 - LD A, (rr) */
fn_row_array ld_a_irr = {
    { "LD A, (rr) [M2]",        _ld_a_irr },
    { "LD A, (rr) [M3]",        _nothing },
    { NULL,                     NULL },
};

/* 00xxx110 - LD r, imm8 */
fn_row_array ld_r_n = {
    { "LD r, imm8 [M2]",        _read_imm8_into_z },
    { "LD r, imm8 [M3]",        _ld_r_n_set_imm },
    { NULL,                     NULL }
};

/* 00110110 - LD (HL), imm8 */
fn_row_array ld_ihl_n = {
    { "LD r, imm8 [M2]",        _read_imm8_into_z },
    { "LD r, imm8 [M3]",        _write_z_to_ihl },
    { "LD r, imm8 [M4]",        _nothing },
    { NULL,                     NULL }
};

/* 11100010 - LDH (C), A */
fn_row_array ldh_ic_a = {
    { "LDH (C), A [M2]",         _write_a_to_hic },
    { "LDH (C), A [M3]",         _nothing },
    { NULL,                     NULL }
};

/* 11110010 - LDH A, (C) */
fn_row_array ldh_a_ic = {
    { "LDH A, (C) [M2]",         _read_hic_into_z },
    { "LDH A, (C) [M3]",         _write_z_to_a },
    { NULL,                     NULL }
};

/* 11100000 - LDH (imm8), A */
fn_row_array ldh_in_a = {
    { "LDH (imm8), A [M2]",      _read_imm8_into_z },
    { "LDH (imm8), A [M3]",      _write_a_to_hiz },
    { "LDH (imm8), A [M4]",      _nothing },
    { NULL,                     NULL }
};

/* 11110000 - LDH A, (imm8) */
fn_row_array ldh_a_in = {
    { "LDH A, (imm8) [M2]",      _read_imm8_into_z },
    { "LDH A, (imm8) [M3]",      _read_hiz_into_z },
    { "LDH A, (imm8) [M4]",      _write_z_to_a },
    { NULL,                     NULL }
};

/* 11101010 - LD (imm16), A */
fn_row_array ld_inn_a = {
    { "LD (imm16), A [M2]",     _read_imm8_into_z },
    { "LD (imm16), A [M3]",     _read_imm8_into_w },
    { "LD (imm16), A [M4]",     _write_a_to_iwz },
    { "LD (imm16), A [M5]",     _nothing },
    { NULL,                     NULL }
};

/* 11111010 - LD A, (imm16) */
fn_row_array ld_a_inn = {
    { "LD A, (imm16) [M2]",     _read_imm8_into_z },
    { "LD A, (imm16) [M3]",     _read_imm8_into_w },
    { "LD A, (imm16) [M4]",     _read_iwz_into_z },
    { "LD A, (imm16) [M5]",     _write_z_to_a },
    { NULL,                     NULL }
};

/* 00xx0001 - LD rr, imm16 */
fn_row_array ld_rr_nn = {
    { "LD rr, imm16 [M2]",      _read_imm8_into_z },
    { "LD rr, imm16 [M3]",      _read_imm8_into_w },
    { "LD rr, imm16 [M4]",      _ld_rr_nn },
    { NULL,                     NULL }
};

/* 00001000 - LD (imm16), SP */
fn_row_array ld_inn_sp = {
    { "LD (imm16), SP [M2]",    _read_imm8_into_z },
    { "LD (imm16), SP [M3]",    _read_imm8_into_w },
    { "LD (imm16), SP [M4]",    _write_p_to_iwz_inc },
    { "LD (imm16), SP [M5]",    _write_s_to_iwz },
    { "LD (imm16), SP [M6]",    _nothing },
    { NULL,                     NULL }
};

/* 11xx0101 - PUSH */
fn_row_array push = {
    { "PUSH [M2]",              _dec_sp },
    { "PUSH [M3]",              _push_hi },
    { "PUSH [M4]",              _push_lo },
    { "PUSH [M5]",              _nothing },
    { NULL,                     NULL }
};

/* 11xx0001 - POP */
fn_row_array pop = {
    { "POP [M2]",               _pop_lo_into_z },
    { "POP [M3]",               _pop_hi_into_w },
    { "POP [M4]",               _pop },
    { NULL,                     NULL }
};

/* 11111001 - LD SP, HL */
fn_row_array ld_sp_hl = {
    { "LD SP, HL [M2]",         _write_hl_to_sp },
    { "LD SP, HL [M3]",         _nothing },
    { NULL,                     NULL }
};

/* 11111000 - LD HL, SP+smm8 */
fn_row_array ld_hl_spe = {
    { "LD HL, SP+smm8 [M2]",    _read_imm8_into_z },
    { "LD HL, SP+smm8 [M3]",    _write_adjusted_pe_to_l },
    { "LD HL, SP+smm8 [M4]",    _write_adjusted_se_to_h },
    { NULL,                     NULL }
};

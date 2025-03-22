#include "soc/cpu_common.h"
#include "soc/instr/instrs.h"
#include "types.h"

static bool
_is_cond_true(cpu_t *cpu, uint8_t val)
{
    assert(val <= 3);
    switch (val) {
        case 0x00:
            return !FLAG_ZERO();
        case 0x01:
            return FLAG_ZERO();
        case 0x02:
            return !FLAG_CARRY();
        case 0x03:
            return FLAG_CARRY();
        default:
            unreachable();
    }
}

static void
_read_imm8_into_z(cpu_t *cpu)
{
    /* read mem into z */
    _cpu_read_imm8(cpu, &cpu->wz.lo);
}

static void
_read_imm8_into_w(cpu_t *cpu)
{
    /* read mem into w */
    _cpu_read_imm8(cpu, &cpu->wz.hi);
}

static void
_write_wz_to_pc(cpu_t *cpu)
{
    /* set PC to WZ */
    cpu->pc.val = cpu->wz.val;
}

static void
_write_wz_to_pc_enable_ime(cpu_t *cpu)
{
    /* set PC to WZ */
    cpu->pc.val = cpu->wz.val;

    /* enable IME */
    cpu->ime = true;
}

static void
_write_wz_to_pc_cc(cpu_t *cpu)
{
    /* check condition */
    bool cond = _is_cond_true(cpu, (cpu->ir & 0x18) >> 3);

    /* if cond is true, perform the move. otherwise, skip a cycle */
    if (cond)
        _write_wz_to_pc(cpu);
    else
        ++cpu->state;
}

static void
_write_hl_to_pc(cpu_t *cpu)
{
    /* write HL to PC */
    cpu->pc.val = cpu->hl.val;
}

static void
_adjust_pc_to_wz(cpu_t *cpu)
{
    /* this is weird: ALU adds low PC to Z, then the carry is used by IDU to
     * increment W. does IDU count as a OAM write in both cases (TODO)?
     * most likely, the carry "enables" the IDU to perform the increment.
     * therefore, if there's no carry, there's no _idu_write either. this,
     * however, requires testing */

    /* get the sign of Z */
    bool neg = to_bool(cpu->wz.lo & 0x80);

    /* get the carry (not using the ALU flags apparently) */
    bool carry = to_bool(((cpu->wz.lo & 0xFF) + (cpu->pc.lo & 0xFF)) & 0x100);

    /* add the Z register */
    cpu->wz.lo += cpu->pc.lo;

    /* calculate adjustment */
    uint8_t adj = 0;
    if (carry && !neg)
        adj = 1;
    else if (!carry && neg)
        adj = -1;

    /* if we adjust we enable IDU */
    cpu->wz.hi = cpu->pc.hi + adj;
    if (adj)
        _idu_write(cpu, cpu->pc.hi);
}

static void
_adjust_pc_to_wz_cc(cpu_t *cpu)
{
    /* check condition */
    bool cond = _is_cond_true(cpu, (cpu->ir & 0x18) >> 3);

    /* if cond is true proceed normally, otherwise do nothing and skip cycle */
    if (cond)
        _adjust_pc_to_wz(cpu);
    else
        ++cpu->state;
}

static void
_dec_sp(cpu_t *cpu)
{
    /* dec SP */
    _idu_write(cpu, cpu->sp.val--);
}

static void
_call_dec_sp_cc(cpu_t *cpu)
{
    /* check if condition is true */
    bool cond = _is_cond_true(cpu, (cpu->ir & 0x18) >> 3);

    /* if cond is true proceed normally, otherwise skip 3 cycles */
    if (cond)
        _dec_sp(cpu);
    else
        cpu->state += 3;
}

static void
_push_pc_1(cpu_t *cpu)
{
    /* write P (high PC) to stack and dec SP again */
    _cpu_write_byte(cpu, cpu->sp.val, cpu->pc.hi);
    _idu_write(cpu, cpu->sp.val--);
}

static void
_push_pc_2(cpu_t *cpu)
{
    /* write C (low PC) to stack and set PC to WZ */
    _cpu_write_byte(cpu, cpu->sp.val, cpu->pc.lo);
    cpu->pc.val = cpu->wz.val;
}

static void
_rst_push_pc_2(cpu_t *cpu)
{
    /* write C (low PC) to stack and set PC to one of the RST addresses */
    _cpu_write_byte(cpu, cpu->sp.val, cpu->pc.lo);
    cpu->pc.val = ((cpu->ir & 0x38) >> 3) * 0x08;
}

static void
_pop_pc_1(cpu_t *cpu)
{
    /* extract low PC into Z and increment SP */
    _cpu_read_byte(cpu, &cpu->wz.lo, cpu->sp.val);
    _idu_write(cpu, cpu->sp.val++);
}

static void
_pop_pc_2(cpu_t *cpu)
{
    /* extract high PC into W and increment SP */
    _cpu_read_byte(cpu, &cpu->wz.hi, cpu->sp.val);
    _idu_write(cpu, cpu->sp.val++);
}

static void
_ret_pop_pc_1_cc(cpu_t *cpu)
{
    /* check if cond is true */
    bool cond = _is_cond_true(cpu, (cpu->ir & 0x18) >> 3);

    /* if cond is true proceed normally, otherwise skip 3 cycles */
    if (cond)
        _pop_pc_1(cpu);
    else
        cpu->state += 3;
}

/* 11000011 - JP nn */
fn_row_array jp_nn = {
    { "JP nn [M2]",             _read_imm8_into_z },
    { "JP nn [M3]",             _read_imm8_into_w },
    { "JP nn [M4]",             _write_wz_to_pc },
    { "JP nn [M5]",             _nothing },
    { NULL,                     NULL }
};

/* 11101001 - JP HL */
fn_row_array jp_hl = {
    { "JP HL [M2]",             _write_hl_to_pc },
    { NULL,                     NULL }
};

/* 110xx010 - JP cc, nn */
fn_row_array jp_cc_nn = {
    { "JP cc, nn [M2]",         _read_imm8_into_z },
    { "JP cc, nn [M3]",         _read_imm8_into_w },
    { "JP cc, nn [M4]",         _write_wz_to_pc_cc },
    { "JP cc, nn [M5]",         _nothing },
    { NULL,                     NULL }
};

/* 00011000 - JR e */
fn_row_array jr_e = {
    { "JR e [M2]",              _read_imm8_into_z },
    { "JR e [M3]",              _adjust_pc_to_wz },
    { "JR e [M4]",              _write_wz_to_pc },
    { NULL,                     NULL }
};

/* 001xx000 - JR cc, e */
fn_row_array jr_cc_e = {
    { "JR cc, e [M2]",          _read_imm8_into_z },
    { "JR cc, e [M3]",          _adjust_pc_to_wz_cc },
    { "JR cc, e [M4]",          _write_wz_to_pc },
    { NULL,                     NULL }
};

/* 11001101 - CALL nn */
fn_row_array call_nn = {
    { "CALL nn [M2]",           _read_imm8_into_z },
    { "CALL nn [M3]",           _read_imm8_into_w },
    { "CALL nn [M4]",           _dec_sp },
    { "CALL nn [M5]",           _push_pc_1 },
    { "CALL nn [M6]",           _push_pc_2 },
    { "CALL nn [M7]",           _nothing },
    { NULL,                     NULL }
};

/* 110xx100 - CALL cc, nn */
fn_row_array call_cc_nn = {
    { "CALL cc, nn [M2]",       _read_imm8_into_z },
    { "CALL cc, nn [M3]",       _read_imm8_into_w },
    { "CALL cc, nn [M4]",       _call_dec_sp_cc },
    { "CALL cc, nn [M5]",       _push_pc_1 },
    { "CALL cc, nn [M6]",       _push_pc_2 },
    { "CALL cc, nn [M7]",       _nothing },
    { NULL,                     NULL }
};

/* 11001001 - RET */
fn_row_array ret = {
    { "RET [M1]",               _pop_pc_1 },
    { "RET [M2]",               _pop_pc_2 },
    { "RET [M3]",               _write_wz_to_pc },
    { "RET [M4]",               _nothing },
    { NULL,                     NULL }
};

/* 110xx000 - RET cc */
fn_row_array ret_cc = {
    { "RET cc [M2]",            _nothing },
    { "RET cc [M3]",            _ret_pop_pc_1_cc },
    { "RET cc [M4]",            _pop_pc_2 },
    { "RET cc [M5]",            _write_wz_to_pc },
    { "RET cc [M6]",            _nothing },
    { NULL,                     NULL }
};

/* 11011001 - RETI */
fn_row_array reti = {
    { "RETI [M1]",              _pop_pc_1 },
    { "RETI [M2]",              _pop_pc_2 },
    { "RETI [M3]",              _write_wz_to_pc_enable_ime },
    { "RETI [M4]",              _nothing },
    { NULL,                     NULL }
};

/* 11xxx111 - RST n */
fn_row_array rst_n = {
    { "RST n [M2]",             _dec_sp },
    { "RST n [M3]",             _push_pc_1 },
    { "RST n [M4]",             _rst_push_pc_2 },
    { "RST n [M5]",             _nothing },
    { NULL,                     NULL }
};

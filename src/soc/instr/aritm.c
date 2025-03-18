#include "soc/cpu_common.h"
#include "soc/instr/instrs.h"
#include "types.h"

static inline uint8_t *
_get_r(cpu_t *cpu, uint8_t val)
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
_get_rr(cpu_t *cpu, uint8_t val)
{
    /* based on value, choose a register */
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

static void
_read_imm8_into_z(cpu_t *cpu)
{
    /* read imm8 into Z */
    _cpu_read_imm8(cpu, &cpu->wz.lo);
}

static void
_read_ihl_into_z(cpu_t *cpu)
{
    /* read (HL) into Z */
    _cpu_read_byte(cpu, &cpu->wz.lo, cpu->hl.val);
}

static void
_add_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    ADD8(cpu->af.hi, *reg);
}

static void
_add_z(cpu_t *cpu)
{
    /* use Z register (previously read) */
    ADD8(cpu->af.hi, cpu->wz.lo);
}

static void
_adc_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    ADC8(cpu->af.hi, *reg, tmp);
}

static void
_adc_z(cpu_t *cpu)
{
    /* use Z register (previously read) */
    uint8_t tmp;
    ADC8(cpu->af.hi, cpu->wz.lo, tmp);
}

static void
_sub_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    SUB8(cpu->af.hi, *reg);
}

static void
_sub_z(cpu_t *cpu)
{
    /* use Z register (previously read) */
    SUB8(cpu->af.hi, cpu->wz.lo);
}

static void
_sbc_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    SBC8(cpu->af.hi, *reg, tmp);
}

static void
_sbc_z(cpu_t *cpu)
{
    /* use Z register (previously read) */
    uint8_t tmp;
    SBC8(cpu->af.hi, cpu->wz.lo, tmp);
}

static void
_cp_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    CP8(cpu->af.hi, *reg);
}

static void
_cp_z(cpu_t *cpu)
{
    /* use Z register (previously read) */
    CP8(cpu->af.hi, cpu->wz.lo);
}

static void
_inc_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, (cpu->ir & 0x38) >> 3);
    INC8(*reg);
}

static void
_inc_z_into_ihl(cpu_t *cpu)
{
    /* increment Z and write to (HL) */
    uint8_t z = cpu->wz.lo;
    INC8(z);
    _cpu_write_byte(cpu, cpu->hl.val, z);
}

static void
_dec_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, (cpu->ir & 0x38) >> 3);
    DEC8(*reg);
}

static void
_dec_z_into_ihl(cpu_t *cpu)
{
    uint8_t z = cpu->wz.lo;
    DEC8(z);
    _cpu_write_byte(cpu, cpu->hl.val, z);
}

static void
_and_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    AND8(cpu->af.hi, *reg);
}

static void
_and_z(cpu_t *cpu)
{
    /* use Z register */
    AND8(cpu->af.hi, cpu->wz.lo);
}

static void
_or_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    OR8(cpu->af.hi, *reg);
}

static void
_or_z(cpu_t *cpu)
{
    /* use Z register */
    OR8(cpu->af.hi, cpu->wz.lo);
}

static void
_xor_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    XOR8(cpu->af.hi, *reg);
}

static void
_xor_z(cpu_t *cpu)
{
    /* use Z register */
    XOR8(cpu->af.hi, cpu->wz.lo);
}

static void
_ccf(cpu_t *cpu)
{
    UNSET_SUB();
    UNSET_HALF();
    if (FLAG_CARRY())
        UNSET_CARRY();
    else
        SET_CARRY();
}

static void
_scf(cpu_t *cpu)
{
    UNSET_SUB();
    UNSET_HALF();
    SET_CARRY();
}

static void
_daa(cpu_t *cpu)
{
    if (!FLAG_SUB()) {
        if (FLAG_CARRY() || cpu->af.hi > 0x99) {
            cpu->af.hi += 0x60;
            SET_CARRY();
        }
        if (FLAG_HALF() || (cpu->af.hi & 0x0F) > 0x09)
            cpu->af.hi += 0x06;
    } else {
        if (FLAG_CARRY())
            cpu->af.hi -= 0x60;
        if (FLAG_HALF())
            cpu->af.hi -= 0x06;
    }

    if (cpu->af.hi == 0)
        SET_ZERO();
    else
        UNSET_ZERO();

    UNSET_HALF();
}

static void
_cpl(cpu_t *cpu)
{
    cpu->af.hi = ~cpu->af.hi;
    SET_SUB();
    SET_HALF();
}

static void
_inc_rr(cpu_t *cpu)
{
    /* inc reg with IDU */
    reg_t *reg = _get_rr(cpu, (cpu->ir & 0x30) >> 4);
    _idu_write(cpu, reg->val++);
}

static void
_dec_rr(cpu_t *cpu)
{
    /* dec reg with IDU */
    reg_t *reg = _get_rr(cpu, (cpu->ir & 0x30) >> 4);
    _idu_write(cpu, reg->val--);
}

static void
_add_lr_to_l(cpu_t *cpu)
{
    /* add low register to L */
    reg_t *reg = _get_rr(cpu, (cpu->ir & 0x30) >> 4);

    /* the zero flag is unaffected. this means we must not use the comfortable
     * ADD8() macro for a normal ALU op */
    if (((cpu->hl.lo & 0xf) + (reg->lo & 0xf)) & 0x10)
        SET_HALF();
    else
        UNSET_HALF();

    if (((cpu->hl.lo & 0xff) + (reg->lo & 0xff)) & 0x100)
        SET_CARRY();
    else
        UNSET_CARRY();

    UNSET_SUB();

    /* perform the op */
    cpu->hl.lo += reg->lo;
}

static void
_add_hr_to_h(cpu_t *cpu)
{
    /* add high register to H with carry */
    reg_t *reg = _get_rr(cpu, (cpu->ir & 0x30) >> 4);

    /* the zero flag is unaffected. this means we must not use the comfortable
     * ADD8() macro for a normal ALU op */
    uint8_t carry = FLAG_CARRY();

    if (((cpu->hl.hi & 0xf) + (reg->hi & 0xf) + carry) & 0x10)
        SET_HALF();
    else
        UNSET_HALF();

    if (((cpu->hl.hi & 0xff) + (reg->hi & 0xff) + carry) & 0x100)
        SET_CARRY();
    else
        UNSET_CARRY();

    UNSET_SUB();

    /* perform the op */
    cpu->hl.hi += reg->hi + carry;
}


static void
_write_pe_to_z(cpu_t *cpu)
{
    /* write adjusted P (low SP) to Z */

    /* the flags zero and sub are reset. BUT! there's no way for the next
     * operation in the pipeline to know the sign of Z (since we are just about
     * to overwrite it), therefore we will temporarily borrow the sub flag to
     * carry the sign of the offset. after calculating our adjustment, we make
     * sure to unset it. */
    if (((cpu->sp.lo & 0xf) + (cpu->wz.lo & 0xf)) & 0x10)
        SET_HALF();
    else
        UNSET_HALF();

    if (((cpu->sp.lo & 0xff) + (cpu->wz.lo & 0xff)) & 0x100)
        SET_CARRY();
    else
        UNSET_CARRY();

    if (cpu->wz.lo & 0x80)
        SET_SUB();
    else
        UNSET_SUB();

    UNSET_ZERO();

    /* perform the op */
    cpu->wz.lo = cpu->sp.lo + cpu->wz.lo;
}

static void
_write_se_to_w(cpu_t *cpu)
{
    /* write adjusted S (high SP) to W (only carry) */

    /* adjustment depends on the sign of Z, which as explained in the previous
     * operation, is carried in the sub flag. */
    uint8_t adj = 0;
    if (FLAG_SUB())
        adj = -1;
    UNSET_SUB();

    /* perform the op */
    cpu->wz.hi = cpu->sp.hi + adj + FLAG_CARRY();
}

static void
_write_wz_to_sp(cpu_t *cpu)
{
    /* self explanatory innit */
    cpu->sp.val = cpu->wz.val;
}

/* 10000xxx - ADD r */
fn_row_array add_r = {
    { "ADD r [M2]",             _add_r },
    { NULL,                     NULL }
};

/* 10000110 - ADD (HL) */
fn_row_array add_ihl = {
    { "ADD (HL) [M2]",          _read_ihl_into_z },
    { "ADD (HL) [M3]",          _add_z },
    { NULL,                     NULL }
};

/* 11000110 - ADD n */
fn_row_array add_n = {
    { "ADD n [M2]",             _read_imm8_into_z },
    { "ADD n [M3]",             _add_z },
    { NULL,                     NULL }
};

/* 10001xxx - ADC r */
fn_row_array adc_r = {
    { "ADC r [M2]",             _adc_r },
    { NULL,                     NULL }
};

/* 10001110 - ADC (HL) */
fn_row_array adc_ihl = {
    { "ADC (HL) [M2]",          _read_ihl_into_z },
    { "ADC (HL) [M3]",          _adc_z },
    { NULL,                     NULL }
};

/* 11001110 - ADC n */
fn_row_array adc_n = {
    { "ADC n [M2]",             _read_imm8_into_z },
    { "ADC n [M3]",             _adc_z },
    { NULL,                     NULL }
};

/* 10010xxx - SUB r */
fn_row_array sub_r = {
    { "SUB r [M2]",             _sub_r },
    { NULL,                     NULL }
};

/* 10010110 - SUB (HL) */
fn_row_array sub_ihl = {
    { "SUB (HL) [M2]",          _read_ihl_into_z },
    { "SUB (HL) [M3]",          _sub_z },
    { NULL,                     NULL }
};

/* 11010110 - SUB n */
fn_row_array sub_n = {
    { "SUB n [M2]",             _read_imm8_into_z },
    { "SUB n [M3]",             _sub_z },
    { NULL,                     NULL }
};

/* 10011xxx - SBC r */
fn_row_array sbc_r = {
    { "SBC r [M2]",             _sbc_r },
    { NULL,                     NULL }
};

/* 10011110 - SBC (HL) */
fn_row_array sbc_ihl = {
    { "SBC (HL) [M2]",          _read_ihl_into_z },
    { "SBC (HL) [M3]",          _sbc_z },
    { NULL,                     NULL }
};

/* 11011110 - SBC n */
fn_row_array sbc_n = {
    { "SBC n [M2]",             _read_imm8_into_z },
    { "SBC n [M3]",             _sbc_z },
    { NULL,                     NULL }
};

/* 10111xxx - CP r */
fn_row_array cp_r = {
    { "CP r [M2]",              _cp_r },
    { NULL,                     NULL }
};

/* 10111110 - CP (HL) */
fn_row_array cp_ihl = {
    { "CP (HL) [M2]",           _read_ihl_into_z },
    { "CP (HL) [M3]",           _cp_z },
    { NULL,                     NULL }
};

/* 11111110 - CP n */
fn_row_array cp_n = {
    { "CP n [M2]",              _read_imm8_into_z },
    { "CP n [M3]",              _cp_z },
    { NULL,                     NULL }
};

/* 00xxx100 - INC r */
fn_row_array inc_r = {
    { "INC r [M2]",             _inc_r },
    { NULL,                     NULL }
};

/* 00110100 - INC (HL) */
fn_row_array inc_ihl = {
    { "INC (HL) [M2]",          _read_ihl_into_z },
    { "INC (HL) [M3]",          _inc_z_into_ihl },
    { "INC (HL) [M4]",          _nothing },
    { NULL,                     NULL }
};

/* 00xxx101 - DEC r */
fn_row_array dec_r = {
    { "DEC r [M2]",             _dec_r },
    { NULL,                     NULL }
};

/* 00110101 - DEC (HL) */
fn_row_array dec_ihl = {
    { "DEC (HL) [M2]",          _read_ihl_into_z },
    { "DEC (HL) [M3]",          _dec_z_into_ihl },
    { "DEC (HL) [M4]",          _nothing },
    { NULL,                     NULL }
};

/* 10100xxx - AND r */
fn_row_array and_r = {
    { "AND r [M2]",             _and_r },
    { NULL,                     NULL }
};

/* 10100110 - AND (HL) */
fn_row_array and_ihl = {
    { "AND (HL) [M2]",          _read_ihl_into_z },
    { "AND (HL) [M3]",          _and_z },
    { NULL,                     NULL }
};

/* 11100110 - AND n */
fn_row_array and_n = {
    { "AND n [M2]",             _read_imm8_into_z },
    { "AND n [M3]",             _and_z },
    { NULL,                     NULL }
};

/* 10110xxx - OR r */
fn_row_array or_r = {
    { "OR r [M2]",              _or_r },
    { NULL,                     NULL }
};

/* 10110110 - OR (HL) */
fn_row_array or_ihl = {
    { "OR (HL) [M2]",           _read_ihl_into_z },
    { "OR (HL) [M3]",           _or_z },
    { NULL,                     NULL }
};

/* 11110110 - OR n */
fn_row_array or_n = {
    { "OR n [M2]",              _read_imm8_into_z },
    { "OR n [M3]",              _or_z },
    { NULL,                     NULL }
};

/* 10101xxx - XOR r */
fn_row_array xor_r = {
    { "XOR r [M2]",             _xor_r },
    { NULL,                     NULL }
};

/* 10101110 - XOR (HL) */
fn_row_array xor_ihl = {
    { "XOR (HL) [M2]",          _read_ihl_into_z },
    { "XOR (HL) [M3]",          _xor_z },
    { NULL,                     NULL }
};

/* 11101110 - XOR n */
fn_row_array xor_n = {
    { "XOR n [M2]",             _read_imm8_into_z },
    { "XOR n [M3]",             _xor_z },
    { NULL,                     NULL }
};

/* 00111111 - CCF */
fn_row_array ccf = {
    { "CCF [M2]",               _ccf },
    { NULL,                     NULL }
};

/* 00110111 - SCF */
fn_row_array scf = {
    { "SCF [M2]",               _scf },
    { NULL,                     NULL }
};

/* 00100111 - DAA */
fn_row_array daa = {
    { "DAA [M2]",               _daa },
    { NULL,                     NULL }
};

/* 00101111 - CPL */
fn_row_array cpl = {
    { "CPL [M2]",               _cpl },
    { NULL,                     NULL }
};

/* 00xx0011 - INC rr */
fn_row_array inc_rr =  {
    { "INC rr [M2]",            _inc_rr },
    { "INC rr [M3]",            _nothing },
    { NULL,                     NULL }
};

/* 00xx1011 - DEC rr */
fn_row_array dec_rr = {
    { "DEC rr [M2]",            _dec_rr },
    { "DEC rr [M3]",            _nothing },
    { NULL,                     NULL }
};

/* 00xx1001 - ADD HL, rr */
fn_row_array add_hl_rr = {
    { "ADD HL, rr [M2]",        _add_lr_to_l },
    { "ADD HL, rr [M3]",        _add_hr_to_h },
    { NULL,                     NULL }
};

/* 11101000 - ADD SP, smm8 */
fn_row_array add_sp_e = {
    { "ADD SP, smm8 [M2]",      _read_imm8_into_z },
    { "ADD SP, smm8 [M3]",      _write_pe_to_z },
    { "ADD SP, smm8 [M4]",      _write_se_to_w },
    { "ADD SP, smm8 [M5]",      _write_wz_to_sp },
    { NULL,                     NULL }
};

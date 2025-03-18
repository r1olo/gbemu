#include "soc/cpu_common.h"
#include "soc/instr/instrs.h"
#include "types.h"

static void
_read_ihl_into_z(cpu_t *cpu)
{
    /* read (HL) into Z */
    _cpu_read_byte(cpu, &cpu->wz.lo, cpu->hl.val);
}

static uint8_t *
_get_r(cpu_t *cpu, uint8_t val)
{
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

static void
_rlca(cpu_t *cpu)
{
    /* apparently, RLCA is a variation of normal RLC that always resets the zero
     * flag */
    uint8_t tmp;
    RLC8(cpu->af.hi, tmp);
    UNSET_ZERO();
}

static void
_rrca(cpu_t *cpu)
{
    /* apparently, RRCA is a variation of normal RRC that always resets the zero
     * flag */
    uint8_t tmp;
    RRC8(cpu->af.hi, tmp);
    UNSET_ZERO();
}

static void
_rla(cpu_t *cpu)
{
    /* apparently, RLA is a variation of normal RL that always resets the zero
     * flag */
    uint8_t tmp;
    RL8(cpu->af.hi, tmp);
    UNSET_ZERO();
}

static void
_rra(cpu_t *cpu)
{
    /* apparently, RRA is a variation of normal RR that always resets the zero
     * flag */
    uint8_t tmp;
    RR8(cpu->af.hi, tmp);
    UNSET_ZERO();
}

static void
_rlc_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    RLC8(*reg, tmp);
}

static void
_rlc_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    RLC8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_rrc_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    RRC8(*reg, tmp);
}

static void
_rrc_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    RRC8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_rl_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    RL8(*reg, tmp);
}

static void
_rl_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    RL8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_rr_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    RR8(*reg, tmp);
}

static void
_rr_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    RR8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_sla_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    SLA8(*reg, tmp);
}

static void
_sla_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    SLA8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_sra_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    SRA8(*reg, tmp);
}

static void
_sra_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    SRA8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_swap_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    SWAP8(*reg, tmp);
}

static void
_swap_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    SWAP8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_srl_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07), tmp;
    SRL8(*reg, tmp);
}

static void
_srl_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo, tmp;
    SRL8(res, tmp);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_bit_b_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    uint8_t bit = (cpu->ir & 0x38) >> 3;
    BIT8(bit, *reg);
}

static void
_bit_b_z(cpu_t *cpu)
{
    uint8_t bit = (cpu->ir & 0x38) >> 3;
    BIT8(bit, cpu->wz.lo);
}

static void
_res_b_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    uint8_t bit = (cpu->ir & 0x38) >> 3;
    RES8(bit, *reg);
}

static void
_res_b_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo;
    uint8_t bit = (cpu->ir & 0x38) >> 3;
    RES8(bit, res);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

static void
_set_b_r(cpu_t *cpu)
{
    uint8_t *reg = _get_r(cpu, cpu->ir & 0x07);
    uint8_t bit = (cpu->ir & 0x38) >> 3;
    SET8(bit, *reg);
}

static void
_set_b_z_into_ihl(cpu_t *cpu)
{
    uint8_t res = cpu->wz.lo;
    uint8_t bit = (cpu->ir & 0x38) >> 3;
    SET8(bit, res);
    _cpu_write_byte(cpu, cpu->hl.val, res);
}

fn_row_array rlca = {
    { "RLCA [M3]",              _rlca },
    { NULL,                     NULL }
};

fn_row_array rrca = {
    { "RRCA [M3]",              _rrca },
    { NULL,                     NULL }
};

fn_row_array rla = {
    { "RLA [M3]",               _rla },
    { NULL,                     NULL }
};

fn_row_array rra = {
    { "RRA [M3]",               _rra },
    { NULL,                     NULL }
};

fn_row_array rlc_r = {
    { NULL,                     NULL },
    { "RLC r [M3]",             _rlc_r },
    { NULL,                     NULL }
};

fn_row_array rlc_ihl = {
    { NULL,                     NULL },
    { "RLC (HL) [M3]",          _read_ihl_into_z },
    { "RLC (HL) [M4]",          _rlc_z_into_ihl },
    { "RLC (HL) [M5]",          _nothing },
    { NULL,                     NULL }
};

fn_row_array rrc_r = {
    { NULL,                     NULL },
    { "RRC r [M3]",             _rrc_r },
    { NULL,                     NULL }
};

fn_row_array rrc_ihl = {
    { NULL,                     NULL },
    { "RRC (HL) [M3]",          _read_ihl_into_z },
    { "RRC (HL) [M4]",          _rrc_z_into_ihl },
    { "RRC (HL) [M5]",          _nothing },
    { NULL,                     NULL }
};

fn_row_array rl_r = {
    { NULL,                     NULL },
    { "RL r [M3]",              _rl_r },
    { NULL,                     NULL }
};

fn_row_array rl_ihl = {
    { NULL,                     NULL },
    { "RL (HL) [M3]",           _read_ihl_into_z },
    { "RL (HL) [M4]",           _rl_z_into_ihl },
    { "RL (HL) [M5]",           _nothing },
    { NULL,                     NULL }
};

fn_row_array rr_r = {
    { NULL,                     NULL },
    { "RR r [M3]",              _rr_r },
    { NULL,                     NULL }
};

fn_row_array rr_ihl = {
    { NULL,                     NULL },
    { "RR (HL) [M3]",           _read_ihl_into_z },
    { "RR (HL) [M4]",           _rr_z_into_ihl },
    { "RR (HL) [M5]",           _nothing },
    { NULL,                     NULL }
};

fn_row_array sla_r = {
    { NULL,                     NULL },
    { "SLA r [M3]",             _sla_r },
    { NULL,                     NULL }
};

fn_row_array sla_ihl = {
    { NULL,                     NULL },
    { "SLA (HL) [M3]",          _read_ihl_into_z },
    { "SLA (HL) [M4]",          _sla_z_into_ihl },
    { "SLA (HL) [M5]",          _nothing },
    { NULL,                     NULL }
};

fn_row_array sra_r = {
    { NULL,                     NULL },
    { "SRA r [M3]",             _sra_r },
    { NULL,                     NULL }
};

fn_row_array sra_ihl = {
    { NULL,                     NULL },
    { "SRA (HL) [M3]",          _read_ihl_into_z },
    { "SRA (HL) [M4]",          _sra_z_into_ihl },
    { "SRA (HL) [M5]",          _nothing },
    { NULL,                     NULL }
};

fn_row_array swap_r = {
    { NULL,                     NULL },
    { "SWAP r [M3]",            _swap_r },
    { NULL,                     NULL }
};

fn_row_array swap_ihl = {
    { NULL,                     NULL },
    { "SWAP (HL) [M3]",         _read_ihl_into_z },
    { "SWAP (HL) [M4]",         _swap_z_into_ihl },
    { "SWAP (HL) [M5]",         _nothing },
    { NULL,                     NULL }
};

fn_row_array srl_r = {
    { NULL,                     NULL },
    { "SRL r [M3]",             _srl_r },
    { NULL,                     NULL }
};

fn_row_array srl_ihl = {
    { NULL,                     NULL },
    { "SRL (HL) [M3]",          _read_ihl_into_z },
    { "SRL (HL) [M4]",          _srl_z_into_ihl },
    { "SRL (HL) [M5]",          _nothing },
    { NULL,                     NULL }
};

fn_row_array bit_b_r = {
    { NULL,                     NULL },
    { "BIT b, n [M3]",          _bit_b_r },
    { NULL,                     NULL }
};

fn_row_array bit_b_ihl = {
    { NULL,                     NULL },
    { "BIT b, (HL) [M3]",       _read_ihl_into_z },
    { "BIT b, (HL) [M4]",       _bit_b_z },
    { NULL,                     NULL }
};

fn_row_array res_b_r = {
    { NULL,                     NULL },
    { "RES b, n [M3]",          _res_b_r },
    { NULL,                     NULL }
};

fn_row_array res_b_ihl = {
    { NULL,                     NULL },
    { "RES b, (HL) [M3]",       _read_ihl_into_z },
    { "RES b, (HL) [M4]",       _res_b_z_into_ihl },
    { "RES b, (HL) [M5]",       _nothing },
    { NULL,                     NULL }
};

fn_row_array set_b_r = {
    { NULL,                     NULL },
    { "SET b, n [M3]",          _set_b_r },
    { NULL,                     NULL }
};

fn_row_array set_b_ihl = {
    { NULL,                     NULL },
    { "SET b, (HL) [M3]",       _read_ihl_into_z },
    { "SET b, (HL) [M4]",       _set_b_z_into_ihl },
    { "SET b, (HL) [M5]",       _nothing },
    { NULL,                     NULL }
};

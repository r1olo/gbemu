#include "soc/instr/instrs.h"

void
_nothing(cpu_t *cpu)
{
    /* explanation of this overly complicated function:
     *
     * basically, */
}

/* big matrix ahead */
fn_row_array_ptrs instructions = {
/*  0x00         0x01         0x02         0x03         0x04         0x05         0x06         0x07  */
    &nop,        &ld_rr_nn,   &ld_irr_a,   &inc_rr,     &inc_r,      &dec_r,      &ld_r_n,     &rlca,
/*  0x08         0x09         0x0A         0x0B         0x0C         0x0D         0x0E         0x0F  */
    &ld_inn_sp,  &add_hl_rr,  &ld_a_irr,   &dec_rr,     &inc_r,      &dec_r,      &ld_r_n,     &rrca,

/*  0x10         0x11         0x12         0x13         0x14         0x15         0x16         0x17  */
    &stop,       &ld_rr_nn,   &ld_irr_a,   &inc_rr,     &inc_r,      &dec_r,      &ld_r_n,     &rla,
/*  0x18         0x19         0x1A         0x1B         0x1C         0x1D         0x1E         0x1F  */
    &jr_e,       &add_hl_rr,  &ld_a_irr,   &dec_rr,     &inc_r,      &dec_r,      &ld_r_n,     &rra,

/*  0x20         0x21         0x22         0x23         0x24         0x25         0x26         0x27  */
    &jr_cc_e,    &ld_rr_nn,   &ld_irr_a,   &inc_rr,     &inc_r,      &dec_r,      &ld_r_n,     &daa,
/*  0x28         0x29         0x2A         0x2B         0x2C         0x2D         0x2E         0x2F  */
    &jr_cc_e,    &add_hl_rr,  &ld_a_irr,   &dec_rr,     &inc_r,      &dec_r,      &ld_r_n,     &cpl,

/*  0x30         0x31         0x32         0x33         0x34         0x35         0x36         0x37  */
    &jr_cc_e,    &ld_rr_nn,   &ld_irr_a,   &inc_rr,     &inc_ihl,    &dec_ihl,    &ld_ihl_n,   &scf,
/*  0x38         0x39         0x3A         0x3B         0x3C         0x3D         0x3E         0x3F  */
    &jr_cc_e,    &add_hl_rr,  &ld_a_irr,   &dec_rr,     &inc_r,      &dec_r,      &ld_r_n,     &ccf,

/*  0x40         0x41         0x42         0x43         0x44         0x45         0x46         0x47  */
    &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_ihl,   &ld_r_r,
/*  0x48         0x49         0x4A         0x4B         0x4C         0x4D         0x4E         0x4F  */
    &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_ihl,   &ld_r_r,

/*  0x50         0x51         0x52         0x53         0x54         0x55         0x56         0x57  */
    &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_ihl,   &ld_r_r,
/*  0x58         0x59         0x5A         0x5B         0x5C         0x5D         0x5E         0x5F  */
    &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_ihl,   &ld_r_r,

/*  0x60         0x61         0x62         0x63         0x64         0x65         0x66         0x67  */
    &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_ihl,   &ld_r_r,
/*  0x68         0x69         0x6A         0x6B         0x6C         0x6D         0x6E         0x6F  */
    &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_ihl,   &ld_r_r,

/*  0x70         0x71         0x72         0x73         0x74         0x75         0x76         0x77  */
    &ld_ihl_r,   &ld_ihl_r,   &ld_ihl_r,   &ld_ihl_r,   &ld_ihl_r,   &ld_ihl_r,   &halt,       &ld_ihl_r,
/*  0x78         0x79         0x7A         0x7B         0x7C         0x7D         0x7E         0x7F  */
    &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_r,     &ld_r_ihl,   &ld_r_r,

/*  0x80         0x81         0x82         0x83         0x84         0x85         0x86         0x87  */
    &add_r,      &add_r,      &add_r,      &add_r,      &add_r,      &add_r,      &add_ihl,    &add_r,
/*  0x88         0x89         0x8A         0x8B         0x8C         0x8D         0x8E         0x8F  */
    &adc_r,      &adc_r,      &adc_r,      &adc_r,      &adc_r,      &adc_r,      &adc_ihl,    &adc_r,

/*  0x90         0x91         0x92         0x93         0x94         0x95         0x96         0x97  */
    &sub_r,      &sub_r,      &sub_r,      &sub_r,      &sub_r,      &sub_r,      &sub_ihl,    &sub_r,
/*  0x98         0x99         0x9A         0x9B         0x9C         0x9D         0x9E         0x9F  */
    &sbc_r,      &sbc_r,      &sbc_r,      &sbc_r,      &sbc_r,      &sbc_r,      &sbc_ihl,    &sbc_r,

/*  0xA0         0xA1         0xA2         0xA3         0xA4         0xA5         0xA6         0xA7  */
    &and_r,      &and_r,      &and_r,      &and_r,      &and_r,      &and_r,      &and_ihl,    &and_r,
/*  0xA8         0xA9         0xAA         0xAB         0xAC         0xAD         0xAE         0xAF  */
    &xor_r,      &xor_r,      &xor_r,      &xor_r,      &xor_r,      &xor_r,      &xor_ihl,    &xor_r,

/*  0xB0         0xB1         0xB2         0xB3         0xB4         0xB5         0xB6         0xB7  */
    &or_r,       &or_r,       &or_r,       &or_r,       &or_r,       &or_r,       &or_ihl,     &or_r,
/*  0xB8         0xB9         0xBA         0xBB         0xBC         0xBD         0xBE         0xBF  */
    &cp_r,       &cp_r,       &cp_r,       &cp_r,       &cp_r,       &cp_r,       &cp_ihl,     &cp_r,

/*  0xC0         0xC1         0xC2         0xC3         0xC4         0xC5         0xC6         0xC7  */
    &ret_cc,     &pop,        &jp_cc_nn,   &jp_nn,      &call_cc_nn, &push,       &add_n,      &rst_n,
/*  0xC8         0xC9         0xCA         0xCB         0xCC         0xCD         0xCE         0xCF  */
    &ret_cc,     &ret,        &jp_cc_nn,   &prefix,     &call_cc_nn, &call_nn,    &adc_n,      &rst_n,

/*  0xD0         0xD1         0xD2         0xD3         0xD4         0xD5         0xD6         0xD7  */
    &ret_cc,     &pop,        &jp_cc_nn,   &not_impl,   &call_cc_nn, &push,       &sub_n,      &rst_n,
/*  0xD8         0xD9         0xDA         0xDB         0xDC         0xDD         0xDE         0xDF  */
    &ret_cc,     &reti,       &jp_cc_nn,   &not_impl,   &call_cc_nn, &not_impl,   &sbc_n,      &rst_n,

/*  0xE0         0xE1         0xE2         0xE3         0xE4         0xE5         0xE6         0xE7  */
    &ldh_in_a ,  &pop,        &ldh_ic_a,   &not_impl,   &not_impl,   &push,       &and_n,      &rst_n,
/*  0xE8         0xE9         0xEA         0xEB         0xEC         0xED         0xEE         0xEF  */
    &add_sp_e,   &jp_hl,      &ld_inn_a,   &not_impl,   &not_impl,   &not_impl,   &xor_n,      &rst_n,

/*  0xF0         0xF1         0xF2         0xF3         0xF4         0xF5         0xF6         0xF7  */
    &ldh_a_in ,  &pop,        &ldh_a_ic,   &di,         &not_impl,   &push,       &or_n,       &rst_n,
/*  0xF8         0xF9         0xFA         0xFB         0xFC         0xFD         0xFE         0xFF  */
    &ld_hl_spe,  &ld_sp_hl,   &ld_a_inn,   &ei,         &not_impl,   &not_impl,   &cp_n,       &rst_n,
};

fn_row_array_ptrs cb_instructions = {
/*  0x00         0x01         0x02         0x03         0x04         0x05         0x06         0x07  */
    &rlc_r,      &rlc_r,      &rlc_r,      &rlc_r,      &rlc_r,      &rlc_r,      &rlc_ihl,    &rlc_r,
/*  0x08         0x09         0x0A         0x0B         0x0C         0x0D         0x0E         0x0F  */
    &rrc_r,      &rrc_r,      &rrc_r,      &rrc_r,      &rrc_r,      &rrc_r,      &rrc_ihl,    &rrc_r,

/*  0x10         0x11         0x12         0x13         0x14         0x15         0x16         0x17  */
    &rl_r,       &rl_r,       &rl_r,       &rl_r,       &rl_r,       &rl_r,       &rl_ihl,     &rl_r,
/*  0x18         0x19         0x1A         0x1B         0x1C         0x1D         0x1E         0x1F  */
    &rr_r,       &rr_r,       &rr_r,       &rr_r,       &rr_r,       &rr_r,       &rr_ihl,     &rr_r,

/*  0x20         0x21         0x22         0x23         0x24         0x25         0x26         0x27  */
    &sla_r,      &sla_r,      &sla_r,      &sla_r,      &sla_r,      &sla_r,      &sla_ihl,    &sla_r,
/*  0x28         0x29         0x2A         0x2B         0x2C         0x2D         0x2E         0x2F  */
    &sra_r,      &sra_r,      &sra_r,      &sra_r,      &sra_r,      &sra_r,      &sra_ihl,    &sra_r,

/*  0x30         0x31         0x32         0x33         0x34         0x35         0x36         0x37  */
    &swap_r,     &swap_r,     &swap_r,     &swap_r,     &swap_r,     &swap_r,     &swap_ihl,   &swap_r,
/*  0x38         0x39         0x3A         0x3B         0x3C         0x3D         0x3E         0x3F  */
    &srl_r,      &srl_r,      &srl_r,      &srl_r,      &srl_r,      &srl_r,      &srl_ihl,    &srl_r,

/*  0x40         0x41         0x42         0x43         0x44         0x45         0x46         0x47  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,
/*  0x48         0x49         0x4A         0x4B         0x4C         0x4D         0x4E         0x4F  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,

/*  0x50         0x51         0x52         0x53         0x54         0x55         0x56         0x57  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,
/*  0x58         0x59         0x5A         0x5B         0x5C         0x5D         0x5E         0x5F  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,

/*  0x60         0x61         0x62         0x63         0x64         0x65         0x66         0x67  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,
/*  0x68         0x69         0x6A         0x6B         0x6C         0x6D         0x6E         0x6F  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,

/*  0x70         0x71         0x72         0x73         0x74         0x75         0x76         0x77  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,
/*  0x78         0x79         0x7A         0x7B         0x7C         0x7D         0x7E         0x7F  */
    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_r,    &bit_b_ihl,  &bit_b_r,

/*  0x80         0x81         0x82         0x83         0x84         0x85         0x86         0x87  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,
/*  0x88         0x89         0x8A         0x8B         0x8C         0x8D         0x8E         0x8F  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,

/*  0x90         0x91         0x92         0x93         0x94         0x95         0x96         0x97  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,
/*  0x98         0x99         0x9A         0x9B         0x9C         0x9D         0x9E         0x9F  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,

/*  0xA0         0xA1         0xA2         0xA3         0xA4         0xA5         0xA6         0xA7  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,
/*  0xA8         0xA9         0xAA         0xAB         0xAC         0xAD         0xAE         0xAF  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,

/*  0xB0         0xB1         0xB2         0xB3         0xB4         0xB5         0xB6         0xB7  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,
/*  0xB8         0xB9         0xBA         0xBB         0xBC         0xBD         0xBE         0xBF  */
    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_r,    &res_b_ihl,  &res_b_r,

/*  0xC0         0xC1         0xC2         0xC3         0xC4         0xC5         0xC6         0xC7  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,
/*  0xC8         0xC9         0xCA         0xCB         0xCC         0xCD         0xCE         0xCF  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,

/*  0xD0         0xD1         0xD2         0xD3         0xD4         0xD5         0xD6         0xD7  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,
/*  0xD8         0xD9         0xDA         0xDB         0xDC         0xDD         0xDE         0xDF  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,

/*  0xE0         0xE1         0xE2         0xE3         0xE4         0xE5         0xE6         0xE7  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,
/*  0xE8         0xE9         0xEA         0xEB         0xEC         0xED         0xEE         0xEF  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,

/*  0xF0         0xF1         0xF2         0xF3         0xF4         0xF5         0xF6         0xF7  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,
/*  0xF8         0xF9         0xFA         0xFB         0xFC         0xFD         0xFE         0xFF  */
    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_r,    &set_b_ihl,  &set_b_r,
};

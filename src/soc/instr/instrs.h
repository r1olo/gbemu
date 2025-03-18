#ifndef __INSTRS_H
#define __INSTRS_H

#include "soc/soc.h"

/* a function that does nothing */
void _nothing(cpu_t *cpu);

/* ld.c */
extern fn_row_array ld_r_r;     /* LD r, r' */
extern fn_row_array ld_ihl_r;   /* LD (HL), r */
extern fn_row_array ld_r_ihl;   /* LD r, (HL) */
extern fn_row_array ld_irr_a;   /* LD (rr), A */
extern fn_row_array ld_a_irr;   /* LD A, (rr) */
extern fn_row_array ld_r_n;     /* LD r, imm8 */
extern fn_row_array ld_ihl_n;   /* LD (HL), imm8 */
extern fn_row_array ldh_ic_a;   /* LDH (C), A */
extern fn_row_array ldh_a_ic;   /* LDH A, (C) */
extern fn_row_array ldh_in_a;   /* LDH (imm8), A */
extern fn_row_array ldh_a_in;   /* LDH A, (imm8) */
extern fn_row_array ld_inn_a;   /* LD (imm16), A */
extern fn_row_array ld_a_inn;   /* LD A, (imm16) */
extern fn_row_array ld_rr_nn;   /* LD rr, imm16 */
extern fn_row_array ld_inn_sp;  /* LD (imm16), SP */
extern fn_row_array push;       /* PUSH */
extern fn_row_array pop;        /* POP */
extern fn_row_array ld_sp_hl;   /* LD SP, HL */
extern fn_row_array ld_hl_spe;  /* LD HL, SP+smm8 */

/* aritm.c */
extern fn_row_array add_r;      /* ADD r */
extern fn_row_array add_ihl;    /* ADD (HL) */
extern fn_row_array add_n;      /* ADD n */
extern fn_row_array adc_r;      /* ADC r */
extern fn_row_array adc_ihl;    /* ADC (HL) */
extern fn_row_array adc_n;      /* ADC n */
extern fn_row_array sub_r;      /* SUB r */
extern fn_row_array sub_ihl;    /* SUB (HL) */
extern fn_row_array sub_n;      /* SUB n */
extern fn_row_array sbc_r;      /* SBC r */
extern fn_row_array sbc_ihl;    /* SBC (HL) */
extern fn_row_array sbc_n;      /* SBC n */
extern fn_row_array cp_r;       /* CP r */
extern fn_row_array cp_ihl;     /* CP (HL) */
extern fn_row_array cp_n;       /* CP n */
extern fn_row_array inc_r;      /* INC r */
extern fn_row_array inc_ihl;    /* INC (HL) */
extern fn_row_array dec_r;      /* DEC r */
extern fn_row_array dec_ihl;    /* DEC (HL) */
extern fn_row_array and_r;      /* AND r */
extern fn_row_array and_ihl;    /* AND (HL) */
extern fn_row_array and_n;      /* AND n */
extern fn_row_array or_r;       /* OR r */
extern fn_row_array or_ihl;     /* OR (HL) */
extern fn_row_array or_n;       /* OR n */
extern fn_row_array xor_r;      /* XOR r */
extern fn_row_array xor_ihl;    /* XOR (HL) */
extern fn_row_array xor_n;      /* XOR n */
extern fn_row_array ccf;        /* CCF */
extern fn_row_array scf;        /* SCF */
extern fn_row_array daa;        /* DAA */
extern fn_row_array cpl;        /* CPL */
extern fn_row_array inc_rr;     /* INC rr */
extern fn_row_array dec_rr;     /* DEC rr */
extern fn_row_array add_hl_rr;  /* ADD HL, rr */
extern fn_row_array add_sp_e;   /* ADD SP, smm8 */

/* flow.c */
extern fn_row_array jp_nn;      /* JP nn */
extern fn_row_array jp_hl;      /* JP HL */
extern fn_row_array jp_cc_nn;   /* JP cc, nn */
extern fn_row_array jr_e;       /* JR e */
extern fn_row_array jr_cc_e;    /* JR cc, e */
extern fn_row_array call_nn;    /* CALL nn */
extern fn_row_array call_cc_nn; /* CALL cc, nn */
extern fn_row_array ret;        /* RET */
extern fn_row_array ret_cc;     /* RET cc */
extern fn_row_array reti;       /* RETI */
extern fn_row_array rst_n;      /* RST n */

/* misc.c */
extern fn_row_array nop;        /* NOP */
extern fn_row_array stop;       /* STOP */
extern fn_row_array halt;       /* HALT */
extern fn_row_array ei;         /* EI */
extern fn_row_array di;         /* DI */
extern fn_row_array prefix;     /* PREFIX CB */
extern fn_row_array not_impl;   /* not implemented */

/* bits.c */
extern fn_row_array rlca;       /* RLCA */
extern fn_row_array rrca;       /* RRCA */
extern fn_row_array rla;        /* RLA */
extern fn_row_array rra;        /* RRA */
extern fn_row_array rlc_r;      /* RLC r */
extern fn_row_array rlc_ihl;    /* RLC (HL) */
extern fn_row_array rrc_r;      /* RRC r */
extern fn_row_array rrc_ihl;    /* RRC (HL) */
extern fn_row_array rl_r;       /* RL r */
extern fn_row_array rl_ihl;     /* RL (HL) */
extern fn_row_array rr_r;       /* RR r */
extern fn_row_array rr_ihl;     /* RR (HL) */
extern fn_row_array sla_r;      /* SLA r */
extern fn_row_array sla_ihl;    /* SLA (HL) */
extern fn_row_array sra_r;      /* SRA r */
extern fn_row_array sra_ihl;    /* SRA (HL) */
extern fn_row_array swap_r;     /* SWAP r */
extern fn_row_array swap_ihl;   /* SWAP (HL) */
extern fn_row_array srl_r;      /* SRL r */
extern fn_row_array srl_ihl;    /* SRL (HL) */
extern fn_row_array bit_b_r;    /* BIT b, r */
extern fn_row_array bit_b_ihl;  /* BIT b, (HL) */
extern fn_row_array res_b_r;    /* RES b, r */
extern fn_row_array res_b_ihl;  /* RES b, (HL) */
extern fn_row_array set_b_r;    /* SET b, r */
extern fn_row_array set_b_ihl;  /* SET b, (HL) */

/* isr.c */
extern fn_row_array isr;        /* Interrupt Service Routine */

/* main instructions array */
extern fn_row_array_ptrs instructions;

/* CB instructions array */
extern fn_row_array_ptrs cb_instructions;

#endif /* __INSTRS_H */

// license:BSD-3-Clause
// copyright-holders:David Haywood
/* Philips MX10EXA, page-zero execution and on-chip RAM for sound firmware. */

#include "driver.h"
#include "debugger.h"
#include "xa.h"
#include <stdbool.h>

typedef UINT8 u8;
typedef UINT16 u16;
typedef UINT32 u32;
typedef INT8 s8;
typedef INT16 s16;
typedef INT32 s32;
#define u8(x) ((u8)(x))
#define u16(x) ((u16)(x))
#define u32(x) ((u32)(x))
#define s8(x) ((s8)(x))
#define s16(x) ((s16)(x))
#define s32(x) ((s32)(x))
#define LOGMASKED(...) do { } while (0)
#define get_addr gr16
#define set_addr sr16

typedef struct {
	u8 m_im;
	u8 m_rs;
	u8 m_zflag;
	u8 m_nflag;
	u8 m_vflag;
	u8 m_cflag;
	u8 m_acflag;
	u8 m_sm_flag;
	u8 m_tm_flag;
	u8 m_p_flag;
	u8 m_f0_flag;
	u8 m_f1_flag;
	u32 m_pc;
	bool m_usermode;
	u8 m_pagezeromode;
	u16 m_USP;
	u16 m_SSP;
	u8 m_WDCON;
	u8 m_SCR;
	u8 m_IEL;
	u8 m_PSWL;
	u8 m_PSWH;
	u8 m_regbank;
	u8 m_PxCFGA[4];
	u8 m_PxCFGB[4];
	u8 m_in_interrupt;
	u8 m_irq_pending;
	u16 m_regs[(4 * 4) + 4];
	u8 ram[0x800];
	const struct xa_config *config;
	int (*irq_callback)(int);
} xa_state;
#define get_directtext(x) "direct"
#define show_expanded_data4(x, size) "immediate"
static xa_state xa;
static int m_icount;
#define m_im xa.m_im
#define m_rs xa.m_rs
#define m_zflag xa.m_zflag
#define m_nflag xa.m_nflag
#define m_vflag xa.m_vflag
#define m_cflag xa.m_cflag
#define m_acflag xa.m_acflag
#define m_sm_flag xa.m_sm_flag
#define m_tm_flag xa.m_tm_flag
#define m_p_flag xa.m_p_flag
#define m_f0_flag xa.m_f0_flag
#define m_f1_flag xa.m_f1_flag
#define m_pc xa.m_pc
#define m_usermode xa.m_usermode
#define m_pagezeromode xa.m_pagezeromode
#define m_USP xa.m_USP
#define m_SSP xa.m_SSP
#define m_WDCON xa.m_WDCON
#define m_SCR xa.m_SCR
#define m_IEL xa.m_IEL
#define m_PSWL xa.m_PSWL
#define m_PSWH xa.m_PSWH
#define m_regbank xa.m_regbank
#define m_PxCFGA xa.m_PxCFGA
#define m_PxCFGB xa.m_PxCFGB
#define m_in_interrupt xa.m_in_interrupt
#define m_irq_pending xa.m_irq_pending
#define m_regs xa.m_regs

static u8 xa_program_read_byte(u32 addr) { return program_read_byte_16le(addr & 0xffff); }
static u16 xa_program_read_word(u32 addr) { return program_read_word_16le(addr & 0xffff); }
static u8 xa_data_read_byte(u32 addr) { return addr < sizeof(xa.ram) ? xa.ram[addr] : 0xff; }
static void xa_data_write_byte(u32 addr, u8 data) { if (addr < sizeof(xa.ram)) xa.ram[addr] = data; }
static u16 xa_data_read_word(u32 addr) { return xa_data_read_byte(addr) | (xa_data_read_byte(addr + 1) << 8); }
static void xa_data_write_word(u32 addr, u16 data) { xa_data_write_byte(addr, data); xa_data_write_byte(addr + 1, data >> 8); }
static u8 xa_sfr_read_byte(u32 addr);
static void xa_sfr_write_byte(u32 addr, u8 data);
static void cy(u8 cycles);
static u8 get_n_flag(void);
static void set_n_flag(void);
static void clear_n_flag(void);
static u8 get_z_flag(void);
static void set_z_flag(void);
static void clear_z_flag(void);
static u8 get_c_flag(void);
static void set_c_flag(void);
static void clear_c_flag(void);
static u8 get_v_flag(void);
static void set_v_flag(void);
static void clear_v_flag(void);
static u8 get_ac_flag(void);
static void set_ac_flag(void);
static void clear_ac_flag(void);
static u8 sfr_port_r(offs_t offset);
static void sfr_port_w(offs_t offset, u8 data);
static u8 sfr_PxCFGA_r(offs_t offset);
static void sfr_PxCFGA_w(offs_t offset, u8 data);
static u8 sfr_PxCFGB_r(offs_t offset);
static void sfr_PxCFGB_w(offs_t offset, u8 data);
static void set_pc_in_current_page(u16 addr);
static u8 sfr_WDCON_r(void);
static u8 sfr_PSWL_r(void);
static void sfr_PSWL_w(u8 data);
static u8 sfr_PSWH_r(void);
static void sfr_PSWH_w(u8 data);
static void sfr_PSW51_w(u8 data);
static u8 sfr_IEL_r(void);
static void sfr_IEL_w(u8 data);
static void sfr_SCR_w(u8 data);
static void sfr_WFEED1_w(u8 data);
static void sfr_WFEED2_w(u8 data);
static u16 expand_rel16(u16 rel16);
static u16 expand_rel8(u8 rel8);
static void do_nz_flags_16(u16 data);
static void do_nz_flags_8(u8 data);
static void push_word_to_user_stack(u16 data);
static void push_word_to_system_stack(u16 data);
static void push_word_to_stack(u16 data);
static u16 pull_word_from_user_stack(void);
static u16 pull_word_from_system_stack(void);
static u16 pull_word_from_stack(void);
static void push_byte_to_user_stack(u8 data);
static void push_byte_to_system_stack(u8 data);
static void push_byte_to_stack(u16 data);
static u8 pull_byte_from_user_stack(void);
static u8 pull_byte_from_system_stack(void);
static u8 pull_byte_from_stack(void);
static u8 gr8(int reg);
static void sr8(int reg, u8 data);
static void sr16(int reg, u16 data);
static u16 gr16(int reg);
static void wdat8(int address, u8 data);
static void wdat16(int address, u16 data);
static u8 rdat8(int address);
static u16 rdat16(int address);
static void write_direct16(u16 addr, u16 data);
static void write_direct8(u16 addr, u8 data);
static u16 read_direct16(u16 addr);
static u8 read_direct8(u16 addr);
static void e_illegal(u8 op);
static void handle_shift(u8 op, int shift_type);
static void handle_alu_type0(u8 op, int alu_op);
static void handle_alu_type1(u8 op, u8 op2);
static void handle_adds_movs(u8 op, int which);
static void push_byte_reglist(u8 op2, int h, bool force_user);
static void push_word_reglist(u8 op2, int h, bool force_user);
static void pull_byte_reglist(u8 op2, int h, bool force_user);
static void pull_word_reglist(u8 op2, int h, bool force_user);
static void handle_push_rlist(u8 op);
static void handle_pushu_rlist(u8 op);
static void handle_pop_rlist(u8 op);
static void handle_popu_rlist(u8 op);
static void e_nop(u8 op);
static void e_bitgroup(u8 op);
static void e_add(u8 op);
static void e_push_rlist(u8 op);
static void e_addc(u8 op);
static void e_pushu_rlist(u8 op);
static void e_sub(u8 op);
static void e_pop_rlist(u8 op);
static void e_subb(u8 op);
static void e_popu_rlist(u8 op);
static void e_lea_offset8(u8 op);
static void e_lea_offset16(u8 op);
static void e_cmp(u8 op);
static void e_xch_type1(u8 op);
static void e_and(u8 op);
static void e_xch_type2(u8 op);
static void e_or(u8 op);
static void e_xor(u8 op);
static void e_movc_rd_rsinc(u8 op);
static void e_mov(u8 op);
static void e_pushpop_djnz_subgroup(u8 op);
static void e_g9_subgroup(u8 op);
static void e_alu(u8 op);
static void e_jb_mov_subgroup(u8 op);
static void e_movdir(u8 op);
static void e_adds(u8 op);
static void e_movx_subgroup(u8 op);
static void e_rr(u8 op);
static void e_movs(u8 op);
static void e_rrc(u8 op);
static void e_lsr_fc(u8 op);
static void e_asl_c(u8 op);
static void e_asr_c(u8 op);
static void e_norm(u8 op);
static void e_lsr_fj(u8 op);
static void e_asl_j(u8 op);
static void e_asr_j(u8 op);
static void e_rl(u8 op);
static void e_rlc(u8 op);
static void e_djnz_cjne(u8 op);
static void e_mulu_b(u8 op);
static void e_divu_b(u8 op);
static void e_mulu_w(u8 op);
static void e_divu_w(u8 op);
static void e_mul_w(u8 op);
static void e_div_w(u8 op);
static void e_div_data8(u8 op);
static void e_div_d16(u8 op);
static void e_divu_d(u8 op);
static void e_div_d(u8 op);
static void e_cjne_d8(u8 op);
static void e_cjne_d16(u8 op);
static void e_jz_rel8(u8 op);
static void e_jnz_rel8(u8 op);
static void e_branch(u8 op);
static void e_bkpt(u8 op);
static void device_reset(void);
static void check_external_irq_level(int level);
static void check_interrupts(void);
static u32 gr32(int reg);
static void sr32(int reg, u32 data);
static u16 do_subb_16(u16 val1, u16 val2);
static u16 do_sub_16(u16 val1, u16 val2);
static u16 do_sub_16_helper(u16 val1, u16 val2, u8 c);
static u16 do_addc_16(u16 val1, u16 val2);
static u16 do_add_16(u16 val1, u16 val2);
static u16 do_add_16_helper(u16 val1, u16 val2, u8 c);
static u16 do_xor_16(u16 val1, u16 val2);
static u16 do_or_16(u16 val1, u16 val2);
static u16 do_and_16(u16 val1, u16 val2);
static u32 asl32_helper(u32 fullreg, u8 amount);
static u32 lsr32_helper(u32 fullreg, u8 amount);
static u16 lsr16_helper(u16 fullreg, u8 amount);
static u8 do_subb_8(u8 val1, u8 val2);
static u8 do_sub_8(u8 val1, u8 val2);
static u8 do_sub_8_helper(u8 val1, u8 val2, u8 c);
static u8 do_addc_8(u8 val1, u8 val2);
static u8 do_add_8(u8 val1, u8 val2);
static u8 do_add_8_helper(u8 val1, u8 val2, u8 c);
static u8 do_xor_8(u8 val1, u8 val2);
static u8 do_or_8(u8 val1, u8 val2);
static u8 do_and_8(u8 val1, u8 val2);
static u8 do_cjne_8_helper(u8 val1, u8 val2);
static void set_bit_8_helper(u16 bit, u8 val);
static void do_nop(void);
static void aluop_byte_rd_data8(int alu_op, u8 rd, u8 data8);
static void add_byte_rd_data8(u8 rd, u8 data8);
static void addc_byte_rd_data8(u8 rd, u8 data8);
static void sub_byte_rd_data8(u8 rd, u8 data8);
static void subb_byte_rd_data8(u8 rd, u8 data8);
static void cmp_byte_rd_data8(u8 rd, u8 data8);
static void and_byte_rd_data8(u8 rd, u8 data8);
static void or_byte_rd_data8(u8 rd, u8 data8);
static void xor_byte_rd_data8(u8 rd, u8 data8);
static void mov_byte_rd_data8(u8 rd, u8 data8);
static void aluop_byte_indrd_data8(int alu_op, u8 rd, u8 data8);
static void add_byte_indrd_data8(u8 rd, u8 data8);
static void addc_byte_indrd_data8(u8 rd, u8 data8);
static void sub_byte_indrd_data8(u8 rd, u8 data8);
static void subb_byte_indrd_data8(u8 rd, u8 data8);
static void cmp_byte_indrd_data8(u8 rd, u8 data8);
static void and_byte_indrd_data8(u8 rd, u8 data8);
static void or_byte_indrd_data8(u8 rd, u8 data8);
static void xor_byte_indrd_data8(u8 rd, u8 data8);
static void mov_byte_indrd_data8(u8 rd, u8 data8);
static void aluop_byte_indrdinc_data8(int alu_op, u8 rd, u8 data8);
static void add_byte_indrdinc_data8(u8 rd, u8 data8);
static void addc_byte_indrdinc_data8(u8 rd, u8 data8);
static void sub_byte_indrdinc_data8(u8 rd, u8 data8);
static void subb_byte_indrdinc_data8(u8 rd, u8 data8);
static void cmp_byte_indrdinc_data8(u8 rd, u8 data8);
static void and_byte_indrdinc_data8(u8 rd, u8 data8);
static void or_byte_indrdinc_data8(u8 rd, u8 data8);
static void xor_byte_indrdinc_data8(u8 rd, u8 data8);
static void mov_byte_indrdinc_data8(u8 rd, u8 data8);
static void aluop_byte_rdoff8_data8(int alu_op, u8 rd, u8 offset8, u8 data8);
static void add_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void addc_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void sub_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void subb_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void cmp_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void and_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void or_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void xor_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void mov_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8);
static void aluop_byte_rdoff16_data8(int alu_op, u8 rd, u16 offset16, u8 data8);
static void add_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void addc_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void sub_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void subb_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void cmp_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void and_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void or_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void xor_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void mov_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8);
static void aluop_byte_direct_data8(int alu_op, u16 direct, u8 data8);
static void add_byte_direct_data8(u16 direct, u8 data8);
static void addc_byte_direct_data8(u16 direct, u8 data8);
static void sub_byte_direct_data8(u16 direct, u8 data8);
static void subb_byte_direct_data8(u16 direct, u8 data8);
static void cmp_byte_direct_data8(u16 direct, u8 data8);
static void and_byte_direct_data8(u16 direct, u8 data8);
static void or_byte_direct_data8(u16 direct, u8 data8);
static void xor_byte_direct_data8(u16 direct, u8 data8);
static void mov_byte_direct_data8(u16 direct, u8 data8);
static void aluop_byte_rd_data16(int alu_op, u8 rd, u16 data16);
static void add_word_rd_data16(u8 rd, u16 data16);
static void addc_word_rd_data16(u8 rd, u16 data16);
static void sub_word_rd_data16(u8 rd, u16 data16);
static void subb_word_rd_data16(u8 rd, u16 data16);
static void cmp_word_rd_data16(u8 rd, u16 data16);
static void and_word_rd_data16(u8 rd, u16 data16);
static void or_word_rd_data16(u8 rd, u16 data16);
static void xor_word_rd_data16(u8 rd, u16 data16);
static void mov_word_rd_data16(u8 rd, u16 data16);
static void aluop_byte_indrd_data16(int alu_op, u8 rd, u16 data16);
static void add_word_indrd_data16(u8 rd, u16 data16);
static void addc_word_indrd_data16(u8 rd, u16 data16);
static void sub_word_indrd_data16(u8 rd, u16 data16);
static void subb_word_indrd_data16(u8 rd, u16 data16);
static void cmp_word_indrd_data16(u8 rd, u16 data16);
static void and_word_indrd_data16(u8 rd, u16 data16);
static void or_word_indrd_data16(u8 rd, u16 data16);
static void xor_word_indrd_data16(u8 rd, u16 data16);
static void mov_word_indrd_data16(u8 rd, u16 data16);
static void aluop_byte_indrdinc_data16(int alu_op, u8 rd, u16 data16);
static void add_word_indrdinc_data16(u8 rd, u16 data16);
static void addc_word_indrdinc_data16(u8 rd, u16 data16);
static void sub_word_indrdinc_data16(u8 rd, u16 data16);
static void subb_word_indrdinc_data16(u8 rd, u16 data16);
static void cmp_word_indrdinc_data16(u8 rd, u16 data16);
static void and_word_indrdinc_data16(u8 rd, u16 data16);
static void or_word_indrdinc_data16(u8 rd, u16 data16);
static void xor_word_indrdinc_data16(u8 rd, u16 data16);
static void mov_word_indrdinc_data16(u8 rd, u16 data16);
static void aluop_byte_rdoff8_data16(int alu_op, u8 rd, u8 offset8, u16 data16);
static void add_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void addc_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void sub_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void subb_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void cmp_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void and_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void or_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void xor_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void mov_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16);
static void aluop_byte_rdoff16_data16(int alu_op, u8 rd, u16 offset16, u16 data16);
static void add_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void addc_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void sub_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void subb_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void cmp_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void and_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void or_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void xor_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void mov_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16);
static void aluop_byte_direct_data16(int alu_op, u16 direct, u16 data16);
static void add_word_direct_data16(u16 direct, u16 data16);
static void addc_word_direct_data16(u16 direct, u16 data16);
static void sub_word_direct_data16(u16 direct, u16 data16);
static void subb_word_direct_data16(u16 direct, u16 data16);
static void cmp_word_direct_data16(u16 direct, u16 data16);
static void and_word_direct_data16(u16 direct, u16 data16);
static void or_word_direct_data16(u16 direct, u16 data16);
static void xor_word_direct_data16(u16 direct, u16 data16);
static void mov_word_direct_data16(u16 direct, u16 data16);
static void aluop_word_rd_rs(int alu_op, u8 rd, u8 rs);
static void aluop_byte_rd_rs(int alu_op, u8 rd, u8 rs);
static void add_word_rd_rs(u8 rd, u8 rs);
static void addc_word_rd_rs(u8 rd, u8 rs);
static void sub_word_rd_rs(u8 rd, u8 rs);
static void subb_word_rd_rs(u8 rd, u8 rs);
static void cmp_word_rd_rs(u8 rd, u8 rs);
static void and_word_rd_rs(u8 rd, u8 rs);
static void or_word_rd_rs(u8 rd, u8 rs);
static void xor_word_rd_rs(u8 rd, u8 rs);
static void mov_word_rd_rs(u8 rd, u8 rs);
static void add_byte_rd_rs(u8 rd, u8 rs);
static void addc_byte_rd_rs(u8 rd, u8 rs);
static void sub_byte_rd_rs(u8 rd, u8 rs);
static void subb_byte_rd_rs(u8 rd, u8 rs);
static void cmp_byte_rd_rs(u8 rd, u8 rs);
static void and_byte_rd_rs(u8 rd, u8 rs);
static void or_byte_rd_rs(u8 rd, u8 rs);
static void xor_byte_rd_rs(u8 rd, u8 rs);
static void mov_byte_rd_rs(u8 rd, u8 rs);
static void aluop_word_rd_indrs(int alu_op, u8 rd, u8 rs);
static void aluop_byte_rd_indrs(int alu_op, u8 rd, u8 rs);
static void add_word_rd_indrs(u8 rd, u8 rs);
static void addc_word_rd_indrs(u8 rd, u8 rs);
static void sub_word_rd_indrs(u8 rd, u8 rs);
static void subb_word_rd_indrs(u8 rd, u8 rs);
static void cmp_word_rd_indrs(u8 rd, u8 rs);
static void and_word_rd_indrs(u8 rd, u8 rs);
static void or_word_rd_indrs(u8 rd, u8 rs);
static void xor_word_rd_indrs(u8 rd, u8 rs);
static void mov_word_rd_indrs(u8 rd, u8 rs);
static void add_byte_rd_indrs(u8 rd, u8 rs);
static void addc_byte_rd_indrs(u8 rd, u8 rs);
static void sub_byte_rd_indrs(u8 rd, u8 rs);
static void subb_byte_rd_indrs(u8 rd, u8 rs);
static void cmp_byte_rd_indrs(u8 rd, u8 rs);
static void and_byte_rd_indrs(u8 rd, u8 rs);
static void or_byte_rd_indrs(u8 rd, u8 rs);
static void xor_byte_rd_indrs(u8 rd, u8 rs);
static void mov_byte_rd_indrs(u8 rd, u8 rs);
static void aluop_word_indrd_rs(int alu_op, u8 rd, u8 rs);
static void aluop_byte_indrd_rs(int alu_op, u8 rd, u8 rs);
static void add_word_indrd_rs(u8 rd, u8 rs);
static void addc_word_indrd_rs(u8 rd, u8 rs);
static void sub_word_indrd_rs(u8 rd, u8 rs);
static void subb_word_indrd_rs(u8 rd, u8 rs);
static void cmp_word_indrd_rs(u8 rd, u8 rs);
static void and_word_indrd_rs(u8 rd, u8 rs);
static void or_word_indrd_rs(u8 rd, u8 rs);
static void xor_word_indrd_rs(u8 rd, u8 rs);
static void mov_word_indrd_rs(u8 rd, u8 rs);
static void add_byte_indrd_rs(u8 rd, u8 rs);
static void addc_byte_indrd_rs(u8 rd, u8 rs);
static void sub_byte_indrd_rs(u8 rd, u8 rs);
static void subb_byte_indrd_rs(u8 rd, u8 rs);
static void cmp_byte_indrd_rs(u8 rd, u8 rs);
static void and_byte_indrd_rs(u8 rd, u8 rs);
static void or_byte_indrd_rs(u8 rd, u8 rs);
static void xor_byte_indrd_rs(u8 rd, u8 rs);
static void mov_byte_indrd_rs(u8 rd, u8 rs);
static void aluop_word_rd_indrsinc(int alu_op, u8 rd, u8 rs);
static void aluop_byte_rd_indrsinc(int alu_op, u8 rd, u8 rs);
static void add_word_rd_indrsinc(u8 rd, u8 rs);
static void addc_word_rd_indrsinc(u8 rd, u8 rs);
static void sub_word_rd_indrsinc(u8 rd, u8 rs);
static void subb_word_rd_indrsinc(u8 rd, u8 rs);
static void cmp_word_rd_indrsinc(u8 rd, u8 rs);
static void and_word_rd_indrsinc(u8 rd, u8 rs);
static void or_word_rd_indrsinc(u8 rd, u8 rs);
static void xor_word_rd_indrsinc(u8 rd, u8 rs);
static void mov_word_rd_indrsinc(u8 rd, u8 rs);
static void add_byte_rd_indrsinc(u8 rd, u8 rs);
static void addc_byte_rd_indrsinc(u8 rd, u8 rs);
static void sub_byte_rd_indrsinc(u8 rd, u8 rs);
static void subb_byte_rd_indrsinc(u8 rd, u8 rs);
static void cmp_byte_rd_indrsinc(u8 rd, u8 rs);
static void and_byte_rd_indrsinc(u8 rd, u8 rs);
static void or_byte_rd_indrsinc(u8 rd, u8 rs);
static void xor_byte_rd_indrsinc(u8 rd, u8 rs);
static void mov_byte_rd_indrsinc(u8 rd, u8 rs);
static void aluop_word_indrdinc_rs(int alu_op, u8 rd, u8 rs);
static void aluop_byte_indrdinc_rs(int alu_op, u8 rd, u8 rs);
static void add_word_indrdinc_rs(u8 rd, u8 rs);
static void addc_word_indrdinc_rs(u8 rd, u8 rs);
static void sub_word_indrdinc_rs(u8 rd, u8 rs);
static void subb_word_indrdinc_rs(u8 rd, u8 rs);
static void cmp_word_indrdinc_rs(u8 rd, u8 rs);
static void and_word_indrdinc_rs(u8 rd, u8 rs);
static void or_word_indrdinc_rs(u8 rd, u8 rs);
static void xor_word_indrdinc_rs(u8 rd, u8 rs);
static void mov_word_indrdinc_rs(u8 rd, u8 rs);
static void add_byte_indrdinc_rs(u8 rd, u8 rs);
static void addc_byte_indrdinc_rs(u8 rd, u8 rs);
static void sub_byte_indrdinc_rs(u8 rd, u8 rs);
static void subb_byte_indrdinc_rs(u8 rd, u8 rs);
static void cmp_byte_indrdinc_rs(u8 rd, u8 rs);
static void and_byte_indrdinc_rs(u8 rd, u8 rs);
static void or_byte_indrdinc_rs(u8 rd, u8 rs);
static void xor_byte_indrdinc_rs(u8 rd, u8 rs);
static void mov_byte_indrdinc_rs(u8 rd, u8 rs);
static void aluop_word_rd_rsoff8(int alu_op, u8 rd, u8 rs, u8 offset8);
static void aluop_byte_rd_rsoff8(int alu_op, u8 rd, u8 rs, u8 offset8);
static void add_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void addc_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void sub_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void subb_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void cmp_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void and_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void or_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void xor_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void mov_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void add_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void addc_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void sub_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void subb_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void cmp_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void and_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void or_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void xor_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void mov_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8);
static void aluop_word_rdoff8_rs(int alu_op, u8 rd, u8 offset8, u8 rs);
static void aluop_byte_rdoff8_rs(int alu_op, u8 rd, u8 offset8, u8 rs);
static void add_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void addc_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void sub_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void subb_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void cmp_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void and_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void or_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void xor_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void mov_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void add_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void addc_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void sub_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void subb_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void cmp_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void and_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void or_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void xor_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void mov_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs);
static void aluop_word_rsoff16(int alu_op, u8 rd, u8 rs, u16 offset16);
static void aluop_byte_rsoff16(int alu_op, u8 rd, u8 rs, u16 offset16);
static void add_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void addc_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void sub_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void subb_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void cmp_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void and_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void or_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void xor_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void mov_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void add_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void addc_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void sub_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void subb_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void cmp_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void and_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void or_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void xor_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void mov_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16);
static void aluop_word_rdoff16_rs(int alu_op, u8 rd, u16 offset16, u8 rs);
static void aluop_byte_rdoff16_rs(int alu_op, u8 rd, u16 offset16, u8 rs);
static void add_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void addc_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void sub_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void subb_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void cmp_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void and_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void or_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void xor_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void mov_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void add_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void addc_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void sub_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void subb_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void cmp_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void and_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void or_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void xor_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void mov_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs);
static void aluop_word_rd_direct(int alu_op, u8 rd, u16 direct);
static void aluop_byte_rd_direct(int alu_op, u8 rd, u16 direct);
static void add_word_rd_direct(u8 rd, u16 direct);
static void addc_word_rd_direct(u8 rd, u16 direct);
static void sub_word_rd_direct(u8 rd, u16 direct);
static void subb_word_rd_direct(u8 rd, u16 direct);
static void cmp_word_rd_direct(u8 rd, u16 direct);
static void and_word_rd_direct(u8 rd, u16 direct);
static void or_word_rd_direct(u8 rd, u16 direct);
static void xor_word_rd_direct(u8 rd, u16 direct);
static void mov_word_rd_direct(u8 rd, u16 direct);
static void add_byte_rd_direct(u8 rd, u16 direct);
static void addc_byte_rd_direct(u8 rd, u16 direct);
static void sub_byte_rd_direct(u8 rd, u16 direct);
static void subb_byte_rd_direct(u8 rd, u16 direct);
static void cmp_byte_rd_direct(u8 rd, u16 direct);
static void and_byte_rd_direct(u8 rd, u16 direct);
static void or_byte_rd_direct(u8 rd, u16 direct);
static void xor_byte_rd_direct(u8 rd, u16 direct);
static void mov_byte_rd_direct(u8 rd, u16 direct);
static void aluop_word_direct_rs(int alu_op, u16 direct, u8 rs);
static void aluop_byte_direct_rs(int alu_op, u16 direct, u8 rs);
static void add_word_direct_rs(u16 direct, u8 rs);
static void addc_word_direct_rs(u16 direct, u8 rs);
static void sub_word_direct_rs(u16 direct, u8 rs);
static void subb_word_direct_rs(u16 direct, u8 rs);
static void cmp_word_direct_rs(u16 direct, u8 rs);
static void and_word_direct_rs(u16 direct, u8 rs);
static void or_word_direct_rs(u16 direct, u8 rs);
static void xor_word_direct_rs(u16 direct, u8 rs);
static void mov_word_direct_rs(u16 direct, u8 rs);
static void add_byte_direct_rs(u16 direct, u8 rs);
static void addc_byte_direct_rs(u16 direct, u8 rs);
static void sub_byte_direct_rs(u16 direct, u8 rs);
static void subb_byte_direct_rs(u16 direct, u8 rs);
static void cmp_byte_direct_rs(u16 direct, u8 rs);
static void and_byte_direct_rs(u16 direct, u8 rs);
static void or_byte_direct_rs(u16 direct, u8 rs);
static void xor_byte_direct_rs(u16 direct, u8 rs);
static void mov_byte_direct_rs(u16 direct, u8 rs);
static void movs_word_rd_data4(u8 rd, u8 data4);
static void movs_byte_rd_data4(u8 rd, u8 data4);
static void adds_word_rd_data4(u8 rd, u8 data4);
static void adds_byte_rd_data4(u8 rd, u8 data4);
static void movs_word_indrd_data4(u8 rd, u8 data4);
static void movs_byte_indrd_data4(u8 rd, u8 data4);
static void adds_word_indrd_data4(u8 rd, u8 data4);
static void adds_byte_indrd_data4(u8 rd, u8 data4);
static void movs_word_indrdinc_data4(u8 rd, u8 data4);
static void movs_byte_indrdinc_data4(u8 rd, u8 data4);
static void adds_word_indrdinc_data4(u8 rd, u8 data4);
static void adds_byte_indrdinc_data4(u8 rd, u8 data4);
static void movs_word_indrdoff8_data4(u8 rd, u8 off8, u8 data4);
static void movs_byte_indrdoff8_data4(u8 rd, u8 off8, u8 data4);
static void adds_word_indrdoff8_data4(u8 rd, u8 off8, u8 data4);
static void adds_byte_indrdoff8_data4(u8 rd, u8 off8, u8 data4);
static void movs_word_indrdoff16_data4(u8 rd, u16 off16, u8 data4);
static void movs_byte_indrdoff16_data4(u8 rd, u16 off16, u8 data4);
static void adds_word_indrdoff16_data4(u8 rd, u16 off16, u8 data4);
static void adds_byte_indrdoff16_data4(u8 rd, u16 off16, u8 data4);
static void movs_word_direct_data4(u16 direct, u8 data4);
static void movs_byte_direct_data4(u16 direct, u8 data4);
static void adds_word_direct_data4(u16 direct, u8 data4);
static void adds_byte_direct_data4(u16 direct, u8 data4);
static void call_rel16(u16 rel16);
static void bcc_rel8(u8 rel8);
static void bcs_rel8(u8 rel8);
static void bne_rel8(u8 rel8);
static void beq_rel8(u8 rel8);
static void bnv_rel8(u8 rel8);
static void bov_rel8(u8 rel8);
static void bpl_rel8(u8 rel8);
static void bmi_rel8(u8 rel8);
static void bg_rel8(u8 rel8);
static void bl_rel8(u8 rel8);
static void bge_rel8(u8 rel8);
static void blt_rel8(u8 rel8);
static void bgt_rel8(u8 rel8);
static void ble_rel8(u8 rel8);
static void br_rel8(u8 rel8);
static void asl_byte_rd_imm4(u8 rd, u8 amount);
static void asl_word_rd_imm4(u8 rd, u8 amount);
static void asl_dword_rd_imm5(u8 rd, u8 amount);
static void asr_byte_rd_imm4(u8 rd, u8 amount);
static void asr_word_rd_imm4(u8 rd, u8 amount);
static void asr_dword_rd_imm5(u8 rd, u8 amount);
static void lsr_byte_rd_imm4(u8 rd, u8 amount);
static void lsr_word_rd_imm4(u8 rd, u8 amount);
static void lsr_dword_rd_imm5(u8 rd, u8 amount);
static void asl_byte_rd_rs(u8 rd, u8 rs);
static void asl_word_rd_rs(u8 rd, u8 rs);
static void asl_dword_rd_rs(u8 rd, u8 rs);
static void asr_byte_rd_rs(u8 rd, u8 rs);
static void asr_word_rd_rs(u8 rd, u8 rs);
static void asr_dword_rd_rs(u8 rd, u8 rs);
static void lsr_byte_rd_rs(u8 rd, u8 rs);
static void lsr_word_rd_rs(u8 rd, u8 rs);
static void lsr_dword_rd_rs(u8 rd, u8 rs);
static void norm_byte_rd_rs(u8 rd, u8 rs);
static void norm_word_rd_rs(u8 rd, u8 rs);
static void norm_dword_rd_rs(u8 rd, u8 rs);
static void mulu_byte_rd_rs(u8 rd, u8 rs);
static void divu_byte_rd_rs(u8 rd, u8 rs);
static void mulu_word_rd_rs(u8 rd, u8 rs);
static void divu_word_rd_rs(u8 rd, u8 rs);
static void mul_word_rd_rs(u8 rd, u8 rs);
static void div_word_rd_rs(u8 rd, u8 rs);
static void div_word_rd_data8(u8 rd, u8 data8);
static void divu_byte_rd_data8(u8 rd, u8 data8);
static void divu_word_rd_data8(u8 rd, u8 data8);
static void mulu_byte_rd_data8(u8 rd, u8 data8);
static void mulu_word_rd_data16(u8 rd, u16 data16);
static void divu_dword_rd_data16(u8 rd, u16 data16);
static void mul_word_rd_data16(u8 rd, u16 data16);
static void div_dword_rd_data16(u8 rd, u16 data16);
static void divu_dword_rd_rs(u8 rd, u8 rs);
static void div_dword_rd_rs(u8 rd, u8 rs);
static void clr_bit(u16 bit);
static void setb_bit(u16 bit);
static void mov_c_bit(u16 bit);
static void mov_bit_c(u16 bit);
static void anl_c_bit(u16 bit);
static void anl_c_notbit(u16 bit);
static void orl_c_bit(u16 bit);
static void orl_c_notbit(u16 bit);
static void lea_word_rd_rs_off8(u8 rd, u8 rs, u8 offs8);
static void lea_word_rd_rs_off16(u8 rd, u8 rs, u16 offs16);
static void xch_word_rd_indrs(u8 rd, u8 rs);
static void xch_byte_rd_indrs(u8 rd, u8 rs);
static void xch_word_rd_rs(u8 rd, u8 rs);
static void xch_byte_rd_rs(u8 rd, u8 rs);
static void movc_word_rd_indrsinc(u8 rd, u8 rs);
static void movc_byte_rd_indrsinc(u8 rd, u8 rs);
static void djnz_word_rd_rel8(u8 rd, u8 rel8);
static void djnz_byte_rd_rel8(u8 rd, u8 rel8);
static void popu_word_direct(u16 direct);
static void popu_byte_direct(u16 direct);
static void pop_word_direct(u16 direct);
static void pop_byte_direct(u16 direct);
static void pushu_word_direct(u16 direct);
static void pushu_byte_direct(u16 direct);
static void push_word_direct(u16 direct);
static void push_byte_direct(u16 direct);
static void mov_word_indrdinc_indrsinc(u8 rd, u8 rs);
static void mov_byte_indrdinc_indrsinc(u8 rd, u8 rs);
static void da_rd(u8 rd);
static void sext_word_rd(u8 rd);
static void sext_byte_rd(u8 rd);
static void cpl_word_rd(u8 rd);
static void cpl_byte_rd(u8 rd);
static void neg_word_rd(u8 rd);
static void neg_byte_rd(u8 rd);
static void movc_a_apc(void);
static void movc_a_adptr(void);
static void mov_rd_usp(u8 rd);
static void mov_usp_rs(u8 rs);
static void jb_bit_rel8(u16 bit, u8 rel8);
static void jnb_bit_rel8(u16 bit, u8 rel8);
static void jbc_bit_rel8(u16 bit, u8 rel8);
static void mov_word_direct_direct(u16 direct_d, u16 direct_s);
static void mov_byte_direct_direct(u16 direct_d, u16 direct_s);
static void xch_word_rd_direct(u8 rd, u16 direct);
static void xch_byte_rd_direct(u8 rd, u16 direct);
static void mov_word_direct_indrs(u16 direct, u8 rs);
static void mov_byte_direct_indrs(u16 direct, u8 rs);
static void mov_word_indrd_direct(u8 rd, u16 direct);
static void mov_byte_indrd_direct(u8 rd, u16 direct);
static void movx_word_indrd_rs(u8 rd, u8 rs);
static void movx_byte_indrd_rs(u8 rd, u8 rs);
static void movx_word_rd_indrs(u8 rd, u8 rs);
static void movx_byte_rd_indrs(u8 rd, u8 rs);
static void rr_word_rd_data4(u8 rd, u8 data4);
static void rr_byte_rd_data4(u8 rd, u8 data4);
static void rrc_word_rd_data4(u8 rd, u8 data4);
static void rrc_byte_rd_data4(u8 rd, u8 data4);
static void rl_word_rd_data4(u8 rd, u8 data4);
static void rl_byte_rd_data4(u8 rd, u8 data4);
static void rlc_word_rd_data4(u8 rd, u8 data4);
static void rlc_byte_rd_data4(u8 rd, u8 data4);
static void fcall_addr24(u32 addr24);
static void call_indrs(u8 rs);
static void fjmp_addr24(u32 addr24);
static void jmp_rel16(u16 rel16);
static void djnz_word_direct_rel8(u16 direct, u8 rel8);
static void djnz_byte_direct_rel8(u16 direct, u8 rel8);
static void cjne_word_rd_direct_rel8(u8 rd, u16 direct, u8 rel8);
static void cjne_byte_rd_direct_rel8(u8 rd, u16 direct, u8 rel8);
static void cjne_indrd_data8_rel8(u8 rd, u8 data8, u8 rel8);
static void cjne_rd_data8_rel8(u8 rd, u8 data8, u8 rel8);
static void cjne_indrd_data16_rel8(u8 rd, u16 data16, u8 rel8);
static void cjne_rd_data16_rel8(u8 rd, u16 data16, u8 rel8);
static void reset(void);
static void trap_data4(u8 data4);
static void jmp_ind_adptr(void);
static void jmp_dblindrs(u8 rs);
static void jmp_indrs(u8 rs);
static void ret(void);
static void reti(void);
static void jz_rel8(u8 rel8);
static void jnz_rel8(u8 rel8);
static void push_word_rlist(u8 bitfield, int h);
static void push_byte_rlist(u8 bitfield, int h);
static void pushu_word_rlist(u8 bitfield, int h);
static void pushu_byte_rlist(u8 bitfield, int h);
static void pop_word_rlist(u8 bitfield, int h);
static void pop_byte_rlist(u8 bitfield, int h);
static void popu_word_rlist(u8 bitfield, int h);
static void popu_byte_rlist(u8 bitfield, int h);

static void (*const s_instruction[256])(u8) =
{

e_nop,          e_add,      e_add,      e_add,     e_add,     e_add,     e_add,      e_push_rlist,
e_bitgroup,     e_add,      e_add,      e_add,     e_add,     e_add,     e_add,      e_push_rlist,

e_illegal,      e_addc,     e_addc,     e_addc,    e_addc,    e_addc,    e_addc,     e_pushu_rlist,
e_illegal,      e_addc,     e_addc,     e_addc,    e_addc,    e_addc,    e_addc,     e_pushu_rlist,

e_illegal,      e_sub,      e_sub,      e_sub,     e_sub,     e_sub,     e_sub,      e_pop_rlist,
e_illegal,      e_sub,      e_sub,      e_sub,     e_sub,     e_sub,     e_sub,      e_pop_rlist,

e_illegal,      e_subb,     e_subb,     e_subb,    e_subb,    e_subb,    e_subb,     e_popu_rlist,
e_illegal,      e_subb,     e_subb,     e_subb,    e_subb,    e_subb,    e_subb,     e_popu_rlist,

e_lea_offset8,  e_cmp,      e_cmp,      e_cmp,     e_cmp,     e_cmp,     e_cmp,      e_push_rlist,
e_lea_offset16, e_cmp,      e_cmp,      e_cmp,     e_cmp,     e_cmp,     e_cmp,      e_push_rlist,

e_xch_type1,    e_and,      e_and,      e_and,     e_and,     e_and,     e_and,      e_pushu_rlist,
e_xch_type1,    e_and,      e_and,      e_and,     e_and,     e_and,     e_and,      e_pushu_rlist,

e_xch_type2,    e_or,       e_or,       e_or,      e_or,      e_or,      e_or,       e_pop_rlist,
e_xch_type2,    e_or,       e_or,       e_or,      e_or,      e_or,      e_or,       e_pop_rlist,

e_illegal,      e_xor,      e_xor,      e_xor,     e_xor,     e_xor,     e_xor,      e_popu_rlist,
e_illegal,      e_xor,      e_xor,      e_xor,     e_xor,     e_xor,     e_xor,      e_popu_rlist,

e_movc_rd_rsinc,e_mov,      e_mov,      e_mov,     e_mov,     e_mov,     e_mov,      e_pushpop_djnz_subgroup,
e_movc_rd_rsinc,e_mov,      e_mov,      e_mov,     e_mov,     e_mov,     e_mov,      e_pushpop_djnz_subgroup,

e_g9_subgroup,  e_alu,      e_alu,      e_alu,     e_alu,     e_alu,     e_alu,      e_jb_mov_subgroup,
e_g9_subgroup,  e_alu,      e_alu,      e_alu,     e_alu,     e_alu,     e_alu,      e_jb_mov_subgroup,

e_movdir,       e_adds,     e_adds,     e_adds,    e_adds,    e_adds,    e_adds,     e_movx_subgroup,
e_movdir,       e_adds,     e_adds,     e_adds,    e_adds,    e_adds,    e_adds,     e_movx_subgroup,

e_rr,           e_movs,     e_movs,     e_movs,    e_movs,    e_movs,    e_movs,     e_rrc,
e_rr,           e_movs,     e_movs,     e_movs,    e_movs,    e_movs,    e_movs,     e_rrc,

e_lsr_fc,       e_asl_c,    e_asr_c,    e_norm,    e_lsr_fc,  e_asl_c,   e_asr_c,    e_norm,
e_lsr_fc,       e_asl_c,    e_asr_c,    e_norm,    e_lsr_fc,  e_asl_c,   e_asr_c,    e_norm,

e_lsr_fj,       e_asl_j,   e_asr_j,     e_rl,       e_lsr_fj,  e_asl_j,  e_asr_j,    e_rlc,
e_lsr_fj,       e_asl_j,   e_asr_j,     e_rl,       e_lsr_fj,  e_asl_j,  e_asr_j,    e_rlc,

e_mulu_b,       e_divu_b,  e_djnz_cjne, e_cjne_d8,  e_mulu_w,  e_divu_w, e_mul_w,    e_div_w,
e_div_data8,    e_div_d16, e_djnz_cjne, e_cjne_d16, e_jz_rel8, e_divu_d, e_jnz_rel8, e_div_d,

e_branch,       e_branch,  e_branch,    e_branch,   e_branch,  e_branch, e_branch,   e_branch,
e_branch,       e_branch,  e_branch,    e_branch,   e_branch,  e_branch, e_branch,   e_bkpt,
};

static void cy(u8 cycles)
{
		m_icount -= cycles;
	}

static u8 get_n_flag(void)
{ return m_nflag; }

static void set_n_flag(void)
{ m_nflag = 1; }

static void clear_n_flag(void)
{ m_nflag = 0; }

static u8 get_z_flag(void)
{ return m_zflag; }

static void set_z_flag(void)
{ m_zflag = 1; }

static void clear_z_flag(void)
{ m_zflag = 0; }

static u8 get_c_flag(void)
{ return m_cflag; }

static void set_c_flag(void)
{ m_cflag = 1; }

static void clear_c_flag(void)
{ m_cflag = 0; }

static u8 get_v_flag(void)
{ return m_vflag; }

static void set_v_flag(void)
{ m_vflag = 1; }

static void clear_v_flag(void)
{ m_vflag = 0; }

static u8 get_ac_flag(void)
{ return m_acflag; }

static void set_ac_flag(void)
{ m_acflag = 1; }

static void clear_ac_flag(void)
{ m_acflag = 0; }

static u8 sfr_port_r(offs_t offset)
{
	return xa.config->port_r(offset);
}

static void sfr_port_w(offs_t offset, u8 data)
{
	xa.config->port_w(offset, data);
}

static u8 sfr_PxCFGA_r(offs_t offset)
{
	return m_PxCFGA[offset];
}

static void sfr_PxCFGA_w(offs_t offset, u8 data)
{
	LOGMASKED(LOG_DEBUG, "P%d CFGA - Port Configuration A write %02x\n", offset, data);
	m_PxCFGA[offset] = data;
}

static u8 sfr_PxCFGB_r(offs_t offset)
{
	return m_PxCFGB[offset];
}

static void sfr_PxCFGB_w(offs_t offset, u8 data)
{
	LOGMASKED(LOG_DEBUG, "P%d CFGB - Port Configuration B write %02x\n", offset, data);
	m_PxCFGB[offset] = data;
}

static void set_pc_in_current_page(u16 addr)
{
	m_pc = addr;
}

static u8 sfr_WDCON_r(void)
{
	return m_WDCON;
}

static u8 sfr_PSWL_r(void)
{
	u8 ret = m_PSWL & 0x38;
	if (get_z_flag()) ret |= 0x01;
	if (get_n_flag()) ret |= 0x02;
	if (get_v_flag()) ret |= 0x04;
	if (get_ac_flag()) ret |= 0x40;
	if (get_c_flag()) ret |= 0x80;
	LOGMASKED(LOG_DEBUG, "read %02x from PSWL\n", ret);
	return ret;
}

static void sfr_PSWL_w(u8 data)
{
	LOGMASKED(LOG_DEBUG, "write %02x to PSWL\n", data);
	m_PSWL = data;
	if (data & 0x01) set_z_flag(); else clear_z_flag();
	if (data & 0x02) set_n_flag(); else clear_n_flag();
	if (data & 0x04) set_v_flag(); else clear_v_flag();
	if (data & 0x40) set_ac_flag(); else clear_ac_flag();
	if (data & 0x80) set_c_flag(); else clear_c_flag();
}

static u8 sfr_PSWH_r(void)
{
	u8 ret = m_PSWH;
	LOGMASKED(LOG_DEBUG, "read %02x from PSWH\n", ret);
	return m_PSWH;
}

static void sfr_PSWH_w(u8 data)
{

	LOGMASKED(LOG_DEBUG, "write %02x to PSWH\n", data);
	m_PSWH = data;

	if (m_PSWH & 0x80)
		m_usermode = false;
	else
		m_usermode = true;

	m_regbank = (m_PSWH & 0x30) >> 4;

}

static void sfr_PSW51_w(u8 data)
{

}

static u8 sfr_IEL_r(void)
{
	return m_IEL;
}

static void sfr_IEL_w(u8 data)
{
	m_IEL = data;
	LOGMASKED(LOG_DEBUG, "write to m_IEL %02x\n", data);
}

static void sfr_SCR_w(u8 data)
{

	LOGMASKED(LOG_DEBUG, "write %02x to SCR\n", data);
	m_SCR = data;
	m_pagezeromode = (data & 1);
}

static void sfr_WFEED1_w(u8 data)
{
	LOGMASKED(LOG_DEBUG, "write %02x to WFEED1\n", data);
}

static void sfr_WFEED2_w(u8 data)
{
	LOGMASKED(LOG_DEBUG, "write %02x to WFEED2\n", data);
}

static u16 expand_rel16(u16 rel16)
{
	int address = m_pc + ((s16)rel16) * 2;
	address &= ~1;
	return address;
}

static u16 expand_rel8(u8 rel8)
{
	int address = m_pc + ((s8)rel8) * 2;
	address &= ~1;
	return address;
}

static void do_nz_flags_16(u16 data)
{
	if (data & 0x8000)
		set_n_flag();
	else
		clear_n_flag();

	if (data == 0x0000)
		set_z_flag();
	else
		clear_z_flag();
}

static void do_nz_flags_8(u8 data)
{
	if (data & 0x80)
		set_n_flag();
	else
		clear_n_flag();

	if (data == 0x00)
		set_z_flag();
	else
		clear_z_flag();
}

static void push_word_to_user_stack(u16 data)
{

	m_USP -= 2;
	xa_data_write_word(m_USP, data);
}

static void push_word_to_system_stack(u16 data)
{
	m_SSP -= 2;
	xa_data_write_word(m_SSP, data);
}

static void push_word_to_stack(u16 data)
{
	if (m_usermode)
	{
		push_word_to_user_stack(data);
	}
	else
	{
		push_word_to_system_stack(data);
	}
}

static u16 pull_word_from_user_stack(void)
{

	u16 data = xa_data_read_word(m_USP);
	m_USP += 2;
	return data;
}

static u16 pull_word_from_system_stack(void)
{
	u16 data = xa_data_read_word(m_SSP);
	m_SSP += 2;
	return data;
}

static u16 pull_word_from_stack(void)
{
	if (m_usermode)
	{
		return pull_word_from_user_stack();
	}
	else
	{
		return pull_word_from_system_stack();
	}
}

static void push_byte_to_user_stack(u8 data)
{

	m_USP -= 2;
	xa_data_write_byte(m_USP, data);
}

static void push_byte_to_system_stack(u8 data)
{
	m_SSP -= 2;
	xa_data_write_byte(m_SSP, data);
}

static void push_byte_to_stack(u16 data)
{
	if (m_usermode)
	{
		push_byte_to_user_stack(data);
	}
	else
	{
		push_byte_to_system_stack(data);
	}
}

static u8 pull_byte_from_user_stack(void)
{

	u8 data = xa_data_read_byte(m_USP);
	m_USP += 2;
	return data;
}

static u8 pull_byte_from_system_stack(void)
{
	u8 data = xa_data_read_byte(m_SSP);
	m_SSP += 2;
	return data;
}

static u8 pull_byte_from_stack(void)
{
	if (m_usermode)
	{
		return pull_byte_from_user_stack();
	}
	else
	{
		return pull_byte_from_system_stack();
	}
}

static u8 gr8(int reg)
{
	int high = reg & 1;

	reg >>= 1;

	if (reg < 4)
	{

		int regbank = m_regbank;

		if (high)
			return (m_regs[(regbank * 4) + reg] & 0xff00) >> 8;
		else
			return (m_regs[(regbank * 4) + reg] & 0x00ff) >> 0;
	}
	else
	{
		if (reg == 7)
		{
			fatalerror("Unsupported XA operation at %04X", m_pc);
			return 0;
		}

		reg -= 4;
		if (high)
			return (m_regs[(4 * 4) + reg] & 0xff00) >> 8;
		else
			return (m_regs[(4 * 4) + reg] & 0x00ff) >> 0;
	}
}

static void sr8(int reg, u8 data)
{
	int high = reg & 1;

	reg >>= 1;

	if (reg < 4)
	{

		int regbank = m_regbank;

		if (high)
			m_regs[(regbank * 4) + reg] = (m_regs[(regbank * 4) + reg] & 0x00ff) | (data << 8);
		else
			m_regs[(regbank * 4) + reg] = (m_regs[(regbank * 4) + reg] & 0xff00) | (data << 0);
	}
	else
	{
		if (reg == 7)
		{
			fatalerror("Unsupported XA operation at %04X", m_pc);
		}

		reg -= 4;
		if (high)
			m_regs[(4 * 4) + reg] = (m_regs[(4 * 4) + reg] & 0x00ff) | (data << 8);
		else
			m_regs[(4 * 4) + reg] = (m_regs[(4 * 4) + reg] & 0xff00) | (data << 0);
	}
}

static void sr16(int reg, u16 data)
{
	if (reg < 4)
	{

		int regbank = m_regbank;
		m_regs[(regbank * 4) + reg] = data;

	}
	else if (reg < 8)
	{
		if (reg == 7)
		{

			if (m_usermode)
				m_USP = data;
			else
				m_SSP = data;

			return;
		}

		reg -= 4;
		m_regs[(4 * 4) + reg] = data;
	}
	else
	{
		fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static u16 gr16(int reg)
{
	if (reg < 4)
	{

		int regbank = m_regbank;
		return m_regs[(regbank * 4) + reg];

	}
	else if (reg < 8)
	{
		if (reg == 7)
		{

			if (m_usermode)
				return m_USP;
			else
				return m_SSP;
		}

		reg -= 4;
		return m_regs[(4 * 4) + reg];
	}
	else
	{
		fatalerror("Unsupported XA operation at %04X", m_pc);
		return 0;
	}
}

static void wdat8(int address, u8 data)
{
	xa_data_write_byte(address, data);
}

static void wdat16(int address, u16 data)
{
	if (address & 1)
		fatalerror("Unsupported XA operation at %04X", m_pc);

	xa_data_write_word(address, data);
}

static u8 rdat8(int address)
{
	return xa_data_read_byte(address);
}

static u16 rdat16(int address)
{
	if (address & 1)
		fatalerror("Unsupported XA operation at %04X", m_pc);

	return xa_data_read_word(address);
}

static void write_direct16(u16 addr, u16 data)
{
	if (addr < 0x400)
	{
		if (addr & 1)
			fatalerror("Unsupported XA operation at %04X", m_pc);

		xa_data_write_word(addr, data);
	}
	else
	{
		fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static void write_direct8(u16 addr, u8 data)
{
	if (addr < 0x400)
	{
		xa_data_write_byte(addr, data);
	}
	else
	{
		xa_sfr_write_byte(addr - 0x400, data);
	}
}

static u16 read_direct16(u16 addr)
{
	if (addr < 0x400)
	{
		return xa_data_read_word(addr);
	}
	else
	{
		fatalerror("Unsupported XA operation at %04X", m_pc);
		return 0;
	}
	return 0;
}

static u8 read_direct8(u16 addr)
{
	if (addr < 0x400)
	{
		return xa_data_read_byte(addr);
	}
	else
	{
		return xa_sfr_read_byte(addr - 0x400);
	}
	return 0;
}

static void e_illegal(u8 op)
{
	fatalerror("Unsupported XA operation at %04X", m_pc);
}

static void handle_shift(u8 op, int shift_type)
{
	int size = (op & 0x0c) >> 2;
	const u8 op2 = xa_program_read_byte(m_pc++);
	u8 data, rd;
	if (size == 0x03)
	{
		data = op2 & 0x1f;
		rd = (op2 & 0xe0) >> 4;
	}
	else
	{
		data = op2 & 0x0f;
		rd = (op2 & 0xf0) >> 4;
	}

	switch (shift_type)
	{
	case 0x0:
	{
		if (size == 0) asl_byte_rd_imm4(rd, data);
		else if (size == 2) asl_word_rd_imm4(rd, data);
		else if (size == 3) asl_dword_rd_imm5(rd, data);
		break;
	}
	case 0x1:
	{
		if (size == 0) asr_byte_rd_imm4(rd, data);
		else if (size == 2) asr_word_rd_imm4(rd, data);
		else if (size == 3) asr_dword_rd_imm5(rd, data);
		break;
	}
	case 0x2:
	{
		if (size == 0) lsr_byte_rd_imm4(rd, data);
		else if (size == 2) lsr_word_rd_imm4(rd, data);
		else if (size == 3) lsr_dword_rd_imm5(rd, data);
		break;
	}
	}
}

static void handle_alu_type0(u8 op, int alu_op)
{
	const int size = op & 0x08;
	const u8 op2 = xa_program_read_byte(m_pc++);

	switch (op & 0x07)
	{
	case 0x01:
	{
		const u8 rs = (op2 & 0x0f);
		const u8 rd = (op2 & 0xf0) >> 4;
		if (size) { aluop_word_rd_rs(alu_op, rd, rs); } else { aluop_byte_rd_rs(alu_op, rd, rs); }
		break;
	}

	case 0x02:
	{
		const int optype = op2 & 0x08;
		if (!optype)
		{
			const u8 rs = (op2 & 0x07);
			const u8 rd = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_rd_indrs(alu_op, rd, rs); } else { aluop_byte_rd_indrs(alu_op, rd, rs); }
		}
		else
		{
			const u8 rd = (op2 & 0x07);
			const u8 rs = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_indrd_rs(alu_op, rd, rs); } else { aluop_byte_indrd_rs(alu_op, rd, rs); }
		}
		break;
	}

	case 0x03:
	{
		const int optype = op2 & 0x08;
		if (!optype)
		{
			const u8 rs = (op2 & 0x07);
			const u8 rd = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_rd_indrsinc(alu_op, rd, rs); } else { aluop_byte_rd_indrsinc(alu_op, rd, rs); }
		}
		else
		{
			const u8 rd = (op2 & 0x07);
			const u8 rs = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_indrdinc_rs(alu_op, rd, rs); } else { aluop_byte_indrdinc_rs(alu_op, rd, rs); }
		}
		break;
	}

	case 0x04:
	{
		const int optype = op2 & 0x08;
		const u8 offset8 = xa_program_read_byte(m_pc++);
		if (!optype)
		{
			const u8 rs = (op2 & 0x07);
			const u8 rd = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_rd_rsoff8(alu_op, rd, rs, offset8); } else { aluop_byte_rd_rsoff8(alu_op, rd, rs, offset8); }
		}
		else
		{
			const u8 rd = (op2 & 0x07);
			const u8 rs = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_rdoff8_rs(alu_op, rd, offset8, rs); } else { aluop_byte_rdoff8_rs(alu_op, rd, offset8, rs); }
		}
		break;
	}

	case 0x05:
	{
		const int optype = op2 & 0x08;
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const int offset16 = (op3 << 8) | op4;
		if (!optype)
		{
			const u8 rs = (op2 & 0x07);
			const u8 rd = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_rsoff16(alu_op, rd, rs, offset16); } else { aluop_byte_rsoff16(alu_op, rd, rs, offset16); }
		}
		else
		{
			const u8 rd = (op2 & 0x07);
			const u8 rs = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_rdoff16_rs(alu_op, rd, offset16, rs); } else { aluop_byte_rdoff16_rs(alu_op, rd, offset16, rs); }
		}
		break;
	}

	case 0x06:
	{
		const int optype = op2 & 0x08;
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u16 direct = ((op2 & 0x07) << 8) | op3;
		if (!optype)
		{
			const u8 rd = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_rd_direct(alu_op, rd, direct); } else { aluop_byte_rd_direct(alu_op, rd, direct); }
		}
		else
		{
			const u8 rs = (op2 & 0xf0) >> 4;
			if (size) { aluop_word_direct_rs(alu_op, direct, rs); } else { aluop_byte_direct_rs(alu_op, direct, rs); }
		}
		break;
	}

	}
}

static void handle_alu_type1(u8 op, u8 op2)
{
	int alu_op = op2 & 0x0f;
	switch (op & 0x0f)
	{
	case 0x01:
	{
		const u8 data8 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		aluop_byte_rd_data8(alu_op, rd, data8);
		break;
	}

	case 0x02:
	{
		const u8 data8 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		aluop_byte_indrd_data8(alu_op, rd, data8);
		break;
	}

	case 0x03:
	{
		const u8 data8 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		aluop_byte_indrdinc_data8(alu_op, rd, data8);
		break;
	}

	case 0x04:
	{
		const u8 offset8 = xa_program_read_byte(m_pc++);
		const u8 data8 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		aluop_byte_rdoff8_data8(alu_op, rd, offset8, data8);
		break;
	}

	case 0x05:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u8 data8 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const u16 offset16 = (op3 << 8) | op4;
		aluop_byte_rdoff16_data8(alu_op, rd, offset16, data8);
		break;
	}

	case 0x06:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 data8 = xa_program_read_byte(m_pc++);
		const u16 direct = ((op2 & 0xf0) << 4) | op3;
		aluop_byte_direct_data8(alu_op, direct, data8);
		break;
	}

	case 0x09:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const u16 data16 = (op3 << 8) | op4;
		aluop_byte_rd_data16(alu_op, rd, data16);
		break;
	}

	case 0x0a:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const u16 data16 = (op3 << 8) | op4;
		aluop_byte_indrd_data16(alu_op, rd, data16);
		break;
	}

	case 0x0b:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const u16 data16 = (op3 << 8) | op4;
		aluop_byte_indrdinc_data16(alu_op, rd, data16);
		break;
	}

	case 0x0c:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u8 op5 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const int offset8 = op3;
		const u16 data16 = (op4 << 8) | op5;
		aluop_byte_rdoff8_data16(alu_op, rd, offset8, data16);
		break;
	}

	case 0x0d:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u8 op5 = xa_program_read_byte(m_pc++);
		const u8 op6 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const int offset16= (op3 << 8) | op4;
		const u16 data16 = (op5 << 8) | op6;
		aluop_byte_rdoff16_data16(alu_op, rd, offset16, data16);
		break;
	}

	case 0x0e:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u8 op5 = xa_program_read_byte(m_pc++);
		const u16 direct =( (op2 & 0xf0) << 4) | op3;
		const u16 data16 = (op4 << 8) | op5;
		aluop_byte_direct_data16(alu_op, direct, data16);
		break;
	}
	}
}

static void handle_adds_movs(u8 op, int which)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	const u16 data4 = op2 & 0x0f;

	switch (op & 0x07)
	{
	case 0x01:
	{
		int rd = (op2 & 0xf0) >> 4;
		if (which) { if (size) { movs_word_rd_data4(rd, data4); } else { movs_byte_rd_data4(rd, data4); } }
		else       { if (size) { adds_word_rd_data4(rd, data4); } else { adds_byte_rd_data4(rd, data4); } }
		break;
	}
	case 0x02:
	{
		int rd = (op2 & 0x70) >> 4;
		if (which == 1) { if (size) { movs_word_indrd_data4(rd, data4); } else { movs_byte_indrd_data4(rd, data4); } }
		else {            if (size) { adds_word_indrd_data4(rd, data4); } else { adds_byte_indrd_data4(rd, data4); } }
		break;
	}
	case 0x03:
	{
		int rd = (op2 & 0x70) >> 4;
		if (which) { if (size) { movs_word_indrdinc_data4(rd, data4); } else { movs_byte_indrdinc_data4(rd, data4); } }
		else       { if (size) { adds_word_indrdinc_data4(rd, data4); } else { adds_byte_indrdinc_data4(rd, data4); } }
		break;
	}
	case 0x04:
	{
		int rd = (op2 & 0x70) >> 4;
		const u8 off8 = xa_program_read_byte(m_pc++);
		if (which == 1) { if (size) { movs_word_indrdoff8_data4(rd, off8, data4); } else { movs_byte_indrdoff8_data4(rd, off8, data4); } }
		else            { if (size) { adds_word_indrdoff8_data4(rd, off8, data4); } else { adds_byte_indrdoff8_data4(rd, off8, data4); } }
		break;
	}
	case 0x05:
	{
		int rd = (op2 & 0x70) >> 4;
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const int off16 = (op3 << 8) | op4;
		if (which == 1) { if (size) { movs_word_indrdoff16_data4(rd, off16, data4); } else { movs_byte_indrdoff16_data4(rd, off16, data4); } }
		else            { if (size) { adds_word_indrdoff16_data4(rd, off16, data4); } else { adds_byte_indrdoff16_data4(rd, off16, data4); } }
		break;
	}
	case 0x06:
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u16 direct = ((op2 & 0xf0) << 4) | op3;
		if (which == 1) { if (size) { movs_word_direct_data4(direct, data4); } else { movs_byte_direct_data4(direct, data4); } }
		else            { if (size) { adds_word_direct_data4(direct, data4); } else { adds_byte_direct_data4(direct, data4); } }
		break;
	}
	}
}

static void push_byte_reglist(u8 op2, int h, bool force_user)
{
	for (int i = 7; i >= 0; i--)
	{
		int bit = (op2 & (1 << i));
		if (bit)
		{
			u8 regval = gr8(i + (h ? 8 : 0));
			if (force_user)
				push_byte_to_user_stack(regval);
			else
				push_byte_to_stack(regval);
			cy(2);
		}
	}
}

static void push_word_reglist(u8 op2, int h, bool force_user)
{

	for (int i = 7; i >= 0; i--)
	{
		int bit = (op2 & (1 << i));
		if (bit)
		{
			u16 regval = gr16(i);
			if (force_user)
				push_word_to_user_stack(regval);
			else
				push_word_to_stack(regval);
			cy(2);
		}
	}
}

static void pull_byte_reglist(u8 op2, int h, bool force_user)
{
	for (int i = 0; i < 8; i++)
	{
		int bit = (op2 & (1 << i));
		if (bit)
		{
			u8 stackval;
			if (force_user)
				stackval = pull_byte_from_user_stack();
			else
				stackval = pull_byte_from_stack();

			sr8(i + (h ? 8 : 0), stackval);
			cy(2);
		}
	}
}

static void pull_word_reglist(u8 op2, int h, bool force_user)
{

	for (int i = 0; i < 8; i++)
	{
		int bit = (op2 & (1 << i));
		if (bit)
		{
			u16 stackval;
			if (force_user)
				stackval = pull_word_from_user_stack();
			else
				stackval = pull_word_from_stack();

			sr16(i, stackval);
			cy(2);
		}
	}
}

static void handle_push_rlist(u8 op)
{
	const u8 h = op & 0x40;
	const u8 size = op & 0x08;
	const u8 op2 = xa_program_read_byte(m_pc++);
	if (size) { push_word_rlist(op2, h); } else { push_byte_rlist(op2, h); }
}

static void handle_pushu_rlist(u8 op)
{
	const u8 h = op & 0x40;
	const u8 size = op & 0x08;
	const u8 op2 = xa_program_read_byte(m_pc++);
	if (size) { pushu_word_rlist(op2, h); } else { pushu_byte_rlist(op2, h); }
}

static void handle_pop_rlist(u8 op)
{
	const u8 h = op & 0x40;
	const u8 size = op & 0x08;
	const u8 op2 = xa_program_read_byte(m_pc++);
	if (size) { pop_word_rlist(op2, h); } else { pop_byte_rlist(op2, h); }
}

static void handle_popu_rlist(u8 op)
{
	const u8 h = op & 0x40;
	const u8 size = op & 0x08;
	const u8 op2 = xa_program_read_byte(m_pc++);
	if (size) { popu_word_rlist(op2, h); } else { popu_byte_rlist(op2, h); }
}

static void e_nop(u8 op)
{
	do_nop();
}

static void e_bitgroup(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);

	u16 bit = ((op2 & 0x03) << 8) | op3;

	switch (op2 & 0xf0)
	{
	case 0x00: clr_bit(bit); break;
	case 0x10: setb_bit(bit); break;
	case 0x20: mov_c_bit(bit);  break;
	case 0x30: mov_bit_c(bit); break;
	case 0x40: anl_c_bit(bit); break;
	case 0x50: anl_c_notbit(bit); break;
	case 0x60: orl_c_bit(bit); break;
	case 0x70: orl_c_notbit(bit); break;
	default:   LOGMASKED(LOG_DEBUG,  "illegal bit op %s", get_bittext(bit) ); do_nop(); break;
	}
	return;
}

static void e_add(u8 op)
{
	handle_alu_type0(op, 0);
}

static void e_push_rlist(u8 op)
{

	handle_push_rlist(op);
}

static void e_addc(u8 op)
{
	handle_alu_type0(op, 1);
}

static void e_pushu_rlist(u8 op)
{

	handle_pushu_rlist(op);
}

static void e_sub(u8 op)
{
	handle_alu_type0(op, 2);
}

static void e_pop_rlist(u8 op)
{

	handle_pop_rlist(op);
}

static void e_subb(u8 op)
{
	handle_alu_type0(op, 3);
}

static void e_popu_rlist(u8 op)
{

	handle_popu_rlist(op);
}

static void e_lea_offset8(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 offs8 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0x70) >> 4;
	const u8 rs = (op2 & 0x07);
	lea_word_rd_rs_off8(rd, rs, offs8);
}

static void e_lea_offset16(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);
	const u8 op4 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0x70) >> 4;
	const u8 rs = (op2 & 0x07);
	const u16 offs16 = (op3 << 8) | op4;
	lea_word_rd_rs_off16(rd, rs, offs16);
}

static void e_cmp(u8 op)
{
	handle_alu_type0(op, 4);
}

static void e_xch_type1(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x07);

	if (size) { xch_word_rd_indrs(rd, rs); } else { xch_byte_rd_indrs(rd, rs); }
}

static void e_and(u8 op)
{
	handle_alu_type0(op, 5);
}

static void e_xch_type2(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x0f);
	if (size) { xch_word_rd_rs(rd, rs); } else { xch_byte_rd_rs(rd, rs); }
}

static void e_or(u8 op)
{
	handle_alu_type0(op, 6);
}

static void e_xor(u8 op)
{
	handle_alu_type0(op, 7);
}

static void e_movc_rd_rsinc(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	int rd = (op2 & 0xf0) >> 4;
	int rs = (op2 & 0x07);
	if (size) { movc_word_rd_indrsinc(rd, rs); } else { movc_byte_rd_indrsinc(rd, rs); }
}

static void e_mov(u8 op)
{
	handle_alu_type0(op, 8);
}

static void e_pushpop_djnz_subgroup(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	if (op2 & 0x08)
	{
		const u8 rel8 = xa_program_read_byte(m_pc++);
		int rd = (op2 & 0xf0) >> 4;
		if (size) { djnz_word_rd_rel8(rd, rel8); } else { djnz_byte_rd_rel8(rd, rel8); }
	}
	else
	{
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u16 direct = ((op2 & 0x07) << 8) | op3;
		switch (op2 & 0xf0)
		{
		case 0x00: { if (size) { popu_word_direct(direct); } else { popu_byte_direct(direct); } break; }
		case 0x10: { if (size) { pop_word_direct(direct); } else { pop_byte_direct(direct); } break; }
		case 0x20: { if (size) { pushu_word_direct(direct); } else { pushu_byte_direct(direct); } break; }
		case 0x30: { if (size) { push_word_direct(direct); } else { push_byte_direct(direct); } break; }
		default: LOGMASKED(LOG_DEBUG, "illegal push/pop"); do_nop(); break;
		}
	}
}

static void e_g9_subgroup(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	if ((op2 & 0x0f) < 0x08)
	{
		int rd = (op2 & 0x70) >> 4;
		int rs = (op2 & 0x07);
		if (size) { mov_word_indrdinc_indrsinc(rd, rs); } else { mov_byte_indrdinc_indrsinc(rd, rs); }
	}
	else
	{
		switch (op2 & 0x0f)
		{
		case 0x08: { int rd = (op2 & 0xf0) >> 4; da_rd(rd); break;  }
		case 0x09: { int rd = (op2 & 0xf0) >> 4; if (size) { sext_word_rd(rd); } else { sext_byte_rd(rd); } break; }
		case 0x0a: { int rd = (op2 & 0xf0) >> 4; if (size) { cpl_word_rd(rd); } else { cpl_byte_rd(rd); } break; }
		case 0x0b: { int rd = (op2 & 0xf0) >> 4; if (size) { neg_word_rd(rd); } else { neg_byte_rd(rd); } break; }
		case 0x0c: { movc_a_apc(); break; }
		case 0x0e: { movc_a_adptr(); break; }
		case 0x0f: { int reg = (op2 & 0xf0) >> 4; if (!size)    { mov_rd_usp(reg); } else { mov_usp_rs(reg); } break; }
		default: { LOGMASKED(LOG_DEBUG, "illegal %02x", op2); do_nop(); break; }
		}
	}
}

static void e_alu(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	handle_alu_type1(op, op2);
}

static void e_jb_mov_subgroup(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);
	const u8 op4 = xa_program_read_byte(m_pc++);

	if (op2 & 0x80)
	{
		int bit = ((op2 & 0x03) << 8) | op3;
		switch (op2 & 0x70)
		{
		case 0x00: jb_bit_rel8(bit, op4); break;
		case 0x20: jnb_bit_rel8(bit, op4); break;
		case 0x40: jbc_bit_rel8(bit, op4); break;
		default:   LOGMASKED(LOG_DEBUG,  "illegal conditional jump %s $%02x", get_bittext(bit), expand_rel8(op4) ); break;
		}
	}
	else
	{
		int direct_dst = ((op2 & 0x70) << 4) | op3;
		int direct_src = ((op2 & 0x07) << 8) | op4;
		int size = op & 0x08;
		if (size) { mov_word_direct_direct(direct_dst, direct_src); } else { mov_byte_direct_direct(direct_dst, direct_src); }
	}
}

static void e_movdir(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	const u16 direct = ((op2 & 0x07) << 8) | op3;

	if (op2 & 0x08)
	{
		const u8 rd = op2 & (0xf0) >> 4;
		if (size) { xch_word_rd_direct(rd, direct); } else { xch_byte_rd_direct(rd, direct); }
	}
	else
	{
		if (op2 & 0x80)
		{
			const u8 rs = op2 & (0x70) >> 4;
			if (size) { mov_word_direct_indrs(direct, rs); } else { mov_byte_direct_indrs(direct, rs); }
		}
		else
		{
			const u8 rd = op2 & (0x70) >> 4;
			if (size) { mov_word_indrd_direct(rd, direct); } else { mov_byte_indrd_direct(rd, direct); }
		}
	}
}

static void e_adds(u8 op)
{
	handle_adds_movs(op, 0);
}

static void e_movx_subgroup(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	if (op2 & 0x08)
	{
		const u8 rs = (op2 & 0xf0) >> 4;
		const u8 rd = (op2 & 0x07);
		if (size) { movx_word_indrd_rs(rd, rs); } else { movx_byte_indrd_rs(rd, rs); }
	}
	else
	{
		const u8 rd = (op2 & 0xf0) >> 4;
		const u8 rs = (op2 & 0x07);
		if (size) { movx_word_rd_indrs(rd, rs); } else { movx_byte_rd_indrs(rd, rs); }
	}
}

static void e_rr(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 data = rd & 0x0f;
	if (size) { rr_word_rd_data4(rd, data); } else { rr_byte_rd_data4(rd, data); }
}

static void e_movs(u8 op)
{
	handle_adds_movs(op, 1);
}

static void e_rrc(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 data = rd & 0x0f;
	if (size) { rrc_word_rd_data4(rd, data); } else { rrc_byte_rd_data4(rd, data); }
}

static void e_lsr_fc(u8 op)
{
	int size = (op & 0x0c) >> 2;
	if (size == 0x01)
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u32 addr = (op2 << 8) | op3 | (op4 << 16);
		fcall_addr24(addr);
	}
	else
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const u8 rs = (op2 & 0x0f);
		if (size == 0) lsr_byte_rd_rs(rd, rs);
		else if (size == 2) lsr_word_rd_rs(rd, rs);
		else if (size == 3) lsr_dword_rd_rs(rd, rs);
	}
}

static void e_asl_c(u8 op)
{
	int size = (op & 0x0c) >> 2;
	if (size == 0x01)
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 op3 = xa_program_read_byte(m_pc++);
		u16 rel16 = (op2 << 8) | op3;
		call_rel16(rel16);
	}
	else
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const u8 rs = (op2 & 0x0f);
		if (size == 0) asl_byte_rd_rs(rd, rs);
		else if (size == 2) asl_word_rd_rs(rd, rs);
		else if (size == 3) asl_dword_rd_rs(rd, rs);
	}
}

static void e_asr_c(u8 op)
{
	int size = (op & 0x0c) >> 2;
	if (size == 0x01)
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 rs = op2 & 0x07;
		call_indrs(rs);
	}
	else
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 rd = (op2 & 0xf0) >> 4;
		const u8 rs = (op2 & 0x0f);
		if (size == 0) asr_byte_rd_rs(rd, rs);
		else if (size == 2) asr_word_rd_rs(rd, rs);
		else if (size == 3) asr_dword_rd_rs(rd, rs);
	}
}

static void e_norm(u8 op)
{
	int size = (op & 0x0c) >> 2;
	if (size == 0x01)
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		LOGMASKED(LOG_DEBUG, "illegal %02x", op2); do_nop();
	}
	else
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		int rd = (op2 & 0xf0) >> 4;
		int rs = (op2 & 0x0f);
		if (size == 0) norm_byte_rd_rs(rd, rs);
		else if (size == 2) norm_word_rd_rs(rd, rs);
		else if (size == 3) norm_dword_rd_rs(rd, rs);
	}
}

static void e_lsr_fj(u8 op)
{
	int size = (op & 0x0c) >> 2;
	if (size == 0x01)
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 op3 = xa_program_read_byte(m_pc++);
		const u8 op4 = xa_program_read_byte(m_pc++);
		const u32 addr = (op2 << 8) | op3 | (op4 << 16);
		fjmp_addr24(addr);
	}
	else
	{
		handle_shift(op, 2);
	}
}

static void e_asl_j(u8 op)
{
	int size = (op & 0x0c) >> 2;
	if (size == 0x01)
	{
		const u8 op2 = xa_program_read_byte(m_pc++);
		const u8 op3 = xa_program_read_byte(m_pc++);
		u16 offset = (op2 << 8) | op3;
		jmp_rel16(offset);
	}
	else
	{
		handle_shift(op, 0);
	}
}

static void e_asr_j(u8 op)
{
	int size = (op & 0x0c) >> 2;
	const u8 op2 = xa_program_read_byte(m_pc++);
	if (size == 0x01)
	{
		switch (op2 & 0xf0)
		{
		case 0x10: reset(); break;
		case 0x30: trap_data4(op2 & 0x0f); break;
		case 0x40: jmp_ind_adptr(); break;
		case 0x60: jmp_dblindrs(op2 & 0x07); break;
		case 0x70: jmp_indrs(op2 & 0x07); break;
		case 0x80: ret(); break;
		case 0x90: reti(); break;
		default:   LOGMASKED(LOG_DEBUG, "illegal"); do_nop(); break;
		}
	}
	else
	{
		handle_shift(op, 1);
	}
}

static void e_rl(u8 op)
{
	int size = op & 0x08;
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 data4 = (op2 & 0x0f);
	if (size) { rl_word_rd_data4(rd, data4); } else { rl_byte_rd_data4(rd, data4); }
}

static void e_rlc(u8 op)
{
	int size = op & 0x08;
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 data4 = (op2 & 0x0f);
	if (size) { rlc_word_rd_data4(rd, data4); } else { rlc_byte_rd_data4(rd, data4); }
}

static void e_djnz_cjne(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);
	const u8 op4 = xa_program_read_byte(m_pc++);
	int size = op & 0x08;

	const u16 direct = ((op2 & 0x07) << 8) | op3;
	if (op2 & 0x08)
	{
		if (size) { djnz_word_direct_rel8(direct, op4); } else { djnz_byte_direct_rel8(direct, op4); }
	}
	else
	{
		int rd = (op2 & 0xf0) >> 4;
		if (size) { cjne_word_rd_direct_rel8(rd, direct, op4); } else { cjne_byte_rd_direct_rel8(rd, direct, op4); }
	}
}

static void e_mulu_b(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x0f);
	mulu_byte_rd_rs(rd, rs);
}

static void e_divu_b(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x0f);
	divu_byte_rd_rs(rd, rs);
}

static void e_mulu_w(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x0f);
	mulu_word_rd_rs(rd, rs);
}

static void e_divu_w(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x0f);
	divu_word_rd_rs(rd, rs);
}

static void e_mul_w(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x0f);
	mul_word_rd_rs(rd, rs);
}

static void e_div_w(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;
	const u8 rs = (op2 & 0x0f);
	div_word_rd_rs(rd, rs);
}

static void e_div_data8(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 data8 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xf0) >> 4;

	switch (op2 & 0x0f)
	{
	case 0x00: { mulu_byte_rd_data8(rd, data8); break; }
	case 0x01: { divu_byte_rd_data8(rd, data8); break; }
	case 0x03: { divu_word_rd_data8(rd, data8); break; }
	case 0x0b: { div_word_rd_data8(rd, data8);break; }
	default:   { LOGMASKED(LOG_DEBUG, "illegal mul/div data8 %s #$%02x", m_regnames8[rd], data8); do_nop(); break; }
	}
}

static void e_div_d16(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);
	const u8 op4 = xa_program_read_byte(m_pc++);
	const u16 data16 = (op3 << 8) | op4;
	switch (op2 & 0x0f)
	{
	case 0x00: { const u8 rd = (op2 & 0xf0) >> 4; mulu_word_rd_data16(rd, data16); break; }
	case 0x01: { const u8 rd = (op2 & 0xe0) >> 4; divu_dword_rd_data16(rd, data16); break; }
	case 0x08: { const u8 rd = (op2 & 0xf0) >> 4; mul_word_rd_data16(rd, data16); break; }
	case 0x09: { const u8 rd = (op2 & 0xe0) >> 4; div_dword_rd_data16(rd, data16); break; }
	default:   { const u8 rd = (op2 & 0xf0) >> 4; LOGMASKED(LOG_DEBUG, "illegal mul/div data16 %s, #$%04x", m_regnames16[rd], data16); do_nop();  break; }
	}
}

static void e_divu_d(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xe0) >> 4;
	const u8 rs = (op2 & 0x0f);
	divu_dword_rd_rs(rd, rs);
}

static void e_div_d(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 rd = (op2 & 0xe0) >> 4;
	const u8 rs = (op2 & 0x0f);
	div_dword_rd_rs(rd, rs);
}

static void e_cjne_d8(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);
	const u8 op4 = xa_program_read_byte(m_pc++);
	if (op2 & 0x08)
	{
		const u8 rd = (op2 & 0x70) >> 4;
		cjne_indrd_data8_rel8(rd, op4, op3);
	}
	else
	{
		const u8 rd = (op2 & 0xf0) >> 4;
		cjne_rd_data8_rel8(rd, op4, op3);
	}
}

static void e_cjne_d16(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	const u8 op3 = xa_program_read_byte(m_pc++);
	const u8 op4 = xa_program_read_byte(m_pc++);
	const u8 op5 = xa_program_read_byte(m_pc++);
	const u16 data = (op4 << 8) | op5;
	if (op2 & 0x08)
	{
		const u8 rd = (op2 & 0x70) >> 4;
		cjne_indrd_data16_rel8(rd, data, op3);
	}
	else
	{
		const u8 rd = (op2 & 0xf0) >> 4;
		cjne_rd_data16_rel8(rd, data, op3);
	}
}

static void e_jz_rel8(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	jz_rel8(op2);
}

static void e_jnz_rel8(u8 op)
{
	const u8 op2 = xa_program_read_byte(m_pc++);
	jnz_rel8(op2);
}

static void e_branch(u8 op)
{
	const u8 rel8 = xa_program_read_byte(m_pc++);
	switch (op & 0x0f)
	{
	case 0x00: bcc_rel8(rel8); break;
	case 0x01: bcs_rel8(rel8); break;
	case 0x02: bne_rel8(rel8); break;
	case 0x03: beq_rel8(rel8); break;
	case 0x04: bnv_rel8(rel8); break;
	case 0x05: bov_rel8(rel8); break;
	case 0x06: bpl_rel8(rel8); break;
	case 0x07: bmi_rel8(rel8); break;
	case 0x08: bg_rel8(rel8); break;
	case 0x09: bl_rel8(rel8); break;
	case 0x0a: bge_rel8(rel8); break;
	case 0x0b: blt_rel8(rel8); break;
	case 0x0c: bgt_rel8(rel8); break;
	case 0x0d: ble_rel8(rel8); break;
	case 0x0e: br_rel8(rel8); break;
	case 0x0f: LOGMASKED(LOG_DEBUG, "Illegal branch type"); do_nop(); break;
	}
}

static void e_bkpt(u8 op)
{
	fatalerror("Unsupported XA operation at %04X", m_pc);
}

static void device_reset(void)
{
	u16 temppsw = xa_program_read_word(0);
	sfr_PSWL_w(temppsw & 0xff);
	sfr_PSWH_w((temppsw >> 8) & 0xff);
	m_pc = xa_program_read_word(2);

	m_WDCON = 0x00;
	m_SCR = 0x00;
	m_IEL = 0x00;

	m_USP = 0x0100;
	m_SSP = 0x0100;

	for (int i = 0; i < 4 * 4 + 4; i++)
	{
		m_regs[i] = 0x0000;
	}

	m_nflag = 0;
	m_zflag = 0;
	m_cflag = 0;
	m_vflag = 0;
	m_acflag = 0;

	m_pagezeromode = 1;

	m_in_interrupt = 0;
	m_irq_pending = 0;

	for (int i = 0; i < 4; i++)
	{
		m_PxCFGA[i] = 0;
		m_PxCFGB[i] = 0;
	}
}

static void check_external_irq_level(int level)
{
	if (m_IEL & 0x80)
	{
		if (m_IEL & (1 << level))
		{
			if (m_irq_pending & (1 << level))
			{
				LOGMASKED(LOG_DEBUG, "testing irq %d\n", level);
				int vector = 0x20 + level;
				push_word_to_system_stack(sfr_PSWH_r());
				push_word_to_system_stack(sfr_PSWL_r());
				push_word_to_system_stack(m_pc);

				u16 temppsw = xa_program_read_word(vector*4);
				sfr_PSWL_w(temppsw & 0xff);
				sfr_PSWH_w((temppsw >> 8) & 0xff);
				m_pc = xa_program_read_word((vector*4)+2);
				m_in_interrupt = 1;
				m_irq_pending &= ~(1 << level);
				if (xa.irq_callback) xa.irq_callback(level);
			}
		}
	}
}

static void check_interrupts(void)
{
	if (m_irq_pending)
	{
		if (!m_in_interrupt)
		{
			for (int i = 0; i < 4; i++)
				check_external_irq_level(i);
		}
	}

}

static u32 gr32(int reg)
{
	if (reg & 1)
		fatalerror("Unsupported XA operation at %04X", m_pc);

	u16 reg1 = gr16(reg);
	u16 reg2 = gr16(reg + 1);

	return (reg2 << 16) | reg1;
}

static void sr32(int reg, u32 data)
{
	if (reg & 1)
		fatalerror("Unsupported XA operation at %04X", m_pc);

	sr16(reg, data & 0xffff);
	sr16(reg + 1, (data >> 16) & 0xffff);
}

static u16 do_subb_16(u16 val1, u16 val2)
{
	return do_sub_16_helper(val1, val2, get_c_flag());
}

static u16 do_sub_16(u16 val1, u16 val2)
{
	return do_sub_16_helper(val1, val2, 0);
}

static u16 do_sub_16_helper(u16 val1, u16 val2, u8 c)
{
	u32 result = val1 - (val2 + c);
	s32 result1 = (s16)val1 - (s16)(val2 + c);
	u32 result3 = (val1 & 0x0fff) - ((val2 & 0x0fff) + c);

	if (result & 0x10000) set_c_flag(); else clear_c_flag();
	if (result3 & 0x1000) set_ac_flag(); else clear_ac_flag();
	if ((result1 < -32768 || result1 > 32767)) set_v_flag(); else clear_v_flag();

	do_nz_flags_16((u16)result);
	return (u16)result;
}

static u16 do_addc_16(u16 val1, u16 val2)
{
	return do_add_16_helper(val1, val2, get_c_flag());
}

static u16 do_add_16(u16 val1, u16 val2)
{
	return do_add_16_helper(val1, val2, 0);
}

static u16 do_add_16_helper(u16 val1, u16 val2, u8 c)
{
	u32 result = val1 + val2 + c;
	s32 result1 = (s16)val1 + (s16)val2 + c;
	u32 result3 = (val1 & 0x0fff) + (val2 & 0x0fff) + c;
	if (result & 0x10000) set_c_flag(); else clear_c_flag();
	if (result3 & 0x1000) set_ac_flag(); else clear_ac_flag();
	if ((result1 < -32768 || result1 > 32767)) set_v_flag(); else clear_v_flag();

	do_nz_flags_16((u16)result);
	return (u16)result;
}

static u16 do_xor_16(u16 val1, u16 val2)
{
	u16 result = val1 ^ val2;
	do_nz_flags_16(result);
	return result;
}

static u16 do_or_16(u16 val1, u16 val2)
{
	u16 result = val1 | val2;
	do_nz_flags_16(result);
	return result;
}

static u16 do_and_16(u16 val1, u16 val2)
{
	u16 result = val1 & val2;
	do_nz_flags_16(result);
	return result;
}

static u32 asl32_helper(u32 fullreg, u8 amount)
{
	cy(6 + (amount >> 1));

	int lastbit = 0;

	u32 topbit = fullreg & 0x8000000;

	while (amount)
	{
		lastbit = fullreg & 0x8000000;
		fullreg = fullreg << 1;
		amount--;
	}

	if (lastbit)
		set_c_flag();
	else
		clear_c_flag();

	if (fullreg == 0)
		set_z_flag();
	else
		clear_z_flag();

	if (fullreg & 0x8000000)
		set_n_flag();
	else
		clear_n_flag();

	if (topbit != (fullreg & 0x8000000))
		set_v_flag();
	else
		clear_v_flag();

	return fullreg;
}

static u32 lsr32_helper(u32 fullreg, u8 amount)
{
	cy(6 + (amount >> 1));

	int lastbit = 0;

	while (amount)
	{
		lastbit = fullreg & 0x0000001;
		fullreg = fullreg >> 1;
		amount--;
	}

	if (lastbit)
		set_c_flag();
	else
		clear_c_flag();

	if (fullreg == 0)
		set_z_flag();
	else
		clear_z_flag();

	if (fullreg & 0x8000000)
		set_n_flag();
	else
		clear_n_flag();

	return fullreg;
}

static u16 lsr16_helper(u16 fullreg, u8 amount)
{
	cy(6 + (amount >> 1));

	int lastbit = 0;

	while (amount)
	{
		lastbit = fullreg & 0x0001;
		fullreg = fullreg >> 1;
		amount--;
	}

	if (lastbit)
		set_c_flag();
	else
		clear_c_flag();

	if (fullreg == 0)
		set_z_flag();
	else
		clear_z_flag();

	if (fullreg & 0x8000)
		set_n_flag();
	else
		clear_n_flag();

	return fullreg;
}

static u8 do_subb_8(u8 val1, u8 val2)
{
	return do_sub_8_helper(val1, val2, get_c_flag());
}

static u8 do_sub_8(u8 val1, u8 val2)
{
	return do_sub_8_helper(val1, val2, 0);
}

static u8 do_sub_8_helper(u8 val1, u8 val2, u8 c)
{
	u16 result = val1 - (val2 + c);
	s16 result1 = (s8)val1 - (s8)(val2 + c);
	u16 result3 = (val1 & 0x0f) - ((val2 & 0x0f) + c);
	if (result & 0x100) set_c_flag(); else clear_c_flag();
	if (result3 & 0x10) set_ac_flag(); else clear_ac_flag();
	if ((result1 < -128 || result1 > 127)) set_v_flag(); else clear_v_flag();

	do_nz_flags_8((u8)result);
	return (u8)result;
}

static u8 do_addc_8(u8 val1, u8 val2)
{
	return do_add_8_helper(val1, val2, get_c_flag());
}

static u8 do_add_8(u8 val1, u8 val2)
{
	return do_add_8_helper(val1, val2, 0);
}

static u8 do_add_8_helper(u8 val1, u8 val2, u8 c)
{
	u16 result = val1 + val2 + c;
	s16 result1 = (s8)val1 + (s8)val2 + c;
	u16 result3 = (val1 & 0x0f) + (val2 & 0x0f) + c;
	if (result & 0x100) set_c_flag(); else clear_c_flag();
	if (result3 & 0x10) set_ac_flag(); else clear_ac_flag();
	if ((result1 < -128 || result1 > 127)) set_v_flag(); else clear_v_flag();

	do_nz_flags_8((u8)result);
	return (u8)result;
}

static u8 do_xor_8(u8 val1, u8 val2)
{
	u8 result = val1 ^ val2;
	do_nz_flags_8(result);
	return result;
}

static u8 do_or_8(u8 val1, u8 val2)
{
	u8 result = val1 | val2;
	do_nz_flags_8(result);
	return result;
}

static u8 do_and_8(u8 val1, u8 val2)
{
	u8 result = val1 & val2;
	do_nz_flags_8(result);
	return result;
}

static u8 do_cjne_8_helper(u8 val1, u8 val2)
{
	u16 result = val1 - val2;
	do_nz_flags_8((u8)result);
	if (result & 0x100) set_c_flag(); else clear_c_flag();
	return (u8)result;
}

static void set_bit_8_helper(u16 bit, u8 val)
{
	int position = bit & 7;

	if (bit < 0x100)
	{
		int reg = ((bit & 0x1ff) >> 3);

		if (reg < 16)
			fatalerror("Unsupported XA operation at %04X", m_pc);
		else
			fatalerror("Unsupported XA operation at %04X", m_pc);
	}
	else if (bit < 0x200)
	{
		int addr = ((bit & 0x1ff) >> 3) + 0x20;
		fatalerror("Unsupported XA operation at %04X", m_pc);
	}

	int sfr = ((bit & 0x1ff) >> 3);
	u8 mask = (1 << position) ^ 0xff;

	u8 sfr_val = xa_sfr_read_byte(sfr) & mask;
	sfr_val |= (val << position);
	xa_sfr_write_byte(sfr, sfr_val);
}

static void do_nop(void)
{ cy(3); }

static void aluop_byte_rd_data8(int alu_op, u8 rd, u8 data8)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rd_data8(rd, data8); break;
	case 0x1: addc_byte_rd_data8(rd, data8); break;
	case 0x2: sub_byte_rd_data8(rd, data8); break;
	case 0x3: subb_byte_rd_data8(rd, data8); break;
	case 0x4: cmp_byte_rd_data8(rd, data8); break;
	case 0x5: and_byte_rd_data8(rd, data8); break;
	case 0x6: or_byte_rd_data8(rd, data8); break;
	case 0x7: xor_byte_rd_data8(rd, data8); break;
	case 0x8: mov_byte_rd_data8(rd, data8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); u8 result = do_add_8(rdval, data8); sr8(rd, result); cy(3); }

static void addc_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); u8 result = do_addc_8(rdval, data8); sr8(rd, result); cy(3); }

static void sub_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); u8 result = do_sub_8(rdval, data8); sr8(rd, result); cy(3); }

static void subb_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); u8 result = do_subb_8(rdval, data8); sr8(rd, result); cy(3); }

static void cmp_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); do_sub_8(rdval, data8); cy(3); }

static void and_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); u8 result = do_and_8(rdval, data8); sr8(rd, result); cy(3); }

static void or_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); u8 result = do_or_8(rdval, data8); sr8(rd, result); cy(3); }

static void xor_byte_rd_data8(u8 rd, u8 data8)
{ u8 rdval = gr8(rd); u8 result = do_xor_8(rdval, data8); sr8(rd, result); cy(3); }

static void mov_byte_rd_data8(u8 rd, u8 data8)
{ u8 result = data8; do_nz_flags_8(result); sr8(rd, result); cy(3); }

static void aluop_byte_indrd_data8(int alu_op, u8 rd, u8 data8)
{
	switch (alu_op)
	{
	case 0x0: add_byte_indrd_data8(rd, data8); break;
	case 0x1: addc_byte_indrd_data8(rd, data8); break;
	case 0x2: sub_byte_indrd_data8(rd, data8); break;
	case 0x3: subb_byte_indrd_data8(rd, data8); break;
	case 0x4: cmp_byte_indrd_data8(rd, data8); break;
	case 0x5: and_byte_indrd_data8(rd, data8); break;
	case 0x6: or_byte_indrd_data8(rd, data8); break;
	case 0x7: xor_byte_indrd_data8(rd, data8); break;
	case 0x8: mov_byte_indrd_data8(rd, data8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_byte_indrd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void addc_byte_indrd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void sub_byte_indrd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void subb_byte_indrd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void cmp_byte_indrd_data8(u8 rd, u8 data8)
{ u16 address = get_addr(rd); u8 rdval = rdat8(address); do_sub_8(rdval, data8); cy(3); }

static void and_byte_indrd_data8(u8 rd, u8 data8)
{ u16 address = get_addr(rd); u8 rdval = rdat8(address); u8 result = do_and_8(rdval, data8); wdat8(address, result); cy(3); }

static void or_byte_indrd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void xor_byte_indrd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_byte_indrd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void aluop_byte_indrdinc_data8(int alu_op, u8 rd, u8 data8)
{
	switch (alu_op)
	{
	case 0x0: add_byte_indrdinc_data8(rd, data8); break;
	case 0x1: addc_byte_indrdinc_data8(rd, data8); break;
	case 0x2: sub_byte_indrdinc_data8(rd, data8); break;
	case 0x3: subb_byte_indrdinc_data8(rd, data8); break;
	case 0x4: cmp_byte_indrdinc_data8(rd, data8); break;
	case 0x5: and_byte_indrdinc_data8(rd, data8); break;
	case 0x6: or_byte_indrdinc_data8(rd, data8); break;
	case 0x7: xor_byte_indrdinc_data8(rd, data8); break;
	case 0x8: mov_byte_indrdinc_data8(rd, data8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void addc_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void sub_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void subb_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void cmp_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void and_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void or_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void xor_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_byte_indrdinc_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void aluop_byte_rdoff8_data8(int alu_op, u8 rd, u8 offset8, u8 data8)
{
	switch (alu_op)
	{
	case 0x0: add_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x1: addc_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x2: sub_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x3: subb_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x4: cmp_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x5: and_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x6: or_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x7: xor_byte_indrdoff8_data8(rd, offset8, data8); break;
	case 0x8: mov_byte_indrdoff8_data8(rd, offset8, data8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_add_8(val, data8); wdat8(address, result); cy(6); }

static void addc_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_addc_8(val, data8); wdat8(address, result); cy(6); }

static void sub_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_sub_8(val, data8); wdat8(address, result); cy(6); }

static void subb_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_subb_8(val, data8); wdat8(address, result); cy(6); }

static void cmp_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); do_sub_8(val, data8); cy(6); }

static void and_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_and_8(val, data8); wdat8(address, result); cy(6); }

static void or_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_or_8(val, data8); wdat8(address, result); cy(6);  }

static void xor_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_xor_8(val, data8); wdat8(address, result); cy(6); }

static void mov_byte_indrdoff8_data8(u8 rd, u8 offset8, u8 data8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; do_nz_flags_8(data8); wdat8(address, data8); cy(5); }

static void aluop_byte_rdoff16_data8(int alu_op, u8 rd, u16 offset16, u8 data8)
{
	switch (alu_op)
	{
	case 0x0: add_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x1: addc_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x2: sub_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x3: subb_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x4: cmp_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x5: and_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x6: or_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x7: xor_byte_indrdoff16_data8(rd, offset16, data8); break;
	case 0x8: mov_byte_indrdoff16_data8(rd, offset16, data8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_add_8(val, data8); wdat8(address, result); cy(6); }

static void addc_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_addc_8(val, data8); wdat8(address, result); cy(6); }

static void sub_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_sub_8(val, data8); wdat8(address, result); cy(6); }

static void subb_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_subb_8(val, data8); wdat8(address, result); cy(6); }

static void cmp_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); do_sub_8(val, data8); cy(6); }

static void and_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_and_8(val, data8); wdat8(address, result); cy(6); }

static void or_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_or_8(val, data8); wdat8(address, result); cy(6);  }

static void xor_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u8 val = rdat8(address); u8 result = do_xor_8(val, data8); wdat8(address, result); cy(6); }

static void mov_byte_indrdoff16_data8(u8 rd, u16 offset16, u8 data8)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; do_nz_flags_8(data8); wdat8(address, data8); cy(5); }

static void aluop_byte_direct_data8(int alu_op, u16 direct, u8 data8)
{
	switch (alu_op)
	{
	case 0x0: add_byte_direct_data8(direct, data8); break;
	case 0x1: addc_byte_direct_data8(direct, data8); break;
	case 0x2: sub_byte_direct_data8(direct, data8); break;
	case 0x3: subb_byte_direct_data8(direct, data8); break;
	case 0x4: cmp_byte_direct_data8(direct, data8); break;
	case 0x5: and_byte_direct_data8(direct, data8); break;
	case 0x6: or_byte_direct_data8(direct, data8); break;
	case 0x7: xor_byte_direct_data8(direct, data8); break;
	case 0x8: mov_byte_direct_data8(direct, data8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); u8 result = do_add_8(val, data8); write_direct8(direct, result); cy(4); }

static void addc_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); u8 result = do_addc_8(val, data8); write_direct8(direct, result); cy(4);}

static void sub_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); u8 result = do_sub_8(val, data8); write_direct8(direct, result); cy(4); }

static void subb_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); u8 result = do_subb_8(val, data8); write_direct8(direct, result); cy(4);}

static void cmp_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); do_sub_8(val, data8); cy(4); }

static void and_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); u8 result = do_and_8(val, data8); write_direct8(direct, result); cy(4); }

static void or_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); u8 result = do_or_8(val, data8); write_direct8(direct, result); cy(4); }

static void xor_byte_direct_data8(u16 direct, u8 data8)
{ u8 val = read_direct8(direct); u8 result = do_xor_8(val, data8); write_direct8(direct, result); cy(4); }

static void mov_byte_direct_data8(u16 direct, u8 data8)
{ u8 result = data8; do_nz_flags_8(result); write_direct8(direct, result); cy(3); }

static void aluop_byte_rd_data16(int alu_op, u8 rd, u16 data16)
{
	switch (alu_op)
	{
	case 0x0: add_word_rd_data16(rd, data16); break;
	case 0x1: addc_word_rd_data16(rd, data16); break;
	case 0x2: sub_word_rd_data16(rd, data16); break;
	case 0x3: subb_word_rd_data16(rd, data16); break;
	case 0x4: cmp_word_rd_data16(rd, data16); break;
	case 0x5: and_word_rd_data16(rd, data16); break;
	case 0x6: or_word_rd_data16(rd, data16); break;
	case 0x7: xor_word_rd_data16(rd, data16); break;
	case 0x8: mov_word_rd_data16(rd, data16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); u16 result = do_add_16(rdval, data16); sr16(rd, result); cy(3); }

static void addc_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); u16 result = do_addc_16(rdval, data16); sr16(rd, result); cy(3); }

static void sub_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); u16 result = do_sub_16(rdval, data16); sr16(rd, result); cy(3); }

static void subb_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); u16 result = do_subb_16(rdval, data16); sr16(rd, result); cy(3); }

static void cmp_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); do_sub_16(rdval, data16); cy(3); }

static void and_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); u16 result = do_and_16(rdval, data16); sr16(rd, result); cy(3); }

static void or_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); u16 result = do_or_16(rdval, data16); sr16(rd, result); cy(3); }

static void xor_word_rd_data16(u8 rd, u16 data16)
{ u16 rdval = gr16(rd); u16 result = do_xor_16(rdval, data16); sr16(rd, result); cy(3); }

static void mov_word_rd_data16(u8 rd, u16 data16)
{ u16 result = data16; do_nz_flags_16(result); sr16(rd, result); cy(3); }

static void aluop_byte_indrd_data16(int alu_op, u8 rd, u16 data16)
{
	switch (alu_op)
	{
	case 0x0: add_word_indrd_data16(rd, data16); break;
	case 0x1: addc_word_indrd_data16(rd, data16); break;
	case 0x2: sub_word_indrd_data16(rd, data16); break;
	case 0x3: subb_word_indrd_data16(rd, data16); break;
	case 0x4: cmp_word_indrd_data16(rd, data16); break;
	case 0x5: and_word_indrd_data16(rd, data16); break;
	case 0x6: or_word_indrd_data16(rd, data16); break;
	case 0x7: xor_word_indrd_data16(rd, data16); break;
	case 0x8: mov_word_indrd_data16(rd, data16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); u16 result = do_add_16(val, data16); wdat16(address, result); cy(4); }

static void addc_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); u16 result = do_addc_16(val, data16); wdat16(address, result); cy(4); }

static void sub_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); u16 result = do_sub_16(val, data16); wdat16(address, result); cy(4); }

static void subb_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); u16 result = do_subb_16(val, data16); wdat16(address, result); cy(4); }

static void cmp_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); do_sub_16(val, data16); cy(4); }

static void and_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); u16 result = do_and_16(val, data16); wdat16(address, result); cy(4); }

static void or_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); u16 result = do_or_16(val, data16); wdat16(address, result); cy(4); }

static void xor_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); u16 val = rdat16(address); u16 result = do_xor_16(val, data16); wdat16(address, result); cy(4); }

static void mov_word_indrd_data16(u8 rd, u16 data16)
{ u16 address = get_addr(rd); do_nz_flags_16(data16); wdat16(address, data16); cy(3); }

static void aluop_byte_indrdinc_data16(int alu_op, u8 rd, u16 data16)
{
	switch (alu_op)
	{
	case 0x0: add_word_indrdinc_data16(rd, data16); break;
	case 0x1: addc_word_indrdinc_data16(rd, data16); break;
	case 0x2: sub_word_indrdinc_data16(rd, data16); break;
	case 0x3: subb_word_indrdinc_data16(rd, data16); break;
	case 0x4: cmp_word_indrdinc_data16(rd, data16); break;
	case 0x5: and_word_indrdinc_data16(rd, data16); break;
	case 0x6: or_word_indrdinc_data16(rd, data16); break;
	case 0x7: xor_word_indrdinc_data16(rd, data16); break;
	case 0x8: mov_word_indrdinc_data16(rd, data16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void addc_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void sub_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void subb_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void cmp_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void and_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void or_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void xor_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_word_indrdinc_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void aluop_byte_rdoff8_data16(int alu_op, u8 rd, u8 offset8, u16 data16)
{
	switch (alu_op)
	{
	case 0x0: add_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x1: addc_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x2: sub_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x3: subb_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x4: cmp_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x5: and_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x6: or_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x7: xor_word_indrdoff8_data16(rd, offset8, data16); break;
	case 0x8: mov_word_indrdoff8_data16(rd, offset8, data16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_add_16(val, data16); wdat16(address, result); cy(6); }

static void addc_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_addc_16(val, data16); wdat16(address, result); cy(6); }

static void sub_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_sub_16(val, data16); wdat16(address, result); cy(6); }

static void subb_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_subb_16(val, data16); wdat16(address, result); cy(6); }

static void cmp_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); do_sub_16(val, data16); cy(6); }

static void and_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_and_16(val, data16); wdat16(address, result); cy(6); }

static void or_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_or_16(val, data16); wdat16(address, result); cy(6); }

static void xor_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_xor_16(val, data16); wdat16(address, result); cy(6); }

static void mov_word_indrdoff8_data16(u8 rd, u8 offset8, u16 data16)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; do_nz_flags_16(data16); wdat16(address, data16); cy(5);  }

static void aluop_byte_rdoff16_data16(int alu_op, u8 rd, u16 offset16, u16 data16)
{
	switch (alu_op)
	{
	case 0x0: add_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x1: addc_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x2: sub_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x3: subb_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x4: cmp_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x5: and_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x6: or_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x7: xor_word_indrdoff16_data16(rd, offset16, data16); break;
	case 0x8: mov_word_indrdoff16_data16(rd, offset16, data16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_add_16(val, data16); wdat16(address, result); cy(6); }

static void addc_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_addc_16(val, data16); wdat16(address, result); cy(6); }

static void sub_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_sub_16(val, data16); wdat16(address, result); cy(6); }

static void subb_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_subb_16(val, data16); wdat16(address, result); cy(6); }

static void cmp_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); do_sub_16(val, data16); cy(6); }

static void and_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_and_16(val, data16); wdat16(address, result); cy(6); }

static void or_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_or_16(val, data16); wdat16(address, result); cy(6); }

static void xor_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 result = do_xor_16(val, data16); wdat16(address, result); cy(6); }

static void mov_word_indrdoff16_data16(u8 rd, u16 offset16, u16 data16)
{ u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; do_nz_flags_16(data16); wdat16(address, data16); cy(5);  }

static void aluop_byte_direct_data16(int alu_op, u16 direct, u16 data16)
{
	switch (alu_op)
	{
	case 0x0: add_word_direct_data16(direct, data16); break;
	case 0x1: addc_word_direct_data16(direct, data16); break;
	case 0x2: sub_word_direct_data16(direct, data16); break;
	case 0x3: subb_word_direct_data16(direct, data16); break;
	case 0x4: cmp_word_direct_data16(direct, data16); break;
	case 0x5: and_word_direct_data16(direct, data16); break;
	case 0x6: or_word_direct_data16(direct, data16); break;
	case 0x7: xor_word_direct_data16(direct, data16); break;
	case 0x8: mov_word_direct_data16(direct, data16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); u16 result = do_add_16(val, data16); write_direct16(direct, result); cy(4); }

static void addc_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); u16 result = do_addc_16(val, data16); write_direct16(direct, result); cy(4);}

static void sub_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); u16 result = do_sub_16(val, data16); write_direct16(direct, result); cy(4); }

static void subb_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); u16 result = do_subb_16(val, data16); write_direct16(direct, result); cy(4);}

static void cmp_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); do_sub_16(val, data16); cy(4); }

static void and_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); u16 result = do_and_16(val, data16); write_direct16(direct, result); cy(4); }

static void or_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); u16 result = do_or_16(val, data16); write_direct16(direct, result); cy(4); }

static void xor_word_direct_data16(u16 direct, u16 data16)
{ u16 val = read_direct16(direct); u16 result = do_xor_16(val, data16); write_direct16(direct, result); cy(4); }

static void mov_word_direct_data16(u16 direct, u16 data16)
{ u16 result = data16; do_nz_flags_16(result); write_direct16(direct, result); cy(3); }

static void aluop_word_rd_rs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_rd_rs(rd, rs); break;
	case 0x1: addc_word_rd_rs(rd, rs); break;
	case 0x2: sub_word_rd_rs(rd, rs); break;
	case 0x3: subb_word_rd_rs(rd, rs); break;
	case 0x4: cmp_word_rd_rs(rd, rs); break;
	case 0x5: and_word_rd_rs(rd, rs); break;
	case 0x6: or_word_rd_rs(rd, rs); break;
	case 0x7: xor_word_rd_rs(rd, rs); break;
	case 0x8: mov_word_rd_rs(rd, rs); break;
	default: fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static void aluop_byte_rd_rs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rd_rs(rd, rs); break;
	case 0x1: addc_byte_rd_rs(rd, rs); break;
	case 0x2: sub_byte_rd_rs(rd, rs); break;
	case 0x3: subb_byte_rd_rs(rd, rs); break;
	case 0x4: cmp_byte_rd_rs(rd, rs); break;
	case 0x5: and_byte_rd_rs(rd, rs); break;
	case 0x6: or_byte_rd_rs(rd, rs); break;
	case 0x7: xor_byte_rd_rs(rd, rs); break;
	case 0x8: mov_byte_rd_rs(rd, rs); break;
	default: fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static void add_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); u16 result = do_add_16(rdval, rsval); sr16(rd, result); cy(3);}

static void addc_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); u16 result = do_addc_16(rdval, rsval); sr16(rd, result); cy(3);}

static void sub_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); u16 result = do_sub_16(rdval, rsval); sr16(rd, result); cy(3);}

static void subb_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); u16 result = do_subb_16(rdval, rsval); sr16(rd, result); cy(3);}

static void cmp_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); do_sub_16(rdval, rsval); cy(3); }

static void and_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); u16 result = do_and_16(rdval, rsval); sr16(rd, result); cy(3); }

static void or_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); u16 result = do_or_16(rdval, rsval); sr16(rd, result); cy(3); }

static void xor_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); u16 result = do_xor_16(rdval, rsval); sr16(rd, result); cy(3);}

static void mov_word_rd_rs(u8 rd, u8 rs)
{ u16 val = gr16(rs);  do_nz_flags_16(val); sr16(rd, val); cy(3); }

static void add_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); u8 result = do_add_8(rdval, rsval); sr8(rd, result); cy(3); }

static void addc_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); u8 result = do_addc_8(rdval, rsval); sr8(rd, result); cy(3); }

static void sub_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); u8 result = do_sub_8(rdval, rsval); sr8(rd, result); cy(3); }

static void subb_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); u8 result = do_subb_8(rdval, rsval); sr8(rd, result); cy(3); }

static void cmp_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); do_sub_8(rdval, rsval); cy(3);}

static void and_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); u8 result = do_and_8(rdval, rsval); sr8(rd, result); cy(3); }

static void or_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); u8 result = do_or_8(rdval, rsval); sr8(rd, result); cy(3); }

static void xor_byte_rd_rs(u8 rd, u8 rs)
{ u8 rdval = gr8(rd); u8 rsval = gr8(rs); u8 result = do_xor_8(rdval, rsval); sr8(rd, result); cy(3); }

static void mov_byte_rd_rs(u8 rd, u8 rs)
{ u8 val = gr8(rs);  do_nz_flags_8(val); sr8(rd, val); cy(3);}

static void aluop_word_rd_indrs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_rd_indrs(rd, rs); break;
	case 0x1: addc_word_rd_indrs(rd, rs); break;
	case 0x2: sub_word_rd_indrs(rd, rs); break;
	case 0x3: subb_word_rd_indrs(rd, rs); break;
	case 0x4: cmp_word_rd_indrs(rd, rs); break;
	case 0x5: and_word_rd_indrs(rd, rs); break;
	case 0x6: or_word_rd_indrs(rd, rs); break;
	case 0x7: xor_word_rd_indrs(rd, rs); break;
	case 0x8: mov_word_rd_indrs(rd, rs); break;
	default: fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static void aluop_byte_rd_indrs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rd_indrs(rd, rs); break;
	case 0x1: addc_byte_rd_indrs(rd, rs); break;
	case 0x2: sub_byte_rd_indrs(rd, rs); break;
	case 0x3: subb_byte_rd_indrs(rd, rs); break;
	case 0x4: cmp_byte_rd_indrs(rd, rs); break;
	case 0x5: and_byte_rd_indrs(rd, rs); break;
	case 0x6: or_byte_rd_indrs(rd, rs); break;
	case 0x7: xor_byte_rd_indrs(rd, rs); break;
	case 0x8: mov_byte_rd_indrs(rd, rs); break;
	default: fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static void add_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_word_rd_indrs(u8 rd, u8 rs)
{ u16 address = get_addr(rs); u16 val = rdat16(address); do_nz_flags_16(val); sr16(rd, val); cy(3); }

static void add_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_rd_indrs(u8 rd, u8 rs)
{ u16 address = get_addr(rs); u8 val = rdat8(address); do_nz_flags_8(val); sr8(rd, val); cy(3); }

static void aluop_word_indrd_rs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_indrd_rs(rd, rs); break;
	case 0x1: addc_word_indrd_rs(rd, rs); break;
	case 0x2: sub_word_indrd_rs(rd, rs); break;
	case 0x3: subb_word_indrd_rs(rd, rs); break;
	case 0x4: cmp_word_indrd_rs(rd, rs); break;
	case 0x5: and_word_indrd_rs(rd, rs); break;
	case 0x6: or_word_indrd_rs(rd, rs); break;
	case 0x7: xor_word_indrd_rs(rd, rs); break;
	case 0x8: mov_word_indrd_rs(rd, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_indrd_rs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_indrd_rs(rd, rs); break;
	case 0x1: addc_byte_indrd_rs(rd, rs); break;
	case 0x2: sub_byte_indrd_rs(rd, rs); break;
	case 0x3: subb_byte_indrd_rs(rd, rs); break;
	case 0x4: cmp_byte_indrd_rs(rd, rs); break;
	case 0x5: and_byte_indrd_rs(rd, rs); break;
	case 0x6: or_byte_indrd_rs(rd, rs); break;
	case 0x7: xor_byte_indrd_rs(rd, rs); break;
	case 0x8: mov_byte_indrd_rs(rd, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_word_indrd_rs(u8 rd, u8 rs)
{ u16 val = gr16(rs); do_nz_flags_16(val); u16 addr = get_addr(rd); wdat16(addr, val); cy(3); }

static void add_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_indrd_rs(u8 rd, u8 rs)
{ u8 val = gr8(rs); do_nz_flags_8(val); u16 addr = get_addr(rd); wdat8(addr, val); cy(3); }

static void aluop_word_rd_indrsinc(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_rd_indrsinc(rd, rs); break;
	case 0x1: addc_word_rd_indrsinc(rd, rs); break;
	case 0x2: sub_word_rd_indrsinc(rd, rs); break;
	case 0x3: subb_word_rd_indrsinc(rd, rs); break;
	case 0x4: cmp_word_rd_indrsinc(rd, rs); break;
	case 0x5: and_word_rd_indrsinc(rd, rs); break;
	case 0x6: or_word_rd_indrsinc(rd, rs); break;
	case 0x7: xor_word_rd_indrsinc(rd, rs); break;
	case 0x8: mov_word_rd_indrsinc(rd, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_rd_indrsinc(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rd_indrsinc(rd, rs); break;
	case 0x1: addc_byte_rd_indrsinc(rd, rs); break;
	case 0x2: sub_byte_rd_indrsinc(rd, rs); break;
	case 0x3: subb_byte_rd_indrsinc(rd, rs); break;
	case 0x4: cmp_byte_rd_indrsinc(rd, rs); break;
	case 0x5: and_byte_rd_indrsinc(rd, rs); break;
	case 0x6: or_byte_rd_indrsinc(rd, rs); break;
	case 0x7: xor_byte_rd_indrsinc(rd, rs); break;
	case 0x8: mov_byte_rd_indrsinc(rd, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_word_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void add_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_rd_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void aluop_word_indrdinc_rs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_indrdinc_rs(rd, rs); break;
	case 0x1: addc_word_indrdinc_rs(rd, rs); break;
	case 0x2: sub_word_indrdinc_rs(rd, rs); break;
	case 0x3: subb_word_indrdinc_rs(rd, rs); break;
	case 0x4: cmp_word_indrdinc_rs(rd, rs); break;
	case 0x5: and_word_indrdinc_rs(rd, rs); break;
	case 0x6: or_word_indrdinc_rs(rd, rs); break;
	case 0x7: xor_word_indrdinc_rs(rd, rs); break;
	case 0x8: mov_word_indrdinc_rs(rd, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_indrdinc_rs(int alu_op, u8 rd, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_indrdinc_rs(rd, rs); break;
	case 0x1: addc_byte_indrdinc_rs(rd, rs); break;
	case 0x2: sub_byte_indrdinc_rs(rd, rs); break;
	case 0x3: subb_byte_indrdinc_rs(rd, rs); break;
	case 0x4: cmp_byte_indrdinc_rs(rd, rs); break;
	case 0x5: and_byte_indrdinc_rs(rd, rs); break;
	case 0x6: or_byte_indrdinc_rs(rd, rs); break;
	case 0x7: xor_byte_indrdinc_rs(rd, rs); break;
	case 0x8: mov_byte_indrdinc_rs(rd, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_word_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void add_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_indrdinc_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_indrdinc_rs(u8 rd, u8 rs)
{ u16 address = get_addr(rd); u8 result = gr8(rs); wdat8(address, result); address++; do_nz_flags_8(result); set_addr(rd, address); cy(4); }

static void aluop_word_rd_rsoff8(int alu_op, u8 rd, u8 rs, u8 offset8)
{
	switch (alu_op)
	{
	case 0x0: add_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x1: addc_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x2: sub_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x3: subb_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x4: cmp_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x5: and_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x6: or_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x7: xor_word_rd_rsoff8(rd, rs, offset8); break;
	case 0x8: mov_word_rd_rsoff8(rd, rs, offset8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_rd_rsoff8(int alu_op, u8 rd, u8 rs, u8 offset8)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x1: addc_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x2: sub_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x3: subb_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x4: cmp_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x5: and_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x6: or_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x7: xor_byte_rd_rsoff8(rd, rs, offset8); break;
	case 0x8: mov_byte_rd_rsoff8(rd, rs, offset8); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ u16 rdval = gr16(rd); u16 fulloffset = ((s8)offset8); u16 address = get_addr(rs) + fulloffset; u16 val = rdat16(address); u16 result = do_add_16(rdval, val); sr16(rd, result); cy(6); }

static void addc_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_word_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rs) + fulloffset; u16 val = rdat16(address); do_nz_flags_16(val); sr16(rd, val); cy(5); }

static void add_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_rd_rsoff8(u8 rd, u8 rs, u8 offset8)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rs) + fulloffset; u8 val = rdat8(address); do_nz_flags_8(val); sr8(rd, val); cy(5); }

static void aluop_word_rdoff8_rs(int alu_op, u8 rd, u8 offset8, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x1: addc_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x2: sub_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x3: subb_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x4: cmp_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x5: and_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x6: or_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x7: xor_word_rdoff8_rs(rd, offset8, rs); break;
	case 0x8: mov_word_rdoff8_rs(rd, offset8, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_rdoff8_rs(int alu_op, u8 rd, u8 offset8, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x1: addc_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x2: sub_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x3: subb_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x4: cmp_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x5: and_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x6: or_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x7: xor_byte_rdoff8_rs(rd, offset8, rs); break;
	case 0x8: mov_byte_rdoff8_rs(rd, offset8, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); u16 result = do_add_16(val, rsval); wdat16(address, result); cy(6); }

static void addc_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); u16 result = do_addc_16(val, rsval); wdat16(address, result); cy(6); }

static void sub_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); u16 result = do_sub_16(val, rsval); wdat16(address, result); cy(6); }

static void subb_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); u16 result = do_subb_16(val, rsval); wdat16(address, result); cy(6); }

static void cmp_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); do_sub_16(val, rsval); cy(6); }

static void and_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); u16 result = do_and_16(val, rsval); wdat16(address, result); cy(6); }

static void or_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); u16 result = do_or_16(val, rsval); wdat16(address, result); cy(6); }

static void xor_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; u16 val = rdat16(address); u16 rsval = gr16(rs); u16 result = do_xor_16(val, rsval); wdat16(address, result); cy(6); }

static void mov_word_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u16 val = gr16(rs); u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; do_nz_flags_16(val); wdat16(address, val); cy(5); }

static void add_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_rdoff8_rs(u8 rd, u8 offset8, u8 rs)
{ u8 val = gr8(rs); u16 fulloffset = ((s8)offset8); u16 address = get_addr(rd) + fulloffset; do_nz_flags_8(val); wdat8(address, val); cy(5); }

static void aluop_word_rsoff16(int alu_op, u8 rd, u8 rs, u16 offset16)
{
	switch (alu_op)
	{
	case 0x0: add_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x1: addc_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x2: sub_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x3: subb_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x4: cmp_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x5: and_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x6: or_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x7: xor_word_rd_rsoff16(rd, rs, offset16); break;
	case 0x8: mov_word_rd_rsoff16(rd, rs, offset16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_rsoff16(int alu_op, u8 rd, u8 rs, u16 offset16)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x1: addc_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x2: sub_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x3: subb_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x4: cmp_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x5: and_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x6: or_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x7: xor_byte_rd_rsoff16(rd, rs, offset16); break;
	case 0x8: mov_byte_rd_rsoff16(rd, rs, offset16); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_word_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void add_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_rd_rsoff16(u8 rd, u8 rs, u16 offset16)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void aluop_word_rdoff16_rs(int alu_op, u8 rd, u16 offset16, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x1: addc_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x2: sub_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x3: subb_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x4: cmp_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x5: and_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x6: or_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x7: xor_word_rdoff16_rs(rd, offset16, rs); break;
	case 0x8: mov_word_rdoff16_rs(rd, offset16, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_rdoff16_rs(int alu_op, u8 rd, u16 offset16, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x1: addc_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x2: sub_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x3: subb_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x4: cmp_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x5: and_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x6: or_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x7: xor_byte_rdoff16_rs(rd, offset16, rs); break;
	case 0x8: mov_byte_rdoff16_rs(rd, offset16, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_word_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ u16 val = gr16(rs); u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; do_nz_flags_16(val); wdat16(address, val); cy(5); }

static void add_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void and_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_rdoff16_rs(u8 rd, u16 offset16, u8 rs)
{ u8 val = gr16(rs); u16 fulloffset = offset16; u16 address = get_addr(rd) + fulloffset; do_nz_flags_8(val); wdat8(address, val); cy(5); }

static void aluop_word_rd_direct(int alu_op, u8 rd, u16 direct)
{
	switch (alu_op)
	{
	case 0x0: add_word_rd_direct(rd, direct); break;
	case 0x1: addc_word_rd_direct(rd, direct); break;
	case 0x2: sub_word_rd_direct(rd, direct); break;
	case 0x3: subb_word_rd_direct(rd, direct); break;
	case 0x4: cmp_word_rd_direct(rd, direct); break;
	case 0x5: and_word_rd_direct(rd, direct); break;
	case 0x6: or_word_rd_direct(rd, direct); break;
	case 0x7: xor_word_rd_direct(rd, direct); break;
	case 0x8: mov_word_rd_direct(rd, direct); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_rd_direct(int alu_op, u8 rd, u16 direct)
{
	switch (alu_op)
	{
	case 0x0: add_byte_rd_direct(rd, direct); break;
	case 0x1: addc_byte_rd_direct(rd, direct); break;
	case 0x2: sub_byte_rd_direct(rd, direct); break;
	case 0x3: subb_byte_rd_direct(rd, direct); break;
	case 0x4: cmp_byte_rd_direct(rd, direct); break;
	case 0x5: and_byte_rd_direct(rd, direct); break;
	case 0x6: or_byte_rd_direct(rd, direct); break;
	case 0x7: xor_byte_rd_direct(rd, direct); break;
	case 0x8: mov_byte_rd_direct(rd, direct); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); u16 result = do_add_16(rdval, val); sr16(rd, result); cy(4); }

static void addc_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); u16 result = do_addc_16(rdval, val); sr16(rd, result); cy(4);}

static void sub_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); u16 result = do_sub_16(rdval, val); sr16(rd, result); cy(4); }

static void subb_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); u16 result = do_subb_16(rdval, val); sr16(rd, result); cy(4); }

static void cmp_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); do_sub_16(rdval, val); cy(4); }

static void and_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); u16 result = do_and_16(rdval, val); sr16(rd, result); cy(4); }

static void or_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); u16 result = do_or_16(rdval, val); sr16(rd, result); cy(4); }

static void xor_word_rd_direct(u8 rd, u16 direct)
{ u16 rdval = gr16(rd); u16 val = read_direct16(direct); u16 result = do_xor_16(rdval, val); sr16(rd, result); cy(4); }

static void mov_word_rd_direct(u8 rd, u16 direct)
{ u16 val = read_direct16(direct); do_nz_flags_16(val); sr16(rd, val); cy(4);}

static void add_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_rd_direct(u8 rd, u16 direct)
{ u8 rdval = gr8(rd); u8 val = read_direct8(direct); do_sub_8(rdval, val); cy(4); }

static void and_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_rd_direct(u8 rd, u16 direct)
{ u8 val = read_direct8(direct); do_nz_flags_8(val); sr8(rd, val); cy(4); }

static void aluop_word_direct_rs(int alu_op, u16 direct, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_word_direct_rs(direct, rs); break;
	case 0x1: addc_word_direct_rs(direct, rs); break;
	case 0x2: sub_word_direct_rs(direct, rs); break;
	case 0x3: subb_word_direct_rs(direct, rs); break;
	case 0x4: cmp_word_direct_rs(direct, rs); break;
	case 0x5: and_word_direct_rs(direct, rs); break;
	case 0x6: or_word_direct_rs(direct, rs); break;
	case 0x7: xor_word_direct_rs(direct, rs); break;
	case 0x8: mov_word_direct_rs(direct, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void aluop_byte_direct_rs(int alu_op, u16 direct, u8 rs)
{
	switch (alu_op)
	{
	case 0x0: add_byte_direct_rs(direct, rs); break;
	case 0x1: addc_byte_direct_rs(direct, rs); break;
	case 0x2: sub_byte_direct_rs(direct, rs); break;
	case 0x3: subb_byte_direct_rs(direct, rs); break;
	case 0x4: cmp_byte_direct_rs(direct, rs); break;
	case 0x5: and_byte_direct_rs(direct, rs); break;
	case 0x6: or_byte_direct_rs(direct, rs); break;
	case 0x7: xor_byte_direct_rs(direct, rs); break;
	case 0x8: mov_byte_direct_rs(direct, rs); break;
	default: do { } while (0); do_nop(); break;
	}
}

static void add_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); u16 result = do_add_16(directval, val); write_direct16(direct, result); cy(4); }

static void addc_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); u16 result = do_addc_16(directval, val); write_direct16(direct, result); cy(4); }

static void sub_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); u16 result = do_sub_16(directval, val); write_direct16(direct, result); cy(4); }

static void subb_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); u16 result = do_subb_16(directval, val); write_direct16(direct, result); cy(4) ; }

static void cmp_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); do_sub_16(directval, val); cy(4); }

static void and_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); u16 result = do_and_16(directval, val); write_direct16(direct, result); cy(4); }

static void or_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); u16 result = do_or_16(directval, val); write_direct16(direct, result); cy(4); }

static void xor_word_direct_rs(u16 direct, u8 rs)
{ u16 directval = read_direct16(direct); u16 val = gr16(rs); u16 result = do_xor_16(directval, val); write_direct16(direct, result); cy(4); }

static void mov_word_direct_rs(u16 direct, u8 rs)
{ u16 val = gr16(rs); do_nz_flags_16(val); write_direct16(direct, val); cy(4); }

static void add_byte_direct_rs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void addc_byte_direct_rs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void sub_byte_direct_rs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void subb_byte_direct_rs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cmp_byte_direct_rs(u16 direct, u8 rs)
{ u8 directval = read_direct8(direct); u8 val = gr8(rs); do_sub_8(directval, val); cy(4); }

static void and_byte_direct_rs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void or_byte_direct_rs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void xor_byte_direct_rs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void mov_byte_direct_rs(u16 direct, u8 rs)
{ u8 val = gr8(rs); do_nz_flags_8(val); write_direct8(direct, val); cy(4); }

static void movs_word_rd_data4(u8 rd, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); sr16(rd, data); do_nz_flags_16(data); cy(3); }

static void movs_byte_rd_data4(u8 rd, u8 data4)
{ u8 data  = ((s16)((data4 & 15) << 12) >> 12); sr8(rd, data);  do_nz_flags_8(data);  cy(3); }

static void adds_word_rd_data4(u8 rd, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); u16 regval = gr16(rd); regval += data; do_nz_flags_16(regval); sr16(rd, regval); cy(3); }

static void adds_byte_rd_data4(u8 rd, u8 data4)
{ u8 data  = ((s16)((data4 & 15) << 12) >> 12); u8 regval  = gr8(rd);  regval += data; do_nz_flags_8(regval);  sr8(rd, regval); cy(3); }

static void movs_word_indrd_data4(u8 rd, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); u16 address = get_addr(rd); wdat16(address, data); do_nz_flags_16(data); cy(3); }

static void movs_byte_indrd_data4(u8 rd, u8 data4)
{ u8 data = ((s16)((data4 & 15) << 12) >> 12); u16 address = get_addr(rd); wdat8(address, data); do_nz_flags_8(data); cy(3); }

static void adds_word_indrd_data4(u8 rd, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); u16 address = get_addr(rd); u16 rdval = rdat16(address); u16 result = do_add_16(rdval, data); wdat16(address, result); cy(4); }

static void adds_byte_indrd_data4(u8 rd, u8 data4)
{ u8 data = ((s16)((data4 & 15) << 12) >> 12); u16 address = get_addr(rd); u8 rdval = rdat8(address); u8 result = do_add_8(rdval, data); wdat8(address, result); cy(4); }

static void movs_word_indrdinc_data4(u8 rd, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); u16 address = get_addr(rd); wdat16(address, data); address += 2; do_nz_flags_16(data); set_addr(rd, address); cy(4); }

static void movs_byte_indrdinc_data4(u8 rd, u8 data4)
{ u8 data  = ((s16)((data4 & 15) << 12) >> 12); u16 address = get_addr(rd); wdat8(address, data);  address++;    do_nz_flags_8(data);  set_addr(rd, address); cy(4); }

static void adds_word_indrdinc_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void adds_byte_indrdinc_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movs_word_indrdoff8_data4(u8 rd, u8 off8, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); u16 fulloffset = ((s8)off8); u16 address = get_addr(rd) + fulloffset; wdat16(address, data); do_nz_flags_16(data); cy(5); }

static void movs_byte_indrdoff8_data4(u8 rd, u8 off8, u8 data4)
{ u8 data = ((s16)((data4 & 15) << 12) >> 12); u16 fulloffset = ((s8)off8); u16 address = get_addr(rd) + fulloffset; wdat8(address, data); do_nz_flags_8(data); cy(5);  }

static void adds_word_indrdoff8_data4(u8 rd, u8 off8, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); u16 fulloffset = ((s8)off8); u16 address = get_addr(rd) + fulloffset; u16 rdval = rdat16(address); u16 result = do_add_16(rdval, data); wdat16(address, result); cy(4); }

static void adds_byte_indrdoff8_data4(u8 rd, u8 off8, u8 data4)
{ u8 data = ((s16)((data4 & 15) << 12) >> 12); u16 fulloffset = ((s8)off8); u16 address = get_addr(rd) + fulloffset; u8 rdval = rdat8(address); u8 result = do_add_8(rdval, data); wdat8(address, result); cy(4); }

static void movs_word_indrdoff16_data4(u8 rd, u16 off16, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movs_byte_indrdoff16_data4(u8 rd, u16 off16, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void adds_word_indrdoff16_data4(u8 rd, u16 off16, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void adds_byte_indrdoff16_data4(u8 rd, u16 off16, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movs_word_direct_data4(u16 direct, u8 data4)
{ u16 data = ((s16)((data4 & 15) << 12) >> 12); do_nz_flags_16(data); write_direct16(direct, data); cy(3); }

static void movs_byte_direct_data4(u16 direct, u8 data4)
{ u8  data = ((s16)((data4 & 15) << 12) >> 12); do_nz_flags_8(data);  write_direct8(direct, data); cy(3); }

static void adds_word_direct_data4(u16 direct, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void adds_byte_direct_data4(u16 direct, u8 data4)
{ u8 directval = read_direct8(direct); u8 data = ((s16)((data4 & 15) << 12) >> 12); directval += data; do_nz_flags_8(directval); write_direct8(direct, directval); cy(4); }

static void call_rel16(u16 rel16)
{ if (m_pagezeromode) { push_word_to_stack(m_pc); set_pc_in_current_page(expand_rel16(rel16)); cy(4); } else { cy(7); fatalerror("Unsupported XA operation at %04X", m_pc); } }

static void bcc_rel8(u8 rel8)
{ if (get_c_flag() == 0) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bcs_rel8(u8 rel8)
{ if (get_c_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bne_rel8(u8 rel8)
{ if (get_z_flag() == 0) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void beq_rel8(u8 rel8)
{ if (get_z_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bnv_rel8(u8 rel8)
{ if (!get_v_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bov_rel8(u8 rel8)
{ if (get_v_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bpl_rel8(u8 rel8)
{ if (!get_n_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bmi_rel8(u8 rel8)
{ if (get_n_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bg_rel8(u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void bl_rel8(u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void bge_rel8(u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void blt_rel8(u8 rel8)
{ if (get_n_flag() ^ get_v_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void bgt_rel8(u8 rel8)
{ if ((get_z_flag() | get_n_flag()) ^ get_v_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(6); } else { cy(3); } }

static void ble_rel8(u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void br_rel8(u8 rel8)
{ set_pc_in_current_page(expand_rel8(rel8)); cy(6); }

static void asl_byte_rd_imm4(u8 rd, u8 amount)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void asl_word_rd_imm4(u8 rd, u8 amount)
{ u16 fullreg = gr16(rd); fullreg = asl32_helper(fullreg, amount); sr16(rd, fullreg); }

static void asl_dword_rd_imm5(u8 rd, u8 amount)
{ u32 fullreg = gr32(rd); fullreg = asl32_helper(fullreg, amount); sr32(rd, fullreg); }

static void asr_byte_rd_imm4(u8 rd, u8 amount)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void asr_word_rd_imm4(u8 rd, u8 amount)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void asr_dword_rd_imm5(u8 rd, u8 amount)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void lsr_byte_rd_imm4(u8 rd, u8 amount)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void lsr_word_rd_imm4(u8 rd, u8 amount)
{ u16 fullreg = gr16(rd); fullreg = lsr32_helper(fullreg, amount); sr16(rd, fullreg); }

static void lsr_dword_rd_imm5(u8 rd, u8 amount)
{ u32 fullreg = gr32(rd); fullreg = lsr32_helper(fullreg, amount); sr32(rd, fullreg); }

static void asl_byte_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void asl_word_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void asl_dword_rd_rs(u8 rd, u8 rs)
{ u32 fullreg = gr32(rd); u8 amount = gr8(rs); fullreg = lsr32_helper(fullreg, amount); sr32(rd, fullreg); cy(7); }

static void asr_byte_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void asr_word_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void asr_dword_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void lsr_byte_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void lsr_word_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void lsr_dword_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void norm_byte_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void norm_word_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void norm_dword_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mulu_byte_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void divu_byte_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mulu_word_rd_rs(u8 rd, u8 rs)
{
	if (rd & 1)
		fatalerror("Unsupported XA operation at %04X", m_pc);

	u16 fullval = gr16(rd);
	u16 rsval = gr16(rs);

	u32 result = (u32)fullval * rsval;

	if (result & 0xffff0000)
		set_v_flag();
	else
		clear_v_flag();

	if (result == 0x0000)
		set_z_flag();
	else
		clear_z_flag();

	if (result & 0x8000)
		set_n_flag();
	else
		clear_n_flag();

	sr16(rd, result & 0xffff);
	sr16(rd + 1, result >> 16);

	clear_c_flag();

	cy(12);
}

static void divu_word_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mul_word_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void div_word_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void div_word_rd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void divu_byte_rd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void divu_word_rd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mulu_byte_rd_data8(u8 rd, u8 data8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mulu_word_rd_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void divu_dword_rd_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mul_word_rd_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void div_dword_rd_data16(u8 rd, u16 data16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void divu_dword_rd_rs(u8 rd, u8 rs)
{
	u32 fullval = gr32(rd);
	u16 rsval = gr16(rs);

	if (rsval)
	{
		u32 result = fullval / rsval;
		u32 remainder = fullval % rsval;

		if (result & 0xffff0000)
			set_v_flag();
		else
			clear_v_flag();

		result &= 0xffff;

		if (result == 0x0000)
			set_z_flag();
		else
			clear_z_flag();

		if (result & 0x8000)
			set_n_flag();
		else
			clear_n_flag();

		sr16(rd, result);
		sr16(rd + 1, remainder);
	}
	else
	{
		set_v_flag();
		do { } while (0);
	}

	clear_c_flag();

	cy(22);
}

static void div_dword_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void clr_bit(u16 bit)
{ set_bit_8_helper(bit, 0); cy(4); }

static void setb_bit(u16 bit)
{ set_bit_8_helper(bit, 1); cy(4); }

static void mov_c_bit(u16 bit)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_bit_c(u16 bit)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void anl_c_bit(u16 bit)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void anl_c_notbit(u16 bit)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void orl_c_bit(u16 bit)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void orl_c_notbit(u16 bit)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void lea_word_rd_rs_off8(u8 rd, u8 rs, u8 offs8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void lea_word_rd_rs_off16(u8 rd, u8 rs, u16 offs16)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void xch_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void xch_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void xch_word_rd_rs(u8 rd, u8 rs)
{ u16 rdval = gr16(rd); u16 rsval = gr16(rs); sr16(rd, rsval); sr16(rs, rdval); cy(5); }

static void xch_byte_rd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movc_word_rd_indrsinc(u8 rd, u8 rs)
{ u16 address = get_addr(rs); u16 data = xa_program_read_word(address); address+=2; set_addr(rs, address); do_nz_flags_16(data); sr16(rd, data); cy(4); }

static void movc_byte_rd_indrsinc(u8 rd, u8 rs)
{ u16 address = get_addr(rs); u8 data = xa_program_read_byte(address); address++; set_addr(rs, address); do_nz_flags_8(data); sr8(rd, data); cy(4); }

static void djnz_word_rd_rel8(u8 rd, u8 rel8)
{ u16 regval = gr16(rd); regval--; do_nz_flags_16(regval); sr16(rd, regval); if (get_z_flag() == 0) { set_pc_in_current_page(expand_rel8(rel8)); cy(8); } else { cy(5); } }

static void djnz_byte_rd_rel8(u8 rd, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void popu_word_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void popu_byte_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void pop_word_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void pop_byte_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void pushu_word_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void pushu_byte_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void push_word_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void push_byte_direct(u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_word_indrdinc_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_byte_indrdinc_indrsinc(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void da_rd(u8 rd)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void sext_word_rd(u8 rd)
{ if(m_nflag) sr16(rd, 0xffff); else sr16(rd, 0); cy(2); }

static void sext_byte_rd(u8 rd)
{ if(m_nflag) sr8(rd, 0xff); else sr8(rd, 0); cy(2);  }

static void cpl_word_rd(u8 rd)
{ u16 rdval = gr16(rd); u16 result = rdval ^ 0xffff; do_nz_flags_16(result); sr16(rd, result); cy(3); }

static void cpl_byte_rd(u8 rd)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void neg_word_rd(u8 rd)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void neg_byte_rd(u8 rd)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movc_a_apc(void)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movc_a_adptr(void)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_rd_usp(u8 rd)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_usp_rs(u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jb_bit_rel8(u16 bit, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jnb_bit_rel8(u16 bit, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jbc_bit_rel8(u16 bit, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_word_direct_direct(u16 direct_d, u16 direct_s)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_byte_direct_direct(u16 direct_d, u16 direct_s)
{ u8 val = read_direct8(direct_s); do_nz_flags_8(val); write_direct8(direct_d, val); cy(4); }

static void xch_word_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void xch_byte_rd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_word_direct_indrs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_byte_direct_indrs(u16 direct, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_word_indrd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void mov_byte_indrd_direct(u8 rd, u16 direct)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movx_word_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movx_byte_indrd_rs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movx_word_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void movx_byte_rd_indrs(u8 rd, u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rr_word_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rr_byte_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rrc_word_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rrc_byte_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rl_word_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rl_byte_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rlc_word_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void rlc_byte_rd_data4(u8 rd, u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void fcall_addr24(u32 addr24)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void call_indrs(u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void fjmp_addr24(u32 addr24)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jmp_rel16(u16 rel16)
{ m_pc = expand_rel16(rel16); cy(6); }

static void djnz_word_direct_rel8(u16 direct, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void djnz_byte_direct_rel8(u16 direct, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void cjne_word_rd_direct_rel8(u8 rd, u16 direct, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void cjne_byte_rd_direct_rel8(u8 rd, u16 direct, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void cjne_indrd_data8_rel8(u8 rd, u8 data8, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cjne_rd_data8_rel8(u8 rd, u8 data8, u8 rel8)
{ u8 regval = gr8(rd); do_cjne_8_helper(regval, data8); if (!get_z_flag()) { set_pc_in_current_page(expand_rel8(rel8)); cy(9); } else { cy(6); } }

static void cjne_indrd_data16_rel8(u8 rd, u16 data16, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc);}

static void cjne_rd_data16_rel8(u8 rd, u16 data16, u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void reset(void)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void trap_data4(u8 data4)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jmp_ind_adptr(void)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jmp_dblindrs(u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jmp_indrs(u8 rs)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void ret(void)
{
	if (m_pagezeromode)
	{
		u16 addr = pull_word_from_stack();
		set_pc_in_current_page(addr);
		cy(6);
	}
	else
	{
		cy(8);
		fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static void reti(void)
{
	if (m_pagezeromode)
	{

		m_pc = pull_word_from_system_stack();
		m_PSWL = pull_word_from_system_stack();
		m_PSWH = pull_word_from_system_stack();
		sfr_PSWL_w(m_PSWL);
		sfr_PSWH_w(m_PSWH);
		cy(8);
		m_in_interrupt = 0;

	}
	else
	{
		fatalerror("Unsupported XA operation at %04X", m_pc);
	}
}

static void jz_rel8(u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void jnz_rel8(u8 rel8)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void push_word_rlist(u8 bitfield, int h)
{ cy(2); push_word_reglist(bitfield, h, false); }

static void push_byte_rlist(u8 bitfield, int h)
{ cy(2); push_byte_reglist(bitfield, h, false); }

static void pushu_word_rlist(u8 bitfield, int h)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void pushu_byte_rlist(u8 bitfield, int h)
{ push_byte_reglist(bitfield, h, true); }

static void pop_word_rlist(u8 bitfield, int h)
{ cy(2); pull_word_reglist(bitfield, h, false); }

static void pop_byte_rlist(u8 bitfield, int h)
{ cy(2); pull_byte_reglist(bitfield, h, false); }

static void popu_word_rlist(u8 bitfield, int h)
{ fatalerror("Unsupported XA operation at %04X", m_pc); }

static void popu_byte_rlist(u8 bitfield, int h)
{ cy(2); pull_byte_reglist(bitfield, h, true); }

static u8 xa_sfr_read_byte(u32 addr)
{
	if (addr >= 0x30 && addr <= 0x33) return sfr_port_r(addr - 0x30);
	if (addr >= 0x70 && addr <= 0x73) return sfr_PxCFGA_r(addr - 0x70);
	if (addr >= 0xf0 && addr <= 0xf3) return sfr_PxCFGB_r(addr - 0xf0);
	switch (addr) {
	case 0: return sfr_PSWL_r();
	case 1: return sfr_PSWH_r();
	case 0x1f: return sfr_WDCON_r();
	case 0x26: return sfr_IEL_r();
	}
	return 0xff;
}
static void xa_sfr_write_byte(u32 addr, u8 data)
{
	if (addr >= 0x30 && addr <= 0x33) { sfr_port_w(addr - 0x30, data); return; }
	if (addr >= 0x70 && addr <= 0x73) { sfr_PxCFGA_w(addr - 0x70, data); return; }
	if (addr >= 0xf0 && addr <= 0xf3) { sfr_PxCFGB_w(addr - 0xf0, data); return; }
	switch (addr) {
	case 0: sfr_PSWL_w(data); break;
	case 1: sfr_PSWH_w(data); break;
	case 2: sfr_PSW51_w(data); break;
	case 0x26: sfr_IEL_w(data); break;
	case 0x40: sfr_SCR_w(data); break;
	case 0x5d: sfr_WFEED1_w(data); break;
	case 0x5e: sfr_WFEED2_w(data); break;
	}
}
static void xa_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	memset(&xa, 0, sizeof(xa));
	xa.config = config;
	xa.irq_callback = irqcallback;
}
static void xa_reset(void) { device_reset(); }
static int xa_execute(int cycles)
{
	m_icount = cycles;
	while (m_icount > 0) {
		u8 op;
		int old_cycles = m_icount;
		check_interrupts();
		CALL_MAME_DEBUG;
		op = xa_program_read_byte(m_pc++);
		s_instruction[op](op);
		if (m_icount == old_cycles) fatalerror("Unsupported XA operation at %04X", m_pc);
	}
	return cycles - m_icount;
}
static void xa_get_context(void *dst) { if (dst) memcpy(dst, &xa, sizeof(xa)); }
static void xa_set_context(void *src) { if (src) memcpy(&xa, src, sizeof(xa)); }
static void xa_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_INPUT_STATE && state < CPUINFO_INT_INPUT_STATE + 4) {
		if (info->i) m_irq_pending |= 1 << (state - CPUINFO_INT_INPUT_STATE);
		check_interrupts();
	} else if (state == CPUINFO_INT_PC) m_pc = info->i;
}
static offs_t xa_dasm(char *buffer, offs_t pc) { sprintf(buffer, "$%02X", xa_program_read_byte(pc)); return 1; }
void xa_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state) {
	case CPUINFO_INT_CONTEXT_SIZE: info->i = sizeof(xa); break;
	case CPUINFO_INT_INPUT_LINES: info->i = 4; break;
	case CPUINFO_INT_ENDIANNESS: info->i = CPU_IS_LE; break;
	case CPUINFO_INT_CLOCK_DIVIDER: info->i = 1; break;
	case CPUINFO_INT_MIN_INSTRUCTION_BYTES: info->i = 1; break;
	case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 6; break;
	case CPUINFO_INT_MIN_CYCLES: info->i = 1; break;
	case CPUINFO_INT_MAX_CYCLES: info->i = 24; break;
	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16; break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16; break;
	case CPUINFO_INT_PC: info->i = m_pc; break;
	case CPUINFO_INT_SP: info->i = m_usermode ? m_USP : m_SSP; break;
	case CPUINFO_PTR_SET_INFO: info->setinfo = xa_set_info; break;
	case CPUINFO_PTR_GET_CONTEXT: info->getcontext = xa_get_context; break;
	case CPUINFO_PTR_SET_CONTEXT: info->setcontext = xa_set_context; break;
	case CPUINFO_PTR_INIT: info->init = xa_init; break;
	case CPUINFO_PTR_RESET: info->reset = xa_reset; break;
	case CPUINFO_PTR_EXECUTE: info->execute = xa_execute; break;
	case CPUINFO_PTR_DISASSEMBLE: info->disassemble = xa_dasm; break;
	case CPUINFO_PTR_INSTRUCTION_COUNTER: info->icount = &m_icount; break;
	case CPUINFO_STR_NAME: strcpy(info->s = cpuintrf_temp_str(), "MX10EXA"); break;
	case CPUINFO_STR_CORE_FAMILY: strcpy(info->s = cpuintrf_temp_str(), "Philips XA"); break;
	case CPUINFO_STR_CORE_VERSION: strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
	case CPUINFO_STR_CORE_FILE: strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
	case CPUINFO_STR_CORE_CREDITS: strcpy(info->s = cpuintrf_temp_str(), "David Haywood"); break;
	}
}

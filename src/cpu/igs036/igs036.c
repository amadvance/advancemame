// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
/* Converted from MAME 0.287 to the AdvanceMAME C CPU interface. */
#include "driver.h"
#include "debugger.h"
#include <stdint.h>
#include <stdbool.h>
#include "igs036.h"
#include "igs036core.h"
#include "igs036help.h"
#define LOGMASKED(...) do { } while (0)
typedef void (*arm7ops_ophandler)(uint32_t);
typedef void (*arm7thumb_ophandler)(uint32_t, uint32_t);
static struct {
	uint32_t r[NUM_REGS], control, vectorbase, previouspc;
	int pendingIrq, pendingFiq, pendingAbtD, pendingAbtP, pendingUnd, pendingSwi, pending_interrupt;
	int (*irq_callback)(int);
	uint32_t cp15_control, cp15_itcm_reg, cp15_dtcm_reg;
	uint32_t cp15_itcm_base, cp15_itcm_size, cp15_itcm_end;
	uint32_t cp15_dtcm_base, cp15_dtcm_size, cp15_dtcm_end;
	uint8_t ITCM[0x8000], DTCM[0x4000];
} cpu;
static int m_icount;
#define m_archRev 5
#define m_archFlags (ARCHFLAG_T | ARCHFLAG_E)
#define m_r cpu.r
#define m_control cpu.control
#define m_vectorbase cpu.vectorbase
#define m_pendingIrq cpu.pendingIrq
#define m_pendingFiq cpu.pendingFiq
#define m_pendingAbtD cpu.pendingAbtD
#define m_pendingAbtP cpu.pendingAbtP
#define m_pendingUnd cpu.pendingUnd
#define m_pendingSwi cpu.pendingSwi
#define m_pending_interrupt cpu.pending_interrupt
#define cp15_control cpu.cp15_control
#define cp15_itcm_reg cpu.cp15_itcm_reg
#define cp15_dtcm_reg cpu.cp15_dtcm_reg
#define cp15_itcm_base cpu.cp15_itcm_base
#define cp15_itcm_size cpu.cp15_itcm_size
#define cp15_itcm_end cpu.cp15_itcm_end
#define cp15_dtcm_base cpu.cp15_dtcm_base
#define cp15_dtcm_size cpu.cp15_dtcm_size
#define cp15_dtcm_end cpu.cp15_dtcm_end
#define ITCM cpu.ITCM
#define DTCM cpu.DTCM
static uint32_t rotr_32(uint32_t v, int n) { n &= 31; return n ? (v >> n) | (v << (32-n)) : v; }
static int32_t sign_extend(uint32_t v, int bits) { return (int32_t)(v << (32-bits)) >> (32-bits); }
static int count_leading_zeros_32(uint32_t v) { return v ? __builtin_clz(v) : 32; }
static int population_count_32(uint32_t v) { return __builtin_popcount(v); }
static int64_t mul_32x32(int32_t a, int32_t b) { return (int64_t)a * b; }
static uint64_t mulu_32x32(uint32_t a, uint32_t b) { return (uint64_t)a * b; }
static int32_t mul_32x32_shift(int32_t a, int32_t b, int shift) { return ((int64_t)a * b) >> shift; }
static void set_cpsr(uint32_t v) { m_r[eCPSR] = v | 0x10; }
static void irq_callback(int line) { if (cpu.irq_callback) cpu.irq_callback(line); }
static void RefreshITCM(void);
static void RefreshDTCM(void);
static void update_irq_state(void);
static void arm7_do_callback(uint32_t insn) { cpu.pendingUnd = 1; update_irq_state(); }
static void arm7_dt_r_callback(uint32_t insn, uint32_t *prn) { cpu.pendingUnd = 1; update_irq_state(); }
static void arm7_dt_w_callback(uint32_t insn, uint32_t *prn) { cpu.pendingUnd = 1; update_irq_state(); }
static int64_t saturate_qbit_overflow(int64_t res);
static void SwitchMode(uint32_t cpsr_mode_val);
static uint32_t decodeShift(uint32_t insn, uint32_t *pCarry);
static int loadInc(uint32_t pat, uint32_t rbv, uint32_t s, int mode);
static int loadDec(uint32_t pat, uint32_t rbv, uint32_t s, int mode);
static int storeInc(uint32_t pat, uint32_t rbv, int mode);
static int storeDec(uint32_t pat, uint32_t rbv, int mode);
static void HandleCoProcDO(uint32_t insn);
static void HandleCoProcRT(uint32_t insn);
static void HandleCoProcDT(uint32_t insn);
static void HandleBranch(uint32_t insn, bool h_bit);
static void HandleMemSingle(uint32_t insn);
static void HandleHalfWordDT(uint32_t insn);
static void HandleSwap(uint32_t insn);
static void HandlePSRTransfer(uint32_t insn);
static void HandleALU(uint32_t insn);
static void HandleMul(uint32_t insn);
static void HandleSMulLong(uint32_t insn);
static void HandleUMulLong(uint32_t insn);
static void HandleMemBlock(uint32_t insn);
static void arm9ops_undef(uint32_t insn);
static void arm9ops_1(uint32_t insn);
static void arm9ops_57(uint32_t insn);
static void arm9ops_89(uint32_t insn);
static void arm9ops_ab(uint32_t insn);
static void arm9ops_c(uint32_t insn);
static void arm9ops_e(uint32_t insn);
static void arm7ops_0123(uint32_t insn);
static void arm7ops_4567(uint32_t insn);
static void arm7ops_89(uint32_t insn);
static void arm7ops_ab(uint32_t insn);
static void arm7ops_cd(uint32_t insn);
static void arm7ops_e(uint32_t insn);
static void arm7ops_f(uint32_t insn);
static void tg00_0(uint32_t pc, uint32_t op);
static void tg00_1(uint32_t pc, uint32_t op);
static void tg01_0(uint32_t pc, uint32_t op);
static void tg01_10(uint32_t pc, uint32_t op);
static void tg01_11(uint32_t pc, uint32_t op);
static void tg01_12(uint32_t pc, uint32_t op);
static void tg01_13(uint32_t pc, uint32_t op);
static void tg02_0(uint32_t pc, uint32_t op);
static void tg02_1(uint32_t pc, uint32_t op);
static void tg03_0(uint32_t pc, uint32_t op);
static void tg03_1(uint32_t pc, uint32_t op);
static void tg04_00_00(uint32_t pc, uint32_t op);
static void tg04_00_01(uint32_t pc, uint32_t op);
static void tg04_00_02(uint32_t pc, uint32_t op);
static void tg04_00_03(uint32_t pc, uint32_t op);
static void tg04_00_04(uint32_t pc, uint32_t op);
static void tg04_00_05(uint32_t pc, uint32_t op);
static void tg04_00_06(uint32_t pc, uint32_t op);
static void tg04_00_07(uint32_t pc, uint32_t op);
static void tg04_00_08(uint32_t pc, uint32_t op);
static void tg04_00_09(uint32_t pc, uint32_t op);
static void tg04_00_0a(uint32_t pc, uint32_t op);
static void tg04_00_0b(uint32_t pc, uint32_t op);
static void tg04_00_0c(uint32_t pc, uint32_t op);
static void tg04_00_0d(uint32_t pc, uint32_t op);
static void tg04_00_0e(uint32_t pc, uint32_t op);
static void tg04_00_0f(uint32_t pc, uint32_t op);
static void tg04_01_00(uint32_t pc, uint32_t op);
static void tg04_01_01(uint32_t pc, uint32_t op);
static void tg04_01_02(uint32_t pc, uint32_t op);
static void tg04_01_03(uint32_t pc, uint32_t op);
static void tg04_01_10(uint32_t pc, uint32_t op);
static void tg04_01_11(uint32_t pc, uint32_t op);
static void tg04_01_12(uint32_t pc, uint32_t op);
static void tg04_01_13(uint32_t pc, uint32_t op);
static void tg04_01_20(uint32_t pc, uint32_t op);
static void tg04_01_21(uint32_t pc, uint32_t op);
static void tg04_01_22(uint32_t pc, uint32_t op);
static void tg04_01_23(uint32_t pc, uint32_t op);
static void tg04_01_30(uint32_t pc, uint32_t op);
static void tg04_01_31(uint32_t pc, uint32_t op);
static void tg04_01_32(uint32_t pc, uint32_t op);
static void tg04_01_33(uint32_t pc, uint32_t op);
static void tg04_0203(uint32_t pc, uint32_t op);
static void tg05_0(uint32_t pc, uint32_t op);
static void tg05_1(uint32_t pc, uint32_t op);
static void tg05_2(uint32_t pc, uint32_t op);
static void tg05_3(uint32_t pc, uint32_t op);
static void tg05_4(uint32_t pc, uint32_t op);
static void tg05_5(uint32_t pc, uint32_t op);
static void tg05_6(uint32_t pc, uint32_t op);
static void tg05_7(uint32_t pc, uint32_t op);
static void tg06_0(uint32_t pc, uint32_t op);
static void tg06_1(uint32_t pc, uint32_t op);
static void tg07_0(uint32_t pc, uint32_t op);
static void tg07_1(uint32_t pc, uint32_t op);
static void tg08_0(uint32_t pc, uint32_t op);
static void tg08_1(uint32_t pc, uint32_t op);
static void tg09_0(uint32_t pc, uint32_t op);
static void tg09_1(uint32_t pc, uint32_t op);
static void tg0a_0(uint32_t pc, uint32_t op);
static void tg0a_1(uint32_t pc, uint32_t op);
static void tg0b_0(uint32_t pc, uint32_t op);
static void tg0b_1(uint32_t pc, uint32_t op);
static void tg0b_2(uint32_t pc, uint32_t op);
static void tg0b_3(uint32_t pc, uint32_t op);
static void tg0b_4(uint32_t pc, uint32_t op);
static void tg0b_5(uint32_t pc, uint32_t op);
static void tg0b_6(uint32_t pc, uint32_t op);
static void tg0b_7(uint32_t pc, uint32_t op);
static void tg0b_8(uint32_t pc, uint32_t op);
static void tg0b_9(uint32_t pc, uint32_t op);
static void tg0b_a(uint32_t pc, uint32_t op);
static void tg0b_b(uint32_t pc, uint32_t op);
static void tg0b_c(uint32_t pc, uint32_t op);
static void tg0b_d(uint32_t pc, uint32_t op);
static void tg0b_e(uint32_t pc, uint32_t op);
static void tg0b_f(uint32_t pc, uint32_t op);
static void tg0c_0(uint32_t pc, uint32_t op);
static void tg0c_1(uint32_t pc, uint32_t op);
static void tg0d_0(uint32_t pc, uint32_t op);
static void tg0d_1(uint32_t pc, uint32_t op);
static void tg0d_2(uint32_t pc, uint32_t op);
static void tg0d_3(uint32_t pc, uint32_t op);
static void tg0d_4(uint32_t pc, uint32_t op);
static void tg0d_5(uint32_t pc, uint32_t op);
static void tg0d_6(uint32_t pc, uint32_t op);
static void tg0d_7(uint32_t pc, uint32_t op);
static void tg0d_8(uint32_t pc, uint32_t op);
static void tg0d_9(uint32_t pc, uint32_t op);
static void tg0d_a(uint32_t pc, uint32_t op);
static void tg0d_b(uint32_t pc, uint32_t op);
static void tg0d_c(uint32_t pc, uint32_t op);
static void tg0d_d(uint32_t pc, uint32_t op);
static void tg0d_e(uint32_t pc, uint32_t op);
static void tg0d_f(uint32_t pc, uint32_t op);
static void tg0e_0(uint32_t pc, uint32_t op);
static void tg0e_1(uint32_t pc, uint32_t op);
static void tg0f_0(uint32_t pc, uint32_t op);
static void tg0f_1(uint32_t pc, uint32_t op);
static void update_irq_state();
static void arm7_check_irq_state();
static uint32_t arm7_rt_r_callback(offs_t offset)
{
	uint32_t opcode = offset;
	uint8_t cReg = ( opcode & INSN_COPRO_CREG ) >> INSN_COPRO_CREG_SHIFT;
	uint8_t op2 =  ( opcode & INSN_COPRO_OP2 )  >> INSN_COPRO_OP2_SHIFT;
	uint8_t op3 =    opcode & INSN_COPRO_OP3;
	uint8_t cpnum = (opcode & INSN_COPRO_CPNUM) >> INSN_COPRO_CPNUM_SHIFT;
	uint32_t data = 0;

	//printf("arm7946: read cpnum %d cReg %d op2 %d op3 %d (%x)\n", cpnum, cReg, op2, op3, opcode);
	if (cpnum == 15)
	{
		switch( cReg )
		{
			case 0:
				switch (op2)
				{
					case 0: // chip ID
						data = 0x41059461;
						break;

					case 1: // cache ID
						data = 0x0f0d2112;
						break;

					case 2: // TCM size
						data = (6 << 6) | (5 << 18);
						break;
				}
				break;

			case 1:
				return cp15_control;
				break;

			case 9:
				if (op3 == 1)
				{
					if (op2 == 0)
					{
						return cp15_dtcm_reg;
					}
					else
					{
						return cp15_itcm_reg;
					}
				}
				break;
		}
	}

	return data;
}

static void arm7_rt_w_callback(offs_t offset, uint32_t data)
{
	uint32_t opcode = offset;
	uint8_t cReg = ( opcode & INSN_COPRO_CREG ) >> INSN_COPRO_CREG_SHIFT;
	uint8_t op2 =  ( opcode & INSN_COPRO_OP2 )  >> INSN_COPRO_OP2_SHIFT;
	uint8_t op3 =    opcode & INSN_COPRO_OP3;
	uint8_t cpnum = (opcode & INSN_COPRO_CPNUM) >> INSN_COPRO_CPNUM_SHIFT;

//  printf("arm7946: copro %d write %x to cReg %d op2 %d op3 %d (mask %08x)\n", cpnum, data, cReg, op2, op3, mem_mask);

	if (cpnum == 15)
	{
		switch (cReg)
		{
			case 1: // control
				cp15_control = data;
				RefreshDTCM();
				RefreshITCM();
				break;

			case 2: // Protection Unit cacheability bits
				break;

			case 3: // write bufferability bits for PU
				break;

			case 5: // protection unit region controls
				break;

			case 6: // protection unit region controls 2
				break;

			case 7: // cache commands
				break;

			case 9: // cache lockdown & TCM controls
				if (op3 == 1)
				{
					if (op2 == 0)
					{
						cp15_dtcm_reg = data;
						RefreshDTCM();
					}
					else if (op2 == 1)
					{
						cp15_itcm_reg = data;
						RefreshITCM();
					}
				}
				break;
		}
	}
}

static void RefreshDTCM()
{
	if (cp15_control & (1U <<16))
	{
		cp15_dtcm_base = (cp15_dtcm_reg & ~0xfff);
		cp15_dtcm_size = 512 << ((cp15_dtcm_reg & 0x3f) >> 1);
		cp15_dtcm_end = cp15_dtcm_base + cp15_dtcm_size;
		//printf("DTCM enabled: base %08x size %x\n", cp15_dtcm_base, cp15_dtcm_size);
	}
	else
	{
		cp15_dtcm_base = 0xffffffff;
		cp15_dtcm_size = cp15_dtcm_end = 0;
	}
}

static void RefreshITCM()
{
	if (cp15_control & (1U <<18))
	{
		cp15_itcm_base = 0; //(cp15_itcm_reg & ~0xfff);
		cp15_itcm_size = 512 << ((cp15_itcm_reg & 0x3f) >> 1);
		cp15_itcm_end = cp15_itcm_base + cp15_itcm_size;
		//printf("ITCM enabled: base %08x size %x\n", cp15_dtcm_base, cp15_dtcm_size);
	}
	else
	{
		cp15_itcm_base = 0xffffffff;
		cp15_itcm_size = cp15_itcm_end = 0;
	}
}

/* TCM is byte-addressed and little endian, independent of host alignment. */
static uint8_t *tcm_pointer(offs_t addr)
{
	if (addr >= cp15_itcm_base && addr - cp15_itcm_base < cp15_itcm_size) return ITCM + (addr & 0x7fff);
	if (addr >= cp15_dtcm_base && addr - cp15_dtcm_base < cp15_dtcm_size) return DTCM + (addr & 0x3fff);
	return NULL;
}
static void arm7_cpu_write32(offs_t addr, uint32_t data)
{
	uint8_t *p;
	addr &= ~3U; p = tcm_pointer(addr);
	if (p) { p[0] = data; p[1] = data >> 8; p[2] = data >> 16; p[3] = data >> 24; }
	else program_write_dword_32le(addr, data);
}
static void arm7_cpu_write16(offs_t addr, uint16_t data)
{
	uint8_t *p;
	addr &= ~1U; p = tcm_pointer(addr);
	if (p) { p[0] = data; p[1] = data >> 8; }
	else program_write_word_32le(addr, data);
}
static void arm7_cpu_write8(offs_t addr, uint8_t data)
{
	uint8_t *p = tcm_pointer(addr);
	if (p) *p = data; else program_write_byte_32le(addr, data);
}
static uint32_t arm7_cpu_read32(offs_t addr)
{
	uint8_t *p = tcm_pointer(addr & ~3U);
	uint32_t v = p ? p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24) : program_read_dword_32le(addr & ~3U);
	return rotr_32(v, 8 * (addr & 3));
}
static uint32_t arm7_cpu_read16(offs_t addr)
{
	uint8_t *p;
	addr &= ~1U; p = tcm_pointer(addr);
	return p ? p[0] | (p[1] << 8) : program_read_word_32le(addr);
}
static uint8_t arm7_cpu_read8(offs_t addr)
{
	uint8_t *p = tcm_pointer(addr);
	return p ? *p : program_read_byte_32le(addr);
}

#include "igs036irq.c"
#include "igs036ops.c"
#include "igs036thumb.c"

static int igs036_execute(int cycles)
{
	uint32_t insn;
	m_icount = cycles;
	do {
		uint32_t pc = GET_PC;
		cpu.previouspc = pc;
		CALL_MAME_DEBUG;
		if (T_IS_SET(GET_CPSR)) {
			insn = arm7_cpu_read16(pc & ~1U);
			thumb_handler[(insn & 0xffc0) >> 6](pc, insn);
		} else {
			insn = arm7_cpu_read32(pc & ~3U);
			int op_offset = 0;
			/* process condition codes for this instruction */
			if ((insn >> INSN_COND_SHIFT) != COND_AL)
			{
				switch (insn >> INSN_COND_SHIFT)
				{
					case COND_EQ:
						if (Z_IS_CLEAR(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_NE:
						if (Z_IS_SET(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_CS:
						if (C_IS_CLEAR(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_CC:
						if (C_IS_SET(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_MI:
						if (N_IS_CLEAR(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_PL:
						if (N_IS_SET(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_VS:
						if (V_IS_CLEAR(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_VC:
						if (V_IS_SET(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_HI:
						if (C_IS_CLEAR(m_r[eCPSR]) || Z_IS_SET(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_LS:
						if (C_IS_SET(m_r[eCPSR]) && Z_IS_CLEAR(m_r[eCPSR]))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_GE:
						if (!(m_r[eCPSR] & N_MASK) != !(m_r[eCPSR] & V_MASK)) /* Use x ^ (x >> ...) method */
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_LT:
						if (!(m_r[eCPSR] & N_MASK) == !(m_r[eCPSR] & V_MASK))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_GT:
						if (Z_IS_SET(m_r[eCPSR]) || (!(m_r[eCPSR] & N_MASK) != !(m_r[eCPSR] & V_MASK)))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_LE:
						if (Z_IS_CLEAR(m_r[eCPSR]) && (!(m_r[eCPSR] & N_MASK) == !(m_r[eCPSR] & V_MASK)))
							{ { R15 += 4; m_icount += 2; }  goto skip_exec; }
						break;
					case COND_NV:
						if (m_archRev < 5)
						  { { R15 += 4; m_icount += 2; }  goto skip_exec; }
						else
							op_offset = 0x10;
						break;
				}
			}
			/*******************************************************************/
			/* If we got here - condition satisfied, so decode the instruction */
			/*******************************************************************/
			ops_handler[((insn & 0xF000000) >> 24) + op_offset](insn);
		}
skip_exec:
		arm7_check_irq_state();
		m_icount -= 3;
	} while (m_icount > 0);
	return cycles - m_icount;
}
static void igs036_reset(void)
{
	int (*cb)(int) = cpu.irq_callback;
	memset(&cpu, 0, sizeof(cpu));
	cpu.irq_callback = cb;
	set_cpsr(0xd3);
	cp15_control = 0x78;
	RefreshITCM(); RefreshDTCM();
}
static void igs036_postload(void) { RefreshITCM(); RefreshDTCM(); update_irq_state(); }
static void igs036_init(int index, int clock, const void *config, int (*cb)(int))
{
	cpu.irq_callback = cb;
	state_save_register_item_array("igs036", index, cpu.r);
	state_save_register_item_array("igs036", index, ITCM);
	state_save_register_item_array("igs036", index, DTCM);
	state_save_register_item("igs036", index, cpu.control);
	state_save_register_item("igs036", index, cpu.vectorbase);
	state_save_register_item("igs036", index, cpu.previouspc);
	state_save_register_item("igs036", index, cpu.pendingIrq);
	state_save_register_item("igs036", index, cpu.pendingFiq);
	state_save_register_item("igs036", index, cpu.pendingAbtD);
	state_save_register_item("igs036", index, cpu.pendingAbtP);
	state_save_register_item("igs036", index, cpu.pendingUnd);
	state_save_register_item("igs036", index, cpu.pendingSwi);
	state_save_register_item("igs036", index, cpu.pending_interrupt);
	state_save_register_item("igs036", index, cp15_control);
	state_save_register_item("igs036", index, cp15_itcm_reg);
	state_save_register_item("igs036", index, cp15_dtcm_reg);
	state_save_register_func_postload(igs036_postload);
}
static void igs036_get_context(void *dst) { if (dst) memcpy(dst, &cpu, sizeof(cpu)); }
static void igs036_set_context(void *src) { if (src) memcpy(&cpu, src, sizeof(cpu)); }
static void igs036_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_REGISTER + IGS036_R0 && state <= CPUINFO_INT_REGISTER + IGS036_R15) {
		SetRegister(state - CPUINFO_INT_REGISTER - IGS036_R0, info->i);
	} else switch (state) {
	case CPUINFO_INT_PC: case CPUINFO_INT_REGISTER + IGS036_PC: R15 = info->i; break;
	case CPUINFO_INT_SP: SetRegister(13, info->i); break;
	case CPUINFO_INT_REGISTER + IGS036_CPSR: set_cpsr(info->i); break;
	case CPUINFO_INT_INPUT_STATE + 0: cpu.pendingIrq = info->i != 0; break;
	case CPUINFO_INT_INPUT_STATE + 1: cpu.pendingFiq = info->i != 0; break;
	}
	update_irq_state();
	arm7_check_irq_state();
}
static offs_t igs036_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	extern void arm7_disasm(char *, unsigned int, unsigned int);
	extern void thumb_disasm(char *, unsigned int, unsigned short);
	if (T_IS_SET(GET_CPSR)) { thumb_disasm(buffer, pc, arm7_cpu_read16(pc)); return 2; }
	arm7_disasm(buffer, pc, arm7_cpu_read32(pc));
#else
	sprintf(buffer, "$%08x", arm7_cpu_read32(pc));
#endif
	return 4;
}
static UINT8 igs036_reg_layout[] = {
 IGS036_R0, IGS036_R1, IGS036_R2, IGS036_R3, IGS036_R4, IGS036_R5, IGS036_R6, IGS036_R7, (UINT8)-1,
 IGS036_R8, IGS036_R9, IGS036_R10, IGS036_R11, IGS036_R12, IGS036_R13, IGS036_R14, IGS036_R15, (UINT8)-1,
 IGS036_CPSR, 0
};
void igs036_get_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_REGISTER + IGS036_R0 && state <= CPUINFO_INT_REGISTER + IGS036_R15) {
		info->i = GetRegister(state - CPUINFO_INT_REGISTER - IGS036_R0); return;
	}
	if (state >= CPUINFO_STR_REGISTER + IGS036_R0 && state <= CPUINFO_STR_REGISTER + IGS036_R15) {
		int r = state - CPUINFO_STR_REGISTER - IGS036_R0;
		sprintf(info->s = cpuintrf_temp_str(), "R%-2d :%08x", r, GetRegister(r)); return;
	}
	switch (state) {
	case CPUINFO_INT_CONTEXT_SIZE: info->i = sizeof(cpu); break;
	case CPUINFO_INT_INPUT_STATE + 0: info->i = cpu.pendingIrq; break;
	case CPUINFO_INT_INPUT_STATE + 1: info->i = cpu.pendingFiq; break;
	case CPUINFO_INT_INPUT_LINES: info->i = 2; break;
	case CPUINFO_INT_ENDIANNESS: info->i = CPU_IS_LE; break;
	case CPUINFO_INT_CLOCK_DIVIDER: info->i = 1; break;
	case CPUINFO_INT_MIN_INSTRUCTION_BYTES: info->i = 2; break;
	case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 4; break;
	case CPUINFO_INT_MIN_CYCLES: info->i = 1; break;
	case CPUINFO_INT_MAX_CYCLES: info->i = 40; break;
	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32; break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32; break;
	case CPUINFO_INT_PC: case CPUINFO_INT_REGISTER + IGS036_PC: info->i = R15; break;
	case CPUINFO_INT_PREVIOUSPC: info->i = cpu.previouspc; break;
	case CPUINFO_INT_SP: info->i = GetRegister(13); break;
	case CPUINFO_INT_REGISTER + IGS036_CPSR: info->i = GET_CPSR; break;
	case CPUINFO_PTR_REGISTER_LAYOUT: info->p = igs036_reg_layout; break;
	case CPUINFO_PTR_SET_INFO: info->setinfo = igs036_set_info; break;
	case CPUINFO_PTR_GET_CONTEXT: info->getcontext = igs036_get_context; break;
	case CPUINFO_PTR_SET_CONTEXT: info->setcontext = igs036_set_context; break;
	case CPUINFO_PTR_INIT: info->init = igs036_init; break;
	case CPUINFO_PTR_RESET: info->reset = igs036_reset; break;
	case CPUINFO_PTR_EXECUTE: info->execute = igs036_execute; break;
	case CPUINFO_PTR_DISASSEMBLE: info->disassemble = igs036_dasm; break;
	case CPUINFO_PTR_INSTRUCTION_COUNTER: info->icount = &m_icount; break;
	case CPUINFO_STR_NAME: strcpy(info->s = cpuintrf_temp_str(), "IGS036"); break;
	case CPUINFO_STR_CORE_FAMILY: strcpy(info->s = cpuintrf_temp_str(), "ARM946E-S"); break;
	case CPUINFO_STR_CORE_VERSION: strcpy(info->s = cpuintrf_temp_str(), "MAME 0.287 C"); break;
	case CPUINFO_STR_CORE_FILE: strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
	case CPUINFO_STR_CORE_CREDITS: strcpy(info->s = cpuintrf_temp_str(), "Steve Ellenoff, R. Belmont, Ryan Holtz"); break;
	case CPUINFO_STR_FLAGS: sprintf(info->s = cpuintrf_temp_str(), "%08X %s", GET_CPSR, GetModeText(GET_CPSR)); break;
	case CPUINFO_STR_REGISTER + IGS036_PC: sprintf(info->s = cpuintrf_temp_str(), "PC  :%08X", R15); break;
	case CPUINFO_STR_REGISTER + IGS036_CPSR: sprintf(info->s = cpuintrf_temp_str(), "CPSR:%08X", GET_CPSR); break;
	}
}

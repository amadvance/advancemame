// license:BSD-3-Clause
// copyright-holders:Steve Ellenoff, R. Belmont, Ryan Holtz
/* ARM920T instruction execution and CP15 address translation. */
#include "driver.h"
#include "debugger.h"
#include <stdint.h>
#include <stdbool.h>
#include "arm920t.h"
#include "../igs036/igs036core.h"
#include "../igs036/igs036help.h"
#define LOGMASKED(...) do { } while (0)
typedef void (*arm7ops_ophandler)(uint32_t);
typedef void (*arm7thumb_ophandler)(uint32_t, uint32_t);
static struct {
	uint32_t r[NUM_REGS], control, vectorbase, previouspc;
	int pendingIrq, pendingFiq, pendingAbtD, pendingAbtP, pendingUnd, pendingSwi, pending_interrupt;
	int (*irq_callback)(int);
	uint32_t cp15_control, cp15_ttb, cp15_domain, cp15_pid;
	uint32_t tlb_tag[64], tlb_address[64];
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
static uint32_t rotr_32(uint32_t v, int n) { n &= 31; return n ? (v >> n) | (v << (32-n)) : v; }
static int32_t sign_extend(uint32_t v, int bits) { return (int32_t)(v << (32-bits)) >> (32-bits); }
static int count_leading_zeros_32(uint32_t v) { return v ? __builtin_clz(v) : 32; }
static int population_count_32(uint32_t v) { return __builtin_popcount(v); }
static int64_t mul_32x32(int32_t a, int32_t b) { return (int64_t)a * b; }
static uint64_t mulu_32x32(uint32_t a, uint32_t b) { return (uint64_t)a * b; }
static int32_t mul_32x32_shift(int32_t a, int32_t b, int shift) { return ((int64_t)a * b) >> shift; }
static void set_cpsr(uint32_t v) { m_r[eCPSR] = v | 0x10; }
static void irq_callback(int line) { if (cpu.irq_callback) cpu.irq_callback(line); }

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
static uint32_t arm7_rt_r_callback(offs_t opcode)
{
	switch ((opcode >> 16) & 15)
	{
	case 0: return ((opcode >> 5) & 7) ? 0x0d172172 : 0x41129200;
	case 1: return cp15_control | 0x70;
	case 2: return cpu.cp15_ttb;
	case 3: return cpu.cp15_domain;
	case 13: return cpu.cp15_pid;
	default: return 0;
	}
}

static void arm7_rt_w_callback(offs_t opcode, uint32_t data)
{
	/* Cached translations are valid only until CP15 changes the address space. */
	int reg = (opcode >> 16) & 15;
	if (reg == 1 || reg == 2 || reg == 3 || reg == 8 || reg == 13)
		memset(cpu.tlb_tag, 0, sizeof(cpu.tlb_tag));
	switch ((opcode >> 16) & 15)
	{
	case 1:
		cp15_control = data;
		m_vectorbase = (data & 0x2000) ? 0xffff0000 : 0;
		break;
	case 2: cpu.cp15_ttb = data & 0xffffc000; break;
	case 3: cpu.cp15_domain = data; break;
	case 13: cpu.cp15_pid = data & 0xfe000000; break;
	}
}

static offs_t translate_address_slow(offs_t address)
{
	uint32_t first, second;
	if (!(cp15_control & 1)) return address;
	if (address < 0x02000000) address |= cpu.cp15_pid;
	first = program_read_dword_32le(cpu.cp15_ttb | ((address >> 18) & 0x3ffc));
	if ((first & 3) == 2)
		return (first & 0xfff00000) | (address & 0xfffff);
	if ((first & 3) == 1)
		second = program_read_dword_32le((first & 0xfffffc00) | ((address >> 10) & 0x3fc));
	else if ((first & 3) == 3)
		second = program_read_dword_32le((first & 0xfffff000) | ((address >> 8) & 0xffc));
	else
		fatalerror("ARM920T unmapped section at %08x (PC %08x)", address, R15);
	switch (second & 3)
	{
	case 1: return (second & 0xffff0000) | (address & 0xffff);
	case 2: return (second & 0xfffff000) | (address & 0xfff);
	case 3: return (second & 0xfffffc00) | (address & 0x3ff);
	default: fatalerror("ARM920T unmapped page at %08x (PC %08x)", address, R15); return 0;
	}
}

static offs_t translate_address(offs_t address)
{
	UINT32 slot, tag, physical;
	if (!(cp15_control & 1)) return address;
	slot = (address >> 10) & 63;
	tag = (address & ~0x3ffU) | 1;
	if (cpu.tlb_tag[slot] == tag)
		return cpu.tlb_address[slot] | (address & 0x3ff);
	physical = translate_address_slow(address);
	cpu.tlb_tag[slot] = tag;
	cpu.tlb_address[slot] = physical & ~0x3ffU;
	return physical;
}

static int arm920t_translate(int space, offs_t *address)
{
	if (space == ADDRESS_SPACE_PROGRAM) *address = translate_address(*address);
	return 1;
}

static void arm7_cpu_write32(offs_t address, uint32_t data)
{
	program_write_dword_32le(translate_address(address & ~3U), data);
}
static void arm7_cpu_write16(offs_t address, uint16_t data)
{
	program_write_word_32le(translate_address(address & ~1U), data);
}
static void arm7_cpu_write8(offs_t address, uint8_t data)
{
	program_write_byte_32le(translate_address(address), data);
}
static uint32_t arm7_cpu_read32(offs_t address)
{
	uint32_t data = program_read_dword_32le(translate_address(address & ~3U));
	return rotr_32(data, (address & 3) * 8);
}
static uint32_t arm7_cpu_read16(offs_t address)
{
	return program_read_word_32le(translate_address(address & ~1U));
}
static uint8_t arm7_cpu_read8(offs_t address)
{
	return program_read_byte_32le(translate_address(address));
}

#include "../igs036/igs036irq.c"
#include "../igs036/igs036ops.c"
#include "../igs036/igs036thumb.c"

static int arm920t_execute(int cycles)
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
						if (!(m_r[eCPSR] & N_MASK) != !(m_r[eCPSR] & V_MASK))
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

			ops_handler[((insn & 0xF000000) >> 24) + op_offset](insn);
		}
skip_exec:
		arm7_check_irq_state();
		m_icount -= 3;
	} while (m_icount > 0);
	return cycles - m_icount;
}
static void arm920t_reset(void)
{
	int (*cb)(int) = cpu.irq_callback;
	memset(&cpu, 0, sizeof(cpu));
	cpu.irq_callback = cb;
	set_cpsr(0xd3);
	cp15_control = 0x78;

}
static void arm920t_postload(void) { memset(cpu.tlb_tag, 0, sizeof(cpu.tlb_tag)); update_irq_state(); }
static void arm920t_init(int index, int clock, const void *config, int (*cb)(int))
{
	cpu.irq_callback = cb;
	state_save_register_item_array("arm920t", index, cpu.r);
	state_save_register_item("arm920t", index, cpu.control);
	state_save_register_item("arm920t", index, cpu.vectorbase);
	state_save_register_item("arm920t", index, cpu.previouspc);
	state_save_register_item("arm920t", index, cpu.pendingIrq);
	state_save_register_item("arm920t", index, cpu.pendingFiq);
	state_save_register_item("arm920t", index, cpu.pendingAbtD);
	state_save_register_item("arm920t", index, cpu.pendingAbtP);
	state_save_register_item("arm920t", index, cpu.pendingUnd);
	state_save_register_item("arm920t", index, cpu.pendingSwi);
	state_save_register_item("arm920t", index, cpu.pending_interrupt);
	state_save_register_item("arm920t", index, cp15_control);
	state_save_register_item("arm920t", index, cpu.cp15_ttb);
	state_save_register_item("arm920t", index, cpu.cp15_domain);
	state_save_register_item("arm920t", index, cpu.cp15_pid);
	state_save_register_func_postload(arm920t_postload);
}
static void arm920t_get_context(void *dst) { if (dst) memcpy(dst, &cpu, sizeof(cpu)); }
static void arm920t_set_context(void *src) { if (src) memcpy(&cpu, src, sizeof(cpu)); }
static void arm920t_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_REGISTER + ARM920T_R0 && state <= CPUINFO_INT_REGISTER + ARM920T_R15) {
		SetRegister(state - CPUINFO_INT_REGISTER - ARM920T_R0, info->i);
	} else switch (state) {
	case CPUINFO_INT_PC: case CPUINFO_INT_REGISTER + ARM920T_PC: R15 = info->i; break;
	case CPUINFO_INT_SP: SetRegister(13, info->i); break;
	case CPUINFO_INT_REGISTER + ARM920T_CPSR: set_cpsr(info->i); break;
	case CPUINFO_INT_INPUT_STATE + 0: cpu.pendingIrq = info->i != 0; break;
	case CPUINFO_INT_INPUT_STATE + 1: cpu.pendingFiq = info->i != 0; break;
	}
	update_irq_state();
	arm7_check_irq_state();
}
static offs_t arm920t_dasm(char *buffer, offs_t pc)
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
static UINT8 arm920t_reg_layout[] = {
 ARM920T_R0, ARM920T_R1, ARM920T_R2, ARM920T_R3, ARM920T_R4, ARM920T_R5, ARM920T_R6, ARM920T_R7, (UINT8)-1,
 ARM920T_R8, ARM920T_R9, ARM920T_R10, ARM920T_R11, ARM920T_R12, ARM920T_R13, ARM920T_R14, ARM920T_R15, (UINT8)-1,
 ARM920T_CPSR, 0
};
void arm920t_get_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_REGISTER + ARM920T_R0 && state <= CPUINFO_INT_REGISTER + ARM920T_R15) {
		info->i = GetRegister(state - CPUINFO_INT_REGISTER - ARM920T_R0); return;
	}
	if (state >= CPUINFO_STR_REGISTER + ARM920T_R0 && state <= CPUINFO_STR_REGISTER + ARM920T_R15) {
		int r = state - CPUINFO_STR_REGISTER - ARM920T_R0;
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
	case CPUINFO_INT_PC: case CPUINFO_INT_REGISTER + ARM920T_PC: info->i = R15; break;
	case CPUINFO_INT_PREVIOUSPC: info->i = cpu.previouspc; break;
	case CPUINFO_INT_SP: info->i = GetRegister(13); break;
	case CPUINFO_INT_REGISTER + ARM920T_CPSR: info->i = GET_CPSR; break;
	case CPUINFO_PTR_REGISTER_LAYOUT: info->p = arm920t_reg_layout; break;
	case CPUINFO_PTR_TRANSLATE: info->translate = arm920t_translate; break;
	case CPUINFO_PTR_SET_INFO: info->setinfo = arm920t_set_info; break;
	case CPUINFO_PTR_GET_CONTEXT: info->getcontext = arm920t_get_context; break;
	case CPUINFO_PTR_SET_CONTEXT: info->setcontext = arm920t_set_context; break;
	case CPUINFO_PTR_INIT: info->init = arm920t_init; break;
	case CPUINFO_PTR_RESET: info->reset = arm920t_reset; break;
	case CPUINFO_PTR_EXECUTE: info->execute = arm920t_execute; break;
	case CPUINFO_PTR_DISASSEMBLE: info->disassemble = arm920t_dasm; break;
	case CPUINFO_PTR_INSTRUCTION_COUNTER: info->icount = &m_icount; break;
	case CPUINFO_STR_NAME: strcpy(info->s = cpuintrf_temp_str(), "ARM920T"); break;
	case CPUINFO_STR_CORE_FAMILY: strcpy(info->s = cpuintrf_temp_str(), "ARM920T"); break;
	case CPUINFO_STR_CORE_VERSION: strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
	case CPUINFO_STR_CORE_FILE: strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
	case CPUINFO_STR_CORE_CREDITS: strcpy(info->s = cpuintrf_temp_str(), "Steve Ellenoff, R. Belmont, Ryan Holtz"); break;
	case CPUINFO_STR_FLAGS: sprintf(info->s = cpuintrf_temp_str(), "%08X %s", GET_CPSR, GetModeText(GET_CPSR)); break;
	case CPUINFO_STR_REGISTER + ARM920T_PC: sprintf(info->s = cpuintrf_temp_str(), "PC  :%08X", R15); break;
	case CPUINFO_STR_REGISTER + ARM920T_CPSR: sprintf(info->s = cpuintrf_temp_str(), "CPSR:%08X", GET_CPSR); break;
	}
}

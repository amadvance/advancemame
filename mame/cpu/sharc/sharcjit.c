// license:BSD-3-Clause
// copyright-holders:Andrea Mazzoleni
/***************************************************************************

    sharcjit.c

    Optional ADSP-2106x SHARC recompiler built on sljit (included by
    sharc.c, after the opcodes). Blocks of up to SHARC_JIT_MAX_BLOCK
    instructions of the internal program memory, decoded once:

    - the instructions that only work on registers and memory (compute,
      data moves, immediate loads and the DAG modifies) run back to back:
      they never see the pipeline addresses, the stacks or the system
      registers, so the block keeps PC, DADDR, FADDR and NFADDR and the
      cycle count up to date only where it leaves. Before an instruction
      that can reach a driver handler the PC is set, as the handlers may
      look at it;
    - the most frequent of them are native code: the R and I registers,
      the accesses to the internal data memory and the DAG updates inline,
      the conditions, the ALU, multiplier and shifter operations and the
      other uregs as direct calls of their C functions. An access outside
      the internal memory, or a write over translated code, decided before
      any side effect, runs the instruction's C handler instead;
    - any other instruction (jumps, calls, loops, stack and system register
      writes, IDLE) ends the block: it runs with the whole state up to date
      and the interpreter's steps after it;
    - the end of the current loop (LADDR) is checked before every
      instruction, and the block leaves before it, so the interpreter runs
      the loop step;
    - an access stalled by a driver handler (sharc_stall) leaves the block
      with the state of the interpreter's rewind; a write over translated
      code, a stalled external write or a handler ending the timeslice
      (the cycle count changed) leaves after the instruction.

    A block only starts with no delayed branch pending, no system register
    write latency and no DMA running, and only when the whole of it fits in
    the timeslice, so every instruction runs with the same cycles as in the
    interpreter. The ASTAT history (astat_old ...) is only read by the loop
    step, so it is shifted after each instruction instead of before.

    Writes over translated code (by any instruction, the DMA or the host)
    drop every translation at the next dispatch; reset and a loaded state
    too. Each SHARC instance has its own translations.

***************************************************************************/

#if SHARC_JIT_SUPPORTED

#define SHARC_JIT_MAX_BLOCK		32
#define SHARC_JIT_PM_BASE		0x20000
#define SHARC_JIT_PM_SIZE		0x20000

static int sharc_jit_enabled;

typedef void (SLJIT_FUNC *sharc_jit_func)(void);

/* internal program memory addresses with a translation (pm_internal) */
static int sharc_jit_pm_valid(UINT32 a)
{
	return a >= SHARC_JIT_PM_BASE && a < SHARC_JIT_PM_BASE + SHARC_JIT_PM_SIZE && (a & 0x7fff) < 0x5000;
}

static void sharc_jit_free(sharc_instance *inst)
{
	UINT32 i;

	if (!inst->jit_code)
		return;
	for (i = 0; i < SHARC_JIT_PM_SIZE; ++i)
		if (inst->jit_code[i])
		{
			sljit_free_code(inst->jit_code[i], NULL);
			inst->jit_code[i] = NULL;
		}
	memset(inst->jit_codemap, 0, 2 * 0x8000);
	inst->jit_dirty = 0;
}

/* a write over translated code: everything is dropped at the next dispatch */
static void sharc_jit_code_written(void)
{
	sharc_cur->jit_dirty = 1;
}

/* the words of the internal memory holding the instruction at 'a' */
static void sharc_jit_mark(UINT32 a)
{
	UINT32 base = 0;
	int odd = 0;
	UINT32 *blk = pm_internal(a, &base, &odd);
	UINT8 *map = sharc_cur->jit_codemap + ((blk == sharc_block[1]) ? 0x8000 : 0);

	map[base + 0x1000] = 1;
	map[odd ? base + 0x2000 : base] = 1;
}

/* the uregs written inside a block: data and DAG registers and PX */
static int sharc_jit_ureg_ok(int ureg)
{
	int g = (ureg >> 4) & 0xf;
	return g <= 4 || g == 0xd;
}

enum
{
	SJ_FULL,		/* ends the block, with the whole state up to date */
	SJ_PURE,		/* registers only */
	SJ_PURE_MEM		/* registers and memory: it may reach a driver handler, stall or write code */
};

static int sharc_jit_kind(UINT64 op)
{
	void (*h)(void) = sharc_op[(op >> 39) & 0x1ff];

	if (h == sharcop_nop || h == sharcop_compute || h == sharcop_imm_shift || h == sharcop_compute_modify || h == sharcop_modify)
		return SJ_PURE;
	if (h == sharcop_compute_ureg_to_ureg)
		return sharc_jit_ureg_ok(op_get_ureg_dst(op)) ? SJ_PURE : SJ_FULL;
	if (h == sharcop_imm_to_ureg)
		return sharc_jit_ureg_ok((int)((op >> 32) & 0xff)) ? SJ_PURE : SJ_FULL;
	if (h == sharcop_compute_dreg_dm_dreg_pm || h == sharcop_compute_dm_to_dreg_immmod || h == sharcop_compute_dreg_to_dm_immmod ||
		h == sharcop_compute_pm_to_dreg_immmod || h == sharcop_compute_dreg_to_pm_immmod || h == sharcop_imm_shift_dreg_dmpm ||
		h == sharcop_ureg_to_dm_direct || h == sharcop_ureg_to_pm_direct || h == sharcop_ureg_to_dm_indirect ||
		h == sharcop_ureg_to_pm_indirect || h == sharcop_imm_to_dmpm)
		return SJ_PURE_MEM;
	if (h == sharcop_compute_ureg_dmpm_premod || h == sharcop_compute_ureg_dmpm_postmod)
		return (((op >> 31) & 1) || sharc_jit_ureg_ok((int)((op >> 23) & 0xff))) ? SJ_PURE_MEM : SJ_FULL;
	if (h == sharcop_dm_to_ureg_direct || h == sharcop_pm_to_ureg_direct || h == sharcop_dm_to_ureg_indirect || h == sharcop_pm_to_ureg_indirect)
		return sharc_jit_ureg_ok((int)((op >> 32) & 0xff)) ? SJ_PURE_MEM : SJ_FULL;
	return SJ_FULL;
}

/*
    An instruction that ends the block, as the interpreter runs it: the
    pipeline addresses and the opcode are already set. Returns nonzero
    when a driver handler stalled it (nothing was committed).
*/
static sljit_sw sharc_jit_full(void)
{
	int dma_stalled = 0;
	int flag_wait;

	m_core->stalled = 0;
	sharc_flag_spin = 0;

	(*sharc_op[(m_core->opcode >> 39) & 0x1ff])();

	flag_wait = sharc_flag_spin;

	m_core->pf_rvalid = 0;
	m_core->pf_wvalid = 0;

	if (m_core->stalled)
	{
		m_core->stalled = 0;
		return 1;
	}

	if (m_core->systemreg_latency_cycles > 0)
	{
		--m_core->systemreg_latency_cycles;
		if (m_core->systemreg_latency_cycles <= 0)
			systemreg_write_latency_effect();
	}

	if (!m_core->write_stalled && (m_core->dma_status & ((1 << 6) | (1 << 7))))
	{
		dma_stalled = dma_run_cycle(6);
		dma_stalled |= dma_run_cycle(7);
	}

	--sharc_icount;

	if (dma_stalled)
		sharc_icount = 0;

	if (flag_wait && sharc_icount > 0)
	{
		while (sharc_icount > 0 && (m_core->dma_status & ((1 << 6) | (1 << 7))) && !m_core->write_stalled)
		{
			dma_stalled = dma_run_cycle(6);
			dma_stalled |= dma_run_cycle(7);
			if (dma_stalled)
				break;
			--sharc_icount;
		}
		sharc_icount = 0;
	}

	return 0;
}

#define SJ_REG(field)		SLJIT_MEM1(SLJIT_S0), offsetof(sharc_regs, field)
#define SJ_R(x)				SLJIT_MEM1(SLJIT_S0), (sljit_sw)(offsetof(sharc_regs, r) + (x) * sizeof(SHARC_REG))
#define SJ_DAG(pm, f, x)	SLJIT_MEM1(SLJIT_S0), (sljit_sw)((pm ? offsetof(sharc_regs, dag2) : offsetof(sharc_regs, dag1)) + offsetof(SHARC_DAG, f) + (x) * sizeof(UINT32))

/* the locals: the PC before the first instruction (for its rewind) and the cycle count before an instruction reaching a handler */
#define SJ_LOCAL_PC			SLJIT_MEM1(SLJIT_SP), 0
#define SJ_LOCAL_ICOUNT		SLJIT_MEM1(SLJIT_SP), sizeof(sljit_sw)

/* the cycles of 'count' instructions */
static void sharc_jit_emit_cycles(struct sljit_compiler *C, int count)
{
	if (count)
		sljit_emit_op2(C, SLJIT_SUB32, SLJIT_MEM1(SLJIT_S1), 0, SLJIT_MEM1(SLJIT_S1), 0, SLJIT_IMM, count);
}

/* PC = pc (an immediate, or the PC at the block start when pc is ~0) and the linear fetch addresses after it */
static void sharc_jit_emit_pipeline(struct sljit_compiler *C, UINT32 pc, UINT32 next)
{
	if (pc == 0xffffffff)
	{
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_LOCAL_PC);
		sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(pc), SLJIT_R0, 0);
	}
	else
		sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(pc), SLJIT_IMM, pc);
	sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(daddr), SLJIT_IMM, next);
	sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(faddr), SLJIT_IMM, next + 1);
	sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(nfaddr), SLJIT_IMM, next + 2);
}

/* the ASTAT history after an instruction, S2 holding ASTAT before it */
static void sharc_jit_emit_history(struct sljit_compiler *C)
{
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_REG(astat_old_old));
	sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(astat_old_old_old), SLJIT_R0, 0);
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_REG(astat_old));
	sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(astat_old_old), SLJIT_R0, 0);
	sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(astat_old), SLJIT_S2, 0);
}

static void sharc_jit_emit_opcode(struct sljit_compiler *C, UINT64 op)
{
	/* the hosts with a recompiler are little-endian */
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), offsetof(sharc_regs, opcode), SLJIT_IMM, (sljit_s32)(UINT32)op);
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), offsetof(sharc_regs, opcode) + 4, SLJIT_IMM, (sljit_s32)(UINT32)(op >> 32));
}

/*
    Native code. S0 holds the state, S3 the word of the internal memory
    of the access, S4 a value kept across the calls, S5 the internal
    memory. The ALU, multiplier and shifter operations, the conditions and
    the system uregs go through their C functions, called directly. The
    code jumps to 'slow' before any side effect when the instruction has to
    run in C (an access outside the internal memory, or a write over
    translated code), and to 'skip' when its condition is false.
*/
typedef struct
{
	struct sljit_compiler *C;
	sharc_instance *inst;
	struct sljit_jump *slow[4];
	int nslow;
	struct sljit_jump *skip;
} sharc_jit_ctx;

static void sharc_jit_emit_cond(sharc_jit_ctx *x, int cond)
{
	struct sljit_compiler *C = x->C;

	if (cond == 0x1f)		/* TRUE */
		return;
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, cond);
	sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(32, 32), SLJIT_IMM, SLJIT_FUNC_ADDR(IF_CONDITION_CODE));
	x->skip = sljit_emit_cmp(C, SLJIT_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, 0);
}

static void sharc_jit_emit_compute(sharc_jit_ctx *x, UINT32 compute)
{
	if (!compute)
		return;
	sljit_emit_op1(x->C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, compute);
	sljit_emit_icall(x->C, SLJIT_CALL, SLJIT_ARGS1V(32), SLJIT_IMM, SLJIT_FUNC_ADDR(COMPUTE));
}

/* GET_UREG(ureg) into dst */
static void sharc_jit_emit_get_ureg(sharc_jit_ctx *x, int ureg, sljit_s32 dst)
{
	struct sljit_compiler *C = x->C;
	int g = (ureg >> 4) & 0xf, reg = ureg & 0xf;

	if (g == 0)
		sljit_emit_op1(C, SLJIT_MOV32, dst, 0, SJ_R(reg));
	else if (g == 1)
		sljit_emit_op1(C, SLJIT_MOV32, dst, 0, SJ_DAG(reg & 8, i, reg & 7));
	else
	{
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, ureg);
		sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(32, 32), SLJIT_IMM, SLJIT_FUNC_ADDR(GET_UREG));
		if (dst != SLJIT_R0)
			sljit_emit_op1(C, SLJIT_MOV32, dst, 0, SLJIT_R0, 0);
	}
}

/* SET_UREG(ureg, src): src a saved register, R0 or an immediate */
static void sharc_jit_emit_set_ureg(sharc_jit_ctx *x, int ureg, sljit_s32 src, sljit_sw srcw)
{
	struct sljit_compiler *C = x->C;
	int g = (ureg >> 4) & 0xf, reg = ureg & 0xf;

	if (g == 0)
		sljit_emit_op1(C, SLJIT_MOV32, SJ_R(reg), src, srcw);
	else if (g == 1)
		sljit_emit_op1(C, SLJIT_MOV32, SJ_DAG(reg & 8, i, reg & 7), src, srcw);
	else
	{
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, src, srcw);
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, ureg);
		sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32), SLJIT_IMM, SLJIT_FUNC_ADDR(SET_UREG));
	}
}

/*
    R0 holds a DM address: S3 = its word in the internal memory (the
    block in bit 15, as data_2m maps it), or a jump to the slow path when
    it is outside 0x20000-0x3ffff, or, for a write, over translated code.
*/
static void sharc_jit_emit_dm_word(sharc_jit_ctx *x, int write)
{
	struct sljit_compiler *C = x->C;

	sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_IMM, 0x20000);
	x->slow[x->nslow++] = sljit_emit_cmp(C, SLJIT_GREATER_EQUAL | SLJIT_32, SLJIT_R1, 0, SLJIT_IMM, 0x20000);
	sljit_emit_op2u(C, SLJIT_AND32 | SLJIT_SET_Z, SLJIT_R0, 0, SLJIT_IMM, 0x18000);
	sljit_emit_op_flags(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_NOT_ZERO);
	sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 15);
	sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0x7fff);
	sljit_emit_op2(C, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
	sljit_emit_op1(C, SLJIT_MOV_U32, SLJIT_S3, 0, SLJIT_R0, 0);
	if (write)
	{
		sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)x->inst->jit_codemap);
		sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R1, 0, SLJIT_MEM2(SLJIT_R1, SLJIT_S3), 0);
		x->slow[x->nslow++] = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_R1, 0, SLJIT_IMM, 0);
	}
}

#define SJ_MEM_WORD		SLJIT_MEM2(SLJIT_S5, SLJIT_S3), 2

/* I += modifier (the M register 'mreg', or 'imm' when mreg < 0), kept in its circular buffer */
static void sharc_jit_emit_modify(sharc_jit_ctx *x, int pm, int i, int mreg, INT32 imm)
{
	struct sljit_compiler *C = x->C;
	struct sljit_jump *linear, *above, *inside, *wrap;

	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_DAG(pm, i, i));
	if (mreg >= 0)
		sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SJ_DAG(pm, m, mreg));
	else
		sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, imm);
	sljit_emit_op1(C, SLJIT_MOV32, SJ_DAG(pm, i, i), SLJIT_R0, 0);

	/* the buffer is [B, B+L): an index at or after its end wraps down, one before its start wraps up */
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SJ_DAG(pm, l, i));
	linear = sljit_emit_cmp(C, SLJIT_EQUAL | SLJIT_32, SLJIT_R1, 0, SLJIT_IMM, 0);
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, SJ_DAG(pm, b, i));
	sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R3, 0, SLJIT_R2, 0, SLJIT_R1, 0);
	above = sljit_emit_cmp(C, SLJIT_LESS | SLJIT_32, SLJIT_R0, 0, SLJIT_R3, 0);
	sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
	wrap = sljit_emit_jump(C, SLJIT_JUMP);
	sljit_set_label(above, sljit_emit_label(C));
	inside = sljit_emit_cmp(C, SLJIT_GREATER_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_R2, 0);
	sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
	sljit_set_label(wrap, sljit_emit_label(C));
	sljit_emit_op1(C, SLJIT_MOV32, SJ_DAG(pm, i, i), SLJIT_R0, 0);
	{
		struct sljit_label *done = sljit_emit_label(C);
		sljit_set_label(linear, done);
		sljit_set_label(inside, done);
	}
}

/* the native form of an instruction; 0 when it has none (it then runs in C) */
static int sharc_jit_emit_native(sharc_jit_ctx *x, UINT64 op)
{
	struct sljit_compiler *C = x->C;
	void (*h)(void) = sharc_op[(op >> 39) & 0x1ff];

	if (h == sharcop_nop)
		return 1;

	if (h == sharcop_compute)
	{
		UINT32 compute = op_get_compute(op);
		/* with no operation the condition has no effect */
		if (compute)
		{
			sharc_jit_emit_cond(x, op_get_cond(op));
			sharc_jit_emit_compute(x, compute);
		}
		return 1;
	}

	if (h == sharcop_compute_dm_to_dreg_immmod || h == sharcop_compute_dreg_to_dm_immmod)
	{
		int write = (h == sharcop_compute_dreg_to_dm_immmod);
		int u = (int)((op >> 38) & 0x1);
		int dreg = (int)((op >> 23) & 0xf);
		int i = (int)((op >> 41) & 0x7);
		INT32 mod = op_get_reladdr(op);

		/* the source register before the operation changes it */
		if (write)
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_S4, 0, SJ_R(dreg));
		sharc_jit_emit_cond(x, op_get_cond(op));
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_DAG(0, i, i));
		if (!u)
			sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, mod);
		sharc_jit_emit_dm_word(x, write);
		sharc_jit_emit_compute(x, op_get_compute(op));
		if (write)
			sljit_emit_op1(C, SLJIT_MOV32, SJ_MEM_WORD, SLJIT_S4, 0);
		else
		{
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_MEM_WORD);
			sljit_emit_op1(C, SLJIT_MOV32, SJ_R(dreg), SLJIT_R0, 0);
		}
		if (u)
			sharc_jit_emit_modify(x, 0, i, -1, mod);
		return 1;
	}

	if (h == sharcop_compute_ureg_dmpm_postmod || h == sharcop_compute_ureg_dmpm_premod)
	{
		int post = (h == sharcop_compute_ureg_dmpm_postmod);
		int i = (int)((op >> 41) & 0x7);
		int m = (int)((op >> 38) & 0x7);
		int g = (int)((op >> 32) & 0x1);
		int d = (int)((op >> 31) & 0x1);
		int ureg = (int)((op >> 23) & 0xff);

		/* program memory goes through C */
		if (g)
			return 0;
		sharc_jit_emit_cond(x, op_get_cond(op));
		if (d)
			sharc_jit_emit_get_ureg(x, ureg, SLJIT_S4);
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_DAG(0, i, i));
		if (!post)
			sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SJ_DAG(0, m, m));
		sharc_jit_emit_dm_word(x, d);
		sharc_jit_emit_compute(x, op_get_compute(op));
		if (d)
			sljit_emit_op1(C, SLJIT_MOV32, SJ_MEM_WORD, SLJIT_S4, 0);
		else
		{
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_S4, 0, SJ_MEM_WORD);
			sharc_jit_emit_set_ureg(x, ureg, SLJIT_S4, 0);
		}
		if (post)
			sharc_jit_emit_modify(x, 0, i, m, 0);
		return 1;
	}

	if (h == sharcop_compute_ureg_to_ureg)
	{
		sharc_jit_emit_cond(x, op_get_cond_ureg(op));
		sharc_jit_emit_get_ureg(x, op_get_ureg_src(op), SLJIT_S4);
		sharc_jit_emit_compute(x, op_get_compute(op));
		sharc_jit_emit_set_ureg(x, op_get_ureg_dst(op), SLJIT_S4, 0);
		return 1;
	}

	if (h == sharcop_imm_to_ureg)
	{
		sharc_jit_emit_set_ureg(x, (int)((op >> 32) & 0xff), SLJIT_IMM, (sljit_s32)(UINT32)op);
		return 1;
	}

	if (h == sharcop_dm_to_ureg_direct || h == sharcop_ureg_to_dm_direct)
	{
		int write = (h == sharcop_ureg_to_dm_direct);
		int ureg = (int)((op >> 32) & 0xff);
		UINT32 address = (UINT32)op;
		UINT32 word;

		/* elsewhere through C */
		if (address < 0x20000 || address >= 0x40000)
			return 0;
		word = ((address & 0x18000) ? 0x8000 : 0) | (address & 0x7fff);
		if (write)
		{
			sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R0, 0, SLJIT_MEM0(), (sljit_sw)&x->inst->jit_codemap[word]);
			x->slow[x->nslow++] = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, 0);
			sharc_jit_emit_get_ureg(x, ureg, SLJIT_R0);
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S5), word * 4, SLJIT_R0, 0);
		}
		else
		{
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_S4, 0, SLJIT_MEM1(SLJIT_S5), word * 4);
			sharc_jit_emit_set_ureg(x, ureg, SLJIT_S4, 0);
		}
		return 1;
	}

	if (h == sharcop_dm_to_ureg_indirect || h == sharcop_ureg_to_dm_indirect)
	{
		int write = (h == sharcop_ureg_to_dm_indirect);
		int ureg = (int)((op >> 32) & 0xff);
		int i = (int)((op >> 41) & 0x7);

		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_DAG(0, i, i));
		sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, (sljit_s32)(UINT32)op);
		sharc_jit_emit_dm_word(x, write);
		if (write)
		{
			sharc_jit_emit_get_ureg(x, ureg, SLJIT_R0);
			sljit_emit_op1(C, SLJIT_MOV32, SJ_MEM_WORD, SLJIT_R0, 0);
		}
		else
		{
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_S4, 0, SJ_MEM_WORD);
			sharc_jit_emit_set_ureg(x, ureg, SLJIT_S4, 0);
		}
		return 1;
	}

	if (h == sharcop_compute_modify)
	{
		sharc_jit_emit_cond(x, op_get_cond(op));
		sharc_jit_emit_compute(x, op_get_compute(op));
		sharc_jit_emit_modify(x, (int)((op >> 38) & 0x1), (int)((op >> 30) & 0x7), (int)((op >> 27) & 0x7), 0);
		return 1;
	}

	if (h == sharcop_modify)
	{
		sharc_jit_emit_modify(x, (int)((op >> 38) & 0x1), (int)((op >> 32) & 0x7), -1, (INT32)(UINT32)op);
		return 1;
	}

	if (h == sharcop_imm_shift)
	{
		sharc_jit_emit_cond(x, op_get_cond(op));
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, (int)((op >> 16) & 0x3f));
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, (int)(((op >> 8) & 0xff) | ((op >> 19) & 0xf00)));
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, (int)((op >> 4) & 0xf));
		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R3, 0, SLJIT_IMM, (int)(op & 0xf));
		sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS4V(32, 32, 32, 32), SLJIT_IMM, SLJIT_FUNC_ADDR(SHIFT_OPERATION_IMM));
		return 1;
	}

	return 0;
}

/*
    The instruction through its C, as the interpreter runs it; a stall
    (nothing committed) jumps to *stall.
*/
static void sharc_jit_emit_c(struct sljit_compiler *C, UINT32 a, UINT64 op, int kind, struct sljit_jump **stall)
{
	sharc_jit_emit_opcode(C, op);
	if (kind == SJ_PURE_MEM)
	{
		/* a driver handler may look at the PC */
		sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(pc), SLJIT_IMM, a);
		sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(stalled), SLJIT_IMM, 0);
	}

	sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS0V(), SLJIT_IMM, SLJIT_FUNC_ADDR(sharc_op[(op >> 39) & 0x1ff]));

	if (kind == SJ_PURE_MEM)
	{
		sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(pf_rvalid), SLJIT_IMM, 0);
		sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(pf_wvalid), SLJIT_IMM, 0);
		*stall = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SJ_REG(stalled), SLJIT_IMM, 0);
	}
}

static void *sharc_jit_compile(UINT32 pc)
{
	sharc_instance *inst = sharc_cur;
	struct sljit_compiler *C;
	struct sljit_jump *before[SHARC_JIT_MAX_BLOCK], *after[SHARC_JIT_MAX_BLOCK][3], *stall[SHARC_JIT_MAX_BLOCK];
	UINT64 ops[SHARC_JIT_MAX_BLOCK];
	UINT8 kind[SHARC_JIT_MAX_BLOCK];
	int synced_before[SHARC_JIT_MAX_BLOCK];
	int n, i, synced = 0;
	void *code;

	/* the instructions of the block: the registers and memory ones, and at most one other at the end */
	for (n = 0; n < SHARC_JIT_MAX_BLOCK && sharc_jit_pm_valid(pc + n); ++n)
	{
		ops[n] = pm_read48(pc + n);
		kind[n] = sharc_jit_kind(ops[n]);
		if (kind[n] == SJ_FULL)
		{
			++n;
			break;
		}
	}
	if (n == 0)
		return NULL;

	C = sljit_create_compiler(NULL);
	if (!C)
		return NULL;

	sljit_emit_enter(C, 0, SLJIT_ARGS0V(), 4, 6, 2 * sizeof(sljit_sw));
	sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_S0, 0, SLJIT_IMM, (sljit_sw)&inst->regs);
	sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_S1, 0, SLJIT_IMM, (sljit_sw)&sharc_icount);
	sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_S5, 0, SLJIT_IMM, (sljit_sw)&inst->block[0][0]);
	sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_REG(pc));
	sljit_emit_op1(C, SLJIT_MOV32, SJ_LOCAL_PC, SLJIT_R0, 0);

	for (i = 0; i < n; ++i)
	{
		UINT32 a = pc + i;
		UINT64 op = ops[i];

		before[i] = NULL;
		after[i][0] = after[i][1] = after[i][2] = NULL;
		stall[i] = NULL;
		synced_before[i] = synced;

		/* the end of the current loop: the interpreter runs the loop step (the dispatcher checks the first) */
		if (i > 0)
			before[i] = sljit_emit_cmp(C, SLJIT_EQUAL | SLJIT_32, SLJIT_MEM0(), (sljit_sw)&inst->jit_lend, SLJIT_IMM, a);

		sljit_emit_op1(C, SLJIT_MOV32, SLJIT_S2, 0, SJ_REG(astat));

		if (kind[i] == SJ_FULL)
		{
			struct sljit_jump *ok;

			sharc_jit_emit_opcode(C, op);
			sharc_jit_emit_cycles(C, i - synced);
			sharc_jit_emit_pipeline(C, a, a + 1);
			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS0(W), SLJIT_IMM, SLJIT_FUNC_ADDR(sharc_jit_full));
			ok = sljit_emit_cmp(C, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0);
			/* stalled: the interpreter's rewind, and the timeslice is over */
			sharc_jit_emit_pipeline(C, (i == 0) ? 0xffffffff : a - 1, a);
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S1), 0, SLJIT_IMM, 0);
			sljit_emit_return_void(C);
			sljit_set_label(ok, sljit_emit_label(C));
			sharc_jit_emit_history(C);
			sljit_emit_return_void(C);
			break;
		}

		if (kind[i] == SJ_PURE_MEM)
		{
			/*
			 * A driver handler may look at the time, and end the timeslice:
			 * the cycle count is brought up to date, and kept to see a change.
			 */
			sharc_jit_emit_cycles(C, i - synced);
			synced = i;
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S1), 0);
			sljit_emit_op1(C, SLJIT_MOV32, SJ_LOCAL_ICOUNT, SLJIT_R0, 0);
		}

		{
			sharc_jit_ctx x;
			x.C = C;
			x.inst = inst;
			x.nslow = 0;
			x.skip = NULL;

			if (sharc_jit_emit_native(&x, op))
			{
				struct sljit_label *join;

				if (x.nslow)
				{
					struct sljit_jump *done = sljit_emit_jump(C, SLJIT_JUMP);
					struct sljit_label *slow = sljit_emit_label(C);
					int k;
					for (k = 0; k < x.nslow; ++k)
						sljit_set_label(x.slow[k], slow);
					sharc_jit_emit_c(C, a, op, kind[i], &stall[i]);
					join = sljit_emit_label(C);
					sljit_set_label(done, join);
				}
				else
					join = sljit_emit_label(C);
				if (x.skip)
					sljit_set_label(x.skip, join);
			}
			else
				sharc_jit_emit_c(C, a, op, kind[i], &stall[i]);
		}

		sharc_jit_emit_history(C);

		/* code written, an external write stalled the bus or a handler ended the timeslice: the rest is the dispatcher's */
		if (kind[i] == SJ_PURE_MEM && i < n - 1)
		{
			after[i][0] = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_MEM0(), (sljit_sw)&inst->jit_dirty, SLJIT_IMM, 0);
			after[i][1] = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SJ_REG(write_stalled), SLJIT_IMM, 0);
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SJ_LOCAL_ICOUNT);
			after[i][2] = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_MEM1(SLJIT_S1), 0, SLJIT_R0, 0);
		}
	}

	/* the end of a block of registers and memory instructions */
	if (kind[n - 1] != SJ_FULL)
	{
		sharc_jit_emit_cycles(C, n - synced);
		sharc_jit_emit_pipeline(C, pc + n - 1, pc + n);
		sljit_emit_return_void(C);
	}

	/* the exits: before an instruction, after one, and the rewind of a stalled one */
	for (i = 0; i < n; ++i)
	{
		UINT32 a = pc + i;

		if (before[i])
		{
			sljit_set_label(before[i], sljit_emit_label(C));
			sharc_jit_emit_cycles(C, i - synced_before[i]);
			sharc_jit_emit_pipeline(C, a - 1, a);
			sljit_emit_return_void(C);
		}
		if (after[i][0])
		{
			struct sljit_label *l = sljit_emit_label(C);
			sljit_set_label(after[i][0], l);
			sljit_set_label(after[i][1], l);
			sljit_set_label(after[i][2], l);
			/* the count was brought up to date before the instruction */
			sharc_jit_emit_cycles(C, 1);
			sharc_jit_emit_pipeline(C, a, a + 1);
			sljit_emit_return_void(C);
		}
		if (stall[i])
		{
			sljit_set_label(stall[i], sljit_emit_label(C));
			sljit_emit_op1(C, SLJIT_MOV32, SJ_REG(stalled), SLJIT_IMM, 0);
			sharc_jit_emit_pipeline(C, (i == 0) ? 0xffffffff : a - 1, a);
			sljit_emit_op1(C, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S1), 0, SLJIT_IMM, 0);
			sljit_emit_return_void(C);
		}
	}

	code = sljit_generate_code(C, 0, NULL);
	sljit_free_compiler(C);

	if (code)
	{
		inst->jit_len[pc - SHARC_JIT_PM_BASE] = n;
		for (i = 0; i < n; ++i)
			sharc_jit_mark(pc + i);
	}
	return code;
}

/*
    Runs the block at DADDR, the next instruction, when the state allows
    it. Returns 0 when the interpreter has to run the next instruction.
*/
static int sharc_jit_run(void)
{
	sharc_instance *inst = sharc_cur;
	UINT32 a = m_core->daddr;
	void *code;

	if (inst->jit_dirty)
		sharc_jit_free(inst);

	/* no delayed branch pending, no write latency or DMA to step at each instruction */
	if (m_core->faddr != a + 1 || m_core->nfaddr != a + 2)
		return 0;
	if (m_core->systemreg_latency_cycles > 0 || (m_core->dma_status & ((1 << 6) | (1 << 7))))
		return 0;
	if (!sharc_jit_pm_valid(a))
		return 0;

	inst->jit_lend = (m_core->stky & LSEM) ? 0xffffffff : m_core->laddr.addr;
	if (inst->jit_lend == a)
		return 0;

	code = inst->jit_code[a - SHARC_JIT_PM_BASE];
	if (!code)
	{
		code = sharc_jit_compile(a);
		if (!code)
			return 0;
		inst->jit_code[a - SHARC_JIT_PM_BASE] = code;
	}

	if ((int)inst->jit_len[a - SHARC_JIT_PM_BASE] > sharc_icount)
		return 0;

	((sharc_jit_func)code)();
	return 1;
}

/* a loaded state: the internal memory changed */
static void sharc_jit_postload(void)
{
	int i;
	for (i = 0; i < SHARC_MAX_INSTANCES; ++i)
		if (sharc_instances[i] && sharc_instances[i]->jit_code)
			sharc_instances[i]->jit_dirty = 1;
}

static void sharc_jit_init(sharc_instance *inst)
{
	static int postload_registered;

	if (!options.jit)
		return;

	inst->jit_code = calloc(SHARC_JIT_PM_SIZE, sizeof(inst->jit_code[0]));
	inst->jit_len = calloc(SHARC_JIT_PM_SIZE, sizeof(inst->jit_len[0]));
	inst->jit_codemap = calloc(2 * 0x8000, sizeof(inst->jit_codemap[0]));
	if (!inst->jit_code || !inst->jit_len || !inst->jit_codemap)
	{
		free(inst->jit_code);
		free(inst->jit_len);
		free(inst->jit_codemap);
		inst->jit_code = NULL;
		inst->jit_len = NULL;
		inst->jit_codemap = NULL;
		return;
	}

	sharc_jit_enabled = 1;
	if (!postload_registered)
	{
		postload_registered = 1;
		state_save_register_func_postload(sharc_jit_postload);
	}
}

static void sharc_jit_exit(void)
{
	int i;
	for (i = 0; i < SHARC_MAX_INSTANCES; ++i)
	{
		sharc_instance *inst = sharc_instances[i];
		if (inst && inst->jit_code)
		{
			sharc_jit_free(inst);
			free(inst->jit_code);
			free(inst->jit_len);
			free(inst->jit_codemap);
			inst->jit_code = NULL;
			inst->jit_len = NULL;
			inst->jit_codemap = NULL;
		}
	}
	sharc_jit_enabled = 0;
}

#else	/* !SHARC_JIT_SUPPORTED */

static void sharc_jit_code_written(void) { }

#endif	/* SHARC_JIT_SUPPORTED */

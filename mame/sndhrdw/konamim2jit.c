// license:BSD-3-Clause
// copyright-holders:Philip Bennett, Ryan Holtz, Andrea Mazzoleni
/* Native blocks for the Bulldog DSP's side-effect-free arithmetic. */

#define DSPP_JIT_MAX_BLOCK 16

/* Build with -DDSPP_JIT_STATS=1 to print dispatch statistics at exit. */
#ifndef DSPP_JIT_STATS
#define DSPP_JIT_STATS 0
#endif

#if DSPP_JIT_STATS
enum dspp_jit_fallback
{
	DSPP_JIT_FIFO,
	DSPP_JIT_WRITEBACK,
	DSPP_JIT_BUDGET,
	DSPP_JIT_REGISTER_RANGE,
	DSPP_JIT_DECODE,
	DSPP_JIT_COMPILE,
	DSPP_JIT_FALLBACK_COUNT
};

enum dspp_jit_reject
{
	DSPP_JIT_REJECT_NONE,
	DSPP_JIT_REJECT_CONTROL,
	DSPP_JIT_REJECT_SATURATION,
	DSPP_JIT_REJECT_VARIABLE_SHIFT,
	DSPP_JIT_REJECT_ALU,
	DSPP_JIT_REJECT_OPERAND_COUNT,
	DSPP_JIT_REJECT_INDIRECT_ADDRESS,
	DSPP_JIT_REJECT_IO_ADDRESS,
	DSPP_JIT_REJECT_INDIRECT_REGISTER,
	DSPP_JIT_REJECT_IMMEDIATE_DEST,
	DSPP_JIT_REJECT_COUNT
};

static struct dspp_jit_stats
{
	UINT64 lengths[DSPP_JIT_MAX_BLOCK + 1], native_words;
	UINT64 wait_batches, wait_skipped;
	struct
	{
		UINT64 instructions, operand_words;
	} fallback[DSPP_JIT_FALLBACK_COUNT];
	UINT64 rejected[DSPP_JIT_REJECT_COUNT], untracked;
	struct dspp_jit_hotspot
	{
		UINT64 count;
		UINT16 pc, opcode, word;
		UINT8 reject;
	} hotspots[4096];
} m_jit_stats;
static enum dspp_jit_fallback m_jit_fallback;

#define DSPP_JIT_SKIP(reason) (m_jit_fallback = (reason), false)
#define DSPP_JIT_REJECT(reason, word) (inst->reject = (reason), inst->reject_word = (word), false)
#else
#define DSPP_JIT_SKIP(reason) false
#define DSPP_JIT_REJECT(reason, word) false
#endif

enum dspp_jit_operand_type
{
	DSPP_JIT_NONE,
	DSPP_JIT_ACC,
	DSPP_JIT_IMMEDIATE,
	DSPP_JIT_RAM,
	DSPP_JIT_REGISTER,
	DSPP_JIT_PRODUCT
};

struct dspp_jit_operand
{
	enum dspp_jit_operand_type type;
	UINT16 value;
};

struct dspp_jit_instruction
{
	struct dspp_jit_operand a, b, dest, mul_a, mul_b;
	UINT16 opcode;
	unsigned words, groups;
	bool writeback;
#if DSPP_JIT_STATS
	UINT8 reject;
	UINT16 reject_word;
#endif
};

typedef void (SLJIT_FUNC *dspp_jit_func)(void);

static struct dspp_jit_block
{
	dspp_jit_func code;
	INT16 cycles;
	UINT16 last_opcode;
	UINT8 groups;
	bool writeback;
#if DSPP_JIT_STATS
	UINT8 length, reject;
	UINT16 reject_word;
#endif
} m_jit_blocks[ARRAY_LENGTH(m_program)];
static bool m_jit_dirty;

#if DSPP_JIT_STATS
static void dspp_jit_count_fallback(enum dspp_jit_fallback reason, unsigned pc, UINT16 opcode, UINT64 operand_words)
{
	const unsigned mask = ARRAY_LENGTH(m_jit_stats.hotspots) - 1;
	const struct dspp_jit_block *block = &m_jit_blocks[pc];
	unsigned slot, limit;

	++m_jit_stats.fallback[reason].instructions;
	m_jit_stats.fallback[reason].operand_words += operand_words;
	if (reason != DSPP_JIT_DECODE)
		return;

	++m_jit_stats.rejected[block->reject];
	/* Keep separate counts when the host loads another instruction at this PC. */
	slot = ((pc * 257U) ^ (opcode * 23U) ^ (block->reject_word * 11U) ^ block->reject) & mask;
	for (limit = 0; limit <= mask; ++limit, slot = (slot + 1) & mask)
	{
		struct dspp_jit_hotspot *hotspot = &m_jit_stats.hotspots[slot];

		if (!hotspot->count)
		{
			hotspot->pc = pc;
			hotspot->opcode = opcode;
			hotspot->word = block->reject_word;
			hotspot->reject = block->reject;
		}
		if (hotspot->pc == pc && hotspot->opcode == opcode &&
			hotspot->word == block->reject_word && hotspot->reject == block->reject)
		{
			++hotspot->count;
			return;
		}
	}
	++m_jit_stats.untracked;
}

static int dspp_jit_hotspot_compare(const void *a, const void *b)
{
	const struct dspp_jit_hotspot *left = a, *right = b;

	return (left->count < right->count) - (left->count > right->count);
}

static void dspp_jit_print_stats(void)
{
	static const char *const fallback_names[DSPP_JIT_FALLBACK_COUNT] = {
		"fifo", "writeback", "budget", "register_range", "decode", "compile"
	};
	static const char *const reject_names[DSPP_JIT_REJECT_COUNT] = {
		"none", "control", "saturation", "variable_shift", "alu", "operand_count",
		"indirect_address", "io_address", "indirect_register", "immediate_dest"
	};
	UINT64 blocks = 0, instructions = 0;
	unsigned i;

	for (i = 1; i <= DSPP_JIT_MAX_BLOCK; ++i)
	{
		blocks += m_jit_stats.lengths[i];
		instructions += m_jit_stats.lengths[i] * i;
	}
	fprintf(stderr, "DSPP native blocks=%llu instructions=%llu words=%llu\n",
		(unsigned long long)blocks, (unsigned long long)instructions, (unsigned long long)m_jit_stats.native_words);
	fprintf(stderr, "DSPP output wait batches=%llu skipped_iterations=%llu\n",
		(unsigned long long)m_jit_stats.wait_batches, (unsigned long long)m_jit_stats.wait_skipped);
	fprintf(stderr, "DSPP native lengths");
	for (i = 1; i <= DSPP_JIT_MAX_BLOCK; ++i)
		fprintf(stderr, " %u=%llu", i, (unsigned long long)m_jit_stats.lengths[i]);
	fprintf(stderr, "\n");

	for (i = 0; i < DSPP_JIT_FALLBACK_COUNT; ++i)
		fprintf(stderr, "DSPP fallback %s instructions=%llu operand_words=%llu\n", fallback_names[i],
			(unsigned long long)m_jit_stats.fallback[i].instructions,
			(unsigned long long)m_jit_stats.fallback[i].operand_words);
	for (i = 1; i < DSPP_JIT_REJECT_COUNT; ++i)
		fprintf(stderr, "DSPP rejected %s=%llu\n", reject_names[i], (unsigned long long)m_jit_stats.rejected[i]);

	qsort(m_jit_stats.hotspots, ARRAY_LENGTH(m_jit_stats.hotspots), sizeof(m_jit_stats.hotspots[0]), dspp_jit_hotspot_compare);
	for (i = 0; i < 10 && m_jit_stats.hotspots[i].count; ++i)
	{
		const struct dspp_jit_hotspot *hotspot = &m_jit_stats.hotspots[i];

		fprintf(stderr, "DSPP hotspot pc=%03x opcode=%04x reason=%s word=%04x count=%llu\n",
			hotspot->pc, hotspot->opcode, reject_names[hotspot->reject], hotspot->word, (unsigned long long)hotspot->count);
	}
	if (m_jit_stats.untracked)
		fprintf(stderr, "DSPP untracked hotspots=%llu\n", (unsigned long long)m_jit_stats.untracked);
}
#endif

static void dspp_jit_clear(void)
{
	unsigned i;

	for (i = 0; i < ARRAY_LENGTH(m_jit_blocks); ++i)
		if (m_jit_blocks[i].code)
			sljit_free_code(m_jit_blocks[i].code, NULL);
	memset(m_jit_blocks, 0, sizeof(m_jit_blocks));
	m_jit_dirty = false;
}

static bool dspp_jit_decode(UINT16 pc, struct dspp_jit_instruction *inst)
{
	struct dspp_jit_operand operands[MAX_OPERANDS];
#if DSPP_JIT_STATS
	UINT16 immediate_words[MAX_OPERANDS];
#endif
	const struct dspp_jit_operand acc = { DSPP_JIT_ACC, 0 };
	const struct dspp_jit_operand product = { DSPP_JIT_PRODUCT, 0 };
	UINT16 op = dspp_read_op(pc);
	unsigned numops = dspp_arithmetic_numops(op);
	unsigned muxa = (op >> 10) & 3, muxb = (op >> 8) & 3;
	unsigned alu = (op >> 4) & 0xf, barrel = op & 0xf;
	unsigned count = 0, source = 0;

	if ((op & 0x8000) || barrel == 7 || barrel == 8 || alu == 3 || (alu >= 5 && alu <= 7))
		return DSPP_JIT_REJECT((op & 0x8000) ? DSPP_JIT_REJECT_CONTROL :
			barrel == 7 ? DSPP_JIT_REJECT_SATURATION :
			barrel == 8 ? DSPP_JIT_REJECT_VARIABLE_SHIFT : DSPP_JIT_REJECT_ALU, op);
	if (muxa == 3 || muxb == 3)
		source += op & 0x1000 ? 2 : 1;
	source += (muxa == 1 || muxa == 2) + (muxb == 1 || muxb == 2);
	if (source > numops)
		return DSPP_JIT_REJECT(DSPP_JIT_REJECT_OPERAND_COUNT, op);

	memset(inst, 0, sizeof(*inst));
	inst->opcode = op;
	inst->words = 1;
	while (count < numops)
	{
		UINT16 word = dspp_read_op(pc + inst->words++);

		if ((word & 0xc000) == 0xc000)
		{
			UINT16 value = word & 0x2000 ? (word & 0x1fff) << 3 : dspp_sign_extend(word, 13);
#if DSPP_JIT_STATS
			immediate_words[count] = word;
#endif
			operands[count++] = (struct dspp_jit_operand){ DSPP_JIT_IMMEDIATE, value };
		}
		else if ((word & 0xe000) == 0x8000)
		{
			UINT16 addr = word & 0x3ff;

			if ((word & 0x0400) || addr >= ARRAY_LENGTH(m_data_ram))
				return DSPP_JIT_REJECT((word & 0x0400) ? DSPP_JIT_REJECT_INDIRECT_ADDRESS : DSPP_JIT_REJECT_IO_ADDRESS, word);
			operands[count] = (struct dspp_jit_operand){ DSPP_JIT_RAM, addr };
			if (word & 0x0800)
			{
				inst->dest = operands[count];
				inst->writeback = true;
			}
			++count;
		}
		else
		{
			unsigned nregs = word & 0x8000 ? (word & 0x0400 ? 2 : 1) : 3;
			unsigned i;

			for (i = 0; i < nregs; ++i)
			{
				unsigned reg = (word >> ((nregs - 1 - i) * 5)) & 0x1f;

				if (reg & 0x10)
					return DSPP_JIT_REJECT(DSPP_JIT_REJECT_INDIRECT_REGISTER, word);
				inst->groups |= 1U << (reg >> 2);
				operands[count] = (struct dspp_jit_operand){ DSPP_JIT_REGISTER, reg };
				if ((nregs == 2 && i == 0 && (word & 0x1000)) ||
					(nregs < 3 && i == nregs - 1 && (word & 0x0800)))
				{
					inst->dest = operands[count];
					inst->writeback = true;
				}
				++count;
			}
		}
	}

	source = 0;
	if (muxa == 3 || muxb == 3)
	{
		inst->mul_a = operands[source++];
		inst->mul_b = op & 0x1000 ? operands[source++] : acc;
	}
	inst->a = !muxa ? acc : muxa == 3 ? product : operands[source++];
	inst->b = !muxb ? acc : muxb == 3 ? product : operands[source++];
	if (!inst->writeback && source < numops)
	{
		inst->dest = operands[source];
		if (inst->dest.type == DSPP_JIT_IMMEDIATE)
			return DSPP_JIT_REJECT(DSPP_JIT_REJECT_IMMEDIATE_DEST, immediate_words[source]);
	}
	return true;
}

#define DSPP_JIT_MEM(field) SLJIT_MEM0(), (sljit_sw)&m_core.field
#define DSPP_JIT_RAM_WORD(addr) SLJIT_MEM0(), (sljit_sw)&m_data_ram[addr]

/* The dispatcher checks every register group used by the block before it runs. */
static void dspp_jit_reg_index(struct sljit_compiler *c, sljit_s32 dst, unsigned reg)
{
	sljit_emit_op1(c, SLJIT_MOV_U16, dst, 0, DSPP_JIT_MEM(m_rbase[reg >> 2]));
	sljit_emit_op2(c, SLJIT_AND, dst, 0, dst, 0, SLJIT_IMM, 0x3ff);
	sljit_emit_op2(c, SLJIT_ADD, dst, 0, dst, 0, SLJIT_IMM, reg & 3);
}

static void dspp_jit_load(struct sljit_compiler *c, sljit_s32 dst, struct dspp_jit_operand operand)
{
	switch (operand.type)
	{
		case DSPP_JIT_ACC:
			sljit_emit_op1(c, SLJIT_MOV32, dst, 0, DSPP_JIT_MEM(m_acc));
			break;
		case DSPP_JIT_IMMEDIATE:
			sljit_emit_op1(c, SLJIT_MOV32, dst, 0, SLJIT_IMM, operand.value);
			break;
		case DSPP_JIT_RAM:
			sljit_emit_op1(c, SLJIT_MOV_U16, dst, 0, DSPP_JIT_RAM_WORD(operand.value));
			break;
		case DSPP_JIT_REGISTER:
			dspp_jit_reg_index(c, dst, operand.value);
			sljit_emit_op1(c, SLJIT_MOV_U16, dst, 0, SLJIT_MEM2(SLJIT_S0, dst), 1);
			break;
		case DSPP_JIT_PRODUCT:
			sljit_emit_op1(c, SLJIT_MOV32, dst, 0, SLJIT_R3, 0);
			break;
		default:
			break;
	}
}

static void dspp_jit_alu_input(struct sljit_compiler *c, sljit_s32 dst, struct dspp_jit_operand operand)
{
	dspp_jit_load(c, dst, operand);
	if (operand.type != DSPP_JIT_ACC && operand.type != DSPP_JIT_PRODUCT)
		sljit_emit_op2(c, SLJIT_SHL32, dst, 0, dst, 0, SLJIT_IMM, 4);
	sljit_emit_op2(c, SLJIT_AND32, dst, 0, dst, 0, SLJIT_IMM, 0xfffff);
}

static void dspp_jit_emit_arithmetic(struct sljit_compiler *c, const struct dspp_jit_instruction *inst)
{
	static const sljit_s32 alu_logic[6] = { SLJIT_AND32, SLJIT_AND32, SLJIT_OR32, SLJIT_OR32, SLJIT_XOR32, SLJIT_XOR32 };
	const unsigned alu = (inst->opcode >> 4) & 0xf;
	const unsigned barrel = inst->opcode & 0xf;
	const bool add_sub = alu == 2 || alu == 4;
	const unsigned shift = dspp_shifts[barrel & 8 ? -barrel & 7 : barrel];

	if (inst->mul_a.type != DSPP_JIT_NONE)
	{
		dspp_jit_load(c, SLJIT_R0, inst->mul_a);
		dspp_jit_load(c, SLJIT_R1, inst->mul_b);
		if (inst->mul_b.type == DSPP_JIT_ACC)
			sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 4);
		sljit_emit_op1(c, SLJIT_MOV32_S16, SLJIT_R0, 0, SLJIT_R0, 0);
		sljit_emit_op1(c, SLJIT_MOV32_S16, SLJIT_R1, 0, SLJIT_R1, 0);
		sljit_emit_op2(c, SLJIT_MUL32, SLJIT_R3, 0, SLJIT_R0, 0, SLJIT_R1, 0);
		sljit_emit_op2(c, SLJIT_ASHR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 11);
	}
	dspp_jit_alu_input(c, SLJIT_R0, inst->a);
	dspp_jit_alu_input(c, SLJIT_R1, inst->b);

	if (add_sub)
	{
		sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R3, 0, SLJIT_R0, 0);
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_R1, 0);
		if (alu == 2)
			sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 0x80000);
		sljit_emit_op2(c, alu == 2 ? SLJIT_ADD32 : SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_R0, 0);
		sljit_emit_op2(c, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
		sljit_emit_op2(c, SLJIT_LSHR32, DSPP_JIT_MEM(m_flag_over), SLJIT_R2, 0, SLJIT_IMM, 19);
		sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 20);
		sljit_emit_op2(c, SLJIT_AND32, DSPP_JIT_MEM(m_flag_carry), SLJIT_R2, 0, SLJIT_IMM, 1);
	}
	else
	{
		if (alu == 1)
			sljit_emit_op2(c, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_IMM, 0, SLJIT_R1, 0);
		else if (alu >= 10)
			sljit_emit_op2(c, alu_logic[alu - 10], SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
		if (alu >= 9 && (alu & 1))
			sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, -1);
		sljit_emit_op1(c, SLJIT_MOV32, DSPP_JIT_MEM(m_flag_over), SLJIT_IMM, 0);
		sljit_emit_op1(c, SLJIT_MOV32, DSPP_JIT_MEM(m_flag_carry), SLJIT_IMM, 0);
	}

	sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 19);
	sljit_emit_op2(c, SLJIT_AND32, DSPP_JIT_MEM(m_flag_neg), SLJIT_R2, 0, SLJIT_IMM, 1);
	sljit_emit_op2u(c, SLJIT_AND32 | SLJIT_SET_Z, SLJIT_R0, 0, SLJIT_IMM, 0xffff0);
	sljit_emit_op_flags(c, SLJIT_MOV32, DSPP_JIT_MEM(m_flag_zero), SLJIT_ZERO);
	sljit_emit_op2u(c, SLJIT_AND32 | SLJIT_SET_Z, SLJIT_R0, 0, SLJIT_IMM, 0xf);
	sljit_emit_op_flags(c, SLJIT_MOV32, DSPP_JIT_MEM(m_flag_exact), SLJIT_ZERO);

	if ((barrel & 8) && alu >= 8)
	{
		sljit_emit_op2(c, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xfffff);
		sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift);
	}
	else
	{
		unsigned left = barrel & 8 ? 12 : 12 + shift;
		unsigned right = barrel & 8 ? 12 + shift : 12;

		sljit_emit_op2(c, SLJIT_SHL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, left);
		sljit_emit_op2(c, SLJIT_ASHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, right);
	}
	sljit_emit_op1(c, SLJIT_MOV32, DSPP_JIT_MEM(m_acc), SLJIT_R0, 0);

	if (inst->dest.type != DSPP_JIT_NONE)
	{
		sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 4);
		if (inst->dest.type == DSPP_JIT_RAM)
			sljit_emit_op1(c, SLJIT_MOV_U16, DSPP_JIT_RAM_WORD(inst->dest.value), SLJIT_R0, 0);
		else
		{
			dspp_jit_reg_index(c, SLJIT_R1, inst->dest.value);
			sljit_emit_op1(c, SLJIT_MOV_U16, SLJIT_MEM2(SLJIT_S0, SLJIT_R1), 1, SLJIT_R0, 0);
		}
	}
}

static void dspp_jit_compile(struct dspp_jit_block *block, UINT16 pc)
{
	struct sljit_compiler *c;
	struct dspp_jit_instruction inst;
	unsigned count = 0, cycles = 0;

	block->cycles = -1;
	if (!dspp_jit_decode(pc, &inst))
	{
#if DSPP_JIT_STATS
		block->reject = inst.reject;
		block->reject_word = inst.reject_word;
#endif
		return;
	}
	c = sljit_create_compiler(NULL);
	if (!c)
		return;
	sljit_emit_enter(c, 0, SLJIT_ARGS0V(), 4, 1, 0);
	sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_S0, 0, SLJIT_IMM, (sljit_sw)m_data_ram);
	do
	{
		dspp_jit_emit_arithmetic(c, &inst);
		block->groups |= inst.groups;
		block->writeback |= inst.writeback;
		block->last_opcode = inst.opcode;
		cycles += inst.words;
	} while (++count < DSPP_JIT_MAX_BLOCK && dspp_jit_decode(pc + cycles, &inst));
	sljit_emit_return_void(c);
	block->code = (dspp_jit_func)sljit_generate_code(c, 0, NULL);
	sljit_free_compiler(c);
	if (block->code)
	{
		block->cycles = cycles;
#if DSPP_JIT_STATS
		block->length = count;
#endif
	}
}

static bool dspp_jit_run(UINT64 remaining)
{
	struct dspp_jit_block *block = &m_jit_blocks[m_core.m_pc & 0x3ff];
	unsigned groups, cycles;

	if (m_jit_dirty)
		dspp_jit_clear();
	if (!block->cycles)
		dspp_jit_compile(block, m_core.m_pc);
	if (block->cycles < 0 || (unsigned)block->cycles > remaining)
		return DSPP_JIT_SKIP(block->cycles >= 0 ? DSPP_JIT_BUDGET :
			block->reject ? DSPP_JIT_DECODE : DSPP_JIT_COMPILE);

	for (groups = block->groups; groups; groups &= groups - 1)
	{
		unsigned group = __builtin_ctz(groups);

		if ((m_core.m_rbase[group] & 0x3ff) + 3 >= ARRAY_LENGTH(m_data_ram))
			return DSPP_JIT_SKIP(DSPP_JIT_REGISTER_RANGE);
	}

	block->code();
	/* Each consumed program word costs one cycle, including operand words. */
	cycles = block->cycles;
#if DSPP_JIT_STATS
	++m_jit_stats.lengths[block->length];
	m_jit_stats.native_words += cycles;
#endif
	m_core.m_pc += cycles;
	m_core.m_tclock -= cycles;
	m_elapsed_ticks += cycles;
	m_core.m_op = block->last_opcode;
	if (block->writeback)
		m_core.m_writeback = -1;
	return true;
}

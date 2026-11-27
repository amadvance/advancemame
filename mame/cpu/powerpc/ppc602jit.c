// license:BSD-3-Clause
// copyright-holders:Andrea Mazzoleni
/***************************************************************************

    ppc602jit.c

    PowerPC 602 recompiler built on SLJIT (included by ppc602.c).
    Enabled by the global misc_jit option for CPUs with a direct memory map.

    Translates up to 64 supported instructions from directly mapped ROM or
    RAM. A block stays within one mapped region and one 4 KB page. Direct
    branches to translated instructions in the same block use native labels.
    The block can follow a conditional branch's fallthrough and ends at a
    store, mtmsr, unconditional branch or unsupported opcode. Native exits
    can jump to a cached ROM or RAM block in the same MSR relocation mode.
    The cycle budget bounds internal jumps and chains to other blocks.

    Aligned loads and stores within mapped RAM access host memory directly.
    A memory access that fails a guard exits before that instruction.
    An mtmsr that changes any MSR bit also exits before the write. The
    native code commits the completed PC and cycle count at every block
    boundary so the interpreter can resume after a chain or a side exit.

    Both CPUs share a map of host RAM words containing compiled code.
    A page summary skips the word check for stores to data-only pages.
    Stores to marked words leave native code before writing. The CPU bus
    and device write handlers then invalidate both caches on such writes.
    JIT_CACHE_STATS enables per-CPU counts of native and interpreted
    instructions, block lengths, cache lookups and exits.

    Performance experiments on x86-64 used the first 721 frames of Evil
    Night and Tobe! Polystars, in that order. The following changes were
    dropped after testing:

    - Keeping frequently used guest GPRs in host registers within each
      block reduced median throughput by 1.12% and 1.35%.
    - Calling the bus for slow reads and resuming the native block reduced
      median throughput by 1.22% and 0.77%.
    - An earlier trial with 16384 direct-mapped slots reduced cache misses.
      Throughput changed by -0.20% and +1.33% relative to 8192 slots.

***************************************************************************/

#define PPC602_JIT_MAX_BLOCK 64
#define PPC602_JIT_MODE (MSR_IR | MSR_DR)
#define PPC602_JIT_PAGE_BITS 12
#define PPC602_JIT_CODE_PAGES (PPC602_CODEMAP_SIZE >> (PPC602_JIT_PAGE_BITS - 3))
#define JIT_CACHE_STATS 0

#define PPC602_JIT_STATE(field) SLJIT_MEM1(SLJIT_S0), offsetof(PPC_REGS, field)
#define PPC602_JIT_IMMEDIATE(value) SLJIT_IMM, (sljit_s32)(value)

#define PPC602_JIT_BUDGET SLJIT_S1, 0

typedef INT32 (SLJIT_FUNC *ppc602_jit_func)(PPC_REGS *, INT32);

struct jit_block
{
	void *code, *entry;
	UINT32 pc, mode;
	UINT16 len;
	UINT8 valid;
};

#define JIT_TABLE_SIZE 8192
#define JIT_CACHE_WAYS 1
#define JIT_CACHE_NAME "ppc602"
#define JIT_CACHE_PC_SHIFT 2
#define JIT_CACHE_XOR_SHIFT 15
#define JIT_CACHE_INVALIDATE 1
#define JIT_CACHE_CHAIN 1
#include "../jitcache.h"

static struct jit_cache ppc602_jit_caches[MAX_CPU];
UINT8 ppc602_codemap[PPC602_CODEMAP_SIZE];
/* Mirror the page flags to let a direct RAM span wrap in the host-address hash. */
static UINT32 ppc602_jit_codepages[2 * PPC602_JIT_CODE_PAGES];
static unsigned ppc602_jit_users;

#if JIT_CACHE_STATS
static const unsigned ppc602_jit_xo_majors[] = { 19, 31, 59, 63 };
#define PPC602_JIT_OPCODE_SLOTS (64 + ARRAY_LENGTH(ppc602_jit_xo_majors) * 1024)

static struct ppc602_jit_statistics
{
	UINT64 interpreted, native, rom_blocks, ram_blocks;
	UINT64 interrupt, outside_code, no_code, slice, decrementer;
	UINT64 memory_exits[4], chain_mode_misses;
	UINT64 mtmsr_native, mtmsr_changed_exits;
	UINT64 lengths[PPC602_JIT_MAX_BLOCK + 1];
	UINT64 interpreted_opcodes[PPC602_JIT_OPCODE_SLOTS];
	UINT64 ending_opcodes[PPC602_JIT_OPCODE_SLOTS];
} ppc602_jit_stats[MAX_CPU];

static unsigned ppc602_jit_opcode_slot(UINT32 op)
{
	unsigned major = op >> 26;
	unsigned i;

	for (i = 0; i < ARRAY_LENGTH(ppc602_jit_xo_majors); ++i)
		if (major == ppc602_jit_xo_majors[i])
			return 64 + i * 1024 + ((op >> 1) & 1023);
	return major;
}

static void ppc602_jit_record_interpreted(int cpu, UINT32 op)
{
	struct ppc602_jit_statistics *stats = &ppc602_jit_stats[cpu];

	++stats->interpreted;
	++stats->interpreted_opcodes[ppc602_jit_opcode_slot(op)];
}

static void ppc602_jit_print_stats(int cpu)
{
	const struct ppc602_jit_statistics *stats = &ppc602_jit_stats[cpu];
	unsigned i;

	fprintf(stderr, "PPC602 CPU %d interpreted=%llu native=%llu rom_blocks=%llu ram_blocks=%llu\n", cpu,
		(unsigned long long)stats->interpreted, (unsigned long long)stats->native,
		(unsigned long long)stats->rom_blocks, (unsigned long long)stats->ram_blocks);
	fprintf(stderr, "PPC602 exits CPU %d interrupt=%llu outside_code=%llu no_code=%llu slice=%llu decrementer=%llu"
		" outside_ram=%llu unaligned=%llu reserved=%llu code_write=%llu chain_mode_misses=%llu\n", cpu,
		(unsigned long long)stats->interrupt, (unsigned long long)stats->outside_code,
		(unsigned long long)stats->no_code, (unsigned long long)stats->slice, (unsigned long long)stats->decrementer,
		(unsigned long long)stats->memory_exits[0], (unsigned long long)stats->memory_exits[1],
		(unsigned long long)stats->memory_exits[2], (unsigned long long)stats->memory_exits[3],
		(unsigned long long)stats->chain_mode_misses);
	fprintf(stderr, "PPC602 mtmsr CPU %d native=%llu changed_exits=%llu\n", cpu,
		(unsigned long long)stats->mtmsr_native, (unsigned long long)stats->mtmsr_changed_exits);
	fprintf(stderr, "PPC602 lengths CPU %d", cpu);
	for (i = 1; i < ARRAY_LENGTH(stats->lengths); ++i)
		fprintf(stderr, " %u=%llu", i, (unsigned long long)stats->lengths[i]);
	fputc('\n', stderr);
	for (i = 0; i < PPC602_JIT_OPCODE_SLOTS; ++i)
		if (stats->interpreted_opcodes[i] || stats->ending_opcodes[i])
		{
			unsigned major = i < 64 ? i : ppc602_jit_xo_majors[(i - 64) >> 10];
			int xo = i < 64 ? -1 : (int)((i - 64) & 1023);
			fprintf(stderr, "PPC602 opcode CPU %d major=%u xo=%d interpreted=%llu block_ends=%llu\n", cpu, major, xo,
				(unsigned long long)stats->interpreted_opcodes[i], (unsigned long long)stats->ending_opcodes[i]);
		}
}

#define PPC602_JIT_COUNT(field) (++stats->field)
#else
#define PPC602_JIT_COUNT(field) ((void)0)
#endif

static struct jit_cache *ppc602_jit_cache_for_cpu(int cpu)
{
	struct jit_cache *cache = &ppc602_jit_caches[cpu];

	if (!cache->table)
	{
		jit_cache_init(cache);
		if (cache->table)
			++ppc602_jit_users;
#if JIT_CACHE_STATS
		memset(&ppc602_jit_stats[cpu], 0, sizeof(ppc602_jit_stats[cpu]));
#endif
	}
	return cache->table ? cache : NULL;
}

static void ppc602_jit_clear(int cpu)
{
	jit_cache_clear(&ppc602_jit_caches[cpu]);
}

static void ppc602_jit_clear_code_marks(void)
{
	memset(ppc602_codemap, 0, sizeof(ppc602_codemap));
	memset(ppc602_jit_codepages, 0, sizeof(ppc602_jit_codepages));
}

static void ppc602_jit_mark_code(const void *address)
{
	unsigned word = ppc602_codemap_index(address);
	unsigned page = word >> (PPC602_JIT_PAGE_BITS - 3);

	ppc602_codemap[word] = 1;
	ppc602_jit_codepages[page] = ppc602_jit_codepages[page + PPC602_JIT_CODE_PAGES] = 1;
}

void ppc602_jit_flush_ram(void)
{
	int cpu;

	/* Both caches can hold translations of the same host word, even at different PCs. */
	for (cpu = 0; cpu < MAX_CPU; ++cpu)
		ppc602_jit_clear(cpu);
	ppc602_jit_clear_code_marks();
}

static void ppc602_jit_exit(int cpu)
{
	if (!ppc602_jit_caches[cpu].table)
		return;
	ppc602_jit_clear(cpu);
#if JIT_CACHE_STATS
	ppc602_jit_print_stats(cpu);
#endif
	jit_cache_exit(&ppc602_jit_caches[cpu]);
	if (!--ppc602_jit_users)
		ppc602_jit_clear_code_marks();
}

/* The left operand is in R1. Preserve XER and copy its SO bit to CR. */
static void ppc602_jit_emit_compare(struct sljit_compiler *c, sljit_s32 condition, unsigned cr,
	sljit_s32 rhs, sljit_sw rhsw)
{
	struct sljit_jump *less, *equal, *greater_done, *less_done;

	less = sljit_emit_cmp(c, condition | SLJIT_32, SLJIT_R1, 0, rhs, rhsw);
	equal = sljit_emit_cmp(c, SLJIT_EQUAL | SLJIT_32, SLJIT_R1, 0, rhs, rhsw);
	sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, 4);
	greater_done = sljit_emit_jump(c, SLJIT_JUMP);
	sljit_set_label(less, sljit_emit_label(c));
	sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, 8);
	less_done = sljit_emit_jump(c, SLJIT_JUMP);
	sljit_set_label(equal, sljit_emit_label(c));
	sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, 2);
	sljit_set_label(greater_done, sljit_emit_label(c));
	sljit_set_label(less_done, sljit_emit_label(c));
	sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R0, 0, PPC602_JIT_STATE(xer), SLJIT_IMM, 31);
	sljit_emit_op2(c, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
	sljit_emit_op1(c, SLJIT_MOV_U8, PPC602_JIT_STATE(cr[cr]), SLJIT_R0, 0);
}

static void ppc602_jit_emit_overflow(struct sljit_compiler *c, struct sljit_jump *no_overflow)
{
	struct sljit_jump *done;

	sljit_emit_op2(c, SLJIT_OR32, PPC602_JIT_STATE(xer), PPC602_JIT_STATE(xer), PPC602_JIT_IMMEDIATE(XER_SO | XER_OV));
	done = sljit_emit_jump(c, SLJIT_JUMP);
	sljit_set_label(no_overflow, sljit_emit_label(c));
	sljit_emit_op2(c, SLJIT_AND32, PPC602_JIT_STATE(xer), PPC602_JIT_STATE(xer), PPC602_JIT_IMMEDIATE(~XER_OV));
	sljit_set_label(done, sljit_emit_label(c));
}

static void ppc602_jit_emit_add(struct sljit_compiler *c, UINT32 op)
{
	sljit_emit_op2(c, SLJIT_ADD32 | (OEBIT ? SLJIT_SET_OVERFLOW : 0), SLJIT_R1, 0,
		PPC602_JIT_STATE(r[RA]), PPC602_JIT_STATE(r[RB]));
	if (OEBIT)
		ppc602_jit_emit_overflow(c, sljit_emit_jump(c, SLJIT_NOT_OVERFLOW));
	sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RT]), SLJIT_R1, 0);
	if (RCBIT)
		ppc602_jit_emit_compare(c, SLJIT_SIG_LESS, 0, SLJIT_IMM, 0);
}

static void ppc602_jit_emit_carry_input(struct sljit_compiler *c)
{
	sljit_emit_op2(c, SLJIT_AND32, SLJIT_R2, 0, PPC602_JIT_STATE(xer), PPC602_JIT_IMMEDIATE(XER_CA));
	sljit_emit_op2u(c, SLJIT_ADD32 | SLJIT_SET_CARRY, SLJIT_R2, 0, PPC602_JIT_IMMEDIATE(0U - XER_CA));
}

static void ppc602_jit_emit_carry_output(struct sljit_compiler *c)
{
	sljit_emit_op_flags(c, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_CARRY);
	sljit_emit_op2(c, SLJIT_SHL32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 29);
	sljit_emit_op2(c, SLJIT_AND32, SLJIT_R0, 0, PPC602_JIT_STATE(xer), PPC602_JIT_IMMEDIATE(~XER_CA));
	sljit_emit_op2(c, SLJIT_OR32, PPC602_JIT_STATE(xer), SLJIT_R0, 0, SLJIT_R2, 0);
}

static void ppc602_jit_emit_add_with_carry(struct sljit_compiler *c, UINT32 op)
{
	int extend = ((op >> 1) & 0x1ff) == 138;

	if (extend)
		ppc602_jit_emit_carry_input(c);
	sljit_emit_op2(c, (extend ? SLJIT_ADDC32 : SLJIT_ADD32) | SLJIT_SET_CARRY, SLJIT_R1, 0,
		PPC602_JIT_STATE(r[RA]), PPC602_JIT_STATE(r[RB]));
	ppc602_jit_emit_carry_output(c);
	if (OEBIT)
	{
		/* Overflow requires equal input signs and a different result sign. */
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R0, 0, PPC602_JIT_STATE(r[RA]), PPC602_JIT_STATE(r[RB]));
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, -1);
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R2, 0, PPC602_JIT_STATE(r[RA]), SLJIT_R1, 0);
		sljit_emit_op2(c, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
		ppc602_jit_emit_overflow(c, sljit_emit_cmp(c, SLJIT_SIG_GREATER_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, 0));
	}
	sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RT]), SLJIT_R1, 0);
	if (RCBIT)
		ppc602_jit_emit_compare(c, SLJIT_SIG_LESS, 0, SLJIT_IMM, 0);
}

static void ppc602_jit_emit_subfe(struct sljit_compiler *c, UINT32 op)
{
	sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R0, 0, PPC602_JIT_STATE(r[RA]), SLJIT_IMM, -1);
	ppc602_jit_emit_carry_input(c);
	sljit_emit_op2(c, SLJIT_ADDC32 | SLJIT_SET_CARRY, SLJIT_R1, 0, SLJIT_R0, 0, PPC602_JIT_STATE(r[RB]));
	ppc602_jit_emit_carry_output(c);
	if (OEBIT)
	{
		/* Check the sign bit of (RB ^ RA) & (RB ^ result) before writing RT. */
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R0, 0, PPC602_JIT_STATE(r[RB]), PPC602_JIT_STATE(r[RA]));
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R2, 0, PPC602_JIT_STATE(r[RB]), SLJIT_R1, 0);
		sljit_emit_op2(c, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
		ppc602_jit_emit_overflow(c, sljit_emit_cmp(c, SLJIT_SIG_GREATER_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, 0));
	}
	sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RT]), SLJIT_R1, 0);
	if (RCBIT)
		ppc602_jit_emit_compare(c, SLJIT_SIG_LESS, 0, SLJIT_IMM, 0);
}

static void ppc602_jit_emit_logic(struct sljit_compiler *c, UINT32 op, sljit_s32 operation)
{
	sljit_s32 dst = RCBIT ? SLJIT_R1 : SLJIT_MEM1(SLJIT_S0);
	sljit_sw dstw = RCBIT ? 0 : offsetof(PPC_REGS, r[RA]);

	if (RS == RB)
	{
		if (RCBIT || RA != RS)
			sljit_emit_op1(c, SLJIT_MOV32, dst, dstw, PPC602_JIT_STATE(r[RS]));
	}
	else
		sljit_emit_op2(c, operation, dst, dstw, PPC602_JIT_STATE(r[RS]), PPC602_JIT_STATE(r[RB]));

	if (RCBIT)
	{
		if (RS != RB || RA != RS)
			sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RA]), SLJIT_R1, 0);
		ppc602_jit_emit_compare(c, SLJIT_SIG_LESS, 0, SLJIT_IMM, 0);
	}
}

static UINT32 ppc602_jit_branch_target(UINT32 pc, UINT32 op)
{
	UINT32 offset = (op >> 26) == 16 ? SIMM16 & ~3U : op & 0x03fffffc;

	if (offset & 0x02000000)
		offset |= 0xfc000000;
	return AABIT ? offset : pc + offset;
}

static int ppc602_jit_conditional(UINT32 op)
{
	return (op >> 26) == 16 && (BO & 20) != 20;
}

struct ppc602_jit_branch
{
	struct sljit_jump *ctr_miss, *cr_miss;
};

static struct ppc602_jit_branch ppc602_jit_emit_branch(struct sljit_compiler *c, UINT32 pc, UINT32 op)
{
	struct ppc602_jit_branch branch = { NULL, NULL };

	if (LKBIT)
		sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(lr), PPC602_JIT_IMMEDIATE(pc + 4));

	if (ppc602_jit_conditional(op))
	{
		if (!(BO & 4))
		{
			sljit_emit_op2(c, SLJIT_SUB32, SLJIT_R0, 0, PPC602_JIT_STATE(ctr), SLJIT_IMM, 1);
			sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(ctr), SLJIT_R0, 0);
			branch.ctr_miss = sljit_emit_cmp(c, ((BO & 2) ? SLJIT_NOT_EQUAL : SLJIT_EQUAL) | SLJIT_32,
				SLJIT_R0, 0, SLJIT_IMM, 0);
		}
		if (!(BO & 16))
		{
			sljit_emit_op1(c, SLJIT_MOV_U8, SLJIT_R0, 0, PPC602_JIT_STATE(cr[BI / 4]));
			sljit_emit_op2(c, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 1 << (3 - BI % 4));
			branch.cr_miss = sljit_emit_cmp(c, ((BO & 8) ? SLJIT_EQUAL : SLJIT_NOT_EQUAL) | SLJIT_32,
				SLJIT_R0, 0, SLJIT_IMM, 0);
		}
	}
	return branch;
}

static const UINT8 ppc602_jit_displaced_size[64] =
{
	[32] = 4, [33] = 4, [34] = 1, [36] = 4, [38] = 1,
	[40] = 2, [42] = 2, [44] = 2
};

static unsigned ppc602_jit_memory_size(UINT32 op)
{
	if ((op >> 26) != 31)
		return ppc602_jit_displaced_size[op >> 26];
	switch ((op >> 1) & 0x3ff)
	{
		case 23: return 4; /* lwzx */
		case 87: return 1; /* lbzx */
		default: return 0;
	}
}

struct ppc602_jit_memory_exit
{
	struct sljit_jump *outside, *unaligned, *reserved, *code_write;
};

static int ppc602_jit_store(unsigned major)
{
	return major == 36 || major == 38 || major == 44;
}

static int ppc602_jit_is_mtmsr(UINT32 op)
{
	return (op >> 26) == 31 && ((op >> 1) & 0x3ff) == 146;
}

/* R0 holds the byte offset in the direct region and must survive both checks. */
static struct sljit_jump *ppc602_jit_emit_store_guard(struct sljit_compiler *c, const struct ppc602_direct_region *ram)
{
	unsigned base = ppc602_codemap_index(ram->base);
	unsigned bias = (base << 3) & ((1U << PPC602_JIT_PAGE_BITS) - 1);
	struct sljit_jump *data_page = NULL, *code_write;

	if (ram->size <= (PPC602_CODEMAP_SIZE << 3))
	{
		sljit_s32 source = SLJIT_R0;
		if (bias)
		{
			sljit_emit_op2(c, SLJIT_ADD32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, bias);
			source = SLJIT_R2;
		}
		sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R2, 0, source, 0, SLJIT_IMM, PPC602_JIT_PAGE_BITS);
		sljit_emit_op1(c, SLJIT_MOV_U32, SLJIT_R2, 0, SLJIT_R2, 0);
		sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_IMM,
			(sljit_sw)(ppc602_jit_codepages + (base >> (PPC602_JIT_PAGE_BITS - 3))));
		data_page = sljit_emit_cmp(c, SLJIT_EQUAL | SLJIT_32, SLJIT_MEM2(SLJIT_R1, SLJIT_R2), 2, SLJIT_IMM, 0);
	}

	/* Data can share a page with code. Only a marked word must use the bus handler. */
	sljit_emit_op2(c, SLJIT_LSHR32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 3);
	sljit_emit_op2(c, SLJIT_ADD32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, base);
	sljit_emit_op2(c, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, PPC602_CODEMAP_SIZE - 1);
	sljit_emit_op1(c, SLJIT_MOV_U32, SLJIT_R2, 0, SLJIT_R2, 0);
	sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)ppc602_codemap);
	sljit_emit_op1(c, SLJIT_MOV_U8, SLJIT_R2, 0, SLJIT_MEM2(SLJIT_R1, SLJIT_R2), 0);
	code_write = sljit_emit_cmp(c, SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
	if (data_page)
		sljit_set_label(data_page, sljit_emit_label(c));
	return code_write;
}

static void ppc602_jit_emit_memory(struct sljit_compiler *c, const struct ppc602_direct_region *ram,
	UINT32 op, struct ppc602_jit_memory_exit *exit)
{
	unsigned major = op >> 26;
	unsigned size = ppc602_jit_memory_size(op);
	UINT32 displacement = (UINT32)SIMM16 - ram->start;
	UINT32 lane = size == 4 ? DWORD_XOR_BE(0) : size == 2 ? WORD2_XOR_BE(0) : BYTE8_XOR_BE(0);
	sljit_s32 move = size == 4 ? SLJIT_MOV32 : size == 2 ? SLJIT_MOV_U16 : SLJIT_MOV_U8;

	if (major == 31)
	{
		if (RA)
			sljit_emit_op2(c, SLJIT_ADD32, SLJIT_R0, 0, PPC602_JIT_STATE(r[RA]), PPC602_JIT_STATE(r[RB]));
		else
			sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R0, 0, PPC602_JIT_STATE(r[RB]));
		sljit_emit_op2(c, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, PPC602_JIT_IMMEDIATE(ram->start));
	}
	else if (RA || major == 33)
		sljit_emit_op2(c, SLJIT_ADD32, SLJIT_R0, 0, PPC602_JIT_STATE(r[RA]), PPC602_JIT_IMMEDIATE(displacement));
	else
		sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R0, 0, PPC602_JIT_IMMEDIATE(displacement));

	exit->outside = sljit_emit_cmp(c, SLJIT_GREATER | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, ram->size - size);
	if (size > 1)
	{
		sljit_emit_op2u(c, SLJIT_AND32 | SLJIT_SET_Z, SLJIT_R0, 0, SLJIT_IMM, size - 1);
		exit->unaligned = sljit_emit_jump(c, SLJIT_NOT_ZERO);
	}
	/* WRITE32 in the interpreter also handles the reservation. */
	if (major == 36)
		exit->reserved = sljit_emit_cmp(c, SLJIT_NOT_EQUAL | SLJIT_32, PPC602_JIT_STATE(reserved), SLJIT_IMM, 0);

	if (ppc602_jit_store(major))
		exit->code_write = ppc602_jit_emit_store_guard(c, ram);

	/* Save the guest address before adjusting byte lanes. Commit RA after the load. */
	if (major == 33)
		sljit_emit_op2(c, SLJIT_ADD32, SLJIT_R2, 0, SLJIT_R0, 0, PPC602_JIT_IMMEDIATE(ram->start));
	if (lane)
		sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, lane);
	sljit_emit_op1(c, SLJIT_MOV_U32, SLJIT_R0, 0, SLJIT_R0, 0);
	sljit_emit_op1(c, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)ram->base);

	if (ppc602_jit_store(major))
	{
		sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R2, 0, PPC602_JIT_STATE(r[RS]));
		sljit_emit_op1(c, move, SLJIT_MEM2(SLJIT_R1, SLJIT_R0), 0, SLJIT_R2, 0);
	}
	else
	{
		sljit_emit_op1(c, major == 42 ? SLJIT_MOV32_S16 : move, SLJIT_R1, 0, SLJIT_MEM2(SLJIT_R1, SLJIT_R0), 0);
		sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RT]), SLJIT_R1, 0);
		if (major == 33)
			sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RA]), SLJIT_R2, 0);
	}
}

static int ppc602_jit_supported(UINT32 op, const struct ppc602_direct_region *ram)
{
	unsigned major = op >> 26;

	switch (major)
	{
		case 10: case 11: case 14: case 15: case 16: case 18: case 20: case 21: case 24: case 25:
			return 1;
		case 19:
			return ((op >> 1) & 0x3ff) == 150; /* isync */
		case 31:
			switch ((op >> 1) & 0x3ff)
			{
				case 0: case 10: case 28: case 32: case 83: case 136: case 138: case 146: case 266:
				case 444: case 522: case 598: case 648: case 650: case 778:
					return 1;
			}
			break;
	}
	return ram && ppc602_jit_memory_size(op);
}

static void ppc602_jit_emit(struct sljit_compiler *c, UINT32 op)
{
	unsigned major = op >> 26;
	unsigned xo = (op >> 1) & 0x3ff;

	switch (major)
	{
		case 10: case 11:
			sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R1, 0, PPC602_JIT_STATE(r[RA]));
			ppc602_jit_emit_compare(c, major == 10 ? SLJIT_LESS : SLJIT_SIG_LESS, CRFD,
				PPC602_JIT_IMMEDIATE(major == 10 ? UIMM16 : (UINT32)SIMM16));
			break;
		case 14: case 15:
		{
			UINT32 imm = op >> 26 == 14 ? (UINT32)SIMM16 : UIMM16 << 16;
			if (RA)
				sljit_emit_op2(c, SLJIT_ADD32, PPC602_JIT_STATE(r[RT]), PPC602_JIT_STATE(r[RA]), PPC602_JIT_IMMEDIATE(imm));
			else
				sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RT]), PPC602_JIT_IMMEDIATE(imm));
			break;
		}
		case 19: /* isync has no state changes in the PPC602 core */
			break;
		case 20: case 21:
		{
			UINT32 mask = GET_ROTATE_MASK(MB, ME);

			if (SH)
				sljit_emit_op2(c, SLJIT_ROTL32, SLJIT_R1, 0, PPC602_JIT_STATE(r[RS]), SLJIT_IMM, SH);
			else
				sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R1, 0, PPC602_JIT_STATE(r[RS]));
			if (mask != ~0U)
			{
				sljit_emit_op2(c, SLJIT_AND32, SLJIT_R1, 0, SLJIT_R1, 0, PPC602_JIT_IMMEDIATE(mask));
				if (major == 20)
				{
					sljit_emit_op2(c, SLJIT_AND32, SLJIT_R0, 0, PPC602_JIT_STATE(r[RA]), PPC602_JIT_IMMEDIATE(~mask));
					sljit_emit_op2(c, SLJIT_OR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
				}
			}
			sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RA]), SLJIT_R1, 0);
			if (RCBIT)
				ppc602_jit_emit_compare(c, SLJIT_SIG_LESS, 0, SLJIT_IMM, 0);
			break;
		}
		case 24: case 25:
			if (RS != RA || UIMM16)
			{
				UINT32 imm = op >> 26 == 24 ? UIMM16 : UIMM16 << 16;
				sljit_emit_op2(c, SLJIT_OR32, PPC602_JIT_STATE(r[RA]), PPC602_JIT_STATE(r[RS]), PPC602_JIT_IMMEDIATE(imm));
			}
			break;
		case 31:
			switch (xo)
			{
				case 10: case 138: case 522: case 650:
					ppc602_jit_emit_add_with_carry(c, op);
					break;
				case 28:
					ppc602_jit_emit_logic(c, op, SLJIT_AND32);
					break;
				case 0: case 32:
					sljit_emit_op1(c, SLJIT_MOV32, SLJIT_R1, 0, PPC602_JIT_STATE(r[RA]));
					ppc602_jit_emit_compare(c, xo == 32 ? SLJIT_LESS : SLJIT_SIG_LESS, CRFD, PPC602_JIT_STATE(r[RB]));
					break;
				case 83:
					sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(r[RT]), PPC602_JIT_STATE(msr));
					break;
				case 136: case 648:
					ppc602_jit_emit_subfe(c, op);
					break;
				case 266: case 778:
					ppc602_jit_emit_add(c, op);
					break;
				case 444:
					ppc602_jit_emit_logic(c, op, SLJIT_OR32);
					break;
				case 598: /* sync has no state changes in the PPC602 core */
					break;
			}
			break;
	}
}

static void ppc602_jit_emit_pc(struct sljit_compiler *c, UINT32 pc, UINT32 target)
{
	sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(pc), PPC602_JIT_IMMEDIATE(pc));
	sljit_emit_op1(c, SLJIT_MOV32, PPC602_JIT_STATE(npc), PPC602_JIT_IMMEDIATE(target));
}

static void ppc602_jit_emit_side_exit(struct sljit_compiler *c, UINT32 pc, unsigned prefix)
{
	/* A failed first instruction must preserve the state left by the previous block. */
	if (prefix)
	{
		sljit_emit_op2(c, SLJIT_SUB32, PPC602_JIT_BUDGET, PPC602_JIT_BUDGET, SLJIT_IMM, prefix);
		ppc602_jit_emit_pc(c, pc - 4, pc);
	}
	/* Complement the budget to distinguish a side exit, including when nothing retired. */
	sljit_emit_op2(c, SLJIT_XOR32, SLJIT_R0, 0, PPC602_JIT_BUDGET, SLJIT_IMM, -1);
	sljit_emit_return(c, SLJIT_MOV32, SLJIT_R0, 0);
}

#if JIT_CACHE_STATS
static void ppc602_jit_emit_block_stats(struct sljit_compiler *c, struct ppc602_jit_statistics *stats,
	int readonly, unsigned length)
{
	if (length)
	{
		jit_cache_emit_count(c, &stats->lengths[length]);
		jit_cache_emit_count(c, readonly ? &stats->rom_blocks : &stats->ram_blocks);
	}
}
#endif

struct ppc602_jit_context
{
	struct sljit_compiler *c;
	struct jit_cache *cache;
	const UINT32 *opcodes;
	UINT32 pc;
	unsigned length;
	UINT8 segment[PPC602_JIT_MAX_BLOCK + 1];
	struct sljit_label *label[PPC602_JIT_MAX_BLOCK];
	unsigned forward_count;
	struct
	{
		struct sljit_jump *jump;
		unsigned target;
	} forward[PPC602_JIT_MAX_BLOCK];
#if JIT_CACHE_STATS
	struct ppc602_jit_statistics *stats;
	int readonly;
#endif
};

static void ppc602_jit_emit_transfer(struct ppc602_jit_context *ctx, unsigned from, unsigned cost, UINT32 target, int fallthrough)
{
	struct sljit_compiler *c = ctx->c;
	UINT32 pc = ctx->pc + from * 4;
	unsigned index = (target - ctx->pc) >> 2;
	struct sljit_jump *within_budget = NULL;
	int pc_published = 0;

	sljit_emit_op2(c, SLJIT_SUB32, PPC602_JIT_BUDGET, PPC602_JIT_BUDGET, SLJIT_IMM, cost);
#if JIT_CACHE_STATS
	ppc602_jit_emit_block_stats(c, ctx->stats, ctx->readonly, cost);
#endif
	if (index < ctx->length)
	{
		UINT32 op = ctx->opcodes[index];

		/* A guard on the first instruction can exit before advancing the guest PC. */
		if (ppc602_jit_memory_size(op) || ppc602_jit_is_mtmsr(op))
		{
			ppc602_jit_emit_pc(c, pc, target);
			pc_published = 1;
		}
		/* Even a loop back to this block's entry must fit before the next event. */
		within_budget = sljit_emit_cmp(c, SLJIT_SIG_GREATER_EQUAL | SLJIT_32,
			PPC602_JIT_BUDGET, SLJIT_IMM, ctx->segment[index]);
	}
	if (!pc_published)
		ppc602_jit_emit_pc(c, pc, target);
	if (!within_budget)
		jit_cache_emit_chain(ctx->cache, c, PPC602_JIT_IMMEDIATE(target), PPC602_JIT_BUDGET);
	sljit_emit_return(c, SLJIT_MOV32, PPC602_JIT_BUDGET);
	if (within_budget)
	{
		if (fallthrough)
			sljit_set_label(within_budget, sljit_emit_label(c));
		else if (ctx->label[index])
			sljit_set_label(within_budget, ctx->label[index]);
		else
		{
			ctx->forward[ctx->forward_count].jump = within_budget;
			ctx->forward[ctx->forward_count++].target = index;
		}
	}
}

static void ppc602_jit_compile(struct jit_cache *cache, struct jit_block *block, const struct ppc602_direct_map *map,
	const struct ppc602_direct_region *region, UINT32 pc, UINT32 mode)
{
	struct ppc602_jit_context ctx = { 0 };
	struct sljit_compiler *c;
	struct ppc602_jit_memory_exit exits[PPC602_JIT_MAX_BLOCK] = {{ 0 }};
	struct sljit_jump *mode_mismatch, *msr_changed = NULL;
	struct sljit_label *entry, *leave;
	const struct ppc602_direct_region *ram = NULL;
	UINT32 opcodes[PPC602_JIT_MAX_BLOCK];
	UINT8 prefix[PPC602_JIT_MAX_BLOCK];
	unsigned i, length = 0, boundary;
	unsigned limit = MIN((region->size - (pc - region->start)) >> 2, (0x1000 - (pc & 0xfff)) >> 2);
#if JIT_CACHE_STATS
	struct ppc602_jit_statistics *stats = &ppc602_jit_stats[cache - ppc602_jit_caches];
	int stops_at_opcode = 0;
#endif

	for (i = 0; i < map->count; ++i)
		if (!map->regions[i].readonly)
		{
			ram = &map->regions[i];
			break;
		}

	block->pc = pc;
	block->mode = mode;
	block->valid = 1;

	for (i = 0; i < MIN(limit, PPC602_JIT_MAX_BLOCK); ++i)
	{
		UINT32 op = ppc602_region_opcode(region, pc + i * 4);
		unsigned major = op >> 26;
		opcodes[i] = op;
		if (!ppc602_jit_supported(op, ram))
		{
#if JIT_CACHE_STATS
			stops_at_opcode = 1;
#endif
			break;
		}
		++length;
		if (major == 18 || (major == 16 && !ppc602_jit_conditional(op)) || ppc602_jit_store(major))
			break;
		/* Keep the interrupt check at the MSR write's instruction boundary. */
		if (ppc602_jit_is_mtmsr(op))
			break;
	}
	/* Track negative entries too: a write may make the first instruction translatable. */
	if (!region->readonly)
		for (i = 0; i < MAX(length, 1); ++i)
			ppc602_jit_mark_code(region->base + ((pc - region->start + i * 4) & ~7U));
	if (!length)
		return;

	/* Partition at branch targets and fallthroughs so every entry has a known cost. */
	ctx.segment[0] = ctx.segment[length] = 1;
	for (i = 0; i < length; ++i)
	{
		UINT32 op = opcodes[i];
		if ((op >> 26) == 16 || (op >> 26) == 18)
		{
			unsigned target = (ppc602_jit_branch_target(pc + i * 4, op) - pc) >> 2;
			ctx.segment[i + 1] = 1;
			if (target < length)
				ctx.segment[target] = 1;
		}
	}
	boundary = length;
	for (i = length; i-- > 0; )
		if (ctx.segment[i])
		{
			ctx.segment[i] = boundary - i;
			boundary = i;
		}

	c = sljit_create_compiler(NULL);
	if (!c)
		return;
	ctx.c = c;
	ctx.cache = cache;
	ctx.opcodes = opcodes;
	ctx.pc = pc;
	ctx.length = length;
#if JIT_CACHE_STATS
	ctx.stats = stats;
	ctx.readonly = region->readonly;
#endif
	sljit_emit_enter(c, 0, SLJIT_ARGS2(32, P, 32), 3, 2, 0);
	entry = sljit_emit_label(c);
	/* The shared lookup checks PC and cycles. A target may have been cached in another mode. */
	sljit_emit_op2(c, SLJIT_AND32, SLJIT_R0, 0, PPC602_JIT_STATE(msr), SLJIT_IMM, PPC602_JIT_MODE);
	mode_mismatch = sljit_emit_cmp(c, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_R0, 0, PPC602_JIT_IMMEDIATE(mode));
	boundary = 0;
	for (i = 0; i < length; ++i)
	{
		UINT32 op = opcodes[i];
		UINT32 address = pc + i * 4;
		unsigned major = op >> 26;

		if (ctx.segment[i])
		{
			boundary = i;
			ctx.label[i] = sljit_emit_label(c);
		}
		prefix[i] = i - boundary;
		if (major == 16 || major == 18)
		{
			struct ppc602_jit_branch branch = ppc602_jit_emit_branch(c, address, op);

			ppc602_jit_emit_transfer(&ctx, i, prefix[i] + 1, ppc602_jit_branch_target(address, op), 0);
			if (!branch.ctr_miss && !branch.cr_miss)
				continue;
			leave = sljit_emit_label(c);
			sljit_set_label(branch.ctr_miss, leave);
			sljit_set_label(branch.cr_miss, leave);
		}
		if (ppc602_jit_memory_size(op))
			ppc602_jit_emit_memory(c, ram, op, &exits[i]);
		else if (ppc602_jit_is_mtmsr(op))
			msr_changed = sljit_emit_cmp(c, SLJIT_NOT_EQUAL | SLJIT_32, PPC602_JIT_STATE(msr), PPC602_JIT_STATE(r[RS]));
		else
			ppc602_jit_emit(c, op);
		if (ctx.segment[i + 1])
		{
#if JIT_CACHE_STATS
			if (i + 1 == length && stops_at_opcode)
				jit_cache_emit_count(c, &stats->ending_opcodes[ppc602_jit_opcode_slot(opcodes[length])]);
			if (msr_changed)
				jit_cache_emit_count(c, &stats->mtmsr_native);
#endif
			ppc602_jit_emit_transfer(&ctx, i, prefix[i] + 1, address + 4, 1);
		}
	}
	for (i = 0; i < ctx.forward_count; ++i)
		sljit_set_label(ctx.forward[i].jump, ctx.label[ctx.forward[i].target]);

	sljit_set_label(mode_mismatch, sljit_emit_label(c));
#if JIT_CACHE_STATS
	jit_cache_emit_count(c, &stats->chain_mode_misses);
#endif
	sljit_emit_return(c, SLJIT_MOV32, PPC602_JIT_BUDGET);

	if (msr_changed)
	{
		sljit_set_label(msr_changed, sljit_emit_label(c));
#if JIT_CACHE_STATS
		jit_cache_emit_count(c, &stats->mtmsr_changed_exits);
		ppc602_jit_emit_block_stats(c, stats, region->readonly, prefix[length - 1]);
#endif
		ppc602_jit_emit_side_exit(c, pc + (length - 1) * 4, prefix[length - 1]);
	}
	for (i = 0; i < length; ++i)
		if (exits[i].outside)
		{
#if JIT_CACHE_STATS
			struct sljit_jump *guards[] = { exits[i].outside, exits[i].unaligned, exits[i].reserved, exits[i].code_write };
			unsigned guard;

			for (guard = 0; guard < ARRAY_LENGTH(guards); ++guard)
				if (guards[guard])
				{
					sljit_set_label(guards[guard], sljit_emit_label(c));
					jit_cache_emit_count(c, &stats->memory_exits[guard]);
					ppc602_jit_emit_block_stats(c, stats, region->readonly, prefix[i]);
					ppc602_jit_emit_side_exit(c, pc + i * 4, prefix[i]);
				}
#else
			struct sljit_label *label = sljit_emit_label(c);
			sljit_set_label(exits[i].outside, label);
			sljit_set_label(exits[i].unaligned, label);
			sljit_set_label(exits[i].reserved, label);
			sljit_set_label(exits[i].code_write, label);
			ppc602_jit_emit_side_exit(c, pc + i * 4, prefix[i]);
#endif
		}
	block->code = sljit_generate_code(c, 0, NULL);
	if (block->code)
	{
		block->entry = (void *)sljit_get_label_addr(entry);
		block->len = ctx.segment[0];
	}
	sljit_free_compiler(c);
}

enum ppc602_jit_result
{
	PPC602_JIT_FALLBACK,
	PPC602_JIT_COMPLETE,
	PPC602_JIT_PARTIAL
};

static enum ppc602_jit_result ppc602_jit_run(struct jit_cache *cache, const struct ppc602_direct_map *map)
{
	UINT32 pc = ppc.npc;
	UINT32 mode = MSR & PPC602_JIT_MODE;
	struct jit_block *block;
	int budget, remaining, done, partial;
#if JIT_CACHE_STATS
	struct ppc602_jit_statistics *stats = &ppc602_jit_stats[cache - ppc602_jit_caches];
#endif

	if ((MSR & MSR_EE) && ppc.interrupt_pending)
	{
		PPC602_JIT_COUNT(interrupt);
		return PPC602_JIT_FALLBACK;
	}
	block = jit_cache_find(cache, pc);
	if (block && block->mode != mode)
	{
		jit_cache_invalidate(cache, pc);
		block = NULL;
	}
	if (!block)
	{
		const struct ppc602_direct_region *region = ppc602_opcode_region(map, pc);
		if (!region)
		{
			PPC602_JIT_COUNT(outside_code);
			return PPC602_JIT_FALLBACK;
		}
		block = jit_cache_reserve(cache, pc);
		ppc602_jit_compile(cache, block, map, region, pc, mode);
	}

	if (!block->len)
	{
		PPC602_JIT_COUNT(no_code);
		return PPC602_JIT_FALLBACK;
	}
	if (block->len > ppc_icount)
	{
		PPC602_JIT_COUNT(slice);
		return PPC602_JIT_FALLBACK;
	}
	/* Native code makes no calls and cannot change MSR or raise an interrupt.
	 * A chain may retire exactly up to the decrementer boundary before returning. */
	budget = ppc_dec_trigger_cycle < 0 ? ppc_icount : ppc_icount - ppc_dec_trigger_cycle;
	if (budget < block->len)
	{
		PPC602_JIT_COUNT(decrementer);
		return PPC602_JIT_FALLBACK;
	}

	remaining = ((ppc602_jit_func)block->code)(&ppc, budget);
	partial = remaining < 0;
	done = budget - (partial ? ~remaining : remaining);
	if (!done)
		return PPC602_JIT_FALLBACK;
#if JIT_CACHE_STATS
	stats->native += done;
#endif
	if (ppc.npc != ppc.pc + 4)
		change_pc(ppc.npc);
	ppc_icount -= done;
	return partial ? PPC602_JIT_PARTIAL : PPC602_JIT_COMPLETE;
}

// license:BSD-3-Clause
// copyright-holders: Andrea Mazzoleni
/*
 * Shared block cache for PSX, MIPS III, SH-2, SH-3 and PPC602. Included once per core,
 * after struct jit_block and sljit_mame.h; intentionally no include guard.
 * The core owns each cache instance. Compilation, memory access, codemaps
 * and code budgets stay in the CPU.
 *
 * Configure each CPU independently in its recompiler .c before inclusion:
 *   #define JIT_CACHE_WAYS 1       (1, 2 or 4 ways; default 1)
 *   #define JIT_CACHE_STATS 0      (set to 1 to print statistics to stderr)
 *   #define JIT_CACHE_POLICY JIT_CACHE_POLICY_ROUND_ROBIN
 *     Or JIT_CACHE_POLICY_TREE_PLRU: binary tree pseudo-LRU, with one bit
 *     per internal node (1 bit for 2 ways, 3 bits for 4 ways), stored in a
 *     byte per set. Each bit points towards the older subtree. Hits and
 *     insertions point the bits along their path towards the opposite child.
 *     Both dispatcher and native tag hits update recency, including negative
 *     hits and hits with too few cycles; maintenance probes do not.
 * Changing these source definitions triggers the normal Make dependencies;
 * there are no Make options or configuration stamps for the cache.
 *
 * Required fields: pc, valid, code, len (16 or 32 bits); entry when
 * JIT_CACHE_CHAIN is set. Other block fields are private to the CPU.
 * Required configuration: JIT_TABLE_SIZE (total slots), JIT_CACHE_PC_SHIFT,
 * JIT_CACHE_NAME. Optional: JIT_CACHE_XOR_SHIFT, JIT_CACHE_CHAIN and
 * JIT_CACHE_INVALIDATE (enable individual entry invalidation).
 *
 * More ways keep the total capacity fixed and reduce the number of sets.
 * Full sets use the selected policy; invalid slots are always used first.
 * Block addresses stay stable until shutdown. Generated chains probe tags
 * at runtime, never retain an evictable code pointer. Eviction leaves CPU
 * codemaps conservatively marked; their clearing belongs to the CPU flush.
 * Free code only while no native block is executing. Perform full flushes
 * before initializing a new block: clearing resets all slot metadata.
 *
 * Statistics reset on initialization and survive flushes:
 *   hit_rate: 100 * (hits + chain_hits) / (lookups + chain_lookups).
 *     Includes negative hits, which avoid retrying compilation. This measures
 *     cache reuse, not native execution coverage or emulation speedup.
 *     Printed as n/a when no lookups occurred.
 *   lookups/hits/misses: dispatcher searches; hits + misses == lookups.
 *   negative_hits: cached nontranslatable instructions (len == 0).
 *   insertions: reservations of slots for newly compiled cache entries.
 *   evictions: valid entries replaced because their set was full.
 *   eviction_rate: 100 * evictions / insertions (lower means less pressure).
 *     Compare the same workload and duration across table sizes/way counts.
 *     Counts negative entries too; cannot distinguish capacity from hash
 *     conflicts, and excludes invalidations and full flushes. A low rate
 *     does not prove optimal sizing or account for native code budgets.
 *   invalidations: individual entries explicitly removed (where enabled).
 *   flushes: full clears, including the shutdown clear.
 *   chain_lookups/chain_hits/chain_misses: native searches, counted separately;
 *     chain_hits + chain_misses == chain_lookups.
 *   chain_negative_hits: native hits on nontranslatable instructions.
 *   short_slices: native hits whose cost exceeds the remaining cycle budget.
 * A chain hit does not imply execution: cycles and entry are also checked.
 * SH-3 has no inter-block native chains, so its chain counters stay zero.
 * Maintenance probes do not count as lookups. Disabling statistics removes
 * both the counters and their inline updates in generated code.
 */
#define JIT_CACHE_POLICY_ROUND_ROBIN 0
#define JIT_CACHE_POLICY_TREE_PLRU 1
#ifndef JIT_CACHE_POLICY
#define JIT_CACHE_POLICY JIT_CACHE_POLICY_ROUND_ROBIN
#endif
#if JIT_CACHE_POLICY != JIT_CACHE_POLICY_ROUND_ROBIN && JIT_CACHE_POLICY != JIT_CACHE_POLICY_TREE_PLRU
#error JIT_CACHE_POLICY must be JIT_CACHE_POLICY_ROUND_ROBIN or JIT_CACHE_POLICY_TREE_PLRU
#endif
#ifndef JIT_CACHE_WAYS
#define JIT_CACHE_WAYS 1
#endif
#ifndef JIT_CACHE_STATS
#define JIT_CACHE_STATS 0
#endif
#ifndef JIT_CACHE_CHAIN
#define JIT_CACHE_CHAIN 0
#endif
#ifndef JIT_CACHE_XOR_SHIFT
#define JIT_CACHE_XOR_SHIFT 0
#endif
#if JIT_CACHE_WAYS != 1 && JIT_CACHE_WAYS != 2 && JIT_CACHE_WAYS != 4
#error JIT_CACHE_WAYS must be 1, 2 or 4
#endif
#if JIT_TABLE_SIZE < JIT_CACHE_WAYS || (JIT_TABLE_SIZE & (JIT_TABLE_SIZE - 1))
#error JIT_TABLE_SIZE must be a power of two at least as large as JIT_CACHE_WAYS
#endif
#define JIT_CACHE_SETS (JIT_TABLE_SIZE / JIT_CACHE_WAYS)

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if JIT_CACHE_STATS
#include <stdio.h>
#define JIT_CACHE_COUNT(field) (++cache->stats.field)
#else
#define JIT_CACHE_COUNT(field) ((void)0)
#endif

struct jit_cache
{
	struct jit_block *table;
#if JIT_CACHE_WAYS > 1
#if JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
	UINT8 tree[JIT_CACHE_SETS];
#else
	UINT8 next[JIT_CACHE_SETS];
#endif
#endif
#if JIT_CACHE_STATS
	struct
	{
		UINT64 lookups, hits, misses, negative_hits;
		UINT64 insertions, evictions, invalidations, flushes;
		UINT64 chain_lookups, chain_hits, chain_misses, chain_negative_hits, short_slices;
	} stats;
#endif
};

static inline UINT32 jit_cache_set(UINT32 pc)
{
	UINT32 hash = pc >> JIT_CACHE_PC_SHIFT;
#if JIT_CACHE_XOR_SHIFT
	hash ^= pc >> JIT_CACHE_XOR_SHIFT;
#endif
	return hash & (JIT_CACHE_SETS - 1);
}

#if JIT_CACHE_WAYS > 1 && JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
/* Heap-order bits: root 0, left child 1, right child 2.
 * Setting a path bit to the opposite child marks the accessed way recent. */
static inline UINT8 jit_cache_tree_mask(int way)
{
#if JIT_CACHE_WAYS == 2
	(void)way;
	return 1;
#else
	return way < 2 ? 3 : 5;
#endif
}

static inline UINT8 jit_cache_tree_bits(int way)
{
#if JIT_CACHE_WAYS == 2
	return way ^ 1;
#else
	return (way < 2 ? 1 : 0) | ((way & 1) ? 0 : (way < 2 ? 2 : 4));
#endif
}

static inline void jit_cache_touch(struct jit_cache *cache, UINT32 index, int way)
{
	cache->tree[index] = (cache->tree[index] & ~jit_cache_tree_mask(way)) | jit_cache_tree_bits(way);
}

static inline int jit_cache_victim(struct jit_cache *cache, UINT32 index)
{
	UINT8 tree = cache->tree[index];
#if JIT_CACHE_WAYS == 2
	return tree & 1;
#else
	int right = tree & 1;
	return (right << 1) | ((tree >> (right + 1)) & 1);
#endif
}
#endif

/* Uncounted probe for invalidation and other maintenance. */
static inline struct jit_block *jit_cache_probe(struct jit_cache *cache, UINT32 pc)
{
	struct jit_block *set = &cache->table[jit_cache_set(pc) * JIT_CACHE_WAYS];
	int way;
	for (way = 0; way < JIT_CACHE_WAYS; way++)
		if (set[way].valid && set[way].pc == pc)
			return &set[way];
	return NULL;
}

static inline struct jit_block *jit_cache_find(struct jit_cache *cache, UINT32 pc)
{
	struct jit_block *block = jit_cache_probe(cache, pc);
	JIT_CACHE_COUNT(lookups);
	if (block)
	{
#if JIT_CACHE_WAYS > 1 && JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
		UINT32 index = jit_cache_set(pc);
		jit_cache_touch(cache, index, (int)(block - &cache->table[index * JIT_CACHE_WAYS]));
#endif
		JIT_CACHE_COUNT(hits);
		if (!block->len)
			JIT_CACHE_COUNT(negative_hits);
	}
	else
		JIT_CACHE_COUNT(misses);
	return block;
}

static void jit_cache_drop(struct jit_block *block)
{
	if (block->code)
		sljit_free_code(block->code, NULL);
	memset(block, 0, sizeof(*block));
}

/* Reserve only after the CPU has decided that compilation is allowed.
 * No compiled block may be executing while a slot is dropped or reused. */
static inline struct jit_block *jit_cache_reserve(struct jit_cache *cache, UINT32 pc)
{
	UINT32 index = jit_cache_set(pc);
	struct jit_block *set = &cache->table[index * JIT_CACHE_WAYS];
	int way;
	JIT_CACHE_COUNT(insertions);
	for (way = 0; way < JIT_CACHE_WAYS; way++)
		if (!set[way].valid)
			break;
	if (way == JIT_CACHE_WAYS)
	{
#if JIT_CACHE_WAYS > 1
#if JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
		way = jit_cache_victim(cache, index);
#else
		way = cache->next[index];
		cache->next[index] = (way + 1) % JIT_CACHE_WAYS;
#endif
#else
		way = 0;
#endif
		JIT_CACHE_COUNT(evictions);
	}
	jit_cache_drop(&set[way]);
#if JIT_CACHE_WAYS > 1 && JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
	jit_cache_touch(cache, index, way);
#endif
	return &set[way];
}

static void jit_cache_clear(struct jit_cache *cache)
{
	int i;
	if (!cache->table)
		return;
	for (i = 0; i < JIT_TABLE_SIZE; i++)
		jit_cache_drop(&cache->table[i]);
#if JIT_CACHE_WAYS > 1
#if JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
	memset(cache->tree, 0, sizeof(cache->tree));
#else
	memset(cache->next, 0, sizeof(cache->next));
#endif
#endif
	JIT_CACHE_COUNT(flushes);
}

#if defined(JIT_CACHE_INVALIDATE) && JIT_CACHE_INVALIDATE
static void jit_cache_invalidate(struct jit_cache *cache, UINT32 pc)
{
	struct jit_block *block = jit_cache_probe(cache, pc);
	if (block)
	{
		jit_cache_drop(block);
		JIT_CACHE_COUNT(invalidations);
	}
}
#endif

static void jit_cache_init(struct jit_cache *cache)
{
	cache->table = calloc(JIT_TABLE_SIZE, sizeof(*cache->table));
#if JIT_CACHE_WAYS > 1
#if JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
	memset(cache->tree, 0, sizeof(cache->tree));
#else
	memset(cache->next, 0, sizeof(cache->next));
#endif
#endif
#if JIT_CACHE_STATS
	memset(&cache->stats, 0, sizeof(cache->stats));
#endif
}

static void jit_cache_exit(struct jit_cache *cache)
{
#if JIT_CACHE_STATS
	if (cache->table)
	{
		double lookups = (double)cache->stats.lookups + (double)cache->stats.chain_lookups;
		double hits = (double)cache->stats.hits + (double)cache->stats.chain_hits;
#define JIT_CACHE_PRINT(field) fprintf(stderr, " " #field "=%llu", (unsigned long long)cache->stats.field)
		fprintf(stderr, "JIT cache %s: slots=%u ways=%u xor_shift=%u", JIT_CACHE_NAME, (unsigned)JIT_TABLE_SIZE, (unsigned)JIT_CACHE_WAYS, (unsigned)JIT_CACHE_XOR_SHIFT);
		fprintf(stderr, " policy=%s", JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU ? "tree-plru" : "round-robin");
		if (lookups > 0)
			fprintf(stderr, " hit_rate=%.2f%%", 100.0 * hits / lookups);
		else
			fprintf(stderr, " hit_rate=n/a");
		if (cache->stats.insertions)
			fprintf(stderr, " eviction_rate=%.2f%%", 100.0 * (double)cache->stats.evictions / (double)cache->stats.insertions);
		else
			fprintf(stderr, " eviction_rate=n/a");
		JIT_CACHE_PRINT(lookups); JIT_CACHE_PRINT(hits); JIT_CACHE_PRINT(misses); JIT_CACHE_PRINT(negative_hits);
		JIT_CACHE_PRINT(insertions); JIT_CACHE_PRINT(evictions); JIT_CACHE_PRINT(invalidations); JIT_CACHE_PRINT(flushes);
		JIT_CACHE_PRINT(chain_lookups); JIT_CACHE_PRINT(chain_hits); JIT_CACHE_PRINT(chain_misses);
		JIT_CACHE_PRINT(chain_negative_hits); JIT_CACHE_PRINT(short_slices);
		fprintf(stderr, "\n");
#undef JIT_CACHE_PRINT
	}
#endif
	/* The CPU flushes first, including its codemap and other side tables. */
	free(cache->table);
	cache->table = NULL;
}

#if JIT_CACHE_CHAIN
#if JIT_CACHE_STATS
static void jit_cache_emit_count(struct sljit_compiler *C, UINT64 *counter)
{
#if defined(SLJIT_64BIT_ARCHITECTURE) && SLJIT_64BIT_ARCHITECTURE
	sljit_emit_op2(C, SLJIT_ADD, SLJIT_MEM0(), (sljit_sw)counter, SLJIT_MEM0(), (sljit_sw)counter, SLJIT_IMM, 1);
#else
	/* The supported sljit hosts are little endian. Keep 64-bit counters. */
	sljit_emit_op2(C, SLJIT_ADD32 | SLJIT_SET_CARRY, SLJIT_MEM0(), (sljit_sw)counter, SLJIT_MEM0(), (sljit_sw)counter, SLJIT_IMM, 1);
	sljit_emit_op2(C, SLJIT_ADDC32, SLJIT_MEM0(), (sljit_sw)counter + 4, SLJIT_MEM0(), (sljit_sw)counter + 4, SLJIT_IMM, 0);
#endif
}
#define JIT_CACHE_EMIT_COUNT(field) jit_cache_emit_count(C, &cache->stats.field)
#else
#define JIT_CACHE_EMIT_COUNT(field) ((void)0)
#endif

/* Emit a complete associative chain lookup. Clobbers R1/R2, preserves
 * src (R0, a saved register, or an immediate) and the saved cycle register.
 * Tree-PLRU with multiple ways also clobbers R3 (four scratch registers
 * required), holding the set's tree pointer across the way probes.
 * All blocks must share a prologue; entry points follow that prologue. */
static void jit_cache_emit_chain(struct jit_cache *cache, struct sljit_compiler *C, sljit_s32 src, sljit_sw srcw,
	sljit_s32 cycles, sljit_sw cyclesw)
{
	struct sljit_jump *done[JIT_CACHE_WAYS * 3];
	struct sljit_jump *next[2], *found;
	struct sljit_label *out;
	int way, i, ndone = 0;

	if (src == SLJIT_IMM && ((UINT32)srcw & ((1 << JIT_CACHE_PC_SHIFT) - 1)))
		return;
	JIT_CACHE_EMIT_COUNT(chain_lookups);
	if (src == SLJIT_IMM)
	{
		sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_IMM,
			(sljit_sw)&cache->table[jit_cache_set((UINT32)srcw) * JIT_CACHE_WAYS]);
#if JIT_CACHE_WAYS > 1 && JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
		sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R3, 0, SLJIT_IMM,
			(sljit_sw)&cache->tree[jit_cache_set((UINT32)srcw)]);
#endif
	}
	else
	{
		sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R1, 0, src, srcw, SLJIT_IMM, JIT_CACHE_PC_SHIFT);
#if JIT_CACHE_XOR_SHIFT
		sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R2, 0, src, srcw, SLJIT_IMM, JIT_CACHE_XOR_SHIFT);
		sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);
#endif
		sljit_emit_op2(C, SLJIT_AND32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, JIT_CACHE_SETS - 1);
		sljit_emit_op1(C, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_R1, 0);
#if JIT_CACHE_WAYS > 1 && JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
		sljit_emit_op2(C, SLJIT_ADD, SLJIT_R3, 0, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)cache->tree);
#endif
		sljit_emit_op2(C, SLJIT_MUL, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, JIT_CACHE_WAYS * sizeof(struct jit_block));
		sljit_emit_op2(C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, (sljit_sw)cache->table);
	}
	for (way = 0; way < JIT_CACHE_WAYS; way++)
	{
		sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_R1), offsetof(struct jit_block, valid));
		next[0] = sljit_emit_cmp(C, SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
		next[1] = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_MEM1(SLJIT_R1), offsetof(struct jit_block, pc), src, srcw);
		JIT_CACHE_EMIT_COUNT(chain_hits);
#if JIT_CACHE_WAYS > 1 && JIT_CACHE_POLICY == JIT_CACHE_POLICY_TREE_PLRU
#if JIT_CACHE_WAYS == 2
		sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_MEM1(SLJIT_R3), 0, SLJIT_IMM, jit_cache_tree_bits(way));
#else
		sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_R3), 0);
		sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 7 & ~jit_cache_tree_mask(way));
		sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, jit_cache_tree_bits(way));
		sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_MEM1(SLJIT_R3), 0, SLJIT_R2, 0);
#endif
#endif
		sljit_emit_op1(C, sizeof(cache->table[0].len) == 2 ? SLJIT_MOV32_U16 : SLJIT_MOV32,
			SLJIT_R2, 0, SLJIT_MEM1(SLJIT_R1), offsetof(struct jit_block, len));
		found = sljit_emit_cmp(C, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_R2, 0, SLJIT_IMM, 0);
		JIT_CACHE_EMIT_COUNT(chain_negative_hits);
		done[ndone++] = sljit_emit_jump(C, SLJIT_JUMP);
		sljit_set_label(found, sljit_emit_label(C));
		found = sljit_emit_cmp(C, SLJIT_SIG_LESS_EQUAL | SLJIT_32, SLJIT_R2, 0, cycles, cyclesw);
		JIT_CACHE_EMIT_COUNT(short_slices);
		done[ndone++] = sljit_emit_jump(C, SLJIT_JUMP);
		sljit_set_label(found, sljit_emit_label(C));
		sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_R1), offsetof(struct jit_block, entry));
		done[ndone++] = sljit_emit_cmp(C, SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
		sljit_emit_ijump(C, SLJIT_JUMP, SLJIT_R2, 0);
		out = sljit_emit_label(C);
		for (i = 0; i < 2; i++)
			sljit_set_label(next[i], out);
		if (way + 1 < JIT_CACHE_WAYS)
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, sizeof(struct jit_block));
	}
	JIT_CACHE_EMIT_COUNT(chain_misses);
	out = sljit_emit_label(C);
	for (i = 0; i < ndone; i++)
		sljit_set_label(done[i], out);
}
#endif

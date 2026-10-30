// license:GPL-2.0-only
// copyright-holders:FinalDave, Gražvydas Ignotas
/*
 * AdvanceMAME port of PSP-MAME4ALL's Cyclone adapter.
 *
 * Musashi remains authoritative at timeslice boundaries. Cyclone executes
 * only 68000 instructions on compatible 32-bit ARM targets.
 */
#include "driver.h"
#include "../m68000/m68kcpu.h"
#include "cyclone.h"
#include <stddef.h>

#define LIKELY(a) a

#if !defined(__arm__) || defined(__ARMEB__) || defined(__aarch64__) || \
    !defined(__ARM_ARCH) || __ARM_ARCH < 7 || \
    !defined(__ARM_ARCH_PROFILE) || __ARM_ARCH_PROFILE != 'A'
#error Cyclone requires 32-bit little-endian ARMv7-A or newer
#endif
typedef char cyclone_abi_layout[(sizeof(void *) == 4 &&
	offsetof(struct Cyclone, internal) == 0xa4 &&
	sizeof(struct Cyclone) == 0xb0) ? 1 : -1];

extern int m68ki_initial_cycles;

static struct Cyclone cyclone;
static int running;
static int running_cpu;
static int pending_nmi;
static unsigned int irq_pin_level;
static UINT32 opcode_generation;

/* Driver-declared, immutable direct ranges, analogous to SH3 fastram.
 * These are independent of the opcode-region cache. They must never
 * overlap an address which may be handled by a device or dynamic overlay.
 */
#define CYCLONE_FASTRAM_MAX 4
struct cyclone_fastram {
    int cpu;
    unsigned int start, end, mask;
    int readonly;
    UINT8 *base;
};
static struct cyclone_fastram fastram[CYCLONE_FASTRAM_MAX];
static int fastram_count;
static int fastram_last;

/* Called by the driver during machine initialization/reset, not from
 * an emulated memory handler. Other CPU cores are unaffected.
 */
void cyclone_mame_clear_fastram(int cpunum)
{
    int i, j = 0;
    for (i = 0; i < fastram_count; ++i)
        if (fastram[i].cpu != cpunum)
            fastram[j++] = fastram[i];
    fastram_count = j;
    fastram_last = 0;
}

void cyclone_mame_set_fastram(int cpunum, unsigned int start,
    unsigned int end, unsigned int mask, int readonly, void *base)
{
    struct cyclone_fastram *f;
    if (!base || end < start || cpunum < 0 || cpunum >= MAX_CPU ||
        fastram_count >= CYCLONE_FASTRAM_MAX)
        return;
    f = &fastram[fastram_count++];
    f->cpu = cpunum;
    f->start = start;
    f->end = end;
    f->mask = mask;
    f->readonly = readonly;
    f->base = (UINT8 *)base;
}

INLINE struct cyclone_fastram *find_fastram(unsigned int address, int write)
{
    int i;
    struct cyclone_fastram *f;
    if (fastram_last < fastram_count) {
        f = &fastram[fastram_last];
        if (f->cpu == running_cpu && address >= f->start &&
            address <= f->end && (!write || !f->readonly))
            return f;
    }
    for (i = 0; i < fastram_count; ++i) {
        f = &fastram[i];
        if (f->cpu == running_cpu && address >= f->start &&
            address <= f->end && (!write || !f->readonly)) {
            fastram_last = i;
            return f;
        }
    }
    return NULL;
}

#ifdef CYCLONE_TEST_STATS
unsigned int cyclone_test_exports;
#endif
static struct m68k_memory_interface default_memory;
static void (*default_reset)(void);
static void (*default_cmpild)(unsigned int, int);
static void (*default_rte)(void);

/* Cache only an interval that is safe for both instruction and extension
 * fetches. Memory generation changes on bank/map/context mutations.
 * The cache is deliberately small: no per-opcode/region allocation.
 */
struct cyclone_code_cache {
	UINT32 generation;
	int cpu;
	unsigned int first, last;
	unsigned int host_first, host_last;
	unsigned int membase;
	UINT8 entry;
	UINT8 *base, *arg_base;
	int valid;
};
static struct cyclone_code_cache code_cache;

static unsigned int checkpc(unsigned int pc);
static unsigned int read8(unsigned int address);
static unsigned int read16(unsigned int address);
static unsigned int read32(unsigned int address);
static void write8(unsigned int address, unsigned char data);
static void write16(unsigned int address, unsigned short data);
static void write32(unsigned int address, unsigned int data);
static int irq_callback(int level);

void cyclone_mame_init(void)
{
	CycloneInit();
	default_memory = m68k_memory_intf;
	default_reset = CALLBACK_RESET_INSTR;
	default_cmpild = CALLBACK_CMPILD_INSTR;
	default_rte = CALLBACK_RTE_INSTR;

	/* These fields are independent of the active CPU's architectural state. */
	memset(&cyclone, 0, sizeof(cyclone));
	cyclone.checkpc = checkpc;
	cyclone.read8 = cyclone.fetch8 = read8;
	cyclone.read16 = cyclone.fetch16 = read16;
	cyclone.read32 = cyclone.fetch32 = read32;
	cyclone.write8 = write8;
	cyclone.write16 = write16;
	cyclone.write32 = write32;
	cyclone.IrqCallback = irq_callback;
	cyclone.jumptab = (unsigned int)CycloneJumpTab;
	code_cache.valid = 0;
}

/* Called only when Cyclone is not currently running or when it is changing
 * its instruction address. In particular, a successful cache hit must not
 * call memory_set_opbase(), since that changes the generation itself.
 */
static int cached_pc(unsigned int pc)
{
	if (!code_cache.valid || memory_cyclone_custom_opbase ||
		code_cache.cpu != cpu_getactivecpu() ||
		code_cache.generation != memory_opcode_generation ||
		pc < code_cache.first || pc > code_cache.last ||
		opcode_entry != code_cache.entry ||
		opcode_base != code_cache.base ||
		opcode_arg_base != code_cache.arg_base)
		return 0;

	cyclone.membase = code_cache.membase;
	cyclone.internal[0] = code_cache.host_first;
	cyclone.internal[1] = code_cache.host_last;
	return 1;
}

static unsigned int checkpc(unsigned int pc)
{
	offs_t first, last, span, map_first, map_last;
	address_space *space = &active_address_space[ADDRESS_SPACE_PROGRAM];
	UINT8 *lookup = space->readlookup;
	UINT8 entry;
	pc = (pc - cyclone.membase) & 0xffffff;

	if (cached_pc(pc)) {
		if (cyclone.internal[2]) {
			cyclone.internal[0] = 1;
			cyclone.internal[1] = 0;
		}
		return cyclone.membase + pc;
	}

	/* A miss requires full validation, including level-2 maps and opbase
	 * callbacks. Do not use change_pc(), which only compares level 1.
	 */
	memory_set_opbase(pc);
	cyclone.membase = (unsigned int)cpu_opptr(pc) - pc;
	span = (opcode_mask ^ (opcode_mask + 1)) >> 1;
	first = MAX(opcode_memory_min, pc & ~span);
	last = MIN(opcode_memory_max, pc | span);

	entry = lookup[LEVEL1_INDEX(pc & space->addrmask)];
	if (entry >= SUBTABLE_BASE)
	{
		map_first = map_last = pc;
		while (map_first > first && (map_first & ((1 << LEVEL2_BITS) - 1)) &&
			lookup[LEVEL2_INDEX(entry, map_first - 1)] == opcode_entry)
			map_first--;
		while (map_last < last && ((map_last + 1) & ((1 << LEVEL2_BITS) - 1)) &&
			lookup[LEVEL2_INDEX(entry, map_last + 1)] == opcode_entry)
			map_last++;
		entry = lookup[LEVEL2_INDEX(entry, pc)];
	}
	else
	{
		map_first = pc & ~((1 << LEVEL2_BITS) - 1);
		map_last = pc | ((1 << LEVEL2_BITS) - 1);
		while (map_first > first && lookup[LEVEL1_INDEX(map_first - 1)] == entry)
			map_first -= 1 << LEVEL2_BITS;
		while (map_last < last && lookup[LEVEL1_INDEX(map_last + 1)] == entry)
			map_last += 1 << LEVEL2_BITS;
	}
	first = MAX(first, map_first);
	last = MIN(last, map_last);

	/* The original bound reserves the final 16 bytes for instructions and
	 * speculative fetches. Never extend a cache beyond these limits.
	 */
	cyclone.internal[0] = cyclone.membase + first;
	cyclone.internal[1] = cyclone.membase + last - 16;
	if (entry != opcode_entry || last < first || last - first < 16 ||
		opcode_base != opcode_arg_base || address_is_unsafe(pc) ||
		cyclone.internal[2])
	{
		cyclone.internal[0] = 1;
		cyclone.internal[1] = 0;
	}

	/* Save only validated, usable intervals. An unsafe mapping is handled
	 * by Musashi and must not be promoted to a cached fast path.
	 */
	code_cache.valid = 0;
	if (cyclone.internal[0] <= cyclone.internal[1])
	{
		code_cache.cpu = cpu_getactivecpu();
		code_cache.generation = memory_opcode_generation;
		code_cache.first = first;
		code_cache.last = last - 16;
		code_cache.host_first = cyclone.internal[0];
		code_cache.host_last = cyclone.internal[1];
		code_cache.membase = cyclone.membase;
		code_cache.entry = opcode_entry;
		code_cache.base = opcode_base;
		code_cache.arg_base = opcode_arg_base;
		code_cache.valid = 1;
	}
	opcode_generation = memory_opcode_generation;
	return cyclone.membase + pc;
}

static void export_state(void)
{
#ifdef CYCLONE_TEST_STATS
	cyclone_test_exports++;
#endif
	memcpy(REG_D, cyclone.d, sizeof(cyclone.d));
	memcpy(REG_A, cyclone.a, sizeof(cyclone.a));
	REG_PC = (cyclone.pc - cyclone.membase) & 0xffffff;
	REG_PPC = (cyclone.prev_pc - cyclone.membase - 2) & 0xffffff;
	m68ki_set_sr_noint_nosp(CycloneGetSr(&cyclone));
	if (FLAG_S)
		REG_USP = cyclone.osp;
	else
		REG_ISP = cyclone.osp;
	CPU_STOPPED = ((cyclone.state_flags & 1) ? STOP_LEVEL_STOP : 0)
		| ((cyclone.state_flags & 0x10) ? STOP_LEVEL_HALT : 0);
	CPU_INT_LEVEL = cyclone.irq << 8;
	m68k_ICount = cyclone.cycles;
}

/* Ordinary accesses retain MAME's accessor semantics. Driver-declared
 * fast RAM is handled directly before reaching these callbacks.
 * No private memory.c handler_data or bank_ptr structures are needed.
 */
INLINE int has_handler(unsigned int address, int write)
{
    address_space *space = &active_address_space[ADDRESS_SPACE_PROGRAM];
    UINT8 *lookup = write ? space->writelookup : space->readlookup;
    UINT8 entry;
    address &= space->addrmask;
    entry = lookup[LEVEL1_INDEX(address)];
    if (entry >= SUBTABLE_BASE)
        entry = lookup[LEVEL2_INDEX(entry, address)];
    return entry >= STATIC_RAM;
}

INLINE void begin_memory(int handler)
{
    if (handler)
        export_state();
    else
        m68k_ICount = cyclone.cycles;
}

/* This runs after a driver callback, never after an ordinary direct access.
 * A callback may have changed the PC mapping without changing logical PC.
 */
INLINE void finish_memory(void)
{
	cyclone.cycles = m68k_ICount;
	if (UNEXPECTED(opcode_generation != memory_opcode_generation))
	{
		unsigned int pc = cyclone.pc;
		unsigned int previous = cyclone.prev_pc - cyclone.membase;
		cyclone.pc = checkpc(pc);
		cyclone.prev_pc = cyclone.membase + previous;
	}
}

static unsigned int read8(unsigned int address)
{
    struct cyclone_fastram *f;
    unsigned int result;
    address &= 0xffffff;
    f = find_fastram(address, 0);
    if (LIKELY(f != NULL))
        return f->base[BYTE_XOR_BE((address - f->start) & f->mask)];
    begin_memory(has_handler(address, 0));
    result = program_read_byte_16be(address);
    finish_memory();
    return result;
}

static unsigned int read16(unsigned int address)
{
    struct cyclone_fastram *f;
    unsigned int result;
    address = (address & 0xffffff) & ~1U;
    f = find_fastram(address, 0);
    if (LIKELY(f != NULL))
        return *(UINT16 *)&f->base[(address - f->start) & f->mask];
    begin_memory(has_handler(address, 0));
    result = program_read_word_16be(address);
    finish_memory();
    return result;
}

static unsigned int read32(unsigned int address)
{
    unsigned int first, second, high, low;
    struct cyclone_fastram *f, *f2;
    first = (address & 0xffffff) & ~1U;
    second = (first + 2) & 0xffffff;
    f = find_fastram(first, 0);
    f2 = find_fastram(second, 0);
    if (LIKELY(f != NULL && f2 != NULL)) {
        high = *(UINT16 *)&f->base[(first - f->start) & f->mask];
        low = *(UINT16 *)&f2->base[(second - f2->start) & f2->mask];
        return (high << 16) | low;
    }
    begin_memory(has_handler(first, 0) || has_handler(second, 0));
    high = program_read_word_16be(first);
    low = program_read_word_16be(second);
    finish_memory();
    return (high << 16) | low;
}

static void write8(unsigned int address, unsigned char data)
{
    struct cyclone_fastram *f;
    address &= 0xffffff;
    f = find_fastram(address, 1);
    if (LIKELY(f != NULL)) {
        f->base[BYTE_XOR_BE((address - f->start) & f->mask)] = data;
        return;
    }
    begin_memory(has_handler(address, 1));
    program_write_byte_16be(address, data);
    finish_memory();
}

static void write16(unsigned int address, unsigned short data)
{
    struct cyclone_fastram *f;
    address = (address & 0xffffff) & ~1U;
    f = find_fastram(address, 1);
    if (LIKELY(f != NULL)) {
        *(UINT16 *)&f->base[(address - f->start) & f->mask] = data;
        return;
    }
    begin_memory(has_handler(address, 1));
    program_write_word_16be(address, data);
    finish_memory();
}

static void write32(unsigned int address, unsigned int data)
{
    unsigned int first, second;
    struct cyclone_fastram *f, *f2;
    first = (address & 0xffffff) & ~1U;
    second = (first + 2) & 0xffffff;
    f = find_fastram(first, 1);
    f2 = find_fastram(second, 1);
    if (LIKELY(f != NULL && f2 != NULL)) {
        *(UINT16 *)&f->base[(first - f->start) & f->mask] = data >> 16;
        *(UINT16 *)&f2->base[(second - f2->start) & f2->mask] = data;
        return;
    }
    begin_memory(has_handler(first, 1) || has_handler(second, 1));
    program_write_word_16be(first, data >> 16);
    program_write_word_16be(second, data);
    finish_memory();
}

static int irq_callback(int level)
{
	int vector;
	export_state();
	vector = CALLBACK_INT_ACK(level);
	cyclone.cycles = m68k_ICount;
	return vector;
}

int cyclone_mame_set_irq(unsigned int level)
{
	if (!running || cpu_getactivecpu() != running_cpu)
		return 0;

	/* Level 7 is edge-triggered. Cyclone cannot service that edge without
	 * treating the pin as permanently asserted, so use Musashi at the next
	 * instruction boundary, but keep the actual pin level separately.
	 */
	if (level == 7 && irq_pin_level != 7)
	{
		pending_nmi = 1;
		cyclone.internal[2] = 1;
		cyclone.internal[0] = 1;
		cyclone.internal[1] = 0;
	}
	irq_pin_level = level;
	cyclone.irq = pending_nmi ? 0 : level;
	CPU_INT_LEVEL = cyclone.irq << 8;
	return 1;
}

int cyclone_mame_execute(int cycles)
{
#ifdef MAME_DEBUG
	return m68k_execute(cycles);
#else
	int remaining, consumed;
	unsigned int pc = REG_PC & 0xffffff;

	if ((CPU_STOPPED & STOP_LEVEL_HALT) || CPU_INT_LEVEL == 0x700)
		return m68k_execute(cycles);

	/* The previous timeslice may have left a nonzero host base. checkpc()
	 * expects a rebased PC; start from the logical address instead.
	 * A cache hit avoids the unconditional memory_set_opbase() entirely.
	 */
	cyclone.membase = 0;
	cyclone.internal[2] = 0;
	cyclone.pc = checkpc(pc);

	if (CALLBACK_RESET_INSTR != default_reset ||
		CALLBACK_CMPILD_INSTR != default_cmpild ||
		CALLBACK_RTE_INSTR != default_rte ||
		opcode_base != opcode_arg_base ||
		m68k_encrypted_opcode_start[cpu_getactivecpu()] !=
		m68k_encrypted_opcode_end[cpu_getactivecpu()] ||
		memcmp(&m68k_memory_intf, &default_memory, sizeof(default_memory)))
		return m68k_execute(cycles);

	/* Refresh every architectural field and transient execution flag.
	 * The immutable callback table was initialized only once.
	 */
	memcpy(cyclone.d, REG_D, sizeof(cyclone.d));
	memcpy(cyclone.a, REG_A, sizeof(cyclone.a));
	CycloneSetSr(&cyclone, m68ki_get_sr());
	cyclone.osp = FLAG_S ? REG_USP : REG_ISP;
	cyclone.prev_pc = cyclone.membase + REG_PPC + 2;
	cyclone.irq = CPU_INT_LEVEL >> 8;
	cyclone.state_flags = ((CPU_STOPPED & STOP_LEVEL_STOP) ? 1 : 0)
		| ((cyclone.srh & 0x80) ? 2 : 0)
		| ((CPU_STOPPED & STOP_LEVEL_HALT) ? 0x10 : 0);
	cyclone.cycles = cycles - CPU_INT_CYCLES;
	CPU_INT_CYCLES = 0;
	m68ki_initial_cycles = cycles;
	pending_nmi = 0;
	irq_pin_level = CPU_INT_LEVEL >> 8;
	running_cpu = cpu_getactivecpu();
	running = 1;
	CycloneRun(&cyclone);
	running = 0;
	export_state();
	REG_PPC = REG_PC;
	CPU_PREF_ADDR = ~0U;
	remaining = m68k_ICount;
	consumed = m68ki_initial_cycles - remaining;

	if (pending_nmi)
	{
		/* Deliver exactly the latched edge, then restore the actual pin. */
		m68k_set_irq(7);
		if (irq_pin_level != 7)
			m68k_set_irq(irq_pin_level);
	}

	if (cyclone.internal[2] && remaining > 0)
		consumed += m68k_execute(remaining);

	m68ki_initial_cycles = consumed + m68k_ICount;
	return consumed;
#endif
}

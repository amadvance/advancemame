// license:GPL-2.0-only
// copyright-holders:FinalDave, Gražvydas Ignotas
/*
 * AdvanceMAME port of PSP-MAME4ALL's Cyclone adapter.
 *
 * Musashi's context is the architectural state at timeslice boundaries. This
 * preserves the existing register interface, reset sequence, context family,
 * and save-state registrations (no host pointers are serialized). Cyclone is
 * an execution backend for the 68000 only. Special Musashi instruction hooks,
 * encrypted instruction streams and debugger builds retain Musashi execution.
 */
#include "driver.h"
#include "../m68000/m68kcpu.h"
#include "cyclone.h"
#include <stddef.h>

#if !defined(__arm__) || defined(__ARMEB__) || defined(__aarch64__)
#error Cyclone requires a 32-bit little-endian ARM target
#endif
typedef char cyclone_abi_layout[(sizeof(void *) == 4 &&
	offsetof(struct Cyclone, internal) == 0xa4 &&
	sizeof(struct Cyclone) == 0xb0) ? 1 : -1];

extern int m68ki_initial_cycles;

static struct Cyclone cyclone;
static int running;
static int pending_nmi;
static UINT32 opcode_generation;
#ifdef CYCLONE_TEST_STATS
unsigned int cyclone_test_exports;
#endif
static int running_cpu;
static struct m68k_memory_interface default_memory;
static void (*default_reset)(void);
static void (*default_cmpild)(unsigned int, int);
static void (*default_rte)(void);

/* Exported only to the Musashi/MAME glue, never to drivers. */
void cyclone_mame_init(void)
{
	CycloneInit();
	default_memory = m68k_memory_intf;
	default_reset = CALLBACK_RESET_INSTR;
	default_cmpild = CALLBACK_CMPILD_INSTR;
	default_rte = CALLBACK_RTE_INSTR;
}

static unsigned int checkpc(unsigned int pc)
{
	offs_t first, last, span, map_first, map_last;
	address_space *space = &active_address_space[ADDRESS_SPACE_PROGRAM];
	UINT8 *lookup = space->readlookup;
	UINT8 entry;
	pc = (pc - cyclone.membase) & 0xffffff;
	if (opcode_generation != memory_opcode_generation)
		memory_set_opbase(pc);
	else
		change_pc(pc);
	cyclone.membase = (unsigned int)cpu_opptr(pc) - pc;
	/* Also stop before an opcode address-mask wrap (mirrored ROM/RAM). */
	span = (opcode_mask ^ (opcode_mask + 1)) >> 1;
	first = MAX(opcode_memory_min, pc & ~span);
	last = MIN(opcode_memory_max, pc | span);
	/* Handler min/max alone can span holes introduced by dynamic maps.
	 * Restrict the fast path to contiguous lookup entries of this bank.
	 */
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
	/* Leave room for the longest instruction and speculative next fetch. */
	cyclone.internal[0] = cyclone.membase + first;
	cyclone.internal[1] = cyclone.membase + last - 16;
	if (entry != opcode_entry || last < first || last - first < 16 ||
		opcode_base != opcode_arg_base || address_is_unsafe(pc) || cyclone.internal[2])
	{
		cyclone.internal[0] = 1;
		cyclone.internal[1] = 0;
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

/* Direct banks cannot call drivers, change CPU state, or switch mappings.
 * Resolve both lookup levels, exactly as the normal memory accessors do.
 * Keep those accessors for endian conversion, masks and bank addressing.
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
	unsigned int result;
	address &= 0xffffff;
	begin_memory(has_handler(address, 0));
	result = program_read_byte_16be(address);
	finish_memory();
	return result;
}

static unsigned int read16(unsigned int address)
{
	unsigned int result;
	address &= 0xffffff;
	begin_memory(has_handler(address, 0));
	result = program_read_word_16be(address);
	finish_memory();
	return result;
}

static unsigned int read32(unsigned int address)
{
	unsigned int high, low, next;
	address &= 0xffffff;
	next = (address + 2) & 0xffffff;
	begin_memory(has_handler(address, 0) || has_handler(next, 0));
	high = program_read_word_16be(address);
	low = program_read_word_16be(next);
	finish_memory();
	return (high << 16) | low;
}

static void write8(unsigned int address, unsigned char data)
{
	address &= 0xffffff;
	begin_memory(has_handler(address, 1));
	program_write_byte_16be(address, data);
	finish_memory();
}

static void write16(unsigned int address, unsigned short data)
{
	address &= 0xffffff;
	begin_memory(has_handler(address, 1));
	program_write_word_16be(address, data);
	finish_memory();
}

static void write32(unsigned int address, unsigned int data)
{
	unsigned int next;
	address &= 0xffffff;
	next = (address + 2) & 0xffffff;
	begin_memory(has_handler(address, 1) || has_handler(next, 1));
	program_write_word_16be(address, data >> 16);
	program_write_word_16be(next, data);
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

/* m68k_set_irq normally takes the exception immediately. While Cyclone owns
 * the registers, let it build the exception frame instead. The CPU interface
 * acknowledges HOLD_LINE through the original callback, preserving ASSERT_LINE.
 */
int cyclone_mame_set_irq(unsigned int level)
{
	if (!running || cpu_getactivecpu() != running_cpu)
		return 0;
	/* Cyclone treats level 7 as continuously non-maskable. Use Musashi's
	 * edge handling at the instruction boundary instead, retaining the pin
	 * state for subsequent context switches and repeated assertions.
	 */
	if (level == 7)
	{
		pending_nmi = 1;
		cyclone.internal[2] = 1;
		cyclone.internal[0] = 1;
		cyclone.internal[1] = 0;
	}
	cyclone.irq = pending_nmi ? 0 : level;
	CPU_INT_LEVEL = cyclone.irq << 8;
	return 1;
}

int cyclone_mame_execute(int cycles)
{
#ifdef MAME_DEBUG
	return m68k_execute(cycles);
#else
	int remaining;
	/* Cyclone fetches instructions and extensions from the same word-swapped
	 * mapping. Separate decrypted opcode/argument streams require Musashi.
	 */
	if ((CPU_STOPPED & STOP_LEVEL_HALT) || CPU_INT_LEVEL == 0x700)
		return m68k_execute(cycles);
	memory_set_opbase(REG_PC & 0xffffff);
	if (CALLBACK_RESET_INSTR != default_reset ||
		CALLBACK_CMPILD_INSTR != default_cmpild ||
		CALLBACK_RTE_INSTR != default_rte ||
		opcode_base != opcode_arg_base ||
		m68k_encrypted_opcode_start[cpu_getactivecpu()] !=
		m68k_encrypted_opcode_end[cpu_getactivecpu()] ||
		memcmp(&m68k_memory_intf, &default_memory, sizeof(default_memory)))
		return m68k_execute(cycles);

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
	memcpy(cyclone.d, REG_D, sizeof(cyclone.d));
	memcpy(cyclone.a, REG_A, sizeof(cyclone.a));
	CycloneSetSr(&cyclone, m68ki_get_sr());
	cyclone.osp = FLAG_S ? REG_USP : REG_ISP;
	cyclone.pc = checkpc(REG_PC);
	cyclone.prev_pc = cyclone.membase + REG_PPC + 2;
	cyclone.irq = CPU_INT_LEVEL >> 8;
	cyclone.state_flags = ((CPU_STOPPED & STOP_LEVEL_STOP) ? 1 : 0)
		| ((cyclone.srh & 0x80) ? 2 : 0)
		| ((CPU_STOPPED & STOP_LEVEL_HALT) ? 0x10 : 0);
	cyclone.cycles = cycles - CPU_INT_CYCLES;
	CPU_INT_CYCLES = 0;
	m68ki_initial_cycles = cycles;
	pending_nmi = 0;
	running_cpu = cpu_getactivecpu();
	running = 1;
	CycloneRun(&cyclone);
	running = 0;
	export_state();
	REG_PPC = REG_PC;
	CPU_PREF_ADDR = ~0U;
	remaining = m68k_ICount;
	if (pending_nmi)
		m68k_set_irq(7);
	if (cyclone.internal[2] && remaining > 0)
	{
		/* Let the safe fetch path handle mapping edges for this timeslice. */
		m68k_execute(remaining);
		m68ki_initial_cycles = cycles;
	}
	return cycles - m68k_ICount;
#endif
}

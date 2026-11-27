#include <stdarg.h>
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LSB_FIRST
#endif

#include "mame/cpu/powerpc/ppc.c"

#define RAM_START 0x40000000U
#define MMIO 0x10000000U

static UINT64 ram[1024];
static struct ppc602_direct_map *const map = &ppc602_direct_maps[0];

address_space active_address_space[ADDRESS_SPACES];
global_options options;
int activecpu;
UINT8 opcode_entry;
UINT8 *opcode_base;
offs_t opcode_mask, opcode_memory_min, opcode_memory_max;
static UINT8 read_lookup[1];
static unsigned io_count;
static UINT32 io_pc, io_value;
static int io_cycles;

static UINT64 bus_read(UINT32 address, unsigned size)
{
	UINT32 offset = address - RAM_START;
	unsigned shift = (8 - size - (address & 7)) * 8;
	UINT64 mask = ~(UINT64)0 >> (64 - size * 8);

	if (offset < sizeof(ram))
		return (ram[offset >> 3] >> shift) & mask;

	assert_always(address == MMIO, "Unexpected bus read");
	++io_count;
	io_pc = ppc.pc;
	io_cycles = ppc_icount;
	return io_value;
}

static void bus_write(UINT32 address, UINT64 data, unsigned size)
{
	UINT32 offset = address - RAM_START;
	unsigned shift = (8 - size - (address & 7)) * 8;
	UINT64 mask = ~(UINT64)0 >> (64 - size * 8);

	if (offset < sizeof(ram))
	{
		UINT64 *word = &ram[offset >> 3];
		*word = (*word & ~(mask << shift)) | ((data & mask) << shift);
		ppc602_ram_written(word);
		return;
	}

	assert_always(address == MMIO, "Unexpected bus write");
	++io_count;
	io_pc = ppc.pc;
	io_cycles = ppc_icount;
	io_value = data;
}

#define BUS_ACCESSOR(name, type) \
	type program_read_##name##_64be(offs_t address) { return bus_read(address, sizeof(type)); } \
	void program_write_##name##_64be(offs_t address, type data) { bus_write(address, data, sizeof(type)); }

BUS_ACCESSOR(byte, UINT8)
BUS_ACCESSOR(word, UINT16)
BUS_ACCESSOR(dword, UINT32)
BUS_ACCESSOR(qword, UINT64)
#undef BUS_ACCESSOR

void memory_set_opbase(offs_t address)
{
	abort();
}

static void test_fetch(void)
{
	unsigned i;

	MSR = MSR_IR;
	for (i = 0; i < ARRAY_LENGTH(ram); ++i)
		ram[i] = ((UINT64)(i ^ 0xfedcba98) << 32) | (i ^ 0x76543210);
	for (i = 0; i < sizeof(ram); i += 4)
		assert_always(ppc602_fetch(map, RAM_START + i) == ppc602_fetch(NULL, RAM_START + i),
			"Direct fetch differs from the program-space accessor");
#ifdef LSB_FIRST
	MSR = 0;
	for (i = 4; i < sizeof(ram); i += 8)
		assert_always(ppc602_fetch(map, RAM_START + i) == ppc602_fetch(NULL, RAM_START + i),
			"Direct fetch differs from the opcode-space accessor");
#endif
	assert_always(!ppc602_opcode_region(map, RAM_START - 4) && !ppc602_opcode_region(map, RAM_START + 1) &&
		!ppc602_opcode_region(map, RAM_START + sizeof(ram)), "Invalid address found in direct fetch range");
	puts("PowerPC direct fetch matches the memory accessors");
}

#if PPC602_JIT_SUPPORTED
static struct jit_cache *cache;
static UINT32 random_state = 1;

static UINT32 bits(void)
{
	random_state ^= random_state << 13;
	random_state ^= random_state >> 17;
	random_state ^= random_state << 5;
	return random_state;
}

static void put_opcode(UINT32 pc, UINT32 op)
{
	if (program_read_dword_64be(pc) != op)
		program_write_dword_64be(pc, op);
}

static void init_state(UINT32 pc)
{
	unsigned i;

	memset(&ppc, 0, sizeof(ppc));
	for (i = 0; i < 32; ++i)
		ppc.r[i] = bits();
	for (i = 0; i < ARRAY_LENGTH(ppc.cr); ++i)
		ppc.cr[i] = bits() & 15;
	ppc.xer = bits();
	ppc.ctr = bits() & 3;
	ppc.npc = pc;
	ppc.is602 = 1;
	ppc_set_msr(bits() & (MSR_IR | MSR_DR | MSR_EE));
	ppc.read16_unaligned = ppc_read16_unaligned;
	ppc.read32_unaligned = ppc_read32_unaligned;
	ppc.write16_unaligned = ppc_write16_unaligned;
	ppc.write32_unaligned = ppc_write32_unaligned;
	ppc_icount = 200;
	ppc_dec_trigger_cycle = -1;
}

static void interpret_opcode(UINT32 op)
{
	ppc.pc = ppc.npc;
	ppc.npc += 4;
	switch (op >> 26)
	{
		case 10: ppc_cmpli(op); break;
		case 11: ppc_cmpi(op); break;
		case 14: ppc_addi(op); break;
		case 15: ppc_addis(op); break;
		case 16: ppc_bcx(op); break;
		case 18: ppc_bx(op); break;
		case 19: ppc_isync(op); break;
		case 20: ppc_rlwimix(op); break;
		case 21: ppc_rlwinmx(op); break;
		case 24: ppc_ori(op); break;
		case 25: ppc_oris(op); break;
		case 31:
			switch ((op >> 1) & 0x3ff)
			{
				case 0: ppc_cmp(op); break;
				case 10: case 522: ppc_addcx(op); break;
				case 23: ppc_lwzx(op); break;
				case 28: ppc_andx(op); break;
				case 32: ppc_cmpl(op); break;
				case 83: ppc_mfmsr(op); break;
				case 87: ppc_lbzx(op); break;
				case 136: case 648: ppc_subfex(op); break;
				case 138: case 650: ppc_addex(op); break;
				case 146: ppc_mtmsr(op); break;
				case 266: case 778: ppc_addx(op); break;
				case 444: ppc_orx(op); break;
				case 598: ppc_sync(op); break;
				default: abort();
			}
			break;
		case 32: ppc_lwz(op); break;
		case 33: ppc_lwzu(op); break;
		case 34: ppc_lbz(op); break;
		case 36: ppc_stw(op); break;
		case 38: ppc_stb(op); break;
		case 40: ppc_lhz(op); break;
		case 42: ppc_lha(op); break;
		case 44: ppc_sth(op); break;
		default: abort();
	}
	--ppc_icount;
}

static void test_arithmetic(void)
{
	static const unsigned alu[] = {10, 11, 14, 15, 19, 20, 21, 24, 25, 31, 31};
	static const unsigned xo[] = {0, 10, 28, 32, 83, 136, 138, 266, 444, 522, 598, 648, 650, 778};
	unsigned iteration;

	for (iteration = 0; iteration < 10000; ++iteration)
	{
		PPC_REGS initial, expected;
		UINT32 pc = RAM_START + (bits() & 0x1ff) * 4;
		unsigned length = 1 + bits() % PPC602_JIT_MAX_BLOCK;
		unsigned i;

		init_state(pc);
		initial = ppc;
		for (i = 0; i < length; ++i)
		{
			UINT32 op = (bits() & 0x03ffffff) | (alu[bits() % ARRAY_LENGTH(alu)] << 26);
			if ((op >> 26) == 19)
				op = (19U << 26) | (150 << 1);
			else if ((op >> 26) == 31)
				op = (op & ~0x7feU) | (xo[bits() % ARRAY_LENGTH(xo)] << 1);
			if (i == length - 1 && (bits() & 1))
				op = (op & 0x03ffffff) | ((bits() & 1 ? 16U : 18U) << 26);
			put_opcode(pc + i * 4, op);
			interpret_opcode(op);
		}
		put_opcode(pc + length * 4, 0xffffffff);
		expected = ppc;
		ppc = initial;
		ppc_icount = length;
		/* A trial can reuse a PC with the same prefix and a longer instruction sequence. */
		jit_cache_invalidate(cache, pc);
		assert_always(ppc602_jit_run(cache, map), "Expected a native block");
		assert_always(!ppc_icount, "Native cycle count");
		assert_always(!memcmp(&ppc, &expected, sizeof(ppc)), "Native arithmetic/branch state");
	}
	puts("10000 native PowerPC arithmetic and branch blocks match");
}

static void test_arithmetic_flags(void)
{
	static const UINT32 values[] = { 0, 1, 0x7fffffff, 0x80000000, 0xffffffff };
	static const unsigned xo[] = { 10, 136, 138, 266 };
	const UINT32 pc = RAM_START + 0xf00;
	unsigned op, a, b, rt, flags;

	for (op = 0; op < ARRAY_LENGTH(xo); ++op)
		for (a = 0; a < ARRAY_LENGTH(values); ++a)
			for (b = 0; b < ARRAY_LENGTH(values); ++b)
				for (rt = 1; rt <= 3; ++rt)
					for (flags = 0; flags < 8; ++flags)
					{
						PPC_REGS initial, expected;
						UINT32 opcode = (31U << 26) | (rt << 21) | (1 << 16) | (2 << 11) |
							(xo[op] << 1) | 0x401;

						init_state(pc);
						ppc.r[1] = values[a];
						ppc.r[2] = values[b];
						ppc.xer = 0x55 | (flags << 29);
						initial = ppc;
						put_opcode(pc, opcode);
						put_opcode(pc + 4, 0xffffffff);
						interpret_opcode(opcode);
						expected = ppc;
						ppc = initial;
						ppc_icount = 200;
						assert_always(ppc602_jit_run(cache, map) && ppc_icount == 199 &&
							!memcmp(&ppc, &expected, sizeof(ppc)), "Native add/subtract flags differ at an edge value");
					}
	puts("2400 addo./addco./addeo./subfeo. edge cases match, including XER and CR0");
}

static void test_msr_boundary(void)
{
	const UINT32 pc = RAM_START + 0xf40;
	const UINT32 write_msr = 0x7c800124;
	const UINT32 initial_msr = MSR_IR | MSR602_AP;
#if JIT_CACHE_STATS
	const struct ppc602_jit_statistics *stats = &ppc602_jit_stats[0];
	UINT64 unchanged = stats->mtmsr_native;
	UINT64 changed = stats->mtmsr_changed_exits;
#endif

	init_state(pc);
	ppc_set_msr(initial_msr);
	ppc.r[4] = initial_msr;
	ppc.r[5] = 0;
	put_opcode(pc, 0x7c6000a6);     /* mfmsr r3 */
	put_opcode(pc + 4, 0x7c0004ac); /* sync */
	put_opcode(pc + 8, write_msr);  /* mtmsr r4 */
	put_opcode(pc + 12, 0x38a00001); /* li r5,1 */
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc.r[3] == initial_msr && !ppc.r[5] &&
		MSR == initial_msr && ppc_icount == 197 && ppc.pc == pc + 8 && ppc.npc == pc + 12,
		"Unchanged MSR write must retire natively and end the block");

	ppc.npc = pc;
	ppc.r[4] = MSR_IR;
	ppc_icount = 200;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_PARTIAL && MSR == initial_msr && ppc_icount == 198 &&
		ppc.pc == pc + 4 && ppc.npc == pc + 8, "A change outside IR and DR must still exit before the MSR write");
	ppc.r[4] = initial_msr | MSR_DR;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_FALLBACK && MSR == initial_msr && ppc_icount == 198,
		"A block starting with a changed MSR write must fall back without consuming a cycle");
	interpret_opcode(write_msr);
	assert_always(MSR == (initial_msr | MSR_DR) && ppc.read32 == ppc_read32_translated && ppc_icount == 197,
		"Interpreter must install the new memory mode");
#if JIT_CACHE_STATS
	assert_always(stats->mtmsr_native - unchanged == 1 && stats->mtmsr_changed_exits - changed == 2,
		"PowerPC MSR statistics must distinguish native writes from changed-value exits");
#endif
	puts("PowerPC MSR writes preserve instruction boundaries and install changed memory modes");
}

static void test_dispatch(void)
{
	static const UINT64 rom[] = { ((UINT64)0x38600005 << 32) | 0xffffffff };
	const UINT32 rom_pc = 0x20000000;
	UINT32 pc = RAM_START + 0xffc;

	init_state(pc);
	put_opcode(pc, 0x38600001);
	put_opcode(pc + 4, 0x38800002);
	assert_always(ppc602_jit_run(cache, map) && ppc.r[3] == 1 && ppc.npc == pc + 4 && ppc_icount == 199,
		"Code must stop at the page boundary");
	put_opcode(pc, 0x38600003);
	ppc.npc = pc;
	assert_always(ppc602_jit_run(cache, map) && ppc.r[3] == 3, "RAM opcode change must invalidate a block");
	put_opcode(pc, 0xffffffff);
	ppc.npc = pc;
	assert_always(!ppc602_jit_run(cache, map) && jit_cache_probe(cache, pc), "Unsupported opcode must be cached");
	put_opcode(pc, 0x38600004);
	assert_always(ppc602_jit_run(cache, map) && ppc.r[3] == 4, "Changed opcode must invalidate a negative entry");
	ppc.npc = pc;
	ppc.msr ^= MSR_DR;
	assert_always(ppc602_jit_run(cache, map) && jit_cache_probe(cache, pc)->mode == (ppc.msr & PPC602_JIT_MODE),
		"Changed mode must replace the previous entry");
	ppc.npc = pc;
	ppc_dec_trigger_cycle = ppc_icount;
	assert_always(!ppc602_jit_run(cache, map), "Decrementer must bound the block");
	ppc_dec_trigger_cycle = -1;
	ppc.msr |= MSR_EE;
	ppc.interrupt_pending = 1;
	assert_always(!ppc602_jit_run(cache, map), "Pending interrupt needs single stepping");
	ppc.interrupt_pending = 0;
	ppc_icount = 0;
	assert_always(!ppc602_jit_run(cache, map), "No remaining cycle budget");

	ppc602_add_direct_memory(0, rom_pc, sizeof(rom), rom, 1);
	init_state(rom_pc);
	ppc_set_msr(MSR_IR);
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc.r[3] == 5,
		"Compile from the second mapped region");
	ppc.npc = rom_pc;
	ppc.r[3] = 0;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc.r[3] == 5 && ppc_icount == 198,
		"A cached block must reuse its ROM region");
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_FALLBACK, "Unsupported opcode in ROM must fall back");
	ppc.npc = rom_pc + sizeof(rom);
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_FALLBACK && !jit_cache_probe(cache, ppc.npc),
		"Code outside the mapped regions must fall back");
	puts("PowerPC page, opcode invalidation and dispatch guards passed");
}

static void test_data(void)
{
	static const unsigned majors[] = { 31, 32, 33, 34, 36, 38, 40, 42, 44 };
	UINT32 pc = RAM_START + 0x100;
	unsigned iteration;

	for (iteration = 0; iteration < 6000; ++iteration)
	{
		PPC_REGS initial, expected;
		unsigned major = majors[bits() % ARRAY_LENGTH(majors)];
		unsigned ra = major == 31 || major == 33 ? bits() & 31 : 1 + bits() % 31;
		unsigned rt = bits() & 31;
		UINT16 imm = bits();
		unsigned rb = (ra + 1 + bits() % 31) & 31;
		unsigned xo = bits() & 1 ? 23 : 87;
		UINT32 op = (major << 26) | (rt << 21) | (ra << 16) | (major == 31 ? (rb << 11) | (xo << 1) : imm);
		unsigned size = (major == 31 && xo == 87) || major == 34 || major == 38 ? 1 : major <= 33 || major == 36 ? 4 : 2;
		UINT32 offset = 0x1000 + (bits() & 0xfff);
		UINT64 before, after;
		UINT64 *word;

		if (!(iteration & 7))
			offset = sizeof(ram) - size;
		offset &= ~(size - 1);
		word = &ram[offset >> 3];
		*word = ((UINT64)bits() << 32) | bits();
		before = *word;
		init_state(pc);
		if (major == 31)
			ppc.r[rb] = RAM_START + offset - (ra ? ppc.r[ra] : 0);
		else
			ppc.r[ra] = RAM_START + offset - (INT16)imm;
		initial = ppc;
		put_opcode(pc, op);
		put_opcode(pc + 4, 0xffffffff);

		interpret_opcode(op);
		after = *word;
		expected = ppc;
		ppc = initial;
		ppc_icount = 200;
		*word = before;
		assert_always(ppc602_jit_run(cache, map), "Expected a native PowerPC memory operation");
		assert_always(ppc_icount == 199 && !memcmp(&ppc, &expected, sizeof(ppc)) && *word == after,
			"Native PowerPC memory operation differs");
	}
	puts("6000 aligned load and store operations match");
}

static void test_memory_exit(void)
{
	UINT32 pc = RAM_START + 0x800;
	UINT32 load = 0x84830004;
#if JIT_CACHE_STATS
	const struct ppc602_jit_statistics *stats = &ppc602_jit_stats[0];
	UINT64 outside = stats->memory_exits[0];
	UINT64 unaligned = stats->memory_exits[1];
	UINT64 reserved = stats->memory_exits[2];
#endif

	init_state(pc);
	put_opcode(pc, 0x38630001); /* addi r3,r3,1 */
	put_opcode(pc + 4, load);  /* lwzu r4,4(r3) */
	put_opcode(pc + 8, 0x48000000);
	ppc.r[3] = MMIO - 5;
	ppc.r[4] = 0x12345678;
	io_count = 0;
	io_value = 0xfedcba98;
	assert_always(ppc602_jit_run(cache, map) && ppc.npc == pc + 4 && ppc.pc == pc && ppc_icount == 199,
		"Side exit must retire only the completed prefix");
	assert_always(!io_count && ppc.r[3] == MMIO - 4 && ppc.r[4] == 0x12345678, "Side exit must leave both load registers intact");
	assert_always(!ppc602_jit_run(cache, map), "A block starting at MMIO must fall back");
	assert_always(ppc.r[3] == MMIO - 4 && ppc.r[4] == 0x12345678, "Fallback must leave both load registers intact");
	interpret_opcode(load);
	assert_always(io_count == 1 && ppc.r[3] == MMIO && ppc.r[4] == io_value && io_pc == pc + 4 && io_cycles == 199,
		"MMIO must execute once with the correct PC and cycle count");

	ppc.npc = pc + 4;
	ppc.r[3] = RAM_START + 1;
	assert_always(!ppc602_jit_run(cache, map), "Unaligned RAM access must fall back");
	assert_always(ppc.r[3] == RAM_START + 1, "Unaligned access must leave the base register intact");
	put_opcode(pc + 4, 0x90830000); /* stw r4,0(r3) */
	ppc.r[3] = RAM_START;
	ppc.reserved = 1;
	ppc.reserved_address = RAM_START;
	assert_always(!ppc602_jit_run(cache, map), "Reserved word store must fall back");
	interpret_opcode(0x90830000);
	assert_always(!ppc.reserved && program_read_dword_64be(RAM_START) == ppc.r[4], "Store must clear reservation");
#if JIT_CACHE_STATS
	assert_always(stats->memory_exits[0] - outside == 2 && stats->memory_exits[1] - unaligned == 1 &&
		stats->memory_exits[2] - reserved == 1, "PowerPC memory-exit statistics must identify the failing guard");
#endif
	puts("PowerPC MMIO, alignment and reservation exits passed");
}

static void test_shared_code(void)
{
	struct jit_cache *cache2;
	UINT32 store = RAM_START + 0xc00;
	UINT32 target = store + 4;
	UINT32 data = target + 32;
	unsigned code_page = ppc602_codemap_index(&ram[(target - RAM_START) >> 3]) >> (PPC602_JIT_PAGE_BITS - 3);

	if ((ppc602_codemap_index(&ram[(data - RAM_START) >> 3]) >> (PPC602_JIT_PAGE_BITS - 3)) != code_page)
		data = target - 32;

	ppc602_add_direct_memory(1, RAM_START, sizeof(ram), ram, 0);
	cache2 = ppc602_jit_cache_for_cpu(1);
	assert_always(cache2 && cache2 != cache, "CPU needs a separate native cache");
	put_opcode(store, 0x90830000);  /* stw r4,0(r3) */
	put_opcode(target, 0x38a00001); /* li r5,1 */
	put_opcode(target + 4, 0xffffffff);
	init_state(target);
	assert_always(ppc602_jit_run(cache, map) && ppc.r[5] == 1, "Initial code for CPU 0");
	init_state(target);
	assert_always(ppc602_jit_run(cache2, &ppc602_direct_maps[1]) && ppc.r[5] == 1, "Initial code for CPU 1");

	init_state(store);
	ppc.r[3] = data;
	ppc.r[4] = 0x12345678;
	assert_always(ppc602_jit_run(cache2, &ppc602_direct_maps[1]) == PPC602_JIT_COMPLETE &&
		program_read_dword_64be(data) == 0x12345678 && jit_cache_probe(cache, target) && jit_cache_probe(cache2, target),
		"An unmarked word on a code page must still take the native store path");

	init_state(store);
	ppc.r[3] = target;
	ppc.r[4] = 0x38a00002;
	assert_always(ppc602_jit_run(cache2, &ppc602_direct_maps[1]) == PPC602_JIT_FALLBACK && ppc_icount == 200,
		"A native store must exit before writing compiled code");
	assert_always(program_read_dword_64be(target) == 0x38a00001, "Code store must not run on a side exit");
	interpret_opcode(0x90830000);
	assert_always(!jit_cache_probe(cache, target) && !jit_cache_probe(cache2, target), "A code store must clear both caches");
	assert_always(ppc602_jit_run(cache2, &ppc602_direct_maps[1]) && ppc.r[5] == 2, "CPU must see its new opcode");
	init_state(target);
	assert_always(ppc602_jit_run(cache, map) && ppc.r[5] == 2, "CPU 0 must see CPU 1's new opcode");

	/* The CDE writes words through the bus during DMA. */
	program_write_word_64be(target + 2, 3);
	assert_always(!jit_cache_probe(cache, target) && !jit_cache_probe(cache2, target), "DMA must clear both caches");
	init_state(target);
	assert_always(ppc602_jit_run(cache, map) && ppc.r[5] == 3, "CPU 0 must see DMA writes");
	init_state(target);
	assert_always(ppc602_jit_run(cache2, &ppc602_direct_maps[1]) && ppc.r[5] == 3, "CPU 1 must see DMA writes");
	/* The DSP and graphics engine write host words directly. */
	ram[(target - RAM_START) >> 3] ^= 7;
	ppc602_ram_written(&ram[(target - RAM_START) >> 3]);
	assert_always(!jit_cache_probe(cache, target) && !jit_cache_probe(cache2, target), "Direct writes must clear both caches");
	init_state(target);
	assert_always(ppc602_jit_run(cache, map) && ppc.r[5] == 4, "CPU 0 must see direct device writes");
	init_state(target);
	assert_always(ppc602_jit_run(cache2, &ppc602_direct_maps[1]) && ppc.r[5] == 4, "CPU 1 must see direct device writes");
	ppc602_jit_clear(1);
	assert_always(jit_cache_probe(cache, target) && !jit_cache_probe(cache2, target), "Clearing CPU 1 must preserve CPU 0's cache");
	ppc602_jit_exit(1);
	puts("PowerPC shared code invalidation passed for CPU, DMA and direct device writes");
}

static void test_chaining(void)
{
	const UINT32 rom_pc = RAM_START - 0x10000;
	const UINT32 ram_a = RAM_START + 0x1400, ram_b = ram_a + 0x10;
	const UINT32 loop = ram_b + 0x10;
	const UINT32 mode = MSR_IR | MSR_DR | MSR_EE;
	const unsigned length[] = { 1, 2, 2, 1, 1 };
	const UINT32 pcs[] = { rom_pc + 16, ram_b, ram_a, rom_pc + 8, rom_pc };
	static const UINT64 rom[] = {
		(U64(0x48000008) << 32) | 0xffffffff,
		(U64(0x480113f8) << 32) | 0xffffffff,
		(U64(0x38c00009) << 32) | 0xffffffff
	};
	unsigned i;

	ppc602_add_direct_memory(0, rom_pc, sizeof(rom), rom, 1);
	put_opcode(ram_a, 0x38a50001); /* addi r5,r5,1 */
	put_opcode(ram_a + 4, 0x4800000c);
	put_opcode(ram_b, 0x38a50002); /* addi r5,r5,2 */
	put_opcode(ram_b + 4, 0x4bfeebfc);
	for (i = 0; i < ARRAY_LENGTH(pcs); ++i)
	{
		init_state(pcs[i]);
		ppc_set_msr(mode);
		ppc_icount = length[i];
		assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && !ppc_icount,
			"Warm one block with an exact cycle budget");
	}

	init_state(rom_pc);
	ppc_set_msr(mode);
	ppc.r[5] = ppc.r[6] = 0;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc.r[5] == 3 && ppc.r[6] == 9 &&
		ppc.pc == rom_pc + 16 && ppc.npc == rom_pc + 20 && ppc_icount == 193,
		"One native call must chain ROM to ROM, ROM to RAM, RAM to RAM and RAM to ROM");

	ppc.npc = rom_pc;
	ppc.r[5] = 0;
	ppc_dec_trigger_cycle = ppc_icount - 4;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc.r[5] == 1 &&
		ppc.npc == ram_b && ppc_icount == ppc_dec_trigger_cycle,
		"The chain must leave the exact instruction boundary of a decrementer event");
	ppc_dec_trigger_cycle = -1;
	ppc.interrupt_pending = 1;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_FALLBACK && ppc.npc == ram_b && ppc_icount == 189,
		"Pending IRQ must prevent entry into a cached chain");
	ppc.interrupt_pending = 0;

	ppc.msr ^= MSR_DR;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc.npc == rom_pc + 16,
		"A chain must reject an entry compiled for a different MSR mode");

	put_opcode(ram_b, 0x80830000); /* lwz r4,0(r3) */
	put_opcode(ram_b + 4, 0xffffffff);
	init_state(ram_b);
	ppc_set_msr(mode);
	ppc.r[3] = RAM_START;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE, "Compile a RAM load for chaining");
	init_state(ram_a);
	ppc_set_msr(mode);
	ppc.r[3] = MMIO;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_PARTIAL && ppc.pc == ram_a + 4 &&
		ppc.npc == ram_b && ppc_icount == 198, "A side exit at a chained block's first instruction must preserve prior progress");

	put_opcode(loop, 0x42000000); /* bdnz to itself */
	init_state(loop);
	ppc_set_msr(mode);
	ppc.ctr = 2048;
	ppc_icount = 1024;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && !ppc_icount && ppc.ctr == 1024 &&
		ppc.npc == loop, "A conditional RAM self-chain must exhaust the slice without growing the stack");
	puts("PowerPC native chains preserve ROM/RAM transitions, cycle boundaries, mode and side exits");
}

static unsigned irq_count;

static void test_internal_branches(void)
{
	const UINT32 pc = RAM_START + 0x1600;
	unsigned i;

	put_opcode(pc, 0x4182000c);      /* beq pc+12 */
	put_opcode(pc + 4, 0x38a50064);  /* addi r5,r5,100 */
	put_opcode(pc + 8, 0x38a50064);
	put_opcode(pc + 12, 0x38a50001); /* addi r5,r5,1 */
	put_opcode(pc + 16, 0x4200fffc); /* bdnz pc+12 */
	put_opcode(pc + 20, 0x38a50004); /* addi r5,r5,4 */
	put_opcode(pc + 24, 0xffffffff);
	init_state(pc);
	ppc.cr[0] = 2;
	ppc.r[5] = 0;
	ppc.ctr = 3;
	ppc_icount = 8;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && !ppc_icount && ppc.ctr == 0 &&
		ppc.r[5] == 7 && ppc.pc == pc + 20 && ppc.npc == pc + 24 && !jit_cache_probe(cache, pc + 12),
		"Forward branches and backward loops must use labels in one translation");

	ppc.npc = pc;
	ppc.cr[0] = 2;
	ppc.r[5] = 0;
	ppc.ctr = 3;
	ppc_icount = 8;
	ppc_dec_trigger_cycle = 3;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc_icount == 3 && ppc.ctr == 1 &&
		ppc.r[5] == 2 && ppc.pc == pc + 16 && ppc.npc == pc + 12,
		"An internal loop must end at the decrementer boundary");

	put_opcode(pc, 0x41820008);      /* beq pc+8 */
	put_opcode(pc + 8, 0x7c83282e); /* lwzx r4,r3,r5 */
	put_opcode(pc + 12, 0xffffffff);
	init_state(pc);
	ppc.cr[0] = 2;
	ppc.r[3] = RAM_START;
	ppc.r[5] = 1;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_PARTIAL && ppc_icount == 199 &&
		ppc.pc == pc && ppc.npc == pc + 8, "A guard at an internal target must preserve the previous branch PC");

	for (i = 0; i < PPC602_JIT_MAX_BLOCK + 1; ++i)
		put_opcode(pc + i * 4, 0x38a50001);
	init_state(pc);
	ppc.r[5] = 0;
	assert_always(ppc602_jit_run(cache, map) == PPC602_JIT_COMPLETE && ppc.r[5] == 64 &&
		ppc_icount == 136 && ppc.npc == pc + 64 * 4 && jit_cache_probe(cache, pc)->len == 64,
		"A linear block must finish at the 64-instruction cap");
	puts("PowerPC internal forward and backward branches, decrementer and long blocks passed");
}

static int test_irq_callback(int line)
{
	assert_always(line == INPUT_LINE_IRQ0, "Unexpected interrupt line");
	++irq_count;
	return 0;
}

static void test_execute(void)
{
	const UINT32 pc = RAM_START + 0xe00;
	PPC_REGS start, reference;
	unsigned jit, kind;

	ppc602_jit_clear(0);
	optable[14] = ppc_addi;
	optable[32] = ppc_lwz;
	optable31[146] = ppc_mtmsr;
	bus_freq_multiplier = 2;
	for (kind = 0; kind < 4; ++kind)
	{
		int cycles = kind >= 2 ? 1 : 3;

		init_state(pc);
		ppc_set_msr(MSR_IR | MSR_DR | (kind == 3 ? 0 : MSR_EE));
		ppc.dec = kind == 1 ? 0 : 10;
		ppc.dec_frac = kind == 1 ? 1 : 0;
		ppc.interrupt_pending = kind >= 2 ? 1 : 0;
		ppc.irq_callback = test_irq_callback;
		ppc.r[3] = MMIO - 1;
		ppc.r[4] = MSR | MSR_EE;
		io_value = 0xfedcba98;
		put_opcode(pc, kind == 3 ? 0x7c800124 : 0x38630001);
		put_opcode(pc + 4, kind == 0 ? 0x80830000 : 0x38630001);
		put_opcode(pc + 8, kind == 1 ? 0x7c800124 : 0x38630001);
		put_opcode(pc + 12, 0xffffffff);
		start = ppc;
		for (jit = 0; jit <= 1; ++jit)
		{
			ppc = start;
			options.jit = jit;
			io_count = irq_count = 0;
			assert_always(ppc602_execute(cycles) == cycles, "PPC602 must consume the same slice");
			if (kind == 0)
			{
				const struct jit_block *entry = jit_cache_probe(cache, pc);
				assert_always((entry && entry->code) == jit, "The global JIT option must control PPC602 block compilation");
				assert_always(io_count == 1 && io_pc == pc + 4 && io_cycles == 2 && ppc.r[4] == io_value,
					"PPC602 MMIO timing differs");
			}
			else
				assert_always(!io_count && irq_count == (kind >= 2) && ppc.srr0 == pc + 4 * cycles &&
					ppc.npc == (kind == 1 ? 0x900 : 0x500), "Unexpected PPC602 interrupt timing");
			if (jit)
				assert_always(!memcmp(&ppc, &reference, offsetof(PPC_REGS, exception_jmpbuf)),
					"PPC602 interpreter and native execution state differ");
			else
				reference = ppc;
		}
	}
	puts("PowerPC execute loop matches for MMIO, decrementer and external IRQ");
}
#endif

int main(void)
{
	active_address_space[ADDRESS_SPACE_PROGRAM].readlookup = read_lookup;
	opcode_base = (UINT8 *)ram;
	opcode_mask = sizeof(ram) - 1;
	opcode_memory_max = ~0U;
	ppc602_add_direct_memory(0, RAM_START, sizeof(ram), ram, 0);
	test_fetch();
#if PPC602_JIT_SUPPORTED
	ppc_init_rotate_masks();
	cache = ppc602_jit_cache_for_cpu(0);
	assert_always(cache, "JIT cache allocation");
	test_arithmetic();
	test_arithmetic_flags();
	test_msr_boundary();
	test_dispatch();
	test_data();
	test_memory_exit();
	test_shared_code();
	test_chaining();
	test_internal_branches();
	test_execute();
	ppc602_jit_exit(0);
#endif
	return 0;
}

void CLIB_DECL fatalerror(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	abort();
}

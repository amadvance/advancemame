// license:GPL-2.0-only
// copyright-holders:FinalDave, Gražvydas Ignotas
/* Standalone ARM regression tests, using the real Musashi and Cyclone cores. */
#include "driver.h"
#include "cpu/m68000/m68kcpu.h"
#include <assert.h>
#include <stdarg.h>
#include "state.h"

void cyclone_mame_init(void);
int cyclone_mame_execute(int cycles);

int activecpu;
UINT8 *opcode_base, *opcode_arg_base;
offs_t opcode_mask = 0xffffff, opcode_memory_min, opcode_memory_max;
UINT8 opcode_entry;
address_space active_address_space[ADDRESS_SPACES];
struct m68k_memory_interface m68k_memory_intf;
offs_t m68k_encrypted_opcode_start[MAX_CPU], m68k_encrypted_opcode_end[MAX_CPU];
static UINT8 ram[0x1000000], saved_ram[0x1000000];
static UINT8 lookup[0x10000];
static unsigned int bank_size = sizeof(ram);
static int irq_count, irq_vector = -1, hold, wait_cycles, reset_count;
static int raise_irq, bank_switch;
static UINT8 alternate[0x1000];

void memory_set_opbase(offs_t pc)
{
	opcode_base = opcode_arg_base = bank_switch ? alternate : ram;
	opcode_memory_min = pc & ~(bank_size - 1);
	opcode_memory_max = opcode_memory_min + bank_size - 1;
	opcode_entry = 1;
}

UINT8 program_read_byte_16be(offs_t address)
{
	m68k_ICount -= wait_cycles;
	return ram[(address & 0xffffff) ^ 1];
}
UINT16 program_read_word_16be(offs_t address)
{
	m68k_ICount -= wait_cycles;
	return *(UINT16 *)(ram + (address & 0xfffffe));
}
void program_write_byte_16be(offs_t address, UINT8 data)
{
	ram[(address & 0xffffff) ^ 1] = data;
}
void program_write_word_16be(offs_t address, UINT16 data)
{
	*(UINT16 *)(ram + (address & 0xfffffe)) = data;
	if (address == 0xf000 && raise_irq)
	{
		int level = raise_irq == 1 ? 2 : raise_irq;
		raise_irq = 0;
		m68k_set_irq(level);
	}
	if (address == 0xf002)
	{
		bank_switch = 1;
		memory_set_opbase(REG_PC);
	}
}
static UINT32 readlong(offs_t address)
{
	UINT32 high = program_read_word_16be(address);
	return (high << 16) | program_read_word_16be(address + 2);
}
static void writelong(offs_t address, UINT32 data)
{
	program_write_word_16be(address, data >> 16);
	program_write_word_16be(address + 2, data);
}
static int acknowledge(int level)
{
	assert(level == 2 || level == 7);
	irq_count++;
	if (hold)
		m68k_set_irq(0);
	return irq_vector;
}
static void reset_hook(void) { reset_count++; }

/* Capture the real core's state registrations and replay a save/load. */
static struct { void *value; UINT32 size; UINT8 saved[128]; } items[32];
static int item_count;
static void (*presave)(void), (*postload)(void);
void state_save_register_memory(const char *module, UINT32 instance,
	const char *name, void *value, UINT32 size, UINT32 count)
{
	assert(item_count < 32 && size * count <= 128);
	items[item_count].value = value;
	items[item_count++].size = size * count;
}
void state_save_register_func_presave(void (*func)(void)) { presave = func; }
void state_save_register_func_postload(void (*func)(void)) { postload = func; }

void fatalerror(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	abort();
}

static void putword(unsigned int address, unsigned int value)
{
	*(UINT16 *)(ram + address) = value;
}
static void prepare(unsigned int pc)
{
	memset(ram, 0, sizeof(ram));
	memset(&m68ki_cpu, 0, sizeof(m68ki_cpu));
	memset(lookup, 1, sizeof(lookup));
	active_address_space[ADDRESS_SPACE_PROGRAM].readlookup = lookup;
	active_address_space[ADDRESS_SPACE_PROGRAM].addrmask = 0xffffff;
	bank_size = sizeof(ram);
	opcode_mask = 0xffffff;
	bank_switch = raise_irq = irq_count = wait_cycles = 0;
	irq_vector = -1;
	hold = 1;
	m68k_memory_intf.read8 = program_read_byte_16be;
	m68k_memory_intf.read16 = program_read_word_16be;
	m68k_memory_intf.read32 = readlong;
	m68k_memory_intf.write8 = program_write_byte_16be;
	m68k_memory_intf.write16 = program_write_word_16be;
	m68k_memory_intf.write32 = writelong;
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	cyclone_mame_init();
	m68k_set_int_ack_callback(acknowledge);
	writelong(0, 0x80000);
	writelong(4, pc);
	m68k_pulse_reset();
	CPU_INT_CYCLES = 0;
}

static void compare(int budget, const char *name)
{
	m68ki_cpu_core initial, expected;
	int used, actual, saved_raise = raise_irq, saved_wait = wait_cycles;
	m68k_get_context(&initial);
	memcpy(saved_ram, ram, sizeof(ram));
	used = m68k_execute(budget);
	m68k_get_context(&expected);
	m68k_set_context(&initial);
	memcpy(ram, saved_ram, sizeof(ram));
	raise_irq = saved_raise;
	wait_cycles = saved_wait;
	irq_count = 0;
	memory_set_opbase(REG_PC);
	actual = cyclone_mame_execute(budget);
	if (memcmp(expected.dar, REG_D, sizeof(expected.dar)) || expected.pc != REG_PC || used != actual)
	{
		fprintf(stderr, "%s: PC %x/%x cycles %d/%d D0 %x/%x\n", name,
			expected.pc, REG_PC, used, actual, expected.dar[0], REG_D[0]);
		abort();
	}
	assert(expected.stopped == CPU_STOPPED);
	assert(expected.int_level == CPU_INT_LEVEL);
	assert(expected.s_flag == FLAG_S);
	assert(expected.x_flag == FLAG_X);
	assert(expected.n_flag == FLAG_N);
	assert(!expected.not_z_flag == !FLAG_Z);
	printf("PASS %s\n", name);
}

int main(void)
{
	int i;
	m68ki_cpu_core first, second;
	setbuf(stdout, NULL);
	prepare(0x100);
	putword(0x100, 0x7001); /* moveq #1,d0 */
	putword(0x102, 0x5280); /* addq.l #1,d0 */
	putword(0x104, 0x60fc); /* bra.s 0x102 */
	compare(10000, "arithmetic and branches");

	prepare(0x100);
	putword(0x100, 0x203c); putword(0x102, 0x1234); putword(0x104, 0x5678);
	putword(0x106, 0x23c0); putword(0x108, 0); putword(0x10a, 0x2000);
	putword(0x10c, 0x2239); putword(0x10e, 0); putword(0x110, 0x2000);
	putword(0x112, 0x4e72); putword(0x114, 0x2700);
	compare(100, "longword memory and STOP");
	assert(REG_D[1] == 0x12345678);

	prepare(0xff8);
	bank_size = 0x1000;
	putword(0xff8, 0x7001); putword(0xffa, 0x5280);
	putword(0xffc, 0x5280); putword(0xffe, 0x5280);
	putword(0x1000, 0x4e72); putword(0x1002, 0x2700);
	compare(100, "sequential bank boundary");

	prepare(0xff8);
	opcode_mask = 0xfff;
	putword(0xff8, 0x7001); putword(0xffa, 0x5280);
	putword(0xffc, 0x5280); putword(0xffe, 0x5280);
	putword(0, 0x4e72); putword(2, 0x2700);
	compare(100, "mirrored opcode address wrap");

	prepare(0x100);
	putword(0x100, 0x3039); putword(0x102, 0); putword(0x104, 0x2000);
	putword(0x106, 0x4e72); putword(0x108, 0x2700);
	putword(0x2000, 0x1234);
	wait_cycles = 8;
	compare(100, "memory handler cycle adjustment");

	prepare(0x100);
	putword(0x100, 0x4ef9); putword(0x102, 0); putword(0x104, 0xffa);
	putword(0xffa, 0x7003); putword(0xffc, 0x4e72); putword(0xffe, 0x2700);
	bank_size = 0x1000;
	compare(100, "jump near bank boundary");

	prepare(0x100);
	putword(0x100, 0x4e70); putword(0x102, 0x4e72); putword(0x104, 0x2700);
	m68k_set_reset_instr_callback(reset_hook);
	reset_count = 0;
	cyclone_mame_execute(200);
	assert(reset_count == 1);
	puts("PASS special instruction callback fallback");

	prepare(0x100);
	putword(0x100, 0x33fc); putword(0x102, 0x1234); putword(0x104, 0); putword(0x106, 0xf000);
	putword(0x108, 0x60fe);
	writelong((24 + 2) * 4, 0x300);
	putword(0x300, 0x7007); putword(0x302, 0x4e72); putword(0x304, 0x2700);
	m68k_set_reg(M68K_REG_SR, 0x2000);
	raise_irq = 1;
	cyclone_mame_execute(200);
	assert(irq_count == 1 && REG_D[0] == 7 && CPU_INT_LEVEL == 0);
	puts("PASS interrupt during memory handler and HOLD acknowledgement");

	prepare(0x100);
	putword(0x100, 0x23fc); putword(0x102, 0); putword(0x104, 1);
	putword(0x106, 0); putword(0x108, 0xf002);
	memcpy(alternate, ram, sizeof(alternate));
	*(UINT16 *)(alternate + 0x10a) = 0x7009;
	*(UINT16 *)(alternate + 0x10c) = 0x4e72;
	*(UINT16 *)(alternate + 0x10e) = 0x2700;
	bank_size = 0x1000;
	cyclone_mame_execute(200);
	assert(REG_D[0] == 9);
	puts("PASS opcode bank switch during memory handler");

	prepare(0x100);
	putword(0x100, 0x5280); putword(0x102, 0x60fc);
	cyclone_mame_execute(100);
	m68k_get_context(&first);
	m68k_set_reg(M68K_REG_D0, 1000);
	m68k_get_context(&second);
	m68k_set_context(&first);
	cyclone_mame_execute(100);
	assert(REG_D[0] < 1000);
	m68k_set_context(&second);
	cyclone_mame_execute(100);
	assert(REG_D[0] > 1000);
	puts("PASS independent CPU contexts");

	item_count = 0;
	m68k_state_register("m68000", 0);
	presave();
	for (i = 0; i < item_count; ++i)
		memcpy(items[i].saved, items[i].value, items[i].size);
	m68k_get_context(&first);
	cyclone_mame_execute(100);
	m68k_get_context(&second);
	for (i = 0; i < item_count; ++i)
		memcpy(items[i].value, items[i].saved, items[i].size);
	postload();
	assert(REG_PC == first.pc && REG_D[0] == first.dar[0]);
	cyclone_mame_execute(100);
	assert(REG_PC == second.pc && REG_D[0] == second.dar[0]);
	puts("PASS save/load and deterministic continuation");

	prepare(0x100);
	putword(0x100, 0x4e72); putword(0x102, 0x2000);
	writelong((24 + 2) * 4, 0x300);
	putword(0x300, 0x7005); putword(0x302, 0x4e72); putword(0x304, 0x2700);
	cyclone_mame_execute(100);
	assert(CPU_STOPPED);
	m68k_set_irq(2);
	cyclone_mame_execute(100);
	assert(REG_D[0] == 5 && irq_count == 1);
	puts("PASS interrupt wakes STOP");

	prepare(0x100);
	putword(0x100, 0x33fc); putword(0x102, 1); putword(0x104, 0); putword(0x106, 0xf000);
	putword(0x108, 0x60fe);
	irq_vector = -2;
	writelong(24 * 4, 0x300);
	putword(0x300, 0x7006); putword(0x302, 0x4e72); putword(0x304, 0x2700);
	m68k_set_reg(M68K_REG_SR, 0x2000);
	raise_irq = 1;
	cyclone_mame_execute(100);
	assert(REG_D[0] == 6 && irq_count == 1);
	puts("PASS spurious interrupt vector");

	prepare(0x100);
	putword(0x100, 0x33fc); putword(0x102, 1); putword(0x104, 0); putword(0x106, 0xf000);
	putword(0x108, 0x60fe);
	irq_vector = 0x50;
	hold = 0;
	writelong(0x50 * 4, 0x300);
	putword(0x300, 0x7004); putword(0x302, 0x4e72); putword(0x304, 0x2700);
	m68k_set_reg(M68K_REG_SR, 0x2000);
	raise_irq = 1;
	cyclone_mame_execute(100);
	assert(REG_D[0] == 4 && irq_count == 1 && CPU_INT_LEVEL == 0x200);
	puts("PASS explicit interrupt vector and persistent ASSERT");

	prepare(0x100);
	putword(0x100, 0x33fc); putword(0x102, 1); putword(0x104, 0); putword(0x106, 0xf000);
	putword(0x108, 0x60fe);
	hold = 0;
	writelong((24 + 7) * 4, 0x300);
	putword(0x300, 0x7002); putword(0x302, 0x4e72); putword(0x304, 0x2700);
	raise_irq = 7;
	cyclone_mame_execute(200);
	assert(REG_D[0] == 2 && irq_count == 1 && CPU_INT_LEVEL == 0x700);
	cyclone_mame_execute(200);
	assert(irq_count == 1);
	puts("PASS masked level-7 edge and persistent assertion");
	return 0;
}

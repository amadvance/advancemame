#include <stdarg.h>

#include "mame/sndhrdw/konamim2.c"

enum event_type
{
	BUS_READ, BUS_WRITE, IRQ, DAC, PC, CLOCK, ACC, FIFO_DEPTH, DMA_COUNT
};

static struct event
{
	UINT64 tick;
	enum event_type type;
	UINT32 address, value;
} events[8192];
static unsigned event_count;
static mame_time test_time;
mame_time time_never = { MAX_SECONDS, MAX_SUBSECONDS - 1 };
global_options options;

static void record(enum event_type type, UINT32 address, UINT32 value)
{
	assert_always(event_count < ARRAY_LENGTH(events), "DSP trace overflow");
	events[event_count].tick = m_elapsed_ticks;
	events[event_count].type = type;
	events[event_count].address = address;
	events[event_count++].value = value;
}

void m2_irq(UINT32 bit, int state)
{
	record(IRQ, bit, state);
}

UINT8 m2_ram_byte(UINT32 address)
{
	UINT8 value = address * 91 + 7;

	record(BUS_READ, address, value);
	return value;
}

void m2_ram_write_byte(UINT32 address, UINT8 value)
{
	record(BUS_WRITE, address, value);
}

void DAC_signed_data_16_w(int channel, UINT16 value)
{
	record(DAC, channel, value);
}

mame_time mame_timer_get_time(void)
{
	return test_time;
}

void mame_timer_adjust(mame_timer *timer, mame_time duration, INT32 param, mame_time period)
{
}

UINT32 mame_rand(void)
{
	fatalerror("Unexpected DSP noise read");
}

void CLIB_DECL logerror(const char *fmt, ...)
{
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

static UINT32 random_state = 1;

static UINT32 bits(void)
{
	random_state ^= random_state << 13;
	random_state ^= random_state >> 17;
	random_state ^= random_state << 5;
	return random_state;
}

static void test_fifo_arbitration(void)
{
	unsigned trial, ch;

	m2_dspp_reset();
	for (trial = 0; trial < 100000; ++trial)
	{
		UINT32 expected = 0;
		UINT32 changes = bits();

		m_dsp.m_dspx_channel_direction = bits();
		m_dsp.m_dspx_channel_enable = bits();
		m_dsp.m_dspx_channel_complete = bits();
		for (ch = 0; ch < NUM_DMA_CHANNELS; ++ch)
		{
			UINT32 bit = 1U << ch;
			struct fifo_dma *fifo = &m_dsp.m_fifo_dma[ch];
			bool output = (m_dsp.m_dspx_channel_direction & bit) != 0;

			if (changes & bit)
				dspp_set_fifo_depth(ch, bits() % (DMA_FIFO_DEPTH + 1));
			fifo->m_current_count = 1;
			fifo->m_current_addr = ch << 8;
			if ((m_dsp.m_dspx_channel_enable & ~m_dsp.m_dspx_channel_complete & bit) &&
				(output ? fifo->m_depth >= 4 : fifo->m_depth <= 4))
				expected |= bit;
		}
		assert_always(dspp_fifo_dma_ready() == expected, "DMA readiness differs from scanning FIFO depths");
		event_count = 0;
		dspp_update_fifo_dma(dspp_fifo_dma_ready());
		assert_always(event_count == (expected ? 2 : 0), "DMA must transfer at most one channel");
		if (expected)
			assert_always(events[0].address >> 8 == 31U - __builtin_clz(expected), "DMA channel priority");
		if (trial % 17 == 0)
			dspp_reset_channel(trial % NUM_DMA_CHANNELS);
	}
	puts("100000 DMA states match the original arbitration");
}

#if SLJIT_MAME_SUPPORTED
static void emit(unsigned *pc, UINT16 word)
{
	m_program[(*pc)++ & 0x3ff] = word;
}

static void test_arithmetic(void)
{
	static const unsigned alu[] = {0,1,2,4,8,9,10,11,12,13,14,15};
	static const unsigned shift[] = {0,1,2,3,4,5,6,9,10,11,12,13,14,15};
	const unsigned trials = 4000;
	unsigned trial;

	for (trial = 0; trial < trials; ++trial)
	{
		__typeof__(m_core) initial, expected;
		UINT16 ram[ARRAY_LENGTH(m_data_ram)], expected_ram[ARRAY_LENGTH(m_data_ram)];
		unsigned i, count = 1 + bits() % DSPP_JIT_MAX_BLOCK;
		unsigned start = bits() & 0xffff, pc = start;
		UINT64 time = bits(), expected_ticks;

		memset(m_program, 0x80, sizeof(m_program));
		memset(&m_core, 0, sizeof(m_core));
		m_core.m_writeback = -2;
		m_core.m_acc = dspp_sign_extend(bits(), 20);
		m_core.m_pc = start;
		m_core.m_tclock = bits();
		for (i = 0; i < 4; ++i)
			m_core.m_rbase[i] = bits() % 0x2dc;
		for (i = 0; i < ARRAY_LENGTH(m_data_ram); ++i)
			m_data_ram[i] = bits();
		for (i = 0; i < count; ++i)
		{
			unsigned a = bits() & 3, b = bits() & 3, mul_b = bits() & 1;
			unsigned reads = (a == 1 || a == 2) + (b == 1 || b == 2) + ((a == 3 || b == 3) ? 1 + mul_b : 0);
			unsigned n = reads + 1, j;
			unsigned kind = bits() % 4;
			UINT16 op = ((n & 3) << 13) | (mul_b << 12) | (a << 10) | (b << 8) |
				(alu[bits() % ARRAY_LENGTH(alu)] << 4) | shift[bits() % ARRAY_LENGTH(shift)];
			emit(&pc, op);
			if (kind == 0 && n == 3)
				emit(&pc, ((bits() & 0xf) << 10) | ((bits() & 0xf) << 5) | (bits() & 0xf));
			else if (kind == 1 && n == 2)
				emit(&pc, 0xac00 | ((bits() & 0xf) << 5) | (bits() & 0xf));
			else
			{
				for (j = 0; j < reads; ++j)
				{
					UINT16 choice = bits();
					switch (choice % 3)
					{
						case 0: emit(&pc, 0xc000 | (choice & 0x3fff)); break;
						case 1: emit(&pc, 0x8000 | choice % 0x2e0); break;
						default: emit(&pc, 0xa000 | (choice & 0xf)); break;
					}
				}
				emit(&pc, (kind == 2 ? 0x8800 : 0x8000) | (bits() % 0x2e0));
			}
		}
		emit(&pc, 0x8380);
		initial = m_core;
		memcpy(ram, m_data_ram, sizeof(ram));
		m_elapsed_ticks = time;
		for (i = 0; i < count; ++i)
		{
			dspp_update_ticks();
			m_core.m_op = dspp_read_op(m_core.m_pc);
			dspp_update_pc();
			dspp_exec_arithmetic();
		}
		expected = m_core;
		expected_ticks = m_elapsed_ticks;
		memcpy(expected_ram, m_data_ram, sizeof(expected_ram));
		m_core = initial;
		m_elapsed_ticks = time;
		memcpy(m_data_ram, ram, sizeof(ram));
		m_jit_dirty = 1;
		if (!dspp_jit_run(expected_ticks - time) ||
			m_elapsed_ticks != expected_ticks ||
			m_core.m_pc != expected.m_pc || m_core.m_tclock != expected.m_tclock ||
			m_core.m_acc != expected.m_acc || m_core.m_op != expected.m_op ||
			m_core.m_writeback != expected.m_writeback ||
			m_core.m_flag_over != expected.m_flag_over || m_core.m_flag_carry != expected.m_flag_carry ||
			m_core.m_flag_neg != expected.m_flag_neg || m_core.m_flag_zero != expected.m_flag_zero ||
			m_core.m_flag_exact != expected.m_flag_exact ||
			memcmp(expected_ram, m_data_ram, sizeof(expected_ram)))
		{
			fprintf(stderr, "trial %u, instructions %u, PC %04x/%04x, ACC %08x/%08x, opcode %04x, elapsed %llu/%llu\n",
				trial, count, m_core.m_pc, expected.m_pc, m_core.m_acc, expected.m_acc,
				expected.m_op, (unsigned long long)m_elapsed_ticks, (unsigned long long) expected_ticks);
			abort();
		}
	}
	dspp_jit_clear();
	printf("%u randomized native blocks match the interpreter\n", trials);
}

static void test_invalidation(void)
{
	m2_dspp_reset();
	dspp_host_write(0, 0x4400);
	dspp_host_write(1, 0xc001);
	dspp_host_write(2, 0x8020);
	dspp_host_write(3, 0xa000);
	assert_always(!dspp_jit_run(2) && !m_core.m_pc, "Block ran without enough cycles for its operands");
	assert_always(dspp_jit_run(3) && m_data_ram[0x20] == 1, "Expected native RAM write");
	dspp_host_write(1, 0xc003);
	m_core.m_pc = 0;
	assert_always(dspp_jit_run(3) && m_data_ram[0x20] == 3, "Operand write failed to invalidate the block");

	dspp_host_write(3, 0x4400);
	dspp_host_write(4, 0xa000);
	dspp_host_write(5, 0x8020);
	dspp_host_write(6, 0xa000);
	m_core.m_rbase[0] = 0x304;
	m_core.m_pc = 3;
	assert_always(!dspp_jit_run(3) && m_core.m_pc == 3, "Register mapped to I/O must use the interpreter");
}

static void run_output_wait(int native, unsigned start, unsigned depth, int writeback, bool dma,
	int changed_word, UINT16 replacement, unsigned budget)
{
	unsigned pc = start;

	test_time = make_mame_time(0, 0);
	event_count = 0;
	options.jit = native;
	m2_dspp_reset();
	emit(&pc, 0x4640);
	emit(&pc, 0x83df);
	emit(&pc, 0xc007);
	emit(&pc, 0xe800 | (start & 0x3ff));
	emit(&pc, 0x8380);
	if (changed_word >= 0)
		dspp_host_write((start + changed_word) & 0x3ff, replacement);
	m_core.m_pc = start;
	m_core.m_dspx_control = DSPX_CONTROL_GWILLING;
	m_core.m_writeback = writeback;
	m_core.m_acc = 0x12340;
	m_core.m_tclock = 3;
	m_core.m_flag_neg = m_core.m_flag_over = m_core.m_flag_carry = 1;
	m_dsp.m_output_fifo_count = depth;
	if (dma)
	{
		m_dsp.m_dspx_channel_enable = 1;
		m_dsp.m_fifo_dma[0].m_current_addr = 0x100;
		m_dsp.m_fifo_dma[0].m_current_count = 1;
	}

	test_time = double_to_mame_time((budget + 0.25) / M2_CLOCK);
	dspp_run();
}

static void test_output_wait(void)
{
	static const unsigned starts[] = {6, 0xfffe};
	static const struct
	{
		unsigned depth;
		int writeback, changed_word;
		bool dma;
		UINT16 replacement;
	} cases[] = {
		{7, -1, -1, false, 0},
		{8, -1, -1, false, 0},
		{6, -1, -1, false, 0},
		{8, 0x20, -1, false, 0},
		{8, -1, -1, true, 0},
		{8, -1, 0, false, 0x4620},
		{8, -1, 1, false, 0x83de},
		{8, -1, 2, false, 0xc009},
		{8, -1, 3, false, 0x8380}
	};
	static struct event expected_events[ARRAY_LENGTH(events)];
	unsigned i, j, budget;

	for (i = 0; i < ARRAY_LENGTH(starts); ++i)
		for (j = 0; j < ARRAY_LENGTH(cases); ++j)
			for (budget = 1; budget <= 20; ++budget)
			{
				__typeof__(m_core) expected_core;
				__typeof__(m_dsp) expected_dsp;
				UINT64 expected_ticks;
				UINT16 expected_ram;
				unsigned expected_count;
#if DSPP_JIT_STATS
				UINT64 skipped = m_jit_stats.wait_skipped;
#endif

				run_output_wait(0, starts[i], cases[j].depth, cases[j].writeback, cases[j].dma,
					cases[j].changed_word, cases[j].replacement, budget);
				memcpy(&expected_core, &m_core, sizeof(m_core));
				memcpy(&expected_dsp, &m_dsp, sizeof(m_dsp));
				expected_ticks = m_elapsed_ticks;
				expected_ram = m_data_ram[0x20];
				expected_count = event_count;
				memcpy(expected_events, events, event_count * sizeof(*events));

				run_output_wait(1, starts[i], cases[j].depth, cases[j].writeback, cases[j].dma,
					cases[j].changed_word, cases[j].replacement, budget);
				assert_always(!memcmp(&expected_core, &m_core, sizeof(m_core)) &&
					!memcmp(&expected_dsp, &m_dsp, sizeof(m_dsp)) &&
					expected_ticks == m_elapsed_ticks && expected_ram == m_data_ram[0x20] &&
					expected_count == event_count && !memcmp(expected_events, events, event_count * sizeof(*events)),
					"Output FIFO wait differs from the interpreter");
#if DSPP_JIT_STATS
				if (j == 0 && budget >= 8)
					assert_always(m_jit_stats.wait_skipped - skipped == budget / 4 - 1,
						"Expected all whole iterations after the first to be skipped");
				if (cases[j].changed_word >= 0)
					assert_always(m_jit_stats.wait_skipped == skipped, "Modified loop was skipped");
#endif
			}
	dspp_jit_clear();
	puts("Output FIFO wait matches the interpreter with partial budgets, DMA, writeback and modified code");
}

static void run_audio_program(int native)
{
	static const UINT16 program[] =
	{
		0x4400, 0x8304, 0x8020,         /* Pop a DMA sample into RAM. */
		0x6520, 0x8020, 0xc001, 0x8020, /* Add an immediate to the sample. */
		0x4400, 0x8020, 0x82e0,         /* Copy to both DAC channels. */
		0x4400, 0x8020, 0x82e1,
		0x4400, 0xc001, 0x83d7,         /* Commit the stereo sample. */
		0xa000
	};
	unsigned frame;

	test_time = make_mame_time(0, 0);
	event_count = 0;
	options.jit = native;
	m2_dspp_reset();
	for (frame = 0; frame < ARRAY_LENGTH(program); ++frame)
		m2_dspp_write(frame, program[frame]);
	m_dsp.m_dspx_channel_enable = 1;
	m_dsp.m_dspx_int_enable = DSPX_F_INT_TIMER | DSPX_F_INT_DMANEXT;
	m_dsp.m_dspx_dmanext_enable = 1;
	m_dsp.m_dspx_audio_duration = 2;
	m_dsp.m_fifo_dma[0].m_current_count = 5;
	m_dsp.m_fifo_dma[0].m_next_addr = 0x100;
	m_dsp.m_fifo_dma[0].m_next_count = 17;
	m_dsp.m_fifo_dma[0].m_next_valid = 1;
	m_dsp.m_fifo_dma[0].m_go_forever = 1;
	m_core.m_dspx_control = DSPX_CONTROL_GWILLING;
	for (frame = 1; frame <= 96; ++frame)
	{
		test_time = double_to_mame_time((frame * 47 + frame % 3) / (double)M2_CLOCK);
		dspp_sample(0);
		record(PC, 0, m_core.m_pc);
		record(CLOCK, 0, m_core.m_tclock);
		record(ACC, 0, m_core.m_acc);
		record(FIFO_DEPTH, 0, m_dsp.m_fifo_dma[0].m_depth);
		record(DMA_COUNT, 0, m_dsp.m_fifo_dma[0].m_current_count);
		if (frame == 17)
			m2_dspp_write(5, 0xc005);
	}
	if (native)
		assert_always(m_jit_blocks[3].code, "Audio program never compiled its arithmetic block");
}

static void test_audio_program(void)
{
	static struct event expected[ARRAY_LENGTH(events)];
	unsigned count, i;
	bool irq = false, audio = false, dma = false;

	run_audio_program(0);
	count = event_count;
	for (i = 0; i < count; ++i)
	{
		irq |= events[i].type == IRQ && events[i].value;
		audio |= events[i].type == DAC && events[i].value != 0x8000;
		dma |= events[i].type == BUS_READ;
	}
	assert_always(irq && audio && dma, "Audio scenario must exercise DMA, IRQ and the DAC");
	memcpy(expected, events, count * sizeof(*expected));
	run_audio_program(1);
	assert_always(count == event_count, "Native and interpreted modes produced different numbers of events");
	for (i = 0; i < count; ++i)
	{
		assert_always(expected[i].tick == events[i].tick && expected[i].type == events[i].type &&
			expected[i].address == events[i].address && expected[i].value == events[i].value,
			"Native and interpreted events differ");
	}
	dspp_jit_clear();
	printf("%u PC, clock, DMA, IRQ and DAC events match the interpreter\n", count);
}
#endif

int main(void)
{
	test_fifo_arbitration();
#if SLJIT_MAME_SUPPORTED
	test_arithmetic();
	test_invalidation();
	test_output_wait();
	test_audio_program();
#endif
	return 0;
}

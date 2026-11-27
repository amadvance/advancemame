// license:BSD-3-Clause
// copyright-holders:Philip Bennett, Ryan Holtz
/* 3DO M2 Bulldog audio DSP */

#include "driver.h"
#include "includes/konamim2.h"
#include "sound/dac.h"
#include "sljit_mame.h"

#include <stdbool.h>

enum
{
	DSPX_CONTROL_GWILLING             = 0x0001,
	DSPX_CONTROL_STEP_CYCLE           = 0x0002,
	DSPX_CONTROL_STEP_PC              = 0x0004,
	DSPX_CONTROL_SNOOP                = 0x0008,

	DSPX_RESET_DSPP                   = 0x0001,
	DSPX_RESET_INPUT                  = 0x0002,
	DSPX_RESET_OUTPUT                 = 0x0004,

	DSPX_F_DMA_NEXTVALID              = 0x0001,
	DSPX_F_DMA_GO_FOREVER             = 0x0002,
	DSPX_F_INT_DMANEXT_EN             = 0x0004,
	DSPX_F_SHADOW_SET_DMANEXT         = 0x00040000,
	DSPX_F_SHADOW_SET_FOREVER         = 0x00020000,
	DSPX_F_SHADOW_SET_NEXTVALID       = 0x00010000,
	DSPX_F_SHADOW_SET_ADDRESS_COUNT   = 0x80000000,

	DSPX_F_INT_TIMER                  = 0x00000100,
	DSPX_F_INT_INPUT_UNDER            = 0x00000080,
	DSPX_F_INT_INPUT_OVER             = 0x00000040,
	DSPX_F_INT_OUTPUT_UNDER           = 0x00000020,
	DSPX_F_INT_OUTPUT_OVER            = 0x00000010,
	DSPX_F_INT_UNDEROVER              = 0x00000008,
	DSPX_F_INT_CONSUMED               = 0x00000002,
	DSPX_F_INT_DMANEXT                = 0x00000001,

	DSPX_F_INT_ALL_DMA                = (DSPX_F_INT_DMANEXT | DSPX_F_INT_CONSUMED | DSPX_F_INT_UNDEROVER),

	DSPX_FLD_INT_SOFT_WIDTH           = 16,

	DSPX_FLD_INT_SOFT_SHIFT           = 16,
	DSPX_FLD_INT_SOFT_MASK            = 0xffff0000
};

#define PC_STACK_DEPTH (4)
#define MAX_OPERANDS (8)

#define NUM_DMA_CHANNELS (32)
#define DMA_FIFO_DEPTH (8)
#define DMA_FIFO_MASK (DMA_FIFO_DEPTH - 1)
#define DMA_FIFO_THRESHOLD (DMA_FIFO_DEPTH / 2)

#define NUM_INPUTS (2)
#define NUM_OUTPUTS (8)
#define OUTPUT_FIFO_DEPTH (8)
#define OUTPUT_FIFO_MASK (OUTPUT_FIFO_DEPTH - 1)

static struct
{
	UINT16    m_pc;
	UINT16    m_stack[PC_STACK_DEPTH];
	UINT32    m_stack_ptr;
	UINT16    m_rbase[4];
	UINT32    m_acc;
	UINT32    m_tclock;

	UINT32    m_flag_carry;
	UINT32    m_flag_zero;
	UINT32    m_flag_neg;
	UINT32    m_flag_over;
	UINT32    m_flag_exact;
	UINT32    m_flag_audlock;
	UINT32    m_flag_sleep;

	UINT32    m_partial_int;
	UINT16    m_op;
	UINT32    m_opidx;
	INT32     m_writeback;
	UINT32    m_jmpdest;

	struct
	{
		UINT32 value;
		UINT32 addr;
	} m_operands[MAX_OPERANDS];

	UINT32    m_dspx_control;
} m_core;

static struct
{
	struct fifo_dma
	{
		UINT32    m_current_addr;
		INT32     m_current_count;
		UINT32    m_next_addr;
		UINT32    m_next_count;
		UINT32    m_prev_value;
		UINT32    m_prev_current;
		UINT8     m_go_forever;
		UINT8     m_next_valid;
		UINT8     m_reserved;
		UINT16    m_fifo[DMA_FIFO_DEPTH];
		UINT32    m_dma_ptr;
		UINT32    m_dspi_ptr;
		UINT32    m_depth;
	} m_fifo_dma[NUM_DMA_CHANNELS];

	UINT32    m_fifo_input_ready;
	UINT32    m_fifo_output_ready;
	UINT32    m_last_osc_count;
	UINT32    m_osc_phase;
	UINT32    m_osc_freq;

	UINT16    m_outputs[NUM_OUTPUTS];
	UINT16    m_output_fifo[OUTPUT_FIFO_DEPTH];
	UINT32    m_output_fifo_start;
	UINT32    m_output_fifo_count;

	UINT32    m_dspx_int_enable;
	UINT32    m_dspx_channel_enable;
	UINT32    m_dspx_channel_complete;
	UINT32    m_dspx_channel_direction;
	UINT32    m_dspx_channel_8bit;
	UINT32    m_dspx_channel_sqxd;
	UINT32    m_dspx_shadow_current_addr;
	UINT32    m_dspx_shadow_current_count;
	UINT32    m_dspx_shadow_next_addr;
	UINT32    m_dspx_shadow_next_count;
	UINT32    m_dspx_dmanext_int;
	UINT32    m_dspx_dmanext_enable;
	UINT32    m_dspx_consumed_int;
	UINT32    m_dspx_consumed_enable;
	UINT32    m_dspx_underover_int;
	UINT32    m_dspx_underover_enable;
	UINT32    m_dspx_audio_time;
	UINT16    m_dspx_audio_duration;
} m_dsp;

static UINT16 m_program[0x400], m_data_ram[0x2e0];
static const UINT8 dspp_shifts[8] = { 0, 1, 2, 3, 4, 5, 8, 16 };
static mame_timer *m_output_timer;
static UINT64 m_elapsed_ticks;

INLINE INT32 dspp_sign_extend(UINT32 value, unsigned bits)
{
	UINT32 sign = 1U << (bits - 1);
	return ((value & ((1U << bits) - 1)) ^ sign) - sign;
}

static UINT16 dspp_data_r(offs_t address);
static void dspp_data_w(offs_t address, UINT16 data);

static void dspp_reset_core(void);
INLINE void dspp_update_pc(void);
INLINE void dspp_update_ticks(void);
static UINT16 dspp_read_op(offs_t pc);
INLINE UINT16 dspp_read_data(offs_t addr);
INLINE void dspp_write_data(offs_t addr, UINT16 data);
static UINT32 dspp_get_interrupt_state(void);
static void dspp_update_host_interrupt(void);
static void dspp_parse_operands(UINT32 numops);
static UINT16 dspp_read_next_operand(void);
static void dspp_write_next_operand(UINT16 value);
INLINE void dspp_push_pc(void);
INLINE UINT16 dspp_pop_pc(void);
INLINE void dspp_set_rbase(UINT32 base, UINT32 addr);
INLINE UINT16 dspp_translate_reg(UINT16 reg);
INLINE void dspp_exec_super_special(void);
INLINE void dspp_exec_special(void);
static void dspp_exec_branch(void);
INLINE void dspp_exec_complex_branch(void);
INLINE void dspp_exec_control(void);
INLINE void dspp_exec_arithmetic(void);
static void dspp_write_dma_to_fifo(INT32 channel, INT16 value);
static void dspp_write_dspp_to_fifo(INT32 channel, INT16 value);
static INT16 dspp_read_fifo_to_dspp(INT32 channel);
static INT16 dspp_read_fifo_to_dma(INT32 channel);
static void dspp_run_oscillator(INT32 channel);
static void dspp_advance_audio_timer(void);
static void dspp_advance_audio_frame(void);
static void dspp_process_next_dma(INT32 channel);
static INT16 dspp_decode_sqxd(INT8 data, INT16 prev);
static void dspp_service_output_dma(INT32 channel);
static void dspp_service_input_dma(INT32 channel);
static void dspp_reset_channel(INT32 channel);
static UINT16 dspp_input_r(void);
static void dspp_output_w(offs_t offset, UINT16 data);
static UINT16 dspp_fifo_osc_r(offs_t offset);
static void dspp_fifo_osc_w(offs_t offset, UINT16 data);
static void dspp_input_control_w(UINT16 data);
static void dspp_output_control_w(UINT16 data);
static UINT16 dspp_input_status_r(void);
static UINT16 dspp_output_status_r(void);
static void dspp_cpu_int_w(UINT16 data);
static UINT16 dspp_pc_r(void);
static void dspp_pc_w(UINT16 data);
static UINT16 dspp_audlock_r(void);
static void dspp_audlock_w(UINT16 data);
static UINT16 dspp_clock_r(void);
static void dspp_clock_w(UINT16 data);
static UINT16 dspp_noise_r(void);
static UINT32 dspp_read_ext_control(offs_t offset);
static void dspp_write_ext_control(offs_t offset, UINT32 data);
static UINT32 dspp_host_read(offs_t offset);
static UINT32 dspp_read_dma_stack(offs_t offset);
static void dspp_write_dma_stack(offs_t offset, UINT32 data);
static void dspp_host_write(offs_t offset, UINT32 data);
static UINT16 dspp_read_output_fifo(void);

static void dspp_reset_core(void)
{
	m_core.m_pc = 0;
	m_core.m_stack_ptr = 0;
	m_dsp.m_output_fifo_start = 0;
	m_dsp.m_output_fifo_count = 0;

	m_core.m_flag_audlock = 0;
	m_core.m_flag_sleep = 0;
	m_core.m_writeback = ~1;
	dspp_set_rbase(0, 0);

	m_core.m_partial_int = 0;
	dspp_update_host_interrupt();
}

INLINE void dspp_update_pc(void)
{
	++m_core.m_pc;
}

INLINE void dspp_update_ticks(void)
{
	--m_core.m_tclock;
	++m_elapsed_ticks;
}

static UINT16 dspp_read_op(offs_t pc)
{
	return m_program[pc & 0x3ff];
}

INLINE UINT16 dspp_read_data(offs_t addr)
{
	return dspp_data_r(addr & 0x3ff);
}

INLINE void dspp_write_data(offs_t addr, UINT16 data)
{
	dspp_data_w(addr & 0x3ff, data);
}

static UINT32 dspp_get_interrupt_state(void)
{
	UINT32 host_int = m_core.m_partial_int;

	if (m_dsp.m_dspx_dmanext_int & m_dsp.m_dspx_dmanext_enable)
		host_int |= DSPX_F_INT_DMANEXT;

	if (m_dsp.m_dspx_consumed_int & m_dsp.m_dspx_consumed_enable)
		host_int |= DSPX_F_INT_CONSUMED;

	return host_int;
}

static void dspp_update_host_interrupt(void)
{
	m2_irq(1U << 25, (dspp_get_interrupt_state() & m_dsp.m_dspx_int_enable) != 0);
}

static void dspp_parse_operands(UINT32 numops)
{
	UINT32 addr, val = 0xBAD;
	UINT32 opidx = 0;
	UINT32 operand = 0;
	UINT32 numregs = 0;

	for (UINT32 i = 0; i < MAX_OPERANDS; ++i)
	{
		m_core.m_operands[i].value = -1;
		m_core.m_operands[i].addr = -1;
	}

	m_core.m_opidx = 0;

	while (opidx < numops)
	{
		operand = dspp_read_op(m_core.m_pc);
		dspp_update_pc();
		dspp_update_ticks();

		if (operand & 0x8000)
		{
			if ((operand & 0xc000) == 0xc000)
			{
				if (operand & 0x2000)
				{
					val = (operand & 0x1fff) << 3;
				}
				else
				{
					val = (UINT16)(dspp_sign_extend(operand, 13));
				}
				m_core.m_operands[opidx++].value = val;
			}
			else if((operand & 0xe000) == 0x8000)
			{
				addr = operand & 0x03ff;

				if (operand & 0x0400)
				{
					addr = dspp_read_data(addr);
				}
				m_core.m_operands[opidx].addr = addr;

				if (operand & 0x0800)
				{
					m_core.m_writeback = addr;
				}
				opidx++;
			}
			else if ((operand & 0xe000) == 0xa000)
			{
				numregs = (operand & 0x0400) ? 2 : 1;
			}
		}
		else
		{
			numregs = 3;
		}

		if (numregs > 0)
		{
			UINT32 shifter, regdi;

			for (UINT32 i = 0; i < numregs; ++i)
			{
				shifter = ((numregs - i) - 1) * 5;
				regdi = (operand >> shifter) & 0x1f;
				addr = dspp_translate_reg(regdi & 0xf);

				if (regdi & 0x0010)
				{
					addr = dspp_read_data(addr);
				}

				if (numregs == 2)
				{
					if ((i == 0) && (operand & 0x1000))
						m_core.m_writeback = addr;
					else if ((i == 1) && (operand & 0x0800))
						m_core.m_writeback = addr;
				}
				else if (numregs == 1)
				{
					if (operand & 0x800)
						m_core.m_writeback = addr;
				}

				m_core.m_operands[opidx++].addr = addr;
			}
			numregs = 0;
		}
	}
}

static UINT16 dspp_read_next_operand(void)
{
	INT32 value = m_core.m_operands[m_core.m_opidx].value;

	if (value < 0)
	{
		value = dspp_read_data(m_core.m_operands[m_core.m_opidx].addr);
	}

	++m_core.m_opidx;

	return value;
}

static void dspp_write_next_operand(UINT16 value)
{
	INT32 addr = m_core.m_operands[m_core.m_opidx].addr;

	assert(addr != -1);

	dspp_write_data(addr, value);

	++m_core.m_opidx;
}

INLINE void dspp_push_pc(void)
{
	if (m_core.m_stack_ptr < PC_STACK_DEPTH)
		m_core.m_stack[m_core.m_stack_ptr++] = m_core.m_pc;
	else
		fatalerror("DSPP stack overflow!");
}

INLINE UINT16 dspp_pop_pc(void)
{
	if (m_core.m_stack_ptr == 0)
		fatalerror("DSPP stack underflow!");

	return m_core.m_stack[--m_core.m_stack_ptr];
}

INLINE void dspp_set_rbase(UINT32 base, UINT32 addr)
{
	switch (base)
	{
		case 4:
			m_core.m_rbase[1] = addr + 4 - base;
			break;
		case 0:
			m_core.m_rbase[0] = addr;
			m_core.m_rbase[1] = addr + 4 - base;

		case 8:
			m_core.m_rbase[2] = addr + 8 - base;

		case 12:
			m_core.m_rbase[3] = addr + 12 - base;
			break;
	}
}

INLINE UINT16 dspp_translate_reg(UINT16 reg)
{
	UINT32 base = (reg >> 2) & 3;
	return m_core.m_rbase[base] + reg - (reg & ~3);
}

INLINE void dspp_exec_super_special(void)
{
	UINT32 sel = (m_core.m_op >> 7) & 7;

	switch (sel)
	{
		case 1:
		{
			m_core.m_pc = m_core.m_acc >> 4;
			break;
		}
		case 4:
		{
			m_core.m_pc = dspp_pop_pc();
			break;
		}
		case 5:
		{
			break;
		}

		case 7:
		{
			--m_core.m_pc;
			m_core.m_flag_sleep = 1;
			break;
		}

		case 0:
		case 2:
		case 3:
		case 6:
			break;
	}
}

INLINE void dspp_exec_special(void)
{
	switch ((m_core.m_op >> 10) & 7)
	{
		case 0:
		{
			dspp_exec_super_special();
			break;
		}
		case 1:
		{
			m_core.m_pc = m_core.m_op & 0x3ff;
			break;
		}
		case 2:
		{
			dspp_push_pc();
			m_core.m_pc = m_core.m_op & 0x3ff;
			break;
		}
		case 3:
		{
			break;
		}
		case 4:
		{
			UINT32 regdi = m_core.m_op & 0x3f;
			UINT32 addr = dspp_translate_reg(regdi & 0xf);

			if (regdi & 0x0010)
			{
				addr = dspp_read_data(addr);
			}

			dspp_parse_operands(1);
			dspp_write_data(addr, dspp_read_next_operand());
			break;
		}
		case 5:
		{
			dspp_set_rbase((m_core.m_op & 3) << 2, m_core.m_op & 0x3fc);
			break;
		}
		case 6:
		{
			dspp_parse_operands(1);
			dspp_write_data(m_core.m_op & 0x3ff, dspp_read_next_operand());
			break;
		}
		case 7:
		{
			dspp_parse_operands(1);
			UINT32 addr = dspp_read_data(m_core.m_op & 0x3ff);
			dspp_write_data(addr, dspp_read_next_operand());
			break;
		}

		default:
			break;
	}
}

static void dspp_exec_branch(void)
{
	UINT32 mode = (m_core.m_op >> 13) & 3;
	UINT32 select = (m_core.m_op >> 12) & 1;
	UINT32 mask = (m_core.m_op >> 10) & 3;

	bool flag0, flag1;

	if (select == 0)
	{
		flag0 = m_core.m_flag_neg;
		flag1 = m_core.m_flag_over;
	}
	else
	{
		flag0 = m_core.m_flag_carry;
		flag1 = m_core.m_flag_zero;
	}

	bool mask0 = (mask & 2) != 0;
	bool mask1 = (mask & 1) != 0;

	bool branch = (flag0 || !mask0) && (flag1 || !mask1);

	if (mode == 2)
		branch = !branch;

	if (branch)
		m_core.m_pc = m_core.m_op & 0x3ff;
}

INLINE void dspp_exec_complex_branch(void)
{
	UINT32 type = (m_core.m_op >> 10) & 7;

	const bool c = m_core.m_flag_carry;
	const bool z = m_core.m_flag_zero;
	const bool n = m_core.m_flag_neg;
	const bool v = m_core.m_flag_over;
	const bool x = m_core.m_flag_exact;

	bool branch = false;

	switch (type)
	{
		case 0:
			branch = (n && !v) || (!n && v);
			break;
		case 1:
			branch = ((n && !v) || (!n && v)) || z;
			break;
		case 2:
			branch = ((n && v) || (!n && !v));
			break;
		case 3:
			branch = ((n && v) || (!n && !v)) && !z;
			break;
		case 4:
			branch = c && !z;
			break;
		case 5:
			branch = !c || z;
			break;
		case 6:
			branch = x;
			break;
		case 7:
			branch = !x;
			break;
	}

	if (branch)
		m_core.m_pc = m_core.m_op & 0x3ff;
}

INLINE void dspp_exec_control(void)
{
	UINT32 mode = (m_core.m_op >> 13) & 3;

	switch (mode)
	{
		case 0:
		{
			dspp_exec_special();
			break;
		}

		case 1: case 2:
		{
			dspp_exec_branch();
			break;
		}

		case 3:
		{
			dspp_exec_complex_branch();
			break;
		}

		default:
			fatalerror("Invalid DSPP control instruction mode");
			break;
	}
}

INLINE UINT32 dspp_arithmetic_numops(UINT16 op)
{
	UINT32 numops = (op >> 13) & 3;
	UINT32 muxa = (op >> 10) & 3;
	UINT32 muxb = (op >> 8) & 3;

	if (numops == 0 && ((muxa == 1) || (muxa == 2) || (muxb == 1) || (muxb == 2)))
		numops = 4;

	return numops + ((op & 0xf) == 8);
}

INLINE void dspp_exec_arithmetic(void)
{
	UINT32 numops = dspp_arithmetic_numops(m_core.m_op);
	UINT32 muxa = (m_core.m_op >> 10) & 3;
	UINT32 muxb = (m_core.m_op >> 8) & 3;
	UINT32 alu_op = (m_core.m_op >> 4) & 0xf;
	UINT32 barrel_code = m_core.m_op & 0xf;

	INT32 mul_res = 0;
	UINT32 alu_res = 0;

	dspp_parse_operands(numops);

	if (muxa == 3 || muxb == 3)
	{
		UINT32 mul_sel = (m_core.m_op >> 12) & 1;

		INT32 op1 = (INT16)(dspp_read_next_operand());
		INT32 op2 = (INT16)(mul_sel ? dspp_read_next_operand() : m_core.m_acc >> 4);

		mul_res = (op1 * op2) >> 11;
	}

	INT32 alu_a, alu_b;

	switch (muxa)
	{
		case 0:
		{
			alu_a = m_core.m_acc;
			break;
		}
		case 1: case 2:
		{
			alu_a = dspp_read_next_operand() << 4;

			break;
		}
		case 3:
		{
			alu_a = mul_res;
			break;
		}
	}

	switch (muxb)
	{
		case 0:
		{
			alu_b = m_core.m_acc;
			break;
		}
		case 1: case 2:
		{
			alu_b = dspp_read_next_operand() << 4;
			break;
		}
		case 3:
		{
			alu_b = mul_res;
			break;
		}
	}

	alu_a &= 0x00fffff;
	alu_b &= 0x00fffff;

	switch (alu_op)
	{
		case 0:
		{
			alu_res = alu_a;
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 1:
		{
			alu_res = -alu_b;
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 2:
		{
			alu_res = alu_a + alu_b;
			m_core.m_flag_over = (((alu_a & 0x80000) == (alu_b & 0x80000) && (alu_a & 0x80000) != (alu_res & 0x80000)));
			m_core.m_flag_carry = (alu_res & 0x00100000) != 0;
			break;
		}
		case 3:
		{
			alu_res = alu_a + (m_core.m_flag_carry << 4);
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = (alu_res & 0x00100000) != 0;
			break;
		}
		case 4:
		{
			alu_res = alu_a - alu_b;
			m_core.m_flag_over = ((alu_a & 0x80000) == (~alu_b & 0x80000) && (alu_a & 0x80000) != (alu_res & 0x80000));
			m_core.m_flag_carry = (alu_res & 0x00100000) != 0;
			break;
		}
		case 5:
		{
			alu_res = alu_a - (m_core.m_flag_carry << 4);
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = (alu_res & 0x00100000) != 0;
			break;
		}
		case 6:
		{
			alu_res = alu_a + 1;
			m_core.m_flag_over = !(alu_a & 0x80000) && (alu_res & 0x80000);
			m_core.m_flag_carry = 0;
			break;
		}
		case 7:
		{
			alu_res = alu_a - 1;
			m_core.m_flag_over = (alu_a & 0x80000) && !(alu_res & 0x80000);
			m_core.m_flag_carry = 0;
			break;
		}
		case 8:
		{
			alu_res = alu_a;
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 9:
		{
			alu_res = ~alu_a;
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 10:
		{
			alu_res = alu_a & alu_b;
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 11:
		{
			alu_res = ~(alu_a & alu_b);
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 12:
		{
			alu_res = alu_a | alu_b;
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 13:
		{
			alu_res = ~(alu_a | alu_b);
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 14:
		{
			alu_res = alu_a ^ alu_b;
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
		case 15:
		{
			alu_res = ~(alu_a ^ alu_b);
			m_core.m_flag_over = 0;
			m_core.m_flag_carry = 0;
			break;
		}
	}

	m_core.m_flag_neg = (alu_res & 0x00080000) != 0;
	m_core.m_flag_zero = (alu_res & 0x000ffff0) == 0;
	m_core.m_flag_exact = (alu_res & 0x0000000f) == 0;

	if (barrel_code == 8)
		barrel_code = dspp_read_next_operand();

	if (barrel_code & 8)
	{
		UINT32 shift = dspp_shifts[(~barrel_code + 1) & 7];

		if (alu_op < 8)
		{
			m_core.m_acc = dspp_sign_extend(alu_res, 20) >> shift;
		}
		else
		{
			m_core.m_acc = (alu_res & 0xfffff) >> shift;
		}
	}
	else
	{
		UINT32 shift = dspp_shifts[barrel_code];

		if (shift == 16)
		{
			if (m_core.m_flag_over)
				m_core.m_acc = m_core.m_flag_neg ? 0x7ffff : 0xfff80000;
			else
				m_core.m_acc = dspp_sign_extend(alu_res, 20);
		}
		else
		{
			m_core.m_acc = dspp_sign_extend(alu_res << shift, 20);
		}
	}

	if (m_core.m_writeback >= 0)
	{
		dspp_write_data(m_core.m_writeback, m_core.m_acc >> 4);
		m_core.m_writeback = -1;
	}
	else if (m_core.m_opidx < numops)
	{
		dspp_write_next_operand(m_core.m_acc >> 4);
	}
}

INLINE void dspp_set_fifo_depth(INT32 channel, UINT32 depth)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];
	UINT32 bit = 1U << channel;

	/* At the threshold the FIFO can accept or supply a DMA burst. */
	if ((dma->m_depth <= DMA_FIFO_THRESHOLD) != (depth <= DMA_FIFO_THRESHOLD))
		m_dsp.m_fifo_input_ready ^= bit;
	if ((dma->m_depth >= DMA_FIFO_THRESHOLD) != (depth >= DMA_FIFO_THRESHOLD))
		m_dsp.m_fifo_output_ready ^= bit;
	dma->m_depth = depth;
}

static void dspp_write_dma_to_fifo(INT32 channel, INT16 value)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	dma->m_fifo[dma->m_dma_ptr] = value;

	if (dma->m_depth < DMA_FIFO_DEPTH)
	{
		dma->m_dma_ptr = (dma->m_dma_ptr + 1) & DMA_FIFO_MASK;
		dspp_set_fifo_depth(channel, dma->m_depth + 1);
	}
	else
	{
		fatalerror("DMA TO FIFO OVERFLOW");
	}
}

static void dspp_write_dspp_to_fifo(INT32 channel, INT16 value)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	dma->m_fifo[dma->m_dspi_ptr] = value;

	if (dma->m_depth < DMA_FIFO_DEPTH)
	{
		dma->m_dspi_ptr = (dma->m_dspi_ptr + 1) & DMA_FIFO_MASK;
		dspp_set_fifo_depth(channel, dma->m_depth + 1);
	}
	else
	{
		fatalerror("DSPP TO FIFO OVERFLOW");
	}
}

static INT16 dspp_read_fifo_to_dspp(INT32 channel)
{
	INT16 data = 0;

	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	if (dma->m_depth > 0)
	{
		data = dma->m_fifo[dma->m_dspi_ptr];

		dma->m_dspi_ptr = (dma->m_dspi_ptr + 1) & DMA_FIFO_MASK;
		dspp_set_fifo_depth(channel, dma->m_depth - 1);

		if (dma->m_depth == 0)
		{
			if (m_dsp.m_dspx_channel_complete & (1U << channel))
			{
				m_dsp.m_dspx_consumed_int |= 1U << channel;
				dspp_update_host_interrupt();
			}
		}

		dma->m_prev_current = data;
	}
	else
	{
		m_dsp.m_dspx_underover_int |= 1U << channel;
		dspp_update_host_interrupt();
		data = dma->m_prev_current;
	}

	return data;
}

static INT16 dspp_read_fifo_to_dma(INT32 channel)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	UINT32 data = dma->m_fifo[dma->m_dma_ptr];

	if (dma->m_depth > 0)
	{
		dma->m_dma_ptr = (dma->m_dma_ptr + 1) & DMA_FIFO_MASK;
		dspp_set_fifo_depth(channel, dma->m_depth - 1);
	}
	else
	{
	}
	return data;
}

static void dspp_run_oscillator(INT32 channel)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	m_dsp.m_osc_phase += m_dsp.m_osc_freq;

	UINT32 count = (m_dsp.m_osc_phase >> 15) & 3;

	m_dsp.m_osc_phase &= 0x7fff;

	if (count > dma->m_depth)
		count = dma->m_depth;

	for (UINT32 i = 0; i < count; ++i)
		dspp_read_fifo_to_dspp(channel);

	m_dsp.m_last_osc_count = count;
}

static void dspp_advance_audio_timer(void)
{
	++m_dsp.m_dspx_audio_time;

	if (--m_dsp.m_dspx_audio_duration == 0xffff)
	{
		m_core.m_partial_int |= DSPX_F_INT_TIMER;
		dspp_update_host_interrupt();
	}
}

static void dspp_advance_audio_frame(void)
{
	dspp_advance_audio_timer();

	if (m_core.m_flag_audlock)
		dspp_reset_core();
}

static void dspp_process_next_dma(INT32 channel)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	if (dma->m_current_count <= 0)
	{
		UINT32 chmask = 1U << channel;

		if (dma->m_next_valid)
		{
			dma->m_current_addr = dma->m_next_addr;
			dma->m_current_count = dma->m_next_count;

			if (!dma->m_go_forever)
				dma->m_next_valid = 0;

			m_dsp.m_dspx_channel_complete &= ~chmask;
			m_dsp.m_dspx_dmanext_int |= chmask;
			dspp_update_host_interrupt();
		}
		else
		{
			m_dsp.m_dspx_channel_enable &= ~chmask;
		}
	}
}

static INT16 dspp_decode_sqxd(INT8 data, INT16 prev)
{
	INT16 temp = (INT8)(data & 0xfe);
	INT32 expanded = temp * abs(temp) * 2;
	INT16 output;

	if (data & 1)
	{
		expanded = expanded >> 2;
		output = expanded + prev;
	}
	else
	{
		output = expanded;
	}

	return output;
}

static void dspp_service_output_dma(INT32 channel)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	if (dma->m_current_count == 0)
		return;

	UINT32 count = dma->m_current_count;

	if (count > 4)
		count = 4;

	for (UINT32 i = 0; i < count; ++i)
	{
		UINT16 sample = dspp_read_fifo_to_dma(channel);

		m2_ram_write_byte(dma->m_current_addr++, sample >> 8);
		m2_ram_write_byte(dma->m_current_addr++, sample & 0xff);
	}

	dma->m_current_count -= count;

	dspp_process_next_dma(channel);
}

static void dspp_service_input_dma(INT32 channel)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	if (dma->m_current_count == 0)
		return;

	UINT32 count = dma->m_current_count;

	if (count > 4)
		count = 4;

	bool is8bit = (m_dsp.m_dspx_channel_8bit & (1U << channel)) != 0;
	bool isSQXD = (m_dsp.m_dspx_channel_sqxd & (1U << channel)) != 0;

	if (is8bit)
	{
		for (UINT32 i = 0; i < count; ++i)
		{
			INT16 sample;
			INT8 curbyte = m2_ram_byte(dma->m_current_addr++);

			if (isSQXD)
			{
				sample = dspp_decode_sqxd(curbyte, dma->m_prev_value);
				dma->m_prev_value = sample;
			}
			else
			{
				sample = curbyte * 256;
			}

			dspp_write_dma_to_fifo(channel, sample);
		}
	}
	else
	{
		for (UINT32 i = 0; i < count; ++i)
		{
			INT16 sample;

			sample = m2_ram_byte(dma->m_current_addr++) << 8;
			sample |= m2_ram_byte(dma->m_current_addr++);

			dspp_write_dma_to_fifo(channel, sample);
		}
	}

	dma->m_current_count -= count;

	dspp_process_next_dma(channel);
}

INLINE UINT32 dspp_fifo_dma_ready(void)
{
	UINT32 direction = m_dsp.m_dspx_channel_direction;
	UINT32 ready = (m_dsp.m_fifo_input_ready & ~direction) |
		(m_dsp.m_fifo_output_ready & direction);

	return ready & m_dsp.m_dspx_channel_enable & ~m_dsp.m_dspx_channel_complete;
}

static void dspp_update_fifo_dma(UINT32 ready)
{
	if (ready)
	{
		unsigned channel = 31 - __builtin_clz(ready);

		if (m_dsp.m_dspx_channel_direction & (1U << channel))
			dspp_service_output_dma(channel);
		else
			dspp_service_input_dma(channel);
	}
}

static void dspp_reset_channel(INT32 channel)
{
	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	m_dsp.m_dspx_channel_complete &= ~(1U << channel);

	dma->m_dma_ptr = 0;
	dma->m_dspi_ptr = 0;
	dspp_set_fifo_depth(channel, 0);
}

static UINT16 dspp_input_r(void)
{
	return 0;
}

static void dspp_output_w(offs_t offset, UINT16 data)
{
	m_dsp.m_outputs[offset] = data;
}

static UINT16 dspp_fifo_osc_r(offs_t offset)
{
	UINT32 data = 0;
	UINT32 channel = offset / 8;

	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

	switch (offset & 7)
	{
		case 0:
		{
			if (dma->m_depth == 0)
				data = dma->m_prev_current;
			else
				data = dma->m_fifo[dma->m_dspi_ptr];

			break;
		}

		case 1:
		{
			if (dma->m_depth == 0)
				data = dma->m_prev_current;
			else if (dma->m_depth == 1)
				data = dma->m_fifo[dma->m_dspi_ptr];
			else
				data = dma->m_fifo[(dma->m_dspi_ptr + 1) & DMA_FIFO_MASK];

			break;
		}

		case 2:
		{
			data = m_dsp.m_last_osc_count;
			break;
		}

		case 3:
		{
			data = m_dsp.m_osc_phase;
			break;
		}

		case 4:
		{
			data = dspp_read_fifo_to_dspp(channel);
			break;
		}

		case 5:
		{
			data = dma->m_depth;
			break;
		}
	}

	return data;
}

static void dspp_fifo_osc_w(offs_t offset, UINT16 data)
{
	UINT32 channel = offset / 8;

	switch (offset & 7)
	{
		case 0:
		{
			break;
		}

		case 1:
		{
			break;
		}

		case 2:
		{
			m_dsp.m_osc_freq = data;
			dspp_run_oscillator(channel);
			break;
		}

		case 3:
		{
			m_dsp.m_osc_phase = data;
			break;
		}

		case 4:
		{
			dspp_write_dspp_to_fifo(channel, data);
			break;
		}

		case 5:
		{
			break;
		}
	}
}

static void dspp_input_control_w(UINT16 data)
{
}

static void dspp_output_control_w(UINT16 data)
{
	if (data & 1)
	{
		UINT32 end;

		if (m_dsp.m_output_fifo_count == OUTPUT_FIFO_DEPTH)
		{
			end = (m_dsp.m_output_fifo_start + m_dsp.m_output_fifo_count) & OUTPUT_FIFO_MASK;

			m_dsp.m_output_fifo_start = (m_dsp.m_output_fifo_start + 2) & OUTPUT_FIFO_MASK;

			m_dsp.m_output_fifo[(end + 0) & OUTPUT_FIFO_MASK] = m_dsp.m_outputs[0];
			m_dsp.m_output_fifo[(end + 1) & OUTPUT_FIFO_MASK] = m_dsp.m_outputs[1];
		}
		else
		{
			end = (m_dsp.m_output_fifo_start + m_dsp.m_output_fifo_count) & OUTPUT_FIFO_MASK;

			m_dsp.m_output_fifo[(end + 0) & OUTPUT_FIFO_MASK] = m_dsp.m_outputs[0];
			m_dsp.m_output_fifo[(end + 1) & OUTPUT_FIFO_MASK] = m_dsp.m_outputs[1];

			m_dsp.m_output_fifo_count += 2;
		}

		dspp_advance_audio_frame();
	}
}

static UINT16 dspp_input_status_r(void)
{
	return 1;
}

static UINT16 dspp_output_status_r(void)
{
	return m_dsp.m_output_fifo_count;
}

static void dspp_cpu_int_w(UINT16 data)
{
	m_core.m_partial_int |= (data << DSPX_FLD_INT_SOFT_SHIFT) & DSPX_FLD_INT_SOFT_MASK;
	dspp_update_host_interrupt();
}

static UINT16 dspp_pc_r(void)
{
	return m_core.m_pc;
}

static void dspp_pc_w(UINT16 data)
{
	m_core.m_pc = data;
	m_core.m_flag_sleep = 0;
}

static UINT16 dspp_audlock_r(void)
{
	return m_core.m_flag_audlock;
}

static void dspp_audlock_w(UINT16 data)
{
	m_core.m_flag_audlock = data & 1;
}

static UINT16 dspp_clock_r(void)
{
	return m_core.m_tclock;
}

static void dspp_clock_w(UINT16 data)
{
	m_core.m_tclock = data;
}

static UINT16 dspp_noise_r(void)
{
	return mame_rand();
}

static UINT32 dspp_read_ext_control(offs_t offset)
{
	UINT32 data = 0;

	switch (offset)
	{
		case 0x4000/4:

		case 0x4004/4:
		{
			data = dspp_get_interrupt_state();
			break;
		}

		case 0x4008/4:

		case 0x400C/4:
		{
			data = m_dsp.m_dspx_int_enable;
			break;
		}

		case 0x4010/4:

		case 0x4014/4:
		{
			data = m_dsp.m_dspx_dmanext_int;
			break;
		}

		case 0x4018/4:
		{
			data = m_dsp.m_dspx_dmanext_enable;
			break;
		}

		case 0x4020/4:

		case 0x4024/4:
		{
			data = m_dsp.m_dspx_consumed_int;
			break;
		}

		case 0x4028/4:

		case 0x402c/4:
		{
			data = m_dsp.m_dspx_consumed_enable;
			break;
		}

		case 0x4030/4:

		case 0x4034/4:
		{
			data = m_dsp.m_dspx_underover_int;
			break;
		}

		case 0x4038/4:

		case 0x403c/4:
		{
			data = m_dsp.m_dspx_underover_enable;
			break;
		}

		case 0x6000/4:

		case 0x6004/4:
		{
			data = m_dsp.m_dspx_channel_enable;
			break;
		}

		case 0x6008/4:

		case 0x600c/4:
		{
			data = m_dsp.m_dspx_channel_direction;
			break;
		}

		case 0x6010/4:

		case 0x6014/4:
		{
			data = m_dsp.m_dspx_channel_8bit;
			break;
		}

		case 0x6018/4:

		case 0x601c/4:
		{
			data = m_dsp.m_dspx_channel_sqxd;
			break;
		}

		case 0x603c/4:
		{
			data = m_dsp.m_dspx_channel_complete;
			break;
		}

		case 0x6040/4:
		{
			data = m_dsp.m_dspx_audio_duration;
			break;
		}

		case 0x6044/4:
		{
			data = m_dsp.m_dspx_audio_time;
			break;
		}

		case 0x6050/4:
		{
			break;
		}

		case 0x6060/4:
		{
			break;
		}

		case 0x6068/4:
		{
			break;
		}

		case 0x6070/4:
		{
			data = m_core.m_dspx_control;
			break;
		}
		default:
		{
			logerror("DSPP: Unhandled external control read (%.4x)\n", offset << 2);
			break;
		}
	}

	return data;
}

static void dspp_write_ext_control(offs_t offset, UINT32 data)
{
	switch (offset)
	{
		case 0x4000/4:
		{
			m_core.m_partial_int |= data & ~DSPX_F_INT_ALL_DMA;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4004/4:
		{
			m_core.m_partial_int &= ~(data & ~DSPX_F_INT_ALL_DMA);
			dspp_update_host_interrupt();
			break;
		}

		case 0x4008/4:
		{
			m_dsp.m_dspx_int_enable |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x400C/4:
		{
			m_dsp.m_dspx_int_enable &= ~data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4010/4:
		{
			m_dsp.m_dspx_dmanext_int |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4014/4:
		{
			m_dsp.m_dspx_dmanext_int &= ~data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4018/4:
		{
			m_dsp.m_dspx_dmanext_enable |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4020/4:
		{
			m_dsp.m_dspx_consumed_int |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4024/4:
		{
			m_dsp.m_dspx_consumed_int &= ~data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4028/4:
		{
			m_dsp.m_dspx_consumed_enable |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x402c/4:
		{
			m_dsp.m_dspx_consumed_enable &= ~data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4030/4:
		{
			m_dsp.m_dspx_underover_int |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4034/4:
		{
			m_dsp.m_dspx_underover_int &= ~data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x4038/4:
		{
			m_dsp.m_dspx_underover_enable |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x403c/4:
		{
			m_dsp.m_dspx_underover_enable &= ~data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x6000/4:
		{
			m_dsp.m_dspx_channel_enable |= data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x6004/4:
		{
			m_dsp.m_dspx_channel_enable &= ~data;
			dspp_update_host_interrupt();
			break;
		}

		case 0x6008/4:
		{
			m_dsp.m_dspx_channel_direction |= data;
			break;
		}

		case 0x600c/4:
		{
			m_dsp.m_dspx_channel_direction &= ~data;
			break;
		}

		case 0x6010/4:
		{
			m_dsp.m_dspx_channel_8bit |= data;
			break;
		}

		case 0x6014/4:
		{
			m_dsp.m_dspx_channel_8bit &= ~data;
			break;
		}

		case 0x6018/4:
		{
			m_dsp.m_dspx_channel_sqxd |= data;
			break;
		}

		case 0x601c/4:
		{
			m_dsp.m_dspx_channel_sqxd &= ~data;
			break;
		}

		case 0x6030/4:
		{
			for (UINT32 i = 0; i < NUM_DMA_CHANNELS; ++i)
			{
				if (data & (1U << i))
					dspp_reset_channel(i);
			}
			break;
		}

		case 0x6040/4:
		{
			m_dsp.m_dspx_audio_duration = data;
			break;
		}

		case 0x6044/4:
		{
			m_dsp.m_dspx_audio_time = data;
			break;
		}

		case 0x6050/4:
		{
			break;
		}

		case 0x6060/4:
		{
			break;
		}

		case 0x6068/4:
		{
			break;
		}

		case 0x6070/4:
		{
			m_core.m_dspx_control = data;
			break;
		}

		case 0x6074/4:
		{
			if (data & 1)
				dspp_reset_core();

			break;
		}
		default:
		{
			logerror("DSPP: Unhandled external control write (%.4x with %.8x)\n", offset << 2, data);
			break;
		}
	}
}

static UINT32 dspp_host_read(offs_t offset)
{
	if (offset < 0x1000/4)
	{
		return m_program[offset];
	}
	else if (offset >= 0x1000/4 && offset < 0x2000/4)
	{
		return dspp_data_r(offset - 0x1000/4);
	}
	else if(offset >= 0x5000/4 && offset < 0x6000/4)
	{
		return dspp_read_dma_stack(offset - 0x5000/4);
	}
	else
	{
		return dspp_read_ext_control(offset);
	}
}

static UINT32 dspp_read_dma_stack(offs_t offset)
{
	UINT32 data = 0;

	if (offset < 0x200 / 4)
	{
		UINT32 channel = offset / (16/4);
		UINT32 reg = offset & 3;

	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

		switch (reg)
		{
			case 0x00/4:
			{
				data = dma->m_current_addr;
				break;
			}
			case 0x04/4:
			{
				data = dma->m_current_count;
				break;
			}
			case 0x08/4:
			{
				data = dma->m_next_addr;
				break;
			}
			case 0x0c/4:
			{
				data = dma->m_next_count;
				break;
			}
		}
	}
	else if (offset >= 0x200/4 && offset < (0x200/4 + (NUM_DMA_CHANNELS * (16/4))))
	{
		UINT32 channel = (offset - 0x200/4) / (16/4);

	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

		data = dma->m_go_forever ? DSPX_F_DMA_GO_FOREVER : 0;
		data |= dma->m_next_valid ? DSPX_F_DMA_NEXTVALID : 0;
	}
	else
	{
		fatalerror("Unhandled DMA stack read");
	}

	return data;
}

static void dspp_write_dma_stack(offs_t offset, UINT32 data)
{
	if (offset < 0x200 / 4)
	{
		switch (offset & 3)
		{
			case 0x00/4:
			{
				m_dsp.m_dspx_shadow_current_addr = data;
				break;
			}
			case 0x04/4:
			{
				m_dsp.m_dspx_shadow_current_count = data;
				break;
			}
			case 0x08/4:
			{
				m_dsp.m_dspx_shadow_next_addr = data;
				break;
			}
			case 0x0c/4:
			{
				m_dsp.m_dspx_shadow_next_count = data;
				break;
			}
		}
	}
	else if (offset >= 0x200/4 && offset < (0x200/4 + (NUM_DMA_CHANNELS * (16/4))))
	{
		UINT32 channel = (offset - 0x200/4) / (16/4);

	struct fifo_dma *dma = &m_dsp.m_fifo_dma[channel];

		if (data & DSPX_F_SHADOW_SET_NEXTVALID)
		{
			dma->m_next_valid = (data & DSPX_F_DMA_NEXTVALID) != 0;
		}
		if (data & DSPX_F_SHADOW_SET_FOREVER)
		{
			dma->m_go_forever = (data & DSPX_F_DMA_GO_FOREVER) != 0;
		}
		if (data & DSPX_F_SHADOW_SET_DMANEXT)
		{
			if (data & DSPX_F_INT_DMANEXT_EN)
			{
				m_dsp.m_dspx_dmanext_enable |= (1U << channel);
			}
			else
			{
				m_dsp.m_dspx_dmanext_enable &= ~(1U << channel);
			}
		}
		if (data & DSPX_F_SHADOW_SET_ADDRESS_COUNT)
		{
			if (offset & (8/4))
			{
				dma->m_next_addr = m_dsp.m_dspx_shadow_next_addr;
				dma->m_next_count = m_dsp.m_dspx_shadow_next_count;
			}
			else
			{
				dma->m_current_addr = m_dsp.m_dspx_shadow_current_addr;
				dma->m_current_count = m_dsp.m_dspx_shadow_current_count;
			}
		}
	}
	else
	{
		fatalerror("Unhandled DMA stack write");
	}
}

#if SLJIT_MAME_SUPPORTED
#include "konamim2jit.c"
#endif

static void dspp_host_write(offs_t offset, UINT32 data)
{
	if (offset < 0x1000/4)
	{
#if SLJIT_MAME_SUPPORTED
		if (m_program[offset] != (UINT16)data)
			m_jit_dirty = 1;
#endif
		m_program[offset] = data;
		if (offset == (m_core.m_pc & 0x3ff))
			m_core.m_flag_sleep = 0;
	}
	else if (offset >= 0x1000/4 && offset < 0x2000/4)
	{
		dspp_data_w(offset - 0x1000/4, data);
	}
	else if(offset >= 0x5000/4 && offset < 0x6000/4)
	{
		dspp_write_dma_stack(offset - 0x5000/4, data);
	}
	else
	{
		dspp_write_ext_control(offset, data);
	}
}

static UINT16 dspp_read_output_fifo(void)
{
	UINT16 data = 0;

	if (m_dsp.m_output_fifo_count == 0)
	{
		return m_dsp.m_output_fifo[m_dsp.m_output_fifo_start];
	}

	data = m_dsp.m_output_fifo[m_dsp.m_output_fifo_start];

	m_dsp.m_output_fifo_start = (m_dsp.m_output_fifo_start + 1) & OUTPUT_FIFO_MASK;
	--m_dsp.m_output_fifo_count;

	return data;
}

static UINT16 dspp_data_r(offs_t address)
{
	if (address < ARRAY_LENGTH(m_data_ram))
		return m_data_ram[address];
	if (address >= 0x300 && address <= 0x37f)
		return dspp_fifo_osc_r(address - 0x300);

	switch (address)
	{
		case 0x2f0:
		case 0x2f1: return dspp_input_r();
		case 0x3de: return dspp_input_status_r();
		case 0x3df: return dspp_output_status_r();
		case 0x3ee: return dspp_pc_r();
		case 0x3f6: return dspp_audlock_r();
		case 0x3f7: return dspp_clock_r();
		case 0x3ff: return dspp_noise_r();
	}
	return 0;
}

static void dspp_data_w(offs_t address, UINT16 data)
{
	if (address < ARRAY_LENGTH(m_data_ram))
		m_data_ram[address] = data;
	else if (address >= 0x2e0 && address <= 0x2e7)
		dspp_output_w(address - 0x2e0, data);
	else if (address >= 0x300 && address <= 0x37f)
		dspp_fifo_osc_w(address - 0x300, data);
	else switch (address)
	{
		case 0x3d6: dspp_input_control_w(data); break;
		case 0x3d7: dspp_output_control_w(data); break;
		case 0x3e6: dspp_cpu_int_w(data); break;
		case 0x3ee: dspp_pc_w(data); break;
		case 0x3f6: dspp_audlock_w(data); break;
		case 0x3f7: dspp_clock_w(data); break;
	}
}

#if SLJIT_MAME_SUPPORTED
static bool dspp_skip_output_wait(UINT64 remaining)
{
	const unsigned loop_ticks = 4;
	const unsigned pc = m_core.m_pc & 0x3ff;
	const UINT16 branch = 0xe800 | pc;
	UINT64 skipped;

	/* Match SUB [output FIFO count], 7 followed by a signed branch back on >=. */
	if (remaining < 2 * loop_ticks || dspp_output_status_r() < 7 ||
		dspp_read_op(pc) != 0x4640 || dspp_read_op(pc + 1) != 0x83df ||
		dspp_read_op(pc + 2) != 0xc007 || dspp_read_op(pc + 3) != branch)
		return false;

	/* Run the comparison once to set the accumulator, flags and operand state. */
	m_core.m_op = 0x4640;
	dspp_update_ticks();
	dspp_update_pc();
	dspp_exec_arithmetic();
	m_core.m_op = branch;
	m_core.m_pc = pc;
	dspp_update_ticks();

	/* The loop has no writes and DMA is idle. The FIFO can drain at the next sample callback. */
	skipped = remaining - remaining % loop_ticks - loop_ticks;
	m_core.m_tclock -= skipped;
	m_elapsed_ticks += skipped;
#if DSPP_JIT_STATS
	++m_jit_stats.wait_batches;
	m_jit_stats.wait_skipped += skipped / loop_ticks;
#endif
	return true;
}
#endif

static void dspp_run(void)
{
	UINT64 now = timer_get_time() * M2_CLOCK;

	/* A CPU may already have run past the time of the next timer callback. */
	if (now <= m_elapsed_ticks)
		return;
	if (!dspp_fifo_dma_ready() &&
		(!(m_core.m_dspx_control & DSPX_CONTROL_GWILLING) || m_core.m_flag_sleep))
	{
		m_core.m_tclock -= now - m_elapsed_ticks;
		m_elapsed_ticks = now;
		return;
	}

	while (m_elapsed_ticks < now)
	{
		UINT32 ready = dspp_fifo_dma_ready();

#if SLJIT_MAME_SUPPORTED
		/* The wait loop and native blocks cannot make a FIFO ready. */
		if (!ready && options.jit && (m_core.m_dspx_control & DSPX_CONTROL_GWILLING) &&
			!m_core.m_flag_sleep && m_core.m_writeback < 0 &&
			(dspp_skip_output_wait(now - m_elapsed_ticks) || dspp_jit_run(now - m_elapsed_ticks)))
			continue;
#endif

		dspp_update_ticks();
		dspp_update_fifo_dma(ready);
		if ((m_core.m_dspx_control & DSPX_CONTROL_GWILLING) && !m_core.m_flag_sleep)
		{
#if SLJIT_MAME_SUPPORTED && DSPP_JIT_STATS
			unsigned pc = m_core.m_pc & 0x3ff;
			UINT64 operand_start = m_elapsed_ticks;
			enum dspp_jit_fallback reason = ready ? DSPP_JIT_FIFO :
				m_core.m_writeback >= 0 ? DSPP_JIT_WRITEBACK : m_jit_fallback;
#endif
			m_core.m_op = dspp_read_op(m_core.m_pc);
#if SLJIT_MAME_SUPPORTED && DSPP_JIT_STATS
			UINT16 opcode = m_core.m_op;
#endif
			dspp_update_pc();
			if (m_core.m_op & 0x8000)
				dspp_exec_control();
			else
				dspp_exec_arithmetic();
#if SLJIT_MAME_SUPPORTED && DSPP_JIT_STATS
			if (options.jit)
				dspp_jit_count_fallback(reason, pc, opcode, m_elapsed_ticks - operand_start);
#endif
		}
	}
}

static void dspp_sample(int param)
{
	dspp_run();
	/* Translate two's complement samples to the DAC's offset-binary input. */
	DAC_signed_data_16_w(0, dspp_read_output_fifo() ^ 0x8000);
	DAC_signed_data_16_w(1, dspp_read_output_fifo() ^ 0x8000);
}

void m2_dspp_init(void)
{
	m_output_timer = timer_alloc(dspp_sample);
#if SLJIT_MAME_SUPPORTED
	if (options.jit)
	{
#if DSPP_JIT_STATS
		memset(&m_jit_stats, 0, sizeof(m_jit_stats));
		add_exit_callback(dspp_jit_print_stats);
#endif
		add_exit_callback(dspp_jit_clear);
	}
#endif
}

void m2_dspp_reset(void)
{
	memset(&m_core, 0, sizeof(m_core));
	memset(&m_dsp, 0, sizeof(m_dsp));
	m_dsp.m_fifo_input_ready = ~0U;
	memset(m_program, 0, sizeof(m_program));
	memset(m_data_ram, 0, sizeof(m_data_ram));
#if SLJIT_MAME_SUPPORTED
	m_jit_dirty = 1;
#endif
	m_elapsed_ticks = timer_get_time() * M2_CLOCK;
	dspp_reset_core();
	timer_adjust(m_output_timer, TIME_IN_HZ(44100), 0, TIME_IN_HZ(44100));
}

UINT32 m2_dspp_read(offs_t offset)
{
	dspp_run();
	return dspp_host_read(offset);
}

void m2_dspp_write(offs_t offset, UINT32 data)
{
	dspp_run();
	dspp_host_write(offset, data);
}

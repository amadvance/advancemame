// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    m6809.c

    Portable 6809 emulator

*********************************************************************/

#include "debugger.h"
#include "m6809.h"

static m6809_Regs m6809;
static int m6809_ICount;

static UINT8 m6809_reg_layout[] = {
	M6809_PC, M6809_S, M6809_CC, M6809_A, M6809_B, M6809_X, -1,
	M6809_Y, M6809_U, M6809_DP, 0
};

static UINT8 m6809_win_layout[] = {
	27, 0, 53, 4,
	0, 0, 26, 22,
	0, 23, 80, 1,
	54, 0, 26, 22
};

#include "6809tbl.c"
#include "6809ops.c"

static void set_irq_line(int irqline, int state)
{
	switch (irqline)
	{
		case INPUT_LINE_NMI:
			m_nmi_asserted = m_nmi_asserted || ((state != CLEAR_LINE) && !m_nmi_line && m_lds_encountered);
			m_nmi_line = (state != CLEAR_LINE);
			break;

		case M6809_FIRQ_LINE:
			m_firq_line = (state != CLEAR_LINE);
			break;

		case M6809_IRQ_LINE:
			m_irq_line = (state != CLEAR_LINE);
			break;
	}
}

static void m6809_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	m6809.irq_callback = irqcallback;

	state_save_register_item("m6809", index, m6809.pc.w);
	state_save_register_item("m6809", index, m6809.ppc.w);
	state_save_register_item("m6809", index, m6809.d.w);
	state_save_register_item("m6809", index, m6809.dp);
	state_save_register_item("m6809", index, m6809.u.w);
	state_save_register_item("m6809", index, m6809.s.w);
	state_save_register_item("m6809", index, m6809.x.w);
	state_save_register_item("m6809", index, m6809.y.w);
	state_save_register_item("m6809", index, m6809.cc);
	state_save_register_item("m6809", index, m6809.state);
	state_save_register_item("m6809", index, m6809.nmi_line);
	state_save_register_item("m6809", index, m6809.nmi_asserted);
	state_save_register_item("m6809", index, m6809.firq_line);
	state_save_register_item("m6809", index, m6809.irq_line);
	state_save_register_item("m6809", index, m6809.lds_encountered);
}

static void m6809_reset(void)
{
	m_nmi_line = 0;
	m_nmi_asserted = 0;
	m_firq_line = 0;
	m_irq_line = 0;
	m_lds_encountered = 0;

	m_dp = 0x00;

	m_cc |= CC_I;
	m_cc |= CC_F;

	m_pc.b.h = program_read_byte_8(VECTOR_RESET_FFFE + 0);
	m_pc.b.l = program_read_byte_8(VECTOR_RESET_FFFE + 1);

	reset_state();
	change_pc(m_pc.w);
}

static void m6809_exit(void)
{
}

static int m6809_execute(int cycles)
{
	m6809_ICount = cycles;
	do
	{
		m6809_execute_one();
	} while (m6809_ICount > 0);
	return cycles - m6809_ICount;
}

static void m6809_get_context(void *dst)
{
	if (dst)
		*(m6809_Regs *)dst = m6809;
}

static void m6809_set_context(void *src)
{
	if (src)
		m6809 = *(m6809_Regs *)src;
	change_pc(m_pc.w);
}

static void m6809_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_INPUT_STATE + M6809_IRQ_LINE:	set_irq_line(M6809_IRQ_LINE, info->i);	break;
		case CPUINFO_INT_INPUT_STATE + M6809_FIRQ_LINE:	set_irq_line(M6809_FIRQ_LINE, info->i); break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	set_irq_line(INPUT_LINE_NMI, info->i);	break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + M6809_PC:			m_pc.w = info->i; change_pc(m_pc.w);	break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + M6809_S:			m_s.w = info->i;						break;
		case CPUINFO_INT_REGISTER + M6809_CC:			m_cc = info->i;							break;
		case CPUINFO_INT_REGISTER + M6809_U:			m_u.w = info->i;						break;
		case CPUINFO_INT_REGISTER + M6809_A:			m_d.b.h = info->i;						break;
		case CPUINFO_INT_REGISTER + M6809_B:			m_d.b.l = info->i;						break;
		case CPUINFO_INT_REGISTER + M6809_X:			m_x.w = info->i;						break;
		case CPUINFO_INT_REGISTER + M6809_Y:			m_y.w = info->i;						break;
		case CPUINFO_INT_REGISTER + M6809_DP:			m_dp = info->i;							break;
	}
}

void m6809_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(m6809);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 5;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 19;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + M6809_IRQ_LINE:	info->i = m_irq_line ? ASSERT_LINE : CLEAR_LINE; break;
		case CPUINFO_INT_INPUT_STATE + M6809_FIRQ_LINE:	info->i = m_firq_line ? ASSERT_LINE : CLEAR_LINE; break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = m_nmi_line ? ASSERT_LINE : CLEAR_LINE; break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = m_ppc.w;						break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + M6809_PC:			info->i = m_pc.w;						break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + M6809_S:			info->i = m_s.w;						break;
		case CPUINFO_INT_REGISTER + M6809_CC:			info->i = m_cc;							break;
		case CPUINFO_INT_REGISTER + M6809_U:			info->i = m_u.w;						break;
		case CPUINFO_INT_REGISTER + M6809_A:			info->i = m_d.b.h;						break;
		case CPUINFO_INT_REGISTER + M6809_B:			info->i = m_d.b.l;						break;
		case CPUINFO_INT_REGISTER + M6809_X:			info->i = m_x.w;						break;
		case CPUINFO_INT_REGISTER + M6809_Y:			info->i = m_y.w;						break;
		case CPUINFO_INT_REGISTER + M6809_DP:			info->i = m_dp;							break;

		case CPUINFO_PTR_SET_INFO:						info->setinfo = m6809_set_info;				break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m6809_get_context;			break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m6809_set_context;			break;
		case CPUINFO_PTR_INIT:							info->init = m6809_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = m6809_reset;					break;
		case CPUINFO_PTR_EXIT:							info->exit = m6809_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = m6809_execute;				break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = m6809_dasm;					break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m6809_ICount;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = m6809_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m6809_win_layout;				break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6809"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola 6809"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "2.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright Nathan Woods"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
				m_cc & 0x80 ? 'E':'.',
				m_cc & 0x40 ? 'F':'.',
				m_cc & 0x20 ? 'H':'.',
				m_cc & 0x10 ? 'I':'.',
				m_cc & 0x08 ? 'N':'.',
				m_cc & 0x04 ? 'Z':'.',
				m_cc & 0x02 ? 'V':'.',
				m_cc & 0x01 ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + M6809_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m_pc.w); break;
		case CPUINFO_STR_REGISTER + M6809_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", m_s.w); break;
		case CPUINFO_STR_REGISTER + M6809_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", m_cc); break;
		case CPUINFO_STR_REGISTER + M6809_U:			sprintf(info->s = cpuintrf_temp_str(), "U:%04X", m_u.w); break;
		case CPUINFO_STR_REGISTER + M6809_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", m_d.b.h); break;
		case CPUINFO_STR_REGISTER + M6809_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", m_d.b.l); break;
		case CPUINFO_STR_REGISTER + M6809_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", m_x.w); break;
		case CPUINFO_STR_REGISTER + M6809_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%04X", m_y.w); break;
		case CPUINFO_STR_REGISTER + M6809_DP:			sprintf(info->s = cpuintrf_temp_str(), "DP:%02X", m_dp); break;
	}
}

void m6809e_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 4;							break;
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6809E"); break;
		default:										m6809_get_info(state, info);			break;
	}
}

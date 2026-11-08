// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    hd6309.c

    Portable 6309 emulator

*********************************************************************/

#include "debugger.h"
#include "hd6309.h"

static hd6309_Regs hd6309;
static int hd6309_ICount;

static UINT8 hd6309_reg_layout[] = {
	HD6309_PC, HD6309_S, HD6309_CC, HD6309_A, HD6309_B, HD6309_X, -1,
	HD6309_Y, HD6309_U, HD6309_DP, HD6309_E, HD6309_F, HD6309_V, HD6309_MD, 0
};

static UINT8 hd6309_win_layout[] = {
	27, 0, 53, 4,
	0, 0, 26, 22,
	0, 23, 80, 1,
	54, 0, 26, 22
};

#include "6309tbl.c"
#include "6309ops.c"

static void set_irq_line(int irqline, int state)
{
	switch (irqline)
	{
		case INPUT_LINE_NMI:
			m_nmi_asserted = m_nmi_asserted || ((state != CLEAR_LINE) && !m_nmi_line && m_lds_encountered);
			m_nmi_line = (state != CLEAR_LINE);
			break;

		case HD6309_FIRQ_LINE:
			m_firq_line = (state != CLEAR_LINE);
			break;

		case HD6309_IRQ_LINE:
			m_irq_line = (state != CLEAR_LINE);
			break;
	}
}

static void hd6309_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	hd6309.irq_callback = irqcallback;

	state_save_register_item("hd6309", index, hd6309.pc.w);
	state_save_register_item("hd6309", index, hd6309.ppc.w);
	state_save_register_item("hd6309", index, hd6309.d.w);
	state_save_register_item("hd6309", index, hd6309.dp);
	state_save_register_item("hd6309", index, hd6309.u.w);
	state_save_register_item("hd6309", index, hd6309.s.w);
	state_save_register_item("hd6309", index, hd6309.x.w);
	state_save_register_item("hd6309", index, hd6309.y.w);
	state_save_register_item("hd6309", index, hd6309.cc);
	state_save_register_item("hd6309", index, hd6309.w.w);
	state_save_register_item("hd6309", index, hd6309.v.w);
	state_save_register_item("hd6309", index, hd6309.md);
	state_save_register_item("hd6309", index, hd6309.state);
	state_save_register_item("hd6309", index, hd6309.nmi_line);
	state_save_register_item("hd6309", index, hd6309.nmi_asserted);
	state_save_register_item("hd6309", index, hd6309.firq_line);
	state_save_register_item("hd6309", index, hd6309.irq_line);
	state_save_register_item("hd6309", index, hd6309.lds_encountered);
}

static void hd6309_reset(void)
{
	m_nmi_line = 0;
	m_nmi_asserted = 0;
	m_firq_line = 0;
	m_irq_line = 0;
	m_lds_encountered = 0;

	m_dp = 0x00;
	m_md = 0x00;

	m_cc |= CC_I;
	m_cc |= CC_F;

	m_pc.b.h = program_read_byte_8(VECTOR_RESET_FFFE + 0);
	m_pc.b.l = program_read_byte_8(VECTOR_RESET_FFFE + 1);

	reset_state();
	change_pc(m_pc.w);
}

static void hd6309_exit(void)
{
}

static int hd6309_execute(int cycles)
{
	hd6309_ICount = cycles;
	do
	{
		hd6309_execute_one();
	} while (hd6309_ICount > 0);
	return cycles - hd6309_ICount;
}

static void hd6309_get_context(void *dst)
{
	if (dst)
		*(hd6309_Regs *)dst = hd6309;
}

static void hd6309_set_context(void *src)
{
	if (src)
		hd6309 = *(hd6309_Regs *)src;
	change_pc(m_pc.w);
}

static void hd6309_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_INPUT_STATE + HD6309_IRQ_LINE:	set_irq_line(HD6309_IRQ_LINE, info->i);	break;
		case CPUINFO_INT_INPUT_STATE + HD6309_FIRQ_LINE:set_irq_line(HD6309_FIRQ_LINE, info->i); break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	set_irq_line(INPUT_LINE_NMI, info->i);	break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + HD6309_PC:			m_pc.w = info->i; change_pc(m_pc.w);	break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + HD6309_S:			m_s.w = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_CC:			m_cc = info->i;							break;
		case CPUINFO_INT_REGISTER + HD6309_U:			m_u.w = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_A:			m_d.b.h = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_B:			m_d.b.l = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_X:			m_x.w = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_Y:			m_y.w = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_DP:			m_dp = info->i;							break;
		case CPUINFO_INT_REGISTER + HD6309_E:			m_w.b.h = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_F:			m_w.b.l = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_V:			m_v.w = info->i;						break;
		case CPUINFO_INT_REGISTER + HD6309_MD:			m_md = info->i;							break;
		case CPUINFO_INT_REGISTER + HD6309_W:			m_w.w = info->i;						break;
	}
}

void hd6309_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(hd6309);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 4;							break;
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

		case CPUINFO_INT_INPUT_STATE + HD6309_IRQ_LINE:	info->i = m_irq_line ? ASSERT_LINE : CLEAR_LINE; break;
		case CPUINFO_INT_INPUT_STATE + HD6309_FIRQ_LINE:info->i = m_firq_line ? ASSERT_LINE : CLEAR_LINE; break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = m_nmi_line ? ASSERT_LINE : CLEAR_LINE; break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = m_ppc.w;						break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + HD6309_PC:			info->i = m_pc.w;						break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + HD6309_S:			info->i = m_s.w;						break;
		case CPUINFO_INT_REGISTER + HD6309_CC:			info->i = m_cc;							break;
		case CPUINFO_INT_REGISTER + HD6309_U:			info->i = m_u.w;						break;
		case CPUINFO_INT_REGISTER + HD6309_A:			info->i = m_d.b.h;						break;
		case CPUINFO_INT_REGISTER + HD6309_B:			info->i = m_d.b.l;						break;
		case CPUINFO_INT_REGISTER + HD6309_X:			info->i = m_x.w;						break;
		case CPUINFO_INT_REGISTER + HD6309_Y:			info->i = m_y.w;						break;
		case CPUINFO_INT_REGISTER + HD6309_DP:			info->i = m_dp;							break;
		case CPUINFO_INT_REGISTER + HD6309_E:			info->i = m_w.b.h;						break;
		case CPUINFO_INT_REGISTER + HD6309_F:			info->i = m_w.b.l;						break;
		case CPUINFO_INT_REGISTER + HD6309_V:			info->i = m_v.w;						break;
		case CPUINFO_INT_REGISTER + HD6309_MD:			info->i = m_md;							break;
		case CPUINFO_INT_REGISTER + HD6309_W:			info->i = m_w.w;						break;

		case CPUINFO_PTR_SET_INFO:						info->setinfo = hd6309_set_info;				break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = hd6309_get_context;			break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = hd6309_set_context;			break;
		case CPUINFO_PTR_INIT:							info->init = hd6309_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = hd6309_reset;					break;
		case CPUINFO_PTR_EXIT:							info->exit = hd6309_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = hd6309_execute;				break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = hd6309_dasm;				break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &hd6309_ICount;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = hd6309_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = hd6309_win_layout;				break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "HD6309"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Hitachi 6309"); break;
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

		case CPUINFO_STR_REGISTER + HD6309_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m_pc.w); break;
		case CPUINFO_STR_REGISTER + HD6309_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", m_s.w); break;
		case CPUINFO_STR_REGISTER + HD6309_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", m_cc); break;
		case CPUINFO_STR_REGISTER + HD6309_U:			sprintf(info->s = cpuintrf_temp_str(), "U:%04X", m_u.w); break;
		case CPUINFO_STR_REGISTER + HD6309_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", m_d.b.h); break;
		case CPUINFO_STR_REGISTER + HD6309_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", m_d.b.l); break;
		case CPUINFO_STR_REGISTER + HD6309_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", m_x.w); break;
		case CPUINFO_STR_REGISTER + HD6309_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%04X", m_y.w); break;
		case CPUINFO_STR_REGISTER + HD6309_DP:			sprintf(info->s = cpuintrf_temp_str(), "DP:%02X", m_dp); break;
		case CPUINFO_STR_REGISTER + HD6309_E:			sprintf(info->s = cpuintrf_temp_str(), "E:%02X", m_w.b.h); break;
		case CPUINFO_STR_REGISTER + HD6309_F:			sprintf(info->s = cpuintrf_temp_str(), "F:%02X", m_w.b.l); break;
		case CPUINFO_STR_REGISTER + HD6309_V:			sprintf(info->s = cpuintrf_temp_str(), "V:%04X", m_v.w); break;
		case CPUINFO_STR_REGISTER + HD6309_MD:			sprintf(info->s = cpuintrf_temp_str(), "MD:%02X", m_md); break;
		case CPUINFO_STR_REGISTER + HD6309_W:			sprintf(info->s = cpuintrf_temp_str(), "W:%04X", m_w.w); break;
	}
}

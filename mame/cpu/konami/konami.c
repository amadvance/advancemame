// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    konami.c

    Portable Konami CPU emulator

*********************************************************************/

#include "debugger.h"
#include "konami.h"

static konami_Regs konami;
static int konami_ICount;

static UINT8 konami_reg_layout[] = {
	KONAMI_PC, KONAMI_S, KONAMI_CC, KONAMI_A, KONAMI_B, KONAMI_X, -1,
	KONAMI_Y, KONAMI_U, KONAMI_DP, 0
};

static UINT8 konami_win_layout[] = {
	27, 0, 53, 4,
	0, 0, 26, 22,
	0, 23, 80, 1,
	54, 0, 26, 22
};

#include "konamtbl.c"
#include "konamops.c"

static void set_irq_line(int irqline, int state)
{
	switch (irqline)
	{
		case INPUT_LINE_NMI:
			m_nmi_asserted = m_nmi_asserted || ((state != CLEAR_LINE) && !m_nmi_line && m_lds_encountered);
			m_nmi_line = (state != CLEAR_LINE);
			break;

		case KONAMI_FIRQ_LINE:
			m_firq_line = (state != CLEAR_LINE);
			break;

		case KONAMI_IRQ_LINE:
			m_irq_line = (state != CLEAR_LINE);
			break;
	}
}

static void konami_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	konami.irq_callback = irqcallback;

	state_save_register_item("konami", index, konami.pc.w);
	state_save_register_item("konami", index, konami.ppc.w);
	state_save_register_item("konami", index, konami.d.w);
	state_save_register_item("konami", index, konami.dp);
	state_save_register_item("konami", index, konami.u.w);
	state_save_register_item("konami", index, konami.s.w);
	state_save_register_item("konami", index, konami.x.w);
	state_save_register_item("konami", index, konami.y.w);
	state_save_register_item("konami", index, konami.cc);
	state_save_register_item("konami", index, konami.state);
	state_save_register_item("konami", index, konami.nmi_line);
	state_save_register_item("konami", index, konami.nmi_asserted);
	state_save_register_item("konami", index, konami.firq_line);
	state_save_register_item("konami", index, konami.irq_line);
	state_save_register_item("konami", index, konami.lds_encountered);
}

static void konami_reset(void)
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

static void konami_exit(void)
{
}

static int konami_execute(int cycles)
{
	konami_ICount = cycles;
	do
	{
		konami_execute_one();
	} while (konami_ICount > 0);
	return cycles - konami_ICount;
}

static void konami_get_context(void *dst)
{
	if (dst)
		*(konami_Regs *)dst = konami;
}

static void konami_set_context(void *src)
{
	if (src)
		konami = *(konami_Regs *)src;
	change_pc(m_pc.w);
}

static offs_t konami_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes)
{
#ifdef MAME_DEBUG
	return Dasmknmi(buffer, pc, oprom, opram, bytes);
#else
	sprintf(buffer, "$%02X", *oprom);
	return 1;
#endif
}

static void konami_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_INPUT_STATE + KONAMI_IRQ_LINE:	set_irq_line(KONAMI_IRQ_LINE, info->i);	break;
		case CPUINFO_INT_INPUT_STATE + KONAMI_FIRQ_LINE:set_irq_line(KONAMI_FIRQ_LINE, info->i); break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	set_irq_line(INPUT_LINE_NMI, info->i);	break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + KONAMI_PC:			m_pc.w = info->i; change_pc(m_pc.w);	break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + KONAMI_S:			m_s.w = info->i;						break;
		case CPUINFO_INT_REGISTER + KONAMI_CC:			m_cc = info->i;							break;
		case CPUINFO_INT_REGISTER + KONAMI_U:			m_u.w = info->i;						break;
		case CPUINFO_INT_REGISTER + KONAMI_A:			m_d.b.h = info->i;						break;
		case CPUINFO_INT_REGISTER + KONAMI_B:			m_d.b.l = info->i;						break;
		case CPUINFO_INT_REGISTER + KONAMI_X:			m_x.w = info->i;						break;
		case CPUINFO_INT_REGISTER + KONAMI_Y:			m_y.w = info->i;						break;
		case CPUINFO_INT_REGISTER + KONAMI_DP:			m_dp = info->i;							break;

		case CPUINFO_PTR_KONAMI_SETLINES_CALLBACK:		konami.setlines_callback = (void (*)(int))info->f; break;
	}
}

void konami_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(konami);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 13;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + KONAMI_IRQ_LINE:	info->i = m_irq_line ? ASSERT_LINE : CLEAR_LINE; break;
		case CPUINFO_INT_INPUT_STATE + KONAMI_FIRQ_LINE:info->i = m_firq_line ? ASSERT_LINE : CLEAR_LINE; break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = m_nmi_line ? ASSERT_LINE : CLEAR_LINE; break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = m_ppc.w;						break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + KONAMI_PC:			info->i = m_pc.w;						break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + KONAMI_S:			info->i = m_s.w;						break;
		case CPUINFO_INT_REGISTER + KONAMI_CC:			info->i = m_cc;							break;
		case CPUINFO_INT_REGISTER + KONAMI_U:			info->i = m_u.w;						break;
		case CPUINFO_INT_REGISTER + KONAMI_A:			info->i = m_d.b.h;						break;
		case CPUINFO_INT_REGISTER + KONAMI_B:			info->i = m_d.b.l;						break;
		case CPUINFO_INT_REGISTER + KONAMI_X:			info->i = m_x.w;						break;
		case CPUINFO_INT_REGISTER + KONAMI_Y:			info->i = m_y.w;						break;
		case CPUINFO_INT_REGISTER + KONAMI_DP:			info->i = m_dp;							break;

		case CPUINFO_PTR_SET_INFO:						info->setinfo = konami_set_info;				break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = konami_get_context;			break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = konami_set_context;			break;
		case CPUINFO_PTR_INIT:							info->init = konami_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = konami_reset;					break;
		case CPUINFO_PTR_EXIT:							info->exit = konami_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = konami_execute;				break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = konami_dasm;				break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &konami_ICount;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = konami_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = konami_win_layout;				break;
		case CPUINFO_PTR_KONAMI_SETLINES_CALLBACK:		info->f = (genf *)konami.setlines_callback;	break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "KONAMI"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "KONAMI 5000x"); break;
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

		case CPUINFO_STR_REGISTER + KONAMI_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m_pc.w); break;
		case CPUINFO_STR_REGISTER + KONAMI_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", m_s.w); break;
		case CPUINFO_STR_REGISTER + KONAMI_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", m_cc); break;
		case CPUINFO_STR_REGISTER + KONAMI_U:			sprintf(info->s = cpuintrf_temp_str(), "U:%04X", m_u.w); break;
		case CPUINFO_STR_REGISTER + KONAMI_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", m_d.b.h); break;
		case CPUINFO_STR_REGISTER + KONAMI_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", m_d.b.l); break;
		case CPUINFO_STR_REGISTER + KONAMI_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", m_x.w); break;
		case CPUINFO_STR_REGISTER + KONAMI_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%04X", m_y.w); break;
		case CPUINFO_STR_REGISTER + KONAMI_DP:			sprintf(info->s = cpuintrf_temp_str(), "DP:%02X", m_dp); break;
	}
}

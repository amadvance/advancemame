// license:BSD-3-Clause
// copyright-holders:Andrew Gardner, Ryan Holtz
#include "debugger.h"
#include "dsp56k.h"
#include "dsp56core.h"

static dsp56156_core dsp56k;
UINT16 *dsp56k_program_ram;

#define LOG(...) do { } while (0)
#define LOGMASKED(...) do { } while (0)
/* DSP addresses count words; the core memory accessors count bytes. */
#define ROPCODE(pc) program_read_word_16le((UINT16)(pc) << 1)
#define dsp_data_read(a) data_read_word_16le((UINT16)(a) << 1)
#define dsp_data_write(a,d) data_write_word_16le((UINT16)(a) << 1,d)
#define dsp_program_read(a) program_read_word_16le((UINT16)(a) << 1)
#define dsp_program_write(a,d) program_write_word_16le((UINT16)(a) << 1,d)

#include "dsp56def.h"
#include "dsp56pcu.h"
#include "dsp56mem.h"
#include "dsp56pcu.c"
#include "dsp56mem.c"

static INT64 dsp_sign_extend(UINT64 value, int bits)
{
	return (INT64)(value << (64 - bits)) >> (64 - bits);
}

#include "dsp56ops.c"

static void dsp56k_reset(void)
{
	dsp56156_core *cpustate = &dsp56k;
	cpustate->program_ram = dsp56k_program_ram;
	cpustate->interrupt_cycles = 0;
	cpustate->repFlag = 0;
	cpustate->repAddr = 0;
	pcu_reset(cpustate);
	mem_reset(cpustate);
	memset(&cpustate->AGU, 0, sizeof(cpustate->AGU));
	M0 = M1 = M2 = M3 = 0xffff;
	memset(&cpustate->ALU, 0, sizeof(cpustate->ALU));
	cpustate->ppc = PC;
	program_write_word_16le(0, 0x0124);
}

static void set_irq_line(int irqline, int state)
{
	dsp56156_core *cpustate = &dsp56k;
	switch (irqline)
	{
	case DSP56K_IRQ_MODA:
		cpustate->modA_state = state != CLEAR_LINE;
		if (!cpustate->reset_state) dsp56156_add_pending_interrupt(cpustate, "IRQA");
		break;
	case DSP56K_IRQ_MODB:
		cpustate->modB_state = state != CLEAR_LINE;
		if (!cpustate->reset_state) dsp56156_add_pending_interrupt(cpustate, "IRQB");
		break;
	case DSP56K_IRQ_MODC:
		cpustate->modC_state = state != CLEAR_LINE;
		break;
	case DSP56K_IRQ_RESET:
		if (state != CLEAR_LINE) cpustate->reset_state = 1;
		else
		{
			if (cpustate->reset_state) dsp56k_reset();
			cpustate->reset_state = 0;
		}
		break;
	}
}

static void dsp56k_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	memset(&dsp56k, 0, sizeof(dsp56k));
	pcu_init(&dsp56k);
}

static void dsp56k_exit(void) { }
static void dsp56k_get_context(void *dst) { if (dst) memcpy(dst, &dsp56k, sizeof(dsp56k)); }
static void dsp56k_set_context(void *src) { if (src) memcpy(&dsp56k, src, sizeof(dsp56k)); }

static int dsp56k_execute(int cycles)
{
	if (dsp56k.reset_state || dsp56k.bootstrap_mode != BOOTSTRAP_OFF) return cycles;
	dsp56k.icount = cycles;
	while (dsp56k.icount > 0)
	{
		execute_one(&dsp56k);
		pcu_service_interrupts(&dsp56k);
	}
	return cycles - dsp56k.icount;
}

static offs_t dsp56k_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	extern unsigned dasm_dsp56k(char *, unsigned);
	return dasm_dsp56k(buffer, pc);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc));
	return 2;
#endif
}

UINT8 dsp56k_host_read(int cpu, UINT8 offset)
{
	UINT8 result;
	cpuintrf_push_context(cpu);
	result = host_interface_read(offset);
	cpuintrf_pop_context();
	return result;
}

void dsp56k_host_write(int cpu, UINT8 offset, UINT8 data)
{
	cpuintrf_push_context(cpu);
	host_interface_write(offset, data);
	cpuintrf_pop_context();
}

void dsp56k_set_portc(int cpu, void (*callback)(UINT16))
{
	cpuintrf_push_context(cpu);
	dsp56k.portc = callback;
	cpuintrf_pop_context();
}

static void dsp56k_set_info(UINT32 state, union cpuinfo *info)
{
	dsp56156_core* cpustate = &dsp56k;

	switch (state)
	{
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODA:   set_irq_line( DSP56K_IRQ_MODA, info->i);  break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODB:   set_irq_line( DSP56K_IRQ_MODB, info->i);  break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODC:   set_irq_line( DSP56K_IRQ_MODC, info->i);  break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_RESET:  set_irq_line( DSP56K_IRQ_RESET, info->i); break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + DSP56K_PC:			PC  = info->i & 0xffff;					break;

		case CPUINFO_INT_REGISTER + DSP56K_SR:			SR  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_LC:			LC  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_LA:			LA  = info->i & 0xffff;					break;

		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + DSP56K_SP:			SP  = info->i & 0xff;					break;

		case CPUINFO_INT_REGISTER + DSP56K_OMR:			OMR = info->i & 0xff;					break;

		case CPUINFO_INT_REGISTER + DSP56K_X:			X   = info->i & 0xffffffff;				break;
		case CPUINFO_INT_REGISTER + DSP56K_Y:			Y   = info->i & 0xffffffff;				break;

		case CPUINFO_INT_REGISTER + DSP56K_A:			A   = info->i & (UINT64)U64(0xffffffffffffffff); break;
		case CPUINFO_INT_REGISTER + DSP56K_B:			B   = info->i & (UINT64)U64(0xffffffffffffffff); break;

		case CPUINFO_INT_REGISTER + DSP56K_R0:			R0  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_R1:			R1  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_R2:			R2  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_R3:			R3  = info->i & 0xffff;					break;

		case CPUINFO_INT_REGISTER + DSP56K_N0:			N0  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_N1:			N1  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_N2:			N2  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_N3:			N3  = info->i & 0xffff;					break;

		case CPUINFO_INT_REGISTER + DSP56K_M0:			M0  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_M1:			M1  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_M2:			M2  = info->i & 0xffff;					break;
		case CPUINFO_INT_REGISTER + DSP56K_M3:			M3  = info->i & 0xffff;					break;

		case CPUINFO_INT_REGISTER + DSP56K_ST0:			ST0 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST1:			ST1 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST2:			ST2 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST3:			ST3 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST4:			ST4 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST5:			ST5 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST6:			ST6 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST7:			ST7 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST8:			ST8 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST9:			ST9 = info->i  & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST10:		ST10 = info->i & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST11:		ST11 = info->i & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST12:		ST12 = info->i & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST13:		ST13 = info->i & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST14:		ST14 = info->i & 0xffffffff;			break;
		case CPUINFO_INT_REGISTER + DSP56K_ST15:		ST15 = info->i & 0xffffffff;			break;
	}
}

static const UINT8 dsp56k_reg_layout[] =
{
	DSP56K_PC, DSP56K_SR, DSP56K_LC, DSP56K_LA, DSP56K_SP, DSP56K_OMR,
	DSP56K_X, DSP56K_Y, DSP56K_A, DSP56K_B,
	DSP56K_R0, DSP56K_R1, DSP56K_R2, DSP56K_R3,
	DSP56K_N0, DSP56K_N1, DSP56K_N2, DSP56K_N3,
	DSP56K_M0, DSP56K_M1, DSP56K_M2, DSP56K_M3, 0
};

void dsp56k_get_info(UINT32 state, union cpuinfo *info)
{
	dsp56156_core* cpustate = &dsp56k;

	switch (state)
	{

		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(dsp56156_core);			break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 4;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0x0000;						break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;			break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 8;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = -1;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = -1;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODA:			info->i = DSP56K_IRQ_MODA;		break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODB:			info->i = DSP56K_IRQ_MODB;		break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODC:			info->i = DSP56K_IRQ_MODC;		break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + DSP56K_PC:			info->i = PC;							break;
		case CPUINFO_INT_PREVIOUSPC:					info->i = cpustate->ppc;				break;

		case CPUINFO_INT_REGISTER + DSP56K_SR:			info->i = SR;							break;
		case CPUINFO_INT_REGISTER + DSP56K_LC:			info->i = LC;							break;
		case CPUINFO_INT_REGISTER + DSP56K_LA:			info->i = LA;							break;

		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + DSP56K_SP:			info->i = SP;							break;

		case CPUINFO_INT_REGISTER + DSP56K_OMR:			info->i = OMR;							break;

		case CPUINFO_INT_REGISTER + DSP56K_X:			info->i = X;							break;
		case CPUINFO_INT_REGISTER + DSP56K_Y:			info->i = Y;							break;

		case CPUINFO_INT_REGISTER + DSP56K_A:			info->i = A;							break;
		case CPUINFO_INT_REGISTER + DSP56K_B:			info->i = B;							break;

		case CPUINFO_INT_REGISTER + DSP56K_R0:			info->i = R0;							break;
		case CPUINFO_INT_REGISTER + DSP56K_R1:			info->i = R1;							break;
		case CPUINFO_INT_REGISTER + DSP56K_R2:			info->i = R2;							break;
		case CPUINFO_INT_REGISTER + DSP56K_R3:			info->i = R3;							break;

		case CPUINFO_INT_REGISTER + DSP56K_N0:			info->i = N0;							break;
		case CPUINFO_INT_REGISTER + DSP56K_N1:			info->i = N1;							break;
		case CPUINFO_INT_REGISTER + DSP56K_N2:			info->i = N2;							break;
		case CPUINFO_INT_REGISTER + DSP56K_N3:			info->i = N3;							break;

		case CPUINFO_INT_REGISTER + DSP56K_M0:			info->i = M0;							break;
		case CPUINFO_INT_REGISTER + DSP56K_M1:			info->i = M1;							break;
		case CPUINFO_INT_REGISTER + DSP56K_M2:			info->i = M2;							break;
		case CPUINFO_INT_REGISTER + DSP56K_M3:			info->i = M3;							break;

		case CPUINFO_INT_REGISTER + DSP56K_ST0:			info->i = ST0;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST1:			info->i = ST1;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST2:			info->i = ST2;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST3:			info->i = ST3;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST4:			info->i = ST4;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST5:			info->i = ST5;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST6:			info->i = ST6;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST7:			info->i = ST7;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST8:			info->i = ST8;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST9:			info->i = ST9;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST10:		info->i = ST10;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST11:		info->i = ST11;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST12:		info->i = ST12;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST13:		info->i = ST13;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST14:		info->i = ST14;							break;
		case CPUINFO_INT_REGISTER + DSP56K_ST15:		info->i = ST15;							break;

		case CPUINFO_PTR_GET_CONTEXT: info->getcontext = dsp56k_get_context; break;
		case CPUINFO_PTR_SET_CONTEXT: info->setcontext = dsp56k_set_context; break;
		case CPUINFO_PTR_SET_INFO:						info->setinfo = dsp56k_set_info;			break;
		case CPUINFO_PTR_INIT:							info->init = dsp56k_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = dsp56k_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = dsp56k_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = dsp56k_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;									break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = dsp56k_dasm;	break;
		case CPUINFO_PTR_REGISTER_LAYOUT: info->p = (void *)dsp56k_reg_layout; break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &cpustate->icount;		break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "DSP56156");			break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola DSP56156");	break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "0.1");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Andrew Gardner");		break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%s%s %s%s%s%s%s%s%s%s %s%s",

				LF_bit(cpustate) ? "L" : ".",
				FV_bit(cpustate) ? "F" : ".",

				S_bit(cpustate) ? "S" : ".",
				L_bit(cpustate) ? "L" : ".",
				E_bit(cpustate) ? "E" : ".",
				U_bit(cpustate) ? "U" : ".",
				N_bit(cpustate) ? "N" : ".",
				Z_bit(cpustate) ? "Z" : ".",
				V_bit(cpustate) ? "V" : ".",
				C_bit(cpustate) ? "C" : ".",

				UF_bit(cpustate) ? "U" : ".",
				SE_bit(cpustate) ? "S" : ".");
            break;

		case CPUINFO_STR_REGISTER + DSP56K_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC : %04x", PC);		break;
		case CPUINFO_STR_REGISTER + DSP56K_SR:			sprintf(info->s = cpuintrf_temp_str(), "SR : %04x", SR);		break;
		case CPUINFO_STR_REGISTER + DSP56K_LC:			sprintf(info->s = cpuintrf_temp_str(), "LC : %04x", LC);		break;
		case CPUINFO_STR_REGISTER + DSP56K_LA:			sprintf(info->s = cpuintrf_temp_str(), "LA : %04x", LA);		break;
		case CPUINFO_STR_REGISTER + DSP56K_SP:			sprintf(info->s = cpuintrf_temp_str(), "SP : %02x", SP);		break;
		case CPUINFO_STR_REGISTER + DSP56K_OMR:			sprintf(info->s = cpuintrf_temp_str(), "OMR: %02x", OMR);		break;

		case CPUINFO_STR_REGISTER + DSP56K_X:			sprintf(info->s = cpuintrf_temp_str(), "X  : %04x %04x", X1, X0); break;
		case CPUINFO_STR_REGISTER + DSP56K_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y  : %04x %04x", Y1, Y0); break;

		case CPUINFO_STR_REGISTER + DSP56K_A:			sprintf(info->s = cpuintrf_temp_str(), "A  : %02x %04x %04x", A2,A1,A0); break;
		case CPUINFO_STR_REGISTER + DSP56K_B:			sprintf(info->s = cpuintrf_temp_str(), "B  : %02x %04x %04x", B2,B1,B0); break;

		case CPUINFO_STR_REGISTER + DSP56K_R0:			sprintf(info->s = cpuintrf_temp_str(), "R0 : %04x", R0);		break;
		case CPUINFO_STR_REGISTER + DSP56K_R1:			sprintf(info->s = cpuintrf_temp_str(), "R1 : %04x", R1);		break;
		case CPUINFO_STR_REGISTER + DSP56K_R2:			sprintf(info->s = cpuintrf_temp_str(), "R2 : %04x", R2);		break;
		case CPUINFO_STR_REGISTER + DSP56K_R3:			sprintf(info->s = cpuintrf_temp_str(), "R3 : %04x", R3);		break;

		case CPUINFO_STR_REGISTER + DSP56K_N0:			sprintf(info->s = cpuintrf_temp_str(), "N0 : %04x", N0);		break;
		case CPUINFO_STR_REGISTER + DSP56K_N1:			sprintf(info->s = cpuintrf_temp_str(), "N1 : %04x", N1);		break;
		case CPUINFO_STR_REGISTER + DSP56K_N2:			sprintf(info->s = cpuintrf_temp_str(), "N2 : %04x", N2);		break;
		case CPUINFO_STR_REGISTER + DSP56K_N3:			sprintf(info->s = cpuintrf_temp_str(), "N3 : %04x", N3);		break;

		case CPUINFO_STR_REGISTER + DSP56K_M0:			sprintf(info->s = cpuintrf_temp_str(), "M0 : %04x", M0);		break;
		case CPUINFO_STR_REGISTER + DSP56K_M1:			sprintf(info->s = cpuintrf_temp_str(), "M1 : %04x", M1);		break;
		case CPUINFO_STR_REGISTER + DSP56K_M2:			sprintf(info->s = cpuintrf_temp_str(), "M2 : %04x", M2);		break;
		case CPUINFO_STR_REGISTER + DSP56K_M3:			sprintf(info->s = cpuintrf_temp_str(), "M3 : %04x", M3);		break;

		case CPUINFO_STR_REGISTER + DSP56K_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0 : %08x", ST0);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1 : %08x", ST1);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2 : %08x", ST2);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3 : %08x", ST3);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4 : %08x", ST4);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5 : %08x", ST5);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6 : %08x", ST6);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7 : %08x", ST7);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST8:			sprintf(info->s = cpuintrf_temp_str(), "ST8 : %08x", ST8);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST9:			sprintf(info->s = cpuintrf_temp_str(), "ST9 : %08x", ST9);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST10:		sprintf(info->s = cpuintrf_temp_str(), "ST10: %08x", ST10);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST11:		sprintf(info->s = cpuintrf_temp_str(), "ST11: %08x", ST11);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST12:		sprintf(info->s = cpuintrf_temp_str(), "ST12: %08x", ST12);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST13:		sprintf(info->s = cpuintrf_temp_str(), "ST13: %08x", ST13);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST14:		sprintf(info->s = cpuintrf_temp_str(), "ST14: %08x", ST14);	break;
		case CPUINFO_STR_REGISTER + DSP56K_ST15:		sprintf(info->s = cpuintrf_temp_str(), "ST15: %08x", ST15);	break;
	}
}

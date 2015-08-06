/*****************************************************************************
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   peter.trauner@jk.uni-linz.ac.at
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/
#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "debugger.h"
#include "cdp1802.h"

/* Layout of the registers in the debugger */
static UINT8 cdp1802_reg_layout[] = {
	CDP1802_R0,
	CDP1802_R1,
	CDP1802_R2,
	CDP1802_R3,
	CDP1802_R4,
	CDP1802_R5,
	CDP1802_R6,
	CDP1802_R7,
	-1,

	CDP1802_R8,
	CDP1802_R9,
	CDP1802_Ra,
	CDP1802_Rb,
	CDP1802_Rc,
	CDP1802_Rd,
	CDP1802_Re,
	CDP1802_Rf,
	-1,

	CDP1802_P,
	CDP1802_X,
	CDP1802_D,
	CDP1802_B,
	CDP1802_T,
	CDP1802_I,
	CDP1802_N,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 cdp1802_win_layout[] = {
	17, 0,63, 4,	/* register window (top, right rows) */
	 0, 0,16,22,	/* disassembler window (left colums) */
	17, 5,63, 8,	/* memory #1 window (right, upper middle) */
	17,14,63, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/****************************************************************************
 * The CDP1802 registers.
 ****************************************************************************/
typedef struct
{
	CDP1802_CONFIG *config;

	PAIR reg[0x10];
	int p, x; // indices to reg, p program count, x data pointer
	int n, i;

	UINT8 d, b, t; // xp after entering interrupt

	UINT16 oldpc;

	bool df, ie, q;
	bool irq_state;

	bool idle;
	int dma_cycles;
} CDP1802_Regs;

static int cdp1802_ICount;

static CDP1802_Regs cdp1802;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "1802tbl.c"

static void cdp1802_get_context (void *dst)
{
	if (dst)
	{
		*(CDP1802_Regs *)dst = cdp1802;
	}
}

static void cdp1802_set_context (void *src)
{
	if (src)
	{
		cdp1802 = *(CDP1802_Regs *)src;
		change_pc(PC);
	}
}

static void cdp1802_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	cdp1802.config = (CDP1802_CONFIG *) config;
}

static void cdp1802_reset(void)
{
	I = 0;
	N = 0;
	cdp1802_q(0);
	IE = 1;

	// initialization cycle, requires 9 clock pulses
	// interrupt and DMA servicing is suppressed

	X = 0;
	P = 0;
	R(0) = 0;

	change_pc(PC);

	cdp1802.idle = 0;
	cdp1802.dma_cycles = 0;
}

static int cdp1802_execute(int cycles)
{
	int ref = cycles;
	cdp1802_ICount = cycles;

	change_pc(PC);

	do
	{
		cdp1802.oldpc = PC;

		CALL_MAME_DEBUG;

		if (!cdp1802.idle)
			cdp1802_instruction();
		else
			cdp1802_ICount--;

		if (cdp1802.config->dma)
		{
			cdp1802.config->dma(ref-cdp1802_ICount);
			ref = cdp1802_ICount;
		}
	} while (cdp1802_ICount > 0);

	return cycles - cdp1802_ICount;
}

static unsigned cdp1802_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmCdp1802(buffer, pc);
#else
	sprintf( buffer, "$%X", cpu_readop(pc) );
	return 1;
#endif
}

static void cdp1802_set_irq_line(int irqline, int state)
{
	cdp1802.idle = 0;

	if (IE)
	{
		T = (X << 4) | P;
		X = 2;
		P = 1;
		IE = 0;
		change_pc(PC);
		cdp1802.irq_state = state;
	}
}

void cdp1802_dma_write(UINT8 data)
{
	MW(R(0)++, data);
	cdp1802.idle = 0;
	cdp1802_ICount--;
}

int cdp1802_dma_read(void)
{
	cdp1802.idle = 0;
	cdp1802_ICount--;
	return M(R(0)++);
}

/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void cdp1802_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + CDP1802_IRQ_STATE:	cdp1802_set_irq_line(CDP1802_IRQ, info->i);	break;

		case CPUINFO_INT_REGISTER + CDP1802_P: cdp1802.p = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_X: cdp1802.x = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_T: cdp1802.t = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_D: cdp1802.d = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_B: cdp1802.b = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R0: cdp1802.reg[0].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R1: cdp1802.reg[1].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R2: cdp1802.reg[2].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R3: cdp1802.reg[3].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R4: cdp1802.reg[4].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R5: cdp1802.reg[5].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R6: cdp1802.reg[6].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R7: cdp1802.reg[7].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R8: cdp1802.reg[8].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_R9: cdp1802.reg[9].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_Ra: cdp1802.reg[0xa].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_Rb: cdp1802.reg[0xb].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_Rc: cdp1802.reg[0xc].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_Rd: cdp1802.reg[0xd].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_Re: cdp1802.reg[0xe].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_Rf: cdp1802.reg[0xf].w.l = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_DF: cdp1802.df = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_IE: cdp1802.ie = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_Q: cdp1802_q(info->i); break;
		case CPUINFO_INT_REGISTER + CDP1802_N: cdp1802.n = info->i; break;
		case CPUINFO_INT_REGISTER + CDP1802_I: cdp1802.i = info->i; break;
	}
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
void cdp1802_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(cdp1802);				break;
		case CPUINFO_INT_INPUT_LINES:						info->i = 1;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 8;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 3;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 19;							break;
		
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 3;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + CDP1802_IRQ_STATE:		info->i = cdp1802.irq_state;		break;

		case CPUINFO_INT_PREVIOUSPC:						info->i = cdp1802.oldpc;			break;

		case CPUINFO_INT_PC:								info->i = PC;						break;
		case CPUINFO_INT_SP:								info->i = 0;						break;
		case CPUINFO_INT_REGISTER + CDP1802_P:				info->i = cdp1802.p;				break;
		case CPUINFO_INT_REGISTER + CDP1802_X:				info->i = cdp1802.x;				break;
		case CPUINFO_INT_REGISTER + CDP1802_T:				info->i = cdp1802.t;				break;
		case CPUINFO_INT_REGISTER + CDP1802_D:				info->i = cdp1802.d;				break;
		case CPUINFO_INT_REGISTER + CDP1802_B:				info->i = cdp1802.b;				break;
		case CPUINFO_INT_REGISTER + CDP1802_R0:				info->i = cdp1802.reg[0].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R1:				info->i = cdp1802.reg[1].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R2:				info->i = cdp1802.reg[2].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R3:				info->i = cdp1802.reg[3].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R4:				info->i = cdp1802.reg[4].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R5:				info->i = cdp1802.reg[5].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R6:				info->i = cdp1802.reg[6].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R7:				info->i = cdp1802.reg[7].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R8:				info->i = cdp1802.reg[8].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_R9:				info->i = cdp1802.reg[9].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_Ra:				info->i = cdp1802.reg[0xa].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_Rb:				info->i = cdp1802.reg[0xb].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_Rc:				info->i = cdp1802.reg[0xc].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_Rd:				info->i = cdp1802.reg[0xd].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_Re:				info->i = cdp1802.reg[0xe].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_Rf:				info->i = cdp1802.reg[0xf].w.l;		break;
		case CPUINFO_INT_REGISTER + CDP1802_DF:				info->i = cdp1802.df;				break;
		case CPUINFO_INT_REGISTER + CDP1802_IE:				info->i = cdp1802.ie;				break;
		case CPUINFO_INT_REGISTER + CDP1802_Q:				info->i = cdp1802.q;				break;
		case CPUINFO_INT_REGISTER + CDP1802_N:				info->i = cdp1802.n;				break;
		case CPUINFO_INT_REGISTER + CDP1802_I:				info->i = cdp1802.i;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = cdp1802_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = cdp1802_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = cdp1802_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = cdp1802_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = cdp1802_reset;			break;
		case CPUINFO_PTR_EXECUTE:						info->execute = cdp1802_execute;		break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = cdp1802_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &cdp1802_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = cdp1802_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = cdp1802_win_layout;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "CDP1802");	break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "CDP1802");	break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0");		break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__);	break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) 2000 Peter Trauner, all rights reserved."); break;

		case CPUINFO_STR_REGISTER + CDP1802_R0:	sprintf(info->s = cpuintrf_temp_str(), "R0:%.4x", cdp1802.reg[0].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R1:	sprintf(info->s = cpuintrf_temp_str(), "R1:%.4x", cdp1802.reg[1].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R2:	sprintf(info->s = cpuintrf_temp_str(), "R2:%.4x", cdp1802.reg[2].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R3:	sprintf(info->s = cpuintrf_temp_str(), "R3:%.4x", cdp1802.reg[3].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R4:	sprintf(info->s = cpuintrf_temp_str(), "R4:%.4x", cdp1802.reg[4].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R5:	sprintf(info->s = cpuintrf_temp_str(), "R5:%.4x", cdp1802.reg[5].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R6:	sprintf(info->s = cpuintrf_temp_str(), "R6:%.4x", cdp1802.reg[6].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R7:	sprintf(info->s = cpuintrf_temp_str(), "R7:%.4x", cdp1802.reg[7].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R8:	sprintf(info->s = cpuintrf_temp_str(), "R8:%.4x", cdp1802.reg[8].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_R9:	sprintf(info->s = cpuintrf_temp_str(), "R9:%.4x", cdp1802.reg[9].w.l);	break;
		case CPUINFO_STR_REGISTER + CDP1802_Ra:	sprintf(info->s = cpuintrf_temp_str(), "Ra:%.4x", cdp1802.reg[0xa].w.l);break;
		case CPUINFO_STR_REGISTER + CDP1802_Rb:	sprintf(info->s = cpuintrf_temp_str(), "Rb:%.4x", cdp1802.reg[0xb].w.l);break;
		case CPUINFO_STR_REGISTER + CDP1802_Rc:	sprintf(info->s = cpuintrf_temp_str(), "Rc:%.4x", cdp1802.reg[0xc].w.l);break;
		case CPUINFO_STR_REGISTER + CDP1802_Rd:	sprintf(info->s = cpuintrf_temp_str(), "Rd:%.4x", cdp1802.reg[0xd].w.l);break;
		case CPUINFO_STR_REGISTER + CDP1802_Re:	sprintf(info->s = cpuintrf_temp_str(), "Re:%.4x", cdp1802.reg[0xe].w.l);break;
		case CPUINFO_STR_REGISTER + CDP1802_Rf:	sprintf(info->s = cpuintrf_temp_str(), "Rf:%.4x", cdp1802.reg[0xf].w.l);break;
		case CPUINFO_STR_REGISTER + CDP1802_P:	sprintf(info->s = cpuintrf_temp_str(), "P:%x", cdp1802.p);	break;
		case CPUINFO_STR_REGISTER + CDP1802_X:	sprintf(info->s = cpuintrf_temp_str(), "X:%x", cdp1802.x);	break;
		case CPUINFO_STR_REGISTER + CDP1802_D:	sprintf(info->s = cpuintrf_temp_str(), "D:%.2x", cdp1802.d);break;
		case CPUINFO_STR_REGISTER + CDP1802_B:	sprintf(info->s = cpuintrf_temp_str(), "B:%.2x", cdp1802.b);break;
		case CPUINFO_STR_REGISTER + CDP1802_T:	sprintf(info->s = cpuintrf_temp_str(), "T:%.2x", cdp1802.t);break;
		case CPUINFO_STR_REGISTER + CDP1802_DF:	sprintf(info->s = cpuintrf_temp_str(), "DF:%x", cdp1802.df);break;
		case CPUINFO_STR_REGISTER + CDP1802_IE:	sprintf(info->s = cpuintrf_temp_str(), "IE:%x", cdp1802.ie);break;
		case CPUINFO_STR_REGISTER + CDP1802_Q:	sprintf(info->s = cpuintrf_temp_str(), "Q:%x", cdp1802.q);	break;
		case CPUINFO_STR_REGISTER + CDP1802_N:	sprintf(info->s = cpuintrf_temp_str(), "N:%x", cdp1802.n);	break;
		case CPUINFO_STR_REGISTER + CDP1802_I:	sprintf(info->s = cpuintrf_temp_str(), "I:%x", cdp1802.i);	break;
		case CPUINFO_STR_FLAGS: sprintf(info->s = cpuintrf_temp_str(),
									"%s%s%s",
									 cdp1802.df ? "DF" : "..",
									 cdp1802.ie ? "IE" : "..",
									 cdp1802.q ? "Q" : "."); break;
	}
}

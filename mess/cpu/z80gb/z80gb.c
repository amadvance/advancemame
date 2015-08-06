/*************************************************************/
/**                                                         **/
/** 						 z80gb.c						**/
/**                                                         **/
/** This file contains implementation for the GameBoy CPU.  **/
/** See z80gb.h for the relevant definitions. Please, note	**/
/** that this code can not be used to emulate a generic Z80 **/
/** because the GameBoy version of it differs from Z80 in   **/
/** many ways.                                              **/
/**                                                         **/
/** Orginal cpu code (PlayBoy)	Carsten Sorensen	1998	**/
/** MESS modifications			Hans de Goede		1998	**/
/** Adapted to new cpuintrf 	Juergen Buchmueller 2000	**/
/** Adapted to new cpuintrf		Anthony Kruize		2002	**/
/** Changed reset function to                               **/
/** reset all registers instead                             **/
/** of just AF.                            Wilbert Pol 2004 **/
/**                                                         **/
/** TODO: Check cycle counts when leaving HALT state        **/
/**                                                         **/
/*************************************************************/
#include "z80gb.h"
#include "daa_tab.h"
#include "debugger.h"

#define FLAG_Z	0x80
#define FLAG_N  0x40
#define FLAG_H  0x20
#define FLAG_C  0x10

static UINT8 z80gb_reg_layout[] = {
	Z80GB_PC, Z80GB_SP, Z80GB_AF, Z80GB_BC, Z80GB_DE, Z80GB_HL, -1,
	Z80GB_IRQ_STATE, 0
};

static UINT8 z80gb_win_layout[] = {
	27, 0,53, 4,	/* register window (top rows) */
	 0, 0,26,22,	/* disassembler window (left colums) */
	27, 5,53, 8,	/* memory #1 window (right, upper middle) */
	27,14,53, 8,	/* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};

/* Nr of cycles to run */
extern int z80gb_ICount;

typedef struct {
	UINT16 AF;
	UINT16 BC;
	UINT16 DE;
	UINT16 HL;

	UINT16 SP;
	UINT16 PC;
	int enable;
	int irq_state;
	int (*irq_callback)(int irqline);
	int leavingHALT;
	int doHALTbug;
	const UINT16 *config;
} z80gb_16BitRegs;

#ifdef LSB_FIRST
typedef struct {
	UINT8 F;
	UINT8 A;
	UINT8 C;
	UINT8 B;
	UINT8 E;
	UINT8 D;
	UINT8 L;
	UINT8 H;
} z80gb_8BitRegs;
#else
typedef struct {
	UINT8 A;
	UINT8 F;
	UINT8 B;
	UINT8 C;
	UINT8 D;
	UINT8 E;
	UINT8 H;
	UINT8 L;
} z80gb_8BitRegs;
#endif

typedef union {
	z80gb_16BitRegs w;
	z80gb_8BitRegs b;
} z80gb_regs;

typedef int (*OpcodeEmulator) (void);

static z80gb_regs Regs;
static UINT8 ICycles;
static UINT8 CheckInterrupts;

#define IME     0x01
#define HALTED	0x02

/* Nr of cycles to run */
int z80gb_ICount;

static int Cycles[256] =
{
	 4,12, 8, 8, 4, 4, 8, 4,20, 8, 8, 8, 4, 4, 8, 4,
	 4,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,
	 8,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,
	 8,12, 8, 8,12,12,12, 4, 8, 8, 8, 8, 4, 4, 8, 4,
	 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	 8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4,
	 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	 8,12,12,12,12,16, 8,16, 8, 8,12, 0,12,24, 8,16,
	 8,12,12, 4,12,16, 8,16, 8,16,12, 4,12, 4, 8,16,
	12,12, 8, 4, 4,16, 8,16,16, 4,16, 4, 4, 4, 8,16,
	12,12, 8, 4, 4,16, 8,16,12, 8,16, 4, 4, 4, 8,16
};

static int CyclesCB[256] =
{
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8,
	 8, 8, 8, 8, 8, 8,16, 8, 8, 8, 8, 8, 8, 8,16, 8
};

static void z80gb_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	Regs.w.config = (const UINT16 *) config;
	Regs.w.irq_callback = irqcallback;
}

/*** Reset Z80 registers: *********************************/
/*** This function can be used to reset the register    ***/
/*** file before starting execution with z80gb_execute()***/
/*** It sets the registers to their initial values.     ***/
/**********************************************************/
static void z80gb_reset(void)
{
	if (Regs.w.config)
	{
		Regs.w.AF = Regs.w.config[0];
		Regs.w.BC = Regs.w.config[1];
		Regs.w.DE = Regs.w.config[2];
		Regs.w.HL = Regs.w.config[3];
		Regs.w.SP = Regs.w.config[4];
		Regs.w.PC = Regs.w.config[5];
	}
	else
	{
		Regs.w.AF = 0x0000;
		Regs.w.BC = 0x0000;
		Regs.w.DE = 0x0000;
		Regs.w.HL = 0x0000;
		Regs.w.SP = 0x0000;
		Regs.w.PC = 0x0000;
	}
	Regs.w.enable &= ~IME;

//FIXME, use this in gb_machine_init!     state->TimerShift=32;
	CheckInterrupts = 0;
	Regs.w.leavingHALT = 0;
	Regs.w.doHALTbug = 0;
}

INLINE void z80gb_ProcessInterrupts (void)
{

	if (CheckInterrupts && (Regs.w.enable & IME))
	{
		UINT8 irq;
		CheckInterrupts = 0;
		irq = ISWITCH & IFLAGS;

		/*
		logerror("Attempting to process Z80GB Interrupt IRQ $%02X\n", irq);
		logerror("Attempting to process Z80GB Interrupt ISWITCH $%02X\n", ISWITCH);
		logerror("Attempting to process Z80GB Interrupt IFLAGS $%02X\n", IFLAGS);
		*/

		if (irq)
		{
			int irqline = 0;
			/*
			logerror("Z80GB Interrupt IRQ $%02X\n", irq);
			*/

			for( ; irqline < 5; irqline++ )
			{
				if( irq & (1<<irqline) )
				{
					if( Regs.w.irq_callback )
							(*Regs.w.irq_callback)(irqline);
					if (Regs.w.enable & HALTED)
					{
						Regs.w.enable &= ~HALTED;
						Regs.w.leavingHALT++;
					}
					Regs.w.enable &= ~IME;
					IFLAGS &= ~(1 << irqline);
					ICycles += 19; /* RST cycles (16) + irq latency (2/3/4??) */
					Regs.w.SP -= 2;
					mem_WriteWord (Regs.w.SP, Regs.w.PC);
					Regs.w.PC = 0x40 + irqline * 8;
					/*logerror("Z80GB Interrupt PC $%04X\n", Regs.w.PC );*/
					return;
				}
			}
		}
	}
}

/**********************************************************/
/*** Execute z80gb code for cycles cycles, return nr of ***/
/*** cycles actually executed.                          ***/
/**********************************************************/
static int z80gb_execute (int cycles)
{
	UINT8 x;

	z80gb_ICount = cycles;

	do
	{
		CALL_MAME_DEBUG;
		ICycles = 0;
		z80gb_ProcessInterrupts ();
		if ( Regs.w.enable & HALTED ) {
			ICycles += Cycles[0x76];
		} else {
			x = mem_ReadByte (Regs.w.PC++);
			if ( Regs.w.doHALTbug ) {
				Regs.w.PC--;
				Regs.w.doHALTbug = 0;
			}
			ICycles += Cycles[x];
			switch (x)
			{
#include "opc_main.h"
			}
		}
		z80gb_ICount -= ICycles;
		gb_divcount += ICycles;
		if (TIMEFRQ & 0x04)
		{
			gb_timer_count += ICycles;
			if (gb_timer_count & (0xFF00 << gb_timer_shift))
			{
				gb_timer_count = TIMEMOD << gb_timer_shift;
				IFLAGS |= TIM_IFLAG;
				CheckInterrupts = 1;
			}
		}
	} while (z80gb_ICount > 0);

	return cycles - z80gb_ICount;
}

static void z80gb_burn(int cycles)
{
    if( cycles > 0 )
    {
        /* NOP takes 4 cycles per instruction */
        int n = (cycles + 3) / 4;
        z80gb_ICount -= 4 * n;
    }
}

/****************************************************************************/
/* Set all registers to given values                                        */
/****************************************************************************/
static void z80gb_set_context (void *src)
{
	if( src )
		Regs = *(z80gb_regs *)src;
	change_pc(Regs.w.PC);
}

/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/
static void z80gb_get_context (void *dst)
{
	if( dst )
		*(z80gb_regs *)dst = Regs;
}

/****************************************************************************
 * Get a specific register
 ****************************************************************************/
unsigned z80gb_get_reg (int regnum)
{
	switch( regnum )
	{
	case REG_PC: return Regs.w.PC;
	case Z80GB_PC: return Regs.w.PC;
	case REG_SP: return Regs.w.SP;
	case Z80GB_SP: return Regs.w.SP;
	case Z80GB_AF: return Regs.w.AF;
	case Z80GB_BC: return Regs.w.BC;
	case Z80GB_DE: return Regs.w.DE;
	case Z80GB_HL: return Regs.w.HL;
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
static void z80gb_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case REG_PC: Regs.w.PC = val; change_pc(Regs.w.PC); break;
	case Z80GB_PC: Regs.w.PC = val; break;
	case REG_SP: Regs.w.SP = val; break;
	case Z80GB_SP: Regs.w.SP = val; break;
	case Z80GB_AF: Regs.w.AF = val; break;
	case Z80GB_BC: Regs.w.BC = val; break;
	case Z80GB_DE: Regs.w.DE = val; break;
	case Z80GB_HL: Regs.w.HL = val; break;
    }
}

static void z80gb_set_irq_line (int irqline, int state)
{
	/*logerror("setting irq line 0x%02x state 0x%08x\n", irqline, state);*/
	//if( Regs.w.irq_state == state )
	//	return;

	Regs.w.irq_state = state;
	if( state == ASSERT_LINE )
	{

		IFLAGS |= (0x01 << irqline);
		CheckInterrupts = 1;
		/*logerror("Z80GB assert irq line %d ($%02X)\n", irqline, IFLAGS);*/

	}
	else
	{

		IFLAGS &= ~(0x01 << irqline);
		if( IFLAGS == 0 )
			CheckInterrupts = 0;
		/*logerror("Z80GB clear irq line %d ($%02X)\n", irqline, IFLAGS);*/

     }
}

/*static void z80gb_clear_pending_interrupts (void)
{
	IFLAGS = 0;
	CheckInterrupts = 0;
}*/

static const char *z80gb_info(void *context, int regnum)
{
	static char buffer[8][47+1];
	static int which = 0;
	z80gb_regs *r = context;

	which = (which + 1) % 8;
    buffer[which][0] = '\0';
	if( !context )
		r = &Regs;

    switch( regnum )
	{
	}
	return buffer[which];
}

static void z80gb_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
	/* --- the following bits of info are set as 64-bit signed integers --- */
	case CPUINFO_INT_INPUT_STATE + 0:
	case CPUINFO_INT_INPUT_STATE + 1:
	case CPUINFO_INT_INPUT_STATE + 2:
	case CPUINFO_INT_INPUT_STATE + 3:
	case CPUINFO_INT_INPUT_STATE + 4:				z80gb_set_irq_line(state-CPUINFO_INT_INPUT_STATE, info->i); break;

	case CPUINFO_INT_SP:						Regs.w.SP = info->i;						break;
	case CPUINFO_INT_PC:						Regs.w.PC = info->i; change_pc(Regs.w.PC); break;

	case CPUINFO_INT_REGISTER + Z80GB_PC:		Regs.w.PC = info->i;						break;
	case CPUINFO_INT_REGISTER + Z80GB_SP:		Regs.w.SP = info->i;						break;
	case CPUINFO_INT_REGISTER + Z80GB_AF:		Regs.w.AF = info->i;						break;
	case CPUINFO_INT_REGISTER + Z80GB_BC:		Regs.w.BC = info->i;						break;
	case CPUINFO_INT_REGISTER + Z80GB_DE:		Regs.w.DE = info->i;						break;
	case CPUINFO_INT_REGISTER + Z80GB_HL:		Regs.w.HL = info->i;						break;
	}
}

void z80gb_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(Regs);					break;
	case CPUINFO_INT_INPUT_LINES:						info->i = 5;							break;
	case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0xff;							break;
	case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
	case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
	case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
	case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
	case CPUINFO_INT_MIN_CYCLES:					info->i = 1;	/* right? */			break;
	case CPUINFO_INT_MAX_CYCLES:					info->i = 16;	/* right? */			break;

	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
	case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
	case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 16;					break;
	case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

	case CPUINFO_INT_SP:							info->i = Regs.w.SP;					break;
	case CPUINFO_INT_PC:							info->i = Regs.w.PC;					break;
	case CPUINFO_INT_PREVIOUSPC:					info->i = 0;	/* TODO??? */			break;

	case CPUINFO_INT_INPUT_STATE + 0:
	case CPUINFO_INT_INPUT_STATE + 1:
	case CPUINFO_INT_INPUT_STATE + 2:
	case CPUINFO_INT_INPUT_STATE + 3:
	case CPUINFO_INT_INPUT_STATE + 4:					info->i = IFLAGS & (1 << (state-CPUINFO_INT_INPUT_STATE)); break;

	case CPUINFO_INT_REGISTER + Z80GB_PC:			info->i = Regs.w.PC;					break;
	case CPUINFO_INT_REGISTER + Z80GB_SP:			info->i = Regs.w.SP;					break;
	case CPUINFO_INT_REGISTER + Z80GB_AF:			info->i = Regs.w.AF;					break;
	case CPUINFO_INT_REGISTER + Z80GB_BC:			info->i = Regs.w.BC;					break;
	case CPUINFO_INT_REGISTER + Z80GB_DE:			info->i = Regs.w.DE;					break;
	case CPUINFO_INT_REGISTER + Z80GB_HL:			info->i = Regs.w.HL;					break;

	/* --- the following bits of info are returned as pointers to data or functions --- */
	case CPUINFO_PTR_SET_INFO:						info->setinfo = z80gb_set_info;			break;
	case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = z80gb_get_context;	break;
	case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = z80gb_set_context;	break;
	case CPUINFO_PTR_INIT:							info->init = z80gb_init;				break;
	case CPUINFO_PTR_RESET:							info->reset = z80gb_reset;				break;
	case CPUINFO_PTR_EXECUTE:						info->execute = z80gb_execute;			break;
	case CPUINFO_PTR_BURN:							info->burn = z80gb_burn;						break;

#ifdef MAME_DEBUG
	case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = z80gb_dasm;	break;
#endif
	case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &z80gb_ICount;			break;
	case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = z80gb_reg_layout;				break;
	case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = z80gb_win_layout;				break;

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	case CPUINFO_STR_NAME: 							strcpy(info->s = cpuintrf_temp_str(), "Z80GB"); break;
	case CPUINFO_STR_CORE_FAMILY: 					strcpy(info->s = cpuintrf_temp_str(), "Nintendo Z80"); break;
	case CPUINFO_STR_CORE_VERSION: 					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
	case CPUINFO_STR_CORE_FILE: 					strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
	case CPUINFO_STR_CORE_CREDITS: 					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2000 by The MESS Team."); break;

	case CPUINFO_STR_FLAGS:
		sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
			Regs.b.F & 0x80 ? 'Z':'.',
			Regs.b.F & 0x40 ? 'N':'.',
			Regs.b.F & 0x20 ? 'H':'.',
			Regs.b.F & 0x10 ? 'C':'.',
			Regs.b.F & 0x08 ? '3':'.',
			Regs.b.F & 0x04 ? '2':'.',
			Regs.b.F & 0x02 ? '1':'.',
			Regs.b.F & 0x01 ? '0':'.');
		break;

	case CPUINFO_STR_REGISTER + Z80GB_PC: sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", Regs.w.PC); break;
	case CPUINFO_STR_REGISTER + Z80GB_SP: sprintf(info->s = cpuintrf_temp_str(), "SP:%04X", Regs.w.SP); break;
	case CPUINFO_STR_REGISTER + Z80GB_AF: sprintf(info->s = cpuintrf_temp_str(), "AF:%04X", Regs.w.AF); break;
	case CPUINFO_STR_REGISTER + Z80GB_BC: sprintf(info->s = cpuintrf_temp_str(), "BC:%04X", Regs.w.BC); break;
	case CPUINFO_STR_REGISTER + Z80GB_DE: sprintf(info->s = cpuintrf_temp_str(), "DE:%04X", Regs.w.DE); break;
	case CPUINFO_STR_REGISTER + Z80GB_HL: sprintf(info->s = cpuintrf_temp_str(), "HL:%04X", Regs.w.HL); break;
	case CPUINFO_STR_REGISTER + Z80GB_IRQ_STATE: sprintf(info->s = cpuintrf_temp_str(), "IRQ:%X", Regs.w.irq_state); break;
	}
}

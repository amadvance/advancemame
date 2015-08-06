/***************************************************************************

    dsp56k.c
    Core implementation for the portable DSP56k emulator.
    Written by Andrew Gardner

****************************************************************************

    Note:

    This CPU emulator is very much a work-in-progress.  Thus far, it appears to be
    complete enough to run the memory tests for Polygonet Commanders.

    Some particularly WIP-like features of this core are as follows :
     * I ask many questions about my code throughout the core
     * The BITS(bits,op) macro is fine for a disassembler, but VERY slow for the
         inner loops of an executing core.  This will go away someday

***************************************************************************/

#include "debugger.h"
#include "dsp56k.h"

// #define PC_E000

/***************************************************************************
    MACROS
***************************************************************************/

// ??? Are there namespace collision issues with just defining something "PC" ???
//     ...doesn't seem like it, but one never knows...

// Register macros
#define PC		dsp56k.pcuProgramCounter
#define SR		dsp56k.pcuStatus
#define OMR		dsp56k.pcuOperatingModeReg
#define SP		dsp56k.pcuStackPointer
#define LA		dsp56k.pcuLoopAddressReg
#define LC		dsp56k.pcuLoopCounter

#define SSH		dsp56k.pcuSystemStack[SP].w.h
#define SSL		dsp56k.pcuSystemStack[SP].w.l


#define X		dsp56k.aluDataRegs[0].d
#define X1		dsp56k.aluDataRegs[0].w.h
#define X0		dsp56k.aluDataRegs[0].w.l
#define Y		dsp56k.aluDataRegs[1].d
#define Y1		dsp56k.aluDataRegs[1].w.h
#define Y0		dsp56k.aluDataRegs[1].w.l

#define A		dsp56k.aluAccumRegs[0].lw
#define A2		dsp56k.aluAccumRegs[0].b.h4
#define A1		dsp56k.aluAccumRegs[0].w.h
#define A0		dsp56k.aluAccumRegs[0].w.l
#define B		dsp56k.aluAccumRegs[1].lw
#define B2		dsp56k.aluAccumRegs[1].b.h4
#define B1		dsp56k.aluAccumRegs[1].w.h
#define B0		dsp56k.aluAccumRegs[1].w.l

#define R0		dsp56k.aguAddressRegs[0]
#define R1		dsp56k.aguAddressRegs[1]
#define R2		dsp56k.aguAddressRegs[2]
#define R3		dsp56k.aguAddressRegs[3]

#define N0		dsp56k.aguOffsetRegs[0]
#define N1		dsp56k.aguOffsetRegs[1]
#define N2		dsp56k.aguOffsetRegs[2]
#define N3		dsp56k.aguOffsetRegs[3]

#define M0		dsp56k.aguModifierRegs[0]
#define M1		dsp56k.aguModifierRegs[1]
#define M2		dsp56k.aguModifierRegs[2]
#define M3		dsp56k.aguModifierRegs[3]

#define TEMP	dsp56k.aguTempReg
#define STATUS	dsp56k.aguStatusReg

#define IPR     dsp56k.interruptPriorityRegister
#define BCR     dsp56k.busControlRegister

// The CPU Stack
#define ST0		dsp56k.pcuSystemStack[0].d
#define ST1		dsp56k.pcuSystemStack[1].d
#define ST2		dsp56k.pcuSystemStack[2].d
#define ST3		dsp56k.pcuSystemStack[3].d
#define ST4		dsp56k.pcuSystemStack[4].d
#define ST5		dsp56k.pcuSystemStack[5].d
#define ST6		dsp56k.pcuSystemStack[6].d
#define ST7		dsp56k.pcuSystemStack[7].d
#define ST8		dsp56k.pcuSystemStack[8].d
#define ST9		dsp56k.pcuSystemStack[9].d
#define ST10	dsp56k.pcuSystemStack[10].d
#define ST11	dsp56k.pcuSystemStack[11].d
#define ST12	dsp56k.pcuSystemStack[12].d
#define ST13	dsp56k.pcuSystemStack[13].d
#define ST14	dsp56k.pcuSystemStack[14].d
#define ST15	dsp56k.pcuSystemStack[15].d
// !!! Is there really only 15 of them, or is there 16 ???

// Other
#define OP		dsp56k.op



// Status Register Flags
#define lfFLAG ((SR & 0x8000) != 0)
#define fvFLAG ((SR & 0x4000) != 0)
#define s1FLAG ((SR & 0x0800) != 0)
#define s0FLAG ((SR & 0x0400) != 0)
#define i1FLAG ((SR & 0x0200) != 0)
#define i0FLAG ((SR & 0x0100) != 0)
#define sFLAG  ((SR & 0x0080) != 0)
#define lFLAG  ((SR & 0x0040) != 0)
#define eFLAG  ((SR & 0x0020) != 0)
#define uFLAG  ((SR & 0x0010) != 0)
#define nFLAG  ((SR & 0x0008) != 0)
#define zFLAG  ((SR & 0x0004) != 0)
#define vFLAG  ((SR & 0x0002) != 0)
#define cFLAG  ((SR & 0x0001) != 0)

#define CLEAR_lfFLAG() (SR &= (~0x8000))
#define CLEAR_fvFLAG() (SR &= (~0x4000))
#define CLEAR_s1FLAG() (SR &= (~0x0800))
#define CLEAR_s0FLAG() (SR &= (~0x0400))
#define CLEAR_i1FLAG() (SR &= (~0x0200))
#define CLEAR_i0FLAG() (SR &= (~0x0100))
#define CLEAR_sFLAG()  (SR &= (~0x0080))
#define CLEAR_lFLAG()  (SR &= (~0x0040))
#define CLEAR_eFLAG()  (SR &= (~0x0020))
#define CLEAR_uFLAG()  (SR &= (~0x0010))
#define CLEAR_nFLAG()  (SR &= (~0x0008))
#define CLEAR_zFLAG()  (SR &= (~0x0004))
#define CLEAR_vFLAG()  (SR &= (~0x0002))
#define CLEAR_cFLAG()  (SR &= (~0x0001))

#define SET_lfFLAG() (SR |= 0x8000)
#define SET_fvFLAG() (SR |= 0x4000)
#define SET_s1FLAG() (SR |= 0x0800)
#define SET_s0FLAG() (SR |= 0x0400)
#define SET_i1FLAG() (SR |= 0x0200)
#define SET_i0FLAG() (SR |= 0x0100)
#define SET_sFLAG()  (SR |= 0x0080)
#define SET_lFLAG()  (SR |= 0x0040)
#define SET_eFLAG()  (SR |= 0x0020)
#define SET_uFLAG()  (SR |= 0x0010)
#define SET_nFLAG()  (SR |= 0x0008)
#define SET_zFLAG()  (SR |= 0x0004)
#define SET_vFLAG()  (SR |= 0x0002)
#define SET_cFLAG()  (SR |= 0x0001)



// Stack Pointer Flags
#define ufFLAG  ((SP & 0x20) != 0)
#define seFLAG  ((SP & 0x10) != 0)

#define CLEAR_ufFLAG() (SP &= (~0x20))
#define CLEAR_seFLAG() (SP &= (~0x10))

#define SET_ufFLAG() (SP |= 0x20)
#define SET_seFLAG() (SP |= 0x10)



// Operating Mode Register Flags
#define cdFLAG	((OMR & 0x80) != 0)
#define sdFLAG	((OMR & 0x40) != 0)
#define  rFLAG	((OMR & 0x20) != 0)
#define saFLAG	((OMR & 0x10) != 0)
#define mcFLAG	((OMR & 0x04) != 0)
#define mbFLAG	((OMR & 0x02) != 0)
#define maFLAG	((OMR & 0x01) != 0)

#define CLEAR_cdFLAG() (OMR &= (~0x80))
#define CLEAR_sdFLAG() (OMR &= (~0x40))
#define CLEAR_rFLAG()  (OMR &= (~0x20))
#define CLEAR_saFLAG() (OMR &= (~0x10))
#define CLEAR_mcFLAG() (OMR &= (~0x04))
#define CLEAR_mbFLAG() (OMR &= (~0x02))
#define CLEAR_maFLAG() (OMR &= (~0x01))

#define SET_cdFLAG() (OMR |= 0x80)
#define SET_sdFLAG() (OMR |= 0x40)
#define SET_rFLAG()  (OMR |= 0x20)
#define SET_saFLAG() (OMR |= 0x10)
#define SET_mcFLAG() (OMR |= 0x04)
#define SET_mbFLAG() (OMR |= 0x02)
#define SET_maFLAG() (OMR |= 0x01)


// IRQ Interfaces
#define LINE_MODA  (dsp56k.irq_modA)
#define LINE_MODB  (dsp56k.irq_modB)
#define LINE_MODC  (dsp56k.irq_modC)
#define LINE_RESET (dsp56k.irq_reset)



/***************************************************************************
    STRUCTURES & TYPEDEFS
***************************************************************************/

// DSP56156 Registers - sizes specific to chip
typedef struct
{
	// See section 1-22 in DSP56156UM.pdf for scrutinization...

	// PCU Registers
	UINT16			pcuProgramCounter ;			//  PC
	UINT16			pcuStatus ;					//  MR,CCR / SR
	UINT16			pcuLoopCounter ;			//  LC
	UINT16			pcuLoopAddressReg ;			//  LA
	UINT8			pcuStackPointer ;			//  SP
	UINT8			pcuOperatingModeReg ;		//  OMR
	PAIR			pcuSystemStack[16] ;		//  SSH,SSL (*15)

	// ALU Registers
	PAIR			aluDataRegs[2] ;			//  X1,X0     &   Y1,Y0
	PAIR64			aluAccumRegs[2] ;			//  A2,A1,A0  &   B2,B1,B0

	// AGU Registers
	UINT16          aguAddressRegs[4] ;			//  R0,R1,R2,R3
	UINT16          aguOffsetRegs[4] ;			//  N0,N1,N2,N3
	UINT16          aguModifierRegs[4] ;		//  M0,M1,M2,M3
	UINT16          aguTempReg ;				//  TEMP
	UINT8           aguStatusReg ;				//  Status

	// Other Registers
	UINT16          interruptPriorityRegister ; //
	UINT16          busControlRegister ;		//  Oddly poorly-documented - has RH and RS bits though...

	// IRQ lines
	UINT8           irq_modA ;					//  aka IRQA - can be defined edge or level sensitive (though i'm not sure how)
	UINT8           irq_modB ;					//  aka IRQA - can be defined edge or level sensitive (though i'm not sure how)
	UINT8           irq_modC ;					//  just modC :)
	UINT8           irq_reset ;					//  Always level-sensitive

	int		(*irq_callback)(int irqline) ;

	// Host Interface (HI) port - page 94 of DSP56156UM


	// Internal Stuff
	UINT32			ppc;						// Previous PC - for debugger
	UINT16			op;							// Current opcode
	int				interrupt_cycles;

	int				repFlag ;					// Knowing if we're in a 'repeat' state (dunno how the processor does this)
	UINT32			repAddr ;					// The address of the instruction to repeat...

	// DSP56156RAM-type processor
	UINT16 programRAM[2048] ;					// 2048 x 16-bit on-chip program RAM
	UINT16 dataRAM[2048] ;						// 2048 x 16-bit on-chip data RAM
	UINT16 bootstrapROM[64] ;					// 64   x 16-bit bootstrap ROM

	const void *	config;
} dsp56k_regs;

/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void dsp56k_reset(void);



/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static dsp56k_regs dsp56k;
static int dsp56k_icount;


/***************************************************************************
    IRQ HANDLING
***************************************************************************/

static void check_irqs(void)
{

}


static void set_irq_line(int irqline, int state)
{
	if (irqline == 3)
	{
		LINE_RESET = state ;

		if(LINE_RESET != CLEAR_LINE)
		{
			int irq_vector = (*dsp56k.irq_callback)(3);

			PC = irq_vector ;

			LINE_RESET = CLEAR_LINE ;
		}
	}
}



/***************************************************************************
    CONTEXT SWITCHING
***************************************************************************/

static void dsp56k_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(dsp56k_regs *)dst = dsp56k;
}


static void dsp56k_set_context(void *src)
{
	/* copy the context */
	if (src)
		dsp56k = *(dsp56k_regs *)src;
	memory_set_opbase(PC);

	/* check for IRQs */
	check_irqs();
}



/***************************************************************************
    INITIALIZATION AND SHUTDOWN
***************************************************************************/

static void dsp56k_init(int index, int clock, const void *_config, int (*irqcallback)(int))
{
	dsp56k.config = _config;
	dsp56k.irq_callback = irqcallback;
}

// Page 101 (7-25) in the Family Manual
static void dsp56k_reset(void)
{
	if (dsp56k.config == NULL)
	{
		LINE_RESET = 1 ;			/* hack - hold the CPU reset at startup */
		memory_set_opbase(PC);

		// Handle internal stuff
		dsp56k.interrupt_cycles = 0 ;


		// Internal peripheral devices are reset, and pins revert to general I/O pins

		// Modifier registers are set
		M0 = M1 = M2 = M3 = 0xffff ;

		// BCR is set
		BCR = 0x43ff ;

		// Stack pointer is cleared
		SP = 0x00 ;					// The docs say nothing about ufFLAG & seFLAG, but this should be right

		// Documentation says 'MR' is setup, but it really means 'SR' is setup
		SR = 0x0300 ;				// Only the Interrupt mask bits of the Status Register are set upon reset


		OMR = 0x00 ;				// All is cleared, except for the IRQ lines below
		IPR = 0x00 ;

		dsp56k.repFlag = 0 ;		// Certainly not repeating to start
		dsp56k.repAddr = 0x0000 ;	// Reset the address too...

		// Wait here in RESET state until the RESET interrupt is deasserted
	//  UINT64 waitLoop = 0 ;
	//  while(LINE_RESET)
	//  {
	//      waitLoop++ ;
	//  }

		// Manipulate everything you need to for the ports (!! maybe these will be callbacks someday !!)...
		data_write_word_16le(0xffde, 0x43ff) ;	// Sets Port A Bus Control Register (BCR) to the really slow bootup mode...
												// Also sets the Bus State status bit high (0x4xxx)
		data_write_word_16le(0xffc0, 0x0000) ;	// Sets Port B Control Register to general I/O
		data_write_word_16le(0xffc2, 0x0000) ;	// Sets Port B Data Direction Register as input
		data_write_word_16le(0xffc1, 0x0000) ;	// Sets Port C Control Register to general I/O
		data_write_word_16le(0xffc3, 0x0000) ;	// Sets Port C Data Direction Register as input

		// Now that we're leaving, set ma, mb, and mc from MODA, MODB, and MODC lines
		// I believe polygonet sets everyone to mode 0...  The following reflects this...
		CLEAR_maFLAG() ;
		CLEAR_mbFLAG() ;

		// switch bootup sequence based on chip operating mode
		switch((mbFLAG << 1) | maFLAG)
		{
			// [Special Bootstrap 1] Bootstrap from an external byte-wide memory located at P:$c000
			case 0x0:
				PC = 0x0000 ; // 0x0030 ; // 0x0032 ; // 0x002e ; // 0x0000 ; // 0x002c ;
				break ;

			// [Special Bootstrap 2] Bootstrap from the Host port or SSI0
			case 0x1:
				PC = 0x0000 ;
				break ;

			// [Normal Expanded] Internal PRAM enabled; External reset at P:$e000
			case 0x2:
				PC = 0xe000 ;
				break ;

			// [Development Expanded] Int. program memory disabled; Ext. reset at P:$0000
			case 0x3:
				PC = 0x0000 ;
				break ;
		}
	}
	else
	{
		PC = *((UINT16*)dsp56k.config) ;
	}
}


static void dsp56k_exit(void)
{
}



/***************************************************************************
    CORE INCLUDE
***************************************************************************/

#define ROPCODE(pc)   cpu_readop16(pc)

#include "dsp56ops.c"



/***************************************************************************
    CORE EXECUTION LOOP
***************************************************************************/

static int dsp56k_execute(int cycles)
{
	/* skip if halted */
	if (LINE_RESET)
		return cycles;

	dsp56k_icount = cycles;
	dsp56k_icount -= dsp56k.interrupt_cycles;
	dsp56k.interrupt_cycles = 0;

	while(dsp56k_icount > 0)
		execute_one() ;

	dsp56k_icount -= dsp56k.interrupt_cycles;
	dsp56k.interrupt_cycles = 0;

	return cycles - dsp56k_icount ;
}



/***************************************************************************
    DEBUGGER DEFINITIONS
***************************************************************************/

static UINT8 dsp56k_reg_layout[] =
{
	0
};

static UINT8 dsp56k_win_layout[] =
{
	 0, 0,30,20,	/* register window (top rows) */
	31, 0,48,14,	/* disassembler window (left colums) */
	 0,21,30, 1,	/* memory #1 window (right, upper middle) */
	31,15,48, 7,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};



/***************************************************************************
    DISASSEMBLY HOOK
***************************************************************************/

static offs_t dsp56k_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	extern unsigned dasm_dsp56k(char *, unsigned);
	return dasm_dsp56k(buffer, pc);
#else
	strcpy(buffer, "???");
	return 2;
#endif
}





/**************************************************************************
 * Generic set_info/get_info
 **************************************************************************/

static void dsp56k_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODA:  set_irq_line(DSP56K_IRQ_MODA, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODB:  set_irq_line(DSP56K_IRQ_MODB, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODC:  set_irq_line(DSP56K_IRQ_MODC, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_RESET: set_irq_line(DSP56K_IRQ_RESET, info->i);			break;


		// !! It might be interesting to use this section as something which masks out the unecessary bits in each register !!

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + DSP56K_PC:			PC  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_SR:			SR  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_LC:			LC  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_LA:			LA  = info->i & 0xffff ;				break;
		case CPUINFO_INT_SP:																	// !!! I think this is correct !!!
		case CPUINFO_INT_REGISTER + DSP56K_SP:			SP  = info->i & 0xff ;					break;
		case CPUINFO_INT_REGISTER + DSP56K_OMR:			OMR = info->i & 0xff ;					break;

		case CPUINFO_INT_REGISTER + DSP56K_X:			X   = info->i & 0xffffffff ;			break;
		case CPUINFO_INT_REGISTER + DSP56K_Y:			Y   = info->i & 0xffffffff ;			break;

		case CPUINFO_INT_REGISTER + DSP56K_A:			A   = info->i & (UINT64)U64(0xffffffffffffffff) ;	break;	// could benefit from a better mask?
		case CPUINFO_INT_REGISTER + DSP56K_B:			B   = info->i & (UINT64)U64(0xffffffffffffffff) ;	break;	// could benefit from a better mask?

		case CPUINFO_INT_REGISTER + DSP56K_R0:			R0  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_R1:			R1  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_R2:			R2  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_R3:			R3  = info->i & 0xffff ;				break;

		case CPUINFO_INT_REGISTER + DSP56K_N0:			N0  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_N1:			N1  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_N2:			N2  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_N3:			N3  = info->i & 0xffff ;				break;

		case CPUINFO_INT_REGISTER + DSP56K_M0:			M0  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_M1:			M1  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_M2:			M2  = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_M3:			M3  = info->i & 0xffff ;				break;

		case CPUINFO_INT_REGISTER + DSP56K_TEMP:		TEMP   = info->i & 0xffff ;				break;
		case CPUINFO_INT_REGISTER + DSP56K_STATUS:		STATUS = info->i & 0xff   ;				break;

		// The CPU stack...
		case CPUINFO_INT_REGISTER + DSP56K_ST0:			ST0 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST1:			ST1 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST2:			ST2 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST3:			ST3 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST4:			ST4 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST5:			ST5 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST6:			ST6 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST7:			ST7 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST8:			ST8 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST9:			ST9 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST10:		ST10 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST11:		ST11 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST12:		ST12 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST13:		ST13 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST14:		ST14 = info->i & 0xffffffff ;			break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST15:		ST15 = info->i & 0xffffffff ;			break ;
	}
}


void dsp56k_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(dsp56k);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 4;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;	// ?
		case CPUINFO_INT_MAX_CYCLES:					info->i = 8;							break;	// ?

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = -1;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = -1;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;	// I think this is the ffc0-fff0 part of data memory?
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;	//
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;	//

		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODA:		info->i = LINE_MODA;				break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODB:		info->i = LINE_MODB;				break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_MODC:		info->i = LINE_MODC;				break;
		case CPUINFO_INT_INPUT_STATE + DSP56K_IRQ_RESET:	info->i = LINE_RESET;				break;  // Is reset a special case?

		case CPUINFO_INT_PREVIOUSPC:					info->i = dsp56k.ppc;					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + DSP56K_PC:			info->i = PC;							break;
		case CPUINFO_INT_REGISTER + DSP56K_SR:			info->i = SR;							break;
		case CPUINFO_INT_REGISTER + DSP56K_LC:			info->i = LC;							break;
		case CPUINFO_INT_REGISTER + DSP56K_LA:			info->i = LA;							break;
		case CPUINFO_INT_SP:																			// !!! I think this is correct !!!
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

		case CPUINFO_INT_REGISTER + DSP56K_TEMP:		info->i = TEMP;							break;
		case CPUINFO_INT_REGISTER + DSP56K_STATUS:		info->i = STATUS;						break;

		// The CPU stack
		case CPUINFO_INT_REGISTER + DSP56K_ST0:			info->i = ST0 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST1:			info->i = ST1 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST2:			info->i = ST2 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST3:			info->i = ST3 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST4:			info->i = ST4 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST5:			info->i = ST5 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST6:			info->i = ST6 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST7:			info->i = ST7 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST8:			info->i = ST8 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST9:			info->i = ST9 ;							break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST10:		info->i = ST10 ;						break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST11:		info->i = ST11 ;						break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST12:		info->i = ST12 ;						break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST13:		info->i = ST13 ;						break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST14:		info->i = ST14 ;						break ;
		case CPUINFO_INT_REGISTER + DSP56K_ST15:		info->i = ST15 ;						break ;



		// --- the following bits of info are returned as pointers to data or functions ---
		case CPUINFO_PTR_SET_INFO:						info->setinfo = dsp56k_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = dsp56k_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = dsp56k_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = dsp56k_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = dsp56k_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = dsp56k_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = dsp56k_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = dsp56k_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &dsp56k_icount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = dsp56k_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = dsp56k_win_layout;			break;


		// --- the following bits of info are returned as NULL-terminated strings ---
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "DSP56156"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola DSP56156"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "0.1"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Andrew Gardner"); break;


		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%s%s%s%s%s%s%s%s%s%s%s%s%s%s %s%s %s%s%s%s%s%s%s",
				lfFLAG ? "L":".",
				fvFLAG ? "F":".",
				s1FLAG ? "S":".",
				s0FLAG ? "S":".",
				i1FLAG ? "I":".",
				i0FLAG ? "I":".",
				sFLAG ?  "S":".",
				lFLAG ?  "L":".",
				eFLAG ?  "E":".",
				uFLAG ?  "U":".",
				nFLAG ?  "N":".",
				zFLAG ?  "Z":".",
				vFLAG ?  "V":".",
				cFLAG ?  "C":".",
				ufFLAG ? "U":".",
				seFLAG ? "S":".",
				cdFLAG ? "C":".",
				sdFLAG ? "S":".",
				rFLAG ?  "R":".",
				saFLAG ? "S":".",
				mcFLAG ? "M":".",
				mbFLAG ? "M":".",
				maFLAG ? "M":".");

            break;


		case CPUINFO_STR_REGISTER + DSP56K_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC : %04x", PC);  break;
		case CPUINFO_STR_REGISTER + DSP56K_SR:			sprintf(info->s = cpuintrf_temp_str(), "SR : %04x", SR);  break;
		case CPUINFO_STR_REGISTER + DSP56K_LC:			sprintf(info->s = cpuintrf_temp_str(), "LC : %04x", LC);  break;
		case CPUINFO_STR_REGISTER + DSP56K_LA:			sprintf(info->s = cpuintrf_temp_str(), "LA : %04x", LA);  break;
		case CPUINFO_STR_REGISTER + DSP56K_SP:			sprintf(info->s = cpuintrf_temp_str(), "SP : %02x", SP);  break;
		case CPUINFO_STR_REGISTER + DSP56K_OMR:			sprintf(info->s = cpuintrf_temp_str(), "OMR: %02x", OMR); break;

		case CPUINFO_STR_REGISTER + DSP56K_X:			sprintf(info->s = cpuintrf_temp_str(), "X  : %04x %04x", X1, X0);   break;
		case CPUINFO_STR_REGISTER + DSP56K_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y  : %04x %04x", Y1, Y0);   break;

		// !! This is silly - it gives me a warning if I try to print an unsigned long with %08x
		//    (and thus won't compile) - any suggestions?  Maybe we change it to a series of UINT16's or something?
		case CPUINFO_STR_REGISTER + DSP56K_A:			sprintf(info->s = cpuintrf_temp_str(), "A  : %02x %04x %04x", A2,A1,A0);   break;
		case CPUINFO_STR_REGISTER + DSP56K_B:			sprintf(info->s = cpuintrf_temp_str(), "B  : %02x %04x %04x", B2,B1,B0);   break;

		case CPUINFO_STR_REGISTER + DSP56K_R0:			sprintf(info->s = cpuintrf_temp_str(), "R0 : %04x", R0);  break;
		case CPUINFO_STR_REGISTER + DSP56K_R1:			sprintf(info->s = cpuintrf_temp_str(), "R1 : %04x", R1);  break;
		case CPUINFO_STR_REGISTER + DSP56K_R2:			sprintf(info->s = cpuintrf_temp_str(), "R2 : %04x", R2);  break;
		case CPUINFO_STR_REGISTER + DSP56K_R3:			sprintf(info->s = cpuintrf_temp_str(), "R3 : %04x", R3);  break;

		case CPUINFO_STR_REGISTER + DSP56K_N0:			sprintf(info->s = cpuintrf_temp_str(), "N0 : %04x", N0);  break;
		case CPUINFO_STR_REGISTER + DSP56K_N1:			sprintf(info->s = cpuintrf_temp_str(), "N1 : %04x", N1);  break;
		case CPUINFO_STR_REGISTER + DSP56K_N2:			sprintf(info->s = cpuintrf_temp_str(), "N2 : %04x", N2);  break;
		case CPUINFO_STR_REGISTER + DSP56K_N3:			sprintf(info->s = cpuintrf_temp_str(), "N3 : %04x", N3);  break;

		case CPUINFO_STR_REGISTER + DSP56K_M0:			sprintf(info->s = cpuintrf_temp_str(), "M0 : %04x", M0);  break;
		case CPUINFO_STR_REGISTER + DSP56K_M1:			sprintf(info->s = cpuintrf_temp_str(), "M1 : %04x", M1);  break;
		case CPUINFO_STR_REGISTER + DSP56K_M2:			sprintf(info->s = cpuintrf_temp_str(), "M2 : %04x", M2);  break;
		case CPUINFO_STR_REGISTER + DSP56K_M3:			sprintf(info->s = cpuintrf_temp_str(), "M3 : %04x", M3);  break;

		case CPUINFO_STR_REGISTER + DSP56K_TEMP:		sprintf(info->s = cpuintrf_temp_str(), "TMP: %04x", TEMP);   break;
		case CPUINFO_STR_REGISTER + DSP56K_STATUS:		sprintf(info->s = cpuintrf_temp_str(), "STS: %02x", STATUS); break;


		// The CPU stack
		case CPUINFO_STR_REGISTER + DSP56K_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0 : %08x", ST0);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1 : %08x", ST1);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2 : %08x", ST2);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3 : %08x", ST3);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4 : %08x", ST4);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5 : %08x", ST5);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6 : %08x", ST6);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7 : %08x", ST7);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST8:			sprintf(info->s = cpuintrf_temp_str(), "ST8 : %08x", ST8);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST9:			sprintf(info->s = cpuintrf_temp_str(), "ST9 : %08x", ST9);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST10:		sprintf(info->s = cpuintrf_temp_str(), "ST10: %08x", ST10);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST11:		sprintf(info->s = cpuintrf_temp_str(), "ST11: %08x", ST11);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST12:		sprintf(info->s = cpuintrf_temp_str(), "ST12: %08x", ST12);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST13:		sprintf(info->s = cpuintrf_temp_str(), "ST13: %08x", ST13);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST14:		sprintf(info->s = cpuintrf_temp_str(), "ST14: %08x", ST14);	 break ;
		case CPUINFO_STR_REGISTER + DSP56K_ST15:		sprintf(info->s = cpuintrf_temp_str(), "ST15: %08x", ST15);	 break ;
	}
}

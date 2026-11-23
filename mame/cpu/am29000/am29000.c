// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    am29000.c
    Core implementation of the Am29000 emulator

    Written by Philip Bennett

    Features missing:
    * MMU
    * Some instructions
    * Various exceptions

***************************************************************************/
/*
    Differences from MAME:

    Only one Am29000 per machine is supported (the state is static).

    The interrupt lines are sampled at the start of every timeslice, as in
    the original core: the driver has to keep the slices short around the
    events that raise them.

    The opcodes are read from the opcode base of the memory system, checking
    both bounds of the region.
*/

#include <stdio.h>
#include <string.h>
#include "driver.h"
#include "cpuintrf.h"
#include "state.h"
#include "debugger.h"
#include "am29000.h"
#include "am29dasm.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define PFLAG_FETCH_EN              (1 << 0)
#define PFLAG_DECODE_EN             (1 << 1)
#define PFLAG_EXECUTE_EN            (1 << 2)
#define PFLAG_WRITEBACK_EN          (1 << 3)
#define PFLAG_IRQ                   (1 << 4)
#define PFLAG_LOADSTORE             (1 << 5)
#define PFLAG_MULTI_LOADSTORE       (1 << 6)
#define PFLAG_JUMP                  (1 << 7)
#define PFLAG_JUMP2                 (1 << 8)
#define PFLAG_IRET                  (1 << 9)
#define PFLAG_TIMER_LOADED          (1 << 10)

#define PFLAG_RA_DEPENDENCY         (1 << 26)
#define PFLAG_RB_DEPENDENCY         (1 << 27)

#define PFLAG_MEM_MULTIPLE          (1 << 29)
#define PFLAG_REG_WRITEBACK         (1 << 30)
#define PFLAG_MEM_WRITEBACK         (1 << 31)

#define MMU_PROGRAM_ACCESS          (0)
#define MMU_DATA_ACCESS             (1)

#define FREEZE_MODE                 (a29k.cps & CPS_FZ)
#define SUPERVISOR_MODE             (a29k.cps & CPS_SM)
#define USER_MODE                   (~a29k.cps & CPS_SM)
#define REGISTER_IS_PROTECTED(x)    (a29k.rbp & (1 << ((x) >> 4)))

#define INST_RB_FIELD(x)            ((x) & 0xff)
#define INST_RA_FIELD(x)            (((x) >> 8) & 0xff)
#define INST_RC_FIELD(x)            (((x) >> 16) & 0xff)
#define INST_SA_FIELD(x)            (((x) >> 8) & 0xff)

#define FIELD_RA                    0
#define FIELD_RB                    1
#define FIELD_RC                    2

#define GET_ALU_FC                  ((a29k.alu >> ALU_FC_SHIFT) & ALU_FC_MASK)
#define GET_ALU_BP                  ((a29k.alu >> ALU_BP_SHIFT) & ALU_BP_MASK)
#define GET_CHC_CR                  ((a29k.chc >> CHC_CR_SHIFT) & CHC_CR_MASK)

#define SET_ALU_FC(x)               do { a29k.alu &= ~(ALU_FC_MASK << ALU_FC_SHIFT); a29k.alu |= ((x) & ALU_FC_MASK) << ALU_FC_SHIFT; } while(0)
#define SET_ALU_BP(x)               do { a29k.alu &= ~(ALU_BP_MASK << ALU_BP_SHIFT); a29k.alu |= ((x) & ALU_BP_MASK) << ALU_BP_SHIFT; } while(0)
#define SET_CHC_CR(x)               do { a29k.chc &= ~(CHC_CR_MASK << CHC_CR_SHIFT); a29k.chc |= ((x) & CHC_CR_MASK) << CHC_CR_SHIFT; } while(0)


/***************************************************************************
    STATE
***************************************************************************/

typedef struct
{
	UINT32          pc;

	/* General purpose */
	UINT32          r[256];     // TODO: There's only 192 implemented!

	/* TLB */
	UINT32          tlb[128];

	/* Protected SPRs */
	UINT32          vab;
	UINT32          ops;
	UINT32          cps;
	UINT32          cfg;
	UINT32          cha;
	UINT32          chd;
	UINT32          chc;
	UINT32          rbp;
	UINT32          tmc;
	UINT32          tmr;
	UINT32          pc0;
	UINT32          pc1;
	UINT32          pc2;
	UINT32          mmu;
	UINT32          lru;

	/* Unprotected SPRs */
	UINT32          ipc;
	UINT32          ipa;
	UINT32          ipb;
	UINT32          q;
	UINT32          alu;
	UINT32          fpe;
	UINT32          inte;
	UINT32          fps;

	/* Pipeline state */
	UINT32          exceptions;
	UINT32          exception_queue[4];

	UINT8           irq_active;
	UINT8           irq_lines;

	UINT32          exec_ir;
	UINT32          next_ir;

	UINT32          pl_flags;
	UINT32          next_pl_flags;

	UINT32          iret_pc;

	UINT32          exec_pc;
	UINT32          next_pc;
} am29000_state;

static am29000_state a29k;
static int am29000_icount;

typedef void (*opcode_func)(void);

typedef struct
{
	opcode_func opcode;
	UINT32 flags;
} op_info;


/***************************************************************************
    HELPER FUNCTIONS
***************************************************************************/

static void signal_exception(UINT32 type)
{
	a29k.exception_queue[a29k.exceptions++] = type;
}

static void external_irq_check(void)
{
	int mask = (a29k.cps >> CPS_IM_SHIFT) & CPS_IM_MASK;
	int irq_en = !(a29k.cps & CPS_DI) && !(a29k.cps & CPS_DA);
	int i;

	/* Clear interrupt pending bit to begin with */
	a29k.cps &= ~CPS_IP;

	for (i = 0; i < 4; ++i)
	{
		if (!(a29k.irq_active & (1 << i)) && (a29k.irq_lines & (1 << i)))
		{
			if (irq_en)
			{
				if (i <= mask)
				{
					a29k.irq_active |= (1 << i);
					signal_exception(EXCEPTION_INTR0 + i);
					a29k.pl_flags |= PFLAG_IRQ;
					return;
				}
			}
			/* Set interrupt pending bit if interrupt was disabled */
			a29k.cps |= CPS_IP;
		}
		else
			a29k.irq_active &= ~(1 << i);
	}
}

static UINT32 read_program_word(UINT32 address)
{
	/* TODO: ROM enable? */
	if (a29k.cps & CPS_PI || a29k.cps & CPS_RE)
	{
		/* both bounds are checked: a jump to a lower region (ROM to RAM)
		   would otherwise read outside the block */
		if (address < opcode_memory_min || address > opcode_memory_max)
			memory_set_opbase(address);
		return cpu_readop32_unsafe(address);
	}
	else
	{
		fatalerror("Am29000 instruction MMU translation enabled!\n");
	}
	return 0;
}

static UINT32 get_abs_reg(UINT8 r, UINT32 iptr)
{
	if (r & 0x80)
	{
		/* Stack pointer access */
		r = ((a29k.r[1] >> 2) & 0x7f) + (r & 0x7f);
		r |= 0x80;
	}
	else if (r == 0)
	{
		/* Indirect pointer access */
		r = (iptr >> IPX_SHIFT) & 0xff;
	}
	else if (r > 1 && r < 64)
	{
		fatalerror("Am29000: Undefined register access (%d)\n", r);
	}
	return r;
}

/***************************************************************************
    CORE INCLUDE
***************************************************************************/

/***************************************************************************
    DEFINES
***************************************************************************/

#define IFLAG_ILLEGAL               (1 << 0)
#define IFLAG_SUPERVISOR_ONLY       (1 << 1)
#define IFLAG_RA_PRESENT            (1 << 2)
#define IFLAG_RB_PRESENT            (1 << 3)
#define IFLAG_RC_PRESENT            (1 << 4)

#define IFLAG_SPR_ACCESS            (1 << 6)
#define IFLAG_MEMORY_ACCESS         (1 << 8)
#define IFLAG_CONTROL               (1 << 9)


#define GET_RA_VAL                  (a29k.r[RA])
#define GET_RB_VAL                  (a29k.r[RB])

#define RA                          (get_abs_reg(a29k.exec_ir >>  8, a29k.ipa))
#define RB                          (get_abs_reg(a29k.exec_ir >>  0, a29k.ipb))
#define RC                          (get_abs_reg(a29k.exec_ir >> 16, a29k.ipc))

#define INST_SA                     ((a29k.exec_ir >> 8) & 0xff)
#define INST_VN                     ((a29k.exec_ir >> 16) & 0xff)
#define INST_M_BIT                  (a29k.exec_ir & (1 << 24))
#define INST_CE_BIT                 (a29k.exec_ir & (1 << 23))
#define INST_AS_BIT                 (a29k.exec_ir & (1 << 22))
#define INST_PA_BIT                 (a29k.exec_ir & (1 << 21))
#define INST_SB_BIT                 (a29k.exec_ir & (1 << 20))
#define INST_UA_BIT                 (a29k.exec_ir & (1 << 19))
#define INST_OPT_MASK               (7)
#define INST_OPT_SHIFT              (16)
#define INST_OPT_FIELD              (((a29k.exec_ir) >> INST_OPT_SHIFT) & INST_OPT_MASK)
#define INST_CNTL_MASK              (0x7f)
#define INST_CNTL_SHIFT             (16)

#define I8                          (a29k.exec_ir & 0xff)
#define I16                         (((a29k.exec_ir >> 8) & 0xff00) | (a29k.exec_ir & 0xff))
#define I16_ZEX                     ((UINT32)(I16))
#define I16_SEX                     ((INT32)(INT16)I16)
#define I16_OEX                     (0xffff0000 | I16)

#define JMP_ZEX                     (I16 << 2)
#define JMP_SEX                     ((INT32)(INT16)(((a29k.exec_ir >> 8) & 0xff00) | (a29k.exec_ir & 0xff)) << 2)

#define BOOLEAN_MASK                (1 << 31)
#define BOOLEAN_TRUE                (1 << 31)
#define BOOLEAN_FALSE               (0)

#define UNHANDLED_OP                fatalerror("Am29000: Unhandled inst %s at %x\n", __FUNCTION__, a29k.exec_pc);


/***************************************************************************
    ALU FLAG CALCULATION
***************************************************************************/

#define SET_ALU_Z(r)            a29k.alu &= ~ALU_Z; \
								a29k.alu |= (r == 0) << ALU_Z_SHIFT;

#define SET_ALU_N(r)            a29k.alu &= ~ALU_N; \
								a29k.alu |= ((UINT32)r & 0x80000000) >> (31 - ALU_N_SHIFT);

#define CALC_C_ADD(r, a)        ((UINT32)(r) < (UINT32)(a))

#define SET_ALU_C_ADD(r, a)     a29k.alu &= ~ALU_C; \
								a29k.alu |= CALC_C_ADD(r, a) << ALU_C_SHIFT;

#define CALC_C_SUB(a, b)        (!((UINT32)(a) < (UINT32)(b)))

#define SET_ALU_C_SUB(a, b)     a29k.alu &= ~ALU_C; \
								a29k.alu |= CALC_C_SUB(a, b) << ALU_C_SHIFT;

#define SET_ALU_V_ADD(r, a, b)  a29k.alu &= ~ALU_V; \
								a29k.alu |= (((INT32)(~((a) ^ (b)) & ((a) ^ (r))) < 0)) << ALU_V_SHIFT;

#define SET_ALU_V_SUB(r, a, b)  a29k.alu &= ~ALU_V; \
								a29k.alu |= ((INT32)(((a) ^ (b)) & ((a) ^ (r))) < 0) << ALU_V_SHIFT;

#define GET_CARRY               ((a29k.alu >> ALU_C_SHIFT) & 1)





static UINT32 read_spr(UINT32 idx)
{
	UINT32 val = 0;

	switch (idx)
	{
		case SPR_VAB:   val = a29k.vab;     break;
		case SPR_OPS:   val = a29k.ops;     break;
		case SPR_CPS:   val = a29k.cps;     break;
		case SPR_CFG:   val = a29k.cfg;     break;
		case SPR_CHA:   val = a29k.cha;     break;
		case SPR_CHD:   val = a29k.chd;     break;
		case SPR_CHC:   val = a29k.chc;     break;
		case SPR_RBP:   val = a29k.rbp;     break;
		case SPR_TMC:   val = a29k.tmc;     break;
		case SPR_TMR:   val = a29k.tmr;     break;
		case SPR_PC0:   val = a29k.pc0;     break;
		case SPR_PC1:   val = a29k.pc1;     break;
		case SPR_PC2:   val = a29k.pc2;     break;
		case SPR_MMU:   val = a29k.mmu;     break;
		case SPR_LRU:   val = a29k.lru;     break;
		case SPR_IPC:   val = a29k.ipc;     break;
		case SPR_IPA:   val = a29k.ipa;     break;
		case SPR_IPB:   val = a29k.ipb;     break;
		case SPR_Q:     val = a29k.q;       break;
		case SPR_ALU:   val = a29k.alu;     break;
		case SPR_BP:    val = GET_ALU_BP;       break;
		case SPR_FC:    val = GET_ALU_FC;       break;
		case SPR_CR:    val = GET_CHC_CR;       break;
		case SPR_FPE:   val = a29k.fpe;     break;
		case SPR_INTE:  val = a29k.inte;    break;
		case SPR_FPS:   val = a29k.fps;     break;
		default:
			logerror("Unknown SPR read (%d)\n", idx);
	}

	return val;
}


static void write_spr(UINT32 idx, UINT32 val)
{
	switch (idx)
	{
		case SPR_VAB:   a29k.vab = val & (VAB_MASK << VAB_SHIFT);
						break;
		case SPR_OPS:   a29k.ops = val & (CPS_CA | CPS_IP | CPS_TE | CPS_TP | CPS_TU | CPS_FZ | CPS_LK | CPS_RE |
						CPS_WM | CPS_PD | CPS_PI | CPS_SM | (CPS_IM_MASK << CPS_IM_SHIFT) | CPS_DI | CPS_DA);
						break;
		case SPR_CPS:   a29k.cps = val & (CPS_CA | CPS_IP | CPS_TE | CPS_TP | CPS_TU | CPS_FZ | CPS_LK | CPS_RE |
						CPS_WM | CPS_PD | CPS_PI | CPS_SM | (CPS_IM_MASK << CPS_IM_SHIFT) | CPS_DI | CPS_DA);
						break;
		case SPR_CFG:   a29k.cfg = val & (CFG_DW | CFG_VF | CFG_RV | CFG_BO | CFG_CP | CFG_CD);
						a29k.cfg |= PROCESSOR_REL_FIELD << CFG_PRL_SHIFT;
						break;
		case SPR_CHA:   a29k.cha = val;
						break;
		case SPR_CHD:   a29k.chd = val;
						break;
		case SPR_CHC:   a29k.chc = val;
						break;
		case SPR_RBP:   a29k.rbp = val & RBP_MASK;
						break;
		case SPR_TMC:   a29k.tmc = val & TCV_MASK;
						break;
		case SPR_TMR:   a29k.tmr = val & (TMR_OV | TMR_IN | TMR_IE | TMR_TRV_MASK);
						break;
		case SPR_PC0:   a29k.pc0 = val & PC_MASK;
						break;
		case SPR_PC1:   a29k.pc1 = val & PC_MASK;
						break;
		case SPR_PC2:   a29k.pc2 = val & PC_MASK;
						break;
		case SPR_MMU:   a29k.mmu = val & ((MMU_PS_MASK << MMU_PS_SHIFT) | MMU_PID_MASK);
						break;
		case SPR_LRU:   a29k.lru = val & (LRU_MASK << LRU_SHIFT);
						break;
		case SPR_IPC:   a29k.ipc = val;// & IPX_MASK;
						break;
		case SPR_IPA:   a29k.ipa = val;// & IPX_MASK;
						break;
		case SPR_IPB:   a29k.ipb = val;// & IPX_MASK;
						break;
		case SPR_Q:     a29k.q = val;
						break;
		case SPR_ALU:   a29k.alu = val & (ALU_DF | ALU_V | ALU_N | ALU_Z | ALU_C | (ALU_BP_MASK << ALU_BP_SHIFT) | (ALU_FC_MASK << ALU_FC_SHIFT));
						break;
		case SPR_BP:    a29k.alu &= ~(ALU_BP_MASK << ALU_BP_SHIFT);
						a29k.alu |= (val & ALU_BP_MASK) << ALU_BP_SHIFT;
						break;
		case SPR_FC:    a29k.alu &= ~(ALU_FC_MASK << ALU_FC_SHIFT);
						a29k.alu |= (val & ALU_FC_MASK) << ALU_FC_SHIFT;
						break;
		case SPR_CR:    a29k.chc &= ~(CHC_CR_MASK << CHC_CR_SHIFT);
						a29k.chc |= (val & CHC_CR_MASK) << CHC_CR_SHIFT;
						break;
//      case SPR_FPE:
//      case SPR_INTE:
//      case SPR_FPS:
		default: logerror("Unhandled SPR write (%d)\n", idx);
	}
}


/***************************************************************************
    INSTRUCTIONS
***************************************************************************/

/***************************************************************************
    INTEGER ARITHMETIC
***************************************************************************/

static void ADD(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a + b;

	if (!FREEZE_MODE)
	{
		SET_ALU_V_ADD(r, a, b);
		SET_ALU_Z(r);
		SET_ALU_N(r);
		SET_ALU_C_ADD(r, a);
	}

	a29k.r[RC] = r;
}

static void ADDS(void)
{
	UNHANDLED_OP;
}

static void ADDU(void)
{
	UNHANDLED_OP;
}

static void ADDC(void)
{
	UNHANDLED_OP;
}

static void ADDCS(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a + b + GET_CARRY;

	if (!FREEZE_MODE)
	{
		UINT32 carry = GET_CARRY;
		UINT32 tmp = a + b;

		SET_ALU_V_ADD(r, a, b);
		SET_ALU_Z(r);
		SET_ALU_N(r);

		carry = CALC_C_ADD(tmp, a) || CALC_C_ADD(tmp + carry, carry);
		a29k.alu &= ~ALU_C;
		a29k.alu |= carry << ALU_C_SHIFT;
	}

	// TODO: Trap on signed overflow
	a29k.r[RC] = r;
}

static void ADDCU(void)
{
	UNHANDLED_OP;
}

static void SUB(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r = a - b;

	if (!FREEZE_MODE)
	{
		SET_ALU_V_SUB(r, a, b);
		SET_ALU_Z(r);
		SET_ALU_N(r);
		SET_ALU_C_SUB(a, b);
	}

	// TODO: Trap on unsigned overflow
	a29k.r[RC] = r;
}

static void SUBS(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r = a - b;

	if (!FREEZE_MODE)
	{
		SET_ALU_V_SUB(r, a, b);
		SET_ALU_Z(r);
		SET_ALU_N(r);
		SET_ALU_C_SUB(a, b);
	}

	if ((INT32)(((a) ^ (b)) & ((a) ^ (r))) < 0)
		signal_exception(EXCEPTION_OUT_OF_RANGE);

	a29k.r[RC] = r;
}

static void SUBU(void)
{
	UNHANDLED_OP;
}

static void SUBC(void)
{
	UNHANDLED_OP;
}

static void SUBCS(void)
{
	UNHANDLED_OP;
}

static void SUBCU(void)
{
	UNHANDLED_OP;
}

static void SUBR(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r = b - a;

	if (!FREEZE_MODE)
	{
		SET_ALU_V_SUB(r, a, b);
		SET_ALU_Z(r);
		SET_ALU_N(r);
		SET_ALU_C_SUB(a, b);
	}

	a29k.r[RC] = r;
}

static void SUBRS(void)
{
	UNHANDLED_OP;
}

static void SUBRU(void)
{
	UNHANDLED_OP;
}

static void SUBRC(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r = b - a - 1 + GET_CARRY;

	if (!FREEZE_MODE)
	{
		SET_ALU_V_SUB(r, a, b);
		SET_ALU_Z(r);
		SET_ALU_N(r);
		SET_ALU_C_SUB(a, b);
	}

	a29k.r[RC] = r;
}

static void SUBRCS(void)
{
	UNHANDLED_OP;
}

static void SUBRCU(void)
{
	UNHANDLED_OP;
}

static void MULTIPLU(void)
{
	UNHANDLED_OP;
}

static void MULTIPLY(void)
{
	UNHANDLED_OP;
}

static void MUL(void)
{
	/* TODO: Zero/Neg flags ? */
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r;
	UINT32 sign;

	if (a29k.q & 1)
	{
		r = a + b;
		sign = (r >> 31) ^ (((INT32)(~((a) ^ (b)) & ((a) ^ (r))) < 0));
	}
	else
	{
		r = b;
		sign = b >> 31;
	}

	// Shift combined value right one bit position, entering sign into bit 31 of destination
	a29k.r[RC] = (r >> 1) | (sign << 31);
	a29k.q = (a29k.q >> 1) | (r << 31);
}

static void MULL(void)
{
	/* TODO: Zero/Neg flags ? */
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r;
	UINT32 sign;

	if (a29k.q & 1)
	{
		r = b - a;
		sign = (r >> 31) ^ ((INT32)(((a) ^ (b)) & ((a) ^ (r))) < 0);
	}
	else
	{
		r = b;
		sign = b >> 31;
	}

	// Shift combined value right one bit position, entering sign into bit 31 of destination
	a29k.r[RC] = (r >> 1) | (sign << 31);
	a29k.q = (a29k.q >> 1) | (r << 31);
}

static void MULU(void)
{
	/* TODO: Zero/Neg flags ? */
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r;
	UINT32 c;

	if (a29k.q & 1)
	{
		r = a + b;
		c = (UINT32)(r) < (UINT32)(a);
	}
	else
	{
		r = b;
		c = 0;
	}

	// Shift combined value right one bit position, entering carry into bit 31 of destination
	a29k.r[RC] = (r >> 1) | (c << 31);
	a29k.q = (a29k.q >> 1) | (r << 31);
}

static void DIVIDE(void)
{
	a29k.ipa = RA << IPX_SHIFT;
	a29k.ipb = RB << IPX_SHIFT;
	a29k.ipc = RC << IPX_SHIFT;

	signal_exception(EXCEPTION_DIVIDE);
}

static void DIVIDU(void)
{
	UNHANDLED_OP;
}

static void DIV0(void)
{
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;

	if (!FREEZE_MODE)
	{
		a29k.alu |= ALU_DF;
		SET_ALU_N(b);
	}

	// Shift combined value left one bit position, zeroing bit 0 of Q
	a29k.r[RC] = (b << 1) | (a29k.q >> 31);
	a29k.q <<= 1;
}

static void DIV(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 c;
	UINT32 r;
	UINT32 df;

	if (a29k.alu & ALU_DF)
	{
		r = a - b;
		c = !((UINT32)(a) < (UINT32)(b));
	}
	else
	{
		r = a + b;
		c = (UINT32)(r) < (UINT32)(a);
	}


	df = (~(c ^ (a29k.alu >> ALU_DF_SHIFT) ^ (a29k.alu >> ALU_N_SHIFT)) & 1);

	if (!FREEZE_MODE)
	{
		a29k.alu &= ~ALU_DF;
		a29k.alu |= df << ALU_DF_SHIFT;
		SET_ALU_N(r);
	}

	// Shift combined value left one bit position, entering DF into bit 0 of Q
	a29k.r[RC] = (r << 1) | (a29k.q >> 31);
	a29k.q = (a29k.q << 1) | df;
}

static void DIVL(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 c;
	UINT32 r;
	UINT32 df;

	if (a29k.alu & ALU_DF)
	{
		r = a - b;
		c = !((UINT32)(a) < (UINT32)(b));
	}
	else
	{
		r = a + b;
		c = (UINT32)(r) < (UINT32)(a);
	}

	df = (~(c ^ (a29k.alu >> ALU_DF_SHIFT) ^ (a29k.alu >> ALU_N_SHIFT)) & 1);

	if (!FREEZE_MODE)
	{
		a29k.alu &= ~ALU_DF;
		a29k.alu |= df << ALU_DF_SHIFT;
		SET_ALU_N(r);
	}

	a29k.q = (a29k.q << 1) | df;
	a29k.r[RC] = r;
}

static void DIVREM(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;

	if (a29k.alu & ALU_DF)
		a29k.r[RC] = a;
	else
		a29k.r[RC] = a + b;
}


/***************************************************************************
    COMPARE
***************************************************************************/

static void CPEQ(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a == b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPNEQ(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a != b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPLT(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (INT32)a < (INT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPLTU(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (UINT32)a < (UINT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPLE(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (INT32)a <= (INT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPLEU(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (UINT32)a <= (UINT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPGT(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (INT32)a > (INT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPGTU(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (UINT32)a > (UINT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPGE(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (INT32)a >= (INT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPGEU(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = (UINT32)a >= (UINT32)b ? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void CPBYTE(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8 : GET_RB_VAL;
	UINT32 r =
			((a & 0xff000000) == (b & 0xff000000)) ||
			((a & 0x00ff0000) == (b & 0x00ff0000)) ||
			((a & 0x0000ff00) == (b & 0x0000ff00)) ||
			((a & 0x000000ff) == (b & 0x000000ff))
			? BOOLEAN_TRUE : BOOLEAN_FALSE;

	a29k.r[RC] = r;
}

static void ASEQ(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!(GET_RA_VAL == GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASNEQ(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!(GET_RA_VAL != GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASLT(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((INT32)GET_RA_VAL < (INT32)GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASLTU(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((UINT32)GET_RA_VAL < (UINT32)GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASLE(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((INT32)GET_RA_VAL <= (INT32)GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASLEU(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((UINT32)GET_RA_VAL <= (UINT32)GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASGT(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((INT32)GET_RA_VAL > (INT32)GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASGTU(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((UINT32)GET_RA_VAL > (UINT32)GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASGE(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((INT32)GET_RA_VAL >= (INT32)GET_RB_VAL))
		signal_exception(INST_VN);
}

static void ASGEU(void)
{
	if (USER_MODE && INST_VN < 64)
		signal_exception(EXCEPTION_PROTECTION_VIOLATION);
	else if (!((UINT32)GET_RA_VAL >= (UINT32)GET_RB_VAL))
		signal_exception(INST_VN);
}


/***************************************************************************
    LOGICAL
***************************************************************************/

static void AND(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a & b;

	if (!FREEZE_MODE)
	{
		SET_ALU_Z(r);
		SET_ALU_N(r);
	}

	a29k.r[RC] = r;
}

static void ANDN(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a & ~b;

	if (!FREEZE_MODE)
	{
		SET_ALU_Z(r);
		SET_ALU_N(r);
	}

	a29k.r[RC] = r;
}

static void NAND(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = ~(a & b);

	if (!FREEZE_MODE)
	{
		SET_ALU_Z(r);
		SET_ALU_N(r);
	}

	a29k.r[RC] = r;
}

static void OR(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a | b;

	if (!FREEZE_MODE)
	{
		SET_ALU_Z(r);
		SET_ALU_N(r);
	}

	a29k.r[RC] = r;
}

static void NOR(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = ~(a | b);

	if (!FREEZE_MODE)
	{
		SET_ALU_Z(r);
		SET_ALU_N(r);
	}

	a29k.r[RC] = r;
}

static void XOR(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = a ^ b;

	if (!FREEZE_MODE)
	{
		SET_ALU_Z(r);
		SET_ALU_N(r);
	}

	a29k.r[RC] = r;
}

static void XNOR(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r = ~(a ^ b);

	if (!FREEZE_MODE)
	{
		SET_ALU_Z(r);
		SET_ALU_N(r);
	}

	a29k.r[RC] = r;
}


/***************************************************************************
    SHIFT
***************************************************************************/

static void SLL(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = (INST_M_BIT ? I8: GET_RB_VAL) & 0x1f;
	UINT32 r = a << b;

	a29k.r[RC] = r;
}

static void SRL(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = (INST_M_BIT ? I8: GET_RB_VAL) & 0x1f;
	UINT32 r = a >> b;

	a29k.r[RC] = r;
}

static void SRA(void)
{
	INT32 a = GET_RA_VAL;
	UINT32 b = (INST_M_BIT ? I8: GET_RB_VAL) & 0x1f;
	UINT32 r = a >> b;

	a29k.r[RC] = r;
}

static void EXTRACT(void)
{
	INT32 a = GET_RA_VAL;
	UINT32 b = (INST_M_BIT ? I8: GET_RB_VAL);
	UINT64 r;

	r = (((UINT64)a << 32) | b) << GET_ALU_FC;

	a29k.r[RC] = r >> 32;
}


/***************************************************************************
    DATA MOVEMENT
***************************************************************************/

static void LOAD(void)
{
	UINT32 addr = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 r;

	if (INST_UA_BIT)
		fatalerror("Am29000: UA bit set on LOAD\n");

	if (INST_CE_BIT)
	{
		logerror("Am29000: Attempting a co-processor LOAD!\n");
		r = 0;
	}
	else
	{
		if (!INST_PA_BIT && !(a29k.cps & CPS_PD))
		{
			fatalerror("Am29000: Address translation on LOAD\n");
		}
		else
		{
			if (USER_MODE)
			{
				signal_exception(EXCEPTION_PROTECTION_VIOLATION);
				return;
			}

			r = data_read_dword_32be(addr);
		}
	}

//  if (opt & 2)
//      logerror("Am29000: Half word LOAD\n");

	if (!FREEZE_MODE)
	{
		a29k.chc = ((a29k.exec_ir << 8) & 0xff) | CHC_LS | RA << CHC_TR_SHIFT | CHC_CV;
		a29k.cha = addr;
		a29k.chd = r;

		if (!(a29k.cfg & CFG_DW) && INST_SB_BIT)
		{
			SET_ALU_BP(addr);
		}
	}

	a29k.r[RA] = r;

	if (a29k.cfg & CFG_DW)
	{
		logerror("DW ON A STORE");
	}
}

static void LOADL(void)
{
	UNHANDLED_OP;
}

static void LOADSET(void)
{
	UNHANDLED_OP;
}

static void LOADM(void)
{
	UINT32 addr = INST_M_BIT ? I8: GET_RB_VAL;

	if (INST_UA_BIT)
	{
		fatalerror("Am29000: UA bit set on LOAD\n");
	}

	UINT32 r;
	if (INST_CE_BIT)
	{
		logerror("Am29000: Attempting a co-processor LOAD!\n");
		r = 0;
	}
	else
	{
		if (!INST_PA_BIT && !(a29k.cps & CPS_PD))
		{
			fatalerror("Am29000: Address translation on LOAD\n");
		}
		else
		{
			if (USER_MODE)
			{
				signal_exception(EXCEPTION_PROTECTION_VIOLATION);
				return;
			}

			r = data_read_dword_32be(addr);
		}
	}

	if (!FREEZE_MODE)
	{
		// TODO
		a29k.chc &= (CHC_CR_MASK << CHC_CR_SHIFT);
		a29k.chc |= ((a29k.exec_ir << 8) & 0xff) | RA << CHC_TR_SHIFT | CHC_CV;
		a29k.cha = addr;
		a29k.chd = r; // ?????

		if (!(a29k.cfg & CFG_DW) && INST_SB_BIT)
		{
			SET_ALU_BP(addr);
		}
	}

	r = RA;

	for (INT32 cnt = 0; cnt <= GET_CHC_CR; ++cnt)
	{
		a29k.r[r] = data_read_dword_32be(addr);

//          SET_CHC_CR(cnt - 1);
		addr += 4;

		if (++r == 256)
		{
			r = 128;
		}
	}
}

static void STORE(void)
{
	UINT32 addr = INST_M_BIT ? I8: GET_RB_VAL;

	if (INST_UA_BIT)
	{
		fatalerror("Am29000: UA bit set on LOAD\n");
	}

	if (INST_CE_BIT)
	{
		logerror("Am29000: Attempting a co-processor LOAD!\n");
	}
	else
	{
		if (!INST_PA_BIT && !(a29k.cps & CPS_PD))
		{
			fatalerror("Am29000: Address translation on LOAD\n");
		}
		else
		{
			if (USER_MODE)
			{
				signal_exception(EXCEPTION_PROTECTION_VIOLATION);
				return;
			}
		}
	}

	data_write_dword_32be(addr, a29k.r[RA]);

	if (!FREEZE_MODE)
	{
		a29k.chc = ((a29k.exec_ir << 8) & 0xff) | RA << CHC_TR_SHIFT | CHC_CV;
		a29k.cha = addr;

		if (!(a29k.cfg & CFG_DW) && INST_SB_BIT)
		{
			SET_ALU_BP(addr);
		}
	}

	if (a29k.cfg & CFG_DW)
	{
		logerror("DW ON A STORE");
	}
}

static void STOREL(void)
{
	UNHANDLED_OP;
}

static void STOREM(void)
{
	UINT32 addr = INST_M_BIT ? I8: GET_RB_VAL;

	if (INST_UA_BIT)
	{
		fatalerror("Am29000: UA bit set on LOAD\n");
	}

	UINT32 r;
	if (INST_CE_BIT)
	{
		logerror("Am29000: Attempting a co-processor LOAD!\n");
		r = 0;
	}
	else
	{
		if (!INST_PA_BIT && !(a29k.cps & CPS_PD))
		{
			fatalerror("Am29000: Address translation on LOAD\n");
		}
		else
		{
			if (USER_MODE)
			{
				signal_exception(EXCEPTION_PROTECTION_VIOLATION);
				return;
			}
		}
	}

	if (!FREEZE_MODE)
	{
		// TODO
		a29k.chc &= (CHC_CR_MASK << CHC_CR_SHIFT);
		a29k.chc |= ((a29k.exec_ir << 8) & 0xff) | RA << CHC_TR_SHIFT | CHC_CV;
		a29k.cha = addr;

		if (!(a29k.cfg & CFG_DW) && INST_SB_BIT)
		{
			SET_ALU_BP(addr);
		}
	}

	r = RA;

	for (INT32 cnt = 0; cnt <= GET_CHC_CR; ++cnt)
	{
		data_write_dword_32be(addr, a29k.r[r]);

		addr += 4;

		if (++r == 256)
		{
			r = 128;
		}
	}
}

static void EXBYTE(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 bp = GET_ALU_BP;
	UINT8 srcbyte;
	UINT32 r;

	if (a29k.cfg & CFG_BO)
		srcbyte = a >> 8 * bp;
	else
		srcbyte = a >> (8 * (3 - bp));

	r = (b & 0xffffff00) | srcbyte;

	a29k.r[RC] = r;
}

static void EXHW(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 wp = ((a29k.alu >> ALU_BP_SHIFT) & ALU_BP_MASK) >> 1;
	UINT16 srcword;
	UINT32 r;

	if (a29k.cfg & CFG_BO)
		srcword = a >> 16 * wp;
	else
		srcword = a >> (16 * (1 - wp));

	r = (b & 0xffff0000) | srcword;

	a29k.r[RC] = r;
}

static void EXHWS(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 wp = ((a29k.alu >> ALU_BP_SHIFT) & ALU_BP_MASK) >> 1;
	UINT16 srcword;
	UINT32 r;

	if (a29k.cfg & CFG_BO)
		srcword = a >> 16 * wp;
	else
		srcword = a >> (16 * (1 - wp));

	r = (INT32)(INT16)srcword;

	a29k.r[RC] = r;
}

static void INBYTE(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 bp = GET_ALU_BP;
	UINT8 shift = (a29k.cfg & CFG_BO) ? 8 * bp : (8 * (3 - bp));
	UINT32 r;

	r = (a & ~(0xff << shift)) | ((b & 0xff) << shift);

	a29k.r[RC] = r;
}

static void INHW(void)
{
	UINT32 a = GET_RA_VAL;
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;
	UINT32 wp = ((a29k.alu >> ALU_BP_SHIFT) & ALU_BP_MASK) >> 1;
	UINT32 shift = (a29k.cfg & CFG_BO) ? 16 * wp : (16 * (1 - wp));
	UINT32 r;

	r = (a & ~(0xffff << shift)) | ((b & 0xffff) << shift);

	a29k.r[RC] = r;
}

static void MFSR(void)
{
	a29k.r[RC] = read_spr(INST_SA);
}

static void MFTLB(void)
{
	a29k.r[RC] = a29k.tlb[GET_RA_VAL & 0x7f];
}

static void MTSR(void)
{
	write_spr(INST_SA, GET_RB_VAL);
}

static void MTSRIM(void)
{
	write_spr(INST_SA, I16_ZEX);
}

static void MTTLB(void)
{
	a29k.tlb[GET_RA_VAL & 0x7f] = GET_RB_VAL;
}


/***************************************************************************
    CONSTANT
***************************************************************************/

static void CONST(void)
{
	a29k.r[RA] = I16_ZEX;
}

static void CONSTH(void)
{
	a29k.r[RA] = (I16 << 16) | GET_RA_VAL;
}

static void CONSTN(void)
{
	a29k.r[RA] = I16_OEX;
}


/***************************************************************************
    BRANCH INSTRUCTIONS
***************************************************************************/

static void CALL(void)
{
	UINT32 ret = a29k.next_pc;

	if (INST_M_BIT)
		a29k.next_pc = JMP_ZEX;
	else
		a29k.next_pc = a29k.exec_pc + JMP_SEX;

	a29k.r[RA] = ret;
	a29k.next_pl_flags |= PFLAG_JUMP;
}

static void CALLI(void)
{
	UINT32 ret = a29k.next_pc;
	a29k.next_pc = GET_RB_VAL;
	a29k.r[RA] = ret;
	a29k.next_pl_flags |= PFLAG_JUMP;
}

static void JMP(void)
{
	if (INST_M_BIT)
		a29k.next_pc = JMP_ZEX;
	else
		a29k.next_pc = a29k.exec_pc + JMP_SEX;

	a29k.next_pl_flags |= PFLAG_JUMP;
}

static void JMPI(void)
{
	a29k.next_pc = GET_RB_VAL;

	a29k.next_pl_flags |= PFLAG_JUMP;
}

static void JMPT(void)
{
	if ((GET_RA_VAL & BOOLEAN_MASK) == BOOLEAN_TRUE)
	{
		if (INST_M_BIT)
			a29k.next_pc = JMP_ZEX;
		else
			a29k.next_pc = a29k.exec_pc + JMP_SEX;

		a29k.next_pl_flags |= PFLAG_JUMP;
	}
}

static void JMPTI(void)
{
	if ((GET_RA_VAL & BOOLEAN_MASK) == BOOLEAN_TRUE)
	{
		a29k.next_pc = GET_RB_VAL;
		a29k.next_pl_flags |= PFLAG_JUMP;
	}
}

static void JMPF(void)
{
	if ((GET_RA_VAL & BOOLEAN_MASK) == BOOLEAN_FALSE)
	{
		if (INST_M_BIT)
			a29k.next_pc = JMP_ZEX;
		else
			a29k.next_pc = a29k.exec_pc + JMP_SEX;

		a29k.next_pl_flags |= PFLAG_JUMP;
	}
}

static void JMPFI(void)
{
	if ((GET_RA_VAL & BOOLEAN_MASK) == BOOLEAN_FALSE)
	{
		a29k.next_pc = GET_RB_VAL;
		a29k.next_pl_flags |= PFLAG_JUMP;
	}
}

static void JMPFDEC(void)
{
	UINT32 a = GET_RA_VAL;

	if ((a & BOOLEAN_MASK) == BOOLEAN_FALSE)
	{
		if (INST_M_BIT)
			a29k.next_pc = JMP_ZEX;
		else
			a29k.next_pc = a29k.exec_pc + JMP_SEX;

		a29k.next_pl_flags |= PFLAG_JUMP;
	}

	a29k.r[RA] = a - 1;
}


/***************************************************************************
    MISCELLANEOUS INSTRUCTIONS
***************************************************************************/

static void CLZ(void)
{
	UINT32 b = INST_M_BIT ? I8: GET_RB_VAL;

	a29k.r[RC] = count_leading_zeros(b);
}

static void SETIP(void)
{
	a29k.ipa = RA << IPX_SHIFT;
	a29k.ipb = RB << IPX_SHIFT;
	a29k.ipc = RC << IPX_SHIFT;
}

static void EMULATE(void)
{
	UNHANDLED_OP;
}

static void INV(void)
{
	/* Nothing to do yet */
}

static void IRET(void)
{
	a29k.iret_pc = a29k.pc0;
	a29k.next_pc = a29k.pc1;
	a29k.cps = a29k.ops;
	a29k.next_pl_flags = PFLAG_IRET;
}

static void IRETINV(void)
{
	UNHANDLED_OP;
}

static void HALT(void)
{
	UNHANDLED_OP;
}


static void ILLEGAL(void)
{
	fatalerror("Am29000: Executed illegal instruction - this should never happen! %x (%x)\n", a29k.pc2, a29k.exec_pc);
}



/***************************************************************************
    UNHANDLED
***************************************************************************/

static void CONVERT(void)
{
	UNHANDLED_OP;
}

static void SQRT(void)
{
	UNHANDLED_OP;
}

static void CLASS(void)
{
	UNHANDLED_OP;
}

static void MULTM(void)
{
	UNHANDLED_OP;
}

static void MULTMU(void)
{
	UNHANDLED_OP;
}



static const op_info op_table[256] =
{
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ CONSTN,   IFLAG_RA_PRESENT                                                   },
	{ CONSTH,   IFLAG_RA_PRESENT                                                   },
	{ CONST,    IFLAG_RA_PRESENT                                                   },
	{ MTSRIM,   0                                                                  },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ LOADL,    0                                                                  },
	{ LOADL,    0                                                                  },
	{ CLZ,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT                                },
	{ CLZ,      IFLAG_RC_PRESENT                                                   },
	{ EXBYTE,   IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ EXBYTE,   IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ INBYTE,   IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ INBYTE,   IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ STOREL,   0                                                                  },
	{ STOREL,   0                                                                  },
	{ ADDS,     0                                                                  },
	{ ADDS,     0                                                                  },
	{ ADDU,     0                                                                  },
	{ ADDU,     0                                                                  },
	{ ADD,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ ADD,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ LOAD,     IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ LOAD,     IFLAG_RA_PRESENT                                                   },
	{ ADDCS,    IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ ADDCS,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ ADDCU,    0                                                                  },
	{ ADDCU,    0                                                                  },
	{ ADDC,     0                                                                  },
	{ ADDC,     0                                                                  },
	{ STORE,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ STORE,    IFLAG_RA_PRESENT                                                   },
	{ SUBS,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ SUBS,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ SUBU,     0                                                                  },
	{ SUBU,     0                                                                  },
	{ SUB,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ SUB,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ LOADSET,  0                                                                  },
	{ LOADSET,  0                                                                  },
	{ SUBCS,    0                                                                  },
	{ SUBCS,    0                                                                  },
	{ SUBCU,    0                                                                  },
	{ SUBCU,    0                                                                  },
	{ SUBC,     0                                                                  },
	{ SUBC,     0                                                                  },
	{ CPBYTE,   IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPBYTE,   IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ SUBRS,    0                                                                  },
	{ SUBRS,    0                                                                  },
	{ SUBRU,    0                                                                  },
	{ SUBRU,    0                                                                  },
	{ SUBR,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ SUBR,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ LOADM,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ LOADM,    IFLAG_RA_PRESENT                                                   },
	{ SUBRCS,   0                                                                  },
	{ SUBRCS,   0                                                                  },
	{ SUBRCU,   0                                                                  },
	{ SUBRCU,   0                                                                  },
	{ SUBRC,    IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ SUBRC,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ STOREM,   IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ STOREM,   IFLAG_RA_PRESENT                                                   },
	{ CPLT,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPLT,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPLTU,    IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPLTU,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPLE,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPLE,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPLEU,    IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPLEU,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPGT,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPGT,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPGTU,    IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPGTU,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPGE,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPGE,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPGEU,    IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPGEU,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ ASLT,     IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASLT,     IFLAG_RA_PRESENT                                                   },
	{ ASLTU,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASLTU,    IFLAG_RA_PRESENT                                                   },
	{ ASLE,     IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASLE,     IFLAG_RA_PRESENT                                                   },
	{ ASLEU,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASLEU,    IFLAG_RA_PRESENT                                                   },
	{ ASGT,     IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASGT,     IFLAG_RA_PRESENT                                                   },
	{ ASGTU,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASGTU,    IFLAG_RA_PRESENT                                                   },
	{ ASGE,     IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASGE,     IFLAG_RA_PRESENT                                                   },
	{ ASGEU,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASGEU,    IFLAG_RA_PRESENT                                                   },
	{ CPEQ,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPEQ,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ CPNEQ,    IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ CPNEQ,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ MUL,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT | IFLAG_RB_PRESENT             },
	{ MUL,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ MULL,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT | IFLAG_RB_PRESENT             },
	{ MULL,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ DIV0,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT                                },
	{ DIV0,     IFLAG_RC_PRESENT                                                   },
	{ DIV,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT | IFLAG_RB_PRESENT             },
	{ DIV,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ DIVL,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT | IFLAG_RB_PRESENT             },
	{ DIVL,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ DIVREM,   IFLAG_RC_PRESENT | IFLAG_RA_PRESENT | IFLAG_RB_PRESENT             },
	{ DIVREM,   IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ ASEQ,     IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASEQ,     IFLAG_RA_PRESENT                                                   },
	{ ASNEQ,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ASNEQ,    IFLAG_RA_PRESENT                                                   },
	{ MULU,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT | IFLAG_RB_PRESENT             },
	{ MULU,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ INHW,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ INHW,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ EXTRACT,  IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ EXTRACT,  IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ EXHW,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ EXHW,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ EXHWS,    IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ SLL,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ SLL,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ SRL,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ SRL,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ SRA,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ SRA,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ IRET,     IFLAG_SUPERVISOR_ONLY                                              },
	{ HALT,     IFLAG_SUPERVISOR_ONLY                                              },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ IRETINV,  IFLAG_SUPERVISOR_ONLY                                              },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ AND,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ AND,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ OR,       IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ OR,       IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ XOR,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ XOR,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ XNOR,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ XNOR,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ NOR,      IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ NOR,      IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ NAND,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ NAND,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ ANDN,     IFLAG_RC_PRESENT | IFLAG_RB_PRESENT | IFLAG_RA_PRESENT             },
	{ ANDN,     IFLAG_RC_PRESENT | IFLAG_RA_PRESENT                                },
	{ SETIP,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT | IFLAG_RC_PRESENT             },
	{ INV,      IFLAG_SUPERVISOR_ONLY                                              },
	{ JMP,      0                                                                  },
	{ JMP,      0                                                                  },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ JMPF,     IFLAG_RA_PRESENT                                                   },
	{ JMPF,     IFLAG_RA_PRESENT                                                   },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ CALL,     IFLAG_RA_PRESENT                                                   },
	{ CALL,     IFLAG_RA_PRESENT                                                   },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ JMPT,     IFLAG_RA_PRESENT                                                   },
	{ JMPT,     IFLAG_RA_PRESENT                                                   },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ JMPFDEC,  IFLAG_RA_PRESENT                                                   },
	{ JMPFDEC,  IFLAG_RA_PRESENT                                                   },
	{ MFTLB,    IFLAG_SUPERVISOR_ONLY | IFLAG_RC_PRESENT | IFLAG_RA_PRESENT        },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ MTTLB,    IFLAG_SUPERVISOR_ONLY | IFLAG_RA_PRESENT | IFLAG_RB_PRESENT        },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ JMPI,     IFLAG_RB_PRESENT                                                   },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ JMPFI,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ MFSR,     IFLAG_RC_PRESENT                                                   },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ CALLI,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ JMPTI,    IFLAG_RA_PRESENT | IFLAG_RB_PRESENT                                },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ MTSR,     IFLAG_RB_PRESENT                                                   },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ EMULATE,  0                                                                  },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ MULTM,    0                                                                  },
	{ MULTMU,   0                                                                  },
	{ MULTIPLY, 0                                                                  },
	{ DIVIDE,   0                                                                  },
	{ MULTIPLU, 0                                                                  },
	{ DIVIDU,   0                                                                  },
	{ CONVERT,  0                                                                  },
	{ SQRT,     0                                                                  },
	{ CLASS,    0                                                                  },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	{ ILLEGAL,  IFLAG_ILLEGAL                                                      },
	// FEQ
	// DEQ
	// TODO! etc
};


/***************************************************************************
    PIPELINE STAGES
***************************************************************************/

static void fetch_decode(void)
{
	UINT32 inst;
	UINT32 op_flags;

	inst = read_program_word(a29k.pc);
	a29k.next_ir = inst;

	op_flags = op_table[inst >> 24].flags;

	/* Privledge violations */
	if (USER_MODE)
	{
		if ((op_flags & IFLAG_SUPERVISOR_ONLY))
		{
			signal_exception(EXCEPTION_PROTECTION_VIOLATION);
			return;
		}

		if ((op_flags & IFLAG_SPR_ACCESS))
		{
			/* TODO: Is this the right place to check this? */
			if (INST_SA_FIELD(inst) < 128)
			{
				signal_exception(EXCEPTION_PROTECTION_VIOLATION);
				return;
			}
		}

		/* Register bank protection */
		if ((op_flags & IFLAG_RA_PRESENT) && REGISTER_IS_PROTECTED(INST_RA_FIELD(inst)))
		{
			signal_exception(EXCEPTION_PROTECTION_VIOLATION);
			return;
		}

		if ((op_flags & IFLAG_RB_PRESENT) && REGISTER_IS_PROTECTED(INST_RB_FIELD(inst)))
		{
			signal_exception(EXCEPTION_PROTECTION_VIOLATION);
			return;
		}

		if ((op_flags & IFLAG_RC_PRESENT) && REGISTER_IS_PROTECTED(INST_RC_FIELD(inst)))
		{
			signal_exception(EXCEPTION_PROTECTION_VIOLATION);
			return;
		}
	}

	if (a29k.pl_flags & PFLAG_IRET)
		a29k.next_pc = a29k.iret_pc;
	else
		a29k.next_pc += 4;
}


/***************************************************************************
    CORE EXECUTION LOOP
***************************************************************************/

static int am29000_execute(int cycles)
{
	am29000_icount = cycles;

	external_irq_check();

	do
	{
		a29k.next_pl_flags = PFLAG_EXECUTE_EN;

		if (!FREEZE_MODE)
		{
			a29k.pc1 = a29k.pc0;
			a29k.pc0 = a29k.pc;
		}

		if (a29k.exceptions)
		{
			a29k.ops = a29k.cps;
			a29k.cps &= ~(CPS_TE | CPS_TP | CPS_TU | CPS_FZ | CPS_LK | CPS_WM | CPS_PD | CPS_PI | CPS_SM | CPS_DI | CPS_DA);
			a29k.cps |= (CPS_FZ | CPS_PD | CPS_PI | CPS_SM | CPS_DI | CPS_DA);

			if (a29k.pl_flags & PFLAG_IRET)
			{
				a29k.pc0 = a29k.iret_pc;
				a29k.pc1 = a29k.next_pc;
			}

			if (a29k.cfg & CFG_VF)
			{
				UINT32 vaddr = a29k.vab | a29k.exception_queue[0] * 4;
				UINT32 vect = data_read_dword_32be(vaddr);

				a29k.pc = vect & ~3;
				a29k.next_pc = a29k.pc;
			}
			else
			{
				fatalerror("Am29000: Non vectored interrupt fetch!\n");
			}

			a29k.exceptions = 0;
			a29k.pl_flags = 0;
		}

		CALL_MAME_DEBUG;

		fetch_decode();

		if (a29k.pl_flags & PFLAG_EXECUTE_EN)
		{
			if (!FREEZE_MODE)
				a29k.pc2 = a29k.pc1;

			op_table[a29k.exec_ir >> 24].opcode();
		}

		a29k.exec_ir = a29k.next_ir;
		a29k.pl_flags = a29k.next_pl_flags;
		a29k.exec_pc = a29k.pc;
		a29k.pc = a29k.next_pc;
	} while (--am29000_icount > 0);

	return cycles - am29000_icount;
}

static void am29000_set_irq_line(int irqline, int state)
{
	if (state)
		a29k.irq_lines |= (1 << irqline);
	else
		a29k.irq_lines &= ~(1 << irqline);

	/* TODO : CHECK IRQs */
}


/***************************************************************************
    INITIALIZATION AND SHUTDOWN
***************************************************************************/

static void am29000_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	memset(&a29k, 0, sizeof(a29k));

	a29k.cfg = (PRL_AM29000 | PRL_REV_D) << CFG_PRL_SHIFT;

	state_save_register_item_array("am29000", index, a29k.r);
	state_save_register_item_array("am29000", index, a29k.tlb);
	state_save_register_item("am29000", index, a29k.pc);
	state_save_register_item("am29000", index, a29k.vab);
	state_save_register_item("am29000", index, a29k.ops);
	state_save_register_item("am29000", index, a29k.cps);
	state_save_register_item("am29000", index, a29k.cfg);
	state_save_register_item("am29000", index, a29k.cha);
	state_save_register_item("am29000", index, a29k.chd);
	state_save_register_item("am29000", index, a29k.chc);
	state_save_register_item("am29000", index, a29k.rbp);
	state_save_register_item("am29000", index, a29k.tmc);
	state_save_register_item("am29000", index, a29k.tmr);
	state_save_register_item("am29000", index, a29k.pc0);
	state_save_register_item("am29000", index, a29k.pc1);
	state_save_register_item("am29000", index, a29k.pc2);
	state_save_register_item("am29000", index, a29k.mmu);
	state_save_register_item("am29000", index, a29k.lru);
	state_save_register_item("am29000", index, a29k.ipc);
	state_save_register_item("am29000", index, a29k.ipa);
	state_save_register_item("am29000", index, a29k.ipb);
	state_save_register_item("am29000", index, a29k.q);
	state_save_register_item("am29000", index, a29k.alu);
	state_save_register_item("am29000", index, a29k.fpe);
	state_save_register_item("am29000", index, a29k.inte);
	state_save_register_item("am29000", index, a29k.fps);
	state_save_register_item("am29000", index, a29k.exceptions);
	state_save_register_item_array("am29000", index, a29k.exception_queue);
	state_save_register_item("am29000", index, a29k.irq_active);
	state_save_register_item("am29000", index, a29k.irq_lines);
	state_save_register_item("am29000", index, a29k.exec_ir);
	state_save_register_item("am29000", index, a29k.next_ir);
	state_save_register_item("am29000", index, a29k.pl_flags);
	state_save_register_item("am29000", index, a29k.next_pl_flags);
	state_save_register_item("am29000", index, a29k.iret_pc);
	state_save_register_item("am29000", index, a29k.exec_pc);
	state_save_register_item("am29000", index, a29k.next_pc);
}

static void am29000_reset(void)
{
	a29k.cps = CPS_FZ | CPS_RE | CPS_PD | CPS_PI | CPS_SM | CPS_DI | CPS_DA;
	a29k.cfg &= ~(CFG_DW | CFG_CD);
	a29k.chc &= ~CHC_CV;

	a29k.pc = 0;
	a29k.next_pc = 0;
	a29k.next_pl_flags = 0;
	a29k.pl_flags = 0;
	a29k.exceptions = 0;
	a29k.irq_lines = 0;
	a29k.irq_active = 0;
}

static void am29000_get_context(void *dst)
{
	if (dst)
		*(am29000_state *)dst = a29k;
}

static void am29000_set_context(void *src)
{
	if (src)
		a29k = *(am29000_state *)src;
}

static void am29000_set_reg(int regnum, UINT32 val)
{
	if (regnum >= AM29000_R64 && regnum <= AM29000_R255)
	{
		a29k.r[64 + regnum - AM29000_R64] = val;
		return;
	}

	switch (regnum)
	{
		case AM29000_PC:
			a29k.pc = val;
			a29k.next_pc = val;
			break;
		case AM29000_VAB:  a29k.vab = val;  break;
		case AM29000_OPS:  a29k.ops = val;  break;
		case AM29000_CPS:  a29k.cps = val;  break;
		case AM29000_CFG:  a29k.cfg = val;  break;
		case AM29000_CHA:  a29k.cha = val;  break;
		case AM29000_CHD:  a29k.chd = val;  break;
		case AM29000_CHC:  a29k.chc = val;  break;
		case AM29000_RBP:  a29k.rbp = val;  break;
		case AM29000_TMC:  a29k.tmc = val;  break;
		case AM29000_TMR:  a29k.tmr = val;  break;
		case AM29000_PC0:  a29k.pc0 = val;  break;
		case AM29000_PC1:  a29k.pc1 = val;  break;
		case AM29000_PC2:  a29k.pc2 = val;  break;
		case AM29000_MMU:  a29k.mmu = val;  break;
		case AM29000_LRU:  a29k.lru = val;  break;
		case AM29000_IPC:  a29k.ipc = val;  break;
		case AM29000_IPA:  a29k.ipa = val;  break;
		case AM29000_IPB:  a29k.ipb = val;  break;
		case AM29000_Q:    a29k.q = val;    break;
		case AM29000_ALU:  a29k.alu = val;  break;
		case AM29000_FPE:  a29k.fpe = val;  break;
		case AM29000_INTE: a29k.inte = val; break;
		case AM29000_FPS:  a29k.fps = val;  break;
		case AM29000_R1:   a29k.r[1] = val; break;
	}
}

static UINT32 am29000_get_reg(int regnum)
{
	if (regnum >= AM29000_R64 && regnum <= AM29000_R255)
		return a29k.r[64 + regnum - AM29000_R64];

	switch (regnum)
	{
		case AM29000_PC:   return a29k.pc;
		case AM29000_VAB:  return a29k.vab;
		case AM29000_OPS:  return a29k.ops;
		case AM29000_CPS:  return a29k.cps;
		case AM29000_CFG:  return a29k.cfg;
		case AM29000_CHA:  return a29k.cha;
		case AM29000_CHD:  return a29k.chd;
		case AM29000_CHC:  return a29k.chc;
		case AM29000_RBP:  return a29k.rbp;
		case AM29000_TMC:  return a29k.tmc;
		case AM29000_TMR:  return a29k.tmr;
		case AM29000_PC0:  return a29k.pc0;
		case AM29000_PC1:  return a29k.pc1;
		case AM29000_PC2:  return a29k.pc2;
		case AM29000_MMU:  return a29k.mmu;
		case AM29000_LRU:  return a29k.lru;
		case AM29000_IPC:  return a29k.ipc;
		case AM29000_IPA:  return a29k.ipa;
		case AM29000_IPB:  return a29k.ipb;
		case AM29000_Q:    return a29k.q;
		case AM29000_ALU:  return a29k.alu;
		case AM29000_BP:   return GET_ALU_BP;
		case AM29000_FC:   return GET_ALU_FC;
		case AM29000_CR:   return GET_CHC_CR;
		case AM29000_FPE:  return a29k.fpe;
		case AM29000_INTE: return a29k.inte;
		case AM29000_FPS:  return a29k.fps;
		case AM29000_R1:   return a29k.r[1];
	}
	return 0;
}

static void am29000_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_INPUT_STATE && state <= CPUINFO_INT_INPUT_STATE + AM29000_INTR3)
	{
		am29000_set_irq_line(state - CPUINFO_INT_INPUT_STATE, info->i);
		return;
	}

	if (state >= CPUINFO_INT_REGISTER && state <= CPUINFO_INT_REGISTER + AM29000_R255)
	{
		am29000_set_reg(state - CPUINFO_INT_REGISTER, info->i);
		return;
	}

	if (state == CPUINFO_INT_PC)
		am29000_set_reg(AM29000_PC, info->i);
}

static offs_t am29000_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	return am29000_disassemble(buffer, pc, program_read_dword_32be(pc));
#else
	return 0;
#endif
}

static UINT8 am29000_reg_layout[] =
{
	AM29000_PC, AM29000_CPS, -1,
	AM29000_OPS, AM29000_CFG, -1,
	AM29000_ALU, AM29000_Q, -1,
	AM29000_PC0, AM29000_PC1, -1,
	AM29000_PC2, AM29000_VAB, -1,
	AM29000_R1, AM29000_RBP, -1,
	AM29000_IPA, AM29000_IPB, -1,
	AM29000_IPC, AM29000_TMC, -1,
	AM29000_R64, AM29000_R64 + 1, -1,
	AM29000_R64 + 2, AM29000_R64 + 3, -1,
	0
};

static UINT8 am29000_win_layout[] =
{
	45, 0,35,19,	/* register window (top right) */
	 0, 0,44,13,	/* disassembler window (left, upper) */
	 0,14,44, 8,	/* memory #1 window (left, middle) */
	45,20,35, 8,	/* memory #2 window (lower) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

static const char *am29000_reg_name(int regnum)
{
	static const char *const names[] =
	{
		"PC", "VAB", "OPS", "CPS", "CFG", "CHA", "CHD", "CHC", "RBP", "TMC", "TMR", "PC0", "PC1", "PC2",
		"MMU", "LRU", "IPC", "IPA", "IPB", "Q", "ALU", "BP", "FC", "CR", "FPE", "INTE", "FPS", "R1"
	};

	return names[regnum - AM29000_PC];
}

void am29000_get_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_REGISTER + AM29000_R64 && state <= CPUINFO_INT_REGISTER + AM29000_R255)
	{
		info->i = am29000_get_reg(state - CPUINFO_INT_REGISTER);
		return;
	}

	if (state >= CPUINFO_STR_REGISTER + AM29000_R64 && state <= CPUINFO_STR_REGISTER + AM29000_R255)
	{
		int regnum = state - CPUINFO_STR_REGISTER;
		sprintf(info->s = cpuintrf_temp_str(), "R%d :%08X", 64 + regnum - AM29000_R64, am29000_get_reg(regnum));
		return;
	}

	if (state >= CPUINFO_INT_REGISTER + AM29000_PC && state <= CPUINFO_INT_REGISTER + AM29000_R1)
	{
		info->i = am29000_get_reg(state - CPUINFO_INT_REGISTER);
		return;
	}

	if (state >= CPUINFO_STR_REGISTER + AM29000_PC && state <= CPUINFO_STR_REGISTER + AM29000_R1)
	{
		int regnum = state - CPUINFO_STR_REGISTER;
		sprintf(info->s = cpuintrf_temp_str(), "%-4s:%08X", am29000_reg_name(regnum), am29000_get_reg(regnum));
		return;
	}

	switch (state)
	{
		/* Interface functions and variables */
		case CPUINFO_PTR_SET_INFO:              info->setinfo       = am29000_set_info;     break;
		case CPUINFO_PTR_GET_CONTEXT:           info->getcontext    = am29000_get_context;  break;
		case CPUINFO_PTR_SET_CONTEXT:           info->setcontext    = am29000_set_context;  break;
		case CPUINFO_PTR_INIT:                  info->init          = am29000_init;         break;
		case CPUINFO_PTR_RESET:                 info->reset         = am29000_reset;        break;
		case CPUINFO_PTR_EXIT:                  info->exit          = 0;                    break;
		case CPUINFO_PTR_EXECUTE:               info->execute       = am29000_execute;      break;
		case CPUINFO_PTR_BURN:                  info->burn          = 0;                    break;
		case CPUINFO_PTR_DISASSEMBLE:           info->disassemble   = am29000_dasm;         break;
		case CPUINFO_PTR_DISASSEMBLE_NEW:       info->disassemble_new = NULL;               break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:   info->icount        = &am29000_icount;      break;
		case CPUINFO_INT_CONTEXT_SIZE:          info->i             = sizeof(am29000_state); break;
		case CPUINFO_PTR_REGISTER_LAYOUT:       info->p             = am29000_reg_layout;   break;
		case CPUINFO_PTR_WINDOW_LAYOUT:         info->p             = am29000_win_layout;   break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES: info->i = 4;                                break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 4;                                break;
		case CPUINFO_INT_MIN_CYCLES:            info->i = 1;                                break;
		case CPUINFO_INT_MAX_CYCLES:            info->i = 2;                                break;

		/* Bus sizes */
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32; break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32; break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;  break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:    info->i = 32; break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA:    info->i = 32; break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA:    info->i = 0;  break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:      info->i = 0;  break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO:      info->i = 0;  break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO:      info->i = 0;  break;

		/* CPU misc parameters */
		case CPUINFO_STR_NAME:                  strcpy(info->s = cpuintrf_temp_str(), "Am29000"); break;
		case CPUINFO_STR_CORE_FAMILY:           strcpy(info->s = cpuintrf_temp_str(), "AMD Am29000"); break;
		case CPUINFO_STR_CORE_VERSION:          strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:             strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:          strcpy(info->s = cpuintrf_temp_str(), "Philip Bennett"); break;
		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c%c|%3d",
					a29k.alu & ALU_V ? 'V' : '.',
					a29k.alu & ALU_Z ? 'Z' : '.',
					a29k.alu & ALU_N ? 'N' : '.',
					a29k.alu & ALU_C ? 'C' : '.',
					a29k.cps & CPS_IP ? 'I' : '.',
					a29k.cps & CPS_FZ ? 'F' : '.',
					a29k.cps & CPS_SM ? 'S' : 'U',
					a29k.cps & CPS_DI ? 'I' : '.',
					a29k.cps & CPS_DA ? 'D' : '.',
					(a29k.r[1] >> 2) & 0x7f);
			break;
		case CPUINFO_INT_ENDIANNESS:            info->i = CPU_IS_BE;                        break;
		case CPUINFO_INT_INPUT_LINES:           info->i = 4;                                break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:    info->i = 0;                                break;
		case CPUINFO_INT_CLOCK_DIVIDER:         info->i = 1;                                break;

		/* CPU main state */
		case CPUINFO_INT_PC:                    info->i = a29k.pc;                          break;
		case CPUINFO_INT_PREVIOUSPC:            info->i = a29k.exec_pc;                     break;
		case CPUINFO_INT_SP:                    info->i = a29k.r[1];                        break;
	}
}

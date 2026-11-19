// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

    xavix.c

    XaviX (SSD 2000 NEC 85605-621) CPU

    6502 with custom opcodes, integrated gfx / sound.

    The 0x0000-0x7fff area is a 'low bus' area, always the same regardless
    of the banking. The 0x8000-0xffff area is banked, with individual code
    and data banks.

    The zero page address 0x00ff contains the DATA bank, and the address
    0x00fe the CODE bank.

    The program address space is the external bus (24 bit), the data
    address space is the low bus (15 bit).

***************************************************************************/

#include "debugger.h"
#include "xavix.h"

/* the 6502 macros use these names */
#define m6502 xavix
#define m6502_ICount xavix_ICount
#define FAST_MEMORY 0

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#include "../m6502/ops02.h"

typedef struct
{
	void	(*dummy)(void);
	PAIR	ppc;			/* previous program counter */
	PAIR	pc; 			/* program counter */
	PAIR	sp; 			/* stack pointer (always 100 - 1FF) */
	PAIR	zp; 			/* zero page address */
	PAIR	ea; 			/* effective address */
	UINT8	a;				/* Accumulator */
	UINT8	x;				/* X index register */
	UINT8	y;				/* Y index register */
	UINT8	p;				/* Processor status */
	UINT8	pending_irq;	/* nonzero if an IRQ is pending */
	UINT8	after_cli;		/* pending IRQ and last insn cleared I */
	UINT8	nmi_state;
	UINT8	irq_state;
	UINT8	databank;
	UINT8	codebank;
	UINT8	j;
	UINT8	k;
	UINT8	l;
	UINT8	m;
	UINT32	pa;				/* 24 bit address registers */
	UINT32	pb;
	int 	(*irq_callback)(int irqline);
	int 	(*get_vector)(int which, int half);
} xavix_Regs;

static int xavix_ICount = 0;

static xavix_Regs xavix;

#define XAVIX_NMI_VEC 0xfffa
#define XAVIX_RST_VEC 0xfffc
#define XAVIX_IRQ_VEC 0xfffe

static UINT8 xavix_reg_layout[] = {
	XAVIX_PC, XAVIX_S, XAVIX_P, XAVIX_A, XAVIX_X, XAVIX_Y, -1,
	XAVIX_CODEBANK, XAVIX_DATABANK, XAVIX_J, XAVIX_K, XAVIX_L, XAVIX_M, XAVIX_PA, XAVIX_PB, 0
};

static UINT8 xavix_win_layout[] = {
	25, 0,55, 2,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 3,55, 9,	/* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/* cycles of the standard 6502, the custom opcodes use the minimum */
static const UINT8 xavix_cycles[256] = {
	7,6,2,2,5,3,5,2,3,2,2,2,6,4,6,2,
	2,5,3,4,6,4,6,4,2,4,2,2,7,4,7,2,
	6,6,8,2,3,3,5,2,4,2,2,2,4,4,6,2,
	2,5,3,4,4,4,6,4,2,4,2,2,4,4,7,2,
	6,6,2,2,2,3,5,2,3,2,2,2,3,4,6,2,
	2,5,3,4,2,4,6,4,2,4,2,2,4,4,7,2,
	6,6,2,2,2,3,5,2,4,2,2,2,5,4,6,2,
	2,5,4,4,2,4,6,4,2,4,2,2,6,4,7,2,
	6,6,3,2,3,3,3,2,2,2,2,2,4,4,4,2,
	2,6,4,4,4,4,4,4,2,5,2,2,5,5,5,2,
	2,6,2,2,3,3,3,2,2,2,2,2,4,4,4,2,
	2,5,2,4,4,4,4,4,2,4,2,2,4,4,4,2,
	2,6,2,2,3,3,5,2,2,2,2,2,4,4,6,2,
	2,5,2,4,2,4,6,4,2,4,2,2,2,4,7,2,
	2,6,2,2,3,3,5,2,2,2,2,2,4,4,6,2,
	2,5,2,4,2,4,6,4,2,4,2,2,2,4,7,2
};

/***************************************************************
 * memory access
 ***************************************************************/

/* The low bus is accessed only by the data banks below 0x80 at the
   addresses below 0x8000, everything else goes to the external bus */
INLINE UINT8 xavix_read_data(UINT8 databank, UINT16 adr)
{
	if (databank < 0x80 && adr < 0x8000)
	{
		/* the bank registers are visible at the same addresses of the zero page */
		if (adr == 0xfe)
			return xavix.codebank;
		if (adr == 0xff)
			return xavix.databank;
		return data_read_byte_8(adr);
	}
	return program_read_byte_8(((databank << 16) | adr) & 0x7fffff);
}

INLINE void xavix_write_data(UINT8 databank, UINT16 adr, UINT8 data)
{
	if (databank < 0x80 && adr < 0x8000)
	{
		if (adr == 0xfe)
		{
			xavix.codebank = data;
			return;
		}
		if (adr == 0xff)
		{
			xavix.databank = data;
			return;
		}
		data_write_byte_8(adr, data);
		return;
	}
	program_write_byte_8(((databank << 16) | adr) & 0x7fffff, data);
}

/* the zero page and the stack are always in the low bus, whatever the data bank is */
INLINE UINT8 xavix_read_zp(UINT8 adr)
{
	if (adr == 0xfe)
		return xavix.codebank;
	if (adr == 0xff)
		return xavix.databank;
	return data_read_byte_8(adr);
}

INLINE void xavix_write_zp(UINT8 adr, UINT8 data)
{
	if (adr == 0xfe)
		xavix.codebank = data;
	else if (adr == 0xff)
		xavix.databank = data;
	else
		data_write_byte_8(adr, data);
}

/* opcodes and operands are read with the code bank. The ROM is the only
   thing visible by the code in the banks above 0x80 */
INLINE UINT8 xavix_read_code(UINT16 adr)
{
	UINT32 addr = adr | (xavix.codebank << 16);

	if ((addr & 0x800000) || (addr & 0x8000))
		return program_read_byte_8(addr & 0x7fffff);
	return data_read_byte_8(addr & 0x7fff);
}

UINT8 xavix_read_full_data(UINT32 addr)
{
	return xavix_read_data((addr >> 16) & 0xff, addr & 0xffff);
}

void xavix_write_full_data(UINT32 addr, UINT8 data)
{
	xavix_write_data((addr >> 16) & 0xff, addr & 0xffff, data);
}

#undef RDOP
#undef RDOPARG
#undef RDMEM
#undef WRMEM
#undef PUSH
#undef PULL
#undef CHANGE_PC
#define RDOP() xavix_read_code(PCW++)
#define RDOPARG() xavix_read_code(PCW++)
#define RDMEM(addr) xavix_read_data(xavix.databank, addr)
#define WRMEM(addr,data) xavix_write_data(xavix.databank, addr, data)
#define RDZP(addr) xavix_read_zp(addr)
#define WRZP(addr,data) xavix_write_zp(addr, data)
#define RDFULL(addr) xavix_read_full_data(addr)
#define WRFULL(addr,data) xavix_write_full_data(addr, data)
#define CHANGE_PC
#define PUSH(Rg) data_write_byte_8(SPD, Rg); S--
#define PULL(Rg) S++; Rg = data_read_byte_8(SPD)

/* the pointers are read from the zero page */
#undef EA_IDX
#define EA_IDX													\
	ZPL = RDOPARG() + X;										\
	EAL = RDZP(ZPL);											\
	ZPL++;														\
	EAH = RDZP(ZPL)

#undef EA_IDY
#define EA_IDY													\
	ZPL = RDOPARG();											\
	EAL = RDZP(ZPL);											\
	ZPL++;														\
	EAH = RDZP(ZPL);											\
	if (EAL + Y > 0xff) 										\
		xavix_ICount--; 										\
	EAW += Y

#define ASR 													\
	P &= ~(F_N|F_Z|F_C);										\
	if (tmp & 1)												\
		P |= F_C;												\
	tmp = (UINT8)tmp >> 1;										\
	if (!tmp)													\
		P |= F_Z;												\
	else if (tmp & 0x40)										\
	{															\
		P |= F_N;												\
		tmp |= 0x80;											\
	}

/***************************************************************
 * interrupts
 ***************************************************************/

/* The interrupts and the BRK force the code bank to 0, and push the old one
   before the return address. The vectors can be redirected by the custom ones */
static void xavix_interrupt(int vector, int which, int software)
{
	int lo = -1;

	PUSH(xavix.codebank);
	xavix.codebank = 0;
	PUSH(PCH);
	PUSH(PCL);
	PUSH(software ? P : (P & ~F_B));
	P |= F_I;

	if (xavix.get_vector)
		lo = xavix.get_vector(which, 1);

	if (lo != -1)
	{
		PCL = lo;
		PCH = xavix.get_vector(which, 0);
	}
	else
	{
		PCL = xavix_read_code(vector);
		PCH = xavix_read_code(vector + 1);
	}
}

INLINE void xavix_take_irq(void)
{
	if (!(P & F_I))
	{
		xavix_ICount -= 7;
		xavix_interrupt(XAVIX_IRQ_VEC, 1, 0);
		if (xavix.irq_callback)
			(*xavix.irq_callback)(0);
	}
	xavix.pending_irq = 0;
}

/***************************************************************
 * opcodes
 ***************************************************************/

/* ALU operations in the order of the opcode bits 7-5 (ora, and, eor, adc, sta, lda, cmp, sbc) */
#define ALU_OP(alu, val)										\
	switch (alu)												\
	{															\
	case 0: tmp = (val); ORA; break;							\
	case 1: tmp = (val); AND; break;							\
	case 2: tmp = (val); EOR; break;							\
	case 3: tmp = (val); ADC; break;							\
	case 5: tmp = (val); LDA; break;							\
	case 6: tmp = (val); CMP; break;							\
	case 7: tmp = (val); SBC; break;							\
	}

#define RMW_ZPG(OP) EA_ZPG; tmp = RDZP(EAD); OP; WRZP(EAD, tmp)
#define RMW_ZPX(OP) EA_ZPX; tmp = RDZP(EAD); OP; WRZP(EAD, tmp)
#define RMW_ABS(OP) EA_ABS; tmp = RDMEM(EAD); OP; WRMEM(EAD, tmp)
#define RMW_ABX(OP) EA_ABX; tmp = RDMEM(EAD); OP; WRMEM(EAD, tmp)

static void xavix_execute_alu(int op)
{
	int tmp;
	int alu = op >> 5;
	int store = (alu == 4);

	switch ((op >> 2) & 7)
	{
	case 0: /* idx */
		EA_IDX;
		if (store) WRMEM(EAD, A); else { ALU_OP(alu, RDMEM(EAD)); }
		break;
	case 1: /* zpg */
		EA_ZPG;
		if (store) WRZP(EAD, A); else { ALU_OP(alu, RDZP(EAD)); }
		break;
	case 2: /* imm */
		ALU_OP(alu, RDOPARG());
		break;
	case 3: /* abs */
		EA_ABS;
		if (store) WRMEM(EAD, A); else { ALU_OP(alu, RDMEM(EAD)); }
		break;
	case 4: /* idy */
		EA_IDY;
		if (store) WRMEM(EAD, A); else { ALU_OP(alu, RDMEM(EAD)); }
		break;
	case 5: /* zpx */
		EA_ZPX;
		if (store) WRZP(EAD, A); else { ALU_OP(alu, RDZP(EAD)); }
		break;
	case 6: /* aby */
		EA_ABY;
		if (store) WRMEM(EAD, A); else { ALU_OP(alu, RDMEM(EAD)); }
		break;
	case 7: /* abx */
		EA_ABX;
		if (store) WRMEM(EAD, A); else { ALU_OP(alu, RDMEM(EAD)); }
		break;
	}
}

/* custom opcodes, with the lower two bits set */
static void xavix_execute_custom(int op)
{
	int tmp;
	int alu = op >> 5;
	int sel = (op >> 2) & 3;

	if (!(op & 0x10))
	{
		/* the operand is one of the J, K, L, M registers */
		UINT8 val = 0;

		switch (sel)
		{
		case 0: val = xavix.j; break;
		case 1: val = xavix.k; break;
		case 2: val = xavix.l; break;
		case 3: val = xavix.m; break;
		}

		if (alu == 4)
		{
			switch (sel)
			{
			case 0: xavix.j = A; break;
			case 1: xavix.k = A; break;
			case 2: xavix.l = A; break;
			case 3: xavix.m = A; break;
			}
		}
		else
		{
			ALU_OP(alu, val);
		}
	}
	else if (sel < 2)
	{
		/* the operand is addressed by the PA or the PB registers */
		UINT32 addr = (sel == 0) ? xavix.pa : xavix.pb;

		if (alu == 4)
			WRFULL(addr, A);
		else
		{
			ALU_OP(alu, RDFULL(addr));
		}
	}
	else
	{
		/* the operations on the PA or PB registers, the low byte, the middle byte or the high byte */
		UINT32 *reg = (sel == 2) ? &xavix.pa : &xavix.pb;

		switch (alu)
		{
		case 0: *reg = (*reg & 0xffff00) | A; break;
		case 1: A = *reg & 0xff; SET_NZ(A); break;
		case 2: *reg = (*reg & 0xff00ff) | (A << 8); break;
		case 3: A = (*reg >> 8) & 0xff; SET_NZ(A); break;
		case 4: *reg = (*reg & 0x00ffff) | (A << 16); break;
		case 5: A = (*reg >> 16) & 0xff; SET_NZ(A); break;
		case 6:
			*reg = (*reg & ~0xff) | ((*reg - 1) & 0xff);
			SET_NZ(*reg & 0xff);
			break;
		case 7:
			*reg = (*reg & ~0xff) | ((*reg + 1) & 0xff);
			SET_NZ(*reg & 0xff);
			break;
		}
	}
}

static void xavix_execute_op(int op)
{
	int tmp;

	if (op == 0x89)
	{
		/* sev */
		P |= F_V;
		return;
	}

	switch (op & 3)
	{
	case 1:
		xavix_execute_alu(op);
		return;
	case 3:
		xavix_execute_custom(op);
		return;
	}

	switch (op)
	{
	case 0x00: /* brk */
		PCW++;
		xavix_interrupt(XAVIX_IRQ_VEC, 1, 1);
		break;
	case 0x20: /* jsr */
		EAL = RDOPARG();
		PUSH(PCH);
		PUSH(PCL);
		EAH = RDOPARG();
		PCD = EAD;
		break;
	case 0x22: /* callf */
		tmp = RDOPARG();
		EAL = RDOPARG();
		PUSH(xavix.codebank);
		PUSH(PCH);
		PUSH(PCL);
		EAH = RDOPARG();
		PCD = EAD;
		xavix.codebank = tmp;
		break;
	case 0x40: /* rti */
		PULL(P);
		P |= F_T | F_B;
		PULL(PCL);
		PULL(PCH);
		PULL(xavix.codebank);
		if ((xavix.irq_state != CLEAR_LINE) && !(P & F_I))
			xavix.after_cli = 1;
		break;
	case 0x60: /* rts */
		PULL(PCL);
		PULL(PCH);
		PCW++;
		break;
	case 0x80: /* retf */
		PULL(PCL);
		PULL(PCH);
		PULL(xavix.codebank);
		PCW++;
		break;

	case 0x10: BPL; break;
	case 0x30: BMI; break;
	case 0x50: BVC; break;
	case 0x70: BVS; break;
	case 0x90: BCC; break;
	case 0xb0: BCS; break;
	case 0xd0: BNE; break;
	case 0xf0: BEQ; break;

	case 0x4c: /* jmp */
		EA_ABS;
		PCD = EAD;
		break;
	case 0x5c: /* jmp far */
		tmp = RDOPARG();
		EAL = RDOPARG();
		EAH = RDOPARG();
		PCD = EAD;
		xavix.codebank = tmp;
		break;
	case 0x6c: /* jmp (ind) */
		EA_IND;
		PCD = EAD;
		break;
	case 0x7c: /* jmp far (ind) */
		EA_ABS;
		PCL = RDMEM(EAD);
		PCH = RDMEM((EAD + 1) & 0xffff);
		xavix.codebank = RDMEM((EAD + 2) & 0xffff);
		break;

	case 0x08: PHP; break;
	case 0x28: PLP; break;
	case 0x48: PHA; break;
	case 0x68: PLA; break;
	case 0x12: PUSH(X); break;
	case 0x52: PUSH(Y); break;
	case 0x32: PULL(X); SET_NZ(X); break;
	case 0x72: PULL(Y); SET_NZ(Y); break;

	case 0x18: CLC; break;
	case 0x38: SEC; break;
	case 0x58: CLI; break;
	case 0x78: SEI; break;
	case 0xb8: CLV; break;
	case 0xd8: CLD; break;
	case 0xf8: SED; break;
	case 0x02: /* cmc */
		P ^= F_C;
		break;

	case 0x88: Y--; SET_NZ(Y); break;
	case 0xc8: Y++; SET_NZ(Y); break;
	case 0xca: X--; SET_NZ(X); break;
	case 0xe8: X++; SET_NZ(X); break;
	case 0x8a: A = X; SET_NZ(A); break;
	case 0x98: A = Y; SET_NZ(A); break;
	case 0x9a: S = X; break;
	case 0xa8: Y = A; SET_NZ(Y); break;
	case 0xaa: X = A; SET_NZ(X); break;
	case 0xba: X = S; SET_NZ(X); break;

	case 0xb2: A = 0; SET_NZ(A); break;
	case 0xc2: A--; SET_NZ(A); break;
	case 0xd2: A ^= 0xff; SET_NZ(A); break;
	case 0xe2: A++; SET_NZ(A); break;
	case 0xf2: A = (0 - A) & 0xff; SET_NZ(A); break;

	case 0x0a: tmp = A; ASL; A = tmp; break;
	case 0x2a: tmp = A; ROL; A = tmp; break;
	case 0x4a: tmp = A; LSR; A = tmp; break;
	case 0x6a: tmp = A; ROR; A = tmp; break;
	case 0x1a: tmp = A; ASR; A = tmp; break;

	case 0x04: RMW_ZPG(ASR); break;
	case 0x0c: RMW_ABS(ASR); break;
	case 0x14: RMW_ZPX(ASR); break;
	case 0x1c: RMW_ABX(ASR); break;

	case 0x06: RMW_ZPG(ASL); break;
	case 0x0e: RMW_ABS(ASL); break;
	case 0x16: RMW_ZPX(ASL); break;
	case 0x1e: RMW_ABX(ASL); break;
	case 0x26: RMW_ZPG(ROL); break;
	case 0x2e: RMW_ABS(ROL); break;
	case 0x36: RMW_ZPX(ROL); break;
	case 0x3e: RMW_ABX(ROL); break;
	case 0x46: RMW_ZPG(LSR); break;
	case 0x4e: RMW_ABS(LSR); break;
	case 0x56: RMW_ZPX(LSR); break;
	case 0x5e: RMW_ABX(LSR); break;
	case 0x66: RMW_ZPG(ROR); break;
	case 0x6e: RMW_ABS(ROR); break;
	case 0x76: RMW_ZPX(ROR); break;
	case 0x7e: RMW_ABX(ROR); break;
	case 0xc6: RMW_ZPG(DEC); break;
	case 0xce: RMW_ABS(DEC); break;
	case 0xd6: RMW_ZPX(DEC); break;
	case 0xde: RMW_ABX(DEC); break;
	case 0xe6: RMW_ZPG(INC); break;
	case 0xee: RMW_ABS(INC); break;
	case 0xf6: RMW_ZPX(INC); break;
	case 0xfe: RMW_ABX(INC); break;

	case 0x24: EA_ZPG; tmp = RDZP(EAD); BIT; break;
	case 0x2c: EA_ABS; tmp = RDMEM(EAD); BIT; break;
	case 0x34: EA_ZPX; tmp = RDZP(EAD); BIT; break;
	case 0x3c: EA_ABX; tmp = RDMEM(EAD); BIT; break;
	case 0x3a: tmp = RDOPARG(); BIT; break;

	case 0x82: EA_ZPG; WRZP(EAD, 0); break;
	case 0x92: EA_ABS; WRMEM(EAD, 0); break;

	case 0x84: EA_ZPG; WRZP(EAD, Y); break;
	case 0x8c: EA_ABS; WRMEM(EAD, Y); break;
	case 0x94: EA_ZPX; WRZP(EAD, Y); break;
	case 0x9c: EA_ABX; WRMEM(EAD, Y); break;
	case 0x86: EA_ZPG; WRZP(EAD, X); break;
	case 0x8e: EA_ABS; WRMEM(EAD, X); break;
	case 0x96: EA_ZPY; WRZP(EAD, X); break;
	case 0x9e: EA_ABY; WRMEM(EAD, X); break;

	case 0xa0: tmp = RDOPARG(); LDY; break;
	case 0xa4: EA_ZPG; tmp = RDZP(EAD); LDY; break;
	case 0xac: EA_ABS; tmp = RDMEM(EAD); LDY; break;
	case 0xb4: EA_ZPX; tmp = RDZP(EAD); LDY; break;
	case 0xbc: EA_ABX; tmp = RDMEM(EAD); LDY; break;
	case 0xa2: tmp = RDOPARG(); LDX; break;
	case 0xa6: EA_ZPG; tmp = RDZP(EAD); LDX; break;
	case 0xae: EA_ABS; tmp = RDMEM(EAD); LDX; break;
	case 0xb6: EA_ZPY; tmp = RDZP(EAD); LDX; break;
	case 0xbe: EA_ABY; tmp = RDMEM(EAD); LDX; break;

	case 0xc0: tmp = RDOPARG(); CPY; break;
	case 0xc4: EA_ZPG; tmp = RDZP(EAD); CPY; break;
	case 0xcc: EA_ABS; tmp = RDMEM(EAD); CPY; break;
	case 0xe0: tmp = RDOPARG(); CPX; break;
	case 0xe4: EA_ZPG; tmp = RDZP(EAD); CPX; break;
	case 0xec: EA_ABS; tmp = RDMEM(EAD); CPX; break;

	default: /* nop */
		break;
	}
}

/***************************************************************
 * CPU interface
 ***************************************************************/

static void xavix_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	const struct xavix_config *intf = config;

	memset(&xavix, 0, sizeof(xavix));
	xavix.irq_callback = irqcallback;
	if (intf)
		xavix.get_vector = intf->get_vector;

	state_save_register_item("xavix", index, xavix.pc.w.l);
	state_save_register_item("xavix", index, xavix.sp.w.l);
	state_save_register_item("xavix", index, xavix.p);
	state_save_register_item("xavix", index, xavix.a);
	state_save_register_item("xavix", index, xavix.x);
	state_save_register_item("xavix", index, xavix.y);
	state_save_register_item("xavix", index, xavix.pending_irq);
	state_save_register_item("xavix", index, xavix.after_cli);
	state_save_register_item("xavix", index, xavix.nmi_state);
	state_save_register_item("xavix", index, xavix.irq_state);
	state_save_register_item("xavix", index, xavix.databank);
	state_save_register_item("xavix", index, xavix.codebank);
	state_save_register_item("xavix", index, xavix.j);
	state_save_register_item("xavix", index, xavix.k);
	state_save_register_item("xavix", index, xavix.l);
	state_save_register_item("xavix", index, xavix.m);
	state_save_register_item("xavix", index, xavix.pa);
	state_save_register_item("xavix", index, xavix.pb);
}

static void xavix_reset(void)
{
	xavix.codebank = 0;
	xavix.databank = 0;

	PCL = xavix_read_code(XAVIX_RST_VEC);
	PCH = xavix_read_code(XAVIX_RST_VEC + 1);

	xavix.sp.d = 0x01ff;
	xavix.p = F_T|F_I|F_Z|F_B;
	xavix.pending_irq = 0;
	xavix.after_cli = 0;
	xavix.irq_state = 0;
	xavix.nmi_state = 0;
}

static void xavix_exit(void)
{
}

static void xavix_get_context(void *dst)
{
	if (dst)
		*(xavix_Regs*)dst = xavix;
}

static void xavix_set_context(void *src)
{
	if (src)
		xavix = *(xavix_Regs*)src;
}

static int xavix_execute(int cycles)
{
	xavix_ICount = cycles;

	do
	{
		UINT8 op;
		PPC = PCD;

		CALL_MAME_DEBUG;

		if (xavix.pending_irq)
			xavix_take_irq();

		op = RDOP();
		xavix_ICount -= xavix_cycles[op];
		xavix_execute_op(op);

		/* check if the I flag was just reset (interrupts enabled) */
		if (xavix.after_cli)
		{
			xavix.after_cli = 0;
			if (xavix.irq_state != CLEAR_LINE)
				xavix.pending_irq = 1;
		}
		else if (xavix.pending_irq)
			xavix_take_irq();

	} while (xavix_ICount > 0);

	return cycles - xavix_ICount;
}

static void xavix_set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (xavix.nmi_state == state)
			return;
		xavix.nmi_state = state;
		if (state != CLEAR_LINE)
		{
			xavix_ICount -= 7;
			xavix_interrupt(XAVIX_NMI_VEC, 0, 0);
		}
	}
	else
	{
		xavix.irq_state = state;
		if (state != CLEAR_LINE)
			xavix.pending_irq = 1;
	}
}

static void xavix_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_INPUT_STATE + XAVIX_IRQ_LINE:	xavix_set_irq_line(XAVIX_IRQ_LINE, info->i);	break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	xavix_set_irq_line(INPUT_LINE_NMI, info->i);	break;

		case CPUINFO_INT_PC:							PCW = info->i;							break;
		case CPUINFO_INT_REGISTER + XAVIX_PC:			xavix.pc.w.l = info->i;					break;
		case CPUINFO_INT_SP:							S = info->i;							break;
		case CPUINFO_INT_REGISTER + XAVIX_S:			xavix.sp.b.l = info->i;					break;
		case CPUINFO_INT_REGISTER + XAVIX_P:			xavix.p = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_A:			xavix.a = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_X:			xavix.x = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_Y:			xavix.y = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_DATABANK:		xavix.databank = info->i;				break;
		case CPUINFO_INT_REGISTER + XAVIX_CODEBANK:		xavix.codebank = info->i;				break;
		case CPUINFO_INT_REGISTER + XAVIX_J:			xavix.j = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_K:			xavix.k = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_L:			xavix.l = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_M:			xavix.m = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_PA:			xavix.pa = info->i;						break;
		case CPUINFO_INT_REGISTER + XAVIX_PB:			xavix.pb = info->i;						break;
	}
}

#ifdef MAME_DEBUG
static unsigned xavix_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes)
{
	sprintf(buffer, "db $%02X", oprom[0]);
	return 1;
}
#endif

void xavix2000_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(xavix);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 2;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 8;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 24;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 15;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + XAVIX_IRQ_LINE:	info->i = xavix.irq_state;				break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = xavix.nmi_state;				break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = xavix.ppc.w.l;				break;

		case CPUINFO_INT_PC:							info->i = PCD;							break;
		case CPUINFO_INT_REGISTER + XAVIX_PC:			info->i = xavix.pc.w.l;					break;
		case CPUINFO_INT_SP:							info->i = S;							break;
		case CPUINFO_INT_REGISTER + XAVIX_S:			info->i = xavix.sp.b.l;					break;
		case CPUINFO_INT_REGISTER + XAVIX_P:			info->i = xavix.p;						break;
		case CPUINFO_INT_REGISTER + XAVIX_A:			info->i = xavix.a;						break;
		case CPUINFO_INT_REGISTER + XAVIX_X:			info->i = xavix.x;						break;
		case CPUINFO_INT_REGISTER + XAVIX_Y:			info->i = xavix.y;						break;
		case CPUINFO_INT_REGISTER + XAVIX_DATABANK:		info->i = xavix.databank;				break;
		case CPUINFO_INT_REGISTER + XAVIX_CODEBANK:		info->i = xavix.codebank;				break;
		case CPUINFO_INT_REGISTER + XAVIX_J:			info->i = xavix.j;						break;
		case CPUINFO_INT_REGISTER + XAVIX_K:			info->i = xavix.k;						break;
		case CPUINFO_INT_REGISTER + XAVIX_L:			info->i = xavix.l;						break;
		case CPUINFO_INT_REGISTER + XAVIX_M:			info->i = xavix.m;						break;
		case CPUINFO_INT_REGISTER + XAVIX_PA:			info->i = xavix.pa;						break;
		case CPUINFO_INT_REGISTER + XAVIX_PB:			info->i = xavix.pb;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = xavix_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = xavix_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = xavix_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = xavix_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = xavix_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = xavix_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = xavix_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = xavix_dasm;		break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &xavix_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = xavix_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = xavix_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "XaviX 2000"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "SSD XaviX"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) David Haywood"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
				xavix.p & 0x80 ? 'N':'.',
				xavix.p & 0x40 ? 'V':'.',
				xavix.p & 0x20 ? 'R':'.',
				xavix.p & 0x10 ? 'B':'.',
				xavix.p & 0x08 ? 'D':'.',
				xavix.p & 0x04 ? 'I':'.',
				xavix.p & 0x02 ? 'Z':'.',
				xavix.p & 0x01 ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + XAVIX_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", xavix.pc.w.l); break;
		case CPUINFO_STR_REGISTER + XAVIX_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%02X", xavix.sp.b.l); break;
		case CPUINFO_STR_REGISTER + XAVIX_P:			sprintf(info->s = cpuintrf_temp_str(), "P:%02X", xavix.p); break;
		case CPUINFO_STR_REGISTER + XAVIX_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", xavix.a); break;
		case CPUINFO_STR_REGISTER + XAVIX_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%02X", xavix.x); break;
		case CPUINFO_STR_REGISTER + XAVIX_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%02X", xavix.y); break;
		case CPUINFO_STR_REGISTER + XAVIX_DATABANK:		sprintf(info->s = cpuintrf_temp_str(), "DATBNK:%02X", xavix.databank); break;
		case CPUINFO_STR_REGISTER + XAVIX_CODEBANK:		sprintf(info->s = cpuintrf_temp_str(), "CODBNK:%02X", xavix.codebank); break;
		case CPUINFO_STR_REGISTER + XAVIX_J:			sprintf(info->s = cpuintrf_temp_str(), "J:%02X", xavix.j); break;
		case CPUINFO_STR_REGISTER + XAVIX_K:			sprintf(info->s = cpuintrf_temp_str(), "K:%02X", xavix.k); break;
		case CPUINFO_STR_REGISTER + XAVIX_L:			sprintf(info->s = cpuintrf_temp_str(), "L:%02X", xavix.l); break;
		case CPUINFO_STR_REGISTER + XAVIX_M:			sprintf(info->s = cpuintrf_temp_str(), "M:%02X", xavix.m); break;
		case CPUINFO_STR_REGISTER + XAVIX_PA:			sprintf(info->s = cpuintrf_temp_str(), "PA:%06X", xavix.pa); break;
		case CPUINFO_STR_REGISTER + XAVIX_PB:			sprintf(info->s = cpuintrf_temp_str(), "PB:%06X", xavix.pb); break;
	}
}

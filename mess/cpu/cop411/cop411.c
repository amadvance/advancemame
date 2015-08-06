/**************************************************************************
 *               National Semiconductor COP411  Emulator                  *
 *                                                                        *
 *                   Copyright (C) 2005 by Dan Boris                      *
 **************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cpuintrf.h"
#include "state.h"
#include "debugger.h"
#include "cop411.h"

/* The opcode table now is a combination of cycle counts and function pointers */
typedef struct {
	unsigned cycles;
	void (*function) (void);
}	s_opcode;



/* Layout of the registers in the debugger */
static UINT8 cop411_reg_layout[] = {
	   COP411_PC, COP411_A, COP411_B, COP411_C, COP411_EN, COP411_Q,
       COP411_SA, COP411_SB, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 cop411_win_layout[] = {
	 0, 0,80, 2,	/* register window (top rows) */
	 0, 3,24,19,	/* disassembler window (left colums) */
	25, 3,55, 9,	/* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};


#define M_RDMEM(A)		COP411_RDMEM(A)
#define M_RDOP(A)		COP411_RDOP(A)
#define M_RDOP_ARG(A)	COP411_RDOP_ARG(A)
#define M_IN(A)			COP411_In(A)
#define M_OUT(A,V)		COP411_Out(A,V)

#define READ_G()		COP411_In(COP411_G)
#define READ_L()		COP411_In(COP411_L)
#define WRITE_G(A)		COP411_Out(COP411_G,A)
#define WRITE_D(A)		COP411_Out(COP411_D,A)


typedef struct
{
	PAIR 	PC;				/* program counter */
	PAIR    PREVPC;
	UINT8	A, B, C, EN, Q;
	UINT16  SA, SB;
	UINT8   skip, skipLBI;
	UINT8	RAM[32];

} COP411_Regs;

static COP411_Regs R;
static int    inst_cycles;
static int    cop411_ICount;
static long	  cop411_masterclock;

static int InstLen[256];
static int LBIops[256];


#define intRAM	R.RAM


static s_opcode opcode_op33[256];

static offs_t cop411_dasm(char *buffer, offs_t pc)
{
#ifdef	MAME_DEBUG
	return DasmCOP411(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

/* Get next opcode argument and increment program counter */
INLINE unsigned M_RDMEM_OPCODE (void)
{
	unsigned retval;
	retval=M_RDOP_ARG(R.PC.w.l);
	R.PC.w.l++;
	return retval;
}

INLINE void M_ILLEGAL(void)
{
	logerror("ICOP411:  PC = %04x,  Illegal opcode = %02x\n", R.PC.w.l-1, M_RDMEM(R.PC.w.l-1));
}

static void illegal(void)	 { M_ILLEGAL(); }

static void writeQ(UINT8 dat) {

}

static void PUSH(UINT16 addr)
{
	R.SB = R.SA;
	R.SA = addr;
}

static UINT16 PULL(void)
{
	UINT16 addr = R.SA;
	R.SA = R.SB;
	return(addr);
}

static void XIS(UINT8 r)
{
	UINT8 t, Bd, Br;
	t = M_IN(R.B);
	M_OUT(R.B,R.A);
	R.A = t;
	Br = (UINT8)((R.B & 0x30) ^ (r << 4));
	Bd = (UINT8)((R.B & 0x0F) + 1);
	R.B = (UINT8)(Br | (Bd & 0x0F));
	if (Bd == 0x10) R.skip = 1;
}

static void AISC(int n)
{
	R.A = (UINT8)(R.A + n);
	if (R.A > 15) {R.skip = 1; R.A &= 0x0F; }
}

static void JMP(UINT8 a8)
{
	R.PC.w.l =  (a8 << 8) | M_RDOP(R.PC.w.l);
}

static void jp(void)
{
	UINT8 op = M_RDOP(R.PREVPC.w.l);

	if (((R.PC.w.l & 0x3E0) >= 0x80) && ((R.PC.w.l & 0x3E0) < 0x100)) //JP pages 2,3
	{
		R.PC.w.l = (UINT16)((R.PC.w.l & 0x380) | (op & 0x7F));
	}
	else
	{
		if ((op & 0xC0) == 0xC0) //JP other pages
		{
			R.PC.w.l = (UINT16)((R.PC.w.l & 0x3C0) | (op & 0x3F));
		}
		else					//JSRP
		{
			PUSH((UINT16)(R.PC.w.l));
			R.PC.w.l = (UINT16)(0x80 | (op & 0x3F));
		}
	}
}

static void JSR(UINT8 a8)
{
	UINT16 tPC = (UINT16)(R.PC.w.l + 1);
	R.PC.w.l = (UINT16)((a8 << 8) | M_RDOP(R.PC.w.l));
	PUSH(tPC);
}

static void LD(UINT8 r)
{
	R.A = M_IN(R.B);
	R.B = (UINT8)(R.B ^ (r << 4));
}

static void STII(UINT8 y)
{
	UINT16 Bd;

	M_OUT(R.B,y);
	Bd = (R.B & 0x0f) + 1;
	if (Bd > 15) Bd = 0;
	R.B = (R.B & 0x30) + Bd;
}

static void X(UINT8 r)
{
	UINT8 t;

	t = M_IN(R.B);
	M_OUT(R.B,R.A);
	R.A = t;
	R.B = (UINT8)(R.B ^ (r << 4));
}

static void XDS(UINT8 r)
{
	UINT8 t, Bd, Br;

	t = M_IN(R.B);
	M_OUT(R.B,R.A);
	R.A = t;
	Br = (UINT8)((R.B & 0x30) ^ (r << 4));
	Bd = (UINT8)((R.B & 0x0F) - 1);
	R.B = (UINT8)(Br | (Bd & 0x0F));
	if (Bd == 255) R.skip = 1;
}


static void LBI(UINT8 r, UINT8 d)
{
	R.B = (UINT8)((r << 4) | d);
	R.skipLBI = 1;
}

static void clra(void){ R.A = 0;  }
static void skmbz0(void) { if ((M_IN(R.B) & 0x01) == 0 ) R.skip = 1; }
static void xor(void) { R.A = (UINT8)(M_IN(R.B) ^ R.A); }
static void skmbz2(void) { if ((M_IN(R.B) & 0x04) == 0) R.skip = 1; }
static void xis0(void) { XIS(0); }
static void ld0(void) { LD(0); }
static void x0(void) {X(0); }
static void xds0(void) {XDS(0); }

static void lbi0_9(void) {LBI(0,9); }
static void lbi0_10(void) {LBI(0,10); }
static void lbi0_11(void) {LBI(0,11); }
static void lbi0_12(void) {LBI(0,12); }
static void lbi0_13(void) {LBI(0,13); }
static void lbi0_14(void) {LBI(0,14); }
static void lbi0_15(void) {LBI(0,15); }
static void lbi0_0(void) {LBI(0,0); }

static void skmbz1(void) {if ((M_IN(R.B) & 0x02) == 0 ) R.skip = 1; }
static void skmbz3(void) {if ((M_IN(R.B) & 0x08) == 0 ) R.skip = 1; }
static void xis1(void) {XIS(1);}
static void ld1(void) {LD(1); }
static void x1(void) {X(1); }
static void xds1(void) {XDS(1); }

static void lbi1_9(void) {LBI(1,9); }
static void lbi1_10(void) {LBI(1,10); }
static void lbi1_11(void) {LBI(1,11); }
static void lbi1_12(void) {LBI(1,12); }
static void lbi1_13(void) {LBI(1,13); }
static void lbi1_14(void) {LBI(1,14); }
static void lbi1_15(void) {LBI(1,15); }
static void lbi1_0(void) {LBI(1,0); }

static void skc(void) {if (R.C == 1) R.skip = 1; }
static void ske(void) {if (R.A == M_IN(R.B)) R.skip = 1; }
static void sc(void) { R.C = 1;  }

static void xad(void)
{
	UINT8 t;

	R.PC.w.l++; //Skip the second byte, not significant on in the COP410
	t = R.A;
	R.A = M_IN(0x3F);
	M_OUT(0x3F,t);
}


static void xis2(void) {XIS(2);}
static void ld2(void) {LD(2); }
static void x2(void) {X(2); }
static void xds2(void) {XDS(2); }

static void lbi2_9(void) {LBI(2,9); }
static void lbi2_10(void) {LBI(2,10); }
static void lbi2_11(void) {LBI(2,11); }
static void lbi2_12(void) {LBI(2,12); }
static void lbi2_13(void) {LBI(2,13); }
static void lbi2_14(void) {LBI(2,14); }
static void lbi2_15(void) {LBI(2,15); }
static void lbi2_0(void) {LBI(2,0); }


static void asc(void)
{
	R.A = (UINT8)(R.A + R.C + M_IN(R.B));
	if (R.A > 15)
	{
		R.C = 1;
		R.skip = 1;
		R.A = (UINT8)(R.A & 0x0F);
	}
	else
	{
		R.C = 0;
	}
}


static void add(void) { R.A = (UINT8)((R.A + M_IN(R.B)) & 0x0F); }

static void rc(void) { R.C = 0; }

//OP33xx
static void op33(void)
{
	(*(opcode_op33[M_RDOP(R.PC.w.l++)].function))();
}

static void skgbz0(void) {if ((READ_G() & 0x01) == 0) R.skip = 1; }

static void skgz(void) { if (READ_G() == 0) R.skip = 1; }

static void skgbz1(void) {if ((READ_G() & 0x02) == 0) R.skip = 1; }

static void skgbz2(void) { if ((READ_G() & 0x04) == 0) R.skip = 1; }

static void skgbz3(void) {if ((READ_G() & 0x08) == 0) R.skip = 1;  }

static void ing(void) { R.A = READ_G();  }

static void inl(void)
{
	UINT8 L = READ_L();
	M_OUT(R.B,L>>4);
	R.A = (UINT8)(L & 0xF);
}

static void camq(void) { writeQ((UINT8)((R.A << 4) | (M_IN(R.B) & 0x0F))); }

static void obd(void)  {
	WRITE_D((UINT8)(R.B & 0x0F));
	}

static void omg(void)  {
	WRITE_G(M_IN(R.B));

	}

static void lei0(void) { R.EN = 0;}
static void lei1(void) { R.EN = 1;}
static void lei2(void) { R.EN = 2;}
static void lei3(void) { R.EN = 3;}
static void lei4(void) { R.EN = 4;}
static void lei5(void) { R.EN = 5;}
static void lei6(void) { R.EN = 6;}
static void lei7(void) { R.EN = 7;}
static void lei8(void) { R.EN = 8;}
static void lei9(void) { R.EN = 9;}
static void lei10(void) { R.EN = 10;}
static void lei11(void) { R.EN = 11;}
static void lei12(void) { R.EN = 12;}
static void lei13(void) { R.EN = 13;}
static void lei14(void) { R.EN = 14;}
static void lei15(void) { R.EN = 15;}


static void xis3(void) {XIS(3);}

static void ld3(void) {LD(3); }

static void x3(void) {X(3); }

static void xds3(void) {XDS(3); }

static void lbi3_9(void) {LBI(3,9); }
static void lbi3_10(void) {LBI(3,10); }
static void lbi3_11(void) {LBI(3,11); }
static void lbi3_12(void) {LBI(3,12); }
static void lbi3_13(void) {LBI(3,13); }
static void lbi3_14(void) {LBI(3,14); }
static void lbi3_15(void) {LBI(3,15); }
static void lbi3_0(void) {LBI(3,0); }

static void comp(void) { R.A = (UINT8)(R.A ^ 0x0F);}

static void rmb2(void) { M_OUT(R.B,M_IN(R.B) & 0xB); }
static void rmb3(void) { M_OUT(R.B,M_IN(R.B) & 0x7); }

static void nop(void) {  }

static void rmb1(void) { M_OUT(R.B,M_IN(R.B) & 0xD);  }
static void smb2(void) { M_OUT(R.B,M_IN(R.B) | 0x4); }
static void smb1(void) { M_OUT(R.B,M_IN(R.B) | 0x2); }

static void ret(void) {R.PC.w.l = PULL(); }
static void retsk(void) {R.PC.w.l = PULL();  R.skip = 1; }

static void smb3(void) { M_OUT(R.B,M_IN(R.B) | 0x8); }
static void rmb0(void) { M_OUT(R.B,M_IN(R.B) & 0xE);}
static void smb0(void) { M_OUT(R.B,M_IN(R.B) | 0x1); }

static void cba(void) { R.A = (UINT8)(R.B & 0x0F); }
static void xas(void) { }

static void cab(void) {R.B = (UINT8)((R.B & 0x30) | R.A);  }

static void lqid(void) {
	UINT16 addr = (UINT16)((R.PC.w.l & 0x100) | (R.A << 4) | M_IN(R.B));
	writeQ(M_RDOP(addr));
	PUSH(R.PC.w.l);
	R.PC.w.l = PULL();
}

static void jid(void)
{
	UINT16 addr = (UINT16)((R.PC.w.l & 0x100) | (R.A << 4) | M_IN(R.B));
	R.PC.w.l = (UINT16)((R.PC.w.l & 0x100) | M_RDOP(addr));
}

//ASIC
static void aisc1(void) { AISC(0x1);}
static void aisc2(void) { AISC(0x2);}
static void aisc3(void) { AISC(0x3);}
static void aisc4(void) { AISC(0x4);}
static void aisc5(void) { AISC(0x5);}
static void aisc6(void) { AISC(0x6);}
static void aisc7(void) { AISC(0x7);}
static void aisc8(void) { AISC(0x8);}
static void aisc9(void) { AISC(0x9);}
static void aisc10(void) { AISC(0xA);}
static void aisc11(void) { AISC(0xB);}
static void aisc12(void) { AISC(0xC);}
static void aisc13(void) { AISC(0xD);}
static void aisc14(void) { AISC(0xE);}
static void aisc15(void) { AISC(0xF);}

static void jmp0(void) { JMP(0); }
static void jmp1(void) { JMP(1); }

static void jsr0(void) {JSR(0); }
static void jsr1(void) {JSR(1); }

//STII
static void stii0(void) {STII(0x0); }
static void stii1(void) {STII(0x1); }
static void stii2(void) {STII(0x2); }
static void stii3(void) {STII(0x3); }

static void stii4(void) {STII(0x4); }
static void stii5(void) {STII(0x5); }
static void stii6(void) {STII(0x6); }
static void stii7(void) {STII(0x7); }

static void stii8(void) {STII(0x8); }
static void stii9(void) {STII(0x9); }
static void stii10(void) {STII(0xA); }
static void stii11(void) {STII(0xB); }

static void stii12(void) {STII(0xC); }
static void stii13(void) {STII(0xD); }
static void stii14(void) {STII(0xE); }
static void stii15(void) {STII(0xF); }

static s_opcode opcode_op33[256]=
{
	{1, illegal 	},{1, skgbz0	},{1, illegal   },{1, skgbz2	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, skgbz1 	},{1, illegal 	},{1, skgbz3 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, skgz 		},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, ing	 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, inl	 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, omg	 	},{1, illegal 	},{1, camq	 	},{1, illegal 	},{1, obd	 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, lei0 		},{1, lei1 		},{1, lei2 		},{1, lei3 		},{1, lei4 		},{1, lei5 		},{1, lei6 		},{1, lei7 		},
	{1, lei8 		},{1, lei9 		},{1, lei10 	},{1, lei11 	},{1, lei12 	},{1, lei13 	},{1, lei14 	},{1, lei15 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	}

};

static s_opcode opcode_main[256]=
{
	{1, clra		},{1, skmbz0	},{1, xor		},{1, skmbz2		},{1, xis0		},{1, ld0		},{1, x0		},{1, xds0		},
	{1, lbi0_9		},{1, lbi0_10	},{1, lbi0_11	},{1, lbi0_12		},{1, lbi0_13	},{1, lbi0_14	},{1, lbi0_15	},{1, lbi0_0	},
	{0, illegal		},{1, skmbz1	},{0, illegal	},{1, skmbz3		},{1, xis0		},{1, ld1		},{1, x1		},{1, xds1		},
	{1, lbi1_9		},{1, lbi1_10	},{1, lbi1_11	},{1, lbi1_12		},{1, lbi1_13	},{1, lbi1_14	},{1, lbi1_15	},{1, lbi1_0	},
	{1, skc			},{1, ske		},{1, sc		},{2, xad			},{1, xis2		},{1, ld2		},{1, x2		},{1, xds2 		},
	{1,	lbi2_9		},{1, lbi2_10	},{1, lbi2_11	},{1, lbi2_12		},{1, lbi2_13	},{1, lbi2_14	},{1, lbi2_15	},{1, lbi2_0	},
	{1, asc			},{1, add		},{1, rc		},{2, op33   		},{1, xis3		},{1, ld3		},{1, x3		},{1, xds3		},
	{1,	lbi3_9		},{1, lbi3_10	},{1, lbi3_11	},{1, lbi3_12		},{1, lbi3_13	},{1, lbi3_14	},{1, lbi3_15	},{1, lbi3_0	},
	{1, comp		},{0, illegal	},{1, rmb2		},{1, rmb2			},{1, nop		},{1, rmb1		},{1, smb2		},{1, smb1		},
	{1,	ret			},{1, retsk		},{0, illegal	},{1, smb3			},{1, rmb0		},{1, smb0		},{1, cba		},{1, xas		},
	{1, cab			},{1, aisc1		},{1, aisc2		},{1, aisc3			},{1, aisc4		},{1, aisc5		},{1, aisc6		},{1, aisc7		},
	{1, aisc8		},{1, aisc9		},{1, aisc10	},{1, aisc11		},{1, aisc12	},{1, aisc13	},{1, aisc14	},{1, aisc15	},
	{2, jmp0		},{2, jmp1		},{0, illegal	},{0, illegal		},{0, illegal	},{0, illegal	},{0, illegal	},{0, illegal   },
	{2, jsr0		},{2, jsr1		},{0, illegal	},{0, illegal		},{0, illegal	},{0, illegal	},{0, illegal	},{0, illegal	},
	{1, stii0		},{1, stii1		},{1, stii2		},{1, stii3			},{1, stii4		},{1, stii5		},{1, stii6		},{1, stii7		},
	{1, stii8		},{1, stii9		},{1, stii10	},{1, stii11		},{1, stii12	},{1, stii13	},{1, stii14	},{1, stii15	},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, lqid		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jid		}
};


/****************************************************************************
 * Initialize emulation
 ****************************************************************************/
static void cop411_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	int i;

	for (i=0; i<256; i++) InstLen[i]=1;
	InstLen[0x60] = InstLen[0x61] = InstLen[0x68] = InstLen[0x69] = InstLen[0x33] = InstLen[0x23] = 2;

	for (i=0; i<256; i++) LBIops[i] = 0;
	for (i=0x08; i<0x10; i++) LBIops[i] = 1;
	for (i=0x18; i<0x20; i++) LBIops[i] = 1;
	for (i=0x28; i<0x30; i++) LBIops[i] = 1;
	for (i=0x38; i<0x40; i++) LBIops[i] = 1;

}

/****************************************************************************
 * Reset registers to their initial values
 ****************************************************************************/
static void cop411_reset(void)
{
	R.PC.d =0;
	R.A=0;
	R.B=0;
	R.C=0;
	WRITE_D(0);
	R.EN=0;
	WRITE_G(0);

	/* TEMP */
/*	M_OUT(0x20,0x0F);*/
	cop411_masterclock=0;
}


/****************************************************************************
 * Execute cycles CPU cycles. Return number of cycles really executed
 ****************************************************************************/
static int cop411_execute(int cycles)
{
	unsigned opcode;

	cop411_ICount = cycles;
	do
	{
		R.PREVPC = R.PC;

		CALL_MAME_DEBUG;

		opcode=M_RDOP(R.PC.w.l);

		if (R.skipLBI == 1)
		{
			if (LBIops[opcode] == 0)
			{
				R.skipLBI = 0;
			}
			else {
				cop411_ICount -=opcode_main[opcode].cycles;

				cop411_masterclock +=opcode_main[opcode].cycles;

				R.PC.w.l += InstLen[opcode];
			}

		}

		if (R.skipLBI == 0)
		{
			R.PC.w.l++;
			inst_cycles = opcode_main[opcode].cycles;
			(*(opcode_main[opcode].function))();
			cop411_ICount -= inst_cycles;
			cop411_masterclock += inst_cycles;

			if (R.skip == 1) {
				opcode=M_RDOP(R.PC.w.l);
				cop411_ICount -=opcode_main[opcode].cycles;
				cop411_masterclock+=opcode_main[opcode].cycles;
				R.PC.w.l += InstLen[opcode];
				R.skip = 0;
			}
		}
	} while (cop411_ICount>0);


	return cycles - cop411_ICount;
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
static void cop411_get_context (void *dst)
{
	if( dst )
		*(COP411_Regs*)dst = R;
}


/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
static void cop411_set_context (void *src)
{
	if( src )
	{
		R = *(COP411_Regs*)src;

		#ifdef MESS
			change_pc(R.PC.w.l);
		#endif
	}
}



/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void cop411_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + COP411_PC:			R.PC.w.l = info->i;						break;

	    case CPUINFO_INT_REGISTER + COP411_A:			R.A = info->i;						break;
		case CPUINFO_INT_REGISTER + COP411_B:			R.B = info->i;							break;
		case CPUINFO_INT_REGISTER + COP411_C:			R.C = info->i;						break;
		case CPUINFO_INT_REGISTER + COP411_EN:			R.EN = info->i;							break;
		case CPUINFO_INT_REGISTER + COP411_Q:			R.Q = info->i;							break;
		case CPUINFO_INT_REGISTER + COP411_SA:			R.SA = info->i;							break;
		case CPUINFO_INT_REGISTER + COP411_SB:			R.SB = info->i;							break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void cop411_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(R);					break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 0;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = COP411_CLOCK_DIVIDER;			break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 2;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 9;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 4;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 6;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 9;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = 0;						    break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = R.PREVPC.w.l;					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + COP411_PC:			info->i = R.PC.w.l;						break;
		case CPUINFO_INT_REGISTER + COP411_A:			info->i = R.A;							break;
		case CPUINFO_INT_REGISTER + COP411_B:			info->i = R.B;							break;
		case CPUINFO_INT_REGISTER + COP411_C:			info->i = R.C;							break;
		case CPUINFO_INT_REGISTER + COP411_EN:			info->i = R.EN;							break;
		case CPUINFO_INT_REGISTER + COP411_Q:			info->i = R.Q;							break;
		case CPUINFO_INT_REGISTER + COP411_SA:			info->i = R.SA;							break;
		case CPUINFO_INT_REGISTER + COP411_SB:			info->i = R.SB;							break;


		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = cop411_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = cop411_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = cop411_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = cop411_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = cop411_reset;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = cop411_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = cop411_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &cop411_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = cop411_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = cop411_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "cop411"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "National Semiconductor COP"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2005 by Dan Boris"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), " ");
			break;

		case CPUINFO_STR_REGISTER +  COP411_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", R.PC.w.l); break;
		case CPUINFO_STR_REGISTER +  COP411_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", R.A ); break;
		case CPUINFO_STR_REGISTER +  COP411_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", R.B ); break;
		case CPUINFO_STR_REGISTER +  COP411_C:			sprintf(info->s = cpuintrf_temp_str(), "C:%02X", R.C); break;
		case CPUINFO_STR_REGISTER +  COP411_EN:			sprintf(info->s = cpuintrf_temp_str(), "EN:%02X", R.EN); break;
		case CPUINFO_STR_REGISTER +  COP411_Q:			sprintf(info->s = cpuintrf_temp_str(), "Q:%02X", R.Q); break;
		case CPUINFO_STR_REGISTER +  COP411_SA:			sprintf(info->s = cpuintrf_temp_str(), "SA:%04X", R.SA); break;
		case CPUINFO_STR_REGISTER +  COP411_SB:			sprintf(info->s = cpuintrf_temp_str(), "SB:%04X", R.SB); break;
	}
}




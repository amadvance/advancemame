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
 * based on info found on an artikel for the tandy trs80 pc2
 *
 *****************************************************************************/
#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "debugger.h"

#include "lh5801.h"

//typedef int bool;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

enum {
	LH5801_T=1,
	LH5801_P,
	LH5801_S,
	LH5801_U,
	LH5801_X,
	LH5801_Y,
	LH5801_A,

	LH5801_TM,
	LH5801_IN,
	LH5801_BF,
	LH5801_PU,
	LH5801_PV,
	LH5801_DP,
	LH5801_IRQ_STATE
};
/* Layout of the registers in the debugger */
static UINT8 lh5801_reg_layout[] = {
	LH5801_P,
	LH5801_S,
	LH5801_U,
	LH5801_X,
	LH5801_Y,
	LH5801_A,
	LH5801_T,
	-1,
	LH5801_TM,
	LH5801_IN,
	LH5801_BF,
	LH5801_PU,
	LH5801_PV,
	LH5801_DP,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 lh5801_win_layout[] = {
	22, 0,58, 2,	/* register window (top, right rows) */
	 0, 0,21,22,	/* disassembler window (left colums) */
	22, 3,58, 9,	/* memory #1 window (right, upper middle) */
	22,13,58, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

typedef struct
{
	LH5801_CONFIG *config;

	PAIR s, p, u, x, y;
	int tm; //9 bit

	UINT8 t, a;

	bool bf, dp, pu, pv;

	UINT16 oldpc;

	bool irq_state;

	bool idle;
} LH5801_Regs;

int lh5801_icount = 0;

static LH5801_Regs lh5801= { 0 };

#define P lh5801.p.w.l
#define S lh5801.s.w.l
#define U lh5801.u.w.l
#define UL lh5801.u.b.l
#define UH lh5801.u.b.h
#define X lh5801.x.w.l
#define XL lh5801.x.b.l
#define XH lh5801.x.b.h
#define Y lh5801.y.w.l
#define YL lh5801.y.b.l
#define YH lh5801.y.b.h

#define C 0x01
#define IE 0x02
#define Z 0x04
#define V 0x08
#define H 0x10

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "5801tbl.c"

void lh5801_init(void)
{
	int cpu = cpu_getactivecpu();
	state_save_register_item("sc61860", cpu, sc61860.pc);
	state_save_register_item("sc61860", cpu, sc61860.dp);
	state_save_register_item("sc61860", cpu, sc61860.p);
	state_save_register_item("sc61860", cpu, sc61860.q);
	state_save_register_item("sc61860", cpu, sc61860.r);
	state_save_register_item("sc61860", cpu, sc61860.carry);
	state_save_register_item("sc61860", cpu, sc61860.zero);
}

void lh5801_reset(void *param)
{
	if (param) {
		lh5801.config=(LH5801_CONFIG *)param;
	}
	P=(cpu_readmem17(0xfffe)<<8)|cpu_readmem17(0xffff);

	change_pc17(P);

	lh5801.idle=0;
}

void lh5801_exit(void)
{
	/* nothing to do yet */
}

unsigned lh5801_get_context (void *dst)
{
	if( dst )
		*(LH5801_Regs*)dst = lh5801;
	return sizeof(LH5801_Regs);
}

void lh5801_set_context (void *src)
{
	if( src )
	{
		lh5801 = *(LH5801_Regs*)src;
		change_pc17(P);
	}
}

unsigned lh5801_get_reg (int regnum)
{
	switch( regnum )
	{
	case REG_PC:
	case LH5801_P: return P;
	case REG_SP:
	case LH5801_S: return S;
	case LH5801_U: return U;
	case LH5801_X: return X;
	case LH5801_Y: return Y;
	case LH5801_T: return lh5801.t;
	case LH5801_TM: return lh5801.tm;
	case LH5801_IN:
		if (lh5801.config&&lh5801.config->in) return lh5801.config->in();
		return 0;
	case LH5801_BF: return lh5801.bf;
	case LH5801_PV: return lh5801.pv;
	case LH5801_PU: return lh5801.pu;
	case LH5801_DP: return lh5801.dp;
	case REG_PREVIOUSPC: return lh5801.oldpc;
	case LH5801_IRQ_STATE: return lh5801.irq_state;
	}
	return 0;
}

void lh5801_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case REG_PC:
	case LH5801_P: P=val;change_pc17(P);break;
	case REG_SP:
	case LH5801_S: S=val;break;
	case LH5801_U: U=val;break;
	case LH5801_X: X=val;break;
	case LH5801_Y: Y=val;break;
	case LH5801_T: lh5801.t=val;break;
	case LH5801_TM: lh5801.tm=val;break;
	case LH5801_IN: break; //inputport!
	case LH5801_PV: lh5801.pv=val;break;
	case LH5801_PU: lh5801.pu=val;break;
	case LH5801_BF: lh5801.bf=val;break;
	case LH5801_DP: lh5801.dp=val;break;
	case REG_PREVIOUSPC: lh5801.oldpc=val;break;
	case LH5801_IRQ_STATE: lh5801.irq_state=val;break;
	}
}

#if 0
INLINE void lh5801_take_irq(void)
{
}
#endif

int lh5801_execute(int cycles)
{
	lh5801_icount = cycles;

	change_pc17(P);

	if (lh5801.idle) {
		lh5801_icount=0;
	} else {
		do
		{
			lh5801.oldpc = P;

			CALL_MAME_DEBUG;
			lh5801_instruction();

		} while (lh5801_icount > 0);
	}

	return cycles - lh5801_icount;
}

void lh5801_set_nmi_line(int state)
{
}

void lh5801_set_irq_line(int irqline, int state)
{
	lh5801.idle=0;
#if 0
	if (cdp1802.ie) {
		cdp1802.ie=0;
		cdp1802.t=(cdp1802.x<<4)|cdp1802.p;
		cdp1802.p=1;
		cdp1802.x=2;
		change_pc17(P);
	}
#endif
}

void lh5801_set_irq_callback(int (*callback)(int))
{
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *lh5801_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
    LH5801_Regs *r = context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &lh5801;

	switch( regnum )
	{
	case CPU_INFO_REG+LH5801_P: sprintf(buffer[which],"P:%.4x",r->p.w.l);break;
	case CPU_INFO_REG+LH5801_S: sprintf(buffer[which],"S:%.4x",r->s.w.l);break;
	case CPU_INFO_REG+LH5801_U: sprintf(buffer[which],"U:%.4x",r->u.w.l);break;
	case CPU_INFO_REG+LH5801_X: sprintf(buffer[which],"X:%.4x",r->x.w.l);break;
	case CPU_INFO_REG+LH5801_Y: sprintf(buffer[which],"Y:%.4x",r->y.w.l);break;
	case CPU_INFO_REG+LH5801_T: sprintf(buffer[which],"T:%.2x",r->t);break;
	case CPU_INFO_REG+LH5801_A: sprintf(buffer[which],"A:%.2x",r->a);break;
	case CPU_INFO_REG+LH5801_TM: sprintf(buffer[which],"TM:%.3x",r->tm);break;
	case CPU_INFO_REG+LH5801_IN: sprintf(buffer[which],"IN:%.2x",lh5801.config->in());break;
	case CPU_INFO_REG+LH5801_PV: sprintf(buffer[which],"PV:%x",r->pv);break;
	case CPU_INFO_REG+LH5801_PU: sprintf(buffer[which],"PU:%x",r->pu);break;
	case CPU_INFO_REG+LH5801_BF: sprintf(buffer[which],"BF:%x",r->bf);break;
	case CPU_INFO_REG+LH5801_DP: sprintf(buffer[which],"DP:%x",r->dp);break;
	case CPU_INFO_FLAGS: sprintf(buffer[which], "%s%s%s%s%s%s%s%s",
								 r->t&0x80?"1":"0",
								 r->t&0x40?"1":"0",
								 r->t&0x20?"1":"0",
								 r->t&0x10?"H":".",
								 r->t&8?"V":".",
								 r->t&4?"Z":".",
								 r->t&2?"I":".",
								 r->t&1?"C":".");
							 break;
	case CPU_INFO_NAME: return "LH5801";
	case CPU_INFO_FAMILY: return "LH5801";
	case CPU_INFO_VERSION: return "1.0alpha";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Copyright (c) 2000 Peter Trauner, all rights reserved.";
	case CPU_INFO_REG_LAYOUT: return (const char*)lh5801_reg_layout;
	case CPU_INFO_WIN_LAYOUT: return (const char*)lh5801_win_layout;
	}
	return buffer[which];
}

#ifndef MAME_DEBUG
unsigned lh5801_dasm(char *buffer, unsigned pc)
{
	sprintf( buffer, "$%X", cpu_readop(pc) );
	return 1;
}
#endif

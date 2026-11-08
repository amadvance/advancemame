// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    m6809.h

    Portable 6809 emulator

*********************************************************************/

#ifndef _M6809_H
#define _M6809_H

#include <stdbool.h>
#include "cpuintrf.h"

enum
{
	M6809_PC = 1,
	M6809_S,
	M6809_CC,
	M6809_A,
	M6809_B,
	M6809_U,
	M6809_X,
	M6809_Y,
	M6809_DP
};

#define M6809_IRQ_LINE	0	/* IRQ line number */
#define M6809_FIRQ_LINE 1   /* FIRQ line number */

#ifndef PAIR16_DEFINED
#define PAIR16_DEFINED
typedef union
{
#ifdef LSB_FIRST
	struct { UINT8 l, h; } b;
#else
	struct { UINT8 h, l; } b;
#endif
	UINT16 w;
} PAIR16;
#endif

typedef struct
{
	UINT8 byte_value;
	UINT16 word_value;
} exgtfr_register;

typedef struct _m6809_Regs m6809_Regs;
struct _m6809_Regs
{
	PAIR16	pc;
	PAIR16	ppc;
	PAIR16	d;
	PAIR16	u;
	PAIR16	s;
	PAIR16	x;
	PAIR16	y;
	UINT8	dp;
	UINT8	cc;
	PAIR16	temp;
	UINT8	opcode;
	PAIR16	ea;
	int		addressing_mode;
	int		reg;
	int		cond;
	UINT32	state;
	int		nmi_asserted;
	int		nmi_line;
	int		firq_line;
	int		irq_line;
	int		lds_encountered;

	UINT8*	reg8;
	PAIR16*	reg16;

	int (*irq_callback)(int irqline);
};

void m6809_get_info(UINT32 state, union cpuinfo *info);
void m6809e_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern offs_t m6809_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes);
#endif

#endif /* _M6809_H */

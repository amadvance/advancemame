// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    konami.h

    Portable Konami CPU emulator

*********************************************************************/

#ifndef _KONAMI_H
#define _KONAMI_H

#include <stdbool.h>
#include "cpuintrf.h"

enum
{
	KONAMI_PC = 1,
	KONAMI_S,
	KONAMI_CC,
	KONAMI_A,
	KONAMI_B,
	KONAMI_U,
	KONAMI_X,
	KONAMI_Y,
	KONAMI_DP
};

enum
{
	CPUINFO_PTR_KONAMI_SETLINES_CALLBACK = CPUINFO_PTR_CPU_SPECIFIC
};

#define KONAMI_IRQ_LINE	0	/* IRQ line number */
#define KONAMI_FIRQ_LINE 1   /* FIRQ line number */

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
} konami_exgtfr_register;

#define exgtfr_register konami_exgtfr_register

typedef struct _konami_Regs konami_Regs;
struct _konami_Regs
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
	void (*setlines_callback)(int lines);
};

void konami_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern offs_t Dasmknmi(char *buffer, UINT32 pc, UINT8 *oprom, UINT8 *opram, int bytes);
#endif

#endif /* _KONAMI_H */

// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    hd6309.h

    Portable 6309 emulator

*********************************************************************/

#ifndef _HD6309_H
#define _HD6309_H

#include <stdbool.h>
#include "cpuintrf.h"

enum
{
	HD6309_PC = 1,
	HD6309_S,
	HD6309_CC,
	HD6309_A,
	HD6309_B,
	HD6309_U,
	HD6309_X,
	HD6309_Y,
	HD6309_DP,
	HD6309_E,
	HD6309_F,
	HD6309_V,
	HD6309_MD,
	HD6309_W
};

#define HD6309_IRQ_LINE	0	/* IRQ line number */
#define HD6309_FIRQ_LINE 1   /* FIRQ line number */

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
} hd6309_exgtfr_register;

#define exgtfr_register hd6309_exgtfr_register

typedef struct _hd6309_Regs hd6309_Regs;
struct _hd6309_Regs
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

	PAIR16	w;
	PAIR16	v;
	UINT8	md;
	UINT8	temp_im;

	UINT8*	reg8;
	PAIR16*	reg16;

	int (*irq_callback)(int irqline);
};

void hd6309_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern offs_t hd6309_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes);
#endif

#endif /* _HD6309_H */

// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
/*****************************************************************************
 *
 *   i8x41.h
 *   Portable UPI-41/8041/8741/8042/8742 emulator interface
 *
 *   Copyright (c) 1999 Juergen Buchmueller, all rights reserved.
 *
 *   TLP (10-Jan-2003)
 *     Added output ports registers to the debug viewer
 *     Added the Clock Divider
 *
 *****************************************************************************/

#ifndef _I8X41_H
#define _I8X41_H

#include "cpuintrf.h"


/* The i8x41/i8x42 input clock is divided by 15. Use it with your CPU speed */
#define I8X41_CLOCK_DIVIDER		15

/* Note:
 * I8X41_DATA is A0 = 0 and R/W
 * I8X41_CMND is A0 = 1 and W only
 * I8X41_STAT is A0 = 1 and R only
 */


/****************************************************************************
 *  Interrupt constants
 */

#define I8X41_INT_IBF	0	/* input buffer full interrupt */
#define I8X41_INT_TEST1 1	/* test1 line (also counter interrupt; taken on cntr overflow) */


/****************************************************************************
 *  Use these in the I/O port fields of your driver for the test lines - i.e,
 *    { I8X41_t0,   I8X41_t0,   i8041_test0_r },
 *    { I8X41_t1,   I8X41_t1,   i8041_test1_r },
 *    { I8X41_ps,   I8X41_ps,   i8041_port_strobe_w },
 */

#define I8X41_t0		0x80	/* TEST0 input port handle */
#define I8X41_t1		0x81	/* TEST1 input port handle */
#define I8X41_ps		0x82	/* Prog pin strobe for expanded port sync */


/****************************************************************************
 *  The i8x41/i8x42 have 128/256 bytes of internal memory respectively
 */

#define I8X41_intRAM_MASK		0x7f
#define I8X42_intRAM_MASK		0xff


enum {
	I8X41_PC=1, I8X41_SP, I8X41_PSW, I8X41_T, I8X41_DATA, I8X41_DATA_DASM,
	I8X41_CMND, I8X41_CMND_DASM, I8X41_STAT, I8X41_P1, I8X41_P2,I8X41_A,
	I8X41_R0, I8X41_R1, I8X41_R2, I8X41_R3, I8X41_R4, I8X41_R5, I8X41_R6, I8X41_R7
};



/****************************************************************************
 *  Public Functions
 */

extern void i8x41_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned Dasm8x41( char *dst, unsigned pc );
#endif

#endif /* _I8X41_H */

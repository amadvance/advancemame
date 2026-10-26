// license:BSD-3-Clause
// copyright-holders:Peter Trauner, Curt Coder
/*****************************************************************************
 *
 *	 cdp1802.h
 *	 portable cosmac cdp1802 emulator interface
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *****************************************************************************/

#ifndef _CDP1802_H
#define _CDP1802_H

#include "cpuintrf.h"

#define CDP1802_INT_NONE	0
#define CDP1802_IRQ			1

// CDP1802 I/O Flags

enum {
	EF1 = 1,
	EF2	= 2,
	EF3	= 4,
	EF4 = 8
};

// CDP1802 Registers

enum {
	CDP1802_P = 1,	// Designates which register is Program Counter
	CDP1802_X,		// Designates which register is Data Pointer
	CDP1802_D,		// Data Register (Accumulator)
	CDP1802_B,		// Auxiliary Holding Register
	CDP1802_T,		// Holds old X, P after Interrupt (X is high nibble)

	CDP1802_R0,		// 1 of 16 Scratchpad Registers
	CDP1802_R1,
	CDP1802_R2,
	CDP1802_R3,
	CDP1802_R4,
	CDP1802_R5,
	CDP1802_R6,
	CDP1802_R7,
	CDP1802_R8,
	CDP1802_R9,
	CDP1802_Ra,
	CDP1802_Rb,
	CDP1802_Rc,
	CDP1802_Rd,
	CDP1802_Re,
	CDP1802_Rf,

	CDP1802_DF,		// Data Flag (ALU Carry)
	CDP1802_IE,		// Interrupt Enable
	CDP1802_Q,		// Output Flip-Flop
	CDP1802_N,		// Holds Low-Order Instruction Digit
	CDP1802_I,		// Holds High-Order Instruction Digit
	CDP1802_IRQ_STATE
};

// CDP1802 Configuration

typedef struct {
	/* called after execution of an instruction with cycles,
	   return cycles taken by dma hardware */
	void (*dma)(int cycles);
	void (*out_q)(int level);
	int (*in_ef)(void);
} CDP1802_CONFIG;


void cdp1802_dma_write(UINT8 data);
int cdp1802_dma_read(void);

extern int cdp1802_icount;				// cycle count

void cdp1802_get_info(UINT32 state, union cpuinfo *info);

unsigned DasmCdp1802(char *dst, unsigned oldpc);

#endif

// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
/*****************************************************************************
 *
 *   m4510.c
 *   Portable 4510 emulator V1.0beta
 *
 *   Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *****************************************************************************/

#ifndef _M4510_H
#define _M4510_H

#include "m6502.h"


enum {
	M4510_PC=1, M4510_S, M4510_P, M4510_A, M4510_X, M4510_Y,
	M4510_Z, M4510_B, M4510_EA, M4510_ZP,
	M4510_NMI_STATE, M4510_IRQ_STATE,
	M4510_MEM_LOW,M4510_MEM_HIGH,
	M4510_MEM0, M4510_MEM1, M4510_MEM2, M4510_MEM3,
	M4510_MEM4, M4510_MEM5, M4510_MEM6, M4510_MEM7
};

#define M4510_IRQ_LINE					M6502_IRQ_LINE

void m4510_get_info(UINT32 state, union cpuinfo *info);


#ifdef MAME_DEBUG
extern unsigned m4510_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes);
#endif

#endif



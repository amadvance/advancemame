// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
/*****************************************************************************
 *
 *   m6509.h
 *   Portable 6509 emulator V1.0beta
 *
 *   Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *****************************************************************************/

#ifndef _M6509_H
#define _M6509_H

#include "m6502.h"

enum {
	M6509_PC=1, M6509_S, M6509_P, M6509_A, M6509_X, M6509_Y,
	M6509_EA, M6509_ZP, M6509_NMI_STATE, M6509_IRQ_STATE, M6509_SO_STATE,
	M6509_PC_BANK, M6509_IND_BANK
};

#define M6509_IRQ_LINE					M6502_IRQ_LINE
/* use cpunum_set_input_line(cpu, M6509_SET_OVERFLOW, level)
   to change level of the so input line
   positiv edge sets overflow flag */
#define M6509_SET_OVERFLOW 3

void m6509_get_info(UINT32 state, union cpuinfo *info);

#endif /* _M6509_H */



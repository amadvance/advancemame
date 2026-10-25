// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
/*****************************************************************************
 *
 *   m65ce02.c
 *   Portable 65ce02 emulator V1.0beta
 *
 *   Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *****************************************************************************/

#ifndef _M65CE02_H
#define _M65CE02_H

#include "m6502.h"

enum {
	M65CE02_PC=1, M65CE02_S, M65CE02_P, M65CE02_A, M65CE02_X, M65CE02_Y,
	M65CE02_Z, M65CE02_B, M65CE02_EA, M65CE02_ZP,
	M65CE02_NMI_STATE, M65CE02_IRQ_STATE
};

#define M65CE02_IRQ_LINE				M6502_IRQ_LINE

extern int m65ce02_ICount;				/* cycle count */

extern void m65ce02_reset(void *param);
extern void m65ce02_exit(void);
extern int	m65ce02_execute(int cycles);
extern unsigned m65ce02_get_context (void *dst);
extern void m65ce02_set_context (void *src);
extern unsigned m65ce02_get_reg (int regnum);
extern void m65ce02_set_reg (int regnum, unsigned val);
extern void m65ce02_set_irq_line(int irqline, int state);
extern void m65ce02_set_irq_callback(int (*callback)(int irqline));
extern void m65ce02_state_save(void *file);
extern void m65ce02_state_load(void *file);
extern const char *m65ce02_info(void *context, int regnum);
extern unsigned m65ce02_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned int Dasm65ce02( char *dst, unsigned pc );
#endif

#endif /* _M65CE02_H */



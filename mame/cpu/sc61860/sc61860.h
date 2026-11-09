// license:BSD-3-Clause
// copyright-holders:Peter Trauner
/*****************************************************************************
 *
 *	 sc61860.h
 *	 portable sharp 61860 emulator interface
 *   (sharp pocket computers)
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *****************************************************************************/
#ifndef _SC61860_H
#define _SC61860_H

/*
  official names seam to be
  ESR-H, ESR-J
  (ESR-L SC62015 ist complete different)
 */

/* unsolved problems
   the processor has 8 kbyte internal rom
   only readable with special instructions and program execution
   64 kb external ram (first 8kbyte not seen for program execution?) */

#include "cpuintrf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool (*reset)(void);
    bool (*brk)(void);
    bool (*x)(void);
    int (*ina)(void);
    void (*outa)(int);
    int (*inb)(void);
    void (*outb)(int);
    void (*outc)(int);
} SC61860_CONFIG;

#ifdef MAME_DEBUG
unsigned sc61860_dasm(char *dst, unsigned oldpc);
#endif

/* timer_pulse(1/500.0, 0,sc61860_2ms_tick) */
void sc61860_2ms_tick(int param);
/* this is though for power on/off of the sharps */
UINT8 *sc61860_internal_ram(void);

void sc61860_get_info(UINT32 state, union cpuinfo *info);

#ifdef __cplusplus
}
#endif

#endif

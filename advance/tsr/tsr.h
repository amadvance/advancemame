/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __TSR_H
#define __TSR_H

#include "types.h"

/* RUThere code for int2f */
extern uint16 TSR_RUTACK1;
extern uint16 TSR_RUTACK2;

/* Return code of tsr */
#define TSR_SUCCESS 0
#define TSR_FAILURE 1
#define TSR_LOAD 2
#define TSR_UNLOAD 3

/* Main call */
uint16 _pascal tsr(int (*remote)(void far* arg), char far* args);

/* Remote call */
int _cdecl far tsr_remote(void far* arg);

/* First level loading stage */
uint16 _pascal tsr_init_0(void);

/* Second level loading stage, after the interrupt installation */
uint16 _pascal tsr_init_1(void);

/* First level unloading stage */
void _pascal tsr_done_0(void);

/* Second level unloading stage */
void _pascal tsr_done_1(void);

#endif
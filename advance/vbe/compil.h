/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#ifndef __COMPIL_H
#define __COMPIL_H

#include "lib.h"

/* SS != DS */
#define STACK_PTR far
/* private stack, doesn't requires "static" local variable */
#define STACK_DECL

typedef uint8 BYTE;
typedef uint16 WORD;
typedef uint32 DWORD;

#define assert(a) do { } while(0)

#define __dpmi_regs regs_32

void _pascal int_1a_call(regs_32 far* regs);
void _pascal int_10_call(regs_32 far* regs);

int __dpmi_int(int _vector, regs_32 far * _regs);

void dosmemget(unsigned long offset, unsigned length, void far* buffer);

#define	_farpokeb(seg,off,value) ((*(BYTE*)MK_FP(seg,off)) = (value))
#define	_farpeekb(seg,off) (*(BYTE*)MK_FP(seg,off))
#define	_farpokew(seg,off,value) ((*(WORD*)MK_FP(seg,off)) = (value))
#define	_farpeekw(seg,off) (*(WORD*)MK_FP(seg,off))
#define	_farpokel(seg,off,value) ((*(DWORD*)MK_FP(seg,off)) = (value))
#define	_farpeekl(seg,off) (*(DWORD*)MK_FP(seg,off))

#endif
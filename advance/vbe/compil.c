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

#include "compil.h"

void dosmemget(unsigned long offset, unsigned length, void far* buffer) {
	memcpy(buffer,MK_FP(offset >> 4,offset & 0xF),length);
}

int __dpmi_int(int vector, regs_32 far *regs) {
	regs_32_preset(regs);
	int_32_call(vector,regs);
	return 0;
}

/* It is never a sys */
int tsr_sys = 0;

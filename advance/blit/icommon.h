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
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#ifndef __ICOMMON_H
#define __ICOMMON_H

/***************************************************************************/
/* internal */

/* Add a byte index to a generic pointer */
#define PADD(p, i) \
	do { \
		p = (void*)(((uint8*)p) + i); \
	} while (0)

/* Dereference a uint8* with a byte index */
#define P8DER(p, i) *(((uint8*)p) + i)
#define P8DER0(p) *(uint8*)p

/* Dereference a uint16* with a byte index */
#define P16DER(p, i) *(uint16*)(((uint8*)p) + i)
#define P16DER0(p) *(uint16*)p

/* Dereference a uint32* with a byte index */
#define P32DER(p, i) *(uint32*)(((uint8*)p) + i)
#define P32DER0(p) *(uint32*)p

/* Dereference a uint64* with a byte index */
#define P64DER(p, i) *(uint64*)(((uint8*)p) + i)
#define P64DER0(p) *(uint64*)p

/* Suggested in "Intel Optimization" for Pentium II */
#define ASM_JUMP_ALIGN ".p2align 4\n"

/* Notes
1) The count argument is the number of source elements (pixels) to process
2) The count divisor at early stage of every functions is the number of
	source elements processed in one subsequent loop iteration
3) The step argument always referes to bytes
4) All the internal_* functions use the fs register for the destination when
	is expressed as an unsigned, before the call the fs register
	must be set at the correct value
5) After any internal_* functions the internal_end() function must be called
	before any use of the FPU (float or double operations)
*/

/*
Assembler notes:
) The register allocation is almost fixed. The speed is greather if these
register are used:

S (esi) Source addr
D (edi) Dest addr
a (eax) Data
c (ecx) Counter
b (ebx) Table address
d (edx) Additional data

Specifically this prevent the use of other special register like ebp, which
causes a sensible slowdown in tigh loop. (Tested on a Pentium II)

Registers used out of the main loop are not assigned specifically but
with "r".
*/

#define assert_align(x) \
	do { } while (0)

#endif


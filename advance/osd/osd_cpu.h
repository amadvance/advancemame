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

#ifndef __OSD_CPU_H
#define __OSD_CPU_H

#include "osdutils.h"

/**
 * Some MAME files require this define.
 * For example sound/fm.h.
 */
#define OSD_CPU_H

/* Common types */
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
__extension__ typedef unsigned long long UINT64;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
__extension__ typedef signed long long INT64;

/* Combine two 32-bit integers into a 64-bit integer */
#define COMBINE_64_32_32(A, B) ((((UINT64)(A))<<32) | (UINT32)(B))
#define COMBINE_U64_U32_U32(A, B) COMBINE_64_32_32(A, B)

/* Return upper 32 bits of a 64-bit integer */
#define HI32_32_64(A) (((UINT64)(A)) >> 32)
#define HI32_U32_U64(A) HI32_32_64(A)

/* Return lower 32 bits of a 64-bit integer */
#define LO32_32_64(A) ((A) & 0xffffffff)
#define LO32_U32_U64(A) LO32_32_64(A)

#define DIV_64_64_32(A, B) ((A)/(B))
#define DIV_U64_U64_U32(A, B) ((A)/(UINT32)(B))

#define MOD_32_64_32(A, B) ((A)%(B))
#define MOD_U32_U64_U32(A, B) ((A)%(UINT32)(B))

#define MUL_64_32_32(A, B) ((A)*(INT64)(B))
#define MUL_U64_U32_U32(A, B) ((A)*(UINT64)(UINT32)(B))

/* Declaration of MAME functions with ... args */
#define CLIB_DECL

#endif


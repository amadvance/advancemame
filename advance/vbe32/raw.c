/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003 Andrea Mazzoleni
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

#include "raw.h"

#include <dpmi.h>
#include <go32.h>
#include <stdlib.h>
#include <string.h>
#include <sys/exceptn.h>

#define FILL    0x00

static unsigned char wrapper[] = {
/* 00 */ 0x8b, 0x06,			/*     mov eax, [esi]            */
/* 03 */ 0x26, 0x89, 0x47, 0x2a,	/*     mov es:[edi+42], eax      */
/* 06 */ 0x06,                          /*     push    es               */
/* 07 */ 0x1e,                          /*     push    ds               */
/* 08 */ 0x06,                          /*     push    es               */
/* 09 */ 0x1f,                          /*     pop     ds               */
/* 0a */ 0x66, 0xb8,                    /*     mov ax,                  */
/* 0c */ FILL, FILL,                    /*         _our_selector        */
/* 0e */ 0x8e, 0xd8,                    /*     mov ds, ax               */
/* 10 */ 0x8e, 0xc0,                    /*     mov es, ax               */
/* 12 */ 0x8e, 0xe0,                    /*     mov fs, ax               */
/* 14 */ 0x8e, 0xe8,                    /*     mov gs, ax               */
/* 16 */ 0xbb,                          /*     mov ebx,                 */
/* 17 */ FILL, FILL, FILL, FILL,        /*         _local_stack         */
/* 1b */ 0xfc,                          /*     cld                      */
/* 1c */ 0x89, 0xe1,                    /*     mov ecx, esp             */
/* 1e */ 0x8c, 0xd2,                    /*     mov dx, ss               */
/* 20 */ 0x8e, 0xd0,                    /*     mov ss, ax               */
/* 22 */ 0x89, 0xdc,                    /*     mov esp, ebx             */
/* 24 */ 0x52,                          /*     push edx                 */
/* 25 */ 0x51,                          /*     push ecx                 */
/* 26 */ 0x56,                          /*     push esi                 */
/* 27 */ 0x57,                          /*     push edi                 */
/* 28 */ 0xe8,                          /*     call                     */
/* 29 */ FILL, FILL, FILL, FILL,        /*         _rmcb                */
/* 2d */ 0x5f,                          /*     pop edi                  */
/* 2e */ 0x5e,                          /*     pop esi                  */
/* 2f */ 0x58,                          /*     pop eax                  */
/* 30 */ 0x5b,                          /*     pop ebx                  */
/* 31 */ 0x8e, 0xd3,                    /*     mov ss, bx               */
/* 33 */ 0x89, 0xc4,                    /*     mov esp, eax             */
/* 35 */ 0x1f,                          /*     pop ds                   */
/* 36 */ 0x07,                          /*     pop es                   */
/* 37 */ 0xcf                           /*     iret                     */
};

int _go32_dpmi_allocate_real_mode_callback_raw_with_stack(_go32_dpmi_seginfo *info, __dpmi_regs *regs, unsigned char *stack, unsigned long stack_length)
{
	*(short *)(wrapper+0x0c) = __djgpp_ds_alias;
	*(long  *)(wrapper+0x17) = (long) stack + stack_length;
	*(long  *)(wrapper+0x29) = info->pm_offset - ((long)wrapper + 0x2d);

	return __dpmi_allocate_real_mode_callback((void*)wrapper, regs, (__dpmi_raddr *)&info->rm_offset);
}




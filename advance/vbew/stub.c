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

#include <stdio.h>
#include <io.h>
#include <dpmi.h>
#include <crt0.h>
#include <sys/farptr.h>
#include <go32.h>
#include <pc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/exceptn.h>

#include "keep.h"
#include "raw.h"
#include "vbe.h"

int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

int _stklen = 32768; /* 32-bit stack size */

static int detect(void) {
	__dpmi_regs regs;

	memset(&regs,0,sizeof(regs));
	regs.x.ax = 0x4F14;

	if (__dpmi_int(0x10, &regs) != 0) {
		printf("Error calling dpmi_int()\n");
		return -1;
	}

	if (regs.x.ax == 0x004F && regs.x.bx == TSR_RUTACK1 && regs.x.cx == TSR_RUTACK2) {
		printf("Already installed\n");
		return -1;
	}

	return 0;
}

static _go32_dpmi_seginfo handler_old;
static unsigned char handler_stack[32256];
static _go32_dpmi_registers handler_regs;

static void handler(_go32_dpmi_registers* r)
{
	/* detect */
	if (r->x.ax == 0x4F14) {
		r->x.ax = 0x004F;
		r->x.bx = TSR_RUTACK1;
		r->x.cx = TSR_RUTACK2;
		r->x.dx = 0;
		r->x.si = 0;
		r->x.di = 0;
		goto ret;
	}

	/* standard interrupt */
	if (r->x.ax < 0x4F00 || r->x.ax > 0x4F1F) {
		vga_service(r);
		goto chain;
	} else {
		vbe_service(r);
		goto ret;
	}

chain:
	/* chain to the original interrupt */
	/* set the new address in the reg struct */
	r->x.ip = handler_old.rm_offset;
	r->x.cs = handler_old.rm_segment;

	/* don't unwind the stack */
	return;

ret:
	/* return from the interrupt */
	/* don't set the address, it is already setup by the call wrapper */

	/* unwind the stack */
	r->x.sp += 6;
	return;
}

static int install(void) {
	_go32_dpmi_seginfo info;
	info.pm_offset = (unsigned)&handler;

	_go32_dpmi_get_real_mode_interrupt_vector(0x10, &handler_old);
	_go32_dpmi_allocate_real_mode_callback_raw_with_stack(&info, &handler_regs, handler_stack, sizeof(handler_stack));

	_go32_dpmi_set_real_mode_interrupt_vector(0x10, &info);

	return 0;
}

int optionmatch(const char* arg, const char* opt) {
	return (arg[0] == '-' || arg[0] == '/') && strcasecmp(arg+1,opt) == 0;
}

int main(int argc, char* argv[])
{
	char config[128];
	int arg_load;
	unsigned i;

	printf(OEM_PRODUCT_STR " by " OEM_VENDOR_STR " v" OEM_VERSION_STR "\n");

	strcpy(config,"vbe.rc");

	arg_load = 0;
	for(i=1;i<argc;++i) {
		const char* tok = argv[i];
		if (optionmatch(tok,"l")) {
			arg_load = 1;
		} else if (optionmatch(tok,"c")) {
			tok = argv[++i];
			if (!tok) {
				printf("Missing config file path\n");
				exit(EXIT_FAILURE);
			}
			strcpy(config,tok);
		} else {
			printf("Unknown option %s\n", tok);
			exit(EXIT_FAILURE);
		}
	}

	if (!arg_load) {
		printf(
		"Usage:\n"
		"    vbew [/l] [/c CONFIG]\n"
		"Commands:\n"
		"    /l         Load the TSR\n"
		"Options:\n"
		"    /c CONFIG  Use this config file instead of vbe.rc\n"
		);

		return EXIT_SUCCESS;
	}

	if (detect() != 0) {
		exit(EXIT_FAILURE);
	}

	if (vbe_init(config) != 0) {
		exit(EXIT_FAILURE);
	}

#if 1
	if (install() != 0) {
		exit(EXIT_FAILURE);
	}

	__djgpp_exception_toggle();
	keep(0, 0);
#else
	{
		_go32_dpmi_registers r;
		r.x.ax = 0x4F02;
		r.x.bx = 0x4103;
		vbe_service(&r);
	}
	printf("Exit\n");
#endif

	return EXIT_SUCCESS;
}

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
#include "lib.h"

int main(void)
{
	regs_32 regs;
	cputs("AdvanceOFF by Andrea Mazzoleni v0.11 " __DATE__ "\r\n\r\n");

	/* SMARTDRV flush */
	regs.x.ax = 0x4A10;
	regs.x.bx = 0x0001;
	int_32_call(0x2F, &regs);

	/* APM detect */
	regs.x.ax = 0x5300;
	regs.x.bx = 0x0000;
	int_32_call(0x15, &regs);
	if ((regs.x.flags & fCF)!=0 || regs.x.bx != 0x504D) {
		cputs("APM BIOS not found\r\n");
		return EXIT_FAILURE;
	}

	if (regs.x.ax < 0x102) {
		cputs("APM BIOS too old\r\n");
		return EXIT_FAILURE;
	}

	/* APM connection */
	regs.x.ax = 0x5301;
	regs.x.bx = 0x0000;
	int_32_call(0x15, &regs);
	if ((regs.x.flags & fCF) != 0) {
		cputs("APM real mode connection failed\r\n");
		return EXIT_FAILURE;
	}

	/* APM notify version */
	regs.x.ax = 0x530E;
	regs.x.bx = 0x0000;
	regs.x.cx = 0x0102;
	int_32_call(0x15, &regs);
	if ((regs.x.flags & fCF) != 0) {
		cputs("APM notify version failed\r\n");
		return EXIT_FAILURE;
	}

	/* APM off */
	regs.x.ax = 0x5307;
	regs.x.bx = 0x0001;
	regs.x.cx = 0x0003;
	int_32_call(0x15, &regs);

	cputs("APM off failed, still alive!\r\n");
	return EXIT_FAILURE;
}
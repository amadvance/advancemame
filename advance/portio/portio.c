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

static void help(void)
{
	cputs("AdvancePORTIO by Andrea Mazzoleni v0.11 " __DATE__ "\r\n");
	cputs("Usage:\r\n");
	cputs("    portio [LPT? | ADDRESS_HEX] [VALUE_HEX]\r\n");
	cputs("\r\nExamples:\r\n");
	cputs("    portio lpt1 ff\r\n");
	cputs("    portio 378 0\r\n");
}

int main(unsigned argl, const char far* args)
{
	unsigned addr;
	char far* tok;

	char arg[128];
	memcpy(arg, args, argl);
	arg[argl] = 0;

	tok = strtok(arg, " \t");
	if (!tok) {
		help();
		return EXIT_FAILURE;
	}

	if (strcmp(tok, "lpt1")==0 || strcmp(tok, "LPT1")==0) {
		addr = *(uint16 far *)MK_FP(0x0, 0x408);
	} else if (strcmp(tok, "lpt2")==0 || strcmp(tok, "LPT2")==0) {
		addr = *(uint16 far *)MK_FP(0x0, 0x408 + 2);
	} else if (strcmp(tok, "lpt3")==0 || strcmp(tok, "LPT3")==0) {
		addr = *(uint16 far *)MK_FP(0x0, 0x408 + 4);
	} else {
		addr = (unsigned)strtou(tok, 16);
	}

	if (!addr) {
		cputs("Invalid port address\r\n");
		return EXIT_FAILURE;
	}

	cputs("Port ");
	cputu(addr, 0, ' ', 16);
	cputs("h\r\n");

	tok = strtok(0, " \t");

	if (tok) {
		unsigned value = (unsigned)strtou(tok, 16);
		cputs("Write ");
		cputu(value, 0, ' ', 16);
		cputs("h\r\n");
		outportb(addr, value);
	} else {
		unsigned value = inportb(addr);
		cputs("Read ");
		cputu(value, 0, ' ', 16);
		cputs("h\r\n");
	}

	return EXIT_SUCCESS;
}
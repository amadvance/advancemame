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

#include "compil.h"
#include "types.h"
#include "lib.h"
#include "tsr.h"

/***************************************************************************/
/* TSR */

struct tsr_type {
	uint16 seg; /* TSR segment CS==DS */
	uint16 psp; /* TSR psp segment, ==0 for device driver */
	int far _cdecl (far* unload)(void); /* deinstallation call */
	int far _cdecl (far* remote)(void far* arg);
};

/* Local TSR info */
struct tsr_type tsr_data;

/* Remote TSR info */
struct tsr_type far* tsr_loaded;

/* Check if is a device driver */
extern int tsr_sys;

/***************************************************************************/
/* RUThere */

/* Old interrupt */
extern void far * int_2f_old;

/* ID used for the interrupt trap */
extern uint8 int_2f_id;

/* New 2f interrupt */
void int_2f(void);

void int_2f_trap(regs_32 far * r)
{
	r->x.cx = TSR_RUTACK1;
	r->x.dx = TSR_RUTACK2;
	r->x.es = _DS;
	r->x.bx = FP_OFF( &tsr_data );
	r->h.al = 0xFF;
}

static void int_2f_call(void)
{
	asm push si
	asm push di
	asm push bp
	asm push ds
	asm int 0x2f
	asm pop ds
	asm pop bp
	asm pop di
	asm pop si
}

static int rut_free_code(unsigned code)
{
	asm mov ah, [byte ptr code]
	asm xor al, al
	asm xor bx, bx
	asm xor cx, cx
	asm xor dx, dx
	int_2f_call();
	asm cmp al, 0
	asm jne error
	return 1;
error:
	return 0;
}

/* RUT install */
static int rut_search(void)
{
	unsigned i;
	int_2f_id = 0;
	for(i=0xC0;i<=0xFF;++i) {
		if (rut_free_code(i)) {
			int_2f_id = i;
			return 1;
		}
	}
	return 0;
}

// test di installazione
// reg:
//   ah    code
//   al    0xFF
//   cx    RUT_ACK1
//   dx    RUT_ACK2
//   es:bx &tsr_type
static struct tsr_type far * rut_is_installed_code(uint8 code)
{
	asm mov ah, [code]
	asm xor al, al
	asm xor bx, bx
	asm xor cx, cx
	asm xor dx, dx
	int_2f_call();
	asm cmp al, 0FFh
	asm jne notinst
	asm cmp cx, TSR_RUTACK1
	asm jne notinst
	asm cmp dx, TSR_RUTACK2
	asm jne notinst

	asm mov dx, es
	asm mov ax, bx
	return (struct tsr_type far*)MK_FP(_DX, _AX);
notinst:
	return 0;
}

// cerca il programma
// effect:
//   se trovato tsr punta alla zona dati
// return:
//   0 non trovato
//   !=0 puntatore alla zona dati del TSR
static struct tsr_type far* tsr_is_installed(void)
{
	unsigned i;
	for(i=0xC0;i<=0xFF;++i) {
		struct tsr_type far* p = rut_is_installed_code(i);
		if (p)
			return p;
	}
	return 0;
}

/***************************************************************************/
/* INT */

extern void far * int_10_old;
#ifdef __RUN__
extern void far * int_21_old;
extern void far * int_8_old;
#endif

void int_10(void);
#ifdef __RUN__
void int_21(void);
void int_8(void);
#endif

/* Load old interrupt value */
static void int_init(void)
{
	int_2f_old = getvect(0x2f);
	int_10_old = getvect(0x10);
#ifdef __RUN__
	int_21_old = getvect(0x21);
	int_8_old = getvect(0x8);
#endif
}

/***************************************************************************/
/* TSR */

/* Unload the TSR */
static int far _cdecl unload(void)
{
	/* device driver can't be unloaded */
	if (tsr_sys)
		return 0;

	if ( (int_10 == getvect(0x10))
#ifdef __RUN__
		&& (int_21 == getvect(0x21))
		&& (int_8 == getvect(0x8))
#endif
		&& (int_2f == getvect(0x2f))) {

		tsr_done_0();

		setvect(0x10, int_10_old);
#ifdef __RUN__
		setvect(0x21, int_21_old);
		setvect(0x8, int_8_old);
#endif
		setvect(0x2f, int_2f_old);

		tsr_done_1();

		return 1;
	}
	return 0;
}

/* Load the TSR */
static int load(void)
{
	(void far*)tsr_data.unload = MK_FP( _CS, FP_OFF( unload ));
	(void far*)tsr_data.remote = MK_FP( _CS, FP_OFF( tsr_remote ));
	tsr_data.seg = _DS;
	if (tsr_sys)
		tsr_data.psp = 0;
	else
		tsr_data.psp = pspseg();

	if (!rut_search()) {
		return 0;
	}

	if (tsr_init_0() != TSR_SUCCESS) {
		return 0;
	}

	setvect(0x10, int_10);
#ifdef __RUN__
	setvect(0x21, int_21);
	setvect(0x8, int_8);
#endif
	setvect(0x2f, int_2f);

	if (tsr_init_1() != TSR_SUCCESS) {
		setvect(0x10, int_10_old);
#ifdef __RUN__
		setvect(0x21, int_21_old);
		setvect(0x8, int_8_old);
#endif
		setvect(0x2f, int_2f_old);

		tsr_done_0();
		return 0;
	}

	return 1;
}

/* Unload the TSR in memory */
static int mem_unload(void)
{
	int far _cdecl (far* func)(void);
	func = tsr_loaded->unload;
	_AX = tsr_loaded->seg;
	asm push ds
	asm mov ds, ax
	func();
	asm pop ds
	return _AX;
}

/* Unload the TSR in memory */
static int mem_remote(void far* arg)
{
	int far _cdecl (far* func)(void far* arg);
	func = tsr_loaded->remote;
	_AX = tsr_loaded->seg;
	asm push ds
	asm mov ds, ax
	func(arg);
	asm pop ds
	return _AX;
}

/***************************************************************************/
/* main */

int main(unsigned argl, const char far* args)
{
	STACK_DECL char arg[128];
	uint16 cmd;

	int_init();

	tsr_loaded = tsr_is_installed();

	/* Duplicate the command line locally */
	memcpy(arg, args, argl);
	arg[argl] = 0;

	/* Call the main TSR function */
	if (tsr_loaded)
		cmd = tsr(mem_remote, arg);
	else
		cmd = tsr(0, arg);

	if (cmd == TSR_LOAD) {
		if (tsr_loaded)
			return EXIT_FAILURE;
		if (!load())
			return EXIT_FAILURE;
		/* device driver hasn't the environment */
		if (!tsr_sys)
			freeenv();
		return EXIT_TSR;
	}

	if (cmd == TSR_UNLOAD) {
		if (!tsr_loaded)
			return EXIT_FAILURE;
		if (!mem_unload()) {
			cputs("Can't unload\n\r");
			return EXIT_FAILURE;
		}
		if (freemem(tsr_loaded->psp) != 0) {
			cputs("Can't release the memory\n\r");
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}

	if (cmd == TSR_SUCCESS)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}
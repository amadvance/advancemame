/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002 Andrea Mazzoleni
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

#include "board.h"

const char* vbe3_driver(void)
{
	return "Generic VBE3 board";
}

int vbe3_detect(void)
{
	return 1;
}

/* Fraction base */
#define HBASE 1
#define VBASE 2

int vbe3_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;

	unsigned hde; /* horizontal display end */
	unsigned vde; /* vertical display end */
	unsigned x_mul; /* x_mul = Horizontal Display End * HBASE / WIDTH */
	unsigned y_mul; /* y_mul = Verical Display End * VBASE / HEIGHT */
	unsigned y_add;
	unsigned r;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("vbe3: incompatible mode\n"));
		return 0;
	}

	hde = card_crtc_get(0x1);
	hde += 1;
	hde *= 8;
	if (hde == co->width) {
		x_mul = 1*HBASE;
	} else if (hde == 2*co->width) {
		x_mul = 2*HBASE;
	} else if (hde == 4*co->width) {
		x_mul = 4*HBASE;
	} else if (hde == 8*co->width) {
		x_mul = 8*HBASE;
	} else if (hde == 16*co->width) {
		x_mul = 16*HBASE;
	} else if (hde == 32*co->width) {
		x_mul = 32*HBASE;
	} else {
		CARD_LOG(("vbe3: HDE doesn't match, width:%d, hde:%d\n", co->width, hde));
		return 0;
	}

	CARD_LOG(("vbe3: width:%d, hde:%d, x_mul:%d\n", co->width, hde, x_mul));

	vde = card_crtc_get(0x12);
	r = card_crtc_get(0x7);
	vde |= (r << 7) & 0x100;
	vde |= (r << 3) & 0x200;
	vde += 1;
	if (vde == co->height/2) {
		y_mul = VBASE/2;
		y_add = 0;
	} else if (vde == co->height/2+1) {
		y_mul = VBASE/2;
		y_add = 1;
	} else if (vde == co->height) {
		y_mul = 1*VBASE;
		y_add = 0;
	} else if (vde == co->height+1) {
		y_mul = 1*VBASE;
		y_add = 1;
	} else if (vde == 2*co->height) {
		y_mul = 2*VBASE;
		y_add = 0;
	} else if (vde == 2*co->height+1) {
		y_mul = 2*VBASE;
		y_add = 1;
	} else if (vde == 4*co->height) {
		y_mul = 4*VBASE;
		y_add = 0;
	} else if (vde == 4*co->height+1) {
		y_mul = 4*VBASE;
		y_add = 1;
	} else {
		CARD_LOG(("vbe3: VDE doesn't match, height:%d, vde:%d\n", co->height, vde));
		return 0;
	}

	CARD_LOG(("vbe3: height:%d, vde:%d, y_mul:%d, y_add:%d\n", co->height, vde, y_mul, y_add));

	/* The values in the cp structure already have the values modified */
	/* for the doublescan and interlaced flag. This change is also done */
	/* bye the y_mul factor that need to be adjusted to not change them */
	/* two times */
	if (cp.interlace)
		y_mul *= 2;
	if (cp.doublescan)
		y_mul /= 2;

	cp.HDisp = cp.HDisp * x_mul / HBASE;
	cp.HBStart = cp.HBStart * x_mul / HBASE;
	cp.HSStart = cp.HSStart * x_mul / HBASE;
	cp.HSEnd = cp.HSEnd * x_mul / HBASE;
	cp.HBEnd = cp.HBEnd * x_mul / HBASE;
	cp.HTotal = cp.HTotal * x_mul / HBASE;

	cp.VDisp = cp.VDisp * y_mul / VBASE + y_add;
	cp.VBStart = cp.VBStart * y_mul / VBASE + y_add;
	cp.VSStart = cp.VSStart * y_mul / VBASE + y_add;
	cp.VSEnd = cp.VSEnd * y_mul / VBASE + y_add;
	cp.VBEnd = cp.VBEnd * y_mul / VBASE + y_add;
	cp.VTotal = cp.VTotal * y_mul / VBASE + y_add;

	CARD_LOG(("vbe3: final hde:%d, vde:%d\n", cp.HDisp, cp.VDisp));

	card_signal_disable();
	card_crtc_all_set(&cp);
	card_signal_enable();

	return 1;
}

void vbe3_reset(void)
{
}

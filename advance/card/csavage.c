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

/*
 * This video driver is STRONGLY derived from the video drivers of the
 * VSyncMAME project by S.Sakamaki (Saka)
 */

#include "board.h"
#include "pci.h"

static unsigned savage_bus_device_func;

static void savage_pll_set(void)
{
	BYTE d0;

	d0 = card_in(0x3CC);
	d0 = d0 | 0x0C;
	card_out(0x3C2, d0);
}

static void savage_doublescan_set(int flag)
{
	if (flag == 0) {
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 0, 0));
	} else {
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 1, 0));
	}
}

static void savage_interlace_set(int flag, card_crtc STACK_PTR *cp)
{
	int d1;

	if (flag == 0) {
		d1 = card_crtc_get(0x42) & ~0x20;
		card_crtc_set(0x42, d1);
	} else {
		d1 = ((cp->HTotal >> 3) - 5) / 2;
		d1 = (d1 * cp->interlaceratio) / 100;
		card_crtc_set(0x3C, d1); /* interlace offset */
		d1 = card_crtc_get(0x42) | 0x20;
		card_crtc_set(0x42, d1);
	}
}

static void savage_ext_register_set(card_crtc STACK_PTR *cp)
{
	int d0;
	int d1;
	int HTotal;
	int HSStart;

	HTotal = cp->HTotal >> 3;
	HSStart = cp->HSStart >> 3;

	d1 = ((HTotal - 5) + HSStart) / 2;
	if ((d1 - HSStart) < 4) {
		if ((HSStart + 4) <= (HTotal - 5))
			d1 = HSStart + 4;
		else
			d1 = HTotal - 5 + 1;
	}

	d0 = 0;
	d0 = card_bitmov(d0, 0, (cp->HTotal >> 3) - 5, 8);
	d0 = card_bitmov(d0, 1, (cp->HDisp >> 3) - 1, 8);
	d0 = card_bitmov(d0, 2, (cp->HBStart >> 3) , 8);
	d0 = card_bitmov(d0, 4, (cp->HSStart >> 3) , 8);
	if ((cp->HBEnd >> 3) - (cp->HBStart >> 3) > 64)
		d0 = card_bitmov(d0, 3, 1, 0);
	if ((cp->HSEnd >> 3) - (cp->HSStart >> 3) > 32)
		d0 = card_bitmov(d0, 5, 1, 0);
	d0 = card_bitmov(d0, 6, d1, 8);

	card_crtc_set(0x5D, d0); /* ext horz reg */
	card_crtc_set(0x3B, d1); /* data transfer exec pos */
	CARD_LOG(("savage: data transfer exec pos %d\n", d1 << 3));

	d0 = 0;
	d0 = card_bitmov(d0, 0, cp->VTotal - 2, 10);
	d0 = card_bitmov(d0, 1, cp->VDisp - 1, 10);
	d0 = card_bitmov(d0, 2, cp->VBStart, 10);
	d0 = card_bitmov(d0, 4, cp->VSStart, 10);
	d0 = card_bitmov(d0, 6, 1, 0); /* line compare */
	card_crtc_set(0x5E, d0); /* ext vert reg */

	savage_doublescan_set(cp->doublescan);
	savage_interlace_set(cp->interlace, cp);
	card_polarity_set(cp->hpolarity, cp->vpolarity);
}

static long savage_clock_get(long vclk, int STACK_PTR *Num, int STACK_PTR *DeN, int STACK_PTR *PS, int STACK_PTR *SeqDiv)
{
	double desfreq;
	double temp;
	double tempdt;
	double dt;
	int d0;
	int d1;

	if (vclk > 175000000) vclk = 175000000;
	*SeqDiv = 1;
	*PS = 1;
	if ( vclk >= 150000000) {
		*PS = 1;
	} else if (vclk >= 75000000) {
		*PS = 2;
	} else if (vclk >= 37500000) {
		*PS = 4;
	} else if (vclk >= 13500000) {
		*PS = 8;
	} else {
		*PS = 16;
	}

	desfreq = vclk * (*PS) * (*SeqDiv);
	dt = 250000000.0;
	*Num = 0;
	*DeN = 0;
	d0 = 2;
	while (d0 < 255) {
		d1 = 2;
		while (d1 < 127) {
			temp = 14318000 * (double)d0 / d1;
			tempdt = desfreq - temp;
			if (tempdt < 0) tempdt *= -1;
			if (tempdt < dt) {
				*Num = d0;
				*DeN = d1;
				dt = tempdt;
			}
			d1 += 1;
		}
		d0 += 1;
	}
	CARD_LOG(("savage: Num %3d, DeN %3d, PS %1d, SeqDiv %1d\n", *Num, *DeN, *PS, *SeqDiv));
	CARD_LOG(("savage: 14.318 * (Num/DeN) : %f\n", 14.318 *((*Num)/(*DeN))));
	CARD_LOG(("savage: 14.318 * (Num/DeN)/(PS*SeqDiv) : %f\n", 14.318 * ((double)(*Num)/(*DeN)) / ((*PS) * (*SeqDiv))));

	vclk = 14318000 * ((double)(*Num)/(*DeN) / ((*PS) * (*SeqDiv)));
	CARD_LOG(("savage: vclk : %f\n", (double)vclk / 1000000.0));

	return vclk;
}

static void savage_clock_set(BYTE Num, BYTE DeN, BYTE PS, BYTE SeqDiv)
{
	DWORD _PS;
	BYTE d0;
	BYTE SR12, SR13, SR29;

	savage_pll_set();

	Num -= 2;
	DeN -= 2;
	_PS = 0;
	while (1) {
		PS = PS >> 1;
		if (PS == 0) break;
		_PS += 1;
	}
	SR12 = (DeN & 0x3F) | (_PS << 6);
	SR13 = Num & 0xFF;
	SR29 = (_PS & 0x04) | ((Num & 0x100) >> 5) | ((DeN & 0x40) >> 2);

	card_seq_set(0x12, SR12);
	card_seq_set(0x13, SR13);
	card_seq_set(0x29, SR29);

	d0 = card_seq_get(0x15) & ~0x21;
	d0 = d0 & ~(1 << 4);
	if (SeqDiv == 2)
		d0 |= (1 << 4);
	card_seq_set(0x15, d0 | 0x03);
	card_seq_set(0x15, d0 | 0x23);
	card_seq_set(0x15, d0 | 0x03);
}

const char* savage_driver(void)
{
	return "Savage 3D/3DM/4";
}

int savage_detect(void)
{
	if (pci_detect()!=0) {
		CARD_LOG(("savage: PCI BIOS not Installed.\n"));
		return 0;
	}

	while (1) {
		if (pci_find_device(0x00005333, 0x00008A20, 0, &savage_bus_device_func)==0) {
			CARD_LOG(("savage: Savage3D found\n"));
			break;
		}
		if (pci_find_device(0x00005333, 0x00008A21, 0, &savage_bus_device_func)==0) {
			CARD_LOG(("savage: Savage3DM found\n"));
			break;
		}
		if (pci_find_device(0x00005333, 0x00008A22, 0, &savage_bus_device_func)==0) {
			CARD_LOG(("savage: Savage4 found\n"));
			break;
		}
		return 0;
	}

	return 1;
}

void savage_reset(void)
{
}

int savage_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int Num, DeN, PS, SeqDiv;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("savage: incompatible mode\n"));
		return 0;
	}

	cp.dotclockHz = savage_clock_get(cp.dotclockHz, &Num, &DeN, &PS, &SeqDiv);

	cp.HBStart = cp.HSStart;
	cp.HBEnd = cp.HTotal - 16;
	cp.VBStart = cp.VDisp;
	cp.VBEnd = cp.VTotal - 2;

	card_signal_disable();
	card_seq_set(0x08, 0x06); /* unlock ext seq regs */
	card_crtc_set(0x35, card_crtc_get(0x35) & 0x0F); /* unlock H V P */
	card_crtc_set(0x38, 0x48);
	card_crtc_set(0x39, 0xA5);
	card_generic_all_set(&cp);
	savage_ext_register_set(&cp);

	savage_clock_set(Num, DeN, PS, SeqDiv);
	card_signal_enable();

	return 1;
}


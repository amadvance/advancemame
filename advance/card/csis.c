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

#define SiS6326 1
#define SiS620 2
#define SiS530 3
#define SiS300 4
#define SiS630 5
#define SiS540 6

static DWORD sis_device;

static int sis_ext_register_unlock(void)
{
	BYTE d0;

	d0 = card_seq_get(0x05);
	if (d0 == 0xA1)
		return 1;
	card_seq_set(0x05, 0x86);
	if (card_seq_get(0x05) == 0xA1) {
		CARD_LOG(("sis: SiS extention registers unlocked\n"));
		return 1;
	}
	card_seq_set(0x05, d0);
	return 0;
}

static void sis_clock_register_internal_set(void)
{
	BYTE d0;

	d0 = card_in(0x3CC);
	d0 = d0 | 0x0C;
	card_out(0x3C2, d0);
}

static BYTE post_scalar_table[]={ 0, 0x00, 0x01, 0x02, 0x03, 0, 0x02, 0, 0x03};

static int sis_6326_clock_set(BYTE Numerator, BYTE Divider, BYTE DeNumerator, BYTE PostScalar, BYTE SeqDivider)
{
	BYTE _2B_DeN;
	BYTE _2B_PS;
	BYTE _2A_Div;
	BYTE _2A_Num;
	BYTE _13_PS;
	BYTE d0;
	int d1;

	sis_clock_register_internal_set();
	card_seq_set(0x38, card_seq_get(0x38) & 0xFC);

	d0 = 0x01;
	if (SeqDivider == 2) d0 = d0 | 0x08;
	card_seq_set(0x01, d0);

	_2A_Div = ((Divider == 1) ? 0 : 1) << 7;
	_2A_Num = (Numerator - 1) & 0x7F;
	_13_PS = ((PostScalar == 6 || PostScalar == 8) ? 1 : 0) << 6;
	_2B_PS = post_scalar_table[PostScalar] << 5;
	_2B_DeN = (DeNumerator - 1) & 0x1F;

	d0 = _2A_Div | _2A_Num;
	card_seq_set(0x2A, d0);
	d0 = card_seq_get(0x13);
	d0 = d0 & ~0x40;
	d0 = d0 | _13_PS;
	card_seq_set(0x13, d0);
	d0 = _2B_PS | _2B_DeN;
	card_seq_set(0x2B, d0);

	d1 = 65536;
	while (d1 > 0) {
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		d1 -= 1;
	}

	return 0;
}

static int sis_300_clock_set(BYTE Numerator, BYTE Divider, BYTE DeNumerator, BYTE PostScalar, BYTE SeqDivider)
{
	BYTE d0;
	int d1;
	BYTE xr2b;
	BYTE xr2c;
	int sbit;

	sis_clock_register_internal_set();

	d0 = 0x01;
	if (SeqDivider == 2) d0 = d0 | 0x08;
	card_seq_set(0x01, d0);

	sbit = (PostScalar == 6 || PostScalar == 8) ? 1 : 0;
	PostScalar = post_scalar_table[PostScalar];

	xr2b = (Numerator - 1) & 0x7f;
	if (Divider == 2)
		xr2b |= 0x80;
	xr2c = (DeNumerator - 1) & 0x1f;
	xr2c |= PostScalar << 5;
	if (sbit)
		xr2c |= 0x80;
	card_seq_set(0x2B, xr2b);
	card_seq_set(0x2C, xr2c);
	card_seq_set(0x2D, 0x80);

	d1 = 65536;
	while (d1 > 0) {
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		inportb(0x3da);
		d1 -= 1;
	}

	return 0;
}

static void sis_6326_ext_register_set(card_crtc STACK_PTR *cp)
{
	int d0;

	d0 = card_seq_get(0x0A);
	d0 = card_bitmov(d0, 0, cp->VTotal - 2, 10);
	d0 = card_bitmov(d0, 1, cp->VDisp - 1 , 10);
	d0 = card_bitmov(d0, 2, cp->VBStart , 10);
	d0 = card_bitmov(d0, 3, cp->VSStart , 10);
	card_seq_set(0x0A, d0);

	d0 = card_bitmov(d0, 0, (cp->HTotal >> 3) - 5, 8);
	d0 = card_bitmov(d0, 1, (cp->HDisp >> 3) - 1, 8);
	d0 = card_bitmov(d0, 2, (cp->HBStart >> 3) , 8);
	d0 = card_bitmov(d0, 3, (cp->HSStart >> 3) , 8);
	d0 = card_bitmov(d0, 4, (cp->HBEnd >> 3) , 6);
	d0 = card_bitmov(d0, 5, cp->HSStart, 0);
	d0 = card_bitmov(d0, 6, cp->HSStart, 1);
	d0 = card_bitmov(d0, 7, cp->HSStart, 2);
	card_seq_set(0x12, d0);
}

static void sis_300_ext_register_set(card_crtc STACK_PTR *cp)
{
	int d0;

	d0 = (
		(((cp->VSEnd) & 0x10 ) << 1) |/* D5 */
		(((cp->VBEnd) & 0x100) >> 4) |/* D4 */
		(((cp->VSStart) & 0x400) >> 7) |/* D3 */
		(((cp->VBStart) & 0x400) >> 8) |/* D2 */
		(((cp->VDisp-1) & 0x400) >> 9) |/* D1 */
		(((cp->VTotal-2) & 0x400) >> 10));/* D0 */
	card_seq_set(0x0A, d0);

	d0 = (
		(( (cp->HSStart >> 3) & 0x300) >> 2) | /* D76 */
		(( (cp->HBStart >> 3) & 0x300) >> 4) | /* D54 */
		((((cp->HDisp >> 3) - 1) & 0x300) >> 6) | /* D32 */
		((((cp->HTotal >> 3) - 5) & 0x300) >> 8)); /* D10 */
	card_seq_set(0x0B, d0);

	d0 = card_seq_get(0x0C) & 0xF8;
	d0 |= (((cp->HSEnd >> 3) & 0x20) >> 3) | /* D2 */
		(((cp->HBEnd >> 3) & 0xC0) >> 6); /* D10 */
	card_seq_set(0x0C, d0);
}

static int sis_6326_bytes_per_line_get(void)
{
	DWORD d0;

	d0 = (DWORD)card_crtc_get(0x13);
	d0 |= (DWORD)((card_seq_get(0x0A) << 4) & 0x0F00);
	d0 = d0 << 3;
	return d0;
}

static void sis_6326_bytes_per_line_set(int d0)
{
	BYTE d1;

	d0 = d0 >> 3;
	d1 = (BYTE)(d0 & 0xFF);
	card_crtc_set(0x13, d1);
	d1 = card_seq_get(0x0A) & 0x0F;
	d1 = d1 | (BYTE)((d0 >> 4) & 0xF0);
	card_seq_set(0x0A, d1);
}

static void sis_doublescan_set(int flag)
{
	if (flag == 0) {
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 0, 0));
	} else {
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 1, 0));
	}
}

static void sis_6326_interlace_set(int flag)
{
	BYTE d1;

	if (flag == 0) {
		d1 = card_seq_get(0x06);
		if ((d1 & (1 << 5)) != 0) {
			d1 = d1 & ~(1 << 5); /* interlace Disable */
			card_seq_set(0x06, d1);
			sis_6326_bytes_per_line_set( sis_6326_bytes_per_line_get() / 2);
		}
	} else {
		d1 = card_seq_get(0x06);
		if ((d1 & (1 << 5)) == 0) {
			d1 = d1 | (1 << 5); /* interlace Enable */
			card_seq_set(0x06, d1);
			sis_6326_bytes_per_line_set( sis_6326_bytes_per_line_get() * 2);
		}
	}
}

static int sis_300_bytes_per_line_get(void)
{
	DWORD d0;

	d0 = (DWORD)card_crtc_get(0x13) & 0xFF;
	d0 |= (DWORD)(card_seq_get(0x0E) << 8) & 0x0F00;
	d0 = d0 << 3;
	return d0;
}

static void sis_300_bytes_per_line_set(int d0)
{
	BYTE d1;

	d0 = d0 >> 3;
	d1 = (BYTE)(d0 & 0xFF);
	card_crtc_set(0x13, d1);
	d1 = card_seq_get(0x0E) & 0xF0;
	d1 = d1 | ((d0 >> 8) & 0x0F);
	card_seq_set(0x0E, d1);
}

static void sis_300_interlace_set(int flag)
{
	BYTE d1;

	if (flag == 0) {
		d1 = card_seq_get(0x06);
		if ((d1 & (1 << 5)) != 0) {
			d1 = d1 & ~(1 << 5); /* interlace Disable */
			card_seq_set(0x06, d1);
			sis_300_bytes_per_line_set( sis_300_bytes_per_line_get() / 2);
		}
	} else {
		d1 = card_seq_get(0x06);
		if ((d1 & (1 << 5)) == 0) {
			d1 = d1 | (1 << 5); /* interlace Enable */
			card_seq_set(0x06, d1);
			sis_300_bytes_per_line_set( sis_300_bytes_per_line_get() * 2);
		}
	}
}

#define MAX_VCO 135000000
#define BASE_FREQ 14318000

static long sis_clock_get(long vclk, int STACK_PTR *Num, int STACK_PTR *Div, int STACK_PTR *DeN, int STACK_PTR *PS, int STACK_PTR *SeqDiv)
{
	double desfreq;
	double temp;
	double tempdt;
	double dt;
	int d0;
	int d1;
	int P[2];
	int PD = 0;
	int idx;

	if (vclk > 135000000) vclk = 135000000;

	P[0] = 1;
	P[1] = 1;
	if (vclk < (BASE_FREQ * 6) / 2) {
		P[0] = 2;
		P[1] = 3;
	}
	if (vclk < (BASE_FREQ * 6) / 3) {
		P[0] = 3;
		P[1] = 4;
	}
	if (vclk < (BASE_FREQ * 6) / 4) {
		P[0] = 4;
		P[1] = 6;
	}
	if (vclk < (BASE_FREQ * 6) / 6) {
		P[0] = 6;
		P[1] = 8;
	}
	if (vclk < (BASE_FREQ * 6) / 8) {
		P[0] = 8;
		P[1] = 12;
	}
	if (vclk < (BASE_FREQ * 6) / 12) {
		P[0] = 12;
		P[1] = 16;
	}
	if (vclk < (BASE_FREQ * 6) / 16) {
		P[0] = 16;
		P[1] = 16;
	}

	*Num = 0;
	*DeN = 0;
	dt = MAX_VCO;
	idx = 0;
	while (idx < 2) {
		desfreq = vclk;
		for(d0 = 2; d0 <= 128*2; d0 += 1) {
			if ((128 <= d0) && (d0 % 2 == 1))
				continue;
			for(d1 = 3; d1 <= 32; d1 += 1) {
				temp = BASE_FREQ * (double)d0 / d1;
				if (temp > MAX_VCO)
					continue;
				temp = temp / P[idx];
				if (desfreq > temp)
					tempdt = desfreq - temp;
				else
					tempdt = temp - desfreq;
				if (tempdt < dt) {
					*Num = d0;
					*DeN = d1;
					dt = tempdt;
					PD = P[idx];
				}
			}
		}
		idx += 1;
	}

	if (*Num > 128) {
		*Div = 2;
		*Num /= 2;
	} else {
		*Div = 1;
	}

	if (PD <= 8) {
		*SeqDiv = 1;
		*PS = PD;
	} else {
		*SeqDiv = 2;
		*PS = PD / 2;
	}

	vclk = (long)((double)BASE_FREQ * (*Num * *Div)/(*DeN * *PS * *SeqDiv));

	CARD_LOG(("sis: Num %3d, DeN %3d, Div %1d, PS %1d, SeqDiv %1d\n", *Num, *DeN, *Div, *PS, *SeqDiv));
	CARD_LOG(("sis: 14.318 * (Num/DeN) : %g\n", (double)(BASE_FREQ/1000000)*(*Num)/(*DeN)));
	CARD_LOG(("sis: vclk : %f\n", (double)vclk / 1000000.0));

	return vclk;
}

const char* sis_driver(void)
{
	switch (sis_device) {
		case SiS6326 : return "Sis6326";
		case SiS620 : return "Sis620";
		case SiS530 : return "Sis530";
		case SiS300 : return "Sis300";
		case SiS630 : return "Sis630";
		case SiS540 : return "Sis540";
	}
	return 0;
}

int sis_detect(void)
{
	unsigned SiS6326BusDeviceFunc;

	if (pci_detect()!=0) {
		CARD_LOG(("matrox: PCI BIOS not Installed.\n"));
		return 0;
	}

	while (1) {
		if (pci_find_device(0x00001039, 0x00006306, 0, &SiS6326BusDeviceFunc)==0) {
			CARD_LOG(("sis: Sis530/SiS620 found\n"));
			sis_device = SiS620;
			break;
		}

		if (pci_find_device(0x00001039, 0x00006326, 0, &SiS6326BusDeviceFunc)==0) {
			CARD_LOG(("sis: SiS6326 found\n"));
			sis_device = SiS6326;
			break;
		}

		if (pci_find_device(0x00001039, 0x00000300, 0, &SiS6326BusDeviceFunc)==0) {
			CARD_LOG(("sis: SiS300 found\n"));
			sis_device = SiS300;
			break;
		}

		if (pci_find_device(0x00001039, 0x00006300, 0, &SiS6326BusDeviceFunc)==0) {
			CARD_LOG(("sis: SiS630 found\n"));
			sis_device = SiS630;
			break;
		}

		if (pci_find_device(0x00001039, 0x0005300, 0, &SiS6326BusDeviceFunc)==0) {
			CARD_LOG(("sis: SiS540 found\n"));
			sis_device = SiS540;
			break;
		}

		return 0;
	}

	if (sis_ext_register_unlock() == 0) {
		CARD_LOG(("sis: Failed unlock extention registers\n"));
		return 0;
	}

	return 1;
}

int sis_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int Num, DeN, Div, PS, SeqDiv;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("sis: incompatible mode\n"));
		return 0;
	}

	cp.dotclockHz = sis_clock_get(cp.dotclockHz, &Num, &Div, &DeN, &PS, &SeqDiv);

	cp.HBStart = cp.HDisp;
	cp.HBEnd = cp.HTotal - 8;
	cp.VBStart = cp.VDisp;
	cp.VBEnd = cp.VTotal - 1;

	card_signal_disable();

	switch (sis_device) {
		case SiS6326:
		case SiS620:
		case SiS530:
			sis_6326_clock_set(Num, Div, DeN, PS, SeqDiv);
			card_generic_all_set(&cp);
			sis_6326_ext_register_set(&cp);
			sis_doublescan_set(cp.doublescan);
			sis_6326_interlace_set(cp.interlace);
			card_polarity_set(cp.hpolarity, cp.vpolarity);
			break;

		case SiS300:
		case SiS630:
		case SiS540:
			sis_300_clock_set(Num, Div, DeN, PS, SeqDiv);
			card_generic_all_set(&cp);
			sis_300_ext_register_set(&cp);
			sis_doublescan_set(cp.doublescan);
			sis_300_interlace_set(cp.interlace);
			card_polarity_set(cp.hpolarity, cp.vpolarity);
			break;
	}

	card_signal_enable();
	return 1;
}

void sis_reset(void)
{
}

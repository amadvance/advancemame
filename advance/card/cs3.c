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

struct s3_id {
	unsigned value;
	const char* name;
	int supported;
	int mul_min;
	int mul_max;
	int div_min;
	int div_max;
	int p_min;
	int p_max;
	long ref; /* kHz */
	long vco_min; /* kHz */
	long vco_max; /* kHz */
	int clock_type;
};

/* Clock */
#define S3_CLOCK_TRIO 1
#define S3_CLOCK_VIRGEGX2 2
#define S3_CLOCK_AURORA 3

/* Configuration */
#define S3_CONF_UNSUPPORTED 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define S3_CONF_TRIO 1, 1+2, 127+2, 1+2, 31+2, 0, 3, 14318, 135000, 270000, S3_CLOCK_TRIO
#define S3_CONF_TRIO64V2 1, 1+2, 127+2, 1+2, 31+2, 0, 4, 14318, 170000, 340000, S3_CLOCK_TRIO
#define S3_CONF_VIRGEDXGX 1, 1+2, 127+2, 1+2, 31+2, 0, 4, 14318, 135000, 270000, S3_CLOCK_TRIO
#define S3_CONF_VIRGEVX 1, 1+2, 127+2, 1+2, 31+2, 0, 4, 14318, 220000, 440000, S3_CLOCK_TRIO
#define S3_CONF_VIRGEGX2 1, 1+2, 127+2, 1+2, 31+2, 0, 4, 14318, 170000, 340000, S3_CLOCK_VIRGEGX2
#define S3_CONF_TRIO3D 1, 1+2, 127+2, 1+2, 31+2, 0, 4, 14318, 230000, 460000, S3_CLOCK_TRIO
#define S3_CONF_AURORA 1, 1+2, 127+2, 1+2, 63+2, 0, 3, 14318, 135000, 270000, S3_CLOCK_AURORA

static struct s3_id s3_id_list[] = {
{ 0x0551, "S3 PLATO/PX", S3_CONF_UNSUPPORTED },
{ 0x5631, "S3 86C325 ViRGE 3D GUI Accelerator", S3_CONF_TRIO },
{ 0x8800, "S3 Vision 866", S3_CONF_TRIO },
{ 0x8801, "S3 Vision 964", S3_CONF_TRIO },
{ 0x8810, "S3 Trio32", S3_CONF_TRIO },
{ 0x8811, "S3 Trio64, or Trio64V+", S3_CONF_TRIO },
{ 0x8812, "S3 Aurora64V+", S3_CONF_AURORA },
{ 0x8814, "S3 Trio64UV+", S3_CONF_TRIO },
{ 0x883D, "S3 ViRGE/VX", S3_CONF_VIRGEVX },
{ 0x8880, "S3 868", S3_CONF_TRIO },
{ 0x88B0, "S3 928", S3_CONF_TRIO },
{ 0x88C0, "S3 864-1", S3_CONF_TRIO },
{ 0x88C1, "S3 864-2", S3_CONF_TRIO },
{ 0x88C2, "S3 864-3", S3_CONF_TRIO },
{ 0x88C3, "S3 864-4", S3_CONF_TRIO },
{ 0x88D0, "S3 964-1", S3_CONF_TRIO },
{ 0x88D1, "S3 964-2", S3_CONF_TRIO },
{ 0x88D2, "S3 964-3", S3_CONF_TRIO },
{ 0x88D3, "S3 964-4", S3_CONF_TRIO },
{ 0x88F0, "S3 968", S3_CONF_TRIO },
{ 0x88F1, "S3 968-2", S3_CONF_TRIO },
{ 0x88F2, "S3 968-3", S3_CONF_TRIO },
{ 0x88F3, "S3 968-3", S3_CONF_TRIO },
{ 0x8900, "S3 Trio64 V2/DX GUI Accelerator", S3_CONF_TRIO },
{ 0x8901, "S3 Trio64V2", S3_CONF_TRIO64V2 }, /* TESTED */
{ 0x8902, "S3 PLATO/PX", S3_CONF_UNSUPPORTED },
{ 0x8904, "S3 Trio3D", S3_CONF_TRIO3D },
{ 0x8A13, "S3 Trio3D/2X", S3_CONF_TRIO3D },
{ 0x8A01, "S3 ViRGE/DX or ViRGE/GX", S3_CONF_VIRGEDXGX }, /* TESTED */
{ 0x8A10, "S3 ViRGE/GX2", S3_CONF_VIRGEGX2  },
{ 0x8A20, "S3 Savage3D", S3_CONF_UNSUPPORTED },
{ 0x8A21, "S3 Savage3D MacroVision", S3_CONF_UNSUPPORTED },
{ 0x8A22, "S3 Savage4", S3_CONF_UNSUPPORTED },
{ 0x9102, "S3 Savage 2000", S3_CONF_UNSUPPORTED },
{ 0x8C01, "S3 ViRGE/MX", S3_CONF_UNSUPPORTED  },
{ 0x8C02, "S3 ViRGE/MX+", S3_CONF_UNSUPPORTED  },
{ 0x8C03, "S3 ViRGE/MX+MV", S3_CONF_UNSUPPORTED  },
{ 0xca00, "S3 SonicVibes", S3_CONF_UNSUPPORTED },
{ 0, 0, S3_CONF_UNSUPPORTED }
};

static unsigned s3_bus_device_func;

static struct s3_id* s3_card; /* S3 detected */

static void s3_pll_set(void)
{
	BYTE d0;

	d0 = card_in(0x3CC);
	d0 = d0 | 0x0C;
	card_out(0x3C2, d0);
}

static void s3_doublescan_set(int flag)
{
	BYTE d0 = card_crtc_get(0x09);
	if(flag == 0){
		card_crtc_set(0x09, d0 & 0xE0);
	}else{
		card_crtc_set(0x09, (d0 & 0xE0) | 0x80);
	}
}

static void s3_interlace_set(int flag, card_crtc STACK_PTR *cp)
{
	int d1;

	if(flag == 0){
		d1 = card_crtc_get(0x42) & ~0x20;
		card_crtc_set(0x42, d1);
	}else{
		d1 = ((cp->HTotal >> 3) - 5) / 2;
		d1 = (d1 * cp->interlaceratio) / 100;
		card_crtc_set(0x3C, d1); /* interlace offset */
		d1 = card_crtc_get(0x42) | 0x20;
		card_crtc_set(0x42, d1);
	}
}

static void s3_ext_set(card_crtc STACK_PTR *cp)
{
	int d0;
	int d1;
	int HTotal;
	int HSStart;

	HTotal  = cp->HTotal  >> 3;
	HSStart = cp->HSStart >> 3;

	d1 = ((HTotal - 5) + HSStart) / 2;
	if((d1 - HSStart) < 4){
	if ((HSStart + 4) <= (HTotal - 5))
		d1 = HSStart + 4;
	else
		d1 = HTotal - 5 + 1;
	}

	d0 = 0;
	d0 = card_bitmov(d0, 0, (cp->HTotal  >> 3) - 5, 8);
	d0 = card_bitmov(d0, 1, (cp->HDisp   >> 3) - 1, 8);
	d0 = card_bitmov(d0, 2, (cp->HBStart >> 3)    , 8);
	d0 = card_bitmov(d0, 4, (cp->HSStart >> 3)    , 8);
	if ((cp->HBEnd >> 3) - (cp->HBStart >> 3) > 64)
		d0 = card_bitmov(d0, 3, 1, 0);
	if ((cp->HSEnd >> 3) - (cp->HSStart >> 3) > 32)
		d0 = card_bitmov(d0, 5, 1, 0);
	d0 = card_bitmov(d0, 6, d1, 8);

	card_crtc_set(0x5D, d0); /* ext horz reg */
	card_crtc_set(0x3B, d1); /* data transfer exec pos */
	CARD_LOG(("s3: data transfer exec pos %d\n", d1 << 3));

	d0 = 0;
	d0 = card_bitmov(d0, 0, cp->VTotal  - 2, 10);
	d0 = card_bitmov(d0, 1, cp->VDisp   - 1, 10);
	d0 = card_bitmov(d0, 2, cp->VBStart,         10);
	d0 = card_bitmov(d0, 4, cp->VSStart,         10);
	d0 = card_bitmov(d0, 6, 1, 0); /* line compare */
	card_crtc_set(0x5E, d0); /* ext vert reg */

	s3_doublescan_set(cp->doublescan);
	s3_interlace_set(cp->interlace, cp);
	card_polarity_set(cp->hpolarity, cp->vpolarity);
}

/*
3C4h index 12h W(R/W):  "Video PLL Data"                            (732/764)
bit  0-4  N1. Frequency divider. Stored as 1-31, actual value 3-33
	5-7  N2. Divides the frequency. 0: /1, 1: /2, 2: /4, 3: /8
	8-14  M.  Quotient. Stored as 1-127, actual value 3-129
Note: Frequency is (M/N1)/(1 << N2) *base frequency.M and N1 are the actual
	values, not the stored ones. Typically the base frequency is 14.318 MHz.
*/

static void s3_clock_set(int pll_mul, int pll_div, int pll_p)
{
	unsigned d0;

	if (s3_card->clock_type == S3_CLOCK_TRIO) {
		unsigned SR12, SR13;
		SR12 = ((pll_div-2) & 0x1F) | ((pll_p & 0x7) << 5);
		SR13 = ((pll_mul-2) & 0x7F);

		card_seq_set(0x12, SR12);
		card_seq_set(0x13, SR13);
	} else if (s3_card->clock_type == S3_CLOCK_VIRGEGX2) {
		unsigned SR12, SR13, SR29, ndiv;

		ndiv = ((pll_div-2) & 0x1F) | ((pll_p & 0x7) << 5);

		SR29 = ndiv >> 7;
		SR12 = (ndiv & 0x1f) | ((ndiv & 0x60) << 1);
		SR13 = ((pll_mul-2) & 0x7F);

		card_seq_set(0x12, SR12);
		card_seq_set(0x13, SR13);
		card_seq_set(0x29, SR29);
	} else if (s3_card->clock_type == S3_CLOCK_AURORA) {
		unsigned SR12, SR13;
		SR12 = ((pll_div-2) & 0x3F) | ((pll_p & 0x7) << 6);
		SR13 = ((pll_mul-2) & 0x7F);

		card_seq_set(0x12, SR12);
		card_seq_set(0x13, SR13);
	}

	d0 = card_seq_get(0x15) & ~0x21;
	d0 = d0 & ~(1 << 4); /* disable sequence divider */

	card_seq_set(0x15, d0 | 0x03);
	card_seq_set(0x15, d0 | 0x23);
	card_seq_set(0x15, d0 | 0x03);

	s3_pll_set();
}

const char* s3_driver(void)
{
	return s3_card->name;
}

#ifndef NDEBUG
static int s3_active;
#endif

int s3_detect(void)
{
	int i;

	assert( !s3_active );

	if (pci_detect()!=0) {
		CARD_LOG(("s3: PCI BIOS not Installed.\n"));
		return 0;
	}

	for(i=0;s3_id_list[i].name;++i) {
		if (pci_find_device(0x00005333, s3_id_list[i].value, 0, &s3_bus_device_func)==0)
			break;
	}

	if(!s3_id_list[i].name){
		return 0;
	}

	CARD_LOG(("s3: found %s, device id %04x\n", s3_id_list[i].name, s3_id_list[i].value));

	if (!s3_id_list[i].supported) {
		CARD_LOG(("s3: card not supported\n"));
		return 0;
	}

	s3_card = s3_id_list + i;

#ifndef NDEBUG
	s3_active = 1;
#endif

	return 1;
}

void s3_reset(void)
{
	assert( s3_active );
#ifndef NDEBUG
	s3_active = 0;
#endif
}

int s3_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int pll_mul, pll_div, pll_p;
	unsigned d0;

	assert( s3_active );

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("s3: incompatible mode\n"));
		return 0;
	}

	d0 = card_crtc_get(0x67);
	if ((d0 & 0xF0) == (5 << 4) || (d0 & 0xF0) == (3 << 4)) {
		CARD_LOG(("s3: double h value for 16 bit mode\n"));
		cp.HDisp *= 2;
		cp.HTotal *= 2;
		cp.HBStart *= 2;
		cp.HBEnd *= 2;
		cp.HSStart *= 2;
		cp.HSEnd *= 2;
	}

	cp.dotclockHz =	card_clock_compute(cp.dotclockHz, s3_card->mul_min,  s3_card->mul_max, s3_card->div_min,  s3_card->div_max, s3_card->p_min,  s3_card->p_max, s3_card->ref, s3_card->vco_min,  s3_card->vco_max, &pll_mul, &pll_div, &pll_p, 0);
	if (cp.dotclockHz < 0)
		return 0;

	card_signal_disable();

	/* Unlock all */
	card_seq_set(0x08, 0x06); /* PLL Unlock */
	/* VERT timing regs (CRTC index 6, 7(bit0, 2, 3, 5, 7), 9, 10, 11(bits0..3), 15, 16 ) */
	/* HOR timing regs (CRTC index 0..5, 17(bit2) ) */
	/* bit 0 of Clocking mode reg unlocked (8/9 dot font selection) */
	/* Clock bits in MISC reg unlocked */
	card_crtc_set(0x35, card_crtc_get(0x35) & 0x0F);
	card_crtc_set(0x38, 0x48); /* S3 register set (CRTC index 0x30..0x3C) */
	card_crtc_set(0x39, 0xA5); /* system extension regs (CRTC index 0x50..0x5E) */

	card_crtc_all_set(&cp);
	s3_ext_set(&cp);
	s3_clock_set(pll_mul, pll_div, pll_p);
	card_signal_enable();

	return 1;
}



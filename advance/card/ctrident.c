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

/* 3C4 */
#define OldNewModeControl 0x0B

#define RevisionID 0x09
#define NewMode2 0x0D
#define NewMode1 0x0E
#define Protection 0x11
#define MCLKLow 0x16
#define MCLKHigh 0x17
#define ClockLow 0x18
#define ClockHigh 0x19

/* 3x4 */
#define InterlacedVsyncAdjust 0x19
#define HorizontalParamOverflow 0x2B
#define PerformanceTuning 0x2F

#define Offset 0x13
#define CRTCModuleTest 0x1E
#define CRTHiOrd 0x27
#define AddColReg 0x29

/* 3CE */
#define MiscInternalControlReg 0x2F

struct trident_id {
	unsigned value;
	unsigned revision;
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
	int is_3d_chip;
	int is_vclk1;
};

/* Clock type */
#define TRIDENT_CLOCK_OLD 0
#define TRIDENT_CLOCK_NEW 1

/* Configuration */
#define TRIDENT_CONF_UNSUPPORTED 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define TRIDENT_CONF_80 1, 0+8, 121+8, 1+2, 31+2, 0, 1, 14318, 50000, 80000, TRIDENT_CLOCK_OLD
#define TRIDENT_CONF_90 1, 0+8, 121+8, 1+2, 31+2, 0, 1, 14318, 50000, 80000, TRIDENT_CLOCK_OLD
#define TRIDENT_CONF_135 1, 0+8, 121+8, 1+2, 31+2, 0, 1, 14318, 50000, 135000, TRIDENT_CLOCK_OLD
#define TRIDENT_CONF_NEW_135 1, 64+8, 247+8, 1+2, 61+2, 0, 3, 14318, 50000, 135000, TRIDENT_CLOCK_NEW
#define TRIDENT_CONF_NEW_170 1, 64+8, 247+8, 1+2, 61+2, 0, 3, 14318, 50000, 170000, TRIDENT_CLOCK_NEW
#define TRIDENT_CONF_NEW_230 1, 64+8, 247+8, 1+2, 61+2, 0, 3, 14318, 50000, 230000, TRIDENT_CLOCK_NEW

static struct trident_id trident_id_list[] = {
{ 0x9320, -1, "Trident Cyber 9320", TRIDENT_CONF_90, 0, 0 }, /* TESTED */
{ 0x9420, -1, "Trident TGUI 9420", TRIDENT_CONF_80, 0, 0 },
{ 0x9440, -1, "Trident TGUI 9440", TRIDENT_CONF_80, 0, 0 },
{ 0x9660, 0x00, "Trident TGUI 9660", TRIDENT_CONF_135, 0, 0 },
{ 0x9660, 0x01, "Trident TGUI 9680", TRIDENT_CONF_135, 0, 0 },
{ 0x9660, 0x10, "Trident Providia 9682", TRIDENT_CONF_135, 0, 0 },
{ 0x9660, 0x21, "Trident Providia 9685", TRIDENT_CONF_NEW_170, 0, 0 },
{ 0x9660, 0x22, "Trident Cyber 9397", TRIDENT_CONF_NEW_170, 0, 0 },
{ 0x9660, 0x23, "Trident Cyber 9397", TRIDENT_CONF_NEW_170, 0, 0 },
{ 0x9660, 0x2a, "Trident Cyber 9397/DVD", TRIDENT_CONF_NEW_230, 0, 0 },
{ 0x9660, 0x30, "Trident Cyber 9385", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x33, "Trident Cyber 9385", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x34, "Trident Cyber 9385", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x35, "Trident Cyber 9385", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0xB3, "Trident Cyber 9385", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x38, "Trident Cyber 9385-1", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x3A, "Trident Cyber 9385-1", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x40, "Trident Cyber 9382", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x41, "Trident Cyber 9382", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x42, "Trident Cyber 9382", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x43, "Trident Cyber 9382", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x40, "Trident Cyber 9382", TRIDENT_CONF_NEW_135, 0, 0 },
{ 0x9660, 0x4a, "Trident Cyber 9388", TRIDENT_CONF_NEW_170, 0, 0 },
{ 0x9388, -1, "Trident Cyber 9388", TRIDENT_CONF_NEW_170, 1, 0 },
{ 0x9397, -1, "Trident Cyber 9397", TRIDENT_CONF_NEW_170, 1, 0 },
{ 0x939A, -1, "Trident Cyber 939A/DVD", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x9520, -1, "Trident Cyber 9520", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x9525, -1, "Trident Cyber 9525/DVD", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x9540, -1, "Trident Cyber 9540", TRIDENT_CONF_NEW_230, 0, 0 },
{ 0x9750, -1, "Trident 3DImage975", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x9850, -1, "Trident 3DImage985", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x9880, -1, "Trident Blade3D", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x9910, -1, "Trident Blade3D/T64", TRIDENT_CONF_NEW_230, 1, 1 },
{ 0x8400, -1, "Trident CyberBlade/i7", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x8420, -1, "Trident CyberBlade/DSTN/i7", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x8500, -1, "Trident CyberBlade/i1", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0x8520, -1, "Trident CyberBlade/DSTN/i1", TRIDENT_CONF_NEW_230, 1, 0 },
{ 0, 0, TRIDENT_CONF_UNSUPPORTED, 0, 0 }
};

static unsigned trident_bus_device_func;

static struct trident_id* trident_card; /* card detected */

static void trident_doublescan_set(int flag)
{
	if (flag == 0){
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 0, 0));
	} else {
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 1, 0));
	}
}

static void trident_double_logical_line_width_set(int flag)
{
	DWORD d0;

	if (flag != 0) flag = 1;
	d0 = card_graph_get(MiscInternalControlReg);
	d0 = card_bitmov(d0, 2, flag, 0);
	card_graph_set(MiscInternalControlReg, (BYTE)d0);
}

static void trident_interlaced_set(int flag)
{
	BYTE status;

	status = card_crtc_get(CRTCModuleTest) & 0x04;
	if (flag == 0) {
		if (status != 0) {
			trident_double_logical_line_width_set(0);
			card_crtc_set(CRTCModuleTest, card_crtc_get(CRTCModuleTest) & ~0x04);
		}
	} else {
		if (status == 0) {
			trident_double_logical_line_width_set(1);
			card_crtc_set(CRTCModuleTest, card_crtc_get(CRTCModuleTest) | 0x04);
		}
	}
}

static void trident_ext_set(const card_crtc STACK_PTR* cp)
{
	BYTE d0;
	int d1;
	int interlaceoffset;

	d1 = (cp->HSStart/8) + (cp->HSEnd/8) - ((cp->HTotal/8) - 5);
	if (d1 < 0) d1 = 0;
	d1 = (d1 * cp->interlaceratio) / 100;
	d1 = d1 / 2;
	interlaceoffset = d1;
	card_crtc_set(InterlacedVsyncAdjust, (BYTE)(interlaceoffset & 0xFF));

	d0 = card_crtc_get(HorizontalParamOverflow);
	d0 = card_bitmov(d0, 0, (cp->HTotal >> 3) - 5, 9);
	d0 = card_bitmov(d0, 1, (cp->HDisp >> 3) - 1, 9);
	d0 = card_bitmov(d0, 2, interlaceoffset, 9);
	d0 = card_bitmov(d0, 3, (cp->HSStart >> 3), 9);
	d0 = card_bitmov(d0, 4, (cp->HBStart >> 3), 9);
	card_crtc_set(HorizontalParamOverflow, d0);

	d0 = card_crtc_get(CRTHiOrd);
	d0 = card_bitmov(d0, 3, 1, 0); /* line compare bit 10 */
	d0 = card_bitmov(d0, 4, cp->VDisp - 1, 10);
	d0 = card_bitmov(d0, 5, cp->VSStart, 10);
	d0 = card_bitmov(d0, 6, cp->VBStart, 10);
	d0 = card_bitmov(d0, 7, cp->VTotal - 2, 10);
	card_crtc_set(CRTHiOrd, d0);

	trident_doublescan_set(cp->doublescan);
	card_char_size_y_set(1);
	trident_interlaced_set(cp->interlace);
	card_polarity_set(cp->hpolarity, cp->vpolarity);
}

static void trident_clock_divider_set(int d0)
{
	BYTE d1;
	BYTE d2;

	switch(d0){
		case 10: /* div 1 */
			d1 = 0;
			break;
		case 15: /* div 1.5 */
			d1 = 3;
			break;
		case 20: /* div 2 */
			d1 = 1;
			break;
		case 40: /* div 4 */
			d1 = 2;
			break;
		default:
			d1 = 0;
	}

	d1 = d1 << 1;
	d1 = d1 & 0x06;
	d2 = card_seq_get(NewMode2);
	d2 = d2 & ~0x06;
	d2 = d2 | d1;
	card_seq_set(NewMode2, d2);
}

static void trident_clock_set(int Num, int DeN, int PS, int PCDiv)
{
	BYTE d0;
	BYTE d1;
	BYTE d2;

	trident_clock_divider_set(PCDiv);
	card_divider_set(1);

	Num -= 8;
	DeN -= 2;

	if (trident_card->clock_type == TRIDENT_CLOCK_NEW) {
		/* N is all 8bits */
		d1 = Num;
		/* M is first 6bits, with K last 2bits */
		d2 = (DeN & 0x3F) | (PS << 6);
	} else {
		/* N is first 7bits, first M bit is 8th bit */
		d1 = ((DeN & 1) << 7) | Num;
		/* first 4bits are rest of M, 1bit for K value */
		d2 = (((DeN & 0xFE) >> 1) | (PS << 4));
	}

	if (trident_card->is_3d_chip) {
		card_seq_set(ClockLow, d1);
		card_seq_set(ClockHigh, d2);
	} else {
		card_out(0x43C8, d1);
		card_out(0x43C9, d2);
	}

	d0 = (card_in(0x3CC) & 0xF3) | 0x08;
	card_out(0x3C2, d0); /* use vclk2 */
}

static void trident_new_mode_set(void)
{
	/* Reading from index Bh selects new mode registers. */
	BYTE d0;
	d0 = card_seq_get(OldNewModeControl);
	(void)d0;
	CARD_LOG(("trident: chip version %2Xh\n", d0));
}

static void trident_unlock(void)
{
	trident_new_mode_set();
	/* card_seq_set(Protection, 0x87); */ /* VSyncMAME */
	card_seq_set(NewMode1, 0xC0 ^ 0x02);
	card_crtc_set(CRTCModuleTest, card_crtc_get(CRTCModuleTest) & 0xBF);
}

static void trident_set_hsync_skew_in_misc(int flag)
{
	DWORD d0;

	if (flag == 0) {
		flag = 1;
	} else {
		flag = 0; /* no skew */
	}
	d0 = card_graph_get(MiscInternalControlReg);
	d0 = card_bitmov(d0, 5, flag, 0);
	card_graph_set(MiscInternalControlReg, (BYTE)d0);
}

const char* trident_driver(void)
{
	return trident_card->name;
}

int trident_detect(void)
{
	int i;

	if (pci_detect()!=0) {
		CARD_LOG(("trident: PCI BIOS not installed.\n"));
		return 0;
	}

	for(i=0;trident_id_list[i].name;++i) {
		if (pci_find_device(0x00001023, trident_id_list[i].value, 0, &trident_bus_device_func)==0) {
			unsigned revision;
			if (trident_id_list[i].revision == (unsigned)(-1))
				break;
			/* check the revision */
			revision = card_seq_get(RevisionID);
			if (revision == trident_id_list[i].revision)
				break;
		}
	}

	if (!trident_id_list[i].name) {
		return 0;
	}

	CARD_LOG(("trident: found %s, device id %04x, revision %x\n", trident_id_list[i].name, trident_id_list[i].value, trident_id_list[i].revision));

	if (!trident_id_list[i].supported) {
		CARD_LOG(("trident: card not supported\n"));
		return 0;
	}

	trident_card = trident_id_list + i;

	return 1;
}

void trident_reset(void)
{
}

static int trident_pll_validate(int pll_mul, int pll_div, int pll_p)
{
	int mul_invalid[] = {
		/* these are hardware values, the constant 8 must be added to get the real multiplicator value */
		 0, 1, 2, 3, 4, 5, 6, 7,
		 9, 10, 11, 12, 13, 14, 15,
		18, 19, 20, 21, 22, 23,
		27, 28, 29, 30, 31,
		36, 37, 38, 39,
		45, 46, 47,
		54, 55,
		63,
		-1
	};
	int i;

	(void)pll_div;
	(void)pll_p;

	for(i=0;mul_invalid[i]>=0;++i)
		if (pll_mul == 8+mul_invalid[i])
			return -1;
	return 0;
}

int trident_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int pll_mul, pll_div, pll_p, pll_s;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("trident: incompatible mode\n"));
		return 0;
	}

	trident_unlock();

	if (cm->bits_per_pixel > 8 && !trident_card->is_3d_chip) {
		unsigned d0;
		cp.dotclockHz *= 2;
		CARD_LOG(("trident: double the dotclock for 16 bit mode\n"));
/*
3CEh index 0Fh (R/W): Miscellaneous Extended Functions (8900CL/D, 9200 +)
      3  If set character clocks are 16pixels wide rather than 8
*/
		d0 = card_graph_get(0xF);
		if ((d0 & 0x8)==0) {
			d0 |= 0x8;
			card_graph_set(0xF, d0);
			CARD_LOG(("trident: set 16 pixel wide\n"));
		}
	}

	/* compute the external divisor 1.0, 1.5, 2.0, 4.0 */
	pll_s = 10;
	while (cp.dotclockHz * pll_s / 10000 < trident_card->vco_min / (1 << trident_card->p_max)) {
		if (pll_s==10)
			pll_s = 15;
		else if (pll_s==15)
			pll_s = 20;
		else if (pll_s==20)
			pll_s = 40;
		else
			break;
	}
	CARD_LOG(("trident: using divisor %d/10, clock multiplied by %d/10\n", pll_s, pll_s));
	cp.dotclockHz = cp.dotclockHz * pll_s / 10;

	cp.dotclockHz = card_clock_compute(cp.dotclockHz, trident_card->mul_min, trident_card->mul_max, trident_card->div_min, trident_card->div_max, trident_card->p_min, trident_card->p_max, trident_card->ref, trident_card->vco_min, trident_card->vco_max, &pll_mul, &pll_div, &pll_p, trident_pll_validate);
	if (cp.dotclockHz < 0)
		return 0;

	cp.HBStart = cp.HDisp;
	cp.HBEnd = cp.HSEnd;
	cp.VBStart = cp.VDisp;
	cp.VBEnd = cp.VSEnd;

	card_signal_disable();
	card_generic_all_set(&cp);
	trident_ext_set(&cp);
	trident_set_hsync_skew_in_misc(0);
	trident_clock_set(pll_mul, pll_div, pll_p, pll_s);

	if (trident_card->is_vclk1) {
		/* select VCLK1 */
		CARD_LOG(("trident: select VCLK1\n"));
		card_crtc_set(PerformanceTuning, card_crtc_get(PerformanceTuning) | 0x02);
	}

	card_signal_enable();

	return 1;
}


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

/* If defined low freq modes are implemented with a zoom effects */
#define USE_ZOOM

struct matrox_id {
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
};

/* Configuration */
#define MATROX_CONF_UNSUPPORTED 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define MATROX_CONF_MYSTIQUE_ID1 1, 101, 128, 2, 32, 0, 3, 14318, 50000, 135000
#define MATROX_CONF_MYSTIQUE_ID2 1, 101, 128, 2, 32, 0, 3, 14318, 50000, 175000
#define MATROX_CONF_MYSTIQUE_ID3 1, 101, 128, 2, 32, 0, 3, 14318, 50000, 220000
#define MATROX_CONF_G100 1, 8, 128, 2, 7, 0, 3, 27050, 50000, 135000
#define MATROX_CONF_G200 1, 8, 128, 2, 7, 0, 3, 27050, 50000, 230000
#define MATROX_CONF_G400 1, 8, 128, 2, 32, 0, 3, 27050, 50000, 300000

static struct matrox_id matrox_id_list[] = {
{ 0x0518, "Matrox MGA 2085PX", MATROX_CONF_UNSUPPORTED },
{ 0x0519, "Matrox Millenium 2064W", MATROX_CONF_UNSUPPORTED },
{ 0x051A, "Matrox Mystique 1064SG", MATROX_CONF_MYSTIQUE_ID1 },
{ 0x051B, "Matrox Millenium 2 2164W", MATROX_CONF_UNSUPPORTED },
{ 0x051F, "Matrox Millenium 2 AGP 2164W", MATROX_CONF_UNSUPPORTED },
{ 0x0D10, "Matrox Impression", MATROX_CONF_UNSUPPORTED },
{ 0x0520, "Matrox G200 PCI", MATROX_CONF_G200 },
{ 0x0521, "Matrox G200 AGP", MATROX_CONF_G200 }, /* TESTED */
{ 0x0525, "Matrox G400 AGP", MATROX_CONF_G400 }, /* TESTED */ /* the G450 has the same PCI id */
{ 0x1000, "Matrox G100 PCI", MATROX_CONF_G100 },
{ 0x1001, "Matrox G100 AGP", MATROX_CONF_G100 },
{ 0, 0, MATROX_CONF_UNSUPPORTED }
};

static unsigned matrox_bus_device_func;

static struct matrox_id* matrox_card; /* Matrox detected */

static BYTE matrox_idx_get(BYTE _index)
{
	_farpokeb(pci_MMIO_selector_get(), 0x3C00, _index);
	return _farpeekb(pci_MMIO_selector_get(), 0x3C0A);
}

static void matrox_idx_set(BYTE _index, BYTE _data)
{
	_farpokeb(pci_MMIO_selector_get(), 0x3C00, _index);
	_farpokeb(pci_MMIO_selector_get(), 0x3C0A, _data);
}

static BYTE matrox_crtc_get(BYTE _index)
{
	_farpokeb(pci_MMIO_selector_get(), 0x1FDE, _index);
	return _farpeekb(pci_MMIO_selector_get(), 0x1FDF);
}

static void matrox_crtc_set(BYTE _index, BYTE _data)
{
	_farpokeb(pci_MMIO_selector_get(), 0x1FDE, _index);
	_farpokeb(pci_MMIO_selector_get(), 0x1FDF, _data);
}

static DWORD matrox_bytes_per_line_get(void)
{
	DWORD d0;

	d0 = card_crtc_get(0x13);
	d0 = card_bitmov(d0, 8, matrox_crtc_get(0x00), 4);
	d0 = card_bitmov(d0, 9, matrox_crtc_get(0x00), 5);
	return d0;
}

static void matrox_bytes_per_line_set(DWORD d0)
{
	card_crtc_set(0x13, d0 & 0xFF);
	matrox_crtc_set(0x00, card_bitmov(matrox_crtc_get(0x00), 4, d0, 8));
	matrox_crtc_set(0x00, card_bitmov(matrox_crtc_get(0x00), 5, d0, 9));
}

static int matrox_zoom_compute(card_crtc STACK_PTR* cp)
{
	int zoom = 1;
#ifdef USE_ZOOM
	if (cp->HTotal < 512) {
		zoom = 4;
	} else if (cp->HTotal < 1024) {
		zoom = 2;
	} else {
		zoom = 1;
	}
#endif
	return zoom;
}

static void matrox_crtc_adjust(card_crtc STACK_PTR* cp, int hzoom)
{
	cp->HTotal *= hzoom;
	cp->HDisp *= hzoom;
	cp->HBStart *= hzoom;
	cp->HBEnd *= hzoom;
	cp->HSStart *= hzoom;
	cp->HSEnd *= hzoom;
	cp->dotclockHz *= hzoom;

	cp->HBStart = cp->HDisp - 8;
	cp->HBEnd = cp->HTotal - 8;
	cp->VBStart = cp->VDisp - 1;
	cp->VBEnd = cp->VTotal - 1;

	if ( (cp->HBEnd - cp->HBStart) > (0x7F << 3) ) {
		cp->HBEnd = cp->HBStart + (0x7F << 3);
	}
	if ( (cp->HSEnd - cp->HSStart) > (0x1F << 3) ) {
		cp->HSEnd = cp->HSStart + (0x1F << 3);
	}

	if (cp->interlace != 0) {
		if ( (cp->VTotal % 2) != 0 ) {
			cp->VTotal += 1;
			cp->VBEnd = cp->VTotal - 1;
		}
	}
}

static void matrox_interlace_set(int flag, int HSyncStart, int HSyncEnd, int HTotal, int ratio)
{
	int d1;

	if (flag != 0) {
		d1 = matrox_crtc_get(0x00);
		if ((d1 & 0x80) == 0) {
			d1 = (HSyncStart/8) + (HSyncEnd/8) - (HTotal/8);
			d1 = (d1 * ratio) / 100;
			d1 = (d1 / 2) - 1;
			if (d1 < 0) d1 = 0;
			if (d1 > 0xFF) d1 = 0xFF;
			matrox_crtc_set(0x05, d1 & 0xFF);
			matrox_crtc_set(0x00, card_bitmov(matrox_crtc_get(0x00), 7, 1, 0));
			matrox_bytes_per_line_set(matrox_bytes_per_line_get() * 2);
		}
	} else {
		d1 = matrox_crtc_get(0x00);
		if ((d1 & 0x80) != 0) {
			matrox_crtc_set(0x00, card_bitmov(matrox_crtc_get(0x00), 7, 0, 0));
			matrox_crtc_set(0x05, 0);
			matrox_bytes_per_line_set(matrox_bytes_per_line_get() / 2);
		}
	}
}

static void matrox_ext_set(card_crtc STACK_PTR* cp)
{
	int d0;

	d0 = 0;
	d0 = card_bitmov(d0, 0, (cp->HTotal >> 3) - 5, 8);
	d0 = card_bitmov(d0, 1, (cp->HBStart >> 3) , 8);
	d0 = card_bitmov(d0, 2, (cp->HSStart >> 3) , 8);
	d0 = card_bitmov(d0, 6, (cp->HBEnd >> 3) , 6);
	matrox_crtc_set(0x01, d0);

	d0 = 0;
	d0 = card_bitmov(d0, 0, cp->VTotal - 2, 10);
	d0 = card_bitmov(d0, 1, cp->VTotal - 2, 11);
	d0 = card_bitmov(d0, 2, cp->VDisp - 1, 10);
	d0 = card_bitmov(d0, 3, cp->VBStart , 10);
	d0 = card_bitmov(d0, 4, cp->VBStart , 11);
	d0 = card_bitmov(d0, 5, cp->VSStart , 10);
	d0 = card_bitmov(d0, 6, cp->VSStart , 11);
	matrox_crtc_set(0x02, d0);

	matrox_interlace_set(cp->interlace, cp->HSStart, cp->HSEnd, cp->HTotal, cp->interlaceratio);
}

static void matrox_pll_s_compute(int pll_mul, int pll_div, int STACK_PTR* s_s)
{
	long vco = matrox_card->ref * 1000L * pll_mul / pll_div;

	/* From XFree 4.0.0 */
	if (vco/1000 < 100000)
		*s_s = 0;
	else if (vco/1000 < 140000)
		*s_s = 1;
	else if (vco/1000 < 180000)
		*s_s = 2;
	else
		*s_s = 3;
}

static void matrox_clock_set(int N, int M, int P, int S)
{
	int PLLn;
	BYTE d0;

	matrox_idx_set(0x1A, card_bitmov(matrox_idx_get(0x1A), 2, 1, 0));

	PLLn = card_in(0x3CC);
	PLLn = PLLn | 0x0C;
	card_out(0x3C2, PLLn);

	M -= 1;
	N -= 1;
	P = 1 << P; /* P is expressed as a (divisior-1) NOT as the number of shift */
	P -= 1;

	d0 = M & 0x1F;
	matrox_idx_set(0x4C, d0);
	d0 = N & 0x7F;
	matrox_idx_set(0x4D, d0);
	d0 = (P & 0x7) | ((S & 0x3) << 3);
	matrox_idx_set(0x4E, d0);

	matrox_idx_set(0x1A, card_bitmov(matrox_idx_get(0x1A), 2, 0, 0));
}

static void matrox_zoom_set(int d0)
{
	if (d0 == 1) {
		d0 = 0;
	} else if (d0 == 2) {
		d0 = 1;
	} else if (d0 == 4) {
		d0 = 3;
	} else {
		d0 = 0;
	}
	matrox_idx_set(0x38, d0);
}

static void matrox_doublescan_set(int flag)
{
	int d0;

	d0 = card_crtc_get(0x09) & ~0x1F;
	if (flag != 0) {
		d0 = d0 | 0x01;
	}
	card_crtc_set(0x09, d0);
}

const char* matrox_driver(void)
{
	return matrox_card->name;
}

int matrox_detect(void)
{
	int i;

	if (pci_detect()!=0) {
		CARD_LOG(("matrox: PCI BIOS not Installed.\n"));
		return 0;
	}

	for(i=0;matrox_id_list[i].name;++i) {
		if (pci_find_device(0x0000102B, matrox_id_list[i].value, 0, &matrox_bus_device_func)==0)
			break;
	}

	if (!matrox_id_list[i].name) {
		return 0;
	}

	CARD_LOG(("matrox: found %s, device id %04x\n", matrox_id_list[i].name, matrox_id_list[i].value));

	if (!matrox_id_list[i].supported) {
		CARD_LOG(("matrox: card not supported\n"));
		return 0;
	}

	matrox_card = matrox_id_list + i;

	if (pci_MMIO_address_map(matrox_bus_device_func, 0x14, 0xFFFFC000)!=0) {
		CARD_LOG(( "matrox: pci_address_map error\n"));
		return 0;
	}

	return 1;
}

void matrox_reset(void)
{
	pci_MMIO_address_unmap();
}

int matrox_set(const card_crtc* _cp, const card_mode* cm, const card_mode* co)
{
	card_crtc cp = *_cp;
	int pll_mul, pll_div, pll_p, pll_s;
	int hzoom;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("matrox: incompatible mode\n"));
		return 0;
	}

	hzoom = matrox_zoom_compute(&cp);
	CARD_LOG(("matrox: horizontal zoom ratio %d\n", hzoom));
	matrox_crtc_adjust(&cp, hzoom);

	cp.dotclockHz = card_clock_compute(cp.dotclockHz, matrox_card->mul_min, matrox_card->mul_max, matrox_card->div_min, matrox_card->div_max, matrox_card->p_min, matrox_card->p_max, matrox_card->ref, matrox_card->vco_min, matrox_card->vco_max, &pll_mul, &pll_div, &pll_p, 0);
	if (cp.dotclockHz < 0)
		return 0;

	matrox_pll_s_compute(pll_mul, pll_div, &pll_s);

	card_signal_disable();
	card_generic_all_set(&cp);
	matrox_ext_set(&cp);
	matrox_clock_set(pll_mul, pll_div, pll_p, pll_s);
	matrox_zoom_set(hzoom);
	matrox_doublescan_set(cp.doublescan);
	card_polarity_set(cp.hpolarity, cp.vpolarity);
	card_signal_enable();

	return 1;
}


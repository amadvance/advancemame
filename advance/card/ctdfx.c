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

struct tdfx_id {
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
#define TDFX_CONF_UNSUPPORTED 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define TDFX_CONF_BANSHEE 1, 3, 257, 3, 65, 0, 3, 14318, 96000, 270000
#define TDFX_CONF_VOODOO3 1, 3, 257, 3, 65, 0, 3, 14318, 96000, 300000
#define TDFX_CONF_VOODOO5 1, 3, 257, 3, 65, 0, 3, 14318, 96000, 350000

static struct tdfx_id tdfx_id_list[] = {
{ 0x0003, "3dfx Banshee", TDFX_CONF_BANSHEE },
{ 0x0005, "3dfx Voodoo3", TDFX_CONF_VOODOO3 },
{ 0x0009, "3dfx Voodoo5", TDFX_CONF_VOODOO5 },
{ 0, 0, TDFX_CONF_UNSUPPORTED }
};

static int tdfx_io_base;

static unsigned tdfx_bus_device_func;

static struct tdfx_id* tdfx_card; /* 3dfx detected */

static void tdfx_doublescan_set(int flag)
{
	DWORD d0;

	if (flag == 0) {
		d0 = card_inl(tdfx_io_base + 0x5C) & ~0x10;
		card_outl(tdfx_io_base + 0x5C, d0);
	}else{
		d0 = card_inl(tdfx_io_base + 0x5C) | 0x10;
		card_outl(tdfx_io_base + 0x5C, d0);
	}
}

static void tdfx_dac_mode_set(int flag)
{
	DWORD d0;

	flag = (flag != 0) ? 1 : 0;
	d0 = card_inl(tdfx_io_base + 0x4C) | flag;
	card_outl(tdfx_io_base + 0x4C, d0);
}

static void tdfx_screen_size_set(card_crtc STACK_PTR *cp, const card_mode STACK_PTR* cm)
{
	int width;
	int height;
	DWORD d0;
	int d1;

	width = cm->width;
	height = cm->height;
	if (cp->doublescan != 0)
		height *= 2;

	d0 = width | (height << 12);
	card_outl(tdfx_io_base + 0x98, d0);
	d0 = card_inl(tdfx_io_base + 0x98);

	CARD_LOG(("3dfx: %ldx%ld\n", d0 & 0x0FFF, (d0 >> 12) & 0x0FFF));

	d0 = card_inl(tdfx_io_base + 0x5C);
	d0 = d0 & ~0x01;
	card_outl(tdfx_io_base + 0x5C, d0); /* off */
	d1 = 0;
	while (d1 < 2048) {
		d0 = card_inl(tdfx_io_base + 0x5C);
		d1 += 1;
	}
	d0 = d0 | 1;
	card_outl(tdfx_io_base + 0x5C, d0); /* on */
}

static void tdfx_pll_set(void)
{
	BYTE d0;

	d0 = card_in(0x3CC);
	d0 = d0 | 0x0C;
	card_out(0x3C2, d0);
}

static void tdfx_ext_set(card_crtc STACK_PTR *cp)
{
	DWORD d0;

	d0 = 0;
	d0 = card_bitmov(d0, 0, (cp->HTotal >> 3) - 5, 8);
	d0 = card_bitmov(d0, 2, (cp->HDisp >> 3) - 1, 8);
	d0 = card_bitmov(d0, 4, (cp->HBStart >> 3) , 8);
	d0 = card_bitmov(d0, 5, (cp->HBEnd >> 3) , 6);
	d0 = card_bitmov(d0, 6, (cp->HSStart >> 3) , 8);
	d0 = card_bitmov(d0, 7, (cp->HSEnd >> 3) , 5);
	card_crtc_set(0x1A, d0); /* ext horz reg */

	d0 = 0;
	d0 = card_bitmov(d0, 0, cp->VTotal - 2, 10);
	d0 = card_bitmov(d0, 2, cp->VDisp - 1, 10);
	d0 = card_bitmov(d0, 4, cp->VBStart, 10);
	d0 = card_bitmov(d0, 6, cp->VSStart, 10);
	card_crtc_set(0x1B, d0); /* ext vert reg */

	tdfx_doublescan_set(cp->doublescan);
	card_polarity_set(cp->hpolarity, cp->vpolarity);
}

static void tdfx_clock_set(int pll_mul, int pll_div, int pll_p)
{
	DWORD d0;

	tdfx_pll_set();

	d0 = pll_p & 0x3;
	d0 |= (((DWORD)pll_div - 2) & 0x3F) << 2;
	d0 |= (((DWORD)pll_mul - 2) & 0xFF) << 8;
	card_outl(tdfx_io_base + 0x40, d0);
}

const char* tdfx_driver(void)
{
	return "3dfx Voodoo3/Voodoo5/Banshee";
}

int tdfx_detect(void)
{
	int i;
	DWORD reg;

	if (pci_detect()!=0) {
		CARD_LOG(("3dfx: PCI BIOS not installed.\n"));
		return 0;
	}

	for(i=0;tdfx_id_list[i].name;++i) {
		if (pci_find_device(0x0000121A, tdfx_id_list[i].value, 0, &tdfx_bus_device_func)==0)
			break;
	}

	if (!tdfx_id_list[i].name) {
		return 0;
	}

	CARD_LOG(("3dfx: found %s, device id %04x\n", tdfx_id_list[i].name, tdfx_id_list[i].value));

	if (!tdfx_id_list[i].supported) {
		CARD_LOG(("3dfx: card not supported\n"));
		return 0;
	}

	tdfx_card = tdfx_id_list + i;

	if (pci_read_dword(tdfx_bus_device_func, 0x18, &reg)!=0) {
		CARD_LOG(( "3dfx: pci_read_dword\n"));
		return 0;
	}

	tdfx_io_base = reg & 0xFFFFFF00;

	CARD_LOG(("3dfx: iobase %X\n", tdfx_io_base));

	return 1;
}

void tdfx_reset(void)
{
}

int tdfx_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int pll_mul, pll_div, pll_p;
	DWORD d0;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("3dfx: incompatible mode\n"));
		return 0;
	}

	if (cp.interlace) {
		CARD_LOG(("3dfx: interlaced modes not supported\n"));
		return 0;
	}

	cp.dotclockHz = card_clock_compute(cp.dotclockHz, tdfx_card->mul_min, tdfx_card->mul_max, tdfx_card->div_min, tdfx_card->div_max, tdfx_card->p_min, tdfx_card->p_max, tdfx_card->ref, tdfx_card->vco_min, tdfx_card->vco_max, &pll_mul, &pll_div, &pll_p, 0);
	if (cp.dotclockHz < 0)
		return 0;

	cp.HBStart = cp.HSStart;
	cp.HBEnd = cp.HTotal - 8;
	cp.VBStart = cp.VDisp;
	cp.VBEnd = cp.VTotal - 1;

	d0 = card_inl(tdfx_io_base + 0x28);
	d0 = d0 | 0x40;
	card_outl(tdfx_io_base + 0x28, d0); /* activate vga ext register */
	d0 = card_inl(tdfx_io_base + 0x2C);
	d0 = d0 & ~0x1FE00000;
	card_outl(tdfx_io_base + 0x2C, d0); /* unlock vga register */

	card_signal_disable();

	card_generic_all_set(&cp);
	tdfx_ext_set(&cp);
	tdfx_dac_mode_set(0);
	tdfx_clock_set(pll_mul, pll_div, pll_p);
	tdfx_screen_size_set(&cp, cm);

	card_signal_enable();

	return 1;
}


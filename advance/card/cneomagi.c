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
#include "pci.h"

struct neomagic_id {
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
#define NEOMAGIC_CLOCK_128 1
#define NEOMAGIC_CLOCK_256 2

/* Configuration */
#define NEOMAGIC_CONF_UNSUPPORTED 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define NEOMAGIC_CONF_NM207X 1, 1, 128, 1, 32, 0, 1, 14318, 11000, 65000, NEOMAGIC_CLOCK_128
#define NEOMAGIC_CONF_NM209X 1, 1, 128, 1, 32, 0, 1, 14318, 11000, 80000, NEOMAGIC_CLOCK_128
#define NEOMAGIC_CONF_NM21XX 1, 1, 128, 1, 32, 0, 1, 14318, 11000, 90000, NEOMAGIC_CLOCK_128
#define NEOMAGIC_CONF_256 1, 1, 128, 1, 32, 0, 1, 14318, 11000, 110000, NEOMAGIC_CLOCK_256

static struct neomagic_id neomagic_id_list[] = {
{ 0x0001, "NeoMagic MagicGraph 128 (NM2070)", NEOMAGIC_CONF_NM207X },
{ 0x0002, "NeoMagic MagicGraph 128V (NM2090)", NEOMAGIC_CONF_NM209X },
{ 0x0003, "NeoMagic MagicGraph 128ZV (NM2093)", NEOMAGIC_CONF_NM209X },
{ 0x0083, "NeoMagic MagicGraph 128ZV+ (NM2097)", NEOMAGIC_CONF_NM209X },
{ 0x0004, "NeoMagic MagicGraph 128XD (NM2160)", NEOMAGIC_CONF_NM21XX },
{ 0x0005, "NeoMagic MagicMedia 256AV (NM2200)", NEOMAGIC_CONF_256 },
{ 0x0006, "NeoMagic MagicMedia 256ZX (NM2360)", NEOMAGIC_CONF_256 },
{ 0x0016, "NeoMagic MagicMedia 256XL+ (NM2380)", NEOMAGIC_CONF_256 },
{ 0, 0, NEOMAGIC_CONF_UNSUPPORTED }
};

static unsigned neomagic_bus_device_func;

static struct neomagic_id* neomagic_card; /* NeoMagic detected */

static void neomagic_pll_set(void)
{
	BYTE d0;
	d0 = card_in(0x3CC);
	d0 = d0 | 0x0C;
	card_out(0x3C2, d0);
}

static void neomagic_clock_set(int pll_mul, int pll_div, int pll_p)
{
	if (neomagic_card->clock_type == NEOMAGIC_CLOCK_128) {
		BYTE num = (pll_mul - 1) | (pll_p << 7);
		BYTE den = pll_div - 1;
		card_graph_set(0x9B, num);
		card_graph_set(0x9F, den);
	} else if (neomagic_card->clock_type == NEOMAGIC_CLOCK_256) {
		BYTE num_l = pll_mul - 1;
		BYTE num_h = pll_p << 7;
		BYTE den = pll_div - 1;
		BYTE d0;
		card_graph_set(0x9B, num_l);
		d0 = card_graph_get(0x8F);
		d0 &= 0x0F;
		d0 |= num_h & ~0x0F;
		card_graph_set(0x8F, d0);
		card_graph_set(0x9F, den);
	}

	card_divider_set(1);
	neomagic_pll_set();
}

static void neomagic_unlock(void)
{
	card_unlock();
	card_graph_set(0x09, 0x26);
}

static void neomagic_lock(void)
{
	card_graph_set(0x09, 0x00);
	card_lock();
}

static void neomagic_doublescan_set(int flag)
{
	BYTE d0 = card_crtc_get(0x09);
	if (flag == 0) {
		card_crtc_set(0x09, d0 & 0xE0);
	} else {
		card_crtc_set(0x09, (d0 & 0xE0) | 0x80);
	}
}

const char* neomagic_driver(void)
{
	return neomagic_card->name;
}

int neomagic_detect(void)
{
	int i;

	if (pci_detect()!=0) {
		CARD_LOG(("neomagic: PCI BIOS not Installed.\n"));
		return 0;
	}

	for(i=0;neomagic_id_list[i].name;++i) {
		if (pci_find_device(0x000010c8, neomagic_id_list[i].value, 0, &neomagic_bus_device_func)==0)
			break;
	}

	if(!neomagic_id_list[i].name) {
		return 0;
	}

	CARD_LOG(("neomagic: found %s, device id %04x\n", neomagic_id_list[i].name, neomagic_id_list[i].value));

	if (!neomagic_id_list[i].supported) {
		CARD_LOG(("neomagic: card not supported\n"));
		return 0;
	}

	neomagic_card = neomagic_id_list + i;

	return 1;
}

void neomagic_reset(void)
{
}

int neomagic_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int pll_mul, pll_div, pll_p;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("neomagic: incompatible mode\n"));
		return 0;
	}

	if (cp.interlace) {
		CARD_LOG(("neomagic: interlaced modes not supported\n"));
		return 0;
	}

	cp.dotclockHz = card_clock_compute(cp.dotclockHz, neomagic_card->mul_min,  neomagic_card->mul_max, neomagic_card->div_min,  neomagic_card->div_max, neomagic_card->p_min,  neomagic_card->p_max, neomagic_card->ref, neomagic_card->vco_min,  neomagic_card->vco_max, &pll_mul, &pll_div, &pll_p, 0);
	if (cp.dotclockHz < 0)
		return 0;

	neomagic_unlock();
	card_signal_disable();
	card_crtc_all_set(&cp);
	neomagic_clock_set(pll_mul, pll_div, pll_p);
	neomagic_doublescan_set(cp.doublescan);
	card_polarity_set(cp.hpolarity, cp.vpolarity);
	card_signal_enable();
	neomagic_lock();

	return 1;
}


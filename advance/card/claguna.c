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
#include "map.h"

static unsigned laguna_bus_device_func;
static unsigned long laguna_lfb_physical_address = 0;
#ifndef __PCIREAL__
static unsigned long laguna_lfb_linear_address = 0;
static unsigned laguna_lfb_linear_selector = 0;
#endif

static BYTE laguna_mmio_inb(DWORD d0)
{
	return _farpeekb(pci_MMIO_selector_get(), d0);
}

static void laguna_mmio_outb(DWORD d0, BYTE d1)
{
	_farpokeb(pci_MMIO_selector_get(), d0, d1);
}

static WORD laguna_mmio_inw(DWORD d0)
{
	return _farpeekw(pci_MMIO_selector_get(), d0);
}

static void laguna_mmio_outw(DWORD d0, WORD d1)
{
	_farpokew(pci_MMIO_selector_get(), d0, d1);
}

#ifndef __PCIREAL__
static void laguna_lfb_writel(DWORD d0, DWORD d1)
{
	_farpokel(laguna_lfb_linear_selector, d0, d1);
}
#endif

static void laguna_pll_set(void)
{
	BYTE d0;

	d0 = card_in(0x3CC);
	d0 = d0 | 0x0C;
	card_out(0x3C2, d0);
}

static void laguna_doublescan_set(int flag)
{
	if (flag == 0) {
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 0, 0));
	} else {
		card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 1, 0));
	}
}

static void laguna_interlace_set(int flag, card_crtc STACK_PTR *cp)
{
	int d0;

	if (flag == 0) {
		d0 = card_crtc_get(0x1A);
		if ((d0 & 0x01) != 0) {
			d0 = d0 & ~0x01;
			card_crtc_set(0x1A, d0);
		}
	} else {
		d0 = ((cp->HTotal >> 3) - 5) / 2;
		d0 = (d0 * cp->interlaceratio) / 100;
		card_crtc_set(0x19, d0); /* interlace offset */
		d0 = card_crtc_get(0x1A);
		if ((d0 & 0x01) == 0) {
			d0 = d0 | 0x01;
			card_crtc_set(0x1A, d0);
		}
	}
}

static void laguna_ext_register_set(card_crtc STACK_PTR *cp)
{
	int d0;

	d0 = card_crtc_get(0x1B);
	d0 = card_bitmov(d0, 5, 1, 0);
	d0 = card_bitmov(d0, 7, 1, 0);
	card_crtc_set(0x1B, d0); /* enable Blank End ext */

	d0 = 0;
	d0 = card_bitmov(d0, 0, cp->VSStart , 10);
	d0 = card_bitmov(d0, 1, cp->VBStart , 10);
	d0 = card_bitmov(d0, 2, cp->VDisp , 10);
	d0 = card_bitmov(d0, 3, cp->VTotal , 10);
	d0 = card_bitmov(d0, 4, (cp->HSStart >> 3) , 8);
	d0 = card_bitmov(d0, 5, (cp->HBStart >> 3) , 8);
	d0 = card_bitmov(d0, 6, (cp->HDisp >> 3) - 1, 8);
	d0 = card_bitmov(d0, 7, (cp->HTotal >> 3) - 5, 8);
	card_crtc_set(0x1E, d0); /* ext horz vert reg */

	d0 = card_crtc_get(0x1A);
	d0 = card_bitmov(d0, 4, (cp->HBEnd >> 3) , 6);
	d0 = card_bitmov(d0, 5, (cp->HBEnd >> 3) , 7);
	d0 = card_bitmov(d0, 6, cp->VBEnd , 8);
	d0 = card_bitmov(d0, 7, cp->VBEnd , 9);
	card_crtc_set(0x1A, d0); /* ext Blank End reg */

	laguna_doublescan_set(cp->doublescan);
	laguna_interlace_set(cp->interlace, cp);
	card_polarity_set(cp->hpolarity, cp->vpolarity);
}

static void laguna_horzadjustreg_clear(void)
{
	BYTE d0;

	d0 = laguna_mmio_inb(0x70);
	d0 = d0 & 0xC0;
	laguna_mmio_outb(0x70, d0);
}

static DWORD laguna_bytes_per_pixel_get(void)
{
	DWORD d0;

	d0 = (laguna_mmio_inw(0xC0) >> 12) & 0x03;
	d0 += 1;
	return d0;
}

static void laguna_fetchs_per_line_set(DWORD d0)
{
	WORD d1;

	d0 = d0 + 127;
	d0 = d0 / 128;
	d0 = d0 & 0x3F;
	d1 = laguna_mmio_inw(0xEA) & ~0x3F00;
	d1 = d1 | (d0 << 8);
	laguna_mmio_outw(0xEA, d1);
}

#ifndef __PCIREAL__
static void laguna_memory_clear(void)
{
	__dpmi_regs r;
	DWORD installedmemory;
	unsigned loopcounter;

	r.h.ah = 0x12;
	r.h.bl = 0x85;
	__dpmi_int(0x10, &r);
	installedmemory = r.h.al * 65536;
	CARD_LOG(("laguna: Installed Memory %ld MByte\n", installedmemory / (1024 * 1024) ));

	loopcounter = 0;
	while (loopcounter < installedmemory) {
		laguna_lfb_writel(loopcounter, 0);
		loopcounter += 4;
	}
}
#endif

static long laguna_clock_get(long vclk, int STACK_PTR *Num, int STACK_PTR *DeN, int STACK_PTR *PS)
{
	double desfreq;
	double temp;
	double tempdt;
	double dt;
	int d0;
	int d1;

	if (vclk > 175000000) vclk = 175000000;
	*PS = 1;
	if (vclk >= 75000000) {
		*PS = 1;
	} else if (vclk >= 37500000) {
		*PS = 2;
	} else if (vclk >= 13500000) {
		*PS = 4;
	} else {
		*PS = 8;
	}

	desfreq = vclk * (*PS);
	dt = 250000000.0;
	*Num = 0;
	*DeN = 0;
	d0 = 2;
	while (d0 < 126) {
		d1 = 2;
		while (d1 < 126) {
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
	CARD_LOG(("Num %3d, DeN %3d, PS %1d\n", *Num, *DeN, *PS));
	CARD_LOG(("14.318 * (Num/DeN) : %f\n", 14.318 *((*Num)/(*DeN))));
	CARD_LOG(("14.318 * (Num/DeN)/PS : %f\n", 14.318 * ((double)(*Num)/(*DeN))/(*PS)));
	vclk = 14318000 * ((double)(*Num)/(*DeN) / (*PS));
	CARD_LOG(("vclk : %f\n", (double)vclk / 1000000.0));

	return vclk;
}

static void laguna_clock_set(int Num, int DeN, int PS)
{
	DWORD _PS;
	WORD d0;
	BYTE _Num;
	BYTE _DeN;

	laguna_pll_set();

	_PS = 0;
	while (1) {
		PS = PS >> 1;
		if (PS == 0) break;
		_PS += 1;
	}

	_Num = Num & 0x7F;
	_DeN = DeN << 1;
	card_seq_set(0x1E, _Num);
	card_seq_set(0x0E, _DeN);

	d0 = laguna_mmio_inw(0xC0) & ~0xC000;
	d0 = d0 | (_PS << 14);
	laguna_mmio_outw(0xC0, d0);
}

const char* laguna_driver(void)
{
	return "Cirrus Logic GD5462/GD5464/GD5465 Laguna";
}

int laguna_detect(void)
{
	DWORD reg;

	if (pci_detect()!=0) {
		CARD_LOG(("laguna: PCI BIOS not Installed.\n"));
		return 0;
	}

	while (1) {
		if (pci_find_device(0x00001013, 0x000000D0, 0, &laguna_bus_device_func)==0) {
			CARD_LOG(("laguna: CL-GD5462 Found\n"));
			break;
		}
		if (pci_find_device(0x00001013, 0x000000D4, 0, &laguna_bus_device_func)==0) {
			CARD_LOG(("laguna: CL-GD5464 Found\n"));
			break;
		}
		if (pci_find_device(0x00001013, 0x000000D6, 0, &laguna_bus_device_func)==0) {
			CARD_LOG(("laguna: CL-GD5465 Found\n"));
			break;
		}

		return 0;
	}

	if (pci_MMIO_address_map(laguna_bus_device_func, 0x14, 0xffff8000)!=0) {
		CARD_LOG(( "laguna: pci_MMIO_address_map error\n"));
		return 0;
	}

	if (pci_read_dword(laguna_bus_device_func, 0x10, &reg)!=0) {
		CARD_LOG(( "laguna: pci_read_dword\n"));
		return 0;
	}

	laguna_lfb_physical_address = reg & 0xFE000000;
	CARD_LOG(("laguna: LFB Address %08lx\n", laguna_lfb_physical_address));

#ifndef __PCIREAL__
	if (map_create_linear_mapping(&laguna_lfb_linear_address, laguna_lfb_physical_address, 32 * 1024 * 1024) != 0) {
		CARD_LOG(("laguna: card_create_linear_mapping error\n"));
		return 0;
	}
	CARD_LOG(("laguna: LFB Linear Address %08lx\n", laguna_lfb_linear_address));

	if (map_create_selector(&laguna_lfb_linear_selector, laguna_lfb_linear_address, 32 * 1024 * 1024) != 0) {
		map_remove_linear_mapping(laguna_lfb_physical_address, 32 * 1024 * 1024);
		CARD_LOG(("laguna: card_create_selector error\n"));
		return 0;
	}
#endif

	return 1;
}

void laguna_reset(void)
{
	pci_MMIO_address_unmap();
#ifndef __PCIREAL__
	map_remove_linear_mapping(laguna_lfb_physical_address, 32 * 1024 * 1024);
	map_remove_selector(&laguna_lfb_linear_selector);
#endif
}

int laguna_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int Num, DeN, PS;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("laguna: incompatible mode\n"));
		return 0;
	}

	cp.dotclockHz = laguna_clock_get(cp.dotclockHz, &Num, &DeN, &PS);

	cp.HBStart = cp.HSStart;
	cp.HBEnd = cp.HTotal - 8;
	cp.VBStart = cp.VDisp;
	cp.VBEnd = cp.VTotal - 1;

	card_signal_disable();
	card_generic_all_set(&cp);
	laguna_ext_register_set(&cp);
	laguna_clock_set(Num, DeN, PS);
	laguna_horzadjustreg_clear();
	laguna_fetchs_per_line_set(laguna_bytes_per_pixel_get() * cp.HDisp);
#ifndef __PCIREAL__
	laguna_memory_clear();
#endif
	card_signal_enable();

	return 1;
}

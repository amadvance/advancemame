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

#include "card.h"

/***************************************************************************/
/* Interface */

void card_unlock(void)
{
	card_crtc_set(0x11, card_crtc_get(0x11) & ~0x80);
}

void card_lock(void)
{
	card_crtc_set(0x11, card_crtc_get(0x11) | 0x80);
}

void card_crtc_all_set(card_crtc STACK_PTR *cp)
{
	int d0;
	int d1;
	int HDispSkew;
	int HSyncDelay;

	HSyncDelay = 0 & 0x03;
	HDispSkew  = 0 & 0x03;

	/* Unlock */
	card_unlock();

	/* HTotal */
	d0 = cp->HTotal;
	d0 = (d0 / 8) - 5;
	card_crtc_set(0x00, d0 & 0xFF);

	/* HDisp */
	d0 = cp->HDisp;
	d0 = (d0 / 8) - 1;
	card_crtc_set(0x01, d0 & 0xFF);

	/* HBlankStart */
	d0 = cp->HBStart;
	d0 = d0 / 8;
	card_crtc_set(0x02, d0 & 0xFF);

	/* HBlankEnd */
	d0 = cp->HBEnd;
	d0 = d0 / 8;
	d1 = card_crtc_get(0x03);
	d1 = card_bitmov(d1, 0, d0, 0);
	d1 = card_bitmov(d1, 1, d0, 1);
	d1 = card_bitmov(d1, 2, d0, 2);
	d1 = card_bitmov(d1, 3, d0, 3);
	d1 = card_bitmov(d1, 4, d0, 4);
	d1 = card_bitmov(d1, 5, HDispSkew, 0);       /* horizontal disp skew */
	d1 = card_bitmov(d1, 6, HDispSkew, 1);       /* horizontal disp skew */
	d1 = card_bitmov(d1, 7, 1, 0);                       /* compatible read */
	card_crtc_set(0x03, d1);
	card_crtc_set(0x05, card_bitmov(card_crtc_get(0x05), 7, d0, 5));

	/* HSyncStart */
	d0 = cp->HSStart;
	d0 = d0 / 8;
	card_crtc_set(0x04, d0 & 0xFF);

	/* HSyncEnd */
	d0 = cp->HSEnd;
	d0 = d0 / 8;
	d1 = card_crtc_get(0x05);
	d1 = card_bitmov(d1, 0, d0, 0);
	d1 = card_bitmov(d1, 1, d0, 1);
	d1 = card_bitmov(d1, 2, d0, 2);
	d1 = card_bitmov(d1, 3, d0, 3);
	d1 = card_bitmov(d1, 4, d0, 4);
	d1 = card_bitmov(d1, 5, HSyncDelay, 0);
	d1 = card_bitmov(d1, 6, HSyncDelay, 1);
	card_crtc_set(0x05, d1);

	/* VTotal */
	d0 = cp->VTotal;
	d0 = d0 - 2;
	card_crtc_set(0x06, d0 & 0xFF);
	card_crtc_set(0x07, card_bitmov(card_crtc_get(0x07), 0, d0, 8));
	card_crtc_set(0x07, card_bitmov(card_crtc_get(0x07), 5, d0, 9));

	/* VDisp */
	d0 = cp->VDisp;
	d0 = d0 - 1;
	card_crtc_set(0x12, d0 & 0xFF);
	card_crtc_set(0x07, card_bitmov(card_crtc_get(0x07), 1, d0, 8));
	card_crtc_set(0x07, card_bitmov(card_crtc_get(0x07), 6, d0, 9));

	/* VBlankStart */
	d0 = cp->VBStart;
	card_crtc_set(0x15, d0 & 0xFF);
	card_crtc_set(0x07, card_bitmov(card_crtc_get(0x07), 3, d0, 8));
	card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 5, d0, 9));

	/* VBlankEnd */
	d0 = cp->VBEnd;
	card_crtc_set(0x16, d0 & 0xFF);

	/* VSyncStart */
	d0 = cp->VSStart;
	card_crtc_set(0x10, d0 & 0xFF);
	d1 = card_crtc_get(0x07);
	d1 = card_bitmov(d1, 2, d0,  8);  /* bit 8 */
	d1 = card_bitmov(d1, 7, d0,  9);  /* bit 9 */
	card_crtc_set(0x07, d1);

	/* VSyncEnd */
	d0 = cp->VSEnd;
	d0 = d0 & 0x0F;
	d1 = card_crtc_get(0x11) & 0xF0;
	d1 = d1 | d0;
	card_crtc_set(0x11, d1);
}

void card_generic_all_set(card_crtc STACK_PTR *cp)
{
	card_crtc_all_set(cp);

	/* Double Scan Dis. */
	card_crtc_set(0x09, card_bitmov(card_crtc_get(0x09), 7, 0, 0));
}

void card_polarity_set(int HPolar, int VPolar)
{
	BYTE d0;

	if(HPolar != 0) HPolar = 1;
	if(VPolar != 0) VPolar = 1;

	d0 = card_in(0x3CC);
	d0 = d0 & 0x3F;
	d0 = d0 | (VPolar << 7) | (HPolar << 6);
	card_out(0x3C2, d0);
}

DWORD card_bitmov(DWORD dstdata, int dstbitpos, DWORD srcdata, int srcbitpos)
{
	DWORD d0;
	DWORD d1;

	d1 = srcdata >> srcbitpos;
	d1 = d1 & 1;
	d0 = dstdata & ~(1 << dstbitpos);
	d1 = d1 << dstbitpos;
	d0 = d0 | d1;
	return d0;
}

void card_seq_set(BYTE __idx, BYTE data)
{
	card_out(0x3c4, __idx);
	card_out(0x3c5, data);
}

BYTE card_seq_get(BYTE __idx)
{
	BYTE d0;

	card_out(0x3c4, __idx);
	d0 = card_in(0x3c5);
	return d0;
}

BYTE card_attr_get(BYTE __idx)
{
	card_in(0x3da);
	card_out(0x3c0, __idx | (1 << 5));
	return card_in(0x3c1);
}

void card_attr_set(BYTE __idx, BYTE data)
{
	card_in(0x3da);
	card_out(0x3c0, __idx | 0x20);
	card_out(0x3c0, data);
}

int card_crtc_address_get(void)
{
	if((card_in(0x3CC) & 0x01) == 0)
		return 0x3B4;
	else
		return 0x3D4;
}

void card_crtc_set(BYTE __idx, BYTE data)
{
	card_out(card_crtc_address_get(),    __idx);
	card_out(card_crtc_address_get() + 1, data);
}

BYTE card_crtc_get(BYTE __idx)
{
	BYTE d0;
	card_out(card_crtc_address_get(), __idx);
	d0 = card_in(card_crtc_address_get() + 1);
	return d0;
}

void card_graph_set(BYTE __idx, BYTE data)
{
	card_out(0x3CE,  __idx);
	card_out(0x3CE + 1, data);
}

BYTE card_graph_get(BYTE __idx)
{
	BYTE d0;
	card_out(0x3CE, __idx);
	d0 = card_in(0x3CE + 1);
	return d0;
}

int card_divider_get(void)
{
	int d0;
	d0 = (card_seq_get(0x01) & 0x08) >>3;
	d0 = d0 + 1;
	return d0;
}

void card_divider_set(BYTE d0)
{
	assert( d0 == 1 || d0 == 2 );
	d0 -= 1;
	card_seq_set(0x01, (card_seq_get(0x01) & ~0x08) | (d0 << 3));
}

void card_out(int edx, BYTE al)
{
	outportb(edx, al);
}

BYTE card_in(int edx)
{
	return inportb(edx);
}

void card_signal_disable(void)
{
	BYTE d0;

	d0 = card_crtc_get(0x17);
	d0 = d0 & 0x7F;
	card_crtc_set(0x17, d0);
}

void card_signal_enable(void)
{
	BYTE d0;

	d0 = card_crtc_get(0x17);
	d0 = d0 | 0x80;
	card_crtc_set(0x17, d0);
}

void card_doublescan_set(int flag)
{
	BYTE d0 = card_crtc_get(0x09);
	if (!flag)
		d0 &= ~0x80;
	else
		d0 |= 0x80;
	card_crtc_set(0x09, d0);
}

void card_char_size_x_set(unsigned x)
{
	BYTE d0;
	assert(x == 8 || x == 9);
	d0 = card_seq_get(0x01);
	if (x == 8)
		d0 |= 0x01;
	else
		d0 &= ~0x01;
	card_seq_set(0x01, d0);
}

void card_char_size_y_set(unsigned y)
{
	BYTE d0;
	assert(y>=1 && y<=32);
	d0 = card_crtc_get(0x9) & ~0x1F;
	d0 |= y - 1;
	card_crtc_set(0x9, d0);
}

void card_pll_set(unsigned clock)
{
	BYTE d0;
	assert(clock <= 3);
	d0 = card_in(0x3CC);
	d0 = (d0 & ~0xC) | (clock << 2);
	card_out(0x3C2, d0);
}

unsigned card_pll_get()
{
	BYTE d0;
	d0 = card_in(0x3CC);
	d0 = (d0 >> 2) & 0x3;
	return d0;
}

#ifndef __CARDNOEXT__

void card_outw(int edx, WORD ax)
{
	outportw(edx, ax);
}

WORD card_inw(int edx)
{
	return inportw(edx);
}

void card_outl(int edx, DWORD eax)
{
	outportl(edx, eax);
}

DWORD card_inl(int edx)
{
	return inportl(edx);
}

/* Compute the PLL parameters */
long card_clock_compute(long dotclock, int mul_min, int mul_max, int div_min, int div_max, int p_min, int p_max, long ref, long vco_min, long vco_max, int STACK_PTR* s_mul, int STACK_PTR* s_div, int STACK_PTR* s_p, int (*validate)(int, int, int))
{
	int best_mul, best_div, best_p;
	long best_clock, best_vco;
	int pll_mul, pll_div, pll_p;

	CARD_LOG(("card: clock requested %.2f MHz\n", (double)dotclock / 1E6 ));
	CARD_LOG(("card: clock range is %.2f - %.2f MHz\n", (double)vco_min / 1000 / (1 << p_max), (double)vco_max / 1000 / (1 << p_min) ));

	best_clock = -1;
	best_mul = 0;
	best_div = 0;
	best_p = 0;
	best_vco = 0;

	for(pll_div=div_min;pll_div<=div_max;++pll_div)
	for(pll_p=p_min;pll_p<=p_max;++pll_p)
	for(pll_mul=mul_min;pll_mul<=mul_max;++pll_mul) {
		long vco = ref * 1000L * pll_mul / pll_div;
		long new_clock = vco / (1 << pll_p);

		if (vco < vco_min*1000L && pll_p!=p_max)
			continue;
		if (vco > vco_max*1000L)
			continue;
		if (validate && validate(pll_mul, pll_div, pll_p)!=0)
			continue;

		if (best_clock < 0 || labs(new_clock - dotclock) < labs(best_clock - dotclock)) {
			best_mul = pll_mul;
			best_div = pll_div;
			best_p = pll_p;
			best_clock = new_clock;
			best_vco = vco;
		}
	}

	if (best_clock < 0) {
		CARD_LOG(("card: clock out of range\n"));
		return -1;
	}

	if (best_vco < vco_min*1000)
		CARD_LOG(("card: VCO clock %.2f is OUT of range %.2f - %.2f MHz\n", (double)best_vco / 1000000, (double)vco_min / 1000, (double)vco_max / 1000));
	else
		CARD_LOG(("card: VCO clock %.2f is in range %.2f - %.2f MHz\n", (double)best_vco / 1000000, (double)vco_min / 1000, (double)vco_max / 1000));

	*s_mul = best_mul;
	*s_div = best_div;
	*s_p = best_p;

	CARD_LOG(("card: ref:%.2f MHz, mul:%d, div:%d, shift:%d\n", (double)ref / 1000, best_mul, best_div, best_p));
	CARD_LOG(("card: clock selected: %.2f MHz\n", (double)best_clock / 1E6 ));

	return best_clock;
}

#endif

/**
 * Check if two modes are compatible.
 * Ensure that the VBE tweak convert one mode to another.
 */
int card_compatible_mode(const card_mode STACK_PTR* mode0, const card_mode STACK_PTR* mode1)
{
	if (mode0->bits_per_pixel != mode1->bits_per_pixel)
		return 0;
	if (mode0->bytes_per_pixel != mode1->bytes_per_pixel)
		return 0;

	return 1;
}


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include "generate.h"
#include "log.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Normalize the values */
void generate_normalize(adv_generate* generate)
{
	double ht = generate->hactive + generate->hfront +  generate->hsync + generate->hback;
	double vt = generate->vactive + generate->vfront +  generate->vsync + generate->vback;
	/* prevent division by 0 for LCD monitor */
	if (ht) {
		generate->hactive /= ht;
		generate->hfront /= ht;
		generate->hsync /= ht;
		generate->hback /= ht;
	}
	if (vt) {
		generate->vactive /= vt;
		generate->vfront /= vt;
		generate->vsync /= vt;
		generate->vback /= vt;
	}
}

/* Set default value for a VGA monitor */
void generate_default_vga(adv_generate* generate)
{
	generate->hactive = 640;
	generate->hfront = 16;
	generate->hsync = 96;
	generate->hback = 48;
	generate->vactive = 480;
	generate->vfront = 10;
	generate->vsync = 2;
	generate->vback = 33;
}

/* format of the modeline.txt file */
#define hmodeline(dst, a, b, c, d) \
	dst->hactive = a; \
	dst->hsync = b; \
	dst->hback = c-b; \
	dst->hfront = d-c-a

#define vmodeline(dst, a, b, c, d) \
	dst->vactive = a; \
	dst->vsync = b; \
	dst->vback = c-b; \
	dst->vfront = d-c-a

void generate_default_atari_standard(adv_generate* generate)
{
	hmodeline(generate, 46.9, 4.7, 11.9, 63.6);
	vmodeline(generate, 15.3, 0.2, 1.2, 16.7);
}

void generate_default_atari_extended(adv_generate* generate)
{
	hmodeline(generate, 48, 3.9, 11.9, 60.6);
	vmodeline(generate, 17.4, 0.2, 1.2, 18.9);
}

void generate_default_atari_medium(adv_generate* generate)
{
	hmodeline(generate, 32, 4, 7.2, 40);
	vmodeline(generate, 15.4, 0.2, 1.2, 16.7);
}

void generate_default_atari_vga(adv_generate* generate)
{
	hmodeline(generate, 25.6, 4, 5.7, 31.7);
	vmodeline(generate, 12.2, 0.2, 1.1, 14.3);
}

void generate_default_pal(adv_generate* generate)
{
	generate->hactive = 52.00;
	generate->hfront = 1.65;
	generate->hsync = 4.70;
	generate->hback = 5.65;
	generate->vactive = 288.5;
	generate->vfront = 3;
	generate->vsync = 3;
	generate->vback = 18;
}

void generate_default_ntsc(adv_generate* generate)
{
	generate->hactive = 52.60;
	generate->hfront = 1.50;
	generate->hsync = 4.70;
	generate->hback = 4.70;
	generate->vactive = 242.5;
	generate->vfront = 3;
	generate->vsync = 3;
	generate->vback = 14;
}

void generate_default_lcd(adv_generate* generate)
{
	/* No generation for LCD monitor */
	generate->hactive = 0;
	generate->hfront = 0;
	generate->hsync = 0;
	generate->hback = 0;
	generate->vactive = 0;
	generate->vfront = 0;
	generate->vsync = 0;
	generate->vback = 0;
}

adv_error generate_find(adv_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const adv_monitor* monitor, const adv_generate* generate, unsigned capability, unsigned adjust)
{
	adv_generate norm = *generate;
	unsigned vtotal;
	double factor;

	crtc_reset(crtc);

	crtc_name_set(crtc, "generate");

	/* use normalized values */
	generate_normalize(&norm);

	/* required */
	vtotal = crtc_step(vsize / norm.vactive, CRTC_VSTEP);
	factor = 0;

	if (crtc_find(&vtotal, &vclock, &factor, monitor, capability, adjust)!=0)
		return -1;

	/* doublescan/interlace */
	if (factor == 1.0) {
		crtc_singlescan_set(crtc);
	} else if (factor == 2.0) {
		crtc_doublescan_set(crtc);
	} else if (factor == 0.5) {
		crtc_interlace_set(crtc);
	} else
		return -1;

	/* compute the horizontal crtc */
	crtc->hde = crtc_step(hsize, CRTC_HSTEP);
	crtc->ht = crtc_step(crtc->hde / norm.hactive, CRTC_HSTEP);
	crtc->hrs = crtc_step(crtc->ht * (norm.hactive + norm.hfront), CRTC_HSTEP);
	crtc->hre = crtc_step(crtc->ht * (norm.hactive + norm.hfront + norm.hsync), CRTC_HSTEP);
	if (crtc->hrs == crtc->hre)
		crtc->hre = crtc->hrs + CRTC_HSTEP;

	/* compute the vertical crtc */
	crtc->vt = crtc_step(vtotal, CRTC_VSTEP);
	crtc->vde = crtc_step(crtc->vt * norm.vactive, CRTC_VSTEP);
	if (abs(crtc->vde - vsize) <= CRTC_VSTEP) /* solve precision problem */
		crtc->vde = crtc_step(vsize, CRTC_VSTEP);
	crtc->vrs = crtc_step(crtc->vt * (norm.vactive + norm.vfront), CRTC_VSTEP);
	crtc->vre = crtc_step(crtc->vt * (norm.vactive + norm.vfront + norm.vsync), CRTC_VSTEP);
	if (crtc->vrs == crtc->vre)
		crtc->vre = crtc->vrs + CRTC_VSTEP;

	/* pixelclock */
	crtc->pixelclock = vclock * factor * (crtc->vt * crtc->ht);

	/* final check, this should never fail */
	if (!crtc_clock_check(monitor, crtc))
		return -1;

	return 0;
}

adv_error generate_find_interpolate(adv_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const adv_monitor* monitor, const adv_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust)
{
	adv_generate norm = interpolate->map[0].gen;
	unsigned vtotal;
	double factor;

	crtc_reset(crtc);

	crtc_name_set(crtc, "generate");

	/* use normalized values */
	generate_normalize(&norm);

	/* compute the VTOTAL from the first interpolate record */
	/* this assume that ALL the interpolate record have the same vertical format */
	vtotal = crtc_step(vsize / norm.vactive, CRTC_VSTEP);
	factor = 0;

	if (crtc_find(&vtotal, &vclock, &factor, monitor, capability, adjust)!=0)
		return -1;

	/* now the h/v clock are know and the format can be computed exactly */
	generate_interpolate_h(&norm, vclock * vtotal * factor, interpolate);

	/* doublescan/interlace */
	if (factor == 1.0) {
		crtc_singlescan_set(crtc);
	} else if (factor == 2.0) {
		crtc_doublescan_set(crtc);
	} else if (factor == 0.5) {
		crtc_interlace_set(crtc);
	} else
		return -1;

	/* compute the horizontal crtc */
	crtc->hde = crtc_step(hsize, CRTC_HSTEP);
	crtc->ht = crtc_step(crtc->hde / norm.hactive, CRTC_HSTEP);
	crtc->hrs = crtc_step(crtc->ht * (norm.hactive + norm.hfront), CRTC_HSTEP);
	crtc->hre = crtc_step(crtc->ht * (norm.hactive + norm.hfront + norm.hsync), CRTC_HSTEP);
	if (crtc->hrs == crtc->hre)
		crtc->hre = crtc->hrs + CRTC_HSTEP;

	/* compute the vertical crtc */
	crtc->vt = crtc_step(vtotal, CRTC_VSTEP);
	crtc->vde = crtc_step(crtc->vt * norm.vactive, CRTC_VSTEP);
	if (abs(crtc->vde - vsize) <= CRTC_VSTEP) /* solve precision problem */
		crtc->vde = crtc_step(vsize, CRTC_VSTEP);
	crtc->vrs = crtc_step(crtc->vt * (norm.vactive + norm.vfront), CRTC_VSTEP);
	crtc->vre = crtc_step(crtc->vt * (norm.vactive + norm.vfront + norm.vsync), CRTC_VSTEP);
	if (crtc->vrs == crtc->vre)
		crtc->vre = crtc->vrs + CRTC_VSTEP;

	/* pixelclock */
	crtc->pixelclock = vclock * factor * (crtc->vt * crtc->ht);

	/* final check, this should never fail */
	if (!crtc_clock_check(monitor, crtc))
		return -1;

	return 0;
}

adv_error generate_find_interpolate_multi(adv_crtc* crtc, unsigned hsize0, unsigned vsize0, unsigned hsize1, unsigned vsize1, unsigned hsize2, unsigned vsize2, unsigned hsize3, unsigned vsize3, double vclock, const adv_monitor* monitor, const adv_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust)
{
	adv_error err;

	assert(hsize0 != 0 && vsize0 != 0);

	err = generate_find_interpolate(crtc, hsize0, vsize0, vclock, monitor, interpolate, capability, adjust);

	/* only if the vclock isn't already changed */
	if ((adjust & GENERATE_ADJUST_VCLOCK) == 0) {
		/* try with the double or more vclock */
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize0, vsize0, vclock*2, monitor, interpolate, capability, adjust);
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize0, vsize0, vclock*3, monitor, interpolate, capability, adjust);
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize0, vsize0, vclock*4, monitor, interpolate, capability, adjust);
	}

	/* try with *2 */
	if (hsize1 != 0) {
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize1, vsize0, vclock, monitor, interpolate, capability, adjust);
		if ((adjust & GENERATE_ADJUST_VCLOCK) == 0) {
			if (err != 0)
				err = generate_find_interpolate(crtc, hsize1, vsize0, 2*vclock, monitor, interpolate, capability, adjust);
		}
	}
	if (vsize1 != 0) {
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize0, vsize1, vclock, monitor, interpolate, capability, adjust);
		if ((adjust & GENERATE_ADJUST_VCLOCK) == 0) {
			if (err != 0)
				err = generate_find_interpolate(crtc, hsize0, vsize1, 2*vclock, monitor, interpolate, capability, adjust);
		}
	}

	/* try with a *2*2 */
	if (hsize1 != 0 && vsize1 != 0) {
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize1, vsize1, vclock, monitor, interpolate, capability, adjust);
	}

	/* try with *3 */
	if (hsize2 != 0) {
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize2, vsize0, vclock, monitor, interpolate, capability, adjust);
		if ((adjust & GENERATE_ADJUST_VCLOCK) == 0) {
			if (err != 0)
				err = generate_find_interpolate(crtc, hsize2, vsize0, 2*vclock, monitor, interpolate, capability, adjust);
		}
	}
	if (vsize2 != 0) {
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize0, vsize2, vclock, monitor, interpolate, capability, adjust);
		if ((adjust & GENERATE_ADJUST_VCLOCK) == 0) {
			if (err != 0)
				err = generate_find_interpolate(crtc, hsize0, vsize2, 2*vclock, monitor, interpolate, capability, adjust);
		}
	}

	/* try with *4 */
	if (hsize3 != 0) {
		if (err != 0)
		err = generate_find_interpolate(crtc, hsize3, vsize0, vclock, monitor, interpolate, capability, adjust);
	}
	if (vsize3 != 0) {
		if (err != 0)
			err = generate_find_interpolate(crtc, hsize0, vsize3, vclock, monitor, interpolate, capability, adjust);
	}

	return err;
}

static double pos(double v)
{
	if (v<0)
		return 0;
	else
		return v;
}

static void generate_interpolate_from2(adv_generate* generate, unsigned hclock, const adv_generate_interpolate* e0, const adv_generate_interpolate* e1)
{
	if (e1->hclock != e0->hclock) {
		/* linear interpolation on the hclock */
		double f1 = (hclock - (double)e0->hclock) / (e1->hclock - (double)e0->hclock);
		double f0 = 1 - f1;

#if 1
		/* compute the horizontal format with the linear interpolation */
		generate->hactive = pos(e0->gen.hactive * f0 + e1->gen.hactive * f1);
		generate->hfront = pos(e0->gen.hfront * f0 + e1->gen.hfront * f1);
		generate->hsync = pos(e0->gen.hsync * f0 + e1->gen.hsync * f1);
		generate->hback = pos(e0->gen.hback * f0 + e1->gen.hback * f1);
#else
		/* I have some doubts on the correctness of this approach */
		/* It computes correctly the size, but it may */
		/* lose the correct centering */

		/* compute the horizontal format with the gtf interpolation */
		double duty_cycle;
		double active;
		double blank;
		double sync;
		double front;
		double back;
		double c;
		double m;

		/* horizontal total */
		double t0 = e0->gen.hactive + e0->gen.hfront + e0->gen.hsync + e0->gen.hback;
		double t1 = e1->gen.hactive + e1->gen.hfront + e1->gen.hsync + e1->gen.hback;

		/* hclock */
		double h0 = e0->hclock;
		double h1 = e1->hclock;

		/* duty cycle */
		double d0 = (t0 - e0->gen.hactive) / t0;
		double d1 = (t1 - e1->gen.hactive) / t1;

		/* compute c and m */
		c = (d0*h0 - d1*h1) / (h0 - h1);
		m = h1*c - d1*h1;

		/* compute duty cycle */
		duty_cycle = c - m / hclock;

		active = 1 - duty_cycle;
		blank = duty_cycle;
		sync = e0->gen.hsync / t0;
		front = blank/2 - sync;
		back = 1 - active - sync - front;

		generate->hactive = pos(active);
		generate->hfront = pos(front);
		generate->hsync = pos(sync);
		generate->hback = pos(back);
#endif

		/* vertical data with the linear interpolation */
		generate->vactive = pos(e0->gen.vactive * f0 + e1->gen.vactive * f1);
		generate->vfront = pos(e0->gen.vfront * f0 + e1->gen.vfront * f1);
		generate->vsync = pos(e0->gen.vsync * f0 + e1->gen.vsync * f1);
		generate->vback = pos(e0->gen.vback * f0 + e1->gen.vback * f1);
	} else {
		generate->hactive = e0->gen.hactive;
		generate->hfront = e0->gen.hfront;
		generate->hsync = e0->gen.hsync;
		generate->hback = e0->gen.hback;
		generate->vactive = e0->gen.vactive;
		generate->vfront = e0->gen.vfront;
		generate->vsync = e0->gen.vsync;
		generate->vback = e0->gen.vback;
	}

	generate_normalize(generate);
}

void generate_interpolate_h(adv_generate* generate, unsigned hclock, const adv_generate_interpolate_set* interpolate)
{
	unsigned i;

	const adv_generate_interpolate* e0 = 0;
	const adv_generate_interpolate* e1 = 0;

	assert( interpolate->mac );

	for(i=0;i<interpolate->mac;++i) {
		const adv_generate_interpolate* e = interpolate->map + i;
		if (e->hclock <= hclock && (!e0 || e0->hclock < e->hclock))
			e0 = e;
		if (e->hclock >= hclock && (!e1 || e1->hclock > e->hclock))
			e1 = e;
	}

	if (e0 && e1)
		generate_interpolate_from2(generate, hclock, e0, e1);
	else if (e0) {
		*generate = e0->gen;
		generate_normalize(generate);
	} else if (e1) {
		*generate = e1->gen;
		generate_normalize(generate);
	}
}

static void generate_crtc_h(adv_crtc* crtc, unsigned hsize, unsigned vsize, const adv_generate* generate_norm)
{
	crtc->hde = crtc_step((double)hsize, CRTC_HSTEP);
	crtc->ht = crtc_step(crtc->hde / generate_norm->hactive, CRTC_HSTEP);
	crtc->hrs = crtc_step(crtc->ht * (generate_norm->hactive + generate_norm->hfront), CRTC_HSTEP);
	crtc->hre = crtc_step(crtc->ht * (generate_norm->hactive + generate_norm->hfront + generate_norm->hsync), CRTC_HSTEP);

	if (crtc->hrs == crtc->hre)
		crtc->hre = crtc->hrs + CRTC_HSTEP;
}

static void generate_crtc_v(adv_crtc* crtc, unsigned hsize, unsigned vsize, const adv_generate* generate_norm)
{
	crtc->vde = crtc_step((double)vsize, CRTC_VSTEP);
	crtc->vt = crtc_step(crtc->vde / generate_norm->vactive, CRTC_VSTEP);
	crtc->vrs = crtc_step(crtc->vt * (generate_norm->vactive + generate_norm->vfront), CRTC_VSTEP);
	crtc->vre = crtc_step(crtc->vt * (generate_norm->vactive + generate_norm->vfront + generate_norm->vsync), CRTC_VSTEP);

	if (crtc->vrs == crtc->vre)
		crtc->vre = crtc->vrs + CRTC_VSTEP;
}

/* Generate the CRTC values */
void generate_crtc(adv_crtc* crtc, unsigned hsize, unsigned vsize, const adv_generate* generate_free)
{
	adv_generate generate_norm = *generate_free;

	generate_normalize(&generate_norm);

	generate_crtc_h(crtc, hsize, vsize, &generate_norm);
	generate_crtc_v(crtc, hsize, vsize, &generate_norm);
}

void generate_interpolate_reset(adv_generate_interpolate_set* interpolate)
{
	interpolate->mac = 0;
}

adv_bool generate_interpolate_is_empty(const adv_generate_interpolate_set* interpolate)
{
	return interpolate->mac == 0;
}

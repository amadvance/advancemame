/*
 * This file is part of the AdvanceMAME project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Normalize the values */
void generate_normalize(video_generate* generate) {
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
void generate_default_vga(video_generate* generate) {
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
#define hmodeline(dst,a,b,c,d) \
	dst->hactive = a; \
	dst->hsync = b; \
	dst->hback = c-b; \
	dst->hfront = d-c-a

#define vmodeline(dst,a,b,c,d) \
	dst->vactive = a; \
	dst->vsync = b; \
	dst->vback = c-b; \
	dst->vfront = d-c-a

void generate_default_atari_standard(video_generate* generate) {
	hmodeline(generate,46.9,4.7,11.9,63.6);
	vmodeline(generate,15.3,0.2,1.2,16.7);
}

void generate_default_atari_extended(video_generate* generate) {
	hmodeline(generate,48,3.9,11.9,60.6);
	vmodeline(generate,17.4,0.2,1.2,18.9);
}

void generate_default_atari_medium(video_generate* generate) {
	hmodeline(generate,32,4,7.2,40);
	vmodeline(generate,15.4,0.2,1.2,16.7);
}

void generate_default_atari_vga(video_generate* generate) {
	hmodeline(generate,25.6,4,5.7,31.7);
	vmodeline(generate,12.2,0.2,1.1,14.3);
}

void generate_default_pal(video_generate* generate) {
	generate->hactive = 52.00;
	generate->hfront = 1.65;
	generate->hsync = 4.70;
	generate->hback = 5.65;
	generate->vactive = 288.5;
	generate->vfront = 3;
	generate->vsync = 3;
	generate->vback = 18;
}

void generate_default_ntsc(video_generate* generate) {
	generate->hactive = 52.60;
	generate->hfront = 1.50;
	generate->hsync = 4.70;
	generate->hback = 4.70;
	generate->vactive = 242.5;
	generate->vfront = 3;
	generate->vsync = 3;
	generate->vback = 14;
}

void generate_default_lcd(video_generate* generate) {
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

video_error generate_find(video_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const video_monitor* monitor, const video_generate* generate, unsigned capability, unsigned adjust) {
	video_generate norm = *generate;
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
	if (!crtc_clock_check(monitor,crtc))
		return -1;

	return 0;
}

video_error generate_find_interpolate(video_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const video_monitor* monitor, const video_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust) {
	video_generate norm = interpolate->map[0].gen;
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
	if (!crtc_clock_check(monitor,crtc))
		return -1;

	return 0;
}

video_error generate_find_interpolate_double(video_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const video_monitor* monitor, const video_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust) {
	video_error err;

	err = generate_find_interpolate(crtc, hsize, vsize, vclock, monitor, interpolate, capability, adjust);

	/* try with the double or more vclock only if the vclock isn't already changed */
	if (err != 0 && (adjust & GENERATE_ADJUST_VCLOCK) == 0)
		err = generate_find_interpolate(crtc, hsize, vsize, vclock*2, monitor, interpolate, capability, adjust);
	if (err != 0 && (adjust & GENERATE_ADJUST_VCLOCK) == 0)
		err = generate_find_interpolate(crtc, hsize, vsize, vclock*3, monitor, interpolate, capability, adjust);
	if (err != 0 && (adjust & GENERATE_ADJUST_VCLOCK) == 0)
		err = generate_find_interpolate(crtc, hsize, vsize, vclock*4, monitor, interpolate, capability, adjust);

	/* try with double or more horizontal size */
	if (err != 0)
		err = generate_find_interpolate(crtc, 2*hsize, vsize, vclock, monitor, interpolate, capability, adjust);
	if (err != 0)
		err = generate_find_interpolate(crtc, 3*hsize, vsize, vclock, monitor, interpolate, capability, adjust);
	if (err != 0)
		err = generate_find_interpolate(crtc, 4*hsize, vsize, vclock, monitor, interpolate, capability, adjust);

	return err;
}

static double pos(double v) {
	if (v<0)
		return 0;
	else
		return v;
}

static void generate_interpolate_from2(video_generate* generate, unsigned hclock, const video_generate_interpolate* e0, const video_generate_interpolate* e1) {
	if (e1->hclock != e0->hclock) {
		double f1 = (hclock - (double)e0->hclock) / (e1->hclock - (double)e0->hclock);
		double f0 = 1 - f1;
		generate->hactive = pos(e0->gen.hactive * f0 + e1->gen.hactive * f1);
		generate->hfront = pos(e0->gen.hfront * f0 + e1->gen.hfront * f1);
		generate->hsync = pos(e0->gen.hsync * f0 + e1->gen.hsync * f1);
		generate->hback = pos(e0->gen.hback * f0 + e1->gen.hback * f1);
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

void generate_interpolate_h(video_generate* generate, unsigned hclock, const video_generate_interpolate_set* interpolate) {
	unsigned i;

	const video_generate_interpolate* e0 = 0;
	const video_generate_interpolate* e1 = 0;

	assert( interpolate->mac );

	for(i=0;i<interpolate->mac;++i) {
		const video_generate_interpolate* e = interpolate->map + i;
		if (e->hclock <= hclock && (!e0 || e0->hclock < e->hclock))
			e0 = e;
		if (e->hclock >= hclock && (!e1 || e1->hclock > e->hclock))
			e1 = e;
	}

	if (e0 && e1)
		generate_interpolate_from2(generate,hclock,e0,e1);
	else if (e0) {
		*generate = e0->gen;
		generate_normalize(generate);
	} else if (e1) {
		*generate = e1->gen;
		generate_normalize(generate);
	}
}

static void generate_crtc_h(video_crtc* crtc, unsigned hsize, unsigned vsize, const video_generate* generate_norm)
{
	crtc->hde = crtc_step((double)hsize, CRTC_HSTEP);
	crtc->ht = crtc_step(crtc->hde / generate_norm->hactive, CRTC_HSTEP);
	crtc->hrs = crtc_step(crtc->ht * (generate_norm->hactive + generate_norm->hfront), CRTC_HSTEP);
	crtc->hre = crtc_step(crtc->ht * (generate_norm->hactive + generate_norm->hfront + generate_norm->hsync), CRTC_HSTEP);

	if (crtc->hrs == crtc->hre)
		crtc->hre = crtc->hrs + CRTC_HSTEP;
}

static void generate_crtc_v(video_crtc* crtc, unsigned hsize, unsigned vsize, const video_generate* generate_norm)
{
	crtc->vde = crtc_step((double)vsize, CRTC_VSTEP);
	crtc->vt = crtc_step(crtc->vde / generate_norm->vactive, CRTC_VSTEP);
	crtc->vrs = crtc_step(crtc->vt * (generate_norm->vactive + generate_norm->vfront), CRTC_VSTEP);
	crtc->vre = crtc_step(crtc->vt * (generate_norm->vactive + generate_norm->vfront + generate_norm->vsync), CRTC_VSTEP);

	if (crtc->vrs == crtc->vre)
		crtc->vre = crtc->vrs + CRTC_VSTEP;
}

/* Generate the CRTC values */
void generate_crtc(video_crtc* crtc, unsigned hsize, unsigned vsize, const video_generate* generate_free) {
	video_generate generate_norm = *generate_free;

	generate_normalize(&generate_norm);

	generate_crtc_h(crtc,hsize,vsize,&generate_norm);
	generate_crtc_v(crtc,hsize,vsize,&generate_norm);
}

void generate_interpolate_reset(video_generate_interpolate_set* interpolate) {
	interpolate->mac = 0;
}

video_bool generate_interpolate_is_empty(const video_generate_interpolate_set* interpolate) {
	return interpolate->mac == 0;
}

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

#include "portable.h"

#include "crtcbag.h"
#include "video.h"

adv_bool crtc_container_is_empty(const adv_crtc_container* cc)
{
	return cc->base == 0;
}

/**
 * Initialize the video crtc container.
 * \note The container MUST be deinitialized.
 */
void crtc_container_init(adv_crtc_container* cc)
{
	cc->base = 0;
}

/**
 * Deinitialize the video crtc container
 */
void crtc_container_done(adv_crtc_container* cc)
{
	adv_crtc* p = cc->base;
	while (p) {
		adv_crtc* todelete = p;
		p = p->container_next;
		free(todelete);
	}
}

/**
 * Check if a crtc is already in container.
 * \return
 *   - ==0 if not found
 *   - !=0 the first found
 */
const adv_crtc* crtc_container_has(adv_crtc_container* cc, const adv_crtc* vm, int (*compare)(const adv_crtc* a, const adv_crtc* b))
{
	adv_crtc* p = cc->base;
	while (p) {
		if (compare(p, vm)==0)
			return p;
		p = p->container_next;
	}
	return 0;
}

/**
 * Remove some crtc from the container.
 * All the crtc for which the select function returns !=0 are removed.
 * \param cc Container to process.
 * \param select Selection function.
 * \param arg Arg passed at the selection function.
 */
void crtc_container_remove(adv_crtc_container* cc, adv_bool (*select)(const adv_crtc*, void*), void* arg)
{
	while (cc->base && select(cc->base, arg)) {
		adv_crtc* todelete = cc->base;
		cc->base = cc->base->container_next;
		free(todelete);
	}
	if (cc->base) {
		adv_crtc* p = cc->base;
		while (p->container_next) {
			if (select(p->container_next, arg)) {
				adv_crtc* todelete = p->container_next;
				p->container_next = p->container_next->container_next;
				free(todelete);
			} else
				p = p->container_next;
		}
	}
}

/**
 * Insert a crtc in the container.
 */
const adv_crtc* crtc_container_insert(adv_crtc_container* cc, const adv_crtc* vm)
{
	adv_crtc* p = malloc(sizeof(adv_crtc));
	*p = *vm;
	p->container_next = cc->base;
	cc->base = p;
	return p;
}

/**
 * Insert a crtc in a sorted container.
 */
const adv_crtc* crtc_container_insert_sort(adv_crtc_container* cc,  const adv_crtc* vm, int (*compare)(const adv_crtc* a, const adv_crtc* b))
{
	if (cc->base && compare(cc->base, vm)<=0) {
		adv_crtc* p = cc->base;
		adv_crtc* c;
		while (p->container_next && compare(p->container_next, vm)<=0) {
			p = p->container_next;
		}
		c = malloc(sizeof(adv_crtc));
		*c = *vm;
		c->container_next = p->container_next;
		p->container_next = c;
		return c;
	} else {
		adv_crtc* p = malloc(sizeof(adv_crtc));
		*p = *vm;
		p->container_next = cc->base;
		cc->base = p;
		return p;
	}
}

void crtc_container_iterator_begin(adv_crtc_container_iterator* cci, adv_crtc_container* cc)
{
	cci->base = cc->base;
}

void crtc_container_iterator_next(adv_crtc_container_iterator* cci)
{
	cci->base = cci->base->container_next;
}

adv_bool crtc_container_iterator_is_end(adv_crtc_container_iterator* cci)
{
	return cci->base == 0;
}

adv_crtc* crtc_container_iterator_get(adv_crtc_container_iterator* cci)
{
	return cci->base;
}

/** Modes for the VGA BIOS */
static const char* BIOS_VGA[] = {
"pc_bios_vga_text40_8x8 12.5876 320 328 376 400 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_bios_vga_text40_9x8 14.161 360 368 422 450 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_bios_vga_text80_8x8 25.1752 640 656 752 800 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_bios_vga_text80_9x8 28.3221 720 738 846 900 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_bios_vga_text40_8x16 12.5876 320 328 376 400 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_bios_vga_text40_9x16 14.161 360 368 422 450 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_bios_vga_text80_8x16 25.1752 640 656 752 800 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_bios_vga_text80_9x16 28.3221 720 738 846 900 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_bios_vga_320x200 12.5876 320 328 376 400 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
0
};

/** Modes for the VBE BIOS */
static const char* BIOS_VBE[] = {
"pc_bios_vbe_320x240 12.576 320 328 376 400 240 245 246 262 -hsync -vsync doublescan", /* H 31440 [Hz], V 60 [Hz] */
"pc_bios_vbe_400x300 19.5226 400 408 464 496 300 306 307 328 -hsync -vsync doublescan", /* H 39360 [Hz], V 60 [Hz] */
"pc_bios_vbe_512x384 32.256 512 528 600 640 384 392 394 420 -hsync -vsync doublescan", /* H 50400 [Hz], V 60 [Hz] */
"pc_bios_vbe_640x480 25.2 640 656 752 800 480 490 492 525 -hsync -vsync", /* H 31500 [Hz], V 60 [Hz] */
"pc_bios_vbe_800x600 39.36 800 816 936 1000 600 612 615 656 -hsync -vsync", /* H 39360 [Hz], V 60 [Hz] */
"pc_bios_vbe_1024x768 64.512 1024 1048 1200 1280 768 784 787 840 -hsync -vsync", /* H 50400 [Hz], V 60 [Hz] */
0
};

static const char* MODELINE_VGA[] = {

/* Arcade 15 kHz TEXT resolution, from ArcadeOS */
/* "arcade_15.75_vga_text 14.16 640 712 792 880 200 225 227 267 -hsync -vsync", */

/* from VGA2PAL */
/*"pal_vga_text_vga2pal 14.16 643 700 828 896 256 276 280 311 -hsync -vsync", */

/* from VGA2NTSC */
/* "ntsc_vga_text_vga2ntsc 14.16 640 736 800 904 200 220 229 263 -hsync -vsync", */

#include "vgamode.dat"

0
};

/* the mode number is set later */
static const char* MODELINE_SVGA[] = {

/* PC MONITOR AT 37.0 kHz */
"pc_37.0_1024x768 47.39 1024 1040 1128 1280 768 770 780 800 -hsync -vsync interlace",

/* PC MONITOR AT 48.0 kHz */
"pc_48.0_512x384 31.10 512 520 552 648 384 385 390 400 doublescan -hsync -vsync",
"pc_48.0_1024x768 61.44 1024 1040 1128 1280 768 770 780 800 -hsync -vsync",

#include "svgamode.dat"

0
};

static adv_error crtc_container_insert_default(adv_crtc_container* cc, const char** modes)
{
	while (*modes) {
		adv_crtc crtc;
		if (crtc_parse(&crtc, *modes, *modes + strlen(*modes))!=0)
			return -1;
		/* insert only if unique */
		if (!crtc_container_has(cc, &crtc, &crtc_compare))
			crtc_container_insert(cc, &crtc);
		++modes;
	}
	return 0;
}

adv_error crtc_container_insert_default_bios_vga(adv_crtc_container* cc)
{
	return crtc_container_insert_default(cc, BIOS_VGA);
}

adv_error crtc_container_insert_default_bios_vbe(adv_crtc_container* cc)
{
	return crtc_container_insert_default(cc, BIOS_VBE);
}

/**
 * Insert some standard video modes than can be made with a standard VGA.
 */
adv_error crtc_container_insert_default_modeline_vga(adv_crtc_container* cc)
{
	return crtc_container_insert_default(cc, MODELINE_VGA);
}

/**
 * Insert some standard video modes than can be made with a complete programmable SVGA.
 */
adv_error crtc_container_insert_default_modeline_svga(adv_crtc_container* cc)
{
	return crtc_container_insert_default(cc, MODELINE_SVGA);
}

/**
 * Insert the standard video modes of the first active video driver.
 */
void crtc_container_insert_default_active(adv_crtc_container* cc)
{
	if (video_driver_vector_max() > 0) {
		const adv_video_driver* driver = video_driver_vector_pos(0);
		if (driver->crtc_container_insert_default) {
			driver->crtc_container_insert_default(cc);
		}
	}
}

/**
 * Insert the standard video modes of all the active video driver.
 */
void crtc_container_insert_default_all(adv_crtc_container* cc)
{
	unsigned i;
	for(i=0;i<video_driver_vector_max();++i) {
		const adv_video_driver* driver = video_driver_vector_pos(i);
		if (driver->crtc_container_insert_default) {
			driver->crtc_container_insert_default(cc);
		}
	}
}


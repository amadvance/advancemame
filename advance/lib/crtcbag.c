/*
 * This file is part of the Advance project.
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

#include "crtcbag.h"
#include "video.h"

#include <stdlib.h>
#include <string.h>

adv_bool video_crtc_container_is_empty(const video_crtc_container* cc) {
	return cc->base == 0;
}

/**
 * Inizialize the video crtc container.
 * \note The container MUST be deinitialized.
 */
void video_crtc_container_init(video_crtc_container* cc) {
	cc->base = 0;
}

/**
 * Deinizialize the video crtc container
 */
void video_crtc_container_done(video_crtc_container* cc) {
	video_crtc* p = cc->base;
	while (p) {
		video_crtc* todelete = p;
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
const video_crtc* video_crtc_container_has(video_crtc_container* cc, const video_crtc* vm, int (*compare)(const video_crtc* a,const video_crtc* b)) {
	video_crtc* p = cc->base;
	while (p) {
		if (compare(p,vm)==0)
			return p;
		p = p->container_next;
	}
	return 0;
}

/**
 * Remove some crtc from the container.
 * All the crtc for which the select function returns !=0 are removed.
 * \param select selection function
 * \param arg arg passed at the selection function
 */
void video_crtc_container_remove(video_crtc_container* cc, int (*select)(const video_crtc*, void*), void* arg) {
	while (cc->base && select(cc->base,arg)) {
		video_crtc* todelete = cc->base;
		cc->base = cc->base->container_next;
		free(todelete);
	}
	if (cc->base) {
		video_crtc* p = cc->base;
		while (p->container_next) {
			if (select(p->container_next,arg)) {
				video_crtc* todelete = p->container_next;
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
const video_crtc* video_crtc_container_insert(video_crtc_container* cc, const video_crtc* vm) {
	video_crtc* p = malloc(sizeof(video_crtc));
	*p = *vm;
	p->container_next = cc->base;
	cc->base = p;
	return p;
}

/**
 * Insert a crtc in a sorted container.
 */
const video_crtc* video_crtc_container_insert_sort(video_crtc_container* cc,  const video_crtc* vm, int (*compare)(const video_crtc* a,const video_crtc* b)) {
	if (cc->base && compare(cc->base,vm)<=0) {
		video_crtc* p = cc->base;
		video_crtc* c;
		while (p->container_next && compare(p->container_next,vm)<=0) {
			p = p->container_next;
		}
		c = malloc(sizeof(video_crtc));
		*c = *vm;
		c->container_next = p->container_next;
		p->container_next = c;
		return c;
	} else {
		video_crtc* p = malloc(sizeof(video_crtc));
		*p = *vm;
		p->container_next = cc->base;
		cc->base = p;
		return p;
	}
}

void video_crtc_container_iterator_begin(video_crtc_container_iterator* cci, video_crtc_container* cc) {
	cci->base = cc->base;
}

void video_crtc_container_iterator_next(video_crtc_container_iterator* cci) {
	cci->base = cci->base->container_next;
}

adv_bool video_crtc_container_iterator_is_end(video_crtc_container_iterator* cci) {
	return cci->base == 0;
}

video_crtc* video_crtc_container_iterator_get(video_crtc_container_iterator* cci) {
	return cci->base;
}

/** Modes for the VGA BIOS */
static const char* BIOS_VGA[] = {
"pc_31.5_bios_vga_text40_8x8 12.5876 320 328 376 400 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_31.5_bios_vga_text40_9x8 14.161 360 368 422 450 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_31.5_bios_vga_text80_8x8 25.1752 640 656 752 800 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_31.5_bios_vga_text80_9x8 28.3221 720 738 846 900 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
"pc_31.5_bios_vga_text40_8x16 12.5876 320 328 376 400 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_31.5_bios_vga_text40_9x16 14.161 360 368 422 450 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_31.5_bios_vga_text80_8x16 25.1752 640 656 752 800 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_31.5_bios_vga_text80_9x16 28.3221 720 738 846 900 400 408 410 437 -hsync +vsync", /* H 31469 [Hz], V 72.0114 [Hz] */
"pc_31.5_bios_vga_320x200 12.5876 320 328 376 400 200 204 205 219 -hsync +vsync doublescan", /* H 31469 [Hz], V 71.847 [Hz] */
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

/* PAL SMALL resolutions */
"pal_vga_small_304x276x8 6.2938 304 320 372 400 276 284 285 312 -hsync -vsync",
"pal_vga_small_336x276x8 7.0805 336 356 404 452 276 284 285 312 -hsync -vsync",

/* from VGA2PAL */
/*"pal_vga_text_vga2pal 14.16 643 700 828 896 256 276 280 311 -hsync -vsync",*/

/* NTSC SMALL resolutions, manual */
"ntsc_vga_small_304x228 6.2938 304 320 372 400 228 235 238 262 -hsync -vsync",
"ntsc_vga_small_336x228 7.0805 336 356 404 452 228 235 238 262 -hsync -vsync",

/* from VGA2NTSC */
/* "ntsc_vga_text_vga2ntsc 14.16 640 736 800 904 200 220 229 263 -hsync -vsync",*/

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

/* PAL SMALL resolutions */
"pal_small_368x276 7.375 368 384 424 472 276 285 288 312 -hsync -vsync",
"pal_small_736x552 14.75 736 776 848 944 552 570 576 625 -hsync -vsync interlace",

/* PAL HORZ resolution */
"pal_horz_768x288 14.75 768 792 864 944 288 291 294 312 -hsync -vsync",
"pal_small_horz_736x276 14.75 736 776 848 944 276 285 288 312 -hsync -vsync",

/* NTSC SMALL resolutions */
"ntsc_small_304x228 6.16773 304 328 352 392 228 238 241 262 -hsync -vsync",
"ntsc_small_608x456 12.2096 608 640 696 776 456 476 482 525 -hsync -vsync interlace",

/* NTSC HORZ */
"ntsc_horz_640x242 12.2096 640 664 720 776 242 245 248 262 -hsync -vsync",
"ntsc_small_horz_608x228 12.2096 608 640 696 776 228 238 241 262 -hsync -vsync",

#include "svgamode.dat"

0
};

static adv_error video_crtc_container_insert_default(video_crtc_container* cc, const char** modes) {
	while (*modes) {
		video_crtc crtc;
		if (video_crtc_parse(&crtc,*modes,*modes + strlen(*modes))!=0)
			return -1;
		video_crtc_container_insert(cc,&crtc);
		++modes;
	}
	return 0;
}

adv_error video_crtc_container_insert_default_bios_vga(video_crtc_container* cc) {
	return video_crtc_container_insert_default(cc,BIOS_VGA);
}

adv_error video_crtc_container_insert_default_bios_vbe(video_crtc_container* cc) {
	return video_crtc_container_insert_default(cc,BIOS_VBE);
}

/**
 * Insert some standard video modes than can be made with a standard VGA.
 */
adv_error video_crtc_container_insert_default_modeline_vga(video_crtc_container* cc) {
	return video_crtc_container_insert_default(cc,MODELINE_VGA);
}

/**
 * Insert some standard video modes than can be made with a complete programmable SVGA.
 */
adv_error video_crtc_container_insert_default_modeline_svga(video_crtc_container* cc) {
	return video_crtc_container_insert_default(cc,MODELINE_SVGA);
}

/**
 * Insert the standard video modes of the first active video driver.
 */
void video_crtc_container_insert_default_system(video_crtc_container* cc) {
	if (video_driver_vector_max() > 0) {
		const video_driver* driver = video_driver_vector_pos(0);
		if (driver->crtc_container_insert_default) {
			driver->crtc_container_insert_default(cc);
		}
	}
}

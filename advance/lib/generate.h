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

/** \file
 * CRTC generation.
 */

/** \addtogroup Generate */
/*@{*/

#ifndef __GENERATE_H
#define __GENERATE_H

#include "video.h"
#include "crtc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adv_generate_struct {
	double hactive;
	double hfront;
	double hsync;
	double hback;
	double vactive;
	double vfront;
	double vsync;
	double vback;
} adv_generate;

typedef struct adv_generate_interpolate_struct {
	unsigned hclock;
	adv_generate gen;
} adv_generate_interpolate;

#define GENERATE_INTERPOLATE_MAX 8

typedef struct adv_generate_interpolate_set_struct {
	unsigned mac;
	adv_generate_interpolate map[GENERATE_INTERPOLATE_MAX];
} adv_generate_interpolate_set;

void generate_crtc_hsize(adv_crtc* crtc, unsigned hsize, const adv_generate* generate);
void generate_crtc_vsize(adv_crtc* crtc, unsigned vsize, const adv_generate* generate);
void generate_crtc_htotal(adv_crtc* crtc, unsigned htotal, const adv_generate* generate);
void generate_crtc_vtotal(adv_crtc* crtc, unsigned vtotal, const adv_generate* generate);

void generate_pixel_clock(adv_crtc* crtc, double clock);
void generate_horz_clock(adv_crtc* crtc, double clock);
void generate_vert_clock(adv_crtc* crtc, double clock);

#define GENERATE_ADJUST_EXACT CRTC_ADJUST_EXACT
#define GENERATE_ADJUST_VCLOCK CRTC_ADJUST_VCLOCK
#define GENERATE_ADJUST_VTOTAL CRTC_ADJUST_VTOTAL
adv_error generate_find(adv_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const adv_monitor* monitor, const adv_generate* generate, unsigned capability, unsigned adjust);
adv_error generate_find_interpolate(adv_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const adv_monitor* monitor, const adv_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust);
adv_error generate_find_interpolate_multi(adv_crtc* crtc, unsigned hsize0, unsigned vsize0, unsigned hsize1, unsigned vsize1, unsigned hsize2, unsigned vsize2, unsigned hsize3, unsigned vsize3, double vclock, const adv_monitor* monitor, const adv_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust);

void generate_default_vga(adv_generate* generate);
void generate_default_atari_standard(adv_generate* generate);
void generate_default_atari_extended(adv_generate* generate);
void generate_default_atari_medium(adv_generate* generate);
void generate_default_atari_vga(adv_generate* generate);
void generate_default_pal(adv_generate* generate);
void generate_default_ntsc(adv_generate* generate);
void generate_default_lcd(adv_generate* generate);

void generate_normalize(adv_generate* generate);
void generate_normalize_copy(adv_generate* norm, const adv_generate* generate);
void generate_interpolate_h(adv_generate* monitor, unsigned hclock, const adv_generate_interpolate_set* interpolate);

void generate_interpolate_reset(adv_generate_interpolate_set* interpolate);
adv_bool generate_interpolate_is_empty(const adv_generate_interpolate_set* interpolate);
adv_error generate_parse(adv_generate* generate, const char* g);
adv_error generate_interpolate_load(adv_conf* context, adv_generate_interpolate_set* interpolate);
void generate_interpolate_save(adv_conf* context, const adv_generate_interpolate_set* interpolate);
void generate_interpolate_clear(adv_conf* context);
void generate_interpolate_register(adv_conf* context);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/

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

#ifndef __GENERATE_H
#define __GENERATE_H

#include "video.h"
#include "crtc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct video_generate_struct {
	double hactive;
	double hfront;
	double hsync;
	double hback;
	double vactive;
	double vfront;
	double vsync;
	double vback;
} video_generate;

typedef struct video_generate_interpolate_struct {
	unsigned hclock;
	video_generate gen;
} video_generate_interpolate;

#define GENERATE_INTERPOLATE_MAX 8

typedef struct video_generate_interpolate_set_struct {
	unsigned mac;
	video_generate_interpolate map[GENERATE_INTERPOLATE_MAX];
} video_generate_interpolate_set;

void generate_crtc(video_crtc* crtc, unsigned hsize, unsigned vsize, const video_generate* generate);
void generate_crtc_change_horz(video_crtc* crtc, unsigned hsize);
void generate_crtc_change_vert(video_crtc* crtc, unsigned vsize);

void generate_pixel_clock(video_crtc* crtc, double clock);
void generate_horz_clock(video_crtc* crtc, double clock);
void generate_vert_clock(video_crtc* crtc, double clock);

#define GENERATE_ADJUST_EXACT CRTC_ADJUST_EXACT
#define GENERATE_ADJUST_VCLOCK CRTC_ADJUST_VCLOCK
#define GENERATE_ADJUST_VTOTAL CRTC_ADJUST_VTOTAL
video_error generate_find(video_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const video_monitor* monitor, const video_generate* generate, unsigned capability, unsigned adjust);
video_error generate_find_interpolate(video_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const video_monitor* monitor, const video_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust);
video_error generate_find_interpolate_double(video_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const video_monitor* monitor, const video_generate_interpolate_set* interpolate, unsigned capability, unsigned adjust);

void generate_default_vga(video_generate* generate);
void generate_default_atari_standard(video_generate* generate);
void generate_default_atari_extended(video_generate* generate);
void generate_default_atari_medium(video_generate* generate);
void generate_default_atari_vga(video_generate* generate);
void generate_default_pal(video_generate* generate);
void generate_default_ntsc(video_generate* generate);
void generate_default_lcd(video_generate* generate);

void generate_normalize(video_generate* generate);
void generate_interpolate_h(video_generate* monitor, unsigned hclock, const video_generate_interpolate_set* interpolate);

video_error generate_parse(video_generate* generate, const char* g);
video_error generate_interpolate_load(struct conf_context* context, video_generate_interpolate_set* interpolate);
void generate_interpolate_save(struct conf_context* context, const video_generate_interpolate_set* interpolate);
void generate_interpolate_clear(struct conf_context* context);
void generate_interpolate_register(struct conf_context* context);

#ifdef __cplusplus
}
#endif

#endif

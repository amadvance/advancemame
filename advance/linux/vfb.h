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

/** \file
 * Video driver "fb".
 */

#ifndef __VFB_H
#define __VFB_H

#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fb_video_mode_struct {
	unsigned bits_per_pixel; /**< bits per pixel (8 bit modes are always palettized) */
	adv_crtc crtc; /**< CRTC values */
} fb_video_mode;

adv_error fb_init(int device_id);
void fb_done(void);

adv_bool fb_is_active(void);
adv_bool fb_mode_is_active(void);

unsigned fb_flags(void);

adv_error fb_mode_set(const fb_video_mode* mode);
adv_error fb_mode_change(const fb_video_mode* mode);
void fb_mode_done(adv_bool restore);

unsigned fb_virtual_x(void);
unsigned fb_virtual_y(void);
unsigned fb_bytes_per_scanline(void);
unsigned fb_adjust_bytes_per_page(unsigned bytes_per_page);
adv_rgb_def fb_rgb_def(void);

extern unsigned char* (*fb_write_line)(unsigned y);

void fb_wait_vsync(void);
adv_error fb_scroll(unsigned offset, adv_bool waitvsync);
adv_error fb_scanline_set(unsigned byte_length);
adv_error fb_palette8_set(const adv_color* palette, unsigned start, unsigned count, adv_bool waitvsync);

adv_error fb_mode_import(adv_mode* mode, const fb_video_mode* fb_mode);
adv_error fb_mode_generate(fb_video_mode* mode, const adv_crtc* crtc, unsigned bits, unsigned flags);
int fb_mode_compare(const fb_video_mode* a, const fb_video_mode* b);

void fb_default(void);
void fb_reg(adv_conf* context);
adv_error fb_load(adv_conf* context);

/**
 * Video driver "fb".
 * \ingroup Video
 */
extern adv_video_driver video_fb_driver;

#ifdef __cplusplus
}
#endif

#endif

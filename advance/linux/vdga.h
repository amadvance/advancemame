/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

#ifndef __VDGA_H
#define __VDGA_H

#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dga_video_mode_struct {
	unsigned bits_per_pixel; /**< bits per pixel (8 bit modes are always palettized) */
	adv_crtc crtc; /**< CRTC values */
} dga_video_mode;

adv_error dga_init(int device_id);
void dga_done(void);

adv_bool dga_is_active(void);
adv_bool dga_mode_is_active(void);

unsigned dga_flags(void);

adv_error dga_mode_set(const dga_video_mode* mode);
adv_error dga_mode_change(const dga_video_mode* mode);
void dga_mode_done(adv_bool restore);

unsigned dga_virtual_x(void);
unsigned dga_virtual_y(void);
unsigned dga_bytes_per_scanline(void);
unsigned dga_adjust_bytes_per_page(unsigned bytes_per_page);
video_rgb_def dga_rgb_def(void);

extern unsigned char* (*dga_write_line)(unsigned y);

void dga_wait_vsync(void);
adv_error dga_scroll(unsigned offset, adv_bool waitvsync);
adv_error dga_scanline_set(unsigned byte_length);
adv_error dga_palette8_set(const video_color* palette, unsigned start, unsigned count, adv_bool waitvsync);

adv_error dga_mode_import(adv_mode* mode, const dga_video_mode* dga_mode);
adv_error dga_mode_generate(dga_video_mode* mode, const adv_crtc* crtc, unsigned bits, unsigned flags);
int dga_mode_compare(const dga_video_mode* a, const dga_video_mode* b);

void dga_default(void);
void dga_reg(adv_conf* context);
adv_error dga_load(adv_conf* context);

extern adv_video_driver video_dga_driver;

#ifdef __cplusplus
}
#endif

#endif

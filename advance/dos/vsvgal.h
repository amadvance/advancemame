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
 * Video driver "svgaline".
 */

#ifndef __SVGALINE_H
#define __SVGALINE_H

#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct svgaline_video_mode_struct {
	unsigned index; /**< Mode index. */
	adv_crtc crtc; /**< CRTC values. */
} svgaline_video_mode;

adv_error svgaline_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor);
void svgaline_done(void);

adv_bool svgaline_is_active(void);
adv_bool svgaline_mode_is_active(void);

unsigned svgaline_flags(void);

adv_error svgaline_mode_set(const svgaline_video_mode* mode);
void svgaline_mode_done(adv_bool restore);

unsigned svgaline_virtual_x(void);
unsigned svgaline_virtual_y(void);
unsigned svgaline_bytes_per_scanline(void);
unsigned svgaline_adjust_bytes_per_page(unsigned bytes_per_page);
adv_color_def svgaline_color_def(void);

extern unsigned char* (*svgaline_write_line)(unsigned y);

void svgaline_wait_vsync(void);
adv_error svgaline_scroll(unsigned offset, adv_bool waitvsync);
adv_error svgaline_scanline_set(unsigned byte_length);
adv_error svgaline_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);

adv_error svgaline_mode_import(adv_mode* mode, const svgaline_video_mode* svgaline_mode);
adv_error svgaline_mode_generate(svgaline_video_mode* mode, const adv_crtc* crtc, unsigned flags);
int svgaline_mode_compare(const svgaline_video_mode* a, const svgaline_video_mode* b);

void svgaline_default(void);
void svgaline_reg(adv_conf* context);
adv_error svgaline_load(adv_conf* context);

/**
 * Video driver "svgaline".
 * \ingroup Video
 */
extern adv_video_driver video_svgaline_driver;

#ifdef __cplusplus
}
#endif

#endif

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

#ifndef __VGALINE_H
#define __VGALINE_H

#include "scrvga.h"
#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vgaline_video_mode_struct {
	video_crtc crtc; /**< CRTC values */
	video_bool is_text; /**< is a text mode */
	unsigned font_x; /**< X size of the font, valid only in text mode */
	unsigned font_y;  /**< Y size of the font, valid only in text mode */
} vgaline_video_mode;

video_error vgaline_init(int device_id);
void vgaline_done(void);

video_bool vgaline_is_active(void);
video_bool vgaline_mode_is_active(void);
unsigned vgaline_flags(void);

video_error vgaline_mode_set(const vgaline_video_mode* mode);
void vgaline_mode_done(video_bool restore);

video_error vgaline_mode_grab(vgaline_video_mode* mode);
video_error vgaline_mode_import(video_mode* mode, const vgaline_video_mode* vgaline_mode);
video_error vgaline_mode_generate(vgaline_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags);
int vgaline_mode_compare(const vgaline_video_mode* a, const vgaline_video_mode* b);

video_error video_crtc_import(video_crtc* crtc, struct vga_info* info, unsigned size_x, unsigned size_y, double vclock);

extern video_driver video_vgaline_driver;

#ifdef __cplusplus
}
#endif

#endif

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
 * Video driver "vgaline".
 */

#ifndef __VGALINE_H
#define __VGALINE_H

#include "scrvga.h"
#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vgaline_video_mode_struct {
	adv_crtc crtc; /**< CRTC values. */
	adv_bool is_text; /**< Is a text mode. */
	unsigned font_x; /**< X size of the font, valid only in text mode. */
	unsigned font_y;  /**< Y size of the font, valid only in text mode. */
} vgaline_video_mode;

adv_error vgaline_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor);
void vgaline_done(void);

adv_bool vgaline_is_active(void);
adv_bool vgaline_mode_is_active(void);
unsigned vgaline_flags(void);

adv_error vgaline_mode_set(const vgaline_video_mode* mode);
void vgaline_mode_done(adv_bool restore);

adv_error vgaline_mode_grab(vgaline_video_mode* mode);
adv_error vgaline_mode_import(adv_mode* mode, const vgaline_video_mode* vgaline_mode);
adv_error vgaline_mode_generate(vgaline_video_mode* mode, const adv_crtc* crtc, unsigned flags);
int vgaline_mode_compare(const vgaline_video_mode* a, const vgaline_video_mode* b);

adv_error crtc_import(adv_crtc* crtc, struct vga_info* info, unsigned size_x, unsigned size_y, double vclock);

/**
 * Video driver "vgaline".
 * \ingroup Video
 */
extern adv_video_driver video_vgaline_driver;

#ifdef __cplusplus
}
#endif

#endif

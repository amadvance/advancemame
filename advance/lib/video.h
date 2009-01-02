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
 * Video definitions.
 */

/** \addtogroup Video */
/*@{*/

#ifndef __VIDEO_H
#define __VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "extra.h"
#include "videodrv.h"

#include "crtc.h"
#include "crtcbag.h"

/***************************************************************************/
/* Option */

void video_reg(adv_conf* context, adv_bool auto_detect);
void video_reg_driver(adv_conf* context, adv_video_driver* driver);
adv_error video_load(adv_conf* context, const char* driver_ignore);

/***************************************************************************/
/* Private */

struct video_state_struct {
	adv_bool active; /**< !=0 if active. */

	unsigned driver_mac; /**< Number of video driver available. */
	adv_video_driver* driver_map[DEVICE_MAX]; /**< Video drivers available. */

	adv_bool old_mode_required; /**< If at least one mode is set. */

	/* Mode */
	adv_bool mode_active; /**< !=0 if a mode is selected. */
	adv_mode mode; /**< Current mode. */
	adv_mode mode_original; /**< Current mode previously any internal change */
	unsigned virtual_x;
	unsigned virtual_y;
	double measured_vclock;

	/* Mode RGB */
	adv_color_def color_def; /**< Definition of the current RGB mode. */

	unsigned rgb_red_mask; /**< Mask of the channel. */
	unsigned rgb_green_mask;
	unsigned rgb_blue_mask;
	int rgb_red_shift; /**< Shift in bit of the channel (shift = pos + len - 8). */
	int rgb_green_shift;
	int rgb_blue_shift;
	unsigned rgb_red_len; /**< Number of bit of the channel. */
	unsigned rgb_green_len;
	unsigned rgb_blue_len;
	unsigned rgb_mask_bit; /**< Whole mask of the three channel. */
	unsigned rgb_high_bit; /**< Highest bits of the three channel. */
	unsigned rgb_low_bit; /**< Lowest bits of the three channel. */

	unsigned char* fake_text_map; /**< Fake text buffer. */
	unsigned char* fake_text_last_map; /**< Fake text last buffer. */
	unsigned fake_text_dx; /**< Fake text columns. */
	unsigned fake_text_dy; /**< Fake text rows. */
};

extern struct video_state_struct video_state;

/***************************************************************************/
/* Informative */

/** If the video library is initialized. */
static inline adv_bool video_is_active(void)
{
	return video_state.active != 0;
}

/** If a video mode is active. */
static inline adv_bool video_mode_is_active(void)
{
	assert(video_is_active());
	return video_state.mode_active;
}

/** Current video mode. */
static inline const adv_mode* video_current_mode(void)
{
	assert(video_mode_is_active());
	return &video_state.mode;
}

/** Video driver of the current mode. */
static inline const adv_video_driver* video_current_driver(void)
{
	return mode_driver(video_current_mode());
}

/** Video driver of the current mode. */
static inline unsigned video_driver_flags(void)
{
	return mode_driver(video_current_mode())->flags();
}

/**
 * Get the color format of the current video mode.
 */
static inline adv_color_def video_color_def(void)
{
	return video_state.color_def;
}

/**
 * Get the index format of the current video mode.
 */
static inline unsigned video_index(void)
{
	return mode_index(video_current_mode());
}

/**
 * Get the scan format of the current video mode.
 */
static inline unsigned video_scan(void)
{
	return mode_scan(video_current_mode());
}

/** Color depth in bits of the current video mode. */
static inline unsigned video_bits_per_pixel(void)
{
	return mode_bits_per_pixel(video_current_mode());
}

/** Color depth in bytes of the current video mode. */
static inline unsigned video_bytes_per_pixel(void)
{
	return mode_bytes_per_pixel(video_current_mode());
}

/** If the current video mode is a text mode. */
static inline adv_bool video_is_text(void)
{
	return mode_is_text(video_current_mode());
}

/** If the current video mode is a graphics mode. */
static inline adv_bool video_is_graphics(void)
{
	return mode_is_graphics(video_current_mode());
}

/** Name of the current video mode */
static inline  const char* video_name(void)
{
	return mode_name(video_current_mode());
}

/** Vertical clock of the current video mode. */
static inline double video_vclock(void)
{
	return mode_vclock(video_current_mode());
}

/** Measured vertical clock of the current video mode. */
static inline double video_measured_vclock(void)
{
	return video_state.measured_vclock;
}

/** Horizontal clock of the current video mode. */
static inline double video_hclock(void)
{
	return mode_hclock(video_current_mode());
}

/** Horizontal size of the current video mode. */
static inline unsigned video_size_x(void)
{
	return mode_size_x(video_current_mode());
}

/** Vertical size of the current video mode. */
static inline unsigned video_size_y(void)
{
	return mode_size_y(video_current_mode());
}

/** Capabilities VIDEO_DRIVER_FLAGS_ * of the current video mode. */
static inline unsigned video_flags(void)
{
	/* the flags may be limited by the video options */
	return mode_flags(video_current_mode());
}

/** Access the memory for writing. */
extern unsigned char* (*video_write_line)(unsigned y);

/** Grant access in writing. */
static inline void video_write_lock(void)
{
	if (video_current_driver()->write_lock)
		video_current_driver()->write_lock();
}

/** Remove the lock for writing. */
static inline void video_write_unlock(unsigned x, unsigned y, unsigned size_x, unsigned size_y, adv_bool waitvsync)
{
	if (video_current_driver()->write_unlock)
		video_current_driver()->write_unlock(x, y, size_x, size_y, waitvsync);
}

/** Write column offset of the current video mode. */
static inline unsigned video_offset(unsigned x)
{
	return x * video_bytes_per_pixel();
}

/***************************************************************************/
/* Colors */

void video_index_packed_to_rgb(adv_bool waitvsync);
adv_bool video_index_rgb_to_packed_is_available(void);
void video_index_rgb_to_packed(void);
adv_bool video_index_packed_to_rgb_is_available(void);

adv_color_rgb* video_palette_get(void);
adv_error video_palette_set(adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);

static inline void video_palette_make(adv_color_rgb* vp, unsigned r, unsigned g, unsigned b)
{
	vp->red = r;
	vp->green = g;
	vp->blue = b;
}

static inline adv_pixel video_pixel_get(unsigned r, unsigned g, unsigned b)
{
	return pixel_make_from_def(r, g, b, video_color_def());
}

static inline void video_pixel_make(adv_pixel* pixel, unsigned r, unsigned g, unsigned b)
{
	*pixel = pixel_make_from_def(r, g, b, video_color_def());
}

static inline unsigned video_rgb_red_mask_bit_get(void)
{
	return video_state.rgb_red_mask;
}

static inline unsigned video_rgb_green_mask_bit_get(void)
{
	return video_state.rgb_green_mask;
}

static inline unsigned video_rgb_blue_mask_bit_get(void)
{
	return video_state.rgb_blue_mask;
}

static inline unsigned video_rgb_mask_bit_get(void)
{
	return video_state.rgb_mask_bit;
}

static inline unsigned video_rgb_high_bit_get(void)
{
	return video_state.rgb_high_bit;
}

static inline unsigned video_rgb_low_bit_get(void)
{
	return video_state.rgb_low_bit;
}

static inline unsigned video_red_get(unsigned rgb)
{
	return rgb_nibble_extract(rgb, video_state.rgb_red_shift, video_state.rgb_red_mask);
}

static inline unsigned video_green_get(unsigned rgb)
{
	return rgb_nibble_extract(rgb, video_state.rgb_green_shift, video_state.rgb_green_mask);
}

static inline unsigned video_blue_get(unsigned rgb)
{
	return rgb_nibble_extract(rgb, video_state.rgb_blue_shift, video_state.rgb_blue_mask);
}

static inline unsigned video_red_get_approx(unsigned rgb)
{
	return rgb_approx(video_red_get(rgb), video_state.rgb_red_len);
}

static inline unsigned video_green_get_approx(unsigned rgb)
{
	return rgb_approx(video_green_get(rgb), video_state.rgb_green_len);
}

static inline unsigned video_blue_get_approx(unsigned rgb)
{
	return rgb_approx(video_blue_get(rgb), video_state.rgb_blue_len);
}

/***************************************************************************/
/* Commands */

void video_wait_vsync(void);
unsigned video_font_size_x(void);
unsigned video_font_size_y(void);
unsigned video_virtual_x(void);
unsigned video_virtual_y(void);
adv_error video_display_set(unsigned offset, adv_bool waitvsync);
unsigned video_bytes_per_scanline(void);
unsigned video_bytes_per_page(void);

adv_error adv_video_init(void);
void adv_video_done(void);
void adv_video_abort(void);

adv_error video_mode_set(adv_mode* mode);
void video_mode_done(adv_bool restore);
void video_mode_restore(void);
adv_error video_mode_grab(adv_mode* mode);
int video_mode_compare(const adv_mode* a, const adv_mode* b);

adv_error video_mode_generate(adv_mode* mode, const adv_crtc* crtc, unsigned flags);
adv_error video_mode_generate_check(const char* driver, unsigned driver_flags, unsigned hstep, unsigned hvmax, const adv_crtc* crtc, unsigned flags);
unsigned video_mode_generate_driver_flags(unsigned flags_out, unsigned flags_in);

void video_mode_print(char* buffer, const adv_mode* vm);

double video_measure_step(void (*wait)(void), double low, double high);

void video_put_pixel(unsigned x, unsigned y, unsigned color);
void video_put_pixel_clip(unsigned x, unsigned y, unsigned color);
void video_put_char(unsigned x, unsigned y, char c, unsigned color);

unsigned video_driver_vector_max(void);
const adv_video_driver* video_driver_vector_pos(unsigned i);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/

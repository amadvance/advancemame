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

#ifndef __SCREEN_H
#define __SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "advstd.h"
#include "videodrv.h"

#include <assert.h>

#include "crtc.h"
#include "crtcbag.h"

/***************************************************************************/
/* Option */

void video_reg(struct conf_context* context, adv_bool auto_detect);
void video_reg_driver(struct conf_context* context, video_driver* driver);
adv_error video_load(struct conf_context* context, const char* driver_ignore);

/***************************************************************************/
/* Private */

typedef struct video_internal_struct {
	adv_bool active; /**< !=0 if active. */

	unsigned driver_mac; /**< Number of video driver available. */
	video_driver* driver_map[DEVICE_MAX]; /**< Video drivers available. */

	adv_bool old_mode_required; /**< If at least one mode is set. */

	/* Mode */
	adv_bool mode_active; /**< !=0 if a mode is selected. */
	video_mode mode; /**< Current mode. */
	unsigned virtual_x;
	unsigned virtual_y;
	double measured_vclock;

	/* Mode RGB */
	video_rgb_def rgb_def; /**< Definition of the current RGB mode. */
	unsigned rgb_red_mask; /**< Positioned mask for the RED channel. */
	unsigned rgb_green_mask;
	unsigned rgb_blue_mask;
	int rgb_red_shift; /**< Shift in bit of the RED channel (shift = pos + len - 8). */
	int rgb_green_shift;
	int rgb_blue_shift;
	unsigned rgb_red_len; /**< Number of bit of the RED channel. */
	unsigned rgb_green_len;
	unsigned rgb_blue_len;
	video_rgb rgb_mask_bit; /**< Whole mask of the three RGB channel. */
	video_rgb rgb_high_bit; /**< High bits of the three RGB channel. */
	video_rgb rgb_low_bit; /**< Low bits of the three RGB channel. */
} video_internal;

extern video_internal video_state;

/***************************************************************************/
/* Informative */

unsigned video_internal_flags(void);

/** If the video library is initialized. */
static __inline__ adv_bool video_is_active(void) {
	return video_state.active != 0;
}

/** If a video mode is active. */
static __inline__ adv_bool video_mode_is_active(void) {
	assert( video_is_active() );
	return video_state.mode_active;
}

/** Current video mode. */
static __inline__ const video_mode* video_current_mode(void) {
	assert( video_mode_is_active() );
	return &video_state.mode;
}

/** Video driver of the current mode. */
static __inline__ const video_driver* video_current_driver(void) {
	return video_mode_driver(video_current_mode());
}

static __inline__ unsigned video_index(void) {
	return video_mode_index(video_current_mode());
}

static __inline__ unsigned video_type(void) {
	return video_mode_type(video_current_mode());
}

static __inline__ unsigned video_scan(void) {
	return video_mode_scan(video_current_mode());
}

/** Color depth in bits of the current video mode. */
static __inline__ unsigned video_bits_per_pixel(void) {
	return video_mode_bits_per_pixel(video_current_mode());
}

/** Color depth in bytes of the current video mode. */
static __inline__ unsigned video_bytes_per_pixel(void) {
	return video_mode_bytes_per_pixel(video_current_mode());
}

/** If the current video mode is a text mode. */
static __inline__ adv_bool video_is_text(void) {
	return video_mode_is_text(video_current_mode());
}

/** If the current video mode is a graphics mode. */
static __inline__ adv_bool video_is_graphics(void) {
	return video_mode_is_graphics(video_current_mode());
}

/** If the current video mode is a linear mode. */
static __inline__ adv_bool video_is_linear(void) {
	return video_mode_is_linear(video_current_mode());
}

/** If the current video mode is an unchained mode. */
static __inline__ adv_bool video_is_unchained(void) {
	return video_mode_is_unchained(video_current_mode());
}

/** If the current video mode is a banked mode. */
static __inline__ adv_bool video_is_banked(void) {
	return video_mode_is_banked(video_current_mode());
}

/** Name of the current video mode */
static __inline__  const char* video_name(void) {
	return video_mode_name(video_current_mode());
}

/** Vertical clock of the current video mode. */
static __inline__ double video_vclock(void) {
	return video_mode_vclock(video_current_mode());
}

/** Measured vertical clock of the current video mode. */
static __inline__ double video_measured_vclock(void) {
	return video_state.measured_vclock;
}

/** Horizontal clock of the current video mode. */
static __inline__ double video_hclock(void) {
	return video_mode_hclock(video_current_mode());
}

/** Horizontal size of the current video mode. */
static __inline__ unsigned video_size_x(void) {
	return video_mode_size_x(video_current_mode());
}

/** Vertical size of the current video mode. */
static __inline__ unsigned video_size_y(void) {
	return video_mode_size_y(video_current_mode());
}

/** Horizontal size of the font of the current video mode. */
unsigned video_font_size_x(void);

/** Vertical size of the font of the current video mode. */
unsigned video_font_size_y(void);

/** Horizontal virtual size of the current video mode. */
static __inline__ unsigned video_virtual_x(void) {
	return video_state.virtual_x;
}

/** Vertical virtual size of the current video mode. */
static __inline__ unsigned video_virtual_y(void) {
	return video_state.virtual_y;
}

/** Capabilities VIDEO_DRIVER_FLAGS_ * of the current video mode. */
static __inline__ unsigned video_flags(void) {
	/* the flags may be limited by the video options */
	return video_mode_flags(video_current_mode());
}

/** Access the memory for writing. */
extern unsigned char* (*video_write_line)(unsigned y);

/** Grant access in writing. */
static __inline__ void video_write_lock(void) {
	if (video_current_driver()->write_lock)
		video_current_driver()->write_lock();
}

/** Remove the lock for writing. */
static __inline__ void video_write_unlock(unsigned x, unsigned y, unsigned size_x, unsigned size_y) {
	if (video_current_driver()->write_unlock)
		video_current_driver()->write_unlock(x, y, size_x, size_y);
}

/** Write column offset of the current video mode. */
static __inline__ unsigned video_offset(unsigned x) {
	unsigned off = x * video_bytes_per_pixel();
	if (video_is_unchained())
		off /= 4;
	return off;
}

/***************************************************************************/
/* Colors */

void video_index_packed_to_rgb(adv_bool waitvsync);
adv_bool video_index_rgb_to_packed_is_available(void);
void video_index_rgb_to_packed(void);
adv_bool video_index_packed_to_rgb_is_available(void);

/** Get the RGB format of the current video mode. */
static __inline__ video_rgb_def video_current_rgb_def_get(void) {
	return video_state.rgb_def;
}

video_color* video_palette_get(void);
adv_error video_palette_set(video_color* palette, unsigned start, unsigned count, int waitvsync);

static __inline__ void video_palette_make(video_color* vp, unsigned r, unsigned g, unsigned b) {
	vp->red = r;
	vp->green = g;
	vp->blue = b;
}

unsigned video_color_dist(const video_color* A, const video_color* B);

static __inline__ unsigned video_rgb_shift(unsigned value, int shift) {
	if (shift >= 0)
		return value >> shift;
	else
		return value << -shift;
}

static __inline__ unsigned video_rgb_nibble_insert(unsigned value, int shift, unsigned mask) {
	return video_rgb_shift(value,-shift) & mask;
}

static __inline__ unsigned video_rgb_nibble_extract(unsigned value, int shift, unsigned mask) {
	return video_rgb_shift(value & mask,shift);
}

static __inline__ video_rgb video_rgb_get(unsigned r, unsigned g, unsigned b) {
	return video_rgb_nibble_insert(r, video_state.rgb_red_shift, video_state.rgb_red_mask) |
		video_rgb_nibble_insert(g, video_state.rgb_green_shift, video_state.rgb_green_mask) |
		video_rgb_nibble_insert(b, video_state.rgb_blue_shift, video_state.rgb_blue_mask);
}

static __inline__ void video_rgb_make(video_rgb* rgb, unsigned r, unsigned g, unsigned b) {
	*rgb = video_rgb_get(r,g,b);
}

static __inline__ video_rgb video_rgb_red_mask_bit_get(void) {
	return video_state.rgb_red_mask;
}

static __inline__ video_rgb video_rgb_green_mask_bit_get(void) {
	return video_state.rgb_green_mask;
}

static __inline__ video_rgb video_rgb_blue_mask_bit_get(void) {
	return video_state.rgb_blue_mask;
}

static __inline__ video_rgb video_rgb_mask_bit_get(void) {
	return video_state.rgb_mask_bit;
}

static __inline__ video_rgb video_rgb_high_bit_get(void) {
	return video_state.rgb_high_bit;
}

static __inline__ video_rgb video_rgb_low_bit_get(void) {
	return video_state.rgb_low_bit;
}

static __inline__ unsigned video_red_get(unsigned rgb) {
	return video_rgb_nibble_extract(rgb, video_state.rgb_red_shift, video_state.rgb_red_mask);
}

static __inline__ unsigned video_green_get(unsigned rgb) {
	return video_rgb_nibble_extract(rgb, video_state.rgb_green_shift, video_state.rgb_green_mask);
}

static __inline__ unsigned video_blue_get(unsigned rgb) {
	return video_rgb_nibble_extract(rgb, video_state.rgb_blue_shift, video_state.rgb_blue_mask);
}

unsigned video_red_get_approx(unsigned rgb);
unsigned video_green_get_approx(unsigned rgb);
unsigned video_blue_get_approx(unsigned rgb);

/***************************************************************************/
/* Commands */

static __inline__ adv_error video_display_set_async(unsigned offset, adv_bool waitvsync) {
	assert( video_mode_is_active() );

	return video_current_driver()->scroll(offset, waitvsync);
}

static __inline__ unsigned video_bytes_per_scanline(void) {
	assert( video_mode_is_active() );

	return video_current_driver()->bytes_per_scanline();
}

static __inline__ unsigned video_bytes_per_page(void) {
	unsigned bytes_per_page = video_size_y() * video_bytes_per_scanline();

	return video_current_driver()->adjust_bytes_per_page(bytes_per_page);
}

static __inline__ void video_unchained_plane_mask_set(unsigned plane_mask) {
	assert( video_mode_is_active() );

	assert( video_current_driver()->unchained_plane_mask_set );
	video_current_driver()->unchained_plane_mask_set(plane_mask);
}

static __inline__ void video_unchained_plane_set(unsigned plane) {
	video_unchained_plane_mask_set(1 << plane);
}

void video_wait_vsync(void);

adv_error video_init(void);
void video_done(void);
void video_abort(void);

adv_error video_mode_set(video_mode* mode);
void video_mode_done(adv_bool restore);
void video_mode_restore(void);
adv_error video_mode_grab(video_mode* mode);
int video_mode_compare(const video_mode* a, const video_mode* b);

adv_error video_mode_generate(video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags);
adv_error video_mode_generate_check(const char* driver, unsigned driver_flags, unsigned hstep, unsigned hvmax, const video_crtc* crtc, unsigned bits, unsigned flags);
unsigned video_mode_generate_driver_flags(void);

void video_mode_print(char* buffer, const video_mode* vm);

double video_measure_step(void (*wait)(void), double low, double high);

adv_error video_display_set_async(unsigned offset, int waitvsync);
adv_error video_display_set_sync(unsigned offset);
void video_put_pixel(unsigned x, unsigned y, unsigned color);
void video_put_pixel_clip(unsigned x, unsigned y, unsigned color);
void video_put_char(unsigned x, unsigned y, char c, unsigned color);

adv_error video_snapshot_save(const char* snapshot_name, int start_x, int start_y);

unsigned video_driver_vector_max(void);
const video_driver* video_driver_vector_pos(unsigned i);

#ifdef __cplusplus
}
#endif

#endif


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

#include "vnone.h"
#include "video.h"

#include <stdlib.h>
#include <string.h>

/***************************************************************************/
/* State */

typedef struct none_internal_struct {
	video_bool active;
	video_bool mode_active;

	video_rgb_def rgb_def;
	unsigned bytes_per_pixel;
	unsigned bytes_per_scanline;
	unsigned size;
	void* pointer;
} none_internal;

static none_internal none_state;

unsigned char* (*none_write_line)(unsigned y);

/***************************************************************************/
/* Internal */

static unsigned char* none_linear_write_line(unsigned y) {
	return (unsigned char*)none_state.pointer + none_state.bytes_per_scanline * y;
}

static device DEVICE[] = {
	{ "auto", -1, "No video" },
	{ 0, 0, 0 }
};

/***************************************************************************/
/* Public */

static video_bool none_is_active(void) {
	return none_state.active != 0;
}

static video_bool none_mode_is_active(void) {
	return none_state.mode_active != 0;
}

static video_error none_init(int device_id) {
	assert( !none_is_active() );

	none_state.size = 4*1024*1024;
	none_state.pointer = 0;

	none_state.active = 1;

	return 0;
}

static void none_done(void) {
	assert(none_is_active() && !none_mode_is_active() );

	assert( !none_state.pointer );
	none_state.active = 0;
}

static unsigned none_flags(void) {
	assert( none_is_active() );
	return VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL;
}

static video_error none_mode_set(const none_video_mode* mode) {

	none_write_line = none_linear_write_line;
	switch (mode->bits_per_pixel) {
		case 8 :
			none_state.bytes_per_pixel = 1;
			none_state.rgb_def = 0;
			break;
		case 15 :
			none_state.bytes_per_pixel = 2;
			none_state.rgb_def = video_rgb_def_make(5,10,5,5,5,0);
			break;
		case 16 :
			none_state.bytes_per_pixel = 2;
			none_state.rgb_def = video_rgb_def_make(5,11,6,5,5,0);
			break;
		case 24 :
			none_state.bytes_per_pixel = 3;
			none_state.rgb_def = video_rgb_def_make(8,16,8,8,8,0);
			break;
		case 32 :
			none_state.bytes_per_pixel = 4;
			none_state.rgb_def = video_rgb_def_make(8,16,8,8,8,0);
			break;
		default :
			return -1;
	}

	assert( !none_state.pointer );
	none_state.pointer = (unsigned char*)malloc(none_state.size);

	none_state.bytes_per_scanline = (none_state.bytes_per_pixel * mode->crtc.hde + 3) & ~3;
	none_state.mode_active = 1;

	return 0;
}

static void none_mode_done(video_bool restore) {
	assert(none_is_active() && none_mode_is_active());

	assert( none_state.pointer );
	free( none_state.pointer );
	none_state.pointer = 0;

	(void)restore; /* ignored */

	none_state.mode_active = 0;
}

static video_error none_mode_change(const none_video_mode* mode) {
	none_mode_done(1);
	return none_mode_set(mode);
}

static unsigned none_virtual_x(void) {
	assert(none_is_active() && none_mode_is_active());
	return none_state.bytes_per_scanline / none_state.bytes_per_pixel;
}

static unsigned none_virtual_y(void) {
	assert(none_is_active() && none_mode_is_active());
	return none_state.size / none_state.bytes_per_scanline;
}

static unsigned none_adjust_bytes_per_page(unsigned bytes_per_page) {
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

static unsigned none_bytes_per_scanline(void) {
	assert(none_is_active() && none_mode_is_active());
	return none_state.bytes_per_scanline;
}

static video_rgb_def none_rgb_def(void) {
	assert(none_is_active() && none_mode_is_active());
	return none_state.rgb_def;
}

static void none_wait_vsync(void) {
	assert(none_is_active() && none_mode_is_active());
}

static video_error none_scroll(unsigned offset, video_bool waitvsync) {
	assert(none_is_active() && none_mode_is_active());
	return 0;
}

static video_error none_scanline_set(unsigned byte_length) {
	assert(none_is_active() && none_mode_is_active());
	none_state.bytes_per_scanline = (byte_length + 3) & ~3;
	return 0;
}

static video_error none_palette8_set(const video_color* palette, unsigned start, unsigned count, video_bool waitvsync) {
	assert(none_is_active() && none_mode_is_active());
	return 0;
}

#define DRIVER(mode) ((none_video_mode*)(&mode->driver_mode))

static video_error none_mode_import(video_mode* mode, const none_video_mode* none_mode)
{
	strcpy(mode->name, none_mode->crtc.name);

	*DRIVER(mode) = *none_mode;

	mode->driver = &video_none_driver;
	mode->flags = VIDEO_FLAGS_ASYNC_SETPAGE
		| VIDEO_FLAGS_MEMORY_LINEAR
		| (mode->flags & VIDEO_FLAGS_USER_MASK);
	switch (none_mode->bits_per_pixel) {
		case 8 : mode->flags |= VIDEO_FLAGS_INDEX_PACKED | VIDEO_FLAGS_TYPE_GRAPHICS; break;
		default: mode->flags |= VIDEO_FLAGS_INDEX_RGB | VIDEO_FLAGS_TYPE_GRAPHICS; break;
	}

	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->bits_per_pixel = none_mode->bits_per_pixel;

	if (crtc_is_doublescan(&none_mode->crtc))
		mode->scan = 1;
	else if (crtc_is_interlace(&none_mode->crtc))
		mode->scan = -1;
	else
		mode->scan = 0;

	return 0;
}

static video_error none_mode_generate(none_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags)
{
	assert( none_is_active() );

	if (video_mode_generate_check("none",none_flags(),8,2048,crtc,bits,flags)!=0)
		return -1;

	mode->crtc = *crtc;
	mode->bits_per_pixel = bits;

	return 0;
}

#define COMPARE(a,b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

static int none_mode_compare(const none_video_mode* a, const none_video_mode* b) {
	COMPARE(a->bits_per_pixel,b->bits_per_pixel);
	return video_crtc_compare(&a->crtc,&b->crtc);
}

static void none_reg(struct conf_context* context) {
}

static video_error none_load(struct conf_context* context) {
	return 0;
}

/***************************************************************************/
/* Driver */

static video_error none_mode_set_void(const void* mode) {
	return none_mode_set((const none_video_mode*)mode);
}

static video_error none_mode_change_void(const void* mode) {
	return none_mode_change((const none_video_mode*)mode);
}

static video_error none_mode_import_void(video_mode* mode, const void* none_mode) {
	return none_mode_import(mode, (const none_video_mode*)none_mode);
}

static video_error none_mode_generate_void(void* mode, const video_crtc* crtc, unsigned bits, unsigned flags) {
	return none_mode_generate((none_video_mode*)mode,crtc,bits,flags);
}

static int none_mode_compare_void(const void* a, const void* b) {
	return none_mode_compare((const none_video_mode*)a, (const none_video_mode*)b);
}

static unsigned none_mode_size(void) {
	return sizeof(none_video_mode);
}

video_driver video_none_driver = {
	"none",
	DEVICE,
	none_load,
	none_reg,
	none_init,
	none_done,
	none_flags,
	none_mode_set_void,
	none_mode_change_void,
	none_mode_done,
	none_virtual_x,
	none_virtual_y,
	0,
	0,
	none_bytes_per_scanline,
	none_adjust_bytes_per_page,
	none_rgb_def,
	0,
	0,
	&none_write_line,
	none_wait_vsync,
	none_scroll,
	none_scanline_set,
	none_palette8_set,
	0,
	none_mode_size,
	0,
	none_mode_generate_void,
	none_mode_import_void,
	none_mode_compare_void,
	0
};


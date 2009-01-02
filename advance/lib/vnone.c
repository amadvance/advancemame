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

#include "portable.h"

#include "vnone.h"
#include "video.h"
#include "snstring.h"

/***************************************************************************/
/* State */

typedef struct none_internal_struct {
	adv_bool active;
	adv_bool mode_active;

	adv_color_def color_def;
	unsigned bytes_per_pixel;
	unsigned bytes_per_scanline;
	unsigned size;
	void* pointer;

	unsigned cap;
} none_internal;

static none_internal none_state;

unsigned char* (*none_write_line)(unsigned y);

/***************************************************************************/
/* Internal */

static unsigned char* none_linear_write_line(unsigned y)
{
	return (unsigned char*)none_state.pointer + none_state.bytes_per_scanline * y;
}

static adv_device DEVICE[] = {
	{ "auto", -1, "No video" },
	{ 0, 0, 0 }
};

/***************************************************************************/
/* Public */

static adv_bool none_is_active(void)
{
	return none_state.active != 0;
}

static adv_bool none_mode_is_active(void)
{
	return none_state.mode_active != 0;
}

static adv_error none_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	(void)cursor;
	(void)overlay_size;

	assert(!none_is_active());

	if (sizeof(none_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	none_state.cap = VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32 | VIDEO_DRIVER_FLAGS_MODE_YUY2
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL;

	switch (output) {
	case adv_output_auto :
	case adv_output_fullscreen :
		none_state.cap |= VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;
		break;
	case adv_output_window :
		none_state.cap |= VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW;
		break;
	case adv_output_overlay :
		none_state.cap |= VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY;
		break;
	}

	none_state.size = 4*1024*1024;
	none_state.pointer = 0;

	none_state.active = 1;

	return 0;
}

static void none_done(void)
{
	assert(none_is_active() && !none_mode_is_active());

	assert(!none_state.pointer);
	none_state.active = 0;
}

static unsigned none_flags(void)
{
	assert(none_is_active());
	return none_state.cap;
}

static adv_error none_mode_set(const none_video_mode* mode)
{
	none_write_line = none_linear_write_line;
	none_state.color_def = color_def_make_from_index(mode->index);
	none_state.bytes_per_pixel = index_bytes_per_pixel(mode->index);

	assert(!none_state.pointer);
	none_state.pointer = (unsigned char*)malloc(none_state.size);

	none_state.bytes_per_scanline = (none_state.bytes_per_pixel * mode->crtc.hde + 3) & ~3;
	none_state.mode_active = 1;

	return 0;
}

static void none_mode_done(adv_bool restore)
{
	assert(none_is_active() && none_mode_is_active());

	assert(none_state.pointer);
	free(none_state.pointer);
	none_state.pointer = 0;

	(void)restore; /* ignored */

	none_state.mode_active = 0;
}

static adv_error none_mode_change(const none_video_mode* mode)
{
	none_mode_done(1);
	return none_mode_set(mode);
}

static unsigned none_virtual_x(void)
{
	assert(none_is_active() && none_mode_is_active());
	return none_state.bytes_per_scanline / none_state.bytes_per_pixel;
}

static unsigned none_virtual_y(void)
{
	assert(none_is_active() && none_mode_is_active());
	return none_state.size / none_state.bytes_per_scanline;
}

static unsigned none_adjust_bytes_per_page(unsigned bytes_per_page)
{
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

static unsigned none_bytes_per_scanline(void)
{
	assert(none_is_active() && none_mode_is_active());
	return none_state.bytes_per_scanline;
}

static adv_color_def none_color_def(void)
{
	assert(none_is_active() && none_mode_is_active());
	return none_state.color_def;
}

static void none_wait_vsync(void)
{
	assert(none_is_active() && none_mode_is_active());
}

static adv_error none_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(none_is_active() && none_mode_is_active());
	return 0;
}

static adv_error none_scanline_set(unsigned byte_length)
{
	assert(none_is_active() && none_mode_is_active());
	none_state.bytes_per_scanline = (byte_length + 3) & ~3;
	return 0;
}

static adv_error none_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
	assert(none_is_active() && none_mode_is_active());
	return 0;
}

#define DRIVER(mode) ((none_video_mode*)(&mode->driver_mode))

static adv_error none_mode_import(adv_mode* mode, const none_video_mode* none_mode)
{
	sncpy(mode->name, MODE_NAME_MAX, none_mode->crtc.name);

	*DRIVER(mode) = *none_mode;

	mode->driver = &video_none_driver;
	mode->flags = MODE_FLAGS_RETRACE_WAIT_SYNC | MODE_FLAGS_RETRACE_SET_ASYNC
		| (mode->flags & MODE_FLAGS_USER_MASK)
		| none_mode->index;
	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);

	if (crtc_is_doublescan(&none_mode->crtc))
		mode->scan = 1;
	else if (crtc_is_interlace(&none_mode->crtc))
		mode->scan = -1;
	else
		mode->scan = 0;

	return 0;
}

static adv_error none_mode_generate(none_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	assert(none_is_active());

	if (video_mode_generate_check("none", none_flags(), 8, 2048, crtc, flags)!=0)
		return -1;

	mode->crtc = *crtc;
	mode->index = flags & MODE_FLAGS_INDEX_MASK;

	return 0;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

static int none_mode_compare(const none_video_mode* a, const none_video_mode* b)
{
	COMPARE(a->index, b->index);
	return crtc_compare(&a->crtc, &b->crtc);
}

static void none_reg(adv_conf* context)
{
}

static adv_error none_load(adv_conf* context)
{
	return 0;
}

void none_crtc_container_insert_default(adv_crtc_container* cc)
{
	crtc_container_insert_default_modeline_svga(cc);
}

/***************************************************************************/
/* Driver */

static adv_error none_mode_set_void(const void* mode)
{
	return none_mode_set((const none_video_mode*)mode);
}

static adv_error none_mode_change_void(const void* mode)
{
	return none_mode_change((const none_video_mode*)mode);
}

static adv_error none_mode_import_void(adv_mode* mode, const void* none_mode)
{
	return none_mode_import(mode, (const none_video_mode*)none_mode);
}

static adv_error none_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return none_mode_generate((none_video_mode*)mode, crtc, flags);
}

static int none_mode_compare_void(const void* a, const void* b)
{
	return none_mode_compare((const none_video_mode*)a, (const none_video_mode*)b);
}

static unsigned none_mode_size(void)
{
	return sizeof(none_video_mode);
}

adv_video_driver video_none_driver = {
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
	none_color_def,
	0,
	0,
	&none_write_line,
	none_wait_vsync,
	none_scroll,
	none_scanline_set,
	none_palette8_set,
	none_mode_size,
	0,
	none_mode_generate_void,
	none_mode_import_void,
	none_mode_compare_void,
	none_crtc_container_insert_default
};


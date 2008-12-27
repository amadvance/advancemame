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

#include "portable.h"

#include "vslang.h"
#include "log.h"
#include "error.h"
#include "snstring.h"

#include "oslinux.h"

#ifdef HAVE_SLANG_H
#include <slang.h>
#else
#ifdef HAVE_SLANG_SLANG_H
#include <slang/slang.h>
#else
#error slang.h file not found!
#endif
#endif

/***************************************************************************/
/* State */

typedef struct slang_internal_struct {
	adv_bool active;
	adv_bool mode_active;

	unsigned size_x;
	unsigned size_y;
	unsigned font_size_x;
	unsigned font_size_y;

	unsigned char* ptr;
} slang_internal;

static slang_internal slang_state;

unsigned char* (*slang_write_line)(unsigned y);

static adv_device DEVICE[] = {
{ "auto", -1, "sLang video" },
{ 0, 0, 0 }
};

/***************************************************************************/
/* Functions */

static unsigned char* slang_linear_write_line(unsigned y)
{
	return slang_state.ptr + slang_state.size_x * y * 2;
}

adv_bool slang_is_active(void)
{
	return slang_state.active != 0;
}

adv_bool slang_mode_is_active(void)
{
	return slang_state.mode_active != 0;
}

unsigned slang_flags(void)
{
	assert(slang_is_active());
	return VIDEO_DRIVER_FLAGS_MODE_TEXT | VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;
}

adv_error slang_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	assert(!slang_is_active());
	(void)cursor;

	log_std(("video:slang: slang_init()\n"));

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		error_set("Only fullscreen output is supported.\n");
		return -1;
	}

	if (!os_internal_slang_get()) {
		error_set("Unsupported without the slang library.\n");
		return -1;
	}

	slang_state.size_x = SLtt_Screen_Cols;
	slang_state.size_y = SLtt_Screen_Rows;

	slang_state.active = 1;
	return 0;
}

void slang_done(void)
{
	assert(slang_is_active() && !slang_mode_is_active());

	log_std(("video:slang: slang_done()\n"));

	slang_state.active = 0;
}

adv_error slang_mode_set(const slang_video_mode* mode)
{
	unsigned size;
	unsigned i;

	assert(slang_is_active() && !slang_mode_is_active());

	SLang_init_tty(-1, 0, 0);
	SLsmg_init_smg();
	SLtt_set_cursor_visibility(0);

	slang_state.font_size_x = mode->font_size_x;
	slang_state.font_size_y = mode->font_size_y;
	size = slang_state.size_x * slang_state.size_y;
	slang_state.ptr = malloc(size * 2);
	for(i=0;i<size;++i) {
		slang_state.ptr[i*2] = ' ';
		slang_state.ptr[i*2+1] = 0;
	}
	slang_write_line = slang_linear_write_line;

	slang_state.mode_active = 1;

	return 0;
}

void slang_mode_done(adv_bool restore)
{
	assert(slang_is_active() && slang_mode_is_active());

	free(slang_state.ptr);

	SLtt_set_cursor_visibility(1);
	SLsmg_reset_smg();
	SLang_reset_tty();

	(void)restore; /* ignored */

	slang_state.mode_active = 0;
}

unsigned slang_virtual_x(void)
{
	return slang_state.size_x * slang_state.font_size_x;
}

unsigned slang_virtual_y(void)
{
	return slang_state.size_y  * slang_state.font_size_y;
}

unsigned slang_bytes_per_scanline(void)
{
	return slang_state.size_x * 2;
}

static const char* COLOR[16] = {
	"black",
	"blue",
	"green",
	"cyan",
	"red",
	"magenta",
	"brown",
	"lightgray",
	"gray",
	"brightblue",
	"brightgreen",
	"brightcyan",
	"brightred",
	"brightmagenta",
	"yellow",
	"white"
};

#define COLOR_MAX 128

static unsigned color_map[COLOR_MAX];
static unsigned color_mac = 1; /* the first color is reserved */

static unsigned getattr(unsigned dos_attr)
{
	unsigned i;
	for(i=1;i<color_mac && i<COLOR_MAX;++i)
		if (color_map[i] == dos_attr)
			return i;
	if (color_mac < COLOR_MAX) {
		unsigned f = (dos_attr) & 0xF;
		unsigned b = (dos_attr >> 4) & 0xF;
		SLtt_set_color(color_mac, 0, (char*)COLOR[f], (char*)COLOR[b]);
		color_map[color_mac] = dos_attr;
		return color_mac++;
	} else
		return 0;
}

void slang_wait_vsync(void)
{
	int x, y;
	unsigned old_a;

	assert(slang_is_active() && slang_mode_is_active());

	old_a = 0x100;

	for(y=0;y<slang_state.size_y;++y) {
		unsigned char* line;
		SLsmg_gotorc(y, 0);
		line = slang_write_line(y);
		for(x=0;x<slang_state.size_x;++x) {
			unsigned a = line[2*x + 1];
			unsigned char c = line[2*x];
			if (a != old_a) {
				SLsmg_set_color(getattr(a));
				old_a = a;
			}
			SLsmg_write_char(c);
		}
	}
	SLsmg_gotorc(0, 0);

	/* update the physical display */
	SLsmg_refresh();
}

#define DRIVER(mode) ((slang_video_mode*)(&mode->driver_mode))

adv_error slang_mode_import(adv_mode* mode, const slang_video_mode* slang_mode)
{
	sncpy(mode->name, MODE_NAME_MAX, "slang");

	*DRIVER(mode) = *slang_mode;

	mode->driver = &video_slang_driver;
	mode->flags = MODE_FLAGS_INDEX_TEXT |
		(mode->flags & MODE_FLAGS_USER_MASK);
	mode->size_x = slang_state.size_x * slang_mode->font_size_x;
	mode->size_y = slang_state.size_y * slang_mode->font_size_y;
	mode->vclock = 0;
	mode->hclock = 0;
	mode->scan = 0; /* assume singlescan */

	return 0;
}

adv_error slang_mode_grab(slang_video_mode* mode)
{
	assert(slang_is_active());

	mode->font_size_x = 8;
	mode->font_size_y = 16;

	return 0;
}

adv_error slang_mode_generate(slang_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	assert(slang_is_active());

	error_nolog_set("Mode generation not supported.\n");

	/* always fail, no hardware control (only the grabbed mode is used) */
	return -1;
}

unsigned slang_font_size_x(void)
{
	return slang_state.font_size_x;
}

unsigned slang_font_size_y(void)
{
	return slang_state.font_size_y;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

int slang_mode_compare(const slang_video_mode* a, const slang_video_mode* b)
{
	COMPARE(a->font_size_y, b->font_size_y);
	COMPARE(a->font_size_x, b->font_size_x);
	return 0;
}

void slang_default(void)
{
}

void slang_reg(adv_conf* context)
{
	assert(!slang_is_active());
}

adv_error slang_load(adv_conf* context)
{
	assert(!slang_is_active());
	return 0;
}

adv_color_def slang_color_def(void)
{
	return color_def_make(adv_color_type_text);
}

/***************************************************************************/
/* Driver */

static adv_error slang_mode_set_void(const void* mode)
{
	return slang_mode_set((const slang_video_mode*)mode);
}

static adv_error slang_mode_import_void(adv_mode* mode, const void* slang_mode)
{
	return slang_mode_import(mode, (const slang_video_mode*)slang_mode);
}

static adv_error slang_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return slang_mode_generate((slang_video_mode*)mode, crtc, flags);
}

static int slang_mode_compare_void(const void* a, const void* b)
{
	return slang_mode_compare((const slang_video_mode*)a, (const slang_video_mode*)b);
}

static unsigned slang_mode_size(void)
{
	return sizeof(slang_video_mode);
}

static adv_error slang_mode_grab_void(void* mode)
{
	return slang_mode_grab((slang_video_mode*)mode);
}

adv_video_driver video_slang_driver = {
	"slang",
	DEVICE,
	slang_load,
	slang_reg,
	slang_init,
	slang_done,
	slang_flags,
	slang_mode_set_void,
	0,
	slang_mode_done,
	slang_virtual_x,
	slang_virtual_y,
	slang_font_size_x,
	slang_font_size_y,
	slang_bytes_per_scanline,
	0,
	slang_color_def,
	0,
	0,
	&slang_write_line,
	slang_wait_vsync,
	0,
	0,
	0,
	slang_mode_size,
	slang_mode_grab_void,
	slang_mode_generate_void,
	slang_mode_import_void,
	slang_mode_compare_void,
	0
};


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004 Andrea Mazzoleni
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

#include "vcurses.h"
#include "log.h"
#include "error.h"
#include "snstring.h"

#include "oslinux.h"

#include <curses.h>

/***************************************************************************/
/* State */

typedef struct curses_internal_struct {
	adv_bool active;
	adv_bool mode_active;

	unsigned size_x;
	unsigned size_y;
	unsigned font_size_x;
	unsigned font_size_y;

	unsigned char* ptr;
} curses_internal;

static curses_internal curses_state;

unsigned char* (*curses_write_line)(unsigned y);

static adv_device DEVICE[] = {
{ "auto", -1, "CURSES video" },
{ 0, 0, 0 }
};

/***************************************************************************/
/* Functions */

static unsigned char* curses_linear_write_line(unsigned y)
{
	return curses_state.ptr + curses_state.size_x * y * 2;
}

adv_bool curses_is_active(void)
{
	return curses_state.active != 0;
}

adv_bool curses_mode_is_active(void)
{
	return curses_state.mode_active != 0;
}

unsigned curses_flags(void)
{
	assert(curses_is_active());
	return VIDEO_DRIVER_FLAGS_MODE_TEXT | VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;
}

adv_error curses_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	assert(!curses_is_active());
	(void)cursor;

	log_std(("video:curses: curses_init()\n"));

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		error_set("Only fullscreen output is supported.\n");
		return -1;
	}

	if (!os_internal_curses_get()) {
		error_set("Unsupported without the curses library.\n");
		return -1;
	}

	curses_state.size_x = COLS;
	curses_state.size_y = LINES;

	curses_state.active = 1;
	return 0;
}

void curses_done(void)
{
	assert(curses_is_active() && !curses_mode_is_active());

	log_std(("video:curses: curses_done()\n"));

	curses_state.active = 0;
}

adv_error curses_mode_set(const curses_video_mode* mode)
{
	unsigned size;
	unsigned i;

	assert(curses_is_active() && !curses_mode_is_active());

	scrollok(stdscr, FALSE);

	curses_state.font_size_x = mode->font_size_x;
	curses_state.font_size_y = mode->font_size_y;
	size = curses_state.size_x * curses_state.size_y;
	curses_state.ptr = malloc(size * 2);
	for(i=0;i<size;++i) {
		curses_state.ptr[i*2] = ' ';
		curses_state.ptr[i*2+1] = 0;
	}
	curses_write_line = curses_linear_write_line;

	curses_state.mode_active = 1;

	return 0;
}

void curses_mode_done(adv_bool restore)
{
	assert(curses_is_active() && curses_mode_is_active());

	free(curses_state.ptr);

	scrollok(stdscr, TRUE);
	endwin();

	(void)restore; /* ignored */

	curses_state.mode_active = 0;
}

unsigned curses_virtual_x(void)
{
	return curses_state.size_x * curses_state.font_size_x;
}

unsigned curses_virtual_y(void)
{
	return curses_state.size_y  * curses_state.font_size_y;
}

unsigned curses_bytes_per_scanline(void)
{
	return curses_state.size_x * 2;
}

static int COLOR[8] = {
	COLOR_BLACK,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_CYAN,
	COLOR_RED,
	COLOR_MAGENTA,
	COLOR_YELLOW,
	COLOR_WHITE
};

#define COLOR_MAX 128

struct color_pair {
	unsigned dos_attr;
	unsigned attr;
};

static struct color_pair color_map[COLOR_MAX];

static unsigned color_mac = 1; /* the first color is reserved */

static unsigned getattr(unsigned dos_attr)
{
	unsigned i;
	for(i=1;i<color_mac && i<COLOR_MAX;++i)
		if (color_map[i].dos_attr == dos_attr)
			return color_map[i].attr;
	if (color_mac<COLOR_MAX && color_mac<(unsigned)COLOR_PAIRS) {
		unsigned f = (dos_attr) & 0x7;
		unsigned b = (dos_attr >> 4) & 0x7;
		unsigned attr;
		init_pair( color_mac, COLOR[f], COLOR[b]);
		attr = COLOR_PAIR( color_mac );
		if (dos_attr & 0x08)
			attr |= A_BOLD;
		if (dos_attr & 0x80)
			attr |= A_BLINK;
		color_map[color_mac].dos_attr = dos_attr;
		color_map[color_mac].attr = attr;
		++color_mac;
		return attr;
	} else
		return COLOR_PAIR( 0 );
}

void curses_wait_vsync(void)
{
	int x, y;
	unsigned old_a;
	unsigned old_m;

	assert(curses_is_active() && curses_mode_is_active());

	old_a = 0x100;
	old_m = 0;

	for(y=0;y<curses_state.size_y;++y) {
		unsigned char* line;
		move(y, 0);
		line = curses_write_line(y);
		for(x=0;x<curses_state.size_x;++x) {
			unsigned a = line[2*x+1];
			unsigned c = line[2*x];
			if (a != old_a) {
				old_m = getattr(a);
				old_a = a;
			}
			addch(c | old_m);
		}
	}

	refresh();
}

#define DRIVER(mode) ((curses_video_mode*)(&mode->driver_mode))

adv_error curses_mode_import(adv_mode* mode, const curses_video_mode* curses_mode)
{
	sncpy(mode->name, MODE_NAME_MAX, "curses");

	*DRIVER(mode) = *curses_mode;

	mode->driver = &video_curses_driver;
	mode->flags = MODE_FLAGS_INDEX_TEXT |
		(mode->flags & MODE_FLAGS_USER_MASK);
	mode->size_x = curses_state.size_x * curses_mode->font_size_x;
	mode->size_y = curses_state.size_y * curses_mode->font_size_y;
	mode->vclock = 0;
	mode->hclock = 0;
	mode->scan = 0; /* assume singlescan */

	return 0;
}

adv_error curses_mode_grab(curses_video_mode* mode)
{
	assert(curses_is_active());

	mode->font_size_x = 8;
	mode->font_size_y = 16;

	return 0;
}

adv_error curses_mode_generate(curses_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	assert(curses_is_active());

	error_nolog_set("Mode generation not supported.\n");

	/* always fail, no hardware control (only the grabbed mode is used) */
	return -1;
}

unsigned curses_font_size_x(void)
{
	return curses_state.font_size_x;
}

unsigned curses_font_size_y(void)
{
	return curses_state.font_size_y;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

int curses_mode_compare(const curses_video_mode* a, const curses_video_mode* b)
{
	COMPARE(a->font_size_y, b->font_size_y);
	COMPARE(a->font_size_x, b->font_size_x);
	return 0;
}

void curses_default(void)
{
}

void curses_reg(adv_conf* context)
{
	assert(!curses_is_active());
}

adv_error curses_load(adv_conf* context)
{
	assert(!curses_is_active());
	return 0;
}

adv_color_def curses_color_def(void)
{
	return color_def_make(adv_color_type_text);
}

/***************************************************************************/
/* Driver */

static adv_error curses_mode_set_void(const void* mode)
{
	return curses_mode_set((const curses_video_mode*)mode);
}

static adv_error curses_mode_import_void(adv_mode* mode, const void* curses_mode)
{
	return curses_mode_import(mode, (const curses_video_mode*)curses_mode);
}

static adv_error curses_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return curses_mode_generate((curses_video_mode*)mode, crtc, flags);
}

static int curses_mode_compare_void(const void* a, const void* b)
{
	return curses_mode_compare((const curses_video_mode*)a, (const curses_video_mode*)b);
}

static unsigned curses_mode_size(void)
{
	return sizeof(curses_video_mode);
}

static adv_error curses_mode_grab_void(void* mode)
{
	return curses_mode_grab((curses_video_mode*)mode);
}

adv_video_driver video_curses_driver = {
	"curses",
	DEVICE,
	curses_load,
	curses_reg,
	curses_init,
	curses_done,
	curses_flags,
	curses_mode_set_void,
	0,
	curses_mode_done,
	curses_virtual_x,
	curses_virtual_y,
	curses_font_size_x,
	curses_font_size_y,
	curses_bytes_per_scanline,
	0,
	curses_color_def,
	0,
	0,
	&curses_write_line,
	curses_wait_vsync,
	0,
	0,
	0,
	curses_mode_size,
	curses_mode_grab_void,
	curses_mode_generate_void,
	curses_mode_import_void,
	curses_mode_compare_void,
	0
};


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

#include "update.h"
#include "video.h"

#include <assert.h>

/* Position for drawing */
static unsigned video_update_offset;

/* Screen in drawing */
static unsigned _video_update_page;

/* Draw possible (boolean) */
static int video_update_draw_allowed;

/* Pages used */
static unsigned _video_update_page_max;

/* Inizialize */
void update_init(int max_buffer) {
	int pages = (video_virtual_y() * video_bytes_per_scanline()) / video_bytes_per_page();
	if (pages > max_buffer)
		pages = max_buffer;

	_video_update_page = 0;

	if (pages >= 3 && (video_flags() & VIDEO_FLAGS_SYNC_SETPAGE)) {
		_video_update_page_max = 3;
	} else if (pages >= 2 && (video_flags() & VIDEO_FLAGS_ASYNC_SETPAGE)) {
		_video_update_page_max = 2;
	} else {
		_video_update_page_max = 1;
	}

	video_update_draw_allowed = 0;
}

/* Return the number of pages used */
unsigned update_page_max_get(void) {
	return _video_update_page_max;
}

/* Deinizialize */
void update_done(void) {
	assert( video_update_draw_allowed == 0 );

	/* display the first page */
	if (_video_update_page_max!=1) {
		video_display_set_async(0,0);
	}
}

/* Relative position of the page in drawing */
unsigned update_x_get(void) {
	unsigned x;
	assert( video_update_draw_allowed );
	x = (video_update_offset % video_bytes_per_scanline()) / video_bytes_per_pixel();
	if (video_is_unchained())
		x *= 4;
	return x;
}

unsigned update_y_get(void) {
	assert( video_update_draw_allowed );
	return video_update_offset / video_bytes_per_scanline();
}

/* Page in drawing */
unsigned update_page_get(void) {
	assert( video_update_draw_allowed );
	return _video_update_page;
}

/* Start drawing to page */
void update_start(void) {
	assert( video_update_draw_allowed == 0 );
	video_update_draw_allowed = 1;
	/* compute coordinate for drawing */
	if (_video_update_page_max!=1) {
		video_update_offset = _video_update_page * video_bytes_per_page();
	} else {
		video_update_offset = 0;
	}

	video_write_lock();
}

/* End drawing to page */
void update_stop(unsigned x, unsigned y, unsigned size_x, unsigned size_y, adv_bool wait_retrace) {
	assert( video_update_draw_allowed );
	video_update_draw_allowed = 0;

	video_write_unlock(x, y, size_x, size_y);

	if (_video_update_page_max!=1) {
		unsigned offset;
		offset = _video_update_page * video_bytes_per_page();
		video_display_set_async(offset,wait_retrace);

		/* next page to draw */
		++_video_update_page;
		if (_video_update_page == _video_update_page_max)
			_video_update_page = 0;
	} else {
		if (wait_retrace)
			video_wait_vsync();
	}
}

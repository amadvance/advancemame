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

#include "advance.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

const char* mode_current_name(const struct advance_video_context* context) {
	return crtc_name_get(context->state.crtc_selected);
}

int mode_current_magnify(const struct advance_video_context* context) {
	if (context->state.game_visible_size_x * 2 <= context->state.mode_visible_size_x
		&& context->state.game_visible_size_y * 2 <= context->state.mode_visible_size_y)
		return 1;
	else
		return 0;
}

/* Return the stretch used by the video configuration */
int mode_current_stretch(const struct advance_video_context* context) {
	if (context->state.mode_visible_size_x == context->state.game_visible_size_x
		&& context->state.mode_visible_size_y == context->state.game_visible_size_y)
		return STRETCH_NONE;
	if (context->state.mode_visible_size_x % context->state.game_visible_size_x == 0
		&& context->state.mode_visible_size_y % context->state.game_visible_size_y == 0)
		return STRETCH_INTEGER_XY;
	if (context->state.mode_visible_size_x % context->state.game_visible_size_x == 0)
		return STRETCH_INTEGER_X_FRACTIONAL_Y;
	return STRETCH_FRACTIONAL_XY;
}

/* Return the description of a video configuration */
const char* mode_desc(struct advance_video_context* context, const video_crtc* crtc) {
	static char buffer[128];
	double factor_x = (double)crtc_hsize_get(crtc) / context->state.mode_best_size_x;
	double factor_y = (double)crtc_vsize_get(crtc) / context->state.mode_best_size_y;
	char c;
	if (crtc_is_doublescan(crtc))
		c = 'd';
	else if (crtc_is_interlace(crtc))
		c = 'i';
	else
		c = 's';
	sprintf(buffer,"%4dx%4d%c %4.2fx%4.2f", crtc_hsize_get(crtc), crtc_vsize_get(crtc), c, factor_x, factor_y);
	return buffer;
}

/* Compute the MCD of two number (Euclide) */
static unsigned long long aspect_MCD(unsigned long long m, unsigned long long n) {
	while (n) {
		unsigned long long r = m % n;
		m = n;
		n = r;
		}
	return m;
}

/* Reduce a fraction */
void video_aspect_reduce(unsigned long long* a, unsigned long long* b) {
	unsigned long long r = aspect_MCD(*a,*b);
	*a /= r;
	*b /= r;
}

/***************************************************************************/
/* Scoring */

/* Less is better */

static int score_compare_scanline(const struct advance_video_context* context, const video_crtc* a, const video_crtc* b) {
	int as;
	int bs;

	if (context->config.scanlines_flag) {
		if (crtc_is_doublescan(a))
			as = 1;
		else if (crtc_is_interlace(a))
			as = 2;
		else
			as = 0;
		if (crtc_is_doublescan(b))
			bs = 1;
		else if (crtc_is_interlace(b))
			bs = 2;
		else
			bs = 0;
	} else {
		if (crtc_is_doublescan(a))
			as = 0;
		else if (crtc_is_interlace(a))
			as = 2;
		else
			as = 1;
		if (crtc_is_doublescan(b))
			bs = 0;
		else if (crtc_is_interlace(b))
			bs = 2;
		else
			bs = 1;
	}

	if (as < bs)
		return -1;
	if (as > bs)
		return 1;

	return 0;
}

static int score_compare_size(const struct advance_video_context* context, const video_crtc* a, const video_crtc* b) {
	int r;
	unsigned best_size_x;
	unsigned best_size_y;

	if (context->config.magnify_flag) {
		best_size_x = context->state.mode_best_size_2x;
		best_size_y = context->state.mode_best_size_2y;
	} else {
		best_size_x = context->state.mode_best_size_x;
		best_size_y = context->state.mode_best_size_y;
	}

	/* nearest is lower */
	r = abs( crtc_vsize_get(a) - best_size_y) - abs( crtc_vsize_get(b) - best_size_y);
	if (r)
		return r;

	/* bigger is lower */
	r = crtc_vsize_get(b) - crtc_vsize_get(a);
	if (r)
		return r;

	/* if adjusted in horizontal size the crtc are equal */
	if ((context->config.adjust & ADJUST_ADJUST_X) != 0)
		return 0;

	/* nearest is lower */
	r = abs( crtc_hsize_get(a) - best_size_x) - abs( crtc_hsize_get(b) - best_size_x);
	if (r)
		return r;

	return 0;
}

/* Scale the rate removing any integer factor */
double video_rate_scale_down(double rate, double reference) {
	double divisor = 1;
	while (rate / divisor >= 1.41 * reference) /* sqrt(2) */
		divisor = divisor + 1;
	return rate / divisor;
}

/**
 * Compare the clock of a video configuration.
 */
static int score_compare_frequency(const struct advance_video_context* context, const video_crtc* a, const video_crtc* b) {
	double freq_a;
	double freq_b;
	double err_a;
	double err_b;

	/* if adjusted in clock the crtc are equal */
	if ((context->config.adjust & ADJUST_ADJUST_CLOCK) != 0)
		return 0;

	freq_a = video_rate_scale_down(crtc_vclock_get(a), context->state.mode_best_vclock);
	freq_b = video_rate_scale_down(crtc_vclock_get(b), context->state.mode_best_vclock);

	err_a = fabs(freq_a - context->state.mode_best_vclock);
	err_b = fabs(freq_b - context->state.mode_best_vclock);

	if (err_a < err_b)
		return -1;
	if (err_a > err_b)
		return 1;

	return 0;
}

/**
 * Compare two video configuration.
 */
static int score_compare_crtc(const struct advance_video_context* context, const video_crtc* a, const video_crtc* b) {
	int r;

	r = score_compare_size(context, a, b);
	if (r) return r;

	if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0) {
		/* only for programamble drivers */

		r = score_compare_scanline(context, a, b);
		if (r) return r;

		r = score_compare_frequency(context, a, b);
		if (r) return r;
	}

	if (strcmp(crtc_name_get(a),crtc_name_get(b))!=0)
		log_std(("video config compare indecision for %s, %s\n", crtc_name_get(a), crtc_name_get(b)));

	return 0;
}

/* Context variable needed by qsort */
static const struct advance_video_context* the_context;

static int void_score_compare_crtc(const void* a, const void* b) {
	return score_compare_crtc(the_context, *(const video_crtc**)a, *(const video_crtc**)b);
}

void crtc_sort(const struct advance_video_context* context, const video_crtc** map, unsigned mac) {
	the_context = context;
	qsort(map,mac,sizeof(map[0]),void_score_compare_crtc);
}



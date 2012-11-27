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

#include "emu.h"

#include "advance.h"

#include <math.h>

const char* mode_current_name(const struct advance_video_context* context)
{
	return crtc_name_get(context->state.crtc_selected);
}

unsigned mode_current_magnify(const struct advance_video_context* context)
{
	if (context->state.game_visible_size_x * 4 <= context->state.mode_visible_size_x
		&& context->state.game_visible_size_y * 4 <= context->state.mode_visible_size_y)
		return 4;

	if (context->state.game_visible_size_x * 3 <= context->state.mode_visible_size_x
		&& context->state.game_visible_size_y * 3 <= context->state.mode_visible_size_y)
		return 3;

	if (context->state.game_visible_size_x * 2 <= context->state.mode_visible_size_x
		&& context->state.game_visible_size_y * 2 <= context->state.mode_visible_size_y)
		return 2;

	return 1;
}

/* Return the stretch used by the video configuration */
int mode_current_stretch(const struct advance_video_context* context)
{
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
void mode_desc_print(struct advance_video_context* context, char* buffer, unsigned size, const adv_crtc* crtc)
{
	double factor_x = (double)crtc_hsize_get(crtc) / context->state.mode_best_size_x;
	double factor_y = (double)crtc_vsize_get(crtc) / context->state.mode_best_size_y;
	char c;
	if (crtc_is_doublescan(crtc))
		c = 'd';
	else if (crtc_is_interlace(crtc))
		c = 'i';
	else
		c = 's';

	if ((context->config.adjust & ADJUST_ADJUST_X) != 0) {
		/* no real x size, it may be adjusted */
		snprintf(buffer, size, "%4d%c %4.2f", crtc_vsize_get(crtc), c, factor_y);
	} else {
		snprintf(buffer, size, "%4dx%4d%c %4.2fx%4.2f", crtc_hsize_get(crtc), crtc_vsize_get(crtc), c, factor_x, factor_y);
	}
}

/* Compute the MCD of two number (Euclide) */
static unsigned long long aspect_MCD(unsigned long long m, unsigned long long n)
{
	while (n) {
		unsigned long long r = m % n;
		m = n;
		n = r;
		}
	return m;
}

/* Reduce a fraction */
void video_aspect_reduce(unsigned long long* a, unsigned long long* b)
{
	unsigned long long r = aspect_MCD(*a, *b);
	*a /= r;
	*b /= r;
}

/***************************************************************************/
/* Scoring */

/* Compare video mode attributes, on comparing lesser values are better */

static int score_compare_scanline(const struct advance_video_context* context, const adv_crtc* a, const adv_crtc* b)
{
	int as;
	int bs;

	log_std(("emu:video: compare scanline\n"));

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

/**
 * Check if a factor has an good scale effect.
 */
static adv_bool factor_has_effect(unsigned x, unsigned y)
{
	if (x == 1 && y == 1) /* real size is assumed a good scale effect */
		return 1;
	if (x == 2 && y == 2)
		return 1;
	if (x == 2 && y == 3)
		return 1;
	if (x == 2 && y == 4)
		return 1;
	if (x == 3 && y == 3)
		return 1;
	if (x == 4 && y == 4)
		return 1;

	return 0;
}

/**
 * Score modes comparing two dimensions. 
 * Modes which have a both dimensions as an exact multipler of the requested size
 * are scored better. All the modes which are not exact multiplier all scored all
 * equals.
 */
static int score_compare_dim2(
	unsigned ax, unsigned ay,
	unsigned bx, unsigned by,
	unsigned rx, unsigned ry,
	unsigned mx0, unsigned mx1, unsigned mx2, unsigned mx3,
	unsigned my0, unsigned my1, unsigned my2, unsigned my3,
	adv_bool best_auto)
{
	adv_bool ae;
	adv_bool be;
	unsigned axi;
	unsigned ayi;
	unsigned bxi;
	unsigned byi;

	if (ax == mx0)
		axi = 1;
	else if (ax == mx1)
		axi = 2;
	else if (ax == mx2)
		axi = 3;
	else if (ax == mx3)
		axi = 4;
	else
		axi = 0;

	if (ay == my0)
		ayi = 1;
	else if (ay == my1)
		ayi = 2;
	else if (ay == my2)
		ayi = 3;
	else if (ay == my3)
		ayi = 4;
	else
		ayi = 0;

	if (bx == mx0)
		bxi = 1;
	else if (bx == mx1)
		bxi = 2;
	else if (bx == mx2)
		bxi = 3;
	else if (bx == mx3)
		bxi = 4;
	else
		bxi = 0;

	if (by == my0)
		byi = 1;
	else if (by == my1)
		byi = 2;
	else if (by == my2)
		byi = 3;
	else if (by == my3)
		byi = 4;
	else
		byi = 0;

	ae = axi > 0 && ayi > 0;
	be = bxi > 0 && byi > 0;

	if (ae && !be) {
		return -1;
	} else if (!ae && be) {
		return 1;
	} else if (!ae && !be) {
		return 0;
	} else {
		/* both are an exact multiplier */
		unsigned as = ax * ay;
		unsigned bs = bx * by;
		unsigned rs = rx * ry;
		unsigned ad = abs(as - rs);
		unsigned bd = abs(bs - rs);

		/* use at least an area big as the requested */
		/* note that this is valid only if all the dimensions are exact multipliers */
		if (as >= rs && bs < rs)
			return -1;
		else if (as < rs && bs >= rs)
			return 1;

		/* if the scale factor is automatically choosen, favorite factors with effects */
		if (best_auto) {
			/* use the same x and y scale factors */
			if (factor_has_effect(axi, ayi) && !factor_has_effect(bxi, byi))
				return -1;
			else if (!factor_has_effect(axi, ayi) && factor_has_effect(bxi, byi))
				return 1;
		}

		if (ad != bd)
			return ad - bd; /* smaller surface difference is better */
		else
			return as - bs; /* smaller surface is better */
	}
}

/**
 * Score modes comparing one dimension.
 * Favorite in order:
 * - Modes which have one dimension as an exact multipler of the requested dimension.
 * - The nearest size at the requested dimension.
 * - The smaller dimension.
 */
static int score_compare_dim1(
	unsigned a,
	unsigned b,
	unsigned r,
	unsigned m0, unsigned m1, unsigned m2, unsigned m3)
{
	adv_bool ae;
	adv_bool be;

	ae = (a == m0 || a == m1 || a == m2 || a == m3);
	be = (b == m0 || b == m1 || b == m2 || b == m3);

	if (ae && !be) {
		return -1;
	} else if (!ae && be) {
		return 1;
	} else {
		unsigned ad = abs(a - r);
		unsigned bd = abs(b - r);

		if (ad != bd)
			return ad - bd; /* smaller difference is better */
		else
			return a - b; /* smaller value is better */
	}
}

static int score_compare_size(const struct advance_video_context* context, const adv_crtc* a, const adv_crtc* b)
{
	int r;
	unsigned best_size_x;
	unsigned best_size_y;
	unsigned av;
	unsigned bv;
	adv_bool best_auto;
	unsigned best_area;

	best_auto = 0;

	if (!context->state.game_vector_flag) {
		switch (context->config.magnify_factor) {
		case 1 :
			best_size_x = context->state.mode_best_size_x;
			best_size_y = context->state.mode_best_size_y;
			break;
		case 2 :
			best_size_x = context->state.mode_best_size_2x;
			best_size_y = context->state.mode_best_size_2y;
			break;
		case 3 :
			best_size_x = context->state.mode_best_size_3x;
			best_size_y = context->state.mode_best_size_3y;
			break;
		case 4 :
			best_size_x = context->state.mode_best_size_4x;
			best_size_y = context->state.mode_best_size_4y;
			break;
		default :
			best_area = context->config.magnify_size * context->config.magnify_size;
			if (context->state.game_used_size_x * context->state.game_used_size_y * 16 <= best_area) {
				best_size_x = context->state.mode_best_size_4x;
				best_size_y = context->state.mode_best_size_4y;
			} else if (context->state.game_used_size_x * context->state.game_used_size_y * 9 <= best_area) {
				best_size_x = context->state.mode_best_size_3x;
				best_size_y = context->state.mode_best_size_3y;
			} else if (context->state.game_used_size_x * context->state.game_used_size_y * 4 <= best_area) {
				best_size_x = context->state.mode_best_size_2x;
				best_size_y = context->state.mode_best_size_2y;
			} else {
				best_size_x = context->state.mode_best_size_x;
				best_size_y = context->state.mode_best_size_y;
			}

			best_auto = 1;
			break;
		}
	} else {
		best_size_x = context->state.mode_best_size_x;
		best_size_y = context->state.mode_best_size_y;
	}

	log_std(("emu:video: compare size integer multiplier\n"));

	r = score_compare_dim2(
		crtc_hsize_get(a), crtc_vsize_get(a),
		crtc_hsize_get(b), crtc_vsize_get(b),
		best_size_x, best_size_y,
		context->state.mode_best_size_x, context->state.mode_best_size_2x, context->state.mode_best_size_3x, context->state.mode_best_size_4x,
		context->state.mode_best_size_y, context->state.mode_best_size_2y, context->state.mode_best_size_3y, context->state.mode_best_size_4y,
		best_auto
	);
	if (r)
		return r;

	log_std(("emu:video: compare size near\n"));

	r = score_compare_dim1(
		crtc_vsize_get(a),
		crtc_vsize_get(b),
		best_size_y,
		context->state.mode_best_size_y, context->state.mode_best_size_2y, context->state.mode_best_size_3y, context->state.mode_best_size_4y
	);
	if (r)
		return r;

	/* if adjusted in horizontal size the crtc are equal */
	if ((context->config.adjust & ADJUST_ADJUST_X) != 0)
		return 0;

	/* nearest is lower */
	r = score_compare_dim1(
		crtc_hsize_get(a),
		crtc_hsize_get(b),
		best_size_x,
		context->state.mode_best_size_x, context->state.mode_best_size_2x, context->state.mode_best_size_3x, context->state.mode_best_size_4x
	);
	if (r)
		return r;

	return 0;
}

/* Scale the rate removing any integer factor */
double video_rate_scale_down(double rate, double reference)
{
	double divisor = 1;
	while (rate / divisor >= 1.41 * reference) /* sqrt(2) */
		divisor = divisor + 1;
	return rate / divisor;
}

/**
 * Compare the clock of a video configuration.
 */
static int score_compare_frequency(const struct advance_video_context* context, const adv_crtc* a, const adv_crtc* b)
{
	double freq_a;
	double freq_b;
	double err_a;
	double err_b;

	log_std(("emu:video: compare frequency\n"));

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
static int score_compare_crtc(const struct advance_video_context* context, const adv_crtc* a, const adv_crtc* b)
{
	int r;

	r = score_compare_size(context, a, b);
	if (r) return r;

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0) {
		/* only for programamble drivers */

		r = score_compare_scanline(context, a, b);
		if (r) return r;

		r = score_compare_frequency(context, a, b);
		if (r) return r;
	}

	return 0;
}

/* Context variable needed by qsort */
static const struct advance_video_context* the_context;

static int void_score_compare_crtc(const void* _a, const void* _b)
{
	int r;

	const adv_crtc* a = *(const adv_crtc**)_a;
	const adv_crtc* b = *(const adv_crtc**)_b;

	r = score_compare_crtc(the_context, a, b);

	if (r < 0)
		log_std(("emu:video: compare %s/%s, first is better\n", crtc_name_get(a), crtc_name_get(b)));
	else if (r > 0)
		log_std(("emu:video: compare %s/%s, second is better\n", crtc_name_get(a), crtc_name_get(b)));
	else
		log_std(("emu:video: compare %s/%s, equal\n", crtc_name_get(a), crtc_name_get(b)));

	return r;
}

void crtc_sort(const struct advance_video_context* context, const adv_crtc** map, unsigned mac)
{
	the_context = context;
	qsort(map, mac, sizeof(map[0]), void_score_compare_crtc);
}


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

#include "emu.h"

static inline double estimate_merge(double estimator, double v)
{
	return 0.95 * estimator + 0.05 * v;
}

void advance_estimate_init(struct advance_estimate_context* context, double step)
{
	context->estimate_mame_flag = 0;
	context->estimate_osd_flag = 0;
	context->estimate_frame_flag = 0;
	context->estimate_common_flag = 0;

	context->estimate_frame = step;
	context->estimate_mame_skip = 0.01 * step;
	context->estimate_mame_full = 0.7 * step;
	context->estimate_osd_skip = 0.01 * step;
	context->estimate_osd_full = 0.1 * step;
	context->estimate_common_skip = 0.001 * step;
	context->estimate_common_full = 0.001 * step;
}

void advance_estimate_mame_end(struct advance_estimate_context* context, adv_bool skip_flag)
{
	double current = advance_timer();

	if (context->estimate_mame_flag) {
		double previous;
		previous = current - context->estimate_mame_last;
		if (skip_flag)
			context->estimate_mame_skip = estimate_merge(context->estimate_mame_skip, previous);
		else
			context->estimate_mame_full = estimate_merge(context->estimate_mame_full, previous);
	}
}

void advance_estimate_osd_begin(struct advance_estimate_context* context)
{
	double current = advance_timer();

	context->estimate_osd_flag = 1;
	context->estimate_osd_last = current;
}

void advance_estimate_osd_end(struct advance_estimate_context* context, adv_bool skip_flag)
{
	double current = advance_timer();

	if (context->estimate_osd_flag) {
		double previous;
		previous = current - context->estimate_osd_last;
		if (skip_flag)
			context->estimate_osd_skip = estimate_merge(context->estimate_osd_skip, previous);
		else
			context->estimate_osd_full = estimate_merge(context->estimate_osd_full, previous);
	}
}

void advance_estimate_common_begin(struct advance_estimate_context* context)
{
	double current = advance_timer();

	context->estimate_common_flag = 1;
	context->estimate_common_last = current;
}

void advance_estimate_common_end(struct advance_estimate_context* context, adv_bool skip_flag)
{
	double current = advance_timer();

	if (context->estimate_common_flag) {
		double previous;
		previous = current - context->estimate_common_last;
		if (skip_flag)
			context->estimate_common_skip = estimate_merge(context->estimate_common_skip, previous);
		else
			context->estimate_common_full = estimate_merge(context->estimate_common_full, previous);
	}
}

void advance_estimate_mame_begin(struct advance_estimate_context* context)
{
	double current = advance_timer();

	context->estimate_mame_flag = 1;
	context->estimate_mame_last = current;
}

void advance_estimate_frame(struct advance_estimate_context* context)
{
	double current = advance_timer();

	if (context->estimate_frame_flag) {
		double previous;
		previous = current - context->estimate_frame_last;
		context->estimate_frame = estimate_merge(context->estimate_frame, previous);
	}

	context->estimate_frame_flag = 1;
	context->estimate_frame_last = current;
}




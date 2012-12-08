/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

/** Max frameskip factor */
#define SYNC_MAX 4

/**
 * Update the skip state.
 * Recompute the skip counters from the config.frameskip_factor variable.
 */
void advance_video_update_skip(struct advance_video_context* context)
{
	context->state.skip_warming_up_flag = 1;

	if (context->config.frameskip_factor >= 1.0) {
		context->state.skip_level_full = SYNC_MAX;
		context->state.skip_level_skip = 0;
	} else if (context->config.frameskip_factor >= 1.0-1.0/SYNC_MAX) {
		context->state.skip_level_full = SYNC_MAX -1;
		context->state.skip_level_skip = 1;
	} else if (context->config.frameskip_factor <= 1.0/SYNC_MAX) {
		context->state.skip_level_full = 1;
		context->state.skip_level_skip = SYNC_MAX -1;
	} else if (context->config.frameskip_factor >= 0.5) {
		context->state.skip_level_full = context->config.frameskip_factor / (1-context->config.frameskip_factor);
		if (context->state.skip_level_full < 1)
			context->state.skip_level_full = 1;
		if (context->state.skip_level_full >= SYNC_MAX)
			context->state.skip_level_full = SYNC_MAX;
		context->state.skip_level_skip = 1;
	} else {
		context->state.skip_level_full = 1;
		context->state.skip_level_skip = (1-context->config.frameskip_factor) / context->config.frameskip_factor;
		if (context->state.skip_level_skip < 1)
			context->state.skip_level_skip = 1;
		if (context->state.skip_level_skip >= SYNC_MAX)
			context->state.skip_level_skip = SYNC_MAX;
	}
}

/**
 * Decremnt by one the frame skip state.
 */
adv_bool advance_video_skip_dec(struct advance_video_context* context)
{
	if (context->state.skip_level_full >= SYNC_MAX) {
		context->state.skip_level_full = SYNC_MAX - 1;
		context->state.skip_level_skip = 1;
	} else if (context->state.skip_level_full > 1) {
		--context->state.skip_level_full;
		context->state.skip_level_skip = 1;
	} else if (context->state.skip_level_skip < SYNC_MAX - 1) {
		context->state.skip_level_full = 1;
		++context->state.skip_level_skip;
	} else {
		return 1;
	}

	context->state.skip_level_counter = 0;

	return 0;
}

/**
 * Increment by one the frame skip state.
 */
adv_bool advance_video_skip_inc(struct advance_video_context* context)
{
	if (context->state.skip_level_skip > 1) {
		context->state.skip_level_full = 1;
		--context->state.skip_level_skip;
	} else if (context->state.skip_level_full < SYNC_MAX - 1) {
		++context->state.skip_level_full;
		context->state.skip_level_skip = 1;
	} else if (context->state.skip_level_full == SYNC_MAX - 1) {
		context->state.skip_level_full = SYNC_MAX;
		context->state.skip_level_skip = 0;
	} else {
		return 1;
	}

	context->state.skip_level_counter = 0;

	return 0;
}

/**
 * Update the sync state.
 * Enables or disables the video vertical sync from the game framerate
 * and the current speed state.
 */
void advance_video_update_sync(struct advance_video_context* context)
{
	double rate = video_measured_vclock();
	double reference = context->state.game_fps;

	context->state.sync_warming_up_flag = 1;

	if (rate > 10 && rate < 300) {
		context->state.mode_vclock = video_rate_scale_down(rate, reference);
		context->state.vsync_flag = context->config.vsync_flag;
	} else {
		/* out of range, surely NOT supported */
		context->state.mode_vclock = reference;
		context->state.vsync_flag = 0;
	}

	if (context->state.vsync_flag) {
		/* disable if no throttling */
		if (!context->state.sync_throttle_flag) {
			log_std(("emu:video: vsync disabled because throttle disactive\n"));
			context->state.vsync_flag = 0;
		}

		/* disable if turbo active */
		if (context->state.turbo_flag) {
			log_std(("emu:video: vsync disabled because turbo active\n"));
			context->state.vsync_flag = 0;
		}

		/* disable if fastest active */
		if (context->state.fastest_flag) {
			log_std(("emu:video: vsync disabled because fastest active\n"));
			context->state.vsync_flag = 0;
		}

		/* disable if measure active */
		if (context->state.measure_flag) {
			log_std(("emu:video: vsync disabled because measure active\n"));
			context->state.vsync_flag = 0;
		}

		/* disable the vsync flag if the frame rate is wrong */
		if (context->state.mode_vclock < reference * 0.97
			|| context->state.mode_vclock > reference * 1.03) {
			log_std(("emu:video: vsync disabled because the vclock is too different %g %g\n", reference, context->state.mode_vclock));
			context->state.vsync_flag = 0;
		}
	}
}

static void video_time(struct advance_video_context* context, struct advance_estimate_context* estimate_context, double* full, double* skip)
{
	if (context->config.smp_flag) {
		/* if SMP is active the time estimation take care of it, */
		/* the times are not added, but the max value is used */
		*full = estimate_context->estimate_mame_full;
		if (*full < estimate_context->estimate_osd_full)
			*full = estimate_context->estimate_osd_full;
		*skip = estimate_context->estimate_mame_skip;
		if (*skip < estimate_context->estimate_osd_skip)
			*skip = estimate_context->estimate_osd_skip;
	} else {
		/* standard time estimation */
		*full = estimate_context->estimate_mame_full + estimate_context->estimate_osd_full;
		*skip = estimate_context->estimate_mame_skip + estimate_context->estimate_osd_skip;
	}

	/* common time */
	*full += estimate_context->estimate_common_full;
	*skip += estimate_context->estimate_common_skip;

	/* correct errors, it may happen that skip is not measured and it contains an old value */
	if (*full < *skip)
		*skip = *full;
}

/* Define to optimize for full CPU usage (reduce the wait time) instead of full speed */
/* #define USE_FULLCPU */

static void video_skip_recompute(struct advance_video_context* context, struct advance_estimate_context* estimate_context)
{
	/* frame time */
	double step = context->state.skip_step;

	/* time required to compute and draw a complete frame */
	double full;

	/* time required to compute a frame without displaying it */
	double skip;

	video_time(context, estimate_context, &full, &skip);

	log_debug(("advance:skip: step %g [sec]\n", step));
	log_debug(("advance:skip: frame full %g [sec], frame skip %g [sec]\n", estimate_context->estimate_mame_full + estimate_context->estimate_osd_full, estimate_context->estimate_mame_skip + estimate_context->estimate_osd_skip));
	log_debug(("advance:skip: mame_full %g [sec], mame_skip %g [sec], osd_full %g [sec], osd_skip %g [sec]\n", estimate_context->estimate_mame_full, estimate_context->estimate_mame_skip, estimate_context->estimate_osd_full, estimate_context->estimate_osd_skip));
	log_debug(("advance:skip: common_full %g [sec], common_skip %g [sec]\n", estimate_context->estimate_common_full, estimate_context->estimate_common_skip));
	log_debug(("advance:skip: full %g [sec], skip %g [sec]\n", full, skip));

	context->state.skip_level_disable_flag = 0;

	assert(skip <= full);

	if (full < step) {
		/* full frame rate */
		context->state.skip_level_full = SYNC_MAX;
		context->state.skip_level_skip = 0;
	} else if (skip >= step /* (this check is implicit on the next one) */
		|| skip * SYNC_MAX + full >= step * (SYNC_MAX+1)
	) {
		/* if the maximum skip plus one isn't enought use a special management */
		/* (the plus one is to avoid to continously activate and deactivate skipping) */
		if (context->state.turbo_flag || context->state.fastest_flag) {
			/* null frame rate */
			context->state.skip_level_full = 1;
			context->state.skip_level_skip = SYNC_MAX - 1;
		} else {
			/* mid frame rate, there isn't reason to skip, the correct speed is impossible */
			/* the full frame rate isn't used to continue measuring the skip time */
			context->state.skip_level_full = 1;
			context->state.skip_level_skip = 1;
			context->state.skip_level_disable_flag = 1; /* signal the special condition */
		}
	} else {
		double full_lose_time = full - step; /* time lost drawing a full frame */
		double skip_gain_time = step - skip; /* time recovered drawing a skip frame */

		assert(full_lose_time > 0);
		assert(skip_gain_time > 0);

		if (skip_gain_time >= full_lose_time) {
			/* compute the number of full frame witch can be draw */
			/* for any skip frame */
			double v = skip_gain_time / full_lose_time;

			/* limit value before converting to int to prevent overflow */
			if (v > SYNC_MAX)
				v = SYNC_MAX;

#ifdef USE_FULLCPU
			/* The use of ceil() instead of floor() generates a frame rate lower than 100% */
			/* but it ensures to use all the CPU time */
			context->state.skip_level_full = ceil(v);
#else
			context->state.skip_level_full = floor(v);
#endif
			context->state.skip_level_skip = 1;

			/* max skip limit */
			if (context->state.skip_level_full >= SYNC_MAX)
				context->state.skip_level_full = SYNC_MAX - 1;
		} else {
			/* compute the number of skip frames required to */
			/* recover the extra time of a full frame */
			double v = full_lose_time / skip_gain_time;

			/* limit value before converting to int to prevent overflow */
			if (v > SYNC_MAX)
				v = SYNC_MAX;

			context->state.skip_level_full = 1;
#ifdef USE_FULLCPU
			/* The use of floor() instead of ceil() generates a frame rate lower than 100% */
			/* but it ensures to use all the CPU time */
			context->state.skip_level_skip = floor(v);
#else
			context->state.skip_level_skip = ceil(v);
#endif

			/* max skip limit */
			if (context->state.skip_level_skip >= SYNC_MAX)
				context->state.skip_level_skip = SYNC_MAX - 1;
		}
	}

	log_debug(("advance:skip: cycle %d/%d\n", context->state.skip_level_full, context->state.skip_level_skip));
}

static double video_frame_wait(double current, double expected)
{
	while (current < expected) {
		double diff = expected - current;

		target_usleep(diff * 1E6);

		current = advance_timer();
	}

	return current;
}

static void video_frame_sync(struct advance_video_context* context)
{
	double current;
	double expected;

	current = advance_timer();

	if (context->state.sync_warming_up_flag) {
		/* syncronize the first time */
		if (context->state.vsync_flag) {
			video_wait_vsync();
			current = advance_timer();
		}

		context->state.sync_pivot = 0;
		context->state.sync_last = current;
		context->state.sync_skip_counter = 0;

		context->state.sync_warming_up_flag = 0;

		log_debug(("advance:sync: throttle warming up\n"));
	} else {
		double time_before_sync, time_after_delay, time_after_sync;

		time_before_sync = current;

		expected = context->state.sync_last + context->state.skip_step * (1 + context->state.sync_skip_counter);
		context->state.sync_skip_counter = 0;

		/* take only a part of the error, this increase the stability */
		context->state.sync_pivot *= 0.99;

		/* adjust with the previous error */
		expected += context->state.sync_pivot;

		/* the vsync is used only if all the frames are displayed */
		if ((video_flags() & MODE_FLAGS_RETRACE_WAIT_SYNC) != 0
			&& context->state.vsync_flag
			&& context->state.skip_level_full == SYNC_MAX
		) {
			/* wait until the retrace is near (3% early), otherwise if the */
			/* mode has a double freq the retrace may be the wrong one. */
			double early = 0.03 / video_measured_vclock();

			if (current < expected - early) {
				current = video_frame_wait(current, expected - early);
				time_after_delay = current;

				if (current < expected) {
					double after;
					video_wait_vsync();
					after = advance_timer();
					
					if (after - current > 1.05 / (double)video_measured_vclock()) {
						log_std(("ERROR:emu:video: sync wait too long. %g instead of %g (max %g)\n", after - current, early, 1.0 / (double)video_measured_vclock()));
					} else if (after - current > 1.05 * early) {
						log_std(("WARNING:emu:video: sync wait too long. %g instead of %g (max %g)\n", after - current, early, 1.0 / (double)video_measured_vclock()));
					}
					
					/* if a sync complete correctly reset the error to 0 */
					/* in this case the vsync is used for the correct clocking */
					expected = after;
					
					current = after;
				} else {
					log_std(("ERROR:emu:video: sync delay too big\n"));
				}
			} else {
				log_std(("ERROR:emu:video: too late for a video sync\n"));
				current = video_frame_wait(current, expected);
				time_after_delay = current;
			}
		} else {
			current = video_frame_wait(current, expected);
			time_after_delay = current;
		}

		time_after_sync = current;

		/* update the error state */
		context->state.sync_pivot = expected - current;

		if (fabs(context->state.sync_pivot) > context->state.skip_step / 50)
			log_std(("advance:sync: %.5f (err %6.1f%%) = %.5f + %.5f + %.5f < %.5f (compute + sleep + sync < max)\n", current - context->state.sync_last, context->state.sync_pivot * 100 / context->state.skip_step, time_before_sync - context->state.sync_last, time_after_delay - time_before_sync, time_after_sync - time_after_delay, context->state.skip_step));

		if (context->state.sync_pivot < - context->state.skip_step * 16) {
			/* if the error is too big (negative) the delay is unrecoverable */
			/* generally it happen with a virtual terminal switch */
			/* the best solution is to restart the sync computation */
			advance_video_update_skip(context);
			advance_video_update_sync(context);
		}

		context->state.sync_last = current;
	}
}

static void video_frame_sync_free(struct advance_video_context* context)
{
	double current;

	current = advance_timer();

	if (context->state.sync_warming_up_flag) {
		context->state.sync_pivot = 0;
		context->state.sync_last = current;
		context->state.sync_skip_counter = 0;

		context->state.sync_warming_up_flag = 0;

		log_debug(("advance:sync: free warming up\n"));
	} else {
		context->state.sync_last = current;

		log_debug(("advance:sync: free\n"));
	}
}

void advance_video_sync(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, adv_bool skip_flag)
{
	if (!skip_flag) {
		double delay;

		if (!context->state.fastest_flag
			&& !context->state.measure_flag
			&& context->state.sync_throttle_flag)
			video_frame_sync(context);
		else
			video_frame_sync_free(context);

		/* this is the time of a single frame, the frame time measure */
		/* is not used because it is not constant */

		delay = context->state.skip_step;

		context->state.latency_diff = advance_sound_latency_diff(sound_context, delay);
	} else {
		++context->state.sync_skip_counter;
	}
}

static void video_frame_skip(struct advance_video_context* context, struct advance_estimate_context* estimate_context)
{
	if (context->state.skip_warming_up_flag) {
		context->state.skip_flag = 0;
		context->state.skip_level_counter = 0;

		if (context->state.measure_flag) {
			context->state.skip_step = 1.0 / context->state.game_fps;
			context->state.skip_level_full = 1;
			context->state.skip_level_skip = 0;
		} else if (context->state.fastest_flag) {
			context->state.skip_step = 1.0 / context->state.game_fps;
			context->state.skip_level_full = 1;
			context->state.skip_level_skip = SYNC_MAX;
		} else if (context->state.turbo_flag) {
			context->state.skip_step = 1.0 / (context->state.game_fps * context->config.turbo_speed_factor);
		} else if (context->state.vsync_flag) {
			context->state.skip_step = 1.0 / context->state.mode_vclock;
		} else {
			context->state.skip_step = 1.0 / context->state.game_fps;
		}

		advance_estimate_init(estimate_context, context->state.skip_step);

		context->state.skip_warming_up_flag = 0;

		log_debug(("advance:skip: throttle warming up\n"));
	} else {
		/* compute if the next (not the current one) frame must be skipped */
		context->state.skip_flag = context->state.skip_level_counter >= context->state.skip_level_full;

		++context->state.skip_level_counter;
		if (context->state.skip_level_counter >= context->state.skip_level_full + context->state.skip_level_skip) {
			/* restart the full/skip sequence */
			context->state.skip_level_counter = 0;

			/* recompute the frameskip */
			if (!context->state.fastest_flag
				&& !context->state.measure_flag
				&& (context->state.turbo_flag || context->config.frameskip_auto_flag)) {
				video_skip_recompute(context, estimate_context);
			}
		}

		log_debug(("advance:skip: skip %d, frame %d/%d/%d\n", context->state.skip_flag, context->state.skip_level_counter, context->state.skip_level_full, context->state.skip_level_skip));
	}
}

static void video_frame_skip_none(struct advance_video_context* context, struct advance_estimate_context* estimate_context)
{
	context->state.skip_step = 1.0 / context->state.game_fps;
	context->state.skip_flag = 0;
	context->state.skip_level_counter = 0;

	/* force a recomputation when needed */
	context->state.skip_warming_up_flag = 1;
}

void advance_video_skip(struct advance_video_context* context, struct advance_estimate_context* estimate_context, struct advance_record_context* record_context)
{
	if (!context->state.sync_throttle_flag
		|| advance_record_video_is_active(record_context)
		|| advance_record_snapshot_is_active(record_context)
	) {
		video_frame_skip_none(context, estimate_context);
	} else {
		video_frame_skip(context, estimate_context);
	}
}


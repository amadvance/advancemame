/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2008 Andrea Mazzoleni
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
#include "hscript.h"
#include "script.h"
#include "thread.h"

#include "advance.h"

#include <math.h>
#include <limits.h>

#ifdef USE_SMP
#include <pthread.h>
#endif

/***************************************************************************/
/* Commands */

static adv_bool video_is_normal_speed(struct advance_video_context* context)
{
	adv_bool normal_speed;

	normal_speed = !context->state.turbo_flag
		&& !context->state.fastest_flag
		&& !context->state.measure_flag
		&& context->state.sync_throttle_flag;

	return normal_speed;
}

static void event_check(unsigned ordinal, adv_bool condition, int id, unsigned previous, unsigned* current)
{
	unsigned mask = 1 << ordinal;

	if (condition)
		*current |= mask;

	if ((previous & mask) != (*current & mask)) {
		if (condition) {
			hardware_script_start(id);
		} else {
			hardware_script_stop(id);
		}
	}
}

static void video_command_event(struct advance_video_context* context, struct advance_safequit_context* safequit_context, int leds_status, adv_bool turbo_status, unsigned input)
{
	unsigned event_mask = 0;

	event_check(0, (leds_status & 0x1) != 0, HARDWARE_SCRIPT_LED1, context->state.event_mask_old, &event_mask);
	event_check(1, (leds_status & 0x2) != 0, HARDWARE_SCRIPT_LED2, context->state.event_mask_old, &event_mask);
	event_check(2, (leds_status & 0x4) != 0, HARDWARE_SCRIPT_LED3, context->state.event_mask_old, &event_mask);
	event_check(3, (input & OSD_INPUT_COIN1) != 0, HARDWARE_SCRIPT_COIN1, context->state.event_mask_old, &event_mask);
	event_check(4, (input & OSD_INPUT_COIN2) != 0, HARDWARE_SCRIPT_COIN2, context->state.event_mask_old, &event_mask);
	event_check(5, (input & OSD_INPUT_COIN3) != 0, HARDWARE_SCRIPT_COIN3, context->state.event_mask_old, &event_mask);
	event_check(6, (input & OSD_INPUT_COIN4) != 0, HARDWARE_SCRIPT_COIN4, context->state.event_mask_old, &event_mask);
	event_check(7, (input & OSD_INPUT_START1) != 0, HARDWARE_SCRIPT_START1, context->state.event_mask_old, &event_mask);
	event_check(8, (input & OSD_INPUT_START2) != 0, HARDWARE_SCRIPT_START2, context->state.event_mask_old, &event_mask);
	event_check(9, (input & OSD_INPUT_START3) != 0, HARDWARE_SCRIPT_START3, context->state.event_mask_old, &event_mask);
	event_check(10, (input & OSD_INPUT_START4) != 0, HARDWARE_SCRIPT_START4, context->state.event_mask_old, &event_mask);
	event_check(11, turbo_status, HARDWARE_SCRIPT_TURBO, context->state.event_mask_old, &event_mask);
	event_check(12, advance_safequit_can_exit(safequit_context), HARDWARE_SCRIPT_SAFEQUIT, context->state.event_mask_old, &event_mask);
	event_check(13, advance_safequit_event_mask(safequit_context) & 0x4, HARDWARE_SCRIPT_EVENT1, context->state.event_mask_old, &event_mask);
	event_check(14, advance_safequit_event_mask(safequit_context) & 0x8, HARDWARE_SCRIPT_EVENT2, context->state.event_mask_old, &event_mask);
	event_check(15, advance_safequit_event_mask(safequit_context) & 0x10, HARDWARE_SCRIPT_EVENT3, context->state.event_mask_old, &event_mask);
	event_check(16, advance_safequit_event_mask(safequit_context) & 0x20, HARDWARE_SCRIPT_EVENT4, context->state.event_mask_old, &event_mask);
	event_check(17, advance_safequit_event_mask(safequit_context) & 0x40, HARDWARE_SCRIPT_EVENT5, context->state.event_mask_old, &event_mask);
	event_check(18, advance_safequit_event_mask(safequit_context) & 0x80, HARDWARE_SCRIPT_EVENT6, context->state.event_mask_old, &event_mask);
	event_check(19, advance_safequit_event_mask(safequit_context) & 0x100, HARDWARE_SCRIPT_EVENT7, context->state.event_mask_old, &event_mask);
	event_check(20, advance_safequit_event_mask(safequit_context) & 0x200, HARDWARE_SCRIPT_EVENT8, context->state.event_mask_old, &event_mask);
	event_check(21, advance_safequit_event_mask(safequit_context) & 0x400, HARDWARE_SCRIPT_EVENT9, context->state.event_mask_old, &event_mask);
	event_check(22, advance_safequit_event_mask(safequit_context) & 0x800, HARDWARE_SCRIPT_EVENT10, context->state.event_mask_old, &event_mask);
	event_check(23, advance_safequit_event_mask(safequit_context) & 0x1000, HARDWARE_SCRIPT_EVENT11, context->state.event_mask_old, &event_mask);
	event_check(24, advance_safequit_event_mask(safequit_context) & 0x2000, HARDWARE_SCRIPT_EVENT12, context->state.event_mask_old, &event_mask);
	event_check(25, advance_safequit_event_mask(safequit_context) & 0x4000, HARDWARE_SCRIPT_EVENT13, context->state.event_mask_old, &event_mask);
	event_check(26, advance_safequit_event_mask(safequit_context) & 0x8000, HARDWARE_SCRIPT_EVENT14, context->state.event_mask_old, &event_mask);

	/* save the new status */
	context->state.event_mask_old = event_mask;
}

static void video_command_resolution(struct advance_video_context* context, unsigned input)
{
	adv_bool modify = 0;
	adv_bool show = 0;
	char new_resolution[MODE_NAME_MAX];

	sncpy(new_resolution, MODE_NAME_MAX, context->config.resolution_buffer);

	if ((input & OSD_INPUT_MODE_NEXT) != 0) {
		show = 1;
		if (strcmp(new_resolution, "auto")==0) {
			if (context->state.crtc_mac > 1) {
				sncpy(new_resolution, MODE_NAME_MAX, crtc_name_get(context->state.crtc_map[1]));
				modify = 1;
			}
		} else {
			unsigned i;
			for(i=0;i<context->state.crtc_mac;++i)
				if (context->state.crtc_map[i] == context->state.crtc_selected)
					break;
			if (i<context->state.crtc_mac && i+1<context->state.crtc_mac) {
				sncpy(new_resolution, MODE_NAME_MAX, crtc_name_get(context->state.crtc_map[i+1]));
				modify = 1;
			}
		}
	} else if ((input & OSD_INPUT_MODE_PRED) != 0) {
		show = 1;
		if (strcmp(new_resolution, "auto")!=0) {
			unsigned i;
			for(i=0;i<context->state.crtc_mac;++i)
				if (context->state.crtc_map[i] == context->state.crtc_selected)
					break;
			if (i<context->state.crtc_mac && i>0) {
				sncpy(new_resolution, MODE_NAME_MAX, crtc_name_get(context->state.crtc_map[i-1]));
				modify = 1;
			}
		}
	}

	if (modify) {
		struct advance_video_config_context config = context->config;
		sncpy(config.resolution_buffer, sizeof(config.resolution_buffer), new_resolution);
		advance_video_reconfigure(context, &config);
	}

	if (show) {
		advance_global_message(&CONTEXT.global, "%s", context->config.resolution_buffer);
	}
}

static void video_command_pan(struct advance_video_context* context, unsigned input)
{
	adv_bool modify = 0;

	if ((input & OSD_INPUT_PAN_RIGHT) != 0) {
		if (context->state.game_visible_pos_x + context->state.game_visible_size_x + context->state.game_visible_pos_x_increment <= context->state.game_used_size_x) {
			context->state.game_visible_pos_x += context->state.game_visible_pos_x_increment;
			modify = 1;
		}
	}

	if ((input & OSD_INPUT_PAN_LEFT) != 0) {
		if (context->state.game_visible_pos_x >= context->state.game_visible_pos_x_increment) {
			context->state.game_visible_pos_x -= context->state.game_visible_pos_x_increment;
			modify = 1;
		}
	}

	if ((input & OSD_INPUT_PAN_UP) != 0) {
		if (context->state.game_visible_pos_y + context->state.game_visible_size_y < context->state.game_used_pos_y + context->state.game_used_size_y) {
			context->state.game_visible_pos_y++;
			modify = 1;
		}
	}

	if ((input & OSD_INPUT_PAN_DOWN) != 0) {
		if (context->state.game_visible_pos_y > context->state.game_used_pos_y) {
			context->state.game_visible_pos_y--;
			modify = 1;
		}
	}

	if (modify) {
		context->config.skipcolumns = context->state.game_visible_pos_x;
		context->config.skiplines = context->state.game_visible_pos_y;
		advance_video_update_pan(context);
	}
}

static void video_command_combine(struct advance_video_context* context, struct advance_ui_context* ui_context, adv_bool skip_flag)
{
	if (advance_ui_buffer_active(ui_context) /* if no ui with buffer is active */
		|| !video_is_normal_speed(context) /* if we are in normal runtime condition */
	) {
		/* reset the counter */
		context->state.skip_level_combine_counter = 0;

		/* don't adjust */
		return;
	}

	if (context->state.skip_level_skip != 0) { /* if we are not 100% full speed */
		/* one more frame too slow */
		++context->state.skip_level_combine_counter;

		/* if we reached some kind of limit */
		if (context->state.skip_level_combine_counter > 60) {
			struct advance_video_config_context config = context->config;

			/* try decreasing the video effects */
			if (config.combine == COMBINE_AUTO) {
				switch (config.combine_max) {
				case COMBINE_SCALEX :
					config.combine_max = COMBINE_NONE;
					log_std(("advance:skip: decreasing combine from scalex to none\n"));
					break;
				case COMBINE_SCALEK :
					config.combine_max = COMBINE_SCALEX;
					log_std(("advance:skip: decreasing combine from scalek to scalex\n"));
					break;
				case COMBINE_XBR :
					config.combine_max = COMBINE_SCALEK;
					log_std(("advance:skip: decreasing combine from xbr to scalek\n"));
					break;
				}
			}

			/* if something changed */
			if (context->config.combine_max != config.combine_max) {
				/* reconfigure */
				advance_video_reconfigure(context, &config);

				/* restart counting */
				context->state.skip_level_combine_counter = 0;
			}
		}
	}
}

static void video_command(struct advance_video_context* context, struct advance_estimate_context* estimate_context, struct advance_safequit_context* safequit_context, struct advance_ui_context* ui_context, adv_conf* cfg_context, int leds_status, unsigned input, adv_bool skip_flag)
{
	/* increment the number of frames */
	++context->state.frame_counter;

	/* events */
	video_command_event(context, safequit_context, leds_status, context->state.turbo_flag, input);

	/* scripts */
	hardware_script_idle(SCRIPT_TIME_UNIT / context->state.game_fps);
	hardware_simulate_input_idle(SIMULATE_EVENT, SCRIPT_TIME_UNIT / context->state.game_fps);
	hardware_simulate_input_idle(SIMULATE_KEY, SCRIPT_TIME_UNIT / context->state.game_fps);

	if (context->state.measure_flag) {
		if (context->state.measure_counter > 0) {
			--context->state.measure_counter;
			if (context->state.measure_counter == 0) {
				context->state.measure_stop = target_clock();

				/* force the exit at the next frame */
				CONTEXT.input.state.input_forced_exit_flag = 1;
			}
		}

		/* don't change anything */
		return;
	}

	if ((input & OSD_INPUT_THROTTLE) != 0) {
		context->state.sync_throttle_flag = !context->state.sync_throttle_flag;

		advance_video_update_skip(context);
		advance_video_update_sync(context);

		if (!context->state.info_flag) {
			context->state.info_flag = 1;
			context->state.info_counter = 2 * context->state.game_fps;
		}
	}

	if ((input & OSD_INPUT_FRAMESKIP_INC) != 0) {
		if (context->config.frameskip_auto_flag) {
			context->config.frameskip_auto_flag = 0;
			advance_video_update_skip(context);
		} else {
			if (advance_video_skip_inc(context))
				context->config.frameskip_auto_flag = 1;
		}

		if (!context->state.info_flag) {
			context->state.info_flag = 1;
			context->state.info_counter = 2 * context->state.game_fps;
		}
	}

	if ((input & OSD_INPUT_FRAMESKIP_DEC) != 0) {
		if (context->config.frameskip_auto_flag) {
			context->config.frameskip_auto_flag = 0;
			advance_video_update_skip(context);
		} else {
			if (advance_video_skip_dec(context))
				context->config.frameskip_auto_flag = 1;
		}

		if (!context->state.info_flag) {
			context->state.info_flag = 1;
			context->state.info_counter = 2 * context->state.game_fps;
		}
	}

	if ((input & OSD_INPUT_SHOW_FPS) != 0) {
		context->state.info_flag = !context->state.info_flag;
		context->state.info_counter = 0;

		if (!context->state.info_flag)
			mame_ui_refresh();
	}

	if (context->state.turbo_flag) {
		if ((input & OSD_INPUT_TURBO) == 0) {
			context->state.turbo_flag = 0;
			advance_video_update_skip(context);
			advance_video_update_sync(context);
		}
	} else {
		if ((input & OSD_INPUT_TURBO) != 0) {
			context->state.turbo_flag = 1;
			advance_video_update_skip(context);
			advance_video_update_sync(context);
		}
	}

	if (context->state.fastest_flag) {
		if (context->state.frame_counter > context->state.fastest_limit) {
			context->state.fastest_flag = 0;

			advance_video_update_skip(context);
			advance_video_update_sync(context);

			hardware_script_start(HARDWARE_SCRIPT_PLAY);
		}
	}
	
	if ((input & OSD_INPUT_TOGGLE_DEBUG) != 0) {
		context->state.debugger_flag = !context->state.debugger_flag;
		advance_video_invalidate_screen(context);
	}

	if ((input & OSD_INPUT_COCKTAIL) != 0) {
		context->config.blit_orientation ^= OSD_ORIENTATION_FLIP_Y | OSD_ORIENTATION_FLIP_X;
		context->config.user_orientation ^= OSD_ORIENTATION_FLIP_Y | OSD_ORIENTATION_FLIP_X;

		advance_video_invalidate_pipeline(context);
	}

	if ((input & OSD_INPUT_HELP) != 0) {
		advance_ui_help(ui_context);
	}

	if ((input & OSD_INPUT_STARTUP_END) != 0) {
		double time = floor(context->state.frame_counter / context->state.game_fps);

		if (time >= 2 && time <= 180) {
			char buffer[32];
			int time_int = time;
			context->config.fastest_time = time_int;
			snprintf(buffer, sizeof(buffer), "%d", time_int);
			conf_string_set(cfg_context, context->config.section_name_buffer, "sync_startuptime", buffer);
			advance_global_message(&CONTEXT.global, "Startup time of %d seconds saved", time_int);
		} else if (time > 180) {
			advance_global_message(&CONTEXT.global, "Startup time TOO LONG");
		} else if (time < 2) {
			context->config.fastest_time = 0;
			conf_remove(cfg_context, context->config.section_name_buffer, "sync_startuptime");
			advance_global_message(&CONTEXT.global, "Startup time cleared");
		}
	}

	advance_ui_direct_fast(ui_context, context->state.fastest_flag || context->state.turbo_flag);

	advance_ui_direct_slow(ui_context, context->state.skip_level_disable_flag);

	if (context->state.info_flag) {
		char buffer[256];
		unsigned skip;
		unsigned rate;
		unsigned l;

		if (context->state.info_counter) {
			--context->state.info_counter;
			if (!context->state.info_counter) {
				context->state.info_flag = 0;
				mame_ui_refresh();
			}
		}

		rate = floor(100.0 / (estimate_context->estimate_frame * context->state.game_fps / context->config.fps_speed_factor) + 0.5);
		skip = 100 * context->state.skip_level_full / (context->state.skip_level_full + context->state.skip_level_skip);

		if (context->state.fastest_flag) {
			snprintf(buffer, sizeof(buffer), " %s %3d%% - %3d%%", "startup", skip, rate);
		} else if (!context->state.sync_throttle_flag) {
			snprintf(buffer, sizeof(buffer), " %s 100%% - %3d%%", "free", rate);
		} else if (context->state.turbo_flag) {
			snprintf(buffer, sizeof(buffer), " %s %3d%% - %3d%%", "turbo", skip, rate);
		} else if (context->state.skip_level_disable_flag) {
			snprintf(buffer, sizeof(buffer), " %s %3d%% - %3d%%", "slow", skip, rate);
		} else if (context->config.frameskip_auto_flag) {
			snprintf(buffer, sizeof(buffer), " %s %3d%% - %3d%%", "auto", skip, rate);
		} else {
			snprintf(buffer, sizeof(buffer), " %s %3d%% - %3d%%", "fix", skip, rate);
		}

		/* use the fixed space char */
		/* 100% - 100% */
		/* 10987654321 */
		l = strlen(buffer);
		if (l>=4 && isspace(buffer[l-4]))
			buffer[l-4] = ADV_FONT_FIXSPACE;
		if (l>=11 && isspace(buffer[l-11]))
			buffer[l-11] = ADV_FONT_FIXSPACE;

		advance_ui_direct_text(ui_context, buffer);

		hardware_script_info(0, 0, 0, buffer);
	} else {
		char buffer[256];
		unsigned rate;

		rate = floor(100.0 / (estimate_context->estimate_frame * context->state.game_fps / context->config.fps_speed_factor) + 0.5);

		snprintf(buffer, sizeof(buffer), "%3d%%", rate);

		hardware_script_info(0, 0, 0, buffer);
	}

	video_command_resolution(context, input);

	video_command_pan(context, input);

	video_command_combine(context, ui_context, skip_flag);
}

/***************************************************************************/
/* Frame */

static void video_frame_update_now(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, struct advance_record_context* record_context, struct advance_ui_context* ui_context, struct advance_safequit_context* safequit_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool skip_flag)
{
	/* Do a yield immediatly before the time syncronization. */
	/* If a schedule will be done, it's better to have it now when */
	/* we need to wait the syncronization point. Obviously it may happen */
	/* to lose the precise time, but it will happen anyway if the system is busy. */
	target_yield();

	/* the frame syncronization is out of the time estimation */
	advance_video_sync(context, sound_context, estimate_context, skip_flag);

	/* estimate the time */
	advance_estimate_osd_begin(estimate_context);

	/* update the video for the new frame */
	advance_video_frame(context, record_context, ui_context, game, debug, debug_palette, debug_palette_size, skip_flag);

	/* update the audio buffer for the new frame */
	advance_sound_frame(sound_context, record_context, context, safequit_context, sample_buffer, sample_count, sample_recount, context->config.rawsound_flag || video_is_normal_speed(context));

	/* estimate the time */
	advance_estimate_osd_end(estimate_context, skip_flag);
}

/**
 * Wait the completion of the video thread.
 */
void advance_video_thread_wait(struct advance_video_context* context)
{
#ifdef USE_SMP
	/* wait until the thread is ready */
	pthread_mutex_lock(&context->state.thread_video_mutex);

	/* wait for the stop notification  */
	while (context->state.thread_state_ready_flag) {
		pthread_cond_wait(&context->state.thread_video_cond, &context->state.thread_video_mutex);
	}

	pthread_mutex_unlock(&context->state.thread_video_mutex);
#else
	/* nothing */
#endif
}

#ifdef USE_SMP
/**
 * Duplicate a bitmap.
 * The old buffer is reused if possible.
 */
static struct osd_bitmap* video_thread_bitmap_duplicate(struct osd_bitmap* old, const struct osd_bitmap* current)
{
	if (current) {
		if (old) {
			if (old->size_y != current->size_y
				|| old->bytes_per_scanline != current->bytes_per_scanline) {
				free(old->ptr);
				old->ptr = 0;
			}
		}

		if (!old) {
			old = malloc(sizeof(struct osd_bitmap));
			old->ptr = 0;
		}

		if (!old->ptr)
			old->ptr = malloc(current->size_y * current->bytes_per_scanline);

		memcpy(old->ptr, current->ptr, current->size_y * current->bytes_per_scanline);
		old->size_x = current->size_x;
		old->size_y = current->size_y;
		old->bytes_per_scanline = current->bytes_per_scanline;
	} else {
		if (old) {
			free(old->ptr);
			free(old);
			old = 0;
		}
	}

	return old;
}

/**
 * Free a bitmap.
 */
static void video_thread_bitmap_free(struct osd_bitmap* bitmap)
{
	if (bitmap) {
		free(bitmap->ptr);
		free(bitmap);
	}
}

#endif

/**
 * Precomputation of the frame before updating.
 * Mainly used to duplicate the data for the video thread.
 */
static void video_frame_prepare(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool skip_flag)
{
#ifdef USE_SMP
	/* don't use the thread if the debugger is active */
	if (context->config.smp_flag && !context->state.debugger_flag) {

		/* duplicate the data */
		pthread_mutex_lock(&context->state.thread_video_mutex);

		/* wait for the stop notification  */
		while (context->state.thread_state_ready_flag) {
			pthread_cond_wait(&context->state.thread_video_cond, &context->state.thread_video_mutex);
		}

		advance_estimate_common_begin(estimate_context);

		if (!skip_flag) {
			context->state.thread_state_game = video_thread_bitmap_duplicate(context->state.thread_state_game, game);
		}

		context->state.thread_state_led = led;
		context->state.thread_state_input = input;
		context->state.thread_state_skip_flag = skip_flag;

		if (sample_count > context->state.thread_state_sample_max) {
			log_std(("advance:thread: realloc sample buffer %d samples -> %d samples, %d bytes\n", context->state.thread_state_sample_max, 2*sample_count, sound_context->state.input_bytes_per_sample * 2 * sample_count));
			context->state.thread_state_sample_max = 2 * sample_count;
			context->state.thread_state_sample_buffer = realloc(context->state.thread_state_sample_buffer, sound_context->state.input_bytes_per_sample * context->state.thread_state_sample_max);
			assert(context->state.thread_state_sample_buffer);
		}

		memcpy(context->state.thread_state_sample_buffer, sample_buffer, sample_count * sound_context->state.input_bytes_per_sample);
		context->state.thread_state_sample_count = sample_count;
		context->state.thread_state_sample_recount = sample_recount;

		advance_estimate_common_end(estimate_context, skip_flag);

		pthread_mutex_unlock(&context->state.thread_video_mutex);
	}
#endif
}

/**
 * Update the frame.
 * If SMP is active only signals at the thread to start the
 * update and returns immeditely. Otherwise it returns only then the
 * frame is complete.
 */
static void video_frame_update(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, struct advance_record_context* record_context, struct advance_ui_context* ui_context, struct advance_safequit_context* safequit_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool skip_flag)
{
#ifdef USE_SMP
	if (context->config.smp_flag && !context->state.debugger_flag) {
		pthread_mutex_lock(&context->state.thread_video_mutex);

		log_debug(("advance:thread: signal\n"));

		/* notify that the data is ready */
		context->state.thread_state_ready_flag = 1;

		/* signal at the thread to start */
		pthread_cond_signal(&context->state.thread_video_cond);

		pthread_mutex_unlock(&context->state.thread_video_mutex);
	} else {
		video_frame_update_now(context, sound_context, estimate_context, record_context, ui_context, safequit_context, game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, sample_recount, skip_flag);
	}
#else
	video_frame_update_now(context, sound_context, estimate_context, record_context, ui_context, safequit_context, game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, sample_recount, skip_flag);
#endif
}

#ifdef USE_SMP
/**
 * Main video thread function.
 */
static void* video_thread(void* void_context)
{
	struct advance_video_context* context = &CONTEXT.video;
	struct advance_sound_context* sound_context = &CONTEXT.sound;
	struct advance_estimate_context* estimate_context = &CONTEXT.estimate;
	struct advance_record_context* record_context = &CONTEXT.record;
	struct advance_ui_context* ui_context = &CONTEXT.ui;
	struct advance_safequit_context* safequit_context = &CONTEXT.safequit;

	log_std(("advance:thread: start\n"));

	pthread_mutex_lock(&context->state.thread_video_mutex);

	while (1) {
		adv_bool exit;
		
		log_debug(("advance:thread: wait\n"));

		/* wait for the start notification */
		while (!context->state.thread_state_ready_flag && !context->state.thread_exit_flag) {
			pthread_cond_wait(&context->state.thread_video_cond, &context->state.thread_video_mutex);
		}

		log_debug(("advance:thread: wakeup\n"));

		exit = context->state.thread_exit_flag;

		/* now we can start to draw outside the lock */
		pthread_mutex_unlock(&context->state.thread_video_mutex);

		/* check for exit */
		if (exit) {
			break;
		}

		log_debug(("advance:thread: draw start\n"));

		/* update the frame */
		video_frame_update_now(
			context,
			sound_context,
			estimate_context,
			record_context,
			ui_context,
			safequit_context,
			context->state.thread_state_game,
			0,
			0,
			0,
			context->state.thread_state_led,
			context->state.thread_state_input,
			context->state.thread_state_sample_buffer,
			context->state.thread_state_sample_count,
			context->state.thread_state_sample_recount,
			context->state.thread_state_skip_flag
		);

		log_debug(("advance:thread: draw stop\n"));

		pthread_mutex_lock(&context->state.thread_video_mutex);

		/* notify that the data was used, and a new one can be setup */
		context->state.thread_state_ready_flag = 0;

		/* wakeup the main thread, signalign that the draw finished */
		pthread_cond_signal(&context->state.thread_video_cond);
	}

	pthread_exit(0);
	return 0;
}
#endif

/**
 * Callback for the osd_parallelize() function.
 * The threads are enabled only if the main SMP flag is activated.
 */
int thread_is_active(void)
{
#ifdef USE_SMP
	struct advance_video_context* context = &CONTEXT.video;
	return context->config.smp_flag;
#else
	return 0;
#endif
}

/**
 * Set a new video configuration.
 * If the configuration is invalid the previous one is restored.
 */
void advance_video_reconfigure(struct advance_video_context* context, struct advance_video_config_context* config)
{
	struct advance_video_config_context old_config;

	/* save the old configuration */
	old_config = context->config;

	/* set the new config */
	context->config = *config;

	/* we cannot change the video mode when the thread is running */
	advance_video_thread_wait(context);

	log_std(("emu:video: select mode %s\n", context->config.resolution_buffer));

	/* update all the complete state, the configuration is choosen by the name */
	if (advance_video_mode_update(context) != 0) {
		/* it fails in some strange conditions, generally when a not supported feature is used */
		/* for example if a interlaced mode is requested and the lower driver refuses to use it */

		/* restore the config */
		context->config = old_config;

		log_std(("emu:video: retrying old config\n"));

		if (advance_video_mode_update(context) != 0) {
			/* if something go wrong abort */
			log_std(("emu:video: video_reconfigure() failed\n"));
			target_err("Unexpected error changing video options.\n");
			abort();
		}
	}
}

/***************************************************************************/
/* OSD */

/**
 * Initialize and start the video thread.
 */
int osd2_thread_init(void)
{
#ifdef USE_SMP
	struct advance_video_context* context = &CONTEXT.video;

	log_std(("osd: osd2_thread_init\n"));

	context->state.thread_exit_flag = 0;
	context->state.thread_state_ready_flag = 0;
	context->state.thread_state_game = 0;
	context->state.thread_state_led = 0;
	context->state.thread_state_input = 0;
	context->state.thread_state_sample_count = 0;
	context->state.thread_state_sample_max = 0;
	context->state.thread_state_sample_buffer = 0;
	context->state.thread_state_skip_flag = 0;
	if (pthread_mutex_init(&context->state.thread_video_mutex, NULL) != 0) {
		log_std(("ERROR:advance: error calling pthread_mutex_init()\n"));
		target_err("Error initializing the thread system.\n");
		return -1;
	}
	if (pthread_cond_init(&context->state.thread_video_cond, NULL) != 0) {
		log_std(("ERROR:advance: error calling pthread_cond_init()\n"));
		target_err("Error initializing the thread system.\n");
		return -1;
	}
	log_std(("advance:thread: create\n"));
	if (pthread_create(&context->state.thread_id, NULL, video_thread, NULL) != 0) {
		log_std(("ERROR:advance: error calling pthread_create()\n"));
		target_err("Error initializing the thread system.\n");
		return -1;
	}
#endif

	return 0;
}

/**
 * Terminate and deallocate the video thread.
 */
void osd2_thread_done(void)
{
#ifdef USE_SMP
	struct advance_video_context* context = &CONTEXT.video;

	log_std(("osd: osd2_thread_done\n"));
	advance_video_thread_wait(context);

	log_std(("advance:thread: exit signal\n"));
	pthread_mutex_lock(&context->state.thread_video_mutex);
	context->state.thread_exit_flag = 1;
	pthread_cond_signal(&context->state.thread_video_cond);
	pthread_mutex_unlock(&context->state.thread_video_mutex);

	log_std(("advance:thread: join\n"));
	pthread_join(context->state.thread_id, NULL);

	log_std(("advance:thread: exit\n"));
	video_thread_bitmap_free(context->state.thread_state_game);
	free(context->state.thread_state_sample_buffer);
	pthread_cond_destroy(&context->state.thread_video_cond);
	pthread_mutex_destroy(&context->state.thread_video_mutex);

	log_std(("advance:thread: done\n"));
#endif
}

int osd2_video_init(struct osd_video_option* req)
{
	struct advance_video_context* context = &CONTEXT.video;
	struct advance_input_context* input_context = &CONTEXT.input;

	log_std(("osd: osd2_video_init\n"));

	log_std(("osd: area_size_x %d, area_size_y %d\n", req->area_size_x, req->area_size_y));
	log_std(("osd: used_size_x %d, used_size_y %d, used_pos_x %d, used_pos_y %d\n", req->used_size_x, req->used_size_y, req->used_pos_x, req->used_pos_y));
	log_std(("osd: aspect_x %d, aspect_y %d\n", req->aspect_x, req->aspect_y));
	log_std(("osd: bits_per_pixel %d, rgb_flag %d, colors %d\n", req->bits_per_pixel, req->rgb_flag, req->colors));
	log_std(("osd: vector_flag %d\n", req->vector_flag));
	log_std(("osd: fps %g\n", req->fps));

	if (advance_video_mode_init(context, req) != 0) {
		target_err("%s\n", error_get());
		return -1;
	}

	hardware_script_start(HARDWARE_SCRIPT_VIDEO);

	/* enable the keyboard input. It must be done after the video mode set. */
	if (keyb_enable(1) != 0) {
		goto err;
	}

	if (mouseb_enable() != 0) {
		keyb_disable();
		goto err;
	}

	if (joystickb_enable() != 0) {
		mouseb_disable();
		keyb_disable();
		goto err;
	}

	return 0;

err:
	context->config.restore_flag = 1; /* force a video mode restore */

	advance_video_mode_done(context);
	target_err("%s\n", error_get());
	return -1;
}

void osd2_video_done(void)
{
	struct advance_video_context* context = &CONTEXT.video;
	struct advance_input_context* input_context = &CONTEXT.input;

	log_std(("osd: osd2_video_done\n"));

	hardware_script_terminate(HARDWARE_SCRIPT_PLAY);
	hardware_script_terminate(HARDWARE_SCRIPT_EMULATION);
	hardware_script_terminate(HARDWARE_SCRIPT_VIDEO);

	joystickb_disable();
	mouseb_disable();
	keyb_disable();

	/* wait for thread end, before resetting the video mode */
	advance_video_thread_wait(context);

	advance_video_mode_done(context);

        /* print the speed measure */
	if (context->state.measure_flag
		&& context->state.measure_stop > context->state.measure_start) {
		target_out("%g\n", (double)(context->state.measure_stop - context->state.measure_start) / TARGET_CLOCKS_PER_SEC);
	}
}

void osd2_area(unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
	struct advance_video_context* context = &CONTEXT.video;
	unsigned pos_x;
	unsigned pos_y;
	unsigned size_x;
	unsigned size_y;

	log_std(("osd: osd2_area(%d, %d, %d, %d)\n", x1, y1, x2, y2));

	pos_x = x1;
	pos_y = y1;
	size_x = x2 - x1 + 1;
	size_y = y2 - y1 + 1;

	/* set the correct blit orientation */
	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(unsigned, pos_x, pos_y);
		SWAP(unsigned, size_x, size_y);
	}

	if (size_x != context->state.game_used_size_x
		|| size_y != context->state.game_used_size_y) {
		log_std(("ERROR:emu:video: change in the game area size. From %dx%d to %dx%d\n", context->state.game_used_size_x, context->state.game_used_size_y, size_x, size_y));
	}

	context->state.game_used_pos_x = pos_x;
	context->state.game_used_pos_y = pos_y;
	context->state.game_used_size_x = size_x;
	context->state.game_used_size_y = size_y;

	/* recompute the scale factor on game size change (you can test it with the game ehrgeiz) */
	advance_video_update_visible(context, &context->state.crtc_effective);
	advance_video_update_effect(context);

	/* recenter */
	advance_video_update_pan(context);

	/* on size change the pipeline must be recomputed */
	advance_video_invalidate_pipeline(context);
}

void osd2_palette(const osd_mask_t* mask, const osd_rgb_t* palette, unsigned size)
{
	struct advance_video_context* context = &CONTEXT.video;
	unsigned dirty_size;
	unsigned i;

	log_debug(("osd: osd2_palette(size:%d)\n", size));

	if (context->state.game_rgb_flag) {
		log_debug(("WARNING:emu:video: no palette because the game is in RGB mode\n"));
		return;
	}

	dirty_size = (size + osd_mask_size - 1) / osd_mask_size;

	if (size > context->state.palette_total || dirty_size > context->state.palette_dirty_total) {
		log_std(("ERROR:emu:video: invalid palette access, size:%d, total:%d, dirty_size:%d, total:%d\n", size, context->state.palette_total, dirty_size, context->state.palette_dirty_total));
		if (size > context->state.palette_total) {
			size = context->state.palette_total;
			dirty_size = (size + osd_mask_size - 1) / osd_mask_size;
			log_std(("WARNING:emu:video: invalid palette access, adjusting to size:%d, dirty_size:%d\n", size, dirty_size));
		} else {
			return;
		}
	}

	if (size > context->state.palette_total || dirty_size > context->state.palette_dirty_total) {
		log_std(("ERROR:emu:video: invalid palette access, second try, size:%d, total:%d, dirty_size:%d, total:%d\n", size, context->state.palette_total, dirty_size, context->state.palette_dirty_total));
		return;
	}

	context->state.palette_dirty_flag = 1;
	for(i=0;i<size;++i) {
		context->state.palette_map[i].red = osd_rgb_red(palette[i]);
		context->state.palette_map[i].green = osd_rgb_green(palette[i]);
		context->state.palette_map[i].blue = osd_rgb_blue(palette[i]);
	}

	if (dirty_size > 0 && dirty_size == context->state.palette_dirty_total) {
		/* the last element must be masked */
		for(i=0;i<dirty_size - 1;++i) {
			context->state.palette_dirty_map[i] |= mask[i];
		}
		context->state.palette_dirty_map[i] |= mask[i] & context->state.palette_dirty_mask;
	} else {
		for(i=0;i<dirty_size;++i) {
			context->state.palette_dirty_map[i] |= mask[i];
		}
	}
}

void osd2_video_pause(int pause)
{
	struct advance_video_context* context = &CONTEXT.video;

	log_std(("osd: osd2_video_pause(paused:%d)\n", pause));

	context->state.pause_flag = pause != 0;
}

void osd_reset(void)
{
	struct advance_video_context* context = &CONTEXT.video;

	log_std(("osd: osd_reset()\n"));

	hardware_script_terminate(HARDWARE_SCRIPT_PLAY);
	hardware_script_terminate(HARDWARE_SCRIPT_EMULATION);

	context->state.frame_counter = 0;

	/* initialize the fastest state */
	context->state.fastest_limit = context->config.fastest_time * context->state.game_fps;
	context->state.fastest_flag = context->state.fastest_limit != 0;

	/* initialize the measure state */
	context->state.measure_counter = context->config.measure_time * context->state.game_fps;
	context->state.measure_flag = context->state.measure_counter != 0;
	context->state.measure_start = target_clock();

	advance_video_update_skip(context);
	advance_video_update_sync(context);

	hardware_script_start(HARDWARE_SCRIPT_EMULATION);

	/* if not in "fastest" mode, the playing is already started */
	if (!context->state.fastest_flag || context->state.measure_flag)
		hardware_script_start(HARDWARE_SCRIPT_PLAY);
}

void osd2_debugger_focus(int debugger_has_focus)
{
	struct advance_video_context* context = &CONTEXT.video;

	log_std(("osd: osd_debugger_focus(debugger_has_focus:%d)\n", debugger_has_focus));

	context->state.debugger_flag = debugger_has_focus;

	advance_video_invalidate_screen(context);
}

int osd_skip_this_frame(void)
{
	struct advance_video_context* context = &CONTEXT.video;
	if (context->state.debugger_flag)
		return 0;
	else
		return context->state.skip_flag;
}

static int int_compare(const void* void_a, const void* void_b)
{
	int* a = (int*)void_a;
	int* b = (int*)void_b;
	if (*a < *b)
		return -1;
	if (*a > *b)
		return 1;
	return 0;
}

/** Number of steps, and maximum correction allowed, for small latency errors. */
#define AUDIOVIDEO_NEAR_STEP_COUNT 16

/** Number of frames on which distribute the latency error. */
#define AUDIOVIDEO_DISTRIBUTE_COUNT 4

int osd2_frame(const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count)
{
	struct advance_video_context* context = &CONTEXT.video;

	/* save the values of the previous skip frame */
	int skip_flag = context->state.skip_flag;

	/* current error of video/sound syncronization */
	int latency_diff;

	/* effective number of sound samples to output */
	int sample_recount;

	unsigned i;
	int sample_limit;
	int latency_limit;

	adv_bool normal_speed = video_is_normal_speed(&CONTEXT.video);

	/* store the current audio video syncronization error measured in sound samples */
	context->state.av_sync_map[context->state.av_sync_mac] = context->state.latency_diff;

	/* move the position on the circular buffer */
	if (context->state.av_sync_mac == AUDIOVIDEO_MEASURE_MAX - 1)
		context->state.av_sync_mac = 0;
	else
		++context->state.av_sync_mac;

	/* get the latency limit, the computation is different if the error */
	/* is bigger or lower than the limit */
	latency_limit = advance_sound_latency(&CONTEXT.sound, 0) / 2;

	if (normal_speed) {
		int latency_median;

		/* latency errors sorted */
		int median_map[AUDIOVIDEO_MEASURE_MAX];

		/* sort the last errors values */
		for(i=0;i<AUDIOVIDEO_MEASURE_MAX;++i) {
			median_map[i] = context->state.av_sync_map[i];
		}
		qsort(median_map, AUDIOVIDEO_MEASURE_MAX, sizeof(median_map[0]), int_compare);

		/* get the median value from the most recent errors */
		latency_median = median_map[AUDIOVIDEO_MEASURE_MAX/2];

		/* if playing at normal speed */
		if (latency_median >= -latency_limit && latency_median <= latency_limit) {
			/* if the error is small (in the latency_limit), use a small correction */
			latency_diff = latency_median / (latency_limit / AUDIOVIDEO_NEAR_STEP_COUNT);
		} else if (latency_median > latency_limit) {
			/* if the error is big, use a stronger correction */
			latency_diff = AUDIOVIDEO_NEAR_STEP_COUNT + (latency_median - latency_limit) / AUDIOVIDEO_DISTRIBUTE_COUNT;
		} else {
			latency_diff = -AUDIOVIDEO_NEAR_STEP_COUNT + (latency_median + latency_limit) / AUDIOVIDEO_DISTRIBUTE_COUNT;
		}

		if (latency_diff <= -AUDIOVIDEO_NEAR_STEP_COUNT || latency_diff >= AUDIOVIDEO_NEAR_STEP_COUNT) {
			/* report the big error */
			log_std(("WARNING:emu:video: audio/video syncronization correction of %d samples\n", latency_diff));
		} else if (latency_diff <= -3 || latency_diff >= 3) {
			/* report the error, an error of 1,2 samples is the normal condition */
			log_std(("emu:video: audio/video syncronization correction of %d samples\n", latency_diff));
		}
	} else {
		/* adjust without any check on sound distortion */
		latency_diff = context->state.latency_diff / AUDIOVIDEO_DISTRIBUTE_COUNT;
	}

	if (context->config.internalresample_flag) {
		sample_recount = sample_count - latency_diff;

		/* lower limit of number of samples */
		sample_limit = sample_count / 32;
		if (sample_limit < 16)
			sample_limit = 16;

		/* correction for a generic sound buffer underflow. */
		/* generally happen that the DMA buffer underflow, */
		/* reporting a fill state instead of an empty one. */
		if (sample_recount < sample_limit) {
			log_std(("WARNING:emu:video: too small sound samples %d adjusted to %d\n", sample_recount, sample_limit));
			sample_recount = sample_limit;
		}

		/* ask always at the core to use the nominal sample rate */
		latency_diff = 0;
	} else {
		/* ask the MAME core to adjust the number of generated samples */
		if (advance_record_sound_is_active(&CONTEXT.record)) {
			/* if recording is active doesn't skip any samples */
			latency_diff = 0;
		}

		sample_recount = sample_count;
	}

	if (context->config.rawsound_flag) {
		sample_recount = sample_count;
		latency_diff = 0;
	}

	/* update the global info */
	video_command(&CONTEXT.video, &CONTEXT.estimate, &CONTEXT.safequit, &CONTEXT.ui, CONTEXT.cfg, led, input, skip_flag);
	advance_video_skip(&CONTEXT.video, &CONTEXT.estimate, &CONTEXT.record);
	advance_input_update(&CONTEXT.input, &CONTEXT.safequit, CONTEXT.video.state.pause_flag);

	/* estimate the time */
	advance_estimate_frame(&CONTEXT.estimate);
	advance_estimate_mame_end(&CONTEXT.estimate, skip_flag);

	/* prepare the frame */
	video_frame_prepare(&CONTEXT.video, &CONTEXT.sound, &CONTEXT.estimate, game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, sample_recount, skip_flag);

	/* update the local info */
	video_frame_update(&CONTEXT.video, &CONTEXT.sound, &CONTEXT.estimate, &CONTEXT.record, &CONTEXT.ui, &CONTEXT.safequit, game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, sample_recount, skip_flag);

	/* estimate the time */
	advance_estimate_mame_begin(&CONTEXT.estimate);

	return latency_diff;
}

/***************************************************************************/
/* Init/Done/Load */

/* Adjust the orientation with the requested rol/ror/flipx/flipy operations */
static unsigned video_orientation_compute(unsigned orientation, adv_bool rol, adv_bool ror, adv_bool flipx, adv_bool flipy)
{
	if (ror) {
		/* if only one of the components is inverted, switch them */
		if ((orientation & OSD_ORIENTATION_ROT180) == OSD_ORIENTATION_FLIP_X || (orientation & OSD_ORIENTATION_ROT180) == OSD_ORIENTATION_FLIP_Y)
			orientation ^= OSD_ORIENTATION_ROT180;
		orientation ^= OSD_ORIENTATION_ROT90;
	}

	if (rol) {
		/* if only one of the components is inverted, switch them */
		if ((orientation & OSD_ORIENTATION_ROT180) == OSD_ORIENTATION_FLIP_X || (orientation & OSD_ORIENTATION_ROT180) == OSD_ORIENTATION_FLIP_Y)
			orientation ^= OSD_ORIENTATION_ROT180;
		orientation ^= OSD_ORIENTATION_ROT270;
	}

	if (flipx) {
		orientation ^= OSD_ORIENTATION_FLIP_X;
	}

	if (flipy) {
		orientation ^= OSD_ORIENTATION_FLIP_Y;
	}

	return orientation;
}

/***************************************************************************/
/* Config */

static adv_conf_enum_int OPTION_RESIZE[] = {
{ "none", STRETCH_NONE },
{ "integer", STRETCH_INTEGER_XY },
{ "fractional", STRETCH_FRACTIONAL_XY },
{ "mixed", STRETCH_INTEGER_X_FRACTIONAL_Y }
};

static adv_conf_enum_int OPTION_RESIZEEFFECT[] = {
{ "auto", COMBINE_AUTO },
{ "none", COMBINE_NONE },
{ "max", COMBINE_MAXMIN },
{ "mean", COMBINE_MEAN },
{ "filter", COMBINE_FILTER },
{ "scalex", COMBINE_SCALEX },
{ "scalek", COMBINE_SCALEK },
{ "lq", COMBINE_LQ },
{ "hq", COMBINE_HQ },
{ "xbr", COMBINE_XBR }
};

static adv_conf_enum_int OPTION_ADJUST[] = {
{ "none", ADJUST_NONE },
{ "x", ADJUST_ADJUST_X },
{ "clock", ADJUST_ADJUST_CLOCK },
{ "xclock", ADJUST_ADJUST_X | ADJUST_ADJUST_CLOCK },
{ "generate_exact", ADJUST_GENERATE | ADJUST_ADJUST_X },
{ "generate_y", ADJUST_GENERATE | ADJUST_ADJUST_X | ADJUST_ADJUST_Y },
{ "generate_clock", ADJUST_GENERATE | ADJUST_ADJUST_X | ADJUST_ADJUST_CLOCK },
{ "generate_yclock", ADJUST_GENERATE | ADJUST_ADJUST_X | ADJUST_ADJUST_Y | ADJUST_ADJUST_CLOCK | ADJUST_FAVORITE_SIZE_OVER_CLOCK },
{ "generate_clocky", ADJUST_GENERATE | ADJUST_ADJUST_X | ADJUST_ADJUST_Y | ADJUST_ADJUST_CLOCK }
};

static adv_conf_enum_int OPTION_MAGNIFY[] = {
{ "auto", 0 },
{ "1", 1 },
{ "2", 2 },
{ "3", 3 },
{ "4", 4 }
};

static adv_conf_enum_int OPTION_RESAMPLE[] = {
{ "auto", -1 },
{ "emulation", 0 },
{ "internal", 1 }
};

static adv_conf_enum_int OPTION_RGBEFFECT[] = {
{ "none", EFFECT_NONE },
{ "triad3dot", EFFECT_RGB_TRIAD3PIX },
{ "triad6dot", EFFECT_RGB_TRIAD6PIX },
{ "triad16dot", EFFECT_RGB_TRIAD16PIX },
{ "triadstrong3dot", EFFECT_RGB_TRIADSTRONG3PIX },
{ "triadstrong6dot", EFFECT_RGB_TRIADSTRONG6PIX },
{ "triadstrong16dot", EFFECT_RGB_TRIADSTRONG16PIX },
{ "scan2horz", EFFECT_RGB_SCANDOUBLEHORZ },
{ "scan2vert", EFFECT_RGB_SCANDOUBLEVERT },
{ "scan3horz", EFFECT_RGB_SCANTRIPLEHORZ },
{ "scan3vert", EFFECT_RGB_SCANTRIPLEVERT }
};

static adv_conf_enum_int OPTION_INTERLACEEFFECT[] = {
{ "none", EFFECT_NONE },
{ "odd", EFFECT_INTERLACE_ODD },
{ "even", EFFECT_INTERLACE_EVEN },
{ "filter", EFFECT_INTERLACE_FILTER }
};

static adv_conf_enum_int OPTION_INDEX[] = {
{ "auto", MODE_FLAGS_INDEX_NONE },
{ "palette8", MODE_FLAGS_INDEX_PALETTE8 },
{ "bgr8", MODE_FLAGS_INDEX_BGR8 },
{ "bgr15", MODE_FLAGS_INDEX_BGR15 },
{ "bgr16", MODE_FLAGS_INDEX_BGR16 },
{ "bgr32", MODE_FLAGS_INDEX_BGR32 },
{ "yuy2", MODE_FLAGS_INDEX_YUY2 }
};

adv_error advance_video_init(struct advance_video_context* context, adv_conf* cfg_context)
{
	unsigned i;

	/* other initialization */
	context->state.av_sync_mac = 0;
	for(i=0;i<AUDIOVIDEO_MEASURE_MAX;++i)
		context->state.av_sync_map[i] = 0;
	context->state.latency_diff = 0;

	conf_bool_register_default(cfg_context, "display_scanlines", 0);
	conf_bool_register_default(cfg_context, "display_vsync", 1);
	conf_bool_register_default(cfg_context, "display_buffer", 0);
	conf_int_register_enum_default(cfg_context, "display_resize", conf_enum(OPTION_RESIZE), STRETCH_FRACTIONAL_XY);
	conf_int_register_enum_default(cfg_context, "display_magnify", conf_enum(OPTION_MAGNIFY), 1);
	conf_int_register_default(cfg_context, "display_magnifysize", 640);
	conf_int_register_enum_default(cfg_context, "display_adjust", conf_enum(OPTION_ADJUST), ADJUST_NONE);
	conf_string_register_default(cfg_context, "display_skiplines", "auto");
	conf_string_register_default(cfg_context, "display_skipcolumns", "auto");
	conf_string_register_default(cfg_context, "display_frameskip", "auto");
	conf_bool_register_default(cfg_context, "display_ror", 0);
	conf_bool_register_default(cfg_context, "display_rol", 0);
	conf_bool_register_default(cfg_context, "display_flipx", 0);
	conf_bool_register_default(cfg_context, "display_flipy", 0);
	conf_int_register_enum_default(cfg_context, "display_resizeeffect", conf_enum(OPTION_RESIZEEFFECT), COMBINE_AUTO);
	conf_int_register_enum_default(cfg_context, "display_rgbeffect", conf_enum(OPTION_RGBEFFECT), EFFECT_NONE);
	conf_int_register_enum_default(cfg_context, "display_interlaceeffect", conf_enum(OPTION_INTERLACEEFFECT), EFFECT_NONE);
	conf_string_register_default(cfg_context, "sync_fps", "auto");
	conf_float_register_limit_default(cfg_context, "sync_speed", 0.1, 10.0, 1.0);
	conf_float_register_limit_default(cfg_context, "sync_turbospeed", 0.1, 30.0, 3.0);
	conf_bool_register_default(cfg_context, "debug_crash", 0);
	conf_bool_register_default(cfg_context, "debug_rawsound", 0);
	conf_string_register_default(cfg_context, "sync_startuptime", "auto");
	conf_int_register_limit_default(cfg_context, "misc_timetorun", 0, 3600, 0);
	conf_string_register_default(cfg_context, "display_mode", "auto");
	conf_int_register_enum_default(cfg_context, "display_color", conf_enum(OPTION_INDEX), 0);
	conf_bool_register_default(cfg_context, "display_restore", 1);
	conf_float_register_limit_default(cfg_context, "display_expand", 1.0, 10.0, 1.0);
	conf_int_register_limit_default(cfg_context, "display_aspectx", 1, 10000, 4);
	conf_int_register_limit_default(cfg_context, "display_aspecty", 1, 10000, 3);

#ifdef USE_SMP
	conf_bool_register_default(cfg_context, "misc_smp", 0);
#endif

	conf_int_register_enum_default(cfg_context, "sync_resample", conf_enum(OPTION_RESAMPLE), -1);

	monitor_register(cfg_context);
	crtc_container_register(cfg_context);
	generate_interpolate_register(cfg_context);

	video_reg(cfg_context, 1);
	video_reg_driver_all(cfg_context);

	/* load graphics modes */
	crtc_container_init(&context->config.crtc_bag);

	return 0;
}

static const char* GAME_BLIT_COMBINE_MAX[] = {
#include "blitmax.h"
0
};

static struct game_startup_struct {
	const char* name;
	int time;
} GAME_STARTUP[] = {
#include "startup.h"
{ 0 }
};

void advance_video_config_save(struct advance_video_context* context, const char* section)
{
	adv_conf* cfg_context = CONTEXT.cfg;

	conf_string_set_if_different(cfg_context, section, "display_mode", context->config.resolution_buffer);
	if (!context->state.game_vector_flag) {
		conf_int_set_if_different(cfg_context, section, "display_resizeeffect", context->config.combine);
		conf_int_set_if_different(cfg_context, section, "display_rgbeffect", context->config.rgb_effect);
		conf_int_set_if_different(cfg_context, section, "display_resize", context->config.stretch);
		conf_int_set_if_different(cfg_context, section, "display_magnify", context->config.magnify_factor);
		conf_int_set_if_different(cfg_context, section, "display_color", context->config.index);
		if (context->state.game_visible_size_x < context->state.game_used_size_x
			|| context->state.game_visible_size_y < context->state.game_used_size_y)
		{
			if (context->config.skipcolumns>=0) {
				char buffer[32];
				snprintf(buffer, sizeof(buffer), "%d", context->config.skipcolumns);
				conf_string_set_if_different(cfg_context, section, "display_skipcolumns", buffer);
			} else
				conf_string_set_if_different(cfg_context, section, "display_skipcolumns", "auto");
			if (context->config.skiplines>=0) {
				char buffer[32];
				snprintf(buffer, sizeof(buffer), "%d", context->config.skiplines);
				conf_string_set_if_different(cfg_context, section, "display_skiplines", buffer);
			} else
				conf_string_set_if_different(cfg_context, section, "display_skiplines", "auto");
		} else {
			conf_remove(cfg_context, section, "display_skipcolumns");
			conf_remove(cfg_context, section, "display_skiplines");
		}
		conf_bool_set_if_different(cfg_context, section, "display_vsync", context->config.vsync_flag);
	}

	advance_global_message(&CONTEXT.global, "Video options saved in %s/", section);
}

adv_error advance_video_config_load(struct advance_video_context* context, adv_conf* cfg_context, struct mame_option* option)
{
	const char* s;
	adv_error err;
	int i;
	adv_bool ror, rol, flipx, flipy;
	double d;

	context->config.debug_flag = option->debug_flag;

	context->config.game_orientation = mame_game_orientation(option->game);

	for(i=0;GAME_BLIT_COMBINE_MAX[i] != 0;++i)
		if (mame_is_game_relative(GAME_BLIT_COMBINE_MAX[i], option->game))
			break;
	context->config.inlist_combinemax_flag = GAME_BLIT_COMBINE_MAX[i] != 0;
	sncpy(context->config.section_name_buffer, sizeof(context->config.section_name_buffer), mame_section_name(option->game, cfg_context));
	sncpy(context->config.section_resolution_buffer, sizeof(context->config.section_resolution_buffer), mame_game_resolution(option->game));
	sncpy(context->config.section_resolutionclock_buffer, sizeof(context->config.section_resolutionclock_buffer), mame_game_resolutionclock(option->game));
	if ((context->config.game_orientation & OSD_ORIENTATION_SWAP_XY) != 0)
		sncpy(context->config.section_orientation_buffer, sizeof(context->config.section_orientation_buffer), "vertical");
	else
		sncpy(context->config.section_orientation_buffer, sizeof(context->config.section_orientation_buffer), "horizontal");

	context->config.scanlines_flag = conf_bool_get_default(cfg_context, "display_scanlines");
	context->config.vsync_flag = conf_bool_get_default(cfg_context, "display_vsync");
	context->config.triplebuf_flag = conf_bool_get_default(cfg_context, "display_buffer");
	context->config.stretch = conf_int_get_default(cfg_context, "display_resize");
	context->config.magnify_factor = conf_int_get_default(cfg_context, "display_magnify");
	context->config.magnify_size = conf_int_get_default(cfg_context, "display_magnifysize");
	context->config.adjust = conf_int_get_default(cfg_context, "display_adjust");

	context->config.monitor_aspect_x = conf_int_get_default(cfg_context, "display_aspectx");
	context->config.monitor_aspect_y = conf_int_get_default(cfg_context, "display_aspecty");

	s = conf_string_get_default(cfg_context, "display_skiplines");
	if (strcmp(s, "auto")==0) {
		context->config.skiplines = -1;
	} else {
		context->config.skiplines = atoi(s);
	}

	s = conf_string_get_default(cfg_context, "display_skipcolumns");
	if (strcmp(s, "auto")==0) {
		context->config.skipcolumns = -1;
	} else {
		context->config.skipcolumns = atoi(s);
	}

	ror = conf_bool_get_default(cfg_context, "display_ror");
	rol = conf_bool_get_default(cfg_context, "display_rol");
	flipx = conf_bool_get_default(cfg_context, "display_flipx");
	flipy = conf_bool_get_default(cfg_context, "display_flipy");

	/* orientation */
	context->config.blit_orientation = video_orientation_compute(context->config.game_orientation, rol, ror, flipx, flipy);
	context->config.user_orientation = video_orientation_compute(0, rol, ror, flipx, flipy);
	option->ui_orientation = adv_orientation_rev(context->config.game_orientation);
	option->direct_orientation = adv_orientation_rev(context->config.game_orientation);

	log_std(("emu:video: orientation game %04x\n", context->config.game_orientation));
	log_std(("emu:video: orientation user %04x\n", context->config.user_orientation));
	log_std(("emu:video: orientation blit %04x\n", context->config.blit_orientation));
	log_std(("emu:video: orientation ui   %04x\n", option->ui_orientation));

	context->config.combine = conf_int_get_default(cfg_context, "display_resizeeffect");
	context->config.combine_max = COMBINE_XBR;
	context->config.rgb_effect = conf_int_get_default(cfg_context, "display_rgbeffect");
	context->config.interlace_effect = conf_int_get_default(cfg_context, "display_interlaceeffect");
	context->config.turbo_speed_factor = conf_float_get_default(cfg_context, "sync_turbospeed");
	s = conf_string_get_default(cfg_context, "sync_fps");
	if (strcmp(s, "auto")==0) {
		context->config.fps_fixed = 0;
	} else {
		char* e;
		context->config.fps_fixed = strtod(s, &e);
		if (context->config.fps_fixed < 10 || context->config.fps_fixed > 300 || *e) {
			target_err("Invalid argument '%s' for option 'sync_fps'.\n", s);
			return -1;
		}
	}
	context->config.fps_speed_factor = conf_float_get_default(cfg_context, "sync_speed");

	s = conf_string_get_default(cfg_context, "sync_startuptime");
	if (strcmp(s, "none") == 0) {
		d = 0;
	} else if (strcmp(s, "auto") == 0) {
		for(i=0;GAME_STARTUP[i].name != 0;++i)
			if (mame_is_game_relative(GAME_STARTUP[i].name, option->game))
				break;
		if (GAME_STARTUP[i].name != 0)
			d = GAME_STARTUP[i].time;
		else
			d = 0;
	} else {
		char* e;
		d = strtod(s, &e);
		if (*e != 0 || d < 0 || d > 180) {
			target_err("Invalid argument '%s' for option 'sync_startuptime'.\n", s);
			return -1;
		}
	}
	context->config.fastest_time = d;
	context->config.measure_time = conf_int_get_default(cfg_context, "misc_timetorun");
	context->config.crash_flag = conf_bool_get_default(cfg_context, "debug_crash");
	context->config.rawsound_flag = conf_bool_get_default(cfg_context, "debug_rawsound");

	s = conf_string_get_default(cfg_context, "display_mode");
	sncpy(context->config.resolution_buffer, sizeof(context->config.resolution_buffer), s);

	context->config.index = conf_int_get_default(cfg_context, "display_color");

	context->config.restore_flag = conf_bool_get_default(cfg_context, "display_restore");
	context->config.aspect_expansion_factor = conf_float_get_default(cfg_context, "display_expand");

	s = conf_string_get_default(cfg_context, "display_frameskip");
	if (strcmp(s, "auto")==0) {
		context->config.frameskip_auto_flag = 1;
		context->config.frameskip_factor = 1.0;
	} else {
		char* e;
		context->config.frameskip_auto_flag = 0;
		context->config.frameskip_factor = strtod(s, &e);
		if (context->config.frameskip_factor < 0.0 || context->config.frameskip_factor > 1.0 || *e) {
			target_err("Invalid argument '%s' for option 'display_frameskip'.\n", s);
			return -1;
		}
	}

#ifdef USE_SMP
	context->config.smp_flag = conf_bool_get_default(cfg_context, "misc_smp");
#else
	context->config.smp_flag = 0;
#endif

	i = conf_int_get_default(cfg_context, "sync_resample");
	if (i==1 || i==-1)
		context->config.internalresample_flag = 1; /* internal resampling */
	else
		context->config.internalresample_flag = 0; /* emulation resampling */

	/* load context->config.monitor config */
	err = monitor_load(cfg_context, &context->config.monitor);
	if (err<0) {
		target_err("%s\n", error_get());
		target_err("Please read the file `install.txt' and `device.txt'.\n");
		return -1;
	}
	if (err==0) {
		char buffer[1024];
		monitor_print(buffer, sizeof(buffer), &context->config.monitor);
		log_std(("emu:video: clock %s\n", buffer));
	}
	if (err>0) {
		monitor_reset(&context->config.monitor);
	}

	/* load generate_linear config */
	err = generate_interpolate_load(cfg_context, &context->config.interpolate);
	if (err<0) {
		target_err("%s\n", error_get());
		target_err("Please read the file `install.txt' and `device.txt'.\n");
		return -1;
	} else if (err>0) {
		if (monitor_clock_check(&context->config.monitor, 7.16E6, 15720, 60)) {
			/* Arcade Standard Resolution */
			log_std(("emu:video: default format standard resolution\n"));
			generate_default_atari_standard(&context->config.interpolate.map[0].gen);
			context->config.interpolate.map[0].hclock = 15720;
			context->config.interpolate.mac = 1;
		} else if (monitor_clock_check(&context->config.monitor, 16E6, 25000, 60)) {
			/* Arcade Medium Resolution */
			log_std(("emu:video: default format medium resolution\n"));
			generate_default_atari_medium(&context->config.interpolate.map[0].gen);
			context->config.interpolate.map[0].hclock = 25000;
			context->config.interpolate.mac = 1;
		} else {
			/* VGA Resolution */
			log_std(("emu:video: default format vga resolution\n"));
			generate_default_vga(&context->config.interpolate.map[0].gen);
			context->config.interpolate.map[0].hclock = 31500;
			context->config.interpolate.mac = 1;
		}
	}

	/* Ignore the unused driver. These are generally the driver needed */
	/* for the text mode, not used by the emulator. */
	if (video_load(cfg_context, "slang") != 0) {
		target_err("Error loading the video configuration.\n");
		return -1;
	}

	if (crtc_container_load(cfg_context, &context->config.crtc_bag)!=0) {
		target_err("%s\n", error_get());
		target_err("Invalid modeline.\n");
		return -1;
	}

	return 0;
}

void advance_video_done(struct advance_video_context* context)
{
	crtc_container_done(&context->config.crtc_bag);
}

adv_error advance_video_inner_init(struct advance_video_context* context, struct mame_option* option)
{
	if (adv_video_init() != 0) {
		target_err("%s\n", error_get());
		return -1;
	}

	if (video_blit_init() != 0) {
		adv_video_done();
		target_err("%s\n", error_get());
		return -1;
	}

	advance_video_mode_preinit(context, option);

	return 0;
}

void advance_video_inner_done(struct advance_video_context* context)
{
	video_blit_done();
	adv_video_done();
}


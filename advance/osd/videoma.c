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
#include "thread.h"
#include "glue.h"
#include "input.h"
#include "hscript.h"
#include "script.h"

#include "advance.h"

#include <math.h>
#include <limits.h>

#ifdef USE_SMP
#include <pthread.h>
#endif

/** Max framskip factor */
#define SYNC_MAX 4

/***************************************************************************/
/*
	Description of the various coordinate systems and variables :
	All these values are already postprocessed after any SWAP/FLIP.

	game_area_size_x/y
		Max size of the visible part of the game screen.

	game_used_size_x/y
		Current size of the used part of the game screen.

	game_used_pos_x/y
		Current position of the used part in the whole game are.

	game_visible_size_x/y
		Size of the visible part of the game screen. Can be smaller
		than game_used_size_x/y if the video mode is too small.
		game_visible_ is always internal of the game_used part.

	game_visible_pos_x/y
		Position of the visible part in the game screen. The coords
		are referenced on the used game area. So, they can be negative
		but still valid because they are in the game area, also if it isn't
		used.

	mode_visible_size_x/y
		Part of the screen used for drawing. game_visible_* area is
		drawn in screen_visible_* area, eventually stretching.
		mode_visible_size may be smaller than the video mode if the game
		doesn't have the same aspect ratio of the screen.
*/

/***************************************************************************/
/* Save */

void advance_video_save(struct advance_video_context* context, const char* section)
{
	adv_conf* cfg_context = CONTEXT.cfg;

	conf_string_set(cfg_context, section, "display_mode", context->config.resolution_buffer);
	if (!context->state.game_vector_flag) {
		conf_int_set(cfg_context, section, "display_resizeeffect", context->config.combine);
		conf_int_set(cfg_context, section, "display_rgbeffect", context->config.rgb_effect);
		conf_int_set(cfg_context, section, "display_resize", context->config.stretch);
		conf_int_set(cfg_context, section, "display_magnify", context->config.magnify_factor);
		conf_int_set(cfg_context, section, "display_color", context->config.index);
		if (context->state.game_visible_size_x < context->state.game_used_size_x
			|| context->state.game_visible_size_y < context->state.game_used_size_y)
		{
			if (context->config.skipcolumns>=0) {
				char buffer[32];
				snprintf(buffer, sizeof(buffer), "%d", context->config.skipcolumns);
				conf_string_set(cfg_context, section, "display_skipcolumns", buffer);
			} else
				conf_string_set(cfg_context, section, "display_skipcolumns", "auto");
			if (context->config.skiplines>=0) {
				char buffer[32];
				snprintf(buffer, sizeof(buffer), "%d", context->config.skiplines);
				conf_string_set(cfg_context, section, "display_skiplines", buffer);
			} else
				conf_string_set(cfg_context, section, "display_skiplines", "auto");
		} else {
			conf_remove(cfg_context, section, "display_skipcolumns");
			conf_remove(cfg_context, section, "display_skiplines");
		}
		conf_bool_set(cfg_context, section, "display_scanlines", context->config.scanlines_flag);
		conf_bool_set(cfg_context, section, "display_vsync", context->config.vsync_flag);
	}

	advance_global_message(&CONTEXT.global, "Video options saved in %s/", section);
}

/***************************************************************************/
/* Update */

/**
 * Check if the video output is programmable.
 */
static adv_bool video_is_programmable(struct advance_video_context* context)
{
	return (video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0;
}

/**
 * Check if the video generation is active.
 */
static adv_bool video_is_generable(struct advance_video_context* context)
{
	return (context->config.adjust & ADJUST_GENERATE) != 0
		&& video_is_programmable(context)
		&& context->config.interpolate.mac > 0;
}

/**
 * Adjust the CRTC value.
 * \return 0 on success
 */
static adv_error video_make_crtc(struct advance_video_context* context, adv_crtc* crtc, const adv_crtc* original_crtc)
{
	*crtc = *original_crtc;

	if (!video_is_programmable(context)) {
		return 0; /* always ok if the driver is not programmable */
	}

	/* adjust the X size */
	if ((context->config.adjust & ADJUST_ADJUST_X) != 0) {
		unsigned best_size_x;

		if (!context->state.game_vector_flag) {
			switch (context->config.magnify_factor) {
			case 1 : best_size_x = context->state.mode_best_size_x; break;
			case 2 : best_size_x = context->state.mode_best_size_2x; break;
			case 3 : best_size_x = context->state.mode_best_size_3x; break;
			case 4 : best_size_x = context->state.mode_best_size_4x; break;
			default: /* auto setting */
				if (context->state.mode_best_size_x >= 512 || context->state.mode_best_size_y >= 384)
					best_size_x = context->state.mode_best_size_x;
				else if (context->state.mode_best_size_x >= 256 || context->state.mode_best_size_y >= 256)
					best_size_x = context->state.mode_best_size_2x;
				else if (context->state.mode_best_size_x >= 192 || context->state.mode_best_size_y >= 192)
					best_size_x = context->state.mode_best_size_3x;
				else
					best_size_x = context->state.mode_best_size_4x;
				break;
			}
		} else {
			best_size_x = context->state.mode_best_size_x;
		}

		crtc_hsize_set(crtc, best_size_x);
	}

	/* adjust the clock */
	if ((context->config.adjust & ADJUST_ADJUST_CLOCK) != 0) {
		crtc_vclock_set(crtc, context->state.game_fps);

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_vclock_set(crtc, 2 * context->state.game_fps);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_vclock_set(crtc, 3 * context->state.game_fps);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_vclock_set(crtc, 4 * context->state.game_fps);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			if (crtc_adjust_clock(crtc, &context->config.monitor) != 0) {
				/* ignore error */
			}
		}
	}

	if (!crtc_clock_check(&context->config.monitor, crtc)) {
		log_std(("emu:video: failed check on the final clock %g/%g/%g\n", (double)crtc->pixelclock / 1E6, crtc_hclock_get(crtc) / 1E3, crtc_vclock_get(crtc)));
		return -1;
	}

	return 0;
}

/**
 * Update the skip state.
 */
static void video_update_skip(struct advance_video_context* context)
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
 * Update the sync state.
 */
static void video_update_sync(struct advance_video_context* context)
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

/**
 * Select the video depth.
 * \return 0 on success
 */
static adv_error video_update_index(struct advance_video_context* context)
{
	unsigned select_pref_palette8[] = { MODE_FLAGS_INDEX_PALETTE8, MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_BGR8, MODE_FLAGS_INDEX_YUY2, 0 };
	unsigned select_pref_bgr8[] = { MODE_FLAGS_INDEX_BGR8, MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_YUY2, 0 };
	unsigned select_pref_bgr15[] = { MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_BGR8,  MODE_FLAGS_INDEX_YUY2, 0 };
	unsigned select_pref_bgr16[] = { MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_BGR8, MODE_FLAGS_INDEX_YUY2, 0 };
	unsigned select_pref_bgr32[] = { MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR8, MODE_FLAGS_INDEX_YUY2, 0 };
	unsigned select_pref_yuy2[] = { MODE_FLAGS_INDEX_YUY2, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR8, 0 };
	unsigned* select;
	unsigned index;

	adv_bool mode_may_be_palette = !context->state.game_rgb_flag
		&& context->state.game_colors <= 256
		&& !context->config.debug_flag;

	index = context->config.index;

	/* remove the palette request if the game is not palettizable */
	if (!mode_may_be_palette && index == MODE_FLAGS_INDEX_PALETTE8)
		index = MODE_FLAGS_INDEX_NONE;

	if (index == MODE_FLAGS_INDEX_NONE) {
		/* get the video driver preferred bit depth */
		switch (video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_DEFAULT_MASK) {
		case VIDEO_DRIVER_FLAGS_DEFAULT_PALETTE8 :
			index = MODE_FLAGS_INDEX_PALETTE8;
			break;
		case VIDEO_DRIVER_FLAGS_DEFAULT_BGR8 :
			index = MODE_FLAGS_INDEX_BGR8;
			break;
		case VIDEO_DRIVER_FLAGS_DEFAULT_BGR15 :
			index = MODE_FLAGS_INDEX_BGR15;
			break;
		case VIDEO_DRIVER_FLAGS_DEFAULT_BGR16 :
			index = MODE_FLAGS_INDEX_BGR16;
			break;
		case VIDEO_DRIVER_FLAGS_DEFAULT_BGR32 :
			index = MODE_FLAGS_INDEX_BGR32;
			break;
		case VIDEO_DRIVER_FLAGS_DEFAULT_YUY2 :
			index = MODE_FLAGS_INDEX_YUY2;
			break;
		}
	};

	if (index == MODE_FLAGS_INDEX_NONE) {
		if (mode_may_be_palette) {
			/* don't use MODE_FLAGS_INDEX_PALETTE8 as default, to allow a rgb user interface */
			index = MODE_FLAGS_INDEX_BGR16;
		} else {
			switch (context->state.game_bits_per_pixel) {
			case 8 :
				index = MODE_FLAGS_INDEX_BGR8;
				break;
			case 15 :
				index = MODE_FLAGS_INDEX_BGR15;
				break;
			case 16 :
				index = MODE_FLAGS_INDEX_BGR16;
				break;
			case 32 :
				index = MODE_FLAGS_INDEX_BGR32;
				break;
			default:
				log_std(("ERROR:emu:video: invalid game_bits_per_pixel\n"));
				return -1;
			}
		}
	}

	switch (index) {
		case MODE_FLAGS_INDEX_PALETTE8 : select = select_pref_palette8; break;
		case MODE_FLAGS_INDEX_BGR8 : select = select_pref_bgr8; break;
		case MODE_FLAGS_INDEX_BGR15 : select = select_pref_bgr15; break;
		case MODE_FLAGS_INDEX_BGR16 : select = select_pref_bgr16; break;
		case MODE_FLAGS_INDEX_BGR32 : select = select_pref_bgr32; break;
		case MODE_FLAGS_INDEX_YUY2 : select = select_pref_yuy2; break;
		default:
			log_std(("ERROR:emu:video: invalid index\n"));
			return -1;
	}

	while (*select) {
		unsigned flag;
		switch (*select) {
			case MODE_FLAGS_INDEX_PALETTE8 : flag = VIDEO_DRIVER_FLAGS_MODE_PALETTE8; break;
			case MODE_FLAGS_INDEX_BGR8 : flag = VIDEO_DRIVER_FLAGS_MODE_BGR8; break;
			case MODE_FLAGS_INDEX_BGR15 : flag = VIDEO_DRIVER_FLAGS_MODE_BGR15; break;
			case MODE_FLAGS_INDEX_BGR16 : flag = VIDEO_DRIVER_FLAGS_MODE_BGR16; break;
			case MODE_FLAGS_INDEX_BGR32 : flag = VIDEO_DRIVER_FLAGS_MODE_BGR32; break;
			case MODE_FLAGS_INDEX_YUY2 : flag = VIDEO_DRIVER_FLAGS_MODE_YUY2; break;
			default:
				log_std(("ERROR:emu:video: invalid *select\n"));
				return -1;
		}
		if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & flag) != 0) {
			/* accept a palette mode only if it's usable */
			if (mode_may_be_palette || *select != MODE_FLAGS_INDEX_PALETTE8) {
				break;
			}
		}
		++select;
	}

	if (!*select) {
		log_std(("ERROR:emu:video: no mode supported\n"));
		return -1;
	}

	context->state.mode_index = *select;

	return 0;
}

/**
 * Create the video mode from the crtc.
 * \return 0 on success
 */
static adv_error video_make_vidmode(struct advance_video_context* context, adv_mode* mode, const adv_crtc* crtc)
{
	if (video_mode_generate(mode, crtc, context->state.mode_index)!=0) {
		log_std(("ERROR:emu:video: video_mode_generate failed '%s'\n", error_get()));
		return -1;
	}

	return 0;
}

/**
 * Clear all the screens.
 */
static void video_invalidate_screen(void)
{
	unsigned i;
	adv_pixel color;

	assert( video_mode_is_active() );

	/* on palettized modes it always return 0 */
	color = video_pixel_get(0, 0, 0);

	/* intentionally doesn't clear the entire video memory, */
	/* it's more safe to clear only the used part, for example */
	/* if case the memory size detection is wrong  */
	for(i=0;i<update_page_max_get();++i) {
		update_start();
		log_std(("emu:video: clear %dx%d %dx%d\n", update_x_get(), update_y_get(), video_size_x(), video_size_y()));
		video_clear(update_x_get(), update_y_get(), video_size_x(), video_size_y(), color);
		update_stop(update_x_get(), update_y_get(), video_size_x(), video_size_y(), color);
	}
}

/**
 * Invalidate all the color information.
 * At the next frame all the color information are recomputed.
 */
static void video_invalidate_color(struct advance_video_context* context)
{
	/* set all dirty */
	if (!context->state.game_rgb_flag) {
		unsigned i;
		context->state.palette_dirty_flag = 1;
		for(i=0;i<context->state.palette_dirty_total;++i)
			context->state.palette_dirty_map[i] = osd_mask_full;
	}
}

static void video_done_pipeline(struct advance_video_context* context)
{
	/* destroy the pipeline */
	if (context->state.blit_pipeline_flag) {
		video_pipeline_done(&context->state.blit_pipeline_video);
		video_pipeline_done(&context->state.buffer_pipeline_video);
		context->state.blit_pipeline_flag = 0;
	}

	if (context->state.buffer_ptr_alloc) {
		free(context->state.buffer_ptr_alloc);
		context->state.buffer_ptr_alloc = 0;
	}
}

static void video_invalidate_pipeline(struct advance_video_context* context)
{
	/* destroy the pipeline, this force the pipeline update at the next frame */
	video_done_pipeline(context);
}

/**
 * Set the video mode.
 * The mode is copyed in the context if it is set with success.
 * \return 0 on success
 */
static adv_error video_init_vidmode(struct advance_video_context* context, adv_mode* mode)
{
	union adv_color_def_union def;

	assert( !context->state.mode_flag );

	if (video_mode_set(mode) != 0) {
		log_std(("ERROR:emu:video: calling video_mode_set() '%s'\n", error_get()));
		return -1;
	}

	def.ordinal = video_color_def();

	log_std(("emu:video: color %s, bytes_per_pixel %d\n", color_def_name_get(def.ordinal), color_def_bytes_per_pixel_get(def.ordinal)));

	/* save the video mode */
	context->state.mode_flag = 1;
	context->state.mode = *mode;

	/* initialize the blit pipeline */
	context->state.blit_pipeline_flag = 0;
	context->state.buffer_ptr_alloc = 0;

	/* initialize the update system */
	update_init( context->config.triplebuf_flag != 0 ? 3 : 1 );

	log_std(("emu:video: using %d hardware video buffers\n", update_page_max_get()));

	video_invalidate_screen();
	video_invalidate_color(context);

	log_std(("emu:video: mode %s, size %dx%d, bits_per_pixel %d, bytes_per_scanline %d, pages %d\n", video_name(), video_size_x(), video_size_y(), video_bits_per_pixel(), video_bytes_per_scanline(), update_page_max_get()));

	return 0;
}

static void video_done_vidmode(struct advance_video_context* context, adv_bool restore)
{
	assert( context->state.mode_flag );

	/* clear all the video memory used */
	if (restore)
		video_invalidate_screen();

	video_done_pipeline(context);

	update_done();

	context->state.mode_flag = 0;
}

static adv_error video_update_vidmode(struct advance_video_context* context, adv_mode* mode, adv_bool ignore_key)
{
	video_invalidate_pipeline(context);

	if (!context->state.mode_flag
		|| video_mode_compare(mode, video_current_mode())!=0
	) {
		if (context->state.mode_flag)
			video_done_vidmode(context, 0);

		if (!ignore_key) {
			keyb_disable();
		}

		if (video_init_vidmode(context, mode) != 0) {
			return -1;
		}

		if (!ignore_key) {
			if (keyb_enable(1) != 0) {
				log_std(("ERROR:emu:video: calling keyb_enable() '%s'\n", error_get()));
				return -1;
			}
		}
	}

	return 0;
}

/**
 * Update the panning state.
 * \precondition The video configuration in the context must be already initialized.
 */
static void video_update_pan(struct advance_video_context* context)
{
	unsigned pos_x;
	unsigned pos_y;
	unsigned size_x;
	unsigned size_y;

	/* center if skips are negative */
	if (context->config.skipcolumns<0)
		context->state.game_visible_pos_x = (context->state.game_used_size_x - context->state.game_visible_size_x) / 2;
	else
		context->state.game_visible_pos_x = context->config.skipcolumns;
	if (context->config.skiplines<0)
		context->state.game_visible_pos_y = (context->state.game_used_size_y - context->state.game_visible_size_y) / 2;
	else
		context->state.game_visible_pos_y = context->config.skiplines;

	/* failsafe against silly parameters */
	if (context->state.game_visible_pos_x < 0) {
		context->state.game_visible_pos_x = 0;
	}
	if (context->state.game_visible_pos_y < 0) {
		context->state.game_visible_pos_y = 0;
	}
	if (context->state.game_visible_pos_x + context->state.game_visible_size_x > context->state.game_used_size_x) {
		context->state.game_visible_pos_x = context->state.game_used_size_x - context->state.game_visible_size_x;
	}
	if (context->state.game_visible_pos_y + context->state.game_visible_size_y > context->state.game_used_size_y) {
		context->state.game_visible_pos_y = context->state.game_used_size_y - context->state.game_visible_size_y;
	}

	log_std(("emu:video: game_visible_pos_x %d\n", context->state.game_visible_pos_x));
	log_std(("emu:video: game_visible_pos_y %d\n", context->state.game_visible_pos_y));
	log_std(("emu:video: game_visible_size_x %d\n", context->state.game_visible_size_x));
	log_std(("emu:video: game_visible_size_y %d\n", context->state.game_visible_size_y));

	/* configure the MAME UI also if it isn't really used */

	pos_x = context->state.game_visible_pos_x;
	pos_y = context->state.game_visible_pos_y;
	size_x = context->state.game_visible_size_x;
	size_y = context->state.game_visible_size_y;

	/* restore the original orientation for the MAME core */
	if (context->config.blit_orientation & OSD_ORIENTATION_FLIP_X) {
		pos_x = context->state.game_used_size_x - size_x - pos_x;
	}

	if (context->config.blit_orientation & OSD_ORIENTATION_FLIP_Y) {
		pos_y = context->state.game_used_size_y - size_y - pos_y;
	}

	pos_x += context->state.game_used_pos_x;
	pos_y += context->state.game_used_pos_y;

	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(unsigned, pos_x, pos_y);
		SWAP(unsigned, size_x, size_y);
	}

	log_std(("emu:video: mame_ui_area_set(min_x:%d, min_y:%d, max_x:%d, max_y:%d)\n", pos_x, pos_y, pos_x+size_x-1, pos_y+size_y-1));

	mame_ui_area_set(pos_x, pos_y, pos_x+size_x-1, pos_y+size_y-1);
}

static void video_update_ui(struct advance_video_context* context, const adv_crtc* crtc)
{
	unsigned aspect_x;
	unsigned aspect_y;

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW) != 0) {
		aspect_x = 1;
		aspect_y = 1;
	} else {
		aspect_x = context->config.monitor_aspect_x;
		aspect_y = context->config.monitor_aspect_y;
	}

	advance_ui_changefont(&CONTEXT.ui, crtc_hsize_get(crtc), crtc_vsize_get(crtc), aspect_x, aspect_y);
}

/**
 * Update the effects state.
 * \precondition The video configuration in the context must be already initialized.
 * \precondition The depth in the context must be already initialized
 */
static void video_update_effect(struct advance_video_context* context)
{
	double previous_gamma_factor;

	context->state.rgb_effect = context->config.rgb_effect;
	context->state.interlace_effect = context->config.interlace_effect;
	context->state.combine = context->config.combine;

	if (context->state.combine == COMBINE_AUTO) {
		if (context->state.mode_visible_size_x == 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 2*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_SCALE;
		} else if (context->state.mode_visible_size_x == 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 3*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_SCALE;
		} else if (context->state.mode_visible_size_x == 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 4*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_SCALE;
		} else if (context->state.mode_visible_size_x == 3*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 3*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_SCALE;
		} else if (context->state.mode_visible_size_x == 4*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 4*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_SCALE;
		} else if (context->state.mode_visible_size_x >= 3*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y >= 3*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_FILTER;
		} else if ((context->state.mode_visible_size_x % context->state.game_visible_size_x) == 0
			&& (context->state.mode_visible_size_y % context->state.game_visible_size_y) == 0) {
			context->state.combine = COMBINE_NONE;
		} else if (context->config.inlist_combinemax_flag) {
			context->state.combine = COMBINE_MAXMIN;
		} else {
			context->state.combine = COMBINE_MEAN;
		}
	}

	if (context->state.combine == COMBINE_MEAN
		|| context->state.combine == COMBINE_FILTER
	) {
		switch (context->state.mode_index) {
		case MODE_FLAGS_INDEX_BGR8 :
		case MODE_FLAGS_INDEX_BGR15 :
		case MODE_FLAGS_INDEX_BGR16 :
		case MODE_FLAGS_INDEX_BGR32 :
			break;
		default:
			log_std(("emu:video: resizeeffect=* disabled because we aren't in a rgb mode\n"));
			context->state.combine = COMBINE_NONE;
			break;
		}
	}

	if ((context->state.combine == COMBINE_SCALE)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 2*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 3*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 4*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
	) {
		log_std(("emu:video: resizeeffect=scale disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	if ((context->state.combine == COMBINE_LQ || context->state.combine == COMBINE_HQ)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 2*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 3*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 4*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
	) {
		log_std(("emu:video: resizeeffect=lq|hq disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	if ((context->state.combine == COMBINE_HQ || context->state.combine == COMBINE_LQ)
		&& (context->state.mode_index != MODE_FLAGS_INDEX_BGR32 && context->state.mode_index != MODE_FLAGS_INDEX_BGR16 && context->state.mode_index != MODE_FLAGS_INDEX_BGR15)) {
		log_std(("emu:video: resizeeffect=lq|hq disabled because is supported only in 15/16/32 bit modes\n"));
		context->state.combine = COMBINE_NONE;
	}

	/* max only in not integer change */
	if (context->state.combine == COMBINE_MAXMIN
		&& context->state.mode_visible_size_y % context->state.game_visible_size_y == 0
		&& context->state.mode_visible_size_x % context->state.game_visible_size_x == 0
	) {
		log_std(("emu:video: resizeeffect=max disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	/* mean only in change */
	if (context->state.combine == COMBINE_MEAN
		&& context->state.mode_visible_size_y == context->state.game_visible_size_y
		&& context->state.mode_visible_size_x == context->state.game_visible_size_x
	) {
		log_std(("emu:video: resizeeffect=mean disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	if (context->state.rgb_effect != EFFECT_NONE) {
		switch (context->state.mode_index) {
		case MODE_FLAGS_INDEX_BGR8 :
		case MODE_FLAGS_INDEX_BGR15 :
		case MODE_FLAGS_INDEX_BGR16 :
		case MODE_FLAGS_INDEX_BGR32 :
			break;
		default:
			log_std(("emu:video: rgbeffect=* disabled because we aren't in a rgb mode\n"));
			context->state.rgb_effect = EFFECT_NONE;
		}
	}

	previous_gamma_factor = context->state.gamma_effect_factor;

	/* adjust the gamma settings */
	switch (context->state.rgb_effect) {
		case EFFECT_NONE :
			context->state.gamma_effect_factor = 1.0;
			break;
		case EFFECT_RGB_TRIAD3PIX :
		case EFFECT_RGB_TRIAD6PIX :
		case EFFECT_RGB_TRIAD16PIX :
		case EFFECT_RGB_SCANDOUBLEHORZ :
		case EFFECT_RGB_SCANDOUBLEVERT :
			context->state.gamma_effect_factor = 1.2;
			break;
		case EFFECT_RGB_TRIADSTRONG3PIX :
		case EFFECT_RGB_TRIADSTRONG6PIX :
		case EFFECT_RGB_TRIADSTRONG16PIX :
		case EFFECT_RGB_SCANTRIPLEHORZ :
		case EFFECT_RGB_SCANTRIPLEVERT :
			context->state.gamma_effect_factor = 1.3;
			break;
	}

	if (!crtc_is_interlace(&context->state.crtc_effective)) {
		context->state.interlace_effect = EFFECT_NONE;
	}

	mame_ui_gamma_factor_set( context->state.gamma_effect_factor / previous_gamma_factor );
}

/**
 * Check if a modeline is acceptable.
 * The complete modeline processing is done. This ensure that later the
 * mode setting will not fail.
 * \return
 *  - !=0 ok
 *  - ==0 error
 */
static adv_bool is_crtc_acceptable(struct advance_video_context* context, const adv_crtc* crtc)
{
	adv_mode mode;
	adv_crtc temp_crtc;
	unsigned flags[] = { MODE_FLAGS_INDEX_PALETTE8, MODE_FLAGS_INDEX_BGR8, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_YUY2, 0 };
	unsigned* i;

	mode_reset(&mode);

	/* try to adjust the crtc if required */
	if (video_make_crtc(context, &temp_crtc, crtc) != 0)
		return 0;

	i = flags;
	while (*i) {
		if (video_mode_generate(&mode, &temp_crtc, *i)==0) {
			return 1;
		}
		++i;
	}

	/* generally this fail due the limitation of the video drivers */
	/* for example the vgaline drivers accepts only some clocks */
	return 0;
}

/**
 * Check if a modeline is acceptable.
 * Only a partial processing is done.
 * This function guess the result of the is_crtc_acceptable function without
 * using any game information.
 * \return
 *  - !=0 ok
 *  - ==0 error
 */
static adv_bool is_crtc_acceptable_preventive(struct advance_video_context* context, const adv_crtc* crtc)
{
	adv_mode mode;
	adv_crtc temp_crtc = *crtc;
	unsigned flags[] = { MODE_FLAGS_INDEX_PALETTE8, MODE_FLAGS_INDEX_BGR8, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_YUY2, 0 };
	unsigned* i;

	mode_reset(&mode);

	if (video_is_programmable(context)) {

		/* adjust the clock if possible */
		if ((context->config.adjust & ADJUST_ADJUST_CLOCK) != 0) {
			if (!crtc_clock_check(&context->config.monitor, &temp_crtc)) {
				if (crtc_adjust_clock(&temp_crtc, &context->config.monitor) != 0) {
					/* ignore error */
				}
			}
		}

		/* final check on the monitor range */
		if (!crtc_clock_check(&context->config.monitor, &temp_crtc)) {
			return 0;
		}
	}

	/* try generating the video mode */
	i = flags;
	while (*i) {
		if (video_mode_generate(&mode, &temp_crtc, *i)==0) {
			return 1;
		}
		++i;
	}

	/* generally this fail due the limitation of the video drivers */
	/* for example the vgaline drivers accepts only some clocks */
	return 0;
}

static void video_update_visible(struct advance_video_context* context, const adv_crtc* crtc)
{
	unsigned stretch;

	assert( crtc );

	stretch = context->config.stretch;

	if ((context->config.adjust & (ADJUST_ADJUST_X | ADJUST_GENERATE)) != 0
		&& video_is_programmable(context)
	) {
		if (stretch == STRETCH_FRACTIONAL_XY
			&& strcmp(context->config.resolution_buffer, "auto")==0) {
			log_std(("emu:video: fractional converted in mixed because generate/x is active\n"));
			stretch = STRETCH_INTEGER_X_FRACTIONAL_Y;
		}
	}

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)!=0) {
		/* only for window */
		context->state.mode_visible_size_x = crtc_hsize_get(crtc);
		context->state.mode_visible_size_y = crtc_vsize_get(crtc);
	} else {
		unsigned long long factor_x;
		unsigned long long factor_y;
		unsigned long long mode_pixelaspect_x;
		unsigned long long mode_pixelaspect_y;
		unsigned long long arcade_aspect_x;
		unsigned long long arcade_aspect_y;

/*
	mode_pixelaspect_x/y
		It's the size in pixel of a visible square drawn on the current
		video mode.

	monitor_aspect_x   mode_pixelaspect_x   mode_size_x
	---------------- * ------------------ = -----------
	monitor_aspect_y   mode_pixelaspect_y   mode_size_y

	if mode_pixelaspect is equal at game_pixelaspect no stretching is
	required to adjust the aspect.
*/
		mode_pixelaspect_x = crtc_hsize_get(crtc) * context->config.monitor_aspect_y;
		mode_pixelaspect_y = crtc_vsize_get(crtc) * context->config.monitor_aspect_x;
		video_aspect_reduce(&mode_pixelaspect_x, &mode_pixelaspect_y);

		arcade_aspect_x = context->state.game_used_size_x * context->state.game_pixelaspect_y;
		arcade_aspect_y = context->state.game_used_size_y * context->state.game_pixelaspect_x;
		video_aspect_reduce(&arcade_aspect_x, &arcade_aspect_y);

/*
	The visible/used part of the video mode is computed in this way :

	arcade_aspect_x   mode_pixelaspect_x   mode_visible_size_x
	--------------- * ------------------ = -------------------
	arcade_aspect_y   mode_pixelaspect_y   mode_visible_size_y

	Note that mode_pixelaspect doesn't change if you use only part of
	the screen. So, we can change safely the visible size.
*/

		factor_x = mode_pixelaspect_x * arcade_aspect_x;
		factor_y = mode_pixelaspect_y * arcade_aspect_y;
		video_aspect_reduce(&factor_x, &factor_y);

		/* compute screen_visible size */
		if (context->config.monitor_aspect_x * arcade_aspect_y > arcade_aspect_x * context->config.monitor_aspect_y) {
			/* vertical game in horizontal screen */
			context->state.mode_visible_size_y = crtc_vsize_get(crtc);
			/* adjust to 8 pixel */
			context->state.mode_visible_size_x = crtc_step( (double)context->state.mode_visible_size_y * factor_x / factor_y, 8);
			if (context->state.mode_visible_size_x > crtc_hsize_get(crtc))
				context->state.mode_visible_size_x = crtc_hsize_get(crtc);
		} else {
			/* horizontal game in vertical screen */
			context->state.mode_visible_size_x = crtc_hsize_get(crtc);
			context->state.mode_visible_size_y = context->state.mode_visible_size_x * factor_y / factor_x;
			if (context->state.mode_visible_size_y > crtc_vsize_get(crtc))
					context->state.mode_visible_size_y = crtc_vsize_get(crtc);
		}
	}

	if (stretch == STRETCH_FRACTIONAL_XY) {
		context->state.game_visible_size_x = context->state.game_used_size_x;
		context->state.game_visible_size_y = context->state.game_used_size_y;

		/* reject fractional for very small adjustement */
		if (context->state.game_used_size_x <= crtc_hsize_get(crtc)
			&& abs(context->state.mode_visible_size_x - context->state.game_used_size_x) <= 8) {
			context->state.mode_visible_size_x = context->state.game_used_size_x;
		}
		if (context->state.game_used_size_y <= crtc_vsize_get(crtc)
			&& abs(context->state.mode_visible_size_y - context->state.game_used_size_y) <= 8) {
			context->state.mode_visible_size_y = context->state.game_used_size_y;
		}
	} else if (stretch == STRETCH_INTEGER_X_FRACTIONAL_Y) {
		unsigned mx;
		mx = floor(context->state.mode_visible_size_x / (double)context->state.game_used_size_x + 0.5);
		if (mx < 1)
			mx = 1;

		if (mx * context->state.game_used_size_x > crtc_hsize_get(crtc)) {
			context->state.game_visible_size_x = crtc_hsize_get(crtc) / mx;
			context->state.mode_visible_size_x = mx * context->state.game_visible_size_x;
		} else {
			context->state.game_visible_size_x = context->state.game_used_size_x;
			context->state.mode_visible_size_x = mx * context->state.game_visible_size_x;
		}

		context->state.game_visible_size_y = context->state.game_used_size_y;
	} else if (stretch == STRETCH_INTEGER_XY) {
		unsigned mx;
		unsigned my;

		mx = floor(context->state.mode_visible_size_x / (double)context->state.game_used_size_x + 0.5);
		if (mx < 1)
			mx = 1;

		my = floor(context->state.mode_visible_size_y / (double)context->state.game_used_size_y + 0.5);
		if (my < 1)
			my = 1;

		if (mx * context->state.game_used_size_x > crtc_hsize_get(crtc)) {
			context->state.game_visible_size_x = crtc_hsize_get(crtc) / mx;
			context->state.mode_visible_size_x = mx * context->state.game_visible_size_x;
		} else {
			context->state.game_visible_size_x = context->state.game_used_size_x;
			context->state.mode_visible_size_x = mx * context->state.game_visible_size_x;
		}

		if (my * context->state.game_used_size_y > crtc_vsize_get(crtc)) {
			context->state.game_visible_size_y = crtc_vsize_get(crtc) / my;
			context->state.mode_visible_size_y = my * context->state.game_visible_size_y;
		} else {
			context->state.game_visible_size_y = context->state.game_used_size_y;
			context->state.mode_visible_size_y = my * context->state.game_visible_size_y;
		}
	} else {
		if (context->state.game_used_size_x > crtc_hsize_get(crtc))
			context->state.mode_visible_size_x = crtc_vsize_get(crtc);
		else
			context->state.mode_visible_size_x = context->state.game_used_size_x;
		context->state.game_visible_size_x = context->state.mode_visible_size_x;

		if (context->state.game_used_size_y > crtc_vsize_get(crtc))
			context->state.mode_visible_size_y = crtc_vsize_get(crtc);
		else
			context->state.mode_visible_size_y = context->state.game_used_size_y;
		context->state.game_visible_size_y = context->state.mode_visible_size_y;
	}
}

/**
 * Compare the resolution name.
 * \return ==0 if equal
 */
static int video_resolution_cmp(const char* resolution, const char* name)
{
	int rl;
	int nl;

	/* match the exact mode name */
	if (strcmp(resolution, name)==0)
		return 0;

	/* LEGACY (to be removed) */
	/* match the old video configuration format "NAME-XxY-XxY" */
	rl = strlen(resolution);
	nl = strlen(name);
	if (rl > nl
		&& strncmp(resolution, name, nl)==0
		&& resolution[nl]=='-'
		&& strspn(resolution+nl, "-0123456789x")==strlen(resolution+nl))
		return 0;

	return -1;
}

/**
 * Select the current video configuration.
 */
static adv_error video_update_crtc(struct advance_video_context* context)
{
	adv_crtc_container_iterator i;
	const adv_crtc* crtc;
	int j;

	/* build the vector of config pointer */
	context->state.crtc_mac = 0;

	log_std(("emu:video: video mode selection\n"));

	for(crtc_container_iterator_begin(&i, &context->config.crtc_bag);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		adv_crtc* crtc = crtc_container_iterator_get(&i);
		if (is_crtc_acceptable(context, crtc)) {
			if (context->state.crtc_mac < VIDEO_CRTC_MAX) {
				char buffer[256];
				context->state.crtc_map[context->state.crtc_mac] = crtc;
				++context->state.crtc_mac;
				crtc_print(buffer, sizeof(buffer), crtc);
				log_std(("emu:video: accepted modeline:\"%s\"\n", buffer));
			} else {
				log_std(("ERROR:emu:video: too many modes\n"));
			}
		} else {
			char buffer[256];
			crtc_print(buffer, sizeof(buffer), crtc);
			log_std(("emu:video: excluded modeline:\"%s\"\n", buffer));
		}
	}

	log_std(("emu:video: %d video modes\n", context->state.crtc_mac));

	if (!context->state.crtc_mac) {
		return -1;
	}

	crtc_sort(context, context->state.crtc_map, context->state.crtc_mac);

	log_std(("emu:video: sorted list of video modes\n"));
	for(j=0;j<context->state.crtc_mac;++j) {
		char buffer[256];
		crtc_print(buffer, sizeof(buffer), context->state.crtc_map[j]);
		log_std(("emu:video: %3d modeline:\"%s\"\n", j, buffer));
	}

	crtc = 0;
	if (strcmp(context->config.resolution_buffer, "auto")!=0) {
		int i;
		for(i=0;i<context->state.crtc_mac;++i) {
			if (video_resolution_cmp(context->config.resolution_buffer, crtc_name_get(context->state.crtc_map[i])) == 0) {
				crtc = context->state.crtc_map[i];
				break;
			}
		}
	}

	if (crtc == 0) {
		/* the first mode is the best mode */
		crtc = context->state.crtc_map[0];
	}

	context->state.crtc_selected = crtc;

	{
		char buffer[256];
		crtc_print(buffer, sizeof(buffer), crtc);
		log_std(("advance:selected: modeline:\"%s\"\n", buffer));
	}

	return 0;
}

/***************************************************************************/
/* Initialization */

/**
 * Generate and add a crtc mode.
 */
adv_error video_init_crtc_generate(struct advance_video_context* context, adv_crtc* crtc, unsigned adjust, unsigned cap, unsigned size_x0, unsigned size_y0, unsigned size_x1, unsigned size_y1, unsigned size_x2, unsigned size_y2, unsigned size_x3, unsigned size_y3, double vclock)
{
	adv_error err = -1;

	/* try with a perfect mode */
	if (err != 0)
		err = generate_find_interpolate_multi(crtc, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_EXACT);
	if ((adjust & (ADJUST_ADJUST_CLOCK | ADJUST_ADJUST_Y)) == (ADJUST_ADJUST_CLOCK | ADJUST_ADJUST_Y)) {
		if ((adjust & ADJUST_FAVORITE_SIZE_OVER_CLOCK) != 0) {
			/* try with a mode with different vclock */
			if (err != 0)
				err = generate_find_interpolate_multi(crtc, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VCLOCK);
			/* try with a mode with different vtotal and different vclock */
			if (err != 0)
				err = generate_find_interpolate_multi(crtc, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VTOTAL | GENERATE_ADJUST_VCLOCK);
		} else {
			/* try with a mode with different vtotal */
			if (err != 0)
				err = generate_find_interpolate_multi(crtc, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VTOTAL);
			/* try with a mode with different vtotal and different vclock */
			if (err != 0)
				err = generate_find_interpolate_multi(crtc, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VTOTAL | GENERATE_ADJUST_VCLOCK);
		}
	} else if ((adjust & ADJUST_ADJUST_CLOCK) != 0) {
		/* try with a mode with different vclock */
		if (err != 0)
			err = generate_find_interpolate_multi(crtc, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VCLOCK);
	} else if ((adjust & ADJUST_ADJUST_Y) != 0) {
		/* try with a mode with different vtotal */
		if (err != 0)
			err = generate_find_interpolate_multi(crtc, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VTOTAL);
	}

	return err;
}

/**
 * Generate and add a crtc mode for a raster game.
 */
static const adv_crtc* video_init_crtc_make_raster(struct advance_video_context* context, const char* name, unsigned size_x0, unsigned size_y0, unsigned size_x1, unsigned size_y1, unsigned size_x2, unsigned size_y2, unsigned size_x3, unsigned size_y3, double vclock, adv_bool force_scanline, adv_bool force_interlace, adv_bool force_correct_size)
{
	char buffer[256];
	adv_crtc crtc;
	adv_error err;
	const adv_crtc* ret;

	unsigned adj = context->config.adjust;
	if (force_correct_size) {
		adj &= ~ADJUST_ADJUST_Y;
	}

	if (force_scanline) {
		/* use only single scanline modes */
		unsigned cap = video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & ~(VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE);
		err = video_init_crtc_generate(context, &crtc, adj, cap, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock);
	} else if (force_interlace) {
		/* use only interlace modes */
		unsigned cap = video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & ~(VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN);
		err = video_init_crtc_generate(context, &crtc, adj, cap, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock);
	} else {
		unsigned cap = video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0);
		err = video_init_crtc_generate(context, &crtc, adj, cap, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock);
	}

	if (err != 0) {
		log_std(("advance:generate: failed to generate mode %s\n", name));
		return 0;
	}

	if (!crtc_clock_check(&context->config.monitor, &crtc)) {
		log_std(("advance:generate: failed to generate a correct mode\n"));
		return 0;
	}

	crtc_name_set(&crtc, name);

	crtc_print(buffer, sizeof(buffer), &crtc);
	log_std(("advance:generate: modeline \"%s\"\n", buffer));

	ret = crtc_container_insert(&context->config.crtc_bag, &crtc);

	return ret;
}

/**
 * Crete and add a fake video mode.
 * The fake video modes are for exclusive use with video driver with a fake mode set,
 * i.e. drivers for a window manager. It never fails.
 */
static void video_init_crtc_make_fake(struct advance_video_context* context, const char* name, unsigned size_x, unsigned size_y)
{
	char buffer[256];

	adv_crtc crtc;
	crtc_fake_set(&crtc, size_x, size_y);

	crtc_name_set(&crtc, name);
	crtc_print(buffer, sizeof(buffer), &crtc);

	log_std(("advance:generate: fake \"%s\"\n", buffer));

	crtc_container_insert(&context->config.crtc_bag, &crtc);
}

/**
 * Generate and add a crtc mode for a vector game.
 */
static void video_init_crtc_make_vector(struct advance_video_context* context, const char* name, unsigned size_x0, unsigned size_y0, unsigned size_x1, unsigned size_y1, unsigned size_x2, unsigned size_y2, unsigned size_x3, unsigned size_y3, double vclock)
{
	char buffer[256];
	adv_crtc crtc;
	adv_error err;

	unsigned cap = video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0);
	err = video_init_crtc_generate(context, &crtc, context->config.adjust, cap, size_x0, size_y0, size_x1, size_y1, size_x2, size_y2, size_x3, size_y3, vclock);

	if (err != 0) {
		log_std(("advance:generate: failed to generate mode vector\n"));
		return;
	}

	if (!crtc_clock_check(&context->config.monitor, &crtc)) {
		log_std(("advance:generate: failed to generate a correct mode\n"));
		return;
	}

	/* adjust the horizontal size */
	size_y0 = crtc_vsize_get(&crtc);
	size_x0 = size_y0 * context->config.monitor_aspect_x / context->config.monitor_aspect_y;
	size_x0 &= ~0x7;

	crtc_hsize_set(&crtc, size_x0);

	crtc_name_set(&crtc, name);

	crtc_print(buffer, sizeof(buffer), &crtc);
	log_std(("advance:generate: modeline \"%s\"\n", buffer));

	crtc_container_insert(&context->config.crtc_bag, &crtc);
}

static unsigned best_step(unsigned value, unsigned step)
{
	if (value % step != 0)
		value = value + step - value % step;
	return value;
}

/**
 * Initialize the state.
 * \return 0 on success
 */
static adv_error video_init_state(struct advance_video_context* context, struct osd_video_option* req)
{
	unsigned best_size_x;
	unsigned best_size_y;
	unsigned best_size_2x;
	unsigned best_size_2y;
	unsigned best_size_3x;
	unsigned best_size_3y;
	unsigned best_size_4x;
	unsigned best_size_4y;
	unsigned best_bits;
	double best_vclock;
	unsigned long long arcade_aspect_x;
	unsigned long long arcade_aspect_y;

	context->state.pause_flag = 0;
	context->state.crtc_selected = 0;
	context->state.gamma_effect_factor = 1;
	context->state.menu_sub_flag = 0;
	context->state.menu_sub_selected = 0;

	context->state.frame_counter = 0;

	context->state.fastest_limit = 0; /* initialize the fastest frame counter */
	context->state.fastest_flag = 0; /* not active until the first reset call */

	context->state.measure_counter = 0; /* initialize the measure frame counter */
	context->state.measure_flag = 0; /* not active until the first reset call */
	context->state.measure_start = 0;
	context->state.measure_stop = 0;

	context->state.debugger_flag = 0;
	context->state.sync_throttle_flag = 1;

	context->state.game_bits_per_pixel = req->bits_per_pixel;
	context->state.game_bytes_per_pixel = (context->state.game_bits_per_pixel + 7) / 8;

	context->state.game_vector_flag = req->vector_flag;

	if (context->config.fps_fixed != 0) {
		context->state.game_fps = context->config.fps_fixed * context->config.fps_speed_factor;
	} else {
		context->state.game_fps = req->fps * context->config.fps_speed_factor;
	}
	context->state.game_area_size_x = req->area_size_x;
	context->state.game_area_size_y = req->area_size_y;
	context->state.game_used_pos_x = req->used_pos_x;
	context->state.game_used_pos_y = req->used_pos_y;
	context->state.game_used_size_x = req->used_size_x;
	context->state.game_used_size_y = req->used_size_y;
	arcade_aspect_x = req->aspect_x;
	arcade_aspect_y = req->aspect_y;

	/* set the correct blit orientation */
	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(unsigned, context->state.game_area_size_x, context->state.game_area_size_y );
		SWAP(unsigned, context->state.game_used_pos_x, context->state.game_used_pos_y );
		SWAP(unsigned, context->state.game_used_size_x, context->state.game_used_size_y );
		SWAP(unsigned, arcade_aspect_x, arcade_aspect_y );
	}

	context->state.game_rgb_flag = req->rgb_flag;
	context->state.game_color_def = req->color_def;
	if (context->state.game_rgb_flag) {
		context->state.game_colors = 0;
	} else {
		context->state.game_colors = req->colors;
	}

	log_std(("emu:video: generating video modes\n"));

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW) != 0) {
		best_size_x = context->state.game_used_size_x;
		best_size_y = context->state.game_used_size_y;
		best_size_2x = 2 * context->state.game_used_size_x;
		best_size_2y = 2 * context->state.game_used_size_y;
		best_size_3x = 3 * context->state.game_used_size_x;
		best_size_3y = 3 * context->state.game_used_size_y;
		best_size_4x = 4 * context->state.game_used_size_x;
		best_size_4y = 4 * context->state.game_used_size_y;
		best_bits = context->state.game_bits_per_pixel;
		best_vclock = context->state.game_fps;

		video_init_crtc_make_fake(context, "generate", best_size_x, best_size_y);
		video_init_crtc_make_fake(context, "generate-double", best_size_2x, best_size_2y);
		video_init_crtc_make_fake(context, "generate-triple", best_size_3x, best_size_3y);
		video_init_crtc_make_fake(context, "generate-quad", best_size_4x, best_size_4y);
	} else {
		unsigned long long factor_x;
		unsigned long long factor_y;
		unsigned step_x;

		/* if the clock is programmable the monitor specification must be present */
		if (video_is_programmable(context)) {
			if (monitor_is_empty(&context->config.monitor)) {
				target_err("Missing options `device_video_p/h/vclock'.\n");
				target_err("Please read the file `install.txt' and `device.txt'.\n");
				return -1;
			}
		}

		/* expand the arcade aspect ratio */
		if (arcade_aspect_y * context->config.monitor_aspect_x < context->config.monitor_aspect_y * arcade_aspect_x) {
			arcade_aspect_x *= 100;
			arcade_aspect_y *= 100 * context->config.aspect_expansion_factor;
			/* limit */
			if (arcade_aspect_y * context->config.monitor_aspect_x > context->config.monitor_aspect_y * arcade_aspect_x) {
				arcade_aspect_x = context->config.monitor_aspect_x;
				arcade_aspect_y = context->config.monitor_aspect_y;
			}
		} else {
			arcade_aspect_y *= 100;
			arcade_aspect_x *= 100 * context->config.aspect_expansion_factor;
			/* limit */
			if (arcade_aspect_y * context->config.monitor_aspect_x < context->config.monitor_aspect_y * arcade_aspect_x) {
				arcade_aspect_x = context->config.monitor_aspect_x;
				arcade_aspect_y = context->config.monitor_aspect_y;
			}
		}

		video_aspect_reduce(&arcade_aspect_x, &arcade_aspect_y);

/*
	arcade_aspect_ratio
		The aspect of the original arcade game.

	game_pixelaspect
		It's the size in pixel of a visible square drawn on the original
		arcade game.

	arcade_aspect_x   game_pixelaspect_x   game_used_size_x
	--------------- * ------------------ = ----------------
	arcade_aspect_y   game_pixelaspect_y   game_used_size_y

	The mode size is computed in the this way :

	monitor_aspect_x   game_pixelaspect_x   mode_size_x
	---------------- * ------------------ = ------------
	monitor_aspect_y   game_pixelaspect_y   mode_size_y

	This formula ensures that the current video mode has the same
	pixelaspect of the original video mode.
*/

		/* compute the game aspect ratio */
		context->state.game_pixelaspect_x = context->state.game_used_size_x * arcade_aspect_y;
		context->state.game_pixelaspect_y = context->state.game_used_size_y * arcade_aspect_x;
		video_aspect_reduce(&context->state.game_pixelaspect_x, &context->state.game_pixelaspect_y);

		factor_x = context->config.monitor_aspect_x * context->state.game_pixelaspect_x;
		factor_y = context->config.monitor_aspect_y * context->state.game_pixelaspect_y;
		video_aspect_reduce(&factor_x, &factor_y);

		log_std(("emu:video: best aspect factor %dx%d (expansion %g)\n", (unsigned)factor_x, (unsigned)factor_y, (double)context->config.aspect_expansion_factor));

		/* Some video cards require a size multiple of 16. */
		/* (for example GeForce in doublescan with 8 bit modes) */
		step_x = 16;

		/* compute the best mode */
		if (context->config.monitor_aspect_x * arcade_aspect_y > arcade_aspect_x * context->config.monitor_aspect_y) {
			best_size_y = context->state.game_used_size_y;
			best_size_x = best_step((double)context->state.game_used_size_y * factor_x / factor_y, step_x);
			best_size_2y = 2*context->state.game_used_size_y;
			best_size_2x = best_step((double)2*context->state.game_used_size_y * factor_x / factor_y, step_x);
			best_size_3y = 3*context->state.game_used_size_y;
			best_size_3x = best_step((double)3*context->state.game_used_size_y * factor_x / factor_y, step_x);
			best_size_4y = 4*context->state.game_used_size_y;
			best_size_4x = best_step((double)4*context->state.game_used_size_y * factor_x / factor_y, step_x);
		} else {
			best_size_x = best_step(context->state.game_used_size_x, step_x);
			best_size_y = context->state.game_used_size_x * factor_y / factor_x;
			best_size_2x = best_step(2*context->state.game_used_size_x, step_x);
			best_size_2y = 2*context->state.game_used_size_x * factor_y / factor_x;
			best_size_3x = best_step(3*context->state.game_used_size_x, step_x);
			best_size_3y = 3*context->state.game_used_size_x * factor_y / factor_x;
			best_size_4x = best_step(4*context->state.game_used_size_x, step_x);
			best_size_4y = 4*context->state.game_used_size_x * factor_y / factor_x;
		}
		best_bits = context->state.game_bits_per_pixel;
		best_vclock = context->state.game_fps;

		if (!context->state.game_vector_flag) { /* nonsense for vector games */
			if (video_is_generable(context)) {
				/* generate modes for a programmable driver */
				const adv_crtc* crtc;
				crtc = video_init_crtc_make_raster(context, "generate", best_size_x, best_size_y, best_size_2x, best_size_2y, best_size_3x, best_size_3y, best_size_4x, best_size_4y, best_vclock, 0, 0, 0);
				if (context->config.scanlines_flag) {
					if (!crtc || !crtc_is_singlescan(crtc))
						video_init_crtc_make_raster(context, "generate-scanline", best_size_x, best_size_y, best_size_2x, best_size_2y, best_size_3x, best_size_3y, best_size_4x, best_size_4y, best_vclock, 1, 0, 0);
				}
				if (!crtc || !crtc_is_interlace(crtc))
					video_init_crtc_make_raster(context, "generate-interlace", best_size_x, best_size_y, best_size_2x, best_size_2y, best_size_3x, best_size_3y, best_size_4x, best_size_4y, best_vclock, 0, 1, 0);
				crtc = video_init_crtc_make_raster(context, "generate-double", best_size_2x, best_size_2y, best_size_3x, best_size_3y, best_size_4x, best_size_4y, 0, 0, best_vclock, 0, 0, 1);
				if (context->config.scanlines_flag) {
					if (!crtc || !crtc_is_singlescan(crtc))
						video_init_crtc_make_raster(context, "generate-double-scanline", best_size_2x, best_size_2y, best_size_3x, best_size_3y, best_size_4x, best_size_4y, 0, 0, best_vclock, 1, 0, 1);
				}
				if (!crtc || !crtc_is_interlace(crtc))
					video_init_crtc_make_raster(context, "generate-double-interlace", best_size_2x, best_size_2y, best_size_3x, best_size_3y, best_size_4x, best_size_4y, 0, 0, best_vclock, 0, 1, 1);
				video_init_crtc_make_raster(context, "generate-triple", best_size_3x, best_size_3y, 0, 0, 0, 0, 0, 0, best_vclock, 0, 0, 1);
				video_init_crtc_make_raster(context, "generate-quad", best_size_4x, best_size_4y, 0, 0, 0, 0, 0, 0, best_vclock, 0, 0, 1);
			}
			if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY) != 0) {
				/* generate modes for a zoom driver */
				video_init_crtc_make_fake(context, "generate", best_size_x, best_size_y);
				video_init_crtc_make_fake(context, "generate-double", best_size_2x, best_size_2y);
				video_init_crtc_make_fake(context, "generate-triple", best_size_3x, best_size_3y);
				video_init_crtc_make_fake(context, "generate-quad", best_size_4x, best_size_4y);
			}
		} else {
			if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY) != 0) {
				/* generate modes for a zoom driver */
				video_init_crtc_make_fake(context, "generate", best_size_x, best_size_y);
			}
		}
	}

	log_std(("emu:video: best mode %dx%d, mode2x %dx%d, mode3x %dx%d, bits_per_pixel %d, vclock %g\n", best_size_x, best_size_y, best_size_2x, best_size_2y, best_size_3x, best_size_3y, best_bits, (double)best_vclock));

	if (context->state.game_vector_flag
		&& context->config.stretch != STRETCH_NONE
	) {
		log_std(("emu:video: stretch disabled because it's a vector game\n"));
		context->config.stretch = STRETCH_NONE;
	}

	context->state.mode_best_size_x = best_size_x;
	context->state.mode_best_size_y = best_size_y;
	context->state.mode_best_size_2x = best_size_2x;
	context->state.mode_best_size_2y = best_size_2y;
	context->state.mode_best_size_3x = best_size_3x;
	context->state.mode_best_size_3y = best_size_3y;
	context->state.mode_best_size_4x = best_size_4x;
	context->state.mode_best_size_4y = best_size_4y;
	context->state.mode_best_vclock = best_vclock;

	log_std(("emu:video: game_area_size_x %d\n", (unsigned)context->state.game_area_size_x));
	log_std(("emu:video: game_area_size_y %d\n", (unsigned)context->state.game_area_size_y));
	log_std(("emu:video: game_used_pos_x %d\n", (unsigned)context->state.game_used_pos_x));
	log_std(("emu:video: game_used_pos_y %d\n", (unsigned)context->state.game_used_pos_y));
	log_std(("emu:video: game_used_size_x %d\n", (unsigned)context->state.game_used_size_x));
	log_std(("emu:video: game_used_size_y %d\n", (unsigned)context->state.game_used_size_y));
	log_std(("emu:video: game_aspect_x %d\n", (unsigned)context->state.game_pixelaspect_x));
	log_std(("emu:video: game_aspect_y %d\n", (unsigned)context->state.game_pixelaspect_y));

	return 0;
}

static void video_done_state(struct advance_video_context* context)
{
}

/**
 * Initialize the color mode
 * \return 0 on success
 */
static adv_error video_init_color(struct advance_video_context* context, struct osd_video_option* req)
{
	unsigned colors;
	unsigned i;

	/* number of colors */
	if (context->state.game_rgb_flag) {
		colors = 0;
	} else {
		colors = context->state.game_colors;
	}

	context->state.palette_total = colors;
	context->state.palette_dirty_total = (context->state.palette_total + osd_mask_size - 1) / osd_mask_size;
	if (context->state.palette_total % osd_mask_size == 0) {
		context->state.palette_dirty_mask = osd_mask_full;
	} else {
		unsigned rest = context->state.palette_total % osd_mask_size;
		context->state.palette_dirty_mask = (1U << rest) - 1;
	}

	log_std(("emu:video: palette_total %d\n", context->state.palette_total));
	log_std(("emu:video: palette_dirty_total %d\n", context->state.palette_dirty_total));
	log_std(("emu:video: palette_dirty_mask %08x\n", context->state.palette_dirty_mask));

	context->state.palette_dirty_map = (osd_mask_t*)malloc(context->state.palette_dirty_total * sizeof(osd_mask_t));
	context->state.palette_map = (adv_color_rgb*)malloc(context->state.palette_total * sizeof(osd_rgb_t));

	/* create the software palette */
	/* it will not be used if a hardware palette is present, but a runtime mode change may require it */
	context->state.palette_index32_map = (uint32*)malloc(context->state.palette_total * sizeof(uint32));
	context->state.palette_index16_map = (uint16*)malloc(context->state.palette_total * sizeof(uint16));
	context->state.palette_index8_map = (uint8*)malloc(context->state.palette_total * sizeof(uint8));

	/* initialize the palette */
	for(i=0;i<context->state.palette_total;++i) {
		context->state.palette_map[i].red = 0;
		context->state.palette_map[i].green = 0;
		context->state.palette_map[i].blue = 0;
	}
	for(i=0;i<context->state.palette_total;++i) {
		context->state.palette_index32_map[i] = 0;
		context->state.palette_index16_map[i] = 0;
		context->state.palette_index8_map[i] = 0;
	}

	/* make the palette completly dirty */
	context->state.palette_dirty_flag = 1;
	for(i=0;i<context->state.palette_dirty_total;++i)
		context->state.palette_dirty_map[i] = osd_mask_full;

	/* set the rgb format for rgb games */
	if (context->state.game_rgb_flag && req->rgb_components) {
		req->rgb_components[0] = pixel_make_from_def(0xFF, 0x00, 0x00, context->state.game_color_def);
		req->rgb_components[1] = pixel_make_from_def(0x00, 0xFF, 0x00, context->state.game_color_def);
		req->rgb_components[2] = pixel_make_from_def(0x00, 0x00, 0xFF, context->state.game_color_def);
	}

	return 0;
}

/**
 * Deinitialize the color mode.
 */
static void video_done_color(struct advance_video_context* context)
{
	free(context->state.palette_dirty_map);
	context->state.palette_dirty_map = 0;
	free(context->state.palette_map);
	context->state.palette_map = 0;
	free(context->state.palette_index32_map);
	context->state.palette_index32_map = 0;
	free(context->state.palette_index16_map);
	context->state.palette_index16_map = 0;
	free(context->state.palette_index8_map);
	context->state.palette_index8_map = 0;
}

/***************************************************************************/
/* Frame */

static void video_frame_resolution(struct advance_video_context* context, unsigned input)
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
		advance_video_change(context, &config);
	}

	if (show) {
		advance_global_message(&CONTEXT.global, "%s", context->config.resolution_buffer);
	}
}

static void video_frame_pan(struct advance_video_context* context, unsigned input)
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
		video_update_pan(context);
	}
}

static void video_buffer_clear(struct advance_video_context* context)
{
	adv_pixel color;
	unsigned bytes_per_pixel;
	unsigned x, y;

	assert( video_mode_is_active() );

	/* on palettized modes it always return 0 */
	color = video_pixel_get(0, 0, 0);

	bytes_per_pixel = video_bytes_per_pixel();

	/* clear */
	for(y=0;y<context->state.buffer_size_y;++y) {
		unsigned char* p = context->state.buffer_ptr + y * context->state.buffer_bytes_per_scanline;
		for(x=0;x<context->state.buffer_size_x;++x) {
			cpu_uint_write(p, bytes_per_pixel, color);
			p += bytes_per_pixel;
		}
	}
}

static void video_update_pipeline(struct advance_video_context* context, const struct osd_bitmap* bitmap)
{
	unsigned combine;
	unsigned combine_video;
	unsigned combine_buffer;
	unsigned size;
	int intermediate_game_used_pos_x;
	int intermediate_game_used_pos_y;
	int intermediate_game_used_size_x;
	int intermediate_game_used_size_y;
	int intermediate_game_visible_size_x;
	int intermediate_game_visible_size_y;
	int intermediate_mode_visible_size_x;
	int intermediate_mode_visible_size_y;

	/* check if the pipeline is already updated */
	if (context->state.blit_pipeline_flag)
		return;

	assert( *mode_name(&context->state.mode) );

	/* source for direct blit */
	context->state.blit_src_dw = bitmap->bytes_per_scanline;
	context->state.blit_src_dp = context->state.game_bytes_per_pixel;

	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(int, context->state.blit_src_dw, context->state.blit_src_dp);
	}

	context->state.blit_src_offset = context->state.game_used_pos_x * context->state.blit_src_dp + context->state.game_used_pos_y * context->state.blit_src_dw;

	if (context->config.blit_orientation & OSD_ORIENTATION_FLIP_Y) {
		context->state.blit_src_offset += (context->state.game_used_size_y - 1) * context->state.blit_src_dw;
		context->state.blit_src_dw = -context->state.blit_src_dw;
	}

	if (context->config.blit_orientation & OSD_ORIENTATION_FLIP_X) {
		context->state.blit_src_offset += (context->state.game_used_size_x - 1) * context->state.blit_src_dp;
		context->state.blit_src_dp = -context->state.blit_src_dp;
	}

	/* the game variable have the final orientation, it may differ from the */
	/* buffer intermediate orientation used with the bufferized blit */
	context->state.buffer_size_x = video_size_x();
	context->state.buffer_size_y = video_size_y();
	intermediate_game_used_pos_x = context->state.game_used_pos_x;
	intermediate_game_used_pos_y = context->state.game_used_pos_y;
	intermediate_game_used_size_x = context->state.game_used_size_x;
	intermediate_game_used_size_y = context->state.game_used_size_y;
	intermediate_game_visible_size_x = context->state.game_visible_size_x;
	intermediate_game_visible_size_y = context->state.game_visible_size_y;
	intermediate_mode_visible_size_x = context->state.mode_visible_size_x;
	intermediate_mode_visible_size_y = context->state.mode_visible_size_y;

	if (context->config.user_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(unsigned, context->state.buffer_size_x, context->state.buffer_size_y);
		SWAP(int, intermediate_game_used_pos_x, intermediate_game_used_pos_y);
		SWAP(int, intermediate_game_used_size_x, intermediate_game_used_size_y);
		SWAP(int, intermediate_game_visible_size_x, intermediate_game_visible_size_y);
		SWAP(int, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y);
	}

	context->state.buffer_src_dw = bitmap->bytes_per_scanline;
	context->state.buffer_src_dp = context->state.game_bytes_per_pixel;

	if (context->config.game_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(int, context->state.buffer_src_dw, context->state.buffer_src_dp);
	}

	context->state.buffer_src_offset = intermediate_game_used_pos_x * context->state.buffer_src_dp + intermediate_game_used_pos_y * context->state.buffer_src_dw;

	if (context->config.game_orientation & OSD_ORIENTATION_FLIP_Y) {
		context->state.buffer_src_offset += (intermediate_game_used_size_y - 1) * context->state.buffer_src_dw;
		context->state.buffer_src_dw = -context->state.buffer_src_dw;
	}

	if (context->config.game_orientation & OSD_ORIENTATION_FLIP_X) {
		context->state.buffer_src_offset += (intermediate_game_used_size_x - 1) * context->state.buffer_src_dp;
		context->state.buffer_src_dp = -context->state.buffer_src_dp;
	}

	/* check the copy alignment */
	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		/* alignement not requiried */
		context->state.game_visible_pos_x_increment = 1;
	} else {
		assert(ALIGN % abs(context->state.blit_src_dp) == 0);
		context->state.game_visible_pos_x_increment = ALIGN / abs(context->state.blit_src_dp);
		if (!context->state.game_visible_pos_x_increment) {
			context->state.game_visible_pos_x_increment = 1;
		}
	}

	/* adjust the source position */
	context->state.game_visible_pos_x = context->state.game_visible_pos_x - context->state.game_visible_pos_x % context->state.game_visible_pos_x_increment;

	combine = 0;

	switch (context->state.rgb_effect) {
	case EFFECT_RGB_TRIAD3PIX :
		combine |= VIDEO_COMBINE_X_RGB_TRIAD3PIX;
		break;
	case EFFECT_RGB_TRIADSTRONG3PIX :
		combine |= VIDEO_COMBINE_X_RGB_TRIADSTRONG3PIX;
		break;
	case EFFECT_RGB_TRIAD6PIX :
		combine |= VIDEO_COMBINE_X_RGB_TRIAD6PIX;
		break;
	case EFFECT_RGB_TRIADSTRONG6PIX :
		combine |= VIDEO_COMBINE_X_RGB_TRIADSTRONG6PIX;
		break;
	case EFFECT_RGB_TRIAD16PIX :
		combine |= VIDEO_COMBINE_X_RGB_TRIAD16PIX;
		break;
	case EFFECT_RGB_TRIADSTRONG16PIX :
		combine |= VIDEO_COMBINE_X_RGB_TRIADSTRONG16PIX;
		break;
	case EFFECT_RGB_SCANDOUBLEHORZ :
		combine |= VIDEO_COMBINE_X_RGB_SCANDOUBLEHORZ;
		break;
	case EFFECT_RGB_SCANTRIPLEHORZ :
		combine |= VIDEO_COMBINE_X_RGB_SCANTRIPLEHORZ;
		break;
	case EFFECT_RGB_SCANDOUBLEVERT :
		combine |= VIDEO_COMBINE_X_RGB_SCANDOUBLEVERT;
		break;
	case EFFECT_RGB_SCANTRIPLEVERT :
		combine |= VIDEO_COMBINE_X_RGB_SCANTRIPLEVERT;
		break;
	}

	switch (context->state.interlace_effect) {
	case EFFECT_INTERLACE_EVEN :
		combine |= VIDEO_COMBINE_SWAP_EVEN;
		break;
	case EFFECT_INTERLACE_ODD :
		combine |= VIDEO_COMBINE_SWAP_ODD;
		break;
	case EFFECT_INTERLACE_FILTER :
		combine |= VIDEO_COMBINE_INTERLACE_FILTER;
		break;
	}

	switch (context->state.combine) {
	case COMBINE_MAXMIN :
		combine |= VIDEO_COMBINE_Y_MAXMIN | VIDEO_COMBINE_X_MAXMIN;
		break;
	case COMBINE_MEAN :
		combine |= VIDEO_COMBINE_Y_MEAN | VIDEO_COMBINE_X_MEAN;
		break;
	case COMBINE_FILTER :
		combine |= VIDEO_COMBINE_Y_FILTER | VIDEO_COMBINE_X_FILTER;
		break;
	case COMBINE_SCALE :
		combine |= VIDEO_COMBINE_Y_SCALE;
		break;
	case COMBINE_LQ :
		combine |= VIDEO_COMBINE_Y_LQ;
		break;
	case COMBINE_HQ :
#ifndef USE_BLIT_SMALL
		combine |= VIDEO_COMBINE_Y_HQ;
#else
		combine |= VIDEO_COMBINE_Y_NONE;
#endif
		break;
	default:
		combine |= VIDEO_COMBINE_Y_NONE;
		break;
	}

	combine_buffer = combine;
	combine_video = combine;

	free(context->state.buffer_ptr_alloc);

	video_pipeline_init(&context->state.blit_pipeline_video);
	video_pipeline_init(&context->state.buffer_pipeline_video);

	context->state.buffer_bytes_per_scanline = context->state.buffer_size_x * video_bytes_per_pixel();

	/* align at 32 bytes */
	context->state.buffer_bytes_per_scanline = ALIGN_UNSIGNED(context->state.buffer_bytes_per_scanline, 32);

	size = 32 + context->state.buffer_size_y * context->state.buffer_bytes_per_scanline;
	context->state.buffer_ptr_alloc = malloc(size);

	/* align at 32 bytes */
	context->state.buffer_ptr = ALIGN_PTR(context->state.buffer_ptr_alloc, 32);

	/* clear */
	video_buffer_clear(context);

	video_pipeline_target(&context->state.buffer_pipeline_video, context->state.buffer_ptr, context->state.buffer_bytes_per_scanline, video_color_def());

	if (context->state.game_rgb_flag) {
		video_pipeline_direct(&context->state.blit_pipeline_video, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.game_color_def, combine_video);
		video_pipeline_direct(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, context->state.game_color_def, combine_buffer);
	} else {
		if (context->state.mode_index == MODE_FLAGS_INDEX_PALETTE8) {
			assert(context->state.game_bytes_per_pixel == 2);
			video_pipeline_palette16hw(&context->state.blit_pipeline_video, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, combine_video);
			video_pipeline_palette16hw(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, combine_buffer);
		} else {
			switch (context->state.game_bytes_per_pixel) {
				case 1 :
					video_pipeline_palette8(&context->state.blit_pipeline_video, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.palette_index8_map, context->state.palette_index16_map, context->state.palette_index32_map, combine_video);
					video_pipeline_palette8(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, context->state.palette_index8_map, context->state.palette_index16_map, context->state.palette_index32_map, combine_buffer);
					break;
				case 2 :
					video_pipeline_palette16(&context->state.blit_pipeline_video, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.palette_index8_map, context->state.palette_index16_map, context->state.palette_index32_map, combine_video);
					video_pipeline_palette16(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, context->state.palette_index8_map, context->state.palette_index16_map, context->state.palette_index32_map, combine_buffer);
					break;
				default :
					assert(0);
					break;
			}
		}
	}

	/* print the pipelines */
	{
		int i;
		const struct video_stage_horz_struct* stage;
		char buffer[256];

		log_std(("emu:video: pipeline scale from %dx%d to %dx%d\n", context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.mode_visible_size_x, context->state.mode_visible_size_y));

		log_std(("emu:video: pipeline_video\n"));
		for(i=1, stage=video_pipeline_begin(&context->state.blit_pipeline_video);stage!=video_pipeline_end(&context->state.blit_pipeline_video);++stage, ++i) {
			if (stage == video_pipeline_pivot(&context->state.blit_pipeline_video)) {
				snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.blit_pipeline_video)->type));
				++i;
				log_std(("emu:video: %s\n", buffer));
			}
			if (stage->sbpp != stage->sdp)
				snprintf(buffer, sizeof(buffer), "(%d) %s, p %d, dp %d", i, pipe_name(stage->type), stage->sbpp, stage->sdp);
			else
				snprintf(buffer, sizeof(buffer), "(%d) %s, p %d", i, pipe_name(stage->type), stage->sbpp);
			log_std(("emu:video: %s\n", buffer));
		}
		if (stage == video_pipeline_pivot(&context->state.blit_pipeline_video)) {
			snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.blit_pipeline_video)->type));
			++i;
			log_std(("emu:video: %s\n", buffer));
		}

		log_std(("emu:video: pipeline_buffer\n"));
		for(i=1, stage=video_pipeline_begin(&context->state.buffer_pipeline_video);stage!=video_pipeline_end(&context->state.buffer_pipeline_video);++stage, ++i) {
			if (stage == video_pipeline_pivot(&context->state.buffer_pipeline_video)) {
				snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.buffer_pipeline_video)->type));
				++i;
				log_std(("emu:video: %s\n", buffer));
			}
			if (stage->sbpp != stage->sdp)
				snprintf(buffer, sizeof(buffer), "(%d) %s, p %d, dp %d", i, pipe_name(stage->type), stage->sbpp, stage->sdp);
			else
				snprintf(buffer, sizeof(buffer), "(%d) %s, p %d", i, pipe_name(stage->type), stage->sbpp);
			log_std(("emu:video: %s\n", buffer));
		}
		if (stage == video_pipeline_pivot(&context->state.buffer_pipeline_video)) {
			snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.buffer_pipeline_video)->type));
			++i;
			log_std(("emu:video: %s\n", buffer));
		}
	}

	context->state.blit_pipeline_flag = 1;

	/* initialize the pipepeline measure */
	context->state.pipeline_measure_flag = 1;
	context->state.pipeline_measure_direct_mac = 0;
	context->state.pipeline_measure_buffer_mac = 0;
}

static void video_frame_put(struct advance_video_context* context, struct advance_ui_context* ui_context, const struct osd_bitmap* bitmap, unsigned x, unsigned y)
{
	unsigned src_offset;
	unsigned dst_x, dst_y;
	unsigned pixel;
	adv_bool ui_buffer_active;
	target_clock_t start;
	target_clock_t stop;
	adv_bool buffer_flag;

	/* screen position */

	dst_x = (video_size_x() - context->state.mode_visible_size_x) / 2;
	dst_y = (video_size_y() - context->state.mode_visible_size_y) / 2;

	pixel = ALIGN / video_bytes_per_pixel();
	dst_x = ALIGN_UNSIGNED(dst_x, pixel);

	log_debug(("osd:frame dst_dxxdst_dy:%dx%d, xxy:%dx%d, dxxdy:%dx%d, dpxdw:%dx%d\n", context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_pos_x, context->state.game_visible_pos_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dp, context->state.blit_src_dw));

	if (advance_ui_direct_active(ui_context)) {
		unsigned pos_x = context->state.game_used_pos_x + context->state.game_visible_pos_x;
		unsigned pos_y = context->state.game_used_pos_y + context->state.game_visible_pos_y;
		unsigned size_x = context->state.game_visible_size_x;
		unsigned size_y = context->state.game_visible_size_y;

		/* restore the original orientation */
		if ((context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
			SWAP(unsigned, pos_x, pos_y);
			SWAP(unsigned, size_x, size_y);
		}

		src_offset = pos_x * context->state.game_bytes_per_pixel + pos_y * bitmap->bytes_per_scanline;

		advance_ui_direct_update(ui_context, (unsigned char*)bitmap->ptr + src_offset, size_x, size_y, bitmap->bytes_per_scanline, context->state.game_color_def, context->state.palette_map, context->state.palette_total);
	}

	ui_buffer_active = advance_ui_buffer_active(ui_context);

	/* use buffer or direct write to screen ? */
	buffer_flag = 0;

	/* if ui active use the buffer */
	if (ui_buffer_active) {
		buffer_flag = 1;
	}

	start = 0;
	stop = 0;

	if (context->state.pipeline_measure_flag) {
		start = target_clock();
		/* alternate buffer and direct */
		if (context->state.pipeline_measure_buffer_mac < context->state.pipeline_measure_direct_mac)
			buffer_flag = 1;
	} else {
		/* use the selected best */
		if (!context->state.pipeline_measure_bestisdirect_flag)
			buffer_flag = 1;
	}

	if (buffer_flag) {
		int buf_dw;
		int buf_dp;
		unsigned char* buf_ptr;
		int intermediate_game_visible_pos_x;
		int intermediate_game_visible_pos_y;
		int final_size_x;
		int final_size_y;

		final_size_x = context->state.buffer_size_x;
		final_size_y = context->state.buffer_size_y;
		intermediate_game_visible_pos_x = context->state.game_visible_pos_x;
		intermediate_game_visible_pos_y = context->state.game_visible_pos_y;

		if (context->config.user_orientation & OSD_ORIENTATION_SWAP_XY) {
			SWAP(unsigned, dst_x, dst_y);
			SWAP(int, intermediate_game_visible_pos_x, intermediate_game_visible_pos_y);
		}

		/* compute the source pointer */
		src_offset = context->state.buffer_src_offset + intermediate_game_visible_pos_y * context->state.buffer_src_dw + intermediate_game_visible_pos_x * context->state.buffer_src_dp;

		video_pipeline_blit(&context->state.buffer_pipeline_video, dst_x, dst_y, (unsigned char*)bitmap->ptr + src_offset);

		if (ui_buffer_active) {
			advance_ui_buffer_update(ui_context, context->state.buffer_ptr, context->state.buffer_size_x, context->state.buffer_size_y, context->state.buffer_bytes_per_scanline, video_color_def(), context->state.palette_map, context->state.palette_total);
		}

		buf_ptr = context->state.buffer_ptr;
		buf_dw = context->state.buffer_bytes_per_scanline;
		buf_dp = video_bytes_per_pixel();

#if 0 /* OSDEF: Save interface image, only for debugging. */
		{
			struct advance_input_context* input_context = &CONTEXT.input;
			if (advance_input_digital_pressed(input_context, DIGITAL_KBD(0,KEYB_ENTER_PAD))) {
				FILE* f;
				static unsigned in = 1;
				char buffer[64];

				snprintf(buffer, sizeof(buffer), "im%d.png", in);
				++in;

				f = fzopen(buffer, "wb");
				advance_record_png_write(f, buf_ptr, context->state.buffer_size_x, context->state.buffer_size_y, buf_dp, buf_dw, video_color_def(), 0, 0, 0);
				fzclose(f);
			}
		}
#endif

		if (context->config.user_orientation & OSD_ORIENTATION_SWAP_XY) {
			SWAP(int, buf_dw, buf_dp);
			SWAP(int, final_size_x, final_size_y);
		}

		if (context->config.user_orientation & OSD_ORIENTATION_FLIP_Y) {
			buf_ptr += (final_size_y - 1) * buf_dw;
			buf_dw = -buf_dw;
		}

		if (context->config.user_orientation & OSD_ORIENTATION_FLIP_X) {
			buf_ptr += (final_size_x - 1) * buf_dp;
			buf_dp = -buf_dp;
		}

		video_stretch_direct(x, y, video_size_x(), video_size_y(), buf_ptr, final_size_x, final_size_y, buf_dw, buf_dp, video_color_def(), 0);

		if (ui_buffer_active) {
			/* clear the buffer for the next update (the ui may write over the game area) */
			video_buffer_clear(context);
		}
	} else {
		/* compute the source pointer */
		src_offset = context->state.blit_src_offset + context->state.game_visible_pos_y * context->state.blit_src_dw + context->state.game_visible_pos_x * context->state.blit_src_dp;

		video_pipeline_blit(&context->state.blit_pipeline_video, dst_x + x, dst_y + y, (unsigned char*)bitmap->ptr + src_offset);
	}

	/* if a valid measure */
	if (context->state.pipeline_measure_flag && !ui_buffer_active) {
		stop = target_clock();

		if (buffer_flag) {
			if (context->state.pipeline_measure_buffer_mac < PIPELINE_MEASURE_MAX) {
				context->state.pipeline_measure_buffer_map[context->state.pipeline_measure_buffer_mac] = stop - start;
				++context->state.pipeline_measure_buffer_mac;
			}
		} else {
			if (context->state.pipeline_measure_direct_mac < PIPELINE_MEASURE_MAX) {
				context->state.pipeline_measure_direct_map[context->state.pipeline_measure_direct_mac] = stop - start;
				++context->state.pipeline_measure_direct_mac;
			}
		}

		if (context->state.pipeline_measure_direct_mac == PIPELINE_MEASURE_MAX
			&& context->state.pipeline_measure_buffer_mac == PIPELINE_MEASURE_MAX) {

			context->state.pipeline_measure_buffer_result = adv_measure_median(0.00001, 0.5, context->state.pipeline_measure_buffer_map, PIPELINE_MEASURE_MAX);
			log_std(("emu:video: measure buffer %g\n", context->state.pipeline_measure_buffer_result));

			context->state.pipeline_measure_direct_result = adv_measure_median(0.00001, 0.5, context->state.pipeline_measure_direct_map, PIPELINE_MEASURE_MAX);
			log_std(("emu:video: measure direct %g\n", context->state.pipeline_measure_direct_result));

			/* select the best */
			context->state.pipeline_measure_bestisdirect_flag = context->state.pipeline_measure_direct_result < context->state.pipeline_measure_buffer_result;

			if (context->state.pipeline_measure_bestisdirect_flag) {
				log_std(("emu:video: best is direct\n"));
			} else {
				log_std(("emu:video: best is buffer\n"));
			}

			/* end the measure process */
			context->state.pipeline_measure_flag = 0;
		}
	}
}

static void video_frame_screen(struct advance_video_context* context, struct advance_ui_context* ui_context, const struct osd_bitmap *bitmap, unsigned input)
{
	update_start();

	video_update_pipeline(context, bitmap);

	video_frame_put(context, ui_context, bitmap, update_x_get(), update_y_get());

	update_stop(update_x_get(), update_y_get(), video_size_x(), video_size_y(), 0);
}

static void video_frame_palette(struct advance_video_context* context)
{
	if (context->state.palette_dirty_flag) {
		unsigned i;

		context->state.palette_dirty_flag = 0;

		for(i=0;i<context->state.palette_dirty_total;++i) {
			if (context->state.palette_dirty_map[i]) {
				unsigned j;
				unsigned jl;
				unsigned t;
				unsigned m;

				m = context->state.palette_dirty_map[i];
				context->state.palette_dirty_map[i] = 0;

				jl = context->state.palette_total - i * osd_mask_size;
				if (jl > osd_mask_size)
					jl = osd_mask_size;

				t = 1;
				for(j=0;j<jl;++j) {
					if ((m & t) != 0) {
						unsigned p = i * osd_mask_size + j;

						adv_color_rgb c = context->state.palette_map[p];

						/* update the palette */
						if (context->state.mode_index == MODE_FLAGS_INDEX_PALETTE8) {
							/* hardware */
							/* note: trying to concatenate palette update */
							/* generate flickering!, one color at time is ok! */
							video_palette_set(&c, p, 1, 0);
						} else {
							/* software */
							adv_pixel pixel;
							video_pixel_make(&pixel, c.red, c.green, c.blue);
							switch (video_bytes_per_pixel()) {
							case 4 :
								context->state.palette_index32_map[p] = pixel;
								break;
							case 2 :
								context->state.palette_index16_map[p] = pixel;
								break;
							case 1 :
								context->state.palette_index8_map[p] = pixel;
								break;
							}
						}
					}

					t <<= 1;
				}
			}
		}
	}
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

void video_frame_event(struct advance_video_context* context, struct advance_safequit_context* safequit_context, int leds_status, adv_bool turbo_status, unsigned input)
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

static adv_bool video_skip_dec(struct advance_video_context* context)
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

static adv_bool video_skip_inc(struct advance_video_context* context)
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

	if (full <= step) {
		/* full frame rate */
		context->state.skip_level_full = SYNC_MAX;
		context->state.skip_level_skip = 0;
	} else if (skip >= step /* (this check is implicit on the next one) */
		|| skip * SYNC_MAX + full >= step * (SYNC_MAX+1)
	) {
		/* if the maximum skip plus one isn't enought use a special management */
		/* (the plus one is to remove jumping on the compare border) */
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
			context->state.skip_level_full = ceil( v );
#else
			context->state.skip_level_full = floor( v );
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
			context->state.skip_level_skip = floor( v );
#else
			context->state.skip_level_skip = ceil( v );
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

		target_usleep( diff * 1E6 );

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
		if (context->state.vsync_flag && context->state.skip_level_full == SYNC_MAX) {
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

		if (fabs(context->state.sync_pivot) > context->state.skip_step / 100)
			log_std(("advance:sync: tot %.5f (max %.5f), err %8.5f, frame %.5f, sleep %.5f, sync %.5f\n", current - context->state.sync_last, context->state.skip_step, context->state.sync_pivot, time_before_sync - context->state.sync_last, time_after_delay - time_before_sync, time_after_sync - time_after_delay));

		if (context->state.sync_pivot < -5) {
			/* if the error is too big (negative) the delay is unrecoverable */
			/* generally it happen with a virtual terminal switch */
			/* the best solution is to restart the sync computation */
			video_update_skip(context);
			video_update_sync(context);
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

static void video_sync_update(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, adv_bool skip_flag)
{
	if (!skip_flag) {
		double delay;

		adv_bool warming_up = context->state.sync_warming_up_flag;

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

	/* force a recoputation when needed */
	context->state.skip_warming_up_flag = 1;
}

static void video_skip_update(struct advance_video_context* context, struct advance_estimate_context* estimate_context, struct advance_record_context* record_context)
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

static adv_bool video_is_normal_speed(struct advance_video_context* context)
{
	adv_bool normal_speed;

	normal_speed = !context->state.turbo_flag
		&& !context->state.fastest_flag
		&& !context->state.measure_flag
		&& context->state.sync_throttle_flag;

	return normal_speed;
}

static void video_cmd_update(struct advance_video_context* context, struct advance_estimate_context* estimate_context, struct advance_safequit_context* safequit_context, struct advance_ui_context* ui_context, adv_conf* cfg_context, int leds_status, unsigned input, adv_bool skip_flag)
{
	/* increment the number of frames */
	++context->state.frame_counter;

	/* events */
	video_frame_event(context, safequit_context, leds_status, context->state.turbo_flag, input);

	/* scripts */
	hardware_script_idle( SCRIPT_TIME_UNIT / context->state.game_fps );
	hardware_simulate_input_idle( SIMULATE_EVENT, SCRIPT_TIME_UNIT / context->state.game_fps );
	hardware_simulate_input_idle( SIMULATE_KEY, SCRIPT_TIME_UNIT / context->state.game_fps );

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

		video_update_skip(context);
		video_update_sync(context);

		if (!context->state.info_flag) {
			context->state.info_flag = 1;
			context->state.info_counter = 2 * context->state.game_fps;
		}
	}

	if ((input & OSD_INPUT_FRAMESKIP_INC) != 0) {
		if (context->config.frameskip_auto_flag) {
			context->config.frameskip_auto_flag = 0;
			video_update_skip(context);
		} else {
			if (video_skip_inc(context))
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
			video_update_skip(context);
		} else {
			if (video_skip_dec(context))
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
			video_update_skip(context);
			video_update_sync(context);
		}
	} else {
		if ((input & OSD_INPUT_TURBO) != 0) {
			context->state.turbo_flag = 1;
			video_update_skip(context);
			video_update_sync(context);
		}
	}

	if (context->state.fastest_flag) {
		if (context->state.frame_counter > context->state.fastest_limit) {
			context->state.fastest_flag = 0;

			video_update_skip(context);
			video_update_sync(context);

			hardware_script_start(HARDWARE_SCRIPT_PLAY);
		}
	}
	
	if ((input & OSD_INPUT_TOGGLE_DEBUG) != 0) {
		context->state.debugger_flag = !context->state.debugger_flag;
		video_invalidate_screen();
	}

	if ((input & OSD_INPUT_COCKTAIL) != 0) {
		context->config.blit_orientation ^= OSD_ORIENTATION_FLIP_Y | OSD_ORIENTATION_FLIP_X;
		context->config.user_orientation ^= OSD_ORIENTATION_FLIP_Y | OSD_ORIENTATION_FLIP_X;

		video_invalidate_pipeline(context);
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

		rate = floor( 100.0 / (estimate_context->estimate_frame * context->state.game_fps / context->config.fps_speed_factor) + 0.5 );
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

		rate = floor( 100.0 / (estimate_context->estimate_frame * context->state.game_fps / context->config.fps_speed_factor) + 0.5 );

		snprintf(buffer, sizeof(buffer), "%3d%%", rate);

		hardware_script_info(0, 0, 0, buffer);
	}
}

static void video_frame_game(struct advance_video_context* context, struct advance_record_context* record_context, struct advance_ui_context* ui_context, const struct osd_bitmap *bitmap, unsigned input, adv_bool skip_flag)
{
	/* bitmap */
	if (!skip_flag) {
		video_frame_palette(context);
		video_frame_screen(context, ui_context, bitmap, input);

		if (advance_record_video_is_active(record_context)
			&& !context->state.pause_flag) {

			unsigned pos_x = context->state.game_used_pos_x;
			unsigned pos_y = context->state.game_used_pos_y;
			unsigned size_x = context->state.game_used_size_x;
			unsigned size_y = context->state.game_used_size_y;
			int dp = context->state.game_bytes_per_pixel;
			int dw = bitmap->bytes_per_scanline;
			int offset;

			/* restore the original orientation */
			if ((context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
				SWAP(unsigned, pos_x, pos_y);
				SWAP(unsigned, size_x, size_y);
			}

			offset = pos_x * dp + pos_y * dw;

			if (context->state.game_rgb_flag) {
				advance_record_video_update(record_context, (unsigned char*)bitmap->ptr + offset, size_x, size_y, dp, dw, context->state.game_color_def, 0, 0, context->config.game_orientation);
			} else {
				advance_record_video_update(record_context, (unsigned char*)bitmap->ptr + offset, size_x, size_y, dp, dw, context->state.game_color_def, context->state.palette_map, context->state.palette_total, context->config.game_orientation);
			}
		}

		if (advance_record_snapshot_is_active(record_context)
			&& !context->state.pause_flag) {

			unsigned pos_x = context->state.game_used_pos_x;
			unsigned pos_y = context->state.game_used_pos_y;
			unsigned size_x = context->state.game_used_size_x;
			unsigned size_y = context->state.game_used_size_y;
			int dp = context->state.game_bytes_per_pixel;
			int dw = bitmap->bytes_per_scanline;
			int offset;

			/* restore the original orientation */
			if ((context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
				SWAP(unsigned, pos_x, pos_y);
				SWAP(unsigned, size_x, size_y);
			}

			offset = pos_x * dp + pos_y * dw;

			if (context->state.game_rgb_flag) {
				advance_record_snapshot_update(record_context, (unsigned char*)bitmap->ptr + offset, size_x, size_y, dp, dw, context->state.game_color_def, 0, 0, context->config.game_orientation);
			} else {
				advance_record_snapshot_update(record_context, (unsigned char*)bitmap->ptr + offset, size_x, size_y, dp, dw, context->state.game_color_def, context->state.palette_map, context->state.palette_total, context->config.game_orientation);
			}
		}
	}

	/* mode */
	video_frame_resolution(context, input);

	/* pan */
	video_frame_pan(context, input);
}

static void video_frame_debugger(struct advance_video_context* context, const struct osd_bitmap* bitmap, const osd_rgb_t* palette, unsigned palette_size)
{
	unsigned size_x;
	unsigned size_y;
	uint8* palette8_raw;
	uint16* palette16_raw;
	uint32* palette32_raw;
	unsigned i;

	if (!bitmap || !palette) {
		log_std(("ERROR:emu:video: null debugger bitmap\n"));
		return;
	}

	if (context->state.mode_index == MODE_FLAGS_INDEX_PALETTE8) {
		log_std(("ERROR:emu:video: debugger not supported in palette mode\n"));
		return;
	}

	/* max size */
	size_x = video_size_x();
	size_y = video_size_y();

	palette32_raw = (uint32*)malloc(palette_size * sizeof(uint32));
	palette16_raw = (uint16*)malloc(palette_size * sizeof(uint16));
	palette8_raw = (uint8*)malloc(palette_size * sizeof(uint8));

	for(i=0;i<palette_size;++i) {
		osd_rgb_t c = palette[i];
		adv_pixel pixel;
		video_pixel_make(&pixel, osd_rgb_red(c), osd_rgb_green(c), osd_rgb_blue(c));
		palette32_raw[i] = pixel;
		palette16_raw[i] = pixel;
		palette8_raw[i] = pixel;
	}

	video_stretch_palette8(0, 0, size_x, size_y, bitmap->ptr, bitmap->size_x, bitmap->size_y, bitmap->bytes_per_scanline, 1, palette8_raw, palette16_raw, palette32_raw, VIDEO_COMBINE_Y_MAXMIN | VIDEO_COMBINE_X_MAXMIN);

	free(palette32_raw);
	free(palette16_raw);
	free(palette8_raw);
}

static void advance_video_update(struct advance_video_context* context, struct advance_record_context* record_context, struct advance_ui_context* ui_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned input, adv_bool skip_flag)
{
	if (context->state.debugger_flag) {
		video_frame_debugger(context, debug, debug_palette, debug_palette_size);
	} else {
		video_frame_game(context, record_context, ui_context, game, input, skip_flag);
	}
}

static void video_frame_update_now(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, struct advance_record_context* record_context, struct advance_ui_context* ui_context, struct advance_safequit_context* safequit_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool skip_flag)
{
	/* Do a yield immediatly before the time syncronization. */
	/* If a schedule will be done, it's better to have it now when */
	/* we need to wait the syncronization point. Obviously it may happen */
	/* to lose the precise time, but it will happen anyway if the system is busy. */
	target_yield();

	/* the frame syncronization is out of the time estimation */
	video_sync_update(context, sound_context, estimate_context, skip_flag);

	/* estimate the time */
	advance_estimate_osd_begin(estimate_context);

	/* update the video for the new frame */
	advance_video_update(context, record_context, ui_context, game, debug, debug_palette, debug_palette_size, input, skip_flag);

	/* update the audio buffer for the new frame */
	advance_sound_update(sound_context, record_context, context, safequit_context, sample_buffer, sample_count, sample_recount, video_is_normal_speed(context));

	/* estimate the time */
	advance_estimate_osd_end(estimate_context, skip_flag);
}

/***************************************************************************/
/* Thread */

/**
 * Wait the completion of the video thread.
 */
static void video_thread_wait(struct advance_video_context* context)
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
		log_debug(("advance:thread: wait\n"));

		/* wait for the start notification  */
		while (!context->state.thread_state_ready_flag && !context->state.thread_exit_flag) {
			pthread_cond_wait(&context->state.thread_video_cond, &context->state.thread_video_mutex);
		}

		log_debug(("advance:thread: wakeup\n"));

		/* check for exit */
		if (context->state.thread_exit_flag) {
			log_std(("advance:thread: stop\n"));
			pthread_mutex_unlock(&context->state.thread_video_mutex);
			break;
		}

		pthread_mutex_unlock(&context->state.thread_video_mutex);

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

		/* notify that the data was used */
		context->state.thread_state_ready_flag = 0;

		/* signal the completion of the operation */
		pthread_cond_signal(&context->state.thread_video_cond);
	}

	pthread_exit(0);
	return 0;
}
#endif

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
	video_thread_wait(context);

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

/***************************************************************************/
/* OSD */

/**
 * Update the state after a configuration change from the user interface.
 */
adv_error advance_video_set(struct advance_video_context* context)
{
	adv_mode mode;

#ifdef USE_SMP
	pthread_mutex_lock(&context->state.thread_video_mutex);
#endif

	mode_reset(&mode);

	if (video_update_index(context) != 0) {
		goto err;
	}

	if (video_update_crtc(context) != 0) {
		goto err;
	}

	if (video_make_crtc(context, &context->state.crtc_effective, context->state.crtc_selected) != 0) {
		goto err;
	}

	video_update_ui(context, &context->state.crtc_effective);

	video_update_visible(context, &context->state.crtc_effective);
	video_update_effect(context);

	if (video_make_vidmode(context, &mode, &context->state.crtc_effective) != 0) {
		goto err;
	}

	video_update_pan(context);

	if (video_update_vidmode(context, &mode, 0) != 0) {
		goto err;
	}

	video_update_skip(context);
	video_update_sync(context);

	video_invalidate_screen();

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.thread_video_mutex);
#endif

	return 0;

err:
#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.thread_video_mutex);
#endif
	return -1;
}

void advance_video_change(struct advance_video_context* context, struct advance_video_config_context* config)
{
	struct advance_video_config_context old_config;

	/* save the old configuration */
	old_config = context->config;

	/* set the new config */
	context->config = *config;

	video_thread_wait(context);

	log_std(("emu:video: select mode %s\n", context->config.resolution_buffer));

	/* update all the complete state, the configuration is choosen by the name */
	if (advance_video_set(context) != 0) {
		/* it fails in some strange conditions, generally when a not supported feature is used */
		/* for example if a interlaced mode is requested and the lower driver refuses to use it */

		/* restore the config */
		context->config = old_config;

		log_std(("emu:video: retrying old config\n"));

		if (advance_video_set(context) != 0) {
			/* if something go wrong abort */
			log_std(("emu:video: advance_video_update_config() failed\n"));
			target_err("Unexpected error changing video options.\n");
			abort();
		}
	}
}

int osd2_video_init(struct osd_video_option* req)
{
	struct advance_video_context* context = &CONTEXT.video;
	struct advance_input_context* input_context = &CONTEXT.input;

	adv_mode mode;
	mode_reset(&mode);

	log_std(("osd: osd2_video_init\n"));

	log_std(("osd: area_size_x %d, area_size_y %d\n", req->area_size_x, req->area_size_y));
	log_std(("osd: used_size_x %d, used_size_y %d, used_pos_x %d, used_pos_y %d\n", req->used_size_x, req->used_size_y, req->used_pos_x, req->used_pos_y));
	log_std(("osd: aspect_x %d, aspect_y %d\n", req->aspect_x, req->aspect_y));
	log_std(("osd: bits_per_pixel %d, rgb_flag %d, colors %d\n", req->bits_per_pixel, req->rgb_flag, req->colors));
	log_std(("osd: vector_flag %d\n", req->vector_flag));
	log_std(("osd: fps %g\n", req->fps));
    
	if (video_init_state(context, req) != 0) {
		return -1;
	}

	if (video_update_index(context)!=0) {
		target_err("Unsupported bit depth.\n");
		return -1;
	}

	if (video_update_crtc(context) != 0) {
		if (strcmp(context->config.resolution_buffer, "auto")==0)
			target_err("No video modes available for the current game.\n");
		else
			target_err("The specified 'display_mode %s' doesn't exist.\n", context->config.resolution_buffer);
		return -1;
	}

	if (video_make_crtc(context, &context->state.crtc_effective, context->state.crtc_selected) != 0) {
		target_err("Unable to generate the crtc values.\n");
		return -1;
	}

	video_update_ui(context, &context->state.crtc_effective);

	video_update_visible(context, &context->state.crtc_effective);
	video_update_effect(context);

	if (video_make_vidmode(context, &mode, &context->state.crtc_effective) != 0) {
		target_err("%s\n", error_get());
		return -1;
	}

	video_init_color(context, req);

	video_update_pan(context);

	if (video_update_vidmode(context, &mode, 1) != 0) {
		target_err("%s\n", error_get());
		return -1;
	}

	video_update_skip(context);
	video_update_sync(context);

	hardware_script_start(HARDWARE_SCRIPT_VIDEO);

	/* enable the keyboard input. It must be done after the video mode set. */
	if (keyb_enable(1) != 0) {
		video_done_vidmode(context, 0);
		video_mode_restore();
		target_err("%s\n", error_get());
		return -1;
	}

	return 0;
}

void osd2_video_done(void)
{
	struct advance_video_context* context = &CONTEXT.video;
	struct advance_input_context* input_context = &CONTEXT.input;

	log_std(("osd: osd2_video_done\n"));

	hardware_script_terminate(HARDWARE_SCRIPT_PLAY);
	hardware_script_terminate(HARDWARE_SCRIPT_EMULATION);
	hardware_script_terminate(HARDWARE_SCRIPT_VIDEO);

	keyb_disable();

	if (context->config.restore_flag || context->state.measure_flag) {
		video_done_vidmode(context, 1);
		video_mode_restore();
	} else {
		video_done_vidmode(context, 0);
		if (video_mode_is_active())
			video_mode_done(0);
	}

	video_done_color(context);
	video_done_state(context);

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
	video_update_visible(context, &context->state.crtc_effective);
	video_update_effect(context);

	/* recenter */
	video_update_pan(context);

	video_invalidate_pipeline(context);
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

void osd_pause(int paused)
{
	struct advance_video_context* context = &CONTEXT.video;

	log_std(("osd: osd_pause(paused:%d)\n", paused));

	context->state.pause_flag = paused != 0;
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

	video_update_skip(context);
	video_update_sync(context);

	hardware_script_start(HARDWARE_SCRIPT_EMULATION);

	/* if not in "fastest" mode, the playing is already started */
	if (!context->state.fastest_flag)
		hardware_script_start(HARDWARE_SCRIPT_PLAY);
}

void osd2_debugger_focus(int debugger_has_focus)
{
	struct advance_video_context* context = &CONTEXT.video;

	log_std(("osd: osd_debugger_focus(debugger_has_focus:%d)\n", debugger_has_focus));

	context->state.debugger_flag = debugger_has_focus;

	video_invalidate_screen();
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

	/* update the global info */
	video_cmd_update(&CONTEXT.video, &CONTEXT.estimate, &CONTEXT.safequit, &CONTEXT.ui, CONTEXT.cfg, led, input, skip_flag);
	video_skip_update(&CONTEXT.video, &CONTEXT.estimate, &CONTEXT.record);
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
{ "scale", COMBINE_SCALE },
{ "lq", COMBINE_LQ },
{ "hq", COMBINE_HQ }
};

static adv_conf_enum_int OPTION_ADJUST[] = {
{ "none", ADJUST_NONE },
{ "x", ADJUST_ADJUST_X },
{ "clock", ADJUST_ADJUST_CLOCK },
{ "xclock", ADJUST_ADJUST_X | ADJUST_ADJUST_CLOCK },
{ "generate_exact", ADJUST_GENERATE },
{ "generate_y", ADJUST_GENERATE | ADJUST_ADJUST_Y },
{ "generate_clock", ADJUST_GENERATE | ADJUST_ADJUST_CLOCK },
{ "generate_yclock", ADJUST_GENERATE | ADJUST_ADJUST_Y | ADJUST_ADJUST_CLOCK | ADJUST_FAVORITE_SIZE_OVER_CLOCK },
{ "generate_clocky", ADJUST_GENERATE | ADJUST_ADJUST_Y | ADJUST_ADJUST_CLOCK }
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
	conf_int_register_enum_default(cfg_context, "display_resize", conf_enum(OPTION_RESIZE), STRETCH_INTEGER_X_FRACTIONAL_Y);
	conf_int_register_enum_default(cfg_context, "display_magnify", conf_enum(OPTION_MAGNIFY), 1);
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
	conf_bool_register_default(cfg_context, "misc_crash", 0);
	conf_string_register_default(cfg_context, "sync_startuptime", "auto");
	conf_int_register_limit_default(cfg_context, "misc_timetorun", 0, 3600, 0);
	conf_string_register_default(cfg_context, "display_mode", "auto");
	conf_int_register_enum_default(cfg_context, "display_color", conf_enum(OPTION_INDEX), 0);
	conf_bool_register_default(cfg_context, "display_restore", 1);
	conf_float_register_limit_default(cfg_context, "display_expand", 1.0, 10.0, 1.0);
	conf_int_register_limit_default(cfg_context, "display_aspectx", 1, INT_MAX, 4);
	conf_int_register_limit_default(cfg_context, "display_aspecty", 1, INT_MAX, 3);

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

static void video_config_mode(struct advance_video_context* context, struct mame_option* option)
{
	adv_crtc_container_iterator i;
	adv_crtc* best_crtc = 0;
	int best_size;

	if (context->config.adjust != ADJUST_NONE
		&& !video_is_programmable(context)
	) {
		/* disable the adjust mode if i isn't supported by the video driver */
		context->config.adjust = ADJUST_NONE;
		log_std(("emu:video: display_adjust=* disabled because the graphics driver is not programmable\n"));
	}

	/* insert some default modeline if no generate option is present */
	if (!video_is_generable(context)) {
		adv_bool has_something = 0;

		/* check if the list of video mode contains something useable */
		for(crtc_container_iterator_begin(&i, &context->config.crtc_bag);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
			adv_crtc* crtc = crtc_container_iterator_get(&i);
			if (is_crtc_acceptable_preventive(context, crtc)) {
				has_something = 1;
			}
		}

		if (has_something) {
			log_std(("emu:video: the user video mode list contains some useable mode\n"));
		} else {
			log_std(("emu:video: the user video mode list doesn't contain any useable mode\n"));
		}

		if (!has_something) {
			log_std(("emu:video: insert default video modes\n"));

			if (video_is_programmable(context)) {
				crtc_container_insert_default_modeline_svga(&context->config.crtc_bag);
				crtc_container_insert_default_modeline_vga(&context->config.crtc_bag);
			} else {
				crtc_container_insert_default_active(&context->config.crtc_bag);
			}
		}
	}

	/* set the debugger size */
	option->debug_width = 640;
	option->debug_height = 480;
	for(crtc_container_iterator_begin(&i, &context->config.crtc_bag);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		adv_crtc* crtc = crtc_container_iterator_get(&i);
		/* if a specific mode is chosen, size the debugger like it */
		if (is_crtc_acceptable_preventive(context, crtc)
			&& video_resolution_cmp(context->config.resolution_buffer, crtc_name_get(crtc)) == 0) {
			option->debug_width = crtc_hsize_get(crtc);
			option->debug_height = crtc_vsize_get(crtc);
		}
	}
	log_std(("emu:video: suggested debugger size %dx%d\n", option->debug_width, option->debug_height));

	/* set the vector game size */
	if (mame_is_game_vector(option->game)) {
		unsigned mode_size_x;
		unsigned mode_size_y;
		unsigned game_size_x;
		unsigned game_size_y;

		mode_size_x = 640;
		mode_size_y = 480;

		log_std(("emu:video: insert vector video modes\n"));

		if (video_is_generable(context)) {
			/* insert the default mode for vector games */
			video_init_crtc_make_vector(context, "generate", mode_size_x, mode_size_y, mode_size_x*2, mode_size_y*2, 0, 0, 0, 0, mame_game_fps(option->game));
		}

		/* select the size of the mode near at the 640x480 size */
		best_crtc = 0;
		best_size = 0;
		for(crtc_container_iterator_begin(&i, &context->config.crtc_bag);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
			adv_crtc* crtc = crtc_container_iterator_get(&i);
			if (is_crtc_acceptable_preventive(context, crtc)
				&& (strcmp(context->config.resolution_buffer, "auto")==0
					|| video_resolution_cmp(context->config.resolution_buffer, crtc_name_get(crtc)) == 0
					)
				) {
				int size = crtc_hsize_get(crtc) * crtc_vsize_get(crtc);
				if (!best_crtc || abs(mode_size_x * mode_size_y - size) < abs(mode_size_x*mode_size_y - best_size)) {
					best_crtc = crtc;
					best_size = size;
				}
			}
		}

		if (best_crtc) {
			mode_size_x = crtc_hsize_get(best_crtc);
			mode_size_y = crtc_vsize_get(best_crtc);
		} else {
			log_std(("emu:video: no specific mode for vector games\n"));
		}

		/* assume a game aspect of 4/3 */
		if ((context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
			game_size_x = mode_size_y;
			game_size_y = crtc_step(mode_size_x * 9 / 16 * context->config.aspect_expansion_factor, 4);
		} else {
			game_size_x = mode_size_x;
			game_size_y = mode_size_y;
		}

		option->vector_width = game_size_x;
		option->vector_height = game_size_y;

		log_std(("emu:video: suggested vector size %dx%d\n", option->vector_width, option->vector_height));
	} else {
		option->vector_width = 0;
		option->vector_height = 0;
	}
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
	context->config.rgb_effect = conf_int_get_default(cfg_context, "display_rgbeffect");
	context->config.interlace_effect = conf_int_get_default(cfg_context, "display_interlaceeffect");
	context->config.turbo_speed_factor = conf_float_get_default(cfg_context, "sync_turbospeed");
	s = conf_string_get_default(cfg_context, "sync_fps");
	if (strcmp(s,"auto")==0) {
		context->config.fps_fixed = 0;
	} else {
		char* e;
		context->config.fps_fixed = strtod(s,&e);
		if (context->config.fps_fixed < 10 || context->config.fps_fixed > 300 || *e) {
			target_err("Invalid argument '%s' for option 'sync_fps'.\n", s);
			return -1;
		}
	}
	context->config.fps_speed_factor = conf_float_get_default(cfg_context, "sync_speed");

	s = conf_string_get_default(cfg_context, "sync_startuptime");
	if (strcmp(s, "auto") == 0) {
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
	context->config.crash_flag = conf_bool_get_default(cfg_context, "misc_crash");

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
	if (i == -1)
		context->config.internalresample_flag = 1;
	else
		context->config.internalresample_flag = 0;

	/* load context->config.monitor config */
	err = monitor_load(cfg_context, &context->config.monitor);
	if (err<0) {
		target_err("%s\n", error_get());
		target_err("Please read the file `install.txt' and `device.txt'.\n");
		return -1;
	}
	if (err == 0) {
		/* print the clock ranges */
		log_std(("emu:video: pclock %.3f - %.3f\n", (double)context->config.monitor.pclock.low, (double)context->config.monitor.pclock.high));
		for(i=0;i<MONITOR_RANGE_MAX;++i)
			if (context->config.monitor.hclock[i].low)
				log_std(("emu:video: hclock %.3f - %.3f\n", (double)context->config.monitor.hclock[i].low, (double)context->config.monitor.hclock[i].high));
		for(i=0;i<MONITOR_RANGE_MAX;++i)
			if (context->config.monitor.vclock[i].low)
				log_std(("emu:video: vclock %.3f - %.3f\n", (double)context->config.monitor.vclock[i].low, (double)context->config.monitor.vclock[i].high));
	}
	if (err > 0) {
		monitor_reset(&context->config.monitor);
	}

	/* load generate_linear config */
	err = generate_interpolate_load(cfg_context, &context->config.interpolate);
	if (err<0) {
		target_err("%s\n", error_get());
		target_err("Please read the file `install.txt' and `device.txt'.\n");
		return -1;
	} else if (err>0) {
		if (monitor_hclock_check(&context->config.monitor, 15720)) {
			/* Arcade Standard Resolution */
			log_std(("emu:video: default format standard resolution\n"));
			generate_default_atari_standard(&context->config.interpolate.map[0].gen);
			context->config.interpolate.map[0].hclock = 15720;
			context->config.interpolate.mac = 1;
		} else if (monitor_hclock_check(&context->config.monitor, 25000)) {
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
	if (video_init() != 0) {
		target_err("%s\n", error_get());
		return -1;
	}

	if (video_blit_init() != 0) {
		video_done();
		target_err("%s\n", error_get());
		return -1;
	}

	video_config_mode(context, option);
	return 0;
}

void advance_video_inner_done(struct advance_video_context* context)
{
	video_blit_done();
	video_done();
}


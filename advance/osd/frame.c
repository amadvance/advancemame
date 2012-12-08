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
#include "input.h"

#include "advance.h"

#include <math.h>

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

static int adjust_step(int multiplier, int divider, int step)
{
	int v;

	v = multiplier / divider;
	if ((v % step) != 0)
		v += step - (v % step);

	while (divider * v < multiplier)
		v += step;

	return v;
}

static int adjust_multiplier(int value, int base, int step, int upper)
{
	int i;
	for(i=1;i<8;++i)
		if (base * i <= upper && abs(value - base * i) <= step * i)
			return base * i;
	return value;
}

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
 * Adjust the CRTC value using the game size and clock information.
 * \return 0 on success
 */
static adv_error video_make_crtc_for_game(struct advance_video_context* context, adv_crtc* crtc, const adv_crtc* original_crtc)
{
	*crtc = *original_crtc;

	if (!video_is_programmable(context)) {
		return 0; /* always ok if the driver is not programmable */
	}

	if ((context->config.adjust & ADJUST_ADJUST_X) != 0) {
		unsigned size_x;
		unsigned size_y;

		size_y = crtc_vsize_get(crtc);

		if (context->config.stretch != STRETCH_INTEGER_XY
			&& context->config.stretch != STRETCH_NONE) {
			/* if a vertical fractional stretch is selected */
			/* set the horizontal size assuming that the whole vertical */
			/* is used */

			double factor;

			factor = size_y / (double)context->state.mode_best_size_y;

			if (factor < 1.5)
				size_x = context->state.mode_best_size_x;
			else if (factor < 2.5)
				size_x = context->state.mode_best_size_2x;
			else if (factor < 3.5)
				size_x = context->state.mode_best_size_3x;
			else
				size_x = context->state.mode_best_size_4x;
		} else {
			/* if a vertical integer stretch is selected */
			/* set the horizontal size considering the current vertical size */
			/* to keep the correct aspect ratio */

			long long unsigned factor_x;
			long long unsigned factor_y;

			factor_x = context->state.mode_aspect_factor_x;
			factor_y = context->state.mode_aspect_factor_y;

			size_x = adjust_step(size_y * factor_x, factor_y, CRTC_HSTEP);
		}

		crtc_hsize_set(crtc, size_x);

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_hsize_set(crtc, 2 * size_x);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_hsize_set(crtc, 3 * size_x);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_hsize_set(crtc, 4 * size_x);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			/* restore the original crtc */
			*crtc = *original_crtc;
		}
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
				/* restore the original crtc */
				*crtc = *original_crtc;
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
	if (video_make_crtc_for_game(context, &temp_crtc, crtc) != 0)
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
 * This function guess the result of the is_crtc_acceptable function without
 * using any game information.
 * This function is used to create default video mode for debugger and vector games.
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

/***************************************************************************/
/* Update */

/**
 * Update the video depth.
 * Recompute the video mode depth from the configuration variables.
 */
adv_error advance_video_update_index(struct advance_video_context* context)
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
 * Invalidates and clears the contents of the screen.
 */
void advance_video_invalidate_screen(struct advance_video_context* context)
{
	unsigned i;
	adv_pixel color;
	unsigned count;

	assert(video_mode_is_active());

	/* on palettized modes it always return 0 */
	color = video_pixel_get(0, 0, 0);

	/* number of times to clear to use all the buffers */
	count = update_page_max_get();
	if (count == 1 && (video_flags() & MODE_FLAGS_RETRACE_WRITE_SYNC) != 0)
		count = 2;

	/* intentionally doesn't clear the entire video memory, */
	/* it's more safe to clear only the used part, for example */
	/* if case the memory size detection is wrong  */
	for(i=0;i<count;++i) {
		update_start();
		log_std(("emu:video: clear %dx%d %dx%d\n", update_x_get(), update_y_get(), video_size_x(), video_size_y()));
		video_clear(update_x_get(), update_y_get(), video_size_x(), video_size_y(), color);
		update_stop(update_x_get(), update_y_get(), video_size_x(), video_size_y(), 0);
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
		unsigned i;
		for(i=0;i<PIPELINE_BLIT_MAX;++i)
			video_pipeline_done(&context->state.blit_pipeline[i]);
		video_pipeline_done(&context->state.buffer_pipeline_video);
		context->state.blit_pipeline_flag = 0;
	}

	if (context->state.buffer_ptr_alloc) {
		free(context->state.buffer_ptr_alloc);
		context->state.buffer_ptr_alloc = 0;
	}
}

/**
 * Invalidate the blit pipeline.
 * Forget the current pipeline and for a recomputation on the next use.
 */
void advance_video_invalidate_pipeline(struct advance_video_context* context)
{
	/* destroy the pipeline, this force the pipeline update at the next frame */
	video_done_pipeline(context);
}

/**
 * Set the video mode.
 * The mode is copyed in the context if it is set with success.
 * \return 0 on success
 */
static adv_error vidmode_init(struct advance_video_context* context, adv_mode* mode)
{
	union adv_color_def_union def;

	assert(!context->state.mode_flag);

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
	update_init(context->config.triplebuf_flag != 0 ? 3 : 1);
	log_std(("emu:video: using %d hardware video buffers\n", update_page_max_get()));

	/* set the buffer color mode */
	if (color_def_type_get(video_color_def()) == adv_color_type_yuy2) {
		/* the buffer is always an RGB mode */
		context->state.buffer_def = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0);
	} else {
		context->state.buffer_def = video_color_def();
	}

	advance_video_invalidate_screen(context);
	video_invalidate_color(context);

	log_std(("emu:video: mode %s, size %dx%d, bits_per_pixel %d, bytes_per_scanline %d, pages %d\n", video_name(), video_size_x(), video_size_y(), video_bits_per_pixel(), video_bytes_per_scanline(), update_page_max_get()));

	return 0;
}

static void vidmode_done(struct advance_video_context* context, adv_bool restore)
{
	assert(context->state.mode_flag);

	/* clear all the video memory used */
	if (restore)
		advance_video_invalidate_screen(context);

	video_done_pipeline(context);

	update_done();

	context->state.mode_flag = 0;
}

static adv_error vidmode_update(struct advance_video_context* context, adv_mode* mode, adv_bool ignore_input)
{
	advance_video_invalidate_pipeline(context);

	if (!context->state.mode_flag
		|| video_mode_compare(mode, video_current_mode())!=0
	) {
		if (context->state.mode_flag)
			vidmode_done(context, 0);

		if (!ignore_input) {
			joystickb_disable();
			mouseb_disable();
			keyb_disable();
		}

		if (vidmode_init(context, mode) != 0) {
			return -1;
		}

		if (!ignore_input) {
			if (keyb_enable(1) != 0) {
				log_std(("ERROR:emu:video: calling keyb_enable() '%s'\n", error_get()));
				return -1;
			}
			if (mouseb_enable() != 0) {
				keyb_disable();
				log_std(("ERROR:emu:video: calling mouseb_enable() '%s'\n", error_get()));
				return -1;
			}
			if (joystickb_enable() != 0) {
				mouseb_disable();
				keyb_disable();
				log_std(("ERROR:emu:video: calling joystickb_enable() '%s'\n", error_get()));
				return -1;
			}
		}
	}

	return 0;
}

/**
 * Update the panning state.
 */
void advance_video_update_pan(struct advance_video_context* context)
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

/**
 * Update the user interface.
 * Recompute the user interface aspect from the configuration variable and
 * from the specified crtc configuration.
 */
void advance_video_update_ui(struct advance_video_context* context, const adv_crtc* crtc)
{
	unsigned aspect_x;
	unsigned aspect_y;

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)!=0) {
		/* with window assume square pixel */
		aspect_x = crtc_hsize_get(crtc);
		aspect_y = crtc_vsize_get(crtc);
	} else {
		aspect_x = context->config.monitor_aspect_x;
		aspect_y = context->config.monitor_aspect_y;
	}

	advance_ui_changefont(&CONTEXT.ui, crtc_hsize_get(crtc), crtc_vsize_get(crtc), aspect_x, aspect_y);
}

/**
 * Update the effect.
 * Recompute the blit effect from the configuration variables.
 */
void advance_video_update_effect(struct advance_video_context* context)
{
	double previous_gamma_factor;

	context->state.rgb_effect = context->config.rgb_effect;
	context->state.interlace_effect = context->config.interlace_effect;
	context->state.combine = context->config.combine;

	if (context->state.combine == COMBINE_AUTO) {
		if (context->state.mode_visible_size_x == 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 2*context->state.game_visible_size_y) {
			context->state.combine = context->config.combine_max;
		} else if (context->state.mode_visible_size_x == 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 3*context->state.game_visible_size_y) {
			context->state.combine = context->config.combine_max;
		} else if (context->state.mode_visible_size_x == 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 4*context->state.game_visible_size_y) {
			context->state.combine = context->config.combine_max;
		} else if (context->state.mode_visible_size_x == 3*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 3*context->state.game_visible_size_y) {
			context->state.combine = context->config.combine_max;
		} else if (context->state.mode_visible_size_x == 4*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 4*context->state.game_visible_size_y) {
			context->state.combine = context->config.combine_max;
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

	if ((context->state.combine == COMBINE_SCALEX)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 2*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 3*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 4*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
	) {
		log_std(("emu:video: resizeeffect=scalex disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	if ((context->state.combine == COMBINE_SCALEK)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 2*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 3*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 4*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
	) {
		log_std(("emu:video: resizeeffect=scalek disabled because the wrong mode size\n"));
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

	if ((context->state.combine == COMBINE_XBR)
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x || context->state.mode_visible_size_y != 2*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 3*context->state.game_visible_size_x || context->state.mode_visible_size_y != 3*context->state.game_visible_size_y)
		&& (context->state.mode_visible_size_x != 4*context->state.game_visible_size_x || context->state.mode_visible_size_y != 4*context->state.game_visible_size_y)
	) {
		log_std(("emu:video: resizeeffect=xbr disabled because the wrong mode size\n"));
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

	switch (context->state.rgb_effect) {
	case EFFECT_RGB_TRIAD3PIX :
	case EFFECT_RGB_TRIAD6PIX :
	case EFFECT_RGB_TRIAD16PIX :
	case EFFECT_RGB_TRIADSTRONG3PIX :
	case EFFECT_RGB_TRIADSTRONG6PIX :
	case EFFECT_RGB_TRIADSTRONG16PIX :
		switch (context->state.mode_index) {
		case MODE_FLAGS_INDEX_BGR8 :
		case MODE_FLAGS_INDEX_BGR15 :
		case MODE_FLAGS_INDEX_BGR16 :
		case MODE_FLAGS_INDEX_BGR32 :
			break;
		default:
			log_std(("emu:video: rgbeffect=triad* disabled because we aren't in a rgb mode\n"));
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

	mame_ui_gamma_factor_set(context->state.gamma_effect_factor / previous_gamma_factor);
}

/**
 * Update the visible range.
 * Recompute the size and offset of the visible part of the frame from the
 * configuration variables and from the specified crtc.
 */
void advance_video_update_visible(struct advance_video_context* context, const adv_crtc* crtc)
{
	unsigned stretch;

	assert(crtc);

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

	/* compute the mode visible part assuming a complete fraction stretch */
	/* some values are overwritten later if the stretch method is different */
	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)!=0) {
		/* with window assume square pixel */
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
		It's the size in pixel of a visible square drawn
		on the current video mode.

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
			context->state.mode_visible_size_x = crtc_step((double)context->state.mode_visible_size_y * factor_x / factor_y, 8);
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
		context->state.mode_visible_size_x = adjust_multiplier(context->state.mode_visible_size_x, context->state.game_used_size_x, CRTC_HSTEP, crtc_hsize_get(crtc));
		context->state.mode_visible_size_y = adjust_multiplier(context->state.mode_visible_size_y, context->state.game_used_size_y, CRTC_VSTEP, crtc_vsize_get(crtc));
	} else if (stretch == STRETCH_INTEGER_X_FRACTIONAL_Y) {
		unsigned mx;
		mx = floor(context->state.mode_visible_size_x / (double)context->state.game_used_size_x);
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

		/* reject fractional for very small adjustement */
		context->state.mode_visible_size_y = adjust_multiplier(context->state.mode_visible_size_y, context->state.game_used_size_y, CRTC_VSTEP, crtc_vsize_get(crtc));
	} else if (stretch == STRETCH_INTEGER_XY) {
		unsigned mx;
		unsigned my;

		mx = floor(context->state.mode_visible_size_x / (double)context->state.game_used_size_x);
		if (mx < 1)
			mx = 1;

		my = floor(context->state.mode_visible_size_y / (double)context->state.game_used_size_y);
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
 * Update the crtc video configuration.
 * Recompute the crtc_selected variable from all the available crtc.
 */
adv_error advance_video_update_selectedcrtc(struct advance_video_context* context)
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
/* State */

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

	memset(context->state.pipeline_timing_map, 0, sizeof(context->state.pipeline_timing_map));
	context->state.pipeline_timing_i = 0;

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
		SWAP(unsigned, context->state.game_area_size_x, context->state.game_area_size_y);
		SWAP(unsigned, context->state.game_used_pos_x, context->state.game_used_pos_y);
		SWAP(unsigned, context->state.game_used_size_x, context->state.game_used_size_y);
		SWAP(unsigned, arcade_aspect_x, arcade_aspect_y);
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

		/* if the clock is programmable the monitor specification must be present */
		if (video_is_programmable(context)) {
			if (monitor_is_empty(&context->config.monitor)) {
				error_set("No monitor clocks specification `device_video_p/h/vclock'.\n");
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
	arcade_aspect
		The aspect of the original arcade game. It's the
		measured size of the original arcade display.
		(Effectively only of the used area of the display).

	monitor_aspect
		The aspect of the current monitor. It's the
		measured size of the current monitor.

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

		/* compute the game pixel aspect ratio */
		context->state.game_pixelaspect_x = context->state.game_used_size_x * arcade_aspect_y;
		context->state.game_pixelaspect_y = context->state.game_used_size_y * arcade_aspect_x;
		video_aspect_reduce(&context->state.game_pixelaspect_x, &context->state.game_pixelaspect_y);

		factor_x = context->config.monitor_aspect_x * context->state.game_pixelaspect_x;
		factor_y = context->config.monitor_aspect_y * context->state.game_pixelaspect_y;
		video_aspect_reduce(&factor_x, &factor_y);

		context->state.mode_aspect_factor_x = factor_x;
		context->state.mode_aspect_factor_y = factor_y;
		context->state.mode_aspect_vertgameinhorzscreen = context->config.monitor_aspect_x * arcade_aspect_y > arcade_aspect_x * context->config.monitor_aspect_y;

		log_std(("emu:video: best aspect factor %dx%d (expansion %g)\n", (unsigned)factor_x, (unsigned)factor_y, (double)context->config.aspect_expansion_factor));

		/* compute the best mode */
		if (context->state.mode_aspect_vertgameinhorzscreen) {
			best_size_y = context->state.game_used_size_y;
			best_size_x = adjust_step(context->state.game_used_size_y * factor_x, factor_y, CRTC_HSTEP);
			best_size_2y = 2*context->state.game_used_size_y;
			best_size_2x = adjust_step(2 * context->state.game_used_size_y * factor_x, factor_y, CRTC_HSTEP);
			best_size_3y = 3*context->state.game_used_size_y;
			best_size_3x = adjust_step(3 * context->state.game_used_size_y * factor_x, factor_y, CRTC_HSTEP);
			best_size_4y = 4*context->state.game_used_size_y;
			best_size_4x = adjust_step(4 * context->state.game_used_size_y * factor_x, factor_y, CRTC_HSTEP);
		} else {
			best_size_x = adjust_step(context->state.game_used_size_x, 1, CRTC_HSTEP);
			best_size_y = context->state.game_used_size_x * factor_y / factor_x;
			best_size_2x = adjust_step(2 * context->state.game_used_size_x, 1, CRTC_HSTEP);
			best_size_2y = 2*context->state.game_used_size_x * factor_y / factor_x;
			best_size_3x = adjust_step(3 * context->state.game_used_size_x, 1, CRTC_HSTEP);
			best_size_3y = 3*context->state.game_used_size_x * factor_y / factor_x;
			best_size_4x = adjust_step(4 * context->state.game_used_size_x, 1, CRTC_HSTEP);
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
				/* generate modes for the overlay driver */
				video_init_crtc_make_fake(context, "generate", best_size_x, best_size_y);
				video_init_crtc_make_fake(context, "generate-double", best_size_2x, best_size_2y);
				video_init_crtc_make_fake(context, "generate-triple", best_size_3x, best_size_3y);
				video_init_crtc_make_fake(context, "generate-quad", best_size_4x, best_size_4y);
			}
		} else {
			if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY) != 0) {
				/* generate modes for the overlay driver */
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
	context->state.buffer_index32_map = (uint32*)malloc(context->state.palette_total * sizeof(uint32));
	context->state.buffer_index16_map = (uint16*)malloc(context->state.palette_total * sizeof(uint16));
	context->state.buffer_index8_map = (uint8*)malloc(context->state.palette_total * sizeof(uint8));

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
		context->state.buffer_index32_map[i] = 0;
		context->state.buffer_index16_map[i] = 0;
		context->state.buffer_index8_map[i] = 0;
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
	free(context->state.buffer_index32_map);
	context->state.buffer_index32_map = 0;
	free(context->state.buffer_index16_map);
	context->state.buffer_index16_map = 0;
	free(context->state.buffer_index8_map);
	context->state.buffer_index8_map = 0;
}

/***************************************************************************/
/* Frame */

static void video_buffer_clear(struct advance_video_context* context)
{
	adv_pixel color;
	unsigned bytes_per_pixel;
	unsigned x, y;

	assert(video_mode_is_active());

	/* on palettized modes it always return 0 */
	color = pixel_make_from_def(0, 0, 0, context->state.buffer_def);

	bytes_per_pixel = color_def_bytes_per_pixel_get(context->state.buffer_def);

	/* clear */
	if (color == 0) {
		/* fast clear to 0 */
		memset(context->state.buffer_ptr, 0, context->state.buffer_size_y * context->state.buffer_bytes_per_scanline);
	} else {
		for(y=0;y<context->state.buffer_size_y;++y) {
			unsigned char* p = context->state.buffer_ptr + y * context->state.buffer_bytes_per_scanline;
			for(x=0;x<context->state.buffer_size_x;++x) {
				cpu_uint_write(p, bytes_per_pixel, color);
				p += bytes_per_pixel;
			}
		}
	}
}

static unsigned pipeline_combine(unsigned p)
{
	if (p == 0)
		return VIDEO_COMBINE_BUFFER;
	else
		return 0;
}

static void video_recompute_pipeline(struct advance_video_context* context, const struct osd_bitmap* bitmap)
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
	unsigned p;

	/* check if the pipeline is already updated */
	if (context->state.blit_pipeline_flag)
		return;

	assert(*mode_name(&context->state.mode));

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
	case COMBINE_SCALEX :
		combine |= VIDEO_COMBINE_Y_SCALEX;
		break;
	case COMBINE_SCALEK :
		combine |= VIDEO_COMBINE_Y_SCALEK;
		break;
	case COMBINE_LQ :
		combine |= VIDEO_COMBINE_Y_LQ;
		break;
#ifndef USE_BLIT_SMALL
	case COMBINE_HQ :
		combine |= VIDEO_COMBINE_Y_HQ;
		break;
	case COMBINE_XBR :
		combine |= VIDEO_COMBINE_Y_XBR;
		break;
#endif
	default:
		combine |= VIDEO_COMBINE_Y_NONE;
		break;
	}

	combine_buffer = combine;
	combine_video = combine;

	free(context->state.buffer_ptr_alloc);

	for(p=0;p<PIPELINE_BLIT_MAX;++p)
		video_pipeline_init(&context->state.blit_pipeline[p]);
	video_pipeline_init(&context->state.buffer_pipeline_video);
	context->state.blit_pipeline_flag = 1;

	context->state.buffer_bytes_per_scanline = context->state.buffer_size_x * color_def_bytes_per_pixel_get(context->state.buffer_def);

	/* align at 32 bytes */
	context->state.buffer_bytes_per_scanline = ALIGN_UNSIGNED(context->state.buffer_bytes_per_scanline, 32);

	size = 32 + context->state.buffer_size_y * context->state.buffer_bytes_per_scanline;
	context->state.buffer_ptr_alloc = malloc(size);

	/* align at 32 bytes */
	context->state.buffer_ptr = ALIGN_PTR(context->state.buffer_ptr_alloc, 32);

	/* clear */
	video_buffer_clear(context);

	video_pipeline_target(&context->state.buffer_pipeline_video, context->state.buffer_ptr, context->state.buffer_bytes_per_scanline, context->state.buffer_def);

	if (context->state.game_rgb_flag) {
		for(p=0;p<PIPELINE_BLIT_MAX;++p)
			video_pipeline_direct(&context->state.blit_pipeline[p], context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.game_color_def, combine_video | pipeline_combine(p));
		video_pipeline_direct(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, context->state.game_color_def, combine_buffer);
	} else {
		if (context->state.mode_index == MODE_FLAGS_INDEX_PALETTE8) {
			assert(context->state.game_bytes_per_pixel == 2);
			for(p=0;p<PIPELINE_BLIT_MAX;++p)
				video_pipeline_palette16hw(&context->state.blit_pipeline[p], context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, combine_video | pipeline_combine(p));
			video_pipeline_palette16hw(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, combine_buffer);
		} else {
			switch (context->state.game_bytes_per_pixel) {
				case 1 :
					for(p=0;p<PIPELINE_BLIT_MAX;++p)
						video_pipeline_palette8(&context->state.blit_pipeline[p], context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.palette_index8_map, context->state.palette_index16_map, context->state.palette_index32_map, combine_video | pipeline_combine(p));
					/* use the alternate palette only if required */
					if (context->state.buffer_def != video_color_def())
						video_pipeline_palette8(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, context->state.buffer_index8_map, context->state.buffer_index16_map, context->state.buffer_index32_map, combine_buffer);
					else
						video_pipeline_palette8(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, context->state.palette_index8_map, context->state.palette_index16_map, context->state.palette_index32_map, combine_buffer);
					break;
				case 2 :
					for(p=0;p<PIPELINE_BLIT_MAX;++p)
						video_pipeline_palette16(&context->state.blit_pipeline[p], context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.palette_index8_map, context->state.palette_index16_map, context->state.palette_index32_map, combine_video | pipeline_combine(p));
					/* use the alternate palette only if required */
					if (context->state.buffer_def != video_color_def())
						video_pipeline_palette16(&context->state.buffer_pipeline_video, intermediate_mode_visible_size_x, intermediate_mode_visible_size_y, intermediate_game_visible_size_x, intermediate_game_visible_size_y, context->state.buffer_src_dw, context->state.buffer_src_dp, context->state.buffer_index8_map, context->state.buffer_index16_map, context->state.buffer_index32_map, combine_buffer);
					else
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

		for(p=0;p<PIPELINE_BLIT_MAX;++p) {
			log_std(("emu:video: pipeline_video %d\n", p));
			for(i=1, stage=video_pipeline_begin(&context->state.blit_pipeline[p]);stage!=video_pipeline_end(&context->state.blit_pipeline[p]);++stage, ++i) {
				if (stage == video_pipeline_pivot(&context->state.blit_pipeline[p])) {
					snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.blit_pipeline[p])->type));
					++i;
					log_std(("emu:video: %s\n", buffer));
				}
				if (stage->sbpp != stage->sdp)
					snprintf(buffer, sizeof(buffer), "(%d) %s, p %d, dp %d", i, pipe_name(stage->type), stage->sbpp, stage->sdp);
				else
					snprintf(buffer, sizeof(buffer), "(%d) %s, p %d", i, pipe_name(stage->type), stage->sbpp);
				log_std(("emu:video: %s\n", buffer));
			}
			if (stage == video_pipeline_pivot(&context->state.blit_pipeline[p])) {
				snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.blit_pipeline[p])->type));
				++i;
				log_std(("emu:video: %s\n", buffer));
			}
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

	/* initialize the pipepeline measure */
	context->state.pipeline_measure_flag = 1;
	context->state.pipeline_measure_i = 0;
	context->state.pipeline_measure_j = 0;
	context->state.blit_pipeline_index = 0;
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

	/* draw the onscreen direct interface */
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

	/* check if the ui requires a buffered write */
	ui_buffer_active = advance_ui_buffer_active(ui_context);

	/* use buffered or direct write to screen ? */
	buffer_flag = 0;

	/* if ui active use the buffer */
	if (ui_buffer_active) {
		buffer_flag = 1;
	}

	start = 0;
	stop = 0;

	/* no buffering is used */
	if (!buffer_flag) {
		/* start measure */
		start = target_clock();
	}

	if (buffer_flag) {
		/* buffered write on screen */
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

		/* draw the game image in the buffer */
		/* the image is rotated to be correctly orientated in this stage to allow an easy ui update */
		video_pipeline_blit(&context->state.buffer_pipeline_video, dst_x, dst_y, (unsigned char*)bitmap->ptr + src_offset);

		/* draw the user interface */
		if (ui_buffer_active) {
			advance_ui_buffer_update(ui_context, context->state.buffer_ptr, context->state.buffer_size_x, context->state.buffer_size_y, context->state.buffer_bytes_per_scanline, context->state.buffer_def, context->state.palette_map, context->state.palette_total);
		}

		buf_ptr = context->state.buffer_ptr;
		buf_dw = context->state.buffer_bytes_per_scanline;
		buf_dp = color_def_bytes_per_pixel_get(context->state.buffer_def);

#if 0 /* OSDEF: Save interface image, only for debugging. */
		{
			struct advance_input_context* input_context = &CONTEXT.input;
			if (advance_input_digital_pressed(input_context, DIGITAL_KBD(0, KEYB_ENTER_PAD))) {
				adv_fz* f;
				static unsigned in = 1;
				char buffer[64];

				snprintf(buffer, sizeof(buffer), "im%d.png", in);
				++in;

				f = fzopen(buffer, "wb");
				advance_record_png_write(f, buf_ptr, context->state.buffer_size_x, context->state.buffer_size_y, buf_dp, buf_dw, context->state.buffer_def, 0, 0, 0);
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

		/* blit the buffer */
		/* the image is rotated to the user requested orientation in this stage */
		/* the whole buffer is blitted, implying a slowdown for vertical games on horizontal monitors */
		video_stretch_direct(x, y, video_size_x(), video_size_y(), buf_ptr, final_size_x, final_size_y, buf_dw, buf_dp, context->state.buffer_def, 0);

		if (ui_buffer_active) {
			/* always clear the buffer for the next update */
			/* because the ui may write over the game area */
			video_buffer_clear(context);
		}
	} else {
		/* direct write on screen */

		/* compute the source pointer */
		src_offset = context->state.blit_src_offset + context->state.game_visible_pos_y * context->state.blit_src_dw + context->state.game_visible_pos_x * context->state.blit_src_dp;

		/* blit directly on the video */
		video_pipeline_blit(&context->state.blit_pipeline[context->state.blit_pipeline_index], dst_x + x, dst_y + y, (unsigned char*)bitmap->ptr + src_offset);
	}

	/* no buffering is used */
	if (!buffer_flag) {
		/* end measure */
		stop = target_clock();

		stop -= start;

		/* if we are in estimation phase */
		if (context->state.pipeline_measure_flag) {

			context->state.pipeline_measure_map[context->state.pipeline_measure_i][context->state.pipeline_measure_j] = stop;
			++context->state.pipeline_measure_i;

			if (context->state.pipeline_measure_i == PIPELINE_BLIT_MAX) {
				context->state.pipeline_measure_i = 0;
				++context->state.pipeline_measure_j;

				if (context->state.pipeline_measure_j == PIPELINE_MEASURE_MAX) {
					unsigned i;

					for(i=0;i<PIPELINE_BLIT_MAX;++i) {
						context->state.pipeline_measure_result[i] = adv_measure_median(0.00001, 0.5, context->state.pipeline_measure_map[i], PIPELINE_MEASURE_MAX);
						log_std(("emu:video: pipeline %d -> time %g\n", i, context->state.pipeline_measure_result[i]));
					}

					context->state.pipeline_measure_i = 0;

					/* The first one is selected, then next one is selected only if a 3% gain is measured */
					for(i=1;i<PIPELINE_BLIT_MAX;++i) {
						if (context->state.pipeline_measure_result[i] < 0.97 * context->state.pipeline_measure_result[context->state.pipeline_measure_i]) {
							context->state.pipeline_measure_i = i;
						}
					}

					log_std(("emu:video: best is %d\n", context->state.pipeline_measure_i));

					/* end the measure process */
					context->state.pipeline_measure_flag = 0;
				}
			}

			context->state.blit_pipeline_index = context->state.pipeline_measure_i;
		}

		context->state.pipeline_timing_map[context->state.pipeline_timing_i] = stop;

		++context->state.pipeline_timing_i;
		if (context->state.pipeline_timing_i == PIPELINE_MEASURE_MAX) {
			context->state.pipeline_timing_i = 0;
		}
	}
}

static void video_frame_screen(struct advance_video_context* context, struct advance_ui_context* ui_context, const struct osd_bitmap *bitmap)
{
	update_start();

	video_recompute_pipeline(context, bitmap);

	video_frame_put(context, ui_context, bitmap, update_x_get(), update_y_get());

	update_stop(update_x_get(), update_y_get(), video_size_x(), video_size_y(), context->state.vsync_flag);
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

							/* update only the currently used palette to not overload the memory cache */
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

							if (video_color_def() != context->state.buffer_def) {
								pixel = pixel_make_from_def(c.red, c.green, c.blue, context->state.buffer_def);
								/* update only the 32 bit palette, the others are never used */
								context->state.buffer_index32_map[p] = pixel;
							} else {
								switch (video_bytes_per_pixel()) {
								case 4 :
									context->state.buffer_index32_map[p] = pixel;
									break;
								case 2 :
									context->state.buffer_index16_map[p] = pixel;
									break;
								case 1 :
									context->state.buffer_index8_map[p] = pixel;
									break;
								}
							}
						}
					}

					t <<= 1;
				}
			}
		}
	}
}

static void video_frame_game(struct advance_video_context* context, struct advance_record_context* record_context, struct advance_ui_context* ui_context, const struct osd_bitmap *bitmap, adv_bool skip_flag)
{
	/* bitmap */
	if (!skip_flag) {
		video_frame_palette(context);
		video_frame_screen(context, ui_context, bitmap);

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

/**
 * Update the video drawing a frame.
 */
void advance_video_frame(struct advance_video_context* context, struct advance_record_context* record_context, struct advance_ui_context* ui_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, adv_bool skip_flag)
{
	if (context->state.debugger_flag) {
		video_frame_debugger(context, debug, debug_palette, debug_palette_size);
	} else {
		video_frame_game(context, record_context, ui_context, game, skip_flag);
	}
}

/***************************************************************************/
/* Mode */

/**
 * Preconfigure the video mode.
 * This is an early stage preconfiguration when almost no information
 * is available.
 */
void advance_video_mode_preinit(struct advance_video_context* context, struct mame_option* option)
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

			crtc_container_insert_default_active(&context->config.crtc_bag);
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

/**
 * Initialize the video mode.
 */
adv_error advance_video_mode_init(struct advance_video_context* context, struct osd_video_option* req)
{
	adv_mode mode;
	mode_reset(&mode);

	if (video_init_state(context, req) != 0) {
		return -1;
	}

	if (advance_video_update_index(context)!=0) {
		error_set("Unsupported bit depth.");
		return -1;
	}

	if (advance_video_update_selectedcrtc(context) != 0) {
		if (strcmp(context->config.resolution_buffer, "auto")==0)
			error_set("No video modes available for the current game.");
		else
			error_set("The specified 'display_mode %s' doesn't exist.", context->config.resolution_buffer);
		return -1;
	}

	if (video_make_crtc_for_game(context, &context->state.crtc_effective, context->state.crtc_selected) != 0) {
		error_set("Unable to generate the crtc values.");
		return -1;
	}

	advance_video_update_ui(context, &context->state.crtc_effective);

	advance_video_update_visible(context, &context->state.crtc_effective);
	advance_video_update_effect(context);

	if (video_make_vidmode(context, &mode, &context->state.crtc_effective) != 0) {
		return -1;
	}

	video_init_color(context, req);

	/* recenter */
	advance_video_update_pan(context);

	if (vidmode_update(context, &mode, 1) != 0) {
		return -1;
	}

	advance_video_update_skip(context);
	advance_video_update_sync(context);

	return 0;
}

/**
 * Deinitialize the video mode.
 */
void advance_video_mode_done(struct advance_video_context* context)
{
	if (context->config.restore_flag || context->state.measure_flag) {
		vidmode_done(context, 1);
		video_mode_restore();
	} else {
		vidmode_done(context, 0);
		if (video_mode_is_active())
			video_mode_done(0);
	}

	video_done_color(context);
	video_done_state(context);
}

/**
 * Update the video mode.
 * Recompute the video mode from the configuration variables.
 */
adv_error advance_video_mode_update(struct advance_video_context* context)
{
	adv_mode mode;

	mode_reset(&mode);

	if (advance_video_update_index(context) != 0) {
		return -1;
	}

	if (advance_video_update_selectedcrtc(context) != 0) {
		return -1;
	}

	if (video_make_crtc_for_game(context, &context->state.crtc_effective, context->state.crtc_selected) != 0) {
		return -1;
	}

	advance_video_update_ui(context, &context->state.crtc_effective);

	advance_video_update_visible(context, &context->state.crtc_effective);
	advance_video_update_effect(context);

	if (video_make_vidmode(context, &mode, &context->state.crtc_effective) != 0) {
		return -1;
	}

	/* recenter */
	advance_video_update_pan(context);

	if (vidmode_update(context, &mode, 0) != 0) {
		return -1;
	}

	advance_video_update_skip(context);
	advance_video_update_sync(context);

	advance_video_invalidate_screen(context);

	return 0;
}


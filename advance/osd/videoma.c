/*
 * This file is part of the AdvanceMAME project.
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
#include "glue.h"

#include "os.h"
#include "video.h"
#include "update.h"
#include "generate.h"
#include "crtcbag.h"
#include "blit.h"
#include "hscript.h"
#include "script.h"
#include "conf.h"
#include "videoall.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>

/** Max framskip factor */
#define SYNC_MAX 12

/***************************************************************************/
/*
	Description of the various coordinate systems and variables :

	game_[width,height]
		Size of the game screen (after any flipxy or swapxy).
	game_used_[width,height]
		Size of the used part of the game screen.
	game_used_[pos_x,pos_y]
		Position of the used part of the game screen.
	game_visible_[width,height] (old gfx_display_columns, gfx_display_lines)
		Size of the visible part of the game screen. Can be smaller
		than game_used_* if the video mode is too small. game_visible_
		is always internal of the game_used part.
	game_visible_[pos_x,pos_y] (old skipcolumns,skiplines)
		Position of the visible part in the game screen (not
		in the game_used part).
	screen_visible_*
		Part of the screen used for drawing. game_visible_* area is
		drawn in screen_visible_* area, eventually with stretch_request.
		screen_visible may be smaller than screen if game don't have
		the same aspect ratio of the screen.
	screen_stretch
		Type of stretching done (is different than stretch_request variable)
			SCREEN_NONE 1 multiplier
			SCREEN_INTEGER interger multiplier
			SCREEN_FRACTIONAL fractional multiplier
		This value is derivate from game_visible_size and screen_visible_size
*/

/*
Aspetto:

pc_aspect_ratio
	Sono le le dimensioni dello schermo di un normale pc 4 (width),
	3 (height). Invertite se il the_monitor Š montato verticale.

arcade_aspect_ratio
	Sono le le dimensioni dello schermo di un arcade 4,3
	se orizzontale e 3,4 se verticale

game_aspect_ratio
	E' la dimensione in pixel ARCADE di un quadrato (a vista)
	disegnato sullo schermo ARCADE alla particolare risoluzione ARCADE
	FIX.game_width, FIX.game_height.

	E' la combinazione di arcade_aspect_ratio con la particolare
	risoluzione FIX.game_width,FIX.game_height.

	arcade_aspect_ratio_x   game_aspect_ratio_x   game_use_size_x
	--------------------- * ------------------- = -----------
	arcade_aspect_ratio_y   game_aspect_ratio_y   game_used_size_y

screen_aspect_ratio
	E' la dimensione in pixel PC di un quadrato (a vista)
	disegnato sullo schermo PC alla particolare risoluzione PC
	mode->size_x, mode->size_y.

	E' la combinazione di pc_aspect_ratio con la particolare
	risoluzione mode->size_x,mode->size_y.

	pc_aspect_ratio_x   screen_aspect_ratio_x   screen_size_x
	----------------- * --------------------- = -------------
	pc_aspect_ratio_y   screen_aspect_ratio_y   screen_size_y


resulting_aspect_ratio
	E' la dimensione in pixel ARCADE di un quadrato (a vista)
	disegnato sullo schermo PC.

	Idealmente dovrebbe essere uguale a game_aspect_ratio.
	Se non c'e' stretching e' uguale per definizione a
	screen_aspect ratio.

	screen_stretch_factor_x   resulting_aspect_ratio_x   screen_aspect_ratio_x
	----------------------- * ------------------------ = ---------------------
	screen_stretch_factor_y   resulting_aspect_ratio_y   screen_aspect_ratio_y

	screen_stretch_factor_x   pc_aspect_ratio_x   resulting_aspect_ratio_x   screen_size_x
	----------------------- * ----------------- * ------------------------ = -------------
	screen_stretch_factor_y   pc_aspect_ratio_y   resulting_aspect_ratio_y   screen_size_y
*/

/***************************************************************************/
/* Save */

void advance_video_save(struct advance_video_context* context, const char* section) {
	conf_string_set(context->state.cfg_context, section, "display_mode", context->config.resolution);
	if (!context->state.game_vector_flag) {
		conf_int_set(context->state.cfg_context, section, "display_resizeeffect", context->config.combine);
		conf_int_set(context->state.cfg_context, section, "display_rgbeffect", context->config.effect);
		conf_int_set(context->state.cfg_context, section, "display_resize", context->config.stretch);
		conf_bool_set(context->state.cfg_context, section, "display_magnify", context->config.magnify_flag);
		conf_int_set(context->state.cfg_context, section, "display_depth", context->config.depth);
		conf_bool_set(context->state.cfg_context, section, "display_rgb", context->config.rgb_flag);
		if (context->state.game_visible_size_x < context->state.game_used_size_x
			|| context->state.game_visible_size_y < context->state.game_used_size_y)
		{
			if (context->config.skipcolumns>=0) {
				char buffer[32];
				sprintf(buffer,"%d",context->config.skipcolumns);
				conf_string_set(context->state.cfg_context, section, "display_skipcolumns", buffer);
			} else
				conf_string_set(context->state.cfg_context, section, "display_skipcolumns", "auto");
			if (context->config.skiplines>=0) {
				char buffer[32];
				sprintf(buffer,"%d",context->config.skiplines);
				conf_string_set(context->state.cfg_context, section, "display_skiplines", buffer);
			} else
				conf_string_set(context->state.cfg_context, section, "display_skiplines", "auto");
		} else {
			conf_remove(context->state.cfg_context, section, "display_skipcolumns");
			conf_remove(context->state.cfg_context, section, "display_skiplines");
		}
		conf_bool_set(context->state.cfg_context, section, "display_scanlines", context->config.scanlines_flag);
		conf_bool_set(context->state.cfg_context, section, "display_vsync", context->config.vsync_flag);
	}

	mame_ui_message("Video options saved in %s/",section);
}

/***************************************************************************/
/* Update */

/**
 * Adjust the CRTC value.
 * \return 0 on success
 */
static int video_make_crtc(struct advance_video_context* context, video_crtc* crtc, const video_crtc* original_crtc)
{
	*crtc = *original_crtc;

	if ((context->config.adjust & ADJUST_ADJUST_X) != 0) {
		unsigned best_size_x;

		if (context->config.magnify_flag) {
			best_size_x = context->state.mode_best_size_2x;
		} else {
			best_size_x = context->state.mode_best_size_x;
		}

		crtc_hsize_set(crtc, best_size_x);
	}

	if ((context->config.adjust & ADJUST_ADJUST_CLOCK) != 0) {

		crtc_vclock_set(crtc,context->state.game_fps);

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_vclock_set(crtc, 2 * context->state.game_fps);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			crtc_vclock_set(crtc, 3 * context->state.game_fps);
		}

		if (!crtc_clock_check(&context->config.monitor, crtc)) {
			if (crtc_adjust_clock(crtc, &context->config.monitor) != 0) {
				/* ignore error */
			}
		}
	}

	if (!crtc_clock_check(&context->config.monitor, crtc)) {
		return -1;
	}

	return 0;
}

/**
 * Update the skip state.
 */
static void video_update_skip(struct advance_video_context* context) {
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
			os_log(("advance:video: vsync disabled because throttle disactive\n"));
			context->state.vsync_flag = 0;
		}

		/* disable if turbo active */
		if (context->state.turbo_flag) {
			os_log(("advance:video: vsync disabled because turbo active\n"));
			context->state.vsync_flag = 0;
		}

		/* disable if fastest active */
		if (context->state.fastest_flag) {
			os_log(("advance:video: vsync disabled because fastest active\n"));
			context->state.vsync_flag = 0;
		}

		/* disable if measure active */
		if (context->state.measure_flag) {
			os_log(("advance:video: vsync disabled because measure active\n"));
			context->state.vsync_flag = 0;
		}

		/* disable the vsync flag if the frame rate is wrong */
		if (context->state.mode_vclock < reference * 0.97
			|| context->state.mode_vclock > reference * 1.03) {
			os_log(("advance:video: vsync disabled because the vclock is too different %g %g\n",reference,context->state.mode_vclock));
			context->state.vsync_flag = 0;
		}
	}
}

/**
 * Select the video depth.
 * \return 0 on success
 */
static int video_update_depthindex(struct advance_video_context* context) {

	unsigned select_pref_8[] = { 8, 16, 15, 32, 0 };
	unsigned select_pref_15[] = { 15, 16, 32, 8, 0 };
	unsigned select_pref_16[] = { 16, 15, 32, 8, 0 };
	unsigned select_pref_32[] = { 32, 16, 15, 8, 0 };
	unsigned* select;
	unsigned bits_per_pixel;

	int mode_may_be_palette = !context->state.game_rgb_flag
		&& context->state.game_colors <= 256
		&& !context->config.rgb_flag;

	if (context->config.depth == 0) {
		if (mode_may_be_palette) {
			bits_per_pixel = 8; /* for hardware palette */
		} else {
			bits_per_pixel = context->state.game_bits_per_pixel;
		}
	} else {
		bits_per_pixel = context->config.depth;
	}

	switch (bits_per_pixel) {
		case 8 : select = select_pref_8; break;
		case 15 : select = select_pref_15; break;
		case 16 : select = select_pref_16; break;
		case 32 : select = select_pref_32; break;
		default: return -1;
	}

	while (*select) {
		int flag;
		switch (*select) {
			case 8 : flag = VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT; break;
			case 15 : flag = VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT; break;
			case 16 : flag = VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT; break;
			case 32 : flag = VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT; break;
			default: return -1;
		}
		if ((video_mode_generate_driver_flags() & flag) != 0)
			break;
		++select;
	}

	if (!*select)
		return -1;

	context->state.mode_bits_per_pixel = *select;

	if (mode_may_be_palette && context->state.mode_bits_per_pixel == 8) {
		context->state.mode_rgb_flag = 0;
	} else {
		context->state.mode_rgb_flag = 1;
	}

	return 0;
}

/**
 * Create the video mode from the crtc.
 * \return 0 on success
 */
static int video_make_mode(struct advance_video_context* context, video_mode* mode, const video_crtc* crtc) {
	if (video_mode_generate(mode,crtc,context->state.mode_bits_per_pixel,VIDEO_FLAGS_TYPE_GRAPHICS | VIDEO_FLAGS_INDEX_RGB)!=0) {
		return -1;
	}

	return 0;
}

/**
 * Clear all the screens.
 */
static void video_invalidate_screen(void) {
	unsigned i;

	assert( video_mode_is_active() );

	/* the entire video memory can't be cleared in some graphics cards (like neomagic -> crash) */
	for(i=0;i<update_page_max_get();++i) {
		update_start();
		video_clear(update_x_get(),update_y_get(),video_size_x(), video_size_y(),0);
		update_stop(0);
	}
}

/**
 * Set the video mode.
 * The mode is copyed in the context if it is set with success.
 * \return 0 on success
 */
static int video_init_mode(struct advance_video_context* context, video_mode* mode)
{
	assert( !context->state.mode_flag );

	if (video_mode_set(mode) != 0) {
		return -1;
	}

	/* save the video mode */
	context->state.mode_flag = 1;
	context->state.mode = *mode;

	if (context->state.mode_rgb_flag
		&& video_index() == VIDEO_FLAGS_INDEX_PACKED) {
		video_index_packed_to_rgb(0);
	}
	if (!context->state.mode_rgb_flag
		&& video_index() == VIDEO_FLAGS_INDEX_RGB) {
		video_index_rgb_to_packed();
	}

	/* initialize the blit pipeline */
	context->state.blit_pipeline_flag = 0;

	/* inizialize the stretch system */
	video_stretch_init();

	/* inizialize the update system */
	update_init( context->config.triplebuf_flag!=0 ? 3 : 1 );

	video_invalidate_screen();

	os_log(("advance:video: mode %s, size %dx%d, bits_per_pixel %d, bytes_per_scanline %d, pages %d\n",video_name(),video_size_x(), video_size_y(), video_bits_per_pixel(), video_bytes_per_scanline(), update_page_max_get()));

	return 0;
}

static void video_done_mode(struct advance_video_context* context, int restore) {
	assert( context->state.mode_flag );

	/* clear all the video memory used */
	if (restore)
		video_invalidate_screen();

	if (context->state.blit_pipeline_flag) {
		video_pipeline_done(&context->state.blit_pipeline);
		context->state.blit_pipeline_flag = 0;
	}

	video_stretch_done();
	update_done();

	context->state.mode_flag = 0;
}

static int video_update_mode(struct advance_video_context* context, video_mode* mode) {

	/* destroy the pipeline, this force the pipeline update */
	if (context->state.blit_pipeline_flag) {
		video_pipeline_done(&context->state.blit_pipeline);
		context->state.blit_pipeline_flag = 0;
	}

	if (!context->state.mode_flag
		|| video_mode_compare(mode,video_current_mode())!=0
	) {
		if (context->state.mode_flag)
			video_done_mode(context, 0);
		if (video_init_mode(context,mode) != 0) {
			return -1;
		}
	} else {
		if (context->state.mode_rgb_flag
			&& video_index() == VIDEO_FLAGS_INDEX_PACKED) {
			video_index_packed_to_rgb(0);
		}
		if (!context->state.mode_rgb_flag
			&& video_index() == VIDEO_FLAGS_INDEX_RGB) {
			video_index_rgb_to_packed();
		}
	}

	return 0;
}

/**
 * Update the panning state.
 * \precondition The video configuration in the context must be already initialized.
 */
static void video_update_pan(struct advance_video_context* context) {
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

	os_log(("advance:video: game_visible_pos_x %d\n",context->state.game_visible_pos_x));
	os_log(("advance:video: game_visible_pos_y %d\n",context->state.game_visible_pos_y));
	os_log(("advance:video: game_visible_size_x %d\n",context->state.game_visible_size_x));
	os_log(("advance:video: game_visible_size_y %d\n",context->state.game_visible_size_y));

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

	os_log(("advance:video: mame_ui_area_set(min_x:%d,min_y:%d,max_x:%d,max_y:%d)\n", pos_x, pos_y, pos_x+size_x-1,pos_y+size_y-1));

	mame_ui_area_set(pos_x, pos_y, pos_x+size_x-1,pos_y+size_y-1);
}

/**
 * Update the effects state.
 * \precondition The video configuration in the context must be already initialized.
 * \precondition The depth in the context must be already initialized
 */
static void video_update_effect(struct advance_video_context* context)
{
	double previous_gamma_factor;

	context->state.effect = context->config.effect;
	context->state.combine = context->config.combine;

	if (context->state.combine == COMBINE_AUTO) {
		if (context->state.mode_visible_size_x == 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y == 2*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_SCALE2X;
		} else if (context->state.mode_visible_size_x >= 2*context->state.game_visible_size_x
			&& context->state.mode_visible_size_y >= 2*context->state.game_visible_size_y) {
			context->state.combine = COMBINE_FILTER;
		} else {
			if (context->config.inlist_combinemax_flag) {
				context->state.combine = COMBINE_MAX;
			} else {
				context->state.combine = COMBINE_MEAN;
			}
		}
	}

	if ((context->state.combine == COMBINE_MEAN || context->state.combine == COMBINE_MAX || context->state.combine == COMBINE_FILTER || context->state.combine == COMBINE_FILTERX || context->state.combine == COMBINE_FILTERY)
		&& !context->state.mode_rgb_flag) {
		os_log(("advance:video: resizeeffect=* disabled because we are in a palettized mode\n"));
		context->state.combine = COMBINE_NONE;
	}

	if (context->state.combine == COMBINE_SCALE2X
		&& (context->state.mode_visible_size_x != 2*context->state.game_visible_size_x
			|| context->state.mode_visible_size_y != 2*context->state.game_visible_size_y
		)
	) {
		os_log(("advance:video: resizeeffect=scale2x disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	/* max only in y reduction */
	if (context->state.combine == COMBINE_MAX
		&& context->state.mode_visible_size_y >= context->state.game_visible_size_y) {
		os_log(("advance:video: resizeeffect=max disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	/* mean only in y change */
	if (context->state.combine == COMBINE_MEAN
		&& context->state.mode_visible_size_y == context->state.game_visible_size_y) {
		os_log(("advance:video: resizeeffect=mean disabled because the wrong mode size\n"));
		context->state.combine = COMBINE_NONE;
	}

	if (context->state.effect != EFFECT_NONE
		&& !context->state.mode_rgb_flag) {
		os_log(("advance:video: rgbeffect=* disabled because we are in a palettized mode\n"));
		context->state.effect = EFFECT_NONE;
	}

	previous_gamma_factor = context->state.gamma_effect_factor;

	/* adjust the gamma settings */
	switch (context->state.effect) {
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

	mame_ui_gamma_factor_set( context->state.gamma_effect_factor / previous_gamma_factor );
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
		context->state.palette_is_dirty = 1;
		for(i=0;i<context->state.palette_dirty_total;++i)
			context->state.palette_dirty_map[i] = osd_mask_full;
	}
}

/**
 * Check if a modeline is acceptable.
 * The complete modeline processing is done.
 */
static int is_crtc_acceptable(struct advance_video_context* context, const video_crtc* crtc) {
	video_mode mode;
	video_crtc temp_crtc;

	/* try to adjust the crtc if required */
	if (video_make_crtc(context,&temp_crtc,crtc) != 0)
		return 0;

	/* try creating a standard 8 bit mode */
	/* the 8 bit mode is used because is for sure supported in any conditions */
	if (video_mode_generate(&mode,&temp_crtc,8,VIDEO_FLAGS_TYPE_GRAPHICS | VIDEO_FLAGS_INDEX_RGB)!=0) {
		/* generally this fail due the limitation os the video drivers */
		/* for example the vgaline drivers accepts only some clocks */
		return 0;
	}

	return 1;
}

/**
 * Check if a modeline is acceptable.
 * Only a partial processing is done. No information on the game are used.
 * This function is theorically equivalent at the is_crtc_acceptable function.
 */
static int is_crtc_acceptable_preventive(struct advance_video_context* context, const video_crtc* crtc) {
	video_mode mode;
	video_crtc temp_crtc = *crtc;

	if ((context->config.adjust & ADJUST_ADJUST_CLOCK) != 0) {
		if (!crtc_clock_check(&context->config.monitor, &temp_crtc)) {
			if (crtc_adjust_clock(&temp_crtc, &context->config.monitor) != 0) {
				/* ignore error */
			}
		}
	}

	if (!crtc_clock_check(&context->config.monitor, &temp_crtc)) {
		return 0;
	}

	/* try creating a standard 8 bit mode */
	/* the 8 bit mode is used because is for sure supported in any conditions */
	if (video_mode_generate(&mode,&temp_crtc,8,VIDEO_FLAGS_TYPE_GRAPHICS | VIDEO_FLAGS_INDEX_RGB)!=0) {
		/* generally this fail due the limitation os the video drivers */
		/* for example the vgaline drivers accepts only some clocks */
		return 0;
	}

	return 1;
}

static void video_update_visible(struct advance_video_context* context, const video_crtc* crtc) {
	unsigned factor_x;
	unsigned factor_y;
	unsigned screen_aspect_ratio_x;
	unsigned screen_aspect_ratio_y;
	unsigned arcade_aspect_ratio_x;
	unsigned arcade_aspect_ratio_y;

	assert( crtc );

	screen_aspect_ratio_x = crtc_hsize_get(crtc) * pc_aspect_ratio_y;
	screen_aspect_ratio_y = crtc_vsize_get(crtc) * pc_aspect_ratio_x;
	video_aspect_reduce(&screen_aspect_ratio_x,&screen_aspect_ratio_y);

	arcade_aspect_ratio_x = context->state.game_used_size_x * context->state.game_aspect_y;
	arcade_aspect_ratio_y = context->state.game_used_size_y * context->state.game_aspect_x;
	video_aspect_reduce(&arcade_aspect_ratio_x,&arcade_aspect_ratio_y);

	factor_x = screen_aspect_ratio_x * arcade_aspect_ratio_x;
	factor_y = screen_aspect_ratio_y * arcade_aspect_ratio_y;
	video_aspect_reduce(&factor_x,&factor_y);

	/* compute screen_visible size */
	if (pc_aspect_ratio_x * arcade_aspect_ratio_y > arcade_aspect_ratio_x * pc_aspect_ratio_y) {
		/* vertical game in horizontal screen */
		context->state.mode_visible_size_y = crtc_vsize_get(crtc);
		/* adjust to 8 pixel */
		context->state.mode_visible_size_x = crtc_step( (double)context->state.mode_visible_size_y * factor_x / factor_y, 8);
		if (context->state.mode_visible_size_x > crtc_hsize_get(crtc))
			context->state.mode_visible_size_x = crtc_hsize_get(crtc);
	} else {
		/* orizontal game in vertical screen */
		context->state.mode_visible_size_x = crtc_hsize_get(crtc);
		context->state.mode_visible_size_y = context->state.mode_visible_size_x * factor_y / factor_x;
	}

	if (context->config.stretch == STRETCH_FRACTIONAL_XY) {

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

	} else if (context->config.stretch == STRETCH_INTEGER_X_FRACTIONAL_Y) {

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
	} else if (context->config.stretch == STRETCH_INTEGER_XY) {
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
static int video_resolution_cmp(const char* resolution, const char* name) {
	int rl;
	int nl;

	/* match the exact mode name */
	if (strcmp(resolution,name)==0)
		return 0;

	/* LEGACY (to be removed) */
	/* match the old video configuration format "NAME-XxY-XxY" */
	rl = strlen(resolution);
	nl = strlen(name);
	if (rl > nl
		&& strncmp(resolution,name,nl)==0
		&& resolution[nl]=='-'
		&& strspn(resolution+nl,"-0123456789x")==strlen(resolution+nl))
		return 0;

	return -1;
}

/**
 * Select the current video configuration.
 */
static int video_update_crtc(struct advance_video_context* context) {
	video_crtc_container_iterator i;
	const video_crtc* crtc = 0;
	int j;

	/* build the vector of config pointer */
	context->state.crtc_mac = 0;

	os_log(("advance:mode:select\n"));

	for(video_crtc_container_iterator_begin(&i,&context->config.crtc_bag);!video_crtc_container_iterator_is_end(&i);video_crtc_container_iterator_next(&i)) {
		video_crtc* crtc = video_crtc_container_iterator_get(&i);
		if (is_crtc_acceptable(context,crtc)) {
			if (context->state.crtc_mac < VIDEO_CRTC_MAX) {
				context->state.crtc_map[context->state.crtc_mac] = crtc;
				++context->state.crtc_mac;
			}
		} else {
			char buffer[256];
			video_crtc_print(buffer, crtc);
			os_log(("advance:excluded: modeline:\"%s\"\n", buffer));
		}
	}

	os_log(("advance:mode:sort\n"));

	crtc_sort(context, context->state.crtc_map, context->state.crtc_mac);

	for(j=0;j<context->state.crtc_mac;++j) {
		char buffer[256];
		video_crtc_print(buffer, context->state.crtc_map[j]);
		os_log(("advance:mode: %3d modeline:\"%s\"\n", j, buffer));
	}

	if (strcmp(context->config.resolution,"auto")!=0) {
		int i;
		for(i=0;i<context->state.crtc_mac;++i) {
			if (video_resolution_cmp(context->config.resolution,crtc_name_get(context->state.crtc_map[i])) == 0
				&& is_crtc_acceptable(context, context->state.crtc_map[i]))
			{
				crtc = context->state.crtc_map[i];
				break;
			}
		}
	} else {
		int i;
		for(i=0;i<context->state.crtc_mac;++i) {
			if (is_crtc_acceptable(context, context->state.crtc_map[i])) {
				crtc = context->state.crtc_map[i];
				break;
			}
		}
	}

	if (!crtc) {
		if (strcmp(context->config.resolution,"auto")==0)
			fprintf(stderr,"No video modes available for this game\n");
		else
			fprintf(stderr,"The specified 'display_mode %s' does not exist\n",context->config.resolution);
		return -1;
	}

	context->state.crtc_selected = crtc;

	return 0;
}

/***************************************************************************/
/* Initialization */

/**
 * Search the modeline with the nearest height not lower.
 */
static const video_crtc* video_init_crtc_bigger_find(struct advance_video_context* context, unsigned size_y) {
	video_crtc_container_iterator j;
	video_crtc* best_crtc = 0;

	for(video_crtc_container_iterator_begin(&j,&context->config.crtc_bag);!video_crtc_container_iterator_is_end(&j);video_crtc_container_iterator_next(&j)) {
		video_crtc result;
		video_crtc* crtc = video_crtc_container_iterator_get(&j);
		if (video_make_crtc(context,&result,crtc) == 0
			&& crtc_vsize_get(&result) >= size_y) {
			if (!best_crtc || crtc_vsize_get(best_crtc) > crtc_vsize_get(&result)) {
				best_crtc = crtc;
			}
		}
	}

	return best_crtc;
}

/**
 * Search the modeline with the nearest height not bigger.
 */
static const video_crtc* video_init_crtc_smaller_find(struct advance_video_context* context, unsigned size_y) {
	video_crtc_container_iterator j;
	video_crtc* best_crtc = 0;

	for(video_crtc_container_iterator_begin(&j,&context->config.crtc_bag);!video_crtc_container_iterator_is_end(&j);video_crtc_container_iterator_next(&j)) {
		video_crtc result;
		video_crtc* crtc = video_crtc_container_iterator_get(&j);
		if (video_make_crtc(context,&result,crtc) == 0
			&& crtc_vsize_get(&result) <= size_y) {
			if (!best_crtc || crtc_vsize_get(best_crtc) < crtc_vsize_get(&result)) {
				best_crtc = crtc;
			}
		}
	}

	return best_crtc;
}

/**
 * Compute and insert in the main list an X size adjusted modeline.
 */
static void video_init_crtc_make_adjustx(struct advance_video_context* context, const char* name, unsigned best_size_x, unsigned best_size_y, const video_crtc* original_crtc) {
	char buffer[256];
	video_crtc crtc;
	unsigned x_factor;

	double y_factor = crtc_vsize_get(original_crtc) / (double)best_size_y;
	int y_factor_int = floor(y_factor + 0.5);
	unsigned x_streched;
	if (y_factor_int < 1)
		y_factor_int = 1;

	crtc = *original_crtc;
	x_streched = best_size_x * y_factor_int;

	for(x_factor=1;x_factor<5;++x_factor) {
		crtc_hsize_set(&crtc,x_streched * x_factor);
		if (monitor_pclock_min(&context->config.monitor) < crtc_pclock_get(&crtc))
			break;
	}

	if (!crtc_clock_check(&context->config.monitor,&crtc))
		return;

	strcpy(crtc.name,name);

	video_crtc_print(buffer,&crtc);
	os_log(("advance:generate: modeline \"%s\"\n",buffer));

	video_crtc_container_insert(&context->config.crtc_bag,&crtc);

	if (context->config.stretch == STRETCH_FRACTIONAL_XY) {
		os_log(("advance:video: fractional coverted in mixed because generate/adjustx is active\n"));
		context->config.stretch = STRETCH_INTEGER_X_FRACTIONAL_Y;
	}
}

/**
 * Compute and insert in the main list a new modeline with the specified parameter.
 */
static const video_crtc* video_init_crtc_make_raster(struct advance_video_context* context, const char* name, unsigned size_x, unsigned size_y, double vclock, int force_scanline, int force_interlace) {
	char buffer[256];
	video_crtc crtc;
	int err = -1;
	const video_crtc* ret;

	if (force_scanline) {
		/* use only single scanline modes */
		unsigned cap = video_mode_generate_driver_flags() & ~(VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE);
		/* try with a perfect mode */
		if (err != 0)
			err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_EXACT);
		/* try with a mode with different vclock but correct vtotal */
		if (err != 0)
			err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VCLOCK);
	} else if (force_interlace) {
		/* use only interlace modes */
		unsigned cap = video_mode_generate_driver_flags() & ~(VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN);
		/* try with a perfect mode */
		if (err != 0)
			err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_EXACT);
		/* try with a mode with different vclock but correct vtotal */
		if (err != 0)
			err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, cap, GENERATE_ADJUST_VCLOCK);
	} else {
		/* try with a perfect mode */
		if (err != 0)
			err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, video_mode_generate_driver_flags(), GENERATE_ADJUST_EXACT);
		/* try with a mode with different vclock but correct vtotal */
		if (err != 0)
			err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, video_mode_generate_driver_flags(), GENERATE_ADJUST_VCLOCK);
		/* try with a mode with different vtotal and different vclock */
		if (err != 0)
			err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, video_mode_generate_driver_flags(), GENERATE_ADJUST_VTOTAL | GENERATE_ADJUST_VCLOCK);
	}

	if (err != 0)
		return 0;

	if (!crtc_clock_check(&context->config.monitor,&crtc))
		return 0;

	strcpy(crtc.name,name);

	video_crtc_print(buffer,&crtc);
	os_log(("advance:generate: modeline \"%s\"\n",buffer));

	ret = video_crtc_container_insert(&context->config.crtc_bag,&crtc);

	if (context->config.stretch == STRETCH_FRACTIONAL_XY) {
		os_log(("advance:video: fractional coverted in mixed because generate/adjustx is active\n"));
		context->config.stretch = STRETCH_INTEGER_X_FRACTIONAL_Y;
	}

	return ret;
}

/**
 * Compute and insert in the main list a new modeline guessing the specified parameters.
 * This is mainly used for vector games.
 */
static void video_init_crtc_make_vector(struct advance_video_context* context, const char* name, unsigned size_x, unsigned size_y, double vclock) {
	char buffer[256];
	video_crtc crtc;
	int err = -1;

	/* try with a perfect mode */
	if (err != 0)
		err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, video_mode_generate_driver_flags(), GENERATE_ADJUST_EXACT);
	/* try with a mode with different vtotal but correct vclock */
	if (err != 0)
		err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, video_mode_generate_driver_flags(), GENERATE_ADJUST_VTOTAL);
	/* try with a mode with different vtotal and different vclock */
	if (err != 0)
		err = generate_find_interpolate_double(&crtc, size_x, size_y, vclock, &context->config.monitor, &context->config.interpolate, video_mode_generate_driver_flags(), GENERATE_ADJUST_VTOTAL | GENERATE_ADJUST_VCLOCK);

	if (err != 0)
		return;

	if (!crtc_clock_check(&context->config.monitor,&crtc))
		return;

	/* adjust the horizontal size */
	size_y = crtc_vsize_get(&crtc);
	size_x = size_y * pc_aspect_ratio_x / pc_aspect_ratio_y;
	size_x &= ~0x7;

	crtc_hsize_set(&crtc, size_x);

	strcpy(crtc.name,name);

	video_crtc_print(buffer,&crtc);
	os_log(("advance:generate: modeline \"%s\"\n",buffer));

	video_crtc_container_insert(&context->config.crtc_bag,&crtc);
}

static unsigned best_step(unsigned value, unsigned step) {
	if (value % step != 0)
		value = value + step - value % step;
	return value;
}

/**
 * Initialize the state.
 * \return 0 on success
 */
static int video_init_state(struct advance_video_context* context, struct osd_video_option* req)
{
	unsigned best_size_x;
	unsigned best_size_y;
	unsigned best_size_2x;
	unsigned best_size_2y;
	unsigned best_size_3x;
	unsigned best_size_3y;
	unsigned best_bits;
	double best_vclock;
	unsigned factor_x;
	unsigned factor_y;
	unsigned arcade_aspect_ratio_x;
	unsigned arcade_aspect_ratio_y;

	unsigned step_x;

	if (context->config.adjust != ADJUST_NONE
		&& (video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)==0  /* not for programmable driver */
	) {
		fprintf(stderr,"Your video board isn't supported, you can't use the option `display_adjust'\n");
		return -1;
	}

	os_log(("advance:video: blit_orientation %d\n", context->config.blit_orientation));

	context->state.pause_flag = 0;
	context->state.crtc_selected = 0;
	context->state.gamma_effect_factor = 1;
	context->state.menu_sub_active = 0;
	context->state.menu_sub_selected = 0;

	context->state.fastest_counter = 0; /* inizialize the fastest frame counter */
	context->state.fastest_flag = 0; /* not active until the first reset call */

	context->state.measure_counter = 0; /* inizialize the measure frame counter */
	context->state.measure_flag = 0; /* not active until the first reset call */
	context->state.measure_start = 0;
	context->state.measure_stop = 0;

	context->state.debugger_flag = 0;
	context->state.sync_throttle_flag = 1;

	context->state.game_bits_per_pixel = req->bits_per_pixel;
	context->state.game_bytes_per_pixel = (context->state.game_bits_per_pixel + 7) / 8;

	context->state.game_vector_flag = req->vector_flag;

	context->state.game_fps = req->fps * context->config.fps_speed_factor;
	context->state.game_area_size_x = req->area_size_x;
	context->state.game_area_size_y = req->area_size_y;
	context->state.game_used_pos_x = req->used_pos_x;
	context->state.game_used_pos_y = req->used_pos_y;
	context->state.game_used_size_x = req->used_size_x;
	context->state.game_used_size_y = req->used_size_y;
	arcade_aspect_ratio_x = req->aspect_x;
	arcade_aspect_ratio_y = req->aspect_y;

	/* set the correct blit orientation */
	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(unsigned, context->state.game_area_size_x, context->state.game_area_size_y );
		SWAP(unsigned, context->state.game_used_pos_x, context->state.game_used_pos_y );
		SWAP(unsigned, context->state.game_used_size_x, context->state.game_used_size_y );
		SWAP(unsigned, arcade_aspect_ratio_x, arcade_aspect_ratio_y );
	}

	context->state.game_rgb_flag = req->rgb_flag;
	if (context->state.game_rgb_flag) {
		context->state.game_rgb_def = req->rgb_def;
		context->state.game_colors = 0;
	} else {
		context->state.game_rgb_def = 0;
		context->state.game_colors = req->colors;
	}

	/* expand the arcade aspect ratio */
	if (arcade_aspect_ratio_y * pc_aspect_ratio_x < pc_aspect_ratio_y * arcade_aspect_ratio_x) {
		arcade_aspect_ratio_x *= 100;
		arcade_aspect_ratio_y *= 100 * context->config.aspect_expansion_factor;
		/* limit */
		if (arcade_aspect_ratio_y * pc_aspect_ratio_x > pc_aspect_ratio_y * arcade_aspect_ratio_x) {
			arcade_aspect_ratio_x = pc_aspect_ratio_x;
			arcade_aspect_ratio_y = pc_aspect_ratio_y;
		}
	} else {
		arcade_aspect_ratio_y *= 100;
		arcade_aspect_ratio_x *= 100 * context->config.aspect_expansion_factor;
		/* limit */
		if (arcade_aspect_ratio_y * pc_aspect_ratio_x < pc_aspect_ratio_y * arcade_aspect_ratio_x) {
			arcade_aspect_ratio_x = pc_aspect_ratio_x;
			arcade_aspect_ratio_y = pc_aspect_ratio_y;
		}
	}

	video_aspect_reduce(&arcade_aspect_ratio_x, &arcade_aspect_ratio_y);

	/* compute the game aspect ratio */
	context->state.game_aspect_x = context->state.game_used_size_x * arcade_aspect_ratio_y;
	context->state.game_aspect_y = context->state.game_used_size_y * arcade_aspect_ratio_x;
	video_aspect_reduce(&context->state.game_aspect_x, &context->state.game_aspect_y);

	factor_x = pc_aspect_ratio_x * context->state.game_aspect_x;
	factor_y = pc_aspect_ratio_y * context->state.game_aspect_y;
	video_aspect_reduce(&factor_x, &factor_y);

	os_log(("advance:video: best aspect factor %dx%d (expansion %g)\n", factor_x, factor_y, (double)context->config.aspect_expansion_factor));

	/* Some video drivers have problem with 8 bit modes and */
	/* not exactly a 16 pixel multiplier size */
	/* Currently nVidia in doublescan mode */
	step_x = 16; /* TODO the correct value is 8 */

	/* compute the best mode */
	if (pc_aspect_ratio_x * arcade_aspect_ratio_y > arcade_aspect_ratio_x * pc_aspect_ratio_y) {
		best_size_y = context->state.game_used_size_y;
		best_size_x = best_step((double)context->state.game_used_size_y * factor_x / factor_y, step_x);
		best_size_2y = 2*context->state.game_used_size_y;
		best_size_2x = best_step((double)2*context->state.game_used_size_y * factor_x / factor_y, step_x);
		best_size_3y = 3*context->state.game_used_size_y;
		best_size_3x = best_step((double)3*context->state.game_used_size_y * factor_x / factor_y, step_x);
	} else {
		best_size_x = best_step(context->state.game_used_size_x, step_x);
		best_size_y = context->state.game_used_size_x * factor_y / factor_x;
		best_size_2x = best_step(2*context->state.game_used_size_x, step_x);
		best_size_2y = 2*context->state.game_used_size_x * factor_y / factor_x;
		best_size_3x = best_step(3*context->state.game_used_size_x, step_x);
		best_size_3y = 3*context->state.game_used_size_x * factor_y / factor_x;
	}
	best_bits = context->state.game_bits_per_pixel;
	best_vclock = context->state.game_fps;

	os_log(("advance:video: best mode %dx%d, mode2x %dx%d, mode3x %dx%d, bits_per_pixel %d, vclock %g\n",best_size_x,best_size_y,best_size_2x,best_size_2y,best_size_3x,best_size_3y,best_bits,(double)best_vclock));

#if 0
	/* create some modelines */
	if ((context->config.adjust & ADJUST_ADJUST_X) != 0
		&& !context->state.game_vector_flag /* nonsense for vector games */
		&& (video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK) != 0 /* only for programmable driver */
	) {
		const video_crtc* best_crtc = 0;
		/* search the nearest video mode */
		best_crtc = video_init_crtc_bigger_find(context, best_size_y);
		if (!best_crtc)
			best_crtc = video_init_crtc_smaller_find(context, best_size_y);
		if (best_crtc)
			video_init_crtc_make_adjustx(context, "generate",best_size_x, best_size_y, best_crtc);
		best_crtc = video_init_crtc_bigger_find(context, best_size_2y);
		if (!best_crtc)
			best_crtc = video_init_crtc_smaller_find(context, best_size_2y);
		if (best_crtc)
			video_init_crtc_make_adjustx(context, "generate-double", best_size_2x, best_size_2y, best_crtc);
	}
#endif

	if ((context->config.adjust & ADJUST_GENERATE) != 0
		&& !context->state.game_vector_flag /* nonsense for vector games */
		&& (video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0 /* only for programmable driver */
		&& context->config.interpolate.mac > 0
	) {
		const video_crtc* crtc;
		crtc = video_init_crtc_make_raster(context, "generate",best_size_x, best_size_y, best_vclock, 0, 0);
		if (!crtc || !crtc_is_singlescan(crtc))
			video_init_crtc_make_raster(context, "generate-scanline",best_size_x, best_size_y, best_vclock, 1, 0);
		if (!crtc || !crtc_is_interlace(crtc))
			video_init_crtc_make_raster(context, "generate-interlace",best_size_x, best_size_y, best_vclock, 0, 1);
		crtc = video_init_crtc_make_raster(context, "generate-double",best_size_2x, best_size_2y, best_vclock, 0, 0);
		if (!crtc || !crtc_is_singlescan(crtc))
			video_init_crtc_make_raster(context, "generate-double-scanline",best_size_2x, best_size_2y, best_vclock, 1, 0);
		if (!crtc || !crtc_is_interlace(crtc))
			video_init_crtc_make_raster(context, "generate-double-interlace",best_size_2x, best_size_2y, best_vclock, 0, 1);
		video_init_crtc_make_raster(context, "generate-triple",best_size_3x, best_size_3y, best_vclock, 0, 0);
	}

	if (context->state.game_vector_flag) {
		context->config.stretch = STRETCH_NONE;
	}

	context->state.mode_best_size_x = best_size_x;
	context->state.mode_best_size_y = best_size_y;
	context->state.mode_best_size_2x = best_size_2x;
	context->state.mode_best_size_2y = best_size_2y;
	context->state.mode_best_vclock = best_vclock;

	os_log(("advance:video: game_area_size_x %d\n",context->state.game_area_size_x));
	os_log(("advance:video: game_area_size_y %d\n",context->state.game_area_size_y));
	os_log(("advance:video: game_used_pos_x %d\n",context->state.game_used_pos_x));
	os_log(("advance:video: game_used_pos_y %d\n",context->state.game_used_pos_y));
	os_log(("advance:video: game_used_size_x %d\n",context->state.game_used_size_x));
	os_log(("advance:video: game_used_size_y %d\n",context->state.game_used_size_y));
	os_log(("advance:video: game_aspect_x %d\n",context->state.game_aspect_x));
	os_log(("advance:video: game_aspect_y %d\n",context->state.game_aspect_y));

	return 0;
}

static void video_done_state(struct advance_video_context* context) {
}

/**
 * Initialize the color mode
 * \return 0 on success
 */
static int video_init_color(struct advance_video_context* context, struct osd_video_option* req)
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

	os_log(("advance:video: palette_total %d\n", context->state.palette_total));
	os_log(("advance:video: palette_dirty_total %d\n", context->state.palette_dirty_total));

	context->state.palette_dirty_map = (osd_mask_t*)malloc(context->state.palette_dirty_total * sizeof(osd_mask_t));
	context->state.palette_map = (osd_rgb_t*)malloc(context->state.palette_total * sizeof(osd_rgb_t));

	/* create the software palette */
	/* it will not be used if a hardware palette is present, but a runtime mode change may require it */
	context->state.palette_index_map = (unsigned*)malloc(context->state.palette_total * sizeof(unsigned));

	/* initialize the palette */
	for(i=0;i<context->state.palette_total;++i) {
		context->state.palette_map[i] = osd_rgb(0,0,0);
	}
	for(i=0;i<context->state.palette_total;++i) {
		context->state.palette_index_map[i] = 0;
	}

	/* make the palette completly dirty */
	context->state.palette_is_dirty = 1;
	for(i=0;i<context->state.palette_dirty_total;++i)
		context->state.palette_dirty_map[i] = osd_mask_full;

	/* set the rgb format for rgb games */
	if (context->state.game_rgb_flag && req->rgb_components) {
		req->rgb_components[0] = video_rgb_make_from_def(0xFF, 0x00, 0x00, context->state.game_rgb_def);
		req->rgb_components[1] = video_rgb_make_from_def(0x00, 0xFF, 0x00, context->state.game_rgb_def);
		req->rgb_components[2] = video_rgb_make_from_def(0x00, 0x00, 0xFF, context->state.game_rgb_def);
	}

	return 0;
}

/**
 * Deinitialize the color mode.
 */
static void video_done_color(struct advance_video_context* context) {
	free(context->state.palette_dirty_map);
	context->state.palette_dirty_map = 0;
	free(context->state.palette_map);
	context->state.palette_map = 0;
	free(context->state.palette_index_map);
	context->state.palette_index_map = 0;
}

/***************************************************************************/
/* Frame */

static __inline__ void video_frame_resolution(struct advance_video_context* context, unsigned input)
{
	int modify = 0;
	int show = 0;
	
	if (input == OSD_INPUT_MODE_NEXT) {
		show = 1;
		if (strcmp(context->config.resolution,"auto")==0) {
			if (context->state.crtc_mac > 0) {
				strcpy(context->config.resolution, crtc_name_get(context->state.crtc_map[0]));
				modify = 1;
			}
		} else {
			unsigned i;
			for(i=0;i<context->state.crtc_mac;++i)
				if (context->state.crtc_map[i] == context->state.crtc_selected)
					break;
			if (i<context->state.crtc_mac && i+1<context->state.crtc_mac) {
				strcpy(context->config.resolution, crtc_name_get(context->state.crtc_map[i+1]));
				modify = 1;
			}
		}
	} else if (input == OSD_INPUT_MODE_PRED) {
		show = 1;
		if (strcmp(context->config.resolution,"auto")!=0) {
			unsigned i;
			for(i=0;i<context->state.crtc_mac;++i)
				if (context->state.crtc_map[i] == context->state.crtc_selected)
					break;
			if (i==0) {
				strcpy(context->config.resolution, "auto");
				modify = 1;
			} else if (i<context->state.crtc_mac && i>0) {
				strcpy(context->config.resolution, crtc_name_get(context->state.crtc_map[i-1]));
				modify = 1;
			}
		}
	}

	if (modify) {
		os_log(("advance:video: select mode %s\n",context->config.resolution));

		/* update all the complete state, the configuration is choosen by the name */
		advance_video_change(context);
	}

	if (show) {
		mame_ui_message(context->config.resolution);
	}
}

static __inline__ void video_frame_pan(struct advance_video_context* context, unsigned input)
{
	int modify = 0;

	if (input == OSD_INPUT_PAN_RIGHT) {
		if (context->state.game_visible_pos_x + context->state.game_visible_size_x + context->state.game_visible_pos_x_increment <= context->state.game_used_size_x) {
			context->state.game_visible_pos_x += context->state.game_visible_pos_x_increment;
			modify = 1;
		}
	}

	if (input == OSD_INPUT_PAN_LEFT) {
		if (context->state.game_visible_pos_x >= context->state.game_visible_pos_x_increment) {
			context->state.game_visible_pos_x -= context->state.game_visible_pos_x_increment;
			modify = 1;
		}
	}

	if (input == OSD_INPUT_PAN_UP) {
		if (context->state.game_visible_pos_y + context->state.game_visible_size_y < context->state.game_used_pos_y + context->state.game_used_size_y) {
			context->state.game_visible_pos_y++;
			modify = 1;
		}
	}

	if (input == OSD_INPUT_PAN_DOWN) {
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

static __inline__ void video_frame_blit(struct advance_video_context* context, unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, void* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp)
{
	unsigned combine = context->state.combine | context->state.effect;
	if (context->state.game_rgb_flag) {
		video_stretch(dst_x, dst_y, dst_dx, dst_dy, src, src_dx, src_dy, src_dw, src_dp, context->state.game_rgb_def, combine);
	} else {
		if (!context->state.mode_rgb_flag) {
			video_stretch_palette_hw(dst_x, dst_y, dst_dx, dst_dy, src, src_dx, src_dy, src_dw, src_dp, combine);
		} else {
			switch (context->state.game_bytes_per_pixel) {
				case 1 :
					video_stretch_palette_8(dst_x, dst_y, dst_dx, dst_dy, src, src_dx, src_dy, src_dw, src_dp, context->state.palette_index_map, combine);
					break;
				case 2 :
					video_stretch_palette_16(dst_x, dst_y, dst_dx, dst_dy, src, src_dx, src_dy, src_dw, src_dp, context->state.palette_index_map, combine);
					break;
			}
		}
	}
}

static __inline__ void video_frame_pipeline(struct advance_video_context* context, const struct osd_bitmap* bitmap) {
	unsigned combine;

	/* check if the pipeline is already allocated */
	if (context->state.blit_pipeline_flag)
		return;

	assert( *video_mode_name(&context->state.mode) );

	/* screen position */
	context->state.blit_dst_x = (video_size_x() - context->state.mode_visible_size_x) / 2;
	context->state.blit_dst_y = (video_size_y() - context->state.mode_visible_size_y) / 2;

	if (video_is_unchained()) {
		unsigned pixel = ALIGN_UNCHAINED / video_bytes_per_pixel();
		context->state.blit_dst_x = (context->state.blit_dst_x + pixel-1) & ~(pixel-1);
	} else {
		unsigned pixel = ALIGN / video_bytes_per_pixel();
		context->state.blit_dst_x = (context->state.blit_dst_x + pixel-1) & ~(pixel-1);
	}

	/* source increment */
	context->state.blit_src_dw = bitmap->bytes_per_scanline;
	context->state.blit_src_dp = context->state.game_bytes_per_pixel;

	/* adjust for the blit orientation */
	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(int,context->state.blit_src_dw,context->state.blit_src_dp);
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

	/* compute the source x position aligment */
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

	combine = context->state.combine | context->state.effect;

	if (context->state.game_rgb_flag) {
		video_stretch_pipeline_init(&context->state.blit_pipeline, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.game_rgb_def, combine);
	} else {
		if (!context->state.mode_rgb_flag) {
			video_stretch_palette_hw_pipeline_init(&context->state.blit_pipeline, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, combine);
		} else {
			switch (context->state.game_bytes_per_pixel) {
				case 1 :
					video_stretch_palette_8_pipeline_init(&context->state.blit_pipeline, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp, context->state.palette_index_map, combine);
					break;
				case 2 :
					video_stretch_palette_16_pipeline_init(&context->state.blit_pipeline, context->state.mode_visible_size_x, context->state.mode_visible_size_y, context->state.game_visible_size_x, context->state.game_visible_size_y, context->state.blit_src_dw, context->state.blit_src_dp,  context->state.palette_index_map, combine);
					break;
			}
		}
	}

	context->state.blit_pipeline_flag = 1;
}

static __inline__ void video_frame_put(struct advance_video_context* context, const struct osd_bitmap* bitmap, unsigned x, unsigned y) {
	unsigned dst_x;
	unsigned dst_y;
	unsigned src_offset;

	/* screen position */
	dst_x = context->state.blit_dst_x + x;
	dst_y = context->state.blit_dst_y + y;

	/* compute the source pointer */
	src_offset = context->state.blit_src_offset + context->state.game_visible_pos_y * context->state.blit_src_dw + context->state.game_visible_pos_x * context->state.blit_src_dp;

	video_blit_pipeline(&context->state.blit_pipeline, dst_x, dst_y, bitmap->ptr + src_offset);
}

static __inline__ void video_frame_screen(struct advance_video_context* context, const struct osd_bitmap *bitmap, unsigned input) {

	update_start();

	video_frame_pipeline(context, bitmap);
	video_frame_put(context, bitmap, update_x_get(), update_y_get());

#if 0
	if (input == OSD_INPUT_TURBO) {
		static int snapshot_num = 0;
		char snapshot_name[256];
		sprintf(snapshot_name,"snap%d.bmp",snapshot_num);
		video_snapshot_save(snapshot_name,update_x_get(),update_y_get());
		++snapshot_num;
	}
#endif

	update_stop(0);
}

static __inline__ void video_frame_palette(struct advance_video_context* context) {
	if (context->state.palette_is_dirty) {
		unsigned i;

		context->state.palette_is_dirty = 0;

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

						osd_rgb_t c = context->state.palette_map[p];

						/* update the palette */
						if (!context->state.mode_rgb_flag) {
							/* hardware */
							/* note: trying to concatenate palette update */
							/* generate flickering!, one color at time is ok! */
							video_color adjusted_palette;
							adjusted_palette.red = osd_rgb_red(c);
							adjusted_palette.green = osd_rgb_green(c);
							adjusted_palette.blue = osd_rgb_blue(c);
							video_palette_set(&adjusted_palette, p, 1, 0);
						} else {
							/* software */
							video_rgb rgb;
							video_rgb_make(&rgb, osd_rgb_red(c), osd_rgb_green(c), osd_rgb_blue(c));
							context->state.palette_index_map[p] = rgb;
						}
					}

					t <<= 1;
				}
			}
		}
	}
}

static void event_compare(unsigned previous, unsigned current, int id) {
	if (previous != current) {
		if (current) {
			hardware_script_start(id);
		} else {
			hardware_script_stop(id);
		}
	}
}

void video_frame_event(struct advance_video_context* context, int leds_status, int turbo_status, unsigned input) {
	unsigned event_mask = 0;

	event_mask |= leds_status & 0x7;
	if ((input & OSD_INPUT_COIN1) != 0) event_mask |= 0x100;
	if ((input & OSD_INPUT_COIN2) != 0) event_mask |= 0x200;
	if ((input & OSD_INPUT_COIN3) != 0) event_mask |= 0x400;
	if ((input & OSD_INPUT_COIN4) != 0) event_mask |= 0x800;
	if ((input & OSD_INPUT_START1) != 0) event_mask |= 0x1000;
	if ((input & OSD_INPUT_START2) != 0) event_mask |= 0x2000;
	if ((input & OSD_INPUT_START3) != 0) event_mask |= 0x4000;
	if ((input & OSD_INPUT_START4) != 0) event_mask |= 0x8000;
	if (turbo_status) event_mask |= 0x10000;

	event_compare(context->state.event_mask_old & 0x1, event_mask & 0x1, HARDWARE_SCRIPT_LED1);
	event_compare(context->state.event_mask_old & 0x2, event_mask & 0x2, HARDWARE_SCRIPT_LED2);
	event_compare(context->state.event_mask_old & 0x4, event_mask & 0x4, HARDWARE_SCRIPT_LED3);

	event_compare(context->state.event_mask_old & 0x100, event_mask & 0x100, HARDWARE_SCRIPT_COIN1);
	event_compare(context->state.event_mask_old & 0x200, event_mask & 0x200, HARDWARE_SCRIPT_COIN2);
	event_compare(context->state.event_mask_old & 0x400, event_mask & 0x400, HARDWARE_SCRIPT_COIN3);
	event_compare(context->state.event_mask_old & 0x800, event_mask & 0x800, HARDWARE_SCRIPT_COIN4);
	event_compare(context->state.event_mask_old & 0x1000, event_mask & 0x1000, HARDWARE_SCRIPT_START1);
	event_compare(context->state.event_mask_old & 0x2000, event_mask & 0x2000, HARDWARE_SCRIPT_START2);
	event_compare(context->state.event_mask_old & 0x4000, event_mask & 0x4000, HARDWARE_SCRIPT_START3);
	event_compare(context->state.event_mask_old & 0x8000, event_mask & 0x8000, HARDWARE_SCRIPT_START4);
	event_compare(context->state.event_mask_old & 0x10000, event_mask & 0x10000, HARDWARE_SCRIPT_TURBO);

	/* save the current status */
	context->state.event_mask_old = event_mask;
}

static int video_skip_dec(struct advance_video_context* context) {
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

static int video_skip_inc(struct advance_video_context* context) {
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

static void video_skip_recompute(struct advance_video_context* context, struct advance_estimate_context* estimate_context) {
	double step = context->state.skip_step;
	double full;
	double skip;

	if (context->config.smp_flag) {
		full = estimate_context->estimate_mame_full;
		if (full < estimate_context->estimate_osd_full)
			full = estimate_context->estimate_osd_full;
		skip = estimate_context->estimate_mame_skip;
		if (skip < estimate_context->estimate_osd_skip)
			skip = estimate_context->estimate_osd_skip;
		full += estimate_context->estimate_common_full;
		skip += estimate_context->estimate_common_skip;
	} else {
		full = estimate_context->estimate_mame_full + estimate_context->estimate_osd_full;
		skip = estimate_context->estimate_mame_skip + estimate_context->estimate_osd_skip;
	}

	os_log_debug(("advance:skip: step %g [sec]\n", step));
	os_log_debug(("advance:skip: frame full %g [sec], frame skip %g [sec]\n",estimate_context->estimate_mame_full + estimate_context->estimate_osd_full, estimate_context->estimate_mame_skip + estimate_context->estimate_osd_skip));
	os_log_debug(("advance:skip: mame_full %g [sec], mame_skip %g [sec], osd_full %g [sec], osd_skip %g [sec]\n",estimate_context->estimate_mame_full,estimate_context->estimate_mame_skip,estimate_context->estimate_osd_full,estimate_context->estimate_osd_skip));
	os_log_debug(("advance:skip: common_full %g [sec], common_skip %g [sec]\n",estimate_context->estimate_common_full,estimate_context->estimate_common_skip));
	os_log_debug(("advance:skip: full %g [sec], skip %g [sec]\n", full, skip));

	if (full < step) {
		context->state.skip_level_full = SYNC_MAX;
		context->state.skip_level_skip = 0;
	} else if (skip > step) {
		context->state.skip_level_full = 1;
		context->state.skip_level_skip = SYNC_MAX - 1;
	} else {
		double v = (step - skip) / (full - step);
		if (v > 1) {
			context->state.skip_level_full = floor( v );
			context->state.skip_level_skip = 1;
			if (context->state.skip_level_full >= SYNC_MAX)
				context->state.skip_level_full = SYNC_MAX - 1;
		} else {
			context->state.skip_level_full = 1;
			context->state.skip_level_skip = ceil( 1 / v );
			if (context->state.skip_level_skip >= SYNC_MAX)
				context->state.skip_level_skip = SYNC_MAX - 1;
		}
	}

	os_log_debug(("advance:skip: cycle %d/%d\n",context->state.skip_level_full,context->state.skip_level_skip));
}

static double video_frame_wait(double current, double expected) {
	while (current < expected) {
		double diff = expected - current;

		os_usleep( diff * 1E6 );

		current = advance_timer();
	}

	return current;
}

static void video_frame_sync(struct advance_video_context* context) {
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

		os_log_debug(("advance:sync: throttle warming up\n"));
	} else {
		double previous = current;

		expected = context->state.sync_last + context->state.skip_step * (1 + context->state.sync_skip_counter);
		context->state.sync_skip_counter = 0;

		/* take only a part of the error, this increase the stability */
		context->state.sync_pivot *= 0.95;

		/* adjust the previous error */
		expected += context->state.sync_pivot;

		/* the vsync is used only if all the frames are displayed */
		if (context->state.vsync_flag && context->state.skip_level_full == SYNC_MAX) {
			if (current < expected) {

				/* wait until the retrace is near, otherwise if the */
				/* mode has a double freq the retrace may be the wrong one */

				double early = 0.90 / video_measured_vclock();
				current = video_frame_wait(current, expected - early);

				if (current < expected) {
					double after;
					video_wait_vsync();
					after = advance_timer();
					if (after - current > 1.1 / video_measured_vclock())
						os_log(("ERROR: sync wait too long. %g instead of %g\n", after - current, 1.0 / (double)video_measured_vclock()));
					current = after;
				} else {
					os_log(("ERROR: sync delay too big\n"));
				}
			} else {
				os_log(("ERROR: too late for a sync\n"));
			}
		} else {
			current = video_frame_wait(current, expected);
		}

		/* update the error state */
		context->state.sync_pivot = expected - current;

		os_log(("advance:sync: total %.5f, error%8.5f, last %.5f, wait %.5f\n", current - context->state.sync_last, context->state.sync_pivot, previous - context->state.sync_last, current - previous));

		context->state.sync_last = current;
	}
}

static void video_frame_sync_free(struct advance_video_context* context) {
	double current;

	current = advance_timer();

	if (context->state.sync_warming_up_flag) {
		context->state.sync_pivot = 0;
		context->state.sync_last = current;
		context->state.sync_skip_counter = 0;

		context->state.sync_warming_up_flag = 0;

		os_log_debug(("advance:sync: free warming up\n"));
	} else {
		context->state.sync_last = current;

		os_log_debug(("advance:sync: free\n"));
	}
}

static void video_sync_update(struct advance_video_context* context, struct advance_sound_context* sound_context, int skip_flag) {
	if (!skip_flag) {
		if (!context->state.fastest_flag
			&& !context->state.measure_flag
			&& context->state.sync_throttle_flag)
			video_frame_sync(context);
		else
			video_frame_sync_free(context);
		context->state.latency_diff = advance_sound_latency_diff(sound_context);
	} else {
		++context->state.sync_skip_counter;
	}
}

static void video_frame_skip(struct advance_video_context* context, struct advance_estimate_context* estimate_context) {

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

		os_log_debug(("advance:skip: throttle warming up\n"));
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
				video_skip_recompute(context,estimate_context);
			}
		}

		os_log_debug(("advance:skip: skip %d, frame %d/%d/%d\n",context->state.skip_flag,context->state.skip_level_counter,context->state.skip_level_full,context->state.skip_level_skip));
	}
}

static void video_frame_skip_none(struct advance_video_context* context, struct advance_estimate_context* estimate_context) {
	context->state.skip_step = 1.0 / context->state.game_fps;
	context->state.skip_flag = 0;
	context->state.skip_level_counter = 0;

	/* force a recoputation when needed */
	context->state.skip_warming_up_flag = 1;
}

static void video_skip_update(struct advance_video_context* context, struct advance_estimate_context* estimate_context, struct advance_record_context* record_context) {
	if (!context->state.sync_throttle_flag
		|| advance_record_video_is_active(record_context)
		|| advance_record_snapshot_is_active(record_context)
	) {
		video_frame_skip_none(context,estimate_context);
	} else {
		video_frame_skip(context,estimate_context);
	}
}

static void video_cmd_update(struct advance_video_context* context, struct advance_estimate_context* estimate_context, int leds_status, unsigned input, int skip_flag) {

	/* events */
	video_frame_event(context, leds_status, context->state.turbo_flag, input);

	/* scripts */
	hardware_script_idle( SCRIPT_TIME_UNIT / context->state.game_fps );
	hardware_simulate_input_idle( SIMULATE_EVENT, SCRIPT_TIME_UNIT / context->state.game_fps );
	hardware_simulate_input_idle( SIMULATE_KEY, SCRIPT_TIME_UNIT / context->state.game_fps );

	if (context->state.measure_flag) {
		if (context->state.measure_counter > 0) {
			--context->state.measure_counter;
			if (context->state.measure_counter == 0) {
				context->state.measure_stop = os_clock();

				/* force the exit at the next frame */
				CONTEXT.input.state.input_forced_exit_flag = 1;
			}
		}

		/* don't change anything */
		return;
	}

	if (input == OSD_INPUT_THROTTLE) {
		context->state.sync_throttle_flag = !context->state.sync_throttle_flag;

		video_update_skip(context);
		video_update_sync(context);

		mame_ui_show_info_temp();
	}

	if (input == OSD_INPUT_FRAMESKIP_INC) {
		if (context->config.frameskip_auto_flag) {
			context->config.frameskip_auto_flag = 0;
			video_update_skip(context);
		} else {
			if (video_skip_inc(context))
				context->config.frameskip_auto_flag = 1;
		}

		mame_ui_show_info_temp();
	}

	if (input == OSD_INPUT_FRAMESKIP_DEC) {
		if (context->config.frameskip_auto_flag) {
			context->config.frameskip_auto_flag = 0;
			video_update_skip(context);
		} else {
			if (video_skip_dec(context))
				context->config.frameskip_auto_flag = 1;
		}

		mame_ui_show_info_temp();
	}

	if (context->state.turbo_flag) {
		if (input != OSD_INPUT_TURBO) {
			context->state.turbo_flag = 0;
			video_update_skip(context);
			video_update_sync(context);
		}
	} else {
		if (input == OSD_INPUT_TURBO) {
			context->state.turbo_flag = 1;
			video_update_skip(context);
			video_update_sync(context);
		}
	}

	if (context->state.fastest_flag) {
		if (context->state.fastest_counter > 0)
			--context->state.fastest_counter;
		if (context->state.fastest_counter == 0) {
			context->state.fastest_flag = 0;

			video_update_skip(context);
			video_update_sync(context);

			hardware_script_start(HARDWARE_SCRIPT_PLAY);
		}
	}

	if (input == OSD_INPUT_TOGGLE_DEBUG) {
		context->state.debugger_flag = !context->state.debugger_flag;
		video_invalidate_screen();
	}
}

static void video_frame_game(struct advance_video_context* context, struct advance_record_context* record_context, const struct osd_bitmap *bitmap, unsigned input, int skip_flag) {

	/* bitmap */
	if (!skip_flag) {
		video_frame_palette(context);
		video_frame_screen(context, bitmap, input);

		if (advance_record_video_is_active(record_context)
			&& !context->state.pause_flag) {

			unsigned pos_x = context->state.game_used_pos_x;
			unsigned pos_y = context->state.game_used_pos_y;
			unsigned size_x = context->state.game_used_size_x;
			unsigned size_y = context->state.game_used_size_y;
			int dp = context->state.game_bytes_per_pixel;
			int dw = bitmap->bytes_per_scanline;

			/* restore the original orientation */
			if ((context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
				SWAP(unsigned, pos_x, pos_y);
				SWAP(unsigned, size_x, size_y);
			}

			int offset = pos_x * dp + pos_y * dw;

			if (context->state.game_rgb_flag) {
				advance_record_video_update(record_context, bitmap->ptr + offset, size_x, size_y, dp, dw, context->state.game_rgb_def, 0, 0, context->config.game_orientation);
			} else {
				advance_record_video_update(record_context, bitmap->ptr + offset, size_x, size_y, dp, dw, 0, context->state.palette_map, context->state.palette_total, context->config.game_orientation);
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

			/* restore the original orientation */
			if ((context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
				SWAP(unsigned, pos_x, pos_y);
				SWAP(unsigned, size_x, size_y);
			}

			int offset = pos_x * dp + pos_y * dw;

			if (context->state.game_rgb_flag) {
				advance_record_snapshot_update(record_context, bitmap->ptr + offset, size_x, size_y, dp, dw, context->state.game_rgb_def, 0, 0, context->config.game_orientation);
			} else {
				advance_record_snapshot_update(record_context, bitmap->ptr + offset, size_x, size_y, dp, dw, 0, context->state.palette_map, context->state.palette_total, context->config.game_orientation);
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

	if (!bitmap || !palette) {
		os_log(("ERROR: null debugger bitmap\n"));
		return;
	}

	/* max size */
	size_x = video_size_x();
	size_y = video_size_y();
	if (size_x > bitmap->size_x)
		size_x = bitmap->size_x;
	if (size_y > bitmap->size_y)
		size_y = bitmap->size_y;

	if (context->state.mode_rgb_flag) {
		unsigned* palette_raw;
		unsigned i;

		palette_raw = (unsigned*)malloc(palette_size * sizeof(unsigned));

		for(i=0;i<palette_size;++i) {
			osd_rgb_t c = palette[i];
			video_rgb rgb;
			video_rgb_make(&rgb, osd_rgb_red(c), osd_rgb_green(c), osd_rgb_blue(c));
			palette_raw[i] = rgb;
		}

		video_stretch_palette_8(0, 0, size_x, size_y, bitmap->ptr, bitmap->size_x, bitmap->size_y, bitmap->bytes_per_scanline, 1, palette_raw, VIDEO_COMBINE_Y_MAX);

		free(palette_raw);
	} else {
		/* TODO set the hardware palette for the debugger */
		video_stretch_palette_hw(0, 0, size_x, size_y, bitmap->ptr, bitmap->size_x, bitmap->size_y, bitmap->bytes_per_scanline, 1, VIDEO_COMBINE_Y_MAX);
	}
}

static void advance_video_update(struct advance_video_context* context, struct advance_record_context* record_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned input, int skip_flag) {
	if (context->state.debugger_flag) {
		video_frame_debugger(context, debug, debug_palette, debug_palette_size);
	} else {
		video_frame_game(context, record_context, game, input, skip_flag);
	}
}

static void video_frame_update_now(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, struct advance_record_context* record_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count, int skip_flag) {

	/* the frame syncronization is out of the time estimation */
	video_sync_update(context, sound_context, skip_flag);

	/* estimate the time */
	advance_estimate_osd_begin(estimate_context);

	/* all the update */
	advance_video_update(context, record_context, game, debug, debug_palette, debug_palette_size, input, skip_flag);
	advance_sound_update(sound_context, record_context, context, sample_buffer, sample_count);

	/* estimate the time */
	advance_estimate_osd_end(estimate_context, skip_flag);
}

/***************************************************************************/
/* Thread */

#ifdef USE_SMP

static struct osd_bitmap* bitmap_duplicate(struct osd_bitmap* old, const struct osd_bitmap* current) {
	if (current) {
		if (!old)
			old = malloc(sizeof(struct osd_bitmap));

		old->ptr = current->ptr;
		old->size_x = current->size_x;
		old->size_y = current->size_y;
		old->bytes_per_scanline = current->bytes_per_scanline;
	} else {
		if (old) {
			free(old);
			old = 0;
		}
	}

	return old;
}

static void bitmap_free(struct osd_bitmap* bitmap) {
	if (bitmap)
		free(bitmap);
}

#endif

/**
 * Precomputation of the frame before updating.
 * Mainly used to duplicate the data for the video thread.
 */
static void video_frame_prepare(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count, int skip_flag) {
#ifdef USE_SMP
	/* don't use the thread if the debugger is active */
	if (context->config.smp_flag && !context->state.debugger_flag) {

		/* duplicate the data */
		pthread_mutex_lock(&context->state.thread_video_mutex);

		advance_estimate_common_begin(estimate_context);

		if (!skip_flag) {
			context->state.thread_state_game = bitmap_duplicate(context->state.thread_state_game, game);
			mame_ui_swap();
		}

		context->state.thread_state_led = led;
		context->state.thread_state_input = input;
		context->state.thread_state_skip_flag = skip_flag;

		if (sample_count > context->state.thread_state_sample_max) {
			os_log(("advance:thread: realloc sample buffer %d samples -> %d samples, %d bytes\n", context->state.thread_state_sample_max, sample_count, sound_context->state.bytes_per_sample * sample_count));
			context->state.thread_state_sample_max = sample_count;
			context->state.thread_state_sample_buffer = realloc(context->state.thread_state_sample_buffer, sound_context->state.bytes_per_sample * context->state.thread_state_sample_max);
			assert(context->state.thread_state_sample_buffer);
		}

		memcpy(context->state.thread_state_sample_buffer, sample_buffer, sample_count * sound_context->state.bytes_per_sample );
		context->state.thread_state_sample_count = sample_count;

		advance_estimate_common_end(estimate_context, skip_flag);

		pthread_mutex_unlock(&context->state.thread_video_mutex);
	}
#endif
}

static void video_frame_update(struct advance_video_context* context, struct advance_sound_context* sound_context, struct advance_estimate_context* estimate_context,struct advance_record_context* record_context, const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count, int skip_flag) {
#ifdef USE_SMP
	if (context->config.smp_flag && !context->state.debugger_flag) {
		pthread_mutex_lock(&context->state.thread_video_mutex);

		os_log_debug(("advance:thread: signal\n"));

		pthread_cond_signal(&context->state.thread_video_cond);

		pthread_mutex_unlock(&context->state.thread_video_mutex);
	} else {
		video_frame_update_now(context, sound_context, estimate_context, record_context, game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, skip_flag);
	}
#else
	video_frame_update_now(context, sound_context, estimate_context, record_context, game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, skip_flag);
#endif
}

#ifdef USE_SMP
/**
 * Main video thread function.
 */
static void* video_thread(void* void_context) {
	struct advance_video_context* context = &CONTEXT.video;
	struct advance_sound_context* sound_context = &CONTEXT.sound;
	struct advance_estimate_context* estimate_context = &CONTEXT.estimate;
	struct advance_record_context* record_context = &CONTEXT.record;

	os_log(("advance:thread: start\n"));

	pthread_mutex_lock(&context->state.thread_video_mutex);

	while (1) {
		os_log_debug(("advance:thread: wait\n"));

		/* wait for the next signal */
		pthread_cond_wait(&context->state.thread_video_cond, &context->state.thread_video_mutex);
		os_log_debug(("advance:thread: wakeup\n"));

		/* check for exit */
		if (context->state.thread_exit_flag) {
			os_log(("advance:thread: stop\n"));
			pthread_mutex_unlock(&context->state.thread_video_mutex);
			pthread_exit(0);
		}

		os_log_debug(("advance:thread: draw\n"));

		/* update the frame */
		video_frame_update_now(
			context,
			sound_context,
			estimate_context,
			record_context,
			context->state.thread_state_game,
			0,
			0,
			0,
			context->state.thread_state_led,
			context->state.thread_state_input,
			context->state.thread_state_sample_buffer,
			context->state.thread_state_sample_count,
			context->state.thread_state_skip_flag
		);
	}
}
#endif

/**
 * Initialize and start the video thread.
 */
static int video_init_thread(struct advance_video_context* context) {
#ifdef USE_SMP
	context->state.thread_exit_flag = 0;
	context->state.thread_state_game = 0;
	context->state.thread_state_led = 0;
	context->state.thread_state_input = 0;
	context->state.thread_state_sample_count = 0;
	context->state.thread_state_sample_max = 0;
	context->state.thread_state_sample_buffer = 0;
	context->state.thread_state_skip_flag = 0;
	if (pthread_mutex_init(&context->state.thread_video_mutex,NULL) != 0)
		return -1;
	if (pthread_cond_init(&context->state.thread_video_cond,NULL) != 0)
		return -1;
	if (pthread_create(&context->state.thread_id, NULL, video_thread, NULL) != 0)
		return -1;
#endif
	return 0;
}

/**
 * Terminate and deallocate the video thread.
 */
static void video_done_thread(struct advance_video_context* context) {
#ifdef USE_SMP
	pthread_mutex_lock(&context->state.thread_video_mutex);
	context->state.thread_exit_flag = 1;
	pthread_cond_signal(&context->state.thread_video_cond);
	pthread_mutex_unlock(&context->state.thread_video_mutex);

	pthread_join(context->state.thread_id, NULL);

	bitmap_free(context->state.thread_state_game);
	free(context->state.thread_state_sample_buffer);
	pthread_cond_destroy(&context->state.thread_video_cond);
	pthread_mutex_destroy(&context->state.thread_video_mutex);
#endif
}

/***************************************************************************/
/* OSD */

/**
 * Update the state after a configuration change from the user interface.
 */
int advance_video_change(struct advance_video_context* context) {
	video_mode mode;

#ifdef USE_SMP
	pthread_mutex_lock(&context->state.thread_video_mutex);
#endif

	if (video_update_depthindex(context) != 0) {
		goto err;
	}

	if (video_update_crtc(context) != 0) {
		goto err;
	}

	if (video_make_crtc(context, &context->state.crtc_effective, context->state.crtc_selected) != 0) {
		goto err;
	}

	video_update_visible(context, &context->state.crtc_effective);
	video_update_effect(context);

	if (video_make_mode(context, &mode, &context->state.crtc_effective) != 0) {
		goto err;
	}

	video_invalidate_color(context);

	video_update_pan(context);

	if (video_update_mode(context,&mode) != 0) {
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

int osd2_video_init(struct osd_video_option* req)
{
	struct advance_video_context* context = &CONTEXT.video;
	struct advance_input_context* input_context = &CONTEXT.input;

	video_mode mode;

	os_log(("osd: osd_video_init\n"));

	os_log(("osd: area_size_x %d, area_size_y %d\n", req->area_size_x, req->area_size_y));
	os_log(("osd: used_size_x %d, used_size_y %d, used_pos_x %d, used_pos_y %d\n", req->used_size_x, req->used_size_y, req->used_pos_x, req->used_pos_y));
	os_log(("osd: aspect_x %d, aspect_y %d\n", req->aspect_x, req->aspect_y));
	os_log(("osd: bits_per_pixel %d, rgb_flag %d, colors %d\n", req->bits_per_pixel, req->rgb_flag, req->colors));
	os_log(("osd: vector_flag %d\n", req->vector_flag));
	os_log(("osd: fps %g\n", req->fps));

	if (video_init_state(context,req) != 0) {
		goto err;
	}

	if (video_update_depthindex(context)!=0) {
		goto err;
	}

	if (video_update_crtc(context) != 0) {
		goto err;
	}

	if (video_make_crtc(context, &context->state.crtc_effective, context->state.crtc_selected) != 0) {
		goto err;
	}

	video_update_visible(context, &context->state.crtc_effective);
	video_update_effect(context);

	if (video_make_mode(context, &mode, &context->state.crtc_effective) != 0) {
		goto err;
	}

	video_init_color(context, req);

	video_update_pan(context);

	if (video_update_mode(context,&mode) != 0) {
		goto err;
	}

	video_update_skip(context);
	video_update_sync(context);

	hardware_script_start(HARDWARE_SCRIPT_VIDEO);

	if (os_key_init(input_context->config.keyboard_id, input_context->config.disable_special_flag) != 0) {
		goto err_mode;
	}

	if (video_init_thread(context) != 0) {
		goto err_os;
	}

	return 0;

err_os:
	os_key_done();
err_mode:
	video_done_mode(context, 0);
	video_mode_reset();
err:
	return -1;
}

void osd2_video_done(void)
{
	struct advance_video_context* context = &CONTEXT.video;

	os_log(("osd: osd_video_done\n"));

	video_done_thread(context);

	os_key_done();

	if (context->config.restore_flag || context->state.measure_flag) {
		video_done_mode(context,1);
		video_mode_reset();
	} else {
		video_done_mode(context,0);
		if (video_mode_is_active())
			video_mode_done(0);
	}

	video_done_color(context);
	video_done_state(context);

	hardware_script_terminate(HARDWARE_SCRIPT_PLAY);
	hardware_script_terminate(HARDWARE_SCRIPT_EMULATION);
	hardware_script_terminate(HARDWARE_SCRIPT_VIDEO);

        /* print the speed measure */
	if (context->state.measure_flag
		&& context->state.measure_stop > context->state.measure_start) {
		printf("%g\n", (double)(context->state.measure_stop - context->state.measure_start) / OS_CLOCKS_PER_SEC);
	}
}

void osd2_area(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
	struct advance_video_context* context = &CONTEXT.video;

	os_log(("osd: osd2_area(%d,%d,%d,%d)\n",x1,y1,x2,y2));

	context->state.game_used_pos_x = x1;
	context->state.game_used_pos_y = y1;
	context->state.game_used_size_x = x2 - x1 + 1;
	context->state.game_used_size_y = y2 - y1 + 1;

	/* set the correct blit orientation */
	if (context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(unsigned, context->state.game_used_pos_x, context->state.game_used_pos_y );
		SWAP(unsigned, context->state.game_used_size_x, context->state.game_used_size_y );
	}

	video_update_pan(context);
}

void osd2_palette(const osd_mask_t* mask, const osd_rgb_t* palette, unsigned size)
{
	struct advance_video_context* context = &CONTEXT.video;
	unsigned dirty_size;
	unsigned i;

	os_log_debug(("osd: osd2_palette(size:%d)\n", size));

	if (context->state.game_rgb_flag) {
		os_log_debug(("WARNING: no palette because the game is in RGB mode\n"));
		return;
	}

	dirty_size = (size + osd_mask_size - 1) / osd_mask_size;

	if (size > context->state.palette_total || dirty_size > context->state.palette_dirty_total) {
		os_log(("ERROR: invalid palette access\n"));
		return;
	}

	context->state.palette_is_dirty = 1;
	for(i=0;i<size;++i)
		context->state.palette_map[i] = palette[i];
	for(i=0;i<dirty_size;++i)
		context->state.palette_dirty_map[i] |= mask[i];
}

void osd_pause(int paused)
{
	struct advance_video_context* context = &CONTEXT.video;

	os_log(("osd: osd_pause(paused:%d)\n",paused));

	context->state.pause_flag = paused != 0;
}

void osd_reset(void)
{
	struct advance_video_context* context = &CONTEXT.video;

	os_log(("osd: osd_reset()\n"));

	hardware_script_terminate(HARDWARE_SCRIPT_PLAY);
	hardware_script_terminate(HARDWARE_SCRIPT_EMULATION);

	/* inizialize the fastest state */
	context->state.fastest_counter = context->config.fastest_time * context->state.game_fps;
	context->state.fastest_flag = context->state.fastest_counter != 0;

	/* inizialize the measure state */
	context->state.measure_counter = context->config.measure_time * context->state.game_fps;
	context->state.measure_flag = context->state.measure_counter != 0;
	context->state.measure_start = os_clock();

	video_update_skip(context);
	video_update_sync(context);

	hardware_script_start(HARDWARE_SCRIPT_EMULATION);

	// if not in "fastest" mode, the playing is already started
	if (!context->state.fastest_flag)
		hardware_script_start(HARDWARE_SCRIPT_PLAY);
}

void osd2_debugger_focus(int debugger_has_focus)
{
	struct advance_video_context* context = &CONTEXT.video;

	os_log(("osd: osd_debugger_focus(debugger_has_focus:%d)\n", debugger_has_focus));

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

int osd2_frame(const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count)
{
	/* save the values of the previous skip frame */
	int skip_flag = CONTEXT.video.state.skip_flag;

	/* current error of video/sound syncronization */
	int latency_diff;

	if (advance_record_sound_is_active(&CONTEXT.record)) {
		/* if recording is active does't skip any samples */
		latency_diff = 0;
	} else {
		/* adjust the next sample count to correct the syncronization error */
		/* the / 16 division is a low pass filter to improve the stability */
		latency_diff = CONTEXT.video.state.latency_diff / 16;
	}

	/* update the global info */
	video_cmd_update(&CONTEXT.video, &CONTEXT.estimate, led, input, skip_flag);
	video_skip_update(&CONTEXT.video, &CONTEXT.estimate, &CONTEXT.record);
	advance_input_update(&CONTEXT.input, CONTEXT.video.state.pause_flag);

	/* estimate the time */
	advance_estimate_frame(&CONTEXT.estimate);
	advance_estimate_mame_end(&CONTEXT.estimate, skip_flag);

	/* prepare the frame */
	video_frame_prepare(&CONTEXT.video, &CONTEXT.sound, &CONTEXT.estimate, game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, skip_flag);

	/* update the local info */
	video_frame_update(&CONTEXT.video, &CONTEXT.sound, &CONTEXT.estimate, &CONTEXT.record,game, debug, debug_palette, debug_palette_size, led, input, sample_buffer, sample_count, skip_flag);

	/* estimate the time */
	advance_estimate_mame_begin(&CONTEXT.estimate);

	return latency_diff;
}

void osd2_info(char* buffer, unsigned size) {
	unsigned skip;
	unsigned rate;

	struct advance_video_context* context = &CONTEXT.video;
	struct advance_estimate_context* estimate_context = &CONTEXT.estimate;

	rate = floor( 100.0 / (estimate_context->estimate_frame * context->state.game_fps / context->config.fps_speed_factor) + 0.5 );
	skip = 100 * context->state.skip_level_full / (context->state.skip_level_full + context->state.skip_level_skip);

	if (context->state.fastest_flag) {
		sprintf(buffer,"%7s %3d%% - %3d%%","startup",skip,rate);
	} else if (!context->state.sync_throttle_flag) {
		sprintf(buffer,"%7s 100%% - %3d%%","free",rate);
	} else if (context->state.turbo_flag) {
		sprintf(buffer,"%7s %3d%% - %3d%%","turbo",skip,rate);
	} else if (context->config.frameskip_auto_flag) {
		sprintf(buffer,"%7s %3d%% - %3d%%","auto",skip,rate);
	} else {
		sprintf(buffer,"%7s %3d%% - %3d%%","fix",skip,rate);
	}
}

/***************************************************************************/
/* Init/Done/Load */

/* Adjust the orientation with the requested rol/ror/flipx/flipy operations */
static unsigned video_orientation_compute(unsigned orientation, int rol, int ror, int flipx, int flipy) {
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

/** Compute the inverse orientation */
static unsigned video_orientation_inverse(unsigned orientation) {
	if ((orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
		int flipx = (orientation & OSD_ORIENTATION_FLIP_X) != 0;
		int flipy = (orientation & OSD_ORIENTATION_FLIP_Y) != 0;

		orientation = OSD_ORIENTATION_SWAP_XY;

		if (flipx)
			orientation |= OSD_ORIENTATION_FLIP_Y;
		if (flipy)
			orientation |= OSD_ORIENTATION_FLIP_X;
	}

	return orientation;

}

/***************************************************************************/
/* Config */

static struct conf_enum_int OPTION_RESIZE[] = {
{ "none", STRETCH_NONE },
{ "integer", STRETCH_INTEGER_XY },
{ "fractional", STRETCH_FRACTIONAL_XY },
{ "mixed", STRETCH_INTEGER_X_FRACTIONAL_Y }
};

static struct conf_enum_int OPTION_RESIZEEFFECT[] = {
{ "auto", COMBINE_AUTO },
{ "none", COMBINE_NONE },
{ "max", COMBINE_MAX },
{ "mean", COMBINE_MEAN },
{ "filter", COMBINE_FILTER },
{ "filterx", COMBINE_FILTERX },
{ "filtery", COMBINE_FILTERY },
{ "scale2x", COMBINE_SCALE2X }
};

static struct conf_enum_int OPTION_ADJUST[] = {
{ "none", ADJUST_NONE },
{ "x", ADJUST_ADJUST_X },
{ "clock", ADJUST_ADJUST_CLOCK },
{ "xclock", ADJUST_ADJUST_X | ADJUST_ADJUST_CLOCK },
{ "generate", ADJUST_GENERATE }
};

static struct conf_enum_int OPTION_ROTATE[] = {
{ "auto", ROTATE_AUTO },
{ "none", ROTATE_NONE },
{ "core", ROTATE_CORE },
{ "blit", ROTATE_BLIT }
};

static struct conf_enum_int OPTION_RGBEFFECT[] = {
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

static struct conf_enum_int OPTION_DEPTH[] = {
{ "auto", 0 },
{ "8", 8 },
{ "15", 15 },
{ "16", 16 },
{ "32", 32 }
};

int advance_video_init(struct advance_video_context* context, struct conf_context* cfg_context) {
	/* save the configuration */
	context->state.cfg_context = cfg_context;

	conf_bool_register_default(cfg_context, "display_scanlines", 1);
	conf_bool_register_default(cfg_context, "display_vsync", 0);
	conf_bool_register_default(cfg_context, "display_waitvsync", 0);
	conf_bool_register_default(cfg_context, "display_buffer", 0);
	conf_int_register_enum_default(cfg_context, "display_resize", conf_enum(OPTION_RESIZE), STRETCH_INTEGER_X_FRACTIONAL_Y);
	conf_bool_register_default(cfg_context, "display_magnify", 0);
	conf_bool_register_default(cfg_context, "display_rgb", 0);
	conf_int_register_enum_default(cfg_context, "display_adjust", conf_enum(OPTION_ADJUST), ADJUST_NONE);
	conf_string_register_default(cfg_context, "display_skiplines", "auto");
	conf_string_register_default(cfg_context, "display_skipcolumns", "auto");
	conf_string_register_default(cfg_context, "display_frameskip", "auto");
	conf_bool_register_default(cfg_context, "display_ror", 0);
	conf_bool_register_default(cfg_context, "display_rol", 0);
	conf_bool_register_default(cfg_context, "display_flipx", 0);
	conf_bool_register_default(cfg_context, "display_flipy", 0);
	conf_int_register_enum_default(cfg_context, "display_rotate", conf_enum(OPTION_ROTATE), ROTATE_AUTO);
	conf_int_register_enum_default(cfg_context, "display_resizeeffect", conf_enum(OPTION_RESIZEEFFECT), COMBINE_AUTO);
	conf_int_register_enum_default(cfg_context, "display_rgbeffect", conf_enum(OPTION_RGBEFFECT), EFFECT_NONE);
	conf_float_register_limit_default(cfg_context, "misc_speed", 0.1, 10.0, 1.0);
	conf_float_register_limit_default(cfg_context, "misc_turbospeed", 0.1, 30.0, 3.0);
	conf_int_register_limit_default(cfg_context, "misc_startuptime", 0, 180, 6);
	conf_int_register_limit_default(cfg_context, "misc_timetorun", 0, 3600, 0);
	conf_string_register_default(cfg_context, "display_mode", "auto");
	conf_int_register_enum_default(cfg_context, "display_depth", conf_enum(OPTION_DEPTH), 0);
	conf_bool_register_default(cfg_context, "display_restore", 1);
	conf_float_register_limit_default(cfg_context, "display_expand", 1.0, 10.0, 1.0);
#ifdef USE_SMP
	conf_bool_register_default(cfg_context, "misc_smp", 0);
#endif

	monitor_register(cfg_context);
	video_crtc_container_register(cfg_context);
	generate_interpolate_register(cfg_context);

	video_reg(cfg_context);
	video_reg_driver_all(cfg_context);

	/* load graphics modes */
	video_crtc_container_init(&context->config.crtc_bag);

	return 0;
}

static const char* GAME_BLIT_COMBINE_MAX[] = {
#include "blitmax.h"
0
};

static int video_config_mode(struct advance_video_context* context, struct mame_option* option) {
	video_crtc_container_iterator i;
	video_crtc* best_crtc = 0;
	int best_size;

	/* insert some default modeline if no generate option is present and the modeline set is empty */
	if ((context->config.adjust & ADJUST_GENERATE) == 0
		&& video_crtc_container_is_empty(&context->config.crtc_bag)) {
		video_crtc_container_insert_default_modeline_svga(&context->config.crtc_bag);
		video_crtc_container_insert_default_modeline_vga(&context->config.crtc_bag);
	}

	/* set the debugger size */
	option->debug_width = 640;
	option->debug_height = 480;
	for(video_crtc_container_iterator_begin(&i,&context->config.crtc_bag);!video_crtc_container_iterator_is_end(&i);video_crtc_container_iterator_next(&i)) {
		video_crtc* crtc = video_crtc_container_iterator_get(&i);
		/* if a specific mode is chosen, size the debugger like it */
		if (is_crtc_acceptable_preventive(context,crtc)
			&& video_resolution_cmp(context->config.resolution, crtc_name_get(crtc)) == 0) {
			option->debug_width = crtc_hsize_get(crtc);
			option->debug_height = crtc_vsize_get(crtc);
		}
	}
	os_log(("advance:video: suggested debugger size %dx%d\n", option->debug_width, option->debug_height));

	/* set the vector game size */
	if (mame_is_game_vector(option->game)) {
		unsigned mode_size_x;
		unsigned mode_size_y;
		unsigned game_size_x;
		unsigned game_size_y;

		mode_size_x = 640;
		mode_size_y = 480;

		if ((context->config.adjust & ADJUST_GENERATE) != 0
			&& (video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0
			&& context->config.interpolate.mac > 0
		) {
			/* insert the default mode for vector games */
			video_init_crtc_make_vector(context, "generate", mode_size_x, mode_size_y, mame_game_fps(option->game));
		}

		/* select the size of the mode near at the 640x480 size */
		best_crtc = 0;
		best_size = 0;
		for(video_crtc_container_iterator_begin(&i,&context->config.crtc_bag);!video_crtc_container_iterator_is_end(&i);video_crtc_container_iterator_next(&i)) {
			video_crtc* crtc = video_crtc_container_iterator_get(&i);
			if (is_crtc_acceptable_preventive(context,crtc)
				&& (strcmp(context->config.resolution,"auto")==0
					|| video_resolution_cmp(context->config.resolution, crtc_name_get(crtc)) == 0
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
			os_log(("advance:video: no specific mode for vector games\n"));
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

		os_log(("advance:video: suggested vector size %dx%d\n", option->vector_width, option->vector_height));
	} else {
		option->vector_width = 0;
		option->vector_height = 0;
	}

	return 0;
}

int advance_video_config_load(struct advance_video_context* context, struct conf_context* cfg_context, struct mame_option* option) {
	const char* s;
	int err;
	unsigned i;
	int rotate;

	assert( cfg_context == context->state.cfg_context );

	context->config.game_orientation = mame_game_orientation(option->game);

	context->config.inlist_combinemax_flag = mame_is_game_in_list(GAME_BLIT_COMBINE_MAX,option->game);
	strcpy(context->config.section_name, mame_game_name(option->game));
	strcpy(context->config.section_resolution, mame_game_resolution(option->game));
	if ((context->config.game_orientation & OSD_ORIENTATION_SWAP_XY) != 0)
		strcpy(context->config.section_orientation, "vertical");
	else
		strcpy(context->config.section_orientation, "horizontal");

	context->config.scanlines_flag = conf_bool_get_default(cfg_context, "display_scanlines");
	context->config.vsync_flag = conf_bool_get_default(cfg_context, "display_vsync");
	context->config.triplebuf_flag = conf_bool_get_default(cfg_context, "display_buffer");
	context->config.stretch = conf_int_get_default(cfg_context, "display_resize");
	context->config.magnify_flag = conf_bool_get_default(cfg_context, "display_magnify");
	context->config.rgb_flag = conf_bool_get_default(cfg_context, "display_rgb");
	context->config.adjust = conf_int_get_default(cfg_context, "display_adjust");

	s = conf_string_get_default(cfg_context, "display_skiplines");
	if (strcmp(s,"auto")==0) {
		context->config.skiplines = -1;
	} else {
		context->config.skiplines = atoi(s);
	}

	s = conf_string_get_default(cfg_context, "display_skipcolumns");
	if (strcmp(s,"auto")==0) {
		context->config.skipcolumns = -1;
	} else {
		context->config.skipcolumns = atoi(s);
	}

	option->ror = conf_bool_get_default(cfg_context, "display_ror");
	option->rol = conf_bool_get_default(cfg_context, "display_rol");
	option->flipx = conf_bool_get_default(cfg_context, "display_flipx");
	option->flipy = conf_bool_get_default(cfg_context, "display_flipy");

	rotate = conf_int_get_default(cfg_context, "display_rotate");
	if (rotate == ROTATE_AUTO) {
		rotate = ROTATE_BLIT;
	}
	switch (rotate) {
		case ROTATE_CORE :
			/* disable the blit orientation */
			context->config.blit_orientation = 0;
			/* enable the core orientation */
			option->norotate = 0;
			/* disable the ui orientation */
			option->ui_orientation = 0;
			break;
		case ROTATE_BLIT :
			/* enable the blit orientation */
			context->config.blit_orientation = video_orientation_compute(context->config.game_orientation, option->rol, option->ror, option->flipx, option->flipy);
			/* disable the core orientation */
			option->norotate = 1;
			option->ror = 0;
			option->rol = 0;
			option->flipx = 0;
			option->flipy = 0;
			/* enable the ui orientation */
			option->ui_orientation = video_orientation_inverse(context->config.game_orientation);
			break;
		case ROTATE_NONE :
			/* disable the blit orientation */
			context->config.blit_orientation = 0;
			/* disable the core orientation */
			option->norotate = 1;
			option->ror = 0;
			option->rol = 0;
			option->flipx = 0;
			option->flipy = 0;
			/* enable the ui orientation */
			option->ui_orientation = video_orientation_inverse(context->config.game_orientation);
			break;
	}

	context->config.combine = conf_int_get_default(cfg_context, "display_resizeeffect");
	context->config.effect = conf_int_get_default(cfg_context, "display_rgbeffect");
	context->config.turbo_speed_factor = conf_float_get_default(cfg_context, "misc_turbospeed");
	context->config.fps_speed_factor = conf_float_get_default(cfg_context, "misc_speed");
	context->config.fastest_time = conf_int_get_default(cfg_context, "misc_startuptime");
	context->config.measure_time = conf_int_get_default(cfg_context, "misc_timetorun");

	s = conf_string_get_default(cfg_context, "display_mode");
	strcpy(context->config.resolution,s);

	context->config.depth = conf_int_get_default(cfg_context, "display_depth");
	context->config.restore_flag = conf_bool_get_default(cfg_context, "display_restore");
	context->config.aspect_expansion_factor = conf_float_get_default(cfg_context, "display_expand");

	s = conf_string_get_default(cfg_context, "display_frameskip");
	if (strcmp(s,"auto")==0) {
		context->config.frameskip_auto_flag = 1;
		context->config.frameskip_factor = 1.0;
	} else {
		context->config.frameskip_auto_flag = 0;
		context->config.frameskip_factor = atof(s);
		if (context->config.frameskip_factor < 0.0)
			context->config.frameskip_factor = 0.0;
		if (context->config.frameskip_factor > 1.0)
			context->config.frameskip_factor = 1.0;
	}

#ifdef USE_SMP
	context->config.smp_flag = conf_bool_get_default(cfg_context, "misc_smp");
#else
	context->config.smp_flag = 0;
#endif

	/* load context->config.monitor config */
	err = monitor_load(cfg_context, &context->config.monitor);
	if (err<0) {
		fprintf(stderr,"%s\n", video_error_description_get());
		fprintf(stderr,"Please read the file `install.txt' and `mv.txt'\n");
		return -1;
	}
	if (err>0) {
		fprintf(stderr,"%s\n", video_error_description_get());
		fprintf(stderr,"Please read the file `install.txt' and `mv.txt'\n");
		return -1;
	}

	/* print the clock ranges */
	os_log(("advance:video: pclock %.3f - %.3f\n",(double)context->config.monitor.pclock.low,(double)context->config.monitor.pclock.high));
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (context->config.monitor.hclock[i].low)
			os_log(("advance:video: hclock %.3f - %.3f\n",(double)context->config.monitor.hclock[i].low,(double)context->config.monitor.hclock[i].high));
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (context->config.monitor.vclock[i].low)
			os_log(("advance:video: vclock %.3f - %.3f\n",(double)context->config.monitor.vclock[i].low,(double)context->config.monitor.vclock[i].high));

	/* load generate_linear config */
	err = generate_interpolate_load(cfg_context, &context->config.interpolate);
	if (err<0) {
		fprintf(stderr,"%s\n", video_error_description_get());
		fprintf(stderr,"Please read the file `install.txt' and `mv.txt'\n");
		return -1;
	} else if (err>0) {
		if (monitor_hclock_check(&context->config.monitor, 15720)) {
			/* Arcade Standard Resolution */
			os_log(("advance:video: default format standard resolution\n"));
			generate_default_atari_standard(&context->config.interpolate.map[0].gen);
			context->config.interpolate.map[0].hclock = 15720;
			context->config.interpolate.mac = 1;
		} else if (monitor_hclock_check(&context->config.monitor, 25000)) {
			/* Arcade Medium Resolution */
			os_log(("advance:video: default format medium resolution\n"));
			generate_default_atari_medium(&context->config.interpolate.map[0].gen);
			context->config.interpolate.map[0].hclock = 25000;
			context->config.interpolate.mac = 1;
		} else {
			/* VGA Resolution */
			os_log(("advance:video: default format vga resolution\n"));
			generate_default_vga(&context->config.interpolate.map[0].gen);
			context->config.interpolate.map[0].hclock = 31500;
			context->config.interpolate.mac = 1;
		}
	}

	/* Ignore the unused driver. These are generally the driver needed */
	/* for the text mode, not used by the emulator. */
	if (video_load(cfg_context, "slang") != 0) {
		return -1;
	}

	if (video_crtc_container_load(cfg_context, &context->config.crtc_bag)!=0) {
		fprintf(stderr,"Invalid modeline.\n");
		fprintf(stderr,"%s\n",video_error_description_get());
		return -1;
	}

	return 0;
}

void advance_video_done(struct advance_video_context* context) {
	video_crtc_container_done(&context->config.crtc_bag);
}

int advance_video_inner_init(struct advance_video_context* context, struct mame_option* option)
{
	video_init();

	if (!video_blit_set_mmx(os_mmx_get())) {
		fprintf(stderr,"This executable version requires an MMX processor\n");
		return -1;
	}

	if (video_config_mode(context,option) != 0) {
		return -1;
	}

	return 0;
}

void advance_video_inner_done(struct advance_video_context* context) {
	video_done();
}

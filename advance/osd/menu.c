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

/***************************************************************************/
/* Menu */

static int video_mode_menu(struct advance_video_context* context, struct advance_ui_context* ui_context, int selected, unsigned input)
{
	const char *menu_item[VIDEO_CRTC_MAX + 2];
	char flag[VIDEO_CRTC_MAX + 2];
	const adv_crtc* entry[VIDEO_CRTC_MAX + 2];
	int i;
	int total;

	if (selected >= 1)
		selected = selected - 1;
	else
		selected = 0;

	total = 0;

	menu_item[total] = strdup("Auto");
	entry[total] = 0;
	flag[total] = strcmp("auto", context->config.resolution_buffer)==0;
	++total;

	for(i=0;i<context->state.crtc_mac;++i) {
		char buffer[128];
		const adv_crtc* crtc = context->state.crtc_map[i];
		entry[total] = crtc;
		mode_desc_print(context, buffer, sizeof(buffer), crtc);
		menu_item[total] = strdup(buffer);
		flag[total] = strcmp(crtc_name_get(crtc), context->config.resolution_buffer)==0;
		++total;
	}

	menu_item[total] = "Return to Main Menu";
	flag[total] = 0;
	++total;

	menu_item[total] = 0;
	flag[total] = 0;

	advance_ui_menu_vect(ui_context, "Video Mode", menu_item, 0, flag, selected, 0);

	if (input == OSD_INPUT_DOWN)
	{
		selected = (selected + 1) % total;
	}

	if (input == OSD_INPUT_UP)
	{
		selected = (selected + total - 1) % total;
	}

	if (input == OSD_INPUT_SELECT)
	{
		if (selected == total - 1) selected = -1;
		else if (selected == 0) {
			struct advance_video_config_context config = context->config;

			sncpy(config.resolution_buffer, sizeof(config.resolution_buffer), "auto");

			advance_video_reconfigure(context, &config);

			/* show at screen the new configuration name */
			advance_ui_message(ui_context, mode_current_name(context));
		} else {
			struct advance_video_config_context config = context->config;

			sncpy(config.resolution_buffer, sizeof(config.resolution_buffer), crtc_name_get(entry[selected]));

			advance_video_reconfigure(context, &config);
		}
	}

	if (input == OSD_INPUT_CANCEL)
		selected = -1;

	if (input == OSD_INPUT_CONFIGURE)
		selected = -2;

	for(i=0;i<total-1;++i) {
		free((char*)menu_item[i]);
	}

	return selected + 1;
}

static int video_pipeline_menu(struct advance_video_context* context, struct advance_ui_context* ui_context, int selected, unsigned input)
{
	const char *menu_item[64 + 2];
	char flag[64 + 2];
	int i;
	const struct video_stage_horz_struct* stage;
	char buffer[256];
	int total;

	if (selected >= 1)
		selected = selected - 1;
	else
		selected = 0;

	total = 0;

	for(i=1, stage=video_pipeline_begin(&context->state.blit_pipeline_video);stage!=video_pipeline_end(&context->state.blit_pipeline_video);++stage, ++i) {
		if (stage == video_pipeline_pivot(&context->state.blit_pipeline_video)) {
			snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.blit_pipeline_video)->type));
			++i;
			menu_item[total] = strdup(buffer);
			flag[total] = 0;
			++total;
		}
		if (stage->sbpp != stage->sdp)
			snprintf(buffer, sizeof(buffer), "(%d) %s, p %d, dp %d", i, pipe_name(stage->type), stage->sbpp, stage->sdp);
		else
			snprintf(buffer, sizeof(buffer), "(%d) %s, p %d", i, pipe_name(stage->type), stage->sbpp);
		menu_item[total] = strdup(buffer);
		flag[total] = 0;
		++total;
	}
	if (stage == video_pipeline_pivot(&context->state.blit_pipeline_video)) {
		snprintf(buffer, sizeof(buffer), "(%d) %s", i, pipe_name(video_pipeline_vert(&context->state.blit_pipeline_video)->type));
		++i;
		menu_item[total] = strdup(buffer);
		flag[total] = 0;
		++total;
	}

	if (context->state.pipeline_measure_flag) {
		menu_item[total] = strdup("Time measure not completed");
		flag[total] = 0;
		++total;
	} else {
		snprintf(buffer, sizeof(buffer), "Direct write %.2f ms", context->state.pipeline_measure_direct_result * 1000);
		menu_item[total] = strdup(buffer);
		flag[total] = 0;
		++total;

		snprintf(buffer, sizeof(buffer), "Buffer write %.2f ms", context->state.pipeline_measure_buffer_result * 1000);
		menu_item[total] = strdup(buffer);
		flag[total] = 0;
		++total;

		if (context->state.pipeline_measure_bestisdirect_flag) {
			menu_item[total] = strdup("Using direct write");
			flag[total] = 0;
			++total;
		} else {
			menu_item[total] = strdup("Using buffered write");
			flag[total] = 0;
			++total;
		}
	}

	menu_item[total] = "Return to Main Menu";
	flag[total] = 0;
	++total;

	menu_item[total] = 0;
	flag[total] = 0;

	advance_ui_menu_vect(ui_context, "Video Pipeline", menu_item, 0, flag, selected, 0);

	if (input == OSD_INPUT_DOWN)
	{
		selected = (selected + 1) % total;
	}

	if (input == OSD_INPUT_UP)
	{
		selected = (selected + total - 1) % total;
	}

	if (input == OSD_INPUT_SELECT)
	{
		if (selected == total - 1) selected = -1;
	}

	if (input == OSD_INPUT_CANCEL)
		selected = -1;

	if (input == OSD_INPUT_CONFIGURE)
		selected = -2;

	for(i=0;i<total-1;++i) {
		free((char*)menu_item[i]);
	}

	return selected + 1;
}

int osd2_video_menu(int selected, unsigned input)
{
	const char* menu_item[32];
	const char* menu_subitem[32];
	char flag[32];
	int total;
	int arrowize;

	int resolution_index;
	int stretch_index;
	int vsync_index;
	int combine_index;
	int effect_index;
	int save_game_index;
	int save_resolution_index;
	int save_resolutionclock_index;
	int pipeline_index;
	int magnify_index;
	int scanline_index;
	int index_index;
	int smp_index;
	int crash_index;
	char mode_buffer[128];

	struct advance_video_context* context = &CONTEXT.video;
	struct advance_global_context* global_context = &CONTEXT.global;
	struct advance_ui_context* ui_context = &CONTEXT.ui;

	if (selected >= 1)
		selected = selected - 1;
	else
		selected = 0;

	if (context->state.menu_sub_flag)
	{
		int ret = 0;
		switch (context->state.menu_sub_flag) {
		case 1 : ret = video_mode_menu(context, ui_context, context->state.menu_sub_selected, input); break;
		case 2 : ret = video_pipeline_menu(context, ui_context, context->state.menu_sub_selected, input); break;
		}
		switch (ret) {
		case -1 : return -1; /* hide interface */
		case 0 : context->state.menu_sub_flag = 0; context->state.menu_sub_selected = 1; break; /* close submenu */
		default: context->state.menu_sub_selected = ret; break;
		}
		return selected + 1;
	}

	total = 0;

	snprintf(mode_buffer, sizeof(mode_buffer), "%dx%dx%d %.1f/%.1f/%.1f",
		video_size_x(),
		video_size_y(),
		video_bits_per_pixel(),
		(double)crtc_pclock_get(&context->state.crtc_effective) / 1E6,
		(double)crtc_hclock_get(&context->state.crtc_effective) / 1E3,
		(double)crtc_vclock_get(&context->state.crtc_effective)
	);
	menu_item[total] = mode_buffer;
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	resolution_index = total;
	menu_item[total] = "Mode";
	menu_subitem[total] = context->config.resolution_buffer;
	flag[total] = 0;
	++total;

	if (!context->state.game_vector_flag) {
		if (strcmp(context->config.resolution_buffer, "auto")==0) {
			magnify_index = total;
			switch (mode_current_magnify(context)) {
			default :
			case 1 : menu_item[total] = "Magnify [1]"; break;
			case 2 : menu_item[total] = "Magnify [2]"; break;
			case 3 : menu_item[total] = "Magnify [3]"; break;
			case 4 : menu_item[total] = "Magnify [4]"; break;
			}
			switch (context->config.magnify_factor) {
			default :
			case 0 : menu_subitem[total] = "auto"; break;
			case 1 : menu_subitem[total] = "1"; break;
			case 2 : menu_subitem[total] = "2"; break;
			case 3 : menu_subitem[total] = "3"; break;
			case 4 : menu_subitem[total] = "4"; break;
			}
			flag[total] = 0;
			++total;

			scanline_index = total;
			switch (video_scan()) {
			case 0 : menu_item[total] = "Scanline [single]"; break;
			case 1 : menu_item[total] = "Scanline [double]"; break;
			case -1 : menu_item[total] = "Scanline [interlace]"; break;
			}
			if (context->config.scanlines_flag)
				menu_subitem[total] = "yes";
			else
				menu_subitem[total] = "no";
			flag[total] = 0;
			++total;

		} else {
			magnify_index = -1;
			scanline_index = -1;
		}

		menu_item[total] = "Options";
		menu_subitem[total] = 0;
		flag[total] = 0;
		++total;

		stretch_index = total;
		switch (mode_current_stretch(context)) {
		case STRETCH_NONE : menu_item[total] = "Resize [no]"; break;
		case STRETCH_INTEGER_XY : menu_item[total] = "Resize [integer]"; break;
		case STRETCH_INTEGER_X_FRACTIONAL_Y : menu_item[total] = "Resize [mixed]"; break;
		case STRETCH_FRACTIONAL_XY : menu_item[total] = "Resize [fractional]"; break;
		}
		switch (context->config.stretch) {
		case STRETCH_NONE : menu_subitem[total] = "no"; break;
		case STRETCH_INTEGER_XY : menu_subitem[total] = "integer"; break;
		case STRETCH_INTEGER_X_FRACTIONAL_Y : menu_subitem[total] = "mixed"; break;
		case STRETCH_FRACTIONAL_XY : menu_subitem[total] = "fractional"; break;
		}
		flag[total] = 0;
		++total;
	} else {
		magnify_index = -1;
		scanline_index = -1;

		menu_item[total] = "Options";
		menu_subitem[total] = 0;
		flag[total] = 0;
		++total;

		stretch_index = -1;
	}

	index_index = total;
	switch (video_index()) {
	case MODE_FLAGS_INDEX_PALETTE8 :
		menu_item[total] = "Color [palette8]";
		break;
	case MODE_FLAGS_INDEX_BGR8 :
		menu_item[total] = "Color [bgr8]";
		break;
	case MODE_FLAGS_INDEX_BGR15 :
		menu_item[total] = "Color [bgr15]";
		break;
	case MODE_FLAGS_INDEX_BGR16 :
		menu_item[total] = "Color [bgr16]";
		break;
	case MODE_FLAGS_INDEX_BGR32 :
		menu_item[total] = "Color [bgr32]";
		break;
	case MODE_FLAGS_INDEX_YUY2 :
		menu_item[total] = "Color [yuy2]";
		break;
	default:
		menu_item[total] = "Color [unknown]";
		break;
	}
	switch (context->config.index) {
	case MODE_FLAGS_INDEX_NONE :
	menu_subitem[total] = "auto";
			break;
	case MODE_FLAGS_INDEX_PALETTE8 :
	menu_subitem[total] = "palette8";
		break;
	case MODE_FLAGS_INDEX_BGR8 :
		menu_subitem[total] = "bgr8";
		break;
	case MODE_FLAGS_INDEX_BGR15 :
		menu_subitem[total] = "bgr15";
		break;
	case MODE_FLAGS_INDEX_BGR16 :
		menu_subitem[total] = "bgr16";
		break;
	case MODE_FLAGS_INDEX_BGR32 :
		menu_subitem[total] = "bgr32";
		break;
	case MODE_FLAGS_INDEX_YUY2 :
		menu_subitem[total] = "yuy2";
		break;
	default:
		menu_subitem[total] = "unknown";
		break;
	}
	flag[total] = 0;
	++total;

	combine_index = total;
	switch (context->state.combine) {
	case COMBINE_NONE : menu_item[total] = "Resize Effect [no]"; break;
	case COMBINE_MAXMIN : menu_item[total] = "Resize Effect [max]"; break;
	case COMBINE_MEAN : menu_item[total] = "Resize Effect [mean]"; break;
	case COMBINE_FILTER : menu_item[total] = "Resize Effect [filter]"; break;
	case COMBINE_SCALE : menu_item[total] = "Resize Effect [scale]"; break;
	case COMBINE_LQ : menu_item[total] = "Resize Effect [lq]"; break;
	case COMBINE_HQ : menu_item[total] = "Resize Effect [hq]"; break;
	}
	switch (context->config.combine) {
	case COMBINE_AUTO : menu_subitem[total] = "auto"; break;
	case COMBINE_NONE : menu_subitem[total] = "no"; break;
	case COMBINE_MAXMIN : menu_subitem[total] = "max"; break;
	case COMBINE_MEAN : menu_subitem[total] = "mean"; break;
	case COMBINE_FILTER : menu_subitem[total] = "filter"; break;
	case COMBINE_SCALE : menu_subitem[total] = "scale"; break;
	case COMBINE_LQ : menu_subitem[total] = "lq"; break;
	case COMBINE_HQ : menu_subitem[total] = "hq"; break;
	}
	flag[total] = 0;
	++total;

	effect_index = total;
	switch (context->state.rgb_effect) {
	case EFFECT_NONE : menu_item[total] = "Rgb Effect [no]"; break;
	case EFFECT_RGB_TRIAD3PIX : menu_item[total] = "Rgb Effect [triad3dot]"; break;
	case EFFECT_RGB_TRIAD6PIX : menu_item[total] = "Rgb Effect [triad6dot]"; break;
	case EFFECT_RGB_TRIAD16PIX : menu_item[total] = "Rgb Effect [triad16dot]"; break;
	case EFFECT_RGB_TRIADSTRONG3PIX : menu_item[total] = "Rgb Effect [triadstrong3dot]"; break;
	case EFFECT_RGB_TRIADSTRONG6PIX : menu_item[total] = "Rgb Effect [triadstrong6dot]"; break;
	case EFFECT_RGB_TRIADSTRONG16PIX : menu_item[total] = "Rgb Effect [triadstrong16dot]"; break;
	case EFFECT_RGB_SCANDOUBLEHORZ : menu_item[total] = "Rgb Effect [scan2horz]"; break;
	case EFFECT_RGB_SCANTRIPLEHORZ : menu_item[total] = "Rgb Effect [scan3horz]"; break;
	case EFFECT_RGB_SCANDOUBLEVERT : menu_item[total] = "Rgb Effect [scan2vert]"; break;
	case EFFECT_RGB_SCANTRIPLEVERT : menu_item[total] = "Rgb Effect [scan3vert]"; break;
	}
	switch (context->config.rgb_effect) {
	case EFFECT_NONE : menu_subitem[total] = "no"; break;
	case EFFECT_RGB_TRIAD3PIX : menu_subitem[total] = "triad3dot"; break;
	case EFFECT_RGB_TRIAD6PIX : menu_subitem[total] = "triad6dot"; break;
	case EFFECT_RGB_TRIAD16PIX : menu_subitem[total] = "triad16dot"; break;
	case EFFECT_RGB_TRIADSTRONG3PIX : menu_subitem[total] = "triadstrong3dot"; break;
	case EFFECT_RGB_TRIADSTRONG6PIX : menu_subitem[total] = "triadstrong6dot"; break;
	case EFFECT_RGB_TRIADSTRONG16PIX : menu_subitem[total] = "triadstrong16dot"; break;
	case EFFECT_RGB_SCANDOUBLEHORZ : menu_subitem[total] = "scan2horz"; break;
	case EFFECT_RGB_SCANTRIPLEHORZ : menu_subitem[total] = "scan3horz"; break;
	case EFFECT_RGB_SCANDOUBLEVERT : menu_subitem[total] = "scan2vert"; break;
	case EFFECT_RGB_SCANTRIPLEVERT : menu_subitem[total] = "scan3vert"; break;
	}
	flag[total] = 0;
	++total;

	vsync_index = total;
	switch (context->state.vsync_flag) {
	case 0 : menu_item[total] = "Vsync [no]"; break;
	case 1 : menu_item[total] = "Vsync [yes]"; break;
	}
	switch (context->config.vsync_flag) {
	case 0 : menu_subitem[total] = "no"; break;
	case 1 : menu_subitem[total] = "yes"; break;
	}
	flag[total] = 0;
	++total;

#ifdef USE_SMP
	smp_index = total;
	switch (context->config.smp_flag) {
	case 0 : menu_item[total] = "SMP [no]"; break;
	case 1 : menu_item[total] = "SMP [yes]"; break;
	}
	switch (context->config.smp_flag) {
	case 0 : menu_subitem[total] = "no"; break;
	case 1 : menu_subitem[total] = "yes"; break;
	}
	flag[total] = 0;
	++total;
#else
	smp_index = -1;
#endif

	pipeline_index = total;
	menu_item[total] = "Show Pipeline";
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	if (global_context->state.is_config_writable) {
		save_game_index = total;
		menu_item[total] = "Save for this game";
		menu_subitem[total] = 0;
		flag[total] = 0;
		++total;

		save_resolution_index = total;
		if (context->state.game_vector_flag)
			menu_item[total] = "Save for all vector games";
		else
			menu_item[total] = "Save for this game size";
		menu_subitem[total] = 0;
		flag[total] = 0;
		++total;

		if (!context->state.game_vector_flag) {
			save_resolutionclock_index = total;
			menu_item[total] = "Save for this game size/freq";
			menu_subitem[total] = 0;
			flag[total] = 0;
			++total;
		} else {
			save_resolutionclock_index = -1;
		}
	} else {
		save_game_index = -1;
		save_resolution_index = -1;
		save_resolutionclock_index = -1;
	}

	if (context->config.crash_flag) {
		crash_index = total;
		menu_item[total] = "Crash";
		menu_subitem[total] = 0;
		flag[total] = 0;
		++total;
	} else {
		crash_index = -1;
	}

	menu_item[total] = "Return to Main Menu";
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	menu_item[total] = 0;
	menu_subitem[total] = 0;
	flag[total] = 0;

	if (selected == vsync_index
		|| selected == smp_index
		|| selected == stretch_index
		|| selected == combine_index
		|| selected == effect_index
		|| selected == magnify_index
		|| selected == index_index
		|| selected == scanline_index
	)
		arrowize = 3;
	else
		arrowize = 0;

	advance_ui_menu_vect(ui_context, "Video Menu", menu_item, menu_subitem, flag, selected, arrowize);

	if (input == OSD_INPUT_DOWN)
	{
		selected = (selected + 1) % total;
	}

	if (input == OSD_INPUT_UP)
	{
		selected = (selected + total - 1) % total;
	}

	if (input == OSD_INPUT_SELECT)
	{
		if (selected == total - 1) {
			selected = -1;
		} else if (selected == resolution_index) {
			context->state.menu_sub_flag = 1;
		} else if (selected == pipeline_index) {
			context->state.menu_sub_flag = 2;
		} else if (selected == save_game_index) {
			advance_video_config_save(context, context->config.section_name_buffer);
		} else if (selected == save_resolution_index) {
			advance_video_config_save(context, context->config.section_resolution_buffer);
		} else if (selected == save_resolutionclock_index) {
			advance_video_config_save(context, context->config.section_resolutionclock_buffer);
		} else if (selected == crash_index) {
			target_crash();
		}
	}

	if (input == OSD_INPUT_RIGHT)
	{
		if (selected == combine_index) {
			struct advance_video_config_context config = context->config;
			switch (config.combine) {
			case COMBINE_AUTO : config.combine = COMBINE_NONE; break;
			case COMBINE_NONE : config.combine = COMBINE_MAXMIN; break;
			case COMBINE_MAXMIN : config.combine = COMBINE_MEAN; break;
			case COMBINE_MEAN : config.combine = COMBINE_FILTER; break;
			case COMBINE_FILTER : config.combine = COMBINE_SCALE; break;
			case COMBINE_SCALE : config.combine = COMBINE_LQ; break;
			case COMBINE_LQ : config.combine = COMBINE_HQ; break;
			case COMBINE_HQ : config.combine = COMBINE_AUTO; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == effect_index) {
			struct advance_video_config_context config = context->config;
			switch (config.rgb_effect) {
			case EFFECT_NONE : config.rgb_effect = EFFECT_RGB_TRIAD3PIX; break;
			case EFFECT_RGB_TRIAD3PIX : config.rgb_effect = EFFECT_RGB_TRIADSTRONG3PIX; break;
			case EFFECT_RGB_TRIADSTRONG3PIX : config.rgb_effect = EFFECT_RGB_TRIAD6PIX; break;
			case EFFECT_RGB_TRIAD6PIX : config.rgb_effect = EFFECT_RGB_TRIADSTRONG6PIX; break;
			case EFFECT_RGB_TRIADSTRONG6PIX : config.rgb_effect = EFFECT_RGB_TRIAD16PIX; break;
			case EFFECT_RGB_TRIAD16PIX : config.rgb_effect = EFFECT_RGB_TRIADSTRONG16PIX; break;
			case EFFECT_RGB_TRIADSTRONG16PIX : config.rgb_effect = EFFECT_RGB_SCANDOUBLEHORZ; break;
			case EFFECT_RGB_SCANDOUBLEHORZ : config.rgb_effect = EFFECT_RGB_SCANTRIPLEHORZ; break;
			case EFFECT_RGB_SCANTRIPLEHORZ : config.rgb_effect = EFFECT_RGB_SCANDOUBLEVERT; break;
			case EFFECT_RGB_SCANDOUBLEVERT : config.rgb_effect = EFFECT_RGB_SCANTRIPLEVERT; break;
			case EFFECT_RGB_SCANTRIPLEVERT : config.rgb_effect = EFFECT_NONE; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == vsync_index) {
			struct advance_video_config_context config = context->config;
			switch (config.vsync_flag) {
			case 0 : config.vsync_flag = 1; break;
			case 1 : config.vsync_flag = 0; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == smp_index) {
			struct advance_video_config_context config = context->config;
			switch (config.smp_flag) {
			case 0 : config.smp_flag = 1; break;
			case 1 : config.smp_flag = 0; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == scanline_index) {
			struct advance_video_config_context config = context->config;
			config.scanlines_flag = !config.scanlines_flag;
			advance_video_reconfigure(context, &config);
		} else if (selected == magnify_index) {
			struct advance_video_config_context config = context->config;
			if (config.magnify_factor == 4)
				config.magnify_factor = 0;
			else
				config.magnify_factor += 1;
			advance_video_reconfigure(context, &config);
		} else if (selected == index_index) {
			struct advance_video_config_context config = context->config;
			switch (config.index) {
			case MODE_FLAGS_INDEX_NONE : config.index = MODE_FLAGS_INDEX_PALETTE8; break;
			case MODE_FLAGS_INDEX_PALETTE8 : config.index = MODE_FLAGS_INDEX_BGR8; break;
			case MODE_FLAGS_INDEX_BGR8 : config.index = MODE_FLAGS_INDEX_BGR15; break;
			case MODE_FLAGS_INDEX_BGR15 : config.index = MODE_FLAGS_INDEX_BGR16; break;
			case MODE_FLAGS_INDEX_BGR16 : config.index = MODE_FLAGS_INDEX_BGR32; break;
			case MODE_FLAGS_INDEX_BGR32 : config.index = MODE_FLAGS_INDEX_YUY2; break;
			case MODE_FLAGS_INDEX_YUY2 : config.index = MODE_FLAGS_INDEX_NONE; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == stretch_index) {
			struct advance_video_config_context config = context->config;
			switch (config.stretch) {
			case STRETCH_NONE : config.stretch = STRETCH_FRACTIONAL_XY; break;
			case STRETCH_INTEGER_XY : config.stretch = STRETCH_NONE; break;
			case STRETCH_INTEGER_X_FRACTIONAL_Y : config.stretch = STRETCH_INTEGER_XY; break;
			case STRETCH_FRACTIONAL_XY : config.stretch = STRETCH_INTEGER_X_FRACTIONAL_Y; break;
			}
			advance_video_reconfigure(context, &config);
		}
	}

	if (input == OSD_INPUT_LEFT)
	{
		if (selected == combine_index) {
			struct advance_video_config_context config = context->config;
			switch (config.combine) {
			case COMBINE_AUTO : config.combine = COMBINE_HQ; break;
			case COMBINE_NONE : config.combine = COMBINE_AUTO; break;
			case COMBINE_MAXMIN : config.combine = COMBINE_NONE; break;
			case COMBINE_MEAN : config.combine = COMBINE_MAXMIN; break;
			case COMBINE_FILTER : config.combine = COMBINE_MEAN; break;
			case COMBINE_SCALE : config.combine = COMBINE_FILTER; break;
			case COMBINE_LQ : config.combine = COMBINE_SCALE; break;
			case COMBINE_HQ : config.combine = COMBINE_LQ; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == effect_index) {
			struct advance_video_config_context config = context->config;
			switch (config.rgb_effect) {
			case EFFECT_NONE : config.rgb_effect = EFFECT_RGB_SCANTRIPLEVERT; break;
			case EFFECT_RGB_TRIAD3PIX : config.rgb_effect = EFFECT_NONE; break;
			case EFFECT_RGB_TRIADSTRONG3PIX : config.rgb_effect = EFFECT_RGB_TRIAD3PIX; break;
			case EFFECT_RGB_TRIAD6PIX : config.rgb_effect = EFFECT_RGB_TRIADSTRONG3PIX; break;
			case EFFECT_RGB_TRIADSTRONG6PIX : config.rgb_effect = EFFECT_RGB_TRIAD6PIX; break;
			case EFFECT_RGB_TRIAD16PIX : config.rgb_effect = EFFECT_RGB_TRIADSTRONG6PIX; break;
			case EFFECT_RGB_TRIADSTRONG16PIX : config.rgb_effect = EFFECT_RGB_TRIAD16PIX; break;
			case EFFECT_RGB_SCANDOUBLEHORZ : config.rgb_effect = EFFECT_RGB_TRIADSTRONG16PIX; break;
			case EFFECT_RGB_SCANTRIPLEHORZ : config.rgb_effect = EFFECT_RGB_SCANDOUBLEHORZ; break;
			case EFFECT_RGB_SCANDOUBLEVERT : config.rgb_effect = EFFECT_RGB_SCANTRIPLEHORZ; break;
			case EFFECT_RGB_SCANTRIPLEVERT : config.rgb_effect = EFFECT_RGB_SCANDOUBLEVERT; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == vsync_index) {
			struct advance_video_config_context config = context->config;
			switch (config.vsync_flag) {
			case 0 : config.vsync_flag = 1; break;
			case 1 : config.vsync_flag = 0; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == smp_index) {
			struct advance_video_config_context config = context->config;
			switch (config.smp_flag) {
			case 0 : config.smp_flag = 1; break;
			case 1 : config.smp_flag = 0;  break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == scanline_index) {
			struct advance_video_config_context config = context->config;
			config.scanlines_flag = !config.scanlines_flag;
			advance_video_reconfigure(context, &config);
		} else if (selected == magnify_index) {
			struct advance_video_config_context config = context->config;
			if (config.magnify_factor == 0)
				config.magnify_factor = 4;
			else
				config.magnify_factor -= 1;
			advance_video_reconfigure(context, &config);
		} else if (selected == index_index) {
			struct advance_video_config_context config = context->config;
			switch (config.index) {
			case MODE_FLAGS_INDEX_NONE : config.index = MODE_FLAGS_INDEX_YUY2; break;
			case MODE_FLAGS_INDEX_PALETTE8 : config.index = MODE_FLAGS_INDEX_NONE; break;
			case MODE_FLAGS_INDEX_BGR8 : config.index = MODE_FLAGS_INDEX_PALETTE8; break;
			case MODE_FLAGS_INDEX_BGR15 : config.index = MODE_FLAGS_INDEX_BGR8; break;
			case MODE_FLAGS_INDEX_BGR16 : config.index = MODE_FLAGS_INDEX_BGR15; break;
			case MODE_FLAGS_INDEX_BGR32 : config.index = MODE_FLAGS_INDEX_BGR16; break;
			case MODE_FLAGS_INDEX_YUY2 : config.index = MODE_FLAGS_INDEX_BGR32; break;
			}
			advance_video_reconfigure(context, &config);
		} else if (selected == stretch_index) {
			struct advance_video_config_context config = context->config;
			switch (config.stretch) {
			case STRETCH_NONE : config.stretch = STRETCH_INTEGER_XY; break;
			case STRETCH_INTEGER_XY : config.stretch = STRETCH_INTEGER_X_FRACTIONAL_Y; break;
			case STRETCH_INTEGER_X_FRACTIONAL_Y : config.stretch = STRETCH_FRACTIONAL_XY; break;
			case STRETCH_FRACTIONAL_XY : config.stretch = STRETCH_NONE; break;
			}
			advance_video_reconfigure(context, &config);
		}
	}

	if (input == OSD_INPUT_CANCEL)
		selected = -1;

	if (input == OSD_INPUT_CONFIGURE)
		selected = -2;

	return selected + 1;
}

int osd2_audio_menu(int selected, unsigned input)
{
	const char* menu_item[32];
	const char* menu_subitem[32];
	char flag[32];
	int total;
	int arrowize;

	char volume_buffer[128];
	char adjust_buffer[128];
	char eql_buffer[128];
	char eqm_buffer[128];
	char eqh_buffer[128];

	struct advance_video_context* video_context = &CONTEXT.video;
	struct advance_sound_context* sound_context = &CONTEXT.sound;
	struct advance_global_context* global_context = &CONTEXT.global;
	struct advance_ui_context* ui_context = &CONTEXT.ui;

	int mode_index;
	int adjust_index;
	int volume_index;
	int normalize_index;
	int eql_index;
	int eqm_index;
	int eqh_index;

	if (selected >= 1)
		selected = selected - 1;
	else
		selected = 0;

	total = 0;

	mode_index = total;
	switch (sound_context->state.output_mode) {
	case SOUND_MODE_MONO : menu_item[total] = "Mode [mono]"; break;
	case SOUND_MODE_STEREO : menu_item[total] = "Mode [stereo]"; break;
	case SOUND_MODE_SURROUND : menu_item[total] = "Mode [surround]"; break;
	}
	switch (sound_context->config.mode) {
	case SOUND_MODE_AUTO : menu_subitem[total] = "auto"; break;
	case SOUND_MODE_MONO : menu_subitem[total] = "mono"; break;
	case SOUND_MODE_STEREO : menu_subitem[total] = "stereo"; break;
	case SOUND_MODE_SURROUND : menu_subitem[total] = "surround"; break;
	}
	flag[total] = 0;
	++total;

	volume_index = total;
	menu_item[total] = "Attenuation (dB)";
	snprintf(volume_buffer, sizeof(volume_buffer), "%d", sound_context->config.attenuation);
	menu_subitem[total] = volume_buffer;
	flag[total] = 0;
	++total;

	menu_item[total] = "Normalize";
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	normalize_index = total;
	menu_item[total] = "Auto limit";
	if (sound_context->config.normalize_flag)
		menu_subitem[total] = "yes";
	else
		menu_subitem[total] = "no";
	flag[total] = 0;
	++total;

	adjust_index = total;
	menu_item[total] = "Amplifier (dB)";
	if (sound_context->config.adjust >= 0)
		snprintf(adjust_buffer, sizeof(adjust_buffer), "%d", sound_context->config.adjust);
	else
		strcpy(adjust_buffer, "auto");
	menu_subitem[total] = adjust_buffer;
	flag[total] = 0;
	++total;

	menu_item[total] = "Equalizer";
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	eql_index = total;
	menu_item[total] = "Low Freq (dB)";
	snprintf(eql_buffer, sizeof(eql_buffer), "%d", sound_context->config.equalizer_low);
	menu_subitem[total] = eql_buffer;
	flag[total] = 0;
	++total;

	eqm_index = total;
	menu_item[total] = "Mid Freq (dB)";
	snprintf(eqm_buffer, sizeof(eqm_buffer), "%d", sound_context->config.equalizer_mid);
	menu_subitem[total] = eqm_buffer;
	flag[total] = 0;
	++total;

	eqh_index = total;
	menu_item[total] = "High Freq (dB)";
	snprintf(eqh_buffer, sizeof(eqh_buffer), "%d", sound_context->config.equalizer_high);
	menu_subitem[total] = eqh_buffer;
	flag[total] = 0;
	++total;

	menu_item[total] = "Return to Main Menu";
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	menu_item[total] = 0;
	menu_subitem[total] = 0;
	flag[total] = 0;

	if (selected == mode_index
		|| selected == volume_index
		|| selected == adjust_index
		|| selected == normalize_index
	)
		arrowize = 3;
	else
		arrowize = 0;

	advance_ui_menu_vect(ui_context, "Audio Menu", menu_item, menu_subitem, flag, selected, arrowize);

	if (input == OSD_INPUT_DOWN)
	{
		selected = (selected + 1) % total;
	}

	if (input == OSD_INPUT_UP)
	{
		selected = (selected + total - 1) % total;
	}

	if (input == OSD_INPUT_SELECT)
	{
		if (selected == total - 1) {
			selected = -1;
		}
	}

	if (input == OSD_INPUT_RIGHT)
	{
		if (selected == volume_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.attenuation = sound_context->config.attenuation + 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == adjust_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.adjust = sound_context->config.adjust + 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == normalize_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.normalize_flag = !sound_context->config.normalize_flag;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == mode_index) {
			struct advance_sound_config_context config = sound_context->config;
			switch (config.mode) {
			case SOUND_MODE_AUTO : config.mode = SOUND_MODE_MONO; break;
			case SOUND_MODE_MONO : config.mode = SOUND_MODE_STEREO; break;
			case SOUND_MODE_STEREO : config.mode = SOUND_MODE_SURROUND; break;
			case SOUND_MODE_SURROUND : config.mode = SOUND_MODE_AUTO; break;
			}
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == eql_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.equalizer_low = sound_context->config.equalizer_low + 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == eqm_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.equalizer_mid = sound_context->config.equalizer_mid + 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == eqh_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.equalizer_high = sound_context->config.equalizer_high + 1;
			advance_sound_reconfigure(sound_context, &config);
		}
	}

	if (input == OSD_INPUT_LEFT)
	{
		if (selected == volume_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.attenuation = sound_context->config.attenuation - 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == adjust_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.adjust = sound_context->config.adjust - 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == normalize_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.normalize_flag = !sound_context->config.normalize_flag;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == mode_index) {
			struct advance_sound_config_context config = sound_context->config;
			switch (config.mode) {
			case SOUND_MODE_AUTO : config.mode = SOUND_MODE_SURROUND; break;
			case SOUND_MODE_MONO : config.mode = SOUND_MODE_AUTO; break;
			case SOUND_MODE_STEREO : config.mode = SOUND_MODE_MONO; break;
			case SOUND_MODE_SURROUND : config.mode = SOUND_MODE_STEREO; break;
			}
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == eql_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.equalizer_low = sound_context->config.equalizer_low - 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == eqm_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.equalizer_mid = sound_context->config.equalizer_mid - 1;
			advance_sound_reconfigure(sound_context, &config);
		} else if (selected == eqh_index) {
			struct advance_sound_config_context config = sound_context->config;
			config.equalizer_high = sound_context->config.equalizer_high - 1;
			advance_sound_reconfigure(sound_context, &config);
		}
	}

	if (input == OSD_INPUT_CANCEL)
		selected = -1;

	if (input == OSD_INPUT_CONFIGURE)
		selected = -2;

	return selected + 1;
}


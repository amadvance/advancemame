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

#include <string.h>
#include <stdlib.h>

/***************************************************************************/
/* Menu */

static int video_mode_menu(struct advance_video_context* context, int selected, unsigned input)
{
	const char *menu_item[VIDEO_CRTC_MAX + 2];
	char flag[VIDEO_CRTC_MAX + 2];
	const video_crtc* entry[VIDEO_CRTC_MAX + 2];
	int i;
	int total;

	if (selected >= 1)
		selected = selected - 1;
	else
		selected = 0;

	total = 0;

	menu_item[total] = strdup("Auto");
	entry[total] = 0;
	flag[total] = strcmp("auto",context->config.resolution)==0;
	++total;

	for(i=0;i<context->state.crtc_mac;++i) {
		const video_crtc* crtc = context->state.crtc_map[i];
		entry[total] = crtc;
		menu_item[total] = strdup(mode_desc(context,crtc));
		flag[total] = strcmp(crtc_name_get(crtc),context->config.resolution)==0;
		++total;
	}

	menu_item[total] = mame_ui_gettext("Return to Main Menu");
	flag[total] = 0;
	++total;

	menu_item[total] = 0;
	flag[total] = 0;

	mame_ui_menu(menu_item,0,flag,selected,0);

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
			strcpy(context->config.resolution,"auto");
			advance_video_change(context);

			/* show at screen the new configuration name */
			mame_ui_message(mode_current_name(context));

			mame_ui_refresh();
		} else {
			strcpy(context->config.resolution,crtc_name_get(entry[selected]));
			advance_video_change(context);
			
			mame_ui_refresh();
		}
	}

	if (input == OSD_INPUT_CANCEL)
		selected = -1;

	if (input == OSD_INPUT_CONFIGURE)
		selected = -2;

	if (selected == -1 || selected == -2) {
		mame_ui_refresh();
	}

	for(i=0;i<total-1;++i) {
		free((char*)menu_item[i]);
	}

	return selected + 1;
}

static int video_pipeline_menu(struct advance_video_context* context, int selected, unsigned input)
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

	for(i=1,stage=video_pipeline_begin(&context->state.blit_pipeline);stage!=video_pipeline_end(&context->state.blit_pipeline);++stage,++i) {
		if (stage == video_pipeline_pivot(&context->state.blit_pipeline)) {
			sprintf(buffer,"(%d) %s\n",i,pipe_name(video_pipeline_vert(&context->state.blit_pipeline)->type));
			++i;
			menu_item[total] = strdup(buffer);
			flag[total] = 0;
			++total;
		}
		sprintf(buffer,"(%d) %s, p %d, dp %d\n",i,pipe_name(stage->type),stage->sbpp,stage->sdp);
		menu_item[total] = strdup(buffer);
		flag[total] = 0;
		++total;
	}
	if (stage == video_pipeline_pivot(&context->state.blit_pipeline)) {
		sprintf(buffer,"(%d) %s\n",i,pipe_name(video_pipeline_vert(&context->state.blit_pipeline)->type));
		++i;
		menu_item[total] = strdup(buffer);
		flag[total] = 0;
		++total;
	}

	menu_item[total] = mame_ui_gettext("Return to Main Menu");
	flag[total] = 0;
	++total;

	menu_item[total] = 0;
	flag[total] = 0;

	mame_ui_menu(menu_item,0,flag,selected,0);

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
		else {
			mame_ui_refresh();
		}
	}

	if (input == OSD_INPUT_CANCEL)
		selected = -1;

	if (input == OSD_INPUT_CONFIGURE)
		selected = -2;

	if (selected == -1 || selected == -2) {
		mame_ui_refresh();
	}

	for(i=0;i<total-1;++i) {
		free((char*)menu_item[i]);
	}

	return selected + 1;
}

int osd2_menu(int selected, unsigned input)
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
	int save_all_index;
	int pipeline_index;
	int magnify_index;
	int scanline_index;
	int rgb_index;
	int depth_index;
	int smp_index;
	char mode_buffer[128];

	struct advance_video_context* context = &CONTEXT.video;

	if (selected >= 1)
		selected = selected - 1;
	else
		selected = 0;

	if (context->state.menu_sub_active)
	{
		int ret = 0;
		switch (context->state.menu_sub_active) {
			case 1 : ret = video_mode_menu(context,context->state.menu_sub_selected,input); break;
			case 2 : ret = video_pipeline_menu(context,context->state.menu_sub_selected,input); break;
		}
		switch (ret) {
			case -1 : return -1; /* hide interface */
			case 0 : context->state.menu_sub_active = 0; context->state.menu_sub_selected = 1; break; /* close submenu */
			default: context->state.menu_sub_selected = ret; break;
		}
		return selected + 1;
	}

	total = 0;

	sprintf(mode_buffer,"%dx%dx%d %.1f/%.1f/%.1f",
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
	if (strcmp(context->config.resolution,"auto")==0)
		menu_subitem[total] = "auto";
	else
		menu_subitem[total] = "custom";
	flag[total] = 0;
	++total;

	if (!context->state.game_vector_flag) {
		if (strcmp(context->config.resolution,"auto")==0) {
			magnify_index = total;
			if (mode_current_magnify(context))
				menu_item[total] = "Magnify [yes]";
			else
				menu_item[total] = "Magnify [no]";
			if (context->config.magnify_flag)
				menu_subitem[total] = "yes";
			else
				menu_subitem[total] = "no";
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
			menu_item[total] = "Magnify";
			menu_subitem[total] = "custom";
			flag[total] = 0;
			++total;

			scanline_index = -1;
			switch (video_scan()) {
				case 0 : menu_item[total] = "Scanline [single]"; break;
				case 1 : menu_item[total] = "Scanline [double]"; break;
				case -1 : menu_item[total] = "Scanline [interlace]"; break;
			}
			menu_subitem[total] = "custom";
			flag[total] = 0;
			++total;
		}

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

		rgb_index = total;
		if (video_index() == VIDEO_FLAGS_INDEX_RGB)
			menu_item[total] = "Rgb [rgb]";
		else
			menu_item[total] = "Rgb [palette]";
		if (context->config.rgb_flag)
			menu_subitem[total] = "yes";
		else
			menu_subitem[total] = "no";
		flag[total] = 0;
		++total;

		depth_index = total;
		switch (video_bits_per_pixel()) {
			case 8 : menu_item[total] = "Depth [8]"; break;
			case 15 : menu_item[total] = "Depth [15]"; break;
			case 16 : menu_item[total] = "Depth [16]"; break;
			case 32 : menu_item[total] = "Depth [32]"; break;
		}
		switch (context->config.depth) {
			case 0 : menu_subitem[total] = "auto"; break;
			case 8 : menu_subitem[total] = "8"; break;
			case 15 : menu_subitem[total] = "15"; break;
			case 16 : menu_subitem[total] = "16"; break;
			case 32 : menu_subitem[total] = "32"; break;
		}
		flag[total] = 0;
		++total;

	} else {
		stretch_index = -1;
		magnify_index = -1;
		rgb_index = -1;
		scanline_index = -1;

		depth_index = total;
		switch (video_bits_per_pixel()) {
			case 8 : menu_item[total] = "Depth [8]"; break;
			case 15 : menu_item[total] = "Depth [15]"; break;
			case 16 : menu_item[total] = "Depth [16]"; break;
			case 32 : menu_item[total] = "Depth [32]"; break;
		}
		switch (context->config.depth) {
			case 0 : menu_subitem[total] = "auto"; break;
			case 8 : menu_subitem[total] = "8"; break;
			case 15 : menu_subitem[total] = "15"; break;
			case 16 : menu_subitem[total] = "16"; break;
			case 32 : menu_subitem[total] = "32"; break;
		}
		flag[total] = 0;
		++total;
	}

	menu_item[total] = "Options";
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	combine_index = total;
	switch (context->state.combine) {
		case COMBINE_NONE : menu_item[total] = "Resize Effect [no]"; break;
		case COMBINE_MAX : menu_item[total] = "Resize Effect [max]"; break;
		case COMBINE_MEAN : menu_item[total] = "Resize Effect [mean]"; break;
		case COMBINE_FILTER : menu_item[total] = "Resize Effect [filter]"; break;
		case COMBINE_FILTERX : menu_item[total] = "Resize Effect [filterx]"; break;
		case COMBINE_FILTERY : menu_item[total] = "Resize Effect [filtery]"; break;
		case COMBINE_SCALE2X : menu_item[total] = "Resize Effect [scale2x]"; break;
	}
	switch (context->config.combine) {
		case COMBINE_AUTO : menu_subitem[total] = "auto"; break;
		case COMBINE_NONE : menu_subitem[total] = "no"; break;
		case COMBINE_MAX : menu_subitem[total] = "max"; break;
		case COMBINE_MEAN : menu_subitem[total] = "mean"; break;
		case COMBINE_FILTER : menu_subitem[total] = "filter"; break;
		case COMBINE_FILTERX : menu_subitem[total] = "filterx"; break;
		case COMBINE_FILTERY : menu_subitem[total] = "filtery"; break;
		case COMBINE_SCALE2X : menu_subitem[total] = "scale2x"; break;
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
	menu_item[total] = mame_ui_gettext("Show Pipeline");
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	save_game_index = total;
	menu_item[total] = mame_ui_gettext("Save for this game");
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	save_resolution_index = total;
	if (context->state.game_vector_flag)
		menu_item[total] = mame_ui_gettext("Save for all vector games");
	else
		menu_item[total] = mame_ui_gettext("Save for this game resolution");
	menu_subitem[total] = 0;
	flag[total] = 0;
	++total;

	if (!context->state.game_vector_flag) {
		save_all_index = total;
		menu_item[total] = mame_ui_gettext("Save for all games");
		menu_subitem[total] = 0;
		flag[total] = 0;
		++total;
	} else {
		save_all_index = -1;
	}

	menu_item[total] = mame_ui_gettext("Return to Main Menu");
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
		|| selected == rgb_index
		|| selected == depth_index
		|| selected == scanline_index
	)
		arrowize = 3;
	else
		arrowize = 0;

	mame_ui_menu(menu_item,menu_subitem,flag,selected,arrowize);

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
			context->state.menu_sub_active = 1;
			mame_ui_refresh();
		} else if (selected == pipeline_index) {
			context->state.menu_sub_active = 2;
			mame_ui_refresh();
		} else if (selected == save_game_index) {
			advance_video_save(context,context->config.section_name);
		} else if (selected == save_resolution_index) {
			advance_video_save(context,context->config.section_resolution);
		} else if (selected == save_all_index) {
			advance_video_save(context,"");
		}
	}

	if (input == OSD_INPUT_RIGHT)
	{
		if (selected == combine_index) {
			switch (context->config.combine) {
				case COMBINE_AUTO : context->config.combine = COMBINE_NONE; break;
				case COMBINE_NONE : context->config.combine = COMBINE_MAX; break;
				case COMBINE_MAX : context->config.combine = COMBINE_MEAN; break;
				case COMBINE_MEAN : context->config.combine = COMBINE_FILTER; break;
				case COMBINE_FILTER : context->config.combine = COMBINE_FILTERX; break;
				case COMBINE_FILTERX : context->config.combine = COMBINE_FILTERY; break;
				case COMBINE_FILTERY : context->config.combine = COMBINE_SCALE2X; break;
				case COMBINE_SCALE2X : context->config.combine = COMBINE_AUTO; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == effect_index) {
			switch (context->config.rgb_effect) {
				case EFFECT_NONE : context->config.rgb_effect = EFFECT_RGB_TRIAD3PIX; break;
				case EFFECT_RGB_TRIAD3PIX : context->config.rgb_effect = EFFECT_RGB_TRIADSTRONG3PIX; break;
				case EFFECT_RGB_TRIADSTRONG3PIX : context->config.rgb_effect = EFFECT_RGB_TRIAD6PIX; break;
				case EFFECT_RGB_TRIAD6PIX : context->config.rgb_effect = EFFECT_RGB_TRIADSTRONG6PIX; break;
				case EFFECT_RGB_TRIADSTRONG6PIX : context->config.rgb_effect = EFFECT_RGB_TRIAD16PIX; break;
				case EFFECT_RGB_TRIAD16PIX : context->config.rgb_effect = EFFECT_RGB_TRIADSTRONG16PIX; break;
				case EFFECT_RGB_TRIADSTRONG16PIX : context->config.rgb_effect = EFFECT_RGB_SCANDOUBLEHORZ; break;
				case EFFECT_RGB_SCANDOUBLEHORZ : context->config.rgb_effect = EFFECT_RGB_SCANTRIPLEHORZ; break;
				case EFFECT_RGB_SCANTRIPLEHORZ : context->config.rgb_effect = EFFECT_RGB_SCANDOUBLEVERT; break;
				case EFFECT_RGB_SCANDOUBLEVERT : context->config.rgb_effect = EFFECT_RGB_SCANTRIPLEVERT; break;
				case EFFECT_RGB_SCANTRIPLEVERT : context->config.rgb_effect = EFFECT_NONE; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == vsync_index) {
			switch (context->config.vsync_flag) {
				case 0 : context->config.vsync_flag = 1; break;
				case 1 : context->config.vsync_flag = 0; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == smp_index) {
			switch (context->config.smp_flag) {
				case 0 : context->config.smp_flag = 1; break;
				case 1 : context->config.smp_flag = 0; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == scanline_index) {
			context->config.scanlines_flag = !context->config.scanlines_flag;
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == magnify_index) {
			context->config.magnify_flag = !context->config.magnify_flag;
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == rgb_index) {
			context->config.rgb_flag = !context->config.rgb_flag;
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == depth_index) {
			switch (context->config.depth) {
				case 0 : context->config.depth = 8; break;
				case 8 : context->config.depth = 15; break;
				case 15 : context->config.depth = 16; break;
				case 16 : context->config.depth = 32; break;
				case 32 : context->config.depth = 0; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == stretch_index) {
			switch (context->config.stretch) {
				case STRETCH_NONE : context->config.stretch = STRETCH_FRACTIONAL_XY; break;
				case STRETCH_INTEGER_XY : context->config.stretch = STRETCH_NONE; break;
				case STRETCH_INTEGER_X_FRACTIONAL_Y : context->config.stretch = STRETCH_INTEGER_XY; break;
				case STRETCH_FRACTIONAL_XY : context->config.stretch = STRETCH_INTEGER_X_FRACTIONAL_Y; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		}
	}

	if (input == OSD_INPUT_LEFT)
	{
		if (selected == combine_index) {
			switch (context->config.combine) {
				case COMBINE_AUTO : context->config.combine = COMBINE_SCALE2X; break;
				case COMBINE_NONE : context->config.combine = COMBINE_AUTO; break;
				case COMBINE_MAX : context->config.combine = COMBINE_NONE; break;
				case COMBINE_MEAN : context->config.combine = COMBINE_MAX; break;
				case COMBINE_FILTER : context->config.combine = COMBINE_MEAN; break;
				case COMBINE_FILTERX : context->config.combine = COMBINE_FILTER; break;
				case COMBINE_FILTERY : context->config.combine = COMBINE_FILTERX; break;
				case COMBINE_SCALE2X : context->config.combine = COMBINE_FILTERY; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == effect_index) {
			switch (context->config.rgb_effect) {
				case EFFECT_NONE : context->config.rgb_effect = EFFECT_RGB_SCANTRIPLEVERT; break;
				case EFFECT_RGB_TRIAD3PIX : context->config.rgb_effect = EFFECT_NONE; break;
				case EFFECT_RGB_TRIADSTRONG3PIX : context->config.rgb_effect = EFFECT_RGB_TRIAD3PIX; break;
				case EFFECT_RGB_TRIAD6PIX : context->config.rgb_effect = EFFECT_RGB_TRIADSTRONG3PIX; break;
				case EFFECT_RGB_TRIADSTRONG6PIX : context->config.rgb_effect = EFFECT_RGB_TRIAD6PIX; break;
				case EFFECT_RGB_TRIAD16PIX : context->config.rgb_effect = EFFECT_RGB_TRIADSTRONG6PIX; break;
				case EFFECT_RGB_TRIADSTRONG16PIX : context->config.rgb_effect = EFFECT_RGB_TRIAD16PIX; break;
				case EFFECT_RGB_SCANDOUBLEHORZ : context->config.rgb_effect = EFFECT_RGB_TRIADSTRONG16PIX; break;
				case EFFECT_RGB_SCANTRIPLEHORZ : context->config.rgb_effect = EFFECT_RGB_SCANDOUBLEHORZ; break;
				case EFFECT_RGB_SCANDOUBLEVERT : context->config.rgb_effect = EFFECT_RGB_SCANTRIPLEHORZ; break;
				case EFFECT_RGB_SCANTRIPLEVERT : context->config.rgb_effect = EFFECT_RGB_SCANDOUBLEVERT; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == vsync_index) {
			switch (context->config.vsync_flag) {
				case 0 : context->config.vsync_flag = 1; break;
				case 1 : context->config.vsync_flag = 0; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == smp_index) {
			switch (context->config.smp_flag) {
				case 0 : context->config.smp_flag = 1; break;
				case 1 : context->config.smp_flag = 0; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == scanline_index) {
			context->config.scanlines_flag = !context->config.scanlines_flag;
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == magnify_index) {
			context->config.magnify_flag = !context->config.magnify_flag;
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == rgb_index) {
			context->config.rgb_flag = !context->config.rgb_flag;
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == depth_index) {
			switch (context->config.depth) {
				case 0 : context->config.depth = 32; break;
				case 8 : context->config.depth = 0; break;
				case 15 : context->config.depth = 8; break;
				case 16 : context->config.depth = 15; break;
				case 32 : context->config.depth = 16; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		} else if (selected == stretch_index) {
			switch (context->config.stretch) {
				case STRETCH_NONE : context->config.stretch = STRETCH_INTEGER_XY; break;
				case STRETCH_INTEGER_XY : context->config.stretch = STRETCH_INTEGER_X_FRACTIONAL_Y; break;
				case STRETCH_INTEGER_X_FRACTIONAL_Y : context->config.stretch = STRETCH_FRACTIONAL_XY; break;
				case STRETCH_FRACTIONAL_XY : context->config.stretch = STRETCH_NONE; break;
			}
			advance_video_change(context);
			mame_ui_refresh();
		}
	}

	if (input == OSD_INPUT_CANCEL)
		selected = -1;

	if (input == OSD_INPUT_CONFIGURE)
		selected = -2;

	if (selected == -1 || selected == -2) {
		mame_ui_refresh();
	}

	return selected + 1;
}

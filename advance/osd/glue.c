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
#include "glue.h"
#include "mame2.h"

#include "hscript.h"

#include <stdio.h>
#include <stdarg.h>

/* Used in os_inline.h */
__extension__ unsigned long long mmx_8to64_map[256] = {
	0x0000000000000000ULL,
	0x0101010101010101ULL,
	0x0202020202020202ULL,
	0x0303030303030303ULL,
	0x0404040404040404ULL,
	0x0505050505050505ULL,
	0x0606060606060606ULL,
	0x0707070707070707ULL,
	0x0808080808080808ULL,
	0x0909090909090909ULL,
	0x0a0a0a0a0a0a0a0aULL,
	0x0b0b0b0b0b0b0b0bULL,
	0x0c0c0c0c0c0c0c0cULL,
	0x0d0d0d0d0d0d0d0dULL,
	0x0e0e0e0e0e0e0e0eULL,
	0x0f0f0f0f0f0f0f0fULL,
	0x1010101010101010ULL,
	0x1111111111111111ULL,
	0x1212121212121212ULL,
	0x1313131313131313ULL,
	0x1414141414141414ULL,
	0x1515151515151515ULL,
	0x1616161616161616ULL,
	0x1717171717171717ULL,
	0x1818181818181818ULL,
	0x1919191919191919ULL,
	0x1a1a1a1a1a1a1a1aULL,
	0x1b1b1b1b1b1b1b1bULL,
	0x1c1c1c1c1c1c1c1cULL,
	0x1d1d1d1d1d1d1d1dULL,
	0x1e1e1e1e1e1e1e1eULL,
	0x1f1f1f1f1f1f1f1fULL,
	0x2020202020202020ULL,
	0x2121212121212121ULL,
	0x2222222222222222ULL,
	0x2323232323232323ULL,
	0x2424242424242424ULL,
	0x2525252525252525ULL,
	0x2626262626262626ULL,
	0x2727272727272727ULL,
	0x2828282828282828ULL,
	0x2929292929292929ULL,
	0x2a2a2a2a2a2a2a2aULL,
	0x2b2b2b2b2b2b2b2bULL,
	0x2c2c2c2c2c2c2c2cULL,
	0x2d2d2d2d2d2d2d2dULL,
	0x2e2e2e2e2e2e2e2eULL,
	0x2f2f2f2f2f2f2f2fULL,
	0x3030303030303030ULL,
	0x3131313131313131ULL,
	0x3232323232323232ULL,
	0x3333333333333333ULL,
	0x3434343434343434ULL,
	0x3535353535353535ULL,
	0x3636363636363636ULL,
	0x3737373737373737ULL,
	0x3838383838383838ULL,
	0x3939393939393939ULL,
	0x3a3a3a3a3a3a3a3aULL,
	0x3b3b3b3b3b3b3b3bULL,
	0x3c3c3c3c3c3c3c3cULL,
	0x3d3d3d3d3d3d3d3dULL,
	0x3e3e3e3e3e3e3e3eULL,
	0x3f3f3f3f3f3f3f3fULL,
	0x4040404040404040ULL,
	0x4141414141414141ULL,
	0x4242424242424242ULL,
	0x4343434343434343ULL,
	0x4444444444444444ULL,
	0x4545454545454545ULL,
	0x4646464646464646ULL,
	0x4747474747474747ULL,
	0x4848484848484848ULL,
	0x4949494949494949ULL,
	0x4a4a4a4a4a4a4a4aULL,
	0x4b4b4b4b4b4b4b4bULL,
	0x4c4c4c4c4c4c4c4cULL,
	0x4d4d4d4d4d4d4d4dULL,
	0x4e4e4e4e4e4e4e4eULL,
	0x4f4f4f4f4f4f4f4fULL,
	0x5050505050505050ULL,
	0x5151515151515151ULL,
	0x5252525252525252ULL,
	0x5353535353535353ULL,
	0x5454545454545454ULL,
	0x5555555555555555ULL,
	0x5656565656565656ULL,
	0x5757575757575757ULL,
	0x5858585858585858ULL,
	0x5959595959595959ULL,
	0x5a5a5a5a5a5a5a5aULL,
	0x5b5b5b5b5b5b5b5bULL,
	0x5c5c5c5c5c5c5c5cULL,
	0x5d5d5d5d5d5d5d5dULL,
	0x5e5e5e5e5e5e5e5eULL,
	0x5f5f5f5f5f5f5f5fULL,
	0x6060606060606060ULL,
	0x6161616161616161ULL,
	0x6262626262626262ULL,
	0x6363636363636363ULL,
	0x6464646464646464ULL,
	0x6565656565656565ULL,
	0x6666666666666666ULL,
	0x6767676767676767ULL,
	0x6868686868686868ULL,
	0x6969696969696969ULL,
	0x6a6a6a6a6a6a6a6aULL,
	0x6b6b6b6b6b6b6b6bULL,
	0x6c6c6c6c6c6c6c6cULL,
	0x6d6d6d6d6d6d6d6dULL,
	0x6e6e6e6e6e6e6e6eULL,
	0x6f6f6f6f6f6f6f6fULL,
	0x7070707070707070ULL,
	0x7171717171717171ULL,
	0x7272727272727272ULL,
	0x7373737373737373ULL,
	0x7474747474747474ULL,
	0x7575757575757575ULL,
	0x7676767676767676ULL,
	0x7777777777777777ULL,
	0x7878787878787878ULL,
	0x7979797979797979ULL,
	0x7a7a7a7a7a7a7a7aULL,
	0x7b7b7b7b7b7b7b7bULL,
	0x7c7c7c7c7c7c7c7cULL,
	0x7d7d7d7d7d7d7d7dULL,
	0x7e7e7e7e7e7e7e7eULL,
	0x7f7f7f7f7f7f7f7fULL,
	0x8080808080808080ULL,
	0x8181818181818181ULL,
	0x8282828282828282ULL,
	0x8383838383838383ULL,
	0x8484848484848484ULL,
	0x8585858585858585ULL,
	0x8686868686868686ULL,
	0x8787878787878787ULL,
	0x8888888888888888ULL,
	0x8989898989898989ULL,
	0x8a8a8a8a8a8a8a8aULL,
	0x8b8b8b8b8b8b8b8bULL,
	0x8c8c8c8c8c8c8c8cULL,
	0x8d8d8d8d8d8d8d8dULL,
	0x8e8e8e8e8e8e8e8eULL,
	0x8f8f8f8f8f8f8f8fULL,
	0x9090909090909090ULL,
	0x9191919191919191ULL,
	0x9292929292929292ULL,
	0x9393939393939393ULL,
	0x9494949494949494ULL,
	0x9595959595959595ULL,
	0x9696969696969696ULL,
	0x9797979797979797ULL,
	0x9898989898989898ULL,
	0x9999999999999999ULL,
	0x9a9a9a9a9a9a9a9aULL,
	0x9b9b9b9b9b9b9b9bULL,
	0x9c9c9c9c9c9c9c9cULL,
	0x9d9d9d9d9d9d9d9dULL,
	0x9e9e9e9e9e9e9e9eULL,
	0x9f9f9f9f9f9f9f9fULL,
	0xa0a0a0a0a0a0a0a0ULL,
	0xa1a1a1a1a1a1a1a1ULL,
	0xa2a2a2a2a2a2a2a2ULL,
	0xa3a3a3a3a3a3a3a3ULL,
	0xa4a4a4a4a4a4a4a4ULL,
	0xa5a5a5a5a5a5a5a5ULL,
	0xa6a6a6a6a6a6a6a6ULL,
	0xa7a7a7a7a7a7a7a7ULL,
	0xa8a8a8a8a8a8a8a8ULL,
	0xa9a9a9a9a9a9a9a9ULL,
	0xaaaaaaaaaaaaaaaaULL,
	0xababababababababULL,
	0xacacacacacacacacULL,
	0xadadadadadadadadULL,
	0xaeaeaeaeaeaeaeaeULL,
	0xafafafafafafafafULL,
	0xb0b0b0b0b0b0b0b0ULL,
	0xb1b1b1b1b1b1b1b1ULL,
	0xb2b2b2b2b2b2b2b2ULL,
	0xb3b3b3b3b3b3b3b3ULL,
	0xb4b4b4b4b4b4b4b4ULL,
	0xb5b5b5b5b5b5b5b5ULL,
	0xb6b6b6b6b6b6b6b6ULL,
	0xb7b7b7b7b7b7b7b7ULL,
	0xb8b8b8b8b8b8b8b8ULL,
	0xb9b9b9b9b9b9b9b9ULL,
	0xbabababababababaULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xbcbcbcbcbcbcbcbcULL,
	0xbdbdbdbdbdbdbdbdULL,
	0xbebebebebebebebeULL,
	0xbfbfbfbfbfbfbfbfULL,
	0xc0c0c0c0c0c0c0c0ULL,
	0xc1c1c1c1c1c1c1c1ULL,
	0xc2c2c2c2c2c2c2c2ULL,
	0xc3c3c3c3c3c3c3c3ULL,
	0xc4c4c4c4c4c4c4c4ULL,
	0xc5c5c5c5c5c5c5c5ULL,
	0xc6c6c6c6c6c6c6c6ULL,
	0xc7c7c7c7c7c7c7c7ULL,
	0xc8c8c8c8c8c8c8c8ULL,
	0xc9c9c9c9c9c9c9c9ULL,
	0xcacacacacacacacaULL,
	0xcbcbcbcbcbcbcbcbULL,
	0xccccccccccccccccULL,
	0xcdcdcdcdcdcdcdcdULL,
	0xcecececececececeULL,
	0xcfcfcfcfcfcfcfcfULL,
	0xd0d0d0d0d0d0d0d0ULL,
	0xd1d1d1d1d1d1d1d1ULL,
	0xd2d2d2d2d2d2d2d2ULL,
	0xd3d3d3d3d3d3d3d3ULL,
	0xd4d4d4d4d4d4d4d4ULL,
	0xd5d5d5d5d5d5d5d5ULL,
	0xd6d6d6d6d6d6d6d6ULL,
	0xd7d7d7d7d7d7d7d7ULL,
	0xd8d8d8d8d8d8d8d8ULL,
	0xd9d9d9d9d9d9d9d9ULL,
	0xdadadadadadadadaULL,
	0xdbdbdbdbdbdbdbdbULL,
	0xdcdcdcdcdcdcdcdcULL,
	0xddddddddddddddddULL,
	0xdedededededededeULL,
	0xdfdfdfdfdfdfdfdfULL,
	0xe0e0e0e0e0e0e0e0ULL,
	0xe1e1e1e1e1e1e1e1ULL,
	0xe2e2e2e2e2e2e2e2ULL,
	0xe3e3e3e3e3e3e3e3ULL,
	0xe4e4e4e4e4e4e4e4ULL,
	0xe5e5e5e5e5e5e5e5ULL,
	0xe6e6e6e6e6e6e6e6ULL,
	0xe7e7e7e7e7e7e7e7ULL,
	0xe8e8e8e8e8e8e8e8ULL,
	0xe9e9e9e9e9e9e9e9ULL,
	0xeaeaeaeaeaeaeaeaULL,
	0xebebebebebebebebULL,
	0xececececececececULL,
	0xededededededededULL,
	0xeeeeeeeeeeeeeeeeULL,
	0xefefefefefefefefULL,
	0xf0f0f0f0f0f0f0f0ULL,
	0xf1f1f1f1f1f1f1f1ULL,
	0xf2f2f2f2f2f2f2f2ULL,
	0xf3f3f3f3f3f3f3f3ULL,
	0xf4f4f4f4f4f4f4f4ULL,
	0xf5f5f5f5f5f5f5f5ULL,
	0xf6f6f6f6f6f6f6f6ULL,
	0xf7f7f7f7f7f7f7f7ULL,
	0xf8f8f8f8f8f8f8f8ULL,
	0xf9f9f9f9f9f9f9f9ULL,
	0xfafafafafafafafaULL,
	0xfbfbfbfbfbfbfbfbULL,
	0xfcfcfcfcfcfcfcfcULL,
	0xfdfdfdfdfdfdfdfdULL,
	0xfefefefefefefefeULL,
	0xffffffffffffffffULL
};

struct advance_glue_context {
	struct mame_bitmap* bitmap;
	struct mame_bitmap* bitmap_alt;
	struct osd_video_option option;

	int video_flag; /** If the video initialization completed with success. */

	int sound_flag; /**< Sound main active flag. */
	double sound_step; /**< Number of sound samples for a single frame. This is the ideal value. */
	unsigned sound_last_count; /** Number of sound samples for the last frame. */
	int sound_latency; /**< Current samples in excess. Updated at every frame. */

	short* sound_silence_buffer; /**< Buffer filled of silence. */
	unsigned sound_silence_count; /**< Number of samples for the silence buffer. */

	const short* sound_current_buffer; /**< Last buffer of samples to play. */
	unsigned sound_current_count; /**< Number of samples in the last buffer. */

	char info_buffer[1024]; /**< Buffer for the reported display info. */

#ifdef MESS
	char crc_file[MAME_MAXPATH]; /**< Storage for the the crcfile MESS pointer. */
	char parent_crc_file[MAME_MAXPATH]; /**< Storage for the the pcrcfile MESS pointer. */
#endif
};

static struct advance_glue_context GLUE;

/***************************************************************************/
/* MAME */

/* internals */
extern char* cheatfile;
extern char* history_filename;
extern char* mameinfo_filename;
#ifdef MESS
const char* crcfile;
const char* pcrcfile;
#endif

void logerror(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	log_va(text,arg);
	va_end(arg);
}

const char* mame_game_resolution(const mame_game* game) {
	static char resolution[32];
	const struct GameDriver* driver = (const struct GameDriver*)game;
	struct InternalMachineDriver machine;
	expand_machine_driver(driver->drv, &machine);
	if ((machine.video_attributes & VIDEO_TYPE_VECTOR) == 0) {
		unsigned dx;
		unsigned dy;
		if (driver->flags & ORIENTATION_SWAP_XY) {
			dx = machine.default_visible_area.max_y - machine.default_visible_area.min_y + 1;
			dy = machine.default_visible_area.max_x - machine.default_visible_area.min_x + 1;
		} else {
			dx = machine.default_visible_area.max_x - machine.default_visible_area.min_x + 1;
			dy = machine.default_visible_area.max_y - machine.default_visible_area.min_y + 1;
		}
		sprintf(resolution,"%dx%d",dx,dy);
	} else {
		strcpy(resolution,"vector");
	}
	return resolution;
}

double mame_game_fps(const mame_game* game) {
	const struct GameDriver* driver = (const struct GameDriver*)game;
	struct InternalMachineDriver machine;
	expand_machine_driver(driver->drv, &machine);
	return machine.frames_per_second;
}

unsigned mame_game_orientation(const mame_game* game) {
	const struct GameDriver* driver = (const struct GameDriver*)game;
	unsigned orientation = 0;

	if ((driver->flags & ORIENTATION_SWAP_XY) != 0)
		orientation |= OSD_ORIENTATION_SWAP_XY;
	if ((driver->flags & ORIENTATION_FLIP_X) != 0)
		orientation |= OSD_ORIENTATION_FLIP_X;
	if ((driver->flags & ORIENTATION_FLIP_Y) != 0)
		orientation |= OSD_ORIENTATION_FLIP_Y;

	return orientation;
}

const char* mame_game_name(const mame_game* game) {
	const struct GameDriver* driver = (const struct GameDriver*)game;
	return driver->name;
}

const char* mame_game_description(const mame_game* game) {
	const struct GameDriver* driver = (const struct GameDriver*)game;
	return driver->description;
}

const mame_game* mame_game_at(unsigned i) {
	return (const mame_game*)drivers[i];
}

void mame_print_info(FILE* out) {
	print_mame_info( out, drivers );
}

int mame_is_game_vector(const mame_game* game) {
	const struct GameDriver* driver = (const struct GameDriver*)game;
	struct InternalMachineDriver machine;
	expand_machine_driver(driver->drv, &machine);
	return (machine.video_attributes & VIDEO_TYPE_VECTOR) != 0;
}

int mame_is_game_in_list(const char* list[], const mame_game* game) {
	const struct GameDriver* driver = (const struct GameDriver*)game;
	while (driver && driver->name) {
		unsigned i;
		for(i=0;list[i];++i) {
			if (strcmp(driver->name,list[i])==0)
				return 1;
		}
		driver = driver->clone_of;
	}
	return 0;
}

#ifdef MESS
static int mess_log(char *fmt, va_list arg) {
	os_msg_va(fmt,arg);
	return 0;
}
#endif

int mame_game_run(struct advance_context* context, const struct mame_option* advance) {
	int r;

	unsigned i;
	const struct GameDriver* driver = (const struct GameDriver*)advance->game;
	for(i=0;drivers[i];++i)
		if (driver == drivers[i])
			break;

	/* store the game pointer */
	context->game = advance->game;

	options.record = 0;
	options.playback = 0;
	options.mame_debug = advance->debug_flag;
	options.cheat = advance->cheat_flag;
	options.gui_host = 1; /* this prevents text mode messages that may stop the execution */
	options.samplerate = advance->samplerate;
	options.use_samples = advance->samples_flag;
	options.use_filter = advance->filter_flag;
	options.color_depth = advance->color_depth;
	options.vector_width = advance->vector_width;
	options.vector_height = advance->vector_height;
	options.debug_width = advance->debug_width;
	options.debug_height = advance->debug_height;
	options.debug_depth = 8;
	options.norotate = advance->norotate;
	options.ui_orientation = advance->ui_orientation;
	options.ror = advance->ror;
	options.rol = advance->rol;
	options.flipx = advance->flipx;
	options.flipy = advance->flipy;
	options.antialias = advance->antialias;
	options.translucency = advance->translucency;
	options.beam = advance->beam;
	options.vector_flicker = advance->vector_flicker;
	options.vector_intensity = advance->vector_intensity;
	options.brightness = advance->brightness;
	options.gamma = advance->gamma;
	options.use_artwork = advance->artwork_flag ? ARTWORK_USE_ALL : ARTWORK_USE_NONE; /* or all or nothing */
	options.artwork_res = 1; /* no artwork scaling */
	options.artwork_crop = advance->artwork_crop_flag;

	options.savegame = advance->savegame;

	options.language_file = osd_fopen(0,advance->language_file,OSD_FILETYPE_LANGUAGE,0);
	cheatfile = (char*)advance->cheat_file;
	history_filename = (char*)advance->history_file;
	mameinfo_filename = (char*)advance->info_file;

#ifdef MESS
	sprintf(GLUE.crc_file,"%s%c%s.crc",advance->crc_dir,os_dir_slash(),driver->name);
        crcfile = GLUE.crc_file;

	os_log(("mess: file_crc %s\n", crcfile));

	if (driver->clone_of
		&& driver->clone_of->name
		&& (driver->clone_of->flags & NOT_A_DRIVER) == 0) {
		sprintf(GLUE.parent_crc_file,"%s%c%s.crc",advance->crc_dir,os_dir_slash(),driver->clone_of->name);
	} else {
		strcpy(GLUE.parent_crc_file,"");
	}
	pcrcfile = GLUE.parent_crc_file;

	os_log(("mess: parent_file_crc %s\n", pcrcfile));

	options.mess_printf_output = mess_log;
#endif

	r = run_game(i);

	if (options.language_file)
		osd_fclose(options.language_file);

	return r;
}

/***************************************************************************/
/* MAME user interface */

void mame_ui_area_set(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
	set_ui_visarea(x1,y1,x2,y2);
}

void mame_ui_message(const char* s, ...) {
	va_list arg;
	char buffer[256];
	va_start(arg,s);
	vsprintf(buffer,s,arg);
	usrintf_showmessage(buffer);
	va_end(arg);
}

void mame_ui_menu(const char** items, const char** subitems, char* flag, int selected, int arrowize_subitem) {
	ui_displaymenu(GLUE.bitmap,items,subitems,flag,selected,arrowize_subitem);
}

void mame_ui_save_snapshot(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
	struct rectangle rec;

	rec.min_x = x1;
	rec.min_y = y1;
	rec.max_x = x2;
	rec.max_y = y2;

	save_screen_snapshot(GLUE.bitmap, &rec);
}

const char* mame_ui_gettext(const char* text) {
	return text;
}

void mame_ui_refresh(void) {
	schedule_full_refresh();
}

void mame_ui_swap(void) {
	struct mame_bitmap tmp;

	if (!Machine->scrbitmap)
		return;

	if (!GLUE.bitmap_alt) {
		/* allocate the alternate bitmap */
		int width = Machine->scrbitmap->width;
		int height = Machine->scrbitmap->height;
		int depth = Machine->scrbitmap->depth;

		if (Machine->orientation & ORIENTATION_SWAP_XY) {
			int temp = width; width = height; height = temp;
		}

		GLUE.bitmap_alt = bitmap_alloc_depth(width,height,depth);
	}

	/* swap the contents of the two bitmap */
	memcpy(&tmp,Machine->scrbitmap,sizeof(struct mame_bitmap));
	memcpy(Machine->scrbitmap,GLUE.bitmap_alt,sizeof(struct mame_bitmap));
	memcpy(GLUE.bitmap_alt,&tmp,sizeof(struct mame_bitmap));

	/* redraw all */
	schedule_full_refresh();
}

void mame_ui_gamma_factor_set(double gamma) {
	palette_set_global_gamma(palette_get_global_gamma() * gamma);
}

void mame_ui_show_info_temp(void) {
	ui_show_fps_temp(2);
}

/***************************************************************************/
/* OSD */

int osd_init(void)
{
	return 0;
}

void osd_exit(void)
{
}

/***************************************************************************/
/* Display */

int osd_create_display(const struct osd_create_params *params, UINT32 *rgb_components)
{
	unsigned width;
	unsigned height;
	unsigned aspect_x;
	unsigned aspect_y;

	log_std(("osd: osd_create_display(width:%d, height:%d, aspect_x:%d, aspect_y:%d, depth:%d, colors:%d, fps:%g, attributes:%d, orientation:%d)\n", params->width, params->height, params->aspect_x, params->aspect_y, params->depth, params->colors, (double)params->fps, params->video_attributes, params->orientation));

	width = params->width;
	height = params->height;
	aspect_x = params->aspect_x;
	aspect_y = params->aspect_y;

	/* set the correct orientation of the aspect */
	if (params->orientation & ORIENTATION_SWAP_XY) {
		SWAP(unsigned, aspect_x, aspect_y);
		SWAP(unsigned, width, height);
	}

	GLUE.video_flag = 0; /* the video isn't initialized */

	GLUE.option.vector_flag = (params->video_attributes & VIDEO_TYPE_VECTOR) != 0;

	GLUE.option.aspect_x = aspect_x;
	GLUE.option.aspect_y = aspect_y;

	GLUE.option.bits_per_pixel = params->depth;
	GLUE.option.fps = params->fps;

	if (GLUE.option.bits_per_pixel == 8 || GLUE.option.bits_per_pixel == 16) {
		GLUE.option.rgb_def = 0;
		GLUE.option.rgb_flag = 0;
	} else if (GLUE.option.bits_per_pixel == 15) {
		GLUE.option.rgb_def = video_rgb_def_make(5,10,5,5,5,0);
		GLUE.option.rgb_flag = 1;
	} else if (GLUE.option.bits_per_pixel == 32) {
		GLUE.option.rgb_def = video_rgb_def_make(8,16,8,8,8,0);
		GLUE.option.rgb_flag = 1;
	} else
		return -1;

	GLUE.option.area_size_x = width;
	GLUE.option.area_size_y = height;
	GLUE.option.used_pos_x = 0;
	GLUE.option.used_pos_y = 0;
	GLUE.option.used_size_x = width;
	GLUE.option.used_size_y = height;

	if (GLUE.option.rgb_flag)
		GLUE.option.colors = 0;
	else {
		GLUE.option.colors = params->colors;
	}
	GLUE.option.rgb_components = rgb_components;

	if (osd2_video_init(&GLUE.option) != 0)
		return -1;

	GLUE.video_flag = 1; /* the video is initialized */

	return 0;
}

void osd_close_display(void) {
	log_std(("osd: osd_close_display()\n"));

	if (GLUE.video_flag)
		osd2_video_done();
}

int osd_menu(struct mame_bitmap *bitmap, int selected) {
	unsigned input;

	log_pedantic(("osd: osd_menu(%d)\n",selected));

	/* save the bitmap */
	GLUE.bitmap = bitmap;

	/* update the input */
	input = 0;
	/* one shot input */
	if (input_ui_pressed(IPT_UI_SELECT))
		input |= OSD_INPUT_SELECT;
	if (input_ui_pressed(IPT_UI_CANCEL))
		input |= OSD_INPUT_CANCEL;
	if (input_ui_pressed(IPT_UI_CONFIGURE))
		input |= OSD_INPUT_CONFIGURE;
	/* continous input */
	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		input |= OSD_INPUT_UP;
	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		input |= OSD_INPUT_DOWN;
	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
		input |= OSD_INPUT_LEFT;
	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
		input |= OSD_INPUT_RIGHT;

	return osd2_menu(selected,input);
}

/**
 * Compute the sound samples required for the next frame.
 */
static unsigned glue_sound_sample(void) {

	int samples = GLUE.sound_step - GLUE.sound_latency;

	/* Correction for a generic sound buffer underflow. */
	/* Generally happen that the DMA buffer underflow reporting */
	/* a very full state instead of an empty one. */
	if (samples < 16) {
		log_std(("WARNING: too less sound samples %d adjusted to 16\n", samples));
		samples = 16;
	}

	GLUE.sound_last_count = samples;

	return GLUE.sound_last_count;
}

void osd_update_video_and_audio(struct mame_display *display) {
	struct osd_bitmap game;
	struct osd_bitmap debug;
	struct osd_bitmap* pgame;
	struct osd_bitmap* pdebug;
	unsigned input;
	const short* sample_buffer;
	unsigned sample_count;

	profiler_mark(PROFILER_BLIT);

	/* save the bitmap */
	GLUE.bitmap = display->game_bitmap;

	/* update the bitmap */
	if (display->game_bitmap) {
		pgame = &game;
		game.size_x = display->game_bitmap->width;
		game.size_y = display->game_bitmap->height;
		game.ptr = display->game_bitmap->base;
		game.bytes_per_scanline = display->game_bitmap->rowbytes;
	} else {
		pgame = 0;
	}
	if (display->debug_bitmap) {
		pdebug = &debug;
		debug.size_x = display->debug_bitmap->width;
		debug.size_y = display->debug_bitmap->height;
		debug.ptr = display->debug_bitmap->base;
		debug.bytes_per_scanline = display->debug_bitmap->rowbytes;
	} else {
		pdebug = 0;
	}

	if ((display->changed_flags & DEBUG_FOCUS_CHANGED) != 0) {
		osd2_debugger_focus(display->debug_focus);
	}

	/* update the palette */
	if ((display->changed_flags & GAME_PALETTE_CHANGED) != 0) {
		osd2_palette(display->game_palette_dirty, display->game_palette, display->game_palette_entries);
	}

	/* update the area */
	if ((display->changed_flags & GAME_VISIBLE_AREA_CHANGED) != 0) {
		osd2_area(display->game_visible_area.min_x, display->game_visible_area.min_y, display->game_visible_area.max_x, display->game_visible_area.max_y);
	}

	/* update the input */
	input = 0;
	/* one shot input */
	if (input_ui_pressed(IPT_UI_THROTTLE))
		input |= OSD_INPUT_THROTTLE;
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
		input |= OSD_INPUT_FRAMESKIP_DEC;
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
		input |= OSD_INPUT_FRAMESKIP_INC;
	if (input_ui_pressed(IPT_UI_TOGGLE_DEBUG))
		input |= OSD_INPUT_TOGGLE_DEBUG;
	if (input_ui_pressed(IPT_UI_MODE_PRED))
		input |= OSD_INPUT_MODE_PRED;
	if (input_ui_pressed(IPT_UI_MODE_NEXT))
		input |= OSD_INPUT_MODE_NEXT;
	/* continous input */
	if (seq_pressed(input_port_type_seq(IPT_UI_PAN_RIGHT)))
		input |= OSD_INPUT_PAN_RIGHT;
	if (seq_pressed(input_port_type_seq(IPT_UI_PAN_LEFT)))
		input |= OSD_INPUT_PAN_LEFT;
	if (seq_pressed(input_port_type_seq(IPT_UI_PAN_UP)))
		input |= OSD_INPUT_PAN_UP;
	if (seq_pressed(input_port_type_seq(IPT_UI_PAN_DOWN)))
		input |= OSD_INPUT_PAN_DOWN;
	if (seq_pressed(input_port_type_seq(IPT_UI_TURBO)))
		input |= OSD_INPUT_TURBO;
	if (seq_pressed(input_port_type_seq(IPT_COIN1)))
		input |= OSD_INPUT_COIN1;
	if (seq_pressed(input_port_type_seq(IPT_COIN2)))
		input |= OSD_INPUT_COIN2;
	if (seq_pressed(input_port_type_seq(IPT_COIN3)))
		input |= OSD_INPUT_COIN3;
	if (seq_pressed(input_port_type_seq(IPT_COIN4)))
		input |= OSD_INPUT_COIN4;
	if (seq_pressed(input_port_type_seq(IPT_START1)))
		input |= OSD_INPUT_START1;
	if (seq_pressed(input_port_type_seq(IPT_START2)))
		input |= OSD_INPUT_START2;
	if (seq_pressed(input_port_type_seq(IPT_START3)))
		input |= OSD_INPUT_START3;
	if (seq_pressed(input_port_type_seq(IPT_START4)))
		input |= OSD_INPUT_START4;

	/* update the sound */
	if (GLUE.sound_flag) {
		/* select the buffer to play */
		if (GLUE.sound_current_buffer) {
			/* if the buffer is available use it */

			sample_buffer = GLUE.sound_current_buffer;
			sample_count = GLUE.sound_current_count;

			GLUE.sound_current_buffer = 0;
			GLUE.sound_current_count = 0;
		} else {
			/* if no sound generated use the silence buffer */
			sample_count = glue_sound_sample();
			if (sample_count > GLUE.sound_silence_count) {
				log_std(("ERROR: silence underflow! %d %d\n", sample_count, GLUE.sound_silence_count));
				sample_count = GLUE.sound_silence_count;
			}

			sample_buffer = GLUE.sound_silence_buffer;
		}
	} else {
		/* no sound to play */
		sample_buffer = 0;
		sample_count = 0;
	}

	GLUE.sound_latency = osd2_frame(
		pgame,
		pdebug,
		display->debug_palette,
		display->debug_palette_entries,
		display->led_state,
		input,
		sample_buffer,
		sample_count
	);

	profiler_mark(PROFILER_END);
}

int osd_start_audio_stream(int stereo) {
	unsigned rate = Machine->sample_rate;

	log_std(("osd: osd_start_audio_stream(sample_rate:%d, stereo_flag:%d)\n", rate, stereo));

	assert( GLUE.sound_flag == 0 );

	if (osd2_sound_init(&rate, stereo) != 0) {
		log_std(("osd: osd_start_audio_stream return no sound. Disable MAME sound generation.\n"));

		/* disable the MAME sound generation */
		Machine->sample_rate = 0;
		return 0;
	}

	log_std(("osd: osd_start_audio_stream return %d rate\n", rate));

	/* adjust the rate */
	Machine->sample_rate = rate;

	GLUE.sound_flag = 1;
	GLUE.sound_step = rate / (double)Machine->drv->frames_per_second;
	GLUE.sound_latency = 0;

	GLUE.sound_silence_count = 2 * GLUE.sound_step; /* double size for safety */
	if (stereo) {
		GLUE.sound_silence_buffer = (short*)malloc( 4 * GLUE.sound_silence_count );
		memset(GLUE.sound_silence_buffer, 0, 4 * GLUE.sound_silence_count);
	} else {
		GLUE.sound_silence_buffer = (short*)malloc( 2 * GLUE.sound_silence_count );
		memset(GLUE.sound_silence_buffer, 0, 2 * GLUE.sound_silence_count);
	}

	return glue_sound_sample();
}

void osd_stop_audio_stream(void) {
	log_std(("osd: osd_stop_audio_stream()\n"));

	if (GLUE.sound_flag) {
		free(GLUE.sound_silence_buffer);
		osd2_sound_done();

		GLUE.sound_flag = 0;
	}
}

int osd_update_audio_stream(short* buffer) {
	log_debug(("osd: osd_update_audio_stream()\n"));

	if (GLUE.sound_flag) {

		/* save the buffer pointer */
		GLUE.sound_current_buffer = buffer;
		GLUE.sound_current_count = GLUE.sound_last_count;

		/* return the next number of samples required */
		return glue_sound_sample();
	} else {
		return 0;
	}
}

void osd_save_snapshot(struct mame_bitmap *bitmap, const struct rectangle *bounds) {
	/* save the bitmap */
	GLUE.bitmap = bitmap;

	osd2_save_snapshot(bounds->min_x, bounds->min_y, bounds->max_x, bounds->max_y);
}

cycles_t osd_cycles(void) {
	return os_clock();
}

cycles_t osd_cycles_per_second(void) {
	return OS_CLOCKS_PER_SEC;
}

/* Filter the user interface input state */
int osd_input_ui_filter(int result, int type) {
	return result || hardware_is_input_simulated(SIMULATE_EVENT,type);
}

/* Filter the main exit request */
int osd_input_exit_filter(int result) {
	return advance_input_exit_filter(&CONTEXT.input,result);
}

/* Filter the input port state */
int osd_input_port_filter(int result, int type) {
	return result || hardware_is_input_simulated(SIMULATE_EVENT,type);
}

const char *osd_get_fps_text(const struct performance_info *performance)
{
	osd2_info(GLUE.info_buffer, sizeof(GLUE.info_buffer) - 1);

	/* for vector games, add the number of vector updates */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		sprintf(GLUE.info_buffer + strlen(GLUE.info_buffer), "\n%4d vector updates", performance->vector_updates_last_second);
	}
	else if (performance->partial_updates_this_frame > 1)
	{
		sprintf(GLUE.info_buffer + strlen(GLUE.info_buffer), "\n%4d partial updates", performance->partial_updates_this_frame);
	}
	
	return GLUE.info_buffer;
}

/***************************************************************************/
/* Config */

static struct conf_enum_int OPTION_DEPTH[] = {
{ "auto", 0 },
{ "8", 8 },
{ "15", 15 },
{ "16", 16 },
{ "32", 32 }
};

#ifdef MESS
extern const struct Devices devices[]; /* from mess device.c */

static void mess_init(struct conf_context* context) {
	const struct Devices* i;

	options.image_count = 0;

	i = devices;
	while (i->id != IO_COUNT) {
		char buffer[256];

		sprintf(buffer,"dev_%s",i->name);

		conf_string_register_multi(context, buffer);

		++i;
	}
}

static void mess_config_load(struct conf_context* context) {
	const struct Devices* i;

	i = devices;
	while (i->id != IO_COUNT) {
		conf_iterator j;
		char buffer[256];

		sprintf(buffer,"dev_%s",i->name);

		conf_iterator_begin(&j, context, buffer);
		while (!conf_iterator_is_end(&j)) {
			const char* arg = conf_iterator_string_get(&j);

			log_std(("mess: register device %s %s\n",i->name,arg));

			register_device(i->id,arg);

			conf_iterator_next(&j);
		}

		++i;
	}
}

static void mess_done(void) {
}

#endif

int mame_init(struct advance_context* context, struct conf_context* cfg_context) {

	/* Clear the MAIN context */
	memset(context, 0, sizeof(struct advance_context));

	/* Clear the GLUE context */
	memset(&GLUE, 0, sizeof(GLUE));

	/* Clear the MAME global struct */
	memset(&options,0,sizeof(options));

	conf_bool_register_default(cfg_context, "display_artwork", 1);
	conf_bool_register_default(cfg_context, "display_artwork_crop", 0);
	conf_bool_register_default(cfg_context, "sound_samples", 1);

	conf_bool_register_default(cfg_context, "display_antialias", 1);
	conf_bool_register_default(cfg_context, "display_translucency", 1);
	conf_float_register_limit_default(cfg_context, "display_beam", 1.0, 16.0, 1.0);
	conf_float_register_limit_default(cfg_context, "display_flicker", 0.0, 100.0, 0.0);
	conf_float_register_limit_default(cfg_context, "display_intensity", 0.5, 3.0, 1.5);

	conf_float_register_limit_default(cfg_context, "display_gamma", 0.5, 2.0, 1.0);
	conf_float_register_limit_default(cfg_context, "display_brightness", 0.1, 10.0, 1.0);
	conf_int_register_enum_default(cfg_context, "misc_internaldepth", conf_enum(OPTION_DEPTH), 0);

	conf_bool_register_default(cfg_context, "misc_cheat", 0);
	conf_bool_register_default(cfg_context, "misc_quiet", 0);
	conf_string_register_default(cfg_context, "misc_languagefile", "english.lng");
	conf_string_register_default(cfg_context, "misc_cheatfile", "cheat.dat" );
	conf_string_register_default(cfg_context, "misc_historyfile", "history.dat");

#ifdef MESS
	conf_string_register_default(cfg_context, "misc_infofile", "sysinfo.dat");
#else
	conf_string_register_default(cfg_context, "misc_infofile", "mameinfo.dat");
#endif

#ifdef MESS
	mess_init(cfg_context);
#endif

	return 0;
}

void mame_done(struct advance_context* context) {
#ifdef MESS
	mess_done();
#endif
}

int mame_config_load(struct conf_context* cfg_context, struct mame_option* option) {
	char* s;

	option->artwork_flag = conf_bool_get_default(cfg_context, "display_artwork");
	option->artwork_crop_flag = conf_bool_get_default(cfg_context, "display_artwork_crop");
	option->samples_flag = conf_bool_get_default(cfg_context, "sound_samples");

	option->antialias = conf_bool_get_default(cfg_context, "display_antialias");
	option->translucency = conf_bool_get_default(cfg_context, "display_translucency");
	option->beam = (int)(0x00010000 * conf_float_get_default(cfg_context, "display_beam"));
	if (option->beam < 0x00010000)
		option->beam = 0x00010000;
	if (option->beam > 0x00100000)
		option->beam = 0x00100000;
	option->vector_intensity = conf_float_get_default(cfg_context, "display_intensity");
	option->vector_flicker = (int)(2.55 * conf_float_get_default(cfg_context, "display_flicker"));
	if (option->vector_flicker < 0)
		options.vector_flicker = 0;
	if (option->vector_flicker > 255)
		options.vector_flicker = 255;

	option->gamma = conf_float_get_default(cfg_context, "display_gamma");
	option->brightness = conf_float_get_default(cfg_context, "display_brightness");

	option->color_depth = conf_int_get_default(cfg_context, "misc_internaldepth");

	option->cheat_flag = conf_bool_get_default(cfg_context, "misc_cheat");
	option->quiet_flag = conf_bool_get_default(cfg_context, "misc_quiet");

	strcpy(option->language_file, conf_string_get_default(cfg_context, "misc_languagefile"));

	strcpy(option->cheat_file, conf_string_get_default(cfg_context, "misc_cheatfile"));

	/* convert the dir separator char to ';'. */
	/* the cheat system use always this char in all the operating system */
	for(s=option->cheat_file;*s;++s)
		if (*s == file_dir_separator())
			*s = ';';

	strcpy(option->history_file, conf_string_get_default(cfg_context,"misc_historyfile"));
	strcpy(option->info_file, conf_string_get_default(cfg_context,"misc_infofile"));

#ifdef MESS
	mess_config_load(cfg_context);
#endif

	return 0;
}

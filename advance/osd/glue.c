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

#include "emu.h"
#include "glue.h"
#include "mame2.h"
#include "log.h"
#include "target.h"
#include "file.h"
#include "hscript.h"
#include "glue.h"

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

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
	char crc_file_buffer[MAME_MAXPATH]; /**< Storage for the the crcfile MESS pointer. */
	char parent_crc_file_buffer[MAME_MAXPATH]; /**< Storage for the the pcrcfile MESS pointer. */
#endif

	char resolution_buffer[32]; /**< Buffer used by mame_resolution(). */
	char resolutionclock_buffer[32]; /**< Buffer used by mame_resolutionclock(). */
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

void logerror(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	log_va(text, arg);
	va_end(arg);
}

const char* mame_game_resolution(const mame_game* game)
{
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
		snprintf(GLUE.resolution_buffer, sizeof(GLUE.resolution_buffer), "%dx%d", dx, dy);
	} else {
		snprintf(GLUE.resolution_buffer, sizeof(GLUE.resolution_buffer), "%s", "vector");
	}
	return GLUE.resolution_buffer;
}

const char* mame_game_resolutionclock(const mame_game* game)
{
	const struct GameDriver* driver = (const struct GameDriver*)game;
	struct InternalMachineDriver machine;
	expand_machine_driver(driver->drv, &machine);
	if ((machine.video_attributes & VIDEO_TYPE_VECTOR) == 0) {
		unsigned dx;
		unsigned dy;
		unsigned clock;
		if (driver->flags & ORIENTATION_SWAP_XY) {
			dx = machine.default_visible_area.max_y - machine.default_visible_area.min_y + 1;
			dy = machine.default_visible_area.max_x - machine.default_visible_area.min_x + 1;
		} else {
			dx = machine.default_visible_area.max_x - machine.default_visible_area.min_x + 1;
			dy = machine.default_visible_area.max_y - machine.default_visible_area.min_y + 1;
		}
		clock = floor(machine.frames_per_second);
		snprintf(GLUE.resolutionclock_buffer, sizeof(GLUE.resolutionclock_buffer), "%dx%dx%d", dx, dy, clock);
	} else {
		snprintf(GLUE.resolutionclock_buffer, sizeof(GLUE.resolutionclock_buffer), "%s", "vector");
	}
	return GLUE.resolutionclock_buffer;
}

double mame_game_fps(const mame_game* game)
{
	const struct GameDriver* driver = (const struct GameDriver*)game;
	struct InternalMachineDriver machine;
	expand_machine_driver(driver->drv, &machine);
	return machine.frames_per_second;
}

unsigned mame_game_orientation(const mame_game* game)
{
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

const char* mame_game_name(const mame_game* game)
{
	const struct GameDriver* driver = (const struct GameDriver*)game;
	return driver->name;
}

const char* mame_game_description(const mame_game* game)
{
	const struct GameDriver* driver = (const struct GameDriver*)game;
	return driver->description;
}

const mame_game* mame_game_at(unsigned i)
{
	return (const mame_game*)drivers[i];
}

void mame_print_info(FILE* out)
{
	print_mame_info(out, drivers);
}

void mame_print_xml(FILE* out)
{
	print_mame_xml(out, drivers);
}

int mame_is_game_vector(const mame_game* game)
{
	const struct GameDriver* driver = (const struct GameDriver*)game;
	struct InternalMachineDriver machine;
	expand_machine_driver(driver->drv, &machine);
	return (machine.video_attributes & VIDEO_TYPE_VECTOR) != 0;
}

int mame_is_game_in_list(const char* list[], const mame_game* game)
{
	const struct GameDriver* driver = (const struct GameDriver*)game;
	while (driver && driver->name) {
		unsigned i;
		for(i=0;list[i];++i) {
			if (strcmp(driver->name, list[i])==0)
				return 1;
		}
		driver = driver->clone_of;
	}
	return 0;
}

const struct mame_game* mame_playback_look(const char* file)
{
	INP_HEADER inp_header;
	void* playback;

	playback = osd_fopen(FILETYPE_INPUTLOG, 0, file, "rb");
	if (!playback) {
		target_err("Error opening the input playback file '%s'.\n", file);
		return 0;
	}

	/* read playback header */
	if (osd_fread(playback, &inp_header, sizeof(INP_HEADER)) != sizeof(INP_HEADER)) {
		osd_fclose(playback);
		target_err("Error reading the input playback file '%s'.\n", file);
		return 0;
	}

	osd_fclose(playback);

	if (!isalnum(inp_header.name[0])) {
		target_err("No game specified in the playback file '%s'.\n", file);
		return 0;
	} else {
		unsigned i;
		const struct mame_game* game = mame_game_at(0);

		for(i=0;game!=0;++i) {
			if (strcmp(mame_game_name(game), inp_header.name) == 0) {
				break;
			}
			game = mame_game_at(i+1);
		}

		if (!game) {
			target_err("Unknown game '%s' specified in the playback file.\n", inp_header.name);
			return 0;
		}

		return game;
	}
}

/**
 * Run a game
 */
int mame_game_run(struct advance_context* context, const struct mame_option* advance)
{
	int r;
	int game_index;

	/* store the game pointer */
	context->game = advance->game;

	options.record = 0;
	options.playback = 0;
	options.language_file = 0;
	options.mame_debug = advance->debug_flag;
	options.cheat = advance->cheat_flag;
	options.gui_host = 1; /* this prevents text mode messages that may stop the execution */
	options.skip_disclaimer = context->global.config.quiet_flag;
	options.skip_gameinfo = context->global.config.quiet_flag;
	options.samplerate = advance->samplerate;
	options.use_samples = advance->samples_flag;
	options.use_filter = advance->filter_flag;
	options.brightness = advance->brightness;
	options.pause_bright = 1.0;
	options.gamma = advance->gamma;
	options.color_depth = advance->color_depth;
	options.vector_width = advance->vector_width;
	options.vector_height = advance->vector_height;
	options.ui_orientation = advance->ui_orientation;
	options.beam = advance->beam;
	options.vector_flicker = advance->vector_flicker;
	options.vector_intensity = advance->vector_intensity;
	options.translucency = advance->translucency;
	options.antialias = advance->antialias;
	options.use_artwork = 0;
	if (advance->artwork_backdrop_flag)
		options.use_artwork |= ARTWORK_USE_BACKDROPS;
	if (advance->artwork_overlay_flag)
		options.use_artwork |= ARTWORK_USE_OVERLAYS;
	if (advance->artwork_bezel_flag)
		options.use_artwork |= ARTWORK_USE_BEZELS;
	options.artwork_res = 1; /* no artwork scaling */
	options.artwork_crop = advance->artwork_crop_flag;
	options.savegame = advance->savegame;
	options.debug_width = advance->debug_width;
	options.debug_height = advance->debug_height;
	options.debug_depth = 8;
	if (advance->bios_buffer[0] == 0 || strcmp(advance->bios_buffer, "default")==0)
		options.bios = 0;
	else
		options.bios = strdup(advance->bios_buffer); /* TODO leak */

	if (advance->language_file_buffer[0])
		options.language_file = mame_fopen(0, advance->language_file_buffer, FILETYPE_LANGUAGE, 0);
	else
		options.language_file = 0;

	if (advance->record_file_buffer[0]) {
		INP_HEADER inp_header;

		log_std(("glue: opening record file %s\n", advance->record_file_buffer));

		options.record = mame_fopen(advance->record_file_buffer, 0, FILETYPE_INPUTLOG, 1);
		if (!options.record) {
			target_err("Error opening the input record file '%s'.\n", advance->record_file_buffer);
			return -1;
		}

		memset(&inp_header, 0, sizeof(INP_HEADER));
		strncpy(inp_header.name, mame_game_name(advance->game), 8);

		inp_header.version[0] = 0;
		inp_header.version[1] = 0;
		inp_header.version[2] = 0;

		if (mame_fwrite(options.record, &inp_header, sizeof(INP_HEADER)) != sizeof(INP_HEADER)) {
			target_err("Error writing the input record file '%s'.\n", advance->record_file_buffer);
			return -1;
		}
	} else
		options.record = 0;

	if (advance->playback_file_buffer[0]) {
		INP_HEADER inp_header;

		log_std(("glue: opening playback file %s\n", advance->playback_file_buffer));

		options.playback = mame_fopen(advance->playback_file_buffer, 0, FILETYPE_INPUTLOG, 0);
		if (!options.playback) {
			target_err("Error opening the input playback file '%s'.\n", advance->playback_file_buffer);
			return -1;
		}

		/* read playback header */
		if (mame_fread(options.playback, &inp_header, sizeof(INP_HEADER)) != sizeof(INP_HEADER)) {
			target_err("Error reading the input playback file '%s'.\n", advance->playback_file_buffer);
			return -1;
		}

		if (!isalnum(inp_header.name[0])) {
			/* without header */
			mame_fseek(options.playback, 0, SEEK_SET);
		} else {
			unsigned i;
			const struct mame_game* game = mame_game_at(0);

			for(i=0;game!=0;++i) {
				if (strcmp(mame_game_name(game), inp_header.name) == 0) {
					break;
				}
				game = mame_game_at(i+1);
			}

			if (!game) {
				target_err("Unknown game '%s' specified in the playback file.\n", inp_header.name);
				return -1;
			}

			if (game != context->game) {
				target_err("The playback game '%s' and the requested game '%s' don't match.\n", mame_game_name(game), mame_game_name(context->game));
				return -1;
			}
		}
	} else
		options.playback = 0;

#ifdef MESS
	options.ram = advance->ram;
#endif

	if (!context->game) {
		target_err("No game specified.\n");
		return -1;
	}

	cheatfile = (char*)advance->cheat_file_buffer;
	history_filename = (char*)advance->history_file_buffer;
	mameinfo_filename = (char*)advance->info_file_buffer;

#ifdef MESS
	{
		const struct GameDriver* driver = (const struct GameDriver*)context->game;

		snprintf(GLUE.crc_file_buffer, sizeof(GLUE.crc_file_buffer), "%s%c%s.crc", advance->crc_dir_buffer, file_dir_slash(), driver->name);
		crcfile = GLUE.crc_file_buffer;

		log_std(("glue: file_crc %s\n", crcfile));

		if (driver->clone_of
			&& driver->clone_of->name
			&& (driver->clone_of->flags & NOT_A_DRIVER) == 0) {
			snprintf(GLUE.parent_crc_file_buffer, sizeof(GLUE.parent_crc_file_buffer), "%s%c%s.crc", advance->crc_dir_buffer, file_dir_slash(), driver->clone_of->name);
		} else {
			GLUE.parent_crc_file_buffer[0] = 0;
		}
		pcrcfile = GLUE.parent_crc_file_buffer;

		log_std(("glue: parent_file_crc %s\n", pcrcfile));
	}
#endif

	for(game_index=0;drivers[game_index];++game_index)
		if ((const struct GameDriver*)context->game == drivers[game_index])
			break;
	assert( drivers[game_index] != 0);
	if (!drivers[game_index])
		return -1;

	r = run_game(game_index);

	if (options.language_file)
		mame_fclose(options.language_file);
	if (options.record)
		mame_fclose(options.record);
	if (options.playback)
		mame_fclose(options.playback);

	return r;
}

/* Single port */
#define S(name, NAME) \
	{ name, IPT_##NAME }, \

/* Player port */
#define P(name, NAME) \
	{ "p1_" name, IPT_##NAME | IPF_PLAYER1 }, \
	{ "p2_" name, IPT_##NAME | IPF_PLAYER2 }, \
	{ "p3_" name, IPT_##NAME | IPF_PLAYER3 }, \
	{ "p4_" name, IPT_##NAME | IPF_PLAYER4 },

/* Player port with extension */
#define PE(name1, name2, NAME) \
	{ "p1_" name1, IPT_##NAME | IPF_PLAYER1 }, \
	{ "p1_" name2, (IPT_##NAME | IPF_PLAYER1) + IPT_EXTENSION }, \
	{ "p2_" name1, IPT_##NAME | IPF_PLAYER2 }, \
	{ "p2_" name2, (IPT_##NAME | IPF_PLAYER2) + IPT_EXTENSION }, \
	{ "p3_" name1, IPT_##NAME | IPF_PLAYER3 }, \
	{ "p3_" name2, (IPT_##NAME | IPF_PLAYER3) + IPT_EXTENSION }, \
	{ "p4_" name1, IPT_##NAME | IPF_PLAYER4 }, \
	{ "p4_" name2, (IPT_##NAME | IPF_PLAYER4) + IPT_EXTENSION }, \

static struct mame_port PORT[] = {
	/* same order of IPT_ declaration in inptport.h */

	/* JOYSTICK */
	P("up", JOYSTICK_UP)
	P("down", JOYSTICK_DOWN)
	P("left", JOYSTICK_LEFT)
	P("right", JOYSTICK_RIGHT)

	/* BUTTON */
	P("button1", BUTTON1)
	P("button2", BUTTON2)
	P("button3", BUTTON3)
	P("button4", BUTTON4)
	P("button5", BUTTON5)
	P("button6", BUTTON6)
	P("button7", BUTTON7)
	P("button8", BUTTON8)
	P("button9", BUTTON9)
	P("button10", BUTTON10)

	/* PADDLE */
	PE("paddle_left", "paddle_right", PADDLE)
	PE("paddle_up", "paddle_down", PADDLE_V)

	/* DIAL */
	PE("dial_left", "dial_right", DIAL)
	PE("dial_up", "dial_down", DIAL_V)

	/* TRACKBALL */
	PE("trackball_left", "trackball_right", TRACKBALL_X)
	PE("trackball_up", "trackball_down", TRACKBALL_Y)

	/* AD_STICK */
	PE("stick_left", "stick_right", AD_STICK_X)
	PE("stick_up", "stick_down", AD_STICK_Y)
	PE("stick_forward", "stick_backward", AD_STICK_Z)

	/* LIGHTGUN */
	PE("lightgun_left", "lightgun_right", LIGHTGUN_X)
	PE("lightgun_up", "lightgun_down", LIGHTGUN_Y)

	/* PEDAL */
	PE("pedal_up", "pedal_down", PEDAL)
	PE("pedal2_up", "pedal2_down", PEDAL2)

	/* START */
	S("start1", START1)
	S("start2", START2)
	S("start3", START3)
	S("start4", START4)

	/* COIN */
	S("coin1", COIN1)
	S("coin2", COIN2)
	S("coin3", COIN3)
	S("coin4", COIN4)

	/* SERVICEK */
	S("service_coin1", SERVICE1)
	S("service_coin2", SERVICE2)
	S("service_coin3", SERVICE3)
	S("service_coin4", SERVICE4)

	/* SERVICE */
	S("service", SERVICE)

	/* TILT */
	S("tilt", TILT)

	/* UI specific of AdvanceMAME */
	S("ui_mode_next", UI_MODE_NEXT)
	S("ui_mode_pred", UI_MODE_PRED)
	S("ui_record_start", UI_RECORD_START)
	S("ui_record_stop", UI_RECORD_STOP)
	S("ui_turbo", UI_TURBO)

	/* UI */
	S("ui_configure", UI_CONFIGURE)
	S("ui_on_screen_display", UI_ON_SCREEN_DISPLAY)
	S("ui_pause", UI_PAUSE)
	S("ui_reset_machine", UI_RESET_MACHINE)
	S("ui_show_gfx", UI_SHOW_GFX)
	S("ui_frameskip_dec", UI_FRAMESKIP_DEC)
	S("ui_frameskip_inc", UI_FRAMESKIP_INC)
	S("ui_throttle", UI_THROTTLE)
	S("ui_show_fps", UI_SHOW_FPS)
	S("ui_snapshot", UI_SNAPSHOT)
	S("ui_toggle_cheat", UI_TOGGLE_CHEAT)
	S("ui_up", UI_UP)
	S("ui_down", UI_DOWN)
	S("ui_left", UI_LEFT)
	S("ui_right", UI_RIGHT)
	S("ui_select", UI_SELECT)
	S("ui_cancel", UI_CANCEL)
	S("ui_pan_up", UI_PAN_UP)
	S("ui_pan_down", UI_PAN_DOWN)
	S("ui_pan_left", UI_PAN_LEFT)
	S("ui_pan_right", UI_PAN_RIGHT)
	S("ui_show_profiler", UI_SHOW_PROFILER)
	S("ui_toggle_ui", UI_TOGGLE_UI)
	S("ui_toggle_debug", UI_TOGGLE_DEBUG)
	S("ui_save_state", UI_SAVE_STATE)
	S("ui_load_state", UI_LOAD_STATE)
	S("ui_add_cheat", UI_ADD_CHEAT)
	S("ui_delete_cheat", UI_DELETE_CHEAT)
	S("ui_save_cheat", UI_SAVE_CHEAT)
	S("ui_watch_value", UI_WATCH_VALUE)

	/* specific of AdvanceMAME */
	S("safequit", MAME_PORT_SAFEQUIT)
	S("event1", MAME_PORT_EVENT1)
	S("event2", MAME_PORT_EVENT2)
	S("event3", MAME_PORT_EVENT3)
	S("event4", MAME_PORT_EVENT4)
	S("event5", MAME_PORT_EVENT5)
	S("event6", MAME_PORT_EVENT6)

	{ 0, 0 }
};

struct mame_port* mame_port_list(void)
{
	return PORT;
}

/***************************************************************************/
/* MAME callback interface */

/**
 * Check if a MAME port is active.
 * A port is active if the associated key sequence is pressed.
 * The port values are only values get from the mame_port_list() function or one of the IPT_MAME_PORT_* defines.
 */
int mame_ui_port_pressed(unsigned port)
{
	struct advance_safequit_context* safequit_context = &CONTEXT.safequit;
	InputSeq* seq;

	switch (port) {
	case IPT_MAME_PORT_SAFEQUIT :
		return advance_safequit_can_exit(safequit_context);
	case IPT_MAME_PORT_EVENT1 :
		return (advance_safequit_event_mask(safequit_context) & 0x4) != 0;
	case IPT_MAME_PORT_EVENT2 :
		return (advance_safequit_event_mask(safequit_context) & 0x8) != 0;
	case IPT_MAME_PORT_EVENT3 :
		return (advance_safequit_event_mask(safequit_context) & 0x10) != 0;
	case IPT_MAME_PORT_EVENT4 :
		return (advance_safequit_event_mask(safequit_context) & 0x20) != 0;
	case IPT_MAME_PORT_EVENT5 :
		return (advance_safequit_event_mask(safequit_context) & 0x40) != 0;
	case IPT_MAME_PORT_EVENT6 :
		return (advance_safequit_event_mask(safequit_context) & 0x80) != 0;
	}

	seq = input_port_type_seq(port);
	if (!seq)
		return 0;
	return seq_pressed(seq);
}

/**
 * Convert a os depended key code in a MAME code.
 */
unsigned mame_ui_code_from_oskey(unsigned oscode)
{
	return keyoscode_to_code(oscode);
}

/**
 * Convert a os depended joystick code in a MAME code.
 */
unsigned mame_ui_code_from_osjoystick(unsigned oscode)
{
	return joyoscode_to_code(oscode);
}

void mame_ui_area_set(unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
	set_ui_visarea(x1, y1, x2, y2);
}

void mame_ui_message(const char* s, ...)
{
	va_list arg;
	char buffer[256];
	va_start(arg, s);
	vsnprintf(buffer, sizeof(buffer), s, arg);
	usrintf_showmessage(buffer);
	va_end(arg);
}

void mame_ui_text(const char* s, unsigned x, unsigned y)
{
	ui_text(GLUE.bitmap, s, x, y);
}

void mame_ui_menu(const char** items, const char** subitems, char* flag, int selected, int arrowize_subitem)
{
	ui_displaymenu(GLUE.bitmap, items, subitems, flag, selected, arrowize_subitem);
}

const char* mame_ui_gettext(const char* text)
{
	return text;
}

void mame_ui_refresh(void)
{
	schedule_full_refresh();
}

void mame_ui_swap(void)
{
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

		GLUE.bitmap_alt = bitmap_alloc_depth(width, height, depth);
	}

	/* swap the contents of the two bitmap */
	memcpy(&tmp, Machine->scrbitmap, sizeof(struct mame_bitmap));
	memcpy(Machine->scrbitmap, GLUE.bitmap_alt, sizeof(struct mame_bitmap));
	memcpy(GLUE.bitmap_alt, &tmp, sizeof(struct mame_bitmap));

	/* redraw all */
	schedule_full_refresh();
}

void mame_ui_gamma_factor_set(double gamma)
{
	palette_set_global_gamma(palette_get_global_gamma() * gamma);
}

void mame_ui_show_info_temp(void)
{
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

	/* print any buffered message before setting the video mode */
	target_flush();

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
		GLUE.option.color_def = 0;
		GLUE.option.rgb_flag = 0;
	} else if (GLUE.option.bits_per_pixel == 15) {
		GLUE.option.color_def = color_def_make_from_rgb_sizelenpos(2, 5, 10, 5, 5, 5, 0);
		GLUE.option.rgb_flag = 1;
	} else if (GLUE.option.bits_per_pixel == 32) {
		GLUE.option.color_def = color_def_make_from_rgb_sizelenpos(4, 8, 16, 8, 8, 8, 0);
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

void osd_close_display(void)
{
	log_std(("osd: osd_close_display()\n"));

	if (GLUE.video_flag)
		osd2_video_done();
}

int osd_menu(struct mame_bitmap *bitmap, int selected)
{
	unsigned input;
	int r;

	log_pedantic(("osd: osd_menu(%d)\n", selected));

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
	if (input_ui_pressed_repeat(IPT_UI_UP, 8))
		input |= OSD_INPUT_UP;
	if (input_ui_pressed_repeat(IPT_UI_DOWN, 8))
		input |= OSD_INPUT_DOWN;
	if (input_ui_pressed_repeat(IPT_UI_LEFT, 8))
		input |= OSD_INPUT_LEFT;
	if (input_ui_pressed_repeat(IPT_UI_RIGHT, 8))
		input |= OSD_INPUT_RIGHT;

	r = osd2_menu(selected, input);
	if (r < 0)
		return -1;
	else
		return r;
}

/**
 * Compute the sound samples required for the next frame.
 */
static unsigned glue_sound_sample(void)
{
	int samples = GLUE.sound_step - GLUE.sound_latency;

	/* Correction for a generic sound buffer underflow. */
	/* Generally happen that the DMA buffer underflow reporting */
	/* a very full state instead of an empty one. */
	if (samples < 0) {
		log_std(("WARNING:glue: negative sound samples %d adjusted to 16\n", samples));
		samples = 16;
	}
	if (samples < 16) {
		log_std(("WARNING:glue: too small sound samples %d adjusted to 16\n", samples));
		samples = 16;
	}

	GLUE.sound_last_count = samples;

	return GLUE.sound_last_count;
}

void osd_update_video_and_audio(struct mame_display *display)
{
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
				log_std(("ERROR:glue: silence underflow! %d %d\n", sample_count, GLUE.sound_silence_count));
				sample_count = GLUE.sound_silence_count;
			}

			sample_buffer = GLUE.sound_silence_buffer;
		}
	} else {
		/* no sound to play */
		sample_buffer = 0;
		sample_count = 0;
	}

	osd2_message();

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

int osd_start_audio_stream(int stereo)
{
	unsigned rate = Machine->sample_rate;

	log_std(("osd: osd_start_audio_stream(sample_rate:%d, stereo_flag:%d)\n", rate, stereo));

	assert( GLUE.sound_flag == 0 );

	if (osd2_sound_init(&rate, stereo) != 0) {
		log_std(("osd: osd_start_audio_stream return no sound. Disable MAME sound generation.\n"));

		osd2_thread_init();

		/* disable the MAME sound generation */
		Machine->sample_rate = 0;
		return 0;
	}

	osd2_thread_init();

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

void osd_stop_audio_stream(void)
{
	log_std(("osd: osd_stop_audio_stream()\n"));

	osd2_thread_done();

	if (GLUE.sound_flag) {
		free(GLUE.sound_silence_buffer);
		osd2_sound_done();

		GLUE.sound_flag = 0;
	}
}

int osd_update_audio_stream(short* buffer)
{
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

cycles_t osd_cycles(void)
{
	return target_clock();
}

cycles_t osd_cycles_per_second(void)
{
	return TARGET_CLOCKS_PER_SEC;
}

cycles_t osd_profiling_ticks(void)
{
	return target_clock();
}

/* Filter the user interface input state */
int osd_input_ui_filter(int result, int type)
{
	return result || hardware_is_input_simulated(SIMULATE_EVENT, type);
}

/* Filter the main exit request */
int osd_input_exit_filter(int result)
{
	return advance_input_exit_filter(&CONTEXT.input, &CONTEXT.safequit, result);
}

/* Filter the input port state */
int osd_input_port_filter(int result, int type)
{
	return result || hardware_is_input_simulated(SIMULATE_EVENT, type);
}

const char *osd_get_fps_text(const struct performance_info *performance)
{
	unsigned l;

	osd2_info(GLUE.info_buffer, sizeof(GLUE.info_buffer));

	l = strlen(GLUE.info_buffer);

	/* for vector games, add the number of vector updates */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) {
		snprintf(GLUE.info_buffer + l, sizeof(GLUE.info_buffer) - l, "\n%4d vector updates", performance->vector_updates_last_second);
	} else if (performance->partial_updates_this_frame > 1) {
		snprintf(GLUE.info_buffer + l, sizeof(GLUE.info_buffer) - l, "\n%4d partial updates", performance->partial_updates_this_frame);
	}
	
	return GLUE.info_buffer;
}

/***************************************************************************/
/* Config */

static adv_conf_enum_int OPTION_DEPTH[] = {
{ "auto", 0 },
{ "8", 8 },
{ "15", 15 },
{ "16", 16 },
{ "32", 32 }
};

#ifdef MESS

/* This is the list of the MESS recognized devices, it must be syncronized */
/* with the devices names present in the mess/device.c file */
static const char* DEVICES[] = {
	"cartridge",
	"floppydisk",
	"harddisk",
	"cylinder",
	"cassette",
	"punchcard",
	"punchtape",
	"printer",
	"serial",
	"parallel",
	"snapshot",
	"quickload",
	0
};

static int mess_printf_callback(const char *fmt, va_list arg)
{
	target_err_va(fmt, arg);
	return 0;
}

static void mess_init(adv_conf* context)
{
	const char** i;

	options.image_count = 0;

	/* use the old portable ui */
	options.disable_normal_ui = 0;

	/* set the callback for messages */
	options.mess_printf_output = mess_printf_callback;

	i = DEVICES;
	while (*i) {
		char buffer[256];

		snprintf(buffer, sizeof(buffer), "dev_%s", *i);

		conf_string_register_multi(context, buffer);

		++i;
	}

	conf_string_register_default(context, "misc_ramsize", "auto");
}

static int mess_config_load(adv_conf* context, struct mame_option* option)
{
	const char** i;
	const char* s;

	i = DEVICES;
	while (*i) {
		adv_conf_iterator j;
		char buffer[256];

		snprintf(buffer, sizeof(buffer), "dev_%s", *i);

		conf_iterator_begin(&j, context, buffer);
		while (!conf_iterator_is_end(&j)) {
			const char* arg = conf_iterator_string_get(&j);
			int id;

			log_std(("glue: register device %s %s\n", *i, arg));

			id = device_typeid(*i);
			if (id < 0) {
				/* If we get to here, log the error - This is mostly due to a mismatch in the array */
				log_std(("ERROR:glue: unknown mess devices %s\n", *i));
				return -1;
			}

			/* register the devices with its arg */
			if (register_device(id, arg) != 0) {
				log_std(("ERROR:glue: calling register_device(type:%d, arg:%s)\n", id, arg));
				return -1;
			}

			conf_iterator_next(&j);
		}

		++i;
	}

	s = conf_string_get_default(context, "misc_ramsize");
	if (strcmp(s, "auto") == 0) {
		option->ram = 0;
	} else {
		char* e;
		option->ram = strtol(s,&e,10);

		if (*e == 'k') {
			option->ram *= 1024;
			++e;
		} else if (*e == 'M') {
			option->ram *= 1024*1024;
			++e;
		} else if (*e == 'G') {
			option->ram *= 1024*1024*1024;
			++e;
		}

		if (option->ram == 0 || *e) {
			target_err("Invalid argument '%s' for option 'misc_ramsize'\n", s);
			return -1;
		}
	}

	return 0;
}

static void mess_done(void)
{
}

#endif

adv_error mame_init(struct advance_context* context, adv_conf* cfg_context)
{

	/* Clear the MAIN context */
	memset(context, 0, sizeof(struct advance_context));

	/* Clear the GLUE context */
	memset(&GLUE, 0, sizeof(GLUE));

	/* Clear the MAME global struct */
	memset(&options, 0, sizeof(options));

	conf_bool_register_default(cfg_context, "display_artwork_backdrop", 1);
	conf_bool_register_default(cfg_context, "display_artwork_overlay", 1);
	conf_bool_register_default(cfg_context, "display_artwork_bezel", 0);
	conf_bool_register_default(cfg_context, "display_artwork_crop", 1);
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
	conf_string_register_default(cfg_context, "misc_languagefile", "english.lng");
	conf_string_register_default(cfg_context, "misc_cheatfile", "cheat.dat" );
	conf_string_register_default(cfg_context, "misc_historyfile", "history.dat");

	conf_string_register_default(cfg_context, "misc_bios", "default");

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

void mame_done(struct advance_context* context)
{
#ifdef MESS
	mess_done();
#endif
}

adv_error mame_config_load(adv_conf* cfg_context, struct mame_option* option)
{
	char* s;

	option->artwork_backdrop_flag = conf_bool_get_default(cfg_context, "display_artwork_backdrop");
	option->artwork_overlay_flag = conf_bool_get_default(cfg_context, "display_artwork_overlay");
	option->artwork_bezel_flag = conf_bool_get_default(cfg_context, "display_artwork_bezel");
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

	sncpy(option->language_file_buffer, sizeof(option->language_file_buffer), conf_string_get_default(cfg_context, "misc_languagefile"));

	sncpy(option->cheat_file_buffer, sizeof(option->cheat_file_buffer), conf_string_get_default(cfg_context, "misc_cheatfile"));

	sncpy(option->bios_buffer, sizeof(option->bios_buffer), conf_string_get_default(cfg_context, "misc_bios"));

	/* convert the dir separator char to ';'. */
	/* the cheat system use always this char in all the operating system */
	for(s=option->cheat_file_buffer;*s;++s)
		if (*s == file_dir_separator())
			*s = ';';

	sncpy(option->history_file_buffer, sizeof(option->history_file_buffer), conf_string_get_default(cfg_context, "misc_historyfile"));
	sncpy(option->info_file_buffer, sizeof(option->info_file_buffer), conf_string_get_default(cfg_context, "misc_infofile"));

#ifdef MESS
	if (mess_config_load(cfg_context, option) != 0) {
		target_err("Error loading the device configuration options.\n");
		return -1;
	}
#endif

	return 0;
}

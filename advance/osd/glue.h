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

#ifndef __GLUE_H
#define __GLUE_H

#include "video.h"

#include <stdio.h>

/** Max path length */
#define MAME_MAXPATH 512

#ifdef MESS
#define MAME_MAXIMAGE 32
struct mame_image {
	const char* option;
	const char* arg;
};
#endif

/** Virtual type of a mame game. */
typedef void mame_game;

struct mame_option {
	const mame_game* game;

	int quiet_flag;

	int cheat_flag;

	double gamma;
	double brightness;

	int samplerate;
	int samples_flag;
	int filter_flag;

	int color_depth;

	int vector_width;
	int vector_height;

	int debug_flag;
	int debug_width;
	int debug_height;

	unsigned ui_orientation;
	int norotate;
	int ror;
	int rol;
	int flipx;
	int flipy;

	int antialias;
	int translucency;
	int beam;
	float vector_flicker;
	float vector_intensity;

	int artwork_flag;
	int artwork_crop_flag;

	char savegame;

	char language_file[MAME_MAXPATH];
	char cheat_file[MAME_MAXPATH];
	char history_file[MAME_MAXPATH];
	char info_file[MAME_MAXPATH];

#ifdef MESS
	char crc_dir[MAME_MAXPATH];
	struct mame_image* image_map[MAME_MAXIMAGE];
	unsigned image_mac;
#endif
};

/***************************************************************************/
/* MAME interface */

const mame_game* mame_game_at(unsigned index);
const char* mame_game_resolution(const mame_game* game);
double mame_game_fps(const mame_game* game);
unsigned mame_game_orientation(const mame_game* game);
const char* mame_game_name(const mame_game* game);
const char* mame_game_description(const mame_game* game);
void mame_print_info(FILE* out);
int mame_is_game_in_list(const char* list[], const mame_game* game);
int mame_is_game_vector(const mame_game* game);

/***************************************************************************/
/* MAME callback interface */

void mame_ui_area_set(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void mame_ui_message(const char* s, ...);
void mame_ui_menu(const char** items, const char** subitems, char* flag, int selected, int arrowize_subitem);
const char* mame_ui_gettext(const char* text);
void mame_ui_save_snapshot(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void mame_ui_refresh(void);
void mame_ui_swap(void);
void mame_ui_gamma_factor_set(double gamma);
void mame_ui_show_info_temp(void);

/***************************************************************************/
/* OSD interface */

/** Mask. */
typedef uint32 osd_mask_t;
#define osd_mask_size 32
#define osd_mask_full 0xFFFFFFFF

/** RGB color. */
/* must match MAME rgb_t. */
typedef uint32 osd_rgb_t;

static inline uint8 osd_rgb_red(osd_rgb_t v) {
	return (v >> 16) & 0xFF;
}

static inline uint8 osd_rgb_green(osd_rgb_t v) {
	return (v >> 8) & 0xFF;
}

static inline uint8 osd_rgb_blue(osd_rgb_t v) {
	return v & 0xFF;
}

static inline osd_rgb_t osd_rgb(uint8 r, uint8 g, uint8 b) {
	return (((unsigned)r) << 16) | (((unsigned)g) << 8) | b;
}

/** Orientations. */
#define OSD_ORIENTATION_FLIP_X 0x0001
#define OSD_ORIENTATION_FLIP_Y 0x0002
#define OSD_ORIENTATION_SWAP_XY 0x0004
#define OSD_ORIENTATION_ROT0 0
#define OSD_ORIENTATION_ROT90 (OSD_ORIENTATION_SWAP_XY | OSD_ORIENTATION_FLIP_X)
#define OSD_ORIENTATION_ROT180 (OSD_ORIENTATION_FLIP_X | OSD_ORIENTATION_FLIP_Y)
#define OSD_ORIENTATION_ROT270 (OSD_ORIENTATION_SWAP_XY | OSD_ORIENTATION_FLIP_Y)

/* Bitmap */
struct osd_bitmap {
	void* ptr;
	unsigned size_x;
	unsigned size_y;
	unsigned bytes_per_scanline;
};

struct osd_video_option {
	unsigned area_size_x; /* max size of the visible part */
	unsigned area_size_y;

	unsigned used_pos_x; /* current pos of the visible part in the bitmap */
	unsigned used_pos_y;
	unsigned used_size_x; /* current size of the visible part in the bitmap, smaller or equal at area_size. */
	unsigned used_size_y;

	unsigned aspect_x; /* aspect of the bitmap */
	unsigned aspect_y;

	unsigned bits_per_pixel; /* depth in bit of the bitmap */

	int rgb_flag; /* !=0 if the bitmap already contains RGB values */
	video_rgb_def rgb_def; /* RGB format */

	int vector_flag;

	double fps; /* frame rate */

	/* game palette */
	unsigned colors;
	uint32* rgb_components;
};

/* Input */
typedef unsigned osd_input;

#define OSD_INPUT_THROTTLE 0x00000002
#define OSD_INPUT_FRAMESKIP_DEC 0x00000004
#define OSD_INPUT_FRAMESKIP_INC 0x00000008
#define OSD_INPUT_TOGGLE_DEBUG 0x00000010
#define OSD_INPUT_MODE_PRED 0x00000020
#define OSD_INPUT_MODE_NEXT 0x00000040
#define OSD_INPUT_PAN_RIGHT 0x00000080
#define OSD_INPUT_PAN_LEFT 0x00000100
#define OSD_INPUT_PAN_UP 0x00000200
#define OSD_INPUT_PAN_DOWN 0x00000400
#define OSD_INPUT_TURBO 0x00000800
#define OSD_INPUT_UP 0x00001000
#define OSD_INPUT_DOWN 0x00002000
#define OSD_INPUT_LEFT 0x00004000
#define OSD_INPUT_RIGHT 0x00008000
#define OSD_INPUT_SELECT 0x00010000
#define OSD_INPUT_CANCEL 0x00020000
#define OSD_INPUT_CONFIGURE 0x00040000
#define OSD_INPUT_COIN1 0x00100000
#define OSD_INPUT_COIN2 0x00200000
#define OSD_INPUT_COIN3 0x00400000
#define OSD_INPUT_COIN4 0x00800000
#define OSD_INPUT_START1 0x01000000
#define OSD_INPUT_START2 0x02000000
#define OSD_INPUT_START3 0x04000000
#define OSD_INPUT_START4 0x08000000

int osd2_video_init(struct osd_video_option* option);
void osd2_video_done(void);
int osd2_menu(int selected, unsigned input);
int osd2_frame(const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count);
void osd2_palette(const osd_mask_t* mask, const osd_rgb_t* palette, unsigned size);
void osd2_area(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void osd2_save_snapshot(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void osd2_info(char* buffer, unsigned size);
void osd2_debugger_focus(int debugger_has_focus);

int osd2_sound_init(unsigned* sample_rate, int stereo_flag);
void osd2_sound_done(void);

#endif

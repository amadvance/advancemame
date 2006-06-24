/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2004 Andrea Mazzoleni
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
#include "bitmap.h"
#include "conf.h"

#include <stdio.h>

/** Max bios name length. */
#define MAME_MAXBIOS 64

/** Max path length. */
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

	adv_bool cheat_flag;

	double gamma;
	double brightness;

	int samplerate;
	int samples_flag;

	int vector_width;
	int vector_height;

	int debug_flag;
	int debug_width;
	int debug_height;

	unsigned ui_orientation;
	unsigned direct_orientation;
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

	adv_bool artwork_backdrop_flag;
	adv_bool artwork_overlay_flag;
	adv_bool artwork_bezel_flag;
	adv_bool artwork_crop_flag;

	char savegame_file_buffer[MAME_MAXPATH];
	char language_file_buffer[MAME_MAXPATH];
	char record_file_buffer[MAME_MAXPATH];
	char playback_file_buffer[MAME_MAXPATH];
	char cheat_file_buffer[MAME_MAXPATH];
	char hiscore_file_buffer[MAME_MAXPATH];
	char bios_buffer[MAME_MAXBIOS];

#ifdef MESS
	char crc_dir_buffer[MAME_MAXPATH];
	struct mame_image* image_map[MAME_MAXIMAGE];
	unsigned image_mac;
	unsigned ram;
#endif
};

/***************************************************************************/
/* MAME interface */

const mame_game* mame_game_at(unsigned index);
adv_bool mame_game_working(const mame_game* game);
const char* mame_game_resolution(const mame_game* game);
const char* mame_game_resolutionclock(const mame_game* game);
double mame_game_fps(const mame_game* game);
unsigned mame_game_orientation(const mame_game* game);
const char* mame_game_name(const mame_game* game);
const char* mame_game_description(const mame_game* game);
const char* mame_game_lang(const mame_game* game);
const char* mame_software_name(const mame_game* game, adv_conf* context);
const char* mame_section_name(const mame_game* game, adv_conf* context);
const mame_game* mame_game_parent(const mame_game* game);
const char* mame_game_description(const mame_game* game);
const char* mame_game_year(const mame_game* game);
const char* mame_game_manufacturer(const mame_game* game);
unsigned mame_game_players(const mame_game* game);
const char* mame_game_control(const mame_game* game);
void mame_print_xml(FILE* out);
adv_bool mame_is_game_vector(const mame_game* game);
adv_bool mame_is_game_relative(const char* relative, const mame_game* game);
const struct mame_game* mame_playback_look(const char* file);

struct mame_port {
	const char* name; /**< Name of the port. */
	const char* desc; /**< Description. */
	unsigned port; /**< Unique port value. */
};

struct mame_port* mame_port_list(void);
struct mame_port* mame_port_find(unsigned port);
int mame_port_player(unsigned port);
void mame_name_adjust(char* dst, unsigned size, const char* s);

struct mame_analog {
	const char* name; /**< Name of the analog port. */
	unsigned port; /**< Unique analog port value. */
};

struct mame_analog* mame_analog_list(void);
struct mame_analog* mame_analog_find(unsigned port);

/***************************************************************************/
/* Conversion */

unsigned glue_port_seq_begin(unsigned type);
unsigned glue_port_seq_end(unsigned type);
int glue_port_seqtype(unsigned type, unsigned index);
unsigned glue_port_convert(unsigned type, unsigned player, unsigned seqtype, const char* name);
void glue_seq_convert(unsigned* mame_seq, unsigned mame_max, unsigned* seq, unsigned max);
void glue_seq_convertback(unsigned* seq, unsigned max, unsigned* mame_seq, unsigned mame_max);

/***************************************************************************/
/* MAME callback interface */

#define MAME_INPUT_MAP_MAX 16

struct mame_digital_map_entry {
	unsigned port; /**< Digital port. */
	unsigned seq[MAME_INPUT_MAP_MAX]; /**< Sequence assigned. */
	adv_bool port_state; /**< State of the port. */
};

/**
 * Mask used to store the player number.
 * The player number uses 1 for player1. 0 is used for global port.
 */
#define MAME_PORT_PLAYER_SHIFT 16
#define MAME_PORT_PLAYER_MASK 0xff0000

/**
 * Mask used to store the port type.
 */
#define MAME_PORT_TYPE_SHIFT 0
#define MAME_PORT_TYPE_MASK 0xff

/**
 * Mask used to store the port index and the key code.
 * The port index is used to double the ports which need more than
 * one keyboard sequence like all the mame analog ports.
 * The key code is used for the mess keyboard emulation and it contains
 * the keyboard code emulated.
 */
#define MAME_PORT_INDEX_SHIFT 8
#define MAME_PORT_INDEX_MASK 0xff00

/**
 * Firt free port for new port types.
 */
#define MAME_PORT_INTERNAL 0xc0

#define IPT_MAME_PORT_SAFEQUIT (MAME_PORT_INTERNAL + 0)
#define IPT_MAME_PORT_EVENT1 (MAME_PORT_INTERNAL + 1)
#define IPT_MAME_PORT_EVENT2 (MAME_PORT_INTERNAL + 2)
#define IPT_MAME_PORT_EVENT3 (MAME_PORT_INTERNAL + 3)
#define IPT_MAME_PORT_EVENT4 (MAME_PORT_INTERNAL + 4)
#define IPT_MAME_PORT_EVENT5 (MAME_PORT_INTERNAL + 5)
#define IPT_MAME_PORT_EVENT6 (MAME_PORT_INTERNAL + 6)
#define IPT_MAME_PORT_EVENT7 (MAME_PORT_INTERNAL + 7)
#define IPT_MAME_PORT_EVENT8 (MAME_PORT_INTERNAL + 8)
#define IPT_MAME_PORT_EVENT9 (MAME_PORT_INTERNAL + 9)
#define IPT_MAME_PORT_EVENT10 (MAME_PORT_INTERNAL + 10)
#define IPT_MAME_PORT_EVENT11 (MAME_PORT_INTERNAL + 11)
#define IPT_MAME_PORT_EVENT12 (MAME_PORT_INTERNAL + 12)
#define IPT_MAME_PORT_EVENT13 (MAME_PORT_INTERNAL + 13)
#define IPT_MAME_PORT_EVENT14 (MAME_PORT_INTERNAL + 14)

#define MAME_PORT(type, player, index) \
	(((type) << MAME_PORT_TYPE_SHIFT) | ((player) << MAME_PORT_PLAYER_SHIFT) | ((index) << MAME_PORT_INDEX_SHIFT))

#define MAME_PORT_UI(type) \
	MAME_PORT(type, 0, 0)

#define MAME_PORT_PLAYER(type, player) \
	MAME_PORT(type, player, 0)

#define MAME_PORT_INDEX(type, player, index) \
	MAME_PORT(type, player, index)

#define MAME_PORT_KEYBOARD(keyboard) \
	MAME_PORT_INDEX(IPT_KEYBOARD, 0, keyboard)

#define MAME_PORT_TYPE_GET(port) \
	(((port) & MAME_PORT_TYPE_MASK) >> MAME_PORT_TYPE_SHIFT)

#define MAME_PORT_PLAYER_GET(port) \
	(((port) & MAME_PORT_PLAYER_MASK) >> MAME_PORT_PLAYER_SHIFT)

#define MAME_PORT_INDEX_GET(port) \
	(((port) & MAME_PORT_INDEX_MASK) >> MAME_PORT_INDEX_SHIFT)

#define MAME_PORT_KEYBOARD_GET(port) \
	MAME_PORT_INDEX_GET(port)

int mame_ui_port_pressed(unsigned port);
void mame_ui_area_set(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void mame_ui_refresh(void);
void mame_ui_gamma_factor_set(double gamma);
unsigned char mame_ui_cpu_read(unsigned cpu, unsigned addr);
unsigned mame_ui_frames_per_second(void);
void mame_ui_input_map(unsigned* pdigital_mac, struct mame_digital_map_entry* digital_map, unsigned digital_max);

/***************************************************************************/
/* OSD interface */

/** Mask. */
typedef uint32 osd_mask_t;
#define osd_mask_size 32
#define osd_mask_full 0xFFFFFFFF

/** RGB color. */
/* must match MAME rgb_t. */
typedef uint32 osd_rgb_t;

static inline uint8 osd_rgb_red(osd_rgb_t v)
{
	return (v >> 16) & 0xFF;
}

static inline uint8 osd_rgb_green(osd_rgb_t v)
{
	return (v >> 8) & 0xFF;
}

static inline uint8 osd_rgb_blue(osd_rgb_t v)
{
	return v & 0xFF;
}

static inline osd_rgb_t osd_rgb(uint8 r, uint8 g, uint8 b)
{
	return (((unsigned)r) << 16) | (((unsigned)g) << 8) | b;
}

/** Orientations. */
#define OSD_ORIENTATION_FLIP_X ADV_ORIENTATION_FLIP_X
#define OSD_ORIENTATION_FLIP_Y ADV_ORIENTATION_FLIP_Y
#define OSD_ORIENTATION_SWAP_XY ADV_ORIENTATION_FLIP_XY
#define OSD_ORIENTATION_ROT0 ADV_ORIENTATION_ROT0
#define OSD_ORIENTATION_ROT90 ADV_ORIENTATION_ROT90
#define OSD_ORIENTATION_ROT180 ADV_ORIENTATION_ROT180
#define OSD_ORIENTATION_ROT270 ADV_ORIENTATION_ROT270

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
	adv_color_def color_def; /* Color format */

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
#define OSD_INPUT_COCKTAIL 0x10000000
#define OSD_INPUT_HELP 0x20000000
#define OSD_INPUT_SHOW_FPS 0x40000000
#define OSD_INPUT_STARTUP_END 0x80000000

int osd2_video_init(struct osd_video_option* option);
void osd2_video_done(void);
void osd2_video_pause(int pause);
int osd2_thread_init(void);
void osd2_thread_done(void);
int osd2_video_menu(int selected, unsigned input);
int osd2_audio_menu(int selected, unsigned input);
int osd2_frame(const struct osd_bitmap* game, const struct osd_bitmap* debug, const osd_rgb_t* debug_palette, unsigned debug_palette_size, unsigned led, unsigned input, const short* sample_buffer, unsigned sample_count);
void osd2_palette(const osd_mask_t* mask, const osd_rgb_t* palette, unsigned size);
void osd2_area(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void osd2_save_snapshot(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void osd2_info(char* buffer, unsigned size);
void osd2_debugger_focus(int debugger_has_focus);
void osd2_message(void);

int osd2_sound_init(unsigned* sample_rate, int stereo_flag);
void osd2_sound_done(void);
void osd2_sound_pause(int pause);

#endif

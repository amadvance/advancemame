/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2004, 2005 Andrea Mazzoleni
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
#include "hscript.h"

#include "glueint.h"

#include "advance.h"

#include <math.h>

#if HAVE_SYS_MMAN_H
#include <sys/mman.h> /* for mprotect */
#endif

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
	"memcard",
	"cdrom",
	0
};
#endif

struct advance_glue_context {
	mame_bitmap* bitmap;
	mame_bitmap* bitmap_alt;
	struct osd_video_option option;

	int video_flag; /** If the video initialization completed with success. */

	int sound_flag; /**< Sound main active flag. */
	unsigned sound_step; /**< Number of sound samples for a single frame. This is the ideal value. */
	unsigned sound_last_count; /** Number of sound samples for the last frame. */
	int sound_latency; /**< Current samples in excess. Updated at every frame. */
	double sound_speed; /**< Current speed adjustment. */
	double sound_fps; /**< Current fps speed. */

	short* sound_silence_buffer; /**< Buffer filled of silence. */
	unsigned sound_silence_count; /**< Number of samples for the silence buffer. */

	const short* sound_current_buffer; /**< Last buffer of samples to play. */
	unsigned sound_current_count; /**< Number of samples in the last buffer. */

#ifdef MESS
	char crc_file_buffer[MAME_MAXPATH]; /**< Storage for the the crcfile MESS pointer. */
	char parent_crc_file_buffer[MAME_MAXPATH]; /**< Storage for the the pcrcfile MESS pointer. */
#endif

	char resolution_buffer[32]; /**< Buffer used by mame_resolution(). */
	char resolutionclock_buffer[32]; /**< Buffer used by mame_resolutionclock(). */
	char software_buffer[256]; /**< Buffer for software name. */

	unsigned input; /**< Last user interface input. */
};

static struct advance_glue_context GLUE;

/***************************************************************************/
/* MAME */

/* MAME internal variables */
extern char* cheatfile;
extern const char *db_filename;
#ifdef MESS
const char* crcfile;
const char* pcrcfile;
#endif

/**
 * Check if the game is working perfectly.
 */
adv_bool mame_game_working(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;

	unsigned mask = GAME_NOT_WORKING
		| GAME_UNEMULATED_PROTECTION
		| GAME_WRONG_COLORS
		| GAME_NO_SOUND
		| GAME_IMPERFECT_COLORS
		| GAME_IMPERFECT_SOUND
		| GAME_IMPERFECT_GRAPHICS;

	if ((driver->flags & mask) != 0)
		return 0;

	return 1;
}

const char* mame_game_resolution(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;
	machine_config machine;
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
	const game_driver* driver = (const game_driver*)game;
	machine_config machine;
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
	const game_driver* driver = (const game_driver*)game;
	machine_config machine;
	expand_machine_driver(driver->drv, &machine);
	return machine.frames_per_second;
}

unsigned mame_game_orientation(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;
	unsigned orientation = 0;

	if ((driver->flags & ORIENTATION_SWAP_XY) != 0)
		orientation |= OSD_ORIENTATION_SWAP_XY;
	if ((driver->flags & ORIENTATION_FLIP_X) != 0)
		orientation |= OSD_ORIENTATION_FLIP_X;
	if ((driver->flags & ORIENTATION_FLIP_Y) != 0)
		orientation |= OSD_ORIENTATION_FLIP_Y;

	return orientation;
}

/**
 * Return the game name.
 * \return
 * - != 0 Game name.
 * - == 0 Game name not found.
 */
const char* mame_game_name(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;

	if (strcmp(driver->name, "root") == 0)
		return 0;

	return driver->name;
}

/**
 * Return the game description.
 * \return Game description in free format.
 */
const char* mame_game_description(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;

	if (strcmp(driver->name, "root") == 0)
		return "";

	if (!driver->description)
		return "";

	return driver->description;
}

/**
 * Return the game language.
 * \return Game language string in free format.
 */
const char* mame_game_lang(const mame_game* game)
{
	const char* description;
	const char* lang;

	description = mame_game_description(game);

	lang = strchr(description, '(');
	if (!lang)
		return "";

	return lang;
}

/**
 * Return the composed GAME[SOFTWARE] name.
 * \return
 * - != 0 Composed name.
 * - == 0 Software name not found.
 */
const char* mame_software_name(const mame_game* game, adv_conf* context)
{
#ifdef MESS
	const char** i;
	char buffer[256];

	i = DEVICES;
	while (*i) {
		adv_conf_iterator j;
		char software_buffer[256];
		const char* s;

		snprintf(buffer, sizeof(buffer), "dev_%s", *i);

		s = 0;
		conf_iterator_begin(&j, context, buffer);
		while (!conf_iterator_is_end(&j)) {
			const char* arg;
			int p;
			char c;

			arg = conf_iterator_string_get(&j);

			sncpy(software_buffer, sizeof(software_buffer), arg);

			/* convert user input to lower case */
			for(p=0;software_buffer[p];++p)
				software_buffer[p] = tolower(software_buffer[p]);

			p = 0;
			s = stoken(&c, &p, software_buffer, ".=", "");

			if (s && s[0]) {
				break;
			}

			conf_iterator_next(&j);
		}

		if (!conf_iterator_is_end(&j)) {
			const char* n = mame_game_name(game);
			if (s && s[0] && n && n[0]) {
				char name_buffer[256];
				mame_name_adjust(name_buffer, sizeof(name_buffer), s);
				snprintf(GLUE.software_buffer, sizeof(GLUE.software_buffer), "%s[%s]", n, name_buffer);
				return GLUE.software_buffer;
			}
		}

		++i;
	}

	return 0;
#else
	return 0;
#endif
}

/**
 * Return the default option section name to use.
 */
const char* mame_section_name(const mame_game* game, adv_conf* context)
{
	const char* s;

	s = mame_software_name(game, context);
	if (s==0 || s[0]==0)
		s = mame_game_name(game);

	if (s==0 || s[0]==0)
		s = "";

	return s;
}

/**
 * Return the parent game.
 * \return
 * - == 0 No parent game present.
 * - != 0 Parent game.
 */
const mame_game* mame_game_parent(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;
	const game_driver* clone_of = driver_get_clone(driver);

	if (clone_of!=0 && clone_of->name!=0 && clone_of->name[0]!=0)
		return (const mame_game*)clone_of;
	else
		return 0;
}

/**
 * Return the game manufacturer.
 * \return Game manufacturer in free format.
 */
const char* mame_game_manufacturer(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;

	if (!driver->manufacturer)
		return "";

	return driver->manufacturer;
}

/**
 * Return the game year.
 * \return Game year in free format.
 */
const char* mame_game_year(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;

	if (!driver->year)
		return "";

	return driver->year;
}

/**
 * Return the game number of player.
 * \return Max number of player of the game.
 */
unsigned mame_game_players(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;
	input_port_entry* input;
	int nplayer;

	/* memory management for input_port_allocate */
	begin_resource_tracking();

	/* create the input port list */
	input = input_port_allocate(driver->construct_ipt, NULL);

	nplayer = 1;
	while (input->type != IPT_END) {
		if (input->player + 1 > nplayer)
			nplayer = input->player + 1;
		++input;
	}

	/* memory management for input_port_allocate */
	end_resource_tracking();

	return nplayer;
}

/**
 * Return the game control string.
 * \return A string like "joy4way" or 0 if unknown.
 */
const char* mame_game_control(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;
	const input_port_entry* input;
	const char* control = 0;

	/* memory management for input_port_allocate */
	begin_resource_tracking();

	/* create the input port list */
	input = input_port_allocate(driver->construct_ipt, NULL);

	while (input->type != IPT_END) {
		switch (input->type) {
		case IPT_JOYSTICK_UP :
		case IPT_JOYSTICK_DOWN :
		case IPT_JOYSTICK_LEFT :
		case IPT_JOYSTICK_RIGHT :
			if (!control) {
				if (input->four_way)
					control = "joy4way";
				else
					control = "joy8way";
			}
			break;
		case IPT_JOYSTICKRIGHT_UP :
		case IPT_JOYSTICKRIGHT_DOWN :
		case IPT_JOYSTICKRIGHT_LEFT :
		case IPT_JOYSTICKRIGHT_RIGHT :
		case IPT_JOYSTICKLEFT_UP :
		case IPT_JOYSTICKLEFT_DOWN :
		case IPT_JOYSTICKLEFT_LEFT :
		case IPT_JOYSTICKLEFT_RIGHT :
			if (!control) {
				if (input->four_way)
					control = "doublejoy4way";
				else
					control = "doublejoy8way";
			}
			break;
		case IPT_PADDLE  :
		case IPT_PADDLE_V  :
			if (!control) {
				control = "paddle";
			}
			break;
		case IPT_DIAL :
		case IPT_DIAL_V :
			if (!control) {
				control = "dial";
			}
			break;
		case IPT_TRACKBALL_X :
		case IPT_TRACKBALL_Y :
			if (!control) {
				control = "trackball";
			}
			break;
		case IPT_AD_STICK_X :
		case IPT_AD_STICK_Y :
		case IPT_AD_STICK_Z :
			if (!control) {
				control = "stick";
			}
			break;
		case IPT_LIGHTGUN_X :
		case IPT_LIGHTGUN_Y :
			if (!control) {
				control = "lightgun";
			}
			break;
		case IPT_MOUSE_X :
		case IPT_MOUSE_Y :
			if (!control) {
				control = "mouse";
			}
			break;
		}

		++input;
	}

	end_resource_tracking();

	return control;
}

/**
 * Return the game at the specified index position.
 * \return
 * - == 0 Last empty position.
 * - != 0 Game at the specified position.
 */
const mame_game* mame_game_at(unsigned i)
{
	return (const mame_game*)drivers[i];
}

/**
 * Print the information database in XML format.
 */
void mame_print_xml(FILE* out)
{
	cpuintrf_init();
	sndintrf_init();
	print_mame_xml(out, drivers);
}

/**
 * Check if a game use a vector display.
 */
adv_bool mame_is_game_vector(const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;
	machine_config machine;
	expand_machine_driver(driver->drv, &machine);
	return (machine.video_attributes & VIDEO_TYPE_VECTOR) != 0;
}

/**
 * Check if the specified game name is the game specified or a parent.
 */
adv_bool mame_is_game_relative(const char* relative, const mame_game* game)
{
	const game_driver* driver = (const game_driver*)game;
	while (driver && driver->name) {
		unsigned i;
		if (driver->name[0]!=0 && strcmp(driver->name, relative)==0)
			return 1;
		driver = driver_get_clone(driver);
	}
	return 0;
}

/**
 * Look for a game specification in a playback file.
 */
const struct mame_game* mame_playback_look(const char* file)
{
	inp_header ih;
	void* playback;
	osd_file_error error;

	playback = osd_fopen(FILETYPE_INPUTLOG, 0, file, "rb", &error);
	if (!playback) {
		target_err("Error opening the input playback file '%s'.\n", file);
		return 0;
	}

	/* read playback header */
	if (osd_fread(playback, &ih, sizeof(inp_header)) != sizeof(inp_header)) {
		osd_fclose(playback);
		target_err("Error reading the input playback file '%s'.\n", file);
		return 0;
	}

	osd_fclose(playback);

	if (!isalnum(ih.name[0])) {
		target_err("No game specified in the playback file '%s'.\n", file);
		return 0;
	} else {
		unsigned i;
		const struct mame_game* game = mame_game_at(0);

		for(i=0;game!=0;++i) {
			if (strcmp(mame_game_name(game), ih.name) == 0) {
				break;
			}
			game = mame_game_at(i+1);
		}

		if (!game) {
			target_err("Unknown game '%s' specified in the playback file.\n", ih.name);
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
	options.logfile = 0; /* use internal logging */
	options.mame_debug = advance->debug_flag;
	options.cheat = advance->cheat_flag;
	options.gui_host = 1; /* this prevents text mode messages that may stop the execution */
	options.skip_disclaimer = context->global.config.quiet_flag;
	options.skip_gameinfo = context->global.config.quiet_flag;
	options.skip_warnings = context->global.config.quiet_flag;
	options.samplerate = advance->samplerate;
	options.use_samples = advance->samples_flag;
	options.brightness = advance->brightness;
	options.pause_bright = context->global.config.pause_brightness;
	options.gamma = advance->gamma;
	options.vector_width = advance->vector_width;
	options.vector_height = advance->vector_height;
	options.ui_orientation = 0;
	if ((advance->ui_orientation & OSD_ORIENTATION_SWAP_XY) != 0)
		options.ui_orientation |= ORIENTATION_SWAP_XY;
	if ((advance->ui_orientation & OSD_ORIENTATION_FLIP_X) != 0)
		options.ui_orientation |= ORIENTATION_FLIP_X;
	if ((advance->ui_orientation & OSD_ORIENTATION_FLIP_Y) != 0)
		options.ui_orientation |= ORIENTATION_FLIP_Y;
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
	if (advance->savegame_file_buffer[0] != 0)
		options.savegame = advance->savegame_file_buffer;
	else
		options.savegame = 0; /* no savegame file to load */
	options.auto_save = 0; /* 1 to automatically save/restore at startup/quitting time */
	options.debug_width = advance->debug_width;
	options.debug_height = advance->debug_height;
	options.debug_depth = 8;
	options.controller = 0; /* no controller file to load */

	if (advance->bios_buffer[0] == 0 || strcmp(advance->bios_buffer, "default")==0)
		options.bios = 0;
	else
		options.bios = strdup(advance->bios_buffer);

	if (advance->language_file_buffer[0])
		options.language_file = mame_fopen(0, advance->language_file_buffer, FILETYPE_LANGUAGE, 0);
	else
		options.language_file = 0;

	if (advance->record_file_buffer[0]) {
		inp_header ih;

		log_std(("glue: opening record file %s\n", advance->record_file_buffer));

		options.record = mame_fopen(advance->record_file_buffer, 0, FILETYPE_INPUTLOG, 1);
		if (!options.record) {
			target_err("Error opening the input record file '%s'.\n", advance->record_file_buffer);
			return -1;
		}

		memset(&ih, 0, sizeof(inp_header));
		strncpy(ih.name, mame_game_name(advance->game), 8);

		ih.version[0] = 0;
		ih.version[1] = 0;
		ih.version[2] = 0;

		if (mame_fwrite(options.record, &ih, sizeof(inp_header)) != sizeof(inp_header)) {
			target_err("Error writing the input record file '%s'.\n", advance->record_file_buffer);
			return -1;
		}
	} else
		options.record = 0;

	if (advance->playback_file_buffer[0]) {
		inp_header ih;

		log_std(("glue: opening playback file %s\n", advance->playback_file_buffer));

		options.playback = mame_fopen(advance->playback_file_buffer, 0, FILETYPE_INPUTLOG, 0);
		if (!options.playback) {
			target_err("Error opening the input playback file '%s'.\n", advance->playback_file_buffer);
			return -1;
		}

		/* read playback header */
		if (mame_fread(options.playback, &ih, sizeof(inp_header)) != sizeof(inp_header)) {
			target_err("Error reading the input playback file '%s'.\n", advance->playback_file_buffer);
			return -1;
		}

		if (!isalnum(ih.name[0])) {
			/* without header */
			mame_fseek(options.playback, 0, SEEK_SET);
		} else {
			unsigned i;
			const struct mame_game* game = mame_game_at(0);

			for(i=0;game!=0;++i) {
				if (strcmp(mame_game_name(game), ih.name) == 0) {
					break;
				}
				game = mame_game_at(i+1);
			}

			if (!game) {
				target_err("Unknown game '%s' specified in the playback file.\n", ih.name);
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
	db_filename = advance->hiscore_file_buffer;

#ifdef MESS
	{
		const game_driver* driver = (const game_driver*)context->game;
		const game_driver* clone_of = driver_get_clone(driver);

		snprintf(GLUE.crc_file_buffer, sizeof(GLUE.crc_file_buffer), "%s%c%s.crc", advance->crc_dir_buffer, file_dir_slash(), driver->name);
		crcfile = GLUE.crc_file_buffer;

		log_std(("glue: file_crc %s\n", crcfile));

		if (clone_of
			&& clone_of->name
			&& (clone_of->flags & NOT_A_DRIVER) == 0) {
			snprintf(GLUE.parent_crc_file_buffer, sizeof(GLUE.parent_crc_file_buffer), "%s%c%s.crc", advance->crc_dir_buffer, file_dir_slash(), clone_of->name);
		} else {
			GLUE.parent_crc_file_buffer[0] = 0;
		}
		pcrcfile = GLUE.parent_crc_file_buffer;

		log_std(("glue: parent_file_crc %s\n", pcrcfile));
	}
#endif

	for(game_index=0;drivers[game_index];++game_index)
		if ((const game_driver*)context->game == drivers[game_index])
			break;
	assert(drivers[game_index] != 0);
	if (!drivers[game_index])
		return -1;

	GLUE.sound_speed = context->video.config.fps_speed_factor;
	GLUE.sound_fps = context->video.config.fps_fixed;

	hardware_script_info(mame_game_description(context->game), mame_game_manufacturer(context->game), mame_game_year(context->game), "Loading");

	r = run_game(game_index);

	if (options.bios) {
		free(options.bios);
		options.bios = 0;
	}
	if (options.language_file) {
		mame_fclose(options.language_file);
		options.language_file = 0;
	}
	if (options.record) {
		mame_fclose(options.record);
		options.record = 0;
	}
	if (options.playback) {
		mame_fclose(options.playback);
		options.playback = 0;
	}

	return r;
}

/**
 * Max number of port.
 */
#define GLUE_PORT_MAX 1024

/**
 * Single port.
 */
#define S(name, desc, NAME) \
	{ name, desc, MAME_PORT_UI(IPT_##NAME) },

/**
 * Unamed single port.
 */
#define SU(name, NAME) \
	{ name, name, MAME_PORT_UI(IPT_##NAME) },

/**
 * Player port.
 */
#define P(name, desc, NAME) \
	{ "p1_" name, "Player 1 " desc, MAME_PORT_PLAYER(IPT_##NAME, 1) }, \
	{ "p2_" name, "Player 2 " desc, MAME_PORT_PLAYER(IPT_##NAME, 2) }, \
	{ "p3_" name, "Player 3 " desc, MAME_PORT_PLAYER(IPT_##NAME, 3) }, \
	{ "p4_" name, "Player 4 " desc, MAME_PORT_PLAYER(IPT_##NAME, 4) },

/**
 * Player port with double keyboard sequence.
 */
#define PE(name1, name2, desc1, desc2, NAME) \
	{ "p1_" name1, "Player 1 " desc1, MAME_PORT_INDEX(IPT_##NAME, 1, 0) }, \
	{ "p1_" name2, "Player 1 " desc2, MAME_PORT_INDEX(IPT_##NAME, 1, 1) }, \
	{ "p2_" name1, "Player 2 " desc1, MAME_PORT_INDEX(IPT_##NAME, 2, 0) }, \
	{ "p2_" name2, "Player 2 " desc2, MAME_PORT_INDEX(IPT_##NAME, 2, 1) }, \
	{ "p3_" name1, "Player 3 " desc1, MAME_PORT_INDEX(IPT_##NAME, 3, 0) }, \
	{ "p3_" name2, "Player 3 " desc2, MAME_PORT_INDEX(IPT_##NAME, 3, 1) }, \
	{ "p4_" name1, "Player 4 " desc1, MAME_PORT_INDEX(IPT_##NAME, 4, 0) }, \
	{ "p4_" name2, "Player 4 " desc2, MAME_PORT_INDEX(IPT_##NAME, 4, 1) },

#define K(name) \
	{ "key_" name, "Key " name, { name } },

#define KR1(name, re1) \
	{ "key_" name, "Key " name, { re1 } },

#define KR2(name, re1, re2) \
	{ "key_" name, "Key " name, { re1, re2 } },

#define KR3(name, re1, re2, re3) \
	{ "key_" name, "Key " name, { re1, re2, re3 } },

#define KR4(name, re1, re2, re3, re4) \
	{ "key_" name, "Key " name, { re1, re2, re3, re4 } },

#define KR5(name, re1, re2, re3, re4, re5) \
	{ "key_" name, "Key " name, { re1, re2, re3, re4, re5 } },

#ifdef MESS
struct glue_keyboard_name {
	const char* name;
	char desc[64];
	const char* glob[8];
};

/**
 * Keyboard name definitions.
 * This list defines some common names used in keyboards.
 * Note that the order of definition is relevant on the recognition of
 * the key from it's free form description on the MESS driver port list.
 */
static struct glue_keyboard_name GLUE_KEYBOARD_STD[] = {

	K("q")
	K("w")
	K("e")
	K("r")
	K("t")
	K("y")
	K("u")
	K("i")
	K("o")
	K("p")
	K("a")
	K("s")
	K("d")
	K("f")
	K("g")
	K("h")
	K("j")
	K("k")
	K("l")
	K("z")
	K("x")
	K("c")
	K("v")
	K("b")
	K("n")
	K("m")

	KR4("pad_0", "keypad*0", "0*keypad?", "0*kp?", "kp*0")
	KR4("pad_1", "keypad*1", "1*keypad?", "1*kp?", "kp*1")
	KR4("pad_2", "keypad*2", "2*keypad?", "2*kp?", "kp*2")
	KR4("pad_3", "keypad*3", "3*keypad?", "3*kp?", "kp*3")
	KR4("pad_4", "keypad*4", "4*keypad?", "4*kp?", "kp*4")
	KR4("pad_5", "keypad*5", "5*keypad?", "5*kp?", "kp*5")
	KR4("pad_6", "keypad*6", "6*keypad?", "6*kp?", "kp*6")
	KR4("pad_7", "keypad*7", "7*keypad?", "7*kp?", "kp*7")
	KR4("pad_8", "keypad*8", "8*keypad?", "8*kp?", "kp*8")
	KR4("pad_9", "keypad*9", "9*keypad?", "9*kp?", "kp*9")
	KR4("pad_enter", "keypad*enter", "enter*keypad?", "enter*kp?", "kp*enter")
	KR4("pad_minus", "keypad*-", "-*keypad?", "-*kp?", "kp*-")
	KR4("pad_plus", "keypad*+", "+*keypad?", "+*kp?", "kp*+")
	KR4("pad_slash", "keypad*/", "/*keypad?", "/*kp?", "kp*/")
	KR4("pad_colon", "keypad*.", ".*keypad?", ".*kp?", "kp*.")
	KR4("pad_diesis", "keypad*#", "#*keypad?", "#*kp?", "kp*#")
	KR4("pad_asterisk", "keypad*\\*", "\\**keypad?", "\\**kp?", "kp*\\*")

	KR1("0", "0")
	KR1("1", "1")
	KR1("2", "2")
	KR1("3", "3")
	KR1("4", "4")
	KR1("5", "5")
	KR1("6", "6")
	KR1("7", "7")
	KR1("8", "8")
	KR1("9", "9")

	KR2("esc", "esc", "escape")
	KR2("enter", "enter", "return")
	KR2("backspace", "back*space", "clr")
	K("tab")
	KR2("space", "space", "?space?")
	KR2("ins", "ins", "insert")
	KR2("del", "del", "delete")
	K("home")
	K("end")
	K("fctn")
	K("restore")
	K("store")
	K("play")
	K("print")
	K("hold")
	K("rew")
	K("record")
	KR2("break", "break", "brk")
	K("graph")
	K("pause")
	K("menu")
	K("stop")
	K("again")
	K("undo")
	K("move")
	K("copy")
	K("open")
	K("edit")
	K("paste")
	K("find")
	K("cut")
	K("help")
	K("back")
	K("forward")
	KR2("capslock", "caps*lock", "shift*lock")
	KR1("scrlock", "scroll*lock")
	KR1("numlock", "num*lock")
	K("quickload")
	KR2("pgup", "pgup", "page*up")
	KR2("pgdn", "pgdn", "page*down")

	KR2("backquote", "backquote", "`")
	KR2("minus", "minus", "-")
	KR2("plus", "plus", "+")
	KR2("asterisk", "asterisk", "\\*")
	KR2("equals", "equals", "=")
	KR2("openbrace", "openbrace", "(")
	KR2("closebrace", "closebrace", ")")
	KR2("semicolon", "semicolon", ";")
	KR2("quote", "quote", "'")
	KR2("backslash", "backslash", "\\\\")
	KR2("less", "less", "<")
	KR2("comma", "comma", ",")
	KR2("period", "period", ".")
	KR2("slash", "slash", "/")
	KR2("colon", "colon", ":")
	KR2("pound", "pound", "£")
	KR2("doublequote", "doublequote", "\"")
	KR2("diesis", "diessi", "#")

	KR3("lshift", "lshift", "left?shift", "shift?left")
	KR3("rshift", "rshift", "right?shift", "shift?right")
	KR4("lctrl", "lctrl", "lcontrol", "left?ctrl", "ctrl?left")
	KR4("rctrl", "rctrl", "rcontrol", "right?ctrl", "ctrl?right")
	KR3("lalt", "lalt", "left?alt", "alt?left")
	KR3("ralt", "ralt", "right?alt", "alt?right")
	KR2("ctrl", "ctrl", "control")
	K("alt")
	K("shift")

	/* it's safe to have directions as last entries to prevent incorrect */
	/* recognition of "ctrl left" or similar */
	KR2("left", "left", "cursor*left")
	KR2("right", "right", "cursor*right")
	KR2("up", "up", "cursor*up")
	KR2("down", "down", "cursor*down")

	K("f1")
	K("f2")
	K("f3")
	K("f4")
	K("f5")
	K("f6")
	K("f7")
	K("f8")
	K("f9")
	K("f10")
	K("f11")
	K("f12")
	K("f13")
	K("f14")
	K("f15")
	K("f16")
	K("f17")
	K("f18")
	K("f19")
	K("f20")
	K("f21")
	K("f22")
	K("f23")
	K("f24")
	{ 0 }
};
#endif

/**
 * Input ports.
 * The port number are in a special uniqe format which allow to have different
 * values for port which requires a double specification.
 */
static struct mame_port GLUE_PORT[GLUE_PORT_MAX];

/**
 * Constant input ports.
 */
static struct mame_port GLUE_PORT_STD[] = {

	P("up", "Up", JOYSTICK_UP)
	P("down", "Down", JOYSTICK_DOWN)
	P("left", "Left", JOYSTICK_LEFT)
	P("right", "Right", JOYSTICK_RIGHT)

	P("doubleright_up", "Double Right Up", JOYSTICKRIGHT_UP)
	P("doubleright_down", "Double Right Down", JOYSTICKRIGHT_DOWN)
	P("doubleright_left", "Double Right Left", JOYSTICKRIGHT_LEFT)
	P("doubleright_right", "Double Right Right", JOYSTICKRIGHT_RIGHT)
	P("doubleleft_up", "Double Left Up", JOYSTICKLEFT_UP)
	P("doubleleft_down", "Double Left Down", JOYSTICKLEFT_DOWN)
	P("doubleleft_left", "Double Left Left", JOYSTICKLEFT_LEFT)
	P("doubleleft_right", "Double Left Right", JOYSTICKLEFT_RIGHT)

	PE("paddle_left", "paddle_right", "Paddle Left", "Paddle Right", PADDLE)
	PE("paddle_up", "paddle_down", "Paddle Up", "Paddle Down", PADDLE_V)

	PE("stick_left", "stick_right", "Stick Left", "Stick Right", AD_STICK_X)
	PE("stick_up", "stick_down", "Stick Up", "Stick Down", AD_STICK_Y)
	PE("stick_forward", "stick_backward",  "Stick Forward", "Stick Backward", AD_STICK_Z)

	PE("lightgun_left", "lightgun_right", "Lightgun Left", "Lightgun Right", LIGHTGUN_X)
	PE("lightgun_up", "lightgun_down", "Lightgun Up", "Lightgun Down", LIGHTGUN_Y)

	PE("pedalgas_push", "pedalgas_release", "Pedal Gas", "Pedal Gas Release", PEDAL)
	PE("pedalbrake_push", "pedalbrake_release", "Pedal Brake", "Pedal Brake Release", PEDAL2)
	PE("pedalother_push", "pedalother_release", "Pedal Other", "Pedal Other Release", PEDAL3)

	PE("dial_left", "dial_right", "Dial Left", "Dial Right", DIAL)
	PE("dial_up", "dial_down", "Dial Up", "Dial Down", DIAL_V)

	PE("trackball_left", "trackball_right", "Trackball Left", "Trackball Right", TRACKBALL_X)
	PE("trackball_up", "trackball_down", "Trackball Up", "Trackball Down", TRACKBALL_Y)

	PE("mouse_left", "mouse_right", "Mouse Left", "Mouse Right", MOUSE_X)
	PE("mouse_up", "mouse_down", "Mouse Up", "Mouse Down", MOUSE_Y)

	P("button1", "Button 1", BUTTON1)
	P("button2", "Button 2", BUTTON2)
	P("button3", "Button 3", BUTTON3)
	P("button4", "Button 4", BUTTON4)
	P("button5", "Button 5", BUTTON5)
	P("button6", "Button 6", BUTTON6)
	P("button7", "Button 7", BUTTON7)
	P("button8", "Button 8", BUTTON8)
	P("button9", "Button 9", BUTTON9)
	P("button10", "Button 10", BUTTON10)

	S("start1", "Start 1 Player", START1)
	S("start2", "Start 2 Players", START2)
	S("start3", "Start 3 Players", START3)
	S("start4", "Start 4 Players", START4)
	S("start5", "Start 5 Players", START5)
	S("start6", "Start 6 Players", START6)
	S("start7", "Start 7 Players", START7)
	S("start8", "Start 8 Players", START8)

	S("coin1", "Player 1 Coin", COIN1)
	S("coin2", "Player 2 Coin", COIN2)
	S("coin3", "Player 3 Coin", COIN3)
	S("coin4", "Player 4 Coin", COIN4)
	S("coin5", "Player 5 Coin", COIN1)
	S("coin6", "Player 6 Coin", COIN2)
	S("coin7", "Player 7 Coin", COIN3)
	S("coin8", "Player 8 Coin", COIN4)
	S("bill1", "Player 1 Bill", BILL1)

	S("service_coin1", "Service 1 Coin", SERVICE1)
	S("service_coin2", "Service 2 Coin", SERVICE2)
	S("service_coin3", "Service 3 Coin", SERVICE3)
	S("service_coin4", "Service 4 Coin", SERVICE4)
	S("service_coin5", "Service 5 Coin", SERVICE1)
	S("service_coin6", "Service 6 Coin", SERVICE2)
	S("service_coin7", "Service 7 Coin", SERVICE3)
	S("service_coin8", "Service 8 Coin", SERVICE4)

	S("service", "Service", SERVICE)
	S("tilt", "Tilt", TILT)
	S("interlock", "Door Interlock", INTERLOCK)
	S("volume_up", "Volume Up", VOLUME_UP)
	S("volume_down", "Volume Down", VOLUME_DOWN)

	P("mahjong_a", "Mahjong A", MAHJONG_A)
	P("mahjong_b", "Mahjong B", MAHJONG_B)
	P("mahjong_c", "Mahjong C", MAHJONG_C)
	P("mahjong_d", "Mahjong D", MAHJONG_D)
	P("mahjong_e", "Mahjong E", MAHJONG_E)
	P("mahjong_f", "Mahjong F", MAHJONG_F)
	P("mahjong_g", "Mahjong G", MAHJONG_G)
	P("mahjong_h", "Mahjong H", MAHJONG_H)
	P("mahjong_i", "Mahjong I", MAHJONG_I)
	P("mahjong_j", "Mahjong J", MAHJONG_J)
	P("mahjong_k", "Mahjong K", MAHJONG_K)
	P("mahjong_l", "Mahjong L", MAHJONG_L)
	P("mahjong_m", "Mahjong M", MAHJONG_M)
	P("mahjong_n", "Mahjong N", MAHJONG_N)
	P("mahjong_kan", "Mahjong Kan", MAHJONG_KAN)
	P("mahjong_pon", "Mahjong Pon", MAHJONG_PON)
	P("mahjong_chi", "Mahjong Chi", MAHJONG_CHI)
	P("mahjong_reach", "Mahjong Reach", MAHJONG_REACH)
	P("mahjong_ron", "Mahjong Ron", MAHJONG_RON)
	P("mahjong_bet", "Mahjong Bet", MAHJONG_BET)
	P("mahjong_chance", "Mahjong Last Chance", MAHJONG_LAST_CHANCE)
	P("mahjong_score", "Mahjong Score", MAHJONG_SCORE)
	P("mahjong_double_up", "Mahjong Double Up", MAHJONG_DOUBLE_UP)
	P("mahjong_flip_flop", "Mahjong Flip Flop", MAHJONG_FLIP_FLOP)

	/* MESS Specific */
#ifdef MESS
	P("start", "Start", START)
	P("select", "Select", SELECT)
#endif

	/* UI specific of AdvanceMAME */
	S("ui_mode_next", "Next Mode", UI_MODE_NEXT)
	S("ui_mode_pred", "Pred Mode", UI_MODE_PRED)
	S("ui_record_start", "Record Start", UI_RECORD_START)
	S("ui_record_stop", "Record Stop", UI_RECORD_STOP)
	S("ui_turbo", "Turbo", UI_TURBO)
	S("ui_cocktail", "Cocktail", UI_COCKTAIL)
	S("ui_help", "Help", UI_HELP)
	S("ui_startup", "Startup", UI_STARTUP_END)

	/* UI */
	S("ui_configure", "Configure", UI_CONFIGURE)
	S("ui_on_screen_display", "On Screen Display", UI_ON_SCREEN_DISPLAY)
	S("ui_pause", "Pause", UI_PAUSE)
	S("ui_reset_machine", "Reset", UI_RESET_MACHINE)
	S("ui_show_gfx", "Graphics", UI_SHOW_GFX)
	S("ui_frameskip_dec", "Frameskip Dev", UI_FRAMESKIP_DEC)
	S("ui_frameskip_inc", "Frameskip Inc", UI_FRAMESKIP_INC)
	S("ui_throttle", "Throttle", UI_THROTTLE)
	S("ui_show_fps", "Show FPS", UI_SHOW_FPS)
	S("ui_snapshot", "Snapshot", UI_SNAPSHOT)
	S("ui_toggle_cheat", "Cheat", UI_TOGGLE_CHEAT)
	S("ui_home", "UI Home", UI_HOME)
	S("ui_end", "UI End", UI_END)
	S("ui_up", "UI Up", UI_UP)
	S("ui_down", "UI Down", UI_DOWN)
	S("ui_left", "UI Left", UI_LEFT)
	S("ui_right", "UI Right", UI_RIGHT)
	S("ui_select", "UI Select", UI_SELECT)
	S("ui_cancel", "UI Cancel", UI_CANCEL)
	S("ui_pan_up", "Pan Up", UI_PAN_UP)
	S("ui_pan_down", "Pan Down", UI_PAN_DOWN)
	S("ui_pan_left", "Pan Left", UI_PAN_LEFT)
	S("ui_pan_right", "Pan Right", UI_PAN_RIGHT)
	S("ui_show_profiler", "Profiler", UI_SHOW_PROFILER)
	S("ui_toggle_ui", "User Interface", UI_TOGGLE_UI)
	S("ui_toggle_debug", "Debug", UI_TOGGLE_DEBUG)
	S("ui_save_state", "Save State", UI_SAVE_STATE)
	S("ui_load_state", "Load State", UI_LOAD_STATE)

	SU("ui_add_cheat", UI_ADD_CHEAT)
	SU("ui_delete_cheat", UI_DELETE_CHEAT)
	SU("ui_save_cheat", UI_SAVE_CHEAT)
	SU("ui_watch_value", UI_WATCH_VALUE)
	SU("ui_edit_cheat", UI_EDIT_CHEAT)
	SU("ui_toggle_crosshair", UI_TOGGLE_CROSSHAIR)

	/* specific of AdvanceMAME */
	SU("safequit", MAME_PORT_SAFEQUIT)
	SU("event1", MAME_PORT_EVENT1)
	SU("event2", MAME_PORT_EVENT2)
	SU("event3", MAME_PORT_EVENT3)
	SU("event4", MAME_PORT_EVENT4)
	SU("event5", MAME_PORT_EVENT5)
	SU("event6", MAME_PORT_EVENT6)
	SU("event7", MAME_PORT_EVENT7)
	SU("event8", MAME_PORT_EVENT8)
	SU("event9", MAME_PORT_EVENT9)
	SU("event10", MAME_PORT_EVENT10)
	SU("event11", MAME_PORT_EVENT11)
	SU("event12", MAME_PORT_EVENT12)
	SU("event13", MAME_PORT_EVENT13)
	SU("event14", MAME_PORT_EVENT14)

	{ 0, 0, 0 }
};

/**
 * Return the list of input port.
 * It's a list in the uniq format, not the MAME format.
 */
struct mame_port* mame_port_list(void)
{
	return GLUE_PORT;
}

struct mame_port* mame_port_find(unsigned port)
{
	struct mame_port* i;
	for(i=mame_port_list();i->name;++i)
		if (i->port == port)
			return i;
	return 0;
}

/**
 * Return the player number of a unique port.
 * \param port Port type. It's a port in the unique format, not the mame format.
 * \return 0 for global port, 1 for player 1 and so on...
 */
int mame_port_player(unsigned port)
{
	unsigned type = MAME_PORT_TYPE_GET(port);
	unsigned player = MAME_PORT_PLAYER_GET(port);

	switch (type) {
	case IPT_START1 : return 1;
	case IPT_START2 : return 2;
	case IPT_START3 : return 3;
	case IPT_START4 : return 4;
	case IPT_START5 : return 5;
	case IPT_START6 : return 6;
	case IPT_START7 : return 7;
	case IPT_START8 : return 8;
	case IPT_COIN1 : return 1;
	case IPT_COIN2 : return 2;
	case IPT_COIN3 : return 3;
	case IPT_COIN4 : return 4;
	case IPT_COIN5 : return 5;
	case IPT_COIN6 : return 6;
	case IPT_COIN7 : return 7;
	case IPT_COIN8 : return 8;
	case IPT_BILL1 : return 1;
	}

	return player;
}

/**
 * Convert a sequence from the MAME format to the internal format.
 */
void glue_seq_convert(unsigned* mame_seq, unsigned mame_max, unsigned* seq, unsigned max)
{
	unsigned j, i;
	adv_bool prev_or;
	adv_bool prev_not;

	j = 0;
	i = 0;
	prev_or = 0;
	prev_not = 0;

	while (j<mame_max && mame_seq[j] != CODE_NONE) {
		unsigned c = DIGITAL_SPECIAL_NONE;
		adv_bool c_set;

		switch (mame_seq[j]) {
		case CODE_OR :
			prev_or = 1; /* implicitely remove consecutive or */
			c_set = 0;
			break;
		case CODE_NOT :
			prev_not = 1;
			c_set = 0;
			break;
		case CODE_DEFAULT :
			c = DIGITAL_SPECIAL_AUTO;
			c_set = 1;
			break;
		default:
			c = code_to_oscode(mame_seq[j]);
			if (c != 0) {
				c_set = 1;
			} else {
				c_set = 0;
				prev_not = 0; /* remove a code specific not */
				log_std(("WARNING:glue: unable to convert MAME code %d\n", mame_seq[j]));
			}
			break;
		}
		if (c_set) {
			if (prev_or) {
				if (i<max) {
					seq[i] = DIGITAL_SPECIAL_OR;
					++i;
				}
				prev_or = 0;
			}
			if (prev_not) {
				if (i<max) {
					seq[i] = DIGITAL_SPECIAL_NOT;
					++i;
				}
				prev_not = 0;
			}
			if (i<max) {
				seq[i] = c;
				++i;
			}
		}
		++j;
	}

	while (i<max) {
		seq[i] = DIGITAL_SPECIAL_NONE;
		++i;
	}
}

/**
 * Convert a sequence to the MAME format from the internal format.
 */
void glue_seq_convertback(unsigned* seq, unsigned max, unsigned* mame_seq, unsigned mame_max)
{
	unsigned j, i;
	adv_bool prev_or;
	adv_bool prev_not;

	j = 0;
	i = 0;
	prev_or = 0;
	prev_not = 0;

	while (j<max && seq[j] != DIGITAL_SPECIAL_NONE) {
		unsigned c = CODE_NONE;
		adv_bool c_set;

		switch (seq[j]) {
		case DIGITAL_SPECIAL_OR :
			prev_or = 1; /* implicitely remove consecutive or */
			c_set = 0;
			break;
		case DIGITAL_SPECIAL_NOT :
			prev_not = 1;
			c_set = 0;
			break;
		case DIGITAL_SPECIAL_AUTO :
			c = CODE_DEFAULT;
			c_set = 1;
			break;
		default:
			c = oscode_to_code(seq[j]);
			/* ignore unmapped codes, it happen if the configuration */
			/* file refers at a control now removed */
			if (c != CODE_NONE) {
				c_set = 1;
			} else {
				c_set = 0;
				prev_not = 0; /* remove a code specific not */
				log_std(("WARNING:glue: unable to convert OSD code %d\n", seq[j]));
			}
			break;
		}
		if (c_set) {
			if (prev_or) {
				if (i<mame_max) {
					mame_seq[i] = CODE_OR;
					++i;
				}
				prev_or = 0;
			}
			if (prev_not) {
				if (i<mame_max) {
					mame_seq[i] = CODE_NOT;
					++i;
				}
				prev_not = 0;
			}
			if (i<mame_max) {
				mame_seq[i] = c;
				++i;
			}
		}
		++j;
	}

	while (i<mame_max) {
		mame_seq[i] = CODE_NONE;
		++i;
	}
}

#ifdef MESS
static unsigned glue_keyboard_find(const char* const_name)
{
	int i;
	char name_buffer[128];
	char* name;
	char* s;
	unsigned name_len;
	unsigned j, k;

	if (!const_name) {
		log_std(("ERROR:glue: keyboard port without a name\n"));
		return 0;
	}

	/* duplicate */
	sncpy(name_buffer, sizeof(name_buffer), const_name);
	name = name_buffer;

	/* comparison is in lower case */
	strlwr(name);

	/* HACK for MESS c128 driver */
	if (strncmp(name, "(64)", 4) == 0)
		name += 4; /* remove starting "(64)" string */

	/* HACK for MESS coleco driver */
	s = strstr(name, "(pad 1)"); /* remove ending "(pad 1)" string */
	if (s && strlen(s) == 7)
		strcpy(s, "");
	s = strstr(name, "(pad 2)"); /* substitute ending "(pad 2)" string */
	if (s && strlen(s) == 7)
		strcpy(s, " keypad");

	/* search for exact match */
	for(j=0;GLUE_KEYBOARD_STD[j].name;++j) {
		for(k=0;GLUE_KEYBOARD_STD[j].glob[k];++k) {
			if (sglob(name_buffer, GLUE_KEYBOARD_STD[j].glob[k])) {
				log_std(("glue: map key '%s' to control '%s' (exact)\n", name, GLUE_KEYBOARD_STD[j].name));
				return MAME_PORT_KEYBOARD(j);
			}
		}
	}

	/* search for a partial match */
	i = 0;
	name_len = strlen(name_buffer);
	while (i < name_len) {
		char c;
		const char* t;

		sskip(&i, name_buffer, " ");
		t = stoken(&c, &i, name_buffer, " ",  "");

		for(j=0;GLUE_KEYBOARD_STD[j].name;++j) {
			for(k=0;GLUE_KEYBOARD_STD[j].glob[k];++k) {
				if (sglob(t, GLUE_KEYBOARD_STD[j].glob[k])) {
					log_std(("glue: map key '%s' to control '%s' (partial)\n", name, GLUE_KEYBOARD_STD[j].name));
					return MAME_PORT_KEYBOARD(j);
				}
			}
		}
	}

	log_std(("WARNING:glue: keyboard port '%s' not recognized\n", name));

	return 0;
}
#endif

/**
 * Begin index of sequences for a MAME port type.
 */
unsigned glue_port_seq_begin(unsigned type)
{
	return 0;
}

/**
 * End index of sequences for a MAME port type.
 */
unsigned glue_port_seq_end(unsigned type)
{
	if (port_type_is_analog(type))
		return 2;
	else
		return 1;
}

/**
 * Return the sequence type of the specified index.
 * \param type Type of the port.
 * \param index Index of the port. The index must be from glue_port_seq_begin() to glue_port_seq_end() - 1;
 */
int glue_port_seqtype(unsigned type, unsigned index)
{
	if (port_type_is_analog(type)) {
		switch (index) {
		case 0 : return SEQ_TYPE_INCREMENT;
		case 1 : return SEQ_TYPE_DECREMENT;
		}
	} else {
		return SEQ_TYPE_STANDARD;
	}

	return 0;
}

/**
 * Return the sequence structure of the specified type.
 * \param port Input port.
 * \param seqtype Sequence type.
 * \return Sequence pointer, or 0 if not available.
 */
input_seq* glue_portdef_seq_get(input_port_default_entry* port, int seqtype)
{
	switch (seqtype) {
	case SEQ_TYPE_STANDARD :
		return &port->defaultseq;
	case SEQ_TYPE_INCREMENT :
		if (port_type_is_analog(port->type))
			return &port->defaultincseq;
		break;
	case SEQ_TYPE_DECREMENT :
		if (port_type_is_analog(port->type))
			return &port->defaultdecseq;
		break;
	}

	return 0;
}

/**
 * Return the sequence structure of the specified type.
 * \param port Input port.
 * \param seqtype Sequence type.
 * \return Sequence pointer, or 0 if not available.
 */
input_seq* glue_port_seq_get(input_port_entry* port, int seqtype)
{
	switch (seqtype) {
	case SEQ_TYPE_STANDARD:
		return &port->seq;
	case SEQ_TYPE_INCREMENT:
		if (port_type_is_analog(port->type))
			return &port->analog.incseq;
		break;
	case SEQ_TYPE_DECREMENT:
		if (port_type_is_analog(port->type))
			return &port->analog.decseq;
		break;
	}

	return 0;
}

/**
 * Return the final evaulation of the sequence structure of the specified type.
 * This structure evaluate any DEFAULT or special code and return the
 * sequence ready for call seq_pressed().
 * \param port Input port.
 * \param seqtype Sequence type.
 * \return Sequence pointer. It's always available.
 */
input_seq* glue_portdef_seqeval_get(input_port_default_entry* port, int seqtype)
{
	return input_port_default_seq(port->type, port->player, seqtype);
}

/**
 * Return the final evaulation of the sequence structure of the specified type.
 * This structure evaluate any DEFAULT or special code and return the
 * sequence ready for call seq_pressed().
 * \param port Input port.
 * \param seqtype Sequence type.
 * \return Sequence pointer, or 0 if not available.
 */
input_seq* glue_port_seqeval_get(input_port_entry* port, int seqtype)
{
	return input_port_seq(port, seqtype);
}

adv_bool glue_is_portplayer(unsigned type)
{
	/* MAME is'nt able to differentiate from player 1 ports and global ports; */
	/* both have the player number 0 in the input_port_entry and input_port_default_entry vectors */
	if ((type >= __ipt_digital_joystick_start && type <= __ipt_digital_joystick_end)
		|| (type >= __ipt_analog_start && type <= __ipt_analog_end)
		|| (type >= IPT_BUTTON1 && type <= IPT_BUTTON10)
		|| (type >= IPT_MAHJONG_A && type <= IPT_MAHJONG_SMALL)
		|| (type == IPT_SELECT) /* MESS specific */
		|| (type == IPT_START) /* MESS specific */
	) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * Convert a MAME port value to the unique format.
 * \param type Type of the port. One of the IPT_* values.
 * \param player Port player.
 * \param seqtype Port squence type. One of SEQ_TYPE_* defines.
 * \param name Name of the port. Used to recognize keyboard port for mess.
 */
unsigned glue_port_convert(unsigned type, unsigned player, unsigned seqtype, const char* name)
{
	unsigned index;

#ifdef MESS
	/* keyboard ports have a special recongnition */
	if (type == IPT_KEYBOARD) {
		return glue_keyboard_find(name);
	}
#endif

	if (port_type_is_analog(type)) {
		switch (seqtype) {
		case SEQ_TYPE_STANDARD : /* used for the mame_analog port */
		case SEQ_TYPE_INCREMENT :
			index = 0;
			break;
		case SEQ_TYPE_DECREMENT :
			index = 1;
			break;
		default:
			log_std(("ERROR:glue: convert unsupported SEQ_TYPE_ constant %d\n", seqtype));
			return 0;
		}
	} else {
		switch (seqtype) {
		case SEQ_TYPE_STANDARD :
			index = 0;
			break;
		default:
			log_std(("ERROR:glue: convert unsupported SEQ_TYPE_ constant %d\n", seqtype));
			return 0;
		}
	}

	if (glue_is_portplayer(type)) {
		/* it's a player port */
		player += 1;
	} else {
		if (player != 0) {
			log_std(("ERROR:glue: convert unexpected player specification for port type %d\n", type));
		}
		/* it's a global port */
		player = 0;
	}

	return MAME_PORT_INDEX(type, player, index);
}

/**
 * Convert a port from the unique format to the MAME format.
 * The conversion works only for global ports. Any game specific
 * port is ignored.
 * \param port Unique port to convert.
 * \param seqtype Resulting sequence type.
 * \return Pointer at the mame port found or 0.
 */
input_port_default_entry* glue_port_convertback(unsigned port, int* seqtype)
{
	input_port_default_entry* j;
	unsigned type;
	unsigned player;
	unsigned index;

	type = MAME_PORT_TYPE_GET(port);

	player = MAME_PORT_PLAYER_GET(port);
	if (glue_is_portplayer(type)) {
		if (player == 0) {
			log_std(("ERROR:glue: convertback missing player specification for port type %d\n", type));
		}
		/* it's a player port */
		player -= 1;
	} else {
		if (player != 0) {
			log_std(("ERROR:glue: convertback unexpected player specification for port type %d\n", type));
		}
		/* it's a global port */
		player = 0;
	}

	index = MAME_PORT_INDEX_GET(port);

	*seqtype = glue_port_seqtype(type, index);

	j = get_input_port_list();
	while (j->type != IPT_END) {
		if (j->type == type && j->player == player) {
			return j;
		}
		++j;
	}

	return 0;
}

/**
 * List of default port types reported at the user interface commmands.
 * These are the MAME user interface port normally not defined in the game driver.
 */
static unsigned PORT_TYPE_REPORT_DEFAULT[] = {
	IPT_UI_MODE_NEXT,
	IPT_UI_MODE_PRED,
	IPT_UI_RECORD_START,
	IPT_UI_RECORD_STOP,
	IPT_UI_TURBO,
	IPT_UI_COCKTAIL,
	IPT_UI_HELP,
	IPT_UI_STARTUP_END,

	IPT_UI_CONFIGURE,
	IPT_UI_ON_SCREEN_DISPLAY,
	IPT_UI_PAUSE,
	IPT_UI_RESET_MACHINE,
	/* IPT_UI_SHOW_GFX, */
	IPT_UI_FRAMESKIP_DEC,
	IPT_UI_FRAMESKIP_INC,
	IPT_UI_THROTTLE,
	IPT_UI_SHOW_FPS,
	IPT_UI_SNAPSHOT,
	IPT_UI_TOGGLE_CHEAT,
	IPT_UI_HOME,
	IPT_UI_END,
	IPT_UI_UP,
	IPT_UI_DOWN,
	IPT_UI_LEFT,
	IPT_UI_RIGHT,
	IPT_UI_SELECT,
	IPT_UI_CANCEL,
	IPT_UI_PAN_UP,
	IPT_UI_PAN_DOWN,
	IPT_UI_PAN_LEFT,
	IPT_UI_PAN_RIGHT,
	/* IPT_UI_SHOW_PROFILER, */
	IPT_UI_TOGGLE_UI,
	/* IPT_UI_TOGGLE_DEBUG, */
	IPT_UI_SAVE_STATE,
	IPT_UI_LOAD_STATE,
	/* IPT_UI_ADD_CHEAT, */
	/* IPT_UI_DELETE_CHEAT, */
	/* IPT_UI_SAVE_CHEAT, */
	/* IPT_UI_WATCH_VALUE, */
	/* IPT_UI_EDIT_CHEAT, */
	IPT_UI_TOGGLE_CROSSHAIR,
	0
};

/**
 * List of game port types reported at the user interface commmands.
 * These are the game specific ports defined in the game driver.
 */
static unsigned PORT_TYPE_REPORT_GAME[] = {
#ifdef MESS
	IPT_START,
	IPT_SELECT,
#endif

	IPT_JOYSTICK_UP,
	IPT_JOYSTICK_DOWN,
	IPT_JOYSTICK_LEFT,
	IPT_JOYSTICK_RIGHT,
	IPT_JOYSTICKRIGHT_UP,
	IPT_JOYSTICKRIGHT_DOWN,
	IPT_JOYSTICKRIGHT_LEFT,
	IPT_JOYSTICKRIGHT_RIGHT,
	IPT_JOYSTICKLEFT_UP,
	IPT_JOYSTICKLEFT_DOWN,
	IPT_JOYSTICKLEFT_LEFT,
	IPT_JOYSTICKLEFT_RIGHT,

	IPT_BUTTON1,
	IPT_BUTTON2,
	IPT_BUTTON3,
	IPT_BUTTON4,
	IPT_BUTTON5,
	IPT_BUTTON6,
	IPT_BUTTON7,
	IPT_BUTTON8,
	IPT_BUTTON9,
	IPT_BUTTON10,

	IPT_START1,
	IPT_START2,
	IPT_START3,
	IPT_START4,
	IPT_START5,
	IPT_START6,
	IPT_START7,
	IPT_START8,

	IPT_COIN1,
	IPT_COIN2,
	IPT_COIN3,
	IPT_COIN4,
	IPT_COIN5,
	IPT_COIN6,
	IPT_COIN7,
	IPT_COIN8,
	IPT_BILL1,

	IPT_SERVICE1,
	IPT_SERVICE2,
	IPT_SERVICE3,
	IPT_SERVICE4,

	IPT_SERVICE,
	IPT_TILT,
	IPT_INTERLOCK,
	IPT_VOLUME_UP,
	IPT_VOLUME_DOWN,

	IPT_MAHJONG_A,
	IPT_MAHJONG_B,
	IPT_MAHJONG_C,
	IPT_MAHJONG_D,
	IPT_MAHJONG_E,
	IPT_MAHJONG_F,
	IPT_MAHJONG_G,
	IPT_MAHJONG_H,
	IPT_MAHJONG_I,
	IPT_MAHJONG_J,
	IPT_MAHJONG_K,
	IPT_MAHJONG_L,
	IPT_MAHJONG_M,
	IPT_MAHJONG_N,
	IPT_MAHJONG_KAN,
	IPT_MAHJONG_PON,
	IPT_MAHJONG_CHI,
	IPT_MAHJONG_REACH,
	IPT_MAHJONG_RON,
	IPT_MAHJONG_BET,
	IPT_MAHJONG_LAST_CHANCE,
	IPT_MAHJONG_SCORE,
	IPT_MAHJONG_DOUBLE_UP,
	IPT_MAHJONG_FLIP_FLOP,

	0
};

/**
 * Get a complete list of input ports and their state.
 */
void mame_ui_input_map(unsigned* pdigital_mac, struct mame_digital_map_entry* digital_map, unsigned digital_max)
{
	unsigned digital_mac;
	input_port_entry* i;
	input_port_default_entry* j;

	digital_mac = 0;

	/* get the default ports */
	j = get_input_port_list();
	assert(j != 0);

	while (j->type != IPT_END) {
		if (digital_mac < digital_max) {
			unsigned n;
			for(n=glue_port_seq_begin(j->type);n<glue_port_seq_end(j->type);++n) {
				unsigned port = glue_port_convert(j->type, j->player, glue_port_seqtype(j->type, n), j->name);
				unsigned k;

				/* only a subset */
				for(k=0;PORT_TYPE_REPORT_DEFAULT[k] != 0;++k)
					if (PORT_TYPE_REPORT_DEFAULT[k] == MAME_PORT_TYPE_GET(port))
						break;

				if (PORT_TYPE_REPORT_DEFAULT[k]) {
					input_seq* seq = glue_portdef_seqeval_get(j, glue_port_seqtype(j->type, n));

					digital_map[digital_mac].port = port;
					digital_map[digital_mac].port_state = seq_pressed(seq);

					glue_seq_convert(seq->code, SEQ_MAX, digital_map[digital_mac].seq, MAME_INPUT_MAP_MAX);

					++digital_mac;
				}
			}
		}

		++j;
	}

	/* get the game ports */
	i = Machine->input_ports;
	assert(i != 0);

	while (i->type != IPT_END) {
		if (digital_mac < digital_max) {
			unsigned n;
			for(n=glue_port_seq_begin(i->type);n<glue_port_seq_end(i->type);++n) {
				unsigned port = glue_port_convert(i->type, i->player, glue_port_seqtype(i->type, n), i->name);
				unsigned k;

				/* only a subset */
				for(k=0;PORT_TYPE_REPORT_GAME[k] != 0;++k)
					if (PORT_TYPE_REPORT_GAME[k] == MAME_PORT_TYPE_GET(port))
						break;

				if (PORT_TYPE_REPORT_GAME[k]) {
					input_seq* seq = glue_port_seqeval_get(i, glue_port_seqtype(i->type, n));

					digital_map[digital_mac].port = port;
					digital_map[digital_mac].port_state = seq_pressed(seq);

					glue_seq_convert(seq->code, SEQ_MAX, digital_map[digital_mac].seq, MAME_INPUT_MAP_MAX);

					++digital_mac;
				}
			}
		}
		++i;
	}

	*pdigital_mac = digital_mac;
}

void mame_name_adjust(char* dst, unsigned size, const char* s)
{
	unsigned i;
	adv_bool require_space = 0;

	dst[0] = 0;

	for(i=0;i<s[i];++i) {
		if (isalnum(s[i])) {
			if (require_space)
				sncatc(dst, size, '_');
			require_space = 0;
			sncatc(dst, size, tolower(s[i]));
		} else {
			if (dst[0])
				require_space = 1;
		}
	}
}

#define A(name, NAME) \
	{ "p1_" name, MAME_PORT_PLAYER(IPT_##NAME, 1) }, \
	{ "p2_" name, MAME_PORT_PLAYER(IPT_##NAME, 2) }, \
	{ "p3_" name, MAME_PORT_PLAYER(IPT_##NAME, 3) }, \
	{ "p4_" name, MAME_PORT_PLAYER(IPT_##NAME, 4) },

static struct mame_analog ANALOG[] = {
	A("paddlex", PADDLE)
	A("paddley", PADDLE_V)
	A("stickx", AD_STICK_X)
	A("sticky", AD_STICK_Y)
	A("stickz", AD_STICK_Z)
	A("lightgunx", LIGHTGUN_X)
	A("lightguny", LIGHTGUN_Y)
	A("pedalgas", PEDAL)
	A("pedalbrake", PEDAL2)
	A("pedalother", PEDAL3)
	A("dialx", DIAL)
	A("dialy", DIAL_V)
	A("trackballx", TRACKBALL_X)
	A("trackbally", TRACKBALL_Y)
	A("mousex", MOUSE_X)
	A("mousey", MOUSE_Y)
	{ 0, 0 }
};

struct mame_analog* mame_analog_list(void)
{
	return ANALOG;
}

struct mame_analog* mame_analog_find(unsigned port)
{
	struct mame_analog* i;
	for(i=ANALOG;i->name;++i)
		if (i->port == port)
			return i;
	return 0;
}

/***************************************************************************/
/* MAME callback interface */

unsigned char mame_ui_cpu_read(unsigned cpu, unsigned addr)
{
	unsigned char r = cpunum_read_byte(cpu, addr);
	return r;
}

unsigned mame_ui_frames_per_second(void)
{
	return Machine->drv->frames_per_second;
}

/**
 * Check if a MAME port is active.
 * A port is active if the associated key sequence is pressed.
 * The port values are only values get from the mame_port_list() function.
 */
int mame_ui_port_pressed(unsigned port)
{
	struct advance_safequit_context* safequit_context = &CONTEXT.safequit;
	input_port_default_entry* i;
	unsigned type;
	int seqtype;

	type = MAME_PORT_TYPE_GET(port);

	switch (type) {
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
	case IPT_MAME_PORT_EVENT7 :
		return (advance_safequit_event_mask(safequit_context) & 0x100) != 0;
	case IPT_MAME_PORT_EVENT8 :
		return (advance_safequit_event_mask(safequit_context) & 0x200) != 0;
	case IPT_MAME_PORT_EVENT9 :
		return (advance_safequit_event_mask(safequit_context) & 0x400) != 0;
	case IPT_MAME_PORT_EVENT10 :
		return (advance_safequit_event_mask(safequit_context) & 0x800) != 0;
	case IPT_MAME_PORT_EVENT11 :
		return (advance_safequit_event_mask(safequit_context) & 0x1000) != 0;
	case IPT_MAME_PORT_EVENT12 :
		return (advance_safequit_event_mask(safequit_context) & 0x2000) != 0;
	case IPT_MAME_PORT_EVENT13 :
		return (advance_safequit_event_mask(safequit_context) & 0x4000) != 0;
	case IPT_MAME_PORT_EVENT14 :
		return (advance_safequit_event_mask(safequit_context) & 0x8000) != 0;
	}

	i = glue_port_convertback(port, &seqtype);

	if (!i)
		return 0;

	return seq_pressed(glue_portdef_seqeval_get(i, seqtype));
}

void mame_ui_area_set(unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
/* TODO MAME 0.105 changed this call. Check if the panning is affected. */
	ui_set_visible_area(x1, y1, x2, y2);
}

void mame_ui_refresh(void)
{
	schedule_full_refresh();
}

void mame_ui_gamma_factor_set(double gamma)
{
	palette_set_global_gamma(palette_get_global_gamma() * gamma);
}

/***************************************************************************/
/* OSD */

/**
 * Deinitialize the system.
 */
void osd2_exit(void)
{
}

/**
 * Initialize the system.
 * \return
 * - == 0 On success.
 * - == -1 On error.
 */
int osd_init(void)
{
	add_pause_callback(osd2_video_pause);
	add_pause_callback(osd2_sound_pause);
	add_exit_callback(osd2_exit);

	return 0;
}

/**
 * Allocate executable memory.
 * Behave like malloc().
 */
void* osd_alloc_executable(size_t size)
{
	void* p = malloc(size);
#if HAVE_MPROTECT
	if (p) {
		int r;
		r = mprotect(p, size, PROT_READ | PROT_WRITE | PROT_EXEC);
		if (r != 0) {
			log_std(("ERROR:osd: mprotect(%p,%d,...) failed, %s\n", p, (unsigned)size, strerror(errno)));
		}
	}
#endif
	return p;
}

/**
 * Free executable memory.
 * Behave like free().
 */
void osd_free_executable(void* p)
{
	free(p);
}

/**
 * Ensure that a memory region is valid.
 * It can return alwasy 0 if this check is not possible.
 */
int osd_is_bad_read_ptr(const void *ptr, size_t size)
{
	return 0;
}

/**
 * Create the video.
 * \return
 * - ==0 On success.
 * - ==-1 On error.
 */
int osd_create_display(const osd_create_params *params, UINT32 *rgb_components)
{
	unsigned width;
	unsigned height;
	unsigned aspect_x;
	unsigned aspect_y;

	log_std(("osd: osd_create_display(width:%d, height:%d, aspect_x:%d, aspect_y:%d, depth:%d, colors:%d, fps:%g, attributes:%d)\n", params->width, params->height, params->aspect_x, params->aspect_y, params->depth, params->colors, (double)params->fps, params->video_attributes));

	/* print any buffered message before setting the video mode */
	target_flush();

	width = params->width;
	height = params->height;
	aspect_x = params->aspect_x;
	aspect_y = params->aspect_y;

	GLUE.video_flag = 0; /* the video isn't initialized */

	GLUE.option.vector_flag = (params->video_attributes & VIDEO_TYPE_VECTOR) != 0;

	GLUE.option.aspect_x = aspect_x;
	GLUE.option.aspect_y = aspect_y;

	GLUE.option.bits_per_pixel = params->depth;
	GLUE.option.fps = params->fps;

	if (GLUE.option.bits_per_pixel == 8 || GLUE.option.bits_per_pixel == 16) {
		GLUE.option.color_def = color_def_make_palette_from_size(GLUE.option.bits_per_pixel / 8);
		GLUE.option.rgb_flag = 0;
	} else if (GLUE.option.bits_per_pixel == 15) {
		GLUE.option.color_def = color_def_make_rgb_from_sizelenpos(2, 5, 10, 5, 5, 5, 0);
		GLUE.option.rgb_flag = 1;
	} else if (GLUE.option.bits_per_pixel == 32) {
		GLUE.option.color_def = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0);
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

/**
 * Destroy the video.
 */
void osd_close_display(void)
{
	log_std(("osd: osd_close_display()\n"));

	if (GLUE.video_flag)
		osd2_video_done();
}

/**
 * Display a menu.
 */
int osd_menu(unsigned menu, int selected)
{
	unsigned input;
	int r;

	log_pedantic(("osd: osd_menu(%d)\n", selected));

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

	switch (menu) {
	case 0 :
		r = osd2_video_menu(selected, input);
		break;
	case 1 :
		r = osd2_audio_menu(selected, input);
		break;
	default:
		r = -1;
		break;
	}

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
	int limit;

	/* Correction for a generic sound buffer underflow. */
	/* Generally happen that the DMA buffer underflow reporting */
	/* a fill state instead of an empty one. */
	/* The value is a guessed estimation which should not */
	/* generated problems on the MAME core */
	limit = 16;

	if (samples < limit) {
		log_std(("WARNING:glue: too small sound samples %d adjusted to %d\n", samples, limit));
		samples = limit;
	}

	GLUE.sound_last_count = samples;

	return GLUE.sound_last_count;
}

/**
 * Update the video frame.
 * \note Called after osd_update_audio_stream().
 */
void osd_update_video_and_audio(mame_display *display)
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

	if (display->game_bitmap != 0) {
		pgame = &game;
		game.size_x = display->game_bitmap->width;
		game.size_y = display->game_bitmap->height;
		game.ptr = display->game_bitmap->base;
		game.bytes_per_scanline = display->game_bitmap->rowbytes;
	} else {
		pgame = 0;
		log_std(("ERROR:glue: null game bitmap\n"));
	}

	if (display->debug_bitmap != 0) {
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
	input = GLUE.input;

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

/**
 * Start the audio stream.
 * Also used to start the thread system.
 * \note Called after osd_create_display().
 * \return
 * - >0 The number of samples required for the next frame.
 * - ==0 Disable the sound generation.
 * - ==-1 On error.
 */
int osd_start_audio_stream(int stereo)
{
	unsigned rate = Machine->sample_rate;

	log_std(("osd: osd_start_audio_stream(sample_rate:%d, stereo_flag:%d)\n", rate, stereo));

	assert(GLUE.sound_flag == 0);

	if (osd2_sound_init(&rate, stereo) != 0) {
		log_std(("osd: osd_start_audio_stream return no sound. Disable MAME sound generation.\n"));

		if (osd2_thread_init() != 0) {
			return -1;
		}

		/* disable the MAME sound generation */
		Machine->sample_rate = 0;
		return 0;
	}

	if (osd2_thread_init() != 0) {
		return -1;
	}

	log_std(("osd: osd_start_audio_stream return %d rate\n", rate));

	/* adjust the MAME sample rate to the effective value */
	Machine->sample_rate = rate;

	GLUE.sound_flag = 1;

	/* compute the rate, any adjustement in the video speed will */
	/* not change the sound speed */
	if (GLUE.sound_fps == 0)
		GLUE.sound_fps = Machine->drv->frames_per_second;
	GLUE.sound_step = rate / (GLUE.sound_fps * GLUE.sound_speed);
	GLUE.sound_latency = 0;

	log_std(("glue: sound samples for frame %d\n", GLUE.sound_step));

	GLUE.sound_silence_count = 2 * GLUE.sound_step; /* double size for safety */
	if (stereo) {
		GLUE.sound_silence_buffer = (short*)malloc(4 * GLUE.sound_silence_count);
		memset(GLUE.sound_silence_buffer, 0, 4 * GLUE.sound_silence_count);
	} else {
		GLUE.sound_silence_buffer = (short*)malloc(2 * GLUE.sound_silence_count);
		memset(GLUE.sound_silence_buffer, 0, 2 * GLUE.sound_silence_count);
	}

	return GLUE.sound_step;
}

/**
 * Stop the audio stream.
 */
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

/**
 * Update the audio stream.
 * \return The number of samples required for the next frame.
 */
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

/**
 * Time measure.
 */
cycles_t osd_cycles(void)
{
	return target_clock();
}

/**
 * Time base.
 */
cycles_t osd_cycles_per_second(void)
{
	return TARGET_CLOCKS_PER_SEC;
}

/**
 * Time measure for profiling.
 * It must return the maximum precise timer available.
 * The time base isn't required.
 */
cycles_t osd_profiling_ticks(void)
{
	return target_clock();
}

/**
 * Filter the main exit request.
 * \param result Result until now.
 */
int osd_input_exit_filter(int result)
{
	return advance_input_exit_filter(&CONTEXT.input, &CONTEXT.safequit, result);
}

/**
 * Filter the input port state.
 * \param result Result until now.
 * \param type Port type.
 * \param player Port player.
 * \param seqtype Port squence type. One of SEQ_TYPE_* defines.
 */
int osd_input_port_filter(int result, unsigned type, unsigned player, int seqtype)
{
	if (!result) {
		unsigned port;

		port = glue_port_convert(type, player, seqtype, 0);

		result = hardware_is_input_simulated(SIMULATE_EVENT, port);
	}

	return result;
}

static int on_exit_menu(int selected)
{
	ui_menu_item exit_menu[8];
	int sel;
	int total;

	sel = selected - 1;

	total = 0;

	exit_menu[total].text = "Continue";
	exit_menu[total].subtext = 0;
	exit_menu[total].flags = 0;
	++total;

	exit_menu[total].text = "Exit";
	exit_menu[total].subtext = 0;
	exit_menu[total].flags = 0;
	++total;

	osd_ui_menu(exit_menu, total, sel);

	if (input_ui_pressed_repeat(IPT_UI_DOWN, 8)) {
		sel = (sel + 1) % total;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP, 8)) {
		sel = (sel + total - 1) % total;
	}

	if (input_ui_pressed(IPT_UI_SELECT)) {
		if (sel == 1)
			sel = -2;
		if (sel == 0)
			sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CANCEL)) {
		sel = -1;
	}

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		schedule_full_refresh();
	}

	return sel + 1;
}

/**
 * Handle the OSD user interface.
 * \return
 * - ==0 Normal condition.
 * - ==1 User asked to exit from the program.
 */
int osd_handle_user_interface(mame_bitmap *bitmap, int is_menu_active)
{
	unsigned input;

	/* save the bitmap */
	GLUE.bitmap = bitmap;

	if (!is_menu_active) {
		int res = osd_input_exit_filter(input_ui_pressed(IPT_UI_CANCEL));
		if (res > 1)
			return 1;
		if (res != 0) {
			mame_pause(1);

			res = 1;
			while (res > 0) {
				res = on_exit_menu(res);
				update_video_and_audio();
			}

			mame_pause(0);

			if (res < 0)
				return 1;
		}
	}

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
	if (input_ui_pressed(IPT_UI_COCKTAIL))
		input |= OSD_INPUT_COCKTAIL;
	if (input_ui_pressed(IPT_UI_HELP))
		input |= OSD_INPUT_HELP;
	if (input_ui_pressed(IPT_UI_SHOW_FPS))
		input |= OSD_INPUT_SHOW_FPS;
	if (input_ui_pressed(IPT_UI_STARTUP_END))
		input |= OSD_INPUT_STARTUP_END;

	/* continous input, a direct MAME function doesn't exist */
	if (seq_pressed(input_port_default_seq(IPT_UI_TURBO, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_TURBO;
	if (seq_pressed(input_port_default_seq(IPT_UI_PAN_RIGHT, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_PAN_RIGHT;
	if (seq_pressed(input_port_default_seq(IPT_UI_PAN_LEFT, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_PAN_LEFT;
	if (seq_pressed(input_port_default_seq(IPT_UI_PAN_UP, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_PAN_UP;
	if (seq_pressed(input_port_default_seq(IPT_UI_PAN_DOWN, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_PAN_DOWN;
	if (seq_pressed(input_port_default_seq(IPT_COIN1, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_COIN1;
	if (seq_pressed(input_port_default_seq(IPT_COIN2, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_COIN2;
	if (seq_pressed(input_port_default_seq(IPT_COIN3, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_COIN3;
	if (seq_pressed(input_port_default_seq(IPT_COIN4, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_COIN4;
	if (seq_pressed(input_port_default_seq(IPT_START1, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_START1;
	if (seq_pressed(input_port_default_seq(IPT_START2, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_START2;
	if (seq_pressed(input_port_default_seq(IPT_START3, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_START3;
	if (seq_pressed(input_port_default_seq(IPT_START4, 0, SEQ_TYPE_STANDARD)))
		input |= OSD_INPUT_START4;

	GLUE.input = input;

	if (input_ui_pressed(IPT_UI_RECORD_START))
		osd_record_start();
	if (input_ui_pressed(IPT_UI_RECORD_STOP))
		osd_record_stop();

	return 0;
}

void osd_log_va(const char* text, va_list arg)
{
	log_va(text, arg);
}

/***************************************************************************/
/* Initialization */

static adv_conf_enum_int OPTION_DEPTH[] = {
{ "auto", 0 },
{ "8", 8 },
{ "15", 15 },
{ "16", 16 },
{ "32", 32 }
};

#ifdef MESS

static void mess_init(adv_conf* context)
{
	const char** i;

	options.image_count = 0;

	/* use the old portable ui */
	options.disable_normal_ui = 0;

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
		option->ram = strtol(s, &e, 10);

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
			target_err("Invalid argument '%s' for option 'misc_ramsize'.\n", s);
			return -1;
		}
	}

	return 0;
}

static void mess_done(void)
{
}

#endif

adv_error mame_init(struct advance_context* context)
{
	unsigned i, j;

	/* clear the GLUE context */
	memset(&GLUE, 0, sizeof(GLUE));

	/* clear the MAME global struct */
	/* clear all, if some new options is added it should be safe to have them at 0 */
	memset(&options, 0, sizeof(options));

	/* setup the port list */
	j = 0;

	/* check that port masks don't overlap */
	assert((MAME_PORT_PLAYER_MASK ^ MAME_PORT_TYPE_MASK ^ MAME_PORT_INDEX_MASK) == (MAME_PORT_PLAYER_MASK | MAME_PORT_TYPE_MASK | MAME_PORT_INDEX_MASK));

	/* add the constant ports */
	for(i=0;GLUE_PORT_STD[i].name;++i) {
		if (j+1<GLUE_PORT_MAX) {
			GLUE_PORT[j] = GLUE_PORT_STD[i];
			++j;
		}
	}

#ifdef MESS
	/* add the keyboard names */
	for(i=0;GLUE_KEYBOARD_STD[i].name;++i) {
		if (j+1<GLUE_PORT_MAX) {
			GLUE_PORT[j].name = GLUE_KEYBOARD_STD[i].name;
			GLUE_PORT[j].desc = GLUE_KEYBOARD_STD[i].desc;
			GLUE_PORT[j].port = MAME_PORT_KEYBOARD(i);
			++j;
		}
	}
#endif

	/* mark the end */
	GLUE_PORT[j].name = 0;
	GLUE_PORT[j].desc = 0;
	GLUE_PORT[j].port = 0;

	conf_bool_register_default(context->cfg, "display_artwork_backdrop", 1);
	conf_bool_register_default(context->cfg, "display_artwork_overlay", 1);
	conf_bool_register_default(context->cfg, "display_artwork_bezel", 0);
	conf_bool_register_default(context->cfg, "display_artwork_crop", 1);
	conf_bool_register_default(context->cfg, "sound_samples", 1);

	conf_bool_register_default(context->cfg, "display_antialias", 1);
	conf_bool_register_default(context->cfg, "display_translucency", 1);
	conf_float_register_limit_default(context->cfg, "display_beam", 1.0, 16.0, 1.0);
	conf_float_register_limit_default(context->cfg, "display_flicker", 0.0, 100.0, 0.0);
	conf_float_register_limit_default(context->cfg, "display_intensity", 0.5, 3.0, 1.5);

	conf_float_register_limit_default(context->cfg, "display_gamma", 0.5, 2.0, 1.0);
	conf_float_register_limit_default(context->cfg, "display_brightness", 0.1, 10.0, 1.0);

	conf_bool_register_default(context->cfg, "misc_cheat", 0);
	conf_string_register_default(context->cfg, "misc_languagefile", "english.lng");
	conf_string_register_default(context->cfg, "misc_cheatfile", "cheat.dat");

	conf_string_register_default(context->cfg, "misc_hiscorefile", "hiscore.dat");

	conf_string_register_default(context->cfg, "misc_bios", "default");

#ifdef MESS
	mess_init(context->cfg);
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
	unsigned i, j;

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

	option->cheat_flag = conf_bool_get_default(cfg_context, "misc_cheat");

	sncpy(option->language_file_buffer, sizeof(option->language_file_buffer), conf_string_get_default(cfg_context, "misc_languagefile"));

	sncpy(option->cheat_file_buffer, sizeof(option->cheat_file_buffer), conf_string_get_default(cfg_context, "misc_cheatfile"));

	sncpy(option->savegame_file_buffer, sizeof(option->savegame_file_buffer), "");

	sncpy(option->bios_buffer, sizeof(option->bios_buffer), conf_string_get_default(cfg_context, "misc_bios"));

	/* convert the dir separator char to ';'. */
	/* the cheat system use always this char in all the operating system */
	for(s=option->cheat_file_buffer;*s;++s)
		if (*s == file_dir_separator())
			*s = ';';

	sncpy(option->hiscore_file_buffer, sizeof(option->hiscore_file_buffer), conf_string_get_default(cfg_context, "misc_hiscorefile"));

#ifdef MESS
	if (mess_config_load(cfg_context, option) != 0) {
		target_err("Error loading the device configuration options.\n");
		return -1;
	}

#if 0 /* TEST: Print the list of digital controls */
	log_std(("glue: MAME/MESS digital port list\n"));
	j = 0;
	log_std(("\t\t"));
	for(i=0;GLUE_PORT_STD[i].name;++i) {
		j += strlen(GLUE_PORT_STD[i].name) + 2;
		log_std(("%s,", GLUE_PORT_STD[i].name));
		if (j>54) {
			j = 0;
			log_std(("\n\t\t"));
		} else {
			log_std((" "));
		}
	}
	for(i=0;GLUE_KEYBOARD_STD[i].name;++i) {
		j += strlen(GLUE_KEYBOARD_STD[i].name) + 2;
		log_std(("%s,", GLUE_KEYBOARD_STD[i].name));
		if (j>54) {
			j = 0;
			log_std(("\n\t\t"));
		} else {
			log_std((" "));
		}
	}
	log_std(("\n"));
#endif
#endif

	return 0;
}


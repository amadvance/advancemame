/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Andrea Mazzoleni
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
#include "glue.h"
#include "thread.h"
#include "fuzzy.h"
#include "hscript.h"

#include "advance.h"

struct advance_context CONTEXT;

/***************************************************************************/
/* Log */

void adv_svgalib_log_va(const char *text, va_list arg)
{
	log_va(text, arg);
}

/***************************************************************************/
/* Signal */

void os_signal(int signum, void* info, void* context)
{
	hardware_script_abort();

	os_default_signal(signum, info, context);
}

/***************************************************************************/
/* Select */

struct game_fuzzy {
	unsigned index;
	unsigned fuzzy;
};

static int game_fuzzy_cmp(const void* a, const void* b)
{
	const struct game_fuzzy* A = (const struct game_fuzzy*)a;
	const struct game_fuzzy* B = (const struct game_fuzzy*)b;
	if (A->fuzzy < B->fuzzy)
		return -1;
	if (A->fuzzy > B->fuzzy)
		return 1;
	return 0;
}

static const mame_game* select_game(const char* gamename)
{
	int game_count = 0;
	struct game_fuzzy* game_map;
	unsigned i;
	int limit;
	unsigned print_count;

	for(i=0;mame_game_at(i);++i) {
		if (strcmp(gamename, mame_game_name(mame_game_at(i))) == 0) {
			return mame_game_at(i);
		}
		++game_count;
	}

	game_map = malloc(game_count*sizeof(struct game_fuzzy));

	target_err("Game \"%s\" isn't supported.\n", gamename);

	limit = (strlen(gamename) / 3) * FUZZY_UNIT_A;
	if (limit < 4*FUZZY_UNIT_A)
		limit = 4*FUZZY_UNIT_A;
	if (limit > 7*FUZZY_UNIT_A)
		limit = 7*FUZZY_UNIT_A;

	for(i=0;i<game_count;++i) {
		const mame_game* game = mame_game_at(i);
		int fuzzy_short = fuzzy(gamename, mame_game_name(game), limit);
		int fuzzy_long = fuzzy(gamename, mame_game_description(game), limit);
		int fuzzy = fuzzy_short < fuzzy_long ? fuzzy_short : fuzzy_long;

		if (limit > fuzzy + 3*FUZZY_UNIT_A)
			limit = fuzzy + 3*FUZZY_UNIT_A; /* limit the error range */

		game_map[i].index = i;
		game_map[i].fuzzy = fuzzy;
	}

	qsort(game_map, game_count, sizeof(struct game_fuzzy), game_fuzzy_cmp);

	print_count = 0;
	while (print_count < game_count && game_map[print_count].fuzzy < limit) {
		unsigned k;
		k = print_count;
		while (k < game_count && game_map[print_count].fuzzy == game_map[k].fuzzy)
			++k;
		if (k >= 15) {
			break;
		}
		print_count = k;
	}

	if (print_count > 0 && print_count < 15) {
		target_err("\nSimilar names are:\n");
		for(i=0;i<print_count && i<game_count;++i) {
			const mame_game* game = mame_game_at(game_map[i].index);
			target_err("%10s %s\n", mame_game_name(game), mame_game_description(game));
		}
	}

	free(game_map);
	return 0;
}

static const mame_game* select_lang(int lang, const mame_game* parent)
{
	unsigned i;
	const mame_game* best;
	unsigned best_score;

	best = parent;
	best_score = lang_identify_text(lang, mame_game_lang(best));

	for(i=0;mame_game_at(i);++i) {
		const mame_game* game = mame_game_at(i);
		if (mame_game_working(game)
			&& mame_game_parent(game) == parent) {
			unsigned score = lang_identify_text(lang, mame_game_lang(game));

			log_std(("emu:language: game %s, score %d, text %s\n", mame_game_name(game), score, mame_game_lang(game)));

			if (score > best_score) {
				best = game;
				best_score = score;
			}
		}
	}

	log_std(("emu:language: select game %s\n", mame_game_name(best)));

	return best;
}

/***************************************************************************/
/* Version/Help */

static void version(void)
{
	char report_buffer[128];
	target_out("%s %s\n", ADV_TITLE, ADV_VERSION);
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__) /* OSDEF Detect compiler version */
#define COMPILER_RESOLVE(a) #a
#define COMPILER(a, b, c) COMPILER_RESOLVE(a) "." COMPILER_RESOLVE(b) "." COMPILER_RESOLVE(c)
	target_out("Compiled %s with gcc-%s\n", __DATE__, COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__));
#endif
	target_out("\n");

	target_out("Drivers (in priority order):\n");
	video_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("  Video:%s\n", report_buffer);
	soundb_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("  Sound:%s\n", report_buffer);
	keyb_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("  Keyboard:%s\n", report_buffer);
	joystickb_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("  Joystick:%s\n", report_buffer);
	mouseb_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("  Mouse:%s\n", report_buffer);
	target_out("\n");

	target_out("Directories:\n");
#ifdef ADV_DATADIR
	target_out("  Data: %s\n", ADV_DATADIR);
#else
	target_out("  Data: . (current directory)\n");
#endif
	target_out("\n");

	target_out("Configuration (in priority order):\n");
	if (file_config_file_host(ADV_NAME ".rc") != 0)
		target_out("  Host configuration file (R): %s\n", file_config_file_host(ADV_NAME ".rc"));
	target_out("  Command line (R)\n");
	target_out("  Home configuration file (RW): %s\n", file_config_file_home(ADV_NAME ".rc"));
	if (file_config_file_data(ADV_NAME ".rc") != 0)
		target_out("  Data configuration file (R): %s\n", file_config_file_data(ADV_NAME ".rc"));
}

static void help(void)
{
#if !defined(__MSDOS__) && !defined(__WIN32__)
	const char* slash = "--";
#else
	const char* slash = "-";
#endif
	target_out(ADV_COPY);
	target_out("\n");
	target_out("Usage: %s [options] GAME\n\n", ADV_NAME);
	target_out("Options:\n");
	target_out("%sdefault        add all the default options at the configuration file\n", slash);
	target_out("%sremove         remove all the default option from the configuration file\n", slash);
	target_out("%slog            create a log of operations\n", slash);
	target_out("%slistxml        output the rom XML file\n", slash);
	target_out("%srecord FILE    record an .inp file\n", slash);
	target_out("%splayback FILE  play an .inp file\n", slash);
	target_out("%sversion        print the version\n", slash);
	target_out("\n");
#ifdef MESS
	target_out("Example: %s ti99_4a\n", ADV_NAME);
#else
	target_out("Example: %s polyplay\n", ADV_NAME);
#endif
	target_out("\n");
#if !defined(__MSDOS__) && !defined(__WIN32__)
	target_out("To get help type 'man %s'.\n", ADV_NAME);
#ifdef ADV_DATADIR
	target_out("Extensive documentation is located at '%s'\n", ADV_DATADIR "/doc");
#endif
	target_out("\n");
#endif
}

/***************************************************************************/
/* Configuration */

static adv_conf_conv STANDARD[] = {
#ifdef __MSDOS__
{ "", "allegro_*", "*", "%s", "%s", "%s", ADV_CONF_CONV_AUTOREG_MULTI }, /* auto registration of the Allegro options */
#endif
{ "*", "input_dipswitch[*]", "*", "%s", "%s", "%s", ADV_CONF_CONV_AUTOREG_SINGLE }, /* auto register */
#ifdef MESS
{ "*", "input_configswitch[*]", "*", "%s", "%s", "%s", ADV_CONF_CONV_AUTOREG_SINGLE }, /* auto register */
#endif

/* 0.57.1 */
{ "*", "misc_mameinfofile", "*", "%s", "misc_infofile", "%s", 0 }, /* rename */
{ "*", "sound_recordtime", "*", "%s", "record_sound_time", "%s", 0 }, /* rename */
/* 0.61.0 */
{ "*", "input_analog[*]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_track[*]", "*", "", "", "", 0 }, /* ignore */
/* 0.61.1 */
{ "*", "input_map[*,track]", "*", "", "", "", 0 }, /* ignore */
/* 0.61.2 */
{ "*", "misc_language", "*", "%s", "misc_languagefile", "%s", 0 }, /* rename */
{ "*", "input_safeexit", "*", "%s", "misc_safequit", "%s", 0 }, /* rename */
{ "*", "device_joystick", "standard", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "dual", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "4button", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "6button", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "8button", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "fspro", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "wingex", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sidewinder", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sidewinderag", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "gamepadpro", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "grip", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "grip4", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sneslpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sneslpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sneslpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "psxlpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "psxlpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "psxlpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "n64lpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "n64lpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "n64lpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "db9lpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "db9lp2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "db9lp3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "tgxlpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "tgxlpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "tgxlpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "segaisa", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "segapci", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "segapcifast", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "wingwarrior", "%s", "%s", "allegro/%s", 0 }, /* rename */
/* 0.61.3 */
{ "*", "display_waitvsync", "*", "", "", "", 0 }, /* ignore */
/* 0.61.4 */
{ "*", "device_svgaline_divide_clock", "*", "%s", "device_svgaline_divideclock", "%s", 0 }, /* rename */
/* 0.62.1 */
{ "*", "dir_imager", "*", "", "", "", 0 }, /* ignore */
{ "*", "dir_imagerw", "*", "", "", "", 0 }, /* ignore */
{ "*", "dir_imagediff", "*", "", "", "", 0 }, /* ignore */
/* 0.62.2 */
{ "*", "sound_stereo", "*", "", "", "", 0 }, /* ignore */
{ "*", "display_rgb", "*", "", "", "", 0 }, /* ignore */
{ "*", "display_depth", "auto", "%s", "display_color", "auto", 0 }, /* rename */
{ "*", "display_depth", "8", "%s", "display_color", "palette8", 0 }, /* rename */
{ "*", "display_depth", "15", "%s", "display_color", "bgr15", 0 }, /* rename */
{ "*", "display_depth", "16", "%s", "display_color", "bgr16", 0 }, /* rename */
{ "*", "display_depth", "32", "%s", "display_color", "bgr32", 0 }, /* rename */
{ "*", "device_sdl_fullscreen", "yes", "%s", "device_video_output", "fullscreen", 0 }, /* rename */
{ "*", "device_sdl_fullscreen", "no", "%s", "device_video_output", "window", 0 }, /* rename */
{ "*", "device_video_8bit", "*", "%s", "device_color_palette8", "%s", 0 }, /* rename */
{ "*", "device_video_15bit", "*", "%s", "device_color_bgr15", "%s", 0 }, /* rename */
{ "*", "device_video_16bit", "*", "%s", "device_color_bgr16", "%s", 0 }, /* rename */
{ "*", "device_video_24bit", "*", "%s", "device_color_bgr24", "%s", 0 }, /* rename */
{ "*", "device_video_32bit", "*", "%s", "device_color_bgr32", "%s", 0 }, /* rename */
/* 0.62.3 */
{ "*", "display_artwork", "*", "%s", "display_artwork_backdrop", "%s", 0 }, /* rename */
/* 0.68.0 */
{ "*", "display_magnify", "yes", "%s", "%s", "2", 0 }, /* rename */
{ "*", "display_magnify", "no", "%s", "%s", "1", 0 }, /* rename */
{ "*", "display_adjust", "generate", "%s", "%s", "generate_yclock", 0 }, /* rename */
/* 0.72.0 */
{ "*", "input_map[0,trakx]", "*", "%s", "input_map[p1_trakx]", "%s", 0 }, /* rename */
{ "*", "input_map[1,trakx]", "*", "%s", "input_map[p2_trakx]", "%s", 0 }, /* rename */
{ "*", "input_map[2,trakx]", "*", "%s", "input_map[p3_trakx]", "%s", 0 }, /* rename */
{ "*", "input_map[3,trakx]", "*", "%s", "input_map[p4_trakx]", "%s", 0 }, /* rename */
{ "*", "input_map[0,traky]", "*", "%s", "input_map[p1_traky]", "%s", 0 }, /* rename */
{ "*", "input_map[1,traky]", "*", "%s", "input_map[p2_traky]", "%s", 0 }, /* rename */
{ "*", "input_map[2,traky]", "*", "%s", "input_map[p3_traky]", "%s", 0 }, /* rename */
{ "*", "input_map[3,traky]", "*", "%s", "input_map[p4_traky]", "%s", 0 }, /* rename */
{ "*", "input_map[0,x]", "*", "%s", "input_map[p1_x]", "%s", 0 }, /* rename */
{ "*", "input_map[1,x]", "*", "%s", "input_map[p2_x]", "%s", 0 }, /* rename */
{ "*", "input_map[2,x]", "*", "%s", "input_map[p3_x]", "%s", 0 }, /* rename */
{ "*", "input_map[3,x]", "*", "%s", "input_map[p4_x]", "%s", 0 }, /* rename */
{ "*", "input_map[0,y]", "*", "%s", "input_map[p1_y]", "%s", 0 }, /* rename */
{ "*", "input_map[1,y]", "*", "%s", "input_map[p2_y]", "%s", 0 }, /* rename */
{ "*", "input_map[2,y]", "*", "%s", "input_map[p3_y]", "%s", 0 }, /* rename */
{ "*", "input_map[3,y]", "*", "%s", "input_map[p4_y]", "%s", 0 }, /* rename */
{ "*", "input_map[0,z]", "*", "%s", "input_map[p1_z]", "%s", 0 }, /* rename */
{ "*", "input_map[1,z]", "*", "%s", "input_map[p2_z]", "%s", 0 }, /* rename */
{ "*", "input_map[2,z]", "*", "%s", "input_map[p3_z]", "%s", 0 }, /* rename */
{ "*", "input_map[3,z]", "*", "%s", "input_map[p4_z]", "%s", 0 }, /* rename */
{ "*", "input_map[0,pedal]", "*", "%s", "input_map[p1_pedal]", "%s", 0 }, /* rename */
{ "*", "input_map[1,pedal]", "*", "%s", "input_map[p2_pedal]", "%s", 0 }, /* rename */
{ "*", "input_map[2,pedal]", "*", "%s", "input_map[p3_pedal]", "%s", 0 }, /* rename */
{ "*", "input_map[3,pedal]", "*", "%s", "input_map[p4_pedal]", "%s", 0 }, /* rename */
{ "*", "script_led[1]", "*", "%s", "script_led1", "%s", 0 }, /* rename */
{ "*", "script_led[2]", "*", "%s", "script_led2", "%s", 0 }, /* rename */
{ "*", "script_led[3]", "*", "%s", "script_led3", "%s", 0 }, /* rename */
{ "*", "script_coin[1]", "*", "%s", "script_coin1", "%s", 0 }, /* rename */
{ "*", "script_coin[2]", "*", "%s", "script_coin2", "%s", 0 }, /* rename */
{ "*", "script_coin[3]", "*", "%s", "script_coin3", "%s", 0 }, /* rename */
{ "*", "script_coin[4]", "*", "%s", "script_coin4", "%s", 0 }, /* rename */
{ "*", "script_start[1]", "*", "%s", "script_start1", "%s", 0 }, /* rename */
{ "*", "script_start[2]", "*", "%s", "script_start2", "%s", 0 }, /* rename */
{ "*", "script_start[3]", "*", "%s", "script_start3", "%s", 0 }, /* rename */
{ "*", "script_start[4]", "*", "%s", "script_start4", "%s", 0 }, /* rename */
{ "*", "misc_safequitdebug", "*", "%s", "misc_eventdebug", "%s", 0 }, /* rename */
{ "*", "misc_safequitfile", "*", "%s", "misc_eventfile", "%s", 0 }, /* rename */
/* 0.74.0 */
{ "*", "display_rotate", "*", "", "", "", 0 }, /* ignore */
/* 0.74.1 */
{ "*", "display_resizeeffect", "scale2x", "%s", "display_resizeeffect", "scale", 0 }, /* rename */
{ "*", "display_resizeeffect", "scale3x", "%s", "display_resizeeffect", "scale", 0 }, /* rename */
{ "*", "display_resizeeffect", "scale4x", "%s", "display_resizeeffect", "scale", 0 }, /* rename */
{ "*", "display_resizeeffect", "lq2x", "%s", "display_resizeeffect", "lq", 0 }, /* rename */
{ "*", "display_resizeeffect", "lq3x", "%s", "display_resizeeffect", "lq", 0 }, /* rename */
{ "*", "display_resizeeffect", "hq2x", "%s", "display_resizeeffect", "hq", 0 }, /* rename */
{ "*", "display_resizeeffect", "hq3x", "%s", "display_resizeeffect", "hq", 0 }, /* rename */
/* 0.74.2 */
{ "*", "input_map[p1_pedal_up]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_map[p1_pedal_down]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_map[p2_pedal_up]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_map[p2_pedal_down]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_map[p3_pedal_up]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_map[p3_pedal_down]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_map[p4_pedal_up]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_map[p4_pedal_down]", "*", "", "", "", 0 }, /* ignore */
/* 0.77.0 */
{ "*", "display_resizeeffect", "filterx", "%s", "%s", "filter", 0 }, /* rename */
{ "*", "display_resizeeffect", "filtery", "%s", "%s", "filter", 0 }, /* rename */
/* 0.77.1 */
{ "*", "device_video_output", "zoom", "%s", "%s", "overlay", 0 }, /* rename */
{ "*", "device_video_zoom", "*", "%s", "device_video_overlay", "%s", 0 }, /* rename */
/* 0.78.0 */
{ "*", "device_video_overlay", "*", "%s", "device_video_overlaysize", "%s", 0 }, /* rename */
{ "*", "dir_cfg", "*", "", "", "", 0 }, /* ignore */
/* 0.78.1 */
{ "*", "misc_fps", "*", "%s", "sync_fps", "%s", 0 }, /* rename */
{ "*", "misc_speed", "*", "%s", "sync_speed", "%s", 0 }, /* rename */
{ "*", "misc_turbospeed", "*", "%s", "sync_turbospeed", "%s", 0 }, /* rename */
{ "*", "misc_startuptime", "*", "%s", "sync_startuptime", "%s", 0 }, /* rename */
/* 0.80.0 */
{ "*", "input_map[doubleright_up]", "*", "%s", "input_map[p1_doubleright_up]", "%s", 0 }, /* rename */
{ "*", "input_map[doubleright_down]", "*", "%s", "input_map[p1_doubleright_down]", "%s", 0 }, /* rename */
{ "*", "input_map[doubleright_left]", "*", "%s", "input_map[p1_doubleright_left]", "%s", 0 }, /* rename */
{ "*", "input_map[doubleright_right]", "*", "%s", "input_map[p1_doubleright_right]", "%s", 0 }, /* rename */
{ "*", "input_map[doubleleft_up]", "*", "%s", "input_map[p1_doubleleft_up]", "%s", 0 }, /* rename */
{ "*", "input_map[doubleleft_down]", "*", "%s", "input_map[p1_doubleleft_down]", "%s", 0 }, /* rename */
{ "*", "input_map[doubleleft_left]", "*", "%s", "input_map[p1_doubleleft_left]", "%s", 0 }, /* rename */
{ "*", "input_map[doubleleft_right]", "*", "%s", "input_map[p1_doubleleft_right]", "%s", 0 }, /* rename */
/* 0.85.0 */
{ "*", "input_map[p1_trakx]", "*", "%s", "input_map[p1_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_traky]", "*", "%s", "input_map[p1_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_trakx]", "*", "%s", "input_map[p2_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_traky]", "*", "%s", "input_map[p2_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_trakx]", "*", "%s", "input_map[p3_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_traky]", "*", "%s", "input_map[p3_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_trakx]", "*", "%s", "input_map[p4_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_traky]", "*", "%s", "input_map[p4_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_x]", "*", "%s", "input_map[p1_stickx]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_y]", "*", "%s", "input_map[p1_sticky]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_z]", "*", "%s", "input_map[p1_stickz]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_x]", "*", "%s", "input_map[p2_stickx]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_y]", "*", "%s", "input_map[p2_sticky]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_z]", "*", "%s", "input_map[p2_stickz]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_x]", "*", "%s", "input_map[p3_stickx]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_y]", "*", "%s", "input_map[p3_sticky]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_z]", "*", "%s", "input_map[p3_stickz]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_x]", "*", "%s", "input_map[p4_stickx]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_y]", "*", "%s", "input_map[p4_sticky]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_z]", "*", "%s", "input_map[p4_stickz]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_pedal]", "*", "%s", "input_map[p1_pedalgas]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_pedal]", "*", "%s", "input_map[p2_pedalgas]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_pedal]", "*", "%s", "input_map[p3_pedalgas]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_pedal]", "*", "%s", "input_map[p4_pedalgas]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_pedal1]", "*", "%s", "input_map[p1_pedalgas_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_pedal2]", "*", "%s", "input_map[p1_pedalbrake_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_pedal1]", "*", "%s", "input_map[p2_pedalgas_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_pedal2]", "*", "%s", "input_map[p2_pedalbrake_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_pedal1]", "*", "%s", "input_map[p3_pedalgas_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_pedal2]", "*", "%s", "input_map[p3_pedalbrake_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_pedal1]", "*", "%s", "input_map[p4_pedalgas_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_pedal2]", "*", "%s", "input_map[p4_pedalbrake_push]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_pedal1_autorelease]", "*", "%s", "input_map[p1_pedalgas_release]", "%s", 0 }, /* rename */
{ "*", "input_map[p1_pedal2_autorelease]", "*", "%s", "input_map[p1_pedalbrake_release]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_pedal1_autorelease]", "*", "%s", "input_map[p2_pedalgas_release]", "%s", 0 }, /* rename */
{ "*", "input_map[p2_pedal2_autorelease]", "*", "%s", "input_map[p2_pedalbrake_release]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_pedal1_autorelease]", "*", "%s", "input_map[p3_pedalgas_release]", "%s", 0 }, /* rename */
{ "*", "input_map[p3_pedal2_autorelease]", "*", "%s", "input_map[p3_pedalbrake_release]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_pedal1_autorelease]", "*", "%s", "input_map[p4_pedalgas_release]", "%s", 0 }, /* rename */
{ "*", "input_map[p4_pedal2_autorelease]", "*", "%s", "input_map[p4_pedalbrake_release]", "%s", 0 }, /* rename */
/* 0.86.0 */
{ "*", "misc_crash", "*", "%s", "debug_crash", "%s", 0 }, /* rename */
{ "*", "misc_rawsound", "*", "%s", "debug_rawsound", "%s", 0 }, /* rename */
{ "*", "misc_internaldepth", "*", "%s", "debug_internaldepth", "%s", 0 }, /* rename */
/* 0.86.1 */
{ "*", "input_setting[p1_paddle_x]", "*", "%s", "input_setting[p1_paddlex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_paddle_x]", "*", "%s", "input_setting[p2_paddlex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_paddle_x]", "*", "%s", "input_setting[p3_paddlex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_paddle_x]", "*", "%s", "input_setting[p4_paddlex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_paddle_y]", "*", "%s", "input_setting[p1_paddley]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_paddle_y]", "*", "%s", "input_setting[p2_paddley]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_paddle_y]", "*", "%s", "input_setting[p3_paddley]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_paddle_y]", "*", "%s", "input_setting[p4_paddley]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_stick_x]", "*", "%s", "input_setting[p1_stickx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_stick_x]", "*", "%s", "input_setting[p2_stickx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_stick_x]", "*", "%s", "input_setting[p3_stickx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_stick_x]", "*", "%s", "input_setting[p4_stickx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_stick_y]", "*", "%s", "input_setting[p1_sticky]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_stick_y]", "*", "%s", "input_setting[p2_sticky]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_stick_y]", "*", "%s", "input_setting[p3_sticky]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_stick_y]", "*", "%s", "input_setting[p4_sticky]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_stick_z]", "*", "%s", "input_setting[p1_stickz]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_stick_z]", "*", "%s", "input_setting[p2_stickz]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_stick_z]", "*", "%s", "input_setting[p3_stickz]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_stick_z]", "*", "%s", "input_setting[p4_stickz]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_lightgun_x]", "*", "%s", "input_setting[p1_lightgunx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_lightgun_x]", "*", "%s", "input_setting[p2_lightgunx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_lightgun_x]", "*", "%s", "input_setting[p3_lightgunx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_lightgun_x]", "*", "%s", "input_setting[p4_lightgunx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_lightgun_y]", "*", "%s", "input_setting[p1_lightguny]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_lightgun_y]", "*", "%s", "input_setting[p2_lightguny]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_lightgun_y]", "*", "%s", "input_setting[p3_lightguny]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_lightgun_y]", "*", "%s", "input_setting[p4_lightguny]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_dial_x]", "*", "%s", "input_setting[p1_dialx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_dial_x]", "*", "%s", "input_setting[p2_dialx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_dial_x]", "*", "%s", "input_setting[p3_dialx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_dial_x]", "*", "%s", "input_setting[p4_dialx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_dial_y]", "*", "%s", "input_setting[p1_dialy]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_dial_y]", "*", "%s", "input_setting[p2_dialy]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_dial_y]", "*", "%s", "input_setting[p3_dialy]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_dial_y]", "*", "%s", "input_setting[p4_dialy]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_trackball_x]", "*", "%s", "input_setting[p1_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_trackball_x]", "*", "%s", "input_setting[p2_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_trackball_x]", "*", "%s", "input_setting[p3_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_trackball_x]", "*", "%s", "input_setting[p4_trackballx]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_trackball_y]", "*", "%s", "input_setting[p1_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_trackball_y]", "*", "%s", "input_setting[p2_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_trackball_y]", "*", "%s", "input_setting[p3_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_trackball_y]", "*", "%s", "input_setting[p4_trackbally]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_mouse_x]", "*", "%s", "input_setting[p1_mousex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_mouse_x]", "*", "%s", "input_setting[p2_mousex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_mouse_x]", "*", "%s", "input_setting[p3_mousex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_mouse_x]", "*", "%s", "input_setting[p4_mousex]", "%s", 0 }, /* rename */
{ "*", "input_setting[p1_mouse_y]", "*", "%s", "input_setting[p1_mousey]", "%s", 0 }, /* rename */
{ "*", "input_setting[p2_mouse_y]", "*", "%s", "input_setting[p2_mousey]", "%s", 0 }, /* rename */
{ "*", "input_setting[p3_mouse_y]", "*", "%s", "input_setting[p3_mousey]", "%s", 0 }, /* rename */
{ "*", "input_setting[p4_mouse_y]", "*", "%s", "input_setting[p4_mousey]", "%s", 0 }, /* rename */
{ "*", "ui_speedmark", "*", "%s", "debug_speedmark", "%s", 0 }, /* rename */
/* 0.88.0 */
{ "*", "debug_internaldepth", "*", "", "", "", 0 }, /* ignore */
/* 0.100.0 */
{ "*", "misc_historyfile", "*", "", "", "", 0 }, /* ignore */
{ "*", "misc_infofile", "*", "", "", "", 0 }, /* ignore */
{ "*", "sound_resamplefilter", "*", "", "", "", 0 }, /* ignore */
/* 1.2 */
{ "*", "display_resizeeffect", "scale", "%s", "%s", "scalex", 0 } /* rename */
};

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...)
{
	va_list arg;
	va_start(arg, desc);
	target_err_va(desc, arg);
	target_err("\n");
	if (valid)
		target_err("%s\n", valid);
	va_end(arg);
}

adv_error include_load(adv_conf* context, int priority, const char* include_spec, adv_bool ignore_unknown, adv_bool multi_line, const adv_conf_conv* conv_map, unsigned conv_mac, conf_error_callback* error, void* error_context)
{
	char separator[2];
	char* s;
	int i;

	separator[0] = file_dir_separator();
	separator[1] = 0;

	i = 0;
	s = strdup(include_spec);

	sskip(&i, s, " \t");
	while (s[i]) {
		char c;
		const char* file;
		const char* include_file;

		file = stoken(&c, &i, s, separator, " \t");
		sskip(&i, s, " \t");

		if (file[0] == 0 || (c != 0 && s[i] == 0)) {
			error_callback(error_context, conf_error_failure, file, 0, 0, "Error in the include file specification.");
			free(s);
			return -1;
		}

		include_file = file_config_file_home(file);

		if (access(include_file, R_OK)!=0) {
			error_callback(error_context, conf_error_failure, include_file, 0, 0, "Missing configuration include file '%s'.", include_file);
			free(s);
			return -1;
		}

		if (conf_input_file_load_adv(context, priority, include_file, 0, ignore_unknown, multi_line, conv_map, conv_mac, error_callback, error_context) != 0) {
			free(s);
			return -1;
		}
	}

	free(s);

	return 0;
}

/***************************************************************************/
/* Main */

int os_main(int argc, char* argv[])
{
	int r;
	int i;
	struct mame_option option;
	int opt_xml;
	int opt_log;
	int opt_logsync;
	int opt_default;
	int opt_remove;
	int opt_help;
	const char* opt_cfg;
	char* opt_gamename;
	int opt_version;
	struct advance_context* context = &CONTEXT;
	const char* section_map[32];
	unsigned section_mac;
	const mame_game* parent;
	char buffer[128];
	char cfg_buffer[512];
	const char* control;

	opt_xml = 0;
	opt_log = 0;
	opt_logsync = 0;
	opt_gamename = 0;
	opt_default = 0;
	opt_remove = 0;
	opt_version = 0;
	opt_help = 0;
	opt_cfg = 0;

	memset(&option, 0, sizeof(option));
	memset(&CONTEXT, 0, sizeof(CONTEXT));

	if (thread_init() != 0) {
		target_err("Error initializing the thread support.\n");
		goto err;
	}

	context->cfg = conf_init();
	if (!context->cfg) {
		target_err("Error initializing the configuration support.\n");
		goto err_thread;
	}

	if (os_init(context->cfg)!=0) {
		target_err("Error initializing the OS support.\n");
		goto err_conf;
	}

	/* include file */
	conf_string_register_default(context->cfg, "include", "");

	if (mame_init(context)!=0)
		goto err_os;
	if (advance_global_init(&context->global, context->cfg)!=0)
		goto err_os;
	if (advance_video_init(&context->video, context->cfg)!=0)
		goto err_os;
	if (advance_sound_init(&context->sound, context->cfg)!=0)
		goto err_os;
	if (advance_input_init(&context->input, context->cfg)!=0)
		goto err_os;
	if (advance_ui_init(&context->ui, context->cfg)!=0)
		goto err_os;
	if (advance_record_init(&context->record, context->cfg)!=0)
		goto err_os;
	if (advance_fileio_init(&context->fileio, context->cfg)!=0)
		goto err_os;
	if (advance_safequit_init(&context->safequit, context->cfg)!=0)
		goto err_os;
	if (hardware_script_init(context->cfg)!=0)
		goto err_os;

	if (conf_input_args_load(context->cfg, 3, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	option.debug_flag = 0;
	for(i=1;i<argc;++i) {
		if (target_option_compare(argv[i], "cfg")) {
			opt_cfg = argv[i+1];
			++i;
		} else if (target_option_compare(argv[i], "default")) {
			opt_default = 1;
		} else if (target_option_compare(argv[i], "version")) {
			opt_version = 1;
		} else if (target_option_compare(argv[i], "help")) {
			opt_help = 1;
		} else if (target_option_compare(argv[i], "log")) {
			opt_log = 1;
		} else if (target_option_compare(argv[i], "logsync")) {
			opt_logsync = 1;
		} else if (target_option_compare(argv[i], "remove")) {
			opt_remove = 1;
		} else if (target_option_compare(argv[i], "debug")) {
			option.debug_flag = 1;
		} else if (target_option_compare(argv[i], "listxml")) {
			opt_xml = 1;
		} else if (target_option_compare(argv[i], "record") && i+1<argc && argv[i+1][0] != '-') {
			if (strchr(argv[i+1], '.') == 0)
				snprintf(option.record_file_buffer, sizeof(option.record_file_buffer), "%s.inp", argv[i+1]);
			else
				snprintf(option.record_file_buffer, sizeof(option.record_file_buffer), "%s", argv[i+1]);
			++i;
		} else if (target_option_compare(argv[i], "playback") && i+1<argc && argv[i+1][0] != '-') {
			if (strchr(argv[i+1], '.') == 0)
				snprintf(option.playback_file_buffer, sizeof(option.playback_file_buffer), "%s.inp", argv[i+1]);
			else
				snprintf(option.playback_file_buffer, sizeof(option.playback_file_buffer), "%s", argv[i+1]);
			++i;
		} else if (target_option_extract(argv[i]) == 0) {
			unsigned j;
			if (opt_gamename) {
				target_err("Multiple game name definition, '%s' and '%s'.\n", opt_gamename, argv[i]);
				goto err_os;
			}
			opt_gamename = argv[i];
			/* case insensitive compare, there are a lot of frontends which use uppercase names */
			for(j=0;opt_gamename[j];++j)
				opt_gamename[j] = tolower(opt_gamename[j]);
		} else {
			target_err("Unknown command line option '%s'.\n", argv[i]);
			goto err_os;
		}
	}

	if (opt_cfg) {
		sncpy(cfg_buffer, sizeof(cfg_buffer), file_config_file_home(opt_cfg));
	} else {
		sncpy(cfg_buffer, sizeof(cfg_buffer), file_config_file_home(ADV_NAME ".rc"));
	}

	if (opt_xml) {
		mame_print_xml(stdout);
		goto done_os;
	}

	if (opt_version) {
		version();
		goto done_os;
	}

	if (opt_help) {
		help();
		goto done_os;
	}

	if (opt_log || opt_logsync) {
		if (log_init(ADV_NAME ".log", opt_logsync) != 0) {
			target_err("Error opening the log file '" ADV_NAME ".log'.\n");
			goto err_os;
		}
	}

	if (file_config_file_host(ADV_NAME ".rc")!=0) {
		if (conf_input_file_load_adv(context->cfg, 4, file_config_file_host(ADV_NAME ".rc"), 0, 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0) {
			goto err_os;
		}
	}

	if (file_config_file_data(ADV_NAME ".rc")!=0) {
		if (conf_input_file_load_adv(context->cfg, 0, file_config_file_data(ADV_NAME ".rc"), 0, 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0) {
			goto err_os;
		}
	}

	if (conf_input_file_load_adv(context->cfg, 1, cfg_buffer, cfg_buffer, 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_os;

	/* check if the configuration file is writable */
	if (access(cfg_buffer, F_OK)) {
		context->global.state.is_config_writable = access(cfg_buffer, W_OK)==0;
	} else {
		context->global.state.is_config_writable = 1;
	}

	if (access(file_config_file_home(ADV_NAME ".rc"), F_OK)!=0) {
		target_out("Creating a standard configuration file...\n");
		advance_fileio_default_dir();
		conf_setdefault_all_if_missing(context->cfg, "");
		conf_sort(context->cfg);
		if (conf_save(context->cfg, 1, 0, error_callback, 0) != 0) {
			goto err_os;
		}
		target_out("Configuration file `%s' created with all the default options.\n", file_config_file_home(ADV_NAME ".rc"));

		/* set the empty section for reading the default options to print */
		section_map[0] = "";
		conf_section_set(context->cfg, section_map, 1);
		target_out("\n");
#ifdef MESS
		target_out("The default bios search path is `%s', the default software search path is `%s'. You can change them using the `dir_rom' and `dir_image' options in the configuration file.\n", conf_string_get_default(context->cfg, "dir_rom"), conf_string_get_default(context->cfg, "dir_image"));
#else
		target_out("The default rom search path is `%s'. You can change it using the `dir_rom' option in the configuration file.\n", conf_string_get_default(context->cfg, "dir_rom"));
#endif
		goto done_os;
	}

	if (opt_default) {
		conf_setdefault_all_if_missing(context->cfg, "");
		if (conf_save(context->cfg, 1, 0, error_callback, 0) != 0) {
			goto err_os;
		}
		target_out("Configuration file `%s' updated with all the default options.\n", file_config_file_home(ADV_NAME ".rc"));
		goto done_os;
	}

	if (opt_remove) {
		conf_remove_all_if_default(context->cfg, "");
		if (conf_save(context->cfg, 1, 0, error_callback, 0) != 0) {
			goto err_os;
		}
		target_out("Configuration file `%s' updated with all the default options removed.\n", file_config_file_home(ADV_NAME ".rc"));
		goto done_os;
	}

	/* setup the config system to search option in the global section. */
	/* It's used to load option before knowning the effective game loaded. */
	/* It implies that after the game is know the options may */
	/* differ because a specific option for the game may be present */
	/* in the configuration */
	section_map[0] = "";
	conf_section_set(context->cfg, section_map, 1);

	if (!opt_gamename) {
		if (!option.playback_file_buffer[0]) {
			target_err("No game specified in the command line.\n");
#if defined(__WIN32__)
			target_err("You must start this program from a command shell\n");
			target_err("and specify a game name in the command line.\n");
			target_err("For example type '" ADV_NAME " polyplay'\n");
#endif
			goto err_os;
		}

		if (advance_fileio_config_load(&context->fileio, context->cfg, &option) != 0)
			goto err_os;

		option.game = mame_playback_look(option.playback_file_buffer);
		if (option.game == 0)
			goto err_os;
	} else {
		int lang;

		option.game = select_game(opt_gamename);
		if (option.game == 0)
			goto err_os;

		lang = conf_int_get_default(context->cfg, "misc_lang");

		option.game = select_lang(lang, option.game);
	}

	/* set the used section */
	section_mac = 0;
	parent = option.game;
	while (parent && section_mac<8) {
		const char* s;
		s = mame_software_name(parent, context->cfg);
		if (s && s[0]) {
			section_map[section_mac++] = strdup(s);
		}
		parent = mame_game_parent(parent);
	}
	parent = option.game;
	while (parent && section_mac<8) {
		const char* s;
		s = mame_game_name(parent);
		if (s && s[0]) {
			section_map[section_mac++] = strdup(s);
		}
		parent = mame_game_parent(parent);
	}
	section_map[section_mac++] = strdup(mame_game_resolutionclock(option.game));
	section_map[section_mac++] = strdup(mame_game_resolution(option.game));
	if ((mame_game_orientation(option.game) & OSD_ORIENTATION_SWAP_XY) != 0)
		section_map[section_mac++] = strdup("vertical");
	else
		section_map[section_mac++] = strdup("horizontal");
	control = mame_game_control(option.game);
	if (control) {
		section_map[section_mac++] = strdup(control);
	}
	snprintf(buffer, sizeof(buffer), "%dplayer", mame_game_players(option.game));
	section_map[section_mac++] = strdup(buffer);
	section_map[section_mac++] = strdup("");
	conf_section_set(context->cfg, section_map, section_mac);
	for(i=0;i<section_mac;++i) {
		log_std(("emu: use configuration section '%s'\n", section_map[i]));
		free((char*)section_map[i]);
	}

	/* setup the include configuration file */
	/* it must be after the final conf_section_set() call */
	if (include_load(context->cfg, 2, conf_string_get_default(context->cfg, "include"), 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0) {
		goto err_os;
	}

	log_std(("emu: *_load()\n"));

	/* load all the options */
	if (mame_config_load(context->cfg, &option) != 0)
		goto err_os;
	if (advance_global_config_load(&context->global, context->cfg)!=0)
		goto err_os;
	if (advance_video_config_load(&context->video, context->cfg, &option) != 0)
		goto err_os;
	if (advance_sound_config_load(&context->sound, context->cfg, &option) != 0)
		goto err_os;
	if (advance_input_config_load(&context->input, context->cfg) != 0)
		goto err_os;
	if (advance_ui_config_load(&context->ui, context->cfg, &option) != 0)
		goto err_os;
	if (advance_record_config_load(&context->record, context->cfg) != 0)
		goto err_os;
	if (advance_fileio_config_load(&context->fileio, context->cfg, &option) != 0)
		goto err_os;
	if (advance_safequit_config_load(&context->safequit, context->cfg) != 0)
		goto err_os;
	if (hardware_script_config_load(context->cfg) != 0)
		goto err_os;

	if (!context->global.config.quiet_flag) {
		target_nfo(ADV_COPY);
	}

	log_std(("emu: os_inner_init()\n"));

	if (os_inner_init(ADV_TITLE) != 0) {
		goto err_os;
	}

	log_std(("emu: *_inner_init()\n"));

	if (advance_global_inner_init(&context->global) != 0)
		goto err_os_inner;
	if (advance_video_inner_init(&context->video, &option) != 0)
		goto err_inner_global;
	if (advance_input_inner_init(&context->input, context->cfg) != 0)
		goto err_inner_video;
	if (advance_ui_inner_init(&context->ui, context->cfg) != 0)
		goto err_inner_input;
	if (advance_safequit_inner_init(&context->safequit, &option) != 0)
		goto err_inner_ui;
	if (hardware_script_inner_init() != 0)
		goto err_inner_safequit;

	log_std(("emu: mame_game_run()\n"));

	r = mame_game_run(context, &option);
	if (r < 0)
		goto err_inner_script;

	log_std(("emu: *_inner_done()\n"));

	hardware_script_inner_done();
	advance_safequit_inner_done(&context->safequit);
	advance_ui_inner_done(&context->ui);
	advance_input_inner_done(&context->input);
	advance_video_inner_done(&context->video);
	advance_global_inner_done(&context->global);

	log_std(("emu: os_inner_done()\n"));

	os_inner_done();

	log_std(("emu: *_done()\n"));

	hardware_script_done();
	advance_safequit_done(&context->safequit);
	advance_fileio_done(&context->fileio);
	advance_record_done(&context->record);
	advance_ui_done(&context->ui);
	advance_input_done(&context->input);
	advance_sound_done(&context->sound);
	advance_video_done(&context->video);
	advance_global_done(&context->global);
	mame_done(context);

	log_std(("emu: os_done()\n"));

	os_done();

	log_std(("emu: conf_save()\n"));

	/* save the configuration only if modified (force_flag=0), ignore the error but print the messages (if quiet_flag is not 0) */
	if (access(file_config_file_home(ADV_NAME ".rc"), W_OK)==0) {
		conf_save(context->cfg, 0, context->global.config.quiet_flag, error_callback, 0);
	} else {
		log_std(("WARNING:emu: configuration file %s not writable\n", file_config_file_home(ADV_NAME ".rc")));
	}

	log_std(("emu: conf_done()\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	conf_done(context->cfg);

	thread_done();

	return r;

done_os:
	os_done();
	conf_done(context->cfg);
	return 0;

err_inner_script:
	hardware_script_inner_done();
err_inner_safequit:
	advance_safequit_inner_done(&context->safequit);
err_inner_ui:
	advance_ui_inner_done(&context->ui);
err_inner_input:
	advance_input_inner_done(&context->input);
err_inner_video:
	advance_video_inner_done(&context->video);
err_inner_global:
	advance_global_inner_done(&context->global);
err_os_inner:
	os_inner_done();
	hardware_script_done();
	advance_safequit_done(&context->safequit);
	advance_fileio_done(&context->fileio);
	advance_record_done(&context->record);
	advance_ui_done(&context->ui);
	advance_input_done(&context->input);
	advance_sound_done(&context->sound);
	advance_video_done(&context->video);
	advance_global_done(&context->global);
	mame_done(context);
err_os:
	os_done();
err_conf:
	conf_done(context->cfg);
err_thread:
	thread_done();
err:
	return -1;
}


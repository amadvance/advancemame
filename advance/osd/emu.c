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
#include "thread.h"
#include "hscript.h"
#include "conf.h"
#include "video.h"
#include "blit.h"
#include "os.h"
#include "fuzzy.h"
#include "log.h"
#include "target.h"
#include "portable.h"
#include "videoall.h"
#include "soundall.h"
#include "keyall.h"
#include "joyall.h"
#include "mouseall.h"
#include "snstring.h"
#include "portable.h"

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef __MSDOS__
#include <dpmi.h> /* for _go32_dpmi_remaining_virtual_memory() */
#endif

struct advance_context CONTEXT;

/***************************************************************************/
/* Log */

void adv_svgalib_log_va(const char *text, va_list arg)
{
	log_va(text, arg);
}

/***************************************************************************/
/* Signal */

void os_signal(int signum)
{
	hardware_script_abort();
	os_default_signal(signum);
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

	for(i=0;mame_game_at(i);++i) {
		if (strcmp(gamename, mame_game_name(mame_game_at(i))) == 0) {
			return mame_game_at(i);
		}
		++game_count;
	}

	game_map = malloc(game_count*sizeof(struct game_fuzzy));

	target_err("Game \"%s\" isn't supported.\n", gamename);

	limit = (strlen(gamename) - 6) * FUZZY_UNIT_A;
	if (limit < 4*FUZZY_UNIT_A)
		limit = 4*FUZZY_UNIT_A;

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

	if (game_map[0].fuzzy < limit) {
		target_err("\nSimilar are:\n");
		for (i=0;i<15 && i<game_count;++i) {
			if (game_map[i].fuzzy < limit) {
				const mame_game* game = mame_game_at(game_map[i].index);
				target_err("%10s %s\n", mame_game_name(game), mame_game_description(game));
			}
		}
	}

	free(game_map);
	return 0;
}

/***************************************************************************/
/* Version */

static void version(void)
{
	char report_buffer[128];
	target_out("%s %s\n\n", ADVANCE_TITLE, VERSION);
	video_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("Video:%s\n", report_buffer);
	sound_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("Sound:%s\n", report_buffer);
	keyb_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("Keyboard:%s\n", report_buffer);
	joystickb_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("Joystick:%s\n", report_buffer);
	mouseb_report_driver_all(report_buffer, sizeof(report_buffer));
	target_out("Mouse:%s\n", report_buffer);
}

/***************************************************************************/
/* Configuration */

static adv_conf_conv STANDARD[] = {
#ifdef __MSDOS__
{ "", "allegro_*", "*", "%s", "%s", "%s", 1 }, /* auto registration of the Allegro options */
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
{ "*", "misc_safequitfile", "*", "%s", "misc_eventfile", "%s", 0 } /* rename */
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
	int opt_info;
	int opt_xml;
	int opt_log;
	int opt_logsync;
	int opt_default;
	int opt_remove;
	char* opt_gamename;
	int opt_version;
	struct advance_context* context = &CONTEXT;
	adv_conf* config_context;
	const char* section_map[5];

	opt_info = 0;
	opt_xml = 0;
	opt_log = 0;
	opt_logsync = 0;
	opt_gamename = 0;
	opt_default = 0;
	opt_remove = 0;
	opt_version = 0;

	memset(&option, 0, sizeof(option));

	if (thread_init() != 0) {
		target_err("Error initializing the thread support.\n");
		goto err;
	}

	config_context = conf_init();
	if (!config_context) {
		target_err("Error initializing the configuration support.\n");
		goto err_thread;
	}

	if (os_init(config_context)!=0) {
		target_err("Error initializing the OS support.\n");
		goto err_conf;
	}

	/* include file */
	conf_string_register_default(config_context, "include", "");

	if (mame_init(context, config_context)!=0)
		goto err_os;
	if (advance_global_init(&context->global, config_context)!=0)
		goto err_os;
	if (advance_video_init(&context->video, config_context)!=0)
		goto err_os;
	if (advance_sound_init(&context->sound, config_context)!=0)
		goto err_os;
	if (advance_input_init(&context->input, config_context)!=0)
		goto err_os;
	if (advance_record_init(&context->record, config_context)!=0)
		goto err_os;
	if (advance_fileio_init(config_context)!=0)
		goto err_os;
	if (advance_safequit_init(&context->safequit, config_context)!=0)
		goto err_os;
	if (hardware_script_init(config_context)!=0)
		goto err_os;

	if (file_config_file_host(ADVANCE_NAME ".rc")!=0) {
		if (conf_input_file_load_adv(config_context, 4, file_config_file_host(ADVANCE_NAME ".rc"), 0, 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0) {
			goto err_os;
		}
	}

	if (file_config_file_data(ADVANCE_NAME ".rc")!=0) {
		if (conf_input_file_load_adv(config_context, 0, file_config_file_data(ADVANCE_NAME ".rc"), 0, 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0) {
			goto err_os;
		}
	}

	if (conf_input_file_load_adv(config_context, 1, file_config_file_home(ADVANCE_NAME ".rc"), file_config_file_home(ADVANCE_NAME ".rc"), 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_os;

	/* check if the configuration file is writable */
	if (access(file_config_file_home(ADVANCE_NAME ".rc"), F_OK)) {
		context->global.state.is_config_writable = access(file_config_file_home(ADVANCE_NAME ".rc"), W_OK)==0;
	} else {
		context->global.state.is_config_writable = access(file_config_file_home("."), W_OK)==0;
	}

	if (conf_input_args_load(config_context, 3, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	option.debug_flag = 0;
	for(i=1;i<argc;++i) {
		if (strcmp(argv[i], "-default") == 0) {
			opt_default = 1;
		} else if (strcmp(argv[i], "-version") == 0) {
			opt_version = 1;
		} else if (strcmp(argv[i], "-log") == 0) {
			opt_log = 1;
		} else if (strcmp(argv[i], "-logsync") == 0) {
			opt_logsync = 1;
		} else if (strcmp(argv[i], "-remove") == 0) {
			opt_remove = 1;
		} else if (strcmp(argv[i], "-debug") == 0) {
			option.debug_flag = 1;
		} else if (strcmp(argv[i], "-listinfo") == 0) {
			opt_info = 1;
		} else if (strcmp(argv[i], "-listxml") == 0) {
			opt_xml = 1;
		} else if (strcmp(argv[i], "-record") == 0 && i+1<argc && argv[i+1][0] != '-') {
			if (strchr(argv[i+1], '.') == 0)
				snprintf(option.record_file_buffer, sizeof(option.record_file_buffer), "%s.inp", argv[i+1]);
			else
				snprintf(option.record_file_buffer, sizeof(option.record_file_buffer), "%s", argv[i+1]);
			++i;
		} else if (strcmp(argv[i], "-playback") == 0 && i+1<argc && argv[i+1][0] != '-') {
			if (strchr(argv[i+1], '.') == 0)
				snprintf(option.playback_file_buffer, sizeof(option.playback_file_buffer), "%s.inp", argv[i+1]);
			else
				snprintf(option.playback_file_buffer, sizeof(option.playback_file_buffer), "%s", argv[i+1]);
			++i;
		} else if (argv[i][0]!='-') {
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

	if (opt_info) {
		mame_print_info(stdout);
		goto done_os;
	}

	if (opt_xml) {
		mame_print_xml(stdout);
		goto done_os;
	}

	if (opt_version) {
		version();
		goto done_os;
	}

	if (access(file_config_file_home(ADVANCE_NAME ".rc"), F_OK)!=0) {
		conf_set_default_if_missing(config_context, "");
		conf_sort(config_context);
		if (conf_save(config_context, 1, 0, error_callback, 0) != 0) {
			goto err_os;
		}
		target_out("Configuration file '%s' created with all the default options\n", file_config_file_home(ADVANCE_NAME ".rc"));
		goto done_os;
	}

	if (opt_default) {
		conf_set_default_if_missing(config_context, "");
		if (conf_save(config_context, 1, 0, error_callback, 0) != 0) {
			goto err_os;
		}
		target_out("Configuration file '%s' updated with all the default options\n", file_config_file_home(ADVANCE_NAME ".rc"));
		goto done_os;
	}

	if (opt_remove) {
		conf_remove_if_default(config_context, "");
		if (conf_save(config_context, 1, 0, error_callback, 0) != 0) {
			goto err_os;
		}
		target_out("Configuration file '%s' updated with all the default options removed\n", file_config_file_home(ADVANCE_NAME ".rc"));
		goto done_os;
	}

	if (opt_log || opt_logsync) {
		if (log_init(ADVANCE_NAME ".log", opt_logsync) != 0) {
			target_err("Error opening the log file '" ADVANCE_NAME ".log'.\n");
			goto err_os;
		}
	}

	if (!opt_gamename) {
		if (!option.playback_file_buffer[0]) {
			target_err("No game specified on the command line.\n");
			goto err_os;
		}

		/* we need to know where to search the playback file. Without knowing the */
		/* game only the global options can be used. */
		/* It implies that after the game is know the playback directory may */
		/* differ because a specific option for the game may be present in the */
		/* configuration */
		section_map[0] = "";
		conf_section_set(config_context, section_map, 1);

		if (advance_fileio_config_load(config_context, &option) != 0)
			goto err_os;

		option.game = mame_playback_look(option.playback_file_buffer);
		if (option.game == 0)
			goto err_os;
	} else {
		option.game = select_game(opt_gamename);
		if (option.game == 0)
			goto err_os;
	}

	/* set the used section */
	section_map[0] = mame_game_name(option.game);
	section_map[1] = mame_game_resolutionclock(option.game);
	section_map[2] = mame_game_resolution(option.game);
	if ((mame_game_orientation(option.game) & OSD_ORIENTATION_SWAP_XY) != 0)
		section_map[3] = "vertical";
	else
		section_map[3] = "horizontal";
	section_map[4] = "";
	conf_section_set(config_context, section_map, 5);
	for(i=0;i<5;++i)
		log_std(("advance: use configuration section '%s'\n", section_map[i]));

	/* setup the include configuration file */
	if (include_load(config_context, 2, conf_string_get_default(config_context, "include"), 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0) {
		goto err_os;
	}

	log_std(("advance: *_load()\n"));

	/* load all the options */
	if (mame_config_load(config_context, &option) != 0)
		goto err_os;
	if (advance_global_config_load(&context->global, config_context)!=0)
		goto err_os;
	if (advance_video_config_load(&context->video, config_context, &option) != 0)
		goto err_os;
	if (advance_sound_config_load(&context->sound, config_context, &option) != 0)
		goto err_os;
	if (advance_input_config_load(&context->input, config_context) != 0)
		goto err_os;
	if (advance_record_config_load(&context->record, config_context) != 0)
		goto err_os;
	if (advance_fileio_config_load(config_context, &option) != 0)
		goto err_os;
	if (advance_safequit_config_load(&context->safequit, config_context) != 0)
		goto err_os;
	if (hardware_script_config_load(config_context) != 0)
		goto err_os;

	if (!context->global.config.quiet_flag) {
		target_nfo(ADVANCE_COPY);
	}

	log_std(("advance: os_inner_init()\n"));

	if (os_inner_init(ADVANCE_TITLE) != 0) {
		goto err_os;
	}

	log_std(("advance: *_inner_init()\n"));

	if (advance_video_inner_init(&context->video, &option) != 0)
		goto err_os_inner;
	if (advance_input_inner_init(&context->input, config_context) != 0)
		goto err_inner_video;
	if (advance_safequit_inner_init(&context->safequit, &option)!=0)
		goto err_inner_input;
	if (hardware_script_inner_init()!=0)
		goto err_inner_safequit;

	log_std(("advance: mame_game_run()\n"));

	r = mame_game_run(context, &option);
	if (r < 0)
		goto err_inner_script;

	log_std(("advance: *_inner_done()\n"));

	hardware_script_inner_done();
	advance_safequit_inner_done(&context->safequit);
	advance_input_inner_done(&context->input);
	advance_video_inner_done(&context->video);

	log_std(("advance: os_inner_done()\n"));

	os_inner_done();

	log_std(("advance: *_done()\n"));

	hardware_script_done();
	advance_safequit_done(&context->safequit);
	advance_fileio_done();
	advance_record_done(&context->record);
	advance_input_done(&context->input);
	advance_sound_done(&context->sound);
	advance_video_done(&context->video);
	advance_global_done(&context->global);
	mame_done(context);

	log_std(("advance: os_msg_done()\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	log_std(("advance: os_done()\n"));

	os_done();

	log_std(("advance: conf_save()\n"));

	/* save the configuration only if modified, ignore the error but print the messages */
	if (access(file_config_file_home(ADVANCE_NAME ".rc"), W_OK)==0) {
		conf_save(config_context, 0, context->global.config.quiet_flag, error_callback, 0);
	}

	log_std(("advance: conf_done()\n"));

	conf_done(config_context);

	thread_done();

	return r;

done_os:
	os_done();
	conf_done(config_context);
	return 0;

err_inner_script:
	hardware_script_inner_done();
err_inner_safequit:
	advance_safequit_inner_done(&context->safequit);
err_inner_input:
	advance_input_inner_done(&context->input);
err_inner_video:
	advance_video_inner_done(&context->video);
err_os_inner:
	os_inner_done();
err_os:
	os_done();
err_conf:
	conf_done(config_context);
err_thread:
	thread_done();
err:
	return -1;
}

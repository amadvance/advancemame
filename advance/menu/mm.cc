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
 */

#include "mconfig.h"
#include "menu.h"
#include "submenu.h"
#include "text.h"
#include "play.h"
#include "log.h"
#include "target.h"
#include "os.h"
#include "option.h"

#include <stdio.h>
#include <unistd.h>

#include <iostream>

using namespace std;

// --------------------------------------------------------------------------
// Run

int run_sub(config_state& rs) {

	log_std(("menu: text_init4 call\n"));

	if (!text_init4(rs.video_font_path,rs.video_orientation_effective)) {
		return TEXT_KEY_ESC;
	}

	bool done = false;
	bool is_run = false;
	int key = 0;

	log_std(("menu: menu start\n"));

	while (!done) {
		key = run_menu(rs,(rs.video_orientation_effective & TEXT_ORIENTATION_SWAP_XY) != 0);

		if (!rs.lock_effective)
		switch (key) {
			case TEXT_KEY_HELP :
				run_help();
				break;
			case TEXT_KEY_GROUP :
				run_group(rs);
				break;
			case TEXT_KEY_EMU :
				run_emu_next(rs);
				break;
			case TEXT_KEY_TYPE :
				run_type(rs);
				break;
			case TEXT_KEY_EXCLUDE :
				if (rs.current_game)
					rs.current_game->emulator_get()->attrib_run();
				break;
			case TEXT_KEY_COMMAND :
				run_command(rs);
				break;
			case TEXT_KEY_SORT :
				run_sort(rs);
				break;
			case TEXT_KEY_MENU :
				run_submenu(rs);
				break;
			case TEXT_KEY_SETGROUP :
				run_group_move(rs);
				break;
			case TEXT_KEY_SETTYPE :
				run_type_move(rs);
				break;
			case TEXT_KEY_ROTATE :
			case TEXT_KEY_ESC :
			case TEXT_KEY_OFF :
				done = true;
				break;
		}
		switch (key) {
			case TEXT_KEY_LOCK :
				rs.lock_effective = !rs.lock_effective;
				break;
			case TEXT_KEY_IDLE_0 :
				if (rs.current_game) {
					rs.current_clone = &rs.current_game->clone_best_get();
					done = true;
					is_run = true;
				}
				break;
			case TEXT_KEY_RUN_CLONE :
				if (rs.current_game) {
					run_clone(rs);
					if (rs.current_clone) {
						done = true;
						is_run = true;
					}
				}
				break;
			case TEXT_KEY_ENTER :
				if (rs.current_game) {
					if (rs.current_game->emulator_get()->tree_get())
						rs.current_clone = &rs.current_game->clone_best_get();
					else
						rs.current_clone = rs.current_game;
					done = true;
					is_run = true;
				}
				break;
		}
	}

	if (is_run) {
		if (!rs.video_reset_mode)
			run_runinfo(rs);
	}

	log_std(("menu: menu stop\n"));

	log_std(("menu: text_done4 call\n"));
	text_done4();

	return key;
}

int run_main(config_state& rs, bool is_first) {
	log_std(("menu: text_init3 call\n"));

	if (!text_init3(rs.video_gamma, rs.video_brightness,
		rs.idle_start_first, rs.idle_start_rep, rs.idle_saver_first, rs.idle_saver_rep, rs.repeat, rs.repeat_rep,
		rs.preview_fast, rs.alpha_mode)) {
		return TEXT_KEY_ESC;
	}

	log_std(("menu: play_init call\n"));
	if (!play_init()) {
		text_done3(true);
		target_err("Error initializing the sound mixer.\n");
		target_err("Try with the option '-device_sound none'.\n");
		return TEXT_KEY_ESC;
	}

	// play start background sounds
	if (is_first) {
		play_background_effect(rs.sound_background_begin,PLAY_PRIORITY_EVENT,false);
	} else {
		play_background_effect(rs.sound_background_stop,PLAY_PRIORITY_EVENT,false);
	}

	// play start foreground sounds
	if (is_first) {
		play_foreground_effect_begin(rs.sound_foreground_begin);
	} else {
		play_foreground_effect_stop(rs.sound_foreground_stop);
	}

	// fill the player buffer
	log_std(("menu: play_fill call\n"));
	play_fill();

	bool done = false;
	bool is_terminate = false;
	bool is_run = false;
	int key = 0;

	log_std(("menu: menu start\n"));

	while (!done) {
		key = run_sub(rs);
		if (!rs.lock_effective)
		switch (key) {
			case TEXT_KEY_ROTATE : {
					unsigned mirror = rs.video_orientation_effective & (TEXT_ORIENTATION_FLIP_X | TEXT_ORIENTATION_FLIP_Y);
					unsigned flip = rs.video_orientation_effective & TEXT_ORIENTATION_SWAP_XY;
					if (mirror == 0) {
						mirror = TEXT_ORIENTATION_FLIP_Y;
					} else if (mirror == TEXT_ORIENTATION_FLIP_Y) {
						mirror = TEXT_ORIENTATION_FLIP_X | TEXT_ORIENTATION_FLIP_Y;
					} else if (mirror == (TEXT_ORIENTATION_FLIP_X | TEXT_ORIENTATION_FLIP_Y)) {
						mirror = TEXT_ORIENTATION_FLIP_X;
					} else {
						mirror = 0;
					}
					flip ^= TEXT_ORIENTATION_SWAP_XY;
					rs.video_orientation_effective = flip | mirror;
				}
				break;
			case TEXT_KEY_ESC :
			case TEXT_KEY_OFF :
				done = true;
				is_terminate = true;
				break;
		}
		switch (key) {
			case TEXT_KEY_IDLE_0 :
			case TEXT_KEY_ENTER :
			case TEXT_KEY_RUN_CLONE :
				if (rs.current_game && rs.current_clone) {
					done = true;
					is_run = true;
				}
				break;
		}
	}

	log_std(("menu: menu stop\n"));

	if (is_terminate) {
		play_foreground_effect_end(rs.sound_foreground_end);
		play_background_effect(rs.sound_background_end,PLAY_PRIORITY_END,false);
	}
	if (is_run) {
		play_foreground_effect_start(rs.sound_foreground_start);
		play_background_effect(rs.sound_background_start,PLAY_PRIORITY_END,false);
	}

	// wait for the sound end
	log_std(("menu: wait foreground stop\n"));
	play_foreground_wait();
	log_std(("menu: background stop\n"));
	play_background_stop(PLAY_PRIORITY_EVENT);
	log_std(("menu: wait background stop\n"));
	play_background_wait();

	log_std(("menu: play_done call\n"));
	play_done();

	log_std(("menu: text_done3 call\n"));
	text_done3(is_terminate || rs.video_reset_mode);

	return key;
}

//---------------------------------------------------------------------------
// run_all

int run_all(struct conf_context* config_context, config_state& rs) {
	bool done = false;
	bool is_first = true;
	int key = 0;

	rs.current_game = 0;
	rs.current_clone = 0;
	rs.fast = "";

	while (!done) {
		key = run_main(rs, is_first);

		is_first = false;

		switch (key) {
			case TEXT_KEY_ESC :
			case TEXT_KEY_OFF :
				done = true;
				break;
			case TEXT_KEY_ENTER :
			case TEXT_KEY_IDLE_0 :
			case TEXT_KEY_RUN_CLONE :
				if (!rs.current_clone)
					rs.current_clone = rs.current_game;

				if (rs.current_clone) {
					// save before
					rs.save(config_context);

					// run the game
					rs.current_clone->emulator_get()->run(*rs.current_clone,rs.video_orientation_effective,key == TEXT_KEY_IDLE_0);

					// update the game info
					rs.current_clone->emulator_get()->update(*rs.current_clone);

					// save after
					rs.save(config_context);
					
					// print the messages
					target_flush();
				}
				break;
		}
	}

	return key;
}

//---------------------------------------------------------------------------
// main

void video_log_va(const char *text, va_list arg)
{
	log_va(text,arg);
}

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...) {
	va_list arg;
	va_start(arg, desc);
	target_err_va(desc, arg);
	target_err("\n");
	if (valid)
		target_err("%s\n", valid);
	va_end(arg);
}

#ifdef __MSDOS__
/* LEGACY (to be removed) */
static struct conf_conv CONV[] = {
{ "", "video_mode_reset", "*", "", "video_restore", "%s", 0 },
{ "", "video_font", "no", "", "%s", "none", 0 },
{ "", "merge", "no", "", "%s", "none", 0 },
{ "", "mouse", "no", "", "device_mouse", "none", 0 },
{ "", "mouse", "like_emulator", "", "device_mouse", "auto", 0 },
{ "", "mouse", "*", "", "device_mouse", "%s", 0 },
{ "", "joystick", "no", "", "device_joystick", "none", 0 },
{ "", "joystick", "like_emulator", "", "device_joystick", "auto", 0 },
{ "", "joystick", "*", "", "device_joystick", "%s", 0 },
{ "", "sound", "no", "", "device_sound", "none", 0 },
{ "", "sound", "sb", "", "device_sound", "seal/sb", 0 },
{ "", "sound", "pas", "", "device_sound", "seal/pas", 0 },
{ "", "sound", "gusmax", "", "device_sound", "seal/gusmax", 0 },
{ "", "sound", "gus", "", "device_sound", "seal/gus", 0 },
{ "", "sound", "wss", "", "device_sound", "seal/wss", 0 },
{ "", "sound", "ess", "", "device_sound", "seal/ess", 0 },
{ "", "sound", "gus", "", "device_sound", "seal/gus", 0 },
{ "", "sound", "like_emulator", "", "device_sound", "auto", 0 },
{ "", "sound", "\"no\"", "", "device_sound", "none", 0 },
{ "", "sound", "\"sb\"", "", "device_sound", "seal/sb", 0 },
{ "", "sound", "\"pas\"", "", "device_sound", "seal/pas", 0 },
{ "", "sound", "\"gusmax\"", "", "device_sound", "seal/gusmax", 0 },
{ "", "sound", "\"gus\"", "", "device_sound", "seal/gus", 0 },
{ "", "sound", "\"wss\"", "", "device_sound", "seal/wss", 0 },
{ "", "sound", "\"ess\"", "", "device_sound", "seal/ess", 0 },
{ "", "sound", "\"gus\"", "", "device_sound", "seal/gus", 0 },
{ "", "sound", "\"like_emulator\"", "", "device_sound", "auto", 0 },
{ "", "sound", "\"*\"", "", "device_sound", "%s", 0 },
{ "", "sound", "*", "", "device_sound", "%s", 0 },
{ "", "video_gamma", "like_emulator", "", "%s", "1.0", 0 },
{ "", "video_brightness", "like_emulator", "", "%s", "1.0", 0 },
{ "", "video_orientation", "like_emulator", "", "%s", "", 0 },
{ "", "sound_volume", "*", "", "%s", "0", 0 }
};
#endif

static struct conf_conv STANDARD[] = {
#ifdef __MSDOS__
{ "", "allegro_*", "*", "%s", "%s", "%s", 1 }, /* auto registration of the Allegro options */
#endif
{ "*", "group_inport", "*", "%s", "group_import", "%s", 0 }, /* 1.16.0 */
{ "*", "type_inport", "*", "%s", "type_import", "%s", 0 }, /* 1.16.0 */
{ "*", "preview_aspect", "fit", "%s", "preview_expand", "3.0", 0 }, /* 1.17.4 */
{ "*", "preview_aspect", "correct", "%s", "preview_expand", "1.15", 0 }, /* 1.17.4 */
/* 2.1.0 */
{ "*", "msg_run", "*", "%s", "run_msg", "%s", 0 }, /* rename */
{ "*", "select_neogeo", "*", "", "", "", 0 }, /* remove */
{ "*", "select_neogeo", "*", "", "", "", 0 }, /* remove */
{ "*", "select_deco", "*", "", "", "", 0 }, /* remove */
{ "*", "select_playchoice", "*", "", "", "", 0 }, /* remove */
{ "*", "select_clone", "*", "", "", "", 0 }, /* remove */
{ "*", "select_bad", "*", "", "", "", 0 }, /* remove */
{ "*", "select_missing", "*", "", "", "", 0 }, /* remove */
{ "*", "select_vector", "*", "", "", "", 0 }, /* remove */
{ "*", "select_vertical", "*", "", "", "", 0 } /* remove */
};

void os_signal(int signum) {
	os_default_signal(signum);
}

int os_main(int argc, char* argv[]) {
	config_state rs;
	struct conf_context* config_context;
	bool opt_verbose;
	bool opt_log;
	bool opt_default;
	bool opt_remove;
	bool opt_logsync;
	int key = 0;
	const char* section_map[1];

	srand(time(0));

	config_context = conf_init();

	if (os_init(config_context)!=0) {
		target_err("Error initializing the OS support.\n");
		goto err_conf;
	}

	config_state::conf_register(config_context);

	text_init(config_context);
	play_reg(config_context);

#ifdef __MSDOS__
	/* LEGACY (to be removed) */
	if (file_config_file_legacy("mm.cfg")!=0 && access(file_config_file_legacy("mm.cfg"),R_OK)==0 && access(file_config_file_home("advmenu.rc"),R_OK)!=0) {
		if (conf_input_file_load_adv(config_context, 0, file_config_file_legacy("mm.cfg"), file_config_file_home("advmenu.rc"), 1, 0, CONV, sizeof(CONV)/sizeof(CONV[0]), error_callback, 0) != 0)
			goto err_init;
		conf_sort(config_context);
		conf_uncomment(config_context);
		if (conf_save(config_context,1) != 0) {
			target_err("Error writing the configuration file '%s'.\n",file_config_file_home("advmenu.rc"));
			goto err_init;
		}
		target_out("Configuration file '%s' created from '%s'.\n", file_config_file_home("advmenu.rc"), file_config_file_legacy("mm.cfg"));
		goto done_init;
	}
#endif

	if (conf_input_args_load(config_context, 1, "", &argc, argv, error_callback, 0) != 0)
		goto err_init;

	opt_verbose = false;
	opt_log = false;
	opt_logsync = false;
	opt_remove = false;
	opt_default = false;
	for(int i=1;i<argc;++i) {
		if (optionmatch(argv[i],"verbose")) {
			opt_verbose = true;
		} else if (optionmatch(argv[i],"remove")) {
			opt_remove = true;
		} else if (optionmatch(argv[i],"default")) {
			opt_default = true;
		} else if (optionmatch(argv[i],"log")) {
			opt_log = true;
		} else if (optionmatch(argv[i],"logsync")) {
			opt_logsync = true;
		} else {
			target_err("Unknown option '%s'.\n",argv[i]);
			goto err_init;
		}
	}

	if (opt_log || opt_logsync) {
		remove("advmenu.log");
		if (log_init("advmenu.log", opt_logsync) != 0) {
			target_err("Error opening the log file 'advmenu.log'.\n");
			goto err_init;
		}
	}

	log_std(("menu: %s %s\n",__DATE__,__TIME__));

	if (file_config_file_root("advmenu.rc") != 0)
		if (conf_input_file_load_adv(config_context, 2, file_config_file_root("advmenu.rc"), 0, 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
			goto err_init;

	if (conf_input_file_load_adv(config_context, 0, file_config_file_home("advmenu.rc"), file_config_file_home("advmenu.rc"), 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_init;

	section_map[0] = "";
	conf_section_set(config_context, section_map, 1);

	if (!(file_config_file_root("advmenu.rc") != 0 && access(file_config_file_root("advmenu.rc"),R_OK)==0)
		&& !(file_config_file_home("advmenu.rc") != 0 && access(file_config_file_home("advmenu.rc"),R_OK)==0)) {
		config_state::conf_default(config_context);
		conf_set_default_if_missing(config_context,"");
		conf_sort(config_context);
		if (conf_save(config_context,1) != 0) {
			target_err("Error writing the configuration file '%s'.\n",file_config_file_home("advmenu.rc"));
			goto err_init;
		}
		target_out("Configuration file '%s' created with all the default options.\n", file_config_file_home("advmenu.rc"));
		goto done_init;
	}

	if (opt_default) {
		config_state::conf_default(config_context);
		conf_set_default_if_missing(config_context,"");
		if (conf_save(config_context,1) != 0) {
			target_err("Error writing the configuration file '%s'.\n", file_config_file_home("advmenu.rc"));
			goto err_init;
		}
		target_out("Configuration file '%s' updated with all the default options.\n", file_config_file_home("advmenu.rc"));
		goto done_init;
	}

	if (opt_remove) {
		conf_remove_if_default(config_context,"");
		if (conf_save(config_context,1) != 0) {
			target_err("Error writing the configuration file '%s'.\n",file_config_file_home("advmenu.rc"));
			goto err_init;
		}
		target_out("Configuration file '%s' updated with all the default options removed.\n", file_config_file_home("advmenu.rc"));
		goto done_init;
	}

	if (!rs.load(config_context,opt_verbose)) {
		goto err_init;
	}
	if (!text_load(config_context)) {
		goto err_init;
	}
	if (!play_load(config_context)) {
		goto err_init;
	}

	if (os_inner_init("AdvanceMENU") != 0) {
		target_err("Error initializing the inner OS support.\n");
		goto err_init;
	}

	if (!text_init2(rs.video_size, rs.video_depth, rs.sound_foreground_key)) {
		goto err_inner_init;
	}
	
	// print the messages after setting the video
	target_flush();

	// set the modifiable data
	rs.restore_load();

	key = run_all(config_context,rs);

	// restore or set the changed data
	if (rs.restore == restore_none) {
		rs.restore_save();
	}

	// print the messages before restoring the video
	target_flush();

	text_done2();
	
	// save all the data
	rs.save(config_context);

	if (opt_log || opt_logsync) {
		log_done();
	}

	os_inner_done();
	
done_init:
	text_done();
	os_done();

	if (key == TEXT_KEY_OFF)
		target_apm_shutdown();

	conf_done(config_context);

	return EXIT_SUCCESS;

err_inner_init:
	os_inner_done();

err_init:
	text_done();
	os_done();

err_conf:
	conf_done(config_context);
	return EXIT_FAILURE;
}

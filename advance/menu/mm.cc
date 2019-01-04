/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2009 Andrea Mazzoleni
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

#include "portable.h"

#include "mconfig.h"
#include "menu.h"
#include "submenu.h"
#include "text.h"
#include "play.h"

#include "advance.h"

#include <iostream>

using namespace std;

// --------------------------------------------------------------------------
// Run

int run_sub(config_state& rs, bool silent)
{

	log_std(("menu: int_enable call\n"));

	if (!int_enable(
			rs.video_font_text_x, rs.video_font_text_y, rs.video_font_text_path,
			rs.video_font_bar_x, rs.video_font_bar_y, rs.video_font_bar_path,
			rs.video_font_menu_x, rs.video_font_menu_y, rs.video_font_menu_path,
			rs.video_orientation_effective)
	) {
		return EVENT_ESC;
	}

	bool done = false;
	bool is_run = false;
	int key = 0;

	log_std(("menu: menu start\n"));

	while (!done) {
		emulator* emu;

		key = run_menu(rs, (rs.video_orientation_effective & ADV_ORIENTATION_FLIP_XY) != 0, silent);

		// don't replay the sound and clip
		silent = true;

		if (rs.console_mode && key == EVENT_ESC)
			key = EVENT_MENU;

		if (!rs.lock_effective) {
			if (key == EVENT_MENU) {
				// replay the sound and clip
				silent = false;
				key = run_submenu(rs);
			}

			switch (key) {
			case EVENT_HELP:
				// replay the sound and clip
				silent = false;
				run_help(rs);
				break;
			case EVENT_GROUP:
				// replay the sound and clip
				silent = false;
				run_group_next(rs);
				break;
			case EVENT_EMU:
				// replay the sound and clip
				silent = false;
				run_emu_next(rs);
				break;
			case EVENT_TYPE:
				// replay the sound and clip
				silent = false;
				run_type_next(rs);
				break;
			case EVENT_ATTRIB:
				// replay the sound and clip
				silent = false;
				emu = run_emu_select(rs);
				if (emu)
					emu->attrib_run(SECOND_CHOICE_X, SECOND_CHOICE_Y);
				break;
			case EVENT_COMMAND:
				run_command(rs);
				break;
			case EVENT_VOLUME:
				// replay the sound and clip
				silent = false;
				run_volume(rs);
				break;
			case EVENT_DIFFICULTY:
				// replay the sound and clip
				silent = false;
				run_difficulty(rs);
				break;
			case EVENT_SORT:
				// replay the sound and clip
				silent = false;
				run_sort(rs);
				break;
			case EVENT_SETGROUP:
				// replay the sound and clip
				silent = false;
				run_group_move(rs);
				break;
			case EVENT_SETTYPE:
				// replay the sound and clip
				silent = false;
				run_type_move(rs);
				break;
			case EVENT_ROTATE:
			case EVENT_ESC:
				done = true;
				break;
			}
		}

		switch (key) {
		case EVENT_LOCK:
			rs.lock_effective = !rs.lock_effective;
			break;
		case EVENT_IDLE_0:
			if (rs.current_game) {
				rs.current_clone = &rs.current_game->clone_best_get();
				done = true;
				is_run = true;
			}
			break;
		case EVENT_CALIBRATION:
			// replay the sound and clip
			silent = false;
			run_calib(rs);
			break;
		case EVENT_CLONE:
			// replay the sound and clip
			silent = false;
			run_clone(rs);
			if (rs.current_clone) {
				done = true;
				is_run = true;
			}
			break;
		case EVENT_ENTER:
			// replay the sound and clip
			silent = false;
			if (rs.current_game) {
				done = true;
				is_run = true;
			}
			break;
		case EVENT_EXIT:
		case EVENT_OFF:
			done = true;
			break;
		}

		switch (key) {
		case EVENT_ESC:
		case EVENT_EXIT:
		case EVENT_OFF:
			// ask confirmation for the exit condition
			if (done && rs.exit_mode == exit_menu)
				done = run_exit(rs, key);
			break;
		}
	}

	if (is_run) {
		assert(rs.current_game);
		if (!rs.current_clone) {
			if (rs.current_game->emulator_get()->tree_get())
				rs.current_clone = &rs.current_game->clone_best_get();
			else
				rs.current_clone = rs.current_game;
		}
		if (!rs.resetgame_get(rs.current_clone))
			run_runinfo(rs);
	}

	log_std(("menu: menu stop\n"));

	log_std(("menu: int_disable call\n"));
	int_disable();

	return key;
}

int run_main(config_state& rs, bool is_first, bool silent)
{
	log_std(("menu: int_set call\n"));

	if (!int_set(rs.video_gamma, rs.video_brightness, rs.idle_start_first, rs.idle_start_rep, rs.idle_saver_first, rs.idle_saver_rep, rs.preview_fast, rs.ui_translucency, rs.disable_special, rs.ui_autocalib)) {
		return EVENT_ESC;
	}

	log_std(("menu: play_init call\n"));
	if (!play_init()) {
		int_unset(true);
		target_err("Error initializing the sound mixer.\n");
		target_err("Try with the option '-device_sound none'.\n");
		return EVENT_ESC;
	}

	// play start background sounds
	if (is_first) {
		play_background_effect(rs.sound_background_begin, PLAY_PRIORITY_EVENT, false);
	} else {
		play_background_effect(rs.sound_background_stop, PLAY_PRIORITY_EVENT, false);
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

	if (is_first) {
		if (rs.ui_startup != "none") {
			if (int_enable(-1, -1, "none", -1, -1, "none", -1, -1, "none", rs.video_orientation_effective)) {
				int_clip(rs.ui_startup, false);
				int_disable();
			}
		}
	}

	bool done = false;
	bool is_terminate = false;
	bool is_run = false;
	int key = 0;

	log_std(("menu: menu start\n"));

	while (!done) {
		key = run_sub(rs, silent);

		// don't replay the sound and clip
		silent = true;

		if (!rs.lock_effective) {
			switch (key) {
			case EVENT_ROTATE: {
				unsigned mirror = rs.video_orientation_effective & (ADV_ORIENTATION_FLIP_X | ADV_ORIENTATION_FLIP_Y);
				unsigned flip = rs.video_orientation_effective & ADV_ORIENTATION_FLIP_XY;
				if (mirror == 0) {
					mirror = ADV_ORIENTATION_FLIP_Y;
				} else if (mirror == ADV_ORIENTATION_FLIP_Y) {
					mirror = ADV_ORIENTATION_FLIP_X | ADV_ORIENTATION_FLIP_Y;
				} else if (mirror == (ADV_ORIENTATION_FLIP_X | ADV_ORIENTATION_FLIP_Y)) {
					mirror = ADV_ORIENTATION_FLIP_X;
				} else {
					mirror = 0;
				}
				flip ^= ADV_ORIENTATION_FLIP_XY;
				rs.video_orientation_effective = flip | mirror;
			}
			break;
			case EVENT_ESC:
				done = true;
				is_terminate = true;
				break;
			}
		}

		switch (key) {
		case EVENT_EXIT:
		case EVENT_OFF:
		case EVENT_EXIT_FORCE:
		case EVENT_OFF_FORCE:
			done = true;
			is_terminate = true;
			break;
		case EVENT_IDLE_0:
		case EVENT_ENTER:
		case EVENT_CLONE:
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
		play_background_effect(rs.sound_background_end, PLAY_PRIORITY_END, false);
	}
	if (is_run) {
		play_foreground_effect_start(rs.sound_foreground_start);
		play_background_effect(rs.sound_background_start, PLAY_PRIORITY_END, false);
	}

	// fill the player buffer
	log_std(("menu: play_fill call\n"));
	play_fill();
	bool wait = true;

	if (is_terminate) {
		if (rs.ui_exit != "none") {
			if (int_enable(-1, -1, "none", -1, -1, "none", -1, -1, "none", rs.video_orientation_effective)) {
				wait = int_clip(rs.ui_exit, false);
				int_disable();
			}
		}
	}

	// wait for the sound end
	if (!wait)
		play_foreground_stop();
	log_std(("menu: wait foreground stop\n"));
	play_foreground_wait();
	log_std(("menu: background stop\n"));
	play_background_stop(PLAY_PRIORITY_EVENT);
	log_std(("menu: wait background stop\n"));
	play_background_wait();

	log_std(("menu: play_done call\n"));
	play_done();

	log_std(("menu: int_unset call\n"));
	int_unset((is_terminate && rs.resetexit_get()) || (!is_terminate && rs.resetgame_get(rs.current_clone)));

	return key;
}

//---------------------------------------------------------------------------
// run_all

int run_all(adv_conf* config_context, config_state& rs)
{
	bool done = false;
	bool is_first = true;
	bool silent = false;
	int key = 0;

	rs.current_game = 0;
	rs.current_clone = 0;
	rs.fast = "";

	while (!done) {
		key = run_main(rs, is_first, silent);

		// the next
		is_first = false;

		// replay the sound and clip
		silent = false;

		switch (key) {
		case EVENT_ESC:
		case EVENT_EXIT:
		case EVENT_OFF:
		case EVENT_EXIT_FORCE:
		case EVENT_OFF_FORCE:
			done = true;
			break;
		case EVENT_IDLE_0:
		case EVENT_ENTER:
		case EVENT_CLONE:
			if (key == EVENT_IDLE_0) {
				// don't replay the sound and clip
				silent = true;
			}

			if (!rs.current_clone)
				rs.current_clone = rs.current_game;

			if (rs.current_clone) {
				// save before
				rs.save(config_context);

				// run the game
				if (rs.current_game->software_get()) {
					const game* bios;
					if (rs.current_clone->software_get())
						bios = &rs.current_clone->bios_get();
					else
						bios = rs.current_clone;
					rs.current_game->emulator_get()->run(*rs.current_game, bios, rs.video_orientation_effective, true, rs.difficulty_effective, rs.console_mode, play_attenuation_get(), key == EVENT_IDLE_0);
				} else {
					rs.current_clone->emulator_get()->run(*rs.current_clone, 0, rs.video_orientation_effective, true, rs.difficulty_effective, rs.console_mode, play_attenuation_get(), key == EVENT_IDLE_0);
				}

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
// Version

static void version(void)
{
	char report_buffer[128];
	target_out("AdvanceMENU %s\n", ADV_VERSION);
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
	if (file_config_file_host("advmenu.rc") != 0)
		target_out("  Host configuration file (R): %s\n", file_config_file_host("advmenu.rc"));
	target_out("  Command line (R)\n");
	target_out("  Home configuration file (RW): %s\n", file_config_file_home("advmenu.rc"));
	if (file_config_file_data("advmenu.rc") != 0)
		target_out("  Data configuration file (R): %s\n", file_config_file_data("advmenu.rc"));
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
	target_out("Usage: advmenu [options]\n\n");
	target_out("Options:\n");
	target_out("%sdefault  add all the default options at the configuration file\n", slash);
	target_out("%sremove   remove all the default option from the configuration file\n", slash);
	target_out("%slog      create a log of operations\n", slash);
	target_out("%sversion  print the version\n", slash);
	target_out("\n");
#if !defined(__MSDOS__) && !defined(__WIN32__)
	target_out("To get an extensive help type 'man advmenu'\n");
	target_out("\n");
#endif
}

//---------------------------------------------------------------------------
// Configuration

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

static adv_conf_conv STANDARD[] = {
#ifdef __MSDOS__
	{ "", "allegro_*", "*", "%s", "%s", "%s", ADV_CONF_CONV_AUTOREG_MULTI }, /* auto registration of the Allegro options */
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
	{ "*", "select_vertical", "*", "", "", "", 0 }, /* remove */
	{ "*", "type_import", "none", "", "", "", 0 }, /* remove */
	{ "*", "group_import", "none", "", "", "", 0 }, /* remove */
	{ "*", "desc_import", "none", "", "", "", 0 }, /* remove */
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
/* 2.2.0 */
	{ "*", "device_svgaline_divide_clock", "*", "%s", "device_svgaline_divideclock", "%s", 0 }, /* rename */
/* 2.2.2 */
	{ "*", "video_depth", "*", "", "", "", 0 }, /* remove */
	{ "*", "device_sdl_fullscreen", "yes", "%s", "device_video_output", "fullscreen", 0 }, /* rename */
	{ "*", "device_sdl_fullscreen", "no", "%s", "device_video_output", "window", 0 }, /* rename */
	{ "*", "device_video_8bit", "*", "%s", "device_color_palette8", "%s", 0 }, /* rename */
	{ "*", "device_video_15bit", "*", "%s", "device_color_bgr15", "%s", 0 }, /* rename */
	{ "*", "device_video_16bit", "*", "%s", "device_color_bgr16", "%s", 0 }, /* rename */
	{ "*", "device_video_24bit", "*", "%s", "device_color_bgr24", "%s", 0 }, /* rename */
	{ "*", "device_video_32bit", "*", "%s", "device_color_bgr32", "%s", 0 }, /* rename */
/* 2.2.12 */
	{ "*", "event_exit_press", "*", "", "", "", 0 }, /* remove */
	{ "*", "color", "*", "%s", "ui_color", "%s", 0 }, /* rename */
/* 2.2.13 */
	{ "*", "misc_console", "*", "%s", "ui_console", "%s", 0 }, /* rename */
/* 2.2.14 */
	{ "*", "loop", "yes", "%s", "ui_clip", "singleloop", 0 }, /* rename */
	{ "*", "loop", "no", "%s", "ui_clip", "single", 0 }, /* rename */
/* 2.2.15 */
	{ "*", "device_video_output", "zoom", "%s", "%s", "overlay", 0 }, /* rename */
	{ "*", "device_video_zoom", "*", "%s", "device_video_overlay", "%s", 0 }, /* rename */
/* 2.2.17 */
	{ "*", "device_video_overlay", "*", "%s", "device_video_overlaysize", "%s", 0 }, /* rename */
	{ "*", "sort", "coin", "%s", "%s", "play", 0 }, /* rename */
	{ "*", "sort", "timepercoin", "%s", "%s", "timeperplay", 0 }, /* rename */
/* 2.3.2 */
	{ "*", "video_size", "*", "%s", "display_size", "%s", 0 }, /* rename */
	{ "*", "video_font", "*", "%s", "ui_font", "%s", 0 }, /* rename */
	{ "*", "video_orientation", "*", "%s", "display_orientation", "%s", 0 }, /* rename */
	{ "*", "video_gamma", "*", "%s", "display_gamma", "%s", 0 }, /* rename */
	{ "*", "video_brightness", "*", "%s", "display_brightness", "%s", 0 }, /* rename */
	{ "*", "video_restore", "*", "%s", "display_restoreatgame", "%s", 0 }, /* rename */
	{ "*", "run_msg", "*", "%s", "ui_gamemsg", "%s", 0 }, /* rename */
	{ "*", "run_preview", "*", "%s", "ui_game", "%s", 0 }, /* rename */
/* 2.4.0 */
	{ "*", "ui_game", "play", "%s", "%s", "snap", 0 }, /* rename */
/* 3.2 */
/* avoid errors when buildind without SDL */
#ifndef USE_VIDEO_SDL
	{ "*", "device_video", "sdl", "", "", "", 0 }, /* ignore */
#endif
#ifndef USE_SOUND_SDL
	{ "*", "device_sdl_samples", "*", "", "", "", 0 }, /* ignore */
	{ "*", "device_sound", "sdl", "", "", "", 0 }, /* ignore */
#endif
#ifndef USE_JOYSTICK_SDL
	{ "*", "device_joystick", "sdl", "", "", "", 0 }, /* ignore */
#endif
#ifndef USE_MOUSE_SDL
	{ "*", "device_mouse", "sdl", "", "", "", 0 }, /* ignore */
#endif
#ifndef USE_KEYBOARD_SDL
	{ "*", "device_keyboard", "sdl", "", "", "", 0 }, /* ignore */
#endif
/* 3.8 */
	{ "*", "ui_font", "*", "%s", "ui_text_font", "%s", 0 }, /* rename */
	{ "*", "ui_fontsize", "*", "%s", "ui_text_size", "%s", 0 }, /* rename */
	{ "*", "misc_exit", "normal", "%s", "misc_exit", "esc", 0 }, /* rename */
};

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

		if (access(include_file, R_OK) != 0) {
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

//---------------------------------------------------------------------------
// Log

extern "C" void adv_svgalib_log_va(const char *text, va_list arg)
{
	log_va(text, arg);
}

//---------------------------------------------------------------------------
// Signal

void os_signal(int signum, void* info, void* context)
{
	os_default_signal(signum, info, context);
}

//---------------------------------------------------------------------------
// Main

int os_main(int argc, char* argv[])
{
	config_state rs;
	adv_conf* config_context;
	bool opt_verbose;
	bool opt_log;
	bool opt_default;
	bool opt_remove;
	bool opt_logsync;
	bool opt_version;
	bool opt_help;
	const char* opt_cfg;
	int key = 0;
	const char* section_map[1];
	char cfg_buffer[512];

	srand(time(0));

	config_context = conf_init();

	if (os_init(config_context) != 0) {
		target_err("Error initializing the OS support.\n");
		goto err_conf;
	}

	/* include file */
	conf_string_register_default(config_context, "include", "");

	config_state::conf_register(config_context);
	int_reg(config_context);
	play_reg(config_context);

	if (conf_input_args_load(config_context, 3, "", &argc, argv, error_callback, 0) != 0)
		goto err_init;

	opt_verbose = false;
	opt_log = false;
	opt_logsync = false;
	opt_remove = false;
	opt_default = false;
	opt_version = false;
	opt_help = false;
	opt_cfg = 0;
	for (int i = 1; i < argc; ++i) {
		if (target_option_compare(argv[i], "cfg")) {
			opt_cfg = argv[i + 1];
			++i;
		} else if (target_option_compare(argv[i], "verbose")) {
			opt_verbose = true;
		} else if (target_option_compare(argv[i], "version")) {
			opt_version = true;
		} else if (target_option_compare(argv[i], "help")) {
			opt_help = true;
		} else if (target_option_compare(argv[i], "remove")) {
			opt_remove = true;
		} else if (target_option_compare(argv[i], "default")) {
			opt_default = true;
		} else if (target_option_compare(argv[i], "log")) {
			opt_log = true;
		} else if (target_option_compare(argv[i], "logsync")) {
			opt_logsync = true;
		} else {
			target_err("Unknown option '%s'.\n", argv[i]);
			goto err_init;
		}
	}

	if (opt_cfg) {
		sncpy(cfg_buffer, sizeof(cfg_buffer), file_config_file_home(opt_cfg));
	} else {
		sncpy(cfg_buffer, sizeof(cfg_buffer), file_config_file_home("advmenu.rc"));
	}

	if (opt_version) {
		version();
		goto done_init;
	}

	if (opt_help) {
		help();
		goto done_init;
	}

	if (opt_log || opt_logsync) {
		remove("advmenu.log");
		if (log_init("advmenu.log", opt_logsync) != 0) {
			target_err("Error opening the log file 'advmenu.log'.\n");
			goto err_init;
		}
	}

	log_std(("menu: %s %s %s %s\n", "AdvanceMENU", ADV_VERSION, __DATE__, __TIME__));

	if (file_config_file_host("advmenu.rc") != 0) {
		if (conf_input_file_load_adv(config_context, 4, file_config_file_host("advmenu.rc"), 0, 0, 1, STANDARD, sizeof(STANDARD) / sizeof(STANDARD[0]), error_callback, 0) != 0) {
			goto err_init;
		}
	}

	if (file_config_file_data("advmenu.rc") != 0) {
		if (conf_input_file_load_adv(config_context, 0, file_config_file_data("advmenu.rc"), 0, 0, 1, STANDARD, sizeof(STANDARD) / sizeof(STANDARD[0]), error_callback, 0) != 0) {
			goto err_init;
		}
	}

	if (conf_input_file_load_adv(config_context, 1, cfg_buffer, cfg_buffer, 0, 1, STANDARD, sizeof(STANDARD) / sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_init;

	if (access(cfg_buffer, F_OK) != 0) {
		target_err("Creating AdvanceMENU standard configuration file...\n");
		config_state::conf_default(config_context);
		conf_setdefault_all_if_missing(config_context, "");
		conf_sort(config_context);
		if (conf_save(config_context, 1, 0, error_callback, 0) != 0) {
			goto err_init;
		}
		target_err("Configuration file `%s' created with all the default options.\n", cfg_buffer);
		target_err("\n");
		/* continue execution */
	}

	if (opt_default) {
		config_state::conf_default(config_context);
		conf_setdefault_all_if_missing(config_context, "");
		if (conf_save(config_context, 1, 0, error_callback, 0) != 0) {
			goto err_init;
		}
		target_out("Configuration file `%s' updated with all the default options.\n", cfg_buffer);
		goto done_init;
	}

	if (opt_remove) {
		conf_remove_all_if_default(config_context, "");
		if (conf_save(config_context, 1, 0, error_callback, 0) != 0) {
			goto err_init;
		}
		target_out("Configuration file `%s' updated with all the default options removed.\n", cfg_buffer);
		goto done_init;
	}

	/* set the used section */
	section_map[0] = "";
	conf_section_set(config_context, section_map, 1);

	/* setup the include configuration file */
	/* it must be after the final conf_section_set() call */
	if (include_load(config_context, 2, conf_string_get_default(config_context, "include"), 0, 1, STANDARD, sizeof(STANDARD) / sizeof(STANDARD[0]), error_callback, 0) != 0) {
		goto err_init;
	}

	log_std(("menu: *_load()\n"));

	if (!rs.load(config_context, opt_verbose)) {
		goto err_init;
	}
	if (!int_load(config_context)) {
		goto err_init;
	}
	if (!play_load(config_context)) {
		goto err_init;
	}

	if (os_inner_init("AdvanceMENU") != 0) {
		target_err("Error initializing the inner OS support.\n");
		goto err_init;
	}

	event_setup(rs.sound_foreground_key, rs.repeat, rs.repeat_rep, rs.alpha_mode);

	if (!int_init(rs.video_sizex, rs.video_sizey)) {
		goto err_inner_init;
	}

	// print the messages after setting the video
	target_flush();

	// set the modifiable data
	rs.restore_load();

	key = run_all(config_context, rs);

	// restore or set the changed data
	if (rs.restore == restore_none) {
		rs.restore_save();
	}

	// print the messages before restoring the video
	target_flush();

	int_done();

	// save all the data
	rs.save(config_context);

	if (opt_log || opt_logsync) {
		log_done();
	}

	os_inner_done();

done_init:
	int_unreg();
	os_done();

	if (key == EVENT_OFF || key == EVENT_OFF_FORCE)
		target_apm_shutdown();

	conf_done(config_context);

	return EXIT_SUCCESS;

err_inner_init:
	os_inner_done();

err_init:
	int_unreg();
	os_done();

err_conf:
	conf_done(config_context);
	return EXIT_FAILURE;
}


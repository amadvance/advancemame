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

#ifndef __MCONFIG_H
#define __MCONFIG_H

#include "game.h"
#include "category.h"
#include "emulator.h"
#include "choice.h"
#include "conf.h"

// Type of sorting
enum game_sort_t {
	sort_by_group,
	sort_by_name,
	sort_by_root_name,
	sort_by_time,
	sort_by_year,
	sort_by_manufacturer,
	sort_by_type,
	sort_by_size,
	sort_by_coin,
	sort_by_res
};

inline bool sort_by_root_name_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_leveldesc_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_name_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_manufacturer_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_manufacturer_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_year_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_year_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_res_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_res_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_time_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_time_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_coin_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_coin_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_group_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_group_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_type_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_type_less,pgame_by_desc_less,pgame_by_name_less);
}

inline bool sort_by_size_func(const game* A, const game* B) {
	return pgame_combine_less(A,B,pgame_by_size_less,pgame_by_desc_less,pgame_by_name_less);
}

typedef bool (*pgame_sort_func)(const game*, const game*);

typedef std::set<const game*,pgame_sort_func> pgame_sort_set;

// List mode
enum show_t {
	mode_list = 1,
	mode_tile_small = 2,
	mode_tile_normal = 4,
	mode_tile_big = 8,
	mode_tile_enormous = 16,
	mode_tile_giant = 32,
	mode_full = 64,
	mode_full_mixed = 128,
	mode_tile_icon = 256,
	mode_tile_marquee = 512,
	mode_list_mixed = 1024,
	mode_text = 2048
};

// Type of image to display
enum preview_t {
	preview_snap = 1,
	preview_flyer = 2,
	preview_cabinet = 4,
	preview_icon = 8,
	preview_marquee = 16,
	preview_title = 32
};

// Type of image to display
enum saver_t {
	saver_snap,
	saver_play,
	saver_flyer,
	saver_cabinet,
	saver_title,
	saver_off
};

enum restore_t {
	restore_none, // save the modification in the config file
	restore_exit, // restore the original before exiting
	restore_idle // restore the original data at the idle
};

struct config_state {

	bool load_game(const std::string& name, const std::string& group, const std::string& type, const std::string& time, const std::string& coin, const std::string& desc);
	bool load_iterator_game(struct conf_context* config_context, const std::string& tag);

public:
	game_set gar; // main game list

	pcategory_container group; // group set
	pcategory_container type; // type set
	category_container include_group_effective; // included groups
	category_container include_type_effective; // included types
	category_container include_group_orig;
	category_container include_type_orig;

	bool lock_effective; // interface locked
	bool lock_orig;

	pemulator_container emu; // list of supported emulators
	pemulator_container emu_active; // list of active emulators, a subset of emu.
	emulator_container include_emu_effective; // emulator listed
	emulator_container include_emu_orig; // emulator listed

	// video
	unsigned video_size; // preferred video x size
	unsigned video_depth; // video bit depth
	bool video_reset_mode; // reset to text mode
	std::string video_font_path; // font, "none"== default
	double video_gamma;
	double video_brightness;
	unsigned video_orientation_orig;
	unsigned video_orientation_effective;

	// desc import
	std::string desc_import_type;
	std::string desc_import_sub;
	std::string desc_import_file;

	// type import
	std::string type_import_type;
	std::string type_import_sub;
	std::string type_import_file;
	std::string type_import_section;

	// group import
	std::string group_import_type;
	std::string group_import_sub;
	std::string group_import_file;
	std::string group_import_section;

	preview_t preview_effective; // type of preview selected
	preview_t preview_orig;
	bool preview_fast; // fast preview (skip if keypress)
	double preview_expand; // max strect factor
	std::string preview_default; // default preview
	std::string preview_default_snap; // default preview for the specified mode
	std::string preview_default_flyer; // default preview for the specified mode
	std::string preview_default_cabinet; // default preview for the specified mode
	std::string preview_default_icon; // default preview for the specified mode
	std::string preview_default_marquee; // default preview for the specified mode
	std::string preview_default_title; // default preview for the specified mode

	unsigned icon_space; // space between icons

	unsigned menu_base_orig; // cursor position
	unsigned menu_rel_orig; // current game
	unsigned menu_base_effective; // cursor position
	unsigned menu_rel_effective; // current game

	bool alpha_mode; // use alphanumeric keys for fast move

	game_sort_t sort_effective; // sort mode
	game_sort_t sort_orig;

	show_t mode_effective; // list mode
	show_t mode_orig; // list mode
	unsigned mode_mask; // list of available modes (derived)
	unsigned mode_skip_mask; // list of mode skipped

	unsigned idle_start_first; // seconds
	unsigned idle_start_rep; // seconds
	unsigned idle_saver_first; // seconds
	unsigned idle_saver_rep; // seconds
	saver_t idle_saver_type;
	unsigned repeat; // milli seconds
	unsigned repeat_rep; // milli seconds

	merge_t merge; // rom merge type

	std::string msg_run_game; // message to display before a game run
	saver_t run_saver_type; // preview to display before a game run

	unsigned exit_count; // number of times the exit buttun need to be pressed

	// foreground sound
	std::string sound_foreground_begin;
	std::string sound_foreground_end;
	std::string sound_foreground_key;
	std::string sound_foreground_start;
	std::string sound_foreground_stop;

	// background sounds
	std::string sound_background_begin;
	std::string sound_background_end;
	std::string sound_background_start;
	std::string sound_background_stop;
	std::string sound_background_loop;
	std::string sound_background_loop_dir; // directory for background file
	path_container sound_background; // list of background file

	// restore
	restore_t restore;

	// internal state
	unsigned preview_mask; // type of preview accepted (computed for the shown games)
	const game* current_game; // gioco alla posizione del cursore, ==0 se nessuno
	const game* current_clone; // clone scelto, ==0 se nessuno
	std::string fast; // stringa per selezione veloce
	resource current_backdrop; // backdrop shown for the current game, "" if none
	resource current_sound; // sound used for the current game, "" if none

	bool quiet; // quiet mode

	config_state();
	~config_state();

	bool load(struct conf_context* config_context, bool opt_verbose);
	bool save(struct conf_context* config_context) const;

	void restore_load();
	void restore_save();

	static void conf_register(struct conf_context* config_context);
	static void conf_default(struct conf_context* config_context);
};

#endif

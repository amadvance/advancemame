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

/// Type of sorting.
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
	sort_by_res,
	sort_by_info,
	sort_by_timepercoin
};

inline bool sort_by_root_name_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_leveldesc_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_name_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_manufacturer_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_manufacturer_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_year_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_year_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_res_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_res_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_time_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_time_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_coin_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_coin_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_timepercoin_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_timepercoin_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_group_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_group_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_type_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_type_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_size_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_size_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_info_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_info_less, pgame_by_desc_less, pgame_by_name_less);
}

typedef bool (*pgame_sort_func)(const game*, const game*);

typedef std::set<const game*, pgame_sort_func> pgame_sort_set;

/// Type of mode.
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

/// Type of image to display.
enum preview_t {
	preview_snap = 1,
	preview_flyer = 2,
	preview_cabinet = 4,
	preview_icon = 8,
	preview_marquee = 16,
	preview_title = 32
};

/// Type of screensaver to display.
enum saver_t {
	saver_snap,
	saver_play,
	saver_flyer,
	saver_cabinet,
	saver_title,
	saver_off
};

/// Type of configuration save/restore.
enum restore_t {
	restore_none, ///< Save the modification in the config file.
	restore_exit, ///< Restore the original before exiting.
	restore_idle ///< Restore the original data at the idle.
};

struct config_state {

	bool load_game(const std::string& name, const std::string& group, const std::string& type, const std::string& time, const std::string& coin, const std::string& desc);
	bool load_iterator_game(adv_conf* config_context, const std::string& tag);
	bool load_iterator_import(adv_conf* config_context, const std::string& tag, void (config_state::*set)(const game&, const std::string&), bool opt_verbose);

	void import_desc(const game& g, const std::string& text);
	void import_info(const game& g, const std::string& text);
	void import_type(const game& g, const std::string& text);
	void import_group(const game& g, const std::string& text);

public:
	game_set gar; ///< Main game list.

	pcategory_container group; ///< Group set.
	pcategory_container type; ///< Type set.
	category_container include_group_orig; ///< Original Included groups.
	category_container include_type_orig; ///< Original Included types.
	category_container include_group_effective; ///< Included groups.
	category_container include_type_effective; ///< Included types.

	bool lock_orig; ///< Original interface locked.
	bool lock_effective; ///< Interface locked.

	pemulator_container emu; ///< Supported emulators set.
	pemulator_container emu_active; ///< Active emulators, a subset of emu.
	emulator_container include_emu_orig; ///< Original included emulators.
	emulator_container include_emu_effective; ///< Included emulators.

	// video
	unsigned video_size; ///< Preferred video x size.
	bool video_reset_mode; ///< Reset to text mode at exit.
	std::string video_font_path; ///< Font path, "none"== default.
	double video_gamma; ///< Video gamma.
	double video_brightness; ///< Video brightness.
	unsigned video_orientation_orig; ///< Original video orientation.
	unsigned video_orientation_effective; ///< Video orientation.

	difficulty_t difficulty_orig; ///< Original difficulty selected.
	difficulty_t difficulty_effective; ///< Difficulty selected.

	preview_t preview_orig; ///< Original preview type selected.
	preview_t preview_effective; ///< Preview type selected.
	bool preview_fast; ///< Fast preview mode.
	double preview_expand; ///< Max strect factor
	std::string preview_default; ///< Default preview file.
	std::string preview_default_snap; ///< Default preview file for the specified mode.
	std::string preview_default_flyer; ///< Default preview file for the specified mode.
	std::string preview_default_cabinet; ///< Default preview file for the specified mode.
	std::string preview_default_icon; ///< Default preview file for the specified mode.
	std::string preview_default_marquee; ///< Default preview file for the specified mode.
	std::string preview_default_title; ///< Default preview file for the specified mode.

	bool loop; ///< Loop the play of all the game clips and sounds.

	unsigned icon_space; ///< Space in pixels between icons.

	unsigned menu_base_orig; ///< Original cursor position.
	unsigned menu_rel_orig; ///< Original current game.
	unsigned menu_base_effective; ///< Cursor position.
	unsigned menu_rel_effective; ///< Current game.

	bool alpha_mode; ///< Use alphanumeric keys for fast move.

	game_sort_t sort_orig; ///< Original sort mode.
	game_sort_t sort_effective; ///< Sort mode.

	show_t mode_orig; ///< Original list mode.
	show_t mode_effective; ///< List mode.

	unsigned mode_skip_mask; ///< Mask of modes to skip.

	unsigned idle_start_first; ///< Idle time before the first automatic start action (seconds).
	unsigned idle_start_rep; ///< Idle time before the next automatic start action (seconds).
	unsigned idle_saver_first; ///< Idle time before the first screensaver action (seconds).
	unsigned idle_saver_rep; ///< Idle time before the next screensaver action (seconds).
	saver_t idle_saver_type; ///< Screensaver type.
	unsigned repeat; // Keyboard first repeat period.
	unsigned repeat_rep; ///< Keyboard next repeat period.

	merge_t merge; ///< Rom merge type.

	std::string msg_run_game; ///< Message to display before a game run.
	saver_t run_saver_type; ///< Preview to display before a game run.

	unsigned exit_count; ///< Number of times the exit buttun need to be pressed.

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
	std::string sound_background_loop_dir; ///< Directory for background file.

	// configure
	restore_t restore; ///< Configuration restore mode.

	bool quiet; ///< Quiet mode.

	// internal state
	unsigned mode_mask; ///< Mask of available modes.
	path_container sound_background; ///< List of background file.
	unsigned preview_mask; ///< Mask of preview accepted (derived for the shown games)
	const game* current_game; ///< Game at the cursor position, ==0 if none.
	const game* current_clone; ///< Clone game selected, ==0 if none.
	std::string fast; ///< Fast selection string.
	resource current_backdrop; ///< Image shown for the current game.
	resource current_sound; ///< Sound played for the current game.

	config_state();
	~config_state();

	bool load(adv_conf* config_context, bool opt_verbose);
	bool save(adv_conf* config_context) const;

	void restore_load();
	void restore_save();

	static void conf_register(adv_conf* config_context);
	static void conf_default(adv_conf* config_context);
};

// ------------------------------------------------------------------------
// config_import

class config_import {
	std::string type;
	std::string emulator;
	std::string file;
	std::string section;

	void import_ini(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string& text));
	void import_mac(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string& text));
	void import_nms(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string& text));
public:
	config_import(const std::string Atype, const std::string Aemulator,  const std::string Afile,  const std::string Asection);

	void import(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string& text));
};

#endif

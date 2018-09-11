/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include <list>

#define ADV_COPY \
	"AdvanceMENU - Copyright (C) 1999-2018 by Andrea Mazzoleni\n"

#define COMBINE_AUTO -1
#define COMBINE_NONE 0
#define COMBINE_SCALEX 6
#define COMBINE_SCALEK 7
#define COMBINE_HQ 8
#define COMBINE_XBR 9

enum clip_mode_t {
	clip_none,
	clip_single,
	clip_singleloop,
	clip_multi,
	clip_multiloop,
	clip_multiloopall
};

/// Type of sorting.
enum listsort_t {
	sort_by_group,
	sort_by_name,
	sort_by_root_name,
	sort_by_time,
	sort_by_smart_time,
	sort_by_year,
	sort_by_manufacturer,
	sort_by_type,
	sort_by_size,
	sort_by_session,
	sort_by_res,
	sort_by_info,
	sort_by_timepersession,
	sort_by_emulator
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

inline bool sort_by_smart_time_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_smart_time_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_session_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_session_less, pgame_by_desc_less, pgame_by_name_less);
}

inline bool sort_by_timepersession_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_timepersession_less, pgame_by_desc_less, pgame_by_name_less);
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

inline bool sort_by_emulator_func(const game* A, const game* B)
{
	return pgame_combine_less(A, B, pgame_by_emulator_less, pgame_by_leveldesc_less, pgame_by_desc_less);
}


typedef bool (*pgame_sort_func)(const game*, const game*);

typedef std::set<const game*, pgame_sort_func> pgame_sort_set;

/// Type of mode.
enum listmode_t {
	mode_list = 1,
	mode_tile_tiny = 2,
	mode_tile_small = 4,
	mode_tile_normal = 8,
	mode_tile_big = 16,
	mode_tile_enormous = 32,
	mode_tile_giant = 64,
	mode_full = 128,
	mode_full_mixed = 256,
	mode_tile_icon = 512,
	mode_tile_marquee = 1024,
	mode_list_mixed = 2048,
	mode_text = 4096
};

/// Type of image to display.
enum listpreview_t {
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
	saver_shutdown,
	saver_exit,
	saver_off
};

/// Type of configuration save/restore.
enum restore_t {
	restore_none, ///< Save the modification in the config file.
	restore_exit, ///< Restore the original before exiting.
	restore_idle ///< Restore the original data at the idle.
};

enum exit_t {
	exit_none,
	exit_esc,
	exit_exit,
	exit_shutdown,
	exit_all,
	exit_menu
};

struct script {
	std::string name;
	std::string text;

	script(const std::string& Aname, const std::string& Atext) : name(Aname), text(Atext) { }
};

typedef std::list<script> script_container;

class config_emulator_state {
	bool sort_set_orig; ///< If the sort is set.
	listsort_t sort_orig; ///< Original sort mode.
	bool sort_set_effective; ///< If the sort is set.
	listsort_t sort_effective; ///< Sort mode.

	bool mode_set_orig; ///< If the mode is set.
	listmode_t mode_orig; ///< Original list mode.
	bool mode_set_effective; ///< If the mode is set.
	listmode_t mode_effective; ///< List mode.

	bool preview_set_orig; ///< If the preview is set.
	listpreview_t preview_orig; ///< Original preview type selected.
	bool preview_set_effective; ///< If the preview is set.
	listpreview_t preview_effective; ///< Preview type selected.

	bool include_group_set_orig; ///< If the group include is set.
	category_container include_group_orig; ///< Original Included groups.
	bool include_group_set_effective; ///< If the group include is set.
	category_container include_group_effective; ///< Included groups.

	bool include_type_set_orig; ///< If the type include is set.
	category_container include_type_orig; ///< Original Included types.
	bool include_type_set_effective; ///< If the type include is set.
	category_container include_type_effective; ///< Included types.

	bool resetgame_set_unique; ///< If the reset is set.
	bool resetgame_unique; ///< Reset to text mode before running games.

public:
	bool load(adv_conf* config_context, const std::string& section);
	void save(adv_conf* config_context, const std::string& section);

	void restore_load();
	void restore_save();

	listsort_t sort_get();
	listmode_t mode_get();
	listpreview_t preview_get();
	const category_container& include_group_get();
	const category_container& include_type_get();

	bool sort_has();
	bool mode_has();
	bool preview_has();
	bool include_group_has();
	bool include_type_has();
	bool resetgame_has();

	void sort_unset();
	void mode_unset();
	void preview_unset();
	void include_group_unset();
	void include_type_unset();

	void sort_set(listsort_t A);
	void mode_set(listmode_t A);
	void preview_set(listpreview_t A);
	void include_group_set(const category_container& A);
	void include_type_set(const category_container& A);

	bool resetgame_get();
};

class config_state {
	bool load_game(const std::string& name, const std::string& group, const std::string& type, const std::string& time, const std::string& session, const std::string& desc);
	bool load_iterator_game(adv_conf* config_context, const std::string& tag);
	bool load_iterator_import(adv_conf* config_context, const std::string & tag, void (config_state::*set)(const game&, const std::string&), bool opt_verbose);
	bool load_iterator_script(adv_conf* config_context, const std::string& tag);

	void import_desc(const game& g, const std::string& text);
	void import_info(const game& g, const std::string& text);
	void import_type(const game& g, const std::string& text);
	void import_group(const game& g, const std::string& text);

	emulator* sub_emu; ///< Sub emu selected.

	listsort_t default_sort_orig; ///< Original sort mode.
	listsort_t default_sort_effective; ///< Sort mode.

	listmode_t default_mode_orig; ///< Original list mode.
	listmode_t default_mode_effective; ///< List mode.

	listpreview_t default_preview_orig; ///< Original preview type selected.
	listpreview_t default_preview_effective; ///< Preview type selected.

	category_container default_include_group_orig; ///< Original Included groups.
	category_container default_include_group_effective; ///< Included groups.

	category_container default_include_type_orig; ///< Original Included types.
	category_container default_include_type_effective; ///< Included types.

	emulator_container include_emu_orig; ///< Original included emulators.
	emulator_container include_emu_effective; ///< Included emulators.

	bool default_resetgame_unique; ///< Reset to text mode before running games.
	bool resetexit; ///< Reset to text mode at exit.

public:
	listsort_t sort_get();
	listmode_t mode_get();
	listpreview_t preview_get();
	const category_container& include_group_get();
	const category_container& include_type_get();
	bool resetgame_get(const game*);
	bool resetexit_get();

	void sort_set(listsort_t A);
	void mode_set(listmode_t A);
	void preview_set(listpreview_t A);
	void include_group_set(const category_container& A);
	void include_type_set(const category_container& A);

	void sub_disable();
	void sub_enable();
	bool sub_has() { return sub_emu != 0; }
	config_emulator_state& sub_get() { return sub_emu->config_get(); }

	void include_emu_set(const emulator_container& A);
	const emulator_container& include_emu_get();

	game_set gar; ///< Main game list.

	pemulator_container emu; ///< Supported emulators set.
	pemulator_container emu_active; ///< Active emulators, a subset of emu.

	pcategory_container group; ///< Group set.
	pcategory_container type; ///< Type set.

	bool lock_orig; ///< Original interface locked.
	bool lock_effective; ///< Interface locked.

	int resizeeffect; ///< Display effect

	// video
	unsigned video_sizex; ///< Preferred video x size.
	unsigned video_sizey;
	std::string video_font_text_path; ///< Font path, "auto"== default.
	std::string video_font_bar_path; ///< Font path, "auto"== default.
	std::string video_font_menu_path; ///< Font path, "auto"== default.
	int video_font_text_x; ///< Font size, "-1"==auto.
	int video_font_text_y; ///< Font size, "-1"==auto.
	int video_font_bar_x; ///< Font size, "-1"==auto.
	int video_font_bar_y; ///< Font size, "-1"==auto.
	int video_font_menu_x; ///< Font size, "-1"==auto.
	int video_font_menu_y; ///< Font size, "-1"==auto.
	double video_gamma; ///< Video gamma.
	double video_brightness; ///< Video brightness.
	unsigned video_orientation_orig; ///< Original video orientation.
	unsigned video_orientation_effective; ///< Video orientation.

	difficulty_t difficulty_orig; ///< Original difficulty selected.
	difficulty_t difficulty_effective; ///< Difficulty selected.

	bool preview_fast; ///< Fast preview mode.
	double preview_expand; ///< Max strect factor
	std::string preview_default; ///< Default preview file.
	std::string preview_default_snap; ///< Default preview file for the specified mode.
	std::string preview_default_flyer; ///< Default preview file for the specified mode.
	std::string preview_default_cabinet; ///< Default preview file for the specified mode.
	std::string preview_default_icon; ///< Default preview file for the specified mode.
	std::string preview_default_marquee; ///< Default preview file for the specified mode.
	std::string preview_default_title; ///< Default preview file for the specified mode.

	clip_mode_t clip_mode; ///< Loop the play of all the game clips and sounds.

	unsigned icon_space; ///< Space in pixels between icons.

	unsigned menu_base_orig; ///< Original cursor position.
	unsigned menu_rel_orig; ///< Original current game.
	unsigned menu_base_effective; ///< Cursor position.
	unsigned menu_rel_effective; ///< Current game.

	bool alpha_mode; ///< Use alphanumeric keys for fast move.

	unsigned mode_skip_mask; ///< Mask of modes to skip.

	unsigned idle_start_first; ///< Idle time before the first automatic start action (seconds).
	unsigned idle_start_rep; ///< Idle time before the next automatic start action (seconds).
	unsigned idle_saver_first; ///< Idle time before the first screensaver action (seconds).
	unsigned idle_saver_rep; ///< Idle time before the next screensaver action (seconds).
	saver_t idle_saver_type; ///< Screensaver type.
	unsigned repeat; // Keyboard first repeat period.
	unsigned repeat_rep; ///< Keyboard next repeat period.
	bool disable_special; ///< Disable special hotkeys.

	merge_t merge; ///< Rom merge type.

	exit_t exit_mode; ///< Exit mode

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

	unsigned ui_translucency; ///< Translucency.
	std::string ui_back; ///< User interface background
	std::string ui_help; ///< User interface help
	std::string ui_exit; ///< User interface exit image/clip
	std::string ui_startup; ///< User interface startup image/clip
	unsigned ui_left; ///< User interface left border
	int ui_x; /// < User interface space inside x, -1 for auto
	int ui_y; /// < User interface space inside y, -1 for auto
	unsigned ui_right; ///< User interface right border
	unsigned ui_top; ///< User interface top border
	unsigned ui_bottom; ///< User interface bottom border
	bool ui_autocalib; ///< Auto calibration
	bool ui_outline; ///< User interface outline around backdrops
	bool ui_top_name; ///< Put the title over the backdrop
	bool ui_top_bar; ///< User interface need top bar
	bool ui_bottom_bar; ///< User interface need bottom bar
	bool ui_scroll_bar; ///< User interface need scroll bar
	bool ui_gamename; ///< User interface game name
	std::string ui_gamemsg; ///< Message to display before a game run.
	saver_t ui_gamesaver; ///< Preview to display before a game run.
	std::string ui_game; ///< User interface game image

	bool console_mode; ///< Run in console mode with limited features.
	bool menu_key; ///< Show in the menu the associated keys.

	std::string script_menu;
	std::string script_error;
	script_container script_bag;

	config_state();
	~config_state();

	bool load(adv_conf* config_context, bool opt_verbose);
	bool save(adv_conf* config_context) const;

	void restore_load();
	void restore_save();
	void restore_save_default();

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

	void import_catver(game_set & gar, config_state & config, void (config_state::*set)(const game&, const std::string& text));
	void import_catlist(game_set & gar, config_state & config, void (config_state::*set)(const game&, const std::string& text));
	void import_mac(game_set & gar, config_state & config, void (config_state::*set)(const game&, const std::string& text));
	void import_nms(game_set & gar, config_state & config, void (config_state::*set)(const game&, const std::string& text));
public:
	config_import(const std::string Atype, const std::string Aemulator, const std::string Afile, const std::string Asection);

	void import(game_set & gar, config_state & config, void (config_state::*set)(const game&, const std::string& text));
};

#endif


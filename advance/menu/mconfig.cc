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
#include "text.h"
#include "target.h"

#include <sstream>

#include <dirent.h>

#ifdef __MSDOS__
#include <dpmi.h> /* for _go32_dpmi_* */
#endif

using namespace std;

// --------------------------------------------------------------------------
// Configuration init

static struct conf_enum_int OPTION_TRISTATE[] = {
{ "include", include },
{ "exclude", exclude },
{ "exclude_not", exclude_not }
};

static struct conf_enum_int OPTION_SORT[] = {
{ "group", sort_by_group },
{ "name", sort_by_name },
{ "parent", sort_by_root_name },
{ "time", sort_by_time },
{ "coin", sort_by_coin },
{ "year", sort_by_year },
{ "manufacturer", sort_by_manufacturer },
{ "type", sort_by_type },
{ "size", sort_by_size },
{ "resolution", sort_by_res }
};

static struct conf_enum_int OPTION_RESTORE[] = {
{ "save_at_exit", restore_none },
{ "restore_at_exit", restore_exit },
{ "restore_at_idle", restore_idle }
};

static struct conf_enum_int OPTION_MODE[] = {
{ "list", mode_list },
{ "list_mixed", mode_list_mixed },
{ "tile_small", mode_tile_small },
{ "tile_normal", mode_tile_normal },
{ "tile_big", mode_tile_big },
{ "tile_enormous", mode_tile_enormous },
{ "tile_giant", mode_tile_giant },
{ "full", mode_full },
{ "full_mixed", mode_full_mixed },
{ "tile_icon", mode_tile_icon },
{ "tile_marquee", mode_tile_marquee },
{ "text", mode_text }
};

static struct conf_enum_int OPTION_SAVER[] = {
{ "snap", saver_snap },
{ "play", saver_play },
{ "flyers", saver_flyer },
{ "cabinets", saver_cabinet },
{ "titles", saver_title },
{ "none", saver_off }
};

static struct conf_enum_int OPTION_PREVIEW[] = {
{ "snap", preview_snap },
{ "flyers", preview_flyer },
{ "cabinets", preview_cabinet },
{ "titles", preview_title }
};

static struct conf_enum_int OPTION_EVENTMODE[] = {
{ "fast", 1 },
{ "wait", 0 }
};

static struct conf_enum_int OPTION_MERGE[] = {
{ "none", merge_no },
{ "differential", merge_differential },
{ "parent", merge_parent },
{ "any", merge_any },
{ "disable", merge_disable }
};

static struct conf_enum_int OPTION_DEPTH[] = {
{ "8", 8 },
{ "15", 15 },
{ "16", 16 },
{ "32", 32 }
};

static void config_error_la(const string& line, const string& arg) {
	target_err("Invalid argument '%s' at line '%s'.\n", arg.c_str(), line.c_str());
}

static void config_error_oa(const string& opt, const string& arg) {
	target_err("Invalid argument '%s' at option '%s'.\n", arg.c_str(), opt.c_str());
}

static void config_error_a(const string& arg) {
	target_err("Invalid argument '%s'.\n", arg.c_str());
}

static bool config_import(const string& s, string& a0) {
	if (!arg_split(s, a0)) {
		config_error_a(s);
		return false;
	}

	a0 = path_import(a0);

	return true;
}

static bool config_split(const string& s, string& a0) {
	if (!arg_split(s, a0)) {
		config_error_a(s);
		return false;
	}
	return true;
}

static bool config_split(const string& s, string& a0, string& a1) {
	if (!arg_split(s, a0, a1)) {
		config_error_a(s);
		return false;
	}
	return true;
}

static bool config_split(const string& s, string& a0, string& a1, string& a2) {
	if (!arg_split(s, a0, a1, a2)) {
		config_error_a(s);
		return false;
	}
	return true;
}

static bool config_split(const string& s, string& a0, string& a1, string& a2, string& a3) {
	if (!arg_split(s, a0, a1, a2, a3)) {
		config_error_a(s);
		return false;
	}
	return true;
}

void config_state::conf_register(struct conf_context* config_context) {
	conf_string_register_multi(config_context, "emulator");
	conf_string_register_multi(config_context, "emulator_roms");
	conf_string_register_multi(config_context, "emulator_roms_filter");
	conf_string_register_multi(config_context, "emulator_altss");
	conf_string_register_multi(config_context, "emulator_flyers");
	conf_string_register_multi(config_context, "emulator_cabinets");
	conf_string_register_multi(config_context, "emulator_icons");
	conf_string_register_multi(config_context, "emulator_marquees");
	conf_string_register_multi(config_context, "emulator_titles");
	conf_string_register_multi(config_context, "emulator_include");
	conf_string_register_multi(config_context, "group");
	conf_string_register_multi(config_context, "type");
	conf_string_register_multi(config_context, "group_include");
	conf_string_register_multi(config_context, "type_include");
	conf_string_register_default(config_context,"type_import","none");
	conf_string_register_default(config_context, "group_import", "none");
	conf_string_register_default(config_context, "desc_import", "none");
	conf_string_register_multi(config_context, "game");
	conf_int_register_enum_default(config_context, "select_neogeo", conf_enum(OPTION_TRISTATE), include);
	conf_int_register_enum_default(config_context, "select_deco", conf_enum(OPTION_TRISTATE), include);
	conf_int_register_enum_default(config_context, "select_playchoice", conf_enum(OPTION_TRISTATE), include);
	conf_int_register_enum_default(config_context, "select_clone", conf_enum(OPTION_TRISTATE), exclude);
	conf_int_register_enum_default(config_context, "select_bad", conf_enum(OPTION_TRISTATE), exclude);
	conf_int_register_enum_default(config_context, "select_missing", conf_enum(OPTION_TRISTATE), include);
	conf_int_register_enum_default(config_context, "select_vector", conf_enum(OPTION_TRISTATE), include);
	conf_int_register_enum_default(config_context, "select_vertical", conf_enum(OPTION_TRISTATE), include);
	conf_int_register_enum_default(config_context, "sort", conf_enum(OPTION_SORT), sort_by_root_name);
	conf_bool_register_default(config_context, "lock", 0);
	conf_int_register_enum_default(config_context, "config", conf_enum(OPTION_RESTORE), restore_none);
	conf_int_register_enum_default(config_context, "mode", conf_enum(OPTION_MODE), mode_list);
	conf_string_register_default(config_context, "mode_skip", "");
	conf_int_register_limit_default(config_context, "event_exit_press", 0, 3, 1);
	conf_string_register_multi(config_context, "event_assign");
	conf_string_register_multi(config_context, "color");
	conf_string_register_default(config_context, "idle_start", "0 0");
	conf_string_register_default(config_context, "idle_screensaver", "60 10");
	conf_int_register_enum_default(config_context, "idle_screensaver_preview", conf_enum(OPTION_SAVER), saver_snap);
	conf_int_register_default(config_context, "menu_base", 0);
	conf_int_register_default(config_context, "menu_rel", 0);
	conf_string_register_default(config_context, "event_repeat", "500 50");
	conf_string_register_default(config_context, "msg_run", "\"Run game\"");
	conf_int_register_enum_default(config_context, "preview", conf_enum(OPTION_PREVIEW), preview_snap);
	conf_float_register_limit_default(config_context, "preview_expand", 1.0, 3.0, 1.15);
	conf_string_register_default(config_context, "preview_default", "none");
	conf_string_register_default(config_context, "preview_default_snap", "none");
	conf_string_register_default(config_context, "preview_default_flyer", "none");
	conf_string_register_default(config_context, "preview_default_cabinet", "none");
	conf_string_register_default(config_context, "preview_default_icon", "none");
	conf_string_register_default(config_context, "preview_default_marquee", "none");
	conf_string_register_default(config_context, "preview_default_title", "none");
	conf_int_register_enum_default(config_context, "event_mode", conf_enum(OPTION_EVENTMODE), 1);
	conf_bool_register_default(config_context, "event_alpha", 1);
	conf_int_register_enum_default(config_context, "merge", conf_enum(OPTION_MERGE), merge_differential);
	conf_int_register_limit_default(config_context, "icon_space", 10, 500, 43);
	conf_string_register_default(config_context, "sound_foreground_begin", "default");
	conf_string_register_default(config_context, "sound_foreground_end", "default");
	conf_string_register_default(config_context, "sound_foreground_key", "default");
	conf_string_register_default(config_context, "sound_foreground_start", "default");
	conf_string_register_default(config_context, "sound_foreground_stop", "default");
	conf_string_register_default(config_context, "sound_background_loop", "default");
	conf_string_register_default(config_context, "sound_background_begin", "none");
	conf_string_register_default(config_context, "sound_background_end", "none");
	conf_string_register_default(config_context, "sound_background_start", "none");
	conf_string_register_default(config_context, "sound_background_stop", "none");
	conf_string_register_default(config_context, "sound_background_loop_dir", "\"mp3\"");
	conf_int_register_limit_default(config_context, "video_size",160,2048,1024);
	conf_int_register_enum_default(config_context, "video_depth", conf_enum(OPTION_DEPTH), 16);
	conf_string_register_default(config_context, "video_font", "none");
	conf_string_register_default(config_context, "video_orientation", "");
	conf_float_register_limit_default(config_context,"video_gamma",0.2,5,1);
	conf_float_register_limit_default(config_context,"video_brightness",0.2,5,1);
	conf_bool_register_default(config_context,"video_restore",1);
	conf_bool_register_default(config_context,"misc_quiet",0);
}

// -------------------------------------------------------------------------
// Configuration load

static bool config_load_background_dir(const string& dir, path_container& c) {
	if (dir=="none")
		return false;

	bool almost_one = false;
	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d)
		return almost_one;

	struct dirent* dd;
	while ((dd = readdir(d))!=0) {
		string file = file_import(dd->d_name);
		if (file_ext(file) == ".mp3" || file_ext(file) == ".wav") {
			c.insert( c.end(), slash_add(dir) + file);
		}
	}

	closedir(d);
	return almost_one;
}

static bool config_load_background_list(const string& list, path_container& c) {
	bool almost_one = false;
	int i = 0;
	while (i<list.length()) {
		string dir = token_get(list,i,";");
		token_skip(list,i,";");
		almost_one = almost_one || config_load_background_dir(dir, c);
	}
	return almost_one;
}

bool config_state::load_game(const string& name, const string& group_name, const string& type_name, const string& time, const string& coin, const string& desc) {
	game_set::const_iterator i = gar.find( game( name ) );
	if (i==gar.end())
		return false;

	if (group_name != CATEGORY_UNDEFINED)
		i->user_group_set(group_name);

	if (type_name != CATEGORY_UNDEFINED)
		i->user_type_set(type_name);

	if (desc.length()!=0)
		i->user_description_set(desc);

	if (time.length()!=0 && isdigit(time[0]))
		i->time_set(atoi(time.c_str()));

	if (coin.length()!=0 && isdigit(coin[0]))
		i->coin_set(atoi(coin.c_str()));

	group.insert_double(group_name,include_group_orig);
	type.insert_double(type_name,include_type_orig);

	return true;
}

static bool config_emulator_load(const string& name, pemulator_container& emu, void (emulator::*set)(const string& s), const string& value) {
	pemulator_container::iterator i = emu.begin();
	while (i!=emu.end() && name!=(*i)->user_name_get())
		++i;
	if (i!=emu.end()) {
		((*i)->*set)(value);
		return true;
	} else {
		return false;
	}
}

static bool config_load_iterator(struct conf_context* config_context, const string& tag, bool (*func)(const string& s)) {
	conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0 = conf_iterator_string_get(&i);
		if (!func(a0)) {
			config_error_a(a0);
			return false;
		}
		conf_iterator_next(&i);
	}
	return true;
}

static bool config_load_iterator_emu_set(struct conf_context* config_context, const string& tag, pemulator_container& emu, void (emulator::*set)(const string& s)) {
	conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string s,a0,a1;
		s = conf_iterator_string_get(&i);
		if (!config_split(s,a0,a1))
			return false;
		if (!config_emulator_load(a0,emu,set,a1)) {
			config_error_a(s);
			return false;
		}
		conf_iterator_next(&i);
	}
	return true;
}

static bool config_load_skip(struct conf_context* config_context, unsigned& mask) {
	string s;
	int i;

	s = conf_string_get_default(config_context, "mode_skip");
	i = 0;
	mask = 0;
	while (i<s.length()) {
		unsigned j;
		string a0;
		a0 = arg_get(s,i);
		for(j=0;j<conf_size(OPTION_MODE);++j)
			if (a0 == OPTION_MODE[j].value)
				break;
		if (j == conf_size(OPTION_MODE)) {
			config_error_la("mode_skip " + s,a0);
			return false;
		}
		mask |= OPTION_MODE[j].map;
	}

	return true;
}

static bool config_load_iterator_emu(struct conf_context* config_context, const string& tag, pemulator_container& emu) {
	conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0,a1,a2,a3;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s,a0,a1,a2,a3))
			return false;
		if (a0.length()==0 || a1.length()==0) {
			config_error_a(s);
			return false;
		}
		emulator* e;
		if (a1 == "mame") {
			e = new wmame(a0,a2,a3);
		} else if (a1 == "dmame") {
			e = new dmame(a0,a2,a3);
		} else if (a1 == "xmame") {
			e = new xmame(a0,a2,a3);
		} else if (a1 == "dmess") {
			e = new dmess(a0,a2,a3);
		} else if (a1 == "draine") {
			e = new draine(a0,a2,a3);
		} else if (a1 == "generic") {
			e = new generic(a0,a2,a3);
		} else if (a1 == "advmame") {
			e = new advmame(a0,a2,a3);
		} else if (a1 == "advmess") {
			e = new advmess(a0,a2,a3);
		} else if (a1 == "advpac") {
			e = new advpac(a0,a2,a3);
		} else {
			config_error_la(tag + " " + s,a1);
			return false;
		}

		emu.insert(emu.end(),e);

		conf_iterator_next(&i);
	}

	return true;
}

static bool config_load_iterator_category(struct conf_context* config_context, const string& tag, category_container& cat) {
	conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s,a0))
			return false;
		cat.insert(cat.end(),a0);
		conf_iterator_next(&i);
	}
	return true;
}

static bool config_load_iterator_emu_include(struct conf_context* config_context, const string& tag, emulator_container& emu) {
	conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s,a0))
			return false;
		emu.insert(emu.end(),a0);
		conf_iterator_next(&i);
	}
	return true;
}

bool config_state::load_iterator_game(struct conf_context* config_context, const string& tag) {
	unsigned ignored_count = 0;

	conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string s = conf_iterator_string_get(&i);
		int j = 0;
		string game = arg_get(s,j);
		string group = arg_get(s,j);
		string type = arg_get(s,j);
		string time = arg_get(s,j);
		string coin = arg_get(s,j);
		string desc = arg_get(s,j);

		if (j != s.length()) {
			config_error_a(s);
			return false;
		}

		if (game.length()==0) {
			config_error_a(s);
			return false;
		}

		if (group.length()==0)
			group = CATEGORY_UNDEFINED;

		if (type.length()==0)
			type = CATEGORY_UNDEFINED;

		if (!load_game(game,group,type,time,coin,desc)) {
			++ignored_count;
			if (ignored_count < 10)
				target_err("Ignoring info for game '%s'.\n", game.c_str());
			else if (ignored_count == 10)
				target_err("And also for other games\n");
		}

		conf_iterator_next(&i);
	}

	if (ignored_count > 0)
		target_err("You may lose the game coins and time information.\n");

	return true;
}

static bool config_load_orientation(struct conf_context* config_context, unsigned& mask) {
	string s;
	int i;

	s = conf_string_get_default(config_context, "video_orientation");
	i = 0;
	mask = 0;
	while (i<s.length()) {
		string arg = arg_get(s,i);
		if (arg == "flip_xy")
			mask ^= TEXT_ORIENTATION_SWAP_XY;
		else if (arg == "mirror_x")
			mask ^= TEXT_ORIENTATION_FLIP_X;
		else if (arg == "mirror_y")
			mask ^= TEXT_ORIENTATION_FLIP_Y;
		else {
			config_error_la("video_orientation " + s,arg);
			return false;
		}
	}

	return true;
}

static bool config_is_emulator(const pemulator_container& ec, const string& emulator) {
	for(pemulator_container::const_iterator i=ec.begin();i!=ec.end();++i) {
		if ((*i)->user_name_get() == emulator)
			return true;
	}
	return false;
}

bool config_state::load(struct conf_context* config_context, bool opt_verbose) {
	string a0,a1;

	preview_mask = 0;
	current_game = 0;
	current_clone = 0;
	fast = "";
	current_backdrop = resource();
	current_sound = resource();

	if (!config_split(conf_string_get_default(config_context, "desc_import"),desc_import_type,desc_import_sub,desc_import_file))
		return false;
	desc_import_file = path_import(desc_import_file);
	if (!config_split(conf_string_get_default(config_context, "type_import"),type_import_type,type_import_sub,type_import_file,type_import_section))
		return false;
	type_import_file = path_import(type_import_file);
	if (!config_split(conf_string_get_default(config_context, "group_import"),group_import_type,group_import_sub,group_import_file,group_import_section))
		return false;
	group_import_file = path_import(group_import_file);
	exclude_neogeo_orig = (tristate_t)conf_int_get_default(config_context, "select_neogeo");
	exclude_deco_orig = (tristate_t)conf_int_get_default(config_context, "select_deco");
	exclude_playchoice_orig = (tristate_t)conf_int_get_default(config_context, "select_playchoice");
	exclude_clone_orig = (tristate_t)conf_int_get_default(config_context, "select_clone");
	exclude_bad_orig = (tristate_t)conf_int_get_default(config_context, "select_bad");
	exclude_vertical_orig = (tristate_t)conf_int_get_default(config_context, "select_vertical");
	exclude_missing_orig = (tristate_t)conf_int_get_default(config_context, "select_missing");
	exclude_vector_orig = (tristate_t)conf_int_get_default(config_context, "select_vector");
	sort_orig = (game_sort_t)conf_int_get_default(config_context, "sort");
	lock_orig = (bool)conf_bool_get_default(config_context, "lock");
	restore = (restore_t)conf_int_get_default(config_context, "config");
	mode_orig = (show_t)conf_int_get_default(config_context, "mode");
	if (!config_load_skip(config_context,mode_skip_mask))
		return false;
	exit_count = conf_int_get_default(config_context, "event_exit_press");
	if (!config_load_iterator(config_context, "event_assign", text_key_in))
		return false;
	if (!config_load_iterator(config_context, "color", text_color_in))
		return false;
	if (!config_split(conf_string_get_default(config_context, "idle_start"), a0, a1))
		return false;
	idle_start_first = atoi( a0.c_str() );
	idle_start_rep = atoi( a1.c_str() );
	menu_base_orig = conf_int_get_default(config_context, "menu_base");
	menu_rel_orig = conf_int_get_default(config_context, "menu_rel");
	if (!config_split(conf_string_get_default(config_context, "idle_screensaver"), a0, a1))
		return false;
	idle_saver_first = atoi( a0.c_str() );
	idle_saver_rep = atoi( a1.c_str() );
	if (!config_split(conf_string_get_default(config_context, "event_repeat"), a0, a1))
		return false;
	repeat = atoi( a0.c_str() );
	repeat_rep = atoi( a1.c_str() );
	video_size = conf_int_get_default(config_context, "video_size");
	video_depth = conf_int_get_default(config_context, "video_depth");
	if (!config_import(conf_string_get_default(config_context, "video_font"), a0))
		return false;
	video_font_path = a0;
	if (!config_load_orientation(config_context,video_orientation_orig))
		return false;
	video_gamma = conf_float_get_default(config_context, "video_gamma");
	video_brightness = conf_float_get_default(config_context, "video_brightness");
	video_reset_mode = conf_bool_get_default(config_context,"video_restore");
	quiet = conf_bool_get_default(config_context,"misc_quiet");
	if (!config_split(conf_string_get_default(config_context, "msg_run"),msg_run_game))
		return false;
	preview_orig = (preview_t)conf_int_get_default(config_context, "preview");
	idle_saver_type = (saver_t)conf_int_get_default(config_context, "idle_screensaver_preview");
	preview_expand = conf_float_get_default(config_context, "preview_expand");
	if (!config_import(conf_string_get_default(config_context, "preview_default"),preview_default))
		return false;
	if (!config_import(conf_string_get_default(config_context, "preview_default_snap"),preview_default_snap))
		return false;
	if (!config_import(conf_string_get_default(config_context, "preview_default_flyer"),preview_default_flyer))
		return false;
	if (!config_import(conf_string_get_default(config_context, "preview_default_cabinet"),preview_default_cabinet))
		return false;
	if (!config_import(conf_string_get_default(config_context, "preview_default_icon"),preview_default_icon))
		return false;
	if (!config_import(conf_string_get_default(config_context, "preview_default_marquee"),preview_default_marquee))
		return false;
	if (!config_import(conf_string_get_default(config_context, "preview_default_title"),preview_default_title))
		return false;
	preview_fast = (bool)conf_int_get_default(config_context, "event_mode");
	alpha_mode = (bool)conf_bool_get_default(config_context, "event_alpha");
	merge = (merge_t)conf_int_get_default(config_context, "merge");
	icon_space = conf_int_get_default(config_context, "icon_space");

	if (!config_import(conf_string_get_default(config_context, "sound_foreground_begin"),sound_foreground_begin))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_foreground_end"),sound_foreground_end))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_foreground_key"),sound_foreground_key))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_foreground_start"),sound_foreground_start))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_foreground_stop"),sound_foreground_stop))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_background_begin"),sound_background_begin))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_background_end"),sound_background_end))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_background_start"),sound_background_start))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_background_stop"),sound_background_stop))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_background_loop"),sound_background_loop))
		return false;
	if (!config_import(conf_string_get_default(config_context, "sound_background_loop_dir"),sound_background_loop_dir))
		return false;
	if (!config_load_iterator_emu(config_context, "emulator", emu))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_roms", emu,&emulator::user_rom_path_set))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_roms_filter", emu, &emulator::user_rom_filter_set))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_altss", emu, &emulator::user_alts_path_set))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_flyers", emu, &emulator::user_flyer_path_set))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_cabinets", emu, &emulator::user_cabinet_path_set))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_icons", emu, &emulator::user_icon_path_set))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_marquees", emu, &emulator::user_marquee_path_set))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_titles", emu, &emulator::user_title_path_set))
		return false;

	// print the copyright message before other messages
	if (!quiet) {
		target_nfo("AdvanceMENU - Copyright (C) 1999-2002 by Andrea Mazzoleni\n");
#ifdef __MSDOS__
		target_nfo("%d [Mb] free physical memory, %d [Mb] free virtual memory\n", (unsigned)_go32_dpmi_remaining_physical_memory()/(1024*1024), (unsigned)_go32_dpmi_remaining_virtual_memory()/(1024*1024));
#endif
	}

	// select the active emulators
	for(pemulator_container::iterator i=emu.begin();i!=emu.end();++i) {
		if ((*i)->is_ready())
			emu_active.insert(emu_active.end(),*i);
		else
			target_err("Emulator '%s' not found.\n", (*i)->user_exe_path_get().c_str());
	}

	if (emu_active.size() == 0) {
		target_err("No emulator found. Add an `emulator' option in your configuration file.\n");
		target_err("These options are documented in the `advmenu.txt' file.\n");
		return false;
	}

	// load the game definitions
	for(pemulator_container::iterator i=emu_active.begin();i!=emu_active.end();++i) {
		if (opt_verbose)
			target_nfo("log: load game for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_game(gar)) {
			return false;
		}
	}

	// load the emulator configurations
	for(pemulator_container::iterator i=emu_active.begin();i!=emu_active.end();++i) {
		if (opt_verbose)
			target_nfo("log: load cfg for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_cfg(gar)) {
			return false;
		}
	}

	// load the software definitions
	for(pemulator_container::iterator i=emu_active.begin();i!=emu_active.end();++i) {
		if (opt_verbose)
			target_nfo("log: load software for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_software(gar)) {
			exit(EXIT_FAILURE);
		}
	}

	if (opt_verbose)
		target_nfo("log: adjust list\n");

	// compile the relations
	gar.sync_relationships();

	// set the previews
	for(pemulator_container::iterator i=emu_active.begin();i!=emu_active.end();++i) {
		if (opt_verbose)
			target_nfo("log: load preview for %s\n", (*i)->user_name_get().c_str());
		(*i)->preview_set(gar);
	}

	if (opt_verbose)
		target_nfo("log: load group and types\n");

	// load the group/type informations
	if (!config_load_iterator_category(config_context,"group",group))
		return false;
	if (!config_load_iterator_category(config_context,"type",type))
		return false;
	if (!config_load_iterator_category(config_context,"group_include",include_group_orig))
		return false;
	if (!config_load_iterator_category(config_context,"type_include",include_type_orig))
		return false;

	group.insert_double(CATEGORY_UNDEFINED,include_group_orig);
	type.insert_double(CATEGORY_UNDEFINED,include_type_orig);
	if (include_group_orig.size() == 0)
		include_group_orig = group;
	if (include_type_orig.size() == 0)
		include_type_orig = type;

	if (opt_verbose)
		target_nfo("log: load games info\n");

	if (!load_iterator_game(config_context, "game"))
		return false;

	// load the emulator active
	if (!config_load_iterator_emu_include(config_context,"emulator_include",include_emu_orig))
		return false;

	if (include_emu_orig.size() == 0) {
		for(pemulator_container::iterator i=emu_active.begin();i!=emu_active.end();++i) {
			include_emu_orig.insert(include_emu_orig.end(),(*i)->user_name_get());
		}
	}

	// load the emulator data
	for(pemulator_container::iterator i=emu_active.begin();i!=emu_active.end();++i) {
		if (opt_verbose)
			target_nfo("log: load data for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_data(gar)) {
			exit(EXIT_FAILURE);
		}
	}

	if (desc_import_type != "none") {
		if (!config_is_emulator(emu,desc_import_sub)) {
			config_error_oa("desc_import",desc_import_sub);
			return false;
		}
	}
	if (desc_import_type == "nms") {
		if (opt_verbose)
			target_nfo("log: importing from %s\n", desc_import_file.c_str());
		gar.import_nms(desc_import_file,desc_import_sub,&game::auto_description_set);
	} else if (desc_import_type != "none") {
		config_error_oa("desc_import", desc_import_type);
		return false;
	}

	if (type_import_type != "none") {
		if (!config_is_emulator(emu,type_import_sub)) {
			config_error_oa("type_import",type_import_sub);
			return false;
		}
	}
	if (type_import_type == "ini") {
		if (opt_verbose)
			target_nfo("log: importing from %s\n", type_import_file.c_str());
		type.import_ini(gar,type_import_file,type_import_section,type_import_sub,&game::auto_type_set,include_type_orig);
	} else if (type_import_type == "mac") {
		if (opt_verbose)
			target_nfo("log: importing from %s\n", type_import_file.c_str());
		type.import_mac(gar,type_import_file,type_import_section,type_import_sub,&game::auto_type_set,include_type_orig);
	} else if (type_import_type != "none") {
		config_error_oa("type_import", type_import_type);
		return false;
	}

	if (group_import_type != "none") {
		if (!config_is_emulator(emu,group_import_sub)) {
			config_error_oa("group_import",group_import_sub);
			return false;
		}
	}
	if (group_import_type == "ini") {
		if (opt_verbose)
			target_nfo("log: importing from %s\n", group_import_file.c_str());
		group.import_ini(gar,group_import_file,group_import_section,group_import_sub,&game::auto_group_set,include_group_orig);
	} else if (group_import_type == "mac") {
		if (opt_verbose)
			target_nfo("log: importing from %s\n", group_import_file.c_str());
		group.import_mac(gar,group_import_file,group_import_section,group_import_sub,&game::auto_group_set,include_group_orig);
	} else if (group_import_type != "none") {
		config_error_oa("group_import", group_import_type);
		return false;
	}

	if (opt_verbose)
		target_nfo("log: load background music list\n");

	config_load_background_list(sound_background_loop_dir,sound_background);

	if (opt_verbose)
		target_nfo("log: start\n");

	return true;
}

void config_state::conf_default(struct conf_context* config_context) {
	conf_iterator i;

	conf_iterator_begin(&i, config_context, "emulator");
	if (conf_iterator_is_end(&i)) {
		char path[FILE_MAXPATH];
#if defined(__MSDOS__) || defined(__WIN32__)
		if (target_search(path,FILE_MAXPATH,"advmame.exe") == 0) {
			conf_set(config_context,"","emulator", "\"advmame\" advmame \"advmame.exe\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"advmess.exe") == 0) {
			conf_set(config_context,"","emulator", "\"advmess\" advmess \"advmess.exe\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"advpac.exe") == 0) {
			conf_set(config_context,"","emulator", "\"advpac\" advpac \"advpac.exe\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"dmame.exe") == 0) {
			conf_set(config_context,"","emulator", "\"dmame\" dmame \"dmame.exe\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"mame.exe") == 0) {
			conf_set(config_context,"","emulator", "\"mame\" mame \"mame.exe\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"raine.exe") == 0) {
			conf_set(config_context,"","emulator", "\"draine\" draine \"raine.exe\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"snes9x.exe") == 0) {
			conf_set(config_context,"","emulator", "\"snes9x\" generic \"snes9x.exe\" \"%f\"");
			conf_set(config_context,"","emulator_roms", "\"snes9x\" \"roms\"");
		}
		if (target_search(path,FILE_MAXPATH,"zsnes.exe") == 0) {
			conf_set(config_context,"","emulator", "\"zsnes\" generic \"zsnes.exe\" \"-e -m roms\\%f\"");
			conf_set(config_context,"","emulator_roms", "\"zsnes\" \"roms\"");
		}
#else
		if (target_search(path,FILE_MAXPATH,"advmame") == 0) {
			conf_set(config_context,"","emulator","\"advmame\" advmame \"advmame\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"advmess") == 0) {
			conf_set(config_context,"","emulator","\"advmess\" advmess \"advmess\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"advpac") == 0) {
			conf_set(config_context,"","emulator","\"advpac\" advpac \"advpac\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"xmame") == 0) {
			conf_set(config_context,"","emulator","\"xmame\" xmame \"xmame\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"xmame.x11") == 0) {
			conf_set(config_context,"","emulator","\"xmame.x11\" xmame \"xmame.x11\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"xmame.SDL") == 0) {
			conf_set(config_context,"","emulator","\"xmame.SDL\" xmame \"xmame.SDL\" \"\"");
		}
		if (target_search(path,FILE_MAXPATH,"xmame.svgalib") == 0) {
			conf_set(config_context,"","emulator","\"xmame.svgalib\" xmame \"xmame.svgalib\" \"\"");
		}
#endif
	}

	conf_iterator_begin(&i, config_context, "group");
	if (conf_iterator_is_end(&i)) {
		conf_set(config_context,"","group","\"Very Good\"");
		conf_set(config_context,"","group","\"Good\"");
		conf_set(config_context,"","group","\"Bad\"");
		conf_set(config_context,"","group","\"<undefined>\"");
	}

	conf_iterator_begin(&i, config_context, "type");
	if (conf_iterator_is_end(&i)) {
		conf_set(config_context,"","type","\"Computer\"");
		conf_set(config_context,"","type","\"Console\"");
		conf_set(config_context,"","type","\"Application\"");
		conf_set(config_context,"","type","\"Arcade\"");
		conf_set(config_context,"","type","\"Shot 'em Up\"");
		conf_set(config_context,"","type","\"Bet 'em Up\"");
		conf_set(config_context,"","type","\"Fight\"");
		conf_set(config_context,"","type","\"Gun\"");
		conf_set(config_context,"","type","\"Puzzle\"");
		conf_set(config_context,"","type","\"RPG\"");
		conf_set(config_context,"","type","\"Sport\"");
		conf_set(config_context,"","type","\"Breakout\"");
		conf_set(config_context,"","type","\"Filler\"");
		conf_set(config_context,"","type","\"Racing\"");
		conf_set(config_context,"","type","\"Flipper\"");
		conf_set(config_context,"","type","\"<undefined>\"");
	}

	conf_iterator_begin(&i, config_context, "color");
	if (conf_iterator_is_end(&i)) {
		text_color_out(config_context, "color");
	}

	conf_iterator_begin(&i, config_context, "event_assign");
	if (conf_iterator_is_end(&i)) {
		text_key_out(config_context, "event_assign");
	}
}

// -------------------------------------------------------------------------
// Configuration save

static string config_out(const string& a0) {
	return "\"" + a0 + "\"";
}

bool config_state::save(struct conf_context* config_context) const {
	conf_int_set(config_context,"","mode",mode_orig);
	conf_int_set(config_context,"","menu_base",menu_base_orig);
	conf_int_set(config_context,"","menu_rel",menu_rel_orig);
	conf_int_set(config_context,"","sort",sort_orig);
	conf_int_set(config_context,"","preview",preview_orig);

	conf_remove(config_context,"","emulator_include");
	for(emulator_container::const_iterator i=include_emu_orig.begin();i!=include_emu_orig.end();++i) {
		conf_string_set(config_context,"","emulator_include",config_out(*i).c_str());
	}

	conf_remove(config_context,"","group_include");
	for(category_container::const_iterator i=include_group_orig.begin();i!=include_group_orig.end();++i) {
		conf_string_set(config_context,"","group_include",config_out(*i).c_str());
	}

	conf_remove(config_context,"","type_include");
	for(category_container::const_iterator i=include_type_orig.begin();i!=include_type_orig.end();++i) {
		conf_string_set(config_context,"","type_include",config_out(*i).c_str());
	}

	conf_int_set(config_context,"","select_neogeo",exclude_neogeo_orig);
	conf_int_set(config_context,"","select_deco",exclude_deco_orig);
	conf_int_set(config_context,"","select_playchoice",exclude_playchoice_orig);
	conf_int_set(config_context,"","select_clone",exclude_clone_orig);
	conf_int_set(config_context,"","select_bad",exclude_bad_orig);
	conf_int_set(config_context,"","select_missing",exclude_missing_orig);
	conf_int_set(config_context,"","select_vector",exclude_vector_orig);
	conf_int_set(config_context,"","select_vertical",exclude_vertical_orig);

	string s;
	if ((video_orientation_orig & TEXT_ORIENTATION_SWAP_XY) != 0) {
		if (s.length()) s += " ";
		s += "flip_xy";
	}
	if ((video_orientation_orig & TEXT_ORIENTATION_FLIP_X) != 0) {
		if (s.length()) s += " ";
		s += "mirror_x";
	}
	if ((video_orientation_orig & TEXT_ORIENTATION_FLIP_Y) != 0) {
		if (s.length()) s += " ";
		s += "mirror_y";
	}
	conf_string_set(config_context, "", "video_orientation", s.c_str());

	conf_remove(config_context,"","game");
	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		if (0
			|| (i->is_user_group_set() && i->group_get().length()!=0)
			|| (i->is_user_type_set() && i->type_get().length()!=0)
			|| (i->is_time_set() && i->time_get()!=0)
			|| (i->is_coin_set() && i->coin_get()!=0)
			|| (i->is_user_description_set() && i->description_get().length()!=0)
		) {
			ostringstream f;
			f << "\"" << i->name_get() << "\"";

			f << " \"";
			if (i->is_user_group_set())
				f << i->group_get();
			f << "\"";

			f << " \"";
			if (i->is_user_type_set())
				f << i->type_get();
			f << "\"";

			f << " " << i->time_get();

			f << " " << i->coin_get();

			f << " \"";
			if (i->is_user_description_set()) {
				 f << i->description_get();
			}
			f << "\"";

			conf_string_set(config_context,"","game",f.str().c_str());
		}
	}

	if (conf_save(config_context,1) != 0)
		return false;

	// prevent data lost if crashing
	target_sync();

	return true;
}

// ------------------------------------------------------------------------
// Configuration restore

void config_state::restore_load() {
	mode_effective = mode_orig;
	preview_effective = preview_orig;
	sort_effective = sort_orig;
	exclude_neogeo_effective = exclude_neogeo_orig;
	exclude_deco_effective = exclude_deco_orig;
	exclude_playchoice_effective = exclude_playchoice_orig;
	exclude_clone_effective = exclude_clone_orig;
	exclude_bad_effective = exclude_bad_orig;
	exclude_missing_effective = exclude_missing_orig;
	exclude_vector_effective = exclude_vector_orig;
	exclude_vertical_effective = exclude_vertical_orig;
	include_group_effective = include_group_orig;
	include_type_effective = include_type_orig;
	include_emu_effective = include_emu_orig;
	menu_base_effective = menu_base_orig;
	menu_rel_effective = menu_rel_orig;
	lock_effective = lock_orig;
	video_orientation_effective = video_orientation_orig;
}

void config_state::restore_save() {
	mode_orig = mode_effective;
	preview_orig = preview_effective;
	sort_orig = sort_effective;
	exclude_neogeo_orig = exclude_neogeo_effective;
	exclude_deco_orig = exclude_deco_effective;
	exclude_playchoice_orig = exclude_playchoice_effective;
	exclude_clone_orig = exclude_clone_effective;
	exclude_bad_orig = exclude_bad_effective;
	exclude_missing_orig = exclude_missing_effective;
	exclude_vector_orig = exclude_vector_effective;
	exclude_vertical_orig = exclude_vertical_effective;
	include_group_orig = include_group_effective;
	include_type_orig = include_type_effective;
	include_emu_orig = include_emu_effective;
	menu_base_orig = menu_base_effective;
	menu_rel_orig = menu_rel_effective;
	lock_orig = lock_effective;
	video_orientation_orig = video_orientation_effective;
}

// ------------------------------------------------------------------------
// Configuration state

config_state::config_state() {
}

config_state::~config_state() {
	// delete the emulator pointer
	for(pemulator_container::iterator i=emu.begin();i!=emu.end();++i) {
		delete *i;
	}
}

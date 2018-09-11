/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2008 Andrea Mazzoleni
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
#include "text.h"

#include "advance.h"

#include <sstream>

using namespace std;

// --------------------------------------------------------------------------
// Configuration init

static adv_conf_enum_int OPTION_SORT[] = {
	{ "group", sort_by_group },
	{ "name", sort_by_name },
	{ "parent", sort_by_root_name },
	{ "time", sort_by_time },
	{ "smart_time", sort_by_smart_time },
	{ "play", sort_by_session },
	{ "year", sort_by_year },
	{ "manufacturer", sort_by_manufacturer },
	{ "type", sort_by_type },
	{ "size", sort_by_size },
	{ "resolution", sort_by_res },
	{ "info", sort_by_info },
	{ "timeperplay", sort_by_timepersession },
	{ "emulator", sort_by_emulator }
};

static adv_conf_enum_int OPTION_MODE[] = {
	{ "list", mode_list },
	{ "list_mixed", mode_list_mixed },
	{ "tile_tiny", mode_tile_tiny },
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

static adv_conf_enum_int OPTION_PREVIEW[] = {
	{ "snap", preview_snap },
	{ "flyers", preview_flyer },
	{ "cabinets", preview_cabinet },
	{ "titles", preview_title }
};

static adv_conf_enum_int OPTION_RESTORE[] = {
	{ "save_at_exit", restore_none },
	{ "restore_at_exit", restore_exit },
	{ "restore_at_idle", restore_idle }
};

static adv_conf_enum_int OPTION_EXIT[] = {
	{ "none", exit_none },
	{ "esc", exit_esc },
	{ "exit", exit_exit },
	{ "shutdown", exit_shutdown },
	{ "all", exit_all },
	{ "menu", exit_menu }
};

static adv_conf_enum_int OPTION_SAVER[] = {
	{ "snap", saver_snap },
	{ "play", saver_play },
	{ "flyers", saver_flyer },
	{ "cabinets", saver_cabinet },
	{ "titles", saver_title },
	{ "shutdown", saver_shutdown },
	{ "exit", saver_exit },
	{ "none", saver_off }
};

static adv_conf_enum_int OPTION_GAMESAVER[] = {
	{ "snap", saver_snap },
	{ "flyers", saver_flyer },
	{ "cabinets", saver_cabinet },
	{ "titles", saver_title },
	{ "none", saver_off }
};

static adv_conf_enum_int OPTION_DIFFICULTY[] = {
	{ "none", difficulty_none },
	{ "easiest", difficulty_easiest },
	{ "easy", difficulty_easy },
	{ "normal", difficulty_medium },
	{ "hard", difficulty_hard },
	{ "hardest", difficulty_hardest }
};

static adv_conf_enum_int OPTION_EVENTMODE[] = {
	{ "fast", 1 },
	{ "wait", 0 }
};

static adv_conf_enum_int OPTION_MERGE[] = {
	{ "none", merge_no },
	{ "differential", merge_differential },
	{ "parent", merge_parent },
	{ "any", merge_any },
	{ "disable", merge_disable }
};

static adv_conf_enum_int OPTION_CLIPMODE[] = {
	{ "none", clip_none },
	{ "single", clip_single },
	{ "singleloop", clip_singleloop },
	{ "multi", clip_multi },
	{ "multiloop", clip_multiloop },
	{ "multiloopall", clip_multiloopall }
};

static adv_conf_enum_int OPTION_RESIZEEFFECT[] = {
	{ "auto", COMBINE_AUTO },
	{ "none", COMBINE_NONE },
	{ "scalex", COMBINE_SCALEX },
	{ "scalek", COMBINE_SCALEK },
	{ "hq", COMBINE_HQ },
	{ "xbr", COMBINE_XBR },
};

static void config_error_la(const string& line, const string& arg)
{
	target_err("Invalid argument '%s' at line '%s'.\n", arg.c_str(), line.c_str());
}

static void config_error_oa(const string& opt, const string& arg)
{
	target_err("Invalid argument '%s' at option '%s'.\n", arg.c_str(), opt.c_str());
}

static void config_error_a(const string& arg)
{
	target_err("Invalid argument '%s'.\n", arg.c_str());
}

static bool config_path_import(const string& s, string& a0)
{
	if (!arg_split(s, a0)) {
		config_error_a(s);
		return false;
	}

	if (a0 == "none" || a0 == "default")
		return true;

	a0 = path_import(file_config_file_home(a0.c_str()));

	return true;
}

static bool config_path(const string& s, string& a0)
{
	if (!arg_split(s, a0)) {
		config_error_a(s);
		return false;
	}

	if (a0 == "none" || a0 == "default")
		return true;

	a0 = file_config_file_home(a0.c_str());

	return true;
}

static bool config_split(const string& s, string& a0)
{
	if (!arg_split(s, a0)) {
		config_error_a(s);
		return false;
	}
	return true;
}

static bool config_split(const string& s, string& a0, string& a1)
{
	if (!arg_split(s, a0, a1)) {
		config_error_a(s);
		return false;
	}
	return true;
}

static bool config_split(const string& s, string& a0, string& a1, string& a2)
{
	if (!arg_split(s, a0, a1, a2)) {
		config_error_a(s);
		return false;
	}
	return true;
}

static bool config_split(const string& s, string& a0, string& a1, string& a2, string& a3)
{
	if (!arg_split(s, a0, a1, a2, a3)) {
		config_error_a(s);
		return false;
	}
	return true;
}

static string config_out(const string& a)
{
	return "\"" + a + "\"";
}

static string config_normalize(const string& a)
{
	string r;
	for (unsigned i = 0; i < a.length(); ++i)
		if (isspace(a[i]) || a[i] == '/')
			r += '_';
		else
			r += tolower(a[i]);
	return r;
}

void config_state::conf_register(adv_conf* config_context)
{
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
	conf_string_register_multi(config_context, "emulator_attrib");
	conf_string_register_multi(config_context, "group");
	conf_string_register_multi(config_context, "type");
	conf_string_register_multi(config_context, "group_include");
	conf_string_register_multi(config_context, "type_include");
	conf_string_register_multi(config_context, "type_import");
	conf_string_register_multi(config_context, "group_import");
	conf_string_register_multi(config_context, "desc_import");
	conf_string_register_multi(config_context, "info_import");
	conf_string_register_multi(config_context, "game");
	conf_int_register_enum_default(config_context, "sort", conf_enum(OPTION_SORT), sort_by_root_name);
	conf_bool_register_default(config_context, "lock", 0);
	conf_int_register_enum_default(config_context, "config", conf_enum(OPTION_RESTORE), restore_none);
	conf_int_register_enum_default(config_context, "mode", conf_enum(OPTION_MODE), mode_list);
	conf_string_register_default(config_context, "mode_skip", "");
	conf_int_register_enum_default(config_context, "misc_exit", conf_enum(OPTION_EXIT), exit_esc);
	conf_bool_register_default(config_context, "ui_console", 0);
	conf_bool_register_default(config_context, "ui_menukey", 1);
	conf_string_register_multi(config_context, "event_assign");
	conf_string_register_multi(config_context, "ui_color");
	conf_string_register_default(config_context, "idle_start", "0 0");
	conf_string_register_default(config_context, "idle_screensaver", "60 14");
	conf_int_register_enum_default(config_context, "idle_screensaver_preview", conf_enum(OPTION_SAVER), saver_play);
	conf_int_register_default(config_context, "menu_base", 0);
	conf_int_register_default(config_context, "menu_rel", 0);
	conf_string_register_default(config_context, "event_repeat", "500 50");
	conf_bool_register_default(config_context, "input_hotkey", 1);
	conf_string_register_default(config_context, "ui_gamemsg", "\"Loading\"");
	conf_int_register_enum_default(config_context, "ui_game", conf_enum(OPTION_GAMESAVER), saver_snap);
	conf_int_register_enum_default(config_context, "difficulty", conf_enum(OPTION_DIFFICULTY), difficulty_none);
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
	conf_int_register_enum_default(config_context, "ui_clip", conf_enum(OPTION_CLIPMODE), clip_multiloopall);
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
	conf_string_register_default(config_context, "display_size", "auto");
	conf_string_register_default(config_context, "ui_text_font", "auto");
	conf_string_register_default(config_context, "ui_text_size", "auto");
	conf_string_register_default(config_context, "ui_bar_font", "auto");
	conf_string_register_default(config_context, "ui_bar_size", "auto");
	conf_string_register_default(config_context, "ui_menu_font", "auto");
	conf_string_register_default(config_context, "ui_menu_size", "auto");
	conf_string_register_default(config_context, "display_orientation", "");
	conf_float_register_limit_default(config_context, "display_gamma", 0.2, 5, 1);
	conf_float_register_limit_default(config_context, "display_brightness", 0.2, 5, 1);
	conf_bool_register_default(config_context, "display_restoreatgame", 1);
	conf_bool_register_default(config_context, "display_restoreatexit", 1);
	conf_int_register_enum_default(config_context, "display_resizeeffect", conf_enum(OPTION_RESIZEEFFECT), COMBINE_AUTO);
	conf_bool_register_default(config_context, "misc_quiet", 0);
	conf_float_register_limit_default(config_context, "ui_translucency", 0, 1, 0.6);
	conf_string_register_default(config_context, "ui_background", "none");
	conf_string_register_default(config_context, "ui_help", "none");
	conf_string_register_default(config_context, "ui_exit", "none");
	conf_string_register_default(config_context, "ui_startup", "none");
	conf_int_register_limit_default(config_context, "ui_skipbottom", 0, 1000, 0);
	conf_int_register_limit_default(config_context, "ui_skiptop", 0, 1000, 0);
	conf_int_register_limit_default(config_context, "ui_skipleft", 0, 1000, 0);
	conf_int_register_limit_default(config_context, "ui_skipright", 0, 1000, 0);
	conf_string_register_default(config_context, "ui_skiphorz", "auto");
	conf_string_register_default(config_context, "ui_skipvert", "auto");
	conf_bool_register_default(config_context, "ui_autocalib", 0);
	conf_bool_register_default(config_context, "ui_outline", 1);
	conf_bool_register_default(config_context, "ui_topname", 0);
	conf_bool_register_default(config_context, "ui_topbar", 1);
	conf_bool_register_default(config_context, "ui_bottombar", 1);
	conf_bool_register_default(config_context, "ui_scrollbar", 1);
	conf_bool_register_default(config_context, "ui_name", 1);
	conf_string_register_default(config_context, "ui_command_menu", "Command...");
	conf_string_register_default(config_context, "ui_command_error", "Error running the command");
	conf_string_register_multi(config_context, "ui_command");
}

// -------------------------------------------------------------------------
// Configuration load

static bool config_is_emulator(const pemulator_container& ec, const string& emulator)
{
	for (pemulator_container::const_iterator i = ec.begin(); i != ec.end(); ++i) {
		if ((*i)->user_name_get() == emulator)
			return true;
	}
	return false;
}

static bool config_load_background_dir(const string& dir, path_container& c)
{
	bool almost_one = false;

	log_std(("menu: load background music dir %s\n", dir.c_str()));

	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d)
		return almost_one;

	struct dirent* dd;
	while ((dd = readdir(d)) != 0) {
		string file = file_import(dd->d_name);
		if (file_ext(file) == ".mp3" || file_ext(file) == ".wav") {
			string path = slash_add(dir) + file;

			log_std(("menu: load background music file %s\n", path.c_str()));

			c.insert(c.end(), path);
		}
	}

	closedir(d);
	return almost_one;
}

static bool config_load_background_list(const string& list, path_container& c)
{
	bool almost_one = false;
	int i = 0;

	while (i < list.length()) {
		char sep[2];
		sep[0] = file_dir_separator();
		sep[1] = 0;

		string arg = token_get(list, i, sep);

		token_skip(list, i, sep);

		string dir;

		if (config_path_import(arg, dir)) {
			if (config_load_background_dir(dir, c))
				almost_one = true;
		}
	}

	return almost_one;
}

bool config_state::load_game(const string& name, const string& group_name, const string& type_name, const string& time, const string& session, const string& desc)
{
	game_set::const_iterator i = gar.find(game(name));
	if (i == gar.end())
		return false;

	if (group_name.length())
		i->user_group_set(group.insert(group_name));

	if (type_name.length())
		i->user_type_set(type.insert(type_name));

	if (desc.length() != 0)
		i->user_description_set(desc);

	if (time.length() != 0) {
		int n = atoi(time.c_str());
		if (n > 0) {
			if (i->is_time_set())
				n += i->time_get();
			i->time_set(n);
		}
	}

	if (session.length() != 0) {
		int n = atoi(session.c_str());
		if (n > 0) {
			if (i->is_session_set())
				n += i->session_get();
			i->session_set(n);
		}
	}

	return true;
}

static bool config_emulator_load(const string& name, pemulator_container& emu, void (emulator::*set)(const string& s), const string& value)
{
	pemulator_container::iterator i = emu.begin();
	while (i != emu.end() && name != (*i)->user_name_get())
		++i;
	if (i != emu.end()) {
		((*i)->*set)(value);
		return true;
	} else {
		return false;
	}
}

static bool config_emulator_attrib_load(const string& name, pemulator_container& emu, const string& value0, const string& value1)
{
	pemulator_container::iterator i = emu.begin();
	while (i != emu.end() && name != (*i)->user_name_get())
		++i;
	if (i != emu.end()) {
		if (!(*i)->attrib_set(value0, value1))
			return false;
		return true;
	} else {
		return false;
	}
}

static bool config_load_iterator(adv_conf* config_context, const string& tag, bool (*func)(const string& s))
{
	adv_conf_iterator i;
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

static bool config_load_iterator_emu_set(adv_conf* config_context, const string& tag, pemulator_container& emu, void (emulator::*set)(const string& s))
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string s, a0, a1;
		s = conf_iterator_string_get(&i);
		if (!config_split(s, a0, a1))
			return false;
		if (!config_emulator_load(a0, emu, set, a1)) {
			config_error_a(s);
			return false;
		}
		conf_iterator_next(&i);
	}
	return true;
}

static bool config_load_iterator_emu_attrib(adv_conf* config_context, const string& tag, pemulator_container& emu)
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string s, a0, a1, a2;
		s = conf_iterator_string_get(&i);
		if (!config_split(s, a0, a1, a2))
			return false;
		if (!config_emulator_attrib_load(a0, emu, a1, a2)) {
			config_error_a(s);
			return false;
		}
		conf_iterator_next(&i);
	}
	return true;
}

static bool config_load_skip(adv_conf* config_context, unsigned& mask)
{
	string s;
	int i;

	s = conf_string_get_default(config_context, "mode_skip");
	i = 0;
	mask = 0;
	while (i < s.length()) {
		unsigned j;
		string a0;
		a0 = arg_get(s, i);
		for (j = 0; j < conf_size(OPTION_MODE); ++j)
			if (a0 == OPTION_MODE[j].value)
				break;
		if (j == conf_size(OPTION_MODE)) {
			config_error_la("mode_skip " + s, a0);
			return false;
		}
		mask |= OPTION_MODE[j].map;
	}

	return true;
}

static bool config_load_iterator_emu(adv_conf* config_context, const string& tag, pemulator_container& emu)
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0, a1, a2, a3;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s, a0, a1, a2, a3))
			return false;
		if (a0.length() == 0 || a1.length() == 0) {
			config_error_a(s);
			return false;
		}
		emulator* e;
		if (a1 == "mame") {
#if !USE_NIX
			e = new wmame(a0, a2, a3);
#else
			e = new umame(a0, a2, a3);
#endif
#if !USE_NIX
		} else if (a1 == "dmame") {
			e = new dmame(a0, a2, a3);
		} else if (a1 == "dmess") {
			e = new dmess(a0, a2, a3);
		} else if (a1 == "draine") {
			e = new draine(a0, a2, a3);
#else
		} else if (a1 == "sdlmame") { // deprecated name but still supported
			e = new umame(a0, a2, a3);
#endif
		} else if (a1 == "generic") {
			e = new generic(a0, a2, a3);
		} else if (a1 == "advmame") {
			e = new advmame(a0, a2, a3);
		} else if (a1 == "advmess") {
			e = new advmess(a0, a2, a3);
		} else if (a1 == "advpac") {
			e = new advpac(a0, a2, a3);
		} else {
			config_error_la(tag + " " + s, a1);
			return false;
		}

		emu.insert(emu.end(), e);

		conf_iterator_next(&i);
	}

	return true;
}

static bool config_load_iterator_pcategory(adv_conf* config_context, const string& tag, pcategory_container& cat)
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s, a0))
			return false;
		cat.insert(a0);
		conf_iterator_next(&i);
	}
	return true;
}

bool config_state::load_iterator_import(adv_conf* config_context, const string& tag, void (config_state::*set)(const game&, const string&), bool opt_verbose)
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0, a1, a2, a3;
		string s = conf_iterator_string_get(&i);

		if (!config_split(s, a0, a1, a2, a3))
			return false;

		if (a0 != "nms" && a0 != "ini" && a0 != "mac" && a0 != "catver" && a0 != "catlist") {
			config_error_oa(tag, a0);
			return false;
		}

		if (!config_is_emulator(emu, a1)) {
			config_error_oa(tag, a1);
			return false;
		}

		if (a2.length() == 0) {
			config_error_oa(tag, a2);
			return false;
		}

		a2 = path_import(a2);

		config_import j(a0, a1, a2, a3);

		if (opt_verbose)
			target_nfo("log: importing from %s\n", a2.c_str());

		j.import(gar, *this, set);

		conf_iterator_next(&i);
	}

	return true;
}

static bool config_load_iterator_category(adv_conf* config_context, const string& tag, category_container& cat)
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s, a0))
			return false;
		cat.insert(cat.end(), a0);
		conf_iterator_next(&i);
	}
	return true;
}

static bool config_load_iterator_category_section(adv_conf* config_context, const string& section, const string& tag, category_container& cat)
{
	adv_conf_iterator i;
	conf_iterator_section_begin(&i, config_context, section.c_str(), tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s, a0))
			return false;
		cat.insert(cat.end(), a0);
		conf_iterator_next(&i);
	}
	return true;
}

static bool config_load_iterator_emu_include(adv_conf* config_context, const string& tag, emulator_container& emu)
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string a0;
		string s = conf_iterator_string_get(&i);
		if (!config_split(s, a0))
			return false;
		emu.insert(emu.end(), a0);
		conf_iterator_next(&i);
	}
	return true;
}

bool config_state::load_iterator_game(adv_conf* config_context, const string& tag)
{
	unsigned ignored_count = 0;

	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string s = conf_iterator_string_get(&i);
		int j = 0;
		string game = arg_get(s, j);
		string group = arg_get(s, j);
		string type = arg_get(s, j);
		string time = arg_get(s, j);
		string session = arg_get(s, j);
		string desc = arg_get(s, j);

		if (j != s.length()) {
			config_error_a(s);
			return false;
		}

		if (game.length() == 0) {
			config_error_a(s);
			return false;
		}

		if (!load_game(game, group, type, time, session, desc)) {
			++ignored_count;
			if (ignored_count < 10)
				target_err("Ignoring info for game '%s'.\n", game.c_str());
			else if (ignored_count == 10)
				target_err("And also for other games.\n");
		}

		conf_iterator_next(&i);
	}

	if (ignored_count > 0)
		target_err("You may lose the game sessions and time information.\n");

	return true;
}

bool config_state::load_iterator_script(adv_conf* config_context, const string& tag)
{
	adv_conf_iterator i;
	conf_iterator_begin(&i, config_context, tag.c_str());
	while (!conf_iterator_is_end(&i)) {
		string s = conf_iterator_string_get(&i);

		int j = 0;
		string name = arg_get(s, j);
		if (name.length() == 0) {
			config_error_a(s);
			return false;
		}

		string text = string(s, j);

		script_bag.insert(script_bag.end(), script(name, text));

		conf_iterator_next(&i);
	}

	return true;
}

static bool config_load_orientation(adv_conf* config_context, unsigned& mask)
{
	string s;
	int i;

	s = conf_string_get_default(config_context, "display_orientation");
	i = 0;
	mask = 0;
	while (i < s.length()) {
		string arg = arg_get(s, i);
		if (arg == "flip_xy")
			mask ^= ADV_ORIENTATION_FLIP_XY;
		else if (arg == "mirror_x")
			mask ^= ADV_ORIENTATION_FLIP_X;
		else if (arg == "mirror_y")
			mask ^= ADV_ORIENTATION_FLIP_Y;
		else {
			config_error_la("display_orientation " + s, arg);
			return false;
		}
	}

	return true;
}

bool config_state::load(adv_conf* config_context, bool opt_verbose)
{
	string a0, a1;
	string::size_type p;

	preview_mask = 0;
	current_game = 0;
	current_clone = 0;
	fast = "";
	current_backdrop = resource();
	current_sound = resource();

	default_sort_orig = (listsort_t)conf_int_get_default(config_context, "sort");
	default_mode_orig = (listmode_t)conf_int_get_default(config_context, "mode");
	default_preview_orig = (listpreview_t)conf_int_get_default(config_context, "preview");

	double d = conf_float_get_default(config_context, "ui_translucency");
	ui_translucency = static_cast<int>(d * 255);
	if (ui_translucency > 255)
		ui_translucency = 255;
	if (!config_path(conf_string_get_default(config_context, "ui_background"), ui_back))
		return false;
	if (!config_path(conf_string_get_default(config_context, "ui_help"), ui_help))
		return false;
	if (!config_path(conf_string_get_default(config_context, "ui_exit"), ui_exit))
		return false;
	if (!config_path(conf_string_get_default(config_context, "ui_startup"), ui_startup))
		return false;
	a0 = conf_string_get_default(config_context, "ui_skiphorz");
	if (a0 == "auto") {
		ui_x = -1;
	} else {
		ui_x = atoi(a0.c_str());
		if (ui_x < 0)
			ui_x = 0;
		if (ui_x > 1000)
			ui_x = 1000;
	}
	a0 = conf_string_get_default(config_context, "ui_skipvert");
	if (a0 == "auto") {
		ui_y = -1;
	} else {
		ui_y = atoi(a0.c_str());
		if (ui_y < 0)
			ui_y = 0;
		if (ui_y > 1000)
			ui_y = 1000;
	}
	ui_left = conf_int_get_default(config_context, "ui_skipleft");
	ui_right = conf_int_get_default(config_context, "ui_skipright");
	ui_top = conf_int_get_default(config_context, "ui_skiptop");
	ui_bottom = conf_int_get_default(config_context, "ui_skipbottom");
	ui_autocalib = conf_bool_get_default(config_context, "ui_autocalib");
	ui_outline = conf_bool_get_default(config_context, "ui_outline");
	ui_top_name = conf_bool_get_default(config_context, "ui_topname");
	ui_top_bar = conf_bool_get_default(config_context, "ui_topbar");
	ui_bottom_bar = conf_bool_get_default(config_context, "ui_bottombar");
	ui_scroll_bar = conf_bool_get_default(config_context, "ui_scrollbar");
	ui_gamename = conf_bool_get_default(config_context, "ui_name");
	script_menu = conf_string_get_default(config_context, "ui_command_menu");
	script_error = conf_string_get_default(config_context, "ui_command_error");
	if (!load_iterator_script(config_context, "ui_command"))
		return false;

	lock_orig = (bool)conf_bool_get_default(config_context, "lock");
	restore = (restore_t)conf_int_get_default(config_context, "config");
	if (!config_load_skip(config_context, mode_skip_mask))
		return false;
	exit_mode = (exit_t)conf_int_get_default(config_context, "misc_exit");
	console_mode = conf_bool_get_default(config_context, "ui_console");
	menu_key = conf_bool_get_default(config_context, "ui_menukey");
	if (!config_load_iterator(config_context, "event_assign", event_in))
		return false;
	if (!config_load_iterator(config_context, "ui_color", color_in))
		return false;
	if (!config_split(conf_string_get_default(config_context, "idle_start"), a0, a1))
		return false;
	idle_start_first = atoi(a0.c_str());
	idle_start_rep = atoi(a1.c_str());
	menu_base_orig = conf_int_get_default(config_context, "menu_base");
	menu_rel_orig = conf_int_get_default(config_context, "menu_rel");
	if (!config_split(conf_string_get_default(config_context, "idle_screensaver"), a0, a1))
		return false;
	idle_saver_first = atoi(a0.c_str());
	idle_saver_rep = atoi(a1.c_str());
	if (!config_split(conf_string_get_default(config_context, "event_repeat"), a0, a1))
		return false;
	repeat = atoi(a0.c_str());
	repeat_rep = atoi(a1.c_str());
	disable_special = !conf_bool_get_default(config_context, "input_hotkey");
	a0 = conf_string_get_default(config_context, "display_size");
	if (a0 == "auto") {
		video_sizex = 0;
		video_sizey = 0;
	} else {
		p = a0.find('x');
		if (p == string::npos) {
			video_sizex = atoi(a0.c_str());
			video_sizey = video_sizex * 3 / 4;
		} else {
			a1 = a0.substr(p + 1);
			a0 = a0.substr(0, p);
			video_sizex = atoi(a0.c_str());
			video_sizey = atoi(a1.c_str());
		}
	}
	if (!config_path(conf_string_get_default(config_context, "ui_text_font"), video_font_text_path))
		return false;
	if (!config_split(conf_string_get_default(config_context, "ui_text_size"), a0, a1))
		return false;
	video_font_text_y = atoi(a0.c_str());
	video_font_text_x = atoi(a1.c_str());
	if (!config_path(conf_string_get_default(config_context, "ui_bar_font"), video_font_bar_path))
		return false;
	if (!config_split(conf_string_get_default(config_context, "ui_bar_size"), a0, a1))
		return false;
	video_font_bar_y = atoi(a0.c_str());
	video_font_bar_x = atoi(a1.c_str());
	if (!config_path(conf_string_get_default(config_context, "ui_menu_font"), video_font_menu_path))
		return false;
	if (!config_split(conf_string_get_default(config_context, "ui_menu_size"), a0, a1))
		return false;
	video_font_menu_y = atoi(a0.c_str());
	video_font_menu_x = atoi(a1.c_str());
	if (!config_load_orientation(config_context, video_orientation_orig))
		return false;
	video_gamma = conf_float_get_default(config_context, "display_gamma");
	video_brightness = conf_float_get_default(config_context, "display_brightness");
	resetexit = conf_bool_get_default(config_context, "display_restoreatexit");
	default_resetgame_unique = conf_bool_get_default(config_context, "display_restoreatgame");
	resizeeffect = conf_int_get_default(config_context, "display_resizeeffect");
	/* on Intel assume a fast machine */
#if defined(__i386__) || defined(__x86_64__)
	if (resizeeffect == COMBINE_AUTO)
		resizeeffect = COMBINE_XBR;
#else
	if (resizeeffect == COMBINE_AUTO)
		resizeeffect = COMBINE_SCALEK;
#endif

	quiet = conf_bool_get_default(config_context, "misc_quiet");
	if (!config_split(conf_string_get_default(config_context, "ui_gamemsg"), ui_gamemsg))
		return false;
	ui_gamesaver = (saver_t)conf_int_get_default(config_context, "ui_game");
	difficulty_orig = (difficulty_t)conf_int_get_default(config_context, "difficulty");
	idle_saver_type = (saver_t)conf_int_get_default(config_context, "idle_screensaver_preview");
	preview_expand = conf_float_get_default(config_context, "preview_expand");
	if (!config_path_import(conf_string_get_default(config_context, "preview_default"), preview_default))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "preview_default_snap"), preview_default_snap))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "preview_default_flyer"), preview_default_flyer))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "preview_default_cabinet"), preview_default_cabinet))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "preview_default_icon"), preview_default_icon))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "preview_default_marquee"), preview_default_marquee))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "preview_default_title"), preview_default_title))
		return false;

	preview_fast = (bool)conf_int_get_default(config_context, "event_mode");
	alpha_mode = (bool)conf_bool_get_default(config_context, "event_alpha");
	clip_mode = (clip_mode_t)conf_int_get_default(config_context, "ui_clip");
	merge = (merge_t)conf_int_get_default(config_context, "merge");
	icon_space = conf_int_get_default(config_context, "icon_space");

	if (!config_path_import(conf_string_get_default(config_context, "sound_foreground_begin"), sound_foreground_begin))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_foreground_end"), sound_foreground_end))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_foreground_key"), sound_foreground_key))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_foreground_start"), sound_foreground_start))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_foreground_stop"), sound_foreground_stop))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_background_begin"), sound_background_begin))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_background_end"), sound_background_end))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_background_start"), sound_background_start))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_background_stop"), sound_background_stop))
		return false;
	if (!config_path_import(conf_string_get_default(config_context, "sound_background_loop"), sound_background_loop))
		return false;
	if (!config_split(conf_string_get_default(config_context, "sound_background_loop_dir"), sound_background_loop_dir))
		return false;
	if (!config_load_iterator_emu(config_context, "emulator", emu))
		return false;
	if (!config_load_iterator_emu_attrib(config_context, "emulator_attrib", emu))
		return false;
	if (!config_load_iterator_emu_set(config_context, "emulator_roms", emu, &emulator::user_rom_path_set))
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

	for (pemulator_container::iterator i = emu.begin(); i != emu.end(); ++i) {
		if (!(*i)->config_get().load(config_context, config_normalize((*i)->user_name_get())))
			return false;
	}

	// print the copyright message before other messages
	if (!quiet) {
		target_nfo(ADV_COPY);
	}

	// select the active emulators
	for (pemulator_container::iterator i = emu.begin(); i != emu.end(); ++i) {
		if ((*i)->is_present()) {
			emu_active.insert(emu_active.end(), *i);
		} else {
			if (!quiet)
				target_err("Emulator '%s' not found, ignoring it.\n", (*i)->user_exe_path_get().c_str());
		}
	}

	if (emu_active.size() == 0) {
		target_err("No emulator found. Add an `emulator' option in your configuration file. These options are documented in the `advmenu.txt' file.\n");
		return false;
	}

	// load the game definitions
	for (pemulator_container::iterator i = emu_active.begin(); i != emu_active.end(); ) {
		if (opt_verbose)
			target_nfo("log: load game for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_game(gar, quiet)) {
			if (!quiet)
				target_err("Emulator '%s' without game information, ignoring it.\n", (*i)->user_exe_path_get().c_str());
			pemulator_container::iterator j = i;
			++i;
			emu_active.erase(j);
		} else {
			++i;
		}
	}

	// load the emulator configurations
	for (pemulator_container::iterator i = emu_active.begin(); i != emu_active.end(); ) {
		if (opt_verbose)
			target_nfo("log: load cfg for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_cfg(gar, quiet)) {
			if (!quiet)
				target_err("Emulator '%s' without configuration, assuming default one.\n", (*i)->user_exe_path_get().c_str());
			++i;
		} else {
			++i;
		}
	}

	// remove emulator without roms
	for (pemulator_container::iterator i = emu_active.begin(); i != emu_active.end(); ) {
		if ((*i)->is_empty()) {
			if (!quiet)
				target_err("Emulator '%s' without rom files, ignoring it.\n", (*i)->user_exe_path_get().c_str());
			pemulator_container::iterator j = i;
			++i;
			emu_active.erase(j);
		} else {
			++i;
		}
	}

	// load the software definitions
	for (pemulator_container::iterator i = emu_active.begin(); i != emu_active.end(); ) {
		if (opt_verbose)
			target_nfo("log: load software for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_software(gar, quiet)) {
			if (!quiet)
				target_err("Emulator '%s' without software information, ignoring it.\n", (*i)->user_exe_path_get().c_str());
			pemulator_container::iterator j = i;
			++i;
			emu_active.erase(j);
		} else {
			++i;
		}
	}

	if (emu_active.size() == 0) {
		target_err("No working emulator found. Adjust the `emulator' options in your configuration file. These options are documented in the `advmenu.txt' file.\n");
		return false;
	}

	// cache some values and relations
	if (opt_verbose)
		target_nfo("log: cache\n");
	gar.cache(merge);

	// set the previews
	for (pemulator_container::iterator i = emu_active.begin(); i != emu_active.end(); ++i) {
		if (opt_verbose)
			target_nfo("log: load preview for %s\n", (*i)->user_name_get().c_str());
		(*i)->preview_set(gar);
	}

	if (opt_verbose)
		target_nfo("log: load group and types\n");

	// load the group/type informations
	if (!config_load_iterator_pcategory(config_context, "group", group))
		return false;
	if (!config_load_iterator_pcategory(config_context, "type", type))
		return false;

	if (!config_load_iterator_category(config_context, "group_include", default_include_group_orig))
		return false;
	if (!config_load_iterator_category(config_context, "type_include", default_include_type_orig))
		return false;

	if (opt_verbose)
		target_nfo("log: load games info\n");

	if (!load_iterator_game(config_context, "game"))
		return false;

	// set the group and type of all the remainig games
	for (game_set::const_iterator i = gar.begin(); i != gar.end(); ++i) {
		i->auto_group_set(group.undefined_get());
		i->auto_type_set(type.undefined_get());
	}

	// load the emulator data
	for (pemulator_container::iterator i = emu_active.begin(); i != emu_active.end(); ++i) {
		if (opt_verbose)
			target_nfo("log: load data for %s\n", (*i)->user_name_get().c_str());
		if (!(*i)->load_data(gar)) {
			return false;
		}
	}

	// load the emulator active
	if (!config_load_iterator_emu_include(config_context, "emulator_include", include_emu_orig))
		return false;

	// if the set is empty add all the emulator
	if (include_emu_orig.size() == 0) {
		for (pemulator_container::iterator i = emu_active.begin(); i != emu_active.end(); ++i) {
			include_emu_orig.insert(include_emu_orig.end(), (*i)->user_name_get());
		}
	}

	if (!load_iterator_import(config_context, "desc_import", &config_state::import_desc, opt_verbose))
		return false;
	if (!load_iterator_import(config_context, "info_import", &config_state::import_info, opt_verbose))
		return false;
	if (!load_iterator_import(config_context, "type_import", &config_state::import_type, opt_verbose))
		return false;
	if (!load_iterator_import(config_context, "group_import", &config_state::import_group, opt_verbose))
		return false;

	/* if the default inclusion are empty add everything */
	/* note that <undefined> is always present */
	if (default_include_group_orig.size() == 0 && group.size() > 1) {
		for (pcategory_container::iterator i = group.begin(); i != group.end(); ++i)
			default_include_group_orig.insert((*i)->name_get());
	}
	if (default_include_type_orig.size() == 0 && type.size() > 1) {
		for (pcategory_container::iterator i = type.begin(); i != type.end(); ++i)
			default_include_type_orig.insert((*i)->name_get());
	}


	if (opt_verbose)
		target_nfo("log: load background music list\n");

	config_load_background_list(sound_background_loop_dir, sound_background);

	if (opt_verbose)
		target_nfo("log: start\n");

	return true;
}

void config_state::import_desc(const game& g, const string& text)
{
	g.auto_description_set(text);
}

void config_state::import_info(const game& g, const string& text)
{
	g.auto_info_set(text);
}

void config_state::import_type(const game& g, const string& text)
{
	g.auto_type_set(type.insert(text));
}

void config_state::import_group(const game& g, const string& text)
{
	g.auto_group_set(group.insert(text));
}

void config_state::conf_default(adv_conf* config_context)
{
	adv_conf_iterator i;

	conf_iterator_begin(&i, config_context, "emulator");
	if (conf_iterator_is_end(&i)) {
		char path[FILE_MAXPATH];
#if !USE_NIX
		if (target_search(path, FILE_MAXPATH, "advmame.exe") == 0) {
			target_out("Adding emulator `advmame'...\n");
			conf_set(config_context, "", "emulator", "\"advmame\" advmame \"advmame.exe\" \"-quiet\"");
			string path;
			path = "category.ini";
			if (access(cpath_export(path), F_OK) == 0) {
				string opt = "catver \"advmame\" \"" + path_export(path) + "\" \"Category\"";
				conf_set(config_context, "", "type_import", opt.c_str());
			}
		}
		if (target_search(path, FILE_MAXPATH, "advmess.exe") == 0) {
			target_out("Adding emulator `advmess'...\n");
			conf_set(config_context, "", "emulator", "\"advmess\" advmess \"advmess.exe\" \"-quiet\"");
		}
		if (target_search(path, FILE_MAXPATH, "dmame.exe") == 0) {
			target_out("Adding emulator `dmame'...\n");
			conf_set(config_context, "", "emulator", "\"dmame\" dmame \"dmame.exe\" \"\"");
		}
		if (target_search(path, FILE_MAXPATH, "mame64.exe") == 0) {
			target_out("Adding emulator `mame'...\n");
			conf_set(config_context, "", "emulator", "\"mame\" mame \"mame64.exe\" \"\"");
		}
		if (target_search(path, FILE_MAXPATH, "mame.exe") == 0) {
			target_out("Adding emulator `mame'...\n");
			conf_set(config_context, "", "emulator", "\"mame\" mame \"mame.exe\" \"\"");
		}
		if (target_search(path, FILE_MAXPATH, "raine.exe") == 0) {
			target_out("Adding emulator `draine'...\n");
			conf_set(config_context, "", "emulator", "\"draine\" draine \"raine.exe\" \"\"");
		}
		if (target_search(path, FILE_MAXPATH, "snes9x.exe") == 0) {
			target_out("Adding emulator `snes9x'...\n");
			conf_set(config_context, "", "emulator", "\"snes9x\" generic \"snes9x.exe\" \"%f\"");
			conf_set(config_context, "", "emulator_roms", "\"snes9x\" \"roms\"");
		}
		if (target_search(path, FILE_MAXPATH, "zsnes.exe") == 0) {
			target_out("Adding emulator `zsnes'...\n");
			conf_set(config_context, "", "emulator", "\"zsnes\" generic \"zsnes.exe\" \"-e -m roms\\%f\"");
			conf_set(config_context, "", "emulator_roms", "\"zsnes\" \"roms\"");
		}
#else
		if (target_search(path, FILE_MAXPATH, "advmame") == 0) {
			target_out("Adding emulator `advmame'...\n");
			conf_set(config_context, "", "emulator", "\"advmame\" advmame \"advmame\" \"-quiet\"");
#ifdef ADV_DATADIR
			string path;
			path = string(ADV_DATADIR) + "/snap";
			if (access(cpath_export(path), F_OK) == 0) {
				string opt = "\"advmame\" \"" + path_export(path) + "\"";
				conf_set(config_context, "", "emulator_altss", opt.c_str());
			}
			path = string(ADV_DATADIR) + "/rom";
			if (access(cpath_export(path), F_OK) == 0) {
				string opt = "\"advmame\" \"" + path_export(path) + "\"";
				conf_set(config_context, "", "emulator_roms", opt.c_str());
			}
			path = string(ADV_DATADIR) + "/category.ini";
			if (access(cpath_export(path), F_OK) == 0) {
				string opt = "catver \"advmame\" \"" + path_export(path) + "\" \"Category\"";
				conf_set(config_context, "", "type_import", opt.c_str());
			}
#endif
		}
		if (target_search(path, FILE_MAXPATH, "advmess") == 0) {
			target_out("Adding emulator `advmess'...\n");
			conf_set(config_context, "", "emulator", "\"advmess\" advmess \"advmess\" \"-quiet\"");
#ifdef ADV_DATADIR
			string path;
			path = string(ADV_DATADIR) + "/snap";
			if (access(cpath_export(path), F_OK) == 0) {
				string opt = "\"advmess\" \"" + path_export(path) + "\"";
				conf_set(config_context, "", "emulator_altss", opt.c_str());
			}
			path = string(ADV_DATADIR) + "/rom";
			if (access(cpath_export(path), F_OK) == 0) {
				string opt = "\"advmess\" \"" + path_export(path) + "\"";
				conf_set(config_context, "", "emulator_roms", opt.c_str());
			}
			path = string(ADV_DATADIR) + "/image";
			if (access(cpath_export(path), F_OK) == 0) {
				string opt = "\"advmess\" \"" + path_export(path) + "\"";
				conf_set(config_context, "", "emulator_images", opt.c_str());
			}
#endif
		}
		if (target_search(path, FILE_MAXPATH, "mame64") == 0) {
			target_out("Adding emulator `mame'...\n");
			conf_set(config_context, "", "emulator", "\"mame\" mame \"mame64\" \"\"");
		}
		if (target_search(path, FILE_MAXPATH, "mame") == 0) {
			target_out("Adding emulator `mame'...\n");
			conf_set(config_context, "", "emulator", "\"mame\" mame \"mame\" \"\"");
		}
#endif
	}

	conf_iterator_begin(&i, config_context, "ui_color");
	if (conf_iterator_is_end(&i)) {
		color_out(config_context, "ui_color");
	}

	conf_iterator_begin(&i, config_context, "event_assign");
	if (conf_iterator_is_end(&i)) {
		event_out(config_context, "event_assign");
	}
}

// -------------------------------------------------------------------------
// Configuration save

bool config_state::save(adv_conf* config_context) const
{
	conf_int_set(config_context, "", "mode", default_mode_orig);
	conf_int_set(config_context, "", "sort", default_sort_orig);
	conf_int_set(config_context, "", "preview", default_preview_orig);

	conf_remove(config_context, "", "group_include");
	for (category_container::const_iterator i = default_include_group_orig.begin(); i != default_include_group_orig.end(); ++i) {
		conf_string_set(config_context, "", "group_include", config_out(*i).c_str());
	}

	conf_remove(config_context, "", "type_include");
	for (category_container::const_iterator i = default_include_type_orig.begin(); i != default_include_type_orig.end(); ++i) {
		conf_string_set(config_context, "", "type_include", config_out(*i).c_str());
	}

	conf_int_set(config_context, "", "menu_base", menu_base_orig);
	conf_int_set(config_context, "", "menu_rel", menu_rel_orig);
	conf_int_set(config_context, "", "difficulty", difficulty_orig);

	for (pemulator_container::const_iterator i = emu.begin(); i != emu.end(); ++i) {
		(*i)->config_get().save(config_context, config_normalize((*i)->user_name_get()));
	}

	conf_remove(config_context, "", "emulator_include");
	for (emulator_container::const_iterator i = include_emu_orig.begin(); i != include_emu_orig.end(); ++i) {
		conf_string_set(config_context, "", "emulator_include", config_out(*i).c_str());
	}

	conf_remove(config_context, "", "group");
	for (pcategory_container::const_iterator i = group.begin(); i != group.end(); ++i) {
		conf_string_set(config_context, "", "group", config_out((*i)->name_get()).c_str());
	}

	conf_remove(config_context, "", "type");
	for (pcategory_container::const_iterator i = type.begin(); i != type.end(); ++i) {
		conf_string_set(config_context, "", "type", config_out((*i)->name_get()).c_str());
	}

	conf_remove(config_context, "", "emulator_attrib");
	for (pemulator_container::const_iterator i = emu.begin(); i != emu.end(); ++i) {
		(*i)->attrib_get(config_context, "", "emulator_attrib");
	}

	string s;
	if ((video_orientation_orig & ADV_ORIENTATION_FLIP_XY) != 0) {
		if (s.length()) s += " ";
		s += "flip_xy";
	}
	if ((video_orientation_orig & ADV_ORIENTATION_FLIP_X) != 0) {
		if (s.length()) s += " ";
		s += "mirror_x";
	}
	if ((video_orientation_orig & ADV_ORIENTATION_FLIP_Y) != 0) {
		if (s.length()) s += " ";
		s += "mirror_y";
	}

	conf_string_set(config_context, "", "display_orientation", s.c_str());

	conf_remove(config_context, "", "game");
	for (game_set::const_iterator i = gar.begin(); i != gar.end(); ++i) {
		if (0
			|| i->is_user_group_set()
			|| i->is_user_type_set()
			|| i->is_time_set()
			|| i->is_session_set()
			|| (i->is_user_description_set() && i->description_get().length() != 0)
		) {
			ostringstream f;
			f << "\"" << i->name_get() << "\"";

			f << " \"";
			if (i->is_user_group_set())
				f << i->group_get()->name_get();
			f << "\"";

			f << " \"";
			if (i->is_user_type_set())
				f << i->type_get()->name_get();
			f << "\"";

			f << " " << i->time_get();

			f << " " << i->session_get();

			f << " \"";
			if (i->is_user_description_set()) {
				f << i->description_get();
			}
			f << "\"";

			conf_string_set(config_context, "", "game", f.str().c_str());
		}
	}

	// don't print the error message (error_callback==0)
	if (conf_save(config_context, 1, 0, 0, 0) != 0)
		return false;

	// prevent data lost if crashing
	target_sync();

	return true;
}

// ------------------------------------------------------------------------
// Configuration restore

void config_state::restore_load()
{
	default_sort_effective = default_sort_orig;
	default_mode_effective = default_mode_orig;
	default_preview_effective = default_preview_orig;
	default_include_group_effective = default_include_group_orig;
	default_include_type_effective = default_include_type_orig;

	difficulty_effective = difficulty_orig;
	include_emu_set(include_emu_orig);
	menu_base_effective = menu_base_orig;
	menu_rel_effective = menu_rel_orig;
	lock_effective = lock_orig;
	video_orientation_effective = video_orientation_orig;
	for (pemulator_container::const_iterator i = emu.begin(); i != emu.end(); ++i) {
		(*i)->config_get().restore_load();
		(*i)->attrib_load();
	}
}

void config_state::restore_save_default()
{
	default_sort_orig = default_sort_effective;
	default_mode_orig = default_mode_effective;
	default_preview_orig = default_preview_effective;
	default_include_group_orig = default_include_group_effective;
	default_include_type_orig = default_include_type_effective;

	difficulty_orig = difficulty_effective;
	include_emu_orig = include_emu_effective;
	menu_base_orig = menu_base_effective;
	menu_rel_orig = menu_rel_effective;
	lock_orig = lock_effective;
	video_orientation_orig = video_orientation_effective;
	for (pemulator_container::const_iterator i = emu.begin(); i != emu.end(); ++i) {
		(*i)->attrib_save();
	}
}

void config_state::restore_save()
{
	restore_save_default();

	for (pemulator_container::const_iterator i = emu.begin(); i != emu.end(); ++i) {
		(*i)->config_get().restore_save();
	}
}

// ------------------------------------------------------------------------
// Configuration state

config_state::config_state()
	: sub_emu(0)
{
}

config_state::~config_state()
{
	// delete the emulators
	for (pemulator_container::iterator i = emu.begin(); i != emu.end(); ++i) {
		delete *i;
	}
	// delete the groups
	for (pcategory_container::iterator i = group.begin(); i != group.end(); ++i) {
		delete *i;
	}
	// delete the types
	for (pcategory_container::iterator i = type.begin(); i != type.end(); ++i) {
		delete *i;
	}
}

void config_state::sub_disable()
{
	sub_emu = 0;
}

void config_state::sub_enable()
{
	if (include_emu_effective.size() == 1) {
		sub_emu = 0;
		for (pemulator_container::iterator i = emu.begin(); i != emu.end(); ++i) {
			if ((*i)->user_name_get() == *include_emu_effective.begin()) {
				sub_emu = *i;
				break;
			}
		}
	} else if (include_emu_effective.size() == 0 && emu_active.size() == 1) {
		sub_emu = *emu_active.begin();
	} else {
		sub_emu = 0;
	}
}

void config_state::include_emu_set(const emulator_container& A)
{
	include_emu_effective = A;

	sub_enable();
}

const emulator_container& config_state::include_emu_get()
{
	return include_emu_effective;
}

listsort_t config_state::sort_get()
{
	if (sub_has() && sub_get().sort_has())
		return sub_get().sort_get();
	else
		return default_sort_effective;
}

listmode_t config_state::mode_get()
{
	if (sub_has() && sub_get().mode_has())
		return sub_get().mode_get();
	else
		return default_mode_effective;
}

listpreview_t config_state::preview_get()
{
	if (sub_has() && sub_get().preview_has())
		return sub_get().preview_get();
	else
		return default_preview_effective;
}

const category_container& config_state::include_group_get()
{
	if (sub_has() && sub_get().include_group_has())
		return sub_get().include_group_get();
	else
		return default_include_group_effective;
}

const category_container& config_state::include_type_get()
{
	if (sub_has() && sub_get().include_type_has())
		return sub_get().include_type_get();
	else
		return default_include_type_effective;
}

bool config_state::resetgame_get(const game* g)
{
	// In multiple emulator listing, returns always the specific
	// emulator value depending on the game selected.
	if (!g)
		return default_resetgame_unique;

	if (!g->emulator_get()->config_get().resetgame_has())
		return default_resetgame_unique;

	return g->emulator_get()->config_get().resetgame_get();
}

bool config_state::resetexit_get()
{
	return resetexit;
}

void config_state::sort_set(listsort_t A)
{
	if (sub_has())
		sub_get().sort_set(A);
	else
		default_sort_effective = A;
}

void config_state::mode_set(listmode_t A)
{
	if (sub_has())
		sub_get().mode_set(A);
	else
		default_mode_effective = A;
}

void config_state::preview_set(listpreview_t A)
{
	if (sub_has())
		sub_get().preview_set(A);
	else
		default_preview_effective = A;
}

void config_state::include_group_set(const category_container& A)
{
	if (sub_has())
		sub_get().include_group_set(A);
	else
		default_include_group_effective = A;
}

void config_state::include_type_set(const category_container& A)
{
	if (sub_has())
		sub_get().include_type_set(A);
	else
		default_include_type_effective = A;
}

// ------------------------------------------------------------------------
// config_import

config_import::config_import(const std::string Atype, const std::string Aemulator, const std::string Afile, const std::string Asection)
	: type(Atype), emulator(Aemulator), file(Afile), section(Asection)
{
}

void config_import::import_catver(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string&))
{
	int j = 0;

	string ss = file_read(file);

	bool in = false;
	while (j < ss.length()) {
		string s = token_get(ss, j, "\r\n");
		token_skip(ss, j, "\r\n");

		int i = 0;
		token_skip(s, i, " \t");

		if (i < s.length() && s[i] == '[') {
			token_skip(s, i, "[");
			string cmd = token_get(s, i, "]");
			in = cmd == section;
		} else if (in && i < s.length() && isalnum(s[i])) {
			string tag = token_get(s, i, " =");
			token_skip(s, i, " =");
			string text = token_get(s, i, "");
			if (text.length()) {
				string name = emulator + "/" + tag;
				game_set::const_iterator k = gar.find(game(name));
				if (k != gar.end()) {
					(config.*set)(*k, text);
				}
			}
		}
	}
}

void config_import::import_catlist(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string&))
{
	int j = 0;

	string ss = file_read(file);
	string cat;

	while (j < ss.length()) {
		string s = token_get(ss, j, "\r\n");
		token_skip(ss, j, "\r\n");

		int i = 0;
		token_skip(s, i, " \t");

		if (i < s.length() && s[i] == '[') {
			token_skip(s, i, "[");
			cat = token_get(s, i, "]");
		} else if (cat.length() && i < s.length() && isalnum(s[i])) {
			string tag = token_get(s, i, " \t");
			token_skip(s, i, " \t");
			if (i == s.length() && tag.length() != 0) {
				string name = emulator + "/" + tag;
				game_set::const_iterator k = gar.find(game(name));
				if (k != gar.end()) {
					(config.*set)(*k, cat);
				}
			}
		}
	}
}

void config_import::import_mac(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string&))
{
	int j = 0;

	string ss = file_read(file);

	string main_text;
	while (j < ss.length()) {

		string s = token_get(ss, j, "\r\n");
		token_skip(ss, j, "\r\n");

		int i = 0;
		token_skip(s, i, " \t");

		if (i < s.length() && isalnum(s[i])) {
			string tag = token_get(s, i, " \t");
			token_skip(s, i, " \t");
			string text = token_get(s, i, "");
			if (text.length()) {
				main_text = text;
				// remove special chars
				if (main_text.length() && main_text[main_text.length() - 1] == '-')
					main_text.erase(main_text.length() - 1, 1);
				if (main_text.length() && main_text[0] == '¥')
					main_text.erase(0, 1);
			}
			if (main_text.length()) {
				string name = emulator + "/" + tag;
				game_set::const_iterator k = gar.find(game(name));
				if (k != gar.end()) {
					(config.*set)(*k, main_text);
				}
			}
		}
	}
}

void config_import::import_nms(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string&))
{
	int j = 0;

	string ss = file_read(file);

	while (j < ss.length()) {
		string s = token_get(ss, j, "\r\n");
		token_skip(ss, j, "\r\n");

		int i = 0;
		token_skip(s, i, " \t");

		string text = token_get(s, i, '|');
		token_skip(s, i, "|");
		string tag = token_get(s, i, "");

		if (text.length() && tag.length()) {
			string name = emulator + "/" + tag;
			game_set::iterator k = gar.find(game(name));
			if (k != gar.end()) {
				(config.*set)(*k, text);
			}
		}
	}
}

void config_import::import(game_set& gar, config_state& config, void (config_state::*set)(const game&, const std::string&))
{
	if (type == "ini" || type == "catver")
		import_catver(gar, config, set);
	else if (type == "catlist")
		import_catlist(gar, config, set);
	else if (type == "mac")
		import_mac(gar, config, set);
	else if (type == "nms")
		import_nms(gar, config, set);
}

bool config_emulator_state::load(adv_conf* config_context, const string& section)
{
	int i;
	adv_bool b;

	if (conf_int_section_get(config_context, section.c_str(), "sort", &i) == 0) {
		sort_set_orig = true;
		sort_orig = (listsort_t)i;
	} else {
		sort_set_orig = false;
	}

	if (conf_int_section_get(config_context, section.c_str(), "mode", &i) == 0) {
		mode_set_orig = true;
		mode_orig = (listmode_t)i;
	} else {
		mode_set_orig = false;
	}

	if (conf_int_section_get(config_context, section.c_str(), "preview", &i) == 0) {
		preview_set_orig = true;
		preview_orig = (listpreview_t)i;
	} else {
		preview_set_orig = false;
	}

	if (!config_load_iterator_category_section(config_context, section, "group_include", include_group_orig))
		return false;
	if (include_group_orig.size() != 0) {
		include_group_set_orig = true;
	} else {
		include_group_set_orig = false;
	}

	if (!config_load_iterator_category_section(config_context, section, "type_include", include_type_orig))
		return false;
	if (include_type_orig.size() != 0) {
		include_type_set_orig = true;
	} else {
		include_type_set_orig = false;
	}

	if (conf_bool_section_get(config_context, section.c_str(), "display_restoreatgame", &b) == 0) {
		resetgame_set_unique = true;
		resetgame_unique = b;
	} else {
		resetgame_set_unique = false;
	}

	return true;
}

void config_emulator_state::save(adv_conf* config_context, const string& section)
{
	if (mode_set_orig) {
		conf_int_set(config_context, section.c_str(), "mode", mode_orig);
	} else {
		conf_remove(config_context, section.c_str(), "mode");
	}

	if (sort_set_orig) {
		conf_int_set(config_context, section.c_str(), "sort", sort_orig);
	} else {
		conf_remove(config_context, section.c_str(), "sort");
	}

	if (preview_set_orig) {
		conf_int_set(config_context, section.c_str(), "preview", preview_orig);
	} else {
		conf_remove(config_context, section.c_str(), "preview");
	}

	conf_remove(config_context, section.c_str(), "group_include");
	if (include_group_set_orig) {
		for (category_container::const_iterator i = include_group_orig.begin(); i != include_group_orig.end(); ++i) {
			conf_string_set(config_context, section.c_str(), "group_include", config_out(*i).c_str());
		}
	}

	conf_remove(config_context, section.c_str(), "type_include");
	if (include_type_set_orig) {
		for (category_container::const_iterator i = include_type_orig.begin(); i != include_type_orig.end(); ++i) {
			conf_string_set(config_context, section.c_str(), "type_include", config_out(*i).c_str());
		}
	}
}

void config_emulator_state::restore_load()
{
	sort_effective = sort_orig;
	mode_effective = mode_orig;
	preview_effective = preview_orig;
	include_group_effective = include_group_orig;
	include_type_effective = include_type_orig;
	sort_set_effective = sort_set_orig;
	mode_set_effective = mode_set_orig;
	preview_set_effective = preview_set_orig;
	include_group_set_effective = include_group_set_orig;
	include_type_set_effective = include_type_set_orig;
}

void config_emulator_state::restore_save()
{
	sort_orig = sort_effective;
	mode_orig = mode_effective;
	preview_orig = preview_effective;
	include_group_orig = include_group_effective;
	include_type_orig = include_type_effective;
	sort_set_orig = sort_set_effective;
	mode_set_orig = mode_set_effective;
	preview_set_orig = preview_set_effective;
	include_group_set_orig = include_group_set_effective;
	include_type_set_orig = include_type_set_effective;
}

bool config_emulator_state::sort_has()
{
	return sort_set_effective;
}

bool config_emulator_state::mode_has()
{
	return mode_set_effective;
}

bool config_emulator_state::preview_has()
{
	return preview_set_effective;
}

bool config_emulator_state::include_group_has()
{
	return include_group_set_effective;
}

bool config_emulator_state::include_type_has()
{
	return include_type_set_effective;
}

bool config_emulator_state::resetgame_has()
{
	return resetgame_set_unique;
}

listsort_t config_emulator_state::sort_get()
{
	return sort_effective;
}

listmode_t config_emulator_state::mode_get()
{
	return mode_effective;
}

listpreview_t config_emulator_state::preview_get()
{
	return preview_effective;
}

const category_container& config_emulator_state::include_group_get()
{
	return include_group_effective;
}

const category_container& config_emulator_state::include_type_get()
{
	return include_type_effective;
}

bool config_emulator_state::resetgame_get()
{
	return resetgame_unique;
}

void config_emulator_state::sort_set(listsort_t A)
{
	sort_set_effective = true;
	sort_effective = A;
}

void config_emulator_state::mode_set(listmode_t A)
{
	mode_set_effective = true;
	mode_effective = A;
}

void config_emulator_state::preview_set(listpreview_t A)
{
	preview_set_effective = true;
	preview_effective = A;
}

void config_emulator_state::include_group_set(const category_container& A)
{
	include_group_set_effective = true;
	include_group_effective = A;
}

void config_emulator_state::include_type_set(const category_container& A)
{
	include_type_set_effective = true;
	include_type_effective = A;
}

void config_emulator_state::sort_unset()
{
	sort_set_effective = false;
}

void config_emulator_state::mode_unset()
{
	mode_set_effective = false;
}

void config_emulator_state::preview_unset()
{
	preview_set_effective = false;
}

void config_emulator_state::include_group_unset()
{
	include_group_set_effective = false;
}

void config_emulator_state::include_type_unset()
{
	include_type_set_effective = false;
}


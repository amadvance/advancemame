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

#include "portable.h"

#include "submenu.h"
#include "text.h"
#include "play.h"

#include "advance.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

#define MSG_CHOICE_DX 30*int_font_dx_get()
#define MSG_CHOICE_X (int_dx_get()-MSG_CHOICE_DX)/2
#define MSG_CHOICE_Y int_dy_get()/2

// ------------------------------------------------------------------------
// Sort menu

#define SORT_CHOICE_DX 15*int_font_dx_get()

void run_sort(config_state& rs)
{
	choice_bag ch;

	ch.insert( ch.end(), choice("Parent name", sort_by_root_name) );
	ch.insert( ch.end(), choice("Name", sort_by_name) );
	ch.insert( ch.end(), choice("Time played", sort_by_time) );
	ch.insert( ch.end(), choice("Play", sort_by_session) );
	ch.insert( ch.end(), choice("Time per play", sort_by_timepersession) );
	ch.insert( ch.end(), choice("Group", sort_by_group) );
	ch.insert( ch.end(), choice("Type", sort_by_type) );
	ch.insert( ch.end(), choice("Manufacturer", sort_by_manufacturer) );
	ch.insert( ch.end(), choice("Year", sort_by_year) );
	ch.insert( ch.end(), choice("Size", sort_by_size) );
	ch.insert( ch.end(), choice("Resolution", sort_by_res) );
	ch.insert( ch.end(), choice("Info", sort_by_info) );

	choice_bag::iterator i = ch.find_by_value(rs.sort_effective);
	if (i == ch.end())
		i = ch.begin();

	int key = ch.run(" Sort mode", SECOND_CHOICE_X, SECOND_CHOICE_Y, SORT_CHOICE_DX, i);
	if (key == INT_KEY_ENTER) {
		rs.sort_effective = (game_sort_t)i->value_get();
	}
}

// ------------------------------------------------------------------------
// Command menu

#define COMMAND_CHOICE_DX 33*int_font_dx_get()

void run_command(config_state& rs)
{
	choice_bag ch;

	if (!rs.console_mode && rs.current_game) {
		bool used_backdrop = false;
		bool used_sound = false;

		if (rs.current_game->preview_snap_get().is_deletable()) {
			string s = "Delete game snapshot";
			if (rs.current_game->preview_snap_get() == rs.current_backdrop) {
				s += " (shown)";
				used_backdrop = true;
			}
			ch.insert( ch.end(), choice(s, 0) );
		}
		if (rs.current_game->preview_clip_get().is_deletable()) {
			string s = "Delete game clip";
			if (rs.current_game->preview_clip_get() == rs.current_backdrop) {
				s += " (shown)";
				used_backdrop = true;
			}
			ch.insert( ch.end(), choice(s, 1) );
		}
		if (rs.current_game->preview_flyer_get().is_deletable()) {
			string s = "Delete game flyer";
			if (rs.current_game->preview_flyer_get() == rs.current_backdrop) {
				s += " (shown)";
				used_backdrop = true;
			}
			ch.insert( ch.end(), choice(s, 2) );
		}
		if (rs.current_game->preview_cabinet_get().is_deletable()) {
			string s = "Delete game cabinet";
			if (rs.current_game->preview_cabinet_get() == rs.current_backdrop) {
				s += " (shown)";
				used_backdrop = true;
			}
			ch.insert( ch.end(), choice(s, 3) );
		}
		if (rs.current_game->preview_icon_get().is_deletable()) {
			string s = "Delete game icon";
			if (rs.current_game->preview_icon_get() == rs.current_backdrop) {
				s += " (shown)";
				used_backdrop = true;
			}
			ch.insert( ch.end(), choice(s, 4) );
		}
		if (!used_backdrop && rs.current_backdrop.is_deletable()) {
			string s = "Delete shown image (from parent)";
			ch.insert( ch.end(), choice(s, 5) );
		}
		if (rs.current_game->preview_sound_get().is_deletable()) {
			string s = "Delete game sound";
			if (rs.current_game->preview_sound_get() == rs.current_sound) {
				s += " (played)";
				used_sound = true;
			}
			ch.insert( ch.end(), choice(s, 6) );
		}
		if (!used_sound && rs.current_sound.is_deletable()) {
			ch.insert( ch.end(), choice("Delete played sound (from parent)", 7) );
		}
	}

	for(script_container::iterator i=rs.script_bag.begin();i!=rs.script_bag.end();++i) {
		if (i->text.find("%s") != string::npos) {
			if (!rs.current_game)
				continue;
		}
		if (i->text.find("%p") != string::npos || i->text.find("%f") != string::npos) {
			if (!rs.current_game)
				continue;
			if (rs.current_game->rom_zip_set_get().size() == 0)
				continue;
		}
		ch.insert( ch.end(), choice(i->name, &*i) );
	}

	if (ch.begin() == ch.end())
		ch.insert( ch.end(), choice("No commands available", -1) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(string(" ") + rs.script_menu, SECOND_CHOICE_X, SECOND_CHOICE_Y, COMMAND_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		int r;
		if (i->value_get()>=-1 && i->value_get()<=256) {
			switch (i->value_get()) {
			case 0 :
				r = remove(cpath_export(rs.current_game->preview_snap_get().archive_get()));
				break;
			case 1 :
				r = remove(cpath_export(rs.current_game->preview_clip_get().archive_get()));
				break;
			case 2 :
				r = remove(cpath_export(rs.current_game->preview_flyer_get().archive_get()));
				break;
			case 3 :
				r = remove(cpath_export(rs.current_game->preview_cabinet_get().archive_get()));
				break;
			case 4 :
				r = remove(cpath_export(rs.current_game->preview_icon_get().archive_get()));
				break;
			case 5 :
				r = remove(cpath_export(rs.current_backdrop.archive_get()));
				break;
			case 6 :
				r = remove(cpath_export(rs.current_game->preview_sound_get().archive_get()));
				break;
			case 7 :
				r = remove(cpath_export(rs.current_sound.archive_get()));
				break;
			case -1 :
				r = 0;
				break;
			default:
				r = -1;
				break;
			}
		} else {
			script* s = static_cast<script*>(i->ptr_get());

			string text = s->text;

			if (rs.current_game) {
				text = subs(text, "%s", rs.current_game->name_without_emulator_get());
				if (rs.current_game->rom_zip_set_get().size()) {
					string path = *rs.current_game->rom_zip_set_get().begin();
					text = subs(text, "%p", path_export(path));
					text = subs(text, "%f", path_export(file_file(path)));
				}
			}

			int_unplug();

			r = target_script(text.c_str());

			int_plug();
		}

		if (r != 0) {
			choice_bag ch;
			ch.insert( ch.end(), choice(rs.script_error, 0) );
			choice_bag::iterator i = ch.begin();
			ch.run(" Error", MSG_CHOICE_X, MSG_CHOICE_Y, MSG_CHOICE_DX, i);
		}
	}
}

// ------------------------------------------------------------------------
// Mode menu

#define MODE_CHOICE_DX 15*int_font_dx_get()

void run_mode(config_state& rs)
{
	choice_bag ch;

	ch.insert( ch.end(), choice("Full", mode_full) );
	ch.insert( ch.end(), choice("Full Mixed", mode_full_mixed) );
	ch.insert( ch.end(), choice("Text", mode_text) );
	ch.insert( ch.end(), choice("List", mode_list) );
	ch.insert( ch.end(), choice("List Mixed", mode_list_mixed) );
	ch.insert( ch.end(), choice("Tile Small", mode_tile_small) );
	ch.insert( ch.end(), choice("Tile Normal", mode_tile_normal) );
	ch.insert( ch.end(), choice("Tile Big", mode_tile_big) );
	ch.insert( ch.end(), choice("Tile Enormous", mode_tile_enormous) );
	ch.insert( ch.end(), choice("Tile Giant", mode_tile_giant) );
	ch.insert( ch.end(), choice("Tile Icon", mode_tile_icon) );
	ch.insert( ch.end(), choice("Tile Marquee", mode_tile_marquee) );

	choice_bag::iterator i = ch.find_by_value(rs.mode_effective);
	if (i == ch.end())
		i = ch.begin();

	int key = ch.run(" Mode", SECOND_CHOICE_X, SECOND_CHOICE_Y, MODE_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.mode_effective = (show_t)i->value_get();
	}
}

// ------------------------------------------------------------------------
// Preview menu

#define PREVIEW_CHOICE_DX 15*int_font_dx_get()

void run_preview(config_state& rs)
{
	choice_bag ch;

	ch.insert( ch.end(), choice("Snap", preview_snap) );
	ch.insert( ch.end(), choice("Title", preview_title) );
	ch.insert( ch.end(), choice("Flyer", preview_flyer) );
	ch.insert( ch.end(), choice("Cabinet", preview_cabinet) );
	ch.insert( ch.end(), choice("Icon", preview_icon) );
	ch.insert( ch.end(), choice("Marquee", preview_marquee) );

	choice_bag::iterator i = ch.find_by_value(rs.preview_effective);
	if (i == ch.end())
		i = ch.begin();

	int key = ch.run(" Preview", SECOND_CHOICE_X, SECOND_CHOICE_Y, PREVIEW_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.preview_effective = (preview_t)i->value_get();
	}
}

// ------------------------------------------------------------------------
// Group menu

#define GROUP_CHOICE_DX 20*int_font_dx_get()

void run_group(config_state& rs)
{
	choice_bag ch;

	for(pcategory_container::const_iterator j = rs.group.begin();j!=rs.group.end();++j) {
		bool tag = rs.include_group_effective.find((*j)->name_get()) != rs.include_group_effective.end();
		ch.insert( ch.end(), choice((*j)->name_get(), tag, 0) );
	}

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Show groups", SECOND_CHOICE_X, SECOND_CHOICE_Y, GROUP_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.include_group_effective.erase(rs.include_group_effective.begin(), rs.include_group_effective.end());
		for(choice_bag::const_iterator j=ch.begin();j!=ch.end();++j) {
			if (j->bistate_get())
				rs.include_group_effective.insert( j->desc_get() );
		}
	}
}

void run_group_next(config_state& rs)
{
	category* next_select = 0;
	bool all_select = true;

	bool pred_in = false;
	for(pcategory_container::const_iterator j=rs.group.begin();j!=rs.group.end();++j) {
		if (pred_in)
			next_select = *j;
		pred_in = false;
		for(category_container::const_iterator k = rs.include_group_effective.begin();k!=rs.include_group_effective.end();++k) {
			if ((*j)->name_get() == *k) {
				pred_in = true;
				break;
			}
		}
		if (!pred_in)
			all_select = false;
	}

	// remove all
	rs.include_group_effective.erase(rs.include_group_effective.begin(), rs.include_group_effective.end());

	if (!all_select && next_select == 0) {
		// insert all
		for(pcategory_container::const_iterator j=rs.group.begin();j!=rs.group.end();++j) {
			rs.include_group_effective.insert( rs.include_group_effective.end(), (*j)->name_get() );
		}
	} else {
		if ((all_select || next_select == 0) && rs.group.begin() != rs.group.end())
			next_select = *rs.group.begin();
		if (next_select != 0) {
			// insert the next
			rs.include_group_effective.insert( rs.include_group_effective.end(), next_select->name_get() );
		}
	}
}

// ------------------------------------------------------------------------
// Emu menu

#define EMU_CHOICE_DX 20*int_font_dx_get()

void run_emu(config_state& rs)
{
	choice_bag ch;

	for(pemulator_container::const_iterator j = rs.emu_active.begin();j!=rs.emu_active.end();++j) {
		bool tag = false;
		for(emulator_container::const_iterator k = rs.include_emu_effective.begin();k!=rs.include_emu_effective.end();++k) {
			if ((*j)->user_name_get() == *k) {
				tag = true;
				break;
			}
		}
		ch.insert( ch.end(), choice((*j)->user_name_get(), tag, 0) );
	}

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Show emulators", SECOND_CHOICE_X, SECOND_CHOICE_Y, EMU_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.include_emu_effective.erase(rs.include_emu_effective.begin(), rs.include_emu_effective.end());
		for(choice_bag::const_iterator j=ch.begin();j!=ch.end();++j) {
			if (j->bistate_get())
				rs.include_emu_effective.insert( rs.include_emu_effective.end(), j->desc_get() );
		}
	}
}

emulator* run_emu_select(config_state& rs)
{
	choice_bag ch;

	for(emulator_container::const_iterator j = rs.include_emu_effective.begin();j!=rs.include_emu_effective.end();++j) {
		ch.insert( ch.end(), choice(*j, 0) );
	}

	string emu;

	if (ch.size() > 1) {
		choice_bag::iterator i = ch.begin();
		int key = ch.run(" Which emulator", SECOND_CHOICE_X, SECOND_CHOICE_Y, EMU_CHOICE_DX, i);

		if (key != INT_KEY_ENTER)
			return 0;

		emu = i->desc_get();
	} else {
		emu = ch.begin()->desc_get();
	}

	for(pemulator_container::const_iterator j = rs.emu_active.begin();j!=rs.emu_active.end();++j) {
		if ((*j)->user_name_get() == emu) {
			return *j;
		}
	}

	return 0;
}

void run_emu_next(config_state& rs)
{
	string next_select = "";
	bool pred_in = false;

	for(pemulator_container::const_iterator j=rs.emu_active.begin();j!=rs.emu_active.end();++j) {
		if (pred_in)
			next_select = (*j)->user_name_get();
		pred_in = false;
		for(emulator_container::const_iterator k = rs.include_emu_effective.begin();k!=rs.include_emu_effective.end();++k) {
			if ((*j)->user_name_get() == *k) {
				pred_in = true;
				break;
			}
		}
	}
	if (next_select.length() == 0 && rs.emu_active.begin() != rs.emu_active.end())
		next_select = (*rs.emu_active.begin())->user_name_get();

	rs.include_emu_effective.erase(rs.include_emu_effective.begin(), rs.include_emu_effective.end());

	if (next_select.length() != 0)
		rs.include_emu_effective.insert( rs.include_emu_effective.end(), next_select );
}

// ------------------------------------------------------------------------
// Type menu

#define TYPE_CHOICE_DX 30*int_font_dx_get()

void run_type(config_state& rs)
{
	choice_bag ch;

	for(pcategory_container::const_iterator j = rs.type.begin();j!=rs.type.end();++j) {
		bool tag = rs.include_type_effective.find((*j)->name_get()) != rs.include_type_effective.end();
		ch.insert( ch.end(), choice((*j)->name_get(), tag, 0) );
	}

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Show types", SECOND_CHOICE_X, SECOND_CHOICE_Y, TYPE_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.include_type_effective.erase(rs.include_type_effective.begin(), rs.include_type_effective.end());
		for(choice_bag::const_iterator j=ch.begin();j!=ch.end();++j) {
			if (j->bistate_get())
				rs.include_type_effective.insert( j->desc_get() );
		}
	}
}

void run_type_next(config_state& rs)
{
	category* next_select = 0;
	bool all_select = true;

	bool pred_in = false;
	for(pcategory_container::const_iterator j=rs.type.begin();j!=rs.type.end();++j) {
		if (pred_in)
			next_select = *j;
		pred_in = false;
		for(category_container::const_iterator k = rs.include_type_effective.begin();k!=rs.include_type_effective.end();++k) {
			if ((*j)->name_get() == *k) {
				pred_in = true;
				break;
			}
		}
		if (!pred_in)
			all_select = false;
	}

	// remove all
	rs.include_type_effective.erase(rs.include_type_effective.begin(), rs.include_type_effective.end());

	if (!all_select && next_select == 0) {
		// insert all
		for(pcategory_container::const_iterator j=rs.type.begin();j!=rs.type.end();++j) {
			rs.include_type_effective.insert( rs.include_type_effective.end(), (*j)->name_get() );
		}
	} else {
		if ((all_select || next_select == 0) && rs.type.begin() != rs.type.end())
			next_select = *rs.type.begin();
		if (next_select != 0) {
			// insert the next
			rs.include_type_effective.insert( rs.include_type_effective.end(), next_select->name_get() );
		}
	}
}


// ------------------------------------------------------------------------
// Move menu

void run_group_move(config_state& rs)
{
	choice_bag ch;

	if (!rs.current_game)
		return;

	for(pcategory_container::const_iterator j = rs.group.begin();j!=rs.group.end();++j) {
		ch.insert( ch.end(), choice((*j)->name_get(), 0) );
	}

	choice_bag::iterator i = ch.find_by_desc(rs.current_game->group_get()->name_get());
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Select game group", SECOND_CHOICE_X, SECOND_CHOICE_Y, GROUP_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.current_game->user_group_set( rs.group.insert(i->desc_get()) );
	}
}

void run_type_move(config_state& rs)
{
	choice_bag ch;

	if (!rs.current_game)
		return;

	for(pcategory_container::const_iterator j = rs.type.begin();j!=rs.type.end();++j) {
		ch.insert( ch.end(), choice((*j)->name_get(), 0) );
	}

	choice_bag::iterator i = ch.find_by_desc(rs.current_game->type_get()->name_get());
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Select game type", SECOND_CHOICE_X, SECOND_CHOICE_Y, TYPE_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.current_game->user_type_set( rs.type.insert(i->desc_get()) );
	}
}

// ------------------------------------------------------------------------
// Clone menu

#define CLONE_CHOICE_X int_dx_get()/10
#define CLONE_CHOICE_Y int_dy_get()/5
#define CLONE_CHOICE_DX int_dx_get()*4/5

void run_clone(config_state& rs)
{
	choice_bag ch;
	choice_bag::iterator i;
	const game* base;

	rs.current_clone = 0;
	if (!rs.current_game)
		return;

	if (rs.current_game->software_get()) {
		base = &rs.current_game->root_get();
	} else {
		base = rs.current_game;
	}

	ostringstream s;
	s << base->description_get() << ", " << base->manufacturer_get() << ", " << base->year_get();
	ch.insert( ch.end(), choice( s.str(), (void*)base ) );

	for(pgame_container::const_iterator j = base->clone_bag_get().begin();j!=base->clone_bag_get().end();++j) {
		if (!(*j)->software_get()) {
			ostringstream s;

			s << (*j)->description_get() << ", " << (*j)->manufacturer_get() << ", " << (*j)->year_get();

			switch ((*j)->play_get()) {
			case play_minor : s << " [with minor problems]"; break;
			case play_major : s << " [not working]"; break;
			case play_not : s << " [not working]"; break;
			default: break;
			}

			ch.insert( ch.end(), choice( s.str(), (void*)&**j ) );
		}
	}

	i = ch.begin();
	int key = ch.run(" Select game clone", CLONE_CHOICE_X, CLONE_CHOICE_Y, CLONE_CHOICE_DX, i);
	if (key == INT_KEY_ENTER) {
		rs.current_clone = (game*)i->ptr_get();
	}
}

// ------------------------------------------------------------------------
// Calib menu

#define CALIB_CHOICE_DX 30*int_font_dx_get()
#define CALIB_CHOICE_X (int_dx_get()-CALIB_CHOICE_DX)/2
#define CALIB_CHOICE_Y int_dy_get()/2

void run_calib(config_state& rs)
{
	const char* message = 0;
	bool at_least_one = false;

	joystickb_calib_start();

	while (1) {
		const char* ope = joystickb_calib_next();

		if (!ope) {
			if (!at_least_one)
				message = "Calibration not needed";
			break;
		}

		choice_bag ch;
		ch.insert( ch.end(), choice(ope, 0) );

		choice_bag::iterator i = ch.begin();
		int key = ch.run(" Joystick Calibration", CALIB_CHOICE_X, CALIB_CHOICE_Y, CALIB_CHOICE_DX, i);

		if (key != INT_KEY_ENTER) {
			message = 0;
			break;
		}

		at_least_one = true;
	}

	if (message) {
		choice_bag ch;
		ch.insert( ch.end(), choice(message, 0) );
		choice_bag::iterator i = ch.begin();
		ch.run(" Joystick Calibration", CALIB_CHOICE_X, CALIB_CHOICE_Y, CALIB_CHOICE_DX, i);
	}
}

// ------------------------------------------------------------------------
// Volume

#define VOLUME_CHOICE_DX 10*int_font_dx_get()

void run_volume(config_state& rs)
{
	choice_bag ch;

	ch.insert( ch.end(), choice("Full", 0) );
	ch.insert( ch.end(), choice("-2 db", -2) );
	ch.insert( ch.end(), choice("-4 db", -4) );
	ch.insert( ch.end(), choice("-6 db", -6) );
	ch.insert( ch.end(), choice("-8 db", -8) );
	ch.insert( ch.end(), choice("-10 db", -10) );
	ch.insert( ch.end(), choice("-12 db", -12) );
	ch.insert( ch.end(), choice("-14 db", -14) );
	ch.insert( ch.end(), choice("-16 db", -16) );
	ch.insert( ch.end(), choice("-18 db", -18) );
	ch.insert( ch.end(), choice("-20 db", -20) );
	ch.insert( ch.end(), choice("Silence", -40) );

	choice_bag::iterator i = ch.find_by_value(play_attenuation_get());
	if (i == ch.end())
		i = ch.find_by_value(play_attenuation_get() - 1); // if the value is odd
	if (i == ch.end())
		i = ch.begin();

	int key = ch.run(" Volume", SECOND_CHOICE_X, SECOND_CHOICE_Y, VOLUME_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		play_attenuation_set(i->value_get());
	}
}

// ------------------------------------------------------------------------
// Difficulty

#define DIFFICULTY_CHOICE_DX 10*int_font_dx_get()

void run_difficulty(config_state& rs)
{
	choice_bag ch;

	ch.insert( ch.end(), choice("Default", difficulty_none) );
	ch.insert( ch.end(), choice("Easiest", difficulty_easiest) );
	ch.insert( ch.end(), choice("Easy", difficulty_easy) );
	ch.insert( ch.end(), choice("Normal", difficulty_medium) );
	ch.insert( ch.end(), choice("Hard", difficulty_hard) );
	ch.insert( ch.end(), choice("Hardest", difficulty_hardest) );

	choice_bag::iterator i = ch.find_by_value(rs.difficulty_effective);
	if (i == ch.end())
		i = ch.begin();
	int key = ch.run(" Difficulty", SECOND_CHOICE_X, SECOND_CHOICE_Y, DIFFICULTY_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		rs.difficulty_effective = static_cast<difficulty_t>(i->value_get());
	}
}

// ------------------------------------------------------------------------
// Sub Menu

#define MENU_CHOICE_DX 25*int_font_dx_get()

unsigned run_submenu(config_state& rs)
{
	choice_bag ch;
	unsigned ret = INT_KEY_NONE;

	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Config - Sort", 0) );
	if (rs.type.size() > 1)
		ch.insert( ch.end(), choice("Config - Type", 1) );
	if (rs.group.size() > 1)
		ch.insert( ch.end(), choice("Config - Group", 2) );
	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Config - Selection", 3) );
	if (rs.emu.size() > 1)
		ch.insert( ch.end(), choice("Config - Emulator", 4) );
	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Config - Mode", 5) );
	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Config - Preview", 6) );
	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Config - Calibration", 13) );
	ch.insert( ch.end(), choice("Config - Volume", 16) );
	ch.insert( ch.end(), choice("Config - Difficulty", 17) );
	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Config - Lock-Unlock", 12) );
	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Config - Save as Default", 11) );
	if (rs.type.size() > 1 && !rs.console_mode)
		ch.insert( ch.end(), choice("Game - Set Type", 8) );
	if (rs.group.size() > 1 && !rs.console_mode)
		ch.insert( ch.end(), choice("Game - Set Group", 9) );
	ch.insert( ch.end(), choice("Game - Run", 14) );
	ch.insert( ch.end(), choice("Game - Run Clone", 15) );
	if (!rs.console_mode || rs.script_bag.size()!=0)
		ch.insert( ch.end(), choice(rs.script_menu, 7) );
	ch.insert( ch.end(), choice("Help", 10) );
	if (!rs.console_mode)
		ch.insert( ch.end(), choice("Statistics", 18) );
	if (rs.exit_mode == exit_normal || rs.exit_mode == exit_all) {
		ch.insert( ch.end(), choice("Exit", 19) );
	}
	if (rs.exit_mode == exit_shutdown || rs.exit_mode == exit_all) {
		ch.insert( ch.end(), choice("Poweroff", 20) );
	}

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Menu", FIRST_CHOICE_X, FIRST_CHOICE_Y, MENU_CHOICE_DX, i);

	if (key == INT_KEY_ENTER) {
		emulator* emu;

		switch (i->value_get()) {
			case 0 :
				run_sort(rs);
				break;
			case 1 :
				run_type(rs);
				break;
			case 2 :
				run_group(rs);
				break;
			case 3 :
				emu = run_emu_select(rs);
				if (emu)
					emu->attrib_run();
				break;
			case 4 :
				run_emu(rs);
				break;
			case 5 :
				run_mode(rs);
				break;
			case 6 :
				run_preview(rs);
				break;
			case 7 :
				run_command(rs);
				break;
			case 8 :
				run_type_move(rs);
				break;
			case 9 :
				run_group_move(rs);
				break;
			case 10 :
				run_help(rs);
				break;
			case 11 :
				rs.restore_save();
				break;
			case 12 :
				rs.lock_effective = !rs.lock_effective;
				break;
			case 13 :
				run_calib(rs);
				break;
			case 14 :
				ret = INT_KEY_ENTER;
				break;
			case 15 :
				ret = INT_KEY_RUN_CLONE;
				break;
			case 16 :
				run_volume(rs);
				break;
			case 17 :
				run_difficulty(rs);
				break;
			case 18 :
				run_stat(rs);
				break;
			case 19 :
				ret = INT_KEY_ESC;
				break;
			case 20 :
				ret = INT_KEY_OFF;
				break;
		}
	}

	return ret;
}

// ------------------------------------------------------------------------
// Help menu

void run_help(config_state& rs)
{
	int_clear(0, 0, int_dx_get(), int_dy_get(), COLOR_HELP_NORMAL.background);
	int_clear(0, 0, int_dx_get(), int_font_dy_get(), COLOR_MENU_BAR.background);

	if (rs.ui_help != "none") {
		unsigned x, y;
		int_image(rs.ui_help.c_str(), x, y);
	} else {
		int_put(4*int_font_dx_get(), 0, "HELP", COLOR_MENU_BAR_TAG);

		int y = 2*int_font_dy_get();
		int xt = 2*int_font_dx_get();
		int xd = (2+12)*int_font_dx_get();
		if (rs.console_mode) {
			int_put(xt, y, "ESC", COLOR_HELP_TAG);
			int_put(xd, y, "Main menu", COLOR_HELP_NORMAL);
		} else {
			int_put(xt, y, "TILDE", COLOR_HELP_TAG);
			int_put(xd, y, "Main menu (TILDE is the key below ESC)", COLOR_HELP_NORMAL);
		}
		y += int_font_dy_get();
		int_put(xt, y, "ENTER", COLOR_HELP_TAG);
		int_put(xd, y, "Run the current game/On menu accept the choice", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "SPACE", COLOR_HELP_TAG);
		int_put(xd, y, "Next preview mode/On menu change the option", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "TAB", COLOR_HELP_TAG);
		int_put(xd, y, "Next menu mode", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		if (rs.exit_mode == exit_normal || rs.exit_mode == exit_all) {
			if (!rs.console_mode) {
				int_put(xt, y, "ESC", COLOR_HELP_TAG);
				int_put(xd, y, "Exit", COLOR_HELP_NORMAL);
				y += int_font_dy_get();
			}
		}
		if (rs.exit_mode == exit_shutdown || rs.exit_mode == exit_all) {
			int_put(xt, y, "CTRL-ESC", COLOR_HELP_TAG);
			int_put(xd, y, "Shutdown", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
		if (rs.group.size() > 1) {
			int_put(xt, y, "F2", COLOR_HELP_TAG);
			int_put(xd, y, "Next game group", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
		if (rs.type.size() > 1) {
			int_put(xt, y, "F3", COLOR_HELP_TAG);
			int_put(xd, y, "Next game type", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
		if (!rs.console_mode) {
			int_put(xt, y, "F4", COLOR_HELP_TAG);
			int_put(xd, y, "Include/Exclude games by attribute", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
			int_put(xt, y, "F5", COLOR_HELP_TAG);
			int_put(xd, y, "Select the game sort method", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
		int_put(xt, y, "F6", COLOR_HELP_TAG);
		int_put(xd, y, "Next emulator", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		if (!rs.console_mode) {
			int_put(xt, y, "F8", COLOR_HELP_TAG);
			int_put(xd, y, "Commands", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
		if (rs.group.size() > 1) {
			int_put(xt, y, "F9", COLOR_HELP_TAG);
			int_put(xd, y, "Change the current game group", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
		if (rs.type.size() > 1) {
			int_put(xt, y, "F10", COLOR_HELP_TAG);
			int_put(xd, y, "Change the current game type", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
		int_put(xt, y, "F12", COLOR_HELP_TAG);
		int_put(xd, y, "Run a clone", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "0 PAD", COLOR_HELP_TAG);
		int_put(xd, y, "Rotate the screen", COLOR_HELP_NORMAL);
		y += int_font_dy_get();

		y += int_font_dy_get();
		int_put(xt, y, "In the selection submenus:", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "ENTER", COLOR_HELP_TAG);
		int_put(xd, y, "Accept", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "DEL", COLOR_HELP_TAG);
		int_put(xd, y, "Unselect all", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "INS", COLOR_HELP_TAG);
		int_put(xd, y, "Select all", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "SPACE", COLOR_HELP_TAG);
		int_put(xd, y, "Toggle (+ include, - exclude, * required)", COLOR_HELP_NORMAL);
		y += int_font_dy_get();
		int_put(xt, y, "ESC", COLOR_HELP_TAG);
		int_put(xd, y, "Cancel", COLOR_HELP_NORMAL);
		y += int_font_dy_get();

		if (rs.console_mode) {
			y += int_font_dy_get();
			int_put(xt, y, "In the emulators:", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
			int_put(xt, y, "F1", COLOR_HELP_TAG);
			int_put(xd, y, "Help", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
			int_put(xt, y, "5 6 7 8", COLOR_HELP_TAG);
			int_put(xd, y, "Insert Coins", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
			int_put(xt, y, "1 2 3 4", COLOR_HELP_TAG);
			int_put(xd, y, "Start Player 1, 2, 3, 4", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
			int_put(xt, y, "CTRL ALT SPACE", COLOR_HELP_TAG);
			int_put(xd, y, "Buttons Player 1", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
			int_put(xt, y, "ARROWS", COLOR_HELP_TAG);
			int_put(xd, y, "Move Player 1", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
			int_put(xt, y, "ESC", COLOR_HELP_TAG);
			int_put(xd, y, "Return to the menu", COLOR_HELP_NORMAL);
			y += int_font_dy_get();
		}
	}

	int_getkey();
}

// ------------------------------------------------------------------------
// Stat

#define STAT_MAX 10

void stat_insert(const game* (&map)[STAT_MAX], unsigned (&val)[STAT_MAX], const game* g, unsigned v)
{
	for(unsigned i=0;i<STAT_MAX;++i) {
		if (!map[i] || val[i] < v) {
			for(unsigned j=STAT_MAX-1;j>i;--j) {
				map[j] = map[j-1];
				val[j] = val[j-1];
			}
			map[i] = g;
			val[i] = v;
			return;
		}
	}
}

string stat_time(unsigned v)
{
	ostringstream os;
	os << (v/3600) << ":" << setw(2) << setfill('0') << ((v/60)%60);
	return os.str();
}

string stat_int(unsigned v)
{
	ostringstream os;
	os << v;
	return os.str();
}

string stat_perc(unsigned v, unsigned t)
{
	ostringstream os;
	if (t)
		os << stat_int(v * 100 / t) << "%";
	else
		os << stat_int(0) << "%";
	return os.str();
}

void run_stat(config_state& rs)
{
	unsigned total_count = 0;
	unsigned total_session = 0;
	unsigned total_time = 0;
	unsigned select_count = 0;
	unsigned select_session = 0;
	unsigned select_time = 0;
	const game* most_session_map[STAT_MAX] = { 0, 0, 0 };
	const game* most_time_map[STAT_MAX] = { 0, 0, 0 };
	const game* most_timepersession_map[STAT_MAX] = { 0, 0, 0 };
	unsigned most_session_val[STAT_MAX] = { 0, 0, 0 };
	unsigned most_time_val[STAT_MAX] = { 0, 0, 0 };
	unsigned most_timepersession_val[STAT_MAX] = { 0, 0, 0 };
	unsigned n;

	int y = 2*int_font_dy_get();
	int xn = 2*int_font_dx_get();
	int xs = (2+1*8)*int_font_dx_get();
	int xt = (2+2*8)*int_font_dx_get();
	int xp = (2+3*8)*int_font_dx_get();
	int xe = (2+4*8)*int_font_dx_get();

	n = (int_dy_get() / int_font_dy_get() - 14) / 3;
	if (n > STAT_MAX)
		n = STAT_MAX;

	int_clear(0, 0, int_dx_get(), int_dy_get(), COLOR_HELP_NORMAL.background);
	int_clear(0, 0, int_dx_get(), int_font_dy_get(), COLOR_MENU_BAR.background);
	int_put(4*int_font_dx_get(), 0, "STATISTICS", COLOR_MENU_BAR_TAG);

	// select and sort
	for(game_set::const_iterator i=rs.gar.begin();i!=rs.gar.end();++i) {
		unsigned session;
		unsigned time;
		unsigned timepersession;

		if (i->emulator_get()->tree_get())
			session = i->session_tree_get();
		else
			session = i->session_get();
		if (i->emulator_get()->tree_get())
			time = i->time_tree_get();
		else
			time = i->time_get();
		if (session)
			timepersession = time / session;
		else
			timepersession = 0;

		total_count += 1;
		total_session += session;
		total_time += time;

		// emulator
		if (!i->emulator_get()->state_get())
			continue;

		// group
		if (!i->group_derived_get()->state_get())
			continue;

		// type
		if (!i->type_derived_get()->state_get())
			continue;

		// filter
		if (!i->emulator_get()->filter(*i))
			continue;

		select_count += 1;
		select_session += session;
		select_time += time;

		stat_insert(most_session_map, most_session_val, &*i, session);
		stat_insert(most_time_map, most_time_val, &*i, time);
		stat_insert(most_timepersession_map, most_timepersession_val, &*i, timepersession);
	}

	int_put_right(xs, y, xt-xs, "Listed", COLOR_HELP_TAG);
	int_put_right(xt, y, xp-xt, "Total", COLOR_HELP_TAG);
	int_put_right(xp, y, xe-xp, "Perc", COLOR_HELP_TAG);

	{

		y += int_font_dy_get();
		int_put(xn, y, "Games", COLOR_HELP_TAG);
		int_put_right(xs, y, xt-xs, stat_int(select_count), COLOR_HELP_NORMAL);
		int_put_right(xt, y, xp-xt, stat_int(total_count), COLOR_HELP_NORMAL);
		int_put_right(xp, y, xe-xp, stat_perc(select_count, total_count), COLOR_HELP_NORMAL);
	}

	{
		y += int_font_dy_get();
		int_put(xn, y, "Play", COLOR_HELP_TAG);
		int_put_right(xs, y, xt-xs, stat_int(select_session), COLOR_HELP_NORMAL);
		int_put_right(xt, y, xp-xt, stat_int(total_session), COLOR_HELP_NORMAL);
		int_put_right(xp, y, xe-xp, stat_perc(select_session, total_session), COLOR_HELP_NORMAL);
	}

	{
		y += int_font_dy_get();
		int_put(xn, y, "Time", COLOR_HELP_TAG);
		int_put_right(xs, y, xt-xs, stat_time(select_time), COLOR_HELP_NORMAL);
		int_put_right(xt, y, xp-xt, stat_time(total_time), COLOR_HELP_NORMAL);
		int_put_right(xp, y, xe-xp, stat_perc(select_time, total_time), COLOR_HELP_NORMAL);
	}

	xs = (1+7)*int_font_dx_get();
	xe = (1+7+5)*int_font_dx_get();
	xt = (1+7+5+2)*int_font_dx_get();

	if (n>0 && most_time_map[0]) {
		y += int_font_dy_get();
		y += int_font_dy_get();
		int_put(xn, y, "Most time", COLOR_HELP_TAG);
		for(unsigned i=0;i<n && most_time_map[i];++i) {
			y += int_font_dy_get();
			int_put_right(xn, y, xs-xn, stat_time(most_time_val[i]), COLOR_HELP_NORMAL);
			int_put_right(xs, y, xe-xs, stat_perc(most_time_val[i], select_time), COLOR_HELP_NORMAL);
			int_put(xt, y, most_time_map[i]->description_get(), COLOR_HELP_NORMAL);
		}
	}

	if (n>0 && most_session_map[0]) {
		ostringstream os;
		y += int_font_dy_get();
		y += int_font_dy_get();
		int_put(xn, y, "Most play", COLOR_HELP_TAG);
		for(unsigned i=0;i<n && most_session_map[i];++i) {
			y += int_font_dy_get();
			int_put_right(xn, y, xs-xn, stat_int(most_session_val[i]), COLOR_HELP_NORMAL);
			int_put_right(xs, y, xe-xs, stat_perc(most_session_val[i], select_session), COLOR_HELP_NORMAL);
			int_put(xt, y, most_session_map[i]->description_get(), COLOR_HELP_NORMAL);
		}
	}

	if (n>0 && most_timepersession_map[0]) {
		ostringstream os;
		y += int_font_dy_get();
		y += int_font_dy_get();
		int_put(xn, y, "Most time per play", COLOR_HELP_TAG);
		for(unsigned i=0;i<n && most_timepersession_map[i];++i) {
			y += int_font_dy_get();
			int_put_right(xn, y, xs-xn, stat_time(most_timepersession_val[i]), COLOR_HELP_NORMAL);
			int_put(xt, y, most_timepersession_map[i]->description_get(), COLOR_HELP_NORMAL);
		}
	}

	int_getkey();
}



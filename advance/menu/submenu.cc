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

#include "submenu.h"
#include "text.h"
#include "joydrv.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

#include <unistd.h>

using namespace std;

// ------------------------------------------------------------------------
// Sort menu

#define SORT_CHOICE_X text_dx_get()/8
#define SORT_CHOICE_Y text_dy_get()/5
#define SORT_CHOICE_DX 15*text_font_dx_get()

void run_sort(config_state& rs) {
	choice_bag ch;

	ch.insert( ch.end(), choice("Parent name", sort_by_root_name) );
	ch.insert( ch.end(), choice("Name", sort_by_name) );
	ch.insert( ch.end(), choice("Time played", sort_by_time) );
	ch.insert( ch.end(), choice("Coins", sort_by_coin) );
	ch.insert( ch.end(), choice("Group", sort_by_group) );
	ch.insert( ch.end(), choice("Type", sort_by_type) );
	ch.insert( ch.end(), choice("Manufacturer", sort_by_manufacturer) );
	ch.insert( ch.end(), choice("Year", sort_by_year) );
	ch.insert( ch.end(), choice("Size", sort_by_size) );
	ch.insert( ch.end(), choice("Resolution", sort_by_res) );
	ch.insert( ch.end(), choice("Info", sort_by_info) );

	choice_bag::iterator i = ch.find_by_value(rs.sort_effective);
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Sort mode", SORT_CHOICE_X, SORT_CHOICE_Y, SORT_CHOICE_DX, i);
	if (key == TEXT_KEY_ENTER) {
		rs.sort_effective = (game_sort_t)i->value_get();
	}
}

// ------------------------------------------------------------------------
// Command menu

#define COMMAND_CHOICE_X text_dx_get()/8
#define COMMAND_CHOICE_Y text_dy_get()/5
#define COMMAND_CHOICE_DX 33*text_font_dx_get()

void run_command(config_state& rs) {
	choice_bag ch;
	bool used_backdrop = false;
	bool used_sound = false;

	if (rs.current_game && rs.current_game->preview_snap_get().is_deletable()) {
		string s = "Delete game snapshot";
		if (rs.current_game->preview_snap_get() == rs.current_backdrop) {
			s += " (shown)";
			used_backdrop = true;
		}
		ch.insert( ch.end(), choice(s, 0) );
	}
	if (rs.current_game && rs.current_game->preview_clip_get().is_deletable()) {
		string s = "Delete game clip";
		if (rs.current_game->preview_clip_get() == rs.current_backdrop) {
			s += " (shown)";
			used_backdrop = true;
		}
		ch.insert( ch.end(), choice(s, 1) );
	}
	if (rs.current_game && rs.current_game->preview_flyer_get().is_deletable()) {
		string s = "Delete game flyer";
		if (rs.current_game->preview_flyer_get() == rs.current_backdrop) {
			s += " (shown)";
			used_backdrop = true;
		}
		ch.insert( ch.end(), choice(s, 2) );
	}
	if (rs.current_game && rs.current_game->preview_cabinet_get().is_deletable()) {
		string s = "Delete game cabinet";
		if (rs.current_game->preview_cabinet_get() == rs.current_backdrop) {
			s += " (shown)";
			used_backdrop = true;
		}
		ch.insert( ch.end(), choice(s, 3) );
	}
	if (rs.current_game && rs.current_game->preview_icon_get().is_deletable()) {
		string s = "Delete game icon";
		if (rs.current_game->preview_icon_get() == rs.current_backdrop) {
			s += " (shown)";
			used_backdrop = true;
		}
		ch.insert( ch.end(), choice(s, 4) );
	}
	if (!used_backdrop && rs.current_game && rs.current_backdrop.is_deletable()) {
		string s = "Delete shown image (from parent)";
		ch.insert( ch.end(), choice(s, 5) );
	}
	if (rs.current_game && rs.current_game->preview_sound_get().is_deletable()) {
		string s = "Delete game sound";
		if (rs.current_game->preview_sound_get() == rs.current_sound) {
			s += " (played)";
			used_sound = true;
		}
		ch.insert( ch.end(), choice(s, 6) );
	}
	if (!used_sound && rs.current_game && rs.current_sound.is_deletable()) {
		ch.insert( ch.end(), choice("Delete played sound (from parent)", 7) );
	}

	if (ch.begin() == ch.end())
		ch.insert( ch.end(), choice("No commands available", -1) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Commands", COMMAND_CHOICE_X, COMMAND_CHOICE_Y, COMMAND_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		switch (i->value_get()) {
			case 0 :
				remove(cpath_export(rs.current_game->preview_snap_get().archive_get()));
				break;
			case 1 :
				remove(cpath_export(rs.current_game->preview_clip_get().archive_get()));
				break;
			case 2 :
				remove(cpath_export(rs.current_game->preview_flyer_get().archive_get()));
				break;
			case 3 :
				remove(cpath_export(rs.current_game->preview_cabinet_get().archive_get()));
				break;
			case 4 :
				remove(cpath_export(rs.current_game->preview_icon_get().archive_get()));
				break;
			case 5 :
				remove(cpath_export(rs.current_backdrop.archive_get()));
				break;
			case 6 :
				remove(cpath_export(rs.current_game->preview_sound_get().archive_get()));
				break;
			case 7 :
				remove(cpath_export(rs.current_sound.archive_get()));
				break;
		}
	}
}

// ------------------------------------------------------------------------
// Mode menu

#define MODE_CHOICE_X text_dx_get()/8
#define MODE_CHOICE_Y text_dy_get()/5
#define MODE_CHOICE_DX 15*text_font_dx_get()

void run_mode(config_state& rs) {
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

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Mode", MODE_CHOICE_X, MODE_CHOICE_Y, MODE_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.mode_effective = (show_t)i->value_get();
	}
}

// ------------------------------------------------------------------------
// Preview menu

#define PREVIEW_CHOICE_X text_dx_get()/8
#define PREVIEW_CHOICE_Y text_dy_get()/5
#define PREVIEW_CHOICE_DX 15*text_font_dx_get()

void run_preview(config_state& rs) {
	choice_bag ch;

	ch.insert( ch.end(), choice("Snap", preview_snap) );
	ch.insert( ch.end(), choice("Title", preview_title) );
	ch.insert( ch.end(), choice("Flyer", preview_flyer) );
	ch.insert( ch.end(), choice("Cabinet", preview_cabinet) );
	ch.insert( ch.end(), choice("Icon", preview_icon) );
	ch.insert( ch.end(), choice("Marquee", preview_marquee) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Preview", PREVIEW_CHOICE_X, PREVIEW_CHOICE_Y, PREVIEW_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.preview_effective = (preview_t)i->value_get();
	}
}

// ------------------------------------------------------------------------
// Group menu

#define GROUP_CHOICE_X text_dx_get()/8
#define GROUP_CHOICE_Y text_dy_get()/5
#define GROUP_CHOICE_DX 20*text_font_dx_get()

void run_group(config_state& rs) {
	choice_bag ch;

	for(pcategory_container::const_iterator j = rs.group.begin();j!=rs.group.end();++j) {
		bool tag = rs.include_group_effective.find((*j)->name_get()) != rs.include_group_effective.end();
		ch.insert( ch.end(), choice((*j)->name_get(), tag, 0) );
	}

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Show groups", GROUP_CHOICE_X, GROUP_CHOICE_Y, GROUP_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.include_group_effective.erase(rs.include_group_effective.begin(), rs.include_group_effective.end());
		for(choice_bag::const_iterator j=ch.begin();j!=ch.end();++j) {
			if (j->bistate_get())
				rs.include_group_effective.insert( j->desc_get() );
		}
	}
}

// ------------------------------------------------------------------------
// Emu menu

#define EMU_CHOICE_X text_dx_get()/8
#define EMU_CHOICE_Y text_dy_get()/5
#define EMU_CHOICE_DX 20*text_font_dx_get()

void run_emu(config_state& rs) {
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
	int key = ch.run(" Show emulators", EMU_CHOICE_X, EMU_CHOICE_Y, EMU_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.include_emu_effective.erase(rs.include_emu_effective.begin(), rs.include_emu_effective.end());
		for(choice_bag::const_iterator j=ch.begin();j!=ch.end();++j) {
			if (j->bistate_get())
				rs.include_emu_effective.insert( rs.include_emu_effective.end(), j->desc_get() );
		}
	}
}

emulator* run_emu_select(config_state& rs) {
	choice_bag ch;

	for(emulator_container::const_iterator j = rs.include_emu_effective.begin();j!=rs.include_emu_effective.end();++j) {
		ch.insert( ch.end(), choice(*j, 0) );
	}

	string emu;

	if (ch.size() > 1) {
		choice_bag::iterator i = ch.begin();
		int key = ch.run(" Which emulator", EMU_CHOICE_X, EMU_CHOICE_Y, EMU_CHOICE_DX, i);

		if (key != TEXT_KEY_ENTER)
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

void run_emu_next(config_state& rs) {
	string last = "";
	bool pred_in = false;

	for(pemulator_container::const_iterator j=rs.emu_active.begin();j!=rs.emu_active.end();++j) {
		if (pred_in)
			last = (*j)->user_name_get();
		pred_in = false;
		for(emulator_container::const_iterator k = rs.include_emu_effective.begin();k!=rs.include_emu_effective.end();++k) {
			if ((*j)->user_name_get() == *k) {
				pred_in = true;
				break;
			}
		}
	}
	if (last.length() == 0 && rs.emu_active.begin() != rs.emu_active.end())
		last = (*rs.emu_active.begin())->user_name_get();

	rs.include_emu_effective.erase(rs.include_emu_effective.begin(), rs.include_emu_effective.end());

	if (last.length() != 0)
		rs.include_emu_effective.insert( rs.include_emu_effective.end(), last );
}

// ------------------------------------------------------------------------
// Type menu

#define TYPE_CHOICE_X text_dx_get()/8
#define TYPE_CHOICE_Y 2*text_font_dy_get()
#define TYPE_CHOICE_DX 30*text_font_dx_get()

void run_type(config_state& rs) {
	choice_bag ch;

	for(pcategory_container::const_iterator j = rs.type.begin();j!=rs.type.end();++j) {
		bool tag = rs.include_type_effective.find((*j)->name_get()) != rs.include_type_effective.end();
		ch.insert( ch.end(), choice((*j)->name_get(), tag, 0) );
	}

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Show types", TYPE_CHOICE_X, TYPE_CHOICE_Y, TYPE_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.include_type_effective.erase(rs.include_type_effective.begin(), rs.include_type_effective.end());
		for(choice_bag::const_iterator j=ch.begin();j!=ch.end();++j) {
			if (j->bistate_get())
				rs.include_type_effective.insert( j->desc_get() );
		}
	}
}

// ------------------------------------------------------------------------
// Move menu

void run_group_move(config_state& rs) {
	choice_bag ch;

	if (!rs.current_game)
		return;

	for(pcategory_container::const_iterator j = rs.group.begin();j!=rs.group.end();++j) {
		ch.insert( ch.end(), choice((*j)->name_get(),0) );
	}

	choice_bag::iterator i = ch.find_by_desc(rs.current_game->group_get()->name_get());
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Select group", GROUP_CHOICE_X, GROUP_CHOICE_Y, GROUP_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.current_game->user_group_set( rs.group.insert(i->desc_get()) );
	}
}

void run_type_move(config_state& rs) {
	choice_bag ch;

	if (!rs.current_game)
		return;

	for(pcategory_container::const_iterator j = rs.type.begin();j!=rs.type.end();++j) {
		ch.insert( ch.end(), choice((*j)->name_get(), 0) );
	}

	choice_bag::iterator i = ch.find_by_desc(rs.current_game->type_get()->name_get());
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Select type", TYPE_CHOICE_X, TYPE_CHOICE_Y, TYPE_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.current_game->user_type_set( rs.type.insert(i->desc_get()) );
	}
}

// ------------------------------------------------------------------------
// Clone menu

#define CLONE_CHOICE_X text_dx_get()/10
#define CLONE_CHOICE_Y text_dy_get()/5
#define CLONE_CHOICE_DX text_dx_get()*4/5

void run_clone(config_state& rs) {
	choice_bag ch;
	choice_bag::iterator i;

	rs.current_clone = 0;

	ostringstream s;
	s << rs.current_game->description_get() << ", " << rs.current_game->manufacturer_get() << ", " << rs.current_game->year_get();
	ch.insert( ch.end(), choice( s.str(), (void*)rs.current_game ) );

	for(pgame_container::const_iterator j = rs.current_game->clone_bag_get().begin();j!=rs.current_game->clone_bag_get().end();++j) {
		if (!(*j)->software_get()) {
			ostringstream s;
			s << (*j)->description_get() << ", " << (*j)->manufacturer_get() << ", " << (*j)->year_get();
			ch.insert( ch.end(), choice( s.str(), (void*)&**j ) );
		}
	}

	i = ch.begin();
	int key = ch.run(" Select clone", CLONE_CHOICE_X, CLONE_CHOICE_Y, CLONE_CHOICE_DX, i);
	if (key == TEXT_KEY_ENTER) {
		rs.current_clone = (game*)i->ptr_get();
	}
}

// ------------------------------------------------------------------------
// Calib menu

#define CALIB_CHOICE_X (text_dx_get()-30*text_font_dx_get())/2
#define CALIB_CHOICE_Y text_dy_get()/2
#define CALIB_CHOICE_DX 30*text_font_dx_get()

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

		if (key != TEXT_KEY_ENTER) {
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
// Sub Menu

#define MENU_CHOICE_X text_dx_get()/16
#define MENU_CHOICE_Y text_dy_get()/10
#define MENU_CHOICE_DX 25*text_font_dx_get()

void run_submenu(config_state& rs) {
	choice_bag ch;

	ch.insert( ch.end(), choice("Config/Sort", 0) );
	ch.insert( ch.end(), choice("Config/Type", 1) );
	ch.insert( ch.end(), choice("Config/Group", 2) );
	ch.insert( ch.end(), choice("Config/Attrib", 3) );
	ch.insert( ch.end(), choice("Config/Emulator", 4) );
	ch.insert( ch.end(), choice("Config/Mode", 5) );
	ch.insert( ch.end(), choice("Config/Preview", 6) );
	ch.insert( ch.end(), choice("Config/Calibration", 13) );
	ch.insert( ch.end(), choice("Config/Lock-Unlock", 12) );
	ch.insert( ch.end(), choice("Config/Save as Default", 11) );
	ch.insert( ch.end(), choice("Game/Type", 8) );
	ch.insert( ch.end(), choice("Game/Group", 9) );
	ch.insert( ch.end(), choice("Command", 7) );
	ch.insert( ch.end(), choice("Help", 10) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Menu", MENU_CHOICE_X, MENU_CHOICE_Y, MENU_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
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
				run_help();
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
		}
	}
}

// ------------------------------------------------------------------------
// Help menu

void run_help() {
	text_clear(0,0,text_dx_get(),text_dy_get(),COLOR_HELP_NORMAL >> 4);
	text_clear(0,0,text_dx_get(),text_font_dy_get(),COLOR_MENU_BAR >> 4);
	text_put(2*text_font_dx_get(),0,"HELP",COLOR_MENU_BAR_TAG);

	int y = 2*text_font_dy_get();
	int xt = 1*text_font_dx_get();
	int xd = 8*text_font_dx_get();
	text_put(xt,y,"TILDE",COLOR_HELP_TAG);
	text_put(xd,y,"Main menu",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"ENTER",COLOR_HELP_TAG);
	text_put(xd,y,"Run the current game/On menu accept the choice",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"SPACE",COLOR_HELP_TAG);
	text_put(xd,y,"Change the preview type/On menu change the option",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"TAB",COLOR_HELP_TAG);
	text_put(xd,y,"Change the menu mode",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"ESC",COLOR_HELP_TAG);
	text_put(xd,y,"Exit (CTRL+ESC to shutdown)",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F2",COLOR_HELP_TAG);
	text_put(xd,y,"Include/Exclude games by group",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F3",COLOR_HELP_TAG);
	text_put(xd,y,"Include/Exclude games by type",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F4",COLOR_HELP_TAG);
	text_put(xd,y,"Include/Exclude games by attribute",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F5",COLOR_HELP_TAG);
	text_put(xd,y,"Select the game sort method",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F6",COLOR_HELP_TAG);
	text_put(xd,y,"Select the emulator",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F8",COLOR_HELP_TAG);
	text_put(xd,y,"Commands",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F9",COLOR_HELP_TAG);
	text_put(xd,y,"Change the current game group",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F10",COLOR_HELP_TAG);
	text_put(xd,y,"Change the current game type",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"F12",COLOR_HELP_TAG);
	text_put(xd,y,"Run a clone",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"0 PAD",COLOR_HELP_TAG);
	text_put(xd,y,"Rotate the screen",COLOR_HELP_NORMAL);
	y += text_font_dy_get();

	y += text_font_dy_get();
	text_put(xt,y,"In the selection submenus:",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"ENTER",COLOR_HELP_TAG);
	text_put(xd,y,"Accept",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"DEL",COLOR_HELP_TAG);
	text_put(xd,y,"Unselect all",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"INS",COLOR_HELP_TAG);
	text_put(xd,y,"Select all",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"SPACE",COLOR_HELP_TAG);
	text_put(xd,y,"Toggle (+ include, - exclude, * required)",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"ESC",COLOR_HELP_TAG);
	text_put(xd,y,"Cancel",COLOR_HELP_NORMAL);

	text_getkey();
}



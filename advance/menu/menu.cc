/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005, 2009 Andrea Mazzoleni
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

#include "game.h"
#include "text.h"
#include "play.h"
#include "menu.h"

#include "advance.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

// ------------------------------------------------------------------------
// Menu entry

string menu_entry::category(sort_item_func* category_extract)
{
	if (has_game())
		return category_extract(game_get());
	else
		return desc_get();
}

menu_entry::menu_entry(const game* Ag, const unsigned Aident)
{
	g = Ag;
	ident = Aident;
}

menu_entry::menu_entry(const string& Adesc)
{
	g = 0;
	desc = Adesc;
	ident = 0;
}

// ------------------------------------------------------------------------
// Menu draw

static inline bool issep(char c)
{
	return isspace(c) || (c == '-');
}

void draw_menu_game_center(const game_set& gar, const game& g, int x, int y, int dx, int dy, bool selected, merge_t merge)
{
	string s;
	if (g.emulator_get()->tree_get())
		s = g.description_tree_get();
	else
		s = g.description_get();

	unsigned ident = int_font_dx_get(text) / 2;

	int_color color;
	int_color color_first;
	int_color color_in;
	int_color color_back;
	const game* gb;
	if (g.emulator_get()->tree_get())
		gb = &g.clone_best_get();
	else
		gb = &g;
	if (gb->play_get() != play_preliminary && gb->present_tree_get()) {
		color = selected ? COLOR_MENU_SELECT : COLOR_MENU_NORMAL;
		color_first = selected ? COLOR_MENU_TAG_SELECT : COLOR_MENU_TAG;
		color_in = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
		color_back = COLOR_MENU_NORMAL;
	} else {
		color = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
		color_first = color;
		color_in = color;
		color_back = COLOR_MENU_HIDDEN;
	}

	// search the split of the first word
	unsigned str_pos = 0;
	unsigned py = 0;
	bool in = false;

	while (dy >= py + int_font_dy_get(text)) {
		unsigned str_len = s.length() - str_pos;
		while (str_len > 0 && issep(s[str_pos])) {
			++str_pos;
			--str_len;
		}
		unsigned pixel_len = int_put_width(text, s.substr(str_pos, str_len));
		while (pixel_len > dx - 2 * ident) {
			while (str_len > 0 && !issep(s[str_pos + str_len - 1]))
				--str_len;
			while (str_len > 0 && issep(s[str_pos + str_len - 1]))
				--str_len;
			pixel_len = int_put_width(text, s.substr(str_pos, str_len));
		}

		if (str_len == 0) {
			// retry splitting in any point
			str_len = s.length() - str_pos;
			if (str_len) {
				pixel_len = int_put_width(text, s.substr(str_pos, str_len));
				while (pixel_len > dx - 2 * ident) {
					--str_len;
					pixel_len = int_put_width(text, s.substr(str_pos, str_len));
				}
			}
		}

		if (!str_len) {
			int_clear_alpha(x, y + py, dx, int_font_dy_get(text), color_back.background);
		} else {
			int space_left = (dx - pixel_len) / 2;
			int ident_left = ident;
			if (ident_left > space_left)
				ident_left = space_left;
			int space_right = dx - pixel_len - space_left;
			int ident_right = ident;
			if (ident_right > space_right)
				ident_right = space_right;

			int_clear_alpha(x, y + py, space_left - ident_left, int_font_dy_get(text), color_back.background);
			int_clear_alpha(x + space_left - ident_left, y + py, ident_left, int_font_dy_get(text), color.background);
			int_put_special_alpha(text, in, x + space_left, y + py, pixel_len, s.substr(str_pos, str_len), color_first, color_in, color);
			int_clear_alpha(x + space_left + pixel_len, y + py, ident_right, int_font_dy_get(text), color.background);
			int_clear_alpha(x + space_left + pixel_len + ident_right, y + py, space_right - ident_right, int_font_dy_get(text), color_back.background);
		}

		color_first = color;
		py += int_font_dy_get(text);
		str_pos += str_len;
	}
}

void draw_menu_game_left(const game_set& gar, const game& g, int x, int y, int dx, bool selected, merge_t merge, unsigned ident)
{
	string s;
	if (g.emulator_get()->tree_get())
		s = g.description_tree_get();
	else
		s = g.description_get();

	ident += int_font_dx_get(text) / 2;

	int_color color;
	int_color color_first;
	int_color color_in;
	const game* gb;
	if (g.emulator_get()->tree_get())
		gb = &g.clone_best_get();
	else
		gb = &g;
	if (gb->play_get() != play_preliminary && gb->present_tree_get()) {
		color = selected ? COLOR_MENU_SELECT : COLOR_MENU_NORMAL;
		color_first = selected ? COLOR_MENU_TAG_SELECT : COLOR_MENU_TAG;
		color_in = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
	} else {
		color = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
		color_first = color;
		color_in = color;
	}

	if (ident)
		int_clear_alpha(x, y, ident, int_font_dy_get(text), color.background);

	bool in = false;
	int_put_special_alpha(text, in, x + ident, y, dx - ident, s, color_first, color_in, color);
}

void draw_menu_empty(int x, int y, int dx, int dy, bool selected)
{
	int_color color;
	color = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
	int_clear_alpha(x, y, dx, dy, color.background);
}

void draw_menu_desc(const string& desc, int x, int y, int dx, bool selected)
{
	int_color color;
	color = selected ? COLOR_MENU_TAG_SELECT : COLOR_MENU_TAG;
	int_put_filled_alpha(text, x, y, dx, desc, color);
}

string reduce_string(font_t font, const string& s, unsigned width)
{
	string r = s;

	while (int_put_width(font, r) > width && r.length()) {
		r.erase(r.length() - 1, 1);
	}

	return r;
}

void draw_tag_left(font_t font, const string& s, int& xl, int& xr, int y, int sep, const int_color& color)
{
	string r = reduce_string(font, s, (xr - xl) - sep);

	int len = sep + int_put_width(font, r);

	if (len <= xr - xl) {
		int_put_alpha(font, xl, y, r, color);
		xl += len;
	}
}

void draw_tag_left_whole(font_t font, const string& s, int& xl, int& xr, int y, int sep, const int_color& color)
{
	if (int_put_width(font, s) <= (xr - xl) - sep)
		draw_tag_left(font, s, xl, xr, y, sep, color);
}

void draw_tag_right(font_t font, const string& s, int& xl, int& xr, int y, int sep, const int_color& color)
{
	string r = reduce_string(font, s, (xr - xl) - sep);

	int len = sep + int_put_width(font, r);

	if (len <= xr - xl) {
		int_put_alpha(font, xr - len + sep, y, r, color);
		xr -= len;
	}
}

void draw_tag_right_whole(font_t font, const string& s, int& xl, int& xr, int y, int sep, const int_color& color)
{
	if (int_put_width(font, s) <= (xr - xl) - sep)
		draw_tag_right(font, s, xl, xr, y, sep, color);
}

void draw_menu_bar(const game* g, int g2, int x, int y, int dx, bool lock, bool bottom)
{
	font_t font = bar;

	int_clear_alpha(x, y, dx, int_font_dy_get(font), COLOR_MENU_BAR.background);

	int separator = dx > 40 * int_font_dx_get(font) ? 1 * int_font_dx_get(font) : 0 * int_font_dx_get(font);
	int in_separator = dx > 40 * int_font_dx_get(font) ? 2 * int_font_dx_get(font) : 1 * int_font_dx_get(font);
	int xr = dx - separator + x;
	int xl = separator + x;

	if (!lock && g) {
		ostringstream os;
		os << setw(4) << setfill(' ') << g2;
		draw_tag_right(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR);
	}

	if (g) {
		ostringstream os;
		unsigned time;
		if (g->emulator_get()->tree_get())
			time = g->time_tree_get();
		else
			time = g->time_get();
		os << setw(3) << setfill('0') << (time / 3600) << ":" << setw(2) << setfill('0') << ((time / 60) % 60);
		draw_tag_right_whole(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR);
	}

	if (g) {
		ostringstream os;
		unsigned session;
		if (g->emulator_get()->tree_get())
			session = g->session_tree_get();
		else
			session = g->session_get();
		os << setw(3) << setfill(' ') << session << "p";
		draw_tag_right_whole(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR);
	}

	if (!lock && g && !g->group_derived_get()->undefined_get()) {
		ostringstream os;
		os << g->group_derived_get()->name_get();
		draw_tag_right(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR_TAG);
	}

	if (!lock && g && !g->type_derived_get()->undefined_get()) {
		ostringstream os;
		os << g->type_derived_get()->name_get();
		draw_tag_right(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR_TAG);
	}

	if (g) {
		ostringstream os;
		if (g->software_get()) {
			string machine = g->root_get().description_tree_get();

			// use some shorcuts for the top bar
			if (machine == "TI99/4A Home Computer")
				machine = "TI99/4A";
			else if (machine == "Nintendo Entertainment System")
				machine = "NES";
			else if (machine == "Super Nintendo Entertainment System")
				machine = "SNES";

			os << machine << " - ";
		}
		if (g->emulator_get()->tree_get())
			os << g->description_tree_get();
		else
			os << g->description_get();

		draw_tag_left(font, os.str(), xl, xr, y, 0, COLOR_MENU_BAR_TAG);

		if (!bottom && g->year_get().length())
			draw_tag_left_whole(font, ", " + g->year_get(), xl, xr, y, 0, COLOR_MENU_BAR_TAG);

		if (xr - xl >= in_separator)
			xl += in_separator;
	}

	if (!lock && g) {
		ostringstream os;
		if (g->size_get() >= 10 * 1000 * 1000)
			os << setw(4) << g->size_get() / 1000 / 1000 << "M";
		else
			os << setw(4) << g->size_get() / 1000 << "k";
		draw_tag_right_whole(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR);
	}

	if (!lock && g && g->sizex_get() && g->sizey_get()) {
		ostringstream os;
		os << g->sizex_get() << "x" << g->sizey_get();
		draw_tag_right_whole(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR);
	}

	if (!lock && g) {
		ostringstream os;
		if (g->emulator_get()->tree_get()) {
			const game* gb = &g->clone_best_get();
			os << gb->name_get();
		} else {
			os << g->name_get();
		}
		draw_tag_left_whole(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR);
	}
}

void draw_menu_info(const game_set& gar, const game* g, int x, int y, int dx, merge_t merge, listpreview_t preview, listsort_t sort_mode, difficulty_t difficulty, bool lock)
{
	font_t font = bar;

	int_clear_alpha(x, y, dx, int_font_dy_get(font), COLOR_MENU_BAR.background);

	int separator = dx > 40 * int_font_dx_get(font) ? 1 * int_font_dx_get(font) : 0 * int_font_dx_get(font);
	int in_separator = dx > 40 * int_font_dx_get(font) ? 2 * int_font_dx_get(font) : 1 * int_font_dx_get(font);
	int xr = dx - separator + x;
	int xl = separator + x;

	if (g) {
		const game* gb;
		if (g->emulator_get()->tree_get())
			gb = &g->clone_best_get();
		else
			gb = g;

		if (!gb->present_tree_get())
			draw_tag_right(font, "MISSING", xl, xr, y, in_separator, COLOR_MENU_BAR_TAG);
		else if (gb->play_get() == play_preliminary)
			draw_tag_right(font, "  ALPHA", xl, xr, y, in_separator, COLOR_MENU_BAR_TAG);
		else
			draw_tag_right(font, "       ", xl, xr, y, in_separator, COLOR_MENU_BAR_TAG);
	}

	if (lock)
		draw_tag_right(font, "locked", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN);

	if (!lock) switch (preview) {
		case preview_flyer: draw_tag_right(font, "flyers", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case preview_cabinet: draw_tag_right(font, "cabinets", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case preview_icon: draw_tag_right(font, "icons", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case preview_marquee: draw_tag_right(font, "marquees", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case preview_title: draw_tag_right(font, "titles", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case preview_snap: draw_tag_right(font, "snap", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		}

	if (!lock) switch (sort_mode) {
		case sort_by_group: draw_tag_right(font, "group", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_name: draw_tag_right(font, "name", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_root_name: draw_tag_right(font, "parent", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_time: draw_tag_right(font, "time", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_smart_time: draw_tag_right(font, "stime", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_session: draw_tag_right(font, "play", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_year: draw_tag_right(font, "year", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_manufacturer: draw_tag_right(font, "manuf", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_type: draw_tag_right(font, "type", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_size: draw_tag_right(font, "size", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_res: draw_tag_right(font, "res", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_info: draw_tag_right(font, "info", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_timepersession: draw_tag_right(font, "timeperplay", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_emulator: draw_tag_right(font, "emulator", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		}

	if (!lock) switch (difficulty) {
		case difficulty_none: draw_tag_right(font, "none", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case difficulty_easiest: draw_tag_right(font, "easiest", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case difficulty_easy: draw_tag_right(font, "easy", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case difficulty_medium: draw_tag_right(font, "normal", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case difficulty_hard: draw_tag_right(font, "hard", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		case difficulty_hardest: draw_tag_right(font, "hardest", xl, xr, y, in_separator, COLOR_MENU_BAR_HIDDEN); break;
		}

	if (g) {
		if (g->info_get().length())
			draw_tag_right(font, g->info_get(), xl, xr, y, 0, COLOR_MENU_BAR_TAG);

		draw_tag_left(font, g->manufacturer_get(), xl, xr, y, 0, COLOR_MENU_BAR);

		if (g->year_get().length())
			draw_tag_left(font, ", " + g->year_get(), xl, xr, y, 0, COLOR_MENU_BAR);

		if (g->clone_get() > 0) {
			ostringstream os;
			os << ", " << g->clone_get() << " clones";
			draw_tag_left(font, os.str(), xl, xr, y, 0, COLOR_MENU_BAR);
		}

		if (!lock && g->parent_get()) {
			ostringstream os;
			os << ", ";
			if (g->software_get())
				os << "software of";
			else
				os << "clone of";
			os << " " << g->parent_get()->name_get();
			draw_tag_left(font, os.str(), xl, xr, y, in_separator, COLOR_MENU_BAR);
		}
	}
}

struct cell_t {
	// position and size
	int x;
	int y;
	int dx;
	int dy;
};

void draw_menu_window(const game_set& gar, const menu_array& gc, struct cell_t* cell, int coln, int rown, int start, int pos, bool use_ident, merge_t merge, bool center)
{
	for (int r = 0; r < rown; ++r) {
		for (int c = 0; c < coln; ++c) {
			if (start < gc.size()) {
				if (gc[start]->has_game()) {
					const game& g = gc[start]->game_get();
					if (center)
						draw_menu_game_center(gar, g, cell->x, cell->y, cell->dx, cell->dy, start == pos, merge);
					else
						draw_menu_game_left(gar, g, cell->x, cell->y, cell->dx, start == pos, merge, use_ident ? gc[start]->ident_get() : 0);
				} else {
					draw_menu_desc(gc[start]->desc_get(), cell->x, cell->y, cell->dx, start == pos);
				}
			} else {
				draw_menu_empty(cell->x, cell->y, cell->dx, cell->dy, start == pos);
			}
			++start;
			++cell;
		}
	}
}

void draw_menu_scroll(int x, int y, int dx, int dy, int pos, int delta, int max)
{
	if (max <= 1)
		return;

	if (pos >= max)
		pos = max - 1;
	if (pos + delta > max)
		delta = max - pos;

	int y0 = pos * (dy - dx - 1) / (max - 1);
	int y1 = dx + (pos + delta - 1) * (dy - dx - 1) / (max - 1);

	int_clear_alpha(x, y, dx, dy, COLOR_MENU_GRID.background);
	int_clear_alpha(x, y + y0, dx, y1 - y0 + 1, COLOR_MENU_GRID.foreground);
}

// ------------------------------------------------------------------------
// Menu utility

bool menu_fast_compare(const string& game, const string& fast)
{
	unsigned igame = 0;
	unsigned ifast = 0;
	while (igame < game.length() && ifast < fast.length()) {
		while (igame < game.length() && !isalnum(game[igame]))
			++igame;
		if (igame < game.length()) {
			if (toupper(game[igame]) != toupper(fast[ifast]))
				return false;
			++igame;
			++ifast;
		}
	}
	return ifast == fast.length();
}

//--------------------------------------------------------------------------
// Backdrop

bool sound_find_preview(resource& path, const config_state& rs, const game* pgame)
{
	(void)rs;
	if (pgame && pgame->preview_find(path, &game::preview_sound_get))
		return true;
	return false;
}

bool backdrop_find_preview_strict(resource& path, listpreview_t preview, const game* pgame, bool only_clip)
{
	if (pgame) {
		switch (preview) {
		case preview_icon:
			if (pgame->preview_find(path, &game::preview_icon_get))
				return true;
			break;
		case preview_marquee:
			if (pgame->preview_find(path, &game::preview_marquee_get))
				return true;
			break;
		case preview_title:
			if (pgame->preview_find(path, &game::preview_title_get))
				return true;
			break;
		case preview_snap:
			if (pgame->preview_find(path, &game::preview_clip_get))
				return true;
			if (!only_clip) {
				if (pgame->preview_find(path, &game::preview_snap_get))
					return true;
			}
			break;
		case preview_flyer:
			if (pgame->preview_find(path, &game::preview_flyer_get))
				return true;
			break;
		case preview_cabinet:
			if (pgame->preview_find(path, &game::preview_cabinet_get))
				return true;
			break;
		}
	}

	return false;
}

bool backdrop_find_preview_default(resource& path, unsigned& aspectx, unsigned& aspecty, listpreview_t preview, const game* pgame, const config_state& rs)
{
	if (backdrop_find_preview_strict(path, preview, pgame, false))
		return true;

	path = resource();

	// reset the aspect, the default preview may be anything
	aspectx = 0;
	aspecty = 0;

	switch (preview) {
	case preview_icon:
		if (rs.preview_default_icon != "none")
			path = rs.preview_default_icon;
		break;
	case preview_marquee:
		if (rs.preview_default_marquee != "none")
			path = rs.preview_default_marquee;
		break;
	case preview_title:
		if (rs.preview_default_title != "none")
			path = rs.preview_default_title;
		break;
	case preview_snap:
		if (rs.preview_default_snap != "none")
			path = rs.preview_default_snap;
		break;
	case preview_flyer:
		if (rs.preview_default_flyer != "none")
			path = rs.preview_default_flyer;
		break;
	case preview_cabinet:
		if (rs.preview_default_cabinet != "none")
			path = rs.preview_default_cabinet;
		break;
	}

	if (!path.is_valid() && rs.preview_default != "none")
		path = rs.preview_default;

	return path.is_valid();
}

void backdrop_game_set(const game* effective_game, unsigned back_pos, listpreview_t preview, bool current, bool highlight, bool clip, config_state& rs)
{
	resource backdrop_res;
	resource clip_res;

	unsigned aspectx;
	unsigned aspecty;
	if (effective_game && (preview == preview_snap || preview == preview_title)) {
		aspectx = effective_game->aspectx_get();
		aspecty = effective_game->aspecty_get();

		if (clip && preview == preview_snap) {
			if (rs.clip_mode != clip_none && effective_game->preview_find(clip_res, &game::preview_clip_get)) {
				int_clip_set(back_pos, clip_res, aspectx, aspecty, current);
			} else {
				int_clip_clear(back_pos);
			}
		} else {
			int_clip_clear(back_pos);
		}
	} else {
		aspectx = 0;
		aspecty = 0;

		int_clip_clear(back_pos);
	}

	if (backdrop_find_preview_default(backdrop_res, aspectx, aspecty, preview, effective_game, rs)) {
		int_backdrop_set(back_pos, backdrop_res, highlight, aspectx, aspecty);
		if (current)
			rs.current_backdrop = backdrop_res;
	} else {
		int_backdrop_clear(back_pos, highlight);
		if (current)
			rs.current_backdrop = resource();
	}
}

void backdrop_index_set(unsigned pos, menu_array& gc, unsigned back_pos, listpreview_t preview, bool current, bool highlight, bool clip, config_state& rs)
{
	const game* effective_game;

	if (pos < gc.size() && gc[pos]->has_game())
		effective_game = &gc[pos]->game_get().clone_best_get();
	else
		effective_game = 0;

	backdrop_game_set(effective_game, back_pos, preview, current, highlight, clip, rs);
}

//--------------------------------------------------------------------------
// Menu run

static void run_background_reset(config_state& rs)
{
	play_background_stop(PLAY_PRIORITY_GAME_BACKGROUND);
	rs.current_sound = resource();
}

static void run_background_set(config_state& rs, const resource& sound)
{
	if (sound.is_valid()) {
		log_std(("menu: play game music '%s'\n", sound.path_get().c_str()));
		play_background_stop(PLAY_PRIORITY_BACKGROUND);
		play_background_effect(sound, PLAY_PRIORITY_GAME_BACKGROUND, false);
		rs.current_sound = sound;
	} else {
		log_std(("menu: no game music found\n"));
	}
}

static void run_background_wait(config_state& rs, const resource& sound, bool silent, int pos_current, int backdrop_max, bool is_idle)
{
	target_clock_t back_start = target_clock();

	// delay before the background music start
	target_clock_t back_stop = back_start + rs.repeat_rep * TARGET_CLOCKS_PER_SEC / 666; // / 666 = * 3 / 2 / 1000

	// short busy wait (int_event_waiting contains a idle call) */
	while (!int_event_waiting() && (target_clock() < back_stop)) {
	}

	// set the backdrop index, with only one, it's always 0
	int backdrop_index = pos_current;
	if (backdrop_max == 1)
		backdrop_index = 0;

	if (!int_event_waiting()) {
		if (!silent) {
			// stop the previous playing
			run_background_reset(rs);

			// start the new game play
			run_background_set(rs, sound);

			// start the new game clip
			int_clip_start(backdrop_index);
		}

		// wait until something pressed
		unsigned loop = 0;
		while (!int_event_waiting()) {

			// if in screensaver play mode, exit when the clip end
			if (loop > 0 && is_idle) {
				break;
			}

			// start some background music if required
			if (!play_background_is_active()) {
				rs.current_sound = resource();
				if (!rs.sound_background.empty()) {
					string path = file_select_random(rs.sound_background);
					log_std(("menu: play background music '%s'\n", path.c_str()));
					play_background_effect(path, PLAY_PRIORITY_BACKGROUND, false);
				} else if (rs.sound_background_loop.length()) {
					log_std(("menu: play background effect '%s'\n", rs.sound_background_loop.c_str()));
					play_background_effect(rs.sound_background_loop, PLAY_PRIORITY_BACKGROUND, true);
				} else {
					log_std(("menu: no background effect\n"));
				}
			}

			// start a new clip if required
			if (rs.clip_mode == clip_singleloop || rs.clip_mode == clip_multiloop) {
				if (!int_clip_is_active(backdrop_index)) {
					int_clip_start(backdrop_index);
				}
			} else if (rs.clip_mode == clip_multiloopall) {
				unsigned i;
				for (i = 0; i < backdrop_max; ++i) {
					if (!int_clip_is_active(i)) {
						int_clip_start(i);
					}
				}
			}

			++loop;
		}
	}
}

static int run_menu_user(config_state& rs, bool flipxy, menu_array& gc, sort_item_func* category_extract, bool silent, string over_msg)
{
	int coln; // number of columns
	int rown; // number of rows

	int scr_x; // useable screen
	int scr_y;
	int scr_dx;
	int scr_dy;

	int win_x; // menu window
	int win_y;
	int win_dx;
	int win_dy;

	int backdrop_x; // backdrop window (if one)
	int backdrop_y;
	int backdrop_dx;
	int backdrop_dy;

	int bar_top_x; // top bar
	int bar_top_y;
	int bar_top_dx;
	int bar_top_dy;

	int bar_bottom_x; // bottom bar
	int bar_bottom_y;
	int bar_bottom_dx;
	int bar_bottom_dy;

	int bar_left_x; // left bar
	int bar_left_y;
	int bar_left_dx;
	int bar_left_dy;

	int bar_right_x; // right bar
	int bar_right_y;
	int bar_right_dx;
	int bar_right_dy;

	int box_x = 0;
	int box_y = 0;
	int box_dx = 0;
	int box_dy = 0;
	adv_bool box;

	int space_x; // space between tiles
	int space_y;

	int name_dy; // vertical space for the name in every cell

	unsigned backdrop_mac; // number of backdrops
	struct cell_t* backdrop_map; // map of the backdrop positions
	struct cell_t* backdrop_map_bis; // alternate map of the backdrop positions

	unsigned game_count; // counter of game in the container

	unsigned ui_right; // user limit
	unsigned ui_left;
	unsigned ui_top;
	unsigned ui_bottom;

	log_std(("menu: user begin\n"));

	// clear all the screen
	int_clear(COLOR_MENU_GRID.background, COLOR_MENU_OVERSCAN.background);

	ui_right = rs.ui_right;
	ui_left = rs.ui_left;
	ui_top = rs.ui_top;
	ui_bottom = rs.ui_bottom;

	// load the background image
	if (rs.ui_back != "none") {
		unsigned scale_x, scale_y;

		if (int_image(rs.ui_back, scale_x, scale_y)) {
			// scale the user limit
			if (scale_x && scale_y) {
				ui_right = rs.ui_right * video_size_x() / scale_x;
				ui_left = rs.ui_left * video_size_x() / scale_x;
				ui_top = rs.ui_top * video_size_y() / scale_y;
				ui_bottom = rs.ui_bottom * video_size_y() / scale_y;
			}
		}
	}

	scr_x = ui_left;
	scr_dx = video_size_x() - ui_right - scr_x;
	scr_y = ui_top;
	scr_dy = video_size_y() - ui_bottom - scr_y;
	if (scr_dx <= 0 || scr_dy <= 0) {
		scr_x = 0;
		scr_y = 0;
		scr_dx = video_size_x();
		scr_dy = video_size_y();
	}

	int_invrotate(scr_x, scr_y, scr_dx, scr_dy);

	// standard bars
	bar_top_x = scr_x;
	bar_top_y = scr_y;
	if (rs.ui_top_bar)
		bar_top_dy = int_font_dy_get(bar);
	else
		bar_top_dy = 0;
	bar_bottom_x = scr_x;
	if (rs.ui_bottom_bar)
		bar_bottom_dy = int_font_dy_get(bar);
	else
		bar_bottom_dy = 0;
	bar_left_x = scr_x;
	bar_left_y = scr_y + bar_top_dy;
	bar_right_y = scr_y + bar_top_dy;

	if (rs.ui_scroll_bar) {
		bar_left_dx = int_font_dx_get(text) / 2;
		bar_right_dx = int_font_dx_get(text) / 2;
	} else {
		bar_left_dx = 0;
		bar_right_dx = 0;
	}

	// effective preview type
	listpreview_t effective_preview;
	switch (rs.mode_get()) {
	case mode_tile_icon: effective_preview = preview_icon; break;
	case mode_tile_marquee: effective_preview = preview_marquee; break;
	default:
		effective_preview = rs.preview_get();
		if (effective_preview == preview_icon || effective_preview == preview_marquee)
			effective_preview = preview_snap;
		// if no image, set another preview
		if ((effective_preview & rs.preview_mask) == 0) {
			if ((preview_snap & rs.preview_mask) != 0)
				effective_preview = preview_snap;
			else if ((preview_title & rs.preview_mask) != 0)
				effective_preview = preview_title;
			else if ((preview_flyer & rs.preview_mask) != 0)
				effective_preview = preview_flyer;
		}
	}

	// cursor
	unsigned cursor_size;
	if (rs.mode_get() != mode_full && rs.mode_get() != mode_list && rs.mode_get() != mode_full_mixed && rs.mode_get() != mode_list_mixed && rs.mode_get() != mode_text) {
		// need a cursor
		cursor_size = video_size_y() / 200 + 1; // size of the flashing cursor
	} else {
		// no cursor
		cursor_size = 0;
	}

	// use identation on the names
	bool use_ident = false;

	if (rs.mode_get() == mode_tile_icon) {
		// icon mode
		unsigned icon_space = rs.icon_space;
		if (icon_space > 64)
			icon_space = 64;
		if (icon_space < 2 * cursor_size + int_font_dy_get(text))
			icon_space = 2 * cursor_size + int_font_dy_get(text);

		space_x = 0;
		space_y = 0;
		name_dy = int_font_dy_get(text);
		if (!flipxy) {
			coln = (int_dx_get() - bar_left_dx - bar_right_dx) / (32 + icon_space);
			rown = (scr_dy - bar_top_dy - bar_bottom_dy) / (32 + icon_space);
		} else {
			coln = (int_dx_get() - bar_left_dx - bar_right_dx) / (32 + icon_space);
			rown = (scr_dy - bar_top_dy - bar_bottom_dy) / (32 + icon_space);
		}

		backdrop_mac = coln * rown;

		win_x = scr_x + bar_left_dx;
		win_y = scr_y + bar_top_dy;

		win_dx = (scr_dx - bar_left_dx - bar_right_dx - (coln - 1) * space_x) / coln * coln + (coln - 1) * space_x;
		win_dy = (scr_dy - bar_top_dy - bar_bottom_dy - (rown - 1) * space_y) / rown * rown + (rown - 1) * space_y;

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;
		box = false;

		bar_right_x = scr_x + bar_left_dx + win_dx;
		bar_bottom_y = scr_y + bar_top_dy + win_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
		bar_left_dy = bar_right_dy = win_dy;
	} else if (rs.mode_get() == mode_tile_marquee) {
		// marquee mode
		space_x = rs.ui_x >= 0 ? rs.ui_x : int_font_dx_get(text) / 2;
		space_y = rs.ui_y >= 0 ? rs.ui_y : int_font_dx_get(text) / 2;

		name_dy = 0;

		if (!flipxy) {
			coln = 3;
			rown = 3 * 4 * 3 / 4;
		} else {
			coln = 2;
			rown = 2 * 4 * 4 / 3;
		}

		backdrop_mac = coln * rown;

		win_x = scr_x + bar_left_dx;
		win_y = scr_y + bar_top_dy;

		win_dx = (scr_dx - bar_left_dx - bar_right_dx - (coln - 1) * space_x) / coln * coln + (coln - 1) * space_x;
		win_dy = (scr_dy - bar_top_dy - bar_bottom_dy - (rown - 1) * space_y) / rown * rown + (rown - 1) * space_y;

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;
		box = false;

		bar_right_x = scr_x + bar_left_dx + win_dx;
		bar_bottom_y = scr_y + bar_top_dy + win_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
		bar_left_dy = bar_right_dy = win_dy;
	} else if (rs.mode_get() == mode_full || rs.mode_get() == mode_full_mixed) {
		// full mode
		if (rs.mode_get() == mode_full_mixed)
			backdrop_mac = 4;
		else
			backdrop_mac = 1;
		name_dy = 0;

		win_x = scr_x + bar_left_dx;
		win_y = scr_y + bar_top_dy;

		space_x = 0;
		space_y = 0;

		coln = 1;
		rown = 1;
		win_dx = 0;
		win_dy = 0;

		backdrop_x = win_x;
		backdrop_y = win_y;
		backdrop_dx = scr_dx - bar_left_dx - bar_right_dx;
		backdrop_dy = scr_dy - bar_top_dy - bar_bottom_dy;
		box = false;

		bar_right_x = scr_x + bar_left_dx + backdrop_dx;
		bar_bottom_y = scr_y + bar_top_dy + backdrop_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + backdrop_dx;
		bar_left_dy = bar_right_dy = backdrop_dy;
	} else if (rs.mode_get() == mode_text) {
		// text mode
		backdrop_mac = 0;

		name_dy = int_font_dy_get(text);

		space_x = rs.ui_x >= 0 ? rs.ui_x : int_font_dx_get(text) / 2;
		space_y = rs.ui_y >= 0 ? rs.ui_y : 0;

		coln = scr_dx / (25 * int_font_dx_get(text));
		if (coln < 2)
			coln = 2;
		rown = (scr_dy - bar_top_dy - bar_bottom_dy) / int_font_dy_get(text);

		win_x = scr_x + bar_left_dx;
		win_y = scr_y + bar_top_dy;
		win_dx = (scr_dx - bar_left_dx - bar_right_dx - (coln - 1) * space_x) / coln * coln + (coln - 1) * space_x;
		win_dy = rown * int_font_dy_get(text);

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;
		box = false;

		bar_right_x = scr_x + bar_left_dx + win_dx;
		bar_bottom_y = scr_y + bar_top_dy + win_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
		bar_left_dy = bar_right_dy = win_dy;
	} else if (rs.mode_get() == mode_list || rs.mode_get() == mode_list_mixed) {
		// list mode
		if (rs.mode_get() == mode_list_mixed)
			backdrop_mac = 4;
		else
			backdrop_mac = 1;
		name_dy = int_font_dy_get(text);

		win_x = scr_x + bar_left_dx;
		win_y = scr_y + bar_top_dy;

		unsigned multiplier;
		unsigned divisor;
		if (rs.mode_get() == mode_list) {
			multiplier = 1;
			divisor = 3;
		} else {
			multiplier = 1;
			divisor = 4;
		}

		if (!flipxy) {
			// horizontal
			use_ident = true;

			space_x = 0;
			space_y = 0;

			coln = 1;
			win_dx = (scr_dx - bar_left_dx - bar_right_dx) * multiplier / divisor;
			win_dx -= win_dx % int_font_dx_get(text);

			rown = (scr_dy - bar_top_dy - bar_bottom_dy) / int_font_dy_get(text);
			win_dy = rown * int_font_dy_get(text);

			backdrop_x = win_x + win_dx;
			backdrop_y = win_y;
			backdrop_dx = scr_dx - win_dx - bar_left_dx - bar_right_dx;
			backdrop_dy = win_dy;
			box = false;

			bar_right_x = scr_x + bar_left_dx + win_dx + backdrop_dx;
			bar_bottom_y = scr_y + bar_top_dy + win_dy;
			bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx + backdrop_dx;
			bar_left_dy = bar_right_dy = win_dy;
		} else {
			// vertical
			space_x = rs.ui_x >= 0 ? rs.ui_x : int_font_dx_get(text) / 2;
			space_y = rs.ui_y >= 0 ? rs.ui_y : 0;

			coln = scr_dx / (20 * int_font_dx_get(text));
			if (coln < 2)
				coln = 2;
			win_dx = (scr_dx - bar_left_dx - bar_right_dx - (coln - 1) * space_x) / coln * coln + (coln - 1) * space_x;

			rown = (scr_dy - bar_top_dy - bar_bottom_dy) * multiplier / (divisor * int_font_dy_get(text));
			win_dy = rown * int_font_dy_get(text);

			backdrop_x = win_x;
			backdrop_y = win_y + win_dy;
			backdrop_dx = win_dx;
			backdrop_dy = scr_dy - win_dy - bar_top_dy - bar_bottom_dy;
			box = false;

			bar_right_x = scr_x + bar_left_dx + win_dx;
			bar_bottom_y = scr_y + bar_top_dy + win_dy + backdrop_dy;
			bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
			bar_left_dy = bar_right_dy = win_dy + backdrop_dy;
		}
	} else {
		// tile modes
		space_x = rs.ui_x >= 0 ? rs.ui_x : int_font_dx_get(text) / 2;
		space_y = rs.ui_y >= 0 ? rs.ui_y : int_font_dx_get(text) / 2;

		if (!flipxy) {
			switch (rs.mode_get()) {
			default: /* for warnings */
			case mode_tile_giant:
				coln = 16;
				rown = 12;
				name_dy = 0;
				space_x = 0;
				space_y = 0;
				bar_left_dx = 0;
				bar_right_dx = 0;
				bar_top_dy = 0;
				bar_bottom_dy = 0;
				break;
			case mode_tile_enormous:
				coln = 12;
				rown = 9;
				name_dy = 0;
				space_x = 0;
				space_y = 0;
				bar_left_dx = 0;
				bar_right_dx = 0;
				bar_top_dy = 0;
				bar_bottom_dy = 0;
				break;
			case mode_tile_big:
				coln = 8;
				rown = 6;
				name_dy = 0;
				space_x = 0;
				space_y = 0;
				break;
			case mode_tile_normal:
				coln = 5;
				rown = 4;
				name_dy = int_font_dy_get(text);
				break;
			case mode_tile_small:
				coln = 4;
				rown = 3;
				name_dy = int_font_dy_get(text);
				break;
			case mode_tile_tiny:
				coln = 3;
				rown = 2;
				name_dy = int_font_dy_get(text);
				break;
			}
		} else {
			switch (rs.mode_get()) {
			default: /* for warnings */
			case mode_tile_giant:
				coln = 12;
				rown = 16;
				name_dy = 0;
				space_x = 0;
				space_y = 0;
				bar_left_dx = 0;
				bar_right_dx = 0;
				bar_top_dy = 0;
				bar_bottom_dy = 0;
				break;
			case mode_tile_enormous:
				coln = 9;
				rown = 12;
				name_dy = 0;
				space_x = 0;
				space_y = 0;
				bar_left_dx = 0;
				bar_right_dx = 0;
				bar_top_dy = 0;
				bar_bottom_dy = 0;
				break;
			case mode_tile_big:
				coln = 6;
				rown = 8;
				name_dy = 0;
				space_x = 0;
				space_y = 0;
				break;
			case mode_tile_normal:
				coln = 4;
				rown = 5;
				name_dy = int_font_dy_get(text);
				break;
			case mode_tile_small:
				coln = 3;
				rown = 4;
				name_dy = int_font_dy_get(text);
				break;
			case mode_tile_tiny:
				coln = 2;
				rown = 3;
				name_dy = int_font_dy_get(text);
				break;
			}
		}

		backdrop_mac = coln * rown;

		box = name_dy == 0;
		if (box) {
			box_x = 1;
			box_y = 1;
			box_dx = 2;
			box_dy = 2;
		} else {
			box_x = 0;
			box_y = 0;
			box_dx = 0;
			box_dy = 0;
		}

		win_x = scr_x + bar_left_dx + box_x;
		win_y = scr_y + bar_top_dy + box_y;
		win_dx = (scr_dx - bar_left_dx - bar_right_dx - box_dx - (coln - 1) * space_x) / coln * coln + (coln - 1) * space_x;
		win_dy = (scr_dy - bar_top_dy - bar_bottom_dy - box_dy - (rown - 1) * space_y) / rown * rown + (rown - 1) * space_y;

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;

		bar_right_x = scr_x + bar_left_dx + win_dx + box_dx;
		bar_bottom_y = scr_y + bar_top_dy + win_dy + box_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx + box_dx;
		bar_left_dy = bar_right_dy = win_dy + box_dy;

		if (box) {
			box_x = win_x - 1;
			box_y = win_y - 1;
			box_dx = win_dx + 2;
			box_dy = win_dy + 2;
		}
	}

	backdrop_map = new cell_t[backdrop_mac];
	backdrop_map_bis = new cell_t[backdrop_mac];

	// text position
	struct cell_t* int_map = new cell_t[coln * rown];

	// size of the text cell
	int cell_dx = (win_dx - space_x * (coln - 1)) / coln;
	int cell_dy = (win_dy - space_y * (rown - 1)) / rown;

	if (rs.mode_get() == mode_full_mixed || rs.mode_get() == mode_list_mixed) {
		int bar_dx; // size of the additional small images
		int bar_dy;
		int aspect_x;
		int aspect_y;

		if (rs.mode_get() == mode_list_mixed) {
			if (!flipxy) {
				aspect_x = 4 * 4 * video_size_y();
				aspect_y = 3 * 3 * video_size_x();
			} else {
				aspect_x = video_size_y();
				aspect_y = video_size_x();
			}
		} else {
			aspect_x = 4 * video_size_y();
			aspect_y = 3 * video_size_x();
		}

		box = true;
		box_x = backdrop_x;
		box_y = backdrop_y;
		box_dx = backdrop_dx;
		box_dy = backdrop_dy;
		backdrop_x += 1;
		backdrop_y += 1;
		backdrop_dx -= 2;
		backdrop_dy -= 2;

		// game vertical
		bar_dx = backdrop_dx - backdrop_dy * aspect_y / aspect_x;
		if (bar_dx < backdrop_dx / 5)
			bar_dx = backdrop_dx / 5;
		bar_dy = backdrop_dy / 3;
		backdrop_map[0].x = backdrop_x;
		backdrop_map[0].y = backdrop_y;
		backdrop_map[0].dx = backdrop_dx - bar_dx;
		backdrop_map[0].dy = backdrop_dy;
		backdrop_map[1].x = backdrop_x + backdrop_dx - bar_dx;
		backdrop_map[1].y = backdrop_y;
		backdrop_map[1].dx = bar_dx;
		backdrop_map[1].dy = bar_dy;
		backdrop_map[2].x = backdrop_x + backdrop_dx - bar_dx;
		backdrop_map[2].y = backdrop_y + bar_dy;
		backdrop_map[2].dx = bar_dx;
		backdrop_map[2].dy = bar_dy;
		backdrop_map[3].x = backdrop_x + backdrop_dx - bar_dx;
		backdrop_map[3].y = backdrop_y + 2 * bar_dy;
		backdrop_map[3].dx = bar_dx;
		backdrop_map[3].dy = backdrop_dy - 2 * bar_dy;

		// game horizontal
		bar_dx = backdrop_dx / 3;
		bar_dy = backdrop_dy - backdrop_dx * aspect_y / aspect_x;
		if (bar_dy < backdrop_dy / 5)
			bar_dy = backdrop_dy / 5;
		backdrop_map_bis[0].x = backdrop_x;
		backdrop_map_bis[0].y = backdrop_y + bar_dy;
		backdrop_map_bis[0].dx = backdrop_dx;
		backdrop_map_bis[0].dy = backdrop_dy - bar_dy;
		backdrop_map_bis[1].x = backdrop_x;
		backdrop_map_bis[1].y = backdrop_y;
		backdrop_map_bis[1].dx = bar_dx;
		backdrop_map_bis[1].dy = bar_dy;
		backdrop_map_bis[2].x = backdrop_x + bar_dx;
		backdrop_map_bis[2].y = backdrop_y;
		backdrop_map_bis[2].dx = bar_dx;
		backdrop_map_bis[2].dy = bar_dy;
		backdrop_map_bis[3].x = backdrop_x + 2 * bar_dx;
		backdrop_map_bis[3].y = backdrop_y;
		backdrop_map_bis[3].dx = backdrop_dx - 2 * bar_dx;
		backdrop_map_bis[3].dy = bar_dy;
	}

	if (rs.mode_get() == mode_full_mixed || rs.mode_get() == mode_full) {
		int_map[0].x = 0;
		int_map[0].y = 0;
		int_map[0].dx = 0;
		int_map[0].dy = 0;
	} else if (rs.mode_get() == mode_list || rs.mode_get() == mode_list_mixed || rs.mode_get() == mode_text) {
		for (int r = 0; r < rown; ++r) {
			for (int c = 0; c < coln; ++c) {
				unsigned i = r * coln + c;
				int x = win_x + (cell_dx + space_x) * c;
				int y = win_y + (cell_dy + space_y) * r;
				int_map[i].x = x;
				int_map[i].y = y + cell_dy - name_dy;
				int_map[i].dx = cell_dx;
				int_map[i].dy = name_dy;
			}
		}
	} else {
		for (int r = 0; r < rown; ++r) {
			for (int c = 0; c < coln; ++c) {
				unsigned i = r * coln + c;
				int x = win_x + (cell_dx + space_x) * c;
				int y = win_y + (cell_dy + space_y) * r;
				if (rs.mode_get() == mode_tile_icon) {
					backdrop_map[i].dx = 32 + 2 * cursor_size;
					backdrop_map[i].dy = 32 + 2 * cursor_size;
					int name_row = 3;
					int space_up;
					do {
						--name_row;
						space_up = (cell_dy - backdrop_map[i].dy - name_dy * name_row) / 3;
					} while (space_up < 0);
					backdrop_map[i].x = x + (cell_dx - 32 - 2 * cursor_size) / 2;
					backdrop_map[i].y = y + space_up;
					int_map[i].x = x;
					int_map[i].y = y + backdrop_map[i].dy + 2 * space_up;
					int_map[i].dx = cell_dx;
					int_map[i].dy = name_row * name_dy;
				} else {
					backdrop_map[i].x = x;
					int_map[i].x = x;
					if (rs.ui_top_name) {
						backdrop_map[i].y = y + name_dy;
						int_map[i].y = y;
					} else {
						backdrop_map[i].y = y;
						int_map[i].y = y + cell_dy - name_dy;
					}
					backdrop_map[i].dx = cell_dx;
					backdrop_map[i].dy = cell_dy - name_dy;
					int_map[i].dx = cell_dx;
					int_map[i].dy = name_dy;
				}
			}
		}
	}

	if (backdrop_mac == 1) {
		int_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, 1, 0, 1, cursor_size, rs.preview_expand, false, rs.resizeeffect);
		int_backdrop_pos(0, backdrop_x, backdrop_y, backdrop_dx, backdrop_dy);
	} else if (backdrop_mac > 1) {
		if (rs.mode_get() == mode_tile_icon)
			int_backdrop_init(COLOR_MENU_ICON, COLOR_MENU_CURSOR, backdrop_mac, coln, cursor_size, cursor_size, rs.preview_expand, false, rs.resizeeffect);
		else if (rs.mode_get() == mode_list_mixed || rs.mode_get() == mode_full_mixed)
			int_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, backdrop_mac, 1, 0, cursor_size, rs.preview_expand, false, rs.resizeeffect);
		else {
			int outline;
			if (space_x == 0 || !rs.ui_outline)
				outline = 0;
			else
				outline = 1;
			int_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, backdrop_mac, coln, outline, cursor_size, rs.preview_expand, rs.clip_mode == clip_multi || rs.clip_mode == clip_multiloop || rs.clip_mode == clip_multiloopall, rs.resizeeffect);
		}
		for (int i = 0; i < backdrop_mac; ++i)
			int_backdrop_pos(i, backdrop_map[i].x, backdrop_map[i].y, backdrop_map[i].dx, backdrop_map[i].dy);
	}

	// reset the sound
	rs.current_sound = resource();

	int pos_rel_max = coln * rown;
	int pos_base_upper = gc.size();
	if (pos_base_upper % coln)
		pos_base_upper = pos_base_upper + coln - pos_base_upper % coln;
	pos_base_upper -= pos_rel_max;
	if (pos_base_upper < 0)
		pos_base_upper = 0;

	// restore the old position
	int pos_base;
	int pos_rel;

	if (rs.current_game) {
		// if a game is selected search the same game
		int i;
		i = 0;

		log_std(("menu: search for game %s\n", rs.current_game->name_get().c_str()));

		while (i < gc.size() && (!gc[i]->has_game() || &gc[i]->game_get() != rs.current_game))
			++i;

		if (i < gc.size()) {
			pos_base = rs.menu_base_effective;
			pos_rel = i - pos_base;
		} else {
			pos_base = rs.menu_base_effective;
			pos_rel = rs.menu_rel_effective;
		}
	} else {
		// otherwise use the old position
		pos_base = rs.menu_base_effective;
		pos_rel = rs.menu_rel_effective;
	}

	// ensure that the position is valid
	if (pos_base + pos_rel >= gc.size()) {
		pos_base = pos_base_upper;
		pos_rel = pos_rel_max - 1;
		if (pos_base + pos_rel >= gc.size()) {
			pos_base = pos_base_upper;
			if (gc.size() > pos_base_upper)
				pos_rel = gc.size() - pos_base_upper - 1;
			else
				pos_rel = 0;
		}
	} else {
		if (pos_base % coln) {
			unsigned rest = pos_base % coln;
			pos_base -= rest;
			pos_rel += rest;
		}
		while (pos_rel < 0) {
			pos_base -= coln;
			pos_rel += coln;
		}
		while (pos_rel >= pos_rel_max) {
			pos_base += coln;
			pos_rel -= coln;
		}
		while (pos_base > pos_base_upper && pos_rel + coln < pos_rel_max) {
			pos_base -= coln;
			pos_rel += coln;
		}
		if (pos_base > pos_base_upper)
			pos_base = pos_base_upper;
	}

	if (pos_base + pos_rel < gc.size() && gc[pos_base + pos_rel]->has_game()) {
		rs.current_game = &gc[pos_base + pos_rel]->game_get();
		rs.current_clone = 0;
	} else {
		rs.current_game = 0;
		rs.current_clone = 0;
	}

	// count the real games
	{
		int i;
		game_count = 0;
		for (i = 0; i < gc.size(); ++i)
			if (gc[i]->has_game())
				++game_count;
	}

	bool done = false;
	int key = 0;

	// clear the used part
	int_clear_alpha(scr_x, scr_y, bar_left_dx + win_dx + bar_right_dx, bar_top_dy + win_dy + bar_bottom_dy, COLOR_MENU_GRID.background);

	log_std(("menu: user end\n"));

	while (!done) {
		const game* effective_game = pos_base + pos_rel < gc.size() && gc[pos_base + pos_rel]->has_game() ? &gc[pos_base + pos_rel]->game_get() : 0;

		resource sound;
		if (!sound_find_preview(sound, rs, effective_game)) {
			sound = resource();
		}

#if 0
		/* use the preview as background */
		if (rs.mode_get() == mode_text) {
			resource snap;
			unsigned aspectx, aspecty;

			switch (effective_preview) {
			case preview_cabinet:
			case preview_snap:
			case preview_title:
			case preview_flyer:
				int_clear(COLOR_MENU_GRID.background);
				if (backdrop_find_preview_default(snap, aspectx, aspecty, effective_preview, effective_game, rs)) {
					unsigned scale_x, scale_y;
					int_image(snap.path_get(), scale_x, scale_y);
				}
				int_clear_alpha(scr_x, scr_y, scr_dx, scr_dy, COLOR_MENU_GRID.background);
				break;
			default:
				break;
			}
		}
#endif

		if (name_dy)
			draw_menu_window(rs.gar, gc, int_map, coln, rown, pos_base, pos_base + pos_rel, use_ident, rs.merge, rs.mode_get() == mode_tile_icon);
		if (bar_top_dy)
			draw_menu_bar(rs.current_game, game_count, bar_top_x, bar_top_y, bar_top_dx, rs.lock_effective, bar_bottom_dy != 0);
		if (bar_bottom_dy)
			draw_menu_info(rs.gar, rs.current_game, bar_bottom_x, bar_bottom_y, bar_bottom_dx, rs.merge, effective_preview, rs.sort_get(), rs.difficulty_effective, rs.lock_effective);
		if (bar_right_dx) {
			if (rs.ui_scroll_bar)
				draw_menu_scroll(bar_right_x, bar_right_y, bar_right_dx, bar_right_dy, pos_base, pos_rel_max, pos_base_upper + pos_rel_max);
			else
				int_clear_alpha(bar_right_x, bar_right_y, bar_right_dx, bar_right_dy, COLOR_MENU_BAR.background);
		}
		if (bar_left_dx)
			int_clear_alpha(bar_left_x, bar_left_y, bar_left_dx, bar_left_dy, COLOR_MENU_BAR.background);

		if (rs.mode_get() == mode_full_mixed || rs.mode_get() == mode_list_mixed) {
			bool game_horz = true;

			if (rs.mode_get() == mode_list_mixed && (effective_preview == preview_snap || effective_preview == preview_title)) {
				if (effective_game) {
					if (effective_game->aspectx_get() && effective_game->aspecty_get()) {
						game_horz = effective_game->aspectx_get() > effective_game->aspecty_get();
					} else if (effective_game->sizex_get() && effective_game->sizey_get()) {
						game_horz = effective_game->sizex_get() > effective_game->sizey_get();
					} else {
						game_horz = true;
					}
				}
			} else {
				game_horz = backdrop_dx < backdrop_dy;
			}

			if (!game_horz) {
				for (int i = 0; i < backdrop_mac; ++i)
					int_backdrop_pos(i, backdrop_map[i].x, backdrop_map[i].y, backdrop_map[i].dx, backdrop_map[i].dy);
			} else {
				for (int i = 0; i < backdrop_mac; ++i)
					int_backdrop_pos(i, backdrop_map_bis[i].x, backdrop_map_bis[i].y, backdrop_map_bis[i].dx, backdrop_map_bis[i].dy);
			}

			if (effective_preview == preview_title) {
				backdrop_game_set(effective_game, 0, preview_title, true, false, false, rs);
				backdrop_game_set(effective_game, 1, preview_snap, false, false, true, rs);
				backdrop_game_set(effective_game, 2, preview_flyer, false, false, false, rs);
				backdrop_game_set(effective_game, 3, preview_cabinet, false, false, false, rs);
			} else if (effective_preview == preview_flyer) {
				backdrop_game_set(effective_game, 0, preview_flyer, true, false, false, rs);
				backdrop_game_set(effective_game, 1, preview_snap, false, false, true, rs);
				backdrop_game_set(effective_game, 2, preview_title, false, false, false, rs);
				backdrop_game_set(effective_game, 3, preview_cabinet, false, false, false, rs);
			} else if (effective_preview == preview_cabinet) {
				backdrop_game_set(effective_game, 0, preview_cabinet, true, false, false, rs);
				backdrop_game_set(effective_game, 1, preview_snap, false, false, true, rs);
				backdrop_game_set(effective_game, 2, preview_title, false, false, false, rs);
				backdrop_game_set(effective_game, 3, preview_flyer, false, false, false, rs);
			} else {
				backdrop_game_set(effective_game, 0, preview_snap, true, false, true, rs);
				backdrop_game_set(effective_game, 1, preview_title, false, false, false, rs);
				backdrop_game_set(effective_game, 2, preview_flyer, false, false, false, rs);
				backdrop_game_set(effective_game, 3, preview_cabinet, false, false, false, rs);
			}
		} else {
			if (backdrop_mac == 1) {
				backdrop_game_set(effective_game, 0, effective_preview, true, false, true, rs);
			} else if (backdrop_mac > 1) {
				if (rs.clip_mode == clip_multi || rs.clip_mode == clip_multiloop || rs.clip_mode == clip_multiloopall) {
					// put all the clip in the internal cache
					for (int i = 0; i < coln * rown; ++i) {
						int_clip_clear(i);
					}
				}
				for (int i = 0; i < coln * rown; ++i) {
					bool current = i == pos_rel;
					if (rs.clip_mode == clip_multi || rs.clip_mode == clip_multiloop || rs.clip_mode == clip_multiloopall)
						backdrop_index_set(pos_base + i, gc, i, effective_preview, current, current, true, rs);
					else
						backdrop_index_set(pos_base + i, gc, i, effective_preview, current, current, current, rs);
				}
			}
		}
		if (box)
			int_box(box_x, box_y, box_dx, box_dy, 1, COLOR_MENU_BACKDROP.foreground);

		if (over_msg.length()) {
			unsigned dx, dy;
			int x = int_dx_get() / 2;
			int y = int_dy_get() / 2;
			int border = int_font_dx_get(bar) / 2;

			// force an update to draw the first time the backdrop images
			int_update(false);

			dx = int_font_dx_get(text, over_msg);
			dy = int_font_dy_get(text);

			int_box(x - 2 * border - dx / 2, y - border, dx + 4 * border, dy + border * 2, 1, COLOR_CHOICE_NORMAL.foreground);
			int_clear(x - 2 * border - dx / 2 + 1, y - border + 1, dx + 4 * border - 2, dy + border * 2 - 2, COLOR_CHOICE_NORMAL.background);
			int_put(text, x - dx / 2, y, dx, over_msg, COLOR_CHOICE_TITLE);
		}

		int_update(rs.mode_get() != mode_full_mixed && rs.mode_get() != mode_list_mixed);

		log_std(("menu: wait begin\n"));

		int_idle_0_enable(rs.current_game && rs.current_game->emulator_get()->is_runnable());
		int_idle_1_enable(true);

		run_background_wait(rs, sound, silent, pos_rel, backdrop_mac, false);

		// replay the sound and clip
		silent = false;

		log_std(("menu: wait end\n"));

		key = int_event_get(false);

		log_std(("menu: key %d\n", key));

		string oldfast = rs.fast;
		rs.fast.erase();

		key = menu_key(key, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, gc.size());

		switch (key) {
		case EVENT_INS:
			if (pos_base + pos_rel < gc.size() && pos_base + pos_rel > 0) {
				unsigned new_pos = pos_base + pos_rel - 1;
				string i = gc[new_pos]->category(category_extract);
				while (new_pos > 0 && gc[new_pos - 1]->category(category_extract) == i)
					--new_pos;
				menu_pos(new_pos, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, gc.size());
			}
			break;
		case EVENT_DEL:
			if (pos_base + pos_rel < gc.size()) {
				unsigned new_pos = pos_base + pos_rel;
				string i = gc[new_pos]->category(category_extract);
				while (new_pos < gc.size() - 1 && gc[new_pos]->category(category_extract) == i)
					++new_pos;
				menu_pos(new_pos, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, gc.size());
			}
			break;
		default:
			if (key > 32 && key < 128 && isalnum(key)) {
				oldfast.insert(oldfast.length(), 1, (char)key);
				menu_array::const_iterator i;
				for (i = gc.begin(); i != gc.end(); ++i) {
					if (menu_fast_compare((*i)->desc_get(), oldfast)) {
						break;
					}
				}
				if (i == gc.end()) {
					for (i = gc.begin(); i != gc.end(); ++i) {
						if ((*i)->has_game()) {
							const game& g = (*i)->game_get().clone_best_get();
							if (menu_fast_compare(g.name_without_emulator_get(), oldfast)) {
								break;
							}
						}
					}
				}
				if (i != gc.end()) {
					int pos = i - gc.begin();
					menu_pos(pos, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, gc.size());
					rs.fast = oldfast;
				}
			}
			break;
		case EVENT_ENTER:
		case EVENT_CLONE:
		case EVENT_LOCK:
			done = true;
			break;
		case EVENT_IDLE_0:
		case EVENT_IDLE_1:
		case EVENT_CALIBRATION:
			done = true;
			break;
		}
		if (!rs.lock_effective) {
			switch (key) {
			case EVENT_MODE:
			case EVENT_HELP:
			case EVENT_GROUP:
			case EVENT_TYPE:
			case EVENT_ATTRIB:
			case EVENT_SORT:
			case EVENT_SETGROUP:
			case EVENT_SETTYPE:
			case EVENT_COMMAND:
			case EVENT_VOLUME:
			case EVENT_DIFFICULTY:
			case EVENT_MENU:
			case EVENT_EMU:
			case EVENT_ROTATE:
			case EVENT_PREVIEW:
				done = true;
				break;
			}
		}

		// allow exit/shutdown in locked mode if the exit mode allows it
		switch (key) {
		case EVENT_ESC:
			if (rs.exit_mode == exit_esc || rs.exit_mode == exit_all || rs.exit_mode == exit_menu)
				done = true;
			break;
		case EVENT_EXIT:
			if (rs.exit_mode == exit_exit || rs.exit_mode == exit_all || rs.exit_mode == exit_menu)
				done = true;
			break;
		case EVENT_OFF:
			if (rs.exit_mode == exit_shutdown || rs.exit_mode == exit_all || rs.exit_mode == exit_menu)
				done = true;
			break;
		}

		if (pos_rel + pos_base < gc.size() && gc[pos_rel + pos_base]->has_game()) {
			rs.current_game = &gc[pos_rel + pos_base]->game_get();
			rs.current_clone = 0;
		} else {
			rs.current_game = 0;
			rs.current_clone = 0;
		}
	}

	if (key == EVENT_IDLE_0) {
		if (gc.size() > 0) {
			unsigned pos = rand() % gc.size();
			while (pos < gc.size() && !gc[pos]->has_game())
				++pos;
			if (pos < gc.size())
				rs.current_game = &gc[pos]->game_get();
		}
	}

	rs.menu_base_effective = pos_base;
	rs.menu_rel_effective = pos_rel;

	delete [] int_map;
	delete [] backdrop_map;
	delete [] backdrop_map_bis;

	if (backdrop_mac > 0) {
		int_backdrop_done();
	}

	return key;
}

int run_menu_idle(config_state& rs, menu_array& gc)
{
	bool done = false;
	int key = 0;
	unsigned counter = 0;
	listpreview_t preview = preview_snap;

	switch (rs.idle_saver_type) {
	case saver_snap: preview = preview_snap; break;
	case saver_play: preview = preview_snap; break;
	case saver_flyer: preview = preview_flyer; break;
	case saver_cabinet: preview = preview_cabinet; break;
	case saver_title: preview = preview_title; break;
	default:
		assert(0);
		break;
	}

	int_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, 1, 0, 0, 0, rs.preview_expand, false, rs.resizeeffect);

	int_backdrop_pos(0, 0, 0, int_dx_get(), int_dy_get());

	// make the set of available snapshot
	vector<unsigned> avail;
	for (unsigned i = 0; i < gc.size(); ++i) {
		resource backdrop;
		if (gc[i]->has_game()) {
			if (backdrop_find_preview_strict(backdrop, preview, &gc[i]->game_get(), rs.idle_saver_type == saver_play)) {
				avail.insert(avail.end(), i);
			}
		}
	}

	// if no play found, retry with snap
	if (avail.size() == 0 && rs.idle_saver_type == saver_play) {
		for (unsigned i = 0; i < gc.size(); ++i) {
			resource backdrop;
			if (gc[i]->has_game()) {
				if (backdrop_find_preview_strict(backdrop, preview, &gc[i]->game_get(), false)) {
					avail.insert(avail.end(), i);
				}
			}
		}
	}

	// randomize the set
	for (unsigned i = 0; i < avail.size(); ++i) {
		unsigned j = rand() % avail.size();
		unsigned t = avail[i];
		avail[i] = avail[j];
		avail[j] = t;
	}

	int_idle_0_enable(false);
	int_idle_1_enable(true);

	while (!done) {
		unsigned pos;
		bool found;

		// replug all joysticks (in case joystick removal is not detected)
		int_joystick_replug();

		if (avail.size() > 0) {
			pos = avail[counter % avail.size()];
			found = true;
		} else {
			pos = 0;
			found = false;
		}

		resource sound;
		if (found) {
			backdrop_index_set(pos, gc, 0, preview, false, false, true, rs);

			if (rs.idle_saver_type == saver_play) {
				if (!sound_find_preview(sound, rs, &gc[pos]->game_get())) {
					sound = resource();
				}
			}

			int_update_pre();

			if (rs.ui_gamename) {
				ostringstream os;
				const game& g = gc[pos]->game_get();
				os << g.description_tree_get();
				if (g.manufacturer_get().length())
					os << ", " << g.manufacturer_get();
				if (g.year_get().length())
					os << ", " << g.year_get();
				int_set_overlay(os.str(), counter);
			}

			int_update_post();
		} else {
			backdrop_game_set(0, 0, preview, false, false, true, rs);

			int_clear_overlay();

			int_update();
		}

		// next game
		++counter;

		run_background_wait(rs, sound, false, 0, 1, rs.idle_saver_type == saver_play);

		key = int_event_get(false);

		if (key != EVENT_IDLE_1) {

			done = true;

			// select the game if the user press enter
			if (found && key == EVENT_ENTER) {
				rs.current_game = &gc[pos]->game_get();
			}
		}
	}

	int_clear_overlay();

	int_backdrop_done();

	return key;
}

int run_menu_idle_off()
{
	bool done = false;
	int key = 0;

	int_clear(COLOR_MENU_GRID.background, COLOR_MENU_OVERSCAN.background);

	target_apm_standby();

	while (!done) {
		// replug all joysticks (in case joystick removal is not detected)
		int_joystick_replug();

		int_update();

		key = int_event_get(false);

		if (key != EVENT_IDLE_1)
			done = true;
	}

	target_apm_wakeup();

	return key;
}

int run_menu_sort(config_state& rs, const pgame_sort_set& gss, sort_item_func* category_func, bool flipxy, bool silent, string over_msg)
{
	menu_array gc;

	log_std(("menu: insert begin\n"));

	bool list_mode = rs.mode_get() == mode_list || rs.mode_get() == mode_list_mixed;
	if (!list_mode || rs.sort_get() == sort_by_name || rs.sort_get() == sort_by_time || rs.sort_get() == sort_by_smart_time || rs.sort_get() == sort_by_size || rs.sort_get() == sort_by_session || rs.sort_get() == sort_by_timepersession) {
		gc.reserve(gss.size());
		for (pgame_sort_set::const_iterator i = gss.begin(); i != gss.end(); ++i) {
			gc.insert(gc.end(), new menu_entry(*i, 0));
		}
	} else if (rs.sort_get() == sort_by_emulator) {
		string category = "dummy";
		gc.reserve(gss.size() + 16);
		for (pgame_sort_set::const_iterator i = gss.begin(); i != gss.end(); ++i) {
			string new_category = category_func(**i);
			if (new_category != category) {
				category = new_category;
				gc.insert(gc.end(), new menu_entry(category));
			}
			unsigned ident = 1;
			if ((*i)->parent_get()) {
				if ((*i)->software_get())
					ident += 2;
				else
					ident += 1;
			}
			gc.insert(gc.end(), new menu_entry(*i, ident * int_font_dx_get(text)));
		}
	} else if (rs.sort_get() == sort_by_root_name) {
		gc.reserve(gss.size());
		for (pgame_sort_set::const_iterator i = gss.begin(); i != gss.end(); ++i) {
			unsigned ident = 0;
			if ((*i)->parent_get()) {
				if ((*i)->software_get())
					ident += 2;
				else
					ident += 1;
			}
			gc.insert(gc.end(), new menu_entry(*i, ident * int_font_dx_get(text)));
		}
	} else {
		string category = "dummy";
		gc.reserve(gss.size() + 256);
		for (pgame_sort_set::const_iterator i = gss.begin(); i != gss.end(); ++i) {
			string new_category = category_func(**i);
			if (new_category != category) {
				category = new_category;
				gc.insert(gc.end(), new menu_entry(category));
			}
			gc.insert(gc.end(), new menu_entry(*i, int_font_dx_get(text)));
		}
	}

	if (gc.empty()) {
		gc.insert(gc.end(), new menu_entry("<empty>"));
	}

	log_std(("menu: insert end\n"));

	bool done = false;
	int key = 0;
	bool idle = false;

	while (!done) {
		if (idle) {
			if (rs.idle_saver_type == saver_exit) {
				key = EVENT_EXIT_FORCE;
				break;
			}
			if (rs.idle_saver_type == saver_shutdown) {
				key = EVENT_OFF_FORCE;
				break;
			}
			if (rs.restore == restore_idle)
				rs.restore_load();
			if (rs.idle_saver_type == saver_off)
				run_menu_idle_off();
			else {
				key = run_menu_idle(rs, gc);
				if (key == EVENT_ENTER)
					done = true;
			}
			idle = false;
		}

		if (!done) {
			key = run_menu_user(rs, flipxy, gc, category_func, silent, over_msg);

			// replay the sound and clip
			silent = false;

			switch (key) {
			case EVENT_IDLE_1:
				idle = true;
				break;
			default:
				done = true;
			}
		}
	}

	for (menu_array::iterator i = gc.begin(); i != gc.end(); ++i)
		delete *i;

	return key;
}

// ------------------------------------------------------------------------
// Run Info

#define RUNINFO_CHOICE_Y int_dy_get() / 10
#define RUNINFO_CHOICE_DY 2 * int_font_dy_get(text)

void run_runinfo(config_state& rs)
{
	int x = int_dx_get() / 2;
	int y = RUNINFO_CHOICE_Y;
	int dy = RUNINFO_CHOICE_DY;
	int border = int_font_dx_get(bar) / 2;

	const game* g = rs.current_clone ? rs.current_clone : rs.current_game;
	if (!g)
		return;

	if (rs.ui_gamesaver != saver_off) {
		listpreview_t preview = preview_snap;
		switch (rs.ui_gamesaver) {
		case saver_flyer: preview = preview_flyer; break;
		case saver_cabinet: preview = preview_cabinet; break;
		case saver_title: preview = preview_title; break;
		default: preview = preview_snap; break;
		}

		resource backdrop;
		if (backdrop_find_preview_strict(backdrop, preview, g, false)) {
			int_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, 1, 0, 0, 0, rs.preview_expand, false, rs.resizeeffect);
			int_backdrop_pos(0, 0, 0, int_dx_get(), int_dy_get());
			backdrop_game_set(g, 0, preview, false, false, false, rs);
			int_update();
			int_backdrop_done();
		}
	}

	if (rs.ui_gamemsg.length() && rs.ui_gamemsg != "none") {
		unsigned w0;
		unsigned w1;
		unsigned dx;
		string desc = g->description_tree_get();

		w0 = int_font_dx_get(text, rs.ui_gamemsg);
		w1 = int_font_dx_get(text, desc);
		if (w0 < w1)
			dx = w1;
		else
			dx = w0;

		int_box(x - 2 * border - dx / 2, y - border, dx + 4 * border, dy + border * 2, 1, COLOR_CHOICE_NORMAL.foreground);
		int_clear(x - 2 * border - dx / 2 + 1, y - border + 1, dx + 4 * border - 2, dy + border * 2 - 2, COLOR_CHOICE_NORMAL.background);

		int_put(text, x - w0 / 2, y, w0, rs.ui_gamemsg, COLOR_CHOICE_TITLE);
		y += int_font_dy_get(text);

		int_put(text, x - w1 / 2, y, w1, desc, COLOR_CHOICE_NORMAL);
		y += int_font_dy_get(text);

		int_update();
	}
}

int run_menu(config_state& rs, bool flipxy, bool silent)
{
	pgame_sort_set* psc;
	sort_item_func* category_func;

	log_std(("menu: sort begin\n"));

	// setup the sorted container
	switch (rs.sort_get()) {
	case sort_by_root_name:
		psc = new pgame_sort_set(sort_by_root_name_func);
		category_func = sort_item_root_name;
		break;
	case sort_by_name:
		psc = new pgame_sort_set(sort_by_name_func);
		category_func = sort_item_name;
		break;
	case sort_by_manufacturer:
		psc = new pgame_sort_set(sort_by_manufacturer_func);
		category_func = sort_item_manufacturer;
		break;
	case sort_by_year:
		psc = new pgame_sort_set(sort_by_year_func);
		category_func = sort_item_year;
		break;
	case sort_by_time:
		psc = new pgame_sort_set(sort_by_time_func);
		category_func = sort_item_time;
		break;
	case sort_by_smart_time:
		psc = new pgame_sort_set(sort_by_smart_time_func);
		category_func = sort_item_smart_time;
		break;
	case sort_by_session:
		psc = new pgame_sort_set(sort_by_session_func);
		category_func = sort_item_session;
		break;
	case sort_by_group:
		psc = new pgame_sort_set(sort_by_group_func);
		category_func = sort_item_group;
		break;
	case sort_by_type:
		psc = new pgame_sort_set(sort_by_type_func);
		category_func = sort_item_type;
		break;
	case sort_by_size:
		psc = new pgame_sort_set(sort_by_size_func);
		category_func = sort_item_size;
		break;
	case sort_by_res:
		psc = new pgame_sort_set(sort_by_res_func);
		category_func = sort_item_res;
		break;
	case sort_by_info:
		psc = new pgame_sort_set(sort_by_info_func);
		category_func = sort_item_info;
		break;
	case sort_by_timepersession:
		psc = new pgame_sort_set(sort_by_timepersession_func);
		category_func = sort_item_timepersession;
		break;
	case sort_by_emulator:
		psc = new pgame_sort_set(sort_by_emulator_func);
		category_func = sort_item_emulator;
		break;
	default:
		return EVENT_NONE;
	}

	string emu_msg;

	// setup the emulator state
	for (pemulator_container::iterator i = rs.emu.begin(); i != rs.emu.end(); ++i) {
		bool state = false;
		if (rs.include_emu_get().size() == 0) {
			state = true;
		} else {
			for (emulator_container::const_iterator j = rs.include_emu_get().begin(); j != rs.include_emu_get().end(); ++j) {
				if ((*i)->user_name_get() == *j) {
					state = true;
					break;
				}
			}
		}
		if (state) {
			if (emu_msg.length())
				emu_msg += ", ";
			emu_msg += (*i)->user_name_get();
		}
		(*i)->state_set(state);
	}

	// setup the group state
	for (pcategory_container::iterator i = rs.group.begin(); i != rs.group.end(); ++i) {
		bool state = false;
		if (rs.include_group_get().size() == 0) {
			state = true;
		} else {
			for (category_container::iterator j = rs.include_group_get().begin(); j != rs.include_group_get().end(); ++j) {
				if ((*i)->name_get() == *j) {
					state = true;
					break;
				}
			}
		}
		(*i)->state_set(state);
	}

	// setup the type state
	for (pcategory_container::iterator i = rs.type.begin(); i != rs.type.end(); ++i) {
		bool state = false;
		if (rs.include_type_get().size() == 0) {
			state = true;
		} else {
			for (category_container::iterator j = rs.include_type_get().begin(); j != rs.include_type_get().end(); ++j) {
				if ((*i)->name_get() == *j) {
					state = true;
					break;
				}
			}
		}
		(*i)->state_set(state);
	}

	bool has_emu = false;
	bool has_group = false;
	bool has_type = false;
	bool has_filter = false;

	// recompute the preview mask
	rs.preview_mask = 0;

	// select and sort
	for (game_set::const_iterator i = rs.gar.begin(); i != rs.gar.end(); ++i) {
		// emulator
		if (!i->emulator_get()->state_get())
			continue;

		has_emu = true;

		// group
		if (!i->group_derived_get()->state_get())
			continue;

		has_group = true;

		// type
		if (!i->type_derived_get()->state_get())
			continue;

		has_type = true;

		// filter
		if (!i->emulator_get()->filter(*i))
			continue;

		has_filter = true;

		psc->insert(&*i);

		// update the preview mask
		if (i->preview_snap_get().is_valid() || i->preview_clip_get().is_valid())
			rs.preview_mask |= preview_snap;
		if (i->preview_flyer_get().is_valid())
			rs.preview_mask |= preview_flyer;
		if (i->preview_cabinet_get().is_valid())
			rs.preview_mask |= preview_cabinet;
		if (i->preview_icon_get().is_valid())
			rs.preview_mask |= preview_icon;
		if (i->preview_marquee_get().is_valid())
			rs.preview_mask |= preview_marquee;
		if (i->preview_title_get().is_valid())
			rs.preview_mask |= preview_title;
	}

	/* prepare a warning message if the game list is empty */
	string empty_msg;
	if (rs.gar.empty())
		empty_msg = "No game was found";
	else if (!has_emu)
		empty_msg = "No game was found for the emulator " + emu_msg;
	else if (!has_group)
		empty_msg = "No game matches the group selection for " + emu_msg;
	else if (!has_type)
		empty_msg = "No game matches the type selection for " + emu_msg;
	else if (!has_filter)
		empty_msg = "No game matches the filter selection for " + emu_msg;
	else
		empty_msg = "";

	rs.mode_mask = ~rs.mode_skip_mask & (mode_full | mode_full_mixed
		| mode_text | mode_list | mode_list_mixed | mode_tile_tiny | mode_tile_small
		| mode_tile_normal | mode_tile_big | mode_tile_enormous
		| mode_tile_giant | mode_tile_icon | mode_tile_marquee);

	// remove some modes if the corresponding preview are not available
	if ((rs.preview_mask & preview_icon) == 0)
		rs.mode_mask &= ~mode_tile_icon;
	if ((rs.preview_mask & preview_marquee) == 0)
		rs.mode_mask &= ~mode_tile_marquee;
	rs.preview_mask &= ~(preview_icon | preview_marquee);

	// if no preview at all, enable only textual mode
	if (rs.preview_mask == 0)
		rs.mode_mask = mode_text;

	// if no mode at all, enable at least textual mode
	if (rs.mode_mask == 0)
		rs.mode_mask = mode_text;

	log_std(("menu: sort end\n"));

	bool done = false;
	int key = 0;

	while (!done) {
		key = run_menu_sort(rs, *psc, category_func, flipxy, silent, empty_msg);

		// don't replay the sound and clip
		silent = true;

		if (!rs.lock_effective) {
			switch (key) {
			case EVENT_MODE:
				if (rs.mode_mask) {
					do {
						switch (rs.mode_get()) {
						case mode_full: rs.mode_set(mode_full_mixed); break;
						case mode_full_mixed: rs.mode_set(mode_text); break;
						case mode_text: rs.mode_set(mode_list); break;
						case mode_list: rs.mode_set(mode_list_mixed); break;
						case mode_list_mixed: rs.mode_set(mode_tile_tiny); break;
						case mode_tile_tiny: rs.mode_set(mode_tile_small); break;
						case mode_tile_small: rs.mode_set(mode_tile_normal); break;
						case mode_tile_normal: rs.mode_set(mode_tile_big); break;
						case mode_tile_big: rs.mode_set(mode_tile_enormous); break;
						case mode_tile_enormous: rs.mode_set(mode_tile_giant); break;
						case mode_tile_giant: rs.mode_set(mode_tile_icon); break;
						case mode_tile_icon: rs.mode_set(mode_tile_marquee); break;
						case mode_tile_marquee: rs.mode_set(mode_full); break;
						}
					} while ((rs.mode_get() & rs.mode_mask) == 0);
				}
				break;
			case EVENT_PREVIEW:
				if (rs.preview_mask) {
					do {
						switch (rs.preview_get()) {
						case preview_icon:
						case preview_marquee:
						case preview_snap: rs.preview_set(preview_title); break;
						case preview_title: rs.preview_set(preview_flyer); break;
						case preview_flyer: rs.preview_set(preview_cabinet); break;
						case preview_cabinet: rs.preview_set(preview_snap); break;
						}
					} while ((rs.preview_get() & rs.preview_mask) == 0);
				}
				break;
			}
		}

		switch (key) {
		case EVENT_ENTER:
		case EVENT_CLONE:
		case EVENT_IDLE_0:
		case EVENT_IDLE_1:
		case EVENT_LOCK:
		case EVENT_HELP:
		case EVENT_GROUP:
		case EVENT_TYPE:
		case EVENT_ATTRIB:
		case EVENT_SORT:
		case EVENT_SETGROUP:
		case EVENT_SETTYPE:
		case EVENT_COMMAND:
		case EVENT_VOLUME:
		case EVENT_DIFFICULTY:
		case EVENT_MENU:
		case EVENT_CALIBRATION:
		case EVENT_EMU:
		case EVENT_ROTATE:
		case EVENT_ESC:
		case EVENT_EXIT:
		case EVENT_OFF:
		case EVENT_EXIT_FORCE:
		case EVENT_OFF_FORCE:
			done = true;
			break;
		}
	}

	delete psc;

	return key;
}


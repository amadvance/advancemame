/*
 * This file is part of the AdvanceMAME project.
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

#include "os.h"
#include "game.h"
#include "text.h"
#include "play.h"
#include "menu.h"
#include "video.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

#include <unistd.h>

using namespace std;

// -------------------------------------------------------------------------
// Sort category

// Return the element sorted for fast moving
typedef string sort_item_func(const game& g);

string sort_item_root_name(const game& g) {
	const game& root = g.root_get();

	string pre = root.description_get().substr(0,1);

	if (pre.length()) {
		if (!isalpha(pre[0]))
			pre = "0-9";
		else
			pre = toupper(pre[0]);
	}

	return pre;
}

string sort_item_name(const game& g) {
	string pre = g.description_get().substr(0,1);

	if (pre.length()) {
		if (isdigit(pre[0]))
			pre = "0-9";
		else
			pre = toupper(pre[0]);
	}

	return pre;
}

string sort_item_manufacturer(const game& g) {
	return g.manufacturer_get();
}

string sort_item_year(const game& g) {
	if (!g.year_get().length())
		return "unknow";
	else
		return g.year_get();
}

string sort_item_time(const game& g) {
	(void)g;
	return "";
}

string sort_item_coin(const game& g) {
	(void)g;
	return "";
}

string sort_item_group(const game& g) {
	return g.group_get();
}

string sort_item_type(const game& g) {
	return g.type_get();
}

string sort_item_size(const game& g) {
	(void)g;
	return "";
}

string sort_item_res(const game& g) {
	char buffer[32];
	sprintf(buffer,"%dx%d", g.sizex_get(), g.sizey_get());
	return buffer;
}

// ------------------------------------------------------------------------
// Menu array

class menu_entry {
	const game* g;
	string desc;
	unsigned ident;
public:
	menu_entry(const game* Ag, unsigned Aident);
	menu_entry(const string& desc);

	bool has_game() const { return g != 0; }
	const game& game_get() const { return *g; }
	bool is_selectable() const { return has_game(); }
	const string& desc_get() const { return has_game() ? game_get().description_get() : desc; }
	unsigned ident_get() const { return ident; }

	string category(sort_item_func* category_extract);
};

string menu_entry::category(sort_item_func* category_extract) {
	if (has_game())
		return category_extract(game_get());
	else
		return desc_get();
}

menu_entry::menu_entry(const game* Ag, const unsigned Aident) {
	g = Ag;
	ident = Aident;
}

menu_entry::menu_entry(const string& Adesc) {
	g = 0;
	desc = Adesc;
	ident = 0;
}

typedef vector<menu_entry*> menu_array;

// ------------------------------------------------------------------------
// Menu draw

string strip_comment(const string& s) {
	bool in = false;
	bool pred_space = true;
	string r;
	for(int i = 0;i<s.length();++i) {
		if (s[i]=='(' || s[i]=='[') {
			in = true;
		} else if (s[i]==')' || s[i]==']') {
			in = false;
		} else {
			if (!in) {
				if (!isspace(s[i]) || !pred_space) {
					r += s[i];
					pred_space = isspace(s[i]);
				}
			}
		}
	}
	return r;
}

static bool issep(char c) {
	return isspace(c) || (c == '-');
}

void draw_menu_game_center(const game_set& gar, const game& g, int x, int y, int dx, int dy, bool selected, merge_t merge, bool clone_listed) {
	string s = g.description_get();

	if (!clone_listed)
		s = strip_comment(s);

	unsigned ident = text_font_dx_get()/2;

	int color;
	int color_first;
	int color_in;
	int color_back;
	const game* gb;
	if (!clone_listed)
		gb = &g.clone_best_get();
	else
		gb = &g;
	if (gb->is_good() && gar.is_tree_rom_of_present(gb->name_get(),merge)) {
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

	while (dy >= py + text_font_dy_get()) {
		unsigned str_len = s.length() - str_pos;
		while (str_len > 0 && issep(s[str_pos])) {
			++str_pos;
			--str_len;
		}
		unsigned pixel_len = text_put_width(s.substr(str_pos,str_len));
		while (pixel_len > dx - 2*ident) {
			while (str_len > 0 && !issep(s[str_pos+str_len-1]))
				--str_len;
			while (str_len > 0 && issep(s[str_pos+str_len-1]))
				--str_len;
			pixel_len = text_put_width(s.substr(str_pos,str_len));
		}

		if (str_len == 0) {
			// retry splitting in any point
			str_len = s.length() - str_pos;
			if (str_len) {
				pixel_len = text_put_width(s.substr(str_pos,str_len));
				while (pixel_len > dx - 2*ident) {
					--str_len;
					pixel_len = text_put_width(s.substr(str_pos,str_len));
				}
			}
		}

		if (!str_len) {
			text_clear(x, y+py, dx, text_font_dy_get(), color_back >> 4);
		} else {
			int space_left = (dx - pixel_len) / 2;
			int ident_left = ident;
			if (ident_left > space_left)
				ident_left = space_left;
			int space_right = dx - pixel_len - space_left;
			int ident_right = ident;
			if (ident_right > space_right)
				ident_right = space_right;

			text_clear(x, y+py, space_left-ident_left, text_font_dy_get(), color_back >> 4);
			text_clear(x+space_left-ident_left, y+py, ident_left, text_font_dy_get(), color >> 4);
			text_put_special(in,x+space_left, y+py, pixel_len, s.substr(str_pos,str_len), color_first, color_in, color);
			text_clear(x+space_left+pixel_len, y+py, ident_right, text_font_dy_get(), color >> 4);
			text_clear(x+space_left+pixel_len+ident_right, y+py, space_right-ident_right, text_font_dy_get(), color_back >> 4);
		}

		color_first = color;
		py += text_font_dy_get();
		str_pos += str_len;
	}
}

void draw_menu_game_left(const game_set& gar, const game& g, int x, int y, int dx, bool selected, merge_t merge, bool clone_listed, unsigned ident) {
	string s = g.description_get();

	if (!clone_listed)
		s = strip_comment(s);

	ident += text_font_dx_get()/2;

	int color;
	int color_first;
	int color_in;
	const game* gb;
	if (!clone_listed)
		gb = &g.clone_best_get();
	else
		gb = &g;
	if (gb->is_good() && gar.is_tree_rom_of_present(gb->name_get(),merge)) {
		color = selected ? COLOR_MENU_SELECT : COLOR_MENU_NORMAL;
		color_first = selected ? COLOR_MENU_TAG_SELECT : COLOR_MENU_TAG;
		color_in = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
	} else {
		color = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
		color_first = color;
		color_in = color;
	}

	text_clear(x,y,ident,text_font_dy_get(),color >> 4);
	bool in = false;
	text_put_special(in,x+ident,y,dx - ident,s,color_first,color_in,color);
}

void draw_menu_empty(int x, int y, int dx, bool selected) {
	int color;
	color = selected ? COLOR_MENU_HIDDEN_SELECT : COLOR_MENU_HIDDEN;
	text_clear(x,y,dx,text_font_dy_get(),color >> 4);
}

void draw_menu_desc(const string& desc, int x, int y, int dx, bool selected) {
	int color;
	color = selected ? COLOR_MENU_TAG_SELECT : COLOR_MENU_TAG;
	text_put_filled(x,y,dx,desc,color);
}

void draw_tag_left(const string& s, int& xl, int& xr, int y, int sep, int color) {
	int len = sep + text_put_width(s);
	if (len < xr-xl) {
		text_put(xl,y,s,color);
		xl += len;
	}
}

void draw_tag_right(const string& s, int& xl, int& xr, int y, int sep, int color) {
	int len = text_put_width(s);
	if (len+sep < xr-xl) {
		text_put(xr-len,y,s,color);
		xr -= len + sep;
	}
}

void draw_menu_bar(const game* g, int g2, int x, int y, int dx, bool clone_listed) {
	text_clear(x,y,dx,text_font_dy_get(),COLOR_MENU_BAR >> 4);

	int separator = dx > 40*text_font_dx_get() ? 1*text_font_dx_get() : 0*text_font_dx_get();
	int in_separator =  dx > 40*text_font_dx_get() ? 2*text_font_dx_get() : 1*text_font_dx_get();
	int xr = dx - separator + x;
	int xl = separator + x;

	if (g) {
		ostringstream os;
		os << setw(4) << setfill(' ') << g2;
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
	}

	if (g) {
		ostringstream os;
		unsigned time;
		if (clone_listed)
			time = g->time_get();
		else
			time = g->time_tree_get();
		os << setw(3) << setfill('0') << (time/3600) << ":" << setw(2) << setfill('0') << ((time/60)%60);
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
	}

	if (g && g->group_derived_get() != CATEGORY_UNDEFINED) {
		ostringstream os;
		os << g->group_derived_get();
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);
	}

	if (g && g->type_derived_get() != CATEGORY_UNDEFINED) {
		ostringstream os;
		os << g->type_derived_get();
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);
	}

	if (g) {
		ostringstream os;
		if (clone_listed)
			os << g->description_get();
		else
			os << strip_comment(g->description_get());
		draw_tag_left(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);
	}

	if (g) {
		ostringstream os;
		unsigned coin;
		if (clone_listed)
			coin = g->coin_get();
		else
			coin = g->coin_tree_get();
		os << setw(3) << setfill(' ') << coin << "c";
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
	}

	if (g) {
		ostringstream os;
		if (g->size_get() >= 10*1000*1000)
			os << setw(4) << g->size_get()/1000/1000 << "M";
		else
			os << setw(4) << g->size_get()/1000 << "k";
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
	}

	if (g && g->sizex_get() && g->sizey_get()) {
		ostringstream os;
		os << g->sizex_get() << "x" << g->sizey_get();
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
	}

	if (clone_listed) {
		if (g) {
			ostringstream os;
			os << g->name_get();
			draw_tag_left(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
		}
	} else {
		if (g) {
			const game* gb = &g->clone_best_get();
			ostringstream os;
			os << gb->name_get();
			draw_tag_left(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
		}
	}
}

void draw_menu_info(const game_set& gar, const game* g, int x, int y, int dx, merge_t merge, preview_t preview, game_sort_t sort_mode, bool clone_listed, bool lock) {
	text_clear(x,y,dx,text_font_dy_get(),COLOR_MENU_BAR >> 4);

	int separator = dx > 40*text_font_dx_get() ? 1*text_font_dx_get() : 0*text_font_dx_get();
	int in_separator = dx > 40*text_font_dx_get() ? 2*text_font_dx_get() : 1*text_font_dx_get();
	int xr = dx - separator + x;
	int xl = separator + x;

	if (g) {
		const game* gb;
		if (!clone_listed && g)
			gb = &g->clone_best_get();
		else
			gb = g;

		if (!gar.is_tree_rom_of_present(gb->name_get(),merge))
			draw_tag_right("MISSING",xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);
		else if (!gb->working_get())
			draw_tag_right("  ALPHA",xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);
		else if (!gb->color_get())
			draw_tag_right("NOCOLOR",xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);
		else if (!gb->sound_get())
			draw_tag_right("NOSOUND",xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);
		else
			draw_tag_right("       ",xl,xr,y,in_separator,COLOR_MENU_BAR_TAG);

		draw_tag_left(g->manufacturer_get(),xl,xr,y,0,COLOR_MENU_BAR);

		if (g->year_get().length())
			draw_tag_left(", " + g->year_get(),xl,xr,y,0,COLOR_MENU_BAR);

		if (g->clone_get() > 0) {
			ostringstream os;
			os << ", " << g->clone_get() << " clones";
			draw_tag_left(os.str(),xl,xr,y,0,COLOR_MENU_BAR);
		}

		if (g->parent_get()) {
			ostringstream os;
			os << ", ";
			if (g->software_get())
				os << "software of";
			else
				os << "clone of";
			os << " " << g->parent_get()->name_get();
			draw_tag_left(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
		}
	}

	if (lock)
		draw_tag_right("locked",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN);

	switch (preview) {
		case preview_flyer: draw_tag_right("flyers",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case preview_cabinet: draw_tag_right("cabinets",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case preview_icon: draw_tag_right("icons",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case preview_marquee: draw_tag_right("marquees",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case preview_title: draw_tag_right("titles",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case preview_snap: draw_tag_right("snap",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
	}

	switch (sort_mode) {
		case sort_by_group : draw_tag_right("group",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_name : draw_tag_right("name",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_root_name : draw_tag_right("parent",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_time : draw_tag_right("time",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_coin : draw_tag_right("coin",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_year : draw_tag_right("year",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_manufacturer : draw_tag_right("manuf",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_type : draw_tag_right("type",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_size : draw_tag_right("size",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
		case sort_by_res : draw_tag_right("res",xl,xr,y,in_separator,COLOR_MENU_BAR_HIDDEN); break;
	}

#ifdef __MSDOS__
	{
		ostringstream os;
		os << _go32_dpmi_remaining_physical_memory() / (1024*1024);
		os << "/" << _go32_dpmi_remaining_virtual_memory() / (1024*1024);
		draw_tag_right(os.str(),xl,xr,y,in_separator,COLOR_MENU_BAR);
	}
#endif

}

struct cell_t {
	// position and size
	int x;
	int y;
	int dx;
	int dy;
};

void draw_menu_window(const game_set& gar, const menu_array& gc, struct cell_t* cell, int coln, int rown, int start, int pos, bool use_ident, merge_t merge, bool clone_listed, bool center) {
	for(int r=0;r<rown;++r) {
		for(int c=0;c<coln;++c) {
			if (start < gc.size()) {
				if (gc[start]->has_game()) {
					const game& g = gc[start]->game_get();
					if (center)
						draw_menu_game_center(gar,g,cell->x,cell->y, cell->dx, cell->dy, start == pos, merge, clone_listed );
					else
						draw_menu_game_left(gar,g,cell->x,cell->y, cell->dx, start == pos, merge, clone_listed, use_ident ? gc[start]->ident_get() : 0);
				} else {
					draw_menu_desc(gc[start]->desc_get(),cell->x,cell->y, cell->dx, start == pos );
				}
			} else {
				draw_menu_empty(cell->x,cell->y, cell->dx, start == pos );
			}
			++start;
			++cell;
		}
	}
}

void draw_menu_scroll(int x, int y, int dx, int dy, int pos, int delta, int max) {
	if (max <= 1)
		return;

	if (pos >= max)
		pos = max - 1;
	if (pos + delta > max)
		delta = max - pos;

	int y0 = pos * (dy-1) / (max-1);
	int y1 = (pos+delta-1) * (dy-1) / (max-1);

	text_clear(x,y,dx,dy,COLOR_MENU_GRID >> 4);
	text_clear(x,y+y0,dx,y1-y0+1,COLOR_MENU_GRID);
}

void draw_menu_info(const string& desc, unsigned counter) {
	int border = text_font_dx_get()/2;

	int dx = text_put_width(desc);
	int x = (text_dx_get() - dx) / 2;

	int dy = text_font_dy_get();
	int y;

	if (counter % 2 == 0)
		y = text_dy_get() - 3 * text_font_dy_get();
	else
		y = 2 * text_font_dy_get();

	text_box(x-border,y-border,dx+2*border,dy+border*2,1,COLOR_CHOICE_NORMAL);
	text_clear(x-border+1,y-border+1,dx+2*border-2,dy+border*2-2,COLOR_CHOICE_NORMAL >> 4);

	text_put(x,y,dx,desc,COLOR_CHOICE_TITLE);
}

// ------------------------------------------------------------------------
// Menu utility

bool menu_fast_compare(const string& game, const string& fast) {
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

bool sound_find_preview(resource& path, const config_state& rs, const game* pgame) {
	(void)rs;
	if (pgame && pgame->preview_find(path,&game::preview_sound_get))
		return true;
	return false;
}

bool backdrop_find_preview_strict(resource& path, preview_t preview, const game* pgame, bool only_clip) {
	if (pgame) {
		switch (preview) {
			case preview_icon :
				if (pgame->preview_find(path,&game::preview_icon_get))
					return true;
				break;
			case preview_marquee :
				if (pgame->preview_find(path,&game::preview_marquee_get))
					return true;
				break;
			case preview_title :
				if (pgame->preview_find(path,&game::preview_title_get))
					return true;
				break;
			case preview_snap :
				if (!only_clip) {
					if (pgame->preview_find(path,&game::preview_snap_get))
						return true;
				}
				if (pgame->preview_find(path,&game::preview_clip_get))
					return true;
				break;
			case preview_flyer :
				if (pgame->preview_find(path,&game::preview_flyer_get))
					return true;
				break;
			case preview_cabinet :
				if (pgame->preview_find(path,&game::preview_cabinet_get))
					return true;
				break;
		}
	}

	return false;
}

bool backdrop_find_preview_default(resource& path, unsigned& aspectx, unsigned& aspecty, preview_t preview, const game* pgame, const config_state& rs) {

	if (backdrop_find_preview_strict(path, preview, pgame, false))
		return true;

	path = resource();

	// reset the aspect, the default preview may be anything
	aspectx = 0;
	aspecty = 0;

	switch (preview) {
		case preview_icon :
			if (rs.preview_default_icon != "none")
				path = rs.preview_default_icon;
			break;
		case preview_marquee :
			if (rs.preview_default_marquee != "none")
				path = rs.preview_default_marquee;
			break;
		case preview_title :
			if (rs.preview_default_title != "none")
				path = rs.preview_default_title;
			break;
		case preview_snap :
			if (rs.preview_default_snap != "none")
				path = rs.preview_default_snap;
			break;
		case preview_flyer :
			if (rs.preview_default_flyer != "none")
				path = rs.preview_default_flyer;
			break;
		case preview_cabinet :
			if (rs.preview_default_cabinet != "none")
				path = rs.preview_default_cabinet;
			break;
	}

	if (!path.is_valid() && rs.preview_default != "none")
		path = rs.preview_default;

	return path.is_valid();
}

void backdrop_game_set(const game* effective_game, unsigned back_pos, preview_t preview, bool current, bool highlight, bool clip, config_state& rs) {
	resource backdrop_res;
	resource clip_res;

	unsigned aspectx;
	unsigned aspecty;
	if (effective_game && (preview == preview_snap || preview == preview_title)) {
		aspectx = effective_game->aspectx_get();
		aspecty = effective_game->aspecty_get();

		if (clip && preview == preview_snap) {
			if (effective_game->preview_find(clip_res, &game::preview_clip_get))
				text_clip_set(back_pos, clip_res, aspectx, aspecty);
			else
				text_clip_clear();
		}
	} else {
		aspectx = 0;
		aspecty = 0;

		if (clip && preview == preview_snap) {
			text_clip_clear();
		}
	}

	if (backdrop_find_preview_default(backdrop_res, aspectx, aspecty, preview, effective_game, rs)) {
		text_backdrop_set(back_pos, backdrop_res, highlight, aspectx, aspecty);
		if (current)
			rs.current_backdrop = backdrop_res;
	} else {
		text_backdrop_clear(back_pos, highlight);
		if (current)
			rs.current_backdrop = resource();
	}
}

void backdrop_index_set(unsigned pos, menu_array& gc, unsigned back_pos, preview_t preview, bool current, bool highlight, bool clip, config_state& rs) {
	const game* effective_game;

	if (pos < gc.size() && gc[pos]->has_game())
		effective_game = &gc[pos]->game_get().clone_best_get();
	else
		effective_game = 0;

	backdrop_game_set(effective_game, back_pos, preview, current, highlight, clip, rs);
}

//--------------------------------------------------------------------------
// Menu run

static void run_background_reset(config_state& rs) {
	play_background_stop(PLAY_PRIORITY_GAME_BACKGROUND);
	rs.current_sound = resource();
}

static void run_background_set(config_state& rs, const resource& sound) {
	if (sound.is_valid()) {
		os_log(("menu: play game music '%s'\n", sound.path_get().c_str()));
		play_background_stop(PLAY_PRIORITY_BACKGROUND);
		play_background_effect(sound,PLAY_PRIORITY_GAME_BACKGROUND,false);
		rs.current_sound = sound;
	} else {
		os_log(("menu: no game music found\n"));
	}
}

static void run_background_idle(config_state& rs) {
	if (!play_background_is_active()) {
		rs.current_sound = resource();
		if (rs.sound_background.size() != 0) {
			string path = file_select_random(rs.sound_background);
			os_log(("menu: play background music '%s'\n", path.c_str()));
			play_background_effect(path,PLAY_PRIORITY_BACKGROUND,false);
		} else if (rs.sound_background_loop.length()) {
			os_log(("menu: play background effect '%s'\n", rs.sound_background_loop.c_str()));
			play_background_effect(rs.sound_background_loop,PLAY_PRIORITY_BACKGROUND,true);
		} else {
			os_log(("menu: no background effect\n"));
		}
	}
}

static void run_background_wait(config_state& rs, const resource& sound) {
	os_clock_t back_start = os_clock();

	// delay before the background music start
	os_clock_t back_stop = back_start + rs.repeat_rep * OS_CLOCKS_PER_SEC / 666; // / 666 = * 3 / 2 / 1000

	// short busy wait (text_keypressed contains a idle call) */
	while (!text_keypressed() && (os_clock() < back_stop)) {
	}

	if (!text_keypressed()) {
		// stop the previous playing
		run_background_reset(rs);

		// start the new game play
		run_background_set(rs, sound);

		// start the new game clip
		text_clip_start();

		// wait until something pressed
		while (!text_keypressed()) {

			// start some background music if required
			run_background_idle(rs);
		}
	}
}

static int run_menu_user(config_state& rs, bool flipxy, menu_array& gc, sort_item_func* category_extract) {
	int coln; // number of columns
	int rown; // number of rows

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

	int bar_right_x; // Right bar
	int bar_right_y;
	int bar_right_dx;
	int bar_right_dy;

	int space_x; // space between tiles
	int space_y;

	int name_dy; // vertical space for the name in every cell

	unsigned backdrop_mac; // number of backdrops
	struct cell_t* backdrop_map; // map of the backdrop positions
	struct cell_t* backdrop_map_bis; // alternate map of the backdrop positions

	os_log(("menu: user begin\n"));

	// standard bars
	bar_top_x = 0;
	bar_top_y = 0;
	bar_top_dy = text_font_dy_get();
	bar_bottom_x = 0;
	bar_bottom_dy = text_font_dy_get();
	bar_left_x = 0;
	bar_left_y = bar_top_dy;
	bar_left_dx = text_font_dx_get()/2;
	bar_right_y = bar_top_dy;
	bar_right_dx = text_font_dx_get()/2;

	// effective preview type
	preview_t effective_preview;
	switch (rs.mode_effective) {
		case mode_tile_icon : effective_preview = preview_icon; break;
		case mode_tile_marquee : effective_preview = preview_marquee; break;
		default:
			effective_preview = rs.preview_effective;
			if (effective_preview == preview_icon || effective_preview == preview_marquee)
				effective_preview = preview_snap;
	}

	// cursor
	unsigned cursor_size;
	if (rs.mode_effective != mode_full && rs.mode_effective != mode_list && rs.mode_effective != mode_full_mixed && rs.mode_effective != mode_list_mixed && rs.mode_effective != mode_text) {
		// need a cursor
		cursor_size = video_size_y() / 300 + 1; // size of the flashing cursor
	} else {
		// no cursor
		cursor_size = 0;
	}

	// use identation on the names
	bool use_ident = false;

	if (rs.mode_effective == mode_tile_icon) {
		// icon mode
		unsigned icon_space = rs.icon_space;
		if (icon_space > 64)
			icon_space = 64;
		if (icon_space < 2*cursor_size + text_font_dy_get())
			icon_space = 2*cursor_size + text_font_dy_get();

		space_x = 0;
		space_y = 0;
		name_dy = text_font_dy_get();
		if (!flipxy) {
			coln = (text_dx_get() - bar_left_dx - bar_right_dx) / (32+icon_space);
			rown = (text_dy_get() - bar_top_dy - bar_bottom_dy) / (32+icon_space);
		} else {
			coln = (text_dx_get() - bar_left_dx - bar_right_dx) / (32+icon_space);
			rown = (text_dy_get() - bar_top_dy - bar_bottom_dy) / (32+icon_space);
		}

		backdrop_mac = coln*rown;

		win_x = bar_left_dx;
		win_y = bar_top_dy;

		win_dx = (text_dx_get() - bar_left_dx - bar_right_dx - (coln-1) * space_x) / coln * coln + (coln-1) * space_x;
		win_dy = (text_dy_get() - bar_top_dy - bar_bottom_dy - (rown-1) * space_y) / rown * rown + (rown-1) * space_y;

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;

		bar_right_x = bar_left_dx + win_dx;
		bar_bottom_y = bar_top_dy + win_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
		bar_left_dy = bar_right_dy = win_dy;
	} else if (rs.mode_effective == mode_tile_marquee) {
		// marquee mode
		space_x = text_font_dx_get()/2;
		space_y = text_font_dx_get()/2;
		name_dy = 0;

		if (!flipxy) {
			coln = 3;
			rown = 3*4* 3/4;
		} else {
			coln = 2;
			rown = 2*4* 4/3;
		}

		backdrop_mac = coln*rown;

		win_x = bar_left_dx;
		win_y = bar_top_dy;

		win_dx = (text_dx_get() - bar_left_dx - bar_right_dx - (coln-1) * space_x) / coln * coln + (coln-1) * space_x;
		win_dy = (text_dy_get() - bar_top_dy - bar_bottom_dy - (rown-1) * space_y) / rown * rown + (rown-1) * space_y;

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;

		bar_right_x = bar_left_dx + win_dx;
		bar_bottom_y = bar_top_dy + win_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
		bar_left_dy = bar_right_dy = win_dy;
	} else if (rs.mode_effective == mode_full || rs.mode_effective == mode_full_mixed) {
		// full mode
		if (rs.mode_effective == mode_full_mixed)
			backdrop_mac = 4;
		else
			backdrop_mac = 1;
		name_dy = 0;

		win_x = bar_left_dx;
		win_y = bar_top_dy;

		space_x = 0;
		space_y = 0;

		coln = 1;
		rown = 1;
		win_dx = 0;
		win_dy = 0;

		backdrop_x = win_x;
		backdrop_y = win_y;
		backdrop_dx = text_dx_get() - bar_left_dx - bar_right_dx;
		backdrop_dy = text_dy_get() - bar_top_dy - bar_bottom_dy;

		bar_right_x = bar_left_dx + backdrop_dx;
		bar_bottom_y = bar_top_dy + backdrop_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + backdrop_dx;
		bar_left_dy = bar_right_dy = backdrop_dy;
	} else if (rs.mode_effective == mode_text) {
		// text mode
		backdrop_mac = 0;

		name_dy = text_font_dy_get();

		space_x = text_font_dx_get() / 2;
		space_y = 0;

		coln = text_dx_get() / (20*text_font_dx_get());
		if (coln < 2)
			coln = 2;
		rown = (text_dy_get() - bar_top_dy - bar_bottom_dy) / text_font_dy_get();

		win_x = bar_left_dx;
		win_y = bar_top_dy;
		win_dx = (text_dx_get() - bar_left_dx - bar_right_dx - (coln-1) * space_x) / coln * coln + (coln-1) * space_x;
		win_dy = rown * text_font_dy_get();

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;

		bar_right_x = bar_left_dx + win_dx;
		bar_bottom_y = bar_top_dy + win_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
		bar_left_dy = bar_right_dy = win_dy;
	} else if (rs.mode_effective == mode_list || rs.mode_effective == mode_list_mixed) {
		// list mode
		if (rs.mode_effective == mode_list_mixed)
			backdrop_mac = 4;
		else
			backdrop_mac = 1;
		name_dy = text_font_dy_get();

		win_x = bar_left_dx;
		win_y = bar_top_dy;

		unsigned multiplier;
		unsigned divisor;
		if (rs.mode_effective == mode_list) {
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
			win_dx = (text_dx_get() - bar_left_dx - bar_right_dx) * multiplier / divisor;
			win_dx -= win_dx % text_font_dx_get();

			rown = (text_dy_get() - bar_top_dy - bar_bottom_dy) / text_font_dy_get();
			win_dy = rown * text_font_dy_get();

			backdrop_x = win_x + win_dx;
			backdrop_y = win_y;
			backdrop_dx = text_dx_get() - win_dx - bar_left_dx - bar_right_dx;
			backdrop_dy = win_dy;

			bar_right_x = bar_left_dx + win_dx + backdrop_dx;
			bar_bottom_y = bar_top_dy + win_dy;
			bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx + backdrop_dx;
			bar_left_dy = bar_right_dy = win_dy;
		} else {
			// vertical
			space_x = text_font_dx_get() / 2;
			space_y = 0;
			coln = text_dx_get()/(20*text_font_dx_get());
			if (coln < 2)
				coln = 2;
			win_dx = (text_dx_get() - bar_left_dx - bar_right_dx - (coln-1) * space_x) / coln * coln + (coln-1) * space_x;

			rown = (text_dy_get() - bar_top_dy - bar_bottom_dy) * multiplier / (divisor * text_font_dy_get());
			win_dy = rown * text_font_dy_get();

			backdrop_x = win_x;
			backdrop_y = win_y + win_dy;
			backdrop_dx = win_dx;
			backdrop_dy = text_dy_get() - win_dy - bar_top_dy - bar_bottom_dy;

			bar_right_x = bar_left_dx + win_dx;
			bar_bottom_y = bar_top_dy + win_dy + backdrop_dy;
			bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
			bar_left_dy = bar_right_dy = win_dy + backdrop_dy;
		}
	} else {
		// tile modes
		space_x = text_font_dx_get()/2;
		space_y = text_font_dx_get()/2;

		if (!flipxy) {
			switch (rs.mode_effective) {
				default: /* for warnings */
				case mode_tile_giant :
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
				case mode_tile_enormous :
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
				case mode_tile_big :
					coln = 8;
					rown = 6;
					name_dy = 0;
					space_x = 0;
					space_y = 0;
				break;
				case mode_tile_normal :
					coln = 5;
					rown = 4;
					name_dy = text_font_dy_get();
				break;
				case mode_tile_small :
					coln = 4;
					rown = 3;
					name_dy = text_font_dy_get();
				break;
			}
		} else {
			switch (rs.mode_effective) {
				default: /* for warnings */
				case mode_tile_giant :
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
				case mode_tile_enormous :
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
				case mode_tile_big :
					coln = 6;
					rown = 8;
					name_dy = 0;
					space_x = 0;
					space_y = 0;
				break;
				case mode_tile_normal :
					coln = 4;
					rown = 5;
					name_dy = text_font_dy_get();
				break;
				case mode_tile_small :
					coln = 3;
					rown = 4;
					name_dy = text_font_dy_get();
				break;
			}
		}

		backdrop_mac = coln*rown;

		win_x = bar_left_dx;
		win_y = bar_top_dy;

		win_dx = (text_dx_get() - bar_left_dx - bar_right_dx - (coln-1) * space_x) / coln * coln + (coln-1) * space_x;
		win_dy = (text_dy_get() - bar_top_dy - bar_bottom_dy - (rown-1) * space_y) / rown * rown + (rown-1) * space_y;

		backdrop_x = 0;
		backdrop_y = 0;
		backdrop_dx = 0;
		backdrop_dy = 0;

		bar_right_x = bar_left_dx + win_dx;
		bar_bottom_y = bar_top_dy + win_dy;
		bar_top_dx = bar_bottom_dx = bar_left_dx + bar_right_dx + win_dx;
		bar_left_dy = bar_right_dy = win_dy;
	}

	backdrop_map = new cell_t[backdrop_mac];
	backdrop_map_bis = new cell_t[backdrop_mac];

	// text position
	struct cell_t* text_map = new cell_t[coln*rown];

	// size of the text cell
	int cell_dx = (win_dx - space_x * (coln-1)) / coln;
	int cell_dy = (win_dy - space_y * (rown-1)) / rown;

	if (rs.mode_effective == mode_full_mixed || rs.mode_effective == mode_list_mixed) {
		unsigned middle_outline = 1;

		int bar_dx; // size of the additional small images
		int bar_dy;
		int aspect_x;
		int aspect_y;

		if (rs.mode_effective == mode_list_mixed) {
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

		// game vertical
		if (rs.mode_effective == mode_list_mixed) {
			bar_dx = backdrop_dx - backdrop_dy * aspect_y / aspect_x;
			if (bar_dx < backdrop_dx / 5)
				bar_dx = backdrop_dx / 5;
			bar_dy = backdrop_dy / 3;
		} else {
			bar_dx = backdrop_dx - backdrop_dy * aspect_y / aspect_x;
			if (bar_dx < backdrop_dx / 5)
				bar_dx = backdrop_dx / 5;
			bar_dy = backdrop_dy / 3;
		}
		backdrop_map[0].x = backdrop_x;
		backdrop_map[0].y = backdrop_y;
		backdrop_map[0].dx = backdrop_dx-bar_dx+middle_outline;
		backdrop_map[0].dy = backdrop_dy;
		backdrop_map[1].x = backdrop_x+backdrop_dx-bar_dx;
		backdrop_map[1].y = backdrop_y;
		backdrop_map[1].dx = bar_dx;
		backdrop_map[1].dy = bar_dy+middle_outline;
		backdrop_map[2].x = backdrop_x+backdrop_dx-bar_dx;
		backdrop_map[2].y = backdrop_y+bar_dy;
		backdrop_map[2].dx = bar_dx;
		backdrop_map[2].dy = bar_dy+middle_outline;
		backdrop_map[3].x = backdrop_x+backdrop_dx-bar_dx;
		backdrop_map[3].y = backdrop_y+2*bar_dy;
		backdrop_map[3].dx = bar_dx;
		backdrop_map[3].dy = backdrop_dy-2*bar_dy;

		// game horizontal
		if (rs.mode_effective == mode_list_mixed) {
			bar_dx = backdrop_dx / 3;
			bar_dy = backdrop_dy - backdrop_dx * aspect_y / aspect_x;
			if (bar_dy < backdrop_dy / 5)
				bar_dy = backdrop_dy / 5;
		} else {
			bar_dx = backdrop_dx / 3;
			bar_dy = backdrop_dy - backdrop_dx * aspect_y / aspect_x;
			if (bar_dy < backdrop_dy / 5)
				bar_dy = backdrop_dy / 5;
		}
		backdrop_map_bis[0].x = backdrop_x;
		backdrop_map_bis[0].y = backdrop_y+bar_dy;
		backdrop_map_bis[0].dx = backdrop_dx;
		backdrop_map_bis[0].dy = backdrop_dy-bar_dy;
		backdrop_map_bis[1].x = backdrop_x;
		backdrop_map_bis[1].y = backdrop_y;
		backdrop_map_bis[1].dx = bar_dx+middle_outline;
		backdrop_map_bis[1].dy = bar_dy+middle_outline;
		backdrop_map_bis[2].x = backdrop_x+bar_dx;
		backdrop_map_bis[2].y = backdrop_y;
		backdrop_map_bis[2].dx = bar_dx+middle_outline;
		backdrop_map_bis[2].dy = bar_dy+middle_outline;
		backdrop_map_bis[3].x = backdrop_x+2*bar_dx;
		backdrop_map_bis[3].y = backdrop_y;
		backdrop_map_bis[3].dx = backdrop_dx-2*bar_dx;
		backdrop_map_bis[3].dy = bar_dy+middle_outline;
	}

	if (rs.mode_effective == mode_full_mixed || rs.mode_effective == mode_full) {
		text_map[0].x = 0;
		text_map[0].y = 0;
		text_map[0].dx = 0;
		text_map[0].dy = 0;
	} else if (rs.mode_effective == mode_list || rs.mode_effective == mode_list_mixed || rs.mode_effective == mode_text) {
		for(int r=0;r<rown;++r) {
			for(int c=0;c<coln;++c) {
				unsigned i = r*coln+c;
				int x = win_x + (cell_dx + space_x) * c;
				int y = win_y + (cell_dy + space_y) * r;
				text_map[i].x = x;
				text_map[i].y = y + cell_dy - name_dy;
				text_map[i].dx = cell_dx;
				text_map[i].dy = name_dy;
			}
		}
	} else {
		for(int r=0;r<rown;++r) {
			for(int c=0;c<coln;++c) {
				unsigned i = r*coln+c;
				int x = win_x + (cell_dx + space_x) * c;
				int y = win_y + (cell_dy + space_y) * r;
				if (rs.mode_effective == mode_tile_icon) {
					backdrop_map[i].dx = 32+2*cursor_size;
					backdrop_map[i].dy = 32+2*cursor_size;
					int name_row = 3;
					int space_up;
					do {
						--name_row;
						space_up = (cell_dy - backdrop_map[i].dy - name_dy*name_row) / 3;
					} while (space_up < 0);
					backdrop_map[i].x = x + (cell_dx - 32 - 2*cursor_size) / 2;
					backdrop_map[i].y = y + space_up;
					text_map[i].x = x;
					text_map[i].y = y + backdrop_map[i].dy + 2*space_up;
					text_map[i].dx = cell_dx;
					text_map[i].dy = name_row*name_dy;
				} else {
					backdrop_map[i].x = x;
					backdrop_map[i].y = y;
					backdrop_map[i].dx = cell_dx;
					backdrop_map[i].dy = cell_dy - name_dy;
					text_map[i].x = x;
					text_map[i].y = y + cell_dy - name_dy;
					text_map[i].dx = cell_dx;
					text_map[i].dy = name_dy;
				}
			}
		}
	}

	if (backdrop_mac == 1) {
		text_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, 1, 1, cursor_size, rs.preview_expand);
		text_backdrop_pos(0,backdrop_x,backdrop_y,backdrop_dx,backdrop_dy);
	} else if (backdrop_mac > 1) {
		if (rs.mode_effective == mode_tile_icon)
			text_backdrop_init(COLOR_MENU_ICON, COLOR_MENU_CURSOR, backdrop_mac, cursor_size, cursor_size, rs.preview_expand);
		else if (rs.mode_effective == mode_list_mixed || rs.mode_effective == mode_full_mixed)
			text_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, backdrop_mac, 1, cursor_size, rs.preview_expand);
		else {
			if (space_x == 0)
				text_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, backdrop_mac, 0, cursor_size, rs.preview_expand);
			else
				text_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, backdrop_mac, 1, cursor_size, rs.preview_expand);
		}
		for(int i=0;i<backdrop_mac;++i)
			text_backdrop_pos(i,backdrop_map[i].x,backdrop_map[i].y,backdrop_map[i].dx,backdrop_map[i].dy);
	}

	// reset the sound
	rs.current_sound = resource();

	// reset the clip
	if (backdrop_mac > 0)
		text_clip_init();

	int pos_rel_max = coln*rown;
	int pos_base_upper = gc.size();
	if (pos_base_upper % coln)
		pos_base_upper = pos_base_upper + coln - pos_base_upper % coln;
	pos_base_upper -= pos_rel_max;
	if (pos_base_upper < 0)
		pos_base_upper = 0;

	// restore the old position
	int pos_base = rs.menu_base_effective;
	int pos_rel = rs.menu_rel_effective;

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

	if (pos_base + pos_rel < gc.size() && gc[pos_base+pos_rel]->has_game()) {
		rs.current_game = &gc[pos_base+pos_rel]->game_get();
		rs.current_clone = 0;
	} else {
		rs.current_game = 0;
		rs.current_clone = 0;
	}

	bool done = false;
	int key = 0;
	int exit_count = 0;

	// clear all the screen
	text_clear();

	// clear the used part
	text_clear(0,0,bar_left_dx + win_dx + bar_right_dx,bar_top_dy + win_dy + bar_bottom_dy,COLOR_MENU_GRID >> 4);

	os_log(("menu: user end\n"));

	while (!done) {
		if (name_dy)
			draw_menu_window(rs.gar,gc,text_map,coln,rown,pos_base, pos_base+pos_rel, use_ident, rs.merge, rs.exclude_clone_effective != exclude, rs.mode_effective == mode_tile_icon);
		if (bar_top_dy)
			draw_menu_bar(rs.current_game,gc.size(),bar_top_x,bar_top_y,bar_top_dx, rs.exclude_clone_effective != exclude);
		if (bar_bottom_dy)
			draw_menu_info(rs.gar,rs.current_game,bar_bottom_x,bar_bottom_y,bar_bottom_dx,rs.merge,effective_preview,rs.sort_effective,rs.exclude_clone_effective != exclude,rs.lock_effective);
		if (bar_right_dx)
			draw_menu_scroll(bar_right_x,bar_right_y,bar_right_dx,bar_right_dy,pos_base,pos_rel_max,pos_base_upper + pos_rel_max);
		if (bar_left_dx)
			text_clear(bar_left_x,bar_left_y,bar_left_dx,bar_left_dy,COLOR_MENU_BAR >> 4);

		resource sound;
		if (!sound_find_preview(sound,rs,rs.current_game)) {
			sound = resource();
		}

		const game* effective_game = pos_base + pos_rel < gc.size() && gc[pos_base + pos_rel]->has_game() ? &gc[pos_base + pos_rel]->game_get() : 0;

		if (rs.mode_effective == mode_full_mixed || rs.mode_effective == mode_list_mixed) {
			bool game_horz = true;

			if (rs.mode_effective == mode_list_mixed && (rs.preview_effective == preview_snap || rs.preview_effective == preview_title)) {
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
				for(int i=0;i<backdrop_mac;++i)
					text_backdrop_pos(i,backdrop_map[i].x,backdrop_map[i].y,backdrop_map[i].dx,backdrop_map[i].dy);
			} else {
				for(int i=0;i<backdrop_mac;++i)
					text_backdrop_pos(i,backdrop_map_bis[i].x,backdrop_map_bis[i].y,backdrop_map_bis[i].dx,backdrop_map_bis[i].dy);
			}

			if (rs.preview_effective == preview_title) {
				backdrop_game_set(effective_game,0,preview_title,true,false,false,rs);
				backdrop_game_set(effective_game,1,preview_snap,false,false,true,rs);
				backdrop_game_set(effective_game,2,preview_flyer,false,false,false,rs);
				backdrop_game_set(effective_game,3,preview_cabinet,false,false,false,rs);
			} else if (rs.preview_effective == preview_flyer) {
				backdrop_game_set(effective_game,0,preview_flyer,true,false,false,rs);
				backdrop_game_set(effective_game,1,preview_snap,false,false,true,rs);
				backdrop_game_set(effective_game,2,preview_title,false,false,false,rs);
				backdrop_game_set(effective_game,3,preview_cabinet,false,false,false,rs);
			} else if (rs.preview_effective == preview_cabinet) {
				backdrop_game_set(effective_game,0,preview_cabinet,true,false,false,rs);
				backdrop_game_set(effective_game,1,preview_snap,false,false,true,rs);
				backdrop_game_set(effective_game,2,preview_title,false,false,false,rs);
				backdrop_game_set(effective_game,3,preview_flyer,false,false,false,rs);
			} else {
				backdrop_game_set(effective_game,0,preview_snap,true,false,true,rs);
				backdrop_game_set(effective_game,1,preview_title,false,false,false,rs);
				backdrop_game_set(effective_game,2,preview_flyer,false,false,false,rs);
				backdrop_game_set(effective_game,3,preview_cabinet,false,false,false,rs);
			}

		} else {
			if (backdrop_mac == 1) {
				backdrop_game_set(effective_game,0,effective_preview,true,false,true,rs);
			} else if (backdrop_mac > 1) {
				for(int i=0;i<coln*rown;++i) {
					bool current = i == pos_rel;
					backdrop_index_set(pos_base+i,gc,i,effective_preview,current,current,current,rs);
				}
			}
		}

		text_update(rs.mode_effective != mode_full_mixed && rs.mode_effective != mode_list_mixed);

		os_log(("menu: wait begin\n"));

		run_background_wait(rs, sound);

		os_log(("menu: wait end\n"));

		key = text_getkey(false);

		os_log(("menu: key %d\n",key));

		string oldfast = rs.fast;
		rs.fast.erase();

		if (key != TEXT_KEY_ESC)
			exit_count = 0;

		key = menu_key(key, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, gc.size());

		switch (key) {
			case TEXT_KEY_INS :
				if (pos_base + pos_rel < gc.size() && pos_base + pos_rel > 0) {
					unsigned new_pos = pos_base + pos_rel - 1;
					string i = gc[new_pos]->category(category_extract);
					while (new_pos>0 && gc[new_pos-1]->category(category_extract)== i)
						--new_pos;
					menu_pos(new_pos,pos_base,pos_rel,pos_rel_max,pos_base_upper,coln,gc.size());
				}
				break;
			case TEXT_KEY_DEL :
				if (pos_base + pos_rel < gc.size()) {
					unsigned new_pos = pos_base + pos_rel;
					string i = gc[new_pos]->category(category_extract);
					while (new_pos<gc.size()-1 && gc[new_pos]->category(category_extract)== i)
						++new_pos;
					menu_pos(new_pos,pos_base,pos_rel,pos_rel_max,pos_base_upper,coln,gc.size());
				}
				break;
			default:
				if (key<0xFF && isalnum(key)) {
					oldfast.insert(oldfast.length(),1,key);
					menu_array::const_iterator i;
					for(i=gc.begin();i!=gc.end();++i) {
						if (menu_fast_compare((*i)->desc_get(),oldfast))
							break;
					}
					if (i!=gc.end()) {
						int pos = i - gc.begin();
						menu_pos(pos,pos_base,pos_rel,pos_rel_max,pos_base_upper,coln,gc.size());
						rs.fast = oldfast;
					}
				}
				break;
			case TEXT_KEY_ENTER :
			case TEXT_KEY_RUN_CLONE :
			case TEXT_KEY_IDLE_0 :
			case TEXT_KEY_IDLE_1 :
			case TEXT_KEY_LOCK :
				done = true;
				break;
		}
		if (!rs.lock_effective)
		switch (key) {
			case TEXT_KEY_MODE :
			case TEXT_KEY_HELP :
			case TEXT_KEY_GROUP :
			case TEXT_KEY_TYPE :
			case TEXT_KEY_EXCLUDE :
			case TEXT_KEY_SORT :
			case TEXT_KEY_SETGROUP :
			case TEXT_KEY_SETTYPE :
			case TEXT_KEY_COMMAND :
			case TEXT_KEY_MENU :
			case TEXT_KEY_EMU :
			case TEXT_KEY_ROTATE :
			case TEXT_KEY_SPACE :
				done = true;
				break;
			case TEXT_KEY_ESC :
				++exit_count;
				if (rs.exit_count && exit_count >= rs.exit_count)
					done = true;
				break;
			case TEXT_KEY_OFF :
				done = true;
				break;
		}

		if (pos_rel + pos_base < gc.size() && gc[pos_rel + pos_base]->has_game()) {
			rs.current_game = &gc[pos_rel+pos_base]->game_get();
			rs.current_clone = 0;
		} else {
			rs.current_game = 0;
			rs.current_clone = 0;
		}
	}

	if (key == TEXT_KEY_IDLE_0) {
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

	delete [] text_map;
	delete [] backdrop_map;
	delete [] backdrop_map_bis;

	if (backdrop_mac > 0) {
		text_clip_done();
		text_backdrop_done();
	}

	return key;
}

int run_menu_idle(config_state& rs, menu_array& gc) {
	bool done = false;
	int key = 0;
	unsigned counter = 0;
	preview_t preview;

	switch (rs.idle_saver_type) {
		case saver_snap : preview = preview_snap; break;
		case saver_play : preview = preview_snap; break;
		case saver_flyer : preview = preview_flyer; break;
		case saver_cabinet : preview = preview_cabinet; break;
		case saver_title : preview = preview_title; break;
		default: preview = preview_snap; break;
	}

	text_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_CURSOR, 1, 0, 0, rs.preview_expand);
	text_clip_init();

	text_backdrop_pos(0,0,0,text_dx_get(),text_dy_get());

	// make the set of available snapshot
	vector<unsigned> avail;
	for(unsigned i=0;i<gc.size();++i) {
		resource backdrop;
		if (gc[i]->has_game()) {
			if (backdrop_find_preview_strict(backdrop, preview, &gc[i]->game_get(), rs.idle_saver_type == saver_play)) {
				avail.insert(avail.end(), i);
			}
		}
	}

	// randomize the set
	for(unsigned i=0;i<avail.size();++i) {
		unsigned j = rand() % avail.size();
		unsigned t = avail[i];
		avail[i] = avail[j];
		avail[j] = t;
	}

	while (!done) {
		unsigned pos;
		bool found;

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
				if (!sound_find_preview(sound,rs,&gc[pos]->game_get())) {
					sound = resource();
				}
			}

			text_update_pre();

			draw_menu_info(gc[pos]->game_get().description_get(), counter);

			text_update_post();
		} else {
			backdrop_game_set(0, 0, preview, false, false, true, rs);

			text_update();
		}

		// next game
		++counter;

		run_background_wait(rs,sound);

		key = text_getkey(false);

		if (key != TEXT_KEY_IDLE_1) {
			done = true;

			// select the game if the user press enter
			if (found && key == TEXT_KEY_ENTER) {
				rs.current_game = &gc[pos]->game_get();
			}
		}
	}

	text_clip_done();
	text_backdrop_done();

	return key;
}

int run_menu_idle_off() {
	bool done = false;
	int key = 0;

	text_clear();

	os_apm_standby();

	while (!done) {

		text_update();

		key = text_getkey(false);

		if (key != TEXT_KEY_IDLE_1)
			done = true;
	}

	os_apm_wakeup();

	return key;
}

int run_menu_sort(config_state& rs, const pgame_sort_set& gss, sort_item_func* category_func, bool flipxy) {
	menu_array gc;

	os_log(("menu: insert begin\n"));

	bool list_mode = rs.mode_effective == mode_list || rs.mode_effective == mode_list_mixed;
	if (!list_mode || rs.sort_effective == sort_by_name || rs.sort_effective == sort_by_time || rs.sort_effective == sort_by_size || rs.sort_effective == sort_by_coin) {
		for(pgame_sort_set::const_iterator i = gss.begin();i!=gss.end();++i) {
			gc.insert(gc.end(),new menu_entry(*i,0));
		}
	} else if (rs.sort_effective == sort_by_root_name) {
		for(pgame_sort_set::const_iterator i = gss.begin();i!=gss.end();++i) {
			unsigned ident = 0;
			if ((*i)->parent_get()) {
				if ((*i)->software_get())
					ident += 2;
				else
					ident += 1;
			}
			gc.insert(gc.end(),new menu_entry(*i,ident * text_font_dx_get()));
		}
	} else {
		string category = "<>";
		for(pgame_sort_set::const_iterator i = gss.begin();i!=gss.end();++i) {
			string new_category = category_func(**i);
			if (new_category != category) {
				category = new_category;
				gc.insert(gc.end(),new menu_entry(category));
			}
			gc.insert(gc.end(),new menu_entry(*i,text_font_dx_get()));
		}
	}

	os_log(("menu: insert end\n"));

	bool done = false;
	int key = 0;
	bool idle = false;

	while (!done) {
		if (idle) {
			if (rs.restore == restore_idle)
				config_restore_load(rs);
			if (rs.idle_saver_type == saver_off)
				run_menu_idle_off();
			else {
				key = run_menu_idle(rs,gc);
				if (key == TEXT_KEY_ENTER)
					done = true;
			}
			idle = false;
		}

		if (!done) {
			key = run_menu_user(rs,flipxy,gc,category_func);

			switch (key) {
				case TEXT_KEY_IDLE_1 :
					idle = true;
					break;
				default:
					done = true;
			}
		}
	}

	for(menu_array::iterator i=gc.begin();i!=gc.end();++i)
		delete *i;

	return key;
}

int run_menu(config_state& rs, bool flipxy) {
	pgame_sort_set* psc;
	sort_item_func* category_func;

	os_log(("menu: sort begin\n"));

	// sort
	switch (rs.sort_effective) {
		case sort_by_root_name :
			psc = new pgame_sort_set(sort_by_root_name_func);
			category_func = sort_item_root_name;
			break;
		case sort_by_name :
			psc = new pgame_sort_set(sort_by_name_func);
			category_func = sort_item_name;
			break;
		case sort_by_manufacturer :
			psc = new pgame_sort_set(sort_by_manufacturer_func);
			category_func = sort_item_manufacturer;
			break;
		case sort_by_year :
			psc = new pgame_sort_set(sort_by_year_func);
			category_func = sort_item_year;
			break;
		case sort_by_time :
			if (rs.exclude_clone_effective == exclude)
				psc = new pgame_sort_set(sort_by_time_tree_func);
			else
				psc = new pgame_sort_set(sort_by_time_func);
			category_func = sort_item_time;
			break;
		case sort_by_coin :
			if (rs.exclude_clone_effective == exclude)
				psc = new pgame_sort_set(sort_by_coin_tree_func);
			else
				psc = new pgame_sort_set(sort_by_coin_func);
			category_func = sort_item_coin;
			break;
		case sort_by_group :
			psc = new pgame_sort_set(sort_by_group_func);
			category_func = sort_item_group;
			break;
		case sort_by_type :
			psc = new pgame_sort_set(sort_by_type_func);
			category_func = sort_item_type;
			break;
		case sort_by_size :
			psc = new pgame_sort_set(sort_by_size_func);
			category_func = sort_item_size;
			break;
		case sort_by_res :
			psc = new pgame_sort_set(sort_by_res_func);
			category_func = sort_item_res;
			break;
		default:
			return TEXT_KEY_NONE;
	}

	// recompute the preview mask
	rs.preview_mask = 0;

	for(game_set::const_iterator i=rs.gar.begin();i!=rs.gar.end();++i) {
		// emulator
		if (rs.include_emu_effective.size()) {
			bool accept_emu = false;
			for(emulator_container::iterator j=rs.include_emu_effective.begin();j!=rs.include_emu_effective.end();++j) {
				if (*j == i->emulator_get()->user_name_get()) {
					accept_emu = true;
					break;
				}
			}
			if (!accept_emu)
				continue;
		}

		// group
		if (rs.include_group_effective.size()) {
			category_container::iterator j = rs.include_group_effective.find(i->group_derived_get());
			if (j == rs.include_group_effective.end())
				continue;
		}

		// type
		if (rs.include_type_effective.size()) {
			category_container::iterator j = rs.include_type_effective.find(i->type_derived_get());
			if (j == rs.include_type_effective.end())
				continue;
		}

		const game& bios = i->bios_get();

		// software of clones is always listed
		if (!i->software_get()) {
			if (rs.exclude_clone_effective == exclude && bios.parent_get()!=0)
				continue;
		}

		if (rs.exclude_clone_effective == exclude_not && bios.parent_get()==0)
			continue;

		if ((rs.exclude_bad_effective == exclude || rs.exclude_bad_effective == exclude_not)) {
			bool good;
			if (rs.exclude_clone_effective == exclude)
				good = bios.clone_best_get().is_good();
			else
				good = bios.is_good();
			if (rs.exclude_bad_effective == exclude && !good)
				continue;
			if (rs.exclude_bad_effective == exclude_not && good)
				continue;
		}
		if (rs.exclude_vertical_effective == exclude && bios.vertical_get())
			continue;
		if (rs.exclude_vertical_effective == exclude_not && !bios.vertical_get())
			continue;
		if (rs.exclude_neogeo_effective == exclude && rs.gar.is_game_tag(bios.name_get(),"neogeo"))
			continue;
		if (rs.exclude_neogeo_effective == exclude_not && !rs.gar.is_game_tag(bios.name_get(),"neogeo"))
			continue;
		if (rs.exclude_deco_effective == exclude && rs.gar.is_game_tag(bios.name_get(),"decocass"))
			continue;
		if (rs.exclude_deco_effective == exclude_not && !rs.gar.is_game_tag(bios.name_get(),"decocass"))
			continue;
		if (rs.exclude_playchoice_effective == exclude && rs.gar.is_game_tag(bios.name_get(),"playch10"))
			continue;
		if (rs.exclude_playchoice_effective == exclude_not && !rs.gar.is_game_tag(bios.name_get(),"playch10"))
			continue;
		if (rs.exclude_vector_effective == exclude && bios.vector_get())
			continue;
		if (rs.exclude_vector_effective == exclude_not && !bios.vector_get())
			continue;
		if ((rs.exclude_missing_effective == exclude || rs.exclude_missing_effective == exclude_not)) {
			bool present = rs.gar.is_tree_rom_of_present(i->name_get(),rs.merge);
			if (rs.exclude_missing_effective == exclude && !present)
				continue;
			if (rs.exclude_missing_effective == exclude_not && present)
				continue;
		}

		// is a resource, not a game
		if (i->resource_get())
			continue;

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

	rs.mode_mask = ~rs.mode_skip_mask & (mode_full | mode_full_mixed
		| mode_text | mode_list | mode_list_mixed | mode_tile_small
		| mode_tile_normal | mode_tile_big | mode_tile_enormous
		| mode_tile_giant | mode_tile_icon | mode_tile_marquee);
	if ((rs.preview_mask & preview_icon) == 0)
		rs.mode_mask &= ~mode_tile_icon;
	if ((rs.preview_mask & preview_marquee) == 0)
		rs.mode_mask &= ~mode_tile_marquee;
	rs.preview_mask &= ~(preview_icon | preview_marquee);
	if (rs.preview_mask == 0)
		rs.mode_mask = mode_text;
	if (!rs.mode_mask)
		rs.mode_mask = mode_text;

	os_log(("menu: sort end\n"));

	bool done = false;
	int key = 0;

	while (!done) {
		key = run_menu_sort(rs,*psc,category_func,flipxy);

		if (!rs.lock_effective)
		switch (key) {
			case TEXT_KEY_MODE :
				if (rs.mode_mask) {
					do {
						switch (rs.mode_effective) {
							case mode_full : rs.mode_effective = mode_full_mixed; break;
							case mode_full_mixed : rs.mode_effective = mode_text; break;
							case mode_text : rs.mode_effective = mode_list; break;
							case mode_list : rs.mode_effective = mode_list_mixed; break;
							case mode_list_mixed : rs.mode_effective = mode_tile_small; break;
							case mode_tile_small : rs.mode_effective = mode_tile_normal; break;
							case mode_tile_normal : rs.mode_effective = mode_tile_big; break;
							case mode_tile_big : rs.mode_effective = mode_tile_enormous; break;
							case mode_tile_enormous : rs.mode_effective = mode_tile_giant; break;
							case mode_tile_giant : rs.mode_effective = mode_tile_icon; break;
							case mode_tile_icon : rs.mode_effective = mode_tile_marquee; break;
							case mode_tile_marquee : rs.mode_effective = mode_full; break;
						}
					} while ((rs.mode_effective & rs.mode_mask) == 0);
				}
				break;
			case TEXT_KEY_SPACE :
				if (rs.preview_mask) {
					do {
						switch (rs.preview_effective) {
							case preview_icon :
							case preview_marquee :
							case preview_snap : rs.preview_effective = preview_title; break;
							case preview_title : rs.preview_effective = preview_flyer; break;
							case preview_flyer : rs.preview_effective = preview_cabinet; break;
							case preview_cabinet : rs.preview_effective = preview_snap; break;
						}
					} while ((rs.preview_effective & rs.preview_mask) == 0);
				}
				break;
		}
		switch (key) {
			case TEXT_KEY_ENTER :
			case TEXT_KEY_RUN_CLONE :
			case TEXT_KEY_IDLE_0 :
			case TEXT_KEY_IDLE_1 :
			case TEXT_KEY_LOCK :
			case TEXT_KEY_HELP :
			case TEXT_KEY_GROUP :
			case TEXT_KEY_TYPE :
			case TEXT_KEY_EXCLUDE :
			case TEXT_KEY_SORT :
			case TEXT_KEY_SETGROUP :
			case TEXT_KEY_SETTYPE :
			case TEXT_KEY_COMMAND :
			case TEXT_KEY_MENU :
			case TEXT_KEY_EMU :
			case TEXT_KEY_ROTATE :
			case TEXT_KEY_ESC :
			case TEXT_KEY_OFF :
				done = true;
				break;
		}
	}

	delete psc;

	return key;
}

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

	choice_bag::iterator i = ch.find_by_value(rs.sort_effective);
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Sort mode", SORT_CHOICE_X, SORT_CHOICE_Y, SORT_CHOICE_DX, i);
	if (key == TEXT_KEY_ENTER) {
		rs.sort_effective = (game_sort_t)i->value_get();
	}
}

// ------------------------------------------------------------------------
// Exclude menu

#define EXCLUDE_CHOICE_X text_dx_get()/8
#define EXCLUDE_CHOICE_Y text_dy_get()/5
#define EXCLUDE_CHOICE_DX 20*text_font_dx_get()

void run_exclude(config_state& rs) {
	choice_bag ch;

	ch.insert( ch.end(), choice("Working or Preliminary"," Only\tWorking"," Only\tPreliminary", rs.exclude_bad_effective, 0) );
	ch.insert( ch.end(), choice("Present or Missing"," Only\tPresent"," Only\tMissing", rs.exclude_missing_effective, 0) );
	ch.insert( ch.end(), choice("Parent or Clone"," Only\tParent"," Only\tClone", rs.exclude_clone_effective, 0) );
	ch.insert( ch.end(), choice("Any Screen Type", " Only\tScreen Raster", " Only\tScreen Vector", rs.exclude_vector_effective, 0) );
	ch.insert( ch.end(), choice("Any Orientation", " Only\tHorizontal", " Only\tVertical", rs.exclude_vertical_effective, 0) );
	ch.insert( ch.end(), choice("Neogeo", rs.exclude_neogeo_effective, 0) );
	ch.insert( ch.end(), choice("Cassette", rs.exclude_deco_effective, 0) );
	ch.insert( ch.end(), choice("PlayChoice-10", rs.exclude_playchoice_effective, 0) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" Show attrib", EXCLUDE_CHOICE_X, EXCLUDE_CHOICE_Y, EXCLUDE_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.exclude_bad_effective = ch[0].tristate_get();
		rs.exclude_missing_effective = ch[1].tristate_get();
		rs.exclude_clone_effective = ch[2].tristate_get();
		rs.exclude_vector_effective = ch[3].tristate_get();
		rs.exclude_vertical_effective = ch[4].tristate_get();
		rs.exclude_neogeo_effective = ch[5].tristate_get();
		rs.exclude_deco_effective = ch[6].tristate_get();
		rs.exclude_playchoice_effective = ch[7].tristate_get();
	}
}

// ------------------------------------------------------------------------
// Command menu

#define COMMAND_CHOICE_X text_dx_get()/8
#define COMMAND_CHOICE_Y text_dy_get()/5
#define COMMAND_CHOICE_DX 33*text_font_dx_get()

void run_command(config_state& rs) {
	choice_bag ch;

	if (rs.current_game && rs.current_game->preview_snap_get().is_deletable())
		ch.insert( ch.end(), choice("Delete game snapshot", 0) );
	if (rs.current_game && rs.current_game->preview_clip_get().is_deletable())
		ch.insert( ch.end(), choice("Delete game clip", 1) );
	if (rs.current_game && rs.current_game->preview_flyer_get().is_deletable())
		ch.insert( ch.end(), choice("Delete game flyer", 2) );
	if (rs.current_game && rs.current_game->preview_cabinet_get().is_deletable())
		ch.insert( ch.end(), choice("Delete game cabinet", 3) );
	if (rs.current_game && rs.current_game->preview_icon_get().is_deletable())
		ch.insert( ch.end(), choice("Delete game icon", 4) );
	if (rs.current_game && rs.current_backdrop.is_deletable())
		ch.insert( ch.end(), choice("Delete current image", 5) );
	if (rs.current_game && rs.current_game->preview_sound_get().is_deletable())
		ch.insert( ch.end(), choice("Delete game sound", 6) );
	if (rs.current_game && rs.current_sound.is_deletable())
		ch.insert( ch.end(), choice("Delete current sound", 7) );

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

	for(category_container::const_iterator j = rs.group.begin();j!=rs.group.end();++j) {
		bool tag = rs.include_group_effective.find(*j) != rs.include_group_effective.end();
		ch.insert( ch.end(), choice(*j, tag, 0) );
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

	for(category_container::const_iterator j = rs.type.begin();j!=rs.type.end();++j) {
		bool tag = rs.include_type_effective.find(*j) != rs.include_type_effective.end();
		ch.insert( ch.end(), choice(*j, tag, 0) );
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

	for(category_container::const_iterator j = rs.group.begin();j!=rs.group.end();++j) {
		ch.insert( ch.end(), choice(*j,0) );
	}

	choice_bag::iterator i = ch.find_by_desc(rs.current_game->group_get());
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Select group", GROUP_CHOICE_X, GROUP_CHOICE_Y, GROUP_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.current_game->user_group_set( i->desc_get() );
	}
}

void run_type_move(config_state& rs) {
	choice_bag ch;

	if (!rs.current_game)
		return;

	for(category_container::const_iterator j = rs.type.begin();j!=rs.type.end();++j) {
		ch.insert( ch.end(), choice(*j, 0) );
	}

	choice_bag::iterator i = ch.find_by_desc(rs.current_game->type_get());
	if (i==ch.end())
		i = ch.begin();
	int key = ch.run(" Select type", TYPE_CHOICE_X, TYPE_CHOICE_Y, TYPE_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		rs.current_game->user_type_set( i->desc_get() );
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

	os_joy_calib_start();

	while (1) {
		const char* ope = os_joy_calib_next();

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
// Menu

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
				run_exclude(rs);
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
				config_restore_save(rs);
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
	text_put(2*text_font_dx_get(),0,"HELP",COLOR_MENU_BAR);

	int y = 2*text_font_dy_get();
	int xt = 1*text_font_dx_get();
	int xd = 8*text_font_dx_get();
	text_put(xt,y,"F1",COLOR_HELP_TAG);
	text_put(xd,y,"Help screen",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
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
	text_put(xd,y,"Exit",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
	text_put(xt,y,"CTRL+ESC",COLOR_HELP_TAG);
	text_put(xd,y,"Shutdown",COLOR_HELP_NORMAL);
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
#if 0
	text_put(xt,y,"F7",COLOR_HELP_TAG);
	text_put(xd,y,"Configuration",COLOR_HELP_NORMAL);
	y += text_font_dy_get();
#endif
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

// ------------------------------------------------------------------------
// Run Info

#define RUNINFO_CHOICE_X text_dx_get()/8
#define RUNINFO_CHOICE_Y text_dy_get()/10
#define RUNINFO_CHOICE_DX text_dx_get()*3/4
#define RUNINFO_CHOICE_DY 2*text_font_dy_get()

void run_runinfo(config_state& rs) {
	int x = RUNINFO_CHOICE_X;
	int y = RUNINFO_CHOICE_Y;
	int dx = RUNINFO_CHOICE_DX;
	int dy = RUNINFO_CHOICE_DY;
	int border = text_font_dx_get()/2;

	const game* g = rs.current_clone ? rs.current_clone : rs.current_game;
	if (!g)
		return;

	text_box(x-border,y-border,dx+2*border,dy+border*2,1,COLOR_CHOICE_NORMAL);
	text_clear(x-border+1,y-border+1,dx+2*border-2,dy+border*2-2,COLOR_CHOICE_NORMAL >> 4);

	text_put(x,y,dx,rs.msg_run_game,COLOR_CHOICE_TITLE);
	y += text_font_dy_get();

	text_put(x,y,dx,g->description_get(),COLOR_CHOICE_NORMAL);
	y += text_font_dy_get();

	text_update();
}



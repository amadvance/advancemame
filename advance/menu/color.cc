/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

#include "color.h"

#include <sstream>
#include <iomanip>

using namespace std;

int_color COLOR_HELP_NORMAL = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_HELP_TAG = { { 0xF0, 0x7e, 0x24 }, { 255, 255, 255 } };
int_color COLOR_CHOICE_TITLE = { { 0xF0, 0x7e, 0x24 }, { 255, 255, 255 } };
int_color COLOR_CHOICE_NORMAL = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_CHOICE_SELECT = { { 0, 0, 0 }, { 0xFF, 0xFF, 0xBF } };
int_color COLOR_CHOICE_HIDDEN = { { 128, 128, 128 }, { 255, 255, 255 } };
int_color COLOR_CHOICE_HIDDEN_SELECT = { { 128, 128, 128 }, { 0xFF, 0xFF, 0xBF } };
int_color COLOR_MENU_NORMAL = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_MENU_HIDDEN = { { 128, 128, 128 }, { 255, 255, 255 } };
int_color COLOR_MENU_TAG = { { 0xF0, 0x7e, 0x24 }, { 255, 255, 255 } };
int_color COLOR_MENU_SELECT = { { 0, 0, 0 }, { 0xFF, 0xFF, 0xBF } };
int_color COLOR_MENU_HIDDEN_SELECT = { { 128, 128, 128 }, { 0xFF, 0xFF, 0xBF } };
int_color COLOR_MENU_TAG_SELECT = { { 0xF0, 0x7e, 0x24 }, { 0xFF, 0xFF, 0xBF } };
int_color COLOR_MENU_BAR = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_MENU_BAR_TAG = { { 0xF0, 0x7e, 0x24 }, { 255, 255, 255 } };
int_color COLOR_MENU_BAR_HIDDEN = { { 128, 128, 128 }, { 255, 255, 255 } };
int_color COLOR_MENU_GRID = { { 0xF0, 0x7e, 0x24 }, { 255, 255, 255 } };
int_color COLOR_MENU_BACKDROP = { { 0, 0, 0 }, { 128, 128, 128 } };
int_color COLOR_MENU_ICON = { { 255, 255, 255 }, { 255, 255, 255 } };
int_color COLOR_MENU_CURSOR = { { 128, 128, 128 }, { 255, 255, 255 } };

static struct {
	int_color* var;
	const char* name;
} COLOR_TAB[] = {
{ &COLOR_HELP_NORMAL, "help" },
{ &COLOR_HELP_TAG, "help_tag" },
{ &COLOR_CHOICE_TITLE, "submenu_bar" },
{ &COLOR_CHOICE_NORMAL, "submenu_item" },
{ &COLOR_CHOICE_SELECT, "submenu_item_select" },
{ &COLOR_CHOICE_HIDDEN, "submenu_hidden" },
{ &COLOR_CHOICE_HIDDEN_SELECT, "submenu_hidden_select" },
{ &COLOR_MENU_NORMAL, "menu_item" },
{ &COLOR_MENU_HIDDEN, "menu_hidden" },
{ &COLOR_MENU_TAG, "menu_tag" },
{ &COLOR_MENU_SELECT, "menu_item_select" },
{ &COLOR_MENU_HIDDEN_SELECT, "menu_hidden_select" },
{ &COLOR_MENU_TAG_SELECT, "menu_tag_select" },
{ &COLOR_MENU_BAR, "bar" },
{ &COLOR_MENU_BAR_TAG, "bar_tag" },
{ &COLOR_MENU_BAR_HIDDEN, "bar_hidden" },
{ &COLOR_MENU_GRID, "grid" },
{ &COLOR_MENU_BACKDROP, "backdrop" },
{ &COLOR_MENU_ICON, "icon" },
{ &COLOR_MENU_CURSOR, "cursor" },
{ 0, 0 }
};

static struct color_name {
	const char* name;
	adv_color_rgb rgb;
}  COLOR_NAME[] = {
{ "black", { 0, 0, 0 } },
{ "blue", { 192, 0, 0 } },
{ "green", { 0, 192, 0 } },
{ "cyan", { 192, 192, 0 } },
{ "red", { 0, 0, 192 } },
{ "magenta", { 192, 0, 192 } },
{ "brown", { 0, 192, 192 } },
{ "lightgray", { 192, 192, 192 } },
{ "gray", { 128, 128, 128 } },
{ "lightblue", { 255, 0, 0 } },
{ "lightgreen", { 0, 255, 0 } },
{ "lightcyan", { 255, 255, 0 } },
{ "lightred", { 0, 0, 255 } },
{ "lightmagenta", { 255, 0, 255 } },
{ "yellow", { 0, 255, 255 } },
{ "white", { 255, 255, 255 } }
};

static unsigned hexdigit2int(char c)
{
	if (c>='A' && c<='F')
		return c - 'A' + 10;
	if (c>='a' && c<='f')
		return c - 'a' + 10;
	if (c>='0' && c<='9')
		return c - '0';
	return 0;
}

static unsigned hexnibble2int(char c0, char c1)
{
	return hexdigit2int(c0) * 16 + hexdigit2int(c1);
}

static adv_color_rgb string2color(const string& s)
{
	for(unsigned i=0;i<16;++i)
		if (s == COLOR_NAME[i].name)
			return COLOR_NAME[i].rgb;

	if (s.length() == 6 && s.find_first_not_of("0123456789abcdefABCDEF") == string::npos) {
		adv_color_rgb c;
		c.red = hexnibble2int(s[0], s[1]);
		c.green = hexnibble2int(s[2], s[3]);
		c.blue = hexnibble2int(s[4], s[5]);
		return c;
	}

	return COLOR_NAME[0].rgb;
}

static string color2string(const adv_color_rgb& c)
{
	ostringstream s;

	s << setfill('0') << setw(2) << hex << (unsigned)c.red;
	s << setfill('0') << setw(2) << hex << (unsigned)c.green;
	s << setfill('0') << setw(2) << hex <<(unsigned)c.blue;

	return s.str();
}

bool color_in(const string& s)
{
	string sname;
	string sarg0;
	string sarg1;
	unsigned i = 0;

	while (i < s.length() && !isspace(s[i])) {
		sname += s[i];
		++i;
	}
	
	while (i < s.length() && isspace(s[i]))
		++i;

	while (i < s.length() && !isspace(s[i])) {
		sarg0 += s[i];
		++i;
	}
	
	while (i < s.length() && isspace(s[i]))
		++i;

	while (i < s.length() && !isspace(s[i])) {
		sarg1 += s[i];
		++i;
	}

	while (i < s.length() && isspace(s[i]))
		++i;

	if (i != s.length())
		return false;

	for(i=0;COLOR_TAB[i].name;++i) {
		if (COLOR_TAB[i].name == sname)
			break;
	}

	if (!COLOR_TAB[i].name)
		return false;

	COLOR_TAB[i].var->foreground = string2color(sarg0);
	COLOR_TAB[i].var->background = string2color(sarg1);

	return true;
}

void color_out(adv_conf* config_context, const char* tag)
{
	for(unsigned i=0;COLOR_TAB[i].name;++i) {
		string s;
		s += COLOR_TAB[i].name;
		s += " ";
		s += color2string(COLOR_TAB[i].var->foreground);
		s += " ";
		s += color2string(COLOR_TAB[i].var->background);
		conf_set(config_context, "", tag, s.c_str());
	}
}

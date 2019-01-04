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

#include "advance.h"

#include <sstream>
#include <iomanip>

using namespace std;

#define C_SELECT { 0xFF, 0xFF, 0xAF }
#define C_NORMAL { 0, 0, 0 }
#define C_TITLE { 0xF0, 0x7e, 0x24 }
#define C_BACK { 255, 255, 255 }
#define C_HIDDEN { 128, 128, 128 }

int_color COLOR_HELP_NORMAL = { C_NORMAL, C_BACK };
int_color COLOR_HELP_TAG = { C_TITLE, C_BACK };
int_color COLOR_CHOICE_TITLE = { C_TITLE, C_BACK };
int_color COLOR_CHOICE_NORMAL = { C_NORMAL, C_BACK };
int_color COLOR_CHOICE_SELECT = { C_NORMAL, C_SELECT };
int_color COLOR_CHOICE_HIDDEN = { C_HIDDEN, C_BACK };
int_color COLOR_CHOICE_HIDDEN_SELECT = { C_HIDDEN, C_SELECT };
int_color COLOR_MENU_NORMAL = { C_NORMAL, C_BACK };
int_color COLOR_MENU_HIDDEN = { C_HIDDEN, C_BACK };
int_color COLOR_MENU_TAG = { C_TITLE, C_BACK };
int_color COLOR_MENU_SELECT = { C_NORMAL, C_SELECT };
int_color COLOR_MENU_HIDDEN_SELECT = { C_HIDDEN, C_SELECT };
int_color COLOR_MENU_TAG_SELECT = { C_TITLE, C_SELECT };
int_color COLOR_MENU_BAR = { C_NORMAL, C_BACK };
int_color COLOR_MENU_BAR_TAG = { C_TITLE, C_BACK };
int_color COLOR_MENU_BAR_HIDDEN = { C_HIDDEN, C_BACK };
int_color COLOR_MENU_GRID = { C_TITLE, C_BACK };
int_color COLOR_MENU_OVERSCAN = { C_NORMAL, C_BACK };
int_color COLOR_MENU_BACKDROP = { C_NORMAL, C_HIDDEN };
int_color COLOR_MENU_ICON = { C_BACK, C_BACK };
int_color COLOR_MENU_CURSOR = { C_HIDDEN, C_BACK };

static struct {
	int_color* var;
	const char* name;
	bool foreground;
} COLOR_TAB[] = {
	{ &COLOR_HELP_NORMAL, "help", false },
	{ &COLOR_HELP_TAG, "help_tag", false },
	{ &COLOR_CHOICE_TITLE, "submenu_bar", false },
	{ &COLOR_CHOICE_NORMAL, "submenu_item", false },
	{ &COLOR_CHOICE_SELECT, "submenu_item_select", true },
	{ &COLOR_CHOICE_HIDDEN, "submenu_hidden", false },
	{ &COLOR_CHOICE_HIDDEN_SELECT, "submenu_hidden_select", true },
	{ &COLOR_MENU_NORMAL, "menu_item", false },
	{ &COLOR_MENU_HIDDEN, "menu_hidden", false },
	{ &COLOR_MENU_TAG, "menu_tag", false },
	{ &COLOR_MENU_SELECT, "menu_item_select", true },
	{ &COLOR_MENU_HIDDEN_SELECT, "menu_hidden_select", true },
	{ &COLOR_MENU_TAG_SELECT, "menu_tag_select", true },
	{ &COLOR_MENU_BAR, "bar", false },
	{ &COLOR_MENU_BAR_TAG, "bar_tag", false },
	{ &COLOR_MENU_BAR_HIDDEN, "bar_hidden", false },
	{ &COLOR_MENU_GRID, "grid", false },
	{ &COLOR_MENU_OVERSCAN, "overscan", false },
	{ &COLOR_MENU_BACKDROP, "backdrop", false },
	{ &COLOR_MENU_ICON, "icon", false },
	{ &COLOR_MENU_CURSOR, "cursor", false },
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
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= '0' && c <= '9')
		return c - '0';
	return 0;
}

static unsigned hexnibble2int(char c0, char c1)
{
	return hexdigit2int(c0) * 16 + hexdigit2int(c1);
}

static adv_color_rgb string2color(const string& s)
{
	for (unsigned i = 0; i < 16; ++i)
		if (s == COLOR_NAME[i].name)
			return COLOR_NAME[i].rgb;

	if (s.length() == 6 && s.find_first_not_of("0123456789abcdefABCDEF") == string::npos) {
		adv_color_rgb c;
		c.red = hexnibble2int(s[0], s[1]);
		c.green = hexnibble2int(s[2], s[3]);
		c.blue = hexnibble2int(s[4], s[5]);
		c.alpha = 255;
		return c;
	}

	return COLOR_NAME[0].rgb;
}

static string color2string(const adv_color_rgb& c)
{
	ostringstream s;

	s << setfill('0') << setw(2) << hex << (unsigned)c.red;
	s << setfill('0') << setw(2) << hex << (unsigned)c.green;
	s << setfill('0') << setw(2) << hex << (unsigned)c.blue;

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

	for (i = 0; COLOR_TAB[i].name; ++i) {
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
	for (unsigned i = 0; COLOR_TAB[i].name; ++i) {
		string s;
		s += COLOR_TAB[i].name;
		s += " ";
		s += color2string(COLOR_TAB[i].var->foreground);
		s += " ";
		s += color2string(COLOR_TAB[i].var->background);
		conf_set(config_context, "", tag, s.c_str());
	}
}

static void color_map(int_color& color, adv_color_def opaque_def, adv_color_def alpha_def, unsigned translucency)
{
	color.foreground.alpha = 255;
	color.background.alpha = translucency;

	for (int i = 0; i < 256; ++i) {
		int cr, cg, cb, ca;

		cr = (color.foreground.red * i + color.background.red * (255 - i)) / 255;
		cg = (color.foreground.green * i + color.background.green * (255 - i)) / 255;
		cb = (color.foreground.blue * i + color.background.blue * (255 - i)) / 255;
		ca = 255;

		color.opaque[i] = alpha_make_from_def(cr, cg, cb, ca, opaque_def);

		// see the AdvanceMAME ui.c file for details on the computation

		double F, B, A;
		double I, T;
		double a, b, c;

		I = i / 255.0;
		T = translucency / 255.0;

		a = I;
		b = T * (1 - I);
		c = (1 - T) * (1 - I);

		A = 1 - c;
		if (A == 0) {
			F = 0;
			B = 0;
		} else {
			F = a / A;
			B = b / A;
		}

		cr = static_cast<int>(color.foreground.red * F + color.background.red * B);
		cg = static_cast<int>(color.foreground.green * F + color.background.green * B);
		cb = static_cast<int>(color.foreground.blue * F + color.background.blue * B);
		ca = static_cast<int>(255 * A);

		if (cr > 255)
			cr = 255;
		if (cg > 255)
			cg = 255;
		if (cb > 255)
			cb = 255;
		if (ca > 255)
			ca = 255;

		color.alpha[i] = alpha_make_from_def(cr, cg, cb, ca, alpha_def);
	}
}

void color_setup(adv_color_def opaque_def, adv_color_def alpha_def, unsigned translucency)
{
	for (unsigned i = 0; COLOR_TAB[i].name; ++i) {
		unsigned t;
		if (COLOR_TAB[i].foreground)
			t = 255;
		else
			t = translucency;
		color_map(*COLOR_TAB[i].var, opaque_def, alpha_def, t);
		log_std(("menu: color %s foreground:%s background:%s opaque:%08x->%08x alpha:%08x->%08x\n", COLOR_TAB[i].name,
			color2string(COLOR_TAB[i].var->foreground).c_str(), color2string(COLOR_TAB[i].var->background).c_str(),
			COLOR_TAB[i].var->opaque[0], COLOR_TAB[i].var->opaque[255],
			COLOR_TAB[i].var->alpha[0], COLOR_TAB[i].var->alpha[255]
			));
	}
}


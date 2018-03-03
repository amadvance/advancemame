/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004 Andrea Mazzoleni
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

#ifndef __COLOR_H
#define __COLOR_H

#include "conf.h"
#include "rgb.h"

#include <string>

struct int_color {
	adv_color_rgb foreground;
	adv_color_rgb background;
	adv_pixel alpha[256];
	adv_pixel opaque[256];
};

extern int_color COLOR_HELP_NORMAL;
extern int_color COLOR_HELP_TAG;
extern int_color COLOR_CHOICE_TITLE;
extern int_color COLOR_CHOICE_NORMAL;
extern int_color COLOR_CHOICE_HIDDEN;
extern int_color COLOR_CHOICE_SELECT;
extern int_color COLOR_CHOICE_HIDDEN_SELECT;
extern int_color COLOR_MENU_NORMAL;
extern int_color COLOR_MENU_HIDDEN;
extern int_color COLOR_MENU_TAG;
extern int_color COLOR_MENU_SELECT;
extern int_color COLOR_MENU_HIDDEN_SELECT;
extern int_color COLOR_MENU_TAG_SELECT;
extern int_color COLOR_MENU_BAR;
extern int_color COLOR_MENU_BAR_TAG;
extern int_color COLOR_MENU_BAR_HIDDEN;
extern int_color COLOR_MENU_GRID;
extern int_color COLOR_MENU_OVERSCAN;
extern int_color COLOR_MENU_BACKDROP;
extern int_color COLOR_MENU_ICON;
extern int_color COLOR_MENU_CURSOR;

bool color_in(const std::string& s);
void color_out(adv_conf* config_context, const char* tag);
void color_setup(adv_color_def opaque_def, adv_color_def alpha_def, unsigned translucency);

#endif


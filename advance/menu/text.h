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

#ifndef __TEXT_H
#define __TEXT_H

#include "resource.h"
#include "conf.h"
#include "bitmap.h"

#include <stdlib.h>
#include <time.h>

#include <string>
#include <iostream>

// -------------------------------------------------------------------------
// Interface

struct int_color {
	adv_color_rgb foreground;
	adv_color_rgb background;
};

void int_reg(adv_conf* config_context);
void int_unreg();
bool int_load(adv_conf* config_context);
bool int_init(unsigned video_size, const std::string& sound_event_key);
void int_done();
bool int_set(double gamma, double brightness, unsigned idle_0, unsigned idle_0_rep, unsigned idle_1, unsigned idle_1_rep, unsigned repeat, unsigned repeat_rep, bool backdrop_fast, bool alpha_mode);
void int_unset(bool reset_video_mode);
bool int_enable(int fontx, int fonty, const std::string& font, unsigned orientation);
void int_disable();
void int_unplug();
void int_plug();

bool int_image(const char* file, unsigned& size_x, unsigned& size_y);
void int_clear();
void int_clear(int x, int y, int dx, int dy, const adv_color_rgb& color);
void int_box(int x, int y, int dx, int dy, int width, const adv_color_rgb& color);
void int_rotate(int& x, int& y, int& dx, int& dy);
void int_invrotate(int& x, int& y, int& dx, int& dy);

void int_put(int x, int y, char c, const int_color& color);
unsigned int_put_width(char c);
void int_put(int x, int y, const std::string& s, const int_color& color);
unsigned int_put_width(const std::string& s);
unsigned int_put(int x, int y, int dx, const std::string& s, const int_color& color);
void int_put_filled(int x, int y, int dx, const std::string& s, const int_color& color);
void int_put_special(bool& in, int x, int y, int dx, const std::string& s, const int_color& c0, const int_color& c1, const int_color& c2);
unsigned int_put_right(int x, int y, int dx, const std::string& s, const int_color& color);

void int_backdrop_init(const int_color& back_color, const int_color& back_box_color, unsigned Amac, unsigned Ainc, unsigned Aoutline, unsigned Acursor, double expand_factor, bool multiclip);
void int_backdrop_done();
void int_backdrop_pos(int index, int x, int y, int dx, int dy);
void int_backdrop_set(int index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty);
void int_backdrop_clear(int index, bool highlight);

void int_clip_set(int index, const resource& res, unsigned aspectx, unsigned aspecty, bool restart);
void int_clip_clear(int index);
void int_clip_start(int index);
bool int_clip_is_active(int index);

void int_update(bool progressive = true);
unsigned int_update_pre(bool progressive = false);
void int_update_post(unsigned y = 0);

unsigned int_getkey(bool update_background = true);
int int_keypressed();

void int_idle_repeat_reset();
void int_idle_time_reset();
void int_idle_0_enable(bool state);
void int_idle_1_enable(bool state);

int int_font_dx_get();
int int_font_dx_get(const std::string& s);
int int_font_dy_get();

int int_dx_get();
int int_dy_get();

// -------------------------------------------------------------------------
// Key

#define INT_KEY_NONE (1 << 16)
#define INT_KEY_UP (2 << 16)
#define INT_KEY_DOWN (3 << 16)
#define INT_KEY_LEFT (4 << 16)
#define INT_KEY_RIGHT (5 << 16)
#define INT_KEY_ENTER (6 << 16)
#define INT_KEY_ESC (7 << 16)
#define INT_KEY_SPACE (8 << 16)
#define INT_KEY_MODE (9 << 16)
#define INT_KEY_HOME (10 << 16)
#define INT_KEY_END (11 << 16)
#define INT_KEY_PGUP (12 << 16)
#define INT_KEY_PGDN (13 << 16)
#define INT_KEY_HELP (14 << 16)
#define INT_KEY_GROUP (15 << 16)
#define INT_KEY_TYPE (16 << 16)
#define INT_KEY_EXCLUDE (17 << 16)
#define INT_KEY_SORT (18 << 16)
#define INT_KEY_SETGROUP (19 << 16)
#define INT_KEY_SETTYPE (20 << 16)
#define INT_KEY_RUN_CLONE (21 << 16)
#define INT_KEY_IDLE_0 (22 << 16)
#define INT_KEY_IDLE_1 (23 << 16)
#define INT_KEY_SNAPSHOT (24 << 16)
#define INT_KEY_INS (25 << 16)
#define INT_KEY_DEL (26 << 16)
#define INT_KEY_COMMAND (27 << 16)
#define INT_KEY_OFF (28 << 16)
#define INT_KEY_MENU (29 << 16)
#define INT_KEY_EMU (30 << 16)
#define INT_KEY_ROTATE (31 << 16)
#define INT_KEY_LOCK (32 << 16)

bool int_key_in(const std::string& s);
void int_key_out(adv_conf* config_context, const char* tag);

// -------------------------------------------------------------------------
// Colors

extern int_color COLOR_HELP_NORMAL;
extern int_color COLOR_HELP_TAG;
extern int_color COLOR_CHOICE_TITLE;
extern int_color COLOR_CHOICE_NORMAL;
extern int_color COLOR_CHOICE_SELECT;
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
extern int_color COLOR_MENU_BACKDROP;
extern int_color COLOR_MENU_ICON;
extern int_color COLOR_MENU_CURSOR;

bool int_color_in(const std::string& s);
void int_color_out(adv_conf* config_context, const char* tag);

#endif

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

void text_init(adv_conf* config_context);
void text_done();
bool text_load(adv_conf* config_context);
bool text_init2(unsigned video_size, const std::string& sound_event_key);
void text_done2();
bool text_init3(double gamma, double brightness, unsigned idle_0, unsigned idle_0_rep,unsigned idle_1, unsigned idle_1_rep, unsigned repeat, unsigned repeat_rep, bool backdrop_fast, bool alpha_mode);
void text_done3(bool reset_video_mode);
bool text_init4(const std::string& font, unsigned orientation);
void text_done4();

void text_clear();
void text_clear(int x, int y, int dx, int dy, int color);
void text_box(int x, int y, int dx, int dy, int width, int color);

void text_put(int x, int y, char c, int color);
unsigned text_put_width(char c);
void text_put(int x, int y, const std::string& s, int color);
unsigned text_put_width(const std::string& s);
unsigned text_put(int x, int y, int dx, const std::string& s, int color);
void text_put_filled(int x, int y, int dx, const std::string& s, int color);
void text_put_special(bool& in, int x, int y, int dx, const std::string& s, int c0, int c1, int c2);

void text_backdrop_init(unsigned back_color, unsigned back_box_color, unsigned Amac, unsigned Aoutline, unsigned Acursor, double expand_factor);
void text_backdrop_done();
void text_backdrop_pos(int back_index, int x, int y, int dx, int dy);
void text_backdrop_set(int back_index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty);
void text_backdrop_clear(int back_index, bool highlight);

void text_clip_init();
void text_clip_start();
void text_clip_done();
void text_clip_set(int back_index, const resource& res, unsigned aspectx, unsigned aspecty);
void text_clip_clear();
bool text_clip_is_active();

void text_update(bool progressive = true);
unsigned text_update_pre(bool progressive = false);
void text_update_post(unsigned y = 0);

unsigned text_getkey(bool update_background = true);
int text_keypressed();

void text_idle_repeat_reset();
void text_idle_time_reset();
void text_idle_0_enable(bool state);
void text_idle_1_enable(bool state);

int text_font_dx_get();
int text_font_dy_get();

int text_dx_get();
int text_dy_get();

// -------------------------------------------------------------------------
// Orientation

#define TEXT_ORIENTATION_FLIP_X ORIENTATION_MIRROR_X
#define TEXT_ORIENTATION_FLIP_Y ORIENTATION_MIRROR_Y
#define TEXT_ORIENTATION_SWAP_XY ORIENTATION_FLIP_XY

#define TEXT_ORIENTATION_ROT0 0
#define TEXT_ORIENTATION_ROT90 (TEXT_ORIENTATION_SWAP_XY | TEXT_ORIENTATION_FLIP_X)
#define TEXT_ORIENTATION_ROT180 (TEXT_ORIENTATION_FLIP_X | TEXT_ORIENTATION_FLIP_Y)
#define TEXT_ORIENTATION_ROT270 (TEXT_ORIENTATION_SWAP_XY | TEXT_ORIENTATION_FLIP_Y)

// -------------------------------------------------------------------------
// Key

#define TEXT_KEY_NONE (1 << 16)
#define TEXT_KEY_UP (2 << 16)
#define TEXT_KEY_DOWN (3 << 16)
#define TEXT_KEY_LEFT (4 << 16)
#define TEXT_KEY_RIGHT (5 << 16)
#define TEXT_KEY_ENTER (6 << 16)
#define TEXT_KEY_ESC (7 << 16)
#define TEXT_KEY_SPACE (8 << 16)
#define TEXT_KEY_MODE (9 << 16)
#define TEXT_KEY_HOME (10 << 16)
#define TEXT_KEY_END (11 << 16)
#define TEXT_KEY_PGUP (12 << 16)
#define TEXT_KEY_PGDN (13 << 16)
#define TEXT_KEY_HELP (14 << 16)
#define TEXT_KEY_GROUP (15 << 16)
#define TEXT_KEY_TYPE (16 << 16)
#define TEXT_KEY_EXCLUDE (17 << 16)
#define TEXT_KEY_SORT (18 << 16)
#define TEXT_KEY_SETGROUP (19 << 16)
#define TEXT_KEY_SETTYPE (20 << 16)
#define TEXT_KEY_RUN_CLONE (21 << 16)
#define TEXT_KEY_IDLE_0 (22 << 16)
#define TEXT_KEY_IDLE_1 (23 << 16)
#define TEXT_KEY_SNAPSHOT (24 << 16)
#define TEXT_KEY_INS (25 << 16)
#define TEXT_KEY_DEL (26 << 16)
#define TEXT_KEY_COMMAND (27 << 16)
#define TEXT_KEY_OFF (28 << 16)
#define TEXT_KEY_MENU (29 << 16)
#define TEXT_KEY_EMU (30 << 16)
#define TEXT_KEY_ROTATE (31 << 16)
#define TEXT_KEY_LOCK (32 << 16)

bool text_key_in(const std::string& s);
void text_key_out(adv_conf* config_context, const char* tag);

// -------------------------------------------------------------------------
// Colors

extern unsigned COLOR_HELP_NORMAL;
extern unsigned COLOR_HELP_TAG;
extern unsigned COLOR_CHOICE_TITLE;
extern unsigned COLOR_CHOICE_NORMAL;
extern unsigned COLOR_CHOICE_SELECT;
extern unsigned COLOR_MENU_NORMAL;
extern unsigned COLOR_MENU_HIDDEN;
extern unsigned COLOR_MENU_TAG;
extern unsigned COLOR_MENU_SELECT;
extern unsigned COLOR_MENU_HIDDEN_SELECT;
extern unsigned COLOR_MENU_TAG_SELECT;
extern unsigned COLOR_MENU_BAR;
extern unsigned COLOR_MENU_BAR_TAG;
extern unsigned COLOR_MENU_BAR_HIDDEN;
extern unsigned COLOR_MENU_GRID;
extern unsigned COLOR_MENU_BACKDROP;
extern unsigned COLOR_MENU_ICON;
extern unsigned COLOR_MENU_CURSOR;

bool text_color_in(const std::string& s);
void text_color_out(adv_conf* config_context, const char* tag);

#endif

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
#include "event.h"
#include "color.h"
#include "conf.h"
#include "bitmap.h"

#include <stdlib.h>
#include <time.h>

#include <string>
#include <iostream>

void int_reg(adv_conf* config_context);
void int_unreg();
bool int_load(adv_conf* config_context);
bool int_init(unsigned video_sizex, unsigned video_sizey);
void int_done();
bool int_set(double gamma, double brightness, unsigned idle_0, unsigned idle_0_rep, unsigned idle_1, unsigned idle_1_rep, bool backdrop_fast, unsigned translucency, bool disable_special, bool auto_calib);
void int_unset(bool reset_video_mode);
bool int_enable(
	int font_text_x, int font_text_y, const std::string& font_text,
	int font_bar_x, int font_bar_y, const std::string& font_bar,
	int font_menu_x, int font_menu_y, const std::string& font_menu,
	unsigned orientation);
void int_disable();
void int_unplug();
void int_plug();
void int_joystick_replug();
void* int_save();
void int_restore(void* buffer);

adv_bitmap* int_image_load(const std::string& file, adv_color_rgb* rgb_map, unsigned& rgb_max);
void int_image_buffer(adv_bitmap* bitmap, adv_color_rgb* rgb_map, unsigned rgb_max);
void int_image_direct(adv_bitmap* bitmap, adv_color_rgb* rgb_map, unsigned rgb_max);
bool int_image(const std::string& file, unsigned& scale_x, unsigned& scale_y);
void int_clear(const adv_color_rgb& cbackground, const adv_color_rgb& coverscan);
void int_clear(int x, int y, int dx, int dy, const adv_color_rgb& color);
void int_clear_alpha(int x, int y, int dx, int dy, const adv_color_rgb& color);
void int_box(int x, int y, int dx, int dy, int width, const adv_color_rgb& color);
void int_rotate(int& x, int& y, int& dx, int& dy);
void int_invrotate(int& x, int& y, int& dx, int& dy);

typedef enum {
	text = 0,
	bar = 1,
	menu = 2
} font_t;

unsigned int_put_width(font_t font, char c);
unsigned int_put_width(font_t font, const std::string& s);
void int_put(font_t font, int x, int y, char c, const int_color& color);
void int_put(font_t font, int x, int y, const std::string& s, const int_color& color);
void int_put_filled(font_t font, int x, int y, int dx, const std::string& s, const int_color& color);
void int_put_filled_center(font_t font, int x, int y, int dx, const std::string& s, const int_color& color);
void int_put_special(font_t font, bool& in, int x, int y, int dx, const std::string& s, const int_color& c0, const int_color& c1, const int_color& c2);
void int_put_special_center(font_t font, bool& in, int x, int y, int dx, const std::string& s, const int_color& c0, const int_color& c1, const int_color& c2);
void int_put_alpha(font_t font, int x, int y, char c, const int_color& color);
void int_put_alpha(font_t font, int x, int y, const std::string& s, const int_color& color);
void int_put_filled_alpha(font_t font, int x, int y, int dx, const std::string& s, const int_color& color);
void int_put_special_alpha(font_t font, bool& in, int x, int y, int dx, const std::string& s, const int_color& c0, const int_color& c1, const int_color& c2);

unsigned int_put(font_t font, int x, int y, int dx, const std::string& s, const int_color& color);
unsigned int_put_alpha(font_t font, int x, int y, int dx, const std::string& s, const int_color& color);
unsigned int_put_right(font_t font, int x, int y, int dx, const std::string& s, const int_color& color);
unsigned int_put_right_alpha(font_t font, int x, int y, int dx, const std::string& s, const int_color& color);

void int_set_overlay(const std::string& desc, unsigned counter);
void int_clear_overlay(void);

void int_backdrop_init(const int_color& back_color, const int_color& back_box_color, unsigned Amac, unsigned Ainc, unsigned Aoutline, unsigned Acursor, double expand_factor, bool multiclip, int resizeeffect);
void int_backdrop_done();
void int_backdrop_pos(int index, int x, int y, int dx, int dy);
void int_backdrop_set(int index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty);
void int_backdrop_clear(int index, bool highlight);
void int_backdrop_redraw_all();

bool int_clip(const std::string& file, bool loop);
void int_clip_set(int index, const resource& res, unsigned aspectx, unsigned aspecty, bool restart);
void int_clip_clear(int index);
void int_clip_start(int index);
bool int_clip_is_active(int index);

void int_update(bool progressive = true);
unsigned int_update_pre(bool progressive = false);
void int_update_post(unsigned y = 0);

unsigned int_event_get(bool update_background = true);
bool int_event_waiting();

void int_idle_time_reset();
void int_idle_0_enable(bool state);
void int_idle_1_enable(bool state);
void int_idle_2_enable(bool state, unsigned delay);

int int_font_dx_get(font_t font);
int int_font_dx_get(font_t font, const std::string& s);
int int_font_dy_get(font_t font);

int int_dx_get();
int int_dy_get();

#endif


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
bool int_init(unsigned video_size);
void int_done();
bool int_set(double gamma, double brightness, unsigned idle_0, unsigned idle_0_rep, unsigned idle_1, unsigned idle_1_rep, bool backdrop_fast, unsigned translucency);
void int_unset(bool reset_video_mode);
bool int_enable(int fontx, int fonty, const std::string& font, unsigned orientation);
void int_disable();
void int_unplug();
void int_plug();
void* int_save();
void int_restore(void* buffer);

bool int_image(const std::string& file, unsigned& scale_x, unsigned& scale_y);
void int_clear(const adv_color_rgb& color);
void int_clear(int x, int y, int dx, int dy, const adv_color_rgb& color);
void int_clear_alpha(int x, int y, int dx, int dy, const adv_color_rgb& color);
void int_box(int x, int y, int dx, int dy, int width, const adv_color_rgb& color);
void int_rotate(int& x, int& y, int& dx, int& dy);
void int_invrotate(int& x, int& y, int& dx, int& dy);

unsigned int_put_width(char c);
unsigned int_put_width(const std::string& s);
void int_put(int x, int y, char c, const int_color& color);
void int_put(int x, int y, const std::string& s, const int_color& color);
void int_put_filled(int x, int y, int dx, const std::string& s, const int_color& color);
void int_put_special(bool& in, int x, int y, int dx, const std::string& s, const int_color& c0, const int_color& c1, const int_color& c2);
void int_put_alpha(int x, int y, char c, const int_color& color);
void int_put_alpha(int x, int y, const std::string& s, const int_color& color);
void int_put_filled_alpha(int x, int y, int dx, const std::string& s, const int_color& color);
void int_put_special_alpha(bool& in, int x, int y, int dx, const std::string& s, const int_color& c0, const int_color& c1, const int_color& c2);

unsigned int_put(int x, int y, int dx, const std::string& s, const int_color& color);
unsigned int_put_alpha(int x, int y, int dx, const std::string& s, const int_color& color);
unsigned int_put_right(int x, int y, int dx, const std::string& s, const int_color& color);
unsigned int_put_right_alpha(int x, int y, int dx, const std::string& s, const int_color& color);

void int_backdrop_init(const int_color& back_color, const int_color& back_box_color, unsigned Amac, unsigned Ainc, unsigned Aoutline, unsigned Acursor, double expand_factor, bool multiclip);
void int_backdrop_done();
void int_backdrop_pos(int index, int x, int y, int dx, int dy);
void int_backdrop_set(int index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty);
void int_backdrop_clear(int index, bool highlight);
void int_backdrop_redraw_all();

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

int int_font_dx_get();
int int_font_dx_get(const std::string& s);
int int_font_dy_get();

int int_dx_get();
int int_dy_get();

#endif


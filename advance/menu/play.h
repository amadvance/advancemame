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

#ifndef __PLAY_H
#define __PLAY_H

#include "resource.h"
#include "conf.h"

#include <string>

void play_reg(adv_conf* cfg_context);
bool play_load(adv_conf* config_context);
bool play_init();
void play_done();

void play_poll();
void play_fill();
void play_attenuation_set(int attenuation);
int play_attenuation_get();
void play_mute_set(bool mute);
bool play_mute_get();

#define PLAY_PRIORITY_END 4
#define PLAY_PRIORITY_EVENT 3
#define PLAY_PRIORITY_BACKGROUND 2
#define PLAY_PRIORITY_GAME_BACKGROUND 1
#define PLAY_PRIORITY_NONE 0

void play_foreground_effect_begin(const resource& s);
void play_foreground_effect_stop(const resource& s);
void play_foreground_effect_end(const resource& s);
void play_foreground_effect_key(const resource& s);
void play_foreground_effect_start(const resource& s);
void play_foreground_stop();
void play_foreground_wait();
bool play_foreground_is_active();

void play_background_effect(const resource& s, unsigned priority, bool loop);
void play_background_stop(unsigned priority);
void play_background_wait();
bool play_background_is_active();

#endif

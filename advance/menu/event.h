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

#ifndef __EVENT_H
#define __EVENT_H

#include "conf.h"

#include <string>

#define EVENT_NONE (1 << 16)
#define EVENT_UP (2 << 16)
#define EVENT_DOWN (3 << 16)
#define EVENT_LEFT (4 << 16)
#define EVENT_RIGHT (5 << 16)
#define EVENT_ENTER (6 << 16)
#define EVENT_ESC (7 << 16)
#define EVENT_SPACE (8 << 16)
#define EVENT_MODE (9 << 16)
#define EVENT_HOME (10 << 16)
#define EVENT_END (11 << 16)
#define EVENT_PGUP (12 << 16)
#define EVENT_PGDN (13 << 16)
#define EVENT_HELP (14 << 16)
#define EVENT_GROUP (15 << 16)
#define EVENT_TYPE (16 << 16)
#define EVENT_ATTRIB (17 << 16)
#define EVENT_SORT (18 << 16)
#define EVENT_SETGROUP (19 << 16)
#define EVENT_SETTYPE (20 << 16)
#define EVENT_CLONE (21 << 16)
#define EVENT_IDLE_0 (22 << 16)
#define EVENT_IDLE_1 (23 << 16)
#define EVENT_IDLE_2 (24 << 16)
#define EVENT_INS (25 << 16)
#define EVENT_DEL (26 << 16)
#define EVENT_COMMAND (27 << 16)
#define EVENT_EXIT (28 << 16)
#define EVENT_OFF (29 << 16)
#define EVENT_MENU (30 << 16)
#define EVENT_EMU (31 << 16)
#define EVENT_ROTATE (32 << 16)
#define EVENT_LOCK (33 << 16)
#define EVENT_PREVIEW (34 << 16)
#define EVENT_MUTE (35 << 16)
#define EVENT_OFF_FORCE (36 << 16)
#define EVENT_EXIT_FORCE (37 << 16)
#define EVENT_CALIBRATION (38 << 16)
#define EVENT_VOLUME (39 << 16)
#define EVENT_DIFFICULTY (40 << 16)
#define EVENT_UNASSIGNED (41 << 16)

bool event_in(const std::string& s);
void event_out(adv_conf* config_context, const char* tag);
std::string event_name(unsigned event);
bool event_is_visible(unsigned event);
void event_poll();
void event_push(int event);
void event_push_repeat(int event);
int event_pop();
int event_peek();

void event_setup(const std::string& press_sound, double delay_repeat_ms, double delay_repeat_next_ms, bool alpha_mode);
void event_unassigned(bool unassigned_mode);

#endif


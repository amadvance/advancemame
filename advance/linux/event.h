/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003 Andrea Mazzoleni
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
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#ifndef __EVENT_H
#define __EVENT_H

#include "extra.h"

int event_open(const char* file, unsigned char* evtype_bitmask, unsigned evtype_size);
void event_close(int f);
void event_log(int f, unsigned char* evtype_bitmask);
adv_error event_read(int f, int* type, int* code, int* value);
adv_error event_write(int f, int type, int code, int value);

adv_bool event_is_mouse(int f, unsigned char* evtype_bitmask);
adv_bool event_is_joystick(int f, unsigned char* evtype_bitmask);
adv_bool event_is_keyboard(int f, unsigned char* evtype_bitmask);

struct event_location {
	char file[128];
	unsigned vendor;
	unsigned product;
	unsigned version;
	unsigned bus;
	unsigned id;
};

unsigned event_locate(struct event_location* event_map, unsigned event_max, const char* prefix, adv_bool* eaccess);

static inline adv_bool event_test_bit(unsigned bit, unsigned char* evtype_bitmask)
{
	return (evtype_bitmask[bit/8] & (1 << (bit % 8))) != 0;
}

#endif

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

#include "portable.h"

#include "joy.h"

struct joy_entry {
	const char* name;
	unsigned code;
};

static struct joy_entry JOY_BUTTON[] = {
	{ "a", JOYB_A },
	{ "b", JOYB_B },
	{ "c", JOYB_C },
	{ "x", JOYB_X },
	{ "y", JOYB_Y },
	{ "z", JOYB_Z },
	{ "tl", JOYB_TL },
	{ "tr", JOYB_TR },
	{ "tl2", JOYB_TL2 },
	{ "tr2", JOYB_TR2 },
	{ "select", JOYB_SELECT },
	{ "start", JOYB_START },
	{ "mode", JOYB_MODE },
	{ "thumbl", JOYB_THUMBL },
	{ "thumbr", JOYB_THUMBR },
	{ "gear_down", JOYB_GEAR_DOWN },
	{ "gear_up", JOYB_GEAR_UP },
	{ "play", JOYB_PLAY },
	{ 0, 0 }
};

static char joy_name_buffer[64];

/**
 * Return a short name for the specified joy code.
 */
const char* joy_button_name(unsigned code)
{
	struct joy_entry* i;

	for (i = JOY_BUTTON; i->name; ++i)
		if (i->code == code)
			return i->name;

	snprintf(joy_name_buffer, sizeof(joy_name_buffer), "button%d", code + 1);

	return joy_name_buffer;
}

/**
 * Convert a short name to the relative joy code.
 * If the short name is unknown the JOYB_MAX value is returned.
 */
unsigned joy_button_code(const char* name)
{
	struct joy_entry* i;

	for (i = JOY_BUTTON; i->name; ++i)
		if (strcmp(name, i->name) == 0)
			return i->code;

	if (strncmp(name, "button", 6) == 0) {
		char* e;
		int v = strtol(name + 6, &e, 10) - 1;
		if (*e == 0 && v >= 0 && v < JOYB_BASE)
			return v;
	}

	return JOYB_MAX;
}

/**
 * Check if a joy code is a defined code.
 * Not all the codes from 0 to JOYB_MAX are defined.
 */
adv_bool joy_button_is_defined(unsigned code)
{
	struct joy_entry* i;

	if (code >= JOYB_MAX)
		return 0;

	for (i = JOY_BUTTON; i->name; ++i)
		if (i->code == code)
			return 1;

	return 0;
}


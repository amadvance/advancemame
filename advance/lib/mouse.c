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

#include "mouse.h"

struct mouse_entry {
	const char* name;
	unsigned code;
};

static struct mouse_entry MOUSE_BUTTON[] = {
	{ "left", MOUSEB_LEFT },
	{ "right", MOUSEB_RIGHT },
	{ "middle", MOUSEB_MIDDLE },
	{ "side", MOUSEB_SIDE },
	{ "extra", MOUSEB_EXTRA },
	{ "forward", MOUSEB_FORWARD },
	{ "back", MOUSEB_BACK },
	{ 0, 0 }
};

static char mouse_name_buffer[64];

/**
 * Return a short name for the specified mouse code.
 */
const char* mouse_button_name(unsigned code)
{
	struct mouse_entry* i;

	for (i = MOUSE_BUTTON; i->name; ++i)
		if (i->code == code)
			return i->name;

	snprintf(mouse_name_buffer, sizeof(mouse_name_buffer), "button%d", code + 1);

	return mouse_name_buffer;
}

/**
 * Convert a short name to the relative mouse code.
 * If the short name is unknown the MOUSEB_MAX value is returned.
 */
unsigned mouse_button_code(const char* name)
{
	struct mouse_entry* i;

	for (i = MOUSE_BUTTON; i->name; ++i)
		if (strcmp(name, i->name) == 0)
			return i->code;

	if (strncmp(name, "button", 6) == 0) {
		char* e;
		int v = strtol(name + 6, &e, 10) - 1;
		if (*e == 0 && v >= 0 && v < MOUSEB_BASE)
			return v;
	}

	return MOUSEB_MAX;
}

/**
 * Check if a mouse code is a defined code.
 * Not all the codes from 0 to MOUSEB_MAX are defined.
 */
adv_bool mouse_button_is_defined(unsigned code)
{
	struct mouse_entry* i;

	if (code >= MOUSEB_MAX)
		return 0;

	for (i = MOUSE_BUTTON; i->name; ++i)
		if (i->code == code)
			return 1;

	return 0;
}


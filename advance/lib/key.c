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

#include "key.h"
#include "os.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct key_entry {
	const char* name;
	unsigned code;
};

static struct key_entry KEY[] = {
{ "a", OS_KEY_A },
{ "b", OS_KEY_B },
{ "c", OS_KEY_C },
{ "d", OS_KEY_D },
{ "e", OS_KEY_E },
{ "f", OS_KEY_F },
{ "g", OS_KEY_G },
{ "h", OS_KEY_H },
{ "i", OS_KEY_I },
{ "j", OS_KEY_J },
{ "k", OS_KEY_K },
{ "l", OS_KEY_L },
{ "m", OS_KEY_M },
{ "n", OS_KEY_N },
{ "o", OS_KEY_O },
{ "p", OS_KEY_P },
{ "q", OS_KEY_Q },
{ "r", OS_KEY_R },
{ "s", OS_KEY_S },
{ "t", OS_KEY_T },
{ "u", OS_KEY_U },
{ "v", OS_KEY_V },
{ "w", OS_KEY_W },
{ "x", OS_KEY_X },
{ "y", OS_KEY_Y },
{ "z", OS_KEY_Z },
{ "0", OS_KEY_0 },
{ "1", OS_KEY_1 },
{ "2", OS_KEY_2 },
{ "3", OS_KEY_3 },
{ "4", OS_KEY_4 },
{ "5", OS_KEY_5 },
{ "6", OS_KEY_6 },
{ "7", OS_KEY_7 },
{ "8", OS_KEY_8 },
{ "9", OS_KEY_9 },
{ "0_pad", OS_KEY_0_PAD },
{ "1_pad", OS_KEY_1_PAD },
{ "2_pad", OS_KEY_2_PAD },
{ "3_pad", OS_KEY_3_PAD },
{ "4_pad", OS_KEY_4_PAD },
{ "5_pad", OS_KEY_5_PAD },
{ "6_pad", OS_KEY_6_PAD },
{ "7_pad", OS_KEY_7_PAD },
{ "8_pad", OS_KEY_8_PAD },
{ "9_pad", OS_KEY_9_PAD },
{ "f1", OS_KEY_F1 },
{ "f2", OS_KEY_F2 },
{ "f3", OS_KEY_F3 },
{ "f4", OS_KEY_F4 },
{ "f5", OS_KEY_F5 },
{ "f6", OS_KEY_F6 },
{ "f7", OS_KEY_F7 },
{ "f8", OS_KEY_F8 },
{ "f9", OS_KEY_F9 },
{ "f10", OS_KEY_F10 },
{ "f11", OS_KEY_F11 },
{ "f12", OS_KEY_F12 },
{ "esc", OS_KEY_ESC },
{ "backquote", OS_KEY_BACKQUOTE },
{ "minus", OS_KEY_MINUS },
{ "equals", OS_KEY_EQUALS },
{ "backspace", OS_KEY_BACKSPACE },
{ "tab", OS_KEY_TAB },
{ "openbrace", OS_KEY_OPENBRACE },
{ "closebrace", OS_KEY_CLOSEBRACE },
{ "enter", OS_KEY_ENTER },
{ "semicolon", OS_KEY_SEMICOLON },
{ "quote", OS_KEY_QUOTE },
{ "backslash", OS_KEY_BACKSLASH },
{ "less", OS_KEY_LESS },
{ "comma", OS_KEY_COMMA },
{ "period", OS_KEY_PERIOD },
{ "slash", OS_KEY_SLASH },
{ "space", OS_KEY_SPACE },
{ "insert", OS_KEY_INSERT },
{ "del", OS_KEY_DEL },
{ "home", OS_KEY_HOME },
{ "end", OS_KEY_END },
{ "pgup", OS_KEY_PGUP },
{ "pgdn", OS_KEY_PGDN },
{ "left", OS_KEY_LEFT },
{ "right", OS_KEY_RIGHT },
{ "up", OS_KEY_UP },
{ "down", OS_KEY_DOWN },
{ "slash", OS_KEY_SLASH_PAD },
{ "asterisk", OS_KEY_ASTERISK },
{ "minus_pad", OS_KEY_MINUS_PAD },
{ "plus_pad", OS_KEY_PLUS_PAD },
{ "period_pad", OS_KEY_PERIOD_PAD },
{ "enter_pad", OS_KEY_ENTER_PAD },
{ "prtscr", OS_KEY_PRTSCR },
{ "pause", OS_KEY_PAUSE },
{ "lshift", OS_KEY_LSHIFT },
{ "rshift", OS_KEY_RSHIFT },
{ "lcontrol", OS_KEY_LCONTROL },
{ "rcontrol", OS_KEY_RCONTROL },
{ "lalt", OS_KEY_ALT },
{ "ralt", OS_KEY_ALTGR },
{ "lwin", OS_KEY_LWIN },
{ "rwin", OS_KEY_RWIN },
{ "menu", OS_KEY_MENU },
{ "scrlock", OS_KEY_SCRLOCK },
{ "numlock", OS_KEY_NUMLOCK },
{ "capslock", OS_KEY_CAPSLOCK },
{ 0, 0 }
};

const char* key_name(unsigned code) {
	static char name[32];
	struct key_entry* i;

	for(i=KEY;i->name;++i)
		if (i->code == code)
			return i->name;

	sprintf(name, "%d", code);
	return name;
}

unsigned key_code(const char* name) {
	struct key_entry* i;

	for(i=KEY;i->name;++i)
		if (strcmp(name, i->name)==0)
			return i->code;

	if (name[0] != 0 && strspn(name,"0123456789") == strlen(name)) {
		int v;
		v = atoi(name);

		if (v >= 0 && v < OS_KEY_MAX)
			return v;
	}

	return OS_KEY_MAX;
}



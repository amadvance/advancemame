/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
 * Copyright (C) 2003 Martin Adrian
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

#include "mame2.h"

#include "emu.h"

#include "log.h"
#include "target.h"
#include "snstring.h"
#include "error.h"
#include "hscript.h"
#include "conf.h"
#include "os.h"
#include "keyall.h"
#include "mouseall.h"
#include "joyall.h"

#include <time.h>

#include "input.h"

/**************************************************************************/
/* MAME/OS equivalence */

/**
 * Equivalence from a system code and a MAME code.
 */
struct input_equiv {
	int os_code;
	int mame_code;
};

/**
 * Equivalence from system key code and MAME key code.
 */
static struct input_equiv input_keyequiv_map[] = {
{ DIGITAL_KBD(0, KEYB_A), KEYCODE_A },
{ DIGITAL_KBD(0, KEYB_B), KEYCODE_B },
{ DIGITAL_KBD(0, KEYB_C), KEYCODE_C },
{ DIGITAL_KBD(0, KEYB_D), KEYCODE_D },
{ DIGITAL_KBD(0, KEYB_E), KEYCODE_E },
{ DIGITAL_KBD(0, KEYB_F), KEYCODE_F },
{ DIGITAL_KBD(0, KEYB_G), KEYCODE_G },
{ DIGITAL_KBD(0, KEYB_H), KEYCODE_H },
{ DIGITAL_KBD(0, KEYB_I), KEYCODE_I },
{ DIGITAL_KBD(0, KEYB_J), KEYCODE_J },
{ DIGITAL_KBD(0, KEYB_K), KEYCODE_K },
{ DIGITAL_KBD(0, KEYB_L), KEYCODE_L },
{ DIGITAL_KBD(0, KEYB_M), KEYCODE_M },
{ DIGITAL_KBD(0, KEYB_N), KEYCODE_N },
{ DIGITAL_KBD(0, KEYB_O), KEYCODE_O },
{ DIGITAL_KBD(0, KEYB_P), KEYCODE_P },
{ DIGITAL_KBD(0, KEYB_Q), KEYCODE_Q },
{ DIGITAL_KBD(0, KEYB_R), KEYCODE_R },
{ DIGITAL_KBD(0, KEYB_S), KEYCODE_S },
{ DIGITAL_KBD(0, KEYB_T), KEYCODE_T },
{ DIGITAL_KBD(0, KEYB_U), KEYCODE_U },
{ DIGITAL_KBD(0, KEYB_V), KEYCODE_V },
{ DIGITAL_KBD(0, KEYB_W), KEYCODE_W },
{ DIGITAL_KBD(0, KEYB_X), KEYCODE_X },
{ DIGITAL_KBD(0, KEYB_Y), KEYCODE_Y },
{ DIGITAL_KBD(0, KEYB_Z), KEYCODE_Z },
{ DIGITAL_KBD(0, KEYB_0), KEYCODE_0 },
{ DIGITAL_KBD(0, KEYB_1), KEYCODE_1 },
{ DIGITAL_KBD(0, KEYB_2), KEYCODE_2 },
{ DIGITAL_KBD(0, KEYB_3), KEYCODE_3 },
{ DIGITAL_KBD(0, KEYB_4), KEYCODE_4 },
{ DIGITAL_KBD(0, KEYB_5), KEYCODE_5 },
{ DIGITAL_KBD(0, KEYB_6), KEYCODE_6 },
{ DIGITAL_KBD(0, KEYB_7), KEYCODE_7 },
{ DIGITAL_KBD(0, KEYB_8), KEYCODE_8 },
{ DIGITAL_KBD(0, KEYB_9), KEYCODE_9 },
{ DIGITAL_KBD(0, KEYB_0_PAD), KEYCODE_0_PAD },
{ DIGITAL_KBD(0, KEYB_1_PAD), KEYCODE_1_PAD },
{ DIGITAL_KBD(0, KEYB_2_PAD), KEYCODE_2_PAD },
{ DIGITAL_KBD(0, KEYB_3_PAD), KEYCODE_3_PAD },
{ DIGITAL_KBD(0, KEYB_4_PAD), KEYCODE_4_PAD },
{ DIGITAL_KBD(0, KEYB_5_PAD), KEYCODE_5_PAD },
{ DIGITAL_KBD(0, KEYB_6_PAD), KEYCODE_6_PAD },
{ DIGITAL_KBD(0, KEYB_7_PAD), KEYCODE_7_PAD },
{ DIGITAL_KBD(0, KEYB_8_PAD), KEYCODE_8_PAD },
{ DIGITAL_KBD(0, KEYB_9_PAD), KEYCODE_9_PAD },
{ DIGITAL_KBD(0, KEYB_F1), KEYCODE_F1_REAL },
{ DIGITAL_KBD(0, KEYB_F2), KEYCODE_F2 },
{ DIGITAL_KBD(0, KEYB_F3), KEYCODE_F3 },
{ DIGITAL_KBD(0, KEYB_F4), KEYCODE_F4 },
{ DIGITAL_KBD(0, KEYB_F5), KEYCODE_F5 },
{ DIGITAL_KBD(0, KEYB_F6), KEYCODE_F6 },
{ DIGITAL_KBD(0, KEYB_F7), KEYCODE_F7 },
{ DIGITAL_KBD(0, KEYB_F8), KEYCODE_F8 },
{ DIGITAL_KBD(0, KEYB_F9), KEYCODE_F9 },
{ DIGITAL_KBD(0, KEYB_F10), KEYCODE_F10 },
{ DIGITAL_KBD(0, KEYB_F11), KEYCODE_F11 },
{ DIGITAL_KBD(0, KEYB_F12), KEYCODE_F12 },
{ DIGITAL_KBD(0, KEYB_ESC), KEYCODE_ESC },
{ DIGITAL_KBD(0, KEYB_BACKQUOTE), KEYCODE_TILDE },
{ DIGITAL_KBD(0, KEYB_MINUS), KEYCODE_MINUS },
{ DIGITAL_KBD(0, KEYB_EQUALS), KEYCODE_EQUALS },
{ DIGITAL_KBD(0, KEYB_BACKSPACE), KEYCODE_BACKSPACE },
{ DIGITAL_KBD(0, KEYB_TAB), KEYCODE_TAB },
{ DIGITAL_KBD(0, KEYB_OPENBRACE), KEYCODE_OPENBRACE },
{ DIGITAL_KBD(0, KEYB_CLOSEBRACE), KEYCODE_CLOSEBRACE },
{ DIGITAL_KBD(0, KEYB_ENTER), KEYCODE_ENTER },
{ DIGITAL_KBD(0, KEYB_SEMICOLON), KEYCODE_COLON },
{ DIGITAL_KBD(0, KEYB_QUOTE), KEYCODE_QUOTE },
{ DIGITAL_KBD(0, KEYB_BACKSLASH), KEYCODE_BACKSLASH },
{ DIGITAL_KBD(0, KEYB_LESS), KEYCODE_BACKSLASH2 },
{ DIGITAL_KBD(0, KEYB_COMMA), KEYCODE_COMMA },
{ DIGITAL_KBD(0, KEYB_PERIOD), KEYCODE_STOP },
{ DIGITAL_KBD(0, KEYB_SLASH), KEYCODE_SLASH },
{ DIGITAL_KBD(0, KEYB_SPACE), KEYCODE_SPACE },
{ DIGITAL_KBD(0, KEYB_INSERT), KEYCODE_INSERT },
{ DIGITAL_KBD(0, KEYB_DEL), KEYCODE_DEL },
{ DIGITAL_KBD(0, KEYB_HOME), KEYCODE_HOME },
{ DIGITAL_KBD(0, KEYB_END), KEYCODE_END },
{ DIGITAL_KBD(0, KEYB_PGUP), KEYCODE_PGUP },
{ DIGITAL_KBD(0, KEYB_PGDN), KEYCODE_PGDN },
{ DIGITAL_KBD(0, KEYB_LEFT), KEYCODE_LEFT },
{ DIGITAL_KBD(0, KEYB_RIGHT), KEYCODE_RIGHT },
{ DIGITAL_KBD(0, KEYB_UP), KEYCODE_UP },
{ DIGITAL_KBD(0, KEYB_DOWN), KEYCODE_DOWN },
{ DIGITAL_KBD(0, KEYB_SLASH_PAD), KEYCODE_SLASH_PAD },
{ DIGITAL_KBD(0, KEYB_ASTERISK), KEYCODE_ASTERISK },
{ DIGITAL_KBD(0, KEYB_MINUS_PAD), KEYCODE_MINUS_PAD },
{ DIGITAL_KBD(0, KEYB_PLUS_PAD), KEYCODE_PLUS_PAD },
{ DIGITAL_KBD(0, KEYB_PERIOD_PAD), KEYCODE_DEL_PAD },
{ DIGITAL_KBD(0, KEYB_ENTER_PAD), KEYCODE_ENTER_PAD },
{ DIGITAL_KBD(0, KEYB_PRTSCR), KEYCODE_PRTSCR },
{ DIGITAL_KBD(0, KEYB_PAUSE), KEYCODE_PAUSE },
{ DIGITAL_KBD(0, KEYB_LSHIFT), KEYCODE_LSHIFT },
{ DIGITAL_KBD(0, KEYB_RSHIFT), KEYCODE_RSHIFT },
{ DIGITAL_KBD(0, KEYB_LCONTROL), KEYCODE_LCONTROL },
{ DIGITAL_KBD(0, KEYB_RCONTROL), KEYCODE_RCONTROL },
{ DIGITAL_KBD(0, KEYB_ALT), KEYCODE_LALT },
{ DIGITAL_KBD(0, KEYB_ALTGR), KEYCODE_RALT },
{ DIGITAL_KBD(0, KEYB_SCRLOCK), KEYCODE_SCRLOCK },
{ DIGITAL_KBD(0, KEYB_NUMLOCK), KEYCODE_NUMLOCK },
{ DIGITAL_KBD(0, KEYB_CAPSLOCK), KEYCODE_CAPSLOCK }
};

/**
 * Equivalence from system joystick/mouse code and MAME joystick/mouse code.
 */
static struct input_equiv input_joyequiv_map[] = {
{ DIGITAL_JOY(0, 0, 0, 1), JOYCODE_1_LEFT },
{ DIGITAL_JOY(0, 0, 0, 0), JOYCODE_1_RIGHT },
{ DIGITAL_JOY(0, 0, 1, 1), JOYCODE_1_UP },
{ DIGITAL_JOY(0, 0, 1, 0), JOYCODE_1_DOWN },
{ DIGITAL_JOY_BUTTON(0, 0), JOYCODE_1_BUTTON1 },
{ DIGITAL_JOY_BUTTON(0, 1), JOYCODE_1_BUTTON2 },
{ DIGITAL_JOY_BUTTON(0, 2), JOYCODE_1_BUTTON3 },
{ DIGITAL_JOY_BUTTON(0, 3), JOYCODE_1_BUTTON4 },
{ DIGITAL_JOY_BUTTON(0, 4), JOYCODE_1_BUTTON5 },
{ DIGITAL_JOY_BUTTON(0, 5), JOYCODE_1_BUTTON6 },
{ DIGITAL_JOY(1, 0, 0, 1), JOYCODE_2_LEFT },
{ DIGITAL_JOY(1, 0, 0, 0), JOYCODE_2_RIGHT },
{ DIGITAL_JOY(1, 0, 1, 1), JOYCODE_2_UP },
{ DIGITAL_JOY(1, 0, 1, 0), JOYCODE_2_DOWN },
{ DIGITAL_JOY_BUTTON(1, 0), JOYCODE_2_BUTTON1 },
{ DIGITAL_JOY_BUTTON(1, 1), JOYCODE_2_BUTTON2 },
{ DIGITAL_JOY_BUTTON(1, 2), JOYCODE_2_BUTTON3 },
{ DIGITAL_JOY_BUTTON(1, 3), JOYCODE_2_BUTTON4 },
{ DIGITAL_JOY_BUTTON(1, 4), JOYCODE_2_BUTTON5 },
{ DIGITAL_JOY_BUTTON(1, 5), JOYCODE_2_BUTTON6 },
{ DIGITAL_JOY(2, 0, 0, 1), JOYCODE_3_LEFT },
{ DIGITAL_JOY(2, 0, 0, 0), JOYCODE_3_RIGHT },
{ DIGITAL_JOY(2, 0, 1, 1), JOYCODE_3_UP },
{ DIGITAL_JOY(2, 0, 1, 0), JOYCODE_3_DOWN },
{ DIGITAL_JOY_BUTTON(2, 0), JOYCODE_3_BUTTON1 },
{ DIGITAL_JOY_BUTTON(2, 1), JOYCODE_3_BUTTON2 },
{ DIGITAL_JOY_BUTTON(2, 2), JOYCODE_3_BUTTON3 },
{ DIGITAL_JOY_BUTTON(2, 3), JOYCODE_3_BUTTON4 },
{ DIGITAL_JOY_BUTTON(2, 4), JOYCODE_3_BUTTON5 },
{ DIGITAL_JOY_BUTTON(2, 5), JOYCODE_3_BUTTON6 },
{ DIGITAL_JOY(3, 0, 0, 1), JOYCODE_4_LEFT },
{ DIGITAL_JOY(3, 0, 0, 0), JOYCODE_4_RIGHT },
{ DIGITAL_JOY(3, 0, 1, 1), JOYCODE_4_UP },
{ DIGITAL_JOY(3, 0, 1, 0), JOYCODE_4_DOWN },
{ DIGITAL_JOY_BUTTON(3, 0), JOYCODE_4_BUTTON1 },
{ DIGITAL_JOY_BUTTON(3, 1), JOYCODE_4_BUTTON2 },
{ DIGITAL_JOY_BUTTON(3, 2), JOYCODE_4_BUTTON3 },
{ DIGITAL_JOY_BUTTON(3, 3), JOYCODE_4_BUTTON4 },
{ DIGITAL_JOY_BUTTON(3, 4), JOYCODE_4_BUTTON5 },
{ DIGITAL_JOY_BUTTON(3, 5), JOYCODE_4_BUTTON6 }
};

/**************************************************************************/
/* MAME input map */

/**
 * Used to store the key list for MAME.
 */
static struct KeyboardInfo input_key_map[INPUT_DIGITAL_MAX];

/**
 * Used to store the joystick/mouse list for MAME.
 */
static struct JoystickInfo input_joy_map[INPUT_DIGITAL_MAX];

/**************************************************************************/
/* Names */

/** Max input name. */
#define INPUT_NAME_MAX 64

/**
 * Used to store the key names.
 */
static char input_keyname_map[INPUT_DIGITAL_MAX][INPUT_NAME_MAX];

/**
 * Used to store the joystick/mouse names.
 */
static char input_joyname_map[INPUT_DIGITAL_MAX][INPUT_NAME_MAX];

static char* input_analog_map_desc[INPUT_ANALOG_MAX] = {
	"x", "y", "z", "pedal"
};

static char* input_trak_map_desc[INPUT_TRAK_MAX] = {
	"trakx", "traky"
};

/**************************************************************************/
/* Parse */

static adv_error parse_int(int* v, const char* s)
{
	char* e;

	*v = strtol(s,&e,10);

	if (*e!=0 || e == s) {
		error_set("Invalid integer '%s'", s);
		return -1;
	}

	return 0;
}

static adv_error parse_joystick_stick(int* v, const char* s, int joystick)
{
	unsigned i;

	if (joystick < 0) {
		error_set("Invalid joystick");
		return -1;
	}

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_stick_count_get(joystick);++i) {
		if (strcmp(joystickb_stick_name_get(joystick,i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	error_set("Invalid stick '%s'", s);
	return -1;
}

static adv_error parse_joystick_stick_axe(int* v, const char* s, int joystick, int stick)
{
	unsigned i;

	if (joystick < 0) {
		error_set("Invalid joystick");
		return -1;
	}

	if (stick < 0) {
		error_set("Invalid stick");
		return -1;
	}

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	if (stick >= joystickb_stick_count_get(joystick)) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_stick_axe_count_get(joystick, stick);++i) {
		if (strcmp(joystickb_stick_axe_name_get(joystick, stick, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	error_set("Invalid joystick axe '%s'", s);

	return -1;
}

static adv_error parse_mouse_axe(int* v, const char* s, int mouse)
{
	unsigned i;

	if (mouse < 0) {
		error_set("Invalid mouse");
		return -1;
	}

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (mouse >= mouseb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<mouseb_axe_count_get(mouse);++i) {
		if (strcmp(mouseb_axe_name_get(mouse, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	error_set("Invalid mouse axe '%s'", s);

	return -1;
}

static adv_error parse_joystick_rel(int* v, const char* s, int joystick)
{
	unsigned i;

	if (joystick < 0) {
		error_set("Invalid joystick");
		return -1;
	}

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_rel_count_get(joystick);++i) {
		if (strcmp(joystickb_rel_name_get(joystick, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	error_set("Invalid joystick rel '%s'", s);

	return -1;
}

static adv_error parse_mouse_button(int* v, const char* s, int mouse)
{
	unsigned i;

	if (mouse < 0) {
		error_set("Invalid mouse");
		return -1;
	}

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (mouse >= mouseb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<mouseb_button_count_get(mouse);++i) {
		if (strcmp(mouseb_button_name_get(mouse, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	error_set("Invalid mouse button '%s'", s);

	return -1;
}

static adv_error parse_joystick_button(int* v, const char* s, int joystick)
{
	unsigned i;

	if (joystick < 0) {
		error_set("Invalid joystick");
		return -1;
	}

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_button_count_get(joystick);++i) {
		if (strcmp(joystickb_button_name_get(joystick, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	error_set("Invalid joystick button '%s'", s);

	return -1;
}

static adv_error parse_key(int* v, const char* s, int keyboard)
{
	if (keyboard < 0) {
		error_set("Invalid keyboard");
		return -1;
	}

	if (keyboard >= keyb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	*v = key_code(s);

	if (*v >= 0 && *v < KEYB_MAX) {
		return 0;
	}

	error_set("Invalid key '%s'", s);

	return -1;
}

static adv_error parse_direction(int* v, const char* s)
{
	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (strcmp(s,"left")==0 || strcmp(s,"up")==0) {
		*v = 1;
		return 0;
	}

	if (strcmp(s,"right")==0 || strcmp(s,"down")==0) {
		*v = 0;
		return 0;
	}

	error_set("Invalid direction '%s'", s);

	return -1;
}

static adv_error parse_analog(unsigned* map, char* s)
{
	int p;
	unsigned mac;
	unsigned i;
	adv_bool first;

	/* initialize */
	for(i=0;i<INPUT_MAP_MAX;++i)
		map[i] = 0;

	/* parse */
	first = 1;
	mac = 0;
	p = 0;
	sskip(&p, s, " \t");
	while (s[p]) {
		char c;
		const char* t;
		const char* v0;
		const char* v1;
		const char* v2;

		t = stoken(&c, &p, s, "[ \t", " \t");
		if (first && strcmp(t,"auto")==0) {
			sskip(&p, s, " \t");

			if (s[p] || c == '[') {
				error_set("Wrong use of 'auto'");
				return -1;
			}

			map[0] = ANALOG_SPECIAL_AUTO;

			return 0;
		}

		if (strcmp(t, "joystick")==0 || strcmp(t, "-joystick")==0) {
			int joystick, stick, axe;
			adv_bool negate;

			if (c!='[') {
				error_set("Missing [ in '%s'", t);
				return -1;
			}

			negate = t[0] == '-';

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v1 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v2 = stoken(&c, &p, s, "]", " \t");
			if (c!=']') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_stick(&stick, v1, joystick) != 0
				|| parse_joystick_stick_axe(&axe, v2, joystick, stick) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
				error_set("Invalid joystick '%d'", joystick);
				return -1;
			}
			if (stick < 0 || stick >= INPUT_STICK_MAX) {
				error_set("Invalid stick '%d'", stick);
				return -1;
			}
			if (axe < 0 || axe >= INPUT_AXE_MAX) {
				error_set("Invalid joystick axe '%d'", axe);
				return -1;
			}

			if (mac >= INPUT_MAP_MAX) {
				error_set("Too long");
				return -1;
			}

			map[mac] = ANALOG_JOY(joystick, stick, axe, negate);
			++mac;
		} else {
			error_set("Unknown '%s'", t);
			return -1;
		}

		sskip(&p, s, " \t");

		first = 0;
	}

	return 0;
}

static adv_error parse_trak(unsigned* map, char* s)
{
	int p;
	unsigned mac;
	unsigned i;
	adv_bool first;

	/* initialize */
	for(i=0;i<INPUT_MAP_MAX;++i)
		map[i] = 0;

	/* parse */
	first = 1;
	mac = 0;
	p = 0;
	sskip(&p, s, " \t");
	while (s[p]) {
		char c;
		const char* t;
		const char* v0;
		const char* v1;

		t = stoken(&c, &p, s, "[ \t", " \t");
		if (first && strcmp(t,"auto")==0) {
			sskip(&p, s, " \t");

			if (s[p] || c == '[') {
				error_set("Wrong use of 'auto'");
				return -1;
			}

			map[0] = ANALOG_SPECIAL_AUTO;

			return 0;
		}

		if (strcmp(t, "mouse")==0 || strcmp(t, "-mouse")==0) {
			int mouse, axe;
			adv_bool negate;
			
			if (c!='[') {
				error_set("Missing [ in '%s'", t);
				return -1;
			}

			negate = t[0] == '-';

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']') {
				error_set("Missing ] in '%s'", t);
				return -1;
			}

			if (parse_int(&mouse, v0) != 0
				|| parse_mouse_axe(&axe, v1, mouse) != 0) {
				return -1;
			}

			if (mouse < 0 || mouse >= INPUT_MOUSE_MAX) {
				error_set("Invalid mouse '%d'", mouse);
				return -1;
			}
			if (axe < 0 || axe >= INPUT_AXE_MAX) {
				error_set("Invalid mouse axe '%d'", axe);
				return -1;
			}

			if (mac >= INPUT_MAP_MAX) {
				error_set("Too long");
				return -1;
			}

			map[mac] = ANALOG_MOUSE(mouse, axe, negate);
			++mac;
		} else if (strcmp(t, "joystick_ball")==0 || strcmp(t, "-joystick_ball")==0) {
			int joystick, axe;
			adv_bool negate;
			
			if (c!='[') {
				error_set("Missing [ in '%s'", t);
				return -1;
			}

			negate = t[0] == '-';

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']') {
				error_set("Missing ] in '%s'", t);
				return -1;
			}

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_rel(&axe, v1, joystick) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
				error_set("Invalid joystick '%d'", joystick);
				return -1;
			}
			if (axe < 0 || axe >= INPUT_AXE_MAX) {
				error_set("Invalid joystick axe '%d'", axe);
				return -1;
			}

			if (mac >= INPUT_MAP_MAX) {
				error_set("Too long");
				return -1;
			}

			map[mac] = ANALOG_BALL(joystick, axe, negate);
			++mac;
		} else {
			error_set("Unknown '%s'", t);
			return -1;
		}

		sskip(&p, s, " \t");

		first = 0;
	}

	return 0;
}

static adv_error validate_digital(unsigned* map)
{
	unsigned i;
	adv_bool positive = 0;
	adv_bool pred_not = 0;
	adv_bool operand = 0;

	i = 0;
	while (i < INPUT_MAP_MAX && map[i] != DIGITAL_SPECIAL_NONE) {
		switch (map[i]) {
		case DIGITAL_SPECIAL_OR :
			/* allow concatenation with the previous mapping */
			if ((!operand || !positive) && i>0)
				return -1;
			pred_not = 0;
			positive = 0;
			operand = 0;
			break;
		case DIGITAL_SPECIAL_NOT :
			if (pred_not)
				return 0;
			pred_not = !pred_not;
			operand = 0;
			break;
		default:
			if (!pred_not)
				positive = 1;
			pred_not = 0;
			operand = 1;
			break;
		}

		++i;
	}

	if ((!operand || !positive) && i>0)
		return -1;

	return 0;
}

adv_error advance_input_parse_digital(unsigned* seq_map, unsigned seq_max, char* s)
{
	int p;
	unsigned mac;
	unsigned i;
	adv_bool first;

	/* initialize */
	for(i=0;i<seq_max;++i)
		seq_map[i] = DIGITAL_SPECIAL_NONE;

	/* parse */
	first = 1;
	mac = 0;
	p = 0;
	sskip(&p, s, " \t");
	while (s[p]) {
		char c;
		const char* t;
		const char* v0;
		const char* v1;
		const char* v2;
		const char* v3;

		t = stoken(&c, &p, s, "[ \t", " \t");
		if (first && strcmp(t,"auto")==0) {
			sskip(&p, s, " \t");

			if (s[p] || c == '[') {
				error_set("Wrong use of 'auto'");
				return -1;
			}

			seq_map[0] = DIGITAL_SPECIAL_AUTO;

			return 0;
		}

		if (strcmp(t, "keyboard")==0) {
			int board;
			int key;

			if (c!='[') {
				error_set("Missing [ in '%s'", t);
				return -1;
			}

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']') {
				error_set("Missing ] in '%s'", t);
				return -1;
			}

			if (parse_int(&board, v0) != 0
				|| parse_key(&key, v1, board) != 0)
				return -1;

			if (board < 0 || board >= INPUT_KEYBOARD_MAX) {
				error_set("Invalid keyboard '%d'", board);
				return -1;
			}
			if (key < 0 || key >= KEYB_MAX) {
				error_set("Invalid key '%d'", key);
				return -1;
			}

			if (mac >= seq_max) {
				error_set("Too long");
				return -1;
			}

			seq_map[mac] = DIGITAL_KBD(board, key);
			++mac;
		} else if (strcmp(t, "joystick_digital")==0) {
			int joystick;
			int stick;
			int axe;
			int dir;

			if (c!='[') {
				error_set("Missing [ in '%s'", t);
				return -1;
			}

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v1 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v2 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v3 = stoken(&c, &p, s, "]", " \t");
			if (c!=']') {
				error_set("Missing ] in '%s'", t);
				return -1;
			}

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_stick(&stick, v1, joystick) != 0
				|| parse_joystick_stick_axe(&axe, v2, joystick, stick) != 0
				|| parse_direction(&dir, v3) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
				error_set("Invalid joystick '%d'", joystick);
				return -1;
			}
			if (stick < 0 || stick >= INPUT_STICK_MAX) {
				error_set("Invalid stick '%d'", stick);
				return -1;
			}
			if (axe < 0 || axe >= INPUT_AXE_MAX) {
				error_set("Invalid joystick axe '%d'", axe);
				return -1;
			}

			if (mac >= seq_max) {
				error_set("Too long");
				return -1;
			}

			seq_map[mac] = DIGITAL_JOY(joystick, stick, axe, dir);
			++mac;
		} else if (strcmp(t, "joystick_button")==0) {
			int joystick;
			int button;

			if (c!='[') {
				error_set("Missing [ in '%s'", t);
				return -1;
			}

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']') {
				error_set("Missing ] in '%s'", t);
				return -1;
			}

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_button(&button, v1, joystick) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
				error_set("Invalid joystick '%d'", joystick);
				return -1;
			}
			if (button < 0 || button >= INPUT_BUTTON_MAX) {
				error_set("Invalid joystick button '%d'", button);
				return -1;
			}

			if (mac >= seq_max) {
				error_set("Too long");
				return -1;
			}

			seq_map[mac] = DIGITAL_JOY_BUTTON(joystick, button);
			++mac;
		} else if (strcmp(t, "mouse_button")==0) {
			int mouse;
			int button;

			if (c!='[') {
				error_set("Missing [ in '%s'", t);
				return -1;
			}

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',') {
				error_set("Missing , in '%s'", t);
				return -1;
			}

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']') {
				error_set("Missing ] in '%s'", t);
				return -1;
			}

			if (parse_int(&mouse, v0) != 0
				|| parse_mouse_button(&button, v1, mouse) != 0)
				return -1;

			if (mouse < 0 || mouse >= INPUT_MOUSE_MAX) {
				error_set("Invalid mouse '%d'", mouse);
				return -1;
			}
			if (button < 0 || button >= INPUT_BUTTON_MAX) {
				error_set("Invalid mouse button '%d'", button);
				return -1;
			}

			if (mac >= seq_max) {
				error_set("Too long");
				return -1;
			}

			seq_map[mac] = DIGITAL_MOUSE_BUTTON(mouse, button);
			++mac;
		} else if (strcmp(t, "or")==0) {
			if (c=='[') {
				error_set("Unexpected [ in '%s'", t);
				return -1;
			}

			if (mac >= seq_max) {
				error_set("Too long");
				return -1;
			}

			seq_map[mac] = DIGITAL_SPECIAL_OR;
			++mac;
		} else if (strcmp(t, "not")==0) {
			if (c=='[') {
				error_set("Unexpected [ in '%s'", t);
				return -1;
			}

			if (mac >= seq_max) {
				error_set("Too long");
				return -1;
			}

			seq_map[mac] = DIGITAL_SPECIAL_NOT;
			++mac;
		} else {
			error_set("Unknown '%s'", t);
			return -1;
		}

		sskip(&p, s, " \t");

		first = 0;
	}

	if (validate_digital(seq_map) != 0) {
		error_set("Wrong use of operators");
		return -1;
	}

	return 0;
}

static void output_digital(char* buffer, unsigned buffer_size, unsigned* seq_map, unsigned seq_max)
{
	unsigned i;

	sncpy(buffer, buffer_size, "");

	for(i=0;i<seq_max;++i) {
		unsigned v = seq_map[i];

		if (v == DIGITAL_SPECIAL_NONE) {
			break;
		} else if (v == DIGITAL_SPECIAL_OR) {
			if (buffer[0] != 0)
				sncat(buffer, buffer_size, " ");
			sncat(buffer, buffer_size, "or");
		} else if (v == DIGITAL_SPECIAL_NOT) {
			if (buffer[0] != 0)
				sncat(buffer, buffer_size, " ");
			sncat(buffer, buffer_size, "not");
		} else {
			switch (DIGITAL_TYPE_GET(v)) {
			case DIGITAL_TYPE_KBD :
				if (buffer[0] != 0)
					sncat(buffer, buffer_size, " ");
				sncatf(buffer, buffer_size, "keyboard[%d,%s]", DIGITAL_KBD_BOARD_GET(v), key_name(DIGITAL_KBD_KEY_GET(v)) );
				break;
			case DIGITAL_TYPE_JOY :
				if (buffer[0] != 0)
					sncat(buffer, buffer_size, " ");
				sncatf(buffer, buffer_size, "joystick_digital[%d,%d,%d,%d]", DIGITAL_JOY_DEV_GET(v), DIGITAL_JOY_STICK_GET(v), DIGITAL_JOY_AXE_GET(v), DIGITAL_JOY_DIR_GET(v));
				break;
			case DIGITAL_TYPE_JOY_BUTTON :
				if (buffer[0] != 0)
					sncat(buffer, buffer_size, " ");
				sncatf(buffer, buffer_size, "joystick_button[%d,%d]", DIGITAL_JOY_BUTTON_DEV_GET(v), DIGITAL_JOY_BUTTON_BUTTON_GET(v));
				break;
			case DIGITAL_TYPE_MOUSE_BUTTON :
				if (buffer[0] != 0)
					sncat(buffer, buffer_size, " ");
				sncatf(buffer, buffer_size, "mouse_button[%d,%d]", DIGITAL_MOUSE_BUTTON_DEV_GET(v), DIGITAL_MOUSE_BUTTON_BUTTON_GET(v));
				break;
			default:
				log_std(("ERROR:input: unknown input type %d (type:%d) in digital\n", v, DIGITAL_TYPE_GET(v)));
				break;
			}
		}
	}
}

static adv_error parse_inputname(char* s)
{
	char c;
	int p;
	const char* t;
	const char* argv[4];
	unsigned argc;
	const char* name;
	int i;
	adv_bool found;
	unsigned code;

	p = 0;
	sskip(&p, s, " \t");

	/* parse until first [ */
	t = stoken(&c, &p, s, "[ \t", " \t");
	if (c!='[') {
		error_set("Missing [ in '%s'", t);
		return -1;
	}

	/* get all arguments */
	argc = 0;
	while (s[p]) {
		argv[argc] = stoken(&c, &p, s, ",] \t", " \t");
		if (c == ',' || c == ']') {
			if (argc == 4) {
				error_set("Too many arguments for '%s'", t);
				return -1;
			}
			++argc;
			if (c == ']')
				break;
		} else {
			error_set("Missing ] in '%s'", t);
			return -1;
		}
	}
	if (c!=']') {
		error_set("Missing ] in '%s'", t);
		return -1;
	}

	/* skip spaces */
	sskip(&p, s, " \t");

	/* get new name */
	name = stoken(&c, &p, s, "", "");

	if (*name == 0) {
		error_set("Missing input name in '%s'", t);
		return -1;
	}

	/* parse arguments */
	if (strcmp(t, "keyboard") == 0) {
		int board, key;

		if (argc != 2) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

		if (parse_int(&board, argv[0]) != 0
			|| parse_key(&key, argv[1], board) != 0)
			return -1;

		if (board < 0 || board >= INPUT_KEYBOARD_MAX) {
			error_set("Invalid keyboard '%d'", board);
			return -1;
		}
		if (key < 0 || key >= KEYB_MAX) {
			error_set("Invalid key '%d'", key);
			return -1;
		}

		code = DIGITAL_KBD(board, key);
	} else if (strcmp(t, "joystick_digital") == 0) {
		int joystick, stick, axe, dir;

		if (argc != 4) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

	        if (parse_int(&joystick, argv[0]) != 0
			|| parse_joystick_stick(&stick, argv[1], joystick) != 0
			|| parse_joystick_stick_axe(&axe, argv[2], joystick, stick) != 0
			|| parse_direction(&dir, argv[3]) != 0)
			return -1;

		if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
			error_set("Invalid joystick '%d'", joystick);
			return -1;
		}
		if (stick < 0 || stick >= INPUT_STICK_MAX) {
			error_set("Invalid stick '%d'", stick);
			return -1;
		}
		if (axe < 0 || axe >= INPUT_AXE_MAX) {
			error_set("Invalid joystick axe '%d'", axe);
			return -1;
		}

		code = DIGITAL_JOY(joystick, stick, axe, dir);
	} else if (strcmp(t, "joystick_button") == 0) {
		int joystick, button;

		if (argc != 2) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

		if (parse_int(&joystick, argv[0]) != 0
			|| parse_joystick_button(&button, argv[1], joystick) != 0)
			return -1;

		if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
			error_set("Invalid joystick '%d'", joystick);
			return -1;
		}
		if (button < 0 || button >= INPUT_BUTTON_MAX) {
			error_set("Invalid joystick button '%d'", button);
			return -1;
		}

		code = DIGITAL_JOY_BUTTON(joystick, button);
	} else if (strcmp(t, "mouse_button") == 0) {
		int mouse, button;

		if (argc != 2) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

		if (parse_int(&mouse, argv[0]) != 0
			|| parse_mouse_button(&button, argv[1], mouse) != 0)
			return -1;

		if (mouse < 0 || mouse >= INPUT_MOUSE_MAX) {
			error_set("Invalid mouse '%d'", mouse);
			return -1;
		}
		if (button < 0 || button >= INPUT_BUTTON_MAX) {
			error_set("Invalid mouse button '%d'", button);
			return -1;
		}

		code = DIGITAL_MOUSE_BUTTON(mouse, button);
	} else {
		error_set("Unknown input type '%s'", t);
		return -1;
	}

	/* look up code in all the tables */
	found = 0;
	for(i=0;input_key_map[i].name;++i) {
		if (input_key_map[i].code == code) {
			sncpy(input_key_map[i].name, INPUT_NAME_MAX, name);
			found = 1;
		}
	}
	for(i=0;input_joy_map[i].name;++i) {
		if (input_joy_map[i].code == code) {
			sncpy(input_joy_map[i].name, INPUT_NAME_MAX, name);
			found = 1;
		}
	}

	/* if the code isn't found it isn't an error */
	/* for example some USB input devices may be removed */
	if (!found) {
		if (argc > 0) {
			log_std(("WARNING:emu:input: input '%s[%s,...]' not found", t, argv[0]));
		} else {
			log_std(("WARNING:emu:input: input '%s' not found", t));
		}
	}

	return 0;
}

adv_error advance_input_parse_analogname(unsigned* type, const char* buffer)
{
	struct mame_analog* i;

	if (strlen(buffer) < 3) {
		log_std(("WARNING:emu:glue: unknown analog name %s\n", buffer));
		return -1;
	}

	for(i=mame_analog_list();i->name;++i)
		if (strcmp(i->name, buffer)==0)
			break;
	if (!i->name) {
		log_std(("WARNING:emu:glue: unknown analog name %s\n", buffer));
		return -1;
	}

	*type = i->type;

	return 0;
}

adv_error advance_input_parse_analogvalue(int* delta, int* sensitivity, int* reverse, int* center, char* buffer)
{
	const char* v0;
	const char* v1;
	const char* v2;
	const char* v3;
	char c;
	int p;

	p = 0;

	v0 = stoken(&c, &p, buffer, ",", "");
	if (c != ',')
		return -1;
	v1 = stoken(&c, &p, buffer, ",", "");
	if (c != ',')
		return -1;
	v2 = stoken(&c, &p, buffer, ",", "");
	if (c != ',')
		return -1;
	v3 = stoken(&c, &p, buffer, "", "");
	if (c != 0)
		return -1;

	*delta = atoi(v0);
	*sensitivity = atoi(v1);
	if (strcmp(v2,"reverse") == 0)
		*reverse = 1;
	else if (strcmp(v2,"noreverse") == 0)
		*reverse = 0;
	else
		return -1;
	if (strcmp(v3,"center") == 0)
		*center = 1;
	else if (strcmp(v3,"nocenter") == 0)
		*center = 0;
	else
		return -1;

	return 0;
}

int advance_input_print_analogname(char* buffer, unsigned buffer_size, unsigned type)
{
	struct mame_analog* a;

	a = mame_analog_find(type & (~IPF_MASK | IPF_PLAYERMASK));
	if (!a) {
		log_std(("WARNING:emu:glue: unknown analog port %d\n", type & (~IPF_MASK | IPF_PLAYERMASK)));
		return -1;
	}

	snprintf(buffer, buffer_size, "%s", a->name);

	return 0;
}

void advance_input_print_analogvalue(char* buffer, unsigned buffer_size, int delta, int sensitivity, int reverse, int center)
{
	snprintf(buffer, buffer_size, "%d,%d,%s,%s",
		delta,
		sensitivity,
		reverse != 0 ? "reverse" : "noreverse",
		center != 0 ? "center" : "nocenter"
	);
}

adv_error advance_ui_parse_help(struct advance_ui_context* context, char* s)
{
	char c;
	int p;
	const char* t;
	const char* argv[4];
	unsigned argc;
	unsigned code;
	const char* sx;
	const char* sy;
	const char* sdx;
	const char* sdy;
	int x, y, dx, dy;

	p = 0;
	sskip(&p, s, " \t");

	/* parse until first [ */
	t = stoken(&c, &p, s, "[ \t", " \t");
	if (c!='[') {
		error_set("Missing [ in '%s'", t);
		return -1;
	}

	/* get all arguments */
	argc = 0;
	while (s[p]) {
		argv[argc] = stoken(&c, &p, s, ",] \t", " \t");
		if (c == ',' || c == ']') {
			if (argc == 4) {
				error_set("Too many arguments for '%s'", t);
				return -1;
			}
			++argc;
			if (c == ']')
				break;
		} else {
			error_set("Missing ] in '%s'", t);
			return -1;
		}
	}
	if (c!=']') {
		error_set("Missing ] in '%s'", t);
		return -1;
	}

	/* skip spaces */
	sskip(&p, s, " \t");

	/* get the pos */
	sx = stoken(&c, &p, s, " \t", " \t");
	sskip(&p, s, " \t");
	sy = stoken(&c, &p, s, " \t", " \t");
	sskip(&p, s, " \t");
	sdx = stoken(&c, &p, s, " \t", " \t");
	sskip(&p, s, " \t");
	sdy = stoken(&c, &p, s, " \t", " \t");

	if (parse_int(&x, sx) != 0
		|| parse_int(&y, sy) != 0
		|| parse_int(&dx, sdx) != 0
		|| parse_int(&dy, sdy) != 0) {
		error_set("Invalid help coordinate '%s %s %s %s'", sx, sy, sdx, sdy);
		return -1;
	}

	/* parse arguments */
	if (strcmp(t, "keyboard") == 0) {
		int board, key;

		if (argc != 2) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

		if (parse_int(&board, argv[0]) != 0
			|| parse_key(&key, argv[1], board) != 0)
			return -1;

		if (board < 0 || board >= INPUT_KEYBOARD_MAX) {
			error_set("Invalid keyboard '%d'", board);
			return -1;
		}
		if (key < 0 || key >= KEYB_MAX) {
			error_set("Invalid key '%d'", key);
			return -1;
		}

		code = DIGITAL_KBD(board, key);
	} else if (strcmp(t, "joystick_digital") == 0) {
		int joystick, stick, axe, dir;

		if (argc != 4) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

	        if (parse_int(&joystick, argv[0]) != 0
			|| parse_joystick_stick(&stick, argv[1], joystick) != 0
			|| parse_joystick_stick_axe(&axe, argv[2], joystick, stick) != 0
			|| parse_direction(&dir, argv[3]) != 0)
			return -1;

		if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
			error_set("Invalid joystick '%d'", joystick);
			return -1;
		}
		if (stick < 0 || stick >= INPUT_STICK_MAX) {
			error_set("Invalid stick '%d'", stick);
			return -1;
		}
		if (axe < 0 || axe >= INPUT_AXE_MAX) {
			error_set("Invalid joystick axe '%d'", axe);
			return -1;
		}

		code = DIGITAL_JOY(joystick, stick, axe, dir);
	} else if (strcmp(t, "joystick_button") == 0) {
		int joystick, button;

		if (argc != 2) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

		if (parse_int(&joystick, argv[0]) != 0
			|| parse_joystick_button(&button, argv[1], joystick) != 0)
			return -1;

		if (joystick < 0 || joystick >= INPUT_JOY_MAX) {
			error_set("Invalid joystick '%d'", joystick);
			return -1;
		}
		if (button < 0 || button >= INPUT_BUTTON_MAX) {
			error_set("Invalid joystick button '%d'", button);
			return -1;
		}

		code = DIGITAL_JOY_BUTTON(joystick, button);
	} else if (strcmp(t, "mouse_button") == 0) {
		int mouse, button;

		if (argc != 2) {
			error_set("Wrong number of arguments for '%s'", t);
			return -1;
		}

		if (parse_int(&mouse, argv[0]) != 0
			|| parse_mouse_button(&button, argv[1], mouse) != 0)
			return -1;

		if (mouse < 0 || mouse >= INPUT_MOUSE_MAX) {
			error_set("Invalid mouse '%d'", mouse);
			return -1;
		}
		if (button < 0 || button >= INPUT_BUTTON_MAX) {
			error_set("Invalid mouse button '%d'", button);
			return -1;
		}

		code = DIGITAL_MOUSE_BUTTON(mouse, button);
	} else {
		error_set("Unknown input type '%s'", t);
		return -1;
	}

	if (context->config.help_mac < INPUT_HELP_MAX) {
		context->config.help_map[context->config.help_mac].code = code;
		context->config.help_map[context->config.help_mac].x = x;
		context->config.help_map[context->config.help_mac].y = y;
		context->config.help_map[context->config.help_mac].dx = dx;
		context->config.help_map[context->config.help_mac].dy = dy;
		++context->config.help_mac;
	}

	return 0;
}


/**************************************************************************/
/* Input */

static inline void input_something_pressed(struct advance_input_context* context)
{
	context->state.input_on_this_frame_flag = 1;
}

static void input_keyboard_update(struct advance_input_context* context)
{
	unsigned char last[INPUT_KEYBOARD_MAX][KEYB_MAX];
	unsigned size = KEYB_MAX * keyb_count_get();
	unsigned i;

	/* read the keys for all the keyboards */
	for(i=0;i<keyb_count_get();++i) {
		keyb_all_get(i, last[i]);
	}

	if (context->config.steadykey_flag) {
		/* since the keyboard controller is slow, it is not capable of reporting multiple */
		/* key presses fast enough. We have to delay them in order not to lose special moves */
		/* tied to simultaneous button presses. */

		if (memcmp(last, context->state.key_old, size)!=0) {
			/* store the new copy */
			memcpy(context->state.key_old, last, size);
			input_something_pressed(context);
		} else {
			/* if the keyboard state is stable, copy it */
			memcpy(context->state.key_current, last, size);
		}
	} else {
		if (memcmp(last, context->state.key_current, size)!=0) {
			/* refresh the new copy */
			memcpy(context->state.key_current, last, size);
			input_something_pressed(context);
		}
	}
}

static unsigned search_joy_axe(unsigned player, const char* stick_name, const char* axe_name)
{
	unsigned axe;
	unsigned stick;

	if (player < joystickb_count_get()) {
		for(stick=0;stick<joystickb_stick_count_get(player);++stick) {
			if (strcmp(stick_name, joystickb_stick_name_get(player, stick)) == 0) {
				for(axe=0;axe<joystickb_stick_axe_count_get(player, stick);++axe) {
					if (strcmp(axe_name, joystickb_stick_axe_name_get(player, stick, axe)) == 0) {
						return ANALOG_JOY(player, stick, axe, 0);
					}
				}
			}
		}
	}

	return ANALOG_SPECIAL_NONE;
}

static unsigned search_mouse_axe(unsigned player, const char* axe_name)
{
	unsigned axe;

	if (player < mouseb_count_get()) {
		for(axe=0;axe<mouseb_axe_count_get(player);++axe) {
			if (strcmp(axe_name, mouseb_axe_name_get(player, axe)) == 0) {
				return ANALOG_MOUSE(player, axe, 0);
			}
		}
	}

	return ANALOG_SPECIAL_NONE;
}

static void input_setup_config(struct advance_input_context* context)
{
	unsigned i, j;

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		unsigned mac;
		unsigned v;

		/* This code tries to map the input device to the axe */
		/* effectively used by MAME. It's an hack heavily depended */
		/* on the current MAME behaviour (at present version 0.71). */
		/* A better approach is not possible due limitations of the */
		/* MAME input interface */

		/* To check the present MAME behaviour see the update_analog_port() */
		/* function in the inpport.c file */

		j = 0; /* X_AXIS (in osdepend.h) */
		if (context->config.analog_map[i][j].seq[0] == ANALOG_SPECIAL_AUTO) {
			mac = 0;
			v = search_joy_axe(i, "stick", "x");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "stick", "rx");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "hat", "x");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "wheel", "mono");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "rudder", "mono");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			context->config.analog_map[i][j].seq[mac++] = ANALOG_SPECIAL_NONE;
		}

		j = 1; /* Y_AXIS (in osdepend.h) */
		if (context->config.analog_map[i][j].seq[0] == ANALOG_SPECIAL_AUTO) {
			mac = 0;
			v = search_joy_axe(i, "stick", "y");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "stick", "ry");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "hat", "y");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			context->config.analog_map[i][j].seq[mac++] = ANALOG_SPECIAL_NONE;
		}

		j = 2; /* Z_AXIS (in osdepend.h) */
		if (context->config.analog_map[i][j].seq[0] == ANALOG_SPECIAL_AUTO) {
			mac = 0;
			v = search_joy_axe(i, "stick", "z");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "stick", "rz");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			v = search_joy_axe(i, "brake", "mono");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			context->config.analog_map[i][j].seq[mac++] = ANALOG_SPECIAL_NONE;
		}

		j = 3; /* PEDAL_AXIS (in osdepend.h) */
		if (context->config.analog_map[i][j].seq[0] == ANALOG_SPECIAL_AUTO) {
			mac = 0;
			v = search_joy_axe(i, "gas", "mono");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.analog_map[i][j].seq[mac++] = v;
			context->config.analog_map[i][j].seq[mac++] = ANALOG_SPECIAL_NONE;
		}
	}

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		j = 0; /* trakx */
		if (context->config.trak_map[i][j].seq[0] == ANALOG_SPECIAL_AUTO) {
			unsigned mac = 0;
			unsigned v = search_mouse_axe(i, "x");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.trak_map[i][j].seq[mac++] = v;
			context->config.trak_map[i][j].seq[mac] = ANALOG_SPECIAL_NONE;
		}

		j = 1; /* traky */
		if (context->config.trak_map[i][j].seq[0] == ANALOG_SPECIAL_AUTO) {
			unsigned mac = 0;
			unsigned v = search_mouse_axe(i, "y");
			if (v != ANALOG_SPECIAL_NONE)
				context->config.trak_map[i][j].seq[mac++] = v;
			context->config.trak_map[i][j].seq[mac] = ANALOG_SPECIAL_NONE;
		}
	}
}

static void input_setup_log(struct advance_input_context* context)
{
	unsigned i, j, k;

	log_std(("emu:input: input devices\n"));

	log_std(("Driver %s, keyboards %d\n", keyb_name(), keyb_count_get()));
	for(i=0;i<keyb_count_get();++i) {
		log_std(("keyboard %d\n", i));
		log_std(("\tkeys"));
		for(j=0;j<KEYB_MAX;++j) {
			if (keyb_has(i, j)) {
				log_std((" %s", key_name(j)));
			}
		}
		log_std(("\n"));
	}

	log_std(("Driver %s, mouses %d\n", mouseb_name(), mouseb_count_get()));
	for(i=0;i<mouseb_count_get();++i) {
		log_std(("mouse %d, axes %d, buttons %d\n", i, mouseb_axe_count_get(i), mouseb_button_count_get(i)));
		for(j=0;j<mouseb_axe_count_get(i);++j) {
			log_std(("\taxe %d [%s]\n", j, mouseb_axe_name_get(i, j)));
		}
		for(j=0;j<mouseb_button_count_get(i);++j) {
			log_std(("\tbutton %d [%s]\n", j, mouseb_button_name_get(i, j)));
		}
	}

	log_std(("Driver %s, joysticks %d\n", joystickb_name(), joystickb_count_get()));
	for(i=0;i<joystickb_count_get();++i) {
		log_std(("joy %d, controls %d, buttons %d, ball axes %d\n", i, joystickb_stick_count_get(i), joystickb_button_count_get(i), joystickb_rel_count_get(i)));
		for(j=0;j<joystickb_stick_count_get(i);++j) {
			log_std(("\tcontrol %d [%s], axes %d\n", j, joystickb_stick_name_get(i, j), joystickb_stick_axe_count_get(i, j)));
			for(k=0;k<joystickb_stick_axe_count_get(i,j);++k) {
				log_std(("\t\taxe %d [%s]\n", k, joystickb_stick_axe_name_get(i, j, k)));
			}
		}
		for(j=0;j<joystickb_button_count_get(i);++j) {
			log_std(("\tbutton %d [%s]\n", j, joystickb_button_name_get(i, j)));
		}
		for(j=0;j<joystickb_rel_count_get(i);++j) {
			log_std(("\tball axe %d [%s]\n", j, joystickb_rel_name_get(i, j)));
		}
	}

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		for(j=0;j<INPUT_ANALOG_MAX;++j) {
			log_std(("emu:input: input analog mapping player:%d axe:%d (%s) :", i, j, input_analog_map_desc[j]));
			for(k=0;k<INPUT_MAP_MAX;++k) {
				unsigned v = context->config.analog_map[i][j].seq[k];
				if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_JOY) {
					unsigned j = ANALOG_JOY_DEV_GET(v);
					unsigned s = ANALOG_JOY_STICK_GET(v);
					unsigned a = ANALOG_JOY_AXE_GET(v);
					adv_bool negate = ANALOG_JOY_NEGATE_GET(v);
					if (negate)
						log_std((" -joystick[%d,%d,%d]", j, s, a));
					else
						log_std((" joystick[%d,%d,%d]", j, s, a));
				} else {
					if (k == 0)
						log_std((" <none>"));
					break;
				}
			}
			log_std(("\n"));
		}
	}

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		for(j=0;j<INPUT_TRAK_MAX;++j) {
			log_std(("emu:input: input trak mapping player:%d axe:%d (%s) :", i, j, input_trak_map_desc[j]));
			for(k=0;k<INPUT_MAP_MAX;++k) {
				unsigned v = context->config.trak_map[i][j].seq[k];
				if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
					unsigned m = ANALOG_MOUSE_DEV_GET(v);
					unsigned a = ANALOG_MOUSE_AXE_GET(v);
					adv_bool negate = ANALOG_MOUSE_NEGATE_GET(v);
					if (negate)
						log_std((" -mouse[%d,%d]", m, a));
					else
						log_std((" mouse[%d,%d]", m, a));
				} else if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_BALL) {
					unsigned m = ANALOG_BALL_DEV_GET(v);
					unsigned a = ANALOG_BALL_AXE_GET(v);
					adv_bool negate = ANALOG_BALL_NEGATE_GET(v);
					if (negate)
						log_std((" -joystick_ball[%d,%d]", m, a));
					else
						log_std((" joystick_ball[%d,%d]", m, a));
				} else {
					if (k == 0)
						log_std((" <none>"));
					break;
				}
			}
			log_std(("\n"));
		}
	}
}

static void input_setup_list(struct advance_input_context* context)
{
	unsigned i, j, k;
	unsigned mac;

	/* fill the joystick/mouse vector */
	mac = 0;

	/* add the available mouse buttons */
	for(i=0;i<mouseb_count_get() && i<INPUT_MOUSE_MAX;++i) {
		for(j=0;j<mouseb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_DIGITAL_MAX) {
				if (i == 0)
					snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "m:%s", mouseb_button_name_get(i,j));
				else
					snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "m%d:%s", i+1, mouseb_button_name_get(i,j));
				input_joy_map[mac].name = input_joyname_map[mac];
				input_joy_map[mac].code = DIGITAL_MOUSE_BUTTON(i, j);
				++mac;
			}
		}
	}

	/* add the available joystick buttons/axes */
	for(i=0;i<joystickb_count_get() && i<INPUT_JOY_MAX;++i) {
		for(j=0;j<joystickb_stick_count_get(i) && j<INPUT_STICK_MAX;++j) {
			for(k=0;k<joystickb_stick_axe_count_get(i, j) && k<INPUT_AXE_MAX;++k) {
				if (mac+1 < INPUT_DIGITAL_MAX) {
					if (i == 0)
						snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j:%s:%s-", joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					else
						snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j%d:%s:%s-", i+1, joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					input_joy_map[mac].name = input_joyname_map[mac];
					input_joy_map[mac].code = DIGITAL_JOY(i, j, k, 0);
					++mac;
				}
				if (mac+1 < INPUT_DIGITAL_MAX) {
					if (i == 0)
						snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j:%s:%s+", joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					else
						snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j%d:%s:%s+", i+1, joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					input_joy_map[mac].name = input_joyname_map[mac];
					input_joy_map[mac].code = DIGITAL_JOY(i, j, k, 1);
					++mac;
				}
			}
		}

		for(j=0;j<joystickb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_DIGITAL_MAX) {
				if (i == 0)
					snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j:%s", joystickb_button_name_get(i, j));
				else
					snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j%d:%s", i+1, joystickb_button_name_get(i, j));
				input_joy_map[mac].name = input_joyname_map[mac];
				input_joy_map[mac].code = DIGITAL_JOY_BUTTON(i, j);
				++mac;
			}
		}
	}

	/* terminate the joystick vector */
	input_joy_map[mac].name = 0;
	input_joy_map[mac].code = 0;
	input_joy_map[mac].standardcode = 0;

	log_std(("emu:input: input digital joystick code %d\n", mac));

	/* set the equivalence */
	for(i=0;i<mac;++i) {
		input_joy_map[i].standardcode = CODE_OTHER;
		for(j=0;j<sizeof(input_joyequiv_map)/sizeof(input_joyequiv_map[0]);++j) {
			if (input_joyequiv_map[j].os_code == input_joy_map[i].code) {
				input_joy_map[i].standardcode = input_joyequiv_map[j].mame_code;
				break;
			}
		}
	}

	/* fill the keyboard vector */
	mac = 0;
	for(i=0;i<INPUT_KEYBOARD_MAX && i<keyb_count_get();++i) {
		for(j=0;j<KEYB_MAX;++j) {
			if (keyb_has(i, j)) {
				if (mac+1 < INPUT_DIGITAL_MAX) {
					if (i == 0)
						snprintf(input_keyname_map[mac], sizeof(input_keyname_map[mac]), "%s", key_name(j));
					else
						snprintf(input_keyname_map[mac], sizeof(input_keyname_map[mac]), "k%d:%s", i+1, key_name(j));
					input_key_map[mac].name = input_keyname_map[mac];
					input_key_map[mac].code = DIGITAL_KBD(i, j);
					++mac;
				}
			}
		}
	}

	/* terminate the keyboard vector */
	input_key_map[mac].name = 0;
	input_key_map[mac].code = 0;
	input_key_map[mac].standardcode = 0;

	/* set the equivalence */
	for(i=0;i<mac;++i) {
		input_key_map[i].standardcode = CODE_OTHER;
		for(j=0;j<sizeof(input_keyequiv_map)/sizeof(input_keyequiv_map[0]);++j) {
			if (input_keyequiv_map[j].os_code == input_key_map[i].code) {
				input_key_map[i].standardcode = input_keyequiv_map[j].mame_code;
				break;
			}
		}
	}

	log_std(("emu:input: input digital keyboard code %d\n", mac));
}

static void input_setup_init(struct advance_input_context* context)
{
	unsigned i, j, k, w;

	/* initialize the mouse state */
	for(i=0;i<INPUT_MOUSE_MAX;++i) {
		for(j=0;j<INPUT_AXE_MAX;++j) {
			context->state.mouse_analog_current[i][j] = 0;
		}
		for(j=0;j<INPUT_BUTTON_MAX;++j) {
			context->state.mouse_button_current[i][j] = 0;
		}
	}

	/* initialize the joystick state */
	for(i=0;i<INPUT_JOY_MAX;++i) {
		for(j=0;j<INPUT_AXE_MAX;++j) {
			context->state.ball_analog_current[i][j] = 0;
		}

		for(j=0;j<INPUT_STICK_MAX;++j) {
			for(k=0;k<INPUT_AXE_MAX;++k) {
				context->state.joystick_analog_current[i][j][k] = 0;
				for(w=0;w<INPUT_DIR_MAX;++w) {
					context->state.joystick_digital_current[i][j][k][w] = 0;
				}
			}
		}

		for(j=0;j<INPUT_BUTTON_MAX;++j) {
			context->state.joystick_button_current[i][j] = 0;
		}
	}

	/* initialize the keyboard state */
	memset(&context->state.key_old, sizeof(context->state.key_old), 0);
	memset(&context->state.key_current, sizeof(context->state.key_current), 0);

	/* initialize the input state */
	context->state.input_forced_exit_flag = 0;
	context->state.input_on_this_frame_flag = 0;
}

static void input_setup(struct advance_input_context* context)
{
	input_setup_init(context);

	input_setup_list(context);

	input_setup_config(context);

	input_setup_log(context);
}

static adv_error input_load_map(struct advance_input_context* context, adv_conf* cfg_context)
{
	unsigned i, j;
	const char* s;
	const struct mame_port* p;
	adv_conf_iterator k;

	/* analog */
	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		for(j=0;j<INPUT_ANALOG_MAX;++j) {
			char tag_buffer[32];
			char* d;
			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[p%d_%s]", i+1, input_analog_map_desc[j]);

			s = conf_string_get_default(cfg_context, tag_buffer);
			d = strdup(s);

			if (parse_analog(context->config.analog_map[i][j].seq, d)!=0) {
				free(d);
				target_err("Invalid argument '%s' for option '%s'.\n%s.\n", s, tag_buffer, error_get());
				target_err("Valid format is [-]joystick[JOYSTICK,STICK,AXE].\n");
				return -1;
			}
			free(d);
		}
	}

	/* trak */
	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		for(j=0;j<INPUT_TRAK_MAX;++j) {
			char tag_buffer[32];
			char* d;

			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[p%d_%s]", i+1, input_trak_map_desc[j]);
			s = conf_string_get_default(cfg_context, tag_buffer);
			d = strdup(s);
			if (parse_trak(context->config.trak_map[i][j].seq, d)!=0) {
				free(d);
				target_err("Invalid argument '%s' for option '%s'.\n%s.\n", s, tag_buffer, error_get());
				target_err("Valid format is [-]mouse[MOUSE,AXE]/[-]joystick_ball[JOYSTICK,AXE].\n");
				return -1;
			}
			free(d);
		}
	}

	/* digital */
	p = mame_port_list();
	while (p->name) {
		char tag_buffer[64];

		snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);

		/* get the game option */
		if (conf_string_get(cfg_context, tag_buffer, &s) == 0) {
				char* d = strdup(s);
			unsigned seq[INPUT_MAP_MAX];
			if (advance_input_parse_digital(seq, INPUT_MAP_MAX, d) != 0) {
				free(d);
				target_err("Invalid argument '%s' for option '%s'.\n%s.\n", s, tag_buffer, error_get());
				target_err("Valid format is keyboard[BOARD,KEY]/joystick_button[JOYSTICK,BUTTON]/mouse_button[MOUSE,BUTTON].\n");
				return -1;
			}
			free(d);
		}

		/* get the default option */
		if (conf_string_section_get(cfg_context, "", tag_buffer, &s) == 0) {
			char* d = strdup(s);
			unsigned seq[INPUT_MAP_MAX];
			if (advance_input_parse_digital(seq, INPUT_MAP_MAX, d) != 0) {
				free(d);
				target_err("Invalid argument '%s' for option '%s'.\n%s.\n", s, tag_buffer, error_get());
				target_err("Valid format is keyboard[BOARD,KEY]/joystick_button[JOYSTICK,BUTTON]/mouse_button[MOUSE,BUTTON].\n");
				return -1;
			}
			free(d);
		}

		++p;
	}

	log_std(("emu:input: input_name start\n"));
	for(conf_iterator_begin(&k, cfg_context, "input_name");!conf_iterator_is_end(&k);conf_iterator_next(&k)) {
		char* d = strdup(conf_iterator_string_get(&k));

		log_std(("emu:input: input_name '%s'\n", d));

		if (parse_inputname(d) != 0) {
			free(d);
			target_err("Invalid 'input_name' option.\n%s\n", error_get());
			return -1;
		}

		free(d);
	}

	return 0;
}

void osd2_customize_inputport_post_defaults(unsigned type, unsigned* seq, unsigned seq_max)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	const struct mame_port* p;

	log_std(("emu:input: osd2_customize_inputport_post_defaults(%d)\n", type));

	p = mame_port_list();
	while (p->name) {
		if (p->port == type) {
			char tag_buffer[64];
			char value_buffer[512];

			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);

			if (!seq || seq[0] == DIGITAL_SPECIAL_AUTO) {
				log_std(("emu:input: customize port default %s\n", tag_buffer));

				conf_remove(cfg_context, "", tag_buffer);
			} else {
				output_digital(value_buffer, sizeof(value_buffer), seq, seq_max);

				log_std(("emu:input: customize port %s %s\n", tag_buffer, value_buffer));

				conf_string_set(cfg_context, "", tag_buffer, value_buffer);
			}

			break;
		}

		++p;
	}
}

void osd2_customize_inputport_post_game(unsigned type, unsigned* seq, unsigned seq_max)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	const mame_game* game = CONTEXT.game;
	const struct mame_port* p;

	log_std(("emu:input: osd2_customize_inputport_post_game(%d)\n", type));

	p = mame_port_list();
	while (p->name) {
		if (p->port == type) {
			char tag_buffer[64];
			char value_buffer[512];

			log_std(("emu:input: setup port %s\n", p->name));

			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);

			if (!seq || seq[0] == DIGITAL_SPECIAL_AUTO) {
				log_std(("emu:input: customize port default %s/%s\n", mame_game_name(game), tag_buffer));

				conf_remove(cfg_context, mame_game_name(game), tag_buffer);
			} else {
				output_digital(value_buffer, sizeof(value_buffer), seq, seq_max);

				log_std(("emu:input: customize port %s/%s %s\n", mame_game_name(game), tag_buffer, value_buffer));

				conf_string_set(cfg_context, mame_game_name(game), tag_buffer, value_buffer);
			}

			break;
		}

		++p;
	}
}

/***************************************************************************/
/* Advance interface */

adv_error advance_input_init(struct advance_input_context* context, adv_conf* cfg_context)
{
	unsigned i;
	struct mame_port* p;
	struct mame_analog* a;

	conf_bool_register_default(cfg_context, "input_hotkey", 1);
	conf_bool_register_default(cfg_context, "input_steadykey", 0);
	conf_int_register_default(cfg_context, "input_idleexit", 0);

	/* analog */
	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		unsigned j;
		for(j=0;j<INPUT_ANALOG_MAX;++j) {
			char tag_buffer[64];
			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[p%d_%s]", i+1, input_analog_map_desc[j]);
			conf_string_register_default(cfg_context, tag_buffer, "auto");
		}
	}

	/* trak */
	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		unsigned j;
		for(j=0;j<INPUT_TRAK_MAX;++j) {
			char tag_buffer[64];
			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[p%d_%s]", i+1, input_trak_map_desc[j]);
			conf_string_register_default(cfg_context, tag_buffer, "auto");
		}
	}

	/* digital */
	p = mame_port_list();
	while (p->name) {
		char tag_buffer[64];
		snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);
		conf_string_register_default(cfg_context, tag_buffer, "auto");
		++p;
	}

	/* analog settings */
	a = mame_analog_list();
	while (a->name) {
		char tag_buffer[64];
		snprintf(tag_buffer, sizeof(tag_buffer), "input_setting[%s]", a->name);
		conf_string_register(cfg_context, tag_buffer);
		++a;
	}

	conf_string_register_multi(cfg_context, "input_name");

	joystickb_reg(cfg_context, 0);
	joystickb_reg_driver_all(cfg_context);
	mouseb_reg(cfg_context, 0);
	mouseb_reg_driver_all(cfg_context);
	keyb_reg(cfg_context, 1);
	keyb_reg_driver_all(cfg_context);

	context->state.active_flag = 0;

	return 0;
}

void advance_input_done(struct advance_input_context* context)
{
	assert(context->state.active_flag == 0);
}

adv_error advance_input_inner_init(struct advance_input_context* context, adv_conf* cfg_context)
{
	assert(context->state.active_flag == 0);

	if (joystickb_init() != 0) {
		target_err("%s\n", error_get());
		goto err;
	}

	if (mouseb_init() != 0) {
		target_err("%s\n", error_get());
		goto err_joystick;
	}

	if (keyb_init(context->config.disable_special_flag) != 0) {
		target_err("%s\n", error_get());
		goto err_mouse;
	}

	/* the map loading requires the joystick/mouse/keyboard initialized */
	if (input_load_map(context, cfg_context) != 0) {
		goto err_key;
	}

	input_setup(context);

	context->state.input_current_clock = target_clock();
	context->state.input_idle_clock = context->state.input_current_clock;
	context->state.input_on_this_frame_flag = 0;

	context->state.active_flag = 1;

	return 0;
err_key:
	keyb_done();
err_mouse:
	mouseb_done();
err_joystick:
	joystickb_done();
err:
	return -1;
}

void advance_input_inner_done(struct advance_input_context* context)
{
	keyb_done();
	mouseb_done();
	joystickb_done();

	context->state.active_flag = 0;
}

static void input_mouse_update(struct advance_input_context* context)
{
	unsigned i, j;

	for(i=0;i<mouseb_count_get() && i<INPUT_MOUSE_MAX;++i) {
		for(j=0;j<mouseb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			context->state.mouse_button_current[i][j] = mouseb_button_get(i, j);
			if (context->state.mouse_button_current[i][j])
				input_something_pressed(context);
		}

		for(j=0;j<mouseb_axe_count_get(i) && j<INPUT_AXE_MAX;++j) {
			context->state.mouse_analog_current[i][j] = mouseb_axe_get(i, j);
			if (context->state.mouse_analog_current[i][j])
				input_something_pressed(context);
		}
	}
}

static void input_joystick_update(struct advance_input_context* context)
{
	unsigned i, j, k, w;

	for(i=0;i<joystickb_count_get() && i<INPUT_JOY_MAX;++i) {
		for(j=0;j<joystickb_rel_count_get(i) && j<INPUT_AXE_MAX;++j) {
			context->state.ball_analog_current[i][j] = joystickb_rel_get(i, j);
			if (context->state.ball_analog_current[i][j])
				input_something_pressed(context);
		}

		for(j=0;j<joystickb_stick_count_get(i) && j<INPUT_STICK_MAX;++j) {
			for(k=0;k<joystickb_stick_axe_count_get(i, j) && k<INPUT_AXE_MAX;++k) {
				context->state.joystick_analog_current[i][j][k] = joystickb_stick_axe_analog_get(i, j, k);
				if (context->state.joystick_analog_current[i][j][k])
					input_something_pressed(context);

				for(w=0;w<2 && w<INPUT_DIR_MAX;++w) {
					context->state.joystick_digital_current[i][j][k][w] = joystickb_stick_axe_digital_get(i, j, k, w);
					if (context->state.joystick_digital_current[i][j][k][w])
						input_something_pressed(context);
				}
			}
		}

		for(j=0;j<joystickb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			context->state.joystick_button_current[i][j] = joystickb_button_get(i, j);
			if (context->state.joystick_button_current[i][j])
				input_something_pressed(context);
		}
	}
}

void advance_input_force_exit(struct advance_input_context* context)
{
	assert(context->state.active_flag != 0);

	context->state.input_forced_exit_flag = 1;
}

void advance_input_update(struct advance_input_context* context, struct advance_safequit_context* safequit_context, adv_bool is_pause)
{
	assert(context->state.active_flag != 0);

	os_poll();
	keyb_poll();
	mouseb_poll();
	joystickb_poll();

	input_keyboard_update(context);
	input_mouse_update(context);
	input_joystick_update(context);

	advance_safequit_update(safequit_context);

	/* forced exit due idle timeout */
	if (context->config.input_idle_limit && (context->state.input_current_clock - context->state.input_idle_clock) > context->config.input_idle_limit * TARGET_CLOCKS_PER_SEC) {
		context->state.input_forced_exit_flag = 1;
	}

	/* forced exit requested by the operating system */
	if (os_is_quit()) {
		context->state.input_forced_exit_flag = 1;
	}

	context->state.input_current_clock = target_clock();

	if (context->state.input_on_this_frame_flag || is_pause) {
		context->state.input_on_this_frame_flag = 0;
		context->state.input_idle_clock = context->state.input_current_clock;
	}
}

adv_error advance_input_config_load(struct advance_input_context* context, adv_conf* cfg_context)
{
	context->config.disable_special_flag = !conf_bool_get_default(cfg_context, "input_hotkey");
	context->config.steadykey_flag = conf_bool_get_default(cfg_context, "input_steadykey");
	context->config.input_idle_limit = conf_int_get_default(cfg_context, "input_idleexit");

	if (joystickb_load(cfg_context) != 0) {
		return -1;
	}

	if (mouseb_load(cfg_context) != 0) {
		return -1;
	}

	if (keyb_load(cfg_context) != 0) {
		return -1;
	}

	return 0;
}

/**
 * Check if an exit is requested.
 * \return
 *  0 - No exit.
 *  1 - Normal exit.
 *  2 - Exit forced.
 */
int advance_input_exit_filter(struct advance_input_context* context, struct advance_safequit_context* safequit_context, adv_bool result_memory)
{
	assert(context->state.active_flag != 0);

	if (context->state.input_forced_exit_flag)
		return 2;

	if (advance_safequit_can_exit(safequit_context)) {
		if (result_memory)
			return 2;
		else
			return 0;
	}

	if (result_memory) {
		return 1;
	}

	return 0;
}

adv_bool advance_input_digital_pressed(struct advance_input_context* context, unsigned code)
{
	unsigned type;

	assert(context->state.active_flag != 0);

	type = DIGITAL_TYPE_GET(code);

	switch (type) {
	case DIGITAL_TYPE_KBD : {
		unsigned b = DIGITAL_KBD_BOARD_GET(code);
		unsigned k = DIGITAL_KBD_KEY_GET(code);
		if (b < INPUT_KEYBOARD_MAX && k < KEYB_MAX) {
			if (context->state.key_current[b][k])
				return 1;
			/* simulate keys on all the keyboards */
			if (hardware_is_input_simulated(SIMULATE_KEY, k)) {
				return 1;
			}
			return 0;
		}
		break;
		}
	case DIGITAL_TYPE_JOY : {
		unsigned j = DIGITAL_JOY_DEV_GET(code);
		unsigned s = DIGITAL_JOY_STICK_GET(code);
		unsigned a = DIGITAL_JOY_AXE_GET(code);
		unsigned d = DIGITAL_JOY_DIR_GET(code);
		if (j < INPUT_JOY_MAX && s < INPUT_STICK_MAX && a < INPUT_AXE_MAX)
			return context->state.joystick_digital_current[j][s][a][d];
		break;
		}
	case DIGITAL_TYPE_JOY_BUTTON : {
		unsigned j = DIGITAL_JOY_BUTTON_DEV_GET(code);
		unsigned b = DIGITAL_JOY_BUTTON_BUTTON_GET(code);
		if (j < INPUT_JOY_MAX && b < INPUT_BUTTON_MAX)
			return context->state.joystick_button_current[j][b];
		break;
		}
	case DIGITAL_TYPE_MOUSE_BUTTON : {
		unsigned m = DIGITAL_MOUSE_BUTTON_DEV_GET(code);
		unsigned b = DIGITAL_MOUSE_BUTTON_BUTTON_GET(code);
		if (m < INPUT_MOUSE_MAX && b < INPUT_BUTTON_MAX)
			return context->state.mouse_button_current[m][b];
		break;
		}
	}

	log_std(("ERROR:emu:input: pressed(code:0x%08x) is not a correct code\n", code));

	return 0;
}

/***************************************************************************/
/* OSD interface */

/**
 * Get the list of all available digital key codes.
 */
const struct KeyboardInfo* osd_get_key_list(void)
{
	struct advance_input_context* context = &CONTEXT.input;

	assert(context->state.active_flag != 0);

	return input_key_map;
}

/**
 * Check if a digital key code is active.
 * This function is called only for the code returned by the osd_get_key_list() function.
 */
int osd_is_key_pressed(int keycode)
{
	struct advance_input_context* context = &CONTEXT.input;

	log_debug(("emu:input: osd_is_key_pressed(keycode:0x%08x)\n", keycode));

	return advance_input_digital_pressed(context, keycode);
}

int osd_readkey_unicode(int flush)
{
	/* no unicode support */
	return 0;
}

/**
 * Get the list of all available digital joystick codes.
 */
const struct JoystickInfo* osd_get_joy_list(void)
{
	struct advance_input_context* context = &CONTEXT.input;

	assert(context->state.active_flag != 0);

	return input_joy_map;
}

/**
 * Check if a digital joystick code is active.
 * This function is called only for the code returned by the osd_get_joy_list() function.
 */
int osd_is_joy_pressed(int joycode)
{
	struct advance_input_context* context = &CONTEXT.input;

	log_debug(("emu:input: osd_is_joy_pressed(joycode:0x%08x)\n", joycode));

	return advance_input_digital_pressed(context, joycode);
}

/**
 * Get the analog control input.
 * This function get all the analog axes for one player.
 * \param player Player.
 * \param analog_axis Vector filled with the analog position. Returned values are in the range -128 - 128.
 * \param analogjoy_input Vector containing the digital code which the osd_is_joystick_axis_code()
 * function reported be a joystick code. This code can be used to remap the joystick axes.
 */
void osd_analogjoy_read(int player, int analog_axis[MAX_ANALOG_AXES], InputCode analogjoy_input[MAX_ANALOG_AXES])
{
	struct advance_input_context* context = &CONTEXT.input;
	unsigned i, n;

	assert(context->state.active_flag != 0);

	/* the variable analogjoy_input is ignored */

	if (player < INPUT_PLAYER_MAX) {
		for(i=0;i<MAX_ANALOG_AXES;++i) {
			analog_axis[i] = 0;

			if (i < INPUT_ANALOG_MAX) {
				for(n=0;n<INPUT_MAP_MAX;++n) {
					unsigned v = context->config.analog_map[player][i].seq[n];
					if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_JOY) {
						unsigned j = ANALOG_JOY_DEV_GET(v);
						unsigned s = ANALOG_JOY_STICK_GET(v);
						unsigned a = ANALOG_JOY_AXE_GET(v);
						adv_bool negate = ANALOG_JOY_NEGATE_GET(v);
						if (j < INPUT_JOY_MAX && s < INPUT_STICK_MAX && a < INPUT_AXE_MAX) {
							if (negate)
								analog_axis[i] -= context->state.joystick_analog_current[j][s][a];
							else
								analog_axis[i] += context->state.joystick_analog_current[j][s][a];
						}
					} else {
						break;
					}
				}
			}

			if (analog_axis[i] < -128)
				analog_axis[i] = -128;
			if (analog_axis[i] > 128)
				analog_axis[i] = 128;
		}
	} else {
		unsigned i;
		for(i=0;i<MAX_ANALOG_AXES;++i) {
			analog_axis[i] = 0;
		}
	}
}

/**
 * Check if a digital code refers to a joystick.
 * This function is used to map analog joystick like digital joystick.
 */
int osd_is_joystick_axis_code(int joycode)
{
	/* not used */
	return 0;
}

int osd_joystick_needs_calibration(void)
{
	return 1;
}

void osd_joystick_start_calibration(void)
{
	joystickb_calib_start();
}

const char* osd_joystick_calibrate_next(void)
{
	return joystickb_calib_next();
}

void osd_joystick_calibrate(void)
{
	/* nothing */
}

void osd_joystick_end_calibration(void)
{
	/* nothing */
}

void osd_trak_read(int player, int* x, int* y)
{
	struct advance_input_context* context = &CONTEXT.input;
	unsigned n;

	assert(context->state.active_flag != 0);

	*x = 0;
	*y = 0;

	if (player < INPUT_PLAYER_MAX) {
		unsigned i;
		for(i=0;i<2;++i) {
			int r;

			r = 0;
			for(n=0;n<INPUT_MAP_MAX;++n) {
				unsigned v = context->config.trak_map[player][i].seq[n];
				if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
					unsigned m = ANALOG_MOUSE_DEV_GET(v);
					unsigned a = ANALOG_MOUSE_AXE_GET(v);
					adv_bool negate = ANALOG_MOUSE_NEGATE_GET(v);
					if (m < INPUT_MOUSE_MAX && a < INPUT_AXE_MAX) {
						if (negate)
							r -= context->state.mouse_analog_current[m][a];
						else
							r += context->state.mouse_analog_current[m][a];
					}
				} else if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_BALL) {
					unsigned j = ANALOG_BALL_DEV_GET(v);
					unsigned a = ANALOG_BALL_AXE_GET(v);
					adv_bool negate = ANALOG_BALL_NEGATE_GET(v);
					if (j < INPUT_JOY_MAX && a < INPUT_AXE_MAX) {
						if (negate)
							r -= context->state.ball_analog_current[j][a];
						else
							r += context->state.ball_analog_current[j][a];
					}
				} else {
					break;
				}
			}

			switch (i) {
			case 0 : *x += r; break;
			case 1 : *y += r; break;
			}
		}
	}
}

/**
 * Read the position of the lightgun.
 * The returned range is from -128 to 128. 0,0 is the center of the screen.
 */
void osd_lightgun_read(int player, int* deltax, int* deltay)
{
	/* no lightgun support */
	*deltax = 0;
	*deltay = 0;
}

#ifdef MESS
/**
 * Check if the keyboard is disabled.
 * This functions disabled all the emulated keyboard input.
 */
int osd_keyboard_disabled(void)
{
	return 0;
}

/**
 * Check if the user asked to quit.
 */
int osd_trying_to_quit(void)
{
	return os_is_quit() != 0;
}
#endif


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2003 Andrea Mazzoleni
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

#include "emu.h"
#include "log.h"
#include "target.h"
#include "error.h"

#include "mame2.h"

#include "hscript.h"
#include "conf.h"
#include "os.h"
#include "keyall.h"
#include "mouseall.h"
#include "joyall.h"

#include <time.h>

/**
 * Equivalence from a system code and a MAME code.
 */
struct input_equiv {
	int os_code;
	int mame_code;
};

/** Max input name. */
#define INPUT_NAME_MAX 64

/**************************************************************************/
/* Digital */

/* The DIGITAL values are also saved in the .cfg file. If you change them */
/* the cfg become invalid. */
#define DIGITAL_TYPE_SPECIAL 0 /* Special codes */
#define DIGITAL_TYPE_JOY 1 /* Joy digital move - DAAASSSDDDTTT */
#define DIGITAL_TYPE_JOY_BUTTON 2 /* Joy button - BBBBDDDTTT */
#define DIGITAL_TYPE_MOUSE_BUTTON 3 /* Mouse button - BBBBDDDTTT */
#define DIGITAL_TYPE_KBD 4 /* Keyboard button - KKKKKKKKKKBBBTTT */
#define DIGITAL_TYPE_GET(i) ((i) & 0x7)

#define DIGITAL_SPECIAL(code) (DIGITAL_TYPE_SPECIAL | (code) << 3)

#define DIGITAL_SPECIAL_NONE DIGITAL_SPECIAL(1)
#define DIGITAL_SPECIAL_OR DIGITAL_SPECIAL(2)
#define DIGITAL_SPECIAL_NOT DIGITAL_SPECIAL(3)
#define DIGITAL_SPECIAL_AUTO DIGITAL_SPECIAL(4)

/**************************************************************************/
/* Keyboard */

#define DIGITAL_KBD_BOARD_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_KBD_KEY_GET(i) (((i) >> 6) & 0x3FF)
#define DIGITAL_KBD(board, key) (DIGITAL_TYPE_KBD | (board) << 3 | (key) << 6)

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
{ DIGITAL_KBD(0, KEYB_F1), KEYCODE_F1 },
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
 * Used to store the key names.
 */
static char input_keyname_map[INPUT_DIGITAL_MAX][INPUT_NAME_MAX];

/**
 * Used to store the key list.
 */
static struct KeyboardInfo input_key_map[INPUT_DIGITAL_MAX];

/**************************************************************************/
/* Joystick/Mouse */

/*
 * T - Type
 * D - Device number
 * S - Stick number
 * A - Axe number
 * D - Direction
 * B - Button number
 */

/* Warning! Check the INPUT_*_MAX, they must match with the following macros. */

#define DIGITAL_JOY_DEV_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_JOY_STICK_GET(i) (((i) >> 6) & 0x7)
#define DIGITAL_JOY_AXE_GET(i) (((i) >> 9) & 0x7)
#define DIGITAL_JOY_DIR_GET(i) (((i) >> 12) & 0x1)
#define DIGITAL_JOY(joy, stick, axe, dir) (DIGITAL_TYPE_JOY | (joy) << 3 | (stick) << 6 | (axe) << 9 | (dir) << 12)

#define DIGITAL_JOY_BUTTON_DEV_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_JOY_BUTTON_BUTTON_GET(i) (((i) >> 6) & 0xF)
#define DIGITAL_JOY_BUTTON(joy, button) (DIGITAL_TYPE_JOY_BUTTON | (joy) << 3 | (button) << 6)

#define DIGITAL_MOUSE_BUTTON_DEV_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_MOUSE_BUTTON_BUTTON_GET(i) (((i) >> 6) & 0xF)
#define DIGITAL_MOUSE_BUTTON(mouse, button) (DIGITAL_TYPE_MOUSE_BUTTON | (mouse) << 3 | (button) << 6)

/** Max number of joystick/mouse digital input. */
#define INPUT_JOYMOUSE_MAX 1024

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

/**
 * Used to store the joystick/mouse list.
 */
static struct JoystickInfo input_joy_map[INPUT_JOYMOUSE_MAX + 1];

/**
 * Used to store the joystick/mouse names.
 */
static char input_joyname_map[INPUT_JOYMOUSE_MAX + 1][INPUT_NAME_MAX];

/**************************************************************************/
/* Analog */

/* The ANALOG value can be changed without limitation. */
#define ANALOG_TYPE_SPECIAL DIGITAL_TYPE_SPECIAL
#define ANALOG_TYPE_MOUSE 5 /* Mouse - NAAADDDTTT */
#define ANALOG_TYPE_JOY 6 /* Joy - NAAASSSDDDTTT */
#define ANALOG_TYPE_BALL 7 /* Ball - NAAADDDTTT */
#define ANALOG_TYPE_GET(i) DIGITAL_TYPE_GET(i)

#define ANALOG_SPECIAL_NONE DIGITAL_SPECIAL_NONE
#define ANALOG_SPECIAL_AUTO DIGITAL_SPECIAL_AUTO

/* Analog Mouse */
#define ANALOG_MOUSE_DEV_GET(i) (((i) >> 3) & 0x7)
#define ANALOG_MOUSE_AXE_GET(i) (((i) >> 6) & 0x7)
#define ANALOG_MOUSE_NEGATE_GET(i) (((i) >> 9) & 0x1)
#define ANALOG_MOUSE(dev, axe, negate) (ANALOG_TYPE_MOUSE | (dev) << 3 | (axe) << 6 | (negate) << 9)

/* Analog Joy */
#define ANALOG_JOY_DEV_GET(i) (((i) >> 3) & 0x7)
#define ANALOG_JOY_STICK_GET(i) (((i) >> 6) & 0x7)
#define ANALOG_JOY_AXE_GET(i) (((i) >> 9) & 0x7)
#define ANALOG_JOY_NEGATE_GET(i) (((i) >> 12) & 0x1)
#define ANALOG_JOY(joy, stick, axe, negate) (ANALOG_TYPE_JOY | (joy) << 3 | (stick) << 6 | (axe) << 9 | (negate) << 12)

/* Analog Joy/ball */
#define ANALOG_BALL_DEV_GET(i) (((i) >> 3) & 0x7)
#define ANALOG_BALL_AXE_GET(i) (((i) >> 6) & 0x7)
#define ANALOG_BALL_NEGATE_GET(i) (((i) >> 9) & 0x1)
#define ANALOG_BALL(joy, axe, negate) (ANALOG_TYPE_BALL | (joy) << 3 | (axe) << 6 | (negate) << 9)

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

	if (*e!=0 || e == s)
		return -1;

	return 0;
}

static adv_error parse_joystick_stick(int* v, const char* s, unsigned joystick)
{
	unsigned i;

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick < 0 || joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_stick_count_get(joystick);++i) {
		if (strcmp(joystickb_stick_name_get(joystick,i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	return -1;
}

static adv_error parse_joystick_stick_axe(int* v, const char* s, unsigned joystick, unsigned stick)
{
	unsigned i;

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick < 0 || joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	if (stick < 0 || stick >= joystickb_stick_count_get(joystick)) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_stick_axe_count_get(joystick, stick);++i) {
		if (strcmp(joystickb_stick_axe_name_get(joystick, stick, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	return -1;
}

static adv_error parse_mouse_axe(int* v, const char* s, unsigned mouse)
{
	unsigned i;

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (mouse < 0 || mouse >= mouseb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<mouseb_axe_count_get(mouse);++i) {
		if (strcmp(mouseb_axe_name_get(mouse, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	return -1;
}

static adv_error parse_joystick_rel(int* v, const char* s, unsigned joystick)
{
	unsigned i;

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick < 0 || joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_rel_count_get(joystick);++i) {
		if (strcmp(joystickb_rel_name_get(joystick, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	return -1;
}

static adv_error parse_mouse_button(int* v, const char* s, unsigned mouse)
{
	unsigned i;

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (mouse < 0 || mouse >= mouseb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<mouseb_button_count_get(mouse);++i) {
		if (strcmp(mouseb_button_name_get(mouse, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	return -1;
}

static adv_error parse_joystick_button(int* v, const char* s, unsigned joystick)
{
	unsigned i;

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	if (joystick < 0 || joystick >= joystickb_count_get()) {
		*v = 0; /* fake value, doesn't fail if you remove a device */
		return 0;
	}

	for(i=0;i<joystickb_button_count_get(joystick);++i) {
		if (strcmp(joystickb_button_name_get(joystick, i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	return -1;
}

static adv_error parse_key(int* v, const char* s, unsigned keyboard)
{
	unsigned i;

	if (strspn(s, "0123456789") == strlen(s)) {
		return parse_int(v, s);
	}

	for(i=0;i<KEYB_MAX;++i) {
		if (strcmp(key_name(i), s) == 0) {
			*v = i;
			return 0;
		}
	}

	return -1;
}

static adv_error parse_direction(int* v, const char* s)
{
	if (strcmp(s,"left")==0 || strcmp(s,"up")==0) {
		*v = 1;
		return 0;
	}

	if (strcmp(s,"right")==0 || strcmp(s,"down")==0) {
		*v = 0;
		return 0;
	}

	return -1;
}

static adv_error parse_analog(int* map, char* s)
{
	unsigned p;
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

		t = stoken(&c, &p, s, "[", " \t");
		if (first && strcmp(t,"auto")==0) {
			sskip(&p, s, " \t");

			if (s[p] || c == '[')
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = ANALOG_SPECIAL_AUTO;
			return 0;
		}

		if (strcmp(t, "joystick")==0 || strcmp(t, "-joystick")==0) {
			int joystick, stick, axe;
			adv_bool negate;

			if (c!='[')
				return -1;

			negate = t[0] == '-';

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v1 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v2 = stoken(&c, &p, s, "]", " \t");
			if (c!=']')
				return -1;

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_stick(&stick, v1, joystick) != 0
				|| parse_joystick_stick_axe(&axe, v2, joystick, stick) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX)
				return -1;
			if (stick < 0 || stick >= INPUT_STICK_MAX)
				return -1;
			if (axe < 0 || axe >= INPUT_AXE_MAX)
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = ANALOG_JOY(joystick, stick, axe, negate);
			++mac;
		} else {
			return -1;
		}

		sskip(&p, s, " \t");

		first = 0;
	}

	return 0;
}

static int parse_trak(int* map, char* s)
{
	unsigned p;
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

		t = stoken(&c, &p, s, "[", " \t");
		if (first && strcmp(t,"auto")==0) {
			sskip(&p, s, " \t");

			if (s[p] || c == '[')
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = ANALOG_SPECIAL_AUTO;

			return 0;
		}

		if (strcmp(t, "mouse")==0 || strcmp(t, "-mouse")==0) {
			int mouse, axe;
			adv_bool negate;
			
			if (c!='[')
				return -1;

			negate = t[0] == '-';

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']')
				return -1;

			if (parse_int(&mouse, v0) != 0
				|| parse_mouse_axe(&axe, v1, mouse) != 0)
				return -1;

			if (mouse < 0 || mouse >= INPUT_MOUSE_MAX)
				return -1;
			if (axe < 0 || axe >= INPUT_AXE_MAX)
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = ANALOG_MOUSE(mouse, axe, negate);
			++mac;
		} else if (strcmp(t, "joystick_ball")==0 || strcmp(t, "-joystick_ball")==0) {
			int joystick, axe;
			adv_bool negate;
			
			if (c!='[')
				return -1;

			negate = t[0] == '-';

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']')
				return -1;

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_rel(&axe, v1, joystick) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX)
				return -1;
			if (axe < 0 || axe >= INPUT_AXE_MAX)
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = ANALOG_BALL(joystick, axe, negate);
			++mac;
		} else {
			return -1;
		}

		sskip(&p, s, " \t");

		first = 0;
	}

	return 0;
}

static int parse_digital(unsigned* map, char* s)
{
	unsigned p;
	unsigned mac;
	unsigned i;
	adv_bool first;

	/* initialize */
	for(i=0;i<INPUT_MAP_MAX;++i)
		map[i] = DIGITAL_SPECIAL_NONE;

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

		t = stoken(&c, &p, s, "[", " \t");
		if (first && strcmp(t,"auto")==0) {
			sskip(&p, s, " \t");

			if (s[p] || c == '[')
				return -1;

			map[0] = DIGITAL_SPECIAL_AUTO;

			return 0;
		}

		if (strcmp(t, "keyboard")==0) {
			int board;
			int key;

			if (c!='[')
				return -1;

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']')
				return -1;

			if (parse_int(&board, v0) != 0
				|| parse_key(&key, v1, board) != 0)
				return -1;

			if (board < 0 || board >= INPUT_KEYBOARD_MAX)
				return -1;
			if (key < 0 || key >= KEYB_MAX)
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = DIGITAL_KBD(board, key);
			++mac;
		} else if (strcmp(t, "joystick_digital")==0) {
			int joystick;
			int stick;
			int axe;
			int dir;

			if (c!='[')
				return -1;

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v1 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v2 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v3 = stoken(&c, &p, s, "]", " \t");
			if (c!=']')
				return -1;

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_stick(&stick, v1, joystick) != 0
				|| parse_joystick_stick_axe(&axe, v2, joystick, stick) != 0
				|| parse_direction(&dir, v3) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX)
				return -1;
			if (stick < 0 || stick >= INPUT_STICK_MAX)
				return -1;
			if (axe < 0 || axe >= INPUT_AXE_MAX)
				return -1;
			if (dir < 0 || dir >= 2)
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = DIGITAL_JOY(joystick, stick, axe, dir);
			++mac;
		} else if (strcmp(t, "joystick_button")==0) {
			int joystick;
			int button;

			if (c!='[')
				return -1;

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']')
				return -1;

			if (parse_int(&joystick, v0) != 0
				|| parse_joystick_button(&button, v1, joystick) != 0)
				return -1;

			if (joystick < 0 || joystick >= INPUT_JOY_MAX)
				return -1;
			if (button < 0 || button >= INPUT_BUTTON_MAX)
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = DIGITAL_JOY_BUTTON(joystick, button);
			++mac;
		} else if (strcmp(t, "mouse_button")==0) {
			int mouse;
			int button;

			if (c!='[')
				return -1;

			v0 = stoken(&c, &p, s, ",", " \t");
			if (c!=',')
				return -1;

			v1 = stoken(&c, &p, s, "]", " \t");
			if (c!=']')
				return -1;

			if (parse_int(&mouse, v0) != 0
				|| parse_mouse_button(&button, v1, mouse) != 0)
				return -1;

			if (mouse < 0 || mouse >= INPUT_MOUSE_MAX)
				return -1;
			if (button < 0 || button >= INPUT_BUTTON_MAX)
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = DIGITAL_MOUSE_BUTTON(mouse, button);
			++mac;
		} else if (strcmp(t, "or")==0) {
			if (c=='[')
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = DIGITAL_SPECIAL_OR;
			++mac;
		} else if (strcmp(t, "not")==0) {
			if (c=='[')
				return -1;

			if (mac >= INPUT_MAP_MAX)
				return -1;

			map[mac] = DIGITAL_SPECIAL_NOT;
			++mac;
		} else {
			return -1;
		}

		sskip(&p, s, " \t");

		first = 0;
	}

	return 0;
}

/**************************************************************************/
/* Input */

static inline void input_something_pressed(struct advance_input_context* context)
{
	context->state.input_on_this_frame_flag = 1;
}

/*
 * Since the keyboard controller is slow, it is not capable of reporting multiple
 * key presses fast enough. We have to delay them in order not to lose special moves
 * tied to simultaneous button presses.
 */
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

		/* To check the MAME behaviour see the update_analog_port() */
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

	log_std(("advance: input devices\n"));

	log_std(("Driver %s, keyboards %d\n", keyb_name(), keyb_count_get()));
	for(i=0;i<keyb_count_get();++i) {
		log_std(("keyboard %d\n", i));
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
			log_std(("advance: input analog mapping player:%d axe:%d (%s) :", i, j, input_analog_map_desc[j]));
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
			log_std(("advance: input trak mapping player:%d axe:%d (%s) :", i, j, input_trak_map_desc[j]));
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

#if 0
	/* print the key list */
	{
		j = 0;
		log_std(("advance: Keys\t\t"));
		for(i=0;i<KEYB_MAX;++i) {
			if (key_is_defined(i)) {
				log_std(("%s, ", key_name(i)));
				j += 2 + strlen(key_name(i));
			}
			if (j > 60) {
				j = 0;
				log_std(("\n\t\t"));
			}
		}
		log_std(("\n"));
	}

	/* print the port list */
	{
		struct mame_port* p;
		j = 0;
		log_std(("advance: Ports\t\t"));
		for(p=mame_port_list();p->name;++p) {
			log_std(("%s, ", p->name));
			j += 2 + strlen(p->name);
			if (j > 60) {
				j = 0;
				log_std(("\n\t\t"));
			}
		}
		log_std(("\n"));
	}
#endif
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
			if (mac+1 < INPUT_JOYMOUSE_MAX) {
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
				if (mac+1 < INPUT_JOYMOUSE_MAX) {
					if (i == 0)
						snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j:%s:%s-", joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					else
						snprintf(input_joyname_map[mac], INPUT_NAME_MAX, "j%d:%s:%s-", i+1, joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					input_joy_map[mac].name = input_joyname_map[mac];
					input_joy_map[mac].code = DIGITAL_JOY(i, j, k, 0);
					++mac;
				}
				if (mac+1 < INPUT_JOYMOUSE_MAX) {
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
			if (mac+1 < INPUT_JOYMOUSE_MAX) {
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

	log_std(("advance: input digital joystick code %d\n", mac));

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

	log_std(("advance: input digital keyboard code %d\n", mac));
}

static void input_setup_init(struct advance_input_context* context)
{
	unsigned i, j, k, w;
	unsigned mac;

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
				target_err("Invalid argument '%s' for option '%s'\n", s, tag_buffer);
				target_err("Valid format is [-]joystick[JOYSTICK,STICK,AXE]\n");
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
				target_err("Invalid argument '%s' for option '%s'\n", s, tag_buffer);
				target_err("Valid format is [-]mouse[MOUSE,AXE]/[-]joystick_ball[JOYSTICK,AXE]\n");
				return -1;
			}
			free(d);
		}
	}

	/* digital */
	p = mame_port_list();
	i = 0;
	while (p->name) {
		char tag_buffer[64];
		char* d;

		snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);
		s = conf_string_get_default(cfg_context, tag_buffer);

		if (i<INPUT_DIGITAL_MAX) {
			d = strdup(s);
			context->config.digital_map[i].port = p->port;
			if (parse_digital(context->config.digital_map[i].seq, d) != 0) {
				free(d);
				target_err("Invalid argument '%s' for option '%s'\n", s, tag_buffer);
				target_err("Valid format is keyboard[KEY]/joystick_button[JOYSTICK,BUTTON]/mouse_button[MOUSE,BUTTON]\n");
				return -1;
			}
			++i;
			free(d);
		}

		++p;
	}
	context->config.digital_mac = i;

	return 0;
}

void osd_customize_inputport_defaults(struct ipd* defaults)
{
	struct advance_input_context* context = &CONTEXT.input;
	struct ipd* i = defaults;

	log_std(("advance: osd_customize_inputport_defaults()\n"));

	while (i->type != IPT_END) {
		unsigned port = i->type & (IPF_PLAYERMASK | ~IPF_MASK);
		unsigned j;

		for(j=0;j<context->config.digital_mac;++j)
			if (context->config.digital_map[j].port == port)
				break;

		if (j<context->config.digital_mac) {
			unsigned* seq = context->config.digital_map[j].seq;

			if (seq[0] != DIGITAL_SPECIAL_AUTO) {
				unsigned k;

				struct mame_port* p = mame_port_list();
				while (p->name) {
					if (p->port == port)
						break;
					++p;
				}
				if (p->name)
					log_std(("advance: customize input %s :", p->name));
				else
					log_std(("advance: customize input 0x%x :", port));

				for(k=0;k<SEQ_MAX && k<INPUT_MAP_MAX;++k) {
					unsigned v;
					switch (seq[k]) {
					case DIGITAL_SPECIAL_NONE :
						v = CODE_NONE;
						break;
					case DIGITAL_SPECIAL_OR :
						v = CODE_OR;
						log_std((" or"));
						break;
					case DIGITAL_SPECIAL_NOT :
						v = CODE_NOT;
						log_std((" not"));
						break;
					default:
						switch (DIGITAL_TYPE_GET(seq[k])) {
						case DIGITAL_TYPE_KBD :
							v = mame_ui_code_from_oskey(seq[k]);
							log_std((" keyboard[raw:0x%x,mamekeycode:%d]", seq[k], v));
							break;
						case DIGITAL_TYPE_MOUSE_BUTTON :
							v = mame_ui_code_from_osjoystick(seq[k]);
							log_std((" mouse_button[raw:0x%x,mamejoycode:%d]", seq[k], v));
							break;
						case DIGITAL_TYPE_JOY_BUTTON :
							v = mame_ui_code_from_osjoystick(seq[k]);
							log_std((" joystick_button[raw:0x%x,mamejoycode:%d]", seq[k], v));
							break;
						case DIGITAL_TYPE_JOY :
							v = mame_ui_code_from_osjoystick(seq[k]);
							log_std((" joystick_digital[raw:0x%x,mamejoycode:%d]", seq[k], v));
							break;
						default :
							v = CODE_NONE;
							break;
						}
					}
					i->seq[k] = v;
				}
				log_std(("\n"));
				for(;k<SEQ_MAX;++k)
					i->seq[k] = CODE_NONE;
			}
		}

		++i;
	}
}

/***************************************************************************/
/* Advance interface */

adv_error advance_input_init(struct advance_input_context* context, adv_conf* cfg_context)
{
	unsigned i;
	struct mame_port* p;

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

	joystickb_reg(cfg_context, 0);
	joystickb_reg_driver_all(cfg_context);
	mouseb_reg(cfg_context, 0);
	mouseb_reg_driver_all(cfg_context);
	keyb_reg(cfg_context, 1);
	keyb_reg_driver_all(cfg_context);

	context->state.active_flag = 0;
	context->config.digital_mac = 0;

	return 0;
}

void advance_input_done(struct advance_input_context* context)
{
	assert(context->state.active_flag == 0);
}

adv_error advance_input_inner_init(struct advance_input_context* context, adv_conf* cfg_context)
{
	unsigned i;

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
	const char* s;
	unsigned i, j;
	struct mame_port* p;

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

/***************************************************************************/
/* OSD interface */

/* return a list of all available keys */
const struct KeyboardInfo* osd_get_key_list(void)
{
	struct advance_input_context* context = &CONTEXT.input;

	assert(context->state.active_flag != 0);

	return input_key_map;
}

int osd_is_key_pressed(int keycode)
{
	struct advance_input_context* context = &CONTEXT.input;
	unsigned type;

	log_debug(("advance: osd_is_key_pressed(keycode:0x%08x)\n", keycode));

	assert(context->state.active_flag != 0);

	type = DIGITAL_TYPE_GET(keycode);

	switch (type) {
	case DIGITAL_TYPE_KBD : {
		unsigned b = DIGITAL_KBD_BOARD_GET(keycode);
		unsigned k = DIGITAL_KBD_KEY_GET(keycode);
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
	}

	log_std(("ERROR:advance: osd_is_key_pressed(keycode:0x%08x) is not a correct code\n", keycode));

	return 0;
}

int osd_readkey_unicode(int flush)
{
	return 0; /* no unicode support */
}

/* return a list of all available joys */
const struct JoystickInfo* osd_get_joy_list(void)
{
	struct advance_input_context* context = &CONTEXT.input;

	assert(context->state.active_flag != 0);

	return input_joy_map;
}

int osd_is_joy_pressed(int joycode)
{
	struct advance_input_context* context = &CONTEXT.input;
	unsigned type;

	log_debug(("advance: osd_is_joy_pressed(joycode:0x%08x)\n", joycode));

	assert(context->state.active_flag != 0);

	type = DIGITAL_TYPE_GET(joycode);

	switch (type) {
	case DIGITAL_TYPE_JOY : {
		unsigned j = DIGITAL_JOY_DEV_GET(joycode);
		unsigned s = DIGITAL_JOY_STICK_GET(joycode);
		unsigned a = DIGITAL_JOY_AXE_GET(joycode);
		unsigned d = DIGITAL_JOY_DIR_GET(joycode);
		if (j < INPUT_JOY_MAX && s < INPUT_STICK_MAX && a < INPUT_AXE_MAX)
			return context->state.joystick_digital_current[j][s][a][d];
		break;
		}
	case DIGITAL_TYPE_JOY_BUTTON : {
		unsigned j = DIGITAL_JOY_BUTTON_DEV_GET(joycode);
		unsigned b = DIGITAL_JOY_BUTTON_BUTTON_GET(joycode);
		if (j < INPUT_JOY_MAX && b < INPUT_BUTTON_MAX)
			return context->state.joystick_button_current[j][b];
		break;
		}
	case DIGITAL_TYPE_MOUSE_BUTTON : {
		unsigned m = DIGITAL_MOUSE_BUTTON_DEV_GET(joycode);
		unsigned b = DIGITAL_MOUSE_BUTTON_BUTTON_GET(joycode);
		if (m < INPUT_MOUSE_MAX && b < INPUT_BUTTON_MAX)
			return context->state.mouse_button_current[m][b];
		break;
		}
	}

	log_std(("ERROR:advance: osd_is_joy_pressed(joycode:0x%08x) is not a correct code\n", joycode));

	return 0;
}

/* return a value in the range -128 .. 128 (yes, 128, not 127) */
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
		}
	} else {
		unsigned i;
		for(i=0;i<MAX_ANALOG_AXES;++i) {
			analog_axis[i] = 0;
		}
	}
}

int osd_is_joystick_axis_code(int joycode)
{
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
				unsigned v = context->config.trak_map[player][0].seq[n];
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

void osd_lightgun_read(int player, int* deltax, int* deltay)
{
	*deltax = 0;
	*deltay = 0;
}

#ifdef MESS
int osd_keyboard_disabled(void)
{
	return 0; /* TODO implement osd_keyboard_disabled */
}

int osd_trying_to_quit(void)
{
	return os_is_quit() != 0;
}
#endif


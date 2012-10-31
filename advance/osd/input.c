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

#include "portable.h"

#include "input.h"
#include "emu.h"
#include "hscript.h"

#include "glueint.h"

#include "advance.h"

/**************************************************************************/
/* MAME/OS equivalence */

/**
 * Equivalence from a os code and a MAME code.
 */
struct input_equiv {
	int os_code;
	int mame_code;
};

/**
 * Equivalence from os codes and MAME codes.
 */
static struct input_equiv input_equiv_map[] = {
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
{ DIGITAL_KBD(0, KEYB_CAPSLOCK), KEYCODE_CAPSLOCK },
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
{ DIGITAL_JOY_BUTTON(0, 6), JOYCODE_1_BUTTON7 },
{ DIGITAL_JOY_BUTTON(0, 7), JOYCODE_1_BUTTON8 },
{ DIGITAL_JOY_BUTTON(0, 8), JOYCODE_1_BUTTON9 },
{ DIGITAL_JOY_BUTTON(0, 9), JOYCODE_1_BUTTON10 },
{ DIGITAL_JOY_BUTTON(0,10), JOYCODE_1_BUTTON11 },
{ DIGITAL_JOY_BUTTON(0,11), JOYCODE_1_BUTTON12 },
{ DIGITAL_JOY_BUTTON(0,12), JOYCODE_1_BUTTON13 },
{ DIGITAL_JOY_BUTTON(0,13), JOYCODE_1_BUTTON14 },
{ DIGITAL_JOY_BUTTON(0,14), JOYCODE_1_BUTTON15 },
{ DIGITAL_JOY_BUTTON(0,15), JOYCODE_1_BUTTON16 },
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
{ DIGITAL_JOY_BUTTON(1, 6), JOYCODE_2_BUTTON7 },
{ DIGITAL_JOY_BUTTON(1, 7), JOYCODE_2_BUTTON8 },
{ DIGITAL_JOY_BUTTON(1, 8), JOYCODE_2_BUTTON9 },
{ DIGITAL_JOY_BUTTON(1, 9), JOYCODE_2_BUTTON10 },
{ DIGITAL_JOY_BUTTON(1,10), JOYCODE_2_BUTTON11 },
{ DIGITAL_JOY_BUTTON(1,11), JOYCODE_2_BUTTON12 },
{ DIGITAL_JOY_BUTTON(1,12), JOYCODE_2_BUTTON13 },
{ DIGITAL_JOY_BUTTON(1,13), JOYCODE_2_BUTTON14 },
{ DIGITAL_JOY_BUTTON(1,14), JOYCODE_2_BUTTON15 },
{ DIGITAL_JOY_BUTTON(1,15), JOYCODE_2_BUTTON16 },
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
{ DIGITAL_JOY_BUTTON(2, 6), JOYCODE_3_BUTTON7 },
{ DIGITAL_JOY_BUTTON(2, 7), JOYCODE_3_BUTTON8 },
{ DIGITAL_JOY_BUTTON(2, 8), JOYCODE_3_BUTTON9 },
{ DIGITAL_JOY_BUTTON(2, 9), JOYCODE_3_BUTTON10 },
{ DIGITAL_JOY_BUTTON(2,10), JOYCODE_3_BUTTON11 },
{ DIGITAL_JOY_BUTTON(2,11), JOYCODE_3_BUTTON12 },
{ DIGITAL_JOY_BUTTON(2,12), JOYCODE_3_BUTTON13 },
{ DIGITAL_JOY_BUTTON(2,13), JOYCODE_3_BUTTON14 },
{ DIGITAL_JOY_BUTTON(2,14), JOYCODE_3_BUTTON15 },
{ DIGITAL_JOY_BUTTON(2,15), JOYCODE_3_BUTTON16 },
{ DIGITAL_JOY(3, 0, 0, 1), JOYCODE_4_LEFT },
{ DIGITAL_JOY(3, 0, 0, 0), JOYCODE_4_RIGHT },
{ DIGITAL_JOY(3, 0, 1, 1), JOYCODE_4_UP },
{ DIGITAL_JOY(3, 0, 1, 0), JOYCODE_4_DOWN },
{ DIGITAL_JOY_BUTTON(3, 0), JOYCODE_4_BUTTON1 },
{ DIGITAL_JOY_BUTTON(3, 1), JOYCODE_4_BUTTON2 },
{ DIGITAL_JOY_BUTTON(3, 2), JOYCODE_4_BUTTON3 },
{ DIGITAL_JOY_BUTTON(3, 3), JOYCODE_4_BUTTON4 },
{ DIGITAL_JOY_BUTTON(3, 4), JOYCODE_4_BUTTON5 },
{ DIGITAL_JOY_BUTTON(3, 5), JOYCODE_4_BUTTON6 },
{ DIGITAL_JOY_BUTTON(3, 6), JOYCODE_4_BUTTON7 },
{ DIGITAL_JOY_BUTTON(3, 7), JOYCODE_4_BUTTON8 },
{ DIGITAL_JOY_BUTTON(3, 8), JOYCODE_4_BUTTON9 },
{ DIGITAL_JOY_BUTTON(3, 9), JOYCODE_4_BUTTON10 },
{ DIGITAL_JOY_BUTTON(3,10), JOYCODE_4_BUTTON11 },
{ DIGITAL_JOY_BUTTON(3,11), JOYCODE_4_BUTTON12 },
{ DIGITAL_JOY_BUTTON(3,12), JOYCODE_4_BUTTON13 },
{ DIGITAL_JOY_BUTTON(3,13), JOYCODE_4_BUTTON14 },
{ DIGITAL_JOY_BUTTON(3,14), JOYCODE_4_BUTTON15 },
{ DIGITAL_JOY_BUTTON(3,15), JOYCODE_4_BUTTON16 },
{ DIGITAL_MOUSE_BUTTON(0, 0), MOUSECODE_1_BUTTON1 },
{ DIGITAL_MOUSE_BUTTON(0, 1), MOUSECODE_1_BUTTON2 },
{ DIGITAL_MOUSE_BUTTON(0, 2), MOUSECODE_1_BUTTON3 },
{ DIGITAL_MOUSE_BUTTON(0, 3), MOUSECODE_1_BUTTON4 },
{ DIGITAL_MOUSE_BUTTON(0, 4), MOUSECODE_1_BUTTON5 },
{ DIGITAL_MOUSE_BUTTON(0, 5), MOUSECODE_1_BUTTON6 },
{ DIGITAL_MOUSE_BUTTON(1, 0), MOUSECODE_2_BUTTON1 },
{ DIGITAL_MOUSE_BUTTON(1, 1), MOUSECODE_2_BUTTON2 },
{ DIGITAL_MOUSE_BUTTON(1, 2), MOUSECODE_2_BUTTON3 },
{ DIGITAL_MOUSE_BUTTON(1, 3), MOUSECODE_2_BUTTON4 },
{ DIGITAL_MOUSE_BUTTON(1, 4), MOUSECODE_2_BUTTON5 },
{ DIGITAL_MOUSE_BUTTON(1, 5), MOUSECODE_2_BUTTON6 },
{ DIGITAL_MOUSE_BUTTON(2, 0), MOUSECODE_3_BUTTON1 },
{ DIGITAL_MOUSE_BUTTON(2, 1), MOUSECODE_3_BUTTON2 },
{ DIGITAL_MOUSE_BUTTON(2, 2), MOUSECODE_3_BUTTON3 },
{ DIGITAL_MOUSE_BUTTON(2, 3), MOUSECODE_3_BUTTON4 },
{ DIGITAL_MOUSE_BUTTON(2, 4), MOUSECODE_3_BUTTON5 },
{ DIGITAL_MOUSE_BUTTON(2, 5), MOUSECODE_3_BUTTON6 },
{ DIGITAL_MOUSE_BUTTON(3, 0), MOUSECODE_4_BUTTON1 },
{ DIGITAL_MOUSE_BUTTON(3, 1), MOUSECODE_4_BUTTON2 },
{ DIGITAL_MOUSE_BUTTON(3, 2), MOUSECODE_4_BUTTON3 },
{ DIGITAL_MOUSE_BUTTON(3, 3), MOUSECODE_4_BUTTON4 },
{ DIGITAL_MOUSE_BUTTON(3, 4), MOUSECODE_4_BUTTON5 },
{ DIGITAL_MOUSE_BUTTON(3, 5), MOUSECODE_4_BUTTON6 }
};

/**************************************************************************/
/* MAME input map */

/**
 * Used to store the input list for MAME.
 */
static os_code_info input_code_map[INPUT_DIGITAL_MAX];

/**************************************************************************/
/* Names */

/** Max input name. */
#define INPUT_NAME_MAX 64

/**
 * Used to store the names.
 */
static char input_name_map[INPUT_DIGITAL_MAX][INPUT_NAME_MAX];

/** Analog inputs. */
/*@{*/
#define INPUT_ANALOG_PADDLE_X 0
#define INPUT_ANALOG_PADDLE_Y 1
#define INPUT_ANALOG_STICK_X 2
#define INPUT_ANALOG_STICK_Y 3
#define INPUT_ANALOG_STICK_Z 4
#define INPUT_ANALOG_LIGHTGUN_X 5
#define INPUT_ANALOG_LIGHTGUN_Y 6
#define INPUT_ANALOG_PEDAL 7
#define INPUT_ANALOG_PEDAL2 8
#define INPUT_ANALOG_PEDAL3 9
#define INPUT_ANALOG_DIAL_X 10
#define INPUT_ANALOG_DIAL_Y 11
#define INPUT_ANALOG_TRACKBALL_X 12
#define INPUT_ANALOG_TRACKBALL_Y 13
#define INPUT_ANALOG_MOUSE_X 14
#define INPUT_ANALOG_MOUSE_Y 15
/*@}*/

struct analog_axe_info {
	const char* control;
	const char* axe;
};

struct analog_info {
	unsigned index;
	const char* name;
	struct analog_axe_info map[8];
};

struct analog_info ANALOG_INFO[] = {
{ INPUT_ANALOG_PADDLE_X, "paddlex", { { "stick", "x" }, { "relative", "x" }, { "stick", "rx" } } },
{ INPUT_ANALOG_PADDLE_Y, "paddley", { { "stick", "y" }, { "relative", "y" }, { "stick", "ry" } } },
{ INPUT_ANALOG_STICK_X, "stickx", { { "stick", "x" }, { "wheel", "mono" }, { "relative", "x" }, { "stick", "rx" }, { "rudder", "mono" }, { "throttle", "mono" } } },
{ INPUT_ANALOG_STICK_Y, "sticky", { { "stick", "y" }, { "relative", "y" }, { "stick", "ry" } } },
{ INPUT_ANALOG_STICK_Z, "stickz", { { "stick", "z" } } },
{ INPUT_ANALOG_LIGHTGUN_X, "lightgunx", { { "stick", "x" }, { "relative", "x" } } },
{ INPUT_ANALOG_LIGHTGUN_Y, "lightguny", { { "stick", "y" }, { "relative", "y" } } },
{ INPUT_ANALOG_PEDAL, "pedalgas", { { "gas", "mono" } } },
{ INPUT_ANALOG_PEDAL2, "pedalbrake", { { "brake", "mono" } } },
{ INPUT_ANALOG_PEDAL3, "pedalother", { } },
{ INPUT_ANALOG_DIAL_X, "dialx", { { "relative", "dial" }, { "relative", "x" }, { "stick", "x" } } },
{ INPUT_ANALOG_DIAL_Y, "dialy", { { "relative", "y" }, { "stick", "y" } } },
{ INPUT_ANALOG_TRACKBALL_X, "trackballx", { { "relative", "x" }, { "stick", "x" } } },
{ INPUT_ANALOG_TRACKBALL_Y, "trackbally", { { "relative", "y" }, { "stick", "y" } } },
{ INPUT_ANALOG_MOUSE_X, "mousex", { { "relative", "x" }, { "stick", "x" } } },
{ INPUT_ANALOG_MOUSE_Y, "mousey", { { "relative", "y" }, { "stick", "y" } } },
{ 0, 0 }
};

/**************************************************************************/
/* Parse */

static adv_error parse_int(int* v, const char* s)
{
	char* e;

	*v = strtol(s, &e, 10);

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
		if (strcmp(joystickb_stick_name_get(joystick, i), s) == 0) {
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

	if (strcmp(s, "left")==0 || strcmp(s, "up")==0) {
		*v = 1;
		return 0;
	}

	if (strcmp(s, "right")==0 || strcmp(s, "down")==0) {
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
		if (first && strcmp(t, "auto")==0) {
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
				|| parse_joystick_stick_axe(&axe, v2, joystick, stick) != 0) {
				return -1;
			}

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
		} else if (strcmp(t, "mouse")==0 || strcmp(t, "-mouse")==0) {
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
				|| parse_joystick_rel(&axe, v1, joystick) != 0) {
				return -1;
			}

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
		if (first && strcmp(t, "auto")==0) {
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
				|| parse_key(&key, v1, board) != 0) {
				return -1;
			}

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
				|| parse_direction(&dir, v3) != 0) {
				return -1;
			}

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
				|| parse_joystick_button(&button, v1, joystick) != 0) {
				return -1;
			}

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
				|| parse_mouse_button(&button, v1, mouse) != 0) {
				return -1;
			}

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

void advance_input_print_digital(char* buffer, unsigned buffer_size, unsigned* seq_map, unsigned seq_max)
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
				sncatf(buffer, buffer_size, "keyboard[%d,%s]", DIGITAL_KBD_BOARD_GET(v), key_name(DIGITAL_KBD_KEY_GET(v)));
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
	for(i=0;input_code_map[i].name;++i) {
		if (input_code_map[i].oscode == code) {
			sncpy(input_code_map[i].name, INPUT_NAME_MAX, name);
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

adv_error advance_input_parse_analogname(unsigned* port, const char* buffer)
{
	struct mame_analog* i;

	if (strlen(buffer) < 3) {
		log_std(("WARNING:emu:input: unknown analog name %s\n", buffer));
		return -1;
	}

	for(i=mame_analog_list();i->name;++i)
		if (strcmp(i->name, buffer)==0)
			break;
	if (!i->name) {
		log_std(("WARNING:emu:input: unknown analog name %s\n", buffer));
		return -1;
	}

	*port = i->port;

	return 0;
}

adv_error advance_input_parse_analogvalue(int* keydelta, int* sensitivity, int* reverse, int* centerdelta, char* buffer)
{
	int p;
	unsigned n;

	p = 0;
	n = 0;
	while (buffer[p]) {
		char* token;
		const char* tag;
		const char* value;
		int q;
		char c;

		token = (char*)stoken(&c, &p, buffer, ",", "");
		if (c != ',' && c != 0)
			return -1;

		q = 0;
		tag = stoken(&c, &q, token, ":", "");
		if (c != ':' && c != 0)
			return -1;
		if (c == 0) { /* LEGACY for 0.84 */
			value = tag;
			switch (n) {
			case 0 : tag = "keydelta"; break;
			case 1 : tag = "sensitivity"; break;
			case 2 : tag = "reverse"; break;
			case 3 : tag = "centerdelta"; break;
			default : tag = "none"; break;
			}
		} else {
			value = stoken(&c, &q, token, "", "");
			if (c != 0)
				return -1;
		}

		if (strcmp(tag, "keydelta") == 0) {
			*keydelta = atoi(value);
			if (*keydelta < 1)
				*keydelta = 1;
			if (*keydelta > 255)
				*keydelta = 255;
		} else if (strcmp(tag, "sensitivity") == 0) {
			*sensitivity = atoi(value);
			if (*sensitivity < 1)
				*sensitivity = 1;
			if (*sensitivity > 255)
				*sensitivity = 255;
		} else if (strcmp(tag, "reverse") == 0) {
			if (strcmp(value, "reverse") == 0 || strcmp(value, "1") == 0)
				*reverse = 1;
			else if (strcmp(value, "noreverse") == 0 || strcmp(value, "0") == 0)
				*reverse = 0;
			else {
				/* ignore */
			}
		} else if (strcmp(tag, "centerdelta") == 0) {
			if (strcmp(value, "nocenter") == 0) { /* LEGACY for 0.84 */
				*centerdelta = 0;
			} else if (strcmp(value, "center") == 0) { /* LEGACY for 0.84 */
				/* nothing */
			} else {
				*centerdelta = atoi(value);
				if (*centerdelta < 0)
					*centerdelta = 0;
				if (*centerdelta > 255)
					*centerdelta = 255;
			}
		} else {
			/* ignore */
		}

		++n;
	}

	return 0;
}

void advance_input_print_analogvalue(char* buffer, unsigned buffer_size, int delta, int sensitivity, int reverse, int centerdelta)
{
	snprintf(buffer, buffer_size, "keydelta:%d,centerdelta:%d,sensitivity:%d,reverse:%d",
		delta,
		centerdelta,
		sensitivity,
		reverse != 0
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

static unsigned search_analog_control(unsigned player, const char* stick_name, const char* axe_name, unsigned* map, unsigned mac, unsigned max)
{
	if (strcmp(stick_name, "relative") == 0) {
		unsigned axe;

		if (player < mouseb_count_get()) {
			for(axe=0;axe<mouseb_axe_count_get(player);++axe) {
				if (strcmp(axe_name, mouseb_axe_name_get(player, axe)) == 0) {
					if (mac < max) {
						map[mac] = ANALOG_MOUSE(player, axe, 0);
						++mac;
					}
				}
			}
		}

		if (player < joystickb_count_get()) {
			for(axe=0;axe<joystickb_rel_count_get(player);++axe) {
				if (strcmp(axe_name, joystickb_rel_name_get(player, axe)) == 0) {
					if (mac < max) {
						map[mac] = ANALOG_BALL(player, axe, 0);
						++mac;
					}
				}
			}
		}

	} else {
		unsigned axe;
		unsigned stick;

		if (player < joystickb_count_get()) {
			for(stick=0;stick<joystickb_stick_count_get(player);++stick) {
				if (strcmp(stick_name, joystickb_stick_name_get(player, stick)) == 0) {
					for(axe=0;axe<joystickb_stick_axe_count_get(player, stick);++axe) {
						if (strcmp(axe_name, joystickb_stick_axe_name_get(player, stick, axe)) == 0) {
							if (mac < max) {
								map[mac] = ANALOG_JOY(player, stick, axe, 0);
								++mac;
							}
						}
					}
				}
			}
		}
	}

	return mac;
}

static void input_setup_config(struct advance_input_context* context)
{
	unsigned i, j, k;

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		for(j=0;j<INPUT_ANALOG_MAX;++j) {
			if (context->config.analog_map[i][j].seq[0] == ANALOG_SPECIAL_AUTO) {
				unsigned mac;

				mac = 0;
				for(k=0;ANALOG_INFO[j].map[k].control;++k) {
					mac = search_analog_control(i, ANALOG_INFO[j].map[k].control, ANALOG_INFO[j].map[k].axe, context->config.analog_map[i][j].seq, mac, INPUT_MAP_MAX - 1);
				}
				context->config.analog_map[i][j].seq[mac] = ANALOG_SPECIAL_NONE;
			}
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
			for(k=0;k<joystickb_stick_axe_count_get(i, j);++k) {
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
			log_std(("emu:input: input analog mapping player:%d control:%s :", i, ANALOG_INFO[j].name));
			for(k=0;k<INPUT_MAP_MAX;++k) {
				unsigned v = context->config.analog_map[i][j].seq[k];
				if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_JOY) {
					unsigned d = ANALOG_JOY_DEV_GET(v);
					unsigned s = ANALOG_JOY_STICK_GET(v);
					unsigned a = ANALOG_JOY_AXE_GET(v);
					adv_bool negate = ANALOG_JOY_NEGATE_GET(v);
					if (negate)
						log_std((" -joystick[%d,%d,%d]", d, s, a));
					else
						log_std((" joystick[%d,%d,%d]", d, s, a));
				} else if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
					unsigned d = ANALOG_MOUSE_DEV_GET(v);
					unsigned a = ANALOG_MOUSE_AXE_GET(v);
					adv_bool negate = ANALOG_MOUSE_NEGATE_GET(v);
					if (negate)
						log_std((" -mouse[%d,%d]", d, a));
					else
						log_std((" mouse[%d,%d]", d, a));
				} else if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_BALL) {
					unsigned d = ANALOG_BALL_DEV_GET(v);
					unsigned a = ANALOG_BALL_AXE_GET(v);
					adv_bool negate = ANALOG_BALL_NEGATE_GET(v);
					if (negate)
						log_std((" -joystick_ball[%d,%d]", d, a));
					else
						log_std((" joystick_ball[%d,%d]", d, a));
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

	/* fill the code vector */
	mac = 0;

	/* add mouse buttons */
	for(i=0;i<INPUT_MOUSE_MAX;++i) {
		for(j=0;j<INPUT_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_DIGITAL_MAX) {
				if (i<mouseb_count_get() && j<mouseb_button_count_get(i)) {
					if (i == 0)
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "m:%s", mouseb_button_name_get(i, j));
					else
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "m%d:%s", i+1, mouseb_button_name_get(i, j));
				} else {
					if (i == 0)
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "m:button%d", j+1);
					else
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "m%d:button%d", i+1, j+1);
				}
				input_code_map[mac].name = input_name_map[mac];
				input_code_map[mac].oscode = DIGITAL_MOUSE_BUTTON(i, j);
				input_code_map[mac].inputcode = CODE_OTHER_DIGITAL;
				++mac;
			} else {
				log_std(("emu:input: ERROR full input code vector\n"));
			}
		}
	}

	/* add joystick buttons/axes */
	for(i=0;i<INPUT_JOY_MAX;++i) {
		for(j=0;j<INPUT_STICK_MAX;++j) {
			for(k=0;k<INPUT_AXE_MAX;++k) {
				if (mac+1 < INPUT_DIGITAL_MAX) {
					if (i<joystickb_count_get() && j<joystickb_stick_count_get(i) && k<joystickb_stick_axe_count_get(i, j)) {
						if (i == 0)
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j:%s:%s-", joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
						else
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j%d:%s:%s-", i+1, joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					} else {
						if (i == 0)
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j:%d:%d-", j+1, k+1);
						else
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j%d:%d:%d-", i+1, j+1, k+1);
					}
					input_code_map[mac].name = input_name_map[mac];
					input_code_map[mac].oscode = DIGITAL_JOY(i, j, k, 0);
					input_code_map[mac].inputcode = CODE_OTHER_DIGITAL;
					++mac;
				} else {
					log_std(("emu:input: ERROR full input code vector\n"));
				}
				if (mac+1 < INPUT_DIGITAL_MAX) {
					if (i<joystickb_count_get() && j<joystickb_stick_count_get(i) && k<joystickb_stick_axe_count_get(i, j)) {
						if (i == 0)
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j:%s:%s+", joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
						else
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j%d:%s:%s+", i+1, joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					} else {
						if (i == 0)
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j:%d:%d+", j+1, k+1);
						else
							snprintf(input_name_map[mac], INPUT_NAME_MAX, "j%d:%d:%d+", i+1, j+1, k+1);
					}
					input_code_map[mac].name = input_name_map[mac];
					input_code_map[mac].oscode = DIGITAL_JOY(i, j, k, 1);
					input_code_map[mac].inputcode = CODE_OTHER_DIGITAL;
					++mac;
				} else {
					log_std(("emu:input: ERROR full input code vector\n"));
				}
			}
		}

		for(j=0;j<INPUT_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_DIGITAL_MAX) {
				if (i<joystickb_count_get() && j<joystickb_button_count_get(i)) {
					if (i == 0)
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "j:%s", joystickb_button_name_get(i, j));
					else
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "j%d:%s", i+1, joystickb_button_name_get(i, j));
				} else {
					if (i == 0)
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "j:button%d", j+1);
					else
						snprintf(input_name_map[mac], INPUT_NAME_MAX, "j%d:button%d", i+1, j+1);
				}
				input_code_map[mac].name = input_name_map[mac];
				input_code_map[mac].oscode = DIGITAL_JOY_BUTTON(i, j);
				input_code_map[mac].inputcode = CODE_OTHER_DIGITAL;
				++mac;
			} else {
				log_std(("emu:input: ERROR full input code vector\n"));
			}
		}
	}

	for(i=0;i<INPUT_KEYBOARD_MAX;++i) {
		for(j=0;j<KEYB_MAX;++j) {
			if (mac+1 < INPUT_DIGITAL_MAX) {
				if (i == 0)
					snprintf(input_name_map[mac], sizeof(input_name_map[mac]), "%s", key_name(j));
				else
					snprintf(input_name_map[mac], sizeof(input_name_map[mac]), "k%d:%s", i+1, key_name(j));
				input_code_map[mac].name = input_name_map[mac];
				input_code_map[mac].oscode = DIGITAL_KBD(i, j);
				input_code_map[mac].inputcode = CODE_OTHER_DIGITAL;
				++mac;
			} else {
				log_std(("emu:input: ERROR full input code vector\n"));
			}
		}
	}

	/* terminate the keyboard vector */
	input_code_map[mac].name = 0;
	input_code_map[mac].oscode = 0;
	input_code_map[mac].inputcode = 0;

	/* set the known inputcodes */
	for(i=0;i<mac;++i) {
		for(j=0;j<sizeof(input_equiv_map)/sizeof(input_equiv_map[0]);++j) {
			if (input_equiv_map[j].os_code == input_code_map[i].oscode) {
				input_code_map[i].inputcode = input_equiv_map[j].mame_code;
				break;
			}
		}
	}

	log_std(("emu:input: input code vector %d\n", mac));
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
	memset(&context->state.key_old, 0, sizeof(context->state.key_old));
	memset(&context->state.key_current, 0, sizeof(context->state.key_current));

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
			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[p%d_%s]", i+1, ANALOG_INFO[j].name);

			s = conf_string_get_default(cfg_context, tag_buffer);
			d = strdup(s);

			if (parse_analog(context->config.analog_map[i][j].seq, d)!=0) {
				free(d);
				target_err("Invalid argument '%s' for option '%s'.\n%s.\n", s, tag_buffer, error_get());
				target_err("Valid format is [-]joystick[JOYSTICK,STICK,AXE]/[-]mouse[MOUSE,AXE]/[-]joystick_ball[JOYSTICK,AXE].\n");
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

	return 0;
}

static adv_error input_load_name(struct advance_input_context* context, adv_conf* cfg_context)
{
	adv_conf_iterator k;
	
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
			snprintf(tag_buffer, sizeof(tag_buffer), "input_map[p%d_%s]", i+1, ANALOG_INFO[j].name);
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

	joystickb_reg(cfg_context, 1);
	joystickb_reg_driver_all(cfg_context);
	mouseb_reg(cfg_context, 1);
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
	
	/* load the customized names */
	if (input_load_name(context, cfg_context) != 0) {
		goto err_key;
	}

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

/**
 * Get the analog control input for the specified player and control
 * Analog absolute inputs return a value between -65536 and +65536.
 * \param player Player.
 * \param control Control to read. One of INPUT_ANALOG_*.
 * \param value Value read.
 * \return Type of value returned.
 */
static int advance_input_analog_read(struct advance_input_context* context, unsigned player, unsigned control, int* value)
{
	unsigned n;
	int absolute = 0;
	int relative = 0;
	unsigned last;

	adv_bool at_least_one_absolute = 0;
	adv_bool at_least_one_relative = 0;

	assert(context->state.active_flag != 0);

	if (player >= INPUT_PLAYER_MAX || control >= INPUT_ANALOG_MAX) {
		*value = 0;
		return ANALOG_TYPE_NONE;
	}

	for(n=0;n<INPUT_MAP_MAX;++n) {
		unsigned v = context->config.analog_map[player][control].seq[n];
		if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_JOY) {
			unsigned j = ANALOG_JOY_DEV_GET(v);
			unsigned s = ANALOG_JOY_STICK_GET(v);
			unsigned a = ANALOG_JOY_AXE_GET(v);
			adv_bool negate = ANALOG_JOY_NEGATE_GET(v);
			if (j < INPUT_JOY_MAX && s < INPUT_STICK_MAX && a < INPUT_AXE_MAX) {
				int z;
				if (negate)
					absolute -= context->state.joystick_analog_current[j][s][a];
				else
					absolute += context->state.joystick_analog_current[j][s][a];
				at_least_one_absolute = 1;
			}
		} else if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
			unsigned m = ANALOG_MOUSE_DEV_GET(v);
			unsigned a = ANALOG_MOUSE_AXE_GET(v);
			adv_bool negate = ANALOG_MOUSE_NEGATE_GET(v);
			if (m < INPUT_MOUSE_MAX && a < INPUT_AXE_MAX) {
				if (negate)
					relative -= context->state.mouse_analog_current[m][a];
				else
					relative += context->state.mouse_analog_current[m][a];
				at_least_one_relative = 1;
			}
		} else if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_BALL) {
			unsigned j = ANALOG_BALL_DEV_GET(v);
			unsigned a = ANALOG_BALL_AXE_GET(v);
			adv_bool negate = ANALOG_BALL_NEGATE_GET(v);
			if (j < INPUT_JOY_MAX && a < INPUT_AXE_MAX) {
				if (negate)
					relative -= context->state.ball_analog_current[j][a];
				else
					relative += context->state.ball_analog_current[j][a];
				at_least_one_relative = 1;
			}
		} else {
			break;
		}
	}

	/* limit the range */
	if (absolute < -65536)
		absolute = -65536;
	if (absolute > 65536)
		absolute = 65536;

	/* adjust from 1 step for pixel to 512 steps for pixels */
	relative *= 512;

	/* limit the range also if relative to prevent overflow in the MAME code */
	if (relative < -65536)
		relative = -65536;
	if (relative > 65536)
		relative = 65536;

	last = context->config.analog_map[player][control].last;

	if (last == INPUT_ANALOG_ABSOLUTE && absolute != 0) {
		*value = absolute;
		return ANALOG_TYPE_ABSOLUTE;
	}

	if (last == INPUT_ANALOG_RELATIVE && relative != 0) {
		*value = relative;
		return ANALOG_TYPE_RELATIVE;
	}

	if (absolute != 0) {
		context->config.analog_map[player][control].last = INPUT_ANALOG_ABSOLUTE;
		*value = absolute;
		return ANALOG_TYPE_ABSOLUTE;
	}

	if (relative != 0) {
		context->config.analog_map[player][control].last = INPUT_ANALOG_RELATIVE;
		*value = relative;
		return ANALOG_TYPE_RELATIVE;
	}

	if (last == INPUT_ANALOG_ABSOLUTE && at_least_one_absolute) {
		*value = 0;
		return ANALOG_TYPE_ABSOLUTE;
	}

	if (last == INPUT_ANALOG_RELATIVE && at_least_one_relative) {
		*value = relative;
		return ANALOG_TYPE_RELATIVE;
	}

	if (at_least_one_absolute) {
		*value = 0;
		return ANALOG_TYPE_ABSOLUTE;
	}

	if (at_least_one_relative) {
		*value = relative;
		return ANALOG_TYPE_RELATIVE;
	}

	*value = 0;
	return ANALOG_TYPE_NONE;
}

/***************************************************************************/
/* OSD interface */

/**
 * Return a list of all available input codes.
 */
const os_code_info *osd_get_code_list(void)
{
	struct advance_input_context* context = &CONTEXT.input;

	assert(context->state.active_flag != 0);

	return input_code_map;
}

/**
 * Read the value of a input code.
 * Return the value of the specified input.
 * Digital inputs return 0 or 1.
 * \param oscode is the OS dependent code specified in the list
 * returned by osd_get_code_list().
 */
INT32 osd_get_code_value(os_code oscode)
{
	struct advance_input_context* context = &CONTEXT.input;

	log_debug(("emu:input: osd_get_code_value(code:0x%08x)\n", oscode));

	return advance_input_digital_pressed(context, oscode);
}

INT32 osd_get_analog_value(unsigned type, unsigned player, int* analog_type)
{
	struct advance_input_context* context = &CONTEXT.input;
	int value_type;
	int value;

	log_debug(("emu:input: osd_get_analog_value(type:%d, player:%d)\n", type, player));

	switch (type) {
	case IPT_PADDLE :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_PADDLE_X, &value);
		break;
	case IPT_PADDLE_V :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_PADDLE_Y, &value);
		break;
	case IPT_AD_STICK_X :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_STICK_X, &value);
		break;
	case IPT_AD_STICK_Y :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_STICK_Y, &value);
		break;
	case IPT_AD_STICK_Z :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_STICK_Z, &value);
		break;
	case IPT_LIGHTGUN_X :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_LIGHTGUN_X, &value);
		break;
	case IPT_LIGHTGUN_Y :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_LIGHTGUN_Y, &value);
		break;
	case IPT_PEDAL :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_PEDAL, &value);
		break;
	case IPT_PEDAL2 :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_PEDAL2, &value);
		break;
	case IPT_PEDAL3 :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_PEDAL3, &value);
		break;
	case IPT_DIAL :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_DIAL_X, &value);
		break;
	case IPT_DIAL_V :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_DIAL_Y, &value);
		break;
	case IPT_TRACKBALL_X :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_TRACKBALL_X, &value);
		break;
	case IPT_TRACKBALL_Y :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_TRACKBALL_Y, &value);
		break;
	case IPT_MOUSE_X :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_MOUSE_X, &value);
		break;
	case IPT_MOUSE_Y :
		value_type = advance_input_analog_read(context, player, INPUT_ANALOG_MOUSE_Y, &value);
		break;
	default:
		log_std(("ERROR:input: invalid port type %d\n", type));
		value_type = ANALOG_TYPE_NONE;
		value = 0;
		break;
	}

	log_debug(("emu:input: osd_get_analog_value() -> %d, %s\n", value, value_type == ANALOG_TYPE_RELATIVE ? "relative" : value_type == ANALOG_TYPE_ABSOLUTE ? "absolute" : "none"));

	*analog_type = value_type;
	return value;
}

/**
 * Get a unicode press.
 * \param flush If !=0 flush the keyboard buffer flush and return 0.
 * \return
 * - == 0 No key in the keyboard buffer.
 * - != 0 Unicode key pressed.
 */
int osd_readkey_unicode(int flush)
{
	/* no unicode support */
	return 0;
}

/**
 * Check if the joystick need calibration.
 * The calibration is started only on user request.
 */
int osd_joystick_needs_calibration(void)
{
	return 1;
}

/**
 * Start the calibration process.
 */
void osd_joystick_start_calibration(void)
{
	joystickb_calib_start();
}

/**
 * Next step of the calibration.
 * \return
 * - != 0 Message to display at the user for the next step of the calibrarion.
 * - == 0 End the calibration process.
 */
const char* osd_joystick_calibrate_next(void)
{
	return joystickb_calib_next();
}

/**
 * Called during the calibration.
 * Use this for polling if required.
 */
void osd_joystick_calibrate(void)
{
	/* nothing */
}

/**
 * End of the calibration.
 */
void osd_joystick_end_calibration(void)
{
	/* nothing */
}

#ifdef MESS
/**
 * Check if the emulated keyboard input need to be disabled.
 * This function disables the emulated keyboard input.
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


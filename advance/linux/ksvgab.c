/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

#include "ksvgab.h"
#include "log.h"
#include "error.h"
#include "oslinux.h"

#include <vgakeyboard.h>

#define LOW_INVALID ((unsigned)0xFFFFFFFF)

struct keyb_svgalib_context {
	unsigned map_up_to_low[KEYB_MAX];
	adv_bool disable_special_flag;
};

static struct keyb_pair {
	unsigned up_code;
	unsigned low_code;
} KEYS[] = {
{ KEYB_A, SCANCODE_A },
{ KEYB_B, SCANCODE_B },
{ KEYB_C, SCANCODE_C },
{ KEYB_D, SCANCODE_D },
{ KEYB_E, SCANCODE_E },
{ KEYB_F, SCANCODE_F },
{ KEYB_G, SCANCODE_G },
{ KEYB_H, SCANCODE_H },
{ KEYB_I, SCANCODE_I },
{ KEYB_J, SCANCODE_J },
{ KEYB_K, SCANCODE_K },
{ KEYB_L, SCANCODE_L },
{ KEYB_M, SCANCODE_M },
{ KEYB_N, SCANCODE_N },
{ KEYB_O, SCANCODE_O },
{ KEYB_P, SCANCODE_P },
{ KEYB_Q, SCANCODE_Q },
{ KEYB_R, SCANCODE_R },
{ KEYB_S, SCANCODE_S },
{ KEYB_T, SCANCODE_T },
{ KEYB_U, SCANCODE_U },
{ KEYB_V, SCANCODE_V },
{ KEYB_W, SCANCODE_W },
{ KEYB_X, SCANCODE_X },
{ KEYB_Y, SCANCODE_Y },
{ KEYB_Z, SCANCODE_Z },
{ KEYB_0, SCANCODE_0 },
{ KEYB_1, SCANCODE_1 },
{ KEYB_2, SCANCODE_2 },
{ KEYB_3, SCANCODE_3 },
{ KEYB_4, SCANCODE_4 },
{ KEYB_5, SCANCODE_5 },
{ KEYB_6, SCANCODE_6 },
{ KEYB_7, SCANCODE_7 },
{ KEYB_8, SCANCODE_8 },
{ KEYB_9, SCANCODE_9 },
{ KEYB_0_PAD, SCANCODE_KEYPAD0 },
{ KEYB_1_PAD, SCANCODE_KEYPAD1 },
{ KEYB_2_PAD, SCANCODE_KEYPAD2 },
{ KEYB_3_PAD, SCANCODE_KEYPAD3 },
{ KEYB_4_PAD, SCANCODE_KEYPAD4 },
{ KEYB_5_PAD, SCANCODE_KEYPAD5 },
{ KEYB_6_PAD, SCANCODE_KEYPAD6 },
{ KEYB_7_PAD, SCANCODE_KEYPAD7 },
{ KEYB_8_PAD, SCANCODE_KEYPAD8 },
{ KEYB_9_PAD, SCANCODE_KEYPAD9 },
{ KEYB_F1, SCANCODE_F1 },
{ KEYB_F2, SCANCODE_F2 },
{ KEYB_F3, SCANCODE_F3 },
{ KEYB_F4, SCANCODE_F4 },
{ KEYB_F5, SCANCODE_F5 },
{ KEYB_F6, SCANCODE_F6 },
{ KEYB_F7, SCANCODE_F7 },
{ KEYB_F8, SCANCODE_F8 },
{ KEYB_F9, SCANCODE_F9 },
{ KEYB_F10, SCANCODE_F10 },
{ KEYB_F11, SCANCODE_F11 },
{ KEYB_F12, SCANCODE_F12 },
{ KEYB_ESC, SCANCODE_ESCAPE },
{ KEYB_BACKQUOTE, SCANCODE_GRAVE },
{ KEYB_MINUS, SCANCODE_MINUS },
{ KEYB_EQUALS, SCANCODE_EQUAL },
{ KEYB_BACKSPACE, SCANCODE_BACKSPACE },
{ KEYB_TAB, SCANCODE_TAB },
{ KEYB_OPENBRACE, SCANCODE_BRACKET_LEFT },
{ KEYB_CLOSEBRACE, SCANCODE_BRACKET_RIGHT },
{ KEYB_ENTER, SCANCODE_ENTER },
{ KEYB_SEMICOLON, SCANCODE_SEMICOLON },
{ KEYB_QUOTE, SCANCODE_APOSTROPHE },
{ KEYB_BACKSLASH, SCANCODE_BACKSLASH },
{ KEYB_LESS, SCANCODE_LESS },
{ KEYB_COMMA, SCANCODE_COMMA },
{ KEYB_PERIOD, SCANCODE_PERIOD },
{ KEYB_SLASH, SCANCODE_SLASH },
{ KEYB_SPACE, SCANCODE_SPACE },
{ KEYB_INSERT, SCANCODE_INSERT },
{ KEYB_DEL, SCANCODE_REMOVE },
{ KEYB_HOME, SCANCODE_HOME },
{ KEYB_END, SCANCODE_END },
{ KEYB_PGUP, SCANCODE_PAGEUP },
{ KEYB_PGDN, SCANCODE_PAGEDOWN },
{ KEYB_LEFT, SCANCODE_CURSORBLOCKLEFT },
{ KEYB_RIGHT, SCANCODE_CURSORBLOCKRIGHT },
{ KEYB_UP, SCANCODE_CURSORBLOCKUP },
{ KEYB_DOWN, SCANCODE_CURSORBLOCKDOWN },
{ KEYB_SLASH_PAD, SCANCODE_KEYPADDIVIDE },
{ KEYB_ASTERISK, SCANCODE_KEYPADMULTIPLY },
{ KEYB_MINUS_PAD, SCANCODE_KEYPADMINUS },
{ KEYB_PLUS_PAD, SCANCODE_KEYPADPLUS },
{ KEYB_PERIOD_PAD, SCANCODE_KEYPADPERIOD },
{ KEYB_ENTER_PAD, SCANCODE_KEYPADENTER },
{ KEYB_PRTSCR, SCANCODE_PRINTSCREEN },
{ KEYB_PAUSE, SCANCODE_BREAK },
{ KEYB_LSHIFT, SCANCODE_LEFTSHIFT },
{ KEYB_RSHIFT, SCANCODE_RIGHTSHIFT },
{ KEYB_LCONTROL, SCANCODE_LEFTCONTROL },
{ KEYB_RCONTROL, SCANCODE_RIGHTCONTROL },
{ KEYB_ALT, SCANCODE_LEFTALT },
{ KEYB_ALTGR, SCANCODE_RIGHTALT },
{ KEYB_LWIN, SCANCODE_LEFTWIN },
{ KEYB_RWIN, SCANCODE_RIGHTWIN },
{ KEYB_MENU, 127 /* Not defined by SVGALIB */ },
{ KEYB_SCRLOCK, SCANCODE_SCROLLLOCK },
{ KEYB_NUMLOCK, SCANCODE_NUMLOCK },
{ KEYB_CAPSLOCK, SCANCODE_CAPSLOCK },
{ KEYB_MAX, 0 }
};

static struct keyb_svgalib_context svgalib_state;

static adv_device DEVICE[] = {
{ "auto", -1, "SVGALIB keyboard" },
{ 0, 0, 0 }
};

adv_error keyb_svgalib_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* i;
	unsigned j;

	log_std(("keyb:svgalib: keyb_svgalib_init(id:%d, disable_special:%d)\n", keyb_id, (int)disable_special));

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

	if (!os_internal_svgalib_get()) {
		error_set("Not supported without the svgalib library.\n");
		return -1;
	}

	for(j=0;j<KEYB_MAX;++j) {
		svgalib_state.map_up_to_low[j] = LOW_INVALID;
	}
	for(i=KEYS;i->up_code != KEYB_MAX;++i) {
		svgalib_state.map_up_to_low[i->up_code] = i->low_code;
	}

	svgalib_state.disable_special_flag = disable_special;

	return 0;
}

void keyb_svgalib_done(void)
{
	log_std(("keyb:svgalib: keyb_svgalib_done()\n"));
}

adv_error keyb_svgalib_enable(adv_bool graphics)
{
	log_std(("keyb:svgalib: keyb_svgalib_enable(graphics:%d)\n", (int)graphics));

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

#if defined(USE_VIDEO_SDL)
	if (os_internal_sdl_is_video_active()) {
		error_set("The svgalib keyboard driver cannot be used with the SDL video driver.\n");
		return -1;
	}
#endif

	if (keyboard_init() != 0) {
		error_set("Error enabling the svgalib keyboard driver. Function keyboard_init() failed.\n");
		return -1;
	}

	if (svgalib_state.disable_special_flag) {
		keyboard_translatekeys(DONT_CATCH_CTRLC);
	}

	keyboard_clearstate();

	return 0;
}

void keyb_svgalib_disable(void)
{
	log_std(("keyb:svgalib: keyb_svgalib_disable()\n"));

	keyboard_close();
}

unsigned keyb_svgalib_count_get(void)
{
	log_debug(("keyb:svgalib: keyb_svgalib_count_get(void)\n"));

	return 1;
}

adv_bool keyb_svgalib_has(unsigned keyboard, unsigned code)
{
	log_debug(("keyb:svgalib: keyb_svgalib_has()\n"));

	return svgalib_state.map_up_to_low[code] != LOW_INVALID;
}

unsigned keyb_svgalib_get(unsigned keyboard, unsigned code)
{
	unsigned low_code;

	log_debug(("keyb:svgalib: keyb_svgalib_get(keyboard:%d,code:%d)\n", keyboard, code));

	/* disable the pause key */
	if (code == KEYB_PAUSE)
		return 0;

	low_code = svgalib_state.map_up_to_low[code];
	if (low_code == LOW_INVALID)
		return 0;

	return keyboard_keypressed(low_code) != KEY_NOTPRESSED;
}

void keyb_svgalib_all_get(unsigned keyboard, unsigned char* code_map)
{
	unsigned i;
	const char* state;

	log_debug(("keyb:svgalib: keyb_svgalib_all_get(keyboard:%d)\n", keyboard));

	state = keyboard_getstate();

	for(i=0;i<KEYB_MAX;++i) {
		unsigned low_code = svgalib_state.map_up_to_low[i];
		if (low_code == LOW_INVALID)
			code_map[i] = 0;
		else
			code_map[i] = state[low_code] != KEY_NOTPRESSED;
	}

	/* disable the pause key */
	code_map[KEYB_PAUSE] = 0;
}

void keyb_svgalib_led_set(unsigned keyboard, unsigned led_mask)
{
	log_debug(("keyb:svgalib: keyb_svgalib_led_set(keyboard:%d,mask:%d)\n", keyboard, led_mask));

	/* TODO led support */
}

void keyb_svgalib_poll(void)
{
	log_debug(("keyb:svgalib: keyb_svgalib_poll()\n"));

	keyboard_update();
}

unsigned keyb_svgalib_flags(void)
{
	return 0;
}

adv_error keyb_svgalib_load(adv_conf* context)
{
	return 0;
}

void keyb_svgalib_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

keyb_driver keyb_svgalib_driver = {
	"svgalib",
	DEVICE,
	keyb_svgalib_load,
	keyb_svgalib_reg,
	keyb_svgalib_init,
	keyb_svgalib_done,
	keyb_svgalib_enable,
	keyb_svgalib_disable,
	keyb_svgalib_flags,
	keyb_svgalib_count_get,
	keyb_svgalib_has,
	keyb_svgalib_get,
	keyb_svgalib_all_get,
	keyb_svgalib_led_set,
	keyb_svgalib_poll
};


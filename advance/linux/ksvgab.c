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

#include "ksvgab.h"
#include "log.h"
#include "oslinux.h"

#include <vgakeyboard.h>

#ifdef USE_VIDEO_SDL
#include "SDL.h"
#endif

struct keyb_svgalib_context {
	unsigned map_os_to_code[OS_KEY_MAX];
};

static struct keyb_pair {
	int os;
	int code;
} KEYS[] = {
{ OS_KEY_A, SCANCODE_A },
{ OS_KEY_B, SCANCODE_B },
{ OS_KEY_C, SCANCODE_C },
{ OS_KEY_D, SCANCODE_D },
{ OS_KEY_E, SCANCODE_E },
{ OS_KEY_F, SCANCODE_F },
{ OS_KEY_G, SCANCODE_G },
{ OS_KEY_H, SCANCODE_H },
{ OS_KEY_I, SCANCODE_I },
{ OS_KEY_J, SCANCODE_J },
{ OS_KEY_K, SCANCODE_K },
{ OS_KEY_L, SCANCODE_L },
{ OS_KEY_M, SCANCODE_M },
{ OS_KEY_N, SCANCODE_N },
{ OS_KEY_O, SCANCODE_O },
{ OS_KEY_P, SCANCODE_P },
{ OS_KEY_Q, SCANCODE_Q },
{ OS_KEY_R, SCANCODE_R },
{ OS_KEY_S, SCANCODE_S },
{ OS_KEY_T, SCANCODE_T },
{ OS_KEY_U, SCANCODE_U },
{ OS_KEY_V, SCANCODE_V },
{ OS_KEY_W, SCANCODE_W },
{ OS_KEY_X, SCANCODE_X },
{ OS_KEY_Y, SCANCODE_Y },
{ OS_KEY_Z, SCANCODE_Z },
{ OS_KEY_0, SCANCODE_0 },
{ OS_KEY_1, SCANCODE_1 },
{ OS_KEY_2, SCANCODE_2 },
{ OS_KEY_3, SCANCODE_3 },
{ OS_KEY_4, SCANCODE_4 },
{ OS_KEY_5, SCANCODE_5 },
{ OS_KEY_6, SCANCODE_6 },
{ OS_KEY_7, SCANCODE_7 },
{ OS_KEY_8, SCANCODE_8 },
{ OS_KEY_9, SCANCODE_9 },
{ OS_KEY_0_PAD, SCANCODE_KEYPAD0 },
{ OS_KEY_1_PAD, SCANCODE_KEYPAD1 },
{ OS_KEY_2_PAD, SCANCODE_KEYPAD2 },
{ OS_KEY_3_PAD, SCANCODE_KEYPAD3 },
{ OS_KEY_4_PAD, SCANCODE_KEYPAD4 },
{ OS_KEY_5_PAD, SCANCODE_KEYPAD5 },
{ OS_KEY_6_PAD, SCANCODE_KEYPAD6 },
{ OS_KEY_7_PAD, SCANCODE_KEYPAD7 },
{ OS_KEY_8_PAD, SCANCODE_KEYPAD8 },
{ OS_KEY_9_PAD, SCANCODE_KEYPAD9 },
{ OS_KEY_F1, SCANCODE_F1 },
{ OS_KEY_F2, SCANCODE_F2 },
{ OS_KEY_F3, SCANCODE_F3 },
{ OS_KEY_F4, SCANCODE_F4 },
{ OS_KEY_F5, SCANCODE_F5 },
{ OS_KEY_F6, SCANCODE_F6 },
{ OS_KEY_F7, SCANCODE_F7 },
{ OS_KEY_F8, SCANCODE_F8 },
{ OS_KEY_F9, SCANCODE_F9 },
{ OS_KEY_F10, SCANCODE_F10 },
{ OS_KEY_F11, SCANCODE_F11 },
{ OS_KEY_F12, SCANCODE_F12 },
{ OS_KEY_ESC, SCANCODE_ESCAPE },
{ OS_KEY_BACKQUOTE, SCANCODE_GRAVE },
{ OS_KEY_MINUS, SCANCODE_MINUS },
{ OS_KEY_EQUALS, SCANCODE_EQUAL },
{ OS_KEY_BACKSPACE, SCANCODE_BACKSPACE },
{ OS_KEY_TAB, SCANCODE_TAB },
{ OS_KEY_OPENBRACE, SCANCODE_BRACKET_LEFT },
{ OS_KEY_CLOSEBRACE, SCANCODE_BRACKET_RIGHT },
{ OS_KEY_ENTER, SCANCODE_ENTER },
{ OS_KEY_SEMICOLON, SCANCODE_SEMICOLON },
{ OS_KEY_QUOTE, SCANCODE_APOSTROPHE },
{ OS_KEY_BACKSLASH, SCANCODE_BACKSLASH },
{ OS_KEY_LESS, SCANCODE_LESS },
{ OS_KEY_COMMA, SCANCODE_COMMA },
{ OS_KEY_PERIOD, SCANCODE_PERIOD },
{ OS_KEY_SLASH, SCANCODE_SLASH },
{ OS_KEY_SPACE, SCANCODE_SPACE },
{ OS_KEY_INSERT, SCANCODE_INSERT },
{ OS_KEY_DEL, SCANCODE_REMOVE },
{ OS_KEY_HOME, SCANCODE_HOME },
{ OS_KEY_END, SCANCODE_END },
{ OS_KEY_PGUP, SCANCODE_PAGEUP },
{ OS_KEY_PGDN, SCANCODE_PAGEDOWN },
{ OS_KEY_LEFT, SCANCODE_CURSORBLOCKLEFT },
{ OS_KEY_RIGHT, SCANCODE_CURSORBLOCKRIGHT },
{ OS_KEY_UP, SCANCODE_CURSORBLOCKUP },
{ OS_KEY_DOWN, SCANCODE_CURSORBLOCKDOWN },
{ OS_KEY_SLASH_PAD, SCANCODE_KEYPADDIVIDE },
{ OS_KEY_ASTERISK, SCANCODE_KEYPADMULTIPLY },
{ OS_KEY_MINUS_PAD, SCANCODE_KEYPADMINUS },
{ OS_KEY_PLUS_PAD, SCANCODE_KEYPADPLUS },
{ OS_KEY_PERIOD_PAD, SCANCODE_KEYPADPERIOD },
{ OS_KEY_ENTER_PAD, SCANCODE_KEYPADENTER },
{ OS_KEY_PRTSCR, SCANCODE_PRINTSCREEN },
{ OS_KEY_PAUSE, SCANCODE_BREAK },
{ OS_KEY_LSHIFT, SCANCODE_LEFTSHIFT },
{ OS_KEY_RSHIFT, SCANCODE_RIGHTSHIFT },
{ OS_KEY_LCONTROL, SCANCODE_LEFTCONTROL },
{ OS_KEY_RCONTROL, SCANCODE_RIGHTCONTROL },
{ OS_KEY_ALT, SCANCODE_LEFTALT },
{ OS_KEY_ALTGR, SCANCODE_RIGHTALT },
{ OS_KEY_LWIN, SCANCODE_LEFTWIN },
{ OS_KEY_RWIN, SCANCODE_RIGHTWIN },
{ OS_KEY_MENU, 127 /* Not defined by SVGALIB */ },
{ OS_KEY_SCRLOCK, SCANCODE_SCROLLLOCK },
{ OS_KEY_NUMLOCK, SCANCODE_NUMLOCK },
{ OS_KEY_CAPSLOCK, SCANCODE_CAPSLOCK },
{ OS_KEY_MAX, 0 }
};

static struct keyb_svgalib_context svgalib_state;

static device DEVICE[] = {
{ "auto", -1, "SVGALIB keyboard" },
{ 0, 0, 0 }
};

adv_error keyb_svgalib_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* i;
	unsigned j;

	log_std(("key:svgalib: keyb_svgalib_init(id:%d,disable_special:%d)\n",keyb_id,(int)disable_special));

	if (!os_internal_svgalib_get()) {
		log_std(("keyb:svgalib: svgalib not initialized\n"));
		error_description_nolog_cat("svgalib: Not supported without the svgalib library\n");
		return -1;
	}
	
#ifdef USE_VIDEO_SDL
	if (SDL_WasInit(SDL_INIT_VIDEO)) {
		log_std(("keyb:svgalib: Incompatible with the SDL video driver\n"));
		error_description_nolog_cat("svgalib: Incompatible with the SDL video driver\n");
		return -1; 
	}
#endif

	for(j=0;j<OS_KEY_MAX;++j) {
		svgalib_state.map_os_to_code[j] = 0;
	}
	for(i=KEYS;i->os != OS_KEY_MAX;++i) {
		svgalib_state.map_os_to_code[i->os] = i->code;
	}

	if (keyboard_init() != 0) {
		log_std(("keyb:svgalib: keyboard_init() failed\n"));
		return -1;
	}

	return 0;
}

void keyb_svgalib_done(void)
{
	log_std(("keyb:svgalib: keyb_svgalib_done()\n"));

	keyboard_close();
}

unsigned keyb_svgalib_get(unsigned code)
{
	unsigned svgalib_code;

	assert( code < OS_KEY_MAX);

	log_debug(("keyb:svgalib: keyb_svgalib_get(code:%d)\n",code));

	/* disable the pause key */
	if (code == OS_KEY_PAUSE)
		return 0;

	svgalib_code = svgalib_state.map_os_to_code[code];

	log_debug(("keyb:svgalib: keyb_svgalib_get() svgalib_code:%d\n",svgalib_code));

	if (!svgalib_code)
		return 0;

	return keyboard_keypressed(svgalib_code);
}

void keyb_svgalib_all_get(unsigned char* code_map)
{
	unsigned i;

	log_debug(("keyb:svgalib: keyb_svgalib_all_get()\n"));

	for(i=0;i<OS_KEY_MAX;++i) {
		unsigned svgalib_code = svgalib_state.map_os_to_code[i];
		if (svgalib_code)
			code_map[i] = keyboard_keypressed(svgalib_code);
		else
			code_map[i] = 0;
	}

	/* disable the pause key */
	code_map[OS_KEY_PAUSE] = 0;
}

void keyb_svgalib_poll()
{
	log_debug(("keyb:svgalib: keyb_svgalib_poll()\n"));

	keyboard_update();
}

unsigned keyb_svgalib_flags(void)
{
	return 0;
}

adv_error keyb_svgalib_load(struct conf_context* context)
{
	return 0;
}

void keyb_svgalib_reg(struct conf_context* context)
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
	keyb_svgalib_flags,
	keyb_svgalib_get,
	keyb_svgalib_all_get,
	keyb_svgalib_poll
};



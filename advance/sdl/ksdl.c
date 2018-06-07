/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2008 Andrea Mazzoleni
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "portable.h"

#include "ksdl.h"
#include "log.h"
#include "error.h"
#include "ossdl.h"

#include "SDL.h"

#ifdef __WIN32__
#include "oswin.h"
#endif

#define LOW_INVALID ((unsigned)0xFFFFFFFF)

#if SDL_MAJOR_VERSION == 1
#define SDL_CODE_MAX SDLK_LAST
#else
#define SDL_CODE_MAX SDL_NUM_SCANCODES
#endif

struct keyb_sdl_context {
	unsigned map_up_to_low[KEYB_MAX];
	unsigned char state[SDL_CODE_MAX];
};

static struct keyb_pair {
	unsigned up_code;
	unsigned low_code;
} KEYS[] = {
#if SDL_MAJOR_VERSION == 1 /* SDL1 uses key codes */
	{ KEYB_A, SDLK_a },
	{ KEYB_B, SDLK_b },
	{ KEYB_C, SDLK_c },
	{ KEYB_D, SDLK_d },
	{ KEYB_E, SDLK_e },
	{ KEYB_F, SDLK_f },
	{ KEYB_G, SDLK_g },
	{ KEYB_H, SDLK_h },
	{ KEYB_I, SDLK_i },
	{ KEYB_J, SDLK_j },
	{ KEYB_K, SDLK_k },
	{ KEYB_L, SDLK_l },
	{ KEYB_M, SDLK_m },
	{ KEYB_N, SDLK_n },
	{ KEYB_O, SDLK_o },
	{ KEYB_P, SDLK_p },
	{ KEYB_Q, SDLK_q },
	{ KEYB_R, SDLK_r },
	{ KEYB_S, SDLK_s },
	{ KEYB_T, SDLK_t },
	{ KEYB_U, SDLK_u },
	{ KEYB_V, SDLK_v },
	{ KEYB_W, SDLK_w },
	{ KEYB_X, SDLK_x },
	{ KEYB_Y, SDLK_y },
	{ KEYB_Z, SDLK_z },
	{ KEYB_0, SDLK_0 },
	{ KEYB_1, SDLK_1 },
	{ KEYB_2, SDLK_2 },
	{ KEYB_3, SDLK_3 },
	{ KEYB_4, SDLK_4 },
	{ KEYB_5, SDLK_5 },
	{ KEYB_6, SDLK_6 },
	{ KEYB_7, SDLK_7 },
	{ KEYB_8, SDLK_8 },
	{ KEYB_9, SDLK_9 },
	{ KEYB_0_PAD, SDLK_KP0 },
	{ KEYB_1_PAD, SDLK_KP1 },
	{ KEYB_2_PAD, SDLK_KP2 },
	{ KEYB_3_PAD, SDLK_KP3 },
	{ KEYB_4_PAD, SDLK_KP4 },
	{ KEYB_5_PAD, SDLK_KP5 },
	{ KEYB_6_PAD, SDLK_KP6 },
	{ KEYB_7_PAD, SDLK_KP7 },
	{ KEYB_8_PAD, SDLK_KP8 },
	{ KEYB_9_PAD, SDLK_KP9 },
	{ KEYB_F1, SDLK_F1 },
	{ KEYB_F2, SDLK_F2 },
	{ KEYB_F3, SDLK_F3 },
	{ KEYB_F4, SDLK_F4 },
	{ KEYB_F5, SDLK_F5 },
	{ KEYB_F6, SDLK_F6 },
	{ KEYB_F7, SDLK_F7 },
	{ KEYB_F8, SDLK_F8 },
	{ KEYB_F9, SDLK_F9 },
	{ KEYB_F10, SDLK_F10 },
	{ KEYB_F11, SDLK_F11 },
	{ KEYB_F12, SDLK_F12 },
	{ KEYB_ESC, SDLK_ESCAPE },
	{ KEYB_BACKQUOTE, SDLK_BACKQUOTE },
	{ KEYB_MINUS, SDLK_MINUS },
	{ KEYB_EQUALS, SDLK_EQUALS },
	{ KEYB_BACKSPACE, SDLK_BACKSPACE },
	{ KEYB_TAB, SDLK_TAB },
	{ KEYB_OPENBRACE, SDLK_LEFTBRACKET },
	{ KEYB_CLOSEBRACE, SDLK_RIGHTBRACKET },
	{ KEYB_ENTER, SDLK_RETURN },
	{ KEYB_SEMICOLON, SDLK_SEMICOLON },
	{ KEYB_QUOTE, SDLK_QUOTE },
	{ KEYB_BACKSLASH, SDLK_BACKSLASH },
	{ KEYB_LESS, SDLK_LESS },
	{ KEYB_COMMA, SDLK_COMMA },
	{ KEYB_PERIOD, SDLK_PERIOD },
	{ KEYB_SLASH, SDLK_SLASH },
	{ KEYB_SPACE, SDLK_SPACE },
	{ KEYB_INSERT, SDLK_INSERT },
	{ KEYB_DEL, SDLK_DELETE },
	{ KEYB_HOME, SDLK_HOME },
	{ KEYB_END, SDLK_END },
	{ KEYB_PGUP, SDLK_PAGEUP },
	{ KEYB_PGDN, SDLK_PAGEDOWN },
	{ KEYB_LEFT, SDLK_LEFT },
	{ KEYB_RIGHT, SDLK_RIGHT },
	{ KEYB_UP, SDLK_UP },
	{ KEYB_DOWN, SDLK_DOWN },
	{ KEYB_SLASH_PAD, SDLK_KP_DIVIDE },
	{ KEYB_ASTERISK, SDLK_KP_MULTIPLY },
	{ KEYB_MINUS_PAD, SDLK_KP_MINUS },
	{ KEYB_PLUS_PAD, SDLK_KP_PLUS },
	{ KEYB_PERIOD_PAD, SDLK_KP_PERIOD },
	{ KEYB_ENTER_PAD, SDLK_KP_ENTER },
	{ KEYB_PRTSCR, SDLK_PRINT },
	{ KEYB_PAUSE, SDLK_PAUSE },
	{ KEYB_LSHIFT, SDLK_LSHIFT },
	{ KEYB_RSHIFT, SDLK_RSHIFT },
	{ KEYB_LCONTROL, SDLK_LCTRL },
	{ KEYB_RCONTROL, SDLK_RCTRL },
	{ KEYB_ALT, SDLK_LALT },
	{ KEYB_ALTGR, SDLK_RALT },
	{ KEYB_LWIN, SDLK_LMETA },
	{ KEYB_RWIN, SDLK_COMPOSE },
	{ KEYB_MENU, SDLK_MENU },
	{ KEYB_SCRLOCK, SDLK_SCROLLOCK },
	{ KEYB_NUMLOCK, SDLK_NUMLOCK },
	{ KEYB_CAPSLOCK, SDLK_CAPSLOCK },
#else /* SDL2 uses scancodes */
	{ KEYB_A, SDL_SCANCODE_A },
	{ KEYB_B, SDL_SCANCODE_B },
	{ KEYB_C, SDL_SCANCODE_C },
	{ KEYB_D, SDL_SCANCODE_D },
	{ KEYB_E, SDL_SCANCODE_E },
	{ KEYB_F, SDL_SCANCODE_F },
	{ KEYB_G, SDL_SCANCODE_G },
	{ KEYB_H, SDL_SCANCODE_H },
	{ KEYB_I, SDL_SCANCODE_I },
	{ KEYB_J, SDL_SCANCODE_J },
	{ KEYB_K, SDL_SCANCODE_K },
	{ KEYB_L, SDL_SCANCODE_L },
	{ KEYB_M, SDL_SCANCODE_M },
	{ KEYB_N, SDL_SCANCODE_N },
	{ KEYB_O, SDL_SCANCODE_O },
	{ KEYB_P, SDL_SCANCODE_P },
	{ KEYB_Q, SDL_SCANCODE_Q },
	{ KEYB_R, SDL_SCANCODE_R },
	{ KEYB_S, SDL_SCANCODE_S },
	{ KEYB_T, SDL_SCANCODE_T },
	{ KEYB_U, SDL_SCANCODE_U },
	{ KEYB_V, SDL_SCANCODE_V },
	{ KEYB_W, SDL_SCANCODE_W },
	{ KEYB_X, SDL_SCANCODE_X },
	{ KEYB_Y, SDL_SCANCODE_Y },
	{ KEYB_Z, SDL_SCANCODE_Z },
	{ KEYB_0, SDL_SCANCODE_0 },
	{ KEYB_1, SDL_SCANCODE_1 },
	{ KEYB_2, SDL_SCANCODE_2 },
	{ KEYB_3, SDL_SCANCODE_3 },
	{ KEYB_4, SDL_SCANCODE_4 },
	{ KEYB_5, SDL_SCANCODE_5 },
	{ KEYB_6, SDL_SCANCODE_6 },
	{ KEYB_7, SDL_SCANCODE_7 },
	{ KEYB_8, SDL_SCANCODE_8 },
	{ KEYB_9, SDL_SCANCODE_9 },
	{ KEYB_0_PAD, SDL_SCANCODE_KP_0 },
	{ KEYB_1_PAD, SDL_SCANCODE_KP_1 },
	{ KEYB_2_PAD, SDL_SCANCODE_KP_2 },
	{ KEYB_3_PAD, SDL_SCANCODE_KP_3 },
	{ KEYB_4_PAD, SDL_SCANCODE_KP_4 },
	{ KEYB_5_PAD, SDL_SCANCODE_KP_5 },
	{ KEYB_6_PAD, SDL_SCANCODE_KP_6 },
	{ KEYB_7_PAD, SDL_SCANCODE_KP_7 },
	{ KEYB_8_PAD, SDL_SCANCODE_KP_8 },
	{ KEYB_9_PAD, SDL_SCANCODE_KP_9 },
	{ KEYB_F1, SDL_SCANCODE_F1 },
	{ KEYB_F2, SDL_SCANCODE_F2 },
	{ KEYB_F3, SDL_SCANCODE_F3 },
	{ KEYB_F4, SDL_SCANCODE_F4 },
	{ KEYB_F5, SDL_SCANCODE_F5 },
	{ KEYB_F6, SDL_SCANCODE_F6 },
	{ KEYB_F7, SDL_SCANCODE_F7 },
	{ KEYB_F8, SDL_SCANCODE_F8 },
	{ KEYB_F9, SDL_SCANCODE_F9 },
	{ KEYB_F10, SDL_SCANCODE_F10 },
	{ KEYB_F11, SDL_SCANCODE_F11 },
	{ KEYB_F12, SDL_SCANCODE_F12 },
	{ KEYB_ESC, SDL_SCANCODE_ESCAPE },
	{ KEYB_BACKQUOTE, SDL_SCANCODE_GRAVE },
	{ KEYB_MINUS, SDL_SCANCODE_MINUS },
	{ KEYB_EQUALS, SDL_SCANCODE_EQUALS },
	{ KEYB_BACKSPACE, SDL_SCANCODE_BACKSPACE },
	{ KEYB_TAB, SDL_SCANCODE_TAB },
	{ KEYB_OPENBRACE, SDL_SCANCODE_LEFTBRACKET },
	{ KEYB_CLOSEBRACE, SDL_SCANCODE_RIGHTBRACKET },
	{ KEYB_ENTER, SDL_SCANCODE_RETURN },
	{ KEYB_SEMICOLON, SDL_SCANCODE_SEMICOLON },
	{ KEYB_QUOTE, SDL_SCANCODE_APOSTROPHE },
	{ KEYB_BACKSLASH, SDL_SCANCODE_BACKSLASH },
	{ KEYB_LESS, SDL_SCANCODE_NONUSBACKSLASH },
	{ KEYB_COMMA, SDL_SCANCODE_COMMA },
	{ KEYB_PERIOD, SDL_SCANCODE_PERIOD },
	{ KEYB_SLASH, SDL_SCANCODE_SLASH },
	{ KEYB_SPACE, SDL_SCANCODE_SPACE },
	{ KEYB_INSERT, SDL_SCANCODE_INSERT },
	{ KEYB_DEL, SDL_SCANCODE_DELETE },
	{ KEYB_HOME, SDL_SCANCODE_HOME },
	{ KEYB_END, SDL_SCANCODE_END },
	{ KEYB_PGUP, SDL_SCANCODE_PAGEUP },
	{ KEYB_PGDN, SDL_SCANCODE_PAGEDOWN },
	{ KEYB_LEFT, SDL_SCANCODE_LEFT },
	{ KEYB_RIGHT, SDL_SCANCODE_RIGHT },
	{ KEYB_UP, SDL_SCANCODE_UP },
	{ KEYB_DOWN, SDL_SCANCODE_DOWN },
	{ KEYB_SLASH_PAD, SDL_SCANCODE_KP_DIVIDE },
	{ KEYB_ASTERISK, SDL_SCANCODE_KP_MULTIPLY },
	{ KEYB_MINUS_PAD, SDL_SCANCODE_KP_MINUS },
	{ KEYB_PLUS_PAD, SDL_SCANCODE_KP_PLUS },
	{ KEYB_PERIOD_PAD, SDL_SCANCODE_KP_PERIOD },
	{ KEYB_ENTER_PAD, SDL_SCANCODE_KP_ENTER },
	{ KEYB_PRTSCR, SDL_SCANCODE_PRINTSCREEN },
	{ KEYB_PAUSE, SDL_SCANCODE_PAUSE },
	{ KEYB_LSHIFT, SDL_SCANCODE_LSHIFT },
	{ KEYB_RSHIFT, SDL_SCANCODE_RSHIFT },
	{ KEYB_LCONTROL, SDL_SCANCODE_LCTRL },
	{ KEYB_RCONTROL, SDL_SCANCODE_RCTRL },
	{ KEYB_ALT, SDL_SCANCODE_LALT },
	{ KEYB_ALTGR, SDL_SCANCODE_RALT },
	{ KEYB_LWIN, SDL_SCANCODE_LGUI },
	{ KEYB_RWIN, SDL_SCANCODE_RGUI },
	{ KEYB_MENU, SDL_SCANCODE_MENU },
	{ KEYB_SCRLOCK, SDL_SCANCODE_SCROLLLOCK },
	{ KEYB_NUMLOCK, SDL_SCANCODE_NUMLOCKCLEAR },
	{ KEYB_CAPSLOCK, SDL_SCANCODE_CAPSLOCK },
#endif
	{ KEYB_MAX, 0 }
};

static struct keyb_sdl_context sdl_state;

static adv_device DEVICE[] = {
	{ "auto", -1, "SDL keyboard" },
	{ 0, 0, 0 }
};

adv_error keyb_sdl_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* i;
	unsigned j;

	log_std(("keyb:sdl: keyb_sdl_init(id:%d, disable_special:%d)\n", keyb_id, (int)disable_special));

	if (!os_internal_sdl_get()) {
		error_set("Unsupported without the SDL library.\n");
		return -1;
	}

	/* SDL requires a video mode set with a window to have keyboard working */
	if (!os_internal_sdl_is_video_active()) {
		error_set("The SDL keyboard driver requires the SDL video driver.\n");
		return -1;
	}

	for (j = 0; j < KEYB_MAX; ++j) {
		sdl_state.map_up_to_low[j] = LOW_INVALID;
	}
	for (i = KEYS; i->up_code != KEYB_MAX; ++i) {
		sdl_state.map_up_to_low[i->up_code] = i->low_code;
	}
	for (j = 0; j < SDL_CODE_MAX; ++j) {
		sdl_state.state[j] = 0;
	}

#ifdef __WIN32__
	// disable hot keys
	if (disable_special) {
		os_internal_ignore_hot_key();
	}
#endif

	return 0;
}

void keyb_sdl_done(void)
{
	log_std(("keyb:sdl: keyb_sdl_done()\n"));

#ifdef __WIN32__
	// called uncoditionally, the internal logic take care of it
	os_internal_restore_hot_key();
#endif
}

adv_error keyb_sdl_enable(adv_bool graphics)
{
	log_std(("keyb:sdl: keyb_sdl_enable(graphics:%d)\n", (int)graphics));

	return 0;
}

void keyb_sdl_disable(void)
{
	log_std(("keyb:sdl: keyb_sdl_disable()\n"));
}

unsigned keyb_sdl_count_get(void)
{
	log_debug(("keyb:sdl: keyb_sdl_count_get(void)\n"));

	return 1;
}

adv_bool keyb_sdl_has(unsigned keyboard, unsigned code)
{
	log_debug(("keyb:sdl: keyb_sdl_has()\n"));

	return sdl_state.map_up_to_low[code] != LOW_INVALID;
}

unsigned keyb_sdl_get(unsigned keyboard, unsigned code)
{
	unsigned low_code;

	log_debug(("keyb:sdl: keyb_sdl_get(keyboard:%d,code:%d)\n", keyboard, code));

	/* disable the pause key */
	if (code == KEYB_PAUSE)
		return 0;

	low_code = sdl_state.map_up_to_low[code];
	if (low_code == LOW_INVALID)
		return 0;

	return sdl_state.state[low_code];
}

void keyb_sdl_all_get(unsigned keyboard, unsigned char* code_map)
{
	unsigned i;

	log_debug(("keyb:sdl: keyb_sdl_all_get(keyboard:%d)\n", keyboard));

	for (i = 0; i < KEYB_MAX; ++i) {
		unsigned low_code = sdl_state.map_up_to_low[i];
		if (low_code == LOW_INVALID)
			code_map[i] = 0;
		else
			code_map[i] = sdl_state.state[low_code];
	}

	/* disable the pause key */
	code_map[KEYB_PAUSE] = 0;
}

int keyb_sdl_poll(void)
{
	log_debug(("keyb:sdl: keyb_sdl_poll()\n"));

	/* the polling is done in the os interface which call the keyb_sdl_event_* function */

	return 0;
}

unsigned keyb_sdl_flags(void)
{
	return 0;
}

adv_error keyb_sdl_load(adv_conf* context)
{
	return 0;
}

void keyb_sdl_reg(adv_conf* context)
{
}

void keyb_sdl_event_press(unsigned code)
{
	if (code < SDL_CODE_MAX)
		sdl_state.state[code] = 1;
}

void keyb_sdl_event_release(unsigned code)
{
	if (code < SDL_CODE_MAX)
		sdl_state.state[code] = 0;
}

void keyb_sdl_event_release_all(void)
{
	unsigned j;

	for (j = 0; j < SDL_CODE_MAX; ++j) {
		sdl_state.state[j] = 0;
	}
}

/***************************************************************************/
/* Driver */

keyb_driver keyb_sdl_driver = {
	"sdl",
	DEVICE,
	keyb_sdl_load,
	keyb_sdl_reg,
	keyb_sdl_init,
	keyb_sdl_done,
	keyb_sdl_enable,
	keyb_sdl_disable,
	keyb_sdl_flags,
	keyb_sdl_count_get,
	keyb_sdl_has,
	keyb_sdl_get,
	keyb_sdl_all_get,
	0,
	keyb_sdl_poll
};


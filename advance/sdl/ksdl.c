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

#include "ksdl.h"
#include "log.h"
#include "error.h"

#include "SDL.h"

struct keyb_sdl_context {
	unsigned map_os_to_code[KEYB_MAX];
	unsigned char state[SDLK_LAST];
};

static struct keyb_pair {
	int os;
	int code;
} KEYS[] = {
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
{ KEYB_MAX, 0 }
};

static struct keyb_sdl_context sdl_state;

static device DEVICE[] = {
{ "auto", -1, "SDL keyboard" },
{ 0, 0, 0 }
};

error keyb_sdl_init(int keyb_id, boolean disable_special)
{
	struct keyb_pair* i;
	unsigned j;

	log_std(("keyb:sdl: keyb_sdl_init(id:%d,disable_special:%d)\n",keyb_id,(int)disable_special));
	 
	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		log_std(("keyb:sdl: not supported without the SDL video driver\n"));
		error_nolog_cat("sdl: Not supported without the SDL video driver\n");
		return -1; 
	}

	for(j=0;j<KEYB_MAX;++j) {
		sdl_state.map_os_to_code[j] = 0;
	}
	for(i=KEYS;i->os != KEYB_MAX;++i) {
		sdl_state.map_os_to_code[i->os] = i->code;
	}
	for(j=0;j<SDLK_LAST;++j) {
		sdl_state.state[j] = 0;
	}

	return 0;
}

void keyb_sdl_done(void)
{
	log_std(("keyb:sdl: keyb_sdl_done()\n"));
}

unsigned keyb_sdl_get(unsigned code)
{
	unsigned sdl_code;

	assert(code < KEYB_MAX);

	log_debug(("keyb:sdl: keyb_sdl_get(code:%d)\n",code));

	/* disable the pause key */
	if (code == KEYB_PAUSE)
		return 0;

	sdl_code = sdl_state.map_os_to_code[code];

	log_debug(("keyb:sdl: keyb_sdl_get() sdl_code:%d\n",sdl_code));

	if (!sdl_code)
		return 0;

	return sdl_state.state[sdl_code];
}

void keyb_sdl_all_get(unsigned char* code_map)
{
	unsigned i;

	log_debug(("keyb:sdl: keyb_sdl_all_get()\n"));

	for(i=0;i<KEYB_MAX;++i) {
		unsigned sdl_code = sdl_state.map_os_to_code[i];
		if (sdl_code)
			code_map[i] = sdl_state.state[sdl_code];
		else
			code_map[i] = 0;
	}

	/* disable the pause key */
	code_map[KEYB_PAUSE] = 0;
}

void keyb_sdl_poll()
{
	log_debug(("keyb:sdl: keyb_sdl_poll()\n"));
}

unsigned keyb_sdl_flags(void)
{
	return 0;
}

error keyb_sdl_load(struct conf_context* context)
{
	return 0;
}

void keyb_sdl_reg(struct conf_context* context)
{
}

void keyb_sdl_event_press(unsigned code) {
	if (code < SDLK_LAST)
		sdl_state.state[code] = 1;
}

void keyb_sdl_event_release(unsigned code) {
	if (code < SDLK_LAST)
		sdl_state.state[code] = 0;
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
	keyb_sdl_flags,
	keyb_sdl_get,
	keyb_sdl_all_get,
	keyb_sdl_poll
};



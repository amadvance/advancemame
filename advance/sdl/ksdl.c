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

#include "SDL.h"

struct keyb_sdl_context {
	unsigned map_os_to_code[OS_KEY_MAX];
	unsigned char state[SDLK_LAST];
};

static struct keyb_pair {
	int os;
	int code;
} KEYS[] = {
{ OS_KEY_A, SDLK_a },
{ OS_KEY_B, SDLK_b },
{ OS_KEY_C, SDLK_c },
{ OS_KEY_D, SDLK_d },
{ OS_KEY_E, SDLK_e },
{ OS_KEY_F, SDLK_f },
{ OS_KEY_G, SDLK_g },
{ OS_KEY_H, SDLK_h },
{ OS_KEY_I, SDLK_i },
{ OS_KEY_J, SDLK_j },
{ OS_KEY_K, SDLK_k },
{ OS_KEY_L, SDLK_l },
{ OS_KEY_M, SDLK_m },
{ OS_KEY_N, SDLK_n },
{ OS_KEY_O, SDLK_o },
{ OS_KEY_P, SDLK_p },
{ OS_KEY_Q, SDLK_q },
{ OS_KEY_R, SDLK_r },
{ OS_KEY_S, SDLK_s },
{ OS_KEY_T, SDLK_t },
{ OS_KEY_U, SDLK_u },
{ OS_KEY_V, SDLK_v },
{ OS_KEY_W, SDLK_w },
{ OS_KEY_X, SDLK_x },
{ OS_KEY_Y, SDLK_y },
{ OS_KEY_Z, SDLK_z },
{ OS_KEY_0, SDLK_0 },
{ OS_KEY_1, SDLK_1 },
{ OS_KEY_2, SDLK_2 },
{ OS_KEY_3, SDLK_3 },
{ OS_KEY_4, SDLK_4 },
{ OS_KEY_5, SDLK_5 },
{ OS_KEY_6, SDLK_6 },
{ OS_KEY_7, SDLK_7 },
{ OS_KEY_8, SDLK_8 },
{ OS_KEY_9, SDLK_9 },
{ OS_KEY_0_PAD, SDLK_KP0 },
{ OS_KEY_1_PAD, SDLK_KP1 },
{ OS_KEY_2_PAD, SDLK_KP2 },
{ OS_KEY_3_PAD, SDLK_KP3 },
{ OS_KEY_4_PAD, SDLK_KP4 },
{ OS_KEY_5_PAD, SDLK_KP5 },
{ OS_KEY_6_PAD, SDLK_KP6 },
{ OS_KEY_7_PAD, SDLK_KP7 },
{ OS_KEY_8_PAD, SDLK_KP8 },
{ OS_KEY_9_PAD, SDLK_KP9 },
{ OS_KEY_F1, SDLK_F1 },
{ OS_KEY_F2, SDLK_F2 },
{ OS_KEY_F3, SDLK_F3 },
{ OS_KEY_F4, SDLK_F4 },
{ OS_KEY_F5, SDLK_F5 },
{ OS_KEY_F6, SDLK_F6 },
{ OS_KEY_F7, SDLK_F7 },
{ OS_KEY_F8, SDLK_F8 },
{ OS_KEY_F9, SDLK_F9 },
{ OS_KEY_F10, SDLK_F10 },
{ OS_KEY_F11, SDLK_F11 },
{ OS_KEY_F12, SDLK_F12 },
{ OS_KEY_ESC, SDLK_ESCAPE },
{ OS_KEY_BACKQUOTE, SDLK_BACKQUOTE },
{ OS_KEY_MINUS, SDLK_MINUS },
{ OS_KEY_EQUALS, SDLK_EQUALS },
{ OS_KEY_BACKSPACE, SDLK_BACKSPACE },
{ OS_KEY_TAB, SDLK_TAB },
{ OS_KEY_OPENBRACE, SDLK_LEFTBRACKET },
{ OS_KEY_CLOSEBRACE, SDLK_RIGHTBRACKET },
{ OS_KEY_ENTER, SDLK_RETURN },
{ OS_KEY_SEMICOLON, SDLK_SEMICOLON },
{ OS_KEY_QUOTE, SDLK_QUOTE },
{ OS_KEY_BACKSLASH, SDLK_BACKSLASH },
{ OS_KEY_LESS, SDLK_LESS },
{ OS_KEY_COMMA, SDLK_COMMA },
{ OS_KEY_PERIOD, SDLK_PERIOD },
{ OS_KEY_SLASH, SDLK_SLASH },
{ OS_KEY_SPACE, SDLK_SPACE },
{ OS_KEY_INSERT, SDLK_INSERT },
{ OS_KEY_DEL, SDLK_DELETE },
{ OS_KEY_HOME, SDLK_HOME },
{ OS_KEY_END, SDLK_END },
{ OS_KEY_PGUP, SDLK_PAGEUP },
{ OS_KEY_PGDN, SDLK_PAGEDOWN },
{ OS_KEY_LEFT, SDLK_LEFT },
{ OS_KEY_RIGHT, SDLK_RIGHT },
{ OS_KEY_UP, SDLK_UP },
{ OS_KEY_DOWN, SDLK_DOWN },
{ OS_KEY_SLASH_PAD, SDLK_KP_DIVIDE },
{ OS_KEY_ASTERISK, SDLK_KP_MULTIPLY },
{ OS_KEY_MINUS_PAD, SDLK_KP_MINUS },
{ OS_KEY_PLUS_PAD, SDLK_KP_PLUS },
{ OS_KEY_PERIOD_PAD, SDLK_KP_PERIOD },
{ OS_KEY_ENTER_PAD, SDLK_KP_ENTER },
{ OS_KEY_PRTSCR, SDLK_PRINT },
{ OS_KEY_PAUSE, SDLK_PAUSE },
{ OS_KEY_LSHIFT, SDLK_LSHIFT },
{ OS_KEY_RSHIFT, SDLK_RSHIFT },
{ OS_KEY_LCONTROL, SDLK_LCTRL },
{ OS_KEY_RCONTROL, SDLK_RCTRL },
{ OS_KEY_ALT, SDLK_LALT },
{ OS_KEY_ALTGR, SDLK_RALT },
{ OS_KEY_LWIN, SDLK_LMETA },
{ OS_KEY_RWIN, SDLK_COMPOSE },
{ OS_KEY_MENU, SDLK_MENU },
{ OS_KEY_SCRLOCK, SDLK_SCROLLOCK },
{ OS_KEY_NUMLOCK, SDLK_NUMLOCK },
{ OS_KEY_CAPSLOCK, SDLK_CAPSLOCK },
{ OS_KEY_MAX, 0 }
};

static struct keyb_sdl_context sdl_state;

static device DEVICE[] = {
{ "auto", -1, "SDL keyboard" },
{ 0, 0, 0 }
};

adv_error keyb_sdl_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* i;
	unsigned j;

	log_std(("keyb:sdl: keyb_sdl_init(id:%d,disable_special:%d)\n",keyb_id,(int)disable_special));
	 
	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		log_std(("keyb:sdl: not supported without the SDL video driver\n"));
		error_description_nolog_cat("sdl: Not supported without the SDL video driver\n");
		return -1; 
	}

	for(j=0;j<OS_KEY_MAX;++j) {
		sdl_state.map_os_to_code[j] = 0;
	}
	for(i=KEYS;i->os != OS_KEY_MAX;++i) {
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

	assert(code < OS_KEY_MAX);

	log_debug(("keyb:sdl: keyb_sdl_get(code:%d)\n",code));

	/* disable the pause key */
	if (code == OS_KEY_PAUSE)
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

	for(i=0;i<OS_KEY_MAX;++i) {
		unsigned sdl_code = sdl_state.map_os_to_code[i];
		if (sdl_code)
			code_map[i] = sdl_state.state[sdl_code];
		else
			code_map[i] = 0;
	}

	/* disable the pause key */
	code_map[OS_KEY_PAUSE] = 0;
}

void keyb_sdl_poll()
{
	log_debug(("keyb:sdl: keyb_sdl_poll()\n"));
}

unsigned keyb_sdl_flags(void)
{
	return 0;
}

adv_error keyb_sdl_load(struct conf_context* context)
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



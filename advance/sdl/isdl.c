/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003 Andrea Mazzoleni
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

#include "isdl.h"
#include "log.h"
#include "target.h"
#include "error.h"
#include "ossdl.h"

#include "SDL.h"

struct inputb_sdl_context {
	unsigned last; /**< Last key pressed. */
	unsigned shift; /**< Shift state counter. If >0 the state is shifted. */
};

static struct inputb_sdl_context sdl_state;

static adv_device DEVICE[] = {
{ "auto", -1, "SDL input" },
{ 0, 0, 0 }
};

adv_error inputb_sdl_init(int inputb_id)
{
	log_std(("inputb:sdl: inputb_sdl_init(id:%d)\n", inputb_id));

	if (!os_internal_sdl_get()) {
		error_set("Unsupported without the SDL library.\n");
		return -1;
	}

	if (!os_internal_sdl_is_video_active()) {
		error_set("Not supported without the SDL video driver.\n");
		return -1; 
	}

	return 0;
}

void inputb_sdl_done(void)
{
	log_std(("inputb:sdl: inputb_sdl_done()\n"));
}

adv_error inputb_sdl_enable(adv_bool graphics)
{
	log_std(("inputb:sdl: inputb_sdl_enable(graphics:%d)\n", (int)graphics));

	if (!os_internal_sdl_is_video_active()) {
		error_set("Not supported without the SDL video driver.\n");
		return -1; 
	}

	sdl_state.last = SDLK_LAST;
	sdl_state.shift = 0;

	return 0;
}

void inputb_sdl_disable(void)
{
	log_std(("inputb:sdl: inputb_sdl_disable()\n"));
}

adv_bool inputb_sdl_hit(void)
{
	log_debug(("inputb:sdl: inputb_sdl_count_get()\n"));

	return sdl_state.last != SDLK_LAST;
}

unsigned inputb_sdl_get(void)
{
	unsigned r;
	int upper;

	log_debug(("inputb:sdl: inputb_sdl_get()\n"));

	r = sdl_state.last;
	sdl_state.last = SDLK_LAST;

	if (sdl_state.shift)
		upper = 'A' - 'a';
	else
		upper = 0;

	switch (r) {
	case SDLK_LAST : return INPUTB_NONE;
	case SDLK_COMMA : return ',';
	case SDLK_PERIOD : return '.';
	case SDLK_a : return 'a' + upper;
	case SDLK_b : return 'b' + upper;
	case SDLK_c : return 'c' + upper;
	case SDLK_d : return 'd' + upper;
	case SDLK_e : return 'e' + upper;
	case SDLK_f : return 'f' + upper;
	case SDLK_g : return 'g' + upper;
	case SDLK_h : return 'h' + upper;
	case SDLK_i : return 'i' + upper;
	case SDLK_j : return 'j' + upper;
	case SDLK_k : return 'k' + upper;
	case SDLK_l : return 'l' + upper;
	case SDLK_m : return 'm' + upper;
	case SDLK_n : return 'n' + upper;
	case SDLK_o : return 'o' + upper;
	case SDLK_p : return 'p' + upper;
	case SDLK_q : return 'q' + upper;
	case SDLK_r : return 'r' + upper;
	case SDLK_s : return 's' + upper;
	case SDLK_t : return 't' + upper;
	case SDLK_u : return 'u' + upper;
	case SDLK_v : return 'v' + upper;
	case SDLK_w : return 'w' + upper;
	case SDLK_x : return 'x' + upper;
	case SDLK_y : return 'y' + upper;
	case SDLK_z : return 'z' + upper;
	case SDLK_0 : return '0';
	case SDLK_1 : return '1';
	case SDLK_2 : return '2';
	case SDLK_3 : return '3';
	case SDLK_4 : return '4';
	case SDLK_5 : return '5';
	case SDLK_6 : return '6';
	case SDLK_7 : return '7';
	case SDLK_8 : return '8';
	case SDLK_9 : return '9';
	case SDLK_MINUS : return '-';
	case SDLK_PLUS : return '+';
	case SDLK_UNDERSCORE : return '_';
	case SDLK_TAB : return INPUTB_TAB;
	case SDLK_RETURN : return INPUTB_ENTER;
	case SDLK_ESCAPE : return INPUTB_ESC;
	case SDLK_SPACE : return INPUTB_SPACE;
	case SDLK_UP : return INPUTB_UP;
	case SDLK_DOWN : return INPUTB_DOWN;
	case SDLK_LEFT : return INPUTB_LEFT;
	case SDLK_RIGHT : return INPUTB_RIGHT;
	case SDLK_HOME : return INPUTB_HOME;
	case SDLK_END : return INPUTB_END;
	case SDLK_PAGEUP : return INPUTB_PGUP;
	case SDLK_PAGEDOWN : return INPUTB_PGDN;
	case SDLK_F1 : return INPUTB_F1;
	case SDLK_F2 : return INPUTB_F2;
	case SDLK_F3 : return INPUTB_F3;
	case SDLK_F4 : return INPUTB_F4;
	case SDLK_F5 : return INPUTB_F5;
	case SDLK_F6 : return INPUTB_F6;
	case SDLK_F7 : return INPUTB_F7;
	case SDLK_F8 : return INPUTB_F8;
	case SDLK_F9 : return INPUTB_F9;
	case SDLK_F10 : return INPUTB_F10;
	case SDLK_DELETE : return INPUTB_DEL;
	case SDLK_INSERT : return INPUTB_INS;
	case SDLK_BACKSPACE : return INPUTB_BACKSPACE;
	}

	return INPUTB_NONE;
}

void inputb_sdl_event_press(unsigned code)
{
	log_debug(("inputb:sdl: inputb_sdl_event_press(%d)\n", code));

	if (code == SDLK_RSHIFT || code == SDLK_LSHIFT)
		++sdl_state.shift;

	if (code < SDLK_LAST && sdl_state.last == SDLK_LAST) {
		sdl_state.last = code;
	}
}

void inputb_sdl_event_release(unsigned code)
{
	log_debug(("inputb:sdl: inputb_sdl_event_release(%d)\n", code));

	if (code == SDLK_RSHIFT || code == SDLK_LSHIFT) {
		if (sdl_state.shift > 0)
			--sdl_state.shift;
	}
}

unsigned inputb_sdl_flags(void)
{
	return 0;
}

adv_error inputb_sdl_load(adv_conf* context)
{
	return 0;
}

void inputb_sdl_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

inputb_driver inputb_sdl_driver = {
	"sdl",
	DEVICE,
	inputb_sdl_load,
	inputb_sdl_reg,
	inputb_sdl_init,
	inputb_sdl_done,
	inputb_sdl_enable,
	inputb_sdl_disable,
	inputb_sdl_flags,
	inputb_sdl_hit,
	inputb_sdl_get
};


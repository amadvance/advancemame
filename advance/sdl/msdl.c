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

#include "msdl.h"
#include "log.h"
#include "error.h"

#include "SDL.h"

struct mouseb_sdl_context {
	int button_map[3];
	int x;
	int y;
};

static struct mouseb_sdl_context sdl_state;

static adv_device DEVICE[] = {
{ "auto", -1, "SDL mouse" },
{ 0, 0, 0 }
};

adv_error mouseb_sdl_init(int mouseb_id)
{
	log_std(("mouseb:sdl: mouseb_sdl_init(id:%d)\n", mouseb_id));

	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		log_std(("mouseb:sdl: not supported without the SDL video driver\n"));
		error_nolog_cat("sdl: Not supported without the SDL video driver\n");
		return -1; 
	}

	sdl_state.button_map[0] = 0;
	sdl_state.button_map[1] = 0;
	sdl_state.button_map[2] = 0;
	sdl_state.x = 0;
	sdl_state.y = 0;

	return 0;
}

void mouseb_sdl_done(void)
{
	log_std(("mouseb:sdl: mouseb_sdl_done()\n"));
}

unsigned mouseb_sdl_count_get(void)
{
	log_debug(("mouseb:sdl: mouseb_sdl_count_get()\n"));

	return 1;
}

unsigned mouseb_sdl_button_count_get(unsigned m)
{
	log_debug(("mouseb:sdl: mouseb_sdl_button_count_get()\n"));

	assert( m < mouseb_sdl_count_get());

	return 3;
}

void mouseb_sdl_pos_get(unsigned m, int* x, int* y, int* z)
{
	log_debug(("mouseb:sdl: mouseb_sdl_pos_get()\n"));

	assert( m < mouseb_sdl_count_get());

	*x = sdl_state.x;
	*y = sdl_state.y;
	*z = 0;

	sdl_state.x = 0;
	sdl_state.y = 0;
}

unsigned mouseb_sdl_button_get(unsigned m, unsigned b)
{
	log_debug(("mouseb:sdl: mouseb_sdl_button_get()\n"));

	assert( m < mouseb_sdl_count_get());
	assert( b < mouseb_sdl_button_count_get(m) );

	return sdl_state.button_map[b];
}

void mouseb_sdl_poll(void)
{
	log_debug(("mouseb:sdl: mouseb_sdl_poll()\n"));
}

unsigned mouseb_sdl_flags(void)
{
	return 0;
}

adv_error mouseb_sdl_load(adv_conf* context)
{
	return 0;
}

void mouseb_sdl_reg(adv_conf* context)
{
}

void mouseb_sdl_event_move(int x, int y)
{
	sdl_state.x += x;
	sdl_state.y += y;
}

void mouseb_sdl_event_press(unsigned code)
{
	if (code < 3)
		sdl_state.button_map[code] = 1;
}

void mouseb_sdl_event_release(unsigned code)
{
	if (code < 3)
		sdl_state.button_map[code] = 0;
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_sdl_driver = {
	"sdl",
	DEVICE,
	mouseb_sdl_load,
	mouseb_sdl_reg,
	mouseb_sdl_init,
	mouseb_sdl_done,
	mouseb_sdl_flags,
	mouseb_sdl_count_get,
	mouseb_sdl_button_count_get,
	mouseb_sdl_pos_get,
	mouseb_sdl_button_get,
	mouseb_sdl_poll
};



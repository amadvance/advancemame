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

#include "jsdl.h"
#include "log.h"
#include "error.h"

#include "ossdl.h"

#include "SDL.h"

#define SDL_JOYSTICK_MAX 4 /**< Max number of joysticks */

struct joystickb_sdl_context {
	unsigned counter; /**< Number of joysticks active */
	SDL_Joystick* map[SDL_JOYSTICK_MAX];
};

static struct joystickb_sdl_context sdl_state;

static adv_device DEVICE[] = {
{ "auto", -1, "SDL joystick" },
{ 0, 0, 0 }
};

adv_error joystickb_sdl_init(int joystickb_id)
{
	unsigned i;

	log_std(("josytickb:sdl: joystickb_sdl_init(id:%d)\n", joystickb_id));

	if (!os_internal_sdl_get()) {
		error_set("Unsupported without the SDL library.\n");
		return -1;
	}

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0) {
		error_set("Function SDL_InitSubSystem(SDL_INIT_JOYSTICK) failed, %s.\n", SDL_GetError());
		goto err;
	}

	SDL_JoystickEventState(SDL_IGNORE);

	sdl_state.counter = SDL_NumJoysticks();
	if (sdl_state.counter > SDL_JOYSTICK_MAX)
		sdl_state.counter = SDL_JOYSTICK_MAX;

	if (sdl_state.counter == 0) {
		error_set("No joystick found.\n");
		goto err_quit;
	}

	for(i=0;i<sdl_state.counter;++i) {
		sdl_state.map[i] = SDL_JoystickOpen(i);
		if (!sdl_state.map[i]) {
			error_set("Function SDL_JoystickOpen(%d) failed, %s.\n", i, SDL_GetError());
			goto err_quit;
		}
	}

	return 0;

err_quit:
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
err:
	return -1;
}

void joystickb_sdl_done(void)
{
	unsigned i;

	log_std(("josytickb:sdl: joystickb_sdl_done()\n"));

	for(i=0;i<sdl_state.counter;++i)
		SDL_JoystickClose(sdl_state.map[i]);

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

unsigned joystickb_sdl_count_get(void)
{
	log_debug(("joystickb:sdl: joystickb_sdl_count_get()\n"));

	return sdl_state.counter;
}

unsigned joystickb_sdl_stick_count_get(unsigned joystick)
{
	log_debug(("joystickb:sdl: joystickb_sdl_stick_count_get()\n"));

	return 1;
}

unsigned joystickb_sdl_stick_axe_count_get(unsigned joystick, unsigned stick)
{
	log_debug(("joystickb:sdl: joystickb_sdl_stick_axe_count_get()\n"));

	return SDL_JoystickNumAxes(sdl_state.map[joystick]);
}

unsigned joystickb_sdl_button_count_get(unsigned joystick)
{
	log_debug(("joystickb:sdl: joystickb_sdl_button_count_get()\n"));

	return SDL_JoystickNumButtons(sdl_state.map[joystick]);
}

unsigned joystickb_sdl_button_get(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:sdl: joystickb_sdl_button_get()\n"));

	return SDL_JoystickGetButton(sdl_state.map[joystick], button) != 0;
}

int joystickb_sdl_stick_axe_analog_get(unsigned joystick, unsigned stick, unsigned axe)
{
	int r;
	log_debug(("joystickb:sdl: joystickb_sdl_stick_axe_analog_get()\n"));

	r = SDL_JoystickGetAxis(sdl_state.map[joystick], axe);

	r = joystickb_adjust_analog(r, -32768, 32768);

	return r;
}

unsigned joystickb_sdl_stick_axe_digital_get(unsigned joystick, unsigned stick, unsigned axe, unsigned d)
{
	int r;
	log_debug(("joystickb:sdl: joystickb_sdl_stick_axe_digital_get()\n"));

	r = joystickb_sdl_stick_axe_analog_get(joystick, stick, axe);

	if (d)
		return r < -JOYSTICK_DRIVER_BASE/8; /* -1/8 of the partial range */
	else
		return r > JOYSTICK_DRIVER_BASE/8; /* +1/8 of the partial range */
}

void joystickb_sdl_poll(void)
{
	log_debug(("josytickb:sdl: joystickb_sdl_poll()\n"));

	SDL_JoystickUpdate();
}

unsigned joystickb_sdl_flags(void)
{
	return 0;
}

adv_error joystickb_sdl_load(adv_conf* context)
{
	return 0;
}

void joystickb_sdl_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

joystickb_driver joystickb_sdl_driver = {
	"sdl",
	DEVICE,
	joystickb_sdl_load,
	joystickb_sdl_reg,
	joystickb_sdl_init,
	joystickb_sdl_done,
	0,
	0,
	joystickb_sdl_flags,
	joystickb_sdl_count_get,
	joystickb_sdl_stick_count_get,
	joystickb_sdl_stick_axe_count_get,
	0,
	0,
	joystickb_sdl_stick_axe_digital_get,
	joystickb_sdl_stick_axe_analog_get,
	joystickb_sdl_button_count_get,
	0,
	joystickb_sdl_button_get,
	0,
	0,
	0,
	0,
	0,
	joystickb_sdl_poll
};


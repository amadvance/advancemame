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

#include "jsdl.h"
#include "log.h"

#include "SDL.h"

#define JOYSTICK_MAX 4 /**< Max number of joysticks */

struct joystickb_sdl_context {
	unsigned counter; /**< Number of joysticks active */
	SDL_Joystick* map[JOYSTICK_MAX];
	char axe_name[32];
	char button_name[32];
	char stick_name[32];
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

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0)
		return -1;

	SDL_JoystickEventState(SDL_IGNORE);

	sdl_state.counter = SDL_NumJoysticks();
	if (sdl_state.counter > JOYSTICK_MAX)
		sdl_state.counter = JOYSTICK_MAX;

	for(i=0;i<sdl_state.counter;++i) {
		sdl_state.map[i] = SDL_JoystickOpen(i);
		if (!sdl_state.map[i])
			return -1;
	}

	return 0;
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

unsigned joystickb_sdl_stick_count_get(unsigned j)
{
	log_debug(("joystickb:sdl: joystickb_sdl_stick_count_get()\n"));

	assert(j < joystickb_sdl_count_get());

	return 1;
}

unsigned joystickb_sdl_stick_axe_count_get(unsigned j, unsigned s)
{
	log_debug(("joystickb:sdl: joystickb_sdl_stick_axe_count_get()\n"));

	assert(j < joystickb_sdl_count_get());
	assert(s < joystickb_sdl_stick_count_get(j) );

	(void)s;

	return SDL_JoystickNumAxes(sdl_state.map[j]);
}

unsigned joystickb_sdl_button_count_get(unsigned j)
{
	log_debug(("joystickb:sdl: joystickb_sdl_button_count_get()\n"));

	assert(j < joystickb_sdl_count_get());

	return SDL_JoystickNumButtons(sdl_state.map[j]);
}

const char* joystickb_sdl_stick_name_get(unsigned j, unsigned s)
{
	log_debug(("joystickb:sdl: joystickb_sdl_stick_name_get()\n"));

	assert(j < joystickb_sdl_count_get());
	assert(s < joystickb_sdl_stick_count_get(j) );

	(void)j;

	sprintf(sdl_state.stick_name, "S%d", s+1);

	return sdl_state.stick_name;
}

const char* joystickb_sdl_stick_axe_name_get(unsigned j, unsigned s, unsigned a)
{
	log_debug(("joystickb:sdl: joystickb_sdl_stick_axe_name_get()\n"));

	assert(j < joystickb_sdl_count_get());
	assert(s < joystickb_sdl_stick_count_get(j) );
	assert(a < joystickb_sdl_stick_axe_count_get(j, s) );

	(void)j;
	(void)s;

	sprintf(sdl_state.axe_name, "A%d", a+1);

	return sdl_state.axe_name;
}

const char* joystickb_sdl_button_name_get(unsigned j, unsigned b)
{
	log_debug(("joystickb:sdl: joystickb_sdl_button_name_get()\n"));
	(void)j;

	assert(j < joystickb_sdl_count_get());
	assert(b < joystickb_sdl_button_count_get(j) );

	sprintf(sdl_state.button_name, "B%d", b+1);

	return sdl_state.button_name;
}

unsigned joystickb_sdl_button_get(unsigned j, unsigned b)
{
	log_debug(("joystickb:sdl: joystickb_sdl_button_get()\n"));

	assert(j < joystickb_sdl_count_get());
	assert(b < joystickb_sdl_button_count_get(j) );

	return SDL_JoystickGetButton(sdl_state.map[j], b) != 0;
}

unsigned joystickb_sdl_stick_axe_digital_get(unsigned j, unsigned s, unsigned a, unsigned d)
{
	int r;
	log_debug(("joystickb:sdl: joystickb_sdl_stick_axe_digital_get()\n"));

	assert(j < joystickb_sdl_count_get());
	assert(s < joystickb_sdl_stick_count_get(j) );
	assert(a < joystickb_sdl_stick_axe_count_get(j, s) );

	r = SDL_JoystickGetAxis(sdl_state.map[j], a);
	if (d)
		return r < -16384;
	else
		return r > 16384;

	return 0;
}

int joystickb_sdl_stick_axe_analog_get(unsigned j, unsigned s, unsigned a)
{
	int r;
	log_debug(("joystickb:sdl: joystickb_sdl_stick_axe_analog_get()\n"));

	assert(j < joystickb_sdl_count_get());
	assert(s < joystickb_sdl_stick_count_get(j) );
	assert(a < joystickb_sdl_stick_axe_count_get(j, s) );

	r = SDL_JoystickGetAxis(sdl_state.map[j], a);
	r = r >> 8; /* adjust the upper limit from -128 to 128 */
	return r;
}

void joystickb_sdl_calib_start(void)
{
	log_debug(("joystickb:sdl: joystickb_sdl_calib_start()\n"));
}

const char* joystickb_sdl_calib_next(void)
{
	log_debug(("joystickb:sdl: joystickb_sdl_calib_next()\n"));
	return 0;
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
	joystickb_sdl_flags,
	joystickb_sdl_count_get,
	joystickb_sdl_stick_count_get,
	joystickb_sdl_stick_axe_count_get,
	joystickb_sdl_button_count_get,
	joystickb_sdl_stick_name_get,
	joystickb_sdl_stick_axe_name_get,
	joystickb_sdl_button_name_get,
	joystickb_sdl_button_get,
	joystickb_sdl_stick_axe_digital_get,
	joystickb_sdl_stick_axe_analog_get,
	joystickb_sdl_calib_start,
	joystickb_sdl_calib_next,
	joystickb_sdl_poll
};


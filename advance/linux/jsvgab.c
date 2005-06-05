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

#include "jsvgab.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"

#include <vgajoystick.h>

#define JOYSTICK_MAX 4 /**< Max number of joysticks */

struct joystickb_svgalib_context {
	unsigned counter; /**< Number of joysticks active. */
};

static struct joystickb_svgalib_context svgalib_state;

static adv_device DEVICE[] = {
{ "auto", -1, "SVGALIB joystick" },
{ 0, 0, 0 }
};

adv_error joystickb_svgalib_init(int joystickb_id)
{
	unsigned i;

	log_std(("josytickb:svgalib: joystickb_svgalib_init(id:%d)\n", joystickb_id));

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

	if (!os_internal_svgalib_get()) {
		error_set("Not supported without the svgalib library.\n");
		return -1;
	}

	for(i=0;i<4;++i) {
		if (joystick_init(i, 0)<=0) {
			break;
		}
	}

	if (i==0) {
		error_set("No joystick found.\n");
		return -1;
	}

	svgalib_state.counter = i;

	return 0;
}

void joystickb_svgalib_done(void)
{
	unsigned i;

	log_std(("josytickb:svgalib: joystickb_svgalib_done()\n"));

	for(i=0;i<svgalib_state.counter;++i)
		joystick_close(i);
}

unsigned joystickb_svgalib_count_get(void)
{
	log_debug(("joystickb:svgalib: joystickb_svgalib_count_get()\n"));

	return svgalib_state.counter;
}

unsigned joystickb_svgalib_stick_count_get(unsigned joystick)
{
	log_debug(("joystickb:svgalib: joystickb_svgalib_stick_count_get()\n"));

	return 1;
}

unsigned joystickb_svgalib_stick_axe_count_get(unsigned joystick, unsigned stick)
{
	log_debug(("joystickb:svgalib: joystickb_svgalib_stick_axe_count_get()\n"));

	return joystick_getnumaxes(joystick);
}

unsigned joystickb_svgalib_button_count_get(unsigned joystick)
{
	log_debug(("joystickb:svgalib: joystickb_svgalib_button_count_get()\n"));

	return joystick_getnumbuttons(joystick);
}

unsigned joystickb_svgalib_button_get(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:svgalib: joystickb_svgalib_button_get()\n"));

	return joystick_getbutton(joystick, button) != 0;
}

unsigned joystickb_svgalib_stick_axe_digital_get(unsigned joystick, unsigned stick, unsigned axe, unsigned d)
{
	int r;
	log_debug(("joystickb:svgalib: joystickb_svgalib_stick_axe_digital_get()\n"));

	r = joystick_getaxis(joystick, axe);

	r = joystickb_adjust_analog(r, -128, 127);

	if (d)
		return r < -JOYSTICK_DRIVER_BASE/8; /* -1/8 of the partial range */
	else
		return r > JOYSTICK_DRIVER_BASE/8; /* +1/8 of the partial range */
}

int joystickb_svgalib_stick_axe_analog_get(unsigned joystick, unsigned stick, unsigned axe)
{
	int r;
	log_debug(("joystickb:svgalib: joystickb_svgalib_stick_axe_analog_get()\n"));

	r = joystick_getaxis(joystick, axe);

	r = joystickb_adjust_analog(r, -128, 127);

	return r;
}

void joystickb_svgalib_poll(void)
{
	log_debug(("joystickb:svgalib: joystickb_svgalib_poll()\n"));

	joystick_update();
}

unsigned joystickb_svgalib_flags(void)
{
	return 0;
}

adv_error joystickb_svgalib_load(adv_conf* context)
{
	return 0;
}

void joystickb_svgalib_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

joystickb_driver joystickb_svgalib_driver = {
	"svgalib",
	DEVICE,
	joystickb_svgalib_load,
	joystickb_svgalib_reg,
	joystickb_svgalib_init,
	joystickb_svgalib_done,
	0,
	0,
	joystickb_svgalib_flags,
	joystickb_svgalib_count_get,
	joystickb_svgalib_stick_count_get,
	joystickb_svgalib_stick_axe_count_get,
	0,
	0,
	joystickb_svgalib_stick_axe_digital_get,
	joystickb_svgalib_stick_axe_analog_get,
	joystickb_svgalib_button_count_get,
	0,
	joystickb_svgalib_button_get,
	0,
	0,
	0,
	0,
	0,
	joystickb_svgalib_poll
};


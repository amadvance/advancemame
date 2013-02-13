/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2006 Andrea Mazzoleni
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

#include "jlgalleg.h"
#include "log.h"
#include "device.h"
#include "target.h"
#include "error.h"

#include "allegro2.h"

#define CALIBRATION_NONE 0
#define CALIBRATION_AUTO 1

#define FILTER_NONE 0
#define FILTER_AUTO 1
#define FILTER_TIME 2
#define FILTER_VALUE 3

struct lgallegro_option_struct {
	int calibration;
	int filter;
};

static struct lgallegro_option_struct lgallegro_option;

#define LG_MAX 8
#define LG_STICK_MAX 8
#define LG_AXE_MAX 8

struct joystick_axe_context {
	int v;
	int l, h;
	int a;
	unsigned d1;
	unsigned d2;
};

struct joystick_item_context {
	struct joystick_axe_context axe_map[LG_STICK_MAX][LG_AXE_MAX];
};

struct joystickb_lgallegro_context {
	int id; /**< Joystick identifier. */
	target_clock_t last; /**< Time of the last poll. */
	struct joystick_item_context map[LG_MAX];
};

static struct joystickb_lgallegro_context lgallegro_state;

static adv_device DEVICE[] = {
	{ "auto", JOY_TYPE_AUTODETECT, "Allegro lightgun" },

	{ "standard", JOY_TYPE_STANDARD, "Lightgun standard" },

/* From Allegro 4.0.1
 *      Joystick driver for PSX controllers.
 *
 *      By Richard Davies.
 *
 *      Based on sample code by Earle F. Philhower, III. from DirectPad
 *      Pro 4.9, for use with the DirectPad Pro parallel port interface.
 *
 *      See <http://www.ziplabel.com/dpadpro> for interface details.
 *
 *      Original parallel port interface and code by Juan Berrocal.
 *
 *      Digital, Analog, Dual Force (control), NegCon and Mouse
 *      information by T. Fujita. Dual Shock (vibrate) information by
 *      Dark Fader. Multi tap, G-con45 and Jogcon information by me
 *      (the weird stuff ;)
 *
 *      This driver recognises Digital, Analog, Dual Shock, Mouse, negCon,
 *      G-con45, Jogcon, Konami Lightgun and Multi tap devices.
 *
 *      Digital, Dual Shock, neGcon, G-con45, Jogcon and Multi tap devices
 *      have all been tested. The Analog (green mode or joystick), Mouse
 *      and Konami Lightgun devices have not. The position data is likely
 *      to be broken for the Konami Lightgun, and may also be broken for
 *      the Mouse.
 *
 *      The G-con45 needs to be connected to (and pointed at) a TV type
 *      monitor connected to your computer. The composite video out on my
 *      video card works fine for this.
 *
 *      The Sony Dual Shock or Namco Jogcon will reset themselves (to
 *      digital mode) after not being polled for 5 seconds. This is normal,
 *      the same thing happens on a Playstation, it's meant to stop any
 *      vibration in case the host machine crashes. However, if this
 *      happens to a Jogcon controller the mode button is disabled. To
 *      reenable the mode button on the Jogcon hold down the Start and
 *      Select buttons at the same time. Other mode switching controllers
 *      may have similar quirks.
 *
 *      Some people may have problems with the psx poll delay set at 3
 *      causing them twitchy controls (this depends on the controllers
 *      more than anything else).
 *
 *      It may be worthwhile to add calibration to some of the analog
 *      controls, although most controller types aren't really meant to
 *      be calibrated:
 *
 *      - My Dual Shock controller centres really badly; most of them do.
 *      - My neGcon centres really well (+/- 1).
 *      - The G-con45 needs calibration for it's centre aim and velocity.
 *      - The Jogcon calibrates itself when initialised.
 *
 *      To Do List:
 *
 *      - Verify Analog Joystick (Green mode) works.
 *      - Verify Mouse position works.
 *      - Verify MegaTap interface.
 *      - Add calibration for the G-con45, Dual Shock and neGcon controllers.
 *      - Implement Konami Lightgun aim.
 *      - Implement Jogcon force feedback.
 *      - Implement unsupported controllers (Ascii Sphere? Beat Mania Decks?)
 *
 *      If you can help with any of these then please let me know.
 *
 *
 *       -----------------------
 *      | o o o | o o o | o o o | Controller Port (looking into plug)
 *       \_____________________/
 *        1 2 3   4 5 6   7 8 9
 *
 *      Controller          Parallel
 *
 *      1 - data            10 (conport 1, 3, 4, 5, 6), 13 (conport 2)
 *      2 - command         2
 *      3 - 9V(shock)       +9V power supply terminal for Dual Shock
 *      4 - gnd             18,19 also -9V and/or -5V power supply terminal
 *      5 - V+              5, 6, 7, 8, 9 through diodes, or +5V power supply terminal
 *      6 - att             3 (conport 1, 2), 5 (conport 3), 6 (conport 4), 7 (conport 5), 8 (conport 6)
 *      7 - clock           4
 *      9 - ack             12 (conport 1, 3, 4, 5, 6), 14 (conport 2) **
 *
 *      ** There is an error in the DirectPad Pro documentation, which states that
 *         the second control port should have pin 9 connected to pin 15 on the
 *         parallel port. This should actually be pin 14 on the parallel port. To
 *         make things more confusing, this error is unlikely to prevent the
 *         interface from working properly. It's also possible that a change to the
 *         scanning code has been made in version 5.0 to prevent people from having
 *         to rewire their interfaces.
 *
 */
	{ "psxlpt1", JOY_TYPE_PSXPAD_LPT1, "Lightgun PSXpad LPT1" },
	{ "psxlpt2", JOY_TYPE_PSXPAD_LPT2, "Lightgun PSXpad LPT2" },
	{ "psxlpt3", JOY_TYPE_PSXPAD_LPT3, "Lightgun PSXpad LPT3" },

	{ 0, 0, 0 }
};

adv_error joystickb_lgallegro_init(int id)
{
	log_std(("joystickb:lgallegro: joystickb_lgallegro_init(id:%d)\n", id));

	if (id == JOY_TYPE_AUTODETECT) {
		error_set("No auto detection");
		return -1;
	}

	lgallegro_state.id = id;

	lgallegro_state.last = target_clock();

	log_std(("joystickb:lgallegro: joystick initialization\n"));
	if (install_joystick(lgallegro_state.id) != 0) {
		log_std(("joystickb:lgallegro: joystick initialization failed\n"));
		return -1;
	}

	return 0;
}

void joystickb_lgallegro_done(void)
{
	log_std(("joystickb:lgallegro: joystickb_lgallegro_done()\n"));

	remove_joystick();
}

unsigned joystickb_lgallegro_count_get(void)
{
	log_debug(("joystickb:lgallegro: joystickb_lgallegro_count_get()\n"));

	return num_joysticks;
}

unsigned joystickb_lgallegro_stick_count_get(unsigned j)
{
	log_debug(("joystickb:lgallegro: joystickb_lgallegro_stick_count_get()\n"));

	return joy[j].num_sticks;
}

unsigned joystickb_lgallegro_stick_axe_count_get(unsigned j, unsigned s)
{
	log_debug(("joystickb:lgallegro: joystickb_lgallegro_stick_axe_count_get()\n"));

	return joy[j].stick[s].num_axis;
}

unsigned joystickb_lgallegro_button_count_get(unsigned j)
{
	log_debug(("joystickb:lgallegro: joystickb_lgallegro_button_count_get()\n"));

	return joy[j].num_buttons;
}

unsigned joystickb_lgallegro_button_get(unsigned j, unsigned b)
{
	log_debug(("joystickb:lgallegro: joystickb_lgallegro_button_get()\n"));

	return joy[j].button[b].b;
}

unsigned joystickb_lgallegro_stick_axe_digital_get(unsigned j, unsigned s, unsigned a, unsigned d)
{
	log_debug(("joystickb:lgallegro: joystickb_lgallegro_stick_axe_digital_get()\n"));

	if (d)
		return lgallegro_state.map[j].axe_map[s][a].d1;
	else
		return lgallegro_state.map[j].axe_map[s][a].d2;

	return 0;
}

int joystickb_lgallegro_stick_axe_analog_get(unsigned j, unsigned s, unsigned a)
{
	int r;
	log_debug(("joystickb:lgallegro: joystickb_lgallegro_stick_axe_analog_get()\n"));

	r = lgallegro_state.map[j].axe_map[s][a].a;

	return r;
}

void joystickb_lgallegro_poll(void)
{
	unsigned j, s, a;

	log_debug(("joystickb:lgallegro: joystickb_lgallegro_poll()\n"));

	target_clock_t now = target_clock();

	if (lgallegro_option.filter == FILTER_AUTO
		|| lgallegro_option.filter == FILTER_TIME
	) {
		/* don't poll too frequently */
		if (now - lgallegro_state.last > TARGET_CLOCKS_PER_SEC / 30) {
			log_debug(("joystickb:lgallegro: effective poll\n"));
			lgallegro_state.last = now;
			poll_joystick();
		} else {
			log_debug(("joystickb:lgallegro: skipped poll\n"));
		}
	} else {
		lgallegro_state.last = now;
		poll_joystick();
	}

	for(j=0;j<joystickb_lgallegro_count_get();++j) {
		for(s=0;s<joystickb_lgallegro_stick_count_get(j);++s) {
			for(a=0;a<joystickb_lgallegro_stick_axe_count_get(j,s);++a) {
				struct joystick_axe_context* c = &lgallegro_state.map[j].axe_map[s][a];

				int r = joy[j].stick[s].axis[a].pos;

				if (lgallegro_option.filter == FILTER_VALUE) {
					int limit = 127;
					if (r <= -limit || r >= limit) {
						/* skip */
						continue;
					}
				}

				/* store the new values */
				c->v = r;
				c->d1 = joy[j].stick[s].axis[a].d1;
				c->d2 = joy[j].stick[s].axis[a].d2;

				/* update autocalibration */
				if (c->l == 0 && c->h == 0) {
					c->l = c->v;
					c->h = c->v;
				} else {
					if (c->v < c->l)
						c->l = c->v;
					if (c->v > c->h)
						c->h = c->v;
				}

				if (lgallegro_option.calibration == CALIBRATION_AUTO) {
					c->a = joystickb_adjust_analog(c->v, c->l, c->h);
				} else if (lgallegro_option.calibration == CALIBRATION_NONE) {
					c->a = joystickb_adjust_analog(c->v, -128, 128);
				} else {
					c->a = 0;
				}

				log_debug(("joystickb:lgallegro: id:%d,%d,%d,v:%d[%d:%d>%d]\n", j, s, a, c->v, c->l, c->h, c->a));
			}
		}
	}
}

unsigned joystickb_lgallegro_flags(void)
{
	return 0;
}

adv_error joystickb_lgallegro_load(adv_conf* context)
{
	lgallegro_option.calibration = conf_int_get_default(context, "device_lgallegro_calibration");
	lgallegro_option.filter = conf_int_get_default(context, "device_lgallegro_filter");

	return 0;
}

static adv_conf_enum_int OPTION_CALIB[] = {
{ "none", CALIBRATION_NONE },
{ "auto", CALIBRATION_AUTO }
};

static adv_conf_enum_int OPTION_FILTER[] = {
{ "none", FILTER_NONE },
{ "auto", FILTER_AUTO },
{ "time", FILTER_TIME },
{ "value", FILTER_VALUE }
};

void joystickb_lgallegro_reg(adv_conf* context)
{
	conf_int_register_enum_default(context, "device_lgallegro_calibration", conf_enum(OPTION_CALIB), CALIBRATION_AUTO);
	conf_int_register_enum_default(context, "device_lgallegro_filter", conf_enum(OPTION_FILTER), FILTER_AUTO);
}

/***************************************************************************/
/* Driver */

joystickb_driver joystickb_lgallegro_driver = {
	"lgallegro",
	DEVICE,
	joystickb_lgallegro_load,
	joystickb_lgallegro_reg,
	joystickb_lgallegro_init,
	joystickb_lgallegro_done,
	0,
	0,
	joystickb_lgallegro_flags,
	joystickb_lgallegro_count_get,
	joystickb_lgallegro_stick_count_get,
	joystickb_lgallegro_stick_axe_count_get,
	0,
	0,
	joystickb_lgallegro_stick_axe_digital_get,
	joystickb_lgallegro_stick_axe_analog_get,
	joystickb_lgallegro_button_count_get,
	0,
	joystickb_lgallegro_button_get,
	0,
	0,
	0,
	0,
	0,
	joystickb_lgallegro_poll
};


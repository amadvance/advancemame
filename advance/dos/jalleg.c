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

#include "jalleg.h"
#include "log.h"
#include "device.h"
#include "target.h"

#include "allegro2.h"

struct joystickb_allegro_context {
	int id; /**< Joystick identifier. */
	int allegro_id; /**< Allegro joystick identifier. */
	target_clock_t last; /**< Time of the last poll. */
	int calibration_target;
	int calibration_first;
};

static struct joystickb_allegro_context allegro_state;

/* extra types */
#define JOY_TYPE_LIGHTGUN_LPT1 0x80000000
#define JOY_TYPE_LIGHTGUN_LPT2 0x80000001
#define JOY_TYPE_LIGHTGUN_LPT3 0x80000002

static adv_device DEVICE[] = {
	{ "auto", JOY_TYPE_AUTODETECT, "Allegro joystick" },
	{ "standard", JOY_TYPE_STANDARD, "Standard joystick" },
	{ "dual", JOY_TYPE_2PADS , "Dual joysticks" },
	{ "4button", JOY_TYPE_4BUTTON, "4-button joystick" },
	{ "6button", JOY_TYPE_6BUTTON, "6-button joystick" },
	{ "8button", JOY_TYPE_8BUTTON, "8-button joystick" },
	{ "fspro", JOY_TYPE_FSPRO, "CH Flightstick Pro" },
	{ "wingex", JOY_TYPE_WINGEX, "Logitech Wingman Extreme" },
	{ "sidewinder", JOY_TYPE_SIDEWINDER, "Sidewinder" },
	{ "sidewinderag", JOY_TYPE_SIDEWINDER_AG, "Sidewinder Aggressive" },
	{ "gamepadpro", JOY_TYPE_GAMEPAD_PRO, "GamePad Pro" },
	{ "grip", JOY_TYPE_GRIP, "GrIP" },
	{ "grip4", JOY_TYPE_GRIP4, "GrIP 4-way" },

/* From Allegro 4.0.1
 *      Joystick driver for the SNES controller.
 *
 *      By Kerry High, based on sample code by Earle F. Philhower, III.
 *
 *      Mucked about with until it works better by Paul Hampson.
 *
 *      Lower CPU usage by Paul Hampson, based on the Linux implementation
 *      by Vojtech Pavlik.
 */
	{ "sneslpt1", JOY_TYPE_SNESPAD_LPT1, "SNESpad LPT1" },
	{ "sneslpt2", JOY_TYPE_SNESPAD_LPT2, "SNESpad LPT2" },
	{ "sneslpt3", JOY_TYPE_SNESPAD_LPT3, "SNESpad LPT3" },

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
	{ "psxlpt1", JOY_TYPE_PSXPAD_LPT1, "PSXpad LPT1" },
	{ "psxlpt2", JOY_TYPE_PSXPAD_LPT2, "PSXpad LPT2" },
	{ "psxlpt3", JOY_TYPE_PSXPAD_LPT3, "PSXpad LPT3" },

	{ "lightgunlpt1", JOY_TYPE_LIGHTGUN_LPT1, "Lightgun LPT1" },
	{ "lightgunlpt2", JOY_TYPE_LIGHTGUN_LPT2, "Lightgun LPT2" },
	{ "lightgunlpt3", JOY_TYPE_LIGHTGUN_LPT3, "Lightgun LPT3" },


/* From Allegro 4.0.1
 *      Joystick driver for N64 controllers.
 *
 *      By Richard Davies, based on sample code by Earle F. Philhower, III.
 *
 *      This driver supports upto four N64 controllers. The analog stick
 *      calibrates itself when the controller is powered up (in hardware).
 *      There is some autodetection code included, but it's unused as it's
 *      unreliable. Care to take a look?
 *
 *      This driver is for the N64 pad -> parallel port interface developed
 *      by Stephan Hans and Simon Nield, supported by DirectPad Pro 4.9.
 *
 *      See http://www.st-hans.de/N64.htm for interface information, and
 *      See http://www.ziplabel.com for DirectPad Pro information.
 */
	{ "n64lpt1", JOY_TYPE_N64PAD_LPT1, "N64pad LPT1" },
	{ "n64lpt2", JOY_TYPE_N64PAD_LPT2, "N64pad LPT2" },
	{ "n64lpt3", JOY_TYPE_N64PAD_LPT3, "N64pad LPT3" },

/* From Allegro 4.0.1
 *      Drivers for multisystem joysticks (Atari, Commodore 64 etc.)
 *      with 9-pin connectors.
 *
 *      By Fabrizio Gennari.
 *
 *      JOY_TYPE_DB9_LPT[123]
 *
 *      Supports 2 two-button joysticks. Port 1 is compatible with Linux
 *      joy-db9 driver (multisystem 2-button), and port 2 is compatible
 *      with Atari interface for DirectPad Pro.
 *
 *      Based on joy-db9 driver for Linux by Vojtech Pavlik
 *      and on Atari interface for DirectPad Pro by Earle F. Philhower, III
 *
 *      Interface pinout
 *
 *
 *      Parallel port                           Joystick port 1
 *       1----------------------------------------------------1
 *      14----------------------------------------------------2
 *      16----------------------------------------------------3
 *      17----------------------------------------------------4
 *      11----------------------------------------------------6
 *      12----------------------------------------------------7 (button 2)
 *      18----------------------------------------------------8
 *
 *                                              Joystick port 2
 *       2----------------------------------------------------1
 *       3----------------------------------------------------2
 *       4----------------------------------------------------3
 *       5----------------------------------------------------4
 *       6----------------------------------------------------6
 *       7----------------------------------------------------7 (button 2)
 *      19----------------------------------------------------8
 *
 *      Advantages
 *
 *      * Simpler to build (no diodes required)
 *      * Autofire will work (if joystick supports it)
 *
 *      Drawbacks
 *
 *      * The parallel port must be in SPP (PS/2) mode in order for this
 *        driver to work. In Normal mode, port 2 won't work because data
 *        pins are not inputs. In EPP/ECP PS/2 mode, directions for
 *        port 1 won't work (buttons will) beacuse control pins are not
 *        inputs.
 *
 *       * The parallel port should not require pull-up resistors
 *         (however, newer ones don't)
 *
 *      JOY_TYPE_TURBOGRAFX_LPT[123]
 *
 *      Supports up to 7 joysticks, each one with up to 5 buttons.
 *
 *      Original interface and driver by Steffen Schwenke
 *      See <http://www.burg-halle.de/~schwenke/parport.html> for details
 *      on how to build the interface
 *
 *      Advantages
 *
 *      * Exploits the parallel port to its limits
 *
 *      Drawbacks
 *
 *      * Autofire will not work
 */
	{ "db9lpt1", JOY_TYPE_DB9_LPT1, "DB9 LPT1" },
	{ "db9lpt2", JOY_TYPE_DB9_LPT2, "DB9 LPT2" },
	{ "db9lpt3", JOY_TYPE_DB9_LPT3, "DB9 LPT3" },
	{ "tgxlpt1", JOY_TYPE_TURBOGRAFX_LPT1, "TGX-LPT1" },
	{ "tgxlpt2", JOY_TYPE_TURBOGRAFX_LPT2, "TGX LPT2" },
	{ "tgxlpt3", JOY_TYPE_TURBOGRAFX_LPT3, "TGX LPT3" },

	{ "segaisa", JOY_TYPE_IFSEGA_ISA, "IF-SEGA/ISA" },
	{ "segapci", JOY_TYPE_IFSEGA_PCI, "IF-SEGA2/PCI" },
	{ "segapcifast", JOY_TYPE_IFSEGA_PCI_FAST, "IF-SEGA2/PCI (normal)" },
	{ "wingwarrior", JOY_TYPE_WINGWARRIOR, "Wingman Warrior" },
	{ 0, 0, 0 }
};

adv_error joystickb_allegro_init(int id)
{
	log_std(("joystickb:allegro: joystickb_allegro_init(id:%d)\n", id));

	allegro_state.id = id;

	switch (id) {
	case JOY_TYPE_LIGHTGUN_LPT1 :
		allegro_state.allegro_id = JOY_TYPE_PSXPAD_LPT1;
		break;
	case JOY_TYPE_LIGHTGUN_LPT2 :
		allegro_state.allegro_id = JOY_TYPE_PSXPAD_LPT2;
		break;
	case JOY_TYPE_LIGHTGUN_LPT3 :
		allegro_state.allegro_id = JOY_TYPE_PSXPAD_LPT3;
		break;
	default:
		allegro_state.allegro_id = id;
		break;
	}

	allegro_state.last = target_clock();

	log_std(("joystickb:allegro: joystick load calibration data\n"));
	if (load_joystick_data(0) != 0) {
		log_std(("joystickb:allegro: joystick error loading calibration data, try reinitializing\n"));
		if (install_joystick(allegro_state.allegro_id) != 0) {
			log_std(("joystickb:allegro: joystick initialization failed\n"));
			return -1;
		}
	}

	return 0;
}

void joystickb_allegro_done(void)
{
	log_std(("joystickb:allegro: joystickb_allegro_done()\n"));

	remove_joystick();
}

unsigned joystickb_allegro_count_get(void)
{
	log_debug(("joystickb:allegro: joystickb_allegro_count_get()\n"));

	return num_joysticks;
}

unsigned joystickb_allegro_stick_count_get(unsigned j)
{
	log_debug(("joystickb:allegro: joystickb_allegro_stick_count_get()\n"));

	return joy[j].num_sticks;
}

unsigned joystickb_allegro_stick_axe_count_get(unsigned j, unsigned s)
{
	log_debug(("joystickb:allegro: joystickb_allegro_stick_axe_count_get()\n"));

	return joy[j].stick[s].num_axis;
}

unsigned joystickb_allegro_button_count_get(unsigned j)
{
	log_debug(("joystickb:allegro: joystickb_allegro_button_count_get()\n"));

	return joy[j].num_buttons;
}

unsigned joystickb_allegro_button_get(unsigned j, unsigned b)
{
	log_debug(("joystickb:allegro: joystickb_allegro_button_get()\n"));

	return joy[j].button[b].b;
}

unsigned joystickb_allegro_stick_axe_digital_get(unsigned j, unsigned s, unsigned a, unsigned d)
{
	const JOYSTICK_AXIS_INFO* jai;
	log_debug(("joystickb:allegro: joystickb_allegro_stick_axe_digital_get()\n"));

	jai = &joy[j].stick[s].axis[a];
	if (d)
		return jai->d1;
	else
		return jai->d2;

	return 0;
}

int joystickb_allegro_stick_axe_analog_get(unsigned j, unsigned s, unsigned a)
{
	int r;
	log_debug(("joystickb:allegro: joystickb_allegro_stick_axe_analog_get()\n"));

	r = joy[j].stick[s].axis[a].pos;

	r = joystickb_adjust_analog(r, -128, 128);

	return r;
}

void joystickb_allegro_calib_start(void)
{
	log_debug(("joystickb:allegro: joystickb_allegro_calib_start()\n"));

	remove_joystick();
	install_joystick(allegro_state.allegro_id);

	allegro_state.calibration_target = 0;
	allegro_state.calibration_first = 1;
}

const char* joystickb_allegro_calib_next(void)
{
	log_debug(("joystickb:allegro: joystickb_allegro_calib_next()\n"));

	while (allegro_state.calibration_target < num_joysticks) {

		if (joy[allegro_state.calibration_target].flags & JOYFLAG_CALIBRATE) {
			if (!allegro_state.calibration_first) {
				if (calibrate_joystick(allegro_state.calibration_target) != 0) {
					log_std(("joystickb:allegro: joystick error in calibration %d\n", allegro_state.calibration_target));
					/* stop on error */
					return 0;
				}
			}

			allegro_state.calibration_first = 0;

			if (joy[allegro_state.calibration_target].flags & JOYFLAG_CALIBRATE) {
				const char* msg;

				msg = calibrate_joystick_name(allegro_state.calibration_target);
				log_std(("joystickb:allegro: joystick calibration msg %s\n", msg));
				return msg;
			}
		} else {
			/* next joystick */
			allegro_state.calibration_target++;
			allegro_state.calibration_first = 1;
		}
	}

	log_std(("joystickb:allegro: joystick saving calibration data\n"));

	save_joystick_data(0);

	return 0;
}

void joystickb_allegro_poll(void)
{
	log_debug(("joystickb:allegro: joystickb_allegro_poll()\n"));

	if (allegro_state.id == JOY_TYPE_LIGHTGUN_LPT1
		|| allegro_state.id == JOY_TYPE_LIGHTGUN_LPT2
		|| allegro_state.id == JOY_TYPE_LIGHTGUN_LPT3) {
			target_clock_t now = target_clock();
			/* don't poll too frequently */
			if (now - allegro_state.last > TARGET_CLOCKS_PER_SEC / 30) {
				log_debug(("joystickb:allegro: effective poll\n"));
				allegro_state.last = now;
				poll_joystick();
			} else {
				log_debug(("joystickb:allegro: skipped poll\n"));
			}
	} else {
		poll_joystick();
	}
}

unsigned joystickb_allegro_flags(void)
{
	return 0;
}

adv_error joystickb_allegro_load(adv_conf* context)
{
	return 0;
}

void joystickb_allegro_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

joystickb_driver joystickb_allegro_driver = {
	"allegro",
	DEVICE,
	joystickb_allegro_load,
	joystickb_allegro_reg,
	joystickb_allegro_init,
	joystickb_allegro_done,
	0,
	0,
	joystickb_allegro_flags,
	joystickb_allegro_count_get,
	joystickb_allegro_stick_count_get,
	joystickb_allegro_stick_axe_count_get,
	0,
	0,
	joystickb_allegro_stick_axe_digital_get,
	joystickb_allegro_stick_axe_analog_get,
	joystickb_allegro_button_count_get,
	0,
	joystickb_allegro_button_get,
	0,
	0,
	0,
	joystickb_allegro_calib_start,
	joystickb_allegro_calib_next,
	joystickb_allegro_poll
};


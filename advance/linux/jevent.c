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

#include "jevent.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "event.h"
#include "portable.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <linux/input.h>

#define EVENT_JOYSTICK_MAX 8
#define EVENT_JOYSTICK_DEVICE_MAX 32
#define EVENT_JOYSTICK_BUTTON_MAX 16
#define EVENT_JOYSTICK_STICK_MAX 16
#define EVENT_JOYSTICK_NAME_MAX 128
#define EVENT_JOYSTICK_AXE_MAX 8

struct joystick_button_context {
	unsigned code;
	adv_bool state;
	char name[EVENT_JOYSTICK_NAME_MAX];
};

struct joystick_axe_context {
	int code;
	int value;
	int value_adj; /**< Value adjusted in range -128, 128. */
	int min;
	int max;
	int fuzz;
	int flat;
	int digit_low;
	int digit_high;
	char name[EVENT_JOYSTICK_NAME_MAX];
};

struct joystick_stick_context {
	unsigned axe_mac;
	struct joystick_axe_context axe_map[EVENT_JOYSTICK_AXE_MAX];
	char name[EVENT_JOYSTICK_NAME_MAX];
};

struct joystick_item_context {
	int f;
	unsigned char evtype_bitmask[EV_MAX/8 + 1];
	unsigned stick_mac;
	struct joystick_stick_context stick_map[EVENT_JOYSTICK_STICK_MAX];
	unsigned button_mac;
	struct joystick_button_context button_map[EVENT_JOYSTICK_BUTTON_MAX];
};

struct joystickb_event_context {
	unsigned mac;
	struct joystick_item_context map[EVENT_JOYSTICK_MAX];
};

static struct joystickb_event_context event_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Linux input-event joystick" },
{ 0, 0, 0 }
};

#define ABS_UNASSIGNED -1

static adv_error joystickb_setup(struct joystick_item_context* item, int f)
{
	unsigned char key_bitmask[KEY_MAX/8 + 1];
	unsigned char abs_bitmask[ABS_MAX/8 + 1];
	unsigned i;
	struct button_entry {
		int code;
		const char* name;
	} button_map[] = {
		{ BTN_TRIGGER, "trigger" },
		{ BTN_TOP, "top" },
		{ BTN_THUMB, "thumb" },
		{ BTN_TOP2, "top2" },
		{ BTN_THUMB2, "thumb2" },
		{ BTN_PINKIE, "pinkie" },
		{ BTN_BASE, "base" },
		{ BTN_BASE2, "base2" },
		{ BTN_BASE3, "base3" },
		{ BTN_BASE4, "base4" },
		{ BTN_BASE5, "base5" },
		{ BTN_BASE6, "base6" },
		{ BTN_DEAD, "dead" }
	};

	struct stick_entry {
		struct axe_entry {
			int code;
			const char* name;
		} axe_map[7];
		const char* name;
	} stick_map[] = {
		{ { { ABS_X, "x" }, { ABS_RX, "rx" }, { ABS_Y, "y" }, { ABS_RY, "ry" }, { ABS_Z, "z" } , { ABS_RZ, "rz" }, { ABS_UNASSIGNED, 0 } }, "stick" },
		{ { { ABS_GAS, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "gas" }, /* acceleratore */
		{ { { ABS_BRAKE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "brake" }, /* freno */
		{ { { ABS_WHEEL, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "wheel" }, /* volante */
		{ { { ABS_HAT0X, "x" }, { ABS_HAT0Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat" },
		{ { { ABS_HAT1X, "x" }, { ABS_HAT1Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat2" },
		{ { { ABS_HAT2X, "x" }, { ABS_HAT2Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat3" },
		{ { { ABS_HAT3X, "x" }, { ABS_HAT3Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat4" },
		{ { { ABS_THROTTLE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "throttle" },
		{ { { ABS_RUDDER, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "rudder" }, /* timone */
		{ { { ABS_PRESSURE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "pressure" },
		{ { { ABS_DISTANCE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "distance" },
		{ { { ABS_TILT_X, "x" }, { ABS_TILT_Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "tilt" },
		{ { { ABS_MISC, "misc" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "misc" }
	};

	item->f = f;

	memset(key_bitmask, 0, sizeof(key_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_KEY,%d))\n", (int)KEY_MAX));
		return -1;
	}

	item->button_mac = 0;
	for(i=0;i<sizeof(button_map)/sizeof(button_map[0]);++i) {
		if (event_test_bit(button_map[i].code, key_bitmask)) {
			if (item->button_mac < EVENT_JOYSTICK_BUTTON_MAX) {
				item->button_map[item->button_mac].code = button_map[i].code;
				item->button_map[item->button_mac].state = 0;
				sncpy(item->button_map[item->button_mac].name, sizeof(item->button_map[item->button_mac].name), button_map[i].name);
				++item->button_mac;
			}
		}
	}

	memset(abs_bitmask, 0, sizeof(abs_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_ABS, sizeof(abs_bitmask)), abs_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_ABS,%d))\n", (int)ABS_MAX));
		return -1;
	}

	item->stick_mac = 0;
	for(i=0;i<sizeof(stick_map)/sizeof(stick_map[0]);++i) {
		if (item->stick_mac < EVENT_JOYSTICK_STICK_MAX) {
			unsigned j;
			struct joystick_stick_context* stick = item->stick_map + item->stick_mac;

			stick->axe_mac = 0;
			sncpy(stick->name, sizeof(stick->name), stick_map[i].name);

			for(j=0;stick_map[i].axe_map[j].code != ABS_UNASSIGNED;++j) {
				int code = stick_map[i].axe_map[j].code;
				if (event_test_bit(code, abs_bitmask)) {
					if (stick->axe_mac < EVENT_JOYSTICK_AXE_MAX) {
						struct joystick_axe_context* axe = stick->axe_map + stick->axe_mac;
						int features[5];

						axe->code = code;

						sncpy(axe->name, sizeof(axe->name), stick_map[i].axe_map[j].name);

						memset(features, 0, sizeof(features));
						if (ioctl(f, EVIOCGABS(code), features) < 0) {
							axe->min = 0;
							axe->max = 0;
							axe->fuzz = 0;
							axe->flat = 0;
							axe->digit_low = -32;
							axe->digit_high = 32;
							axe->value = 0;
							axe->value_adj = 0;
						} else {
							int middle = (features[1] + features[2]) / 2;
							int size = features[2] - features[1];
							axe->min = features[1];
							axe->max = features[2];
							axe->fuzz = features[3];
							axe->flat = features[4];
							if (features[4]>=2) {
								axe->digit_low = middle - features[4] - features[3];
								axe->digit_high = middle + features[4] + features[3];
							} else {
								/* if size is very small digit_low and digit_high are equal at middle */
								axe->digit_low = middle - size / 8;
								axe->digit_high = middle + size / 8;
							}
							axe->value = middle;
							axe->value_adj = 0;
						}

						++stick->axe_mac;
					}
				}
			}

			/* save the stick only if it hash some axes */
			if (stick->axe_mac)
				++item->stick_mac;
		}
	}

	return 0;
}

adv_error joystickb_event_init(int joystickb_id)
{
	unsigned i;

	log_std(("josytickb:event: joystickb_event_init(id:%d)\n", joystickb_id));

	log_std(("joystickb:event: opening joystick from 0 to %d\n", EVENT_JOYSTICK_DEVICE_MAX));

	event_state.mac = 0;
	for(i=0;i<EVENT_JOYSTICK_DEVICE_MAX;++i) {
		int f;
		char file[128];

		if (event_state.mac >= EVENT_JOYSTICK_MAX)
			continue;

		snprintf(file, sizeof(file), "/dev/input/event%d", i);

		f = event_open(file, event_state.map[event_state.mac].evtype_bitmask);
		if (f == -1)
			continue;

		event_log(f, event_state.map[event_state.mac].evtype_bitmask);

		if (!event_is_joystick(event_state.map[event_state.mac].evtype_bitmask)) {
			log_std(("joystickb:event: not a joystick on device %s\n", file));
			event_close(f);
			continue;
		}

		if (joystickb_setup(&event_state.map[event_state.mac], f) != 0) {
			event_close(f);
			continue;
		}

		++event_state.mac;
	}

	if (!event_state.mac) {
		error_set("No joystick found.\n");
		return -1;
	}

	return 0;
}

void joystickb_event_done(void)
{
	unsigned i;

	log_std(("josytickb:event: joystickb_event_done()\n"));

	for(i=0;i<event_state.mac;++i)
		event_close(event_state.map[i].f);
	event_state.mac = 0;
}

unsigned joystickb_event_count_get(void)
{
	log_debug(("joystickb:event: joystickb_event_count_get()\n"));

	return event_state.mac;
}

unsigned joystickb_event_stick_count_get(unsigned joystick)
{
	log_debug(("joystickb:event: joystickb_event_stick_count_get()\n"));

	assert(joystick < joystickb_event_count_get());

	return event_state.map[joystick].stick_mac;
}

unsigned joystickb_event_stick_axe_count_get(unsigned joystick, unsigned stick)
{
	log_debug(("joystickb:event: joystickb_event_stick_axe_count_get()\n"));

	assert(joystick < joystickb_event_count_get());
	assert(stick < joystickb_event_stick_count_get(joystick) );

	return event_state.map[joystick].stick_map[stick].axe_mac;
}

unsigned joystickb_event_button_count_get(unsigned joystick)
{
	log_debug(("joystickb:event: joystickb_event_button_count_get()\n"));

	assert(joystick < joystickb_event_count_get());

	return event_state.map[joystick].button_mac;
}

const char* joystickb_event_stick_name_get(unsigned joystick, unsigned stick)
{
	log_debug(("joystickb:event: joystickb_event_stick_name_get()\n"));

	assert(joystick < joystickb_event_count_get());
	assert(stick < joystickb_event_stick_count_get(joystick) );

	return event_state.map[joystick].stick_map[stick].name;
}

const char* joystickb_event_stick_axe_name_get(unsigned joystick, unsigned stick, unsigned axe)
{
	log_debug(("joystickb:event: joystickb_event_stick_axe_name_get()\n"));

	assert(joystick < joystickb_event_count_get());
	assert(stick < joystickb_event_stick_count_get(joystick) );
	assert(axe < joystickb_event_stick_axe_count_get(joystick, stick) );

	return event_state.map[joystick].stick_map[stick].axe_map[axe].name;
}

const char* joystickb_event_button_name_get(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:event: joystickb_event_button_name_get()\n"));

	assert(joystick < joystickb_event_count_get());
	assert(button < joystickb_event_button_count_get(joystick) );

	return event_state.map[joystick].button_map[button].name;
}

unsigned joystickb_event_button_get(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:event: joystickb_event_button_get()\n"));

	assert(joystick < joystickb_event_count_get());
	assert(button < joystickb_event_button_count_get(joystick) );

	return event_state.map[joystick].button_map[button].state != 0;
}

unsigned joystickb_event_stick_axe_digital_get(unsigned joystick, unsigned stick, unsigned axe, unsigned d)
{
	int r;
	log_debug(("joystickb:event: joystickb_event_stick_axe_digital_get()\n"));

	assert(joystick < joystickb_event_count_get());
	assert(stick < joystickb_event_stick_count_get(joystick) );
	assert(axe < joystickb_event_stick_axe_count_get(joystick, stick) );

	r = event_state.map[joystick].stick_map[stick].axe_map[axe].value;
	if (d)
		return r < event_state.map[joystick].stick_map[stick].axe_map[axe].digit_low;
	else
		return r > event_state.map[joystick].stick_map[stick].axe_map[axe].digit_high;

	return 0;
}

int joystickb_event_stick_axe_analog_get(unsigned joystick, unsigned stick, unsigned axe)
{
	int r;
	log_debug(("joystickb:event: joystickb_event_stick_axe_analog_get()\n"));

	assert(joystick < joystickb_event_count_get());
	assert(stick < joystickb_event_stick_count_get(joystick) );
	assert(axe < joystickb_event_stick_axe_count_get(joystick, stick) );

	r = event_state.map[joystick].stick_map[stick].axe_map[axe].value_adj;

	return r;
}

void joystickb_event_calib_start(void)
{
	log_debug(("joystickb:event: joystickb_event_calib_start()\n"));
}

const char* joystickb_event_calib_next(void)
{
	log_debug(("joystickb:event: joystickb_event_calib_next()\n"));

	return 0;
}

static void joystickb_event_axe_set(struct joystick_axe_context* axe, int value)
{
	if (axe->flat) {
		/* center position */
		int middle = (axe->max + axe->min) / 2;
		/* dead range */
		int dead_min = middle - axe->flat / 2;
		int dead_max = middle + axe->flat / 2;
		if (dead_min <= axe->value && axe->value <= dead_max
			&& dead_min <= value && value <= dead_max) {
                        /* both in the dead zone */
			if (abs(value - middle) >= abs(axe->value - middle)) {
				/* moving out of the center, do nothing */
				return;
			}
		}
	}

	axe->value = value;

	if (axe->min == axe->max) {
		axe->value_adj = value;
	}  else {
		int a = value - axe->min;
		int d = axe->max - axe->min;
		axe->value_adj = a * 256 / d - 128;
	}
	if (axe->value_adj < -128)
		axe->value_adj = -128;
	if (axe->value_adj > 128)
		axe->value_adj = 128;
}

void joystickb_event_poll(void)
{
	unsigned i;
	int type, code, value;

	log_debug(("joystickb:event: joystickb_event_poll()\n"));

	for(i=0;i<event_state.mac;++i) {
		struct joystick_item_context* item = event_state.map + i;

		while (event_read(item->f, &type, &code, &value) == 0) {
			if (type == EV_KEY) {
				unsigned j;
				for(j=0;j<item->button_mac;++j) {
					if (code == item->button_map[j].code) {
						item->button_map[j].state = value != 0;
						break;
					}
				}
			} else if (type == EV_ABS) {
				unsigned j;
				for(j=0;j<item->stick_mac;++j) {
					unsigned k;
					struct joystick_stick_context* stick = item->stick_map + j;
					for(k=0;k<stick->axe_mac;++k) {
						struct joystick_axe_context* axe = stick->axe_map + k;
						if (code == axe->code)
							joystickb_event_axe_set(axe, value);
					}
				}
			}
		}
	}
}

unsigned joystickb_event_flags(void)
{
	return 0;
}

adv_error joystickb_event_load(adv_conf* context)
{
	return 0;
}

void joystickb_event_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

joystickb_driver joystickb_event_driver = {
	"event",
	DEVICE,
	joystickb_event_load,
	joystickb_event_reg,
	joystickb_event_init,
	joystickb_event_done,
	joystickb_event_flags,
	joystickb_event_count_get,
	joystickb_event_stick_count_get,
	joystickb_event_stick_axe_count_get,
	joystickb_event_button_count_get,
	joystickb_event_stick_name_get,
	joystickb_event_stick_axe_name_get,
	joystickb_event_button_name_get,
	joystickb_event_button_get,
	joystickb_event_stick_axe_digital_get,
	joystickb_event_stick_axe_analog_get,
	joystickb_event_calib_start,
	joystickb_event_calib_next,
	joystickb_event_poll
};


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003 Andrea Mazzoleni
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

#include "jraw.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "snstring.h"

#include <linux/joystick.h>

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#define RAW_JOYSTICK_MAX 4
#define RAW_JOYSTICK_DEVICE_MAX 4
#define RAW_JOYSTICK_BUTTON_MAX 64
#define RAW_JOYSTICK_STICK_MAX 4
#define RAW_JOYSTICK_AXE_MAX 2

struct joystick_button_context {
	unsigned code;
	adv_bool state;
};

struct joystick_axe_context {
	int code;
	int value;
	int value_adj; /**< Value adjusted in range -JOYSTICK_DRIVER_BASE, JOYSTICK_DRIVER_BASE. */
};

struct joystick_stick_context {
	unsigned axe_mac;
	struct joystick_axe_context axe_map[RAW_JOYSTICK_AXE_MAX];
};

struct joystick_item_context {
	int f;
	unsigned stick_mac;
	struct joystick_stick_context stick_map[RAW_JOYSTICK_STICK_MAX];
	unsigned button_mac;
	struct joystick_button_context button_map[RAW_JOYSTICK_BUTTON_MAX];
};

struct joystickb_raw_context {
	unsigned mac;
	struct joystick_item_context map[RAW_JOYSTICK_MAX];
};

static struct joystickb_raw_context raw_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Linux raw joystick" },
{ 0, 0, 0 }
};

#define ABS_UNASSIGNED -1

static void joystickb_log(int f)
{
	char c;
	int version;
	char name_buffer[64];

	if (ioctl(f, JSIOCGVERSION, &version) >= 0) {
		log_std(("joystickb:raw: version %08x\n", version));
	}

	if (ioctl(f, JSIOCGNAME(sizeof(name_buffer)), name_buffer) >= 0) {
		log_std(("joystickb:raw: name %s\n", name_buffer));
	}

	if (ioctl(f, JSIOCGAXES, &c) >= 0) {
		log_std(("joystickb:raw: axes %d\n", (int)c));
	}

	if (ioctl(f, JSIOCGBUTTONS, &c) >= 0) {
		log_std(("joystickb:raw: buttons %d\n", (int)c));
	}
}

static adv_error joystickb_setup(struct joystick_item_context* item, int f)
{
	char c;
	unsigned i;

	item->f = f;

	if (ioctl(f, JSIOCGBUTTONS, &c) < 0) {
		log_std(("ERROR:joystickb:raw: ioctl(JSIOCGBUTTONS) failed\n"));
		return -1;
	}
	item->button_mac = 0;
	for(i=0;i<c;++i) {
		if (item->button_mac < RAW_JOYSTICK_BUTTON_MAX) {
			item->button_map[item->button_mac].code = i;
			item->button_map[item->button_mac].state = 0;
			++item->button_mac;
		}
	}

	if (ioctl(f, JSIOCGAXES, &c) < 0) {
		log_std(("ERROR:joystickb:raw: ioctl(JSIOCGAXES) failed\n"));
		return -1;
	}
	item->stick_mac = 0;
	for(i=0;i<c/2;++i) {
		if (item->stick_mac < RAW_JOYSTICK_STICK_MAX) {
			struct joystick_stick_context* stick = item->stick_map + item->stick_mac;

			stick->axe_mac = 2;
			stick->axe_map[0].code = i*2;
			stick->axe_map[0].value = 0;
			stick->axe_map[0].value_adj = 0;
			stick->axe_map[1].code = i*2+1;
			stick->axe_map[1].value = 0;
			stick->axe_map[1].value_adj = 0;

			++item->stick_mac;
		}
	}

	return 0;
}

adv_error joystickb_raw_init(int joystickb_id)
{
	unsigned i;
	adv_bool eacces = 0;

	log_std(("josytickb:raw: joystickb_raw_init(id:%d)\n", joystickb_id));

	log_std(("joystickb:raw: opening joystick from 0 to %d\n", RAW_JOYSTICK_DEVICE_MAX));

	raw_state.mac = 0;
	for(i=0;i<RAW_JOYSTICK_DEVICE_MAX;++i) {
		int f;
		int version;
		char file[128];

		if (raw_state.mac >= RAW_JOYSTICK_MAX)
			continue;

		snprintf(file, sizeof(file), "/dev/js%d", i);
		f = open(file, O_RDONLY | O_NONBLOCK);
		if (f == -1) {
			if (errno != ENODEV) {
				log_std(("joystickb:raw: error opening device %s, errno %d (%s)\n", file, errno, strerror(errno)));
			}
			if (errno == EACCES) {
				eacces = 1;
			}

			snprintf(file, sizeof(file), "/dev/input/js%d", i);
			f = open(file, O_RDONLY | O_NONBLOCK);
		}
		if (f == -1) {
			if (errno != ENODEV) {
				log_std(("joystickb:raw: error opening device %s, errno %d (%s)\n", file, errno, strerror(errno)));
			}
			continue;
		}

		log_std(("joystickb:raw: open device %s\n", file));

		if (ioctl(f, JSIOCGVERSION, &version) < 0) {
			log_std(("ERROR:joystickb:raw: ioctl(JSIOCVERSION) failed\n"));
			close(f);
			continue;
		}

		joystickb_log(f);

		if (joystickb_setup(&raw_state.map[raw_state.mac], f) != 0) {
			close(f);
			continue;
		}

		++raw_state.mac;
	}

	if (!raw_state.mac) {
		if (eacces)
			error_set("No joystick found. Check the /dev/js* and /dev/input/js* permissions.\n");
		else
			error_set("No joystick found.\n");
		return -1;
	}

	return 0;
}

void joystickb_raw_done(void)
{
	unsigned i;

	log_std(("josytickb:raw: joystickb_raw_done()\n"));

	for(i=0;i<raw_state.mac;++i)
		close(raw_state.map[i].f);
	raw_state.mac = 0;
}

unsigned joystickb_raw_count_get(void)
{
	log_debug(("joystickb:raw: joystickb_raw_count_get()\n"));

	return raw_state.mac;
}

unsigned joystickb_raw_stick_count_get(unsigned joystick)
{
	log_debug(("joystickb:raw: joystickb_raw_stick_count_get()\n"));

	return raw_state.map[joystick].stick_mac;
}

unsigned joystickb_raw_stick_axe_count_get(unsigned joystick, unsigned stick)
{
	log_debug(("joystickb:raw: joystickb_raw_stick_axe_count_get()\n"));

	return raw_state.map[joystick].stick_map[stick].axe_mac;
}

int joystickb_raw_stick_axe_analog_get(unsigned joystick, unsigned stick, unsigned axe)
{
	int r;
	log_debug(("joystickb:raw: joystickb_raw_stick_axe_analog_get()\n"));

	r = raw_state.map[joystick].stick_map[stick].axe_map[axe].value_adj;

	return r;
}

unsigned joystickb_raw_stick_axe_digital_get(unsigned joystick, unsigned stick, unsigned axe, unsigned d)
{
	int r;
	log_debug(("joystickb:raw: joystickb_raw_stick_axe_digital_get()\n"));

	r = joystickb_raw_stick_axe_analog_get(joystick, stick, axe);

	if (d)
		return r < -JOYSTICK_DRIVER_BASE/8; /* -1/8 of the partial range */
	else
		return r > JOYSTICK_DRIVER_BASE/8; /* +1/8 of the partial range */
}

unsigned joystickb_raw_button_count_get(unsigned joystick)
{
	log_debug(("joystickb:raw: joystickb_raw_button_count_get()\n"));

	return raw_state.map[joystick].button_mac;
}

unsigned joystickb_raw_button_get(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:raw: joystickb_raw_button_get()\n"));

	return raw_state.map[joystick].button_map[button].state != 0;
}

static void joystickb_raw_axe_set(struct joystick_axe_context* axe, int value)
{
	axe->value = value;

	axe->value_adj = joystickb_adjust_analog(value, -32767, 32767);
}

static adv_error joystickb_read(int f, int* type, int* code, int* value)
{
	int size;
	struct js_event e;

	size = read(f, &e, sizeof(e));

	if (size == -1 && errno == EAGAIN) {
		/* normal exit if data is missing */
		return -1;
	}

	if (size != sizeof(e)) {
		log_std(("ERROR:joystickb:raw: invalid read size %d on the joystick interface, errno %d (%s)\n", size, errno, strerror(errno)));
		return -1;
	}

	log_debug(("joystickb:raw: read time %d, type %d, code %d, value %d\n", e.time, e.type, e.number, e.value));

	*type = e.type;
	*code = e.number;
	*value = e.value;

	return 0;
}

void joystickb_raw_poll(void)
{
	unsigned i;
	int type, code, value;

	log_debug(("joystickb:raw: joystickb_raw_poll()\n"));

	for(i=0;i<raw_state.mac;++i) {
		struct joystick_item_context* item = raw_state.map + i;

		while (joystickb_read(item->f, &type, &code, &value) == 0) {

			type &= ~JS_EVENT_INIT; /* doesn't differentiate the INIT events */

			if (type == JS_EVENT_BUTTON) {
				unsigned j;
				for(j=0;j<item->button_mac;++j) {
					if (code == item->button_map[j].code) {
						item->button_map[j].state = value != 0;
						break;
					}
				}
			} else if (type == JS_EVENT_AXIS) {
				unsigned j;
				for(j=0;j<item->stick_mac;++j) {
					unsigned k;
					struct joystick_stick_context* stick = item->stick_map + j;
					for(k=0;k<stick->axe_mac;++k) {
						struct joystick_axe_context* axe = stick->axe_map + k;
						if (code == axe->code)
							joystickb_raw_axe_set(axe, value);
					}
				}
			}
		}
	}
}

unsigned joystickb_raw_flags(void)
{
	return 0;
}

adv_error joystickb_raw_load(adv_conf* context)
{
	return 0;
}

void joystickb_raw_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

joystickb_driver joystickb_raw_driver = {
	"raw",
	DEVICE,
	joystickb_raw_load,
	joystickb_raw_reg,
	joystickb_raw_init,
	joystickb_raw_done,
	0,
	0,
	joystickb_raw_flags,
	joystickb_raw_count_get,
	joystickb_raw_stick_count_get,
	joystickb_raw_stick_axe_count_get,
	0,
	0,
	joystickb_raw_stick_axe_digital_get,
	joystickb_raw_stick_axe_analog_get,
	joystickb_raw_button_count_get,
	0,
	joystickb_raw_button_get,
	0,
	0,
	0,
	0,
	0,
	joystickb_raw_poll
};


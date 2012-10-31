/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2004, 2005 Andrea Mazzoleni
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

#include "mevent.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "snstring.h"
#include "event.h"

#if defined(USE_SVGALIB)
#include <vga.h>
#include <vgamouse.h>
#endif

#include <linux/input.h>

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#define EVENT_MOUSE_MAX 8
#define EVENT_MOUSE_DEVICE_MAX 32
#define EVENT_MOUSE_AXE_MAX 8
#define EVENT_MOUSE_BUTTON_MAX 16
#define EVENT_MOUSE_NAME_MAX 128

struct mouse_axe_context {
	unsigned code;
	int value;
	char name[EVENT_MOUSE_NAME_MAX];
};

struct mouse_button_context {
	unsigned code;
	adv_bool state;
	char name[EVENT_MOUSE_NAME_MAX];
};

struct mouse_item_context {
	int f;
	unsigned char evtype_bitmask[EV_MAX/8 + 1];
	unsigned axe_mac;
	struct mouse_axe_context axe_map[EVENT_MOUSE_AXE_MAX];
	unsigned button_mac;
	struct mouse_button_context button_map[EVENT_MOUSE_BUTTON_MAX];
};

struct mouseb_event_context {
	unsigned mac;
	struct mouse_item_context map[EVENT_MOUSE_MAX];
};

static struct mouseb_event_context event_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Linux input-event mouse" },
{ 0, 0, 0 }
};

static adv_error mouseb_setup(struct mouse_item_context* item, int f)
{
	unsigned char key_bitmask[KEY_MAX/8 + 1];
	unsigned char rel_bitmask[REL_MAX/8 + 1];
	unsigned i;
	struct button_entry {
		int code;
		const char* name;
	} button_map[] = {
		#ifdef BTN_LEFT
		{ BTN_LEFT, "left" },
		#endif
		#ifdef BTN_RIGHT
		{ BTN_RIGHT, "right" },
		#endif
		#ifdef BTN_MIDDLE
		{ BTN_MIDDLE, "middle" },
		#endif
		#ifdef BTN_SIDE
		{ BTN_SIDE, "side" },
		#endif
		#ifdef BTN_EXTRA
		{ BTN_EXTRA, "extra" },
		#endif
		#ifdef BTN_FORWARD
		{ BTN_FORWARD, "forward" },
		#endif
		#ifdef BTN_BACK
		{ BTN_BACK, "back" }
		#endif
	};

	/* WARNING: It must be syncronized with the list in event.c */
	struct axe_entry {
		int code;
		const char* name;
	} axe_map[] = {
		#ifdef REL_X
		{ REL_X, "x" },
		#endif
		#ifdef REL_Y
		{ REL_Y, "y" },
		#endif
		#ifdef REL_Z
		{ REL_Z, "z" },
		#endif
		#ifdef REL_WHEEL
		{ REL_WHEEL, "wheel" },
		#endif
		#ifdef REL_HWHEEL
		{ REL_HWHEEL, "hwheel" },
		#endif
		#ifdef REL_DIAL
		{ REL_DIAL, "dial" },
		#endif
		#ifdef REL_MISC
		{ REL_MISC, "misc" }
		#endif
	};

	item->f = f;

	memset(key_bitmask, 0, sizeof(key_bitmask));
	if (event_test_bit(EV_KEY, item->evtype_bitmask)) {
		if (ioctl(f, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask) < 0) {
			log_std(("event: error in ioctl(EVIOCGBIT(EV_KEY,%d))\n", (int)KEY_MAX));
			return -1;
		}
	}

	item->button_mac = 0;
	for(i=0;i<sizeof(button_map)/sizeof(button_map[0]);++i) {
		if (event_test_bit(button_map[i].code, key_bitmask)) {
			if (item->button_mac < EVENT_MOUSE_BUTTON_MAX) {
				item->button_map[item->button_mac].code = button_map[i].code;
				item->button_map[item->button_mac].state = 0;
				sncpy(item->button_map[item->button_mac].name, sizeof(item->button_map[item->button_mac].name), button_map[i].name);
				++item->button_mac;
			}
		}
	}

	memset(rel_bitmask, 0, sizeof(rel_bitmask));
	if (event_test_bit(EV_REL, item->evtype_bitmask)) {
		if (ioctl(f, EVIOCGBIT(EV_REL, sizeof(rel_bitmask)), rel_bitmask) < 0) {
			log_std(("event: error in ioctl(EVIOCGBIT(EV_REL,%d))\n", (int)REL_MAX));
			return -1;
		}
	}

	item->axe_mac = 0;
	for(i=0;i<sizeof(axe_map)/sizeof(axe_map[0]);++i) {
		if (event_test_bit(axe_map[i].code, rel_bitmask)) {
			if (item->axe_mac < EVENT_MOUSE_AXE_MAX) {
				item->axe_map[item->axe_mac].code = axe_map[i].code;
				item->axe_map[item->axe_mac].value = 0;
				sncpy(item->axe_map[item->axe_mac].name, sizeof(item->axe_map[item->axe_mac].name), axe_map[i].name);
				++item->axe_mac;
			}
		}
	}

	return 0;
}

adv_error mouseb_event_init(int mouseb_id)
{
	unsigned i;
	adv_bool eacces = 0;
	struct event_location map[EVENT_MOUSE_DEVICE_MAX];
	unsigned mac;

	log_std(("mouseb:event: mouseb_event_init(id:%d)\n", mouseb_id));

#if defined(USE_SVGALIB)
	/* close the SVGALIB mouse device. SVGALIB always call mouse_init(), also */
	/* if mouse input is not requested */
	if (os_internal_svgalib_get()) {
		mouse_close();
	}
#endif

	log_std(("mouseb:event: opening mouse from 0 to %d\n", EVENT_MOUSE_DEVICE_MAX));

	mac = event_locate(map, EVENT_MOUSE_DEVICE_MAX, "event", &eacces);

	event_state.mac = 0;
	for(i=0;i<mac;++i) {
		int f;

		if (event_state.mac >= EVENT_MOUSE_MAX)
			continue;

		f = event_open(map[i].file, event_state.map[event_state.mac].evtype_bitmask, sizeof(event_state.map[event_state.mac].evtype_bitmask));
		if (f == -1) {
			if (errno == EACCES) {
				eacces = 1;
			}
			continue;
		}

		if (!event_is_mouse(f, event_state.map[event_state.mac].evtype_bitmask)) {
			log_std(("mouseb:event: not a mouse on device %s\n", map[i].file));
			event_close(f);
			continue;
		}

		if (mouseb_setup(&event_state.map[event_state.mac], f) != 0) {
			event_close(f);
			continue;
		}

		++event_state.mac;
	}

	if (!event_state.mac) {
		if (eacces)
			error_set("No mouse found. Check the /dev/input/event* permissions.\n");
		else
			error_set("No mouse found.\n");
		return -1;
	}

	return 0;
}

void mouseb_event_done(void)
{
	unsigned i;

	log_std(("mouseb:event: mouseb_event_done()\n"));

	for(i=0;i<event_state.mac;++i)
		event_close(event_state.map[i].f);
	event_state.mac = 0;
}

unsigned mouseb_event_count_get(void)
{
	log_debug(("mouseb:event: mouseb_event_count_get()\n"));

	return event_state.mac;
}

unsigned mouseb_event_axe_count_get(unsigned mouse)
{
	log_debug(("mouseb:event: mouseb_event_axe_count_get()\n"));

	return event_state.map[mouse].axe_mac;
}

const char* mouseb_event_axe_name_get(unsigned mouse, unsigned axe)
{
	log_debug(("mouseb:event: mouseb_event_button_axe_get()\n"));

	return event_state.map[mouse].axe_map[axe].name;
}

unsigned mouseb_event_button_count_get(unsigned mouse)
{
	log_debug(("mouseb:event: mouseb_event_button_count_get()\n"));

	return event_state.map[mouse].button_mac;
}

const char* mouseb_event_button_name_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:event: mouseb_event_button_name_get()\n"));

	return event_state.map[mouse].button_map[button].name;
}

int mouseb_event_axe_get(unsigned mouse, unsigned axe)
{
	int r;

	log_debug(("mouseb:event: mouseb_event_axe_get()\n"));

	r = event_state.map[mouse].axe_map[axe].value;
	event_state.map[mouse].axe_map[axe].value = 0;

	return r;
}

unsigned mouseb_event_button_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:event: mouseb_event_button_get()\n"));

	return event_state.map[mouse].button_map[button].state;
}

void mouseb_event_poll(void)
{
	unsigned i;
	int type, code, value;

	log_debug(("mouseb:event: mouseb_event_poll()\n"));

	for(i=0;i<event_state.mac;++i) {
		struct mouse_item_context* item = event_state.map + i;

		while (event_read(item->f, &type, &code, &value) == 0) {
			if (type == EV_KEY) {
				unsigned j;
				for(j=0;j<item->button_mac;++j) {
					if (code == item->button_map[j].code) {
						item->button_map[j].state = value != 0;
						break;
					}
				}
			} else if (type == EV_REL) {
				unsigned j;
				for(j=0;j<item->axe_mac;++j) {
					if (code == item->axe_map[j].code) {
						item->axe_map[j].value += value;
						break;
					}
				}
			}
		}
	}
}

unsigned mouseb_event_flags(void)
{
	return 0;
}

adv_error mouseb_event_load(adv_conf* context)
{
	return 0;
}

void mouseb_event_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_event_driver = {
	"event",
	DEVICE,
	mouseb_event_load,
	mouseb_event_reg,
	mouseb_event_init,
	mouseb_event_done,
	0,
	0,
	mouseb_event_flags,
	mouseb_event_count_get,
	mouseb_event_axe_count_get,
	mouseb_event_axe_name_get,
	mouseb_event_button_count_get,
	mouseb_event_button_name_get,
	mouseb_event_axe_get,
	mouseb_event_button_get,
	mouseb_event_poll
};


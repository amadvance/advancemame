/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2003 Andrea Mazzoleni
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

#include "kevent.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "event.h"
#include "portable.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <linux/input.h>

#define EVENT_KEYBOARD_MAX 8
#define EVENT_KEYBOARD_DEVICE_MAX 32

struct keyboard_item_context {
	int f;
	unsigned char evtype_bitmask[EV_MAX/8 + 1];
	unsigned char key_bitmask[KEY_MAX/8 + 1];
	adv_bool state[KEY_MAX];
};

#define LOW_INVALID ((unsigned)0xFFFFFFFF)

struct keyb_event_context {
	unsigned map_up_to_low[KEYB_MAX];
	unsigned mac;
	struct keyboard_item_context map[EVENT_KEYBOARD_MAX];
};

static struct keyb_pair {
	unsigned up_code;
	unsigned low_code;
} KEYS[] = {
{ KEYB_A, KEY_A },
{ KEYB_B, KEY_B },
{ KEYB_C, KEY_C },
{ KEYB_D, KEY_D },
{ KEYB_E, KEY_E },
{ KEYB_F, KEY_F },
{ KEYB_G, KEY_G },
{ KEYB_H, KEY_H },
{ KEYB_I, KEY_I },
{ KEYB_J, KEY_J },
{ KEYB_K, KEY_K },
{ KEYB_L, KEY_L },
{ KEYB_M, KEY_M },
{ KEYB_N, KEY_N },
{ KEYB_O, KEY_O },
{ KEYB_P, KEY_P },
{ KEYB_Q, KEY_Q },
{ KEYB_R, KEY_R },
{ KEYB_S, KEY_S },
{ KEYB_T, KEY_T },
{ KEYB_U, KEY_U },
{ KEYB_V, KEY_V },
{ KEYB_W, KEY_W },
{ KEYB_X, KEY_X },
{ KEYB_Y, KEY_Y },
{ KEYB_Z, KEY_Z },
{ KEYB_0, KEY_0 },
{ KEYB_1, KEY_1 },
{ KEYB_2, KEY_2 },
{ KEYB_3, KEY_3 },
{ KEYB_4, KEY_4 },
{ KEYB_5, KEY_5 },
{ KEYB_6, KEY_6 },
{ KEYB_7, KEY_7 },
{ KEYB_8, KEY_8 },
{ KEYB_9, KEY_9 },
{ KEYB_0_PAD, KEY_KP0 },
{ KEYB_1_PAD, KEY_KP1 },
{ KEYB_2_PAD, KEY_KP2 },
{ KEYB_3_PAD, KEY_KP3 },
{ KEYB_4_PAD, KEY_KP4 },
{ KEYB_5_PAD, KEY_KP5 },
{ KEYB_6_PAD, KEY_KP6 },
{ KEYB_7_PAD, KEY_KP7 },
{ KEYB_8_PAD, KEY_KP8 },
{ KEYB_9_PAD, KEY_KP9 },
{ KEYB_F1, KEY_F1 },
{ KEYB_F2, KEY_F2 },
{ KEYB_F3, KEY_F3 },
{ KEYB_F4, KEY_F4 },
{ KEYB_F5, KEY_F5 },
{ KEYB_F6, KEY_F6 },
{ KEYB_F7, KEY_F7 },
{ KEYB_F8, KEY_F8 },
{ KEYB_F9, KEY_F9 },
{ KEYB_F10, KEY_F10 },
{ KEYB_F11, KEY_F11 },
{ KEYB_F12, KEY_F12 },
{ KEYB_ESC, KEY_ESC },
{ KEYB_BACKQUOTE, KEY_GRAVE },
{ KEYB_MINUS, KEY_MINUS },
{ KEYB_EQUALS, KEY_EQUAL },
{ KEYB_BACKSPACE, KEY_BACKSPACE },
{ KEYB_TAB, KEY_TAB },
{ KEYB_OPENBRACE, KEY_LEFTBRACE },
{ KEYB_CLOSEBRACE, KEY_RIGHTBRACE },
{ KEYB_ENTER, KEY_ENTER },
{ KEYB_SEMICOLON, KEY_SEMICOLON },
{ KEYB_QUOTE, KEY_APOSTROPHE },
{ KEYB_BACKSLASH, /* KEY_BACKSLASH */ KEY_103RD },
{ KEYB_LESS, KEY_102ND },
{ KEYB_COMMA, KEY_COMMA },
{ KEYB_PERIOD, KEY_DOT },
{ KEYB_SLASH, KEY_SLASH },
{ KEYB_SPACE, KEY_SPACE },
{ KEYB_INSERT, KEY_INSERT },
{ KEYB_DEL, KEY_DELETE },
{ KEYB_HOME, KEY_HOME },
{ KEYB_END, KEY_END },
{ KEYB_PGUP, KEY_PAGEUP },
{ KEYB_PGDN, KEY_PAGEDOWN },
{ KEYB_LEFT, KEY_LEFT },
{ KEYB_RIGHT, KEY_RIGHT },
{ KEYB_UP, KEY_UP },
{ KEYB_DOWN, KEY_DOWN },
{ KEYB_SLASH_PAD, KEY_KPSLASH },
{ KEYB_ASTERISK, KEY_KPASTERISK },
{ KEYB_MINUS_PAD, KEY_KPMINUS },
{ KEYB_PLUS_PAD, KEY_KPPLUS },
{ KEYB_PERIOD_PAD, KEY_KPDOT },
{ KEYB_ENTER_PAD, KEY_KPENTER },
{ KEYB_PRTSCR, KEY_SYSRQ },
{ KEYB_PAUSE, KEY_PAUSE },
{ KEYB_LSHIFT, KEY_LEFTSHIFT },
{ KEYB_RSHIFT, KEY_RIGHTSHIFT },
{ KEYB_LCONTROL, KEY_LEFTCTRL },
{ KEYB_RCONTROL, KEY_RIGHTCTRL },
{ KEYB_ALT, KEY_LEFTALT },
{ KEYB_ALTGR, KEY_RIGHTALT },
{ KEYB_LWIN, KEY_LEFTMETA },
{ KEYB_RWIN, KEY_RIGHTMETA },
{ KEYB_MENU, KEY_MENU },
{ KEYB_SCRLOCK, KEY_SCROLLLOCK },
{ KEYB_NUMLOCK, KEY_NUMLOCK },
{ KEYB_CAPSLOCK, KEY_CAPSLOCK },
{ KEYB_STOP, KEY_STOP },
{ KEYB_AGAIN, KEY_AGAIN },
{ KEYB_PROPS, KEY_PROPS },
{ KEYB_UNDO, KEY_UNDO },
{ KEYB_FRONT, KEY_FRONT },
{ KEYB_COPY, KEY_COPY },
{ KEYB_OPEN, KEY_OPEN },
{ KEYB_PASTE, KEY_PASTE },
{ KEYB_FIND, KEY_FIND },
{ KEYB_CUT, KEY_CUT },
{ KEYB_HELP, KEY_HELP },
{ KEYB_MENU, KEY_MENU },
{ KEYB_CALC, KEY_CALC },
{ KEYB_SETUP, KEY_SETUP },
{ KEYB_SLEEP, KEY_SLEEP },
{ KEYB_WAKEUP, KEY_WAKEUP },
{ KEYB_FILE, KEY_FILE },
{ KEYB_SENDFILE, KEY_SENDFILE },
{ KEYB_DELETEFILE, KEY_DELETEFILE },
{ KEYB_XFER, KEY_XFER },
{ KEYB_PROG1, KEY_PROG1 },
{ KEYB_PROG2, KEY_PROG2 },
{ KEYB_WWW, KEY_WWW },
{ KEYB_MSDOS, KEY_MSDOS },
{ KEYB_COFFEE, KEY_COFFEE },
{ KEYB_DIRECTION, KEY_DIRECTION },
{ KEYB_CYCLEWINDOWS, KEY_CYCLEWINDOWS },
{ KEYB_MAIL, KEY_MAIL },
{ KEYB_BOOKMARKS, KEY_BOOKMARKS },
{ KEYB_COMPUTER, KEY_COMPUTER },
{ KEYB_BACK, KEY_BACK },
{ KEYB_FORWARD, KEY_FORWARD },
{ KEYB_CLOSECD, KEY_CLOSECD },
{ KEYB_EJECTCD, KEY_EJECTCD },
{ KEYB_EJECTCLOSECD, KEY_EJECTCLOSECD },
{ KEYB_NEXTSONG, KEY_NEXTSONG },
{ KEYB_PLAYPAUSE, KEY_PLAYPAUSE },
{ KEYB_PREVIOUSSONG, KEY_PREVIOUSSONG },
{ KEYB_STOPCD, KEY_STOPCD },
{ KEYB_RECORD, KEY_RECORD },
{ KEYB_REWIND, KEY_REWIND },
{ KEYB_PHONE, KEY_PHONE },
{ KEYB_ISO, KEY_ISO },
{ KEYB_CONFIG, KEY_CONFIG },
{ KEYB_HOMEPAGE, KEY_HOMEPAGE },
{ KEYB_REFRESH, KEY_REFRESH },
{ KEYB_EXIT, KEY_EXIT },
{ KEYB_MOVE, KEY_MOVE },
{ KEYB_EDIT, KEY_EDIT },
{ KEYB_SCROLLUP, KEY_SCROLLUP },
{ KEYB_SCROLLDOWN, KEY_SCROLLDOWN },
{ KEYB_KPLEFTPAREN, KEY_KPLEFTPAREN },
{ KEYB_KPRIGHTPAREN, KEY_KPRIGHTPAREN },
{ KEYB_INTL1, KEY_INTL1 },
{ KEYB_INTL2, KEY_INTL2 },
{ KEYB_INTL3, KEY_INTL3 },
{ KEYB_INTL4, KEY_INTL4 },
{ KEYB_INTL5, KEY_INTL5 },
{ KEYB_INTL6, KEY_INTL6 },
{ KEYB_INTL7, KEY_INTL7 },
{ KEYB_INTL8, KEY_INTL8 },
{ KEYB_INTL9, KEY_INTL9 },
{ KEYB_LANG1, KEY_LANG1 },
{ KEYB_LANG2, KEY_LANG2 },
{ KEYB_LANG3, KEY_LANG3 },
{ KEYB_LANG4, KEY_LANG4 },
{ KEYB_LANG5, KEY_LANG5 },
{ KEYB_LANG6, KEY_LANG6 },
{ KEYB_LANG7, KEY_LANG7 },
{ KEYB_LANG8, KEY_LANG8 },
{ KEYB_LANG9, KEY_LANG9 },
{ KEYB_PLAYCD, KEY_PLAYCD },
{ KEYB_PAUSECD, KEY_PAUSECD },
{ KEYB_PROG3, KEY_PROG3 },
{ KEYB_PROG4, KEY_PROG4 },
{ KEYB_SUSPEND, KEY_SUSPEND },
{ KEYB_CLOSE, KEY_CLOSE },
{ KEYB_BRIGHTNESSDOWN, KEY_BRIGHTNESSDOWN },
{ KEYB_BRIGHTNESSUP, KEY_BRIGHTNESSUP },
{ KEYB_MACRO, KEY_MACRO },
{ KEYB_MUTE, KEY_MUTE },
{ KEYB_VOLUMEDOWN, KEY_VOLUMEDOWN },
{ KEYB_VOLUMEUP, KEY_VOLUMEUP },
{ KEYB_POWER, KEY_POWER },
{ KEYB_COMPOSE, KEY_COMPOSE },
{ KEYB_F13, KEY_F13 },
{ KEYB_F14, KEY_F14 },
{ KEYB_F15, KEY_F15 },
{ KEYB_F16, KEY_F16 },
{ KEYB_F17, KEY_F17 },
{ KEYB_F18, KEY_F18 },
{ KEYB_F19, KEY_F19 },
{ KEYB_F20, KEY_F20 },
{ KEYB_F21, KEY_F21 },
{ KEYB_F22, KEY_F22 },
{ KEYB_F23, KEY_F23 },
{ KEYB_F24, KEY_F24 },
{ KEYB_MAX, 0 }
};

static struct keyb_event_context event_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Linux input-event keyboard" },
{ 0, 0, 0 }
};

static adv_error keyb_setup(struct keyboard_item_context* item, int f)
{
	unsigned i;

	item->f = f;

	for(i=0;i<KEY_MAX;++i)
		item->state[i] = 0;

	memset(item->key_bitmask, 0, sizeof(item->key_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_KEY, sizeof(item->key_bitmask)), item->key_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_KEY,%d))\n", (int)KEY_MAX));
		return -1;
	}

	return 0;
}

adv_error keyb_event_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* j;
	unsigned i;

	log_std(("keyb:event: keyb_event_init(id:%d, disable_special:%d)\n", keyb_id, (int)disable_special));

	for(i=0;i<KEYB_MAX;++i) {
		event_state.map_up_to_low[i] = LOW_INVALID;
	}
	for(j=KEYS;j->up_code != KEYB_MAX;++j) {
		event_state.map_up_to_low[j->up_code] = j->low_code;
	}

	event_state.mac = 0;
	for(i=0;i<EVENT_KEYBOARD_DEVICE_MAX;++i) {
		int f;
		char file[128];

		if (event_state.mac >= EVENT_KEYBOARD_MAX)
			continue;

		snprintf(file, sizeof(file), "/dev/input/event%d", i);

		f = event_open(file, event_state.map[event_state.mac].evtype_bitmask);
		if (f == -1)
			continue;

		event_log(f, event_state.map[event_state.mac].evtype_bitmask);

		if (!event_is_keyboard(event_state.map[event_state.mac].evtype_bitmask)) {
			log_std(("keyb:event: not a keyboard on device %s\n", file));
			event_close(f);
			continue;
		}

		if (keyb_setup(&event_state.map[event_state.mac], f) != 0) {
			event_close(f);
			continue;
		}

		++event_state.mac;
	}

	if (!event_state.mac) {
		error_set("No keyboard found on /dev/input/event*.\n");
		return -1;
	}

	return 0;
}

void keyb_event_done(void)
{
	unsigned i;

	log_std(("keyb:event: keyb_event_done()\n"));

	for(i=0;i<event_state.mac;++i)
		event_close(event_state.map[i].f);

	event_state.mac = 0;
}

adv_error keyb_event_enable(void)
{
	log_std(("keyb:event: keyb_event_enable()\n"));

	return 0;
}

void keyb_event_disable(void)
{
	log_std(("keyb:event: keyb_event_disable()\n"));
}

unsigned keyb_event_count_get(void)
{
	log_debug(("keyb:event: keyb_event_count_get(void)\n"));

	return event_state.mac;
}

adv_bool keyb_event_has(unsigned keyboard, unsigned code)
{
	log_debug(("keyb:event: keyb_event_has()\n"));

	assert(keyboard < keyb_event_count_get());
	assert(code < KEYB_MAX);

	/* remove unknown key */
	if (event_state.map_up_to_low[code] == LOW_INVALID)
		return 0;

	/* check if the key is really present in the keyboard */
	if (!event_test_bit(code, event_state.map[keyboard].key_bitmask))
		return 0;

	return 1;
}

unsigned keyb_event_get(unsigned keyboard, unsigned code)
{
	unsigned low_code;

	assert(keyboard < keyb_event_count_get());
	assert(code < KEYB_MAX);

	log_debug(("keyb:event: keyb_event_get(keyboard:%d,code:%d)\n", keyboard, code));

	/* disable the pause key */
	if (code == KEYB_PAUSE)
		return 0;

	low_code = event_state.map_up_to_low[code];
	if (low_code == LOW_INVALID)
		return 0;

	return event_state.map[keyboard].state[low_code];
}

void keyb_event_all_get(unsigned keyboard, unsigned char* code_map)
{
	unsigned i;

	assert(keyboard < keyb_event_count_get());

	log_debug(("keyb:event: keyb_event_all_get(keyboard:%d)\n", keyboard));

	for(i=0;i<KEYB_MAX;++i) {
		unsigned low_code = event_state.map_up_to_low[i];
		if (low_code == LOW_INVALID)
			code_map[i] = 0;
		else
			code_map[i] = event_state.map[keyboard].state[low_code];
	}

	/* disable the pause key */
	code_map[KEYB_PAUSE] = 0;
}

void keyb_event_poll(void)
{
	unsigned i;
	int type, code, value;

	log_debug(("keyb:event: keyb_event_poll()\n"));

	for(i=0;i<event_state.mac;++i) {
		struct keyboard_item_context* item = event_state.map + i;
		while (event_read(item->f, &type, &code, &value) == 0) {
			if (type == EV_KEY) {
				if (code < KEY_MAX)
					item->state[code] = value != 0;
			}
		}
	}
}

unsigned keyb_event_flags(void)
{
	return 0;
}

adv_error keyb_event_load(adv_conf* context)
{
	return 0;
}

void keyb_event_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

keyb_driver keyb_event_driver = {
	"event",
	DEVICE,
	keyb_event_load,
	keyb_event_reg,
	keyb_event_init,
	keyb_event_done,
	keyb_event_enable,
	keyb_event_disable,
	keyb_event_flags,
	keyb_event_count_get,
	keyb_event_has,
	keyb_event_get,
	keyb_event_all_get,
	keyb_event_poll
};


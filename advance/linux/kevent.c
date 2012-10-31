/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2005 Andrea Mazzoleni
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

#include "kevent.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "event.h"

#include <linux/input.h>

#if HAVE_TERMIOS_H
#include <termios.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_KD_H
#include <sys/kd.h>
#endif
#if HAVE_SYS_VT_H
#include <sys/vt.h>
#endif

#define EVENT_KEYBOARD_MAX 8
#define EVENT_KEYBOARD_DEVICE_MAX 32

struct keyboard_item_context {
	int fe; /**< Handle of the event interface. */
	unsigned char evtype_bitmask[EV_MAX/8 + 1];
	unsigned char key_bitmask[KEY_MAX/8 + 1];
	adv_bool state[KEY_MAX];
	unsigned led_state;
};

#define LOW_INVALID ((unsigned)0xFFFFFFFF)

struct keyb_event_context {
	struct termios old_kdbtermios;
	struct termios kdbtermios;
	int old_kdbmode;
	int old_terminalmode;
	int f; /**< Handle of the console interface. */
	adv_bool disable_special_flag; /**< Disable special hotkeys. */
	unsigned map_up_to_low[KEYB_MAX];
	unsigned mac;
	struct keyboard_item_context map[EVENT_KEYBOARD_MAX];
	adv_bool graphics_flag; /**< Set the terminal in graphics mode. */
	adv_bool passive_flag; /**< Be passive on some actions. Required for compatibility with other libs. */
};

static struct keyb_pair {
	unsigned up_code;
	unsigned low_code;
} KEYS[] = {
#ifdef KEY_A
{ KEYB_A, KEY_A },
#endif
#ifdef KEY_B
{ KEYB_B, KEY_B },
#endif
#ifdef KEY_C
{ KEYB_C, KEY_C },
#endif
#ifdef KEY_D
{ KEYB_D, KEY_D },
#endif
#ifdef KEY_E
{ KEYB_E, KEY_E },
#endif
#ifdef KEY_F
{ KEYB_F, KEY_F },
#endif
#ifdef KEY_G
{ KEYB_G, KEY_G },
#endif
#ifdef KEY_H
{ KEYB_H, KEY_H },
#endif
#ifdef KEY_I
{ KEYB_I, KEY_I },
#endif
#ifdef KEY_J
{ KEYB_J, KEY_J },
#endif
#ifdef KEY_K
{ KEYB_K, KEY_K },
#endif
#ifdef KEY_L
{ KEYB_L, KEY_L },
#endif
#ifdef KEY_M
{ KEYB_M, KEY_M },
#endif
#ifdef KEY_N
{ KEYB_N, KEY_N },
#endif
#ifdef KEY_O
{ KEYB_O, KEY_O },
#endif
#ifdef KEY_P
{ KEYB_P, KEY_P },
#endif
#ifdef KEY_Q
{ KEYB_Q, KEY_Q },
#endif
#ifdef KEY_R
{ KEYB_R, KEY_R },
#endif
#ifdef KEY_S
{ KEYB_S, KEY_S },
#endif
#ifdef KEY_T
{ KEYB_T, KEY_T },
#endif
#ifdef KEY_U
{ KEYB_U, KEY_U },
#endif
#ifdef KEY_V
{ KEYB_V, KEY_V },
#endif
#ifdef KEY_W
{ KEYB_W, KEY_W },
#endif
#ifdef KEY_X
{ KEYB_X, KEY_X },
#endif
#ifdef KEY_Y
{ KEYB_Y, KEY_Y },
#endif
#ifdef KEY_Z
{ KEYB_Z, KEY_Z },
#endif
#ifdef KEY_0
{ KEYB_0, KEY_0 },
#endif
#ifdef KEY_1
{ KEYB_1, KEY_1 },
#endif
#ifdef KEY_2
{ KEYB_2, KEY_2 },
#endif
#ifdef KEY_3
{ KEYB_3, KEY_3 },
#endif
#ifdef KEY_4
{ KEYB_4, KEY_4 },
#endif
#ifdef KEY_5
{ KEYB_5, KEY_5 },
#endif
#ifdef KEY_6
{ KEYB_6, KEY_6 },
#endif
#ifdef KEY_7
{ KEYB_7, KEY_7 },
#endif
#ifdef KEY_8
{ KEYB_8, KEY_8 },
#endif
#ifdef KEY_9
{ KEYB_9, KEY_9 },
#endif
#ifdef KEY_KP0
{ KEYB_0_PAD, KEY_KP0 },
#endif
#ifdef KEY_KP1
{ KEYB_1_PAD, KEY_KP1 },
#endif
#ifdef KEY_KP2
{ KEYB_2_PAD, KEY_KP2 },
#endif
#ifdef KEY_KP3
{ KEYB_3_PAD, KEY_KP3 },
#endif
#ifdef KEY_KP4
{ KEYB_4_PAD, KEY_KP4 },
#endif
#ifdef KEY_KP5
{ KEYB_5_PAD, KEY_KP5 },
#endif
#ifdef KEY_KP6
{ KEYB_6_PAD, KEY_KP6 },
#endif
#ifdef KEY_KP7
{ KEYB_7_PAD, KEY_KP7 },
#endif
#ifdef KEY_KP8
{ KEYB_8_PAD, KEY_KP8 },
#endif
#ifdef KEY_KP9
{ KEYB_9_PAD, KEY_KP9 },
#endif
#ifdef KEY_F1
{ KEYB_F1, KEY_F1 },
#endif
#ifdef KEY_F2
{ KEYB_F2, KEY_F2 },
#endif
#ifdef KEY_F3
{ KEYB_F3, KEY_F3 },
#endif
#ifdef KEY_F4
{ KEYB_F4, KEY_F4 },
#endif
#ifdef KEY_F5
{ KEYB_F5, KEY_F5 },
#endif
#ifdef KEY_F6
{ KEYB_F6, KEY_F6 },
#endif
#ifdef KEY_F7
{ KEYB_F7, KEY_F7 },
#endif
#ifdef KEY_F8
{ KEYB_F8, KEY_F8 },
#endif
#ifdef KEY_F9
{ KEYB_F9, KEY_F9 },
#endif
#ifdef KEY_F10
{ KEYB_F10, KEY_F10 },
#endif
#ifdef KEY_F11
{ KEYB_F11, KEY_F11 },
#endif
#ifdef KEY_F12
{ KEYB_F12, KEY_F12 },
#endif
#ifdef KEY_ESC
{ KEYB_ESC, KEY_ESC },
#endif
#ifdef KEY_GRAVE
{ KEYB_BACKQUOTE, KEY_GRAVE },
#endif
#ifdef KEY_MINUS
{ KEYB_MINUS, KEY_MINUS },
#endif
#ifdef KEY_EQUAL
{ KEYB_EQUALS, KEY_EQUAL },
#endif
#ifdef KEY_BACKSPACE
{ KEYB_BACKSPACE, KEY_BACKSPACE },
#endif
#ifdef KEY_TAB
{ KEYB_TAB, KEY_TAB },
#endif
#ifdef KEY_LEFTBRACE
{ KEYB_OPENBRACE, KEY_LEFTBRACE },
#endif
#ifdef KEY_RIGHTBRACE
{ KEYB_CLOSEBRACE, KEY_RIGHTBRACE },
#endif
#ifdef KEY_ENTER
{ KEYB_ENTER, KEY_ENTER },
#endif
#ifdef KEY_SEMICOLON
{ KEYB_SEMICOLON, KEY_SEMICOLON },
#endif
#ifdef KEY_APOSTROPHE
{ KEYB_QUOTE, KEY_APOSTROPHE },
#endif
#ifdef KEY_103RD
{ KEYB_BACKSLASH, KEY_103RD }, /* KEY_BACKSLASH */
#endif
#ifdef KEY_102ND
{ KEYB_LESS, KEY_102ND },
#endif
#ifdef KEY_COMMA
{ KEYB_COMMA, KEY_COMMA },
#endif
#ifdef KEY_DOT
{ KEYB_PERIOD, KEY_DOT },
#endif
#ifdef KEY_SLASH
{ KEYB_SLASH, KEY_SLASH },
#endif
#ifdef KEY_SPACE
{ KEYB_SPACE, KEY_SPACE },
#endif
#ifdef KEY_INSERT
{ KEYB_INSERT, KEY_INSERT },
#endif
#ifdef KEY_DELETE
{ KEYB_DEL, KEY_DELETE },
#endif
#ifdef KEY_HOME
{ KEYB_HOME, KEY_HOME },
#endif
#ifdef KEY_END
{ KEYB_END, KEY_END },
#endif
#ifdef KEY_PAGEUP
{ KEYB_PGUP, KEY_PAGEUP },
#endif
#ifdef KEY_PAGEDOWN
{ KEYB_PGDN, KEY_PAGEDOWN },
#endif
#ifdef KEY_LEFT
{ KEYB_LEFT, KEY_LEFT },
#endif
#ifdef KEY_RIGHT
{ KEYB_RIGHT, KEY_RIGHT },
#endif
#ifdef KEY_UP
{ KEYB_UP, KEY_UP },
#endif
#ifdef KEY_DOWN
{ KEYB_DOWN, KEY_DOWN },
#endif
#ifdef KEY_KPSLASH
{ KEYB_SLASH_PAD, KEY_KPSLASH },
#endif
#ifdef KEY_KPASTERISK
{ KEYB_ASTERISK, KEY_KPASTERISK },
#endif
#ifdef KEY_KPMINUS
{ KEYB_MINUS_PAD, KEY_KPMINUS },
#endif
#ifdef KEY_KPPLUS
{ KEYB_PLUS_PAD, KEY_KPPLUS },
#endif
#ifdef KEY_KPDOT
{ KEYB_PERIOD_PAD, KEY_KPDOT },
#endif
#ifdef KEY_KPENTER
{ KEYB_ENTER_PAD, KEY_KPENTER },
#endif
#ifdef KEY_SYSRQ
{ KEYB_PRTSCR, KEY_SYSRQ },
#endif
#ifdef KEY_PAUSE
{ KEYB_PAUSE, KEY_PAUSE },
#endif
#ifdef KEY_LEFTSHIFT
{ KEYB_LSHIFT, KEY_LEFTSHIFT },
#endif
#ifdef KEY_RIGHTSHIFT
{ KEYB_RSHIFT, KEY_RIGHTSHIFT },
#endif
#ifdef KEY_LEFTCTRL
{ KEYB_LCONTROL, KEY_LEFTCTRL },
#endif
#ifdef KEY_RIGHTCTRL
{ KEYB_RCONTROL, KEY_RIGHTCTRL },
#endif
#ifdef KEY_LEFTALT
{ KEYB_ALT, KEY_LEFTALT },
#endif
#ifdef KEY_RIGHTALT
{ KEYB_ALTGR, KEY_RIGHTALT },
#endif
#ifdef KEY_LEFTMETA
{ KEYB_LWIN, KEY_LEFTMETA },
#endif
#ifdef KEY_RIGHTMETA
{ KEYB_RWIN, KEY_RIGHTMETA },
#endif
#ifdef KEY_MENU
{ KEYB_MENU, KEY_MENU },
#endif
#ifdef KEY_SCROLLLOCK
{ KEYB_SCRLOCK, KEY_SCROLLLOCK },
#endif
#ifdef KEY_NUMLOCK
{ KEYB_NUMLOCK, KEY_NUMLOCK },
#endif
#ifdef KEY_CAPSLOCK
{ KEYB_CAPSLOCK, KEY_CAPSLOCK },
#endif
#ifdef KEY_STOP
{ KEYB_STOP, KEY_STOP },
#endif
#ifdef KEY_AGAIN
{ KEYB_AGAIN, KEY_AGAIN },
#endif
#ifdef KEY_PROPS
{ KEYB_PROPS, KEY_PROPS },
#endif
#ifdef KEY_UNDO
{ KEYB_UNDO, KEY_UNDO },
#endif
#ifdef KEY_FRONT
{ KEYB_FRONT, KEY_FRONT },
#endif
#ifdef KEY_COPY
{ KEYB_COPY, KEY_COPY },
#endif
#ifdef KEY_OPEN
{ KEYB_OPEN, KEY_OPEN },
#endif
#ifdef KEY_PASTE
{ KEYB_PASTE, KEY_PASTE },
#endif
#ifdef KEY_FIND
{ KEYB_FIND, KEY_FIND },
#endif
#ifdef KEY_CUT
{ KEYB_CUT, KEY_CUT },
#endif
#ifdef KEY_HELP
{ KEYB_HELP, KEY_HELP },
#endif
#ifdef KEY_MENU
{ KEYB_MENU, KEY_MENU },
#endif
#ifdef KEY_CALC
{ KEYB_CALC, KEY_CALC },
#endif
#ifdef KEY_SETUP
{ KEYB_SETUP, KEY_SETUP },
#endif
#ifdef KEY_SLEEP
{ KEYB_SLEEP, KEY_SLEEP },
#endif
#ifdef KEY_WAKEUP
{ KEYB_WAKEUP, KEY_WAKEUP },
#endif
#ifdef KEY_FILE
{ KEYB_FILE, KEY_FILE },
#endif
#ifdef KEY_SENDFILE
{ KEYB_SENDFILE, KEY_SENDFILE },
#endif
#ifdef KEY_DELETEFILE
{ KEYB_DELETEFILE, KEY_DELETEFILE },
#endif
#ifdef KEY_XFER
{ KEYB_XFER, KEY_XFER },
#endif
#ifdef KEY_PROG1
{ KEYB_PROG1, KEY_PROG1 },
#endif
#ifdef KEY_PROG2
{ KEYB_PROG2, KEY_PROG2 },
#endif
#ifdef KEY_WWW
{ KEYB_WWW, KEY_WWW },
#endif
#ifdef KEY_MSDOS
{ KEYB_MSDOS, KEY_MSDOS },
#endif
#ifdef KEY_COFFEE
{ KEYB_COFFEE, KEY_COFFEE },
#endif
#ifdef KEY_DIRECTION
{ KEYB_DIRECTION, KEY_DIRECTION },
#endif
#ifdef KEY_CYCLEWINDOWS
{ KEYB_CYCLEWINDOWS, KEY_CYCLEWINDOWS },
#endif
#ifdef KEY_MAIL
{ KEYB_MAIL, KEY_MAIL },
#endif
#ifdef KEY_BOOKMARKS
{ KEYB_BOOKMARKS, KEY_BOOKMARKS },
#endif
#ifdef KEY_COMPUTER
{ KEYB_COMPUTER, KEY_COMPUTER },
#endif
#ifdef KEY_BACK
{ KEYB_BACK, KEY_BACK },
#endif
#ifdef KEY_FORWARD
{ KEYB_FORWARD, KEY_FORWARD },
#endif
#ifdef KEY_CLOSECD
{ KEYB_CLOSECD, KEY_CLOSECD },
#endif
#ifdef KEY_EJECTCD
{ KEYB_EJECTCD, KEY_EJECTCD },
#endif
#ifdef KEY_EJECTCLOSECD
{ KEYB_EJECTCLOSECD, KEY_EJECTCLOSECD },
#endif
#ifdef KEY_NEXTSONG
{ KEYB_NEXTSONG, KEY_NEXTSONG },
#endif
#ifdef KEY_PLAYPAUSE
{ KEYB_PLAYPAUSE, KEY_PLAYPAUSE },
#endif
#ifdef KEY_PREVIOUSSONG
{ KEYB_PREVIOUSSONG, KEY_PREVIOUSSONG },
#endif
#ifdef KEY_STOPCD
{ KEYB_STOPCD, KEY_STOPCD },
#endif
#ifdef KEY_RECORD
{ KEYB_RECORD, KEY_RECORD },
#endif
#ifdef KEY_REWIND
{ KEYB_REWIND, KEY_REWIND },
#endif
#ifdef KEY_PHONE
{ KEYB_PHONE, KEY_PHONE },
#endif
#ifdef KEY_ISO
{ KEYB_ISO, KEY_ISO },
#endif
#ifdef KEY_CONFIG
{ KEYB_CONFIG, KEY_CONFIG },
#endif
#ifdef KEY_HOMEPAGE
{ KEYB_HOMEPAGE, KEY_HOMEPAGE },
#endif
#ifdef KEY_REFRESH
{ KEYB_REFRESH, KEY_REFRESH },
#endif
#ifdef KEY_EXIT
{ KEYB_EXIT, KEY_EXIT },
#endif
#ifdef KEY_MOVE
{ KEYB_MOVE, KEY_MOVE },
#endif
#ifdef KEY_EDIT
{ KEYB_EDIT, KEY_EDIT },
#endif
#ifdef KEY_SCROLLUP
{ KEYB_SCROLLUP, KEY_SCROLLUP },
#endif
#ifdef KEY_SCROLLDOWN
{ KEYB_SCROLLDOWN, KEY_SCROLLDOWN },
#endif
#ifdef KEY_KPLEFTPAREN
{ KEYB_KPLEFTPAREN, KEY_KPLEFTPAREN },
#endif
#ifdef KEY_KPRIGHTPAREN
{ KEYB_KPRIGHTPAREN, KEY_KPRIGHTPAREN },
#endif
#ifdef KEY_INTL1
{ KEYB_INTL1, KEY_INTL1 },
#endif
#ifdef KEY_INTL2
{ KEYB_INTL2, KEY_INTL2 },
#endif
#ifdef KEY_INTL3
{ KEYB_INTL3, KEY_INTL3 },
#endif
#ifdef KEY_INTL4
{ KEYB_INTL4, KEY_INTL4 },
#endif
#ifdef KEY_INTL5
{ KEYB_INTL5, KEY_INTL5 },
#endif
#ifdef KEY_INTL6
{ KEYB_INTL6, KEY_INTL6 },
#endif
#ifdef KEY_INTL7
{ KEYB_INTL7, KEY_INTL7 },
#endif
#ifdef KEY_INTL8
{ KEYB_INTL8, KEY_INTL8 },
#endif
#ifdef KEY_INTL9
{ KEYB_INTL9, KEY_INTL9 },
#endif
#ifdef KEY_LANG1
{ KEYB_LANG1, KEY_LANG1 },
#endif
#ifdef KEY_LANG2
{ KEYB_LANG2, KEY_LANG2 },
#endif
#ifdef KEY_LANG3
{ KEYB_LANG3, KEY_LANG3 },
#endif
#ifdef KEY_LANG4
{ KEYB_LANG4, KEY_LANG4 },
#endif
#ifdef KEY_LANG5
{ KEYB_LANG5, KEY_LANG5 },
#endif
#ifdef KEY_LANG6
{ KEYB_LANG6, KEY_LANG6 },
#endif
#ifdef KEY_LANG7
{ KEYB_LANG7, KEY_LANG7 },
#endif
#ifdef KEY_LANG8
{ KEYB_LANG8, KEY_LANG8 },
#endif
#ifdef KEY_LANG9
{ KEYB_LANG9, KEY_LANG9 },
#endif
#ifdef KEY_PLAYCD
{ KEYB_PLAYCD, KEY_PLAYCD },
#endif
#ifdef KEY_PAUSECD
{ KEYB_PAUSECD, KEY_PAUSECD },
#endif
#ifdef KEY_PROG3
{ KEYB_PROG3, KEY_PROG3 },
#endif
#ifdef KEY_PROG4
{ KEYB_PROG4, KEY_PROG4 },
#endif
#ifdef KEY_SUSPEND
{ KEYB_SUSPEND, KEY_SUSPEND },
#endif
#ifdef KEY_CLOSE
{ KEYB_CLOSE, KEY_CLOSE },
#endif
#ifdef KEY_BRIGHTNESSDOWN
{ KEYB_BRIGHTNESSDOWN, KEY_BRIGHTNESSDOWN },
#endif
#ifdef KEY_BRIGHTNESSUP
{ KEYB_BRIGHTNESSUP, KEY_BRIGHTNESSUP },
#endif
#ifdef KEY_MACRO
{ KEYB_MACRO, KEY_MACRO },
#endif
#ifdef KEY_MUTE
{ KEYB_MUTE, KEY_MUTE },
#endif
#ifdef KEY_VOLUMEDOWN
{ KEYB_VOLUMEDOWN, KEY_VOLUMEDOWN },
#endif
#ifdef KEY_VOLUMEUP
{ KEYB_VOLUMEUP, KEY_VOLUMEUP },
#endif
#ifdef KEY_POWER
{ KEYB_POWER, KEY_POWER },
#endif
#ifdef KEY_COMPOSE
{ KEYB_COMPOSE, KEY_COMPOSE },
#endif
#ifdef KEY_F13
{ KEYB_F13, KEY_F13 },
#endif
#ifdef KEY_F14
{ KEYB_F14, KEY_F14 },
#endif
#ifdef KEY_F15
{ KEYB_F15, KEY_F15 },
#endif
#ifdef KEY_F16
{ KEYB_F16, KEY_F16 },
#endif
#ifdef KEY_F17
{ KEYB_F17, KEY_F17 },
#endif
#ifdef KEY_F18
{ KEYB_F18, KEY_F18 },
#endif
#ifdef KEY_F19
{ KEYB_F19, KEY_F19 },
#endif
#ifdef KEY_F20
{ KEYB_F20, KEY_F20 },
#endif
#ifdef KEY_F21
{ KEYB_F21, KEY_F21 },
#endif
#ifdef KEY_F22
{ KEYB_F22, KEY_F22 },
#endif
#ifdef KEY_F23
{ KEYB_F23, KEY_F23 },
#endif
#ifdef KEY_F24
{ KEYB_F24, KEY_F24 },
#endif
{ KEYB_MAX, 0 }
};

static struct keyb_event_context event_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Linux input-event keyboard" },
{ 0, 0, 0 }
};

static void keyb_event_clear(void)
{
	unsigned i, j;

	for(i=0;i<event_state.mac;++i) {
		for(j=0;j<KEY_MAX;++j) {
			event_state.map[i].state[j] = 0;
			event_state.map[i].led_state = 0;
		}
	}
}

static adv_error keyb_event_setup(struct keyboard_item_context* item, int f)
{
	item->fe = f;

	memset(item->key_bitmask, 0, sizeof(item->key_bitmask));
	if (event_test_bit(EV_KEY, item->evtype_bitmask)) {
		if (ioctl(f, EVIOCGBIT(EV_KEY, sizeof(item->key_bitmask)), item->key_bitmask) < 0) {
			log_std(("event: error in ioctl(EVIOCGBIT(EV_KEY,%d))\n", (int)KEY_MAX));
			return -1;
		}
	}

	return 0;
}

adv_error keyb_event_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* j;
	unsigned i;
	adv_bool eacces = 0;
	struct event_location map[EVENT_KEYBOARD_DEVICE_MAX];
	unsigned mac;

	log_std(("keyb:event: keyb_event_init(id:%d, disable_special:%d)\n", keyb_id, (int)disable_special));

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

	for(i=0;i<KEYB_MAX;++i) {
		event_state.map_up_to_low[i] = LOW_INVALID;
	}
	for(j=KEYS;j->up_code != KEYB_MAX;++j) {
		event_state.map_up_to_low[j->up_code] = j->low_code;
	}

	mac = event_locate(map, EVENT_KEYBOARD_DEVICE_MAX, "event", &eacces);

	event_state.mac = 0;
	for(i=0;i<mac;++i) {
		int f;

		if (event_state.mac >= EVENT_KEYBOARD_MAX)
			continue;

		f = event_open(map[i].file, event_state.map[event_state.mac].evtype_bitmask, sizeof(event_state.map[event_state.mac].evtype_bitmask));
		if (f == -1)
			continue;

		if (!event_is_keyboard(f, event_state.map[event_state.mac].evtype_bitmask)) {
			log_std(("keyb:event: not a keyboard on device %s\n", map[i].file));
			event_close(f);
			continue;
		}

		if (keyb_event_setup(&event_state.map[event_state.mac], f) != 0) {
			event_close(f);
			continue;
		}

		++event_state.mac;
	}

	if (!event_state.mac) {
		error_set("No keyboard found.\n");
		return -1;
	}

	event_state.disable_special_flag = disable_special;

	return 0;
}

void keyb_event_done(void)
{
	unsigned i;

	log_std(("keyb:event: keyb_event_done()\n"));

	for(i=0;i<event_state.mac;++i)
		event_close(event_state.map[i].fe);

	event_state.mac = 0;
}

adv_error keyb_event_enable(adv_bool graphics)
{
	log_std(("keyb:event: keyb_event_enable(graphics:%d)\n", (int)graphics));

#if defined(USE_VIDEO_SDL)
	if (os_internal_sdl_is_video_active()) {
		error_set("The event keyboard driver cannot be used with the SDL video driver\n");
		return -1;
	}
#endif

	event_state.passive_flag = 0;
#ifdef USE_VIDEO_SVGALIB
	/* SVGALIB already set the terminal in KD_GRAPHICS mode and */
	/* it waits on a signal on a vt switch */
	if (os_internal_svgalib_is_video_mode_active()) {
		event_state.passive_flag = 1;
	}
#endif

	event_state.graphics_flag = graphics;

	event_state.f = open("/dev/tty", O_RDONLY | O_NONBLOCK);
	if (event_state.f == -1) {
		error_set("Error enabling the event keyboard driver. Function open(/dev/tty) failed.\n");
		goto err;
	}

	if (ioctl(event_state.f, KDGKBMODE, &event_state.old_kdbmode) != 0) {
		error_set("Error enabling the event keyboard driver. Function ioctl(KDGKBMODE) failed.\n");
		goto err_close;
	}

	if (tcgetattr(event_state.f, &event_state.old_kdbtermios) != 0) {
		error_set("Error enabling the event keyboard driver. Function tcgetattr() failed.\n");
		goto err_close;
	}

	event_state.kdbtermios = event_state.old_kdbtermios;

	/* setting taken from SVGALIB */
	event_state.kdbtermios.c_lflag &= ~(ICANON | ECHO | ISIG);
	event_state.kdbtermios.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
	event_state.kdbtermios.c_cc[VMIN] = 0;
	event_state.kdbtermios.c_cc[VTIME] = 0;

	if (tcsetattr(event_state.f, TCSAFLUSH, &event_state.kdbtermios) != 0) {
		error_set("Error enabling the event keyboard driver. Function tcsetattr(TCSAFLUSH) failed.\n");
		goto err_close;
	}

	if (event_state.disable_special_flag) {
		/* enter in raw mode only to disable the ALT+Fx sequences */
		if (ioctl(event_state.f, KDSKBMODE, K_MEDIUMRAW) != 0) {
			error_set("Error enabling the event keyboard driver. Function ioctl(KDSKBMODE) failed.\n");
			goto err_term;
		}
	}

	if (event_state.graphics_flag) {
		if (ioctl(event_state.f, KDGETMODE, &event_state.old_terminalmode) != 0) {
			error_set("Error enabling the event keyboard driver. Function ioctl(KDGETMODE) failed.\n");
			goto err_mode;
		}

		if (event_state.old_terminalmode == KD_GRAPHICS) {
			log_std(("WARNING:keyb:event: terminal already in KD_GRAPHICS mode\n"));
		}

		/* set the console in graphics mode, it only disable the cursor and the echo */
		log_std(("keyb:event: ioctl(KDSETMODE, KD_GRAPHICS)\n"));
		if (ioctl(event_state.f, KDSETMODE, KD_GRAPHICS) < 0) {
			log_std(("keyb:event: ioctl(KDSETMODE, KD_GRAPHICS) failed\n"));
			error_set("Error setting the tty in graphics mode.\n");
			goto err_mode;
		}
	}

	keyb_event_clear();

	return 0;

err_mode:
	if (ioctl(event_state.f, KDSKBMODE, event_state.old_kdbmode) < 0) {
		/* ignore error */
		log_std(("keyb:event: ioctl(KDSKBMODE,old) failed\n"));
	}
err_term:
	if (tcsetattr(event_state.f, TCSAFLUSH, &event_state.old_kdbtermios) != 0) {
		/* ignore error */
		log_std(("keyb:event: tcsetattr(TCSAFLUSH) failed\n"));
	}
err_close:
	close(event_state.f);
err:
	return -1;
}

void keyb_event_disable(void)
{
	log_std(("keyb:event: keyb_event_disable()\n"));

	if (event_state.graphics_flag) {
		if (ioctl(event_state.f, KDSETMODE, event_state.old_terminalmode) < 0) {
			/* ignore error */
			log_std(("ERROR:keyb:event: ioctl(KDSETMODE, KD_TEXT) failed\n"));
		}
	}

	if (ioctl(event_state.f, KDSKBMODE, event_state.old_kdbmode) < 0) {
		/* ignore error */
		log_std(("ERROR:keyb:event: ioctl(KDSKBMODE,old) failed\n"));
	}

	if (tcsetattr(event_state.f, TCSAFLUSH, &event_state.old_kdbtermios) != 0) {
		/* ignore error */
		log_std(("ERROR:keyb:event: tcsetattr(TCSAFLUSH) failed\n"));
	}

	close(event_state.f);
}

unsigned keyb_event_count_get(void)
{
	log_debug(("keyb:event: keyb_event_count_get(void)\n"));

	return event_state.mac;
}

adv_bool keyb_event_has(unsigned keyboard, unsigned code)
{
	log_debug(("keyb:event: keyb_event_has()\n"));

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

void keyb_event_led_set(unsigned keyboard, unsigned led_mask)
{
	log_debug(("keyb:event: keyb_event_led_set(keyboard:%d,mask:%d)\n", keyboard, led_mask));

#define led_update(i, k) \
	do { \
		if (((led_mask ^ event_state.map[keyboard].led_state) & k) != 0) \
			event_write(event_state.map[keyboard].fe, EV_LED, i, (led_mask & k) != 0); \
	} while (0)

#ifdef LED_NUML
	led_update(LED_NUML, KEYB_LED_NUML);
#endif
#ifdef LED_CAPSL
	led_update(LED_CAPSL, KEYB_LED_CAPSL);
#endif
#ifdef LED_SCROLLL
	led_update(LED_SCROLLL, KEYB_LED_SCROLLL);
#endif
#ifdef LED_COMPOSE
	led_update(LED_COMPOSE, KEYB_LED_COMPOSE);
#endif
#ifdef LED_KANA
	led_update(LED_KANA, KEYB_LED_KANA);
#endif
#ifdef LED_SLEEP
	led_update(LED_SLEEP, KEYB_LED_SLEEP);
#endif
#ifdef LED_SUSPEND
	led_update(LED_SUSPEND, KEYB_LED_SUSPEND);
#endif
#ifdef LED_MUTE
	led_update(LED_MUTE, KEYB_LED_MUTE);
#endif

	event_state.map[keyboard].led_state = led_mask;
}

static void keyb_event_process(unsigned keyboard, unsigned code)
{
	/* check only if required */
	if (event_state.disable_special_flag)
		return;

	/* check only if a switch key is pressed/released */
	switch (code) {
	case KEY_LEFTCTRL :
	case KEY_RIGHTCTRL :
	case KEY_C :
		break;
	default:
		return;
	}

	/* check for CTRL+C */
	if ((event_state.map[keyboard].state[KEY_LEFTCTRL] || event_state.map[keyboard].state[KEY_RIGHTCTRL])
		&& event_state.map[keyboard].state[KEY_C]) {
		raise(SIGINT);
		return;
	}
}

void keyb_event_poll(void)
{
	unsigned i;
	int type, code, value;

	log_debug(("keyb:event: keyb_event_poll()\n"));

	for(i=0;i<event_state.mac;++i) {
		struct keyboard_item_context* item = event_state.map + i;
		while (event_read(item->fe, &type, &code, &value) == 0) {
			if (type == EV_KEY) {
				if (code < KEY_MAX)
					item->state[code] = value != 0;
				keyb_event_process(i, code);
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
	keyb_event_led_set,
	keyb_event_poll
};


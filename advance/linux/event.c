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

#include "event.h"
#include "log.h"

#include <linux/input.h>

static void event_key_log(int f)
{
	unsigned char key_bitmask[KEY_MAX/8 + 1];
	unsigned i;

	memset(key_bitmask, 0, sizeof(key_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_KEY,%d))\n", (int)KEY_MAX));
		return;
	}

	log_std(("event: EV_KEY:"));

	for(i=0;i<KEY_MAX;++i) {
		if (event_test_bit(i, key_bitmask)) {
			const char* desc;
			switch (i) {
			#ifdef KEY_ESC
			case KEY_ESC : desc = "KEY_ESC"; break;
			#endif
			#ifdef KEY_1
			case KEY_1 : desc = "KEY_1"; break;
			#endif
			#ifdef KEY_2
			case KEY_2 : desc = "KEY_2"; break;
			#endif
			#ifdef KEY_3
			case KEY_3 : desc = "KEY_3"; break;
			#endif
			#ifdef KEY_4
			case KEY_4 : desc = "KEY_4"; break;
			#endif
			#ifdef KEY_5
			case KEY_5 : desc = "KEY_5"; break;
			#endif
			#ifdef KEY_6
			case KEY_6 : desc = "KEY_6"; break;
			#endif
			#ifdef KEY_7
			case KEY_7 : desc = "KEY_7"; break;
			#endif
			#ifdef KEY_8
			case KEY_8 : desc = "KEY_8"; break;
			#endif
			#ifdef KEY_9
			case KEY_9 : desc = "KEY_9"; break;
			#endif
			#ifdef KEY_0
			case KEY_0 : desc = "KEY_0"; break;
			#endif
			#ifdef KEY_MINUS
			case KEY_MINUS : desc = "KEY_MINUS"; break;
			#endif
			#ifdef KEY_EQUAL
			case KEY_EQUAL : desc = "KEY_EQUAL"; break;
			#endif
			#ifdef KEY_BACKSPACE
			case KEY_BACKSPACE : desc = "KEY_BACKSPACE"; break;
			#endif
			#ifdef KEY_TAB
			case KEY_TAB : desc = "KEY_TAB"; break;
			#endif
			#ifdef KEY_Q
			case KEY_Q : desc = "KEY_Q"; break;
			#endif
			#ifdef KEY_W
			case KEY_W : desc = "KEY_W"; break;
			#endif
			#ifdef KEY_E
			case KEY_E : desc = "KEY_E"; break;
			#endif
			#ifdef KEY_R
			case KEY_R : desc = "KEY_R"; break;
			#endif
			#ifdef KEY_T
			case KEY_T : desc = "KEY_T"; break;
			#endif
			#ifdef KEY_Y
			case KEY_Y : desc = "KEY_Y"; break;
			#endif
			#ifdef KEY_U
			case KEY_U : desc = "KEY_U"; break;
			#endif
			#ifdef KEY_I
			case KEY_I : desc = "KEY_I"; break;
			#endif
			#ifdef KEY_O
			case KEY_O : desc = "KEY_O"; break;
			#endif
			#ifdef KEY_P
			case KEY_P : desc = "KEY_P"; break;
			#endif
			#ifdef KEY_LEFTBRACE
			case KEY_LEFTBRACE : desc = "KEY_LEFTBRACE"; break;
			#endif
			#ifdef KEY_RIGHTBRACE
			case KEY_RIGHTBRACE : desc = "KEY_RIGHTBRACE"; break;
			#endif
			#ifdef KEY_ENTER
			case KEY_ENTER : desc = "KEY_ENTER"; break;
			#endif
			#ifdef KEY_LEFTCTRL
			case KEY_LEFTCTRL : desc = "KEY_LEFTCTRL"; break;
			#endif
			#ifdef KEY_A
			case KEY_A : desc = "KEY_A"; break;
			#endif
			#ifdef KEY_S
			case KEY_S : desc = "KEY_S"; break;
			#endif
			#ifdef KEY_D
			case KEY_D : desc = "KEY_D"; break;
			#endif
			#ifdef KEY_F
			case KEY_F : desc = "KEY_F"; break;
			#endif
			#ifdef KEY_G
			case KEY_G : desc = "KEY_G"; break;
			#endif
			#ifdef KEY_H
			case KEY_H : desc = "KEY_H"; break;
			#endif
			#ifdef KEY_J
			case KEY_J : desc = "KEY_J"; break;
			#endif
			#ifdef KEY_K
			case KEY_K : desc = "KEY_K"; break;
			#endif
			#ifdef KEY_L
			case KEY_L : desc = "KEY_L"; break;
			#endif
			#ifdef KEY_SEMICOLON
			case KEY_SEMICOLON : desc = "KEY_SEMICOLON"; break;
			#endif
			#ifdef KEY_APOSTROPHE
			case KEY_APOSTROPHE : desc = "KEY_APOSTROPHE"; break;
			#endif
			#ifdef KEY_GRAVE
			case KEY_GRAVE : desc = "KEY_GRAVE"; break;
			#endif
			#ifdef KEY_LEFTSHIFT
			case KEY_LEFTSHIFT : desc = "KEY_LEFTSHIFT"; break;
			#endif
			#ifdef KEY_BACKSLASH
			case KEY_BACKSLASH : desc = "KEY_BACKSLASH"; break;
			#endif
			#ifdef KEY_Z
			case KEY_Z : desc = "KEY_Z"; break;
			#endif
			#ifdef KEY_X
			case KEY_X : desc = "KEY_X"; break;
			#endif
			#ifdef KEY_C
			case KEY_C : desc = "KEY_C"; break;
			#endif
			#ifdef KEY_V
			case KEY_V : desc = "KEY_V"; break;
			#endif
			#ifdef KEY_B
			case KEY_B : desc = "KEY_B"; break;
			#endif
			#ifdef KEY_N
			case KEY_N : desc = "KEY_N"; break;
			#endif
			#ifdef KEY_M
			case KEY_M : desc = "KEY_M"; break;
			#endif
			#ifdef KEY_COMMA
			case KEY_COMMA : desc = "KEY_COMMA"; break;
			#endif
			#ifdef KEY_DOT
			case KEY_DOT : desc = "KEY_DOT"; break;
			#endif
			#ifdef KEY_SLASH
			case KEY_SLASH : desc = "KEY_SLASH"; break;
			#endif
			#ifdef KEY_RIGHTSHIFT
			case KEY_RIGHTSHIFT : desc = "KEY_RIGHTSHIFT"; break;
			#endif
			#ifdef KEY_KPASTERISK
			case KEY_KPASTERISK : desc = "KEY_KPASTERISK"; break;
			#endif
			#ifdef KEY_LEFTALT
			case KEY_LEFTALT : desc = "KEY_LEFTALT"; break;
			#endif
			#ifdef KEY_SPACE
			case KEY_SPACE : desc = "KEY_SPACE"; break;
			#endif
			#ifdef KEY_CAPSLOCK
			case KEY_CAPSLOCK : desc = "KEY_CAPSLOCK"; break;
			#endif
			#ifdef KEY_F1
			case KEY_F1 : desc = "KEY_F1"; break;
			#endif
			#ifdef KEY_F2
			case KEY_F2 : desc = "KEY_F2"; break;
			#endif
			#ifdef KEY_F3
			case KEY_F3 : desc = "KEY_F3"; break;
			#endif
			#ifdef KEY_F4
			case KEY_F4 : desc = "KEY_F4"; break;
			#endif
			#ifdef KEY_F5
			case KEY_F5 : desc = "KEY_F5"; break;
			#endif
			#ifdef KEY_F6
			case KEY_F6 : desc = "KEY_F6"; break;
			#endif
			#ifdef KEY_F7
			case KEY_F7 : desc = "KEY_F7"; break;
			#endif
			#ifdef KEY_F8
			case KEY_F8 : desc = "KEY_F8"; break;
			#endif
			#ifdef KEY_F9
			case KEY_F9 : desc = "KEY_F9"; break;
			#endif
			#ifdef KEY_F10
			case KEY_F10 : desc = "KEY_F10"; break;
			#endif
			#ifdef KEY_NUMLOCK
			case KEY_NUMLOCK : desc = "KEY_NUMLOCK"; break;
			#endif
			#ifdef KEY_SCROLLLOCK
			case KEY_SCROLLLOCK : desc = "KEY_SCROLLLOCK"; break;
			#endif
			#ifdef KEY_KP7
			case KEY_KP7 : desc = "KEY_KP7"; break;
			#endif
			#ifdef KEY_KP8
			case KEY_KP8 : desc = "KEY_KP8"; break;
			#endif
			#ifdef KEY_KP9
			case KEY_KP9 : desc = "KEY_KP9"; break;
			#endif
			#ifdef KEY_KPMINUS
			case KEY_KPMINUS : desc = "KEY_KPMINUS"; break;
			#endif
			#ifdef KEY_KP4
			case KEY_KP4 : desc = "KEY_KP4"; break;
			#endif
			#ifdef KEY_KP5
			case KEY_KP5 : desc = "KEY_KP5"; break;
			#endif
			#ifdef KEY_KP6
			case KEY_KP6 : desc = "KEY_KP6"; break;
			#endif
			#ifdef KEY_KPPLUS
			case KEY_KPPLUS : desc = "KEY_KPPLUS"; break;
			#endif
			#ifdef KEY_KP1
			case KEY_KP1 : desc = "KEY_KP1"; break;
			#endif
			#ifdef KEY_KP2
			case KEY_KP2 : desc = "KEY_KP2"; break;
			#endif
			#ifdef KEY_KP3
			case KEY_KP3 : desc = "KEY_KP3"; break;
			#endif
			#ifdef KEY_KP0
			case KEY_KP0 : desc = "KEY_KP0"; break;
			#endif
			#ifdef KEY_KPDOT
			case KEY_KPDOT : desc = "KEY_KPDOT"; break;
			#endif
			#ifdef KEY_103RD
			case KEY_103RD : desc = "KEY_103RD"; break;
			#endif
			#ifdef KEY_F13
			case KEY_F13 : desc = "KEY_F13"; break;
			#endif
			#ifdef KEY_102ND
			case KEY_102ND : desc = "KEY_102ND"; break;
			#endif
			#ifdef KEY_F11
			case KEY_F11 : desc = "KEY_F11"; break;
			#endif
			#ifdef KEY_F12
			case KEY_F12 : desc = "KEY_F12"; break;
			#endif
			#ifdef KEY_F14
			case KEY_F14 : desc = "KEY_F14"; break;
			#endif
			#ifdef KEY_F15
			case KEY_F15 : desc = "KEY_F15"; break;
			#endif
			#ifdef KEY_F16
			case KEY_F16 : desc = "KEY_F16"; break;
			#endif
			#ifdef KEY_F17
			case KEY_F17 : desc = "KEY_F17"; break;
			#endif
			#ifdef KEY_F18
			case KEY_F18 : desc = "KEY_F18"; break;
			#endif
			#ifdef KEY_F19
			case KEY_F19 : desc = "KEY_F19"; break;
			#endif
			#ifdef KEY_F20
			case KEY_F20 : desc = "KEY_F20"; break;
			#endif
			#ifdef KEY_KPENTER
			case KEY_KPENTER : desc = "KEY_KPENTER"; break;
			#endif
			#ifdef KEY_RIGHTCTRL
			case KEY_RIGHTCTRL : desc = "KEY_RIGHTCTRL"; break;
			#endif
			#ifdef KEY_KPSLASH
			case KEY_KPSLASH : desc = "KEY_KPSLASH"; break;
			#endif
			#ifdef KEY_SYSRQ
			case KEY_SYSRQ : desc = "KEY_SYSRQ"; break;
			#endif
			#ifdef KEY_RIGHTALT
			case KEY_RIGHTALT : desc = "KEY_RIGHTALT"; break;
			#endif
			#ifdef KEY_LINEFEED
			case KEY_LINEFEED : desc = "KEY_LINEFEED"; break;
			#endif
			#ifdef KEY_HOME
			case KEY_HOME : desc = "KEY_HOME"; break;
			#endif
			#ifdef KEY_UP
			case KEY_UP : desc = "KEY_UP"; break;
			#endif
			#ifdef KEY_PAGEUP
			case KEY_PAGEUP : desc = "KEY_PAGEUP"; break;
			#endif
			#ifdef KEY_LEFT
			case KEY_LEFT : desc = "KEY_LEFT"; break;
			#endif
			#ifdef KEY_RIGHT
			case KEY_RIGHT : desc = "KEY_RIGHT"; break;
			#endif
			#ifdef KEY_END
			case KEY_END : desc = "KEY_END"; break;
			#endif
			#ifdef KEY_DOWN
			case KEY_DOWN : desc = "KEY_DOWN"; break;
			#endif
			#ifdef KEY_PAGEDOWN
			case KEY_PAGEDOWN : desc = "KEY_PAGEDOWN"; break;
			#endif
			#ifdef KEY_INSERT
			case KEY_INSERT : desc = "KEY_INSERT"; break;
			#endif
			#ifdef KEY_DELETE
			case KEY_DELETE : desc = "KEY_DELETE"; break;
			#endif
			#ifdef KEY_MACRO
			case KEY_MACRO : desc = "KEY_MACRO"; break;
			#endif
			#ifdef KEY_MUTE
			case KEY_MUTE : desc = "KEY_MUTE"; break;
			#endif
			#ifdef KEY_VOLUMEDOWN
			case KEY_VOLUMEDOWN : desc = "KEY_VOLUMEDOWN"; break;
			#endif
			#ifdef KEY_VOLUMEUP
			case KEY_VOLUMEUP : desc = "KEY_VOLUMEUP"; break;
			#endif
			#ifdef KEY_POWER
			case KEY_POWER : desc = "KEY_POWER"; break;
			#endif
			#ifdef KEY_KPEQUAL
			case KEY_KPEQUAL : desc = "KEY_KPEQUAL"; break;
			#endif
			#ifdef KEY_KPPLUSMINUS
			case KEY_KPPLUSMINUS : desc = "KEY_KPPLUSMINUS"; break;
			#endif
			#ifdef KEY_PAUSE
			case KEY_PAUSE : desc = "KEY_PAUSE"; break;
			#endif
			#ifdef KEY_F21
			case KEY_F21 : desc = "KEY_F21"; break;
			#endif
			#ifdef KEY_F22
			case KEY_F22 : desc = "KEY_F22"; break;
			#endif
			#ifdef KEY_F23
			case KEY_F23 : desc = "KEY_F23"; break;
			#endif
			#ifdef KEY_F24
			case KEY_F24 : desc = "KEY_F24"; break;
			#endif
			#ifdef KEY_KPCOMMA
			case KEY_KPCOMMA : desc = "KEY_KPCOMMA"; break;
			#endif
			#ifdef KEY_LEFTMETA
			case KEY_LEFTMETA : desc = "KEY_LEFTMETA"; break;
			#endif
			#ifdef KEY_RIGHTMETA
			case KEY_RIGHTMETA : desc = "KEY_RIGHTMETA"; break;
			#endif
			#ifdef KEY_COMPOSE
			case KEY_COMPOSE : desc = "KEY_COMPOSE"; break;
			#endif
			#ifdef KEY_STOP
			case KEY_STOP : desc = "KEY_STOP"; break;
			#endif
			#ifdef KEY_AGAIN
			case KEY_AGAIN : desc = "KEY_AGAIN"; break;
			#endif
			#ifdef KEY_PROPS
			case KEY_PROPS : desc = "KEY_PROPS"; break;
			#endif
			#ifdef KEY_UNDO
			case KEY_UNDO : desc = "KEY_UNDO"; break;
			#endif
			#ifdef KEY_FRONT
			case KEY_FRONT : desc = "KEY_FRONT"; break;
			#endif
			#ifdef KEY_COPY
			case KEY_COPY : desc = "KEY_COPY"; break;
			#endif
			#ifdef KEY_OPEN
			case KEY_OPEN : desc = "KEY_OPEN"; break;
			#endif
			#ifdef KEY_PASTE
			case KEY_PASTE : desc = "KEY_PASTE"; break;
			#endif
			#ifdef KEY_FIND
			case KEY_FIND : desc = "KEY_FIND"; break;
			#endif
			#ifdef KEY_CUT
			case KEY_CUT : desc = "KEY_CUT"; break;
			#endif
			#ifdef KEY_HELP
			case KEY_HELP : desc = "KEY_HELP"; break;
			#endif
			#ifdef KEY_MENU
			case KEY_MENU : desc = "KEY_MENU"; break;
			#endif
			#ifdef KEY_CALC
			case KEY_CALC : desc = "KEY_CALC"; break;
			#endif
			#ifdef KEY_SETUP
			case KEY_SETUP : desc = "KEY_SETUP"; break;
			#endif
			#ifdef KEY_SLEEP
			case KEY_SLEEP : desc = "KEY_SLEEP"; break;
			#endif
			#ifdef KEY_WAKEUP
			case KEY_WAKEUP : desc = "KEY_WAKEUP"; break;
			#endif
			#ifdef KEY_FILE
			case KEY_FILE : desc = "KEY_FILE"; break;
			#endif
			#ifdef KEY_SENDFILE
			case KEY_SENDFILE : desc = "KEY_SENDFILE"; break;
			#endif
			#ifdef KEY_DELETEFILE
			case KEY_DELETEFILE : desc = "KEY_DELETEFILE"; break;
			#endif
			#ifdef KEY_XFER
			case KEY_XFER : desc = "KEY_XFER"; break;
			#endif
			#ifdef KEY_PROG1
			case KEY_PROG1 : desc = "KEY_PROG1"; break;
			#endif
			#ifdef KEY_PROG2
			case KEY_PROG2 : desc = "KEY_PROG2"; break;
			#endif
			#ifdef KEY_WWW
			case KEY_WWW : desc = "KEY_WWW"; break;
			#endif
			#ifdef KEY_MSDOS
			case KEY_MSDOS : desc = "KEY_MSDOS"; break;
			#endif
			#ifdef KEY_COFFEE
			case KEY_COFFEE : desc = "KEY_COFFEE"; break;
			#endif
			#ifdef KEY_DIRECTION
			case KEY_DIRECTION : desc = "KEY_DIRECTION"; break;
			#endif
			#ifdef KEY_CYCLEWINDOWS
			case KEY_CYCLEWINDOWS : desc = "KEY_CYCLEWINDOWS"; break;
			#endif
			#ifdef KEY_MAIL
			case KEY_MAIL : desc = "KEY_MAIL"; break;
			#endif
			#ifdef KEY_BOOKMARKS
			case KEY_BOOKMARKS : desc = "KEY_BOOKMARKS"; break;
			#endif
			#ifdef KEY_COMPUTER
			case KEY_COMPUTER : desc = "KEY_COMPUTER"; break;
			#endif
			#ifdef KEY_BACK
			case KEY_BACK : desc = "KEY_BACK"; break;
			#endif
			#ifdef KEY_FORWARD
			case KEY_FORWARD : desc = "KEY_FORWARD"; break;
			#endif
			#ifdef KEY_CLOSECD
			case KEY_CLOSECD : desc = "KEY_CLOSECD"; break;
			#endif
			#ifdef KEY_EJECTCD
			case KEY_EJECTCD : desc = "KEY_EJECTCD"; break;
			#endif
			#ifdef KEY_EJECTCLOSECD
			case KEY_EJECTCLOSECD : desc = "KEY_EJECTCLOSECD"; break;
			#endif
			#ifdef KEY_NEXTSONG
			case KEY_NEXTSONG : desc = "KEY_NEXTSONG"; break;
			#endif
			#ifdef KEY_PLAYPAUSE
			case KEY_PLAYPAUSE : desc = "KEY_PLAYPAUSE"; break;
			#endif
			#ifdef KEY_PREVIOUSSONG
			case KEY_PREVIOUSSONG : desc = "KEY_PREVIOUSSONG"; break;
			#endif
			#ifdef KEY_STOPCD
			case KEY_STOPCD : desc = "KEY_STOPCD"; break;
			#endif
			#ifdef KEY_RECORD
			case KEY_RECORD : desc = "KEY_RECORD"; break;
			#endif
			#ifdef KEY_REWIND
			case KEY_REWIND : desc = "KEY_REWIND"; break;
			#endif
			#ifdef KEY_PHONE
			case KEY_PHONE : desc = "KEY_PHONE"; break;
			#endif
			#ifdef KEY_ISO
			case KEY_ISO : desc = "KEY_ISO"; break;
			#endif
			#ifdef KEY_CONFIG
			case KEY_CONFIG : desc = "KEY_CONFIG"; break;
			#endif
			#ifdef KEY_HOMEPAGE
			case KEY_HOMEPAGE : desc = "KEY_HOMEPAGE"; break;
			#endif
			#ifdef KEY_REFRESH
			case KEY_REFRESH : desc = "KEY_REFRESH"; break;
			#endif
			#ifdef KEY_EXIT
			case KEY_EXIT : desc = "KEY_EXIT"; break;
			#endif
			#ifdef KEY_MOVE
			case KEY_MOVE : desc = "KEY_MOVE"; break;
			#endif
			#ifdef KEY_EDIT
			case KEY_EDIT : desc = "KEY_EDIT"; break;
			#endif
			#ifdef KEY_SCROLLUP
			case KEY_SCROLLUP : desc = "KEY_SCROLLUP"; break;
			#endif
			#ifdef KEY_SCROLLDOWN
			case KEY_SCROLLDOWN : desc = "KEY_SCROLLDOWN"; break;
			#endif
			#ifdef KEY_KPLEFTPAREN
			case KEY_KPLEFTPAREN : desc = "KEY_KPLEFTPAREN"; break;
			#endif
			#ifdef KEY_KPRIGHTPAREN
			case KEY_KPRIGHTPAREN : desc = "KEY_KPRIGHTPAREN"; break;
			#endif
			#ifdef KEY_INTL1
			case KEY_INTL1 : desc = "KEY_INTL1"; break;
			#endif
			#ifdef KEY_INTL2
			case KEY_INTL2 : desc = "KEY_INTL2"; break;
			#endif
			#ifdef KEY_INTL3
			case KEY_INTL3 : desc = "KEY_INTL3"; break;
			#endif
			#ifdef KEY_INTL4
			case KEY_INTL4 : desc = "KEY_INTL4"; break;
			#endif
			#ifdef KEY_INTL5
			case KEY_INTL5 : desc = "KEY_INTL5"; break;
			#endif
			#ifdef KEY_INTL6
			case KEY_INTL6 : desc = "KEY_INTL6"; break;
			#endif
			#ifdef KEY_INTL7
			case KEY_INTL7 : desc = "KEY_INTL7"; break;
			#endif
			#ifdef KEY_INTL8
			case KEY_INTL8 : desc = "KEY_INTL8"; break;
			#endif
			#ifdef KEY_INTL9
			case KEY_INTL9 : desc = "KEY_INTL9"; break;
			#endif
			#ifdef KEY_LANG1
			case KEY_LANG1 : desc = "KEY_LANG1"; break;
			#endif
			#ifdef KEY_LANG2
			case KEY_LANG2 : desc = "KEY_LANG2"; break;
			#endif
			#ifdef KEY_LANG3
			case KEY_LANG3 : desc = "KEY_LANG3"; break;
			#endif
			#ifdef KEY_LANG4
			case KEY_LANG4 : desc = "KEY_LANG4"; break;
			#endif
			#ifdef KEY_LANG5
			case KEY_LANG5 : desc = "KEY_LANG5"; break;
			#endif
			#ifdef KEY_LANG6
			case KEY_LANG6 : desc = "KEY_LANG6"; break;
			#endif
			#ifdef KEY_LANG7
			case KEY_LANG7 : desc = "KEY_LANG7"; break;
			#endif
			#ifdef KEY_LANG8
			case KEY_LANG8 : desc = "KEY_LANG8"; break;
			#endif
			#ifdef KEY_LANG9
			case KEY_LANG9 : desc = "KEY_LANG9"; break;
			#endif
			#ifdef KEY_PLAYCD
			case KEY_PLAYCD : desc = "KEY_PLAYCD"; break;
			#endif
			#ifdef KEY_PAUSECD
			case KEY_PAUSECD : desc = "KEY_PAUSECD"; break;
			#endif
			#ifdef KEY_PROG3
			case KEY_PROG3 : desc = "KEY_PROG3"; break;
			#endif
			#ifdef KEY_PROG4
			case KEY_PROG4 : desc = "KEY_PROG4"; break;
			#endif
			#ifdef KEY_SUSPEND
			case KEY_SUSPEND : desc = "KEY_SUSPEND"; break;
			#endif
			#ifdef KEY_CLOSE
			case KEY_CLOSE : desc = "KEY_CLOSE"; break;
			#endif
			#ifdef KEY_UNKNOWN
			case KEY_UNKNOWN : desc = "KEY_UNKNOWN"; break;
			#endif
			#ifdef KEY_BRIGHTNESSDOWN
			case KEY_BRIGHTNESSDOWN : desc = "KEY_BRIGHTNESSDOWN"; break;
			#endif
			#ifdef KEY_BRIGHTNESSUP
			case KEY_BRIGHTNESSUP : desc = "KEY_BRIGHTNESSUP"; break;
			#endif
			#ifdef BTN_0
			case BTN_0 : desc = "BTN_0"; break;
			#endif
			#ifdef BTN_1
			case BTN_1 : desc = "BTN_1"; break;
			#endif
			#ifdef BTN_2
			case BTN_2 : desc = "BTN_2"; break;
			#endif
			#ifdef BTN_3
			case BTN_3 : desc = "BTN_3"; break;
			#endif
			#ifdef BTN_4
			case BTN_4 : desc = "BTN_4"; break;
			#endif
			#ifdef BTN_5
			case BTN_5 : desc = "BTN_5"; break;
			#endif
			#ifdef BTN_6
			case BTN_6 : desc = "BTN_6"; break;
			#endif
			#ifdef BTN_7
			case BTN_7 : desc = "BTN_7"; break;
			#endif
			#ifdef BTN_8
			case BTN_8 : desc = "BTN_8"; break;
			#endif
			#ifdef BTN_9
			case BTN_9 : desc = "BTN_9"; break;
			#endif
			#ifdef BTN_LEFT
			case BTN_LEFT : desc = "BTN_LEFT"; break;
			#endif
			#ifdef BTN_RIGHT
			case BTN_RIGHT : desc = "BTN_RIGHT"; break;
			#endif
			#ifdef BTN_MIDDLE
			case BTN_MIDDLE : desc = "BTN_MIDDLE"; break;
			#endif
			#ifdef BTN_SIDE
			case BTN_SIDE : desc = "BTN_SIDE"; break;
			#endif
			#ifdef BTN_EXTRA
			case BTN_EXTRA : desc = "BTN_EXTRA"; break;
			#endif
			#ifdef BTN_FORWARD
			case BTN_FORWARD : desc = "BTN_FORWARD"; break;
			#endif
			#ifdef BTN_BACK
			case BTN_BACK : desc = "BTN_BACK"; break;
			#endif
			#ifdef BTN_TRIGGER
			case BTN_TRIGGER : desc = "BTN_TRIGGER"; break;
			#endif
			#ifdef BTN_THUMB
			case BTN_THUMB : desc = "BTN_THUMB"; break;
			#endif
			#ifdef BTN_THUMB2
			case BTN_THUMB2 : desc = "BTN_THUMB2"; break;
			#endif
			#ifdef BTN_TOP
			case BTN_TOP : desc = "BTN_TOP"; break;
			#endif
			#ifdef BTN_TOP2
			case BTN_TOP2 : desc = "BTN_TOP2"; break;
			#endif
			#ifdef BTN_PINKIE
			case BTN_PINKIE : desc = "BTN_PINKIE"; break;
			#endif
			#ifdef BTN_BASE
			case BTN_BASE : desc = "BTN_BASE"; break;
			#endif
			#ifdef BTN_BASE2
			case BTN_BASE2 : desc = "BTN_BASE2"; break;
			#endif
			#ifdef BTN_BASE3
			case BTN_BASE3 : desc = "BTN_BASE3"; break;
			#endif
			#ifdef BTN_BASE4
			case BTN_BASE4 : desc = "BTN_BASE4"; break;
			#endif
			#ifdef BTN_BASE5
			case BTN_BASE5 : desc = "BTN_BASE5"; break;
			#endif
			#ifdef BTN_BASE6
			case BTN_BASE6 : desc = "BTN_BASE6"; break;
			#endif
			#ifdef BTN_DEAD
			case BTN_DEAD : desc = "BTN_DEAD"; break;
			#endif
			#ifdef BTN_A
			case BTN_A : desc = "BTN_A"; break;
			#endif
			#ifdef BTN_B
			case BTN_B : desc = "BTN_B"; break;
			#endif
			#ifdef BTN_C
			case BTN_C : desc = "BTN_C"; break;
			#endif
			#ifdef BTN_X
			case BTN_X : desc = "BTN_X"; break;
			#endif
			#ifdef BTN_Y
			case BTN_Y : desc = "BTN_Y"; break;
			#endif
			#ifdef BTN_Z
			case BTN_Z : desc = "BTN_Z"; break;
			#endif
			#ifdef BTN_TL
			case BTN_TL : desc = "BTN_TL"; break;
			#endif
			#ifdef BTN_TR
			case BTN_TR : desc = "BTN_TR"; break;
			#endif
			#ifdef BTN_TL2
			case BTN_TL2 : desc = "BTN_TL2"; break;
			#endif
			#ifdef BTN_TR2
			case BTN_TR2 : desc = "BTN_TR2"; break;
			#endif
			#ifdef BTN_SELECT
			case BTN_SELECT : desc = "BTN_SELECT"; break;
			#endif
			#ifdef BTN_START
			case BTN_START : desc = "BTN_START"; break;
			#endif
			#ifdef BTN_MODE
			case BTN_MODE : desc = "BTN_MODE"; break;
			#endif
			#ifdef BTN_THUMBL
			case BTN_THUMBL : desc = "BTN_THUMBL"; break;
			#endif
			#ifdef BTN_THUMBR
			case BTN_THUMBR : desc = "BTN_THUMBR"; break;
			#endif
			#ifdef BTN_TOOL_PEN
			case BTN_TOOL_PEN : desc = "BTN_TOOL_PEN"; break;
			#endif
			#ifdef BTN_TOOL_RUBBER
			case BTN_TOOL_RUBBER : desc = "BTN_TOOL_RUBBER"; break;
			#endif
			#ifdef BTN_TOOL_BRUSH
			case BTN_TOOL_BRUSH : desc = "BTN_TOOL_BRUSH"; break;
			#endif
			#ifdef BTN_TOOL_PENCIL
			case BTN_TOOL_PENCIL : desc = "BTN_TOOL_PENCIL"; break;
			#endif
			#ifdef BTN_TOOL_AIRBRUSH
			case BTN_TOOL_AIRBRUSH : desc = "BTN_TOOL_AIRBRUSH"; break;
			#endif
			#ifdef BTN_TOOL_FINGER
			case BTN_TOOL_FINGER : desc = "BTN_TOOL_FINGER"; break;
			#endif
			#ifdef BTN_TOOL_MOUSE
			case BTN_TOOL_MOUSE : desc = "BTN_TOOL_MOUSE"; break;
			#endif
			#ifdef BTN_TOOL_LENS
			case BTN_TOOL_LENS : desc = "BTN_TOOL_LENS"; break;
			#endif
			#ifdef BTN_TOUCH
			case BTN_TOUCH : desc = "BTN_TOUCH"; break;
			#endif
			#ifdef BTN_STYLUS
			case BTN_STYLUS : desc = "BTN_STYLUS"; break;
			#endif
			#ifdef BTN_STYLUS2
			case BTN_STYLUS2 : desc = "BTN_STYLUS2"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}

static void event_rel_log(int f)
{
	unsigned char rel_bitmask[REL_MAX/8 + 1];
	unsigned i;

	memset(rel_bitmask, 0, sizeof(rel_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_REL, sizeof(rel_bitmask)), rel_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_REL,%d))\n", (int)REL_MAX));
		return;
	}

	log_std(("event: EV_REL:"));

	for(i=0;i<REL_MAX;++i) {
		if (event_test_bit(i, rel_bitmask)) {
			const char* desc;
			switch (i) {
			#ifdef REL_X
			case REL_X : desc = "REL_X"; break;
			#endif
			#ifdef REL_Y
			case REL_Y : desc = "REL_Y"; break;
			#endif
			#ifdef REL_Z
			case REL_Z : desc = "REL_Z"; break;
			#endif
			#ifdef REL_HWHEEL
			case REL_HWHEEL : desc = "REL_HWHEEL"; break;
			#endif
			#ifdef REL_DIAL
			case REL_DIAL : desc = "REL_DIAL"; break;
			#endif
			#ifdef REL_WHEEL
			case REL_WHEEL : desc = "REL_WHEEL"; break;
			#endif
			#ifdef REL_MISC
			case REL_MISC : desc = "REL_MISC"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}

static void event_abs_log(int f)
{
	unsigned char abs_bitmask[ABS_MAX/8 + 1];
	unsigned i;

	memset(abs_bitmask, 0, sizeof(abs_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_ABS, sizeof(abs_bitmask)), abs_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_ABS,%d))\n", (int)ABS_MAX));
		return;
	}

	log_std(("event: EV_ABS:"));

	for(i=0;i<ABS_MAX;++i) {
		if (event_test_bit(i, abs_bitmask)) {
			const char* desc;
			int features[5];
			switch (i) {
			#ifdef ABS_X
			case ABS_X : desc = "ABS_X"; break;
			#endif
			#ifdef ABS_Y
			case ABS_Y : desc = "ABS_Y"; break;
			#endif
			#ifdef ABS_Z
			case ABS_Z : desc = "ABS_Z"; break;
			#endif
			#ifdef ABS_RX
			case ABS_RX : desc = "ABS_RX"; break;
			#endif
			#ifdef ABS_RY
			case ABS_RY : desc = "ABS_RY"; break;
			#endif
			#ifdef ABS_RZ
			case ABS_RZ : desc = "ABS_RZ"; break;
			#endif
			#ifdef ABS_THROTTLE
			case ABS_THROTTLE : desc = "ABS_THROTTLE"; break;
			#endif
			#ifdef ABS_RUDDER
			case ABS_RUDDER : desc = "ABS_RUDDER"; break;
			#endif
			#ifdef ABS_WHEEL
			case ABS_WHEEL : desc = "ABS_WHEEL"; break;
			#endif
			#ifdef ABS_GAS
			case ABS_GAS : desc = "ABS_GAS"; break;
			#endif
			#ifdef ABS_BRAKE
			case ABS_BRAKE : desc = "ABS_BRAKE"; break;
			#endif
			#ifdef ABS_HAT0X
			case ABS_HAT0X : desc = "ABS_HAT0X"; break;
			#endif
			#ifdef ABS_HAT0Y
			case ABS_HAT0Y : desc = "ABS_HAT0Y"; break;
			#endif
			#ifdef ABS_HAT1X
			case ABS_HAT1X : desc = "ABS_HAT1X"; break;
			#endif
			#ifdef ABS_HAT1Y
			case ABS_HAT1Y : desc = "ABS_HAT1Y"; break;
			#endif
			#ifdef ABS_HAT2X
			case ABS_HAT2X : desc = "ABS_HAT2X"; break;
			#endif
			#ifdef ABS_HAT2Y
			case ABS_HAT2Y : desc = "ABS_HAT2Y"; break;
			#endif
			#ifdef ABS_HAT3X
			case ABS_HAT3X : desc = "ABS_HAT3X"; break;
			#endif
			#ifdef ABS_HAT3Y
			case ABS_HAT3Y : desc = "ABS_HAT3Y"; break;
			#endif
			#ifdef ABS_PRESSURE
			case ABS_PRESSURE : desc = "ABS_PRESSURE"; break;
			#endif
			#ifdef ABS_DISTANCE
			case ABS_DISTANCE : desc = "ABS_DISTANCE"; break;
			#endif
			#ifdef ABS_TILT_X
			case ABS_TILT_X : desc = "ABS_TILT_X"; break;
			#endif
			#ifdef ABS_TILT_Y
			case ABS_TILT_Y : desc = "ABS_TILT_Y"; break;
			#endif
			#ifdef ABS_MISC
			case ABS_MISC : desc = "ABS_MISC"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));

			memset(features, 0, sizeof(features));
			if (ioctl(f, EVIOCGABS(i), features) >= 0) {
				log_std(("[val:%d,min:%d,max:%d,fuzz:%d,flat:%d]", features[0], features[1], features[2], features[3], features[4]));
			}
		}
	}

	log_std(("\n"));
}

#ifdef EV_MSC
static void event_msc_log(int f)
{
	unsigned char msc_bitmask[MSC_MAX/8 + 1];
	unsigned i;

	memset(msc_bitmask, 0, sizeof(msc_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_MSC, sizeof(msc_bitmask)), msc_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_MSC,%d))\n", (int)MSC_MAX));
		return;
	}

	log_std(("event: EV_MSC:"));

	for(i=0;i<MSC_MAX;++i) {
		if (event_test_bit(i, msc_bitmask)) {
			const char* desc;
			switch (i) {
			#ifdef MSC_SERIAL
			case MSC_SERIAL : desc = "MSC_SERIAL"; break;
			#endif
			#ifdef MSC_PULSELED
			case MSC_PULSELED : desc = "MSC_PULSELED"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}
#endif

static void event_led_log(int f)
{
	unsigned char led_bitmask[LED_MAX/8 + 1];
	unsigned i;

	memset(led_bitmask, 0, sizeof(led_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_LED, sizeof(led_bitmask)), led_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_LED,%d))\n", (int)LED_MAX));
		return;
	}

	log_std(("event: EV_LED:"));

	for(i=0;i<LED_MAX;++i) {
		if (event_test_bit(i, led_bitmask)) {
			const char* desc;
			switch (i) {
			#ifdef LED_NUML
			case LED_NUML : desc = "LED_NUML"; break;
			#endif
			#ifdef LED_CAPSL
			case LED_CAPSL : desc = "LED_CAPSL"; break;
			#endif
			#ifdef LED_SCROLLL
			case LED_SCROLLL : desc = "LED_SCROLLL"; break;
			#endif
			#ifdef LED_COMPOSE
			case LED_COMPOSE : desc = "LED_COMPOSE"; break;
			#endif
			#ifdef LED_KANA
			case LED_KANA : desc = "LED_KANA"; break;
			#endif
			#ifdef LED_SLEEP
			case LED_SLEEP : desc = "LED_SLEEP"; break;
			#endif
			#ifdef LED_SUSPEND
			case LED_SUSPEND : desc = "LED_SUSPEND"; break;
			#endif
			#ifdef LED_MUTE
			case LED_MUTE : desc = "LED_MUTE"; break;
			#endif
			#ifdef LED_MISC
			case LED_MISC : desc = "LED_MISC"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}

static void event_snd_log(int f)
{
	unsigned char snd_bitmask[SND_MAX/8 + 1];
	unsigned i;

	memset(snd_bitmask, 0, sizeof(snd_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_SND, sizeof(snd_bitmask)), snd_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_SND,%d))\n", (int)SND_MAX));
		return;
	}

	log_std(("event: EV_SND:"));

	for(i=0;i<SND_MAX;++i) {
		if (event_test_bit(i, snd_bitmask)) {
			const char* desc;
			switch (i) {
			#ifdef SND_CLICK
			case SND_CLICK : desc = "SND_CLICK"; break;
			#endif
			#ifdef SND_BELL
			case SND_BELL : desc = "SND_BELL"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}

static void event_rep_log(int f)
{
	unsigned char rep_bitmask[REP_MAX/8 + 1];
	unsigned i;

	memset(rep_bitmask, 0, sizeof(rep_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_REP, sizeof(rep_bitmask)), rep_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_REP,%d))\n", (int)REP_MAX));
		return;
	}

	log_std(("event: EV_REP:"));

	for(i=0;i<REP_MAX;++i) {
		if (event_test_bit(i, rep_bitmask)) {
			const char* desc;
			switch (i) {
			#ifdef REP_DELAY
			case REP_DELAY : desc = "REP_DELAY"; break;
			#endif
			#ifdef REP_PERIOD
			case REP_PERIOD : desc = "REP_PERIOD"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}

#ifdef EV_FF
static void event_ff_log(int f)
{
	unsigned char ff_bitmask[FF_MAX/8 + 1];
	unsigned i;

	memset(ff_bitmask, 0, sizeof(ff_bitmask));
	if (ioctl(f, EVIOCGBIT(EV_FF, sizeof(ff_bitmask)), ff_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(EV_FF,%d))\n", (int)FF_MAX));
		return;
	}

	log_std(("event: EV_FF:"));

	for(i=0;i<FF_MAX;++i) {
		if (event_test_bit(i, ff_bitmask)) {
			const char* desc;
			switch (i) {
			#ifdef FF_BTN
			#ifdef BTN_TRIGGER
			case FF_BTN(BTN_TRIGGER) : desc = "FF_BTN(BTN_TRIGGER)"; break;
			#endif
			#ifdef BTN_THUMB
			case FF_BTN(BTN_THUMB) : desc = "FF_BTN(BTN_THUMB)"; break;
			#endif
			#ifdef BTN_THUMB2
			case FF_BTN(BTN_THUMB2) : desc = "FF_BTN(BTN_THUMB2)"; break;
			#endif
			#ifdef BTN_TOP
			case FF_BTN(BTN_TOP) : desc = "FF_BTN(BTN_TOP)"; break;
			#endif
			#ifdef BTN_TOP2
			case FF_BTN(BTN_TOP2) : desc = "FF_BTN(BTN_TOP2)"; break;
			#endif
			#ifdef BTN_PINKIE
			case FF_BTN(BTN_PINKIE) : desc = "FF_BTN(BTN_PINKIE)"; break;
			#endif
			#ifdef BTN_BASE
			case FF_BTN(BTN_BASE) : desc = "FF_BTN(BTN_BASE)"; break;
			#endif
			#ifdef BTN_BASE2
			case FF_BTN(BTN_BASE2) : desc = "FF_BTN(BTN_BASE2)"; break;
			#endif
			#ifdef BTN_BASE3
			case FF_BTN(BTN_BASE3) : desc = "FF_BTN(BTN_BASE3)"; break;
			#endif
			#ifdef BTN_BASE4
			case FF_BTN(BTN_BASE4) : desc = "FF_BTN(BTN_BASE4)"; break;
			#endif
			#ifdef BTN_BASE5
			case FF_BTN(BTN_BASE5) : desc = "FF_BTN(BTN_BASE5)"; break;
			#endif
			#ifdef BTN_BASE6
			case FF_BTN(BTN_BASE6) : desc = "FF_BTN(BTN_BASE6)"; break;
			#endif
			#ifdef BTN_DEAD
			case FF_BTN(BTN_DEAD) : desc = "FF_BTN(BTN_DEAD)"; break;
			#endif
			#endif
			#ifdef FF_ABS
			#ifdef ABS_X
			case FF_ABS(ABS_X) : desc = "FF_ABS(ABS_X)"; break;
			#endif
			#ifdef ABS_Y
			case FF_ABS(ABS_Y) : desc = "FF_ABS(ABS_Y)"; break;
			#endif
			#ifdef ABS_Z
			case FF_ABS(ABS_Z) : desc = "FF_ABS(ABS_Z)"; break;
			#endif
			#endif
			#ifdef FF_RUMBLE
			case FF_RUMBLE : desc = "FF_RUMBLE"; break;
			#endif
			#ifdef FF_PERIODIC
			case FF_PERIODIC : desc = "FF_PERIODIC"; break;
			#endif
			#ifdef FF_CONSTANT
			case FF_CONSTANT : desc = "FF_CONSTANT"; break;
			#endif
			#ifdef FF_SPRING
			case FF_SPRING : desc = "FF_SPRING"; break;
			#endif
			#ifdef FF_FRICTION
			case FF_FRICTION : desc = "FF_FRICTION"; break;
			#endif
			#ifdef FF_SQUARE
			case FF_SQUARE : desc = "FF_SQUARE"; break;
			#endif
			#ifdef FF_TRIANGLE
			case FF_TRIANGLE : desc = "FF_TRIANGLE"; break;
			#endif
			#ifdef FF_SINE
			case FF_SINE : desc = "FF_SINE"; break;
			#endif
			#ifdef FF_SAW_UP
			case FF_SAW_UP : desc = "FF_SAW_UP"; break;
			#endif
			#ifdef FF_SAW_DOWN
			case FF_SAW_DOWN : desc = "FF_SAW_DOWN"; break;
			#endif
			#ifdef FF_CUSTOM
			case FF_CUSTOM : desc = "FF_CUSTOM"; break;
			#endif
			#ifdef FF_GAIN
			case FF_GAIN : desc = "FF_GAIN"; break;
			#endif
			#ifdef FF_AUTOCENTER
			case FF_AUTOCENTER : desc = "FF_AUTOCENTER"; break;
			#endif
			default : desc = 0; break;
			}
			if (desc)
				log_std((" %s", desc));
			else
				log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}
#endif

#define UK_MAX 256

static void event_unknown_log(int f, unsigned e)
{
	unsigned char unk_bitmask[UK_MAX / 8];
	unsigned i;

	memset(unk_bitmask, 0, sizeof(unk_bitmask));
	if (ioctl(f, EVIOCGBIT(e, sizeof(unk_bitmask)), unk_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(0x%x,%d))\n", e, (int)UK_MAX));
		return;
	}

	log_std(("event: EV_0x%x:", e));

	for(i=0;i<UK_MAX;++i) {
		if (event_test_bit(i, unk_bitmask)) {
			log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}

int event_open(const char* file, unsigned char* evtype_bitmask, unsigned evtype_size)
{
	int f;

	f = open(file, O_RDONLY | O_NONBLOCK);
	if (f == -1) {
		if (errno != ENODEV) {
			log_std(("event: error opening device %s, errno %d (%s)\n", file, errno, strerror(errno)));
		}
		goto err;
	}

	log_std(("event: open device %s\n", file));

	if (evtype_bitmask) {
		memset(evtype_bitmask, 0, evtype_size);
		if (ioctl(f, EVIOCGBIT(0, EV_MAX), evtype_bitmask) < 0) {
			log_std(("event: error in ioctl(EVIOCGBIT(0,%d)) on device %s\n", (int)EV_MAX, file));
			goto err_close;
		}
	}

	return f;

err_close:
	close(f);
err:
	return -1;
}

void event_close(int f)
{
	close(f);
}

void event_log(int f, unsigned char* evtype_bitmask)
{
	int version;
	unsigned short device_info[4];
	const char* bus;
	char name[256];
	unsigned i;

	version = 0;
	if (ioctl(f, EVIOCGVERSION, &version)) {
		log_std(("event: error in ioctl(EVIOCGVERSION)\n"));
	} else {
		log_std(("event: driver:%d.%d.%d\n", version >> 16, (version >> 8) & 0xff, version & 0xff));
	}

	if (ioctl(f, EVIOCGID, &device_info)) {
		log_std(("event: error in ioctl(EVIOCGID)\n"));
	} else {
		switch (device_info[ID_BUS]) {
		#ifdef BUS_PCI
		case BUS_PCI : bus = "pci"; break;
		#endif
		#ifdef BUS_ISAPNP
		case BUS_ISAPNP : bus = "isapnp"; break;
		#endif
		#ifdef BUS_USB
		case BUS_USB : bus = "usb"; break;
		#endif
		#ifdef BUS_ISA
		case BUS_ISA : bus = "usa"; break;
		#endif
		#ifdef BUS_I8042
		case BUS_I8042 : bus = "i8042"; break;
		#endif
		#ifdef BUS_RS232
		case BUS_RS232 : bus = "rs232"; break;
		#endif
		#ifdef BUS_GAMEPORT
		case BUS_GAMEPORT : bus = "gameport"; break;
		#endif
		#ifdef BUS_PARPORT
		case BUS_PARPORT : bus = "parport"; break;
		#endif
		#ifdef BUS_AMIGA
		case BUS_AMIGA : bus = "amiga"; break;
		#endif
		#ifdef BUS_ADB
		case BUS_ADB : bus = "adb"; break;
		#endif
		#ifdef BUS_I2C
		case BUS_I2C : bus = "i2c"; break;
		#endif
		default: bus = "unknown"; break;
		}

		log_std(("event: device vendor:0x%04hx product:0x%04hx version:0x%04hx bus:0x%04hx (%s)\n",
			(unsigned)device_info[ID_VENDOR],
			(unsigned)device_info[ID_PRODUCT],
			(unsigned)device_info[ID_VERSION],
			(unsigned)device_info[ID_BUS],
			bus
		));
	}

	if (ioctl(f, EVIOCGNAME(sizeof(name)), name) < 0) {
		log_std(("event: error in ioctl(EVIOCGNAME)\n"));
	} else {
		log_std(("event: name:\"%s\"\n", name));
	}

	for(i=0;i<EV_MAX;++i) {
		if (event_test_bit(i, evtype_bitmask)) {
			switch (i) {
			case EV_KEY : event_key_log(f); break;
			case EV_REL : event_rel_log(f); break;
			case EV_ABS : event_abs_log(f); break;
#ifdef EV_MSC
			case EV_MSC : event_msc_log(f); break;
#endif
			case EV_LED : event_led_log(f); break;
			case EV_SND : event_snd_log(f); break;
			case EV_REP : event_rep_log(f); break;
#ifdef EV_FF
			case EV_FF : event_ff_log(f); break;
#endif
			default: event_unknown_log(f, i); break;
			}
		}
	}
}

static adv_bool event_test_bit_feature(int f, unsigned bit, unsigned char* evtype_bitmask, const int* feature)
{
	unsigned i;
	unsigned char bitmask[64];
	unsigned bitmax = 64 * 8;

	if (!event_test_bit(bit, evtype_bitmask))
		return 0;

	memset(bitmask, 0, sizeof(bitmask));

	if (ioctl(f, EVIOCGBIT(bit, sizeof(bitmask)), bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(0x%x))\n", bit));
		return 0;
	}

	for(i=0;feature[i] >= 0;++i)
		if (event_test_bit(feature[i], bitmask)) {
			return 1;
	}

	return 0;
}

int ABS_FEATURE[] = {
#ifdef ABS_X
	ABS_X,
#endif
#ifdef ABS_Y
	ABS_Y,
#endif
#ifdef ABS_Z
	ABS_Z,
#endif
#ifdef ABS_RX
	ABS_RX,
#endif
#ifdef ABS_RY
	ABS_RY,
#endif
#ifdef ABS_RZ
	ABS_RZ,
#endif
#ifdef ABS_GAS
	ABS_GAS,
#endif
#ifdef ABS_BRAKE
	ABS_BRAKE,
#endif
#ifdef ABS_WHELL
	ABS_WHEEL,
#endif
#ifdef ABS_HAT0X
	ABS_HAT0X,
	ABS_HAT0Y,
#endif
#ifdef ABS_HAT1X
	ABS_HAT1X,
	ABS_HAT1Y,
#endif
#ifdef ABS_HAT2X
	ABS_HAT2X,
	ABS_HAT2Y,
#endif
#ifdef ABS_HAT3X
	ABS_HAT3X,
	ABS_HAT3Y,
#endif
#ifdef ABS_THROTTLE
	ABS_THROTTLE,
#endif
#ifdef ABS_RUDDER
	ABS_RUDDER,
#endif
/* ABS_MISC is used by some mouses */
	-1
};

int REL_FEATURE[] = {
#ifdef REL_X
	REL_X,
#endif
#ifdef REL_Y
	REL_Y,
#endif
#ifdef REL_Z
	REL_Z,
#endif
#ifdef REL_WHEEL
	REL_WHEEL,
#endif
#ifdef REL_HWHEEL
	REL_HWHEEL,
#endif
#ifdef REL_DIAL
	REL_DIAL,
#endif
/* REL_MISC is too generic */
	-1
};

int KEY_FEATURE[] = {
#ifdef KEY_1
	KEY_1,
#endif
#ifdef KEY_A
	KEY_A,
#endif
	-1
};

adv_bool event_is_joystick(int f, unsigned char* evtype_bitmask)
{
	return event_test_bit_feature(f, EV_ABS, evtype_bitmask, ABS_FEATURE);
}

adv_bool event_is_mouse(int f, unsigned char* evtype_bitmask)
{
	return !event_test_bit_feature(f, EV_ABS, evtype_bitmask, ABS_FEATURE)
		&& event_test_bit_feature(f, EV_REL, evtype_bitmask, REL_FEATURE);
}

adv_bool event_is_keyboard(int f, unsigned char* evtype_bitmask)
{
	return !event_test_bit_feature(f, EV_ABS, evtype_bitmask, ABS_FEATURE)
		&& !event_test_bit_feature(f, EV_REL, evtype_bitmask, REL_FEATURE)
		&& event_test_bit_feature(f, EV_KEY, evtype_bitmask, KEY_FEATURE);
}

adv_error event_read(int f, int* type, int* code, int* value)
{
	int size;
	struct input_event e;

	size = read(f, &e, sizeof(e));

	if (size == -1 && errno == EAGAIN) {
		/* normal exit if data is missing */
		return -1;
	}

	if (size != sizeof(e)) {
		log_std(("ERROR:event: invalid read size %d on the event interface, errno %d (%s)\n", size, errno, strerror(errno)));
		return -1;
	}

	log_debug(("event: read time %ld.%06ld, type %d, code %d, value %d\n", e.time.tv_sec, e.time.tv_usec, e.type, e.code, e.value));

	*type = e.type;
	*code = e.code;
	*value = e.value;

	return 0;
}

adv_error event_write(int f, int type, int code, int value)
{
	int size;
	struct input_event e;

	e.type = type;
	e.code = code;
	e.value = value;

	size = write(f, &e, sizeof(e));

	if (size != sizeof(e)) {
		log_std(("ERROR:event: invalid write size %d on the event interface, errno %d (%s)\n", size, errno, strerror(errno)));
		return -1;
	}

	log_debug(("event: write type %d, code %d, value %d\n", e.type, e.code, e.value));

	return 0;
}

int event_compare(const void* void_a, const void* void_b)
{
	const struct event_location* a = (const struct event_location*)void_a;
	const struct event_location* b = (const struct event_location*)void_b;

	if (a->vendor < b->vendor)
		return -1;
	if (a->vendor > b->vendor)
		return 1;
	if (a->product < b->product)
		return -1;
	if (a->product > b->product)
		return 1;
	if (a->id < b->id)
		return -1;
	if (a->id > b->id)
		return 1;

	return 0;
}

unsigned event_locate(struct event_location* event_map, unsigned event_max, const char* prefix, adv_bool* eacces)
{
	unsigned event_mac;
	unsigned i;

	event_mac = 0;
	for(i=0;i<event_max;++i) {
		unsigned short device_info[4];
		unsigned char evtype_bitmask[EV_MAX/8 + 1];
		int f;

		snprintf(event_map[event_mac].file, sizeof(event_map[event_mac].file), "/dev/input/%s%d", prefix, i);

		f = event_open(event_map[event_mac].file, evtype_bitmask, sizeof(evtype_bitmask));
		if (f == -1) {
			if (errno == EACCES) {
				*eacces = 1;
			}
			continue;
		}

		event_log(f, evtype_bitmask);

		if (ioctl(f, EVIOCGID, &device_info)) {
			log_std(("event: error in ioctl(EVIOCGID)\n"));
			event_close(f);
			continue;
		}

		event_map[event_mac].vendor = device_info[ID_VENDOR];
		event_map[event_mac].product = device_info[ID_PRODUCT];
		event_map[event_mac].version = device_info[ID_VERSION];
		event_map[event_mac].bus = device_info[ID_BUS];
		event_map[event_mac].id = i;
		++event_mac;

		event_close(f);
	}

	qsort(event_map, event_mac, sizeof(event_map[0]), event_compare);

	return event_mac;
}


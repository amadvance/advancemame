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

#include "event.h"

#include "log.h"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

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
			case KEY_ESC : desc = "KEY_ESC"; break;
			case KEY_1 : desc = "KEY_1"; break;
			case KEY_2 : desc = "KEY_2"; break;
			case KEY_3 : desc = "KEY_3"; break;
			case KEY_4 : desc = "KEY_4"; break;
			case KEY_5 : desc = "KEY_5"; break;
			case KEY_6 : desc = "KEY_6"; break;
			case KEY_7 : desc = "KEY_7"; break;
			case KEY_8 : desc = "KEY_8"; break;
			case KEY_9 : desc = "KEY_9"; break;
			case KEY_0 : desc = "KEY_0"; break;
			case KEY_MINUS : desc = "KEY_MINUS"; break;
			case KEY_EQUAL : desc = "KEY_EQUAL"; break;
			case KEY_BACKSPACE : desc = "KEY_BACKSPACE"; break;
			case KEY_TAB : desc = "KEY_TAB"; break;
			case KEY_Q : desc = "KEY_Q"; break;
			case KEY_W : desc = "KEY_W"; break;
			case KEY_E : desc = "KEY_E"; break;
			case KEY_R : desc = "KEY_R"; break;
			case KEY_T : desc = "KEY_T"; break;
			case KEY_Y : desc = "KEY_Y"; break;
			case KEY_U : desc = "KEY_U"; break;
			case KEY_I : desc = "KEY_I"; break;
			case KEY_O : desc = "KEY_O"; break;
			case KEY_P : desc = "KEY_P"; break;
			case KEY_LEFTBRACE : desc = "KEY_LEFTBRACE"; break;
			case KEY_RIGHTBRACE : desc = "KEY_RIGHTBRACE"; break;
			case KEY_ENTER : desc = "KEY_ENTER"; break;
			case KEY_LEFTCTRL : desc = "KEY_LEFTCTRL"; break;
			case KEY_A : desc = "KEY_A"; break;
			case KEY_S : desc = "KEY_S"; break;
			case KEY_D : desc = "KEY_D"; break;
			case KEY_F : desc = "KEY_F"; break;
			case KEY_G : desc = "KEY_G"; break;
			case KEY_H : desc = "KEY_H"; break;
			case KEY_J : desc = "KEY_J"; break;
			case KEY_K : desc = "KEY_K"; break;
			case KEY_L : desc = "KEY_L"; break;
			case KEY_SEMICOLON : desc = "KEY_SEMICOLON"; break;
			case KEY_APOSTROPHE : desc = "KEY_APOSTROPHE"; break;
			case KEY_GRAVE : desc = "KEY_GRAVE"; break;
			case KEY_LEFTSHIFT : desc = "KEY_LEFTSHIFT"; break;
			case KEY_BACKSLASH : desc = "KEY_BACKSLASH"; break;
			case KEY_Z : desc = "KEY_Z"; break;
			case KEY_X : desc = "KEY_X"; break;
			case KEY_C : desc = "KEY_C"; break;
			case KEY_V : desc = "KEY_V"; break;
			case KEY_B : desc = "KEY_B"; break;
			case KEY_N : desc = "KEY_N"; break;
			case KEY_M : desc = "KEY_M"; break;
			case KEY_COMMA : desc = "KEY_COMMA"; break;
			case KEY_DOT : desc = "KEY_DOT"; break;
			case KEY_SLASH : desc = "KEY_SLASH"; break;
			case KEY_RIGHTSHIFT : desc = "KEY_RIGHTSHIFT"; break;
			case KEY_KPASTERISK : desc = "KEY_KPASTERISK"; break;
			case KEY_LEFTALT : desc = "KEY_LEFTALT"; break;
			case KEY_SPACE : desc = "KEY_SPACE"; break;
			case KEY_CAPSLOCK : desc = "KEY_CAPSLOCK"; break;
			case KEY_F1 : desc = "KEY_F1"; break;
			case KEY_F2 : desc = "KEY_F2"; break;
			case KEY_F3 : desc = "KEY_F3"; break;
			case KEY_F4 : desc = "KEY_F4"; break;
			case KEY_F5 : desc = "KEY_F5"; break;
			case KEY_F6 : desc = "KEY_F6"; break;
			case KEY_F7 : desc = "KEY_F7"; break;
			case KEY_F8 : desc = "KEY_F8"; break;
			case KEY_F9 : desc = "KEY_F9"; break;
			case KEY_F10 : desc = "KEY_F10"; break;
			case KEY_NUMLOCK : desc = "KEY_NUMLOCK"; break;
			case KEY_SCROLLLOCK : desc = "KEY_SCROLLLOCK"; break;
			case KEY_KP7 : desc = "KEY_KP7"; break;
			case KEY_KP8 : desc = "KEY_KP8"; break;
			case KEY_KP9 : desc = "KEY_KP9"; break;
			case KEY_KPMINUS : desc = "KEY_KPMINUS"; break;
			case KEY_KP4 : desc = "KEY_KP4"; break;
			case KEY_KP5 : desc = "KEY_KP5"; break;
			case KEY_KP6 : desc = "KEY_KP6"; break;
			case KEY_KPPLUS : desc = "KEY_KPPLUS"; break;
			case KEY_KP1 : desc = "KEY_KP1"; break;
			case KEY_KP2 : desc = "KEY_KP2"; break;
			case KEY_KP3 : desc = "KEY_KP3"; break;
			case KEY_KP0 : desc = "KEY_KP0"; break;
			case KEY_KPDOT : desc = "KEY_KPDOT"; break;
			case KEY_103RD : desc = "KEY_103RD"; break;
			case KEY_F13 : desc = "KEY_F13"; break;
			case KEY_102ND : desc = "KEY_102ND"; break;
			case KEY_F11 : desc = "KEY_F11"; break;
			case KEY_F12 : desc = "KEY_F12"; break;
			case KEY_F14 : desc = "KEY_F14"; break;
			case KEY_F15 : desc = "KEY_F15"; break;
			case KEY_F16 : desc = "KEY_F16"; break;
			case KEY_F17 : desc = "KEY_F17"; break;
			case KEY_F18 : desc = "KEY_F18"; break;
			case KEY_F19 : desc = "KEY_F19"; break;
			case KEY_F20 : desc = "KEY_F20"; break;
			case KEY_KPENTER : desc = "KEY_KPENTER"; break;
			case KEY_RIGHTCTRL : desc = "KEY_RIGHTCTRL"; break;
			case KEY_KPSLASH : desc = "KEY_KPSLASH"; break;
			case KEY_SYSRQ : desc = "KEY_SYSRQ"; break;
			case KEY_RIGHTALT : desc = "KEY_RIGHTALT"; break;
			case KEY_LINEFEED : desc = "KEY_LINEFEED"; break;
			case KEY_HOME : desc = "KEY_HOME"; break;
			case KEY_UP : desc = "KEY_UP"; break;
			case KEY_PAGEUP : desc = "KEY_PAGEUP"; break;
			case KEY_LEFT : desc = "KEY_LEFT"; break;
			case KEY_RIGHT : desc = "KEY_RIGHT"; break;
			case KEY_END : desc = "KEY_END"; break;
			case KEY_DOWN : desc = "KEY_DOWN"; break;
			case KEY_PAGEDOWN : desc = "KEY_PAGEDOWN"; break;
			case KEY_INSERT : desc = "KEY_INSERT"; break;
			case KEY_DELETE : desc = "KEY_DELETE"; break;
			case KEY_MACRO : desc = "KEY_MACRO"; break;
			case KEY_MUTE : desc = "KEY_MUTE"; break;
			case KEY_VOLUMEDOWN : desc = "KEY_VOLUMEDOWN"; break;
			case KEY_VOLUMEUP : desc = "KEY_VOLUMEUP"; break;
			case KEY_POWER : desc = "KEY_POWER"; break;
			case KEY_KPEQUAL : desc = "KEY_KPEQUAL"; break;
			case KEY_KPPLUSMINUS : desc = "KEY_KPPLUSMINUS"; break;
			case KEY_PAUSE : desc = "KEY_PAUSE"; break;
			case KEY_F21 : desc = "KEY_F21"; break;
			case KEY_F22 : desc = "KEY_F22"; break;
			case KEY_F23 : desc = "KEY_F23"; break;
			case KEY_F24 : desc = "KEY_F24"; break;
			case KEY_KPCOMMA : desc = "KEY_KPCOMMA"; break;
			case KEY_LEFTMETA : desc = "KEY_LEFTMETA"; break;
			case KEY_RIGHTMETA : desc = "KEY_RIGHTMETA"; break;
			case KEY_COMPOSE : desc = "KEY_COMPOSE"; break;
#if 1
			case KEY_STOP : desc = "KEY_STOP"; break;
			case KEY_AGAIN : desc = "KEY_AGAIN"; break;
			case KEY_PROPS : desc = "KEY_PROPS"; break;
			case KEY_UNDO : desc = "KEY_UNDO"; break;
			case KEY_FRONT : desc = "KEY_FRONT"; break;
			case KEY_COPY : desc = "KEY_COPY"; break;
			case KEY_OPEN : desc = "KEY_OPEN"; break;
			case KEY_PASTE : desc = "KEY_PASTE"; break;
			case KEY_FIND : desc = "KEY_FIND"; break;
			case KEY_CUT : desc = "KEY_CUT"; break;
			case KEY_HELP : desc = "KEY_HELP"; break;
			case KEY_MENU : desc = "KEY_MENU"; break;
			case KEY_CALC : desc = "KEY_CALC"; break;
			case KEY_SETUP : desc = "KEY_SETUP"; break;
			case KEY_SLEEP : desc = "KEY_SLEEP"; break;
			case KEY_WAKEUP : desc = "KEY_WAKEUP"; break;
			case KEY_FILE : desc = "KEY_FILE"; break;
			case KEY_SENDFILE : desc = "KEY_SENDFILE"; break;
			case KEY_DELETEFILE : desc = "KEY_DELETEFILE"; break;
			case KEY_XFER : desc = "KEY_XFER"; break;
			case KEY_PROG1 : desc = "KEY_PROG1"; break;
			case KEY_PROG2 : desc = "KEY_PROG2"; break;
			case KEY_WWW : desc = "KEY_WWW"; break;
			case KEY_MSDOS : desc = "KEY_MSDOS"; break;
			case KEY_COFFEE : desc = "KEY_COFFEE"; break;
			case KEY_DIRECTION : desc = "KEY_DIRECTION"; break;
			case KEY_CYCLEWINDOWS : desc = "KEY_CYCLEWINDOWS"; break;
			case KEY_MAIL : desc = "KEY_MAIL"; break;
			case KEY_BOOKMARKS : desc = "KEY_BOOKMARKS"; break;
			case KEY_COMPUTER : desc = "KEY_COMPUTER"; break;
			case KEY_BACK : desc = "KEY_BACK"; break;
			case KEY_FORWARD : desc = "KEY_FORWARD"; break;
			case KEY_CLOSECD : desc = "KEY_CLOSECD"; break;
			case KEY_EJECTCD : desc = "KEY_EJECTCD"; break;
			case KEY_EJECTCLOSECD : desc = "KEY_EJECTCLOSECD"; break;
			case KEY_NEXTSONG : desc = "KEY_NEXTSONG"; break;
			case KEY_PLAYPAUSE : desc = "KEY_PLAYPAUSE"; break;
			case KEY_PREVIOUSSONG : desc = "KEY_PREVIOUSSONG"; break;
			case KEY_STOPCD : desc = "KEY_STOPCD"; break;
			case KEY_RECORD : desc = "KEY_RECORD"; break;
			case KEY_REWIND : desc = "KEY_REWIND"; break;
			case KEY_PHONE : desc = "KEY_PHONE"; break;
			case KEY_ISO : desc = "KEY_ISO"; break;
			case KEY_CONFIG : desc = "KEY_CONFIG"; break;
			case KEY_HOMEPAGE : desc = "KEY_HOMEPAGE"; break;
			case KEY_REFRESH : desc = "KEY_REFRESH"; break;
			case KEY_EXIT : desc = "KEY_EXIT"; break;
			case KEY_MOVE : desc = "KEY_MOVE"; break;
			case KEY_EDIT : desc = "KEY_EDIT"; break;
			case KEY_SCROLLUP : desc = "KEY_SCROLLUP"; break;
			case KEY_SCROLLDOWN : desc = "KEY_SCROLLDOWN"; break;
			case KEY_KPLEFTPAREN : desc = "KEY_KPLEFTPAREN"; break;
			case KEY_KPRIGHTPAREN : desc = "KEY_KPRIGHTPAREN"; break;
			case KEY_INTL1 : desc = "KEY_INTL1"; break;
			case KEY_INTL2 : desc = "KEY_INTL2"; break;
			case KEY_INTL3 : desc = "KEY_INTL3"; break;
			case KEY_INTL4 : desc = "KEY_INTL4"; break;
			case KEY_INTL5 : desc = "KEY_INTL5"; break;
			case KEY_INTL6 : desc = "KEY_INTL6"; break;
			case KEY_INTL7 : desc = "KEY_INTL7"; break;
			case KEY_INTL8 : desc = "KEY_INTL8"; break;
			case KEY_INTL9 : desc = "KEY_INTL9"; break;
			case KEY_LANG1 : desc = "KEY_LANG1"; break;
			case KEY_LANG2 : desc = "KEY_LANG2"; break;
			case KEY_LANG3 : desc = "KEY_LANG3"; break;
			case KEY_LANG4 : desc = "KEY_LANG4"; break;
			case KEY_LANG5 : desc = "KEY_LANG5"; break;
			case KEY_LANG6 : desc = "KEY_LANG6"; break;
			case KEY_LANG7 : desc = "KEY_LANG7"; break;
			case KEY_LANG8 : desc = "KEY_LANG8"; break;
			case KEY_LANG9 : desc = "KEY_LANG9"; break;
			case KEY_PLAYCD : desc = "KEY_PLAYCD"; break;
			case KEY_PAUSECD : desc = "KEY_PAUSECD"; break;
			case KEY_PROG3 : desc = "KEY_PROG3"; break;
			case KEY_PROG4 : desc = "KEY_PROG4"; break;
			case KEY_SUSPEND : desc = "KEY_SUSPEND"; break;
			case KEY_CLOSE : desc = "KEY_CLOSE"; break;
			case KEY_UNKNOWN : desc = "KEY_UNKNOWN"; break;
			case KEY_BRIGHTNESSDOWN : desc = "KEY_BRIGHTNESSDOWN"; break;
			case KEY_BRIGHTNESSUP : desc = "KEY_BRIGHTNESSUP"; break;
#endif
			case BTN_0 : desc = "BTN_0"; break;
			case BTN_1 : desc = "BTN_1"; break;
			case BTN_2 : desc = "BTN_2"; break;
			case BTN_3 : desc = "BTN_3"; break;
			case BTN_4 : desc = "BTN_4"; break;
			case BTN_5 : desc = "BTN_5"; break;
			case BTN_6 : desc = "BTN_6"; break;
			case BTN_7 : desc = "BTN_7"; break;
			case BTN_8 : desc = "BTN_8"; break;
			case BTN_9 : desc = "BTN_9"; break;
			case BTN_LEFT : desc = "BTN_LEFT"; break;
			case BTN_RIGHT : desc = "BTN_RIGHT"; break;
			case BTN_MIDDLE : desc = "BTN_MIDDLE"; break;
			case BTN_SIDE : desc = "BTN_SIDE"; break;
			case BTN_EXTRA : desc = "BTN_EXTRA"; break;
			case BTN_FORWARD : desc = "BTN_FORWARD"; break;
			case BTN_BACK : desc = "BTN_BACK"; break;
			case BTN_TRIGGER : desc = "BTN_TRIGGER"; break;
			case BTN_THUMB : desc = "BTN_THUMB"; break;
			case BTN_THUMB2 : desc = "BTN_THUMB2"; break;
			case BTN_TOP : desc = "BTN_TOP"; break;
			case BTN_TOP2 : desc = "BTN_TOP2"; break;
			case BTN_PINKIE : desc = "BTN_PINKIE"; break;
			case BTN_BASE : desc = "BTN_BASE"; break;
			case BTN_BASE2 : desc = "BTN_BASE2"; break;
			case BTN_BASE3 : desc = "BTN_BASE3"; break;
			case BTN_BASE4 : desc = "BTN_BASE4"; break;
			case BTN_BASE5 : desc = "BTN_BASE5"; break;
			case BTN_BASE6 : desc = "BTN_BASE6"; break;
			case BTN_DEAD : desc = "BTN_DEAD"; break;
			case BTN_A : desc = "BTN_A"; break;
			case BTN_B : desc = "BTN_B"; break;
			case BTN_C : desc = "BTN_C"; break;
			case BTN_X : desc = "BTN_X"; break;
			case BTN_Y : desc = "BTN_Y"; break;
			case BTN_Z : desc = "BTN_Z"; break;
			case BTN_TL : desc = "BTN_TL"; break;
			case BTN_TR : desc = "BTN_TR"; break;
			case BTN_TL2 : desc = "BTN_TL2"; break;
			case BTN_TR2 : desc = "BTN_TR2"; break;
			case BTN_SELECT : desc = "BTN_SELECT"; break;
			case BTN_START : desc = "BTN_START"; break;
			case BTN_MODE : desc = "BTN_MODE"; break;
			case BTN_THUMBL : desc = "BTN_THUMBL"; break;
			case BTN_THUMBR : desc = "BTN_THUMBR"; break;
			case BTN_TOOL_PEN : desc = "BTN_TOOL_PEN"; break;
			case BTN_TOOL_RUBBER : desc = "BTN_TOOL_RUBBER"; break;
			case BTN_TOOL_BRUSH : desc = "BTN_TOOL_BRUSH"; break;
			case BTN_TOOL_PENCIL : desc = "BTN_TOOL_PENCIL"; break;
			case BTN_TOOL_AIRBRUSH : desc = "BTN_TOOL_AIRBRUSH"; break;
			case BTN_TOOL_FINGER : desc = "BTN_TOOL_FINGER"; break;
			case BTN_TOOL_MOUSE : desc = "BTN_TOOL_MOUSE"; break;
			case BTN_TOOL_LENS : desc = "BTN_TOOL_LENS"; break;
			case BTN_TOUCH : desc = "BTN_TOUCH"; break;
			case BTN_STYLUS : desc = "BTN_STYLUS"; break;
			case BTN_STYLUS2 : desc = "BTN_STYLUS2"; break;
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
			case REL_X : desc = "REL_X"; break;
			case REL_Y : desc = "REL_Y"; break;
			case REL_Z : desc = "REL_Z"; break;
			case REL_HWHEEL : desc = "REL_HWHEEL"; break;
			case REL_DIAL : desc = "REL_DIAL"; break;
			case REL_WHEEL : desc = "REL_WHEEL"; break;
			case REL_MISC : desc = "REL_MISC"; break;
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
			case ABS_X : desc = "ABS_X"; break;
			case ABS_Y : desc = "ABS_Y"; break;
			case ABS_Z : desc = "ABS_Z"; break;
			case ABS_RX : desc = "ABS_RX"; break;
			case ABS_RY : desc = "ABS_RY"; break;
			case ABS_RZ : desc = "ABS_RZ"; break;
			case ABS_THROTTLE : desc = "ABS_THROTTLE"; break;
			case ABS_RUDDER : desc = "ABS_RUDDER"; break;
			case ABS_WHEEL : desc = "ABS_WHEEL"; break;
			case ABS_GAS : desc = "ABS_GAS"; break;
			case ABS_BRAKE : desc = "ABS_BRAKE"; break;
			case ABS_HAT0X : desc = "ABS_HAT0X"; break;
			case ABS_HAT0Y : desc = "ABS_HAT0Y"; break;
			case ABS_HAT1X : desc = "ABS_HAT1X"; break;
			case ABS_HAT1Y : desc = "ABS_HAT1Y"; break;
			case ABS_HAT2X : desc = "ABS_HAT2X"; break;
			case ABS_HAT2Y : desc = "ABS_HAT2Y"; break;
			case ABS_HAT3X : desc = "ABS_HAT3X"; break;
			case ABS_HAT3Y : desc = "ABS_HAT3Y"; break;
			case ABS_PRESSURE : desc = "ABS_PRESSURE"; break;
			case ABS_DISTANCE : desc = "ABS_DISTANCE"; break;
			case ABS_TILT_X : desc = "ABS_TILT_X"; break;
			case ABS_TILT_Y : desc = "ABS_TILT_Y"; break;
			case ABS_MISC : desc = "ABS_MISC"; break;
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
			case MSC_SERIAL : desc = "MSC_SERIAL"; break;
			case MSC_PULSELED : desc = "MSC_PULSELED"; break;
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
			case LED_NUML : desc = "LED_NUML"; break;
			case LED_CAPSL : desc = "LED_CAPSL"; break;
			case LED_SCROLLL : desc = "LED_SCROLLL"; break;
			case LED_COMPOSE : desc = "LED_COMPOSE"; break;
			case LED_KANA : desc = "LED_KANA"; break;
			case LED_SLEEP : desc = "LED_SLEEP"; break;
			case LED_SUSPEND : desc = "LED_SUSPEND"; break;
			case LED_MUTE : desc = "LED_MUTE"; break;
			case LED_MISC : desc = "LED_MISC"; break;
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
			case SND_CLICK : desc = "SND_CLICK"; break;
			case SND_BELL : desc = "SND_BELL"; break;
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
			case REP_DELAY : desc = "REP_DELAY"; break;
			case REP_PERIOD : desc = "REP_PERIOD"; break;
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
			case FF_BTN(BTN_TRIGGER) : desc = "FF_BTN(BTN_TRIGGER)"; break;
			case FF_BTN(BTN_THUMB) : desc = "FF_BTN(BTN_THUMB)"; break;
			case FF_BTN(BTN_THUMB2) : desc = "FF_BTN(BTN_THUMB2)"; break;
			case FF_BTN(BTN_TOP) : desc = "FF_BTN(BTN_TOP)"; break;
			case FF_BTN(BTN_TOP2) : desc = "FF_BTN(BTN_TOP2)"; break;
			case FF_BTN(BTN_PINKIE) : desc = "FF_BTN(BTN_PINKIE)"; break;
			case FF_BTN(BTN_BASE) : desc = "FF_BTN(BTN_BASE)"; break;
			case FF_BTN(BTN_BASE2) : desc = "FF_BTN(BTN_BASE2)"; break;
			case FF_BTN(BTN_BASE3) : desc = "FF_BTN(BTN_BASE3)"; break;
			case FF_BTN(BTN_BASE4) : desc = "FF_BTN(BTN_BASE4)"; break;
			case FF_BTN(BTN_BASE5) : desc = "FF_BTN(BTN_BASE5)"; break;
			case FF_BTN(BTN_BASE6) : desc = "FF_BTN(BTN_BASE6)"; break;
			case FF_BTN(BTN_DEAD) : desc = "FF_BTN(BTN_DEAD)"; break;
			case FF_ABS(ABS_X) : desc = "FF_ABS(ABS_X)"; break;
			case FF_ABS(ABS_Y) : desc = "FF_ABS(ABS_Y)"; break;
			case FF_ABS(ABS_Z) : desc = "FF_ABS(ABS_Z)"; break;
			case FF_RUMBLE : desc = "FF_RUMBLE"; break;
			case FF_PERIODIC : desc = "FF_PERIODIC"; break;
			case FF_CONSTANT : desc = "FF_CONSTANT"; break;
			case FF_SPRING : desc = "FF_SPRING"; break;
			case FF_FRICTION : desc = "FF_FRICTION"; break;
			case FF_SQUARE : desc = "FF_SQUARE"; break;
			case FF_TRIANGLE : desc = "FF_TRIANGLE"; break;
			case FF_SINE : desc = "FF_SINE"; break;
			case FF_SAW_UP : desc = "FF_SAW_UP"; break;
			case FF_SAW_DOWN : desc = "FF_SAW_DOWN"; break;
			case FF_CUSTOM : desc = "FF_CUSTOM"; break;
			case FF_GAIN : desc = "FF_GAIN"; break;
			case FF_AUTOCENTER : desc = "FF_AUTOCENTER"; break;
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

static void event_unknown_log(int f, unsigned e)
{
	unsigned char unk_bitmask[256 / 8];
	unsigned i;

	memset(unk_bitmask, 0, sizeof(unk_bitmask));
	if (ioctl(f, EVIOCGBIT(e, sizeof(unk_bitmask)), unk_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(0x%x,%d))\n", e, (int)FF_MAX));
		return;
	}

	log_std(("event: EV_0x%x:", e));

	for(i=0;i<256;++i) {
		if (event_test_bit(i, unk_bitmask)) {
			log_std((" 0x%x", i));
		}
	}

	log_std(("\n"));
}

int event_open(const char* file, unsigned char* evtype_bitmask)
{
	int version;
	short device_info[4];
	int f;
	const char* bus;

	f = open(file, O_RDWR | O_NDELAY);
	if (f == -1) {
		if (errno != ENODEV) {
			log_std(("event: error opening device %s, errno %d (%s)\n", file, errno, strerror(errno)));
		}
		goto err;
	}

	log_std(("event: open device %s\n", file));

	memset(evtype_bitmask, 0, sizeof(evtype_bitmask));
	if (ioctl(f, EVIOCGBIT(0, EV_MAX), evtype_bitmask) < 0) {
		log_std(("event: error in ioctl(EVIOCGBIT(0,%d)) on device %s\n", (int)EV_MAX, file));
		goto err_close;
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
	short device_info[4];
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
		case BUS_PCI : bus = "pci"; break;
		case BUS_ISAPNP : bus = "isapnp"; break;
		case BUS_USB : bus = "usb"; break;
		case BUS_ISA : bus = "usa"; break;
		case BUS_I8042 : bus = "i8042"; break;
		case BUS_RS232 : bus = "rs232"; break;
		case BUS_GAMEPORT : bus = "gameport"; break;
		case BUS_PARPORT : bus = "parport"; break;
		case BUS_AMIGA : bus = "amiga"; break;
		case BUS_ADB : bus = "adb"; break;
		case BUS_I2C : bus = "i2c"; break;
		default: bus = "unknown"; break;
		}

		log_std(("event: device vendor:0x%04hx product:0x%04hx version:0x%04hx bus:0x%04hx (%s)\n",
			device_info[ID_VENDOR],
			device_info[ID_PRODUCT],
			device_info[ID_VERSION],
			device_info[ID_BUS],
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
			case EV_MSC : event_msc_log(f); break;
			case EV_LED : event_led_log(f); break;
			case EV_SND : event_snd_log(f); break;
			case EV_REP : event_rep_log(f); break;
			case EV_FF : event_ff_log(f); break;
			default: event_unknown_log(f, i); break;
			}
		}
	}
}

adv_bool event_is_joystick(unsigned char* evtype_bitmask)
{
	return event_test_bit(EV_ABS, evtype_bitmask);
}

adv_bool event_is_mouse(unsigned char* evtype_bitmask)
{
	return !event_test_bit(EV_ABS, evtype_bitmask)
		&& event_test_bit(EV_REL, evtype_bitmask);
}

adv_bool event_is_keyboard(unsigned char* evtype_bitmask)
{
	return !event_test_bit(EV_ABS, evtype_bitmask)
		&& !event_test_bit(EV_REL, evtype_bitmask)
		&& event_test_bit(EV_KEY, evtype_bitmask);
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

	log_debug(("event: read %ld.%06ld, type %d, code %d, value %d\n", e.time.tv_sec, e.time.tv_usec, e.type, e.code, e.value));

	*type = e.type;
	*code = e.code;
	*value = e.value;

	return 0;
}


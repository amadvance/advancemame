/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include "kalleg.h"
#include "log.h"
#include "error.h"

#include "allegro2.h"

#include <sys/exceptn.h>

#define LOW_INVALID ((unsigned)0xFFFFFFFF)

struct keyb_allegro_context {
	unsigned map_up_to_low[KEYB_MAX];
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
{ KEYB_0_PAD, KEY_0_PAD },
{ KEYB_1_PAD, KEY_1_PAD },
{ KEYB_2_PAD, KEY_2_PAD },
{ KEYB_3_PAD, KEY_3_PAD },
{ KEYB_4_PAD, KEY_4_PAD },
{ KEYB_5_PAD, KEY_5_PAD },
{ KEYB_6_PAD, KEY_6_PAD },
{ KEYB_7_PAD, KEY_7_PAD },
{ KEYB_8_PAD, KEY_8_PAD },
{ KEYB_9_PAD, KEY_9_PAD },
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
{ KEYB_BACKQUOTE, KEY_TILDE },
{ KEYB_MINUS, KEY_MINUS },
{ KEYB_EQUALS, KEY_EQUALS },
{ KEYB_BACKSPACE, KEY_BACKSPACE },
{ KEYB_TAB, KEY_TAB },
{ KEYB_OPENBRACE, KEY_OPENBRACE },
{ KEYB_CLOSEBRACE, KEY_CLOSEBRACE },
{ KEYB_ENTER, KEY_ENTER },
{ KEYB_SEMICOLON, KEY_COLON },
{ KEYB_QUOTE, KEY_QUOTE },
{ KEYB_BACKSLASH, KEY_BACKSLASH },
{ KEYB_LESS, KEY_BACKSLASH2 },
{ KEYB_COMMA, KEY_COMMA },
{ KEYB_PERIOD, KEY_STOP },
{ KEYB_SLASH, KEY_SLASH },
{ KEYB_SPACE, KEY_SPACE },
{ KEYB_INSERT, KEY_INSERT },
{ KEYB_DEL, KEY_DEL },
{ KEYB_HOME, KEY_HOME },
{ KEYB_END, KEY_END },
{ KEYB_PGUP, KEY_PGUP },
{ KEYB_PGDN, KEY_PGDN },
{ KEYB_LEFT, KEY_LEFT },
{ KEYB_RIGHT, KEY_RIGHT },
{ KEYB_UP, KEY_UP },
{ KEYB_DOWN, KEY_DOWN },
{ KEYB_SLASH_PAD, KEY_SLASH_PAD },
{ KEYB_ASTERISK, KEY_ASTERISK },
{ KEYB_MINUS_PAD, KEY_MINUS_PAD },
{ KEYB_PLUS_PAD, KEY_PLUS_PAD },
{ KEYB_PERIOD_PAD, KEY_DEL_PAD },
{ KEYB_ENTER_PAD, KEY_ENTER_PAD },
{ KEYB_PRTSCR, KEY_PRTSCR },
{ KEYB_PAUSE, KEY_PAUSE },
{ KEYB_LSHIFT, KEY_LSHIFT },
{ KEYB_RSHIFT, KEY_RSHIFT },
{ KEYB_LCONTROL, KEY_LCONTROL },
{ KEYB_RCONTROL, KEY_RCONTROL },
{ KEYB_ALT, KEY_ALT },
{ KEYB_ALTGR, KEY_ALTGR },
{ KEYB_LWIN, KEY_LWIN },
{ KEYB_RWIN, KEY_RWIN },
{ KEYB_MENU, KEY_MENU },
{ KEYB_SCRLOCK, KEY_SCRLOCK },
{ KEYB_NUMLOCK, KEY_NUMLOCK },
{ KEYB_CAPSLOCK, KEY_CAPSLOCK },
{ KEYB_MAX, 0 }
};

static struct keyb_allegro_context allegro_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Allegro keyboard" },
{ 0, 0, 0 }
};

adv_error keyb_allegro_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* i;
	unsigned j;

	log_std(("key:allegro: keyb_allegro_init(id:%d, disable_special:%d)\n", keyb_id, (int)disable_special));

	for(j=0;j<KEYB_MAX;++j) {
		allegro_state.map_up_to_low[j] = LOW_INVALID;
	}
	for(i=KEYS;i->up_code != KEYB_MAX;++i) {
		allegro_state.map_up_to_low[i->up_code] = i->low_code;
	}

	if (disable_special) {
		/* disable BREAK (CTRL+C) and QUIT (CTRL+\) in protect mode */
		__djgpp_set_ctrl_c(0);

		/* disable BREAK in real mode */
		_go32_want_ctrl_break(1);

		/* disable the Allegro CTRL+ALT+END and CTRL+BREAK */
		three_finger_flag = 0;
	}

	return 0;
}

void keyb_allegro_done(void)
{
	log_std(("keyb:allegro: keyb_allegro_done()\n"));
}

adv_error keyb_allegro_enable(adv_bool graphics)
{
	log_std(("key:allegro: keyb_allegro_enable(graphics:%d)\n", (int)graphics));

	if (install_keyboard() != 0) {
		error_set("Function install_keyboard() failed.\n");
		return -1;
	}

	return 0;
}

void keyb_allegro_disable(void)
{
	log_std(("keyb:allegro: keyb_allegro_disable()\n"));

	remove_keyboard();
}

unsigned keyb_allegro_count_get(void)
{
	log_debug(("keyb:allegro: keyb_allegro_count_get()\n"));

	return 1;
}

adv_bool keyb_allegro_has(unsigned keyboard, unsigned code)
{
	log_debug(("keyb:allegro: keyb_svgalib_has()\n"));

	return allegro_state.map_up_to_low[code] != LOW_INVALID;
}

unsigned keyb_allegro_get(unsigned keyboard, unsigned code)
{
	unsigned low_code;

	log_debug(("keyb:allegro: keyb_allegro_get(keyboard:%d,code:%d)\n", keyboard, code));

	/* disable the pause key */
	if (code == KEYB_PAUSE)
		return 0;

	low_code = allegro_state.map_up_to_low[code];
	if (low_code == LOW_INVALID)
		return 0;

	return key[low_code];
}

void keyb_allegro_all_get(unsigned keyboard, unsigned char* code_map)
{
	unsigned i;

	log_debug(("keyb:allegro: keyb_allegro_all_get(keyboard:%d)\n", keyboard));

	for(i=0;i<KEYB_MAX;++i) {
		unsigned low_code = allegro_state.map_up_to_low[i];
		if (low_code == LOW_INVALID)
			code_map[i] = 0;
		else
			code_map[i] = key[low_code];
	}

	/* disable the pause key */
	code_map[KEYB_PAUSE] = 0;
}

void keyb_allegro_led_set(unsigned keyboard, unsigned led_mask)
{
	unsigned allegro_mask;

	log_debug(("keyb:allegro: keyb_allegro_led_set(keyboard:%d,mask:%d)\n", led_mask));

	allegro_mask = 0;

	if ((led_mask & KEYB_LED_NUML) != 0)
		allegro_mask |= KB_NUMLOCK_FLAG;
	if ((led_mask & KEYB_LED_CAPSL) != 0)
		allegro_mask |= KB_CAPSLOCK_FLAG;
	if ((led_mask & KEYB_LED_SCROLLL) != 0)
		allegro_mask |= KB_SCROLOCK_FLAG;

	set_leds(allegro_mask);
}

void keyb_allegro_poll(void)
{
	log_debug(("keyb:allegro: keyb_allegro_poll()\n"));

	if (keyboard_needs_poll())
		poll_keyboard();
}

unsigned keyb_allegro_flags(void)
{
	return 0;
}

adv_error keyb_allegro_load(adv_conf* context)
{
	return 0;
}

void keyb_allegro_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

keyb_driver keyb_allegro_driver = {
	"allegro",
	DEVICE,
	keyb_allegro_load,
	keyb_allegro_reg,
	keyb_allegro_init,
	keyb_allegro_done,
	keyb_allegro_enable,
	keyb_allegro_disable,
	keyb_allegro_flags,
	keyb_allegro_count_get,
	keyb_allegro_has,
	keyb_allegro_get,
	keyb_allegro_all_get,
	keyb_allegro_led_set,
	keyb_allegro_poll
};


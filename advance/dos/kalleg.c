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

#include "kalleg.h"
#include "log.h"
#include "target.h"

#include "allegro2.h"

#include <sys/exceptn.h>

struct keyb_allegro_context {
	unsigned map_os_to_code[OS_KEY_MAX];
};

static struct keyb_pair {
	int os;
	int code;
} KEYS[] = {
{ OS_KEY_A, KEY_A },
{ OS_KEY_B, KEY_B },
{ OS_KEY_C, KEY_C },
{ OS_KEY_D, KEY_D },
{ OS_KEY_E, KEY_E },
{ OS_KEY_F, KEY_F },
{ OS_KEY_G, KEY_G },
{ OS_KEY_H, KEY_H },
{ OS_KEY_I, KEY_I },
{ OS_KEY_J, KEY_J },
{ OS_KEY_K, KEY_K },
{ OS_KEY_L, KEY_L },
{ OS_KEY_M, KEY_M },
{ OS_KEY_N, KEY_N },
{ OS_KEY_O, KEY_O },
{ OS_KEY_P, KEY_P },
{ OS_KEY_Q, KEY_Q },
{ OS_KEY_R, KEY_R },
{ OS_KEY_S, KEY_S },
{ OS_KEY_T, KEY_T },
{ OS_KEY_U, KEY_U },
{ OS_KEY_V, KEY_V },
{ OS_KEY_W, KEY_W },
{ OS_KEY_X, KEY_X },
{ OS_KEY_Y, KEY_Y },
{ OS_KEY_Z, KEY_Z },
{ OS_KEY_0, KEY_0 },
{ OS_KEY_1, KEY_1 },
{ OS_KEY_2, KEY_2 },
{ OS_KEY_3, KEY_3 },
{ OS_KEY_4, KEY_4 },
{ OS_KEY_5, KEY_5 },
{ OS_KEY_6, KEY_6 },
{ OS_KEY_7, KEY_7 },
{ OS_KEY_8, KEY_8 },
{ OS_KEY_9, KEY_9 },
{ OS_KEY_0_PAD, KEY_0_PAD },
{ OS_KEY_1_PAD, KEY_1_PAD },
{ OS_KEY_2_PAD, KEY_2_PAD },
{ OS_KEY_3_PAD, KEY_3_PAD },
{ OS_KEY_4_PAD, KEY_4_PAD },
{ OS_KEY_5_PAD, KEY_5_PAD },
{ OS_KEY_6_PAD, KEY_6_PAD },
{ OS_KEY_7_PAD, KEY_7_PAD },
{ OS_KEY_8_PAD, KEY_8_PAD },
{ OS_KEY_9_PAD, KEY_9_PAD },
{ OS_KEY_F1, KEY_F1 },
{ OS_KEY_F2, KEY_F2 },
{ OS_KEY_F3, KEY_F3 },
{ OS_KEY_F4, KEY_F4 },
{ OS_KEY_F5, KEY_F5 },
{ OS_KEY_F6, KEY_F6 },
{ OS_KEY_F7, KEY_F7 },
{ OS_KEY_F8, KEY_F8 },
{ OS_KEY_F9, KEY_F9 },
{ OS_KEY_F10, KEY_F10 },
{ OS_KEY_F11, KEY_F11 },
{ OS_KEY_F12, KEY_F12 },
{ OS_KEY_ESC, KEY_ESC },
{ OS_KEY_BACKQUOTE, KEY_TILDE },
{ OS_KEY_MINUS, KEY_MINUS },
{ OS_KEY_EQUALS, KEY_EQUALS },
{ OS_KEY_BACKSPACE, KEY_BACKSPACE },
{ OS_KEY_TAB, KEY_TAB },
{ OS_KEY_OPENBRACE, KEY_OPENBRACE },
{ OS_KEY_CLOSEBRACE, KEY_CLOSEBRACE },
{ OS_KEY_ENTER, KEY_ENTER },
{ OS_KEY_SEMICOLON, KEY_COLON },
{ OS_KEY_QUOTE, KEY_QUOTE },
{ OS_KEY_BACKSLASH, KEY_BACKSLASH },
{ OS_KEY_LESS, KEY_BACKSLASH2 },
{ OS_KEY_COMMA, KEY_COMMA },
{ OS_KEY_PERIOD, KEY_STOP },
{ OS_KEY_SLASH, KEY_SLASH },
{ OS_KEY_SPACE, KEY_SPACE },
{ OS_KEY_INSERT, KEY_INSERT },
{ OS_KEY_DEL, KEY_DEL },
{ OS_KEY_HOME, KEY_HOME },
{ OS_KEY_END, KEY_END },
{ OS_KEY_PGUP, KEY_PGUP },
{ OS_KEY_PGDN, KEY_PGDN },
{ OS_KEY_LEFT, KEY_LEFT },
{ OS_KEY_RIGHT, KEY_RIGHT },
{ OS_KEY_UP, KEY_UP },
{ OS_KEY_DOWN, KEY_DOWN },
{ OS_KEY_SLASH_PAD, KEY_SLASH_PAD },
{ OS_KEY_ASTERISK, KEY_ASTERISK },
{ OS_KEY_MINUS_PAD, KEY_MINUS_PAD },
{ OS_KEY_PLUS_PAD, KEY_PLUS_PAD },
{ OS_KEY_PERIOD_PAD, KEY_DEL_PAD },
{ OS_KEY_ENTER_PAD, KEY_ENTER_PAD },
{ OS_KEY_PRTSCR, KEY_PRTSCR },
{ OS_KEY_PAUSE, KEY_PAUSE },
{ OS_KEY_LSHIFT, KEY_LSHIFT },
{ OS_KEY_RSHIFT, KEY_RSHIFT },
{ OS_KEY_LCONTROL, KEY_LCONTROL },
{ OS_KEY_RCONTROL, KEY_RCONTROL },
{ OS_KEY_ALT, KEY_ALT },
{ OS_KEY_ALTGR, KEY_ALTGR },
{ OS_KEY_LWIN, KEY_LWIN },
{ OS_KEY_RWIN, KEY_RWIN },
{ OS_KEY_MENU, KEY_MENU },
{ OS_KEY_SCRLOCK, KEY_SCRLOCK },
{ OS_KEY_NUMLOCK, KEY_NUMLOCK },
{ OS_KEY_CAPSLOCK, KEY_CAPSLOCK },
{ OS_KEY_MAX, 0 }
};

static struct keyb_allegro_context allegro_state;

static device DEVICE[] = {
{ "auto", -1, "Allegro keyboard" },
{ 0, 0, 0 }
};

adv_error keyb_allegro_init(int keyb_id, adv_bool disable_special)
{
	struct keyb_pair* i;
	unsigned j;

	log_std(("key:allegro: keyb_allegro_init(id:%d,disable_special:%d)\n",keyb_id,(int)disable_special));

	for(j=0;j<OS_KEY_MAX;++j) {
		allegro_state.map_os_to_code[j] = KEY_MAX;
	}
	for(i=KEYS;i->os != OS_KEY_MAX;++i) {
		allegro_state.map_os_to_code[i->os] = i->code;
	}

	if (install_keyboard() != 0) {
		log_std(("keyb:allegro: install_keyboard() failed\n"));
		return -1;
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

	remove_keyboard();
}

unsigned keyb_allegro_get(unsigned code)
{
	unsigned allegro_code;

	assert(code < OS_KEY_MAX);

	log_debug(("keyb:allegro: keyb_allegro_get(code:%d)\n",code));

	/* disable the pause key */
	if (code == OS_KEY_PAUSE)
		return 0;

	allegro_code = allegro_state.map_os_to_code[code];

	log_debug(("keyb:allegro: keyb_allegro_get() allegro_code:%d\n",allegro_code));

	if (allegro_code == KEY_MAX)
		return 0;

	return key[allegro_code];
}

void keyb_allegro_all_get(unsigned char* code_map)
{
	unsigned i;

	log_debug(("keyb:allegro: keyb_allegro_all_get()\n"));

	for(i=0;i<OS_KEY_MAX;++i) {
		unsigned allegro_code = allegro_state.map_os_to_code[i];
		if (allegro_code == KEY_MAX)
			code_map[i] = 0;
		else
			code_map[i] = key[allegro_code];
	}

	/* disable the pause key */
	code_map[OS_KEY_PAUSE] = 0;
}

void keyb_allegro_poll()
{
	log_debug(("keyb:allegro: keyb_allegro_poll()\n"));

	if (keyboard_needs_poll())
		poll_keyboard();
}

unsigned keyb_allegro_flags(void)
{
	return 0;
}

adv_error keyb_allegro_load(struct conf_context* context)
{
	return 0;
}

void keyb_allegro_reg(struct conf_context* context)
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
	keyb_allegro_flags,
	keyb_allegro_get,
	keyb_allegro_all_get,
	keyb_allegro_poll
};



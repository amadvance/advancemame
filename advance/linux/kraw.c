/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002 Kari Hautio <rusa@iki.fi>
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

#include "kraw.h"

#include "log.h"
#include "error.h"

#ifdef USE_VIDEO_SDL
#include "SDL.h"
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/kd.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned char scan2keycode[128] = {
	0, KEYB_ESC, KEYB_1, KEYB_2, KEYB_3, KEYB_4, KEYB_5, KEYB_6, KEYB_7, /* 0-8 */
	KEYB_8, KEYB_9, KEYB_0, KEYB_MINUS, KEYB_EQUALS, KEYB_BACKSPACE, /* 9-14 */
	KEYB_TAB, KEYB_Q, KEYB_W, KEYB_E, KEYB_R, KEYB_T, KEYB_Y, KEYB_U, /* 15-22 */
	KEYB_I, KEYB_O, KEYB_P, KEYB_OPENBRACE, KEYB_CLOSEBRACE, /* 23-27 */
	KEYB_ENTER, KEYB_LCONTROL, KEYB_A, KEYB_S, KEYB_D, KEYB_F, /* 28-33 */
	KEYB_G, KEYB_H, KEYB_J, KEYB_K, KEYB_L, KEYB_SEMICOLON, /* 34-39 */
	KEYB_QUOTE, KEYB_BACKQUOTE, KEYB_LSHIFT, KEYB_BACKSLASH, /* 40-43 */
	KEYB_Z, KEYB_X, KEYB_C, KEYB_V, KEYB_B, KEYB_N, KEYB_M, KEYB_COMMA, /* 44-51 */
	KEYB_PERIOD, KEYB_SLASH, KEYB_RSHIFT, KEYB_ASTERISK, KEYB_ALT, /* 52-56 */
	KEYB_SPACE, KEYB_CAPSLOCK, KEYB_F1, KEYB_F2, KEYB_F3, KEYB_F4, /* 57-62 */
	KEYB_F5, KEYB_F6, KEYB_F7, KEYB_F8, KEYB_F9, KEYB_F10, /* 63-68 */
	KEYB_NUMLOCK, KEYB_SCRLOCK, KEYB_7_PAD, KEYB_8_PAD, KEYB_9_PAD, /* 69-73 */
	KEYB_MINUS_PAD, KEYB_4_PAD, KEYB_5_PAD, KEYB_6_PAD, KEYB_PLUS_PAD, /* 74-78 */
	KEYB_1_PAD, KEYB_2_PAD, KEYB_3_PAD, KEYB_0_PAD, KEYB_PERIOD_PAD, /* 79-83 */
	0, 0, KEYB_LESS, KEYB_F11, KEYB_F12, 0, 0, 0, 0, 0, 0, 0, /* 84-95 */
	KEYB_ENTER_PAD, KEYB_RCONTROL, KEYB_SLASH_PAD, KEYB_PRTSCR, /* 96-99 */
	KEYB_ALTGR, KEYB_PAUSE, KEYB_HOME, KEYB_UP, KEYB_PGUP, KEYB_LEFT, /* 100-105 */
	KEYB_RIGHT, KEYB_END, KEYB_DOWN, KEYB_PGDN, KEYB_INSERT, /* 106-110 */
	KEYB_DEL, 0, 0, 0, 0, 0, 0, 0, KEYB_PAUSE, 0, 0, 0, 0, 0, /* 111-124 */
	KEYB_LWIN, KEYB_RWIN, KEYB_MENU /* 125-127 */
};

static adv_device DEVICE[] = {
{ "auto", -1, "RAW keyboard" },
{ 0, 0, 0 }
};

struct keyb_raw_context {
	unsigned char keystate[KEYB_MAX];
	struct termios oldkbdtermios;
	struct termios newkbdtermios;
	int oldkbmode;
	int kbd_fd;
};

static struct keyb_raw_context raw_state;

adv_error keyb_raw_init(int keyb_id, adv_bool disable_special)
{
	log_std(("keyb:raw: keyb_raw_init(id:%d, disable_special:%d)\n", keyb_id, (int)disable_special));

#ifdef USE_VIDEO_SDL
	/* If the SDL video driver is used, also the SDL */
	/* keyboard input must be used. */
	if (SDL_WasInit(SDL_INIT_VIDEO)) {
		log_std(("keyb:raw: Incompatible with the SDL video driver\n"));
		error_nolog_cat("raw: Incompatible with the SDL video driver\n");
		return -1; 
	}
#endif

	raw_state.kbd_fd = open("/dev/tty", O_RDONLY);
	if (raw_state.kbd_fd == -1) {
		log_std(("keyb:raw: open(\"/dev/tty\") failed\n"));
		return -1;
	}

	if (ioctl(raw_state.kbd_fd, KDGKBMODE, &raw_state.oldkbmode) != 0) {
		log_std(("keyb:raw: ioctl(KDGKBMODE) failed\n"));
		close(raw_state.kbd_fd);
		return -1;
	}

	if (tcgetattr(raw_state.kbd_fd, &raw_state.oldkbdtermios) != 0) {
		log_std(("keyb:raw: tcgetattr() failed\n"));
		close(raw_state.kbd_fd);
		return -1;
	}

	raw_state.newkbdtermios = raw_state.oldkbdtermios;

	raw_state.newkbdtermios.c_lflag &= ~(ICANON | ECHO | ISIG);
	raw_state.newkbdtermios.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
	raw_state.newkbdtermios.c_cc[VMIN] = 0; /* Making these 0 seems to have the */
	raw_state.newkbdtermios.c_cc[VTIME] = 0; /* desired effect. */

	if (tcsetattr(raw_state.kbd_fd, TCSAFLUSH, &raw_state.newkbdtermios) != 0) {
		log_std(("keyb:raw: tcsetattr(TCSAFLUSH) failed\n"));
		close(raw_state.kbd_fd);
		return -1;
	}

	if (ioctl(raw_state.kbd_fd, KDSKBMODE, K_MEDIUMRAW) != 0) {
		log_std(("keyb:raw: ioctl(KDGKBMODE) failed\n"));
		close(raw_state.kbd_fd);
		return -1;
	}

	memset(raw_state.keystate, 0, sizeof(raw_state.keystate));

	return 0;
}

void keyb_raw_done(void)
{
	log_std(("keyb:raw: keyb_raw_done()\n"));

	ioctl(raw_state.kbd_fd, KDSKBMODE, raw_state.oldkbmode);
	tcsetattr(raw_state.kbd_fd, 0, &raw_state.oldkbdtermios);
	close(raw_state.kbd_fd);
}

unsigned keyb_raw_get(unsigned code)
{
	return raw_state.keystate[code];
}

void keyb_raw_all_get(unsigned char* code_map)
{ 
	memcpy(code_map, raw_state.keystate, KEYB_MAX);
}

void keyb_raw_poll()
{
	unsigned char c;

	log_debug(("keyb:svgalib: keyb_svgalib_poll()\n"));

	while ((1==read(raw_state.kbd_fd, &c, 1)) && (c)) {
		raw_state.keystate[scan2keycode[c & 0x7f]] = (c & 0x80) ? 0 : 1;
	}
}

unsigned keyb_raw_flags(void)
{
	return 0;
}

adv_error keyb_raw_load(adv_conf* context)
{
	return 0;
}

void keyb_raw_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

keyb_driver keyb_raw_driver = {
	"raw",
	DEVICE,
	keyb_raw_load,
	keyb_raw_reg,
	keyb_raw_init,
	keyb_raw_done,
	keyb_raw_flags,
	keyb_raw_get,
	keyb_raw_all_get,
	keyb_raw_poll
};


/*
 * This file is part of the AdvanceMAME project.
 *
 * Copyright (C) 2001 Andrea Mazzoleni
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

#ifndef __OS_H
#define __OS_H

#ifndef __MSDOS__
#error This module is for MSDOS only
#endif

#include "conf.h"
#include "device.h"
#include "allegro2.h"

#include <time.h>
#include <keys.h>
#include <pc.h>
#include <sys/farptr.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Generic */

/** Max path length */
#define OS_MAXPATH 256

/** Max path list */
#define OS_MAXLIST 8192

/** Max command line length */
#define OS_MAXCMD 512

/** Max number of arguments */
#define OS_MAXARG 32

/***************************************************************************/
/* debug */

void os_msg_va(const char *text, va_list arg);
void os_msg(const char *text, ...);
int os_msg_init(const char* file, int sync_flag);
void os_msg_done(void);

#ifndef NDEBUG
#define os_log(a) os_msg a
#define os_log_debug(a) os_msg a
#define os_log_pedantic(a) do { } while (0)
#else
#define os_log(a) os_msg a
#define os_log_debug(a) do { } while (0)
#define os_log_pedantic(a) do { } while (0)
#endif

/***************************************************************************/
/* Init/Done */

int os_init(struct conf_context* context);
void os_done(void);

int os_inner_init(void);
void os_inner_done();

void os_poll(void);

extern int os_main(int argc, char* argv[]);

/***************************************************************************/
/* Clocks */

__extension__ typedef long long os_clock_t;

extern os_clock_t OS_CLOCKS_PER_SEC;

static __inline__ os_clock_t os_clock(void) {
	os_clock_t r;

	__asm__ __volatile__ (
		"rdtsc"
		: "=A" (r)
	);

	return r;
}

/***************************************************************************/
/* Keyboard */

#define OS_KEY_A KEY_A
#define OS_KEY_B KEY_B
#define OS_KEY_C KEY_C
#define OS_KEY_D KEY_D
#define OS_KEY_E KEY_E
#define OS_KEY_F KEY_F
#define OS_KEY_G KEY_G
#define OS_KEY_H KEY_H
#define OS_KEY_I KEY_I
#define OS_KEY_J KEY_J
#define OS_KEY_K KEY_K
#define OS_KEY_L KEY_L
#define OS_KEY_M KEY_M
#define OS_KEY_N KEY_N
#define OS_KEY_O KEY_O
#define OS_KEY_P KEY_P
#define OS_KEY_Q KEY_Q
#define OS_KEY_R KEY_R
#define OS_KEY_S KEY_S
#define OS_KEY_T KEY_T
#define OS_KEY_U KEY_U
#define OS_KEY_V KEY_V
#define OS_KEY_W KEY_W
#define OS_KEY_X KEY_X
#define OS_KEY_Y KEY_Y
#define OS_KEY_Z KEY_Z
#define OS_KEY_0 KEY_0
#define OS_KEY_1 KEY_1
#define OS_KEY_2 KEY_2
#define OS_KEY_3 KEY_3
#define OS_KEY_4 KEY_4
#define OS_KEY_5 KEY_5
#define OS_KEY_6 KEY_6
#define OS_KEY_7 KEY_7
#define OS_KEY_8 KEY_8
#define OS_KEY_9 KEY_9
#define OS_KEY_0_PAD KEY_0_PAD
#define OS_KEY_1_PAD KEY_1_PAD
#define OS_KEY_2_PAD KEY_2_PAD
#define OS_KEY_3_PAD KEY_3_PAD
#define OS_KEY_4_PAD KEY_4_PAD
#define OS_KEY_5_PAD KEY_5_PAD
#define OS_KEY_6_PAD KEY_6_PAD
#define OS_KEY_7_PAD KEY_7_PAD
#define OS_KEY_8_PAD KEY_8_PAD
#define OS_KEY_9_PAD KEY_9_PAD
#define OS_KEY_F1 KEY_F1
#define OS_KEY_F2 KEY_F2
#define OS_KEY_F3 KEY_F3
#define OS_KEY_F4 KEY_F4
#define OS_KEY_F5 KEY_F5
#define OS_KEY_F6 KEY_F6
#define OS_KEY_F7 KEY_F7
#define OS_KEY_F8 KEY_F8
#define OS_KEY_F9 KEY_F9
#define OS_KEY_F10 KEY_F10
#define OS_KEY_F11 KEY_F11
#define OS_KEY_F12 KEY_F12
#define OS_KEY_ESC KEY_ESC
#define OS_KEY_TILDE KEY_TILDE
#define OS_KEY_MINUS KEY_MINUS
#define OS_KEY_EQUALS KEY_EQUALS
#define OS_KEY_BACKSPACE KEY_BACKSPACE
#define OS_KEY_TAB KEY_TAB
#define OS_KEY_OPENBRACE KEY_OPENBRACE
#define OS_KEY_CLOSEBRACE KEY_CLOSEBRACE
#define OS_KEY_ENTER KEY_ENTER
#define OS_KEY_COLON KEY_COLON
#define OS_KEY_QUOTE KEY_QUOTE
#define OS_KEY_BACKSLASH KEY_BACKSLASH
#define OS_KEY_BACKSLASH2 KEY_BACKSLASH2
#define OS_KEY_COMMA KEY_COMMA
#define OS_KEY_STOP KEY_STOP
#define OS_KEY_SLASH KEY_SLASH
#define OS_KEY_SPACE KEY_SPACE
#define OS_KEY_INSERT KEY_INSERT
#define OS_KEY_DEL KEY_DEL
#define OS_KEY_HOME KEY_HOME
#define OS_KEY_END KEY_END
#define OS_KEY_PGUP KEY_PGUP
#define OS_KEY_PGDN KEY_PGDN
#define OS_KEY_LEFT KEY_LEFT
#define OS_KEY_RIGHT KEY_RIGHT
#define OS_KEY_UP KEY_UP
#define OS_KEY_DOWN KEY_DOWN
#define OS_KEY_SLASH_PAD KEY_SLASH_PAD
#define OS_KEY_ASTERISK KEY_ASTERISK
#define OS_KEY_MINUS_PAD KEY_MINUS_PAD
#define OS_KEY_PLUS_PAD KEY_PLUS_PAD
#define OS_KEY_DEL_PAD KEY_DEL_PAD
#define OS_KEY_ENTER_PAD KEY_ENTER_PAD
#define OS_KEY_PRTSCR KEY_PRTSCR
#define OS_KEY_PAUSE KEY_PAUSE
#define OS_KEY_YEN KEY_YEN
#define OS_KEY_YEN2 KEY_YEN2
#define OS_KEY_KANA KEY_KANA
#define OS_KEY_HENKAN KEY_HENKAN
#define OS_KEY_MUHENKAN KEY_MUHENKAN
#define OS_KEY_LSHIFT KEY_LSHIFT
#define OS_KEY_RSHIFT KEY_RSHIFT
#define OS_KEY_LCONTROL KEY_LCONTROL
#define OS_KEY_RCONTROL KEY_RCONTROL
#define OS_KEY_ALT KEY_ALT
#define OS_KEY_ALTGR KEY_ALTGR
#define OS_KEY_LWIN KEY_LWIN
#define OS_KEY_RWIN KEY_RWIN
#define OS_KEY_MENU KEY_MENU
#define OS_KEY_SCRLOCK KEY_SCRLOCK
#define OS_KEY_NUMLOCK KEY_NUMLOCK
#define OS_KEY_CAPSLOCK KEY_CAPSLOCK
#define OS_KEY_MAX KEY_MAX

extern device OS_KEY[];

int os_key_init(int key_id, int disable_special);
void os_key_done(void);

static __inline__ unsigned os_key_get(unsigned code) {
	if (code == KEY_PAUSE) /* disable the pause key */
		return 0;
	else
		return key[code] != 0;
}

static __inline__ void os_key_all_get(unsigned char* code_map) {
	unsigned i;
	for(i=0;i<OS_KEY_MAX;++i)
		code_map[i] = key[i];
	code_map[KEY_PAUSE] = 0; /* disable the pause key */
}

/***************************************************************************/
/* Led */

#define OS_LED_NUMLOCK KB_NUMLOCK_FLAG
#define OS_LED_CAPSLOCK KB_CAPSLOCK_FLAG
#define OS_LED_SCROLOCK KB_SCROLOCK_FLAG

static __inline__ void os_led_set(unsigned mask) {
	set_leds(mask);
}

/***************************************************************************/
/* Input */

int os_input_init(void);
void os_input_done(void);
int os_input_hit(void);
unsigned os_input_get(void);

#define OS_INPUT_UP K_Up
#define OS_INPUT_DOWN K_Down
#define OS_INPUT_LEFT K_Left
#define OS_INPUT_RIGHT K_Right
#define OS_INPUT_CTRLUP K_Control_Up
#define OS_INPUT_CTRLDOWN K_Control_Down
#define OS_INPUT_CTRLRIGHT K_Control_Right
#define OS_INPUT_CTRLLEFT K_Control_Left
#define OS_INPUT_ENTER K_Return
#define OS_INPUT_ESC K_Escape
#define OS_INPUT_HOME K_Home
#define OS_INPUT_END K_End
#define OS_INPUT_PGUP K_PageUp
#define OS_INPUT_PGDN K_PageDown
#define OS_INPUT_F1 K_F1
#define OS_INPUT_F2 K_F2
#define OS_INPUT_F3 K_F3
#define OS_INPUT_F4 K_F4
#define OS_INPUT_F5 K_F5
#define OS_INPUT_F6 K_F6
#define OS_INPUT_F7 K_F7
#define OS_INPUT_F8 K_F8
#define OS_INPUT_F9 K_F9
#define OS_INPUT_F10 K_F10
#define OS_INPUT_BACKSPACE K_BackSpace
#define OS_INPUT_DEL K_Delete
#define OS_INPUT_INS K_Insert
#define OS_INPUT_SPACE K_Space
#define OS_INPUT_TAB K_Tab

/***************************************************************************/
/* Mouse */

extern device OS_MOUSE[];

int os_mouse_init(int mouse_id);
void os_mouse_done(void);

unsigned os_mouse_count_get(void);

unsigned os_mouse_button_count_get(unsigned mouse);

void os_mouse_pos_get(unsigned mouse, int* x, int* y);
unsigned os_mouse_button_get(unsigned mouse, unsigned button);

/***************************************************************************/
/* Joystick */

extern device OS_JOY[];

int os_joy_init(int joystick_id);
void os_joy_done(void);

static __inline__ const char* os_joy_name_get(void) {
	return joystick_driver->name;
}

static __inline__ const char* os_joy_driver_name_get(void) {
	return joystick_driver->desc;
}

static __inline__ unsigned os_joy_count_get(void) {
	return num_joysticks;
}

static __inline__ unsigned os_joy_stick_count_get(unsigned j) {
	return joy[j].num_sticks;
}

static __inline__ unsigned os_joy_stick_axe_count_get(unsigned j, unsigned s) {
	return joy[j].stick[s].num_axis;
}

static __inline__ unsigned os_joy_button_count_get(unsigned j) {
	return joy[j].num_buttons;
}

static __inline__ const char* os_joy_stick_name_get(unsigned j, unsigned s) {
	return joy[j].stick[s].name;
}

static __inline__ const char* os_joy_stick_axe_name_get(unsigned j, unsigned s, unsigned a) {
	return joy[j].stick[s].axis[a].name;
}

static __inline__ const char* os_joy_button_name_get(unsigned j, unsigned b) {
	return joy[j].button[b].name;
}

static __inline__ int os_joy_button_get(unsigned j, unsigned b) {
	return joy[j].button[b].b;
}

static __inline__ int os_joy_stick_axe_digital_get(unsigned j, unsigned s, unsigned a, unsigned d) {
	const JOYSTICK_AXIS_INFO* jai = &joy[j].stick[s].axis[a];
	if (d)
		return jai->d1;
	else
		return jai->d2;
}

static __inline__ int os_joy_stick_axe_analog_get(unsigned j, unsigned s, unsigned a) {
	return joy[j].stick[s].axis[a].pos;
}

void os_joy_calib_start(void);
const char* os_joy_calib_next(void);

/***************************************************************************/
/* Hardware */

static __inline__ void os_port_set(unsigned addr, unsigned value) {
	outportb(addr,value);
}

static __inline__ unsigned os_port_get(unsigned addr) {
	return inportb(addr);
}

static __inline__ void os_writeb(unsigned addr, unsigned char c) {
	_farpokeb( _dos_ds, addr, c);
}

static __inline__ unsigned char os_readb(unsigned addr) {
	return _farpeekb(_dos_ds, addr);
}

static __inline__ int os_mmx_get(void) {
	return cpu_mmx;
}

/***************************************************************************/
/* Library */

void os_mode_reset(void);

void os_signal(int signum);
void os_default_signal(int signum);

void os_idle(void);
void os_usleep(unsigned us);

void os_sound_error(void);
void os_sound_warn(void);
void os_sound_signal(void);

int os_apm_shutdown(void);
int os_apm_standby(void);
int os_apm_wakeup(void);

int os_system(const char* cmd);
int os_spawn(const char* file, const char** argv);

/***************************************************************************/
/* File System */

char os_dir_separator(void);
char os_dir_slash(void);

const char* os_import(const char* path);
const char* os_export(const char* path);

/***************************************************************************/
/* Config */

const char* os_config_file_root(const char* file);
const char* os_config_file_home(const char* file);
const char* os_config_file_legacy(const char* file);

const char* os_config_dir_multidir(const char* tag);
const char* os_config_dir_singledir(const char* tag);
const char* os_config_dir_singlefile(void);

#ifdef __cplusplus
}
#endif

#endif

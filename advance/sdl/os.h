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

#include "conf.h"

#include "SDL.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Generic */

/** Max path length. */
#define OS_MAXPATH 256

/** Max command line length. */
#define OS_MAXCMD 1024

/** Max number of arguments. */
#define OS_MAXARG 64

struct os_device {
	const char *name;
	int id;
	const char* desc;
};

/***************************************************************************/
/* Debug */

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
void os_inner_done(void);
void os_poll(void);
void os_idle(void);
void os_usleep(unsigned us);
int os_main(int argc, char* argv[]);

/***************************************************************************/
/* Signal */

int os_is_term(void);
void os_signal(int signum);
void os_default_signal(int signum);

/***************************************************************************/
/* Clocks */

typedef long long os_clock_t;

extern os_clock_t OS_CLOCKS_PER_SEC;

os_clock_t os_clock(void);

/***************************************************************************/
/* Keyboard */

#define OS_KEY_A SDLK_a
#define OS_KEY_B SDLK_b
#define OS_KEY_C SDLK_c
#define OS_KEY_D SDLK_d
#define OS_KEY_E SDLK_e
#define OS_KEY_F SDLK_f
#define OS_KEY_G SDLK_g
#define OS_KEY_H SDLK_h
#define OS_KEY_I SDLK_i
#define OS_KEY_J SDLK_j
#define OS_KEY_K SDLK_k
#define OS_KEY_L SDLK_l
#define OS_KEY_M SDLK_m
#define OS_KEY_N SDLK_n
#define OS_KEY_O SDLK_o
#define OS_KEY_P SDLK_p
#define OS_KEY_Q SDLK_q
#define OS_KEY_R SDLK_r
#define OS_KEY_S SDLK_s
#define OS_KEY_T SDLK_t
#define OS_KEY_U SDLK_u
#define OS_KEY_V SDLK_v
#define OS_KEY_W SDLK_w
#define OS_KEY_X SDLK_x
#define OS_KEY_Y SDLK_y
#define OS_KEY_Z SDLK_z
#define OS_KEY_0 SDLK_0
#define OS_KEY_1 SDLK_1
#define OS_KEY_2 SDLK_2
#define OS_KEY_3 SDLK_3
#define OS_KEY_4 SDLK_4
#define OS_KEY_5 SDLK_5
#define OS_KEY_6 SDLK_6
#define OS_KEY_7 SDLK_7
#define OS_KEY_8 SDLK_8
#define OS_KEY_9 SDLK_9
#define OS_KEY_0_PAD SDLK_KP0
#define OS_KEY_1_PAD SDLK_KP1
#define OS_KEY_2_PAD SDLK_KP2
#define OS_KEY_3_PAD SDLK_KP3
#define OS_KEY_4_PAD SDLK_KP4
#define OS_KEY_5_PAD SDLK_KP5
#define OS_KEY_6_PAD SDLK_KP6
#define OS_KEY_7_PAD SDLK_KP7
#define OS_KEY_8_PAD SDLK_KP8
#define OS_KEY_9_PAD SDLK_KP9
#define OS_KEY_F1 SDLK_F1
#define OS_KEY_F2 SDLK_F2
#define OS_KEY_F3 SDLK_F3
#define OS_KEY_F4 SDLK_F4
#define OS_KEY_F5 SDLK_F5
#define OS_KEY_F6 SDLK_F6
#define OS_KEY_F7 SDLK_F7
#define OS_KEY_F8 SDLK_F8
#define OS_KEY_F9 SDLK_F9
#define OS_KEY_F10 SDLK_F10
#define OS_KEY_F11 SDLK_F11
#define OS_KEY_F12 SDLK_F12
#define OS_KEY_ESC SDLK_ESCAPE
#define OS_KEY_BACKQUOTE SDLK_BACKQUOTE
#define OS_KEY_MINUS SDLK_MINUS
#define OS_KEY_EQUALS SDLK_EQUALS
#define OS_KEY_BACKSPACE SDLK_BACKSPACE
#define OS_KEY_TAB SDLK_TAB
#define OS_KEY_OPENBRACE SDLK_LEFTBRACKET
#define OS_KEY_CLOSEBRACE SDLK_RIGHTBRACKET
#define OS_KEY_ENTER SDLK_RETURN
#define OS_KEY_SEMICOLON SDLK_SEMICOLON
#define OS_KEY_QUOTE SDLK_QUOTE
#define OS_KEY_BACKSLASH SDLK_BACKSLASH
#define OS_KEY_LESS SDLK_LESS
#define OS_KEY_COMMA SDLK_COMMA
#define OS_KEY_PERIOD SDLK_PERIOD
#define OS_KEY_SLASH SDLK_SLASH
#define OS_KEY_SPACE SDLK_SPACE
#define OS_KEY_INSERT SDLK_INSERT
#define OS_KEY_DEL SDLK_DELETE
#define OS_KEY_HOME SDLK_HOME
#define OS_KEY_END SDLK_END
#define OS_KEY_PGUP SDLK_PAGEUP
#define OS_KEY_PGDN SDLK_PAGEDOWN
#define OS_KEY_LEFT SDLK_LEFT
#define OS_KEY_RIGHT SDLK_RIGHT
#define OS_KEY_UP SDLK_UP
#define OS_KEY_DOWN SDLK_DOWN
#define OS_KEY_SLASH_PAD SDLK_KP_DIVIDE
#define OS_KEY_ASTERISK SDLK_KP_MULTIPLY
#define OS_KEY_MINUS_PAD SDLK_KP_MINUS
#define OS_KEY_PLUS_PAD SDLK_KP_PLUS
#define OS_KEY_PERIOD_PAD SDLK_KP_PERIOD
#define OS_KEY_ENTER_PAD SDLK_KP_ENTER
#define OS_KEY_PRTSCR SDLK_PRINT
#define OS_KEY_PAUSE SDLK_PAUSE
#define OS_KEY_LSHIFT SDLK_LSHIFT
#define OS_KEY_RSHIFT SDLK_RSHIFT
#define OS_KEY_LCONTROL SDLK_LCTRL
#define OS_KEY_RCONTROL SDLK_RCTRL
#define OS_KEY_ALT SDLK_LALT
#define OS_KEY_ALTGR SDLK_RALT
#define OS_KEY_LWIN SDLK_LMETA
#define OS_KEY_RWIN SDLK_COMPOSE
#define OS_KEY_MENU SDLK_MENU
#define OS_KEY_SCRLOCK SDLK_SCROLLOCK
#define OS_KEY_NUMLOCK SDLK_NUMLOCK
#define OS_KEY_CAPSLOCK SDLK_CAPSLOCK
#define OS_KEY_NONE SDLK_LAST
#define OS_KEY_MAX SDLK_LAST

extern struct os_device OS_KEY[];

int os_key_init(int key_id, int disable_special);
void os_key_done(void);

unsigned os_key_get(unsigned code);
void os_key_all_get(unsigned char* code_map);

/***************************************************************************/
/* Input */

#define OS_INPUT_UP 256
#define OS_INPUT_DOWN 257
#define OS_INPUT_LEFT 258
#define OS_INPUT_RIGHT 259
#define OS_INPUT_CTRLUP 260
#define OS_INPUT_CTRLDOWN 261
#define OS_INPUT_CTRLRIGHT 262
#define OS_INPUT_CTRLLEFT 263
#define OS_INPUT_HOME 264
#define OS_INPUT_END 265
#define OS_INPUT_PGUP 266
#define OS_INPUT_PGDN 267
#define OS_INPUT_F1 268
#define OS_INPUT_F2 269
#define OS_INPUT_F3 270
#define OS_INPUT_F4 271
#define OS_INPUT_F5 272
#define OS_INPUT_F6 273
#define OS_INPUT_F7 274
#define OS_INPUT_F8 275
#define OS_INPUT_F9 276
#define OS_INPUT_F10 277
#define OS_INPUT_DEL 278
#define OS_INPUT_INS 279
#define OS_INPUT_BACKSPACE 280
#define OS_INPUT_ENTER 13
#define OS_INPUT_ESC 27
#define OS_INPUT_SPACE 32
#define OS_INPUT_TAB 9

int os_input_init(void);
void os_input_done(void);
int os_input_hit(void);
unsigned os_input_get(void);

/***************************************************************************/
/* Led */

#define OS_LED_NUMLOCK 1
#define OS_LED_CAPSLOCK 2
#define OS_LED_SCROLOCK 4

/**
 * Set the keyboard leds state.
 */
void os_led_set(unsigned mask);

/***************************************************************************/
/* Mouse */

extern struct os_device OS_MOUSE[];

int os_mouse_init(int mouse_id);
void os_mouse_done(void);

unsigned os_mouse_count_get(void);

unsigned os_mouse_button_count_get(unsigned mouse);

void os_mouse_pos_get(unsigned mouse, int* x, int* y);
unsigned os_mouse_button_get(unsigned mouse, unsigned button);

/***************************************************************************/
/* Joystick */

extern struct os_device OS_JOY[];

int os_joy_init(int joy_id);
void os_joy_done(void);

const char* os_joy_name_get(void);
const char* os_joy_driver_name_get(void);
unsigned os_joy_count_get(void);
unsigned os_joy_stick_count_get(unsigned j);
unsigned os_joy_stick_axe_count_get(unsigned j, unsigned s);
unsigned os_joy_button_count_get(unsigned j);
const char* os_joy_stick_name_get(unsigned j, unsigned s);
const char* os_joy_stick_axe_name_get(unsigned j, unsigned s, unsigned a);
const char* os_joy_button_name_get(unsigned j, unsigned b);
int os_joy_button_get(unsigned j, unsigned b);
int os_joy_stick_axe_digital_get(unsigned j, unsigned s, unsigned a, unsigned d);
int os_joy_stick_axe_analog_get(unsigned j, unsigned s, unsigned a);

void os_joy_calib_start(void);
const char* os_joy_calib_next(void);

/***************************************************************************/
/* Hardware */

/**
 * Write a byte in a port.
 */
void os_port_set(unsigned addr, unsigned value);

/**
 * Read a byte from a port.
 */
unsigned os_port_get(unsigned addr);

/**
 * Write a byte an a absolute memory address.
 */
void os_writeb(unsigned addr, unsigned char c);

/**
 * Read a byte an a absolute memory address.
 */
unsigned char os_readb(unsigned addr);

/**
 * Check the MMX presence.
 * \return
 *  - 0 not present
 *  - 1 present
 */
int os_mmx_get(void);

/**
 * Reset the current video mode.
 * Generally called on emergency.
 */
void os_mode_reset(void);

/***************************************************************************/
/* Sound */

void os_sound_error(void);
void os_sound_warn(void);
void os_sound_signal(void);

/***************************************************************************/
/* APM */

/**
 * Shutdown the system.
 * \return ==0 (or never return) if success
 */
int os_apm_shutdown(void);

/**
 * Put the system in standby mode.
 * \return ==0 if success
 */
int os_apm_standby(void);

/**
 * Restore the system after a standby.
 * \return ==0 if success
 */
int os_apm_wakeup(void);

/***************************************************************************/
/* System */

/**
 * Execute an external program with pipe support.
 * \return Like system().
 */
int os_system(const char* cmd);

/**
 * Execute an external program.
 * \return Like spawn().
 */
int os_spawn(const char* file, const char** argv);

/***************************************************************************/
/* FileSystem */

/**
 * Return the char used as a dir separator.
 * Example ':' in UNIX and ';' in MSDOS.
 */
char os_dir_separator(void);

/**
 * Return the char used as a dir slashr.
 * Example '/' in UNIX and '\' in MSDOS.
 */
char os_dir_slash(void);

/**
 * Convert a path from the OS format in a standard UNIX format.
 * The returned buffer may be the same argument or a static buffer.
 * If a static buffer is used, you need at least two static buffer to use
 * alternatively.
 */
const char* os_import(const char* path);

/*
 * Convert a path to the OS format from a standard UNIX format.
 * The returned buffer may be the same argument or a static buffer.
 * If a static buffer is used, you need at least two static buffer to use
 * alternatively.
 */
const char* os_export(const char* path);

/***************************************************************************/
/* Files */

/**
 * Complete path of a file in the root data directory.
 * If the path is relative, the root directory is added. If the
 * path is absolute the path isn't changed.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path or 0 if the root dir is not supported.
 */
const char* os_config_file_root(const char* file);

/**
 * Complete path of a file in the home data directory.
 * If the path is relative, the home directory is added. If the
 * path is absolute the path isn't changed.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path.
 */
const char* os_config_file_home(const char* file);

/**
 * Complete path of a file in the legacy data directory.
 * If the path is relative, the legacye directory is added. If the
 * path is absolute the path isn't changed.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path or 0 if the legacy dir is not supported.
 */
const char* os_config_file_legacy(const char* file);

/**
 * Directory list where to search a subdirectory or a file.
 * \note The returned value is in the OS depended format.
 * \return The directory list with the tag added. Generally first the HOME DATA directory and as second choice the ROOT DATA directory.
 */
const char* os_config_dir_multidir(const char* tag);

/**
 * Single directory where to search a subdirectory or a file.
 * \note The returned value is in the OS depended format.
 * \return The directory with the tag added. Generally the HOME DATA directory.
 */
const char* os_config_dir_singledir(const char* tag);

/**
 * Directory list where search a single support file.
 * \note The returned value is in the OS depended format.
 * \return The directory list. Generally first the HOME DATA directory and as second choice the ROOT DATA directory.
 */
const char* os_config_dir_singlefile(void);

#ifdef __cplusplus
}
#endif

#endif

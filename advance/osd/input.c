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

#include "emu.h"
#include "log.h"
#include "target.h"
#include "error.h"

#include "mame2.h"

#include "hscript.h"
#include "conf.h"
#include "os.h"
#include "keyall.h"
#include "mouseall.h"
#include "joyall.h"

#include <time.h>

/**************************************************************************/
/* Internals */

static struct KeyboardInfo input_key_map[] = {
	{ "A", KEYB_A, KEYCODE_A },
	{ "B", KEYB_B, KEYCODE_B },
	{ "C", KEYB_C, KEYCODE_C },
	{ "D", KEYB_D, KEYCODE_D },
	{ "E", KEYB_E, KEYCODE_E },
	{ "F", KEYB_F, KEYCODE_F },
	{ "G", KEYB_G, KEYCODE_G },
	{ "H", KEYB_H, KEYCODE_H },
	{ "I", KEYB_I, KEYCODE_I },
	{ "J", KEYB_J, KEYCODE_J },
	{ "K", KEYB_K, KEYCODE_K },
	{ "L", KEYB_L, KEYCODE_L },
	{ "M", KEYB_M, KEYCODE_M },
	{ "N", KEYB_N, KEYCODE_N },
	{ "O", KEYB_O, KEYCODE_O },
	{ "P", KEYB_P, KEYCODE_P },
	{ "Q", KEYB_Q, KEYCODE_Q },
	{ "R", KEYB_R, KEYCODE_R },
	{ "S", KEYB_S, KEYCODE_S },
	{ "T", KEYB_T, KEYCODE_T },
	{ "U", KEYB_U, KEYCODE_U },
	{ "V", KEYB_V, KEYCODE_V },
	{ "W", KEYB_W, KEYCODE_W },
	{ "X", KEYB_X, KEYCODE_X },
	{ "Y", KEYB_Y, KEYCODE_Y },
	{ "Z", KEYB_Z, KEYCODE_Z },
	{ "0", KEYB_0, KEYCODE_0 },
	{ "1", KEYB_1, KEYCODE_1 },
	{ "2", KEYB_2, KEYCODE_2 },
	{ "3", KEYB_3, KEYCODE_3 },
	{ "4", KEYB_4, KEYCODE_4 },
	{ "5", KEYB_5, KEYCODE_5 },
	{ "6", KEYB_6, KEYCODE_6 },
	{ "7", KEYB_7, KEYCODE_7 },
	{ "8", KEYB_8, KEYCODE_8 },
	{ "9", KEYB_9, KEYCODE_9 },
	{ "0PAD", KEYB_0_PAD, KEYCODE_0_PAD },
	{ "1PAD", KEYB_1_PAD, KEYCODE_1_PAD },
	{ "2PAD", KEYB_2_PAD, KEYCODE_2_PAD },
	{ "3PAD", KEYB_3_PAD, KEYCODE_3_PAD },
	{ "4PAD", KEYB_4_PAD, KEYCODE_4_PAD },
	{ "5PAD", KEYB_5_PAD, KEYCODE_5_PAD },
	{ "6PAD", KEYB_6_PAD, KEYCODE_6_PAD },
	{ "7PAD", KEYB_7_PAD, KEYCODE_7_PAD },
	{ "8PAD", KEYB_8_PAD, KEYCODE_8_PAD },
	{ "9PAD", KEYB_9_PAD, KEYCODE_9_PAD },
	{ "F1", KEYB_F1, KEYCODE_F1 },
	{ "F2", KEYB_F2, KEYCODE_F2 },
	{ "F3", KEYB_F3, KEYCODE_F3 },
	{ "F4", KEYB_F4, KEYCODE_F4 },
	{ "F5", KEYB_F5, KEYCODE_F5 },
	{ "F6", KEYB_F6, KEYCODE_F6 },
	{ "F7", KEYB_F7, KEYCODE_F7 },
	{ "F8", KEYB_F8, KEYCODE_F8 },
	{ "F9", KEYB_F9, KEYCODE_F9 },
	{ "F10", KEYB_F10, KEYCODE_F10 },
	{ "F11", KEYB_F11, KEYCODE_F11 },
	{ "F12", KEYB_F12, KEYCODE_F12 },
	{ "ESC", KEYB_ESC, KEYCODE_ESC },
	{ "~", KEYB_BACKQUOTE, KEYCODE_TILDE },
	{ "-", KEYB_MINUS, KEYCODE_MINUS },
	{ "=", KEYB_EQUALS, KEYCODE_EQUALS },
	{ "BKSPACE", KEYB_BACKSPACE, KEYCODE_BACKSPACE },
	{ "TAB", KEYB_TAB, KEYCODE_TAB },
	{ "[", KEYB_OPENBRACE, KEYCODE_OPENBRACE },
	{ "]", KEYB_CLOSEBRACE, KEYCODE_CLOSEBRACE },
	{ "ENTER", KEYB_ENTER, KEYCODE_ENTER },
	{ ";", KEYB_SEMICOLON, KEYCODE_COLON },
	{ ":", KEYB_QUOTE, KEYCODE_QUOTE },
	{ "\\", KEYB_BACKSLASH, KEYCODE_BACKSLASH },
	{ "<", KEYB_LESS, KEYCODE_BACKSLASH2 },
	{ ",", KEYB_COMMA, KEYCODE_COMMA },
	{ ".", KEYB_PERIOD, KEYCODE_STOP },
	{ "/", KEYB_SLASH, KEYCODE_SLASH },
	{ "SPACE", KEYB_SPACE, KEYCODE_SPACE },
	{ "INS", KEYB_INSERT, KEYCODE_INSERT },
	{ "DEL", KEYB_DEL, KEYCODE_DEL },
	{ "HOME", KEYB_HOME, KEYCODE_HOME },
	{ "END", KEYB_END, KEYCODE_END },
	{ "PGUP", KEYB_PGUP, KEYCODE_PGUP },
	{ "PGDN", KEYB_PGDN, KEYCODE_PGDN },
	{ "LEFT", KEYB_LEFT, KEYCODE_LEFT },
	{ "RIGHT", KEYB_RIGHT, KEYCODE_RIGHT },
	{ "UP", KEYB_UP, KEYCODE_UP },
	{ "DOWN", KEYB_DOWN, KEYCODE_DOWN },
	{ "/PAD", KEYB_SLASH_PAD, KEYCODE_SLASH_PAD },
	{ "*PAD", KEYB_ASTERISK, KEYCODE_ASTERISK },
	{ "-PAD", KEYB_MINUS_PAD, KEYCODE_MINUS_PAD },
	{ "+PAD", KEYB_PLUS_PAD, KEYCODE_PLUS_PAD },
	{ ".PAD", KEYB_PERIOD_PAD, KEYCODE_DEL_PAD },
	{ "ENTERPAD", KEYB_ENTER_PAD, KEYCODE_ENTER_PAD },
	{ "PRTSCR", KEYB_PRTSCR, KEYCODE_PRTSCR },
	{ "PAUSE", KEYB_PAUSE, KEYCODE_PAUSE },
	{ "LSHIFT", KEYB_LSHIFT, KEYCODE_LSHIFT },
	{ "RSHIFT", KEYB_RSHIFT, KEYCODE_RSHIFT },
	{ "LCTRL", KEYB_LCONTROL, KEYCODE_LCONTROL },
	{ "RCTRL", KEYB_RCONTROL, KEYCODE_RCONTROL },
	{ "ALT", KEYB_ALT, KEYCODE_LALT },
	{ "ALTGR", KEYB_ALTGR, KEYCODE_RALT },
	{ "LWIN", KEYB_LWIN, KEYCODE_OTHER },
	{ "RWIN", KEYB_RWIN, KEYCODE_OTHER },
	{ "MENU", KEYB_MENU, KEYCODE_OTHER },
	{ "SCRLOCK", KEYB_SCRLOCK, KEYCODE_SCRLOCK },
	{ "NUMLOCK", KEYB_NUMLOCK, KEYCODE_NUMLOCK },
	{ "CAPSLOCK", KEYB_CAPSLOCK, KEYCODE_CAPSLOCK },
	{ 0, 0, 0 }
};

/*
 * T - Type
 * D - Device number
 * S - Stick number
 * A - Axe number
 * D - Direction
 * B - Button number
 */
/* Warning! Check the INPUT_*_MAX, they must match. */

/* The DIGITAL values are also saved in the .cfg file. If you change them */
/* the cfg become invalid. */
#define DIGITAL_TYPE_JOY 0 /* Joy digital move - DAAASSSDDDTT */
#define DIGITAL_TYPE_JOY_BUTTON 1 /* Joy button - BBBBDDDTT */
#define DIGITAL_TYPE_MOUSE_BUTTON 2 /* Mouse button - BBBBDDDTT */
#define DIGITAL_TYPE_GET(i) ((i) & 0x3)

#define DIGITAL_JOY_DEV_GET(i) (((i) >> 2) & 0x7)
#define DIGITAL_JOY_STICK_GET(i) (((i) >> 5) & 0x7)
#define DIGITAL_JOY_AXE_GET(i) (((i) >> 8) & 0x7)
#define DIGITAL_JOY_DIR_GET(i) (((i) >> 11) & 0x1)
#define DIGITAL_JOY(joy, stick, axe, dir) (DIGITAL_TYPE_JOY | (joy) << 2 | (stick) << 5 | (axe) << 8 | (dir) << 11)

#define DIGITAL_JOY_BUTTON_DEV_GET(i) (((i) >> 2) & 0x7)
#define DIGITAL_JOY_BUTTON_BUTTON_GET(i) (((i) >> 5) & 0xF)
#define DIGITAL_JOY_BUTTON(joy, button) (DIGITAL_TYPE_JOY_BUTTON | (joy) << 2 | (button) << 5)

#define DIGITAL_MOUSE_BUTTON_DEV_GET(i) (((i) >> 2) & 0x7)
#define DIGITAL_MOUSE_BUTTON_BUTTON_GET(i) (((i) >> 5) & 0xF)
#define DIGITAL_MOUSE_BUTTON(mouse, button) (DIGITAL_TYPE_MOUSE_BUTTON | (mouse) << 2 | (button) << 5)

/* The ANALOG value can be changed without limitation */
#define ANALOG_TYPE_NONE 0
#define ANALOG_TYPE_MOUSE 1 /* Mouse - NAAADDDTT */
#define ANALOG_TYPE_JOY 2 /* Joy - NAAASSSDDDTT */
#define ANALOG_TYPE_GET(i) ((i) & 0x3)

#define ANALOG_MOUSE_DEV_GET(i) (((i) >> 2) & 0x7)
#define ANALOG_MOUSE_AXE_GET(i) (((i) >> 5) & 0x7)
#define ANALOG_MOUSE_NEGATE_GET(i) (((i) >> 8) & 0x1)
#define ANALOG_MOUSE(dev, axe, negate) (ANALOG_TYPE_MOUSE | (dev) << 2 | (axe) << 5 | (negate) << 8)

#define ANALOG_JOY_DEV_GET(i) (((i) >> 2) & 0x7)
#define ANALOG_JOY_STICK_GET(i) (((i) >> 5) & 0x7)
#define ANALOG_JOY_AXE_GET(i) (((i) >> 8) & 0x7)
#define ANALOG_JOY_NEGATE_GET(i) (((i) >> 11) & 0x1)
#define ANALOG_JOY(joy, stick, axe, negate) (ANALOG_TYPE_JOY | (joy) << 2 | (stick) << 5 | (axe) << 8 | (negate) << 11)

/** Max number of joystick different input. */
#define INPUT_MAX 512

/** Max input name. */
#define INPUT_NAME_MAX 48

static struct JoystickInfo input_joy_map[INPUT_MAX] = {
	/* will be filled later */
	{ 0, 0, 0 } /* end of table */
};

/* will be used to store names for the above */
static char input_joyname_map[INPUT_MAX][INPUT_NAME_MAX];

static int input_joyequiv_map[][2] = {
	{ DIGITAL_JOY(0, 0, 0, 1), JOYCODE_1_LEFT },
	{ DIGITAL_JOY(0, 0, 0, 0), JOYCODE_1_RIGHT },
	{ DIGITAL_JOY(0, 0, 1, 1), JOYCODE_1_UP },
	{ DIGITAL_JOY(0, 0, 1, 0), JOYCODE_1_DOWN },
	{ DIGITAL_JOY_BUTTON(0, 0), JOYCODE_1_BUTTON1 },
	{ DIGITAL_JOY_BUTTON(0, 1), JOYCODE_1_BUTTON2 },
	{ DIGITAL_JOY_BUTTON(0, 2), JOYCODE_1_BUTTON3 },
	{ DIGITAL_JOY_BUTTON(0, 3), JOYCODE_1_BUTTON4 },
	{ DIGITAL_JOY_BUTTON(0, 4), JOYCODE_1_BUTTON5 },
	{ DIGITAL_JOY_BUTTON(0, 5), JOYCODE_1_BUTTON6 },
	{ DIGITAL_JOY(1, 0, 0, 1), JOYCODE_2_LEFT },
	{ DIGITAL_JOY(1, 0, 0, 0), JOYCODE_2_RIGHT },
	{ DIGITAL_JOY(1, 0, 1, 1), JOYCODE_2_UP },
	{ DIGITAL_JOY(1, 0, 1, 0), JOYCODE_2_DOWN },
	{ DIGITAL_JOY_BUTTON(1, 0), JOYCODE_2_BUTTON1 },
	{ DIGITAL_JOY_BUTTON(1, 1), JOYCODE_2_BUTTON2 },
	{ DIGITAL_JOY_BUTTON(1, 2), JOYCODE_2_BUTTON3 },
	{ DIGITAL_JOY_BUTTON(1, 3), JOYCODE_2_BUTTON4 },
	{ DIGITAL_JOY_BUTTON(1, 4), JOYCODE_2_BUTTON5 },
	{ DIGITAL_JOY_BUTTON(1, 5), JOYCODE_2_BUTTON6 },
	{ DIGITAL_JOY(2, 0, 0, 1), JOYCODE_3_LEFT },
	{ DIGITAL_JOY(2, 0, 0, 0), JOYCODE_3_RIGHT },
	{ DIGITAL_JOY(2, 0, 1, 1), JOYCODE_3_UP },
	{ DIGITAL_JOY(2, 0, 1, 0), JOYCODE_3_DOWN },
	{ DIGITAL_JOY_BUTTON(2, 0), JOYCODE_3_BUTTON1 },
	{ DIGITAL_JOY_BUTTON(2, 1), JOYCODE_3_BUTTON2 },
	{ DIGITAL_JOY_BUTTON(2, 2), JOYCODE_3_BUTTON3 },
	{ DIGITAL_JOY_BUTTON(2, 3), JOYCODE_3_BUTTON4 },
	{ DIGITAL_JOY_BUTTON(2, 4), JOYCODE_3_BUTTON5 },
	{ DIGITAL_JOY_BUTTON(2, 5), JOYCODE_3_BUTTON6 },
	{ DIGITAL_JOY(3, 0, 0, 1), JOYCODE_4_LEFT },
	{ DIGITAL_JOY(3, 0, 0, 0), JOYCODE_4_RIGHT },
	{ DIGITAL_JOY(3, 0, 1, 1), JOYCODE_4_UP },
	{ DIGITAL_JOY(3, 0, 1, 0), JOYCODE_4_DOWN },
	{ DIGITAL_JOY_BUTTON(3, 0), JOYCODE_4_BUTTON1 },
	{ DIGITAL_JOY_BUTTON(3, 1), JOYCODE_4_BUTTON2 },
	{ DIGITAL_JOY_BUTTON(3, 2), JOYCODE_4_BUTTON3 },
	{ DIGITAL_JOY_BUTTON(3, 3), JOYCODE_4_BUTTON4 },
	{ DIGITAL_JOY_BUTTON(3, 4), JOYCODE_4_BUTTON5 },
	{ DIGITAL_JOY_BUTTON(3, 5), JOYCODE_4_BUTTON6 }
};

/*
 * Since the keyboard controller is slow, it is not capable of reporting multiple
 * key presses fast enough. We have to delay them in order not to lose special moves
 * tied to simultaneous button presses.
 */
static void input_keyboard_update(struct advance_input_context* context)
{
	if (context->config.steadykey_flag) {
		unsigned char last[KEYB_MAX];

		keyb_all_get(last);

		assert( sizeof(last) == sizeof(context->state.key_current) );
		assert( sizeof(last) == sizeof(context->state.key_old) );

		if (memcmp(last, context->state.key_old, sizeof(last))==0) {
			/* if keyboard state is stable, copy it over */
			memcpy(context->state.key_current, last, sizeof(last));
		} else {
			/* save the new copy */
			memcpy(context->state.key_old, last, sizeof(last));
		}

	} else {
		keyb_all_get(context->state.key_current);
	}
}

static inline int input_is_key_pressed(struct advance_input_context* context, int keycode)
{
	if (keycode >= KEYB_MAX)
		return 0;

	return context->state.key_current[keycode];
}

static void input_init(struct advance_input_context* context)
{
	unsigned i, j, k, w;
	unsigned mac;
	char buf[256];

	/* initialize the state */
	for(i=0;i<INPUT_MOUSE_MAX;++i) {
		for(j=0;j<INPUT_AXE_MAX;++j) {
			context->state.mouse_analog_current[i][j] = 0;
		}
		for(j=0;j<INPUT_BUTTON_MAX;++j) {
			context->state.mouse_button_current[i][j] = 0;
		}
	}

	for(i=0;i<INPUT_JOY_MAX;++i) {
		for(j=0;j<INPUT_STICK_MAX;++j) {
			for(k=0;k<INPUT_AXE_MAX;++k) {
				context->state.joystick_analog_current[i][j][k] = 0;
				for(w=0;w<INPUT_DIR_MAX;++w) {
					context->state.joystick_digital_current[i][j][k][w] = 0;
				}
			}
		}

		for(j=0;j<INPUT_BUTTON_MAX;++j) {
			context->state.joystick_button_current[i][j] = 0;
		}
	}

	for(i=0;i<KEYB_MAX;++i) {
		context->state.key_old[i] = 0;
		context->state.key_current[i] = 0;
	}

	context->state.input_forced_exit_flag = 0;
	context->state.input_on_this_frame_flag = 0;

	/* fill */
	mac = 0;

	for(i=0;i<mouseb_count_get() && i<INPUT_MOUSE_MAX;++i) {
		for(j=0;j<mouseb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_MAX) {
				sprintf(buf, "MOUSE%d B%d", i+1, j+1);
				strncpy(input_joyname_map[mac], buf, INPUT_NAME_MAX-1);
				input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
				input_joy_map[mac].name = input_joyname_map[mac];
				input_joy_map[mac].code = DIGITAL_MOUSE_BUTTON(i, j);
				++mac;
			}
		}
	}

	for(i=0;i<joystickb_count_get() && i<INPUT_JOY_MAX;++i) {
		for(j=0;j<joystickb_stick_count_get(i) && j<INPUT_STICK_MAX;++j) {
			for(k=0;k<joystickb_stick_axe_count_get(i, j) && k<INPUT_AXE_MAX;++k) {
				if (mac+1 < INPUT_MAX) {
					sprintf(buf, "J%d %s %s -", i+1, joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					strncpy(input_joyname_map[mac], buf, INPUT_NAME_MAX-1);
					input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
					input_joy_map[mac].name = input_joyname_map[mac];
					input_joy_map[mac].code = DIGITAL_JOY(i, j, k, 0);
					++mac;
				}

				if (mac+1 < INPUT_MAX) {
					sprintf(buf, "J%d %s %s +", i+1, joystickb_stick_name_get(i, j), joystickb_stick_axe_name_get(i, j, k));
					strncpy(input_joyname_map[mac], buf, INPUT_NAME_MAX-1);
					input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
					input_joy_map[mac].name = input_joyname_map[mac];
					input_joy_map[mac].code = DIGITAL_JOY(i, j, k, 1);
					++mac;
				}
			}
		}

		for(j=0;j<joystickb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_MAX) {
				sprintf(buf, "J%d %s", i+1, joystickb_button_name_get(i, j));
				strncpy(input_joyname_map[mac], buf, INPUT_NAME_MAX-1);
				input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
				input_joy_map[mac].name = input_joyname_map[mac];
				input_joy_map[mac].code = DIGITAL_JOY_BUTTON(i, j);
				++mac;
			}
		}
	}

	/* terminate the array */
	input_joy_map[mac].name = 0;
	input_joy_map[mac].code = 0;
	input_joy_map[mac].standardcode = 0;

	/* fill in equivalences */
	for(i=0;i<mac;++i) {
		input_joy_map[i].standardcode = JOYCODE_OTHER;

		for(j=0;j<sizeof(input_joyequiv_map)/sizeof(input_joyequiv_map[0]);++j) {
			if (input_joyequiv_map[j][0] == input_joy_map[i].code) {
				input_joy_map[i].standardcode = input_joyequiv_map[j][1];
				break;
			}
		}
	}
}

static inline void input_something_pressed(struct advance_input_context* context)
{
	context->state.input_on_this_frame_flag = 1;
}

/***************************************************************************/
/* Advance interface */

static char* input_map_axe_desc[INPUT_PLAYER_AXE_MAX] = {
	"x", "y", "z", "pedal"
};

int advance_input_init(struct advance_input_context* context, adv_conf* cfg_context)
{
	unsigned i;

	conf_bool_register_default(cfg_context, "input_hotkey", 1);
	conf_bool_register_default(cfg_context, "input_steadykey", 0);
	conf_int_register_default(cfg_context, "input_idleexit", 0);

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		unsigned j;
		for(j=0;j<INPUT_PLAYER_AXE_MAX;++j) {
			char tag[32];
			char def[32];
			sprintf(tag, "input_map[%d,%s]", i, input_map_axe_desc[j]);
			sprintf(def, "joystick[%d,0,%d]", i, j);
			conf_string_register_default(cfg_context, tag, def);
		}
	}

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		char tag[32];
		char def[32];

		sprintf(tag, "input_map[%d,trakx]", i);
		sprintf(def, "mouse[%d,0]", i);
		conf_string_register_default(cfg_context, tag, def);

		sprintf(tag, "input_map[%d,traky]", i);
		sprintf(def, "mouse[%d,1]", i);
		conf_string_register_default(cfg_context, tag, def);
	}

	joystickb_reg(cfg_context, 0);
	joystickb_reg_driver_all(cfg_context);
	mouseb_reg(cfg_context, 0);
	mouseb_reg_driver_all(cfg_context);
	keyb_reg(cfg_context, 1);
	keyb_reg_driver_all(cfg_context);

	return 0;
}

void advance_input_done(struct advance_input_context* context)
{
}

int advance_input_inner_init(struct advance_input_context* context)
{
	unsigned i;

	if (joystickb_init() != 0) {
		goto err;
	}

	if (mouseb_init() != 0) {
		joystickb_done();
		goto err;
	}

	log_std(("advance:mouse: %d available\n", mouseb_count_get() ));
	for(i=0;i<mouseb_count_get();++i) {
		log_std(("advance:mouse: %d, buttons %d\n", i, mouseb_button_count_get(i)));
	}
	log_std(("advance:joystick: %d available\n", joystickb_count_get() ));
	for(i=0;i<joystickb_count_get();++i) {
		log_std(("advance:joystick: %d, buttons %d, stick %d, axes %d\n", i, joystickb_button_count_get(i), joystickb_stick_count_get(i), joystickb_stick_axe_count_get(i, 0)));
	}

	input_init(context);

	context->state.input_current_clock = os_clock();
	context->state.input_idle_clock = context->state.input_current_clock;
	context->state.input_on_this_frame_flag = 0;

	return 0;
err:
	target_err("%s\n", error_get());
	return -1;
}

void advance_input_inner_done(struct advance_input_context* context)
{
	mouseb_done();
	joystickb_done();
}

static void input_mouse_update(struct advance_input_context* context)
{
	unsigned i, j;

	for(i=0;i<mouseb_count_get() && i<INPUT_MOUSE_MAX;++i) {
		int v0, v1;

		for(j=0;j<mouseb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			context->state.mouse_button_current[i][j] = mouseb_button_get(i, j);
			if (context->state.mouse_button_current[i][j])
				input_something_pressed(context);
		}

		mouseb_pos_get(i, &v0, &v1);

		context->state.mouse_analog_current[i][0] = v0;
		context->state.mouse_analog_current[i][1] = v1;

		if (v0!=0 || v1!=0)
			input_something_pressed(context);
	}
}

static void input_joystick_update(struct advance_input_context* context)
{
	unsigned i, j, k, w;

	for(i=0;i<joystickb_count_get() && i<INPUT_JOY_MAX;++i) {
		for(j=0;j<joystickb_stick_count_get(i) && j<INPUT_STICK_MAX;++j) {
			for(k=0;k<joystickb_stick_axe_count_get(i, j) && k<INPUT_AXE_MAX;++k) {
				context->state.joystick_analog_current[i][j][k] = joystickb_stick_axe_analog_get(i, j, k);
				if (context->state.joystick_analog_current[i][j][k])
					input_something_pressed(context);

				for(w=0;w<2 && w<INPUT_DIR_MAX;++w) {
					context->state.joystick_digital_current[i][j][k][w] = joystickb_stick_axe_digital_get(i, j, k, w);
					if (context->state.joystick_digital_current[i][j][k][w])
						input_something_pressed(context);
				}
			}
		}

		for(j=0;j<joystickb_button_count_get(i) && j<INPUT_BUTTON_MAX;++j) {
			context->state.joystick_button_current[i][j] = joystickb_button_get(i, j);
			if (context->state.joystick_button_current[i][j])
				input_something_pressed(context);
		}
	}
}

void advance_input_update(struct advance_input_context* context, int is_pause)
{
	os_poll();
	keyb_poll();
	mouseb_poll();
	joystickb_poll();

	input_keyboard_update(context);
	input_mouse_update(context);
	input_joystick_update(context);

	/* forced exit due idle timeout */
	if (context->config.input_idle_limit && (context->state.input_current_clock - context->state.input_idle_clock) > context->config.input_idle_limit * OS_CLOCKS_PER_SEC) {
		context->state.input_forced_exit_flag = 1;
	}

	/* forced exit requested by the operating system */
	if (os_is_quit()) {
		context->state.input_forced_exit_flag = 1;
	}

	context->state.input_current_clock = os_clock();

	if (context->state.input_on_this_frame_flag || is_pause) {
		context->state.input_on_this_frame_flag = 0;
		context->state.input_idle_clock = context->state.input_current_clock;
	}
}

static void parse_skip(int* p, const char* s, const char* sep)
{
	while (s[*p] && strchr(sep, s[*p])!=0)
		++*p;
}

static const char* parse_token(char* c, int* p, char* s, const char* sep, const char* ignore)
{
	int v;

	while (s[*p] && strchr(ignore, s[*p])!=0)
		++*p;

	v = *p;

	while (s[*p] && strchr(sep, s[*p])==0 && strchr(ignore, s[*p])==0)
		++*p;

	while (s[*p] && strchr(ignore, s[*p])!=0)
		++*p;

	*c = s[*p];
	if (s[*p]) {
		s[*p] = 0;
		++*p;
	}

	return s + v;
}

static int parse_joystick(int* map, char* s)
{
	unsigned p;
	unsigned mac;
	unsigned i;

	/* initialize */
	for(i=0;i<INPUT_MAP_MAX;++i)
		map[i] = 0;

	/* parse */
	mac = 0;
	p = 0;
	parse_skip(&p, s, " \t");
	while (s[p]) {
		char c;
		const char* t;
		const char* v0;
		const char* v1;
		const char* v2;
		int joy, stick, axe, negate;

		t = parse_token(&c, &p, s, "[", " \t");
		if (strcmp(t, "joystick")!=0 && strcmp(t, "-joystick")!=0)
			return -1;
		if (c!='[')
			return -1;

		if (strcmp(t, "joystick")==0)
			negate = 0;
		else
			negate = 1;

		v0 = parse_token(&c, &p, s, ",", " \t");
		if (c!=',')
			return -1;

		v1 = parse_token(&c, &p, s, ",", " \t");
		if (c!=',')
			return -1;

		v2 = parse_token(&c, &p, s, "]", " \t");
		if (c!=']')
			return -1;

		joy = atoi(v0);
		stick = atoi(v1);
		axe = atoi(v2);

		if (joy < 0 || joy >= INPUT_JOY_MAX)
			return -1;
		if (stick < 0 || stick >= INPUT_STICK_MAX)
			return -1;
		if (axe < 0 || axe >= INPUT_AXE_MAX)
			return -1;

		if (mac >= INPUT_MAP_MAX)
			return -1;

		map[mac] = ANALOG_JOY(joy, stick, axe, negate);
		++mac;

		parse_skip(&p, s, " \t");
	}

	return 0;
}

static int parse_mouse(int* map, char* s)
{
	unsigned p;
	unsigned mac;
	unsigned i;

	/* initialize */
	for(i=0;i<INPUT_MAP_MAX;++i)
		map[i] = 0;

	/* parse */
	mac = 0;
	p = 0;
	parse_skip(&p, s, " \t");
	while (s[p]) {
		char c;
		const char* t;
		const char* v0;
		const char* v1;
		int mouse, axe, negate;

		t = parse_token(&c, &p, s, "[", " \t");
		if (strcmp(t, "mouse")!=0 && strcmp(t, "-mouse")!=0)
			return -1;
		if (c!='[')
			return -1;

		if (strcmp(t, "mouse")==0)
			negate = 0;
		else
			negate = 1;

		v0 = parse_token(&c, &p, s, ",", " \t");
		if (c!=',')
			return -1;

		v1 = parse_token(&c, &p, s, "]", " \t");
		if (c!=']')
			return -1;

		mouse = atoi(v0);
		axe = atoi(v1);

		if (mouse < 0 || mouse >= INPUT_MOUSE_MAX)
			return -1;
		if (axe < 0 || axe >= INPUT_AXE_MAX)
			return -1;

		if (mac >= INPUT_MAP_MAX)
			return -1;

		map[mac] = ANALOG_MOUSE(mouse, axe, negate);
		++mac;

		parse_skip(&p, s, " \t");
	}

	return 0;
}

int advance_input_config_load(struct advance_input_context* context, adv_conf* cfg_context)
{
	const char* s;
	unsigned i, j, k;

	context->config.disable_special_flag = !conf_bool_get_default(cfg_context, "input_hotkey");
	context->config.steadykey_flag = conf_bool_get_default(cfg_context, "input_steadykey");
	context->config.input_idle_limit = conf_int_get_default(cfg_context, "input_idleexit");

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		for(j=0;j<INPUT_PLAYER_AXE_MAX;++j) {
			char tag[32];
			char* d;
			sprintf(tag, "input_map[%d,%s]", i, input_map_axe_desc[j]);

			s = conf_string_get_default(cfg_context, tag);
			d = strdup(s);
			if (parse_joystick(context->config.analog_map[i][j], d)!=0) {
				free(d);
				printf("Invalid argument '%s' for option '%s'\n", s, tag);
				printf("Valid format is [-]joystick[JOYSTICK,STICK,AXE] ...\n");
				return -1;
			}
			free(d);

			log_std(("advance: input analog mapping player:%d axe:%d (%s) : ", i, j, input_map_axe_desc[j]));
			for(k=0;k<INPUT_MAP_MAX;++k) {
				unsigned v = context->config.analog_map[i][j][k];
				if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_JOY) {
					unsigned j = ANALOG_JOY_DEV_GET(v);
					unsigned s = ANALOG_JOY_STICK_GET(v);
					unsigned a = ANALOG_JOY_AXE_GET(v);
					int negate = ANALOG_JOY_NEGATE_GET(v);
					if (negate)
						log_std((" -joystick[%d,%d,%d]", j, s, a));
					else
						log_std((" joystick[%d,%d,%d]", j, s, a));
				} else {
					break;
				}
			}
			log_std(("\n"));
		}
	}

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		char tag[32];
		char* d;

		sprintf(tag, "input_map[%d,trakx]", i);
		s = conf_string_get_default(cfg_context, tag);
		d = strdup(s);
		if (parse_mouse(context->config.trakx_map[i], d)!=0) {
			free(d);
			printf("Invalid argument '%s' for option '%s'\n", s, tag);
			printf("Valid format is [-]mouse[MOUSE,AXE] ...\n");
			return -1;
		}
		free(d);

		log_std(("advance: input trakx mapping player:%d :", i));
		for(k=0;k<INPUT_MAP_MAX;++k) {
			unsigned v = context->config.trakx_map[i][k];
			if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
				unsigned m = ANALOG_MOUSE_DEV_GET(v);
				unsigned a = ANALOG_MOUSE_AXE_GET(v);
				int negate = ANALOG_MOUSE_NEGATE_GET(v);
				if (negate)
					log_std((" -mouse[%d,%d]", m, a));
				else
					log_std((" mouse[%d,%d]", m, a));
			} else {
				break;
			}
		}
		log_std(("\n"));

		sprintf(tag, "input_map[%d,traky]", i);
		s = conf_string_get_default(cfg_context, tag);
		d = strdup(s);
		if (parse_mouse(context->config.traky_map[i], d)!=0) {
			free(d);
			printf("Invalid argument '%s' for option '%s'\n", s, tag);
			printf("Valid format is [-]mouse[MOUSE,AXE] ...\n");
			return -1;
		}
		free(d);

		log_std(("advance: input traky mapping player:%d :", i));
		for(k=0;k<INPUT_MAP_MAX;++k) {
			unsigned v = context->config.traky_map[i][k];
			if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
				unsigned m = ANALOG_MOUSE_DEV_GET(v);
				unsigned a = ANALOG_MOUSE_AXE_GET(v);
				int negate = ANALOG_MOUSE_NEGATE_GET(v);
				if (negate)
					log_std((" -mouse[%d,%d]", m, a));
				else
					log_std((" mouse[%d,%d]", m, a));
			} else {
				break;
			}
		}
		log_std(("\n"));
	}

	if (joystickb_load(cfg_context) != 0) {
		return -1;
	}

	if (mouseb_load(cfg_context) != 0) {
		return -1;
	}

	if (keyb_load(cfg_context) != 0) {
		return -1;
	}

	return 0;
}

int advance_input_exit_filter(struct advance_input_context* context, struct advance_safequit_context* safequit_context, int result_memory)
{
	if (context->state.input_forced_exit_flag)
		return 2;

	advance_safequit_update(safequit_context);

	if (advance_safequit_can_exit(safequit_context)) {
		if (result_memory)
			return 2;
		else
			return 0;
	}

	if (result_memory) {
		return 1;
	}

	return 0;
}

/***************************************************************************/
/* OSD interface */

/* return a list of all available keys */
const struct KeyboardInfo* osd_get_key_list(void)
{
	return input_key_map;
}

int osd_is_key_pressed(int keycode)
{
	struct advance_input_context* context = &CONTEXT.input;

	if (input_is_key_pressed(context, keycode)) {
		input_something_pressed(context);
		return 1;
	}

	if (hardware_is_input_simulated(SIMULATE_KEY, keycode)) {
		return 1;
	}

	return 0;
}

int osd_readkey_unicode(int flush)
{
	return 0; /* no unicode support */
}

/* return a list of all available joys */
const struct JoystickInfo* osd_get_joy_list(void)
{
	return input_joy_map;
}

int osd_is_joy_pressed(int joycode)
{
	struct advance_input_context* context = &CONTEXT.input;

	unsigned type = DIGITAL_TYPE_GET(joycode);

	switch (type) {
		case DIGITAL_TYPE_JOY : {
			unsigned j = DIGITAL_JOY_DEV_GET(joycode);
			unsigned s = DIGITAL_JOY_STICK_GET(joycode);
			unsigned a = DIGITAL_JOY_AXE_GET(joycode);
			unsigned d = DIGITAL_JOY_DIR_GET(joycode);
			if (j < INPUT_JOY_MAX && s < INPUT_STICK_MAX && a < INPUT_AXE_MAX)
				return context->state.joystick_digital_current[j][s][a][d];
			break;
		}
		case DIGITAL_TYPE_JOY_BUTTON : {
			unsigned j = DIGITAL_JOY_BUTTON_DEV_GET(joycode);
			unsigned b = DIGITAL_JOY_BUTTON_BUTTON_GET(joycode);
			if (j < INPUT_JOY_MAX && b < INPUT_BUTTON_MAX)
				return context->state.joystick_button_current[j][b];
			break;
		}
		case DIGITAL_TYPE_MOUSE_BUTTON : {
			unsigned m = DIGITAL_MOUSE_BUTTON_DEV_GET(joycode);
			unsigned b = DIGITAL_MOUSE_BUTTON_BUTTON_GET(joycode);
			if (m < INPUT_MOUSE_MAX && b < INPUT_BUTTON_MAX)
				return context->state.mouse_button_current[m][b];
			break;
		}
	}

	return 0;
}

/* return a value in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player, int analog_axis[MAX_ANALOG_AXES], InputCode analogjoy_input[MAX_ANALOG_AXES])
{
	struct advance_input_context* context = &CONTEXT.input;
	unsigned i, n;

	/* the variable analogjoy_input is ignored */

	if (player < INPUT_PLAYER_MAX) {
		for(i=0;i<MAX_ANALOG_AXES;++i) {
			analog_axis[i] = 0;

			if (i < INPUT_PLAYER_AXE_MAX) {
				for(n=0;n<INPUT_MAP_MAX;++n) {
					unsigned v = context->config.analog_map[player][i][n];
					if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_JOY) {
						unsigned j = ANALOG_JOY_DEV_GET(v);
						unsigned s = ANALOG_JOY_STICK_GET(v);
						unsigned a = ANALOG_JOY_AXE_GET(v);
						int negate = ANALOG_JOY_NEGATE_GET(v);
						if (j < INPUT_JOY_MAX && s < INPUT_STICK_MAX && a < INPUT_AXE_MAX) {
							if (negate)
								analog_axis[i] -= context->state.joystick_analog_current[j][s][a];
							else
								analog_axis[i] += context->state.joystick_analog_current[j][s][a];
						}
					} else {
						break;
					}
				}
			}
		}
	} else {
		unsigned i;
		for(i=0;i<MAX_ANALOG_AXES;++i) {
			analog_axis[i] = 0;
		}
	}
}

int osd_is_joystick_axis_code(int joycode)
{
	return 0;
}

int osd_joystick_needs_calibration(void)
{
	return 1;
}

void osd_joystick_start_calibration(void)
{
	joystickb_calib_start();
}

const char* osd_joystick_calibrate_next(void)
{
	return joystickb_calib_next();
}

void osd_joystick_calibrate(void)
{
	/* nothing */
}

void osd_joystick_end_calibration(void)
{
	/* nothing */
}

void osd_trak_read(int player, int* x, int* y)
{
	struct advance_input_context* context = &CONTEXT.input;
	unsigned n;

	*x = 0;
	*y = 0;

	if (player < INPUT_PLAYER_MAX) {
		for(n=0;n<INPUT_MAP_MAX;++n) {
			unsigned v = context->config.trakx_map[player][n];
			if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
				unsigned m = ANALOG_MOUSE_DEV_GET(v);
				unsigned a = ANALOG_MOUSE_AXE_GET(v);
				int negate = ANALOG_MOUSE_NEGATE_GET(v);
				if (m < INPUT_MOUSE_MAX && a < INPUT_AXE_MAX) {
					if (negate)
						*x -= context->state.mouse_analog_current[m][a];
					else
						*x += context->state.mouse_analog_current[m][a];
				}
			} else {
				break;
			}
		}

		for(n=0;n<INPUT_MAP_MAX;++n) {
			unsigned v = context->config.traky_map[player][n];
			if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
				unsigned m = ANALOG_MOUSE_DEV_GET(v);
				unsigned a = ANALOG_MOUSE_AXE_GET(v);
				int negate = ANALOG_MOUSE_NEGATE_GET(v);
				if (ANALOG_TYPE_GET(v) == ANALOG_TYPE_MOUSE) {
					if (m < INPUT_MOUSE_MAX && a < INPUT_AXE_MAX) {
						if (negate) {
							*y -= context->state.mouse_analog_current[m][a];
						} else {
							*y += context->state.mouse_analog_current[m][a];
						}
					}
				} else {
					break;
				}
			}
		}
	}
}

void osd_lightgun_read(int player, int* deltax, int* deltay)
{
	*deltax = 0;
	*deltay = 0;
}

void osd_customize_inputport_defaults(struct ipd* defaults)
{
	/* nothing */
	(void)defaults;
}

#ifdef MESS
int osd_keyboard_disabled(void)
{
	return 0; /* TODO implement osd_keyboard_disabled */
}

int osd_trying_to_quit(void)
{
	return os_is_quit() != 0;
}

#endif

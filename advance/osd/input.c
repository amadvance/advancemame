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

#include "advance.h"
#include "mame2.h"

#include "hscript.h"
#include "conf.h"
#include "os.h"

#include <time.h>

/**************************************************************************/
/* Internals */

static struct KeyboardInfo input_key_map[] = {
	{ "A", OS_KEY_A, KEYCODE_A },
	{ "B", OS_KEY_B, KEYCODE_B },
	{ "C", OS_KEY_C, KEYCODE_C },
	{ "D", OS_KEY_D, KEYCODE_D },
	{ "E", OS_KEY_E, KEYCODE_E },
	{ "F", OS_KEY_F, KEYCODE_F },
	{ "G", OS_KEY_G, KEYCODE_G },
	{ "H", OS_KEY_H, KEYCODE_H },
	{ "I", OS_KEY_I, KEYCODE_I },
	{ "J", OS_KEY_J, KEYCODE_J },
	{ "K", OS_KEY_K, KEYCODE_K },
	{ "L", OS_KEY_L, KEYCODE_L },
	{ "M", OS_KEY_M, KEYCODE_M },
	{ "N", OS_KEY_N, KEYCODE_N },
	{ "O", OS_KEY_O, KEYCODE_O },
	{ "P", OS_KEY_P, KEYCODE_P },
	{ "Q", OS_KEY_Q, KEYCODE_Q },
	{ "R", OS_KEY_R, KEYCODE_R },
	{ "S", OS_KEY_S, KEYCODE_S },
	{ "T", OS_KEY_T, KEYCODE_T },
	{ "U", OS_KEY_U, KEYCODE_U },
	{ "V", OS_KEY_V, KEYCODE_V },
	{ "W", OS_KEY_W, KEYCODE_W },
	{ "X", OS_KEY_X, KEYCODE_X },
	{ "Y", OS_KEY_Y, KEYCODE_Y },
	{ "Z", OS_KEY_Z, KEYCODE_Z },
	{ "0", OS_KEY_0, KEYCODE_0 },
	{ "1", OS_KEY_1, KEYCODE_1 },
	{ "2", OS_KEY_2, KEYCODE_2 },
	{ "3", OS_KEY_3, KEYCODE_3 },
	{ "4", OS_KEY_4, KEYCODE_4 },
	{ "5", OS_KEY_5, KEYCODE_5 },
	{ "6", OS_KEY_6, KEYCODE_6 },
	{ "7", OS_KEY_7, KEYCODE_7 },
	{ "8", OS_KEY_8, KEYCODE_8 },
	{ "9", OS_KEY_9, KEYCODE_9 },
	{ "0PAD", OS_KEY_0_PAD, KEYCODE_0_PAD },
	{ "1PAD", OS_KEY_1_PAD, KEYCODE_1_PAD },
	{ "2PAD", OS_KEY_2_PAD, KEYCODE_2_PAD },
	{ "3PAD", OS_KEY_3_PAD, KEYCODE_3_PAD },
	{ "4PAD", OS_KEY_4_PAD, KEYCODE_4_PAD },
	{ "5PAD", OS_KEY_5_PAD, KEYCODE_5_PAD },
	{ "6PAD", OS_KEY_6_PAD, KEYCODE_6_PAD },
	{ "7PAD", OS_KEY_7_PAD, KEYCODE_7_PAD },
	{ "8PAD", OS_KEY_8_PAD, KEYCODE_8_PAD },
	{ "9PAD", OS_KEY_9_PAD, KEYCODE_9_PAD },
	{ "F1", OS_KEY_F1, KEYCODE_F1 },
	{ "F2", OS_KEY_F2, KEYCODE_F2 },
	{ "F3", OS_KEY_F3, KEYCODE_F3 },
	{ "F4", OS_KEY_F4, KEYCODE_F4 },
	{ "F5", OS_KEY_F5, KEYCODE_F5 },
	{ "F6", OS_KEY_F6, KEYCODE_F6 },
	{ "F7", OS_KEY_F7, KEYCODE_F7 },
	{ "F8", OS_KEY_F8, KEYCODE_F8 },
	{ "F9", OS_KEY_F9, KEYCODE_F9 },
	{ "F10", OS_KEY_F10, KEYCODE_F10 },
	{ "F11", OS_KEY_F11, KEYCODE_F11 },
	{ "F12", OS_KEY_F12, KEYCODE_F12 },
	{ "ESC", OS_KEY_ESC, KEYCODE_ESC },
	{ "~", OS_KEY_BACKQUOTE, KEYCODE_TILDE },
	{ "-", OS_KEY_MINUS, KEYCODE_MINUS },
	{ "=", OS_KEY_EQUALS, KEYCODE_EQUALS },
	{ "BKSPACE", OS_KEY_BACKSPACE, KEYCODE_BACKSPACE },
	{ "TAB", OS_KEY_TAB, KEYCODE_TAB },
	{ "[", OS_KEY_OPENBRACE, KEYCODE_OPENBRACE },
	{ "]", OS_KEY_CLOSEBRACE, KEYCODE_CLOSEBRACE },
	{ "ENTER", OS_KEY_ENTER, KEYCODE_ENTER },
	{ ";", OS_KEY_SEMICOLON, KEYCODE_COLON },
	{ ":", OS_KEY_QUOTE, KEYCODE_QUOTE },
	{ "\\", OS_KEY_BACKSLASH, KEYCODE_BACKSLASH },
	{ "<", OS_KEY_LESS, KEYCODE_BACKSLASH2 },
	{ ", ", OS_KEY_COMMA, KEYCODE_COMMA },
	{ ".", OS_KEY_PERIOD, KEYCODE_STOP },
	{ "/", OS_KEY_SLASH, KEYCODE_SLASH },
	{ "SPACE", OS_KEY_SPACE, KEYCODE_SPACE },
	{ "INS", OS_KEY_INSERT, KEYCODE_INSERT },
	{ "DEL", OS_KEY_DEL, KEYCODE_DEL },
	{ "HOME", OS_KEY_HOME, KEYCODE_HOME },
	{ "END", OS_KEY_END, KEYCODE_END },
	{ "PGUP", OS_KEY_PGUP, KEYCODE_PGUP },
	{ "PGDN", OS_KEY_PGDN, KEYCODE_PGDN },
	{ "LEFT", OS_KEY_LEFT, KEYCODE_LEFT },
	{ "RIGHT", OS_KEY_RIGHT, KEYCODE_RIGHT },
	{ "UP", OS_KEY_UP, KEYCODE_UP },
	{ "DOWN", OS_KEY_DOWN, KEYCODE_DOWN },
	{ "/PAD", OS_KEY_SLASH_PAD, KEYCODE_SLASH_PAD },
	{ "*PAD", OS_KEY_ASTERISK, KEYCODE_ASTERISK },
	{ "-PAD", OS_KEY_MINUS_PAD, KEYCODE_MINUS_PAD },
	{ "+PAD", OS_KEY_PLUS_PAD, KEYCODE_PLUS_PAD },
	{ ".PAD", OS_KEY_PERIOD_PAD, KEYCODE_DEL_PAD },
	{ "ENTERPAD", OS_KEY_ENTER_PAD, KEYCODE_ENTER_PAD },
	{ "PRTSCR", OS_KEY_PRTSCR, KEYCODE_PRTSCR },
	{ "PAUSE", OS_KEY_PAUSE, KEYCODE_PAUSE },
	{ "LSHIFT", OS_KEY_LSHIFT, KEYCODE_LSHIFT },
	{ "RSHIFT", OS_KEY_RSHIFT, KEYCODE_RSHIFT },
	{ "LCTRL", OS_KEY_LCONTROL, KEYCODE_LCONTROL },
	{ "RCTRL", OS_KEY_RCONTROL, KEYCODE_RCONTROL },
	{ "ALT", OS_KEY_ALT, KEYCODE_LALT },
	{ "ALTGR", OS_KEY_ALTGR, KEYCODE_RALT },
	{ "LWIN", OS_KEY_LWIN, KEYCODE_OTHER },
	{ "RWIN", OS_KEY_RWIN, KEYCODE_OTHER },
	{ "MENU", OS_KEY_MENU, KEYCODE_OTHER },
	{ "SCRLOCK", OS_KEY_SCRLOCK, KEYCODE_SCRLOCK },
	{ "NUMLOCK", OS_KEY_NUMLOCK, KEYCODE_NUMLOCK },
	{ "CAPSLOCK", OS_KEY_CAPSLOCK, KEYCODE_CAPSLOCK },
	{ 0, 0, 0 }
};

/*
 *
 * T - Type
 * D - Device number
 * S - Stick number
 * A - Axe number
 * D - Direction
 * B - Button number
 */

#define CODE_TYPE_JOYPOS 0 /* Joy axe - DAAASSSDDDTT */
#define CODE_TYPE_JOYBUTTON 1 /* Joy button - BBBBDDDTT */
#define CODE_TYPE_MOUSEBUTTON 2 /* Mouse button - BBBBDDDTT */
#define CODE_TYPE_TRAK 3 /* Track - AAADDDTT */

#define CODE_TYPE_GET(i) ((i) & 0x3)
#define CODE_JOYPOS_DEV_GET(i) (((i) >> 2) & 0x7)
#define CODE_JOYPOS_STICK_GET(i) (((i) >> 5) & 0x7)
#define CODE_JOYPOS_AXE_GET(i) (((i) >> 8) & 0x7)
#define CODE_JOYPOS_DIR_GET(i) (((i) >> 11) & 0x1)
#define CODE_JOYPOS_BUTTON_GET(i) (((i) >> 5) & 0xF)
#define CODE_MOUSEBUTTON_DEV_GET(i) (((i) >> 2) & 0x7)
#define CODE_MOUSEBUTTON_BUTTON_GET(i) (((i) >> 5) & 0xF)
#define CODE_TRAK_DEV_GET(i) (((i) >> 2) & 0x7)
#define CODE_TRAK_AXE_GET(i) (((i) >> 5) & 0x7)

#define CODE_JOY_MAX 8
#define CODE_STICK_MAX 8
#define CODE_AXE_MAX 8
#define CODE_DIR_MAX 2
#define CODE_MOUSE_MAX 8
#define CODE_BUTTON_MAX 16

#define CODE_JOY_STICK_AXE(joy,stick,axe) (CODE_TYPE_JOYPOS | (joy) << 2 | (stick) << 5 | (axe) << 8)
#define CODE_JOY_STICK_AXE_DIR(joy,stick,axe,dir) (CODE_TYPE_JOYPOS | (joy) << 2 | (stick) << 5 | (axe) << 8 | (dir) << 11)
#define CODE_JOYBUTTON(joy,button) (CODE_TYPE_JOYBUTTON | (joy) << 2 | (button) << 5)
#define CODE_MOUSE_BUTTON(mouse,button) (CODE_TYPE_MOUSEBUTTON | (mouse) << 2 | (button) << 5)
#define CODE_TRAK(dev,axe) (CODE_TYPE_TRAK | (dev) << 2 | (axe) << 5)

/* Max number of joystick different input */
#define INPUT_MAX 512

/* Max input name */
#define INPUT_NAME_MAX 48

static struct JoystickInfo input_joy_map[INPUT_MAX] = {
	/* will be filled later */
	{ 0, 0, 0 } /* end of table */
};

/* will be used to store names for the above */
static char input_joyname_map[INPUT_MAX][INPUT_NAME_MAX];

static int input_joyequiv_map[][2] = {
	{ CODE_JOY_STICK_AXE_DIR(0,0,0,1), JOYCODE_1_LEFT },
	{ CODE_JOY_STICK_AXE_DIR(0,0,0,0), JOYCODE_1_RIGHT },
	{ CODE_JOY_STICK_AXE_DIR(0,0,1,1), JOYCODE_1_UP },
	{ CODE_JOY_STICK_AXE_DIR(0,0,1,0), JOYCODE_1_DOWN },
	{ CODE_JOYBUTTON(0,0), JOYCODE_1_BUTTON1 },
	{ CODE_JOYBUTTON(0,1), JOYCODE_1_BUTTON2 },
	{ CODE_JOYBUTTON(0,2), JOYCODE_1_BUTTON3 },
	{ CODE_JOYBUTTON(0,3), JOYCODE_1_BUTTON4 },
	{ CODE_JOYBUTTON(0,4), JOYCODE_1_BUTTON5 },
	{ CODE_JOYBUTTON(0,5), JOYCODE_1_BUTTON6 },
	{ CODE_JOY_STICK_AXE_DIR(1,0,0,1), JOYCODE_2_LEFT },
	{ CODE_JOY_STICK_AXE_DIR(1,0,0,0), JOYCODE_2_RIGHT },
	{ CODE_JOY_STICK_AXE_DIR(1,0,1,1), JOYCODE_2_UP },
	{ CODE_JOY_STICK_AXE_DIR(1,0,1,0), JOYCODE_2_DOWN },
	{ CODE_JOYBUTTON(1,0), JOYCODE_2_BUTTON1 },
	{ CODE_JOYBUTTON(1,1), JOYCODE_2_BUTTON2 },
	{ CODE_JOYBUTTON(1,2), JOYCODE_2_BUTTON3 },
	{ CODE_JOYBUTTON(1,3), JOYCODE_2_BUTTON4 },
	{ CODE_JOYBUTTON(1,4), JOYCODE_2_BUTTON5 },
	{ CODE_JOYBUTTON(1,5), JOYCODE_2_BUTTON6 },
	{ CODE_JOY_STICK_AXE_DIR(2,0,0,1), JOYCODE_3_LEFT },
	{ CODE_JOY_STICK_AXE_DIR(2,0,0,0), JOYCODE_3_RIGHT },
	{ CODE_JOY_STICK_AXE_DIR(2,0,1,1), JOYCODE_3_UP },
	{ CODE_JOY_STICK_AXE_DIR(2,0,1,0), JOYCODE_3_DOWN },
	{ CODE_JOYBUTTON(2,0), JOYCODE_3_BUTTON1 },
	{ CODE_JOYBUTTON(2,1), JOYCODE_3_BUTTON2 },
	{ CODE_JOYBUTTON(2,2), JOYCODE_3_BUTTON3 },
	{ CODE_JOYBUTTON(2,3), JOYCODE_3_BUTTON4 },
	{ CODE_JOYBUTTON(2,4), JOYCODE_3_BUTTON5 },
	{ CODE_JOYBUTTON(2,5), JOYCODE_3_BUTTON6 },
	{ CODE_JOY_STICK_AXE_DIR(3,0,0,1), JOYCODE_4_LEFT },
	{ CODE_JOY_STICK_AXE_DIR(3,0,0,0), JOYCODE_4_RIGHT },
	{ CODE_JOY_STICK_AXE_DIR(3,0,1,1), JOYCODE_4_UP },
	{ CODE_JOY_STICK_AXE_DIR(3,0,1,0), JOYCODE_4_DOWN },
	{ CODE_JOYBUTTON(3,0), JOYCODE_4_BUTTON1 },
	{ CODE_JOYBUTTON(3,1), JOYCODE_4_BUTTON2 },
	{ CODE_JOYBUTTON(3,2), JOYCODE_4_BUTTON3 },
	{ CODE_JOYBUTTON(3,3), JOYCODE_4_BUTTON4 },
	{ CODE_JOYBUTTON(3,4), JOYCODE_4_BUTTON5 },
	{ CODE_JOYBUTTON(3,5), JOYCODE_4_BUTTON6 }
};

/*
 * Since the keyboard controller is slow, it is not capable of reporting multiple
 * key presses fast enough. We have to delay them in order not to lose special moves
 * tied to simultaneous button presses.
 */
static void input_keyboard_update(struct advance_input_context* context)
{
	if (context->config.steadykey_flag) {
		unsigned char last[OS_KEY_MAX];

		os_key_all_get(last);

		assert( sizeof(last) == sizeof(context->state.key_current) );
		assert( sizeof(last) == sizeof(context->state.key_old) );

		if (memcmp(last,context->state.key_old,sizeof(last))==0) {
			/* if keyboard state is stable, copy it over */
			memcpy(context->state.key_current,last,sizeof(last));
		} else {
			/* save the new copy */
			memcpy(context->state.key_old,last,sizeof(last));
		}

	} else {
		os_key_all_get(context->state.key_current);
	}
}

static __inline__ int input_is_key_pressed(struct advance_input_context* context, int keycode)
{
	if (keycode >= OS_KEY_MAX)
		return 0;

	return context->state.key_current[keycode];
}

static void input_init_joystick(void)
{
	unsigned i,j,k;
	unsigned mac;
	char buf[256];

	mac = 0;

	for(i=0;i<os_mouse_count_get() && i<CODE_MOUSE_MAX;++i) {
		for(j=0;j<os_mouse_button_count_get(i) && j<CODE_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_MAX) {
				sprintf(buf,"MOUSE%d B%d",i+1,j+1);
				strncpy(input_joyname_map[mac],buf,INPUT_NAME_MAX-1);
				input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
				input_joy_map[mac].name = input_joyname_map[mac];
				input_joy_map[mac].code = CODE_MOUSE_BUTTON(i,j);
				++mac;
			}
		}
	}

	for(i=0;i<os_joy_count_get() && i<CODE_JOY_MAX;++i) {
		for(j=0;j<os_joy_stick_count_get(i) && j<CODE_STICK_MAX;++j) {
			for(k=0;k<os_joy_stick_axe_count_get(i,j) && k<CODE_AXE_MAX;++k) {
				if (mac+1 < INPUT_MAX) {
					sprintf(buf,"J%d %s %s -", i+1, os_joy_stick_name_get(i,j), os_joy_stick_axe_name_get(i,j,k));
					strncpy(input_joyname_map[mac],buf,INPUT_NAME_MAX-1);
					input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
					input_joy_map[mac].name = input_joyname_map[mac];
					input_joy_map[mac].code = CODE_JOY_STICK_AXE_DIR(i,j,k,0);
					++mac;
				}

				if (mac+1 < INPUT_MAX) {
					sprintf(buf,"J%d %s %s +", i+1, os_joy_stick_name_get(i,j), os_joy_stick_axe_name_get(i,j,k));
					strncpy(input_joyname_map[mac],buf,INPUT_NAME_MAX-1);
					input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
					input_joy_map[mac].name = input_joyname_map[mac];
					input_joy_map[mac].code = CODE_JOY_STICK_AXE_DIR(i,j,k,1);
					++mac;
				}
			}
		}

		for(j=0;j<os_joy_button_count_get(i) && j<CODE_BUTTON_MAX;++j) {
			if (mac+1 < INPUT_MAX) {
				sprintf(buf,"J%d %s",i+1, os_joy_button_name_get(i,j));
				strncpy(input_joyname_map[mac],buf,INPUT_NAME_MAX-1);
				input_joyname_map[mac][INPUT_NAME_MAX-1] = 0;
				input_joy_map[mac].name = input_joyname_map[mac];
				input_joy_map[mac].code = CODE_JOYBUTTON(i,j);
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

static __inline__ int input_is_joy_pressed(int joycode)
{
	unsigned type = CODE_TYPE_GET(joycode);

	switch (type) {
		case CODE_TYPE_JOYPOS : {
			unsigned j = CODE_JOYPOS_DEV_GET(joycode);
			unsigned s = CODE_JOYPOS_STICK_GET(joycode);
			unsigned a = CODE_JOYPOS_AXE_GET(joycode);
			unsigned d = CODE_JOYPOS_DIR_GET(joycode);
			if (j < os_joy_count_get() && s < os_joy_stick_count_get(j) && a < os_joy_stick_axe_count_get(j,s))
				return os_joy_stick_axe_digital_get(j,s,a,d);
			break;
		}
		case CODE_TYPE_JOYBUTTON : {
			unsigned j = CODE_JOYPOS_DEV_GET(joycode);
			unsigned b = CODE_JOYPOS_BUTTON_GET(joycode);
			if (j < os_joy_count_get() && b < os_joy_button_count_get(j))
				return os_joy_button_get(j,b);
			break;
		}
		case CODE_TYPE_MOUSEBUTTON : {
			unsigned m = CODE_MOUSEBUTTON_DEV_GET(joycode);
			unsigned b = CODE_MOUSEBUTTON_BUTTON_GET(joycode);
			if (m < os_mouse_count_get() && b < os_mouse_button_count_get(m))
				return os_mouse_button_get(m,b);
			break;
		}
	}

	return 0;
}

static __inline__ void input_something_pressed(struct advance_input_context* context) {
	context->state.input_on_this_frame_flag = 1;
}

/***************************************************************************/
/* Advance interface */

static char* input_map_axe_desc[INPUT_PLAYER_AXE_MAX] = {
	"x", "y", "z", "pedal"
};

int advance_input_init(struct advance_input_context* context, struct conf_context* cfg_context) {
	unsigned i;

	conf_bool_register_default(cfg_context, "input_hotkey", 1);
	conf_bool_register_default(cfg_context, "input_steadykey", 0);
	conf_int_register_default(cfg_context, "input_idleexit", 0);

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		unsigned j;
		for(j=0;j<INPUT_PLAYER_AXE_MAX;++j) {
			char tag[32];
			char def[32];
			sprintf(tag,"input_map[%d,%s]",i,input_map_axe_desc[j]);
			sprintf(def,"joystick[%d,0,%d]",i,j);
			conf_string_register_default(cfg_context, tag, def);
		}
	}

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		char tag[32];
		char def[32];

		sprintf(tag,"input_map[%d,trakx]",i);
		sprintf(def,"mouse[%d,0]",i);
		conf_string_register_default(cfg_context, tag, def);

		sprintf(tag,"input_map[%d,traky]",i);
		sprintf(def,"mouse[%d,1]",i);
		conf_string_register_default(cfg_context, tag, def);
	}

	conf_string_register_default(cfg_context, "device_joystick", "none");
	conf_string_register_default(cfg_context, "device_mouse", "none");
	conf_string_register_default(cfg_context, "device_keyboard", "auto");

	return 0;
}

void advance_input_done(struct advance_input_context* context) {
}

int advance_input_inner_init(struct advance_input_context* context)
{
	unsigned i;

	if (os_joy_init(context->config.joystick_id) != 0)
		return -1;

	if (os_mouse_init(context->config.mouse_id) != 0) {
		os_joy_done();
		return -1;
	}

	log_std(("advance:mouse: %d available\n", os_mouse_count_get() ));
	for(i=0;i<os_mouse_count_get();++i) {
		log_std(("advance:mouse: %d, buttons %d\n",i,os_mouse_button_count_get(i)));
	}
	log_std(("advance:joystick: %d available\n", os_joy_count_get() ));
	if (os_joy_count_get())
		log_std(("advance:joystick: name %s, driver %s\n", os_joy_name_get(), os_joy_driver_name_get() ));
	for(i=0;i<os_joy_count_get();++i) {
		log_std(("advance:joystick: %d, buttons %d, stick %d, axes %d\n",i,os_joy_button_count_get(i),os_joy_stick_count_get(i),os_joy_stick_axe_count_get(i,0)));
	}

	input_init_joystick();

	context->state.input_current_clock = os_clock();
	context->state.input_idle_clock = context->state.input_current_clock;
	context->state.input_on_this_frame_flag = 0;

	return 0;
}

void advance_input_inner_done(struct advance_input_context* context) {
	os_mouse_done();
	os_joy_done();
}

void advance_input_update(struct advance_input_context* context, int is_pause)
{
	os_poll();

	input_keyboard_update(context);

	/* forced exit due idle timeout */
	if (context->config.input_idle_limit && (context->state.input_current_clock - context->state.input_idle_clock) > context->config.input_idle_limit * OS_CLOCKS_PER_SEC) {
		context->state.input_forced_exit_flag = 1;
	}

	/* forced exit requested by the operating system */
	if (os_is_term()) {
		context->state.input_forced_exit_flag = 1;
	}

	context->state.input_current_clock = os_clock();

	if (context->state.input_on_this_frame_flag || is_pause) {
		context->state.input_on_this_frame_flag = 0;
		context->state.input_idle_clock = context->state.input_current_clock;
	}
}

int advance_input_config_load(struct advance_input_context* context, struct conf_context* cfg_context) {
	const char* s;
	int i;

	context->config.disable_special_flag = !conf_bool_get_default(cfg_context, "input_hotkey");
	context->config.steadykey_flag = conf_bool_get_default(cfg_context, "input_steadykey");
	context->config.input_idle_limit = conf_int_get_default(cfg_context, "input_idleexit");

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		unsigned j;
		for(j=0;j<INPUT_PLAYER_AXE_MAX;++j) {
			char tag[32];
			int joy,stick,axe;
			sprintf(tag,"input_map[%d,%s]",i,input_map_axe_desc[j]);
			s = conf_string_get_default(cfg_context, tag);
			if (sscanf(s, "joystick[%d,%d,%d]", &joy, &stick, &axe)!=3) {
				printf("Invalid argument '%s' for option '%s'\n",s,tag);
				printf("Valid format is joystick[JOYSTICK,STICK,AXE]\n");
				return -1;
			}

			log_std(("advance: input analog mapping player:%d axe:%d (%s) <- joy:%d, stick:%d, axe:%d\n", i, j, input_map_axe_desc[j], joy, stick, axe));
			context->config.analog_map[i][j] = CODE_JOY_STICK_AXE(joy,stick,axe);
		}
	}

	for(i=0;i<INPUT_PLAYER_MAX;++i) {
		char tag[32];
		int mouse;
		int axe;

		sprintf(tag,"input_map[%d,trakx]",i);
		s = conf_string_get_default(cfg_context, tag);
		if (sscanf(s, "mouse[%d,%d]", &mouse, &axe)!=2) {
			printf("Invalid argument '%s' for option '%s'\n",s,tag);
			printf("Valid format is mouse[MOUSE,AXE]\n");
			return -1;
		}

		log_std(("advance: input trakx mapping player:%d <- mouse:%d, axe:%d\n", i, mouse, axe));
		context->config.trakx_map[i] = CODE_TRAK(mouse,axe);

		sprintf(tag,"input_map[%d,traky]",i);
		s = conf_string_get_default(cfg_context, tag);
		if (sscanf(s, "mouse[%d,%d]", &mouse, &axe)!=2) {
			printf("Invalid argument '%s' for option '%s'\n",s,tag);
			printf("Valid format is mouse[MOUSE,AXE]\n");
			return -1;
		}

		log_std(("advance: input traky mapping player:%d <- mouse:%d, axe:%d\n", i, mouse, axe));
		context->config.traky_map[i] = CODE_TRAK(mouse,axe);
	}

	s = conf_string_get_default(cfg_context, "device_joystick");
	for (i=0;OS_JOY[i].name;++i) {
		if (strcmp(OS_JOY[i].name, s) == 0) {
			context->config.joystick_id = OS_JOY[i].id;
			break;
		}
	}
	if (!OS_JOY[i].name) {
		printf("Invalid argument '%s' for option 'device_joystick'\n",s);
		printf("Valid values are:\n");
		for (i=0;OS_JOY[i].name;++i) {
			printf("%8s %s\n", OS_JOY[i].name, OS_JOY[i].desc);
		}
		return -1;
	}

	s = conf_string_get_default(cfg_context, "device_mouse");
	for (i=0;OS_MOUSE[i].name;++i) {
		if (strcmp(OS_MOUSE[i].name, s) == 0) {
			context->config.mouse_id = OS_MOUSE[i].id;
			break;
		}
	}
	if (!OS_MOUSE[i].name) {
		printf("Invalid argument '%s' for option 'device_mouse'\n",s);
		printf("Valid values are:\n");
		for (i=0;OS_MOUSE[i].name;++i) {
			printf("%8s %s\n", OS_MOUSE[i].name, OS_MOUSE[i].desc);
		}
		return -1;
	}

	s = conf_string_get_default(cfg_context, "device_keyboard");
	for (i=0;OS_KEY[i].name;++i) {
		if (strcmp(OS_KEY[i].name, s) == 0) {
			context->config.keyboard_id = OS_KEY[i].id;
			break;
		}
	}
	if (!OS_KEY[i].name) {
		printf("Invalid argument '%s' for option 'device_keyboard'\n",s);
		printf("Valid values are:\n");
		for (i=0;OS_KEY[i].name;++i) {
			printf("%8s %s\n", OS_KEY[i].name, OS_KEY[i].desc);
		}
		return -1;
	}

	return 0;
}

int advance_input_exit_filter(struct advance_input_context* context, struct advance_safequit_context* safequit_context, int result_memory) {
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

	if (input_is_key_pressed(context,keycode)) {
		input_something_pressed(context);
		return 1;
	}

	if (hardware_is_input_simulated(SIMULATE_KEY,keycode)) {
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

	if (input_is_joy_pressed(joycode)) {

		input_something_pressed(context);
		return 1;
	} else {
		return 0;
	}
}

/* return a value in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player,int analog_axis[MAX_ANALOG_AXES], InputCode analogjoy_input[MAX_ANALOG_AXES])
{
	struct advance_input_context* context = &CONTEXT.input;

	/* the variable analogjoy_input is ignored */

	if (player < INPUT_PLAYER_MAX) {
		unsigned i;
		for(i=0;i<MAX_ANALOG_AXES;++i) {
			if (i < INPUT_PLAYER_AXE_MAX) {

				unsigned v = context->config.analog_map[player][i];
				unsigned j = CODE_JOYPOS_DEV_GET(v);
				unsigned s = CODE_JOYPOS_STICK_GET(v);
				unsigned a = CODE_JOYPOS_AXE_GET(v);

				if (j < os_joy_count_get()
					&& s < os_joy_stick_count_get(j)
					&& a < os_joy_stick_axe_count_get(j,s))
				{
					analog_axis[i] = os_joy_stick_axe_analog_get(j,s,a);
					if (analog_axis[i])
						input_something_pressed(context);
				} else {
					analog_axis[i] = 0;
				}
			} else {
				analog_axis[i] = 0;
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
	os_joy_calib_start();
}

const char* osd_joystick_calibrate_next(void)
{
	return os_joy_calib_next();
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

	*x = 0;
	*y = 0;

	if (player < INPUT_PLAYER_MAX) {
		unsigned vx = context->config.trakx_map[player];
		unsigned mx = CODE_TRAK_DEV_GET(vx);
		unsigned ax = CODE_TRAK_AXE_GET(vx);
		unsigned vy = context->config.traky_map[player];
		unsigned my = CODE_TRAK_DEV_GET(vy);
		unsigned ay = CODE_TRAK_AXE_GET(vy);

		if (mx < os_mouse_count_get() && ax < 2) {
			int v0, v1;
			os_mouse_pos_get(mx,&v0,&v1);

			if (v0 || v1)
				input_something_pressed(context);

			if (ax)
				*x = v0;
			else
				*x = v1;
		}

		if (my < os_mouse_count_get() && ay < 2) {
			int v0, v1;
			os_mouse_pos_get(my,&v0,&v1);

			if (v0 || v1)
				input_something_pressed(context);

			if (ay)
				*y = v0;
			else
				*y = v1;
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


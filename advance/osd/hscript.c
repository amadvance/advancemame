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

#include "hscript.h"
#include "script.h"

#include "os.h"
#include "target.h"
#include "keydrv.h"

#include "mame2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct hardware_script_pair {
	int condition;
	struct script_cmd* script;
	struct script_state* state;
};

struct hardware_script_state {
	int active_flag; /**< !=0 if init() called */

	/* Current script event state (==0 stopped, !=0 running) */
	int script_condition;

	/* Script source */
	char* text_map[HARDWARE_SCRIPT_MAX];

	struct hardware_script_pair map[HARDWARE_SCRIPT_MAX];

	unsigned char kdb_state;

	const char* script_text;
};

static struct hardware_script_state STATE;

/* Set the source */
void hardware_script_set(int id, const char* script) {
	free(STATE.text_map[id]);
	if (script && *script)
		STATE.text_map[id] = strdup(script);
	else
		STATE.text_map[id] = 0;
}

/* Port callback */
unsigned char script_port_read(int address) {
	if (address == 0) {
		return STATE.kdb_state;
	} else
		return target_port_get(address);
}

void script_port_write(int address,unsigned char value) {
	if (address == 0) {
		value &= 0x7;
		if (STATE.kdb_state != value) {
			unsigned led_mask = 0;
			if (value & 0x1) led_mask |= OS_LED_NUMLOCK;
			if (value & 0x2) led_mask |= OS_LED_CAPSLOCK;
			if (value & 0x4) led_mask |= OS_LED_SCROLOCK;
			os_led_set(led_mask);
			STATE.kdb_state = value;
		}
	} else {
		target_port_set(address,value);
	}
}

struct symbol {
	const char* name;
	int value;
} SYMBOL[] = {
/* IO Ports */
{ "lpt1", 0x378 },
{ "lpt2", 0x278 },
{ "lpt3", 0x3bc },
{ "kdb", 0 },
/* MAME Ports */
{ "p1_up", IPT_JOYSTICK_UP | IPF_PLAYER1 },
{ "p1_down", IPT_JOYSTICK_DOWN | IPF_PLAYER1 },
{ "p1_left", IPT_JOYSTICK_LEFT | IPF_PLAYER1 },
{ "p1_right", IPT_JOYSTICK_RIGHT | IPF_PLAYER1 },
{ "p2_up", IPT_JOYSTICK_UP | IPF_PLAYER2 },
{ "p2_down", IPT_JOYSTICK_DOWN | IPF_PLAYER2 },
{ "p2_left", IPT_JOYSTICK_LEFT | IPF_PLAYER2 },
{ "p2_right", IPT_JOYSTICK_RIGHT | IPF_PLAYER2 },
{ "p1_button1", IPT_BUTTON1 | IPF_PLAYER1 },
{ "p1_button2", IPT_BUTTON2 | IPF_PLAYER1 },
{ "p1_button3", IPT_BUTTON3 | IPF_PLAYER1 },
{ "p1_button4", IPT_BUTTON4 | IPF_PLAYER1 },
{ "p1_button5", IPT_BUTTON5 | IPF_PLAYER1 },
{ "p1_button6", IPT_BUTTON6 | IPF_PLAYER1 },
{ "p1_button7", IPT_BUTTON7 | IPF_PLAYER1 },
{ "p1_button8", IPT_BUTTON8 | IPF_PLAYER1 },
{ "p1_button9", IPT_BUTTON9 | IPF_PLAYER1 },
{ "p1_button10", IPT_BUTTON10 | IPF_PLAYER1 },
{ "p2_button1", IPT_BUTTON1 | IPF_PLAYER2 },
{ "p2_button2", IPT_BUTTON2 | IPF_PLAYER2 },
{ "p2_button3", IPT_BUTTON3 | IPF_PLAYER2 },
{ "p2_button4", IPT_BUTTON4 | IPF_PLAYER2 },
{ "p2_button5", IPT_BUTTON5 | IPF_PLAYER2 },
{ "p2_button6", IPT_BUTTON6 | IPF_PLAYER2 },
{ "p2_button7", IPT_BUTTON7 | IPF_PLAYER2 },
{ "p2_button8", IPT_BUTTON8 | IPF_PLAYER2 },
{ "p2_button9", IPT_BUTTON9 | IPF_PLAYER2 },
{ "p2_button10", IPT_BUTTON10 | IPF_PLAYER2 },
{ "start1", IPT_START1 },
{ "start2", IPT_START2 },
{ "start3", IPT_START3 },
{ "start4", IPT_START4 },
{ "coin1", IPT_COIN1 },
{ "coin2", IPT_COIN2 },
{ "coin3", IPT_COIN3 },
{ "coin4", IPT_COIN4 },
{ "service_coin1", IPT_SERVICE1 },
{ "service_coin2", IPT_SERVICE2 },
{ "service_coin3", IPT_SERVICE3 },
{ "service_coin4", IPT_SERVICE4 },
{ "service", IPT_SERVICE },
{ "tilt", IPT_TILT },
{ "ui_mode_next", IPT_UI_MODE_NEXT },
{ "ui_mode_pred", IPT_UI_MODE_PRED },
{ "ui_record_start", IPT_UI_RECORD_START },
{ "ui_record_stop", IPT_UI_RECORD_STOP },
{ "ui_turbo", IPT_UI_TURBO },
{ "ui_configure", IPT_UI_CONFIGURE },
{ "ui_on_screen_display", IPT_UI_ON_SCREEN_DISPLAY },
{ "ui_pause", IPT_UI_PAUSE },
{ "ui_reset_machine", IPT_UI_RESET_MACHINE },
{ "ui_show_gfx", IPT_UI_SHOW_GFX },
{ "ui_frameskip_dec", IPT_UI_FRAMESKIP_DEC },
{ "ui_frameskip_inc", IPT_UI_FRAMESKIP_INC },
{ "ui_throttle", IPT_UI_THROTTLE },
{ "ui_show_fps", IPT_UI_SHOW_FPS },
{ "ui_shapshot", IPT_UI_SNAPSHOT },
{ "ui_toggle_cheat", IPT_UI_TOGGLE_CHEAT },
{ "ui_up", IPT_UI_UP },
{ "ui_down", IPT_UI_DOWN },
{ "ui_left", IPT_UI_LEFT },
{ "ui_right", IPT_UI_RIGHT },
{ "ui_select", IPT_UI_SELECT },
{ "ui_cancel", IPT_UI_CANCEL },
{ "ui_pan_up", IPT_UI_PAN_UP },
{ "ui_pan_down", IPT_UI_PAN_DOWN },
{ "ui_pan_left", IPT_UI_PAN_LEFT },
{ "ui_pan_right", IPT_UI_PAN_RIGHT },
{ "ui_show_profiler", IPT_UI_SHOW_PROFILER },
{ "ui_toggle_ui", IPT_UI_TOGGLE_UI },
{ "ui_toggle_debug", IPT_UI_TOGGLE_DEBUG },
{ "ui_save_state", IPT_UI_SAVE_STATE },
{ "ui_load_state", IPT_UI_LOAD_STATE },
{ "ui_add_cheat", IPT_UI_ADD_CHEAT },
{ "ui_delete_cheat", IPT_UI_DELETE_CHEAT },
{ "ui_save_cheat", IPT_UI_SAVE_CHEAT },
{ "ui_watch_value", IPT_UI_WATCH_VALUE },
/* Keys */
{ "key_a", OS_KEY_A },
{ "key_b", OS_KEY_B },
{ "key_c", OS_KEY_C },
{ "key_d", OS_KEY_D },
{ "key_e", OS_KEY_E },
{ "key_f", OS_KEY_F },
{ "key_g", OS_KEY_G },
{ "key_h", OS_KEY_H },
{ "key_i", OS_KEY_I },
{ "key_j", OS_KEY_J },
{ "key_k", OS_KEY_K },
{ "key_l", OS_KEY_L },
{ "key_m", OS_KEY_M },
{ "key_n", OS_KEY_N },
{ "key_o", OS_KEY_O },
{ "key_p", OS_KEY_P },
{ "key_q", OS_KEY_Q },
{ "key_r", OS_KEY_R },
{ "key_s", OS_KEY_S },
{ "key_t", OS_KEY_T },
{ "key_u", OS_KEY_U },
{ "key_v", OS_KEY_V },
{ "key_w", OS_KEY_W },
{ "key_x", OS_KEY_X },
{ "key_y", OS_KEY_Y },
{ "key_z", OS_KEY_Z },
{ "key_0", OS_KEY_0 },
{ "key_1", OS_KEY_1 },
{ "key_2", OS_KEY_2 },
{ "key_3", OS_KEY_3 },
{ "key_4", OS_KEY_4 },
{ "key_5", OS_KEY_5 },
{ "key_6", OS_KEY_6 },
{ "key_7", OS_KEY_7 },
{ "key_8", OS_KEY_8 },
{ "key_9", OS_KEY_9 },
{ "key_0_pad", OS_KEY_0_PAD },
{ "key_1_pad", OS_KEY_1_PAD },
{ "key_2_pad", OS_KEY_2_PAD },
{ "key_3_pad", OS_KEY_3_PAD },
{ "key_4_pad", OS_KEY_4_PAD },
{ "key_5_pad", OS_KEY_5_PAD },
{ "key_6_pad", OS_KEY_6_PAD },
{ "key_7_pad", OS_KEY_7_PAD },
{ "key_8_pad", OS_KEY_8_PAD },
{ "key_9_pad", OS_KEY_9_PAD },
{ "key_f1", OS_KEY_F1 },
{ "key_f2", OS_KEY_F2 },
{ "key_f3", OS_KEY_F3 },
{ "key_f4", OS_KEY_F4 },
{ "key_f5", OS_KEY_F5 },
{ "key_f6", OS_KEY_F6 },
{ "key_f7", OS_KEY_F7 },
{ "key_f8", OS_KEY_F8 },
{ "key_f9", OS_KEY_F9 },
{ "key_f10", OS_KEY_F10 },
{ "key_f11", OS_KEY_F11 },
{ "key_f12", OS_KEY_F12 },
{ "key_esc", OS_KEY_ESC },
{ "key_backquote", OS_KEY_BACKQUOTE },
{ "key_minus", OS_KEY_MINUS },
{ "key_equals", OS_KEY_EQUALS },
{ "key_backspace", OS_KEY_BACKSPACE },
{ "key_tab", OS_KEY_TAB },
{ "key_openbrace", OS_KEY_OPENBRACE },
{ "key_closebrace", OS_KEY_CLOSEBRACE },
{ "key_enter", OS_KEY_ENTER },
{ "key_semicolon", OS_KEY_SEMICOLON },
{ "key_quote", OS_KEY_QUOTE },
{ "key_backslash", OS_KEY_BACKSLASH },
{ "key_less", OS_KEY_LESS },
{ "key_comma", OS_KEY_COMMA },
{ "key_period", OS_KEY_PERIOD },
{ "key_slash", OS_KEY_SLASH },
{ "key_space", OS_KEY_SPACE },
{ "key_insert", OS_KEY_INSERT },
{ "key_del", OS_KEY_DEL },
{ "key_home", OS_KEY_HOME },
{ "key_end", OS_KEY_END },
{ "key_pgup", OS_KEY_PGUP },
{ "key_pgdn", OS_KEY_PGDN },
{ "key_left", OS_KEY_LEFT },
{ "key_right", OS_KEY_RIGHT },
{ "key_up", OS_KEY_UP },
{ "key_down", OS_KEY_DOWN },
{ "key_slash_pad", OS_KEY_SLASH_PAD },
{ "key_asterisk", OS_KEY_ASTERISK },
{ "key_minus_pad", OS_KEY_MINUS_PAD },
{ "key_plus_pad", OS_KEY_PLUS_PAD },
{ "key_period_pad", OS_KEY_PERIOD_PAD },
{ "key_enter_pad", OS_KEY_ENTER_PAD },
{ "key_prtscr", OS_KEY_PRTSCR },
{ "key_pause", OS_KEY_PAUSE },
{ "key_lshift", OS_KEY_LSHIFT },
{ "key_rshift", OS_KEY_RSHIFT },
{ "key_lcontrol", OS_KEY_LCONTROL },
{ "key_rcontrol", OS_KEY_RCONTROL },
{ "key_lalt", OS_KEY_ALT },
{ "key_ralt", OS_KEY_ALTGR },
{ "key_scrlock", OS_KEY_SCRLOCK },
{ "key_numlock", OS_KEY_NUMLOCK },
{ "key_capslock", OS_KEY_CAPSLOCK },
{ "key_lwin", OS_KEY_LWIN },
{ "key_rwin", OS_KEY_RWIN },
{ "key_menu", OS_KEY_MENU },
{ 0,0 }
};

/* Evaluate a symbol */
static int script_symbol_get(union script_arg_extra argextra) {
	return argextra.value;
}

/* Check a symbol */
script_exp_op1s_evaluator* script_symbol_check(const char* sym, union script_arg_extra* argextra) {
	struct symbol* p = SYMBOL;
	while (p->name) {
		if (strcmp(sym,p->name)==0) {
			argextra->value = p->value;
			return &script_symbol_get;
		}
		++p;
	}
	return 0;
}

int script_function1_get(union script_arg_extra argextra) {
	switch (argextra.value) {
		case 0 : /* event */
			return STATE.script_condition;
	}
	return 0;
}

int script_function2_get(int arg0, union script_arg_extra argextra) {
	switch (argextra.value) {
		case 0 : /* get */
			return script_port_read(arg0);
		case 1 : { /* event */
			InputSeq* seq = input_port_type_seq(arg0);
			return seq_pressed(seq);
		}
	}
	return 0;
}

int script_function3_get(int arg0, int arg1, union script_arg_extra argextra) {
	switch (argextra.value) {
		case 0 : /* set */
			script_port_write(arg0,arg1);
			return 0;
		case 1 : /* on */
			script_port_write(arg0, script_port_read(arg0) | arg1);
			return 0;
		case 2 : /* off */
			script_port_write(arg0, script_port_read(arg0) & ~arg1);
			return 0;
		case 3 : /* toggle */
			script_port_write(arg0, script_port_read(arg0) ^ arg1);
			return 0;
		case 4 : /* simulate_event */
			hardware_simulate_input(SIMULATE_EVENT,arg0, arg1 * (SCRIPT_TIME_UNIT / 1000));
			return 0;
		case 5 : /* simulate_key */
			hardware_simulate_input(SIMULATE_KEY,arg0, arg1 * (SCRIPT_TIME_UNIT / 1000));
			return 0;
	}
	return 0;
}

script_exp_op1f_evaluator* script_function1_check(const char* sym, union script_arg_extra* argextra) {
	if (strcmp(sym,"event")==0) {
		argextra->value = 0;
		return &script_function1_get;
	}
	return 0;
}

script_exp_op2fe_evaluator* script_function2_check(const char* sym, union script_arg_extra* argextra) {
	if (strcmp(sym,"event")==0) {
		argextra->value = 0;
		return &script_function2_get;
	} else if (strcmp(sym,"get")==0) {
		argextra->value = 1;
		return &script_function2_get;
	}
	return 0;
}

script_exp_op3fee_evaluator* script_function3_check(const char* sym, union script_arg_extra* argextra) {
	(void)sym;
	if (strcmp(sym,"set")==0) {
		argextra->value = 0;
		return &script_function3_get;
	} else if (strcmp(sym,"on")==0) {
		argextra->value = 1;
		return &script_function3_get;
	} else if (strcmp(sym,"off")==0) {
		argextra->value = 2;
		return &script_function3_get;
	} else if (strcmp(sym,"toggle")==0) {
		argextra->value = 3;
		return &script_function3_get;
	} else if (strcmp(sym,"simulate_event")==0) {
		argextra->value = 4;
		return &script_function3_get;
	} else if (strcmp(sym,"simulate_key")==0) {
		argextra->value = 5;
		return &script_function3_get;
	}
	return 0;
}

/* Parse error callback */
void script_error(const char* s) {
	target_err("Error compiling the script: %s\n",STATE.script_text);
	target_err("%s\n",s);
}

int hardware_script_init(struct conf_context* context) {

	conf_string_register_default(context, "script_video", "wait(!event()); set(kdb,0);");
	conf_string_register_default(context, "script_emulation", "");
	conf_string_register_default(context, "script_play", "");
	conf_string_register_default(context, "script_led[1]", "on(kdb, 0b1); wait(!event()); off(kdb, 0b1);");
	conf_string_register_default(context, "script_led[2]", "on(kdb, 0b10); wait(!event()); off(kdb, 0b10);");
	conf_string_register_default(context, "script_led[3]", "");
	conf_string_register_default(context, "script_coin[1]", "");
	conf_string_register_default(context, "script_coin[2]", "");
	conf_string_register_default(context, "script_coin[3]", "");
	conf_string_register_default(context, "script_coin[4]", "");
	conf_string_register_default(context, "script_start[1]", "");
	conf_string_register_default(context, "script_start[2]", "");
	conf_string_register_default(context, "script_start[3]", "");
	conf_string_register_default(context, "script_start[4]", "");
	conf_string_register_default(context, "script_turbo", "while (event()) { toggle(kdb, 0b100); delay(100); } off(kdb, 0b100);");

	STATE.active_flag  = 0;

	return 0;
}

void hardware_script_done(void) {
	assert( !STATE.active_flag );
}

int hardware_script_inner_init(void) {
	int i;

	assert( !STATE.active_flag );

	for(i=0;i<HARDWARE_SCRIPT_MAX;++i) {
		if (STATE.text_map[i]) {
			STATE.script_text = STATE.text_map[i];
			STATE.map[i].script = script_parse(STATE.text_map[i]);
			if (!STATE.map[i].script)
				return -1;
			STATE.map[i].state = script_run_alloc();
		} else {
			STATE.map[i].script = 0;
			STATE.map[i].state = 0;
		}
	}

	STATE.active_flag = 1;

	return 0;
}

void hardware_script_inner_done(void) {
	int i;

	assert( STATE.active_flag );

	STATE.active_flag = 0;

	for(i=0;i<HARDWARE_SCRIPT_MAX;++i) {
		script_free(STATE.map[i].script);
		script_run_free(STATE.map[i].state);
		free(STATE.text_map[i]);
	}

	/* free the lex buffer */
	script_flush();
}

void hardware_script_abort(void) {
	if (STATE.active_flag) {
		hardware_script_terminate(HARDWARE_SCRIPT_PLAY);
		hardware_script_terminate(HARDWARE_SCRIPT_EMULATION);
		hardware_script_terminate(HARDWARE_SCRIPT_VIDEO);
	}
}

/* Start the script */
void hardware_script_start(int id) {
	if (STATE.map[id].script) {
		STATE.map[id].condition = 1;
		/* start only if not already running */
		if (script_run_end(STATE.map[id].state))
			script_run_restart(STATE.map[id].state,STATE.map[id].script);
	}
}

void hardware_script_stop(int id) {
	if (STATE.map[id].script) {
		STATE.map[id].condition = 0;
	}
}

static void hardware_script_idle_single(unsigned id, unsigned time_to_play) {
	if (STATE.map[id].script) {
		/* set the global condition flag */
		STATE.script_condition = STATE.map[id].condition;
		script_run(STATE.map[id].state,time_to_play);
	}
}

/* Use idle time */
void hardware_script_idle(unsigned time_to_play) {
	int i;
	for(i=0;i<HARDWARE_SCRIPT_MAX;++i) {
		hardware_script_idle_single(i,time_to_play);
	}
}

void hardware_script_terminate(int id) {
	int time_to_play = SCRIPT_TIME_UNIT; /* one second time to flush any delay */
	hardware_script_stop(id);
	hardware_script_idle_single(id,time_to_play);
}

int hardware_script_config_load(struct conf_context* context) {
	const char* s;

	s = conf_string_get_default(context, "script_video");
	hardware_script_set(HARDWARE_SCRIPT_VIDEO, s);

	s = conf_string_get_default(context, "script_emulation");
	hardware_script_set(HARDWARE_SCRIPT_EMULATION, s);

	s = conf_string_get_default(context, "script_play");
	hardware_script_set(HARDWARE_SCRIPT_PLAY, s);

	s = conf_string_get_default(context, "script_led[1]");
	hardware_script_set(HARDWARE_SCRIPT_LED1, s);

	s = conf_string_get_default(context, "script_led[2]");
	hardware_script_set(HARDWARE_SCRIPT_LED2, s);

	s = conf_string_get_default(context, "script_led[3]");
	hardware_script_set(HARDWARE_SCRIPT_LED3, s);

	s = conf_string_get_default(context, "script_coin[1]");
	hardware_script_set(HARDWARE_SCRIPT_COIN1, s);

	s = conf_string_get_default(context, "script_coin[2]");
	hardware_script_set(HARDWARE_SCRIPT_COIN2, s);

	s = conf_string_get_default(context, "script_coin[3]");
	hardware_script_set(HARDWARE_SCRIPT_COIN3, s);

	s = conf_string_get_default(context, "script_coin[4]");
	hardware_script_set(HARDWARE_SCRIPT_COIN4, s);

	s = conf_string_get_default(context, "script_start[1]");
	hardware_script_set(HARDWARE_SCRIPT_START1, s);

	s = conf_string_get_default(context, "script_start[2]");
	hardware_script_set(HARDWARE_SCRIPT_START2, s);

	s = conf_string_get_default(context, "script_start[3]");
	hardware_script_set(HARDWARE_SCRIPT_START3, s);

	s = conf_string_get_default(context, "script_start[4]");
	hardware_script_set(HARDWARE_SCRIPT_START4, s);

	s = conf_string_get_default(context, "script_turbo");
	hardware_script_set(HARDWARE_SCRIPT_TURBO, s);

	return 0;
}

/***************************************************************************/
/* simulate */

struct simulate SIMULATE_EVENT[SIMULATE_MAX];
struct simulate SIMULATE_KEY[SIMULATE_MAX];

void hardware_simulate_input(struct simulate* SIMULATE, int type, unsigned time_to_play) {
	int best = 0;
	int i;
	for(i=1;i<SIMULATE_MAX;++i) {
		if (SIMULATE[i].time_to_play < SIMULATE[best].time_to_play)
			best = i;
	}
	SIMULATE[best].type = type;
	SIMULATE[best].time_to_play = time_to_play;
}

void hardware_simulate_input_idle(struct simulate* SIMULATE, unsigned time_to_play) {
	int i;
	for(i=0;i<SIMULATE_MAX;++i) {
		if (SIMULATE[i].time_to_play > time_to_play)
			SIMULATE[i].time_to_play -= time_to_play;
		else {
			SIMULATE[i].time_to_play = 0;
		}
	}
}


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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
#include "key.h"
#include "target.h"
#include "keydrv.h"

#include "glue.h"

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
void hardware_script_set(int id, const char* script)
{
	free(STATE.text_map[id]);
	if (script && *script)
		STATE.text_map[id] = strdup(script);
	else
		STATE.text_map[id] = 0;
}

/* Port callback */
unsigned char script_port_read(int address)
{
	if (address == 0) {
		return STATE.kdb_state;
	} else
		return target_port_get(address);
}

void script_port_write(int address, unsigned char value)
{
	if (address == 0) {
		value &= 0x7;
		if (STATE.kdb_state != value) {
			unsigned led_mask = 0;
			if (value & 0x1) led_mask |= TARGET_LED_NUMLOCK;
			if (value & 0x2) led_mask |= TARGET_LED_CAPSLOCK;
			if (value & 0x4) led_mask |= TARGET_LED_SCROLOCK;
			target_led_set(led_mask);
			STATE.kdb_state = value;
		}
	} else {
		target_port_set(address, value);
	}
}

struct symbol {
	const char* name;
	int value;
} SYMBOL[] = {
{ "lpt1", 0x378 },
{ "lpt2", 0x278 },
{ "lpt3", 0x3bc },
{ "kdb", 0 },
{ 0, 0 }
};

/* Evaluate a symbol */
static int script_symbol_get(union script_arg_extra argextra)
{
	return argextra.value;
}

/* Check a symbol */
script_exp_op1s_evaluator* script_symbol_check(const char* sym, union script_arg_extra* argextra)
{
	struct symbol* p = SYMBOL;
	const struct mame_port* mp;

	/* check for global symbol */
	while (p->name) {
		if (strcmp(sym, p->name)==0) {
			argextra->value = p->value;
			return &script_symbol_get;
		}
		++p;
	}

	/* check for key names in the form key_NAME */
	if (memcmp(sym, "key_", 4) == 0) {
		unsigned i;
		const char* sym_name = sym + 4;
		for(i=0;i<KEYB_MAX;++i) {
			const char* name = key_name(i);
			if (name) {
				if (strcmp(sym_name, name)==0) {
					argextra->value = i;
					return &script_symbol_get;
				}
			}
		}
	}

	/* check for port names */
	mp = mame_port_list();
	while (mp->name) {
		if (strcmp(sym, mp->name)==0) {
			argextra->value = mp->port;
			return &script_symbol_get;
		}
		++mp;
	}

	return 0;
}

int script_function1_get(union script_arg_extra argextra)
{
	switch (argextra.value) {
		case 0 : /* event */
			return STATE.script_condition;
	}
	return 0;
}

int script_function2_get(int arg0, union script_arg_extra argextra)
{
	switch (argextra.value) {
		case 0 : /* get */
			return script_port_read(arg0);
		case 1 :  /* event */
			return mame_ui_port_pressed(arg0);
	}
	return 0;
}

int script_function3_get(int arg0, int arg1, union script_arg_extra argextra)
{
	switch (argextra.value) {
		case 0 : /* set */
			script_port_write(arg0, arg1);
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
			hardware_simulate_input(SIMULATE_EVENT, arg0, arg1 * (SCRIPT_TIME_UNIT / 1000));
			return 0;
		case 5 : /* simulate_key */
			hardware_simulate_input(SIMULATE_KEY, arg0, arg1 * (SCRIPT_TIME_UNIT / 1000));
			return 0;
	}
	return 0;
}

script_exp_op1f_evaluator* script_function1_check(const char* sym, union script_arg_extra* argextra)
{
	if (strcmp(sym, "event")==0) {
		argextra->value = 0;
		return &script_function1_get;
	}
	return 0;
}

script_exp_op2fe_evaluator* script_function2_check(const char* sym, union script_arg_extra* argextra)
{
	if (strcmp(sym, "event")==0) {
		argextra->value = 0;
		return &script_function2_get;
	} else if (strcmp(sym, "get")==0) {
		argextra->value = 1;
		return &script_function2_get;
	}
	return 0;
}

script_exp_op3fee_evaluator* script_function3_check(const char* sym, union script_arg_extra* argextra)
{
	(void)sym;
	if (strcmp(sym, "set")==0) {
		argextra->value = 0;
		return &script_function3_get;
	} else if (strcmp(sym, "on")==0) {
		argextra->value = 1;
		return &script_function3_get;
	} else if (strcmp(sym, "off")==0) {
		argextra->value = 2;
		return &script_function3_get;
	} else if (strcmp(sym, "toggle")==0) {
		argextra->value = 3;
		return &script_function3_get;
	} else if (strcmp(sym, "simulate_event")==0) {
		argextra->value = 4;
		return &script_function3_get;
	} else if (strcmp(sym, "simulate_key")==0) {
		argextra->value = 5;
		return &script_function3_get;
	}
	return 0;
}

/* Parse error callback */
void script_error(const char* s)
{
	target_err("Error compiling the script: '%s'.\n", STATE.script_text);
	target_err("%s\n", s);
}

int hardware_script_init(adv_conf* context)
{
	conf_string_register_default(context, "script_video", "wait(!event()); set(kdb, 0);");
	conf_string_register_default(context, "script_emulation", "");
	conf_string_register_default(context, "script_play", "");
	conf_string_register_default(context, "script_led1", "on(kdb, 0b1); wait(!event()); off(kdb, 0b1);");
	conf_string_register_default(context, "script_led2", "on(kdb, 0b10); wait(!event()); off(kdb, 0b10);");
	conf_string_register_default(context, "script_led3", "");
	conf_string_register_default(context, "script_coin1", "");
	conf_string_register_default(context, "script_coin2", "");
	conf_string_register_default(context, "script_coin3", "");
	conf_string_register_default(context, "script_coin4", "");
	conf_string_register_default(context, "script_start1", "");
	conf_string_register_default(context, "script_start2", "");
	conf_string_register_default(context, "script_start3", "");
	conf_string_register_default(context, "script_start4", "");
	conf_string_register_default(context, "script_turbo", "while (event()) { toggle(kdb, 0b100); delay(100); } off(kdb, 0b100);");
	conf_string_register_default(context, "script_safequit", "");
	conf_string_register_default(context, "script_event1", "");
	conf_string_register_default(context, "script_event2", "");
	conf_string_register_default(context, "script_event3", "");
	conf_string_register_default(context, "script_event4", "");
	conf_string_register_default(context, "script_event5", "");
	conf_string_register_default(context, "script_event6", "");

	STATE.active_flag  = 0;

	return 0;
}

void hardware_script_done(void)
{
	assert( !STATE.active_flag );
}

int hardware_script_inner_init(void)
{
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

void hardware_script_inner_done(void)
{
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

void hardware_script_abort(void)
{
	if (STATE.active_flag) {
		hardware_script_terminate(HARDWARE_SCRIPT_PLAY);
		hardware_script_terminate(HARDWARE_SCRIPT_EMULATION);
		hardware_script_terminate(HARDWARE_SCRIPT_VIDEO);
	}
}

/* Start the script */
void hardware_script_start(int id)
{
	if (STATE.map[id].script) {
		STATE.map[id].condition = 1;
		/* start only if not already running */
		if (script_run_end(STATE.map[id].state))
			script_run_restart(STATE.map[id].state, STATE.map[id].script);
	}
}

void hardware_script_stop(int id)
{
	if (STATE.map[id].script) {
		STATE.map[id].condition = 0;
	}
}

static void hardware_script_idle_single(unsigned id, unsigned time_to_play)
{
	if (STATE.map[id].script) {
		/* set the global condition flag */
		STATE.script_condition = STATE.map[id].condition;
		script_run(STATE.map[id].state, time_to_play);
	}
}

/* Use idle time */
void hardware_script_idle(unsigned time_to_play)
{
	int i;
	for(i=0;i<HARDWARE_SCRIPT_MAX;++i) {
		hardware_script_idle_single(i, time_to_play);
	}
}

void hardware_script_terminate(int id)
{
	int time_to_play = SCRIPT_TIME_UNIT; /* one second time to flush any delay */
	hardware_script_stop(id);
	hardware_script_idle_single(id, time_to_play);
}

int hardware_script_config_load(adv_conf* context)
{
	const char* s;

	s = conf_string_get_default(context, "script_video");
	hardware_script_set(HARDWARE_SCRIPT_VIDEO, s);

	s = conf_string_get_default(context, "script_emulation");
	hardware_script_set(HARDWARE_SCRIPT_EMULATION, s);

	s = conf_string_get_default(context, "script_play");
	hardware_script_set(HARDWARE_SCRIPT_PLAY, s);

	s = conf_string_get_default(context, "script_led1");
	hardware_script_set(HARDWARE_SCRIPT_LED1, s);

	s = conf_string_get_default(context, "script_led2");
	hardware_script_set(HARDWARE_SCRIPT_LED2, s);

	s = conf_string_get_default(context, "script_led3");
	hardware_script_set(HARDWARE_SCRIPT_LED3, s);

	s = conf_string_get_default(context, "script_coin1");
	hardware_script_set(HARDWARE_SCRIPT_COIN1, s);

	s = conf_string_get_default(context, "script_coin2");
	hardware_script_set(HARDWARE_SCRIPT_COIN2, s);

	s = conf_string_get_default(context, "script_coin3");
	hardware_script_set(HARDWARE_SCRIPT_COIN3, s);

	s = conf_string_get_default(context, "script_coin4");
	hardware_script_set(HARDWARE_SCRIPT_COIN4, s);

	s = conf_string_get_default(context, "script_start1");
	hardware_script_set(HARDWARE_SCRIPT_START1, s);

	s = conf_string_get_default(context, "script_start2");
	hardware_script_set(HARDWARE_SCRIPT_START2, s);

	s = conf_string_get_default(context, "script_start3");
	hardware_script_set(HARDWARE_SCRIPT_START3, s);

	s = conf_string_get_default(context, "script_start4");
	hardware_script_set(HARDWARE_SCRIPT_START4, s);

	s = conf_string_get_default(context, "script_turbo");
	hardware_script_set(HARDWARE_SCRIPT_TURBO, s);

	s = conf_string_get_default(context, "script_safequit");
	hardware_script_set(HARDWARE_SCRIPT_SAFEQUIT, s);

	s = conf_string_get_default(context, "script_event1");
	hardware_script_set(HARDWARE_SCRIPT_EVENT1, s);

	s = conf_string_get_default(context, "script_event2");
	hardware_script_set(HARDWARE_SCRIPT_EVENT2, s);

	s = conf_string_get_default(context, "script_event3");
	hardware_script_set(HARDWARE_SCRIPT_EVENT3, s);

	s = conf_string_get_default(context, "script_event4");
	hardware_script_set(HARDWARE_SCRIPT_EVENT4, s);

	s = conf_string_get_default(context, "script_event5");
	hardware_script_set(HARDWARE_SCRIPT_EVENT5, s);

	s = conf_string_get_default(context, "script_event6");
	hardware_script_set(HARDWARE_SCRIPT_EVENT6, s);

	return 0;
}

/***************************************************************************/
/* simulate */

struct simulate SIMULATE_EVENT[SIMULATE_MAX];
struct simulate SIMULATE_KEY[SIMULATE_MAX];

void hardware_simulate_input(struct simulate* SIMULATE, int type, unsigned time_to_play)
{
	int best = 0;
	int i;
	for(i=1;i<SIMULATE_MAX;++i) {
		if (SIMULATE[i].time_to_play < SIMULATE[best].time_to_play)
			best = i;
	}
	SIMULATE[best].type = type;
	SIMULATE[best].time_to_play = time_to_play;
}

void hardware_simulate_input_idle(struct simulate* SIMULATE, unsigned time_to_play)
{
	int i;
	for(i=0;i<SIMULATE_MAX;++i) {
		if (SIMULATE[i].time_to_play > time_to_play)
			SIMULATE[i].time_to_play -= time_to_play;
		else {
			SIMULATE[i].time_to_play = 0;
		}
	}
}


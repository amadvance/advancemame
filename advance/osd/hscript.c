/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

#include "hscript.h"
#include "script.h"
#include "glue.h"
#include "emu.h"

#include "advance.h"

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

	char info_desc_buffer[256];
	char info_manufacturer_buffer[256];
	char info_year_buffer[256];
	char info_throttle_buffer[256];

	unsigned seed; /**< Random seed. */
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
	unsigned led_bit[] = {
		KEYB_LED_NUML,
		KEYB_LED_CAPSL,
		KEYB_LED_SCROLLL,
		KEYB_LED_COMPOSE,
		KEYB_LED_KANA,
		KEYB_LED_SLEEP,
		KEYB_LED_SUSPEND,
		KEYB_LED_MUTE,
		KEYB_LED_MISC,
		0
	};

	if (address == 0) {
		if (STATE.kdb_state != value) {
			unsigned i;
			unsigned led_mask = 0;

			for(i=0;led_bit[i] != 0;++i) {
				if ((value & (1 << i)) != 0) {
					led_mask |= led_bit[i];
				}
			}

			keyb_led_set(0, led_mask);

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
{ "lpt1", 0x378 }, /* LEGACY Removed in 0.87 */
{ "lpt2", 0x278 }, /* LEGACY Removed in 0.87 */
{ "lpt3", 0x3bc }, /* LEGACY Removed in 0.87 */
{ "kdb", 0 }, /* LEGACY Removed in 0.87 */
{ 0, 0 }
};

/* Evaluate a constant */
static struct script_value* script_constant_get(union script_arg_extra argextra)
{
	return script_value_alloc_num(argextra.value);
}

/* Evaluate a text variable */
static struct script_value* script_text_get(union script_arg_extra argextra)
{
	switch (argextra.value) {
	case 0 :
		return script_value_alloc_text(STATE.info_desc_buffer);
	case 1 :
		return script_value_alloc_text(STATE.info_manufacturer_buffer);
	case 2 :
		return script_value_alloc_text(STATE.info_year_buffer);
	case 3 :
		return script_value_alloc_text(STATE.info_throttle_buffer);
	}

	return script_value_alloc_num(0);
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
			return &script_constant_get;
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
					return &script_constant_get;
				}
			}
		}
	}

	/* check for port names */
	mp = mame_port_list();
	while (mp->name) {
		if (strcmp(sym, mp->name)==0) {
			argextra->value = mp->port;
			return &script_constant_get;
		}
		++mp;
	}

	if (strcmp(sym, "info_desc")==0) {
		argextra->value = 0;
		return &script_text_get;
	}
	if (strcmp(sym, "info_manufacturer")==0) {
		argextra->value = 1;
		return &script_text_get;
	}
	if (strcmp(sym, "info_year")==0) {
		argextra->value = 2;
		return &script_text_get;
	}
	if (strcmp(sym, "info_throttle")==0) {
		argextra->value = 3;
		return &script_text_get;
	}

	return 0;
}

static struct script_value* script_function1_get(union script_arg_extra argextra)
{
	int r;

	switch (argextra.value) {
	case 0 : /* event */
		r = STATE.script_condition;
		break;
	default :
		r = 0;
		break;
	}

	return script_value_alloc_num(r);
}

static struct script_value* script_function2_get(struct script_value* varg0, union script_arg_extra argextra)
{
	int arg0 = script_value_free_num(varg0);
	int r;

	switch (argextra.value) {
	case 0 :  /* event */
		r = mame_ui_port_pressed(arg0);
		break;
	case 1 : /* get */
		r = script_port_read(arg0);
		break;
	case 4 : /* rand */
		if (arg0 > 0) {
			/* use an internal pseudo random number generator */
			STATE.seed = STATE.seed * 1103515245 + 12345;
			r = (STATE.seed / 65536) % arg0;
		} else {
			r = -1;
		}
		break;
	default :
		r = -1;
		break;
	}

	return script_value_alloc_num(r);
}

static struct script_value* script_function2t_get(struct script_value* varg0, union script_arg_extra argextra)
{
	int r;

	switch (argextra.value) {
	case 2 : /* log */
		if (varg0->type == SCRIPT_VALUE_NUM) {
			log_std(("script: %d\n", varg0->value.num));
			r = 0;
		} else if (varg0->type == SCRIPT_VALUE_TEXT) {
			log_std(("script: %s\n", varg0->value.text));
			r = 0;
		} else {
			log_std(("ERROR:script: invalid type\n"));
			r = -1;
		}
		break;
	case 3 : /* msg */
		if (varg0->type == SCRIPT_VALUE_NUM) {
			advance_global_message(&CONTEXT.global, "%d", varg0->value.num);
			r = 0;
		} else if (varg0->type == SCRIPT_VALUE_TEXT) {
			advance_global_message(&CONTEXT.global, "%s", varg0->value.text);
			r = 0;
		} else {
			log_std(("ERROR:script: invalid type\n"));
			r = -1;
		}
		break;
	case 5 : /* system */
		if (varg0->type == SCRIPT_VALUE_TEXT) {
			r = advance_global_script(&CONTEXT.global, varg0->value.text);
		} else {
			log_std(("ERROR:script: invalid type\n"));
			r = -1;
		}
		break;
	default :
		r = -1;
		break;
	}

	script_value_free(varg0);

	return script_value_alloc_num(r);
}

static struct script_value* script_function3_get(struct script_value* varg0, struct script_value* varg1, union script_arg_extra argextra)
{
	int arg0 = script_value_free_num(varg0);
	int arg1 = script_value_free_num(varg1);
	int r;

	switch (argextra.value) {
	case 0 : /* set */
		script_port_write(arg0, arg1);
		r = 0;
		break;
	case 1 : /* on */
		script_port_write(arg0, script_port_read(arg0) | arg1);
		r = 0;
		break;
	case 2 : /* off */
		script_port_write(arg0, script_port_read(arg0) & ~arg1);
		r = 0;
		break;
	case 3 : /* toggle */
		script_port_write(arg0, script_port_read(arg0) ^ arg1);
		r = 0;
		break;
	case 4 : /* simulate_event */
		hardware_simulate_input(SIMULATE_EVENT, arg0, arg1 * (SCRIPT_TIME_UNIT / 1000));
		r = 0;
		break;
	case 5 : /* simulate_key */
		hardware_simulate_input(SIMULATE_KEY, arg0, arg1 * (SCRIPT_TIME_UNIT / 1000));
		r = 0;
		break;
	default :
		r = -1;
		break;
	}

	return script_value_alloc_num(r);
}

static struct script_value* script_function3t_get(struct script_value* varg0, struct script_value* varg1, union script_arg_extra argextra)
{
	int arg0 = script_value_free_num(varg0);
	int r;

	switch (argextra.value) {
	case 6 : /* lcd */
		if (varg1->type == SCRIPT_VALUE_NUM) {
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%d", varg1->value.num);
			advance_global_lcd(&CONTEXT.global, arg0, buffer);
			r = 0;
		} else if (varg1->type == SCRIPT_VALUE_TEXT) {
			advance_global_lcd(&CONTEXT.global, arg0, varg1->value.text);
			r = 0;
		} else {
			log_std(("ERROR:script: invalid type\n"));
			r = -1;
		}
		break;
	default :
		r = -1;
		break;
	}

	script_value_free(varg1);

	return script_value_alloc_num(r);
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
	} else if (strcmp(sym, "log")==0) {
		argextra->value = 2;
		return &script_function2t_get;
	} else if (strcmp(sym, "msg")==0) {
		argextra->value = 3;
		return &script_function2t_get;
	} else if (strcmp(sym, "rand")==0) {
		argextra->value = 4;
		return &script_function2_get;
	} else if (strcmp(sym, "system")==0) {
		argextra->value = 5;
		return &script_function2t_get;
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
	} else if (strcmp(sym, "lcd")==0) {
		argextra->value = 6;
		return &script_function3t_get;
	}
	return 0;
}

/* Parse error callback */
void script_error(const char* s)
{
	target_err("Error compiling the script: '%s', %s\n", STATE.script_text, s);
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
	conf_string_register_default(context, "script_event7", "");
	conf_string_register_default(context, "script_event8", "");
	conf_string_register_default(context, "script_event9", "");
	conf_string_register_default(context, "script_event10", "");
	conf_string_register_default(context, "script_event11", "");
	conf_string_register_default(context, "script_event12", "");
	conf_string_register_default(context, "script_event13", "");
	conf_string_register_default(context, "script_event14", "");

	STATE.active_flag  = 0;

	STATE.info_desc_buffer[0] = 0;
	STATE.info_manufacturer_buffer[0] = 0;
	STATE.info_year_buffer[0] = 0;
	STATE.info_throttle_buffer[0] = 0;

	STATE.seed = time(0);

	return 0;
}

void hardware_script_done(void)
{
	assert(!STATE.active_flag);
}

int hardware_script_inner_init(void)
{
	int i;

	assert(!STATE.active_flag);

	for(i=0;i<HARDWARE_SCRIPT_MAX;++i) {
		if (STATE.text_map[i]) {
			STATE.script_text = STATE.text_map[i];
			STATE.map[i].script = script_parse(STATE.text_map[i]);
			if (!STATE.map[i].script) {
				return -1;
			}
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

	assert(STATE.active_flag);

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

	s = conf_string_get_default(context, "script_event7");
	hardware_script_set(HARDWARE_SCRIPT_EVENT7, s);

	s = conf_string_get_default(context, "script_event8");
	hardware_script_set(HARDWARE_SCRIPT_EVENT8, s);

	s = conf_string_get_default(context, "script_event9");
	hardware_script_set(HARDWARE_SCRIPT_EVENT9, s);

	s = conf_string_get_default(context, "script_event10");
	hardware_script_set(HARDWARE_SCRIPT_EVENT10, s);

	s = conf_string_get_default(context, "script_event11");
	hardware_script_set(HARDWARE_SCRIPT_EVENT11, s);

	s = conf_string_get_default(context, "script_event12");
	hardware_script_set(HARDWARE_SCRIPT_EVENT12, s);

	s = conf_string_get_default(context, "script_event13");
	hardware_script_set(HARDWARE_SCRIPT_EVENT13, s);

	s = conf_string_get_default(context, "script_event14");
	hardware_script_set(HARDWARE_SCRIPT_EVENT14, s);

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

void hardware_script_info(const char* desc, const char* manufacturer, const char* year, const char* throttle)
{
	if (desc)
		sncpy(STATE.info_desc_buffer, sizeof(STATE.info_desc_buffer), desc);
	if (manufacturer)
		sncpy(STATE.info_manufacturer_buffer, sizeof(STATE.info_manufacturer_buffer), manufacturer);
	if (year)
		sncpy(STATE.info_year_buffer, sizeof(STATE.info_year_buffer), year);
	if (throttle)
		sncpy(STATE.info_throttle_buffer, sizeof(STATE.info_throttle_buffer), throttle);
}


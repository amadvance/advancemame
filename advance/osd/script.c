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

#include "script.h"

#include "advance.h"

struct script_exp* script_exp_alloc(void)
{
	return malloc(sizeof(struct script_exp));
}

void script_exp_free(struct script_exp* exp)
{
	switch (script_exp_type_get(exp->type)) {
		case SCRIPT_EXP_TYPE_2EE :
			script_exp_free(exp->data.op2ee.arg0);
			script_exp_free(exp->data.op2ee.arg1);
			break;
		case SCRIPT_EXP_TYPE_1E :
			script_exp_free(exp->data.op1e.arg0);
			break;
		case SCRIPT_EXP_TYPE_1V :
			break;
		case SCRIPT_EXP_TYPE_1S :
			break;
		case SCRIPT_EXP_TYPE_1T :
			free(exp->data.op1t.arg0);
			break;
		case SCRIPT_EXP_TYPE_1F :
			break;
		case SCRIPT_EXP_TYPE_2FE :
			script_exp_free(exp->data.op2fe.arg1);
			break;
		case SCRIPT_EXP_TYPE_3FEE :
			script_exp_free(exp->data.op3fee.arg1);
			script_exp_free(exp->data.op3fee.arg2);
			break;
	}
	free(exp);
}

struct script_exp* script_exp_make_op1v(int type, int arg0)
{
	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op1v.arg0 = arg0;
	return exp;
}

struct script_exp* script_exp_make_op1s(int type, const char* arg0)
{
	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op1s.eval = script_symbol_check(arg0, &exp->data.op1s.argextra);
	if (!exp->data.op1s.eval) {
		char buffer[128];
		script_exp_free(exp);
		snprintf(buffer, sizeof(buffer), "Unknown symbol %s", arg0);
		script_error(buffer);
		return 0;
	}
	return exp;
}

struct script_exp* script_exp_make_op1t(int type, const char* arg0)
{
	char* d;
	const char* i;
	const char* begin;
	const char* end;

	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op1t.arg0 = malloc(strlen(arg0) + 1);

	/* the argument always starts and ends with " */
	assert(strlen(arg0) >= 2 && arg0[0] == '"' && arg0[strlen(arg0)-1] == '"');

	begin = arg0 + 1;
	end = begin + strlen(begin) - 1;
	d = exp->data.op1t.arg0;

	i = begin;
	while (i != end) {
		switch (i[0]) {
		case '\\' :
			if (i+1<end) {
				switch (i[1]) {
				case 'a' :
					*d++ = '\a';
					i += 2;
					break;
				case 'b' :
					*d++ = '\b';
					i += 2;
					break;
				case 'f' :
					*d++ = '\f';
					i += 2;
					break;
				case 'n' :
					*d++ = '\n';
					i += 2;
					break;
				case 'r' :
					*d++ = '\r';
					i += 2;
					break;
				case 't' :
					*d++ = '\t';
					i += 2;
					break;
				case 'v' :
					*d++ = '\v';
					i += 2;
					break;
				default:
					*d++ = i[0];
					*d++ = i[1];
					i += 2;
					break;
				}
			} else {
				*d++ = i[0];
				i += 1;
			}
			break;
		default:
			*d++ = i[0];
			i += 1;
			break;
		}
	}

	*d = 0;

	return exp;
}

struct script_exp* script_exp_make_op1e(int type, struct script_exp* arg0)
{
	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op1e.arg0 = arg0;
	return exp;
}

struct script_exp* script_exp_make_op2ee(int type, struct script_exp* arg0, struct script_exp* arg1)
{
	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op2ee.arg0 = arg0;
	exp->data.op2ee.arg1 = arg1;
	return exp;
}

struct script_exp* script_exp_make_op1f(int type, const char* arg0)
{
	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op1f.eval = script_function1_check(arg0, &exp->data.op1f.argextra);
	if (!exp->data.op1f.eval) {
		char buffer[128];
		script_exp_free(exp);
		snprintf(buffer, sizeof(buffer), "Unknown function %s", arg0);
		script_error(buffer);
		return 0;
	}
	return exp;
}

struct script_exp* script_exp_make_op2fe(int type, const char* arg0, struct script_exp* arg1)
{
	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op2fe.arg1 = arg1;
	exp->data.op2fe.eval = script_function2_check(arg0, &exp->data.op2fe.argextra);
	if (!exp->data.op2fe.eval) {
		char buffer[128];
		script_exp_free(exp);
		snprintf(buffer, sizeof(buffer), "Unknown function %s", arg0);
		script_error(buffer);
		return 0;
	}
	return exp;
}

struct script_exp* script_exp_make_op3fee(int type, const char* arg0, struct script_exp* arg1, struct script_exp* arg2)
{
	struct script_exp* exp = script_exp_alloc();
	exp->type = type;
	exp->data.op3fee.arg1 = arg1;
	exp->data.op3fee.arg2 = arg2;
	exp->data.op3fee.eval = script_function3_check(arg0, &exp->data.op3fee.argextra);
	if (!exp->data.op3fee.eval) {
		char buffer[128];
		script_exp_free(exp);
		snprintf(buffer, sizeof(buffer), "Unknown function %s", arg0);
		script_error(buffer);
		return 0;
	}
	return exp;
}

struct script_value* script_value_alloc(int type)
{
	struct script_value* p = malloc(sizeof(struct script_value));
	p->type = type;
	return p;
}

void script_value_free(struct script_value* p)
{
	switch (p->type) {
	case SCRIPT_VALUE_NUM :
		break;
	case SCRIPT_VALUE_TEXT :
		free(p->value.text);
		break;
	}
	free(p);
}

struct script_value* script_value_alloc_num(int v)
{
	struct script_value* p = script_value_alloc(SCRIPT_VALUE_NUM);
	p->value.num = v;
	return p;
}

struct script_value* script_value_alloc_text(const char* v)
{
	struct script_value* p = script_value_alloc(SCRIPT_VALUE_TEXT);
	p->value.text = strdup(v);
	return p;
}

int script_value_free_num(struct script_value* p)
{
	int v;
	if (p->type == SCRIPT_VALUE_NUM) {
		v = p->value.num;
	} else {
		v = 0;
	}
	script_value_free(p);
	return v;
}

static struct script_value* script_value_add(struct script_value* a, struct script_value* b)
{
	if (a->type == SCRIPT_VALUE_TEXT && b->type == SCRIPT_VALUE_TEXT) {
		struct script_value* p = script_value_alloc(SCRIPT_VALUE_TEXT);
		unsigned la = strlen(a->value.text);
		unsigned lb = strlen(b->value.text);
		char* s = malloc(la + lb);

		memcpy(s, a->value.text, la);
		memcpy(s + la, b->value.text, lb);
		s[la + lb] = 0;

		p->value.text = s;

		return p;
	} else {
		return script_value_alloc_num(script_value_free_num(a) + script_value_free_num(b));
	}
}

struct script_value* script_evaluate(const struct script_exp* exp)
{
	switch (exp->type) {
		case SCRIPT_EXP_VALUE :
			return script_value_alloc_num(exp->data.op1v.arg0);
		case SCRIPT_EXP_VARIABLE :
			return exp->data.op1s.eval(exp->data.op1s.argextra);
		case SCRIPT_EXP_TEXT :
			return script_value_alloc_text(exp->data.op1t.arg0);
		case SCRIPT_EXP_F0 :
			return exp->data.op1f.eval(exp->data.op1f.argextra);
		case SCRIPT_EXP_F1 :
			return exp->data.op2fe.eval(script_evaluate(exp->data.op2fe.arg1), exp->data.op2fe.argextra);
		case SCRIPT_EXP_F2 :
			return exp->data.op3fee.eval(script_evaluate(exp->data.op3fee.arg1), script_evaluate(exp->data.op3fee.arg2), exp->data.op3fee.argextra);
		case SCRIPT_EXP_EXPRESSION :
			return script_evaluate(exp->data.op1e.arg0);
		case SCRIPT_EXP_NOT :
			return script_value_alloc_num(~script_value_free_num(script_evaluate(exp->data.op1e.arg0)));
		case SCRIPT_EXP_LNOT :
			return script_value_alloc_num(!script_value_free_num(script_evaluate(exp->data.op1e.arg0)));
		case SCRIPT_EXP_ADD :
			return script_value_add(script_evaluate(exp->data.op2ee.arg0), script_evaluate(exp->data.op2ee.arg1));
		case SCRIPT_EXP_SUB :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) - script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_AND :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) & script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_OR :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) | script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_XOR :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) ^ script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_L :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) < script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_G :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) > script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_E :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) == script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_LE :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) <= script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_GE :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) >= script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_SL :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) << script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_SR :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) >> script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_LOR :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) || script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		case SCRIPT_EXP_LAND :
			return script_value_alloc_num(script_value_free_num(script_evaluate(exp->data.op2ee.arg0)) && script_value_free_num(script_evaluate(exp->data.op2ee.arg1)));
		default:
			assert(0);
	}

	return script_value_alloc_num(0);
}

struct script_cmd* script_cmd_alloc(void)
{
	struct script_cmd* cmd = malloc(sizeof(struct script_cmd));
	cmd->next = 0;
	return cmd;
}

/* Free the space allocated for the script */
void script_free(struct script_cmd* script)
{
	while (script) {
		struct script_cmd* next;
		switch (script_cmd_type_get(script->type)) {
			case SCRIPT_CMD_TYPE_1C :
				script_free(script->data.op1c.arg0);
				break;
			case SCRIPT_CMD_TYPE_1E :
				script_exp_free(script->data.op1e.arg0);
				break;
			case SCRIPT_CMD_TYPE_1ED :
				script_exp_free(script->data.op1ed.arg0);
				break;
			case SCRIPT_CMD_TYPE_2EC :
				script_exp_free(script->data.op2ec.arg0);
				script_free(script->data.op2ec.arg1);
				break;
			case SCRIPT_CMD_TYPE_2ECD :
				script_exp_free(script->data.op2ecd.arg0);
				script_free(script->data.op2ecd.arg1);
				break;
		}
		next = script->next;
		free(script);
		script = next;
	}
}

/***************************************************************************/
/* Parse */

struct script_cmd* script_cmd_make_op2sc(const char* tag, struct script_cmd* arg0)
{
	if (strcmp(tag, "loop")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_LOOP;
		cmd->data.op1c.arg0 = arg0;
		return cmd;
	} else if (strcmp(tag, "inner")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_INNER;
		cmd->data.op1c.arg0 = arg0;
		return cmd;
	} else {
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "Unknown operation %s", tag);
		script_error(buffer);
		return 0;
	}
}

struct script_cmd* script_cmd_make_op2cc(struct script_cmd* arg0, struct script_cmd* arg1)
{
	struct script_cmd* last = arg0;
	while (last->next)
		last = last->next;
	last->next = arg1;
	return arg0;
}

struct script_cmd* script_cmd_make_op2se(const char* arg0, struct script_exp* arg1)
{
	if (strcmp(arg0, "wait")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_WAIT;
		cmd->data.op1e.arg0 = arg1;
		return cmd;
	} else if (strcmp(arg0, "delay")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_DELAY;
		cmd->data.op1ed.arg0 = arg1;
		cmd->data.op1ed.value_set = 0;
		cmd->data.op1ed.value = 0;
		return cmd;
	} else if (strcmp(arg0, "evaluate")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_EVALUATE;
		cmd->data.op1e.arg0 = arg1;
		return cmd;
	} else {
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "Unknown operation %s", arg0);
		script_error(buffer);
		return 0;
	}
}

struct script_cmd* script_cmd_make_op1c(struct script_cmd* arg0)
{
	return arg0;
}

struct script_cmd* script_cmd_make_op3sec(const char* arg0, struct script_exp* arg1, struct script_cmd* arg2)
{
	if (strcmp(arg0, "while")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_WHILE;
		cmd->data.op2ec.arg0 = arg1;
		cmd->data.op2ec.arg1 = arg2;
		return cmd;
	} else if (strcmp(arg0, "repeat")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_REPEAT;
		cmd->data.op2ecd.arg0 = arg1;
		cmd->data.op2ecd.arg1 = arg2;
		cmd->data.op2ecd.value_set = 0;
		cmd->data.op2ecd.value = 0;
		return cmd;
	} else if (strcmp(arg0, "if")==0) {
		struct script_cmd* cmd = script_cmd_alloc();
		cmd->type = SCRIPT_CMD_IF;
		cmd->data.op2ec.arg0 = arg1;
		cmd->data.op2ec.arg1 = arg2;
		return cmd;
	} else {
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "Unknown command %s", arg0);
		script_error(buffer);
		return 0;
	}
}

/***************************************************************************/
/* Stack */

inline int script_run_cursor_empty(const struct script_state* state)
{
	return state->cursor_mac == 0;
}

inline struct script_cmd* script_run_cursor_get(struct script_state* state)
{
	return state->cursor_map[state->cursor_mac-1];
}

inline void script_run_cursor_pop(struct script_state* state)
{
	--state->cursor_mac;
}

inline void script_run_cursor_set(struct script_state* state, struct script_cmd* cmd)
{
	state->cursor_map[state->cursor_mac-1] = cmd;
}

inline void script_run_cursor_push(struct script_state* state, struct script_cmd* cmd)
{
	++state->cursor_mac;
	state->cursor_map[state->cursor_mac-1] = cmd;
}

/***************************************************************************/
/* Commands */

void script_run_next(struct script_state* state)
{
	/* next statement */
	script_run_cursor_set(state, script_run_cursor_get(state)->next);
	/* skip if terminated unrolling the stack */
	while (!script_run_cursor_empty(state) && !script_run_cursor_get(state)) {
		script_run_cursor_pop(state);
	}
}

void script_run_exp(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	struct script_value* value = script_evaluate(cursor->data.op1e.arg0);
	script_value_free(value);
	script_run_next(state);
}

void script_run_wait(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	struct script_value* value = script_evaluate(cursor->data.op1e.arg0);
	int condition = script_value_free_num(value);
	if (condition) {
		script_run_next(state);
	} else {
		state->time_to_play = 0;
	}
}

void script_run_delay(struct script_state* state)
{
	unsigned unit = SCRIPT_TIME_UNIT / SCRIPT_DELAY_UNIT;

	struct script_cmd* cursor = script_run_cursor_get(state);
	if (!cursor->data.op1ed.value_set) {
		cursor->data.op1ed.value_set = 1;
		cursor->data.op1ed.value = script_value_free_num(script_evaluate(cursor->data.op1ed.arg0));
	}

	if (state->time_to_play < cursor->data.op1ed.value * unit) {
		cursor->data.op1ed.value -= state->time_to_play / unit;
		state->time_to_play = state->time_to_play % unit;
	} else {
		state->time_to_play -= cursor->data.op1ed.value * unit;
		cursor->data.op1ed.value_set = 0;
		script_run_next(state);
	}
}

void script_run_repeat(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	if (!cursor->data.op2ecd.value_set) {
		cursor->data.op2ecd.value_set = 1;
		cursor->data.op2ecd.value = script_value_free_num(script_evaluate(cursor->data.op2ecd.arg0));
	}
	if (cursor->data.op2ecd.value) {
		--cursor->data.op2ecd.value;
		script_run_cursor_push(state, cursor->data.op2ecd.arg1);
	} else {
		cursor->data.op2ecd.value_set = 0;
		script_run_next(state);
	}
}

void script_run_loop(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	script_run_cursor_push(state, cursor->data.op1c.arg0);
}

void script_run_inner(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	script_run_cursor_set(state, cursor->next); /* set the next, also if is 0 */
	script_run_cursor_push(state, cursor->data.op1c.arg0);
}

void script_run_if(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	struct script_value* value = script_evaluate(cursor->data.op2ec.arg0);
	int condition = script_value_free_num(value);
	if (condition) {
		script_run_cursor_set(state, cursor->next); /* set the next, also if is 0 */
		script_run_cursor_push(state, cursor->data.op2ec.arg1);
	} else {
		script_run_next(state);
	}
}

void script_run_while(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	struct script_value* value = script_evaluate(cursor->data.op2ec.arg0);
	int condition = script_value_free_num(value);
	if (condition) {
		script_run_cursor_push(state, cursor->data.op2ec.arg1);
	} else {
		script_run_next(state);
	}
}

/***************************************************************************/
/* Run */

/* Start a script */
void script_run_restart(struct script_state* state, struct script_cmd* cmd)
{
	state->cursor_mac = 1;
	state->cursor_map[0] = cmd;
	state->time_to_play = 0;
}

struct script_state* script_run_alloc(void)
{
	struct script_state* state = malloc(sizeof(struct script_state));
	state->cursor_mac = 0;
	state->time_to_play = 0;
	return state;
}

void script_run_free(struct script_state* state)
{
	free(state);
}

/* Play a step of the script */
static void script_run_step(struct script_state* state)
{
	struct script_cmd* cursor = script_run_cursor_get(state);
	switch (cursor->type) {
		case SCRIPT_CMD_EVALUATE :
			script_run_exp(state);
		break;
		case SCRIPT_CMD_WAIT :
			script_run_wait(state);
		break;
		case SCRIPT_CMD_DELAY :
			script_run_delay(state);
		break;
		case SCRIPT_CMD_LOOP :
			script_run_loop(state);
		break;
		case SCRIPT_CMD_INNER :
			script_run_inner(state);
		break;
		case SCRIPT_CMD_WHILE :
			script_run_while(state);
		break;
		case SCRIPT_CMD_IF :
			script_run_if(state);
		break;
		case SCRIPT_CMD_REPEAT :
			script_run_repeat(state);
		break;
		default:
			assert(0);
	}
}

int script_run_end(const struct script_state* state)
{
	return script_run_cursor_empty(state);
}

/* Play the script for the given time */
void script_run(struct script_state* state, unsigned time_to_play)
{
	unsigned unit = SCRIPT_TIME_UNIT / SCRIPT_DELAY_UNIT;

	state->time_to_play += time_to_play;
	while (!script_run_cursor_empty(state) && state->time_to_play > unit) {
		script_run_step(state);
	}
}


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

#ifndef __SCRIPT_H
#define __SCRIPT_H

/***************************************************************************/
/* Value */

#define SCRIPT_VALUE_NUM 0x00
#define SCRIPT_VALUE_TEXT 0x01

union script_value_arg {
	char* text;
	int num;
};

struct script_value {
	int type;
	union script_value_arg value;
};

void script_value_free(struct script_value* p);
int script_value_free_num(struct script_value* p);
struct script_value* script_value_alloc_num(int v);
struct script_value* script_value_alloc_text(const char* v);

/***************************************************************************/
/* Expression */

union script_arg_extra {
	void* ptr;
	int value;
};

struct script_exp;

struct script_exp_op1e {
	struct script_exp* arg0; /* expression */
};

struct script_exp_op2ee {
	struct script_exp* arg0; /* expression */
	struct script_exp* arg1; /* expression */
};

struct script_exp_op1v {
	int arg0; /* value */
};

typedef struct script_value* (script_exp_op1s_evaluator)(union script_arg_extra argextra);

struct script_exp_op1s {
	union script_arg_extra argextra; /* extra value for the evaluator */
	script_exp_op1s_evaluator* eval; /* evaluator */
};

struct script_exp_op1t {
	char* arg0; /* text */
};

typedef struct script_value* (script_exp_op1f_evaluator)(union script_arg_extra argextra);

struct script_exp_op1f {
	union script_arg_extra argextra; /* extra value for the evaluator */
	script_exp_op1f_evaluator* eval; /* evaluator */
};

typedef struct script_value* (script_exp_op2fe_evaluator)(struct script_value* arg1, union script_arg_extra argextra);

struct script_exp_op2fe {
	struct script_exp* arg1; /* expression */
	union script_arg_extra argextra; /* extra value for the evaluator */
	script_exp_op2fe_evaluator* eval; /* evaluator */
};

typedef struct script_value* (script_exp_op3fee_evaluator)(struct script_value* arg1, struct script_value* arg2, union script_arg_extra argextra);

struct script_exp_op3fee {
	struct script_exp* arg1; /* expression */
	struct script_exp* arg2; /* expression */
	union script_arg_extra argextra; /* extra value for the evaluator */
	script_exp_op3fee_evaluator* eval; /* evaluator */
};

union script_exp_data {
	struct script_exp_op2ee op2ee;
	struct script_exp_op1e op1e;
	struct script_exp_op1v op1v;
	struct script_exp_op1s op1s;
	struct script_exp_op1t op1t;
	struct script_exp_op1f op1f;
	struct script_exp_op2fe op2fe;
	struct script_exp_op3fee op3fee;
};

/* Script type */
#define SCRIPT_EXP_TYPE_2EE 0x100
#define SCRIPT_EXP_TYPE_1E 0x200
#define SCRIPT_EXP_TYPE_1V 0x300
#define SCRIPT_EXP_TYPE_1S 0x400
#define SCRIPT_EXP_TYPE_1F 0x500
#define SCRIPT_EXP_TYPE_2FE 0x600
#define SCRIPT_EXP_TYPE_3FEE 0x700
#define SCRIPT_EXP_TYPE_1T 0x800
#define SCRIPT_EXP_TYPE_MASK 0xF00

static inline int script_exp_type_get(unsigned type)
{
	return type & SCRIPT_EXP_TYPE_MASK;
}

/* Script expression */
#define SCRIPT_EXP_VALUE (0x00 | SCRIPT_EXP_TYPE_1V) /* value */
#define SCRIPT_EXP_VARIABLE (0x01 | SCRIPT_EXP_TYPE_1S) /* symbol */
#define SCRIPT_EXP_F0 (0x02 | SCRIPT_EXP_TYPE_1F) /* function 0 arg */
#define SCRIPT_EXP_F1 (0x03 | SCRIPT_EXP_TYPE_2FE) /* function 1 arg */
#define SCRIPT_EXP_F2 (0x04 | SCRIPT_EXP_TYPE_3FEE) /* function 2 arg */
#define SCRIPT_EXP_EXPRESSION (0x05 | SCRIPT_EXP_TYPE_1E) /* expression */
#define SCRIPT_EXP_NOT (0x06 | SCRIPT_EXP_TYPE_1E)
#define SCRIPT_EXP_LNOT (0x7 | SCRIPT_EXP_TYPE_1E)
#define SCRIPT_EXP_ADD (0x08 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_SUB (0x09 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_AND (0x0a | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_OR (0x0b | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_XOR (0x0c | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_L (0x0d | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_G (0x0e | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_E (0x0f | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_LE (0x10 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_GE (0x11 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_SL (0x12 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_SR (0x13 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_LOR (0x14 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_LAND (0x15 | SCRIPT_EXP_TYPE_2EE)
#define SCRIPT_EXP_TEXT (0x16 | SCRIPT_EXP_TYPE_1T) /* text */

struct script_exp {
	int type;
	union script_exp_data data;
};

struct script_value* script_evaluate(const struct script_exp* exp);

/* Symbol callback */
script_exp_op1s_evaluator* script_symbol_check(const char* sym, union script_arg_extra* argextra);
script_exp_op1f_evaluator* script_function1_check(const char* sym, union script_arg_extra* argextra);
script_exp_op2fe_evaluator* script_function2_check(const char* sym, union script_arg_extra* argextra);
script_exp_op3fee_evaluator* script_function3_check(const char* sym, union script_arg_extra* argextra);

/***************************************************************************/
/* Parse */

struct script_exp* script_exp_make_op1v(int type, int arg0);
struct script_exp* script_exp_make_op1s(int type, const char* arg0);
struct script_exp* script_exp_make_op1t(int type, const char* arg0);
struct script_exp* script_exp_make_op1e(int type, struct script_exp* arg0);
struct script_exp* script_exp_make_op2ee(int type, struct script_exp* arg0, struct script_exp* arg1);
struct script_exp* script_exp_make_op1f(int type, const char* arg0);
struct script_exp* script_exp_make_op2fe(int type, const char* arg0, struct script_exp* arg1);
struct script_exp* script_exp_make_op3fee(int type, const char* arg0, struct script_exp* arg1, struct script_exp* arg2);

/***************************************************************************/
/* Commands */

/* Script type */
#define SCRIPT_CMD_TYPE_1C 0x100
#define SCRIPT_CMD_TYPE_1E 0x200
#define SCRIPT_CMD_TYPE_1ED 0x300
#define SCRIPT_CMD_TYPE_2EC 0x400
#define SCRIPT_CMD_TYPE_2ECD 0x500
#define SCRIPT_CMD_TYPE_MASK 0xF00

static inline int script_cmd_type_get(unsigned type)
{
	return type & SCRIPT_CMD_TYPE_MASK;
}

/* Script commands */
#define SCRIPT_CMD_WAIT (0x01 | SCRIPT_CMD_TYPE_1E)
#define SCRIPT_CMD_LOOP (0x02 | SCRIPT_CMD_TYPE_1C)
#define SCRIPT_CMD_INNER (0x03 | SCRIPT_CMD_TYPE_1C)
#define SCRIPT_CMD_EVALUATE (0x04 | SCRIPT_CMD_TYPE_1E)
#define SCRIPT_CMD_WHILE (0x05 | SCRIPT_CMD_TYPE_2EC)
#define SCRIPT_CMD_IF (0x06 | SCRIPT_CMD_TYPE_2EC)
#define SCRIPT_CMD_DELAY (0x07 | SCRIPT_CMD_TYPE_1ED)
#define SCRIPT_CMD_REPEAT (0x08 | SCRIPT_CMD_TYPE_2ECD)

/* Commands data */
struct script_cmd_op1e {
	struct script_exp* arg0;
};

struct script_cmd_op1ed {
	struct script_exp* arg0;
	int value;
	int value_set;
};

struct script_cmd_op1c {
	struct script_cmd* arg0;
};

struct script_cmd_op2ec {
	struct script_exp* arg0;
	struct script_cmd* arg1;
};

struct script_cmd_op2ecd {
	struct script_exp* arg0;
	struct script_cmd* arg1;
	int value;
	int value_set;
};

union script_cmd_data {
	struct script_cmd_op1e op1e;
	struct script_cmd_op1ed op1ed;
	struct script_cmd_op1c op1c;
	struct script_cmd_op2ec op2ec;
	struct script_cmd_op2ecd op2ecd;
};

/***************************************************************************/
/* Script */

struct script_cmd {
	struct script_cmd* next; /* ==0 if none */
	int type;
	union script_cmd_data data;
};

/***************************************************************************/
/* Parse */

struct script_cmd* script_cmd_make_op1c(struct script_cmd* arg0);
struct script_cmd* script_cmd_make_op2sc(const char* arg0, struct script_cmd* arg1);
struct script_cmd* script_cmd_make_op2se(const char* arg0, struct script_exp* arg1);
struct script_cmd* script_cmd_make_op2cc(struct script_cmd* arg0, struct script_cmd* arg1);
struct script_cmd* script_cmd_make_op3sec(const char* arg0, struct script_exp* arg1, struct script_cmd* arg2);

/***************************************************************************/
/* State */

#define SCRIPT_STATE_STACK_MAX 16

struct script_state {
	struct script_cmd* cursor_map[SCRIPT_STATE_STACK_MAX];
	unsigned cursor_mac;
	unsigned time_to_play;
};

/* Port callback */
unsigned char script_port_read(int address);
void script_port_write(int address, unsigned char value);

/* Error callback */
void script_error(const char* s);

void script_flush(void);
struct script_cmd* script_parse(const char* text);
void script_free(struct script_cmd* script);

struct script_state* script_run_alloc(void);
void script_run_free(struct script_state* state);
void script_run_restart(struct script_state* state, struct script_cmd* cmd);
int script_run_end(const struct script_state* state);

/* Unit time (1 second) for the idle call */
#define SCRIPT_TIME_UNIT 1000000

/* Unit time (1 second) for the script delay() call */
#define SCRIPT_DELAY_UNIT 1000

void script_run(struct script_state* state, unsigned time_to_play);

#endif

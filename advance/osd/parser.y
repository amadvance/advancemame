%{
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

#include <stdlib.h>
#include <string.h>

#include "script.h"
#include "portable.h"

#define YYERROR_VERBOSE

int yylex(void);

void yyerror(const char* s)
{
	script_error(s);
}

static struct script_cmd* script_result;

%}

%union {
	int val;
	const char* str;
	const char* text;
	struct script_exp* exp;
	struct script_cmd* cmd;
}

%token_table

%left OP_LAND OP_LOR
%left '&' '|' '^'
%left '<' '>' OP_E OP_LE OP_GE
%left OP_SL OP_SR
%left '+' '-'
%left '~' '!'

%token WAIT
%token DELAY
%token <val> VAL
%token <str> STRING
%token <text> TEXT
%type <cmd> cmd cmd_list script
%type <exp> exp

%%

script: cmd_list {
		script_result = $1;
		YYACCEPT;
	}
;

cmd_list: cmd_list cmd {
		$$ = script_cmd_make_op2cc($1, $2);
		if (!$$)
			YYERROR;
	}
	| cmd {
		$$ = $1; 
	}
;

exp: VAL {
		$$ = script_exp_make_op1v(SCRIPT_EXP_VALUE,$1);
		if (!$$) YYERROR;
	}
	| STRING {
		$$ = script_exp_make_op1s(SCRIPT_EXP_VARIABLE,$1);
		free((char*)$1);
		if (!$$) YYERROR;
	}
	| TEXT {
		$$ = script_exp_make_op1t(SCRIPT_EXP_TEXT,$1);
		free((char*)$1);
		if (!$$) YYERROR;
	}
	| '(' exp ')' {
		$$ = $2;
	}
	| exp '+' exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_ADD,$1,$3);
		if (!$$) YYERROR;
	}
	| exp '-' exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_SUB,$1,$3);
		if (!$$) YYERROR;
	}
	| exp '&' exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_AND,$1,$3);
		if (!$$) YYERROR;
	}
	| exp '|' exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_OR,$1,$3);
		if (!$$) YYERROR;
	}
	| exp '^' exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_XOR,$1,$3);
		if (!$$) YYERROR;
	}
	| exp '<' exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_L,$1,$3);
		if (!$$) YYERROR;
	}
	| exp '>' exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_G,$1,$3);
		if (!$$) YYERROR;
	}
	| exp OP_E exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_E,$1,$3);
		if (!$$) YYERROR;
	}
	| exp OP_LE exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_LE,$1,$3);
		if (!$$) YYERROR;
	}
	| exp OP_GE exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_GE,$1,$3);
		if (!$$) YYERROR;
	}
	| exp OP_SL exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_SL,$1,$3);
		if (!$$) YYERROR;
	}
	| exp OP_SR exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_SR,$1,$3);
		if (!$$) YYERROR;
	}
	| exp OP_LAND exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_LAND,$1,$3);
		if (!$$) YYERROR;
	}
	| exp OP_LOR exp {
		$$ = script_exp_make_op2ee(SCRIPT_EXP_LOR,$1,$3);
		if (!$$) YYERROR;
	}
	| '!' exp {
		$$ = script_exp_make_op1e(SCRIPT_EXP_LNOT,$2);
		if (!$$) YYERROR;
	}
	| '~' exp {
		$$ = script_exp_make_op1e(SCRIPT_EXP_NOT,$2);
		if (!$$) YYERROR;
	}
	| STRING '(' ')' {
		$$ = script_exp_make_op1f(SCRIPT_EXP_F0,$1);
		free((char*)$1);
		if (!$$) YYERROR;
	}
	| STRING '(' exp ')' {
		$$ = script_exp_make_op2fe(SCRIPT_EXP_F1,$1,$3);
		free((char*)$1);
		if (!$$) YYERROR;
	}
	| STRING '(' exp ',' exp ')' {
		$$ = script_exp_make_op3fee(SCRIPT_EXP_F2,$1,$3,$5);
		free((char*)$1);
		if (!$$) YYERROR;
	}
;

cmd: '{' cmd_list '}' {
		$$ = script_cmd_make_op2sc("inner",$2);
		if (!$$) YYERROR;
	}
	| STRING '(' exp ')' '{' cmd_list '}' {
		$$ = script_cmd_make_op3sec($1,$3,$6);
		free((char*)$1);
		if (!$$) YYERROR;
	}
	| STRING '{' cmd_list '}' {
		$$ = script_cmd_make_op2sc($1,$3);
		free((char*)$1);
		if (!$$) YYERROR;
	}
	| WAIT '(' exp ')' ';' {
		$$ = script_cmd_make_op2se("wait",$3);
		if (!$$) YYERROR;
	}
	| DELAY '(' exp ')' ';' {
		$$ = script_cmd_make_op2se("delay",$3);
		if (!$$) YYERROR;
	}
	| exp ';' {
		$$ = script_cmd_make_op2se("evaluate",$1);
		if (!$$) YYERROR;
	}
;

%%

const char* script_input_begin;
const char* script_input_end;

int script_input(char* buf, int max_size)
{
	(void)max_size;
	if (script_input_begin == script_input_end) {
		return 0;
	} else {
		*buf = *script_input_begin++;
		return 1;
	}
}

struct script_cmd* script_parse(const char* text)
{
	int r;
	script_input_begin = text;
	script_input_end = script_input_begin + strlen(script_input_begin);
	r = yyparse();
	if (r!=0)
		return 0;

	r = yylex();
	if (r!=0) {
		if (r == STRING) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Unexpected token '%s'",yylval.str);
			yyerror(buffer);
		} else if (r<255) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Unexpected token '%c'",(char)r);
			yyerror(buffer);
		} else {
			yyerror("Unexpected token");
		}
		return 0;
	}

	return script_result;
}


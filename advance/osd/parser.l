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

#include <string.h>
#include "script.h"

#include "y_tab.h"

extern int script_input(char* buf, int max_size);

void script_flush(void)
{
	yy_delete_buffer(YY_CURRENT_BUFFER);
}

#define YY_INPUT(buf,result,max_size) \
	{ \
		int script_cc = script_input(buf,max_size); \
		if (!script_cc) \
			result = YY_NULL; \
		else \
			result = script_cc; \
	}

#define YY_NO_UNPUT

%}

%option noyywrap

ID_VAL      [0-9]+|0x[0-9a-fA-F]+|0b[0-1]+
ID_STR      [a-z_][a-z_0-9]*
ID_TEXT     \"[^"\n]*\"

%%

{ID_VAL} {
		if (yytext[0]=='0' && yytext[1]=='b')
			yylval.val = strtol(yytext+2, 0, 2);
		else
			yylval.val = strtol(yytext, 0, 0);
		return VAL;
	}

wait {
		return WAIT;
	}

delay {
		return DELAY;
	}

{ID_STR} {
		yylval.str = strdup( yytext );
		return STRING;
	}

{ID_TEXT} {
		yylval.text = strdup( yytext );
		return TEXT;
	}

\<= {
		return OP_LE;
	}

\>= {
		return OP_GE;
	}

== {
		return OP_E;
	}

\<\< {
		return OP_SL;
	}

\>\> {
		return OP_SR;
	}

[ \t\n]+	/* eat up whitespace */

.	return yytext[0];



/*  A Bison parser, made from parser.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	OP_LAND	257
#define	OP_LOR	258
#define	OP_E	259
#define	OP_LE	260
#define	OP_GE	261
#define	OP_SL	262
#define	OP_SR	263
#define	WAIT	264
#define	DELAY	265
#define	VAL	266
#define	STRING	267


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
 */

#include <stdlib.h>
#include <string.h>

#include "script.h"

#define YYERROR_VERBOSE

int yylex(void);

void yyerror(const char* s) {
	script_error(s);
}

struct script_cmd* yyresult;


typedef union {
	int val;
	const char* str;
	struct script_exp* exp;
	struct script_cmd* cmd;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		75
#define	YYFLAG		-32768
#define	YYNTBASE	29

#define YYTRANSLATE(x) ((unsigned)(x) <= 267 ? yytranslate[x] : 33)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    18,     2,     2,     2,     2,     5,     2,    23,
    24,     2,    15,    25,    16,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    28,     8,
     2,     9,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     7,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    26,     6,    27,    17,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,    10,    11,
    12,    13,    14,    19,    20,    21,    22
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     5,     7,     9,    11,    15,    19,    23,    27,
    31,    35,    39,    43,    47,    51,    55,    59,    63,    67,
    71,    74,    77,    81,    86,    93,    97,   105,   110,   116,
   122
};

static const short yyrhs[] = {    30,
     0,    30,    32,     0,    32,     0,    21,     0,    22,     0,
    23,    31,    24,     0,    31,    15,    31,     0,    31,    16,
    31,     0,    31,     5,    31,     0,    31,     6,    31,     0,
    31,     7,    31,     0,    31,     8,    31,     0,    31,     9,
    31,     0,    31,    10,    31,     0,    31,    11,    31,     0,
    31,    12,    31,     0,    31,    13,    31,     0,    31,    14,
    31,     0,    31,     3,    31,     0,    31,     4,    31,     0,
    18,    31,     0,    17,    31,     0,    22,    23,    24,     0,
    22,    23,    31,    24,     0,    22,    23,    31,    25,    31,
    24,     0,    26,    30,    27,     0,    22,    23,    31,    24,
    26,    30,    27,     0,    22,    26,    30,    27,     0,    19,
    23,    31,    24,    28,     0,    20,    23,    31,    24,    28,
     0,    31,    28,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    64,    69,    74,    79,    83,    88,    91,    95,    99,   103,
   107,   111,   115,   119,   123,   127,   131,   135,   139,   143,
   147,   151,   155,   160,   165,   172,   176,   181,   186,   190,
   194
};
#endif

#define YYNTOKENS 29
#define YYNNTS 4
#define YYNRULES 31
#define YYNSTATES 76
#define YYMAXUTOK 267

static const char * const yytname[] = {   "$","error","$undefined.","OP_LAND",
"OP_LOR","'&'","'|'","'^'","'<'","'>'","OP_E","OP_LE","OP_GE","OP_SL","OP_SR",
"'+'","'-'","'~'","'!'","WAIT","DELAY","VAL","STRING","'('","')'","','","'{'",
"'}'","';'","script","cmd_list","exp","cmd", NULL
};
static const short yytoknum[] = { 0,
   256,     2,   257,   258,    38,   124,    94,    60,    62,   259,
   260,   261,   262,   263,    43,    45,   126,    33,   264,   265,
   266,   267,    40,    41,    44,   123,   125,    59,     0
};

static const short yyr1[] = {     0,
    29,    30,    30,    31,    31,    31,    31,    31,    31,    31,
    31,    31,    31,    31,    31,    31,    31,    31,    31,    31,
    31,    31,    31,    31,    31,    32,    32,    32,    32,    32,
    32
};

static const short yyr2[] = {     0,
     1,     2,     1,     1,     1,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     2,     2,     3,     4,     6,     3,     7,     4,     5,     5,
     2
};

static const short yydefact[] = {     0,
     0,     0,     0,     0,     4,     5,     0,     0,     1,     0,
     3,     5,    22,    21,     0,     0,     0,     0,     0,     0,
     2,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    31,     0,     0,     0,    23,
     0,     0,     6,    26,    19,    20,     9,    10,    11,    12,
    13,    14,    15,    16,    17,    18,     7,     8,     0,     0,
     0,    24,     0,    28,    24,    29,    30,     0,     0,     0,
    25,    27,     0,     0,     0
};

static const short yydefgoto[] = {    73,
     9,    10,    11
};

static const short yypact[] = {   232,
   247,   247,   -20,   -18,-32768,   -19,   247,   232,   232,    36,
-32768,    -6,-32768,-32768,   247,   247,   239,   232,   112,   199,
-32768,   247,   247,   247,   247,   247,   247,   247,   247,   247,
   247,   247,   247,   247,   247,-32768,   239,   134,   156,-32768,
    66,   210,-32768,-32768,   198,   198,    45,    45,    45,    -4,
    -4,    -4,    -4,    -4,     4,     4,-32768,-32768,    89,     9,
    10,    39,   247,-32768,-32768,-32768,-32768,   232,   178,   221,
-32768,-32768,    67,    83,-32768
};

static const short yypgoto[] = {-32768,
     0,    -1,    -7
};


#define	YYLAST		270


static const short yytable[] = {    13,
    14,    21,    15,    17,    16,    19,    18,    20,    32,    33,
    34,    35,    21,    38,    39,    41,    37,    42,    34,    35,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    58,    21,    59,    66,    67,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    34,    35,    27,    28,    29,    30,    31,    32,    33,    34,
    35,    69,    21,    36,    68,     0,    74,    70,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    34,    35,    75,     0,     0,     0,     0,     0,     0,    62,
    63,    22,    23,    24,    25,    26,    27,    28,    29,    30,
    31,    32,    33,    34,    35,     0,     0,     0,     0,     0,
     0,     0,    65,    63,    22,    23,    24,    25,    26,    27,
    28,    29,    30,    31,    32,    33,    34,    35,     0,     0,
     0,     0,     0,     0,     0,    43,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
     0,     0,     0,     0,     0,     0,     0,    60,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    34,    35,     0,     0,     0,     0,     0,     0,     0,    61,
    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    33,    34,    35,     0,     0,     0,     0,     0,     0,
     0,    71,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    33,    34,    35,     0,     1,     2,     3,     4,     5,
     6,     7,     0,     0,     8,    44,     1,     2,     3,     4,
     5,     6,     7,     0,     0,     8,    64,     1,     2,     3,
     4,     5,     6,     7,     0,     0,     8,    72,     1,     2,
     3,     4,     5,     6,     7,     1,     2,     8,     0,     5,
    12,     7,    40,     1,     2,     0,     0,     5,    12,     7
};

static const short yycheck[] = {     1,
     2,     9,    23,    23,    23,     7,    26,     8,    13,    14,
    15,    16,    20,    15,    16,    17,    23,    18,    15,    16,
    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    33,    34,    35,    42,    37,    28,    28,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    63,    70,    28,    26,    -1,     0,    68,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,     0,    -1,    -1,    -1,    -1,    -1,    -1,    24,
    25,     3,     4,     5,     6,     7,     8,     9,    10,    11,
    12,    13,    14,    15,    16,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    24,    25,     3,     4,     5,     6,     7,     8,
     9,    10,    11,    12,    13,    14,    15,    16,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    24,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,
     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
    13,    14,    15,    16,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    24,     5,     6,     7,     8,     9,    10,    11,    12,
    13,    14,    15,    16,    -1,    17,    18,    19,    20,    21,
    22,    23,    -1,    -1,    26,    27,    17,    18,    19,    20,
    21,    22,    23,    -1,    -1,    26,    27,    17,    18,    19,
    20,    21,    22,    23,    -1,    -1,    26,    27,    17,    18,
    19,    20,    21,    22,    23,    17,    18,    26,    -1,    21,
    22,    23,    24,    17,    18,    -1,    -1,    21,    22,    23
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */

/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (__MSDOS__)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not __MSDOS__, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not __MSDOS__, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif



/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
{
		yyresult = yyvsp[0].cmd;
		YYACCEPT;
	;
    break;}
case 2:
{
		yyval.cmd = script_cmd_make_op2cc(yyvsp[-1].cmd, yyvsp[0].cmd);
		if (!yyval.cmd)
			YYERROR;
	;
    break;}
case 3:
{
		yyval.cmd = yyvsp[0].cmd; 
	;
    break;}
case 4:
{
		yyval.exp = script_exp_make_op1v(SCRIPT_EXP_VALUE,yyvsp[0].val);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 5:
{
		yyval.exp = script_exp_make_op1s(SCRIPT_EXP_VARIABLE,yyvsp[0].str);
		free((char*)yyvsp[0].str);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 6:
{
		yyval.exp = yyvsp[-1].exp;
	;
    break;}
case 7:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_ADD,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 8:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_SUB,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 9:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_AND,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 10:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_OR,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 11:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_XOR,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 12:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_L,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 13:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_G,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 14:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_E,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 15:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_LE,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 16:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_GE,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 17:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_SL,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 18:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_SR,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 19:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_LAND,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 20:
{
		yyval.exp = script_exp_make_op2ee(SCRIPT_EXP_LOR,yyvsp[-2].exp,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 21:
{
		yyval.exp = script_exp_make_op1e(SCRIPT_EXP_LNOT,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 22:
{
		yyval.exp = script_exp_make_op1e(SCRIPT_EXP_NOT,yyvsp[0].exp);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 23:
{
		yyval.exp = script_exp_make_op1f(SCRIPT_EXP_F0,yyvsp[-2].str);
		free((char*)yyvsp[-2].str);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 24:
{
		yyval.exp = script_exp_make_op2fe(SCRIPT_EXP_F1,yyvsp[-3].str,yyvsp[-1].exp);
		free((char*)yyvsp[-3].str);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 25:
{
		yyval.exp = script_exp_make_op3fee(SCRIPT_EXP_F2,yyvsp[-5].str,yyvsp[-3].exp,yyvsp[-1].exp);
		free((char*)yyvsp[-5].str);
		if (!yyval.exp) YYERROR;
	;
    break;}
case 26:
{
		yyval.cmd = script_cmd_make_op2sc("inner",yyvsp[-1].cmd);
		if (!yyval.cmd) YYERROR;
	;
    break;}
case 27:
{
		yyval.cmd = script_cmd_make_op3sec(yyvsp[-6].str,yyvsp[-4].exp,yyvsp[-1].cmd);
		free((char*)yyvsp[-6].str);
		if (!yyval.cmd) YYERROR;
	;
    break;}
case 28:
{
		yyval.cmd = script_cmd_make_op2sc(yyvsp[-3].str,yyvsp[-1].cmd);
		free((char*)yyvsp[-3].str);
		if (!yyval.cmd) YYERROR;
	;
    break;}
case 29:
{
		yyval.cmd = script_cmd_make_op2se("wait",yyvsp[-2].exp);
		if (!yyval.cmd) YYERROR;
	;
    break;}
case 30:
{
		yyval.cmd = script_cmd_make_op2se("delay",yyvsp[-2].exp);
		if (!yyval.cmd) YYERROR;
	;
    break;}
case 31:
{
		yyval.cmd = script_cmd_make_op2se("evaluate",yyvsp[-1].exp);
		if (!yyval.cmd) YYERROR;
	;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */


  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}


const char* script_input_begin;
const char* script_input_end;

int script_input(char* buf, int max_size) {
	(void)max_size;
	if (script_input_begin == script_input_end) {
		return 0;
	} else {
		*buf = *script_input_begin++;
		return 1;
	}
}

struct script_cmd* script_parse(const char* text) {
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
			sprintf(buffer,"Unexpected token '%s'",yylval.str);
			yyerror(buffer);
		} else if (r<255) {
			char buffer[256];
			sprintf(buffer,"Unexpected token '%c'",(char)r);
			yyerror(buffer);
		} else {
			yyerror("Unexpected token");
		}
		return 0;
	}
	return yyresult;
}

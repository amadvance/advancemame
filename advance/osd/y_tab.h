typedef union {
	int val;
	const char* str;
	struct script_exp* exp;
	struct script_cmd* cmd;
} YYSTYPE;
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


extern YYSTYPE yylval;

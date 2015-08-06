/*****************************************************************************
 *
 *	 z80gbd.c
 *	 Portable Z80 Gameboy disassembler
 *
 *	 Copyright (C) 2000 by The MESS Team.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#include "debugger.h"
#include "debug/eainfo.h"
#include "z80gb.h"

enum e_mnemonics
{
	zADC,  zADD,  zAND,  zBIT,	zCALL, zCCF,  zCP,
	zCPL,  zDAA,  zDB,	 zDEC,	zDI,   zEI,   zHLT,
	zIN,   zINC,  zJP,	 zJR,	zLD,   zNOP,  zOR,
	zPOP,  zPUSH, zRES,  zRET,	zRETI, zRL,   zRLA,
	zRLC,  zRLCA, zRR,	 zRRA,	zRRC,  zRRCA, zRST,
	zSBC,  zSCF,  zSET,  zSLA,	zSLL,  zSRA,  zSRL,
	zSTOP, zSUB,  zXOR,  zSWAP
};

static const char *s_mnemonic[] =
{
	"adc", "add", "and", "bit", "call","ccf", "cp",
	"cpl", "daa", "db",  "dec", "di",  "ei",  "halt",
	"in",  "inc", "jp",  "jr",  "ld",  "nop", "or",
	"pop", "push","res", "ret", "reti","rl",  "rla",
	"rlc", "rlca","rr",  "rra", "rrc", "rrca","rst",
	"sbc", "scf", "set", "sla", "sll", "sra", "srl",
	"stop","sub", "xor", "swap"
};

#define _OVER DASMFLAG_STEP_OVER
#define _OUT  DASMFLAG_STEP_OUT

static const UINT32 s_flags[] = {
	0    ,0    ,0    ,0    ,_OVER,0    ,0    ,
	0    ,0    ,0    ,0    ,0    ,0    ,_OVER,
	0    ,0    ,0    ,0    ,0    ,0    ,0    ,
	0    ,0    ,0    ,_OUT ,_OUT ,0    ,0    ,
	0    ,0    ,0    ,0    ,0    ,0    ,_OVER,
	0    ,0    ,0    ,0    ,0    ,0    ,0    ,
	_OVER,0    ,0    ,0
};

typedef struct
{
	UINT8	access;
	UINT8	mnemonic;
	const char *arguments;
}	z80gbdasm;

#define _0	EA_NONE
#define _JP EA_ABS_PC
#define _JR EA_REL_PC
#define _RM EA_MEM_RD
#define _WM EA_MEM_WR
#define _RW EA_MEM_RDWR
#define _RP EA_PORT_RD
#define _WP EA_PORT_WR

static z80gbdasm mnemonic_cb[256] = {
	{_0, zRLC,"b"},     {_0, zRLC,"c"},     {_0, zRLC,"d"},     {_0, zRLC,"e"},
	{_0, zRLC,"h"},     {_0, zRLC,"l"},     {_RW,zRLC,"(hl)"},  {_0, zRLC,"a"},
	{_0, zRRC,"b"},     {_0, zRRC,"c"},     {_0, zRRC,"d"},     {_0, zRRC,"e"},
	{_0, zRRC,"h"},     {_0, zRRC,"l"},     {_RW,zRRC,"(hl)"},  {_0, zRRC,"a"},
	{_0, zRL,"b"},      {_0, zRL,"c"},      {_0, zRL,"d"},      {_0, zRL,"e"},
	{_0, zRL,"h"},      {_0, zRL,"l"},      {_RW,zRL,"(hl)"},   {_0, zRL,"a"},
	{_0, zRR,"b"},      {_0, zRR,"c"},      {_0, zRR,"d"},      {_0, zRR,"e"},
	{_0, zRR,"h"},      {_0, zRR,"l"},      {_RW,zRR,"(hl)"},   {_0, zRR,"a"},
	{_0, zSLA,"b"},     {_0, zSLA,"c"},     {_0, zSLA,"d"},     {_0, zSLA,"e"},
	{_0, zSLA,"h"},     {_0, zSLA,"l"},     {_RW,zSLA,"(hl)"},  {_0, zSLA,"a"},
	{_0, zSRA,"b"},     {_0, zSRA,"c"},     {_0, zSRA,"d"},     {_0, zSRA,"e"},
	{_0, zSRA,"h"},     {_0, zSRA,"l"},     {_RW,zSRA,"(hl)"},  {_0, zSRA,"a"},
	{_0, zSWAP,"b"},    {_0, zSWAP,"c"},    {_0, zSWAP,"d"},    {_0, zSWAP,"e"},
	{_0, zSWAP,"h"},    {_0, zSWAP,"l"},    {_RW,zSWAP,"(hl)"}, {_0, zSWAP,"a"},
	{_0, zSRL,"b"},     {_0, zSRL,"c"},     {_0, zSRL,"d"},     {_0, zSRL,"e"},
	{_0, zSRL,"h"},     {_0, zSRL,"l"},     {_RW,zSRL,"(hl)"},  {_0, zSRL,"a"},
	{_0, zBIT,"0,b"},   {_0, zBIT,"0,c"},   {_0, zBIT,"0,d"},   {_0, zBIT,"0,e"},
	{_0, zBIT,"0,h"},   {_0, zBIT,"0,l"},   {_RM,zBIT,"0,(hl)"},{_0, zBIT,"0,a"},
	{_0, zBIT,"1,b"},   {_0, zBIT,"1,c"},   {_0, zBIT,"1,d"},   {_0, zBIT,"1,e"},
	{_0, zBIT,"1,h"},   {_0, zBIT,"1,l"},   {_RM,zBIT,"1,(hl)"},{_0, zBIT,"1,a"},
	{_0, zBIT,"2,b"},   {_0, zBIT,"2,c"},   {_0, zBIT,"2,d"},   {_0, zBIT,"2,e"},
	{_0, zBIT,"2,h"},   {_0, zBIT,"2,l"},   {_RM,zBIT,"2,(hl)"},{_0, zBIT,"2,a"},
	{_0, zBIT,"3,b"},   {_0, zBIT,"3,c"},   {_0, zBIT,"3,d"},   {_0, zBIT,"3,e"},
	{_0, zBIT,"3,h"},   {_0, zBIT,"3,l"},   {_RM,zBIT,"3,(hl)"},{_0, zBIT,"3,a"},
	{_0, zBIT,"4,b"},   {_0, zBIT,"4,c"},   {_0, zBIT,"4,d"},   {_0, zBIT,"4,e"},
	{_0, zBIT,"4,h"},   {_0, zBIT,"4,l"},   {_RM,zBIT,"4,(hl)"},{_0, zBIT,"4,a"},
	{_0, zBIT,"5,b"},   {_0, zBIT,"5,c"},   {_0, zBIT,"5,d"},   {_0, zBIT,"5,e"},
	{_0, zBIT,"5,h"},   {_0, zBIT,"5,l"},   {_RM,zBIT,"5,(hl)"},{_0, zBIT,"5,a"},
	{_0, zBIT,"6,b"},   {_0, zBIT,"6,c"},   {_0, zBIT,"6,d"},   {_0, zBIT,"6,e"},
	{_0, zBIT,"6,h"},   {_0, zBIT,"6,l"},   {_RM,zBIT,"6,(hl)"},{_0, zBIT,"6,a"},
	{_0, zBIT,"7,b"},   {_0, zBIT,"7,c"},   {_0, zBIT,"7,d"},   {_0, zBIT,"7,e"},
	{_0, zBIT,"7,h"},   {_0, zBIT,"7,l"},   {_RM,zBIT,"7,(hl)"},{_0, zBIT,"7,a"},
	{_0, zRES,"0,b"},   {_0, zRES,"0,c"},   {_0, zRES,"0,d"},   {_0, zRES,"0,e"},
	{_0, zRES,"0,h"},   {_0, zRES,"0,l"},   {_WM,zRES,"0,(hl)"},{_0, zRES,"0,a"},
	{_0, zRES,"1,b"},   {_0, zRES,"1,c"},   {_0, zRES,"1,d"},   {_0, zRES,"1,e"},
	{_0, zRES,"1,h"},   {_0, zRES,"1,l"},   {_WM,zRES,"1,(hl)"},{_0, zRES,"1,a"},
	{_0, zRES,"2,b"},   {_0, zRES,"2,c"},   {_0, zRES,"2,d"},   {_0, zRES,"2,e"},
	{_0, zRES,"2,h"},   {_0, zRES,"2,l"},   {_WM,zRES,"2,(hl)"},{_0, zRES,"2,a"},
	{_0, zRES,"3,b"},   {_0, zRES,"3,c"},   {_0, zRES,"3,d"},   {_0, zRES,"3,e"},
	{_0, zRES,"3,h"},   {_0, zRES,"3,l"},   {_WM,zRES,"3,(hl)"},{_0, zRES,"3,a"},
	{_0, zRES,"4,b"},   {_0, zRES,"4,c"},   {_0, zRES,"4,d"},   {_0, zRES,"4,e"},
	{_0, zRES,"4,h"},   {_0, zRES,"4,l"},   {_WM,zRES,"4,(hl)"},{_0, zRES,"4,a"},
	{_0, zRES,"5,b"},   {_0, zRES,"5,c"},   {_0, zRES,"5,d"},   {_0, zRES,"5,e"},
	{_0, zRES,"5,h"},   {_0, zRES,"5,l"},   {_WM,zRES,"5,(hl)"},{_0, zRES,"5,a"},
	{_0, zRES,"6,b"},   {_0, zRES,"6,c"},   {_0, zRES,"6,d"},   {_0, zRES,"6,e"},
	{_0, zRES,"6,h"},   {_0, zRES,"6,l"},   {_WM,zRES,"6,(hl)"},{_0, zRES,"6,a"},
	{_0, zRES,"7,b"},   {_0, zRES,"7,c"},   {_0, zRES,"7,d"},   {_0, zRES,"7,e"},
	{_0, zRES,"7,h"},   {_0, zRES,"7,l"},   {_WM,zRES,"7,(hl)"},{_0, zRES,"7,a"},
	{_0, zSET,"0,b"},   {_0, zSET,"0,c"},   {_0, zSET,"0,d"},   {_0, zSET,"0,e"},
	{_0, zSET,"0,h"},   {_0, zSET,"0,l"},   {_WM,zSET,"0,(hl)"},{_0, zSET,"0,a"},
	{_0, zSET,"1,b"},   {_0, zSET,"1,c"},   {_0, zSET,"1,d"},   {_0, zSET,"1,e"},
	{_0, zSET,"1,h"},   {_0, zSET,"1,l"},   {_WM,zSET,"1,(hl)"},{_0, zSET,"1,a"},
	{_0, zSET,"2,b"},   {_0, zSET,"2,c"},   {_0, zSET,"2,d"},   {_0, zSET,"2,e"},
	{_0, zSET,"2,h"},   {_0, zSET,"2,l"},   {_WM,zSET,"2,(hl)"},{_0, zSET,"2,a"},
	{_0, zSET,"3,b"},   {_0, zSET,"3,c"},   {_0, zSET,"3,d"},   {_0, zSET,"3,e"},
	{_0, zSET,"3,h"},   {_0, zSET,"3,l"},   {_WM,zSET,"3,(hl)"},{_0, zSET,"3,a"},
	{_0, zSET,"4,b"},   {_0, zSET,"4,c"},   {_0, zSET,"4,d"},   {_0, zSET,"4,e"},
	{_0, zSET,"4,h"},   {_0, zSET,"4,l"},   {_WM,zSET,"4,(hl)"},{_0, zSET,"4,a"},
	{_0, zSET,"5,b"},   {_0, zSET,"5,c"},   {_0, zSET,"5,d"},   {_0, zSET,"5,e"},
	{_0, zSET,"5,h"},   {_0, zSET,"5,l"},   {_WM,zSET,"5,(hl)"},{_0, zSET,"5,a"},
	{_0, zSET,"6,b"},   {_0, zSET,"6,c"},   {_0, zSET,"6,d"},   {_0, zSET,"6,e"},
	{_0, zSET,"6,h"},   {_0, zSET,"6,l"},   {_WM,zSET,"6,(hl)"},{_0, zSET,"6,a"},
	{_0, zSET,"7,b"},   {_0, zSET,"7,c"},   {_0, zSET,"7,d"},   {_0, zSET,"7,e"},
	{_0, zSET,"7,h"},   {_0, zSET,"7,l"},   {_WM,zSET,"7,(hl)"},{_0, zSET,"7,a"}
};

static z80gbdasm mnemonic_main[256]= {
	{_0, zNOP,0},		{_0, zLD,"bc,N"},   {_WM,zLD,"(bc),a"}, {_0, zINC,"bc"},
	{_0, zINC,"b"},     {_0, zDEC,"b"},     {_0, zLD,"b,B"},    {_0, zRLCA,0},
	{_WM,zLD,"(W),sp"}, {_0, zADD,"hl,bc"}, {_RM,zLD,"a,(bc)"}, {_0, zDEC,"bc"},
	{_0, zINC,"c"},     {_0, zDEC,"c"},     {_0, zLD,"c,B"},    {_0, zRRCA,0},
	{_0, zSTOP,0},		{_0, zLD,"de,N"},   {_WM,zLD,"(de),a"}, {_0, zINC,"de"},
	{_0, zINC,"d"},     {_0, zDEC,"d"},     {_0, zLD,"d,B"},    {_0, zRLA,0},
	{_JR,zJR,"O"},      {_0, zADD,"hl,de"}, {_RM,zLD,"a,(de)"}, {_0, zDEC,"de"},
	{_0, zINC,"e"},     {_0, zDEC,"e"},     {_0, zLD,"e,B"},    {_0, zRRA,0},
	{_JR,zJR,"nz,O"},   {_0, zLD,"hl,N"},   {_WM,zLD,"(hl+),a"},{_0, zINC,"hl"},
	{_0, zINC,"h"},     {_0, zDEC,"h"},     {_0, zLD,"h,B"},    {_0, zDAA,0},
	{_JR,zJR,"z,O"},    {_0, zADD,"hl,hl"}, {_RM,zLD,"a,(hl+)"},{_0, zDEC,"hl"},
	{_0, zINC,"l"},     {_0, zDEC,"l"},     {_0, zLD,"l,B"},    {_0, zCPL,0},
	{_JR,zJR,"nc,O"},   {_0, zLD,"sp,N"},   {_WM,zLD,"(hl-),a"},{_0, zINC,"sp"},
	{_RW,zINC,"(hl)"},  {_RW,zDEC,"(hl)"},  {_WM,zLD,"(hl),B"}, {_0, zSCF,0},
	{_JR,zJR,"c,O"},    {_0, zADD,"hl,sp"}, {_RM,zLD,"a,(hl-)"},{_0, zDEC,"sp"},
	{_0, zINC,"a"},     {_0, zDEC,"a"},     {_0, zLD,"a,B"},    {_0, zCCF,0},
	{_0, zLD,"b,b"},    {_0, zLD,"b,c"},    {_0, zLD,"b,d"},    {_0, zLD,"b,e"},
	{_0, zLD,"b,h"},    {_0, zLD,"b,l"},    {_RM,zLD,"b,(hl)"}, {_0, zLD,"b,a"},
	{_0, zLD,"c,b"},    {_0, zLD,"c,c"},    {_0, zLD,"c,d"},    {_0, zLD,"c,e"},
	{_0, zLD,"c,h"},    {_0, zLD,"c,l"},    {_RM,zLD,"c,(hl)"}, {_0, zLD,"c,a"},
	{_0, zLD,"d,b"},    {_0, zLD,"d,c"},    {_0, zLD,"d,d"},    {_0, zLD,"d,e"},
	{_0, zLD,"d,h"},    {_0, zLD,"d,l"},    {_RM,zLD,"d,(hl)"}, {_0, zLD,"d,a"},
	{_0, zLD,"e,b"},    {_0, zLD,"e,c"},    {_0, zLD,"e,d"},    {_0, zLD,"e,e"},
	{_0, zLD,"e,h"},    {_0, zLD,"e,l"},    {_RM,zLD,"e,(hl)"}, {_0, zLD,"e,a"},
	{_0, zLD,"h,b"},    {_0, zLD,"h,c"},    {_0, zLD,"h,d"},    {_0, zLD,"h,e"},
	{_0, zLD,"h,h"},    {_0, zLD,"h,l"},    {_RM,zLD,"h,(hl)"}, {_0, zLD,"h,a"},
	{_0, zLD,"l,b"},    {_0, zLD,"l,c"},    {_0, zLD,"l,d"},    {_0, zLD,"l,e"},
	{_0, zLD,"l,h"},    {_0, zLD,"l,l"},    {_RM,zLD,"l,(hl)"}, {_0, zLD,"l,a"},
	{_WM,zLD,"(hl),b"}, {_WM,zLD,"(hl),c"}, {_WM,zLD,"(hl),d"}, {_WM,zLD,"(hl),e"},
	{_WM,zLD,"(hl),h"}, {_WM,zLD,"(hl),l"}, {_0, zHLT,0},       {_WM,zLD,"(hl),a"},
	{_0, zLD,"a,b"},    {_0, zLD,"a,c"},    {_0, zLD,"a,d"},    {_0, zLD,"a,e"},
	{_0, zLD,"a,h"},    {_0, zLD,"a,l"},    {_RM,zLD,"a,(hl)"}, {_0, zLD,"a,a"},
	{_0, zADD,"a,b"},   {_0, zADD,"a,c"},   {_0, zADD,"a,d"},   {_0, zADD,"a,e"},
	{_0, zADD,"a,h"},   {_0, zADD,"a,l"},   {_RM,zADD,"a,(hl)"},{_0, zADD,"a,a"},
	{_0, zADC,"a,b"},   {_0, zADC,"a,c"},   {_0, zADC,"a,d"},   {_0, zADC,"a,e"},
	{_0, zADC,"a,h"},   {_0, zADC,"a,l"},   {_RM,zADC,"a,(hl)"},{_0, zADC,"a,a"},
	{_0, zSUB,"b"},     {_0, zSUB,"c"},     {_0, zSUB,"d"},     {_0, zSUB,"e"},
	{_0, zSUB,"h"},     {_0, zSUB,"l"},     {_RM,zSUB,"(hl)"},  {_0, zSUB,"a"},
	{_0, zSBC,"a,b"},   {_0, zSBC,"a,c"},   {_0, zSBC,"a,d"},   {_0, zSBC,"a,e"},
	{_0, zSBC,"a,h"},   {_0, zSBC,"a,l"},   {_RM,zSBC,"a,(hl)"},{_0, zSBC,"a,a"},
	{_0, zAND,"b"},     {_0, zAND,"c"},     {_0, zAND,"d"},     {_0, zAND,"e"},
	{_0, zAND,"h"},     {_0, zAND,"l"},     {_RM,zAND,"(hl)"},  {_0, zAND,"a"},
	{_0, zXOR,"b"},     {_0, zXOR,"c"},     {_0, zXOR,"d"},     {_0, zXOR,"e"},
	{_0, zXOR,"h"},     {_0, zXOR,"l"},     {_RM,zXOR,"(hl)"},  {_0, zXOR,"a"},
	{_0, zOR,"b"},      {_0, zOR,"c"},      {_0, zOR,"d"},      {_0, zOR,"e"},
	{_0, zOR,"h"},      {_0, zOR,"l"},      {_RM,zOR,"(hl)"},   {_0, zOR,"a"},
	{_0, zCP,"b"},      {_0, zCP,"c"},      {_0, zCP,"d"},      {_0, zCP,"e"},
	{_0, zCP,"h"},      {_0, zCP,"l"},      {_RM,zCP,"(hl)"},   {_0, zCP,"a"},
	{_0, zRET,"nz"},    {_0, zPOP,"bc"},    {_JP,zJP,"nz,A"},   {_JP,zJP,"A"},
	{_JP,zCALL,"nz,A"}, {_0, zPUSH,"bc"},   {_0, zADD,"a,B"},   {_JP,zRST,"V"},
	{_0, zRET,"z"},     {_0, zRET,0},       {_JP,zJP,"z,A"},    {_0, zDB,"cb"},
	{_JP,zCALL,"z,A"},  {_JP,zCALL,"A"},    {_0, zADC,"a,B"},   {_JP,zRST,"V"},
	{_0, zRET,"nc"},    {_0, zPOP,"de"},    {_JP,zJP,"nc,A"},   {_0, zDB,"d3"},
	{_JP,zCALL,"nc,A"}, {_0, zPUSH,"de"},   {_0, zSUB,"B"},     {_JP,zRST,"V"},
	{_0, zRET,"c"},     {_0, zRETI,0},      {_JP,zJP,"c,A"},    {_0, zDB,"db"},
	{_JP,zCALL,"c,A"},  {_0, zDB,"dd"},     {_0, zSBC,"a,B"},   {_JP,zRST,"V"},
	{_WM,zLD,"(F),a"},  {_0, zPOP,"hl"},    {_WM,zLD,"(C),a"},  {_0, zDB,"e3"},
	{_0, zDB,"e4"},     {_0, zPUSH,"hl"},   {_0, zAND,"B"},     {_JP,zRST,"V"},
	{_0, zADD,"SP,B"},  {_JP,zJP,"(hl)"},   {_WM,zLD,"(W),a"},  {_0, zDB,"eb"},
	{_0, zDB,"ec"},     {_0, zDB,"ed"},     {_0, zXOR,"B"},     {_JP,zRST,"V"},
	{_RM,zLD,"a,(F)"},  {_0, zPOP,"af"},    {_RM,zLD,"a,(C)"},  {_0, zDI,0},
	{_0, zDB,"f4"},     {_0, zPUSH,"af"},   {_0, zOR,"B"},      {_JP,zRST,"V"},
	{_0, zLD,"hl,sp+B"},{_0, zLD,"sp,hl"},  {_RM,zLD,"a,(W)"},  {_0, zEI,0},
	{_0, zDB,"fc"},     {_0, zDB,"fd"},     {_0, zCP,"B"},      {_JP,zRST,"V"}
};

/****************************************************************************
 * Disassemble opcode at PC and return number of bytes it takes
 ****************************************************************************/
unsigned z80gb_dasm( char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes )
{
	z80gbdasm *d;
	const char *symbol, *src;
	char *dst;
	unsigned PC = pc;
	INT8 offset = 0;
	UINT8 op, op1;
	UINT16 ea = 0;
	int pos = 0;

	dst = buffer;
	symbol = NULL;

	op = oprom[pos++];
	op1 = 0; /* keep GCC happy */

	if( op == 0xcb ) {
		op = oprom[pos++];
		d = &mnemonic_cb[op];
	} else {
		d = &mnemonic_main[op];
	}

	if( d->arguments ) {
		dst += sprintf(dst, "%-4s ", s_mnemonic[d->mnemonic]);
		src = d->arguments;
		while( *src ) {
			switch( *src ) {
			case '?':   /* illegal opcode */
				dst += sprintf( dst, "$%02x,$%02x", op, op1);
				break;
			case 'A':
				ea = opram[pos] + ( opram[pos+1] << 8);
				pos += 2;
				symbol = set_ea_info(0, ea, EA_UINT16, d->access);
				dst += sprintf( dst, "%s", symbol );
				break;
			case 'B':   /* Byte op arg */
				ea = opram[pos++];
				symbol = set_ea_info(1, ea, EA_UINT8, EA_VALUE);
				dst += sprintf( dst, "%s", symbol );
				break;
			case '(':   /* Memory byte at (...) */
				*dst++ = *src;
				if( !strncmp( src, "(bc)", 4) ) {
					ea = z80gb_get_reg( Z80GB_BC );
					set_ea_info(0, ea, EA_UINT8, d->access);
				} else if( !strncmp( src, "(de)", 4) ) {
					ea = z80gb_get_reg( Z80GB_DE );
					set_ea_info(0, ea, EA_UINT8, d->access);
				} else if( !strncmp( src, "(hl)", 4) ) {
					ea = z80gb_get_reg( Z80GB_HL );
					if( d->access == EA_ABS_PC )
						set_ea_info(0, ea, EA_DEFAULT, EA_ABS_PC);
					else
						set_ea_info(0, ea, EA_UINT8, d->access);
				} else if( !strncmp( src, "(sp)", 4) ) {
					ea = z80gb_get_reg( Z80GB_SP );
					set_ea_info(0, ea, EA_UINT16, d->access);
				} else if( !strncmp( src, "(F)", 3) ) {
					ea = 0xFF00 + opram[pos++];
					symbol = set_ea_info(0, ea, EA_UINT8, d->access);
					dst += sprintf( dst, "%s", symbol );
					src++;
				} else if( !strncmp( src, "(C)", 3) ) {
					ea = 0xff00 + (z80gb_get_reg( Z80GB_BC ) & 0xff);
					symbol = set_ea_info(0, 0xff00, EA_UINT16, EA_VALUE);
					set_ea_info(1, ea, EA_UINT8, d->access);
					dst += sprintf( dst, "%s+c", symbol );
					src++;
				}
				break;
			case 'N':   /* Immediate 16 bit */
				ea = opram[pos] + ( opram[pos+1] << 8 );
				pos += 2;
				symbol = set_ea_info(1, ea, EA_UINT16, EA_VALUE );
				dst += sprintf( dst, "%s", symbol );
				break;
			case 'O':   /* Offset relative to PC */
				offset = (INT8) opram[pos++];
				symbol = set_ea_info(0, PC, offset + 2, d->access);
				dst += sprintf( dst, "%s", symbol );
				break;
			case 'V':   /* Restart vector */
				ea = op & 0x38;
				symbol = set_ea_info(0, ea, EA_UINT8, d->access);
				dst += sprintf( dst, "%s", symbol );
				break;
			case 'W':   /* Memory address word */
				ea = opram[pos] + ( opram[pos+1] << 8 );
				pos += 2;
				symbol = set_ea_info(0, ea, EA_UINT16, d->access);
				dst += sprintf( dst, "%s", symbol );
				break;
			default:
				*dst++ = *src;
			}
			src++;
		}
		*dst = '\0';
	} else {
		dst += sprintf(dst, "%s", s_mnemonic[d->mnemonic]);
	}

	return pos | s_flags[d->mnemonic] | DASMFLAG_SUPPORTED;
}

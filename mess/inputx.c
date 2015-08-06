/*********************************************************************

	inputx.h

	Secondary input related functions for MESS specific functionality

*********************************************************************/

#include <ctype.h>
#include <assert.h>
#include <wctype.h>
#include "inputx.h"
#include "inptport.h"
#include "mame.h"

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debug/debugcon.h"
#endif

#define NUM_CODES		128
#define NUM_SIMUL_KEYS	(UCHAR_SHIFT_END - UCHAR_SHIFT_BEGIN + 1)
#define LOG_INPUTX		0

struct InputCode
{
	UINT32 port[NUM_SIMUL_KEYS];
	const input_port_entry *ipt[NUM_SIMUL_KEYS];
};

struct KeyBuffer
{
	int begin_pos;
	int end_pos;
	unsigned int status_keydown : 1;
	unicode_char_t buffer[4096];
};

struct CharInfo
{
	unicode_char_t ch;
	const char *name;
	const char *alternate;	/* alternative string, in UTF-8 */
};

static const struct CharInfo charinfo[] =
{
	{ 0x0008,					"Backspace",	NULL },		/* Backspace */	
	{ 0x0009,					"Tab",			"    " },	/* Tab */
	{ 0x000c,					"Clear",		NULL },		/* Clear */
	{ 0x000d,					"Enter",		NULL },		/* Enter */
	{ 0x001a,					"Esc",			NULL },		/* Esc */
	{ 0x0061,					NULL,			"A" },		/* a */
	{ 0x0062,					NULL,			"B" },		/* b */
	{ 0x0063,					NULL,			"C" },		/* c */
	{ 0x0064,					NULL,			"D" },		/* d */
	{ 0x0065,					NULL,			"E" },		/* e */
	{ 0x0066,					NULL,			"F" },		/* f */
	{ 0x0067,					NULL,			"G" },		/* g */
	{ 0x0068,					NULL,			"H" },		/* h */
	{ 0x0069,					NULL,			"I" },		/* i */
	{ 0x006a,					NULL,			"J" },		/* j */
	{ 0x006b,					NULL,			"K" },		/* k */
	{ 0x006c,					NULL,			"L" },		/* l */
	{ 0x006d,					NULL,			"M" },		/* m */
	{ 0x006e,					NULL,			"N" },		/* n */
	{ 0x006f,					NULL,			"O" },		/* o */
	{ 0x0070,					NULL,			"P" },		/* p */
	{ 0x0071,					NULL,			"Q" },		/* q */
	{ 0x0072,					NULL,			"R" },		/* r */
	{ 0x0073,					NULL,			"S" },		/* s */
	{ 0x0074,					NULL,			"T" },		/* t */
	{ 0x0075,					NULL,			"U" },		/* u */
	{ 0x0076,					NULL,			"V" },		/* v */
	{ 0x0077,					NULL,			"W" },		/* w */
	{ 0x0078,					NULL,			"X" },		/* x */
	{ 0x0079,					NULL,			"Y" },		/* y */
	{ 0x007a,					NULL,			"Z" },		/* z */
	{ 0x00a0,					NULL,			" " },		/* non breaking space */
	{ 0x00a1,					NULL,			"!" },		/* inverted exclaimation mark */
	{ 0x00a6,					NULL,			"|" },		/* broken bar */
	{ 0x00a9,					NULL,			"(c)" },	/* copyright sign */
	{ 0x00ab,					NULL,			"<<" },		/* left pointing double angle */
	{ 0x00ae,					NULL,			"(r)" },	/* registered sign */
	{ 0x00bb,					NULL,			">>" },		/* right pointing double angle */
	{ 0x00bc,					NULL,			"1/4" },	/* vulgar fraction one quarter */
	{ 0x00bd,					NULL,			"1/2" },	/* vulgar fraction one half */
	{ 0x00be,					NULL,			"3/4" },	/* vulgar fraction three quarters */
	{ 0x00bf,					NULL,			"?" },		/* inverted question mark */
	{ 0x00c0,					NULL,			"A" },		/* 'A' grave */
	{ 0x00c1,					NULL,			"A" },		/* 'A' acute */
	{ 0x00c2,					NULL,			"A" },		/* 'A' circumflex */
	{ 0x00c3,					NULL,			"A" },		/* 'A' tilde */
	{ 0x00c4,					NULL,			"A" },		/* 'A' diaeresis */
	{ 0x00c5,					NULL,			"A" },		/* 'A' ring above */
	{ 0x00c6,					NULL,			"AE" },		/* 'AE' ligature */
	{ 0x00c7,					NULL,			"C" },		/* 'C' cedilla */
	{ 0x00c8,					NULL,			"E" },		/* 'E' grave */
	{ 0x00c9,					NULL,			"E" },		/* 'E' acute */
	{ 0x00ca,					NULL,			"E" },		/* 'E' circumflex */
	{ 0x00cb,					NULL,			"E" },		/* 'E' diaeresis */
	{ 0x00cc,					NULL,			"I" },		/* 'I' grave */
	{ 0x00cd,					NULL,			"I" },		/* 'I' acute */
	{ 0x00ce,					NULL,			"I" },		/* 'I' circumflex */
	{ 0x00cf,					NULL,			"I" },		/* 'I' diaeresis */
	{ 0x00d0,					NULL,			"D" },		/* 'ETH' */
	{ 0x00d1,					NULL,			"N" },		/* 'N' tilde */
	{ 0x00d2,					NULL,			"O" },		/* 'O' grave */
	{ 0x00d3,					NULL,			"O" },		/* 'O' acute */
	{ 0x00d4,					NULL,			"O" },		/* 'O' circumflex */
	{ 0x00d5,					NULL,			"O" },		/* 'O' tilde */
	{ 0x00d6,					NULL,			"O" },		/* 'O' diaeresis */
	{ 0x00d7,					NULL,			"X" },		/* multiplication sign */
	{ 0x00d8,					NULL,			"O" },		/* 'O' stroke */
	{ 0x00d9,					NULL,			"U" },		/* 'U' grave */
	{ 0x00da,					NULL,			"U" },		/* 'U' acute */
	{ 0x00db,					NULL,			"U" },		/* 'U' circumflex */
	{ 0x00dc,					NULL,			"U" },		/* 'U' diaeresis */
	{ 0x00dd,					NULL,			"Y" },		/* 'Y' acute */
	{ 0x00df,					NULL,			"SS" },		/* sharp S */
	{ 0x00e0,					NULL,			"a" },		/* 'a' grave */
	{ 0x00e1,					NULL,			"a" },		/* 'a' acute */
	{ 0x00e2,					NULL,			"a" },		/* 'a' circumflex */
	{ 0x00e3,					NULL,			"a" },		/* 'a' tilde */
	{ 0x00e4,					NULL,			"a" },		/* 'a' diaeresis */
	{ 0x00e5,					NULL,			"a" },		/* 'a' ring above */
	{ 0x00e6,					NULL,			"ae" },		/* 'ae' ligature */
	{ 0x00e7,					NULL,			"c" },		/* 'c' cedilla */
	{ 0x00e8,					NULL,			"e" },		/* 'e' grave */
	{ 0x00e9,					NULL,			"e" },		/* 'e' acute */
	{ 0x00ea,					NULL,			"e" },		/* 'e' circumflex */
	{ 0x00eb,					NULL,			"e" },		/* 'e' diaeresis */
	{ 0x00ec,					NULL,			"i" },		/* 'i' grave */
	{ 0x00ed,					NULL,			"i" },		/* 'i' acute */
	{ 0x00ee,					NULL,			"i" },		/* 'i' circumflex */
	{ 0x00ef,					NULL,			"i" },		/* 'i' diaeresis */
	{ 0x00f0,					NULL,			"d" },		/* 'eth' */
	{ 0x00f1,					NULL,			"n" },		/* 'n' tilde */
	{ 0x00f2,					NULL,			"o" },		/* 'o' grave */
	{ 0x00f3,					NULL,			"o" },		/* 'o' acute */
	{ 0x00f4,					NULL,			"o" },		/* 'o' circumflex */
	{ 0x00f5,					NULL,			"o" },		/* 'o' tilde */
	{ 0x00f6,					NULL,			"o" },		/* 'o' diaeresis */
	{ 0x00f8,					NULL,			"o" },		/* 'o' stroke */
	{ 0x00f9,					NULL,			"u" },		/* 'u' grave */
	{ 0x00fa,					NULL,			"u" },		/* 'u' acute */
	{ 0x00fb,					NULL,			"u" },		/* 'u' circumflex */
	{ 0x00fc,					NULL,			"u" },		/* 'u' diaeresis */
	{ 0x00fd,					NULL,			"y" },		/* 'y' acute */
	{ 0x00ff,					NULL,			"y" },		/* 'y' diaeresis */
	{ 0x2010,					NULL,			"-" },		/* hyphen */
	{ 0x2011,					NULL,			"-" },		/* non-breaking hyphen */
	{ 0x2012,					NULL,			"-" },		/* figure dash */
	{ 0x2013,					NULL,			"-" },		/* en dash */
	{ 0x2014,					NULL,			"-" },		/* em dash */
	{ 0x2015,					NULL,			"-" },		/* horizontal dash */
	{ 0x2018,					NULL,			"\'" },		/* left single quotation mark */
	{ 0x2019,					NULL,			"\'" },		/* right single quotation mark */
	{ 0x201a,					NULL,			"\'" },		/* single low quotation mark */
	{ 0x201b,					NULL,			"\'" },		/* single high reversed quotation mark */
	{ 0x201c,					NULL,			"\"" },		/* left double quotation mark */
	{ 0x201d,					NULL,			"\"" },		/* right double quotation mark */
	{ 0x201e,					NULL,			"\"" },		/* double low quotation mark */
	{ 0x201f,					NULL,			"\"" },		/* double high reversed quotation mark */
	{ 0x2024,					NULL,			"." },		/* one dot leader */
	{ 0x2025,					NULL,			".." },		/* two dot leader */
	{ 0x2026,					NULL,			"..." },	/* horizontal ellipsis */
	{ 0x2047,					NULL,			"??" },		/* double question mark */
	{ 0x2048,					NULL,			"?!" },		/* question exclamation mark */
	{ 0x2049,					NULL,			"!?" },		/* exclamation question mark */
	{ 0xff01,					NULL,			"!" },		/* fullwidth exclamation point */
	{ 0xff02,					NULL,			"\"" },		/* fullwidth quotation mark */
	{ 0xff03,					NULL,			"#" },		/* fullwidth number sign */
	{ 0xff04,					NULL,			"$" },		/* fullwidth dollar sign */
	{ 0xff05,					NULL,			"%" },		/* fullwidth percent sign */
	{ 0xff06,					NULL,			"&" },		/* fullwidth ampersand */
	{ 0xff07,					NULL,			"\'" },		/* fullwidth apostrophe */
	{ 0xff08,					NULL,			"(" },		/* fullwidth left parenthesis */
	{ 0xff09,					NULL,			")" },		/* fullwidth right parenthesis */
	{ 0xff0a,					NULL,			"*" },		/* fullwidth asterisk */
	{ 0xff0b,					NULL,			"+" },		/* fullwidth plus */
	{ 0xff0c,					NULL,			"," },		/* fullwidth comma */
	{ 0xff0d,					NULL,			"-" },		/* fullwidth minus */
	{ 0xff0e,					NULL,			"." },		/* fullwidth period */
	{ 0xff0f,					NULL,			"/" },		/* fullwidth slash */
	{ 0xff10,					NULL,			"0" },		/* fullwidth zero */
	{ 0xff11,					NULL,			"1" },		/* fullwidth one */
	{ 0xff12,					NULL,			"2" },		/* fullwidth two */
	{ 0xff13,					NULL,			"3" },		/* fullwidth three */
	{ 0xff14,					NULL,			"4" },		/* fullwidth four */
	{ 0xff15,					NULL,			"5" },		/* fullwidth five */
	{ 0xff16,					NULL,			"6" },		/* fullwidth six */
	{ 0xff17,					NULL,			"7" },		/* fullwidth seven */
	{ 0xff18,					NULL,			"8" },		/* fullwidth eight */
	{ 0xff19,					NULL,			"9" },		/* fullwidth nine */
	{ 0xff1a,					NULL,			":" },		/* fullwidth colon */
	{ 0xff1b,					NULL,			";" },		/* fullwidth semicolon */
	{ 0xff1c,					NULL,			"<" },		/* fullwidth less than sign */
	{ 0xff1d,					NULL,			"=" },		/* fullwidth equals sign */
	{ 0xff1e,					NULL,			">" },		/* fullwidth greater than sign */
	{ 0xff1f,					NULL,			"?" },		/* fullwidth question mark */
	{ 0xff20,					NULL,			"@" },		/* fullwidth at sign */
	{ 0xff21,					NULL,			"A" },		/* fullwidth 'A' */
	{ 0xff22,					NULL,			"B" },		/* fullwidth 'B' */
	{ 0xff23,					NULL,			"C" },		/* fullwidth 'C' */
	{ 0xff24,					NULL,			"D" },		/* fullwidth 'D' */
	{ 0xff25,					NULL,			"E" },		/* fullwidth 'E' */
	{ 0xff26,					NULL,			"F" },		/* fullwidth 'F' */
	{ 0xff27,					NULL,			"G" },		/* fullwidth 'G' */
	{ 0xff28,					NULL,			"H" },		/* fullwidth 'H' */
	{ 0xff29,					NULL,			"I" },		/* fullwidth 'I' */
	{ 0xff2a,					NULL,			"J" },		/* fullwidth 'J' */
	{ 0xff2b,					NULL,			"K" },		/* fullwidth 'K' */
	{ 0xff2c,					NULL,			"L" },		/* fullwidth 'L' */
	{ 0xff2d,					NULL,			"M" },		/* fullwidth 'M' */
	{ 0xff2e,					NULL,			"N" },		/* fullwidth 'N' */
	{ 0xff2f,					NULL,			"O" },		/* fullwidth 'O' */
	{ 0xff30,					NULL,			"P" },		/* fullwidth 'P' */
	{ 0xff31,					NULL,			"Q" },		/* fullwidth 'Q' */
	{ 0xff32,					NULL,			"R" },		/* fullwidth 'R' */
	{ 0xff33,					NULL,			"S" },		/* fullwidth 'S' */
	{ 0xff34,					NULL,			"T" },		/* fullwidth 'T' */
	{ 0xff35,					NULL,			"U" },		/* fullwidth 'U' */
	{ 0xff36,					NULL,			"V" },		/* fullwidth 'V' */
	{ 0xff37,					NULL,			"W" },		/* fullwidth 'W' */
	{ 0xff38,					NULL,			"X" },		/* fullwidth 'X' */
	{ 0xff39,					NULL,			"Y" },		/* fullwidth 'Y' */
	{ 0xff3a,					NULL,			"Z" },		/* fullwidth 'Z' */
	{ 0xff3b,					NULL,			"[" },		/* fullwidth left bracket */
	{ 0xff3c,					NULL,			"\\" },		/* fullwidth backslash */
	{ 0xff3d,					NULL,			"]"	},		/* fullwidth right bracket */
	{ 0xff3e,					NULL,			"^" },		/* fullwidth caret */
	{ 0xff3f,					NULL,			"_" },		/* fullwidth underscore */
	{ 0xff40,					NULL,			"`" },		/* fullwidth backquote */
	{ 0xff41,					NULL,			"a" },		/* fullwidth 'a' */
	{ 0xff42,					NULL,			"b" },		/* fullwidth 'b' */
	{ 0xff43,					NULL,			"c" },		/* fullwidth 'c' */
	{ 0xff44,					NULL,			"d" },		/* fullwidth 'd' */
	{ 0xff45,					NULL,			"e" },		/* fullwidth 'e' */
	{ 0xff46,					NULL,			"f" },		/* fullwidth 'f' */
	{ 0xff47,					NULL,			"g" },		/* fullwidth 'g' */
	{ 0xff48,					NULL,			"h" },		/* fullwidth 'h' */
	{ 0xff49,					NULL,			"i" },		/* fullwidth 'i' */
	{ 0xff4a,					NULL,			"j" },		/* fullwidth 'j' */
	{ 0xff4b,					NULL,			"k" },		/* fullwidth 'k' */
	{ 0xff4c,					NULL,			"l" },		/* fullwidth 'l' */
	{ 0xff4d,					NULL,			"m" },		/* fullwidth 'm' */
	{ 0xff4e,					NULL,			"n" },		/* fullwidth 'n' */
	{ 0xff4f,					NULL,			"o" },		/* fullwidth 'o' */
	{ 0xff50,					NULL,			"p" },		/* fullwidth 'p' */
	{ 0xff51,					NULL,			"q" },		/* fullwidth 'q' */
	{ 0xff52,					NULL,			"r" },		/* fullwidth 'r' */
	{ 0xff53,					NULL,			"s" },		/* fullwidth 's' */
	{ 0xff54,					NULL,			"t" },		/* fullwidth 't' */
	{ 0xff55,					NULL,			"u" },		/* fullwidth 'u' */
	{ 0xff56,					NULL,			"v" },		/* fullwidth 'v' */
	{ 0xff57,					NULL,			"w" },		/* fullwidth 'w' */
	{ 0xff58,					NULL,			"x" },		/* fullwidth 'x' */
	{ 0xff59,					NULL,			"y" },		/* fullwidth 'y' */
	{ 0xff5a,					NULL,			"z" },		/* fullwidth 'z' */
	{ 0xff5b,					NULL,			"{" },		/* fullwidth left brace */
	{ 0xff5c,					NULL,			"|" },		/* fullwidth vertical bar */
	{ 0xff5d,					NULL,			"}" },		/* fullwidth right brace */
	{ 0xff5e,					NULL,			"~" },		/* fullwidth tilde */
	{ 0xff5f,					NULL,			"((" },		/* fullwidth double left parenthesis */
	{ 0xff60,					NULL,			"))" },		/* fullwidth double right parenthesis */
	{ 0xffe0,					NULL,			"\xC2\xA2" },		/* fullwidth cent sign */
	{ 0xffe1,					NULL,			"\xC2\xA3" },		/* fullwidth pound sign */
	{ 0xffe4,					NULL,			"\xC2\xA4" },		/* fullwidth broken bar */
	{ 0xffe5,					NULL,			"\xC2\xA5" },		/* fullwidth yen sign */
	{ 0xffe6,					NULL,			"\xE2\x82\xA9" },	/* fullwidth won sign */
	{ 0xffe9,					NULL,			"\xE2\x86\x90" },	/* fullwidth left arrow */
	{ 0xffea,					NULL,			"\xE2\x86\x91" },	/* fullwidth up arrow */
	{ 0xffeb,					NULL,			"\xE2\x86\x92" },	/* fullwidth right arrow */
	{ 0xffec,					NULL,			"\xE2\x86\x93" },	/* fullwidth down arrow */
	{ 0xffed,					NULL,			"\xE2\x96\xAA" },	/* fullwidth solid box */
	{ 0xffee,					NULL,			"\xE2\x97\xA6" },	/* fullwidth open circle */	
	{ UCHAR_MAMEKEY(F1),		"F1",			NULL },		/* F1 function key */
	{ UCHAR_MAMEKEY(F2),		"F2",			NULL },		/* F2 function key */
	{ UCHAR_MAMEKEY(F3),		"F3",			NULL },		/* F3 function key */
	{ UCHAR_MAMEKEY(F4),		"F4",			NULL },		/* F4 function key */
	{ UCHAR_MAMEKEY(F5),		"F5",			NULL },		/* F5 function key */
	{ UCHAR_MAMEKEY(F6),		"F6",			NULL },		/* F6 function key */
	{ UCHAR_MAMEKEY(F7),		"F7",			NULL },		/* F7 function key */
	{ UCHAR_MAMEKEY(F8),		"F8",			NULL },		/* F8 function key */
	{ UCHAR_MAMEKEY(F9),		"F9",			NULL },		/* F9 function key */
	{ UCHAR_MAMEKEY(F10),		"F10",			NULL },		/* F10 function key */
	{ UCHAR_MAMEKEY(F11),		"F11",			NULL },		/* F11 function key */
	{ UCHAR_MAMEKEY(F12),		"F12",			NULL },		/* F12 function key */
	{ UCHAR_MAMEKEY(F13),		"F13",			NULL },		/* F13 function key */
	{ UCHAR_MAMEKEY(F14),		"F14",			NULL },		/* F14 function key */
	{ UCHAR_MAMEKEY(F15),		"F15",			NULL },		/* F15 function key */
	{ UCHAR_MAMEKEY(ESC),		"Esc",			"\033" },	/* esc key */
	{ UCHAR_MAMEKEY(DEL),		"Delete",		"\010" },	/* delete key */
	{ UCHAR_MAMEKEY(HOME),		"Home",			"\014" },	/* home key */
	{ UCHAR_MAMEKEY(LSHIFT),	"Left Shift",	NULL },		/* left shift key */
	{ UCHAR_MAMEKEY(RSHIFT),	"Right Shift",	NULL },		/* right shift key */
	{ UCHAR_MAMEKEY(LCONTROL),	"Left Ctrl",	NULL },		/* left control key */
	{ UCHAR_MAMEKEY(RCONTROL),	"Right Ctrl",	NULL }		/* right control key */
};

#define INVALID_CHAR '?'

/***************************************************************************

	Char info lookup

***************************************************************************/

static const struct CharInfo *find_charinfo(unicode_char_t target_char)
{
	int low = 0;
	int high = sizeof(charinfo) / sizeof(charinfo[0]);
	int i;
	unicode_char_t ch;

	/* perform a simple binary search to find the proper alternate */
	while(high > low)
	{
		i = (high + low) / 2;
		ch = charinfo[i].ch;
		if (ch < target_char)
			low = i + 1;
		else if (ch > target_char)
			high = i;
		else
			return &charinfo[i];
	}
	return NULL;
}



/***************************************************************************

	Code assembling

***************************************************************************/

static const char *charstr(unicode_char_t ch)
{
	static char buf[3];

	switch(ch)
	{
		case '\0':	strcpy(buf, "\\0");		break;
		case '\r':	strcpy(buf, "\\r");		break;
		case '\n':	strcpy(buf, "\\n");		break;
		case '\t':	strcpy(buf, "\\t");		break;
		default:
			buf[0] = (char) ch;
			buf[1] = '\0';
			break;
	}
	return buf;
}



static int scan_keys(const input_port_entry *input_ports, struct InputCode *codes, UINT32 *ports, const input_port_entry **shift_ports, int keys, int shift)
{
	int result = 0;
	const input_port_entry *ipt;
	const input_port_entry *ipt_key = NULL;
	UINT32 port = (UINT32) -1;
	unicode_char_t code;

	assert(keys < NUM_SIMUL_KEYS);

	ipt = input_ports;
	while(ipt->type != IPT_END)
	{
		switch(ipt->type) {
		case IPT_KEYBOARD:
			ipt_key = ipt;

			code = ipt->keyboard.chars[shift];
			if (code)
			{
				/* mark that we have found some natural keyboard codes */
				result = 1;

				/* is this a shifter key? */
				if ((code >= UCHAR_SHIFT_BEGIN) && (code <= UCHAR_SHIFT_END))
				{
					ports[keys] = port;
					shift_ports[keys] = ipt_key;
					scan_keys(input_ports, codes, ports, shift_ports, keys+1, code - UCHAR_SHIFT_1 + 1);
				}
				else if (code < NUM_CODES)
				{
					memcpy(codes[code].port, ports, sizeof(ports[0]) * keys);
					memcpy((void *) codes[code].ipt, shift_ports, sizeof(shift_ports[0]) * keys);
					codes[code].port[keys] = port;
					codes[code].ipt[keys] = ipt_key;

					if (LOG_INPUTX)
						logerror("inputx: code=%i (%s) port=%i ipt->name='%s'\n", (int) code, charstr(code), port, ipt->name);
				}
			}
			break;

		case IPT_PORT:
			port++;
			/* fall through */

		default:
			ipt_key = NULL;
			break;
		}
		ipt++;
	}
	return result;
}



static unicode_char_t unicode_tolower(unicode_char_t c)
{
	return (c < 128) ? tolower((char) c) : c;
}



#define CODE_BUFFER_SIZE	(sizeof(struct InputCode) * NUM_CODES)

static int build_codes(const input_port_entry *input_ports, struct InputCode *codes, int map_lowercase)
{
	UINT32 ports[NUM_SIMUL_KEYS];
	const input_port_entry *ipts[NUM_SIMUL_KEYS];
	int switch_upper, rc = 0;
	unicode_char_t c;

	/* first clear the buffer */
	memset(codes, 0, CODE_BUFFER_SIZE);

	if (!scan_keys(input_ports, codes, ports, ipts, 0, 0))
		goto done;

	if (map_lowercase)
	{
		/* special case; scan to see if upper case characters are specified, but not lower case */
		switch_upper = 1;
		for (c = 'A'; c <= 'Z'; c++)
		{
			if (!inputx_can_post_key(c) || inputx_can_post_key(unicode_tolower(c)))
			{
				switch_upper = 0;
				break;
			}
		}

		if (switch_upper)
			memcpy(&codes['a'], &codes['A'], sizeof(codes[0]) * 26);
	}

	rc = 1;

done:
	return rc;
}



/***************************************************************************

	Validity checks

***************************************************************************/

int inputx_validitycheck(const game_driver *gamedrv, input_port_entry **memory)
{
	char buf[CODE_BUFFER_SIZE];
	struct InputCode *codes;
	const input_port_entry *input_ports;
	const input_port_entry *ipt;
	int port_count, i, j;
	int error = 0;
	unicode_char_t last_char = 0;
	const struct CharInfo *ci;

	if (gamedrv)
	{
		if (gamedrv->flags & GAME_COMPUTER)
		{
			codes = (struct InputCode *) buf;

			/* allocate the input ports */
			*memory = input_port_allocate(gamedrv->construct_ipt, *memory);
			input_ports = *memory;

			build_codes(input_ports, codes, FALSE);

			port_count = 0;
			for (ipt = input_ports; ipt->type != IPT_END; ipt++)
			{
				if (ipt->type == IPT_PORT)
					port_count++;
			}

			if (port_count > 0)
			{
				for (i = 0; i < NUM_CODES; i++)
				{
					for (j = 0; j < NUM_SIMUL_KEYS; j++)
					{
						if (codes[i].port[j] >= port_count)
						{
							printf("%s: invalid inputx translation for code %i port %i\n", gamedrv->name, i, j);
							error = 1;
						}
					}
				}
			}
		}
	}
	else
	{
		/* check to make sure that charinfo is in order */
		for (i = 0; i < sizeof(charinfo) / sizeof(charinfo[0]); i++)
		{
			if (last_char >= charinfo[i].ch)
			{
				printf("inputx: charinfo is out of order; 0x%08x should be higher than 0x%08x\n", charinfo[i].ch, last_char);
				error = 1;
			}
			last_char = charinfo[i].ch;
		}

		/* check to make sure that I can look up everything on alternate_charmap */
		for (i = 0; i < sizeof(charinfo) / sizeof(charinfo[0]); i++)
		{
			ci = find_charinfo(charinfo[i].ch);
			if (ci != &charinfo[i])
			{
				printf("inputx: expected find_charinfo(0x%08x) to work properly\n", charinfo[i].ch);
				error = 1;
			}
		}
	}
	return error;
}



/***************************************************************************

	Core

***************************************************************************/

static struct InputCode *codes;
static struct KeyBuffer *keybuffer;
static mame_timer *inputx_timer;
static int (*queue_chars)(const unicode_char_t *text, size_t text_len);
static int (*accept_char)(unicode_char_t ch);
static int (*charqueue_empty)(void);
static mame_time current_rate;

static void inputx_timerproc(int dummy);



#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
static void execute_input(int ref, int params, const char *param[])
{
	inputx_post_coded(param[0]);
}
#endif



static void setup_keybuffer(void)
{
	inputx_timer = mame_timer_alloc(inputx_timerproc);
	keybuffer = auto_malloc(sizeof(struct KeyBuffer));
	memset(keybuffer, 0, sizeof(*keybuffer));
}



void inputx_init(void)
{
	codes = NULL;
	inputx_timer = NULL;
	queue_chars = NULL;
	accept_char = NULL;
	charqueue_empty = NULL;
	keybuffer = NULL;

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	debug_console_register_command("input", CMDFLAG_NONE, 0, 1, 1, execute_input);
#endif /* defined(MAME_DEBUG) && defined(NEW_DEBUGGER) */

	/* posting keys directly only makes sense for a computer */
	if (Machine->gamedrv->flags & GAME_COMPUTER)
	{
		codes = (struct InputCode *) auto_malloc(CODE_BUFFER_SIZE);
		if (!build_codes(Machine->input_ports, codes, TRUE))
			goto error;

		setup_keybuffer();
	}
	return;

error:
	codes = NULL;
}

void inputx_setup_natural_keyboard(
	int (*queue_chars_)(const unicode_char_t *text, size_t text_len),
	int (*accept_char_)(unicode_char_t ch),
	int (*charqueue_empty_)(void))
{
	setup_keybuffer();
	queue_chars = queue_chars_;
	accept_char = accept_char_;
	charqueue_empty = charqueue_empty_;
}

int inputx_can_post(void)
{
	return queue_chars || codes;
}

static struct KeyBuffer *get_buffer(void)
{
	assert(inputx_can_post());
	return (struct KeyBuffer *) keybuffer;
}

static int can_post_key_directly(unicode_char_t ch)
{
	int rc;

	if (queue_chars)
	{
		rc = accept_char ? accept_char(ch) : TRUE;
	}
	else
	{
		assert(codes);
		rc = ((ch < NUM_CODES) && codes[ch].ipt[0] != NULL);
	}
	return rc;
}

static int can_post_key_alternate(unicode_char_t ch)
{
	const char *s;
	const struct CharInfo *ci;
	unicode_char_t uchar;
	int rc;

	ci = find_charinfo(ch);
	s = ci ? ci->alternate : NULL;
	if (!s)
		return 0;

	while(*s)
	{
		rc = uchar_from_utf8(&uchar, s, strlen(s));
		if (rc <= 0)
			return 0;
		if (!can_post_key_directly(uchar))
			return 0;
		s += rc;
	}
	return 1;
}



int inputx_can_post_key(unicode_char_t ch)
{
	return inputx_can_post() && (can_post_key_directly(ch) || can_post_key_alternate(ch));
}



static mame_time choose_delay(unicode_char_t ch)
{
	subseconds_t delay = 0;

	if (current_rate.seconds || current_rate.subseconds)
		return current_rate;

	if (queue_chars)
	{
		/* systems with queue_chars can afford a much smaller delay */
		delay = DOUBLE_TO_SUBSECONDS(0.01);
	}
	else
	{
		switch(ch) {
		case '\r':
			delay = DOUBLE_TO_SUBSECONDS(0.2);
			break;

		default:
			delay = DOUBLE_TO_SUBSECONDS(0.05);
			break;
		}
	}
	return make_mame_time(0, delay);
}



static void internal_post_key(unicode_char_t ch)
{
	struct KeyBuffer *keybuf;

	keybuf = get_buffer();

	/* need to start up the timer? */
	if (keybuf->begin_pos == keybuf->end_pos)
	{
		mame_timer_adjust(inputx_timer, choose_delay(ch), 0, time_zero);
		keybuf->status_keydown = 0;
	}

	keybuf->buffer[keybuf->end_pos++] = ch;
	keybuf->end_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
}



static int buffer_full(void)
{
	struct KeyBuffer *keybuf;
	keybuf = get_buffer();
	return ((keybuf->end_pos + 1) % (sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]))) == keybuf->begin_pos;
}



void inputx_postn_rate(const unicode_char_t *text, size_t text_len, mame_time rate)
{
	int last_cr = 0;
	unicode_char_t ch;
	const char *s;
	const struct CharInfo *ci;

	current_rate = rate;

	if (inputx_can_post())
	{
		while((text_len > 0) && !buffer_full())
		{
			ch = *(text++);
			text_len--;

			/* change all eolns to '\r' */
			if ((ch != '\n') || !last_cr)
			{
				if (ch == '\n')
					ch = '\r';
				else
					last_cr = (ch == '\r');

				if (LOG_INPUTX)
					logerror("inputx_postn(): code=%i (%s) port=%i ipt->name='%s'\n", (int) ch, charstr(ch), codes[ch].port[0], codes[ch].ipt[0] ? codes[ch].ipt[0]->name : "<null>");

				if (can_post_key_directly(ch))
				{
					/* we can post this key in the queue directly */
					internal_post_key(ch);
				}
				else if (can_post_key_alternate(ch))
				{
					/* we can post this key with an alternate representation */
					ci = find_charinfo(ch);
					assert(ci && ci->alternate);
					s = ci->alternate;
					while(*s)
					{
						s += uchar_from_utf8(&ch, s, strlen(s));
						internal_post_key(ch);
					}
				}
			}
			else
			{
				last_cr = 0;
			}
		}
	}
}



static void inputx_timerproc(int dummy)
{
	struct KeyBuffer *keybuf;
	mame_time delay;

	keybuf = get_buffer();

	if (queue_chars)
	{
		/* the driver has a queue_chars handler */
		while((keybuf->begin_pos != keybuf->end_pos) && queue_chars(&keybuf->buffer[keybuf->begin_pos], 1))
		{
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);

			if (current_rate.seconds || current_rate.subseconds)
				break;
		}
	}
	else
	{
		/* the driver does not have a queue_chars handler */
		if (keybuf->status_keydown)
		{
			keybuf->status_keydown = FALSE;
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
		}
		else
		{
			keybuf->status_keydown = TRUE;
		}
	}

	/* need to make sure timerproc is called again if buffer not empty */
	if (keybuf->begin_pos != keybuf->end_pos)
	{
		delay = choose_delay(keybuf->buffer[keybuf->begin_pos]);
		mame_timer_adjust(inputx_timer, delay, 0, time_zero);
	}
}



void inputx_update(void)
{
	const struct KeyBuffer *keybuf;
	const struct InputCode *code;
	unicode_char_t ch;
	int i;
	UINT32 value;

	if (inputx_can_post())
	{
		keybuf = get_buffer();

		/* is the key down right now? */
		if (keybuf->status_keydown && (keybuf->begin_pos != keybuf->end_pos))
		{
			/* identify the character that is down right now, and its component codes */
			ch = keybuf->buffer[keybuf->begin_pos];
			code = &codes[ch];

			/* loop through this character's component codes */
			for (i = 0; code->ipt[i] && (i < sizeof(code->ipt) / sizeof(code->ipt[0])); i++)
			{
				value = code->ipt[i]->mask;
				input_port_set_digital_value(code->port[i], value, value);
			}
		}
	}
}



void inputx_handle_mess_extensions(input_port_entry *ipt)
{
	char buf[256];
	int i, pos;
	unicode_char_t ch;

	/* process MESS specific extensions to all ports */
	while(ipt->type != IPT_END)
	{
		/* is this a keyboard port with the default name? */
		if (ipt->type == IPT_KEYBOARD && (ipt->name == IP_NAME_DEFAULT))
		{
			buf[0] = '\0';
			pos = 0;

			for (i = 0; ipt->keyboard.chars[i] && (i < sizeof(ipt->keyboard.chars)
				/ sizeof(ipt->keyboard.chars[0])); i++)
			{
				ch = ipt->keyboard.chars[i];
				pos += sprintf(&buf[pos], "%s ", inputx_key_name(ch));
			}

			rtrim(buf);

			if (buf[0])
				ipt->name = auto_strdup(buf);
			else
				ipt->name = "Unnamed Key";
		}
		ipt++;
	}
}



const char *inputx_key_name(unicode_char_t ch)
{
	static char buf[2];
	const struct CharInfo *ci;
	const char *result;

	ci = find_charinfo(ch);
	result = ci ? ci->name : NULL;

	if (ci && ci->name)
	{
		result = ci->name;
	}
	else
	{
		if ((ch <= 0x7F) && isprint(ch))
		{
			buf[0] = (char) ch;
			buf[1] = '\0';
			result = buf;
		}
		else
			result = "???";
	}
	return result;
}



/* --------------------------------------------------------------------- */

int inputx_is_posting(void)
{
	const struct KeyBuffer *keybuf;
	keybuf = get_buffer();
	return (keybuf->begin_pos != keybuf->end_pos) || (charqueue_empty && !charqueue_empty());
}



/***************************************************************************

	Coded input

***************************************************************************/

void inputx_postn_coded_rate(const char *text, size_t text_len, mame_time rate)
{
	size_t i, j, key_len, increment;
	unicode_char_t ch;

	static const struct
	{
		const char *key;
		unicode_char_t code;
	} codes[] =
	{
		{ "BACKSPACE",	8 },
		{ "BS",			8 },
		{ "BKSP",		8 },
		{ "DEL",		UCHAR_MAMEKEY(DEL) },
		{ "DELETE",		UCHAR_MAMEKEY(DEL) },
		{ "END",		UCHAR_MAMEKEY(END) },
		{ "ENTER",		13 },
		{ "ESC",		'\033' },
		{ "HOME",		UCHAR_MAMEKEY(HOME) },
		{ "INS",		UCHAR_MAMEKEY(INSERT) },
		{ "INSERT",		UCHAR_MAMEKEY(INSERT) },
		{ "PGDN",		UCHAR_MAMEKEY(PGDN) },
		{ "PGUP",		UCHAR_MAMEKEY(PGUP) },
		{ "SPACE",		32 },
		{ "TAB",		9 },
		{ "F1",			UCHAR_MAMEKEY(F1) },
		{ "F2",			UCHAR_MAMEKEY(F2) },
		{ "F3",			UCHAR_MAMEKEY(F3) },
		{ "F4",			UCHAR_MAMEKEY(F4) },
		{ "F5",			UCHAR_MAMEKEY(F5) },
		{ "F6",			UCHAR_MAMEKEY(F6) },
		{ "F7",			UCHAR_MAMEKEY(F7) },
		{ "F8",			UCHAR_MAMEKEY(F8) },
		{ "F9",			UCHAR_MAMEKEY(F9) },
		{ "F10",		UCHAR_MAMEKEY(F10) },
		{ "F11",		UCHAR_MAMEKEY(F11) },
		{ "F12",		UCHAR_MAMEKEY(F12) },
		{ "QUOTE",		'\"' }
	};

	i = 0;
	while(i < text_len)
	{
		ch = text[i];
		increment = 1;

		if (ch == '{')
		{
			for (j = 0; j < sizeof(codes) / sizeof(codes[0]); j++)
			{
				key_len = strlen(codes[j].key);
				if (i + key_len + 2 <= text_len)
				{
					if (!memcmp(codes[j].key, &text[i + 1], key_len) && (text[i + key_len + 1] == '}'))
					{
						ch = codes[j].code;
						increment = key_len + 2;
					}
				}
			}
		}

		if (ch)
			inputx_postc_rate(ch, rate);
		i += increment;
	}
}



/***************************************************************************

	Alternative calls

***************************************************************************/

void inputx_postn(const unicode_char_t *text, size_t text_len)
{
	inputx_postn_rate(text, text_len, make_mame_time(0, 0));
}



void inputx_post_rate(const unicode_char_t *text, mame_time rate)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn_rate(text, len, rate);
}



void inputx_post(const unicode_char_t *text)
{
	inputx_post_rate(text, make_mame_time(0, 0));
}



void inputx_postc_rate(unicode_char_t ch, mame_time rate)
{
	inputx_postn_rate(&ch, 1, rate);
}



void inputx_postc(unicode_char_t ch)
{
	inputx_postc_rate(ch, make_mame_time(0, 0));
}



void inputx_postn_utf16_rate(const utf16_char_t *text, size_t text_len, mame_time rate)
{
	size_t len = 0;
	unicode_char_t c;
	utf16_char_t w1, w2;
	unicode_char_t buf[256];

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(buf, len);
			len = 0;
		}

		w1 = *(text++);
		text_len--;

		if ((w1 >= 0xd800) && (w1 <= 0xdfff))
		{
			if (w1 <= 0xDBFF)
			{
				w2 = 0;
				if (text_len > 0)
				{
					w2 = *(text++);
					text_len--;
				}
				if ((w2 >= 0xdc00) && (w2 <= 0xdfff))
				{
					c = w1 & 0x03ff;
					c <<= 10;
					c |= w2 & 0x03ff;
				}
				else
				{
					c = INVALID_CHAR;
				}
			}
			else
			{
				c = INVALID_CHAR;
			}
		}
		else
		{
			c = w1;
		}
		buf[len++] = c;
	}
	inputx_postn_rate(buf, len, rate);
}



void inputx_postn_utf16(const utf16_char_t *text, size_t text_len)
{
	inputx_postn_utf16_rate(text, text_len, make_mame_time(0, 0));
}



void inputx_post_utf16_rate(const utf16_char_t *text, mame_time rate)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn_utf16_rate(text, len, rate);
}



void inputx_post_utf16(const utf16_char_t *text)
{
	inputx_post_utf16_rate(text, make_mame_time(0, 0));
}



void inputx_postn_utf8_rate(const char *text, size_t text_len, mame_time rate)
{
	size_t len = 0;
	unicode_char_t buf[256];
	unicode_char_t c;
	int rc;

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(buf, len);
			len = 0;
		}

		rc = uchar_from_utf8(&c, text, text_len);
		if (rc < 0)
		{
			rc = 1;
			c = INVALID_CHAR;
		}
		text += rc;
		text_len -= rc;
		buf[len++] = c;
	}
	inputx_postn_rate(buf, len, rate);
}



void inputx_postn_utf8(const char *text, size_t text_len)
{
	inputx_postn_utf8_rate(text, text_len, make_mame_time(0, 0));
}



void inputx_post_utf8_rate(const char *text, mame_time rate)
{
	inputx_postn_utf8_rate(text, strlen(text), rate);
}



void inputx_post_utf8(const char *text)
{
	inputx_post_utf8_rate(text, make_mame_time(0, 0));
}



void inputx_post_coded(const char *text)
{
	inputx_postn_coded(text, strlen(text));
}



void inputx_post_coded_rate(const char *text, mame_time rate)
{
	inputx_postn_coded_rate(text, strlen(text), rate);
}



void inputx_postn_coded(const char *text, size_t text_len)
{
	inputx_postn_coded_rate(text, text_len, make_mame_time(0, 0));
}



/***************************************************************************

	Other stuff

	This stuff is here more out of convienience than anything else
***************************************************************************/

int input_classify_port(const input_port_entry *port)
{
	int result;

	if (port->unused)
		return INPUT_CLASS_INTERNAL;
	if (port->category && (port->type != IPT_CATEGORY_SETTING))
		return INPUT_CLASS_CATEGORIZED;

	switch(port->type) {
	case IPT_JOYSTICK_UP:
	case IPT_JOYSTICK_DOWN:
	case IPT_JOYSTICK_LEFT:
	case IPT_JOYSTICK_RIGHT:
	case IPT_JOYSTICKLEFT_UP:
	case IPT_JOYSTICKLEFT_DOWN:
	case IPT_JOYSTICKLEFT_LEFT:
	case IPT_JOYSTICKLEFT_RIGHT:
	case IPT_JOYSTICKRIGHT_UP:
	case IPT_JOYSTICKRIGHT_DOWN:
	case IPT_JOYSTICKRIGHT_LEFT:
	case IPT_JOYSTICKRIGHT_RIGHT:
	case IPT_BUTTON1:
	case IPT_BUTTON2:
	case IPT_BUTTON3:
	case IPT_BUTTON4:
	case IPT_BUTTON5:
	case IPT_BUTTON6:
	case IPT_BUTTON7:
	case IPT_BUTTON8:
	case IPT_BUTTON9:
	case IPT_BUTTON10:
	case IPT_AD_STICK_X:
	case IPT_AD_STICK_Y:
	case IPT_AD_STICK_Z:
	case IPT_TRACKBALL_X:
	case IPT_TRACKBALL_Y:
	case IPT_LIGHTGUN_X:
	case IPT_LIGHTGUN_Y:
	case IPT_MOUSE_X:
	case IPT_MOUSE_Y:
	case IPT_START:
	case IPT_SELECT:
		result = INPUT_CLASS_CONTROLLER;
		break;

	case IPT_KEYBOARD:
		result = INPUT_CLASS_KEYBOARD;
		break;

	case IPT_CONFIG_NAME:
		result = INPUT_CLASS_CONFIG;
		break;

	case IPT_DIPSWITCH_NAME:
		result = INPUT_CLASS_DIPSWITCH;
		break;

	case 0:
		if (port->name && (port->name != (const char *) -1))
			result = INPUT_CLASS_MISC;
		else
			result = INPUT_CLASS_INTERNAL;
		break;

	default:
		result = INPUT_CLASS_INTERNAL;
		break;
	}
	return result;
}



int input_player_number(const input_port_entry *port)
{
	return port->player;
}



int input_has_input_class(int inputclass)
{
	input_port_entry *in;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		if (input_classify_port(in) == inputclass)
			return TRUE;
	}
	return FALSE;
}



int input_count_players(void)
{
	const input_port_entry *in;
	int joystick_count;

	joystick_count = 0;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		if (input_classify_port(in) == INPUT_CLASS_CONTROLLER)
		{
			if (joystick_count <= in->player + 1)
				joystick_count = in->player + 1;
		}
	}
	return joystick_count;
}



int input_category_active(int category)
{
	const input_port_entry *in;
	const input_port_entry *in_base = NULL;

	assert(category >= 1);

	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		switch(in->type) {
		case IPT_CATEGORY_NAME:
			in_base = in;
			break;

		case IPT_CATEGORY_SETTING:
			if ((in->category == category) && (in_base->default_value == in->default_value))
				return TRUE;
			break;
		}
	}
	return FALSE;
}

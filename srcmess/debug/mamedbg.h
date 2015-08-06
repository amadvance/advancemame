/****************************************************************************

    mamedbg.h

    MAME debugger V0.54
    Juergen Buchmueller <pullmoll@t-online.de>

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************/

#pragma once

#ifndef _MAMEDBG_H
#define _MAMEDBG_H

#include "mamecore.h"

#define DEBUGGER_TOTAL_COLORS 16

#ifdef  MAME_DEBUG

enum {
	DBG_BLACK,
	DBG_BLUE,
	DBG_GREEN,
	DBG_CYAN,
	DBG_RED,
	DBG_MAGENTA,
	DBG_BROWN,
	DBG_LIGHTGRAY,
	DBG_GRAY,
	DBG_LIGHTBLUE,
	DBG_LIGHTGREEN,
	DBG_LIGHTCYAN,
	DBG_LIGHTRED,
	DBG_LIGHTMAGENTA,
	DBG_YELLOW,
	DBG_WHITE
};

#define COLOR_TITLE 		DBG_YELLOW
#define COLOR_FRAME 		DBG_LIGHTCYAN
#define COLOR_REGS			DBG_WHITE
#define COLOR_DASM			DBG_WHITE
#define COLOR_MEM1			DBG_WHITE
#define COLOR_MEM2			DBG_WHITE
#define COLOR_CMDS			DBG_WHITE
#define COLOR_BRK_EXEC		DBG_YELLOW
#define COLOR_BRK_DATA		(DBG_YELLOW+DBG_BLUE*16)
#define COLOR_BRK_REGS		(DBG_YELLOW+DBG_BLUE*16)
#define COLOR_ERROR 		(DBG_YELLOW+DBG_RED*16)
#define COLOR_HELP			(DBG_WHITE+DBG_BLUE*16)
#define COLOR_PROMPT		DBG_CYAN
#define COLOR_CHANGES		DBG_LIGHTCYAN
#define COLOR_PC			(DBG_WHITE+DBG_BLUE*16) /* MB 980103 */
#define COLOR_CURSOR		(DBG_WHITE+DBG_RED*16)	/* MB 980103 */

extern int debug_trace_delay;	/* set to 0 to force a screen update */

#ifndef DECL_SPEC
#define DECL_SPEC
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef INVALID
#define INVALID 0xffffffff
#endif

extern UINT8 debugger_bitmap_changed;
extern UINT8 debugger_focus;

extern rgb_t debugger_palette[DEBUGGER_TOTAL_COLORS];
gfx_element *build_debugger_font(void);
void dbg_put_screen_char (int ch, int attr, int x, int y);

void CLIB_DECL mame_debug_trace_write (int cpunum, const char *fmt, ...) ATTR_PRINTF(2,3);

#endif  /* !MAME_DEBUG */

#endif

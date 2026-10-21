/****************************************************************************

    debugger.h

    General debugging interfaces

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************/

#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "mame.h"


/***************************************************************************
    MACROS
***************************************************************************/

/* handy macro for hard-coding debugger breaks */
#ifdef MAME_DEBUG
#define DEBUGGER_BREAK			if (Machine->debug_mode) mame_debug_break();
#else
#define DEBUGGER_BREAK
#endif


#ifdef MAME_DEBUG
extern int debug_cpu_active;
extern int debug_break_active;
extern int debug_watch_active;
void debug_cpu_hook(void);
void debug_watch_hook(int space, UINT32 address, unsigned size, UINT64 data, int write);

#define CALL_MAME_DEBUG do { if (debug_cpu_active >= -1 || debug_break_active) debug_cpu_hook(); if (Machine->debug_mode) mame_debug_hook(); } while (0)
#else
#define CALL_MAME_DEBUG
#endif



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* initialize the debugger */
void mame_debug_init(void);

/* call this once per instruction from CPU cores */
void mame_debug_hook(void);

/* call this to break into the debugger as soon as possible */
void mame_debug_break(void);


#endif	/* __DEBUGGER_H__ */

// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
/*
    Differences from MAME:

    Written in C. The four address spaces (program, data, io and register
    file) are the callbacks of struct mb86233_config.
*/

#ifndef MB86233_H
#define MB86233_H

#include "cpuintrf.h"

enum
{
	MB86233_PC = 1,
	MB86233_SP
};

struct mb86233_config
{
	UINT32 (*prog_r)( UINT32 offset );
	UINT32 (*data_r)( UINT32 offset );
	void   (*data_w)( UINT32 offset, UINT32 data );
	UINT32 (*io_r)( UINT32 offset );
	void   (*io_w)( UINT32 offset, UINT32 data );
	UINT32 (*rf_r)( UINT32 offset );
	void   (*rf_w)( UINT32 offset, UINT32 data );
};

extern int mb86233_icount;

void mb86233_get_info(UINT32 state, union cpuinfo *info);

/* a memory callback that cannot complete now (FIFO empty or full):
   the instruction is repeated */
extern void mb86233_stall(void);
extern void mb86233_gpio_w(int pin, int state);

#endif

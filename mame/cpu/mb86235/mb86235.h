// license:BSD-3-Clause
// copyright-holders:Angelo Salese, ElSemi, Ville Linde, Matthew Daniels
/*****************************************************************************
 *
 * template for CPU cores
 *
 *****************************************************************************/

/*
    Differences from MAME:

    Written in C. Program memory, the external bus and the FIFOs are the
    callbacks of struct mb86235_config; the internal A and B bus RAMs are part
    of the CPU.
*/

#ifndef MB86235_H
#define MB86235_H

#include "cpuintrf.h"

#define MB86235_FIFO_IFF	0	/* input FIFO full */
#define MB86235_FIFO_IFE	1	/* input FIFO empty */
#define MB86235_FIFO_OFF	2	/* output FIFO full */
#define MB86235_FIFO_OFE	3	/* output FIFO empty */

struct mb86235_config
{
	UINT64 (*prog_r)( UINT32 offset );				/* 64-bit words, 12-bit address */
	UINT32 (*ext_r)( UINT32 offset );				/* external bus, dword address */
	void   (*ext_w)( UINT32 offset, UINT32 data );
	UINT32 (*fifoin_r)( void );						/* calls mb86235_stall() when empty */
	void   (*fifoout_w)( UINT32 data );
	int    (*fifo_flag)( int which );
	void   (*fifo_clear)( int which );				/* 0: input, 1: output */
};

extern int mb86235_icount;

enum
{
	MB86235_PC = 1,
	MB86235_SP
};

void mb86235_get_info(UINT32 state, union cpuinfo *info);

/* a FIFO read that cannot complete now: the instruction is repeated */
extern void mb86235_stall(void);

#endif

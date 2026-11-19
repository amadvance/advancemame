// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

    xavix.h

    XaviX (SSD 2000) CPU

***************************************************************************/

#ifndef _XAVIX_H
#define _XAVIX_H

#include "cpuintrf.h"

enum {
	XAVIX_PC=1, XAVIX_S, XAVIX_P, XAVIX_A, XAVIX_X, XAVIX_Y,
	XAVIX_DATABANK, XAVIX_CODEBANK,
	XAVIX_J, XAVIX_K, XAVIX_L, XAVIX_M, XAVIX_PA, XAVIX_PB
};

#define XAVIX_IRQ_LINE 0

/* Provides the custom interrupt vectors. Returns -1 when the vectors are not
   enabled and the vectors in the ROM must be used.
   which is 0 for NMI and 1 for IRQ, half is 0 for the high byte and 1 for the low byte */
struct xavix_config
{
	int (*get_vector)(int which, int half);
};

extern void xavix2000_get_info(UINT32 state, union cpuinfo *info);

extern UINT8 xavix_read_full_data(UINT32 addr);
extern void xavix_write_full_data(UINT32 addr, UINT8 data);

#endif

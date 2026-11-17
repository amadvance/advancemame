// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, hap

/* Panasonic MN10200 external interrupt pins and port callbacks. */

#ifndef _MN10200_H_
#define _MN10200_H_

#include "driver.h"

enum
{
	MN10200_PC = 1,
	MN10200_PSW,
	MN10200_MDR,
	MN10200_D0,
	MN10200_D1,
	MN10200_D2,
	MN10200_D3,
	MN10200_A0,
	MN10200_A1,
	MN10200_A2,
	MN10200_A3,
	MN10200_NMICR,
	MN10200_IAGR
};


enum
{
	MN10200_IRQ0 = 0,
	MN10200_IRQ1,
	MN10200_IRQ2,
	MN10200_IRQ3,

	MN10200_MAX_EXT_IRQ
};

typedef UINT8 (*mn10200_port_read_func)(int port);
typedef void (*mn10200_port_write_func)(int port, UINT8 data);


void mn10200_set_port_handlers(mn10200_port_read_func readfn, mn10200_port_write_func writefn);

void mn10200_get_info(UINT32 state, union cpuinfo *info);

#endif

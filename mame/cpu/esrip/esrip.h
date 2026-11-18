// license:BSD-3-Clause
// copyright-holders:Philip Bennett

/* Interface for the Entertainment Sciences RIP */

#ifndef _ESRIP_H
#define _ESRIP_H

#include "cpuintrf.h"

enum
{
	ESRIP_PC = 1,
	ESRIP_ACC,
	ESRIP_DLATCH,
	ESRIP_ILATCH,
	ESRIP_RAM00,
	ESRIP_RAM01,
	ESRIP_RAM02,
	ESRIP_RAM03,
	ESRIP_RAM04,
	ESRIP_RAM05,
	ESRIP_RAM06,
	ESRIP_RAM07,
	ESRIP_RAM08,
	ESRIP_RAM09,
	ESRIP_RAM0A,
	ESRIP_RAM0B,
	ESRIP_RAM0C,
	ESRIP_RAM0D,
	ESRIP_RAM0E,
	ESRIP_RAM0F,
	ESRIP_RAM10,
	ESRIP_RAM11,
	ESRIP_RAM12,
	ESRIP_RAM13,
	ESRIP_RAM14,
	ESRIP_RAM15,
	ESRIP_RAM16,
	ESRIP_RAM17,
	ESRIP_RAM18,
	ESRIP_RAM19,
	ESRIP_RAM1A,
	ESRIP_RAM1B,
	ESRIP_RAM1C,
	ESRIP_RAM1D,
	ESRIP_RAM1E,
	ESRIP_RAM1F,
	ESRIP_STATW,
	ESRIP_FDTC,
	ESRIP_IPTC,
	ESRIP_XSCALE,
	ESRIP_YSCALE,
	ESRIP_LINE,
	ESRIP_FIG,
	ESRIP_ATTR,
	ESRIP_ADRL,
	ESRIP_ADRR,
	ESRIP_COLR,
	ESRIP_IADDR,
};

typedef struct _esrip_config_ esrip_config;
struct _esrip_config_
{
	read16_handler	fdt_r;
	write16_handler	fdt_w;
	UINT8 (*status_in)(void);
	int (*draw)(int l, int r, int fig, int attr, int addr, int col, int x_scale, int line_latch);
	int lbrm_region;
};

void esrip_get_info(UINT32 state, union cpuinfo *info);

extern UINT8 esrip_get_status(int cpunum);

#endif

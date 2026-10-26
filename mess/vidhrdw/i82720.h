// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Miodrag Milanovic, Carl
/******************************************************************************

	i82720.h
	Video driver for the Intel 82720 and NEC uPD7220 GDC.

******************************************************************************/

#ifndef __I82720_H__
#define __I82720_H__

#include "driver.h"
#include "i82720cm.h"

#define COMPIS_PALETTE_SIZE 16

/* I/O functions */
READ8_HANDLER( compis_gdc_r );
WRITE8_HANDLER( compis_gdc_w );

UINT8 i82720_r(int offset);
void i82720_w(int offset, UINT8 data);

void compis_gdc_vblank_int(void);
void compis_gdc_reset(void);

PALETTE_INIT( compis_gdc );
VIDEO_START( compis_gdc );
VIDEO_UPDATE( compis_gdc );

typedef enum
{
	GDC_MODE_HRG,
	GDC_MODE_UHRG
} compis_gdc_modes;

typedef struct compis_gdc_interface
{
	compis_gdc_modes mode;
	UINT32 vramsize;
} compis_gdc_interface;

extern void mdrv_compisgdc(machine_config *machine, const compis_gdc_interface *intf);

#define MDRV_COMPISGDC(intf) mdrv_compisgdc(machine, (intf));

#endif

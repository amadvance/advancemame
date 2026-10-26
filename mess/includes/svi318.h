// license:GPL-2.0+
// copyright-holders:Dirk Best
/*****************************************************************************
 *
 * includes/svi318.h
 *
 * Spectravideo SVI-318 / SVI-328
 *
 ****************************************************************************/

#ifndef __SVI318_H__
#define __SVI318_H__

#include "driver.h"

typedef struct {
	/* machine state */
	int is_svi328;
	int intvdp;
	int intexp;
	int romdis;
	int ramdis;
	int cart;
	int bk21;
	int bk22;
	int bk31;
	int bk32;
	int rom2;
	int rom3;
	int ctrl1;
	UINT8 keyboard_row;
	UINT8 bank_switch;

	/* memory buffers */
	UINT8 *empty_bank;
	UINT8 *basic_rom;
	UINT8 *cart_rom;
	UINT8 *ram;
	UINT8 *ram_bank1;
	UINT8 *ram_bank2;

	/* printer */
	UINT8 prn_data;
	UINT8 prn_strobe;

	/* fdc */
	UINT8 fdc_driveselect;
	UINT8 fdc_irq_drq;
	UINT8 fdc_heads[2];

	/* 80-column card (SV-806) */
	UINT8 sv806_ram_enabled;
	UINT8 *sv806_ram;
} SVI_318;

extern SVI_318 svi;

DRIVER_INIT( svi318 );
MACHINE_RESET( svi318 );

DEVICE_LOAD( svi318_cart );
DEVICE_UNLOAD( svi318_cart );

INTERRUPT_GEN( svi318_interrupt );
void svi318_vdp_interrupt( int state );

WRITE8_HANDLER( svi318_writemem0 );
WRITE8_HANDLER( svi318_writemem1 );

READ8_HANDLER( svi318_ppi_r );
WRITE8_HANDLER( svi318_ppi_w );

READ8_HANDLER( svi318_psg_port_a_r );
WRITE8_HANDLER( svi318_psg_port_b_w );

READ8_HANDLER( svi318_printer_r );
WRITE8_HANDLER( svi318_printer_w );

DEVICE_LOAD( svi318_floppy );
READ8_HANDLER( svi318_fdc_irqdrq_r );
WRITE8_HANDLER( svi318_fdc_drive_motor_w );
WRITE8_HANDLER( svi318_fdc_density_side_w );

READ8_HANDLER( svi318_crtc_r );
WRITE8_HANDLER( svi318_crtc_w );
WRITE8_HANDLER( svi318_crtcbank_w );
VIDEO_UPDATE( svi328b );
MACHINE_RESET( svi328b );

int svi318_cassette_present( int id );

#endif /* __SVI318_H__ */

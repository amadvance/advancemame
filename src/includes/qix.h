// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Zsolt Vasvari
// thanks-to:John Butler, Ed Mueller
/***************************************************************************

    Taito Qix hardware

***************************************************************************/

#include "sound/discrete.h"

#define MAIN_CLOCK_OSC			20000000	/* 20 MHz */
#define SLITHER_CLOCK_OSC		21300000	/* 21.3 MHz */
#define SOUND_CLOCK_OSC			7372800		/* 7.3728 MHz */
#define COIN_CLOCK_OSC			4000000		/* 4 MHz */
#define QIX_CHARACTER_CLOCK		(20000000/2/16)


/*----------- defined in machine/qix.c -----------*/

extern UINT8 *qix_68705_port_out;
extern UINT8 *qix_68705_ddr;

MACHINE_START( qix );
MACHINE_START( qixmcu );
MACHINE_START( slither );
MACHINE_RESET( qix );
MACHINE_RESET( qixmcu );
MACHINE_RESET( slither );

WRITE8_HANDLER( zookeep_bankswitch_w );

READ8_HANDLER( qix_data_firq_r );
READ8_HANDLER( qix_data_firq_ack_r );
WRITE8_HANDLER( qix_data_firq_w );
WRITE8_HANDLER( qix_data_firq_ack_w );

READ8_HANDLER( qix_video_firq_r );
READ8_HANDLER( qix_video_firq_ack_r );
WRITE8_HANDLER( qix_video_firq_w );
WRITE8_HANDLER( qix_video_firq_ack_w );

READ8_HANDLER( qix_68705_portA_r );
READ8_HANDLER( qix_68705_portB_r );
READ8_HANDLER( qix_68705_portC_r );
WRITE8_HANDLER( qix_68705_portA_w );
WRITE8_HANDLER( qix_68705_portB_w );
WRITE8_HANDLER( qix_68705_portC_w );

WRITE8_HANDLER( qix_pia_0_w );

INTERRUPT_GEN( qix_vblank_start );


/*----------- defined in vidhrdw/qix.c -----------*/

extern UINT8 *qix_videoaddress;
extern UINT8 qix_cocktail_flip;

VIDEO_START( qix );
VIDEO_UPDATE( qix );

READ8_HANDLER( qix_scanline_r );
READ8_HANDLER( qix_videoram_r );
WRITE8_HANDLER( qix_videoram_w );
WRITE8_HANDLER( slither_videoram_w );
READ8_HANDLER( qix_addresslatch_r );
WRITE8_HANDLER( qix_addresslatch_w );
WRITE8_HANDLER( slither_addresslatch_w );
WRITE8_HANDLER( slither_vram_mask_w );
WRITE8_HANDLER( qix_paletteram_w );
WRITE8_HANDLER( qix_palettebank_w );
WRITE8_HANDLER( qix_flip_screen_w );


/*----------- defined in sndhrdw/qix.c -----------*/

extern struct discrete_sound_block qix_discrete_interface[];

WRITE8_HANDLER( qix_dac_w );
WRITE8_HANDLER( qix_vol_w );

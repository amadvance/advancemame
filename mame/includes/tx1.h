// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Tatsumi TX-1/Buggy Boy hardware

***************************************************************************/
#ifndef TX1_H
#define TX1_H
#include "machine/8255ppi.h"

#define TX1_PIXEL_CLOCK		(18000000 / 3)
#define TX1_HBSTART			256
#define TX1_HBEND			0
#define TX1_HTOTAL			384
#define TX1_VBSTART			240
#define TX1_VBEND			0
#define TX1_VTOTAL			264

#define BB_PIXEL_CLOCK		(18000000 / 3)
#define BB_HBSTART			256
#define BB_HBEND			0
#define BB_HTOTAL			384
#define BB_VBSTART			240
#define BB_VBEND			0
#define BB_VTOTAL			288

#define CPU_MASTER_CLOCK	(15000000)
#define BUGGYBOY_ZCLK		(CPU_MASTER_CLOCK / 2)

extern UINT16 *tx1_math_ram;

READ16_HANDLER( tx1_spcs_rom_r );
READ16_HANDLER( tx1_spcs_ram_r );
WRITE16_HANDLER( tx1_spcs_ram_w );
READ16_HANDLER( tx1_math_r );
WRITE16_HANDLER( tx1_math_w );
MACHINE_RESET( tx1 );

READ16_HANDLER( buggyboy_spcs_rom_r );
READ16_HANDLER( buggyboy_spcs_ram_r );
WRITE16_HANDLER( buggyboy_spcs_ram_w );
READ16_HANDLER( buggyboy_math_r );
WRITE16_HANDLER( buggyboy_math_w );
MACHINE_RESET( buggyb1 );
MACHINE_RESET( buggyboy );

READ8_HANDLER( tx1_pit8253_r );
WRITE8_HANDLER( tx1_pit8253_w );

WRITE8_HANDLER( bb_ym1_a_w );
WRITE8_HANDLER( bb_ym2_a_w );
WRITE8_HANDLER( bb_ym2_b_w );
WRITE8_HANDLER( tx1_ay8910_a_w );
WRITE8_HANDLER( tx1_ay8910_b_w );


READ16_HANDLER( tx1_crtc_r );
WRITE16_HANDLER( tx1_crtc_w );

extern UINT16 *tx1_vram;
extern UINT16 *tx1_objram;
extern UINT16 *tx1_rcram;
extern size_t tx1_objram_size;
PALETTE_INIT( tx1 );
VIDEO_START( tx1 );
VIDEO_UPDATE( tx1 );
VIDEO_EOF( tx1 );
WRITE16_HANDLER( tx1_slincs_w );
WRITE16_HANDLER( tx1_slock_w );
WRITE16_HANDLER( tx1_scolst_w );
WRITE16_HANDLER( tx1_bankcs_w );
WRITE16_HANDLER( tx1_flgcs_w );

extern UINT16 *buggyboy_objram;
extern UINT16 *buggyboy_rcram;
extern UINT16 *buggyboy_vram;
extern size_t buggyboy_objram_size;
extern size_t buggyboy_rcram_size;
PALETTE_INIT( buggyboy );
VIDEO_START( buggyboy );
VIDEO_UPDATE( buggyboy );
VIDEO_EOF( buggyboy );

VIDEO_START( buggyb1 );
VIDEO_UPDATE( buggyb1 );
WRITE16_HANDLER( buggyboy_slincs_w );
WRITE16_HANDLER( buggyboy_scolst_w );
WRITE16_HANDLER( buggyboy_gas_w );
WRITE16_HANDLER( buggyboy_sky_w );

#include "sound/custom.h"
extern struct CustomSound_interface tx1_custom_interface;
extern struct CustomSound_interface buggyboy_custom_interface;

#endif

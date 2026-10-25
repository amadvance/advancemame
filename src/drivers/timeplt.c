// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

Time Pilot memory map (preliminary)

driver by Nicola Salmoria

Main processor memory map.
0000-5fff ROM
a000-a3ff Color RAM
a400-a7ff Video RAM
a800-afff RAM
b000-b7ff sprite RAM (only areas 0xb010 and 0xb410 are used).

memory mapped ports:

read:
c000      video scan line. This is used by the program to multiplex the cloud
          sprites, drawing them twice offset by 128 pixels.
c200      DSW2
c300      IN0
c320      IN1
c340      IN2
c360      DSW1

write:
c000      command for the audio CPU
c200      watchdog reset
c300      interrupt enable
c302      flip screen
c304      trigger interrupt on audio CPU
c308      Protection ???  Stuffs in some values computed from ROM content
c30a      coin counter 1
c30c      coin counter 2

interrupts:
standard NMI at 0x66

SOUND BOARD:
same as Pooyan

***************************************************************************/

#include "driver.h"
#include "sndhrdw/timeplt.h"
#include "machine/ticket.h"
#include "sound/custom.h"


extern unsigned char *timeplt_videoram,*timeplt_colorram;

READ8_HANDLER( timeplt_scanline_r );
WRITE8_HANDLER( timeplt_videoram_w );
WRITE8_HANDLER( timeplt_colorram_w );
WRITE8_HANDLER( timeplt_flipscreen_w );
VIDEO_START( timeplt );
VIDEO_START( bikkuric );
VIDEO_UPDATE( bikkuric );
WRITE8_HANDLER( bikkuric_video_enable_w );
PALETTE_INIT( timeplt );
VIDEO_UPDATE( timeplt );
INTERRUPT_GEN( timeplt_interrupt );



static WRITE8_HANDLER( timeplt_coin_counter_w )
{
	coin_counter_w(offset >> 1, data);
}

static READ8_HANDLER( psurge_protection_r )
{
	return 0x80;
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6004, 0x6004) AM_READ(psurge_protection_r)	/* psurge only */
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc000, 0xc000) AM_READ(timeplt_scanline_r)
	AM_RANGE(0xc200, 0xc200) AM_READ(input_port_4_r)	/* DSW2 */
	AM_RANGE(0xc300, 0xc300) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0xc320, 0xc320) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0xc340, 0xc340) AM_READ(input_port_2_r)	/* IN2 */
	AM_RANGE(0xc360, 0xc360) AM_READ(input_port_3_r)	/* DSW1 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xa000, 0xa3ff) AM_WRITE(timeplt_colorram_w) AM_BASE(&timeplt_colorram)
	AM_RANGE(0xa400, 0xa7ff) AM_WRITE(timeplt_videoram_w) AM_BASE(&timeplt_videoram)
	AM_RANGE(0xa800, 0xafff) AM_WRITE(MWA8_RAM)

	AM_RANGE(0xb000, 0xb00f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xb010, 0xb03f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xb040, 0xb40f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xb410, 0xb43f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)
	AM_RANGE(0xb440, 0xbfff) AM_WRITE(MWA8_RAM)

	AM_RANGE(0xc000, 0xc000) AM_WRITE(soundlatch_w)
	AM_RANGE(0xc200, 0xc200) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xc300, 0xc300) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0xc302, 0xc302) AM_WRITE(timeplt_flipscreen_w)
	AM_RANGE(0xc304, 0xc304) AM_WRITE(timeplt_sh_irqtrigger_w)
	AM_RANGE(0xc30a, 0xc30c) AM_WRITE(timeplt_coin_counter_w)  /* c30b is not used */
ADDRESS_MAP_END

static READ8_HANDLER( peni_input_r )
{
	return (readinputport(1) & ~2) | ((ticket_dispenser_r(0) >> 6) & 2);
}
static WRITE8_HANDLER( peni_hopper_w )
{
	ticket_dispenser_w(0, (data & 1) << 7);
}
static MACHINE_RESET( peni )
{
	ticket_dispenser_w(0, 0);
	cpu_interrupt_enable(0, 0);
	bikkuric_video_enable_w(0, 0);
}
static DRIVER_INIT( peni )
{
	ticket_dispenser_init(100, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_LOW);
}
static ADDRESS_MAP_START( peni_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x6000, 0x67ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc320, 0xc320) AM_READ(peni_input_r) AM_MIRROR(0x0c9f)
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc000, 0xc000) AM_READ(timeplt_scanline_r) AM_MIRROR(0x0cff)
	AM_RANGE(0xc200, 0xc200) AM_READ(input_port_4_r) AM_MIRROR(0x0cff)
	AM_RANGE(0xc300, 0xc300) AM_READ(input_port_0_r) AM_MIRROR(0x0c9f)
	AM_RANGE(0xc340, 0xc340) AM_READ(input_port_2_r) AM_MIRROR(0x0c9f)
	AM_RANGE(0xc360, 0xc360) AM_READ(input_port_3_r) AM_MIRROR(0x0c9f)
ADDRESS_MAP_END
static ADDRESS_MAP_START( peni_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x67ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc308, 0xc308) AM_WRITE(bikkuric_video_enable_w)
	AM_RANGE(0xc30a, 0xc30a) AM_WRITE(peni_hopper_w)
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xa000, 0xa3ff) AM_WRITE(timeplt_colorram_w) AM_BASE(&timeplt_colorram)
	AM_RANGE(0xa400, 0xa7ff) AM_WRITE(timeplt_videoram_w) AM_BASE(&timeplt_videoram)
	AM_RANGE(0xa800, 0xafff) AM_WRITE(MWA8_RAM)

	AM_RANGE(0xb000, 0xb00f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xb010, 0xb03f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xb040, 0xb40f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xb410, 0xb43f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)
	AM_RANGE(0xb440, 0xbfff) AM_WRITE(MWA8_RAM)

	AM_RANGE(0xc000, 0xc000) AM_WRITE(soundlatch_w) AM_MIRROR(0x0cff)
	AM_RANGE(0xc200, 0xc200) AM_WRITE(watchdog_reset_w) AM_MIRROR(0x0cff)
	AM_RANGE(0xc300, 0xc300) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0xc302, 0xc302) AM_WRITE(timeplt_flipscreen_w)
	AM_RANGE(0xc304, 0xc304) AM_WRITE(timeplt_sh_irqtrigger_w)
	AM_RANGE(0xc30a, 0xc30c) AM_WRITE(timeplt_coin_counter_w)
ADDRESS_MAP_END

#define TIMEPILOT_INPUTS \
	PORT_START_TAG("IN0")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_START_TAG("IN1")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_START_TAG("IN2")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( timeplt )
TIMEPILOT_INPUTS

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "255 (Cheat)")
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, "Bonus" )
	PORT_DIPSETTING(    0x08, "10000 50000" )
	PORT_DIPSETTING(    0x00, "20000 60000" )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x70, "1 (Easiest)" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x50, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x30, "5 (Average)" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPSETTING(    0x10, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hardest)" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( psurge )
TIMEPILOT_INPUTS

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Initial Energy" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x08, "6" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x10, 0x10, "Infinite Shots (Cheat)")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Stop at Junctions" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END



INPUT_PORTS_START( chkun )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Key Out")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Bet 3B")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Bet 1B")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Bet 2B")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Bet HR")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bikkuric )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Key Out")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8*8+0,8*8+1,8*8+2,8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};
static const gfx_layout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3,  8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,        0, 32 },
	{ REGION_GFX2, 0, &spritelayout,   32*4, 64 },
	{ -1 } /* end of array */
};



static const gfx_layout peni_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8*8+0,8*8+1,8*8+2,8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};
static const gfx_layout peni_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3,  8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};
static const gfx_decode peni_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &peni_charlayout, 0, 32 },
	{ REGION_GFX2, 0, &peni_spritelayout, 32*4, 64 },
	{ -1 }
};

static MACHINE_DRIVER_START( timeplt )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3072000)	/* 3.072 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(timeplt_interrupt,256)

	MDRV_CPU_ADD_TAG("sound", Z80,14318180/8)
	/* audio CPU */	/* 1.789772727 MHz */						\
	MDRV_CPU_PROGRAM_MAP(timeplt_sound_readmem,timeplt_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32*4+64*4)

	MDRV_PALETTE_INIT(timeplt)
	MDRV_VIDEO_START(timeplt)
	MDRV_VIDEO_UPDATE(timeplt)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 14318180/8)
	MDRV_SOUND_CONFIG(timeplt_ay8910_interface)
	MDRV_SOUND_ROUTE(0, "filter.0.0", 0.60)
	MDRV_SOUND_ROUTE(1, "filter.0.1", 0.60)
	MDRV_SOUND_ROUTE(2, "filter.0.2", 0.60)

	MDRV_SOUND_ADD_TAG("ay2", AY8910, 14318180/8)
	MDRV_SOUND_ROUTE(0, "filter.1.0", 0.60)
	MDRV_SOUND_ROUTE(1, "filter.1.1", 0.60)
	MDRV_SOUND_ROUTE(2, "filter.1.2", 0.60)

	MDRV_SOUND_ADD_TAG("filter.0.0", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.0.1", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.0.2", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD_TAG("filter.1.0", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.1.1", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD_TAG("filter.1.2", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

static MACHINE_DRIVER_START( bikkuric )
	MDRV_IMPORT_FROM(timeplt)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(peni_readmem, peni_writemem)
	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PROGRAM_MAP(peni_sound_map, 0)
	MDRV_MACHINE_RESET(peni)
	MDRV_GFXDECODE(peni_gfxdecodeinfo)
	MDRV_VIDEO_START(bikkuric)
	MDRV_VIDEO_UPDATE(bikkuric)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( chkun )
	MDRV_IMPORT_FROM(bikkuric)
	MDRV_SOUND_MODIFY("ay2")
	MDRV_SOUND_CONFIG(chkun_ay8910_interface)
	MDRV_SOUND_ROUTE(0, "filter.1.0", 0.60)
	MDRV_SOUND_ROUTE(1, "filter.1.1", 0.60)
	MDRV_SOUND_ROUTE(2, "filter.1.2", 0.60)
	MDRV_SOUND_ADD(CUSTOM, 512000)
	MDRV_SOUND_CONFIG(chkun_voice_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_DRIVER_END

ROM_START( timeplt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tm1",          0x0000, 0x2000, CRC(1551f1b9) SHA1(c72f30988ac00cbe6549b71c3bcb414511e8b997) )
	ROM_LOAD( "tm2",          0x2000, 0x2000, CRC(58636cb5) SHA1(ab517efa93ae7be780af55faea82a6e83edd828c) )
	ROM_LOAD( "tm3",          0x4000, 0x2000, CRC(ff4e0d83) SHA1(ef98a1abb45b22d7498a0aca520f43bbee248b22) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "tm7",          0x0000, 0x1000, CRC(d66da813) SHA1(408fca4515e8af84211df3e204c8776b2f8adb23) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm6",          0x0000, 0x2000, CRC(c2507f40) SHA1(07221875e3f81d9def67c57a7ccd82d52ce65e01) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tm4",          0x0000, 0x2000, CRC(7e437c3e) SHA1(cbe2ccd2cd503af62f009cd5aab73aa7366230b1) )
	ROM_LOAD( "tm5",          0x2000, 0x2000, CRC(e8ca87b9) SHA1(5dd30d3fb9fd8cf9e6a8e37e7ea858c7fd038a7e) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, CRC(34c91839) SHA1(f62e279e21fce171231d3139be7adabe1f4b8c2e) ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, CRC(463b2b07) SHA1(9ad275365eba4869f94749f39ff8705d92056a10) ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, CRC(4bbb2150) SHA1(678433b21aae1daa938e32d3293eeed529a42ef9) ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, CRC(f7b7663e) SHA1(151bd2dff4e4ef76d6438c1ab2cae71f987b9dad) ) /* char lookup table */
ROM_END

ROM_START( timepltc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cd1y",         0x0000, 0x2000, CRC(83ec72c2) SHA1(f3dbc8362f6bdad1baa65cf5d95611e79de381a4) )
	ROM_LOAD( "cd2y",         0x2000, 0x2000, CRC(0dcf5287) SHA1(c36628367e81ac07f5ace72b45ebb7140b6aa116) )
	ROM_LOAD( "cd3y",         0x4000, 0x2000, CRC(c789b912) SHA1(dead7b20a40769e48738fccc3a17e2266aac445d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "tm7",          0x0000, 0x1000, CRC(d66da813) SHA1(408fca4515e8af84211df3e204c8776b2f8adb23) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm6",          0x0000, 0x2000, CRC(c2507f40) SHA1(07221875e3f81d9def67c57a7ccd82d52ce65e01) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tm4",          0x0000, 0x2000, CRC(7e437c3e) SHA1(cbe2ccd2cd503af62f009cd5aab73aa7366230b1) )
	ROM_LOAD( "tm5",          0x2000, 0x2000, CRC(e8ca87b9) SHA1(5dd30d3fb9fd8cf9e6a8e37e7ea858c7fd038a7e) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, CRC(34c91839) SHA1(f62e279e21fce171231d3139be7adabe1f4b8c2e) ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, CRC(463b2b07) SHA1(9ad275365eba4869f94749f39ff8705d92056a10) ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, CRC(4bbb2150) SHA1(678433b21aae1daa938e32d3293eeed529a42ef9) ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, CRC(f7b7663e) SHA1(151bd2dff4e4ef76d6438c1ab2cae71f987b9dad) ) /* char lookup table */
ROM_END

ROM_START( spaceplt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sp1",          0x0000, 0x2000, CRC(ac8ca3ae) SHA1(9781138becd17aa70e877138e126ebb1fbff6192) )
	ROM_LOAD( "sp2",          0x2000, 0x2000, CRC(1f0308ef) SHA1(dd88378fc4cefe473f310d4730268c98354a4a44) )
	ROM_LOAD( "sp3",          0x4000, 0x2000, CRC(90aeca50) SHA1(9c6fddfeafa84f5284ec8f7c9d46216b110badc1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "tm7",          0x0000, 0x1000, CRC(d66da813) SHA1(408fca4515e8af84211df3e204c8776b2f8adb23) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sp6",          0x0000, 0x2000, CRC(76caa8af) SHA1(f81bb73877d415a6587a32bddaad6db8a8fd4941) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sp4",          0x0000, 0x2000, CRC(3781ce7a) SHA1(68bb73f67494c3b24f7fd0d79153c9793f4b3a5b) )
	ROM_LOAD( "tm5",          0x2000, 0x2000, CRC(e8ca87b9) SHA1(5dd30d3fb9fd8cf9e6a8e37e7ea858c7fd038a7e) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, CRC(34c91839) SHA1(f62e279e21fce171231d3139be7adabe1f4b8c2e) ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, CRC(463b2b07) SHA1(9ad275365eba4869f94749f39ff8705d92056a10) ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, CRC(4bbb2150) SHA1(678433b21aae1daa938e32d3293eeed529a42ef9) ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, CRC(f7b7663e) SHA1(151bd2dff4e4ef76d6438c1ab2cae71f987b9dad) ) /* char lookup table */
ROM_END

ROM_START( psurge )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p1",           0x0000, 0x2000, CRC(05f9ba12) SHA1(ad88838d1a0c64830281e425d4ad2498ba959098) )
	ROM_LOAD( "p2",           0x2000, 0x2000, CRC(3ff41576) SHA1(9bdbad31c65dff76942967b5a334407b0326f752) )
	ROM_LOAD( "p3",           0x4000, 0x2000, CRC(e8fe120a) SHA1(b6320c9cb1a67097692aa0de7d88b0dfb63dedd7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "p6",           0x0000, 0x1000, CRC(b52d01fa) SHA1(9b6cf9ea51d3a87c174f34d42a4b1b5f38b48723) )
	ROM_LOAD( "p7",           0x1000, 0x1000, CRC(9db5c0ce) SHA1(b5bc1d89a7f7d7a0baae64390c37ee11f69a0e76) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "p4",           0x0000, 0x2000, CRC(26fd7f81) SHA1(eb282313a37d7d611bf90f9b0b527adee9ae283f) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "p5",           0x0000, 0x2000, CRC(6066ec8e) SHA1(7f1155cf8a2d63c0740a4b56f1e09e7dfc749302) )
	ROM_LOAD( "tm5",          0x2000, 0x2000, CRC(e8ca87b9) SHA1(5dd30d3fb9fd8cf9e6a8e37e7ea858c7fd038a7e) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, BAD_DUMP CRC(34c91839) SHA1(f62e279e21fce171231d3139be7adabe1f4b8c2e)  ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, BAD_DUMP CRC(463b2b07) SHA1(9ad275365eba4869f94749f39ff8705d92056a10)  ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, BAD_DUMP CRC(4bbb2150) SHA1(678433b21aae1daa938e32d3293eeed529a42ef9)  ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, BAD_DUMP CRC(f7b7663e) SHA1(151bd2dff4e4ef76d6438c1ab2cae71f987b9dad)  ) /* char lookup table */
ROM_END

ROM_START( timeplta )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "cd_e1.bin",         0x0000, 0x2000, CRC(a4513b35) SHA1(1b1944ec5317d71af86e21e0691caae180dee7b5) )
	ROM_LOAD( "cd_e2.bin",         0x2000, 0x2000, CRC(38b0c72a) SHA1(8f0950deb2f9e2b65714318b9e837a1c837f52a9) )
	ROM_LOAD( "cd_e3.bin",         0x4000, 0x2000, CRC(83846870) SHA1(b1741e7e5674f9e63e113ead0cb7f5ef874eac5f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "tm7",          0x0000, 0x1000, CRC(d66da813) SHA1(408fca4515e8af84211df3e204c8776b2f8adb23) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm6",          0x0000, 0x2000, CRC(c2507f40) SHA1(07221875e3f81d9def67c57a7ccd82d52ce65e01) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tm4",          0x0000, 0x2000, CRC(7e437c3e) SHA1(cbe2ccd2cd503af62f009cd5aab73aa7366230b1) )
	ROM_LOAD( "tm5",          0x2000, 0x2000, CRC(e8ca87b9) SHA1(5dd30d3fb9fd8cf9e6a8e37e7ea858c7fd038a7e) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, CRC(34c91839) SHA1(f62e279e21fce171231d3139be7adabe1f4b8c2e) ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, CRC(463b2b07) SHA1(9ad275365eba4869f94749f39ff8705d92056a10) ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, CRC(4bbb2150) SHA1(678433b21aae1daa938e32d3293eeed529a42ef9) ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, CRC(f7b7663e) SHA1(151bd2dff4e4ef76d6438c1ab2cae71f987b9dad) ) /* char lookup table */
ROM_END



ROM_START( chkun )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )
	ROM_LOAD( "n1.16a",   0x0000, 0x4000, CRC(c5879f9b) SHA1(68e3a87dfe6b3d1e0cdadd1ed8ad115a9d3055f9) )
	ROM_LOAD( "12.14a",   0x4000, 0x2000, CRC(80cc55da) SHA1(68727721479624cd0d38d895b98dcef4edac13e9) )

	ROM_REGION( 0x12000, REGION_CPU2, 0 )
	ROM_LOAD( "15.3l",    0x0000, 0x2000, CRC(1f1463ca) SHA1(870abbca35236fcce6a2f640a238e20b9e57f10f))

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "13.4d",   0x0000, 0x4000, CRC(776427c0) SHA1(1e8387685f7e86aad31577f2186596b2a2dfc4de) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14.8h",   0x0000, 0x4000, CRC(0cb76a48) SHA1(0beebbc3d30eb978f6fe7b15c0a7b0c2152815b7) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "v1.8k",   0x00000, 0x10000, CRC(d5ca802d) SHA1(0c2867c86132745063e36d03f41e6c3e150fd3ad) )
	ROM_LOAD( "v2.9k",   0x10000, 0x10000, CRC(70e902eb) SHA1(e1b2392446f2878f9e811a63bbbbdc56fd517a9c) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "3.2j",        0x0000, 0x0020, CRC(34c91839) SHA1(f62e279e21fce171231d3139be7adabe1f4b8c2e) )
	ROM_LOAD( "2.1h",        0x0020, 0x0020, CRC(463b2b07) SHA1(9ad275365eba4869f94749f39ff8705d92056a10) )
	ROM_LOAD( "4.10h",       0x0040, 0x0100, CRC(4bbb2150) SHA1(678433b21aae1daa938e32d3293eeed529a42ef9) )
	ROM_LOAD( "mb7114e.2b",  0x0140, 0x0100, CRC(adfa399a) SHA1(3b18971ddaae7734f0aaa0fcc82d4ddccc282959) )

	ROM_REGION( 0x0200, REGION_USER1, 0 )
	ROM_LOAD( "a.16c",  0x0000, 0x00eb, CRC(e0d54999) SHA1(9e1c749873572ade2b925ce1519d31a6e19f841f) )
	ROM_LOAD( "b.9f",   0x0100, 0x00eb, CRC(e3857f83) SHA1(674e70dc960fc02a9fbda4a0ef0770eb8214c466) )
ROM_END

ROM_START( bikkuric )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.a16", 0x00000, 0x04000, CRC(e8d595ab) SHA1(01f6a5321274befcd03a0ec18ed9770aca4527b6) )
	ROM_LOAD( "2.a14", 0x04000, 0x02000, CRC(63fd7d53) SHA1(b1ef666453c5c9e344bee544a0673068d60158fa) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "5.l3",  0x00000, 0x02000, CRC(bc438531) SHA1(e19badc417b0538010cf535d3f733acc54b0cd96) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3.d4",  0x00000, 0x08000, CRC(74e8a64b) SHA1(b2542e1f6f4b54d8f7aec8f673cedcf5bff5e429) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "4.h8",  0x00000, 0x02000, CRC(d303942d) SHA1(688d43e6dbe505d44fc41fdde74858a02910080d) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "3.2j",  0x0000, 0x0020, BAD_DUMP CRC(34c91839) SHA1(f62e279e21fce171231d3139be7adabe1f4b8c2e) )
	ROM_LOAD( "2.1h",  0x0020, 0x0020, BAD_DUMP CRC(463b2b07) SHA1(9ad275365eba4869f94749f39ff8705d92056a10) )
	ROM_LOAD( "4.10h", 0x0040, 0x0100, BAD_DUMP CRC(4bbb2150) SHA1(678433b21aae1daa938e32d3293eeed529a42ef9) )
	ROM_LOAD( "1.2b",  0x0140, 0x0100, BAD_DUMP CRC(f7b7663e) SHA1(151bd2dff4e4ef76d6438c1ab2cae71f987b9dad) )

	ROM_REGION( 0x0200, REGION_USER1, 0 )
	ROM_LOAD( "a.16c",  0x0000, 0x00eb, NO_DUMP )
	ROM_LOAD( "b.9f",   0x0100, 0x00eb, NO_DUMP )
ROM_END

GAME( 1982, timeplt,  0,       timeplt, timeplt, 0, ROT90,  "Konami", "Time Pilot", 0 )
GAME( 1982, timepltc, timeplt, timeplt, timeplt, 0, ROT90,  "Konami (Centuri license)", "Time Pilot (Centuri)", 0 )
GAME( 1982, timeplta, timeplt, timeplt, timeplt, 0, ROT90,  "Konami (Atari license)", "Time Pilot (Atari)", 0 )
GAME( 1982, spaceplt, timeplt, timeplt, timeplt, 0, ROT90,  "bootleg", "Space Pilot", 0 )
GAME( 1988, psurge,   0,       timeplt, psurge,  0, ROT270, "<unknown>", "Power Surge", 0 )

GAME( 1988, chkun, 0, chkun, chkun, peni, ROT90, "Peni", "Chance Kun (Japan)", GAME_SUPPORTS_SAVE | GAME_IMPERFECT_SOUND )
GAME( 1987, bikkuric, 0, bikkuric, bikkuric, peni, ROT90, "Peni", "Bikkuri Card (Japan)", GAME_SUPPORTS_SAVE )

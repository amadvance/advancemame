/*****************************************************************************

    Irem M90/M97 system games:

    Hasamu                                                  1991 M90
    Bomberman / Atomic Punk / Dynablaster                   1992 M90
    Bomberman World / New Atomic Punk / New Dyna Blaster    1992 M99 A
    Quiz F-1 1,2finish                                      1992 M97
    Risky Challenge / Gussun Oyoyo                          1993 M97
    Match It II / Shisensho II                              1993 M97-A


    Uses M72 sound hardware.

    Emulation by Bryan McPhail, mish@tendril.co.uk, thanks to Chris Hardy!

Notes:

- Not sure about the clock speeds. In hasamu and quizf1 service mode, the
  selection moves too fast with the clock set at 16 MHz. It's still fast at
  8 MHz, but at least it's usable.

*****************************************************************************/

#include "driver.h"
#include "m92.h"
#include "machine/irem_cpu.h"
#include "sndhrdw/m72.h"
#include "sound/dac.h"
#include "sound/2151intf.h"

static UINT32 bankaddress;

extern UINT8 *m90_video_data;

VIDEO_UPDATE( m90 );
VIDEO_UPDATE( m90_bootleg );
WRITE8_HANDLER( m90_video_control_w );
WRITE8_HANDLER( m90_video_w );
VIDEO_START( m90 );

/***************************************************************************/

static void set_m90_bank(void)
{
	UINT8 *rom = memory_region(REGION_USER1);

	if (!rom)
		ui_popup("bankswitch with no banked ROM!");
	else
		memory_set_bankptr(1,rom + bankaddress);
}

/***************************************************************************/

static WRITE8_HANDLER( m90_coincounter_w )
{
	if (offset==0)
	{
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		if (data&0xfe) logerror("Coin counter %02x\n",data);
	}
}

static WRITE8_HANDLER( quizf1_bankswitch_w )
{
	if (offset == 0)
	{
		bankaddress = 0x10000 * (data & 0x0f);
		set_m90_bank();
	}
}

/***************************************************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x7ffff) AM_READ(MRA8_ROM)
	AM_RANGE(0x80000, 0x8ffff) AM_READ(MRA8_BANK1)	/* Quiz F1 only */
	AM_RANGE(0xa0000, 0xa3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd0000, 0xdffff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe0000, 0xe03ff) AM_READ(paletteram_r)
	AM_RANGE(0xffff0, 0xfffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x7ffff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x80000, 0x8ffff) AM_WRITE(MWA8_ROM)	/* Quiz F1 only */
	AM_RANGE(0xa0000, 0xa3fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xd0000, 0xdffff) AM_WRITE(m90_video_w) AM_BASE(&m90_video_data)
	AM_RANGE(0xe0000, 0xe03ff) AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_le_w) AM_BASE(&paletteram)
	AM_RANGE(0xffff0, 0xfffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bootleg_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x3ffff) AM_READ(MRA8_ROM)
	AM_RANGE(0x60000, 0x60fff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa0000, 0xa3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd0000, 0xdffff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe0000, 0xe03ff) AM_READ(paletteram_r)
	AM_RANGE(0xffff0, 0xfffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bootleg_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x3ffff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000e, 0x60fff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)
	AM_RANGE(0xa0000, 0xa3fff) AM_WRITE(MWA8_RAM)
//  AM_RANGE(0xd0000, 0xdffff) AM_WRITE(m90_bootleg_video_w) AM_BASE(&m90_video_data)
	AM_RANGE(0xe0000, 0xe03ff) AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_le_w) AM_BASE(&paletteram)
	AM_RANGE(0xffff0, 0xfffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r) /* Player 1 */
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r) /* Player 2 */
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r) /* Coins */
	AM_RANGE(0x03, 0x03) AM_READ(MRA8_NOP)		/* Unused?  High byte of above */
	AM_RANGE(0x04, 0x04) AM_READ(input_port_3_r) /* Dip 1 */
	AM_RANGE(0x05, 0x05) AM_READ(input_port_4_r) /* Dip 2 */
	AM_RANGE(0x06, 0x06) AM_READ(input_port_5_r) /* Player 3 */
	AM_RANGE(0x07, 0x07) AM_READ(input_port_6_r) /* Player 4 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x01) AM_WRITE(m72_sound_command_w)
	AM_RANGE(0x02, 0x03) AM_WRITE(m90_coincounter_w)
	AM_RANGE(0x04, 0x05) AM_WRITE(quizf1_bankswitch_w)
	AM_RANGE(0x80, 0x8f) AM_WRITE(m90_video_control_w)
ADDRESS_MAP_END

/*****************************************************************************/

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x80, 0x80) AM_READ(soundlatch_r)
	AM_RANGE(0x84, 0x84) AM_READ(m72_sample_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x80, 0x81) AM_WRITE(rtype2_sample_addr_w)
	AM_RANGE(0x82, 0x82) AM_WRITE(m72_sample_w)
	AM_RANGE(0x83, 0x83) AM_WRITE(m72_sound_irq_ack_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bbmanw_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x41, 0x41) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x42, 0x42) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bbmanw_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(poundfor_sample_addr_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x41, 0x41) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x42, 0x42) AM_WRITE(m72_sound_irq_ack_w)
ADDRESS_MAP_END

/*****************************************************************************/


INPUT_PORTS_START( hasamu )
	IREM_JOYSTICK_1_2(1)
	IREM_JOYSTICK_1_2(2)
	IREM_COINS

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2
INPUT_PORTS_END

INPUT_PORTS_START( dynablst )
	IREM_JOYSTICK_1_2(1)
	IREM_JOYSTICK_1_2(2)
	IREM_COINS

	PORT_START_TAG("DSWA")	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Title" )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, "Dynablaster" )
	PORT_DIPSETTING(    0x00, "Bomber Man" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSWB")	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, "2 Player Upright" )
	PORT_DIPSETTING(    0x06, "4 Player Upright A" ) /* Seperate Coin Slots */
	PORT_DIPSETTING(    0x02, "4 Player Upright B" ) /* Shared Coin Slots */
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )  /* This setting shows screen with offset, no cocktail support :-( */
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2

	IREM_JOYSTICK_3_4(3)
	IREM_JOYSTICK_3_4(4)
INPUT_PORTS_END

INPUT_PORTS_START( atompunk )
	PORT_INCLUDE(dynablst)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bombrman ) /* Does not appear to support 4 players or cocktail mode */
	IREM_JOYSTICK_1_2(1)
	IREM_JOYSTICK_1_2(2)
	IREM_COINS

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2
INPUT_PORTS_END

INPUT_PORTS_START( bbmanw )
	IREM_JOYSTICK_1_2(1)
	IREM_JOYSTICK_1_2(2)
	IREM_COINS

	PORT_START_TAG("DSWA")	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Title" )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, "Bomber Man World" )
	PORT_DIPSETTING(    0x00, "New Dyna Blaster Global Quest" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSWB")	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, "2 Player" )
	PORT_DIPSETTING(    0x06, "4 Player Seprate Coins" )		/* Each player has a seperate Coin Slot */
	PORT_DIPSETTING(    0x02, "4 Player Shared Coins" )		/* All 4 players Share coin 1&2 */
	PORT_DIPSETTING(    0x00, "4 Player 1&2 3&4 Share Coins" )	/* Players 1&2 share coin 1&2, Players 3&4 share coin 3&4 */
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2

	IREM_JOYSTICK_3_4(3)
	IREM_JOYSTICK_3_4(4)
INPUT_PORTS_END

INPUT_PORTS_START( bbmanwj )
	PORT_INCLUDE(bbmanw)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( quizf1 )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	IREM_COINS

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* Probably difficulty */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Input Device" )	/* input related (joystick/buttons select?) */
	PORT_DIPSETTING(    0x20, DEF_STR( Joystick ) )
	PORT_DIPSETTING(    0x00, "Buttons" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2
INPUT_PORTS_END

INPUT_PORTS_START( matchit2 )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, "Girls Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "China Tiles" )
	PORT_DIPSETTING(    0x02, "Mahjong" )
	PORT_DIPSETTING(    0x00, "Alpha-Numeric" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x30, 0x30, "Timer Speed" )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy )  )
	PORT_DIPNAME( 0x40, 0x40, "Title Screen" )
	PORT_DIPSETTING(    0x40, "Match It II" )
	PORT_DIPSETTING(    0x00, "Shisensho II" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x06, DEF_STR( English ) )
	PORT_DIPSETTING(    0x04, DEF_STR( German ) )
	PORT_DIPSETTING(    0x02, "Korean" )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2
INPUT_PORTS_END

INPUT_PORTS_START( shisen2 )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, "Girls Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x30, 0x30, "Timer Speed" )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy )  )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2
INPUT_PORTS_END

INPUT_PORTS_START( riskchal )
	IREM_JOYSTICK_1_2(1)
	IREM_JOYSTICK_1_2(2)
	IREM_COINS

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, "Game Title" )	/* Manual says "NOT USE" */
	PORT_DIPSETTING(    0x10, "Bomber Man World" )
	PORT_DIPSETTING(    0x00, "New Dyna Blaster Global Quest" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, "2 Player" )
	PORT_DIPSETTING(    0x06, "4 Player Seprate Coins" )		/* Each player has a seperate Coin Slot */
	PORT_DIPSETTING(    0x02, "4 Player Shared Coins" )		/* All 4 players Share coin 1&2 */
	PORT_DIPSETTING(    0x00, "4 Player 1&2 3&4 Share Coins" )	/* Players 1&2 share coin 1&2, Players 3&4 share coin 3&4 */
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	/* Coin Mode 1 */
	IREM_COIN_MODE_1_NEW
	/* Coin Mode 2, not supported yet */
//  IREM_COIN_MODE_2

	IREM_JOYSTICK_3_4(3)
	IREM_JOYSTICK_3_4(4)
INPUT_PORTS_END

/*****************************************************************************/

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },
	{ REGION_GFX1, 0, &spritelayout, 256, 16 },
	{ -1 } /* end of array */
};

/*****************************************************************************/

static struct YM2151interface ym2151_interface =
{
	m72_ym2151_irq_handler
};

static INTERRUPT_GEN( m90_interrupt )
{
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x60/4);
}

static INTERRUPT_GEN( bomblord_interrupt )
{
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x50/4);
}

static INTERRUPT_GEN( bbmanw_fake_nmi )
{
	int sample = m72_sample_r(0);
	if (sample)
		m72_sample_w(0,sample);
}

static MACHINE_DRIVER_START( m90 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 8 MHz ??????? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(m90_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* 3.579545 MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(m72_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(6*8, 54*8-1, 17*8, 47*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m90)
	MDRV_VIDEO_UPDATE(m90)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.90)
	MDRV_SOUND_ROUTE(1, "mono", 0.90)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( quizf1 )

	MDRV_IMPORT_FROM( m90 )
	MDRV_VISIBLE_AREA(6*8, 54*8-1, 17*8-8, 47*8-1+8)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bombrman )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 8 MHz ??????? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(m90_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* 3.579545 MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(m72_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(10*8, 50*8-1, 17*8, 47*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m90)
	MDRV_VIDEO_UPDATE(m90)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.90)
	MDRV_SOUND_ROUTE(1, "mono", 0.90)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bbmanw )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",V30,32000000/4)	/* 8 MHz ??????? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(m90_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* 3.579545 MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(bbmanw_sound_readport,bbmanw_sound_writeport)
	MDRV_CPU_VBLANK_INT(bbmanw_fake_nmi,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(m72_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(10*8, 50*8-1, 17*8, 47*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m90)
	MDRV_VIDEO_UPDATE(m90)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.90)
	MDRV_SOUND_ROUTE(1, "mono", 0.90)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bomblord )

	MDRV_IMPORT_FROM( bbmanw )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(bomblord_interrupt,1)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bootleg )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz */
	MDRV_CPU_PROGRAM_MAP(bootleg_readmem,bootleg_writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(m90_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* 3.579545 MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(m72_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m90)
	MDRV_VIDEO_UPDATE(m90)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.90)
	MDRV_SOUND_ROUTE(1, "mono", 0.90)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END

/***************************************************************************/

#define CODE_SIZE 0x100000

ROM_START( hasamu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "hasc-p1.bin",  0x00001, 0x20000, CRC(53df9834) SHA1(2e7e38157a497e3def69c4abcae5803f71a098da) )
	ROM_LOAD16_BYTE( "hasc-p0.bin",  0x00000, 0x20000, CRC(dff0ba6e) SHA1(83e20b3ae10b57c1e58d3d44bfca2ffd5f142056) )
	ROM_COPY( REGION_CPU1, 0x3fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hasc-sp.bin",    0x0000, 0x10000, CRC(259b1687) SHA1(39c3a89b1d0f5fec2a94a3048cc4639fe96820e2) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hasc-c0.bin",    0x000000, 0x20000, CRC(dd5a2174) SHA1(c28499419f961d126a838dd1390db74c1475ee02) )
	ROM_LOAD( "hasc-c1.bin",    0x020000, 0x20000, CRC(76b8217c) SHA1(8b21562875d856a1ce4863f325d049090f5716ae) )
	ROM_LOAD( "hasc-c2.bin",    0x040000, 0x20000, CRC(d90f9a68) SHA1(c9eab3e87dd5d3eb88461be493d88f5482c9e257) )
	ROM_LOAD( "hasc-c3.bin",    0x060000, 0x20000, CRC(6cfe0d39) SHA1(104feeacbbc86b168c41cd37fc5797781d9b5a0f) )

	ROM_REGION( 0x20000, REGION_SOUND1, ROMREGION_ERASE00 )	/* samples */
	/* No samples */
ROM_END

ROM_START( dynablst )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bbm-cp1e.62",   0x00001, 0x20000, CRC(27667681) SHA1(7d5f762026ea01817a65ea13b4b5793640e3e8fd) )
	ROM_LOAD16_BYTE( "bbm-cp0e.65",   0x00000, 0x20000, CRC(95db7a67) SHA1(1a224d73615a60530cbcc54fdbb526e8d5a6c555) )
	ROM_COPY( REGION_CPU1, 0x3fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm-sp.23",    0x0000, 0x10000, CRC(251090cd) SHA1(9245072c1afbfa3e4a1d1549942765d58bd78ed3) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bbm-c0.66",    0x000000, 0x40000, CRC(695d2019) SHA1(3537e9fb0e7dc13d6113b4af71cba3c73392335a) )
	ROM_LOAD( "bbm-c1.67",    0x040000, 0x40000, CRC(4c7c8bbc) SHA1(31ab5557d96c4184a9c02ed1c309f3070d148e25) )
	ROM_LOAD( "bbm-c2.68",    0x080000, 0x40000, CRC(0700d406) SHA1(0d43a31a726b0de0004beef41307de2508106b69) )
	ROM_LOAD( "bbm-c3.69",    0x0c0000, 0x40000, CRC(3c3613af) SHA1(f9554a73e95102333e449f6e81f2bb817ec00881) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "bbm-v0.20",    0x0000, 0x20000, CRC(0fa803fe) SHA1(d2ac1e624de38bed385442ceae09a76f203fa084) )
ROM_END

ROM_START( bombrman )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bbm-p1.62",   0x00001, 0x20000, CRC(982bd166) SHA1(ed67393ec319127616bff5fa3b7f84e8ac8e1d93) )
	ROM_LOAD16_BYTE( "bbm-p0.65",   0x00000, 0x20000, CRC(0a20afcc) SHA1(a42b7458938300b0c84c820c1ea627aed9080f1b) )
	ROM_COPY( REGION_CPU1, 0x3fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm-sp.23",    0x0000, 0x10000, CRC(251090cd) SHA1(9245072c1afbfa3e4a1d1549942765d58bd78ed3) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bbm-c0.66",    0x000000, 0x40000, CRC(695d2019) SHA1(3537e9fb0e7dc13d6113b4af71cba3c73392335a) )
	ROM_LOAD( "bbm-c1.67",    0x040000, 0x40000, CRC(4c7c8bbc) SHA1(31ab5557d96c4184a9c02ed1c309f3070d148e25) )
	ROM_LOAD( "bbm-c2.68",    0x080000, 0x40000, CRC(0700d406) SHA1(0d43a31a726b0de0004beef41307de2508106b69) )
	ROM_LOAD( "bbm-c3.69",    0x0c0000, 0x40000, CRC(3c3613af) SHA1(f9554a73e95102333e449f6e81f2bb817ec00881) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "bbm-v0.20",    0x0000, 0x20000, CRC(0fa803fe) SHA1(d2ac1e624de38bed385442ceae09a76f203fa084) )
ROM_END

ROM_START( atompunk )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bbm-cp0d.65",   0x00001, 0x20000, CRC(860c0479) SHA1(7556d62955d0d7a7100fbd9d9cb7356b96a4df78) )
	ROM_LOAD16_BYTE( "bbm-cp1d.62",   0x00000, 0x20000, CRC(be57bf74) SHA1(cd3f887f7ec8a5721551477ec2d4a7336f422c6f) )
	ROM_COPY( REGION_CPU1, 0x3fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm-sp.23",    0x0000, 0x10000, CRC(251090cd) SHA1(9245072c1afbfa3e4a1d1549942765d58bd78ed3) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bbm-c0.66",    0x000000, 0x40000, CRC(695d2019) SHA1(3537e9fb0e7dc13d6113b4af71cba3c73392335a) ) /* Labeled as 9134HD004 */
	ROM_LOAD( "bbm-c1.67",    0x040000, 0x40000, CRC(4c7c8bbc) SHA1(31ab5557d96c4184a9c02ed1c309f3070d148e25) ) /* Labeled as 9134HD001 */
	ROM_LOAD( "bbm-c2.68",    0x080000, 0x40000, CRC(0700d406) SHA1(0d43a31a726b0de0004beef41307de2508106b69) ) /* Labeled as 9134HD002 */
	ROM_LOAD( "bbm-c3.69",    0x0c0000, 0x40000, CRC(3c3613af) SHA1(f9554a73e95102333e449f6e81f2bb817ec00881) ) /* Labeled as 9134HD003 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "bbm-v0.20",    0x0000, 0x20000, CRC(0fa803fe) SHA1(d2ac1e624de38bed385442ceae09a76f203fa084) ) /* Labeled as 9132E9001 */
ROM_END

ROM_START( dynablsb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "db2-26.bin",   0x00001, 0x20000, CRC(a78c72f8) SHA1(e3ed1bce0278bada6357b5d0823511fa0241f3cd) )
	ROM_LOAD16_BYTE( "db3-25.bin",   0x00000, 0x20000, CRC(bf3137c3) SHA1(64bbca4b3a509b552ee8a19b3b50fe6638fd90e2) )
	ROM_COPY( REGION_CPU1, 0x3fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "db1-17.bin",    0x0000, 0x10000, CRC(e693c32f) SHA1(b6f228d26318718eedae765de9479706a3e4c38d) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bbm-c0.66",    0x000000, 0x40000, CRC(695d2019) SHA1(3537e9fb0e7dc13d6113b4af71cba3c73392335a) )
	ROM_LOAD( "bbm-c1.67",    0x040000, 0x40000, CRC(4c7c8bbc) SHA1(31ab5557d96c4184a9c02ed1c309f3070d148e25) )
	ROM_LOAD( "bbm-c2.68",    0x080000, 0x40000, CRC(0700d406) SHA1(0d43a31a726b0de0004beef41307de2508106b69) )
	ROM_LOAD( "bbm-c3.69",    0x0c0000, 0x40000, CRC(3c3613af) SHA1(f9554a73e95102333e449f6e81f2bb817ec00881) )

	ROM_REGION( 0x20000, REGION_SOUND1, ROMREGION_ERASE00 )	/* samples */
	/* Does this have a sample rom? */
ROM_END

ROM_START( bbmanw )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bbm2-h0-b.77",  0x00001, 0x40000, CRC(567d3709) SHA1(1447fc68798589a8757ee2d133d053b80f052113) )
	ROM_LOAD16_BYTE( "bbm2-l0-b.79",  0x00000, 0x40000, CRC(e762c22b) SHA1(b389a65adf1348e6529a992d9b68178d7503238e) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm2-sp.33",    0x0000, 0x10000, CRC(6bc1689e) SHA1(099c275632965e19eb6131863f69d2afa9916e90) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bbm2-c0.81",  0x000000, 0x40000, CRC(e7ce058a) SHA1(f2336718ecbce4771f27abcdc4d28fe91c702a9e) )
	ROM_LOAD( "bbm2-c1.82",  0x080000, 0x40000, CRC(636a78a9) SHA1(98562ea056e5bd36c1a094ae6f267367236d166f) )
	ROM_LOAD( "bbm2-c2.83",  0x100000, 0x40000, CRC(9ac2142f) SHA1(744fe1acae2fcba0051c303b644081546b4aed9e) )
	ROM_LOAD( "bbm2-c3.84",  0x180000, 0x40000, CRC(47af1750) SHA1(dce176a6ca95852208b6eba7fb88a0d96467c34b) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "bbm2-v0.30",    0x0000, 0x20000, CRC(4ad889ed) SHA1(b685892a2348f17f89c6d6ce91216f6cf1e33751) )
ROM_END

ROM_START( bbmanwj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bbm2-h0.77",  0x00001, 0x40000, CRC(e1407b91) SHA1(6c94afc6b1d2a469295890ee5dd9d9d5a02ae5c4) )
	ROM_LOAD16_BYTE( "bbm2-l0.79",  0x00000, 0x40000, CRC(20873b49) SHA1(30ae595f7961cd56f2506608ae76973b2d0e73ca) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm2-sp-b.bin", 0x0000, 0x10000, CRC(b8d8108c) SHA1(ef4fb46d843819c273db2083754eb312f5abd44e) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bbm2-c0.81",  0x000000, 0x40000, CRC(e7ce058a) SHA1(f2336718ecbce4771f27abcdc4d28fe91c702a9e) )
	ROM_LOAD( "bbm2-c1.82",  0x080000, 0x40000, CRC(636a78a9) SHA1(98562ea056e5bd36c1a094ae6f267367236d166f) )
	ROM_LOAD( "bbm2-c2.83",  0x100000, 0x40000, CRC(9ac2142f) SHA1(744fe1acae2fcba0051c303b644081546b4aed9e) )
	ROM_LOAD( "bbm2-c3.84",  0x180000, 0x40000, CRC(47af1750) SHA1(dce176a6ca95852208b6eba7fb88a0d96467c34b) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "bbm2-v0-b.30",  0x0000, 0x20000, CRC(0ae655ff) SHA1(78752182662fd8f5b55bbbc2787c9f2b04096ea1) )
ROM_END

ROM_START( newapunk )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bbm2-h0-a.77",  0x00001, 0x40000, CRC(7d858682) SHA1(03580e2903becb69766023585c6ecffbb8e0b9c5) )
	ROM_LOAD16_BYTE( "bbm2-l0-a.79",  0x00000, 0x40000, CRC(c7568031) SHA1(ff4d0809260a088f530098a0173eec16fa6396f1) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm2-sp.33",    0x0000, 0x10000, CRC(6bc1689e) SHA1(099c275632965e19eb6131863f69d2afa9916e90) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bbm2-c0.81",  0x000000, 0x40000, CRC(e7ce058a) SHA1(f2336718ecbce4771f27abcdc4d28fe91c702a9e) )
	ROM_LOAD( "bbm2-c1.82",  0x080000, 0x40000, CRC(636a78a9) SHA1(98562ea056e5bd36c1a094ae6f267367236d166f) )
	ROM_LOAD( "bbm2-c2.83",  0x100000, 0x40000, CRC(9ac2142f) SHA1(744fe1acae2fcba0051c303b644081546b4aed9e) )
	ROM_LOAD( "bbm2-c3.84",  0x180000, 0x40000, CRC(47af1750) SHA1(dce176a6ca95852208b6eba7fb88a0d96467c34b) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "bbm2-v0.30",    0x0000, 0x20000, CRC(4ad889ed) SHA1(b685892a2348f17f89c6d6ce91216f6cf1e33751) )
ROM_END

ROM_START( bomblord )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bomblord.3",  0x00001, 0x40000, CRC(65d5c54a) SHA1(f794a193d5927b5fb838ab2351c176d8cbd37236) )
	ROM_LOAD16_BYTE( "bomblord.4",  0x00000, 0x40000, CRC(cfe65f81) SHA1(8dae94abc67bc53f1c8dbe13243dc08a62fd5d22) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bbm2-sp.33",    0x0000, 0x10000, CRC(6bc1689e) SHA1(099c275632965e19eb6131863f69d2afa9916e90) ) // bomblord.1

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bomblord.5",  0x000000, 0x40000, CRC(3ded3278) SHA1(2fec2f10d875e44d966b6f652e3b09308db9b343) )
	ROM_LOAD( "bomblord.6",  0x080000, 0x40000, CRC(1c489632) SHA1(f4412b138e4933c8d152ec51d05baa02abc7fc00) )
	ROM_LOAD( "bomblord.7",  0x100000, 0x40000, CRC(68935e94) SHA1(6725c7ad49bd0ee6ed1db22193852a11cdf95aaa) )
	ROM_LOAD( "bomblord.8",  0x180000, 0x40000, CRC(6a423b24) SHA1(d30dac90a7dc2a616714eae7450ae0edef566c31) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "bomblord.2",    0x0000, 0x20000, CRC(37d356bd) SHA1(15f187954f94e2b1a4757e4a27ab7be9598972ff) )
ROM_END

ROM_START( quizf1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "qf1-h0-.77",   0x000001, 0x40000, CRC(280e3049) SHA1(3b1f303d803f844fd260ed93e4d12a72876e4dbe) )
	ROM_LOAD16_BYTE( "qf1-l0-.79",   0x000000, 0x40000, CRC(94588a6f) SHA1(ee912739c7719fc2b099da0c63f7473eedcfc718) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "qf1-h1-.78",   0x000001, 0x80000, CRC(c6c2eb2b) SHA1(83de08b0c72da8c3e4786063802d83cb1015032a) )	/* banked at 80000-8FFFF */
	ROM_LOAD16_BYTE( "qf1-l1-.80",   0x000000, 0x80000, CRC(3132c144) SHA1(de3ae35cdfbb1231cab343142ac700df00f9b77a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "qf1-sp-.33",   0x0000, 0x10000, CRC(0664fa9f) SHA1(db003beb4f8461bf4411efa8df9f700770fb153b) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "qf1-c0-.81",   0x000000, 0x80000, CRC(c26b521e) SHA1(eb5d33a21d1f82e361e0c0945abcf42562c32f03) )
	ROM_LOAD( "qf1-c1-.82",   0x080000, 0x80000, CRC(db9d7394) SHA1(06b41288c41df8ae0cafb53e77b519d0419cf1d9) )
	ROM_LOAD( "qf1-c2-.83",   0x100000, 0x80000, CRC(0b1460ae) SHA1(c6394e6bb2a4e3722c20d9f291cb6ba7aad5766d) )
	ROM_LOAD( "qf1-c3-.84",   0x180000, 0x80000, CRC(2d32ff37) SHA1(f414f6bad1ffc4396fd757155e602bdefdc99408) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "qf1-v0-.30",   0x0000, 0x40000, CRC(b8d16e7c) SHA1(28a20afb171dc68848f9fe793f53571d4c7502dd) )
ROM_END

ROM_START( riskchal )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rc_h0.rom",    0x00001, 0x40000, CRC(4c9b5344) SHA1(61e26950a672c6404e2386acdd098536b61b9933) )
	ROM_LOAD16_BYTE( "rc_l0.rom",    0x00000, 0x40000, CRC(0455895a) SHA1(1072b8d280f7ccc48cd8fbd81323e1f8c8d0db95) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "rc_sp.rom",    0x0000, 0x10000, CRC(bb80094e) SHA1(1c62e702c395b7ebb666a79af1912b270d5f95aa) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rc_c0.rom",    0x000000, 0x80000, CRC(84d0b907) SHA1(a686ccd67d068e5e4ba41bb8b73fdc1cad8eb5ee) )
	ROM_LOAD( "rc_c1.rom",    0x080000, 0x80000, CRC(cb3784ef) SHA1(51b8cdc35c8f3b452939ab6023a15f1c7e1a4423) )
	ROM_LOAD( "rc_c2.rom",    0x100000, 0x80000, CRC(687164d7) SHA1(0f0beb0a85ae5ae4434d1e45a27bbe67f5ee378a) )
	ROM_LOAD( "rc_c3.rom",    0x180000, 0x80000, CRC(c86be6af) SHA1(c8a66b8b38a62e3eebb4a0e65a85e20f91182097) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "rc_v0.rom",    0x0000, 0x40000, CRC(cddac360) SHA1(a3b18325991473c6d54b778a02bed86180aad37c) )
ROM_END

ROM_START( gussun )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "l4_h0.rom",    0x00001, 0x40000, CRC(9d585e61) SHA1(e108a9dc2dc1b75c1439271a2391f943c3a53fe1) )
	ROM_LOAD16_BYTE( "l4_l0.rom",    0x00000, 0x40000, CRC(c7b4c519) SHA1(44887ccf54f5e507d2db4f09a7c2b7b9ea217058) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "rc_sp.rom",    0x0000, 0x10000, CRC(bb80094e) SHA1(1c62e702c395b7ebb666a79af1912b270d5f95aa) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rc_c0.rom",    0x000000, 0x80000, CRC(84d0b907) SHA1(a686ccd67d068e5e4ba41bb8b73fdc1cad8eb5ee) )
	ROM_LOAD( "rc_c1.rom",    0x080000, 0x80000, CRC(cb3784ef) SHA1(51b8cdc35c8f3b452939ab6023a15f1c7e1a4423) )
	ROM_LOAD( "rc_c2.rom",    0x100000, 0x80000, CRC(687164d7) SHA1(0f0beb0a85ae5ae4434d1e45a27bbe67f5ee378a) )
	ROM_LOAD( "rc_c3.rom",    0x180000, 0x80000, CRC(c86be6af) SHA1(c8a66b8b38a62e3eebb4a0e65a85e20f91182097) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "rc_v0.rom",    0x0000, 0x40000, CRC(cddac360) SHA1(a3b18325991473c6d54b778a02bed86180aad37c) )
ROM_END

ROM_START( matchit2 )
	ROM_REGION( CODE_SIZE * 2, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sis2-h0b.bin", 0x00001, 0x40000, CRC(9a2556ac) SHA1(3e4d5ac2869c703c5d5b769c2a09e501b5e6462e) ) /* Actually labeled as "SIS2-H0-B" */
	ROM_LOAD16_BYTE( "sis2-l0b.bin", 0x00000, 0x40000, CRC(d35d948a) SHA1(e4f119fa00fd8ede2533323e14d94ad4d5fabbc5) ) /* Actually labeled as "SIS2-L0-B" */
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "sis2-sp-.rom", 0x0000, 0x10000, CRC(6fc0ff3a) SHA1(2b8c648c1fb5d516552fc260b8f18ffd56bbe062) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic81.rom",     0x000000, 0x80000, CRC(5a7cb88f) SHA1(ce3befcd956b803655b261c2ece911f444aa3a13) )
	ROM_LOAD( "ic82.rom",     0x080000, 0x80000, CRC(54a7852c) SHA1(887e7543f09d00323ce1986e72c5613dde1dc6cc) )
	ROM_LOAD( "ic83.rom",     0x100000, 0x80000, CRC(2bd65dc6) SHA1(b50dec707ea5a71972df0a8dc47141d75e8f874e) )
	ROM_LOAD( "ic84.rom",     0x180000, 0x80000, CRC(876d5fdb) SHA1(723c58268be60f4973e914df238b264708d3f1e3) )

	ROM_REGION( 0x20000, REGION_SOUND1, ROMREGION_ERASE00 )	/* samples */
	/* Does this have a sample rom? */
ROM_END

ROM_START( shisen2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sis2-h0-.rom", 0x00001, 0x40000, CRC(6fae0aea) SHA1(7ebecbfdb17e15b8c0ebd293cd42a618c596782e) )
	ROM_LOAD16_BYTE( "sis2-l0-.rom", 0x00000, 0x40000, CRC(2af25182) SHA1(ec6dcc3913e1b7e7a3958b78610e83f51c404e07) )
	ROM_COPY( REGION_CPU1, 0x7fff0,  0xffff0, 0x10 )	/* start vector */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "sis2-sp-.rom", 0x0000, 0x10000, CRC(6fc0ff3a) SHA1(2b8c648c1fb5d516552fc260b8f18ffd56bbe062) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic81.rom",     0x000000, 0x80000, CRC(5a7cb88f) SHA1(ce3befcd956b803655b261c2ece911f444aa3a13) )
	ROM_LOAD( "ic82.rom",     0x080000, 0x80000, CRC(54a7852c) SHA1(887e7543f09d00323ce1986e72c5613dde1dc6cc) )
	ROM_LOAD( "ic83.rom",     0x100000, 0x80000, CRC(2bd65dc6) SHA1(b50dec707ea5a71972df0a8dc47141d75e8f874e) )
	ROM_LOAD( "ic84.rom",     0x180000, 0x80000, CRC(876d5fdb) SHA1(723c58268be60f4973e914df238b264708d3f1e3) )

	ROM_REGION( 0x20000, REGION_SOUND1, ROMREGION_ERASE00 )	/* samples */
	/* Does this have a sample rom? */
ROM_END



static DRIVER_INIT( hasamu )
{
	irem_cpu_decrypt(0,gunforce_decryption_table);
}

static DRIVER_INIT( bombrman )
{
	irem_cpu_decrypt(0,bomberman_decryption_table);
}

/* Bomberman World executes encrypted code from RAM! */
static UINT8 *bbmanw_ram_base;
extern UINT8 *irem_cpu_decrypted;

static WRITE8_HANDLER( bbmanw_ram_write )
{
	bbmanw_ram_base[offset]=data;
	irem_cpu_decrypted[0xa0c00+offset]=dynablaster_decryption_table[data];
}

static READ8_HANDLER( bbmanw_ram_read )
{
	return bbmanw_ram_base[offset];
}

static DRIVER_INIT( bbmanw )
{
	irem_cpu_decrypt(0,dynablaster_decryption_table);

	bbmanw_ram_base = memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa0c00, 0xa0cff, 0, 0, bbmanw_ram_write);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa0c00, 0xa0cff, 0, 0, bbmanw_ram_read);
}

static DRIVER_INIT( quizf1 )
{
	irem_cpu_decrypt(0,lethalth_decryption_table);

	bankaddress = 0;
	set_m90_bank();

	state_save_register_global(bankaddress);
	state_save_register_func_postload(set_m90_bank);
}

static DRIVER_INIT( riskchal )
{
	irem_cpu_decrypt(0,gussun_decryption_table);
}

static DRIVER_INIT( matchit2 )
{
	irem_cpu_decrypt(0,matchit2_decryption_table);
}

static DRIVER_INIT( bomblord )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	int i;
	for (i=0; i<0x100000; i+=8)
	{
		RAM[i+0]=BITSWAP8(RAM[i+0], 6, 4, 7, 3, 1, 2, 0, 5);
		RAM[i+1]=BITSWAP8(RAM[i+1], 4, 0, 5, 6, 7, 3, 2, 1);
		RAM[i+2]=BITSWAP8(RAM[i+2], 0, 6, 1, 5, 3, 4, 2, 7);
		RAM[i+3]=BITSWAP8(RAM[i+3], 4, 3, 5, 2, 6, 1, 7, 0);
		RAM[i+4]=BITSWAP8(RAM[i+4], 4, 7, 3, 2, 5, 6, 1, 0);
		RAM[i+5]=BITSWAP8(RAM[i+5], 5, 1, 4, 0, 6, 7, 2, 3);
		RAM[i+6]=BITSWAP8(RAM[i+6], 6, 3, 7, 5, 0, 1, 4, 2);
		RAM[i+7]=BITSWAP8(RAM[i+7], 6, 5, 7, 0, 3, 2, 1, 4);
	}
}



GAME( 1991, hasamu,   0,        m90,      hasamu,   hasamu,   ROT0, "Irem", "Hasamu (Japan)", GAME_NO_COCKTAIL )
GAME( 1991, dynablst, 0,        bombrman, dynablst, bombrman, ROT0, "Irem (licensed from Hudson Soft)", "Dynablaster / Bomber Man", GAME_NO_COCKTAIL )
GAME( 1991, bombrman, dynablst, bombrman, bombrman, bombrman, ROT0, "Irem (licensed from Hudson Soft)", "Bomber Man (Japan)", GAME_NO_COCKTAIL )
GAME( 1991, atompunk, dynablst, bombrman, atompunk, bombrman, ROT0, "Irem America (licensed from Hudson Soft)", "Atomic Punk (US)", GAME_NO_COCKTAIL )
GAME( 1991, dynablsb, dynablst, bootleg,  bombrman, 0,        ROT0, "bootleg", "Dynablaster (bootleg)", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAME( 1992, bbmanw,   0,        bbmanw,   bbmanw,   bbmanw,   ROT0, "Irem", "Bomber Man World / New Dyna Blaster - Global Quest", GAME_NO_COCKTAIL )
GAME( 1992, bbmanwj,  bbmanw,   bombrman, bbmanwj,  bbmanw,   ROT0, "Irem", "Bomber Man World (Japan)", GAME_NO_COCKTAIL )
GAME( 1992, newapunk, bbmanw,   bbmanw,   bbmanwj,  bbmanw,   ROT0, "Irem America", "New Atomic Punk - Global Quest (US)", GAME_NO_COCKTAIL )
GAME( 1992, bomblord, bbmanw,   bomblord, bbmanw,   bomblord, ROT0, "bootleg", "Bomber Lord (bootleg)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL | GAME_NOT_WORKING )
GAME( 1992, quizf1,   0,        quizf1,   quizf1,   quizf1,   ROT0, "Irem", "Quiz F-1 1,2finish", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
GAME( 1993, riskchal, 0,        m90,      riskchal, riskchal, ROT0, "Irem", "Risky Challenge", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAME( 1993, gussun,   riskchal, m90,      riskchal, riskchal, ROT0, "Irem", "Gussun Oyoyo (Japan)", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAME( 1993, matchit2, 0,        quizf1,   matchit2, matchit2, ROT0, "Tamtex", "Match It II", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAME( 1993, shisen2,  matchit2, quizf1,   shisen2,  matchit2, ROT0, "Tamtex", "Shisensho II", GAME_NOT_WORKING | GAME_NO_COCKTAIL )

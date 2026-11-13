// license:BSD-3-Clause
// copyright-holders:Brad Oliver,Stephane Humbert
/***************************************************************************

    Arkanoid driver (Preliminary)


    Japanese version support cocktail mode (DSW #7), the others don't.

    Here are the versions we have:

    arkanoid    World version, probably an earlier revision
    arknoidu    USA version, probably a later revision; There has been code
                inserted, NOT patched, so I don't think it's a bootleg
                The 68705 code for this one was not available; I made it up from
                the World version changing the level data pointer table.
    arknoiuo    USA version, probably an earlier revision
                ROM a75-10.bin should be identical to the real World one.
                (It only differs in the country byte from A75-11.ROM)
                This version works fine with the real MCU ROM
    arkatour    Tournament version
                The 68705 code for this one was not available; I made it up from
                the World version changing the level data pointer table.
    arknoidj    Japanese version with level selector.
                The 68705 code for this one was not available; I made it up from
                the World version changing the level data pointer table.
    arkbl2      Bootleg of the early Japanese version.
                The only difference is that the warning text has been replaced
                by "WAIT"
                ROM E2.6F should be identical to the real Japanese one.
                (It only differs in the country byte from A75-11.ROM)
                This version works fine with the real MCU ROM
    arkatayt    Another bootleg of the early Japanese one, more heavily modified
    arkblock    Another bootleg of the early Japanese one, more heavily modified
    arkbloc2    Another bootleg
    arkbl3      Another bootleg of the early Japanese one, more heavily modified
    paddle2     Another bootleg of the early Japanese one, more heavily modified
    arkangc     Game Corporation bootleg with level selector


    Most if not all Arkanoid sets have a bug in their game code. It occurs on the
    final level where the player has to dodge falling objects. The bug resides in
    the collision detection routine which sometimes reads from unmapped addresses
    above $F000. For these addresses it is vital to read zero values, or else the
    player will die for no reason.


***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"

extern WRITE8_HANDLER( arkanoid_videoram_w );
extern VIDEO_START( arkanoid );
extern VIDEO_UPDATE( arkanoid );

extern MACHINE_START( arkanoid );
extern MACHINE_RESET( arkanoid );

extern WRITE8_HANDLER( arkanoid_d008_w );
extern VIDEO_UPDATE( brixian );

static UINT8 *brixian_ram;

static WRITE8_HANDLER( brixian_d008_w )
{
	arkanoid_d008_w(offset, data | 0x08);
}

static WRITE8_HANDLER( tetrsark_d008_w )
{
	arkanoid_d008_w(offset, (data & ~0x08) | ((data & 0x80) >> 4));
}

extern READ8_HANDLER( arkanoid_Z80_mcu_r );
extern WRITE8_HANDLER( arkanoid_Z80_mcu_w );

extern READ8_HANDLER( arkanoid_68705_portA_r );
extern WRITE8_HANDLER( arkanoid_68705_portA_w );
extern WRITE8_HANDLER( arkanoid_68705_ddrA_w );

extern READ8_HANDLER( arkanoid_68705_portC_r );
extern WRITE8_HANDLER( arkanoid_68705_portC_w );
extern WRITE8_HANDLER( arkanoid_68705_ddrC_w );

extern READ8_HANDLER( arkanoid_68705_input_0_r );
extern READ8_HANDLER( arkanoid_input_2_r );

extern READ8_HANDLER( paddle2_prot_r );
extern WRITE8_HANDLER( paddle2_prot_w );
extern READ8_HANDLER( paddle2_track_kludge_r );

/* Memory Maps */

static ADDRESS_MAP_START( arkanoid_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xd008, 0xd008) AM_WRITE(arkanoid_d008_w)	/* gfx bank, flip screen etc. */
	AM_RANGE(0xd00c, 0xd00c) AM_READ(arkanoid_68705_input_0_r)  /* mainly an input port, with 2 bits from the 68705 */
	AM_RANGE(0xd010, 0xd010) AM_READWRITE(input_port_1_r, watchdog_reset_w)
	AM_RANGE(0xd018, 0xd018) AM_READWRITE(arkanoid_Z80_mcu_r, arkanoid_Z80_mcu_w)  /* input from the 68705 */
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_WRITE(arkanoid_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe800, 0xe83f) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe840, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_READNOP	/* fixes instant death in final level */
ADDRESS_MAP_END

static ADDRESS_MAP_START( bootleg_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xd008, 0xd008) AM_WRITE(arkanoid_d008_w)	/* gfx bank, flip screen etc. */
	AM_RANGE(0xd00c, 0xd00c) AM_READ(input_port_0_r)
	AM_RANGE(0xd010, 0xd010) AM_READWRITE(input_port_1_r, watchdog_reset_w)
	AM_RANGE(0xd018, 0xd018) AM_READ(arkanoid_input_2_r) AM_WRITENOP
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_WRITE(arkanoid_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe800, 0xe83f) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe840, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_READNOP	/* fixes instant death in final level */
ADDRESS_MAP_END

static ADDRESS_MAP_START( brixian_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM AM_BASE(&brixian_ram)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xd008, 0xd008) AM_WRITE(brixian_d008_w)
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_WRITE(arkanoid_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe800, 0xe83f) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe840, 0xefff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READWRITE(arkanoid_68705_portA_r, arkanoid_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_READ(arkanoid_input_2_r)
	AM_RANGE(0x0002, 0x0002) AM_READWRITE(arkanoid_68705_portC_r, arkanoid_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(arkanoid_68705_ddrA_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(arkanoid_68705_ddrC_w)
	AM_RANGE(0x0010, 0x007f) AM_RAM
	AM_RANGE(0x0080, 0x07ff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */
#define ARKNOI_IN0\
	PORT_START_TAG("IN0")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* input from the 68705, some bootlegs need it to be 1 */\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* input from the 68705 */

#define ARKNOI_IN1\
	PORT_START_TAG("IN1")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL\
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define ARKNOI_SPINNERS\
	PORT_START_TAG("IN2")      /* Spinner Player 1 */\
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15)\
	PORT_START_TAG("IN3")      /* Spinner Player 2  */\
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_COCKTAIL

INPUT_PORTS_START( arkanoid )
	ARKNOI_IN0
	ARKNOI_IN1
	ARKNOI_SPINNERS

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20K 60K 60K+" )
	PORT_DIPSETTING(    0x00, "20K" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
INPUT_PORTS_END

/* These are the input ports of the real Japanese ROM set                        */
/* 'Block' uses the these ones as well. The Tayto bootleg is different           */
/*  in coinage and # of lives.                                                   */

INPUT_PORTS_START( arknoidj )
	ARKNOI_IN0
	ARKNOI_IN1
	ARKNOI_SPINNERS

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20K 60K 60K+" )
	PORT_DIPSETTING(    0x00, "20K" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

/* Is the same as arkanoij, but the Coinage,
  Lives and Bonus_Life dips are different */
INPUT_PORTS_START( arkatayt )
	ARKNOI_IN0
	ARKNOI_IN1
	ARKNOI_SPINNERS

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "60K 100K 60K+" )
	PORT_DIPSETTING(    0x00, "60K" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END
INPUT_PORTS_START( cruisin5 )
	ARKNOI_IN0
	ARKNOI_IN1
	ARKNOI_SPINNERS

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) ) /* must be on? */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Loop" )
	PORT_DIPSETTING(    0x04, "Auto" )
	PORT_DIPSETTING(    0x00, "Manual" )
	PORT_DIPNAME( 0x08, 0x08, "??? Animation" )
	PORT_DIPSETTING(    0x08, "Squeeze Out" )
	PORT_DIPSETTING(    0x00, "Tear Apart" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Sound Hardware" )
	PORT_DIPSETTING(    0x40, "AY-3-8910" )
	PORT_DIPSETTING(    0x00, "YM2149" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( tetrsark )
	PORT_START_TAG("IN0")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START_TAG("DSW")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )

	PORT_START_TAG("INPUTS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )
INPUT_PORTS_END


INPUT_PORTS_START( brixian )
	PORT_START_TAG("INPUTS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x02, "Time Left" )
	PORT_DIPSETTING(    0x00, "More" )
	PORT_DIPSETTING(    0x01, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, "Normal (dupe)" )
	PORT_DIPSETTING(    0x03, "Less" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown )  )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Speed of Elevator" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPSETTING(    0x08, "Slow" )
	PORT_DIPNAME( 0xf0, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xf0, "2" )
	PORT_DIPSETTING(    0x70, "3" )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
INPUT_PORTS_END


/* Graphics Layouts */

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	3,	/* 3 bits per pixel */
	{ 2*4096*8*8, 4096*8*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 64 },
	// sprites use the same characters above, but are 16x8
	{ -1 }
};

/* Sound Interfaces */

static struct AY8910interface ay8910_interface =
{
	input_port_5_r,
	input_port_4_r
};

/* Machine Drivers */

static MACHINE_DRIVER_START( arkanoid )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", Z80, 6000000)	// 6 MHz ???
	MDRV_CPU_PROGRAM_MAP(arkanoid_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD_TAG("mcu", M68705, 12000000/4)
	MDRV_CPU_PROGRAM_MAP(mcu_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)					// 100 CPU slices per second to synchronize between the MCU and the main CPU

	MDRV_MACHINE_START(arkanoid)
	MDRV_MACHINE_RESET(arkanoid)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(arkanoid)
	MDRV_VIDEO_UPDATE(arkanoid)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("aysnd", AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bootleg )
	MDRV_IMPORT_FROM(arkanoid)

	// basic machine hardware
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(bootleg_map, 0)

	MDRV_CPU_REMOVE("mcu")
MACHINE_DRIVER_END

static struct AY8910interface brixian_ay8910_interface =
{
	input_port_0_r,
	input_port_1_r
};

/* Cruisin 5 multiplexes the sprites during the frame, so the screen is drawn
   scanline by scanline and the interrupt is raised at the end of the vblank */
static mame_timer *cruisin5_timer;

static void cruisin5_scanline(int scanline)
{
	/* the beam is leaving the vblank */
	if (scanline == Machine->visible_area.min_y)
		cpunum_set_input_line(0, 0, HOLD_LINE);

	/* draw what the game has put on screen so far, before it changes the sprites again */
	force_partial_update(scanline);

	scanline = (scanline + 1) & 255;
	mame_timer_adjust(cruisin5_timer, cpu_getscanlinetime_mt(scanline), scanline, time_zero);
}

static MACHINE_START( cruisin5 )
{
	cruisin5_timer = mame_timer_alloc(cruisin5_scanline);
	return machine_start_arkanoid();
}

static MACHINE_RESET( cruisin5 )
{
	machine_reset_arkanoid();
	mame_timer_adjust(cruisin5_timer, cpu_getscanlinetime_mt(0), 0, time_zero);
}

static MACHINE_DRIVER_START( cruisin5 )
	MDRV_IMPORT_FROM(bootleg)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(NULL, 0)

	MDRV_MACHINE_START(cruisin5)
	MDRV_MACHINE_RESET(cruisin5)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( brixian )
	MDRV_IMPORT_FROM(bootleg)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(brixian_map, 0)
	MDRV_VIDEO_UPDATE(brixian)
	MDRV_SOUND_REPLACE("aysnd", AY8910, 1500000)
	MDRV_SOUND_CONFIG(brixian_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( arkanoid )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75__01-1.ic17", 0x0000, 0x8000, CRC(5bcda3b0) SHA1(52cadd38b5f8e8856f007a9c602d6b508f30be65) )
	ROM_LOAD( "a75__11.ic16",   0x8000, 0x8000, CRC(eafd7191) SHA1(d2f8843b716718b1de209e97a874e8ce600f3f87) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a75__06.ic14", 0x0000, 0x0800, CRC(0be83647) SHA1(625fd1e6061123df612f115ef14a06cd6009f5d1) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75__03.ic64",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75__04.ic63",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75__05.ic62",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.ic24",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.ic23",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.ic22",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */

	ROM_REGION( 0x1800, REGION_USER2, ROMREGION_DISPOSE )	/* alternate MCUs, all work in place of a75__06 */
	ROM_LOAD( "arkanoid_mcu.ic14", 0x0000, 0x0800, CRC(4e44b50a) SHA1(c61e7d158dc8e2b003c8158053ec139b904599af) )
	ROM_LOAD( "a75-06__bootleg_68705.ic14", 0x0800, 0x0800, CRC(515d77b6) SHA1(a302937683d11f663abd56a2fd7c174374e4d7fb) )
	ROM_LOAD( "arkanoid1_68705p3.ic14", 0x1000, 0x0800, CRC(1b68e2d8) SHA1(f642a7cb624ee14fb0e410de5ae1fc799d2fa1c2) )
ROM_END

ROM_START( arkanoidu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75__19.ic17",   0x0000, 0x8000, CRC(d3ad37d7) SHA1(a172a1ef5bb83ee2d8ed2842ef8968af19ad411e) )
	ROM_LOAD( "a75__18.ic16",   0x8000, 0x8000, CRC(cdc08301) SHA1(05f54353cc8333af14fa985a2764960e20e8161a) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a75__20.ic14", 0x0000, 0x0800, CRC(3994ee92) SHA1(31f6577956f49ba0b0705b490ce3254033795552) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75__03.ic64",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75__04.ic63",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75__05.ic62",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.ic24",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.ic23",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.ic22",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkanoiduo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75__01-1.ic17", 0x0000, 0x8000, CRC(5bcda3b0) SHA1(52cadd38b5f8e8856f007a9c602d6b508f30be65) )
	ROM_LOAD( "a75__10.ic16",   0x8000, 0x8000, CRC(a1769e15) SHA1(fbb45731246a098b29eb08de5d63074b496aaaba) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a75__06.ic14", 0x0000, 0x0800, CRC(0be83647) SHA1(625fd1e6061123df612f115ef14a06cd6009f5d1) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75__03.ic64",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75__04.ic63",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75__05.ic62",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.ic24",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.ic23",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.ic22",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */

	ROM_REGION( 0x8000, REGION_USER2, ROMREGION_DISPOSE )	/* alternate GFX, the first 32 tiles are 0xFF instead of 0x00 */
	ROM_LOAD( "a75__03_alternate.ic64", 0x00000, 0x8000, CRC(983d4485) SHA1(603a8798d1f531a70a527a5c6122f0ffd6adcfb6) )
ROM_END

ROM_START( arkatour )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75__27.ic17",   0x0000, 0x8000, CRC(e3b8faf5) SHA1(4c09478fa41881fa89ee6afb676aeb780f17ac2e) )
	ROM_LOAD( "a75__28.ic16",   0x8000, 0x8000, CRC(326aca4d) SHA1(5a194b7a0361236d471b24905dc6434372f81252) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a75__32.ic14", 0x0000, 0x0800, CRC(8c20d15c) SHA1(912996bf08de318e19dc420261f554a09dacd443) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75__29.ic64",   0x00000, 0x8000, CRC(5ddea3cf) SHA1(58f16515898b7cc2697bf7663a60d9ca0db6da95) )
	ROM_LOAD( "a75__30.ic63",   0x08000, 0x8000, CRC(5fcf2e85) SHA1(f721f0afb0550cc64bff26681856a7576398d9b5) )
	ROM_LOAD( "a75__31.ic62",   0x10000, 0x8000, CRC(7b76b192) SHA1(a68aa08717646a6c322cf3455df07f50df9e9f33) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-33.ic24", 0x0000, 0x0200, CRC(b4bf3c81) SHA1(519188937ac9728c653fabac877e37dc43c3f71a) )
	ROM_LOAD( "a75-34.ic23", 0x0200, 0x0200, CRC(de85a803) SHA1(325214995996de36a0470fbfc00e4e393c0b17ad) )
	ROM_LOAD( "a75-35.ic22", 0x0400, 0x0200, CRC(38acfd3b) SHA1(2841e9db047aa039eff8567a518b6250b355507b) )
ROM_END

ROM_START( arkanoidja )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75__21.ic17",   0x0000, 0x8000, CRC(bf0455fc) SHA1(250522b84b9f491c3f4efc391bf6aa6124361369) )
	ROM_LOAD( "a75__22.ic16",   0x8000, 0x8000, CRC(3a2688d3) SHA1(9633a661352def3d85f95ca830f6d761b0b5450e) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a75__23.ic14", 0x0000, 0x0800, CRC(35938431) SHA1(04c6a888d967af0ceba06ab3cdd2670fe3bca396) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75__03.ic64",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75__04.ic63",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75__05.ic62",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.ic24",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.ic23",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.ic22",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkanoidjbl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "e1.6d",        0x0000, 0x8000, CRC(dd4f2b72) SHA1(399a8636030a702dafc1da926f115df6f045bef1) )
	ROM_LOAD( "e2.6f",        0x8000, 0x8000, CRC(bbc33ceb) SHA1(e9b6fef98d0d20e77c7a1c25eff8e9a8c668a258) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "68705p3.6i",   0x0000, 0x0800, CRC(389a8cfb) SHA1(9530c051b61b5bdec7018c6fdc1ea91288a406bd) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkgcbl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "electric__16.6e",   0x0000, 0x8000, CRC(b0f73900) SHA1(2c9a36cc1d2a3f33ec81d63c1c325554b818d2d3) )
	ROM_LOAD( "electric__17.6f",   0x8000, 0x8000, CRC(9827f297) SHA1(697874e73e045eb5a7bf333d7310934b239c0adf) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "electric__18.3a",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "electric__19.3c",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "electric__20.3d",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.5k", 0x0000, 0x0100, CRC(fa70b64d) SHA1(273669d05f793cf1ee0741b175be281307fa9b5e) )
	ROM_LOAD( "82s129.5jk", 0x0100, 0x0100, CRC(cca69884) SHA1(fdcd66110c8eb901a401f8618821c7980946a511) )
	ROM_LOAD( "82s129.5l", 0x0200, 0x0100, CRC(3e4d2bf5) SHA1(c475887302dd137d6965769070b7d55f488c1b25) )
	ROM_LOAD( "82s129.5kl", 0x0300, 0x0100, CRC(085d625a) SHA1(26c96a1c1b7562fed84c31dd92fdf7829e96a9c7) )
	ROM_LOAD( "82s129.5mn", 0x0400, 0x0100, CRC(0fe0b108) SHA1(fcf27619208922345a1e42b3a219b4274f66968d) )
	ROM_LOAD( "63s141.5m", 0x0500, 0x0100, CRC(5553f675) SHA1(c50255af8d99664b92e0bb34a527fd42ebf7e759) )

	ROM_REGION( 0x0200, REGION_USER2, ROMREGION_DISPOSE )
	ROM_LOAD( "pal16r8.5f", 0x0000, 0x0104, CRC(36471917) SHA1(d0f295a94d480b44416e66be4b480b299aad5c3c) ) /* likely used for the d008/d018/f000/f002 protection */
ROM_END

ROM_START( paddle2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "paddle2.16",   0x0000, 0x8000, CRC(a286333c) SHA1(0b2c9cb0df236f327413d0c541453e1ba979ea38) )
	ROM_LOAD( "paddle2.17",   0x8000, 0x8000, CRC(04c2acb5) SHA1(7ce8ba31224f705b2b6ed0200404ef5f8f688001) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arktayt2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic81.3f",   0x0000, 0x8000, CRC(6e0a2b6f) SHA1(5227d7a944cb1e815f60ec87a67f7462870ff9fe) )
	ROM_LOAD( "ic82.5f",   0x8000, 0x8000, CRC(5a97dd56) SHA1(b71c7b5ced2b0eebbcc5996dd21a1bb1c2da4819) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-ic33.2c",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "2-ic34.3c",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "3-ic35.5c",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "ic73.11e",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "ic74.12e",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "ic75.13e",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkbloc2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "block01.bin",  0x0000, 0x8000, CRC(5be667e1) SHA1(fbc5c97d836c404a2e6c007c3836e36b52ae75a1) )
	ROM_LOAD( "block02.bin",  0x8000, 0x8000, CRC(4f883ef1) SHA1(cb090a57fc75f17a3e2ba637f0e3ec93c1d02cea) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkblock )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ark-6.bin",    0x0000, 0x8000, CRC(0be015de) SHA1(f4209085b59d2c96a62ac9657c7bf097da55362b) )
	ROM_LOAD( "arkgc.2",      0x8000, 0x8000, CRC(9f0d4754) SHA1(731c9224616a338084edd6944c754d68eabba7f2) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkangc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "arkgc.1",      0x0000, 0x8000, CRC(c54232e6) SHA1(beb759cee68009a06824b755d2aa26d7d436b5b0) )
	ROM_LOAD( "arkgc.2",      0x8000, 0x8000, CRC(9f0d4754) SHA1(731c9224616a338084edd6944c754d68eabba7f2) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75-03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75-04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75-05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

/* the other Dr. Korea game (Hexa, hexa.c) also appears to be derived from Arkanoid hardware */

ROM_START( tetrsark )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )		/* 64k for code + 32k for banked ROM */
	ROM_LOAD( "ic17.1",      0x00000, 0x8000, CRC(1a505eda) SHA1(92f171a12cf0c326d29c244514718df04b998426) )
	ROM_LOAD( "ic16.2",      0x08000, 0x8000, CRC(157bc4df) SHA1(b2c704148e7e3ca61ab51308ee0d66ea1088bff3) ) // it doens't care if this is here??

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic64.3",      0x00000, 0x8000, CRC(c3e9b290) SHA1(6e99520606c654e531dbeb9a598cfbb443c24dff) )
	ROM_LOAD( "ic63.4",      0x08000, 0x8000, CRC(de9a368f) SHA1(ffbb2479200648da3f3e7ab7cebcdb604f6dfb3d) )
	ROM_LOAD( "ic62.5",      0x10000, 0x8000, CRC(c8e80a00) SHA1(4bee4c36ee768ae68ebc64e639fdc43f61c74f92) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "a75-07.bpr",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "a75-08.bpr",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "a75-09.bpr",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */

	ROM_REGION( 0x0800, REGION_USER2, ROMREGION_DISPOSE )	/* unused, not programmed */
	ROM_LOAD( "14_mc68705p5_rom.bin", 0x000, 0x800, CRC(dfbc4239) SHA1(d97c44d90d09142fd00731c1e44646bcba0402ec) )
ROM_END

ROM_START( cruisin5 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic81-v.3f",   0x0000, 0x8000, CRC(41ed7668) SHA1(afd1600317d210b6da2c63a65f1614be1f77854b) )
	ROM_LOAD( "ic82-w.5f",   0x8000, 0x8000, CRC(77e310cc) SHA1(e2638bf6c565df9dab98d92c857d2212b150a2d1) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-ic33.2c",   0x00000, 0x8000, CRC(7fb9dafc) SHA1(74a312729f10ab4753204b41de59ff41b93e80cb) )
	ROM_LOAD( "2-ic34.3c",   0x08000, 0x8000, CRC(05229af9) SHA1(76c469506430bc23ee77bbcefbc357478437bba7) )
	ROM_LOAD( "3-ic35.5c",   0x10000, 0x8000, CRC(a4c1a25f) SHA1(29d49572c1e7c6c8ac428ad2a3e625f0f548f66c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "ic73.11e",    0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "ic74.12e",    0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "ic75.13e",    0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( brixian )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "b1.bin",    0x00000, 0x8000, CRC(3d167d09) SHA1(1d5bd098b655b8d2f956cfcb718213915bee3e41) )
	ROM_LOAD( "e7.bin",    0x08000, 0x2000, CRC(9e3707ab) SHA1(a04fb4824239f8ed1ef1de2f3c0f9d749320b2ba) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )
	ROM_LOAD( "68705p5", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x200, REGION_USER1, 0 )
	ROM_LOAD( "protdata.bin", 0x00000, 0x200, CRC(a4131c0b) SHA1(5ddbd39c26e1bc9ec5f216e399c09994a23d09a7) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b4.bin",     0x00000, 0x8000, CRC(34a7a693) SHA1(793fa6dd065a158bedcd0fdc494cc8fc793ae8be) )
	ROM_LOAD( "c4.bin",     0x08000, 0x8000, CRC(d422eda5) SHA1(4874b57ec8a8aa29937f5ccc2a734ffeb7834d8a) )
	ROM_LOAD( "e4.bin",     0x10000, 0x8000, CRC(9b2e79d6) SHA1(8a40e0ef2a792efc37ea50eec01cf3fb5a3e3215) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "n82s131n.6l", 0x0000, 0x0200, CRC(0fa51a5b) SHA1(8c5cb69fbff8a3ba90f945c35f72754f9cc8f18c) )
	ROM_LOAD( "n82s131n.6p", 0x0200, 0x0200, CRC(d833ad33) SHA1(a7c17c96a670916e7102afc94dc2f0cb0455f0ce) )
	ROM_LOAD( "n82s131n.6m", 0x0400, 0x0200, CRC(05297649) SHA1(35f99cf8dddd66e26e2110619eb46bd6ccff41df) )
ROM_END

/* Driver Initialization */

static DRIVER_INIT( brixian )
{
	memcpy(brixian_ram + 0x600, memory_region(REGION_USER1), 0x200);
}


static DRIVER_INIT( paddle2 )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf002, 0xf002, 0, 0, paddle2_prot_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd018, 0xd018, 0, 0, paddle2_prot_w );
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd008, 0xd008, 0, 0, paddle2_track_kludge_r );
}

static DRIVER_INIT( tetrsark )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	int x;

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd008, 0xd008, 0, 0, tetrsark_d008_w);

	for (x=0;x<0x8000;x++)
	{
		ROM[x]=ROM[x]^0x94;
	}
}

/* Game Drivers */

GAME( 1986, arkanoid, 0,        arkanoid, arkanoid, 0,       ROT90, "Taito Corporation Japan", "Arkanoid (World, older)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkanoidu, arkanoid, arkanoid, arkanoid, 0,       ROT90, "Taito America Corporation (Romstar license)", "Arkanoid (US, newer)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkanoiduo, arkanoid, arkanoid, arkanoid, 0,       ROT90, "Taito America Corporation (Romstar license)", "Arkanoid (US, older)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkanoidja, arkanoid, arkanoid, arknoidj, 0,       ROT90, "Taito Corporation", "Arkanoid (Japan, newer w/level select)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkanoidjbl,   arkanoid, arkanoid, arknoidj, 0,       ROT90, "bootleg", "Arkanoid (bootleg with MCU, set 1)", GAME_NOT_WORKING )
GAME( 1986, arkgcbl,   arkanoid, bootleg,  arknoidj, paddle2, ROT90, "bootleg", "Arkanoid (bootleg on Block hardware, set 1)", GAME_NOT_WORKING )
GAME( 1986, arkangc,  arkanoid, bootleg,  arknoidj, 0,       ROT90, "bootleg (Game Corporation)", "Arkanoid (Game Corporation bootleg, set 1)", GAME_SUPPORTS_SAVE )
GAME( 1986, arkbloc2, arkanoid, bootleg,  arknoidj, 0,       ROT90, "bootleg (Game Corporation)", "Block (Game Corporation bootleg, set 2)", GAME_NOT_WORKING )
GAME( 1986, arkblock, arkanoid, bootleg,  arknoidj, 0,       ROT90, "bootleg (Game Corporation)", "Block (Game Corporation bootleg, set 1)", GAME_SUPPORTS_SAVE )
GAME( 1988, paddle2,  arkanoid, bootleg,  arknoidj, paddle2, ROT90, "bootleg", "Paddle 2 (bootleg on Block hardware)", GAME_UNEMULATED_PROTECTION | GAME_SUPPORTS_SAVE )
GAME( 1986, arktayt2, arkanoid, bootleg,  arkatayt, 0,       ROT90, "bootleg (Tayto)", "Arkanoid (Tayto bootleg, harder)", GAME_SUPPORTS_SAVE )
GAME( 1987, arkatour, 0,        arkanoid, arkanoid, 0,       ROT90, "Taito America Corporation (Romstar license)", "Tournament Arkanoid (US, older)", GAME_SUPPORTS_SAVE )
GAME( 19??, tetrsark, 0,        bootleg,  tetrsark, tetrsark,ROT0,  "D.R. Korea", "Tetris (D.R. Korea, set 1, encrypted)", GAME_SUPPORTS_SAVE )
GAME( 2023, cruisin5, 0,        cruisin5,  cruisin5, 0,       ROT90, "Abyss", "Cruisin 5: Cruise Back", GAME_SUPPORTS_SAVE )
GAME( 1993, brixian,  0,        brixian,  brixian,  brixian, ROT0, "Cheil Computer System", "Brixian", GAME_SUPPORTS_SAVE )

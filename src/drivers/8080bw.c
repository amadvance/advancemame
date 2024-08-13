/****************************************************************************/
/*                                                                          */
/*  8080bw.c                                                                */
/*                                                                          */
/*  Michael Strutts, Nicola Salmoria, Tormod Tjaberg, Mirko Buffoni         */
/*  Lee Taylor, Valerio Verrando, Marco Cassili, Zsolt Vasvari and others   */
/*                                                                          */
/*                                                                          */
/*  Notes:                                                                  */
/*  -----                                                                   */
/*                                                                          */
/*  - "The Amazing Maze Game" on title screen, but manual, flyer,           */
/*    cabinet side art all call it just "Amazing Maze"                      */
/*                                                                          */
/*  - Desert Gun was originally named Road Runner. The name was changed     */
/*    when Midway merged with Bally who had a game by the same title        */
/*                                                                          */
/*  - Space Invaders Deluxe still says Space Invaders Part II,              */
/*    because according to KLOV, Midway was only allowed to make minor      */
/*    modifications of the Taito code.                                      */
/*                                                                          */
/*                                                                          */
/*  To Do:                                                                  */
/*  -----                                                                   */
/*                                                                          */
/*  - Space Encounters: 'trench' circuit                                    */
/*                                                                          */
/*  - Phantom II: verify clouds                                             */
/*                                                                          */
/*                                                                          */
/* Change Log:                                                              */
/* ----------                                                               */
/*                                                                          */
/* 24 Dec 1998 - added sitv [LT]                                            */
/*                                                                          */
/* 21 Nov 1999 - added spacewar3 [LT]                                       */
/*                                                                          */
/* 26 May 2001 - added galxwars                                             */
/*                     galxwar2                                             */
/*                     jspectr2                                             */
/*                     ozmawar2                                             */
/*                     spaceatt                                             */
/*                     sstrangr                                             */
/*                                                                          */
/* 26 May 2001 - changed galxwars input port so the new sets work           */
/*                                                                          */
/* 30 Jul 2001 - added sstrngr2                                             */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "driver.h"
#include "8080bw.h"
#include "includes/circus.h"
#include "cpu/i8039/i8039.h"


/* gmissile and m4 need the RAM mirror */

static ADDRESS_MAP_START( c8080bw_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_MIRROR(0x4000) AM_READWRITE(MRA8_RAM, c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x5fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( c8080bw_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(c8080bw_shift_data_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_0_3, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(c8080bw_shift_amount_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(c8080bw_shift_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_1_2, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_WRITE(c8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(c8080bw_shift_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_2_4, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(c8080bw_shift_amount_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(c8080bw_shift_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_4_3, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x03, 0x03) AM_WRITE(c8080bw_shift_data_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(c8080bw_shift_amount_w)
ADDRESS_MAP_END

static MACHINE_DRIVER_START( 8080bw )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",8080,2000000)        /* 2 MHz? */
	MDRV_CPU_PROGRAM_MAP(c8080bw_cpu_map,0)
	MDRV_CPU_IO_MAP(c8080bw_readport,writeport_2_4)
	MDRV_CPU_VBLANK_INT(c8080bw_interrupt,2)    /* two interrupts per frame */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(8080bw)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Space Invaders"                             */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( invaders )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* must be ACTIVE_HIGH Super Invaders */

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( invaders )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_MACHINE_RESET(invaders)

	/* video hardware */
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(invaders_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invaders_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Space Invaders TV Version (Taito)                   */
/*                                                     */
/*******************************************************/

/* same as Invaders with a test mode switch */

INPUT_PORTS_START( sitv )
	PORT_START_TAG("IN0")
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Midway "Space Invaders Part II"                     */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( invadpt2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )	/* otherwise high score entry ends right away */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, "High Score Preset Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/* same as regular invaders, but with a color board added */

static MACHINE_DRIVER_START( invadpt2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(invaders)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Cosmo                                               */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( cosmo )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN ) /* must be HIGH normally or the joystick won't work */

	PORT_START_TAG("FAKE")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END
static ADDRESS_MAP_START( cosmo_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x57ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x5c00, 0x5fff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmo_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x57ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x5c00, 0x5fff) AM_WRITE(cosmo_colorram_w) AM_BASE(&colorram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmo_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
ADDRESS_MAP_END

/* at least one of these MWA8_NOPs must be sound related */
static ADDRESS_MAP_START( cosmo_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x01, 0x01) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x02, 0x02) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x06, 0x06) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static MACHINE_DRIVER_START( cosmo )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(invaders)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(cosmo_readmem, cosmo_writemem)
	MDRV_CPU_IO_MAP(cosmo_readport, cosmo_writeport)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(cosmo)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* ?????? "Super Earth Invasion"                       */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( earthinv )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPNAME( 0x02, 0x02, "Pence Coinage" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) /* Pence Coin */
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* Not bonus */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "2C/1C 50p/3C (+ Bonus Life)" )
	PORT_DIPSETTING(    0x80, "1C/1C 50p/5C" )

	PORT_START_TAG("FAKE")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* ?????? "Space Attack II"                            */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( spaceatt )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode (not used) */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Zenitone Microsec "Invaders Revenge"                */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( invrvnge )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_START_TAG("FAKE")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


static MACHINE_DRIVER_START( invrvnge )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")

	/* video hardware */
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Space Invaders II Cocktail"                 */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( invad2ct )
	PORT_START_TAG("IN0")
	PORT_SERVICE(0x01, IP_ACTIVE_LOW) 			  /* dip 8 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* tied to pull-down */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* tied to pull-up */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* tied to pull-down */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* tied to pull-up */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* tied to pull-up */
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* labelled reset but tied to pull-up */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* tied to pull-up */

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* tied to pull-down */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )  /* tied to pull-up */

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) /* dips 4 & 3 */
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )   /* tied to pull-up */
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* dip 2 */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Bonus_Life ) ) /* dip 1 */
	PORT_DIPSETTING(    0x80, "1500" )
	PORT_DIPSETTING(    0x00, "2000" )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode (not used) */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static MACHINE_DRIVER_START( invad2ct )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_MACHINE_RESET(invad2ct)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(invad2ct_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invad2ct_sn76477_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invad2ct_sn76477_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Yachiyo "Space Stranger"                            */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( sstrangr_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x41, 0x41) AM_READ(input_port_2_r)
	AM_RANGE(0x42, 0x42) AM_READ(input_port_1_r)
	AM_RANGE(0x44, 0x44) AM_READ(input_port_4_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sstrangr_writeport, ADDRESS_SPACE_IO, 8 )
	/* no shifter circuit */
ADDRESS_MAP_END

INPUT_PORTS_START( sstrangr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x01, "Extra Play" )
	PORT_DIPSETTING(    0x00, "Never" )
	PORT_DIPSETTING(    0x01, "3000" )
	PORT_DIPSETTING(    0x02, "4000" )
	PORT_DIPSETTING(    0x03, "5000" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "2000" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Must be ACTIVE_LOW for game to boot */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )

	PORT_START_TAG("EXT")      /* External switches */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_DIPNAME( 0x02, 0x00, "Player's Bullet Speed (Cheat)" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x02, "Fast" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( sstrangr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(sstrangr_readport,sstrangr_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_MACHINE_RESET(sstrangr)

	/* video hardware */
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(invaders_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invaders_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Yachiyo "Space Stranger 2"                          */
/*                                                     */
/*******************************************************/

/* colour version of Space Stranger, board has Stranger 2 written on it */

static ADDRESS_MAP_START( sstrngr2_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(MRA8_RAM, c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x63ff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( sstrngr2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x01, "Extra Play" )
	PORT_DIPSETTING(    0x00, "Never" )
	PORT_DIPSETTING(    0x01, "3000" )
	PORT_DIPSETTING(    0x02, "4000" )
	PORT_DIPSETTING(    0x03, "5000" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "2000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR(Coinage) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )

	PORT_START_TAG("EXT")      /* External switches */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_DIPNAME( 0x02, 0x00, "Player's Bullet Speed (Cheat)" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x02, "Fast" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( sstrngr2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sstrngr2_cpu_map,0)
	MDRV_CPU_IO_MAP(sstrangr_readport,sstrangr_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_MACHINE_RESET(sstrangr)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(invaders_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invaders_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Taito "Space Laser"                                 */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( spclaser )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /*This is not 2 Player*/
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	/*PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
    PORT_DIPNAME( 0x80, 0x00, DEF_STR(Coinage) )
    PORT_DIPSETTING(    0x00, "1 Coin/1 or 2 Players" )
    PORT_DIPSETTING(    0x80, "1 Coin/1 Player  2 Coins/2 Players" )   Irrelevant, causes bugs*/

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode (not used) */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/*******************************************************/
/*                                                     */
/* Space War (Leijac)                                  */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( spcewarl )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
    PORT_DIPNAME( 0x80, 0x00, DEF_STR(Coinage) )
    PORT_DIPSETTING(    0x00, "1 Coin/1 or 2 Players" )
    PORT_DIPSETTING(    0x80, "1 Coin/1 Player  2 Coins/2 Players" )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode (not used) */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/*******************************************************/
/*                                                     */
/* Space War Part 3                                    */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( spacewr3 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Taito "Galaxy Wars"                                 */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( galxwars )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* must be IP_ACTIVE_LOW for Universal Sets */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Taito "Lunar Rescue"                                */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( lrescue )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Universal "Cosmic Monsters"                         */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( cosmicmo )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )


	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Sidam "Invasion"                                    */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( invasion )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* must be ACTIVE_HIGH Super Invaders */

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1500" )
	PORT_DIPSETTING(    0x00, "2500" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Laser Bonus Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END



/*******************************************************/
/*                                                     */
/* Nichibutsu "Rolling Crash"                          */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( rollingc_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)
//  AM_RANGE(0x2000, 0x2002) AM_READ(MRA8_RAM)
//  AM_RANGE(0x2003, 0x2003) AM_READ(hack)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xa000, 0xbfff) AM_READ(schaser_colorram_r)
	AM_RANGE(0xe400, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rollingc_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(schaser_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xe400, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

INPUT_PORTS_START( rollingc )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) /* Game Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) /* Game Select */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( rollingc )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(rollingc_readmem,rollingc_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)

	/* sound hardware */
MACHINE_DRIVER_END



/*******************************************************/
/*                                                     */
/* Midway "Space Encounters"                           */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( spcenctr_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(spcenctr_port_0_r) /* These 2 ports use Gray's binary encoding */
	AM_RANGE(0x01, 0x01) AM_READ(spcenctr_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
ADDRESS_MAP_END


INPUT_PORTS_START( spcenctr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x3f, 0x1f, IPT_AD_STICK_X ) PORT_MINMAX(0,0x3f) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_REVERSE /* 6 bit horiz encoder - Gray's binary */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )    /* fire */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x3f, 0x1f, IPT_AD_STICK_Y ) PORT_MINMAX(0,0x3f) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) /* 6 bit vert encoder - Gray's binary */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN2")      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "2000 4000 8000" )
	PORT_DIPSETTING(    0x01, "3000 6000 12000" )
	PORT_DIPSETTING(    0x02, "4000 8000 16000" )
	PORT_DIPSETTING(    0x03, "5000 10000 20000" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x30, 0x00, "Bonus/Test Mode" )
	PORT_DIPSETTING(    0x00, "Bonus On" )
	PORT_DIPSETTING(    0x30, "Bonus Off" )
	PORT_DIPSETTING(    0x20, "Cross Hatch" )
	PORT_DIPSETTING(    0x10, "Test Mode" )
	PORT_DIPNAME( 0xc0, 0x00, "Time" )
	PORT_DIPSETTING(    0x00, "45" )
	PORT_DIPSETTING(    0x40, "60" )
	PORT_DIPSETTING(    0x80, "75" )
	PORT_DIPSETTING(    0xc0, "90" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( spcenctr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(spcenctr_readport,0)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Gun Fight"                                  */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( gunfight_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(boothill_shift_data_r)
ADDRESS_MAP_END

INPUT_PORTS_START( gunfight )
    /* Gun position uses bits 4-6, handled using fake paddles */
	PORT_START_TAG("IN0")      /* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)        /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Fire")             /* Fire */

	PORT_START_TAG("IN1")      /* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY              /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Fire")                   /* Fire */

#ifdef NOTDEF
	PORT_START_TAG("IN2")      /* IN2 Dips & Coins */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x0C, 0x00, "Plays" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0C, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Time" ) /* These are correct */
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPSETTING(    0x30, "90" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Player" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Players" )
	PORT_DIPSETTING(    0x80, "1 Coin/3 Players" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Players" )
#endif

	PORT_START_TAG("IN2")      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "1 Coin" )
	PORT_DIPSETTING(    0x01, "2 Coins" )
	PORT_DIPSETTING(    0x02, "3 Coins" )
	PORT_DIPSETTING(    0x03, "4 Coins" )
	PORT_DIPNAME( 0x0C, 0x00, "Plays" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0C, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Time" ) /* These are correct */
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPSETTING(    0x30, "90" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START_TAG("FAKE1")                                        /* Player 2 Gun */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(1,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_CODE_DEC(KEYCODE_M) PORT_CODE_INC(KEYCODE_J) PORT_PLAYER(2)

	PORT_START_TAG("FAKE2")                                         /* Player 1 Gun */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(1,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_H)
INPUT_PORTS_END

static MACHINE_DRIVER_START( gunfight )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(gunfight_readport,writeport_2_4)
	MDRV_MACHINE_RESET(gunfight)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "M-4"                                        */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( m4 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Trigger")/* left trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Reload")/* left reload */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_2WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_NAME("P1 Trigger")/* right trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_NAME("P1 Reload")/* right reload */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START_TAG("IN2")      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Time" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x04, "70" )
	PORT_DIPSETTING(    0x08, "80" )
	PORT_DIPSETTING(    0x0C, "90" )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static MACHINE_DRIVER_START( m4 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(gunfight_readport,writeport_1_2)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Boot Hill"                                  */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( boothill )
    /* Gun position uses bits 4-6, handled using fake paddles */
	PORT_START_TAG("IN0")      /* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)        /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Fire")/* Fire */

	PORT_START_TAG("IN1")      /* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Fire")/* Fire */

	PORT_START_TAG("IN2")      /* Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
//  PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, "Time" )
	PORT_DIPSETTING(    0x00, "60 seconds" )
	PORT_DIPSETTING(    0x04, "70 seconds" )
	PORT_DIPSETTING(    0x08, "80 seconds" )
	PORT_DIPSETTING(    0x0C, "90 seconds" )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("FAKE1")                                             /* Player 2 Gun */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(1,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_M) PORT_CODE_INC(KEYCODE_J) PORT_CENTERDELTA(0) PORT_PLAYER(2)

	PORT_START_TAG("FAKE2")                                             /* Player 1 Gun */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(1,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_H) PORT_CENTERDELTA(0)
INPUT_PORTS_END

static MACHINE_DRIVER_START( boothill )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(gunfight_readport,writeport_1_2)
	MDRV_MACHINE_RESET(boothill)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(boothill_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Taito "Space Chaser"                                */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( schaser_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_READ(schaser_colorram_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( schaser_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(schaser_colorram_w) AM_BASE(&colorram)
ADDRESS_MAP_END

INPUT_PORTS_START( schaser )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x40, 0x00, "Number of Controllers" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )

	PORT_START_TAG("4")
	PORT_ADJUSTER( 70, "VR1 - Music Volume" )

	PORT_START_TAG("5")
	PORT_ADJUSTER( 90, "VR2 - Explosion/Effect Volume" )

	PORT_START_TAG("6")
	PORT_ADJUSTER( 70, "VR3 - Dot Volume" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( schaser )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_REPLACE("main",8080,1996800)        /* 19.968MHz / 10 */
	MDRV_CPU_PROGRAM_MAP(schaser_readmem,schaser_writemem)
	MDRV_WATCHDOG_VBLANK_INIT(255)
	MDRV_MACHINE_RESET(schaser)

	/* video hardware */

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(schaser_sn76477_interface)
	// This will be routed to the discrete system when that feature is working.
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(schaser_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Taito "Space Chaser" (CV version)                   */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( schasrcv )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

/*******************************************************/
/*                                                     */
/* Taito "Straight Flush"                              */
/*                                                     */
/*******************************************************/

static int sfl_int=0;

static READ8_HANDLER( sfl_input_r )
{
	sfl_int^=0x80;//vblank flag ?
	return sfl_int|input_port_1_r(0);
}

static ADDRESS_MAP_START( sflush_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_RAM) //?
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa000, 0xbfff) AM_READ(schaser_colorram_r)
	AM_RANGE(0x8008, 0x8008) AM_READ(input_port_2_r)
	AM_RANGE(0x8009, 0x8009) AM_READ(c8080bw_shift_data_r)
	AM_RANGE(0x800a, 0x800a) AM_READ(sfl_input_r)
	AM_RANGE(0x800b, 0x800b) AM_READ(input_port_0_r)
	AM_RANGE(0xd800, 0xffff) AM_READ(MRA8_ROM)

ADDRESS_MAP_END

static ADDRESS_MAP_START( sflush_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8018, 0x8018) AM_WRITE(c8080bw_shift_data_w)
	AM_RANGE(0x8019, 0x8019) AM_WRITE(c8080bw_shift_amount_w)
	AM_RANGE(0x801a, 0x801a) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x801c, 0x801c) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x801d, 0x801d) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(schaser_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xd800, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static MACHINE_DRIVER_START( sflush )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_REPLACE("main",M6800,2000000)        /* ?? */
	MDRV_CPU_PROGRAM_MAP(sflush_readmem,sflush_writemem)
	MDRV_CPU_IO_MAP(0,0)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,2)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(sflush)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 4*8, 30*8-1)

MACHINE_DRIVER_END

INPUT_PORTS_START( sflush )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_TILT  )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x08, 0x00, "Hiscore" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "30 000" )
	PORT_DIPNAME( 0x40, 0x00, "Coinage Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0x6a, IPT_PADDLE ) PORT_MINMAX(0x16,0xbf) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_CENTERDELTA(0)
INPUT_PORTS_END



/*******************************************************/
/*                                                     */
/* Midway "Clowns"                                     */
/*                                                     */
/*******************************************************/

/*
 * Clowns (EPROM version)
 */
INPUT_PORTS_START( clowns )
	PORT_START_TAG("IN0")
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Game" )
	PORT_DIPSETTING(    0x00, "No Bonus" )
	PORT_DIPSETTING(    0x04, "9000" )
	PORT_DIPSETTING(    0x08, "11000" )
	PORT_DIPSETTING(    0x0c, "13000" )
	PORT_DIPNAME( 0x10, 0x00, "Balloon Resets" )
	PORT_DIPSETTING(    0x00, "Each row" )
	PORT_DIPSETTING(    0x10, "All rows" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "4000" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	/* Test Mode - Open Coin SW for RAM-ROM Test,
    Close Coin SW for Springboard Alignment */
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Test) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( clowns1 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Balloon Resets" )
	PORT_DIPSETTING(    0x00, "Each row" )
	PORT_DIPSETTING(    0x10, "All rows" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "4000" )
	PORT_DIPNAME( 0x40, 0x00, "Input Test"  )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Test ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( clowns )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(c8080bw_readport,writeport_1_2)
	MDRV_MACHINE_RESET(clowns)
	MDRV_WATCHDOG_VBLANK_INIT(255)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(circus_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.70)

	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(clowns_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Guided Missile"                             */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( gmissile )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Time" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x04, "80" )
	PORT_DIPSETTING(    0x0c, "90" )
	PORT_DIPNAME( 0x30, 0x00, "Extra Play" )
	PORT_DIPSETTING(    0x00, "500" )
	PORT_DIPSETTING(    0x20, "700" )
	PORT_DIPSETTING(    0x10, "1000" )
	PORT_DIPSETTING(    0x30, "1300" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Midway "280 ZZZAP"                                  */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( 280zzzap )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0f, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0x0f) PORT_SENSITIVITY(100) PORT_KEYDELTA(64)	/* accelerator */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_TOGGLE PORT_NAME("P1 Shift") /* shift */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START_TAG("IN1")      /* Steering Wheel */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x00, "Time" )
	PORT_DIPSETTING(    0x0c, "60" )
	PORT_DIPSETTING(    0x00, "80" )
	PORT_DIPSETTING(    0x08, "99" )
	PORT_DIPSETTING(    0x04, "Test Mode" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Time" )
	PORT_DIPSETTING(    0x00, "Score >= 2.5" )
	PORT_DIPSETTING(    0x10, "Score >= 2" )
	PORT_DIPSETTING(    0x20, DEF_STR( None ) )
/* 0x30 same as 0x20 */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Language ))
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( German ) )
	PORT_DIPSETTING(    0x80, DEF_STR( French ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Spanish ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( 280zzzap )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(c8080bw_readport,writeport_4_3)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Taito "Lupin III"                                   */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( lupin3 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* selects color mode (dynamic vs. static) */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* something has to do with sound */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x00, "Bags to Collect" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Japanese ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,  IPT_UNUSED )
	PORT_DIPNAME(0x80,  0x00, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( lupin3 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(schaser_readmem,schaser_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Taito "Polaris"                                     */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( polaris )

	PORT_START_TAG("IN0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	/* 0x04 should be Cabinet - Upright/Cocktail,
       but until the cocktail hack is changed,
       this will have to do. */
	PORT_DIPNAME( 0x04, 0x00, "Number of Controls" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPNAME( 0x08, 0x00, "Invincible Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	/* The Demo Sounds dip switch does function.
     * It allows the sonar sounds to play in demo mode. */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Not Used" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Not Used" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "High Score Preset Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )

	PORT_START_TAG("4")
	PORT_ADJUSTER( 80, "Sub Volume VR1" )

	PORT_START_TAG("5")
	PORT_ADJUSTER( 70, "Sub Volume VR2" )

	PORT_START_TAG("6")
	PORT_ADJUSTER( 90, "Sub Volume VR3" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( polaris )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_REPLACE("main",8080,1996800)        /* 19.968MHz / 10 */
	MDRV_CPU_PROGRAM_MAP(schaser_readmem,schaser_writemem)
	MDRV_WATCHDOG_VBLANK_INIT(255)

	MDRV_CPU_IO_MAP(c8080bw_readport,writeport_0_3)
	MDRV_CPU_VBLANK_INT(polaris_interrupt,2)
	MDRV_MACHINE_RESET(polaris)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(polaris_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Laguna Racer"                               */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( lagunar )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0f, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0x0f) PORT_SENSITIVITY(100) PORT_KEYDELTA(64)	/* accelerator */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_TOGGLE  PORT_NAME ("P1 Shift")/* shift */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START_TAG("IN2")      /* Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Time" )
	PORT_DIPSETTING(    0x00, "45" )
	PORT_DIPSETTING(    0x04, "60" )
	PORT_DIPSETTING(    0x08, "75" )
	PORT_DIPSETTING(    0x0c, "90" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Time" )
	PORT_DIPSETTING(    0x00, "350" )
	PORT_DIPSETTING(    0x10, "400" )
	PORT_DIPSETTING(    0x20, "450" )
	PORT_DIPSETTING(    0x30, "500" )
	PORT_DIPNAME( 0xc0, 0x00, "Test Modes")
	PORT_DIPSETTING(    0x00, "Play Mode" )
	PORT_DIPSETTING(    0x40, "RAM/ROM" )
	PORT_DIPSETTING(    0x80, "Steering" )
	PORT_DIPSETTING(    0xc0, "No Extended Play" )
INPUT_PORTS_END



/*******************************************************/
/*                                                     */
/* Midway "Phantom II"                                 */
/*                                                     */
/*******************************************************/

static PALETTE_INIT( phantom2 )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0xff,0xff,0xff); /* white */
	palette_set_color(2,0xc0,0xc0,0xc0); /* grey */
}


INPUT_PORTS_START( phantom2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN2")      /* Dips & Coins */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, "Time" )
	PORT_DIPSETTING(    0x00, "45sec 20sec 20" )
	PORT_DIPSETTING(    0x02, "60sec 25sec 25" )
	PORT_DIPSETTING(    0x04, "75sec 30sec 30" )
	PORT_DIPSETTING(    0x06, "90sec 35sec 35" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )

INPUT_PORTS_END

static MACHINE_DRIVER_START( phantom2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(gunfight_readport,writeport_1_2)
	MDRV_CPU_VBLANK_INT(phantom2_interrupt,2)
	MDRV_MACHINE_RESET(phantom2)

	/* video hardware */
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	MDRV_PALETTE_LENGTH(3)
	MDRV_PALETTE_INIT(phantom2)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Dog Patch"                                  */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( dogpatch )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x38, 0x1f, IPT_AD_STICK_X ) PORT_MINMAX(0x05,0x48) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_PLAYER(2)
	/* 6 bit horiz encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x3f, 0x1f, IPT_AD_STICK_X ) PORT_MINMAX(0x01,0x3e) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) /* 6 bit horiz encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, "# Cans" )
	PORT_DIPSETTING(    0x03, "10" )
	PORT_DIPSETTING(    0x02, "15" )
	PORT_DIPSETTING(    0x01, "20" )
	PORT_DIPSETTING(    0x00, "25" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x10, "3 extra cans" )
	PORT_DIPSETTING(    0x00, "5 extra cans" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0xc0, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0xc0, "150 Pts" )
	PORT_DIPSETTING(    0x80, "175 Pts" )
	PORT_DIPSETTING(    0x40, "225 Pts" )
	PORT_DIPSETTING(    0x00, "275 Pts" )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Midway "4 Player Bowling"                           */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( bowler_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(c8080bw_shift_data_comp_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_0_r)				/* dip switch */
	AM_RANGE(0x04, 0x04) AM_READ(input_port_1_r)				/* coins / switches */
	AM_RANGE(0x05, 0x05) AM_READ(input_port_2_r)				/* ball vert */
	AM_RANGE(0x06, 0x06) AM_READ(input_port_3_r)				/* ball horz */
ADDRESS_MAP_END

INPUT_PORTS_START( bowler )
	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x01, DEF_STR( French ) )
	PORT_DIPSETTING(    0x02, DEF_STR( German ) )
  	PORT_DIPSETTING(    0x03, DEF_STR( German ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )				/* Every 17 minutes */
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Game_Time ) )
	PORT_DIPSETTING(    0x00, "No Limit" )
	PORT_DIPSETTING(    0x08, "5 Min" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Test ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("IN5")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START_TAG("IN6")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10)
INPUT_PORTS_END

static MACHINE_DRIVER_START( bowler )


	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(bowler_readport,writeport_1_2)
	MDRV_MACHINE_RESET(bowler)

	/* video hardware */
	MDRV_SCREEN_SIZE(35*8, 32*8)	/* Extra 3 lines for the bonus display */
	MDRV_VISIBLE_AREA(0*8, 35*8-1, 4*8, 32*8-1)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Shuffleboard"                               */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( shuffle_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(c8080bw_shift_data_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_0_r)				/* dip switch */
	AM_RANGE(0x04, 0x04) AM_READ(input_port_1_r)				/* coins / switches */
	AM_RANGE(0x05, 0x05) AM_READ(input_port_2_r)				/* ball vert */
	AM_RANGE(0x06, 0x06) AM_READ(input_port_3_r)				/* ball horz */
ADDRESS_MAP_END

INPUT_PORTS_START( shuffle )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x01, DEF_STR( French ) )
	PORT_DIPSETTING(    0x02, DEF_STR( German ) )
  /*PORT_DIPSETTING(    0x03, DEF_STR( German ) )*/
	PORT_DIPNAME( 0x0c, 0x04, "Points to Win" )
	PORT_DIPSETTING(    0x00, "25" )
	PORT_DIPSETTING(    0x04, "35" )
	PORT_DIPSETTING(    0x08, "40" )
	PORT_DIPSETTING(    0x0c, "50" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x30, "2 Coins/1 Player  4 Coins/2 Players" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 or 2 Players" )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Player  2 Coins/2 Players" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 or 2 Players" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )	/* time limit? */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Game Select")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10)

	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_REVERSE
INPUT_PORTS_END

static MACHINE_DRIVER_START( shuffle )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(shuffle_readport,writeport_1_2)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Sea Wolf"                                   */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( seawolf_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(c8080bw_shift_data_rev_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_READ(c8080bw_shift_data_r)
ADDRESS_MAP_END

INPUT_PORTS_START( seawolf )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x1f, 0x01, IPT_PADDLE ) PORT_MINMAX(0,0x1f) PORT_SENSITIVITY(20) PORT_KEYDELTA(5) PORT_CENTERDELTA(0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0xc0, 0x00, "Time" )
	PORT_DIPSETTING(    0x00, "61" )
	PORT_DIPSETTING(    0x40, "71" )
	PORT_DIPSETTING(    0x80, "81" )
	PORT_DIPSETTING(    0xc0, "91" )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_TILT ) // Reset High Scores
	PORT_DIPNAME( 0xe0, 0x20, "Extended Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x20, "2000" )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPSETTING(    0x60, "4000" )
	PORT_DIPSETTING(    0x80, "5000" )
	PORT_DIPSETTING(    0xa0, "6000" )
	PORT_DIPSETTING(    0xc0, "7000" )
	PORT_DIPSETTING(    0xe0, "Test Mode" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( seawolf )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(seawolf_readport,writeport_4_3)
	MDRV_MACHINE_RESET(seawolf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(seawolf_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Blue Shark"                                 */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( blueshrk )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x7f, 0x45, IPT_PADDLE ) PORT_MINMAX(0xf,0x7f) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x60, 0x20, "Replay" )
	PORT_DIPSETTING(    0x20, "14000" )
	PORT_DIPSETTING(    0x40, "18000" )
	PORT_DIPSETTING(    0x60, "22000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

static MACHINE_DRIVER_START( blueshrk )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(seawolf_readport,writeport_1_2)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Desert Gun"                                 */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( desertgu )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x7f, 0x55, IPT_LIGHTGUN_X ) PORT_MINMAX(0xf,0x7f) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, "Time" )
	PORT_DIPSETTING(    0x00, "40" )
	PORT_DIPSETTING(    0x01, "50" )
	PORT_DIPSETTING(    0x02, "60" )
	PORT_DIPSETTING(    0x03, "70" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x04, DEF_STR( German ) )
	PORT_DIPSETTING(    0x08, DEF_STR( French ) )
	PORT_DIPSETTING(    0x0c, "Norwegian?" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "7000" )
	PORT_DIPSETTING(    0x20, "9000" )
	PORT_DIPSETTING(    0x30, "Test Mode" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x7f, 0x45, IPT_LIGHTGUN_Y ) PORT_MINMAX(0xf,0x7f) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)
INPUT_PORTS_END

static MACHINE_DRIVER_START( desertgu )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(seawolf_readport,writeport_1_2)
	MDRV_MACHINE_RESET(desertgu)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Midway "Extra Innings"                              */
/*                                                     */
/*******************************************************/

/*
 * The cocktail version has independent bat, pitch, and field controls
 * while the upright version ties the pairs of inputs together through
 * jumpers in the wiring harness.
 */
INPUT_PORTS_START( einnings )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 (Home) Bat")	/* home bat */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_NAME("P2 (Home) Fielders Left")	/* home fielders left */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_NAME("P2 (Home) Fielders Right")	/* home fielders right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)	PORT_NAME("P2 (Home) Pitch Left")	/* home pitch left */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_NAME("P2 (Home) Pitch Right")	/* home pitch right */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 (Home) Pitch Slow")			/* home pitch slow */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2) PORT_NAME("P2 (Home) Pitch Fast")			/* home pitch fast */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 (Visitor) Bat")		/* visitor bat */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("P1 (Visitor) Fielders Left")	/* visitor fielders left */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	PORT_NAME("P1 (Visitor) Fielders Right")/* visitor fielders right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_NAME("P1 (Visitor) Pitch Left")/* visitor pitch left */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	PORT_NAME("P1 (Visitor) Pitch Right")/* visitor pitch right */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )		PORT_NAME("P1 (Visitor) Pitch Slow")/* visitor pitch slow */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )		PORT_NAME("P1 (Visitor) Pitch Fast")/* visitor pitch fast */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x02, "2C/1 In (1 or 2 Players)" )
	PORT_DIPSETTING(    0x03, "2C/1 In 4C/3 In (1 or 2 Pls)" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Inning (1 or 2 Pls)" )
	PORT_DIPSETTING(    0x01, "1C/1 In 2C/3 In (1 or 2 Pls)" )
	PORT_DIPSETTING(    0x04, "1C/1Pl 2C/2Pl 4C/3Inn" )
	PORT_DIPSETTING(    0x05, "2C/1Pl 4C/2Pl 8C/3Inn" )
/* 0x06 and 0x07 same as 0x00 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Midway "Amazing Maze"                               */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( maze )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1  )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Midway "Tornado Baseball"                           */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( tornbase )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("DSW0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_DIPNAME( 0x78, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x18, "4 Coins/1 Inning 32/9" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Inning 24/9" )
	PORT_DIPSETTING(    0x38, "4 Coins/2 Innings 16/9" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Inning 16/9" )
	PORT_DIPSETTING(    0x30, "3 Coins/2 Innings 12/9" )
	PORT_DIPSETTING(    0x28, "2 Coins/2 Innings 8/9" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Inning 8/9" )
	PORT_DIPSETTING(    0x58, "4 Coins/4 Innings 8/9" )
	PORT_DIPSETTING(    0x50, "3 Coins/4 Innings 6/9" )
	PORT_DIPSETTING(    0x48, "2 Coins/4 Innings 4/9" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Innings 4/9" )
	PORT_DIPSETTING(    0x40, "1 Coin/4 Innings 2/9" )
	PORT_DIPSETTING(    0x78, "4 Coins/9 Innings" )
	PORT_DIPSETTING(    0x70, "3 Coins/9 Innings" )
	PORT_DIPSETTING(    0x68, "2 Coins/9 Innings" )
	PORT_DIPSETTING(    0x60, "1 Coin/9 Innings" )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Midway "Checkmate"                                  */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( checkmat_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)
ADDRESS_MAP_END


INPUT_PORTS_START( checkmat )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "1 Coin/1 or 2 Players" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 to 4 Players" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, "Rounds" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "Language?" )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x20, DEF_STR( German ) )
	PORT_DIPSETTING(    0x40, DEF_STR( French ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Spanish ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END

static MACHINE_DRIVER_START( checkmat )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(checkmat_readport,0)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Taito "Ozma Wars"                                   */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( ozmawars )
	PORT_START_TAG("IN0")

	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, "Energy" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "25000" )
	PORT_DIPSETTING(    0x03, "35000" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Bonus Energy" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_DIPSETTING(    0x08, "10000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( spaceph )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, "Energy" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "25000" )
	PORT_DIPSETTING(    0x03, "35000" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Bonus Energy" )
	PORT_DIPSETTING(    0x08, "10000" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
INPUT_PORTS_END



/*******************************************************/
/*                                                     */
/* Emag "Super Invaders"                               */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( sinvemag )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END



/*******************************************************/
/*                                                     */
/* Jatre Specter (Taito?)                              */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( jspecter )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Difficulty ) ) //Bug reports imply this is a toggle switch, confirm?
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*******************************************************/
/*                                                     */
/* Taito "Balloon Bomber"                              */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( ballbomb )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( ballbomb )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_MACHINE_RESET(ballbomb)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Wing "Yosaku To Donbei"                             */
/*                                                     */
/*******************************************************/

static ADDRESS_MAP_START( yosakdon_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( yosakdon_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(MWA8_RAM) /* what's this? */
ADDRESS_MAP_END


static ADDRESS_MAP_START( yosakdon_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( yosakdon_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x03, 0x03) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x05, 0x05) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP) /* character numbers */
ADDRESS_MAP_END


INPUT_PORTS_START( yosakdon )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static MACHINE_DRIVER_START( yosakdon )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(yosakdon_readmem, yosakdon_writemem)
	MDRV_CPU_IO_MAP(yosakdon_readport, yosakdon_writeport)

	/* video hardware */
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 32*8-1)

	/* sound hardware */
MACHINE_DRIVER_END


/*******************************************************/
/*                                                     */
/* Leijac "Space King"                                 */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( spceking )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "High Score Preset Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x00, "Coin Info" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

/*******************************************************/
/*                                                     */
/* Taito "Steel Worker"                                */
/*                                                     */
/*******************************************************/


static WRITE8_HANDLER( steelwkr_sh_port_3_w )
{
	coin_lockout_global_w(!(~data & 0x03));		/* possibly */
}

static ADDRESS_MAP_START( steelwkr_readport, ADDRESS_SPACE_IO, 8 )
/*	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r) */
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(c8080bw_shift_data_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( steelwkr_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(c8080bw_shift_amount_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(c8080bw_shift_data_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(steelwkr_sh_port_3_w)
ADDRESS_MAP_END

INPUT_PORTS_START( steelwkr )
	PORT_START_TAG("IN0")
/*
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
*/
	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )									
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )										
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( steelwkr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(invaders)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(steelwkr_readport,steelwkr_writeport)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(invadpt2)
MACHINE_DRIVER_END

/*******************************************************/
/*                                                     */
/* Taito  "Indian battle"                                 */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( indianbt )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, "Number of Catch Animals" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_DIPNAME(0x80,  0x00, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DUMMY") /*  cabinet fake port  must be 4th  */

	PORT_START_TAG("FAKE")		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


/*
 Protection / sound hw checks ?

 ld    a ,$b
 out  ($03),a
 out  ($01),a
 in   a,($00)
 and  $f0
 cp   $10
 jp   nz,$3000
 ld   a,$03
 out  ($03),a
 out  ($01),a
 in   a,($00)
 jp   $5de7
 and  $f0
 jp   z,$052b
 jp   $3000

*/

static READ8_HANDLER(indianbt_r)
{
	switch(activecpu_get_pc())
	{
		case 0x5fed:	return 0x10;
		case 0x5ffc:	return 0;
	}
	logerror("unknown port 0 read @ %x\n",activecpu_get_pc());
	return rand();
}

static ADDRESS_MAP_START( indianbt_port, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(indianbt_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r) AM_WRITE(c8080bw_shift_amount_w)
	AM_RANGE(0x03, 0x03) AM_READ(c8080bw_shift_data_r)
	AM_RANGE(0x04, 0x04) AM_WRITE(c8080bw_shift_data_w)
	AM_RANGE(0x06, 0x06) AM_WRITENOP /* sound ? */
	AM_RANGE(0x07, 0x07) AM_WRITE(indianbt_sh_port7_w)

ADDRESS_MAP_END

static MACHINE_DRIVER_START( indianbt )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(invaders)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(indianbt_port,0)

	/* video hardware */
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(indianbt)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(indianbt_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

MACHINE_DRIVER_END

/*****************************************************

 Omori "Shuttle Invader" ??

 8080 CPU

 1x  SN76477
 2x  SN75452
 4x  8216 RAM
 2x  TMS4045 RAM
 16x MCM4027 RAM
 1x  empty small socket. maybe (missing) PROM?
 1x  8 position dipsw
 1x  556
 1x  458
 1x  lm380 (amp chip)

 xtal 18MHz
 xtal 5.545MHz

******************************************************/

INPUT_PORTS_START( shuttlei )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x06, "6" )

	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
INPUT_PORTS_END

static ADDRESS_MAP_START( shuttlei_memory_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_ROM
	AM_RANGE(0x1c00, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(MRA8_RAM, c8080bw_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x43ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( shuttlei_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xfd, 0xfd) AM_WRITENOP
	AM_RANGE(0xfe, 0xfe) AM_READ(input_port_0_r) AM_WRITENOP
	AM_RANGE(0xff, 0xff) AM_READ(input_port_1_r) AM_WRITENOP
ADDRESS_MAP_END

static MACHINE_DRIVER_START( shuttlei )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(8080bw)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(shuttlei_memory_map,0)
	MDRV_CPU_IO_MAP(shuttlei_io_map,0)

	/* video hardware */
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 24*8-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	/* sound hardware */
MACHINE_DRIVER_END





ROM_START( invaders )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "invaders.h",   0x0000, 0x0800, CRC(734f5ad8) SHA1(ff6200af4c9110d8181249cbcef1a8a40fa40b7f) )
	ROM_LOAD( "invaders.g",   0x0800, 0x0800, CRC(6bfaca4a) SHA1(16f48649b531bdef8c2d1446c429b5f414524350) )
	ROM_LOAD( "invaders.f",   0x1000, 0x0800, CRC(0ccead96) SHA1(537aef03468f63c5b9e11dd61e253f7ae17d9743) )
	ROM_LOAD( "invaders.e",   0x1800, 0x0800, CRC(14e538b0) SHA1(1d6ca0c99f9df71e2990b610deb9d7da0125e2d8) )
ROM_END

ROM_START( searthin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )             /* 64k for code */
	ROM_LOAD( "earthinv.h",   0x0000, 0x0800, CRC(58a750c8) SHA1(90bfa4ea06f38e67fe4286d37d151632439249d2) )
	ROM_LOAD( "earthinv.g",   0x0800, 0x0800, CRC(b91742f1) SHA1(8d9ca92405fbaf1d5a7138d400986616378d061e) )
	ROM_LOAD( "earthinv.f",   0x1000, 0x0800, CRC(4acbbc60) SHA1(b8c1efb4251a1e690ff6936ec956d6f66136a085) )
	ROM_LOAD( "earthinv.e",   0x1800, 0x0800, CRC(df397b12) SHA1(e7e8c080cb6baf342ec637532e05d38129ae73cf) )
ROM_END

ROM_START( searthia )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "unkh.h1",   0x0000, 0x0400, CRC(272b9bf3) SHA1(dd57d6a88d42024a39640931114107b547b4c520) )
	ROM_LOAD( "unkg.g1",   0x0400, 0x0400, CRC(61bb6101) SHA1(8fc8bbd8ac93d239e0cf0e4881f709860ec2c973) )
	ROM_LOAD( "unkf.f1",   0x0800, 0x0400, CRC(2a8d9cd5) SHA1(7948d79b326e729bcb629607c8797156ff9fb0e8) )
	ROM_LOAD( "unke.e1",   0x0c00, 0x0400, CRC(1938d349) SHA1(3bd2a0deb126cf2e22bc3cb53e9a59c3875be260) )
	ROM_LOAD( "unkd.d1",   0x1000, 0x0400, CRC(9bc2ab88) SHA1(1e9f3b780135827d16ba25978382b097a8110828) )
	ROM_LOAD( "unkc.c1",   0x1400, 0x0400, CRC(d4e2dada) SHA1(e98271212fc89e240fdf97d292edd17dc8dd4191) )
	ROM_LOAD( "unkb.b1",   0x1800, 0x0400, CRC(ab645a9c) SHA1(9c286f8a031a8babfb8e9b594e05e133c338b342) )
	ROM_LOAD( "unka.a1",   0x1c00, 0x0400, CRC(4b65bd7c) SHA1(3931f9f5b0e3339ab484eee14473d3a474935fd9) )
ROM_END


ROM_START( invadrmr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	/* yes, this rom is really on the PCB twice?! */
	ROM_LOAD( "11.1s",       0x0000, 0x0400, CRC(389d44b6) SHA1(5d2581b8bc0da918ce57cf319e06b5b31989c681) )
	ROM_LOAD( "11.1t",       0x0000, 0x0400, CRC(389d44b6) SHA1(5d2581b8bc0da918ce57cf319e06b5b31989c681) )

	ROM_LOAD( "sv02.1p",     0x0400, 0x0400, CRC(0e159534) SHA1(94b2015a9d38ca738705b8d024a79fd2f9855b98) )
	ROM_LOAD( "20.1n",       0x0800, 0x0400, CRC(805b04f0) SHA1(209f42dfde1593699ccf3755e9267d425416d910) )
	ROM_LOAD( "sv04.1j",     0x1400, 0x0400, CRC(1293b826) SHA1(165cd5d08a19eadbe954145b12807f10df9e691a) )
	ROM_LOAD( "13.1h",       0x1800, 0x0400, CRC(76b4a6ea) SHA1(076f8d12ba7ebe66b83a40d9a848075627776554) )
	ROM_LOAD( "sv06.1g",     0x1c00, 0x0400, CRC(2c68e0b4) SHA1(a5e5357120102ad32792bf3ef6362f45b7ba7070) )
ROM_END

ROM_START( spaceatt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "h",            0x0000, 0x0400, CRC(d0c32d72) SHA1(b3bd950b1ba940fbeb5d95e55113ed8f4c311434) )
	ROM_LOAD( "sv02.bin",     0x0400, 0x0400, CRC(0e159534) SHA1(94b2015a9d38ca738705b8d024a79fd2f9855b98) )
	ROM_LOAD( "f",            0x0800, 0x0400, CRC(483e651e) SHA1(ae795ee3bc53ac3936f6cf2c72cca7a890783513) )
	ROM_LOAD( "c",            0x1400, 0x0400, CRC(1293b826) SHA1(165cd5d08a19eadbe954145b12807f10df9e691a) )
	ROM_LOAD( "b",            0x1800, 0x0400, CRC(6fc782aa) SHA1(0275adbeec455e146f4443b0b836b1171436b79b) )
	ROM_LOAD( "a",            0x1c00, 0x0400, CRC(211ac4a3) SHA1(e08e90a4e77cfa30400626a484c9f37c87ea13f9) )
ROM_END

ROM_START( spaceat2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "spaceatt.h",   0x0000, 0x0800, CRC(a31d0756) SHA1(2b76929654ed0b180091348546dac29fc6e5438e) )
	ROM_LOAD( "spaceatt.g",   0x0800, 0x0800, CRC(f41241f7) SHA1(d93cead75922510075433849c4f7099279eafc18) )
	ROM_LOAD( "spaceatt.f",   0x1000, 0x0800, CRC(4c060223) SHA1(957e75a978aa600627399061cae0a6525e92ad11) )
	ROM_LOAD( "spaceatt.e",   0x1800, 0x0800, CRC(7cf6f604) SHA1(469557de15178c4b2d686e5724e1006f7c20d7a4) )
ROM_END

ROM_START( sinvzen )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1.bin",        0x0000, 0x0400, CRC(9b0da779) SHA1(a52ccdb252eb69c497aa5eafb35d7f25a311b44e) )
	ROM_LOAD( "2.bin",        0x0400, 0x0400, CRC(9858ccab) SHA1(5ad8e5ef0d95779f0e513634b97bc330c9269ce4) )
	ROM_LOAD( "3.bin",        0x0800, 0x0400, CRC(a1cc38b5) SHA1(45fc9466b548d511b8174f6f3a4783164dd59489) )
	ROM_LOAD( "4.bin",        0x0c00, 0x0400, CRC(1f2db7a8) SHA1(354ad155743f724f2bebcab422f1ef96cb57c683) )
	ROM_LOAD( "5.bin",        0x1000, 0x0400, CRC(9b505fcd) SHA1(7461b7087d31dbe09f7b3078584ccaa2c9122c95) )
	ROM_LOAD( "6.bin",        0x1400, 0x0400, CRC(de0ca0ae) SHA1(a15d1218361839a2a2bf8da3f78d81621251fe1c) )
	ROM_LOAD( "7.bin",        0x1800, 0x0400, CRC(25a296f6) SHA1(37df98384c1513f0e33a350dfcaa99655f91c9ba) )
	ROM_LOAD( "8.bin",        0x1c00, 0x0400, CRC(f4bc4a98) SHA1(bff3806750a3695a136f398c7dbb69a0b7daa88a) )
ROM_END

ROM_START( sinvemag )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sv0h.bin",     0x0000, 0x0400, CRC(86bb8cb6) SHA1(a75648e7f2446c756d86624b15d387d25ce47b66) )
	ROM_LOAD( "emag_si.b",    0x0400, 0x0400, CRC(febe6d1a) SHA1(e1c3a24b4fa5862107ada1f9d7249466e8c3f06a) )
	ROM_LOAD( "emag_si.c",    0x0800, 0x0400, CRC(aafb24f7) SHA1(6718cdfae09f77d735be5145b9d202a73d8ed9db) )
	ROM_LOAD( "emag_si.d",    0x1400, 0x0400, CRC(68c4b9da) SHA1(8953dc0427b09b71bd763e65caa7deaca09a15da) )
	ROM_LOAD( "emag_si.e",    0x1800, 0x0400, CRC(c4e80586) SHA1(3d427d5a2eea3c911ec7bd055e06e6747ce5e84d) )
	ROM_LOAD( "emag_si.f",    0x1c00, 0x0400, CRC(077f5ef2) SHA1(625de6839073ac4c904f949efc1b2e0afea5d676) )
ROM_END

ROM_START( tst_invd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "test.h",   0x0000, 0x0800, CRC(f86a2eea) SHA1(4a72ff01f3e6d16bbe9bf7f123cd98895bfbed9a) )   /*  The Test ROM */
	ROM_LOAD( "invaders.g",   0x0800, 0x0800, CRC(6bfaca4a) SHA1(16f48649b531bdef8c2d1446c429b5f414524350) )
	ROM_LOAD( "invaders.f",   0x1000, 0x0800, CRC(0ccead96) SHA1(537aef03468f63c5b9e11dd61e253f7ae17d9743) )
	ROM_LOAD( "invaders.e",   0x1800, 0x0800, CRC(14e538b0) SHA1(1d6ca0c99f9df71e2990b610deb9d7da0125e2d8) )
ROM_END

ROM_START( alieninv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1h.bin",       0x0000, 0x0800, CRC(c46df7f4) SHA1(eec34b3d5585bae03c7b80585daaa05ddfcc2164) )
	ROM_LOAD( "1g.bin",       0x0800, 0x0800, CRC(4b1112d6) SHA1(b693667656e5d8f44eeb2ea730f4d4db436da579) )
	ROM_LOAD( "1f.bin",       0x1000, 0x0800, CRC(adca18a5) SHA1(7e02651692113db31fd469868ae5ffdb0f941ecf) )
	ROM_LOAD( "1e.bin",       0x1800, 0x0800, CRC(0449cb52) SHA1(8adcb7cd4492fa6649d9ee81172d8dff56621d64) )
ROM_END

ROM_START( sitv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "tv0h.s1",      0x0000, 0x0800, CRC(fef18aad) SHA1(043edeefe6a6d4934bd384eafea19326de1dbeec) )
	ROM_LOAD( "tv02.rp1",     0x0800, 0x0800, CRC(3c759a90) SHA1(d847d592dee592b1d3a575c21d89eaf3f7f6ae1b) )
	ROM_LOAD( "tv03.n1",      0x1000, 0x0800, CRC(0ad3657f) SHA1(a501f316535c50f7d7a20ef8e6dede1526a3f2a8) )
	ROM_LOAD( "tv04.m1",      0x1800, 0x0800, CRC(cd2c67f6) SHA1(60f9d8fe2d36ff589277b607f07c1edc917c755c) )
ROM_END

ROM_START( sicv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cv17.bin",     0x0000, 0x0800, CRC(3dfbe9e6) SHA1(26487df7fa0bbd0b9b7f74347c4b9318b0a73b89) )
	ROM_LOAD( "cv18.bin",     0x0800, 0x0800, CRC(bc3c82bf) SHA1(33e39fc97bd46699be1f9b9741a86f433efdc911) )
	ROM_LOAD( "cv19.bin",     0x1000, 0x0800, CRC(d202b41c) SHA1(868fe938ef768655c894ec95b7d9a81bf21f69ca) )
	ROM_LOAD( "cv20.bin",     0x1800, 0x0800, CRC(c74ee7b6) SHA1(4f52db274a2d4433ab67c099ee805e8eb8516c0f) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "cv01_1.bin",   0x0000, 0x0400, CRC(aac24f34) SHA1(ad110e776547fb48baac568bb50d61854537ca34) )
	ROM_LOAD( "cv02_2.bin",   0x0400, 0x0400, CRC(2bdf83a0) SHA1(01ffbd43964c41987e7d44816271308f9a70802b) )
ROM_END

ROM_START( sisv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sv0h.bin",     0x0000, 0x0400, CRC(86bb8cb6) SHA1(a75648e7f2446c756d86624b15d387d25ce47b66) )
	ROM_LOAD( "sv02.bin",     0x0400, 0x0400, CRC(0e159534) SHA1(94b2015a9d38ca738705b8d024a79fd2f9855b98) )
	ROM_LOAD( "invaders.g",   0x0800, 0x0800, CRC(6bfaca4a) SHA1(16f48649b531bdef8c2d1446c429b5f414524350) )
	ROM_LOAD( "invaders.f",   0x1000, 0x0800, CRC(0ccead96) SHA1(537aef03468f63c5b9e11dd61e253f7ae17d9743) )
	ROM_LOAD( "tv04.m1",      0x1800, 0x0800, CRC(cd2c67f6) SHA1(60f9d8fe2d36ff589277b607f07c1edc917c755c) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "cv01_1.bin",   0x0000, 0x0400, CRC(aac24f34) SHA1(ad110e776547fb48baac568bb50d61854537ca34) )
	ROM_LOAD( "cv02_2.bin",   0x0400, 0x0400, CRC(2bdf83a0) SHA1(01ffbd43964c41987e7d44816271308f9a70802b) )
ROM_END

ROM_START( sisv2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sv0h.bin",     0x0000, 0x0400, CRC(86bb8cb6) SHA1(a75648e7f2446c756d86624b15d387d25ce47b66) )
	ROM_LOAD( "emag_si.b",    0x0400, 0x0400, CRC(febe6d1a) SHA1(e1c3a24b4fa5862107ada1f9d7249466e8c3f06a) )
	ROM_LOAD( "sv12",         0x0800, 0x0400, CRC(a08e7202) SHA1(de9f7c851d1b894915e720cfc5d794cdb31752f6) )
	ROM_LOAD( "invaders.f",   0x1000, 0x0800, CRC(0ccead96) SHA1(537aef03468f63c5b9e11dd61e253f7ae17d9743) )
	ROM_LOAD( "sv13",         0x1800, 0x0400, CRC(a9011634) SHA1(1f1369ecb02078042cfdf17a497b8dda6dd23793) )
	ROM_LOAD( "sv14",         0x1c00, 0x0400, CRC(58730370) SHA1(13dc806bcecd2d6089a85dd710ac2869413f7475) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "cv01_1.bin",   0x0000, 0x0400, CRC(aac24f34) SHA1(ad110e776547fb48baac568bb50d61854537ca34) )
	ROM_LOAD( "cv02_2.bin",   0x0400, 0x0400, CRC(2bdf83a0) SHA1(01ffbd43964c41987e7d44816271308f9a70802b) )
ROM_END

ROM_START( spceking )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "invaders.h",   0x0000, 0x0800, CRC(734f5ad8) SHA1(ff6200af4c9110d8181249cbcef1a8a40fa40b7f) )
	ROM_LOAD( "spcekng2",     0x0800, 0x0800, CRC(96dcdd42) SHA1(e18d7ffca92e863ef40e235b2be973d8c5879fdb) )
	ROM_LOAD( "spcekng3",     0x1000, 0x0800, CRC(95fc96ad) SHA1(38175edad0e538a1561cec8f7613f15ae274dd14) )
	ROM_LOAD( "spcekng4",     0x1800, 0x0800, CRC(54170ada) SHA1(1e8b3774355ec0d448f04805a917f4c1fe64bceb) )
ROM_END

ROM_START( spcewars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sanritsu.1",   0x0000, 0x0400, CRC(ca331679) SHA1(5c362c3d1c721d293bcddbef4033533769c8f0e0) )
	ROM_LOAD( "sanritsu.2",   0x0400, 0x0400, CRC(48dc791c) SHA1(91a98205c83ca38961e6ba2ac43a41e6e8bc2675) )
	ROM_LOAD( "ic35.bin",     0x0800, 0x0800, CRC(40c2d55b) SHA1(b641b63046d242ad23911143ed840011fc98eaff) )
	ROM_LOAD( "sanritsu.5",   0x1000, 0x0400, CRC(77475431) SHA1(15a04a2655847ee462be65d1065d643c872bb47c) )
	ROM_LOAD( "sanritsu.6",   0x1400, 0x0400, CRC(392ef82c) SHA1(77c98c11ee727ed3ed6e118f13d97aabdb555540) )
	ROM_LOAD( "sanritsu.7",   0x1800, 0x0400, CRC(b3a93df8) SHA1(3afc96814149d4d5343fe06eac09f808384d02c4) )
	ROM_LOAD( "sanritsu.8",   0x1c00, 0x0400, CRC(64fdc3e1) SHA1(c3c278bc236ced7fc85e1a9b018e80be6ab33402) )
	ROM_LOAD( "sanritsu.9",   0x4000, 0x0400, CRC(b2f29601) SHA1(ce855e312f50df7a74682974803cb4f9b2d184f3) )
ROM_END

ROM_START( spacewr3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ic36.bin",     0x0000, 0x0800, CRC(9e30f88a) SHA1(314dfb2920d9b43b977cc19e40ac315e6933c3b9) )
	ROM_LOAD( "ic35.bin",     0x0800, 0x0800, CRC(40c2d55b) SHA1(b641b63046d242ad23911143ed840011fc98eaff) )
	ROM_LOAD( "ic34.bin",     0x1000, 0x0800, CRC(b435f021) SHA1(2d0d813b99d571b53770fa878a1f82ca67827caa) )
	ROM_LOAD( "ic33.bin",     0x1800, 0x0800, CRC(cbdc6fe8) SHA1(63038ea09d320c54e3d1cf7f043c17bba71bf13c) )
	ROM_LOAD( "ic32.bin",     0x4000, 0x0800, CRC(1e5a753c) SHA1(5b7cd7b347203f4edf816f02c366bd3b1b9517c4) )
ROM_END

ROM_START( invaderl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "c01",          0x0000, 0x0400, CRC(499f253a) SHA1(e13353194277f5d35e92db9b11912b5f392f51b7) )
	ROM_LOAD( "c02",          0x0400, 0x0400, CRC(2d0b2e1f) SHA1(2e0262d9dba607824fcd720d2995531649bdd03d) )
	ROM_LOAD( "c03",          0x0800, 0x0400, CRC(03033dc2) SHA1(87d7838e6a6542c2c5510af593df45137cb397c6) )
	ROM_LOAD( "c07",          0x1000, 0x0400, CRC(5a7bbf1f) SHA1(659f2a8c646660d316d6e70f1d9548375f1da63f) )
	ROM_LOAD( "c04",          0x1400, 0x0400, CRC(455b1fa7) SHA1(668800a0a3ba18d8b54c2aa4dfd4bd01a667d679) )
	ROM_LOAD( "c05",          0x1800, 0x0400, CRC(40cbef75) SHA1(15994ed8bb8ab8faed6198926873851062c9d95f) )
	ROM_LOAD( "sv06.bin",     0x1c00, 0x0400, CRC(2c68e0b4) SHA1(a5e5357120102ad32792bf3ef6362f45b7ba7070) )
ROM_END

ROM_START( invader4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "spin4.a",      0x0000, 0x0800, CRC(bb386dfe) SHA1(cc00f3e4f6ca4c05bae038a24ccdb213fb951cfc) )
	ROM_LOAD( "spin4.b",      0x0800, 0x0800, CRC(63afa11d) SHA1(d8cedfa010a49237e31f6ebaed35134cb1c3ce68) )
	ROM_LOAD( "spin4.c",      0x1000, 0x0800, CRC(22b0317c) SHA1(8fd037bf5f89a7bcb06042697410566d5180912a) )
	ROM_LOAD( "spin4.d",      0x1800, 0x0800, CRC(9102fd68) SHA1(3523e69314844fcd1863b1e9a9d7fcebe9ee174b) )
ROM_END

ROM_START( jspecter )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "3305.u6",      0x0000, 0x1000, CRC(ab211a4f) SHA1(d675ed29c3479d7318f8559bd56dd619cf631b6a) )
	ROM_LOAD( "3306.u7",      0x1400, 0x1000, CRC(0df142a7) SHA1(2f1c32d6fe7eafb7808fef0bdeb69b4909427417) )
ROM_END

ROM_START( jspectr2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "unksi.b2",     0x0000, 0x1000, CRC(0584b6c4) SHA1(c130021b878bde2beda4a189f71bbfed61088535) )
	ROM_LOAD( "unksi.a2",     0x1400, 0x1000, CRC(58095955) SHA1(545df3bb9ee4ff09f491d7a4b704e31aa311a8d7) )
ROM_END

ROM_START( invadpt2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "pv01",        0x0000, 0x0800, CRC(7288a511) SHA1(ff617872784c28ed03591aefa9f0519e5651701f) )
	ROM_LOAD( "pv02",        0x0800, 0x0800, CRC(097dd8d5) SHA1(8d68654d54d075c0f0d7f63c87ff4551ce8b7fbf) )
	ROM_LOAD( "pv03",        0x1000, 0x0800, CRC(1766337e) SHA1(ea959bf06c9930d83a07559e191a28641efb07ac) )
	ROM_LOAD( "pv04",        0x1800, 0x0800, CRC(8f0e62e0) SHA1(a967b155f15f8432222fcc78b23121b00c405c5c) )
	ROM_LOAD( "pv05",        0x4000, 0x0800, CRC(19b505e9) SHA1(6a31a37586782ce421a7d2cffd8f958c00b7b415) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "pv06.1",   0x0000, 0x0400, CRC(a732810b) SHA1(a5fabffa73ca740909e23b9530936f9274dff356) )
	ROM_LOAD( "pv07.2",   0x0400, 0x0400, CRC(2c5b91cb) SHA1(7fa4d4aef85473b1b4f18734230c164e72be44e7) )
ROM_END

ROM_START( invaddlx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "invdelux.h",   0x0000, 0x0800, CRC(e690818f) SHA1(0860fb03a64d34a9704a1459a5e96929eafd39c7) )
	ROM_LOAD( "invdelux.g",   0x0800, 0x0800, CRC(4268c12d) SHA1(df02419f01cf0874afd1f1aa16276751acd0604a) )
	ROM_LOAD( "invdelux.f",   0x1000, 0x0800, CRC(f4aa1880) SHA1(995d77b67cb4f2f3781c2c8747cb058b7c1b3412) )
	ROM_LOAD( "invdelux.e",   0x1800, 0x0800, CRC(408849c1) SHA1(f717e81017047497a2e9f33f0aafecfec5a2ed7d) )
	ROM_LOAD( "invdelux.d",   0x4000, 0x0800, CRC(e8d5afcd) SHA1(91fde9a9e7c3dd53aac4770bd169721a79b41ed1) )
ROM_END

ROM_START( moonbase )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	   /* 64k for code */
	ROM_LOAD( "ze3-1.bin",    0x0000, 0x0400, CRC(82dbf2c7) SHA1(c767d8b866db4a5059bd79f962a90ce3a962e1e6) )
	ROM_LOAD( "ze3-2.bin",    0x0400, 0x0400, CRC(c867f5b4) SHA1(686318fda6edde297aecaf33f480bfa075fa6eca) )
	ROM_LOAD( "ze3-3.bin",    0x0800, 0x0400, CRC(cb23ccc1) SHA1(86be2d14d52b3404e1a25c573bd25b97729d82a1) )
	ROM_LOAD( "ze3-4.bin",    0x0c00, 0x0400, CRC(9a11abe2) SHA1(f5337183c7f279d75ddeeab24f4f132aa2ee103b) )
	ROM_LOAD( "ze3-5.bin",    0x1000, 0x0400, CRC(2b105ed3) SHA1(fa0767089b3aaec25be39e950e7163ecbdc2f39f) )
	ROM_LOAD( "ze3-6.bin",    0x1400, 0x0400, CRC(cb3d6dcb) SHA1(b4923b12a141c76b7d50274f19a3224db26a5669) )
	ROM_LOAD( "ze3-7.bin",    0x1800, 0x0400, CRC(774b52c9) SHA1(ddbbba874ac069fb930b364a890c45675ec389f7) )
	ROM_LOAD( "ze3-8.bin",    0x1c00, 0x0400, CRC(e88ea83b) SHA1(ef05be4783c860369ee5ecd4844837207e99ad9f) )
	ROM_LOAD( "ze3-9.bin",    0x4000, 0x0400, CRC(2dd5adfa) SHA1(62cb98cad1e48de0e0cbf30392d35834b38dadbd) )
	ROM_LOAD( "ze3-10.bin",   0x4400, 0x0400, CRC(1e7c22a4) SHA1(b34173375494ffbf5400dd4014a683a9807f4f08) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "n02prm.6a",    0x0000, 0x0400, CRC(2bdf83a0) SHA1(01ffbd43964c41987e7d44816271308f9a70802b) )
	ROM_LOAD( "n01prm.6b",    0x0400, 0x0400, CRC(aac24f34) SHA1(ad110e776547fb48baac568bb50d61854537ca34) )
ROM_END

ROM_START( invad2ct )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
    ROM_LOAD( "invad2ct.h",   0x0000, 0x0800, CRC(51d02a71) SHA1(2fa82ddc2702a72de0a9559ec244b70ab3db3f18) )
    ROM_LOAD( "invad2ct.g",   0x0800, 0x0800, CRC(533ac770) SHA1(edb65c289027432dad7861a7d6abbda9223c13b1) )
    ROM_LOAD( "invad2ct.f",   0x1000, 0x0800, CRC(d1799f39) SHA1(f7f1ba34d57f9883241ba3ef90e34ed20dfb8003) )
    ROM_LOAD( "invad2ct.e",   0x1800, 0x0800, CRC(291c1418) SHA1(0d9f7973ed81d28c43ef8b96f1180d6629871785) )
    ROM_LOAD( "invad2ct.b",   0x5000, 0x0800, CRC(8d9a07c4) SHA1(4acbe15185d958b5589508dc0ea3a615fbe3bcca) )
    ROM_LOAD( "invad2ct.a",   0x5800, 0x0800, CRC(efdabb03) SHA1(33f4cf249e88e2b7154350e54c479eb4fa86f26f) )
ROM_END

ROM_START( invrvnge )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "invrvnge.h",   0x0000, 0x0800, CRC(aca41bbb) SHA1(ca71f792abd6d9a44d15b19d2ccf678e82ccba4f) )
	ROM_LOAD( "invrvnge.g",   0x0800, 0x0800, CRC(cfe89dad) SHA1(218b6a0b636c49c4cdc3667e8b1387ef0e257115) )
	ROM_LOAD( "invrvnge.f",   0x1000, 0x0800, CRC(e350de2c) SHA1(e845565e2f96f9dec3242ec5ab75910a515428c9) )
	ROM_LOAD( "invrvnge.e",   0x1800, 0x0800, CRC(1ec8dfc8) SHA1(fc8fbe1161958f57c9f4ccbcab8a769184b1c562) )
ROM_END

ROM_START( invrvnga )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "5m.bin",       0x0000, 0x0800, CRC(b145cb71) SHA1(127eb11de7ab9835f06510fb12838c0b728c0d42) )
	ROM_LOAD( "5n.bin",       0x0800, 0x0800, CRC(660e8af3) SHA1(bd52eadf4ee3d717fd5bd7206e1e87d729250c92) )
	ROM_LOAD( "5p.bin",       0x1000, 0x0800, CRC(6ec5a9ad) SHA1(d1e84d2d60c6128c092f2cd20a2b87216df3034b) )
	ROM_LOAD( "5r.bin",       0x1800, 0x0800, CRC(74516811) SHA1(0f595c7b0fae5f3f83fdd1ffed5a408ee77c9438) )
ROM_END

ROM_START( spclaser )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "la01",         0x0000, 0x0800, CRC(bedc0078) SHA1(a5bb0cbbb8e3f27d03beb8101b2be1111d73689d) )
	ROM_LOAD( "spcewarl.2",   0x0800, 0x0800, CRC(43bc65c5) SHA1(5f9827c02c2d221e1607359c840374ff7fb92fbf) )
	ROM_LOAD( "la03",         0x1000, 0x0800, CRC(1083e9cc) SHA1(7ad45c6230c9e02fcf51e3414c15e2237eebbd7a) )
	ROM_LOAD( "la04",         0x1800, 0x0800, CRC(5116b234) SHA1(b165b2574cbcb26a5bb43f91df5f8be5f111f486) )
ROM_END

ROM_START( laser )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1.u36",        0x0000, 0x0800, CRC(b44e2c41) SHA1(00e0b2e088495d6f3bc175e8a53dcb3686ea8484) )
	ROM_LOAD( "2.u35",        0x0800, 0x0800, CRC(9876f331) SHA1(14e36b26d186d9a195492834ef989ed5664d7b65) )
	ROM_LOAD( "3.u34",        0x1000, 0x0800, CRC(ed79000b) SHA1(bfe0407e833ce61aa909f5f1f93c3fc1d46605e9) )
	ROM_LOAD( "4.u33",        0x1800, 0x0800, CRC(10a160a1) SHA1(e2d4208af11b65fc42d2856e57ee3c196f89d360) )
ROM_END

ROM_START( spcewarl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "spcewarl.1",   0x0000, 0x0800, CRC(1fcd34d2) SHA1(674139944e0d842a85bd21b326bd735e15453038) )
	ROM_LOAD( "spcewarl.2",   0x0800, 0x0800, CRC(43bc65c5) SHA1(5f9827c02c2d221e1607359c840374ff7fb92fbf) )
	ROM_LOAD( "spcewarl.3",   0x1000, 0x0800, CRC(7820df3a) SHA1(53315857f4282c68624b338b068d80ee6828af4c) )
	ROM_LOAD( "spcewarl.4",   0x1800, 0x0800, CRC(adc05b8d) SHA1(c4acf75537c0662a4785d5d6a90643239a54bf43) )
ROM_END

ROM_START( galxwars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "univgw3.0",    0x0000, 0x0400, CRC(937796f4) SHA1(88e9494cc532498e51e3a68fa1122c40f22b27dd) )
	ROM_LOAD( "univgw4.1",    0x0400, 0x0400, CRC(4b86e7a6) SHA1(167f9f7491a2de39d08e3e6f7057cc75b36c9340) )
	ROM_LOAD( "univgw5.2",    0x0800, 0x0400, CRC(47a187cd) SHA1(640c896ba25f34d323624005bd676257ad17b687) )
	ROM_LOAD( "univgw6.3",    0x0c00, 0x0400, CRC(7b7d22ff) SHA1(74364cf2b04dcfbbc8e0131fa12c0e574f693d34) )
	ROM_LOAD( "univgw1.4",    0x4000, 0x0400, CRC(0871156e) SHA1(3726d0bfe153a0afc62ea56737662074986064b0) )
	ROM_LOAD( "univgw2.5",    0x4400, 0x0400, CRC(6036d7bf) SHA1(36c2ad2ffdb47bbecc40fd67ced6ab51a5cd2f3e) )
ROM_END

ROM_START( galxwar2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "3192.h6",      0x0000, 0x1000, CRC(bde6860b) SHA1(e04b8add32d8f7ea588fae6d6a387f1d40495f1b) )
	ROM_LOAD( "3193.h7",      0x4000, 0x1000, CRC(a17cd507) SHA1(554ab0e8bdc0e7af4a30b0ddc8aa053c8e70255c) ) /* 2nd half unused */
ROM_END

ROM_START( galxwart )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "galxwars.0",   0x0000, 0x0400, CRC(608bfe7f) SHA1(a41a40a2f0a1bb61a70b9ff8a7da925ab1db7f74) )
	ROM_LOAD( "galxwars.1",   0x0400, 0x0400, CRC(a810b258) SHA1(030a72fffcf240f643bc3006028cb4883cf58bbc) )
	ROM_LOAD( "galxwars.2",   0x0800, 0x0400, CRC(74f31781) SHA1(1de70e8ebbb26eea20ffedb7bd0ca051a67f45e7) )
	ROM_LOAD( "galxwars.3",   0x0c00, 0x0400, CRC(c88f886c) SHA1(4d705fbb97e3868c3f6c90c5e5753ad17cfbf5d6) )
	ROM_LOAD( "galxwars.4",   0x4000, 0x0400, CRC(ae4fe8fb) SHA1(494f44167dc84e4515b769c12f6e24419461dce4) )
	ROM_LOAD( "galxwars.5",   0x4400, 0x0400, CRC(37708a35) SHA1(df6fd521ddfa146ef93e390e47741bdbfda1e7ba) )
ROM_END

ROM_START( starw )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "roma",         0x0000, 0x0400, CRC(60e8993c) SHA1(0bdf163ff0f2e6a8771987d4e7ac604c45af21b8) )
	ROM_LOAD( "romb",         0x0400, 0x0400, CRC(b8060773) SHA1(92aa358c338ef8f5773bccada8988d068764e7ea) )
	ROM_LOAD( "romc",         0x0800, 0x0400, CRC(307ce6b8) SHA1(f4b6f54db3d2377ec27d62d33fa1c4946559a092) )
	ROM_LOAD( "romd",         0x1400, 0x0400, CRC(2b0d0a88) SHA1(d079d12b6d4136519ded32415d668a02147b7601) )
	ROM_LOAD( "rome",         0x1800, 0x0400, CRC(5b1c3ad0) SHA1(edb42eec59c3dd7e274e2ea08fed0f3e8fc72e9e) )
	ROM_LOAD( "romf",         0x1c00, 0x0400, CRC(c8e42d3d) SHA1(841b27af251b9c3a964972e864fb7c88acc742e0) )
ROM_END

ROM_START( lrescue )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "lrescue.1",    0x0000, 0x0800, CRC(2bbc4778) SHA1(0167f1ac1501ab0b4c4e555023fa5efed59d56ae) )
	ROM_LOAD( "lrescue.2",    0x0800, 0x0800, CRC(49e79706) SHA1(bed675bb97d59ae0132c007ccead0d096ed2ddf1) )
	ROM_LOAD( "lrescue.3",    0x1000, 0x0800, CRC(1ac969be) SHA1(67ac47f45b9fa5c530bf6047bb7d5776b52847be) )
	ROM_LOAD( "lrescue.4",    0x1800, 0x0800, CRC(782fee3c) SHA1(668295e9d6d99084bb4e7c5491f00fe75f4f5a88) )
	ROM_LOAD( "lrescue.5",    0x4000, 0x0800, CRC(58fde8bc) SHA1(663665ac5254204c1eba18357d9867034eae55eb) )
	ROM_LOAD( "lrescue.6",    0x4800, 0x0800, CRC(bfb0f65d) SHA1(ea0943d764a16094b6e2289f62ef117c9f838c98) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color map */
	ROM_LOAD( "7643-1.cpu",   0x0000, 0x0400, CRC(8b2e38de) SHA1(d6a757be31c3a179d31bd3709e71f9e38ec632e9) )
	ROM_RELOAD(  			  0x0400, 0x0400 )
ROM_END

ROM_START( grescue )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "lrescue.1",    0x0000, 0x0800, CRC(2bbc4778) SHA1(0167f1ac1501ab0b4c4e555023fa5efed59d56ae) )
	ROM_LOAD( "lrescue.2",    0x0800, 0x0800, CRC(49e79706) SHA1(bed675bb97d59ae0132c007ccead0d096ed2ddf1) )
	ROM_LOAD( "lrescue.3",    0x1000, 0x0800, CRC(1ac969be) SHA1(67ac47f45b9fa5c530bf6047bb7d5776b52847be) )
	ROM_LOAD( "grescue.4",    0x1800, 0x0800, CRC(ca412991) SHA1(41b59f338a6c246e0942a8bfa3c0bca2c24c7f81) )
	ROM_LOAD( "grescue.5",    0x4000, 0x0800, CRC(a419a4d6) SHA1(8eeeb31cbebffc98d2c6c5b964f9b320fcf303d2) )
	ROM_LOAD( "lrescue.6",    0x4800, 0x0800, CRC(bfb0f65d) SHA1(ea0943d764a16094b6e2289f62ef117c9f838c98) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color map */
	ROM_LOAD( "7643-1.cpu",   0x0000, 0x0400, CRC(8b2e38de) SHA1(d6a757be31c3a179d31bd3709e71f9e38ec632e9) )
	ROM_RELOAD(  			  0x0400, 0x0400 )
ROM_END

ROM_START( desterth )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "36_h.bin",     0x0000, 0x0800, CRC(f86923e5) SHA1(d19935ba3d2c1c2553b3779f1a7ad8856c003dae) )
	ROM_LOAD( "35_g.bin",     0x0800, 0x0800, CRC(797f440d) SHA1(a96917f2296ae467acc795eacc1533a2a2d2f401) )
	ROM_LOAD( "34_f.bin",     0x1000, 0x0800, CRC(993d0846) SHA1(6be0c45add41fa7e43cac96c776cd0ebb45ade7b) )
	ROM_LOAD( "33_e.bin",     0x1800, 0x0800, CRC(8d155fc5) SHA1(1ef5e62d71abbf870c027fa1e477121ff124b8da) )
	ROM_LOAD( "32_d.bin",     0x4000, 0x0800, CRC(3f531b6f) SHA1(2fc1f4912688986650e20a050a5d63ddecd4267e) )
	ROM_LOAD( "31_c.bin",     0x4800, 0x0800, CRC(ab019c30) SHA1(33931510a722168bcf7c30d22eac9345576b6631) )
	ROM_LOAD( "42_b.bin",     0x5000, 0x0800, CRC(ed9dbac6) SHA1(4553f445ac32ebb1be490b02df4924f76557e8f9) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color map */
	ROM_LOAD( "7643-1.cpu",   0x0000, 0x0400, CRC(8b2e38de) SHA1(d6a757be31c3a179d31bd3709e71f9e38ec632e9) )
	ROM_RELOAD(  			  0x0400, 0x0400 )
ROM_END

ROM_START( cosmo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1.36",         0x0000, 0x0800, CRC(445c9a98) SHA1(89bce80a061e9c12544231f970d9dec801eb1b94) )
	ROM_LOAD( "2.35",         0x0800, 0x0800, CRC(df3eb731) SHA1(fb90c1d0f2518195dd49062c9f0fd890536d89f4) )
	ROM_LOAD( "3.34",         0x1000, 0x0800, CRC(772c813f) SHA1(a1c0d857c660fb0b838dd0466af7bf5d73bcd55d) )
	ROM_LOAD( "4.33",         0x1800, 0x0800, CRC(279f66e6) SHA1(8ce71c08cca0bdde2f2e0ef21622731c4610c030) )
	ROM_LOAD( "5.32",         0x4000, 0x0800, CRC(cefb18df) SHA1(bb500cf3f7d1a54045a165d3613a92ab3f11d3e8) )
	ROM_LOAD( "6.31",         0x4800, 0x0800, CRC(b037f6c4) SHA1(b9a42948052b8cda8d2e4575e59909589f4e7a8d) )
	ROM_LOAD( "7.42",         0x5000, 0x0800, CRC(c3831ea2) SHA1(8c67ef0312656ef0eeff34b8463376c736bd8ea1) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )		/* color map */
	ROM_LOAD( "n-1.7d",       0x0800, 0x0800, CRC(bd8576f1) SHA1(aa5fe0a4d024f21a3bca7a6b3f5022779af6f3f4) )
	ROM_LOAD( "n-2.6e",       0x0000, 0x0800, CRC(48f1ade5) SHA1(a1b45f82f3649cde8ae6a2ef494a3a6cdb5e65d0) )
ROM_END

ROM_START( cosmicmo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cosmicmo.1",   0x0000, 0x0400, CRC(d6e4e5da) SHA1(8b4275a3c71ac3fa80d17237dc04de5f586645f4) )
	ROM_LOAD( "cosmicmo.2",   0x0400, 0x0400, CRC(8f7988e6) SHA1(b6a01d5dcab013350f8f7f3e3ebfc986bb939fe0) )
	ROM_LOAD( "cosmicmo.3",   0x0800, 0x0400, CRC(2d2e9dc8) SHA1(dd3da4fc752e003e5e7c64bf189288133aed545b) )
	ROM_LOAD( "cosmicmo.4",   0x0c00, 0x0400, CRC(26cae456) SHA1(2f2262340c10e5c29d71317f6eb8072c26655563) )
	ROM_LOAD( "cosmicmo.5",   0x4000, 0x0400, CRC(b13f228e) SHA1(a0de05aa36435e72c77f5333f3ad964ec448a8f0) )
	ROM_LOAD( "cosmicmo.6",   0x4400, 0x0400, CRC(4ae1b9c4) SHA1(8eed87eebe68caa775fa679363b0fe3728d98c34) )
	ROM_LOAD( "cosmicmo.7",   0x4800, 0x0400, CRC(6a13b15b) SHA1(dc03a6c3e938cfd08d16bd1660899f951ba72ea2) )
ROM_END

ROM_START( cosmicm2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "3907.bin",   0x0000, 0x1000, CRC(bbffede6) SHA1(e7505ee8e3f19557ebbfd0145dc2ae0d1c529eba) )
	ROM_LOAD( "3906.bin",   0x4000, 0x1000, CRC(b841f894) SHA1(b1f9e1800969baab14da2fd8873b58d4707b7236) )
ROM_END

ROM_START( superinv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )             /* 64k for code */
	ROM_LOAD( "00",           0x0000, 0x0400, CRC(7a9b4485) SHA1(dde918ec106971972bf7c7e5085c1262522f7e35) )
	ROM_LOAD( "01",           0x0400, 0x0400, CRC(7c86620d) SHA1(9e92ec0aa4eee96a7fa115a14a611c488d13b9dd) )
	ROM_LOAD( "02",           0x0800, 0x0400, CRC(ccaf38f6) SHA1(8eb0456e8abdba0d1dda20a335a9ecbe7c38f9ed) )
	ROM_LOAD( "03",           0x1400, 0x0400, CRC(8ec9eae2) SHA1(48d7a7dc61e0417ca4093e5c2a36efd96e359233) )
	ROM_LOAD( "04",           0x1800, 0x0400, CRC(68719b30) SHA1(2084bd63cd61ef1d2497c32112cdb42b7b582da4) )
	ROM_LOAD( "05",           0x1c00, 0x0400, CRC(8abe2466) SHA1(17494b1e5db207e37a7d28d7c89cbc5f36b7aefc) )
ROM_END

ROM_START( invasion )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )             /* 64k for code */
	ROM_LOAD( "10136-0.0k",   0x0000, 0x0400, CRC(7a9b4485) SHA1(dde918ec106971972bf7c7e5085c1262522f7e35) )
	ROM_LOAD( "10136-1.1k",   0x0400, 0x0400, CRC(7c86620d) SHA1(9e92ec0aa4eee96a7fa115a14a611c488d13b9dd) )
	ROM_LOAD( "10136-2.2k",   0x0800, 0x0400, CRC(ccaf38f6) SHA1(8eb0456e8abdba0d1dda20a335a9ecbe7c38f9ed) )
	ROM_LOAD( "10136-5.5k",   0x1400, 0x0400, CRC(8ec9eae2) SHA1(48d7a7dc61e0417ca4093e5c2a36efd96e359233) )
	ROM_LOAD( "10136-6.6k",   0x1800, 0x0400, CRC(ff0b0690) SHA1(8547c4b2a228f1690287217a916613c8f0caccf6) )
	ROM_LOAD( "10136-7.7k",   0x1c00, 0x0400, CRC(75d7acaf) SHA1(977d146d7df555cea1bb2156d29d88bec9731f98) )
ROM_END

ROM_START( rollingc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "rc01.bin",     0x0000, 0x0400, CRC(66fa50bf) SHA1(7451d4ff8d3b351a324aaecdbdc5b46672f5fdd0) )
	ROM_LOAD( "rc02.bin",     0x0400, 0x0400, CRC(61c06ae4) SHA1(7685c806e20e4a4a0508a547ac08ca8f6d75bb79) )
	ROM_LOAD( "rc03.bin",     0x0800, 0x0400, CRC(77e39fa0) SHA1(16bf88af1b97c5a2a81e105af08b8d9d1f10dcc8) )
	ROM_LOAD( "rc04.bin",     0x0c00, 0x0400, CRC(3fdfd0f3) SHA1(4c5e7136a766f3f16399e61eaaa0e00ef6b619f7) )
	ROM_LOAD( "rc05.bin",     0x1000, 0x0400, CRC(c26a8f5b) SHA1(f7a541999cfe04c6d6927d285484f0f81857e04a) )
	ROM_LOAD( "rc06.bin",     0x1400, 0x0400, CRC(0b98dbe5) SHA1(33cedab82ddccb4caaf681fce553b5230a8d6f92) )
	ROM_LOAD( "rc07.bin",     0x1800, 0x0400, CRC(6242145c) SHA1(b01bb02835dda89dc02604ec52e423167183e8c9) )
	ROM_LOAD( "rc08.bin",     0x1c00, 0x0400, CRC(d23c2ef1) SHA1(909e3d53291dbd219f4f9e0047c65317b9f6d5bd) )

	ROM_LOAD( "rc09.bin",     0x4000, 0x0800, CRC(2e2c5b95) SHA1(33f4e2789d67e355ccd99d2c0d07301ec2bd3bc1) )
	ROM_LOAD( "rc10.bin",     0x4800, 0x0800, CRC(ef94c502) SHA1(07c0504b2ebce0fa6e53e6957e7b6c0e9caab430) )
	ROM_LOAD( "rc11.bin",     0x5000, 0x0800, CRC(a3164b18) SHA1(7270af25fa4171f86476f5dc409e658da7fba7fc) )
	ROM_LOAD( "rc12.bin",     0x5800, 0x0800, CRC(2052f6d9) SHA1(036702fc40cf133eb374ed674695d7c6c79e8311) )
ROM_END

ROM_START( boothill )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "romh.cpu",     0x0000, 0x0800, CRC(1615d077) SHA1(e59a26c2f2fc67ab24301e22d2e3f33043acdf72) )
	ROM_LOAD( "romg.cpu",     0x0800, 0x0800, CRC(65a90420) SHA1(9f36c44b5ae5b912cdbbeb9ff11a42221b8362d2) )
	ROM_LOAD( "romf.cpu",     0x1000, 0x0800, CRC(3fdafd79) SHA1(b18e8ac9df40c4687ac1acd5174eb99f2ef60081) )
	ROM_LOAD( "rome.cpu",     0x1800, 0x0800, CRC(374529f4) SHA1(18c57b79df0c66052eef40a694779a5ade15d0e0) )
ROM_END

ROM_START( schaser )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "rt13.bin",     0x0000, 0x0400, CRC(0dfbde68) SHA1(7367b138ad8448aba9222fed632a892df65cecbd) )
	ROM_LOAD( "rt14.bin",     0x0400, 0x0400, CRC(5a508a25) SHA1(c681d0bbf49317e79b596fb094e66b8912f0e409) )
	ROM_LOAD( "rt15.bin",     0x0800, 0x0400, CRC(2ac43a93) SHA1(d364f0940681a888c0147e06bcb01f8a0d4a24c8) )
	ROM_LOAD( "rt16.bin",     0x0c00, 0x0400, CRC(f5583afc) SHA1(5e8edb43ccb138fd47ac8f3da1af79b4444a4a82) )
	ROM_LOAD( "rt17.bin",     0x1000, 0x0400, CRC(51cf1155) SHA1(fd8c82d951602fd7e0ada65fc7cdee9f277c70db) )
	ROM_LOAD( "rt18.bin",     0x1400, 0x0400, CRC(3f0fc73a) SHA1(b801c3f1e8e6e41c564432db7c5891f6b27293b2) )
	ROM_LOAD( "rt19.bin",     0x1800, 0x0400, CRC(b66ea369) SHA1(d277f572f9c7c4301518546cf60671a6539326ee) )
	ROM_LOAD( "rt20.bin",     0x1c00, 0x0400, CRC(e3a7466a) SHA1(2378970f38b0cec066ef853a6540500e468e4ab4) )
	ROM_LOAD( "rt21.bin",     0x4000, 0x0400, CRC(b368ac98) SHA1(6860efe0496955db67611183be0efecda92c9c98) )
	ROM_LOAD( "rt22.bin",     0x4400, 0x0400, CRC(6e060dfb) SHA1(614e2ecf676c3ea2f9ea869125cfffef2f713684) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* background color map */
	ROM_LOAD( "rt06.ic2",     0x0000, 0x0400, CRC(950cf973) SHA1(d22df09b325835a0057ccd0d54f827b374254ac6) )
ROM_END

ROM_START( sflush )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "taitofr.005",        0xd800, 0x800, CRC(c4f08f9f) SHA1(997f216f5244942fc1a19f5c1988adbfadc301fc) )
	ROM_LOAD( "taitofr.004",        0xe000, 0x800, CRC(87a754a5) SHA1(07c0e2c3cb7aa0086d8f4dd202a452bc6c20d4ee) )
	ROM_LOAD( "taitofr.003",        0xe800, 0x800, CRC(5b12847f) SHA1(4b62342723dd49a387fae6637c331d7c853712a3) )
	ROM_LOAD( "taitofr.002",        0xf000, 0x800, CRC(291c9b1f) SHA1(7e5b3e1605581abf3d8165f4de9d4e32a5ee3bb0) )
	ROM_LOAD( "taitofr.001",        0xf800, 0x800, CRC(55d688c6) SHA1(574a3a2ca73cabb4b8f3444aa4464e6d64daa3ad) )
ROM_END

ROM_START( schasrcv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1",     		  0x0000, 0x0400, CRC(bec2b16b) SHA1(c62210ecb64d7c38e5b63481d7fe04eb59bb1068) )
	ROM_LOAD( "2",     		  0x0400, 0x0400, CRC(9d25e608) SHA1(4cc52a93a3ab96a0ec1d07593e17832fa59b30a1) )
	ROM_LOAD( "3",     		  0x0800, 0x0400, CRC(113d0635) SHA1(ab5e98d0b5fc37d7d69bb5c541681a0f66460440) )
	ROM_LOAD( "4",     		  0x0c00, 0x0400, CRC(f3a43c8d) SHA1(29a7a8b7d1de763a255cfec79157fd95e7bff551) )
	ROM_LOAD( "5",     		  0x1000, 0x0400, CRC(47c84f23) SHA1(61b475fa92b8335f8edd3a128d8ac8561658e464) )
	ROM_LOAD( "6",     		  0x1400, 0x0400, CRC(02ff2199) SHA1(e12c235b2064cb4bb426145172e523256e3c6358) )
	ROM_LOAD( "7",     		  0x1800, 0x0400, CRC(87d06b88) SHA1(2d743161f85e47cb8ee2a600cbee790b1ad7ad99) )
	ROM_LOAD( "8",     		  0x1c00, 0x0400, CRC(6dfaad08) SHA1(2184c4e2f4b6bffdc4fe13e178134331fcd43253) )
	ROM_LOAD( "9",     		  0x4000, 0x0400, CRC(3d1a2ae3) SHA1(672ad6590aebdfebc2748455fa638107f3934c41) )
	ROM_LOAD( "10",    		  0x4400, 0x0400, CRC(037edb99) SHA1(f2fc5e61f962666e7f6bb81753ac24ea0b97e581) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 (not used, but they were on the board) */
	ROM_LOAD( "cv01",         0x0000, 0x0400, CRC(037e16ac) SHA1(d585030aaff428330c91ae94d7cd5c96ebdd67dd) )
	ROM_LOAD( "cv02",         0x0400, 0x0400, CRC(8263da38) SHA1(2e7c769d129e6f8a1a31eba1e02777bb94ac32b2) )
ROM_END

ROM_START( spcenctr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "4m33.cpu",     0x0000, 0x0800, CRC(7458b2db) SHA1(c4f41efb8a35fd8bebc75bff0111476affe2b34d) )
	ROM_LOAD( "4m32.cpu",     0x0800, 0x0800, CRC(1b873788) SHA1(6cdf0d602a65c7efcf8abe149c6172b4c7ab87a1) )
	ROM_LOAD( "4m31.cpu",     0x1000, 0x0800, CRC(d4319c91) SHA1(30830595c220f490fe150ad018fbf4671bb71e02) )
	ROM_LOAD( "4m30.cpu",     0x1800, 0x0800, CRC(9b9a1a45) SHA1(8023a05c13e8b541f9e2fe4d389e6a2dcd4766ea) )
	ROM_LOAD( "4m29.cpu",     0x4000, 0x0800, CRC(294d52ce) SHA1(0ee63413c5caf60d45ae8bef08f6c07099d30f79) )
	ROM_LOAD( "4m28.cpu",     0x4800, 0x0800, CRC(ce44c923) SHA1(9d35908de3194c5fe6fc8495ae413fa722018744) )
	ROM_LOAD( "4m27.cpu",     0x5000, 0x0800, CRC(098070ab) SHA1(72ae344591df0174353dc2e3d22daf5a70e2261f) )
	ROM_LOAD( "4m26.cpu",     0x5800, 0x0800, CRC(7f1d1f44) SHA1(2f4951171a55e7ac072742fa24eceeee6aca7e39) )
ROM_END

ROM_START( clowns )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "h2.cpu",       0x0000, 0x0400, CRC(ff4432eb) SHA1(997aee1e3669daa1d8169b4e103d04baaab8ea8d) )
	ROM_LOAD( "g2.cpu",       0x0400, 0x0400, CRC(676c934b) SHA1(72b681ca9ef23d820fdd297cc417932aecc9677b) )
	ROM_LOAD( "f2.cpu",       0x0800, 0x0400, CRC(00757962) SHA1(ef39211493393e97284a08eea63be0757643ac88) )
	ROM_LOAD( "e2.cpu",       0x0c00, 0x0400, CRC(9e506a36) SHA1(8aad486a72d148d8b03e7bec4c12abd14e425c5f) )
	ROM_LOAD( "d2.cpu",       0x1000, 0x0400, CRC(d61b5b47) SHA1(6051c0a2e81d6e975e82c2d48d0e52dc0d4723e3) )
	ROM_LOAD( "c2.cpu",       0x1400, 0x0400, CRC(154d129a) SHA1(61eebb319ee3a6be598b764b295c18a93a953c1e) )
ROM_END

ROM_START( clowns1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "clownsv1.h",   0x0000, 0x0400, CRC(5560c951) SHA1(b6972e1918604263579de577ec58fa6a91e8ff3e) )
	ROM_LOAD( "clownsv1.g",   0x0400, 0x0400, CRC(6a571d66) SHA1(e825f95863e901a1b648c74bb47098c8e74f179b) )
	ROM_LOAD( "clownsv1.f",   0x0800, 0x0400, CRC(a2d56cea) SHA1(61bc07e6a24a1980216453b4dd2688695193a4ae) )
	ROM_LOAD( "clownsv1.e",   0x0c00, 0x0400, CRC(bbd606f6) SHA1(1cbaa21d9834c8d76cf335fd118851591e815c86) )
	ROM_LOAD( "clownsv1.d",   0x1000, 0x0400, CRC(37b6ff0e) SHA1(bf83bebb6c14b3663ca86a180f9ae3cddb84e571) )
	ROM_LOAD( "clownsv1.c",   0x1400, 0x0400, CRC(12968e52) SHA1(71e4f09d30b992a4ac44b0e88e83b4f8a0f63caa) )
ROM_END

ROM_START( gmissile )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "gm_623.h",     0x0000, 0x0800, CRC(a3ebb792) SHA1(30d9613de849c1a868056c5e28cf2a8608b63e88) )
	ROM_LOAD( "gm_623.g",     0x0800, 0x0800, CRC(a5e740bb) SHA1(963c0984953eb58fe7eab84fabb724ec6e29e706) )
	ROM_LOAD( "gm_623.f",     0x1000, 0x0800, CRC(da381025) SHA1(c9d0511567ed571b424459896ce7de0326850388) )
	ROM_LOAD( "gm_623.e",     0x1800, 0x0800, CRC(f350146b) SHA1(a07000a979b1a735754eca623cc880988924877f) )
ROM_END

ROM_START( seawolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sw0041.h",     0x0000, 0x0400, CRC(8f597323) SHA1(b538277d3a633dd8a3179cff202f18d322e6fe17) )
	ROM_LOAD( "sw0042.g",     0x0400, 0x0400, CRC(db980974) SHA1(cc2a99b18695f61e0540c9f6bf8fe3b391dde4a0) )
	ROM_LOAD( "sw0043.f",     0x0800, 0x0400, CRC(e6ffa008) SHA1(385198434b08fe4651ad2c920d44fb49cfe0bc33) )
	ROM_LOAD( "sw0044.e",     0x0c00, 0x0400, CRC(c3557d6a) SHA1(bd345dd72fed8ce15da76c381782b025f71b006f) )
ROM_END

ROM_START( gunfight )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "7609h.bin",    0x0000, 0x0400, CRC(0b117d73) SHA1(99d01313e251818d336281700e206d9003c71dae) )
	ROM_LOAD( "7609g.bin",    0x0400, 0x0400, CRC(57bc3159) SHA1(c177e3f72db9af17ab99b2481448ca26318184b9) )
	ROM_LOAD( "7609f.bin",    0x0800, 0x0400, CRC(8049a6bd) SHA1(215b068663e431582591001cbe028929fa96d49f) )
	ROM_LOAD( "7609e.bin",    0x0c00, 0x0400, CRC(773264e2) SHA1(de3f2e6841122bbe6e2fda5b87d37842c072289a) )
ROM_END

ROM_START( 280zzzap )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "zzzaph",       0x0000, 0x0400, CRC(1fa86e1c) SHA1(b9cf16eb037ada73631ed24297e9e3b3bf6ab3cd) )
	ROM_LOAD( "zzzapg",       0x0400, 0x0400, CRC(9639bc6b) SHA1(b2e2497e421e79a411d07ebf2eed2bb8dc227003) )
	ROM_LOAD( "zzzapf",       0x0800, 0x0400, CRC(adc6ede1) SHA1(206bf2575696c4b14437f3db37a215ba33211943) )
	ROM_LOAD( "zzzape",       0x0c00, 0x0400, CRC(472493d6) SHA1(ae5cf4481ee4b78ca0d2f4d560d295e922aa04a7) )
	ROM_LOAD( "zzzapd",       0x1000, 0x0400, CRC(4c240ee1) SHA1(972475f80253bb0d24773a10aec26a12f28e7c23) )
	ROM_LOAD( "zzzapc",       0x1400, 0x0400, CRC(6e85aeaf) SHA1(ffa6bb84ef1f7c2d72fd26c24bd33aa014aeab7e) )
ROM_END

ROM_START( lupin3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "lp12.bin",     0x0000, 0x0800, CRC(68a7f47a) SHA1(dce99b3810331d7603fa468f1dea984e571f709b) )
	ROM_LOAD( "lp13.bin",     0x0800, 0x0800, CRC(cae9a17b) SHA1(a333ba7db45325996e3254ab36162bb7577e8a38) )
	ROM_LOAD( "lp14.bin",     0x1000, 0x0800, CRC(3553b9e4) SHA1(6affb5b6caf08f365c0dce669e44046295c3df91) )
	ROM_LOAD( "lp15.bin",     0x1800, 0x0800, CRC(acbeef64) SHA1(50d78cdc9938285b6bf9fa81fa0f6c30b23e0756) )
	ROM_LOAD( "lp16.bin",     0x4000, 0x0800, CRC(19fcdc54) SHA1(2f18ee8158321fff68886ffe793724001e8b18c2) )
	ROM_LOAD( "lp17.bin",     0x4800, 0x0800, CRC(66289ab2) SHA1(fc9b4a7b7a08d43f34beaf1a8e68ed0ff6148534) )
	ROM_LOAD( "lp18.bin",     0x5000, 0x0800, CRC(2f07b4ba) SHA1(982e4c437b39b45e23d15af1b2fc8c7aa3034559) )
ROM_END

ROM_START( polaris )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ps-01",        0x0000, 0x0800, CRC(c04ce5a9) SHA1(62cc9b3b682ebecfb7600393862c65e26ff5263f) )
	ROM_LOAD( "ps-09",        0x0800, 0x0800, CRC(9a5c8cb2) SHA1(7a8c5d74f8b431072d9476d3ef65a3fe1d639813) )
	ROM_LOAD( "ps-08",        0x1000, 0x0800, CRC(8680d7ea) SHA1(7fd4b8a415666c36842fed80d2798b48f8b29d0d) )
	ROM_LOAD( "ps-04",        0x1800, 0x0800, CRC(65694948) SHA1(de92a7f3e3ef732b573254baa60df60f8e068a5d) )
	ROM_LOAD( "ps-05",        0x4000, 0x0800, CRC(772e31f3) SHA1(fa0b866b6df1a9217e286ca880b3bb3fb0644bf3) )
	ROM_LOAD( "ps-10",        0x4800, 0x0800, CRC(3df77bac) SHA1(b3275c34b8d42df83df2c404c5b7d220aae651fa) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* background color map */
	ROM_LOAD( "ps07",         0x0000, 0x0400, CRC(164aa05d) SHA1(41c699ce45c76a60c71294f25d8df6c6e6c1280a) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )		/* cloud graphics */
	ROM_LOAD( "mb7052.2c",    0x0000, 0x0100, CRC(2953253b) SHA1(2fb851bc9652ca4e51d473b484ede6dab05f1b51) )
ROM_END

ROM_START( polarisa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ps01-1",       0x0000, 0x0800, CRC(7d41007c) SHA1(168f002fe997aac6e4141292de826d389859bb04) )
	ROM_LOAD( "ps-09",        0x0800, 0x0800, CRC(9a5c8cb2) SHA1(7a8c5d74f8b431072d9476d3ef65a3fe1d639813) )
	ROM_LOAD( "ps03-1",       0x1000, 0x0800, CRC(21f32415) SHA1(6ac9ae9b55e342729fe260147021ed3911a24dc2) )
	ROM_LOAD( "ps-04",        0x1800, 0x0800, CRC(65694948) SHA1(de92a7f3e3ef732b573254baa60df60f8e068a5d) )
	ROM_LOAD( "ps-05",        0x4000, 0x0800, CRC(772e31f3) SHA1(fa0b866b6df1a9217e286ca880b3bb3fb0644bf3) )
	ROM_LOAD( "ps-10",        0x4800, 0x0800, CRC(3df77bac) SHA1(b3275c34b8d42df83df2c404c5b7d220aae651fa) )
	ROM_LOAD( "ps26",         0x5000, 0x0800, CRC(9d5c3d50) SHA1(a6acf9ca6e807625156cb1759269014d5830a44f) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* background color map */
	ROM_LOAD( "ps07",         0x0000, 0x0400, CRC(164aa05d) SHA1(41c699ce45c76a60c71294f25d8df6c6e6c1280a) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )		/* cloud graphics */
	ROM_LOAD( "mb7052.2c",    0x0000, 0x0100, CRC(2953253b) SHA1(2fb851bc9652ca4e51d473b484ede6dab05f1b51) )
ROM_END

ROM_START( lagunar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "lagunar.h",    0x0000, 0x0800, CRC(0cd5a280) SHA1(89a744c912070f11b0b90b0cc92061e238b00b64) )
	ROM_LOAD( "lagunar.g",    0x0800, 0x0800, CRC(824cd6f5) SHA1(a74f6983787cf040eab6f19de2669c019962b9cb) )
	ROM_LOAD( "lagunar.f",    0x1000, 0x0800, CRC(62692ca7) SHA1(d62051bd1b45ca6e60df83942ff26a64ae25a97b) )
	ROM_LOAD( "lagunar.e",    0x1800, 0x0800, CRC(20e098ed) SHA1(e0c52c013f5e93794b363d7762ce0f34ba98c660) )
ROM_END

ROM_START( m4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "m4.h",         0x0000, 0x0800, CRC(9ee2a0b5) SHA1(b81b4001c90ac6db25edd838652c42913022d9a9) )
	ROM_LOAD( "m4.g",         0x0800, 0x0800, CRC(0e84b9cb) SHA1(a7b74851979aaaa16496e506c487a18df14ab6dc) )
	ROM_LOAD( "m4.f",         0x1000, 0x0800, CRC(9ded9956) SHA1(449204a50efd3345cde815ca5f1fb596843a30ac) )
	ROM_LOAD( "m4.e",         0x1800, 0x0800, CRC(b6983238) SHA1(3f3b99b33135e144c111d2ebaac8f9433c269bc5) )
ROM_END

ROM_START( phantom2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "phantom2.h",   0x0000, 0x0800, CRC(0e3c2439) SHA1(450182e590845c651530b2c84e1f11fe2451dcf6) )
	ROM_LOAD( "phantom2.g",   0x0800, 0x0800, CRC(e8df3e52) SHA1(833925e44e686df4d4056bce4c0ffae3269d57df) )
	ROM_LOAD( "phantom2.f",   0x1000, 0x0800, CRC(30e83c6d) SHA1(fe34a3e4519a7e5ffe66e76fe974049988656b71) )
	ROM_LOAD( "phantom2.e",   0x1800, 0x0800, CRC(8c641cac) SHA1(c4986daacb7ed9efed59b022c6101240b0eddcdc) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )	   /* cloud graphics */
	ROM_LOAD( "p2clouds",     0x0000, 0x0800, CRC(dcdd2927) SHA1(d8d42c6594e36c12b40ee6342a9ad01a8bbdef75) )
ROM_END

ROM_START( dogpatch )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "dogpatch.h",   0x0000, 0x0800, CRC(74ebdf4d) SHA1(6b31f9563b0f79fe9128ee83e85a3e2f90d7985b) )
	ROM_LOAD( "dogpatch.g",   0x0800, 0x0800, CRC(ac246f70) SHA1(7ee356c3218558a78ee0ff495f9f51ef88cac951) )
	ROM_LOAD( "dogpatch.f",   0x1000, 0x0800, CRC(a975b011) SHA1(fb807d9eefde7177d7fd7ab06fc2dbdc58ae6fcb) )
	ROM_LOAD( "dogpatch.e",   0x1800, 0x0800, CRC(c12b1f60) SHA1(f0504e16d2ce60a0fb3fc2af8c323bfca0143818) )
ROM_END

ROM_START( bowler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "h.cpu",        0x0000, 0x0800, CRC(74c29b93) SHA1(9cbd5b7b8a4c889406b6bc065360f74c036320b2) )
	ROM_LOAD( "g.cpu",        0x0800, 0x0800, CRC(ca26d8b4) SHA1(cf18991cde8044a961cf556f18c6eb60a7ade595) )
	ROM_LOAD( "f.cpu",        0x1000, 0x0800, CRC(ba8a0bfa) SHA1(bb017ddac58d031b249596b70ab1068cd1bad499) )
	ROM_LOAD( "e.cpu",        0x1800, 0x0800, CRC(4da65a40) SHA1(7795d59870fa722da89888e72152145662554080) )
	ROM_LOAD( "d.cpu",        0x4000, 0x0800, CRC(e7dbc9d9) SHA1(05049a69ee588de85db86df188e7670778b77e90) )
ROM_END

ROM_START( shuffle )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "shuffle.h",    0x0000, 0x0800, CRC(0d422a18) SHA1(909c5b9e3c1194abd101cbf993a2ed7c8fbeb5d0) )
	ROM_LOAD( "shuffle.g",    0x0800, 0x0800, CRC(7db7fcf9) SHA1(f41b568f2340e5307a7a45658946cfd4cf4056bf) )
	ROM_LOAD( "shuffle.f",    0x1000, 0x0800, CRC(cd04d848) SHA1(f0f7e9bc483f08934d5c29568b4a7fe084623031) )
	ROM_LOAD( "shuffle.e",    0x1800, 0x0800, CRC(2c118357) SHA1(178db02aaa70963dd8dbcb9b8651209913c539af) )
ROM_END

ROM_START( blueshrk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "blueshrk.h",   0x0000, 0x0800, CRC(4ff94187) SHA1(7cb80e2ccc34983bfd688c549ffc032d6dacf880) )
	ROM_LOAD( "blueshrk.g",   0x0800, 0x0800, CRC(e49368fd) SHA1(2495ba48532bb714361e4f0e94c9317161c6c77f) )
	ROM_LOAD( "blueshrk.f",   0x1000, 0x0800, CRC(86cca79d) SHA1(7b4633fb8033ee2c0e692135c383ebf57deef0e5) )
ROM_END

ROM_START( einnings )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ei.h",         0x0000, 0x0800, CRC(eff9c7af) SHA1(316fffc972bd9935ead5ee4fd629bddc8a8ed5ce) )
	ROM_LOAD( "ei.g",         0x0800, 0x0800, CRC(5d1e66cb) SHA1(a5475362e12b7c251a05d67c2fd070cf7d333ad0) )
	ROM_LOAD( "ei.f",         0x1000, 0x0800, CRC(ed96785d) SHA1(d5557620227fcf6f30dcf6c8f5edd760d77d30ae) )
	ROM_LOAD( "ei.e",         0x1800, 0x0800, CRC(ad096a5d) SHA1(81d48302a0e039b8601a6aed7276e966592af693) )
	ROM_LOAD( "ei.b",         0x5000, 0x0800, CRC(56b407d4) SHA1(95e4be5b2f28192df85c6118079de2e68838b67c) )
ROM_END

ROM_START( dplay )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "dplay619.h",   0x0000, 0x0800, CRC(6680669b) SHA1(49ad2333f81613c2f27231de60b415cbc254546a) )
	ROM_LOAD( "dplay619.g",   0x0800, 0x0800, CRC(0eec7e01) SHA1(2661e77061119d7d95d498807bd29d2630c6b6ab) )
	ROM_LOAD( "dplay619.f",   0x1000, 0x0800, CRC(3af4b719) SHA1(3122138ac36b1a129226836ddf1916d763d73e10) )
	ROM_LOAD( "dplay619.e",   0x1800, 0x0800, CRC(65cab4fc) SHA1(1ce7cb832e95e4a6d0005bf730eec39225b2e960) )
ROM_END

ROM_START( maze )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "invaders.h",   0x0000, 0x0800, CRC(f2860cff) SHA1(62b3fd3d04bf9c5dd9b50964374fb884dc0ab79c) )
	ROM_LOAD( "invaders.g",   0x0800, 0x0800, CRC(65fad839) SHA1(893f0a7621e7df19f777be991faff0db4a9ad571) )
ROM_END

ROM_START( tornbase )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "tb.h",         0x0000, 0x0800, CRC(653f4797) SHA1(feb4c802aa3e0c2a66823cd032496cca5742c883) )
	ROM_LOAD( "tb.g",         0x0800, 0x0800, CRC(b63dcdb3) SHA1(bdaa0985bcb5257204ee10faa11a4e02a38b9ac5) )
	ROM_LOAD( "tb.f",         0x1000, 0x0800, CRC(215e070c) SHA1(425915b37e5315f9216707de0850290145f69a30) )
ROM_END

ROM_START( checkmat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "checkmat.h",   0x0000, 0x0400, CRC(3481a6d1) SHA1(f758599d6393398a6a8e6e7399dc1a3862604f65) )
	ROM_LOAD( "checkmat.g",   0x0400, 0x0400, CRC(df5fa551) SHA1(484ff9bfb95166ba09f34c753a7908a73de3cc7d) )
	ROM_LOAD( "checkmat.f",   0x0800, 0x0400, CRC(25586406) SHA1(39e0cf502735819a7e1d933e3686945fcfae21af) )
	ROM_LOAD( "checkmat.e",   0x0c00, 0x0400, CRC(59330d84) SHA1(453f95dd31968d439339c41e625481170437eb0f) )
ROM_END

ROM_START( desertgu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "desertgu.h",   0x0000, 0x0800, CRC(c0030d7c) SHA1(4d0a3a59d4f8181c6e30966a6b1d19ba5b29c398) )
	ROM_LOAD( "desertgu.g",   0x0800, 0x0800, CRC(1ddde10b) SHA1(8fb8e85844a8ec6c0722883013ecdd4eeaeb08c1) )
	ROM_LOAD( "desertgu.f",   0x1000, 0x0800, CRC(808e46f1) SHA1(1cc4e9b0aa7e9546c133bd40d40ede6f2fbe93ba) )
	ROM_LOAD( "desertgu.e",   0x1800, 0x0800, CRC(ac64dc62) SHA1(202433dfb174901bd3b91e843d9d697a8333ef9e) )
ROM_END

ROM_START( ozmawars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "mw01",         0x0000, 0x0800, CRC(31f4397d) SHA1(bba9765aadd608d19e2515a5edf8e0eceb70916a) )
	ROM_LOAD( "mw02",         0x0800, 0x0800, CRC(d8e77c62) SHA1(84fc81cf9a924ecbb13a008cd7435b7d465bddf6) )
	ROM_LOAD( "mw03",         0x1000, 0x0800, CRC(3bfa418f) SHA1(7318878202322a2263551ca463e4c70943401f68) )
	ROM_LOAD( "mw04",         0x1800, 0x0800, CRC(e190ce6c) SHA1(120898e9a683f5ce874c6fde761570a26de2fa8c) )
	ROM_LOAD( "mw05",         0x4000, 0x0800, CRC(3bc7d4c7) SHA1(b084f8cd2ce0f502c2e915da3eceffcbb448e9c0) )
	ROM_LOAD( "mw06",         0x4800, 0x0800, CRC(99ca2eae) SHA1(8d0f220f68043eff0c85d2de7bee7fd4365fb51c) )
ROM_END

ROM_START( ozmawar2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "mw01",         0x0000, 0x0800, CRC(31f4397d) SHA1(bba9765aadd608d19e2515a5edf8e0eceb70916a) )
	ROM_LOAD( "mw02",         0x0800, 0x0800, CRC(d8e77c62) SHA1(84fc81cf9a924ecbb13a008cd7435b7d465bddf6) )
	ROM_LOAD( "oz5",          0x1000, 0x0400, CRC(5597bf52) SHA1(626c7348365ed974d416485d94d057745b5d9b96) )
	ROM_LOAD( "oz6",          0x1400, 0x0400, CRC(19b43578) SHA1(3609b7c77f5ee6f10f302892f56fcc8375577f20) )
	ROM_LOAD( "oz7",          0x1800, 0x0400, CRC(a285bfde) SHA1(ed7a9fce4d887d3b5d596645893ea87c0bafda02) )
	ROM_LOAD( "oz8",          0x1c00, 0x0400, CRC(ae59a629) SHA1(0c9ea67dc35f93ec65ec91e1dab2e4b6212428bf) )
	ROM_LOAD( "mw05",         0x4000, 0x0800, CRC(3bc7d4c7) SHA1(b084f8cd2ce0f502c2e915da3eceffcbb448e9c0) )
	ROM_LOAD( "oz11",         0x4800, 0x0400, CRC(660e934c) SHA1(1d50ae3a9de041b908e256892203ce1738d588f6) )
	ROM_LOAD( "oz12",         0x4c00, 0x0400, CRC(8b969f61) SHA1(6d12cacc73c31a897812ccd8de24725ee56dd975) )
ROM_END

ROM_START( solfight )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "solfight.m",   0x0000, 0x0800, CRC(a4f2814e) SHA1(e2437e3543dcc97eeaea32babcd4aec6455581ac) )
	ROM_LOAD( "solfight.n",   0x0800, 0x0800, CRC(5657ec07) SHA1(9a2fb398841160f59483bb70060caba37addb8a4) )
	ROM_LOAD( "solfight.p",   0x1000, 0x0800, CRC(ef9ce96d) SHA1(96867b4f2d72f3a8827b1eb3a0748922eaa8d608) )
	ROM_LOAD( "solfight.r",   0x1800, 0x0800, CRC(4f1ef540) SHA1(a798e57959e72bfb554dd2fed0e37027312f9ed3) )
	ROM_LOAD( "mw05",         0x4000, 0x0800, CRC(3bc7d4c7) SHA1(b084f8cd2ce0f502c2e915da3eceffcbb448e9c0) )
	ROM_LOAD( "solfight.t",   0x4800, 0x0800, CRC(3b6fb206) SHA1(db631f4a0bd5344d130ff8d723d949e9914b6f92) )
ROM_END

ROM_START( spaceph )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sv01.bin",     0x0000, 0x0400, CRC(de84771d) SHA1(13a7e5eedb826cca4d59634d38db9fcf5e65b732) )
	ROM_LOAD( "sv02.bin",     0x0400, 0x0400, CRC(957fc661) SHA1(ac0edc901d8033619f62967f8eaf53a02947e109) )
	ROM_LOAD( "sv03.bin",     0x0800, 0x0400, CRC(dbda38b9) SHA1(73a277616a0c236b07c9ffa66f16a27a78c12d70) )
	ROM_LOAD( "sv04.bin",     0x0c00, 0x0400, CRC(f51544a5) SHA1(368411a2dadaebcbb4d5b6cf6c2beec036ce817f) )
	ROM_LOAD( "sv05.bin",     0x1000, 0x0400, CRC(98d02683) SHA1(f13958df8d385f532e993e4c34569d992904a4ed) )
	ROM_LOAD( "sv06.bin",     0x1400, 0x0400, CRC(4ec390fd) SHA1(ade23efde5d55d282fbb28a5f8a1346601501b79) )
	ROM_LOAD( "sv07.bin",     0x1800, 0x0400, CRC(170862fd) SHA1(ac64a97b1510ca81d4ef3a5fcf45b7e6c7414914) )
	ROM_LOAD( "sv08.bin",     0x1c00, 0x0400, CRC(511b12cf) SHA1(08ba43024c8574ded11aa457eca24b72984f5ea9) )
	ROM_LOAD( "sv09.bin",     0x4000, 0x0400, CRC(af1cd1af) SHA1(286d77e8556e475b291a3b1a53acaca8b7dc3678) )
	ROM_LOAD( "sv10.bin",     0x4400, 0x0400, CRC(31b7692e) SHA1(043880750d134d04311eab55e30ee223977d3d17) )
	ROM_LOAD( "sv11.bin",     0x4800, 0x0400, CRC(50257351) SHA1(5c3eb29f36f04b7fb8f0351ccf9c8cfc7587f927) )
	ROM_LOAD( "sv12.bin",     0x4c00, 0x0400, CRC(a2a3366a) SHA1(87032787450216d378406122effa95ea01145bf7) )
ROM_END

ROM_START( ballbomb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "tn01",         0x0000, 0x0800, CRC(551585b5) SHA1(7c17b046bdfca6ab107b7e68ba9bde6ca590c3d4) )
	ROM_LOAD( "tn02",         0x0800, 0x0800, CRC(7e1f734f) SHA1(a15656818cd730d9bc98d00ff1e7fe3f860bd624) )
	ROM_LOAD( "tn03",         0x1000, 0x0800, CRC(d93e20bc) SHA1(2bf72f813750cef8fad572a18fb8e9fd5bf38804) )
	ROM_LOAD( "tn04",         0x1800, 0x0800, CRC(d0689a22) SHA1(1f6b258431b7eb878853ff979e4d97a05fb6b797) )
	ROM_LOAD( "tn05-1",       0x4000, 0x0800, CRC(5d5e94f1) SHA1(b9f8ba38161ef4f0940c274e9d93fed4bb7db017) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "tn06",         0x0000, 0x0400, CRC(7ec554c4) SHA1(b638605ba2043fdca4c5e18755fa5fa81ed3db07) )
	ROM_LOAD( "tn07",         0x0400, 0x0400, CRC(deb0ac82) SHA1(839581c4e58cb7b0c2c14cf4f239220017cc26eb) )
ROM_END

ROM_START( yosakdon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "yd1.bin", 	  0x0000, 0x0400, CRC(607899c9) SHA1(219c0c99894715818606fba49cc75517f6f43e0c) )
	ROM_LOAD( "yd2.bin", 	  0x0400, 0x0400, CRC(78336df4) SHA1(b0b6254568d191d2d0b9c9280a3ccf2417ef3f38) )
	ROM_LOAD( "yd3.bin", 	  0x0800, 0x0400, CRC(c5af6d52) SHA1(c40af79fe060562c64fc316881b7d0348e11ee3f) )
	ROM_LOAD( "yd4.bin", 	  0x0c00, 0x0400, CRC(dca8064f) SHA1(77a58137cc7f0b5fbe0e9e8deb9c5be88b1ebbcf) )
	ROM_LOAD( "yd5.bin", 	  0x1400, 0x0400, CRC(38804ff1) SHA1(9b7527b9d2b106355f0c8df46666b1e3f286b2e3) )
	ROM_LOAD( "yd6.bin", 	  0x1800, 0x0400, CRC(988d2362) SHA1(deaf864b4e287cbc2585c2a11343b1ae82e15463) )
	ROM_LOAD( "yd7.bin", 	  0x1c00, 0x0400, CRC(2744e68b) SHA1(5ad5a7a615d36f57b6d560425e035c15e25e9005) )
ROM_END

ROM_START( yosakdoa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "yosaku1",      0x0000, 0x0400, CRC(d132f4f0) SHA1(373c7ea1bd6debcb3dad5881793b8c31dc7a01e6) )
	ROM_LOAD( "yd2.bin", 	  0x0400, 0x0400, CRC(78336df4) SHA1(b0b6254568d191d2d0b9c9280a3ccf2417ef3f38) )
	ROM_LOAD( "yosaku3",      0x0800, 0x0400, CRC(b1a0b3eb) SHA1(4eb80668920b45dc6216424f8ca53d753a35f4f1) )
	ROM_LOAD( "yosaku4",      0x0c00, 0x0400, CRC(c06c225e) SHA1(2699e3c13b09b6de16bd3ca3ca2e9d7a91b7e268) )
	ROM_LOAD( "yosaku5",      0x1400, 0x0400, CRC(ae422a43) SHA1(5219680f9d6c5d984b29167f85106fa375856121) )
	ROM_LOAD( "yosaku6",      0x1800, 0x0400, CRC(26b24a12) SHA1(387589fa4027d41b6fb06555661d4f92fe2f990c) )
	ROM_LOAD( "yosaku7",      0x1c00, 0x0400, CRC(878d5a18) SHA1(6adc8763d5644602eed7fe6d9186a48be105aace) )
ROM_END

ROM_START( sstrangr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "hss-01.58",     0x0000, 0x0400, CRC(feec7600) SHA1(787a6be4e24ce931e7678e777699b9f6789bc199) )
	ROM_LOAD( "hss-02.59",     0x0400, 0x0400, CRC(7281ff0b) SHA1(56649d1362be1b9f517cb8616cbf9e4f955e9a2d) )
	ROM_LOAD( "hss-03.60",     0x0800, 0x0400, CRC(a09ec572) SHA1(9c4ad811a6c0460403f9cdc9fe5381c460249ff5) )
	ROM_LOAD( "hss-04.61",     0x0c00, 0x0400, CRC(ec411aca) SHA1(b72eb6f7c3d69e2829280d1ab982099f6eff0bde) )
	ROM_LOAD( "hss-05.62",     0x1000, 0x0400, CRC(7b1b81dd) SHA1(3fa6e244e203fb75f92b19db7b4b18645b3f66a3) )
	ROM_LOAD( "hss-06.63",     0x1400, 0x0400, CRC(de383625) SHA1(7ec0d7171e771c4b43e026f3f50a88d8ab2236bb) )
	ROM_LOAD( "hss-07.64",     0x1800, 0x0400, CRC(2e41d0f0) SHA1(bba720b0c5a7bd47abb8bc8498a989e17dc52428) )
	ROM_LOAD( "hss-08.65",     0x1c00, 0x0400, CRC(bd14d0b0) SHA1(9665f639afef9c1291f2efc054216ff44c595b45) )
ROM_END

ROM_START( sstrngr2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "4764.09",      0x0000, 0x2000, CRC(d88f86cc) SHA1(9f284ee50caf3c64bd04a79a798de620348881bc) )
	ROM_LOAD( "2708.10",      0x6000, 0x0400, CRC(eba304c1) SHA1(3fa6fbb29fa46c146283f69a712bfc51cbb2a43c) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "2708.15",      0x0000, 0x0400, CRC(c176a89d) SHA1(955dd540dc3787091c3f34ae122a13e6b7523414) )
ROM_END

ROM_START( indianbt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1.36",       0x0000, 0x0800, CRC(ddc2b25d) SHA1(120ae17492b79d7d2ad515de9f1e3be7f8b9d4eb) )
	ROM_LOAD( "2.35",       0x0800, 0x0800, CRC(6499b062) SHA1(62a301d532b9fc4e7a17cbe8d2061eb0e842bdfa) )
	ROM_LOAD( "3.34",       0x1000, 0x0800, CRC(5c51675d) SHA1(1313e8794ee6cd0252452b96d42cff7907eeaa21) )
	ROM_LOAD( "4.33",       0x1800, 0x0800, CRC(70ebec95) SHA1(f6e1e7a28033d89e49b88c559ea8926b1b4ff21b) )
	ROM_LOAD( "5.32",       0x4000, 0x0800, CRC(7b4022f4) SHA1(10dec8110e8f4bc79764d3183bdfb3c135e27faf) )
	ROM_LOAD( "6.31",       0x4800, 0x0800, CRC(89bd6f73) SHA1(5dc63871252c530ef0aae4f4cd02fee44b397815) )
	ROM_LOAD( "7.42",       0x5000, 0x0800, CRC(7060ba0b) SHA1(366ce02b7b0a3391afef23b8b41cd98a91034830) )
	ROM_LOAD( "8.41",       0x5800, 0x0800, CRC(eaccfc0a) SHA1(c6c2d702243bdd1d2ad5fbaaceadb5a5798577bc) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 */
	ROM_LOAD( "mb7054.1",   0x0000, 0x0400, CRC(4acf4db3) SHA1(842a6c9f91806b424b7cc437670b4fe0bd57dff1) )
	ROM_LOAD( "mb7054.2",   0x0400, 0x0400, CRC(62cb3419) SHA1(3df65062945589f1df37359dbd3e30ae4b23f469) )
ROM_END

ROM_START( shuttlei )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.13c",   0x0000, 0x0400, CRC(b6d4f0cd) SHA1(f855a793e78ff6283288c815b59e6942513ab4f8) )
	ROM_LOAD( "2.11c",   0x0400, 0x0400, CRC(168d6138) SHA1(e0e5ba58eb5a3a00802504c48a96d63522f9865f) )
	ROM_LOAD( "3.13d",   0x0800, 0x0400, CRC(804bd7fb) SHA1(f019bcc2894f9b819a14c069de8f1a7d228b79eb) )
	ROM_LOAD( "4.11d",   0x0c00, 0x0400, CRC(8205b369) SHA1(685dd244881f5762d0f53cbfa935da2b857e3fba) )
	ROM_LOAD( "5.13e",   0x1000, 0x0400, CRC(b50df820) SHA1(27a846ac3da4c0890a80f60483ed5750cb0b2476) )

	ROM_LOAD( "8.11f",   0x1c00, 0x0400, CRC(4978552b) SHA1(5a6b6e39f57a353580ed9281d7da24950f058426) )
ROM_END

ROM_START( steelwkr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.36",     		  0x0000, 0x0400, CRC(5d78873a) SHA1(293cbc067937668148181453877239cb5ed57600) )
	ROM_LOAD( "2.35",     		  0x0400, 0x0400, CRC(99cd70c6) SHA1(a08bf4db6b39d22dfcf052cc6603aab041db0208) )
	ROM_LOAD( "3.34",     		  0x0800, 0x0400, CRC(18103b67) SHA1(45929ea56ab15769fc68873570aab3d403e8e913) )
	ROM_LOAD( "4.33",     		  0x0c00, 0x0400, CRC(c413ae82) SHA1(302b933b45b2aaa515434b5268fd74aec4160e3f) )
	ROM_LOAD( "5.32",     		  0x1000, 0x0400, CRC(ca7b07b5) SHA1(cbea221c4daf84825f99bbef6d731fc2ef88feeb) )
	ROM_LOAD( "6.31",     		  0x1400, 0x0400, CRC(f8181fa0) SHA1(a907611529a1500a2ae118e834c2d4b6d11974f1) )
	ROM_LOAD( "7.42",     		  0x1800, 0x0400, CRC(a35f113e) SHA1(53073037db55c14055810c0bee7b85eb75bbaa72) )
	ROM_LOAD( "8.41",     		  0x1c00, 0x0400, CRC(af208370) SHA1(ccbd002accda26cc0a02987d9801a47e5f49921a) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )		/* color maps player 1/player 2 (not used, but they were on the board) */
	ROM_LOAD( "la05.1",         0x0000, 0x0400, CRC(98f31392) SHA1(ccdd1bd2ddd24bd6b1f8255a87e138f937eaf5b4) )
	ROM_LOAD( "la06.2",         0x0400, 0x0400, CRC(98f31392) SHA1(ccdd1bd2ddd24bd6b1f8255a87e138f937eaf5b4) )
ROM_END

/* Midway games */

/* board #            rom       parent    machine   inp       init (overlay/color hardware setup) */

/* 596 */ GAME( 1976, seawolf,  0,        seawolf,  seawolf,  seawolf,  ROT0,   "Midway", "Sea Wolf", GAME_IMPERFECT_SOUND )
/* 597 */ GAME( 1975, gunfight, 0,        gunfight, gunfight, gunfight, ROT0,   "Midway", "Gun Fight", GAME_NO_SOUND )
/* 605 */ GAME( 1976, tornbase, 0,        8080bw,   tornbase, 8080bw,	ROT0,   "Midway", "Tornado Baseball", GAME_NO_SOUND )
/* 610 */ GAME( 1976, 280zzzap, 0,        280zzzap, 280zzzap, 8080bw,	ROT0,   "Midway", "Datsun 280 Zzzap", GAME_NO_SOUND )
/* 611 */ GAME( 1976, maze,     0,        8080bw,   maze,     8080bw,	ROT0,   "Midway", "Amazing Maze", GAME_NO_SOUND )
/* 612 */ GAME( 1977, boothill, 0,        boothill, boothill, 8080bw,   ROT0,   "Midway", "Boot Hill" , GAME_IMPERFECT_SOUND )
/* 615 */ GAME( 1977, checkmat, 0,        checkmat, checkmat, 8080bw,	ROT0,   "Midway", "Checkmate", GAME_NO_SOUND )
/* 618 */ GAME( 1977, desertgu, 0,        desertgu, desertgu, desertgu,	ROT0,   "Midway", "Desert Gun", GAME_NO_SOUND )
/* 619 */ GAME( 1977, dplay,    einnings, m4,       einnings, 8080bw,	ROT0,   "Midway", "Double Play", GAME_NO_SOUND )
/* 622 */ GAME( 1977, lagunar,  0,        280zzzap, lagunar,  8080bw,   ROT90,  "Midway", "Laguna Racer", GAME_NO_SOUND )
/* 623 */ GAME( 1977, gmissile, 0,        m4,       gmissile, 8080bw,   ROT0,   "Midway", "Guided Missile", GAME_NO_SOUND )
/* 626 */ GAME( 1977, m4,       0,        m4,       m4,       8080bw,   ROT0,   "Midway", "M-4", GAME_NO_SOUND )
/* 630 */ GAME( 1978, clowns,   0,        clowns,   clowns,   8080bw,   ROT0,   "Midway", "Clowns (rev. 2)", GAME_IMPERFECT_SOUND )
/* 630 */ GAME( 1978, clowns1,  clowns,   clowns,   clowns1,  8080bw,   ROT0,   "Midway", "Clowns (rev. 1)", GAME_IMPERFECT_SOUND )
/* 640                                                                          "Midway", "Space Walk" */
/* 642 */ GAME( 1978, einnings, 0,        m4,       einnings, 8080bw,	ROT0,   "Midway", "Extra Inning", GAME_NO_SOUND )
/* 643 */ GAME( 1978, shuffle,  0,        shuffle,  shuffle,  8080bw,	ROT90,  "Midway", "Shuffleboard", GAME_NO_SOUND )
/* 644 */ GAME( 1977, dogpatch, 0,        clowns,   dogpatch, 8080bw,   ROT0,   "Midway", "Dog Patch", GAME_NO_SOUND )
/* 645 */ GAME( 1980, spcenctr, 0,        spcenctr, spcenctr, 8080bw,	ROT0,   "Midway", "Space Encounters", GAME_NO_SOUND )
/* 652 */ GAME( 1979, phantom2, 0,        phantom2, phantom2, phantom2, ROT0,   "Midway", "Phantom II", GAME_NO_SOUND )
/* 730 */ GAME( 1978, bowler,   0,        bowler,   bowler,   bowler,	ROT90,  "Midway", "4 Player Bowling Alley", GAME_NO_SOUND | GAME_NO_COCKTAIL )
/* 739 */ GAME( 1978, invaders, 0,        invaders, invaders, invaders, ROT270, "Midway", "Space Invaders", 0 )
/* 742 */ GAME( 1978, blueshrk, 0,        blueshrk, blueshrk, blueshrk, ROT0,   "Midway", "Blue Shark", GAME_NO_SOUND )
/* 749                                                                          "Midway", "4 Player Bowling Alley (cocktail) "*/
/* 851 */ GAME( 1980, invad2ct, 0,        invad2ct, invad2ct, invad2ct, ROT90,  "Midway", "Space Invaders II (Midway, cocktail)", 0 )
/* 852 */ GAME( 1980, invaddlx, invadpt2, invaders, invadpt2, invaddlx, ROT270, "Midway", "Space Invaders Deluxe", 0 )
/* 870                                                                          "Midway", "Space Invaders Deluxe (cocktail) "*/

/* Taito games */

	  GAME( 1978, sitv,     invaders, invaders, sitv,     invaders, ROT270, "Taito", "Space Invaders (TV Version)", 0 )
	  GAME( 1979, sicv,     invaders, invadpt2, invaders, invadpt2, ROT270, "Taito", "Space Invaders (CV Version)", 0 )
	  GAME( 1978, sisv,     invaders, invadpt2, invaders, invadpt2, ROT270, "Taito", "Space Invaders (SV Version)", 0 )
	  GAME( 1978, sisv2,    invaders, invadpt2, invaders, invadpt2, ROT270, "Taito", "Space Invaders (SV Version 2)", 0 )
	  GAME( 1979, galxwars, 0,        invaders, galxwars, invaders, ROT270, "Universal", "Galaxy Wars (Universal set 1)", 0 )
	  GAME( 1979, galxwar2, galxwars, invaders, galxwars, invaders, ROT270, "Universal", "Galaxy Wars (Universal set 2)", 0 )
	  GAME( 1979, galxwart, galxwars, invaders, galxwars, invaders, ROT270,	"Taito?", "Galaxy Wars (Taito?)" , 0) /* Copyright Not Displayed */
	  GAME( 1979, starw,    galxwars, invaders, galxwars, invaders, ROT270, "bootleg", "Star Wars", 0 )
	  GAME( 1979, lrescue,  0,        invadpt2, lrescue,  invadpt2, ROT270, "Taito", "Lunar Rescue", 0 )
	  GAME( 1979, grescue,  lrescue,  invadpt2, lrescue,  invadpt2, ROT270, "Taito (Universal license?)", "Galaxy Rescue", 0 )
	  GAME( 1979, desterth, lrescue,  invadpt2, invrvnge, invadpt2, ROT270, "bootleg", "Destination Earth", 0 )
	  GAME( 1979, invadpt2, 0,        invadpt2, invadpt2, invadpt2, ROT270, "Taito", "Space Invaders Part II (Taito)", 0 )
	  GAME( 1979, cosmo,    0,        cosmo,    cosmo,    cosmo,    ROT90,  "TDS & Mints", "Cosmo", GAME_NO_SOUND )
	  GAME( 1979, schaser,  0,        schaser,  schaser,  schaser,  ROT270, "Taito", "Space Chaser", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_COLORS )
	  GAME( 1979, schasrcv, schaser,  lupin3,   schasrcv, schaser,  ROT270, "Taito", "Space Chaser (CV version)", GAME_NO_SOUND | GAME_IMPERFECT_COLORS | GAME_NO_COCKTAIL )
	  GAME( 1979, sflush,   0,        sflush,   sflush,   rollingc,	ROT270, "Taito", "Straight Flush",GAME_NO_SOUND| GAME_IMPERFECT_COLORS | GAME_NO_COCKTAIL)
	  GAME( 1980, lupin3,   0,        lupin3,   lupin3,   lupin3,   ROT270, "Taito", "Lupin III", GAME_NO_SOUND | GAME_NO_COCKTAIL )
	  GAME( 1980, polaris,  0,        polaris,  polaris,  polaris,  ROT270, "Taito", "Polaris (set 1)", 0 )
	  GAME( 1980, polarisa, polaris,  polaris,  polaris,  polaris,  ROT270, "Taito", "Polaris (set 2)", 0 )
	  GAME( 1980, ballbomb, 0,        ballbomb, ballbomb, invadpt2, ROT270, "Taito", "Balloon Bomber", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )	/* missing clouds and blue background */
	  GAME( 1980, indianbt, 0,        indianbt, indianbt, indianbt, ROT270, "Taito", "Indian Battle", 0 )
	  GAME( 1980, steelwkr, 0,        steelwkr, steelwkr, invadpt2, ROT0,   "Taito", "Steel Worker", 0 )

/* Misc. manufacturers */

	  GAME( 1980, searthin, invaders, invaders, earthinv, invaders, ROT270, "bootleg", "Super Earth Invasion (set 1)", 0 )
	  GAME( 1980, searthia, invaders, invaders, earthinv, invaders, ROT270, "bootleg", "Super Earth Invasion (set 2)", 0 )
	  GAME( 1978, invadrmr, invaders, invaders, invaders, invaders, ROT270, "Model Racing", "Space Invaders (Model Racing)", 0 )
	  GAME( 1978, spaceatt, invaders, invaders, invaders, invaders, ROT270, "Video Games GMBH", "Space Attack", 0 )
	  GAME( 1980, spaceat2, invaders, invaders, spaceatt, invaders, ROT270, "Zenitone-Microsec Ltd", "Space Attack II", 0 )
	  GAME( 19??, sinvzen,  invaders, invaders, spaceatt, invaders, ROT270, "Zenitone-Microsec Ltd", "Super Invaders (Zenitone-Microsec)", 0 )
	  GAME( 19??, sinvemag, invaders, invaders, sinvemag, invaders, ROT270, "bootleg", "Super Invaders (EMAG)", 0 )
	  GAME( 19??, tst_invd, invaders, invaders, invaders, invaders, ROT0,   "Test ROM", "Space Invaders Test ROM", 0 )
	  GAME( 19??, alieninv, invaders, invaders, earthinv, invaders, ROT270, "bootleg", "Alien Invasion Part II", 0 )
	  GAME( 1978, spceking, invaders, invaders, spceking, invaders, ROT270, "Leijac (Konami)","Space King", 0 )
	  GAME( 1978, spcewars, invaders, invaders, invadpt2, invaders, ROT270, "Sanritsu", "Space War (Sanritsu)", 0 )
	  GAME( 1978, spacewr3, invaders, invaders, spacewr3, invaders, ROT270, "bootleg", "Space War Part 3", 0 )
	  GAME( 1978, invaderl, invaders, invaders, invaders, invaders, ROT270, "bootleg", "Space Invaders (Logitec)", 0 )
	  GAME( 1978, invader4, invaders, invaders, invaders, invaders, ROT270, "bootleg", "Space Invaders Part Four", 0 )
	  GAME( 1979, jspecter, invaders, invaders, jspecter, invaders, ROT270, "Jatre", "Jatre Specter (set 1)", 0 )
	  GAME( 1979, jspectr2, invaders, invaders, jspecter, invaders, ROT270, "Jatre", "Jatre Specter (set 2)", 0 )
	  GAME( 1979, cosmicmo, invaders, invaders, cosmicmo, invaders, ROT270, "Universal", "Cosmic Monsters", 0 )
          GAME( 1979, cosmicm2, invaders, invaders, cosmicmo, invaders, ROT270, "Universal", "Cosmic Monsters 2", 0 )
	  GAME( 19??, superinv, invaders, invaders, invaders, invaders, ROT270, "bootleg", "Super Invaders", 0 )
	  GAME( 19??, invasion, invaders, invaders, invasion, invaders, ROT270, "Sidam", "Invasion", 0 )
	  GAME( 1978, sstrangr, 0,		  sstrangr, sstrangr, 8080bw,   ROT270,	"Yachiyo Electronics, Ltd.", "Space Stranger", 0 )
	  GAME( 1979, sstrngr2, 0,        sstrngr2, sstrngr2, sstrngr2, ROT270, "Yachiyo Electronics, Ltd.", "Space Stranger 2", 0 )
	  GAME( 1979, moonbase, invadpt2, invadpt2, invadpt2, invadpt2, ROT270, "Nichibutsu", "Moon Base", 0 )
	  GAME( 19??, invrvnge, 0,        invrvnge, invrvnge, invrvnge, ROT270, "Zenitone-Microsec Ltd.", "Invader's Revenge",  GAME_NO_SOUND )
	  GAME( 19??, invrvnga, invrvnge, invrvnge, invrvnge, invrvnge, ROT270, "Zenitone-Microsec Ltd. (Dutchford license)", "Invader's Revenge (Dutchford)", GAME_NO_SOUND )
	  GAME( 1980, spclaser, 0,        invaders, spclaser, invaddlx, ROT270, "GamePlan (Taito)", "Space Laser", 0 )
	  GAME( 1980, laser,    spclaser, invaders, spclaser, invaddlx, ROT270, "<unknown>", "Laser", 0 )
	  GAME( 1979, spcewarl, spclaser, invaders, spcewarl, invaddlx, ROT270, "Leijac (Konami)","Space War (Leijac)", 0 )
	  GAME( 1979, rollingc, 0,        rollingc, rollingc, rollingc, ROT270, "Nichibutsu", "Rolling Crash / Moon Base", GAME_NO_SOUND )
	  GAME( 1979, ozmawars, 0,        invaders, ozmawars, 8080bw,   ROT270, "SNK", "Ozma Wars (set 1)", 0 )
	  GAME( 1979, ozmawar2, ozmawars, invaders, ozmawars, 8080bw,   ROT270, "SNK", "Ozma Wars (set 2)" , 0) /* Uses Taito's three board colour version of Space Invaders PCB */
	  GAME( 1979, solfight, ozmawars, invaders, ozmawars, 8080bw,   ROT270, "bootleg", "Solar Fight", 0 )
	  GAME( 1979, spaceph,  ozmawars, invaders, spaceph,  8080bw,   ROT270, "Zilec Games", "Space Phantoms", 0 )
	  GAME( 1979, yosakdon, 0,        yosakdon, yosakdon, 8080bw,   ROT270, "Wing", "Yosaku To Donbei (set 1)", GAME_NO_SOUND ) /* bootleg? */
	  GAME( 1979, yosakdoa, yosakdon, yosakdon, yosakdon, 8080bw,   ROT270, "Wing", "Yosaku To Donbei (set 2)", GAME_NO_SOUND ) /* bootleg? */
	  GAME( 197?, shuttlei, 0,  	  shuttlei, shuttlei, shuttlei, ROT270, "Omori", "Shuttle Invader", GAME_NO_COCKTAIL | GAME_NO_SOUND )
             

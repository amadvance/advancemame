// license:BSD-3-Clause
// copyright-holders:Takahiro Nogi, hap
/***************************************************************************

Dottori Kun (Head On's mini game)
(c)1990 SEGA

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/12/15 -


CPU   : Z-80 (4MHz)
SOUND : (none)

14479.MPR  ; PRG (FIRST VER)
14479A.MPR ; PRG (NEW VER)

* This game is only for the test of cabinet
* BackRaster = WHITE on the FIRST version.
* BackRaster = BLACK on the NEW version.
* On the NEW version, push COIN-SW as TEST MODE.
* 0000-3FFF:ROM 8000-85FF:VRAM(128x96) 8600-87FF:WORK-RAM

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"


#define DOTRIKUN_MASTER_CLOCK 4000000
#define DOTRIKUN_HTOTAL 256
#define DOTRIKUN_VTOTAL 256
#define DOTRIKUN_VDISPLAY 192

WRITE8_HANDLER( dotrikun_videoram_w );
VIDEO_UPDATE( dotrikun );

WRITE8_HANDLER( dotrikun_color_w );


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x85ff) AM_WRITE(dotrikun_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8600, 0x87ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0xff) AM_READ(input_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0xff) AM_WRITE(dotrikun_color_w)
ADDRESS_MAP_END


INPUT_PORTS_START( dotrikun )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END


static MACHINE_DRIVER_START( dotrikun )

	/* basic machine hardware */
	/* Video fetches steal a quarter of the CPU cycles during the
	   visible 128 of 256 clocks on each of the 192 active lines. */
	MDRV_CPU_ADD(Z80, DOTRIKUN_MASTER_CLOCK * 29 / 32)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND((double)DOTRIKUN_MASTER_CLOCK / (DOTRIKUN_HTOTAL * DOTRIKUN_VTOTAL))
	MDRV_VBLANK_DURATION(1000000 * DOTRIKUN_HTOTAL / DOTRIKUN_MASTER_CLOCK * (DOTRIKUN_VTOTAL - DOTRIKUN_VDISPLAY))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, DOTRIKUN_VDISPLAY-1)
	MDRV_ASPECT_RATIO(1, 1)
	MDRV_PALETTE_LENGTH(2)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(dotrikun)

MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( dotrikun )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mpr-14479a.ic2", 0x0000, 0x4000, CRC(b77a50db) SHA1(2a5d812d39f0f58f5c3e1b46f80aca75aa225115) )
ROM_END

ROM_START( dotriku2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mpr-14479.ic2",  0x0000, 0x4000, CRC(a6aa7fa5) SHA1(4dbea33fb3541fdacf2195355751078a33bb30d5) )
ROM_END


GAME( 1990, dotrikun, 0,        dotrikun, dotrikun, 0, ROT0, "Sega", "Dottori Kun (new version)", 0 )
GAME( 1990, dotriku2, dotrikun, dotrikun, dotrikun, 0, ROT0, "Sega", "Dottori Kun (old version)", GAME_NO_SOUND )

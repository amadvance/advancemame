/*
 *  Chack'n Pop
 *  (C) 1983 TAITO Corp.
 *
 * driver by BUT
 */

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"
#include "sound/ay8910.h"

/* machine/chaknpop.c */
READ8_HANDLER( chaknpop_68705_portA_r );
WRITE8_HANDLER( chaknpop_68705_portA_w );
READ8_HANDLER( chaknpop_68705_portB_r );
WRITE8_HANDLER( chaknpop_68705_portB_w );
READ8_HANDLER( chaknpop_68705_portC_r );
WRITE8_HANDLER( chaknpop_68705_portC_w );
WRITE8_HANDLER( chaknpop_68705_ddrA_w );
WRITE8_HANDLER( chaknpop_68705_ddrB_w );
WRITE8_HANDLER( chaknpop_68705_ddrC_w );
WRITE8_HANDLER( chaknpop_mcu_w );
READ8_HANDLER( chaknpop_mcu_r );
READ8_HANDLER( chaknpop_mcu_status_r );


/* vidhrdw/chaknpop.c */
extern UINT8 *chaknpop_txram;
extern UINT8 *chaknpop_sprram;
extern size_t chaknpop_sprram_size;
extern UINT8 *chaknpop_attrram;


PALETTE_INIT( chaknpop );
VIDEO_START( chaknpop );
VIDEO_UPDATE( chaknpop );

READ8_HANDLER( chaknpop_gfxmode_r );
WRITE8_HANDLER( chaknpop_gfxmode_w );

WRITE8_HANDLER( chaknpop_txram_w );

WRITE8_HANDLER( chaknpop_attrram_w );


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static WRITE8_HANDLER ( unknown_port_1_w )
{
	//logerror("%04x: write to unknow port 1: 0x%02x\n", activecpu_get_pc(), data);
}

static WRITE8_HANDLER ( unknown_port_2_w )
{
	//logerror("%04x: write to unknow port 2: 0x%02x\n", activecpu_get_pc(), data);
}

static WRITE8_HANDLER ( coinlock_w )
{
	logerror("%04x: coin lock %sable\n", activecpu_get_pc(), data ? "dis" : "en");
}


/***************************************************************************

  Memory Map(s)

***************************************************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
    AM_RANGE(0x8800, 0x8800) AM_READ(chaknpop_mcu_r)
	AM_RANGE(0x8801, 0x8801) AM_READ(chaknpop_mcu_status_r)
	AM_RANGE(0x8802, 0x8802) AM_READ(MRA8_NOP)            /* watchdog? */
	AM_RANGE(0x8805, 0x8805) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x8807, 0x8807) AM_READ(AY8910_read_port_1_r)
	AM_RANGE(0x8808, 0x8808) AM_READ(input_port_3_r)		// DSW C
	AM_RANGE(0x8809, 0x8809) AM_READ(input_port_1_r)		// IN1
	AM_RANGE(0x880a, 0x880a) AM_READ(input_port_0_r)		// IN0
	AM_RANGE(0x880b, 0x880b) AM_READ(input_port_2_r)		// IN2
	AM_RANGE(0x880c, 0x880c) AM_READ(chaknpop_gfxmode_r)
	AM_RANGE(0x9000, 0x93ff) AM_READ(MRA8_RAM)			// TX tilemap
	AM_RANGE(0x9800, 0x983f) AM_READ(MRA8_RAM)			// Color attribute
	AM_RANGE(0x9840, 0x98ff) AM_READ(MRA8_RAM)			// sprite
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xffff) AM_READ(MRA8_BANK1)			// bitmap plane 1-4
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
    AM_RANGE(0x8800, 0x8800) AM_WRITE(chaknpop_mcu_w)
	AM_RANGE(0x8802, 0x8802) AM_WRITE(MWA8_NOP) 
	AM_RANGE(0x8804, 0x8804) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8805, 0x8805) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x8806, 0x8806) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8807, 0x8807) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x880c, 0x880c) AM_WRITE(chaknpop_gfxmode_w)
	AM_RANGE(0x880D, 0x880D) AM_WRITE(coinlock_w)			// coin lock out
	AM_RANGE(0x9000, 0x93ff) AM_WRITE(chaknpop_txram_w) AM_BASE(&chaknpop_txram)
	AM_RANGE(0x9800, 0x983f) AM_WRITE(chaknpop_attrram_w) AM_BASE(&chaknpop_attrram)
	AM_RANGE(0x9840, 0x98ff) AM_WRITE(MWA8_RAM) AM_BASE(&chaknpop_sprram) AM_SIZE(&chaknpop_sprram_size)
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(MWA8_BANK1)			// bitmap plane 1-4
ADDRESS_MAP_END

static ADDRESS_MAP_START( chaknpop_m68705_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READ(chaknpop_68705_portA_r)
	AM_RANGE(0x0001, 0x0001) AM_READ(chaknpop_68705_portB_r)
	AM_RANGE(0x0002, 0x0002) AM_READ(chaknpop_68705_portC_r)
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( chaknpop_m68705_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(chaknpop_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(chaknpop_68705_portB_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE(chaknpop_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(chaknpop_68705_ddrA_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE(chaknpop_68705_ddrB_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(chaknpop_68705_ddrC_w)
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static struct AY8910interface ay8910_interface_1 =
{
	input_port_5_r,		// DSW A
	input_port_4_r		// DSW B
};

static struct AY8910interface ay8910_interface_2 =
{
	0,
	0,
	unknown_port_1_w,	// ??
	unknown_port_2_w	// ??
};


/***************************************************************************

  Input Port(s)

***************************************************************************/

INPUT_PORTS_START( chaknpop )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )	// LEFT COIN
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )	// RIGHT COIN
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")      /* DSW C */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Super Chack'n" )
	PORT_DIPSETTING(    0x04, "pi" )
	PORT_DIPSETTING(    0x00, "1st Chance" )
	PORT_DIPNAME( 0x08, 0x08, "Endless (Cheat)")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Credit Info" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Show Year" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Infinite (Cheat)")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "1 Way" )
	PORT_DIPSETTING(    0x80, "2 Way" )

	PORT_START_TAG("IN4")      /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "80k and every 100k" )
	PORT_DIPSETTING(    0x01, "60k and every 100k" )
	PORT_DIPSETTING(    0x02, "40k and every 100k" )
	PORT_DIPSETTING(    0x03, "20k and every 100k" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPNAME( 0x20, 0x00, "Training/Difficulty" )
	PORT_DIPSETTING(    0x20, "Off/Every 10 Min." )
	PORT_DIPSETTING(    0x00, "On/Every 7 Min." )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START_TAG("IN5")      /* DSW A */
	PORT_DIPNAME(0x0f,  0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME(0xf0,  0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static const gfx_layout spritelayout =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 0x2000*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 ,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0, 0x2000*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0,  8 },
	{ REGION_GFX2, 0, &charlayout,   32, 8 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( chaknpop )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18000000 / 6)	/* Verified on PCB */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
	
	MDRV_CPU_ADD(M68705, 18000000 / 6)	/* Verified on PCB */
	MDRV_CPU_PROGRAM_MAP(chaknpop_m68705_readmem,chaknpop_m68705_writemem)

	MDRV_FRAMES_PER_SECOND(59.1828)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_PALETTE_INIT(chaknpop)
	MDRV_VIDEO_START(chaknpop)
	MDRV_VIDEO_UPDATE(chaknpop)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 18000000 / 12)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)

	MDRV_SOUND_ADD(AY8910, 18000000 / 12)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( chaknpop )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )			/* Main CPU */
	ROM_LOAD( "a04-01.28",    0x00000, 0x2000, CRC(386fe1c8) SHA1(cca24abfb8a7f439251e7936036475c694002561) )
	ROM_LOAD( "a04-02.27",    0x02000, 0x2000, CRC(5562a6a7) SHA1(0c5d81f9aaf858f88007a6bca7f83dc3ef59c5b5) )
	ROM_LOAD( "a04-03.26",    0x04000, 0x2000, CRC(3e2f0a9c) SHA1(f1cf87a4cb07f77104d4a4d369807dac522e052c) )
	ROM_LOAD( "a04-04.25",    0x06000, 0x2000, CRC(5209c7d4) SHA1(dcba785a697df55d84d65735de38365869a1da9d) )
	ROM_LOAD( "a04-05.3",     0x0a000, 0x2000, CRC(8720e024) SHA1(99e445c117d1501a245f9eb8d014abc4712b4963) )

	ROM_REGION( 0x0800,  REGION_CPU2, 0 )	/* 2k for the microcontroller */
    ROM_LOAD( "ao4_06.ic23", 0x0000, 0x0800, CRC(9c78c24c) SHA1(f74c7f3ee106e5c45c907e590ec09614a2bc6751) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprite */
	ROM_LOAD( "a04-08.14",     0x0000, 0x2000, CRC(5575a021) SHA1(c2fad53fe6a12c19cec69d27c13fce6aea2502f2) )
	ROM_LOAD( "a04-07.15",     0x2000, 0x2000, CRC(ae687c18) SHA1(65b25263da88d30cbc0dad94511869596e5c975a) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "a04-09.98",     0x0000, 0x2000, CRC(757a723a) SHA1(62ab84d2aaa9bc1ea5aa9df8155aa3b5a1e93889) )
	ROM_LOAD( "a04-10.97",     0x2000, 0x2000, CRC(3e3fd608) SHA1(053a8fbdb35bf1c142349f78a63e8cd1adb41ef6) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )			/* Palette */
	ROM_LOAD( "a04-11.bin",    0x0000, 0x0400, CRC(9bf0e85f) SHA1(44f0a4712c99a715dec54060afb0b27dc48998b4) )
	ROM_LOAD( "a04-12.bin",    0x0400, 0x0400, CRC(954ce8fc) SHA1(e187f9e2cb754264d149c2896ca949dea3bcf2eb) )
ROM_END


/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT  MONITOR  COMPANY              FULLNAME ) */
GAME( 1983, chaknpop, 0,        chaknpop, chaknpop, 0,    ROT0,  "Taito Corporation", "Chack'n Pop", 0)

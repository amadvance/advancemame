// license:BSD-3-Clause
// copyright-holders:Joseba Epalza,Andreas Naive
/***************************************************************************

Speed Ball / Music Ball map

driver by Joseba Epalza

Z80 MAIN CPU
0000-7fff ROM 1 32k
8000-dbff ROM 2 data
dc00-dfff for both CPU&SOUND (Shared)
e000-e1ff Video RAM background tiles 16*16 (2 bytes: 1 for tile char + 1 for color)
e800-efff Video RAM characters 32*32 (2 bytes: 1 for char and 1 for color)
f000-feff RAM
ff00-ffff Sprites (64 total: four bytes to one)

in:
00  DSW ONE
10  DSW TWO
20  IN1  JOY 1 STATUS & COIN & START
30  IN2  JOY 2 STATUS & TILT

out:
40  coin counters
50  ????  maybe VSYNC ????

 ======================================================================

Z80 SOUND
0000-7fff ROM
dc00-dfff shared with MAIN CPU
f000-dfff RAM

out:
00  YM3812 control
01  YM3812 data
40  ??
80  ??
82  ??
c1  ??

 ======================================================================

  Colors : 2 bits for foreground characters =  4 colors * 16 palettes
           4 bits for background tiles      = 16 colors * 16 palettes
           4 bits for sprites               = 16 colors * 16 palettes

 Note:
 - To enter test mode, keep pressed COIN1 and COIN2 during boot,
   until the RAM / ROM tests are finished

***************************************************************************/

#include "driver.h"
#include "sound/3812intf.h"


extern UINT8 *speedbal_background_videoram;
extern UINT8 *speedbal_foreground_videoram;

VIDEO_START( speedbal );
VIDEO_UPDATE( speedbal );
WRITE8_HANDLER( speedbal_foreground_videoram_w );
WRITE8_HANDLER( speedbal_background_videoram_w );

static WRITE8_HANDLER( speedbal_coincounter_w )
{
	coin_counter_w(0, data & 0x80);
	coin_counter_w(1, data & 0x40);
	flip_screen_set(data & 8); // also changes data & 0x10 at the same time too (flipx and flipy?)
	/* unknown: (data & 0x10) and (data & 4) */
}

static ADDRESS_MAP_START( main_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdbff) AM_ROM
	AM_RANGE(0xdc00, 0xdfff) AM_RAM AM_SHARE(1) // shared with SOUND
	AM_RANGE(0xe000, 0xe1ff) AM_RAM AM_WRITE(speedbal_background_videoram_w) AM_BASE(&speedbal_background_videoram)
	AM_RANGE(0xe800, 0xefff) AM_RAM AM_WRITE(speedbal_foreground_videoram_w) AM_BASE(&speedbal_foreground_videoram)
	AM_RANGE(0xf000, 0xf5ff) AM_RAM AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_be_w) AM_BASE(&paletteram)
	AM_RANGE(0xf600, 0xfeff) AM_RAM
	AM_RANGE(0xff00, 0xffff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_cpu_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x10, 0x10) AM_READ(input_port_1_r)
	AM_RANGE(0x20, 0x20) AM_READ(input_port_2_r)
	AM_RANGE(0x30, 0x30) AM_READ(input_port_3_r)
	AM_RANGE(0x40, 0x40) AM_WRITE(speedbal_coincounter_w)
	AM_RANGE(0x50, 0x50) AM_WRITENOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xd000, 0xdbff) AM_RAM
	AM_RANGE(0xdc00, 0xdfff) AM_RAM AM_SHARE(1) // shared with MAIN CPU
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_cpu_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YM3812_status_port_0_r, YM3812_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x40, 0x40) AM_WRITENOP
	AM_RANGE(0x80, 0x80) AM_WRITENOP
	AM_RANGE(0x82, 0x82) AM_WRITENOP
	AM_RANGE(0xc1, 0xc1) AM_WRITENOP
ADDRESS_MAP_END


INPUT_PORTS_START( speedbal )
	PORT_START_TAG("DSW2")      /* DSW2 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "70000 200000 1M" )
	PORT_DIPSETTING(    0x07, "70000 200000" )
	PORT_DIPSETTING(    0x03, "100000 300000 1M" )
	PORT_DIPSETTING(    0x04, "100000 300000" )
	PORT_DIPSETTING(    0x01, "200000 1M" )
	PORT_DIPSETTING(    0x05, "200000" )
/*  PORT_DIPSETTING(    0x02, "200000" ) */
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty 1" )
	PORT_DIPSETTING(    0x30, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty 2" )
	PORT_DIPSETTING(    0xc0, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW , IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW , IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW , IPT_BUTTON4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW , IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW , IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW , IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW , IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW , IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW , IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW , IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW , IPT_BUTTON4 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW , IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW , IPT_TILT    )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( musicbal )
	PORT_INCLUDE(speedbal)

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "1M 2M 2.5M" )
	PORT_DIPSETTING(    0x06, "1.2M 1.8M 2.5M" )
	PORT_DIPSETTING(    0x07, "1.2M 1.8M" )
	PORT_DIPSETTING(    0x04, "1.5M 2M" )
	PORT_DIPSETTING(    0x05, "1.5M" )
	PORT_DIPSETTING(    0x01, "1.8M 2.5M" )
	PORT_DIPSETTING(    0x02, "1.8M" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	4,      /* actually 2 bits per pixel - two of the planes are empty */
	{ 1024*16*8+4, 1024*16*8+0, 4, 0 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },   /* characters are rotated 90 degrees */
	16*8	   /* every char takes 16 bytes */
};

static const gfx_layout tilelayout =
{
	16,16,  /* 16*16 tiles */
	1024,   /* 1024 tiles */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 }, /* the bitplanes are packed in one nibble */
	{ 0*8+0, 0*8+1, 7*8+0, 7*8+1, 6*8+0, 6*8+1, 5*8+0, 5*8+1,
			4*8+0, 4*8+1, 3*8+0, 3*8+1, 2*8+0, 2*8+1, 1*8+0, 1*8+1 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	16,16,  /* 16*16 sprites */
	512,    /* 512 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 }, /* the bitplanes are packed in one nibble */
	{ 7*8+1, 7*8+0, 6*8+1, 6*8+0, 5*8+1, 5*8+0, 4*8+1, 4*8+0,
			3*8+1, 3*8+0, 2*8+1, 2*8+0, 1*8+1, 1*8+0, 0*8+1, 0*8+0 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	 256, 16 },
	{ REGION_GFX2, 0, &tilelayout,	 512, 16 },
	{ REGION_GFX3, 0, &spritelayout,   0, 16 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( speedbal )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(main_cpu_map,0)
	MDRV_CPU_IO_MAP(main_cpu_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 2660000)	/* 2.66 MHz ???  Maybe yes */
	MDRV_CPU_PROGRAM_MAP(sound_cpu_map,0)
	MDRV_CPU_IO_MAP(sound_cpu_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(speedbal)
	MDRV_VIDEO_UPDATE(speedbal)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 3600000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( speedbal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64K for code: main */
	ROM_LOAD( "sb1.bin",  0x0000,  0x8000, CRC(1c242e34) SHA1(8b2e8983e0834c99761ce2b5ea765dba56e77964) )
	ROM_LOAD( "sb3.bin",  0x8000,  0x8000, CRC(7682326a) SHA1(15a72bf088a9adfaa50c11202b4970e07c309a21) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64K for second CPU: sound */
	ROM_LOAD( "sb2.bin",  0x0000, 0x8000, CRC(e6a6d9b7) SHA1(35d228d13d4305f606fdd84adad1d6e435f4b7ce) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD("sb10.bin",  0x00000, 0x08000, CRC(36dea4bf) SHA1(60095f482af4595a39be5ae6def8cd30298c1ef8) )    /* chars */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sb9.bin",  0x00000, 0x08000, CRC(b567e85e) SHA1(7036792ea70ad48384f348399ed9b136272fedb6) )    /* bg tiles */
	ROM_LOAD( "sb5.bin",  0x08000, 0x08000, CRC(b0eae4ba) SHA1(baee3fcb1399c56efaa5f97912de324d7b38f286) )
	ROM_LOAD( "sb8.bin",  0x10000, 0x08000, CRC(d2bfbdb6) SHA1(b552b055450f438729c83337f561d05b6518ae75) )
	ROM_LOAD( "sb4.bin",  0x18000, 0x08000, CRC(1d23a130) SHA1(aabf7c46f9299ffb8b8ca92839622d000a470a0b) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_INVERT | ROMREGION_DISPOSE )
	ROM_LOAD( "sb7.bin",  0x00000, 0x08000, CRC(9f1b33d1) SHA1(1f8be8f8e6a2ee99a7dafeead142ccc629fa792d) )   /* sprites */
	ROM_LOAD( "sb6.bin",  0x08000, 0x08000, CRC(0e2506eb) SHA1(56f779266b977819063c475b84ca246fc6d8d6a7) )
ROM_END


ROM_START( musicbal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "01.bin",  0x0000,  0x8000, CRC(412298a2) SHA1(3c3247b466880cd78dd7f7f73911f475352c15df) )
	ROM_LOAD( "03.bin",  0x8000,  0x8000, CRC(fdf14446) SHA1(9e52810ebc2b18d83f349fb78884b3c380d93903) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "02.bin",  0x0000,  0x8000, CRC(b7d3840d) SHA1(825289c3ca51284a47cfc4937a18d098183c396a) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "10.bin",  0x00000, 0x08000, CRC(5afd3c42) SHA1(5b9a44ef03e5519c9601bb636eb26768cf800278) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "09.bin",  0x00000, 0x08000, CRC(dcde4233) SHA1(99f204ddc97ee45330ea3cb0bc2971cd95f8e1ac) )
	ROM_LOAD( "05.bin",  0x08000, 0x08000, CRC(e1eec437) SHA1(ce77f3bb01db80ec69da9639d193d8656f9c6692) )
	ROM_LOAD( "08.bin",  0x10000, 0x08000, CRC(7e7af52b) SHA1(3bb1c5abfb1fe53f01520e93124708df6750d8b5) )
	ROM_LOAD( "04.bin",  0x18000, 0x08000, CRC(bf931a33) SHA1(b2ab5c6103af0e0508f08fd58b425e5acfe9ef8a) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_INVERT | ROMREGION_DISPOSE )
	ROM_LOAD( "07.bin",  0x00000, 0x08000, CRC(310e1e23) SHA1(290f3e1c7b907165fe60a4ebe7a8b04b2451b3b1) )
	ROM_LOAD( "06.bin",  0x08000, 0x08000, CRC(2e7772f8) SHA1(caded1a72356501282e627e23718c30cb8f09370) )
ROM_END

static DRIVER_INIT( musicbal )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	static const UINT8 xor_table[8] = { 0x05, 0x06, 0x84, 0x84, 0x00, 0x87, 0x84, 0x84 };
	static const int swap_table[4][4] =
	{
		{ 1, 0, 7, 2 },
		{ 2, 7, 0, 1 },
		{ 7, 2, 1, 0 },
		{ 0, 2, 1, 7 }
	};
	int i;

	/* Address lines select the data permutation only in the first program ROM. */
	for (i = 0; i < 0x8000; ++i)
	{
		int index = BIT(i, 3) | (BIT(i, 5) << 1) | (BIT(i, 9) << 2);
		const int *swap = swap_table[xor_table[index] & 3];
		rom[i] = BITSWAP8(rom[i], swap[3], 6, 5, 4, 3, swap[2], swap[1], swap[0]) ^ xor_table[index];
	}
}

GAME( 1987, speedbal, 0, speedbal, speedbal, 0, ROT270, "Tecfri", "Speed Ball", 0 )
GAME( 1988, musicbal, 0, speedbal, musicbal, musicbal, ROT270, "Tecfri / Desystem S.A.", "Music Ball", 0 )

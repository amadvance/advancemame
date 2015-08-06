/*
    'Swinging Singles' by Yachiyo

    xxx game
    very preliminary driver by Tomasz Slanina

    Bad dump ?
    upper half of 7.bin = upper half of 8.bin

*/

#include "driver.h"

VIDEO_START(ssingles)
{
	return 0;
}

VIDEO_UPDATE(ssingles)
{

}

static ADDRESS_MAP_START( ssingles_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x6000, 0xbfff) AM_ROM
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ssingles_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITENOP //e 3f - audio 1 (volume?)
	AM_RANGE(0x04, 0x04) AM_WRITENOP

	AM_RANGE(0x06, 0x06) AM_WRITENOP //e 3f - audio 2 (volume?)
	AM_RANGE(0x0a, 0x0a) AM_WRITENOP

	AM_RANGE(0x16, 0x16) AM_READNOP // $6049 - dips ?
	AM_RANGE(0x18, 0x18) AM_READNOP // $604e - dips ?
	AM_RANGE(0x1c, 0x1c) AM_READNOP // $635c - coin(s) ? (inc+daa after read)

	AM_RANGE(0x1a, 0x1a) AM_WRITENOP

	AM_RANGE(0xfe, 0xfe) AM_WRITENOP // index ? (0-f)
	AM_RANGE(0xff, 0xff) AM_WRITENOP // data ?
ADDRESS_MAP_END


INPUT_PORTS_START( ssingles )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ 0, RGN_FRAC(1,2) },
	{ 0,1,2,3,4,5,6,7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8
};

static const gfx_layout charlayout2 =
{
	16,16,
	RGN_FRAC(1,4),
	2,
	{ 0, RGN_FRAC(1,2) },
	{ 0,1,2,3,4,5,6,7,RGN_FRAC(1,4),RGN_FRAC(1,4)+ 1,RGN_FRAC(1,4)+ 2,RGN_FRAC(1,4)+ 3,RGN_FRAC(1,4)+ 4,RGN_FRAC(1,4)+ 5,RGN_FRAC(1,4) + 6,RGN_FRAC(1,4) + 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	8*8*2
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout2,   0x0000, 1 },
	{ REGION_GFX1, 0, &charlayout,    0x0000, 1 },
	{ -1 }
};

static MACHINE_DRIVER_START( ssingles )
	MDRV_CPU_ADD(Z80,4000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(ssingles_map,0)
	MDRV_CPU_IO_MAP(ssingles_io_map,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)

	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256*200)

	MDRV_VIDEO_START(ssingles)
	MDRV_VIDEO_UPDATE(ssingles)
MACHINE_DRIVER_END

ROM_START( ssingles )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 main CPU  */
	ROM_LOAD( "1.bin", 0x00000, 0x2000, CRC(43f02215) SHA1(9f04a7d4671ff39fd2bd8ec7afced4981ee7be05) )

	ROM_LOAD( "2.bin", 0x06000, 0x2000, CRC(281f27e4) SHA1(cef28717ab2ed991a5709464c01490f0ab1dc17c) )
	ROM_LOAD( "3.bin", 0x08000, 0x2000, CRC(14fdcb65) SHA1(70f7fcb46e74937de0e4037c9fe79349a30d0d07) )
	ROM_LOAD( "4.bin", 0x0a000, 0x2000, CRC(acb44685) SHA1(d68aab8b7e68d842a350d3fb76985ac857b1d972) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 )
	ROM_LOAD( "9.bin",  0x0000, 0x4000, CRC(57fac6f9) SHA1(12f6695c9831399e599a95008ebf9db943725437) )
	ROM_LOAD( "10.bin", 0x4000, 0x4000, CRC(cd3ba260) SHA1(2499ad9982cc6356e2eb3a0f10d77886872a0c9f) )
	ROM_LOAD( "11.bin", 0x8000, 0x4000, CRC(f7107b29) SHA1(a405926fd3cb4b3d2a1c705dcde25d961dba5884) )
	ROM_LOAD( "12.bin", 0xc000, 0x4000, CRC(e5585a93) SHA1(04d55699b56d869066f2be2c6ac48042aa6c3108) )

	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* code ? data ? */
	ROM_LOAD( "5.bin", 0x00000, 0x2000, CRC(242a8dda) SHA1(e140893cc05fb8cee75904d98b02626f2565ed1b) )
	ROM_LOAD( "6.bin", 0x02000, 0x2000, CRC(85ab8aab) SHA1(566f034e1ba23382442f27457447133a0e0f1cfc) )
	ROM_LOAD( "7.bin", 0x04000, 0x2000, CRC(57cc112d) SHA1(fc861c58ae39503497f04d302a9f16fca19b37fb) )
	ROM_LOAD( "8.bin", 0x06000, 0x2000, CRC(52de717a) SHA1(e60399355165fb46fac862fb7fcdff16ff351631) )

ROM_END

GAME ( 19??, ssingles, 0, ssingles, ssingles, 0, ROT90, "Yachiyo", "Swinging Singles",GAME_NOT_WORKING|GAME_NO_SOUND )

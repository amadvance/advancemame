// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
    Space Cyclone

    3 board stack? - the PCB pictures make it very difficult to figure much out

    seems a bit like 8080bw hardware but with extra sprites and a Z80 cpu?
    maybe an evolution of Polaris etc.?


    there's an MB14241 near the Sprite ROM

    1x 8DSW, 1x 4DSW

    there are 11 HM4716AP RAM chips near the main CPU (1 is unmarked, could be different?)
    HM4716AP is 16384 * 1-bit (2048 [0x800] bytes)

    1bpp of video needs 0x3800 bytes

    1bpp video would need 4 of these (with 0x400 bytes leftover)
    2bpp video would need 7 of these
    3bpp video would need 11 of these (with 0x400 bytes leftover)
    mainram is 0x400 bytes (so the leftover in above calc?)


    Notes:

    (to check on hardware)
    The stars scroll backwards when in cocktail mode, the direction changes when the screen
    is flipped but is still reversed compared to upright.  We would have no way of knowing
    if we're in cocktail mode without checking the dipswitch, so I think this is a game bug.

    Furthermore, the game seems to set what I believe to be the 'flipscreen' bit for player 2
    even when in 'upright' mode, so it's possible this romset was only really made for a
    cocktail table?

    The speech chip is unknown, it seems like it's some sort of 1-bit delta modulation.
    Probably HC55516 or something similar.
*/

#include "driver.h"
#include "sound/dac.h"
#include "sound/hc55516.h"

#define VRAM_SIZE 0x1c00 /* bytes per plane */
#define SPRITE_SIZE 32

static UINT8 vram[3][VRAM_SIZE];

static UINT8 videowritemask;
static UINT8 videowritemask2;
static UINT8 sprite_xpos;
static UINT8 sprite_ypos;
static UINT8 sprite_colour;
static UINT8 sprite_tile;
static UINT8 starscroll;
static UINT8 hascollided;
static UINT32 frame_count;
static UINT8 speech_data;
static UINT16 shift_data;
static UINT8 shift_count;


/***************************************************************************

  Video hardware

***************************************************************************/

static PALETTE_INIT( scyclone )
{
	int i;

	for (i = 0; i < 8; i++)
		palette_set_color(i, pal1bit(i >> 0), pal1bit(i >> 1), pal1bit(i >> 2));

	/* only the first 16 bytes of the PROM are used */
	for (i = 0; i < 16; i++)
	{
		UINT8 col = color_prom[i];

		palette_set_color(8 + i, pal1bit(col >> 2), pal1bit(col >> 1), pal1bit(col >> 0));
	}
}

static VIDEO_START( scyclone )
{
	return 0;
}

static UINT8 get_sprite_pixel(int x, int y)
{
	int minx = 0xe0 - sprite_xpos;
	int miny = 0xe0 - sprite_ypos;
	int code = sprite_tile & 7;
	const gfx_element *gfx = Machine->gfx[0];

	if (x < minx || x >= minx + SPRITE_SIZE || y < miny || y >= miny + SPRITE_SIZE)
		return 0;

	return gfx->gfxdata[code * gfx->char_modulo + (y - miny) * gfx->line_modulo + (x - minx)];
}

static UINT8 get_bitmap_pixel(int x, int y)
{
	int offs = y * 32 + (x / 8);
	int bit = x & 7;

	return ((vram[0][offs] >> bit) & 1) | (((vram[1][offs] >> bit) & 1) << 1) | (((vram[2][offs] >> bit) & 1) << 2);
}

static void update_collision(void)
{
	int minx = 0xe0 - sprite_xpos;
	int miny = 0xe0 - sprite_ypos;
	int x, y;

	hascollided = 0;

	/* a white bitmap pixel under a sprite pixel */
	for (y = miny; y < miny + SPRITE_SIZE; y++)
	{
		if (y < 0 || y >= 224)
			continue;

		for (x = minx; x < minx + SPRITE_SIZE; x++)
		{
			if (x < 0 || x >= 256)
				continue;

			if ((get_sprite_pixel(x, y) & 3) && get_bitmap_pixel(x, y) == 7)
			{
				hascollided = 1;
				return;
			}
		}
	}
}

static VIDEO_UPDATE( scyclone )
{
	int x, y;
	int col = sprite_colour & 3;

	fillbitmap(bitmap, Machine->pens[0], cliprect);

	/* starfield */
	for (x = 0; x < 32; x++)
	{
		for (y = 0; y < 32; y++)
		{
			UINT8 sx = (x + (starscroll >> 3)) & 0x1f;
			UINT8 star = memory_region(REGION_USER1)[y << 5 | sx] & 0xf;
			UINT8 color = ((frame_count + (star << 2)) >> 3) & 7;
			UINT8 dx = x << 3 | (~starscroll & 7);
			UINT8 dy = (y << 3) - 32 + 7;

			if (dx >= 64 && star && dy < 224)
				plot_pixel(bitmap, dx, dy, Machine->pens[color]);
		}
	}

	/* bitmap and sprite */
	for (y = 0; y < 224; y++)
	{
		for (x = 0; x < 256; x++)
		{
			UINT8 pal = get_bitmap_pixel(x, y);
			UINT8 pal2 = get_sprite_pixel(x, y);

			if (pal2 & 3)
				plot_pixel(bitmap, x, y, Machine->pens[8 + pal2 + col * 4]);
			else if (pal)
				plot_pixel(bitmap, x, y, Machine->pens[pal]);
		}
	}
}


/***************************************************************************

  Memory handlers

***************************************************************************/

static READ8_HANDLER( vram_r )
{
	return vram[0][offset] | vram[1][offset] | vram[2][offset];
}

static WRITE8_HANDLER( vram_w )
{
	int plane;

	/* each pair of bits has its own set of 3 plane enables (active low) */
	for (plane = 0; plane < 3; plane++)
	{
		int mask = 1 << plane;
		UINT8 enable = 0;

		if (!((videowritemask >> 4) & mask)) enable |= 0xc0;
		if (!(videowritemask & mask)) enable |= 0x30;
		if (!((videowritemask2 >> 4) & mask)) enable |= 0x0c;
		if (!(videowritemask2 & mask)) enable |= 0x03;

		vram[plane][offset] = data & enable;
	}
}

static WRITE8_HANDLER( videomask1_w )
{
	videowritemask = data;
}

static WRITE8_HANDLER( videomask2_w )
{
	videowritemask2 = data;
}

static WRITE8_HANDLER( sprite_xpos_w )
{
	sprite_xpos = data;
}

static WRITE8_HANDLER( sprite_ypos_w )
{
	sprite_ypos = data;
}

static WRITE8_HANDLER( sprite_colour_w )
{
	sprite_colour = data;
}

static WRITE8_HANDLER( sprite_tile_w )
{
	sprite_tile = data;
}

static WRITE8_HANDLER( starscroll_w )
{
	starscroll = data;
}

static WRITE8_HANDLER( shift_count_w )
{
	shift_count = ~data & 0x07;
}

static WRITE8_HANDLER( shift_data_w )
{
	shift_data = (shift_data >> 8) | ((UINT16)data << 7);
}

static READ8_HANDLER( shift_result_r )
{
	return (shift_data >> shift_count) & 0xff;
}

static READ8_HANDLER( in1_r )
{
	return input_port_2_r(0) | hascollided;
}

static INTERRUPT_GEN( main_irq )
{
	++frame_count;
	update_collision();

	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xd7); /* RST 10h */
}


/***************************************************************************

  Sound hardware

***************************************************************************/

static WRITE8_HANDLER( snd_3001_w )
{
	soundlatch_clear_w(0, 0);
}

static WRITE8_HANDLER( snd_3002_w )
{
	speech_data = data;
}

static WRITE8_HANDLER( snd_3003_w )
{
}

static WRITE8_HANDLER( snd_3004_w )
{
}

static WRITE8_HANDLER( snd_3005_w )
{
}

static INTERRUPT_GEN( sub_irq )
{
	hc55516_clock_w(0, 0);
	hc55516_digit_w(0, (speech_data >> 7) & 1);
	hc55516_clock_w(0, 1);
	speech_data <<= 1;

	cpunum_set_input_line(1, 0, HOLD_LINE);
}


/***************************************************************************

  Address maps

***************************************************************************/

static ADDRESS_MAP_START( scyclone_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0x4400, 0x5fff) AM_READWRITE(vram_r, vram_w)
	AM_RANGE(0x6000, 0x60ff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( scyclone_iomap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(shift_result_r, shift_count_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(input_port_1_r, shift_data_w)
	AM_RANGE(0x02, 0x02) AM_READ(in1_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)
	AM_RANGE(0x04, 0x04) AM_WRITE(sprite_xpos_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(sprite_ypos_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x08, 0x08) AM_WRITE(sprite_colour_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(sprite_tile_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(starscroll_w)
	AM_RANGE(0x0e, 0x0e) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x0f, 0x0f) AM_WRITE(soundlatch_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(videomask1_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(videomask2_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( scyclone_sub_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x3000, 0x3000) AM_READWRITE(soundlatch_r, DAC_0_data_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(snd_3001_w)
	AM_RANGE(0x3002, 0x3002) AM_WRITE(snd_3002_w)
	AM_RANGE(0x3003, 0x3003) AM_WRITE(snd_3003_w)
	AM_RANGE(0x3004, 0x3004) AM_WRITE(snd_3004_w)
	AM_RANGE(0x3005, 0x3005) AM_WRITE(snd_3005_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( scyclone_sub_iomap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
ADDRESS_MAP_END


/***************************************************************************

  Input ports

***************************************************************************/

INPUT_PORTS_START( scyclone )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* hw collision, merged in by in1_r */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )		PORT_DIPLOCATION("DSW0:1,2")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )		PORT_DIPLOCATION("DSW0:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x08, 0x00, "Disable Collision (Buggy!)" )	PORT_DIPLOCATION("DSW0:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Show Copyright Date" )	PORT_DIPLOCATION("DSW0:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )		PORT_DIPLOCATION("DSW0:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )		PORT_DIPLOCATION("DSW0:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )		PORT_DIPLOCATION("DSW0:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************

  GFX Layouts

***************************************************************************/

static const gfx_layout tiles32x32_layout =
{
	32,32,
	8,
	2,
	{ 0, 4 },
	{
		0,      1,      2,      3,      256+0,  256+1,  256+2,  256+3,
		512+0,  512+1,  512+2,  512+3,  768+0,  768+1,  768+2,  768+3,
		1024+0, 1024+1, 1024+2, 1024+3, 1280+0, 1280+1, 1280+2, 1280+3,
		1536+0, 1536+1, 1536+2, 1536+3, 1792+0, 1792+1, 1792+2, 1792+3
	},
	{
		0*8,  1*8,  2*8 , 3*8,  4*8,  5*8,  6*8,  7*8,
		8*8,  9*8,  10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
		16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
		24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8
	},
	8*32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles32x32_layout, 8, 4 },
	{ -1 }
};


/***************************************************************************

  Machine configs

***************************************************************************/

static MACHINE_DRIVER_START( scyclone )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000/2)
	MDRV_CPU_PROGRAM_MAP(scyclone_map, 0)
	MDRV_CPU_IO_MAP(scyclone_iomap, 0)
	MDRV_CPU_VBLANK_INT(main_irq, 1)

	MDRV_CPU_ADD(Z80, 5000000/2)
	MDRV_CPU_PROGRAM_MAP(scyclone_sub_map, 0)
	MDRV_CPU_IO_MAP(scyclone_sub_iomap, 0)
	MDRV_CPU_PERIODIC_INT(sub_irq, TIME_IN_HZ(14400))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-32-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8 + 4*4)

	MDRV_PALETTE_INIT(scyclone)
	MDRV_VIDEO_START(scyclone)
	MDRV_VIDEO_UPDATE(scyclone)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(HC55516, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)
MACHINE_DRIVER_END


/***************************************************************************

  ROM definitions

***************************************************************************/

ROM_START( scyclone )
	ROM_REGION( 0x3000, REGION_CPU1, 0 )
	ROM_LOAD( "de07.1h.2716", 0x0000, 0x0800, CRC(fa9c93a3) SHA1(701a3196ee67533d69f1264e14085d04dfb1b915) )
	ROM_LOAD( "de08.3h.2716", 0x0800, 0x0800, CRC(bdff3e77) SHA1(e3fc6840348fa6df834cb1fb8b3832db05b5c585) )
	ROM_LOAD( "de09.4h.2716", 0x1000, 0x0800, CRC(eed05875) SHA1(e00a1b2d2d723c2deedf5f5e3159936a5d27891d) )
	ROM_LOAD( "de10.5h.2716", 0x1800, 0x0800, CRC(71301be6) SHA1(70c1983d256a6a9e18c7a15b93f5e3781d884fdb) )
	ROM_LOAD( "de11.6h.2716", 0x2000, 0x0800, CRC(d241db51) SHA1(53c6489d715baf5f03032a7ac367a6af64bce040) )
	ROM_LOAD( "de12.7h.2716", 0x2800, 0x0800, CRC(208e3e9a) SHA1(07f0e6c5417584eef171d8dcad83d741f88c9348) )

	ROM_REGION( 0x2000, REGION_CPU2, 0 )
	ROM_LOAD( "de18.ic1.2716",  0x0000, 0x0800, CRC(182a5c24) SHA1(0ba29b6b3e7ec97c0b0748a6e60bb1a737dc55fa) )
	ROM_LOAD( "de04.ic9.2716",  0x0800, 0x0800, CRC(098269ac) SHA1(138af858bbbd73380086f971bcdd52ae47e2b667) )
	ROM_LOAD( "de05.ic14.2716", 0x1000, 0x0800, CRC(878f68b2) SHA1(2c2fa1b9053664ec26452ab0b35e2ae550601cc9) )
	ROM_LOAD( "de06.ic19.2716", 0x1800, 0x0800, CRC(0b1ead90) SHA1(38322b39f4420408c223cdbed3b75692a3d70746) )

	ROM_REGION( 0x0800, REGION_GFX1, 0 ) /* 32x32x2 sprites */
	ROM_LOAD( "de01.11c.2716", 0x0000, 0x0800, CRC(bf770730) SHA1(1d0f9235b0618e3f4dd6db47efbdf92e2b00f5f6) )

	ROM_REGION( 0x0440, REGION_PROMS, 0 )
	ROM_LOAD( "de02.5b.82s137", 0x0000, 0x0400, CRC(fe4b278c) SHA1(c03080ab4d3fb84b8eec7087b925d1a1d8565fcc) ) /* only 16 bytes of this are actually used */
	ROM_LOAD( "de16.4a.82s123", 0x0400, 0x0020, CRC(5178e9c5) SHA1(dd2f81894069282f37feae21c5cfacf50f77dcd5) )
	ROM_LOAD( "de17.2e.82s123", 0x0420, 0x0020, CRC(3c8572e4) SHA1(c908c4ed99828fff576c3d0963cd8b99edeb993b) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* starfield bitmap */
	ROM_LOAD( "de15.3a.82s137", 0x0000, 0x0400, CRC(c2816161) SHA1(2d0e7b4dbdb2af1481a4b7e45b1f9e3640f69aec) )
ROM_END

GAME( 1980, scyclone, 0, scyclone, scyclone, 0, ROT270, "Taito Corporation", "Space Cyclone (Japan)", GAME_NO_COCKTAIL | GAME_IMPERFECT_SOUND )

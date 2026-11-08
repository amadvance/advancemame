// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
    Poke Champ (D.G.R.M., 1995)
    Pocket Gal-derived hardware with 8bpp tiles, RAM palette and OKIM6295 samples.
*/

#include "driver.h"
#include "sound/2203intf.h"
#include "sound/3812intf.h"
#include "sound/okim6295.h"

static tilemap *background;
static int sample_bank;

static void get_tile_info(int tile_index)
{
	int attr = videoram[tile_index * 2];
	int code = videoram[tile_index * 2 + 1] | ((attr & 63) << 8);
	SET_TILE_INFO(0, code, attr >> 6, 0)
}

static WRITE8_HANDLER( video_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(background, offset / 2);
}

static WRITE8_HANDLER( flip_w )
{
	flip_screen_set(data & 0x80);
}

static PALETTE_INIT( pokechmp )
{
	int i;
	/* Palette addressing has ten bits, including when an 8bpp color bank wraps. */
	for (i = 0; i < 0x500; ++i)
		colortable[i] = i & 0x3ff;
}

static VIDEO_START( pokechmp )
{
	background = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);
	return background ? 0 : 1;
}

static VIDEO_UPDATE( pokechmp )
{
	int offset;
	tilemap_draw(bitmap, cliprect, background, 0, 0);
	for (offset = 0; offset < spriteram_size; offset += 4)
	{
		int attr = spriteram[offset + 1];
		int x, y, fx, fy, code;
		if (spriteram[offset] == 0xf8)
			continue;
		x = 240 - spriteram[offset + 2];
		y = 240 - spriteram[offset];
		fx = (attr >> 2) & 1;
		fy = (attr >> 1) & 1;
		if (flip_screen)
		{
			x = 240 - x;
			y = 240 - y;
			fx = !fx;
			fy = !fy;
		}
		code = spriteram[offset + 3] | ((attr & 1) << 8) | ((attr & 8) << 6);
		drawgfx(bitmap, Machine->gfx[1], code, attr >> 4, fx, fy, x, y,
			cliprect, TRANSPARENCY_PEN, 0);
	}
}

static WRITE8_HANDLER( main_bank_w )
{
	static const int bases[4] = { 0, 0x4000, 0x10000, 0x14000 };
	memory_set_bankptr(1, memory_region(REGION_CPU1) + bases[data & 3]);
}

static WRITE8_HANDLER( sample_bank_w )
{
	int bank = data & 15;
	if (bank != sample_bank)
	{
		/* Flush queued audio while it still sees the old 32 KB sample window. */
		OKIM6295_set_bank_base(0, 0);
		memcpy(memory_region(REGION_SOUND1) + 0x38000,
			memory_region(REGION_USER1) + bank * 0x8000, 0x8000);
		sample_bank = bank;
	}
}

static WRITE8_HANDLER( sound_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
	cpu_boost_interleave(TIME_IN_USEC(10), TIME_IN_MSEC(1));
}

static READ8_HANDLER( sound_r )
{
	UINT8 data = soundlatch_r(0);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
	return data;
}

static INTERRUPT_GEN( vblank_interrupt )
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x0fff) AM_READWRITE(MRA8_RAM, video_w) AM_BASE(&videoram)
	AM_RANGE(0x1000, 0x11ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x1800, 0x1800) AM_READ(input_port_0_r)
	AM_RANGE(0x1801, 0x1801) AM_WRITE(flip_w)
	AM_RANGE(0x1802, 0x181f) AM_WRITENOP
	AM_RANGE(0x1a00, 0x1a00) AM_READWRITE(input_port_1_r, sound_w)
	AM_RANGE(0x1c00, 0x1c00) AM_READWRITE(input_port_2_r, main_bank_w)
	AM_RANGE(0x2000, 0x23ff) AM_READWRITE(MRA8_RAM, paletteram_xBBBBBGGGGGRRRRR_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0x2400, 0x27ff) AM_READWRITE(MRA8_RAM, paletteram_xBBBBBGGGGGRRRRR_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION(REGION_CPU1, 0x18000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( audio_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x0800) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x0801, 0x0801) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x1000, 0x1000) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0x1001, 0x1001) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x1800, 0x1800) AM_WRITENOP
	AM_RANGE(0x2000, 0x2000) AM_WRITE(sample_bank_w)
	AM_RANGE(0x2800, 0x2800) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x3000, 0x3000) AM_READ(sound_r)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static MACHINE_RESET( pokechmp )
{
	main_bank_w(0, 0);
	sample_bank = -1;
	sample_bank_w(0, 8);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
	flip_screen_set(0);
}

INPUT_PORTS_START( pokechmp )
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )      PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )  PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow 2 Players Game" )  PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x20, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )        PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout pokechmp_charlayout =
{
	8,8,
	RGN_FRAC(1,8),
	8,

	{ RGN_FRAC(1,8), RGN_FRAC(3,8),RGN_FRAC(0,8),RGN_FRAC(5,8),RGN_FRAC(2,8),RGN_FRAC(7,8),RGN_FRAC(4,8),RGN_FRAC(6,8) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout pokechmp_spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{RGN_FRAC(0,4),RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4)},
	{ 128+7, 128+6, 128+5, 128+4, 128+3, 128+2, 128+1, 128+0, 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pokechmp_charlayout, 0x100, 4 },
	{ REGION_GFX2, 0, &pokechmp_spritelayout, 0, 32 },
	{ -1 }
};

static MACHINE_DRIVER_START( pokechmp )
	MDRV_CPU_ADD(M6502, 4000000 / 4)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(vblank_interrupt, 1)
	MDRV_CPU_ADD(M6502, 4000000 / 4)
	MDRV_CPU_PROGRAM_MAP(audio_map, 0)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_RESET(pokechmp)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_COLORTABLE_LENGTH(1280)
	MDRV_PALETTE_INIT(pokechmp)
	MDRV_VIDEO_START(pokechmp)
	MDRV_VIDEO_UPDATE(pokechmp)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2203, 4000000 / 4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
	MDRV_SOUND_ADD(YM3812, 24000000 / 16)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD(OKIM6295, 1500000 / 165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( pokechmp )
	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "pokechamp_11_27010.bin", 0x00000, 0x20000, CRC(9afb6912) SHA1(e45da9524e3bb6f64a68200b70d0f83afe6e4379) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pokechamp_09_27c512.bin", 0x00000, 0x10000, CRC(c78f6483) SHA1(a0d063effd8d1850f674edccb6e7a285b2311d21) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pokechamp_05_27c020.bin",       0x00000, 0x40000, CRC(554cfa42) SHA1(862d0dd83697da7bd52dc640c34926c62691afea) )
	ROM_LOAD( "pokechamp_06_27c020.bin",       0x40000, 0x40000, CRC(00bb9536) SHA1(1a5584297ebb425d6ce331955e0c6a4f467cd1e6) )
	ROM_LOAD( "pokechamp_07_27c020.bin",       0x80000, 0x40000, CRC(4b15ab5e) SHA1(5523134853b9ea1c81fd5aeb58061376d94e9298) )
	ROM_LOAD( "pokechamp_08_27c020.bin",       0xc0000, 0x40000, CRC(e9db54d6) SHA1(ac3b7c06d0f61847bf9bc6147f2f88d712f2b4b3) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )

	ROM_LOAD( "pokechamp_02_27c512.bin",       0x00000, 0x08000, CRC(1ff44545) SHA1(2eee44484accce7b0ba21babf6e8344b234a4e87) ) ROM_CONTINUE( 0x00000, 0x8000 )
	ROM_LOAD( "pokechamp_01_27c512.bin",       0x08000, 0x08000, CRC(338fc412) SHA1(bb8ae99ee6a399a8c67bedb88d0837fd0a4a426c) ) ROM_CONTINUE( 0x08000, 0x8000 )
	ROM_LOAD( "pokechamp_04_27c512.bin",       0x10000, 0x08000, CRC(ee6991af) SHA1(8eca3cdfd2eb74257253957a87b245b7f85bd038) ) ROM_CONTINUE( 0x10000, 0x8000 )
	ROM_LOAD( "pokechamp_03_27c512.bin",       0x18000, 0x08000, CRC(99f9884a) SHA1(096d6ce70dc51fb9142e80e1ec45d6d7225481f5) ) ROM_CONTINUE( 0x18000, 0x8000 )

	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "pokechamp_10_27c040.bin",       0x00000, 0x80000, CRC(b54806ed) SHA1(c6e1485c263ebd9102ff1e8c09b4c4ca5f63c3da) )
ROM_END

static DRIVER_INIT( pokechmp )
{
	memcpy(memory_region(REGION_SOUND1), memory_region(REGION_USER1), 0x38000);
}

GAME( 1995, pokechmp, 0, pokechmp, pokechmp, pokechmp, ROT0, "D.G.R.M.", "Poke Champ (set 1)", 0 )

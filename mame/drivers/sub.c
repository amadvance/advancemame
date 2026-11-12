// license:BSD-3-Clause
// copyright-holders: Angelo Salese, David Haywood
/*************************************************************************************

Submarine (c) 1985 Sigma

driver by David Haywood & Angelo Salese

TODO:
- finish dip-switches;
- a bunch of unemulated writes at 0xe***

PCB1 - video board: 3 tile ROMs, 3 sprite ROMs, color PROMs.
PCB2 - CPU board (F-020): main Z80, sound Z80, two AY8910, 18.432 MHz.

*************************************************************************************/

#include "driver.h"
#include "sndhrdw/generic.h"
#include "sound/ay8910.h"

#define MASTER_CLOCK 18432000

static UINT8 *sub_vram, *sub_attr, *sub_spriteram1, *sub_spriteram2, *sub_scrolly;
static tilemap *sub_tilemap;
static int int_en, nmi_en;

static PALETTE_INIT( sub )
{
	const UINT8 *proms = memory_region(REGION_PROMS);
	const UINT8 *lookup = memory_region(REGION_USER1);
	int i;

	for (i = 0; i < 0x100; ++i)
	{
		int r = proms[i | 0x000] & 0x0f;
		int g = proms[i | 0x100] & 0x0f;
		int b = proms[i | 0x200] & 0x0f;

		palette_set_color(i, r * 0x11, g * 0x11, b * 0x11);
	}

	for (i = 0; i < 0x400; ++i)
		colortable[i] = lookup[i | 0x400] | (lookup[i | 0x000] << 4);
}

static void get_tile_info(int tile_index)
{
	int code = sub_vram[tile_index] | ((sub_attr[tile_index] & 0xe0) << 3);
	int color = (sub_attr[tile_index] & 0x1f) + 0x40;

	SET_TILE_INFO(0, code, color, 0)
}

static WRITE8_HANDLER( sub_vram_w )
{
	sub_vram[offset] = data;
	tilemap_mark_tile_dirty(sub_tilemap, offset & 0x3ff);
}

static WRITE8_HANDLER( sub_attr_w )
{
	sub_attr[offset] = data;
	tilemap_mark_tile_dirty(sub_tilemap, offset & 0x3ff);
}

static WRITE8_HANDLER( sub_scrolly_w )
{
	sub_scrolly[offset] = data;
	tilemap_set_scrolly(sub_tilemap, offset, data);
}

static VIDEO_START( sub )
{
	sub_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);
	if (!sub_tilemap)
		return 1;
	tilemap_set_scroll_cols(sub_tilemap, 32);
	return 0;
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int i;

	for (i = 0; i < 0x40; i += 2)
	{
		int spr_offs = sub_spriteram1[i + 1];
		int x = sub_spriteram1[i + 0];
		int y = 0xe0 - sub_spriteram2[i + 1];
		int col = sub_spriteram2[i + 0] & 0x3f;
		int dx = (sub_spriteram2[i + 0] & 0x80) ? 0 : 1;
		int fy = (sub_spriteram2[i + 0] & 0x40) ? 0 : 1;
		int fx = 0;

		if (flip_screen)
		{
			x = 0xe0 - x;
			fx = 1;
		}

		if (dx)
			x = 0xe0 - x;

		drawgfx(bitmap, Machine->gfx[1], spr_offs, col, fx, fy, (UINT8)x, (UINT8)y, cliprect, TRANSPARENCY_PEN, 0);
	}
}

static VIDEO_UPDATE( sub )
{
	rectangle opaque_rect = *cliprect;

	tilemap_draw(bitmap, cliprect, sub_tilemap, 0, 0);
	draw_sprites(bitmap, cliprect);

	/* re-draw score display above the sprites (window effect) */
	if (flip_screen)
		opaque_rect.max_x = cliprect->min_x + 32;
	else
		opaque_rect.min_x = cliprect->max_x - 32;
	tilemap_draw(bitmap, &opaque_rect, sub_tilemap, 0, 0);
}

static INTERRUPT_GEN( sub_vblank_irq )
{
	if (int_en)
		cpunum_set_input_line(0, 0, ASSERT_LINE);
}

static INTERRUPT_GEN( sub_sound_irq )
{
	if (nmi_en)
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

/* 74LS259 latch at e800 */
static WRITE8_HANDLER( sub_mainlatch_w )
{
	data &= 1;
	switch (offset)
	{
		case 0:
			int_en = data;
			if (!int_en)
				cpunum_set_input_line(0, 0, CLEAR_LINE);
			break;
		case 1:
			flip_screen_set(!data);
			break;
	}
}

static WRITE8_HANDLER( sub_nmi_mask_w )
{
	nmi_en = data & 1;
}

static UINT8 sound_latch_data;

static void sub_latch_callback(int param)
{
	/* data and interrupt line must change together, in sync with the sound CPU */
	sound_latch_data = param;
	cpunum_set_input_line(1, 0, ASSERT_LINE);
}

static WRITE8_HANDLER( sub_main_latch_w )
{
	timer_set(TIME_NOW, data, sub_latch_callback);
}

static READ8_HANDLER( sub_sound_latch_r )
{
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	return sound_latch_data;
}

static MACHINE_RESET( sub )
{
	int_en = 0;
	nmi_en = 0;
}

static ADDRESS_MAP_START( main_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xafff) AM_READ(MRA8_ROM)
	AM_RANGE(0xb000, 0xbfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xd03f) AM_READ(MRA8_RAM)
	AM_RANGE(0xd800, 0xd85f) AM_READ(MRA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_READ(input_port_2_r)
	AM_RANGE(0xf020, 0xf020) AM_READ(input_port_3_r)
	AM_RANGE(0xf040, 0xf040) AM_READ(input_port_0_r)
	AM_RANGE(0xf060, 0xf060) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xafff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xb000, 0xbfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc000, 0xc3ff) AM_WRITE(sub_attr_w) AM_BASE(&sub_attr)
	AM_RANGE(0xc400, 0xc7ff) AM_WRITE(sub_vram_w) AM_BASE(&sub_vram)
	AM_RANGE(0xd000, 0xd03f) AM_WRITE(MWA8_RAM) AM_BASE(&sub_spriteram1)
	AM_RANGE(0xd800, 0xd83f) AM_WRITE(MWA8_RAM) AM_BASE(&sub_spriteram2)
	AM_RANGE(0xd840, 0xd85f) AM_WRITE(sub_scrolly_w) AM_BASE(&sub_scrolly)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xe800, 0xe807) AM_WRITE(sub_mainlatch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(sub_main_latch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(sub_nmi_mask_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(sub_sound_latch_r)
	AM_RANGE(0x40, 0x41) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x80, 0x81) AM_READ(AY8910_read_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(soundlatch2_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x41, 0x41) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x81, 0x81) AM_WRITE(AY8910_write_port_1_w)
ADDRESS_MAP_END

INPUT_PORTS_START( sub )
	PORT_START	/* SYSTEM */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "DSWC" )
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
	PORT_DIPNAME( 0xf0, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_1C ) ) // Duplicate
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) ) // Duplicate
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) ) // Duplicate
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) ) // Duplicate
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) ) // Duplicate
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )  // separate controls for each player
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) ) // Controls via player 1 for both, but need to get x/y screen flip working to fully test
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8
};

static const gfx_layout tiles16x32_layout =
{
	16,32,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 64+0, 64+1, 64+2, 64+3, 64+4, 64+5, 64+6, 64+7, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 55*8, 54*8, 53*8, 52*8, 51*8, 50*8, 49*8, 48*8,
		39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
		23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
		7*8,  6*8,  5*8,  4*8,  3*8,  2*8,  1*8,  0*8
	},
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout,   0, 0x80 },
	{ REGION_GFX2, 0, &tiles16x32_layout, 0, 0x80 },
	{ -1 }
};

static MACHINE_DRIVER_START( sub )
	MDRV_CPU_ADD(Z80, MASTER_CLOCK / 6)
	MDRV_CPU_PROGRAM_MAP(main_readmem, main_writemem)
	MDRV_CPU_IO_MAP(main_readport, main_writeport)
	MDRV_CPU_VBLANK_INT(sub_vblank_irq, 1)

	MDRV_CPU_ADD(Z80, MASTER_CLOCK / 6)
	MDRV_CPU_PROGRAM_MAP(sound_readmem, sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport, sound_writeport)
	MDRV_CPU_PERIODIC_INT(sub_sound_irq, TIME_IN_HZ(120))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(sub)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(0x400)
	MDRV_PALETTE_INIT(sub)
	MDRV_VIDEO_START(sub)
	MDRV_VIDEO_UPDATE(sub)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK / 6 / 2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.23)
	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK / 6 / 2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.23)
MACHINE_DRIVER_END

ROM_START( sub )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "temp 1 pos b6 27128.bin",  0x0000, 0x4000, CRC(6875b31d) SHA1(e7607e53687f1331cc97de939de144a7954ca3c3) )
	ROM_LOAD( "temp 2 pos c6 27128.bin",  0x4000, 0x4000, CRC(bc7f8f43) SHA1(088156a66acb2214c638d9d1ad18e9836b27eff0) )
	ROM_LOAD( "temp 3 pos d6 2764.bin",   0x8000, 0x2000, CRC(3546c226) SHA1(35e53c0db75c89e8e222d2139b841e77f5cc282c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "m sound pos f14 2764.bin", 0x0000, 0x2000, CRC(61536a97) SHA1(84effc2251bf7c91e0bb670a651117503de8940d) )
	ROM_RELOAD( 0x2000, 0x2000 )

	ROM_REGION( 0xc000, REGION_GFX1, 0 )
	ROM_LOAD( "vram 1 pos f12 27128  version3.bin",   0x0000, 0x4000, CRC(8d176ba0) SHA1(b0bf4af97e991545d6b38e8159eb909376e6df35) )
	ROM_LOAD( "vram 2 pos f14 27128  version3.bin",   0x4000, 0x4000, CRC(0677cf3a) SHA1(072e9391f6a230b78124e820da0f0d27ffa45dc3) )
	ROM_LOAD( "vram 3 pos f15 27128  version3.bin",   0x8000, 0x4000, CRC(9a4cd1a0) SHA1(a321b88386424d73d7d73a7f321317b0f21d2eb6) )

	ROM_REGION( 0xc000, REGION_GFX2, 0 )
	ROM_LOAD( "obj 1 pos h1 27128  version3.bin",     0x0000, 0x4000, CRC(63173e65) SHA1(2be3776c0e08d2c876cfce842e02345389e1fba0) )
	ROM_LOAD( "obj 2 pos h3 27128  version3.bin",     0x4000, 0x4000, CRC(3898d1a8) SHA1(acd3d7695a0fe9faa5e4315032c65e131d24a3ce) )
	ROM_LOAD( "obj 3 pos h4 27128  version3.bin",     0x8000, 0x4000, CRC(304e2145) SHA1(d4eb49b5502872718d64e53f02acd2150f6bf713) )

	ROM_REGION( 0x300, REGION_PROMS, 0 )
	ROM_LOAD( "prom pos a9 n82s129",      0x0200, 0x100, CRC(8df9cefe) SHA1(86320eb8135932d79c4478929b9fd90ffba55712) )
	ROM_LOAD( "prom pos a10 n82s129",     0x0100, 0x100, CRC(3c834094) SHA1(4d681431376a8ed071566d74d4accc737bf965dd) )
	ROM_LOAD( "prom pos a11 n82s129",     0x0000, 0x100, CRC(339afa95) SHA1(ff4ff712960f41c26419a681e8dcceaeef75d2e3) )

	ROM_REGION( 0x800, REGION_USER1, 0 )
	ROM_LOAD( "prom pos e5 n82s131",      0x0000, 0x200, CRC(0024b5dd) SHA1(7d623f8e8964336d643820850cef0fb641e52e22) )
	ROM_LOAD( "prom pos c7 n82s129",      0x0200, 0x100, CRC(9072d259) SHA1(9679fa01372d14a866836c9193204ff6e33cf67c) )
	ROM_LOAD( "prom pos e4 n82s131",      0x0400, 0x200, CRC(307aa2cf) SHA1(839eccf1d34adaf9a5006bfb30e3524bc19a9b41) )
	ROM_LOAD( "prom pos c8 n82s129",      0x0600, 0x100, CRC(351e1ef8) SHA1(530c9012ff5abda1c4ba9787ca999ca1ae1a893d) )
ROM_END


GAME( 1985, sub, 0, sub, sub, 0, ROT270, "Sigma Enterprises Inc.", "Submarine (Sigma)", 0 )

// license:BSD-3-Clause
// copyright-holders:Alberto Salso
/*
ORCA OVG-33C + ORCA OVG-46C

Notes:
- Wolves "teleports" left and right when coming from rocks, btanb.
*/

#include "driver.h"
#include "sound/ay8910.h"

#define MASTER_CLOCK 18432000

static UINT8 *bg0videoram, *bg1videoram, *fgvideoram;
static UINT8 *priority_reg;
static tilemap *fg_tilemap, *bg0_tilemap, *bg1_tilemap;
static int nmi_mask, nmi_sub_mask;
static UINT8 latch_data[2];

/*************************************
 *
 *  Video
 *
 *************************************/

static void get_fg_tile_info(int tile_index)
{
	int code = fgvideoram[tile_index] | (fgvideoram[tile_index + 0x400] << 8);
	int color = fgvideoram[tile_index + 0x800];
	SET_TILE_INFO(0, code, color & 0x3f, TILE_FLIPXY((code & 0xc00) >> 10));
}

static void get_bg0_tile_info(int tile_index)
{
	int code = bg0videoram[tile_index] | (bg0videoram[tile_index + 0x800] << 8);
	int color = bg0videoram[tile_index + 0x400];
	SET_TILE_INFO(4, code, color & 0x3f, TILE_FLIPXY((code & 0xc00) >> 10));
}

static void get_bg1_tile_info(int tile_index)
{
	int code = bg1videoram[tile_index] | (bg1videoram[tile_index + 0x800] << 8);
	int color = bg1videoram[tile_index + 0x400];
	SET_TILE_INFO(3, code, color & 0x3f, TILE_FLIPXY((code & 0xc00) >> 10));
}

static VIDEO_START( akazukin )
{
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	bg0_tilemap = tilemap_create(get_bg0_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	bg1_tilemap = tilemap_create(get_bg1_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if (!fg_tilemap || !bg0_tilemap || !bg1_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);
	tilemap_set_transparent_pen(bg0_tilemap, 0);
	tilemap_set_transparent_pen(bg1_tilemap, 0);

	tilemap_set_scroll_cols(bg0_tilemap, 32);
	tilemap_set_scroll_cols(bg1_tilemap, 32);

	return 0;
}

static WRITE8_HANDLER( fgvideoram_w )
{
	fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset & 0x3ff);
}

static WRITE8_HANDLER( bg0videoram_w )
{
	bg0videoram[offset] = data;
	tilemap_mark_tile_dirty(bg0_tilemap, offset & 0x3ff);
}

static WRITE8_HANDLER( bg1videoram_w )
{
	bg1videoram[offset] = data;
	tilemap_mark_tile_dirty(bg1_tilemap, offset & 0x3ff);
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int rambase, int tilebase)
{
	const UINT8 *spriteram = &fgvideoram[rambase];
	int offs;

	for (offs = 0; offs < 0x10; offs += 2)
	{
		int code = ((spriteram[offs] & 0xfc) >> 2) + ((spriteram[0x400 + offs] & 0x01) << 6) + tilebase;
		int sx = spriteram[offs + 1];
		int sy = spriteram[0x800 + offs];
		int color = spriteram[0x800 + offs + 1] & 0x3f;
		int flipy = spriteram[offs] & 0x02;
		int flipx = spriteram[offs] & 0x01;

		if (flip_screen)
		{
			flipx = !flipx;
			flipy = !flipy;
		}

		if (spriteram[0x400 + offs] & 0x08) /* double width */
		{
			if (!flip_screen)
				sy = 224 - sy;

			drawgfx(bitmap, Machine->gfx[2], code / 2, color, flipx, flipy, sx, sy, cliprect, TRANSPARENCY_PEN, 0);
			drawgfx(bitmap, Machine->gfx[2], code / 2, color, flipx, flipy, sx, sy + 256, cliprect, TRANSPARENCY_PEN, 0);
			drawgfx(bitmap, Machine->gfx[2], code / 2, color, flipx, flipy, sx - 256, sy, cliprect, TRANSPARENCY_PEN, 0);
			drawgfx(bitmap, Machine->gfx[2], code / 2, color, flipx, flipy, sx - 256, sy + 256, cliprect, TRANSPARENCY_PEN, 0);
		}
		else
		{
			if (!flip_screen)
				sy = 240 - sy;

			drawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, sx, sy, cliprect, TRANSPARENCY_PEN, 0);
			drawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, sx - 256, sy, cliprect, TRANSPARENCY_PEN, 0);
		}
	}
}

static void draw_all_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	draw_sprites(bitmap, cliprect, 0x30, 0x80);
	draw_sprites(bitmap, cliprect, 0x10, 0x00);
}

static VIDEO_UPDATE( akazukin )
{
	int i;

	for (i = 0; i < 32; i++)
	{
		tilemap_set_scrolly(bg0_tilemap, i, fgvideoram[0xbe0 + i]);
		tilemap_set_scrolly(bg1_tilemap, i, fgvideoram[0xbc0 + i]);
	}

	switch (*priority_reg)
	{
	case 0:
		tilemap_draw(bitmap, cliprect, bg0_tilemap, TILEMAP_IGNORE_TRANSPARENCY, 0);
		draw_all_sprites(bitmap, cliprect);
		tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
		tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
		break;

	case 1:
		tilemap_draw(bitmap, cliprect, bg0_tilemap, TILEMAP_IGNORE_TRANSPARENCY, 0);
		tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
		draw_all_sprites(bitmap, cliprect);
		tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
		break;

	case 2:
		tilemap_draw(bitmap, cliprect, bg0_tilemap, TILEMAP_IGNORE_TRANSPARENCY, 0);
		draw_all_sprites(bitmap, cliprect);
		tilemap_draw(bitmap, cliprect, bg0_tilemap, 0, 0);
		tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
		tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
		break;

	case 3:
		tilemap_draw(bitmap, cliprect, bg0_tilemap, TILEMAP_IGNORE_TRANSPARENCY, 0);
		tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
		tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
		draw_all_sprites(bitmap, cliprect);
		break;

	case 4: /* title screen */
		tilemap_draw(bitmap, cliprect, fg_tilemap, TILEMAP_IGNORE_TRANSPARENCY, 0);
		draw_all_sprites(bitmap, cliprect);
		tilemap_draw(bitmap, cliprect, bg0_tilemap, 0, 0);
		tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
		break;
	}
}

/*************************************
 *
 *  Machine
 *
 *************************************/

static MACHINE_RESET( akazukin )
{
	nmi_mask = 0;
	nmi_sub_mask = 0;
	latch_data[0] = 0;
	latch_data[1] = 0;

	/* the latch outputs are all low at reset, so the sound CPU starts halted */
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
}

static INTERRUPT_GEN( akazukin_vblank_irq )
{
	if (nmi_mask)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static INTERRUPT_GEN( akazukin_sub_irq )
{
	if (nmi_sub_mask)
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

/* the data and the interrupt line must change together, in sync with the receiving CPU */
static void latch_callback(int param)
{
	int which = param >> 8;

	latch_data[which] = param & 0xff;
	cpunum_set_input_line(which ? 0 : 1, 0, ASSERT_LINE);
}

static WRITE8_HANDLER( main_latch_w )
{
	timer_set(TIME_NOW, data, latch_callback);
}

static WRITE8_HANDLER( sub_latch_w )
{
	timer_set(TIME_NOW, 0x100 | data, latch_callback);
}

static READ8_HANDLER( main_latch_r )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	return latch_data[1];
}

static READ8_HANDLER( sub_latch_r )
{
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	return latch_data[0];
}

static WRITE8_HANDLER( mainlatch_w )
{
	data &= 1;
	switch (offset)
	{
		case 0:
			nmi_mask = data;
			break;
		case 1:
			flip_screen_set(data);
			break;
		case 2:
			cpunum_set_input_line(1, INPUT_LINE_RESET, data ? CLEAR_LINE : ASSERT_LINE);
			break;
	}
}

static WRITE8_HANDLER( nmi_sub_mask_w )
{
	nmi_sub_mask = data & 1;
}

static READ8_HANDLER( port_b_r )
{
	return 0xff;
}

static ADDRESS_MAP_START( main_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xafff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(input_port_2_r)
	AM_RANGE(0xe800, 0xe800) AM_READ(input_port_0_r)
	AM_RANGE(0xf000, 0xf000) AM_READ(input_port_1_r)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(bg1videoram_w) AM_BASE(&bg1videoram)
	AM_RANGE(0x9000, 0x9fff) AM_WRITE(bg0videoram_w) AM_BASE(&bg0videoram)
	AM_RANGE(0xa000, 0xabff) AM_WRITE(fgvideoram_w) AM_BASE(&fgvideoram)
	AM_RANGE(0xac00, 0xac00) AM_WRITE(MWA8_RAM) AM_BASE(&priority_reg)
	AM_RANGE(0xac01, 0xafff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc000, 0xc007) AM_WRITE(mainlatch_w)
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_READ(main_latch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(main_latch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(nmi_sub_mask_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_READ(sub_latch_r)
	AM_RANGE(0x42, 0x42) AM_READ(AY8910_read_port_1_r)
	AM_RANGE(0x82, 0x82) AM_READ(AY8910_read_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(sub_latch_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x41, 0x41) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x81, 0x81) AM_WRITE(AY8910_write_port_0_w)
ADDRESS_MAP_END

INPUT_PORTS_START( akazukin )
	PORT_START	/* P1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* P2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* SYSTEM */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Skip current level (Cheat)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 */
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
	PORT_DIPNAME( 0xf0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static const gfx_layout spritelayoutdw =
{
	16,32,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			96*8, 97*8, 98*8, 99*8, 100*8, 101*8, 102*8, 103*8 },
	128*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 64 },
	{ REGION_GFX2, 0, &spritelayout,   0, 64 },
	{ REGION_GFX2, 0, &spritelayoutdw, 0, 64 },
	{ REGION_GFX3, 0, &charlayout,     0, 64 },
	{ REGION_GFX4, 0, &charlayout,     0, 64 },
	{ -1 }
};

static struct AY8910interface ay8910_interface_0 =
{
	input_port_3_r,
	port_b_r
};

static struct AY8910interface ay8910_interface_1 =
{
	input_port_4_r,
	port_b_r
};

static MACHINE_DRIVER_START( akazukin )
	MDRV_CPU_ADD(Z80, MASTER_CLOCK / 6)
	MDRV_CPU_PROGRAM_MAP(main_readmem, main_writemem)
	MDRV_CPU_IO_MAP(main_readport, main_writeport)
	MDRV_CPU_VBLANK_INT(akazukin_vblank_irq, 1)

	MDRV_CPU_ADD(Z80, MASTER_CLOCK / 6)
	MDRV_CPU_PROGRAM_MAP(sub_readmem, sub_writemem)
	MDRV_CPU_IO_MAP(sub_readport, sub_writeport)
	MDRV_CPU_PERIODIC_INT(akazukin_sub_irq, TIME_IN_HZ(60))

	MDRV_FRAMES_PER_SECOND(60.58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)
	MDRV_MACHINE_RESET(akazukin)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(akazukin)
	MDRV_VIDEO_UPDATE(akazukin)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK / 12)
	MDRV_SOUND_CONFIG(ay8910_interface_0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK / 12)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( akazukin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.j2",  0x0000, 0x2000, CRC(8987f1e0) SHA1(a7a705f83a8b53c3c7b74b695a086516e3316219) )
	ROM_LOAD( "2.l2",  0x2000, 0x2000, CRC(c8b040e4) SHA1(36b37f75080b682405ce1ad093538f6a1231fe72) )
	ROM_LOAD( "3.m2",  0x4000, 0x2000, CRC(262edb0d) SHA1(afb2994274ae3d189ca3ffad556727afb0a8b1a4) )
	ROM_LOAD( "4.n2",  0x6000, 0x2000, CRC(3569221f) SHA1(045cf99fe85b26503d368fecf1dad5f3785b5d79) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "5.h7",  0x0000, 0x2000, CRC(c34486ae) SHA1(28b3db7e906202ee0d49907f66202544d352ff3f) )
	ROM_LOAD( "6.j7",  0x2000, 0x1000, CRC(b5e1d77f) SHA1(637f17d04976bc43801012280010df8500313331) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "10.b9",  0x0000, 0x1000, CRC(0145ded8) SHA1(515f3943579f2a8e17ca05959b9354981219d9d9) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "9.e9",   0x0000, 0x2000, CRC(5fecc3d7) SHA1(619584936382c38c391c654684b24f6d642dff03) )
	ROM_LOAD( "11.e7",  0x2000, 0x2000, CRC(448b28b8) SHA1(3a379c0a57ac89698b332d0af20ca8a3b0476d79) )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "7.t4",  0x0000, 0x1000, CRC(7b4124da) SHA1(9bd713ac660920f6c5b407e80df9438df575de88) )

	ROM_REGION( 0x2000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "8.p4",  0x0000, 0x2000, CRC(193f6bcb) SHA1(0f4699052b2c66fabd293e7ef08fd25de7af42f3) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "r.r6",  0x0000, 0x0100, CRC(77ccc932) SHA1(5bd23ca5ab80ac9c19e85ea79ba3d276c4be59cb) )
	ROM_LOAD( "g.m6",  0x0100, 0x0100, CRC(eddc3acf) SHA1(0fca5d36ccbd5191ce8a59d070112ae4a3297b0b) )
	ROM_LOAD( "b.l6",  0x0200, 0x0100, CRC(059dae45) SHA1(26c9b975804fc206e80ee21361e489824f39e0c0) )
	ROM_LOAD( "8n",  0x0300, 0x0100, CRC(e1b815ca) SHA1(df2b99259bf1023d37aa3b54247721d657d7e9c9) )
ROM_END

GAME( 1983, akazukin, 0, akazukin, akazukin, 0, ROT0, "Sigma", "Akazukin (Japan)", 0 )

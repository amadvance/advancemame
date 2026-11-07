// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina, David Haywood
/*
  Domino Block, Wonwoo Systems, 1996.
  Custom Korean Arkanoid hardware with high colour backgrounds.
  Z80 6 MHz, YM2149 3 MHz, 12 MHz oscillator, 59.1524 Hz VSync.
*/

#include "driver.h"
#include "sound/ay8910.h"

static UINT8 *fg_ram, *bg_ram, *object_ram;
static tilemap *fg_tilemap, *bg_tilemap;

static void get_fg_tile_info(int tile_index)
{
	int attr = fg_ram[tile_index * 2];
	SET_TILE_INFO(0, fg_ram[tile_index * 2 + 1] | ((attr & 7) << 8), attr >> 3, 0)
}
static void get_bg_tile_info(int tile_index)
{
	int attr = bg_ram[tile_index * 2 + 1];
	SET_TILE_INFO(1, bg_ram[tile_index * 2] | ((attr & 15) << 8), attr >> 4, 0)
}
static WRITE8_HANDLER( fg_ram_w )
{
	if (fg_ram[offset] != data)
	{
		fg_ram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset / 2);
	}
}
static WRITE8_HANDLER( bg_ram_w )
{
	if (bg_ram[offset] != data)
	{
		bg_ram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}
static void restore_video(void) { tilemap_mark_all_tiles_dirty(ALL_TILEMAPS); }
static VIDEO_START( dominob )
{
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 32, 32, 8, 8);
	if (!fg_tilemap || !bg_tilemap) return 1;
	tilemap_set_transparent_pen(fg_tilemap, 0);
	state_save_register_func_postload(restore_video);
	return 0;
}
static VIDEO_UPDATE( dominob )
{
	int pos;
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
	for (pos = 0; pos < 0x40; pos += 4)
	{
		int x = object_ram[pos], y = 248 - object_ram[pos + 1];
		int code = object_ram[pos + 3] | ((object_ram[pos + 2] & 3) << 8);
		int color = object_ram[pos + 2] >> 3;
		if (flip_screen_x) x = 248 - x;
		if (flip_screen_y) y = 248 - y;
		drawgfx(bitmap, Machine->gfx[0], code * 2, color, flip_screen_x, flip_screen_y,
			x, y + (flip_screen_y ? 8 : -8), cliprect, TRANSPARENCY_PEN, 0);
		drawgfx(bitmap, Machine->gfx[0], code * 2 + 1, color, flip_screen_x, flip_screen_y,
			x, y, cliprect, TRANSPARENCY_PEN, 0);
	}
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM AM_WRITENOP
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xd008, 0xd008) AM_WRITENOP
	AM_RANGE(0xd00c, 0xd00c) AM_READ(input_port_0_r)
	AM_RANGE(0xd010, 0xd010) AM_READ(input_port_1_r) AM_WRITENOP
	AM_RANGE(0xd018, 0xd018) AM_READ(input_port_2_r) AM_WRITENOP
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_WRITE(fg_ram_w) AM_BASE(&fg_ram)
	AM_RANGE(0xe800, 0xe83f) AM_RAM AM_BASE(&object_ram)
	AM_RANGE(0xe840, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xf07f) AM_RAM AM_WRITE(bg_ram_w) AM_BASE(&bg_ram)
	AM_RANGE(0xf080, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xfbff) AM_RAM AM_WRITE(paletteram_xxxxRRRRGGGGBBBB_le_w) AM_BASE(&paletteram)
	AM_RANGE(0xfc00, 0xffff) AM_RAM
ADDRESS_MAP_END
static READ8_HANDLER( port02_r ) { return 0xff; }
static ADDRESS_MAP_START( main_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x02) AM_READ(port02_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( dominob )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_SPECIAL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15)

	PORT_START

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, "Difficulty 1" )
	PORT_DIPSETTING(    0x06, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x18, "Difficulty 2" )
	PORT_DIPSETTING(    0x18, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x60, "1" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x80, 0x80, "Striptease" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout fglayout =
{
	8, 8, RGN_FRAC(1,3), 3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	64
};
static const gfx_layout bglayout =
{
	32,32,
	RGN_FRAC(1,4),
	4,
	{
		RGN_FRAC(3,4),
		RGN_FRAC(2,4),
		RGN_FRAC(1,4),
		RGN_FRAC(0,4) },
	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 ,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32,
		16*32,17*32,18*32,19*32,20*32,21*32,22*32,23*32,24*32,25*32,26*32,27*32,28*32,29*32,30*32,31*32},
	16*32*2
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fglayout, 0, 32 },
	{ REGION_GFX2, 0, &bglayout, 0x100, 16 },
	{ -1 }
};
static struct AY8910interface ay_interface = { 0, input_port_4_r };

static MACHINE_DRIVER_START( dominob )
	MDRV_CPU_ADD(Z80, 12000000 / 2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(main_io, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)
	MDRV_FRAMES_PER_SECOND(59.1524)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
	MDRV_VIDEO_START(dominob)
	MDRV_VIDEO_UPDATE(dominob)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 12000000 / 4)
	MDRV_SOUND_CONFIG(ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

ROM_START( dominob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u81",   0x0000, 0x10000, CRC(709b7a29) SHA1(7c95cbaf669a0885101a48e937868c245f87567e) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "u33",   0x00000, 0x8000, CRC(359c98de) SHA1(5c96dfb538c6b25530582f8c2a0cb20d85c39f68) )
	ROM_LOAD( "u34",   0x08000, 0x8000, CRC(0031f713) SHA1(9341f84081e2d8954e476236e93e49b4d8819b8f) )
	ROM_LOAD( "u35",   0x10000, 0x8000, CRC(6eb87657) SHA1(40ff9d6f21ade48b16f0cefea08a9364a4ee9144) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD( "u111",   0x00000, 0x40000, CRC(b835be84) SHA1(bbb744a28df00017f81d6eac12b00b5f3aca3a8b) )
	ROM_CONTINUE(0x00000,0x40000)
	ROM_LOAD( "u112",   0x40000, 0x40000, CRC(60d7bfd7) SHA1(9f6475ce717e3d5a42aaaacc3ec340e74b7e40b4) )
	ROM_CONTINUE(0x40000,0x40000)
	ROM_LOAD( "u113",   0x80000, 0x40000, CRC(3c999d1e) SHA1(e10c61d7868d9c902e337c9947506761878e31a5) )
	ROM_CONTINUE(0x80000,0x40000)
	ROM_LOAD( "u114",   0xc0000, 0x40000, CRC(2364124e) SHA1(55dc6c46f54655c574a96f5c920e4e1f2e05fd5d) )
	ROM_CONTINUE(0xc0000,0x40000)
ROM_END

GAME( 1996, dominob, 0, dominob, dominob, 0, ROT0, "Wonwoo Systems", "Domino Block", GAME_SUPPORTS_SAVE )

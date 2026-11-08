// license:BSD-3-Clause
// copyright-holders:Phil Bennett
/***************************************************************************

    Inter Geo Muscle Master hardware

    driver by Phil Bennett

***************************************************************************/

#include "driver.h"
#include "sound/okim6295.h"

static UINT16 *musclem_vram[3], *musclem_spriteram;
static UINT16 musclem_palette[0x400], musclem_scroll[4];
static UINT8 musclem_oki_bank;
static tilemap *musclem_tilemap[3];

static void get_tile0_info(int tile_index)
{
	SET_TILE_INFO(0, musclem_vram[0][tile_index], 0, 0)
}
static void get_tile1_info(int tile_index)
{
	SET_TILE_INFO(1, musclem_vram[1][tile_index], 0, 0)
}
static void get_tile2_info(int tile_index)
{
	SET_TILE_INFO(2, musclem_vram[2][tile_index], 0, 0)
}

static WRITE16_HANDLER( vram0_w )
{
	COMBINE_DATA(&musclem_vram[0][offset]);
	tilemap_mark_tile_dirty(musclem_tilemap[0], offset);
}
static WRITE16_HANDLER( vram1_w )
{
	COMBINE_DATA(&musclem_vram[1][offset]);
	tilemap_mark_tile_dirty(musclem_tilemap[1], offset);
}
static WRITE16_HANDLER( vram2_w )
{
	COMBINE_DATA(&musclem_vram[2][offset]);
	tilemap_mark_tile_dirty(musclem_tilemap[2], offset);
}

static void update_color(int index)
{
	UINT16 color = musclem_palette[index];
	palette_set_color(index, pal5bit(color), pal5bit(color >> 5), pal5bit(color >> 10));
}
static void palette_w(int offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&musclem_palette[offset]);
	update_color(offset);
}
static WRITE16_HANDLER( palette0_w ) { palette_w(offset, data, mem_mask); }
static WRITE16_HANDLER( palette1_w ) { palette_w(offset + 0x100, data, mem_mask); }
static WRITE16_HANDLER( palette2_w ) { palette_w(offset + 0x200, data, mem_mask); }
static WRITE16_HANDLER( palette3_w ) { palette_w(offset + 0x300, data, mem_mask); }

static WRITE16_HANDLER( scroll_w )
{
	COMBINE_DATA(&musclem_scroll[offset]);
	if (offset & 1)
		tilemap_set_scrolly(musclem_tilemap[1 + offset / 2], 0, musclem_scroll[offset]);
	else
		tilemap_set_scrollx(musclem_tilemap[1 + offset / 2], 0, musclem_scroll[offset]);
}
static WRITE16_HANDLER( oki_bank_w )
{
	if (ACCESSING_LSB)
	{
		musclem_oki_bank = data & 3;
		OKIM6295_set_bank_base(0, musclem_oki_bank * 0x40000);
	}
}

static void musclem_postload(void)
{
	int i;
	/* Rebuild derived colors and the sample ROM pointer from the saved registers. */
	for (i = 0; i < 0x400; ++i)
		update_color(i);
	for (i = 0; i < 4; ++i)
		scroll_w(i, musclem_scroll[i], 0);
	for (i = 0; i < 3; ++i)
		tilemap_mark_all_tiles_dirty(musclem_tilemap[i]);
	OKIM6295_set_bank_base(0, musclem_oki_bank * 0x40000);
}
static MACHINE_START( musclem )
{
	state_save_register_global_array(musclem_palette);
	state_save_register_global_array(musclem_scroll);
	state_save_register_global(musclem_oki_bank);
	state_save_register_func_postload(musclem_postload);
	return 0;
}
static MACHINE_RESET( musclem )
{
	int i;
	oki_bank_w(0, 0, 0);
	for (i = 0; i < 4; ++i)
		scroll_w(i, 0, 0);
}
static VIDEO_START( musclem )
{
	musclem_tilemap[0] = tilemap_create(get_tile0_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	musclem_tilemap[1] = tilemap_create(get_tile1_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	musclem_tilemap[2] = tilemap_create(get_tile2_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 16, 32, 32);
	if (!musclem_tilemap[0] || !musclem_tilemap[1] || !musclem_tilemap[2])
		return 1;
	tilemap_set_transparent_pen(musclem_tilemap[0], 0);
	tilemap_set_transparent_pen(musclem_tilemap[1], 0);
	tilemap_set_scrolldx(musclem_tilemap[0], 16, 0);
	tilemap_set_scrolldx(musclem_tilemap[2], 1, 0);
	return 0;
}
static VIDEO_UPDATE( musclem )
{
	int i;
	tilemap_draw(bitmap, cliprect, musclem_tilemap[2], 0, 0);
	tilemap_draw(bitmap, cliprect, musclem_tilemap[1], 0, 0);
	for (i = 0; i < 0x200; ++i)
	{
		int code = musclem_spriteram[i];
		int sx, sy;
		if (!code)
			continue;
		sy = musclem_spriteram[0x200 + i] & 0x1ff;
		if (sy & 0x100)
			sy -= 0x200;
		sx = (musclem_spriteram[0x400 + i] & 0x3ff) - 1;
		drawgfx(bitmap, Machine->gfx[3], code, 0, 0, 0, sx, sy, cliprect, TRANSPARENCY_PEN, 0);
		if (sx > 512 - 16)
			drawgfx(bitmap, Machine->gfx[3], code, 0, 0, 0, sx - 512, sy, cliprect, TRANSPARENCY_PEN, 0);
	}
	tilemap_draw(bitmap, cliprect, musclem_tilemap[0], 0, 0);
}

static ADDRESS_MAP_START( musclem_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x800000, 0x8007ff) AM_RAM AM_WRITE(vram0_w) AM_BASE(&musclem_vram[0])
	AM_RANGE(0x840000, 0x840fff) AM_RAM AM_BASE(&musclem_spriteram)
	AM_RANGE(0x880000, 0x8807ff) AM_RAM AM_WRITE(vram1_w) AM_BASE(&musclem_vram[1])
	AM_RANGE(0x8c0000, 0x8c07ff) AM_RAM AM_WRITE(vram2_w) AM_BASE(&musclem_vram[2])
	AM_RANGE(0x900000, 0x9001ff) AM_WRITE(palette0_w)
	AM_RANGE(0x900400, 0x9005ff) AM_WRITE(palette1_w)
	AM_RANGE(0x900800, 0x9009ff) AM_WRITE(palette2_w)
	AM_RANGE(0x900c00, 0x900dff) AM_WRITE(palette3_w)
	AM_RANGE(0xc00000, 0xc00001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xc00080, 0xc00087) AM_WRITE(scroll_w)
	AM_RANGE(0xc00088, 0xc00089) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0xc0008a, 0xc0008b) AM_READWRITE(OKIM6295_status_1_lsb_r, OKIM6295_data_1_lsb_w)
	AM_RANGE(0xc0008c, 0xc0008d) AM_WRITE(oki_bank_w)
	AM_RANGE(0xc0008e, 0xc0008f) AM_WRITENOP
	AM_RANGE(0xc00090, 0xc00091) AM_READ(input_port_1_word_r)
	AM_RANGE(0xc00092, 0xc00093) AM_READ(input_port_2_word_r)
	AM_RANGE(0xc00094, 0xc00095) AM_WRITENOP
ADDRESS_MAP_END

static INPUT_PORTS_START( musclem )
	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Demo Lamp" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Pump sensor 1")
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Pump sensor 2")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Pump sensor 3")
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static const gfx_layout musclem_layout =
{
	16, 16, RGN_FRAC(1,1), 8,
	{ GFX_RAW }, { 0 }, { 16*8 }, 16*16*8
};
static const gfx_decode musclem_gfxdecode[] =
{
	{ REGION_GFX1, 0, &musclem_layout, 0x000, 1 },
	{ REGION_GFX2, 0, &musclem_layout, 0x200, 1 },
	{ REGION_GFX3, 0, &musclem_layout, 0x300, 1 },
	{ REGION_GFX4, 0, &musclem_layout, 0x100, 1 },
	{ -1 }
};

static MACHINE_DRIVER_START( musclem )
	MDRV_CPU_ADD(M68000, 24000000/2)
	MDRV_CPU_PROGRAM_MAP(musclem_map, 0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold, 1)
	MDRV_MACHINE_START(musclem)
	MDRV_MACHINE_RESET(musclem)
	MDRV_FRAMES_PER_SECOND(14318181.0 / (2 * 440 * 273))
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(440, 273)
	MDRV_VISIBLE_AREA(16, 327, 0, 239)
	MDRV_PALETTE_LENGTH(0x400)
	MDRV_GFXDECODE(musclem_gfxdecode)
	MDRV_VIDEO_START(musclem)
	MDRV_VIDEO_UPDATE(musclem)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( musclem )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "u-71.bin",   0x000000, 0x20000, CRC(da0648ad) SHA1(e7de583d5e6de4e2f0d617527d168fe691243728) )
	ROM_LOAD16_BYTE( "u-67.bin",   0x000001, 0x20000, CRC(77f4b6ad) SHA1(8de11378903d48f0c57c6dde3923d8c5fa614ccc) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "u-148.bin", 0x000000, 0x100000, CRC(97aa308a) SHA1(54b2f5a25dcd3c35330f384a9010fbf75a72b8d0) )
	ROM_LOAD16_BYTE( "u-150.bin", 0x000001, 0x100000, CRC(0c7247bd) SHA1(cab4f83f56ac6567feb27c397d31116c213d0a86) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "u-129.bin", 0x000000, 0x100000, CRC(8268bba6) SHA1(bef3ee971ddf452346c6a7877e1aba577cdaf82c) )
	ROM_LOAD16_BYTE( "u-130.bin", 0x000001, 0x100000, CRC(be4755e6) SHA1(66a7444bad5def32d44031680a4e8e2f8c1bdc81) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "u-134.bin", 0x000000, 0x080000, CRC(79c8f776) SHA1(830a3905fc4ae88af48946ffc9bc0d58a8906d81) )
	ROM_LOAD16_BYTE( "u-135.bin", 0x000001, 0x080000, CRC(56b89595) SHA1(ea6efac73857189faa1aa46a31e480770b1b14c7) )

	ROM_REGION( 0x200000, REGION_GFX4, 0 )
	ROM_LOAD32_BYTE( "u-21.bin", 0x000000, 0x80000, CRC(efc2ba0d) SHA1(a72e5943dc71870f7c7eec30015d38ab06ed1846) )
	ROM_LOAD32_BYTE( "u-23.bin", 0x000001, 0x80000, CRC(a8f96912) SHA1(69bc5c7c9a391e8a4b5b260bf9f05ffd44b9cadc) )
	ROM_LOAD32_BYTE( "u-25.bin", 0x000002, 0x80000, CRC(711563c0) SHA1(f0ddd102e5c6dc7286bbeac868c3f08012c54abd) )
	ROM_LOAD32_BYTE( "u-27.bin", 0x000003, 0x80000, CRC(5d6026f3) SHA1(2afc247c7c6129c184c3ae2f98a39781183c99a5) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "u-173.bin", 0x00000, 0x100000, CRC(d93bc7c2) SHA1(bfa3e3936e6fec4f94bfc0bf53cf8b04c085dc92) )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )
	ROM_LOAD( "u-177.bin", 0x00000, 0x40000, CRC(622c7b2f) SHA1(6b1721b80f71e5a5b5463c3320efa2e400fd9eac) )
ROM_END

GAME( 1997, musclem, 0, musclem, musclem, 0, ROT0, "Inter Geo", "Muscle Master", GAME_SUPPORTS_SAVE )

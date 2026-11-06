// license:BSD-3-Clause
// copyright-holders:David Haywood, ClawGrip
/*
    Xor World (Modular System).
    Gaelco's original Modular System hardware, using 68000 and Z80 CPUs,
    three tilemap layers, YM3812 and MSM5205 audio.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/3812intf.h"
#include "sound/msm5205.h"

static UINT16 *fg_vram, *mg_vram, *bg_vram, *scrollregs;
static tilemap *fg_layer, *mg_layer, *bg_layer;
static int adpcm_data;

static VIDEO_UPDATE( xorworld_ms )
{

	int global_x_offset = 128;

	tilemap_set_scrollx(fg_layer, 0, global_x_offset-(scrollregs[4]));
	tilemap_set_scrolly(fg_layer, 0, -(scrollregs[5]));
	tilemap_set_scrollx(mg_layer, 0, global_x_offset-(scrollregs[0]));
	tilemap_set_scrolly(mg_layer, 0, -(scrollregs[1]));
	tilemap_set_scrollx(bg_layer, 0, global_x_offset-(scrollregs[6]));
	tilemap_set_scrolly(bg_layer, 0, -(scrollregs[7]));

	tilemap_draw(bitmap, cliprect, bg_layer, 0, 0);
	tilemap_draw(bitmap, cliprect, mg_layer, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_layer, 0, 0);

	int spritexoffset = scrollregs[2] & 0xfff;
	if (spritexoffset & 0x800)
		spritexoffset -= 0x1000;

	const int NUM_SPRITES = 0x200;
	const int X_EXTRA_OFFSET = global_x_offset + spritexoffset;

	for (int i = NUM_SPRITES - 2; i >= 0; i -= 2)
	{

		UINT16 attr0 = spriteram16[i + 0];
		UINT16 attr1 = spriteram16[i + 1];

		UINT16 attr2 = spriteram16[i + NUM_SPRITES];

		int ypos = attr0 & 0x00ff;

		if (ypos == 0x00f8)
			continue;

		int xpos = (attr1 & 0xff00) >> 8;

		xpos |= (attr2 & 0x8000) ? 0x100 : 0x000;

		ypos = (0xff - ypos);
		ypos |= (attr2 & 0x4000) ? 0x100 : 0x000;

		int tile = (attr0 & 0xff00) >> 8;
		tile |= (attr1 & 0x003f) << 8;

		int flipx = (attr1 & 0x0040);
		int flipy = (attr1 & 0x0080);

		drawgfx(bitmap, Machine->gfx[0], tile, (attr2 & 0x0f00) >> 8, flipx, flipy, xpos - 16 - X_EXTRA_OFFSET, ypos - 16, cliprect, TRANSPARENCY_PEN, 15);
	}

}

static WRITE16_HANDLER( videoram8x8_mg_w )
{
	COMBINE_DATA(&mg_vram[offset]);
	tilemap_mark_tile_dirty(mg_layer, offset/2);
}

static void get_tile_info_tilemap_8x8mg(int tile_index)
{
	int tile = mg_vram[tile_index * 2];
	int attr = mg_vram[(tile_index * 2) + 1] & 0x1f;

	SET_TILE_INFO(2, tile, attr, 0);
}

static WRITE16_HANDLER( videoram8x8_fg_w )
{
	COMBINE_DATA(&fg_vram[offset]);
	tilemap_mark_tile_dirty(fg_layer, offset / 2);
}

static void get_tile_info_tilemap_8x8fg(int tile_index)
{
	int tile = fg_vram[tile_index * 2];
	int attr = fg_vram[(tile_index * 2) + 1] & 0x1f;

	SET_TILE_INFO(1, tile, attr, 0);
}

static WRITE16_HANDLER( videoram16x16_bg_w )
{
	COMBINE_DATA(&bg_vram[offset]);
	tilemap_mark_tile_dirty(bg_layer, offset / 2);
}

static void get_tile_info_tilemap_16x16bg(int tile_index)
{

	int tile = bg_vram[tile_index * 2];
	int attr = (bg_vram[(tile_index * 2) + 1]);

	SET_TILE_INFO(3, tile, attr & 0x1f, 0);
}

static VIDEO_START( xorworld_ms )
{
	fg_layer = tilemap_create(get_tile_info_tilemap_8x8fg, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	mg_layer = tilemap_create(get_tile_info_tilemap_8x8mg, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	bg_layer = tilemap_create(get_tile_info_tilemap_16x16bg, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 16, 64, 32);
	if (!fg_layer || !mg_layer || !bg_layer) return 1;
	tilemap_set_transparent_pen(fg_layer, 0);
	tilemap_set_transparent_pen(mg_layer, 0);
	return 0;
}

static WRITE16_HANDLER( soundcmd_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(0, data >> 8);
		cpunum_set_input_line(1, 0, ASSERT_LINE);
		cpu_boost_interleave(0, TIME_IN_USEC(50));
	}
}

static READ8_HANDLER( soundcmd_r )
{
	/* Reading the command acknowledges the latch's level-triggered interrupt. */
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	return soundlatch_r(0);
}

static WRITE8_HANDLER( adpcm_data_w ) { adpcm_data = data; }
static WRITE8_HANDLER( adpcm_control_w ) { MSM5205_reset_w(0, data & 0x80); }
static void adpcm_clock(int chip)
{
	MSM5205_data_w(0, adpcm_data >> 4);
	adpcm_data = (adpcm_data << 4) & 0xf0;
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x080000, 0x081fff) AM_RAM AM_WRITE(videoram8x8_fg_w) AM_BASE(&fg_vram)
	AM_RANGE(0x090000, 0x091fff) AM_RAM AM_WRITE(videoram8x8_mg_w) AM_BASE(&mg_vram)
	AM_RANGE(0x0a0000, 0x0a1fff) AM_RAM AM_WRITE(videoram16x16_bg_w) AM_BASE(&bg_vram)
	AM_RANGE(0x0c0000, 0x0c000f) AM_RAM AM_BASE(&scrollregs)
	AM_RANGE(0x100000, 0x1007ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x200000, 0x2007ff) AM_RAM AM_WRITE(paletteram16_xxxxBBBBRRRRGGGG_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x400000, 0x400001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x400002, 0x400003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x400006, 0x400007) AM_READ(input_port_2_word_r)
	AM_RANGE(0x400008, 0x400009) AM_READ(input_port_3_word_r)
	AM_RANGE(0x40000c, 0x40000d) AM_READWRITE(MRA16_NOP, MWA16_NOP)
	AM_RANGE(0x40000e, 0x40000f) AM_WRITE(soundcmd_w)
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xe000, 0xe000) AM_WRITE(adpcm_control_w)
	AM_RANGE(0xe400, 0xe400) AM_WRITE(adpcm_data_w)
	AM_RANGE(0xe800, 0xe801) AM_READ(YM3812_status_port_0_r)
	AM_RANGE(0xe800, 0xe800) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0xe801, 0xe801) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xf800) AM_READ(soundcmd_r)
ADDRESS_MAP_END

INPUT_PORTS_START( xorworld_ms )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("P1 Button 3 / P1 Start")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 ) PORT_NAME("P2 Button 3 / P2 Start")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("DSW1")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x6000, 0x6000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW2")

	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static const gfx_layout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7 },
	{ STEP8(0,32) },
	16 * 16
};

static const gfx_layout tiles16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7, 512,513,514,515,516,517,518,519 },
	{ STEP16(0,32) },
	16 * 16 * 4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16x4_layout, 0x200, 16 },
	{ REGION_GFX2, 0, &tiles8x8x4_layout, 0, 32 },
	{ REGION_GFX3, 0, &tiles8x8x4_layout, 0, 32 },
	{ REGION_GFX4, 0, &tiles16x16x4_layout, 0, 32 },
	{ -1 }
};

static const struct MSM5205interface msm_interface = { adpcm_clock, MSM5205_S48_4B };

static MACHINE_RESET( xorworld_ms )
{
	adpcm_data = 0;
	MSM5205_reset_w(0, 1);
	cpunum_set_input_line(1, 0, CLEAR_LINE);
}

static MACHINE_DRIVER_START( xorworld_ms )
	MDRV_CPU_ADD(M68000, 20000000 / 2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold, 1)
	MDRV_CPU_ADD(Z80, 16000000 / 4)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse, TIME_IN_HZ(2500))
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_MACHINE_RESET(xorworld_ms)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)
	MDRV_VIDEO_START(xorworld_ms)
	MDRV_VIDEO_UPDATE(xorworld_ms)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM3812, 16000000 / 4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END

static DRIVER_INIT( xorworld_ms )
{
	UINT8 *src = memory_region(REGION_GFX4);
	int len = memory_region_length(REGION_GFX4), i;
	UINT8 *buffer = auto_malloc(len);
	/* Restore the graphics address lines before decoding the 16x16 background. */
	for (i = 0; i < len; ++i)
	{
		int j = BITSWAP24(i,23,22,21,20,19,18,17,16,15,12,11,10,9,8,7,6,5,14,13,4,3,2,1,0);
		buffer[j] = src[i];
	}
	memcpy(src, buffer, len);
}

ROM_START( xorwldms )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mod_6-1_xo_608a_27c512.ic8",  0x000001, 0x010000, CRC(cebcd3e7) SHA1(ee6d107dd13e4faa8d5b6ba9815c57919a69568e) )
	ROM_LOAD16_BYTE( "mod_6-1_xo_617a_27c512.ic17", 0x000000, 0x010000, CRC(47bae292) SHA1(f36e2f80d4da31d7edc7dc8c07abb158ef21cb20) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic15", 0x00003, 0x10000, CRC(01d16cac) SHA1(645b3380e66ab158392f8161485ae957ab2dfa44) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic22", 0x00002, 0x10000, CRC(3aadacaf) SHA1(f640a1d6a32774cb70e180c49965f70f5b79ba6a) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic30", 0x00001, 0x10000, CRC(fa75826e) SHA1(1db9cc8ec5811b9d497b060bfc3245c6e082e98c) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic37", 0x00000, 0x10000, CRC(157832ed) SHA1(5dec1c7046d2449d3d4330654554f1a9430c8057) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE | 0 )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic11", 0x00003, 0x10000, CRC(93373f1f) SHA1(06718a09c76060914a1b2a4c57d0328a9f59eb99) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic18", 0x00002, 0x10000, CRC(7015d8ff) SHA1(83d40ef2ca1cf8dfb2229b8f47c08a303b8490ea) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic26", 0x00001, 0x10000, CRC(4be8db92) SHA1(8983893570587de9cdd63bc645d1ae2d8c3400a2) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic33", 0x00000, 0x10000, CRC(c29cd83b) SHA1(e8833b6ab8f7f6ce4c01937e94332a86908353db) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE | 0 )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic13", 0x00003, 0x10000, CRC(56a683fc) SHA1(986a6b26e38308dd3230dd13a1a2f5cfc7b1dda8) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic20", 0x00002, 0x10000, CRC(950090e6) SHA1(cdec2eb93884c5a759af39d02f5cc0fa25011103) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic28", 0x00001, 0x10000, CRC(ca950a11) SHA1(cef2a65d1f5562556fc28a95f6ba8f3ff1a3678d) )
	ROM_LOAD32_BYTE( "mod_8-2_27c512.ic35", 0x00000, 0x10000, CRC(5e7582f9) SHA1(fdb3ddb2a224fdea610f6bd55a3e3efe1a5515a2) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF | ROMREGION_INVERT )
	ROM_LOAD32_BYTE( "mod_5-1_27c512.ic3",  0x00003, 0x10000, CRC(26b2181e) SHA1(03cc280a4e9d9d8ac1c04da7d90c684d83fc2444) )
	ROM_LOAD32_BYTE( "mod_5-1_27c512.ic12", 0x00002, 0x10000, CRC(b2bdb8d1) SHA1(eeace8dc4e6c192ad7b2f53cd01fcc2e92125f35) )
	ROM_LOAD32_BYTE( "mod_5-1_27c512.ic18", 0x00001, 0x10000, CRC(d0c7a07b) SHA1(698883f2a91a34c802c1bc9a86bec0a77f4de7fe) )
	ROM_LOAD32_BYTE( "mod_5-1_27c512.ic24", 0x00000, 0x10000, CRC(fd2cbaed) SHA1(037523a3eae058e848763bbfc067d8271c70074b) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "mod_9-2_27c512.ic6", 0x00000, 0x10000, CRC(c722165b) SHA1(b1cd3ac80521036059579e71b6cb9bd4b97da4c2) )

	ROM_REGION( 0x100, REGION_PROMS, ROMREGION_ERASE00 )
	ROM_LOAD( "mod_51-3_502_82s129a.ic10", 0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x157, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "mod_5-1_52fx_gal16v8.ic8",  0x0000, 0x117, CRC(f3d686c9) SHA1(6bc1bd40f49663e776c0d40b2f146338a2057097) )
	ROM_LOAD( "mod_5-1_51fx_gal16v8.ic9",  0x0000, 0x117, CRC(0070e8b9) SHA1(8a3ef9445599dc88c001a919e01f47844eec81eb) )
	ROM_LOAD( "mod_6-1_gal16v8.ic7",       0x0000, 0x117, BAD_DUMP CRC(470a0194) SHA1(810c51fca7d2430b7c2292f3e1dd02c86d74d64b) )
	ROM_LOAD( "mod_6-1_palce16v8.ic13",    0x0000, 0x117, BAD_DUMP CRC(0b90f141) SHA1(4b0f2b98073d52b6a49941031e8e37b5aeb6d507) )
	ROM_LOAD( "mod_7-4_gal20v8.ic7",       0x0000, 0x157, NO_DUMP )
	ROM_LOAD( "mod_7-4_gal16v8.ic9",       0x0000, 0x117, NO_DUMP )
	ROM_LOAD( "mod_7-4_gal20v8.ic44",      0x0000, 0x157, NO_DUMP )
	ROM_LOAD( "mod_7-4_gal20v8.ic54",      0x0000, 0x157, NO_DUMP )
	ROM_LOAD( "mod_7-4_gal16v8.ic55",      0x0000, 0x117, NO_DUMP )
	ROM_LOAD( "mod_7-4_gal16v8.ic59",      0x0000, 0x117, BAD_DUMP CRC(55b86dd5) SHA1(4a05d86002f929f38171282d2aad5d986283c45b) )
	ROM_LOAD( "mod_9-2_gal16v8.ic10",      0x0000, 0x117, BAD_DUMP CRC(23070765) SHA1(3c87f7ab299fcd62da366fc9c1985ab21ee14960) )
	ROM_LOAD( "mod_9-2_gal20v8.ic18",      0x0000, 0x157, NO_DUMP )
	ROM_LOAD( "mod_9-2_gal16v8.ic42",      0x0000, 0x117, BAD_DUMP CRC(d62f0a0d) SHA1(76bd995c8b71b12b752ed517d87a78e55a8bab00) )
	ROM_LOAD( "mod_51-3_503_gal16v8.ic46", 0x0000, 0x117, NO_DUMP )
ROM_END

GAME( 1990, xorwldms, 0, xorworld_ms, xorworld_ms, xorworld_ms, ROT0, "Gaelco", "Xor World (Modular System, prototype, set 1)", GAME_IMPERFECT_SOUND )

// license:BSD-3-Clause
// copyright-holders:David Haywood, Bryan McPhail
/*
    Silver Millennium / PuzzLove
    Para hardware derived from Data East / Tumble Pop designs.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

static UINT16 *bg_videoram, *fg_videoram;
static tilemap *bg_layer, *fg_layer;
static int tilebank[4], whichbank, puzzlove_sprites;
static UINT16 scroll_regs[4];

static void get_bg_tile_info(int tile_index)
{
	int data = bg_videoram[tile_index];
	SET_TILE_INFO(0, (data & 0x3ff) + tilebank[(data >> 10) & 3] * 0x400, (data >> 12) + 0x20, 0);
}

static void get_fg_tile_info(int tile_index)
{
	int data = fg_videoram[tile_index];
	SET_TILE_INFO(0, (data & 0x3ff) + tilebank[(data >> 10) & 3] * 0x400, (data >> 12) + 0x10, 0);
}

static UINT32 scan_rows(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	return (col & 31) + ((row & 31) << 5) + ((col & 32) << 5) + ((row & 32) << 6);
}

static WRITE16_HANDLER( tilebank_select_w )
{
	if (ACCESSING_MSB) whichbank = (data >> 8) & 3;
}

static WRITE16_HANDLER( tilebank_w )
{
	if (ACCESSING_MSB)
	{
		tilebank[whichbank] = (data >> 8) & 31;
		tilemap_mark_all_tiles_dirty(bg_layer);
		tilemap_mark_all_tiles_dirty(fg_layer);
	}
}

static WRITE16_HANDLER( scroll_w )
{
	COMBINE_DATA(&scroll_regs[offset]);
	switch (offset)
	{
	case 0: tilemap_set_scrollx(fg_layer, 0, scroll_regs[0]); break;
	case 1: tilemap_set_scrolly(fg_layer, 0, scroll_regs[1] + 8); break;
	case 2: tilemap_set_scrollx(bg_layer, 0, scroll_regs[2] + 4); break;
	case 3: tilemap_set_scrolly(bg_layer, 0, scroll_regs[3] + 8); break;
	}
}

static WRITE16_HANDLER( fg_videoram_w )
{
	COMBINE_DATA(&fg_videoram[offset]);
	tilemap_mark_tile_dirty(fg_layer, offset);
}

static WRITE16_HANDLER( bg_videoram_w )
{
	COMBINE_DATA(&bg_videoram[offset]);
	tilemap_mark_tile_dirty(bg_layer, offset);
}

static VIDEO_START( silvmil )
{
	bg_layer = tilemap_create(get_bg_tile_info, scan_rows, TILEMAP_OPAQUE, 16, 16, 64, 32);
	fg_layer = tilemap_create(get_fg_tile_info, scan_rows, TILEMAP_TRANSPARENT, 16, 16, 64, 32);
	if (!bg_layer || !fg_layer) return 1;
	tilemap_set_transparent_pen(fg_layer, 0);
	return 0;
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int offs;
	for (offs = 0; offs < 0x400; offs += 4)
	{
		int y = spriteram16[offs];
		int sprite = spriteram16[offs + 1];
		int x = spriteram16[offs + 2];
		int colour = (x >> 9) & 31;
		int fx = (y >> 13) & 1, fy = (y >> 14) & 1;
		int wide = y & 0x800;
		int height, multi, inc;
		/* The PuzzLove sprite chip moves both the flash and height control bits. */
		if ((y & (puzzlove_sprites ? 0x400 : 0x1000)) && (cpu_getcurrentframe() & 1)) continue;
		height = puzzlove_sprites ? (((y >> 8) & 2) | ((y >> 12) & 1)) : ((y >> 9) & 3);
		multi = (1 << height) - 1;
		x = ((x & 0x1ff) + 5) & 0x1ff;
		y = ((y & 0x1ff) + 7) & 0x1ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		x = 304 - x;
		y = 240 - y;
		/* This chip keeps the original tile number for multi-height sprites. */
		inc = fy ? -1 : 1;
		if (!fy) sprite += multi;
		for (height = multi; height >= 0; --height)
		{
			int tile = sprite - height * inc;
			drawgfx(bitmap, Machine->gfx[1], tile, colour, fx, fy, x, y - 16 * height, cliprect, TRANSPARENCY_PEN, 0);
			if (wide)
				drawgfx(bitmap, Machine->gfx[1], tile - multi - 1, colour, fx, fy, x - 16, y - 16 * height, cliprect, TRANSPARENCY_PEN, 0);
		}
	}
}

static VIDEO_UPDATE( silvmil )
{
	tilemap_draw(bitmap, cliprect, bg_layer, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_layer, 0, 0);
	draw_sprites(bitmap, cliprect);
}

static WRITE16_HANDLER( soundcmd_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0, data);
		cpu_boost_interleave(0, TIME_IN_USEC(20));
	}
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x100001) AM_WRITE(tilebank_select_w)
	AM_RANGE(0x100002, 0x100009) AM_WRITE(scroll_w)
	AM_RANGE(0x10000e, 0x10000f) AM_WRITE(tilebank_w)
	AM_RANGE(0x120000, 0x120fff) AM_RAM AM_WRITE(fg_videoram_w) AM_BASE(&fg_videoram)
	AM_RANGE(0x122000, 0x122fff) AM_RAM AM_WRITE(bg_videoram_w) AM_BASE(&bg_videoram)
	AM_RANGE(0x200000, 0x2005ff) AM_RAM AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x210000, 0x2107ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x270000, 0x270001) AM_WRITE(soundcmd_w)
	AM_RANGE(0x280000, 0x280001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x280002, 0x280003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x280004, 0x280005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x300000, 0x30ffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc001) AM_READ(YM2151_status_port_0_r) AM_WRITE(YM2151_word_0_w)
	AM_RANGE(0xc002, 0xc003) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xc006, 0xc006) AM_READ(soundlatch_r)
	AM_RANGE(0xc00f, 0xc00f) AM_WRITENOP
	AM_RANGE(0xd000, 0xd7ff) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( silvmil )
	PORT_START_TAG("P1_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )       PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DSW1:3 - Unknown" )      PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DSW1:4 - Unknown" )      PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DSW1:5 - Unknown" )      PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DSW1:6 - Unknown" )      PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DSW1:7 - Unknown" )      PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DSW1:8 - Unknown" )      PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DSW2:1 - Unknown" )      PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DSW2:2 - Unknown" )      PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, DEF_STR( Language ) )     PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(      0x0400, "Korean" )
	PORT_DIPSETTING(      0x0000, DEF_STR( English ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coinage ) )      PORT_DIPLOCATION("SW2:4,5,6")
	PORT_DIPSETTING(      0x0800, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Coin Box" )          PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(      0x4000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_SERVICE(0x8000, IP_ACTIVE_LOW)
INPUT_PORTS_END

INPUT_PORTS_START( puzzlove )
	PORT_START_TAG("P1_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME(0x0001, 0x0001, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(0x0001, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME(0x0002, 0x0002, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(0x0002, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME(0x0004, 0x0004, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(0x0004, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME(0x0008, 0x0008, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(0x0008, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME(0x0010, 0x0010, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(0x0010, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME(0x0020, 0x0020, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(0x0020, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME(0x0040, 0x0040, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(0x0040, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME(0x0080, 0x0080, DEF_STR(Unused)) PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(0x0080, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
	PORT_DIPNAME( 0x0300, 0x0100, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(      0x0300, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coinage ) )  PORT_DIPLOCATION("SW2:3,4,5")
	PORT_DIPSETTING(      0x1000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Coin Box" )      PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(      0x2000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Lives ) )      PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x4000, "3" )
	PORT_SERVICE(0x8000, IP_ACTIVE_LOW)
INPUT_PORTS_END

INPUT_PORTS_START( puzzlovek )
	PORT_INCLUDE(puzzlove)

	PORT_MODIFY("DSW")

	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )   PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING( 0x0001, DEF_STR ( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR ( On ) )

	PORT_DIPNAME( 0x0300, 0x0100, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(      0x0300, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )
INPUT_PORTS_END

static const gfx_layout tlayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ STEP8(8*2*16,1), STEP8(0,1) },
	{ STEP16(0,8*2) },
	64*8
};

static const gfx_layout tlayout_alt =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 8, 0, RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0 },
	{ STEP8(0,1), STEP8(8*2*16,1) },
	{ STEP16(0,8*2) },
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tlayout_alt, 0, 48 },
	{ REGION_GFX2, 0, &tlayout, 0, 32 },
	{ -1 }
};

static void ym_irq(int state)
{
	cpunum_set_input_line(1, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct YM2151interface ym_interface = { ym_irq, 0 };

static MACHINE_RESET( silvmil )
{
	memset(tilebank, 0, sizeof(tilebank));
	memset(scroll_regs, 0, sizeof(scroll_regs));
	whichbank = 0;
	tilemap_mark_all_tiles_dirty(bg_layer);
	tilemap_mark_all_tiles_dirty(fg_layer);
	tilemap_set_scrollx(fg_layer, 0, 0);
	tilemap_set_scrollx(bg_layer, 0, 4);
	tilemap_set_scrolly(fg_layer, 0, 8);
	tilemap_set_scrolly(bg_layer, 0, 8);
}

static MACHINE_DRIVER_START( silvmil )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold, 1)
	MDRV_CPU_ADD_TAG("sound", Z80, 4096000)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_MACHINE_RESET(silvmil)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x300)
	MDRV_VIDEO_START(silvmil)
	MDRV_VIDEO_UPDATE(silvmil)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("ym", YM2151, 14318181 / 4)
	MDRV_SOUND_CONFIG(ym_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD_TAG("oki", OKIM6295, 4096000 / 4 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( puzzlove )
	MDRV_IMPORT_FROM(silvmil)
	MDRV_CPU_REPLACE("sound", Z80, 4000000)
	MDRV_SOUND_REPLACE("oki", OKIM6295, 4000000 / 4 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( puzzlovek )
	MDRV_IMPORT_FROM(puzzlove)
	MDRV_SOUND_REPLACE("ym", YM2151, 15000000 / 4)
	MDRV_SOUND_CONFIG(ym_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static DRIVER_INIT( silvmil ) { puzzlove_sprites = 0; }
static DRIVER_INIT( puzzlove ) { puzzlove_sprites = 1; }

ROM_START( silvmil )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "d-13_u3.bin", 0x00000, 0x40000, CRC(46106c59) SHA1(ab965c1ea9ce89a67875530d76ac2bde7fd13e13) )
	ROM_LOAD16_BYTE( "d-14_u2.bin", 0x00001, 0x40000, CRC(65421eca) SHA1(5258df7ceab6427e1b6f93f51896f7f3a651300c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "d-11_uz02.bin", 0x00000, 0x10000, CRC(b41289e6) SHA1(edf83206de418757a154d5a085b0b2fd97a77807) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "d-12_uz1.bin", 0x00000, 0x40000, CRC(a170d8a9) SHA1(77339382570498f9f6eeb80595bfe72fc853fd68))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "d-16_u41.bin",   0x000000, 0x20000, CRC(ff5ea605) SHA1(38e32f391b211ed280dd7f05eb13301cb9ddf57c) )
	ROM_CONTINUE ( 0x200000,0x20000 )
	ROM_CONTINUE ( 0x040000,0x20000 )
	ROM_CONTINUE ( 0x240000,0x20000 )
	ROM_LOAD16_BYTE( "d-15_u42.bin",   0x000001, 0x20000, CRC(5f72d6cc) SHA1(88641a42c0fc6e84ffd009fb40d0b1774e8c0630) )
	ROM_CONTINUE ( 0x200001,0x20000 )
	ROM_CONTINUE ( 0x040001,0x20000 )
	ROM_CONTINUE ( 0x240001,0x20000 )
	ROM_LOAD16_BYTE( "d-20_pat01.bin", 0x080001, 0x20000, CRC(cc4d3b49) SHA1(a2ef0d949443c9133487f83d5879daabca688d66) )
	ROM_CONTINUE ( 0x280001,0x20000 )
	ROM_CONTINUE ( 0x0c0001,0x20000 )
	ROM_CONTINUE ( 0x2c0001,0x20000 )
	ROM_LOAD16_BYTE( "d-17_pat11.bin", 0x080000, 0x20000, CRC(2facb3f1) SHA1(ea380345e1f15451e1ded5de178ec4648c33e2f0) )
	ROM_CONTINUE ( 0x280000,0x20000 )
	ROM_CONTINUE ( 0x0c0000,0x20000 )
	ROM_CONTINUE ( 0x2c0000,0x20000 )
	ROM_LOAD16_BYTE( "d-17_pat02.bin", 0x100001, 0x20000, CRC(addc5261) SHA1(73c5e29a39364c286a110678eb65b85ab1cf97d4) )
	ROM_CONTINUE ( 0x300001,0x20000)
	ROM_CONTINUE ( 0x140001,0x20000)
	ROM_CONTINUE ( 0x340001,0x20000)
	ROM_LOAD16_BYTE( "d-18_pat12.bin", 0x100000, 0x20000, CRC(a429b237) SHA1(ab709b93c5232f3fdbb60954f93d04bb3828ac76) )
	ROM_CONTINUE ( 0x300000,0x20000)
	ROM_CONTINUE ( 0x140000,0x20000)
	ROM_CONTINUE ( 0x340000,0x20000)
	ROM_LOAD16_BYTE( "d-18_pat03.bin", 0x180001, 0x20000, CRC(36b9c407) SHA1(291e29a2f5de6db639988e99a010fcac233387ad) )
	ROM_CONTINUE ( 0x380001,0x20000)
	ROM_CONTINUE ( 0x1c0001,0x20000)
	ROM_CONTINUE ( 0x3c0001,0x20000)
	ROM_LOAD16_BYTE( "d-19_pat13.bin", 0x180000, 0x20000, CRC(af2507ce) SHA1(280b3995a6164e4ef834247d637ee46c61bf9950) )
	ROM_CONTINUE ( 0x380000,0x20000)
	ROM_CONTINUE ( 0x1c0000,0x20000)
	ROM_CONTINUE ( 0x3c0000,0x20000)

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "d-17_u53.bin", 0x000000, 0x80000, CRC(4d177bda) SHA1(980205dead92830362095dac61d2e99bf62f0f5d) )
	ROM_LOAD16_BYTE( "d-18_u54.bin", 0x000001, 0x80000, CRC(218c4471) SHA1(6a64d7a6c18eb078a5848f4f97aa0c65e74ad3d9) )
	ROM_LOAD16_BYTE( "d-19_u55.bin", 0x100000, 0x80000, CRC(59507521) SHA1(8e0eaf8ecdfcaefccb5657278ecb2fea7489afb3) )
	ROM_LOAD16_BYTE( "d-20_u56.bin", 0x100001, 0x80000, CRC(e67c2c7d) SHA1(cddfd6a3d934e71853af62e3d2bf312618c9b4ff) )
ROM_END

ROM_START( puzzlove )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "3.u3", 0x00000, 0x40000, CRC(826c8472) SHA1(54f1a9fa0431de840d6fce466e09098a8d601660) )
	ROM_LOAD16_BYTE( "4.u2", 0x00001, 0x40000, CRC(64ddc708) SHA1(60b29f8cd5dd654be34452fd197a77abc872e63d) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "1.uz02", 0x00000, 0x20000, CRC(3077e7f3) SHA1(e2bf634a2166e1851486a801e74a7ec0d4599c28) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "2.uz11", 0x00000, 0x40000, CRC(4c06ec68) SHA1(3cfca1c98e73c65a45b65d43e012c5529572c057) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "10.u41",   0x000000, 0x20000, CRC(7200f878) SHA1(27c6389f802f6e0af0210e2b01788914c0eb1d04) )
	ROM_CONTINUE ( 0x080000,0x20000 )
	ROM_CONTINUE ( 0x040000,0x20000 )
	ROM_CONTINUE ( 0x0c0000,0x20000 )
	ROM_LOAD16_BYTE( "9.u42",   0x000001, 0x20000, CRC(21b1b297) SHA1(0f589d1c62d0f79b1379e4444b119bdc4cc70cfb) )
	ROM_CONTINUE ( 0x080001,0x20000 )
	ROM_CONTINUE ( 0x040001,0x20000 )
	ROM_CONTINUE ( 0x0c0001,0x20000 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "5.u53", 0x000000, 0x80000, CRC(8707d5a0) SHA1(05480ac34982a4e4768b7f3fccd2e557ca4b2545) )
	ROM_LOAD16_BYTE( "6.u54", 0x000001, 0x80000, CRC(60a6d614) SHA1(0693c08c51d6b3a05373c9999f01b0b8d23a1c89) )
	ROM_LOAD16_BYTE( "7.u55", 0x100000, 0x80000, CRC(0f2ea5c4) SHA1(4cb46fc6272e3cc14dfdcd7831157433ee7cf247) )
	ROM_LOAD16_BYTE( "8.u56", 0x100001, 0x80000, CRC(037dcd3d) SHA1(fcdf604710518982e0b4acc81a56fa703d0c9407) )
ROM_END

ROM_START( puzzlovek )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "3.u3", 0x00000, 0x40000, CRC(28f403f0) SHA1(5f9fc18f705fe81f0dceaf2d62b6caffb0b0462e) )
	ROM_LOAD16_BYTE( "4.u2", 0x00001, 0x40000, CRC(809371b9) SHA1(6b2dc899a4e2cf4fca777f0ec4a08e636099e0d2) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "1.uz02", 0x00000, 0x20000, CRC(3077e7f3) SHA1(e2bf634a2166e1851486a801e74a7ec0d4599c28) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "2.uz11", 0x00000, 0x40000, CRC(4c06ec68) SHA1(3cfca1c98e73c65a45b65d43e012c5529572c057) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "10.u41",  0x000000, 0x20000, CRC(3f952c54) SHA1(1f18579ce98305d20ec0f4e216f8170a62b9c68b) )
	ROM_CONTINUE ( 0x080000,0x20000 )
	ROM_CONTINUE ( 0x040000,0x20000 )
	ROM_CONTINUE ( 0x0c0000,0x20000 )
	ROM_LOAD16_BYTE( "9.u42",   0x000001, 0x20000, CRC(4f71ee93) SHA1(db0d00fa0f6c9b7bc638dcb50f041fb32827c7c4) )
	ROM_CONTINUE ( 0x080001,0x20000 )
	ROM_CONTINUE ( 0x040001,0x20000 )
	ROM_CONTINUE ( 0x0c0001,0x20000 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "5.u53", 0x000000, 0x80000, CRC(c5732995) SHA1(e7faecb19f4bdb103b782e38463d32b357ea63bc) )
	ROM_LOAD16_BYTE( "6.u54", 0x000001, 0x80000, CRC(a3b98fd1) SHA1(2b2b7c9df19882a0565e38504b73f56ea27d71ab) )
	ROM_LOAD16_BYTE( "7.u55", 0x100000, 0x80000, CRC(a4c73b48) SHA1(9f26af7b961c96cfd1c45f85f1d6dc4f364e3541) )
	ROM_LOAD16_BYTE( "8.u56", 0x100001, 0x80000, CRC(95b5f049) SHA1(1104dac1fbf6a894b7d8294b3f44a0edbf363157) )
ROM_END

GAME( 1995, silvmil,   0,        silvmil,   silvmil,   silvmil,  ROT270, "Para", "Silver Millennium", 0 )
GAME( 1994, puzzlove,  0,        puzzlove,  puzzlove,  puzzlove, ROT0,   "Para", "PuzzLove", 0 )
GAME( 1994, puzzlovek, puzzlove, puzzlovek, puzzlovek, puzzlove, ROT0,   "Para", "PuzzLove (Korea)", 0 )

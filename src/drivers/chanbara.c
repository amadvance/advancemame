// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina, David Haywood
/*
    Chanbara
    Data East, 1985
    Inputs and DIP switches by stephh.
*/

#include "driver.h"
#include "sound/2203intf.h"

static UINT8 *text_ram, *text_color, *back_ram, *back_color;
static tilemap *text_map, *back_map;
static UINT16 scroll;

static void get_text_info(int tile_index)
{
	int code = text_ram[tile_index] + ((text_color[tile_index] & 1) << 8);
	SET_TILE_INFO(0, code, (text_color[tile_index] >> 1) & 31, 0)
}

static void get_back_info(int tile_index)
{
	SET_TILE_INFO(2, back_ram[tile_index], (back_color[tile_index] >> 1) & 31,
		(back_color[tile_index] & 1) ? TILE_FLIPY : 0)
}

static WRITE8_HANDLER( text_w )
{
	text_ram[offset] = data;
	tilemap_mark_tile_dirty(text_map, offset);
}

static WRITE8_HANDLER( text_color_w )
{
	text_color[offset] = data;
	tilemap_mark_tile_dirty(text_map, offset);
}

static WRITE8_HANDLER( back_w )
{
	back_ram[offset] = data;
	tilemap_mark_tile_dirty(back_map, offset);
}

static WRITE8_HANDLER( back_color_w )
{
	back_color[offset] = data;
	tilemap_mark_tile_dirty(back_map, offset);
}

static PALETTE_INIT( chanbara )
{
	int i;
	for (i = 0; i < 256; ++i)
		palette_set_color(i, pal4bit(color_prom[i] << 1),
			pal4bit(color_prom[256 + i] << 1), pal4bit(color_prom[512 + i] << 1));
}

static VIDEO_START( chanbara )
{
	text_map = tilemap_create(get_text_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	back_map = tilemap_create(get_back_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 16, 32);
	if (!text_map || !back_map)
		return 1;
	tilemap_set_transparent_pen(text_map, 0);
	tilemap_set_transparent_pen(back_map, 0);
	return 0;
}

static void chanbara_sprites(mame_bitmap *bitmap, const rectangle *clip)
{
	int offset;
	for (offset = 0x7c; offset >= 0; offset -= 4)
	{
		int attr = spriteram[offset];
		int extra = spriteram[offset + 0x80];
		int code = spriteram[offset + 1];
		int color = extra & 15;
		int flipx = (attr >> 2) & 1, flipy = (attr >> 1) & 1;
		int x = (240 - spriteram[offset + 3]) & 255;
		int y = (240 - spriteram[offset + 2]) & 255;
		int mask = (extra & 0x80) ? 0xfffc : 0xfffe;
		if (!(attr & 1))
			continue;
		if (flip_screen)
		{
			x = 240 - x;
			y = 240 - y;
			flipx = !flipx;
			flipy = !flipy;
		}
		code |= ((extra & 0x10) << 5) | ((extra & 0x20) << 5) | ((extra & 0x40) << 2);
		if (attr & 0x10)
		{
			code &= ~1;
			if (flip_screen) y += 16;
			pdrawgfx(bitmap, Machine->gfx[1], code | (flipy ^ 1), color, flipx, flipy,
				x, y, clip, TRANSPARENCY_PEN, 0, mask);
			pdrawgfx(bitmap, Machine->gfx[1], code | flipy, color, flipx, flipy,
				x, y - 16, clip, TRANSPARENCY_PEN, 0, mask);
		}
		else
			pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy,
				x, y, clip, TRANSPARENCY_PEN, 0, mask);
	}
}

static VIDEO_UPDATE( chanbara )
{
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_set_scrolly(back_map, 0, scroll);
	/* Pen zero supplies the backdrop without blocking the low-priority sprites. */
	tilemap_draw(bitmap, cliprect, back_map, TILEMAP_IGNORE_TRANSPARENCY, 0);
	tilemap_draw(bitmap, cliprect, back_map, 0, 1);
	tilemap_draw(bitmap, cliprect, text_map, 0, 2);
	chanbara_sprites(bitmap, cliprect);
}

static WRITE8_HANDLER( ym_porta_w )
{
	scroll = (scroll & 0x100) | data;
}

static WRITE8_HANDLER( ym_portb_w )
{
	scroll = (scroll & 255) | ((data & 1) << 8);
	flip_screen_set((data >> 1) & 1);
	memory_set_bankptr(1, memory_region(REGION_USER1) + ((data & 4) ? 0x4000 : 0));
}

static void ym_irq(int state)
{
	/* The YM2203 timer holds IRQ until the program clears its status flag. */
	cpunum_set_input_line(0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym_interface =
{
	0, 0, ym_porta_w, ym_portb_w, ym_irq
};

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x0bff) AM_READWRITE(MRA8_RAM, text_w) AM_BASE(&text_ram)
	AM_RANGE(0x0c00, 0x0fff) AM_READWRITE(MRA8_RAM, text_color_w) AM_BASE(&text_color)
	AM_RANGE(0x1000, 0x10ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x1800, 0x19ff) AM_READWRITE(MRA8_RAM, back_w) AM_BASE(&back_ram)
	AM_RANGE(0x1a00, 0x1bff) AM_READWRITE(MRA8_RAM, back_color_w) AM_BASE(&back_color)
	AM_RANGE(0x2000, 0x2000) AM_READ(input_port_0_r)
	AM_RANGE(0x2001, 0x2001) AM_READ(input_port_1_r)
	AM_RANGE(0x2002, 0x2002) AM_READ(input_port_3_r)
	AM_RANGE(0x2003, 0x2003) AM_READ(input_port_2_r)
	AM_RANGE(0x3800, 0x3800) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x3801, 0x3801) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static MACHINE_RESET( chanbara )
{
	scroll = 0;
	flip_screen_set(0);
	memory_set_bankptr(1, memory_region(REGION_USER1));
}

INPUT_PORTS_START( chanbara )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )     PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )     PORT_DIPLOCATION("SW1:3,4")
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )      PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, "50k and 70k" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )    PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN )  PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP )    PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT )  PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN )   PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP )     PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )   PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT )  PORT_4WAY

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN )  PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP )    PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT )  PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN )   PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP )     PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )   PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT )  PORT_4WAY PORT_COCKTAIL
INPUT_PORTS_END

static const gfx_layout tilelayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ 0, 4 },
	{ RGN_FRAC(1,2)+0,  RGN_FRAC(1,2)+1, RGN_FRAC(1,2)+2, RGN_FRAC(1,2)+3, 0,1,2,3 },
	{ 0*8,1*8,2*8,3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tile16layout =
{
	16,16,
	RGN_FRAC(1,4),
	3,
	{ RGN_FRAC(1,2),0,4 },
	{
		16*8+RGN_FRAC(1,4)+0,16*8+ RGN_FRAC(1,4)+1,16*8+ RGN_FRAC(1,4)+2,16*8+ RGN_FRAC(1,4)+3,
		0,1,2,3,
		RGN_FRAC(1,4)+0,  RGN_FRAC(1,4)+1, RGN_FRAC(1,4)+2, RGN_FRAC(1,4)+3,
		16*8+0, 16*8+1, 16*8+2, 16*8+3,
	},
	{ 0*8,1*8,2*8,3*8, 4*8, 5*8, 6*8, 7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3),RGN_FRAC(1,3), 0 },
	{ 2*8*8+0,2*8*8+1,2*8*8+2,2*8*8+3,2*8*8+4,2*8*8+5,2*8*8+6,2*8*8+7,
	0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
	1*8*8+0*8,1*8*8+1*8,1*8*8+2*8,1*8*8+3*8,1*8*8+4*8,1*8*8+5*8,1*8*8+6*8,1*8*8+7*8 },
	16*16
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0x40, 32 },
	{ REGION_GFX2, 0, &spritelayout, 0x80, 16 },
	{ REGION_GFX3, 0, &tile16layout, 0, 32 },
	{ -1 }
};

static MACHINE_DRIVER_START( chanbara )
	MDRV_CPU_ADD(M6809, 12000000 / 8)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_MACHINE_RESET(chanbara)
	MDRV_FRAMES_PER_SECOND(6000000.0 / (384 * 272))
	MDRV_VBLANK_DURATION(1000000.0 * 384 * (272 - 240) / 6000000)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 272)
	MDRV_VISIBLE_AREA(0, 255, 8, 247)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(chanbara)
	MDRV_VIDEO_START(chanbara)
	MDRV_VIDEO_UPDATE(chanbara)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2203, 12000000 / 8)
	MDRV_SOUND_CONFIG(ym_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( chanbara )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cp01.16c",   0x8000, 0x4000, CRC(a0c3c24c) SHA1(8445dc39dd763187a2d66c6165b487f146e7d474) )
	ROM_LOAD( "cp00-2.17c", 0xc000, 0x4000, CRC(a045e463) SHA1(2eb546e16f163be6ed72238f2f0203527a957efd) )

	ROM_REGION( 0x8000, REGION_USER1, 0 )
	ROM_LOAD( "cp02.14c", 0x0000, 0x8000, CRC(c2b66cea) SHA1(f72f57add5f38313a72f5c521dce157edf49f70e) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cp12.17h", 0x0000, 0x2000, CRC(b87b96de) SHA1(f8bb9f094917df305c4fed071edaa775071e40fd) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cp13.15h", 0x0000, 0x4000, CRC(2dc38c3d) SHA1(4bb1335b8285e91b51c28e74d8de11a8d6df0486) )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "cp14.13h",   0x0000, 0x2000, CRC(d31db368) SHA1(b62834137bfe4ac2013d2d16b0ead10bf2a2df83) )

	ROM_REGION( 0x30000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD( "cp05.9c",  0x00000, 0x4000, CRC(df2dc3cb) SHA1(3505042c91566bb09fcd2102fecbe2034551b8eb) )
	ROM_LOAD( "cp04.10c", 0x04000, 0x4000, CRC(f7dce87b) SHA1(129ae41d70d96720e020ec1bc1d3f2d9e87ebf47) )
	ROM_LOAD( "cp03.12c", 0x08000, 0x4000, CRC(dea247fb) SHA1(d54fa30813613ef6c3b5f86b563e9ab618a9f627) )

	ROM_LOAD( "cp08.5c",  0x10000, 0x4000, CRC(4cf35192) SHA1(1891dcc412caf72ba5a2ea56c1cab35cb3ae6123) )
	ROM_LOAD( "cp07.6c",  0x14000, 0x4000, CRC(0e3727f2) SHA1(d177651bc20a56f5651ae5ce6f3d3ff7ad0e2053) )
	ROM_LOAD( "cp06.7c",  0x18000, 0x4000, CRC(2f337c08) SHA1(657ee6776780fa0a979a278ff27a49b459232cad) )

	ROM_LOAD( "cp11.1c",  0x20000, 0x4000, CRC(33e6160a) SHA1(b0171b554825072eebe935d12a6085d158b87bdc) )
	ROM_LOAD( "cp10.2c",  0x24000, 0x4000, CRC(bfa324c0) SHA1(c7ff09bb5f1dd2d3707970fae1fd60b6004250c0) )
	ROM_LOAD( "cp09.4c",  0x28000, 0x4000, CRC(3f58b647) SHA1(4eb212667aedd7c397a4911ac7f1b542c5c0a70d) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "cp17.4k", 0x0000, 0x0100, CRC(cf03706e) SHA1(2dd2b29067f418ec590c56a38cc64d09d8dc8e09) )
	ROM_LOAD( "cp16.5k", 0x0100, 0x0100, CRC(5fedc8ba) SHA1(8b685ce71d833fefb3e4502d1dd0cca96ba9162a) )
	ROM_LOAD( "cp15.6k", 0x0200, 0x0100, CRC(655936eb) SHA1(762b419c0571fafd8e1c5e96d0d94999768ba325) )
ROM_END

static DRIVER_INIT( chanbara )
{
	UINT8 *src = memory_region(REGION_GFX4);
	UINT8 *dst = memory_region(REGION_GFX3) + 0x4000;
	int i;
	/* The packed ROM supplies the third plane for both background tile halves. */
	for (i = 0; i < 0x1000; ++i)
	{
		dst[i + 0x1000] = src[i] & 0xf0;
		dst[i] = (src[i] & 15) << 4;
		dst[i + 0x3000] = src[i + 0x1000] & 0xf0;
		dst[i + 0x2000] = (src[i + 0x1000] & 15) << 4;
	}
}

GAME( 1985, chanbara, 0, chanbara, chanbara, chanbara, ROT270, "Data East Corporation", "Chanbara (Japan)", 0 )

// license:BSD-3-Clause
// copyright-holders:David Haywood, Bryan McPhail, Acho A. Tang, Nicola Salmoria
/*
    Black Touch '96 (D.G.R.M., 1996)
    D.G.R.M. NO 1947 hardware with 68000, PIC16C57 and two OKIM6295 chips.
    The game program keeps its timer at 20 and forces flip off during play.
*/

#include "driver.h"
#include "cpu/pic16c5x/pic16c5x.h"
#include "sound/okim6295.h"

static UINT16 *text_ram;
static tilemap *text_map;
static UINT8 sound_command, sound_ready, pic_port_c, pic_port_b, oki_selected, text_bank;
static int music_bank;

static void get_text_info(int tile_index)
{
	int code = (text_ram[tile_index * 2] & 255) | (text_bank << 8);
	SET_TILE_INFO(0, code, text_ram[tile_index * 2 + 1] & 15, 0)
}

static WRITE16_HANDLER( text_w )
{
	COMBINE_DATA(&text_ram[offset]);
	tilemap_mark_tile_dirty(text_map, offset / 2);
}

static READ16_HANDLER( sprite_r )
{
	return spriteram16[offset] | ((offset & 1) ? 0 : 0xff00);
}

static WRITE16_HANDLER( sprite_w )
{
	COMBINE_DATA(&spriteram16[offset]);
	/* Only three bytes per entry are populated; the fourth byte floats high. */
	if (!(offset & 1))
		spriteram16[offset] |= 0xff00;
}

static void draw_group(mame_bitmap *bitmap, const rectangle *clip, int group)
{
	int offset;
	for (offset = 0; offset < 0x800; offset += 0x40)
	{
		UINT16 *tiles = spriteram16 + group * 0x800 + offset;
		int yreg = spriteram16[offset + group * 2 + 1];
		int x = ((spriteram16[offset + group * 2] & 255) << 4) | (yreg >> 12);
		int y = -yreg;
		int i;
		x = ((x + 16) & 511) - 16;
		if (flip_screen)
		{
			x = 240 - x;
			y = 240 - y;
		}
		for (i = 0; i < 32; ++i, tiles += 2)
		{
			y &= 511;
			if (y <= clip->max_y && y + 15 >= clip->min_y)
			{
				int tile = tiles[1];
				int color = tiles[0] & 127;
				int fx = (tile & 0x4000) != 0, fy = (tile & 0x8000) != 0;
				int gfx = 2;
				tile &= 0x3fff;
				if (tile & 0x2000)
				{
					gfx = 1;
					tile &= 0x1fff;
					color >>= 4;
				}
				if (flip_screen) { fx = !fx; fy = !fy; }
				drawgfx(bitmap, Machine->gfx[gfx], tile, color, fx, fy,
					x, y, clip, TRANSPARENCY_PEN, 0);
			}
			y += flip_screen ? -16 : 16;
		}
	}
}

static VIDEO_START( blackt96 )
{
	text_map = tilemap_create(get_text_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	if (!text_map)
		return 1;
	tilemap_set_transparent_pen(text_map, 0);
	return 0;
}

static VIDEO_UPDATE( blackt96 )
{
	fillbitmap(bitmap, get_black_pen(), cliprect);
	draw_group(bitmap, cliprect, 2);
	draw_group(bitmap, cliprect, 3);
	draw_group(bitmap, cliprect, 1);
	tilemap_draw(bitmap, cliprect, text_map, 0, 0);
}

static WRITE16_HANDLER( output_w )
{
	if (ACCESSING_LSB)
	{
		int bank = (data >> 4) & 7;
		if (bank != text_bank)
		{
			text_bank = bank;
			tilemap_mark_all_tiles_dirty(text_map);
		}
		flip_screen_set((data >> 3) & 1);
		coin_counter_w(0, data & 1);
		coin_counter_w(1, data & 2);
	}
}

static WRITE16_HANDLER( sound_w )
{
	if (ACCESSING_MSB)
	{
		sound_command = data >> 8;
		sound_ready = 1;
		cpu_boost_interleave(TIME_IN_USEC(10), TIME_IN_MSEC(1));
	}
}

static READ16_HANDLER( unknown_r ) { return mame_rand(); }

static WRITE8_HANDLER( pic_a_w )
{
	int bank = data & 7;
	if (bank != music_bank)
	{
		/* Preserve the first 192 KB and flush audio before changing the upper window. */
		OKIM6295_set_bank_base(0, 0);
		memcpy(memory_region(REGION_SOUND1) + 0x30000,
			memory_region(REGION_USER1) + bank * 0x10000, 0x10000);
		music_bank = bank;
	}
}

static READ8_HANDLER( pic_b_r ) { return pic_port_b; }
static WRITE8_HANDLER( pic_b_w ) { pic_port_b = data; }
static READ8_HANDLER( pic_c_r ) { return sound_ready ? 0x40 : 0; }

static WRITE8_HANDLER( pic_c_w )
{
	UINT8 falling = pic_port_c & ~data;
	/* Each device operation is latched by a falling edge, not by its held level. */
	if (falling & 0x20) sound_ready = 0;
	if (falling & 0x10) pic_port_b = sound_command;
	if (falling & 0x08) oki_selected = 1;
	if (falling & 0x04) oki_selected = 0;
	if (falling & 0x02)
	{
		if (oki_selected) OKIM6295_data_1_w(0, pic_port_b);
		else OKIM6295_data_0_w(0, pic_port_b);
	}
	if (falling & 0x01)
		pic_port_b = oki_selected ? OKIM6295_status_1_r(0) : OKIM6295_status_0_r(0);
	pic_port_c = data;
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READWRITE(input_port_0_word_r, sound_w)
	AM_RANGE(0x0c0000, 0x0c0001) AM_READWRITE(input_port_1_word_r, output_w)
	AM_RANGE(0x0e0000, 0x0e0001) AM_READ(unknown_r)
	AM_RANGE(0x0e8000, 0x0e8001) AM_READ(unknown_r)
	AM_RANGE(0x0f0000, 0x0f0001) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0f0008, 0x0f0009) AM_READWRITE(input_port_3_word_r, MWA16_NOP)
	AM_RANGE(0x100000, 0x100fff) AM_READWRITE(MRA16_RAM, text_w) AM_BASE(&text_ram)
	AM_RANGE(0x200000, 0x207fff) AM_READWRITE(sprite_r, sprite_w) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(MRA16_RAM, paletteram16_xxxxRRRRGGGGBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xc00000, 0xc03fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pic_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0, 0) AM_WRITE(pic_a_w)
	AM_RANGE(1, 1) AM_READWRITE(pic_b_r, pic_b_w)
	AM_RANGE(2, 2) AM_READWRITE(pic_c_r, pic_c_w)
ADDRESS_MAP_END

static MACHINE_RESET( blackt96 )
{
	sound_command = sound_ready = pic_port_c = pic_port_b = oki_selected = text_bank = 0;
	music_bank = -1;
	pic_a_w(0, 0);
	flip_screen_set(0);
	tilemap_mark_all_tiles_dirty(text_map);
}

INPUT_PORTS_START( blackt96 )
	PORT_START_TAG("P1_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x0300, 0x0000, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c00, 0x0000, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(      0x0c00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x1000, "3" )
	PORT_DIPNAME( 0x2000, 0x2000, "Bonus Life Type" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(      0x2000, "Every" )
	PORT_DIPSETTING(      0x0000, "Second Only" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Flip_Screen ) ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Yes ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Service_Mode ) ) PORT_DIPLOCATION("SW2:8") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0000, "Continue" ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c00, 0x0400, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW2:5,6")
	PORT_DIPSETTING(      0x0000, "20000 / 50000" )
	PORT_DIPSETTING(      0x0400, "60000 / 150000" )
	PORT_DIPSETTING(      0x0800, "40000 / 100000" )
	PORT_DIPSETTING(      0x0c00, "No Bonus" )
	PORT_DIPNAME( 0x3000, 0x0000, "Demo Sound / Video Freeze" ) PORT_DIPLOCATION("SW2:3,4")
	PORT_DIPSETTING(      0x0000, "Demo Sound On" )
	PORT_DIPSETTING(      0x1000, "Never Finish" )
	PORT_DIPSETTING(      0x2000, "Demo Sound Off" )
	PORT_DIPSETTING(      0x3000, "Stop Video" )
	PORT_DIPNAME( 0xc000, 0x0000, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(      0x8000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x4000, "3" )
	PORT_DIPSETTING(      0xc000, "4" )
INPUT_PORTS_END

static const gfx_layout blackt96_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{  1024+32, 1024+40, 1024+48, 1024+56, 1024+0, 1024+8, 1024+16, 1024+24,
		32,40,48,56,0,8,16,24 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	32*64
};

static const gfx_layout blackt962_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,24,8, 16 },
	{ 519, 515, 518, 514,  517,513,  516,512, 7,3,6,2,5,1,4,0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32 },
	32*32
};

static const gfx_layout blackt96_text_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,4,8, 12 },
	{ 131,130,129,128,3,2,1,0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*16
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &blackt96_text_layout, 0, 16 },
	{ REGION_GFX2, 0, &blackt96_layout, 0, 8 },
	{ REGION_GFX3, 0, &blackt962_layout, 0, 128 },
	{ -1 }
};

static MACHINE_DRIVER_START( blackt96 )
	MDRV_CPU_ADD(M68000, 18000000 / 2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold, 1)
	MDRV_CPU_ADD(PIC16C57, 4000000 / PIC16C5x_CLOCK_DIVIDER)
	MDRV_CPU_IO_MAP(pic_map, 0)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_RESET(blackt96)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	MDRV_VIDEO_START(blackt96)
	MDRV_VIDEO_UPDATE(blackt96)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.47)
	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.47)
MACHINE_DRIVER_END

ROM_START( blackt96 )
	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "3", 0x00001, 0x40000, CRC(fc2c1d79) SHA1(742478237819af16d3fd66039283202b3c07eedd) )
	ROM_LOAD16_BYTE( "4", 0x00000, 0x40000, CRC(caff5b4a) SHA1(9a388cbb07211fa66f27082a8a5b847168c86a4f) )

	ROM_REGION( 0x2000, REGION_CPU2, 0 )
	ROM_LOAD( "pic16c57.bin", 0x00000, 0x2000, CRC(6053ba2f) SHA1(5dd28ddff17555de0e8574b78ff9e71204c503d3) )

	ROM_REGION( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "1", 0x00000, 0x80000, CRC(6a934174) SHA1(087f5fa226dc68ee217f99c64d16cdf14372d44c) )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 )
	ROM_LOAD( "2", 0x00000, 0x40000, CRC(94009cd4) SHA1(aa36298e280c20bf86d70f3eb3fb33aca4df07e3) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "5", 0x100000, 0x40000, CRC(6e52c331) SHA1(31ef1d352d4ee5f7b3ef336b1f052c3a1468f22e) )
	ROM_LOAD16_BYTE( "6", 0x100001, 0x40000, CRC(69637a5a) SHA1(a5731478856d8bb91d34b747838b2b47772864ef) )
	ROM_LOAD16_BYTE( "7", 0x000000, 0x80000, CRC(6b04e8a8) SHA1(309ba1efd60600a30e1ae8f6e8b92939c23cd9c6) )
	ROM_LOAD16_BYTE( "8", 0x000001, 0x80000, CRC(16c656be) SHA1(06c40c16080a97b01a638776d28f36594ce4fb3b) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "11", 0x00000, 0x40000, CRC(9eb773a3) SHA1(9c91ee938438a61f5fa650ced6249e34aa5321bd) )
	ROM_LOAD32_BYTE( "12", 0x00001, 0x40000, CRC(8894e608) SHA1(389974a0b208b7cbf7d5f83641ddc058ad5ebe87) )
	ROM_LOAD32_BYTE( "13", 0x00002, 0x40000, CRC(0acceb9d) SHA1(e8a85c7eab45d84613ac37a9b7ffbc45b44eb2e5) )
	ROM_LOAD32_BYTE( "14", 0x00003, 0x40000, CRC(b5e3de25) SHA1(33ac5602ab6bcadc8b0d1aa805a3bdce0b67c215) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "9",  0x00000, 0x08000, CRC(81a4cf4c) SHA1(94b2bbcbc8327d9babbc3b222bd88954c7e7b80e) )
	ROM_CONTINUE(          0x00000, 0x08000 )
	ROM_LOAD16_BYTE( "10", 0x00001, 0x08000, CRC(b78232a2) SHA1(36a4f01011faf64e46b73f0082ab04843ac8b0e2) )
	ROM_CONTINUE(          0x00001, 0x08000 )
ROM_END

static DRIVER_INIT( blackt96 )
{
	memcpy(memory_region(REGION_SOUND1), memory_region(REGION_USER1), 0x30000);
	pic16c5x_config(0);
}

GAME( 1996, blackt96, 0, blackt96, blackt96, blackt96, ROT0, "D.G.R.M.", "Black Touch '96", 0 )

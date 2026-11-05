// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
    Super Duck (c) 1992 Comad
    68000, Z80 and OKIM6295 hardware with Capcom-style tilemaps and sprites.
*/

#include "driver.h"
#include "sound/okim6295.h"

static UINT16 *text_ram, *fore_ram, *back_ram;
static tilemap *text_map, *fore_map, *back_map;
static UINT16 scroll_regs[4];
static int oki_bank;

static UINT32 supduck_scan(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	return (((col & ~7) * 8 + ((row ^ 63) & 7) * 8 + (col & 7)) & 0x3ff)
		+ (((row ^ 63) & ~7) / 8) * 0x400;
}

static void get_text_info(int tile_index)
{
	UINT16 data = text_ram[tile_index];
	int attr = data >> 8;
	int code = (data & 255) + ((attr & 0xc0) << 2) + ((attr & 0x20) << 5);
	SET_TILE_INFO(0, code, attr & 15, (attr & 0x10) ? TILE_FLIPY : 0)
}

static void get_fore_info(int tile_index)
{
	UINT16 data = fore_ram[tile_index];
	int code = (data & 255) | ((data & 0xc000) >> 6);
	int flags = ((data & 0x2000) ? TILE_FLIPX : 0) | ((data & 0x1000) ? TILE_FLIPY : 0);
	SET_TILE_INFO(1, code, (data >> 8) & 15, flags)
}

static void get_back_info(int tile_index)
{
	UINT16 data = back_ram[tile_index];
	int code = (data & 255) | ((data & 0xc000) >> 6);
	int flags = ((data & 0x2000) ? TILE_FLIPX : 0) | ((data & 0x1000) ? TILE_FLIPY : 0);
	SET_TILE_INFO(2, code, (data >> 8) & 15, flags)
}

static WRITE16_HANDLER( text_w )
{
	COMBINE_DATA(&text_ram[offset]);
	tilemap_mark_tile_dirty(text_map, offset);
}

static WRITE16_HANDLER( fore_w )
{
	COMBINE_DATA(&fore_ram[offset]);
	tilemap_mark_tile_dirty(fore_map, offset);
}

static WRITE16_HANDLER( back_w )
{
	COMBINE_DATA(&back_ram[offset]);
	tilemap_mark_tile_dirty(back_map, offset);
}

static WRITE16_HANDLER( scroll_w )
{
	COMBINE_DATA(&scroll_regs[offset]);
	switch (offset)
	{
		case 0: tilemap_set_scrollx(back_map, 0, scroll_regs[0]); break;
		case 1: tilemap_set_scrolly(back_map, 0, -scroll_regs[1] - 256); break;
		case 2: tilemap_set_scrollx(fore_map, 0, scroll_regs[2]); break;
		case 3: tilemap_set_scrolly(fore_map, 0, -scroll_regs[3] - 256); break;
	}
}

static WRITE16_HANDLER( palette_w )
{
	int r, g, b;
	COMBINE_DATA(&paletteram16[offset]);
	data = paletteram16[offset];
	/* The three extra channel bits are the most significant bits. */
	r = ((data >> 8) & 15) | ((data >> 10) & 16);
	g = ((data >> 4) & 15) | ((data >> 9) & 16);
	b = (data & 15) | ((data >> 8) & 16);
	palette_set_color(offset, pal5bit(r), pal5bit(g), pal5bit(b));
}

static VIDEO_START( supduck )
{
	text_map = tilemap_create(get_text_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	fore_map = tilemap_create(get_fore_info, supduck_scan, TILEMAP_TRANSPARENT, 32, 32, 128, 64);
	back_map = tilemap_create(get_back_info, supduck_scan, TILEMAP_OPAQUE, 32, 32, 128, 64);
	if (!text_map || !fore_map || !back_map)
		return 1;
	tilemap_set_transparent_pen(text_map, 3);
	tilemap_set_transparent_pen(fore_map, 15);
	return 0;
}

static VIDEO_UPDATE( supduck )
{
	int offset;
	tilemap_draw(bitmap, cliprect, back_map, 0, 0);
	tilemap_draw(bitmap, cliprect, fore_map, 0, 0);
	for (offset = spriteram_size / 2 - 4; offset >= 0; offset -= 4)
	{
		UINT16 *sprite = buffered_spriteram16 + offset;
		int x = sprite[3] & 0x1ff;
		int y = sprite[2] & 0x1ff;
		if (x > 0x100) x -= 0x200;
		if (y > 0x100) y -= 0x200;
		drawgfx(bitmap, Machine->gfx[3], sprite[0], (sprite[1] >> 2) & 15,
			sprite[1] & 2, sprite[1] & 1, x, 240 - y, cliprect, TRANSPARENCY_PEN, 15);
	}
	tilemap_draw(bitmap, cliprect, text_map, 0, 0);
}

static VIDEO_EOF( supduck )
{
	/* The sprite generator reads the VBLANK copy rather than live CPU RAM. */
	buffer_spriteram16_w(0, 0, 0);
}

static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(0, data >> 8);
		cpunum_set_input_line(1, 0, ASSERT_LINE);
		cpu_boost_interleave(0, TIME_IN_USEC(100));
	}
}

static READ8_HANDLER( sound_command_r )
{
	UINT8 data = soundlatch_r(0);
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	return data;
}

static WRITE16_HANDLER( coin_w )
{
	if (ACCESSING_MSB)
	{
		coin_counter_w(0, data & 0x4000);
		coin_counter_w(1, data & 0x8000);
	}
}

static WRITE8_HANDLER( oki_bank_w )
{
	int bank = data & 3;
	if (bank != oki_bank)
	{
		/* Finish queued samples against the old bank before replacing its window. */
		OKIM6295_set_bank_base(0, 0);
		memcpy(memory_region(REGION_SOUND1) + 0x20000,
			memory_region(REGION_USER1) + bank * 0x20000, 0x20000);
		oki_bank = bank;
	}
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READWRITE(MRA16_ROM, MWA16_NOP)
	AM_RANGE(0xfe0000, 0xfe1fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0xfe4000, 0xfe4001) AM_READWRITE(input_port_0_word_r, coin_w)
	AM_RANGE(0xfe4002, 0xfe4003) AM_READWRITE(input_port_1_word_r, sound_command_w)
	AM_RANGE(0xfe4004, 0xfe4005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfe8000, 0xfe8007) AM_WRITE(scroll_w)
	AM_RANGE(0xfe800e, 0xfe800f) AM_WRITENOP
	AM_RANGE(0xfec000, 0xfecfff) AM_READWRITE(MRA16_RAM, text_w) AM_BASE(&text_ram)
	AM_RANGE(0xff0000, 0xff3fff) AM_READWRITE(MRA16_RAM, back_w) AM_BASE(&back_ram)
	AM_RANGE(0xff4000, 0xff7fff) AM_READWRITE(MRA16_RAM, fore_w) AM_BASE(&fore_ram)
	AM_RANGE(0xff8000, 0xff87ff) AM_READWRITE(MRA16_RAM, palette_w) AM_BASE(&paletteram16)
	AM_RANGE(0xffc000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x9000, 0x9000) AM_WRITE(oki_bank_w)
	AM_RANGE(0x9800, 0x9800) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xa000, 0xa000) AM_READ(sound_command_r)
ADDRESS_MAP_END

static MACHINE_RESET( supduck )
{
	oki_bank = -1;
	oki_bank_w(0, 0);
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	memset(scroll_regs, 0, sizeof(scroll_regs));
}

INPUT_PORTS_START( supduck )
	PORT_START_TAG("P1_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) ) PORT_DIPLOCATION("DIP-A:1,2,3")
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) ) PORT_DIPLOCATION("DIP-A:4")
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("DIP-A:5")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Game Sound" ) PORT_DIPLOCATION("DIP-A:6")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x0080, DEF_STR( Lives ) ) PORT_DIPLOCATION("DIP-A:7,8")
	PORT_DIPSETTING(      0x00c0, "2" )
	PORT_DIPSETTING(      0x0080, "3" )
	PORT_DIPSETTING(      0x0040, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) ) PORT_DIPLOCATION("DIP-B:1")
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) ) PORT_DIPLOCATION("DIP-B:2")
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) ) PORT_DIPLOCATION("DIP-B:3")
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) ) PORT_DIPLOCATION("DIP-B:4")
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) ) PORT_DIPLOCATION("DIP-B:5")
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) ) PORT_DIPLOCATION("DIP-B:6")
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Character Test" ) PORT_DIPLOCATION("DIP-B:7")
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Service_Mode ) ) PORT_DIPLOCATION("DIP-B:8") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_DIPSETTING( 0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout vramlayout=
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4,0 },
	{ STEP4(0,1), STEP4(4*2,1) },
	{ STEP8(0,16) },
	128
};

static const gfx_layout tile_layout =
{
	32, 32,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ STEP4(0,1),        STEP4(4*2,1),          STEP4(4*2*2*32,1), STEP4(4*2*2*32+4*2,1),
	  STEP4(4*2*2*64,1), STEP4(4*2*2*64+4*2,1), STEP4(4*2*2*96,1), STEP4(4*2*2*96+4*2,1) },
	{ STEP16(0,16), STEP16(16*16,16) },
	256 * 8
};

static const gfx_layout sprite_layout =
{
	16, 16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,8) },
	{ STEP8(0,1), STEP8(8*4*16,1) },
	{ STEP16(0,8*4) },
	16*16*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &vramlayout, 768, 64 },
	{ REGION_GFX2, 0, &tile_layout, 0, 16 },
	{ REGION_GFX3, 0, &tile_layout, 256, 16 },
	{ REGION_GFX4, 0, &sprite_layout, 512, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( supduck )
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(irq2_line_hold, 1)
	MDRV_CPU_ADD(Z80, 8000000 / 4)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_MACHINE_RESET(supduck)
	MDRV_INTERLEAVE(10)
	MDRV_FRAMES_PER_SECOND(6000000.0 / (384 * 262))
	MDRV_VBLANK_DURATION(1000000.0 * 384 * (262 - 224) / 6000000)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(384, 262)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_VIDEO_START(supduck)
	MDRV_VIDEO_UPDATE(supduck)
	MDRV_VIDEO_EOF(supduck)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( supduck )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "5.u16n", 0x00000, 0x20000, CRC(837a559a) SHA1(ed5ad744a4145dfbef56ad2e6eec3ff14c20de1c) )
	ROM_LOAD16_BYTE( "6.u16l", 0x00001, 0x20000, CRC(508e9905) SHA1(2da3f12caa29066b4d54b22573cfdfcea8916f99) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "4.su6",  0x00000, 0x8000, CRC(d75863ea) SHA1(497d11b86f4f69134943fc3448d195c6e7acbe8f) )

	ROM_REGION( 0x08000, REGION_GFX1, 0 )
	ROM_LOAD( "3.cu15",   0x00000, 0x8000, CRC(b1cacca4) SHA1(b4a486618197cf2b85a121b5640cd773b2d453fc) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD( "7.uu29",   0x00000, 0x20000, CRC(f3251b20) SHA1(8ebb9b98324de14356c9a57ae8a77dc4118fb5c2) )
	ROM_LOAD( "8.uu30",   0x20000, 0x20000, CRC(03c60cbd) SHA1(bf3be7161f69187350eb9d9d4209b93f8b67d0f1) )
	ROM_LOAD( "9.uu31",   0x40000, 0x20000, CRC(9b6d3430) SHA1(ade2decc5bcf817498b1198a2244d1c65bc20bea) )
	ROM_LOAD( "10.uu32",  0x60000, 0x20000, CRC(beed2616) SHA1(c077a3de4a6d451a568694ab70e85830d585a41d) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )
	ROM_LOAD( "11.ul29",   0x00000, 0x20000, CRC(1b6958a4) SHA1(ca93f898702e14ece24d5cfced38d622d3596d0f) )
	ROM_LOAD( "12.ul30",   0x20000, 0x20000, CRC(3e6bd24b) SHA1(f93b5c78d815bd30ecb9cfe2cd257548e467e852) )
	ROM_LOAD( "13.ul31",   0x40000, 0x20000, CRC(bff7b7cd) SHA1(2f65cadcfcc02fe31ba721eea9f45d4a729e4374) )
	ROM_LOAD( "14.ul32",   0x60000, 0x20000, CRC(97a7310b) SHA1(76b82bfea64b59890c0ba2e1688b7321507a4da7) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 )
	ROM_LOAD32_BYTE( "15.u1d",   0x00000, 0x20000, CRC(81bf1f27) SHA1(7a66630a2da85387904917d3c136880dffcb9649) )
	ROM_LOAD32_BYTE( "16.u2d",   0x00001, 0x20000, CRC(9573d6ec) SHA1(9923be782bae47c49913d01554bcf3e5efb5395b) )
	ROM_LOAD32_BYTE( "17.u1c",   0x00002, 0x20000, CRC(21ef14d4) SHA1(66e389aaa1186921a07da9a9a9eda88a1083ad42) )
	ROM_LOAD32_BYTE( "18.u2c",   0x00003, 0x20000, CRC(33dd0674) SHA1(b95dfcc16d939bac77f338b8a8cada19328a1993) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "2.su12",   0x00000, 0x20000, CRC(745d42fb) SHA1(f9aee3ddbad3cc2f3a7002ee0d762eb041967e1e) )

	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "1.su13",   0x00000, 0x80000, CRC(7fb1ed42) SHA1(77ec86a6454398e329066aa060e9b6a39085ce71) )
ROM_END

GAME( 1992, supduck, 0, supduck, supduck, 0, ROT0, "Comad", "Super Duck", 0 )

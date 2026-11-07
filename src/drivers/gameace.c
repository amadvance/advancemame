// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
Hot Body I

ACE9412 / DM941204
Z80: 6 MHz main CPU, 4 MHz audio CPU
YM2151: 4 MHz, OKIM6295: 1 MHz
*/

#include "driver.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

static UINT8 *gameace_fgram, *gameace_spriteram, *gameace_colram, *gameace_paletteram;
static UINT8 gameace_bank, gameace_vidbank;
static UINT16 gameace_palbank;
static tilemap *gameace_tilemap;

static void gameace_get_tile_info(int tile_index)
{
	int code = gameace_fgram[tile_index * 2] | (gameace_fgram[tile_index * 2 + 1] << 8);
	SET_TILE_INFO(0, code, gameace_colram[tile_index], 0)
}

static void gameace_update_color(int offset)
{
	int color = gameace_paletteram[offset & ~1] | (gameace_paletteram[offset | 1] << 8);
	palette_set_color(offset / 2, pal5bit(color >> 10), pal5bit(color >> 5), pal5bit(color));
}

static WRITE8_HANDLER( gameace_bank_w )
{
	gameace_bank = data & 15;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + gameace_bank * 0x4000);
}

static WRITE8_HANDLER( gameace_vidbank_w ) { gameace_vidbank = data & 1; }
static WRITE8_HANDLER( gameace_palbank_w ) { gameace_palbank = (data & 0x20) << 6; }

static READ8_HANDLER( gameace_palette_r )
{
	return gameace_paletteram[offset + gameace_palbank];
}

static WRITE8_HANDLER( gameace_palette_w )
{
	offset += gameace_palbank;
	gameace_paletteram[offset] = data;
	gameace_update_color(offset);
}

static WRITE8_HANDLER( gameace_colram_w )
{
	gameace_colram[offset] = data;
	tilemap_mark_tile_dirty(gameace_tilemap, offset);
}

static READ8_HANDLER( gameace_video_r )
{
	return gameace_vidbank ? gameace_spriteram[offset] : gameace_fgram[offset];
}

static WRITE8_HANDLER( gameace_video_w )
{
	/* Sprite writes must not change the tile RAM hidden behind the same addresses. */
	if (gameace_vidbank)
		gameace_spriteram[offset] = data;
	else
	{
		gameace_fgram[offset] = data;
		tilemap_mark_tile_dirty(gameace_tilemap, offset / 2);
	}
}

static void gameace_postload(void)
{
	int i;
	gameace_bank_w(0, gameace_bank);
	tilemap_mark_all_tiles_dirty(gameace_tilemap);
	for (i = 0; i < 0x1000; i += 2)
		gameace_update_color(i);
}

static VIDEO_START( gameace )
{
	gameace_tilemap = tilemap_create(gameace_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 64, 32);
	if (!gameace_tilemap)
		return 1;
	gameace_fgram = auto_malloc(0x1000);
	gameace_spriteram = auto_malloc(0x1000);
	gameace_paletteram = auto_malloc(0x1000);
	memset(gameace_fgram, 0, 0x1000);
	memset(gameace_spriteram, 0, 0x1000);
	memset(gameace_paletteram, 0, 0x1000);
	state_save_register_global_pointer(gameace_fgram, 0x1000);
	state_save_register_global_pointer(gameace_spriteram, 0x1000);
	state_save_register_global_pointer(gameace_paletteram, 0x1000);
	state_save_register_global(gameace_bank);
	state_save_register_global(gameace_vidbank);
	state_save_register_global(gameace_palbank);
	state_save_register_func_postload(gameace_postload);
	return 0;
}

static VIDEO_UPDATE( gameace )
{
	int offs;
	tilemap_draw(bitmap, cliprect, gameace_tilemap, 0, 0);
	for (offs = 0x1000 - 0x40; offs >= 0; offs -= 0x20)
	{
		int attr = gameace_spriteram[offs + 0x13];
		int code = gameace_spriteram[offs + 0x10] + ((attr & 0xe0) << 3);
		int sx = gameace_spriteram[offs + 0x12] + ((attr & 0x10) << 4);
		int sy = 255 - gameace_spriteram[offs + 0x11];
		drawgfx(bitmap, Machine->gfx[1], code, attr & 15, 0, 0,
			sx - 7, sy - 14, cliprect, TRANSPARENCY_PEN, 15);
	}
}

static WRITE8_HANDLER( gameace_sound_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
}

static READ8_HANDLER( gameace_sound_r )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
	return soundlatch_r(0);
}

static void gameace_sound_irq(int state)
{
	cpunum_set_input_line(1, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct YM2151interface gameace_ym2151 = { gameace_sound_irq, 0 };

static MACHINE_RESET( gameace )
{
	gameace_bank_w(0, 7);
	gameace_vidbank = gameace_palbank = 0;
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
}

static ADDRESS_MAP_START( gameace_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xc7ff) AM_READWRITE(gameace_palette_r, gameace_palette_w)
	AM_RANGE(0xc800, 0xcfff) AM_RAM AM_WRITE(gameace_colram_w) AM_BASE(&gameace_colram)
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(gameace_video_r, gameace_video_w)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( gameace_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_4_r)
	AM_RANGE(0x02, 0x02) AM_READWRITE(input_port_0_r, gameace_vidbank_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x05, 0x05) AM_WRITE(gameace_sound_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(input_port_2_r, gameace_bank_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(input_port_3_r, gameace_palbank_w)
ADDRESS_MAP_END

static READ8_HANDLER( gameace_sound_status_r ) { return 0; }

static ADDRESS_MAP_START( gameace_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc001) AM_READWRITE(YM2151_status_port_0_r, YM2151_word_0_w)
	AM_RANGE(0xc002, 0xc003) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xc006, 0xc006) AM_READ(gameace_sound_r)
	AM_RANGE(0xc00f, 0xc00f) AM_READWRITE(gameace_sound_status_r, MWA8_NOP)
	AM_RANGE(0xd000, 0xd7ff) AM_RAM
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static INPUT_PORTS_START( hotbody )

	PORT_START
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE(0x80, IP_ACTIVE_LOW)
INPUT_PORTS_END

static const gfx_layout tilelayout =
{
	8, 8, RGN_FRAC(1,4), 4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2) + 4, RGN_FRAC(1,2) + 0, 4, 0},
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3, 32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	64*8
};

static const gfx_decode gameace_gfxdecode[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0, 0x80 },
	{ REGION_GFX2, 0, &spritelayout, 0, 0x80 },
	{ -1 }
};

static MACHINE_DRIVER_START( gameace )
	MDRV_CPU_ADD(Z80, 12000000/2)
	MDRV_CPU_PROGRAM_MAP(gameace_map, 0)
	MDRV_CPU_IO_MAP(gameace_io, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(gameace_sound_map, 0)
	MDRV_MACHINE_RESET(gameace)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(64, 447, 8, 247)
	MDRV_GFXDECODE(gameace_gfxdecode)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_VIDEO_START(gameace)
	MDRV_VIDEO_UPDATE(gameace)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(gameace_ym2151)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD(OKIM6295, 1000000/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( hotbody )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "2.14b", 0x00000, 0x40000, CRC(4eff1b0c) SHA1(d2b443b59f50fa9013f528c18b0d38da7c938d22) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "1.4b", 0x00000, 0x20000, CRC(87e15d1d) SHA1(648d29dbf35638639bbf2ffbcd594e455cecaed2) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_INVERT )
	ROM_LOAD( "3.1f", 0x00000, 0x20000, CRC(680ad651) SHA1(c1e53e7ab0b39d1ab4b6769f64323759ebb976c2) )
	ROM_LOAD( "4.2f", 0x20000, 0x20000, CRC(33d7cf7b) SHA1(8ed80382e727bee8ccfa7c24aac8b3058264c398) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_INVERT )
	ROM_LOAD( "6.14g", 0x00000, 0x40000, CRC(c5f744b1) SHA1(0e979f41d7e0a66b45a789384e6a6008e539798a) )
	ROM_LOAD( "8.17g", 0x40000, 0x40000, CRC(4328f371) SHA1(3a5d1c0afb671943234120a0758077f76712f624) )
	ROM_LOAD( "5.13g", 0x80000, 0x40000, CRC(70341256) SHA1(5763351b0c6cb83b4fddd93a2b6a95b96adac148) )
	ROM_LOAD( "7.16g", 0xc0000, 0x40000, CRC(bce62a37) SHA1(8f340af1dd74f2a1b7b13c903abb2806a6a5c6dc) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD( "5a", 0x00000, 0x20000, CRC(2404da21) SHA1(1333634112eef8664b5d72af5fc57c4c800ce00d) )

	ROM_REGION( 0x800, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "pal1.4d",  0x000, 0x117, NO_DUMP )
	ROM_LOAD( "pal2.10c", 0x200, 0x157, NO_DUMP )
	ROM_LOAD( "pal3.17e", 0x400, 0x117, NO_DUMP )
	ROM_LOAD( "pal4.1d",  0x600, 0x157, NO_DUMP )
ROM_END

static void decode_cpu()
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *buffer = malloc(0x40000);
	int i;
	if (!buffer)
		fatalerror("Unable to allocate ROM decode buffer");
	memcpy(&buffer[0], rom, 0x40000);

	static const UINT8 swap_code[0x10] = { 0x0a, 0x07, 0x08, 0x05,   0x06, 0x03, 0x04, 0x01,   0x02, 0x0f, 0x00, 0x0d,   0x0e, 0x0b, 0x0c, 0x09 };
	static const UINT8 swap_data[0x10] = { 0x03, 0x02, 0x01, 0x00,   0x05, 0x04, 0x07, 0x06,   0x0d, 0x0c, 0x0f, 0x0e,   0x09, 0x08, 0x0b, 0x0a };

	for (i = 0x00000; i < 0x40000; i++)
	{

		UINT8 col = i & 0x0f;
		UINT32 addr;

		if (i < 0x10000)
		{
			addr = swap_code[col];
		}
		else
		{
			addr = swap_data[col];
		}

		addr = (i & 0x3fff0) | addr;

		rom[i] = buffer[addr];
	}
	free(buffer);
}

static void decode_sprites()
{
	UINT8 *rom = memory_region(REGION_GFX2);
	UINT8 *buffer = malloc(0x40000);
	int i;
	if (!buffer)
		fatalerror("Unable to allocate ROM decode buffer");
	memcpy(&buffer[0], rom, 0x40000);

	static const UINT8 decode_table[0x20] = {
								   (2<<1)+1, (2<<1),
								   (7<<1),   (7<<1)+1,
								   (0<<1)+1, (0<<1),
								   (5<<1),   (5<<1)+1,
								   (6<<1),   (6<<1)+1,
								   (3<<1),   (3<<1)+1,
								   (4<<1),   (4<<1)+1,
								   (1<<1),   (1<<1)+1,

								   (10<<1)+1,(10<<1),
								   (15<<1),  (15<<1)+1,
								   (8<<1)+1, (8<<1),
								   (13<<1),  (13<<1)+1,
								   (14<<1),  (14<<1)+1,
								   (11<<1),  (11<<1)+1,
								   (12<<1),  (12<<1)+1,
								   (9<<1),   (9<<1)+1
								 };

	for (i = 0x00000; i < 0x40000; i++)
	{
		UINT8 col = i & 0x1f;
		UINT32 addr = decode_table[col];
		addr = (i & 0x3ffe0) | addr;
		rom[i] = buffer[addr];
	}
	free(buffer);
}

static DRIVER_INIT( hotbody )
{
	decode_cpu();
	decode_sprites();
}

GAME( 1995, hotbody, 0, gameace, hotbody, hotbody, ROT0, "Gameace", "Hot Body I", GAME_SUPPORTS_SAVE )

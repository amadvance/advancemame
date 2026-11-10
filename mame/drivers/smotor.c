// license:BSD-3-Clause
// copyright-holders:Roberto Fresca, Mirko Buffoni
/*
 * Super Motor (prototype), Duintronic, year unknown.
 * Z80 at 5 MHz, YM2413 at 2.5 MHz, two tile layers and three palette PROMs.
 * The DIP switches select a motorcycle or car and four difficulty levels.
 */

#include "driver.h"
#include "sound/2413intf.h"
#include "vidhrdw/res_net.h"

static UINT8 *smotor_videoram;
static UINT8 videoreg[6];
static tilemap *fg_tilemap, *bg_tilemap;

static void get_fg_tile_info(int tile_index)
{
	int attr = smotor_videoram[tile_index * 2 + 1];
	int code = smotor_videoram[tile_index * 2] | ((attr & 0x0f) << 8);
	SET_TILE_INFO(0, code, attr >> 4, 0)
}

static void get_bg_tile_info(int tile_index)
{
	const UINT8 *rom = memory_region(REGION_USER1);
	int attr = rom[tile_index * 2 + 1];
	int code = (rom[tile_index * 2] | ((attr & 7) << 8)) + 0x800;
	SET_TILE_INFO(0, code, attr >> 4, 0)
}

static WRITE8_HANDLER( smotor_videoram_w )
{
	smotor_videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset / 2);
}

static VIDEO_START( smotor )
{
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 1024);
	if (!fg_tilemap || !bg_tilemap)
		return 1;
	tilemap_set_scroll_rows(bg_tilemap, 32);
	tilemap_set_transparent_pen(fg_tilemap, 0);
	return 0;
}

static VIDEO_UPDATE( smotor )
{
	if (videoreg[0] & 1)
	{
		tilemap_set_scrolly(bg_tilemap, 0, videoreg[3] | (videoreg[5] << 8));
		tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	}
	else
		fillbitmap(bitmap, 0, cliprect);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}

static PALETTE_INIT( smotor )
{
	static const int resistances[4] = { 2200, 1000, 680, 220 };
	double weights[4];
	int i;
	compute_resistor_weights(0, 255, -1.0, 4, resistances, weights, 330, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	for (i = 0; i < 256; ++i)
	{
		int r = color_prom[i], g = color_prom[i + 0x100], b = color_prom[i + 0x200];
		palette_set_color(i,
			combine_4_weights(weights, BIT(r,0), BIT(r,1), BIT(r,2), BIT(r,3)),
			combine_4_weights(weights, BIT(g,0), BIT(g,1), BIT(g,2), BIT(g,3)),
			combine_4_weights(weights, BIT(b,0), BIT(b,1), BIT(b,2), BIT(b,3)));
	}
}

static WRITE8_HANDLER( smotor_videoreg_w )
{
	switch (offset)
	{
	case 0:
		videoreg[0] = data;
		sndti_set_output_gain(SOUND_YM2413, 0, 0, (data & 0x10) ? 1.0 : 0.0);
		sndti_set_output_gain(SOUND_YM2413, 0, 1, (data & 0x10) ? 1.0 : 0.0);
		/* Initialization pulses the lock and counter together; do not count it. */
		if ((data & 5) == 5)
			break;
		coin_lockout_global_w(data & 1);
		coin_counter_w(1, data & 2);
		coin_counter_w(0, data & 4);
		break;
	case 2:
	case 3:
		videoreg[3] = data;
		break;
	case 4:
	case 5:
		videoreg[5] = data;
		break;
	}
}

static ADDRESS_MAP_START( smotor_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM AM_WRITE(smotor_videoram_w) AM_BASE(&smotor_videoram)
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( smotor_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r) AM_WRITE(YM2413_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r) AM_WRITE(YM2413_data_port_0_w)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x02, 0x07) AM_WRITE(smotor_videoreg_w)
ADDRESS_MAP_END

INPUT_PORTS_START( smotor )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x05, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x00, "5 Coins/2 Credits" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x50, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x00, "5 Coins/2 Credits" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy (with gas cans, only trucks)" )
	PORT_DIPSETTING(    0x04, "Normal (with gas cans, trucks and cars)" )
	PORT_DIPSETTING(    0x02, "Medium (no gas cans, both trucks and cars)" )
	PORT_DIPSETTING(    0x00, "Hard (no gas cans, both trucks and cars)" )
	PORT_DIPNAME( 0x18, 0x18, "Game Time" )
	PORT_DIPSETTING(    0x18, "1:00" )
	PORT_DIPSETTING(    0x10, "1:20" )
	PORT_DIPSETTING(    0x08, "1:40" )
	PORT_DIPSETTING(    0x00, "2:00" )
	PORT_DIPNAME( 0x20, 0x20, "Cycle / Car" )
	PORT_DIPSETTING(    0x20, "Cycle" )
	PORT_DIPSETTING(    0x00, "Car" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static const gfx_layout tiles8x8_layout =
{
	8, 8, 0x1000, 4,
	{ 0, 2, 4, 6 },
	{ 1, 0, 9, 8, 17, 16, 25, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32 * 8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};

static MACHINE_START( smotor )
{
	state_save_register_global_array(videoreg);
	return 0;
}

static MACHINE_DRIVER_START( smotor )
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_PROGRAM_MAP(smotor_mem, 0)
	MDRV_CPU_IO_MAP(smotor_io, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)
	MDRV_MACHINE_START(smotor)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(smotor)
	MDRV_VIDEO_START(smotor)
	MDRV_VIDEO_UPDATE(smotor)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2413, 2500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 2.0)
MACHINE_DRIVER_END

ROM_START( smotor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "2_27c512.d4", 0x00000, 0x10000, CRC(82ca877d) SHA1(fc11c793c25865ebd3d5199c61ce14c535e0d2a8) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "1_27c512.a2", 0x00000, 0x10000, CRC(f12cb8bb) SHA1(0aaa7040b3bdba01e823212550cc0f990eee02e8) )
	ROM_LOAD( "4_27c512.i1", 0x10000, 0x10000, CRC(4c82a10b) SHA1(215354f37f6c13378aca85c96c209610f774711a) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "3_27c512.g3", 0x00000, 0x10000, CRC(0f67d74b) SHA1(90bb8884f95c883fa3d95e97fba2c0420668dab3) )

	ROM_REGION( 0x300, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.l4", 0x000, 0x100, CRC(cd06515a) SHA1(589961f0438b889d20223d8139992dc8bf35b935) )
	ROM_LOAD( "82s129.j4", 0x100, 0x100, CRC(26472e34) SHA1(c08f61911135cf1fe847a109f25de7f32646f5ec) )
	ROM_LOAD( "82s129.k4", 0x200, 0x100, CRC(b4cffc3b) SHA1(1536ea938f1dc8e506ad98ae6810279e83fe9192) )

	ROM_REGION( 0x0600, REGION_USER2, 0 )
	ROM_LOAD( "1_gal16v8.a3", 0x0000, 0x0117, NO_DUMP )
	ROM_LOAD( "2_gal16v8.k3", 0x0000, 0x0117, NO_DUMP )
	ROM_LOAD( "3_gal16v8.l2", 0x0000, 0x0117, NO_DUMP )
ROM_END

GAME( 19??, smotor, 0, smotor, smotor, 0, ROT0, "Duintronic", "Super Motor (prototype)", GAME_SUPPORTS_SAVE )

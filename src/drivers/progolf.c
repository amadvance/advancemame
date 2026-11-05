// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Roberto Zandona'
/*
    18 Holes Pro Golf (c) 1981 Data East
    Based on early work by Pierpaolo Prazzoli and David Haywood.
*/

#include "driver.h"
#include "sound/ay8910.h"
#include "vidhrdw/crtc6845.h"

static UINT8 *tile_ram, *frame_ram;
static tilemap *background;
static UINT8 video_control, scroll_lo, scroll_hi, flip_latch;

static UINT32 progolf_scan(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	return ((32 - col) & 127) * 32 + row;
}

static void get_tile_info(int tile_index)
{
	SET_TILE_INFO(0, tile_ram[tile_index], 1, 0)
}

static READ8_HANDLER( tile_r )
{
	int half = (offset >> 11) & 1;
	int bank = (video_control >> 4) & 3;
	if (bank && half == ((video_control >> 6) & 1))
		return memory_region(REGION_GFX1)[(bank - 1) * 0x1000 + offset];
	return tile_ram[offset];
}

static WRITE8_HANDLER( tile_w )
{
	/* ROM selection affects reads only; the same address still writes tile RAM. */
	tile_ram[offset] = data;
	tilemap_mark_tile_dirty(background, offset);
}

static READ8_HANDLER( frame_r )
{
	UINT8 data = 0;
	int plane;
	for (plane = 0; plane < 3; ++plane)
		if (video_control & (1 << plane))
			data |= frame_ram[offset + plane * 0x2000];
	return data;
}

static WRITE8_HANDLER( frame_w )
{
	int plane;
	/* A zero plane mask clears all three planes rather than inhibiting writes. */
	for (plane = 0; plane < 3; ++plane)
		if (!(video_control & 7))
			frame_ram[offset + plane * 0x2000] = 0;
		else if (video_control & (1 << plane))
			frame_ram[offset + plane * 0x2000] = data;
}

static WRITE8_HANDLER( video_control_w ) { video_control = data; }
static WRITE8_HANDLER( scroll_hi_w ) { scroll_hi = data; }
static WRITE8_HANDLER( scroll_lo_w ) { scroll_lo = data; }
static WRITE8_HANDLER( flip_w ) { flip_latch = data & 1; }

static PALETTE_INIT( progolf )
{
	int i;
	for (i = 0; i < 96; ++i)
	{
		int data = color_prom[i];
		int r = 0x21 * (data & 1) + 0x47 * ((data >> 1) & 1) + 0x97 * ((data >> 2) & 1);
		int g = 0x21 * ((data >> 3) & 1) + 0x47 * ((data >> 4) & 1) + 0x97 * ((data >> 5) & 1);
		int b = 0x47 * ((data >> 6) & 1) + 0x97 * ((data >> 7) & 1);
		palette_set_color(i, r, g, b);
	}
}

static VIDEO_START( progolf )
{
	frame_ram = auto_malloc(0x6000);
	memset(frame_ram, 0, 0x6000);
	background = tilemap_create(get_tile_info, progolf_scan, TILEMAP_OPAQUE, 8, 8, 128, 32);
	return background ? 0 : 1;
}

static VIDEO_UPDATE( progolf )
{
	int x, y;
	tilemap_set_scrollx(background, 0, -(scroll_lo | ((scroll_hi & 3) << 8)));
	tilemap_draw(bitmap, cliprect, background, 0, 0);
	for (x = 0; x < 32; ++x)
		for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
		{
			int offset = (x * 32 + (y >> 3)) * 8 + (y & 7);
			UINT8 p0 = frame_ram[offset], p1 = frame_ram[offset + 0x2000], p2 = frame_ram[offset + 0x4000];
			int bit;
			for (bit = 0; bit < 8; ++bit)
			{
				int dx = 256 - x * 8 + bit;
				int pen = ((p0 >> (7 - bit)) & 1) | (((p1 >> (7 - bit)) & 1) << 1) | (((p2 >> (7 - bit)) & 1) << 2);
				if (pen && dx >= cliprect->min_x && dx <= cliprect->max_x)
					plot_pixel(bitmap, dx, y, Machine->pens[pen]);
			}
		}
}

static WRITE8_HANDLER( sound_command_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, 0, ASSERT_LINE);
	cpu_boost_interleave(TIME_IN_USEC(10), TIME_IN_MSEC(1));
}

static WRITE8_HANDLER( sound_ack_w )
{
	/* Reading the command does not acknowledge it; firmware clears IRQ by writing. */
	cpunum_set_input_line(1, 0, CLEAR_LINE);
}

static INTERRUPT_GEN( coin_interrupt )
{
	int active = ((readinputport(3) & 0xc0) != 0xc0) || (readinputport(4) & 0x40);
	cpunum_set_input_line(0, INPUT_LINE_NMI, active ? ASSERT_LINE : CLEAR_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_RAM
	AM_RANGE(0x6000, 0x7fff) AM_READWRITE(frame_r, frame_w)
	AM_RANGE(0x8000, 0x8fff) AM_READWRITE(tile_r, tile_w) AM_BASE(&tile_ram)
	AM_RANGE(0x9000, 0x9000) AM_READWRITE(input_port_3_r, video_control_w)
	AM_RANGE(0x9200, 0x9200) AM_READWRITE(input_port_1_r, scroll_hi_w)
	AM_RANGE(0x9400, 0x9400) AM_READWRITE(input_port_2_r, scroll_lo_w)
	AM_RANGE(0x9600, 0x9600) AM_READWRITE(input_port_0_r, flip_w)
	AM_RANGE(0x9800, 0x9800) AM_READWRITE(input_port_4_r, crtc6845_address_w)
	AM_RANGE(0x9801, 0x9801) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x9a00, 0x9a00) AM_READWRITE(input_port_5_r, sound_command_w)
	AM_RANGE(0xb000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( audio_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x4000, 0x4fff) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x5000, 0x5fff) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x6000, 0x6fff) AM_READWRITE(AY8910_read_port_1_r, AY8910_write_port_1_w)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8000, 0x8fff) AM_READWRITE(soundlatch_r, sound_ack_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static MACHINE_RESET( progolf )
{
	video_control = scroll_lo = scroll_hi = flip_latch = 0;
	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
	cpunum_set_input_line(1, 0, CLEAR_LINE);
}

INPUT_PORTS_START( progolf )
	PORT_START_TAG("VBLANK")
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )        PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START_TAG("COINS")
	PORT_BIT( 0x3f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("DSW1:1,2")
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )  PORT_CONDITION("DSW2",0x40,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )  PORT_CONDITION("DSW2",0x40,PORTCOND_EQUALS,0x40)
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("DSW1:3,4")
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )  PORT_CONDITION("DSW2",0x40,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )  PORT_CONDITION("DSW2",0x40,PORTCOND_EQUALS,0x40)
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("DSW1:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DSW1:6")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x20, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Service_Mode ) ) PORT_DIPLOCATION("DSW1:8") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) ) PORT_DIPLOCATION("DSW2:1")
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("DSW2:2,3")
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x06, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("DSW2:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x00, "Display Strength and Position" ) PORT_DIPLOCATION("DSW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Force Coinage = A 1C/3C - B 1C/8C" ) PORT_DIPLOCATION("DSW2:6")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, "Coin Mode" ) PORT_DIPLOCATION("DSW2:7")
	PORT_DIPSETTING(    0x00, "Mode 1" )
	PORT_DIPSETTING(    0x40, "Mode 2" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DSW2:8")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x80, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout tile_layout =
{
	8, 8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), 0 },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	64
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 8 },
	{ -1 }
};

static MACHINE_DRIVER_START( progolf )
	MDRV_CPU_ADD(M6502, 10595000 / 8)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(coin_interrupt, 1)
	MDRV_CPU_ADD(M6502, 10595000 / 24)
	MDRV_CPU_PROGRAM_MAP(audio_map, 0)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_RESET(progolf)
	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(3072)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(96)
	MDRV_PALETTE_INIT(progolf)
	MDRV_VIDEO_START(progolf)
	MDRV_VIDEO_UPDATE(progolf)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 10595000 / 8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.23)
	MDRV_SOUND_ADD(AY8910, 10595000 / 8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.23)
MACHINE_DRIVER_END

ROM_START( progolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "g4-m.2a",      0xb000, 0x1000, CRC(8f06ebc0) SHA1(c012dcaf06cbd9e49f3ae819d9cbed4df8751cec) )
	ROM_LOAD( "g3-m.4a",      0xc000, 0x1000, CRC(8101b231) SHA1(d933992c93b3cd9a052ac40ec1fa92a181b28691) )
	ROM_LOAD( "g2-m.6a",      0xd000, 0x1000, CRC(a4a0d8dc) SHA1(04db60d5cfca4834ac2cc7661f772704489cb329) )
	ROM_LOAD( "g1-m.8a",      0xe000, 0x1000, CRC(749032eb) SHA1(daa356b2c70bcd8cdd0c4df4268b6158bc8aae8e) )
	ROM_LOAD( "g0-m.9a",      0xf000, 0x1000, CRC(8f8b1e8e) SHA1(fc877a8f2b26ea48c5ba2324678d6077f3432a79) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "g6-m.1b",      0xf000, 0x1000, CRC(0c6fadf5) SHA1(9af2c2152b339cadab7aff0b0164d4431d2558bd) )

	ROM_REGION( 0x3000, REGION_GFX1, 0 )
	ROM_LOAD( "g7-m.7a",      0x0000, 0x1000, CRC(16b42975) SHA1(29268a8a660781ff0de77b3b1bfc16edff7be134) )
	ROM_LOAD( "g8-m.9a",      0x1000, 0x1000, CRC(cf3f35da) SHA1(06acc29a5e282b5a9960eabebdb1a529910286b6) )
	ROM_LOAD( "g9-m.10a",     0x2000, 0x1000, CRC(7712e248) SHA1(4e7dd12d323cf8378adb1e32a763a1799e2b4bdc) )

	ROM_REGION( 0x60, REGION_PROMS, 0 )
	ROM_LOAD( "gcm.a14",      0x0000, 0x0020, CRC(8259e7db) SHA1(f98db5ebf8182eb0359fa372fa664cb6d3b09437) )
	ROM_LOAD( "gbm.k4",       0x0020, 0x0020, CRC(1ea3319f) SHA1(809af38e73fa1f30410e7d6b4504fe360ee9b091) )
	ROM_LOAD( "gam.k11",      0x0040, 0x0020, CRC(b9665de3) SHA1(4c5aba5f6589f4bce4692c0d5bb2811ab8e14aed) )
ROM_END

static DRIVER_INIT( progolf )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *decrypted = auto_malloc(0x5000);
	int address;
	/* DECO222 swaps opcode bits 5 and 6; operands and checksum reads stay encrypted. */
	for (address = 0xb000; address < 0x10000; ++address)
		decrypted[address - 0xb000] = BITSWAP8(rom[address], 7, 5, 6, 4, 3, 2, 1, 0);
	memory_set_decrypted_region(0, 0xb000, 0xffff, decrypted);
}

GAME( 1981, progolf, 0, progolf, progolf, progolf, ROT270, "Data East Corporation", "18 Holes Pro Golf (set 1)", GAME_NO_COCKTAIL )

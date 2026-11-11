// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
 Stunt Air by Nuova Videotron 1983

 Main CPU: Z80A
 Sound CPU: Z80A
 Sound IC: AY-3-8910 x2 @ 1.536MHz
 Oscillator: 18.432 MHz

 Remaining issues: palette resistor weights, foreground colours,
 sprite flicker/timing and discrete sound circuitry.
*/

#include "driver.h"
#include "sound/ay8910.h"

static UINT8 *fgram, *bgram, *bgattrram, *sprram;
static tilemap *fg_tilemap, *bg_tilemap;
static UINT8 bg_xscroll, nmi_enable, spritebank0, spritebank1;

static PALETTE_INIT( stuntair )
{
	int i;
	for (i = 0; i < 0x100; i++)
		palette_set_color(i, pal3bit(color_prom[i]), pal3bit(color_prom[i] >> 3), pal2bit(color_prom[i] >> 6));
	palette_set_color(0x100, 0, 0, 0);
	palette_set_color(0x101, 255, 255, 255);
}

static void get_fg_tile_info(int tile_index)
{
	int code = fgram[tile_index];
	SET_TILE_INFO(0, code & 0x7f, 0, TILE_SPLIT((code >> 7) & 1))
}

static void get_bg_tile_info(int tile_index)
{
	SET_TILE_INFO(1, bgram[tile_index] | ((bgattrram[tile_index] & 8) << 5), bgattrram[tile_index] & 7, 0)
}

static VIDEO_START( stuntair )
{
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_SPLIT, 8, 8, 32, 32);
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);
	if (!fg_tilemap || !bg_tilemap)
		return 1;
	/* Only bit 7 text belongs to the visible plane; redraw it over sprites. */
	tilemap_set_transmask(fg_tilemap, 0, 0x03, 0x03);
	tilemap_set_transmask(fg_tilemap, 1, 0x00, 0x00);
	return 0;
}

static VIDEO_UPDATE( stuntair )
{
	int i;
	tilemap_set_scrollx(bg_tilemap, 0, bg_xscroll);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, TILEMAP_BACK, 0);
	for (i = 0; i < 0x400; i += 16)
	{
		int code = (sprram[i+1] & 0x3f) | (spritebank1 << 6) | (spritebank0 << 7);
		drawgfx(bitmap, Machine->gfx[2], code, sprram[i+4] & 7, 0, sprram[i+1] & 0x80,
			sprram[i+5], 240 - sprram[i], cliprect, TRANSPARENCY_PEN, 0);
	}
	tilemap_draw(bitmap, cliprect, fg_tilemap, TILEMAP_FRONT, 0);
}

static WRITE8_HANDLER( fgram_w )
{
	fgram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset);
}

static WRITE8_HANDLER( bgram_w )
{
	bgram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static WRITE8_HANDLER( bgattrram_w )
{
	bgattrram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static WRITE8_HANDLER( scroll_w ) { bg_xscroll = data; }

static WRITE8_HANDLER( latch_w )
{
	switch (offset)
	{
	case 1:
		nmi_enable = data & 1;
		if (!nmi_enable)
			cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
		break;
	case 3: spritebank1 = data & 1; break;
	case 5: spritebank0 = data & 1; break;
	}
}

static WRITE8_HANDLER( coin_w )
{
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);
}

static WRITE8_HANDLER( sound_w )
{
	soundlatch_w(0, data);
	/* Only the falling command strobe creates an NMI edge. */
	cpunum_set_input_line(1, INPUT_LINE_NMI, (data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
}

static INTERRUPT_GEN( stuntair_irq )
{
	if (nmi_enable)
		cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
}

static MACHINE_RESET( stuntair )
{
	bg_xscroll = nmi_enable = spritebank0 = spritebank1 = 0;
	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xc800, 0xcbff) AM_RAM AM_WRITE(bgattrram_w) AM_BASE(&bgattrram)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_WRITE(bgram_w) AM_BASE(&bgram)
	AM_RANGE(0xd800, 0xdfff) AM_RAM AM_BASE(&sprram)
	AM_RANGE(0xe000, 0xe000) AM_READ(input_port_0_r) AM_WRITE(coin_w)
	AM_RANGE(0xe800, 0xe800) AM_READ(input_port_1_r) AM_WRITE(scroll_w)
	AM_RANGE(0xf000, 0xf000) AM_READ(input_port_2_r)
	AM_RANGE(0xf002, 0xf002) AM_READ(input_port_3_r)
	AM_RANGE(0xf003, 0xf003) AM_READ(watchdog_reset_r)
	AM_RANGE(0xf000, 0xf007) AM_WRITE(latch_w)
	AM_RANGE(0xf800, 0xfbff) AM_RAM AM_WRITE(fgram_w) AM_BASE(&fgram)
	AM_RANGE(0xfc03, 0xfc03) AM_WRITE(sound_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_ports, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x03, 0x03) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x0c, 0x0c) AM_READ(AY8910_read_port_0_r) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0d, 0x0d) AM_READ(AY8910_read_port_0_r) AM_WRITE(AY8910_write_port_0_w)
ADDRESS_MAP_END

INPUT_PORTS_START( stuntair )
	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, "1 Coin/1 Credit - 2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x24, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x24, "1 Coin/1 Credit - 2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x42, 0x40, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000 20000" )
	PORT_DIPSETTING(    0x40, "20000 30000" )
	PORT_DIPSETTING(    0x02, "30000 50000" )
	PORT_DIPSETTING(    0x42, "50000 100000" )
	PORT_DIPNAME( 0x81, 0x80, "Lives" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x81, "5" )

	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x28, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x28, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x00, "Infinite Lives (Cheat)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x04, 0x04, "Clear Credits on Reset" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static const gfx_layout tiles8x8x2_layout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static const gfx_layout tiles16x8x2_layout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,64,65,66,67,68,69,70,71 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 128+0*8, 128+1*8, 128+2*8, 128+3*8, 128+4*8, 128+5*8, 128+6*8, 128+7*8 },
	16*16
};



static const gfx_layout text_layout =
{
	8,8, RGN_FRAC(1,1), 1, { 0 },
	{ 0,1,2,3,4,5,6,7 }, { 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, 8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &text_layout, 0x100, 1 },
	{ REGION_GFX2, 0, &tiles8x8x2_layout, 0xe0, 8 },
	{ REGION_GFX3, 0, &tiles16x8x2_layout, 0xe0, 8 },
	{ -1 }
};

static const struct AY8910interface ay_interface = { soundlatch_r, 0, 0, 0 };

static MACHINE_DRIVER_START( stuntair )
	MDRV_CPU_ADD(Z80, 18432000/6)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(stuntair_irq, 1)
	MDRV_CPU_ADD(Z80, 18432000/6)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_IO_MAP(sound_ports, 0)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, TIME_IN_HZ(420))
	MDRV_MACHINE_RESET(stuntair)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x102)
	MDRV_PALETTE_INIT(stuntair)
	MDRV_VIDEO_START(stuntair)
	MDRV_VIDEO_UPDATE(stuntair)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 18432000/12)
	MDRV_SOUND_CONFIG(ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(AY8910, 18432000/12)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( stuntair )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "stuntair.a0", 0x0000, 0x2000, CRC(f61c4a1d) SHA1(29a227b447866e27e7619a0a676c9ad66364c323) )
	ROM_LOAD( "stuntair.a1", 0x2000, 0x2000, CRC(1546f041) SHA1(6a2346edf39700f7b1e609f19e0f2e46b3a78a2a) )
	ROM_LOAD( "stuntair.a3", 0x4000, 0x2000, CRC(63d00b97) SHA1(63efa151147a3c0ac33e226d38aecfd06b36ad38) )
	ROM_LOAD( "stuntair.a4", 0x6000, 0x2000, CRC(01fe2697) SHA1(f7efc6af8047245ad92dff0e62f61abc71a2e9d1) )
	ROM_LOAD( "stuntair.a6", 0x8000, 0x2000, CRC(6704d05c) SHA1(5b1af8be86ffc44ae0207397b33769556ab456df) )

	ROM_REGION( 0x2000, REGION_CPU2, 0 )
	ROM_LOAD( "stuntair.e14", 0x0000, 0x2000, CRC(641fc9db) SHA1(959a4b6617f840f52d1856e12a9fad8e12293387) )

	ROM_REGION( 0x2000, REGION_GFX1, 0 )
	ROM_LOAD( "stuntair.a9", 0x0000, 0x2000, CRC(bfd861f5) SHA1(ff089ec2e98b21202aeefc31158961e3b1d1ccca) )

	ROM_REGION( 0x4000, REGION_GFX2, 0 )
	ROM_LOAD( "stuntair.a11", 0x0000, 0x2000, CRC(421fef4c) SHA1(e46abead8cd44253cf6da74326f6f3bdd3aa26e5) )
	ROM_LOAD( "stuntair.a12", 0x2000, 0x2000, CRC(e6ee7489) SHA1(6eeea137fc8968e84d2aadbbac982fd9cd161b16) )

	ROM_REGION( 0x4000, REGION_GFX3, 0 )
	ROM_LOAD( "stuntair.a13", 0x0000, 0x2000, CRC(bfdc0d38) SHA1(ea0a22971e9cf1b1682c35facc9c4e30607faed7) )
	ROM_LOAD( "stuntair.a15", 0x2000, 0x2000, CRC(4531cab5) SHA1(35271555377ec3454a5d74bf8c21d7e8acc05782) )

	ROM_REGION( 0x120, REGION_PROMS, 0 )
	ROM_LOAD_NIB_LOW ( "dm74s287n.11m", 0x000, 0x100, CRC(d330ff90) SHA1(e223935464109a3c4c7b29641b3736484c22c47a) )
	ROM_LOAD_NIB_HIGH( "dm74s287n.11l", 0x000, 0x100, CRC(6c98f964) SHA1(abf7bdeccd33e62fa106d2056d1949cf278483a7) )
	ROM_LOAD_NIB_LOW ( "dm74s288n.7a",  0x100, 0x020, CRC(5779e751) SHA1(89c955ef8635ad3e9d699f33ec0e4d6c9205d01c) )
ROM_END

GAME( 1983, stuntair, 0, stuntair, stuntair, 0, ROT90, "Nuova Videotron", "Stunt Air", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

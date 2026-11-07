// license:BSD-3-Clause
// copyright-holders:David Haywood, Phil Bennett
/***************************************************************************

    Kyuukoukabakugekitai - Dive Bomber Squad
    (C) Konami 1989
    GX840 PWB35165

***************************************************************************/

#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "vidhrdw/res_net.h"
#include "sound/sn76496.h"

static UINT8 *fgram;
static tilemap *fg_tilemap;
static UINT8 roz_pal, roz_enable[2];
static UINT8 comm_data[4], comm_pending[4];

static void comm_irq_update(void)
{
	/* Either reply can hold the foreground IRQ active until its latch is read. */
	cpunum_set_input_line(0, 0, (comm_pending[2] || comm_pending[3]) ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(1, 0, comm_pending[0] ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(2, 0, comm_pending[1] ? ASSERT_LINE : CLEAR_LINE);
}

static void comm_callback(int param)
{
	int channel = param & 3;
	comm_data[channel] = param >> 2;
	comm_pending[channel] = 1;
	comm_irq_update();
}

static void comm_write(int channel, int data)
{
	/* Deliver at a scheduler boundary so the receiving CPU cannot read future data. */
	timer_set(TIME_NOW, (data << 2) | channel, comm_callback);
}

static int comm_read(int channel)
{
	comm_pending[channel] = 0;
	comm_irq_update();
	return comm_data[channel];
}

static WRITE8_HANDLER( fg2spr_w ) { comm_write(0, data); }
static WRITE8_HANDLER( fg2roz_w ) { comm_write(1, data); }
static WRITE8_HANDLER( spr2fg_w ) { comm_write(2, data); }
static WRITE8_HANDLER( roz2fg_w ) { comm_write(3, data); }
static READ8_HANDLER( fg2spr_r ) { return comm_read(0); }
static READ8_HANDLER( fg2roz_r ) { return comm_read(1); }
static READ8_HANDLER( spr2fg_r ) { return comm_read(2); }
static READ8_HANDLER( roz2fg_r ) { return comm_read(3); }
static READ8_HANDLER( comm_flags_r ) { return comm_pending[3] | (comm_pending[2] << 1); }

static void get_fg_tile_info(int tile_index)
{
	int attr = fgram[tile_index + 0x400];
	SET_TILE_INFO(0, fgram[tile_index] | ((attr & 3) << 8), attr >> 4, 0)
}

static WRITE8_HANDLER( fgram_w )
{
	fgram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset & 0x3ff);
}

static void zoom_callback_0(int *code, int *color)
{
	*code |= (*color & 3) << 8;
	*color = (roz_pal >> 4) & 3;
}

static void zoom_callback_1(int *code, int *color)
{
	*code |= (*color & 3) << 8;
	*color = 4 + (roz_pal & 3);
}

static WRITE8_HANDLER( roz_pal_w )
{
	if (roz_pal != data) tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	roz_pal = data;
}

static WRITE8_HANDLER( roz_bank_w )
{
	int bank = BITSWAP8(data,4,5,6,7,3,2,1,0) >> 4;
	memory_set_bankptr(1, memory_region(REGION_USER1) + bank * 0x4000);
}

static WRITE8_HANDLER( roz_wrap_0_w ) { K051316_wraparound_enable(0, !(data & 1)); }
static WRITE8_HANDLER( roz_wrap_1_w ) { K051316_wraparound_enable(1, !(data & 1)); }
static WRITE8_HANDLER( roz_enable_0_w ) { roz_enable[0] = !(data & 1); }
static WRITE8_HANDLER( roz_enable_1_w ) { roz_enable[1] = !(data & 1); }

static void decode_proms(const UINT8 *prom, int size, int base, int invert)
{
	static const int resistances[4] = { 2000,1000,470,220 };
	double weights[4];
	int i;
	compute_resistor_weights(0, 255, -1.0, 4, resistances, weights, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	for (i = 0; i < size; ++i)
	{
		int r = prom[i + size*2], g = prom[i + size], b = prom[i];
		palette_set_color(base + (invert ? (i ^ 0xff) : i),
			combine_4_weights(weights, BIT(r,0), BIT(r,1), BIT(r,2), BIT(r,3)),
			combine_4_weights(weights, BIT(g,0), BIT(g,1), BIT(g,2), BIT(g,3)),
			combine_4_weights(weights, BIT(b,0), BIT(b,1), BIT(b,2), BIT(b,3)));
	}
}

static PALETTE_INIT( divebomb )
{
	decode_proms(memory_region(REGION_USER4), 0x100, 0xc00, 0);
	decode_proms(color_prom, 0x400, 0x800, 0);
	decode_proms(memory_region(REGION_USER2), 0x400, 0, 1);
	decode_proms(memory_region(REGION_USER3), 0x400, 0x400, 1);
}

static VIDEO_START( divebomb )
{
	if (K051316_vh_start_0(REGION_GFX3, 8, TILEMAP_TRANSPARENT, 0, zoom_callback_0)) return 1;
	if (K051316_vh_start_1(REGION_GFX4, 8, TILEMAP_TRANSPARENT, 0, zoom_callback_1)) return 1;
	/* The chip API includes a 96-pixel horizontal bias in its offset argument. */
	K051316_set_offset(0, -88, -16);
	K051316_set_offset(1, -88, -16);
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,32,32);
	if (!fg_tilemap) return 1;
	tilemap_set_transparent_pen(fg_tilemap, 0);
	tilemap_set_scrolly(fg_tilemap, 0, 16);
	return 0;
}

static VIDEO_UPDATE( divebomb )
{
	int i;
	fillbitmap(bitmap, get_black_pen(), cliprect);
	if (roz_enable[1]) K051316_zoom_draw_1(bitmap, cliprect, 0, 0);
	if (roz_enable[0]) K051316_zoom_draw_0(bitmap, cliprect, 0, 0);
	for (i = 0; i < spriteram_size; i += 4)
	{
		int sy = spriteram[i + 3], sx = spriteram[i];
		int attr = spriteram[i + 1];
		int code = spriteram[i + 2] | ((attr & 15) << 8);
		drawgfx(bitmap, Machine->gfx[1], code, attr >> 4, 0,0,sx,sy,cliprect,TRANSPARENCY_PEN,0);
		drawgfx(bitmap, Machine->gfx[1], code, attr >> 4, 0,0,sx,sy-256,cliprect,TRANSPARENCY_PEN,0);
	}
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}

static ADDRESS_MAP_START( fg_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM AM_WRITE(fgram_w) AM_BASE(&fgram)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fg_ports, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(SN76496_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(SN76496_1_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(SN76496_2_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(SN76496_3_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(SN76496_4_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(SN76496_5_w)
	AM_RANGE(0x10, 0x10) AM_READ(roz2fg_r) AM_WRITE(fg2roz_w)
	AM_RANGE(0x20, 0x20) AM_READ(spr2fg_r) AM_WRITE(fg2spr_w)
	AM_RANGE(0x30, 0x30) AM_READ(input_port_4_r)
	AM_RANGE(0x31, 0x31) AM_READ(input_port_5_r)
	AM_RANGE(0x32, 0x32) AM_READ(input_port_0_r)
	AM_RANGE(0x33, 0x33) AM_READ(input_port_1_r)
	AM_RANGE(0x34, 0x34) AM_READ(input_port_2_r)
	AM_RANGE(0x35, 0x35) AM_READ(input_port_3_r)
	AM_RANGE(0x36, 0x36) AM_READ(input_port_6_r)
	AM_RANGE(0x37, 0x37) AM_READ(comm_flags_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sprite_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xc800, 0xdfff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sprite_ports, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x80, 0x80) AM_READ(fg2spr_r) AM_WRITE(spr2fg_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( roz_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc000, 0xc7ff) AM_READWRITE(K051316_0_r, K051316_0_w)
	AM_RANGE(0xd000, 0xd7ff) AM_READWRITE(K051316_1_r, K051316_1_w)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( roz_ports, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(roz_bank_w)
	AM_RANGE(0x10, 0x10) AM_WRITE(roz_wrap_1_w)
	AM_RANGE(0x12, 0x12) AM_WRITE(roz_enable_0_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(roz_enable_1_w)
	AM_RANGE(0x14, 0x14) AM_WRITE(roz_wrap_0_w)
	AM_RANGE(0x20, 0x2f) AM_WRITE(K051316_ctrl_0_w)
	AM_RANGE(0x30, 0x3f) AM_WRITE(K051316_ctrl_1_w)
	AM_RANGE(0x40, 0x40) AM_READ(fg2roz_r) AM_WRITE(roz2fg_w)
	AM_RANGE(0x50, 0x50) AM_WRITE(roz_pal_w)
ADDRESS_MAP_END

INPUT_PORTS_START( divebomb )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "9" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END




static const gfx_layout fg_layout =
{
	8,8,RGN_FRAC(1,1),2,{8,0},
	{0,1,2,3,4,5,6,7}, {0,16,32,48,64,80,96,112},128
};

static const gfx_layout sprite_layout =
{
	16,16,RGN_FRAC(1,1),4,{24,16,8,0},
	{0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39},
	{0,64,128,192,256,320,384,448,512,576,640,704,768,832,896,960},1024
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fg_layout, 0x800, 16 },
	{ REGION_GFX2, 0, &sprite_layout, 0xc00, 16 },
	{ -1 }
};

static MACHINE_RESET( divebomb )
{
	int chip;
	memset(comm_data, 0, sizeof(comm_data));
	memset(comm_pending, 0, sizeof(comm_pending));
	roz_pal = 0;
	for (chip = 0; chip < 2; ++chip)
	{
		roz_enable[chip] = 0;
		K051316_wraparound_enable(chip, 0);
	}
	roz_bank_w(0, 0);
	comm_irq_update();
}

static MACHINE_DRIVER_START( divebomb )
	MDRV_CPU_ADD(Z80, 24000000/4)
	MDRV_CPU_PROGRAM_MAP(fg_map, 0)
	MDRV_CPU_IO_MAP(fg_ports, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)
	MDRV_CPU_ADD(Z80, 24000000/4)
	MDRV_CPU_PROGRAM_MAP(sprite_map, 0)
	MDRV_CPU_IO_MAP(sprite_ports, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)
	MDRV_CPU_ADD(Z80, 24000000/4)
	MDRV_CPU_PROGRAM_MAP(roz_map, 0)
	MDRV_CPU_IO_MAP(roz_ports, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)
	/* Short slices are needed even outside a pending latch transfer to avoid lost commands. */
	MDRV_INTERLEAVE(25000)
	MDRV_MACHINE_RESET(divebomb)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 223)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0xd00)
	MDRV_PALETTE_INIT(divebomb)
	MDRV_VIDEO_START(divebomb)
	MDRV_VIDEO_UPDATE(divebomb)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 24000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)
	MDRV_SOUND_ADD(SN76496, 24000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)
	MDRV_SOUND_ADD(SN76496, 24000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)
	MDRV_SOUND_ADD(SN76496, 24000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)
	MDRV_SOUND_ADD(SN76496, 24000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)
	MDRV_SOUND_ADD(SN76496, 24000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)
MACHINE_DRIVER_END

ROM_START( divebomb )
	ROM_REGION( 0x08000, REGION_CPU1, 0 )
	ROM_LOAD( "u20.27256", 0x00000, 0x08000, CRC(89a30c82) SHA1(ecb3dcc00192b9646dab55ec6c102ab1d2e403e8) )

	ROM_REGION( 0x08000, REGION_CPU2, 0 )
	ROM_LOAD( "u21.27256", 0x00000, 0x08000, CRC(3896d3f6) SHA1(86d10c907bf00977656d8fa426b2f1ac211acdb3) )

	ROM_REGION( 0x08000, REGION_CPU3, 0 )
	ROM_LOAD( "u19.27256", 0x00000, 0x08000, CRC(16e26fb9) SHA1(9602c79f947f5267e5f4f4f8dedc4697861a519b) )

	ROM_REGION( 0x40000, REGION_USER1, 0 )
	ROM_LOAD( "u9.27512",  0x00000, 0x10000, CRC(c842f831) SHA1(badb38908a285e54b85e369342fd6ecb8fd68bbb) )
	ROM_LOAD( "u10.27512", 0x10000, 0x10000, CRC(c77f3574) SHA1(06b7576d5949906ee3209c011cd30e7066bb20cc) )
	ROM_LOAD( "u11.27512", 0x20000, 0x10000, CRC(8d46be7d) SHA1(7751df1f39b208169f04a5b904cb63e9fb53bba8) )


	ROM_REGION( 0x10000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "u22.27256", 0x00000, 0x08000, CRC(f816f9c5) SHA1(b8e136463a1b4c81960c6b7350472d82af0fb1fb) )
	ROM_LOAD16_BYTE( "u23.27256", 0x00001, 0x08000, CRC(d2600570) SHA1(a7f7e182670e7b95321c4ec8278ce915bbe2b5ca) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "u15.27c100", 0x00000, 0x20000, CRC(ccba7fa0) SHA1(5eb4c1e458e7810e0f9db92946474d6da65f1a1b) )
	ROM_LOAD32_BYTE( "u16.27c100", 0x00001, 0x20000, CRC(16891fef) SHA1(a4723958509bccc73138306e58c355325ec342a3) )
	ROM_LOAD32_BYTE( "u17.27c100", 0x00002, 0x20000, CRC(f4cbc97f) SHA1(1e13bc18db128575ca8e6998e9dd6f7dc37a99b8) )
	ROM_LOAD32_BYTE( "u18.27c100", 0x00003, 0x20000, CRC(91ab9d89) SHA1(98454df4638bb831babb1796b095169851e6cf40) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_INVERT | ROMREGION_ERASE00 )
	ROM_LOAD( "u1.27512", 0x00000, 0x10000, CRC(99af1e18) SHA1(5d63130313fdd85c58e1d6b59e42b75a15328a6b) )
	ROM_LOAD( "u2.27512", 0x10000, 0x10000, CRC(99c8d516) SHA1(6205907bd526181542f4d58d442667595aec9730) )
	ROM_LOAD( "u3.27512", 0x20000, 0x10000, CRC(5ab4af3c) SHA1(ab8632e37a42f2f0db9b22c8577c4f09718ccc7c) )


	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_INVERT )
	ROM_LOAD( "u5.27512", 0x00000, 0x10000, CRC(6726d022) SHA1(314fcec87f3bf335356b24224158233d91012675) )
	ROM_LOAD( "u6.27512", 0x10000, 0x10000, CRC(756b8a12) SHA1(2869c18ef1592d00dbc340facac2002d21adf6bc) )
	ROM_LOAD( "u7.27512", 0x20000, 0x10000, CRC(01c07d84) SHA1(45e05f15e94a32adbd488a4f77a9619e7e6b63f3) )
	ROM_LOAD( "u8.27512", 0x30000, 0x10000, CRC(5b9e7caa) SHA1(85510c0b861bad6a1411afc1d628bc7c448c9fef) )


	ROM_REGION( 0xc00, REGION_PROMS, 0 )
	ROM_LOAD( "u35.mb7122.bin", 0x800, 0x400, CRC(e890259d) SHA1(f96e00da6de3b868e50f2347b83f8e5727b85d9b) )
	ROM_LOAD( "u36.mb7122.bin", 0x400, 0x400, CRC(0c1ecdb5) SHA1(4ba9e283c747d6a760fe2e0a8b2bfdfcc55a3969) )
	ROM_LOAD( "u37.mb7122.bin", 0x000, 0x400, CRC(55c17465) SHA1(153e99a09604467ddd8c641d60c5f1b8d9a205b4) )

	ROM_REGION( 0xc00, REGION_USER2, 0 )

	ROM_LOAD( "u29.mb7122.bin", 0x000, 0x400, CRC(8b3d60d2) SHA1(a9ac4d3dd5e72522717dd18c32d3f88b75bb077c) )
	ROM_LOAD( "u30.mb7122.bin", 0x400, 0x400, CRC(0aeb1a88) SHA1(7b00e85ced210a5f7dfc100c15baa9e1735c7c80) )
	ROM_LOAD( "u31.mb7122.bin", 0x800, 0x400, CRC(75cf5f3d) SHA1(ec02b99ab65e561596b773918569b28313f21835) )

	ROM_REGION( 0xc00, REGION_USER3, 0 )

	ROM_LOAD( "u34.mb7122.bin", 0x000, 0x400, CRC(e0e2d93b) SHA1(9043929520abde15727e9333c153cf97104c9003) )
	ROM_LOAD( "u33.mb7122.bin", 0x400, 0x400, CRC(4df75f4f) SHA1(0157d6e268cdd797622c712648eb2e88214b12d9) )
	ROM_LOAD( "u32.mb7122.bin", 0x800, 0x400, CRC(e2e4b443) SHA1(e97a4e2988e29f992c5dec6f817a783b14535742) )

	ROM_REGION( 0x300, REGION_USER4, 0 )
	ROM_LOAD( "u83.mb7114.bin", 0x000, 0x100, CRC(d216110d) SHA1(3de935dbf876f82864b8b69049c8681101619411) )
	ROM_LOAD( "u82.mb7114.bin", 0x100, 0x100, CRC(52637774) SHA1(919727e337a716dbd18b51e26e45025b82aeba79) )
	ROM_LOAD( "u81.mb7114.bin", 0x200, 0x100, CRC(c59b0857) SHA1(aea4cb8d1ab59b54f90edb96d4ac42b1dd6bdcbe) )
ROM_END




GAME( 1989, divebomb, 0, divebomb, divebomb, 0, ROT270, "Konami", "Kyuukoukabakugekitai - Dive Bomber Squad (Japan, prototype)", 0 )

// license:LGPL-2.1+
// copyright-holders:Angelo Salese, Tomasz Slanina
/***************************************************************************

 Super Crowns Golf (c) 1989 Nasco Japan

 driver by Tomasz Slanina & Angelo Salese

***************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/2203intf.h"
#include "sound/msm5205.h"

static tilemap *suprgolf_tilemap;
static UINT8 tile_ram[0x1000], palette_ram[0x1000];
static UINT8 bg_vram[0x20 * 0x2000];
static UINT16 framebuffer[2][512 * 512];
static UINT8 video_regs, pen;
static UINT8 adpcm_data, adpcm_nibbles, adpcm_nmi_enable;

static void get_tile_info(int tile_index)
{
	int code = tile_ram[tile_index * 2] | (tile_ram[tile_index * 2 + 1] << 8);
	int color = tile_ram[tile_index * 2 + 0x800] & 0x7f;

	SET_TILE_INFO(0, code, color, 0)
}

static VIDEO_START( suprgolf )
{
	suprgolf_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	if (!suprgolf_tilemap)
		return 1;

	tilemap_set_transparent_pen(suprgolf_tilemap, 0x0f);
	memset(tile_ram, 0, sizeof(tile_ram));
	memset(palette_ram, 0, sizeof(palette_ram));
	memset(bg_vram, 0, sizeof(bg_vram));
	memset(framebuffer, 0, sizeof(framebuffer));
	return 0;
}

static VIDEO_UPDATE( suprgolf )
{
	int x, y;

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		const UINT16 *bg = framebuffer[0] + y * 512;
		const UINT16 *fg = framebuffer[1] + y * 512;
		UINT16 *dst = bitmap->line[y];

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dst[x] = Machine->pens[(fg[x] & 0x0f) == 0x0f ? bg[x] : fg[x]];
	}
	tilemap_draw(bitmap, cliprect, suprgolf_tilemap, 0, 0);
}

static READ8_HANDLER( video_r )
{
	return (video_regs & 0x80 ? palette_ram : tile_ram)[offset];
}

static WRITE8_HANDLER( video_w )
{
	if (video_regs & 0x80)
	{
		UINT16 rgb;

		palette_ram[offset] = data;
		offset &= ~1;
		rgb = palette_ram[offset] | (palette_ram[offset + 1] << 8);
		if (rgb & 0x8000)
			rgb = 0;
		palette_set_color(offset / 2, pal5bit((rgb >> 10) & 0x1f),
			pal5bit((rgb >> 5) & 0x1f), pal5bit(rgb & 0x1f));
	}
	else
	{
		tile_ram[offset] = data;
		tilemap_mark_tile_dirty(suprgolf_tilemap, (offset & 0x7fe) >> 1);
	}
}

static READ8_HANDLER( bg_vram_r )
{
	return bg_vram[(video_regs & 0x1f) * 0x2000 + offset];
}

static WRITE8_HANDLER( bg_vram_w )
{
	int bank = video_regs & 0x1f;
	UINT8 *ram = &bg_vram[bank * 0x2000 + offset];
	UINT16 *pixels = &framebuffer[bank >> 4][((bank & 0x0f) * 0x2000 + offset) * 2];
	UINT16 color = (pen & 0x7f) << 4;
	UINT8 mask = 0xff;

	/* In masked background writes, a nibble of 0xf preserves both RAM and pixel color. */
	if (!(pen & 0x80) && !(bank & 0x10))
	{
		if ((data & 0x0f) == 0x0f)
			mask &= 0xf0;
		if ((data & 0xf0) == 0xf0)
			mask &= 0x0f;
	}

	*ram = (*ram & ~mask) | (data & mask);
	if (mask & 0x0f)
		pixels[0] = color | (data & 0x0f);
	if (mask & 0xf0)
		pixels[1] = color | (data >> 4);
}

static WRITE8_HANDLER( pen_w )
{
	pen = data;
}

static WRITE8_HANDLER( video_regs_w )
{
	video_regs = data;
}

static WRITE8_HANDLER( adpcm_data_w )
{
	adpcm_data = data;
	adpcm_nibbles = 2;
}

static void adpcm_vclk(int chip)
{
	if (adpcm_nibbles)
	{
		MSM5205_data_w(chip, adpcm_data >> 4);
		adpcm_data <<= 4;
		adpcm_nibbles--;
	}

	/* The legacy callback runs just before the decoder consumes the nibble. */
	if (!adpcm_nibbles && adpcm_nmi_enable)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( rom_bank_select_w )
{
	memory_set_bankptr(2, memory_region(REGION_USER1) + (data & 0x3f) * 0x4000);
	adpcm_nmi_enable = data & 0x40;
	MSM5205_reset_w(0, !adpcm_nmi_enable);
	flip_screen_set(data & 0x80);
}

static WRITE8_HANDLER( rom2_bank_select_w )
{
	memory_set_bankptr(1, memory_region(REGION_USER2) + (data & 0x0f) * 0x4000);
}

static READ8_HANDLER( p1_r )
{
	return (readinputport(0) & 0xf0) | (readinputport(1) & 0x0f);
}

static READ8_HANDLER( p2_r )
{
	return (readinputport(3) & 0xf0) | (readinputport(4) & 0x0f);
}

static READ8_HANDLER( pedal_extra_bits_r )
{
	return ((readinputport(2) & 0x80) >> 7) | ((readinputport(5) & 0x80) >> 6);
}

static const ppi8255_interface ppi_interface =
{
	2,
	{ p1_r, input_port_6_r },
	{ p2_r, 0 },
	{ pedal_extra_bits_r, 0 },
	{ 0, 0 },
	{ 0, rom_bank_select_w },
	{ 0, video_regs_w }
};

static void suprgolf_reset(void)
{
	ppi8255_0_w(3, 0x9b);
	ppi8255_1_w(3, 0x9b);
	rom2_bank_select_w(0, 0);
	rom_bank_select_w(0, 0);
	video_regs = 0;
	pen = 0;
	adpcm_data = 0;
	adpcm_nibbles = 0;
}

static MACHINE_START( suprgolf )
{
	ppi8255_init(&ppi_interface);

	/* Hold the decoder in reset after the sound core resets its chips. */
	add_reset_callback(suprgolf_reset);
	return 0;
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(rom2_bank_select_w)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xdfff) AM_READWRITE(bg_vram_r, bg_vram_w)
	AM_RANGE(0xe000, 0xefff) AM_READWRITE(video_r, video_w)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(pen_w)
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0x03) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x04, 0x07) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0x08, 0x08) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x09, 0x09) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0x0c, 0x0c) AM_WRITE(adpcm_data_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( suprgolf )
	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x0f, 0x00, IPT_DIAL ) PORT_SENSITIVITY(5) PORT_KEYDELTA(5) PORT_CODE_DEC(KEYCODE_Z) PORT_CODE_INC(KEYCODE_X) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x0f, 0x00, IPT_DIAL ) PORT_SENSITIVITY(5) PORT_KEYDELTA(5) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_M) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )

	PORT_DIPNAME( 0x08, 0x08, "Indoor Practice" )  PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Number of Balls" ) PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )

	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x00, "Percentage of wind over 10m/s" ) PORT_DIPLOCATION("SW2:2,3")
	PORT_DIPSETTING(    0x00, "30%" )
	PORT_DIPSETTING(    0x04, "40%" )
	PORT_DIPSETTING(    0x02, "50%" )
	PORT_DIPSETTING(    0x06, "60%" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static struct YM2203interface ym2203_interface =
{
	input_port_7_r,
	input_port_8_r
};

static const struct MSM5205interface msm5205_interface =
{
	adpcm_vclk,
	MSM5205_S48_4B
};

static const gfx_layout gfxlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gfxlayout,   0, 0x80 },
	{ -1 }
};

static MACHINE_DRIVER_START( suprgolf )


	MDRV_CPU_ADD(Z80,6000000)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_IO_MAP(io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_VIDEO_START(suprgolf)
	MDRV_VIDEO_UPDATE(suprgolf)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(suprgolf)


	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 191)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)


	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END




ROM_START( suprgolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cg34.k6",0x000000, 0x08000, CRC(4f5ffbce) SHA1(8ca0d41a359a927340058be6d18e01b398f1b077) )

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASEFF )
	ROM_LOAD( "cg1.hj6", 0x000000, 0x10000, CRC(ee545c71) SHA1(8ee459a85e52257d3f9a2aa7263b641aad87bafd) )
	ROM_LOAD( "cg2.g6",  0x010000, 0x10000, CRC(a2ed2159) SHA1(5e13b6c4eaba8146a4c6c2ff24197f3ffca29b92) )
	ROM_LOAD( "cg3.ef6", 0x020000, 0x10000, CRC(4543334d) SHA1(7ee268ed6d02c78db8c222418313593df37cde4b) )
	ROM_LOAD( "cg4.d6",  0x030000, 0x10000, CRC(85ace664) SHA1(5267406c98e2d124a4985816f8e2e32e74e09614) )
	ROM_LOAD( "cg5.c6",  0x040000, 0x10000, CRC(609d5b37) SHA1(60640a9bd0883bf4dc999077d89ef793e827ac23) )
	ROM_LOAD( "cg6.a6",  0x050000, 0x10000, CRC(5e4a8ddb) SHA1(0c71c7eba9fe79187c4214eb639a481305070dcc) )
	ROM_LOAD( "cg7.hj4", 0x060000, 0x10000, CRC(90ac6734) SHA1(2656397fca6dceabf8e35c093c0ba25e08d2ad1e) )
	ROM_LOAD( "cg8.g4",  0x070000, 0x10000, CRC(2e9edece) SHA1(a0961bb23f312ed137134746d2d3d438fe098085) )
	ROM_LOAD( "cg9.ef4", 0x080000, 0x10000, CRC(139d71f1) SHA1(756ed068e1e2b76a9d1df95b432976e632edfa77) )
	ROM_LOAD( "cg10.d4", 0x090000, 0x10000, CRC(c069e75e) SHA1(77f1b7571e677aef601b8b1c481b352ca6e485d6) )

	ROM_LOAD( "cg11.a4", 0x0b0000, 0x10000, CRC(cfec1a0f) SHA1(c09ece059cb3c456b66c016c6fab3139d3f61c6a) )

	ROM_REGION( 0x100000, REGION_USER2, ROMREGION_ERASEFF )
	ROM_LOAD( "cg30.ic14", 0x000000, 0x10000, CRC(6b7ffee9) SHA1(7b7f0f9801ab604ea4280c6d75dfcfdb4123520c) )
	ROM_LOAD( "cg31.ic13", 0x010000, 0x10000, CRC(c5ba8e39) SHA1(aff8d5fd532f1e1d90c21bc42a349e3e83c67064) )
	ROM_LOAD( "cg32.ic12", 0x020000, 0x10000, CRC(a2265aa0) SHA1(841e1794bc1b1fc60cdfd4003d1d87bb7ebf503e) )
	ROM_LOAD( "cg33.ic11", 0x030000, 0x10000, CRC(90f1f09d) SHA1(e6c4b6c088a40f97281a1ceb71ed0e73a07ff040) )

	ROM_REGION( 0x70000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cg12.ic10", 0x00000, 0x10000, CRC(5707b3d5) SHA1(9102a40fefb6426f2cd9d92d66fdc77e078e3f4c) )
	ROM_LOAD( "cg13.ic9",  0x10000, 0x10000, CRC(02ff0187) SHA1(aeeb3b2d15c3c8ff4695ecf6cfc0c385295ecce6) )
	ROM_LOAD( "cg14.ic6",  0x20000, 0x10000, CRC(ca12e01d) SHA1(9c627fb527c8966e16dc6bdb99ec0b9728b5c5f9) )
	ROM_LOAD( "cg15.ic5",  0x30000, 0x10000, CRC(0fb88270) SHA1(d85a7f1bc5b3c4b13bbd887cea4c055541cbb737) )
	ROM_LOAD( "cg16.ic4",  0x40000, 0x10000, CRC(0498aa2e) SHA1(988965c3a584dac17ad8c7e504fa1f1e49775611) )
	ROM_LOAD( "cg17.ic3",  0x50000, 0x10000, CRC(d27f87b5) SHA1(5b2927e89615589540e3853593aeff517584b6a0) )
	ROM_LOAD( "cg18.ic2",  0x60000, 0x10000, CRC(36edd88e) SHA1(374c95721198a88831d6f7e0b71d05e2f8465271) )
ROM_END

ROM_START( suprgolfj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cg24.6k",0x000000, 0x08000, CRC(de548044) SHA1(f96b4cfcfca4dffabfaf205eb903cbc70972626b) )

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASEFF )
	ROM_LOAD( "cg1.6j", 0x000000, 0x10000, CRC(ee545c71) SHA1(8ee459a85e52257d3f9a2aa7263b641aad87bafd) )
	ROM_LOAD( "cg2.6g", 0x010000, 0x10000, CRC(a2ed2159) SHA1(5e13b6c4eaba8146a4c6c2ff24197f3ffca29b92) )
	ROM_LOAD( "cg3.6f", 0x020000, 0x10000, CRC(4543334d) SHA1(7ee268ed6d02c78db8c222418313593df37cde4b) )
	ROM_LOAD( "cg4.6d", 0x030000, 0x10000, CRC(85ace664) SHA1(5267406c98e2d124a4985816f8e2e32e74e09614) )
	ROM_LOAD( "cg5.6c", 0x040000, 0x10000, CRC(609d5b37) SHA1(60640a9bd0883bf4dc999077d89ef793e827ac23) )
	ROM_LOAD( "cg6.6a", 0x050000, 0x10000, CRC(5e4a8ddb) SHA1(0c71c7eba9fe79187c4214eb639a481305070dcc) )
	ROM_LOAD( "cg7.5j", 0x060000, 0x10000, CRC(90ac6734) SHA1(2656397fca6dceabf8e35c093c0ba25e08d2ad1e) )
	ROM_LOAD( "cg8.5g", 0x070000, 0x10000, CRC(2e9edece) SHA1(a0961bb23f312ed137134746d2d3d438fe098085) )
	ROM_LOAD( "cg9.5f", 0x080000, 0x10000, CRC(139d71f1) SHA1(756ed068e1e2b76a9d1df95b432976e632edfa77) )
	ROM_LOAD( "cg10.5d",0x090000, 0x10000, CRC(c069e75e) SHA1(77f1b7571e677aef601b8b1c481b352ca6e485d6) )

	ROM_LOAD( "cg11.5a",0x0b0000, 0x10000, CRC(cfec1a0f) SHA1(c09ece059cb3c456b66c016c6fab3139d3f61c6a) )

	ROM_REGION( 0x100000, REGION_USER2, ROMREGION_ERASEFF )
	ROM_LOAD( "cg20.7k",0x000000, 0x10000, CRC(1e3fa2fd) SHA1(4771b90e40ebfbae4a98ff7ce6db50f635232597) )
	ROM_LOAD( "cg21.7j",0x010000, 0x10000, CRC(0323a2cd) SHA1(d7d4b35ad451acb2fa3d117bb0ae2f8fbd883f17) )
	ROM_LOAD( "cg22.7g",0x020000, 0x10000, CRC(83bcbefd) SHA1(77f29cfd1583d2506e95b8513cb9f87569c31821) )
	ROM_LOAD( "cg23.7f",0x030000, 0x10000, CRC(50191b4d) SHA1(8f74cba2a2b5fd2a03eaf13a6d6b39af8833a4ab) )

	ROM_REGION( 0x70000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cg12.6k",0x00000, 0x10000, CRC(5707b3d5) SHA1(9102a40fefb6426f2cd9d92d66fdc77e078e3f4c) )
	ROM_LOAD( "cg13.6j",0x10000, 0x10000, CRC(02ff0187) SHA1(aeeb3b2d15c3c8ff4695ecf6cfc0c385295ecce6) )
	ROM_LOAD( "cg14.5k",0x20000, 0x10000, CRC(ca12e01d) SHA1(9c627fb527c8966e16dc6bdb99ec0b9728b5c5f9) )
	ROM_LOAD( "cg15.5j",0x30000, 0x10000, CRC(0fb88270) SHA1(d85a7f1bc5b3c4b13bbd887cea4c055541cbb737) )
	ROM_LOAD( "cg16.5g",0x40000, 0x10000, CRC(0498aa2e) SHA1(988965c3a584dac17ad8c7e504fa1f1e49775611) )
	ROM_LOAD( "cg17.5f",0x50000, 0x10000, CRC(d27f87b5) SHA1(5b2927e89615589540e3853593aeff517584b6a0) )
	ROM_LOAD( "cg18.3k",0x60000, 0x10000, CRC(36edd88e) SHA1(374c95721198a88831d6f7e0b71d05e2f8465271) )
ROM_END

static void patch_course_rom(int branch)
{
	UINT8 *rom = memory_region(REGION_USER2);

	/* Force the polygon fill path regardless of the port-C readback. */
	rom[0x74f4 - 0x4000] = 0x00;
	rom[0x74f5 - 0x4000] = 0x00;

	/* Adjust the checksum branch after modifying the course ROM. */
	rom[3 * 0x4000 + branch - 0x4000] = 0x20;
}

static DRIVER_INIT( suprgolf )
{
	patch_course_rom(0x74fa);
}

static DRIVER_INIT( suprgolfj )
{
	patch_course_rom(0x6d72);
}

GAME( 1989, suprgolf,  0,        suprgolf, suprgolf, suprgolf,  ROT0, "Nasco", "Super Crowns Golf (World)",        GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
GAME( 1989, suprgolfj, suprgolf, suprgolf, suprgolf, suprgolfj, ROT0, "Nasco", "Super Crowns Golf (Japan, set 1)", GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )

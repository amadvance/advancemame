// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
  Disco Boy, Soft Art Co. 1993 NO-1021.
  Two Z80 CPUs, YM3812, MSM5205, 10 MHz and 12 MHz oscillators.
  Video hardware is derived from Mitchell and Yun Sung designs.
*/

#include "driver.h"
#include "sound/3812intf.h"
#include "sound/msm5205.h"

static UINT8 palette_ram[0x1000], tile_ram[0x1000], sprite_ram[0x1000];
static UINT8 *attribute_ram;
static UINT8 video_bank, palette_bank, main_bank, sound_bank, gfx_bank;
static UINT8 adpcm_data, adpcm_toggle;
static tilemap *bg_tilemap;

static void get_bg_tile_info(int tile_index)
{
	int code = tile_ram[tile_index * 2] | (tile_ram[tile_index * 2 + 1] << 8);
	int gfx_index = 1;
	if (code > 0x2000)
	{
		code = (code & 0x1fff) + ((gfx_bank & 0xc0) << 7);
		gfx_index = 2;
	}
	SET_TILE_INFO(gfx_index, code, attribute_ram[tile_index] & 0x7f, 0)
}
static VIDEO_START( discoboy )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 64, 32);
	return bg_tilemap ? 0 : 1;
}
static VIDEO_UPDATE( discoboy )
{
	int pos;
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	for (pos = 0x1000 - 0x40; pos >= 0; pos -= 0x20)
	{
		int attr = sprite_ram[pos + 1];
		int code = sprite_ram[pos] | ((attr & 0xe0) << 3);
		int x = sprite_ram[pos + 3] + ((attr & 0x10) << 4);
		int y = ((sprite_ram[pos + 2] + 8) & 255) - 8;
		if (code >= 0x400) code = 0x400 + (code & 0x3ff) + ((gfx_bank & 0x30) << 6);
		drawgfx(bitmap, Machine->gfx[0], code, attr & 15, 0, 0, x, y, cliprect, TRANSPARENCY_PEN, 15);
	}
}
static READ8_HANDLER( palette_r ) { return palette_ram[palette_bank * 0x800 + offset]; }
static WRITE8_HANDLER( palette_w )
{
	int color, addr = palette_bank * 0x800 + offset;
	palette_ram[addr] = data;
	color = palette_ram[addr & ~1] | (palette_ram[addr | 1] << 8);
	palette_set_color(addr / 2, ((color >> 8) & 15) << 4, ((color >> 4) & 15) << 4, (color & 15) << 4);
}
static READ8_HANDLER( video_r ) { return video_bank ? sprite_ram[offset] : tile_ram[offset]; }
static WRITE8_HANDLER( video_w )
{
	if (video_bank) sprite_ram[offset] = data;
	else if (tile_ram[offset] != data)
	{
		tile_ram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}
static WRITE8_HANDLER( attribute_w )
{
	if (attribute_ram[offset] != data)
	{
		attribute_ram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}
static WRITE8_HANDLER( video_bank_w ) { video_bank = data & 1; }
static WRITE8_HANDLER( palette_bank_w ) { palette_bank = (data >> 5) & 1; }
static WRITE8_HANDLER( main_bank_w )
{
	main_bank = data & 7;
	if ((gfx_bank ^ data) & 0xc0) tilemap_mark_all_tiles_dirty(bg_tilemap);
	gfx_bank = data & 0xf0;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000 + main_bank * 0x4000);
}
static READ8_HANDLER( port06_r ) { return 0; }
static WRITE8_HANDLER( sound_command_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, 0, ASSERT_LINE);
}
static READ8_HANDLER( sound_command_r )
{
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	return soundlatch_r(0);
}
static WRITE8_HANDLER( sound_bank_w )
{
	sound_bank = data & 7;
	memory_set_bankptr(2, memory_region(REGION_CPU2) + sound_bank * 0x4000);
	MSM5205_reset_w(0, data & 8);
}
static void adpcm_update(void) { MSM5205_data_w(0, adpcm_toggle ? adpcm_data >> 4 : adpcm_data & 15); }
static WRITE8_HANDLER( adpcm_w )
{
	adpcm_data = data;
	adpcm_update();
}
static void adpcm_clock(int chip)
{
	/* The nibble selector also divides the sample clock into byte requests. */
	adpcm_toggle ^= 1;
	adpcm_update();
	cpunum_set_input_line(1, INPUT_LINE_NMI, adpcm_toggle ? ASSERT_LINE : CLEAR_LINE);
}
static struct MSM5205interface msm_interface = { adpcm_clock, MSM5205_S96_4B };

static void restore_banks(void)
{
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000 + main_bank * 0x4000);
	memory_set_bankptr(2, memory_region(REGION_CPU2) + sound_bank * 0x4000);
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}
static MACHINE_START( discoboy )
{
	state_save_register_global_array(palette_ram);
	state_save_register_global_array(tile_ram);
	state_save_register_global_array(sprite_ram);
	state_save_register_global(video_bank);
	state_save_register_global(palette_bank);
	state_save_register_global(main_bank);
	state_save_register_global(sound_bank);
	state_save_register_global(gfx_bank);
	state_save_register_global(adpcm_data);
	state_save_register_global(adpcm_toggle);
	state_save_register_func_postload(restore_banks);
	return 0;
}
static MACHINE_RESET( discoboy )
{
	video_bank = palette_bank = main_bank = sound_bank = gfx_bank = adpcm_data = adpcm_toggle = 0;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000);
	memory_set_bankptr(2, memory_region(REGION_CPU2));
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xc7ff) AM_READWRITE(palette_r, palette_w)
	AM_RANGE(0xc800, 0xcfff) AM_RAM AM_WRITE(attribute_w) AM_BASE(&attribute_ram)
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(video_r, video_w)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END
static ADDRESS_MAP_START( main_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(input_port_0_r, video_bank_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(input_port_1_r, main_bank_w)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READWRITE(input_port_3_r, sound_command_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_4_r)
	AM_RANGE(0x06, 0x06) AM_READ(port06_r) AM_WRITENOP
	AM_RANGE(0x07, 0x07) AM_WRITE(palette_bank_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(2)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(sound_bank_w)
	AM_RANGE(0xe400, 0xe400) AM_WRITE(adpcm_w)
	AM_RANGE(0xec00, 0xec00) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0xec01, 0xec01) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xf800) AM_READ(sound_command_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( discoboy )
	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "Every 150000" )
	PORT_DIPSETTING(    0x00, "Every 300000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hardest ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4,RGN_FRAC(1,2),  RGN_FRAC(0,2)+4,RGN_FRAC(0,2) },
	{ 0, 1, 2, 3, 8, 9, 10, 11, 256, 257, 258, 259, 264,265,266,267  },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*32
};

static const gfx_layout tiles8x8_layout2 =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(1,4), RGN_FRAC(0,4),RGN_FRAC(3,4),RGN_FRAC(2,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 128 },
	{ REGION_GFX2, 0, &tiles8x8_layout2, 0, 128 },
	{ REGION_GFX3, 0, &tiles8x8_layout2, 0, 128 },
	{ -1 }
};

static MACHINE_DRIVER_START( discoboy )
	MDRV_CPU_ADD(Z80, 12000000 / 2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(main_io, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)
	MDRV_CPU_ADD(Z80, 10000000 / 2)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_MACHINE_START(discoboy)
	MDRV_MACHINE_RESET(discoboy)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(64, 447, 8, 247)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000)
	MDRV_VIDEO_START(discoboy)
	MDRV_VIDEO_UPDATE(discoboy)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(YM3812, 10000000 / 4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.6)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.6)
	MDRV_SOUND_ADD(MSM5205, 400000)
	MDRV_SOUND_CONFIG(msm_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.8)
MACHINE_DRIVER_END

ROM_START( discoboy )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "u2",  0x00000, 0x10000, CRC(44a4fefa) SHA1(29b74bb739afffb7baefb5ed4da09cdb1559b011) )
	ROM_LOAD( "u18", 0x10000, 0x20000, CRC(88d1282d) SHA1(1f11dad0f577198c54a1dc182ba7502e398b998f) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "2.u28",  0x00000, 0x10000, CRC(7c2ed174) SHA1(ace209dc4cc7a4ffca062842defd84cefc5b10d2) )
	ROM_LOAD( "1.u45",  0x10000, 0x10000, CRC(c266c6df) SHA1(f76e38ded43f56a486cf6569c679ddb57a4165fb) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_INVERT )
	ROM_LOAD( "5.u94",   0x00000, 0x10000, CRC(dbd20836) SHA1(d97651626b1dc16b93f8aed28bac19fd177e626f) )
	ROM_LOAD( "6.u124",  0x10000, 0x40000, CRC(e20d41f8) SHA1(792294a34840867072bc484d6f3cae3502c8bc28) )
	ROM_LOAD( "7.u95",   0x80000, 0x10000, CRC(1d5617a2) SHA1(6b6bd50c1984748dc8bf6600431d9bb6fe443873) )
	ROM_LOAD( "8.u125",  0x90000, 0x40000, CRC(30be1340) SHA1(e4765b75c8f774c6f7f7b5496a50c33ee3950550) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_INVERT )
	ROM_LOAD( "u80",   0x00000, 0x10000, CRC(4cc642ae) SHA1(2a59ebc8ab27bf7c3c1aa389ea32fb01d5cfdce8) )
	ROM_LOAD( "u81",   0x10000, 0x10000, CRC(9e04274e) SHA1(70c28212b242335353e6dd48b7eb176146bec457) )
	ROM_LOAD( "u78",   0x20000, 0x10000, CRC(04571f70) SHA1(afdc7d84f7804c2ced413d13e6985a05f841e79e) )
	ROM_LOAD( "u79",   0x30000, 0x10000, CRC(646f0f83) SHA1(d5cd050872d4b8c2fc89c3c0f434b1d66e5f1c59) )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_INVERT )
	ROM_LOAD( "u50",   0x00000, 0x20000, CRC(1557ca92) SHA1(5a0afbeede6f0ae1c75bdec446132c673aeb0fe7) )
	ROM_LOAD( "u5",    0x20000, 0x20000, CRC(a07df669) SHA1(7f09b2508b9bffed7a4cd191f707af3c0c2a1de2) )
	ROM_LOAD( "u46",   0x40000, 0x20000, CRC(764ffde4) SHA1(637df403a6ac73456892add3f2403a92afb67f19) )
	ROM_LOAD( "u49",   0x60000, 0x20000, CRC(0b6c0d8d) SHA1(820a12c84af4fd5a04e1eca3cbace0002d3024b6) )
ROM_END

GAME( 1993, discoboy, 0, discoboy, discoboy, 0, ROT270, "Soft Art Co.", "Disco Boy", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )

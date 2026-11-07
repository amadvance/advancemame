// license:BSD-3-Clause
// copyright-holders:ElSemi, Roberto Fresca
/*
  El Fin Del Tiempo, Niemer S.A., 1981.
  Z80A, F6802P, two AY-3-8910s, 18.432 MHz and 14.318 MHz crystals.
*/

#include "driver.h"
#include "sound/ay8910.h"

#define MAIN_CLOCK 18432000
#define SOUND_CLOCK 14318181

static UINT8 *video_ram;
static UINT8 video_latch[2][8], sound_latch[4], sound_command, sound_control;
static tilemap *fg_tilemap, *bg_tilemap;

static int tile_bank(void) { return (video_latch[0][7] | video_latch[1][0] << 1 | video_latch[1][1] << 2) << 8; }
static PALETTE_INIT( efdt )
{
	int i;
	for (i = 0; i < 256; i++)
	{
		int color = color_prom[i];
		if (i & 7) palette_set_color(i, ((color >> 3) & 7) << 5, (color & 7) << 5, ((color >> 6) & 3) << 6);
		else palette_set_color(i, 0, 0, 0);
	}
}
static void get_fg_tile_info(int tile_index)
{
	int color = video_ram[0x800 + (tile_index & 31) * 2 + 1] & 31;
	SET_TILE_INFO(0, video_ram[tile_index] + tile_bank(), color, 0)
}
static void get_bg_tile_info(int tile_index)
{
	SET_TILE_INFO(1, video_ram[0xc00 + tile_index], 0x1c, 0)
}
static WRITE8_HANDLER( video_ram_w )
{
	if (video_ram[offset] != data)
	{
		int i;
		video_ram[offset] = data;
		if (offset < 0x400) tilemap_mark_tile_dirty(fg_tilemap, offset);
		else if (offset >= 0xc00) tilemap_mark_tile_dirty(bg_tilemap, offset - 0xc00);
		else if (offset >= 0x800 && offset < 0x840 && (offset & 1))
			for (i = 0; i < 32; i++) tilemap_mark_tile_dirty(fg_tilemap, i * 32 + ((offset & 0x3f) >> 1));
	}
}
static void restore_video(void) { tilemap_mark_all_tiles_dirty(ALL_TILEMAPS); }
static VIDEO_START( efdt )
{
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	if (!fg_tilemap || !bg_tilemap) return 1;
	tilemap_set_transparent_pen(fg_tilemap, 0);
	tilemap_set_transparent_pen(bg_tilemap, 0);
	tilemap_set_scroll_cols(fg_tilemap, 32);
	state_save_register_func_postload(restore_video);
	return 0;
}
static VIDEO_UPDATE( efdt )
{
	int i, bank = tile_bank();
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	for (i = 0; i < 32; i++) tilemap_set_scrolly(fg_tilemap, i, video_ram[0x800 + i * 2]);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
	for (i = 0; i < 8; i++)
	{
		UINT8 *sprite = video_ram + 0x840 + i * 4;
		int q, x = sprite[3], y = (240 - sprite[0]) & 255;
		int fx = (sprite[1] >> 6) & 1, fy = (sprite[1] >> 7) & 1;
		int code = ((sprite[1] & 63) << 2) + bank;
		if (!sprite[0] && !x) continue;
		for (q = 0; q < 4; q++)
		{
			int tile = q ^ fx ^ (fy << 1);
			drawgfx(bitmap, Machine->gfx[0], code + tile, sprite[2] & 7, fx, fy,
				x + (q & 1) * 8, y + (q >> 1) * 8, cliprect, TRANSPARENCY_PEN, 0);
		}
	}
	for (i = 0; i < 8; i++)
	{
		int row = (-video_ram[0x861 + i * 4]) & 255;
		int col = (-video_ram[0x863 + i * 4] - 8) & 255;
		if (row >= cliprect->min_y && row <= cliprect->max_y && col >= cliprect->min_x && col <= cliprect->max_x)
			((UINT16 *)bitmap->line[row])[col] = Machine->pens[7];
	}
}
static void latch_write(int which, int offset, UINT8 data)
{
	int oldbank = tile_bank();
	video_latch[which][offset] = data & 1;
	/* Q0 must drop the NMI line before the following VBLANK can trigger it. */
	if (!which && !offset && !(data & 1)) cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
	if (oldbank != tile_bank()) tilemap_mark_all_tiles_dirty(fg_tilemap);
}
static WRITE8_HANDLER( video_latch0_w ) { latch_write(0, offset, data); }
static WRITE8_HANDLER( video_latch1_w ) { latch_write(1, offset, data); }
static INTERRUPT_GEN( efdt_nmi )
{
	if (video_latch[0][0]) cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
}
static READ8_HANDLER( main_sound_r ) { return sound_latch[offset]; }
static WRITE8_HANDLER( main_sound_w )
{
	sound_latch[offset] = data;
	if (!offset) sound_command = data;
	else if (offset == 1 && (data & 8)) sound_control |= 2;
}
static READ8_HANDLER( sound_command_r ) { return sound_command; }
static READ8_HANDLER( sound_control_r ) { return sound_control; }
static WRITE8_HANDLER( sound_ack_w ) { if (data & 4) sound_control &= ~2; }
static WRITE8_HANDLER( sound_control_w ) { sound_control = data; }
static struct AY8910interface ay1_interface = { sound_command_r, sound_control_r, 0, sound_ack_w };
static struct AY8910interface ay2_interface = { sound_control_r, sound_control_r, 0, sound_control_w };

static MACHINE_START( efdt )
{
	state_save_register_global_2d_array(video_latch);
	state_save_register_global_array(sound_latch);
	state_save_register_global(sound_command);
	state_save_register_global(sound_control);
	return 0;
}
static MACHINE_RESET( efdt )
{
	memset(video_latch, 0, sizeof(video_latch));
	memset(sound_latch, 0, sizeof(sound_latch));
	sound_command = sound_control = 0;
	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0x8803) AM_READWRITE(main_sound_r, main_sound_w)
	AM_RANGE(0x9000, 0x93ff) AM_READ(input_port_0_r)
	AM_RANGE(0x9400, 0x97ff) AM_READ(input_port_1_r)
	AM_RANGE(0xa000, 0xafff) AM_RAM AM_WRITE(video_ram_w) AM_BASE(&video_ram)
	AM_RANGE(0xb000, 0xb000) AM_READ(watchdog_reset_r)
	AM_RANGE(0xb400, 0xb407) AM_WRITE(video_latch0_w)
	AM_RANGE(0xb800, 0xb807) AM_WRITE(video_latch1_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_RAM
	AM_RANGE(0x6000, 0x6000) AM_WRITENOP
	AM_RANGE(0x7000, 0x7000) AM_WRITENOP
	AM_RANGE(0x8000, 0x83ff) AM_RAM
	AM_RANGE(0x9000, 0x9000) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x9200, 0x9200) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x9400, 0x9400) AM_READWRITE(AY8910_read_port_1_r, AY8910_write_port_1_w)
	AM_RANGE(0x9600, 0x9600) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM AM_REGION(REGION_CPU2, 0)
ADDRESS_MAP_END

static INPUT_PORTS_START( efdt )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME(0x03, 0x00, DEF_STR(Coinage))
	PORT_DIPSETTING(0x00, DEF_STR(1C_1C))
	PORT_DIPSETTING(0x01, DEF_STR(1C_2C))
	PORT_DIPSETTING(0x02, DEF_STR(1C_3C))
	PORT_DIPSETTING(0x03, DEF_STR(2C_3C))
	PORT_DIPNAME(0x0c, 0x00, DEF_STR(Coinage))
	PORT_DIPSETTING(0x00, DEF_STR(1C_1C))
	PORT_DIPSETTING(0x04, DEF_STR(1C_2C))
	PORT_DIPSETTING(0x08, DEF_STR(1C_3C))
	PORT_DIPSETTING(0x0c, DEF_STR(2C_3C))
	PORT_DIPNAME(0x30, 0x00, DEF_STR(Lives))
	PORT_DIPSETTING(0x00, "3")
	PORT_DIPSETTING(0x10, "4")
	PORT_DIPSETTING(0x20, "5")
	PORT_DIPSETTING(0x30, "6")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)

INPUT_PORTS_END

static const gfx_layout tilelayout3bpp =
{
	8, 8, RGN_FRAC(1,3), 3,
	{ 0, RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	64
};
static const gfx_layout tilelayout1bpp =
{
	8, 8, RGN_FRAC(1,1), 1, { 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	64
};
static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout3bpp, 0, 32 },
	{ REGION_GFX2, 0, &tilelayout1bpp, 0, 128 },
	{ -1 }
};

static MACHINE_DRIVER_START( efdt )
	MDRV_CPU_ADD(Z80, MAIN_CLOCK / 6)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(efdt_nmi, 1)
	/* The 6802 core takes the E clock, after the oscillator's divide by four. */
	MDRV_CPU_ADD(M6802, SOUND_CLOCK / 16)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, TIME_IN_HZ(SOUND_CLOCK / 4 / 8192))
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_START(efdt)
	MDRV_MACHINE_RESET(efdt)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(efdt)
	MDRV_VIDEO_START(efdt)
	MDRV_VIDEO_UPDATE(efdt)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, SOUND_CLOCK / 8)
	MDRV_SOUND_CONFIG(ay1_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, SOUND_CLOCK / 8)
	MDRV_SOUND_CONFIG(ay2_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( efdt )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "22805.b10", 0x0000, 0x1000, CRC(7e27df91) SHA1(0f2ef3563af5d6e0de77a6ac929dbd3802aea8f0))
	ROM_LOAD( "22806.b9",  0x1000, 0x1000, CRC(00cad810) SHA1(e93f9365227c1e5c2dea1325b379e78e37c0a953))
	ROM_LOAD( "22807.b8",  0x2000, 0x1000, CRC(8e51af2b) SHA1(ac496781fb599d26905aa28449eefc3959de0e9a))
	ROM_LOAD( "22808.b7",  0x3000, 0x1000, CRC(932bd16d) SHA1(3df0f222b0803da9021d3144ec7bc28453fdd947))
	ROM_LOAD( "22801.a10", 0x4000, 0x1000, CRC(ea646049) SHA1(bca30cb2dde8b5c78f6108cb9a43e0dce697f761))
	ROM_LOAD( "22802.a9",  0x5000, 0x1000, CRC(74457952) SHA1(f5f4ece564cbdb650204ccd5abdf39d0d3c595b3))

	ROM_REGION( 0x2000, REGION_CPU2, 0 )
	ROM_LOAD(   "1811.d8",   0x0000, 0x1000, CRC(0ff5d0c2) SHA1(93df487d3236284765dd3d690474c130464e3e27))
	ROM_LOAD(   "1812.d7",   0x1000, 0x1000, CRC(48e5a4ac) SHA1(9da4800215c91b2be9df3375f9601b19353c0ec0))

	ROM_REGION( 0x9000, REGION_GFX1, 0 )
	ROM_LOAD( "12822.j3", 0x2000, 0x1000, CRC(f4d28a60) SHA1(bc1d7f4392805cd204ecfe9c3301990a7b710567) )
	ROM_LOAD( "12821.j4", 0x1000, 0x1000, CRC(b7ef75a6) SHA1(057f737fce63639879db95659de7d1d659058759) )
	ROM_LOAD( "12820.j5", 0x0000, 0x1000, CRC(70126c8d) SHA1(f380868f3afad2898c136b15210aaa6231f1d3c2) )
	ROM_LOAD( "12819.j6", 0x5000, 0x1000, CRC(2987b5b6) SHA1(0e57aae21e674155e407512f1edfb3d8b31d1fa3) )
	ROM_LOAD( "12818.j7", 0x4000, 0x1000, CRC(e0a61419) SHA1(65caa9e2700a0bec9e105dc763e9fe61dae8c3d6) )
	ROM_LOAD( "12817.j8", 0x3000, 0x1000, CRC(856a2537) SHA1(5e8f96239721a0dd64b37267bb3b343ac3034898) )
	ROM_LOAD( "12816.j9", 0x8000, 0x1000, CRC(69664044) SHA1(57465c4c37be2b4846b49a13dec9e354dabb155a) )
	ROM_LOAD( "12815.j10", 0x7000, 0x1000, CRC(abe7a7b6) SHA1(e3bc6aa3a741fcfa2eafb1464be3cb5437d5fd90) )
	ROM_LOAD( "12814.j11", 0x6000, 0x1000, CRC(6c06f746) SHA1(c7e80c5dde733e9ef520b9afa78bed902f04b74d) )

	ROM_REGION( 0x800, REGION_GFX2, 0 )
	ROM_LOAD( "12813.h10", 0x0000, 0x0800, CRC(ea03c5a8) SHA1(7ce385b43a24cbbc780162ed89031d1cc1b0b9ef))

	ROM_REGION( 0x100, REGION_PROMS, 0)
	ROM_LOAD( "ft1-tbp28l22n.bin", 0x000, 0x100, CRC(04d7b68c) SHA1(c13281280340ccf4f82a2a04bfe661a26253a05c))
ROM_END

GAME( 1981, efdt, 0, efdt, efdt, 0, ROT90, "Niemer", "El Fin Del Tiempo", GAME_IMPERFECT_GRAPHICS )

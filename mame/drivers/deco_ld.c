// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

Bega's Battle (c) 1983 Data East Corporation

Cobra Command (LD) and Road Blaster (c) 1984-1985 Data East Corporation

Preliminary driver by Angelo Salese

Main processor  - 6502
                - EF68B50P   communications interface
                - AM2950DC   I/O port to sound processor

Sound processor - 6502
             2x - AY-3-8910

There are three hardware versions of Cobra Command (LD):

Data East single PCB, Pioneer LD-V1000
Data East 3-boardset, same hardware as Bega's Battle & Road Blaster, Sony
LDP-1000
MACH3 conversion kit (ROMs, disc, decals), Pioneer PR-8210

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"

#define PALETTE_SIZE 0x800

static UINT8 *vram[2];
static UINT8 *attr[2];
static UINT8 palette_ram[PALETTE_SIZE];
static tilemap *tile[2];
static UINT8 latch_main;
static UINT8 latch_sound;

static READ8_HANDLER( palette_r )
{
	return palette_ram[offset];
}

static WRITE8_HANDLER( palette_w )
{
	int d;

	palette_ram[offset] = data;
	d = ~data;
	palette_set_color(offset, pal3bit(d), pal3bit(d >> 3), pal2bit(d >> 6));
}

static WRITE8_HANDLER( vram0_w )
{
	vram[0][offset] = data;
	tilemap_mark_tile_dirty(tile[0], offset);
}

static WRITE8_HANDLER( vram1_w )
{
	vram[1][offset] = data;
	tilemap_mark_tile_dirty(tile[1], offset);
}

static WRITE8_HANDLER( attr0_w )
{
	attr[0][offset] = data;
	tilemap_mark_tile_dirty(tile[0], offset);
}

static WRITE8_HANDLER( attr1_w )
{
	attr[1][offset] = data;
	tilemap_mark_tile_dirty(tile[1], offset);
}

static void get_tile_info0(int tile_index)
{
	SET_TILE_INFO(0, vram[0][tile_index] | ((attr[0][tile_index] & 3) << 8), 5, 0)
}

static void get_tile_info1(int tile_index)
{
	SET_TILE_INFO(0, vram[1][tile_index] | ((attr[1][tile_index] & 3) << 8), 5, 0)
}

static PALETTE_INIT( deco_ld )
{
	int i;

	for (i = 0; i < PALETTE_SIZE; ++i)
		palette_set_color(i, 0, 0, 0);
}

static VIDEO_START( deco_ld )
{
	tile[0] = tilemap_create(get_tile_info0, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	tile[1] = tilemap_create(get_tile_info1, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	if (!tile[0] || !tile[1])
		return 1;

	tilemap_set_transparent_pen(tile[0], 0);
	tilemap_set_transparent_pen(tile[1], 0);

	return 0;
}

static void draw_sprite_range(mame_bitmap *bitmap, const rectangle *cliprect, const UINT8 *sprites, int start, int end, int bank)
{
	int i;

	for (i = start; i < end; i += 4)
	{
		if (sprites[i] & 1)
		{
			drawgfx(bitmap, Machine->gfx[1],
					sprites[i + 1] | bank,
					6,
					sprites[i] & 0x04, sprites[i] & 0x02,
					sprites[i + 3], sprites[i + 2],
					cliprect, TRANSPARENCY_PEN, 0);
		}
	}
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, const UINT8 *sprites, int bank)
{
	draw_sprite_range(bitmap, cliprect, sprites, 0, 0x20, bank);
	draw_sprite_range(bitmap, cliprect, sprites, 0x3e0, 0x400, bank);
}

static VIDEO_UPDATE( deco_ld )
{
	fillbitmap(bitmap, 0, cliprect);

	draw_sprites(bitmap, cliprect, vram[1], 0x000);
	draw_sprites(bitmap, cliprect, vram[0], 0x100);

	tilemap_draw(bitmap, cliprect, tile[0], 0, 0);
	tilemap_draw(bitmap, cliprect, tile[1], 0, 0);
}

static READ8_HANDLER( sound_latch_r )
{
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	return latch_main;
}

static WRITE8_HANDLER( sound_latch_w )
{
	latch_main = data;
	cpunum_set_input_line(1, 0, ASSERT_LINE);
}

static READ8_HANDLER( main_latch_r )
{
	return latch_sound;
}

static WRITE8_HANDLER( main_latch_w )
{
	latch_sound = data;
}

static READ8_HANDLER( sound_status_r )
{
	return 0xff ^ 0x40;
}

/* the laserdisc player and its serial interface are not emulated */
static READ8_HANDLER( acia_status_r )
{
	return 0xff;
}

static READ8_HANDLER( laserdisc_status_r )
{
	return 0xff;
}

static MACHINE_RESET( deco_ld )
{
	latch_main = 0;
	latch_sound = 0;
}

static INTERRUPT_GEN( deco_ld_interrupt )
{
	/* the NMI line follows the coin inputs */
	cpunum_set_input_line(0, INPUT_LINE_NMI, (readinputport(2) & 0xc0) != 0xc0 ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x1000) AM_READ(input_port_0_r) AM_WRITENOP
	AM_RANGE(0x1001, 0x1001) AM_READ(input_port_2_r)
	AM_RANGE(0x1002, 0x1002) AM_READ(input_port_3_r)
	AM_RANGE(0x1003, 0x1003) AM_READ(input_port_1_r)
	AM_RANGE(0x1004, 0x1004) AM_READWRITE(main_latch_r, sound_latch_w)
	AM_RANGE(0x1005, 0x1005) AM_READ(sound_status_r)
	AM_RANGE(0x1006, 0x1006) AM_READ(acia_status_r)
	AM_RANGE(0x1007, 0x1007) AM_READ(laserdisc_status_r) AM_WRITENOP
	AM_RANGE(0x1800, 0x1fff) AM_READ(palette_r) AM_WRITE(palette_w)
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x800) AM_READ(MRA8_RAM) AM_WRITE(vram0_w) AM_BASE(&vram[0])
	AM_RANGE(0x2400, 0x27ff) AM_MIRROR(0x800) AM_READ(MRA8_RAM) AM_WRITE(attr0_w) AM_BASE(&attr[0])
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x800) AM_READ(MRA8_RAM) AM_WRITE(vram1_w) AM_BASE(&vram[1])
	AM_RANGE(0x3400, 0x37ff) AM_MIRROR(0x800) AM_READ(MRA8_RAM) AM_WRITE(attr1_w) AM_BASE(&attr[1])
	AM_RANGE(0x4000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_RAM
	AM_RANGE(0x2000, 0x2000) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0xa000, 0xa000) AM_READWRITE(sound_latch_r, main_latch_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( begas )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "DSWA" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "DSW1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "DSW2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 7, 6, 5, 4, 3, 2, 1, 0, 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	16*16
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 8 },
	{ REGION_GFX1, 0, &spritelayout, 0, 8 },
	{ -1 }
};

static const struct AY8910interface ay8910_interface = { 0, 0, 0, 0 };

static MACHINE_DRIVER_START( deco_ld )
	MDRV_CPU_ADD(M6502, 8000000/2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(deco_ld_interrupt, 1)

	MDRV_CPU_ADD(M6502, 8000000/2)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse, TIME_IN_HZ(640))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(deco_ld)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 8, 247)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(PALETTE_SIZE)
	MDRV_PALETTE_INIT(deco_ld)

	MDRV_VIDEO_START(deco_ld)
	MDRV_VIDEO_UPDATE(deco_ld)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
MACHINE_DRIVER_END

ROM_START( begas )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "an05-3",   0x4000, 0x2000, CRC(c917a283) SHA1(b91f8cd18b8cc1189e4b69d6932d6f01d4ccfb81) )
	ROM_LOAD( "an04-3",   0x6000, 0x2000, CRC(935b2b0a) SHA1(e7c09960607569bd88e9af396aa70661f4352efb) )
	ROM_LOAD( "an03-3",   0x8000, 0x2000, CRC(79438d80) SHA1(e641336f23c6b84d84313ef3e94871ac9aa8b612) )
	ROM_LOAD( "an02-3",   0xa000, 0x2000, CRC(98ce4ca0) SHA1(e7db66b1f0f06b0a21e7450962ba70f460a24847) )
	ROM_LOAD( "an01",     0xc000, 0x2000, CRC(15f8921d) SHA1(32f945bee8f30e5896da38ac6184a11c0a8194bb) )
	ROM_LOAD( "an00-3",   0xe000, 0x2000, CRC(124a3a36) SHA1(e2f7110196cb46fcda429c613388285b46ec1a9e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "an06",   0xe000, 0x2000, CRC(cbbcd730) SHA1(2f2e78fcf2eba71044bec60d27d8756d9b5af551) )

	ROM_REGION( 0xc000, REGION_GFX1, 0 )
	ROM_LOAD( "an0a",   0x0000, 0x2000, CRC(e429305d) SHA1(9a05ab7916235d028b6b05270703516581825660) )
	ROM_LOAD( "an0b",   0x4000, 0x2000, CRC(09e4b780) SHA1(0735420b8529017e507feecf8f74fecd80fbf7d5) )
	ROM_LOAD( "an0c",   0x8000, 0x2000, CRC(0c127207) SHA1(b8372b2fa20ffe5ac278f558c07fd761c86e514b) )
	ROM_LOAD( "an07",   0x2000, 0x2000, CRC(6b8ad735) SHA1(a703523202d40e409e2345a6626b9e29b7a59cd3) )
	ROM_LOAD( "an08",   0x6000, 0x2000, CRC(b5518391) SHA1(57f6407491cff075f76a8b459cc33e8b9a91e7de) )
	ROM_LOAD( "an09",   0xa000, 0x2000, CRC(b7375fd7) SHA1(93a59e99e375bdba77199a705b5e304ece221617) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )
	ROM_LOAD( "af-8.bpr",    0x00, 0x20, CRC(20006a72) SHA1(6d0e1c6de45079f9e128186478a7e0ed3fd471d0) )

	ROM_REGION( 0x1000, REGION_PLDS, 0 )
	ROM_LOAD( "lp1-1.pld",   0x0000, 44, CRC(cc84cb09) SHA1(61620ef30dfd6c81cc517f95ab6358c619ca3298) )
	ROM_LOAD( "lp1-2.pld",   0x0100, 44, CRC(60e16fc4) SHA1(1df735f393ed0fcf1272fceada9764084ff11e07) )
	ROM_LOAD( "lp1-3.pld",   0x0200, 52, CRC(976a7c57) SHA1(202c55a236799fb44a977c074c231ed54c71a872) )
	ROM_LOAD( "lp1-4.pld",   0x0300, 52, CRC(cc9a442f) SHA1(5d08873b204b15f888d02d79e049119e05e41b45) )
	ROM_LOAD( "lp1-5.pld",   0x0400, 44, CRC(2d9f3118) SHA1(02e40a99f131bb47562d5b90fdfb11ca8cd90da6) )
	ROM_LOAD( "lp2-1.pld",   0x0500, 44, CRC(dbb05313) SHA1(fc37db24f12c4f5170945c9ec9a333e4583c1712) )
	ROM_LOAD( "lp2-4.pld",   0x0600, 44, CRC(4c72736c) SHA1(6f7521284a5d960ff05c4361095c3e89a79f7475) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "begas", 0, NO_DUMP )
ROM_END

ROM_START( begas1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "an05",   0x4000, 0x2000, CRC(91a05549) SHA1(425668ee0dcf44bc011ee3649aa82df6ad3180eb) )
	ROM_LOAD( "an04",   0x6000, 0x2000, CRC(670966fe) SHA1(c179e3045ed0e46c5829fce5297ada475141e662) )
	ROM_LOAD( "an03",   0x8000, 0x2000, CRC(d2d85cdf) SHA1(da557ce5c3252297d2c073a0242e1989b0b7388b) )
	ROM_LOAD( "an02",   0xa000, 0x2000, CRC(84d13c20) SHA1(6474d90b84bca88c35cdb1d4c117ce431d6addf7) )
	ROM_LOAD( "an01",   0xc000, 0x2000, CRC(15f8921d) SHA1(32f945bee8f30e5896da38ac6184a11c0a8194bb) )
	ROM_LOAD( "an00",   0xe000, 0x2000, CRC(184297f3) SHA1(6813f076fde3eb583929506b2e65d9cd988b1b75) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "an06",   0xe000, 0x2000, CRC(cbbcd730) SHA1(2f2e78fcf2eba71044bec60d27d8756d9b5af551) )

	ROM_REGION( 0xc000, REGION_GFX1, 0 )
	ROM_LOAD( "an0a",   0x0000, 0x2000, CRC(e429305d) SHA1(9a05ab7916235d028b6b05270703516581825660) )
	ROM_LOAD( "an0b",   0x4000, 0x2000, CRC(09e4b780) SHA1(0735420b8529017e507feecf8f74fecd80fbf7d5) )
	ROM_LOAD( "an0c",   0x8000, 0x2000, CRC(0c127207) SHA1(b8372b2fa20ffe5ac278f558c07fd761c86e514b) )
	ROM_LOAD( "an07",   0x2000, 0x2000, CRC(6b8ad735) SHA1(a703523202d40e409e2345a6626b9e29b7a59cd3) )
	ROM_LOAD( "an08",   0x6000, 0x2000, CRC(b5518391) SHA1(57f6407491cff075f76a8b459cc33e8b9a91e7de) )
	ROM_LOAD( "an09",   0xa000, 0x2000, CRC(b7375fd7) SHA1(93a59e99e375bdba77199a705b5e304ece221617) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )
	ROM_LOAD( "af-8.bpr",    0x00, 0x20, CRC(20006a72) SHA1(6d0e1c6de45079f9e128186478a7e0ed3fd471d0) )

	ROM_REGION( 0x1000, REGION_PLDS, 0 )
	ROM_LOAD( "lp1-1.pld",   0x0000, 44, CRC(cc84cb09) SHA1(61620ef30dfd6c81cc517f95ab6358c619ca3298) )
	ROM_LOAD( "lp1-2.pld",   0x0100, 44, CRC(60e16fc4) SHA1(1df735f393ed0fcf1272fceada9764084ff11e07) )
	ROM_LOAD( "lp1-3.pld",   0x0200, 52, CRC(976a7c57) SHA1(202c55a236799fb44a977c074c231ed54c71a872) )
	ROM_LOAD( "lp1-4.pld",   0x0300, 52, CRC(cc9a442f) SHA1(5d08873b204b15f888d02d79e049119e05e41b45) )
	ROM_LOAD( "lp1-5.pld",   0x0400, 44, CRC(2d9f3118) SHA1(02e40a99f131bb47562d5b90fdfb11ca8cd90da6) )
	ROM_LOAD( "lp2-1.pld",   0x0500, 44, CRC(dbb05313) SHA1(fc37db24f12c4f5170945c9ec9a333e4583c1712) )
	ROM_LOAD( "lp2-4.pld",   0x0600, 44, CRC(4c72736c) SHA1(6f7521284a5d960ff05c4361095c3e89a79f7475) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "begas", 0, NO_DUMP )
ROM_END

ROM_START( rblaster )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "01.bin",   0xc000, 0x2000, CRC(e4733c49) SHA1(357f46a80273f8a365d16cddf5e2caaeeacaf4ad) )
	ROM_LOAD( "00.bin",   0xe000, 0x2000, CRC(084d6ae2) SHA1(f49eb2d53bad5af88a12535ba628c9decce690ff) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "02.bin",   0xe000, 0x2000, CRC(6c20335d) SHA1(b28e80f112553af8e3fba9ebbfc10d1f56396ac1) )

	ROM_REGION( 0xc000, REGION_GFX1, 0 )
	ROM_LOAD( "03.bin",   0x0000, 0x2000, CRC(d1ff5ffb) SHA1(29df207e225e3b0477d5566d256198310d6ae526) )
	ROM_LOAD( "06.bin",   0x2000, 0x2000, CRC(d1ff5ffb) SHA1(29df207e225e3b0477d5566d256198310d6ae526) )
	ROM_LOAD( "04.bin",   0x4000, 0x2000, CRC(da2c84d9) SHA1(3452b0e2a45fa771e226c3a3668afbf3ceb0ec11) )
	ROM_LOAD( "07.bin",   0x6000, 0x2000, CRC(da2c84d9) SHA1(3452b0e2a45fa771e226c3a3668afbf3ceb0ec11) )
	ROM_LOAD( "05.bin",   0x8000, 0x2000, CRC(4608b516) SHA1(44af4be84a0b807ea0813ce86376a4b6fd927e5a) )
	ROM_LOAD( "08.bin",   0xa000, 0x2000, CRC(4608b516) SHA1(44af4be84a0b807ea0813ce86376a4b6fd927e5a) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "rblaster", 0, SHA1(1563ea907d461592e17646848fdf2dce904bba32) )
ROM_END

ROM_START( cobra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "au03-2",   0x8000, 0x2000, CRC(8f0a8fba) SHA1(8e11d2bd665a5ca6b3bb11aa2b707458c1534327) )
	ROM_LOAD( "au02-2",   0xa000, 0x2000, CRC(7db11acf) SHA1(1eebae0741f5735bc8966f3c31a9c07dac2e3916) )
	ROM_LOAD( "au01-2",   0xc000, 0x2000, CRC(523dd8f6) SHA1(47bd4c9b2272e9a710e6e97f2505075df68101ed) )
	ROM_LOAD( "au00-2",   0xe000, 0x2000, CRC(6c0f1f16) SHA1(ed05d3eaa24e84b1dfb4e1eb5f69b23e4a1494ba) )
	ROM_COPY( REGION_CPU1, 0x8000, 0x4000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "au06",   0xe000, 0x2000, CRC(ccc94eb0) SHA1(354a933ddc6a1e1118c2bf176faaab5d01fc92d3) )

	ROM_REGION( 0xc000, REGION_GFX1, 0 )
	ROM_LOAD( "au0a",   0x0000, 0x2000, CRC(6aaedcf3) SHA1(52dc913eecf8a159784d500217cffd7a6d8eb45c) )
	ROM_LOAD( "au0b",   0x4000, 0x2000, CRC(92247877) SHA1(f9bb0c20212ab13caabfb5beb9b6afc807bc9555) )
	ROM_LOAD( "au0c",   0x8000, 0x2000, CRC(d00a2762) SHA1(84d4329b39b9fd30682b7efa5cb2744934c5ee5c) )
	ROM_LOAD( "au07",   0x2000, 0x2000, CRC(d4bf12a5) SHA1(e172f69ae02ac2670b70af0cfcf3887dd99c2761) )
	ROM_LOAD( "au08",   0x6000, 0x2000, CRC(63158274) SHA1(c728e8ba0a11ea67cf508877ad74a3aab9ef26fc) )
	ROM_LOAD( "au09",   0xa000, 0x2000, CRC(74e93394) SHA1(7a1470cf2008b1bef8d950939b758707297b3655) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "cobra", 0, SHA1(8390498294aca97a5d1769032e7b115d1a42f5d3) )
ROM_END

ROM_START( cobraa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "bd03-a4.bin",      0x8000, 0x2000, CRC(f1b9df77) SHA1(7fcc613463441f69e4336c12252a616508c296d1) )
	ROM_LOAD( "bd02-a3.bin",      0xa000, 0x2000, CRC(3d802707) SHA1(89033b2f9b295b74b6b5034c837509377f5aba62) )
	ROM_LOAD( "bd01-a2.bin",      0xc000, 0x2000, CRC(1b4db507) SHA1(c58021cb7dcbcb159d9bb24d231755b3d07aa74a) )
	ROM_LOAD( "bd000-1-a1.bin",   0xe000, 0x2000, CRC(8d9ad777) SHA1(10914251350988a82c0cfa1dc6e22587b885cc71) )
	ROM_COPY( REGION_CPU1, 0x8000, 0x4000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "bd07-b11.bin",   0xe000, 0x2000, CRC(584d714a) SHA1(e3df3c42367d879c5f12db78d2797049ba18e6f8) )

	ROM_REGION( 0xc000, REGION_GFX1, 0 )
	ROM_LOAD( "bd04-.a10",     0x0000, 0x2000, CRC(33013cc2) SHA1(380ca35a891805e2aa1d35000a3770483aaedbfc) )
	ROM_LOAD( "bd06-a8.bin",   0x4000, 0x2000, CRC(b1340125) SHA1(eef325c56d3a441432c7d31ea1dadfaa0c871c10) )
	ROM_LOAD( "bd05-a9.bin",   0x8000, 0x2000, CRC(98412178) SHA1(20e93f377a0d572a6752d7435aa4d0a3feec0e92) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "cobra", 0, SHA1(8390498294aca97a5d1769032e7b115d1a42f5d3) )

	ROM_REGION( 0x40000, REGION_USER1, 0 )
	ROM_LOAD( "lp4-1.pal16l8cn.bin",        0x0000, 0x40000, CRC(4aeb2c7e) SHA1(3c962656cffc8d927047c64a15afccab767d776f) )
	ROM_LOAD( "lp4-1.pal16l8cn.pld",        0x0000, 0x00f71, CRC(ac1f1177) SHA1(ab721a840207354916c96e0ae83220fed12c6352) )
	ROM_LOAD( "lp4-2-pal10l8.d6.bin",       0x0000, 0x0002c, CRC(e594fd13) SHA1(4bb8a9b7cf8f8eaa3c9f290b6e5085a10c927e20) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )
	ROM_LOAD( "vd0-c.h15",        0x0000, 0x00020, CRC(02c27aa0) SHA1(e7b814aabbfbcd992f78254b29b6ab6fa8115429) )
	ROM_LOAD( "vd0-t.f6",         0x0020, 0x00020, CRC(78449942) SHA1(584e25f7bffccd943c4db1edf05552f7989e08a4) )
ROM_END

GAME( 1983, begas,    0,     deco_ld, begas, 0, ROT0, "Data East", "Bega's Battle (Revision 3)", GAME_NOT_WORKING )
GAME( 1983, begas1,   begas, deco_ld, begas, 0, ROT0, "Data East", "Bega's Battle (Revision 1)", GAME_NOT_WORKING )
GAME( 1984, cobra,    0,     deco_ld, begas, 0, ROT0, "Data East", "Cobra Command (Data East LD, set 1)", GAME_NOT_WORKING )
GAME( 1984, cobraa,   cobra, deco_ld, begas, 0, ROT0, "Data East", "Cobra Command (Data East LD, set 2)", GAME_NOT_WORKING )
GAME( 1985, rblaster, 0,     deco_ld, begas, 0, ROT0, "Data East", "Road Blaster (Data East LD)", GAME_NOT_WORKING )

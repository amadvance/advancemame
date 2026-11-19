// license:BSD-3-Clause
// copyright-holders:Uki, David Haywood
/*****************************************************************************

Himeshikibu (C) 1989 Hi-Soft
Android (C) 1987? Nasco

    Driver by Uki

*****************************************************************************/

/*
Himeshikibu
(c)1989 Hi-Soft (distributed by Rollertron)

CPU: Z80x2
XTAL1: 8.000MHz
Sound: YM2203C Y3014B

Android uses PCBS MK-P102 and MK-P101 ONLY, there is no MK-P103
(extra sprites used on Himeshikibu)
*/

#include "driver.h"
#include "sound/2203intf.h"

static UINT8 *bg_ram;
static UINT8 *spriteram_p103a;
static UINT8 *spriteram_main;
static tilemap *bg_tilemap;
static UINT8 scrollx[2];
static UINT8 scrolly;
static UINT8 flipscreen;

static void get_bg_tile_info(int tile_index)
{
	int code = bg_ram[tile_index * 2] + bg_ram[tile_index * 2 + 1] * 0x100;
	int col = code >> 12;

	code &= 0xfff;

	SET_TILE_INFO(0, code, col, 0)
}

static VIDEO_START( himesiki )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 64, 32);
	if (!bg_tilemap)
		return 1;

	return 0;
}

static WRITE8_HANDLER( bg_ram_w )
{
	bg_ram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
}

static WRITE8_HANDLER( scrollx_w )
{
	scrollx[offset] = data;
}

static WRITE8_HANDLER( scrolly_w )
{
	scrolly = data;
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int offs;

	/* sprites of the optional ET-P103A board */
	for (offs = 0; offs < 0x60; offs += 4)
	{
		int attr = spriteram_p103a[offs + 1];
		int code = spriteram_p103a[offs + 0] | (attr & 3) << 8;
		int x = spriteram_p103a[offs + 3] | (attr & 8) << 5;
		int y = spriteram_p103a[offs + 2];
		int col = (attr & 0xf0) >> 4;
		int fx = attr & 4;
		int fy = 0;

		if (x > 0x1e0)
			x -= 0x200;

		if (flipscreen)
		{
			y = (y - 31) & 0xff;
			x = 224 - x;
			fx ^= 4;
			fy = 1;
		}
		else
		{
			y = 257 - y;
			if (y > 0xc0)
				y -= 0x100;
		}

		drawgfx(bitmap, Machine->gfx[2], code, col, fx, fy, x, y, cliprect, TRANSPARENCY_PEN, 15);
	}

	for (offs = 0; offs < 0x100; offs += 4)
	{
		int attr, code, x, y, col, f;

		/* an all zero entry would show a garbage sprite in the corner */
		if (spriteram_main[offs + 0] == 0 && spriteram_main[offs + 1] == 0
			&& spriteram_main[offs + 2] == 0 && spriteram_main[offs + 3] == 0)
			continue;

		attr = spriteram_main[offs + 1];
		code = spriteram_main[offs + 0] | (attr & 7) << 8;
		x = spriteram_main[offs + 3] | (attr & 8) << 5;
		y = spriteram_main[offs + 2];
		col = (attr & 0xf0) >> 4;
		f = 0;

		if (x > 0x1e0)
			x -= 0x200;

		if (flipscreen)
		{
			y = (y - 15) & 0xff;
			x = 240 - x;
			f = 1;
		}
		else
			y = 257 - y;

		y &= 0xff;
		if (y > 0xf0)
			y -= 0x100;

		drawgfx(bitmap, Machine->gfx[1], code, col, f, f, x, y, cliprect, TRANSPARENCY_PEN, 15);
	}
}

static VIDEO_UPDATE( himesiki )
{
	int x = -(scrollx[0] << 8 | scrollx[1]) & 0x1ff;

	tilemap_set_scrolldx(bg_tilemap, x, x);
	tilemap_set_scrolldy(bg_tilemap, -scrolly, -scrolly - 64);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	draw_sprites(bitmap, cliprect);
}

static WRITE8_HANDLER( rombank_w )
{
	memory_set_bank(1, (data & 0x0c) >> 2);

	flipscreen = (data & 0x10) >> 4;
	flip_screen_set(flipscreen);
}

static WRITE8_HANDLER( sound_w )
{
	soundlatch_w(offset, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static READ8_HANDLER( port_1p_r ) { return readinputportbytag("1P"); }
static READ8_HANDLER( port_2p_r ) { return readinputportbytag("2P"); }
static READ8_HANDLER( port_others_r ) { return readinputportbytag("OTHERS"); }
static READ8_HANDLER( port_dsw1_r ) { return readinputportbytag("DSW1"); }
static READ8_HANDLER( port_dsw2_r ) { return readinputportbytag("DSW2"); }

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xa0ff) AM_RAM AM_BASE(&spriteram_main)
	AM_RANGE(0xa100, 0xa7ff) AM_RAM AM_BASE(&spriteram_p103a)
	AM_RANGE(0xa800, 0xafff) AM_RAM AM_WRITE(paletteram_xRRRRRGGGGGBBBBB_le_w) AM_BASE(&paletteram)
	AM_RANGE(0xb000, 0xbfff) AM_RAM AM_WRITE(bg_ram_w) AM_BASE(&bg_ram)
	AM_RANGE(0xc000, 0xffff) AM_ROMBANK(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(port_1p_r)
	AM_RANGE(0x01, 0x01) AM_READ(port_2p_r)
	AM_RANGE(0x02, 0x02) AM_READ(port_others_r)
	AM_RANGE(0x03, 0x03) AM_WRITENOP
	AM_RANGE(0x04, 0x04) AM_READ(port_dsw1_r)
	AM_RANGE(0x05, 0x05) AM_READ(port_dsw2_r)
	AM_RANGE(0x06, 0x06) AM_WRITE(rombank_w)
	AM_RANGE(0x07, 0x07) AM_WRITENOP
	AM_RANGE(0x08, 0x08) AM_WRITE(scrolly_w)
	AM_RANGE(0x09, 0x0a) AM_WRITE(scrollx_w)
	AM_RANGE(0x0b, 0x0b) AM_WRITE(sound_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0x04, 0x04) AM_READ(soundlatch_r)
ADDRESS_MAP_END


INPUT_PORTS_START( himesiki )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "1-2" )                   PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "1-3" )                   PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )      PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x10, "1-5" )                   PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "1-6" )                   PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )  PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "2-1" )                   PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "2-2" )                   PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "2-3" )                   PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "2-4" )                   PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "2-5" )                   PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "2-6" )                   PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2-7" )                   PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2-8" )                   PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("1P")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("2P")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )        PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )        PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )        PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("OTHERS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) // coin?
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 ) // service?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( androidpo )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" ) // first dragon scene only shows 3?
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, "Invalid" ) // can't coin up or start? (probably a non-functioning freeplay)

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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("1P")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("2P")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )        PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )        PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )        PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("OTHERS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) // coin?
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 ) // service?
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( androidp )
	PORT_INCLUDE( androidpo )

	PORT_INCLUDE(androidpo)

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )

	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
INPUT_PORTS_END


static const gfx_layout layout_bg =
{
	8, 8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ STEP8(0,32) },
	8*8*4
};

static const gfx_layout layout_sprite =
{
	16, 16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24, 36, 32, 44, 40, 52, 48, 60, 56 },
	{ STEP16(0,64) },
	16*16*4
};

static const gfx_layout layout_p103a =
{
	32, 32,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24, 36, 32, 44, 40, 52, 48, 60, 56,
		68, 64, 76, 72, 84, 80, 92, 88, 100, 96, 108, 104, 116, 112, 124, 120 },
	{ STEP16(0,128), STEP16(16*128,128) },
	32*32*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_bg,     0x000, 16 },
	{ REGION_GFX2, 0, &layout_sprite, 0x200, 16 },
	{ REGION_GFX3, 0, &layout_p103a,  0x200, 16 },
	{ -1 }
};

static void ym2203_irq(int irq)
{
	cpunum_set_input_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	0, 0, 0, 0,
	ym2203_irq
};

static MACHINE_START( himesiki )
{
	memory_configure_bank(1, 0, 4, memory_region(REGION_USER1), 0x4000);

	state_save_register_global_array(scrollx);
	state_save_register_global(scrolly);
	state_save_register_global(flipscreen);
	return 0;
}

static MACHINE_RESET( himesiki )
{
	scrollx[0] = 0;
	scrollx[1] = 0;
	scrolly = 0;
	flipscreen = 0;
}

static MACHINE_DRIVER_START( himesiki )
	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(main_io_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_IO_MAP(sound_io_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)

	MDRV_MACHINE_START(himesiki)
	MDRV_MACHINE_RESET(himesiki)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 24*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(himesiki)
	MDRV_VIDEO_UPDATE(himesiki)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 2000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.10)
	MDRV_SOUND_ROUTE(1, "mono", 0.10)
	MDRV_SOUND_ROUTE(2, "mono", 0.10)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( himesiki )
	ROM_REGION( 0x08000, REGION_CPU1, 0 )
	ROM_LOAD( "1.1k",  0x00000,  0x08000, CRC(fb4604b3) SHA1(e8155bbafb881125e1bf9a04808d6a6546887e90) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "2.1g",  0x00000,  0x04000, CRC(0c30ded1) SHA1(0ad67115fa15d0b6261a278a946a6d46c06430ef) )
	ROM_CONTINUE(      0x08000,  0x04000)

	ROM_REGION( 0x08000, REGION_CPU2, 0 )
	ROM_LOAD( "5.6n",  0x00000,  0x08000, CRC(b1214ac7) SHA1(ee5459c28d9c3c2eb3467261716b1259ec486534) )

	ROM_REGION( 0x020000, REGION_GFX1, 0 )
	ROM_LOAD( "3.5f",  0x000000,  0x010000, CRC(73843e60) SHA1(0d8a397d8798e15f3fa7bf7a83e4c2ee44f6fa86) )
	ROM_LOAD( "4.5d",  0x010000,  0x010000, CRC(443a3164) SHA1(08aa002214251a870581a01d775f497dd390957c) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "13.9e", 0x000000,  0x010000, CRC(43102682) SHA1(0d4bde8bece0cbc6c06071aa8ad210a0636d862f) )
	ROM_LOAD16_BYTE( "12.9c", 0x000001,  0x010000, CRC(19c8f9f4) SHA1(b14c8a6b94fd474be375e7a6a03d7f4517da2247) )
	ROM_LOAD16_BYTE( "15.8e", 0x020000,  0x010000, CRC(2630d394) SHA1(b2e9e836b1f053fce3212912c53d3cdca3372439) )
	ROM_LOAD16_BYTE( "14.8c", 0x020001,  0x010000, CRC(8103a207) SHA1(0dde8a0aaf2618d9c1589f35841db210439d0388) )

	ROM_REGION( 0x060000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "6.1a",  0x000000,  0x010000, CRC(14989c22) SHA1(fe0c31df10237294ea8ef0ab8965ba5bb25113a2) )
	ROM_LOAD16_BYTE( "7.1c",  0x000001,  0x010000, CRC(cec56e16) SHA1(836ff413301044313fdf7af5d304c145137b898a) )
	ROM_LOAD16_BYTE( "8.2a",  0x020000,  0x010000, CRC(44ba127e) SHA1(d756b6c3075d75287f9c8be662c1eab02f4245a3) )
	ROM_LOAD16_BYTE( "9.2c",  0x020001,  0x010000, CRC(0dda724a) SHA1(2b064b1d657f896e8385f17def9e4ffc0802bf97) )
	ROM_LOAD16_BYTE( "10.4a", 0x040000,  0x010000, CRC(0adda8d1) SHA1(dfee2c7921fdc972b4e95fdf89520f74a4e8b4ee) )
	ROM_LOAD16_BYTE( "11.4c", 0x040001,  0x010000, CRC(aa032946) SHA1(bd8900e4a22580e3bfe33b8164909db19bb07a8f) )
ROM_END

ROM_START( androidpo )
	ROM_REGION( 0x08000, REGION_CPU1, 0 )
	ROM_LOAD( "mitsubishi__ad1__m5l27256k.toppcb.k1", 0x00000, 0x08000, CRC(25ab85eb) SHA1(e1fab149c83ff880b119258206d5818f3db641c5) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "mitsubishi__ad-3__m5l27256k.toppcb.g1", 0x00000, 0x04000, CRC(6cf5f48a) SHA1(b9b4e5e7bace0e8d98fbc9f4ad91bc56ef42099e) )
	ROM_CONTINUE(                                      0x08000, 0x04000)
	ROM_LOAD( "mitsubishi__ad2__m5l27256k.toppcb.j1",  0x04000, 0x04000, CRC(e41426be) SHA1(e7e06ef3ff5160bb7d870e148ba2799da52cf24c) )
	ROM_CONTINUE(                                      0x0c000, 0x04000)

	ROM_REGION( 0x08000, REGION_CPU2, 0 )
	ROM_LOAD( "mitsubishi__ad-4__m5l27256k.toppcb.n6", 0x00000, 0x08000, CRC(13c38fe4) SHA1(34a35fa057159a5c83892a88b8c908faa39d5cb3) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 )
	ROM_LOAD( "mitsubishi__ad-5__m5l27512k.toppcb.f5", 0x00000, 0x10000, CRC(4c72a930) SHA1(f1542844391b55fe43293eef7ce48c09b7aca75a) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mitsubishi__ad-6__m5l27512k.botpcb.def9", 0x00000, 0x10000, CRC(5e42984e) SHA1(2a928960c740dfb94589e011cce093bed2fd7685) )
	ROM_LOAD16_BYTE( "mitsubishi__ad-7__m5l27512k.botpcb.bc9",  0x00001, 0x10000, CRC(611ff400) SHA1(1a9aed33d0e3f063811f92b9fee3ecbff0e965bf) )

	ROM_REGION( 0x2000, REGION_USER2, 0 )
	ROM_LOAD( "ricoh_7a2_19__epl10p8bp_japan_i.f1.jed", 0x0000, 0x0473, CRC(c5e51ea2) SHA1(3e35a30935f562227f0afa32a6be6eb33f9a8372) )
	ROM_LOAD( "ricoh_7a2_19__epl10p8bp_japan_m.j3.jed", 0x1000, 0x0473, CRC(807d1553) SHA1(257be9eacd57d2e2dbaab3be5725d8d06d6a9a0b) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_ERASEFF )
	
ROM_END

ROM_START( androidp )
	ROM_REGION( 0x08000, REGION_CPU1, 0 )
	ROM_LOAD( "andr1.bin", 0x00000, 0x08000, CRC(fff04130) SHA1(9bdafa8b311cc5d0851b04df3c6dd16eb087a5dd) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "andr3.bin", 0x00000, 0x04000, CRC(112d5123) SHA1(653109eae7b58d9dcb8892ea9aca17427f14c145) )
	ROM_CONTINUE(          0x08000, 0x04000)

	ROM_REGION( 0x08000, REGION_CPU2, 0 )
	ROM_LOAD( "andr4.bin", 0x00000, 0x08000, CRC(65f5e98b) SHA1(69f979d653695413a1c503c402d4bf5ffcfb6e5d) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 )
	ROM_LOAD( "andr5.bin", 0x00000, 0x10000, CRC(0a0b44c0) SHA1(8d359b802c7dee5faea9464f06b672fd401799cf) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "andr6.bin", 0x00000, 0x10000, CRC(122b7dd1) SHA1(5dffd2b97c8222afc98552513b84a91d6127f41b) )
	ROM_LOAD16_BYTE( "andr7.bin", 0x00001, 0x10000, CRC(fc0f9234) SHA1(496a918cc1f4d0e7191a49cc43c51fbd71e0bdf5) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_ERASEFF )

	
ROM_END


GAME( 1989, himesiki,  0,        himesiki, himesiki,  0, ROT90, "Hi-Soft", "Himeshikibu (Japan)", GAME_SUPPORTS_SAVE )
GAME( 1987, androidp,  0,        himesiki, androidp,  0, ROT90, "Nasco", "Android (prototype, later build)", GAME_SUPPORTS_SAVE )
GAME( 198?, androidpo, androidp, himesiki, androidpo, 0, ROT90, "Nasco", "Android (prototype, early build)", GAME_SUPPORTS_SAVE )

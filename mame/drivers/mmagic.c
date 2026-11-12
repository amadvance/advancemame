// license:GPL-2.0+
// copyright-holders:Dirk Best
/***************************************************************************

    "Monkey Magic" (c) 1979 Nintendo

    TZF-MP  - Main Board
    TZF-SOU - Sound Board

    CPU is an NEC D8085A, crystal is marked 6.1440.
    Horizontal sync 15.9982 kHz, vertical sync 60.5992 Hz.

***************************************************************************/

#include "driver.h"
#include "sound/samples.h"

static UINT8 ball_x, ball_y;
static UINT8 tile_color_base;
static UINT8 audio_state;
static tilemap *bg_tilemap;

/*************************************
 *
 *  Sound
 *
 *************************************/

static const char *mmagic_sample_names[] =
{
	"*mmagic",
	"4",
	"3",
	"5",
	"2",
	"2-2",
	"6",
	"6-2",
	"1",
	0
};

static struct Samplesinterface mmagic_samples_interface =
{
	1,
	mmagic_sample_names,
	0
};

static WRITE8_HANDLER( mmagic_audio_w )
{
	data ^= 0xff;
	if (data != audio_state)
	{
		if (data & 0x80)
			sample_start(0, audio_state & 7, 0);

		audio_state = data;
	}
}

/*************************************
 *
 *  Video
 *
 *************************************/

static void get_tile_info(int tile_index)
{
	int code = videoram[tile_index] & 0x7f;
	int color = (memory_region(REGION_PROMS)[code | tile_color_base] ^ 0xff) & 7;

	SET_TILE_INFO(0, code, color, 0)
}

static VIDEO_START( mmagic )
{
	bg_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 12, 32, 16);
	if (!bg_tilemap)
		return 1;
	return 0;
}

static VIDEO_UPDATE( mmagic )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* the ball is disabled with x = 0xff */
	if (ball_x != 0xff)
	{
		int x = ball_x;
		int y = (ball_y >> 4) * 12 + (ball_y & 15);

		if (flip_screen)
		{
			x = 255 - x;
			y = 191 - y;
		}
		plot_box(bitmap, x, y, 4, 4, Machine->pens[16]);
	}
}

static PALETTE_INIT( mmagic )
{
	int i;

	for (i = 0; i < 16; ++i)
	{
		int r = (!(i & 1) && (i & 2)) ? 0xff : 0x00;
		int g = (!(i & 1) && (i & 4)) ? 0xff : 0x00;
		int b = (!(i & 1) && (i & 8)) ? 0xff : 0x00;

		palette_set_color(i, r, g, b);
		colortable[i] = i;
	}

	/* ball */
	palette_set_color(16, 0xff, 0xff, 0xff);
}

/*************************************
 *
 *  Memory handlers
 *
 *************************************/

static WRITE8_HANDLER( mmagic_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static WRITE8_HANDLER( mmagic_ball_x_w )
{
	ball_x = data;
}

static WRITE8_HANDLER( mmagic_ball_y_w )
{
	ball_y = data;
}

static READ8_HANDLER( mmagic_vblank_r )
{
	return 0xfe | (cpu_getvblank() ? 1 : 0);
}

static WRITE8_HANDLER( mmagic_color_w )
{
	int base = (data & 0x40) << 1;

	flip_screen_set(data & 8);

	if (tile_color_base != base)
	{
		tile_color_base = base;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

static READ8_HANDLER( mmagic_paddle_r )
{
	return readinputport(flip_screen ? 3 : 4);
}

static READ8_HANDLER( mmagic_buttons_r )
{
	if (flip_screen)
		return readinputport(1);
	return readinputport(2) | (readinputport(1) & ~1);
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x21ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x3000, 0x31ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8004, 0x8004) AM_READ(mmagic_vblank_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x21ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x3000, 0x31ff) AM_WRITE(mmagic_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8002, 0x8002) AM_WRITE(mmagic_ball_x_w)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(mmagic_ball_y_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x85, 0x85) AM_READ(mmagic_paddle_r)
	AM_RANGE(0x86, 0x86) AM_READ(mmagic_buttons_r)
	AM_RANGE(0x87, 0x87) AM_READ(input_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x80, 0x80) AM_WRITE(mmagic_color_w)
	AM_RANGE(0x81, 0x81) AM_WRITE(mmagic_audio_w)
ADDRESS_MAP_END

INPUT_PORTS_START( mmagic )
	PORT_START	/* DIPSW */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "10000" )
	PORT_DIPSETTING(    0x04, "15000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START	/* ANALOG0 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_CENTERDELTA(0)

	PORT_START	/* ANALOG1 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_CENTERDELTA(0) PORT_PLAYER(2)
INPUT_PORTS_END

static const gfx_layout tile_layout =
{
	8, 12,
	128,
	1,
	{ 0 },
	{ 3, 2, 1, 0, 7, 6, 5, 4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
	16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tile_layout, 0, 8 },
	{ -1 }
};

static MACHINE_DRIVER_START( mmagic )
	MDRV_CPU_ADD(8085A, 6144000 / 2)
	MDRV_CPU_PROGRAM_MAP(readmem, writemem)
	MDRV_CPU_IO_MAP(readport, writeport)

	MDRV_FRAMES_PER_SECOND(6144000.0 / (384 * 264))
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_VISIBLE_AREA(0, 255, 0, 191)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(17)
	MDRV_COLORTABLE_LENGTH(16)
	MDRV_PALETTE_INIT(mmagic)
	MDRV_VIDEO_START(mmagic)
	MDRV_VIDEO_UPDATE(mmagic)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(mmagic_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( mmagic )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1ai.2a",  0x0000, 0x0400, CRC(ec772e2e) SHA1(7efc1bbb24b2ed73c518aea1c4ef4b9a93034e31) )
	ROM_LOAD( "2ai.3a",  0x0400, 0x0400, CRC(e5d482ca) SHA1(208b808e9208bb6f5f5f89ffbeb5a885be33733a) )
	ROM_LOAD( "3ai.4a",  0x0800, 0x0400, CRC(e8d38deb) SHA1(d7384234fb47e4b1d0421f58571fa748662b05f5) )
	ROM_LOAD( "4ai.45a", 0x0c00, 0x0400, CRC(3048bd6c) SHA1(740051589f6ba44b2ee68edf76a3177bb973d78e) )
	ROM_LOAD( "5ai.5a",  0x1000, 0x0400, CRC(2cab8f04) SHA1(203a3c005f18f968cd14c972bbb9fd7e0fc3b670) )
	ROM_FILL( 0x1400, 0x0400, 0xff )

	ROM_REGION( 0x800, REGION_GFX1, 0 )
	ROM_LOAD( "6h.6hi", 0x000, 0x200, CRC(b6321b6f) SHA1(06611f7419d2982e006a3e81b79677e59e194f38) )
	ROM_LOAD( "7h.7hi", 0x200, 0x200, CRC(9ec0e82c) SHA1(29983f690a1b6134bb1983921f42c14898788095) )
	ROM_LOAD( "6j.6jk", 0x400, 0x200, CRC(7ce83302) SHA1(1870610ff07ab11622e183e04e3fce29328ff291) )
	ROM_FILL( 0x600, 0x200, 0xff )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "7j.7jk", 0x000, 0x200, CRC(b7eb8e1c) SHA1(b65a8efb88668dcf1c1d00e31a9b15a67c2972c8) )
ROM_END

GAME( 1979, mmagic, 0, mmagic, mmagic, 0, ROT90, "Nintendo", "Monkey Magic", GAME_IMPERFECT_SOUND )

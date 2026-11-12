// license:BSD-3-Clause
// copyright-holders:smf
/*
Block Fever (Nintendo, 1978)

CPU Board TBF-CPU

  NEC D8085AC CPU, 6.144 MHz crystal
  4 x 1k EPROM program ROMs (A1-D1)
  NEC M3624 4096 bit PROM character generator (MR005/MR006/unpopulated)
  National Semiconductor ADC0800PN paddle ADC
*/

#include "driver.h"
#include "sound/custom.h"
#include "streams.h"

static UINT8 ball_x, ball_y;
static tilemap *bg_tilemap;

/*************************************
 *
 *  Sound
 *
 *************************************/

#define SOUND_RATE 44100

static sound_stream *beep_stream;
static int beep_state[3];
static int beep_counter[3];
static const int beep_frequency[3] = { 1332, 666, 333 };

static void beep_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int i, j;

	for (i = 0; i < samples; ++i)
	{
		int sample = 0;

		for (j = 0; j < 3; ++j)
		{
			if (beep_state[j])
			{
				/* square wave at the given frequency */
				sample += (beep_counter[j] < SOUND_RATE / 2) ? 0x1000 : -0x1000;
				beep_counter[j] += beep_frequency[j] * 2;
				if (beep_counter[j] >= SOUND_RATE)
					beep_counter[j] -= SOUND_RATE;
			}
		}
		outputs[0][i] = sample;
	}
}

static void beep_reset(void *token)
{
	memset(beep_state, 0, sizeof(beep_state));
	memset(beep_counter, 0, sizeof(beep_counter));
}

static void *beep_start(int clock, const struct CustomSound_interface *config)
{
	beep_reset(0);
	beep_stream = stream_create(0, 1, SOUND_RATE, 0, beep_update);
	return beep_stream;
}

static const struct CustomSound_interface beep_interface =
{
	beep_start, 0, beep_reset, 0
};

/*************************************
 *
 *  Video
 *
 *************************************/

static void get_tile_info(int tile_index)
{
	SET_TILE_INFO(0, videoram[tile_index] & 0x7f, 0, 0)
}

static VIDEO_START( blockfvr )
{
	bg_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 12, 32, 16);
	if (!bg_tilemap)
		return 1;
	return 0;
}

static VIDEO_UPDATE( blockfvr )
{
	int x, y;

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	y = (ball_y >> 4) * 12 + (ball_y & 15);
	x = ball_x;
	if (flip_screen)
	{
		x = 255 - x;
		y = 191 - y;
	}
	plot_box(bitmap, x, y, 4, 4, Machine->pens[0]);
}

static PALETTE_INIT( blockfvr )
{
	palette_set_color(0, 0xff, 0xff, 0xff);
	palette_set_color(1, 0x00, 0x00, 0x00);
	colortable[0] = 0;
	colortable[1] = 1;
}

/*************************************
 *
 *  Memory handlers
 *
 *************************************/

static WRITE8_HANDLER( blockfvr_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static WRITE8_HANDLER( blockfvr_sound_w )
{
	int i;

	stream_update(beep_stream, 0);
	for (i = 0; i < 3; ++i)
		beep_state[i] = (data >> i) & 1;
	flip_screen_set(data & 8);
}

static WRITE8_HANDLER( blockfvr_lamp_w )
{
	set_led_status(0, (data >> 1) & 1);
	set_led_status(1, (data >> 2) & 1);
}

static WRITE8_HANDLER( blockfvr_ball_x_w )
{
	ball_x = data;
}

static WRITE8_HANDLER( blockfvr_ball_y_w )
{
	ball_y = data;
}

static READ8_HANDLER( blockfvr_vblank_r )
{
	return 0xfe | (cpu_getvblank() ? 1 : 0);
}

static READ8_HANDLER( blockfvr_adc_r )
{
	return readinputport(flip_screen ? 3 : 4);
}

static READ8_HANDLER( blockfvr_in_r )
{
	UINT8 data = (readinputport(flip_screen ? 1 : 2) & 1) | (readinputport(1) & ~1);

	coin_counter_w(0, !(data & 8));
	return data;
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1000, 0x11ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x21ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8004, 0x8004) AM_READ(blockfvr_vblank_r)
	AM_RANGE(0x8005, 0x8005) AM_READ(blockfvr_adc_r)
	AM_RANGE(0x8006, 0x8006) AM_READ(blockfvr_in_r)
	AM_RANGE(0x8007, 0x8007) AM_READ(input_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1000, 0x11ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x21ff) AM_WRITE(blockfvr_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(blockfvr_sound_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(blockfvr_lamp_w)
	AM_RANGE(0x8002, 0x8002) AM_WRITE(blockfvr_ball_x_w)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(blockfvr_ball_y_w)
ADDRESS_MAP_END

INPUT_PORTS_START( blockfvr )
	PORT_START	/* DIPSW */
	PORT_DIPNAME( 0x01, 0x01, "Replay" ) PORT_DIPLOCATION("DSW:8")
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPNAME( 0x06, 0x06, "Replay Points" ) PORT_DIPLOCATION("DSW:7,6")
	PORT_DIPSETTING(    0x00, "1600" )
	PORT_DIPSETTING(    0x02, "1200" )
	PORT_DIPSETTING(    0x04, "800" )
	PORT_DIPSETTING(    0x06, "400" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) ) PORT_DIPLOCATION("DSW:5,4")
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPNAME( 0x20, 0x20, "Game A First Rack Flashing Blocks" ) PORT_DIPLOCATION("DSW:3")
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Game A Blocks Descend" ) PORT_DIPLOCATION("DSW:2")
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x70, 0x60, "Game" )
	PORT_DIPSETTING(    0x60, "A" )
	PORT_DIPSETTING(    0x50, "B" )
	PORT_DIPSETTING(    0x30, "C" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START	/* ADC0800PN_0 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_REVERSE PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_CENTERDELTA(0)

	PORT_START	/* ADC0800PN_1 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_REVERSE PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_CENTERDELTA(0) PORT_PLAYER(2)
INPUT_PORTS_END

static const gfx_layout tile_layout =
{
	8, 12,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
	16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tile_layout, 0, 1 },
	{ -1 }
};

static MACHINE_DRIVER_START( blockfvr )
	MDRV_CPU_ADD(8085A, 6144000 / 2)
	MDRV_CPU_PROGRAM_MAP(readmem, writemem)

	MDRV_FRAMES_PER_SECOND(6144000.0 / (384 * 264))
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_VISIBLE_AREA(0, 255, 0, 191)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2)
	MDRV_PALETTE_INIT(blockfvr)
	MDRV_VIDEO_START(blockfvr)
	MDRV_VIDEO_UPDATE(blockfvr)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(beep_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( blockfvr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "d1_.4",   0x000000, 0x000400, CRC(c3efd8e7) SHA1(54613d94e8674e711215a8a65a25c455e18bd37a) )
	ROM_LOAD( "c1_.3",   0x000400, 0x000400, CRC(c0540779) SHA1(4773deabdd3352467cc9eb69cc788b8bd22a78ff) )
	ROM_LOAD( "b1_.2",   0x000800, 0x000400, CRC(8ba0436d) SHA1(ae31ece035b1f5841068bfc02762200fd56dba5e) )
	ROM_LOAD( "a1_.1",   0x000c00, 0x000400, CRC(e6de8867) SHA1(9dad61fc26cd6208cc9a3020527f38cf85f41556) )

	ROM_REGION( 0x800, REGION_GFX1, 0 )
	ROM_LOAD( "mr005.9",  0x000000, 0x000200, CRC(0b2f27a5) SHA1(65c7f943898df65330b8e6dce866ed86507a62d8) )
	ROM_LOAD( "mr006.20", 0x000200, 0x000200, CRC(eadec4c5) SHA1(2ce6e6216163216097e57295f6dc54ff221382c0) )
	ROM_FILL( 0x400, 0x200, 0xff )
	ROM_FILL( 0x600, 0x200, 0xff )
ROM_END

GAME( 1978, blockfvr, 0, blockfvr, blockfvr, 0, ROT90, "Nintendo", "Block Fever", GAME_IMPERFECT_SOUND )

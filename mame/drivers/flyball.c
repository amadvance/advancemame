// license:BSD-3-Clause
// copyright-holders:Stefan Jokisch
/***************************************************************************

Atari Flyball Driver

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "streams.h"
#include "sound/custom.h"

extern VIDEO_START( flyball );
extern VIDEO_UPDATE( flyball );

extern UINT8 flyball_pitcher_pic;
extern UINT8 flyball_pitcher_vert;
extern UINT8 flyball_pitcher_horz;
extern UINT8 flyball_ball_vert;
extern UINT8 flyball_ball_horz;

extern UINT8* flyball_playfield_ram;

static UINT8 flyball_potmask;
static UINT8 flyball_potsense;

static UINT8 *rombase;


static void flyball_joystick_callback(int potsense)
{
	if (potsense & ~flyball_potmask)
	{
		cpunum_set_input_line(0, 0, PULSE_LINE);
	}

	flyball_potsense |= potsense;
}


static void flyball_quarter_callback(int scanline)
{
	int potsense[64], i;

	memset(potsense, 0, sizeof potsense);

	potsense[readinputport(1)] |= 1;
	potsense[readinputport(2)] |= 2;
	potsense[readinputport(3)] |= 4;
	potsense[readinputport(4)] |= 8;

	for (i = 0; i < 64; i++)
	{
		if (potsense[i] != 0)
		{
			timer_set(cpu_getscanlinetime(scanline + i), potsense[i], flyball_joystick_callback);
		}
	}

	scanline += 0x40;
	scanline &= 0xff;

	timer_set(cpu_getscanlinetime(scanline), scanline, flyball_quarter_callback);

	flyball_potsense = 0;
	flyball_potmask = 0;
}


static MACHINE_RESET( flyball )
{
	int i;

	/* address bits 0 through 8 are inverted */

	UINT8* ROM = memory_region(REGION_CPU1) + 0x2000;

	for (i = 0; i < 0x1000; i++)
		rombase[i] = ROM[i ^ 0x1ff];

	timer_set(cpu_getscanlinetime(0), 0, flyball_quarter_callback);
}


/* two physical buttons (start game and stop runner) share the same port bit */

READ8_HANDLER( flyball_input_r )
{
	return readinputport(0) & readinputport(5) & readinputport(6);
}

READ8_HANDLER( flyball_scanline_r )
{
	return cpu_getscanline() & 0x3f;
}

READ8_HANDLER( flyball_potsense_r )
{
	return flyball_potsense & ~flyball_potmask;
}

WRITE8_HANDLER( flyball_potmask_w )
{
	flyball_potmask |= data & 0xf;
}

WRITE8_HANDLER( flyball_pitcher_pic_w )
{
	flyball_pitcher_pic = data & 0xf;
}

WRITE8_HANDLER( flyball_ball_vert_w )
{
	flyball_ball_vert = data;
}

WRITE8_HANDLER( flyball_ball_horz_w )
{
	flyball_ball_horz = data;
}

WRITE8_HANDLER( flyball_pitcher_vert_w )
{
	flyball_pitcher_vert = data;
}

WRITE8_HANDLER( flyball_pitcher_horz_w )
{
	flyball_pitcher_horz = data;
}

/* simplified model of the discrete sound circuit */

#define FLYBALL_BAT_FREQ1 1640
#define FLYBALL_BAT_FREQ2 1524

static sound_stream *flyball_channel;
static int flyball_snd_bat;
static int flyball_snd_footstep;
static int flyball_snd_crowd_on;
static int flyball_snd_crowd_soft;
static int flyball_snd_crowd_very;

static UINT32 flyball_snd_rng;
static unsigned flyball_snd_bat_phase1;
static unsigned flyball_snd_bat_phase2;
static double flyball_snd_foot_env;
static double flyball_snd_crowd_env;
static double flyball_snd_crowd_x1;
static double flyball_snd_crowd_x2;
static double flyball_snd_crowd_y1;
static double flyball_snd_crowd_y2;
static double flyball_snd_crowd_x3;
static double flyball_snd_crowd_x4;
static double flyball_snd_crowd_y3;
static double flyball_snd_crowd_y4;

static void flyball_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	stream_sample_t *buffer = outputs[0];
	double rate = Machine->sample_rate;

	/* the attenuators close when their control line is high, so the sounds are active with the line low */
	/* the crowd volume is a resistor divider (10k, 24.4k, 10k, 33k) smoothed by a 100uF capacitor */
	double g_on = 1 / 10.0;
	double g_very = 1 / 10.0 + 1 / 24.4;
	double g_soft = 1 / 33.0;
	double g_high = (flyball_snd_crowd_on ? 0 : g_on) + (flyball_snd_crowd_very ? 0 : g_very) + (flyball_snd_crowd_soft ? 0 : g_soft);
	double crowd_target = g_high / (g_on + g_very + g_soft);
	double foot_target = flyball_snd_footstep ? 0.0 : 1.0;

	/* time constants: 3.3k * 10uF for the footstep, about 3.7k * 100uF for the crowd */
	double foot_k = 1 - exp(-1 / (0.033 * rate));
	double crowd_k = 1 - exp(-1 / (0.37 * rate));

	/* two cascaded band pass filters centered at 500 Hz */
	double bp_w = 2 * 3.14159265 * 500 / rate;
	double bp_alpha = sin(bp_w) / (2 * 1.0);
	double bp_a0 = 1 + bp_alpha;
	double bp_b0 = bp_alpha / bp_a0;
	double bp_a1 = -2 * cos(bp_w) / bp_a0;
	double bp_a2 = (1 - bp_alpha) / bp_a0;

	while (length--)
	{
		double noise, stage, filtered, out;

		/* 17 bit LFSR noise */
		flyball_snd_rng = (flyball_snd_rng >> 1) | (((flyball_snd_rng ^ (flyball_snd_rng >> 3)) & 1) << 16);
		noise = (flyball_snd_rng & 1) ? 1.0 : -1.0;

		flyball_snd_foot_env += (foot_target - flyball_snd_foot_env) * foot_k;
		flyball_snd_crowd_env += (crowd_target - flyball_snd_crowd_env) * crowd_k;

		/* band pass filtered noise for the crowd */
		filtered = bp_b0 * (noise - flyball_snd_crowd_x2) - bp_a1 * flyball_snd_crowd_y1 - bp_a2 * flyball_snd_crowd_y2;
		flyball_snd_crowd_x2 = flyball_snd_crowd_x1;
		flyball_snd_crowd_x1 = noise;
		flyball_snd_crowd_y2 = flyball_snd_crowd_y1;
		flyball_snd_crowd_y1 = filtered;
		stage = filtered;
		filtered = bp_b0 * (stage - flyball_snd_crowd_x4) - bp_a1 * flyball_snd_crowd_y3 - bp_a2 * flyball_snd_crowd_y4;
		flyball_snd_crowd_x4 = flyball_snd_crowd_x3;
		flyball_snd_crowd_x3 = stage;
		flyball_snd_crowd_y4 = flyball_snd_crowd_y3;
		flyball_snd_crowd_y3 = filtered;

		out = noise * flyball_snd_foot_env * 0.25;
		out += filtered * flyball_snd_crowd_env * 0.27;

		if (flyball_snd_bat)
		{
			/* two astable oscillators mixed together */
			flyball_snd_bat_phase1 += (unsigned)(FLYBALL_BAT_FREQ1 * 65536.0 / rate);
			flyball_snd_bat_phase2 += (unsigned)(FLYBALL_BAT_FREQ2 * 65536.0 / rate);
			out += ((flyball_snd_bat_phase1 & 0x8000) ? 0.2 : -0.2) + ((flyball_snd_bat_phase2 & 0x8000) ? 0.2 : -0.2);
		}

		if (out > 1)
			out = 1;
		else if (out < -1)
			out = -1;

		*buffer++ = (stream_sample_t)(out * 32767 * 0.5);
	}
}

static void *flyball_sh_start(int clock, const struct CustomSound_interface *config)
{
	flyball_channel = stream_create(0, 1, Machine->sample_rate, 0, flyball_sound_update);
	flyball_snd_rng = 1;
	return auto_malloc(1);
}

static struct CustomSound_interface flyball_custom_interface =
{
	flyball_sh_start
};


WRITE8_HANDLER( flyball_misc_w )
{
	int bit = ~data & 1;

	if (offset >= 1 && offset <= 5)
		stream_update(flyball_channel, 0);

	switch (offset)
	{
	case 0:
		set_led_status(0, bit);
		break;
	case 1:
		flyball_snd_crowd_very = bit;
		break;
	case 2:
		flyball_snd_footstep = bit;
		break;
	case 3:
		flyball_snd_crowd_on = bit;
		break;
	case 4:
		flyball_snd_crowd_soft = bit;
		break;
	case 5:
		flyball_snd_bat = bit;
		break;
	}
}


static ADDRESS_MAP_START( flyball_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(13) )
	AM_RANGE(0x0000, 0x00ff) AM_MIRROR(0x100) AM_RAM
	AM_RANGE(0x0800, 0x0800) AM_NOP
	AM_RANGE(0x0801, 0x0801) AM_WRITE(flyball_pitcher_pic_w)
	AM_RANGE(0x0802, 0x0802) AM_READ(flyball_scanline_r)
	AM_RANGE(0x0803, 0x0803) AM_READ(flyball_potsense_r)
	AM_RANGE(0x0804, 0x0804) AM_WRITE(flyball_ball_vert_w)
	AM_RANGE(0x0805, 0x0805) AM_WRITE(flyball_ball_horz_w)
	AM_RANGE(0x0806, 0x0806) AM_WRITE(flyball_pitcher_vert_w)
	AM_RANGE(0x0807, 0x0807) AM_WRITE(flyball_pitcher_horz_w)
	AM_RANGE(0x0900, 0x0900) AM_WRITE(flyball_potmask_w)
	AM_RANGE(0x0a00, 0x0a07) AM_WRITE(flyball_misc_w)
	AM_RANGE(0x0b00, 0x0b00) AM_READ(flyball_input_r)
	AM_RANGE(0x0d00, 0x0eff) AM_WRITE(MWA8_RAM) AM_BASE(&flyball_playfield_ram)
	AM_RANGE(0x1000, 0x1fff) AM_ROM AM_BASE(&rombase) /* program */
ADDRESS_MAP_END


INPUT_PORTS_START( flyball )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING( 0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING( 0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, "Innings Per Game" )
	PORT_DIPSETTING( 0x00, "1" )
	PORT_DIPSETTING( 0x40, "2" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x80, DEF_STR( On ) )

	PORT_START /* IN1 */
	PORT_BIT( 0x3f, 0x20, IPT_AD_STICK_Y ) PORT_MINMAX(1,63) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START /* IN2 */
	PORT_BIT( 0x3f, 0x20, IPT_AD_STICK_X ) PORT_MINMAX(1,63) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START /* IN3 */
	PORT_BIT( 0x3f, 0x20, IPT_AD_STICK_Y ) PORT_MINMAX(1,63) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START /* IN4 */
	PORT_BIT( 0x3f, 0x20, IPT_AD_STICK_X ) PORT_MINMAX(1,63) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xFE, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xFE, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static const gfx_layout flyball_tiles_layout =
{
	8, 16,    /* width, height */
	128,      /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x80      /* increment */
};

static const gfx_layout flyball_sprites_layout =
{
	16, 16,   /* width, height */
	16,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
		0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0
	},
	0x100     /* increment */
};

static const gfx_decode flyball_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &flyball_tiles_layout, 0, 2 },
	{ REGION_GFX2, 0, &flyball_sprites_layout, 2, 2 },
	{ -1 } /* end of array */
};


PALETTE_INIT( flyball )
{
	palette_set_color(0, 0x3F, 0x3F, 0x3F);  /* tiles, ball */
	palette_set_color(1, 0xFF, 0xFF, 0xFF);
	palette_set_color(2, 0xFF ,0xFF, 0xFF);  /* sprites */
	palette_set_color(3, 0x00, 0x00, 0x00);
}


static MACHINE_DRIVER_START( flyball )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16)
	MDRV_CPU_PROGRAM_MAP(flyball_map, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int) ((22. * 1000000) / (262. * 60) + 0.5))

	MDRV_MACHINE_RESET(flyball)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(flyball_gfx_decode_info)
	MDRV_PALETTE_LENGTH(4)

	MDRV_PALETTE_INIT(flyball)
	MDRV_VIDEO_UPDATE(flyball)
	MDRV_VIDEO_START(flyball)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(flyball_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( flyball )
	ROM_REGION( 0x3000, REGION_CPU1, 0 )                  /* program */
	ROM_LOAD( "6129-02.d5", 0x2000, 0x0200, CRC(105ffe40) SHA1(20225571ccf76df5d96a42168d9223cccdff90a8) )
	ROM_LOAD( "6130-02.f5", 0x2200, 0x0200, CRC(188210e1) SHA1(6d837dd9ea44d16f0d54ea9e14260de5f7c05b6b) )
	ROM_LOAD( "6131-01.h5", 0x2400, 0x0200, CRC(a9c7e858) SHA1(aee4a359d6a5729dc1be5b8ce8fbe54d032d12b0) )
	ROM_LOAD( "6132-01.j5", 0x2600, 0x0200, CRC(31fefd8a) SHA1(97e3ef278ce2175cd33c0f3147bdf7974752c836) )
	ROM_LOAD( "6133-01.k5", 0x2800, 0x0200, CRC(6fdb09b1) SHA1(04ad412b437bb24739b3e31fa5a413e63d5897f8) )
	ROM_LOAD( "6134-01.m5", 0x2A00, 0x0200, CRC(7b526c73) SHA1(e47c8f33b7edc143ab1713556c59b93571933daa) )
	ROM_LOAD( "6135-01.n5", 0x2C00, 0x0200, CRC(b352cb51) SHA1(39b9062fb51d0a78a47dcd470ceae47fcdbd7891) )
	ROM_LOAD( "6136-02.r5", 0x2E00, 0x0200, CRC(ae06a0f5) SHA1(6034176b255eeaa2980e8fef1b17ef6f0a743941) )

	ROM_REGION( 0x0C00, REGION_GFX1, ROMREGION_DISPOSE )   /* tiles */
	ROM_LOAD( "6142.l2", 0x0000, 0x0200, CRC(65650cfa) SHA1(7d17455146fc9def22c7bd06f7fde32df0a0c2bc) )
	ROM_LOAD( "6139.j2", 0x0200, 0x0200, CRC(a5d1358e) SHA1(33cecbe40ae299549a3395e3dffbe7b6021803ba) )
	ROM_LOAD( "6141.m2", 0x0400, 0x0200, CRC(98b5f803) SHA1(c4e323ced2393fa4a9720ff0086c559fb9b3a9f8) )
	ROM_LOAD( "6140.k2", 0x0600, 0x0200, CRC(66aeec61) SHA1(f577bad015fe9e3708fd95d5d2bc438997d14d2c) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )   /* sprites */
	ROM_LOAD16_BYTE( "6137.e2", 0x0000, 0x0200, CRC(68961fda) SHA1(a06c7b453cce04716f49bd65ecfe1ba67cb8681e) )
	ROM_LOAD16_BYTE( "6138.f2", 0x0001, 0x0200, CRC(aab314f6) SHA1(6625c719fdc000d6af94bc9474de8f7e977cee97) )
ROM_END

ROM_START( flyball1 )
	ROM_REGION( 0x3000, REGION_CPU1, 0 )                  /* program */
	ROM_LOAD( "6129.d5", 0x2000, 0x0200, CRC(17eda069) SHA1(e4ef0bf4546cf00668d759a188e0989a4f003825) )
	ROM_LOAD( "6130.f5", 0x2200, 0x0200, CRC(a756955b) SHA1(220b7f1789bba4481d595b36b4bae25f98d3ad8d) )
	ROM_LOAD( "6131.h5", 0x2400, 0x0200, CRC(a9c7e858) SHA1(aee4a359d6a5729dc1be5b8ce8fbe54d032d12b0) )
	ROM_LOAD( "6132.j5", 0x2600, 0x0200, CRC(31fefd8a) SHA1(97e3ef278ce2175cd33c0f3147bdf7974752c836) )
	ROM_LOAD( "6133.k5", 0x2800, 0x0200, CRC(6fdb09b1) SHA1(04ad412b437bb24739b3e31fa5a413e63d5897f8) )
	ROM_LOAD( "6134.m5", 0x2A00, 0x0200, CRC(7b526c73) SHA1(e47c8f33b7edc143ab1713556c59b93571933daa) )
	ROM_LOAD( "6135.n5", 0x2C00, 0x0200, CRC(b352cb51) SHA1(39b9062fb51d0a78a47dcd470ceae47fcdbd7891) )
	ROM_LOAD( "6136.r5", 0x2E00, 0x0200, CRC(1622d890) SHA1(9ad342aefdc02e022eb79d84d1c856bed538bebe) )

	ROM_REGION( 0x0C00, REGION_GFX1, ROMREGION_DISPOSE )   /* tiles */
	ROM_LOAD( "6142.l2", 0x0000, 0x0200, CRC(65650cfa) SHA1(7d17455146fc9def22c7bd06f7fde32df0a0c2bc) )
	ROM_LOAD( "6139.j2", 0x0200, 0x0200, CRC(a5d1358e) SHA1(33cecbe40ae299549a3395e3dffbe7b6021803ba) )
	ROM_LOAD( "6141.m2", 0x0400, 0x0200, CRC(98b5f803) SHA1(c4e323ced2393fa4a9720ff0086c559fb9b3a9f8) )
	ROM_LOAD( "6140.k2", 0x0600, 0x0200, CRC(66aeec61) SHA1(f577bad015fe9e3708fd95d5d2bc438997d14d2c) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )   /* sprites */
	ROM_LOAD16_BYTE( "6137.e2", 0x0000, 0x0200, CRC(68961fda) SHA1(a06c7b453cce04716f49bd65ecfe1ba67cb8681e) )
	ROM_LOAD16_BYTE( "6138.f2", 0x0001, 0x0200, CRC(aab314f6) SHA1(6625c719fdc000d6af94bc9474de8f7e977cee97) )
ROM_END


GAME( 1976, flyball,  0,       flyball, flyball, 0, 0, "Atari", "Flyball (rev 2)", 0 )
GAME( 1976, flyball1, flyball, flyball, flyball, 0, 0, "Atari", "Flyball (rev 1)", 0 )

// license:BSD-3-Clause
// copyright-holders:Hau, Derrick Renaud
/*

--------------------------
Galaxy Force
PCB: SIV-01-A
Runaway
PCB: SIV-01-B
--------------------------
Dai 3 Wakusei
(c)1979 Sun Electronics

PCB: SIV-01-B
TVG_13.6     [8e8b40b1]
TVG_14.7     [d48cbabe]
TVG_15.8     [cf44bd60]
TVG_16.9     [ae723f56]
--------------------------
Warp 1
(c)1979 Sun Electronics

PCB: SIV-01-C
TVG_23.6     [c956b3a8]
TVG_24.7     [6cca323d]
TVG_25.8     [5aa6eb1e]
TVG_26.9     [840c1a75]
--------------------------
Warp 1
PCB: SIV-01-C, Taito logo

Z80 @ 2.5MHz
1KB SRAM (2*MB8114)
16KB(nibbles) DRAM (4*UPD416C)
5*SN76477, dai3wksi has 6
--------------------------

TODO:
- Get rid of those big lookup tables, I see no proms on the PCB so it's hardcoded
  logic somehow? it's 3bpp colors, not b&w + color overlay.
  Note: warp1 colors match the ones of dai3wksi according to flyer and game photos.
  DIP switch for changing cyan to white does not make sense.
  PCB video of warp1 does not red-blink the 'fuel low' message.
- runaways colors are wrong (video is available).
- Two player games are automatically displayed in cocktail mode.
  Is this by design (a cocktail only romset)?
- Discrete audio needs adding to replace hardcoded samples
- Is warp1 sound same as dai3wksi?
- runaways uses less SN76477, it has different sound and should use different samples
  (until it's done via netlist).
- Dips need identifying
- warp1 service mode is started by booting with coin1 held down,
  the service switch can still be tested there but otherwise has no function?
  In warp1bl, it is a service coin input.

*/

#include "driver.h"
#include "sound/samples.h"


/*************************************
 *
 *  Video system
 *
 *************************************/

static UINT8 *dai3wksi_videoram;
static size_t dai3wksi_videoram_size;
static int dai3wksi_flipscreen;
static int dai3wksi_redscreen;
static int dai3wksi_redterop;

static const UINT8 vr_prom1[64*8*2]={
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,

	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
};

static const UINT8 vr_prom2[64*8*2]={
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,6,6,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,

	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	6, 6,6,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
	3, 3,3,2,2,6,6,6,6, 6,6,6,6,6,6,6,6, 3,3,3,3,3,3,3,3, 7,7,7,7,7,7,7,7, 3,3,3,3,3,3,3,3, 2,2,2,2,2,2,2,2, 6,6,6,6,6,6,6,6, 4,4,4,4,4,4,4,
};

static PALETTE_INIT( dai3wksi )
{
	int i;

	/* the colors are blue, red and green bits, in this order */
	for (i = 0; i < 8; i++)
		palette_set_color(i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
}

static void dai3wksi_draw(mame_bitmap *bitmap, int bw)
{
	offs_t offs;

	for (offs = 0; offs < dai3wksi_videoram_size; offs++)
	{
		int i;
		UINT8 x = offs << 2;
		UINT8 y = offs >> 6;
		UINT8 data = dai3wksi_videoram[offs];
		UINT8 color;

		if (bw)
		{
			color = 7;
		}
		else if (dai3wksi_redscreen)
		{
			color = 0x02;
		}
		else
		{
			int value = (x >> 2) + ((y >> 5) << 6) + 64 * 8 * (dai3wksi_redterop ? 1 : 0);

			if (readinputport(2) & 0x03)
				color = vr_prom2[value];
			else
				color = vr_prom1[value];
		}

		for (i = 0; i < 4; i++)
		{
			UINT8 pen = (data & (1 << i)) ? color : 0;

			if (dai3wksi_flipscreen)
				plot_pixel(bitmap, 255-x, 255-y, Machine->pens[pen]);
			else
				plot_pixel(bitmap, x, y, Machine->pens[pen]);

			x++;
		}
	}
}

static VIDEO_UPDATE( dai3wksi )
{
	dai3wksi_draw(bitmap, 0);
}

static VIDEO_UPDATE( warp1bl )
{
	dai3wksi_draw(bitmap, 1);
}


/*************************************
 *
 *  Audio system
 *
 *************************************/

#define SAMPLE_SOUND1		0
#define SAMPLE_SOUND2		1
#define SAMPLE_SOUND3_1		2
#define SAMPLE_SOUND3_2		3
#define SAMPLE_SOUND4		4
#define SAMPLE_SOUND5		5
#define SAMPLE_SOUND6_1		6
#define SAMPLE_SOUND6_2		7

#define CHANNEL_SOUND1		0
#define CHANNEL_SOUND2		1
#define CHANNEL_SOUND3		2
#define CHANNEL_SOUND4		3
#define CHANNEL_SOUND5		4
#define CHANNEL_SOUND6		5

static UINT8 audio_data[3];
static int enabled_sound;
static int sound3_counter;

static MACHINE_RESET( dai3wksi )
{
	audio_data[0] = 0;
	audio_data[1] = 0;
	audio_data[2] = 0;
	enabled_sound = 0;
	sound3_counter = 0;
}

static WRITE8_HANDLER( dai3wksi_audio_1_w )
{
	UINT8 rising_bits = data & ~audio_data[0];

	enabled_sound = data & 0x80;

	if ((rising_bits & 0x20) && enabled_sound)
	{
		if (data & 0x04)
			sample_start(CHANNEL_SOUND5, SAMPLE_SOUND5, 0);
		else
			sample_start(CHANNEL_SOUND5, SAMPLE_SOUND5, 1);
	}
	if (!(data & 0x20) && (audio_data[0] & 0x20))
		sample_stop(CHANNEL_SOUND5);

	audio_data[0] = data;
}

static WRITE8_HANDLER( dai3wksi_audio_2_w )
{
	UINT8 rising_bits = data & ~audio_data[1];

	dai3wksi_flipscreen = data & 0x10;
	dai3wksi_redscreen  = ~data & 0x20;
	dai3wksi_redterop   = data & 0x40;

	if (enabled_sound)
	{
		if (rising_bits & 0x01) sample_start(CHANNEL_SOUND1, SAMPLE_SOUND1, 0);
		if (rising_bits & 0x02) sample_start(CHANNEL_SOUND2, SAMPLE_SOUND2, 0);
		if (rising_bits & 0x08) sample_start(CHANNEL_SOUND4, SAMPLE_SOUND4, 0);
		if (rising_bits & 0x04)
		{
			if (!sound3_counter)
				sample_start(CHANNEL_SOUND3, SAMPLE_SOUND3_1, 0);
			else
				sample_start(CHANNEL_SOUND3, SAMPLE_SOUND3_2, 0);

			sound3_counter ^= 1;
		}
	}

	audio_data[1] = data;
}

static WRITE8_HANDLER( dai3wksi_audio_3_w )
{
	if (enabled_sound)
	{
		if (data & 0x40)
			sample_start(CHANNEL_SOUND6, SAMPLE_SOUND6_1, 0);
		else if (data & 0x80)
			sample_start(CHANNEL_SOUND6, SAMPLE_SOUND6_2, 0);
	}

	audio_data[2] = data;
}

static const char *dai3wksi_sample_names[] =
{
	"*dai3wksi",
	"1.wav",
	"2.wav",
	"3.wav",
	"3-2.wav",
	"4.wav",
	"5.wav",
	"6.wav",
	"6-2.wav",
	0
};

static struct Samplesinterface dai3wksi_samples_interface =
{
	6,	/* 6 channels */
	dai3wksi_sample_names
};


/*************************************
 *
 *  Memory handlers
 *
 *************************************/

/* the protection of warp1 echoes the first bit of the audio latch */
static READ8_HANDLER( warp1_in0_r )
{
	UINT8 data = readinputport(0) & ~0x10;

	if (!(audio_data[0] & 1))
		data |= 0x10;

	return data;
}

static ADDRESS_MAP_START( main_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2400, 0x25ff) AM_READ(input_port_0_r)
	AM_RANGE(0x2800, 0x29ff) AM_READ(input_port_1_r)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1bff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(dai3wksi_audio_1_w)
	AM_RANGE(0x3400, 0x3400) AM_WRITE(dai3wksi_audio_2_w)
	AM_RANGE(0x3800, 0x3800) AM_WRITE(dai3wksi_audio_3_w)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_RAM) AM_BASE(&dai3wksi_videoram) AM_SIZE(&dai3wksi_videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( warp1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2400, 0x25ff) AM_READ(warp1_in0_r)
	AM_RANGE(0x2800, 0x29ff) AM_READ(input_port_1_r)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END


/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( dai3wksi )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* hold down at boot for service mode */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x10, 0x00, "DIPSW #7" )						PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW #8" )						PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x01, 0x00, "DIPSW #1" )						PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW #2" )						PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( warp1 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* protection, see warp1_in0_r */
	PORT_DIPNAME( 0x20, 0x20, "High Score Table" )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/* the bootleg seems to expect active low */
INPUT_PORTS_START( warp1bl )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x20, "High Score Table" )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/* only dips 7 and 8 tested in service mode, but 1 8 dip bank on PCB */
INPUT_PORTS_START( runaways )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* hold down at boot for service mode */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x10, 0x00, "DIPSW #7" )						PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )				PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( dai3wksi )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 10000000/4)
	MDRV_CPU_PROGRAM_MAP(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_MACHINE_RESET(dai3wksi)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 8, 247)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(dai3wksi)
	MDRV_VIDEO_UPDATE(dai3wksi)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(dai3wksi_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( warp1 )

	MDRV_IMPORT_FROM(dai3wksi)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(warp1_readmem,main_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( warp1bl )

	MDRV_IMPORT_FROM(dai3wksi)

	/* the bootleg is in b&w */
	MDRV_VIDEO_UPDATE(warp1bl)
MACHINE_DRIVER_END


/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( dai3wksi )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tvg_13.6",  0x0000, 0x0800, CRC(8e8b40b1) SHA1(25b9223486dd348ea302e8e8f1d47c804a88b142) )
	ROM_LOAD( "tvg_14.7",  0x0800, 0x0800, CRC(d48cbabe) SHA1(64b571cd778fc7d67a5fa998a0defd36c04f111f) )
	ROM_LOAD( "tvg_15.8",  0x1000, 0x0800, CRC(cf44bd60) SHA1(61e0b3f9c4a1f9da1de57fb8276d4fc9e43b8f24) )
	ROM_LOAD( "tvg_16.9",  0x1800, 0x0400, CRC(ae723f56) SHA1(c25c27d6144533b2b2a888bfa8dbf48ed8d8b09a) )
ROM_END

ROM_START( warp1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tvg_23.6",  0x0000, 0x0800, CRC(c956b3a8) SHA1(1e90d3480ac8ebb26eed5800a453b2b9484a8b05) )
	ROM_LOAD( "tvg_24.7",  0x0800, 0x0800, CRC(6cca323d) SHA1(e5dc4f09990c3f9e408e25e189a9355d7c021128) )
	ROM_LOAD( "tvg_25.8",  0x1000, 0x0800, CRC(5aa6eb1e) SHA1(ccf18d9618635c38747a57dad2214e6ca14c835d) )
	ROM_LOAD( "tvg_26.9",  0x1800, 0x0400, CRC(840c1a75) SHA1(63853e560c0fba5600a8ad0408cc9e7a55db6d93) )
ROM_END

ROM_START( warp1t )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tvg_23.6",  0x0000, 0x0800, CRC(2025a316) SHA1(704798ec1d0039cb977396330801a625c8b02cff) )
	ROM_LOAD( "tvg_24.7",  0x0800, 0x0800, CRC(6cca323d) SHA1(e5dc4f09990c3f9e408e25e189a9355d7c021128) )
	ROM_LOAD( "tvg_25.8",  0x1000, 0x0800, CRC(5aa6eb1e) SHA1(ccf18d9618635c38747a57dad2214e6ca14c835d) )
	ROM_LOAD( "tvg_26.9",  0x1800, 0x0400, CRC(6bf25327) SHA1(ee9f635d70a97628ab82efa12efdb39349b52e2b) )
ROM_END

ROM_START( warp1bl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "0",  0x0000, 0x0400, CRC(d4383a9a) SHA1(eda0b388bd0adf22059f0848ce91fee889700f0c) )
	ROM_LOAD( "1",  0x0400, 0x0400, CRC(6005eddc) SHA1(a06dbdbe3356eb011dbce756e6c76adef4c50beb) )
	ROM_LOAD( "2",  0x0800, 0x0400, CRC(76ea7ebc) SHA1(60eb870f30aaf95589c7326ca6664974e39f3e27) )
	ROM_LOAD( "3",  0x0c00, 0x0400, CRC(0c34ef33) SHA1(513fe5a875746d94d2ea0b7eed81ca0cf869c457) )
	ROM_LOAD( "4",  0x1000, 0x0400, CRC(41ffad36) SHA1(479af4506f3637a95d2d41d04e1d66debb28da03) )
	ROM_LOAD( "5",  0x1400, 0x0400, CRC(f67a1e1d) SHA1(aeb7b018889de4bd32ee8f65d8c0c32bbee07440) )
	ROM_LOAD( "6",  0x1800, 0x0400, CRC(44e9327f) SHA1(aa217b0fbbef6a408231dc0dfedd94167c351c28) )
ROM_END

ROM_START( runaways )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "gf10",  0x0000, 0x0800, CRC(3d8ace0a) SHA1(e1f1393724e2f66d4ce9269dd982fdbfa96f8b6f) )
	ROM_LOAD( "gf11",  0x0800, 0x0800, CRC(a67b889f) SHA1(bfe1ebbcf2fb14b7557343229eaa0a42af70a3ad) )
	ROM_LOAD( "gf12",  0x1000, 0x0800, CRC(10350c4c) SHA1(32a505180d3212eb4c7abbe1459b3a3b6a55fe20) )
ROM_END


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1979, runaways, 0,     dai3wksi, runaways, 0, ROT270, "Sun Electronics", "Runaway (Sun Electronics, Japan)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

GAME( 1979, dai3wksi, 0,     dai3wksi, dai3wksi, 0, ROT270, "Sun Electronics", "Dai 3 Wakusei (Japan)", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )

GAME( 1979, warp1,    0,     warp1,    warp1,    0, ROT270, "Sun Electronics", "Warp-1 (Japan)", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1979, warp1t,   warp1, warp1,    warp1,    0, ROT90,  "Sun Electronics (Taito license)", "Warp-1 (Japan, Taito license)", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1979, warp1bl,  warp1, warp1bl,  warp1bl,  0, ROT270, "bootleg (Igleck)", "Warp-1 (Japan, bootleg)", GAME_IMPERFECT_SOUND )

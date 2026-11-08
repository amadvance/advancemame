// license:BSD-3-Clause
// copyright-holders:Barry Rodewald
/* Rotary Fighter

driver by Barry Rodewald
 based on Initial work by David Haywood

 todo:

 verify game speed if possible (related to # of interrupts)

*/

#include "driver.h"
#include "8080bw.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"
#include "sound/samples.h"
#include "sound/sn76477.h"


static UINT8 sound_last;
static int rotaryf_flip;

static const char *rotaryf_sample_names[] = {
	"*invaders", "1.wav", "2.wav", "3.wav", "4.wav", "5.wav",
	"6.wav", "7.wav", "8.wav", "9.wav", 0
};

static struct Samplesinterface rotaryf_samples_interface = {
	6, rotaryf_sample_names
};

static READ8_HANDLER( rotaryf_controls_r )
{
	UINT8 data = readinputport(2);
	if (rotaryf_flip)
		return data;
	return (data & 0xcd) | ((data & 0x01) << 1) | ((data & 0x0c) << 2);
}

static WRITE8_HANDLER( rotaryf_sound_w )
{
	UINT8 rising = data & ~sound_last;
	if (rising & 0x01) sample_start(3, 7, 0);
	if (rising & 0x04) sample_start(5, 8, 0);
	if (rising & 0x20) sample_start(1, 1, 0);
	if (rising & 0x40) sample_start(2, 2, 0);
	if (rising & 0x80) sample_start(0, 0, 0);
	if (rising & 0x10) sample_start(4, (rising & 0x08) ? 3 : 6, 0);
	SN76477_enable_w(0, (data & 3) ? 1 : 0);
	sound_last = data;
}

static WRITE8_HANDLER( rotaryf_coin_w )
{
	coin_counter_w(0, data & 2);
}

static WRITE8_HANDLER( rotaryf_flip_w )
{
	rotaryf_flip = (data & 1) && (readinputport(4) & 1);
}

static const ppi8255_interface rotaryf_ppi = {
	1, { 0 }, { rotaryf_controls_r }, { 0 },
	{ rotaryf_sound_w }, { 0 }, { rotaryf_coin_w }
};

static MACHINE_RESET( rotaryf )
{
	ppi8255_init(&rotaryf_ppi);
	sound_last = 0xff;
	rotaryf_flip = 0;
	SN76477_vco_w(0, 1);
	SN76477_mixer_w(0, 0);
	SN76477_envelope_w(0, 1);
	SN76477_enable_w(0, 1);
}

static VIDEO_UPDATE( rotaryf )
{
	int offset, bit;
	for (offset = 0; offset < videoram_size; offset++)
	{
		int x = (offset << 3) & 0xff;
		int y = offset >> 5;
		UINT8 data = videoram[offset];
		for (bit = 0; bit < 8; bit++)
		{
			int px = rotaryf_flip ? 247 - (x | bit) : (x | bit);
			int py = rotaryf_flip ? 255 - y : y;
			if (px >= cliprect->min_x && px <= cliprect->max_x && py >= cliprect->min_y && py <= cliprect->max_y)
				plot_pixel(bitmap, px, py, Machine->pens[(data >> bit) & 1]);
		}
	}
}

static PALETTE_INIT( rotaryf )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0xff,0xff,0xff); /* white */
}

static INTERRUPT_GEN( rotaryf_interrupt )
{
	int line = 261 - cpu_getiloops();
	if (line == 256)
		cpunum_set_input_line(0, I8085_RST55_LINE, HOLD_LINE);
	else if ((line % 64) == 0 && line < 256)
	{
		cpunum_set_input_line(0, I8085_RST75_LINE, ASSERT_LINE);
		cpunum_set_input_line(0, I8085_RST75_LINE, CLEAR_LINE);
	}
}

static ADDRESS_MAP_START( rotaryf_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_MIRROR(0x4000) AM_ROM
	AM_RANGE(0x7000, 0x73ff) AM_MIRROR(0x0c00) AM_RAM
	AM_RANGE(0x8000, 0x9fff) AM_MIRROR(0x4000) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xa000, 0xa1ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rotaryf_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x02, 0x02) AM_WRITENOP
	AM_RANGE(0x04, 0x04) AM_WRITENOP
	AM_RANGE(0x07, 0x07) AM_WRITENOP
	AM_RANGE(0x20, 0x20) AM_WRITENOP
	AM_RANGE(0x21, 0x21) AM_READWRITE(input_port_1_r, MWA8_NOP)
	AM_RANGE(0x26, 0x26) AM_READ(input_port_3_r)
	AM_RANGE(0x28, 0x2b) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x30, 0x30) AM_WRITE(rotaryf_flip_w)
ADDRESS_MAP_END

INPUT_PORTS_START( rotaryf )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x81, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x81, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING( 0x04, "1000" )
	PORT_DIPSETTING( 0x00, "1500" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x40, DEF_STR( On ) )
//  PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
//  PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
//  PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)

	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


static MACHINE_DRIVER_START( rotaryf )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",8085A,2000000)
	MDRV_CPU_PROGRAM_MAP(rotaryf_map, 0)
	MDRV_CPU_IO_MAP(rotaryf_io, 0)
	MDRV_CPU_VBLANK_INT(rotaryf_interrupt,262)
	MDRV_MACHINE_RESET(rotaryf)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 30*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(rotaryf)
	MDRV_VIDEO_UPDATE(rotaryf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(rotaryf_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invaders_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END

ROM_START( rotaryf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "krf-1.bin",            0x0000, 0x0400, CRC(f7b2d3e6) SHA1(be7afc1a14be60cb895fc4180167353c7156fc4c) )
	ROM_LOAD( "krf-2.bin",			  0x0400, 0x0400, CRC(be9f047a) SHA1(e5dd2b5b4fda7f178e7f1137592ba49fbc9cc82e) )
	ROM_LOAD( "krf-3.bin",            0x0800, 0x0400, CRC(c7629eb6) SHA1(03aae964783ce4b1de77737e83fd2094483fbda4) )
	ROM_LOAD( "krf-4.bin",            0x0c00, 0x0400, CRC(b4703093) SHA1(9239d6da818049bc98a631c3bf5b962b5df5b2ea) )
	ROM_LOAD( "krf-5.bin",            0x1000, 0x0400, CRC(ae233f07) SHA1(a7bbd2ee4477ee041d170e2fc4e94c99c3b564fc) )
	ROM_LOAD( "krf-6.bin",            0x1400, 0x0400, CRC(e28b3713) SHA1(428f73891125f80c722357f1029b18fa9416bcfd) )
ROM_END

GAME( 1979, rotaryf, 0, rotaryf, rotaryf, 0, ROT270, "Kasco", "Rotary Fighter", GAME_IMPERFECT_SOUND )

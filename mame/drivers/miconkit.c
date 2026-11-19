// license:BSD-3-Clause
// copyright-holders:hap
/***************************************************************************

SNK Micon-Kit series

Games on this hardware:
- Micon-Kit
- Micon-Kit Part II (aka Yamato)
- Space Micon Kit

The cocktail cabinets were called Table Micon, and the upright version of the
first game was apparently called Micon-Block.

Micon-Kit was SNK's first arcade game, it's a simple Breakout clone. The sequel
adds moving obstacles. The 3rd game in the series, Space Micon Kit, adds a 2nd
row of bricks.

Hardware notes:

Micon-Kit (1/2):
- 2 PCBs: one with CPU and video hardware, one with RAM, ROM, PPI
- NEC D8080A-C, 18.432MHz XTAL, NEC uPB8224C (/9 divider)
- NEC uPB8228C, NEC D8255C
- 4KB ROM (4*MB8518), 256 bytes RAM (2*D2111AL-4)
- 4KB VRAM (32*MB8102), 1bpp video with color overlay
- video timing: 10MHz XTAL, h/v: ?
- beeper

Space Micon Kit:
- PCB label: OKAYA ELECTRIC IND.CO.,LTD., SHIN NIHON KIKAKU
- 5KB ROM (room for 8KB), 1KB RAM (2*TMM314APL)
- 4KB VRAM (8*TMM314APL)
- same CPU/divider/PPI but Mitsubishi brand, rest is same as above

***************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "streams.h"
#include "sound/custom.h"

static UINT8 input_select;

static PALETTE_INIT( miconkit )
{
	palette_set_color(0, 0x00, 0x00, 0x00);
	palette_set_color(1, 0xff, 0xff, 0xff);
}

static WRITE8_HANDLER( vram_w )
{
	int x = (offset & 0x1f) << 3;
	int y = (offset >> 5) << 1;
	int i;

	videoram[offset] = data;

	/* every VRAM row is displayed on two scanlines */
	for (i = 0; i < 8; ++i)
	{
		pen_t pen = Machine->pens[(data >> i) & 1];
		plot_pixel(tmpbitmap, x + i, y, pen);
		plot_pixel(tmpbitmap, x + i, y + 1, pen);
	}
}

/* square wave beeper */
static sound_stream *beeper_stream;
static int beeper_freq;
static int beeper_pos;

static void beeper_update(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	stream_sample_t *buffer = _buffer[0];
	int i;

	for (i = 0; i < length; ++i)
	{
		if (beeper_freq)
		{
			beeper_pos += beeper_freq;
			if (beeper_pos >= options.samplerate)
				beeper_pos -= options.samplerate;
			buffer[i] = beeper_pos < options.samplerate / 2 ? 16000 : -16000;
		}
		else
			buffer[i] = 0;
	}
}

static void *beeper_start(int clock, const struct CustomSound_interface *config)
{
	beeper_stream = stream_create(0, 1, options.samplerate, 0, beeper_update);
	beeper_freq = 0;
	beeper_pos = 0;
	return (void *)~0;
}

static struct CustomSound_interface beeper_interface =
{
	beeper_start
};

static READ8_HANDLER( paddle_r )
{
	return readinputport(2 + input_select);
}

static WRITE8_HANDLER( sound_w )
{
	/* d0-d3: beeper pitch (0 is off) */
	stream_update(beeper_stream, 0);
	beeper_freq = 248 * (data & 0xf);
}

static WRITE8_HANDLER( select_w )
{
	/* d0: input input_select */
	input_select = data & 1;
}

static READ8_HANDLER( vblank_r )
{
	/* d6: vblank flag */
	return cpu_getvblank() ? 0xbf : 0xff;
}

static READ8_HANDLER( input_r )
{
	/* d0: serve button, other: misc inputs */
	return (readinputport(input_select) & 1) | (readinputport(4) & 0xfe);
}

static ppi8255_interface ppi8255_intf =
{
	1,
	{ paddle_r },
	{ 0 },
	{ vblank_r },
	{ 0 },
	{ sound_w },
	{ select_w },
};

static MACHINE_RESET( miconkit )
{
	ppi8255_init(&ppi8255_intf);
	input_select = 0;
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x4000, 0x40ff) AM_RAM
	AM_RANGE(0x7000, 0x7fff) AM_READWRITE(MRA8_RAM, vram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x03) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( smiconk_main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0x7000, 0x7fff) AM_READWRITE(MRA8_RAM, vram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( smiconk_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x30, 0x33) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x34, 0x34) AM_READ(input_r)
ADDRESS_MAP_END

INPUT_PORTS_START( micon2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG("IN2")
	PORT_BIT( 0x7f, 0x38, IPT_PADDLE ) PORT_MINMAX(0x00,0x70) PORT_SENSITIVITY(40) PORT_KEYDELTA(8) PORT_CENTERDELTA(0) PORT_REVERSE

	PORT_START_TAG("IN3")
	PORT_BIT( 0x7f, 0x38, IPT_PADDLE ) PORT_MINMAX(0x00,0x70) PORT_SENSITIVITY(40) PORT_KEYDELTA(8) PORT_CENTERDELTA(0) PORT_COCKTAIL

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x30, 0x10, "Replay" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x10, "400" )
	PORT_DIPSETTING(    0x20, "500" )
	PORT_DIPSETTING(    0x30, "600" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x80, "5" )
INPUT_PORTS_END

INPUT_PORTS_START( smiconk )
	PORT_INCLUDE( micon2 )

	PORT_MODIFY("IN4")
	PORT_DIPNAME( 0x30, 0x10, "Replay" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x10, "800" )
	PORT_DIPSETTING(    0x20, "1000" )
	PORT_DIPSETTING(    0x30, "1200" )
INPUT_PORTS_END

static MACHINE_DRIVER_START( micon2 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8080, 18432000/9)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(io_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)

	MDRV_MACHINE_RESET(miconkit)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 240-1, 24, 256-24-1)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(miconkit)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(beeper_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( smiconk )
	MDRV_IMPORT_FROM(micon2)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(smiconk_main_map, 0)
	MDRV_CPU_IO_MAP(smiconk_io_map, 0)
MACHINE_DRIVER_END

ROM_START( micon2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ufo_no_0", 0x0000, 0x0400, CRC(3eb5a299) SHA1(5e7de4cb8312be8b84f7e5e035b61a6cb9798bc0) )
	ROM_LOAD( "ufo_no_1", 0x0400, 0x0400, CRC(e796338e) SHA1(86c5f283b4a41e19dd0b624d04e1a62ff2ffbf58) )
	ROM_LOAD( "ufo_no_2", 0x0800, 0x0400, CRC(bf246cd7) SHA1(147fb9b877ee108c9c09461ae7e0d72af9ab3275) )
	ROM_LOAD( "ufo_no_3", 0x0c00, 0x0400, CRC(0e93b4f0) SHA1(9405e85a7e005edd0043cb43ce2ef283b4c1b341) )
ROM_END

ROM_START( smiconk )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "1.1a", 0x0000, 0x0400, CRC(927dfbb8) SHA1(a73e7352ddcc3409701a9037ac32868c1a2592bd) )
	ROM_LOAD( "2.1b", 0x0400, 0x0400, CRC(1d0f1474) SHA1(2ead9a60ef1770f8704fff951caba31ad06a38c7) )
	ROM_LOAD( "3.1c", 0x0800, 0x0400, CRC(b89cb388) SHA1(6ec6af0c4e809e8b0b9544eb7007a3e483f921f2) )
	ROM_LOAD( "4.1d", 0x0c00, 0x0400, CRC(a5897e6e) SHA1(5ecc65bfbecb6e1b10d272693309be827fdb3ef7) )
	ROM_LOAD( "5.1e", 0x1000, 0x0400, CRC(98db7810) SHA1(32d6b828bb22145ae1b4e70d6f69baba1d5aad6b) )
ROM_END

GAME( 1978, micon2,  0, micon2,  micon2,  0, ROT90, "SNK", "Micon-Kit Part II", 0 )
GAME( 1978, smiconk, 0, smiconk, smiconk, 0, ROT90, "SNK", "Space Micon Kit", 0 )

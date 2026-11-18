// license:BSD-3-Clause
// copyright-holders:Phil Stroffolino
/***************************************************************************

  IMOLA GP by RB Bologna (Alberici?)

  Hardware: two Z80s, AY-3-8910 and Intel 8255.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"
#include "state.h"

static UINT8 imola_videoram[2][0x4000];
static UINT8 imola_comms_latch[2];
static UINT8 imola_vcontrol;
static UINT8 imola_vreg[0x10];
static UINT8 imola_scroll;
static UINT8 imola_steerlatch;
static UINT8 imola_draw_mode;
static mame_timer *imola_steer_timer;

static PALETTE_INIT( imolagp )
{
	int i;

	for (i = 0; i < 8; i++)
	{
		int r = (i & 4) ? 255 : 0;
		int g = (i & 2) ? 255 : 0;
		int b = (i & 1) ? 255 : 0;

		palette_set_color(i * 4 + 0, 0, 0, 0);
		palette_set_color(i * 4 + 1, r / 2, g / 2, b / 2);
		palette_set_color(i * 4 + 2, 0, 0, 0);
		palette_set_color(i * 4 + 3, r, g, b);
	}
}

static VIDEO_UPDATE( imolagp )
{
	int y, i, pixel;

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = bitmap->line[y];
		const UINT8 *background = &imola_videoram[0][y * 0x40];
		const UINT8 *sprites = &imola_videoram[1][y * 0x40];

		for (i = 0; i < 0x40; i++)
		{
			UINT8 data = sprites[i] ? sprites[i] : background[i];
			int color = (data & 0xf0) >> 3;
			int x = i * 4 - (imola_scroll ^ 3);

			for (pixel = 0; pixel < 4; pixel++)
			{
				int column = (x + pixel) & 0xff;
				if (column >= cliprect->min_x && column <= cliprect->max_x)
					dest[column] = color | ((data >> pixel) & 1);
			}
		}
	}
}

static void imola_steer_tick(int param)
{
	int steer = readinputportbytag("STEER");
	int period = 20000;

	if (steer & 0x7f)
	{
		if (!(steer & 0x80))
		{
			steer = -steer;
			imola_steerlatch = (imola_steerlatch << 1) | ((~imola_steerlatch & 2) >> 1);
		}

		period = 6500 + 100000 / (steer & 0x7f);
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}

	timer_adjust(imola_steer_timer, TIME_IN_USEC(period), 0, 0);
}

static VIDEO_EOF( imolagp )
{
	imola_scroll = imola_vreg[0xe];
}

static INTERRUPT_GEN( imolagp_vblank )
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

static WRITE8_HANDLER( transmit_data_w )
{
	imola_comms_latch[offset] = data;
}

static READ8_HANDLER( receive_data_r )
{
	return imola_comms_latch[offset];
}

static READ8_HANDLER( trigger_slave_nmi_r )
{
	/* Give the receiver time to read the latch before the master sends the next word. */
	cpu_boost_interleave(0, TIME_IN_USEC(100));
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	return 0;
}

static WRITE8_HANDLER( vreg_control_w )
{
	imola_vcontrol = data & 0xf;
}

static READ8_HANDLER( vreg_data_r )
{
	return 0;
}

static WRITE8_HANDLER( vreg_data_w )
{
	imola_vreg[imola_vcontrol] = data;
}

static WRITE8_HANDLER( screenram_w )
{
	int layer = (imola_draw_mode & 0x81) != 0;

	if (imola_videoram[layer][offset] != data)
	{
		/* Preserve the pixels the raster has already scanned before changing bitmap RAM. */
		force_partial_update(cpu_getscanline() - 1);
		imola_videoram[layer][offset] = data;
	}
}

static READ8_HANDLER( draw_mode_r )
{
	imola_draw_mode = offset;
	return 0;
}

static UINT32 steerlatch_r(void *param)
{
	return imola_steerlatch & 0xf;
}

static const ppi8255_interface imolagp_ppi =
{
	1,
	{ input_port_2_r },
	{ 0 },
	{ input_port_3_r },
	{ 0 },
	{ 0 },
	{ 0 }
};

static ADDRESS_MAP_START( imolagp_master_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x2800, 0x2803) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(vreg_control_w)
	AM_RANGE(0x37f0, 0x37f0) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x3800, 0x3800) AM_READWRITE(vreg_data_r, vreg_data_w)
	AM_RANGE(0x3810, 0x3810) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x4000, 0x4000) AM_READ(input_port_0_r)
	AM_RANGE(0x47ff, 0x4800) AM_WRITE(transmit_data_w)
	AM_RANGE(0x5000, 0x50ff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x5800, 0x5800) AM_READ(input_port_0_r)
	AM_RANGE(0x6000, 0x6000) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( imolagp_master_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0x00) AM_READ(trigger_slave_nmi_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( imolagp_slave_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0x9fff, 0xa000) AM_READ(receive_data_r)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(screenram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( imolagp_slave_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0xff) AM_READ(draw_mode_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( imolagp )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Test ) )
	PORT_DIPSETTING(    0x00, DEF_STR(Off) )
	PORT_DIPSETTING(    0x20, "TEST A" )
	PORT_DIPSETTING(    0x40, "TEST C" )
	PORT_DIPSETTING(    0x60, "TEST D" )
	PORT_DIPSETTING(    0x80, "Memory" )
	PORT_DIPSETTING(    0xa0, "Color Test" )
	PORT_DIPSETTING(    0xc0, "Grid Test" )
	PORT_DIPSETTING(    0xe0, DEF_STR( Unused ) )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(steerlatch_r, 0)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("STEER")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x01, 0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(24)
INPUT_PORTS_END


static INPUT_PORTS_START( imolagpo )
	PORT_INCLUDE( imolagp )

	PORT_MODIFY("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("IN1")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(steerlatch_r, 0)
INPUT_PORTS_END

static MACHINE_START( imolagp )
{
	ppi8255_init(&imolagp_ppi);
	imola_steer_timer = timer_alloc(imola_steer_tick);

	memset(imola_videoram, 0, sizeof(imola_videoram));
	memset(imola_comms_latch, 0, sizeof(imola_comms_latch));
	memset(imola_vreg, 0, sizeof(imola_vreg));
	imola_vcontrol = 0;
	imola_scroll = 0;
	imola_draw_mode = 0;

	state_save_register_global_2d_array(imola_videoram);
	state_save_register_global_array(imola_comms_latch);
	state_save_register_global_array(imola_vreg);
	state_save_register_global(imola_vcontrol);
	state_save_register_global(imola_scroll);
	state_save_register_global(imola_steerlatch);
	state_save_register_global(imola_draw_mode);

	return 0;
}

static MACHINE_RESET( imolagp )
{
	imola_steerlatch = 0;
	timer_adjust(imola_steer_timer, TIME_IN_MSEC(20), 0, 0);
}

static MACHINE_DRIVER_START( imolagp )
	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(imolagp_master_map, 0)
	MDRV_CPU_IO_MAP(imolagp_master_io, 0)
	MDRV_CPU_VBLANK_INT(imolagp_vblank, 1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(imolagp_slave_map, 0)
	MDRV_CPU_IO_MAP(imolagp_slave_io, 0)

	MDRV_MACHINE_START(imolagp)
	MDRV_MACHINE_RESET(imolagp)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(100)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(48, 255, 16, 255)
	MDRV_ASPECT_RATIO(13, 12)
	MDRV_PALETTE_LENGTH(0x20)
	MDRV_PALETTE_INIT(imolagp)
	MDRV_VIDEO_UPDATE(imolagp)
	MDRV_VIDEO_EOF(imolagp)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)
MACHINE_DRIVER_END

ROM_START( imolagp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "03.bin",   0x0000, 0x0800, CRC(a8be1c83) SHA1(e3147515f9f44e435192db838ffbb5c592f6e8d7) )
	ROM_LOAD( "01.bin",   0x0800, 0x0800, CRC(aa6fc1ea) SHA1(3f9c559aaba7b00ffd0210c6977dd4f966451a4b) )
	ROM_LOAD( "04.bin",   0x1000, 0x0800, CRC(c45c459c) SHA1(e51bbfe79bcd66d80b9179067611ea2029c9fd7a) )
	ROM_LOAD( "02.bin",   0x1800, 0x0800, CRC(a80e193b) SHA1(b31bf30dfe1bc498a4324719e4a6656fb94b8d96) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "12.bin",   0x0000, 0x0800, CRC(f9658100) SHA1(00fe32ef6b7cd909e8b69f0f8431c78591318aff) )
	ROM_LOAD( "08.bin",   0x0800, 0x0800, CRC(3a23a90e) SHA1(3a9ce5717147f2cf8c58432dd5ddcf70c2a041aa) )
	ROM_LOAD( "11.bin",   0x1000, 0x0800, CRC(5252e22e) SHA1(bb032af93f5a027235b35467c8a2c2c6fe6d1461) )
	ROM_LOAD( "07.bin",   0x1800, 0x0800, CRC(ce3459ff) SHA1(e336f9411cff71d85cdcc30af7405eca02c8c8f8) )
	ROM_LOAD( "10.bin",   0x2000, 0x0800, CRC(a043ded9) SHA1(0e3b53897da98ef3953622f8bd7dc379916ac0c0) )
	ROM_LOAD( "06.bin",   0x2800, 0x0800, CRC(3ff5997d) SHA1(95ac9dbda782b94b9b2dc4c9baea86113968077f) )
	ROM_LOAD( "09.bin",   0x3000, 0x0800, CRC(b5eb210e) SHA1(81340b1797f2401fbf0485091bf3b309c153476a) )
	ROM_LOAD( "05.bin",   0x3800, 0x0800, CRC(f59e426e) SHA1(ec9bdeed74e2450acb7f00abd13cb5ceb3205016) )
ROM_END

ROM_START( imolagpo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "yd.bin",   0x0000, 0x0800, CRC(5eb61bb7) SHA1(b897ecc7fa9aa1ae4e095d22d16a901b9d439a8e) )
	ROM_LOAD( "yc.bin",   0x0800, 0x0800, CRC(f7468a3b) SHA1(af1664e30b732b3d5321e76659961af3ebeb1237) )
	ROM_LOAD( "yb.bin",   0x1000, 0x0800, CRC(9f21506e) SHA1(6b46ff4815b8a02b190ec13e067f9a6687980774) )
	ROM_LOAD( "ya.bin",   0x1800, 0x0800, CRC(23fbcf14) SHA1(e8b5a9b01f715356c14aa41dbc9ca26732d3a4e4) )

	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF )
	ROM_LOAD( "xx.bin",   0x0000, 0x0400, CRC(059d6294) SHA1(38f075753e7a9fcabb857e5587e8a5966052cbcd) )
	ROM_LOAD( "xm.bin",   0x0800, 0x0400, CRC(64ebb7de) SHA1(fc5477bbedf44e93a578a71d2ff376f6f0b51a71) )
	ROM_LOAD( "xc.bin",   0x1000, 0x0400, CRC(397fd1f3) SHA1(e6b927933847ddcdbbcbeb5e5f37fea063356b24) )
	ROM_LOAD( "xi.bin",   0x1c00, 0x0400, CRC(ef54efa2) SHA1(c8464f11ccfd9eaf9aefb2cd3ac2b9e8bc2d11b6) )
	ROM_LOAD( "xy.bin",   0x2000, 0x0400, CRC(fea8e31e) SHA1(f85eac74d32ebd28170b466b136faf21a8ab220f) )
	ROM_LOAD( "xd.bin",   0x2400, 0x0400, CRC(0c601fc9) SHA1(e655f292b502a14068f5c35428001f8ceedf3637) )
	ROM_LOAD( "xs.bin",   0x2800, 0x0400, CRC(5d15ac52) SHA1(b4f97854018f72e4086c7d830d1b312aea1420a7) )
	ROM_LOAD( "xa.bin",   0x2c00, 0x0400, CRC(a95f5461) SHA1(2645fb93bc4ad5354eef5a385fa94021fb7291dc) )
	ROM_LOAD( "xp.bin",   0x3000, 0x0400, CRC(4b6d63ef) SHA1(16f9e31e588b989f5259ab59c0a3a2c7787f3a16) )
	ROM_LOAD( "xo.bin",   0x3400, 0x0400, CRC(c1d7f67c) SHA1(2ddfe9e59e323cd041fd760531b9e15ccd050058) )
	ROM_LOAD( "xr.bin",   0x3800, 0x0400, CRC(8a8667aa) SHA1(53f34b6c5327d4398de644d7f318d460da56c2de) )
	ROM_LOAD( "xe.bin",   0x3c00, 0x0400, CRC(e0e81120) SHA1(14a77dfd069be342df4dbb1b747443c6d121d3fe) )
ROM_END

GAME(1983?, imolagp,  0,       imolagp, imolagp,  0, ROT90, "RB Bologna", "Imola Grand Prix (set 1)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_SUPPORTS_SAVE)
GAME(1983?, imolagpo, imolagp, imolagp, imolagpo, 0, ROT90, "RB Bologna", "Imola Grand Prix (set 2)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_SUPPORTS_SAVE)

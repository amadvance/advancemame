// license:BSD-3-Clause
// copyright-holders:hap
/***************************************************************************

	Car Jamboree
	Omori Electric CAD (OEC) 1983

	TODO:
	- verify colors against the board;
	- the first two title letters appear misaligned with the tilemap;
	- determine why sprite RAM contains two identical lists.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"

#define MASTER_CLOCK 18432000

extern UINT8 *carjmbre_videoram, *carjmbre_spriteram;
WRITE8_HANDLER( carjmbre_bgcolor_w );
WRITE8_HANDLER( carjmbre_videoram_w );
PALETTE_INIT( carjmbre );
VIDEO_START( carjmbre );
VIDEO_UPDATE( carjmbre );

static UINT8 carjmbre_outlatch;

static WRITE8_HANDLER( carjmbre_outlatch_w )
{
	UINT8 mask = 1 << (offset & 7);
	UINT8 value = (carjmbre_outlatch & ~mask) | ((data & 1) ? mask : 0);
	if(value == carjmbre_outlatch)
		return;
	carjmbre_outlatch = value;

	/* 74259: each address selects one output; only D0 changes that output. */
	switch(offset & 7)
	{
		case 5:
		case 6:
			carjmbre_bgcolor_w((offset & 7) - 5,data & 1);
			break;
		case 7:
			flip_screen_set(data & 1);
			break;
	}
}

static INTERRUPT_GEN( carjmbre_vblank_nmi )
{
	if(carjmbre_outlatch & 8)
		cpunum_set_input_line(0,INPUT_LINE_NMI,PULSE_LINE);
}

static MACHINE_START( carjmbre )
{
	state_save_register_global(carjmbre_outlatch);
	return 0;
}

static MACHINE_RESET( carjmbre )
{
	carjmbre_outlatch = 0;
	carjmbre_bgcolor_w(0,0);
	carjmbre_bgcolor_w(1,0);
	flip_screen_set(0);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0x8800) AM_READ(MRA8_NOP)
	AM_RANGE(0x8800, 0x8807) AM_WRITE(carjmbre_outlatch_w)
	AM_RANGE(0x9000, 0x97ff) AM_READWRITE(MRA8_RAM,carjmbre_videoram_w) AM_BASE(&carjmbre_videoram)
	AM_RANGE(0x9800, 0x98ff) AM_RAM AM_BASE(&carjmbre_spriteram)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa800, 0xa800) AM_READ(input_port_1_r)
	AM_RANGE(0xb800, 0xb800) AM_READWRITE(input_port_2_r,soundlatch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM AM_MIRROR(0x1000)
	AM_RANGE(0x2000, 0x27ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch_r)
	AM_RANGE(0x20, 0x20) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x21, 0x21) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x22, 0x22) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x24, 0x24) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x30, 0x30) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x31, 0x31) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x32, 0x32) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x34, 0x34) AM_READ(AY8910_read_port_1_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( carjmbre )
	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW:4,5")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x18, "Free")
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW:6")
	PORT_DIPSETTING(    0x00, "10k then every 100k" )
	PORT_DIPSETTING(    0x20, "20k then every 100k" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
INPUT_PORTS_END

static const gfx_layout carjmbre_charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), RGN_FRAC(0,2) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout carjmbre_spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	2,
	{ RGN_FRAC(2,4), RGN_FRAC(0,4) },
	{ STEP8(0,1), STEP8(256*16*8,1) },
	{ STEP16(0,8) },
	16*8
};

static const gfx_decode carjmbre_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &carjmbre_charlayout,   0, 16 },
	{ REGION_GFX2, 0, &carjmbre_spritelayout, 0, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( carjmbre )
	MDRV_CPU_ADD(Z80,MASTER_CLOCK/6)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(carjmbre_vblank_nmi,1)

	MDRV_CPU_ADD(Z80,MASTER_CLOCK/6/2)
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_IO_MAP(sound_io_map,0)

	MDRV_MACHINE_START(carjmbre)
	MDRV_MACHINE_RESET(carjmbre)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8,32*8)
	MDRV_VISIBLE_AREA(0,255,16,239)
	MDRV_GFXDECODE(carjmbre_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_PALETTE_INIT(carjmbre)
	MDRV_VIDEO_START(carjmbre)
	MDRV_VIDEO_UPDATE(carjmbre)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910,MASTER_CLOCK/6/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.25)
	MDRV_SOUND_ADD(AY8910,MASTER_CLOCK/6/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.25)
MACHINE_DRIVER_END

ROM_START( carjmbre )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "c1",    0x0000, 0x1000, CRC(62b21739) SHA1(710e5c52f27603aa8f864f6f28d7272f21271d60) )
	ROM_LOAD( "c2",    0x1000, 0x1000, CRC(9ab1a0fa) SHA1(519cf67b98e62b2b42232788ba01ab6637880afc) )
	ROM_LOAD( "c3",    0x2000, 0x1000, CRC(bb29e100) SHA1(93e3cfcf7f8b0b36327f402d9a64c04c3b2c7549) )
	ROM_LOAD( "c4",    0x3000, 0x1000, CRC(c63d8f97) SHA1(9f08fd1cd24a1fb4011864c06580985e009d9af4) )
	ROM_LOAD( "c5",    0x4000, 0x1000, CRC(4d593942) SHA1(30cc649a4be3d7f3705f55d8d0dadb0b63d59ec9) )
	ROM_LOAD( "c6",    0x5000, 0x1000, CRC(fb576963) SHA1(5bf5c54a7c12aa55272629c12b414bf49cda0f1f) )
	ROM_LOAD( "c7",    0x6000, 0x1000, CRC(2b8c4511) SHA1(428a48d6b14455d66720a115bc5f35293dc50de7) )
	ROM_LOAD( "c8",    0x7000, 0x1000, CRC(51cc22a7) SHA1(f614368bfee04f084c70bf145801ac46e5631acb) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "c15",   0x0000, 0x1000, CRC(7d7779d1) SHA1(f8f5246be4cc9632076d3330fc3d3343b911dfee) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c10",   0x0000, 0x1000, CRC(75ddbe56) SHA1(5e1363967a822265618793ccb74bf3ef5e0e00b5) )
	ROM_LOAD( "c9",    0x1000, 0x1000, CRC(2accb821) SHA1(ce2804536fc1abd3377dc864c8c9976ca28c1b6e) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11",   0x0000, 0x1000, CRC(d90cd126) SHA1(7ee110cf19b45ee654016ba0ce92f3db6ea2ed92) )
	ROM_LOAD( "c12",   0x1000, 0x1000, CRC(b3bb39d7) SHA1(89c901be6fae2356ce4d2653e94bf28d6bcf41fe) )
	ROM_LOAD( "c13",   0x2000, 0x1000, CRC(3004010b) SHA1(00d5d2185014159112eb90d8ed50092a3b4ab664) )
	ROM_LOAD( "c14",   0x3000, 0x1000, CRC(fb5f0d31) SHA1(7a27af91efc836bb48c6ed3b283b7c5f7b31c4b5) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "c.d19", 0x0000, 0x0020, CRC(220bceeb) SHA1(46b9f867d014596e2aa7503f104dc721965f0ed5) )
	ROM_LOAD( "c.d18", 0x0020, 0x0020, CRC(7b9ed1b0) SHA1(ec5e1f56e5a2fc726083866c08ac0e1de0ed6ace) )
ROM_END


GAME( 1983, carjmbre, 0, carjmbre, carjmbre, 0, ROT90, "Omori Electric Co., Ltd.", "Car Jamboree", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_GRAPHICS )

/***************************************************************************

Ashita no Joe (Success Joe) [Wave]

driver by David Haywood and bits from Pierpaolo Prazzoli

there is an english logo in the roms, needs different program roms?

Tow sub-boards:

Upper:

 Program
  ROMS 1 to 8 (ST M27512)
  Standard Motorola MC68000P8
  8.0000 MHz osc.
  PALs W9011A (AMD PALCE16V8H) + W9011B (MMI PAL 16L88CN)

 Sound
  ROM 9 (ST M27256)
  Standard Zilog Z80 (Z0840004PSC)
  Yamaha YM2203C

 GFX?
  Mask ROMs 401, 402 & 403 (Hitachi HN62414 Mask ROMs)

Lower:

 GFX?
  Mask ROMs 404, 405, 406, 407, 408 & 409 (Hitachi HN62414 Mask ROMs)
  PAL W90120R2 (MMI PAL 16L88CN)
  EPL (Ricoh EPL16P8BP, not dumped)
  13.3330 MHz osc.

Dips:

 Two banks (* = default)
  A
                                    1   2   3   4   5   6   7   8
   Game Style      * Table          ON                      OFF OFF
                   Upright          OFF                     OFF OFF
   Screen Reverse  * Usual              OFF                 OFF OFF
                   Reverse              ON                  OFF OFF
   Test Mode       * Game mode              OFF             OFF OFF
                   Test Mode                ON              OFF OFF
   Demo Sound      * Yes                        OFF         OFF OFF
                   No                           ON          OFF OFF
   Play Fee - Coin * 1 Coin 1 Play                  OFF OFF OFF OFF
                   1 Coin 2 Play                    ON  OFF OFF OFF
                   2 Coin 1 Play                    OFF ON  OFF OFF
                   2 Coin 3 Play                    ON  ON  OFF OFF

  B
                                    1   2   3   4   5   6   7   8
   Difficulty      * Rank B         OFF OFF OFF OFF OFF OFF OFF OFF
                   Rank A           ON  OFF OFF OFF OFF OFF OFF OFF
                   Rank C           OFF ON  OFF OFF OFF OFF OFF OFF
                   Rank D           ON  ON  OFF OFF OFF OFF OFF OFF

   Easy (A) -> Difficult (D)

Game is controled with 4-direction lever and two buttons
Coin B is not used

*************************************************************************/

#include "driver.h"
#include "sound/2203intf.h"
#include "sound/msm5205.h"

extern UINT16 *ashnojoetileram16, *ashnojoetileram16_2, *ashnojoetileram16_3, *ashnojoetileram16_4, *ashnojoetileram16_5, *ashnojoetileram16_6, *ashnojoetileram16_7;
extern UINT16 *ashnojoe_tilemap_reg;

extern WRITE16_HANDLER( ashnojoe_tileram_w );
extern WRITE16_HANDLER( ashnojoe_tileram2_w );
extern WRITE16_HANDLER( ashnojoe_tileram3_w );
extern WRITE16_HANDLER( ashnojoe_tileram4_w );
extern WRITE16_HANDLER( ashnojoe_tileram5_w );
extern WRITE16_HANDLER( ashnojoe_tileram6_w );
extern WRITE16_HANDLER( ashnojoe_tileram7_w );
extern WRITE16_HANDLER( joe_tilemaps_xscroll_w );
extern WRITE16_HANDLER( joe_tilemaps_yscroll_w );

extern VIDEO_START( ashnojoe );
extern VIDEO_UPDATE( ashnojoe );

static UINT8 adpcm_byte;
static int soundlatch_status;
static int msm5205_vclk_toggle;

static READ16_HANDLER(fake_4a00a_r)
{
	/* If it returns 1 there's no sound. Is it used to sync the game and sound?
	or just a debug enable/disable register? */
	return 0;
}

static WRITE16_HANDLER( ashnojoe_soundlatch_w )
{
	if(ACCESSING_LSB)
	{
		soundlatch_status = 1;
		soundlatch_w(0, data & 0xff);
	}
}

static ADDRESS_MAP_START( ashnojoe_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x040000, 0x043fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x044000, 0x048fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x049000, 0x049fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x04a000, 0x04a001) AM_READ(input_port_0_word_r) // p1 inputs, coins
	AM_RANGE(0x04a002, 0x04a003) AM_READ(input_port_1_word_r) // p2 inputs
	AM_RANGE(0x04a004, 0x04a005) AM_READ(input_port_2_word_r) // dipswitches
	AM_RANGE(0x04a00a, 0x04a00b) AM_READ(fake_4a00a_r) // ??
	AM_RANGE(0x04c000, 0x04ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x080000, 0x0bffff) AM_READ(MRA16_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ashnojoe_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x040000, 0x041fff) AM_WRITE(ashnojoe_tileram3_w) AM_BASE(&ashnojoetileram16_3)
	AM_RANGE(0x042000, 0x043fff) AM_WRITE(ashnojoe_tileram4_w) AM_BASE(&ashnojoetileram16_4)
	AM_RANGE(0x044000, 0x044fff) AM_WRITE(ashnojoe_tileram5_w) AM_BASE(&ashnojoetileram16_5)
	AM_RANGE(0x045000, 0x045fff) AM_WRITE(ashnojoe_tileram2_w) AM_BASE(&ashnojoetileram16_2)
	AM_RANGE(0x046000, 0x046fff) AM_WRITE(ashnojoe_tileram6_w) AM_BASE(&ashnojoetileram16_6)
	AM_RANGE(0x047000, 0x047fff) AM_WRITE(ashnojoe_tileram7_w) AM_BASE(&ashnojoetileram16_7)
	AM_RANGE(0x048000, 0x048fff) AM_WRITE(ashnojoe_tileram_w) AM_BASE(&ashnojoetileram16)
	AM_RANGE(0x049000, 0x049fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x04a006, 0x04a007) AM_WRITE(MWA16_RAM) AM_BASE(&ashnojoe_tilemap_reg)
	AM_RANGE(0x04a008, 0x04a009) AM_WRITE(ashnojoe_soundlatch_w)
	AM_RANGE(0x04a010, 0x04a019) AM_WRITE(joe_tilemaps_xscroll_w)
	AM_RANGE(0x04a020, 0x04a029) AM_WRITE(joe_tilemaps_yscroll_w)
	AM_RANGE(0x04c000, 0x04ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x080000, 0x0bffff) AM_WRITE(MWA16_ROM)
ADDRESS_MAP_END

static WRITE8_HANDLER( adpcm_w )
{
	adpcm_byte = data;
}

static READ8_HANDLER( sound_latch_r )
{
	soundlatch_status = 0;
	return soundlatch_r(0);
}

static READ8_HANDLER( sound_latch_status_r )
{
    return soundlatch_status;
}

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK4)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0x04, 0x04) AM_READ(sound_latch_r)
	AM_RANGE(0x06, 0x06) AM_READ(sound_latch_status_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(adpcm_w)
ADDRESS_MAP_END

INPUT_PORTS_START( ashnojoe )
	PORT_START_TAG("IN0")	/* player 1 16-bit */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // anything else and the controls don't work
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")	/* player 2 16-bit */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // anything else and the controls don't work
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* unused ? */
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	/* 16-bit */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x0400, 0x0000, "Number of controller" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0400, "1" )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
	 32,36,40, 44, 48, 52, 56, 60 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};

static const gfx_decode ashnojoe_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 0x100 },
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 0x100 },
	{ REGION_GFX3, 0, &tiles8x8_layout, 0, 0x100 },
	{ REGION_GFX4, 0, &tiles16x16_layout, 0, 0x100 },
	{ REGION_GFX5, 0, &tiles16x16_layout, 0, 0x100 },
	{ -1 }
};

static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE8_HANDLER(writeA)
{
	/* This gets called at 8910 startup with 0xff before the 5205 exists, causing a crash */
	if (data == 0xff)
		return;

	MSM5205_reset_w(0, !(data & 0x01));
}

static WRITE8_HANDLER(writeB)
{
	memory_set_bankptr(4, memory_region(REGION_SOUND1) + ((data & 0xf) * 0x8000));
}

static void ashnojoe_vclk_cb(int data)
{
	if (msm5205_vclk_toggle == 0)
	{
		MSM5205_data_w(0, adpcm_byte >> 4);
	}
	else
	{
		MSM5205_data_w(0, adpcm_byte & 0xf);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}

	msm5205_vclk_toggle ^= 1;
}

static struct MSM5205interface msm5205_interface =
{
	ashnojoe_vclk_cb,	/* interrupt function */
	MSM5205_S48_4B		/* 4KHz 4-bit */
};

static struct YM2203interface ym2203_interface =
{
	0,
	0,
	writeA,
	writeB,
	irqhandler
};

static DRIVER_INIT( ashnojoe )
{
	memory_set_bankptr(4, memory_region(REGION_SOUND1));
}

static MACHINE_DRIVER_START( ashnojoe )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz? */
	MDRV_CPU_PROGRAM_MAP(ashnojoe_readmem,ashnojoe_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(14*8, 50*8-1, 3*8, 29*8-1)

	MDRV_GFXDECODE(ashnojoe_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000/2)

	MDRV_VIDEO_START(ashnojoe)
	MDRV_VIDEO_UPDATE(ashnojoe)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.1)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( scessjoe )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_BYTE( "5.4q", 0x00000, 0x10000, CRC(c805f9e7) SHA1(e1e85701bde496b1fd64211b94bfb0def597ae51) )
	ROM_LOAD16_BYTE( "6.4s", 0x00001, 0x10000, CRC(eda7a537) SHA1(3bb19fbdfb6c8af4e2078958fa445ac1f4434d0d) )
	ROM_LOAD16_WORD_SWAP( "sj201-nw.6m", 0x80000, 0x40000, CRC(5a64ca42) SHA1(660b8bca21ef3c2230adce7cb7e7d1f018714f23) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 32k for Z80 code */
	ROM_LOAD( "9.8q", 0x0000, 0x8000, CRC(8767e212) SHA1(13bf927febedff9d7d164fbf0da7fb3a588c2a94) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8.5e", 0x00000, 0x10000, CRC(9bcb160e) SHA1(1677048e5ce26562ff7ba36fcc2d0ed5a652b91e) )
	ROM_LOAD( "7.5c", 0x10000, 0x10000, CRC(b250c69d) SHA1(594b1bb94a162b07944a971b7fedddca5c37f2cb) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "4.4e", 0x00000, 0x10000, CRC(aa6336d3) SHA1(43f70cc3223f11d7929dd44b0edf0a31f5fe41c3) )
	ROM_LOAD( "3.4c", 0x10000, 0x10000, CRC(7e2d86b5) SHA1(8b8d1b9240a700e29afc109eddf6e58a0a7666a4) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2.3m", 0x00000, 0x10000, CRC(c3254938) SHA1(fd57163f740cd4fdecca94cced91314c289741ae) )
	ROM_LOAD( "1.1m", 0x10000, 0x10000, CRC(5d16a6fa) SHA1(2af907b0fcb9ff93340de3301da4b10e945455e5) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "sj402-nw.8e", 0x000000, 0x80000, CRC(b6d33d06) SHA1(688ccf467a5112ec522811894e2626ab5f155903) )
	ROM_LOAD16_WORD_SWAP( "sj403-nw.7e", 0x080000, 0x80000, CRC(07143f56) SHA1(1b953c8826d3993a486eed6b9d94d37145fd2e79) )

	ROM_REGION( 0x300000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "sj404-nw.7a", 0x000000, 0x80000, CRC(8f134128) SHA1(026a6076d54cd5f1d06b29c51031cb79a6b2c11d) )
	ROM_LOAD16_WORD_SWAP( "sj405-nw.7c", 0x080000, 0x80000, CRC(6fd81699) SHA1(8a4f9e47dd39b4b0213c3682da2221ca53bba658) )
	ROM_LOAD16_WORD_SWAP( "sj406-nw.7d", 0x100000, 0x80000, CRC(634e33e6) SHA1(1d6a72a4ca80cd1c1fd6ce9359c304b45091cdfe) )
	ROM_LOAD16_WORD_SWAP( "sj407-nw.7f", 0x180000, 0x80000, CRC(5c66ff06) SHA1(9923ba00679e1b47b5da63c1a13e0f8dd4c78bb5) )
	ROM_LOAD16_WORD_SWAP( "sj408-nw.7g", 0x200000, 0x80000, CRC(6a3b1ea1) SHA1(e39a6e52d930f291bf237cf9db3d4b3d2fad53e0) )
	ROM_LOAD16_WORD_SWAP( "sj409-nw.7j", 0x280000, 0x80000, CRC(d8764213) SHA1(89eadefb956863216c8e3d0380394aba35e8c856) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )   /* samples? */
	ROM_LOAD( "sj401-nw.10r", 0x00000, 0x80000, CRC(25dfab59) SHA1(7d50159204ba05323a2442778f35192e66117dda) )
ROM_END

ROM_START( ashnojoe )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_BYTE( "5.bin", 0x00000, 0x10000, CRC(c61e1569) SHA1(422c18f5810539b5a9e3a9bd4e3b4d70bde8d1d5) )
	ROM_LOAD16_BYTE( "6.bin", 0x00001, 0x10000, CRC(c0a16338) SHA1(fb127b9d38f2c9807b6e23ff71935fc8a22a2e8f) )
	ROM_LOAD16_WORD_SWAP( "sj201-nw.bin", 0x80000, 0x40000, CRC(5a64ca42) SHA1(660b8bca21ef3c2230adce7cb7e7d1f018714f23) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 32k for Z80 code */
	ROM_LOAD( "9.bin", 0x0000, 0x8000, CRC(8767e212) SHA1(13bf927febedff9d7d164fbf0da7fb3a588c2a94) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8.bin", 0x00000, 0x10000, CRC(9bcb160e) SHA1(1677048e5ce26562ff7ba36fcc2d0ed5a652b91e) )
	ROM_LOAD( "7.bin", 0x10000, 0x10000, CRC(7e1efc42) SHA1(e3c282072fdaa0b98c2a1bf25fd02c680d9ca4d7) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "4.bin", 0x00000, 0x10000, CRC(aa6336d3) SHA1(43f70cc3223f11d7929dd44b0edf0a31f5fe41c3) )
	ROM_LOAD( "3.bin", 0x10000, 0x10000, CRC(7e2d86b5) SHA1(8b8d1b9240a700e29afc109eddf6e58a0a7666a4) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bin", 0x00000, 0x10000, CRC(c3254938) SHA1(fd57163f740cd4fdecca94cced91314c289741ae) )
	ROM_LOAD( "1.bin", 0x10000, 0x10000, CRC(1bf585f0) SHA1(4003941636e7fded95e880109c3c9dd1d8f28b07) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "sj402-nw.bin", 0x000000, 0x80000, CRC(b6d33d06) SHA1(688ccf467a5112ec522811894e2626ab5f155903) )
	ROM_LOAD16_WORD_SWAP( "sj403-nw.bin", 0x080000, 0x80000, CRC(07143f56) SHA1(1b953c8826d3993a486eed6b9d94d37145fd2e79) )

	ROM_REGION( 0x300000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "sj404-nw.bin", 0x000000, 0x80000, CRC(8f134128) SHA1(026a6076d54cd5f1d06b29c51031cb79a6b2c11d) )
	ROM_LOAD16_WORD_SWAP( "sj405-nw.bin", 0x080000, 0x80000, CRC(6fd81699) SHA1(8a4f9e47dd39b4b0213c3682da2221ca53bba658) )
	ROM_LOAD16_WORD_SWAP( "sj406-nw.bin", 0x100000, 0x80000, CRC(634e33e6) SHA1(1d6a72a4ca80cd1c1fd6ce9359c304b45091cdfe) )
	ROM_LOAD16_WORD_SWAP( "sj407-nw.bin", 0x180000, 0x80000, CRC(5c66ff06) SHA1(9923ba00679e1b47b5da63c1a13e0f8dd4c78bb5) )
	ROM_LOAD16_WORD_SWAP( "sj408-nw.bin", 0x200000, 0x80000, CRC(6a3b1ea1) SHA1(e39a6e52d930f291bf237cf9db3d4b3d2fad53e0) )
	ROM_LOAD16_WORD_SWAP( "sj409-nw.bin", 0x280000, 0x80000, CRC(d8764213) SHA1(89eadefb956863216c8e3d0380394aba35e8c856) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )   /* samples? */
	ROM_LOAD( "sj401-nw.bin", 0x00000, 0x80000, CRC(25dfab59) SHA1(7d50159204ba05323a2442778f35192e66117dda) )
ROM_END

GAME(1990, scessjoe, 0,        ashnojoe, ashnojoe, ashnojoe, ROT0, "WAVE / Taito Corporation", "Success Joe (World)", 0 )
GAME(1990, ashnojoe, scessjoe, ashnojoe, ashnojoe, ashnojoe, ROT0, "WAVE / Taito Corporation", "Ashita no Joe (Japan)", 0 )

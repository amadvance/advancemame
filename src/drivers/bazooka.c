/***************************************************************************

    Bazooka (Taito do Brasil), 1977, TTL

    Driver by Antonio "Nino MegaDriver" Tornisiello.
    Based on two repaired boards of the game back to working condition.

    Important Notes:
    The game runs entirely on TTL and the Z80 is used here just for
    simulating playability. Game logic is simulated within this driver to
    demonstrate the graphics, using original dumped and decoded ROMs,
    and investigation of the circuit during repair time and knoledge of
    acual gameplay.

    Original game had a rack of external boards to generate discrete
    sound for each vehile and a sum of of all to generate an explosion (noise)
    effect. Each vehicle and noise reproduced here using SN76496. Sound
    accuracy limited to the recorded stream and fftw from the working board,
    and the fact that "Taito do Brasil" used SN76496 and AY-3-8910 for almost all
    of their video games...

***************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "bazooka.h"

/* Input Ports */
ADDRESS_MAP_START( bazooka_readport, ADDRESS_SPACE_IO, 8 )
  AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
ADDRESS_MAP_END

INPUT_PORTS_START( bazooka )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Fire")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
  PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_NAME("Hard Reset") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/* Graphics Layouts */
static const gfx_layout bazooka_tiles =
{
    8, 16, 32, 1,
  { 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7 },
  { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
    16*8
};

static const gfx_layout bazooka_nbr =
{   7, 12, 11, 1,
  { 0 },
  { 0, 1, 2, 3, 4, 5, 6 },
  { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
    12*8
};

static const gfx_layout bazooka_go =
{   6, 8, 9, 1,
  { 0 },
  { 0, 1, 2, 3, 4, 5 },
  { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8
};

/* Graphics Decode Information */
static gfx_decode bazooka_gfxdecodeinfo[] = {
	{ REGION_GFX1,  0, &bazooka_tiles, 0, 1 },
	{ REGION_GFX2,  0, &bazooka_tiles, 0, 1 },
	{ REGION_USER3, 0, &bazooka_nbr,   0, 2 },
	{ REGION_USER4, 0, &bazooka_go,    0, 1 },
	{ -1 } };

INTERRUPT_GEN( bazooka_interrupt )
{
    bazooka_video_irq();
}

/* Machine Driver */
MACHINE_DRIVER_START( bazooka )
  MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(0, 0)
	MDRV_CPU_IO_MAP(0, bazooka_readport)
	MDRV_CPU_VBLANK_INT(bazooka_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
  MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_GFXDECODE(bazooka_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(bazooka)
	MDRV_VIDEO_START(bazooka)
	MDRV_VIDEO_UPDATE(bazooka)
	
	MDRV_SPEAKER_STANDARD_MONO("mono")

	/* SN76496 #0: For Vehicle Lines 0, 1, and 2 */
	MDRV_SOUND_ADD(SN76496, 640000 )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.70)

	/* SN76496 #1: For Tank Sound (Chan 0) */
	MDRV_SOUND_ADD(SN76496, 640000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	/* SN76496 #2: For Fire/Explosion noise. */
  MDRV_SOUND_ADD(SN76496, 12000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	/* SN76496 #3: For Ambulance Sound ONLY */
	MDRV_SOUND_ADD(SN76496, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

MACHINE_DRIVER_END

/* 
   bk[0-9] are original dumped proms,
   bk01.1l and bk02.4l was transcoded to format to
   make it easier to implement here. All versions
   also added to keep track of dumped ROMs
*/
ROM_START( bazooka )

	ROM_REGION( 0x1000, REGION_CPU1, 0 )

	ROM_REGION( 0x200, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bk01.XX", 0x0000, 0x0200, CRC(1a9c3340) SHA1(426549fe52c8f8e028fe657d23d8c1c54b38d3f6) )

  ROM_REGION( 0x200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bk02.XX", 0x0000, 0x0200, CRC(22262d25) SHA1(dd91f74f76269aba27c244b74a706b83f087d853) )

	ROM_REGION( 0x200, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "bk01.1l", 0x0000, 0x0200, CRC(edc34cb0) SHA1(f76a81833b015784e55b33189e9058cd24922f9b) )

  ROM_REGION( 0x200, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "bk02.4l", 0x0000, 0x0200, CRC(3e78e4c2) SHA1(814509eb773bfa87f1df933214f079e7dd2a8fa2) )

  ROM_REGION( 0x400, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD( "bk03.8j", 0x0000, 0x0200, CRC(4fc10886) SHA1(b1c6f890994ba2182a4e7fc17582d6797dbd6ce9) )
	ROM_LOAD( "bk04.8h", 0x200, 0x0200, CRC(00179936) SHA1(e5417b8d3814dafe1278179b307a1b563a378cbe) )

  ROM_REGION( 0x40, REGION_USER2, ROMREGION_DISPOSE )
	ROM_LOAD( "bk05.6c", 0x0000, 0x0020, CRC(4193d32e) SHA1(d9e3392a8681198e110cfcd68ef20ae3dc366527) )
	ROM_LOAD( "bk06.6d", 0x0020, 0x0020, CRC(1bfb073f) SHA1(f6b26dcece71b2cf2ed4a537434edbe31cb10399) )

  ROM_REGION( 0x100, REGION_USER3, ROMREGION_DISPOSE )
	ROM_LOAD( "bkXX.bin", 0x0000, 0x84, CRC(945b0849) SHA1(bf92a549c25f4a7c88b0a5f9733e839e31d4f40f) )

  ROM_REGION( 0x100, REGION_USER4, ROMREGION_DISPOSE )
	ROM_LOAD( "bkYY.bin",  0x0000, 0x48, CRC(5a713e2f) SHA1(f2d0115598436b72d2427a14e911226b5d95d10d) )

ROM_END

/* Driver Initialization */
DRIVER_INIT( bazooka ) {
    bazooka_video_reset();
}

/* Game Driver */
GAME( 1977, bazooka, 0, bazooka, bazooka, bazooka, ROT0, "Taito do Brasil", "Bazooka", 0 )

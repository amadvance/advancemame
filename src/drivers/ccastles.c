/***************************************************************************

    Atari Crystal Castles hardware

    driver by Pat Lawrence

    Games supported:
        * Crystal Castles (1983) [3 sets]

    Known issues:
        * none at this time

****************************************************************************

    Crystal Castles memory map.

     Address  A A A A A A A A A A A A A A A A  R  D D D D D D D D  Function
              1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0  /  7 6 5 4 3 2 1 0
              5 4 3 2 1 0                      W
    -------------------------------------------------------------------------------
    0000      X X X X X X X X X X X X X X X X  W  X X X X X X X X  X Coordinate
    0001      0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1  W  D D D D D D D D  Y Coordinate
    0002      0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 R/W D D D D          Bit Mode
    0003-0BFF 0 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (DRAM)
    0C00-7FFF 0 A A A A A A A A A A A A A A A R/W D D D D D D D D  Screen RAM
    8000-8DFF 1 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (STATIC)
    8E00-8EFF 1 0 0 0 1 1 1 0 A A A A A A A A R/W D D D D D D D D  MOB BUF 2
    -------------------------------------------------------------------------------
    8F00-8FFF 1 0 0 0 1 1 1 1 A A A A A A A A R/W D D D D D D D D  MOB BUF 1
                                          0 0 R/W D D D D D D D D  MOB Picture
                                          0 1 R/W D D D D D D D D  MOB Vertical
                                          1 0 R/W D D D D D D D D  MOB Priority
                                          1 1 R/W D D D D D D D D  MOB Horizontal
    -------------------------------------------------------------------------------
    9000-90FF 1 0 0 1 0 0 X X A A A A A A A A R/W D D D D D D D D  NOVRAM
    9400-9401 1 0 0 1 0 1 0 X X X X X X X 0 A  R                   TRAK-BALL 1
    9402-9403 1 0 0 1 0 1 0 X X X X X X X 1 A  R                   TRAK-BALL 2
    9500-9501 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL 1 mirror
    9600      1 0 0 1 0 1 1 X X X X X X X X X  R                   IN0
                                               R                D  COIN R
                                               R              D    COIN L
                                               R            D      COIN AUX
                                               R          D        SLAM
                                               R        D          SELF TEST
                                               R      D            VBLANK
                                               R    D              JMP1
                                               R  D                JMP2
    -------------------------------------------------------------------------------
    9800-980F 1 0 0 1 1 0 0 X X X X X A A A A R/W D D D D D D D D  CI/O 0
    9A00-9A0F 1 0 0 1 1 0 1 X X X X X A A A A R/W D D D D D D D D  CI/O 1
    9A08                                                    D D D  Option SW
                                                          D        SPARE
                                                        D          SPARE
                                                      D            SPARE
    9C00      1 0 0 1 1 1 0 0 0 X X X X X X X  W                   RECALL
    -------------------------------------------------------------------------------
    9C80      1 0 0 1 1 1 0 0 1 X X X X X X X  W  D D D D D D D D  H Scr Ctr Load
    9D00      1 0 0 1 1 1 0 1 0 X X X X X X X  W  D D D D D D D D  V Scr Ctr Load
    9D80      1 0 0 1 1 1 0 1 1 X X X X X X X  W                   Int. Acknowledge
    9E00      1 0 0 1 1 1 1 0 0 X X X X X X X  W                   WDOG
              1 0 0 1 1 1 1 0 1 X X X X A A A  W                D  OUT0
    9E80                                0 0 0  W                D  Trak Ball Light P1
    9E81                                0 0 1  W                D  Trak Ball Light P2
    9E82                                0 1 0  W                D  Store Low
    9E83                                0 1 1  W                D  Store High
    9E84                                1 0 0  W                D  Spare
    9E85                                1 0 1  W                D  Coin Counter R
    9E86                                1 1 0  W                D  Coin Counter L
    9E87                                1 1 1  W                D  BANK0-BANK1
              1 0 0 1 1 1 1 1 0 X X X X A A A  W          D        OUT1
    9F00                                0 0 0  W          D        ^AX
    9F01                                0 0 1  W          D        ^AY
    9F02                                0 1 0  W          D        ^XINC
    9F03                                0 1 1  W          D        ^YINC
    9F04                                1 0 0  W          D        PLAYER2 (flip screen)
    9F05                                1 0 1  W          D        ^SIRE
    9F06                                1 1 0  W          D        BOTHRAM
    9F07                                1 1 1  W          D        BUF1/^BUF2 (sprite bank)
    9F80-9FBF 1 0 0 1 1 1 1 1 1 X A A A A A A  W  D D D D D D D D  COLORAM
    A000-FFFF 1 A A A A A A A A A A A A A A A  R  D D D D D D D D  Program ROM

***************************************************************************/

#include "driver.h"
#include "sound/pokey.h"
#include "ccastles.h"



/*************************************
 *
 *  Output ports
 *
 *************************************/

static WRITE8_HANDLER( ccastles_led_w )
{
	set_led_status(offset,~data & 1);
}


static WRITE8_HANDLER( ccastles_coin_counter_w )
{
	/* this is not working, haven't investigated why */
	coin_counter_w(offset^1, ~data);
}


static WRITE8_HANDLER( ccastles_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);


	if (data) { memory_set_bankptr(1,&RAM[0x10000]); }
	else { memory_set_bankptr(1,&RAM[0xa000]); }
}


static WRITE8_HANDLER( flip_screen_w )
{
	flip_screen_set(data);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0001) AM_WRITE(MWA8_RAM) AM_BASE(&ccastles_screen_addr)
	AM_RANGE(0x0002, 0x0002) AM_READWRITE(ccastles_bitmode_r, ccastles_bitmode_w)
	AM_RANGE(0x0000, 0x8fff) AM_RAM
	AM_RANGE(0x0c00, 0x7fff) AM_BASE(&videoram)
	AM_RANGE(0x8e00, 0x8eff) AM_BASE(&spriteram_2) AM_SIZE(&spriteram_size)
	AM_RANGE(0x8f00, 0x8fff) AM_BASE(&spriteram)
	AM_RANGE(0x9000, 0x90ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x9400, 0x9400) AM_READ(input_port_2_r)	/* trackball y - player 1 */
	AM_RANGE(0x9402, 0x9402) AM_READ(input_port_2_r)	/* trackball y - player 2 */
	AM_RANGE(0x9500, 0x9500) AM_READ(input_port_2_r)	/* trackball y - player 1 mirror */
	AM_RANGE(0x9401, 0x9401) AM_READ(input_port_3_r)	/* trackball x - player 1 */
	AM_RANGE(0x9403, 0x9403) AM_READ(input_port_3_r)	/* trackball x - player 2 */
	AM_RANGE(0x9501, 0x9501) AM_READ(input_port_3_r)	/* trackball x - player 1 mirror */
	AM_RANGE(0x9600, 0x9600) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x9800, 0x980f) AM_READWRITE(pokey1_r, pokey1_w) /* Random # generator on a Pokey */
	AM_RANGE(0x9a00, 0x9a0f) AM_READWRITE(pokey2_r, pokey2_w) /* Random #, IN1 */
	AM_RANGE(0x9c80, 0x9c80) AM_WRITE(MWA8_RAM) AM_BASE(&ccastles_scrollx)
	AM_RANGE(0x9d00, 0x9d00) AM_WRITE(MWA8_RAM) AM_BASE(&ccastles_scrolly)
	AM_RANGE(0x9d80, 0x9d80) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x9e00, 0x9e00) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x9e80, 0x9e81) AM_WRITE(ccastles_led_w)
	AM_RANGE(0x9e85, 0x9e86) AM_WRITE(ccastles_coin_counter_w)
	AM_RANGE(0x9e87, 0x9e87) AM_WRITE(ccastles_bankswitch_w)
	AM_RANGE(0x9f00, 0x9f01) AM_WRITE(MWA8_RAM) AM_BASE(&ccastles_screen_inc_enable)
	AM_RANGE(0x9f02, 0x9f03) AM_WRITE(MWA8_RAM) AM_BASE(&ccastles_screen_inc)
	AM_RANGE(0x9f04, 0x9f04) AM_WRITE(flip_screen_w)
	AM_RANGE(0x9f05, 0x9f06) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x9f07, 0x9f07) AM_WRITE(MWA8_RAM) AM_BASE(&ccastles_sprite_bank)
	AM_RANGE(0x9f80, 0x9fbf) AM_WRITE(ccastles_paletteram_w)
	AM_RANGE(0xa000, 0xdfff) AM_ROMBANK(1)
	AM_RANGE(0xe000, 0xffff) AM_ROM					/* ROMs/interrupt vectors */
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( ccastles )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* 1p Jump, non-cocktail start1 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)		/* 2p Jump, non-cocktail start2 */

	PORT_START	/* IN1 */
	PORT_BIT ( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START1 )				/* cocktail only */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )				/* cocktail only */
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING (   0x20, DEF_STR( Cocktail ) )
	PORT_BIT ( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, 0x7f, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(30)
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout ccastles_spritelayout =
{
	8,16,
	256,
	4,
	{ 0x2000*8+0, 0x2000*8+4, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &ccastles_spritelayout,  0, 1 },
	{ -1 }
};



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	{ 0 },
	input_port_1_r
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( ccastles )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,1500000)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 232)
	MDRV_VISIBLE_AREA(0, 255, 0, 231)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_VIDEO_START(ccastles)
	MDRV_VIDEO_UPDATE(ccastles)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(POKEY, 1250000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(POKEY, 1250000)
	MDRV_SOUND_CONFIG(pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( ccastles )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "136022-403.bin",  0x0a000, 0x2000, CRC(81471ae5) SHA1(8ec13b48119ecf8fe85207403c0a0de5240cded4) )
     ROM_LOAD( "136022-404.bin",  0x0c000, 0x2000, CRC(820daf29) SHA1(a2cff00e9ddce201344692b75038431e4241fedd) )
     ROM_LOAD( "136022-405.bin",  0x0e000, 0x2000, CRC(4befc296) SHA1(2e789a32903808014e9d5f3021d7eff57c3e2212) )
     ROM_LOAD( "136022-102.bin", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "136022-101.bin", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "136022-107.bin", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "136022-106.bin", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )

     ROM_REGION( 0x0400, REGION_PROMS, 0 )
     ROM_LOAD( "136022-108.bin", 0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) ) /* Synchronous PROM I.C., not used */
     ROM_LOAD( "136022-109.bin", 0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) ) /* Bus PROM I.C., not used */
     ROM_LOAD( "136022-110.bin", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) ) /* Write PROM I.C., not used */
     ROM_LOAD( "136022-111.bin", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) ) /* Color PROM I.C., not used */
ROM_END


ROM_START( ccastle3 )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "136022-303.bin", 0x0a000, 0x2000, CRC(10e39fce) SHA1(5247f52e14ccf39f0ec699a39c8ebe35e61e07d2) )
     ROM_LOAD( "136022-304.bin", 0x0c000, 0x2000, CRC(74510f72) SHA1(d22550f308ff395d51869b52449bc0669a4e35e4) )
     ROM_LOAD( "136022-305.bin", 0x0e000, 0x2000, CRC(9418cf8a) SHA1(1f835db94270e4a16e721b2ac355fb7e7c052285) )
     ROM_LOAD( "136022-102.bin", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "136022-101.bin", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "136022-107.bin", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "136022-106.bin", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )

     ROM_REGION( 0x0400, REGION_PROMS, 0 )
     ROM_LOAD( "136022-108.bin", 0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) ) /* Synchronous PROM I.C., not used */
     ROM_LOAD( "136022-109.bin", 0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) ) /* Bus PROM I.C., not used */
     ROM_LOAD( "136022-110.bin", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) ) /* Write PROM I.C., not used */
     ROM_LOAD( "136022-111.bin", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) ) /* Color PROM I.C., not used */
ROM_END


ROM_START( ccastle2 )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "136022-203.bin", 0x0a000, 0x2000, CRC(348a96f0) SHA1(76de7bf6a01ccb15a4fe7333c1209f623a2e0d1b) )
     ROM_LOAD( "136022-204.bin", 0x0c000, 0x2000, CRC(d48d8c1f) SHA1(8744182a3e2096419de63e341feb77dd8a8bcb34) )
     ROM_LOAD( "136022-205.bin", 0x0e000, 0x2000, CRC(0e4883cc) SHA1(a96abbf654e087409a90c1686d9dd553bd08c14e) )
     ROM_LOAD( "136022-102.bin", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "136022-101.bin", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "136022-107.bin", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "136022-106.bin", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )

     ROM_REGION( 0x0400, REGION_PROMS, 0 )
     ROM_LOAD( "136022-108.bin", 0x0000, 0x0100, CRC(6ed31e3b) SHA1(c3f3e4e7f313ecfd101cc52dfc44bd6b51a2ac88) ) /* Synchronous PROM I.C., not used */
     ROM_LOAD( "136022-109.bin", 0x0100, 0x0100, CRC(b3515f1a) SHA1(c1bf077242481ef2f958580602b8113532b58612) ) /* Bus PROM I.C., not used */
     ROM_LOAD( "136022-110.bin", 0x0200, 0x0100, CRC(068bdc7e) SHA1(ae155918fdafd14299bc448b43eed8ad9c1ef5ef) ) /* Write PROM I.C., not used */
     ROM_LOAD( "136022-111.bin", 0x0300, 0x0100, CRC(c29c18d9) SHA1(278bf61a290ae72ddaae2bafb4ab6739d3fb6238) ) /* Color PROM I.C., not used */
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1983, ccastles, 0,        ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 4)", 0 )
GAME( 1983, ccastle3, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 3)", 0 )
GAME( 1983, ccastle2, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 2)", 0 )

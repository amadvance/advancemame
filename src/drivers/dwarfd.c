/*

  Electro-Sport Dwarfs Den
  http://www.arcadeflyers.net/?page=flyerdb&subpage=thumbs&id=3993

                                               ______________
  _____________________________________________||||||||||||||_______________________________________
  |         1         2         3         4         5         6         7         8         9      |
  |                                                                                      ________  |
  ||| L                                                   SN7445N74  74LS224N  7404-PC   |9L    |  |
D |||                                                                                    |______|  |
  |||                                                                   _____________    ________  |
  ||| K                         BATTERY         16-1-471  7400-PC       | M5L8085AP |    |9K    |  |
  |                                                                     |___________|    |______|  |
  |                                                                                      ________  |
  ||| J           74LS273NA                     --------  --------             74LS373N  |9J    |  |
C |||                                                                                    |______|  |
  |||                                                                                    ________  |
  ||| H           74LS273NA  SN7442AN  74107N   74161N    SN7432N    M3-7602-5 MDP1603   |9H    |  |
  |                                                                                      |______|  |
  |                                                                                                |
  ||| F           74LS273NA  7408N     7486N    OSC       7404                 MM2114N   MM2114N   |
P |||                                                                                              |
  |||                                  _____________                                               |
  ||| E                      SN7442AN  |iP8274     |      7400       74LS244N  MM2114N   MM2114N   |
  |                                    |___________|                                               |
  |                                    _____________      ________                                 |
  ||| D 16-1-471  MDP1603    7414      |AY-3-8910  |      |6D    |   74LS245   MM2114N   MM2114N   |
B |||                                  |___________|      |______|                                 |
  |||                                                     ________   _______                       |
  ||| C 16-1-471  MDP1603    7414      7414     7400      |6C    |   |7C   |   M3-6514-9 M3-6514-9 |
  |                                                       |______|                                 |
  |                                                       ________   __________________            |
  ||| B SN74175N             74174N    74LS374N 74LS374N  |6B    |   |                |  6-1-471   |
A |||                                                     |______|   |                |            |
  |||                        _______                      ________   |    ______      |            |
  ||| A DN74505N             |3A   |   74153N   74153N    |6A    |   |    LM324N      |  DIP-SW    |
  |                                                       |______|   |                |            |
  |                                                                  |________________|            |
  |________________________________________________________________________________________________|

  OSC = 10.595 MHz
  D,C,A = 20-pin connectors
  B = 26-pin connector
  P = 12-pin connector (seems power)
  Edge connector (top) is not JAMMA
  3A (63S080N) dumped as 82S123
  7C = non populated



    todo:
    fix gfx decode first.. then we'll see

*/

#include "driver.h"
#include "cpu/i8085/i8085.h"

static UINT8 *dwarfd_videoram;

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x4fff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(MWA8_RAM) AM_BASE(&dwarfd_videoram)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END


INPUT_PORTS_START( dwarfd )
	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )
INPUT_PORTS_END


/* Wrong! */
static const gfx_layout tiles8x8_layout =
{
	16,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20,24,28, 32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX3, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX4, 0, &tiles8x8_layout, 0, 16 },

	{ -1 }
};

VIDEO_START(dwarfd)
{
	return 0;
}

VIDEO_UPDATE(dwarfd)
{
	/* temp test.. */
	int x,y;
	int count=0;

	fillbitmap(bitmap, get_black_pen(), cliprect);

	for (y=0;y<128;y++)
	{
		for (x=0;x<32;x++)
		{
			int tile;

			tile = dwarfd_videoram[count];
			count++;

			drawgfx(bitmap,Machine->gfx[0],
					tile,
					0, // col
					0, 0, // flipx,flipy
					x*16,y*8,
					cliprect,TRANSPARENCY_PEN,0);


		}
	}


}

static MACHINE_DRIVER_START( dwarfd )
	/* basic machine hardware */
	MDRV_CPU_ADD(8085A, 10595000/4)        /* ? MHz */

	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	/* add io map */
//  MDRV_CPU_VBLANK_INT(irq0_line_hold,1)  // ?

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(512, 1024)
	MDRV_VISIBLE_AREA(0, 512-1, 0, 1024-1) // so we can fit all vid ram on screen ;-)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(dwarfd)
	MDRV_VIDEO_UPDATE(dwarfd)
MACHINE_DRIVER_END



ROM_START( dwarfd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "9l_pd_50-3196_m5l2732k.bin", 0x0000, 0x1000, CRC(34e942ae) SHA1(d4f0ee7f29e1c1a93b4b30b950023dbf60596100) )
	ROM_LOAD( "9k_pd_50-3193_hn462732g.bin",0x1000, 0x1000, CRC(78f0c260) SHA1(d6c3b8b3ef4ce99a811e291f1396a47106683df9) )
	ROM_LOAD( "9j_pd_50-3192_mbm2732.bin",  0x2000, 0x1000, CRC(9c66ee6e) SHA1(49c20fa276508b3c7b0134909295ae04ee46890f) )
	ROM_LOAD( "9h_pd_50-3375_2732.bin",     0x3000, 0x1000, CRC(daf5551d) SHA1(933e3453c9e74ca6695137c9f6b1abc1569ad019) )

	/* a pair? */
	ROM_REGION( 0x1000, REGION_GFX1, 0 )
	ROM_LOAD( "6a_pd_50_1991_2732.bin"      ,0x0000, 0x1000, CRC(6da494bc) SHA1(0323eaa5f81e3b8561225ccdd4654c9a11f2167c) )
	ROM_REGION( 0x1000, REGION_GFX2, 0 )
	ROM_LOAD( "6c_pd_50-1993_tms2732ajl.bin",0x0000, 0x1000, CRC(cd8e5e54) SHA1(0961739d72d80e0ac00e6cbf9643bcebfe74830d) )

	/* a pair? */
	ROM_REGION( 0x1000, REGION_GFX3, 0 )
	ROM_LOAD( "6b_pd_50-1992_tms2732ajl.bin",0x0000, 0x1000, CRC(69208e1a) SHA1(8706f8f0d2dfeba5cebc71985ea46a67de13bc7d) )
	ROM_REGION( 0x1000, REGION_GFX4, 0 )
	ROM_LOAD( "6d_pd_50-1994_tms2732ajl.bin",0x0000, 0x1000, CRC(ef52b88c) SHA1(3405152da3194a71f6dac6492f275c746e781ee7) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )
	ROM_LOAD( "3a_50-1381_63s080n.bin",0x0000, 0x20, CRC(451d0a72) SHA1(9ff6e2c5bd2b57bd607cb33e60e7ed25bea164b3) )
ROM_END


GAME( 198?, dwarfd, 0, dwarfd, dwarfd, 0, ROT0, "Electro-Sport", "Dwarfs Den", GAME_NO_SOUND|GAME_NOT_WORKING )

/***************************************************************************

    Mustache Boy
    (c)1987 March Electronics

    (there are also Seibu and Taito logos/copyrights in the ROMs)

 driver by Tomasz Slanina dox@space.pl

 The hardware similar to Knuckle Joe.

***************************************************************************/
#include "driver.h"
#include "sndhrdw/seibu.h" // for seibu_sound_decrypt on the MAIN cpu (not sound)
#include "sndhrdw/t5182.h"

WRITE8_HANDLER( mustache_videoram_w );
WRITE8_HANDLER( mustache_scroll_w );
WRITE8_HANDLER ( mustache_video_control_w);
VIDEO_START( mustache );
VIDEO_UPDATE( mustache );
PALETTE_INIT( mustache );


static READ8_HANDLER(t5182shared_r)
{
	return t5182_sharedram[offset];
}

static WRITE8_HANDLER(t5182shared_w)
{
	t5182_sharedram[offset] = data;
}

static int read_coins=0;

READ8_HANDLER ( mustache_coin_hack_r )
{
	return (read_coins)?((offset&1	)?(input_port_5_r(0)<<5)|(input_port_5_r(0)<<7):(input_port_5_r(0)<<4)):0;
}


static ADDRESS_MAP_START( memmap, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xcfff) AM_READWRITE(videoram_r, mustache_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(t5182_sound_irq_w)
	AM_RANGE(0xd001, 0xd001) AM_READ(t5182_sharedram_semaphore_snd_r)
	AM_RANGE(0xd002, 0xd002) AM_WRITE(t5182_sharedram_semaphore_main_acquire_w)
	AM_RANGE(0xd003, 0xd003) AM_WRITE(t5182_sharedram_semaphore_main_release_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(t5182shared_r, t5182shared_w)
	AM_RANGE(0xd800, 0xd800) AM_READ(input_port_0_r) /* IN 0 */
	AM_RANGE(0xd801, 0xd801) AM_READ(input_port_1_r) /* IN 1 */
	AM_RANGE(0xd802, 0xd802) AM_READ(input_port_2_r) /* IN 2 */
	AM_RANGE(0xd803, 0xd803) AM_READ(input_port_3_r)	/* DSW A */
	AM_RANGE(0xd804, 0xd804) AM_READ(input_port_4_r)	/* DSW B */
	AM_RANGE(0xd806, 0xd806) AM_WRITE(mustache_scroll_w)
	AM_RANGE(0xd807, 0xd807) AM_WRITE(mustache_video_control_w)
	AM_RANGE(0xe800, 0xefff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

/******************************************************************************/

INPUT_PORTS_START( mustache )
	PORT_START	/* IN 1 */

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN 2 */

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL

	PORT_START	/* IN 3 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0xf9, IP_ACTIVE_LOW, IPT_UNUSED  )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )

	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )

	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)

INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3),RGN_FRAC(2,3)},
	{STEP8(7,-1)},
	{STEP8(0,8)},
	8*8
};
static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(1,4), RGN_FRAC(3,4),RGN_FRAC(0,4),RGN_FRAC(2,4)},
	{STEP16(15,-1)},
	{STEP16(0,16)},
	16*16
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x00, 16 },
	{ REGION_GFX2, 0, &spritelayout, 0x80, 8 },
	{ -1 } /* end of array */
};


static void clear_irq_cb(int param)
{
    read_coins^=1;
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static void assert_irq(void)
{
	cpunum_set_input_line(0, 0, ASSERT_LINE);
	timer_set(TIME_IN_CYCLES(14288, 0),0, clear_irq_cb);
       /* Timing here is an educated GUESS, Z80 /INT must stay high so the irq
          fires no less than TWICE per frame, else game doesn't work right.
      6000000 / 56.747 = 105732.4616 cycles per frame, we'll call it A
      screen size is 256x256, though less is visible.
          lets assume we have 256 lines L and 40 'lines' (really line-times)
          of vblank V:
      So (A/(L+V))*V = the number of cycles spent in vblank.
      (105732.4616 / (256+40)) * 40 = 14288.17049 z80 clocks in vblank
       */
}


static MACHINE_DRIVER_START( mustache )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/4)
	MDRV_CPU_PROGRAM_MAP(memmap, 0)
    MDRV_CPU_VBLANK_INT(assert_irq,1)

	MDRV_CPU_ADD_TAG(CPUTAG_T5182,Z80, 14318180/4)
	MDRV_CPU_PROGRAM_MAP(t5182_map, 0)
	MDRV_CPU_IO_MAP(t5182_io, 0)

	MDRV_FRAMES_PER_SECOND(56.747)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8*16+16*8)

	MDRV_PALETTE_INIT(mustache)
	MDRV_VIDEO_START(mustache)
	MDRV_VIDEO_UPDATE(mustache)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 14318180/4)	/* 3.579545 MHz */
	MDRV_SOUND_CONFIG(t5182_ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 1.0)
	MDRV_SOUND_ROUTE(1, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( mustache )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "mustache.h18", 0x0000, 0x8000, CRC(123bd9b8) SHA1(33a7cba5c3a54b0b1a15dd1e24d298b6f7274321) )
	ROM_LOAD( "mustache.h16", 0x8000, 0x4000, CRC(62552beb) SHA1(ee10991d7de0596608fa1db48805781cbfbbdb9f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Toshiba T5182 module */
	ROM_LOAD( "t5182.rom",   0x0000, 0x2000, CRC(d354c8fc) SHA1(a1c9e1ac293f107f69cc5788cf6abc3db1646e33) )
	ROM_LOAD( "mustache.e5", 0x8000, 0x8000, CRC(efbb1943) SHA1(3320e9eaeb776d09ed63f7dedc79e720674e6718) )

	ROM_REGION( 0x0c000, REGION_GFX1,0)	/* BG tiles  */
	ROM_LOAD( "mustache.a13", 0x0000,  0x4000, CRC(9baee4a7) SHA1(31bcec838789462e67e54ebe7256db9fc4e51b69) )
	ROM_LOAD( "mustache.a14", 0x4000,  0x4000, CRC(8155387d) SHA1(5f0a394c7671442519a831b0eeeaba4eecd5a406) )
	ROM_LOAD( "mustache.a16", 0x8000,  0x4000, CRC(4db4448d) SHA1(50a94fd65c263d95fd24b4009dbb87707929fdcb) )

	ROM_REGION( 0x20000, REGION_GFX2,0 )	/* sprites */
	ROM_LOAD( "mustache.a4", 0x00000,  0x8000, CRC(d5c3bbbf) SHA1(914e3feea54246476701f492c31bd094ad9cea10) )
	ROM_LOAD( "mustache.a7", 0x08000,  0x8000, CRC(e2a6012d) SHA1(4e4cd1a186870c8a88924d5bff917c6889da953d) )
	ROM_LOAD( "mustache.a5", 0x10000,  0x8000, CRC(c975fb06) SHA1(4d166bd79e19c7cae422673de3e095ad8101e013) )
	ROM_LOAD( "mustache.a8", 0x18000,  0x8000, CRC(2e180ee4) SHA1(a5684a25c337aeb4effeda7982164d35bc190af9) )

	ROM_REGION( 0x1300, REGION_PROMS,0 )	/* proms */
	ROM_LOAD( "mustache.c3",0x0000, 0x0100, CRC(68575300) SHA1(bc93a38df91ad8c2f335f9bccc98b52376f9b483) )
	ROM_LOAD( "mustache.c2",0x0100, 0x0100, CRC(eb008d62) SHA1(a370fbd1affaa489210ea36eb9e365263fb4e232) )
	ROM_LOAD( "mustache.c1",0x0200, 0x0100, CRC(65da3604) SHA1(e4874d4152a57944d4e47306250833ea5cd0d89b) )

	ROM_LOAD( "mustache.b6",0x0300, 0x1000, CRC(5f83fa35) SHA1(cb13e63577762d818e5dcbb52b8a53f66e284e8f) ) /* 63S281N near SEI0070BU */
ROM_END

static DRIVER_INIT( mustache )
{
	int i;

	int G1 = memory_region_length(REGION_GFX1)/3;
	int G2 = memory_region_length(REGION_GFX2)/2;
	UINT8 *gfx1 = memory_region(REGION_GFX1);
	UINT8 *gfx2 = memory_region(REGION_GFX2);
	UINT8 *buf=malloc(G2*2);

	if (!buf)
		return;

	/* BG data lines */
	for (i=0;i<G1; i++)
	{
		UINT16 w;

		buf[i] = BITSWAP8(gfx1[i], 0,5,2,6,4,1,7,3);

		w = (gfx1[i+G1] << 8) | gfx1[i+G1*2];
		w = BITSWAP16(w, 14,1,13,5,9,2,10,6, 3,8,4,15,0,11,12,7);

		buf[i+G1]   = w >> 8;
		buf[i+G1*2] = w & 0xff;
	}

	/* BG address lines */
	for (i = 0; i < 3*G1; i++)
		gfx1[i] = buf[BITSWAP16(i,15,14,13,2,1,0,12,11,10,9,8,7,6,5,4,3)];

	/* SPR data lines */
	for (i=0;i<G2; i++)
	{
		UINT16 w;

		w = (gfx2[i] << 8) | gfx2[i+G2];
		w = BITSWAP16(w, 5,7,11,4,15,10,3,14, 9,2,13,8,1,12,0,6 );

		buf[i]    = w >> 8;
		buf[i+G2] = w & 0xff;
	}

	/* SPR address lines */
	for (i = 0; i < 2*G2; i++)
		gfx2[i] = buf[BITSWAP24(i,23,22,21,20,19,18,17,16,15,12,11,10,9,8,7,6,5,4,13,14,3,2,1,0)];

	free(buf);
	seibu_sound_decrypt(REGION_CPU1,0x8000);

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd400, 0xd401, 0, 0, mustache_coin_hack_r);
}


GAME( 1987, mustache, 0, mustache, mustache, mustache, ROT90, "[Seibu Kaihatsu] (March license)", "Mustache Boy", 0 )

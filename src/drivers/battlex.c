// license:BSD-3-Clause
// copyright-holders:David Haywood
/* battlex.c - by David Haywood

Stephh's notes :

  - I don't know exactly how to call the "Free Play" Dip Switch 8(
    It's effect is the following :
      * you need to insert at least one credit and start a game
      * when the game is over, you can start another games WITHOUT
        inserting another coins
    Note that the number of credits is decremented though.
    Credits are BCD coded on 3 bytes (0x000000-0x999999) at addresses
    0xa039 (LSB), 0xa03a and 0xa03b (MSB), but only the LSB is displayed.

   - Setting the flipscreen dip to ON also hides the copyright message (?)

TO DO :

  - missing starfield

  - game speed, its seems to be controlled by the IRQ's, how fast should it
    be? firing seems frustratingly inconsistant

  - background highlight colors still need verification.
*/

/*

Battle Cross (c)1982 Omori

CPU: Z80A
Sound: AY-3-8910
Other: 93419 (in socket marked 93219)

RAM: 4116(x12), 2114(x2), 2114(x6)
PROMS: none

XTAL: 10.0 MHz

*/


#include "driver.h"
#include "sound/ay8910.h"


extern WRITE8_HANDLER( battlex_palette_w );
extern WRITE8_HANDLER( battlex_videoram_w );
extern WRITE8_HANDLER( battlex_scroll_x_lsb_w );
extern WRITE8_HANDLER( battlex_scroll_x_msb_w );
extern WRITE8_HANDLER( battlex_flipscreen_w );


extern VIDEO_START( battlex );
extern VIDEO_START( dodgeman );
extern VIDEO_UPDATE( battlex );


/*** MEMORY & PORT READ / WRITE **********************************************/

static int battlex_irq_pending;

static INTERRUPT_GEN( battlex_interrupt )
{
	battlex_irq_pending = 1;
	cpunum_set_input_line(0, 0, ASSERT_LINE);
}

static READ8_HANDLER( battlex_dsw_r )
{
	int data = (readinputport(0) & ~0x10) | (battlex_irq_pending << 4);
	battlex_irq_pending = 0;
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	return data;
}

static MACHINE_RESET( battlex )
{
	battlex_irq_pending = 0;
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	battlex_scroll_x_lsb_w(0, 0);
	battlex_scroll_x_msb_w(0, 0);
	battlex_flipscreen_w(0, 0);
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA8_RAM) /* not read? */
	AM_RANGE(0x9000, 0x91ff) AM_READ(MRA8_RAM) /* not read? */
	AM_RANGE(0xa000, 0xa3ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe03f) AM_READ(MRA8_RAM) /* not read? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(battlex_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9000, 0x91ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)
	AM_RANGE(0xa000, 0xa3ff) AM_WRITE(MWA8_RAM) /* main */
	AM_RANGE(0xe000, 0xe03f) AM_WRITE(battlex_palette_w) /* probably palette */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(battlex_dsw_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(battlex_flipscreen_w)
	/* verify all of these */
	AM_RANGE(0x22, 0x22) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x23, 0x23) AM_WRITE(AY8910_control_port_0_w)

	/* 0x30 looks like scroll, but can't be ? changes (increases or decreases)
        depending on the direction your ship is facing on lev 2. at least */
	AM_RANGE(0x30, 0x30) AM_WRITE(MWA8_NOP)

	AM_RANGE(0x32, 0x32) AM_WRITE(battlex_scroll_x_lsb_w)
	AM_RANGE(0x33, 0x33) AM_WRITE(battlex_scroll_x_msb_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dodgeman_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(battlex_flipscreen_w)

	AM_RANGE(0x22, 0x22) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x23, 0x23) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x26, 0x26) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x27, 0x27) AM_WRITE(AY8910_control_port_1_w)


	AM_RANGE(0x30, 0x30) AM_WRITE(MWA8_NOP)

	AM_RANGE(0x32, 0x32) AM_WRITE(battlex_scroll_x_lsb_w)
	AM_RANGE(0x33, 0x33) AM_WRITE(battlex_scroll_x_msb_w)
ADDRESS_MAP_END


/*** INPUT PORTS *************************************************************/

INPUT_PORTS_START( battlex )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Allow_Continue ) )			// Not on 1st stage
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_IMPULSE(4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_IMPULSE(4) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("INPUTS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x40, "15000" )
	PORT_DIPSETTING(    0x60, "20000" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Free_Play ) )		// See notes
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( dodgeman )
	PORT_INCLUDE(battlex)
	PORT_MODIFY("SYSTEM")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
INPUT_PORTS_END

/*** GFX DECODE **************************************************************/


static const gfx_layout battlex_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4 },
	8*8*4
};


static const gfx_layout battlex_spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ 0,RGN_FRAC(1,3),RGN_FRAC(2,3) },
	{ 7,6,5,4,3,2,1,0,
		15,14,13,12,11,10,9,8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &battlex_charlayout,     64, 8 },
	{ REGION_GFX2, 0, &battlex_spritelayout,  0, 8 },
	{ -1 } /* end of array */
};

/*** MACHINE DRIVERS *********************************************************/

static MACHINE_DRIVER_START( battlex )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,10000000/4 )		 /* 10 MHz, divided ? (Z80A CPU) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_PERIODIC_INT(battlex_interrupt, TIME_IN_HZ(400))

	MDRV_MACHINE_RESET(battlex)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*8+64)

	MDRV_VIDEO_START(battlex)
	MDRV_VIDEO_UPDATE(battlex)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 10000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dodgeman )
	MDRV_IMPORT_FROM(battlex)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(readport,dodgeman_writeport)
	MDRV_VIDEO_START(dodgeman)
	MDRV_SOUND_ADD(AY8910, 10000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)
MACHINE_DRIVER_END

/*** ROM LOADING *************************************************************/

ROM_START( battlex )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p-rom1.6",    0x0000, 0x1000, CRC(b00ae551) SHA1(32a963fea23ea58fc3aab93cc814784a932f045e) )
	ROM_LOAD( "p-rom2.5",    0x1000, 0x1000, CRC(e765bb11) SHA1(99671e63f4c7d3d8754277451f0b35cba03b532d) )
	ROM_LOAD( "p-rom3.4",    0x2000, 0x1000, CRC(21675a91) SHA1(5bbd5b53b1a1b7aaed5d8c7b09b57f35e4a774dc) )
	ROM_LOAD( "p-rom4.3",    0x3000, 0x1000, CRC(fff1ccc4) SHA1(2cb9b096b30e441559e57992df8f30aee46b1f1c) )
	ROM_LOAD( "p-rom5.2",    0x4000, 0x1000, CRC(ceb63d38) SHA1(92cab905d009c59115f52172ba7d01c8ff8991d7) )
	ROM_LOAD( "p-rom6.1",    0x5000, 0x1000, CRC(6923f601) SHA1(e6c33cbd8d8679299d7b2c568d56f96ed3073971) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_ERASE00 )
	/* filled in later */
//  ROM_LOAD( "2732.e",    0x0000, 0x1000, CRC(126842b7) SHA1(2da4f64e077232c1dd0853d07d801f9781517850) )

	ROM_REGION( 0x3000, REGION_GFX2, 0 )
	ROM_LOAD( "2732.f",    0x0000, 0x1000, CRC(2b69287a) SHA1(30c0edaec44118b95ec390bd41c1bd49a2802451) )
	ROM_LOAD( "2732.h",    0x1000, 0x1000, CRC(9f4c3bdd) SHA1(e921ecafefe54c033d05d9cd289808e971ac7940) )
	ROM_LOAD( "2732.j",    0x2000, 0x1000, CRC(c1345b05) SHA1(17194c8ec961990222bd295ff1d036a64f497b0e) )

	ROM_REGION( 0x1000, REGION_USER1, 0 ) /* line colours? or bad? */
	ROM_LOAD( "2732.d",    0x0000, 0x1000, CRC(750a46ef) SHA1(b6ab93e084ab0b7c6ad90ee6431bc1b7ab9ed46d) )

	ROM_REGION( 0x1000, REGION_USER2, 0 )
	ROM_LOAD( "2732.e",    0x0000, 0x1000, CRC(126842b7) SHA1(2da4f64e077232c1dd0853d07d801f9781517850) )
ROM_END

static DRIVER_INIT( battlex )
{
	UINT8 *colormask = memory_region(REGION_USER1);
	UINT8 *gfxdata = memory_region(REGION_USER2);
	UINT8 *dest = memory_region(REGION_GFX1);
	int tile, line, bit, offset = 0;
	int tile_size = memory_region_length(REGION_GFX1) / 32;

	for (tile = 0; tile < tile_size; tile++)
		for (line = 0; line < 8; line++)
			for (bit = 0; bit < 8; bit++)
			{
				int color = colormask[(tile << 3) | ((line & 6) + (bit > 3 ? 1 : 0))];
				if (!(gfxdata[(tile << 3) | line] & (1 << bit)))
					color >>= 4;
				color &= 0x0f;
				if (offset & 1)
					dest[offset >> 1] |= color;
				else
					dest[offset >> 1] = color << 4;
				offset++;
			}
}

ROM_START( dodgeman )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "dg0.7f",       0x0000, 0x001000, CRC(1219b5db) SHA1(e4050a5e52f7b125f317b6e2ef615774c81cf679) )
	ROM_LOAD( "dg1.5f",       0x1000, 0x001000, CRC(fff9a086) SHA1(e4e528789d07755cf999054c191e242ea3ebe55f) )
	ROM_LOAD( "dg2.4f",       0x2000, 0x001000, CRC(2ca9ac99) SHA1(7fe81626ab2b5c01189fbb578999157863fcc29f) )
	ROM_LOAD( "dg3.3f",       0x3000, 0x001000, CRC(55a51c0e) SHA1(a5b253f096e1fe85ee391ad2aa0373809a3b48c2) )
	ROM_LOAD( "dg4.2f",       0x4000, 0x001000, CRC(14169361) SHA1(86d3cd1fa0aa4f21029daea2eba99bdaa34372e8) )
	ROM_LOAD( "dg5.1f",       0x5000, 0x001000, CRC(8f83ae2f) SHA1(daad41b61ba3d55531021d444bbe4acfc275cfc9) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_ERASE00 )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_ERASE00 )
	ROM_LOAD( "f.6f",         0x0000, 0x002000, CRC(dfaaf4c8) SHA1(1e09f1d72e7e5e6782d73ae60bca7982fc04df0e) )
	ROM_LOAD( "h.6h",         0x2000, 0x002000, CRC(e2525ffe) SHA1(a17b608b4089014be381b26f16597b83d4a66ebd) )
	ROM_LOAD( "j.6j",         0x4000, 0x002000, CRC(2731ee46) SHA1(15b9350e19f31b1cea99deb9935543777644e6a8) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "d.6d",         0x0000, 0x002000, CRC(451c1c3a) SHA1(214f775e242f7f29ac799cdd554708acddd1e34f) )

	ROM_REGION( 0x2000, REGION_USER2, 0 )
	ROM_LOAD( "e.6e",         0x0000, 0x002000, CRC(c9a515df) SHA1(5232d2d1bd02b89cb651d817995daf33469f0e2f) )
ROM_END

/*** GAME DRIVERS ************************************************************/

GAME( 1982, battlex, 0, battlex, battlex, battlex, ROT180, "Omori Electric Co., Ltd.", "Battle Cross", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_NO_COCKTAIL )
GAME( 1983, dodgeman, 0, dodgeman, dodgeman, battlex, ROT180, "Omori Electric Co., Ltd.", "Dodge Man", GAME_IMPERFECT_COLORS | GAME_NO_COCKTAIL )

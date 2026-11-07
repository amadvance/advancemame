// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

Irem M52 hardware

Moon Patrol memory map

driver by Nicola Salmoria

0000-3fff ROM
8000-83ff Video RAM
8400-87ff Color RAM
e000-e7ff RAM


read:
8800      protection
d000      IN0
d001      IN1
d002      IN2
d003      DSW1
d004      DSW2

write:
c800-c8ff sprites
d000      sound command
d001      flip screen

I/O ports
write:
10-1f     scroll registers
40        background #1 x position
60        background #1 y position
80        background #2 x position
a0        background #2 y position
c0        background control

NOTE: It may be possible to remove the fake port now that conditional DIPS
are supported. What should really be filled in for each mode?
***************************************************************************/

#include "driver.h"
#include "sndhrdw/irem.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"
#include "sound/discrete.h"



WRITE8_HANDLER( mpatrol_scroll_w );
WRITE8_HANDLER( mpatrol_bg1xpos_w );
WRITE8_HANDLER( mpatrol_bg1ypos_w );
WRITE8_HANDLER( mpatrol_bg2xpos_w );
WRITE8_HANDLER( mpatrol_bg2ypos_w );
WRITE8_HANDLER( mpatrol_bgcontrol_w );
WRITE8_HANDLER( mpatrol_flipscreen_w );
WRITE8_HANDLER( mpatrol_videoram_w );
WRITE8_HANDLER( mpatrol_colorram_w );

PALETTE_INIT( mpatrol );
VIDEO_START( mpatrol );
VIDEO_UPDATE( mpatrol );



/* this looks like some kind of protection. The game does strange things */
/* if a read from this address doesn't return the value it expects. */
static READ8_HANDLER( mpatrol_protection_r )
{
	return activecpu_get_reg(Z80_DE) & 0xff;
}


READ8_HANDLER( mpatrol_input_port_3_r )
{
	int ret = input_port_3_r(0);

	/* Based on the coin mode fill in the upper bits */
	if (input_port_4_r(0) & 0x04)
	{
		/* Mode 1 */
		ret	|= (input_port_5_r(0) << 4);
	}
	else
	{
		/* Mode 2 */
		ret	|= (input_port_5_r(0) & 0xf0);
	}

	return ret;
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8800, 0x8800) AM_READ(mpatrol_protection_r)
	AM_RANGE(0xd000, 0xd000) AM_READ(input_port_0_r)          /* IN0 */
	AM_RANGE(0xd001, 0xd001) AM_READ(input_port_1_r)          /* IN1 */
	AM_RANGE(0xd002, 0xd002) AM_READ(input_port_2_r)          /* IN2 */
	AM_RANGE(0xd003, 0xd003) AM_READ(mpatrol_input_port_3_r)  /* DSW1 */
	AM_RANGE(0xd004, 0xd004) AM_READ(input_port_4_r)          /* DSW2 */
	AM_RANGE(0xe000, 0xe7ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(mpatrol_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8400, 0x87ff) AM_WRITE(mpatrol_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xc800, 0xc8ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(irem_sound_cmd_w)
	AM_RANGE(0xd001, 0xd001) AM_WRITE(mpatrol_flipscreen_w)	/* + coin counters */
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END



static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x1f) AM_WRITE(mpatrol_scroll_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(mpatrol_bg1xpos_w)
	AM_RANGE(0x60, 0x60) AM_WRITE(mpatrol_bg1ypos_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(mpatrol_bg2xpos_w)
	AM_RANGE(0xa0, 0xa0) AM_WRITE(mpatrol_bg2ypos_w)
	AM_RANGE(0xc0, 0xc0) AM_WRITE(mpatrol_bgcontrol_w)
ADDRESS_MAP_END

#define MPATROL_COMMON\
	PORT_START_TAG("IN0")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )\
	/* coin input must be active for ? frames to be consistently recognized */\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_IMPULSE(17)\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_START_TAG("IN1")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )\
	PORT_START_TAG("IN2")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

#define MPATROL_COMMON2\
	PORT_START_TAG("DSW1")\
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )\
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )\
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )\
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )\
	PORT_DIPSETTING(    0x04, "Mode 1" )\
	PORT_DIPSETTING(    0x00, "Mode 2" )\
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )\
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	/* In stop mode, press 2 to stop and 1 to restart */\
	PORT_DIPNAME( 0x10, 0x10, "Stop Mode (Cheat)")\
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x20, 0x20, "Sector Selection (Cheat)")\
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability (Cheat)")\
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )\
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )\
	/* Fake port to support the two different coin modes */\
	PORT_START_TAG("FAKE")\
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage Mode 1" )   /* mapped on coin mode 1 */\
	PORT_DIPSETTING(    0x09, DEF_STR( 7C_1C ) )\
	PORT_DIPSETTING(    0x0a, DEF_STR( 6C_1C ) )\
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )\
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x0d, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )\
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_7C ) )\
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_8C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
 	PORT_DIPNAME( 0x30, 0x30, "Coin A  Mode 2" )   /* mapped on coin mode 2 */\
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B  Mode 2" )\
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

INPUT_PORTS_START( mpatrol )
MPATROL_COMMON

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "10000 30000 50000" )
	PORT_DIPSETTING(    0x08, "20000 40000 60000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Gets filled in based on the coin mode */

MPATROL_COMMON2
INPUT_PORTS_END

/* Identical to mpatrol, the only difference is the number of lives */
INPUT_PORTS_START( mpatrolw )
MPATROL_COMMON

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "10000 30000 50000" )
	PORT_DIPSETTING(    0x08, "20000 40000 60000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Gets filled in based on the coin mode */

MPATROL_COMMON2
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	2,      /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static const gfx_layout spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 128 sprites */
	2,      /* 2 bits per pixel */
	{ 0, 128*16*16 },       /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static const UINT32 bgcharlayout_xoffset[256] =
{
		0x000, 0x001, 0x002, 0x003, 0x008, 0x009, 0x00a, 0x00b,
		0x010, 0x011, 0x012, 0x013, 0x018, 0x019, 0x01a, 0x01b,
		0x020, 0x021, 0x022, 0x023, 0x028, 0x029, 0x02a, 0x02b,
		0x030, 0x031, 0x032, 0x033, 0x038, 0x039, 0x03a, 0x03b,
		0x040, 0x041, 0x042, 0x043, 0x048, 0x049, 0x04a, 0x04b,
		0x050, 0x051, 0x052, 0x053, 0x058, 0x059, 0x05a, 0x05b,
		0x060, 0x061, 0x062, 0x063, 0x068, 0x069, 0x06a, 0x06b,
		0x070, 0x071, 0x072, 0x073, 0x078, 0x079, 0x07a, 0x07b,
		0x080, 0x081, 0x082, 0x083, 0x088, 0x089, 0x08a, 0x08b,
		0x090, 0x091, 0x092, 0x093, 0x098, 0x099, 0x09a, 0x09b,
		0x0a0, 0x0a1, 0x0a2, 0x0a3, 0x0a8, 0x0a9, 0x0aa, 0x0ab,
		0x0b0, 0x0b1, 0x0b2, 0x0b3, 0x0b8, 0x0b9, 0x0ba, 0x0bb,
		0x0c0, 0x0c1, 0x0c2, 0x0c3, 0x0c8, 0x0c9, 0x0ca, 0x0cb,
		0x0d0, 0x0d1, 0x0d2, 0x0d3, 0x0d8, 0x0d9, 0x0da, 0x0db,
		0x0e0, 0x0e1, 0x0e2, 0x0e3, 0x0e8, 0x0e9, 0x0ea, 0x0eb,
		0x0f0, 0x0f1, 0x0f2, 0x0f3, 0x0f8, 0x0f9, 0x0fa, 0x0fb,
		0x100, 0x101, 0x102, 0x103, 0x108, 0x109, 0x10a, 0x10b,
		0x110, 0x111, 0x112, 0x113, 0x118, 0x119, 0x11a, 0x11b,
		0x120, 0x121, 0x122, 0x123, 0x128, 0x129, 0x12a, 0x12b,
		0x130, 0x131, 0x132, 0x133, 0x138, 0x139, 0x13a, 0x13b,
		0x140, 0x141, 0x142, 0x143, 0x148, 0x149, 0x14a, 0x14b,
		0x150, 0x151, 0x152, 0x153, 0x158, 0x159, 0x15a, 0x15b,
		0x160, 0x161, 0x162, 0x163, 0x168, 0x169, 0x16a, 0x16b,
		0x170, 0x171, 0x172, 0x173, 0x178, 0x179, 0x17a, 0x17b,
		0x180, 0x181, 0x182, 0x183, 0x188, 0x189, 0x18a, 0x18b,
		0x190, 0x191, 0x192, 0x193, 0x198, 0x199, 0x19a, 0x19b,
		0x1a0, 0x1a1, 0x1a2, 0x1a3, 0x1a8, 0x1a9, 0x1aa, 0x1ab,
		0x1b0, 0x1b1, 0x1b2, 0x1b3, 0x1b8, 0x1b9, 0x1ba, 0x1bb,
		0x1c0, 0x1c1, 0x1c2, 0x1c3, 0x1c8, 0x1c9, 0x1ca, 0x1cb,
		0x1d0, 0x1d1, 0x1d2, 0x1d3, 0x1d8, 0x1d9, 0x1da, 0x1db,
		0x1e0, 0x1e1, 0x1e2, 0x1e3, 0x1e8, 0x1e9, 0x1ea, 0x1eb,
		0x1f0, 0x1f1, 0x1f2, 0x1f3, 0x1f8, 0x1f9, 0x1fa, 0x1fb,
};

static const UINT32 bgcharlayout_yoffset[64] =
{
		0x0000, 0x0200, 0x0400, 0x0600, 0x0800, 0x0a00, 0x0c00, 0x0e00,
		0x1000, 0x1200, 0x1400, 0x1600, 0x1800, 0x1a00, 0x1c00, 0x1e00,
		0x2000, 0x2200, 0x2400, 0x2600, 0x2800, 0x2a00, 0x2c00, 0x2e00,
		0x3000, 0x3200, 0x3400, 0x3600, 0x3800, 0x3a00, 0x3c00, 0x3e00,
		0x4000, 0x4200, 0x4400, 0x4600, 0x4800, 0x4a00, 0x4c00, 0x4e00,
		0x5000, 0x5200, 0x5400, 0x5600, 0x5800, 0x5a00, 0x5c00, 0x5e00,
		0x6000, 0x6200, 0x6400, 0x6600, 0x6800, 0x6a00, 0x6c00, 0x6e00,
		0x7000, 0x7200, 0x7400, 0x7600, 0x7800, 0x7a00, 0x7c00, 0x7e00,
};

static const gfx_layout bgcharlayout =
{
	256, 64, /* 256x64 image format */
	1,       /* 1 image */
	2,       /* 2 bits per pixel */
	{ 4, 0 },       /* the two bitplanes for 4 pixels are packed into one byte */
	EXTENDED_XOFFS,
	EXTENDED_YOFFS,
	0x8000,
	bgcharlayout_xoffset,
	bgcharlayout_yoffset
};



static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,                0, 128 },
	{ REGION_GFX2, 0x0000, &spritelayout,          128*4,  16 },
	{ REGION_GFX3, 0x0000, &bgcharlayout, 128*4+16*4+0*4,   1 },
	{ REGION_GFX4, 0x0000, &bgcharlayout, 128*4+16*4+1*4,   1 },
	{ REGION_GFX5, 0x0000, &bgcharlayout, 128*4+16*4+2*4,   1 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( mpatrol )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)        /* 3.072 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(0,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(1790)	/* accurate frequency, measured on a real board, is 56.75Hz. */
				/* the Lode Runner manual (similar but different hardware) */
				/* talks about 55Hz and 1790ms vblank duration. */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 1*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512+32+32)
	MDRV_COLORTABLE_LENGTH(128*4+16*4+3*4)

	MDRV_PALETTE_INIT(mpatrol)
	MDRV_VIDEO_START(mpatrol)
	MDRV_VIDEO_UPDATE(mpatrol)

	/* sound hardware */
	MDRV_IMPORT_FROM(irem_audio)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mpatrol )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "mpa-1.3m",      0x0000, 0x1000, CRC(5873a860) SHA1(8c03726d6e049c3edbc277440184e31679f78258) )
	ROM_LOAD( "mpa-2.3l",      0x1000, 0x1000, CRC(f4b85974) SHA1(dfb2efb57378a20af6f20569f4360cde95596f93) )
	ROM_LOAD( "mpa-3.3k",      0x2000, 0x1000, CRC(2e1a598c) SHA1(112c3c9678db8a8540a8df3708020c87fd10c91b) )
	ROM_LOAD( "mpa-4.3j",      0x3000, 0x1000, CRC(dd05b587) SHA1(727961b0dafa4a96b580d51013336db2a18aff1e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for code */
	ROM_LOAD( "mp-s1.1a",     0xf000, 0x1000, CRC(561d3108) SHA1(4998c68a9e9a8002251fa8f07aa1082444a9dc80) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-5.3e",     0x0000, 0x1000, CRC(e3ee7f75) SHA1(b03d0d56150d3e9da4a4c871338097b4f450b649) )       /* chars */
	ROM_LOAD( "mpe-4.3f",     0x1000, 0x1000, CRC(cca6d023) SHA1(fecb3059fb09897a096add9452b50aec55c07545) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mpb-2.3m",     0x0000, 0x1000, CRC(707ace5e) SHA1(93c682e13e74bce29ced3a87bffb29569c114c3b) )       /* sprites */
	ROM_LOAD( "mpb-1.3n",     0x1000, 0x1000, CRC(9b72133a) SHA1(1393ef92ae1ad58a4b62ca1660c0793d30a8b5e2) )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-1.3l",     0x0000, 0x1000, CRC(c46a7f72) SHA1(8bb7c9acaf6833fb6c0575b015991b873a305a84) )       /* background graphics */

	ROM_REGION( 0x1000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-2.3k",     0x0000, 0x1000, CRC(c7aa1fb0) SHA1(14c6c76e1d0db2c0745e5d6d33ea6945fac8e9ee) )

	ROM_REGION( 0x1000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-3.3h",     0x0000, 0x1000, CRC(a0919392) SHA1(8a090cb8d483a3d67c7360058e3fdd70e151cd62) )

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "mpc-4.2a",     0x0000, 0x0200, CRC(07f99284) SHA1(dfc52958f2520e1ce4446dd4c84c91413bbacf76) )
	ROM_LOAD( "mpc-3.1m",     0x0200, 0x0020, CRC(6a57eff2) SHA1(2d1c12dab5915da2ccd466e39436c88be434d634) ) /* background palette */
	ROM_LOAD( "mpc-1.1f",     0x0220, 0x0020, CRC(26979b13) SHA1(8c41a8cce4f3384c392a9f7a223a50d7be0e14a5) ) /* sprite palette */
	ROM_LOAD( "mpc-2.2h",     0x0240, 0x0100, CRC(7ae4cd97) SHA1(bc0662fac82ffe65f02092d912b2c2b0c7a8ac2b) ) /* sprite lookup table */
ROM_END

ROM_START( mpatrolw )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "mpa-1w.3m",    0x0000, 0x1000, CRC(baa1a1d4) SHA1(7968a7f221e7f4c9c81ddc8de17f6568e17b9ea8) )
	ROM_LOAD( "mpa-2w.3l",    0x1000, 0x1000, CRC(52459e51) SHA1(ae685b7848baa1b87a3f2bce97356286171e16d4) )
	ROM_LOAD( "mpa-3w.3k",    0x2000, 0x1000, CRC(9b249fe5) SHA1(c01e0d572c4c163f3cf4b2aa9f4246427811b78d) )
	ROM_LOAD( "mpa-4w.3j",    0x3000, 0x1000, CRC(fee76972) SHA1(c3166b027f89f61964ead804d3c2da387454c4c2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for code */
	ROM_LOAD( "mp-s1.1a",     0xf000, 0x1000, CRC(561d3108) SHA1(4998c68a9e9a8002251fa8f07aa1082444a9dc80) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-5w.3e",    0x0000, 0x1000, CRC(f56e01fe) SHA1(93f582d63b9cd5c6dca207aa57b213c939cdda1d) )       /* chars */
	ROM_LOAD( "mpe-4w.3f",    0x1000, 0x1000, CRC(caaba2d9) SHA1(7016a26c2d01e3209749598e993cd8ce91f12c88) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mpb-2.3m",     0x0000, 0x1000, CRC(707ace5e) SHA1(93c682e13e74bce29ced3a87bffb29569c114c3b) )       /* sprites */
	ROM_LOAD( "mpb-1.3n",     0x1000, 0x1000, CRC(9b72133a) SHA1(1393ef92ae1ad58a4b62ca1660c0793d30a8b5e2) )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-1.3l",     0x0000, 0x1000, CRC(c46a7f72) SHA1(8bb7c9acaf6833fb6c0575b015991b873a305a84) )       /* background graphics */

	ROM_REGION( 0x1000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-2.3k",     0x0000, 0x1000, CRC(c7aa1fb0) SHA1(14c6c76e1d0db2c0745e5d6d33ea6945fac8e9ee) )

	ROM_REGION( 0x1000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "mpe-3.3h",     0x0000, 0x1000, CRC(a0919392) SHA1(8a090cb8d483a3d67c7360058e3fdd70e151cd62) )

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "mpc-4a.2a",    0x0000, 0x0200, CRC(cb0a5ff3) SHA1(d3f88b4e0c4858abac8b52105656ecece0cf4df9) )
	ROM_LOAD( "mpc-3.1m",     0x0200, 0x0020, CRC(6a57eff2) SHA1(2d1c12dab5915da2ccd466e39436c88be434d634) ) /* background palette */
	ROM_LOAD( "mpc-1.1f",     0x0220, 0x0020, CRC(26979b13) SHA1(8c41a8cce4f3384c392a9f7a223a50d7be0e14a5) ) /* sprite palette */
	ROM_LOAD( "mpc-2.2h",     0x0240, 0x0100, CRC(7ae4cd97) SHA1(bc0662fac82ffe65f02092d912b2c2b0c7a8ac2b) ) /* sprite lookup table */
ROM_END




WRITE8_HANDLER( alpha1v_scroll_w );
VIDEO_START( alpha1v );
VIDEO_UPDATE( alpha1v );

static WRITE8_HANDLER( alpha1v_flipscreen_w )
{
	flip_screen_set(data & 1);
}

static ADDRESS_MAP_START( alpha1v_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x6fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM AM_WRITE(mpatrol_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8400, 0x87ff) AM_RAM AM_WRITE(mpatrol_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x8800, 0x8fff) AM_READ(mpatrol_protection_r)
	AM_RANGE(0xc800, 0xc9ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xd000, 0xd000) AM_READ(input_port_0_r) AM_WRITE(irem_sound_cmd_w)
	AM_RANGE(0xd001, 0xd001) AM_READ(input_port_1_r) AM_WRITE(alpha1v_flipscreen_w)
	AM_RANGE(0xd002, 0xd002) AM_READ(input_port_2_r)
	AM_RANGE(0xd003, 0xd003) AM_READ(input_port_3_r)
	AM_RANGE(0xd004, 0xd004) AM_READ(input_port_4_r)
	AM_RANGE(0xe000, 0xefff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( alpha1v_ports, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x1f) AM_WRITE(alpha1v_scroll_w)
	AM_RANGE(0x40, 0x5f) AM_WRITE(mpatrol_bg1xpos_w)
	AM_RANGE(0x60, 0x7f) AM_WRITE(mpatrol_bg1ypos_w)
	AM_RANGE(0x80, 0x9f) AM_WRITE(mpatrol_bg2xpos_w)
	AM_RANGE(0xa0, 0xbf) AM_WRITE(mpatrol_bg2ypos_w)
	AM_RANGE(0xc0, 0xdf) AM_WRITE(mpatrol_bgcontrol_w)
ADDRESS_MAP_END

INPUT_PORTS_START( alpha1v )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_1C ) )



	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_1C ) )









	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )








	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_7C ) )
INPUT_PORTS_END



static const gfx_layout alpha1v_charlayout =
{
	8,8,512,2, { 0x1000*8, 0 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, 8*8
};

static const gfx_layout alpha1v_spritelayout =
{
	16,16,128,3, { 0x2000*8, 0, 0x1000*8 },
	{ 0,1,2,3,4,5,6,7,16*8,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },32*8
};

static const UINT32 alpha1v_bg_yoffset[128] =
{
	0x0, 0x200, 0x400, 0x600, 0x800, 0xa00, 0xc00, 0xe00,
	0x1000, 0x1200, 0x1400, 0x1600, 0x1800, 0x1a00, 0x1c00, 0x1e00,
	0x2000, 0x2200, 0x2400, 0x2600, 0x2800, 0x2a00, 0x2c00, 0x2e00,
	0x3000, 0x3200, 0x3400, 0x3600, 0x3800, 0x3a00, 0x3c00, 0x3e00,
	0x4000, 0x4200, 0x4400, 0x4600, 0x4800, 0x4a00, 0x4c00, 0x4e00,
	0x5000, 0x5200, 0x5400, 0x5600, 0x5800, 0x5a00, 0x5c00, 0x5e00,
	0x6000, 0x6200, 0x6400, 0x6600, 0x6800, 0x6a00, 0x6c00, 0x6e00,
	0x7000, 0x7200, 0x7400, 0x7600, 0x7800, 0x7a00, 0x7c00, 0x7e00,
	0x8000, 0x8200, 0x8400, 0x8600, 0x8800, 0x8a00, 0x8c00, 0x8e00,
	0x9000, 0x9200, 0x9400, 0x9600, 0x9800, 0x9a00, 0x9c00, 0x9e00,
	0xa000, 0xa200, 0xa400, 0xa600, 0xa800, 0xaa00, 0xac00, 0xae00,
	0xb000, 0xb200, 0xb400, 0xb600, 0xb800, 0xba00, 0xbc00, 0xbe00,
	0xc000, 0xc200, 0xc400, 0xc600, 0xc800, 0xca00, 0xcc00, 0xce00,
	0xd000, 0xd200, 0xd400, 0xd600, 0xd800, 0xda00, 0xdc00, 0xde00,
	0xe000, 0xe200, 0xe400, 0xe600, 0xe800, 0xea00, 0xec00, 0xee00,
	0xf000, 0xf200, 0xf400, 0xf600, 0xf800, 0xfa00, 0xfc00, 0xfe00,
};

static const gfx_layout alpha1v_bglayout =
{
	256,128,1,2,{ 4,0 },EXTENDED_XOFFS,EXTENDED_YOFFS,0x8000,
	bgcharlayout_xoffset,alpha1v_bg_yoffset
};

static const gfx_decode alpha1v_gfxdecode[] =
{
	{ REGION_GFX1, 0, &alpha1v_charlayout, 0, 128 },
	{ REGION_GFX2, 0, &alpha1v_spritelayout, 512, 16 },
	{ REGION_GFX3, 0, &alpha1v_bglayout, 768, 1 },
	{ REGION_GFX4, 0, &alpha1v_bglayout, 772, 1 },
	{ REGION_GFX5, 0, &alpha1v_bglayout, 776, 1 },
	{ -1 }
};

ADDRESS_MAP_EXTERN( irem_sound_readmem );
ADDRESS_MAP_EXTERN( irem_sound_readport );
ADDRESS_MAP_EXTERN( irem_sound_writeport );

static WRITE8_HANDLER( alpha1v_adpcm_w )
{
	if (offset & 1) MSM5205_data_w(0, data);
}

static WRITE8_HANDLER( alpha1v_sound_control_w )
{
	MSM5205_playmode_w(0, (data >> 2) & 7);
	MSM5205_reset_w(0, data & 1);
}

static void alpha1v_adpcm_int(int data)
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static const struct AY8910interface alpha1v_ay_interface =
{
	soundlatch_r, 0, 0, alpha1v_sound_control_w
};

static const struct MSM5205interface alpha1v_msm_interface =
{
	alpha1v_adpcm_int, MSM5205_S96_4B
};

static ADDRESS_MAP_START( alpha1v_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(alpha1v_adpcm_w)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(alpha1v_adpcm_w)
	AM_RANGE(0x1000, 0x1fff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x9000, 0x9fff) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static DISCRETE_SOUND_START( alpha1v_filter )
	DISCRETE_INPUT_STREAM(NODE_01)
	/* The output coupling capacitor removes the decoder's idle DC level. */
	DISCRETE_CRFILTER(NODE_02, 1, NODE_01, 560 + 330, CAP_U(1))
	DISCRETE_OUTPUT(NODE_02, 0.50)
DISCRETE_SOUND_END

static MACHINE_DRIVER_START( alpha1v )
	MDRV_CPU_ADD(Z80, 18432000/6)
	MDRV_CPU_PROGRAM_MAP(alpha1v_map, 0)
	MDRV_CPU_IO_MAP(alpha1v_ports, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)
	MDRV_FRAMES_PER_SECOND(18432000.0/3/384/282)
	MDRV_VBLANK_DURATION((int)(1000000.0*26*384/(18432000.0/3)))
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 282)
	MDRV_VISIBLE_AREA(136, 375, 16, 271)
	MDRV_GFXDECODE(alpha1v_gfxdecode)
	MDRV_PALETTE_LENGTH(512+32+32)
	MDRV_COLORTABLE_LENGTH(512+256+12)
	MDRV_PALETTE_INIT(mpatrol)
	MDRV_VIDEO_START(alpha1v)
	MDRV_VIDEO_UPDATE(alpha1v)
	MDRV_CPU_ADD(M6803, 3579545/4)
	MDRV_CPU_PROGRAM_MAP(irem_sound_readmem, alpha1v_sound_writemem)
	MDRV_CPU_IO_MAP(irem_sound_readport, irem_sound_writeport)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 3579545/4)
	MDRV_SOUND_CONFIG(alpha1v_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
	MDRV_SOUND_ADD(AY8910, 3579545/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
	/* This sound board has one ADPCM decoder; an idle second decoder adds DC. */
	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(alpha1v_msm_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "adpcm_filter", 1.0)
	MDRV_SOUND_ADD_TAG("adpcm_filter", DISCRETE, 0)
	MDRV_SOUND_CONFIG(alpha1v_filter)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START(alpha1v)
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("2-m3",  0x0000, 0x1000, CRC(3a679d34) SHA1(1a54a43070c56dc91d4d258f29e29613bb309f1c))
	ROM_LOAD("3-l3",  0x1000, 0x1000, CRC(2f09df64) SHA1(e91602e9e41ad24dd1d7f384ed81b9bdaadd03e1))
	ROM_LOAD("4-k3",  0x2000, 0x1000, CRC(64fb9c8a) SHA1(735fd00cc42193a417e6cde75f12b4cf2e804942))
	ROM_LOAD("5-j3",  0x3000, 0x1000, CRC(d1643d18) SHA1(7c794b82e17e2ba0a6237e3fc20d8314f6c2481c))
	ROM_LOAD("6-h3",  0x4000, 0x1000, CRC(cf34ab51) SHA1(3696da71e2bc7edd1ee7aeaac87be5386608c09e))
	ROM_LOAD("7-f3",  0x5000, 0x1000, CRC(99db9781) SHA1(a56a675cc4cbc9681bfe8052f51f19336eb2a0a6))
	ROM_LOAD("7a e3", 0x6000, 0x1000, CRC(3b0b4b0d) SHA1(0d8eea1e2db269943611289b3490a578ee347f85))

	ROM_REGION(0x10000, REGION_CPU2, 0)
	ROM_LOAD("1-a1", 0xf000, 0x1000, CRC(9e07fdd5) SHA1(ed4f462fcfe91fa8e88bfeaaba0a0c11fa0b4601))

	ROM_REGION(0x2000, REGION_GFX1, ROMREGION_DISPOSE)
	ROM_LOAD("13-f3", 0x0000, 0x1000, CRC(4b799229) SHA1(42cbdcf787b08b041d30504d699a12c378224933))
	ROM_LOAD("14-e3", 0x1000, 0x1000, CRC(cf00c737) SHA1(415e90289039cac4d04cb1d559f1378ca6a32132))

	ROM_REGION(0x3000, REGION_GFX2, ROMREGION_DISPOSE)
	ROM_LOAD("15-n3", 0x0000, 0x1000, CRC(dc26df76) SHA1(dd1cff7935f5559f9d1b440e02d5e5aa521b0054))
	ROM_LOAD("16-l3", 0x1000, 0x1000, CRC(39b9863b) SHA1(da9da9a1066188f050c422dfed1bbbd3ba612ccc))
	ROM_LOAD("17-k3", 0x2000, 0x1000, CRC(cfd90773) SHA1(052e126888b6de636db9c521a090699c282b620b))


	ROM_REGION(0x2000, REGION_GFX3, ROMREGION_ERASEFF)
	ROM_LOAD("11-k3",  0x0000, 0x1000, CRC(7659440a) SHA1(2efd27c82913513dd03e799f1ed3c10b0863677d))
	ROM_LOAD("12-jh3", 0x1000, 0x1000, CRC(7659440a) SHA1(2efd27c82913513dd03e799f1ed3c10b0863677d))

	ROM_REGION(0x2000, REGION_GFX4, ROMREGION_ERASEFF)
	ROM_LOAD("9-n3",   0x0000, 0x1000, CRC(0fdb7d13) SHA1(e828254a4f94df633d338b5772719276d41c6b7f))
	ROM_LOAD("10-lm3", 0x1000, 0x1000, CRC(9dde3a75) SHA1(293d093485be19bfb20685d76a08ac78e24062bf))

	ROM_REGION(0x2000, REGION_GFX5, ROMREGION_ERASEFF)


	ROM_REGION(0x0340, REGION_PROMS, 0)
	ROM_LOAD("63s481-a2", 0x0000, 0x0200, CRC(58678ea8) SHA1(b13a78a5bca8ad5bdec1293512b53654768a7a7a))
	ROM_LOAD("18s030-m1", 0x0200, 0x0020, CRC(6a57eff2) SHA1(2d1c12dab5915da2ccd466e39436c88be434d634))
	ROM_LOAD("mb7051-f1", 0x0220, 0x0020, CRC(d8bdd0df) SHA1(ca522428927911808214d319af314f601497ded4))
	ROM_LOAD("mb7052-h2", 0x0240, 0x0100, CRC(ce9f0ef9) SHA1(3afb94ed033f272983bbed22a59856df7824ef8a))
ROM_END


GAME( 1982, mpatrol,  0,       mpatrol, mpatrol,  0, ROT0, "Irem", "Moon Patrol", 0 )
GAME( 1982, mpatrolw, mpatrol, mpatrol, mpatrolw, 0, ROT0, "Irem (Williams license)", "Moon Patrol (Williams)", 0 )

GAME( 1988, alpha1v, 0, alpha1v, alpha1v, 0, ROT0, "Vision Electronics", "Alpha One (Vision Electronics)", 0 )

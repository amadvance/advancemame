/***************************************************************************

Grand Champion
(C) 1981 Taito

MAME driver by Ernesto Corvi and Phil Stroffolino

Known Issues:

-   PC3259 handling (for collision detection) is not accurate.
    Collision detection does work, but there are bits which define
    the type of collision (and determine whether a pit stop is
    required) that I don't know how to handle.

-   sound: missing engine noise

-   "radar" is probably wrong

-   LED and tachometer display are missing/faked
    Note that a dipswitch setting allows score to be displayed
    onscreen, but there's no equivalent for tachometer.

Notes:

-   The object of the game is to avoid the opposing cars.

-   The player has to drive through Dark Tunnels, Rain,
    Lightning, Sleet, Snow, and a Track that suddenly
    divides to get to the Finish Line first.

-   "GRAND CHAMPION" has a Radar Feature which enables
    the Player to see his position relative to the other
    cars.

-   A Rank Feature is also provided which shows the
    Player's numerical rank all through the race.

-   The Speech Feature enhances the game play.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "grchamp.h"

/***************************************************************************/

static UINT8 grchamp_led_data0;
static UINT8 grchamp_led_data1;
static UINT8 grchamp_led_data2;

UINT8 grchamp_led_reg[8][3];
/*  5 digits for score
**  2 digits for time
**  1 digit for rank (?)
*/

WRITE8_HANDLER( grchamp_led_data0_w )
{
	grchamp_led_data0 = data;
}
WRITE8_HANDLER( grchamp_led_data1_w )
{
	grchamp_led_data1 = data;
}
WRITE8_HANDLER( grchamp_led_data2_w )
{
	grchamp_led_data2 = data;
}
WRITE8_HANDLER( grchamp_led_data3_w )
{
	if( data )
	{
		int which = data&0x7;
		grchamp_led_reg[which][0] = grchamp_led_data0; /* digit */
		grchamp_led_reg[which][1] = grchamp_led_data1; /* ? */
		grchamp_led_reg[which][2] = grchamp_led_data2;
	}
}

/***************************************************************************/

static int PC3259_data;

WRITE8_HANDLER( PC3259_control_w )
{
	PC3259_data = data;
}

READ8_HANDLER( PC3259_0_r )
{ /* 0x01 (401a)*/
	return 0xff; /* unknown */
}

READ8_HANDLER( PC3259_1_r )
{ /* 0x09 (401b)*/
	return 0xff; /* unknown */
}

READ8_HANDLER( PC3259_2_r )
{ /* 0x11 (401c)*/
	int result = 0;
	if( grchamp_player_ypos<128 )
	{
		result |= 0x4; // crash on bottom half of screen
	}
	if( grchamp_collision&2 )
	{
		result = rand()&0xff; // OBJECT crash
	}
	return result;
}

READ8_HANDLER( PC3259_3_r )
{ /* 0x19 (401d)*/
	int result = grchamp_collision?1:0; /* crash */
	return result;
}

/***************************************************************************/


static WRITE8_HANDLER( NC_w )
{

}


static struct AY8910interface ay8910_interface_1 =
{
	0,
	0,
	grchamp_portA_0_w,
	grchamp_portB_0_w
};

static struct AY8910interface ay8910_interface_2 =
{
	0,
	0,
	grchamp_portA_1_w,
	grchamp_portB_1_w
};

static struct AY8910interface ay8910_interface_3 =
{
	0,
	0,
	grchamp_portA_2_w,
	grchamp_portB_2_w
};

/***************************************************************************/
static const gfx_layout char_layout =
{
	8,8,
	0x200,
	2,
	{ 0,0x1000*8 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tile_layout =
{
	8,8,
	0x300,
	4,
	{ 0,4,0x3000*8,0x3000*8+4 },
	{ 8*8+3,8*8+2,8*8+1,8*8+0,3,2,1,0 },
	{ 7*8,6*8,5*8,4*8,3*8,2*8,1*8,0*8 },
	16*8
};

static const gfx_layout player_layout =
{
	32,32,
	0x10,
	2,
	{ 0,4 },
	{
		0x000,0x001,0x002,0x003,0x008,0x009,0x00a,0x00b,
		0x010,0x011,0x012,0x013,0x018,0x019,0x01a,0x01b,
		0x020,0x021,0x022,0x023,0x028,0x029,0x02a,0x02b,
		0x030,0x031,0x032,0x033,0x038,0x039,0x03a,0x03b,
	},
	{
		0x40*0x00,0x40*0x01,0x40*0x02,0x40*0x03,
		0x40*0x04,0x40*0x05,0x40*0x06,0x40*0x07,
		0x40*0x08,0x40*0x09,0x40*0x0a,0x40*0x0b,
		0x40*0x0c,0x40*0x0d,0x40*0x0e,0x40*0x0f,
		0x40*0x10,0x40*0x11,0x40*0x12,0x40*0x13,
		0x40*0x14,0x40*0x15,0x40*0x16,0x40*0x17,
		0x40*0x18,0x40*0x19,0x40*0x1a,0x40*0x1b,
		0x40*0x1c,0x40*0x1d,0x40*0x1e,0x40*0x1f,
	},
	0x800
};

static const gfx_layout rain_layout =
{
	16,16,
	0x10,
	1,
	{ 0 },
	{ /* ? */
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	},
	{ /* ? */
		0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,
		8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16
	},
	0x100
};

static const gfx_layout sprite_layout =
{
	16,16,
	0x80,
	2,
	{ 0,0x1000*8 },
	{
		0,1,2,3,4,5,6,7,
		64+0,64+1,64+2,64+3,64+4,64+5,64+6,64+7
	},
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		128+0*8, 128+1*8, 128+2*8, 128+3*8, 128+4*8, 128+5*8, 128+6*8, 128+7*8
	},
	0x100
};


static const UINT32 headlights_layout_xoffset[64] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
};

static const UINT32 headlights_layout_yoffset[64] =
{
	0x0000,0x0040,0x0080,0x00c0,0x0100,0x0140,0x0180,0x01c0,
	0x0200,0x0240,0x0280,0x02c0,0x0300,0x0340,0x0380,0x03c0,
	0x0400,0x0440,0x0480,0x04c0,0x0500,0x0540,0x0580,0x05c0,
	0x0600,0x0640,0x0680,0x06c0,0x0700,0x0740,0x0780,0x07c0,
	0x0800,0x0840,0x0880,0x08c0,0x0900,0x0940,0x0980,0x09c0,
	0x0a00,0x0a40,0x0a80,0x0ac0,0x0b00,0x0b40,0x0b80,0x0bc0,
	0x0c00,0x0c40,0x0c80,0x0cc0,0x0d00,0x0d40,0x0d80,0x0dc0,
	0x0e00,0x0e40,0x0e80,0x0ec0,0x0f00,0x0f40,0x0f80,0x0fc0
};

static const gfx_layout headlights_layout =
{
	64,64,
	4, // Number of sprites
	1, // One bitplane
	{0},
	EXTENDED_XOFFS,
	EXTENDED_YOFFS,
	4096,
	headlights_layout_xoffset,
	headlights_layout_yoffset
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &char_layout,		0x20, 8 },
	{ REGION_GFX2, 0x0000, &tile_layout,		0x00, 16 },
	{ REGION_GFX1, 0x2000, &player_layout,		0x20, 32 },
	{ REGION_GFX1, 0x0000, &sprite_layout,		0x20, 32 },
	{ REGION_GFX3, 0x0000, &rain_layout,		0x20, 1 },
	{ REGION_GFX4, 0x0000, &headlights_layout,	0x00, 1 },
	{ -1 }
};

/***************************************************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_READ(MRA8_RAM) /* radar */
	AM_RANGE(0x5000, 0x53ff) AM_READ(MRA8_RAM) /* text layer */
	AM_RANGE(0x5800, 0x58ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
 	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(MWA8_RAM) AM_BASE(&grchamp_radar)
	AM_RANGE(0x5000, 0x53ff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram)
	AM_RANGE(0x5800, 0x583f) AM_WRITE(MWA8_RAM) AM_BASE(&colorram)
	AM_RANGE(0x5840, 0x58ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_3_r)		/* accel */
	AM_RANGE(0x01, 0x01) AM_READ(PC3259_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(grchamp_port_0_r)	/* comm */
	AM_RANGE(0x03, 0x03) AM_READ(input_port_4_r)		/* wheel */
	AM_RANGE(0x00, 0x03) AM_READ(grchamp_port_0_r)	/* scanline read, cpu2 read, etc */
	AM_RANGE(0x04, 0x04) AM_READ(input_port_0_r)		/* DSWA */
	AM_RANGE(0x05, 0x05) AM_READ(input_port_1_r)		/* DSWB */
	AM_RANGE(0x06, 0x06) AM_READ(input_port_2_r)		/* tilt, coin, reset HS, etc */
	AM_RANGE(0x09, 0x09) AM_READ(PC3259_1_r)
	AM_RANGE(0x11, 0x11) AM_READ(PC3259_2_r)
	AM_RANGE(0x19, 0x19) AM_READ(PC3259_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(grchamp_control0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(PC3259_control_w) // ?
	AM_RANGE(0x02, 0x02) AM_WRITE(grchamp_player_xpos_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(grchamp_player_ypos_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(grchamp_tile_select_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(MWA8_NOP) // NC according to schematics
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP) // NC according to schematics
	AM_RANGE(0x07, 0x07) AM_WRITE(grchamp_rain_xpos_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(grchamp_rain_ypos_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(grchamp_coinled_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(MWA8_NOP) // Unknown
	AM_RANGE(0x0b, 0x0b) AM_WRITE(MWA8_NOP) // NC according to schematics
	AM_RANGE(0x0c, 0x0c) AM_WRITE(MWA8_NOP) // NC according to schematics
	AM_RANGE(0x0d, 0x0d) AM_WRITE(MWA8_NOP) // Unknown, watchdog?
	AM_RANGE(0x0e, 0x0e) AM_WRITE(grchamp_sound_w)
	AM_RANGE(0x0f, 0x0f) AM_WRITE(MWA8_NOP) // NC according to schematics
	AM_RANGE(0x10, 0x13) AM_WRITE(grchamp_comm_w)
	AM_RANGE(0x20, 0x20) AM_WRITE(grchamp_led_data0_w)
	AM_RANGE(0x24, 0x24) AM_WRITE(grchamp_led_data1_w)
	AM_RANGE(0x28, 0x28) AM_WRITE(grchamp_led_data2_w)
	AM_RANGE(0x2c, 0x2c) AM_WRITE(grchamp_led_data3_w)
ADDRESS_MAP_END

/***************************************************************************/

static ADDRESS_MAP_START( readmem2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x37ff) AM_READ(MRA8_RAM) /* tilemaps */
	AM_RANGE(0x3800, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5000, 0x6fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem2, ADDRESS_SPACE_PROGRAM, 8 )
 	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x37ff) AM_WRITE(grchamp_videoram_w) AM_BASE(&grchamp_videoram)
	AM_RANGE(0x3800, 0x3fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(MWA8_RAM) /* working ram */
	AM_RANGE(0x5000, 0x6fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readport2, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x03) AM_READ(grchamp_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport2, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_WRITE(grchamp_port_1_w)
ADDRESS_MAP_END

/***************************************************************************/

static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4801, 0x4801) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x4803, 0x4803) AM_READ(AY8910_read_port_1_r)
	AM_RANGE(0x4805, 0x4805) AM_READ(AY8910_read_port_2_r)
	AM_RANGE(0x5000, 0x5000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
 	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4800) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x4801, 0x4801) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x4802, 0x4802) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x4803, 0x4803) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x4804, 0x4804) AM_WRITE(AY8910_control_port_2_w)
	AM_RANGE(0x4805, 0x4805) AM_WRITE(AY8910_write_port_2_w)
ADDRESS_MAP_END

/***************************************************************************/

static INTERRUPT_GEN( grchamp_interrupt )
{
	int cpu = cpu_getactivecpu();

	if ( grchamp_cpu_irq_enable[cpu] )
	{
		grchamp_cpu_irq_enable[cpu] = 0;
		cpunum_set_input_line(cpu, 0, HOLD_LINE);
	}
}

static MACHINE_DRIVER_START( grchamp )

	/* basic machine hardware */
	/* CPU BOARD */
	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(grchamp_interrupt,1)

	/* GAME BOARD */
	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(readmem2,writemem2)
	MDRV_CPU_IO_MAP(readport2,writeport2)
	MDRV_CPU_VBLANK_INT(grchamp_interrupt,1)	/* irq's are triggered from the main cpu */

	/* SOUND BOARD */
	MDRV_CPU_ADD(Z80, 3000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,TIME_IN_HZ(75))		/* irq's are triggered every 75 Hz */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 255-16)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x40+0x40*70)
	MDRV_COLORTABLE_LENGTH(0x40+0x40*70)

	MDRV_PALETTE_INIT(grchamp)
	MDRV_VIDEO_START(grchamp)
	MDRV_VIDEO_UPDATE(grchamp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.2)

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.2)

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface_3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.2)

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(grchamp_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

INPUT_PORTS_START( grchamp )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x02, "Extra Race" )
	PORT_DIPSETTING(    0x00, "4th" )
	PORT_DIPSETTING(    0x02, "5th" )
	PORT_DIPSETTING(    0x01, "6th" )
	PORT_DIPSETTING(    0x03, "7th" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x00, "RAM Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin System" )
	PORT_DIPSETTING(    0x10, "1 Way" )
	PORT_DIPSETTING(    0x00, "2 Way" )
	PORT_DIPNAME( 0x20, 0x00, "Display '1981'" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Display Score" )
	PORT_DIPSETTING(    0x00, "LEDs" )
	PORT_DIPSETTING(    0x40, "On Screen" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )	/* High Score reset switch */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_TOGGLE	/* High Gear */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )		/* Tilt */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* Service */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )		/* Coin A */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		/* Coin B */

	PORT_START /* Accel */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(16) PORT_CODE_DEC(KEYCODE_LCONTROL)
	//mask,default,type,sensitivity,delta,min,max

	PORT_START /* Wheel */
	PORT_BIT( 0xff, 0x40, IPT_DIAL ) PORT_MINMAX(0x00,0x7f) PORT_SENSITIVITY(25) PORT_KEYDELTA(5) PORT_REVERSE
	//mask,default,type,sensitivity,delta,min,max
INPUT_PORTS_END

ROM_START( grchamp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "gm03",   0x0000, 0x1000, CRC(47fda76e) SHA1(fd5f1a651481669d64e5e0799369c22472265535) )
	ROM_LOAD( "gm04",   0x1000, 0x1000, CRC(07a623dc) SHA1(bb8a6531d95e996148c06fd336db4054eb1d28dd) )
	ROM_LOAD( "gm05",	0x2000, 0x1000, CRC(716e1fba) SHA1(fe596873c932513227b982cd23af440d31612de9) )
	ROM_LOAD( "gm06",	0x3000, 0x1000, CRC(157db30b) SHA1(a74314d3aef4659ea96ed659e5db2883e7ae1cb1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "gm09",	0x0000, 0x1000, CRC(d57bd109) SHA1(d1cb5ba783eaceda45893f6404fe9dbac740a2de) )
	ROM_LOAD( "gm10",	0x1000, 0x1000, CRC(41ba07f1) SHA1(103eeacdd36b4347fc62debb6b5f4163083313f4) )
	ROM_LOAD( "gr16",	0x5000, 0x1000, CRC(885d708e) SHA1(d5d2978a0eeca167ec1fb9f6f981388de46fbf81) )
	ROM_LOAD( "gr15",	0x6000, 0x1000, CRC(a822430b) SHA1(4d29612489362d2dc3f3a9eab609902a50c34aff) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "gm07",	0x0000, 0x1000, CRC(65dcc572) SHA1(c9b19af365fa7ade2698be0bb892591ba281ecb0) )
	ROM_LOAD( "gm08",	0x1000, 0x1000, CRC(224d880c) SHA1(68aaaa0213d09cf34ba50c91d8c031d041f8a76f) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "gm01",	0x0000, 0x1000, CRC(846f8e89) SHA1(346bfd69268606fde27643b4d135b481536b73b1) )
	ROM_LOAD( "gm02",	0x1000, 0x1000, CRC(5911948d) SHA1(6f3a9a7f8d6a04b8e6d83756764c9c4185983d9b) )
	ROM_LOAD( "gr11",	0x2000, 0x1000, CRC(54eb3ec9) SHA1(22739240f53c708d8e53094d96916778e12beeed) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "gr20",	0x0000, 0x1000, CRC(88ba2c03) SHA1(4dfd136f122663223043c6cd79566f8eeec72681) )
	ROM_LOAD( "gr21",	0x1000, 0x1000, CRC(2f77a9f3) SHA1(9e20a776c5e8c7577c3e8467d4f8ac7ac909901f) )
	ROM_LOAD( "gr13",	0x2000, 0x1000, CRC(d5e19ebd) SHA1(d0ca553eec87619ec489f7ba6238f1fdde7c480b) )
	ROM_LOAD( "gr19",	0x3000, 0x1000, CRC(ff34b444) SHA1(51c67a1691da3a2d8ddcff5fd8fa816b1f9c60c0) )
	ROM_LOAD( "gr22",	0x4000, 0x1000, CRC(31bb5fc7) SHA1(9f638e632e7c72461bedecb710ac9b30f015eebf) )
	ROM_LOAD( "gr14",	0x5000, 0x1000, CRC(d129b8e4) SHA1(db25bfde2a48e14d38a43133d88d479c3cc1397a) )

	ROM_REGION( 0x0800, REGION_GFX3, ROMREGION_DISPOSE ) /* rain */
	ROM_LOAD( "gr10",	0x0000, 0x0800, CRC(b1f0a873) SHA1(f7ef1a16556ae3e7d70209bcb38ea3ae94208789) )

	ROM_REGION( 0x0800, REGION_GFX4, 0 ) /* headlights */
	ROM_LOAD( "gr12",	0x0000, 0x0800, CRC(f3bc599e) SHA1(3ec19584896a0bf10b9c5750f3c78ad3e722cc49) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "gr23.bpr", 0x00, 0x20, CRC(41c6c48d) SHA1(8bd14b5f02f9da0a68e3125955be18462b57401d) ) /* background colors */
	ROM_LOAD( "gr09.bpr", 0x20, 0x20, CRC(260fb2b9) SHA1(db0bf49f12a944613d113317d7dfea25bd7469fc) ) /* sprite/text colors */
ROM_END

/***************************************************************************/

GAME( 1981, grchamp, 0, grchamp, grchamp, grchamp, ROT90, "Taito", "Grand Champion", GAME_IMPERFECT_SOUND )

/************************************************************************

  Zaccaria S2650 '80s games

  Driver by Mike Coates and Pierpaolo Prazzoli

  TODO:

  Quasar
  ------
  - Sound (missing invader effect - still not sure all noise in correct places)
  - Phase 3 - seems awfully hard - dip settings ?


  - Make Asto Wars to boot
  - Make Cat'n Mouse to boot
    (it jumps to unpopulated rom, check jump at 0x2e62 and 0x4076)
  - ...

************************************************************************

Quasar by Zaccaria (1980)

1K files were 2708
2K files were 2716
512 file was an 82S130 (colour and priority PROM)

2650A CPU

I8085 Sound Board

************************************************************************

Zaccaria "Cat 'N Mouse" 1982

similar to "Quasar" execept it uses an 82s100 for color table lookup
and has a larger program prom


Cat N Mouse (Zaccaria 1982)

CPU Board

               2650    7b 6b 5b 3b 2b
                       7c 6c 5c 3c 2c

                       2636 2636 2636
        11g 10g 8g
     14l
                  clr

Sound Board 1b11107

6802
6821
8910

************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "cpu/i8039/i8039.h"
#include "sound/dac.h"
#include "vidhrdw/s2636.h"

PALETTE_INIT( quasar );
VIDEO_UPDATE( quasar );
VIDEO_START( quasar );

extern unsigned char *bullet_ram;

extern unsigned char *effectram;
extern unsigned char *effectdirty;
extern int			 effectcontrol;

static int page = 0;
static int IOpage = 8;


WRITE8_HANDLER( cvs_videoram_w );
WRITE8_HANDLER( cvs_bullet_w );
WRITE8_HANDLER( cvs_2636_1_w );
WRITE8_HANDLER( cvs_2636_2_w );
WRITE8_HANDLER( cvs_2636_3_w );
WRITE8_HANDLER( cvs_scroll_w );
WRITE8_HANDLER( cvs_video_fx_w );

READ8_HANDLER( cvs_collision_r );
READ8_HANDLER( cvs_collision_clear );
READ8_HANDLER( cvs_videoram_r );
READ8_HANDLER( cvs_bullet_r );
READ8_HANDLER( cvs_2636_1_r );
READ8_HANDLER( cvs_2636_2_r );
READ8_HANDLER( cvs_2636_3_r );
READ8_HANDLER( cvs_character_mode_r );

/************************************************************************

  Quasar memory layout

  Paging for screen is controlled by OUT to 0,1,2 or 3

  Paging for IO ports is controlled by OUT to 8,9,A or B

************************************************************************/

static WRITE8_HANDLER( page_0_w )
{
	page = 0;
}

static WRITE8_HANDLER( page_1_w )
{
	page = 1;
}

static WRITE8_HANDLER( page_2_w )
{
	page = 2;
}

static WRITE8_HANDLER( page_3_w )
{
	page = 3;
}

static WRITE8_HANDLER( page_8_w )
{
	IOpage = 8;
}

static WRITE8_HANDLER( page_9_w )
{
	IOpage = 9;
}

static WRITE8_HANDLER( page_A_w )
{
	IOpage = 10;
}

static WRITE8_HANDLER( page_B_w )
{
	IOpage = 11;
}

static WRITE8_HANDLER( quasar_video_w )
{
	if (page == 0) videoram_w(offset,data);
	if (page == 1) colorram_w(offset,(data & 7));	// 3 bits of ram only - 3 x 2102
	if (page == 2)
	{
		effectram[offset]   = data;
		effectdirty[offset] = 1;
	}
	if (page == 3)
	{
		effectcontrol = data;
		memset(effectdirty,1,sizeof(effectdirty));
	}
}

static READ8_HANDLER( quasar_IO_r )
{
	unsigned int ans = 0;

	if (IOpage == 8) ans = input_port_0_r(0);
	if (IOpage == 9) ans = input_port_1_r(0);
	if (IOpage == 10) ans = input_port_2_r(0);
	if (IOpage == 11) ans = input_port_3_r(0);

	return ans;
}

WRITE8_HANDLER( quasar_bullet_w )
{
	bullet_ram[offset] = (data ^ 0xff);
}

static int Quasar_T1=0;
static int Quasar_Command=0;
//static int sh_page=0;

WRITE8_HANDLER( quasar_sh_command_w )
{
	// bit 4 = Sound Invader : Linked to an NE555V circuit
	// Not handled yet

	// lower nibble = command to I8035
	// not necessarily like this, but it seems to work better than direct mapping
	// (although schematics has it as direct - but then the schematics are wrong elsewhere to!)
	Quasar_Command = (data & 8) + ((data >> 1) & 3) + ((data << 2) & 4);
	Quasar_T1      = (Quasar_Command != 15);
}

READ8_HANDLER( quasar_sh_command_r )
{
	// Clear T1 signal
	Quasar_T1 = 0;

	// Testing ...
	// return input_port_5_r(0);

	// Add in sound DIP switch
	return (Quasar_Command) + (input_port_5_r(0) & 0x30);
}

READ8_HANDLER( Quasar_T1_r )
{
	return Quasar_T1;
}

WRITE8_HANDLER( Quasar_DAC_w )
{
	DAC_0_signed_data_w(0,data);
}

// memory map taken from the manual

static ADDRESS_MAP_START( quasar, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_ROM
	AM_RANGE(0x1400, 0x14ff) AM_MIRROR(0x6000) AM_READWRITE(cvs_bullet_r, quasar_bullet_w) AM_BASE(&bullet_ram)
	AM_RANGE(0x1500, 0x15ff) AM_MIRROR(0x6000) AM_READWRITE(cvs_2636_1_r, cvs_2636_1_w) AM_BASE(&s2636_1_ram)
	AM_RANGE(0x1600, 0x16ff) AM_MIRROR(0x6000) AM_READWRITE(cvs_2636_2_r, cvs_2636_2_w) AM_BASE(&s2636_2_ram)
	AM_RANGE(0x1700, 0x17ff) AM_MIRROR(0x6000) AM_READWRITE(cvs_2636_3_r, cvs_2636_3_w) AM_BASE(&s2636_3_ram)
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0x6000) AM_READWRITE(MRA8_RAM /*quasar_video_r*/, quasar_video_w) AM_BASE(&videoram) AM_SIZE(&videoram_size) //0 = background, 1 = colour, 2 = effect, 3 = port blank
	AM_RANGE(0x1c00, 0x1fff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x2000, 0x33ff) AM_ROM
	AM_RANGE(0x4000, 0x53ff) AM_ROM
	AM_RANGE(0x6000, 0x73ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( quasar_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READWRITE(quasar_IO_r, page_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(page_1_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(page_2_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(page_3_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(page_8_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(page_9_w)
	AM_RANGE(0x0A, 0x0A) AM_WRITE(page_A_w)
	AM_RANGE(0x0B, 0x0B) AM_WRITE(page_B_w)
	AM_RANGE(S2650_DATA_PORT,  S2650_DATA_PORT) AM_READWRITE(cvs_collision_clear, quasar_sh_command_w)
	AM_RANGE(S2650_CTRL_PORT,  S2650_CTRL_PORT) AM_READWRITE(cvs_collision_r, MWA8_NOP)
	AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_4_r)
ADDRESS_MAP_END

/*************************************
 *
 *  Sound board memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_sound, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00    , 0x7F    ) AM_READ(MRA8_RAM)
	AM_RANGE(0x80    , 0x80    ) AM_READ(quasar_sh_command_r)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(Quasar_T1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_sound, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00    , 0x7F    ) AM_WRITE(MWA8_RAM)
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(Quasar_DAC_w)
ADDRESS_MAP_END

/************************************************************************

  Inputs

************************************************************************/

INPUT_PORTS_START( quasar )
	PORT_START	/* Controls 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE )			/* switch collaudo */

	PORT_START	/* Controls 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )			/* tavalino */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )				/* count enable */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coin_A ) )			/* confirmed */
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )			/* confirmed */
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x30, 0x00, "Number of Rockets" )			/* confirmed */
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Test Mode" )					/* confirmed */
	PORT_DIPSETTING(    0x00, "Collisions excluded" )
	PORT_DIPSETTING(    0x80, "Collisions included" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x01, "High Score" )
	PORT_DIPSETTING(    0x00, "No H.S." ) // this option only wants bit 0 OFF
	PORT_DIPSETTING(    0x01, "Normal H.S." )
	PORT_DIPSETTING(    0x03, "Low H.S." )
	PORT_DIPSETTING(    0x05, "Medium H.S." )
	PORT_DIPSETTING(    0x07, "High H.S." )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x08, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x60, 0x20, "Extended Play" )
	PORT_DIPSETTING(    0x20, "5500" )						/* confirmed */
	PORT_DIPSETTING(    0x40, "7500" )
	PORT_DIPSETTING(    0x60, "9500" )
	PORT_DIPSETTING(    0x00, "17500" )
	PORT_DIPNAME( 0x80, 0x80, "Full Screen Rocket" )		/* confirmed */
	PORT_DIPSETTING(    0x80, "Stop at edge" )
	PORT_DIPSETTING(    0x00, "Wrap Around" )

	PORT_START	/* SENSE */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	// Sound DIP switch
#if 0
	PORT_DIPNAME( 0x0f, 0x00, "Noise to play" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x01, "01" )
	PORT_DIPSETTING(    0x02, "02" )
	PORT_DIPSETTING(    0x03, "03" )
	PORT_DIPSETTING(    0x04, "04" )
	PORT_DIPSETTING(    0x05, "05" )
	PORT_DIPSETTING(    0x06, "06" )
	PORT_DIPSETTING(    0x07, "07" )
	PORT_DIPSETTING(    0x08, "08" )
	PORT_DIPSETTING(    0x09, "09" )
	PORT_DIPSETTING(    0x0a, "0A" )
	PORT_DIPSETTING(    0x0b, "0B" )
	PORT_DIPSETTING(    0x0c, "0C" )
	PORT_DIPSETTING(    0x0d, "0D" )
	PORT_DIPSETTING(    0x0e, "0E" )
	PORT_DIPSETTING(    0x0f, "0F" )
#endif
	PORT_DIPNAME( 0x10, 0x10, "Sound Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

/* S2636 Mappings */

static const gfx_layout s2636_character10 =
{
	8,10,
	5,
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	8*16
};

static const gfx_layout charlayout8colour =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,		/* 3 bits per pixel */
	{ 0, 0x800*8, 0x1000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,     0, 256},		/* Rom chars */
	{ REGION_GFX1, 0x0000, &charlayout8colour, 0, 259 },	/* Ram chars (NOT USED) */
	{ REGION_GFX1, 0x0000, &s2636_character10, 2072, 8 },	/* s2636 #1  */
	{ REGION_GFX1, 0x0000, &s2636_character10, 2072, 8 },	/* s2636 #2  */
	{ REGION_GFX1, 0x0000, &s2636_character10, 2072, 8 },	/* s2636 #3  */
	{ -1 } /* end of array */
};

static INTERRUPT_GEN( quasar_interrupt )
{
	cpunum_set_input_line_and_vector(0,0,PULSE_LINE,0x03);
}

// ************************(***************
// Quasar S2650 Main CPU, I8035 sound board
// ****************************************

static MACHINE_DRIVER_START( quasar )

	MDRV_CPU_ADD_TAG("main", S2650, 14318000/4)	/* 14 mhz crystal divide by 4 on board */
	MDRV_CPU_PROGRAM_MAP(quasar,0)
	MDRV_CPU_IO_MAP(quasar_io,0)
	MDRV_CPU_VBLANK_INT(quasar_interrupt,1)

	MDRV_CPU_ADD_TAG("sound",I8035,6000000/15)			/* 6MHz crystal divide by 15 in CPU */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_IO_MAP(readport_sound,writeport_sound)

	MDRV_FRAMES_PER_SECOND(50)							/* From dot clock */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(1*8+1, 29*8-1, 2*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_COLORTABLE_LENGTH(4096)

	MDRV_PALETTE_INIT(quasar)
	MDRV_VIDEO_START(quasar)
	MDRV_VIDEO_UPDATE(quasar)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( quasar )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "7b_01.bin",    0x0000, 0x0400, CRC(20a7feaf) SHA1(ab89087efca2fcb9568f49ba117755ae2c1bd3a3) )
	ROM_CONTINUE(			  0x4000, 0x0400 )
	ROM_LOAD( "6b_02.bin",    0x0400, 0x0400, CRC(c14af4a1) SHA1(ca2d3aff94db43aa7c25d33b345a53f484f679cd) )
	ROM_CONTINUE(			  0x4400, 0x0400 )
	ROM_LOAD( "5b_03.bin",    0x0800, 0x0400, CRC(3f051d8b) SHA1(1dd7a5eddfb0d7871705ac9ec1b9c16c2b80ddf0) )
	ROM_CONTINUE(			  0x4800, 0x0400 )
	ROM_LOAD( "3b_04.bin",    0x0c00, 0x0400, CRC(e14d04ed) SHA1(3176902e3efd72946468c7e7a221d88fcbf63c97) )
	ROM_CONTINUE(			  0x4c00, 0x0400 )
	ROM_LOAD( "2b_05.bin",    0x1000, 0x0400, CRC(f2113222) SHA1(576e0ac92ba076e00eeeae73892246f92fff252a) )
	ROM_CONTINUE(			  0x5000, 0x0400 )
	ROM_LOAD( "7c_06.bin",    0x2000, 0x0400, CRC(f7f1267d) SHA1(29c99191b0b6186af6772d04543a5fd235f5eafd) )
	ROM_LOAD( "6c_07.bin",    0x2400, 0x0400, CRC(772004eb) SHA1(bfafb6005a1a0cff39b76ec0ad4ea1f438a2f174) )
	ROM_LOAD( "5c_08.bin",    0x2800, 0x0400, CRC(7a87b6f3) SHA1(213b8ccd7bdd650e19d2746b2d617c1950ba3d2b) )
	ROM_LOAD( "3c_09.bin",    0x2c00, 0x0400, CRC(ef87c2cb) SHA1(1ba10dd3996c047e595c54a37c1abb44df3b63c6) )
	ROM_LOAD( "2c_10.bin",    0x3000, 0x0400, CRC(be6c4f84) SHA1(b3a779457bd0d33ccb23c21a7e7cd4a6fc78bb7f) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "quasar.snd",   0x0000, 0x0800, CRC(9e489768) SHA1(a9f01ef0a6512543bbdfec56037f37a0440b2b94) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6g.bin",       0x0000, 0x0800, CRC(deb2f4a9) SHA1(9a15d07a9b35bef34ce923973fc59fbd911f8111) )
	ROM_LOAD( "7g.bin",       0x0800, 0x0800, CRC(76222f30) SHA1(937286fcb50bd0a61db9e71e04b5eb1a0746e1c0) )
	ROM_LOAD( "9g.bin",       0x1000, 0x0800, CRC(fd0a36e9) SHA1(93f1207a36418b9ab15a25163a092308b9916528) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "12m_q.bin",    0x0000, 0x0200, CRC(1ab8633d) SHA1(3aed29f2326676a8d8a5de6f6bb923b6510896d8) )
ROM_END

ROM_START( quasara )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "7b_01.bin",    0x0000, 0x0400, CRC(20a7feaf) SHA1(ab89087efca2fcb9568f49ba117755ae2c1bd3a3) )
	ROM_CONTINUE(			  0x4000, 0x0400 )
	ROM_LOAD( "6b_02.bin",    0x0400, 0x0400, CRC(c14af4a1) SHA1(ca2d3aff94db43aa7c25d33b345a53f484f679cd) )
	ROM_CONTINUE(			  0x4400, 0x0400 )
	ROM_LOAD( "5b_03.bin",    0x0800, 0x0400, CRC(3f051d8b) SHA1(1dd7a5eddfb0d7871705ac9ec1b9c16c2b80ddf0) )
	ROM_CONTINUE(			  0x4800, 0x0400 )
	ROM_LOAD( "3b_04.bin",    0x0c00, 0x0400, CRC(e14d04ed) SHA1(3176902e3efd72946468c7e7a221d88fcbf63c97) )
	ROM_CONTINUE(			  0x4c00, 0x0400 )
	ROM_LOAD( "2b_05.bin",    0x1000, 0x0400, CRC(f2113222) SHA1(576e0ac92ba076e00eeeae73892246f92fff252a) )
	ROM_CONTINUE(			  0x5000, 0x0400 )
	ROM_LOAD( "7c_06.bin",    0x2000, 0x0400, CRC(f7f1267d) SHA1(29c99191b0b6186af6772d04543a5fd235f5eafd) )
	ROM_LOAD( "6c_07.bin",    0x2400, 0x0400, CRC(772004eb) SHA1(bfafb6005a1a0cff39b76ec0ad4ea1f438a2f174) )
	ROM_LOAD( "5c_08.bin",    0x2800, 0x0400, CRC(7a87b6f3) SHA1(213b8ccd7bdd650e19d2746b2d617c1950ba3d2b) )
	ROM_LOAD( "3c_09.bin",    0x2c00, 0x0400, CRC(ef87c2cb) SHA1(1ba10dd3996c047e595c54a37c1abb44df3b63c6) )
	ROM_LOAD( "2c_10a.bin",   0x3000, 0x0400, CRC(a31c0435) SHA1(48e1c5da455610145310dfe4c6b6e4302b531876) ) // different from quasar set

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "quasar.snd",   0x0000, 0x0800, CRC(9e489768) SHA1(a9f01ef0a6512543bbdfec56037f37a0440b2b94) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6g.bin",       0x0000, 0x0800, CRC(deb2f4a9) SHA1(9a15d07a9b35bef34ce923973fc59fbd911f8111) )
	ROM_LOAD( "7g.bin",       0x0800, 0x0800, CRC(76222f30) SHA1(937286fcb50bd0a61db9e71e04b5eb1a0746e1c0) )
	ROM_LOAD( "9g.bin",       0x1000, 0x0800, CRC(fd0a36e9) SHA1(93f1207a36418b9ab15a25163a092308b9916528) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "12m_q.bin",    0x0000, 0x0200, CRC(1ab8633d) SHA1(3aed29f2326676a8d8a5de6f6bb923b6510896d8) )
ROM_END

GAME( 1980, quasar,        0, quasar,   quasar,   0,		ROT90, "Zelco / Zaccaria",                         "Quasar",             GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1980, quasara,  quasar, quasar,   quasar,   0,		ROT90, "Zelco / Zaccaria",                         "Quasar (Alternate)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )


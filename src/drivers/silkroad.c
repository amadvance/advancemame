#include "driver.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

/* The Legend of Silk Road - Unico 1999 */

/* Preliminary Driver by David Haywood */
/* Inputs, DIPs by Stephh & R. Belmont */
/* and preliminary sound hookup by R. Belmont */
/* todo:

Clean Up

*/

/*

68020 interrupts
lev 1 : 0x64 : 0000 01d6 - just rte
lev 2 : 0x68 : 0000 01d6 - just rte
lev 3 : 0x6c : 0000 01d6 - just rte
lev 4 : 0x70 : 0000 012c - vblank?
lev 5 : 0x74 : 0000 01d6 - just rte
lev 6 : 0x78 : 0000 01d6 - just rte
lev 7 : 0x7c : 0000 01d6 - just rte

*/

/*

The Legend of Silk Road
Unico, 1999

PCB No.: SR2001A
CPU    : MC68EC020FG16 (68020, 100 pin PQFP)
SND    : YM2151, YM3012, OKI M6295 (x2) (Note: No sound CPU)
OSC    : 32.000MHz, 3.579545MHz
RAM    : 62256 (x8), 6116 (x2), KM681000BLG-7L (x4, SOP32, Surface-mounted)
Other  : 2x Actel A40MX04 (84 pin PLCC, same video chips as Multi Champ...ESD16.c)
         15x PALs

DIPs   : 8 position (x2)

Typed from sheet supplied with PCB (* = Default)

DIP SWA
            1   2   3   4   5   6   7   8
--------------------------------------------------------------------------------------
Lives       1   OFF
        2*  ON

Special     OFF     OFF
Effect      ON*     ON

Position 3-5    Not used        OFF OFF OFF

Difficulty  1                       OFF OFF ON
        2                       ON  OFF ON
        3                       OFF ON  ON
        4*                      ON  ON  ON
        5                       OFF OFF OFF
        6                       ON  OFF OFF
        7                       OFF ON  OFF
        8                       ON  ON  OFF
--------------------------------------------------------------------------------------


DIP SWB
            1   2   3   4   5   6   7   8
--------------------------------------------------------------------------------------
Position 1    Not Used  OFF

Freeplay    No*     OFF
        Yes     ON

Position 3  Not Used        OFF

Demo Sound  No              OFF
        Yes*                ON

Chute Type  Single*                 OFF
        Multi                   ON

Coin/Credit 1 Coin 1 Credit*                OFF OFF OFF
        1 Coin 2 Credit                 ON  OFF OFF
        1 Coin 3 Credit                 OFF ON  OFF
        1 Coin 4 Credit                 ON  ON  OFF
        2 Coin 1 Credit                 OFF OFF ON
        3 Coin 1 Credit                 ON  OFF ON
        4 Coin 1 Credit                 OFF ON  ON
        5 Coin 1 Credit                 ON  ON  ON
--------------------------------------------------------------------------------------

ROMs:
ROM00.BIN   32 pin 4M Mask, read as 27C4001         OKI Samples
ROM01.BIN   MX27C2000                   Sound Program

ROM02.BIN   42 pin 8M Mask, read as uPD27C8000        \
ROM03.BIN   42 pin 8M Mask, read as uPD27C8000        / Main Program

ROM04.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM \
ROM05.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM06.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM07.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM08.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM09.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM10.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |    GFX
ROM11.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM12.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM13.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM14.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM  |
ROM15.BIN       MX29F1610, SOP44 Surface Mounted Mask ROM /

*/

/* Stephh's notes :

     - Bit 7 of "system inputs" seems to freeze the inputs (and skip parts of code)
       when it is active :
         * at start, all inputs are read
         * when pressed for the 1st time, the inputs are NOT read and some code is skipped
         * when pressed for the 2nd time, the inputs are read and NO code is skipped
       Here are the routines where this bit is involved :
         * "initialization" at 0x000320
         * test at 0x00014e
     - Bit 7 of "misc inputs" freezes the game (code at 0x001570). VBLANK ?

     - 0xc00025 is read and written (code at 0x001724) but its effect is unknown
     - 0xc00031 is read and written (code at 0x001a58) but its effect is unknown

*/

UINT32 *silkroad_vidram,*silkroad_vidram2,*silkroad_vidram3, *silkroad_sprram, *silkroad_regs;

WRITE32_HANDLER( silkroad_fgram_w );
WRITE32_HANDLER( silkroad_fgram2_w );
WRITE32_HANDLER( silkroad_fgram3_w );
VIDEO_START(silkroad);
VIDEO_UPDATE(silkroad);

static WRITE32_HANDLER( paletteram32_xRRRRRGGGGGBBBBB_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	r = (paletteram32[offset] & 0x7c000000) >> (10+16);
	g = (paletteram32[offset] & 0x03e00000) >> (5+16);
	b = (paletteram32[offset] & 0x001f0000) >> (0+16);

	b = b << 3;
	r = r << 3;
	g = g << 3;

	palette_set_color(offset,r,g,b);
}

/* player inputs */
static READ32_HANDLER(io32_r)
{
	return ((readinputport(0) << 16) |  (readinputport(1) << 0));
}

/* dipswitches */
static READ32_HANDLER(io32_1_r)
{
	return readinputport(2)<<16;
}

/* sound I/O */

static READ32_HANDLER(silk_6295_0_r)
{
	return OKIM6295_status_0_r(0)<<16;
}

static WRITE32_HANDLER(silk_6295_0_w)
{
	if (!(mem_mask & 0x00ff0000))
	{
	//	logerror("OKI0: write %x mem_mask %8x\n", data>>16, mem_mask);
		OKIM6295_data_0_w(0, (data>>16) & 0xff);
	}
}

static READ32_HANDLER(silk_6295_1_r)
{
	return OKIM6295_status_1_r(0)<<16;
}

static WRITE32_HANDLER(silk_6295_1_w)
{
	if (!(mem_mask & 0x00ff0000))
	{
		//logerror("OKI1: write %x mem_mask %8x\n", data>>16, mem_mask);
		OKIM6295_data_1_w(0, (data>>16) & 0xff);
	}
}

static READ32_HANDLER(silk_ym_r)
{
	return YM2151_status_port_0_r(0)<<16;
}

static WRITE32_HANDLER(silk_ym_regport_w)
{
	if (!(mem_mask & 0x00ff0000))
	{
		YM2151_register_port_0_w(0, (data>>16) & 0xff);
	}
}

static WRITE32_HANDLER(silk_ym_dataport_w)
{
	if (!(mem_mask & 0x00ff0000))
	{
		YM2151_data_port_0_w(0, (data>>16) & 0xff);
	}
}

static WRITE32_HANDLER( silk_6295_bank_w )
{
	if (!(mem_mask & 0xff000000))
	{
		int bank = (data & 0x3000000) >> 24;
		if(bank < 3)
		OKIM6295_set_bank_base(0, (memory_region_length(REGION_SOUND1), 0x40000 * bank )); // correct.??
	}
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA32_ROM)
	AM_RANGE(0x40c000, 0x40cfff) AM_READ(MRA32_RAM)
	AM_RANGE(0x600000, 0x603fff) AM_READ(MRA32_RAM)

	AM_RANGE(0x800000, 0x803fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x804000, 0x807fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x808000, 0x80bfff) AM_READ(MRA32_RAM)

	AM_RANGE(0xC00000, 0xC00003) AM_READ(io32_r)	// player inputs
	AM_RANGE(0xC00004, 0xC00007) AM_READ(io32_1_r) // dip switches
	AM_RANGE(0xC00024, 0xC00027) AM_READ(silk_6295_0_r)
	AM_RANGE(0xC0002C, 0xC0002f) AM_READ(silk_ym_r)
	AM_RANGE(0xC00030, 0xC00033) AM_READ(silk_6295_1_r)

	AM_RANGE(0xfe0000, 0xffffff) AM_READ(MRA32_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x40c000, 0x40cfff) AM_WRITE(MWA32_RAM) AM_BASE(&silkroad_sprram) // sprites
	AM_RANGE(0x600000, 0x603fff) AM_WRITE(paletteram32_xRRRRRGGGGGBBBBB_dword_w) AM_BASE(&paletteram32) // palette

	AM_RANGE(0x800000, 0x803fff) AM_WRITE(silkroad_fgram_w) AM_BASE(&silkroad_vidram)  // lower Layer
	AM_RANGE(0x804000, 0x807fff) AM_WRITE(silkroad_fgram2_w) AM_BASE(&silkroad_vidram2)  // higher layer
	AM_RANGE(0x808000, 0x80bfff) AM_WRITE(silkroad_fgram3_w) AM_BASE(&silkroad_vidram3) // even higher layer


	AM_RANGE(0xC00024, 0xC00027) AM_WRITE(silk_6295_0_w)
	AM_RANGE(0xC00028, 0xC0002b) AM_WRITE(silk_ym_regport_w)
	AM_RANGE(0xC0002C, 0xC0002f) AM_WRITE(silk_ym_dataport_w)
	AM_RANGE(0xC00030, 0xC00033) AM_WRITE(silk_6295_1_w)
    AM_RANGE(0xc00034, 0xc00037) AM_WRITE(silk_6295_bank_w)
	// C00038 appears to be the coin counter, bit 0 is pulsed when a coin is inserted
/*
    AM_RANGE(0xC00034, 0xC00037) AM_WRITE(MWA32_NOP)
*/

	AM_RANGE(0xC0010c, 0xC00123) AM_WRITE(MWA32_RAM) AM_BASE(&silkroad_regs)
/*
    AM_RANGE(0xC0010C, 0xC0010f) AM_WRITE(MWA32_NOP) // 0
    AM_RANGE(0xC00110, 0xC00113) AM_WRITE(MWA32_NOP) // 1
    AM_RANGE(0xC00114, 0xC00117) AM_WRITE(MWA32_NOP) // 2

    AM_RANGE(0xC0011c, 0xC0011f) AM_WRITE(MWA32_NOP) // 4
    AM_RANGE(0xC00120, 0xC00123) AM_WRITE(MWA32_NOP) // 5
*/
	AM_RANGE(0xfe0000, 0xffffff) AM_WRITE(MWA32_RAM)
ADDRESS_MAP_END


INPUT_PORTS_START( silkroad )
	PORT_START	/* Players inputs */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)

	PORT_START	/* System inputs */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* Not mentioned in the "test mode" */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SPECIAL )	/* See notes - Stephh*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )	// this input makes the 020 lock up...- RB
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0002, 0x0000, "Special Effect" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unused DIP A-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unused DIP A-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unused DIP A-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00e0, 0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( Easiest ) )			// "1"
	PORT_DIPSETTING(      0x0040, DEF_STR( Easier ) )			// "2"
	PORT_DIPSETTING(      0x0020, DEF_STR( Easy ) )				// "3"
	PORT_DIPSETTING(      0x0000, DEF_STR( Normal ) )			// "4"
	PORT_DIPSETTING(      0x00e0, DEF_STR( Medium ) )			// "5"
	PORT_DIPSETTING(      0x00c0, DEF_STR( Hard ) )				// "6"
	PORT_DIPSETTING(      0x00a0, DEF_STR( Harder ) )			// "7"
	PORT_DIPSETTING(      0x0080, DEF_STR( Hardest ) )			// "8"
	PORT_DIPNAME( 0x0100, 0x0100, "Unused DIP B-0" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Unused DIP B-2" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Chute Type" )		    	// "Coin Box"
	PORT_DIPSETTING(      0x1000, DEF_STR( Single ) )			// "1"
	PORT_DIPSETTING(      0x0000, "Multi" )			            // "2"
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING( 0x0000, DEF_STR(5C_1C))
	PORT_DIPSETTING( 0x2000, DEF_STR(4C_1C))
	PORT_DIPSETTING( 0x4000, DEF_STR(3C_1C))
	PORT_DIPSETTING( 0x6000, DEF_STR(2C_1C))
	PORT_DIPSETTING( 0xe000, DEF_STR(1C_1C))
	PORT_DIPSETTING( 0xc000, DEF_STR(1C_2C))
	PORT_DIPSETTING( 0xa000, DEF_STR(1C_3C))
	PORT_DIPSETTING( 0x8000, DEF_STR(1C_4C))

//  PORT_START  /* Misc inputs */
//  PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* VBLANK ? */
//  PORT_BIT( 0xff7f, IP_ACTIVE_LOW, IPT_UNUSED ) /* unknown / unused */
INPUT_PORTS_END


/* BACKGROUNDS */
static const gfx_layout tiles16x16x6_layout =
{
	16,16,
	RGN_FRAC(1,3),
	6,
	{ 0x0000000*8+8,0x0000000*8+0,  0x0800000*8+8, 0x0800000*8+0, 0x1000000*8+8,0x1000000*8+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	16*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16x6_layout,  0x0000, 256 },
	{ -1 }
};

static MACHINE_DRIVER_START( silkroad )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 16000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(6*8+2, 64*8-1-(10*8)-2, 2*8, 32*8-1-(2*8))
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(silkroad)
	MDRV_VIDEO_UPDATE(silkroad)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 16000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
DRIVER_INIT( silkroad )
{

	/* why? rom04.bin looks like a bad dump, but it seems not since it was
       verified as correct... problem with the original which the gfx
       hardware didn't care about? */

	UINT8 *src = memory_region(REGION_GFX1)+0x1000000;
	int len = 0x0200000;
	unsigned char *buffer;

	int tileoffset = 0x1300*64; // verify

	src += tileoffset; len -=tileoffset;

	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i] = src[i-1];
		memcpy(src,buffer,len);
		free(buffer);
	}
}

ROM_START( silkroad )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_WORD_SWAP( "rom02.bin", 0x000000, 0x100000, CRC(4e5200fc) SHA1(4d4cab03a6ec4ad825001e1e92193940646141e5) )
	ROM_LOAD32_WORD_SWAP( "rom03.bin", 0x000002, 0x100000, CRC(73ccc78c) SHA1(2ac17aa8d7dac8636d29a4e4228a556334b51f1a) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	/* Sprites */
	ROM_LOAD( "rom12.bin",	0x0000000, 0x0200000, CRC(96393d04) SHA1(f512bb8603510d39e649f4ec1c5e2d0e4bf3a2cc) ) // 0
	ROM_LOAD( "rom08.bin",	0x0800000, 0x0200000, CRC(23f1d462) SHA1(6ca8052b16ccc1fe59716e03f66bd33af5145b37) ) // 0
	ROM_LOAD( "rom04.bin",	0x1000000, 0x0200000, CRC(2cf6ed30) SHA1(e96585cd109debc45960090d73b15db87e91ce0f) ) // 0

	ROM_LOAD( "rom13.bin",	0x0200000, 0x0200000, CRC(4ca1698e) SHA1(4fffc2f2a5fb434c42463ce904fd811866c53f81) ) // 1
	ROM_LOAD( "rom09.bin",	0x0a00000, 0x0200000, CRC(ef0b5bf4) SHA1(acd3bc5070de84608c5da0d091094382853cb048) ) // 1
	ROM_LOAD( "rom05.bin",	0x1200000, 0x0200000, CRC(512d6e25) SHA1(fc0a56663d77bbdfbd4242e14a55563073634582) ) // 1
	ROM_LOAD( "rom14.bin",	0x0400000, 0x0200000, CRC(d00b19c4) SHA1(d5b955dca5d0d251166a7f35a0bbbda6a91ecbd0) ) // 2
	ROM_LOAD( "rom10.bin",	0x0c00000, 0x0200000, CRC(7d324280) SHA1(cdf6d9342292f693cc5ec1b72816f2788963fcec) ) // 2
	ROM_LOAD( "rom06.bin",	0x1400000, 0x0200000, CRC(3ac26060) SHA1(98ad8efbbf8020daf7469db3e0fda02af6c4c767) ) // 2
	/* Backgrounds */
	ROM_LOAD( "rom07.bin",	0x0600000, 0x0200000, CRC(9fc6ff9d) SHA1(51c3ca9709a01e0ad6bc76c0d674ed03f9822598) ) // 3
	ROM_LOAD( "rom11.bin",	0x0e00000, 0x0200000, CRC(11abaf1c) SHA1(19e86f3ebfec518a96c0520f36cfc1b525e7e55c) ) // 3
	ROM_LOAD( "rom15.bin",	0x1600000, 0x0200000, CRC(26a3b168) SHA1(a4b7955cc4d4fbec7c975a9456f2219ef33f1166) ) // 3

	ROM_REGION( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "rom00.bin", 0x000000, 0x080000, CRC(b10ba7ab) SHA1(a6a3ae71b803af9c31d7e97dc86cfcc123ee9a40) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is what gets switched */
	ROM_REGION( 0xc0000, REGION_SOUND1, 0 ) /* Samples */
	ROM_COPY( REGION_USER1, 0x000000, 0x000000, 0x020000)
	ROM_COPY( REGION_USER1, 0x020000, 0x020000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x020000)
	ROM_COPY( REGION_USER1, 0x040000, 0x060000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x080000, 0x020000)
	ROM_COPY( REGION_USER1, 0x060000, 0x0a0000, 0x020000)

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )
	ROM_LOAD( "rom01.bin", 0x000000, 0x040000, CRC(db8cb455) SHA1(6723b4018208d554bd1bf1e0640b72d2f4f47302) )
ROM_END


GAME( 1999, silkroad, 0, silkroad, silkroad, silkroad, ROT0, "Unico", "The Legend of Silkroad", 0 )

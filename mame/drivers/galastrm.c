// license:BSD-3-Clause
// copyright-holders: Hau
/*

Galactic Storm
(c)1992 Taito

----------------------------------------------------------
MAIN PCB
CPU:MC68EC020RP25
TC0480SCP
TC0100SCN
TC0510NIO
TC0580PIV x2
TC0110PCR
TC0470LIN x2
TC0570SPC
TC0610
ADC0809CCN

OSC1:32MHz
OSC2:20MHz
----------------------------------------------------------
SOUND BOARD
CPU:MC68000P12F,MC68681P
ENSONIQ 5701,5510,5505

OSC1:16MHz
OSC2:30.47618MHz
----------------------------------------------------------
based on driver from taito/gunbustr.cpp by Bryan McPhail & David Graves
Written by Hau
07/03/2008


tips
$300.b debugmode
$305.b invincibility

TODO:
- device-ify TC0610? (no other known users)
- full screen rotation is incorrect in taito logo, end of stage, etc... (see https://youtu.be/lzPnO8Kej20)

*/

/*

Differences from MAME:
- The TC0610 rotation polygon is drawn with the legacy triangle rasterizer
  with integer vertices, instead of the poly manager.
- The EEPROM is initialized from the default data, instead of the ROM region.
- The sound board is the one used by gunbustr.

*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"
#include "machine/eeprom.h"
#include "sound/es5506.h"
#include "includes/taito_f3.h"

VIDEO_START( galastrm );
VIDEO_UPDATE( galastrm );

static UINT16 coin_word, frame_counter = 0;
static UINT32 *galastrm_ram;
static UINT16 *sound_ram;
extern INT16 galastrm_tc0610_ctrl_reg[2][8];

/*********************************************************************/

static void galastrm_interrupt6(int x)
{
	cpunum_set_input_line(0, 6, HOLD_LINE);
}

static INTERRUPT_GEN( galastrm_interrupt )
{
	frame_counter ^= 1;
	cpunum_set_input_line(0, 5, HOLD_LINE);
}

static int tc0110pcr_addr;
static int tc0610_0_addr;
static int tc0610_1_addr;

static WRITE32_HANDLER( galastrm_palette_w )
{
	if (ACCESSING_MSW32)
		tc0110pcr_addr = data >> 16;
	if (ACCESSING_LSW32 && tc0110pcr_addr < 4096)
		palette_set_color(tc0110pcr_addr, pal5bit(data >> 10), pal5bit(data >> 5), pal5bit(data >> 0));
}

static WRITE32_HANDLER( galastrm_tc0610_0_w )
{
	if (ACCESSING_MSW32)
		tc0610_0_addr = data >> 16;
	if (ACCESSING_LSW32 && tc0610_0_addr < 8)
		galastrm_tc0610_ctrl_reg[0][tc0610_0_addr] = data;
}

static WRITE32_HANDLER( galastrm_tc0610_1_w )
{
	if (ACCESSING_MSW32)
		tc0610_1_addr = data >> 16;
	if (ACCESSING_LSW32 && tc0610_1_addr < 8)
		galastrm_tc0610_ctrl_reg[1][tc0610_1_addr] = data;
}

static READ32_HANDLER( galastrm_input_r )
{
	switch (offset)
	{
		case 0x00:
			return (0xffff << 16) | input_port_0_word_r(0,0) | (EEPROM_read_bit() << 7) | (frame_counter << 9);

		case 0x01:
			return input_port_1_word_r(0,0) | (coin_word << 16);
	}

	return 0x0;
}

static WRITE32_HANDLER( galastrm_input_w )
{
	switch (offset)
	{
		case 0x00:
			if (ACCESSING_MSB32)	/* $400000 is watchdog */
				watchdog_reset_w(0,data >> 24);

			if (ACCESSING_LSB32)
			{
				EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
				EEPROM_write_bit(data & 0x40);
				EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
			}
			break;

		case 0x01:
			if (ACCESSING_MSB32)
			{
				coin_lockout_w(0, ~data & 0x01000000);
				coin_lockout_w(1, ~data & 0x02000000);
				coin_counter_w(0, data & 0x04000000);
				coin_counter_w(1, data & 0x04000000);
				coin_word = (data >> 16) & 0xffff;
			}
			break;
	}
}

static READ32_HANDLER( galastrm_adstick_ctrl_r )
{
	if (offset == 0x00)
	{
		if (ACCESSING_MSB32)
			return input_port_2_word_r(0,0) << 24;
		if ((mem_mask & 0x00ff0000) == 0)
			return input_port_3_word_r(0,0) << 16;
	}
	return 0;
}

static WRITE32_HANDLER( galastrm_adstick_ctrl_w )
{
	/* 1000 cycle delay is arbitrary */
	timer_set(TIME_IN_CYCLES(1000,0), 0, galastrm_interrupt6);
}

/***********************************************************
             MEMORY STRUCTURES
***********************************************************/

static ADDRESS_MAP_START( galastrm_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA32_ROM)
	AM_RANGE(0x200000, 0x21ffff) AM_READ(MRA32_RAM)	/* main CPUA ram */
	AM_RANGE(0x300000, 0x303fff) AM_READ(MRA32_RAM)	/* sprite ram */
	AM_RANGE(0x400000, 0x400007) AM_READ(galastrm_input_r)
	AM_RANGE(0x500000, 0x500007) AM_READ(galastrm_adstick_ctrl_r)
	AM_RANGE(0x600000, 0x6007ff) AM_READ(MRA32_RAM)	/* Sound shared ram */
	AM_RANGE(0x800000, 0x80ffff) AM_READ(TC0480SCP_long_r)
	AM_RANGE(0x830000, 0x83002f) AM_READ(TC0480SCP_ctrl_long_r)
	AM_RANGE(0xd00000, 0xd0ffff) AM_READ(TC0100SCN_long_r)	/* piv tilemaps */
	AM_RANGE(0xd20000, 0xd2000f) AM_READ(TC0100SCN_ctrl_long_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( galastrm_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x200000, 0x21ffff) AM_WRITE(MWA32_RAM) AM_BASE(&galastrm_ram)
	AM_RANGE(0x300000, 0x303fff) AM_WRITE(MWA32_RAM) AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x400000, 0x400007) AM_WRITE(galastrm_input_w)	/* eerom etc. */
	AM_RANGE(0x40fff0, 0x40fff3) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x500000, 0x500007) AM_WRITE(galastrm_adstick_ctrl_w)
	AM_RANGE(0x600000, 0x6007ff) AM_WRITE(MWA32_RAM) AM_BASE(&f3_shared_ram)
	AM_RANGE(0x800000, 0x80ffff) AM_WRITE(TC0480SCP_long_w)
	AM_RANGE(0x830000, 0x83002f) AM_WRITE(TC0480SCP_ctrl_long_w)
	AM_RANGE(0x900000, 0x900003) AM_WRITE(galastrm_palette_w)	/* TC0110PCR */
	AM_RANGE(0xb00000, 0xb00003) AM_WRITE(galastrm_tc0610_0_w)	/* TC0610 */
	AM_RANGE(0xc00000, 0xc00003) AM_WRITE(galastrm_tc0610_1_w)
	AM_RANGE(0xd00000, 0xd0ffff) AM_WRITE(TC0100SCN_long_w)	/* piv tilemaps */
	AM_RANGE(0xd20000, 0xd2000f) AM_WRITE(TC0100SCN_ctrl_long_w)
ADDRESS_MAP_END

/******************************************************************************/

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x140fff) AM_READ(f3_68000_share_r)
	AM_RANGE(0x200000, 0x20001f) AM_READ(ES5505_data_0_r)
	AM_RANGE(0x260000, 0x2601ff) AM_READ(es5510_dsp_r)
	AM_RANGE(0x280000, 0x28001f) AM_READ(f3_68681_r)
	AM_RANGE(0xc00000, 0xcfffff) AM_READ(MRA16_BANK1)
	AM_RANGE(0xff8000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_RAM) AM_BASE(&sound_ram)
	AM_RANGE(0x140000, 0x140fff) AM_WRITE(f3_68000_share_w)
	AM_RANGE(0x200000, 0x20001f) AM_WRITE(ES5505_data_0_w)
	AM_RANGE(0x260000, 0x2601ff) AM_WRITE(es5510_dsp_w)
	AM_RANGE(0x280000, 0x28001f) AM_WRITE(f3_68681_w)
	AM_RANGE(0x300000, 0x30003f) AM_WRITE(f3_es5505_bank_w)
	AM_RANGE(0x340000, 0x340003) AM_WRITE(f3_volume_w) /* 8 channel volume control */
	AM_RANGE(0xc00000, 0xcfffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0xff8000, 0xffffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

/***********************************************************
             INPUT PORTS (dips in eprom)
***********************************************************/

INPUT_PORTS_START( galastrm )
	PORT_START      /* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)	/* Freeze input */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* reserved for EEROM */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* frame counter */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2, STICKX */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(60) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START      /* IN3, STICKY */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(60) PORT_KEYDELTA(15) PORT_REVERSE PORT_PLAYER(1)
INPUT_PORTS_END

/***********************************************************
                GFX DECODING
**********************************************************/

static const gfx_layout tile16x16_layout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16	/* every sprite takes 128 consecutive bytes */
};

static const gfx_layout charlayout =
{
	16,16,    /* 16*16 characters */
	RGN_FRAC(1,1),
	4,        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 3*4, 2*4, 7*4, 6*4, 1*4, 0*4, 5*4, 4*4, 11*4, 10*4, 15*4, 14*4, 9*4, 8*4, 13*4, 12*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static const gfx_decode galastrm_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x16_layout, 0, 4096 },
	{ REGION_GFX1, 0x0, &charlayout,       0, 4096 },
	{ -1 } /* end of array */
};

/***********************************************************
                 MACHINE DRIVERS
***********************************************************/

static MACHINE_RESET( galastrm )
{
	/* Sound cpu program loads to 0xc00000 so we use a bank */
	UINT16 *ROM = (UINT16 *)memory_region(REGION_CPU2);
	memory_set_bankptr(1,&ROM[0x80000]);

	sound_ram[0]=ROM[0x80000]; /* Stack and Reset vectors */
	sound_ram[1]=ROM[0x80001];
	sound_ram[2]=ROM[0x80002];
	sound_ram[3]=ROM[0x80003];

	f3_68681_reset();
}

static struct ES5505interface es5505_interface =
{
	REGION_SOUND1,	/* Bank 0: Unused by F3 games? */
	REGION_SOUND1,	/* Bank 1: All games seem to use this */
	0				/* irq callback */
};

static UINT8 default_eeprom[128]={
	0x45,0x58,0x00,0x00,0xff,0xff,0x00,0x00,0x00,0x28,0x00,0x01,0x00,0x00,0x00,0xfa,
	0x00,0xec,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

static struct EEPROM_interface galastrm_eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* unlock command */
	"0100110000",	/* lock command */
};

static NVRAM_HANDLER( galastrm )
{
	if (read_or_write)
		EEPROM_save(file);
	else {
		EEPROM_init(&galastrm_eeprom_interface);
		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(default_eeprom,128);
	}
}

static MACHINE_DRIVER_START( galastrm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 16000000)	/* 16 MHz */
	MDRV_CPU_PROGRAM_MAP(galastrm_readmem,galastrm_writemem)
	MDRV_CPU_VBLANK_INT(galastrm_interrupt,1) /* VBL */

	MDRV_CPU_ADD(M68000, 16000000)
	/* audio CPU */	/* 16 MHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(galastrm)
	MDRV_NVRAM_HANDLER(galastrm)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(64*8, 50*8)
	MDRV_VISIBLE_AREA(0+96, 40*8-1+96, 3*8+60, 32*8-1+60)
	MDRV_GFXDECODE(galastrm_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(galastrm)
	MDRV_VIDEO_UPDATE(galastrm)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(ES5505, 30476100/2)
	MDRV_SOUND_CONFIG(es5505_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( galastrm )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "c99_15.ic105", 0x00000, 0x40000,  CRC(7eae8efd) SHA1(6bbb3da697dfcd93337b53895678e2a4ff2de457) )
	ROM_LOAD32_BYTE( "c99_12.ic102", 0x00001, 0x40000,  CRC(e059d1ee) SHA1(560951f95f270f0559b5289dda7f4ba74538cfcb) )
	ROM_LOAD32_BYTE( "c99_13.ic103", 0x00002, 0x40000,  CRC(885fcb35) SHA1(be10e109c461c1f776e98efa1b2a4d588aa0c41c) )
	ROM_LOAD32_BYTE( "c99_14.ic104", 0x00003, 0x40000,  CRC(457ef6b1) SHA1(06c2613d46addacd380a0f2413cd795b17ac9474) )

	ROM_REGION( 0x180000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "c99_23.ic8",  0x100000, 0x20000,  CRC(5718ee92) SHA1(33cfa60c5bceb1525498f27b598067d2dc620431) )
	ROM_LOAD16_BYTE( "c99_22.ic7",  0x100001, 0x20000,  CRC(b90f7c42) SHA1(e2fa9ee10ad61ae1a672c3357c0072b79ec7fbcb) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c99-06.ic2",  0x000000, 0x100000, CRC(812ed3ae) SHA1(775904dd42643d0e3a30890590d5f8eac1fe78db) )	/* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "c99-05.ic1",  0x000001, 0x100000, CRC(a91ffba4) SHA1(467af9646ddad5fbb520b6bc13517ed4deacf479) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "c99-02.ic50", 0x000000, 0x100000, CRC(81e9fc6f) SHA1(4495a7d130b755b5a48eaa814d884d6bb8243bcb) )	/* OBJ 16x16 tiles */
	ROM_LOAD32_BYTE( "c99-01.ic51", 0x000001, 0x100000, CRC(9dda1267) SHA1(c639ba064496dcadf5f1e55332a12bb442e9dc86) )
	ROM_LOAD32_BYTE( "c99-04.ic66", 0x000002, 0x100000, CRC(a681760f) SHA1(23d4fc7eb778c8a25c4bc7cee1d0c8cdd828a996) )
	ROM_LOAD32_BYTE( "c99-03.ic67", 0x000003, 0x100000, CRC(a2807a27) SHA1(977e395ea2ab2fb82807d3cf5fe5f1dbbde99da0) )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "c99-11.ic90",  0x00000,  0x80000, CRC(26a6926c) SHA1(918860e2829131e9ecfe983b2ae3e49e1c9ecd72) )	/* STY, spritemap */

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "c99-08.ic3",  0x000000, 0x100000, CRC(fedb4187) SHA1(83563e4af795a0dfeb261a62c31b6fed72f45a4d) )	/* Ensoniq samples */
	ROM_LOAD16_BYTE( "c99-09.ic4",  0x200000, 0x100000, CRC(ba70b86b) SHA1(ffbb9547d6b6e47a3ef23206b5f40c57f3ea7619) )
	ROM_LOAD16_BYTE( "c99-10.ic5",  0x400000, 0x100000, CRC(da016f1e) SHA1(581ef158c6f6576618dd75429b1d3aa92cd3581d) )
	ROM_LOAD16_BYTE( "c99-07.ic2",  0x680000, 0x040000, CRC(4cc3136f) SHA1(d9d7556bbe6af161fa0651b1fbd72e7dbf0a8e82) )
	ROM_CONTINUE( 0x600000, 0x040000 )
	ROM_CONTINUE( 0x780000, 0x040000 )
	ROM_CONTINUE( 0x700000, 0x040000 )
ROM_END

GAME( 1992, galastrm, 0, galastrm, galastrm, 0, ROT0, "Taito Corporation", "Galactic Storm (Japan)", GAME_IMPERFECT_GRAPHICS )

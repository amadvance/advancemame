/*----------------------------------------------------------------
   Psikyo PS3/PS5/PS5v2 SH-2 Based Systems
   driver by David Haywood (+ Paul Priest)
   thanks to Farfetch'd for information about the sprite zoom table.
------------------------------------------------------------------

Moving on from the 68020 based system used for the first Strikers
1945 game Psikyo introduced a system using Hitachi's SH-2 CPU

This driver is for the single-screen PS3/PS5/PS5v2 boards

There appear to be multiple revisions of this board

 Board PS3-V1 (Custom Chip PS6406B)
 -----------------------------------
 Sol Divide (c)1997
 Strikers 1945 II (c)1997
 Space Bomber Ver.B (c)1998
 Daraku Tenshi - The Fallen Angels (c)1998

 Board PS5 (Custom Chip PS6406B)
 -------------------------------
 Gunbird 2 (c)1998
 Strikers 1999 / Strikers 1945 III (c)1999

 The PS5 board appears to just have a different memory map to PS3
 Otherwise identical.

 Board PS5V2 (Custom Chip PS6406B)
 ---------------------------------
 Dragon Blaze (c)2000
 Tetris The Grand Master 2 (c)2000
 Tetris The Grand Master 2 Plus (c)2000 (Confirmed by Japump to be a Dragon Blaze upgraded board)
 GunBarich (c)2001 (Appears to be a Dragon Blaze upgraded board, easily replaced chips have been swapped)
 Mahjong G-Taste (c)2002

 The PS5v2 board is only different physically.

All the boards have

YMF278B-F (80 pin PQFP) & YAC513 (16 pin SOIC)
( YMF278B-F is OPL4 == OPL3 plus a sample playback engine. )

93C56 EEPROM
( 93c56 is a 93c46 with double the address space. )

To Do:

  - see notes in vidhrdw file -

  Sol Divid's music is not correct, related to sh-2 timers.


*-----------------------------------*
|         Tips and Tricks           |
*-----------------------------------*

Hold Button during booting to test roms (Checksum 16-bit, on Words for gfx and Bytes for sound) for:

Daraku:           PL1 Button 1 (passes, doesn't test sound)
Space Bomber:     PL1 Start (passes all, only if bit 0x40 is set. But then EEPROM resets?)
Gunbird 2:        PL1 Start (passes all, only if bit 0x40 is set. But then EEPROM resets)
Strikers 1945III: PL1 Start (passes all, only if bit 0x40 is set)
Dragon Blaze:     PL1 Start (passes all, only if bit 0x40 is set)
Gunbarich:        PL1 Start (passes all, only if bit 0x40 is set)


Hold PL1 Button 1 and Test Mode button to get Maintenance mode for:

Space Bomber, Strikers 1945 II, Sol Divide, Daraku
(this works for earlier Psikyo games as well)

--- Space Bomber ---

Keywords, what are these for???, you earn them when you complete the game
with different points.:

DOG-1
CAT-2
BUTA-3
KAME-4
IKA-5
RABBIT-6
FROG-7
TAKO-8

--- Gunbird 2 ---

5-2-0-4-8 Maintenance Mode
5-3-5-7-3 All Data Initialised

[Aine]
5-1-0-2-4 Secret Command Enabled ["Down" on ?]
5-3-7-6-5 Secret Random Enabled
5-3-1-5-7 Secret All Disabled

--- Strikers 1945 III / S1999 ---

8-1-6-5-0 Maintenance Mode
8-1-6-1-0 All Data Initialised
1-2-3-4-5 Best Score Erased

[X-36]
0-1-9-9-9 Secret Command Enabled ["Up" on ?]
8-1-6-3-0 Secret Random Enabled
8-1-6-2-0 Secret All Disabled

--- Dragon Blaze ---

9-2-2-2-0 Maintenance Mode
9-2-2-1-0 All Data Initialised
1-2-3-4-5 Best Score Erased

--- Gunbarich ---

0-2-9-2-0 Maintainance Mode
0-2-9-1-0 All Data Initialised
1-2-3-4-5 Best Score Erased

--- Tetris The Grand Master 2 / TGM2+ ---

4-1-5-7-3 All Data Initialised
4-1-7-6-5 Best Score Erased

The following 4 are also tested for, but appear to be disabled:
1-3-5-7-9
0-2-4-6-8
4-1-3-7-3
5-0-2-1-3

----------------------------------------------------------------*/

/*

Psikyo PS3-V1 hardware readme
-----------------------------

Strikers 1945 II
Sol Divid
Daraku
Space Bomber

PCB Layout
----------

PS3-V1
|-------------------------------------------------|
|HA13118         3771    PROG_L                   |
|    VOL  YAC513         PROG_H      |-----|      |
|      JRC4741                       | SH2 |      |
|                          *U16      |     |      |
|     YMF278B  SOUND.U32             |-----|      |
|                                                 |
|J                                                |
|A                57.2727MHz   HY514260   HY514260|
|M                                                |
|M                 |-------|                      |
|A                 |PSIKYO |                      |
|                  |PS6406B|        *4L.10  0L.4  |
|          62256   |       |                      |
|93C56     62256   |-------|        *5L.9   1L.3  |
|JP5                                              |
|   *6H.37  *4H.31   2H.20   0H.13  *6L.8   2L.2  |
|                                                 |
|                                   *7L.7   3L.1  |
|   *7H.36  *5H.30   3H.19   1H.12                |
|-------------------------------------------------|
Notes:
      JP5 - hardwired jumper bank (x4) for region selection. Cut 2ND jumper from left
            for International/English region. All jumpers shorted = Japan region (default)
      SH2 - Hitachi HD6417604F28 SH-2 CPU, clock input 28.63635 [57.2727/2] (QFP144)
      YMF278B - Yamaha YMF278B OPL4 sound chip, clock input 28.63635MHz [57.2727/2] (QFP80)
      PROG_H/PROG_L - 27C4096 DIP40 EPROM
      All other ROMs - 32M SOP44 MaskROM
      * - ROM locations not populated
      VSync - 60Hz
      HSync - 15.27kHz


Psikyo PS5 hardware readme
--------------------------

Gunbird 2
Strikers 1945 III / Strikers 1999

PCB Layout
----------

PS5
|-------------------------------------------------|
|HA13118      M514260      PROG_L.U16    DATA.U1  |
| VOL    3771 M514260      PROG_H.U17             |
|                                    *PROG_DATA.U2|
|                          PAL                    |
|                                                 |
|   JRC4741                          0H.10   0L.3 |
|J          |-----|                               |
|A          | SH2 |      57.2727MHz  1H.11   1L.4 |
|M  YAC513  |     |                               |
|M          |-----|       |-------|  2H.12   2L.5 |
|A                        |PSIKYO |               |
|                         |PS6406B|  3H.13   3L.6 |
|                         |       |               |
|                         |-------| *4H.14  *4L.7 |
|                                                 |
|                           62256   *5H.15  *5L.8 |
|                           62256                 |
|                   93C56                         |
|            JP4             YMF278B    SOUND.9   |
|-------------------------------------------------|
Notes:
      JP4 - hardwired jumper bank (x4) for region selection. Cut leftmost jumper
            for International/English region. All jumpers shorted = Japan region (default)
      SH2 - Hitachi HD6417604F28 SH-2 CPU, clock input 28.63635 [57.2727/2] (QFP144)
      YMF278B - Yamaha YMF278B OPL4 sound chip, clock input 28.63635MHz [57.2727/2] (QFP80)
      PAL - AMD PALCE 16V8H stamped 'PS5-1' (DIP20)
      PROG_H/PROG_L/DATA - 27C4096 DIP40 EPROM
      All other ROMs - 64M/32M SOP44 MaskROM
      * - ROM locations not populated
      VSync - 60Hz
      HSync - 15.27kHz


Psikyo PS5V2 hardware readme
----------------------------

Dragon Blaze, Psikyo, 2000
Gunbarich, Psikyo, 2001
Tetris The Grand Master 2 , Psikyo, 2000
Tetris The Grand Master 2+, Psikyo, 2000

PCB Layout
----------

PS5V2
|----------------------------------------------------|
|HA13118  PS5-1  SM81C256  PROG_H.U21 *0H.U11 *0L.U3 |
|VOL  JRC4741    SM81C256  PROG_L.U22                |
|     YAC516  3771                    *1H.U12 *1L.U4 |
|              |-----|          *U23                 |
|              | SH2 |                *2H.U13 *2L.U5 |
|              |     |                               |
|J    YMF278B  |-----|                 3H.U14  3L.U6 |
|A                          57.2727MHz               |
|M                                     4H.U15  4L.U7 |
|M    SND.U52               |-------|                |
|A                          |PSIKYO |  5H.U16  5L.U8 |
|                           |PS6406B|                |
|                           |       | *6H.U17 *6L.U9 |
|                    62256  |-------|                |
|         JP3 93C56  62256            *7H.U18 *7L.U10|
|                                                    |
|  10L.U58    9L.U41    8L.U28    7L.U19     6L.U1   |
|                                                    |
|  10H.U59    9H.U42    8H.U29    7H.U20     6H.U2   |
|----------------------------------------------------|
Notes:
      JP3     - hardwired jumper bank (x4) for region selection. Cut rightmost jumper
                for International/English region. All jumpers shorted = Japan region (default)
      SH2     - Hitachi HD6417604F28 SH-2 CPU, clock input 28.63635 [57.2727/2] (QFP144)
      YMF278B - Yamaha YMF278B OPL4 sound chip, clock input 28.63635MHz [57.2727/2] (QFP80)
      PROG_H/PROG_L - 27C4096 DIP40 EPROM
      ROMs U1-U59 (at bottom of PCB) - 16M DIP42 MaskROM
      ROMs U3-U10 & U11-U18 (at side of PCB) - 16M TSOP48 Type-II surface-mounted MaskROM
      ROM U52 - 32M TSOP48 Type-II surface-mounted MaskROM
      * - ROM locations not populated on tgm2 & tgm2+
      VSync - 60Hz
      HSync - 15.27kHz

*/

#include "driver.h"

#include "cpu/sh2/sh2.h"
#include "machine/eeprom.h"
#include "sound/ymf278b.h"

#include "psikyosh.h"

#define ROMTEST 1 /* Does necessary stuff to perform rom test, uses RAM as it doesn't dispose of GFX after decoding */

static UINT8 factory_eeprom[16]  = { 0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00 };
static UINT8 daraku_eeprom[16]   = { 0x03,0x02,0x00,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static UINT8 s1945iii_eeprom[16] = { 0x00,0x00,0x00,0x00,0x00,0x01,0x11,0x70,0x25,0x25,0x25,0x00,0x01,0x00,0x11,0xe0 };
static UINT8 dragnblz_eeprom[16] = { 0x00,0x01,0x11,0x70,0x25,0x25,0x25,0x00,0x01,0x00,0x11,0xe0,0x00,0x00,0x00,0x00 };
static UINT8 gnbarich_eeprom[16] = { 0x00,0x0f,0x42,0x40,0x08,0x0a,0x00,0x00,0x01,0x06,0x42,0x59,0x00,0x00,0x00,0x00 };

int use_factory_eeprom;

UINT32 *psikyosh_bgram, *psikyosh_zoomram, *psikyosh_vidregs, *psh_ram;

static const gfx_layout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{STEP4(0,1)},
	{STEP16(0,4)},
	{STEP16(0,16*4)},
	16*16*4
};

static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{STEP8(0,1)},
	{STEP16(0,8)},
	{STEP16(0,16*8)},
	16*16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0x000, 0x100 }, // 4bpp tiles
	{ REGION_GFX1, 0, &layout_16x16x8, 0x000, 0x100 }, // 8bpp tiles
	{ -1 }
};

static struct EEPROM_interface eeprom_interface_93C56 =
{
	8,		// address bits 8
	8,		// data bits    8
	"*110x",	// read         110x aaaaaaaa
	"*101x",	// write        101x aaaaaaaa dddddddd
	"*111x",	// erase        111x aaaaaaaa
	"*10000xxxxxxx",// lock         100x 00xxxx
	"*10011xxxxxxx",// unlock       100x 11xxxx
//  "*10001xxxx",   // write all    1 00 01xxxx dddddddddddddddd
//  "*10010xxxx"    // erase all    1 00 10xxxx
};

static NVRAM_HANDLER(93C56)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C56);
		if (file)
		{
			EEPROM_load(file);
		}
		else	// these games want the eeprom all zeros by default
		{
			int length;
			UINT8 *dat;

			dat = EEPROM_get_data_pointer(&length);
			memset(dat, 0, length);

 			if (use_factory_eeprom!=EEPROM_0) /* Set the EEPROM to Factory Defaults for games needing them*/
 			{
				UINT8 eeprom_data[0x100];
				int i;

				for(i=0; i<0x100; i++) eeprom_data[i] = 0;

				memcpy(eeprom_data, factory_eeprom, 0x10);

  				if (use_factory_eeprom==EEPROM_DARAKU) /* Daraku, replace top 10 bytes with defaults (different to other games) */
 					memcpy(eeprom_data, daraku_eeprom, 0x10);

				if (use_factory_eeprom==EEPROM_S1945III) /* S1945iii suffers from corruption on highscore unless properly initialised at the end of the eeprom */
 					memcpy(eeprom_data+0xf0, s1945iii_eeprom, 0x10);

 				if (use_factory_eeprom==EEPROM_DRAGNBLZ) /* Dragnblz too */
 					memcpy(eeprom_data+0xf0, dragnblz_eeprom, 0x10);

 				if (use_factory_eeprom==EEPROM_GNBARICH) /* Might as well do Gnbarich as well, otherwise the highscore is incorrect */
 					memcpy(eeprom_data+0xf0, gnbarich_eeprom, 0x10);

				EEPROM_set_data(eeprom_data,0x100);
			}
		}
	}
}

static WRITE32_HANDLER( psh_eeprom_w )
{
	if (ACCESSING_MSB32)
	{
		EEPROM_write_bit((data & 0x20000000) ? 1 : 0);
		EEPROM_set_cs_line((data & 0x80000000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40000000) ? ASSERT_LINE : CLEAR_LINE);

		return;
	}

	logerror("Unk EEPROM write %x mask %x\n", data, mem_mask);
}

static READ32_HANDLER( psh_eeprom_r )
{
	if (ACCESSING_MSB32)
	{
		return ((EEPROM_read_bit() << 28) | (readinputport(4) << 24)); /* EEPROM | Region */
	}

	logerror("Unk EEPROM read mask %x\n", mem_mask);

	return 0;
}

static INTERRUPT_GEN(psikyosh_interrupt)
{
	cpunum_set_input_line(0, 4, HOLD_LINE);
}

static READ32_HANDLER(io32_r)
{
	return ((readinputport(0) << 24) | (readinputport(1) << 16) | (readinputport(2) << 8) | (readinputport(3) << 0));
}

static WRITE32_HANDLER( paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]); /* is this ok .. */

	b = ((paletteram32[offset] & 0x0000ff00) >>8);
	g = ((paletteram32[offset] & 0x00ff0000) >>16);
	r = ((paletteram32[offset] & 0xff000000) >>24);

	palette_set_color(offset,r,g,b);
}

static WRITE32_HANDLER( psikyosh_vidregs_w )
{
	COMBINE_DATA(&psikyosh_vidregs[offset]);

#if ROMTEST
	if(offset==4) /* Configure bank for gfx test */
	{
		if (!(mem_mask & 0x000000ff) || !(mem_mask & 0x0000ff00))	// Bank
		{
			unsigned char *ROM = memory_region(REGION_GFX1);
			memory_set_bankptr(2,&ROM[0x20000 * (psikyosh_vidregs[offset]&0xfff)]); /* Bank comes from vidregs */
		}
	}
#endif
}

#if ROMTEST
static UINT32 sample_offs = 0;

static READ32_HANDLER( psh_sample_r ) /* Send sample data for test */
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

	return ROM[sample_offs++]<<16;
}
#endif

static READ32_HANDLER( psh_ymf_fm_r )
{
	return YMF278B_status_port_0_r(0)<<24; /* Also, bit 0 being high indicates not ready to send sample data for test */
}

static WRITE32_HANDLER( psh_ymf_fm_w )
{
	if (!(mem_mask & 0xff000000))	// FM bank 1 address (OPL2/OPL3 compatible)
	{
		YMF278B_control_port_0_A_w(0, data>>24);
	}

	if (!(mem_mask & 0x00ff0000))	// FM bank 1 data
	{
		YMF278B_data_port_0_A_w(0, data>>16);
	}

	if (!(mem_mask & 0x0000ff00))	// FM bank 2 address (OPL3/YMF 262 extended)
	{
		YMF278B_control_port_0_B_w(0, data>>8);
	}

	if (!(mem_mask & 0x000000ff))	// FM bank 2 data
	{
		YMF278B_data_port_0_B_w(0, data);
	}
}

static WRITE32_HANDLER( psh_ymf_pcm_w )
{
	if (!(mem_mask & 0xff000000))	// PCM address (OPL4/YMF 278B extended)
	{
		YMF278B_control_port_0_C_w(0, data>>24);

#if ROMTEST
		if (data>>24 == 0x06)	// Reset Sample reading (They always write this code immediately before reading data)
		{
			sample_offs = 0;
		}
#endif
	}

	if (!(mem_mask & 0x00ff0000))	// PCM data
	{
		YMF278B_data_port_0_C_w(0, data>>16);
	}
}

static ADDRESS_MAP_START( ps3v1_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_READ(MRA32_ROM)	// program ROM (1 meg)
	AM_RANGE(0x02000000, 0x021fffff) AM_READ(MRA32_BANK1) // data ROM
	AM_RANGE(0x03000000, 0x03003fff) AM_READ(MRA32_RAM)	// sprites
	AM_RANGE(0x03004000, 0x0300ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x03040000, 0x03044fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x03050000, 0x030501ff) AM_READ(MRA32_RAM)
	AM_RANGE(0x0305ffdc, 0x0305ffdf) AM_READ(MRA32_NOP) // also writes to this address - might be vblank reads?
	AM_RANGE(0x0305ffe0, 0x0305ffff) AM_READ(MRA32_RAM) //  video registers
	AM_RANGE(0x05000000, 0x05000003) AM_READ(psh_ymf_fm_r) // read YMF status
	AM_RANGE(0x05800000, 0x05800003) AM_READ(io32_r)
	AM_RANGE(0x05800004, 0x05800007) AM_READ(psh_eeprom_r)
	AM_RANGE(0x06000000, 0x060fffff) AM_READ(MRA32_RAM) // main RAM (1 meg)

#if ROMTEST
	AM_RANGE(0x05000004, 0x05000007) AM_READ(psh_sample_r) // data for rom tests (Used to verify Sample rom)
	AM_RANGE(0x03060000, 0x0307ffff) AM_READ(MRA32_BANK2) // data for rom tests (gfx), data is controlled by vidreg
	AM_RANGE(0x04060000, 0x0407ffff) AM_READ(MRA32_BANK2) // data for rom tests (gfx) (Mirrored?)
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START( ps3v1_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_WRITE(MWA32_ROM)	// program ROM (1 meg)
	AM_RANGE(0x02000000, 0x021fffff) AM_WRITE(MWA32_ROM) // data ROM
	AM_RANGE(0x03000000, 0x03003fff) AM_WRITE(MWA32_RAM) AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)	// sprites (might be a bit longer)
	AM_RANGE(0x03004000, 0x0300ffff) AM_WRITE(MWA32_RAM) AM_BASE(&psikyosh_bgram) // backgrounds
	AM_RANGE(0x03040000, 0x03044fff) AM_WRITE(paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w) AM_BASE(&paletteram32) // palette..
	AM_RANGE(0x03050000, 0x030501ff) AM_WRITE(MWA32_RAM) AM_BASE(&psikyosh_zoomram) // a gradient sometimes ...
	AM_RANGE(0x0305ffdc, 0x0305ffdf) AM_WRITE(MWA32_RAM) // also reads from this address
	AM_RANGE(0x0305ffe0, 0x0305ffff) AM_WRITE(psikyosh_vidregs_w) AM_BASE(&psikyosh_vidregs) //  video registers
	AM_RANGE(0x05000000, 0x05000003) AM_WRITE(psh_ymf_fm_w) // first 2 OPL4 register banks
	AM_RANGE(0x05000004, 0x05000007) AM_WRITE(psh_ymf_pcm_w) // third OPL4 register bank
	AM_RANGE(0x05800004, 0x05800007) AM_WRITE(psh_eeprom_w)
	AM_RANGE(0x06000000, 0x060fffff) AM_WRITE(MWA32_RAM) AM_BASE(&psh_ram) // work RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ps5_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_READ(MRA32_ROM) // program ROM (1 meg)
	AM_RANGE(0x03000000, 0x03000003) AM_READ(io32_r)
	AM_RANGE(0x03000004, 0x03000007) AM_READ(psh_eeprom_r)
	AM_RANGE(0x03100000, 0x03100003) AM_READ(psh_ymf_fm_r)
	AM_RANGE(0x04000000, 0x04003fff) AM_READ(MRA32_RAM)	// sprites
	AM_RANGE(0x04004000, 0x0400ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x04040000, 0x04044fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x04050000, 0x040501ff) AM_READ(MRA32_RAM)
	AM_RANGE(0x0405ffdc, 0x0405ffdf) AM_READ(MRA32_NOP) // also writes to this address - might be vblank reads?
	AM_RANGE(0x0405ffe0, 0x0405ffff) AM_READ(MRA32_RAM) // video registers
	AM_RANGE(0x05000000, 0x0507ffff) AM_READ(MRA32_BANK1) // data ROM
	AM_RANGE(0x06000000, 0x060fffff) AM_READ(MRA32_RAM)

#if ROMTEST
	AM_RANGE(0x03100004, 0x03100007) AM_READ(psh_sample_r) // data for rom tests (Used to verify Sample rom)
	AM_RANGE(0x04060000, 0x0407ffff) AM_READ(MRA32_BANK2) // data for rom tests (gfx), data is controlled by vidreg
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START( ps5_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_WRITE(MWA32_ROM)	// program ROM (1 meg)
	AM_RANGE(0x03000004, 0x03000007) AM_WRITE(psh_eeprom_w)
	AM_RANGE(0x03100000, 0x03100003) AM_WRITE(psh_ymf_fm_w) // first 2 OPL4 register banks
	AM_RANGE(0x03100004, 0x03100007) AM_WRITE(psh_ymf_pcm_w) // third OPL4 register bank
	AM_RANGE(0x04000000, 0x04003fff) AM_WRITE(MWA32_RAM) AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x04004000, 0x0400ffff) AM_WRITE(MWA32_RAM) AM_BASE(&psikyosh_bgram) // backgrounds
	AM_RANGE(0x04040000, 0x04044fff) AM_WRITE(paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w) AM_BASE(&paletteram32)
	AM_RANGE(0x04050000, 0x040501ff) AM_WRITE(MWA32_RAM) AM_BASE(&psikyosh_zoomram)
	AM_RANGE(0x0405ffdc, 0x0405ffdf) AM_WRITE(MWA32_RAM) // also reads from this address
	AM_RANGE(0x0405ffe0, 0x0405ffff) AM_WRITE(psikyosh_vidregs_w) AM_BASE(&psikyosh_vidregs) // video registers
	AM_RANGE(0x05000000, 0x0507ffff) AM_WRITE(MWA32_ROM) // data ROM
	AM_RANGE(0x06000000, 0x060fffff) AM_WRITE(MWA32_RAM) AM_BASE(&psh_ram)
ADDRESS_MAP_END

static void irqhandler(int linestate)
{
	if (linestate)
		cpunum_set_input_line(0, 12, ASSERT_LINE);
	else
		cpunum_set_input_line(0, 12, CLEAR_LINE);
}

static struct YMF278B_interface ymf278b_interface =
{
	REGION_SOUND1,
	irqhandler
};

static MACHINE_DRIVER_START( psikyo3v1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, MASTER_CLOCK/2)
	MDRV_CPU_PROGRAM_MAP(ps3v1_readmem,ps3v1_writemem)
	MDRV_CPU_VBLANK_INT(psikyosh_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C56)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT) /* If using alpha */
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x5000/4)

	MDRV_VIDEO_START(psikyosh)
	MDRV_VIDEO_EOF(psikyosh)
	MDRV_VIDEO_UPDATE(psikyosh)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMF278B, MASTER_CLOCK/2)
	MDRV_SOUND_CONFIG(ymf278b_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psikyo5 )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(psikyo3v1)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(ps5_readmem,ps5_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psikyo5_240 )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(psikyo3v1)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(ps5_readmem,ps5_writemem)

	/* It probably has a register to change visarea */
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 30*8-1)
MACHINE_DRIVER_END


#define UNUSED_PORT \
	PORT_START_TAG("IN2")/* not read? */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define PORT_COIN( debug ) \
	PORT_START_TAG("IN3") /* System inputs */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1    ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2    ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN  ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_SERVICE_NO_TOGGLE(0x20, IP_ACTIVE_LOW) \
	PORT_DIPNAME( 0x40, debug ? 0x00 : 0x40, "Debug" ) /* Must be high for dragnblz, low for others (Resets EEPROM?). Debug stuff */ \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

#define PSIKYOSH_PORT_PLAYER( player, start, buttons ) \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, start ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, (buttons>=3)?(IPT_BUTTON3 ) :IPT_UNKNOWN ) PORT_PLAYER(player) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, (buttons>=2)?(IPT_BUTTON2 ) :IPT_UNKNOWN ) PORT_PLAYER(player) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, (buttons>=1)?(IPT_BUTTON1 ) :IPT_UNKNOWN ) PORT_PLAYER(player)\
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(player) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(player) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(player) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(player)

INPUT_PORTS_START( s1945ii )
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 2 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 2 )

	UNUSED_PORT
	PORT_COIN( 0 )

	PORT_START_TAG("IN4") /* jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x01, DEF_STR( World ) )
INPUT_PORTS_END

INPUT_PORTS_START( soldivid )
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 3 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 3 )

	UNUSED_PORT
	PORT_COIN( 0 )

	PORT_START_TAG("IN4")/* jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x01, DEF_STR( World ) )
INPUT_PORTS_END

INPUT_PORTS_START( daraku )
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 2 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 2 )

	PORT_START_TAG("IN2")  /* more controls */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_COIN( 0 )

	PORT_START_TAG("IN4")/* jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x01, DEF_STR( World ) ) /* Title screen is different, English is default now */
INPUT_PORTS_END

INPUT_PORTS_START( sbomberb )
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 2 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 2 )

	UNUSED_PORT
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, but EEPROM resets? */

	PORT_START_TAG("IN4")/* jumper pads on the PCB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x01, DEF_STR( World ) )
INPUT_PORTS_END

INPUT_PORTS_START( gunbird2 ) /* Different Region */
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 3 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 3 )

	UNUSED_PORT
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, but EEPROM resets */

	PORT_START_TAG("IN4")/* jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x01, "International Ver A." )
	PORT_DIPSETTING(    0x02, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( s1945iii ) /* Different Region again */
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 3 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 3 )

	UNUSED_PORT
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, EEPROM doesn't reset */

	PORT_START_TAG("IN4")/* IN4 jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( dragnblz ) /* Security requires bit high */
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 3 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 3 )

	UNUSED_PORT

	PORT_COIN( 1 ) /* Must be HIGH (Or Security Error), so can perform test */

	PORT_START_TAG("IN4")/* jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( gnbarich ) /* Same as S1945iii except only one button */
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 3 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 3 )

	UNUSED_PORT
	PORT_COIN( 0 ) /* If HIGH then you can perform rom test, but EEPROM resets? */

	PORT_START_TAG("IN4")/* jumper pads on the PCB */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x02, "International Ver A." )
	PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END

INPUT_PORTS_START( mjgtaste ) /* This will need the Mahjong inputs */
	PORT_START_TAG("IN0")
	PSIKYOSH_PORT_PLAYER( 1, IPT_START1, 3 )
	PORT_START_TAG("IN1")
	PSIKYOSH_PORT_PLAYER( 2, IPT_START2, 3 )

	UNUSED_PORT
	PORT_COIN( 0 )

	PORT_START_TAG("IN4")/* jumper pads on the PCB */
//  PORT_DIPNAME( 0x03, 0x01, DEF_STR( Region ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
//  PORT_DIPSETTING(    0x02, "International Ver A." )
//  PORT_DIPSETTING(    0x01, "International Ver B." )
INPUT_PORTS_END


#if ROMTEST
#define ROMTEST_GFX 0
#else
#define ROMTEST_GFX ROMREGION_DISPOSE
#endif

/* PS3 */

ROM_START( soldivid )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-prog_l.u18", 0x000002, 0x080000, CRC(cf179b04) SHA1(343f00a81cffd44334a4db81b6b828b7cf73c1e8) )
	ROM_LOAD32_WORD_SWAP( "1-prog_h.u17", 0x000000, 0x080000, CRC(f467d1c4) SHA1(a011e6f310a54f09efa0bf4597783cd78c05ad6f) )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMTEST_GFX )
	/* This Space Empty! */
	ROM_LOAD32_WORD_SWAP( "4l.u10", 0x2000000, 0x400000, CRC(9eb9f269) SHA1(4a4d90eefe62b5462f5ed5e062eea7b6b4900f85) )
	ROM_LOAD32_WORD_SWAP( "4h.u31", 0x2000002, 0x400000, CRC(7c76cfe7) SHA1(14e291e840a4afe3802fe1847615c5e806d7492a) )
	ROM_LOAD32_WORD_SWAP( "5l.u9",  0x2800000, 0x400000, CRC(c59c6858) SHA1(bd580b57e432ef42295060c5a84c8129d9b995f7) )
	ROM_LOAD32_WORD_SWAP( "5h.u30", 0x2800002, 0x400000, CRC(73bc66d0) SHA1(7988ce81ff43235a3b30ddd8fd9419530a07b6ba) )
	ROM_LOAD32_WORD_SWAP( "6l.u8",  0x3000000, 0x400000, CRC(f01b816e) SHA1(2a0d86c1c106eef539028aa9ebe49d13216a6b9c) )
	ROM_LOAD32_WORD_SWAP( "6h.u37", 0x3000002, 0x400000, CRC(fdd57361) SHA1(f58d91acde1f4e6d4f0e8dcd1b23aa5092d89916) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "sound.bin", 0x000000, 0x400000, CRC(e98f8d45) SHA1(7791c0f31d08f37c6ec65e7cecf8ef54ca73b1fd) )
ROM_END

ROM_START( s1945ii )
	ROM_REGION( 0x100000, REGION_CPU1, 0) /* Code */
	ROM_LOAD32_WORD_SWAP( "2_prog_l.u18", 0x000002, 0x080000, CRC(20a911b8) SHA1(82ba7b93bd621fc45a4dc2722752077b59a0a233) )
	ROM_LOAD32_WORD_SWAP( "1_prog_h.u17", 0x000000, 0x080000, CRC(4c0fe85e) SHA1(74f810a1c3e9d629c8b190f68d73ce07b11f77b7) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )	/* Tiles */
	ROM_LOAD32_WORD( "0l.u4",    0x0000000, 0x400000, CRC(bfacf98d) SHA1(19954f12881e6e95e808bd1f2c2f5a425786727f) )
	ROM_LOAD32_WORD( "0h.u13",   0x0000002, 0x400000, CRC(1266f67c) SHA1(cf93423a827aa92aa54afbbecf8509d2590edc9b) )
	ROM_LOAD32_WORD( "1l.u3",    0x0800000, 0x400000, CRC(2d3332c9) SHA1(f2e54100a48061bfd589e8765f59ca051176a38b) )
	ROM_LOAD32_WORD( "1h.u12",   0x0800002, 0x400000, CRC(27b32c3e) SHA1(17a80b3c919d8a282169c019ede8a22d2079c018) )
	ROM_LOAD32_WORD( "2l.u2",    0x1000000, 0x400000, CRC(91ba6d23) SHA1(fd016a90204b2de43bb709971f7cd891f839de1a) )
	ROM_LOAD32_WORD( "2h.u20",   0x1000002, 0x400000, CRC(fabf4334) SHA1(f8ec43e083b674700f532575f0d067bd49c5aaf7) )
	ROM_LOAD32_WORD( "3l.u1",    0x1800000, 0x400000, CRC(a6c3704e) SHA1(cb9881e4235cc8e4bcca4c6ccbd8d8d8634e3624) )
	ROM_LOAD32_WORD( "3h.u19",   0x1800002, 0x400000, CRC(4cd3ca70) SHA1(5b0a6ea4fe0e821cebe6e840596f648e24dded51) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u32", 0x000000, 0x400000, CRC(ba680ca7) SHA1(b645896e297aad426784aa656bff738e1b33c2a2) )
	ROM_RELOAD ( 0x400000, 0x400000 )
	/* 0x400000 - 0x7fffff allocated but left blank, it randomly reads from here on the
        Iron Casket level causing a crash otherwise, not sure why, bug in the sound emulation? */
ROM_END

ROM_START( daraku )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "4_prog_l.u18", 0x000002, 0x080000, CRC(660b4609) SHA1(ee6b5606fae41881c3e671ee642baae5c03331ca) )
	ROM_LOAD32_WORD_SWAP( "3_prog_h.u17", 0x000000, 0x080000, CRC(7a9cf601) SHA1(8df464ce3fd02b30dd2ab77828594f4916375fd5) )
	ROM_LOAD16_WORD_SWAP( "prog.u16",     0x100000, 0x100000, CRC(3742e990) SHA1(dd4b8777e57245151b3d520ed1bdab207530420b) )

	ROM_REGION( 0x3400000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u4",  0x0000000, 0x400000, CRC(565d8427) SHA1(090ce9213c530d29e488cfb89bb39fd7169985d5) )
	ROM_LOAD32_WORD( "0h.u13", 0x0000002, 0x400000, CRC(9a602630) SHA1(ab176490b36aec7ce30d1cf20b57c02c926c59d3) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(ac5ce8e1) SHA1(7df6a04ea2530cc669581474e8b8ee6f59caae1b) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(b0a59f7b) SHA1(8704705aa0977f11da8bcdafae6e2531190878d0) )
	ROM_LOAD32_WORD( "2l.u2",  0x1000000, 0x400000, CRC(2daa03b2) SHA1(475badc60cbd26786242d685a3d7dbaf385862a8) )
	ROM_LOAD32_WORD( "2h.u20", 0x1000002, 0x400000, CRC(e98e185a) SHA1(124d5fcf6cfb1faf70d665b687564bf6589d17c4) )
	ROM_LOAD32_WORD( "3l.u1",  0x1800000, 0x400000, CRC(1d372aa1) SHA1(e5965a1d8919409a314dfd56482a848d6ab9f5ac) )
	ROM_LOAD32_WORD( "3h.u19", 0x1800002, 0x400000, CRC(597f3f15) SHA1(62bf74ed29732e6cc1979458745cdb53a8edddf3) )
	ROM_LOAD32_WORD( "4l.u10", 0x2000000, 0x400000, CRC(e3d58cd8) SHA1(9482d0b71f840d72b20029804cfc8dca207462de) )
	ROM_LOAD32_WORD( "4h.u31", 0x2000002, 0x400000, CRC(aebc9cd0) SHA1(c20a1f9851ace74e00f1a0746e0c9e751ccec336) )
	ROM_LOAD32_WORD( "5l.u9",  0x2800000, 0x400000, CRC(eab5a50b) SHA1(76ce96e89afc438bafb9f8caa86eb48fb7e4e154) )
	ROM_LOAD32_WORD( "5h.u30", 0x2800002, 0x400000, CRC(f157474f) SHA1(89509f0772a40829070cea708c21438ff61d1019) )
	ROM_LOAD32_WORD( "6l.u8",  0x3000000, 0x200000, CRC(9f008d1b) SHA1(9607e09bde430eefe126569a6e251114bc8f754b) )
	ROM_LOAD32_WORD( "6h.u37", 0x3000002, 0x200000, CRC(acd2d0e3) SHA1(dee96bdf3b8efde1298b73c5e7dd62abcdc101cf) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u32", 0x000000, 0x400000, CRC(ef2c781d) SHA1(1313f082f6dbe4da0efaf261226085eb7325667f) )
ROM_END

ROM_START( sbomberb )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1-b_pr_l.u18", 0x000002, 0x080000, CRC(52d12225) SHA1(0a31a5d557414e7bf51dc6f7fbdd417a20b78df1) )
	ROM_LOAD32_WORD_SWAP( "1-b_pr_h.u17", 0x000000, 0x080000, CRC(1bbd0345) SHA1(c6ccb7c97cc9e9ea298c1883d1dd5563907a7255) )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u4",  0x0000000, 0x400000, CRC(b7e4ac51) SHA1(70e802b6235932116496a77ee0c78a256e85aff3) )
	ROM_LOAD32_WORD( "0h.u13", 0x0000002, 0x400000, CRC(235e6c27) SHA1(c597d7b5bef4edac1474ad0024cfb33eb1257106) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(3c88c48c) SHA1(d1ce4ab60ba18449bbd96e29c310e060a0bb6de6) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(15626a6e) SHA1(5493e92c9724982938591d758bee7d86cf96fd19) )
	ROM_LOAD32_WORD( "2l.u2",  0x1000000, 0x400000, CRC(41e92f64) SHA1(ea121c7cb35266ed0c21af4bb958fe5d73d84977) )
	ROM_LOAD32_WORD( "2h.u20", 0x1000002, 0x400000, CRC(4ae62e84) SHA1(adc1dab2f09aa4f5665d7bb7603a9b75c978031e) )
	ROM_LOAD32_WORD( "3l.u1",  0x1800000, 0x400000, CRC(43ba5f0f) SHA1(b8f93ed055441fd06b68103c9fd62b6aa3f3da7d) )
	ROM_LOAD32_WORD( "3h.u19", 0x1800002, 0x400000, CRC(ff01bb12) SHA1(df6fab898356c02f34ee7a45fdcc265218f2f20e) )
	ROM_LOAD32_WORD( "4l.u10", 0x2000000, 0x400000, CRC(e491d593) SHA1(12a7f6c282969be342b70443b8c802a399571245) )
	ROM_LOAD32_WORD( "4h.u31", 0x2000002, 0x400000, CRC(7bdd377a) SHA1(e357c98f82b8ea3ae4fd8eae6c1ad2dfb500db9c) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u32", 0x000000, 0x400000, CRC(85cbff69) SHA1(34c7f4d337111de2064f84214294b6bdc37bf16c) )
ROM_END

/* PS5 */

ROM_START( gunbird2 )
	ROM_REGION( 0x180000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2_prog_l.u16", 0x000002, 0x080000, CRC(76f934f0) SHA1(cf197796d66f15639a6b3d5311c18da33cefd06b) )
	ROM_LOAD32_WORD_SWAP( "1_prog_h.u17", 0x000000, 0x080000, CRC(7328d8bf) SHA1(c640de1ab5b32400b2d77e0dc6e3ee0f78ab7803) )
	ROM_LOAD16_WORD_SWAP( "3_pdata.u1",   0x100000, 0x080000, CRC(a5b697e6) SHA1(947f124fa585c2cf77c6571af7559bd652897b89) )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u3",  0x0000000, 0x800000, CRC(5c826bc8) SHA1(74fb6b242b4c5fe5365cfcc3029ed6da4cf3a621) )
	ROM_LOAD32_WORD( "0h.u10", 0x0000002, 0x800000, CRC(3df0cb6c) SHA1(271d276fa0f63d84e458223316a9517865fc2255) )
	ROM_LOAD32_WORD( "1l.u4",  0x1000000, 0x800000, CRC(1558358d) SHA1(e3b9c3da4e9b29ffa9568b57d14fe2b600aead68) )
	ROM_LOAD32_WORD( "1h.u11", 0x1000002, 0x800000, CRC(4ee0103b) SHA1(29bbe0162dda39919fcd188ea4a6b7b5f20366ff) )
	ROM_LOAD32_WORD( "2l.u5",  0x2000000, 0x800000, CRC(e1c7a7b8) SHA1(b5f6e5d53e21928197773df7dde0e7c83f4082af) )
	ROM_LOAD32_WORD( "2h.u12", 0x2000002, 0x800000, CRC(bc8a41df) SHA1(90460b11eea778f17cf8be67430e2ab149680686) )
	ROM_LOAD32_WORD( "3l.u6",  0x3000000, 0x400000, CRC(0229d37f) SHA1(f9d98d1d2dda2d552b2a46c76b4c7fc84b1aa4c6) )
	ROM_LOAD32_WORD( "3h.u13", 0x3000002, 0x400000, CRC(f41bbf2b) SHA1(b705274e392541e2f513a4ae4bae543c03be0913) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u9", 0x000000, 0x400000, CRC(f19796ab) SHA1(b978f0550ebd675e8ce9d9edcfcc3f6214e49e8b) )
	ROM_RELOAD ( 0x400000, 0x400000 ) /* crash fix similar to s1945 II + III */
ROM_END

ROM_START( s1945iii )
	ROM_REGION( 0x180000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2_progl.u16", 0x000002, 0x080000, CRC(5d5d385f) SHA1(67b3bcabd71cf084bcea7a59939281a8d6257059) )
	ROM_LOAD32_WORD_SWAP( "1_progh.u17", 0x000000, 0x080000, CRC(1b8a5a18) SHA1(718a176bd48e16f964fcb07c568b5227cfc0515f) )
	ROM_LOAD16_WORD_SWAP( "3_data.u1",   0x100000, 0x080000, CRC(8ff5f7d3) SHA1(420a3d7f2d5ab6a56789d36b418431f12f5f73f5) )

	ROM_REGION( 0x3800000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u3",  0x0000000, 0x800000, CRC(70a0d52c) SHA1(c9d9534da59123b577dc22020273b94ccdeeb67d) )
	ROM_LOAD32_WORD( "0h.u10", 0x0000002, 0x800000, CRC(4dcd22b4) SHA1(2df7a7d08df17d2a62d574fccc8ba40aaae21a13) )
	ROM_LOAD32_WORD( "1l.u4",  0x1000000, 0x800000, CRC(de1042ff) SHA1(468f6dfd5c1f2084c573b6851e314ff2826dc350) )
	ROM_LOAD32_WORD( "1h.u11", 0x1000002, 0x800000, CRC(b51a4430) SHA1(b51117591b0e351e922f9a6a7930e8b50237e54e) )
	ROM_LOAD32_WORD( "2l.u5",  0x2000000, 0x800000, CRC(23b02dca) SHA1(0249dceca02b312301a917d98fac481b6a0a9122) )
	ROM_LOAD32_WORD( "2h.u12", 0x2000002, 0x800000, CRC(9933ab04) SHA1(710e6b20e111c1898666b4466554d039309883cc) )
	ROM_LOAD32_WORD( "3l.u6",  0x3000000, 0x400000, CRC(f693438c) SHA1(d70e25a3f56aae6575c696d9b7b6d7a9d04f0104) )
	ROM_LOAD32_WORD( "3h.u13", 0x3000002, 0x400000, CRC(2d0c334f) SHA1(74d94abb34484c7b79dbb989645f53124e53e3b7) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.u9", 0x000000, 0x400000, CRC(c5374beb) SHA1(d13e12cbd249246d953c45bb3bfa576a0ec75595) )
	ROM_RELOAD ( 0x400000, 0x400000 )
ROM_END

/* PS5v2 */

ROM_START( dragnblz )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2prog_h.u21",   0x000000, 0x080000, CRC(fc5eade8) SHA1(e5d05543641e4a3900b0d42e0d5f75734683d635) )
	ROM_LOAD32_WORD_SWAP( "1prog_l.u22",   0x000002, 0x080000, CRC(95d6fd02) SHA1(2b2830e7fa66cbd13666191762bfddc40571caec) )

	ROM_REGION( 0x2c00000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "1l.u4",  0x0400000, 0x200000, CRC(c2eb565c) SHA1(07e41b36cc03a87f28d091754fdb0d1a7316a532) )
	ROM_LOAD32_WORD( "1h.u12", 0x0400002, 0x200000, CRC(23cb46b7) SHA1(005b7cc40eea103688a64a72c219c7535970dbfb) )
	ROM_LOAD32_WORD( "2l.u5",  0x0800000, 0x200000, CRC(bc256aea) SHA1(1f1d678e8a63513a95f296b8a07d2ea485d1e53f) )
	ROM_LOAD32_WORD( "2h.u13", 0x0800002, 0x200000, CRC(b75f59ec) SHA1(a6cde94bc972e46e54c962fde49fc2174b312882) )
	ROM_LOAD32_WORD( "3l.u6",  0x0c00000, 0x200000, CRC(4284f008) SHA1(610b13304043411b3088fd4299b3cb0a4d8b0cc2) )
	ROM_LOAD32_WORD( "3h.u14", 0x0c00002, 0x200000, CRC(abe5cbbf) SHA1(c2fb1d8ea8772572c08b36496cf9fc5b91cf848b) )
	ROM_LOAD32_WORD( "4l.u7",  0x1000000, 0x200000, CRC(c9fcf2e5) SHA1(7cecdf3406da11289b54aaf58d12883ddfdc5e6b) )
	ROM_LOAD32_WORD( "4h.u15", 0x1000002, 0x200000, CRC(0ab0a12a) SHA1(1b29b6dc79e69edb56634517365d0ee8e6ea78ae) )
	ROM_LOAD32_WORD( "5l.u8",  0x1400000, 0x200000, CRC(68d03ccf) SHA1(d2bf6da5fa6e346b05872ed9616ffe51c3768f50) )
	ROM_LOAD32_WORD( "5h.u16", 0x1400002, 0x200000, CRC(5450fbca) SHA1(7a804263549cea951782a67855e69cb8cb417e98) )
	ROM_LOAD32_WORD( "6l.u1",  0x1800000, 0x200000, CRC(8b52c90b) SHA1(e1067ef252870787e46c62015e5778b4e641e68d) )
	ROM_LOAD32_WORD( "6h.u2",  0x1800002, 0x200000, CRC(7362f929) SHA1(9ced06202e3f104d30377aeef489021d26e87f73) )
	ROM_LOAD32_WORD( "7l.u19", 0x1c00000, 0x200000, CRC(b4f4d86e) SHA1(2ad786c5626c98e6943ae05688a1b66307ceac84) )
	ROM_LOAD32_WORD( "7h.u20", 0x1c00002, 0x200000, CRC(44b7b9cc) SHA1(3f8122b62ea1183d9fb3aad32d0e47bd32244f87) )
	ROM_LOAD32_WORD( "8l.u28", 0x2000000, 0x200000, CRC(cd079f89) SHA1(49c46eb36bc0458428a7fad3fe622f5ed974073b) )
	ROM_LOAD32_WORD( "8h.u29", 0x2000002, 0x200000, CRC(3edb508a) SHA1(72b07fb34a94cc127de02070604b1ff31f3d46c7) )
	ROM_LOAD32_WORD( "9l.u41", 0x2400000, 0x200000, CRC(0b53cd78) SHA1(e2071d9fe6c7be4e289b491587ab431c164e59da) )
	ROM_LOAD32_WORD( "9h.u42", 0x2400002, 0x200000, CRC(bc61998a) SHA1(75dbefe712104c64576196c27c25dbed59ae3923) )
	ROM_LOAD32_WORD( "10l.u58",0x2800000, 0x200000, CRC(a3f5c7f8) SHA1(d17478ca3e7ef46270f350ffa35d43acb05b1185) )
	ROM_LOAD32_WORD( "10h.u59",0x2800002, 0x200000, CRC(30e304c4) SHA1(1d866276bfe7f7524306a880d225aaf11ac2e5dd) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - Not Dumped */
	ROM_LOAD( "snd0.u52", 0x000000, 0x200000, CRC(7fd1b225) SHA1(6aa61021ada51393bbb34fd1aea00b8feccc8197) )
ROM_END

ROM_START( gnbarich )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-prog_l.u21",   0x000000, 0x080000, CRC(c136cd9c) SHA1(ab66c4f5196a66a97dbb5832336a203421cf40fa) )
	ROM_LOAD32_WORD_SWAP( "1-prog_h.u22",   0x000002, 0x080000, CRC(6588fc96) SHA1(3db29fcf17e8b2aee465319b557bd3e45bc966b2) )

	ROM_REGION( 0x2c00000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	/* Gunbarich doesn't actually use 1-5 and 10, they're on the board, but all the gfx are in 6-9
       The game was an upgrade to Dragon Blaze, only some of the roms were replaced however it
       appears the board needs to be fully populated to work correctly so the Dragon Blaze roms
       were left on it.  After hooking up hidden rom test we can see only the 8 roms we load are
       tested */
//  ROM_LOAD32_WORD( "1l.u4",  0x0400000, 0x200000, CRC(c2eb565c) SHA1(07e41b36cc03a87f28d091754fdb0d1a7316a532) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "1h.u12", 0x0400002, 0x200000, CRC(23cb46b7) SHA1(005b7cc40eea103688a64a72c219c7535970dbfb) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "2l.u5",  0x0800000, 0x200000, CRC(bc256aea) SHA1(1f1d678e8a63513a95f296b8a07d2ea485d1e53f) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "2h.u13", 0x0800002, 0x200000, CRC(b75f59ec) SHA1(a6cde94bc972e46e54c962fde49fc2174b312882) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "3l.u6",  0x0c00000, 0x200000, CRC(4284f008) SHA1(610b13304043411b3088fd4299b3cb0a4d8b0cc2) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "3h.u14", 0x0c00002, 0x200000, CRC(abe5cbbf) SHA1(c2fb1d8ea8772572c08b36496cf9fc5b91cf848b) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "4l.u7",  0x1000000, 0x200000, CRC(c9fcf2e5) SHA1(7cecdf3406da11289b54aaf58d12883ddfdc5e6b) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "4h.u15", 0x1000002, 0x200000, CRC(0ab0a12a) SHA1(1b29b6dc79e69edb56634517365d0ee8e6ea78ae) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "5l.u8",  0x1400000, 0x200000, CRC(68d03ccf) SHA1(d2bf6da5fa6e346b05872ed9616ffe51c3768f50) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "5h.u16", 0x1400002, 0x200000, CRC(5450fbca) SHA1(7a804263549cea951782a67855e69cb8cb417e98) ) /* From Dragon Blaze */
	ROM_LOAD32_WORD( "6l.u1",  0x1800000, 0x200000, CRC(0432e1a8) SHA1(632cb6534a19a92aa16d1dc8bb98c0c1fa17e428) )
	ROM_LOAD32_WORD( "6h.u2",  0x1800002, 0x200000, CRC(f90fa3ea) SHA1(773861c6c559f2df88e395669f27c43bd4dd6eb6) )
	ROM_LOAD32_WORD( "7l.u19", 0x1c00000, 0x200000, CRC(36bf9a58) SHA1(b546425f17f4b0b1112f0a22f9f5c695f5d97fe9) )
	ROM_LOAD32_WORD( "7h.u20", 0x1c00002, 0x200000, CRC(4b3eafd8) SHA1(8d0a4516bab2a188a66291e805c3c265774a6b72) )
	ROM_LOAD32_WORD( "8l.u28", 0x2000000, 0x200000, CRC(026754da) SHA1(66072e7584dcfea614a1e37592bda65733c9ce11) )
	ROM_LOAD32_WORD( "8h.u29", 0x2000002, 0x200000, CRC(8cd7aaa0) SHA1(83469c5407cba134ec1d22330623d8be8e0eabec) )
	ROM_LOAD32_WORD( "9l.u41", 0x2400000, 0x200000, CRC(02c066fe) SHA1(ecd5f36d9e55a341aff956bab4e7b0ae9e6cc15f) )
	ROM_LOAD32_WORD( "9h.u42", 0x2400002, 0x200000, CRC(5433385a) SHA1(138d62409cfb9e1a4eb3ca378ab8f6df45d478c0) )
//  ROM_LOAD32_WORD( "10l.u58",0x2800000, 0x200000, CRC(a3f5c7f8) SHA1(d17478ca3e7ef46270f350ffa35d43acb05b1185) ) /* From Dragon Blaze */
//  ROM_LOAD32_WORD( "10h.u59",0x2800002, 0x200000, CRC(30e304c4) SHA1(1d866276bfe7f7524306a880d225aaf11ac2e5dd) ) /* From Dragon Blaze */

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "snd0.u52", 0x000000, 0x200000, CRC(7b10436b) SHA1(c731fcce024e286a677ca10a91761c1ee06094a5) )
ROM_END

ROM_START( mjgtaste )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u21",   0x000000, 0x080000, CRC(5f2041dc) SHA1(f3862ffdb8df0cf921ce1cb0236935731e7729a7) )
	ROM_LOAD32_WORD_SWAP( "1.u22",   0x000002, 0x080000, CRC(f5ff7876) SHA1(4c909db9c97f29fd79df6dacd29762688701b973) )

	/* exact number of gfx / sound roms may be incorrect */
	ROM_REGION( 0x2c00000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "1l.u4",  0x0400000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "1h.u12", 0x0400002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "2l.u5",  0x0800000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "2h.u13", 0x0800002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "3l.u6",  0x0c00000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "3h.u14", 0x0c00002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "4l.u7",  0x1000000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "4h.u15", 0x1000002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "5l.u8",  0x1400000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "5h.u16", 0x1400002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "6l.u1",  0x1800000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "6h.u2",  0x1800002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "7l.u19", 0x1c00000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "7h.u20", 0x1c00002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "8l.u28", 0x2000000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "8h.u29", 0x2000002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "9l.u41", 0x2400000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "9h.u42", 0x2400002, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "10l.u58",0x2800000, 0x200000, NO_DUMP )
	ROM_LOAD32_WORD( "10h.u59",0x2800002, 0x200000, NO_DUMP )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - Not Dumped */
	ROM_LOAD( "snd0.u52", 0x000000, 0x200000, NO_DUMP )
ROM_END

/* are these right? should i fake the counter return?
   'speedups / idle skipping isn't needed for 'hotgmck, hgkairak'
   as the core catches and skips the idle loops automatically'
*/

static READ32_HANDLER( soldivid_speedup_r )
{
 /*
PC  : 0001AE74: MOV.L   @R14,R1
PC  : 0001AE76: ADD     #$01,R1
PC  : 0001AE78: MOV.L   R1,@R14
PC  : 0001AE7A: MOV.L   @($7C,PC),R3
PC  : 0001AE7C: MOV.L   @R3,R0
PC  : 0001AE7E: TST     R0,R0
PC  : 0001AE80: BT      $0001AE74
*/
	if (activecpu_get_pc()==0x0001AFAC) cpu_spinuntil_int(); // Character Select + InGame
	if (activecpu_get_pc()==0x0001AE76) cpu_spinuntil_int(); // Everything Else?

	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( s1945ii_speedup_r )
{
/*
PC  : 0609FC68: MOV.L   @R13,R1  // R13 is 600000C  R1 is counter  (read from r13)
PC  : 0609FC6A: ADD     #$01,R1  // add 1 to counter
PC  : 0609FC6C: MOV.L   R1,@R13  // write it back
PC  : 0609FC6E: MOV.L   @($3C,PC),R3 // 609fdac into r3
PC  : 0609FC70: MOV.L   @R3,R0  // whats there into r0
PC  : 0609FC72: TST     R0,R0 // test
PC  : 0609FC74: BT      $0609FC68
*/
	if (activecpu_get_pc()==0x609FC6A) cpu_spinuntil_int(); // Title Screens
	if (activecpu_get_pc()==0x609FED4) cpu_spinuntil_int(); // In Game
	if (activecpu_get_pc()==0x60A0172) cpu_spinuntil_int(); // Attract Demo

	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( daraku_speedup_r )
{
/*
PC  : 00047618: MOV.L   @($BC,PC),R0
PC  : 0004761A: MOV.L   @R0,R1
PC  : 0004761C: ADD     #$01,R1
PC  : 0004761E: MOV.L   R1,@R0
PC  : 00047620: MOV.L   @($BC,PC),R3
PC  : 00047622: MOV.L   @R3,R0
PC  : 00047624: TST     R0,R0
PC  : 00047626: BT      $00047618
*/
	if (activecpu_get_pc()==0x0004761C) cpu_spinuntil_int(); // title
	if (activecpu_get_pc()==0x00047978) cpu_spinuntil_int(); // ingame

	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( sbomberb_speedup_r )
{
/*
PC  : 060A10EC: MOV.L   @R13,R3
PC  : 060A10EE: ADD     #$01,R3
PC  : 060A10F0: MOV.L   R3,@R13
PC  : 060A10F2: MOV.L   @($34,PC),R1
PC  : 060A10F4: MOV.L   @R1,R2
PC  : 060A10F6: TST     R2,R2
PC  : 060A10F8: BT      $060A10EC
*/
	if (activecpu_get_pc()==0x060A10EE) cpu_spinuntil_int(); // title
	if (activecpu_get_pc()==0x060A165A) cpu_spinuntil_int(); // attract
	if (activecpu_get_pc()==0x060A1382) cpu_spinuntil_int(); // game

	return psh_ram[0x00000C/4];
}

static READ32_HANDLER( gunbird2_speedup_r )
{
/*
PC  : 06028972: MOV.L   @R14,R3   // r14 is 604000c on this one
PC  : 06028974: MOV.L   @($D4,PC),R1
PC  : 06028976: ADD     #$01,R3
PC  : 06028978: MOV.L   R3,@R14
PC  : 0602897A: MOV.L   @R1,R2
PC  : 0602897C: TST     R2,R2
PC  : 0602897E: BT      $06028972
*/
	if (activecpu_get_pc()==0x06028974) cpu_spinuntil_int();
	if (activecpu_get_pc()==0x06028E64) cpu_spinuntil_int();
	if (activecpu_get_pc()==0x06028BE6) cpu_spinuntil_int();

	return psh_ram[0x04000C/4];
}

static READ32_HANDLER( s1945iii_speedup_r )
{
	if (activecpu_get_pc()==0x0602B464) cpu_spinuntil_int(); // start up text
	if (activecpu_get_pc()==0x0602B6E2) cpu_spinuntil_int(); // intro attract
	if (activecpu_get_pc()==0x0602BC1E) cpu_spinuntil_int(); // game attract
	if (activecpu_get_pc()==0x0602B97C) cpu_spinuntil_int(); // game

	return psh_ram[0x06000C/4];
}


static READ32_HANDLER( dragnblz_speedup_r )
{
	if (activecpu_get_pc()==0x06027440) cpu_spinuntil_int(); // startup texts
	if (activecpu_get_pc()==0x060276E6) cpu_spinuntil_int(); // attract intro
	if (activecpu_get_pc()==0x06027C74) cpu_spinuntil_int(); // attract game
	if (activecpu_get_pc()==0x060279A8) cpu_spinuntil_int(); // game

	return psh_ram[0x006000C/4];
}

static READ32_HANDLER( gnbarich_speedup_r )
{
/*
PC  :0602CAE6: MOV.L   @R14,R3 // R14 = 0x606000C
PC  :0602CAE8: MOV.L   @($F4,PC),R1
PC  :0602CAEA: ADD     #$01,R3
PC  :0602CAEC: MOV.L   R3,@R14 // R14 = 0x606000C
PC  :0602CAEE: MOV.L   @R1,R2
PC  :0602CAF0: TST     R2,R2
PC  :0602CAF2: BT      $0602CAE6
*/

	if (activecpu_get_pc()==0x0602CAE8) cpu_spinuntil_int(); // title logos
	if (activecpu_get_pc()==0x0602CD88) cpu_spinuntil_int(); // attract intro
	if (activecpu_get_pc()==0x0602D2F0) cpu_spinuntil_int(); // game attract
	if (activecpu_get_pc()==0x0602D042) cpu_spinuntil_int(); // game play

	return psh_ram[0x006000C/4];
}

static READ32_HANDLER( mjgtaste_speedup_r )
{

	if (activecpu_get_pc()==0x6031f04) {cpu_spinuntil_int();return psh_ram[0x006000C/4];} // title logos
	if (activecpu_get_pc()==0x603214c) {cpu_spinuntil_int();return psh_ram[0x006000C/4];} // attract game

//  printf("at %08x\n",activecpu_get_pc());

	return psh_ram[0x006000C/4];
}



static DRIVER_INIT( soldivid )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x600000c, 0x600000f, 0, 0, soldivid_speedup_r );
	use_factory_eeprom=EEPROM_0;
}

static DRIVER_INIT( s1945ii )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x600000c, 0x600000f, 0, 0, s1945ii_speedup_r );
	use_factory_eeprom=EEPROM_DEFAULT;
}

static DRIVER_INIT( daraku )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	memory_set_bankptr(1,&RAM[0x100000]);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x600000c, 0x600000f, 0, 0, daraku_speedup_r );
	use_factory_eeprom=EEPROM_DARAKU;
}

static DRIVER_INIT( sbomberb )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x600000c, 0x600000f, 0, 0, sbomberb_speedup_r );
	use_factory_eeprom=EEPROM_DEFAULT;
}

static DRIVER_INIT( gunbird2 )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	memory_set_bankptr(1,&RAM[0x100000]);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x604000c, 0x604000f, 0, 0, gunbird2_speedup_r );
	use_factory_eeprom=EEPROM_DEFAULT;
}

static DRIVER_INIT( s1945iii )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	memory_set_bankptr(1,&RAM[0x100000]);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x606000c, 0x606000f, 0, 0, s1945iii_speedup_r );
	use_factory_eeprom=EEPROM_S1945III;
}

static DRIVER_INIT( dragnblz )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x606000c, 0x606000f, 0, 0, dragnblz_speedup_r );
	use_factory_eeprom=EEPROM_DRAGNBLZ;
}

static DRIVER_INIT( gnbarich )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x606000c, 0x606000f, 0, 0, gnbarich_speedup_r );
	use_factory_eeprom=EEPROM_GNBARICH;
}

static DRIVER_INIT( mjgtaste )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x606000c, 0x606000f, 0, 0, mjgtaste_speedup_r );
	use_factory_eeprom=EEPROM_DEFAULT;
	/* needs to install mahjong controls too (can select joystick in test mode tho) */
}


/*     YEAR  NAME      PARENT    MACHINE    INPUT     INIT      MONITOR COMPANY   FULLNAME FLAGS */

/* ps3-v1 */
GAME( 1997, soldivid, 0,        psikyo3v1,   soldivid, soldivid, ROT0,   "Psikyo", "Sol Divide - The Sword Of Darkness", GAME_IMPERFECT_SOUND ) // Music Tempo
GAME( 1997, s1945ii,  0,        psikyo3v1,   s1945ii,  s1945ii,  ROT270, "Psikyo", "Strikers 1945 II", GAME_IMPERFECT_GRAPHICS ) // linescroll/zoom
GAME( 1998, daraku,   0,        psikyo3v1,   daraku,   daraku,   ROT0,   "Psikyo", "Daraku Tenshi - The Fallen Angels", 0 )
GAME( 1998, sbomberb, 0,        psikyo3v1,   sbomberb, sbomberb, ROT270, "Psikyo", "Space Bomber (ver. B)", 0 )

/* ps5 */
GAME( 1998, gunbird2, 0,        psikyo5,     gunbird2, gunbird2, ROT270, "Psikyo", "Gunbird 2", 0 )
GAME( 1999, s1945iii, 0,        psikyo5,     s1945iii, s1945iii, ROT270, "Psikyo", "Strikers 1945 III (World) / Strikers 1999 (Japan)", GAME_IMPERFECT_GRAPHICS ) // linescroll/zoom

/* ps5v2 */
GAME( 2000, dragnblz, 0,        psikyo5,     dragnblz, dragnblz, ROT270, "Psikyo", "Dragon Blaze", 0 )
GAME( 2001, gnbarich, 0,        psikyo5,     gnbarich, gnbarich, ROT270, "Psikyo", "Gunbarich", 0 )
GAME( 2002, mjgtaste, 0,        psikyo5,     mjgtaste, mjgtaste, ROT0,   "Psikyo", "Mahjong G-Taste", GAME_NOT_WORKING )

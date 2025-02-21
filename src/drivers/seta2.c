/***************************************************************************

                          -= Newer Seta Hardware =-

                    driver by   Luca Elia (l.elia@tin.it)


CPU    :    TMP68301*

Custom :    X1-010              Sound: 8 Bit PCM
            DX-101              Sprites
            DX-102 x3

OSC:    50.0000MHz
        32.5304MHz

*   The Toshiba TMP68301 is a 68HC000 + serial I/O, parallel I/O,
    3 timers, address decoder, wait generator, interrupt controller,
    all integrated in a single chip.

-------------------------------------------------------------------------------------------
Ordered by Board        Year    Game                                    By
-------------------------------------------------------------------------------------------
P-FG01-1                1995    Guardians / Denjin Makai II             Banpresto
PO-113A                 1994    Mobile Suit Gundam EX Revue             Banpresto
P0-123A                 1996    Wakakusamonogatari Mahjong Yonshimai    Maboroshi Ware
P0-125A ; KE (Namco)    1996    Kosodate Quiz My Angel                  Namco
P0-136A ; KL (Namco)    1997    Kosodate Quiz My Angel 2                Namco
P0-142A                 1999    Puzzle De Bowling                       Nihon System / Moss
P0-142A + extra parts   2000    Penguin Brothers                        Subsino
B0-003B                 2000    Deer Hunting USA                        Sammy
B0-003B                 2001    Turkey Hunting USA                      Sammy
B0-003B?                2001    Wing Shooting Championship              Sammy
B0-003B?                2002    Trophy Hunting - Bear & Moose           Sammy
-------------------------------------------------------------------------------------------

TODO:

- Proper emulation of the TMP68301 CPU, in a core file.
- Flip screen / Zooming support.
- Fix some graphics imperfections (e.g. color depth selection,
  "tilemap" sprites) [all done? - NS]
- I added a kludge involving a -0x10 yoffset, this fixes the lifeline in myangel.
  I didn't find a better way to do it without breaking pzlbowl's title screen.

mj4simai:
- test mode doesn't work correctly, the grid is ok but when you press a key to go to the
  next screen (input test) it stays up a second and then drops back into the game

myangel:
- some gfx at the end of the game (rays just before fireworks, and the border during
  the wedding) have wrong colors. You can see the rays red, green and yellow because
  that's how the palette is preinitialized by MAME, but the game never sets up those
  palette entries. The game selects color depth "1", whose meaning is uncertain, and
  color code 0 so I see no way to point to a different section of palette RAM.

- there are glitches in the bg horizontal scroll in the wedding sequence at the end of
  the game. It looks like "scrollx" should be delayed one frame wrt "xoffs".

myangel2:
- before each level, the background image is shown with completely wrong colors. It
  corrects itself when the level starts.

grdians:
- the map screen after the character selection needs zooming. There is a global
  zoom register that should affect the background map and the level picture but
  not the frontmost frame. This latter should use color 7ff (the last one)
  and ignore the individual color codes in the tiles data. Zooming is also
  used briefly in pengbros.

***************************************************************************/

/***************************************************************************

MS Gundam Ex Revue
Banpresto, 1994

This game runs on Seta/Allumer hardware

PCB Layout
----------

PO-113A   BP949KA
|----------------------------------|
|  X1-010  6264  U28               |
|                     581001   U19 |
|     U3  U5  U2  U4  581001   U17 |
|      62256   62256           U15 |
|J                             U20 |
|A    U77  68301               U18 |
|M                     *       U16 |
|M    93C46                    U23 |
|A                             U22 |
|                              U21 |
|  DSW1            50MHz           |
|  DSW2       PAL  32.5304MHz      |
|       20MHz PAL                  |
|----------------------------------|

Notes:
      *: unknown QFP208 (has large heatsink on it). Should be similar to other known
         graphics chips used on Seta hardware of this era.
      68301 clock: 16.000MHz
      VSync: 60Hz

***************************************************************************/

/***************************************************************************

Guardians
Banpresto, 1995

This hardware is not common Banpresto hardware. Possibly licenced
to them from another manufacturer? Or an early design that they decided
not to use for future games? Either way, this game is _extremely_ rare :-)

PCB Layout
----------

P-FG01-1
------------------------------------------------------
|        X1-010 6264          U32 CXK581000          |
|                                 CXK581000      U16 |
|                                                    |
|                                                U20 |
|    U3 U5 U2 U4 62256 CXK58257                      |
|                62256 CXK58257                  U15 |
|                                                    |
|J                                               U19 |
|A    TMP68301AF-16                                  |
|M                                               U18 |
|M                           NEC                     |
|A          NEC              DX-101              U22 |
|           DX-102                                   |
|                                                U17 |
|                   PAL   50MHz                      |
|                                                U21 |
|           DSW1(8)                                  |
|           DSW2(8)                   CXK58257 NEC   |
|                                     CXK58257 DX-102|
------------------------------------------------------

Notes:
      HSync: 15.23kHz
      VSync: 58.5Hz

***************************************************************************/

/***************************************************************************

                            Penguin Brothers (Japan)

(c)2000 Subsino
Board:  P0-142A
CPU:    TMP68301 (68000 core)

OSC:    50.0000MHz
        32.5304MHz
        28.0000MHz

Chips.: DX-101
        DX-102 x3
Sound:  X1-010

Notes:  pzlbowl PCB with extra parts:
        28MHz OSC
        2x 62256 SRAM
        74HC00

***************************************************************************/

/***************************************************************************

Sammy USA Outdoor Shooting Series PCB

Deer Hunting USA (c) 2000 Sammy USA
Turkey Hunting USA (c) 2001 Sammy USA

Wing Shooting Championship (c) 2001 Sammy USA*
Trophy Hunting - Bear & Moose (c) 2002 Sammy USA*

* Slightly different PCB then listed below.  Two Player input / connection ports

   CPU: Toshiba TMP68301AF-16 (100 Pin PQFP)
 Video: NEC DX-101 (240 Pin PQFP)
        NEC DX-102 (52 Pin PQFP x3)
 Sound: X1-010 (Mitsubishi M60016 Gate Array, 80 Pin PQFP)
EEPROM: 93LC46BX (1K Low-power 64 x 16-bit organization serial EEPROM)
   OSC: 50MHz & 28MHz
 Other: 8 Position Dipswitch x 2
        Lattice ispLSI2032
        Lattice isp1016E

PCB Number: B0-003B
+-----------------------------------------------------------+
|             VOL                          +------+         |
|                                          |X1-010|     M1  |
|   +---+ +---+                            |M60016|         |
|   |   | |   |  M    M                    |CALRUA|  +---+  |
+-+ | U | | U |  2    1                    +------+  |   |  |
  | | 0 | | 0 |                                      |   |  |
+-+ | 7 | | 6 |  M    M                              | U |  |
|   |   | |   |  2    1                              | 1 |  |
|   +---+ +---+                                      | 8 |  |
|                                          Lattice   |   |  |
|   D +---+  C                            ispLSI2032 |   |  |
|   S |DX |  N   BAT1           +-------+            +---+  |
|   W |102|  5                  |Toshiba|  D                |
|   1 +---+                     |  TMP  |  S EEPROM       C |
|            C                  | 68301 |  W              N |
|            N  Lattice         +-------+  2              2 |
|            6  isp1016E                                    |
|                               +----------+    50MHz       |
|     +---+                     |          |                |
|     |DX |  SW1                |   NEC    |    M   M       |
|     |102|                     |  DX-101  |    3   3       |
|     +---+         M  M        |          |                |
|                   1  1        |          |                |
|                               +----------+                |
|                                                           |
|                             +---+      +---++---++---+    |
|                             |   |      |   ||   ||   |    |
|     +---+        28MHz      |   |      |   ||   ||   |    |
+-+   |DX |                   | U |      | U || U || U |    |
  |   |102|                   | 4 |      | 4 || 3 || 3 |    |
+-+   +---+                   | 0 |      | 1 || 8 || 9 |    |
|                             |   |      |   ||   ||   |    |
|                             |   |      |   ||   ||   |    |
|                             +---+      +---++---++---+    |
+-----------------------------------------------------------+

Ram M1 are Toshiba TC55257DFL-70L
Ram M2 are NEC D43001GU-70L
Ram M3 are ISSI IS62C1024L-70Q

U06 Program rom ST27C801 (even)
U07 Program rom ST27C801 (odd)

U18 Mask rom (Samples 23C32000 32Mbit (read as 27C322))

U38 - U40 Mask roms (Graphics 23c64020 64Mbit) - 23C64020 read as 27C322 with pin11 +5v & 27C322 with pin11 GND

***************************************************************************/

#include "driver.h"
#include "machine/tmp68301.h"
#include "machine/eeprom.h"
#include "sound/x1_010.h"
#include "seta.h"

/***************************************************************************


                            Memory Maps - Main CPU


***************************************************************************/

WRITE16_HANDLER( seta2_sound_bank_w )
{
	if (ACCESSING_LSB)
	{
		UINT8 *ROM = memory_region( REGION_SOUND1 );
		int banks = (memory_region_length( REGION_SOUND1 ) - 0x100000) / 0x20000;
		if (data >= banks)
		{
			logerror("CPU #0 PC %06X: invalid sound bank %04X\n",activecpu_get_pc(),data);
			data %= banks;
		}
		memcpy(ROM + offset * 0x20000, ROM + 0x100000 + data * 0x20000, 0x20000);
	}
}


/***************************************************************************
                                Guardians
***************************************************************************/

static WRITE16_HANDLER( grdians_lockout_w )
{
	if (ACCESSING_LSB)
	{
		// initially 0, then either $25 (coin 1) or $2a (coin 2)
		coin_counter_w(0,data & 0x01);	// or 0x04
		coin_counter_w(1,data & 0x02);	// or 0x08
	}
//  ui_popup("%04X", data & 0xffff);
}

static ADDRESS_MAP_START( grdians_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x304000, 0x30ffff) AM_READ(MRA16_RAM					)	// ? seems tile data
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_0_word_r		)	// DSW 1
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_1_word_r		)	// DSW 2
	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_2_word_r		)	// P1
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_3_word_r		)	// P2
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_4_word_r		)	// Coins
	AM_RANGE(0x70000c, 0x70000d) AM_READ(watchdog_reset16_r		)	// Watchdog
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(seta_sound_word_r 		)	// Sound
	AM_RANGE(0xc00000, 0xc3ffff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0xc40000, 0xc4ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0xfffc00, 0xffffff) AM_READ(MRA16_RAM					)	// TMP68301 Registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( grdians_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM							)	// RAM
	AM_RANGE(0x304000, 0x30ffff) AM_WRITE(MWA16_RAM							)	// ? seems tile data
	AM_RANGE(0x800000, 0x800001) AM_WRITE(grdians_lockout_w					)
	AM_RANGE(0xb00000, 0xb03fff) AM_WRITE(seta_sound_word_w 				)	// Sound
	AM_RANGE(0xc00000, 0xc3ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xc40000, 0xc4ffff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0xc50000, 0xc5ffff) AM_WRITE(MWA16_RAM							)	// cleared
	AM_RANGE(0xc60000, 0xc6003f) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs		)	// Video Registers
	AM_RANGE(0xe00010, 0xe0001f) AM_WRITE(seta2_sound_bank_w				)	// Samples Banks
	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(tmp68301_regs_w) AM_BASE(&tmp68301_regs	)	// TMP68301 Registers
ADDRESS_MAP_END

/***************************************************************************
                        Mobile Suit Gundam EX Revue
***************************************************************************/

static NVRAM_HANDLER(93C46_gundamex)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C46);
		if (file)
		{
			EEPROM_load(file);
		}
		else
		{
			int length;
			UINT8 *dat;

			dat = EEPROM_get_data_pointer(&length);
			dat[0]=0x70;
			dat[1]=0x08;
		}
	}
}

READ16_HANDLER( gundamex_eeprom_r )
{
	return ((EEPROM_read_bit() & 1)) << 3;
}

WRITE16_HANDLER( gundamex_eeprom_w )
{
		EEPROM_set_clock_line((data & 0x2) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x1);
		EEPROM_set_cs_line((data & 0x4) ? CLEAR_LINE : ASSERT_LINE);
}

static ADDRESS_MAP_START( gundamex_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x500000, 0x57ffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_0_word_r		)	// DSW 1
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_1_word_r		)	// DSW 2
	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_2_word_r		)	// P1
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_3_word_r		)	// P2
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_4_word_r		)	// Coins
	AM_RANGE(0x700008, 0x700009) AM_READ(input_port_5_word_r		)	// P1
	AM_RANGE(0x70000a, 0x70000b) AM_READ(input_port_6_word_r		)	// P2
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(seta_sound_word_r 		)	// Sound
	AM_RANGE(0xfffd0a, 0xfffd0b) AM_READ(gundamex_eeprom_r		)	// parallel data register
ADDRESS_MAP_END

static ADDRESS_MAP_START( gundamex_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM							)	// RAM
	AM_RANGE(0x500000, 0x57ffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x70000c, 0x70000d) AM_WRITE(watchdog_reset16_w				)
	AM_RANGE(0x800000, 0x800001) AM_WRITE(grdians_lockout_w					)
	AM_RANGE(0xb00000, 0xb03fff) AM_WRITE(seta_sound_word_w 				)	// Sound
	AM_RANGE(0xc00000, 0xc3ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xc40000, 0xc4ffff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0xc50000, 0xc5ffff) AM_WRITE(MWA16_RAM							)	// cleared
	AM_RANGE(0xc60000, 0xc6003f) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs		)	// Video Registers
	AM_RANGE(0xe00010, 0xe0001f) AM_WRITE(seta2_sound_bank_w				)	// Samples Banks
	AM_RANGE(0xfffd0a, 0xfffd0b) AM_WRITE(gundamex_eeprom_w 				)   // parallel data register
	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(tmp68301_regs_w) AM_BASE(&tmp68301_regs	)	// TMP68301 Registers
ADDRESS_MAP_END

/***************************************************************************
                      Wakakusamonogatari Mahjong Yonshimai
***************************************************************************/

static int keyboard_row;

static READ16_HANDLER( mj4simai_p1_r )
{
	switch (keyboard_row)
	{
		case 0x01: return readinputport(3);
		case 0x02: return readinputport(4);
		case 0x04: return readinputport(5);
		case 0x08: return readinputport(6);
		case 0x10: return readinputport(7);
		default:   logerror("p1_r with keyboard_row = %02x\n",keyboard_row); return 0xffff;
	}
}

static WRITE16_HANDLER( mj4simai_keyboard_w )
{
	if (ACCESSING_LSB)
		keyboard_row = data & 0xff;
}

static ADDRESS_MAP_START( mj4simai_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x600000, 0x600001) AM_READ(mj4simai_p1_r				)	// P1
	AM_RANGE(0x600002, 0x600003) AM_READ(mj4simai_p1_r				)	// P2, but I'm using P1 again
	AM_RANGE(0x600006, 0x600007) AM_READ(watchdog_reset16_r		)	// Watchdog
	AM_RANGE(0x600100, 0x600101) AM_READ(input_port_2_word_r		)	//
	AM_RANGE(0x600300, 0x600301) AM_READ(input_port_0_word_r		)	// DSW 1
	AM_RANGE(0x600302, 0x600303) AM_READ(input_port_1_word_r		)	// DSW 2
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(seta_sound_word_r 		)	// Sound
	AM_RANGE(0xc00000, 0xc3ffff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0xc40000, 0xc4ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0xfffc00, 0xffffff) AM_READ(MRA16_RAM					)	// TMP68301 Registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( mj4simai_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x600004, 0x600005) AM_WRITE(mj4simai_keyboard_w			)	// select keyboard row to read
	AM_RANGE(0x600200, 0x600201) AM_WRITE(MWA16_NOP						)	// Leds? Coins?
	AM_RANGE(0x600300, 0x60030f) AM_WRITE(seta2_sound_bank_w			)	// Samples Banks
	AM_RANGE(0xb00000, 0xb03fff) AM_WRITE(seta_sound_word_w 			)	// Sound
	AM_RANGE(0xc00000, 0xc3ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xc40000, 0xc4ffff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0xc60000, 0xc6003f) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs	)	// Video Registers
	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(tmp68301_regs_w) AM_BASE(&tmp68301_regs	)	// TMP68301 Registers
ADDRESS_MAP_END


/***************************************************************************
                            Kosodate Quiz My Angel
***************************************************************************/

static ADDRESS_MAP_START( myangel_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_2_word_r		)	// P1
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_3_word_r		)	// P2
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_4_word_r		)	// Coins
	AM_RANGE(0x700006, 0x700007) AM_READ(watchdog_reset16_r		)	// Watchdog
	AM_RANGE(0x700300, 0x700301) AM_READ(input_port_0_word_r		)	// DSW 1
	AM_RANGE(0x700302, 0x700303) AM_READ(input_port_1_word_r		)	// DSW 2
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(seta_sound_word_r 		)	// Sound
	AM_RANGE(0xc00000, 0xc3ffff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0xc40000, 0xc4ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0xfffc00, 0xffffff) AM_READ(MRA16_RAM					)	// TMP68301 Registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( myangel_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x700200, 0x700201) AM_WRITE(MWA16_NOP						)	// Leds? Coins?
	AM_RANGE(0x700310, 0x70031f) AM_WRITE(seta2_sound_bank_w			)	// Samples Banks
	AM_RANGE(0xb00000, 0xb03fff) AM_WRITE(seta_sound_word_w 			)	// Sound
	AM_RANGE(0xc00000, 0xc3ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xc40000, 0xc4ffff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0xc60000, 0xc6003f) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs	)	// Video Registers
	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(tmp68301_regs_w) AM_BASE(&tmp68301_regs	)	// TMP68301 Registers
ADDRESS_MAP_END


/***************************************************************************
                            Kosodate Quiz My Angel 2
***************************************************************************/

static ADDRESS_MAP_START( myangel2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_2_word_r		)	// P1
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_3_word_r		)	// P2
	AM_RANGE(0x600004, 0x600005) AM_READ(input_port_4_word_r		)	// Coins
	AM_RANGE(0x600006, 0x600007) AM_READ(watchdog_reset16_r		)	// Watchdog
	AM_RANGE(0x600300, 0x600301) AM_READ(input_port_0_word_r		)	// DSW 1
	AM_RANGE(0x600302, 0x600303) AM_READ(input_port_1_word_r		)	// DSW 2
	AM_RANGE(0xb00000, 0xb03fff) AM_READ(seta_sound_word_r 		)	// Sound
	AM_RANGE(0xd00000, 0xd3ffff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0xd40000, 0xd4ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0xfffc00, 0xffffff) AM_READ(MRA16_RAM					)	// TMP68301 Registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( myangel2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x600200, 0x600201) AM_WRITE(MWA16_NOP						)	// Leds? Coins?
	AM_RANGE(0x600300, 0x60030f) AM_WRITE(seta2_sound_bank_w			)	// Samples Banks
	AM_RANGE(0xb00000, 0xb03fff) AM_WRITE(seta_sound_word_w 			)	// Sound
	AM_RANGE(0xd00000, 0xd3ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xd40000, 0xd4ffff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0xd60000, 0xd6003f) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs	)	// Video Registers
	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(tmp68301_regs_w) AM_BASE(&tmp68301_regs	)	// TMP68301 Registers
ADDRESS_MAP_END


/***************************************************************************
                                Puzzle De Bowling
***************************************************************************/

/*  The game checks for a specific value read from the ROM region.
    The offset to use is stored in RAM at address 0x20BA16 */
READ16_HANDLER( pzlbowl_protection_r )
{
	UINT32 address = (program_read_word(0x20ba16) << 16) | program_read_word(0x20ba18);
	return memory_region(REGION_CPU1)[address - 2];
}

READ16_HANDLER( pzlbowl_coins_r )
{
	return readinputport(4) | (rand() & 0x80 );
}

WRITE16_HANDLER( pzlbowl_coin_counter_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,data & 0x10);
		coin_counter_w(1,data & 0x20);
	}
}

static ADDRESS_MAP_START( pzlbowl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x400300, 0x400301) AM_READ(input_port_0_word_r		)	// DSW 1
	AM_RANGE(0x400302, 0x400303) AM_READ(input_port_1_word_r		)	// DSW 2
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_2_word_r		)	// P1
	AM_RANGE(0x500002, 0x500003) AM_READ(input_port_3_word_r		)	// P2
	AM_RANGE(0x500004, 0x500005) AM_READ(pzlbowl_coins_r			)	// Coins + Protection?
	AM_RANGE(0x500006, 0x500007) AM_READ(watchdog_reset16_r		)	// Watchdog
	AM_RANGE(0x700000, 0x700001) AM_READ(pzlbowl_protection_r		)	// Protection
	AM_RANGE(0x800000, 0x83ffff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0x840000, 0x84ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x900000, 0x903fff) AM_READ(seta_sound_word_r 		)	// Sound
	AM_RANGE(0xfffc00, 0xffffff) AM_READ(MRA16_RAM					)	// TMP68301 Registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( pzlbowl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x400300, 0x40030f) AM_WRITE(seta2_sound_bank_w			)	// Samples Banks
	AM_RANGE(0x500004, 0x500005) AM_WRITE(pzlbowl_coin_counter_w		)	// Coins Counter
	AM_RANGE(0x800000, 0x83ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x840000, 0x84ffff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0x860000, 0x86003f) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs	)	// Video Registers
	AM_RANGE(0x900000, 0x903fff) AM_WRITE(seta_sound_word_w 			)	// Sound
	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(tmp68301_regs_w) AM_BASE(&tmp68301_regs	)	// TMP68301 Registers
ADDRESS_MAP_END


/***************************************************************************
                            Penguin Bros
***************************************************************************/

static ADDRESS_MAP_START( penbros_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x210000, 0x23ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x300000, 0x30ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x500300, 0x500301) AM_READ(input_port_0_word_r		)	// DSW 1
	AM_RANGE(0x500302, 0x500303) AM_READ(input_port_1_word_r		)	// DSW 2
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_2_word_r		)	// P1
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_3_word_r		)	// P2
	AM_RANGE(0x600004, 0x600005) AM_READ(input_port_4_word_r		)	// Coins
	AM_RANGE(0x600006, 0x600007) AM_READ(watchdog_reset16_r		)	// Watchdog
//  AM_RANGE(0x700000, 0x700001) AM_READ(pzlbowl_protection_r       )   // Protection
	AM_RANGE(0xb00000, 0xb3ffff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0xb40000, 0xb4ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0xa00000, 0xa03fff) AM_READ(seta_sound_word_r 		)	// Sound
	AM_RANGE(0xfffc00, 0xffffff) AM_READ(MRA16_RAM					)	// TMP68301 Registers
ADDRESS_MAP_END

static ADDRESS_MAP_START( penbros_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x300000, 0x30ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x210000, 0x23ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x500300, 0x50030f) AM_WRITE(seta2_sound_bank_w			)	// Samples Banks
	AM_RANGE(0x600004, 0x600005) AM_WRITE(pzlbowl_coin_counter_w		)	// Coins Counter
	AM_RANGE(0xb00000, 0xb3ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xb40000, 0xb4ffff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0xb60000, 0xb6003f) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs	)	// Video Registers
	AM_RANGE(0xa00000, 0xa03fff) AM_WRITE(seta_sound_word_w 			)	// Sound
	AM_RANGE(0xfffc00, 0xffffff) AM_WRITE(tmp68301_regs_w) AM_BASE(&tmp68301_regs	)	// TMP68301 Registers
ADDRESS_MAP_END


/***************************************************************************
                            Sammy Outdoor Shooting
***************************************************************************/

static READ16_HANDLER( samshoot_lightgun1_r )
{
	return (readinputport(2) << 8) | readinputport(3);
}

static READ16_HANDLER( samshoot_lightgun2_r )
{
	return (readinputport(4) << 8) | readinputport(5);
}

static WRITE16_HANDLER( samshoot_coin_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 0x10);
		coin_counter_w(1, data & 0x20);
		// Are these connected? They are set in I/O test
		coin_lockout_w(0,~data & 0x40);
		coin_lockout_w(1,~data & 0x80);
	}
//  popmessage("%04x",data);
}

static ADDRESS_MAP_START( samshoot_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x1fffff ) AM_ROM
	AM_RANGE( 0x200000, 0x20ffff ) AM_RAM
	AM_RANGE( 0x300000, 0x30ffff ) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE( 0x400000, 0x400001 ) AM_READ( input_port_0_word_r )	// DSW 1
	AM_RANGE( 0x400002, 0x400003 ) AM_READ( input_port_9_word_r )	// Buttons
	AM_RANGE( 0x400300, 0x40030f ) AM_WRITE( seta2_sound_bank_w )	// Samples Banks
	AM_RANGE( 0x500000, 0x500001 ) AM_READ( samshoot_lightgun1_r )	// P1
	AM_RANGE( 0x580000, 0x580001 ) AM_READ( samshoot_lightgun2_r )	// P2
	AM_RANGE( 0x700000, 0x700001 ) AM_READ( input_port_6_word_r )	// Trigger
	AM_RANGE( 0x700002, 0x700003 ) AM_READ( input_port_7_word_r )	// Pump
	AM_RANGE( 0x700004, 0x700005 ) AM_READWRITE( input_port_8_word_r, samshoot_coin_w )	// Coins
	AM_RANGE( 0x700006, 0x700007 ) AM_READ( watchdog_reset16_r )	// Watchdog?
	AM_RANGE( 0x800000, 0x83ffff ) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)	// Sprites
	AM_RANGE( 0x840000, 0x84ffff ) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE( 0x860000, 0x86003f ) AM_WRITE(seta2_vregs_w) AM_BASE(&seta2_vregs)	// Video Registers
	AM_RANGE( 0x900000, 0x903fff ) AM_READWRITE(seta_sound_word_r, seta_sound_word_w	)	// Sound
	AM_RANGE( 0xfffd0a, 0xfffd0b ) AM_READ( input_port_1_word_r )	// parallel data register (DSW 2)
	AM_RANGE( 0xfffc00, 0xffffff ) AM_READWRITE( MRA16_RAM, tmp68301_regs_w) AM_BASE(&tmp68301_regs )	// TMP68301 Registers
ADDRESS_MAP_END



/***************************************************************************

                                Input Ports

***************************************************************************/

/***************************************************************************
                        Mobile Suit Gundam EX Revue
***************************************************************************/

INPUT_PORTS_START( gundamex )
	PORT_START_TAG("IN0")	// $600000.w
	PORT_DIPNAME( 0x0001, 0x0001, "1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Freeze" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Show Targets" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")	// $600002.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0000, "3 Coins/5 Credits" )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Debug Mode" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// $700000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")	// $700002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4")	// $700004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN ) //jumper pad
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Language ) ) 		 //jumper pad
	PORT_DIPSETTING(      0x0020, DEF_STR( English ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Japanese ) )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("IN5")	// $700008.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW,  IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("IN6")	// $70000a.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW,  IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************
                                Guardians
***************************************************************************/

INPUT_PORTS_START( grdians )
	PORT_START_TAG("IN0")	// $600000.w
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy )    )	// 0
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal )  )	// 1
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard )    )	// 2
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )	// 3
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Title" )
	PORT_DIPSETTING(      0x0008, "Guardians" )
	PORT_DIPSETTING(      0x0000, "Denjin Makai II" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0030, "2" )
	PORT_DIPSETTING(      0x0010, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")	// $600002.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// $700000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")	// $700002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4")	// $700004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
                      Wakakusamonogatari Mahjong Yonshimai
***************************************************************************/

INPUT_PORTS_START( mj4simai )
	PORT_START_TAG("IN0")	// $600300.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Tumo Pin" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")	// $600302.w
	PORT_DIPNAME( 0x0007, 0x0004, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0004, "0" )
	PORT_DIPSETTING(      0x0003, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0001, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPSETTING(      0x0007, "5" )
	PORT_DIPSETTING(      0x0006, "6" )
	PORT_DIPSETTING(      0x0005, "7" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Select Girl" )
	PORT_DIPSETTING(      0x0010, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Com Put" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// $600100.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")	// $600000(0)
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4")	// $600000(1)
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_BET )
	PORT_BIT(0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START_TAG("IN5")	// $600000(2)
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN6")	// $600000(3)
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN7")	// $600000(4)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_BIG )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL )
	PORT_BIT(  0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
                            Kosodate Quiz My Angel
***************************************************************************/

INPUT_PORTS_START( myangel )
	PORT_START_TAG("IN0")	// $700300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 1-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")	// $700302.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Push Start To Freeze (Cheat)")
	PORT_DIPSETTING(      0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2") //$700000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3") //$700002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4") //$700004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
                            Kosodate Quiz My Angel 2
***************************************************************************/

INPUT_PORTS_START( myangel2 )
	PORT_START_TAG("IN0") //$600300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 1-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1") //$600302.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2") //$600000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3") //$600002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4") //$600004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
                                Puzzle De Bowling
***************************************************************************/

INPUT_PORTS_START( pzlbowl )
	PORT_START_TAG("IN0") //$400300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( Normal ) )
//  PORT_DIPSETTING(      0x0028, DEF_STR( Normal ) )
//  PORT_DIPSETTING(      0x0020, DEF_STR( Normal ) )
//  PORT_DIPSETTING(      0x0018, DEF_STR( Normal ) )
//  PORT_DIPSETTING(      0x0010, DEF_STR( Normal ) )
//  PORT_DIPSETTING(      0x0008, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x0040, "1" )
	PORT_DIPSETTING(      0x00c0, "2" )
//  PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0080, "3" )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1") //$400302.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
//  PORT_DIPSETTING(      0x0002, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
//  PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Join In" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Language ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( English ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Japanese ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2") //$500000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3") //$500002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4") //$500004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)	// unused, test mode shows it
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL  )	// Protection?

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************
                            Penguin Bros
***************************************************************************/

INPUT_PORTS_START( penbros )
	PORT_START_TAG("IN0") //$500300.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1") //$500302.w
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0004, "4" )
	PORT_DIPSETTING(      0x0008, "5" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0010, "150k and 500k" )
	PORT_DIPSETTING(      0x0030, "200k and 700k" )
	PORT_DIPSETTING(      0x0000, "Every 250k" )	// no extra life after the one at 1500k
	PORT_DIPSETTING(      0x0020, DEF_STR( None ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x00c0, "2" )
	PORT_DIPSETTING(      0x0040, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPSETTING(      0x0000, "5" )

	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2") //$600000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)	// unsure if used
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3") //$600002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)	// unsure if used
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4") //$600004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)	// unused, test mode shows it
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
                            Sammy Outdoor Shooting
***************************************************************************/

INPUT_PORTS_START( deerhunt )
	PORT_START_TAG("DSW1") // IN0 - $400000.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Discount To Continue" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE(         0x0080, IP_ACTIVE_LOW )
	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW2") // IN1 - fffd0a.w
	PORT_DIPNAME( 0x0001, 0x0001, "Vert. Flip Screen" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Horiz. Flip Screen" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Difficulty ) ) 
	PORT_DIPSETTING(      0x0010, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Blood Color" ) 
	PORT_DIPSETTING(      0x0020, "Red" )
	PORT_DIPSETTING(      0x0000, "Yellow" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0040, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0080, 0x0080, "Gun Type" )
	PORT_DIPSETTING(      0x0080, "Pump Action" )
	PORT_DIPSETTING(      0x0000, "Hand Gun" )
	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("GUN1Y") // IN2 - $500000.b
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x08,0xf8) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("GUN1X")	// IN3 - $500001.b
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0x25,0xc5) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("GUN2Y")	// IN4 - $580000.b
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )	// P2 gun, read but not used

	PORT_START_TAG("GUN2X")	// IN5 - $580001.b
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )	// P2 gun, read but not used

	PORT_START_TAG("TRIGGER")	// IN6 - $700000
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SPECIAL )	// trigger
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0xff3f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("PUMP")	// IN7 - $700003.b
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SPECIAL )	// pump
	PORT_BIT( 0xffbf, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("COIN")	// IN8 - $700005.b
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("BUTTONS")	// IN9 - $400002
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	// trigger
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	// pump
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( turkhunt )
	PORT_INCLUDE(deerhunt)

	PORT_MODIFY("DSW2") // IN1 - fffd0a.w
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
INPUT_PORTS_END


INPUT_PORTS_START( wschamp )
	PORT_INCLUDE(deerhunt)

	PORT_MODIFY("DSW1")	// IN0 - $400000.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0009, "4 Coins Start, 4 Coins Continue" )
	PORT_DIPSETTING(      0x0008, "4 Coins Start, 3 Coins Continue" )
	PORT_DIPSETTING(      0x0007, "4 Coins Start, 2 Coins Continue" )
	PORT_DIPSETTING(      0x0006, "4 Coins Start, 1 Coin Continue" )
	PORT_DIPSETTING(      0x000c, "3 Coins Start, 3 Coins Continue" )
	PORT_DIPSETTING(      0x000b, "3 Coins Start, 2 Coins Continue" )
	PORT_DIPSETTING(      0x000a, "3 Coins Start, 1 Coin Continue" )
	PORT_DIPSETTING(      0x000e, "2 Coins Start, 2 Coins Continue" )
	PORT_DIPSETTING(      0x000d, "2 Coins Start, 1 Coin Continue" )
	PORT_DIPSETTING(      0x000f, "1 Coin Start, 1 Coin Continue" )
	PORT_DIPSETTING(      0x0005, "1 Coin 2 Credits, 1 Credit Start & Continue" )
	PORT_DIPSETTING(      0x0004, "1 Coin 3 Credits, 1 Credit Start & Continue" )
	PORT_DIPSETTING(      0x0003, "1 Coin 4 Credits, 1 Credit Start & Continue" )
	PORT_DIPSETTING(      0x0002, "1 Coin 5 Credits, 1 Credit Start & Continue" )
	PORT_DIPSETTING(      0x0001, "1 Coin 6 Credits, 1 Credit Start & Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_SERVICE(         0x0080, IP_ACTIVE_LOW )
	PORT_BIT(     0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("DSW2") // IN1 - fffd0a.w
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x0000, "3" )

	PORT_MODIFY("GUN2Y") // IN4 - $580000.b
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x08,0xf8) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_MODIFY("GUN2X")	// IN5 - $580001.b
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0x25,0xc5) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_MODIFY("TRIGGER")	// IN6 - $700000
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SPECIAL )	// trigger P2
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SPECIAL )	// trigger P1
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0xff1f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("PUMP")	// IN7 - $700003.b
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SPECIAL )	// pump P2
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SPECIAL )	// pump P1
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0xff1f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("COIN")	// IN8 - $700005.b
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("BUTTONS")	// IN9 - $400002
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	// trigger P1
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	// pump P1
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )	PORT_PLAYER(2)	// trigger P2
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )	PORT_PLAYER(2)	// pump P2
	PORT_BIT( 0xffcc, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( trophyh )
	PORT_INCLUDE(wschamp)

	PORT_MODIFY("DSW2") // IN1 - fffd0a.w
	PORT_DIPNAME( 0x0020, 0x0020, "Blood Color" )  /* WSChamp doesn't use Blood Color, so add it back in */
	PORT_DIPSETTING(      0x0020, "Red" )
	PORT_DIPSETTING(      0x0000, "Yellow" )
INPUT_PORTS_END


/***************************************************************************


                            Graphics Layouts


***************************************************************************/

static const gfx_layout layout_4bpp_lo =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{	RGN_FRAC(1,4)+8,RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

static const gfx_layout layout_4bpp_hi =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{	RGN_FRAC(3,4)+8,RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8,RGN_FRAC(2,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

static const gfx_layout layout_6bpp =
{
	8,8,
	RGN_FRAC(1,4),
	6,
	{
		RGN_FRAC(2,4)+8,RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8,RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

static const gfx_layout layout_8bpp =
{
	8,8,
	RGN_FRAC(1,4),
	8,
	{	RGN_FRAC(3,4)+8,RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8,RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8,RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

static const gfx_layout layout_3bpp_lo =
{
	8,8,
	RGN_FRAC(1,4),
	3,
	{	                RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0		},
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

static const gfx_layout layout_2bpp_hi =
{
	8,8,
	RGN_FRAC(1,4),
	2,
	{	RGN_FRAC(2,4)+8,RGN_FRAC(2,4)+0 },
	{	STEP8(0,1)		},
	{	STEP8(0,8*2)	},
	8*8*2
};

/*  Tiles are 8bpp, but the hardware is additionally able to discard
    some bitplanes and use the low 4 bits only, or the high 4 bits only */
static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_4bpp_lo, 0, 0x8000/16 },
	{ REGION_GFX1, 0, &layout_4bpp_hi, 0, 0x8000/16 },
	{ REGION_GFX1, 0, &layout_6bpp,    0, 0x8000/16 },	/* 6bpp, but 4bpp granularity */
	{ REGION_GFX1, 0, &layout_8bpp,    0, 0x8000/16 },	/* 8bpp, but 4bpp granularity */
	{ REGION_GFX1, 0, &layout_3bpp_lo, 0, 0x8000/16 },	/* 3bpp, but 4bpp granularity */
	{ REGION_GFX1, 0, &layout_2bpp_hi, 0, 0x8000/16 },	/* ??? */
	{ -1 }
};


/***************************************************************************

                                Machine Drivers

***************************************************************************/

static INTERRUPT_GEN( seta2_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			/* VBlank is connected to INT0 (external interrupts pin 0) */
			tmp68301_external_interrupt_0();
			break;
	}
}

static INTERRUPT_GEN( samshoot_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			tmp68301_external_interrupt_0();	// vblank
			break;
		case 1:
			tmp68301_external_interrupt_2();	// to do: hook up x1-10 interrupts
			break;
	}
}

static struct x1_010_interface x1_010_sound_intf =
{
	0x0000,		/* address */
};


static MACHINE_DRIVER_START( mj4simai )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,32530400 / 2)			/* !! TMP68301 !! */
	MDRV_CPU_PROGRAM_MAP(mj4simai_readmem,mj4simai_writemem)
	MDRV_CPU_VBLANK_INT(seta2_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( tmp68301 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 0x200)
	MDRV_VISIBLE_AREA(0x40, 0x1c0-1, 0x80, 0x170-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(seta2)
	MDRV_VIDEO_UPDATE(seta2)
	MDRV_VIDEO_EOF(seta2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(X1_010, 50000000/3)
	MDRV_SOUND_CONFIG(x1_010_sound_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gundamex )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_REPLACE("main",M68000,16000000)
	MDRV_CPU_PROGRAM_MAP(gundamex_readmem,gundamex_writemem)

	MDRV_NVRAM_HANDLER(93C46_gundamex)

	/* video hardware */
	MDRV_VISIBLE_AREA(0x00, 0x180-1, 0x100, 0x1e0-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( grdians )

	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(grdians_readmem,grdians_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0x80, 0x80 + 0x130 -1, 0x80, 0x80 + 0xe8 -1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( myangel )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(myangel_readmem,myangel_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x178-1, 0x00, 0xf0-1)

	MDRV_VIDEO_START(seta2_offset)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( myangel2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(myangel2_readmem,myangel2_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x178-1, 0x00, 0xf0-1)

	MDRV_VIDEO_START(seta2_offset)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pzlbowl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(pzlbowl_readmem,pzlbowl_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0x10, 0x190-1, 0x100, 0x1f0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( penbros )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(penbros_readmem,penbros_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x140-1, 0x80, 0x160-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( samshoot )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(mj4simai)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(samshoot_map, 0)
	MDRV_CPU_VBLANK_INT(samshoot_interrupt,2)

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_UPDATE(seta2_gun)

	/* video hardware */
	MDRV_VISIBLE_AREA(0x40, 0x180-1, 0x40, 0x130-1)
MACHINE_DRIVER_END


/***************************************************************************

                                ROMs Loading

***************************************************************************/

ROM_START( gundamex )
	ROM_REGION( 0x600000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE(	  "ka002002.u2",  0x000000, 0x080000, CRC(e850f6d8) SHA1(026325e305676b1f8d3d9e7573920f8b70d7bccb) )
	ROM_LOAD16_BYTE(	  "ka002004.u3",  0x000001, 0x080000, CRC(c0fb1208) SHA1(84b25e4c73cb8e023ee5dbf69f588be98700b43f) )
	ROM_LOAD16_BYTE(	  "ka002001.u4",  0x100000, 0x080000, CRC(553ebe6b) SHA1(7fb8a159513d31a1d60520ff14e4c4d133fd3e19) )
	ROM_LOAD16_BYTE(	  "ka002003.u5",  0x100001, 0x080000, CRC(946185aa) SHA1(524911c4c510d6c3e17a7ab42c7077c2fffbf06b) )
	ROM_LOAD16_WORD_SWAP( "ka001005.u77", 0x500000, 0x080000, CRC(f01d3d00) SHA1(ff12834e99a76261d619f10d186f4b329fb9cb7a) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_ERASE)	/* Sprites */
	ROM_LOAD( "ka001009.u16",  0x0000000, 0x200000, CRC(997d8d93) SHA1(4cb4cdb7e8208af4b14483610d9d6aa5e13acd89) )
	ROM_LOAD( "ka001010.u18",  0x0200000, 0x200000, CRC(811b67ca) SHA1(c8cfae6f54c76d63bd625ff011c872ffb75fd2e2) )
	ROM_LOAD( "ka001011.u20",  0x0400000, 0x200000, CRC(08a72700) SHA1(fb8003aa02dd249c30a757cb43b516260b41c1bf) )
	ROM_LOAD( "ka001012.u15",  0x0800000, 0x200000, CRC(b789e4a8) SHA1(400b773f24d677a9d47466fdbbe68cb6efc1ad37) )
	ROM_LOAD( "ka001013.u17",  0x0a00000, 0x200000, CRC(d8a0201f) SHA1(fe8a2407c872adde8aec8e9340b00be4f00a2872) )
	ROM_LOAD( "ka001014.u19",  0x0c00000, 0x200000, CRC(7635e026) SHA1(116a3daab14a17faca85c4a956b356aaf0fc2276) )
	ROM_LOAD( "ka001006.u21",  0x1000000, 0x200000, CRC(6aac2f2f) SHA1(fac5478ca2941a93c57f670a058ff626e537bcde) )
	ROM_LOAD( "ka001007.u22",  0x1200000, 0x200000, CRC(588f9d63) SHA1(ed5148d09d02e3bc12c50c39c5c86e6356b2dd7a) )
	ROM_LOAD( "ka001008.u23",  0x1400000, 0x200000, CRC(db55a60a) SHA1(03d118c7284ca86219891c473e2a89489710ea27) )
	ROM_FILL(                  0x1800000, 0x600000, 0 )	/* 6bpp instead of 8bpp */

	ROM_REGION( 0x300000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "ka001015.u28", 0x100000, 0x200000, CRC(ada2843b) SHA1(09d06026031bc7558da511c3c0e29187ea0a0099) )
ROM_END

ROM_START( grdians )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "u2.bin", 0x000000, 0x080000, CRC(36adc6f2) SHA1(544e87f88179fe1342e7a06a8948ac1828e85108) )
	ROM_LOAD16_BYTE( "u3.bin", 0x000001, 0x080000, CRC(2704f416) SHA1(9081a12cbb9927d36e1c50b52aa2c6003810ee42) )
	ROM_LOAD16_BYTE( "u4.bin", 0x100000, 0x080000, CRC(bb52447b) SHA1(61433f683210ab2bc2cf1cc4b5b7a39cc5b6493d) )
	ROM_LOAD16_BYTE( "u5.bin", 0x100001, 0x080000, CRC(9c164a3b) SHA1(6d688c7af9e7e8e8d54b2e4dfbf41f59c79242eb) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE|ROMREGION_ERASE)	/* Sprites */
	ROM_LOAD( "u16.bin",  0x0000000, 0x400000, CRC(6a65f265) SHA1(6cad11f718f8bbcff464d41eb4717460769237ed) )
	ROM_LOAD( "u20.bin",  0x0600000, 0x200000, CRC(a7226ab7) SHA1(408580dd35c568ffef1ebbd87359e3ec1f867020) )
	ROM_CONTINUE(         0x0400000, 0x200000 )

	ROM_LOAD( "u15.bin",  0x0800000, 0x400000, CRC(01672dcd) SHA1(f61f60e3343cc5b6ccee391ee529966a141566db) )
	ROM_LOAD( "u19.bin",  0x0e00000, 0x200000, CRC(c0c998a0) SHA1(498fb1877527ed37412537f06a2c39ff0c60f146) )
	ROM_CONTINUE(         0x0c00000, 0x200000 )

	ROM_LOAD( "u18.bin",  0x1000000, 0x400000, CRC(967babf4) SHA1(42a6311576417c44aeaceb8ba6bb3cd7794e4882) )
	ROM_LOAD( "u22.bin",  0x1600000, 0x200000, CRC(6239997a) SHA1(87b6d6f30f152f625f82fd858c1290176c7e156e) )
	ROM_CONTINUE(         0x1400000, 0x200000 )

	ROM_LOAD( "u17.bin",  0x1800000, 0x400000, CRC(0fad0629) SHA1(1bdc8e7c5e39e83d327f14a672ec81b049112da6) )
	ROM_LOAD( "u21.bin",  0x1e00000, 0x200000, CRC(6f95e466) SHA1(28482fad16a3ac9302f152d81552e6f84a44f3e4) )
	ROM_CONTINUE(         0x1c00000, 0x200000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "u32.bin", 0x100000, 0x100000, CRC(cf0f3017) SHA1(8376d3a674f71aec72f52c72758fbc53d9feb1a1) )
ROM_END

ROM_START( mj4simai )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "ll.u2",       0x000000, 0x080000, CRC(7be9c781) SHA1(d29e579706d98909933f6bed2ee292c88ed10d2c) )
	ROM_LOAD16_BYTE( "lh1.u3",      0x000001, 0x080000, CRC(82aa3f72) SHA1(a93d5dc7cdf12f852a692759d91f6f2951b6b5b5) )
	ROM_LOAD16_BYTE( "hl.u4",       0x100000, 0x080000, CRC(226063b7) SHA1(1737baffc16ff7261f887911187ece96925fa6ff) )
	ROM_LOAD16_BYTE( "hh.u5",       0x100001, 0x080000, CRC(23aaf8df) SHA1(b3d678afce4ddef32e48d690c6d07b723dd0c28f) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "cha-03.u16",  0x0000000, 0x400000, CRC(d367429a) SHA1(b32c215ef85c3d0a4c5550cef4f5c4c0e7030b7c) )
	ROM_LOAD( "cha-04.u18",  0x0400000, 0x400000, CRC(7f2008c3) SHA1(e45d863540eb2381f5d7660d64cdfef87c890768) )
	ROM_LOAD( "cha-05.u15",  0x0800000, 0x400000, CRC(e94ec40a) SHA1(2685dbc5680b5f76688c6b4fbe40ae682c525bfe) )
	ROM_LOAD( "cha-06.u17",  0x0c00000, 0x400000, CRC(5cb0b3a9) SHA1(92fb82d45b4c46326d5796981f812e20a8ddb4f2) )
	ROM_LOAD( "cha-01.u21",  0x1000000, 0x400000, CRC(35f47b37) SHA1(4a8eb088890272f2a069e2c3f00fadf6421f7b0e) )
	ROM_LOAD( "cha-02.u22",  0x1400000, 0x400000, CRC(f6346860) SHA1(4eebd3fa315b97964fa39b88224f9de7622ba881) )
	ROM_FILL(                0x1800000, 0x800000, 0 )	/* 6bpp instead of 8bpp */

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "cha-07.u32",  0x100000, 0x400000, CRC(817519ee) SHA1(ed09740cdbf61a328f7b50eb569cf498fb749416) )
ROM_END

ROM_START( myangel )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "kq1-prge.u2", 0x000000, 0x080000, CRC(6137d4c0) SHA1(762341e11b56e4a7787a0662833b702b78aee0a9) )
	ROM_LOAD16_BYTE( "kq1-prgo.u3", 0x000001, 0x080000, CRC(4aad10d8) SHA1(a08e1c4f57c64be829e0807ae2791da947fd60aa) )
	ROM_LOAD16_BYTE( "kq1-tble.u4", 0x100000, 0x080000, CRC(e332a514) SHA1(dfd255239c80c48c9865e70681b9ddd175b8bf55) )
	ROM_LOAD16_BYTE( "kq1-tblo.u5", 0x100001, 0x080000, CRC(760cab15) SHA1(fa7ea85ec2ebfaab3111b8631ea6ea3d794d449c) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "kq1-cg2.u20", 0x000000, 0x200000, CRC(80b4e8de) SHA1(c8685c4f4e3c0415ce0ec88e0288835e504cab00) )
	ROM_LOAD( "kq1-cg0.u16", 0x200000, 0x200000, CRC(f8ae9a05) SHA1(4f3b41386a48a1608aa96b911e6b74ca775260fb) )
	ROM_LOAD( "kq1-cg3.u19", 0x400000, 0x200000, CRC(9bdc35c9) SHA1(fd0a1eb3dd10705bce5462263667353632558b58) )
	ROM_LOAD( "kq1-cg1.u15", 0x600000, 0x200000, CRC(23bd7ea4) SHA1(e925bbadc33fc2586bb18283cf989ab35f28c1e9) )
	ROM_LOAD( "kq1-cg6.u22", 0x800000, 0x200000, CRC(b25acf12) SHA1(5cca35921f3b376c3cc36f5b009eb845db2e1897) )
	ROM_LOAD( "kq1-cg4.u18", 0xa00000, 0x200000, CRC(dca7f8f2) SHA1(20595c7940a28d01bdc6610b67aaaeac61ba92e2) )
	ROM_LOAD( "kq1-cg7.u21", 0xc00000, 0x200000, CRC(9f48382c) SHA1(80dfc33a55123b5d3cdb3ed97b43a527f0254d61) )
	ROM_LOAD( "kq1-cg5.u17", 0xe00000, 0x200000, CRC(a4bc4516) SHA1(0eb11fa54d16bba1b96f9dd943a68949a3bb9a2f) )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "kq1-snd.u32", 0x100000, 0x200000, CRC(8ca1b449) SHA1(f54096fb5400843af4879135c96760485b6cb319) )
ROM_END

ROM_START( myangel2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "kqs1ezpr.u2", 0x000000, 0x080000, CRC(2469aac2) SHA1(7dade2de31252e305d24c659c4801dd4687ad1f6) )
	ROM_LOAD16_BYTE( "kqs1ozpr.u3", 0x000001, 0x080000, CRC(6336375c) SHA1(72089f77e94832e74e0512944acadeccd0dec8b0) )
	ROM_LOAD16_BYTE( "kqs1e-tb.u4", 0x100000, 0x080000, CRC(e759b4cc) SHA1(4f806a144a47935b2710f8af800ec0d771f12a18) )
	ROM_LOAD16_BYTE( "kqs1o-tb.u5", 0x100001, 0x080000, CRC(b6168737) SHA1(4c3de877c0c1dca1c43ac737a0bf231335237d3a) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "kqs1-cg4.u20", 0x0000000, 0x200000, CRC(d1802241) SHA1(52c45a13d46f7ee8043e85b99d07b1765ca93dcc) )
	ROM_LOAD( "kqs1-cg0.u16", 0x0200000, 0x400000, CRC(c21a33a7) SHA1(bc6f479a8f4c716ba79a725f160ddeb95fdedbcb) )
	ROM_LOAD( "kqs1-cg5.u19", 0x0600000, 0x200000, CRC(d86cf19c) SHA1(da5a5b576ce107433605b24d8b9dcd0abd46bcde) )
	ROM_LOAD( "kqs1-cg1.u15", 0x0800000, 0x400000, CRC(dca799ba) SHA1(8379b11472c27b1945fe7fc274c7fedf756accba) )
	ROM_LOAD( "kqs1-cg6.u22", 0x0c00000, 0x200000, CRC(3f08886b) SHA1(054546ae44ffa5d0973f4ead080fe720a340e144) )
	ROM_LOAD( "kqs1-cg2.u18", 0x0e00000, 0x400000, CRC(f7f92c7e) SHA1(24a525a15fded0de6e382b346da6bd5e7b9eced5) )
	ROM_LOAD( "kqs1-cg7.u21", 0x1200000, 0x200000, CRC(2c977904) SHA1(2589447f2471cdc414266b34aff552044c680d93) )
	ROM_LOAD( "kqs1-cg3.u17", 0x1400000, 0x400000, CRC(de3b2191) SHA1(d7d6ea07b665cfd834747d3c0776b968ce03bc6a) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "kqs1-snd.u32", 0x100000, 0x400000, CRC(792a6b49) SHA1(341b4e8f248b5032217733bada32e353c67e3888) )
ROM_END

ROM_START( pzlbowl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "kup-u06.i03", 0x000000, 0x080000, CRC(314e03ac) SHA1(999398e55161dd75570d418f4c9899e3bf311cc8) )
	ROM_LOAD16_BYTE( "kup-u07.i03", 0x000001, 0x080000, CRC(a0423a04) SHA1(9539023c5c2f2bf72ee3fb6105443ffd3d61e2f8) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "kuc-u38.i00", 0x000000, 0x400000, CRC(3db24172) SHA1(89c39963e15c53b799994185d0c8b2e795478939) )
	ROM_LOAD( "kuc-u39.i00", 0x400000, 0x400000, CRC(9b26619b) SHA1(ea7a0bf46641d15353217b01e761d1a148bee4e7) )
	ROM_LOAD( "kuc-u40.i00", 0x800000, 0x400000, CRC(7e49a2cf) SHA1(d24683addbc54515c33fb620ac500e6702bd9e17) )
	ROM_LOAD( "kuc-u41.i00", 0xc00000, 0x400000, CRC(2febf19b) SHA1(8081ac590c0463529777b5e4817305a1a6f6ea41) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "kus-u18.i00", 0x100000, 0x400000, CRC(e2b1dfcf) SHA1(fb0b8be119531a1a27efa46ed7b86b05a37ed585) )
ROM_END

ROM_START( penbros )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "u06.bin", 0x000000, 0x080000, CRC(7bbdffac) SHA1(d5766cb171b8d2e4c04a6bae37181fa5ada9d797) )
	ROM_LOAD16_BYTE( "u07.bin", 0x000001, 0x080000, CRC(d50cda5f) SHA1(fc66f55f2070b447c5db85c948ce40adc37512f7) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "u38.bin", 0x000000, 0x400000, CRC(4247b39e) SHA1(f273931293beced312e02c870bf35e9cf0c91a8b) )
	ROM_LOAD( "u39.bin", 0x400000, 0x400000, CRC(f9f07faf) SHA1(66fc4a9ad422fb384d2c775e43619137226898fc) )
	ROM_LOAD( "u40.bin", 0x800000, 0x400000, CRC(dc9e0a96) SHA1(c2c8ccf9039ee0e179b08fdd2d37f29899349cda) )
	ROM_FILL(            0xc00000, 0x400000, 0 )	/* 6bpp instead of 8bpp */

	ROM_REGION( 0x300000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "u18.bin", 0x100000, 0x200000, CRC(de4e65e2) SHA1(82d4e590c714b3e9bf0ffaf1500deb24fd315595) )
ROM_END

/* American Sammy Hunting Games */

ROM_START( deerhunt ) /* Deer Hunting USA V4.3 (11/1/2000) - The "E05" breaks version label conventions but is correct & verified */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "as0906e05.u06", 0x000000, 0x100000, CRC(20c81f17) SHA1(d41d93d6ee88738cec55f7bf3ce6be1dbec68e09) ) /* checksum 694E printed on label */
	ROM_LOAD16_BYTE( "as0907e05.u07", 0x000001, 0x100000, CRC(1731aa2a) SHA1(cffae7a99a7f960a62ef0c4454884df17a93c1a6) ) /* checksum 5D89 printed on label */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "as0901m01.u38", 0x0000000, 0x800000, CRC(1d6acf8f) SHA1(6f61fe21bebb7c87e8e6c3ef3ba73b8cf327dde9) )
	ROM_LOAD( "as0902m01.u39", 0x0800000, 0x800000, CRC(c7ca2128) SHA1(86be3a3ec2f86f61acfa3d4d261faea3c27dc378) )
	ROM_LOAD( "as0903m01.u40", 0x1000000, 0x800000, CRC(e8ef81b3) SHA1(97666942ca6cca5b8ea6451314a2aaabad9e06ba) )
	ROM_LOAD( "as0904m01.u41", 0x1800000, 0x800000, CRC(d0f97fdc) SHA1(776c9d42d03a9f61155521212305e1ed696eaf47) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "as0905m01.u18", 0x100000, 0x400000, CRC(8d8165bb) SHA1(aca7051613d260734ee787b4c3db552c336bd600) )
ROM_END

ROM_START( deerhunta ) /* Deer Hunting USA V4.2 (xx/x/2000) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "as0906e04-v4_2.u06", 0x000000, 0x100000, CRC(bb3af36f) SHA1(f04071347e8ad361bf666fcb6c0136e522f19d47) ) /* checksum 6640 printed on label */
	ROM_LOAD16_BYTE( "as0907e04-v4_2.u07", 0x000001, 0x100000, CRC(83f02117) SHA1(70fc2291bc93af3902aae88688be6a8078f7a07e) ) /* checksum 595A printed on label */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "as0901m01.u38", 0x0000000, 0x800000, CRC(1d6acf8f) SHA1(6f61fe21bebb7c87e8e6c3ef3ba73b8cf327dde9) )
	ROM_LOAD( "as0902m01.u39", 0x0800000, 0x800000, CRC(c7ca2128) SHA1(86be3a3ec2f86f61acfa3d4d261faea3c27dc378) )
	ROM_LOAD( "as0903m01.u40", 0x1000000, 0x800000, CRC(e8ef81b3) SHA1(97666942ca6cca5b8ea6451314a2aaabad9e06ba) )
	ROM_LOAD( "as0904m01.u41", 0x1800000, 0x800000, CRC(d0f97fdc) SHA1(776c9d42d03a9f61155521212305e1ed696eaf47) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "as0905m01.u18", 0x100000, 0x400000, CRC(8d8165bb) SHA1(aca7051613d260734ee787b4c3db552c336bd600) )
ROM_END

ROM_START( deerhuntb ) /* Deer Hunting USA V4.0 (6/15/2000) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "as0906e04.u06", 0x000000, 0x100000, CRC(07d9b64a) SHA1(f9aac644aab920bbac84b14836ee589ccd51f6db) ) /* checksum 7BBB printed on label */
	ROM_LOAD16_BYTE( "as0907e04.u07", 0x000001, 0x100000, CRC(19973d08) SHA1(da1cc02ce480a62ccaf94d0af1246a340f054b43) ) /* checksum 4C78 printed on label */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "as0901m01.u38", 0x0000000, 0x800000, CRC(1d6acf8f) SHA1(6f61fe21bebb7c87e8e6c3ef3ba73b8cf327dde9) )
	ROM_LOAD( "as0902m01.u39", 0x0800000, 0x800000, CRC(c7ca2128) SHA1(86be3a3ec2f86f61acfa3d4d261faea3c27dc378) )
	ROM_LOAD( "as0903m01.u40", 0x1000000, 0x800000, CRC(e8ef81b3) SHA1(97666942ca6cca5b8ea6451314a2aaabad9e06ba) )
	ROM_LOAD( "as0904m01.u41", 0x1800000, 0x800000, CRC(d0f97fdc) SHA1(776c9d42d03a9f61155521212305e1ed696eaf47) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "as0905m01.u18", 0x100000, 0x400000, CRC(8d8165bb) SHA1(aca7051613d260734ee787b4c3db552c336bd600) )
ROM_END

	/* There are known versions 3.x of Deer Hunting USA.... just none are currently dumped.  roms should be "AS0906 E03 U06" & "AS0907 E03 U07" */

ROM_START( deerhuntc ) /* Deer Hunting USA V2.x - No version number is printed to screen but "E02" in EPROM label signifies V2 */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "as0906e02.u06", 0x000000, 0x100000, CRC(190cca42) SHA1(aef63f5e8c71ed0156b8b0104c5d23872c119167) ) /* Version in program code is listed as 0.00 */
	ROM_LOAD16_BYTE( "as0907e02.u07", 0x000001, 0x100000, CRC(9de2b901) SHA1(d271bc54c41e30c0d9962eedd22f3ef2b7b8c9e5) ) /* Verified with two different sets of chips */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "as0901m01.u38", 0x0000000, 0x800000, CRC(1d6acf8f) SHA1(6f61fe21bebb7c87e8e6c3ef3ba73b8cf327dde9) )
	ROM_LOAD( "as0902m01.u39", 0x0800000, 0x800000, CRC(c7ca2128) SHA1(86be3a3ec2f86f61acfa3d4d261faea3c27dc378) )
	ROM_LOAD( "as0903m01.u40", 0x1000000, 0x800000, CRC(e8ef81b3) SHA1(97666942ca6cca5b8ea6451314a2aaabad9e06ba) )
	ROM_LOAD( "as0904m01.u41", 0x1800000, 0x800000, CRC(d0f97fdc) SHA1(776c9d42d03a9f61155521212305e1ed696eaf47) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "as0905m01.u18", 0x100000, 0x400000, CRC(8d8165bb) SHA1(aca7051613d260734ee787b4c3db552c336bd600) )
ROM_END

ROM_START( turkhunt ) /* V1.0 is currently the only known version */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "asx906e01.u06", 0x000000, 0x100000, CRC(c96266e1) SHA1(0ca462b3b0f27198e36384eee6ea5c5d4e7e1293) ) /* checksum E510 printed on label */
	ROM_LOAD16_BYTE( "asx907e01.u07", 0x000001, 0x100000, CRC(7c67b502) SHA1(6a0e8883a115dac4095d86897e7eca2a007a1c71) ) /* checksum AB40 printed on label */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "asx901m01.u38", 0x0000000, 0x800000, CRC(eabd3f44) SHA1(5a1ac986d11a8b019e18761cf4ea0a6f49fbdbfc) )
	ROM_LOAD( "asx902m01.u39", 0x0800000, 0x800000, CRC(c32130c8) SHA1(70d56ebed1f51657aaee02f95ac51589733e6eb7) )
	ROM_LOAD( "asx903m01.u40", 0x1000000, 0x800000, CRC(5f86c322) SHA1(5a72adb99eea176199f172384cb051e2b045ab94) )
	ROM_LOAD( "asx904m01.u41", 0x1800000, 0x800000, CRC(c77e0b66) SHA1(0eba30e62e4bd38c198fa6cb69fb94d002ded77a) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "asx905m01.u18", 0x100000, 0x400000, CRC(8d9dd9a9) SHA1(1fc2f3688d2c24c720dca7357bca6bf5f4016c53) )
ROM_END

ROM_START( wschamp ) /* Wing Shootiong Championship V2.00 (01/23/2002) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "as1006e03.u06", 0x000000, 0x100000, CRC(0ad01677) SHA1(63e09b9f7cc8b781af1756f86caa0cc0962ae584) ) /* checksum 421E printed on label */
	ROM_LOAD16_BYTE( "as1007e03.u07", 0x000001, 0x100000, CRC(572624f0) SHA1(0c2f67daa22f4edd66a2be990dc6cd999faff0fa) ) /* checksum A48F printed on label */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "as1001m01.u38", 0x0000000, 0x800000, CRC(92595579) SHA1(75a7131aedb18b7103677340c3cca7c91aaca2bf) )
	ROM_LOAD( "as1002m01.u39", 0x0800000, 0x800000, CRC(16c2bb08) SHA1(63926464c8bd8db7d05905a953765e645942beb4) )
	ROM_LOAD( "as1003m01.u40", 0x1000000, 0x800000, CRC(89618858) SHA1(a8bd07f233482e8f5a256af7ff9577648eb58ef4) )
	ROM_LOAD( "as1004m01.u41", 0x1800000, 0x800000, CRC(500c0909) SHA1(73ff27d46b9285f34a50a81c21c54437f21e1939) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "as1005m01.u18", 0x100000, 0x400000, CRC(e4b137b8) SHA1(4d8d15073c51f7d383282cc5755ae5b2eab6226c) )
ROM_END

ROM_START( wschampa ) /* Wing Shootiong Championship V1.01 */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "as1006e02.u06", 0x000000, 0x100000, CRC(d3d3b2b5) SHA1(2d036d795b40a4ed78bb9f7751f875cfc76276a9) ) /* checksum 31EF printed on label */
	ROM_LOAD16_BYTE( "as1007e02.u07", 0x000001, 0x100000, CRC(78ede6d9) SHA1(e6d10f52cd4c6bf97288df44911f23bb64fc012c) ) /* checksum 615E printed on label */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "as1001m01.u38", 0x0000000, 0x800000, CRC(92595579) SHA1(75a7131aedb18b7103677340c3cca7c91aaca2bf) )
	ROM_LOAD( "as1002m01.u39", 0x0800000, 0x800000, CRC(16c2bb08) SHA1(63926464c8bd8db7d05905a953765e645942beb4) )
	ROM_LOAD( "as1003m01.u40", 0x1000000, 0x800000, CRC(89618858) SHA1(a8bd07f233482e8f5a256af7ff9577648eb58ef4) )
	ROM_LOAD( "as1004m01.u41", 0x1800000, 0x800000, CRC(500c0909) SHA1(73ff27d46b9285f34a50a81c21c54437f21e1939) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "as1005m01.u18", 0x100000, 0x400000, CRC(e4b137b8) SHA1(4d8d15073c51f7d383282cc5755ae5b2eab6226c) )
ROM_END

ROM_START( trophyh ) /* V1.0 is currently the only known version */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "as1106e01.u06", 0x000000, 0x100000, CRC(b4950882) SHA1(2749f7ffc5b543c9f39815f0913a1d1e385b63f4) ) /* checksum D8DA printed on label */
	ROM_LOAD16_BYTE( "as1107e01.u07", 0x000001, 0x100000, CRC(19ee67cb) SHA1(e75ce66d3ff5aad46ba997c09d6514260e617f55) ) /* checksum CEEF printed on label */

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "as1101m01.u38", 0x0000000, 0x800000, CRC(855ed675) SHA1(84ce229a9feb6331413253a5aed10b362e8102e5) )
	ROM_LOAD( "as1102m01.u39", 0x0800000, 0x800000, CRC(d186d271) SHA1(3c54438b35adfab8be91df0a633270d6db49beef) )
	ROM_LOAD( "as1103m01.u40", 0x1000000, 0x800000, CRC(adf8a54e) SHA1(bb28bf219d18082246f7964851a5c49b9c0ba7f5) )
	ROM_LOAD( "as1104m01.u41", 0x1800000, 0x800000, CRC(387882e9) SHA1(0fdd0c77dabd1066c6f3bd64e357236a76f524ab) )

	ROM_REGION( 0x500000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave 1MB empty (addressable by the chip) */
	ROM_LOAD( "as1105m01.u18", 0x100000, 0x400000, CRC(633d0df8) SHA1(3401c424f5c207ef438a9269e0c0e7d482771fed) )
ROM_END


GAME( 1994, gundamex,  0,        gundamex, gundamex, 0,        ROT0, "Banpresto",             "Mobile Suit Gundam EX Revue", 0 )
GAME( 1995, grdians,   0,        grdians,  grdians,  0,        ROT0, "Banpresto",             "Guardians / Denjin Makai II",                  GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )	// Displays (c) Winky Soft at game's end.
GAME( 1996, mj4simai,  0,        mj4simai, mj4simai, 0,        ROT0, "Maboroshi Ware",        "Wakakusamonogatari Mahjong Yonshimai (Japan)", GAME_NO_COCKTAIL )
GAME( 1996, myangel,   0,        myangel,  myangel,  0,        ROT0, "Namco",                 "Kosodate Quiz My Angel (Japan)",               GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 1997, myangel2,  0,        myangel2, myangel2, 0,        ROT0, "Namco",                 "Kosodate Quiz My Angel 2 (Japan)",             GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 1999, pzlbowl,   0,        pzlbowl,  pzlbowl,  0,	       ROT0, "Nihon System / Moss",   "Puzzle De Bowling (Japan)",                    GAME_NO_COCKTAIL )
GAME( 2000, penbros,   0,        penbros,  penbros,  0,        ROT0, "Subsino",               "Penguin Brothers (Japan)",                     GAME_NO_COCKTAIL )
GAME( 2000, deerhunt,  0,        samshoot, deerhunt, 0,        ROT0, "Sammy USA Corporation", "Deer Hunting USA V4.3",                        GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 2000, deerhunta, deerhunt, samshoot, deerhunt, 0,        ROT0, "Sammy USA Corporation", "Deer Hunting USA V4.2",                        GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 2000, deerhuntb, deerhunt, samshoot, deerhunt, 0,        ROT0, "Sammy USA Corporation", "Deer Hunting USA V4.0",                        GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 2000, deerhuntc, deerhunt, samshoot, deerhunt, 0,        ROT0, "Sammy USA Corporation", "Deer Hunting USA V2",                          GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 2001, turkhunt,  0,        samshoot, turkhunt, 0,        ROT0, "Sammy USA Corporation", "Turkey Hunting USA V1.0",                      GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 2001, wschamp,   0,        samshoot, wschamp,  0,        ROT0, "Sammy USA Corporation", "Wing Shooting Championship V2.00",             GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 2001, wschampa,  wschamp,  samshoot, wschamp,  0,        ROT0, "Sammy USA Corporation", "Wing Shooting Championship V1.01",             GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAME( 2002, trophyh,   0,        samshoot, trophyh,  0,        ROT0, "Sammy USA Corporation", "Trophy Hunting - Bear & Moose V1.0",           GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )

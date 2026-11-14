// license:BSD-3-Clause
// copyright-holders:David Haywood, Andreas Naive, Tomasz Slanina, ElSemi
/*

CPS3 Driver (preliminary)

Decryption by Andreas Naive

Driver by David Haywood
 with help from Tomasz Slanina and ElSemi

Sound emulation by Philip Bennett



Emulation Notes:

  sets are region hacked for now, to make the test modes readable, don't take the regions
  you see on bootup as being correct.

ToDo: (in order or priority?)

Convert to custom rendering.  There are just too many colours to use MAME's palettes, and the ram based tiles
make tilemap management more complex than it needs to be.  Currently we're having to overwrite palettes to
display the text layer.

SFIII2 seems to crash sometimes, it's probably reading past the end of memory somewhere

Fix sprite attributes, palette select (seems to be per sprite and per cell), y flip, zoom

Fix backgrounds - palettes, zooming, linescroll, see 'write custom rendering'

Emulate CD-Controller (not too important right now, games can be run as no-cd)  Note, due to the
flash roms region being split over 4 interleaved regions this will require a bit of code to copy
anything written to the flash roms into an area we can map opbase to.

update to use Nicola's newer decryption function

---

Capcom CP SYSTEM III Hardware Overview
Capcom, 1996-1999

From late 1996 to 1999 Capcom developed another hardware platform to rival the CPS2 System and called
it CP SYSTEM III. Only 6 games were produced....

Game                                                 Date   S/W Rev        CD Part#     Label         Cart Part#
----------------------------------------------------------------------------------------------------------------
JoJo no Kimyouna Bouken / JoJo's Venture             1998   98/12/02       CAP-JJK000   CAP-JJK-140   JJK98c00F
                                                            99/01/08                    CAP-JJK-160
                                                            99/01/28 **
JoJo no Kimyouna Bouken  Miraie no Isan
/ JoJo's Bizarre Adventure Heritage for the Future   1999   99/09/13       CAP-JJM000   CAP-JJM-110   JJM99900F
                                                            99/09/27 **

Street Fighter III New Generation                    1997   97/02/04       CAP-SF3000   CAP-SF3-3     SF397200F

Street Fighter III 2nd Impact Giant Attack           1997   97/09/30       CAP-3GA000   CAP-3GA-1     3GA97a00F

Street Fighter III 3rd Strike Fight for the Future   1999   99/05/12       CAP-33S000   CAP-33S-1     33S99400F
                                                            99/06/08                    CAP-33S-2

Warzard / Red Earth                                  1996   96/11/21       CAP-WZD000   CAP-WZD-5     WZD96a00F

** NOT DUMPED but known to exist

The Game Region / No CD Flags / Development flags etc. are controlled by a byte in the bios roms.  The CDs
contain revisions of the game code and are independant of the region.

The CP SYSTEM III comprises a main board with several custom ASICs, custom 72-pin SIMMs for program
and graphics storage (the same SIMMs are also used in some CPS2 titles), SCSI CDROM and CDROM disc,
and a plug-in security cart containing a boot ROM, an NVRAM and another custom ASIC containing vital
decryption information held by a [suicide] battery.

Not much is known about the actual CPU used in this system due to the extensive use of encryption,
and the volatile nature of the security information. There appears to be a custom Hitachi SH-2
CPU on the mainboard and there has been confirmed to be one in the cart. Tests were done by decrypting
the BIOS and code and running it on the PCB. It is known that neither of these CPU's will run standard
(i.e. unencrypted) SH2 code.

The security cart is thought to work like this....  the flashROM in the cart contains a program BIOS which is
decrypted by the CPU in the cart (the CPU has built-in decryption) then executed by that CPU to boot the BIOS
code.  Even though the code in the flashROM is encrypted, the cart can run it even if it is dead/suicided because
it has been discovered that the BIOS contains a hidden security menu allowing the cart to be loaded with the
security data. This proves the cart runs the BIOS even if it is dead. The special security menu is not
normally available but is likely accessed with a special key/button combination which is unknown ATM.
The cart contains a FM1208S NVRAM which appears to either be unused or holds game settings. Because the CPU
in the cart is always powered by a battery, it has stealth capability that allows it to continually monitor
the situation. If the custom CPU detects any tampering (generally things such as voltage fluctuation or
voltage dropping or even removal of the cart with the power on), it immediately erases the SRAM inside the
CPU (and thus the key) which effectively kills the security cart dead. This also suggests that the custom
Capcom CPU contains some additional internal code to initiate the boot process which is battery-backed
as well. It is known (from decapping it) that the CPU in the security cart does contain an amount of static
RAM for data storage and a SH2 core.

The main board uses the familiar Capcom SIMM modules to hold the data from the CDROM so that the life of
the CD drive is maximized. The SIMMs don't contain RAM, but instead TSOP48 surface mounted flashROMs
that can be updated with different games on bootup using a built-in software updating system.
The SIMMs that hold the program code are located in positions 1 & 2 and are 64MBit.
The SIMMs that hold the graphics are located in positions 3, 4, 5, 6 & 7 and are 128MBit.
The data in the SIMMs is not decrypted, it is merely taken directly from the CDROM and shuffled slightly
then programmed to the flashROMs. The SIMMs hold the entire contents of the CDROM.

To swap games requires the security cart for the game, it's CDROM disc and the correctly populated type
and number of SIMMs on the main board.
On first power-up after switching the cart and CD, you're presented with a screen asking if you want to
re-program the SIMMs with the new game. Pressing player 1 button 2 cancels it. Pressing player 1 button 1
allows it to proceed whereby you wait about 25 minutes then the game boots up almost immediately. On
subsequent power-ups, the game boots immediately.
If the CDROM is not present in the drive on a normal bootup, a message tells you to insert the CDROM.
Then you press button 1 to continue and the game boots immediately.
Note that not all of the SIMMs are populated on the PCB for each game. Some games have more, some less,
depending on game requirements, so flash times can vary per game. See the table below for details.

                                                     |----------- Required SIMM Locations & Types -----------|
Game                                                 1       2       3        4        5         6         7
--------------------------------------------------------------------------------------------------------------
JoJo's Venture                                       64MBit  64MBit  128MBit  128MBit  32MBit    -         -
JoJo's Bizarre Adventure                             64MBit  64MBit  128MBit  128MBit  128MBit   -         -
Street Fighter III New Generation                    64MBit  -       128MBit  128MBit  32MBit*   -         -
Street Fighter III 2nd Impact Giant Attack           64MBit  64MBit  128MBit  128MBit  128MBit   -         -
Street Fighter III 3rd Strike Fight for the Future   64MBit  64MBit  128MBit  128MBit  128MBit   128MBit   -
Warzard / Red Earth                                  64MBit  -       128MBit  128MBit  32MBit*   -         -

                                                     Notes:
                                                           - denotes not populated
                                                           * 32MBit SIMMs have only 2 FlashROMs populated on them.
                                                             128MBit SIMMs can also be used.
                                                           No game uses a SIMM at 7
                                                           See main board diagram below for SIMM locations.

Due to the built-in upgradability of the hardware, and the higher frame-rates the hardware seems to have,
it appears Capcom had big plans for this system and possibly intended to create many games on it, as they
did with CPS2. Unfortunately for Capcom, CP SYSTEM III was an absolute flop in the arcades so those plans
were cancelled. Possible reasons include...
- The games were essentially just 2D, and already there were many 3D games coming out onto the market that
  interested operators more than this.
- The cost of the system was quite expensive when compared to other games on the market.
- It is rumoured that the system was difficult to program for developers.
- These PCBs were not popular with operators because the security carts are extremely static-sensitive and most
  of them failed due to the decryption information being zapped by simple handling of the PCBs or by touching
  the security cart edge connector underneath the PCB while the security cart was plugged in, or by power
  fluctuations while flashing the SIMMs. You will know if your cart has been zapped because on bootup, you get
  a screen full of garbage coloured pixels instead of the game booting up, or just a black or single-coloured
  screen. You should also not touch the inside of the security cart because it will be immediately zapped
  when you touch it! The PCB can detect the presence of the security cart and if it is removed on a working game,
  the game will freeze immediately and it will also erase the security cart battery-backed data.


PCB Layouts
-----------

CAPCOM
CP SYSTEMIII
95682A-4 (older rev 95682A-3)
   |----------------------------------------------------------------------|
  |= J1             HM514260     |------------|      |  |  |  |  |        |
   |                             |CAPCOM      |      |  |  |  |  |        |
  |= J2     TA8201  TC5118160    |DL-2729 PPU |      |  |  |  |  |        |
   |                             |(QFP304)    |      |  |  |  |  |        |
|--|          VOL   TC5118160    |            |      |  |  |  |  |        |
|    LM833N                      |            |      S  S  S  S  S        |
|    LM833N         TC5118160    |------------|      I  I  I  I  I        |
|           TDA1306T                      |--------| M  M  M  M  M        |
|                   TC5118160  60MHz      |CAPCOM  | M  M  M  M  M       |-|
|                              42.9545MHz |DL-3329 | 7  6  5  4  3       | |
|           LM385                         |SSU     | |  |  |  |  |       | |
|J                         KM681002       |--------| |  |  |  |  |       | |
|A                         KM681002  62256 |-------| |  |  |  |  |       | |
|M                                         |DL3529 | |  |  |  |  |       | |
|M          MC44200FU                      |GLL2   | |  |  |  |  |       | |
|A                              3.6864MHz  |-------|                  CN6| |
|                                                             |  |       | |
|                               |--------|   |-|              |  |       | |
|                               |CAPCOM  |   | |   |-------|  |  |       | |
|        TD62064                |DL-2929 |   | |   |CAPCOM |  |  |       | |
|                               |IOU     |   | |   |DL-3429|  |  |       | |
|        TD62064                |--------|   | |   |GLL1   |  S  S       | |
|--|                            *HA16103FPJ  | |   |-------|  I  I       |-|
   |                                         | |CN5           M  M        |
   |                                         | |   |-------|  M  M        |
  |-|                        93C46           | |   |CAPCOM |  2  1        |
  | |      PS2501                            | |   |DL-2829|  |  | |-----||
  | |CN1                                     | |   |CCU    |  |  | |AMD  ||
  | |      PS2501                            | |   |-------|  |  | |33C93||
  |-|                                        |-|              |  | |-----||
   |   SW1                                         HM514260   |  |        |
   |----------------------------------------------------------------------|
Notes:
      TA8201     - Toshiba TA8201 18W BTL x 2-Channel Audio Power Amplifier
      PS2501     - NEC PS2501 High Isolation Voltage Single Transistor Type Multi Photocoupler (DIP16)
      TDA1306T   - Philips TDA1306T Noise Shaping Filter DAC (SOIC24). The clock (on pin 12) measures
                   14.3181667MHz (42.9545/3)
      MC44200FU  - Motorola MC44200FU Triple 8-bit Video DAC (QFP44)
      LM833N     - ST Microelectronics LM833N Low Noise Audio Dual Op-Amp (DIP8)
      TD62064    - Toshiba TD62064AP NPN 50V 1.5A Quad Darlington Driver (DIP16)
      HA16103FPJ - Hitachi HA16103FPJ Watchdog Timer (SOIC20)
                   *Note this IC is not populated on the rev -4 board
      93C46      - National Semiconductor NM93C46A 128bytes x8 Serial EEPROM (SOIC8)
                   Note this IC is covered by a plastic housing on the PCB. The chip is just a normal
                   (unsecured) EEPROM so why it was covered is not known.
      LM385      - National Semiconductor LM385 Adjustable Micropower Voltage Reference Diode (SOIC8)
      33C93      - AMD 33C93A-16 SCSI Controller (PLCC44)
      KM681002   - Samsung Electronics KM681002 128k x8 SRAM (SOJ32)
      62256      - 8k x8 SRAM (SOJ28)
      HM514260   - Hitachi HM514260CJ7 1M x16 DRAM (SOJ42)
      TC5118160  - Toshiba TC5118160BJ-60 256k x16 DRAM (SOJ42)
      SW1        - Push-button Test Switch
      VOL        - Master Volume Potentiometer
      J1/J2      - Optional RCA Left/Right Audio Out Connectors
      CN1        - 34-Pin Capcom Kick Button Harness Connector
      CN5        - Security Cartridge Slot
      CN6        - 4-Pin Power Connector and 50-pin SCSI Data Cable Connector
                   CDROM Drive is a CR504-KCM 4X SCSI drive manufactured By Panasonic / Matsushita
      SIMM 1-2   - 72-Pin SIMM Connector, holds single sided SIMMs containing 4x Fujitsu 29F016A
                   surface mounted TSOP48 FlashROMs
      SIMM 3-7   - 72-Pin SIMM Connector, holds double sided SIMMs containing 8x Fujitsu 29F016A
                   surface mounted TSOP48 FlashROMs

                   SIMM Layout -
                          |----------------------------------------------------|
                          |                                                    |
                          |   |-------|   |-------|   |-------|   |-------|    |
                          |   |Flash_A|   |Flash_B|   |Flash_C|   |Flash_D|    |
                          |   |-------|   |-------|   |-------|   |-------|    |
                          |-                                                   |
                           |-------------------------/\------------------------|
                           Notes:
                                  For SIMMs 1-2, Flash_A & Flash_C and regular pinout (Fujitsu 29F016A-90PFTN)
                                  Flash_B & Flash_D are reverse pinout (Fujitsu 29F016A-90PFTR)
                                  and are mounted upside down also so that pin1 lines up with
                                  the normal pinout of FlashROMs A & C.
                                  For SIMMs 3-7, the 8 FlashROMs are populated on both sides using a similar layout.

      Capcom Custom ASICs -
                           DL-2729 PPU SD10-505   (QFP304). Decapping reveals this is the main graphics chip.
                           DL-2829 CCU SD07-1514  (QFP208). Decapping reveals this to be a custom Toshiba ASIC.
                           DL-2929 IOU SD08-1513  (QFP208). This is the I/O controller.
                           DL-3329 SSU SD04-1536  (QFP144). This is might be the main CPU. It appears to be a SH2
                                                            variant with built-in encryption. It is clocked at
                                                            21.47725MHz (42.9545/2)
                           DL-3429 GLL1 SD06-1537 (QFP144). Unknown, possibly a DMA or bus controller.
                           DL-3529 GLL2 SD11-1755 (QFP80).  This might be the sound chip (it has 32k SRAM connected to it).


Connector Pinouts
-----------------

                       JAMMA Connector                                       Extra Button Connector
                       ---------------                                       ----------------------
                    PART SIDE    SOLDER SIDE                                       TOP    BOTTOM
                ----------------------------                               --------------------------
                      GND  01    A  GND                                        GND  01    02  GND
                      GND  02    B  GND                                        +5V  03    04  +5V
                      +5V  03    C  +5V                                       +12V  05    06  +12V
                      +5V  04    D  +5V                                             07    08
                       NC  05    E  NC                           Player 2 Button 4  09    10
                     +12V  06    F  +12V                                            11    12
                           07    H                                                  13    14
           Coin Counter 1  08    J  NC                           Player 1 Button 4  15    16
             Coin Lockout  09    K  Coin Lockout                 Player 1 Button 5  17    18
               Speaker (+) 10    L  Speaker (-)                  Player 1 Button 6  19    20
                       NC  11    M  NC                           Player 2 Button 5  21    22
                Video Red  12    N  Video Green                  Player 2 Button 6  23    24
               Video Blue  13    P  Video Composite Sync                            25    26
             Video Ground  14    R  Service Switch                                  27    28
                     Test  15    S  NC                                 Volume Down  29    30  Volume UP
                   Coin A  16    T  Coin B                                     GND  31    32  GND
           Player 1 Start  17    U  Player 2 Start                             GND  33    34  GND
              Player 1 Up  18    V  Player 2 Up
            Player 1 Down  19    W  Player 2 Down
            Player 1 Left  20    X  Player 2 Left
           Player 1 Right  21    Y  Player 2 Right
        Player 1 Button 1  22    Z  Player 2 Button 1
        Player 1 Button 2  23    a  Player 2 Button 2
        Player 1 Button 3  24    b  Player 2 Button 3
                       NC  25    c  NC
                       NC  26    d  NC
                      GND  27    e  GND
                      GND  28    f  GND


Security Cartridge PCB Layout
-----------------------------

CAPCOM 95682B-3 TORNADE
|------------------------------------------------|
|      BATTERY                                   |
|                          |-------|             |
|                          |CAPCOM |   29F400    |
|                          |DL-3229|   *28F400   |
|                          |SCU    |     *FM1208S|
| 74HC00                   |-------|             |
|               6.25MHz                    74F00 |
|---|     |-|                             |------|
    |     | |                             |
    |-----| |-----------------------------|
Notes:
      74F00        - 74F00 Quad 2-Input NAND Gate (SOIC14)
      74HC00       - Philips 74HC00N Quad 2-Input NAND Gate (DIP14)
      29F400       - Fujitsu 29F400TA-90PFTN 512k x8 FlashROM (TSOP48)
      Custom ASIC  - CAPCOM DL-3229 SCU (QFP144). Decapping reveals this is a Hitachi HD6417099 SH2 variant
                     with built-in encryption, clocked at 6.250MHz
      FM1208S      - RAMTRON FM1208S 4k (512bytes x8) Nonvolatile Ferroelectric RAM (SOIC24)
      28F400       - 28F400 SOP44 FlashROM (not populated)
      *            - These components located on the other side of the PCB
      The battery powers the CPU only. A small board containing some transistors is wired to the 74HC00
      to switch the CPU from battery power to main power to increase the life of the battery.

*/

#include "driver.h"
#include "machine/intelfsh.h"
#include "cpu/sh2/sh2.h"
#include "streams.h"
#include "sound/custom.h"
#include "cps3.h"

#define DEBUG_PRINTF 0
#define CPS3_VOICES 16

#define CPS3_TRANSPARENCY_NONE 0
#define CPS3_TRANSPARENCY_PEN 1
#define CPS3_TRANSPARENCY_PEN_INDEX 2
#define CPS3_TRANSPARENCY_PEN_INDEX_BLEND 3

#ifdef LSB_FIRST
	#define DMA_XOR(a)	((a) ^ 1)
#else
	#define DMA_XOR(a)	((a) ^ 2)
#endif

UINT32* decrypted_bios;
UINT32* decrypted_gamerom;
UINT32 cram_gfxflash_bank;
UINT32* cps3_nops;

UINT32*tilemap20_regs_base;
UINT32*tilemap30_regs_base;
UINT32*tilemap40_regs_base;
UINT32*tilemap50_regs_base;

UINT32* cps3_ram;
UINT32* cps3_ram_decrypted;

UINT32* cps3_char_ram;

UINT32* cps3_spriteram;
UINT32* cps3_eeprom;
UINT32* cps3_fullscreenzoom;

UINT32 cps3_ss_pal_base = 0;
UINT32* cps3_colourram;
UINT32 cps3_unk_vidregs[0x20/4];
UINT32 cps3_ss_bank_base = 0;

UINT32* cps3_mame_colours;//[0x20000]; // actual values to write to 32-bit bitmap

static mame_bitmap *renderbuffer_bitmap;
static rectangle renderbuffer_clip;

static offs_t cps3_opbase_handler(offs_t address);
static sound_stream *cps3_stream;

typedef struct _cps3_voice_
{
	UINT32 regs[8];
	UINT32 pos;
	UINT32 frac;
} cps3_voice;

#define CPS3_VOL_SHIFT	8	/* add_int(..., 32768<<8) on float → INT32 stream is >> 8 */

struct
{
	cps3_voice voice[CPS3_VOICES];
	UINT16     key;
} chip;

#ifndef LSB_FIRST
INLINE UINT32 LE32(UINT32 addr)
{
	UINT32 res = (((addr&0xff000000)>>24) |
		 ((addr&0x00ff0000)>>8) |
		 ((addr&0x0000ff00)<<8) |
		 ((addr&0x000000ff)<<24));

	return res;
}
#else
#define LE32(a) a
#endif

INLINE void cps3_drawgfxzoom( mame_bitmap *dest_bmp,const gfx_element *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color,
		int scalex, int scaley,mame_bitmap *pri_buffer,UINT32 pri_mask)
{
	rectangle myclip;

//  UINT8 al;

//  al = (pdrawgfx_shadow_lowpri) ? 0 : 0x80;

	if (!scalex || !scaley) return;

// todo: reimplement this optimization!!
//  if (scalex == 0x10000 && scaley == 0x10000)
//  {
//      common_drawgfx(dest_bmp,gfx,code,color,flipx,flipy,sx,sy,clip,transparency,transparent_color,pri_buffer,pri_mask);
//      return;
//  }

	/*
    scalex and scaley are 16.16 fixed point numbers
    1<<15 : shrink to 50%
    1<<16 : uniform scale
    1<<17 : double to 200%
    */


	/* force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}


	/* 32-bit ONLY */
	{
		if( gfx )
		{
//          const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
			UINT32 palbase = (gfx->color_granularity * color) & 0x1ffff;
			const pen_t *pal = &cps3_mame_colours[palbase];
			UINT8 *source_base = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;

			int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
			int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;

			if (sprite_screen_width && sprite_screen_height)
			{
				/* compute sprite increment per screen pixel */
				int dx = (gfx->width<<16)/sprite_screen_width;
				int dy = (gfx->height<<16)/sprite_screen_height;

				int ex = sx+sprite_screen_width;
				int ey = sy+sprite_screen_height;

				int x_index_base;
				int y_index;

				if( flipx )
				{
					x_index_base = (sprite_screen_width-1)*dx;
					dx = -dx;
				}
				else
				{
					x_index_base = 0;
				}

				if( flipy )
				{
					y_index = (sprite_screen_height-1)*dy;
					dy = -dy;
				}
				else
				{
					y_index = 0;
				}

				if( clip )
				{
					if( sx < clip->min_x)
					{ /* clip left */
						int pixels = clip->min_x-sx;
						sx += pixels;
						x_index_base += pixels*dx;
					}
					if( sy < clip->min_y )
					{ /* clip top */
						int pixels = clip->min_y-sy;
						sy += pixels;
						y_index += pixels*dy;
					}
					if( ex > clip->max_x+1 )
					{ /* clip right */
						int pixels = ex-clip->max_x-1;
						ex -= pixels;
					}
					if( ey > clip->max_y+1 )
					{ /* clip bottom */
						int pixels = ey-clip->max_y-1;
						ey -= pixels;
					}
				}

				if( ex>sx )
				{ /* skip if inner loop doesn't draw anything */
					int y;

					/* case 0: TRANSPARENCY_NONE */
					if (transparency == CPS3_TRANSPARENCY_NONE)
					{
						{
							for( y=sy; y<ey; y++ )
							{
								UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
								UINT32 *dest = (UINT32 *)dest_bmp->line[y];

								int x, x_index = x_index_base;
								for( x=sx; x<ex; x++ )
								{
									dest[x] = pal[source[x_index>>16]];
									x_index += dx;
								}

								y_index += dy;
							}
						}
					}
					else if (transparency == CPS3_TRANSPARENCY_PEN)
					{
						{
							for( y=sy; y<ey; y++ )
							{
								UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
								UINT32 *dest = (UINT32 *)dest_bmp->line[y];

								int x, x_index = x_index_base;
								for( x=sx; x<ex; x++ )
								{
									int c = source[x_index>>16];
									if( c != transparent_color ) dest[x] = pal[c];
									x_index += dx;
								}

								y_index += dy;
							}
						}
					}
					else if (transparency == CPS3_TRANSPARENCY_PEN_INDEX)
					{
						{
							for( y=sy; y<ey; y++ )
							{
								UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
								UINT32 *dest = (UINT32 *)dest_bmp->line[y];

								int x, x_index = x_index_base;
								for( x=sx; x<ex; x++ )
								{
									int c = source[x_index>>16];
									if( c != transparent_color ) dest[x] = c | palbase;
									x_index += dx;
								}

								y_index += dy;
							}
						}
					}
					else if (transparency == CPS3_TRANSPARENCY_PEN_INDEX_BLEND)
					{
						{
							for( y=sy; y<ey; y++ )
							{
								UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
								UINT32 *dest = (UINT32 *)dest_bmp->line[y];

								int x, x_index = x_index_base;
								for( x=sx; x<ex; x++ )
								{
									int c = source[x_index>>16];
									if( c != transparent_color )
									{
										/* blending isn't 100% understood */
										if (gfx->color_granularity == 64)
										{
											// OK for sfiii2 spotlight
											if (c&0x01) dest[x] |= 0x2000;
											if (c&0x02) dest[x] |= 0x4000;
											if (c&0x04) dest[x] |= 0x8000;
											if (c&0x08) dest[x] |= 0x10000;
											if (c&0xf0) dest[x] |= mame_rand(); // ?? not used?
										}
										else
										{
											// OK for jojo intro, and warzard swords, and various shadows in sf games
											if (c&0x01) dest[x] |= 0x8000;
											if (color&0x100) dest[x]|=0x10000;
										}
									}


									x_index += dx;
								}

								y_index += dy;
							}
						}
					}
				}
			}
		}
	}
}

/* Encryption */

static UINT32 cps3_key1, cps3_key2, cps3_isSpecial;

static UINT16 rotate_left(UINT16 value, int n)
{
   int aux = value>>(16-n);
   return ((value<<n)|aux)%0x10000;
}

static UINT16 rotxor(UINT16 val, UINT16 xor)
{
	UINT16 res;

	res = val + rotate_left(val,2);

	res = rotate_left(res,4) ^ (res & (val ^ xor));

	return res;
}

static UINT32 cps3_mask(UINT32 address, UINT32 key1, UINT32 key2)
{
	UINT16 val;

	address ^= key1;

	val = (address & 0xffff) ^ 0xffff;

	val = rotxor(val, key2 & 0xffff);

	val ^= (address >> 16) ^ 0xffff;

	val = rotxor(val, key2 >> 16);

	val ^= (address & 0xffff) ^ (key2 & 0xffff);

	return val | (val << 16);
}

struct game_keys2
{
	const char *name;             /* game driver name */
	const UINT32 keys[2];
	int isSpecial;
};

static const struct game_keys2 keys_table2[] =
{
	// name         key1       key2
	{ "jojo",     { 0x02203ee3, 0x01301972 },0 },
	{ "jojojr2",     { 0x02203ee3, 0x01301972 },0 },
	{ "jojojr1",  { 0x02203ee3, 0x01301972 },0 },
	{ "jojoba",   { 0x23323ee3, 0x03021972 },0 },
	{ "jojobanr2",   { 0x23323ee3, 0x03021972 },0 },
	{ "jojobajr2",  { 0x23323ee3, 0x03021972 },0 },
	{ "sfiii",    { 0xb5fe053e, 0xfc03925a },0 },
	{ "sfiiij",    { 0xb5fe053e, 0xfc03925a },0 },
	{ "sfiii2",   { 0x00000000, 0x00000000 },1 },
	{ "sfiii2j",   { 0x00000000, 0x00000000 },1 },
	{ "sfiii3",   { 0xa55432b4, 0x0c129981 },0 },
	{ "sfiii3ur1",   { 0xa55432b4, 0x0c129981 },0 },
	{ "sfiii3u",  { 0xa55432b4, 0x0c129981 },0 },
	{ "sfiii4n",  { 0xa55432b4, 0x0c129981 },0 },
	{ "redearth",  { 0x9e300ab1, 0xa175b82c },0 },
	{ "warzard",  { 0x9e300ab1, 0xa175b82c },0 },
	{ 0 }	// end of table
};

static void cps3_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	int i;
	INT8 *base = (INT8*)memory_region(REGION_USER5);

	/* Clear the buffers */
	memset(buffer[0], 0, length*sizeof(*buffer[0]));
	memset(buffer[1], 0, length*sizeof(*buffer[1]));

	for (i = 0; i < CPS3_VOICES; i ++)
	{
		if (chip.key & (1 << i))
		{
			int j;
			cps3_voice *vptr = &chip.voice[i];
			UINT32 start = (vptr->regs[1] >> 16 & 0x0000ffff) | (vptr->regs[1] << 16 & 0xffff0000);
			UINT32 end   = (vptr->regs[5] >> 16 & 0x0000ffff) | (vptr->regs[5] << 16 & 0xffff0000);
			UINT32 loop  = (vptr->regs[3] & 0x0000ffff) | (vptr->regs[4] << 16 & 0xffff0000);
			int loop_enable = (vptr->regs[2] & 1);
			UINT32 step  = vptr->regs[3] >> 16 & 0xffff;
			INT16 vol_l = (INT16)(vptr->regs[7] & 0xffff);
			INT16 vol_r = (INT16)(vptr->regs[7] >> 16 & 0xffff);
			UINT32 pos = vptr->pos;
			UINT32 frac = vptr->frac;

			start -= 0x400000;
			end -= 0x400000;
			loop -= 0x400000;

			for (j = 0; j < length; j ++)
			{
				INT32 sample;

				pos += (frac >> 12);
				frac &= 0xfff;

				if ((start + pos) >= end)
				{
					if (loop_enable)
						pos = (pos + loop) - end;
					else
						break;
				}

				sample = base[BYTE4_XOR_LE(start + pos)];
				frac += step;

				buffer[0][j] += ((INT32)sample * (INT32)vol_l) >> CPS3_VOL_SHIFT;
				buffer[1][j] += ((INT32)sample * (INT32)vol_r) >> CPS3_VOL_SHIFT;
			}

			vptr->pos = pos;
			vptr->frac = frac;
		}
	}

}

void *cps3_sh_start(int clock, const struct CustomSound_interface *config)
{
	/* Allocate the stream */
	cps3_stream = stream_create(0, 2, clock / 384, NULL, cps3_stream_update);

	memset(&chip, 0, sizeof(chip));

	return auto_malloc(1);
}

WRITE32_HANDLER( cps3_sound_w )
{
	stream_update(cps3_stream,0);

	if (offset < 0x80)
	{
		COMBINE_DATA(&chip.voice[offset / 8].regs[offset & 7]);
	}
	else if (offset == 0x80)
	{
		int i;
		UINT16 key = data >> 16;

		for (i = 0; i < CPS3_VOICES; i++)
		{
			// Key off -> Key on
			if ((key & (1 << i)) && !(chip.key & (1 << i)))
			{
				chip.voice[i].frac = 0;
				chip.voice[i].pos = 0;
			}
		}
		chip.key = key;
	}
	else
	{
		//printf("Sound [%x] %x\n", offset, data);
	}
}

READ32_HANDLER( cps3_sound_r )
{
	stream_update(cps3_stream,0);

	if (offset < 0x80)
	{
		return chip.voice[offset / 8].regs[offset & 7] & ~mem_mask;
	}
	else if (offset == 0x80)
	{
		return chip.key << 16;
	}
	else
	{
		//printf("Unk sound read : %x\n", offset);
		return 0;
	}
}

void cps3_decrypt_bios(void)
{
	int i;
	UINT32 *coderegion = (UINT32*)memory_region(REGION_USER1);

	decrypted_bios = (UINT32*)memory_region(REGION_USER1);

	for (i=0;i<0x80000;i+=4)
	{
		UINT32 dword = coderegion[i/4];
		UINT32 xormask = cps3_mask(i, cps3_key1, cps3_key2);

		/* a bit of a hack, don't decrypt the FLASH commands which are transfered by SH2 DMA */
		if (((i<0x1ff00) || (i>0x1ff6b)) && (i<0x20000) )
		{
			decrypted_bios[i/4] = dword ^ xormask;
		}
		else
		{
			decrypted_bios[i/4] = dword;
		}
	}
}

void cps3_decrypt_gamerom(void)
{
	int i;
	UINT32 *coderegion = (UINT32*)memory_region(REGION_USER4);

	decrypted_gamerom = auto_malloc(0x1000000);

	for (i=0;i<0x1000000;i+=4)
	{
		UINT32 dword = coderegion[i/4];
		UINT32 xormask = cps3_mask(i+0x6000000, cps3_key1, cps3_key2);
		decrypted_gamerom[i/4] = dword ^ xormask;
	}
}


DRIVER_INIT( cps3crpt )
{
	const char *gamename = Machine->gamedrv->name;
	const struct game_keys2 *k = &keys_table2[0];
	int i;

	cps3_key1 = -1;
	cps3_key2 = -1;
	cps3_isSpecial = -1;

	while (k->name)
	{
		if (strcmp(k->name, gamename) == 0)
		{
			// we have a proper key set the global variables to it (so that we can decrypt code in ram etc.)
			cps3_key1 = k->keys[0];
			cps3_key2 = k->keys[1];
			cps3_isSpecial = k->isSpecial;
			break;
		}
		++k;
	}

	cps3_decrypt_bios();
	cps3_decrypt_gamerom();

	/* just some NOPs for the game to execute if it crashes and starts executing unmapped addresses
     - this prevents MAME from crashing */
	cps3_nops = auto_malloc(0x4);
	cps3_nops[0] = 0x00090009;

	cps3_ram_decrypted = auto_malloc(0x400);
	memory_set_opbase_handler(0, cps3_opbase_handler);

	// flash roms

	for (i=0;i<48;i++)
		intelflash_init( i, FLASH_FUJITSU_29F016A, NULL );

	cps3_eeprom = auto_malloc(0x400);



}


static const gfx_layout cps3_tiles16x16_layout =
{
	16,16,
	0x8000,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 3*8,2*8,1*8,0*8,7*8,6*8,5*8,4*8,
	  11*8,10*8,9*8,8*8,15*8,14*8,13*8,12*8 },
	{ 0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128},
	8*256
};



static const gfx_layout cps3_tiles8x8_layout =
{
	8,8,
	0x400,
	4,
	{ /*8,9,10,11,*/ 0,1,2,3 },
	{ 20,16,4,0,52,48,36,32 },

	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

UINT32* cps3_ss_ram;
UINT8* cps3_ss_ram_dirty;
int cps3_ss_ram_is_dirty;

UINT8* cps3_char_ram_dirty;
int cps3_char_ram_is_dirty;

void cps3_set_mame_colours( int colournum, UINT16 data, UINT32 fadeval )
{
    int r,g,b;
    UINT16* dst = (UINT16*)cps3_colourram;

    r = (data >> 0) & 0x1f;
    g = (data >> 5) & 0x1f;
    b = (data >> 10) & 0x1f;

    /* is this 100% correct? */
    if (fadeval & 0x40400040)
    {
        int fade;
        //printf("fadeval %08x\n", fadeval);

        /* Apply fade to Red channel */
        fade = (fadeval & 0x7f000000) >> 24;
        if (fade & 0x40)
        {
            if (fade & 0x20)
                r = (((0x1f - r) * (0x1f - (fade & 0x1f))) >> 5) ^ 0x1f;
            else
                r = (r * (fade & 0x1f)) >> 5;
        }

        /* Apply fade to Green channel */
        fade = (fadeval & 0x007f0000) >> 16;
        if (fade & 0x40)
        {
            if (fade & 0x20)
                g = (((0x1f - g) * (0x1f - (fade & 0x1f))) >> 5) ^ 0x1f;
            else
                g = (g * (fade & 0x1f)) >> 5;
        }

        /* Apply fade to Blue channel */
        fade = (fadeval & 0x0000007f) >> 0;
        if (fade & 0x40)
        {
            if (fade & 0x20)
                b = (((0x1f - b) * (0x1f - (fade & 0x1f))) >> 5) ^ 0x1f;
            else
                b = (b * (fade & 0x1f)) >> 5;
        }

        /* Reconstruct 16-bit color, preserving the top bit (if any) */
        data = (data & 0x8000) | (r << 0) | (g << 5) | (b << 10);
    }

    /* Write color to palette RAM, within valid range */
    colournum &= 0x1ffff;
    dst[colournum] = data;
    cps3_mame_colours[colournum] = (r << (16+3)) | (g << (8+3)) | (b << (0+3));
    if (colournum < 0x10000)
        palette_set_color(colournum, r << 3, g << 3, b << 3);
}


void decode_ssram(void)
{
	if (cps3_ss_ram_dirty)
	{
		int i;
		for (i=0;i<0x400;i++)
		{
			if (cps3_ss_ram_dirty[i])
			{
				decodechar(Machine->gfx[0], i, (UINT8*)cps3_ss_ram, &cps3_tiles8x8_layout);
				cps3_ss_ram_dirty[i] = 0;
			}
		}

		cps3_ss_ram_is_dirty = 0;
	}

	return;
}

void decode_charram(void)
{
	if (cps3_char_ram_is_dirty)
	{
		int i;
		for (i=0;i<0x8000;i++)
		{
			if (cps3_char_ram_dirty[i])
			{
				decodechar(Machine->gfx[1], i, (UINT8*)cps3_char_ram, &cps3_tiles16x16_layout);
				cps3_char_ram_dirty[i] = 0;
			}
		}

		cps3_char_ram_is_dirty = 0;
	}

	return;
}

VIDEO_START(cps3)
{
	cps3_ss_ram       = auto_malloc(0x10000);
	cps3_ss_ram_dirty = auto_malloc(0x400);
	cps3_ss_ram_is_dirty = 1;
	memset(cps3_ss_ram, 0x00, 0x10000);
	memset(cps3_ss_ram_dirty, 1, 0x400);
	state_save_register_global_pointer(cps3_ss_ram, 0x10000/4);

	cps3_char_ram = auto_malloc(0x800000);
	cps3_char_ram_dirty = auto_malloc(0x800000/256);
	cps3_char_ram_is_dirty = 1;
	memset(cps3_char_ram, 0x00, 0x800000);
	memset(cps3_char_ram_dirty, 1, 0x8000);
	state_save_register_global_pointer(cps3_char_ram, 0x800000 /4);

	/* create the char set (gfx will then be updated dynamically from RAM) */
	Machine->gfx[0] = allocgfx(&cps3_tiles8x8_layout);
	Machine->gfx[0]->colortable   = Machine->pens;
	Machine->gfx[0]->total_colors = Machine->drv->total_colors / 16;

	//decode_ssram();

	/* create the char set (gfx will then be updated dynamically from RAM) */
	Machine->gfx[1] = allocgfx(&cps3_tiles16x16_layout);
	Machine->gfx[1]->colortable   = Machine->pens;
	Machine->gfx[1]->total_colors = Machine->drv->total_colors / 64;
	Machine->gfx[1]->color_granularity=64;

	//decode_charram();

	cps3_mame_colours = auto_malloc(0x80000);
	memset(cps3_mame_colours, 0x00, 0x80000);

	// the renderbuffer can be twice the size of the screen, this allows us to handle framebuffer zoom values
	// between 0x00 and 0x80 (0x40 is normal, 0x80 would be 'view twice as much', 0x20 is 'view half as much')
	renderbuffer_bitmap = auto_bitmap_alloc(Machine->drv->screen_width*2,Machine->drv->screen_height*2);

	renderbuffer_clip.min_x = 0;
	renderbuffer_clip.max_x = Machine->drv->screen_width*2-1;
	renderbuffer_clip.min_y = 0;
	renderbuffer_clip.max_y = Machine->drv->screen_height*2-1;

	fillbitmap(renderbuffer_bitmap,0x3f,&renderbuffer_clip);

return 0;
}

void cps3_draw_tilemapsprite_line(int tmnum, int drawline, mame_bitmap *bitmap, const rectangle *cliprect )
{
	UINT32* tmapregs[4] = { tilemap20_regs_base, tilemap30_regs_base, tilemap40_regs_base, tilemap50_regs_base };
	UINT32* regs;
	int line;
	int scrolly;
	if (tmnum>3)
	{
		printf("cps3_draw_tilemapsprite_line Illegal tilemap number %d\n",tmnum);
		return;
	}
	regs = tmapregs[tmnum];

	scrolly =  ((regs[0]&0x0000ffff)>>0)+4;
	line = drawline+scrolly;
	line&=0x3ff;


	if (!(regs[1]&0x00008000)) return;

	{
		UINT32 mapbase =  (regs[2]&0x007f0000)>>16;
		UINT32 linebase=  (regs[2]&0x7f000000)>>24;
		int linescroll_enable = (regs[1]&0x00004000);

		int scrollx;
		int x;
		int tileline = (line/16)+1;
		int tilesubline = line % 16;
		rectangle clip;

		mapbase = mapbase << 10;
		linebase = linebase << 10;

		if (!linescroll_enable)
		{
			scrollx =  (regs[0]&0xffff0000)>>16;
		}
		else
		{
		//	printf("linebae %08x\n", linebase);

			scrollx =  (regs[0]&0xffff0000)>>16;
			scrollx+= (cps3_spriteram[linebase+((line+16-4)&0x3ff)]>>16)&0x3ff;

		}

//  zoombase    =  (layerregs[1]&0xffff0000)>>16;

		drawline&=0x3ff;

		if (drawline>cliprect->max_y+4) return;

		clip.min_x = cliprect->min_x;
		clip.max_x = cliprect->max_x;
		clip.min_y = drawline;
		clip.max_y = drawline;
		
		for (x=0;x<(cliprect->max_x/16)+2;x++)
		{
			
			UINT32 dat;
			int tileno; 
			int colour;
			int bpp;
			int alpha;
			int xflip,yflip;

			dat = cps3_spriteram[mapbase+((tileline&63)*64)+((x+scrollx/16)&63)];
			tileno = (dat & 0xffff0000)>>17;
			colour = (dat & 0x000001ff)>>0;
			bpp = (dat & 0x0000200)>>9;
			alpha = (dat & 0x00000400)>>10;
			yflip  = (dat & 0x00000800)>>11;
			xflip  = (dat & 0x00001000)>>12;

			if (!bpp) Machine->gfx[1]->color_granularity=256;
			else Machine->gfx[1]->color_granularity=64;

			if (cps3_char_ram_dirty[tileno])
			{
				decodechar(Machine->gfx[1], tileno, (UINT8*)cps3_char_ram, &cps3_tiles16x16_layout);
				cps3_char_ram_dirty[tileno] = 0;
			}

			if (alpha)
			{
			cps3_drawgfxzoom(bitmap, Machine->gfx[1],tileno,colour,xflip,yflip,(x*16)-scrollx%16,drawline-tilesubline,&clip,CPS3_TRANSPARENCY_PEN_INDEX_BLEND,0, 0x10000, 0x10000, NULL, 0);
			}
			else
			{
			cps3_drawgfxzoom(bitmap, Machine->gfx[1],tileno,colour,xflip,yflip,(x*16)-scrollx%16,drawline-tilesubline,&clip,CPS3_TRANSPARENCY_PEN_INDEX,0, 0x10000, 0x10000, NULL, 0);
			}
		}
	}
}

VIDEO_UPDATE(cps3)
{
	int x, count;
//	int offset;

	int bg_drawn[4] = { 0, 0, 0, 0 };

	UINT32 fullscreenzoomx, fullscreenzoomy;
	UINT32 fszx, fszy;

	//decode_ssram();
	//decode_charram();

	fullscreenzoomx = cps3_fullscreenzoom[3] & 0x000000ff;
	fullscreenzoomy = cps3_fullscreenzoom[3] & 0x000000ff;
	/* clamp at 0x80, I don't know if this is accurate */
	if (fullscreenzoomx>0x80) fullscreenzoomx = 0x80;
	if (fullscreenzoomy>0x80) fullscreenzoomy = 0x80;

	fszx = (fullscreenzoomx<<16)/0x40;
	fszy = (fullscreenzoomy<<16)/0x40;

	renderbuffer_clip.min_x = 0;
	renderbuffer_clip.max_x = ((384*fszx)>>16)-1;
	renderbuffer_clip.min_y = 0;
	renderbuffer_clip.max_y = ((224*fszx)>>16)-1;

	fillbitmap(renderbuffer_bitmap,0,&renderbuffer_clip);

	/* Sprites */
	{
		int i;

		//printf("Spritelist start:\n");
		for (i=0x00000/4;i<0x2000/4;i+=4)
		{
			int xpos =  	(cps3_spriteram[i+1]&0x03ff0000)>>16;
			int ypos =  	cps3_spriteram[i+1]&0x000003ff;
			int j;
			int gscroll =      (cps3_spriteram[i+0]&0x70000000)>>28;
			int length =       (cps3_spriteram[i+0]&0x01ff0000)>>16; // how many entries in the sprite table
			UINT32 start  =    (cps3_spriteram[i+0]&0x00007ff0)>>4;

			int whichbpp =     (cps3_spriteram[i+2]&0x40000000)>>30; // not 100% sure if this is right, jojo title / characters
			int whichpal =     (cps3_spriteram[i+2]&0x20000000)>>29;
			int global_xflip = (cps3_spriteram[i+2]&0x10000000)>>28;
			int global_yflip = (cps3_spriteram[i+2]&0x08000000)>>27;
			int global_alpha = (cps3_spriteram[i+2]&0x04000000)>>26; // alpha / shadow? set on sfiii2 shadows, and big black image in jojo intro
			int global_bpp =   (cps3_spriteram[i+2]&0x02000000)>>25;
			int global_pal =   (cps3_spriteram[i+2]&0x01ff0000)>>16;

			int gscrollx = (cps3_unk_vidregs[gscroll]&0x03ff0000)>>16;
			int gscrolly = (cps3_unk_vidregs[gscroll]&0x000003ff)>>0;
			start = (start * 0x100) >> 2;

			if ((cps3_spriteram[i+0]&0xf0000000) == 0x80000000)
				break;

			for (j=0;j<(length)*4;j+=4)
			{

				UINT32 value1 =  	(cps3_spriteram[start+j+0]);
				UINT32 value2 =  	(cps3_spriteram[start+j+1]);
				UINT32 value3 =  	(cps3_spriteram[start+j+2]);


				//UINT8* srcdata = (UINT8*)cps3_char_ram;
				//UINT32 sourceoffset = (value1 >>14)&0x7fffff;
				int count;

				UINT32 tileno = (value1&0xfffe0000)>>17;

				int xpos2 = (value2 & 0x03ff0000)>>16;
				int ypos2 = (value2 & 0x000003ff)>>0;
				int flipx = (value1 & 0x00001000)>>12;
				int flipy = (value1 & 0x00000800)>>11;
				int alpha = (value1 & 0x00000400)>>10; //? this one is used for alpha effects on warzard
				int bpp =   (value1 & 0x00000200)>>9;
				int pal =  (value1  & 0x000001ff);


				/* these are the sizes to actually draw */
				int ysizedraw2 = ((value3 & 0x7f000000)>>24);
				int xsizedraw2 = ((value3 & 0x007f0000)>>16);
				int xx,yy;

				int tilestable[4] = { 8,1,2,4 };
				int ysize2 = ((value3 & 0x0000000c)>>2);
				int xsize2 = ((value3 & 0x00000003)>>0);
				UINT32 xinc,yinc;

				if (ysize2==0)
				{
				//	printf("invalid sprite ysize of 0 tiles\n");
					continue;
				}

				if (xsize2==0) // xsize of 0 tiles seems to be a special command to draw tilemaps
				{
					int tilemapnum = ((value3 & 0x00000030)>>4);
					int startline;// = value2 & 0x3ff;
					int endline;
					int height = (value3 & 0x7f000000)>>24;
					int uu;
					UINT32* tmapregs[4] = { tilemap20_regs_base, tilemap30_regs_base, tilemap40_regs_base, tilemap50_regs_base };
					UINT32* regs;
					regs = tmapregs[tilemapnum];
					endline = value2;
					startline = endline - height;

					startline &=0x3ff;
					endline &=0x3ff;

					//printf("tilemap draw %01x %02x %02x %02x\n",tilemapnum, value2, height, regs[0]&0x000003ff );

					//printf("tilemap draw %01x %d %d\n",tilemapnum, startline, endline );
	

					/* Urgh, the startline / endline seem to be direct screen co-ordinates regardless of fullscreen zoom
					   which probably means the fullscreen zoom is applied when rendering everything, not aftewards */
					//for (uu=startline;uu<endline+1;uu++)
					
					if (bg_drawn[tilemapnum]==0)
					{
						for (uu=0;uu<1023;uu++)
						{
							cps3_draw_tilemapsprite_line( tilemapnum, uu, renderbuffer_bitmap, &renderbuffer_clip );
						}
					}
					bg_drawn[tilemapnum] = 1;
				}
				else
				{
					ysize2 = tilestable[ysize2];
					xsize2 = tilestable[xsize2];

					xinc = ((xsizedraw2+1)<<16) / ((xsize2*0x10));
					yinc = ((ysizedraw2+1)<<16) / ((ysize2*0x10));

					xsize2-=1;
					ysize2-=1;

					flipx ^= global_xflip;
					flipy ^= global_yflip;

					if (!flipx) xpos2+=((xsizedraw2+1)/2);
					else xpos2-=((xsizedraw2+1)/2);

					ypos2+=((ysizedraw2+1)/2);

					if (!flipx) xpos2-= (((xsize2+1)*16*xinc)>>16);
					else  xpos2+= (((xsize2)*16*xinc)>>16);

					if (flipy) ypos2-= ((ysize2*16*yinc)>>16);

					{
						count = 0;
						for (xx=0;xx<xsize2+1;xx++)
						{
							int current_xpos;

							if (!flipx) current_xpos = (xpos+xpos2+((xx*16*xinc)>>16)  );
							else current_xpos = (xpos+xpos2-((xx*16*xinc)>>16));
							//current_xpos +=  rand()&0x3ff;
							current_xpos += gscrollx;
							current_xpos += 1;
							current_xpos &=0x3ff;
							if (current_xpos&0x200) current_xpos-=0x400;

							for (yy=0;yy<ysize2+1;yy++)
							{
								int current_ypos;
								int actualpal;

								if (flipy) current_ypos = (ypos+ypos2+((yy*16*yinc)>>16));
								else current_ypos = (ypos+ypos2-((yy*16*yinc)>>16));

								current_ypos += gscrolly;
								current_ypos = 0x3ff-current_ypos;
								current_ypos -= 17;
								current_ypos &=0x3ff;

								if (current_ypos&0x200) current_ypos-=0x400;

								//if ( (whichbpp) && (cpu_getcurrentframe() & 1)) continue;

								/* use the palette value from the main list or the sublists? */
								if (whichpal)
								{
									actualpal = global_pal;
								}
								else
								{
									actualpal = pal;
								}

								/* use the bpp value from the main list or the sublists? */
								if (whichbpp)
								{
									if (!global_bpp) Machine->gfx[1]->color_granularity=256;
									else Machine->gfx[1]->color_granularity=64;
								}
								else
								{
									if (!bpp) Machine->gfx[1]->color_granularity=256;
									else Machine->gfx[1]->color_granularity=64;
								}

								{
									int realtileno = tileno+count;

									if (cps3_char_ram_dirty[realtileno])
									{
										decodechar(Machine->gfx[1], realtileno, (UINT8*)cps3_char_ram, &cps3_tiles16x16_layout);
										cps3_char_ram_dirty[realtileno] = 0;
									}

									if (global_alpha || alpha)
									{
										cps3_drawgfxzoom(renderbuffer_bitmap, Machine->gfx[1],realtileno,actualpal,0^flipx,0^flipy,current_xpos,current_ypos,&renderbuffer_clip,CPS3_TRANSPARENCY_PEN_INDEX_BLEND,0,xinc,yinc, NULL, 0);
									}
									else
									{
										cps3_drawgfxzoom(renderbuffer_bitmap, Machine->gfx[1],realtileno,actualpal,0^flipx,0^flipy,current_xpos,current_ypos,&renderbuffer_clip,CPS3_TRANSPARENCY_PEN_INDEX,0,xinc,yinc, NULL, 0);
									}
									count++;
								}
							}
						}
					}
	//              */

				//  printf("cell %08x %08x %08x\n",value1, value2, value3);
				}
			}
		}
	}

	/* copy render bitmap with zoom */
	{
		UINT32 renderx,rendery;
		UINT32 srcx, srcy;
		UINT32* srcbitmap;
		UINT32* dstbitmap;


		srcy=0;
		for (rendery=0;rendery<224;rendery++)
		{
			dstbitmap = (UINT32 *)bitmap->line[rendery];
			srcbitmap = (UINT32 *)renderbuffer_bitmap->line[srcy>>16];
			srcx=0;

			for (renderx=0;renderx<384;renderx++)
			{
				dstbitmap[renderx] = cps3_mame_colours[srcbitmap[srcx>>16]&0x1ffff];
				srcx += fszx;
			}

			srcy += fszy;
		}
	}

       /* fg layer - joystick combo meter in JoJo games relies on rowscroll */
       {
               int line;

               /* bank select? (sfiii2 intro) */
               if (cps3_ss_bank_base & 0x01000000)
                       count = 0x000;
               else
                       count = 0x800;

               for (line = cliprect->min_y; line <= cliprect->max_y; line++)
               {
                       rectangle clip_line;
                       int y = line / 8;
                       int rowscroll;

                       clip_line = *cliprect;
                       clip_line.min_y = clip_line.max_y = line;

                       rowscroll = cps3_ss_ram[((line - 1) & 0x1ff) + 0x4000/4] >> 16;
                       count = (y * 64) + (cps3_ss_bank_base & 0x01000000 ? 0x000 : 0x800);

                       for (x = 0; x < 64; x++)
                       {
                               UINT32 data = cps3_ss_ram[count]; /* +0x800 = 2nd bank, used on sfiii2 intro.. */
                               UINT32 tile = (data >> 16) & 0x1ff;
                               int pal = (data & 0x003f) >> 1;
                               int flipx = (data & 0x0080) >> 7;
                               int flipy = (data & 0x0040) >> 6;
                               pal += cps3_ss_pal_base << 5;
                               tile += 0x200;

                               if (cps3_ss_ram_dirty[tile])
                               {
                                       decodechar(Machine->gfx[0], tile, (UINT8*)cps3_ss_ram, &cps3_tiles8x8_layout);
                                       cps3_ss_ram_dirty[tile] = 0;
                               }

                               cps3_drawgfxzoom(bitmap, Machine->gfx[0], tile, pal, flipx, flipy,
                                               (x * 8) - rowscroll, y * 8, &clip_line,
                                               CPS3_TRANSPARENCY_PEN, 0, 0x10000, 0x10000, NULL, 0);
                               cps3_drawgfxzoom(bitmap, Machine->gfx[0], tile, pal, flipx, flipy,
                                               512 + (x * 8) - rowscroll, y * 8, &clip_line,
                                               CPS3_TRANSPARENCY_PEN, 0, 0x10000, 0x10000, NULL, 0);
                               count++;
                       }
               }
       }
	return;
}

READ32_HANDLER( cps3_ssram_r )
{
	if (offset>0x8000/4)
		return LE32(cps3_ss_ram[offset]);
	else
		return cps3_ss_ram[offset];
}

WRITE32_HANDLER( cps3_ssram_w )
{
	if (offset>0x8000/4)
	{
		// we only want to endian-flip the character data, the tilemap info is fine
		data = LE32(data);
		mem_mask = LE32(mem_mask);

		cps3_ss_ram_dirty[offset/16] = 1;
		cps3_ss_ram_is_dirty = 1;
	}

	COMBINE_DATA(&cps3_ss_ram[offset]);
}

WRITE32_HANDLER( cps3_ram_w )
{
	COMBINE_DATA( &cps3_ram[offset] );
	// store a decrypted copy
	cps3_ram_decrypted[offset] = cps3_ram[offset]^cps3_mask(offset*4+0xc0000000, cps3_key1, cps3_key2);
}

static offs_t cps3_opbase_handler(offs_t address)
{
//  if(DEBUG_PRINTF) printf("address %04x\n",address);

	/* BIOS ROM */
	if (address < 0x80000)
	{
		opcode_base = opcode_arg_base = memory_region(REGION_USER1);
		return ~0;
	}
	/* RAM */
	else if (address >= 0x06000000 && address <= 0x06ffffff)
	{
		opcode_base = (UINT8*)decrypted_gamerom-0x06000000;
		opcode_arg_base = (UINT8*)decrypted_gamerom-0x06000000;

		if (cps3_isSpecial) opcode_arg_base = (UINT8*) memory_region(REGION_USER4)-0x06000000;


		return ~0;
	}
	else if (address >= 0xc0000000 && address <= 0xc00003ff)
	{
//      opcode_base = (void*)cps3_ram_decrypted;
		opcode_base = (UINT8*)cps3_ram_decrypted-0xc0000000;
		opcode_arg_base = (UINT8*)cps3_ram-0xc0000000;
		return ~0;
	}

	/* anything else falls through to NOPs */
	opcode_base = (UINT8*)cps3_nops-address;
	opcode_arg_base = (UINT8*)cps3_nops-address;
	return ~0;
}

UINT32 cram_bank = 0;

WRITE32_HANDLER( cram_bank_w )
{
	if (ACCESSING_LSB)
	{
		// this seems to be related to accesses to the 0x04100000 region
		if (cram_bank != data)
		{
			cram_bank = data;
		//if(data&0xfffffff0)
		//bank_w 00000000, ffff0000
		//bank_w 00000001, ffff0000
		//bank_w 00000002, ffff0000
		//bank_w 00000003, ffff0000
		//bank_w 00000004, ffff0000
		//bank_w 00000005, ffff0000
		//bank_w 00000006, ffff0000
		//bank_w 00000007, ffff0000
		// during CHARACTER RAM test..
			if(DEBUG_PRINTF) printf("bank_w %08x, %08x\n",data,mem_mask);

		}
	}
	else
	{
		if(DEBUG_PRINTF) printf("bank_w LSB32 %08x, %08x\n",data,mem_mask);

	}
}

READ32_HANDLER( cram_data_r )
{
	UINT32 fulloffset = (((cram_bank&0x7)*0x100000)/4) + offset;

	return cps3_char_ram[fulloffset];
}

WRITE32_HANDLER( cram_data_w )
{
	UINT32 fulloffset = (((cram_bank&0x7)*0x100000)/4) + offset;
	mem_mask = LE32(mem_mask);
	data = LE32(data);
	COMBINE_DATA(&cps3_char_ram[fulloffset]);
	cps3_char_ram_dirty[fulloffset/0x40] = 1;
	cps3_char_ram_is_dirty = 1;
}

/* FLASH ROM ACCESS */

READ32_HANDLER( cps3_gfxflash_r )
{
	UINT32 result = 0;

	if(DEBUG_PRINTF) printf("gfxflash_r\n");

	if (!(mem_mask & 0xff000000))	// GFX Flash 1
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1));
		result |= intelflash_read(8, (offset<<1) ) << 24;
	}
	if (!(mem_mask & 0x00ff0000))	// GFX Flash 2
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1));
		result |= intelflash_read(9, (offset<<1) ) << 16;
	}
	if (!(mem_mask & 0x0000ff00))	// GFX Flash 1
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1)+1);
		result |= intelflash_read(8, (offset<<1)+0x1 ) << 8;
	}
	if (!(mem_mask & 0x000000ff))	// GFX Flash 2
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1)+1);
		result |= intelflash_read(9, (offset<<1)+0x1 ) << 0;
	}

	printf("read GFX flash chips addr %02x returning %08x mem_mask %08x crambank %08x gfxbank %08x\n", offset*2, result,mem_mask,  cram_bank, cram_gfxflash_bank  );

	return result;
}

WRITE32_HANDLER( cps3_gfxflash_w )
{

	int command;

	if(DEBUG_PRINTF) printf("cps3_gfxflash_w %08x %08x %08x\n", offset *2, data, mem_mask);


	if (!(mem_mask & 0xff000000))	// GFX Flash 1
	{
		command = (data >> 24) & 0xff;
		logerror("write to GFX flash chip 1 addr %02x cmd %02x\n", (offset<<1), command);
		intelflash_write(8, (offset<<1), command);
	}
	if (!(mem_mask & 0x00ff0000))	// GFX Flash 2
	{
		command = (data >> 16) & 0xff;
		logerror("write to GFX flash chip 2 addr %02x cmd %02x\n", (offset<<1), command);
		intelflash_write(9, (offset<<1), command);
	}
	if (!(mem_mask & 0x0000ff00))	// GFX Flash 1
	{
		command = (data >> 8) & 0xff;
		logerror("write to GFX flash chip 1 addr %02x cmd %02x\n", (offset<<1)+1, command);
		intelflash_write(8, (offset<<1)+0x1, command);
	}
	if (!(mem_mask & 0x000000ff))	// GFX Flash 2
	{
		command = (data >> 0) & 0xff;
		logerror("write to GFX flash chip 2 addr %02x cmd %02x\n", (offset<<1)+1, command);
		intelflash_write(9, (offset<<1)+0x1, command);
	}
}



UINT32 cps3_flashmain_r(int base, UINT32 offset, UINT32 mem_mask)
{
	UINT32 result = 0;

	if (!(mem_mask & 0xff000000))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+0, offset*4 );
		result |= (intelflash_read(base+0, offset)<<24);
	}
	if (!(mem_mask & 0x00ff0000))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+1, offset*4 );
		result |= (intelflash_read(base+1, offset)<<16);
	}
	if (!(mem_mask & 0x0000ff00))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+2, offset*4 );
		result |= (intelflash_read(base+2, offset)<<8);
	}
	if (!(mem_mask & 0x000000ff))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+3, offset*4 );
		result |= (intelflash_read(base+3, offset)<<0);
	}

	if (base==4) logerror("read flash chips addr %02x returning %08x\n", offset*4, result );

	return result;
}

READ32_HANDLER( cps3_flash1_r )
{
	return cps3_flashmain_r(0, offset,mem_mask);
}

READ32_HANDLER( cps3_flash2_r )
{
	return cps3_flashmain_r(4, offset,mem_mask);
}

void cps3_flashmain_w(int base, UINT32 offset, UINT32 data, UINT32 mem_mask)
{
	int command;
	if (!(mem_mask & 0xff000000))	// Flash 1
	{
		command = (data >> 24) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+0, offset, command);
		intelflash_write(base+0, offset, command);
	}
	if (!(mem_mask & 0x00ff0000))	// Flash 2
	{
		command = (data >> 16) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+1, offset, command);
		intelflash_write(base+1, offset, command);
	}
	if (!(mem_mask & 0x0000ff00))	// Flash 2
	{
		command = (data >> 8) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+2, offset, command);
		intelflash_write(base+2, offset, command);
	}
	if (!(mem_mask & 0x000000ff))	// Flash 2
	{
		command = (data >> 0) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+3, offset, command);
		intelflash_write(base+3, offset, command);
	}
}
WRITE32_HANDLER( cps3_flash1_w )
{
	cps3_flashmain_w(0, offset,data,mem_mask);
}

WRITE32_HANDLER( cps3_flash2_w )
{
	cps3_flashmain_w(4, offset,data,mem_mask);
}

WRITE32_HANDLER( cram_gfxflash_bank_w )
{
	if (ACCESSING_MSB32)
	{
		cram_gfxflash_bank = (data & 0xffff0000) >> 16;
		cram_gfxflash_bank-= 0x0002;// as with sound access etc. first 4 meg is 'special' and skipped
	}

	if (ACCESSING_LSB32)
	{
		if(DEBUG_PRINTF) printf("cram_gfxflash_bank_LSB_w LSB32 %08x\n",data);
	}
}

READ32_HANDLER( cps3_io1_r )
{
	return readinputport(0);
}

READ32_HANDLER( cps3_io2_r )
{
	return readinputport(1);
}

// this seems to be dma active flags, and maybe vblank... not if it is anything else
READ32_HANDLER( cps3_vbl_r )
{
	return 0x00000000;
}

READ32_HANDLER( cps3_unk_io_r )
{
	//  warzard will crash before booting if you return anything here
	return 0xffffffff;
}

READ32_HANDLER( cps3_40C0000_r )
{
	return 0x00000000;
}

READ32_HANDLER( cps3_40C0004_r )
{
	return 0x00000000;
}

/* EEPROM access is a little odd, I think it accesses eeprom through some kind of
   additional interface, as these writes aren't normal for the type of eeprom we have */
UINT16 cps3_current_eeprom_read;

READ32_HANDLER( cps3_eeprom_r )
{
	int addr = offset*4;

	if (addr>=0x100 && addr<=0x17f)
	{
		if (ACCESSING_MSB32) cps3_current_eeprom_read = (cps3_eeprom[offset-0x100/4] & 0xffff0000)>>16;
		else cps3_current_eeprom_read = (cps3_eeprom[offset-0x100/4] & 0x0000ffff)>>0;
		// read word to latch...
		return 0x00000000;
	}
	else if (addr == 0x200)
	{
		// busy flag / read data..
		if (ACCESSING_MSB32) return 0;
		else
		{
			//if(DEBUG_PRINTF) printf("reading %04x from eeprom\n", cps3_current_eeprom_read);
			return cps3_current_eeprom_read;
		}
	}
	else
	{
	//  if(DEBUG_PRINTF) printf("unk read eeprom addr %04x, mask %08x\n", addr, mem_mask);
		return 0x00000000;
	}
	return 0x00000000;
}

WRITE32_HANDLER( cps3_eeprom_w )
{
	int addr = offset*4;

	if (addr>=0x080 && addr<=0x0ff)
	{
		offset -= 0x80/4;
		COMBINE_DATA(&cps3_eeprom[offset]);
		// write word to storage

	}
	else if (addr>=0x180 && addr<=0x1ff)
	{
		// always 00000000 ? incrememnt access?
	}
	else
	{
	//  if(DEBUG_PRINTF) printf("unk write eeprom addr %04x, data %08x, mask %08x\n", addr, data, mem_mask);
	}

}

READ32_HANDLER( cps3_cdrom_r )
{
	return 0x00000000;
}

WRITE32_HANDLER( cps3_ss_bank_base_w )
{
	// might be scroll registers or something else..
	// used to display bank with 'insert coin' on during sfiii2 attract intro
	COMBINE_DATA(&cps3_ss_bank_base);

//  printf("cps3_ss_bank_base_w %08x %08x\n", data, mem_mask);
}

WRITE32_HANDLER( cps3_ss_pal_base_w )
{
	 if(DEBUG_PRINTF) printf ("cps3_ss_pal_base_w %08x %08x\n", data, mem_mask);

	if(ACCESSING_MSB32)
	{
		cps3_ss_pal_base = (data & 0x00ff0000)>>16;

		if (data & 0xff000000) printf("cps3_ss_pal_base MSB32 upper bits used %04x \n", data);
	}
	else
	{
	//	printf("cps3_ss_pal_base LSB32 used %04x \n", data);
	}
}

UINT32*tilemap20_regs_base;
UINT32*tilemap30_regs_base;
UINT32*tilemap40_regs_base;
UINT32*tilemap50_regs_base;

//<ElSemi> +0 X  +2 Y +4 unknown +6 enable (&0x8000) +8 low part tilemap base, high part linescroll base
//<ElSemi> (a word each)

UINT32 paldma_source;
UINT32 paldma_realsource;
UINT32 paldma_dest;
UINT32 paldma_fade;
UINT32 paldma_other2;
UINT32 paldma_length;
static mame_timer* cps3_dma10_timer;
static int cps3_dma10_pending;

WRITE32_HANDLER( cps3_palettedma_w )
{
	if (offset==0)
	{
		COMBINE_DATA(&paldma_source);
		paldma_realsource = (paldma_source<<1)-0x400000;
	}
	else if (offset==1)
	{
		COMBINE_DATA(&paldma_dest);
	}
	else if (offset==2)
	{
		COMBINE_DATA(&paldma_fade);
	}
	else if (offset==3)
	{
		COMBINE_DATA(&paldma_other2);

		if (ACCESSING_MSB32)
		{
			paldma_length = (data & 0xffff0000)>>16;
		}
		if (ACCESSING_LSB32)
		{
			if (data & 0x0002)
			{
				int i;
				UINT32 dmalen = paldma_length | ((data & 0x0001) << 16);
			//  if(DEBUG_PRINTF) printf("CPS3 pal dma start %08x (real: %08x) dest %08x fade %08x other2 %08x (length %04x)\n", paldma_source, paldma_realsource, paldma_dest, paldma_fade, paldma_other2, paldma_length);

				for (i=0;i<dmalen;i++)
				{
					UINT16* src = (UINT16*)memory_region(REGION_USER5);
					UINT16 coldata = src[BYTE_XOR_BE(((paldma_realsource>>1)+i))];

					cps3_set_mame_colours((paldma_dest+i)^1, coldata, paldma_fade);
				}
				cps3_dma10_pending = 1;
				timer_adjust(cps3_dma10_timer, TIME_IN_USEC(100), 0, 0);
			}
		}
	}

}

UINT32 chardma_source;
UINT32 chardma_other;

UINT8* current_table;
UINT32 current_table_address = -1;

int cps3_rle_length = 0;

UINT8 last_normal_byte = 0;


UINT32 process_byte( UINT8 real_byte, UINT32 destination, int max_length )
{
	UINT8* dest       = (UINT8*)cps3_char_ram;

	//printf("process byte for destination %08x\n", destination);

	destination&=0x7fffff;

	if (real_byte&0x40)
	{
		int tranfercount = 0;

		//printf("Set RLE Mode\n");
		cps3_rle_length = (real_byte&0x3f)+1;

		//printf("RLE Operation (length %08x\n", cps3_rle_length );

		while (cps3_rle_length)
		{
			dest[((destination+tranfercount)&0x7fffff)^3] = (last_normal_byte&0x3f);
			cps3_char_ram_dirty[((destination+tranfercount)&0x7fffff)/0x100] = 1;
			cps3_char_ram_is_dirty = 1;
			//printf("RLE WRite Byte %08x, %02x\n", destination+tranfercount, real_byte);

			tranfercount++;
			cps3_rle_length--;
			max_length--;

			if ((destination+tranfercount) > 0x7fffff)  return max_length;

	//      if (max_length==0) return max_length; // this is meant to abort the transfer if we exceed dest length,, not working
		}
		return tranfercount;
	}
	else
	{
		//printf("Write Normal Data\n");
		dest[(destination&0x7fffff)^3] = real_byte;
		last_normal_byte = real_byte;
		cps3_char_ram_dirty[(destination&0x7fffff)/0x100] = 1;
		cps3_char_ram_is_dirty = 1;
		return 1;
	}
}

void cps3_do_char_dma( UINT32 real_source, UINT32 real_destination, UINT32 real_length )
{
	UINT8* sourcedata = (UINT8*)memory_region(REGION_USER5);
	int length_remaining;

	last_normal_byte = 0;
	cps3_rle_length = 0;
	length_remaining = real_length;
	while (length_remaining)
	{
		UINT8 current_byte;

		current_byte = sourcedata[DMA_XOR(real_source)];
		real_source++;

		if (current_byte & 0x80)
		{
			UINT8 real_byte;
			UINT32 length_processed;
			current_byte &= 0x7f;

			real_byte = sourcedata[DMA_XOR((current_table_address+current_byte*2+0))];
			//if (real_byte&0x80) return;
			length_processed = process_byte( real_byte, real_destination, length_remaining );
			length_remaining-=length_processed; // subtract the number of bytes the operation has taken
			real_destination+=length_processed; // add it onto the destination
			if (real_destination>0x7fffff) return;
			if (length_remaining<=0) return; // if we've expired, exit

			real_byte = sourcedata[DMA_XOR((current_table_address+current_byte*2+1))];
			//if (real_byte&0x80) return;
			length_processed = process_byte( real_byte, real_destination, length_remaining );
			length_remaining-=length_processed; // subtract the number of bytes the operation has taken
			real_destination+=length_processed; // add it onto the destination
			if (real_destination>0x7fffff) return;
			if (length_remaining<=0) return;  // if we've expired, exit
		}
		else
		{
			UINT32 length_processed;
			length_processed = process_byte( current_byte, real_destination, length_remaining );
			length_remaining-=length_processed; // subtract the number of bytes the operation has taken
			real_destination+=length_processed; // add it onto the destination
			if (real_destination>0x7fffff) return;
			if (length_remaining<=0) return;  // if we've expired, exit
		}

//      length_remaining--;
	}
}

unsigned short lastb;
unsigned short lastb2;
unsigned int ProcessByte8(unsigned char b,UINT32 dst_offset)
{
	UINT8* destRAM = (UINT8*)cps3_char_ram;
 	int l=0;

 	if(lastb==lastb2)	//rle
 	{
		int i;
 		int rle=(b+1)&0xff;

 		for(i=0;i<rle;++i)
 		{
			destRAM[(dst_offset&0x7fffff)^3] = lastb;
			cps3_char_ram_dirty[(dst_offset&0x7fffff)/0x100] = 1;
			cps3_char_ram_is_dirty = 1;

			dst_offset++;
 			++l;
 		}
 		lastb2=0xffff;

 		return l;
 	}
 	else
 	{
 		lastb2=lastb;
 		lastb=b;
		destRAM[(dst_offset&0x7fffff)^3] = b;
		cps3_char_ram_dirty[(dst_offset&0x7fffff)/0x100] = 1;
		cps3_char_ram_is_dirty = 1;
 		return 1;
 	}
 }

void cps3_do_alt_char_dma( UINT32 src, UINT32 real_dest, UINT32 real_length )
{
	UINT8* px = (UINT8*)memory_region(REGION_USER5);
	UINT32 start = real_dest;
	UINT32 ds = real_dest;

	lastb=0xfffe;
	lastb2=0xffff;

	while(1)
	{
		int i;
		UINT8 ctrl=px[DMA_XOR(src)];
 		++src;

		for(i=0;i<8;++i)
		{
			UINT8 p=px[DMA_XOR(src)];

			if(ctrl&0x80)
			{
				UINT8 real_byte;
				p&=0x7f;
				real_byte = px[DMA_XOR((current_table_address+p*2+0))];
				ds+=ProcessByte8(real_byte,ds);
				real_byte = px[DMA_XOR((current_table_address+p*2+1))];
				ds+=ProcessByte8(real_byte,ds);
 			}
 			else
 			{
 				ds+=ProcessByte8(p,ds);
 			}
 			++src;
 			ctrl<<=1;

			if((ds-start)>=real_length)
				return;
 		}
	}
}

void cps3_process_character_dma(UINT32 address)
{
	int i;

	//printf("charDMA start:\n");

	for (i=0;i<0x1000;i+=3)
	{
		UINT32 dat1 = cps3_char_ram[i+0+(address)];
		UINT32 dat2 = cps3_char_ram[i+1+(address)];
		UINT32 dat3 = cps3_char_ram[i+2+(address)];
		UINT32 real_source      = (dat3<<1)-0x400000;
		UINT32 real_destination =  dat2<<3;
		UINT32 real_length      = (((dat1&0x001fffff)+1)<<3);

		/* 0x01000000 is the end of list marker, 0x13131313 is our default fill */
		if ((dat1==0x01000000) || (dat1==0x13131313)) break;

        //printf("%08x %08x %08x real_source %08x (rom %d offset %08x) real_destination %08x, real_length %08x\n", dat1, dat2, dat3, real_source, real_source/0x800000, real_source%0x800000, real_destination, real_length);

		if  ( (dat1&0x00e00000) ==0x00800000 )
		{
			/* Sets a table used by the decompression routines */
			{
				/* We should probably copy this, but a pointer to it is fine for our purposes as the data doesn't change */
				current_table_address = real_source;
			}
			cpunum_set_input_line(0,10, ASSERT_LINE);
		}
		else if  ( (dat1&0x00e00000) ==0x00400000 )
		{
			/* 6bpp DMA decompression
              - this is used for the majority of sprites and backgrounds */
			cps3_do_char_dma( real_source, real_destination, real_length );
			cpunum_set_input_line(0,10, ASSERT_LINE);

		}
		else if  ( (dat1&0x00e00000) ==0x00600000 )
		{
			/* 8bpp DMA decompression
              - this is used on SFIII NG Sean's Stage ONLY */
			cps3_do_alt_char_dma( real_source, real_destination, real_length);
			cpunum_set_input_line(0,10, ASSERT_LINE);
		}
		else
		{
			printf("Unknown DMA List Command Type\n"); // warzard uses command 0, uncompressed? but for what?
		}

	}
}

WRITE32_HANDLER( cps3_characterdma_w )
{
	if(DEBUG_PRINTF) printf("chardma_w %08x %08x %08x\n", offset, data, mem_mask);

	if (offset==0)
	{
		//COMBINE_DATA(&chardma_source);
		if (ACCESSING_LSB32)
		{
			chardma_source = data & 0x0000ffff;
		}
		if (ACCESSING_MSB32)
		{
			if(DEBUG_PRINTF) printf("chardma_w accessing MSB32 of offset 0");
		}
	}
	else if (offset==1)
	{
		COMBINE_DATA(&chardma_other);

		if (ACCESSING_MSB32)
		{
			if ((data>>16) & 0x0040)
			{
				UINT32 list_address;
				list_address = (chardma_source | ((chardma_other&0x003f0000)));

				//printf("chardma_w activated %08x %08x (address = cram %08x)\n", chardma_source, chardma_other, list_address*4 );
				cps3_process_character_dma(list_address);
			}
			else
			{
				if(DEBUG_PRINTF) printf("chardma_w NOT activated %08x %08x\n", chardma_source, chardma_other );
			}

			if ((data>>16) & 0xff80)
				if(DEBUG_PRINTF) printf("chardma_w unknown bits in activate command %08x %08x\n", chardma_source, chardma_other );
		}
		else
		{
			if(DEBUG_PRINTF) printf("chardma_w LSB32 write to activate command %08x %08x\n", chardma_source, chardma_other );
		}
	}
}

WRITE32_HANDLER( cps3_irq10_ack_w )
{
	cpunum_set_input_line(0,10, CLEAR_LINE); return;
}

WRITE32_HANDLER( cps3_irq12_ack_w )
{
	cpunum_set_input_line(0,12, CLEAR_LINE); return;
}

WRITE32_HANDLER( cps3_unk_vidregs_w )
{
	COMBINE_DATA(&cps3_unk_vidregs[offset]);
}

READ32_HANDLER( cps3_colourram_r )
{
	UINT16* src = (UINT16*)cps3_colourram;

	return src[offset*2+1] | (src[offset*2+0]<<16);
}

WRITE32_HANDLER( cps3_colourram_w )
{
//	COMBINE_DATA(&cps3_colourram[offset]);

	if (ACCESSING_MSB32)
	{
		cps3_set_mame_colours(offset*2, (data & 0xffff0000) >> 16,0);
	}
	
	if (ACCESSING_LSB32)
	{
		cps3_set_mame_colours(offset*2+1, (data & 0x0000ffff) >> 0,0);
	}
}

UINT32* cps3_mainram;

/* there are more unknown writes, but you get the idea */
static ADDRESS_MAP_START( cps3_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM AM_REGION(REGION_USER1, 0) // Bios ROM
	AM_RANGE(0x02000000, 0x0207ffff) AM_RAM AM_BASE(&cps3_mainram) // Main RAM

	AM_RANGE(0x03000000, 0x030003ff) AM_RAM // 'FRAM' (SFIII memory test mode ONLY)

//	AM_RANGE(0x04000000, 0x0407dfff) AM_RAM AM_BASE(&cps3_spriteram)//AM_WRITE(MWA32_RAM) // Sprite RAM (jojoba tests this size)
	AM_RANGE(0x04000000, 0x0407ffff) AM_RAM AM_BASE(&cps3_spriteram)//AM_WRITE(MWA32_RAM) // Sprite RAM 

	AM_RANGE(0x04080000, 0x040bffff) AM_RAM AM_READWRITE(cps3_colourram_r, cps3_colourram_w) AM_BASE(&cps3_colourram)  // Colour RAM (jojoba tests this size) 0x20000 colours?!

	// video registers of some kind probably
	AM_RANGE(0x040C0000, 0x040C0003) AM_READ(cps3_40C0000_r)//?? every frame
	AM_RANGE(0x040C0004, 0x040C0007) AM_READ(cps3_40C0004_r)//AM_READ(cps3_40C0004_r) // warzard reads this!
//  AM_RANGE(0x040C0008, 0x040C000b) AM_WRITE(MWA32_NOP)//??
    AM_RANGE(0x040C000c, 0x040C000f) AM_READ(cps3_vbl_r)// AM_WRITE(MWA32_NOP)/

	AM_RANGE(0x040C0000, 0x040C001f) AM_WRITE(cps3_unk_vidregs_w)
	AM_RANGE(0x040C0020, 0x040C002b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap20_regs_base)
	AM_RANGE(0x040C0030, 0x040C003b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap30_regs_base)
	AM_RANGE(0x040C0040, 0x040C004b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap40_regs_base)
	AM_RANGE(0x040C0050, 0x040C005b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap50_regs_base)

	AM_RANGE(0x040C0060, 0x040C007f) AM_RAM AM_BASE(&cps3_fullscreenzoom)


	AM_RANGE(0x040C0094, 0x040C009b) AM_WRITE(cps3_characterdma_w)


	AM_RANGE(0x040C00a0, 0x040C00af) AM_WRITE(cps3_palettedma_w)


	AM_RANGE(0x040C0084, 0x040C0087) AM_WRITE(cram_bank_w)
	AM_RANGE(0x040C0088, 0x040C008b) AM_WRITE(cram_gfxflash_bank_w)

	AM_RANGE(0x040e0000, 0x040e02ff) AM_READWRITE(cps3_sound_r, cps3_sound_w)

	AM_RANGE(0x04100000, 0x041fffff) AM_READWRITE(cram_data_r, cram_data_w)
	AM_RANGE(0x04200000, 0x043fffff) AM_READWRITE(cps3_gfxflash_r, cps3_gfxflash_w) // GFX Flash ROMS

	AM_RANGE(0x05000000, 0x05000003) AM_READ( cps3_io1_r )
	AM_RANGE(0x05000004, 0x05000007) AM_READ( cps3_io2_r )

	AM_RANGE(0x05000008, 0x0500000b) AM_WRITE( MWA32_NOP ) // ?? every frame

	AM_RANGE(0x05000a00, 0x05000a1f) AM_READ( cps3_unk_io_r ) // ?? every frame

	AM_RANGE(0x05001000, 0x05001203) AM_READWRITE( cps3_eeprom_r, cps3_eeprom_w )

	AM_RANGE(0x05040000, 0x0504ffff) AM_RAM AM_READWRITE(cps3_ssram_r,cps3_ssram_w) // 'SS' RAM (Score Screen) (text tilemap + toles)
	//0x25050020
	AM_RANGE(0x05050020, 0x05050023) AM_WRITE( cps3_ss_bank_base_w )
	AM_RANGE(0x05050024, 0x05050027) AM_WRITE( cps3_ss_pal_base_w )

	AM_RANGE(0x05100000, 0x05100003) AM_WRITE( cps3_irq12_ack_w )
	AM_RANGE(0x05110000, 0x05110003) AM_WRITE( cps3_irq10_ack_w )

	AM_RANGE(0x05140000, 0x05140003) AM_READ( cps3_cdrom_r )

	AM_RANGE(0x06000000, 0x067fffff) AM_ROM AM_READWRITE( cps3_flash1_r, cps3_flash1_w ) /* Flash ROMs simm 1 */
	AM_RANGE(0x06800000, 0x06ffffff) AM_ROM AM_READWRITE( cps3_flash2_r, cps3_flash2_w ) /* Flash ROMs simm 2 */

	AM_RANGE(0xc0000000, 0xc00003ff) AM_RAM AM_WRITE( cps3_ram_w ) AM_BASE(&cps3_ram) /* Executes code from here */
ADDRESS_MAP_END




/* ------------------------------------------------------------------------- */
/* 置き換え用：INPUT_PORTS_START(cps3) 全体                                  */
/* ------------------------------------------------------------------------- */
INPUT_PORTS_START( cps3 )
    /* ---------- PLAYERS 1 & 2 COMMON (PORT 0) ---------- */
    PORT_START
    PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1)
    PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1)
    PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)
    PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
    PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1        ) PORT_PLAYER(1)
    PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2        ) PORT_PLAYER(1)
    PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3        ) PORT_PLAYER(1)
    PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2)
    PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2)
    PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2)
    PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
    PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1        ) PORT_PLAYER(2)
    PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2        ) PORT_PLAYER(2)
    PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_BUTTON3        ) PORT_PLAYER(2)
    PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_SERVICE         ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
    PORT_BIT( 0x00fc0000, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x04000000, IP_ACTIVE_LOW, IPT_BUTTON6        ) PORT_PLAYER(2)
    PORT_BIT( 0x08000000, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0xc0000000, IP_ACTIVE_LOW, IPT_UNUSED )

    /* ---------- EXTRA BUTTONS / UNUSED (PORT 1) ---------- */
    PORT_START
    PORT_BIT( 0x0001ffff, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
    PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
    PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
    PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
    PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
    PORT_BIT( 0xffc00000, IP_ACTIVE_LOW, IPT_UNUSED )

    /* ---------- CONFIGURATION DIP (PORT 2) ---------- */
    PORT_START
    /* 0x0F = Region (既存) */
    PORT_DIPNAME( 0x0000000f, 0x00000000, DEF_STR( Region ) )
    PORT_DIPSETTING( 0x00000000, "Default" )
    PORT_DIPSETTING( 0x00000001, DEF_STR( Japan ) )
    PORT_DIPSETTING( 0x00000002, DEF_STR( Asia ) )
    PORT_DIPSETTING( 0x00000003, DEF_STR( Europe ) )
    PORT_DIPSETTING( 0x00000004, DEF_STR( USA ) )
    PORT_DIPSETTING( 0x00000005, DEF_STR( Hispanic ) )
    PORT_DIPSETTING( 0x00000006, "Brazil" )
    PORT_DIPSETTING( 0x00000007, "Oceania" )
    PORT_DIPSETTING( 0x00000008, "Korea" )

    /* 0x10 = Buffer Delay ON/OFF（新規追加） */
    PORT_DIPNAME( 0x00000010, 0x00000010, "Buffer Delay (Sprite Lag)" )
    PORT_DIPSETTING( 0x00000010, DEF_STR( On ) )   /* 1‑frame lag – hardware‑accurate */
    PORT_DIPSETTING( 0x00000000, DEF_STR( Off ) )  /* No lag – low‑latency mode      */
INPUT_PORTS_END


/* ------------------------------------------------------------------------- */
/* 置き換え用：static INTERRUPT_GEN(cps3_interrupt) 全体                      */
/* ------------------------------------------------------------------------- */
static INTERRUPT_GEN(cps3_interrupt)
{
    static int i = 0;
    i++;
    i %= 16;            /* 1 フレームを 16 分割して呼び出される */

    /* Buffer Delay DIP（Port 2 bit 0x10）
       ON  : ハードウェア同等の 1 フレーム遅延
       OFF : 遅延を無効化（低レイテンシ）                       */
    if (readinputport(2) & 0x00000010)     /* Buffer Delay = ON */
    {
        if (i == 8)                        /* 従来どおり半フレーム後 */
        {
            cpunum_set_input_line(0, 10, ASSERT_LINE);   /* IRQ10 : スプライト DMA 完了 */
            return;
        }
    }
    else                                   /* Buffer Delay = OFF */
    {
        if (i == 1)                        /* ほぼ即時（slice 1）で IRQ10 */
        {
            cpunum_set_input_line(0, 10, ASSERT_LINE);   /* 1 フレーム短縮 */
            /* return しない—IRQ12 を後で発生させるため継続 */
        }
    }

    /* V‑Blank 対応 IRQ12（従来どおり slice 14） */
    if (i == 14)
    {
        cpunum_set_input_line(0, 12, ASSERT_LINE);
        return;
    }
}


struct sh2_config sh2cp_conf_slave  = { 1 };


static struct CustomSound_interface custom_interface =
{
	cps3_sh_start
};


mame_timer* fastboot_timer;

static void fastboot_timer_callback(int num)
{
	UINT32 *rom =  (UINT32*)decrypted_gamerom;//memory_region ( REGION_USER4 );
	if (cps3_isSpecial) rom = (UINT32*)memory_region(REGION_USER4);

//  printf("fastboot callback %08x %08x", rom[0], rom[1]);
	cpunum_set_reg(0,SH2_PC, rom[0]);
	cpunum_set_reg(0,SH2_R15, rom[1]);
	cpunum_set_reg(0,SH2_VBR, 0x6000000);
}

static void cps3_dma10_timer_callback(int num)
{
	if (cps3_dma10_pending)
	{
		cps3_dma10_pending = 0;
		cpunum_set_input_line(0,10, ASSERT_LINE);
	}
}

MACHINE_RESET(cps3_reset)
{
	UINT32 *rom = (UINT32*)memory_region ( REGION_USER1 );
	UINT32 dip = readinputport(2) & 0xf;

	fastboot_timer = timer_alloc(fastboot_timer_callback);
	cps3_dma10_timer = timer_alloc(cps3_dma10_timer_callback);
	cps3_dma10_pending = 0;
//  printf("reset\n");
	timer_adjust(fastboot_timer,  TIME_NOW, 0, 0);

	if (dip)
	{
	if(strcmp(Machine->gamedrv->name,"warzard")==0 || strcmp(Machine->gamedrv->name,"redearth")==0)
		{
		rom[0x1fed8/4] &= ~0x0000000f; // region hack
		rom[0x1fed8/4] |= dip;
		}
	else
		{
		rom[0x1fec8/4] &= ~0x0000000f; // region hack
		rom[0x1fec8/4] |= dip;
		}
	}

}

#define MASTER_CLOCK	42954500

static NVRAM_HANDLER( cps3 )
{
	/* ALSO save NVRAMs */

	if (read_or_write)
		mame_fwrite(file, cps3_eeprom, 0x400);
	else if (file)
		mame_fread(file, cps3_eeprom, 0x400);
}


static MACHINE_DRIVER_START( cps3 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, 6250000*4) // external clock is 6.25 Mhz, it sets the intenral multiplier to 4x (this should probably be handled in the core..)
	MDRV_CPU_PROGRAM_MAP(cps3_map,0)
	MDRV_CPU_VBLANK_INT(cps3_interrupt,16)
//  MDRV_CPU_CONFIG(sh2cp_conf_slave)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(384*2, 512)
//  MDRV_SCREEN_VISIBLE_AREA(0, 383, 0, 223/*511*/) // you can see debug texts in jojo with 511

	MDRV_MACHINE_RESET(cps3_reset)
	MDRV_NVRAM_HANDLER( cps3 )

	MDRV_VISIBLE_AREA(0, (384*1)-1, 0, 223/*511*/) // you can see debug texts in jojo with 511

	MDRV_PALETTE_LENGTH(0x10000)

	MDRV_VIDEO_START(cps3)
	MDRV_VIDEO_UPDATE(cps3)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(CUSTOM, MASTER_CLOCK / 3)
	MDRV_SOUND_CONFIG(custom_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


ROM_START( redearth )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "redearth_euro.29f400.u2", 0x000000, 0x080000, CRC(02e0f336) SHA1(acc37e830dfeb9674f5a0fb24f4cc23217ae4ff5) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "redearth-simm1.0", 0x0000000, 0x200000, CRC(cad468f8) SHA1(b3aa4f7d3fae84e8821417ccde9528d3eda2b7a6) )
	ROM_LOAD32_BYTE( "redearth-simm1.1", 0x0000001, 0x200000, CRC(e9721d89) SHA1(5c63d10bdbce52d50b6dde14d4a0f1369383d656) )
	ROM_LOAD32_BYTE( "redearth-simm1.2", 0x0000002, 0x200000, CRC(2889ec98) SHA1(a94310eb4777f908d87e9d90969db8504b4140ff) )
	ROM_LOAD32_BYTE( "redearth-simm1.3", 0x0000003, 0x200000, CRC(5a6cd148) SHA1(d65c6e8378a91828474a16a3bbcd13c4b3b15f13) )

	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "redearth-simm3.0", 0x0000000, 0x200000, CRC(83350cc5) SHA1(922b1abf80a4a89f35279b66311a7369d3965bd0) )
	ROM_LOAD16_BYTE( "redearth-simm3.1", 0x0000001, 0x200000, CRC(56734de6) SHA1(75699fa6efe5bec335e4b02e15b3c45726b68fa8) )
	ROM_LOAD16_BYTE( "redearth-simm3.2", 0x0400000, 0x200000, CRC(800ea0f1) SHA1(33871ab56dc1cd24441389d53e43fb8e43b149d9) )
	ROM_LOAD16_BYTE( "redearth-simm3.3", 0x0400001, 0x200000, CRC(97e9146c) SHA1(ab7744709615081440bee72f4080d6fd5b938668) )
	ROM_LOAD16_BYTE( "redearth-simm3.4", 0x0800000, 0x200000, CRC(0cb1d648) SHA1(7042a590c2b7ec55323062127e254da3cdc790a1) )
	ROM_LOAD16_BYTE( "redearth-simm3.5", 0x0800001, 0x200000, CRC(7a1099f0) SHA1(c6a92ec86eb24485f1db530e0e78f647e8432231) )
	ROM_LOAD16_BYTE( "redearth-simm3.6", 0x0c00000, 0x200000, CRC(aeff8f54) SHA1(fd760e237c2e5fb2da45e32a1c12fd3defb4c3e4) )
	ROM_LOAD16_BYTE( "redearth-simm3.7", 0x0c00001, 0x200000, CRC(f770acd0) SHA1(4b3ccb6f91568f95f04ede6c574144918d131201) )
	ROM_LOAD16_BYTE( "redearth-simm4.0", 0x1000000, 0x200000, CRC(301e56f2) SHA1(4847d971bff70a2aeed4599e1201c7ec9677da60) )
	ROM_LOAD16_BYTE( "redearth-simm4.1", 0x1000001, 0x200000, CRC(2048e103) SHA1(b21f95b05cd99749bd3f25cc71b2671c2026847b) )
	ROM_LOAD16_BYTE( "redearth-simm4.2", 0x1400000, 0x200000, CRC(c9433455) SHA1(63a269d76bac332c2e991d0f6a20c35e0e88680a) )
	ROM_LOAD16_BYTE( "redearth-simm4.3", 0x1400001, 0x200000, CRC(c02171a8) SHA1(2e9228729b27a6113d9f2e42af310a834979f714) )
	ROM_LOAD16_BYTE( "redearth-simm4.4", 0x1800000, 0x200000, CRC(2ddbf276) SHA1(b232baaa8edc8db18f8a3bdcc2d38fe984a94a34) )
	ROM_LOAD16_BYTE( "redearth-simm4.5", 0x1800001, 0x200000, CRC(fea820a6) SHA1(55ee8ef95751f5a509fb126513e1b2a70a3414e5) )
	ROM_LOAD16_BYTE( "redearth-simm4.6", 0x1c00000, 0x200000, CRC(c7528df1) SHA1(aa312f80c2d7759d18d1aa8d416cf932b2850824) )
	ROM_LOAD16_BYTE( "redearth-simm4.7", 0x1c00001, 0x200000, CRC(2449cf3b) SHA1(c60d8042136d74e547f668ad787cae529c42eed9) )
	ROM_LOAD16_BYTE( "redearth-simm5.0", 0x2000000, 0x200000, CRC(424451b9) SHA1(250fb92254c9e7ff5bc8dbeea5872f8a771dc9bd) )
	ROM_LOAD16_BYTE( "redearth-simm5.1", 0x2000001, 0x200000, CRC(9b8cb56b) SHA1(2ff1081dc99bb7c2f1e036f4c112137c96b83d23) )
ROM_END

ROM_START( warzard )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "warzard_japan.29f400.u2", 0x000000, 0x080000, CRC(f8e2f0c6) SHA1(93d6a986f44c211fff014e55681eca4d2a2774d6) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "redearth-simm1.0", 0x0000000, 0x200000, CRC(cad468f8) SHA1(b3aa4f7d3fae84e8821417ccde9528d3eda2b7a6) )
	ROM_LOAD32_BYTE( "redearth-simm1.1", 0x0000001, 0x200000, CRC(e9721d89) SHA1(5c63d10bdbce52d50b6dde14d4a0f1369383d656) )
	ROM_LOAD32_BYTE( "redearth-simm1.2", 0x0000002, 0x200000, CRC(2889ec98) SHA1(a94310eb4777f908d87e9d90969db8504b4140ff) )
	ROM_LOAD32_BYTE( "redearth-simm1.3", 0x0000003, 0x200000, CRC(5a6cd148) SHA1(d65c6e8378a91828474a16a3bbcd13c4b3b15f13) )

	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "redearth-simm3.0", 0x0000000, 0x200000, CRC(83350cc5) SHA1(922b1abf80a4a89f35279b66311a7369d3965bd0) )
	ROM_LOAD16_BYTE( "redearth-simm3.1", 0x0000001, 0x200000, CRC(56734de6) SHA1(75699fa6efe5bec335e4b02e15b3c45726b68fa8) )
	ROM_LOAD16_BYTE( "redearth-simm3.2", 0x0400000, 0x200000, CRC(800ea0f1) SHA1(33871ab56dc1cd24441389d53e43fb8e43b149d9) )
	ROM_LOAD16_BYTE( "redearth-simm3.3", 0x0400001, 0x200000, CRC(97e9146c) SHA1(ab7744709615081440bee72f4080d6fd5b938668) )
	ROM_LOAD16_BYTE( "redearth-simm3.4", 0x0800000, 0x200000, CRC(0cb1d648) SHA1(7042a590c2b7ec55323062127e254da3cdc790a1) )
	ROM_LOAD16_BYTE( "redearth-simm3.5", 0x0800001, 0x200000, CRC(7a1099f0) SHA1(c6a92ec86eb24485f1db530e0e78f647e8432231) )
	ROM_LOAD16_BYTE( "redearth-simm3.6", 0x0c00000, 0x200000, CRC(aeff8f54) SHA1(fd760e237c2e5fb2da45e32a1c12fd3defb4c3e4) )
	ROM_LOAD16_BYTE( "redearth-simm3.7", 0x0c00001, 0x200000, CRC(f770acd0) SHA1(4b3ccb6f91568f95f04ede6c574144918d131201) )
	ROM_LOAD16_BYTE( "redearth-simm4.0", 0x1000000, 0x200000, CRC(301e56f2) SHA1(4847d971bff70a2aeed4599e1201c7ec9677da60) )
	ROM_LOAD16_BYTE( "redearth-simm4.1", 0x1000001, 0x200000, CRC(2048e103) SHA1(b21f95b05cd99749bd3f25cc71b2671c2026847b) )
	ROM_LOAD16_BYTE( "redearth-simm4.2", 0x1400000, 0x200000, CRC(c9433455) SHA1(63a269d76bac332c2e991d0f6a20c35e0e88680a) )
	ROM_LOAD16_BYTE( "redearth-simm4.3", 0x1400001, 0x200000, CRC(c02171a8) SHA1(2e9228729b27a6113d9f2e42af310a834979f714) )
	ROM_LOAD16_BYTE( "redearth-simm4.4", 0x1800000, 0x200000, CRC(2ddbf276) SHA1(b232baaa8edc8db18f8a3bdcc2d38fe984a94a34) )
	ROM_LOAD16_BYTE( "redearth-simm4.5", 0x1800001, 0x200000, CRC(fea820a6) SHA1(55ee8ef95751f5a509fb126513e1b2a70a3414e5) )
	ROM_LOAD16_BYTE( "redearth-simm4.6", 0x1c00000, 0x200000, CRC(c7528df1) SHA1(aa312f80c2d7759d18d1aa8d416cf932b2850824) )
	ROM_LOAD16_BYTE( "redearth-simm4.7", 0x1c00001, 0x200000, CRC(2449cf3b) SHA1(c60d8042136d74e547f668ad787cae529c42eed9) )
	ROM_LOAD16_BYTE( "redearth-simm5.0", 0x2000000, 0x200000, CRC(424451b9) SHA1(250fb92254c9e7ff5bc8dbeea5872f8a771dc9bd) )
	ROM_LOAD16_BYTE( "redearth-simm5.1", 0x2000001, 0x200000, CRC(9b8cb56b) SHA1(2ff1081dc99bb7c2f1e036f4c112137c96b83d23) )
ROM_END

ROM_START( sfiii )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii_euro.29f400.u2", 0x000000, 0x080000, CRC(27699ddc) SHA1(d8b525cd27e584560b129598df31fd2c5b2a682a) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "sfiii-simm1.0", 0x0000000, 0x200000, CRC(cfc9e45a) SHA1(5d9061f76680642e730373e3ac29b24926dc5c0c) )
	ROM_LOAD32_BYTE( "sfiii-simm1.1", 0x0000001, 0x200000, CRC(57920546) SHA1(c8452e7e101b8888fb806d1c9874c6be49fc7dbd) )
	ROM_LOAD32_BYTE( "sfiii-simm1.2", 0x0000002, 0x200000, CRC(0d8f2680) SHA1(ade7b28acd11023696c4b20136f3d2f34da6b1be) )
	ROM_LOAD32_BYTE( "sfiii-simm1.3", 0x0000003, 0x200000, CRC(ea4ca054) SHA1(f91c55c4e4fc428ce15d27be38aeed3a483d028c) )

	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii-simm3.0", 0x0000000, 0x200000, CRC(080b3bd3) SHA1(f51bc5de95ab22b87ba09ea721285b308afd0bda) )
	ROM_LOAD16_BYTE( "sfiii-simm3.1", 0x0000001, 0x200000, CRC(5c356f2f) SHA1(e969ce388f6e565d9612e65b0895560c7bb472e6) )
	ROM_LOAD16_BYTE( "sfiii-simm3.2", 0x0400000, 0x200000, CRC(f9c97a45) SHA1(58a9691696c3f26a1150a451567c501f55cf1874) )
	ROM_LOAD16_BYTE( "sfiii-simm3.3", 0x0400001, 0x200000, CRC(09de3ead) SHA1(2f41d84a96cb5e0d169200a4e9358ad5f407a2b7) )
	ROM_LOAD16_BYTE( "sfiii-simm3.4", 0x0800000, 0x200000, CRC(7dd7e1f3) SHA1(bcf1023287457d97f09d9f6e9c93fdf24cc24a07) )
	ROM_LOAD16_BYTE( "sfiii-simm3.5", 0x0800001, 0x200000, CRC(47a03a3a) SHA1(2509e5737059251888e4e1efbcdfac86a89ff1a1) )
	ROM_LOAD16_BYTE( "sfiii-simm3.6", 0x0c00000, 0x200000, CRC(e9eb7a26) SHA1(b8547edb7085e9149aa59d5226ad2d1976cab2bd) )
	ROM_LOAD16_BYTE( "sfiii-simm3.7", 0x0c00001, 0x200000, CRC(7f44395c) SHA1(f4d2e283cb3a4aad4eae4e13963a74e20be7c181) )
	ROM_LOAD16_BYTE( "sfiii-simm4.0", 0x1000000, 0x200000, CRC(9ac080fc) SHA1(2e5024b35b147513ee42eda8748df9d669410377) )
	ROM_LOAD16_BYTE( "sfiii-simm4.1", 0x1000001, 0x200000, CRC(6e2c4c94) SHA1(5a185cb76b5999bd826bc9b5ea584a5c3498f69d) )
	ROM_LOAD16_BYTE( "sfiii-simm4.2", 0x1400000, 0x200000, CRC(8afc22d4) SHA1(04a419a3092c98fc4a7693e6acf30ae5a849e5c1) )
	ROM_LOAD16_BYTE( "sfiii-simm4.3", 0x1400001, 0x200000, CRC(9f3873b8) SHA1(33499d6f02bc84c80acb56be078aaed7f8d1300d) )
	ROM_LOAD16_BYTE( "sfiii-simm4.4", 0x1800000, 0x200000, CRC(166b3c97) SHA1(40e6e9d43cbbd8496b430931b8ab7db01dc1c6d5) )
	ROM_LOAD16_BYTE( "sfiii-simm4.5", 0x1800001, 0x200000, CRC(e5ea2547) SHA1(a823c689098f37a3054d728bddb0033a4b8396f1) )
	ROM_LOAD16_BYTE( "sfiii-simm4.6", 0x1c00000, 0x200000, CRC(e85b9fdd) SHA1(264cb10fe9b3ede384c7db42bfc58ed5c21ea8f8) )
	ROM_LOAD16_BYTE( "sfiii-simm4.7", 0x1c00001, 0x200000, CRC(362c01b7) SHA1(9c404312a6aabe8e91e68dde193e3972bc1636cd) )
	ROM_LOAD16_BYTE( "sfiii-simm5.0", 0x2000000, 0x200000, CRC(9bc108b2) SHA1(894dadab7957044bf877029c7f8e556d5d6e85d3) )
	ROM_LOAD16_BYTE( "sfiii-simm5.1", 0x2000001, 0x200000, CRC(c6f1c066) SHA1(00de492dd1ef7aef05027a8c501c296b6602e917) )
ROM_END

ROM_START( sfiiij )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii_japan.29f400.u2", 0x000000, 0x080000, CRC(74205250) SHA1(c3e83ace7121d32da729162662ec6b5285a31211) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "sfiii-simm1.0", 0x0000000, 0x200000, CRC(cfc9e45a) SHA1(5d9061f76680642e730373e3ac29b24926dc5c0c) )
	ROM_LOAD32_BYTE( "sfiii-simm1.1", 0x0000001, 0x200000, CRC(57920546) SHA1(c8452e7e101b8888fb806d1c9874c6be49fc7dbd) )
	ROM_LOAD32_BYTE( "sfiii-simm1.2", 0x0000002, 0x200000, CRC(0d8f2680) SHA1(ade7b28acd11023696c4b20136f3d2f34da6b1be) )
	ROM_LOAD32_BYTE( "sfiii-simm1.3", 0x0000003, 0x200000, CRC(ea4ca054) SHA1(f91c55c4e4fc428ce15d27be38aeed3a483d028c) )

	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii-simm3.0", 0x0000000, 0x200000, CRC(080b3bd3) SHA1(f51bc5de95ab22b87ba09ea721285b308afd0bda) )
	ROM_LOAD16_BYTE( "sfiii-simm3.1", 0x0000001, 0x200000, CRC(5c356f2f) SHA1(e969ce388f6e565d9612e65b0895560c7bb472e6) )
	ROM_LOAD16_BYTE( "sfiii-simm3.2", 0x0400000, 0x200000, CRC(f9c97a45) SHA1(58a9691696c3f26a1150a451567c501f55cf1874) )
	ROM_LOAD16_BYTE( "sfiii-simm3.3", 0x0400001, 0x200000, CRC(09de3ead) SHA1(2f41d84a96cb5e0d169200a4e9358ad5f407a2b7) )
	ROM_LOAD16_BYTE( "sfiii-simm3.4", 0x0800000, 0x200000, CRC(7dd7e1f3) SHA1(bcf1023287457d97f09d9f6e9c93fdf24cc24a07) )
	ROM_LOAD16_BYTE( "sfiii-simm3.5", 0x0800001, 0x200000, CRC(47a03a3a) SHA1(2509e5737059251888e4e1efbcdfac86a89ff1a1) )
	ROM_LOAD16_BYTE( "sfiii-simm3.6", 0x0c00000, 0x200000, CRC(e9eb7a26) SHA1(b8547edb7085e9149aa59d5226ad2d1976cab2bd) )
	ROM_LOAD16_BYTE( "sfiii-simm3.7", 0x0c00001, 0x200000, CRC(7f44395c) SHA1(f4d2e283cb3a4aad4eae4e13963a74e20be7c181) )
	ROM_LOAD16_BYTE( "sfiii-simm4.0", 0x1000000, 0x200000, CRC(9ac080fc) SHA1(2e5024b35b147513ee42eda8748df9d669410377) )
	ROM_LOAD16_BYTE( "sfiii-simm4.1", 0x1000001, 0x200000, CRC(6e2c4c94) SHA1(5a185cb76b5999bd826bc9b5ea584a5c3498f69d) )
	ROM_LOAD16_BYTE( "sfiii-simm4.2", 0x1400000, 0x200000, CRC(8afc22d4) SHA1(04a419a3092c98fc4a7693e6acf30ae5a849e5c1) )
	ROM_LOAD16_BYTE( "sfiii-simm4.3", 0x1400001, 0x200000, CRC(9f3873b8) SHA1(33499d6f02bc84c80acb56be078aaed7f8d1300d) )
	ROM_LOAD16_BYTE( "sfiii-simm4.4", 0x1800000, 0x200000, CRC(166b3c97) SHA1(40e6e9d43cbbd8496b430931b8ab7db01dc1c6d5) )
	ROM_LOAD16_BYTE( "sfiii-simm4.5", 0x1800001, 0x200000, CRC(e5ea2547) SHA1(a823c689098f37a3054d728bddb0033a4b8396f1) )
	ROM_LOAD16_BYTE( "sfiii-simm4.6", 0x1c00000, 0x200000, CRC(e85b9fdd) SHA1(264cb10fe9b3ede384c7db42bfc58ed5c21ea8f8) )
	ROM_LOAD16_BYTE( "sfiii-simm4.7", 0x1c00001, 0x200000, CRC(362c01b7) SHA1(9c404312a6aabe8e91e68dde193e3972bc1636cd) )
	ROM_LOAD16_BYTE( "sfiii-simm5.0", 0x2000000, 0x200000, CRC(9bc108b2) SHA1(894dadab7957044bf877029c7f8e556d5d6e85d3) )
	ROM_LOAD16_BYTE( "sfiii-simm5.1", 0x2000001, 0x200000, CRC(c6f1c066) SHA1(00de492dd1ef7aef05027a8c501c296b6602e917) )
ROM_END

ROM_START( sfiii2 )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii2_usa.29f400.u2", 0x000000, 0x080000, CRC(75dd72e0) SHA1(5a12d6ea6734df5de00ecee6f9ef470749d2f242) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "sfiii2-simm1.0", 0x0000000, 0x200000, CRC(2d666f0b) SHA1(68de034b3a3aeaf4b26122a84ad48b0b763e4122) )
	ROM_LOAD32_BYTE( "sfiii2-simm1.1", 0x0000001, 0x200000, CRC(2a3a8ef6) SHA1(31fb58fd1360ed8c951e2c4ac898a5a7104528d6) )
	ROM_LOAD32_BYTE( "sfiii2-simm1.2", 0x0000002, 0x200000, CRC(161d2206) SHA1(58999f876e64c1a088e8765962a9cd504f22a706) )
	ROM_LOAD32_BYTE( "sfiii2-simm1.3", 0x0000003, 0x200000, CRC(87ded8a3) SHA1(4ccef64f80d2ee63940b0958b500364ee515db51) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.0", 0x0800000, 0x200000, CRC(94a4ce0f) SHA1(2c8e26a66d1dcd17c22c70baa2a3ff5a54511514) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.1", 0x0800001, 0x200000, CRC(67585033) SHA1(24df9968a54c330fbe95f8e4dfe6e7dfd144ed0c) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.2", 0x0800002, 0x200000, CRC(fabffcd5) SHA1(9399f64c42f63a64e44a21a2690e44779943a2b2) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.3", 0x0800003, 0x200000, CRC(623c09ca) SHA1(dc9618a08bb7f44e569ac17605d268511155a14e) )

	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii2-simm3.0", 0x0000000, 0x200000, CRC(dab2d766) SHA1(d265cc8b1b497eb4bedd63b3f1de60eb1c1db0df) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.1", 0x0000001, 0x200000, CRC(1f2aa34b) SHA1(38b224d34c4550f1f33c2c368e2a252d0d176cc0) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.2", 0x0400000, 0x200000, CRC(6f1a04eb) SHA1(980ca929114075d1920e2da44f9a22087cc92e55) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.3", 0x0400001, 0x200000, CRC(e05ef205) SHA1(e604e3832549740f953581fc91e850beda6a73c8) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.4", 0x0800000, 0x200000, CRC(affb074f) SHA1(0e76973807039bc66fd0f3233401cea8d2c45f84) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.5", 0x0800001, 0x200000, CRC(6962872e) SHA1(f16b2d0792697345145d0e9d950e912a2ffabe0d) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.6", 0x0c00000, 0x200000, CRC(6eed87de) SHA1(5d5067ad36234c5efd57a2baebeffa2f44f2caec) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.7", 0x0c00001, 0x200000, CRC(e18f479e) SHA1(cd4c1812ab422336bfa414e0b2098b472d2f9251) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.0", 0x1000000, 0x200000, CRC(764c2503) SHA1(cad3f20ade2e1d3ac52f8c318443da20062ae943) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.1", 0x1000001, 0x200000, CRC(3e16af6e) SHA1(afde2ed4bf3a3e95035fc02c572c5b83178a9467) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.2", 0x1400000, 0x200000, CRC(215705e6) SHA1(42d3849f8a9242a89ba465dbc205f310186c67cd) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.3", 0x1400001, 0x200000, CRC(e30cbd9c) SHA1(c205101ada86154921e09fed4f6908d15ec60761) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.4", 0x1800000, 0x200000, CRC(4185ded9) SHA1(24bf9b5f25d7753f1feb09b82611f7482f30d304) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.5", 0x1800001, 0x200000, CRC(4e8db013) SHA1(6816df2b6c60005fb375530ea93bb30a960c9b01) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.6", 0x1c00000, 0x200000, CRC(08df48ce) SHA1(e8a3b68ebeab193539446c3f6e0a19b37f1f3495) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.7", 0x1c00001, 0x200000, CRC(bb8f80a5) SHA1(35d9e86637d54405c97fdb7da9c42cc53907cae3) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.0", 0x2000000, 0x200000, CRC(ebdc4787) SHA1(f86e8ebf4b2214be166dbe4ea921058a552364ea) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.1", 0x2000001, 0x200000, CRC(6b7c550e) SHA1(77cdabccf3ecebf142ac86dffe6e24052941e3a1) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.2", 0x2400000, 0x200000, CRC(56ff8c50) SHA1(16f7602a4549a5b724e3fcdb75b0f3c397077b81) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.3", 0x2400001, 0x200000, CRC(3f2ac3e9) SHA1(a7b631f18ce572a42f46314f37a01d9840abc765) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.4", 0x2800000, 0x200000, CRC(48cda50e) SHA1(35e9f27fb8b69e3b3a313ea33dc53b1102e5f66e) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.5", 0x2800001, 0x200000, CRC(520c0af6) SHA1(7bed1b6707974eafbfb62ccb84a51df8a100e070) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.6", 0x2c00000, 0x200000, CRC(2edc5986) SHA1(761ab2c67d0d873ffd74158eb77f7722c076f3e3) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.7", 0x2c00001, 0x200000, CRC(93ffa199) SHA1(33ec2379f30c6fdf47ba72c1d0cad8bdd02f17df) )
ROM_END

ROM_START( sfiii2j )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii2_japan.29f400.u2", 0x000000, 0x080000, CRC(faea0a3e) SHA1(a03cd63bcf52e4d57f7a598c8bc8e243694624ec) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "sfiii2-simm1.0", 0x0000000, 0x200000, CRC(2d666f0b) SHA1(68de034b3a3aeaf4b26122a84ad48b0b763e4122) )
	ROM_LOAD32_BYTE( "sfiii2-simm1.1", 0x0000001, 0x200000, CRC(2a3a8ef6) SHA1(31fb58fd1360ed8c951e2c4ac898a5a7104528d6) )
	ROM_LOAD32_BYTE( "sfiii2-simm1.2", 0x0000002, 0x200000, CRC(161d2206) SHA1(58999f876e64c1a088e8765962a9cd504f22a706) )
	ROM_LOAD32_BYTE( "sfiii2-simm1.3", 0x0000003, 0x200000, CRC(87ded8a3) SHA1(4ccef64f80d2ee63940b0958b500364ee515db51) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.0", 0x0800000, 0x200000, CRC(94a4ce0f) SHA1(2c8e26a66d1dcd17c22c70baa2a3ff5a54511514) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.1", 0x0800001, 0x200000, CRC(67585033) SHA1(24df9968a54c330fbe95f8e4dfe6e7dfd144ed0c) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.2", 0x0800002, 0x200000, CRC(fabffcd5) SHA1(9399f64c42f63a64e44a21a2690e44779943a2b2) )
	ROM_LOAD32_BYTE( "sfiii2-simm2.3", 0x0800003, 0x200000, CRC(623c09ca) SHA1(dc9618a08bb7f44e569ac17605d268511155a14e) )

	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii2-simm3.0", 0x0000000, 0x200000, CRC(dab2d766) SHA1(d265cc8b1b497eb4bedd63b3f1de60eb1c1db0df) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.1", 0x0000001, 0x200000, CRC(1f2aa34b) SHA1(38b224d34c4550f1f33c2c368e2a252d0d176cc0) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.2", 0x0400000, 0x200000, CRC(6f1a04eb) SHA1(980ca929114075d1920e2da44f9a22087cc92e55) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.3", 0x0400001, 0x200000, CRC(e05ef205) SHA1(e604e3832549740f953581fc91e850beda6a73c8) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.4", 0x0800000, 0x200000, CRC(affb074f) SHA1(0e76973807039bc66fd0f3233401cea8d2c45f84) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.5", 0x0800001, 0x200000, CRC(6962872e) SHA1(f16b2d0792697345145d0e9d950e912a2ffabe0d) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.6", 0x0c00000, 0x200000, CRC(6eed87de) SHA1(5d5067ad36234c5efd57a2baebeffa2f44f2caec) )
	ROM_LOAD16_BYTE( "sfiii2-simm3.7", 0x0c00001, 0x200000, CRC(e18f479e) SHA1(cd4c1812ab422336bfa414e0b2098b472d2f9251) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.0", 0x1000000, 0x200000, CRC(764c2503) SHA1(cad3f20ade2e1d3ac52f8c318443da20062ae943) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.1", 0x1000001, 0x200000, CRC(3e16af6e) SHA1(afde2ed4bf3a3e95035fc02c572c5b83178a9467) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.2", 0x1400000, 0x200000, CRC(215705e6) SHA1(42d3849f8a9242a89ba465dbc205f310186c67cd) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.3", 0x1400001, 0x200000, CRC(e30cbd9c) SHA1(c205101ada86154921e09fed4f6908d15ec60761) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.4", 0x1800000, 0x200000, CRC(4185ded9) SHA1(24bf9b5f25d7753f1feb09b82611f7482f30d304) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.5", 0x1800001, 0x200000, CRC(4e8db013) SHA1(6816df2b6c60005fb375530ea93bb30a960c9b01) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.6", 0x1c00000, 0x200000, CRC(08df48ce) SHA1(e8a3b68ebeab193539446c3f6e0a19b37f1f3495) )
	ROM_LOAD16_BYTE( "sfiii2-simm4.7", 0x1c00001, 0x200000, CRC(bb8f80a5) SHA1(35d9e86637d54405c97fdb7da9c42cc53907cae3) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.0", 0x2000000, 0x200000, CRC(ebdc4787) SHA1(f86e8ebf4b2214be166dbe4ea921058a552364ea) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.1", 0x2000001, 0x200000, CRC(6b7c550e) SHA1(77cdabccf3ecebf142ac86dffe6e24052941e3a1) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.2", 0x2400000, 0x200000, CRC(56ff8c50) SHA1(16f7602a4549a5b724e3fcdb75b0f3c397077b81) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.3", 0x2400001, 0x200000, CRC(3f2ac3e9) SHA1(a7b631f18ce572a42f46314f37a01d9840abc765) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.4", 0x2800000, 0x200000, CRC(48cda50e) SHA1(35e9f27fb8b69e3b3a313ea33dc53b1102e5f66e) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.5", 0x2800001, 0x200000, CRC(520c0af6) SHA1(7bed1b6707974eafbfb62ccb84a51df8a100e070) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.6", 0x2c00000, 0x200000, CRC(2edc5986) SHA1(761ab2c67d0d873ffd74158eb77f7722c076f3e3) )
	ROM_LOAD16_BYTE( "sfiii2-simm5.7", 0x2c00001, 0x200000, CRC(93ffa199) SHA1(33ec2379f30c6fdf47ba72c1d0cad8bdd02f17df) )
ROM_END

ROM_START( jojo )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojo_euro.29f400.u2", 0x000000, 0x080000, CRC(513e40ec) SHA1(03b91f0fbd5be56d24feed4698c7543d4df07837) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "jojo-simm1.0", 0x0000000, 0x200000, CRC(9516948b) SHA1(4d7e6c1eb7d1bebff2a5069bcd186070a9105474) )
	ROM_LOAD32_BYTE( "jojo-simm1.1", 0x0000001, 0x200000, CRC(a847848d) SHA1(4df70309395f1d2a2e8f85bc34e17453d4a76f81) )
	ROM_LOAD32_BYTE( "jojo-simm1.2", 0x0000002, 0x200000, CRC(853e8846) SHA1(d120b7e2de9502e3261e8dd101f97589b2ed1c38) )
	ROM_LOAD32_BYTE( "jojo-simm1.3", 0x0000003, 0x200000, CRC(c04fe00e) SHA1(d09409b77460d19b56aaaf4a64356f3d37a1ee41) )
	ROM_LOAD32_BYTE( "jojo-simm2.0", 0x0800000, 0x200000, CRC(e1a4b3c8) SHA1(5dc298431644e1ca470aaab752a7d74f2f9dc7a1) )
	ROM_LOAD32_BYTE( "jojo-simm2.1", 0x0800001, 0x200000, CRC(189cef95) SHA1(ebe42a019358461557f69fb17d65d84d0f733415) )
	ROM_LOAD32_BYTE( "jojo-simm2.2", 0x0800002, 0x200000, CRC(47db5ec6) SHA1(e80271e4013e4391c2cc4229ff1fbd4a2b7c6f04) )
	ROM_LOAD32_BYTE( "jojo-simm2.3", 0x0800003, 0x200000, CRC(e3d3a155) SHA1(75e9b5da93dd8894cf70fa4dac56f3958be4c766) )

	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "jojo-simm3.0", 0x0000000, 0x200000, CRC(de7fc9c1) SHA1(662b85a990b04c855773506c936317e62fab4a05) )
	ROM_LOAD16_BYTE( "jojo-simm3.1", 0x0000001, 0x200000, CRC(43d053d3) SHA1(54ff0e9c164e0d1649522c330ccc7e5d79e0bc85) )
	ROM_LOAD16_BYTE( "jojo-simm3.2", 0x0400000, 0x200000, CRC(2ffd7fa5) SHA1(9018c8e2b286a333ba606208e90caa764951ea3f) )
	ROM_LOAD16_BYTE( "jojo-simm3.3", 0x0400001, 0x200000, CRC(4da4985b) SHA1(2552b1730a21ce17d58b69a79ad212a6a5829439) )
	ROM_LOAD16_BYTE( "jojo-simm3.4", 0x0800000, 0x200000, CRC(fde98d72) SHA1(654563e12d033e8656dc74a268a08b15b171470d) )
	ROM_LOAD16_BYTE( "jojo-simm3.5", 0x0800001, 0x200000, CRC(edb2a266) SHA1(19ebada8422c7f4bf70d0c9ad42b84268967b316) )
	ROM_LOAD16_BYTE( "jojo-simm3.6", 0x0c00000, 0x200000, CRC(be7cf319) SHA1(7893f5907992e6b903b2683980bba6d3d003bb06) )
	ROM_LOAD16_BYTE( "jojo-simm3.7", 0x0c00001, 0x200000, CRC(56fe1a9f) SHA1(01741fe1256f4e682f687e94040f4e8bbb8bedb2) )
	ROM_LOAD16_BYTE( "jojo-simm4.0", 0x1000000, 0x200000, CRC(c4e7bf68) SHA1(a4d1ddea58a3d42db82a63a5e974cbf38d9b792a) )
	ROM_LOAD16_BYTE( "jojo-simm4.1", 0x1000001, 0x200000, CRC(b62b2719) SHA1(cb577b89e9e14fda67715716fefd47a782d518ab) )
	ROM_LOAD16_BYTE( "jojo-simm4.2", 0x1400000, 0x200000, CRC(18d15809) SHA1(2b406cd1aaa4799a436213dcaa65473eacb4c6d7) )
	ROM_LOAD16_BYTE( "jojo-simm4.3", 0x1400001, 0x200000, CRC(9af0ad79) SHA1(075ee048e17b50188876f25d7a6571d6ace84d7d) )
	ROM_LOAD16_BYTE( "jojo-simm4.4", 0x1800000, 0x200000, CRC(4124c1f0) SHA1(e4946a8029adc5d0bacead8d766521b4ccd1722b) )
	ROM_LOAD16_BYTE( "jojo-simm4.5", 0x1800001, 0x200000, CRC(5e001fd1) SHA1(6457a39f336381b46e587aa2f5f719810ee5bcf9) )
	ROM_LOAD16_BYTE( "jojo-simm4.6", 0x1c00000, 0x200000, CRC(9affa23b) SHA1(e3d77e777c47277d841a9dadc1dd6e3157706a2e) )
	ROM_LOAD16_BYTE( "jojo-simm4.7", 0x1c00001, 0x200000, CRC(2511572a) SHA1(725adcf71bcee5c8bb839d2d1c5e3456b8c6886b) )
	ROM_LOAD16_BYTE( "jojo-simm5.0", 0x2000000, 0x200000, CRC(797615fc) SHA1(29874be9f1da5515c90f5d601aa5924c263f8feb) )
	ROM_LOAD16_BYTE( "jojo-simm5.1", 0x2000001, 0x200000, CRC(734fd162) SHA1(16cdfac74d18a6c2216afb1ce6afbd7f15297c32) )
ROM_END

ROM_START( jojojr2 )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojo_japan.29f400.u2", 0x000000, 0x080000, CRC(02778f60) SHA1(a167f9ebe030592a0cdb0c6a3c75835c6a43be4c) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "jojo-simm1.0", 0x0000000, 0x200000, CRC(e06ba886) SHA1(4defd5e8e1e6d0c439fed8a6454e89a59e24ea4c) )
	ROM_LOAD32_BYTE( "jojo-simm1.1", 0x0000001, 0x200000, CRC(6dd177c8) SHA1(c39db980f6fcca9c221e9be6f777eaf38f1b136b) )
	ROM_LOAD32_BYTE( "jojo-simm1.2", 0x0000002, 0x200000, CRC(d35a15e0) SHA1(576b92a94505764a10b9bcf82c02335e7ef62014) )
	ROM_LOAD32_BYTE( "jojo-simm1.3", 0x0000003, 0x200000, CRC(66d865ac) SHA1(5248c3f124af62b4a672d954ef15f86629feeacb) )
	ROM_LOAD32_BYTE( "jojo-simm2.0", 0x0800000, 0x200000, CRC(417e5dc1) SHA1(54ee9596c1c51811f3bdef7dbe77b44b34f230ca) )
	ROM_LOAD32_BYTE( "jojo-simm2.1", 0x0800001, 0x200000, CRC(c891c887) SHA1(42e84f774ee655e9a39b016a3cfe94262ed2e9f1) )
	ROM_LOAD32_BYTE( "jojo-simm2.2", 0x0800002, 0x200000, CRC(1e101f30) SHA1(56518c1646bb9452334856bb8bcc58892f9f93b9) )
	ROM_LOAD32_BYTE( "jojo-simm2.3", 0x0800003, 0x200000, CRC(1fd1d3e4) SHA1(bed2b77d58f1fdf7ba5ca7126d3db1dd0f8c80b4) )

	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "jojo-simm3.0", 0x0000000, 0x200000, CRC(de7fc9c1) SHA1(662b85a990b04c855773506c936317e62fab4a05) )
	ROM_LOAD16_BYTE( "jojo-simm3.1", 0x0000001, 0x200000, CRC(43d053d3) SHA1(54ff0e9c164e0d1649522c330ccc7e5d79e0bc85) )
	ROM_LOAD16_BYTE( "jojo-simm3.2", 0x0400000, 0x200000, CRC(2ffd7fa5) SHA1(9018c8e2b286a333ba606208e90caa764951ea3f) )
	ROM_LOAD16_BYTE( "jojo-simm3.3", 0x0400001, 0x200000, CRC(4da4985b) SHA1(2552b1730a21ce17d58b69a79ad212a6a5829439) )
	ROM_LOAD16_BYTE( "jojo-simm3.4", 0x0800000, 0x200000, CRC(fde98d72) SHA1(654563e12d033e8656dc74a268a08b15b171470d) )
	ROM_LOAD16_BYTE( "jojo-simm3.5", 0x0800001, 0x200000, CRC(edb2a266) SHA1(19ebada8422c7f4bf70d0c9ad42b84268967b316) )
	ROM_LOAD16_BYTE( "jojo-simm3.6", 0x0c00000, 0x200000, CRC(be7cf319) SHA1(7893f5907992e6b903b2683980bba6d3d003bb06) )
	ROM_LOAD16_BYTE( "jojo-simm3.7", 0x0c00001, 0x200000, CRC(56fe1a9f) SHA1(01741fe1256f4e682f687e94040f4e8bbb8bedb2) )
	ROM_LOAD16_BYTE( "jojo-simm4.0", 0x1000000, 0x200000, CRC(c4e7bf68) SHA1(a4d1ddea58a3d42db82a63a5e974cbf38d9b792a) )
	ROM_LOAD16_BYTE( "jojo-simm4.1", 0x1000001, 0x200000, CRC(b62b2719) SHA1(cb577b89e9e14fda67715716fefd47a782d518ab) )
	ROM_LOAD16_BYTE( "jojo-simm4.2", 0x1400000, 0x200000, CRC(18d15809) SHA1(2b406cd1aaa4799a436213dcaa65473eacb4c6d7) )
	ROM_LOAD16_BYTE( "jojo-simm4.3", 0x1400001, 0x200000, CRC(9af0ad79) SHA1(075ee048e17b50188876f25d7a6571d6ace84d7d) )
	ROM_LOAD16_BYTE( "jojo-simm4.4", 0x1800000, 0x200000, CRC(4124c1f0) SHA1(e4946a8029adc5d0bacead8d766521b4ccd1722b) )
	ROM_LOAD16_BYTE( "jojo-simm4.5", 0x1800001, 0x200000, CRC(5e001fd1) SHA1(6457a39f336381b46e587aa2f5f719810ee5bcf9) )
	ROM_LOAD16_BYTE( "jojo-simm4.6", 0x1c00000, 0x200000, CRC(9affa23b) SHA1(e3d77e777c47277d841a9dadc1dd6e3157706a2e) )
	ROM_LOAD16_BYTE( "jojo-simm4.7", 0x1c00001, 0x200000, CRC(2511572a) SHA1(725adcf71bcee5c8bb839d2d1c5e3456b8c6886b) )
	ROM_LOAD16_BYTE( "jojo-simm5.0", 0x2000000, 0x200000, CRC(797615fc) SHA1(29874be9f1da5515c90f5d601aa5924c263f8feb) )
	ROM_LOAD16_BYTE( "jojo-simm5.1", 0x2000001, 0x200000, CRC(734fd162) SHA1(16cdfac74d18a6c2216afb1ce6afbd7f15297c32) )
ROM_END

ROM_START( jojojr1 )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojo_japan.29f400.u2", 0x000000, 0x080000, CRC(02778f60) SHA1(a167f9ebe030592a0cdb0c6a3c75835c6a43be4c) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "jojo-simm1.0", 0x0000000, 0x200000, CRC(cfbc38d6) SHA1(c33e3a51fe8ab54e0912a1d6e662fe1ade73cee7) )
	ROM_LOAD32_BYTE( "jojo-simm1.1", 0x0000001, 0x200000, CRC(42578d94) SHA1(fa46f92ac1a6716430adec9ab27214a11fa61749) )
	ROM_LOAD32_BYTE( "jojo-simm1.2", 0x0000002, 0x200000, CRC(1b40c566) SHA1(9833799e9b4fecf7f9ce14bca64936646b3fdbde) )
	ROM_LOAD32_BYTE( "jojo-simm1.3", 0x0000003, 0x200000, CRC(bba709b4) SHA1(0dd71e575f2193505f2ab960568ac1eccf40d53f) )
	ROM_LOAD32_BYTE( "jojo-simm2.0", 0x0800000, 0x200000, CRC(417e5dc1) SHA1(54ee9596c1c51811f3bdef7dbe77b44b34f230ca) )
	ROM_LOAD32_BYTE( "jojo-simm2.1", 0x0800001, 0x200000, CRC(d3b3267d) SHA1(eb2cff347880f1489fb5b1b8bd16df8f50c7f494) )
	ROM_LOAD32_BYTE( "jojo-simm2.2", 0x0800002, 0x200000, CRC(c66d96b1) SHA1(909d5aac165748b549b6056a6091c41df012f5df) )
	ROM_LOAD32_BYTE( "jojo-simm2.3", 0x0800003, 0x200000, CRC(aa34cc85) SHA1(7677cc6fa913755fc699691b350698bbe8904118) )

	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "jojo-simm3.0", 0x0000000, 0x200000, CRC(de7fc9c1) SHA1(662b85a990b04c855773506c936317e62fab4a05) )
	ROM_LOAD16_BYTE( "jojo-simm3.1", 0x0000001, 0x200000, CRC(43d053d3) SHA1(54ff0e9c164e0d1649522c330ccc7e5d79e0bc85) )
	ROM_LOAD16_BYTE( "jojo-simm3.2", 0x0400000, 0x200000, CRC(2ffd7fa5) SHA1(9018c8e2b286a333ba606208e90caa764951ea3f) )
	ROM_LOAD16_BYTE( "jojo-simm3.3", 0x0400001, 0x200000, CRC(4da4985b) SHA1(2552b1730a21ce17d58b69a79ad212a6a5829439) )
	ROM_LOAD16_BYTE( "jojo-simm3.4", 0x0800000, 0x200000, CRC(fde98d72) SHA1(654563e12d033e8656dc74a268a08b15b171470d) )
	ROM_LOAD16_BYTE( "jojo-simm3.5", 0x0800001, 0x200000, CRC(edb2a266) SHA1(19ebada8422c7f4bf70d0c9ad42b84268967b316) )
	ROM_LOAD16_BYTE( "jojo-simm3.6", 0x0c00000, 0x200000, CRC(be7cf319) SHA1(7893f5907992e6b903b2683980bba6d3d003bb06) )
	ROM_LOAD16_BYTE( "jojo-simm3.7", 0x0c00001, 0x200000, CRC(56fe1a9f) SHA1(01741fe1256f4e682f687e94040f4e8bbb8bedb2) )
	ROM_LOAD16_BYTE( "jojo-simm4.0", 0x1000000, 0x200000, CRC(c4e7bf68) SHA1(a4d1ddea58a3d42db82a63a5e974cbf38d9b792a) )
	ROM_LOAD16_BYTE( "jojo-simm4.1", 0x1000001, 0x200000, CRC(b62b2719) SHA1(cb577b89e9e14fda67715716fefd47a782d518ab) )
	ROM_LOAD16_BYTE( "jojo-simm4.2", 0x1400000, 0x200000, CRC(18d15809) SHA1(2b406cd1aaa4799a436213dcaa65473eacb4c6d7) )
	ROM_LOAD16_BYTE( "jojo-simm4.3", 0x1400001, 0x200000, CRC(9af0ad79) SHA1(075ee048e17b50188876f25d7a6571d6ace84d7d) )
	ROM_LOAD16_BYTE( "jojo-simm4.4", 0x1800000, 0x200000, CRC(4124c1f0) SHA1(e4946a8029adc5d0bacead8d766521b4ccd1722b) )
	ROM_LOAD16_BYTE( "jojo-simm4.5", 0x1800001, 0x200000, CRC(5e001fd1) SHA1(6457a39f336381b46e587aa2f5f719810ee5bcf9) )
	ROM_LOAD16_BYTE( "jojo-simm4.6", 0x1c00000, 0x200000, CRC(9affa23b) SHA1(e3d77e777c47277d841a9dadc1dd6e3157706a2e) )
	ROM_LOAD16_BYTE( "jojo-simm4.7", 0x1c00001, 0x200000, CRC(2511572a) SHA1(725adcf71bcee5c8bb839d2d1c5e3456b8c6886b) )
	ROM_LOAD16_BYTE( "jojo-simm5.0", 0x2000000, 0x200000, CRC(797615fc) SHA1(29874be9f1da5515c90f5d601aa5924c263f8feb) )
	ROM_LOAD16_BYTE( "jojo-simm5.1", 0x2000001, 0x200000, CRC(734fd162) SHA1(16cdfac74d18a6c2216afb1ce6afbd7f15297c32) )
ROM_END

ROM_START( sfiii3 )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii3_euro.29f400.u2", 0x000000, 0x080000, CRC(30bbf293) SHA1(f094c2eeaf4f6709060197aca371a4532346bf78) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "sfiii3-simm1.0", 0x0000000, 0x200000, CRC(11dfd3cd) SHA1(dba1f77c46e80317e3279298411154dfb6db2309) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.1", 0x0000001, 0x200000, CRC(c50585e6) SHA1(a289237957ea1c7f58b1c65e24c54ceb34cb1712) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.2", 0x0000002, 0x200000, CRC(8e011d9b) SHA1(e0861bcd3c4f865474d7ce47aa9eeec7b3d28da6) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.3", 0x0000003, 0x200000, CRC(dca8d92f) SHA1(7cd241641c943df446e2c75b88b5cf2d2ebf7b2e) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.0", 0x0800000, 0x200000, CRC(06eb969e) SHA1(d89f6a6585b76692d57d337f0f8186398fb056da) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.1", 0x0800001, 0x200000, CRC(e7039f82) SHA1(8e81e66b5a4f45ae14b070a491bde47a6a74499f) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.2", 0x0800002, 0x200000, CRC(645c96f7) SHA1(06d5a54874d4bf100b776131ec9060da209ad037) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.3", 0x0800003, 0x200000, CRC(610efab1) SHA1(bbc21ed6ff6220ff6017a3f02ebd9a341fbc9040) )

	ROM_REGION16_BE( 0x4000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii3-simm3.0", 0x0000000, 0x200000, CRC(7baa1f79) SHA1(3f409df28c24dd7221966b5340d59898ea756b6f) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.1", 0x0000001, 0x200000, CRC(234bf8fe) SHA1(2191781ae4d726cab28de97f27efa4a13f3bdd69) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.2", 0x0400000, 0x200000, CRC(d9ebc308) SHA1(af6a0dca77e5181c9f20533a06760a782c5fd51d) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.3", 0x0400001, 0x200000, CRC(293cba77) SHA1(294604cacdc24261aec4d39e489de91c41fa1758) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.4", 0x0800000, 0x200000, CRC(6055e747) SHA1(3813852c5a4a5355ef739ca8f0913bbd390b984b) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.5", 0x0800001, 0x200000, CRC(499aa6fc) SHA1(5b9b6eab3e99ff3e1d7c1f50b9d8bc6a81f3f8a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.6", 0x0c00000, 0x200000, CRC(6c13879e) SHA1(de189b0b8f42bc7dd89983e62bc2ecb4237b3277) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.7", 0x0c00001, 0x200000, CRC(cf4f8ede) SHA1(e0fb68fcb0e445f824c62fa828d6e1dcd7e3683a) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.0", 0x1000000, 0x200000, CRC(091fd5ba) SHA1(3327ad7c2623c119bf728af717ea2ce3b74673a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.1", 0x1000001, 0x200000, CRC(0bca8917) SHA1(b7b284e2f16f46d46bcfaae779b232c5b980924f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.2", 0x1400000, 0x200000, CRC(a0fd578b) SHA1(100c9db9f00ecd88d518076f5a0822e6ac3695b3) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.3", 0x1400001, 0x200000, CRC(4bf8c699) SHA1(2c0b4288b5ebc5e54d9e782dfc39eb8c78fd4c21) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.4", 0x1800000, 0x200000, CRC(137b8785) SHA1(56a579520a8ce2abbf36be57777f024e80474eee) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.5", 0x1800001, 0x200000, CRC(4fb70671) SHA1(9aba83c18cfc099a5ce18793119bff0c2b9c777f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.6", 0x1c00000, 0x200000, CRC(832374a4) SHA1(c84629e32fbf47cb7b5b4ee7555bfc2ac9b3857f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.7", 0x1c00001, 0x200000, CRC(1c88576d) SHA1(0f039944d0c2305999ed5dbd351c3eb87812dc3b) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.0", 0x2000000, 0x200000, CRC(c67d9190) SHA1(d265475244099d0ec153059986f3445c7bd910a3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.1", 0x2000001, 0x200000, CRC(6cb79868) SHA1(c94237f30e05bfcb2e23945530c812d9e4c73416) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.2", 0x2400000, 0x200000, CRC(df69930e) SHA1(c76b7c559a1d5558138afbc796249efa2f49f6a8) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.3", 0x2400001, 0x200000, CRC(333754e0) SHA1(4c18a569c26524a492ecd6f4c8b3c8e803a077d3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.4", 0x2800000, 0x200000, CRC(78f6d417) SHA1(a69577cc5399fcf0a24548661168f27f3e7e8e40) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.5", 0x2800001, 0x200000, CRC(8ccad9b1) SHA1(f8bda399f87be2497b7ac39e9661f9863bf4f873) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.6", 0x2c00000, 0x200000, CRC(85de59e5) SHA1(748b5c91f15777b85d8c1d35b685cd90d3185ec6) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.7", 0x2c00001, 0x200000, CRC(ee7e29b3) SHA1(63dc30c6904ca2f58d229249bee5eef51fafa158) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.0", 0x3000000, 0x200000, CRC(8da69042) SHA1(fd3d08295342635b2136e48d543c9350d287bb22) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.1", 0x3000001, 0x200000, CRC(1c8c7ac4) SHA1(ac9f8353a4c356ef98aa7c226baba00b01f5a80f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.2", 0x3400000, 0x200000, CRC(a671341d) SHA1(636f4c04962bc1e1ddb29d2e01244b00389b234f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.3", 0x3400001, 0x200000, CRC(1a990249) SHA1(2acc639e2c0c53bf24096b8620eab090bc25d03b) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.4", 0x3800000, 0x200000, CRC(20cb39ac) SHA1(7d13a0fea1ef719dd2ff77dfb547d53c6023cc9e) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.5", 0x3800001, 0x200000, CRC(5f844b2f) SHA1(564e4934f89ed3b92a4c4874519f8f00f3b48696) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.6", 0x3c00000, 0x200000, CRC(450e8d28) SHA1(885db658132aa27926df617ec2d2a1f38abdbb60) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.7", 0x3c00001, 0x200000, CRC(cc5f4187) SHA1(248ddace21ed4736a56e92f77cc6ad219d7fef0b) )
ROM_END

ROM_START( sfiii3ur1 )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii3_usa.29f400.u2", 0x000000, 0x080000, CRC(ecc545c1) SHA1(e39083820aae914fd8b80c9765129bedb745ceba) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "sfiii3-simm1.0", 0x0000000, 0x200000, CRC(66e66235) SHA1(0a98038721d176458d4f85dbd76c5edb93a65322) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.1", 0x0000001, 0x200000, CRC(186e8c5f) SHA1(a63040201a660b56217a8cbab32f5c2c466ee5dd) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.2", 0x0000002, 0x200000, CRC(bce18cab) SHA1(a5c28063d98c22403756fc926a20631456fb7dcc) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.3", 0x0000003, 0x200000, CRC(129dc2c9) SHA1(c1e634d94b1c8f7f02a47703622de5cab3d0da3f) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.0", 0x0800000, 0x200000, CRC(06eb969e) SHA1(d89f6a6585b76692d57d337f0f8186398fb056da) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.1", 0x0800001, 0x200000, CRC(e7039f82) SHA1(8e81e66b5a4f45ae14b070a491bde47a6a74499f) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.2", 0x0800002, 0x200000, CRC(645c96f7) SHA1(06d5a54874d4bf100b776131ec9060da209ad037) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.3", 0x0800003, 0x200000, CRC(610efab1) SHA1(bbc21ed6ff6220ff6017a3f02ebd9a341fbc9040) )

	ROM_REGION16_BE( 0x4000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii3-simm3.0", 0x0000000, 0x200000, CRC(7baa1f79) SHA1(3f409df28c24dd7221966b5340d59898ea756b6f) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.1", 0x0000001, 0x200000, CRC(234bf8fe) SHA1(2191781ae4d726cab28de97f27efa4a13f3bdd69) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.2", 0x0400000, 0x200000, CRC(d9ebc308) SHA1(af6a0dca77e5181c9f20533a06760a782c5fd51d) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.3", 0x0400001, 0x200000, CRC(293cba77) SHA1(294604cacdc24261aec4d39e489de91c41fa1758) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.4", 0x0800000, 0x200000, CRC(6055e747) SHA1(3813852c5a4a5355ef739ca8f0913bbd390b984b) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.5", 0x0800001, 0x200000, CRC(499aa6fc) SHA1(5b9b6eab3e99ff3e1d7c1f50b9d8bc6a81f3f8a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.6", 0x0c00000, 0x200000, CRC(6c13879e) SHA1(de189b0b8f42bc7dd89983e62bc2ecb4237b3277) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.7", 0x0c00001, 0x200000, CRC(cf4f8ede) SHA1(e0fb68fcb0e445f824c62fa828d6e1dcd7e3683a) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.0", 0x1000000, 0x200000, CRC(091fd5ba) SHA1(3327ad7c2623c119bf728af717ea2ce3b74673a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.1", 0x1000001, 0x200000, CRC(0bca8917) SHA1(b7b284e2f16f46d46bcfaae779b232c5b980924f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.2", 0x1400000, 0x200000, CRC(a0fd578b) SHA1(100c9db9f00ecd88d518076f5a0822e6ac3695b3) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.3", 0x1400001, 0x200000, CRC(4bf8c699) SHA1(2c0b4288b5ebc5e54d9e782dfc39eb8c78fd4c21) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.4", 0x1800000, 0x200000, CRC(137b8785) SHA1(56a579520a8ce2abbf36be57777f024e80474eee) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.5", 0x1800001, 0x200000, CRC(4fb70671) SHA1(9aba83c18cfc099a5ce18793119bff0c2b9c777f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.6", 0x1c00000, 0x200000, CRC(832374a4) SHA1(c84629e32fbf47cb7b5b4ee7555bfc2ac9b3857f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.7", 0x1c00001, 0x200000, CRC(1c88576d) SHA1(0f039944d0c2305999ed5dbd351c3eb87812dc3b) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.0", 0x2000000, 0x200000, CRC(c67d9190) SHA1(d265475244099d0ec153059986f3445c7bd910a3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.1", 0x2000001, 0x200000, CRC(6cb79868) SHA1(c94237f30e05bfcb2e23945530c812d9e4c73416) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.2", 0x2400000, 0x200000, CRC(df69930e) SHA1(c76b7c559a1d5558138afbc796249efa2f49f6a8) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.3", 0x2400001, 0x200000, CRC(333754e0) SHA1(4c18a569c26524a492ecd6f4c8b3c8e803a077d3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.4", 0x2800000, 0x200000, CRC(78f6d417) SHA1(a69577cc5399fcf0a24548661168f27f3e7e8e40) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.5", 0x2800001, 0x200000, CRC(8ccad9b1) SHA1(f8bda399f87be2497b7ac39e9661f9863bf4f873) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.6", 0x2c00000, 0x200000, CRC(85de59e5) SHA1(748b5c91f15777b85d8c1d35b685cd90d3185ec6) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.7", 0x2c00001, 0x200000, CRC(ee7e29b3) SHA1(63dc30c6904ca2f58d229249bee5eef51fafa158) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.0", 0x3000000, 0x200000, CRC(8da69042) SHA1(fd3d08295342635b2136e48d543c9350d287bb22) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.1", 0x3000001, 0x200000, CRC(1c8c7ac4) SHA1(ac9f8353a4c356ef98aa7c226baba00b01f5a80f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.2", 0x3400000, 0x200000, CRC(a671341d) SHA1(636f4c04962bc1e1ddb29d2e01244b00389b234f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.3", 0x3400001, 0x200000, CRC(1a990249) SHA1(2acc639e2c0c53bf24096b8620eab090bc25d03b) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.4", 0x3800000, 0x200000, CRC(20cb39ac) SHA1(7d13a0fea1ef719dd2ff77dfb547d53c6023cc9e) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.5", 0x3800001, 0x200000, CRC(5f844b2f) SHA1(564e4934f89ed3b92a4c4874519f8f00f3b48696) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.6", 0x3c00000, 0x200000, CRC(450e8d28) SHA1(885db658132aa27926df617ec2d2a1f38abdbb60) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.7", 0x3c00001, 0x200000, CRC(cc5f4187) SHA1(248ddace21ed4736a56e92f77cc6ad219d7fef0b) )
ROM_END

ROM_START( sfiii3u )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii3_usa.29f400.u2", 0x000000, 0x080000, CRC(ecc545c1) SHA1(e39083820aae914fd8b80c9765129bedb745ceba) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "sfiii3-simm1.0", 0x0000000, 0x200000, CRC(11dfd3cd) SHA1(dba1f77c46e80317e3279298411154dfb6db2309) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.1", 0x0000001, 0x200000, CRC(c50585e6) SHA1(a289237957ea1c7f58b1c65e24c54ceb34cb1712) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.2", 0x0000002, 0x200000, CRC(8e011d9b) SHA1(e0861bcd3c4f865474d7ce47aa9eeec7b3d28da6) )
	ROM_LOAD32_BYTE( "sfiii3-simm1.3", 0x0000003, 0x200000, CRC(dca8d92f) SHA1(7cd241641c943df446e2c75b88b5cf2d2ebf7b2e) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.0", 0x0800000, 0x200000, CRC(06eb969e) SHA1(d89f6a6585b76692d57d337f0f8186398fb056da) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.1", 0x0800001, 0x200000, CRC(e7039f82) SHA1(8e81e66b5a4f45ae14b070a491bde47a6a74499f) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.2", 0x0800002, 0x200000, CRC(645c96f7) SHA1(06d5a54874d4bf100b776131ec9060da209ad037) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.3", 0x0800003, 0x200000, CRC(610efab1) SHA1(bbc21ed6ff6220ff6017a3f02ebd9a341fbc9040) )

	ROM_REGION16_BE( 0x4000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii3-simm3.0", 0x0000000, 0x200000, CRC(7baa1f79) SHA1(3f409df28c24dd7221966b5340d59898ea756b6f) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.1", 0x0000001, 0x200000, CRC(234bf8fe) SHA1(2191781ae4d726cab28de97f27efa4a13f3bdd69) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.2", 0x0400000, 0x200000, CRC(d9ebc308) SHA1(af6a0dca77e5181c9f20533a06760a782c5fd51d) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.3", 0x0400001, 0x200000, CRC(293cba77) SHA1(294604cacdc24261aec4d39e489de91c41fa1758) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.4", 0x0800000, 0x200000, CRC(6055e747) SHA1(3813852c5a4a5355ef739ca8f0913bbd390b984b) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.5", 0x0800001, 0x200000, CRC(499aa6fc) SHA1(5b9b6eab3e99ff3e1d7c1f50b9d8bc6a81f3f8a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.6", 0x0c00000, 0x200000, CRC(6c13879e) SHA1(de189b0b8f42bc7dd89983e62bc2ecb4237b3277) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.7", 0x0c00001, 0x200000, CRC(cf4f8ede) SHA1(e0fb68fcb0e445f824c62fa828d6e1dcd7e3683a) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.0", 0x1000000, 0x200000, CRC(091fd5ba) SHA1(3327ad7c2623c119bf728af717ea2ce3b74673a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.1", 0x1000001, 0x200000, CRC(0bca8917) SHA1(b7b284e2f16f46d46bcfaae779b232c5b980924f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.2", 0x1400000, 0x200000, CRC(a0fd578b) SHA1(100c9db9f00ecd88d518076f5a0822e6ac3695b3) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.3", 0x1400001, 0x200000, CRC(4bf8c699) SHA1(2c0b4288b5ebc5e54d9e782dfc39eb8c78fd4c21) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.4", 0x1800000, 0x200000, CRC(137b8785) SHA1(56a579520a8ce2abbf36be57777f024e80474eee) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.5", 0x1800001, 0x200000, CRC(4fb70671) SHA1(9aba83c18cfc099a5ce18793119bff0c2b9c777f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.6", 0x1c00000, 0x200000, CRC(832374a4) SHA1(c84629e32fbf47cb7b5b4ee7555bfc2ac9b3857f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.7", 0x1c00001, 0x200000, CRC(1c88576d) SHA1(0f039944d0c2305999ed5dbd351c3eb87812dc3b) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.0", 0x2000000, 0x200000, CRC(c67d9190) SHA1(d265475244099d0ec153059986f3445c7bd910a3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.1", 0x2000001, 0x200000, CRC(6cb79868) SHA1(c94237f30e05bfcb2e23945530c812d9e4c73416) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.2", 0x2400000, 0x200000, CRC(df69930e) SHA1(c76b7c559a1d5558138afbc796249efa2f49f6a8) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.3", 0x2400001, 0x200000, CRC(333754e0) SHA1(4c18a569c26524a492ecd6f4c8b3c8e803a077d3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.4", 0x2800000, 0x200000, CRC(78f6d417) SHA1(a69577cc5399fcf0a24548661168f27f3e7e8e40) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.5", 0x2800001, 0x200000, CRC(8ccad9b1) SHA1(f8bda399f87be2497b7ac39e9661f9863bf4f873) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.6", 0x2c00000, 0x200000, CRC(85de59e5) SHA1(748b5c91f15777b85d8c1d35b685cd90d3185ec6) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.7", 0x2c00001, 0x200000, CRC(ee7e29b3) SHA1(63dc30c6904ca2f58d229249bee5eef51fafa158) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.0", 0x3000000, 0x200000, CRC(8da69042) SHA1(fd3d08295342635b2136e48d543c9350d287bb22) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.1", 0x3000001, 0x200000, CRC(1c8c7ac4) SHA1(ac9f8353a4c356ef98aa7c226baba00b01f5a80f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.2", 0x3400000, 0x200000, CRC(a671341d) SHA1(636f4c04962bc1e1ddb29d2e01244b00389b234f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.3", 0x3400001, 0x200000, CRC(1a990249) SHA1(2acc639e2c0c53bf24096b8620eab090bc25d03b) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.4", 0x3800000, 0x200000, CRC(20cb39ac) SHA1(7d13a0fea1ef719dd2ff77dfb547d53c6023cc9e) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.5", 0x3800001, 0x200000, CRC(5f844b2f) SHA1(564e4934f89ed3b92a4c4874519f8f00f3b48696) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.6", 0x3c00000, 0x200000, CRC(450e8d28) SHA1(885db658132aa27926df617ec2d2a1f38abdbb60) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.7", 0x3c00001, 0x200000, CRC(cc5f4187) SHA1(248ddace21ed4736a56e92f77cc6ad219d7fef0b) )
ROM_END

ROM_START( jojoba )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojoba_euro_nocd.29f400.u2", 0x000000, 0x080000, CRC(1ee2d679) SHA1(9e129b454a376606b3f7e8aec64de425cf9c635c) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "jojoba-simm1.0", 0x0000000, 0x200000, CRC(b3cc516d) SHA1(cac67a5bf9d01af226be377e2b1b8ed4ea7fd164) )
	ROM_LOAD32_BYTE( "jojoba-simm1.1", 0x0000001, 0x200000, CRC(dab4bdc7) SHA1(47283071de152a0b2b96d3f3fc22537677c35d3f) )
	ROM_LOAD32_BYTE( "jojoba-simm1.2", 0x0000002, 0x200000, CRC(a6a4bf48) SHA1(1684663252894538de4855eee336ffc031142ba9) )
	ROM_LOAD32_BYTE( "jojoba-simm1.3", 0x0000003, 0x200000, CRC(731229ee) SHA1(2171ffa115752b0dcc1695f4a5d0557237a73db8) )
	ROM_LOAD32_BYTE( "jojoba-simm2.0", 0x0800000, 0x200000, CRC(535f2eba) SHA1(167bec0dccfc2f91cb10cb1e2631ee619b3eb9fe) )
	ROM_LOAD32_BYTE( "jojoba-simm2.1", 0x0800001, 0x200000, CRC(01dd3a01) SHA1(08c462219796baa3ec28d78d038a18187cd838bb) )
	ROM_LOAD32_BYTE( "jojoba-simm2.2", 0x0800002, 0x200000, CRC(61432672) SHA1(d0416a75d395926041f90a3d34edb96a080acfd6) )
	ROM_LOAD32_BYTE( "jojoba-simm2.3", 0x0800003, 0x200000, CRC(acdc9aca) SHA1(89f77ddd6286709182a676fd9bd6c333a3b16271) )

	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "jojoba-simm3.0", 0x0000000, 0x200000, CRC(4d16e111) SHA1(f198007375be65e89856d64ee2b3857a18b4eab8) )
	ROM_LOAD16_BYTE( "jojoba-simm3.1", 0x0000001, 0x200000, CRC(9b3406d3) SHA1(54e90cd334d13e2c74305c6b87ebce1365ef3d59) )
	ROM_LOAD16_BYTE( "jojoba-simm3.2", 0x0400000, 0x200000, CRC(f2414997) SHA1(fb89d5784250538ad17fd527267b513afb6eca20) )
	ROM_LOAD16_BYTE( "jojoba-simm3.3", 0x0400001, 0x200000, CRC(954b9c7d) SHA1(0d64d97167d4e669d7e4f3a388f9d5ec1e18ed42) )
	ROM_LOAD16_BYTE( "jojoba-simm3.4", 0x0800000, 0x200000, CRC(625adc1d) SHA1(533d62759ecece10c711d99bfca403e5cba279b5) )
	ROM_LOAD16_BYTE( "jojoba-simm3.5", 0x0800001, 0x200000, CRC(20a70bb4) SHA1(3bd8376304ffc974fb8031eac8bebff27969538c) )
	ROM_LOAD16_BYTE( "jojoba-simm3.6", 0x0c00000, 0x200000, CRC(a10ec5af) SHA1(9b403260e8fbdacaa5369ab79fc05855cc6a6bdb) )
	ROM_LOAD16_BYTE( "jojoba-simm3.7", 0x0c00001, 0x200000, CRC(0bd0de7a) SHA1(1debecda5f282f2a1dd17e887e522a4d00c5dc9d) )
	ROM_LOAD16_BYTE( "jojoba-simm4.0", 0x1000000, 0x200000, CRC(6ea14adc) SHA1(696b2ec66f3c197817a60f507a1b4c78db37f488) )
	ROM_LOAD16_BYTE( "jojoba-simm4.1", 0x1000001, 0x200000, CRC(8f4c42fb) SHA1(363d769b0b066ce139125426d2da6dfa15d1eb28) )
	ROM_LOAD16_BYTE( "jojoba-simm4.2", 0x1400000, 0x200000, CRC(ef0586d1) SHA1(8fcc350da20e3e59fa76fa14e10f2c47233ba9dc) )
	ROM_LOAD16_BYTE( "jojoba-simm4.3", 0x1400001, 0x200000, CRC(93ccc470) SHA1(5d267679e61c0fb592ad5f696d3c06ec1746d0b3) )
	ROM_LOAD16_BYTE( "jojoba-simm4.4", 0x1800000, 0x200000, CRC(3d9ec7d2) SHA1(665b867bab928be183c2006527e55f9b8ec4a271) )
	ROM_LOAD16_BYTE( "jojoba-simm4.5", 0x1800001, 0x200000, CRC(03e66850) SHA1(8478662dc9db20d9a186d315a883bd1cbb5e5000) )
	ROM_LOAD16_BYTE( "jojoba-simm4.6", 0x1c00000, 0x200000, CRC(01606ac3) SHA1(ccc74edeca6abdd86fc1cf42ececa1ea393b3261) )
	ROM_LOAD16_BYTE( "jojoba-simm4.7", 0x1c00001, 0x200000, CRC(36392b87) SHA1(e62080c8461775c1e180400dfb44414679fd0fc1) )
	ROM_LOAD16_BYTE( "jojoba-simm5.0", 0x2000000, 0x200000, CRC(2ef8c60c) SHA1(dea87a73a11b8edd27c3c9c5ab2af295cb5508f9) )
	ROM_LOAD16_BYTE( "jojoba-simm5.1", 0x2000001, 0x200000, CRC(cf7d7ca6) SHA1(b347707b1e5bc71d28b282273f893592e5f9e333) )
	ROM_LOAD16_BYTE( "jojoba-simm5.2", 0x2400000, 0x200000, CRC(b7815bfa) SHA1(0b5a3a2ffe1b3c0ca765dcedc297e78e5928302b) )
	ROM_LOAD16_BYTE( "jojoba-simm5.3", 0x2400001, 0x200000, CRC(9bfec049) SHA1(62cc9a1920047863205544b77344ee18f310f084) )
	ROM_LOAD16_BYTE( "jojoba-simm5.4", 0x2800000, 0x200000, CRC(d167536b) SHA1(e2637d3486f168ce44e0a00413d38960cb86db4c) )
	ROM_LOAD16_BYTE( "jojoba-simm5.5", 0x2800001, 0x200000, CRC(55e7a042) SHA1(c18bda61fa005d9174a27b7b7d324004262a4525) )
	ROM_LOAD16_BYTE( "jojoba-simm5.6", 0x2c00000, 0x200000, CRC(4fb32906) SHA1(3a5965b3197517932c8aa4c07a6ea6a190a338d7) )
	ROM_LOAD16_BYTE( "jojoba-simm5.7", 0x2c00001, 0x200000, CRC(8c8be520) SHA1(c461f3f76a83592b36b29afb316679a7c8972404) )
ROM_END

ROM_START( jojobanr2 )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojoba_japan_nocd.29f400.u2", 0x000000, 0x080000, CRC(4dab19f5) SHA1(ba07190e7662937fc267f07285c51e99a45c061e) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "jojoba-simm1.0", 0x0000000, 0x200000, CRC(76976231) SHA1(90adde7e5983ec6a4e02789d5cefe9e85c9c52d5) )
	ROM_LOAD32_BYTE( "jojoba-simm1.1", 0x0000001, 0x200000, CRC(cedd78e7) SHA1(964988b90a2f14c1da2cfc48d943e16e54da3fd3) )
	ROM_LOAD32_BYTE( "jojoba-simm1.2", 0x0000002, 0x200000, CRC(2955b77f) SHA1(2a907a5cd91448bfc420c318584e5ef4bbe55a91) )
	ROM_LOAD32_BYTE( "jojoba-simm1.3", 0x0000003, 0x200000, CRC(280139d7) SHA1(b7c28f6f0218688fb873a3106d2f95ea2e1e927c) )
	ROM_LOAD32_BYTE( "jojoba-simm2.0", 0x0800000, 0x200000, CRC(305c4914) SHA1(c3a73ffe58f61ab8f1cd9e3f0891037638dc5a9b) )
	ROM_LOAD32_BYTE( "jojoba-simm2.1", 0x0800001, 0x200000, CRC(18af4f3b) SHA1(04b8fdf23a782b10c203b111cc634a6d3474044a) )
	ROM_LOAD32_BYTE( "jojoba-simm2.2", 0x0800002, 0x200000, CRC(397e5c9e) SHA1(021d86ee66bf951fb6a1dd90fb7007c6865cbb8b) )
	ROM_LOAD32_BYTE( "jojoba-simm2.3", 0x0800003, 0x200000, CRC(a9d0a7d7) SHA1(b2cfc0661f8903ddbeea8a604ee8b42097e10ab8) )

	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "jojoba-simm3.0", 0x0000000, 0x200000, CRC(4d16e111) SHA1(f198007375be65e89856d64ee2b3857a18b4eab8) )
	ROM_LOAD16_BYTE( "jojoba-simm3.1", 0x0000001, 0x200000, CRC(9b3406d3) SHA1(54e90cd334d13e2c74305c6b87ebce1365ef3d59) )
	ROM_LOAD16_BYTE( "jojoba-simm3.2", 0x0400000, 0x200000, CRC(f2414997) SHA1(fb89d5784250538ad17fd527267b513afb6eca20) )
	ROM_LOAD16_BYTE( "jojoba-simm3.3", 0x0400001, 0x200000, CRC(954b9c7d) SHA1(0d64d97167d4e669d7e4f3a388f9d5ec1e18ed42) )
	ROM_LOAD16_BYTE( "jojoba-simm3.4", 0x0800000, 0x200000, CRC(625adc1d) SHA1(533d62759ecece10c711d99bfca403e5cba279b5) )
	ROM_LOAD16_BYTE( "jojoba-simm3.5", 0x0800001, 0x200000, CRC(20a70bb4) SHA1(3bd8376304ffc974fb8031eac8bebff27969538c) )
	ROM_LOAD16_BYTE( "jojoba-simm3.6", 0x0c00000, 0x200000, CRC(a10ec5af) SHA1(9b403260e8fbdacaa5369ab79fc05855cc6a6bdb) )
	ROM_LOAD16_BYTE( "jojoba-simm3.7", 0x0c00001, 0x200000, CRC(0bd0de7a) SHA1(1debecda5f282f2a1dd17e887e522a4d00c5dc9d) )
	ROM_LOAD16_BYTE( "jojoba-simm4.0", 0x1000000, 0x200000, CRC(6ea14adc) SHA1(696b2ec66f3c197817a60f507a1b4c78db37f488) )
	ROM_LOAD16_BYTE( "jojoba-simm4.1", 0x1000001, 0x200000, CRC(8f4c42fb) SHA1(363d769b0b066ce139125426d2da6dfa15d1eb28) )
	ROM_LOAD16_BYTE( "jojoba-simm4.2", 0x1400000, 0x200000, CRC(ef0586d1) SHA1(8fcc350da20e3e59fa76fa14e10f2c47233ba9dc) )
	ROM_LOAD16_BYTE( "jojoba-simm4.3", 0x1400001, 0x200000, CRC(93ccc470) SHA1(5d267679e61c0fb592ad5f696d3c06ec1746d0b3) )
	ROM_LOAD16_BYTE( "jojoba-simm4.4", 0x1800000, 0x200000, CRC(3d9ec7d2) SHA1(665b867bab928be183c2006527e55f9b8ec4a271) )
	ROM_LOAD16_BYTE( "jojoba-simm4.5", 0x1800001, 0x200000, CRC(03e66850) SHA1(8478662dc9db20d9a186d315a883bd1cbb5e5000) )
	ROM_LOAD16_BYTE( "jojoba-simm4.6", 0x1c00000, 0x200000, CRC(01606ac3) SHA1(ccc74edeca6abdd86fc1cf42ececa1ea393b3261) )
	ROM_LOAD16_BYTE( "jojoba-simm4.7", 0x1c00001, 0x200000, CRC(36392b87) SHA1(e62080c8461775c1e180400dfb44414679fd0fc1) )
	ROM_LOAD16_BYTE( "jojoba-simm5.0", 0x2000000, 0x200000, CRC(2ef8c60c) SHA1(dea87a73a11b8edd27c3c9c5ab2af295cb5508f9) )
	ROM_LOAD16_BYTE( "jojoba-simm5.1", 0x2000001, 0x200000, CRC(cf7d7ca6) SHA1(b347707b1e5bc71d28b282273f893592e5f9e333) )
	ROM_LOAD16_BYTE( "jojoba-simm5.2", 0x2400000, 0x200000, CRC(b7815bfa) SHA1(0b5a3a2ffe1b3c0ca765dcedc297e78e5928302b) )
	ROM_LOAD16_BYTE( "jojoba-simm5.3", 0x2400001, 0x200000, CRC(9bfec049) SHA1(62cc9a1920047863205544b77344ee18f310f084) )
	ROM_LOAD16_BYTE( "jojoba-simm5.4", 0x2800000, 0x200000, CRC(d167536b) SHA1(e2637d3486f168ce44e0a00413d38960cb86db4c) )
	ROM_LOAD16_BYTE( "jojoba-simm5.5", 0x2800001, 0x200000, CRC(55e7a042) SHA1(c18bda61fa005d9174a27b7b7d324004262a4525) )
	ROM_LOAD16_BYTE( "jojoba-simm5.6", 0x2c00000, 0x200000, CRC(4fb32906) SHA1(3a5965b3197517932c8aa4c07a6ea6a190a338d7) )
	ROM_LOAD16_BYTE( "jojoba-simm5.7", 0x2c00001, 0x200000, CRC(8c8be520) SHA1(c461f3f76a83592b36b29afb316679a7c8972404) )
ROM_END

ROM_START( jojobajr2 )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojoba_japan.29f400.u2", 0x000000, 0x080000, CRC(3085478c) SHA1(055eab1fc42816f370a44b17fd7e87ffcb10e8b7) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "jojoba-simm1.0", 0x0000000, 0x200000, CRC(76976231) SHA1(90adde7e5983ec6a4e02789d5cefe9e85c9c52d5) )
	ROM_LOAD32_BYTE( "jojoba-simm1.1", 0x0000001, 0x200000, CRC(cedd78e7) SHA1(964988b90a2f14c1da2cfc48d943e16e54da3fd3) )
	ROM_LOAD32_BYTE( "jojoba-simm1.2", 0x0000002, 0x200000, CRC(2955b77f) SHA1(2a907a5cd91448bfc420c318584e5ef4bbe55a91) )
	ROM_LOAD32_BYTE( "jojoba-simm1.3", 0x0000003, 0x200000, CRC(280139d7) SHA1(b7c28f6f0218688fb873a3106d2f95ea2e1e927c) )
	ROM_LOAD32_BYTE( "jojoba-simm2.0", 0x0800000, 0x200000, CRC(305c4914) SHA1(c3a73ffe58f61ab8f1cd9e3f0891037638dc5a9b) )
	ROM_LOAD32_BYTE( "jojoba-simm2.1", 0x0800001, 0x200000, CRC(18af4f3b) SHA1(04b8fdf23a782b10c203b111cc634a6d3474044a) )
	ROM_LOAD32_BYTE( "jojoba-simm2.2", 0x0800002, 0x200000, CRC(397e5c9e) SHA1(021d86ee66bf951fb6a1dd90fb7007c6865cbb8b) )
	ROM_LOAD32_BYTE( "jojoba-simm2.3", 0x0800003, 0x200000, CRC(a9d0a7d7) SHA1(b2cfc0661f8903ddbeea8a604ee8b42097e10ab8) )

	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "jojoba-simm3.0", 0x0000000, 0x200000, CRC(4d16e111) SHA1(f198007375be65e89856d64ee2b3857a18b4eab8) )
	ROM_LOAD16_BYTE( "jojoba-simm3.1", 0x0000001, 0x200000, CRC(9b3406d3) SHA1(54e90cd334d13e2c74305c6b87ebce1365ef3d59) )
	ROM_LOAD16_BYTE( "jojoba-simm3.2", 0x0400000, 0x200000, CRC(f2414997) SHA1(fb89d5784250538ad17fd527267b513afb6eca20) )
	ROM_LOAD16_BYTE( "jojoba-simm3.3", 0x0400001, 0x200000, CRC(954b9c7d) SHA1(0d64d97167d4e669d7e4f3a388f9d5ec1e18ed42) )
	ROM_LOAD16_BYTE( "jojoba-simm3.4", 0x0800000, 0x200000, CRC(625adc1d) SHA1(533d62759ecece10c711d99bfca403e5cba279b5) )
	ROM_LOAD16_BYTE( "jojoba-simm3.5", 0x0800001, 0x200000, CRC(20a70bb4) SHA1(3bd8376304ffc974fb8031eac8bebff27969538c) )
	ROM_LOAD16_BYTE( "jojoba-simm3.6", 0x0c00000, 0x200000, CRC(a10ec5af) SHA1(9b403260e8fbdacaa5369ab79fc05855cc6a6bdb) )
	ROM_LOAD16_BYTE( "jojoba-simm3.7", 0x0c00001, 0x200000, CRC(0bd0de7a) SHA1(1debecda5f282f2a1dd17e887e522a4d00c5dc9d) )
	ROM_LOAD16_BYTE( "jojoba-simm4.0", 0x1000000, 0x200000, CRC(6ea14adc) SHA1(696b2ec66f3c197817a60f507a1b4c78db37f488) )
	ROM_LOAD16_BYTE( "jojoba-simm4.1", 0x1000001, 0x200000, CRC(8f4c42fb) SHA1(363d769b0b066ce139125426d2da6dfa15d1eb28) )
	ROM_LOAD16_BYTE( "jojoba-simm4.2", 0x1400000, 0x200000, CRC(ef0586d1) SHA1(8fcc350da20e3e59fa76fa14e10f2c47233ba9dc) )
	ROM_LOAD16_BYTE( "jojoba-simm4.3", 0x1400001, 0x200000, CRC(93ccc470) SHA1(5d267679e61c0fb592ad5f696d3c06ec1746d0b3) )
	ROM_LOAD16_BYTE( "jojoba-simm4.4", 0x1800000, 0x200000, CRC(3d9ec7d2) SHA1(665b867bab928be183c2006527e55f9b8ec4a271) )
	ROM_LOAD16_BYTE( "jojoba-simm4.5", 0x1800001, 0x200000, CRC(03e66850) SHA1(8478662dc9db20d9a186d315a883bd1cbb5e5000) )
	ROM_LOAD16_BYTE( "jojoba-simm4.6", 0x1c00000, 0x200000, CRC(01606ac3) SHA1(ccc74edeca6abdd86fc1cf42ececa1ea393b3261) )
	ROM_LOAD16_BYTE( "jojoba-simm4.7", 0x1c00001, 0x200000, CRC(36392b87) SHA1(e62080c8461775c1e180400dfb44414679fd0fc1) )
	ROM_LOAD16_BYTE( "jojoba-simm5.0", 0x2000000, 0x200000, CRC(2ef8c60c) SHA1(dea87a73a11b8edd27c3c9c5ab2af295cb5508f9) )
	ROM_LOAD16_BYTE( "jojoba-simm5.1", 0x2000001, 0x200000, CRC(cf7d7ca6) SHA1(b347707b1e5bc71d28b282273f893592e5f9e333) )
	ROM_LOAD16_BYTE( "jojoba-simm5.2", 0x2400000, 0x200000, CRC(b7815bfa) SHA1(0b5a3a2ffe1b3c0ca765dcedc297e78e5928302b) )
	ROM_LOAD16_BYTE( "jojoba-simm5.3", 0x2400001, 0x200000, CRC(9bfec049) SHA1(62cc9a1920047863205544b77344ee18f310f084) )
	ROM_LOAD16_BYTE( "jojoba-simm5.4", 0x2800000, 0x200000, CRC(d167536b) SHA1(e2637d3486f168ce44e0a00413d38960cb86db4c) )
	ROM_LOAD16_BYTE( "jojoba-simm5.5", 0x2800001, 0x200000, CRC(55e7a042) SHA1(c18bda61fa005d9174a27b7b7d324004262a4525) )
	ROM_LOAD16_BYTE( "jojoba-simm5.6", 0x2c00000, 0x200000, CRC(4fb32906) SHA1(3a5965b3197517932c8aa4c07a6ea6a190a338d7) )
	ROM_LOAD16_BYTE( "jojoba-simm5.7", 0x2c00001, 0x200000, CRC(8c8be520) SHA1(c461f3f76a83592b36b29afb316679a7c8972404) )
ROM_END

/* hack of 3rd Strike, from FBNeo: SIMM 1 and two chips of SIMM 5 changed */
ROM_START( sfiii4n )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sfiii3_japan_nocd.29f400.u2", 0x000000, 0x080000, CRC(1edc6366) SHA1(60b4b9adeb030a33059d74fdf03873029e465b52) )

	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* program (SIMM 1-2) */
	ROM_LOAD32_BYTE( "4rd-simm1.0", 0x0000000, 0x200000, CRC(ca97f95e) SHA1(624f1af8c3722c0554a65bfb4f0682ed66a70b55) )
	ROM_LOAD32_BYTE( "4rd-simm1.1", 0x0000001, 0x200000, CRC(5bc7faa6) SHA1(291b6102d8ad7478ea45e73d34ad8d65332beaf0) )
	ROM_LOAD32_BYTE( "4rd-simm1.2", 0x0000002, 0x200000, CRC(ec2eaa29) SHA1(981d452f15da43d4577339ab5799c7561f298d62) )
	ROM_LOAD32_BYTE( "4rd-simm1.3", 0x0000003, 0x200000, CRC(41c4ce7c) SHA1(273f21735200becb0da06ab3e40023918df2a062) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.0", 0x0800000, 0x200000, CRC(06eb969e) SHA1(d89f6a6585b76692d57d337f0f8186398fb056da) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.1", 0x0800001, 0x200000, CRC(e7039f82) SHA1(8e81e66b5a4f45ae14b070a491bde47a6a74499f) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.2", 0x0800002, 0x200000, CRC(645c96f7) SHA1(06d5a54874d4bf100b776131ec9060da209ad037) )
	ROM_LOAD32_BYTE( "sfiii3-simm2.3", 0x0800003, 0x200000, CRC(610efab1) SHA1(bbc21ed6ff6220ff6017a3f02ebd9a341fbc9040) )

	ROM_REGION16_BE( 0x4000000, REGION_USER5, 0 ) /* graphics & samples (SIMM 3-6) */
	ROM_LOAD16_BYTE( "sfiii3-simm3.0", 0x0000000, 0x200000, CRC(7baa1f79) SHA1(3f409df28c24dd7221966b5340d59898ea756b6f) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.1", 0x0000001, 0x200000, CRC(234bf8fe) SHA1(2191781ae4d726cab28de97f27efa4a13f3bdd69) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.2", 0x0400000, 0x200000, CRC(d9ebc308) SHA1(af6a0dca77e5181c9f20533a06760a782c5fd51d) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.3", 0x0400001, 0x200000, CRC(293cba77) SHA1(294604cacdc24261aec4d39e489de91c41fa1758) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.4", 0x0800000, 0x200000, CRC(6055e747) SHA1(3813852c5a4a5355ef739ca8f0913bbd390b984b) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.5", 0x0800001, 0x200000, CRC(499aa6fc) SHA1(5b9b6eab3e99ff3e1d7c1f50b9d8bc6a81f3f8a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.6", 0x0c00000, 0x200000, CRC(6c13879e) SHA1(de189b0b8f42bc7dd89983e62bc2ecb4237b3277) )
	ROM_LOAD16_BYTE( "sfiii3-simm3.7", 0x0c00001, 0x200000, CRC(cf4f8ede) SHA1(e0fb68fcb0e445f824c62fa828d6e1dcd7e3683a) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.0", 0x1000000, 0x200000, CRC(091fd5ba) SHA1(3327ad7c2623c119bf728af717ea2ce3b74673a9) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.1", 0x1000001, 0x200000, CRC(0bca8917) SHA1(b7b284e2f16f46d46bcfaae779b232c5b980924f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.2", 0x1400000, 0x200000, CRC(a0fd578b) SHA1(100c9db9f00ecd88d518076f5a0822e6ac3695b3) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.3", 0x1400001, 0x200000, CRC(4bf8c699) SHA1(2c0b4288b5ebc5e54d9e782dfc39eb8c78fd4c21) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.4", 0x1800000, 0x200000, CRC(137b8785) SHA1(56a579520a8ce2abbf36be57777f024e80474eee) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.5", 0x1800001, 0x200000, CRC(4fb70671) SHA1(9aba83c18cfc099a5ce18793119bff0c2b9c777f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.6", 0x1c00000, 0x200000, CRC(832374a4) SHA1(c84629e32fbf47cb7b5b4ee7555bfc2ac9b3857f) )
	ROM_LOAD16_BYTE( "sfiii3-simm4.7", 0x1c00001, 0x200000, CRC(1c88576d) SHA1(0f039944d0c2305999ed5dbd351c3eb87812dc3b) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.0", 0x2000000, 0x200000, CRC(c67d9190) SHA1(d265475244099d0ec153059986f3445c7bd910a3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.1", 0x2000001, 0x200000, CRC(6cb79868) SHA1(c94237f30e05bfcb2e23945530c812d9e4c73416) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.2", 0x2400000, 0x200000, CRC(df69930e) SHA1(c76b7c559a1d5558138afbc796249efa2f49f6a8) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.3", 0x2400001, 0x200000, CRC(333754e0) SHA1(4c18a569c26524a492ecd6f4c8b3c8e803a077d3) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.4", 0x2800000, 0x200000, CRC(78f6d417) SHA1(a69577cc5399fcf0a24548661168f27f3e7e8e40) )
	ROM_LOAD16_BYTE( "sfiii3-simm5.5", 0x2800001, 0x200000, CRC(8ccad9b1) SHA1(f8bda399f87be2497b7ac39e9661f9863bf4f873) )
	ROM_LOAD16_BYTE( "4rd-simm5.6", 0x2c00000, 0x200000, CRC(e9b61a56) SHA1(9277ac5da4715aa5325ae9f8c055f1e826b4ce54) )
	ROM_LOAD16_BYTE( "4rd-simm5.7", 0x2c00001, 0x200000, CRC(8db3a249) SHA1(db38963ce011535a95f11426c17fb3eeda300917) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.0", 0x3000000, 0x200000, CRC(8da69042) SHA1(fd3d08295342635b2136e48d543c9350d287bb22) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.1", 0x3000001, 0x200000, CRC(1c8c7ac4) SHA1(ac9f8353a4c356ef98aa7c226baba00b01f5a80f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.2", 0x3400000, 0x200000, CRC(a671341d) SHA1(636f4c04962bc1e1ddb29d2e01244b00389b234f) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.3", 0x3400001, 0x200000, CRC(1a990249) SHA1(2acc639e2c0c53bf24096b8620eab090bc25d03b) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.4", 0x3800000, 0x200000, CRC(20cb39ac) SHA1(7d13a0fea1ef719dd2ff77dfb547d53c6023cc9e) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.5", 0x3800001, 0x200000, CRC(5f844b2f) SHA1(564e4934f89ed3b92a4c4874519f8f00f3b48696) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.6", 0x3c00000, 0x200000, CRC(450e8d28) SHA1(885db658132aa27926df617ec2d2a1f38abdbb60) )
	ROM_LOAD16_BYTE( "sfiii3-simm6.7", 0x3c00001, 0x200000, CRC(cc5f4187) SHA1(248ddace21ed4736a56e92f77cc6ad219d7fef0b) )
ROM_END


/* Idle loop skipping speedups */
UINT32 cps3_speedup_ram_address;
UINT32 cps3_speedup_code_address;
	
struct cps3_speedups
{
	const char *name;             /* game driver name */
	const UINT32 ram_address;
	const UINT32 code_address;
};

static const struct cps3_speedups speedup_table[] =
{
	// name        mainram address  code address
	{ "jojojr2",      0x223c0,         0x600065c  },
	{ "jojojr1",   0x223d8,         0x600065c  },
	{ "jojoba",       0x267dc,         0x600065c  },
	{ "jojobanr2",    0x267dc,         0x600065c  },
	{ "jojobajr2",   0x267dc,         0x600065c  },
	{ "sfiii",      0x0cc6c,         0x6000884  },
	{ "sfiiij",     0x0cc6c,         0x6000884  },
	{ "sfiii2",     0x0dfe4,         0x6000884  },
	{ "sfiii2j",    0x0dfe4,         0x6000884  },
	{ "sfiii3",     0x0d794,         0x6000884  },
	{ "sfiii3ur1",    0x0d794,         0x6000884  },
	{ "sfiii3u",   0x0d794,         0x6000884  },
	{ "sfiii4n",   0x0d794,         0x6000884  },
	{ "redearth",  0x2136c,         0x600194e  },
	{ "warzard",   0x2136c,         0x600194e  },
	{ 0 }	// end of table
};

READ32_HANDLER(cps3_speedup_r)
{
//	printf("speedup read %08x %d\n",activecpu_get_pc(), cpu_getexecutingcpu());
	if (cpu_getexecutingcpu()>=0) // prevent cheat search crash..
		if (activecpu_get_pc()==cps3_speedup_code_address) {cpu_spinuntil_int();return cps3_mainram[cps3_speedup_ram_address/4];}
	
	return cps3_mainram[cps3_speedup_ram_address/4];
}

DRIVER_INIT( cps3_speedups )
{
	const char *gamename = Machine->gamedrv->name;
	const struct cps3_speedups *k = &speedup_table[0];

	cps3_speedup_ram_address = 0;
	cps3_speedup_code_address = 0;

	while (k->name)
	{
		if (strcmp(k->name, gamename) == 0)
		{
			// we have a proper key set the global variables to it (so that we can decrypt code in ram etc.)
			cps3_speedup_ram_address = k->ram_address;
			cps3_speedup_code_address = k->code_address;
			break;
		}
		++k;
	}

	//printf("speedup %08x %08x\n",cps3_speedup_ram_address,cps3_speedup_code_address);

	if (cps3_speedup_code_address!=0) memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, cps3_speedup_ram_address+0x02000000, cps3_speedup_ram_address+0x02000003, 0, 0, cps3_speedup_r );
}

void precopy_to_flash(void)
{
	int i;
	for (i=0;i<0x800000;i+=4)
	{
		UINT32* romdata = (UINT32*)decrypted_gamerom;
		UINT32 data;
		UINT8* ptr1 = intelflash_getmemptr(0);
		UINT8* ptr2 = intelflash_getmemptr(1);
		UINT8* ptr3 = intelflash_getmemptr(2);
		UINT8* ptr4 = intelflash_getmemptr(3);

		if (cps3_isSpecial) romdata = (UINT32*)memory_region(REGION_USER4);
		data = romdata[i/4];

		ptr1[i/4] = (data & 0xff000000)>>24;
		ptr2[i/4] = (data & 0x00ff0000)>>16;
		ptr3[i/4] = (data & 0x0000ff00)>>8;
		ptr4[i/4] = (data & 0x000000ff)>>0;
	}

	for (i=0;i<0x800000;i+=4)
	{
		UINT32* romdata = (UINT32*)decrypted_gamerom;
		UINT32 data;
		UINT8* ptr1 = intelflash_getmemptr(4);
		UINT8* ptr2 = intelflash_getmemptr(5);
		UINT8* ptr3 = intelflash_getmemptr(6);
		UINT8* ptr4 = intelflash_getmemptr(7);

		if (cps3_isSpecial) romdata = (UINT32*)memory_region(REGION_USER4);

		data = romdata[(0x800000+i)/4];

		ptr1[i/4] = (data & 0xff000000)>>24;
		ptr2[i/4] = (data & 0x00ff0000)>>16;
		ptr3[i/4] = (data & 0x0000ff00)>>8;
		ptr4[i/4] = (data & 0x000000ff)>>0;
	}

	for (i=0;i<0x800000;i+=4)
	{
		UINT32* romdata = (UINT32*)memory_region(REGION_USER5);
		UINT32 data;
		UINT8* ptr1 = intelflash_getmemptr(8);
		UINT8* ptr2 = intelflash_getmemptr(9);

		data = romdata[(0x800000+i)/4];

		ptr1[i/4] = (data & 0xff000000)>>24;
		ptr2[i/4] = (data & 0x00ff0000)>>16;
		ptr1[i/4] = (data & 0x0000ff00)>>8;
		ptr2[i/4] = (data & 0x000000ff)>>0;
	}

}



DRIVER_INIT (jojoba)
{
	init_cps3crpt();
	init_cps3_speedups();

	precopy_to_flash();
}

DRIVER_INIT( warzard )
{
	/* where is NOCD? */
	/* region 04 usa */
	UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );
	rom[0x1fed8/4]^=0x00000001; // clear region to 0 (invalid)
	rom[0x1fed8/4]^=0x00000061; // region 8 - ASIA NO CD
	rom[0x1fedc/4]^=0x01000000; // region 8 - ASIA NO CD

	init_cps3crpt();
	init_cps3_speedups();

	precopy_to_flash();

}

DRIVER_INIT( sfiii3 )
{
	UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );

	rom[0x1fec8/4]^=0x00000001; // region hack
	rom[0x1fecc/4]^=0x01000000; //  nocd hack

	init_cps3crpt();
	init_cps3_speedups();

	precopy_to_flash();

}

DRIVER_INIT( sfiii4n )
{
	init_cps3crpt();
	init_cps3_speedups();

	precopy_to_flash();
}

DRIVER_INIT( sfiii2 )
{
//  UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );
//  rom[0x1fec8/4]^=0x00000002; // region hack
//  rom[0x1fecc/4]^=0x01000000; //  nocd hack

	init_cps3crpt();
	init_cps3_speedups();

	precopy_to_flash();
}

DRIVER_INIT( sfiii )
{
	UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );

//	rom[0x1fec8/4]^=0x00000001; // region hack
	rom[0x1fecc/4]^=0x01000000; //  nocd hack

	init_cps3crpt();
	init_cps3_speedups();

	precopy_to_flash();

}

DRIVER_INIT( jojo )
{
	init_cps3crpt();
	init_cps3_speedups();

	precopy_to_flash();
}

GAME( 1996, redearth,  0,         cps3, cps3, warzard, ROT0, "Capcom", "Red Earth (Europe 961121)", 0 )
GAME( 1996, warzard,   redearth,  cps3, cps3, warzard, ROT0, "Capcom", "Warzard (Japan 961121)", 0 )

GAME( 1997, sfiii,     0,         cps3, cps3, sfiii,   ROT0, "Capcom", "Street Fighter III: New Generation (Europe 970204)", 0 )
GAME( 1997, sfiiij,    sfiii,     cps3, cps3, sfiii,   ROT0, "Capcom", "Street Fighter III: New Generation (Japan 970204)", 0 )

GAME( 1997, sfiii2,    0,         cps3, cps3, sfiii2,  ROT0, "Capcom", "Street Fighter III 2nd Impact: Giant Attack (USA 970930)", 0 )
GAME( 1997, sfiii2j,   sfiii2,    cps3, cps3, sfiii2,  ROT0, "Capcom", "Street Fighter III 2nd Impact: Giant Attack (Japan 970930)", 0 )

GAME( 1998, jojo,      0,         cps3, cps3, jojo,    ROT0, "Capcom", "JoJo's Venture (Europe 990128)", 0 )
GAME( 1998, jojojr2,   jojo,      cps3, cps3, jojo,    ROT0, "Capcom", "JoJo no Kimyou na Bouken (Japan 981202)", 0 )
GAME( 1998, jojojr1,   jojo,      cps3, cps3, jojo,    ROT0, "Capcom", "JoJo no Kimyou na Bouken (Japan 990108)", 0 )

GAME( 1999, sfiii3,    0,         cps3, cps3, sfiii3,  ROT0, "Capcom", "Street Fighter III 3rd Strike: Fight for the Future (Europe 990608)", 0 )
GAME( 1999, sfiii3ur1, sfiii3,    cps3, cps3, sfiii3,  ROT0, "Capcom", "Street Fighter III 3rd Strike: Fight for the Future (USA 990512)", 0 )
GAME( 1999, sfiii3u,   sfiii3,    cps3, cps3, sfiii3,  ROT0, "Capcom", "Street Fighter III 3rd Strike: Fight for the Future (USA 990608)", 0 )
GAME( 2013, sfiii4n,   sfiii3,    cps3, cps3, sfiii4n, ROT0, "hack",   "Street Fighter III 3rd Strike: Fight for the Future (4rd Arrange Edition 2013)", 0 )

GAME( 1999, jojoba,    0,         cps3, cps3, jojoba,  ROT0, "Capcom", "JoJo's Bizarre Adventure (Europe 991015, NO CD)", 0 )
GAME( 1999, jojobanr2, jojoba,    cps3, cps3, jojoba,  ROT0, "Capcom", "JoJo no Kimyou na Bouken: Mirai e no Isan (Japan 990913, NO CD)", 0 )
GAME( 1999, jojobajr2, jojoba,    cps3, cps3, jojoba,  ROT0, "Capcom", "JoJo no Kimyou na Bouken: Mirai e no Isan (Japan 990913)", 0 )

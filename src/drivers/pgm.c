/* PGM System (c)1997 IGS

Based on Information from ElSemi

A flexible cartridge based platform some would say was designed to compete with
SNK's NeoGeo and Capcom's CPS Hardware systems, despite its age it only uses a
68000 for the main processor and a Z80 to drive the sound, just like the two
previously mentioned systems in that respect..

Resolution is 448x224, 15 bit colour

Sound system is ICS WaveFront 2115 Wavetable midi synthesizer, used in some
actual sound cards (Turtle Beach)

Later games are encrypted.  Latest games (kov2, ddp2) include an arm7
coprocessor with an internal rom and an encrypted external rom.

Roms Contain the Following Data

Pxxxx - 68k Program
Txxxx - TX & BG Graphics (2 formats within the same rom)
Mxxxx - Music samples (8 bit mono 11025Hz)
Axxxx - Colour Data (for sprites)
Bxxxx - Masks & A Rom Colour Indexes (for sprites)

There is no rom for the Z80, the program is uploaded by the 68k

Known Games on this Platform
----------------------------


010x  - 1997  - Oriental Legend
020x  - 1997  - Dragon World 2
030x  - 1998  - The Killing Blade
040x  - 1998  - Dragon World 3
050x  - 1999? - Oriental Legend Super
060x  - 1999  - Knights of Valor, Knights of Valor Plus, Knights of Valor Superheroes
070x  - 1999  - Photo Y2k
080x  - 1999  - Puzzle Star
090x  - 2001  - Puzzli II
100x  - 2001  - Martial Masters

120x  - 2001  - Knights of Valor 2 Plus (9 Dragons?)
130x  - 2001  - DoDonpachi II

0450x - 2002  - Demon Front

---

unknown codes:
Dragon World 2001
Photo Y2k2
The Gladiator

---


Oriental Legend
Oriental Legend Super
Sengoku Senki / Knights of Valour Series
-
Sangoku Senki (c)1999 IGS
Sangoku Senki Super Heroes (c)1999 IGS
Sangoku Senki 2 Knights of Valour (c)2000 IGS
Sangoku Senki Busyou Souha (c)2001 IGS
-
DoDonPachi II (Bee Storm)
Photo Y2K
Photo Y2K II
Martial Masters
The Killing Blade
Dragon World 2
Dragon World 3
Dragon World 2001
Demon Front
The Gladiator
Puzzli II

There is also a single board version of the PGM system used by

Demon Front
Some Later (2002/2003) Cave shooters (Uses a Custom CAVE BIOS)

To Do / Notes:  (Revised December 2005)

Missing Sprite Features
  Zooming (table is uploaded to Video Ram)
  It is possible sprites should be transfered out of RAM with a DMA device.
  Priority?

Protection in Mnny Games
  It is possible to read the Internal Rom of the ASIC27A games with external data
  rom, but not the ones with no External Rom.
  Some of the other protection devices aren't understood at all yet, for example
  the ones on Dragon World 3, Oriental Legend Super and The Killing Blade.
  an ARM core with thumbs support is required to emulate the ASIC27A based games
  even with the internal rom.  At the current time the MAME core does not support
  this, Nebula does.

fix sound comms, several games fail prior to their protection checks due to the
current sound implementation.

Fix IRQs, maybe the protection device generates one of them on DW2 as I believe
it's the only game that needs IRQ4 and Puzzli2 explicitly doesn't want IRQ4 to be
active.

Some dumps are suspicious (orlegend super clones are missing roms, drgw3k sets
might not have the right protection rom)  In many cases the external protection
data roms change with each revision of the game.


General Notes:
--------------

Tit makes more sense to name them kov since the roms are probably the same on the various
boards.  The current sets were taken from taiwan boards incase somebody finds
it not to be the case however due to the previous note.

As we can't dump the internal rom of rhte protection devices (which contain the region
information the only way we can support multiple regions is with a fake dipswitch, this
isn't idea as it gives the false impression that the board contain a region dipswitch)

Dragon World 2 still has strange protection issues, we have to patch the code for now, what
should really happen, it jumps to invalid code, should the protection device cause the 68k
to see valid code there or something?  The English version of Dragon World 2 still appears
to have some problems which the current patching doesn't cover.

kov superheroes uses a different protection chip / different protection commands and doesn't
work, some of the gfx also need redumping to check they're the same as kov, its using invalid
codes for the ones we have (could just be protection tho)


Protection Devices / Co-processors
----------------------------------

IGS used a variety of additional ASIC chips on the game boards, these act as protection and
also give additional power to the board to make up for the limited power of the 68000
processor.  Some protection devices use external data roms, others have internal code only.
Most of these are not emulated correctly. In most cases the protection device supplies the
game region code..

ASIC 3:
    used by:
    different per region, supplies region code
    used by:
    Oriental Legend
    function:

ASIC 12 + ASIC 25
    these seem to be used together
    ASIC 25 appears to perform some kind of bitswap operations
    used by:
    Dragon World 2

ASIC 22 + ASIC 25
    these seem to be used together, ASIC25 has an external software decrypted? data rom
    ASIC 22 might be an updated version of ASIC12 ?
    used by:
    Dragon World 3
    The Killing Blade

ASIC 25 + ASIC 28
    Oriental Legend Super

ASIC 28:
    performs a variety of calculations, quite complex, different per region, supplies region code
    used by:
    Knights of Valour 1 / Plus / Superheroes (plus & superheroes seems to use extra functions, emulation issues reported in places in plus)
    Photo Y2k / Real and Fake (maybe..)
    This could be an ARM chip like the 27A below, but without the support for an external ROM (or nothing uses it..)

ASIC 27A:
    arm9? cpu with (max?) 64kb internal rom (different per game / revision) + optional external data rom
    probably used to give extra power to the system, lots of calculations are offloaded to it
    used by:
    DoDonPachi II
    Knights of Valor 2 / 2 Plus
    Martial Masters
    Demon Front
    Puzzli II
    The Gladiator

there are probably more...

PCB Layout
----------

IGS PCB NO-0133-2 (Main Board)
|-------------------------------------------------------------------------------------|
|   |----------------------------|   |----------|   |----------------------------|    |
|   |----------------------------|   |----------|   |----------------------------|    |
|                                      PGM_T01S.U29  UM61256    SRM2B61256  SRM2B61256|
| |---------|  33.8688MHz   |----------|                        SRM2B61256  SRM2B61256|
| |WAVEFRONT|               |L8A0290   |   UM6164  UM6164                             |
| |ICS2115V |               |IGS023    |                 PGM_P01S.U20              SW2|
| |(PLCC84) |               |(QFP256)  |                                              |
| |         |               |          |                                              |
| |---------|        50MHz  |----------|                                              |
|    UPD6379  PGM_M01S.U18                             |----------|                   |
|VOL                                                   |MC68HC000 |          74HC132  |
|                                                      |FN20      |   20MHz  74HC132  |
|  UPC844C    |------|                                 |(PLCC68)  |                   |
|             |Z80   |                                 |          |          V3021    |
|             |PLCC44|                  PAL            |----------|                   |
|             |------|    |--------|                                      32.768kHz   |-|
|                         |IGS026  |                                                    |
|                         |(QFP144)|           |--------|                              I|
|                         |        |           |IGS026  |                              D|
|                         |--------|           |(QFP144)|                              C|
|TDA1519A    UM61256 UM61256                   |        |                              3|
|                              TD62064         |--------|                              4|
|                                                                          3.6V_BATT    |
|                                                                                     |-|
|              |----|                                           |-----|     SW3       |
|              |    |               J  A  M  M  A               |     | SW1           |
|--------------|    |-------------------------------------------|     |---------------|


IGS PCB NO-0136 (Riser)
|-------------------------------------------------------------------------------------|
|      |---------------------------------|  |---------------------------------|       |
|      |---------------------------------|  |---------------------------------|       |
|                                                                                     |
|      |---------------------------------|  |---------------------------------|       |
|      |---------------------------------|  |---------------------------------|       |
|                                                                                     |
|   |----------------------------|   |----------|   |----------------------------|    |
|---|                            |---|          |---|                            |----|
    |----------------------------|   |----------|   |----------------------------|

Notes:
      All IC's are shown.

      CPU's
      -----
         68HC000FN20 - Motorola 68000 processor, clocked at 20.000MHz (PLCC68)
         Z80         - Zilog Z0840008VSC Z80 processor, clocked at 8.468MHz (PLCC44)

      SOUND
      -----
         ICS2115     - ICS WaveFront ICS2115V Wavetable Midi Synthesizer, clocked at 33.8688MHz (PLCC84)

      RAM
      ---
         SRM2B256 - Epson SRM2B256SLMX55 8K x8 SRAM (x4, SOP28)
         UM6164   - Unicorn Microelectronics UM6164DS-12 8K x8 SRAM (x2, SOJ28)
         UM61256  - Unicorn Microelectronics UM61256FS-15 32K x8 SRAM (x3, SOJ28)

      ROMs
      ----
         PGM_M01S.U18 - 16MBit MASKROM (TSOP48)
         PGM_P01S.U20 - 1MBit  MASKROM (DIP40, socketed, equivalent to 27C1024 EPROM)
         PGM_T01S.U29 - 16MBit MASKROM (SOP44)

      CUSTOM IC's
      -----------
         IGS023 (QFP256)
         IGS026 (x2, QFP144)

      OTHER
      -----
         3.6V_BATT - 3.6V NICad battery, connected to the V3021 RTC
         IDC34     - IDC34 way flat cable plug, doesn't appear to be used for any games. Might be for
                     re-programming some of the custom IC's or on-board surface mounted ROMs?
         PAL       - Atmel ATF16V8B PAL (DIP20)
         SW1       - Push button switch to enter Test Mode
         SW2       - 8 position DIP Switch (for configuration of PCB/game options)
         SW3       - SPDT switch (purpose unknown)
         TD62064   - Toshiba NPN 50V 1.5A Quad Darlinton Switch; for driving coin meters (DIP16)
         TDA1519A  - Philips 2x 6W Stereo Power AMP (SIL9)
         uPD6379   - NEC 2-channel 16-bit D/A converter 10mW typ. (SOIC8)
         uPC844C   - NEC Quad High Speed Wide Band Operational Amplifier (DIP14)
         V3021     - EM Microelectronic-Marin SA Ultra Low Power 32kHz CMOS Real Time Clock (DIP8)
         VOL       - Volume potentiometer

*/

#define PGMLOGERROR 0
#define PGMARM7LOGERROR 0

#include "driver.h"
#include "sound/ics2115.h"
#include "cpu/arm7/arm7core.h"
#include <time.h>
#include "timer.h"

static MACHINE_RESET( killbld );
static MACHINE_RESET( olds );
static MACHINE_RESET( theglad );
UINT16 *pgm_mainram, *pgm_bg_videoram, *pgm_tx_videoram, *pgm_videoregs, *pgm_rowscrollram;
static UINT8 *z80_mainram;
static UINT32 *arm7_shareram;
static UINT32 *svg_shareram[2];	//for 5585G MACHINE
static UINT32 kov2_latchdata_68k_w;
static UINT32 kov2_latchdata_arm_w;
static UINT32* arm_ram, *arm_ram2; // speedups
WRITE16_HANDLER( pgm_tx_videoram_w );
WRITE16_HANDLER( pgm_bg_videoram_w );
VIDEO_START( pgm );
VIDEO_EOF( pgm );
VIDEO_UPDATE( pgm );
void pgm_kov_decrypt(void);
void pgm_kovsh_decrypt(void);
void pgm_kov2_decrypt(void);
void pgm_kov2p_decrypt(void);
void pgm_mm_decrypt(void);
void pgm_dw2_decrypt(void);
void pgm_djlzz_decrypt(void);
void pgm_dw3_decrypt(void);
void pgm_killbld_decrypt(void);
void pgm_pstar_decrypt(void);
void pgm_puzzli2_decrypt(void);
void pgm_ddp2_decrypt(void);
void pgm_dfront_decrypt(void);
void pgm_theglad_decrypt(void);

READ16_HANDLER( pgm_asic3_r );
WRITE16_HANDLER( pgm_asic3_w );
WRITE16_HANDLER( pgm_asic3_reg_w );

READ16_HANDLER (sango_protram_r);
READ16_HANDLER (ASIC28_r16);
WRITE16_HANDLER (ASIC28_w16);

READ16_HANDLER (dw2_d80000_r );


static READ16_HANDLER ( z80_ram_r )
{
	return (z80_mainram[offset*2] << 8)|z80_mainram[offset*2+1];
}

static READ32_HANDLER( arm7_latch_arm_r )
{
	cpunum_set_input_line(2, ARM7_FIRQ_LINE, CLEAR_LINE ); // guess
	
	if (PGMARM7LOGERROR) logerror("ARM7: Latch read: %08x (%08x) (%06x)\n", kov2_latchdata_68k_w, mem_mask, activecpu_get_pc() );
	return kov2_latchdata_68k_w;
}

static WRITE32_HANDLER( arm7_latch_arm_w )
{
	if (PGMARM7LOGERROR) logerror("ARM7: Latch write: %08x (%08x) (%06x)\n", data, mem_mask, activecpu_get_pc() );
	COMBINE_DATA(&kov2_latchdata_arm_w);
}

static READ32_HANDLER( arm7_shareram_r )
{
	if (PGMARM7LOGERROR) logerror("ARM7: ARM7 Shared RAM Read: %04x = %08x (%08x) (%06x)\n", offset << 2, arm7_shareram[offset], mem_mask, activecpu_get_pc() );
	return arm7_shareram[offset];
}

static WRITE32_HANDLER( arm7_shareram_w )
{
	if (PGMARM7LOGERROR) logerror("ARM7: ARM7 Shared RAM Write: %04x = %08x (%08x) (%06x)\n", offset << 2, data, mem_mask, activecpu_get_pc() );
	COMBINE_DATA(&arm7_shareram[offset]);
}

static READ16_HANDLER( arm7_latch_68k_r )
{
	if (PGMARM7LOGERROR) logerror("M68K: Latch read: %04x (%04x) (%06x)\n", kov2_latchdata_arm_w & 0x0000ffff, mem_mask, activecpu_get_pc() );
	return kov2_latchdata_arm_w;
}

static WRITE16_HANDLER( arm7_latch_68k_w )
{
	if (PGMARM7LOGERROR) logerror("M68K: Latch write: %04x (%04x) (%06x)\n", data & 0x0000ffff, mem_mask, activecpu_get_pc() );
	COMBINE_DATA(&kov2_latchdata_68k_w);

    cpunum_set_input_line(2, ARM7_FIRQ_LINE, ASSERT_LINE ); // guess
}

static READ16_HANDLER( arm7_ram_r )
{
	UINT16 *share16 = (UINT16 *)arm7_shareram;
	if (PGMARM7LOGERROR) logerror("M68K: ARM7 Shared RAM Read: %04x = %04x (%08x) (%06x)\n", BYTE_XOR_LE(offset), share16[BYTE_XOR_LE(offset)], mem_mask, activecpu_get_pc() );
	return share16[BYTE_XOR_LE(offset)];
}

static WRITE16_HANDLER( arm7_ram_w )
{
	UINT16 *share16 = (UINT16 *)arm7_shareram;
	if (PGMARM7LOGERROR) logerror("M68K: ARM7 Shared RAM Write: %04x = %04x (%04x) (%06x)\n", BYTE_XOR_LE(offset), data, mem_mask, activecpu_get_pc() );

	COMBINE_DATA(&share16[BYTE_XOR_LE(offset)]);
}

static WRITE16_HANDLER ( z80_ram_w )
{
	int pc = activecpu_get_pc();
	if(ACCESSING_MSB)
		z80_mainram[offset*2] = data >> 8;
	if(ACCESSING_LSB)
		z80_mainram[offset*2+1] = data;

	if(pc != 0xf12 && pc != 0xde2 && pc != 0x100c50 && pc != 0x100b20)
		if (PGMLOGERROR) logerror("Z80: write %04x, %04x @ %04x (%06x)\n", offset*2, data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER ( z80_reset_w )
{
	if (PGMLOGERROR) logerror("Z80: reset %04x @ %04x (%06x)\n", data, mem_mask, activecpu_get_pc());

	if(data == 0x5050) {
		sndti_reset(SOUND_ICS2115, 0);
		cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
		cpunum_set_input_line(1, INPUT_LINE_RESET, PULSE_LINE);
		if(0) {
			FILE *out;
			out = fopen("z80ram.bin", "wb");
			fwrite(z80_mainram, 1, 65536, out);
			fclose(out);
		}
	}
	else
	{
		/* this might not be 100% correct, but several of the games (ddp2, puzzli2 etc. expect the z80 to be turned
           off during data uploads, they write here before the upload */
		cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
	}
}

static WRITE16_HANDLER ( z80_ctrl_w )
{
	if (PGMLOGERROR) logerror("Z80: ctrl %04x @ %04x (%06x)\n", data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER ( m68k_l1_w )
{
	if(ACCESSING_LSB) {
		if (PGMLOGERROR) logerror("SL 1 m68.w %02x (%06x) IRQ\n", data & 0xff, activecpu_get_pc());
		soundlatch_w(0, data);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE );
	}
}

static WRITE8_HANDLER( z80_l3_w )
{
	if (PGMLOGERROR) logerror("SL 3 z80.w %02x (%04x)\n", data, activecpu_get_pc());
	soundlatch3_w(0, data);
}

static void sound_irq(int level)
{
	cpunum_set_input_line(1, 0, level);
}

struct ics2115_interface pgm_ics2115_interface = {
	REGION_SOUND1,
	sound_irq
};

/* Calendar Emulation */

static unsigned char CalVal, CalMask, CalCom=0, CalCnt=0;

static unsigned char bcd(unsigned char data)
{
	return ((data / 10) << 4) | (data % 10);
}

READ16_HANDLER( pgm_calendar_r )
{
	unsigned char calr;
	calr = (CalVal & CalMask) ? 1 : 0;
	CalMask <<= 1;
	return calr;
}

WRITE16_HANDLER( pgm_calendar_w )
{
	static time_t ltime;
	static struct tm *today;

	// initialize the time, otherwise it crashes
	if( !ltime )
	{
		time(&ltime);
		today = localtime(&ltime);
	}

	CalCom <<= 1;
	CalCom |= data & 1;
	++CalCnt;
	if(CalCnt==4)
	{
		CalMask = 1;
		CalVal = 1;
		CalCnt = 0;
		switch(CalCom & 0xf)
		{
			case 1: case 3: case 5: case 7: case 9: case 0xb: case 0xd:
				CalVal++;
				break;

			case 0:
				CalVal=bcd(today->tm_wday); //??
				break;

			case 2:  //Hours
				CalVal=bcd(today->tm_hour);
				break;

			case 4:  //Seconds
				CalVal=bcd(today->tm_sec);
				break;

			case 6:  //Month
				CalVal=bcd(today->tm_mon + 1); //?? not bcd in MVS
				break;

			case 8:
				CalVal=0; //Controls blinking speed, maybe milliseconds
				break;

			case 0xa: //Day
				CalVal=bcd(today->tm_mday);
				break;

			case 0xc: //Minute
				CalVal=bcd(today->tm_min);
				break;

			case 0xe:  //Year
				CalVal=bcd(today->tm_year % 100);
				break;

			case 0xf:  //Load Date
				time(&ltime);
				today = localtime(&ltime);
				break;
		}
	}
}

/*** Memory Maps *************************************************************/

static ADDRESS_MAP_START( pgm_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x01ffff) AM_ROM   /* BIOS ROM */
	AM_RANGE(0x100000, 0x5fffff) AM_ROMBANK(1) /* Game ROM */

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

	AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) /* Main Ram */

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) /* Backgrounds */
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) /* Text Layer */
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) /* Video Regs inc. Zoom Table */

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(pgm_calendar_r, pgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
ADDRESS_MAP_END

static UINT16 *killbld_sharedprotram;

static ADDRESS_MAP_START( killbld_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x01ffff) AM_ROM   /* BIOS ROM */
	AM_RANGE(0x100000, 0x2fffff) AM_ROMBANK(1) /* Game ROM */
	AM_RANGE(0x300000, 0x303fff) AM_RAM AM_BASE(&killbld_sharedprotram) // Shared with protection device

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

	AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) /* Main Ram */

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) /* Backgrounds */
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) /* Text Layer */
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) /* Video Regs inc. Zoom Table */

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(pgm_calendar_r, pgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
ADDRESS_MAP_END

UINT16*olds_sharedprotram;

static ADDRESS_MAP_START( olds_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x01ffff) AM_ROM   /* BIOS ROM */
	AM_RANGE(0x100000, 0x3fffff) AM_ROMBANK(1) /* Game ROM */
	AM_RANGE(0x400000, 0x403fff) AM_RAM AM_BASE(&olds_sharedprotram) // Shared with protection device

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

	AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) /* Main Ram */

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) /* Backgrounds */
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) /* Text Layer */
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) /* Video Regs inc. Zoom Table */

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(pgm_calendar_r, pgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
ADDRESS_MAP_END

static ADDRESS_MAP_START( kov2_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x01ffff) AM_ROM   /* BIOS ROM */
	AM_RANGE(0x100000, 0x5fffff) AM_ROMBANK(1) /* Game ROM */

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

	AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) /* Main Ram */

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) /* Backgrounds */
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) /* Text Layer */
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) /* Video Regs inc. Zoom Table */

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(pgm_calendar_r, pgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
	AM_RANGE(0xd00000, 0xd0ffff) AM_READWRITE(arm7_ram_r, arm7_ram_w) /* ARM7 Shared RAM */
	AM_RANGE(0xd10000, 0xd10001) AM_READWRITE(arm7_latch_68k_r, arm7_latch_68k_w) /* ARM7 Latch */
ADDRESS_MAP_END

static ADDRESS_MAP_START( cavepgm_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x07ffff) AM_ROM   /* larger BIOS ROM */
	AM_RANGE(0xfffffe, 0xffffff) AM_ROMBANK(1) /* Game ROM (unmapped for now, might not even have it) */
	AM_RANGE(0x400000, 0x4fffff) AM_RAM AM_BASE(&olds_sharedprotram) // Shared with protection device

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

	AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) /* Main Ram */

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) /* Backgrounds */
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) /* Text Layer */
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) /* Video Regs inc. Zoom Table */

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(pgm_calendar_r, pgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&z80_mainram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x8000, 0x8003) AM_READWRITE(ics2115_r, ics2115_w)
	AM_RANGE(0x8100, 0x81ff) AM_READWRITE(soundlatch3_r, z80_l3_w)
	AM_RANGE(0x8200, 0x82ff) AM_READWRITE(soundlatch_r, soundlatch_w)
	AM_RANGE(0x8400, 0x84ff) AM_READWRITE(soundlatch2_r, soundlatch2_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( arm7_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00003fff) AM_ROM
	AM_RANGE(0x08000000, 0x083fffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x10000000, 0x100003ff) AM_RAM
	AM_RANGE(0x18000000, 0x1800ffff) AM_RAM /*AM_BASE(&arm_ram) */
	AM_RANGE(0x38000000, 0x38000003) AM_READWRITE(arm7_latch_arm_r, arm7_latch_arm_w) /* 68k Latch */
	AM_RANGE(0x48000000, 0x4800ffff) AM_READWRITE(arm7_shareram_r, arm7_shareram_w) AM_BASE(&arm7_shareram)
	AM_RANGE(0x50000000, 0x500003ff) AM_RAM
ADDRESS_MAP_END

//5585G
unsigned char svg_ram_sel;

static WRITE32_HANDLER( svg_arm7_ram_sel_w )
{
	svg_ram_sel=data&1;
}

static READ32_HANDLER( svg_arm7_shareram_r )
{
	unsigned char ram_sel=svg_ram_sel&1;

	return svg_shareram[ram_sel][offset];
}

static WRITE32_HANDLER( svg_arm7_shareram_w )
{
	unsigned char ram_sel=svg_ram_sel&1;
	COMBINE_DATA(&svg_shareram[ram_sel][offset]);
}

static READ16_HANDLER( svg_m68k_ram_r )
{
	unsigned char ram_sel=(svg_ram_sel&1)^1;
	UINT16 *share16 = (UINT16 *)(svg_shareram[ram_sel]);
	return share16[BYTE_XOR_LE(offset)];
}

static WRITE16_HANDLER( svg_m68k_ram_w )
{
	unsigned char ram_sel=(svg_ram_sel&1)^1;
	UINT16 *share16 = (UINT16 *)(svg_shareram[ram_sel]);
	COMBINE_DATA(&share16[BYTE_XOR_LE(offset)]);
}

static READ16_HANDLER( svg_68k_nmi_r )
{
	return 0;
}

static WRITE16_HANDLER( svg_68k_nmi_w )
{
	cpunum_set_input_line(2, ARM7_FIRQ_LINE, PULSE_LINE);

}

static WRITE16_HANDLER( svg_latch_68k_w )
{
	if (PGMARM7LOGERROR) logerror("M68K: Latch write: %04x (%04x) (%06x)\n", data & 0x0000ffff, mem_mask, activecpu_get_pc() );
	COMBINE_DATA(&kov2_latchdata_68k_w);
}

static ADDRESS_MAP_START( svg_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x07ffff) AM_ROM   /* BIOS ROM */
	AM_RANGE(0x100000, 0x1fffff) AM_ROMBANK(1) /* Game ROM */

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

	AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) /* Main Ram */

	AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) /* Backgrounds */
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) /* Text Layer */
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) /* Video Regs inc. Zoom Table */

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(pgm_calendar_r, pgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
	AM_RANGE(0x500000, 0x51ffff) AM_READWRITE(svg_m68k_ram_r, svg_m68k_ram_w)    /* ARM7 Shared RAM */
	AM_RANGE(0x5c0000, 0x5c0001) AM_READWRITE(svg_68k_nmi_r, svg_68k_nmi_w)      /* ARM7 FIQ */	
	AM_RANGE(0x5c0300, 0x5c0301) AM_READWRITE(arm7_latch_68k_r, svg_latch_68k_w) /* ARM7 Latch */	
ADDRESS_MAP_END

static ADDRESS_MAP_START( svg_arm7_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00003fff) AM_ROM
	AM_RANGE(0x08000000, 0x087fffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x10000000, 0x100003ff) AM_RAM /*AM_BASE(&arm_ram2) */
	AM_RANGE(0x18000000, 0x1803ffff) AM_RAM /*AM_BASE(&arm_ram) */
	AM_RANGE(0x38000000, 0x3801ffff) AM_READWRITE(svg_arm7_shareram_r, svg_arm7_shareram_w)
	AM_RANGE(0x48000000, 0x48000003) AM_READWRITE(arm7_latch_arm_r, arm7_latch_arm_w) /* 68k Latch */
	AM_RANGE(0x40000018, 0x4000001b) AM_WRITE(svg_arm7_ram_sel_w) /* RAM SEL */
	AM_RANGE(0x50000000, 0x500003ff) AM_RAM
ADDRESS_MAP_END


/*** Input Ports *************************************************************/

/* enough for 4 players, the basic dips mapped are listed in the test mode */

static INPUT_PORTS_START( pgm )
	PORT_START_TAG("P1P2")	/* P1P2 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START_TAG("P3P4")	/* P3P4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)

	PORT_START_TAG("Service")	/* Service */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//  PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//  PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?

	PORT_START_TAG("DSW")	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("Region")	/* Region */
	PORT_DIPNAME( 0x0003, 0x0000, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( World ) )
//  PORT_DIPSETTING(      0x0001, DEF_STR( World ) ) // again?
	PORT_DIPSETTING(      0x0002, "Korea" )
	PORT_DIPSETTING(      0x0003, "China" )
INPUT_PORTS_END

static INPUT_PORTS_START( orld105k )
	PORT_INCLUDE ( pgm )

	PORT_MODIFY("Region")
	PORT_DIPNAME( 0x0003, 0x0002, DEF_STR( Unused ) ) // region switch
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) ) // if enabled, game gives
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) ) // "incorrect version" error
INPUT_PORTS_END

INPUT_PORTS_START( sango )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//  PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//  PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* Region - supplied by protection device */
	PORT_DIPNAME( 0x000f, 0x0005, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	PORT_DIPSETTING(      0x0002, "Japan (Alta License)" )
	PORT_DIPSETTING(      0x0003, "Korea" )
	PORT_DIPSETTING(      0x0004, "Hong Kong" )
	PORT_DIPSETTING(      0x0005, DEF_STR( World ) )
INPUT_PORTS_END

INPUT_PORTS_START( olds )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//  PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//  PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

    PORT_START	/* Region - supplied by protection device */
	PORT_DIPNAME( 0x000f, 0x0006, "Region" )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	PORT_DIPSETTING(      0x0002, "China" )
	PORT_DIPSETTING(      0x0003, "Japan" )
	PORT_DIPSETTING(      0x0004, "Korea")
	PORT_DIPSETTING(      0x0005, "Hong Kong" )
	PORT_DIPSETTING(      0x0006, "World" )
INPUT_PORTS_END

INPUT_PORTS_START( killbld )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//  PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//  PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* Region - supplied by protection device */
	PORT_DIPNAME( 0x00ff, 0x0021, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0016, "Taiwan" )
	PORT_DIPSETTING(      0x0017, "China" )
	PORT_DIPSETTING(      0x0018, "Hong Kong" )
	PORT_DIPSETTING(      0x0019, DEF_STR( Japan ) )
//  PORT_DIPSETTING(      0x001a, "1a" ) // invalid
//  PORT_DIPSETTING(      0x001b, "1b" ) // invalid
//  PORT_DIPSETTING(      0x001c, "1c" ) // invalid
//  PORT_DIPSETTING(      0x001d, "1d" ) // invalid
//  PORT_DIPSETTING(      0x001e, "1e" ) // invalid
//  PORT_DIPSETTING(      0x001f, "1f" ) // invalid
	PORT_DIPSETTING(      0x0020, "Korea" )
	PORT_DIPSETTING(      0x0021, DEF_STR( World ) )
INPUT_PORTS_END

INPUT_PORTS_START( photoy2k )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//  PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//  PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* Region - supplied by protection device */
	PORT_DIPNAME( 0x000f, 0x0003, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, "Taiwan" )
	PORT_DIPSETTING(      0x0001, "China" )
	PORT_DIPSETTING(      0x0002, "Japan (Alta License)" )
	PORT_DIPSETTING(      0x0003, DEF_STR( World ))
	PORT_DIPSETTING(      0x0004, "Korea" )
	PORT_DIPSETTING(      0x0005, "Hong Kong" )
INPUT_PORTS_END

INPUT_PORTS_START( ddp2 )
	PORT_START	/* P1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* P2 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3                       )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4                       )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)

	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//  PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//  PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // uused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("RegionHack")	/* Region - supplied by protection device */
	PORT_DIPNAME( 0x00ff, 0x00ff, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	PORT_DIPSETTING(      0x0002, "Japan (Cave License)" )
	PORT_DIPSETTING(      0x0003, "Korea" )
	PORT_DIPSETTING(      0x0004, "Hong Kong" )
	PORT_DIPSETTING(      0x0005, DEF_STR( World ) )
	PORT_DIPSETTING(      0x00ff, "Untouched" ) // don't hack the region
INPUT_PORTS_END

INPUT_PORTS_START( theglad )
	PORT_START	/* P1P2 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	/* P3P4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)

	PORT_START	/* Service */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
//  PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) // test 1p+2p
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) //  what should i use?
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 ) // service 1p+2p
//  PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) // test 3p+4p
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN ) // what should i use?
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 ) // service 3p+4p
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // unused?

	PORT_START	/* DSW */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, "Music" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Free" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Stop" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

    PORT_START_TAG("RegionHack")	/* Region - supplied by protection device */
	PORT_DIPNAME( 0x00ff, 0x00ff, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	PORT_DIPSETTING(      0x0002, "Japan" )
	PORT_DIPSETTING(      0x0003, "Korea" )
	PORT_DIPSETTING(      0x0004, "Hong Kong" )
	PORT_DIPSETTING(      0x0005, "Spanish Territories" )
	PORT_DIPSETTING(      0x0006, DEF_STR( World ) )
	PORT_DIPSETTING(      0x00ff, "Dont Change" ) // don't hack the region
INPUT_PORTS_END


/*** GFX Decodes *************************************************************/

/* we can't decode the sprite data like this because it isn't tile based.  the
   data decoded by pgm32_charlayout was rearranged at start-up */

static const gfx_layout pgm8_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20,16,  28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static const gfx_layout pgm32_charlayout =
{
	32,32,
	RGN_FRAC(1,1),
	5,
	{ 3,4,5,6,7 },
	{ 0  , 8 ,16 ,24 ,32 ,40 ,48 ,56 ,
	  64 ,72 ,80 ,88 ,96 ,104,112,120,
	  128,136,144,152,160,168,176,184,
	  192,200,208,216,224,232,240,248 },
	{ 0*256, 1*256, 2*256, 3*256, 4*256, 5*256, 6*256, 7*256,
	  8*256, 9*256,10*256,11*256,12*256,13*256,14*256,15*256,
	 16*256,17*256,18*256,19*256,20*256,21*256,22*256,23*256,
	 24*256,25*256,26*256,27*256,28*256,29*256,30*256,31*256 },
	 32*256
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pgm8_charlayout,    0x800, 32  }, /* 8x8x4 Tiles */
	{ REGION_GFX2, 0, &pgm32_charlayout,   0x400, 32  }, /* 32x32x5 Tiles */
	{ -1 } /* end of array */
};

/*** Machine Driver **********************************************************/

/* only dragon world 2 NEEDs irq4, Puzzli 2 explicitly doesn't want it, what
   is the source? maybe the protection device? */
static INTERRUPT_GEN( drgw_interrupt ) {
	if( cpu_getiloops() == 0 )
		cpunum_set_input_line(0, 6, HOLD_LINE);
	else
		cpunum_set_input_line(0, 4, HOLD_LINE);
}

static MACHINE_RESET ( pgm )
{
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
}

static MACHINE_DRIVER_START( pgm )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 20000000) /* 20 mhz! verified on real board */
	MDRV_CPU_PROGRAM_MAP(pgm_mem,0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 8468000)
	MDRV_CPU_PROGRAM_MAP(z80_mem, 0)
	MDRV_CPU_IO_MAP(z80_io, 0)
	/* audio CPU */

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(ICS2115, 0)
	MDRV_SOUND_CONFIG(pgm_ics2115_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 5.0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 56*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1200/2)
	MDRV_MACHINE_RESET ( pgm )

	MDRV_VIDEO_START(pgm)
	MDRV_VIDEO_EOF(pgm)
	MDRV_VIDEO_UPDATE(pgm)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( drgw2 )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(drgw_interrupt,2) // needs an extra IRQ, puzzli2 doesn't want this irq!
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( killbld )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(killbld_mem,0)

	MDRV_MACHINE_RESET(killbld)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( olds )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(olds_mem,0)

	MDRV_MACHINE_RESET(olds)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kov2 )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(kov2_mem,0)

	/* protection CPU */
	MDRV_CPU_ADD_TAG("prot", ARM7, 20000000)	// ???
	MDRV_CPU_PROGRAM_MAP(arm7_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cavepgm )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(cavepgm_mem,0)

	/* protection CPU */
//  MDRV_CPU_ADD_TAG("prot", ARM7, 20000000)    // ???
//  MDRV_CPU_PROGRAM_MAP(arm7_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( svg )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(svg_mem,0)

	/* protection CPU */
	MDRV_CPU_ADD_TAG("prot", ARM7, 33333333)
	MDRV_CPU_PROGRAM_MAP(svg_arm7_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( theglad )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(svg_mem,0)

	/* protection CPU */
	MDRV_CPU_ADD_TAG("prot", ARM7, 33333333)
	MDRV_CPU_PROGRAM_MAP(svg_arm7_map, 0)
	
	MDRV_MACHINE_RESET(theglad)
	
MACHINE_DRIVER_END

/*** Init Stuff **************************************************************/

/* This function expands the 32x32 5-bit data into a format which is easier to
   decode in MAME */

static void expand_32x32x5bpp(void)
{
	UINT8 *src    = memory_region       ( REGION_GFX1 );
	UINT8 *dst    = memory_region       ( REGION_GFX2 );
	size_t  srcsize = memory_region_length( REGION_GFX1 );
	int cnt, pix;

	for (cnt = 0; cnt < srcsize/5 ; cnt ++)
	{
		pix =  ((src[0+5*cnt] >> 0)& 0x1f );							  dst[0+8*cnt]=pix;
		pix =  ((src[0+5*cnt] >> 5)& 0x07) | ((src[1+5*cnt] << 3) & 0x18);dst[1+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 2)& 0x1f );		 					  dst[2+8*cnt]=pix;
		pix =  ((src[1+5*cnt] >> 7)& 0x01) | ((src[2+5*cnt] << 1) & 0x1e);dst[3+8*cnt]=pix;
		pix =  ((src[2+5*cnt] >> 4)& 0x0f) | ((src[3+5*cnt] << 4) & 0x10);dst[4+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 1)& 0x1f );							  dst[5+8*cnt]=pix;
		pix =  ((src[3+5*cnt] >> 6)& 0x03) | ((src[4+5*cnt] << 2) & 0x1c);dst[6+8*cnt]=pix;
		pix =  ((src[4+5*cnt] >> 3)& 0x1f );							  dst[7+8*cnt]=pix;
	}
}

/* This function expands the sprite colour data (in the A Roms) from 3 pixels
   in each word to a byte per pixel making it easier to use */

UINT8 *pgm_sprite_a_region;
size_t	pgm_sprite_a_region_allocate;

static void expand_colourdata(void)
{
	UINT8 *src    = memory_region       ( REGION_GFX3 );
	size_t  srcsize = memory_region_length( REGION_GFX3 );
	int cnt;
	size_t	needed = srcsize / 2 * 3;

	/* work out how much ram we need to allocate to expand the sprites into
       and be able to mask the offset */
	pgm_sprite_a_region_allocate = 1;
	while (pgm_sprite_a_region_allocate < needed)
		pgm_sprite_a_region_allocate = pgm_sprite_a_region_allocate <<1;

	pgm_sprite_a_region = auto_malloc (pgm_sprite_a_region_allocate);


	for (cnt = 0 ; cnt < srcsize/2 ; cnt++)
	{
		UINT16 colpack;

		colpack = ((src[cnt*2]) | (src[cnt*2+1] << 8));
		pgm_sprite_a_region[cnt*3+0] = (colpack >> 0 ) & 0x1f;
		pgm_sprite_a_region[cnt*3+1] = (colpack >> 5 ) & 0x1f;
		pgm_sprite_a_region[cnt*3+2] = (colpack >> 10) & 0x1f;
	}
}

static void pgm_basic_init(void)
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	memory_set_bankptr(1,&ROM[0x100000]);

	expand_32x32x5bpp();
	expand_colourdata();
}

static DRIVER_INIT( pgm )
{
	pgm_basic_init();
}

/* Oriental Legend INIT */

static DRIVER_INIT( orlegend )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xC0400e, 0xC0400f, 0, 0, pgm_asic3_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xC04000, 0xC04001, 0, 0, pgm_asic3_reg_w);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xC0400e, 0xC0400f, 0, 0, pgm_asic3_w);
}

static void drgwld2_common_init(void)
{
	pgm_basic_init();
	pgm_dw2_decrypt();
	/*
    Info from Elsemi
    Here is how to "bypass" the dw2 hang protection, it fixes the mode
    select and after failing in the 2nd stage (probably there are other checks
    out there).
    */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xd80000, 0xd80003, 0, 0, dw2_d80000_r);
}

static DRIVER_INIT( drgw2 )
{	/* incomplete? */
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
	drgwld2_common_init();
	/* These ROM patches are not hacks, the protection device
       overlays the normal ROM code, this has been confirmed on a real PCB
       although some addresses may be missing */
	mem16[0x131098/2]=0x4e93;
	mem16[0x13113e/2]=0x4e93;
	mem16[0x1311ce/2]=0x4e93;
}

static DRIVER_INIT( drgw2c )
{
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
	drgwld2_common_init();
	/* These ROM patches are not hacks, the protection device
       overlays the normal ROM code, this has been confirmed on a real PCB
       although some addresses may be missing */
	mem16[0x1303bc/2]=0x4e93;
	mem16[0x130462/2]=0x4e93;
	mem16[0x1304F2/2]=0x4e93;
}

static DRIVER_INIT( drgw2j )
{
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
	drgwld2_common_init();
	/* These ROM patches are not hacks, the protection device
       overlays the normal ROM code, this has been confirmed on a real PCB
       although some addresses may be missing */
	mem16[0x1302C0/2]=0x4e93;
	mem16[0x130366/2]=0x4e93;
	mem16[0x1303F6/2]=0x4e93;
}

static DRIVER_INIT( kov )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
      the protection device provides the region code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4fffff, 0, 0, sango_protram_r);

 	pgm_kov_decrypt();
}

static void kov2_latch_init( void )
{
    kov2_latchdata_68k_w = 0;
    kov2_latchdata_arm_w = 0;
}

static DRIVER_INIT( kov2 )
{
	pgm_basic_init();
	pgm_kov2_decrypt();
	kov2_latch_init();
}

static DRIVER_INIT( kov2p )
{
	pgm_basic_init();
	pgm_kov2p_decrypt();
	kov2_latch_init();
}

static DRIVER_INIT( martmast )
{
	pgm_basic_init();
	pgm_mm_decrypt();
	kov2_latch_init();
}


extern READ16_HANDLER( PSTARS_protram_r );
extern READ16_HANDLER( PSTARS_r16 );
extern WRITE16_HANDLER( PSTARS_w16 );

static DRIVER_INIT( pstar )
{
	pgm_basic_init();
 	pgm_pstar_decrypt();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4f0025, 0, 0, PSTARS_protram_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, PSTARS_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500005, 0, 0, PSTARS_w16);
}



static DRIVER_INIT( kovsh )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
      the protection device provides the region code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4fffff, 0, 0, sango_protram_r);

 	pgm_kovsh_decrypt();
}

static DRIVER_INIT( djlzz )
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
      the protection device provides the region code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4fffff, 0, 0, sango_protram_r);

 	pgm_djlzz_decrypt();
}

static DRIVER_INIT( dw3 )
{
	pgm_basic_init();

//  memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xda0000, 0xdaffff, 0, 0, dw3_prot_r);
//  memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xda0000, 0xdaffff, 0, 0, dw3_prot_w);

 	pgm_dw3_decrypt();
}


/* Killing Blade uses some kind of DMA protection device which can copy data from a data rom.  The
   MCU appears to have an internal ROM as if you remove the data ROM then the shared ram is filled
   with a constant value.
   The device can perform various decryption operations on the data it copies.
*/

static int kb_cmd;
static int kb_reg;
static int kb_ptr;
UINT32 kb_regs[0x10];


static WRITE16_HANDLER( killbld_prot_w )
{
	offset &= 0xf;

	if (offset == 0)
		kb_cmd = data;
	else /*offset==2 */
	{
		logerror("%06X: ASIC25 W CMD %X  VAL %X\n", activecpu_get_pc(),kb_cmd,data);
		if (kb_cmd == 0)
			kb_reg = data;
		else if (kb_cmd == 2)
		{
			if (data == 1)	/* Execute cmd */
			{
				UINT16 cmd = killbld_sharedprotram[0x200/2];

				if (cmd == 0x6d)	//Store values to asic ram
				{
					UINT32 p1 = (killbld_sharedprotram[0x298/2] << 16) | killbld_sharedprotram[0x29a/2];
					UINT32 p2 = (killbld_sharedprotram[0x29c/2] << 16) | killbld_sharedprotram[0x29e/2];

					if ((p2 & 0xffff) == 0x9)	//Set value
					{
						int reg = (p2 >> 16) & 0xffff;
						if (reg & 0x200)
							kb_regs[reg & 0xff] = p1;
					}
					if ((p2 & 0xffff) == 0x6)	//Add value
					{
						int src1 = (p1 >> 16) & 0xff;
						int src2 = (p1 >> 0) & 0xff;
						int dst = (p2 >> 16) & 0xff;
						kb_regs[dst] = kb_regs[src2] - kb_regs[src1];
					}
					if ((p2 & 0xffff) == 0x1)	//Add Imm?
					{
						int reg = (p2 >> 16) & 0xff;
						int imm = (p1 >> 0) & 0xffff;
						kb_regs[reg] += imm;
					}
					if ((p2 & 0xffff) == 0xa)	//Get value
					{
						int reg = (p1 >> 16) & 0xFF;
						killbld_sharedprotram[0x29c/2] = (kb_regs[reg] >> 16) & 0xffff;
						killbld_sharedprotram[0x29e/2] = kb_regs[reg] & 0xffff;
					}
				}
				if(cmd == 0x4f)	//memcpy with encryption / scrambling
				{
					UINT16 src = killbld_sharedprotram[0x290 / 2] >> 1; // ?
					UINT32 dst = killbld_sharedprotram[0x292 / 2];
					UINT16 size = killbld_sharedprotram[0x294 / 2];
					UINT16 mode = killbld_sharedprotram[0x296 / 2];

					UINT16 param;
					
					/*
                    P_SRC =0x300290 (offset from prot rom base)
                    P_DST =0x300292 (words from 0x300000)
                    P_SIZE=0x300294 (words)
                    P_MODE=0x300296

                    Mode 5 direct
                    Mode 6 swap nibbles and bytes
                    1,2,3 table based ops
                    */
	
					param = mode >> 8;
					mode &=0xf;  // what are the other bits?

					if (mode == 0)
					{
						printf("unhandled copy mode %04x!\n", mode);
						// not used by killing blade
						/* plain byteswapped copy */
					}
					if ((mode == 1) || (mode == 2) || (mode == 3))
					{
						/* mode3 applies a xor from a 0x100 byte table to the data being
						   transferred
						   the table is stored at the start of the protection rom.
						   the param used with the mode gives a start offset into the table
						   odd offsets seem to change the table slightly (see rawDataOdd)
					   */
						  					
						/*
						unsigned char rawDataOdd[256] = {
							0xB6, 0xA8, 0xB1, 0x5D, 0x2C, 0x5D, 0x4F, 0xC1,
							0xCF, 0x39, 0x3A, 0xB7,	0x65, 0x85, 0xD9, 0xEE,
							0xDB, 0x7B, 0x5F, 0x81, 0x03, 0x6D, 0xEB, 0x07,
							0x0F, 0xB5, 0x61, 0x59, 0xCD, 0x60, 0x06, 0x21,
							0xA0, 0x99, 0xDD, 0x27,	0x42, 0xD7, 0xC5, 0x5B,
							0x3B, 0xC6, 0x4F, 0xA2, 0x20, 0xF6, 0x61, 0x61,
							0x8C, 0x46, 0x8C, 0xCA, 0xE0, 0x0E, 0x2C, 0xE9,
							0xBA, 0x0F, 0x45, 0x6D,	0x36, 0x1C, 0x18, 0x37,
							0xE7, 0x85, 0x89, 0xA4, 0x94, 0x46, 0x30, 0x9B,
							0xB2, 0xF4, 0x41, 0x55, 0xA5, 0x63, 0x1C, 0xEF,
							0xB7, 0x18, 0xB3, 0xB1,	0xD4, 0x72, 0xA0, 0x1C,
							0x0B, 0x97, 0x02, 0xB6, 0xC5, 0x1F, 0x1B, 0x94,
							0xC3, 0x83, 0xAA, 0xAC, 0xD9, 0x44, 0x09, 0xD7,
							0x6C, 0xDB, 0x07, 0xA9,	0xAD, 0x64, 0x83, 0xF1,
							0x92, 0x09, 0xCD, 0x0E, 0x99, 0x2F, 0xBC, 0xF8,
							0x3C, 0x63, 0x8F, 0x0A, 0x33, 0x03, 0x84, 0x91,
							0x6C, 0xAC, 0x3A, 0x15,	0xCB, 0x67, 0xC7, 0x69,
							0xA1, 0x92, 0x99, 0x74, 0xEE, 0x90, 0x0D, 0xBE,
							0x57, 0x30, 0xD1, 0xBA, 0xE5, 0xDE, 0xFA, 0xD6,
							0x83, 0x8C, 0xE4, 0x43,	0x36, 0x5E, 0xCD, 0x84,
							0x1A, 0x18, 0x31, 0xB9, 0x20, 0x48, 0xE3, 0xA8,
							0x89, 0x32, 0xF0, 0x90, 0x21, 0x80, 0x33, 0xAE,
							0x3C, 0xA6, 0xB8, 0x8C,	0x72, 0x17, 0xD1, 0x0C,
							0x1A, 0x29, 0xFA, 0x38, 0x87, 0xC9, 0x6E, 0xC7,
							0x05, 0xDE, 0x85, 0x6E, 0x92, 0x7E, 0xD4, 0xED,
							0x5C, 0xD3, 0x03, 0xD4,	0xFE, 0xCB, 0x6C, 0x19,
							0x7A, 0x83, 0x79, 0x5B, 0xF6, 0x71, 0xBA, 0xF4,
							0x37, 0x53, 0xC9, 0xC1, 0xDE, 0xDB, 0xDE, 0xB1,
							0x64, 0x17, 0x31, 0x0E,	0xD7, 0xA2, 0x13, 0x8E,
							0x52, 0x8D, 0xCB, 0x19, 0x3D, 0x0B, 0x31, 0x58,
							0x4A, 0xDE, 0x0C, 0x01, 0x2B, 0x85, 0x2D, 0xE5,
							0x13, 0x22, 0x48, 0xB6,	0xF3, 0x2D, 0x00, 0x9A
						};
						*/
						int x;
						UINT16 *PROTROM = (UINT16*)memory_region(REGION_USER1);

						for (x = 0; x < size; x++)
						{
				
							UINT16 dat2 = PROTROM[src + x];

							UINT8 extraoffset = param&0xfe; // the lowest bit changed the table addressing in tests, see 'rawDataOdd' table instead.. it's still related to the main one, not identical
							UINT8* dectable = (UINT8*)memory_region(REGION_USER1);//rawDataEven; // the basic decryption table is at the start of the mcu data rom! at least in killbld
							UINT16 extraxor = ((dectable[((x*2)+0+extraoffset)&0xff]) << 8) | (dectable[((x*2)+1+extraoffset)&0xff] << 0);
							
							dat2 = ((dat2 & 0x00ff)<<8) | ((dat2 & 0xff00)>>8);
							
							if (mode==3) dat2 ^= extraxor;						
							if (mode==2) dat2 += extraxor;
							if (mode==1) dat2 -= extraxor;
							
							/*if (dat!=dat2)
								printf("Mode %04x Param %04x Mismatch %04x %04x\n", mode, param, dat, dat2);
							*/

							killbld_sharedprotram[dst + x] = dat2;
						}
	
						/* hack, patches out some additional security checks... we need to emulate them instead!
						  they occur before it displays the disclaimer, so if you remove the overlay patches it will display
						  the highscore table before coming up with this error... */
						if ((mode==3) && (param==0x54) && (src*2==0x2120) && (dst*2==0x2600)) killbld_sharedprotram[0x2600 / 2] = 0x4e75;
						
					}
					if (mode == 4)
					{
						printf("unhandled copy mode %04x!\n", mode);
						/* not used by killing blade */
						/* looks almost like a fixed value xor, but isn't */
					}
					else if (mode == 5)
					{
						/* mode 5 seems to be a straight copy */
						int x;
						UINT16 *PROTROM = (UINT16*)memory_region(REGION_USER1);
						for (x = 0; x < size; x++)
						{
							UINT16 dat = PROTROM[src + x];


							killbld_sharedprotram[dst + x] = dat;
						}
					}
					else if (mode == 6)
					{
						/* mode 6 seems to swap bytes and nibbles */
						int x;
						UINT16 *PROTROM = (UINT16*)memory_region(REGION_USER1);
						for (x = 0; x < size; x++)
						{
							UINT16 dat = PROTROM[src + x];

							dat = ((dat & 0xf000) >> 12)|
								  ((dat & 0x0f00) >> 4)|
								  ((dat & 0x00f0) << 4)|
								  ((dat & 0x000f) << 12);

							killbld_sharedprotram[dst + x] = dat;
						}
					}
					else if (mode == 7)
					{
						printf("unhandled copy mode %04x!\n", mode);
						/* not used by killing blade */
						/* weird mode, the params get left in memory? - maybe it's a NOP? */
					}
					else
					{
						printf("unhandled copy mode %04x!\n", mode);
						/* not used by killing blade */
						/* invalid? */

					}

				}
				kb_reg++;
			}
		}
		else if (kb_cmd == 4)
			kb_ptr = data;
		else if (kb_cmd == 0x20)
			kb_ptr++;
	}
}

static READ16_HANDLER( killbld_prot_r )
{

	UINT16 res ;

	offset &= 0xf;
	res = 0;

	if (offset == 1)
	{
		if (kb_cmd == 1)
		{
			res = kb_reg & 0x7f;
		}
		else if (kb_cmd == 5)
		{
			UINT32 protvalue = 0x89911400 | readinputport(4);
			res = (protvalue >> (8 * (kb_ptr - 1))) & 0xff;
		}
	}
	logerror("%06X: ASIC25 R CMD %X  VAL %X\n", activecpu_get_pc(),kb_cmd,res);
	return res;
}

static MACHINE_RESET( killbld )
{
	int i;

	machine_reset_pgm();

	/* fill the protection ram with a5 */
	for (i = 0; i < 0x4000/2; i++)
		killbld_sharedprotram[i] = 0xa5a5;

}


/* ASIC025/ASIC022 don't provide rom patches like the DW2 protection does, the previous dump was bad :-) */
static DRIVER_INIT( killbld )
{

	pgm_basic_init();
	pgm_killbld_decrypt();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xd40000, 0xd40003, 0, 0, killbld_prot_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xd40000, 0xd40003, 0, 0, killbld_prot_w);
	
	kb_cmd = 0;
	kb_reg = 0;
	kb_ptr = 0;
	memset(kb_regs, 0, sizeof(kb_regs));
}

static WRITE32_HANDLER( ddp2_arm_region_w )
{	
    int pc = activecpu_get_pc();
	int regionhack = readinputportbytag("RegionHack");
	if (pc==0x0174 && regionhack != 0xff) data = (data & 0x0000ffff) | (regionhack << 16);
	COMBINE_DATA(&arm7_shareram[0x0]);
}

static READ32_HANDLER( ddp2_speedup_r )
{
	int pc = activecpu_get_pc();
	UINT32 data = arm_ram[0x300c/4];

	if (pc==0x080109b4)
	{
		/* if we've hit the loop where this is read and both values are 0 then the only way out is an interrupt */
//		int r4 = (cpu_get_reg(&space->device(), ARM7_R4));
		int r4 = activecpu_get_reg(ARM7_R4);
		r4 += 0xe;
		
		if (r4==0x18002f9e)
		{
			UINT32 data2 = arm_ram[0x2F9C/4]&0xffff0000;
			if ((data==0x00000000) && (data2==0x00000000)) cpu_spinuntil_int();
		}
	}

	return data;
}

static DRIVER_INIT( ddp2 )
{
	pgm_basic_init();
	pgm_ddp2_decrypt();
	kov2_latch_init();
 
	// we only have a Japan internal ROM dumped for now, allow us to override that for debugging purposes.
//	machine.device("prot")->memory().space(AS_PROGRAM)->install_legacy_write_handler(0x48000000, 0x48000003, FUNC(ddp2_arm_region_w));
	memory_install_write32_handler(2, ADDRESS_SPACE_PROGRAM, 0x48000000, 0x48000003, 0, 0, ddp2_arm_region_w);	// prot = CPU3 in this driver so it's cpu 0 1 "2" correct.??
	
//	memory_install_read32_handler(2, ADDRESS_SPACE_PROGRAM, 0x1800300c, 0x1800300f, 0, 0, ddp2_speedup_r); // 16 or 32.??
}

static DRIVER_INIT( puzzli2 )
{
	/* this protection emulation is wrong
     it uses an arm with no external rom
     an acts in a similar way to kov etc. */

	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);

	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_r16);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500003, 0, 0, ASIC28_w16);

	/* 0x4f0000 - ? is actually ram shared with the protection device,
      the protection device provides the region code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f0000, 0x4fffff, 0, 0, sango_protram_r);

 	pgm_puzzli2_decrypt();

	/* protection related? */
	mem16[0x1548ec/2]=0x4e71;
	mem16[0x1548fc/2]=0x4e71;
	mem16[0x1549FA/2]=0x4e71;
	mem16[0x154A0A/2]=0x4e71;
	mem16[0x15496A/2]=0x4e71;
	mem16[0x14cee0/2]=0x4e71;
	mem16[0x1268c0/2]=0x4e71;
	mem16[0x1268c2/2]=0x4e71;
	mem16[0x1268c4/2]=0x4e71;
	mem16[0x154948/2]=0x4e71;
	mem16[0x13877a/2]=0x662c;

	/* patch irq4 vector (irq4 should be disabled on this game? how?) */
//  mem16[0x100070/2]=0x0012;
//  mem16[0x100072/2]=0x5D78;
}

static MACHINE_RESET( olds )
{
	machine_reset_pgm();
	/*	written by protection device
	there seems to be an auto-dma that writes from $401000-402573?
	*/
	olds_sharedprotram[0x1000/2] = 0x4749; /* 'IGS.28' */
	olds_sharedprotram[0x1002/2] = 0x2E53;
	olds_sharedprotram[0x1004/2] = 0x3832;

	olds_sharedprotram[0x3064/2] = 0xB315; /* crc? */
}

/* olds */
int           olds_cmd;
int           olds_reg;
int           olds_ptr;
UINT16        olds_bs;
UINT16        olds_cmd3;
UINT16        olds_prot_hold;
UINT16        olds_prot_hilo;
UINT16        olds_prot_hilo_select;
const UINT8  *olds_prot_hilo_source2;
UINT32 olds_prot_addr( UINT16 addr );
UINT32 olds_read_reg( UINT16 addr );
void olds_write_reg( UINT16 addr, UINT32 val );
void IGS028_do_dma(UINT16 src, UINT16 dst, UINT16 size, UINT16 mode);
void olds_protection_calculate_hilo();
void olds_protection_calculate_hold(int y, int z);
READ16_HANDLER( olds_r );
WRITE16_HANDLER( olds_w );
/* tables are xored by table at $1998dc
   tables are the same as drgw3 and drgw2
*/
static const UINT8 olds_source_data[8][0xec] = /* table addresses $2951CA */
{
	{ /* region 0, unused... */
		0,
	},
	{ /* region 1, $1A669A */
		0x67, 0x51, 0xf3, 0x19, 0xa0, 0x11, 0xe1, 0x11, 0x10, 0xee, 0xe3, 0xf6, 0xbe, 0x81, 0x35, 0xe3,
		0xfb, 0xe6, 0xef, 0xdf, 0x61, 0x01, 0xfa, 0x22, 0x5d, 0x43, 0x01, 0xa5, 0x3b, 0x17, 0xd4, 0x74,
		0xf0, 0xf4, 0xf3, 0x43, 0xb5, 0x19, 0x04, 0xd5, 0x84, 0xce, 0x87, 0xfe, 0x35, 0x3e, 0xc4, 0x3c,
		0xc7, 0x85, 0x2a, 0x33, 0x00, 0x86, 0xd0, 0x4d, 0x65, 0x4b, 0xf9, 0xe9, 0xc0, 0xba, 0xaa, 0x77,
		0x9e, 0x66, 0xf6, 0x0f, 0x4f, 0x3a, 0xb6, 0xf1, 0x64, 0x9a, 0xe9, 0x25, 0x1a, 0x5f, 0x22, 0xa3,
		0xa2, 0xbf, 0x4b, 0x77, 0x3f, 0x34, 0xc9, 0x6e, 0xdb, 0x12, 0x5c, 0x33, 0xa5, 0x8b, 0x6c, 0xb1,
		0x74, 0xc8, 0x40, 0x4e, 0x2f, 0xe7, 0x46, 0xae, 0x99, 0xfc, 0xb0, 0x55, 0x54, 0xdf, 0xa7, 0xa1,
		0x0f, 0x5e, 0x49, 0xcf, 0x56, 0x3c, 0x90, 0x2b, 0xac, 0x65, 0x6e, 0xdb, 0x58, 0x3e, 0xc9, 0x00,
		0xae, 0x53, 0x4d, 0x92, 0xfa, 0x40, 0xb2, 0x6b, 0x65, 0x4b, 0x90, 0x8a, 0x0c, 0xe2, 0xa5, 0x9a,
		0xd0, 0x20, 0x29, 0x55, 0xa4, 0x44, 0xac, 0x51, 0x87, 0x54, 0x53, 0x34, 0x24, 0x4b, 0x81, 0x67,
		0x34, 0x4c, 0x5f, 0x31, 0x4e, 0xf2, 0xf1, 0x19, 0x18, 0x1c, 0x34, 0x38, 0xe1, 0x81, 0x17, 0xcf,
		0x24, 0xb9, 0x9a, 0xcb, 0x34, 0x51, 0x50, 0x59, 0x44, 0xb1, 0x0b, 0x50, 0x95, 0x6c, 0x48, 0x7e,
		0x14, 0xa4, 0xc6, 0xd9, 0xd3, 0xa5, 0xd6, 0xd0, 0xc5, 0x97, 0xf0, 0x45, 0xd0, 0x98, 0x51, 0x91,
		0x9f, 0xa3, 0x43, 0x51, 0x05, 0x90, 0xee, 0xca, 0x7e, 0x5f, 0x72, 0x53, 0xb1, 0xd3, 0xaf, 0x36,
		0x08, 0x75, 0xb0, 0x9b, 0xe0, 0x0d, 0x43, 0x88, 0xaa, 0x27, 0x44, 0x11
	},
	{ /* region 2, $19A5F8 */
		0xf9, 0x19, 0xf3, 0x09, 0xa0, 0x11, 0xe0, 0x11, 0x10, 0x22, 0xfd, 0x8e, 0xd3, 0xc8, 0x31, 0x67,
		0xc0, 0x10, 0x3c, 0xc2, 0x03, 0xf2, 0x6a, 0x0a, 0x54, 0x49, 0xca, 0xb5, 0x4b, 0xe0, 0x94, 0xe8,
		0x8d, 0xc8, 0x90, 0xee, 0x6b, 0x6f, 0xfa, 0x09, 0x76, 0x84, 0x6f, 0x55, 0xd1, 0x94, 0xca, 0x9c,
		0xe1, 0x22, 0xc6, 0x02, 0xb5, 0x8c, 0xf9, 0x3a, 0x52, 0x10, 0xf0, 0x22, 0xe4, 0x11, 0x15, 0x73,
		0x5e, 0x9e, 0xde, 0xc4, 0x5a, 0xbd, 0xa3, 0x89, 0xe7, 0x9b, 0x95, 0x5d, 0x75, 0xf6, 0xc3, 0x9f,
		0xe4, 0xcf, 0x65, 0x73, 0x90, 0xd0, 0x75, 0x56, 0xfa, 0xcc, 0xe4, 0x3e, 0x9c, 0x41, 0x81, 0x62,
		0xb1, 0xd3, 0x28, 0xbd, 0x6c, 0xed, 0x60, 0x28, 0x27, 0xee, 0xf2, 0xa1, 0xb4, 0x2c, 0x6c, 0xbb,
		0x42, 0xd7, 0x1d, 0x62, 0xc0, 0x33, 0x7d, 0xf9, 0xe4, 0x5c, 0xe2, 0x41, 0xa4, 0x1c, 0x98, 0xa1,
		0x87, 0x95, 0xad, 0x61, 0x56, 0x96, 0x40, 0x08, 0x6b, 0xe2, 0x4b, 0x95, 0x7b, 0x1b, 0xd8, 0x64,
		0xb3, 0xee, 0x9d, 0x79, 0x69, 0xea, 0x5d, 0xcf, 0x01, 0x91, 0xea, 0x3f, 0x70, 0x29, 0xdc, 0xe0,
		0x08, 0x20, 0xbf, 0x46, 0x90, 0xa8, 0xfc, 0x29, 0x14, 0xd1, 0x0d, 0x20, 0x79, 0xd2, 0x2c, 0xe9,
		0x52, 0xa6, 0x8c, 0xbd, 0xa3, 0x3e, 0x88, 0x2d, 0xb8, 0x4e, 0xf2, 0x74, 0x50, 0xcc, 0x12, 0xde,
		0xd3, 0x5a, 0xa4, 0x7b, 0xa2, 0x8d, 0x91, 0x68, 0x12, 0x0c, 0x9c, 0xb9, 0x6d, 0x26, 0x66, 0x60,
		0xc3, 0x6d, 0xd0, 0x11, 0x33, 0x05, 0x1d, 0xa8, 0xb6, 0x51, 0xe6, 0xe0, 0x58, 0x61, 0x74, 0x37,
		0xcc, 0x3a, 0x4d, 0x6a, 0x0a, 0x09, 0x71, 0xe3, 0x7e, 0xa5, 0x3b, 0xe9
	},
	{ /* region 3, $1F9508 */
		0x73, 0x59, 0xf3, 0x09, 0xa0, 0x11, 0xe1, 0x11, 0x10, 0x55, 0x18, 0x0d, 0xe8, 0x29, 0x2d, 0x04,
		0x85, 0x39, 0x88, 0xbe, 0x8b, 0xcb, 0xd9, 0x0b, 0x32, 0x36, 0x94, 0xac, 0x74, 0xc3, 0x3b, 0x5d,
		0x2a, 0x83, 0x46, 0xb3, 0x3a, 0xac, 0xd8, 0x55, 0x68, 0x21, 0x57, 0xab, 0x6e, 0xd1, 0xd0, 0xfc,
		0xe2, 0xbe, 0x63, 0xd0, 0x6b, 0x79, 0x23, 0x40, 0x58, 0xd4, 0xe7, 0x73, 0x22, 0x67, 0x7f, 0x88,
		0x05, 0xbd, 0xdf, 0x7a, 0x65, 0x41, 0x90, 0x3a, 0x52, 0x83, 0x28, 0xae, 0xe9, 0x8e, 0x65, 0x82,
		0x0e, 0xdf, 0x98, 0x88, 0xe1, 0x86, 0x21, 0x3e, 0x1a, 0x87, 0x6d, 0x62, 0x7a, 0xf6, 0xaf, 0x2c,
		0xd5, 0xc5, 0x10, 0x2d, 0xa9, 0xda, 0x93, 0xa1, 0x9b, 0xc7, 0x35, 0xd4, 0x15, 0x78, 0x18, 0xd5,
		0x75, 0x6a, 0xd7, 0xdb, 0x12, 0x2a, 0x6a, 0xc8, 0x36, 0x53, 0x57, 0xa6, 0xf0, 0x13, 0x67, 0x43,
		0x79, 0xf0, 0x0e, 0x49, 0xb1, 0xec, 0xcd, 0xa4, 0x8a, 0x61, 0x06, 0xb9, 0xea, 0x53, 0xf2, 0x47,
		0x7d, 0xd6, 0xf8, 0x9d, 0x2e, 0xaa, 0x27, 0x35, 0x61, 0xce, 0x9b, 0x63, 0xbc, 0x07, 0x51, 0x5a,
		0xc2, 0x0d, 0x39, 0x42, 0xd2, 0x5e, 0x21, 0x20, 0x10, 0xa0, 0xe5, 0x08, 0xf7, 0x3d, 0x28, 0x04,
		0x99, 0x93, 0x97, 0xaf, 0xf9, 0x12, 0xc0, 0x01, 0x2d, 0xea, 0xf3, 0x98, 0x0b, 0x46, 0xc2, 0x26,
		0x93, 0x10, 0x69, 0x1d, 0x71, 0x8e, 0x33, 0x00, 0x5e, 0x80, 0x2f, 0x47, 0x0a, 0xcc, 0x94, 0x16,
		0xe7, 0x37, 0x45, 0xd0, 0x61, 0x79, 0x32, 0x86, 0x08, 0x2a, 0x5b, 0x55, 0xfe, 0xee, 0x52, 0x38,
		0xaa, 0x18, 0xe9, 0x39, 0x1a, 0x1e, 0xb8, 0x26, 0x6b, 0x3d, 0x4b, 0xa9
	},
	{ /* region 4, $1CA7B8 */
		0x06, 0x01, 0xf3, 0x39, 0xa0, 0x11, 0xf0, 0x11, 0x10, 0x6f, 0x32, 0x8b, 0xfd, 0x89, 0x29, 0xa0,
		0x4a, 0x62, 0xed, 0xa1, 0x2d, 0xa4, 0x49, 0xf2, 0x10, 0x3c, 0x77, 0xa3, 0x84, 0x8d, 0xfa, 0xd1,
		0xc6, 0x57, 0xe2, 0x78, 0xef, 0xe9, 0xb6, 0xa1, 0x5a, 0xbd, 0x3f, 0x02, 0x0b, 0x28, 0xd6, 0x76,
		0xfc, 0x5b, 0x19, 0x9f, 0x21, 0x66, 0x4c, 0x2d, 0x45, 0x99, 0xde, 0xab, 0x46, 0xbd, 0xe9, 0x84,
		0xc4, 0xdc, 0xc7, 0x30, 0x70, 0xdd, 0x64, 0xea, 0xbc, 0x6b, 0xd3, 0xe6, 0x45, 0x3f, 0x07, 0x7e,
		0x50, 0xef, 0xb2, 0x84, 0x33, 0x3c, 0xcc, 0x3f, 0x39, 0x5b, 0xf5, 0x6d, 0x71, 0xc5, 0xdd, 0xf5,
		0xf9, 0xd0, 0xf7, 0x9c, 0xe6, 0xc7, 0xad, 0x1b, 0x29, 0xb9, 0x90, 0x08, 0x75, 0xc4, 0xc3, 0xef,
		0xa8, 0xfc, 0xab, 0x55, 0x7c, 0x21, 0x57, 0x97, 0x87, 0x4a, 0xcb, 0x0c, 0x56, 0x0a, 0x4f, 0xcb,
		0x52, 0x33, 0x87, 0x31, 0xf3, 0x43, 0x5b, 0x41, 0x90, 0xf8, 0xc0, 0xdd, 0x5a, 0xa4, 0x26, 0x2a,
		0x60, 0xa5, 0x6d, 0xda, 0xf2, 0x6a, 0xf0, 0xb3, 0xda, 0x25, 0x33, 0x87, 0x22, 0xe4, 0xac, 0xd3,
		0x96, 0xe0, 0x99, 0x3e, 0xfb, 0x14, 0x45, 0x17, 0x25, 0x56, 0xbe, 0xef, 0x8f, 0x8e, 0x3d, 0x1e,
		0xc7, 0x99, 0xa2, 0xa1, 0x50, 0xfe, 0xdf, 0xd4, 0xa1, 0x87, 0xf4, 0xd5, 0xde, 0xa6, 0x8c, 0x6d,
		0x6c, 0xde, 0x47, 0xbe, 0x59, 0x8f, 0xd4, 0x97, 0xc3, 0xf4, 0xda, 0xbb, 0xa6, 0x73, 0xa9, 0xcb,
		0xf2, 0x01, 0xb9, 0x90, 0x8f, 0xed, 0x60, 0x64, 0x40, 0x1c, 0xb6, 0xc9, 0xa5, 0x7c, 0x17, 0x52,
		0x6f, 0xdc, 0x6d, 0x08, 0x2a, 0x1a, 0xe6, 0x68, 0x3f, 0xd4, 0x42, 0x69
	},
	{ /* region 5, $1A19FA */
		0x7f, 0x41, 0xf3, 0x39, 0xa0, 0x11, 0xf1, 0x11, 0x10, 0xa2, 0x4c, 0x23, 0x13, 0xe9, 0x25, 0x3d,
		0x0f, 0x72, 0x3a, 0x9d, 0xb5, 0x96, 0xd1, 0xda, 0x07, 0x29, 0x41, 0x9a, 0xad, 0x70, 0xba, 0x46,
		0x63, 0x2b, 0x7f, 0x3d, 0xbe, 0x40, 0xad, 0xd4, 0x4c, 0x73, 0x27, 0x58, 0xa7, 0x65, 0xdc, 0xd6,
		0xfd, 0xde, 0xb5, 0x6e, 0xd6, 0x6c, 0x75, 0x1a, 0x32, 0x45, 0xd5, 0xe3, 0x6a, 0x14, 0x6d, 0x80,
		0x84, 0x15, 0xaf, 0xcc, 0x7b, 0x61, 0x51, 0x82, 0x40, 0x53, 0x7f, 0x38, 0xa0, 0xd6, 0x8f, 0x61,
		0x79, 0x19, 0xe5, 0x99, 0x84, 0xd8, 0x78, 0x27, 0x3f, 0x16, 0x97, 0x78, 0x4f, 0x7b, 0x0c, 0xa6,
		0x37, 0xdb, 0xc6, 0x0c, 0x24, 0xb4, 0xc7, 0x94, 0x9d, 0x92, 0xd2, 0x3b, 0xd5, 0x11, 0x6f, 0x0a,
		0xdb, 0x76, 0x66, 0xe7, 0xcd, 0x18, 0x2b, 0x66, 0xd8, 0x41, 0x40, 0x58, 0xa2, 0x01, 0x1e, 0x6d,
		0x44, 0x75, 0xe7, 0x19, 0x4f, 0xb2, 0xe8, 0xc4, 0x96, 0x77, 0x62, 0x02, 0xc9, 0xdc, 0x59, 0xf3,
		0x43, 0x8d, 0xc8, 0xfe, 0x9e, 0x2a, 0xba, 0x32, 0x3b, 0x62, 0xe3, 0x92, 0x6e, 0xc2, 0x08, 0x4d,
		0x51, 0xcd, 0xf9, 0x3a, 0x3e, 0xc9, 0x50, 0x27, 0x21, 0x25, 0x97, 0xd7, 0x0e, 0xf8, 0x39, 0x38,
		0xf5, 0x86, 0x94, 0x93, 0xbf, 0xeb, 0x18, 0xa8, 0xfc, 0x24, 0xf5, 0xf9, 0x99, 0x20, 0x3d, 0xcd,
		0x2c, 0x94, 0x25, 0x79, 0x28, 0x77, 0x8f, 0x2f, 0x10, 0x69, 0x86, 0x30, 0x43, 0x01, 0xd7, 0x9a,
		0x17, 0xe3, 0x47, 0x37, 0xbd, 0x62, 0x75, 0x42, 0x78, 0xf4, 0x2b, 0x57, 0x4c, 0x0a, 0xdb, 0x53,
		0x4d, 0xa1, 0x0a, 0xd6, 0x3a, 0x16, 0x15, 0xaa, 0x2c, 0x6c, 0x39, 0x42
	},
	{ /* region 6, $2937EA */
		0x12, 0x09, 0xf3, 0x29, 0xa0, 0x11, 0xf0, 0x11, 0x10, 0xd5, 0x66, 0xa1, 0x28, 0x4a, 0x21, 0xc0,
		0xd3, 0x9b, 0x86, 0x80, 0x57, 0x6f, 0x41, 0xc2, 0xe4, 0x2f, 0x0b, 0x91, 0xbd, 0x3a, 0x7a, 0xba,
		0x00, 0xe5, 0x35, 0x02, 0x74, 0x7d, 0x8b, 0x21, 0x57, 0x10, 0x0f, 0xae, 0x44, 0xbb, 0xe2, 0x37,
		0x18, 0x7b, 0x52, 0x3d, 0x8c, 0x59, 0x9e, 0x20, 0x1f, 0x0a, 0xcc, 0x1c, 0x8e, 0x6a, 0xd7, 0x95,
		0x2b, 0x34, 0xb0, 0x82, 0x6d, 0xfd, 0x25, 0x33, 0xaa, 0x3b, 0x2b, 0x70, 0x15, 0x87, 0x31, 0x5d,
		0xbb, 0x29, 0x19, 0x95, 0xd5, 0x8e, 0x24, 0x28, 0x5e, 0xd0, 0x20, 0x83, 0x46, 0x4a, 0x21, 0x70,
		0x5b, 0xcd, 0xae, 0x7b, 0x61, 0xa1, 0xfa, 0xf4, 0x2b, 0x84, 0x15, 0x6e, 0x36, 0x5d, 0x1b, 0x24,
		0x0f, 0x09, 0x3a, 0x61, 0x38, 0x0f, 0x18, 0x35, 0x11, 0x38, 0xb4, 0xbd, 0xee, 0xf7, 0xec, 0x0f,
		0x1d, 0xb7, 0x48, 0x01, 0xaa, 0x09, 0x8f, 0x61, 0xb5, 0x0f, 0x1d, 0x26, 0x39, 0x2e, 0x8c, 0xd6,
		0x26, 0x5c, 0x3d, 0x23, 0x63, 0xe9, 0x6b, 0x97, 0xb4, 0x9f, 0x7b, 0xb6, 0xba, 0xa0, 0x7c, 0xc6,
		0x25, 0xa1, 0x73, 0x36, 0x67, 0x7f, 0x74, 0x1e, 0x1d, 0xda, 0x70, 0xbf, 0xa5, 0x63, 0x35, 0x39,
		0x24, 0x8c, 0x9f, 0x85, 0x16, 0xd8, 0x50, 0x95, 0x71, 0xc0, 0xf6, 0x1e, 0x6d, 0x80, 0xed, 0x15,
		0xeb, 0x63, 0xe9, 0x1b, 0xf6, 0x78, 0x31, 0xc6, 0x5c, 0xdd, 0x19, 0xbd, 0xdf, 0xa7, 0xec, 0x50,
		0x22, 0xad, 0xbb, 0xf6, 0xeb, 0xd6, 0xa3, 0x20, 0xc9, 0xe6, 0x9f, 0xcb, 0xf2, 0x97, 0xb9, 0x54,
		0x12, 0x66, 0xa6, 0xbe, 0x4a, 0x12, 0x43, 0xec, 0x00, 0xea, 0x49, 0x02
	},
	{ /* region 7, $255E8C */
		0xa4, 0x49, 0xf3, 0x29, 0xa0, 0x11, 0xf1, 0x11, 0x10, 0xef, 0x80, 0x20, 0x3d, 0xaa, 0x36, 0x5d,
		0x98, 0xc4, 0xd2, 0x63, 0xdf, 0x61, 0xb0, 0xc3, 0xc2, 0x35, 0xd4, 0x88, 0xe6, 0x1d, 0x3a, 0x2f,
		0x9c, 0xb9, 0xd1, 0xc6, 0x43, 0xba, 0x69, 0x6d, 0x49, 0xac, 0xdd, 0x05, 0xe0, 0xf8, 0xe8, 0x97,
		0x19, 0x18, 0x08, 0x0c, 0x42, 0x46, 0xc7, 0x0d, 0x25, 0xce, 0xc3, 0x54, 0xb2, 0xd9, 0x42, 0x91,
		0xea, 0x53, 0x98, 0x38, 0x78, 0x81, 0x12, 0xca, 0x15, 0x23, 0xbd, 0xc1, 0x70, 0x1f, 0xd2, 0x40,
		0xfd, 0x39, 0x33, 0xaa, 0x27, 0x2b, 0xe8, 0x10, 0x7d, 0xa4, 0xa8, 0x8e, 0x3d, 0x00, 0x4f, 0x3a,
		0x7f, 0xd8, 0x96, 0xea, 0x9e, 0x8e, 0x15, 0x6e, 0x9f, 0x76, 0x57, 0xba, 0x7d, 0xc2, 0xdf, 0x57,
		0x42, 0x82, 0xf4, 0xda, 0x89, 0x06, 0x05, 0x04, 0x62, 0x2f, 0x29, 0x23, 0x54, 0xd5, 0xbb, 0x97,
		0xf5, 0xf9, 0xc1, 0xcf, 0xec, 0x5f, 0x1d, 0xfd, 0xbb, 0xa6, 0xd7, 0x4a, 0xa8, 0x66, 0xbf, 0xb9,
		0x09, 0x44, 0xb1, 0x60, 0x28, 0xa9, 0x35, 0x16, 0x15, 0xf5, 0x13, 0xc1, 0x07, 0x7e, 0xd7, 0x40,
		0xdf, 0x8e, 0xd3, 0x32, 0xa9, 0x35, 0x98, 0x15, 0x32, 0xa9, 0x49, 0xc0, 0x24, 0xb4, 0x4a, 0x53,
		0x6b, 0x79, 0xaa, 0x77, 0x6c, 0xc5, 0x88, 0x69, 0xe5, 0x5d, 0xde, 0x42, 0x28, 0xf9, 0xb7, 0x5c,
		0xab, 0x19, 0xc7, 0xbc, 0xc5, 0x60, 0xeb, 0x5e, 0xa8, 0x52, 0xc4, 0x32, 0x7c, 0x35, 0x02, 0x06,
		0x46, 0x77, 0x30, 0xb6, 0x33, 0x4b, 0xb8, 0xfd, 0x02, 0xd8, 0x14, 0x40, 0x99, 0x25, 0x7e, 0x55,
		0xd6, 0x44, 0x43, 0x8d, 0x73, 0x0e, 0x71, 0x48, 0xd3, 0x82, 0x40, 0xda
	}
};

UINT32 olds_prot_addr(UINT16 addr)
{
	switch (addr & 0xff)
	{
		case 0x0:
		case 0x5:
		case 0xa: return 0x402a00 + ((addr >> 8) << 2);
		case 0x2:
		case 0x8: return 0x402e00 + ((addr >> 8) << 2);
		case 0x1: return 0x40307e;
		case 0x3: return 0x403090;
		case 0x4: return 0x40309a;
		case 0x6: return 0x4030a4;
		case 0x7: return 0x403000;
		case 0x9: return 0x40306e;
	}

	return 0;
}

UINT32 olds_read_reg(UINT16 addr)
{
	UINT32 protaddr = (olds_prot_addr(addr) - 0x400000) / 2;
	return olds_sharedprotram[protaddr] << 16 | olds_sharedprotram[protaddr + 1];
}

void olds_write_reg( UINT16 addr, UINT32 val )
{
	olds_sharedprotram[((olds_prot_addr(addr) - 0x400000) / 2) + 0] = val >> 16;
	olds_sharedprotram[((olds_prot_addr(addr) - 0x400000) / 2) + 1] = val & 0xffff;
}

void IGS028_do_dma(UINT16 src, UINT16 dst, UINT16 size, UINT16 mode)
{
	UINT16 param = mode >> 8;
	UINT16 *PROTROM = (UINT16*)memory_region(REGION_USER1);

/*	logerror ("mode: %2.2x, src: %4.4x, dst: %4.4x, size: %4.4x, data: %4.4x\n", (mode &0xf), src, dst, size, mode); */

	mode &= 0x0f;

	switch (mode)
	{
		case 0x00: 
		/* This mode copies code later on in the game! Encrypted somehow?
		 src:2fc8, dst: 045a, size: 025e, mode: 0000
		 jumps from 12beb4 in unprotected set
		 jumps from 1313e0 in protected set
		*/
		case 0x01: /* swap bytes and nibbles */
		case 0x02: /* ^= encryption */
		case 0x05: /* copy */
		case 0x06: /* += encryption (correct?) */
		{
			UINT8 extraoffset = param & 0xff;
			UINT8 *dectable = (UINT8 *)(PROTROM + (0x100 / 2));

			INT32 x = 0;
			for (x = 0; x < size; x++)
			{
				UINT16 dat2 = PROTROM[src + x];

				int taboff = ((x*2)+extraoffset) & 0xff; /* must allow for overflow in instances of odd offsets */
				unsigned short extraxor = ((dectable[taboff + 0]) << 0) | (dectable[taboff + 1] << 8);

				if (mode==0) dat2 = 0x4e75; /* hack */
				if (mode==1) dat2  = ((dat2 & 0xf000) >> 12) | ((dat2 & 0x0f00) >> 4) | ((dat2 & 0x00f0) << 4) | ((dat2 & 0x000f) << 12);
				if (mode==2) dat2 ^= extraxor;
			/*	if (mode==5) dat2  = dat2; */
				if (mode==6) dat2 += extraxor;

				if (mode==2 || mode==6) dat2 = (dat2<<8)|(dat2>>8);

				olds_sharedprotram[dst + x] = (dat2 << 8) | (dat2 >> 8);
			}
		}
		break;

	/*	default: */
	/*		logerror ("DMA mode unknown!!!\nsrc:%4.4x, dst: %4.4x, size: %4.4x, mode: %4.4x\n", src, dst, size, mode); */
	}
}

void olds_protection_calculate_hold(int y, int z) /* calculated in routine $12dbc2 in olds */
{
	unsigned short old = olds_prot_hold;

	olds_prot_hold = ((old << 1) | (old >> 15));

	olds_prot_hold ^= 0x2bad;
	olds_prot_hold ^= BIT(z, y);
	olds_prot_hold ^= BIT( old,  7) <<  0;
	olds_prot_hold ^= BIT(~old, 13) <<  4;
	olds_prot_hold ^= BIT( old,  3) << 11;

	olds_prot_hold ^= (olds_prot_hilo & ~0x0408) << 1; /* $81790c */
}

void olds_protection_calculate_hilo() /* calculated in routine $12dbc2 in olds */
{
	UINT8 source;

	olds_prot_hilo_select++;
	if (olds_prot_hilo_select > 0xeb) {
		olds_prot_hilo_select = 0;
	}

	source = olds_source_data[readinputport(4)][olds_prot_hilo_select];

	if (olds_prot_hilo_select & 1)    /* $8178fa */
	{
		olds_prot_hilo = (olds_prot_hilo & 0x00ff) | (source << 8);     /* $8178d8 */
	}
	else
	{
		olds_prot_hilo = (olds_prot_hilo & 0xff00) | (source << 0);     /* $8178d8 */
	}
}

WRITE16_HANDLER(olds_w )
{
	if (offset == 0)
	{
		olds_cmd = data;
	}
	else
	{
		switch (olds_cmd)
		{
			case 0x00:
				olds_reg = data;
			break;

			case 0x02:
				olds_bs = ((data & 0x03) << 6) | ((data & 0x04) << 3) | ((data & 0x08) << 1);
			break;

			case 0x03:
			{
				UINT16 cmd = olds_sharedprotram[0x3026 / 2];

			/*  logerror ("command: %x\n", cmd); */

				switch (cmd)
				{
					case 0x12:
					{
						UINT16 mode = olds_sharedprotram[0x303e / 2];  /* ? */
						UINT16 src  = olds_sharedprotram[0x306a / 2] >> 1; /* ? */
						UINT16 dst  = olds_sharedprotram[0x3084 / 2] & 0x1fff;
						UINT16 size = olds_sharedprotram[0x30a2 / 2] & 0x1fff;

						IGS028_do_dma(src, dst, size, mode);
					}
					break;

					case 0x64: /* incomplete? */
					{
					        UINT16 p1 = olds_sharedprotram[0x3050 / 2];
					        UINT16 p2 = olds_sharedprotram[0x3082 / 2];
					        UINT16 p3 = olds_sharedprotram[0x3054 / 2];
					        UINT16 p4 = olds_sharedprotram[0x3088 / 2];

					        if (p2  == 0x02)
					                olds_write_reg(p1, olds_read_reg(p1) + 0x10000);

					        switch (p4)
					        {
					                case 0xd:
					                        olds_write_reg(p1,olds_read_reg(p3));
					                        break;
					                case 0x0:
					                        olds_write_reg(p3,(olds_read_reg(p2))^(olds_read_reg(p1)));
					                        break;
					                case 0xe:
					                        olds_write_reg(p3,olds_read_reg(p3)+0x10000);
					                        break;
					                case 0x2:
					                        olds_write_reg(p1,(olds_read_reg(p2))+(olds_read_reg(p3)));
					                        break;
					                case 0x6:
					                        olds_write_reg(p3,(olds_read_reg(p2))&(olds_read_reg(p1)));
					                        break;
					                case 0x1:
					                        olds_write_reg(p2,olds_read_reg(p1)+0x10000);
					                        break;
					                case 0x7:
					                        olds_write_reg(p3,olds_read_reg(p1));
					                        break;
					                default:
					                        break;
					        }
					}
					break;

				/*  default: */
				/*      logerror ("unemulated command!\n"); */
				}

				olds_cmd3 = ((data >> 4) + 1) & 0x3;
			}
			break;

			case 0x04:
				olds_ptr = data;
			break;

			case 0x20:
			case 0x21:
			case 0x22:
			case 0x23:
			case 0x24:
			case 0x25:
			case 0x26:
			case 0x27:
				olds_ptr++;
				olds_protection_calculate_hold(olds_cmd & 0x0f, data & 0xff);
			break;

		/*	default: */
		/*		logerror ("unemulated write mode!\n"); */
		}
	}
}

READ16_HANDLER(olds_r )
{
	if (offset)
	{
		switch (olds_cmd)
		{
			case 0x01:
				return olds_reg & 0x7f;

			case 0x02:
				return olds_bs | 0x80;

			case 0x03:
				return olds_cmd3;

			case 0x05:
			{
				switch (olds_ptr)
				{
					case 1: return 0x3f00 | readinputport(4);

					case 2:
						return 0x3f00 | 0x00;

					case 3:
						return 0x3f00 | 0x90;

					case 4:
						return 0x3f00 | 0x00;

					case 5:
					default: /* >= 5 */
						return 0x3f00 | BITSWAP8(olds_prot_hold, 5,2,9,7,10,13,12,15);    /* $817906 */
				}
			}

			case 0x40:
				olds_protection_calculate_hilo();
				return 0; /* unused? */
		}
	}

	return 0;
}


DRIVER_INIT(olds)
{
	pgm_basic_init();

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xdcb400, 0xdcb403, 0, 0, olds_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xdcb400, 0xdcb403, 0, 0, olds_w);

	olds_prot_hold = 0;
	olds_prot_hilo = 0;
	olds_prot_hilo_select = 0;

	olds_cmd = 0;
	olds_reg = 0;
	olds_ptr = 0;
	olds_bs = 0;
	olds_cmd3 = 0;

}

/* no protection */
static DRIVER_INIT( olds103t )
{
	pgm_basic_init();
}

static READ32_HANDLER( dmnfrnt_speedup_r )
{
	int pc = activecpu_get_pc();
    if (pc == 0x8000fea) activecpu_eat_cycles(500); // correct i think
//	else printf("dmn_speedup_r %08x\n", pc);
	return arm_ram[0x000444/4];
}

static void svg_basic_init(void)
{
	pgm_basic_init();
	svg_shareram[0]=auto_malloc(0x10000);
	svg_shareram[1]=auto_malloc(0x10000);
	svg_ram_sel=0;
}

static DRIVER_INIT( dmnfrnt )
{
	svg_basic_init();
	pgm_dfront_decrypt();
	kov2_latch_init();
	
	/* put some fake code for the ARM here ... */
	UINT16 *temp16 = (UINT16 *)memory_region(REGION_CPU3);
	temp16[(0x0000)/2] = 0xd088;
	temp16[(0x0002)/2] = 0xe59f;
	temp16[(0x0004)/2] = 0x0680;
	temp16[(0x0006)/2] = 0xe3a0;
	temp16[(0x0008)/2] = 0xff10;
	temp16[(0x000a)/2] = 0xe12f;
	temp16[(0x0090)/2] = 0x0400;
	temp16[(0x0092)/2] = 0x1000;
	
//	memory_install_read32_handler(2, ADDRESS_SPACE_PROGRAM, 0x18000444, 0x18000447, 0, 0, dmnfrnt_speedup_r); // 16 or 32.??

	// the internal rom probably also supplies the region here
	UINT16 *share16 = (UINT16 *)(svg_shareram[0]);
	share16[0x158/2] = 0x0005;
}

static MACHINE_RESET(theglad)
{
	// this is the location of the region in the internal rom, for some reason Japan doesn't play attract music (original game feature? bad code flow?)
	UINT16 *temp16 = (UINT16 *)memory_region(REGION_CPU3);
	int base = -1;

	if(strcmp(Machine->gamedrv->name, "theglad") == 0)
	{
       base = 0x3316; // correct.??
	} 
	//if (!strcmp(machine().system().name, "theglad")) base = 0x3316;

	if (base != -1)
	{
		int regionhack = readinputportbytag("RegionHack");
		if (regionhack != 0xff)
		{
			temp16[(base) / 2] = regionhack; base += 2;
		}
	}
	machine_reset_pgm();
}

READ32_HANDLER(theglad_speedup_r )
{
	int pc = activecpu_get_pc();
	if (pc == 0x7c4) activecpu_eat_cycles(500); // correct i think
	//else printf("theglad_speedup_r %08x\n", pc);
	return arm_ram2[0x00c/4];
}


static void armsim_theglad(void)
{
	UINT16 *temp16 = (UINT16 *) memory_region(REGION_CPU3);

	int i;
	for (i=0;i<0x188/2;i+=2)
	{
		temp16[i] = 0xFFFE;
		temp16[i+1] = 0xEAFF;

	}

	// the interrupt code appears to be at 0x08000010
	// so point the FIQ vector to jump there, the actual internal EO area code
	// would not look like this because this reads from the EO area to get the jump address which is verified
	// as impossible
	int base = 0x1c;
	temp16[(base) /2] = 0xf000; base += 2;
	temp16[(base) /2] = 0xe59f; base += 2;
	temp16[(base) /2] = 0x0010; base += 2;
	temp16[(base) /2] = 0x0800; base += 2;
	temp16[(base) /2] = 0x0010; base += 2;
	temp16[(base) /2] = 0x0800; base += 2;
	 

	// some startup code to set up the stacks etc. we're assuming
	// behavior is basically the same as killing blade plus here, this code
	// could be very wrong
	base = 0x30;

	temp16[(base) /2] = 0x00D2; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;

	temp16[(base) /2] = 0x4001; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x4B06; base += 2;
	temp16[(base) /2] = 0xE284; base += 2;
	temp16[(base) /2] = 0x0CFA; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xD804; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;
	temp16[(base) /2] = 0x00D1; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0x0CF6; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xD804; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;
	temp16[(base) /2] = 0x00D7; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0x0CFF; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xD804; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;
	temp16[(base) /2] = 0x00DB; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0x4140; base += 2;
	temp16[(base) /2] = 0xE1C4; base += 2;
	temp16[(base) /2] = 0x0CFE; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xD804; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;
	temp16[(base) /2] = 0x00D3; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;

	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0x4A01; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x0B01; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xD804; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;
	temp16[(base) /2] = 0x5A0F; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x0008; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x8805; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;
	temp16[(base) /2] = 0x0010; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x0000; base += 2;
	temp16[(base) /2] = 0xE5C8; base += 2;
	temp16[(base) /2] = 0x7805; base += 2;
	temp16[(base) /2] = 0xE1A0; base += 2;
	temp16[(base) /2] = 0x6A01; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x0012; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x0A02; base += 2;
	temp16[(base) /2] = 0xE280; base += 2;
	temp16[(base) /2] = 0x6806; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;
	temp16[(base) /2] = 0x6000; base += 2;
	temp16[(base) /2] = 0xE587; base += 2;
	
	// set the SR13 to something sensible
	temp16[(base) /2] = 0x00D3; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0x4001; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x4B06; base += 2;
	temp16[(base) /2] = 0xE284; base += 2;
	temp16[(base) /2] = 0x0CF2; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xD804; base += 2;
	temp16[(base) /2] = 0xE080; base += 2;

	temp16[(base) /2] = 0x0013; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;

	temp16[(base) /2] = 0x0028; base += 2; // jump to 0x184
	temp16[(base) /2] = 0xEA00; base += 2;

	base = 0;
	temp16[(base) /2] = 0x000a; base += 2;
	temp16[(base) /2] = 0xEA00; base += 2;

	// see table at ~080824A4 in The Gladiator (ARM space)
	// there are pointers to
	// 0000 00FC
	// 0000 00E8
	// 0000 0110
	// 0000 0150
	// in the table.. for e8 / fc we can deduce from the calling code and size of the functions expected that they should be the
	// same as those in the killing blade plus 'killbldp'  (there are also explicit jumps to these addresses in the code)
	//
	// 0x110 is called after the 'continue' screen, and on inserting coin, I guess it should change the game state, causing it to jump to the title screen when you insert a coin, and back to the attract after the game.. some kind of 'soft reset'
	// 0x150 I haven't seen called, I guess it is 0x38 in size because the execute-only area ends at 0x188

	base = 0xe8;
	temp16[(base) /2] = 0xE004; base += 2; // based on killbldp
	temp16[(base) /2] = 0xE52D; base += 2;
	temp16[(base) /2] = 0x00D3; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0xE004; base += 2;
	temp16[(base) /2] = 0xE49D; base += 2;
	temp16[(base) /2] = 0xFF1E; base += 2;
	temp16[(base) /2] = 0xE12F; base += 2;

//	base = 0xfc; // already at 0xfc
	temp16[(base) /2] = 0xE004; base += 2; // based on killbldp
	temp16[(base) /2] = 0xE52D; base += 2;
	temp16[(base) /2] = 0x0013; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0xE004; base += 2;
	temp16[(base) /2] = 0xE49D; base += 2;
	temp16[(base) /2] = 0xFF1E; base += 2;
	temp16[(base) /2] = 0xE12F; base += 2;

//	base = 0x110; // already at 0x110
//	temp16[(base) /2] = 0xff1e; base += 2;
//	temp16[(base) /2] = 0xe12f; base += 2;
//	temp16[(base) /2] = 0xf302; base += 2;
//	temp16[(base) /2] = 0xe3a0; base += 2;
	// set up stack again, soft-reset reset with a ram variable set to 0
	temp16[(base) /2] = 0x00D1; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0xD0b8; base += 2;
	temp16[(base) /2] = 0xE59F; base += 2;
	temp16[(base) /2] = 0x00D3; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0xF000; base += 2;
	temp16[(base) /2] = 0xE121; base += 2;
	temp16[(base) /2] = 0xD0b0; base += 2;
	temp16[(base) /2] = 0xE59F; base += 2;
	temp16[(base) /2] = 0x10b8; base += 2;
	temp16[(base) /2] = 0xE59F; base += 2;
	temp16[(base) /2] = 0x0000; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;
	temp16[(base) /2] = 0x0000; base += 2;
	temp16[(base) /2] = 0xE581; base += 2;
	temp16[(base) /2] = 0xF302; base += 2;
	temp16[(base) /2] = 0xE3A0; base += 2;

	base = 0x150;
	temp16[(base) /2] = 0xff1e; base += 2;
	temp16[(base) /2] = 0xe12f; base += 2;

	// the non-EO area starts in the middle of a function that seems similar to those  at 000037E4 / 000037D4 in killbldp.. by setting this up we allow the intro to run, but can no longer can get in game..
	// it sets '0x10000038' to a value ot 1
	// maybe this flag needs to be flipped on interrupts or similar??
	base = 0x184;
	temp16[(base) /2] = 0x105c; base += 2;
	temp16[(base) /2] = 0xE59F; base += 2;
}

DRIVER_INIT(theglad)
{
	svg_basic_init();
	pgm_theglad_decrypt();
	kov2_latch_init();
	armsim_theglad();
	
//	memory_install_read32_handler(2, ADDRESS_SPACE_PROGRAM, 0x1000000c, 0x1000000f, 0, 0, theglad_speedup_r);
}

/*** Rom Loading *************************************************************/

/* take note of REGION_GFX2 needed for expanding the 32x32x5bpp data and
   REGION_GFX4 needed for expanding the Sprite Colour Data */

/* The Bios - NOT A GAME */
ROM_START( pgm )
	ROM_REGION( 0x520000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x00000, 0x20000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* 8x8 Text Layer Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) )
ROM_END

ROM_START( orlegend )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0103.rom",    0x100000, 0x200000, CRC(d5e93543) SHA1(f081edc26514ca8354c13c7f6f89aba8e4d3e7d2) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

ROM_START( orlegnde )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0102.rom",    0x100000, 0x200000, CRC(4d0f6cc5) SHA1(8d41f0a712fb11a1da865f5159e5e27447b4388a) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

ROM_START( orlegndc )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0101.160",    0x100000, 0x200000, CRC(b24f0c1e) SHA1(a2cf75d739681f091c24ef78ed6fc13aa8cfe0c6) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

/*

Oriental Legend / Xi Yo Gi Shi Re Zuang (CHINA 111 Ver.)
(c)1997 IGS

PGM system
IGS PCB NO-0134-1
IGS PCB NO-0135


OLV111CH.U11 [b80ddd3c]
OLV111CH.U6  [5fb86373]
OLV111CH.U7  [6ee79faf]
OLV111CH.U9  [83cf09c8]

T0100.U8


A0100.U5
A0101.U6
A0102.U7
A0103.U8
A0104.U11
A0105.U12

B0100.U9
B0101.U10
B0102.U15

M0100.U1

*/

ROM_START( orld111c )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "olv111ch.u6",     0x100001, 0x080000, CRC(5fb86373) SHA1(2fc58eff1f38754c75819fde666244b867ca4f05) )
	ROM_LOAD16_BYTE( "olv111ch.u9",     0x100000, 0x080000, CRC(83cf09c8) SHA1(959780b45326059517f3008a356657f4f3d2908f) )
	ROM_LOAD16_BYTE( "olv111ch.u7",     0x200001, 0x080000, CRC(6ee79faf) SHA1(039b4b07b8577f0d3022ae01210c00375624cb3c) )
	ROM_LOAD16_BYTE( "olv111ch.u11",    0x200000, 0x080000, CRC(b80ddd3c) SHA1(55c700ce71ffdee392e03fd9d4719542c3527132) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

/*

Oriental Legend / Xi Yo Gi Shi Re Zuang (KOREA 105 Ver.)
(c)1997 IGS

PGM system
IGS PCB NO-0134-2
IGS PCB NO-0135


OLV105KO.U11 [40ae4d9e]
OLV105KO.U6  [b86703fe]
OLV105KO.U7  [5712facc]
OLV105KO.U9  [5a108e39]

T0100.U8


A0100.U5
A0101.U6
A0102.U7
A0103.U8
A0104.U11
A0105.U12

B0100.U9
B0101.U10
B0102.U15

M0100.U1

*/

ROM_START( orld105k )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "olv105ko.u6",     0x100001, 0x080000, CRC(b86703fe) SHA1(a3529b45efd400ecd5e76f764b528ebce46e24ab) )
	ROM_LOAD16_BYTE( "olv105ko.u9",     0x100000, 0x080000, CRC(5a108e39) SHA1(2033f4fe3f2dfd725dac535324f58348b9ac3914) )
	ROM_LOAD16_BYTE( "olv105ko.u7",     0x200001, 0x080000, CRC(5712facc) SHA1(2d95ebd1703874e89ac3a206f8c1f0ece6e833e0) )
	ROM_LOAD16_BYTE( "olv105ko.u11",    0x200000, 0x080000, CRC(40ae4d9e) SHA1(62d7a96438b7fe93f74753333f50e077d417971e) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0100.rom",    0x400000, 0x400000, CRC(61425e1e) SHA1(20753b86fc12003cfd763d903f034dbba8010b32) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0100.rom",    0x0000000, 0x400000, CRC(8b3bd88a) SHA1(42db3a60c6ba9d83ebe2008c8047d094027f65a7) )
	ROM_LOAD( "a0101.rom",    0x0400000, 0x400000, CRC(3b9e9644) SHA1(5b95ec1d25c3bc3504c93547f5adb5ce24376405) )
	ROM_LOAD( "a0102.rom",    0x0800000, 0x400000, CRC(069e2c38) SHA1(9bddca8c2f5bd80f4abe4e1f062751736dc151dd) )
	ROM_LOAD( "a0103.rom",    0x0c00000, 0x400000, CRC(4460a3fd) SHA1(cbebdb65c17605853f7d0b298018dd8801a25a58) )
	ROM_LOAD( "a0104.rom",    0x1000000, 0x400000, CRC(5f8abb56) SHA1(6c1ddc0309862a141aa0c0f63b641aec9257aaee) )
	ROM_LOAD( "a0105.rom",    0x1400000, 0x400000, CRC(a17a7147) SHA1(44eeb43c6b0ebb829559a20ae357383fbdeecd82) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0100.rom",    0x0000000, 0x400000, CRC(69d2e48c) SHA1(5b5f759007264c07b3b39be8e03a713698e1fc2a) )
	ROM_LOAD( "b0101.rom",    0x0400000, 0x400000, CRC(0d587bf3) SHA1(5347828b0a6e4ddd7a263663d2c2604407e4d49c) )
	ROM_LOAD( "b0102.rom",    0x0800000, 0x400000, CRC(43823c1e) SHA1(e10a1a9a81b51b11044934ff702e35d8d7ab1b08) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0100.rom",    0x400000, 0x200000, CRC(e5c36c83) SHA1(50c6f66770e8faa3df349f7d68c407a7ad021716) )
ROM_END

/*

Dragon World 2 (English / World Version)
IGS, 1997

This is a cart for the IGS PGM system.


PCB Layout
----------

IGS PCB NO-0162
|-----------------------------------------------|
| |------|                                      |
| |IGS012|       *1                    T0200.U7 |
| |      |                                      |
| |------|                                      |
|              |--------|                       |
|              |        |                       |
|              | IGS025 |  *2   V-110X.U2       |
| PAL    PAL   |        |                  PAL  |
|              |--------|                       |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      IGS012       - Custom IGS IC (QFP80)

      -- on english version
      IGS025       - Custom IGS IC (PLCC68, labelled "DRAGON II 0006")
      -- on china version
      IGS025       - Custom IGS IC (PLCC68, labelled "DRAGON II 0005")


      T0200.U7     - 32MBit MaskROM (SOP44)

      -- on english version
      V-110X.U2    - AM27C4096 4MBit EPROM (DIP42, labelled "DRAGON II V-110X")
      -- on china version
      V-110X.U2    - AM27C4096 4MBit EPROM (DIP42, labelled "DRAGON II V-100C")

      PALs         - x3, labelled "CZ U3", "CZ U4", "CZ U6"
      *1           - Unpopulated position for MX23C4100 SOP40 MASKROM
      *2           - Unpopulated position for MX23C4100 DIP40 EPROM/MASKROM


IGS PCB NO-0135
|-----------------------------------------------|
| U11    U12     U13      U14       U15      U16|
|                                               |
|                                               |
|A0200.U5                       B0200.U9        |
|        U6      U7       U8                 U10|
|                                               |
|                                               |
|74LS138         U1       U2             74LS139|
|                                               |
|-|                                           |-|
  |--------------------||---------------------|

Notes:
      This PCB contains only SOP44 MASKROMS and 2 logic IC's
      Only U5 and U9 are populated

      glitch on select screen exists on real board.

*/

ROM_START( drgw2 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "v-110x.u2",    0x100000, 0x080000, CRC(1978106b) SHA1(af8a13d7783b755a58762c98bdc32cab845b2251) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "pgmt0200.u7",    0x400000, 0x400000, CRC(b0f6534d) SHA1(174cacd81169a0e0d14790ac06d03caed737e05d) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "pgma0200.u5",    0x0000000, 0x400000, CRC(13b95069) SHA1(4888b06002afb18eab81c010e9362629045767af) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "pgmb0200.u9",    0x0000000, 0x400000, CRC(932d0f13) SHA1(4b8e008f9c617cb2b95effeb81abc065b30e5c86) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
ROM_END


ROM_START( drgw2c )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "v-100c.u2",    0x100000, 0x080000, CRC(67467981) SHA1(58af01a3871b6179fe42ff471cc39a2161940043) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "pgmt0200.u7",    0x400000, 0x400000, CRC(b0f6534d) SHA1(174cacd81169a0e0d14790ac06d03caed737e05d) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "pgma0200.u5",    0x0000000, 0x400000, CRC(13b95069) SHA1(4888b06002afb18eab81c010e9362629045767af) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "pgmb0200.u9",    0x0000000, 0x400000, CRC(932d0f13) SHA1(4b8e008f9c617cb2b95effeb81abc065b30e5c86) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
ROM_END

ROM_START( drgw2j )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "v-100j.u2",    0x100000, 0x080000, CRC(f8f8393e) SHA1(ef0db668b4e4f661d4c1e95d57afe881bcdf13cc) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x800000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "pgmt0200.u7",    0x400000, 0x400000, CRC(b0f6534d) SHA1(174cacd81169a0e0d14790ac06d03caed737e05d) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "pgma0200.u5",    0x0000000, 0x400000, CRC(13b95069) SHA1(4888b06002afb18eab81c010e9362629045767af) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "pgmb0200.u9",    0x0000000, 0x400000, CRC(932d0f13) SHA1(4b8e008f9c617cb2b95effeb81abc065b30e5c86) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
ROM_END

/*

Dragon World 3
Alta Co./IGS, 1998

Cart for IGS PGM system

Top board of cart contains.....
8MHz Xtal
32.768kHz Xtal
UM6164 (RAM x 2)
MACH211 CPLD
IGS022 ASIC
IGS025 ASIC
1x PAL
2x 27C040 EPROMs (main 68k program)
1x 27C512 EPROM (protection code?)
1x 32MBit smt MASKROM (T0400)

Bottom board contains.....
4x 32MBit smt MASKROMs (A0400, A0401, B0400, M0400)

*/

ROM_START( drgw3 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "dw3_v100.u12",     0x100001, 0x080000,  CRC(47243906) SHA1(9cd46e3cba97f049bcb238ceb6edf27a760ef831) )
	ROM_LOAD16_BYTE( "dw3_v100.u13",     0x100000, 0x080000,  CRC(b7cded21) SHA1(c1ae2af2e42227503c81bbcd2bd6862aa416bd78) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* Protection Data */
	ROM_LOAD( "dw3_v100.u15", 0x000000, 0x010000, CRC(03dc4fdf) SHA1(b329b04325d4f725231b1bb7862eedef2319b652) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "dw3t0400.u18",   0x400000, 0x400000, CRC(b70f3357) SHA1(8733969d7d21f540f295a9f747a4bb8f0d325cf0) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "dw3a0400.u9",     0x0000000, 0x400000, CRC(dd7bfd40) SHA1(fb7ec5bf89a413c5208716083762a725ff63f5db) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "dw3a0401.u10",    0x0400000, 0x400000, CRC(cab6557f) SHA1(1904dd86645eea27ac1ab8a2462b20f6531356f8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "dw3b0400.u13",    0x0000000, 0x400000,  CRC(4bb87cc0) SHA1(71b2dc43fd11f7a6dffaba501e4e344b843583d8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "dw3m0400.u1",  0x400000, 0x400000, CRC(031eb9ce) SHA1(0673ec194732becc6648c2ae1396e894aa269f9a) )
ROM_END

/*

Dragon World 3 (KOREA 106 Ver.)
(c)1998 IGS

PGM system
IGS PCB NO-0189
IGS PCB NO-0178


DW3_V106.U12 [c3f6838b]
DW3_V106.U13 [28284e22]


*/

ROM_START( drgw3k )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "dw3_v106.u12",     0x100001, 0x080000,  CRC(c3f6838b) SHA1(c135b1d4dd62af308139d40d03c29be7508fb1e7) )
	ROM_LOAD16_BYTE( "dw3_v106.u13",     0x100000, 0x080000,  CRC(28284e22) SHA1(4643a69881ddb7383ca10f3eb2aa2cf41be39e9f) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* Protection Data - is it correct for this set? */
	ROM_LOAD( "dw3_v100.u15", 0x000000, 0x010000, CRC(03dc4fdf) SHA1(b329b04325d4f725231b1bb7862eedef2319b652) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "dw3t0400.u18",   0x400000, 0x400000, CRC(b70f3357) SHA1(8733969d7d21f540f295a9f747a4bb8f0d325cf0) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "dw3a0400.u9",     0x0000000, 0x400000, CRC(dd7bfd40) SHA1(fb7ec5bf89a413c5208716083762a725ff63f5db) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "dw3a0401.u10",    0x0400000, 0x400000, CRC(cab6557f) SHA1(1904dd86645eea27ac1ab8a2462b20f6531356f8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "dw3b0400.u13",    0x0000000, 0x400000,  CRC(4bb87cc0) SHA1(71b2dc43fd11f7a6dffaba501e4e344b843583d8) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "dw3m0400.u1",  0x400000, 0x400000, CRC(031eb9ce) SHA1(0673ec194732becc6648c2ae1396e894aa269f9a) )
ROM_END

ROM_START( kov )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.117",    0x100000, 0x400000, CRC(c4d19fe6) SHA1(14ef31539bfbc665e76c9703ee01b12228344052) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kov115 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.115",    0x100000, 0x400000, CRC(527a2924) SHA1(7e3b166dddc5245d7b408e78437c16fd2986d1d9) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

/*

Sangoku Senki / Knights of Valour (JPN 100 Ver.)
(c)1999 ALTA / IGS

PGM system
IGS PCB NO-0212-1
IGS PCB NO-0213T


SAV111.U10   [d5536107]
SAV111.U4    [ae2f1b4e]
SAV111.U5    [5fdd4aa8]
SAV111.U7    [95eedf0e]
SAV111.U8    [003cbf49]

T0600.U11


A0600.U2
A0601.U4
A0602.U6
A0603.U9

M0600.U3

B0600.U5
B0601.U7

*/

ROM_START( kovj )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "sav111.u4",      0x100001, 0x080000, CRC(ae2f1b4e) SHA1(2ac9d84f5dee52f374941cfd68e2b98ecad436a8) )
	ROM_LOAD16_BYTE( "sav111.u7",      0x100000, 0x080000, CRC(95eedf0e) SHA1(582a54e9a1eda7ff73e20f0e69d2d50141772378) )
	ROM_LOAD16_BYTE( "sav111.u5",      0x200001, 0x080000, CRC(5fdd4aa8) SHA1(43c96e21ad4f11148e1e94a59c53780b2edd43ba) )
	ROM_LOAD16_BYTE( "sav111.u8",      0x200000, 0x080000, CRC(003cbf49) SHA1(fb5bea47ecae025b1b425af52cd05e061f45e377) )
	ROM_LOAD16_WORD_SWAP( "sav111.u10",0x300000, 0x080000, CRC(d5536107) SHA1(f963e015d99c1621323eecf63e773c0b9f4b6a43) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( kovplus )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) ) // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.119",    0x100000, 0x400000, CRC(e4b0875d) SHA1(e8382e131b0e431406dc2a05cc1ef128302d987c) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

/*

Sangoku Senki Plus / Knights of Valour Plus (Alt 119 Ver.)
(c)1999 IGS

PGM system
IGS PCB NO-0222
IGS PCB NO-0213


V119.U2      [29588ef2]
V119.U3      [6750388f]
V119.U4      [8200ece6]
V119.U5      [d4101ffd]
V119.U6      [71e28f27]

T0600.U11


A0600.U2
A0601.U4
A0602.U6
A0603.U9

M0600.U3

B0600.U5
B0601.U7

*/

ROM_START( kovplusa )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) ) // (BIOS)
	ROM_LOAD16_BYTE( "v119.u3",     0x100001, 0x080000, CRC(6750388f) SHA1(869f4ad27f2992cc62baa9a78bf7984a43ec4cc5) )
	ROM_LOAD16_BYTE( "v119.u5",     0x100000, 0x080000, CRC(d4101ffd) SHA1(a327fd56eec65b07df9305cd93ef2c46bf8e40f3) )
	ROM_LOAD16_BYTE( "v119.u4",     0x200001, 0x080000, CRC(8200ece6) SHA1(97081d2e8aed2ac6fbe5951890aecea18af5ce2e) )
	ROM_LOAD16_BYTE( "v119.u6",     0x200000, 0x080000, CRC(71e28f27) SHA1(db382807e9185f0dc17124f210165fa1b36ca6ac) )
	ROM_LOAD16_WORD_SWAP( "v119.u2",0x300000, 0x080000, CRC(29588ef2) SHA1(17d1a308d44434cf65224a24360cf4b6e32d28f3) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.rom",    0x400000, 0x800000, CRC(4acc1ad6) SHA1(0668dbd5e856c2406910c6b7382548b37c631780) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

/* is this loading the right roms? */
ROM_START( kovsh )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0600.322",    0x100000, 0x400000, CRC(7c78e5f3) SHA1(9b1e4bd63fb1294ebeb539966842273c8dc7683b) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0600.320",    0x400000, 0x400000, CRC(164b3c94) SHA1(f00ea66886ca6bff74bbeaa49e7f5c75c275d5d7) ) // bad? its half the size of the kov one

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0600.rom",    0x0000000, 0x0800000, CRC(d8167834) SHA1(fa55a99629d03b2ea253392352f70d2c8639a991) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0601.rom",    0x0800000, 0x0800000, CRC(ff7a4373) SHA1(7def9fca7513ad5a117da230bebd2e3c78679041) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0602.rom",    0x1000000, 0x0800000, CRC(e7a32959) SHA1(3d0ed684dc5b269238890836b2ce7ef46aa5265b) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0603.rom",    0x1800000, 0x0400000, CRC(ec31abda) SHA1(ee526655369bae63b0ef0730e9768b765c9950fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0600.rom",    0x0000000, 0x0800000, CRC(7d3cd059) SHA1(00cf994b63337e0e4ebe96453daf45f24192af1c) )
	ROM_LOAD( "b0601.rom",    0x0800000, 0x0400000, CRC(a0bb1c2f) SHA1(0542348c6e27779e0a98de16f04f9c18158f2b28) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0600.rom",    0x400000, 0x400000, CRC(3ada4fd6) SHA1(4c87adb25d31cbd41f04fbffe31f7bc37173da76) )
ROM_END

ROM_START( photoy2k )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v104.16m",     0x100000, 0x200000, CRC(e051070f) SHA1(a5a1a8dd7542a30632501af8d02fda07475fd9aa) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x480000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0700.rom",    0x400000, 0x080000, CRC(93943b4d) SHA1(3b439903853727d45d62c781af6073024eb3c5a3) )

	ROM_REGION( 0x480000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x1080000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0700.l",    0x0000000, 0x0400000, CRC(26a9ae9c) SHA1(c977c89db6fdf47ee260ff687b80375caeab975c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0700.h",    0x0400000, 0x0400000, CRC(79bc1fc1) SHA1(a09472a9b75704c1d31ab828f92c2a5007b2b4ed) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.l",    0x0800000, 0x0400000, CRC(23607f81) SHA1(8b6dbcdce9b131370693847ed9771aa04b62711c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.h",    0x0c00000, 0x0400000, CRC(5f2efd37) SHA1(9a5bd9751691bc085b0751b9fa8ede9eb97b1248) )
	ROM_LOAD( "a0702.rom",  0x1000000, 0x0080000, CRC(42239e1b) SHA1(2b6d20958abf8a67ce525d5c8964b6d225ccaeda) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0700.l",    0x0000000, 0x0400000, CRC(af096904) SHA1(8e86b36cc44720ece68022e409279bf9144341ba) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "b0700.h",    0x0400000, 0x0400000, CRC(6d53de26) SHA1(f3f93fd2f87adb815834ba0242b94073fbb5e333) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "cgv101.rom", 0x0800000, 0x0020000, CRC(da02ec3e) SHA1(7ee21d748c9b932f53e790a9040167f904fecefc) )

	ROM_REGION( 0x480000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0700.rom",    0x400000, 0x080000, CRC(acc7afce) SHA1(ac2d344ebac336f0f363bb045dd8ea4e83d1fb50) )
ROM_END

/*

Real and Fake / Photo Y2K (JPN 102 Ver.)
(c)1999 ALTA / IGS

PGM system
IGS PCB NO-0220
IGS PCB NO-0221


V102.U4      [a65eda9f]
V102.U5      [9201621b]
V102.U6      [b9ca5504]
V102.U8      [3be22b8f]

T0700.U11


A0700.U2
A0701.U4

SP_V102.U5

B0700.U7

CG_V101.U3
CG_V101.U6

*/

ROM_START( raf102j )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "v102.u4",     0x100001, 0x080000, CRC(a65eda9f) SHA1(6307cacf4a262e781753eff14700a0455837780c) )
	ROM_LOAD16_BYTE( "v102.u6",     0x100000, 0x080000, CRC(b9ca5504) SHA1(058cf01316f233236ca9861349f515935283b75e) )
	ROM_LOAD16_BYTE( "v102.u5",     0x200001, 0x080000, CRC(9201621b) SHA1(1ca3ebe7eec40614bfa8b911657fa2b51f2c51a4) )
	ROM_LOAD16_BYTE( "v102.u8",     0x200000, 0x080000, CRC(3be22b8f) SHA1(03634fbd6a8a8369c6cb1fd6694a3784dac5bf59) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x480000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0700.rom",    0x400000, 0x080000, CRC(93943b4d) SHA1(3b439903853727d45d62c781af6073024eb3c5a3) )

	ROM_REGION( 0x480000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	/* all roms below need checking to see if they're the same on this board */
	ROM_REGION( 0x1080000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0700.l",    0x0000000, 0x0400000, CRC(26a9ae9c) SHA1(c977c89db6fdf47ee260ff687b80375caeab975c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0700.h",    0x0400000, 0x0400000, CRC(79bc1fc1) SHA1(a09472a9b75704c1d31ab828f92c2a5007b2b4ed) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.l",    0x0800000, 0x0400000, CRC(23607f81) SHA1(8b6dbcdce9b131370693847ed9771aa04b62711c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a0701.h",    0x0c00000, 0x0400000, CRC(5f2efd37) SHA1(9a5bd9751691bc085b0751b9fa8ede9eb97b1248) )
	ROM_LOAD( "a0702.rom",  0x1000000, 0x0080000, CRC(42239e1b) SHA1(2b6d20958abf8a67ce525d5c8964b6d225ccaeda) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0700.l",    0x0000000, 0x0400000, CRC(af096904) SHA1(8e86b36cc44720ece68022e409279bf9144341ba) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "b0700.h",    0x0400000, 0x0400000, CRC(6d53de26) SHA1(f3f93fd2f87adb815834ba0242b94073fbb5e333) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "cgv101.rom", 0x0800000, 0x0020000, CRC(da02ec3e) SHA1(7ee21d748c9b932f53e790a9040167f904fecefc) )

	ROM_REGION( 0x480000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0700.rom",    0x400000, 0x080000, CRC(acc7afce) SHA1(ac2d344ebac336f0f363bb045dd8ea4e83d1fb50) )
ROM_END

/*

The Killing Blade (English / World Version)
IGS, 1998

This is a cart for the IGS PGM system.


PCB Layout
----------

IGS PCB NO-0179
|-----------------------------------------------|
|                      8MHz  |--------|         |
|            32.768kHz|----| |        |T0300.U14|
|6164  6164           |IGS | | IGS025 |         |
|                     |022 | |        |         |
|*                    |----| |--------|         |
|                                               |
|           U2     U3     U4     U5     U6      |
| PAL   PAL                                PAL  |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      IGS022       - Custom IGS IC (QFP100)
      IGS025       - Custom IGS IC (PLCC68, labelled "ENGLISH")
      T0300.U14    - 32MBit MaskROM (SOP44, labelled "T0300")
      6164         - x2, 8K x8 SRAM (SOJ28)
      U2           - 27C512 512KBit EPROM (DIP28, labelled "KB U2 V104")
      U3           - 27C4000 4MBit EPROM (DIP32, labelled "KB U3 V104")
      U4           - 27C4000 4MBit EPROM (DIP32, labelled "KB U4 V104")
      U5           - 27C4000 4MBit EPROM (DIP32, labelled "KB U5 V104")
      U6           - 27C4000 4MBit EPROM (DIP32, labelled "KB U6 V104")
      PALs         - x3, labelled "DH U8", "DH U1", "DH U7"
      *            - Unpopulated position for DIP42 EPROM/MASKROM (labelled "P0300")


IGS PCB NO-0178
|-----------------------------------------------|
| U9    U10   U11    U12     U13     U14     U15|
|                                               |
|                                               |
|                                               |
| U1    U2                         74LS138      |
|                                  74LS139      |
|             U3     U4      U5              U8 |
|                                               |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|

Notes:
      U1           - 32MBit MASKROM (SOP44, labelled "M0300")
      U2           - 32MBit MASKROM (SOP44, labelled "A0307")
      U3           - 16MBit MASKROM (DIP42, labelled "A0302")
      U4           - 16MBit MASKROM (DIP42, labelled "A0304")
      U5           - 16MBit MASKROM (DIP42, labelled "A0305")
      U8           - 16MBit MASKROM (DIP42, labelled "B0301")
      U9           - 32MBit MASKROM (SOP44, labelled "A0300")
      U10          - 32MBit MASKROM (SOP44, labelled "A0301")
      U11          - 32MBit MASKROM (SOP44, labelled "A0303")
      U12          - 32MBit MASKROM (SOP44, labelled "A0306")
      U13          - 32MBit MASKROM (SOP44, labelled "B0300")
      U14          - 32MBit MASKROM (SOP44, labelled "B0302")
      U15          - 32MBit MASKROM (SOP44, labelled "B0303")

*/

ROM_START( killbld )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0300_v109.u9", 0x100000, 0x200000, CRC(2fcee215) SHA1(855281a9090bfdf3da9f4d50c121765131a13400) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* Protection Data */
	ROM_LOAD16_WORD_SWAP( "kb_u2.rom", 0x000000, 0x010000,  CRC(de3eae63) SHA1(03af767ef764055bda528b5cc6a24b9e1218cca8) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0300.u14",    0x400000, 0x400000, CRC(0922f7d9) SHA1(4302b4b7369e13f315fad14f7d6cad1321101d24) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x2000000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0300.u9",   0x0000000, 0x0400000,  CRC(3f9455d3) SHA1(3718ce00ad93975383aafc14e5a74dc297b011a1) )
	ROM_LOAD( "a0301.u10",  0x0400000, 0x0400000,  CRC(92776889) SHA1(6d677837fefff47bfd1c6166322f69f89989a5e2) )
	ROM_LOAD( "a0303.u11",  0x0800000, 0x0400000,  CRC(33f5cc69) SHA1(9cacd5058d4bb25b77f71658bbbbd4b38d0a6b6a) )
	ROM_LOAD( "a0306.u12",  0x0c00000, 0x0400000,  CRC(cc018a8e) SHA1(37752d46f238fb57c0ab5a4f96b1e013f2077347) )
	ROM_LOAD( "a0307.u2",   0x1000000, 0x0400000,  CRC(bc772e39) SHA1(079cc42a190cb916f02b59bca8fa90e524acefe9) )
//  ROM_LOAD( "a0302.u3",   0x1400000, 0x0200000,  CRC(a4810e38) SHA1(c31fe641feab2c93795fc35bf71d4f37af1056d4) ) // from lord of gun! unused..
//  ROM_LOAD( "a0304.u4",   0x1600000, 0x0200000,  CRC(3096de1c) SHA1(d010990d21cfda9cb8ab5b4bc0e329c23b7719f5) ) // from lord of gun! unused..
//  ROM_LOAD( "a0305.u5",   0x1800000, 0x0200000,  CRC(2234531e) SHA1(58a82e31a1c0c1a4dd026576319f4e7ecffd140e) ) // from lord of gun! unused..

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0300.u13",    0x0000000, 0x0400000, CRC(7f876981) SHA1(43555a200929ad5ecc42137fc9aeb42dc4f50d20) )
	ROM_LOAD( "b0302.u14",    0x0400000, 0x0400000, CRC(eea9c502) SHA1(04b3972c7111ea59a3cceab6ad124080c4ce3520) )
	ROM_LOAD( "b0303.u15",    0x0800000, 0x0200000, CRC(77a9652e) SHA1(2342f643d37945fbda224a5034c013796e5134ca) )
//  ROM_LOAD( "b0301.u8",     0x0a00000, 0x0200000, CRC(400abe33) SHA1(20de1eb626424ea41bd55eb3cecd6b50be744ee0) ) // from lord of gun! unused..

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0300.u1",     0x400000, 0x400000, CRC(93159695) SHA1(50c5976c9b681bd3d1ebefa3bfa9fe6e72dcb96f) )
ROM_END

ROM_START( killbld104 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "kb_u3_v104.u3",     0x100001, 0x080000, CRC(6db1d719) SHA1(804002f014d275aaf0368fb7f904938fe4ac07ee) )
	ROM_LOAD16_BYTE( "kb_u6_v104.u6",     0x100000, 0x080000, CRC(31ecc978) SHA1(82666d534e4151775063af6d39f575faba0f1047) )
	ROM_LOAD16_BYTE( "kb_u4_v104.u4",     0x200001, 0x080000, CRC(1ed8b2e7) SHA1(331c037640cfc1fe743cd0e65a1156c470b3303e) )
	ROM_LOAD16_BYTE( "kb_u5_v104.u5",     0x200000, 0x080000, CRC(a0bafc29) SHA1(b20db7c16353c6f87ed3c08c9d037b07336711f1) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* Protection Data */
	ROM_LOAD16_WORD_SWAP( "kb_u2_v104.u2", 0x000000, 0x010000,  CRC(c970f6d5) SHA1(399fc6f80262784c566363c847dc3fdc4fb37494) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0300.u14",    0x400000, 0x400000, CRC(0922f7d9) SHA1(4302b4b7369e13f315fad14f7d6cad1321101d24) )

	ROM_REGION( 0x800000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x2000000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0300.u9",   0x0000000, 0x0400000,  CRC(3f9455d3) SHA1(3718ce00ad93975383aafc14e5a74dc297b011a1) )
	ROM_LOAD( "a0301.u10",  0x0400000, 0x0400000,  CRC(92776889) SHA1(6d677837fefff47bfd1c6166322f69f89989a5e2) )
	ROM_LOAD( "a0303.u11",  0x0800000, 0x0400000,  CRC(33f5cc69) SHA1(9cacd5058d4bb25b77f71658bbbbd4b38d0a6b6a) )
	ROM_LOAD( "a0306.u12",  0x0c00000, 0x0400000,  CRC(cc018a8e) SHA1(37752d46f238fb57c0ab5a4f96b1e013f2077347) )
	ROM_LOAD( "a0307.u2",   0x1000000, 0x0400000,  CRC(bc772e39) SHA1(079cc42a190cb916f02b59bca8fa90e524acefe9) )
//  ROM_LOAD( "a0302.u3",   0x1400000, 0x0200000,  CRC(a4810e38) SHA1(c31fe641feab2c93795fc35bf71d4f37af1056d4) ) // from lord of gun! unused..
//  ROM_LOAD( "a0304.u4",   0x1600000, 0x0200000,  CRC(3096de1c) SHA1(d010990d21cfda9cb8ab5b4bc0e329c23b7719f5) ) // from lord of gun! unused..
//  ROM_LOAD( "a0305.u5",   0x1800000, 0x0200000,  CRC(2234531e) SHA1(58a82e31a1c0c1a4dd026576319f4e7ecffd140e) ) // from lord of gun! unused..

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0300.u13",    0x0000000, 0x0400000, CRC(7f876981) SHA1(43555a200929ad5ecc42137fc9aeb42dc4f50d20) )
	ROM_LOAD( "b0302.u14",    0x0400000, 0x0400000, CRC(eea9c502) SHA1(04b3972c7111ea59a3cceab6ad124080c4ce3520) )
	ROM_LOAD( "b0303.u15",    0x0800000, 0x0200000, CRC(77a9652e) SHA1(2342f643d37945fbda224a5034c013796e5134ca) )
//  ROM_LOAD( "b0301.u8",     0x0a00000, 0x0200000, CRC(400abe33) SHA1(20de1eb626424ea41bd55eb3cecd6b50be744ee0) ) // from lord of gun! unused..

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0300.u1",     0x400000, 0x400000, CRC(93159695) SHA1(50c5976c9b681bd3d1ebefa3bfa9fe6e72dcb96f) )
ROM_END

ROM_START( puzlstar )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) ) // (BIOS)
	ROM_LOAD16_BYTE( "v100mg.u1",     0x100001, 0x080000, CRC(5788b77d) SHA1(7770aae6e686da92b2623c977d1bc8f019f48267) )
	ROM_LOAD16_BYTE( "v100mg.u2",     0x100000, 0x080000, CRC(4c79d979) SHA1(3b92052a35994f2b3dd164930154184c45d5e2d0) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, ROMREGION_ERASEFF ) /* ARM protection ASIC - internal rom */
	/* this has no external rom so the internal rom probably can't be dumped */
//  ROM_LOAD( "puzlstar_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0800.u5",    0x400000, 0x200000, CRC(f9d84e59) SHA1(80ec77025ac5bf355b1a60f2a678dd4c56071f6b) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0800.u1",    0x0000000, 0x0400000, CRC(e1e6ec40) SHA1(390432431f144ef63424a426582b311765a61771) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0800.u3",    0x0000000, 0x0200000, CRC(52e7bef5) SHA1(a678251b7e46a1016d0afc1d8d5c9928008ad5b1) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0800.u2",    0x400000, 0x400000,  CRC(e1a46541) SHA1(6fe9de5700d8638374734d80551dcedb62975140) )
ROM_END


/*

Oriental Legend Super
IGS, 1998

This is a cart for the IGS PGM system.

PCB Layout
----------
IGS PCB NO-0191-1
|-----------------------------------------------|
|6264                 8MHz|--------|            |
|6264                     |        |   T0500.U18|
|                         | IGS025 |            |
|                 |-----| |        |   T0501.U19|
|                 | IGS | |--------|            |
|                 | 028 |                       |
|        *1       |-----|           V101.U1     |
|              V101.U2   V101.U4  PAL      PAL  |
|  V101.U6          V101.U3   V101.U5           |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      IGS028       - Custom IGS IC (QFP100)
      IGS025       - Custom IGS IC (PLCC68, labelled "KOREA")
      T0500.U18    - 32MBit MaskROM (SOP44)
      T0501.U19    - 16MBit MaskROM (SOP44)
      V101.U1      - MX27C4096 4MBit EPROM (DIP40)
      V101.U2/3/4/5- MX27C4000 4MBit EPROM (DIP32)
      PALs         - x2, labelled "CW-2 U8", "CW-2 U7"
      6264         - 8K x8 SRAM
      *1           - Unpopulated position for SOP44 MASKROM labelled "P0500"


IGS PCB NO-0135
|-----------------------------------------------|
|A0504.U11        A0506.U13     B0502.U15       |
|         A0505.U12         U14        B0503.U16|
|                                               |
|A0500.U5                       B0500.U9        |
|         A0501.U6       A0503.U8      B0501.U10|
|                 A0502.U7                      |
|                                               |
|74LS138          M0500.U1               74LS139|
|                           U2                  |
|-|                                           |-|
  |--------------------||---------------------|

Notes:
      This PCB contains only SOP44 MaskROMS and 2 logic IC's
      U2 and U14 are not populated.
      All are 32MBit except M0500 which is 16MBit.

*/

ROM_START( olds )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "sp_v101.u2",      0x100001, 0x080000,   CRC(08eb9661) SHA1(105946e72e562adb1a9fd794ca0fd2c91967eb56) )
	ROM_LOAD16_BYTE( "sp_v101.u3",      0x100000, 0x080000,   CRC(0a358c1e) SHA1(95c7c3f069c5d05001e22535750f6b3cd7de105f) )
	ROM_LOAD16_BYTE( "sp_v101.u4",      0x200001, 0x080000,   CRC(766570e0) SHA1(e7c3f5664ec69b662b82c2e1375555db7305390c) )
	ROM_LOAD16_BYTE( "sp_v101.u5",      0x200000, 0x080000,   CRC(58662e12) SHA1(2b39bd847e9c4968a8e77a2f3cec77cf323ceee3) )
	ROM_LOAD16_WORD_SWAP( "sp_v101.u1",0x300000, 0x080000,    CRC(2b2f4f1e) SHA1(67b97cf8cc7f517d67cd45588addd2ad8e24612a) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* ASIC25? Protection Data */
	ROM_LOAD( "sp_v101.u6", 0x000000, 0x010000,  CRC(097046bc) SHA1(6d75db85cf4c79b63e837897785c253014b2126d) )

//	ROM_REGION( 0x200000, REGION_USER2, ROMREGION_ERASEFF ) /* its a dump of the shared protection rom/ram from pcb. */
	// clearly not for this revision
	//ROM_LOAD16_WORD_SWAP( "ram_dump", 0x000000, 0x200000, CRC(e7b26aea) SHA1(17d101f760d790619ce4858984787b494bdbbc8a) )

	ROM_REGION( 0xc00000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0500.rom",    0x400000, 0x400000, CRC(d881726c) SHA1(a82517e665996f7b7017c940f1fcf016fccb65c2) )
	ROM_LOAD( "t0501.rom",    0x800000, 0x200000, CRC(d2106864) SHA1(65d827135b87d82196433aea3279608ee263feca) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0500.rom",    0x0000000, 0x400000, CRC(80a59197) SHA1(7d0108e2f0d0347d43ace2d96c0388202c05fdfb) )
	ROM_LOAD( "a0501.rom",    0x0400000, 0x400000, CRC(98c931b1) SHA1(9b70d1a7beb4c3a0c1436c25fd1fb76e47539538) )
	ROM_LOAD( "a0502.rom",    0x0800000, 0x400000, CRC(c3fcdf1d) SHA1(107585fd103fcd0af0fb7db283be4f7c7058aef7) )
	ROM_LOAD( "a0503.rom",    0x0c00000, 0x400000, CRC(066dffec) SHA1(f023032a7202b7090fb609a39e0f19018e664bf3) )
	ROM_LOAD( "a0504.rom",    0x1000000, 0x400000, CRC(45337583) SHA1(c954d0e5bf7fa99c90b0d154e7119d2b0c461f1c) )
	ROM_LOAD( "a0505.rom",    0x1400000, 0x400000, CRC(5b8cf3a5) SHA1(856d1e47b5d9a66dcfbdc74a51ed646fd7d96a35) )
	ROM_LOAD( "a0506.rom",    0x1800000, 0x400000, CRC(087ac60c) SHA1(3d5bf7dd40c8a3c1224cf82e12410ca904c0c5db) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0500.rom",    0x0000000, 0x400000, CRC(cde07f74) SHA1(1fe90db7c57faa28f3a054b2c07909bef89e3efb) )
	ROM_LOAD( "b0501.rom",    0x0400000, 0x400000, CRC(1546c2e9) SHA1(a7b9c8b44203db54a59d49fe469bb52bba807ba2) )
	ROM_LOAD( "b0502.rom",    0x0800000, 0x400000, CRC(e97b31c3) SHA1(1a7ca4f6c8644e84a33ae41cd4637f21046b14c5) )
	ROM_LOAD( "b0503.u16",    0x0c00000, 0x400000, CRC(e41d98e4) SHA1(f80b27fcee81762993e09bf1b3cad6e85274760c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0500.rom",    0x400000, 0x200000, CRC(37928cdd) SHA1(e80498cabc2a6a54d4f3ebcb097d4b3fad96fe55) )
ROM_END

ROM_START( olds100 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_BYTE( "v100-u2.040",      0x100001, 0x080000,  CRC(517c2a06) SHA1(bbf5b311fac9b0bb4d4129c0561e5e24f6963fa2) )
	ROM_LOAD16_BYTE( "v100-u3.040",      0x100000, 0x080000,  CRC(d0e2b741) SHA1(2e671dbb4320d1f0c059b35efd33cdea26f12131) )
	ROM_LOAD16_BYTE( "v100-u4.040",      0x200001, 0x080000,  CRC(32a6bdbd) SHA1(a93d7f4eae722a58eca9ec351ad5890cefda56f0) )
	ROM_LOAD16_BYTE( "v100-u5.040",      0x200000, 0x080000,  CRC(b4a1cafb) SHA1(b2fccd480ede93f58ad043387b18b898152f06ef) )
    ROM_LOAD16_WORD_SWAP( "v100-u1.040", 0x300000, 0x080000,  CRC(37ea4e75) SHA1(a94fcb89da3394a43d360f885419677f511d2580) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

//	ROM_REGION( 0x200000, REGION_USER2, ROMREGION_ERASEFF ) /* its a dump of the shared protection rom/ram from pcb. */
	// used to simulate encrypted DMA protection device for now ..
	//ROM_LOAD16_WORD_SWAP( "ram_dump", 0x000000, 0x200000, CRC(e7b26aea) SHA1(17d101f760d790619ce4858984787b494bdbbc8a) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* ASIC25? Protection Data */
	ROM_LOAD( "kd-u6.512", 0x000000, 0x010000,  CRC(e7613dda) SHA1(0d7c043b90e2f9a36a45066f22e3e305dc716676) )

	ROM_REGION( 0xc00000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0500.rom",    0x400000, 0x400000, CRC(d881726c) SHA1(a82517e665996f7b7017c940f1fcf016fccb65c2) )
	ROM_LOAD( "t0501.rom",    0x800000, 0x200000, CRC(d2106864) SHA1(65d827135b87d82196433aea3279608ee263feca) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0500.rom",    0x0000000, 0x400000, CRC(80a59197) SHA1(7d0108e2f0d0347d43ace2d96c0388202c05fdfb) )
	ROM_LOAD( "a0501.rom",    0x0400000, 0x400000, CRC(98c931b1) SHA1(9b70d1a7beb4c3a0c1436c25fd1fb76e47539538) )
	ROM_LOAD( "a0502.rom",    0x0800000, 0x400000, CRC(c3fcdf1d) SHA1(107585fd103fcd0af0fb7db283be4f7c7058aef7) )
	ROM_LOAD( "a0503.rom",    0x0c00000, 0x400000, CRC(066dffec) SHA1(f023032a7202b7090fb609a39e0f19018e664bf3) )
	ROM_LOAD( "a0504.rom",    0x1000000, 0x400000, CRC(45337583) SHA1(c954d0e5bf7fa99c90b0d154e7119d2b0c461f1c) )
	ROM_LOAD( "a0505.rom",    0x1400000, 0x400000, CRC(5b8cf3a5) SHA1(856d1e47b5d9a66dcfbdc74a51ed646fd7d96a35) )
	ROM_LOAD( "a0506.rom",    0x1800000, 0x400000, CRC(087ac60c) SHA1(3d5bf7dd40c8a3c1224cf82e12410ca904c0c5db) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0500.rom",    0x0000000, 0x400000, CRC(cde07f74) SHA1(1fe90db7c57faa28f3a054b2c07909bef89e3efb) )
	ROM_LOAD( "b0501.rom",    0x0400000, 0x400000, CRC(1546c2e9) SHA1(a7b9c8b44203db54a59d49fe469bb52bba807ba2) )
	ROM_LOAD( "b0502.rom",    0x0800000, 0x400000, CRC(e97b31c3) SHA1(1a7ca4f6c8644e84a33ae41cd4637f21046b14c5) )
	ROM_LOAD( "b0503.u16",    0x0c00000, 0x400000, CRC(e41d98e4) SHA1(f80b27fcee81762993e09bf1b3cad6e85274760c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0500.rom",    0x400000, 0x200000, CRC(37928cdd) SHA1(e80498cabc2a6a54d4f3ebcb097d4b3fad96fe55) )
ROM_END

/* this is the set which the protection ram dump seems to be for.. */
ROM_START( olds100a )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	/* this rom had a lame hack applied to it by the dumper, this was removed, hopefully it is correct now */
	ROM_LOAD16_WORD_SWAP( "p0500.v10",    0x100000, 0x400000, CRC(8981fc87) SHA1(678d6705d06b99bca5951ff77708adadc4c4396b) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x010000, REGION_USER1, ROMREGION_ERASEFF ) /* ASIC25? Protection Data */
	/* missing from this set .. */
	ROM_LOAD( "protection_data.u6", 0x000000, 0x010000, NO_DUMP )

//	ROM_REGION( 0x200000, REGION_USER2, ROMREGION_ERASEFF ) /* its a dump of the shared protection rom/ram from pcb. */
	// used to simulate encrypted DMA protection device for now ..
//	ROM_LOAD16_WORD_SWAP( "ram_dump", 0x000000, 0x200000, CRC(e7b26aea) SHA1(17d101f760d790619ce4858984787b494bdbbc8a) )

	ROM_REGION( 0xc00000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0500.rom",    0x400000, 0x400000, CRC(d881726c) SHA1(a82517e665996f7b7017c940f1fcf016fccb65c2) )
	ROM_LOAD( "t0501.rom",    0x800000, 0x200000, CRC(d2106864) SHA1(65d827135b87d82196433aea3279608ee263feca) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprite Colour Data */
	ROM_LOAD( "a0500.rom",    0x0000000, 0x400000, CRC(80a59197) SHA1(7d0108e2f0d0347d43ace2d96c0388202c05fdfb) )
	ROM_LOAD( "a0501.rom",    0x0400000, 0x400000, CRC(98c931b1) SHA1(9b70d1a7beb4c3a0c1436c25fd1fb76e47539538) )
	ROM_LOAD( "a0502.rom",    0x0800000, 0x400000, CRC(c3fcdf1d) SHA1(107585fd103fcd0af0fb7db283be4f7c7058aef7) )
	ROM_LOAD( "a0503.rom",    0x0c00000, 0x400000, CRC(066dffec) SHA1(f023032a7202b7090fb609a39e0f19018e664bf3) )
	ROM_LOAD( "a0504.rom",    0x1000000, 0x400000, CRC(45337583) SHA1(c954d0e5bf7fa99c90b0d154e7119d2b0c461f1c) )
	ROM_LOAD( "a0505.rom",    0x1400000, 0x400000, CRC(5b8cf3a5) SHA1(856d1e47b5d9a66dcfbdc74a51ed646fd7d96a35) )
	ROM_LOAD( "a0506.rom",    0x1800000, 0x400000, CRC(087ac60c) SHA1(3d5bf7dd40c8a3c1224cf82e12410ca904c0c5db) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0500.rom",    0x0000000, 0x400000, CRC(cde07f74) SHA1(1fe90db7c57faa28f3a054b2c07909bef89e3efb) )
	ROM_LOAD( "b0501.rom",    0x0400000, 0x400000, CRC(1546c2e9) SHA1(a7b9c8b44203db54a59d49fe469bb52bba807ba2) )
	ROM_LOAD( "b0502.rom",    0x0800000, 0x400000, CRC(e97b31c3) SHA1(1a7ca4f6c8644e84a33ae41cd4637f21046b14c5) )
	ROM_LOAD( "b0503.u16",    0x0c00000, 0x400000, CRC(e41d98e4) SHA1(f80b27fcee81762993e09bf1b3cad6e85274760c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0500.rom",    0x400000, 0x200000, CRC(37928cdd) SHA1(e80498cabc2a6a54d4f3ebcb097d4b3fad96fe55) )
ROM_END

ROM_START( olds103t )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code  */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )// (BIOS)
	ROM_LOAD16_WORD_SWAP( "p0500.v103",   0x100000, 0x400000, CRC(17e32e14) SHA1(b8f731087af2c59fe5b1da31f1cb055d35c8b440) )

	/* CPU2 = Z80, romless, code uploaded by 68k */
	
	ROM_REGION( 0xc00000, REGION_GFX1,  ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0500.rom",    0x400000, 0x400000, CRC(d881726c) SHA1(a82517e665996f7b7017c940f1fcf016fccb65c2) )
	ROM_LOAD( "t0501.rom",    0x800000, 0x200000, CRC(d2106864) SHA1(65d827135b87d82196433aea3279608ee263feca) )
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, ROMREGION_DISPOSE )/* Sprite Colour Data */
	ROM_LOAD( "a0500.rom",    0x0000000, 0x400000, CRC(80a59197) SHA1(7d0108e2f0d0347d43ace2d96c0388202c05fdfb) )
	ROM_LOAD( "a0501.rom",    0x0400000, 0x400000, CRC(98c931b1) SHA1(9b70d1a7beb4c3a0c1436c25fd1fb76e47539538) )
	ROM_LOAD( "a0502.rom",    0x0800000, 0x400000, CRC(c3fcdf1d) SHA1(107585fd103fcd0af0fb7db283be4f7c7058aef7) )
	ROM_LOAD( "a0503.rom",    0x0c00000, 0x400000, CRC(066dffec) SHA1(f023032a7202b7090fb609a39e0f19018e664bf3) )
	ROM_LOAD( "a0504.rom",    0x1000000, 0x400000, CRC(45337583) SHA1(c954d0e5bf7fa99c90b0d154e7119d2b0c461f1c) )
	ROM_LOAD( "a0505.rom",    0x1400000, 0x400000, CRC(5b8cf3a5) SHA1(856d1e47b5d9a66dcfbdc74a51ed646fd7d96a35) )
	ROM_LOAD( "a0506.rom",    0x1800000, 0x400000, CRC(087ac60c) SHA1(3d5bf7dd40c8a3c1224cf82e12410ca904c0c5db) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0500.rom",    0x0000000, 0x400000, CRC(cde07f74) SHA1(1fe90db7c57faa28f3a054b2c07909bef89e3efb) )
	ROM_LOAD( "b0501.rom",    0x0400000, 0x400000, CRC(1546c2e9) SHA1(a7b9c8b44203db54a59d49fe469bb52bba807ba2) )
	ROM_LOAD( "b0502.rom",    0x0800000, 0x400000, CRC(e97b31c3) SHA1(1a7ca4f6c8644e84a33ae41cd4637f21046b14c5) )
	ROM_LOAD( "b0503.u16",    0x0c00000, 0x400000, CRC(e41d98e4) SHA1(f80b27fcee81762993e09bf1b3cad6e85274760c) )

	ROM_REGION( 0x1000000,  REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0500.rom",    0x400000, 0x200000, CRC(37928cdd) SHA1(e80498cabc2a6a54d4f3ebcb097d4b3fad96fe55) )
ROM_END

ROM_START( kov2 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "igs_u18.rom",    0x100000, 0x400000, CRC(86205879) SHA1(f73d5b70b41d39be1cac75e474b025de2cce0b01) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
	ROM_LOAD( "sango2.rom", 0x000000, 0x04000,  CRC(e0d7679f) SHA1(e1c2d127eba4ddbeb8ad173c55b90ac1467e1ca8) )

	ROM_REGION( 0x400000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data) */
	ROM_LOAD( "igs_u19.rom", 0x000000, 0x200000,   CRC(edd59922) SHA1(09b14f20f685944a93292c83e5830849aade42c9) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t1200.rom",    0x400000, 0x800000, CRC(d7e26609) SHA1(bdad810f82fcf1d50a8791bdc495374ec5a309c6) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x2800000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a1200.rom",    0x0000000, 0x0800000, CRC(ceeb81d8) SHA1(5476729443fc1bc9593ae10fbf7cbc5d7290b017) )
	ROM_LOAD( "a1201.rom",    0x0800000, 0x0800000, CRC(82f0a878) SHA1(ddd13e404252a71de1b2b3b974b910f899f51c38) )
	ROM_LOAD( "a1202.rom",    0x1000000, 0x0800000, CRC(4bb92fae) SHA1(f0b6d72ed425de1c69dc8f8d5795ea760a4a59b0) )
	ROM_LOAD( "a1203.rom",    0x1800000, 0x0800000, CRC(e73cb627) SHA1(4c6e48b845a5d1e8f9899010fbf273d54c2b8899) )
	ROM_LOAD( "a1204.rom",    0x2000000, 0x0800000, CRC(27527099) SHA1(e23cf366bdeaca1e009a5cec6b13164310a34384) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b1200.rom",   0x0000000, 0x0800000,  CRC(bed7d994) SHA1(019dfba8154256d64cd249eb0fa4c451edce34b8) )
	ROM_LOAD( "b1201.rom",   0x0800000, 0x0800000,  CRC(f251eb57) SHA1(56a5fc14ab7822f83379cecb26638e5bb266349a) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m1200.rom",    0x800000, 0x800000, CRC(b0d88720) SHA1(44ab137e3f8e15b7cb5697ffbd9b1143d8210c4f) )
ROM_END


ROM_START( kov2106 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "u18.106",    0x100000, 0x400000, CRC(40051ad9) SHA1(ba2ddf267fe688d5dfed575aeeccbab10135b37b) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
	ROM_LOAD( "sango2.rom", 0x000000, 0x04000,  CRC(e0d7679f) SHA1(e1c2d127eba4ddbeb8ad173c55b90ac1467e1ca8) )

	ROM_REGION( 0x400000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data) */
	ROM_LOAD( "u19.102", 0x000000, 0x200000,   CRC(462e2980) SHA1(3da7c3d2c65b59f50c78be1c25922b71d40f6080) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t1200.rom",    0x400000, 0x800000, CRC(d7e26609) SHA1(bdad810f82fcf1d50a8791bdc495374ec5a309c6) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x2800000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a1200.rom",    0x0000000, 0x0800000, CRC(ceeb81d8) SHA1(5476729443fc1bc9593ae10fbf7cbc5d7290b017) )
	ROM_LOAD( "a1201.rom",    0x0800000, 0x0800000, CRC(82f0a878) SHA1(ddd13e404252a71de1b2b3b974b910f899f51c38) )
	ROM_LOAD( "a1202.rom",    0x1000000, 0x0800000, CRC(4bb92fae) SHA1(f0b6d72ed425de1c69dc8f8d5795ea760a4a59b0) )
	ROM_LOAD( "a1203.rom",    0x1800000, 0x0800000, CRC(e73cb627) SHA1(4c6e48b845a5d1e8f9899010fbf273d54c2b8899) )
	ROM_LOAD( "a1204.rom",    0x2000000, 0x0800000, CRC(27527099) SHA1(e23cf366bdeaca1e009a5cec6b13164310a34384) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b1200.rom",   0x0000000, 0x0800000,  CRC(bed7d994) SHA1(019dfba8154256d64cd249eb0fa4c451edce34b8) )
	ROM_LOAD( "b1201.rom",   0x0800000, 0x0800000,  CRC(f251eb57) SHA1(56a5fc14ab7822f83379cecb26638e5bb266349a) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m1200.rom",    0x800000, 0x800000, CRC(b0d88720) SHA1(44ab137e3f8e15b7cb5697ffbd9b1143d8210c4f) )
ROM_END


ROM_START( kov2p )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v204-32m.rom",    0x100000, 0x400000, CRC(583e0650) SHA1(2e5656dd9c6cba9f84af9baa3f5f70cdccf9db47) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
    ROM_LOAD( "kov2p_igs027a_china.bin", 0x000000, 0x04000, CRC(19a0bd95) SHA1(83e9f22512832a51d41c588debe8be7adb3b1df7) )

	ROM_REGION( 0x400000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data) */
	ROM_LOAD( "v200-16.rom", 0x000000, 0x200000,  CRC(16a0c11f) SHA1(ce449cef76ebd5657d49b57951e2eb0f132e203e) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t1200.rom",    0x400000, 0x800000, CRC(d7e26609) SHA1(bdad810f82fcf1d50a8791bdc495374ec5a309c6) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x2800000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a1200.rom",    0x0000000, 0x0800000, CRC(ceeb81d8) SHA1(5476729443fc1bc9593ae10fbf7cbc5d7290b017) )
	ROM_LOAD( "a1201.rom_p",  0x0800000, 0x0800000, CRC(21063ca7) SHA1(cf561b44902425a920d5cbea5bf65dd9530b2289) ) // either this or a1201.rom in kov2 is probably bad
	ROM_LOAD( "a1202.rom",    0x1000000, 0x0800000, CRC(4bb92fae) SHA1(f0b6d72ed425de1c69dc8f8d5795ea760a4a59b0) )
	ROM_LOAD( "a1203.rom",    0x1800000, 0x0800000, CRC(e73cb627) SHA1(4c6e48b845a5d1e8f9899010fbf273d54c2b8899) )
	ROM_LOAD( "a1204.rom_p",  0x2000000, 0x0200000, CRC(14b4b5bb) SHA1(d7db5740eec971f2782fb2885ee3af8f2a796550) ) // either this or a1204.rom in kov2 is probably bad

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b1200.rom",   0x0000000, 0x0800000,  CRC(bed7d994) SHA1(019dfba8154256d64cd249eb0fa4c451edce34b8) )
	ROM_LOAD( "b1201.rom",   0x0800000, 0x0800000,  CRC(f251eb57) SHA1(56a5fc14ab7822f83379cecb26638e5bb266349a) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m1200.rom",    0x800000, 0x800000, CRC(b0d88720) SHA1(44ab137e3f8e15b7cb5697ffbd9b1143d8210c4f) )
ROM_END

/*

Do Donpachi II
Cave, 2001

This is a PGM cart containing not a lot....
5x SOP44 mask ROMs (4x 64M, 1x 32M)
2x EPROMs (1x 4M, 1x 16M)
2x PALs (labelled FN U14 and FN U15)
1x custom IGS027A (QFP120)
3x RAMs WINBOND W24257AJ-8N
Some logic IC's, resistors, caps etc.

*/

ROM_START( ddp2 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v100.u8",    0x100000, 0x200000, CRC(0c8aa8ea) SHA1(57e33224622607a1df8daabf26ba063cf8a6d3fc) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
        ROM_LOAD( "ddp2_igs027a_japan.bin", 0x000000, 0x04000, CRC(742d34d2) SHA1(4491c08f3cefef2933ad5a741f4bb05cc2f3e1a0) )

	ROM_REGION( 0x400000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data, internal missing) */
	ROM_LOAD( "v100.u23", 0x000000, 0x20000, CRC(06c3dd29) SHA1(20c9479f158467fc2037dcf162b6c6be18c91d46) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t1300.u21",    0x400000, 0x800000, CRC(e748f0cb) SHA1(5843bee3a17c33648ce904af2b98c6a90aff7393) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1000000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a1300.u1",    0x0000000, 0x0800000, CRC(fc87a405) SHA1(115c21ecc56997652e527c92654076870bc9fa51) ) // FIXED BITS (xxxxxxxx1xxxxxxx)
	ROM_LOAD( "a1301.u2",    0x0800000, 0x0800000, CRC(0c8520da) SHA1(390317857ae5baa94a4cc042874b00a811f06a63) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x0800000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b1300.u7",    0x0000000, 0x0800000,  CRC(ef646604) SHA1(d737ff513792962f18df88c2caa9dd71de449079) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m1300.u5",    0x400000, 0x400000, CRC(82d4015d) SHA1(d4cdc1aec1c97cf23ff7a20ccaad822962e66ffa) )
ROM_END

/*

Puzzli 2
IGS, 2001

Cart for IGS PGM system. The layout of the PCB is virtually identical to Puzzle Star.

PCB Layout
----------

IGS PCB NO- 0259
|-----------------------------------------------|
|                        U6 U7                  |
|         |-------|                             |
|         |IGS027A|                             |
|         |       |                     T0900.U9|
|         |       |                             |
|         |-------|                             |
|          2SP_V200.U3                          |
|          2SP_V200.U4    U5   PAL              |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      IGS027A     - Custom IGS IC, ARM7/9? based CPU (QFP120, stamped 'IGS027A')
      T0900.U9    - 16MBit MaskROM (SOP44)
      2SP_V200.U3 - MX27C4000 512K x8 EPROM (DIP32, labelled '2SP V200 U3')
      2SP_V200.U4 - MX27C4000 512K x8 EPROM (DIP32, labelled '2SP V200 U4')
      PAL         - AMD PALCE22V10 PAL (DIP24, labelled 'EL U8')
      U5          - Unpopulated position for 16MBit MaskROM (DIP42)
      U6, U7      - Unpopulated position for 74LS245 logic chip (x2)


IGS PCB NO- 0258
|-----------------------------------------------|
|                                               |
|                                               |
|                                               |
|                                               |
|   *    M0900.U2   A0900.U3   B0900.U4         |
|                                               |
|                                               |
|                                               |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      *  - Unpopulated position for Oki MSM27C3202CZ 32MBit MaskROM (TSOP48 Type II)
      U2 - 32MBit MaskROM (DIP42, Byte mode)
      U3 - 32MBit MaskROM (SOP44)
      U4 - 16MBit MaskROM (SOP44)

*/
ROM_START( puzzli2 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_BYTE( "2sp_v200.u3",     0x100001, 0x080000, CRC(2a5ba8a6) SHA1(4c87b849fd6f39152e3e2ef699b78ce24b3fb6d0) )
	ROM_LOAD16_BYTE( "2sp_v200.u4",     0x100000, 0x080000, CRC(fa5c86c1) SHA1(11c219722b891b775c0f7f9bc8276cdd8f74d657) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, ROMREGION_ERASEFF ) /* ARM protection ASIC - internal rom */
	/* this has no external rom so the internal rom probably can't be dumped */
//  ROM_LOAD( "puzzli2_igs027a.bin", 0x000000, 0x04000, NO_DUMP )


	ROM_REGION( 0x600000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t0900.u9",    0x400000, 0x200000, CRC(70615611) SHA1(a46d4aa71396947b427f9ba4ba0e636876c09d6b) )

	ROM_REGION( 0x600000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a0900.u3",    0x0000000, 0x0400000, CRC(14911251) SHA1(e0d10ef50c408dbcf0907f81d4f0e49aeb651a6c) ) // FIXED BITS (xxxxxxxx1xxxxxxx)

	ROM_REGION( 0x0200000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b0900.u4",    0x0000000, 0x0200000,  CRC(6f0638b6) SHA1(14b315fe9e80b3314bb63487e6ea9ce04c9703bd) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m0900.u2",    0x400000, 0x400000, CRC(9ea7af2e) SHA1(d2593d391a93c5cf5a554750c32886dea6599b3d) )
ROM_END


/*

Martial Masters
IGS, 2001

Cart for IGS PGM system. This game is a straight rip-off of any of the
late side-by-side fighting games made by SNK or Capcom such as King Of Fighters
or Super Street Fighter II etc


PCB Layout
----------

IGS PCB-0293-01
|-----------------------------------------------|
| 62256              62256         IGS027A      |
| 62256                                         |
|                      *                        |
|                                               |
| PAL                                           |
|                                               |
| PAL             V102_16M.U10  T1000.U3        |
|                                               |
|                 V104_32M.U9              22MHz|
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      62256        - 32K x8 SRAM (SOJ28)
      IGS027A      - Custom IGS IC, ARM7 based CPU with internal 64K ROM (QFP120)
      T1000.U3     - 23C6410 64MBit MaskROM (SOP44)
      V102_16M.U10 - MX29F1610MC 16MBit SOP44 FlashROM mounted onto a tiny DIP42 to SOP44 adapter board
                     (manufactured by IGS) which is plugged into a standard DIP42 socket. This chip was
                     read directly on the adapter as a 27C160 EPROM. The socket is wired to accept 32MBit
                     DIP42 EPROMs.
      V104_32M.U9  - M27C3202CZ 32MBit TSOP48 Type II OTP MaskROM mounted onto a tiny DIP42 to TSOP48 Type II
                     adapter board (manufactured by IGS) which is plugged into a standard DIP42 socket. This
                     chip was read directly on the adapter as a 27C322 EPROM. The socket is wired to accept
                     32MBit DIP42 EPROMs.
      *            - Unpopulated position for 62256 SRAM


IGS PCB-0292-00
|-----------------------------------------------|
| A1000.U3         A1002.U6           A1004.U10 |
|          A1001.U4         A1003.U8            |
|                                               |
|                                               |
|                                               |
|                                               |
|                  M1001.U7           B1001.U11 |
|          M1000.U5         B1000.U9            |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|



*/

ROM_START( martmast )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v104_32m.u9",    0x100000, 0x400000, CRC(cfd9dff4) SHA1(328eaf6ac49a73265ee4e0f992b1b1312f49877b) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
	ROM_LOAD( "mmasters_arm.bin", 0x000000, 0x04000, BAD_DUMP CRC(a6c0828c) SHA1(0a5bda56dca264c3c7ff7698b8f699563f203c4d) ) // not verified, could be bad

	ROM_REGION( 0x400000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data) */
	ROM_LOAD( "v102_16m.u10", 0x000000, 0x200000,  CRC(18b745e6) SHA1(7bcb58dd3a2d6072f492cf0dd7181cb061c1f49d) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t1000.u3",    0x400000, 0x800000, CRC(bbf879b5) SHA1(bd9a6aea34ad4001e89e62ff4b7a2292eb833c00) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x2800000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a1000.u3",    0x0000000, 0x0800000, CRC(43577ac8) SHA1(6eea8b455985d5bac74dcc9943cdc3c0902de6cc) )
	ROM_LOAD( "a1001.u4",    0x0800000, 0x0800000, CRC(fe7a476f) SHA1(a8c7f1f0dd3e53141aed6d927eb88a3ceebb81e4) )
	ROM_LOAD( "a1002.u6",    0x1000000, 0x0800000, CRC(62e33d38) SHA1(96163d583e25073594f8413ce263e56b66bd69a1) )
	ROM_LOAD( "a1003.u8",    0x1800000, 0x0800000, CRC(b2c4945a) SHA1(7b18287a2db56db3651cfd4deb607af53522fefd) )
	ROM_LOAD( "a1004.u10",   0x2000000, 0x0400000, CRC(9fd3f5fd) SHA1(057531f91062be51589c6cf8f4170089b9be6380) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b1000.u9",    0x0000000, 0x0800000,  CRC(c5961f6f) SHA1(a68060b10edbd084cbde79d2ed1c9084777beb10) )
	ROM_LOAD( "b1001.u11",   0x0800000, 0x0800000,  CRC(0b7e1c06) SHA1(545e15e0087f8621d593fecd8b4013f7ca311686) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "m1000.u5",    0x400000, 0x800000, CRC(ed407ae8) SHA1(a6e9c09b39c13e8fb7fbc89fa9f823cbeb66e901) )
	ROM_LOAD( "m1001.u7",    0xc00000, 0x400000, CRC(662d2d48) SHA1(2fcc3099d9c04456cae3b13035fb28eaf709e7d8) )
ROM_END

/*

Demon Front
IGS, 2002

Cart for IGS PGM system. This game is a straight rip-off of Metal Slug.

PCB Layout
----------

IGS PCB-0387-02-FV
|-----------------------------------------------|
| BS616LV1010                      IGS027A      |
| BS616LV1010                                   |
|                                               |
|                              *     BS616LV1010|
|            PAL  PAL                           |
|                                               |
| V102_16M.U5        V101_32M.U26               |
|                                        PAL    |
|                             T04501.U29   22MHz|
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      BS616LV1010  - 64K x16 SRAM (TSOP44)
      IGS027A      - Custom IGS IC, ARM7 based CPU (QFP120)
      T04501.U29   - 23C6410 64MBit MaskROM (SOP44)
      V102_16M.U5  - 27C160 16MBit EPROM (DIP42)
      V101_32M.U26 - 27C322 32MBit EPROM (DIP42)
      *            - Unpopulated position for 29F1610 16MBit SOP44 FlashROM, linked to IGS027A


IGS PCB-0390-00-FV-A
|-----------------------------------------------|
| A04501.U3  A04502.U4  A04503.U6   U8*     U10*|
|                                               |
|                                               |
|                                               |
|                                               |
|     W04501.U5   U7*    B04501.U9   B04502.U11 |
|                                               |
|                                               |
|                                               |
|-|                                           |-|
  |--------------------||---------------------|
Notes:
      *  - Unpopulated SOP44 ROM position.

*/

ROM_START( dmnfrnt )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v102_16m.u5",    0x100000, 0x200000, CRC(3d4d481a) SHA1(95953b8f31343389405cc722b4177ff5adf67b62) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
	ROM_LOAD( "dmnfrnt_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION( 0x800000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data, internal missing) */
	ROM_LOAD( "v101_32m.u26", 0x000000, 0x400000,  CRC(93965281) SHA1(89da198aaa7ca759cb96b5f18859a477e55fd590) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t04501.u29",    0x400000, 0x800000, CRC(900eaaac) SHA1(4033cb7b28fcadb92d5af3ea7fdd1c22747618fd) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04501.u3",    0x0000000, 0x0800000, CRC(9741bea6) SHA1(e3e904249be228628c8c2bd3495cda23586dc048) )
	ROM_LOAD( "a04502.u4",    0x0800000, 0x0800000, CRC(e104f405) SHA1(124b3deed3e838f8bae6c7d78bdd788859597585) )
	ROM_LOAD( "a04503.u6",    0x1000000, 0x0800000, CRC(bfd5cfe3) SHA1(fbe4c0a2987c2036df707b86597d78124ee2e665) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04501.u9",    0x0000000, 0x0800000,  CRC(29320b7d) SHA1(59c78805e666f912df201c34616744f46057937b) )
	ROM_LOAD( "b04502.u11",   0x0800000, 0x0200000,  CRC(578c00e9) SHA1(14235cc8b0f8c7dd659512f017a2d4aacd91d89d) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "w04501.u5",    0x400000, 0x800000, CRC(3ab58137) SHA1(b221f7e551ff0bfa3fd97b6ebedbac69442a66e9) )
ROM_END

ROM_START( dmnfrnta )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v105_16m.u5",    0x100000, 0x200000, CRC(bda083bd) SHA1(58d6438737a2c43aa8bbcb7f34fb51375b781b1c) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
	ROM_LOAD( "dmnfrnt_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION( 0x800000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data, internal missing) */
	ROM_LOAD( "v105_32m.u26", 0x000000, 0x400000,  CRC(d200ee63) SHA1(3128c27c5f5a4361d31e7b4bb006de631b3a228c) )
        ROM_LOAD( "chinese-v105.u62", 0x000000, 0x400000,  CRC(c798c2ef) SHA1(91e364c33b935293fa765ca521cdb67ac45ec70f) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
	ROM_LOAD( "t04501.u29",    0x400000, 0x800000, CRC(900eaaac) SHA1(4033cb7b28fcadb92d5af3ea7fdd1c22747618fd) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04501.u3",    0x0000000, 0x0800000, CRC(9741bea6) SHA1(e3e904249be228628c8c2bd3495cda23586dc048) )
	ROM_LOAD( "a04502.u4",    0x0800000, 0x0800000, CRC(e104f405) SHA1(124b3deed3e838f8bae6c7d78bdd788859597585) )
	ROM_LOAD( "a04503.u6",    0x1000000, 0x0800000, CRC(bfd5cfe3) SHA1(fbe4c0a2987c2036df707b86597d78124ee2e665) )

	ROM_REGION( 0xc00000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04501.u9",    0x0000000, 0x0800000,  CRC(29320b7d) SHA1(59c78805e666f912df201c34616744f46057937b) )
	ROM_LOAD( "b04502.u11",   0x0800000, 0x0200000,  CRC(578c00e9) SHA1(14235cc8b0f8c7dd659512f017a2d4aacd91d89d) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "w04501.u5",    0x400000, 0x800000, CRC(3ab58137) SHA1(b221f7e551ff0bfa3fd97b6ebedbac69442a66e9) )
ROM_END

ROM_START( theglad )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "pgm_p01s.rom", 0x000000, 0x020000, CRC(e42b166e) SHA1(2a9df9ec746b14b74fae48b1a438da14973702ea) )  // (BIOS)
	ROM_LOAD16_WORD_SWAP( "v101.u6",      0x100000, 0x080000, CRC(f799e866) SHA1(dccc3c903357c40c3cf85ac0ae8fc12fb0f853a6) )

	/* CPU2 = Z80, romless, code uploaded by 68k */

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* ARM protection ASIC - internal rom */
	ROM_LOAD( "theglad_igs027a_execute_only_area", 0x0000, 0x00188, NO_DUMP )
        ROM_LOAD( "theglad_igs027a_v100_overseas.bin", 0x0188, 0x3e78, CRC(02fe6f52) SHA1(0b0ddf4507856cfc5b7d4ef7e4c5375254c2a024) )

	ROM_REGION( 0x800000, REGION_USER1, 0 ) /* Protection Data (encrypted external ARM data, internal missing) */
	ROM_LOAD( "v107.u26", 0x000000, 0x200000,  CRC(f7c61357) SHA1(52d31c464dfc83c5371b078cb6b73c0d0e0d57e3) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // (BIOS)
        ROM_LOAD( "t04601.u33",   0x400000, 0x800000, CRC(e5dab371) SHA1(2e3c93958eb0326b6b84b95c2168626f26bbac76) )

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1800000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04601.u2",    0x0000000, 0x0800000,  CRC(d9b2e004) SHA1(8e1882b800fe9f12d7d49303e7417ba5b6f8ef85) )
	ROM_LOAD( "a04602.u4",    0x0800000, 0x0800000,  CRC(14f22308) SHA1(7fad54704e8c97eab723f53dfb50fb3e7bb606d2) )
	ROM_LOAD( "a04603.u6",    0x1000000, 0x0800000,  CRC(8f621e17) SHA1(b0f87f378e0115d0c95017ca0f1b0d508827a7c6) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04601.u11",    0x0000000, 0x0800000, CRC(ee72bccf) SHA1(73c25fe659f6c903447066e4ef83d2f580449d76) )
	ROM_LOAD( "b04602.u12",    0x0800000, 0x0400000, CRC(7dba9c38) SHA1(a03d509274e8f6a500a7ebe2da5aab8bed4e7f2f) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // (BIOS)
	ROM_LOAD( "w04601.u1",    0x400000, 0x800000, CRC(5f15ddb3) SHA1(c38dcef8e06802a84e42a7fc9fa505475fc3ac65) )
ROM_END


/*** GAME ********************************************************************/

GAME( 1997, pgm,      0,          pgm, pgm,      0,          ROT0,   "IGS", "PGM (Polygame Master) System BIOS", NOT_A_DRIVER )

GAME( 1997, orlegend, pgm,        pgm, pgm,      orlegend,   ROT0,   "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 126)", GAME_IMPERFECT_SOUND  )
GAME( 1997, orlegnde, orlegend,   pgm, pgm,      orlegend,   ROT0,   "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112)", GAME_IMPERFECT_SOUND  )
GAME( 1997, orlegndc, orlegend,   pgm, pgm,      orlegend,   ROT0,   "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 112, Chinese Board)", GAME_IMPERFECT_SOUND  )
GAME( 1997, orld111c, orlegend,   pgm, pgm,      orlegend,   ROT0,   "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 111, Chinese Board)", GAME_IMPERFECT_SOUND  )
GAME( 1997, orld105k, orlegend,   pgm, orld105k, orlegend,   ROT0,   "IGS", "Oriental Legend / Xi Yo Gi Shi Re Zuang (ver. 105, Korean Board)", GAME_IMPERFECT_SOUND  )
GAME( 1997, drgw2c,   drgw2,      drgw2, pgm,      drgw2c,     ROT0,   "IGS", "Zhong Guo Long II (ver. 100C, China)", GAME_IMPERFECT_SOUND )
GAME( 1998, olds,     pgm,        olds, olds,    olds,       ROT0,   "IGS", "Oriental Legend Special / Xi You Shi E Zhuan Super (Korea 101)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION )
GAME( 1998, olds100,  olds,       olds, olds,    olds,       ROT0,   "IGS", "Oriental Legend Special / Xi You Shi E Zhuan Super (100)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION )
GAME( 1998, olds100a, olds,       olds, olds,    olds,       ROT0,   "IGS", "Oriental Legend Special / Xi You Shi E Zhuan Super (100 alt)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION )
GAME( 1998, olds103t, olds,       pgm, pgm,      olds103t,   ROT0,   "IGS", "Oriental Legend Special / Xi You Shi E Zhuan Super (ver. 103, China, Tencent) (unprotected)", GAME_IMPERFECT_SOUND )  // without overseas region
GAME( 1999, kov,      pgm,        pgm, sango,    kov,        ROT0,   "IGS", "Knights of Valour / Sangoku Senki (ver. 117)", GAME_IMPERFECT_SOUND ) /* ver # provided by protection? */
GAME( 1999, kov115,   kov,        pgm, sango,    kov,        ROT0,   "IGS", "Knights of Valour / Sangoku Senki (ver. 115)", GAME_IMPERFECT_SOUND ) /* ver # provided by protection? */
GAME( 1999, kovj,     kov,        pgm, sango,    kov,        ROT0,   "IGS", "Knights of Valour / Sangoku Senki (ver. 100, Japanese Board)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) /* ver # provided by protection? */
GAME( 1999, photoy2k, pgm,        pgm, photoy2k,    djlzz,      ROT0,   "IGS", "Photo Y2K", GAME_IMPERFECT_SOUND ) /* region provided by protection device */
GAME( 1999, raf102j,  photoy2k,   pgm, photoy2k,    djlzz,      ROT0,   "IGS", "Real and Fake / Photo Y2K (ver. 102, Japan Board)", GAME_IMPERFECT_SOUND ) /* region provided by protection device */
GAME( 2000, kov2,     pgm,        kov2, sango,    kov2,       ROT0,   "IGS", "Knights of Valour 2", GAME_IMPERFECT_SOUND )
GAME( 2000, kov2106,  kov2,       kov2, sango,    kov2,       ROT0,   "IGS", "Knights of Valour 2 (106)", GAME_IMPERFECT_SOUND )
GAME( 2000, kov2p,    kov2,       kov2, sango,    kov2p,       ROT0,   "IGS", "Knights of Valour 2 Plus - Nine Dragons", GAME_IMPERFECT_SOUND )
GAME( 2001, martmast, pgm,        kov2, sango,    martmast,   ROT0,   "IGS", "Martial Masters", GAME_IMPERFECT_SOUND )

/* Playable but maybe imperfect protection emulation */
GAME( 1997, drgw2,    pgm,        drgw2, pgm,      drgw2,      ROT0,   "IGS", "Dragon World II (ver. 110X, Export)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1997, drgw2j,   drgw2,      drgw2, pgm,      drgw2j,     ROT0,   "IGS", "Chuugokuryuu II (ver. 100J, Japan)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1999, kovplus,  kov,        pgm, sango,    kov,        ROT0,   "IGS", "Knights of Valour Plus / Sangoku Senki Plus (ver. 119)", GAME_IMPERFECT_SOUND )
GAME( 1999, kovplusa, kov,        pgm, sango,    kov,        ROT0,   "IGS", "Knights of Valour Plus / Sangoku Senki Plus (alt ver. 119)", GAME_IMPERFECT_SOUND )
GAME( 1998, killbld,  pgm,        killbld, killbld, killbld, ROT0,   "IGS", "The Killing Blade (ver. 109, Chinese Board)", GAME_IMPERFECT_SOUND )
GAME( 1998, killbld104, killbld,  killbld, killbld, killbld, ROT0,   "IGS", "The Killing Blade (ver. 104)", GAME_IMPERFECT_SOUND )
GAME( 1999, puzlstar, pgm,        pgm, sango,    pstar,      ROT0,   "IGS", "Puzzle Star", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION )

/* not working */
GAME( 1998, drgw3,    pgm,        pgm, sango,    dw3,        ROT0,   "IGS", "Dragon World 3", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1998, drgw3k,   drgw3,      pgm, sango,    dw3,        ROT0,   "IGS", "Dragon World 3 (Korean Board)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1999, kovsh,    kov,        pgm, sango,    kovsh,      ROT0,   "IGS", "Knights of Valour Superheroes / Sangoku Senki Superheroes (ver. 322)", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 2001, ddp2,     pgm,       kov2, ddp2,     ddp2,       ROT270, "IGS", "DoDonPachi II - Bee Storm ", GAME_IMPERFECT_SOUND )
GAME( 2001, puzzli2,  pgm,        pgm, sango,    puzzli2,    ROT0,   "IGS", "Puzzli 2 Super", GAME_IMPERFECT_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 2001, theglad,  pgm,        theglad, theglad, theglad, ROT0,   "IGS", "The Gladiator / Road of the Sword / Shen Jian (V101)", GAME_IMPERFECT_SOUND )
GAME( 2002, dmnfrnt,  pgm,        svg, sango,    dmnfrnt,        ROT0,   "IGS", "Demon Front (V102)", GAME_IMPERFECT_SOUND )
GAME( 2002, dmnfrnta, dmnfrnt,    svg, sango,    dmnfrnt,        ROT0,   "IGS", "Demon Front (V105)", GAME_IMPERFECT_SOUND )

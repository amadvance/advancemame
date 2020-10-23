
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
#define PGMARM7LOGERROR 1
#define PGMARM7SPEEDHACK 1

#include "driver.h"
#include "sound/ics2115.h"
#include "cpu/arm7/arm7core.h"
#include <time.h>
#include "timer.h"


extern UINT16 *pgm_mainram, *pgm_bg_videoram, *pgm_tx_videoram, *pgm_videoregs, *pgm_rowscrollram;
static UINT8 *z80_mainram;
static UINT32 *arm7_shareram;
static UINT32 arm7_latch;
WRITE16_HANDLER( pgm_tx_videoram_w );
WRITE16_HANDLER( pgm_bg_videoram_w );
VIDEO_START( pgm );
VIDEO_EOF( pgm );
VIDEO_UPDATE( pgm );
void pgm_mm_decrypt(void);
void pgm_ket_decrypt(void);
void pgm_espgal_decrypt(void);
void pgm_py2k2_decrypt(void);

READ16_HANDLER( cavepgm_calendar_r );
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
	if (PGMARM7LOGERROR) logerror("ARM7: Latch read: %08x (%08x) (%06x)\n", arm7_latch, mem_mask, activecpu_get_pc() );
	return arm7_latch;
}


#ifdef PGMARM7SPEEDHACK
static void arm_irq(int param)
{
	cpunum_set_input_line(2, ARM7_FIRQ_LINE, PULSE_LINE);
}
#endif

//static mame_timer *   arm_comms_timer;
static WRITE32_HANDLER( arm7_latch_arm_w )
{
	if (PGMARM7LOGERROR) logerror("ARM7: Latch write: %08x (%08x) (%06x)\n", data, mem_mask, activecpu_get_pc() );
	COMBINE_DATA(&arm7_latch);

#ifdef PGMARM7SPEEDHACK
//  cpu_boost_interleave(0, TIME_IN_USEC(100));
	if (data!=0xaa) cpu_spinuntil_trigger(1000);
	cpu_trigger(1002);
#else
	cpu_boost_interleave(0, TIME_IN_USEC(100));
	cpu_spinuntil_time(TIME_IN_CYCLES(100, 0));
#endif
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
	if (PGMARM7LOGERROR) logerror("M68K: Latch read: %04x (%04x) (%06x)\n", arm7_latch & 0x0000ffff, mem_mask, activecpu_get_pc() );
	return arm7_latch;
}

static WRITE16_HANDLER( arm7_latch_68k_w )
{
	if (PGMARM7LOGERROR) logerror("M68K: Latch write: %04x (%04x) (%06x)\n", data & 0x0000ffff, mem_mask, activecpu_get_pc() );
	COMBINE_DATA(&arm7_latch);

#ifdef PGMARM7SPEEDHACK
	cpu_trigger(1000);
	timer_set(TIME_IN_USEC(50), 0, arm_irq); // i don't know how long..
	cpu_spinuntil_trigger(1002);
#else
	cpunum_set_input_line(2, ARM7_FIRQ_LINE, PULSE_LINE);
	cpu_boost_interleave(0, TIME_IN_USEC(200));
	cpu_spinuntil_time(TIME_IN_CYCLES(200, 2)); // give the arm time to respond (just boosting the interleave doesn't help
#endif
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

static struct ics2115_interface pgm_ics2115_interface = {
	REGION_SOUND1,
	sound_irq
};

/* Calendar Emulation */

static unsigned char CalVal, CalMask, CalCom=0, CalCnt=0;

static unsigned char bcd(unsigned char data)
{
	return ((data / 10) << 4) | (data % 10);
}

READ16_HANDLER( cavepgm_calendar_r )
{
	unsigned char calr;
	calr = (CalVal & CalMask) ? 1 : 0;
	CalMask <<= 1;
	return calr;
}

WRITE16_HANDLER( cavepgm_calendar_w )
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

static READ16_HANDLER( nvram_r )
{

	return generic_nvram16[offset] | 0xfff0;
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
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(cavepgm_calendar_r, cavepgm_calendar_w)
	AM_RANGE(0xc00008, 0xc00009) AM_WRITE(z80_reset_w)
	AM_RANGE(0xc0000a, 0xc0000b) AM_WRITE(z80_ctrl_w)
	AM_RANGE(0xc0000c, 0xc0000d) AM_READWRITE(soundlatch3_word_r, soundlatch3_word_w)

	AM_RANGE(0xc08000, 0xc08001) AM_READ(input_port_0_word_r) // p1+p2 controls
	AM_RANGE(0xc08002, 0xc08003) AM_READ(input_port_1_word_r) // p3+p4 controls
	AM_RANGE(0xc08004, 0xc08005) AM_READ(input_port_2_word_r) // extra controls
	AM_RANGE(0xc08006, 0xc08007) AM_READ(input_port_3_word_r) // dipswitches

	AM_RANGE(0xc10000, 0xc1ffff) AM_READWRITE(z80_ram_r, z80_ram_w) /* Z80 Program */
ADDRESS_MAP_END

static ADDRESS_MAP_START( cavepgm_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x3fffff) AM_ROM

	AM_RANGE(0x700006, 0x700007) AM_WRITENOP // Watchdog?

//  AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram) AM_SHARE("sram") /* Main Ram */
	//AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_BASE(&pgm_mainram) AM_SHARE("sram") /* Main Ram */
		AM_RANGE(0x800000, 0x81ffff) AM_RAM AM_MIRROR(0x0e0000) AM_BASE(&pgm_mainram)//AM_BASE(&generic_nvram16) Main Ram 
		AM_RANGE(0x800000, 0x81ffff) AM_READWRITE(nvram_r, MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	/* 0x880000 - 0x89ffff seems to be protection related, maybe they just happen to access protection results
       via a mirror tho. */

	//AM_RANGE(0x900000, 0x907fff) AM_MIRROR(0x0f8000) AM_READWRITE(pgm_videoram_r, pgm_videoram_w) AM_BASE_MEMBER(pgm_state, videoram) /* IGS023 VIDEO CHIP */
		AM_RANGE(0x900000, 0x903fff) AM_READWRITE(MRA16_RAM, pgm_bg_videoram_w) AM_BASE(&pgm_bg_videoram) // Backgrounds 
	AM_RANGE(0x904000, 0x905fff) AM_READWRITE(MRA16_RAM, pgm_tx_videoram_w) AM_BASE(&pgm_tx_videoram) // Text Layer 
	AM_RANGE(0x907000, 0x9077ff) AM_RAM AM_BASE(&pgm_rowscrollram)
	//AM_RANGE(0xa00000, 0xa011ff) AM_RAM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE_GENERIC(paletteram)
	AM_RANGE(0xa00000, 0xa011ff) AM_READWRITE(MRA16_RAM, paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xb00000, 0xb0ffff) AM_RAM AM_BASE(&pgm_videoregs) // Video Regs inc. Zoom Table

	AM_RANGE(0xc00002, 0xc00003) AM_READWRITE(soundlatch_word_r, m68k_l1_w)
	AM_RANGE(0xc00004, 0xc00005) AM_READWRITE(soundlatch2_word_r, soundlatch2_word_w)
	AM_RANGE(0xc00006, 0xc00007) AM_READWRITE(cavepgm_calendar_r, cavepgm_calendar_w)
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
	AM_RANGE(0x18000000, 0x1800ffff) AM_RAM
	AM_RANGE(0x38000000, 0x38000003) AM_READWRITE(arm7_latch_arm_r, arm7_latch_arm_w) /* 68k Latch */
	AM_RANGE(0x48000000, 0x4800ffff) AM_READWRITE(arm7_shareram_r, arm7_shareram_w) AM_BASE(&arm7_shareram)
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
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN1 )
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
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
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

/*** GFX Decodes *************************************************************/

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


/* we can't decode the sprite data like this because it isn't tile based.  the
   data decoded by pgm32_charlayout was rearranged at start-up */

/*** Machine Driver **********************************************************/

static MACHINE_DRIVER_START( pgm )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 20000000) /* 20 mhz! verified on real board */
	MDRV_CPU_PROGRAM_MAP(pgm_mem,0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

//	MDRV_CPU_ADD_TAG("sound", Z80, 8468000)
	MDRV_CPU_ADD_TAG("sound", Z80, 8467200)

	MDRV_CPU_PROGRAM_MAP(z80_mem, 0)
	MDRV_CPU_IO_MAP(z80_io, 0)
	/* audio CPU */

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(ICS2115, 0)
	MDRV_SOUND_CONFIG(pgm_ics2115_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 3.0)

	MDRV_FRAMES_PER_SECOND(59.170000)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 56*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1200/2)
	//MDRV_MACHINE_RESET ( pgm )

	MDRV_VIDEO_START(pgm)
	MDRV_VIDEO_EOF(pgm)
	MDRV_VIDEO_UPDATE(pgm)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cavepgm )
	MDRV_IMPORT_FROM(pgm)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(cavepgm_mem,0)
	MDRV_NVRAM_HANDLER(generic_0fill)
	//MDRV_NVRAM_HANDLER(cavepgm)

	/* protection CPU */
//  MDRV_CPU_ADD_TAG("prot", ARM7, 20000000)    // ???
//  MDRV_CPU_PROGRAM_MAP(arm7_map, 0)
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

extern UINT8 *pgm_sprite_a_region;
extern size_t	pgm_sprite_a_region_allocate;

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

ROM_START( ket )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	/* doesn't use a separate BIOS rom */
	ROM_LOAD16_WORD_SWAP( "ketsui_v100.u38", 0x000000, 0x200000, CRC(dfe62f3b) SHA1(baa58d1ce47a707f84f65779ac0689894793e9d9) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "ket_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM text bios - surface scratched to remove details
	//ROM_LOAD( "text-1.u19",   0x400000, 0x800000, CRC(2665b041) SHA1(fb1107778b66f2af0de77ac82e1ee2902f53a959) )
	ROM_LOAD( "t04701w064.u19",   0x400000, 0x800000, CRC(2665b041) SHA1(fb1107778b66f2af0de77ac82e1ee2902f53a959) ) //text-1

	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */
	
	ROM_REGION( 0x1000000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04701w064.u7",    0x0000000, 0x0800000, CRC(5ef1b94b) SHA1(f10dfa46e0a4d297c3a856aea5b49d648f98935c) ) //image-1
	ROM_LOAD( "a04702w064.u8",    0x0800000, 0x0800000, CRC(26d6da7f) SHA1(f20e07a7994f41b5ed917f8b0119dc5542f3541c) ) //image-2

	ROM_REGION( 0x0800000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04701w064.u1",    0x0000000, 0x0800000, CRC(1bec008d) SHA1(07d117dc2eebb35727fb18a7c563acbaf25a8d36) ) //bitmap-1

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 ) /* Samples - (8 bit mono 11025Hz) - */
	/* there is a position for the PGM audio bios rom, but it's unpopulated, and the M of PGM has been scratched off the PCB */
	ROM_LOAD( "m04701b032.u17",    0x400000, 0x400000, CRC(b46e22d1) SHA1(670853dc485942fb96380568494bdf3235f446ee) ) //music-1
ROM_END

ROM_START( keta )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	/* doesn't use a separate BIOS rom */
	ROM_LOAD16_WORD_SWAP( "ketsui_prg_revised.bin", 0x000000, 0x200000, CRC(69fcf5eb) SHA1(f726e251b4daa2f8d717e32000d4d7abc71c710d) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "ket_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM text bios - surface scratched to remove details
	ROM_LOAD( "t04701w064.u19",   0x400000, 0x800000, CRC(2665b041) SHA1(fb1107778b66f2af0de77ac82e1ee2902f53a959) ) //text-1
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1000000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04701w064.u7",    0x0000000, 0x0800000, CRC(5ef1b94b) SHA1(f10dfa46e0a4d297c3a856aea5b49d648f98935c) ) //image-1
	ROM_LOAD( "a04702w064.u8",    0x0800000, 0x0800000, CRC(26d6da7f) SHA1(f20e07a7994f41b5ed917f8b0119dc5542f3541c) ) //image-2

	ROM_REGION( 0x0800000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04701w064.u1",    0x0000000, 0x0800000, CRC(1bec008d) SHA1(07d117dc2eebb35727fb18a7c563acbaf25a8d36) ) //bitmap-1

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 ) /* Samples - (8 bit mono 11025Hz) - */
	/* there is a position for the PGM audio bios rom, but it's unpopulated, and the M of PGM has been scratched off the PCB */
	ROM_LOAD( "music-1.u17",    0x400000, 0x400000, CRC(b46e22d1) SHA1(670853dc485942fb96380568494bdf3235f446ee) )
ROM_END

ROM_START( ketb )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	/* doesn't use a separate BIOS rom */
	ROM_LOAD16_WORD_SWAP( "ketsui_prg_original.bin", 0x000000, 0x200000, CRC(cca5e153) SHA1(b653feaa2004c379312def6b1613c3497f654ddf) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "ket_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM text bios - surface scratched to remove details
	ROM_LOAD( "t04701w064.u19",   0x400000, 0x800000, CRC(2665b041) SHA1(fb1107778b66f2af0de77ac82e1ee2902f53a959) ) //text-1
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1000000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04701w064.u7",    0x0000000, 0x0800000, CRC(5ef1b94b) SHA1(f10dfa46e0a4d297c3a856aea5b49d648f98935c) ) //image-1
	ROM_LOAD( "a04702w064.u8",    0x0800000, 0x0800000, CRC(26d6da7f) SHA1(f20e07a7994f41b5ed917f8b0119dc5542f3541c) ) //image-2

	ROM_REGION( 0x0800000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04701w064.u1",    0x0000000, 0x0800000, CRC(1bec008d) SHA1(07d117dc2eebb35727fb18a7c563acbaf25a8d36) ) //bitmap-1

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 ) /* Samples - (8 bit mono 11025Hz) - */
	/* there is a position for the PGM audio bios rom, but it's unpopulated, and the M of PGM has been scratched off the PCB */
	ROM_LOAD( "music-1.u17",    0x400000, 0x400000, CRC(b46e22d1) SHA1(670853dc485942fb96380568494bdf3235f446ee) )
ROM_END

ROM_START( espgal )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	/* doesn't use a separate BIOS rom */
	ROM_LOAD16_WORD_SWAP( "espgaluda_v100.u38", 0x000000, 0x200000, CRC(08ecec34) SHA1(bce2e7fb9105ed51603d09cbd3a9eeb5b8f47ee2) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "espgal_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM text bios - surface scratched to remove details
	//ROM_LOAD( "text-1.u19",   0x400000, 0x800000, NO_DUMP )
	ROM_LOAD( "t04801w064.u19",   0x400000, 0x800000, CRC(6021c79e) SHA1(fbc340dafb18aa3094de29b881318a5a9794e4bc) ) //text-1
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1000000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04801w064.u7",    0x0000000, 0x0800000, CRC(26dd4932) SHA1(9bbabb5a53cb5ba88397cc2c258980f3b70314ce) ) //image-1
	ROM_LOAD( "a04802w064.u8",    0x0800000, 0x0800000, CRC(0e6bf7a9) SHA1(a7541e2b5a0df2bc62a5b347e54dbc2ed1922db2) ) //image-2

	ROM_REGION( 0x0800000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04801w064.u1",    0x0000000, 0x0800000, CRC(98dce13a) SHA1(61d48b7117459f7babc022b68231f6928177a71d) ) //bitmap-1

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 ) /* Samples - (8 bit mono 11025Hz) - */
	/* there is a position for the PGM audio bios rom, but it's unpopulated, and the M of PGM has been scratched off the PCB */
	ROM_LOAD( "w04801b032.u17",    0x400000, 0x400000, CRC(60298536) SHA1(6b7333f16cce778c5725dbdf75a5446f0906397a) ) //music-1
ROM_END

ROM_START( ddp3 )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "ddp3_bios.u37",    0x00000, 0x080000, CRC(b3cc5c8f) SHA1(02d9511cf71e4a0d6ca8fd9a1ef2c79b0d001824) ) // uses a standard PGM bios with the startup logos hacked out
	//ROM_LOAD16_WORD_SWAP( "ddp3_d_d_1_0.u36",  0x100000, 0x200000, CRC(5d3f85ba) SHA1(4c24ea206140863d456179750366921442e1d2b8) )
	ROM_LOAD16_WORD_SWAP( "ddp3_v101.u36",  0x100000, 0x200000, CRC(195b5c1e) SHA1(f18d791c034b0a3d85888a92fb5d326ee3deb04f) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "ddp3_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM bios
	//ROM_LOAD( "t04401w064.u19",0x400000, 0x800000, CRC(3a95f19c) SHA1(fd3c47cf0b8b1e20c6bec4be68a089fc8bbf4dbe) )
	ROM_LOAD( "t04401w064.u19",0x400000, 0x800000, CRC(3a95f19c) SHA1(fd3c47cf0b8b1e20c6bec4be68a089fc8bbf4dbe) ) //text-1
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04401w064.u7",  0x0000000, 0x0800000, CRC(ed229794) SHA1(1cf1863495a18c7c7d277a9be43ec116b00960b0) )
	ROM_LOAD( "a04402w064.u8",  0x0800000, 0x0800000, CRC(752167b0) SHA1(c33c3398dd8e479c9d5bd348924958a6aecbf0fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04401w064.u1",  0x0000000, 0x0800000, CRC(8cbff066) SHA1(eef1cd566bc70ebf45f047e56026803d5c1dac43) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // same as standard PGM bios
	ROM_LOAD( "m04401b032.u17",  0x400000, 0x400000, CRC(5a0dbd76) SHA1(06ab202f6bd5ebfb35b9d8cc7a8fb83ec8840659) )
ROM_END

ROM_START( ddp3a )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "ddp3_bios.u37",    0x00000, 0x080000, CRC(b3cc5c8f) SHA1(02d9511cf71e4a0d6ca8fd9a1ef2c79b0d001824) ) // uses a standard PGM bios with the startup logos hacked out
	ROM_LOAD16_WORD_SWAP( "ddp3_d_d_1_0.u36",  0x100000, 0x200000, CRC(5d3f85ba) SHA1(4c24ea206140863d456179750366921442e1d2b8) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "ddp3_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM bios
	//ROM_LOAD( "t04401w064.u19",0x400000, 0x800000, CRC(3a95f19c) SHA1(fd3c47cf0b8b1e20c6bec4be68a089fc8bbf4dbe) )
	ROM_LOAD( "t04401w064.u19",0x400000, 0x800000, CRC(3a95f19c) SHA1(fd3c47cf0b8b1e20c6bec4be68a089fc8bbf4dbe) ) //text-1
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04401w064.u7",  0x0000000, 0x0800000, CRC(ed229794) SHA1(1cf1863495a18c7c7d277a9be43ec116b00960b0) )
	ROM_LOAD( "a04402w064.u8",  0x0800000, 0x0800000, CRC(752167b0) SHA1(c33c3398dd8e479c9d5bd348924958a6aecbf0fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04401w064.u1",  0x0000000, 0x0800000, CRC(8cbff066) SHA1(eef1cd566bc70ebf45f047e56026803d5c1dac43) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // same as standard PGM bios
	ROM_LOAD( "m04401b032.u17",  0x400000, 0x400000, CRC(5a0dbd76) SHA1(06ab202f6bd5ebfb35b9d8cc7a8fb83ec8840659) )
ROM_END

ROM_START( ddp3b )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "ddp3_bios.u37",    0x00000, 0x080000, CRC(b3cc5c8f) SHA1(02d9511cf71e4a0d6ca8fd9a1ef2c79b0d001824) ) // uses a standard PGM bios with the startup logos hacked out
	ROM_LOAD16_WORD_SWAP( "dd v100.bin",  0x100000, 0x200000, CRC(7da0c1e4) SHA1(aca2fe35ba0ab3628900fa2aba2d22fc4fd7046d) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "ddp3_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM bios
	//ROM_LOAD( "t04401w064.u19",0x400000, 0x800000, CRC(3a95f19c) SHA1(fd3c47cf0b8b1e20c6bec4be68a089fc8bbf4dbe) )
	ROM_LOAD( "t04401w064.u19",0x400000, 0x800000, CRC(3a95f19c) SHA1(fd3c47cf0b8b1e20c6bec4be68a089fc8bbf4dbe) ) //text-1
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04401w064.u7",  0x0000000, 0x0800000, CRC(ed229794) SHA1(1cf1863495a18c7c7d277a9be43ec116b00960b0) )
	ROM_LOAD( "a04402w064.u8",  0x0800000, 0x0800000, CRC(752167b0) SHA1(c33c3398dd8e479c9d5bd348924958a6aecbf0fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04401w064.u1",  0x0000000, 0x0800000, CRC(8cbff066) SHA1(eef1cd566bc70ebf45f047e56026803d5c1dac43) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // same as standard PGM bios
	ROM_LOAD( "m04401b032.u17",  0x400000, 0x400000, CRC(5a0dbd76) SHA1(06ab202f6bd5ebfb35b9d8cc7a8fb83ec8840659) )
ROM_END

/* this expects Magic values in NVRAM to boot */
ROM_START( ddp3blk )
	ROM_REGION( 0x600000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "ddp3_bios.u37",    0x00000, 0x080000, CRC(b3cc5c8f) SHA1(02d9511cf71e4a0d6ca8fd9a1ef2c79b0d001824) ) // uses a standard PGM bios with the startup logos hacked out
	ROM_LOAD16_WORD_SWAP( "ddb10.u45",  0x100000, 0x200000, CRC(72b35510) SHA1(9a432e5e1ebe61aafd737b6acc905653e5af0d38) )

	//ROM_REGION( 0x4000, "prot", 0 ) /* ARM protection ASIC - internal rom */
	//ROM_LOAD( "ddp3_igs027a.bin", 0x000000, 0x04000, NO_DUMP )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_ERASE00 )
	/* no external protection rom */

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) /* 8x8 Text Tiles + 32x32 BG Tiles */
	ROM_LOAD( "pgm_t01s.rom", 0x000000, 0x200000, CRC(1a7123a0) SHA1(cc567f577bfbf45427b54d6695b11b74f2578af3) ) // same as standard PGM bios
	ROM_LOAD( "t04401w064.u19",0x400000, 0x800000, CRC(3a95f19c) SHA1(fd3c47cf0b8b1e20c6bec4be68a089fc8bbf4dbe) ) //text-1
	
	ROM_REGION( 0xc00000/5*8, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF ) /* Region for 32x32 BG Tiles */
	/* 32x32 Tile Data is put here for easier Decoding */

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 ) /* Sprite Colour Data */
	ROM_LOAD( "a04401w064.u7",  0x0000000, 0x0800000, CRC(ed229794) SHA1(1cf1863495a18c7c7d277a9be43ec116b00960b0) )
	ROM_LOAD( "a04402w064.u8",  0x0800000, 0x0800000, CRC(752167b0) SHA1(c33c3398dd8e479c9d5bd348924958a6aecbf0fc) )

	ROM_REGION( 0x1000000, REGION_GFX4, 0 ) /* Sprite Masks + Colour Indexes */
	ROM_LOAD( "b04401w064.u1",  0x0000000, 0x0800000, CRC(8cbff066) SHA1(eef1cd566bc70ebf45f047e56026803d5c1dac43) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* Samples - (8 bit mono 11025Hz) - */
	ROM_LOAD( "pgm_m01s.rom", 0x000000, 0x200000, CRC(45ae7159) SHA1(d3ed3ff3464557fd0df6b069b2e431528b0ebfa8) ) // same as standard PGM bios
	ROM_LOAD( "m04401b032.u17",  0x400000, 0x400000, CRC(5a0dbd76) SHA1(06ab202f6bd5ebfb35b9d8cc7a8fb83ec8840659) )

	//ROM_REGION( 0x20000, REGION_NVRAM, 0 ) /* NVRAM with factory programmed values - needed to boot */
	//ROM_LOAD( "ddp3blk_defaults.nv",  0x0000000, 0x020000, CRC(a1651904) SHA1(5b80d3c4c764895c40953a66161d4dd84f742604) )
ROM_END



static UINT16 value0, value1, valuekey, ddp3lastcommand;
static UINT32 valueresponse;
int ddp3internal_slot = 0;
UINT32 ddp3slots[0xff];
INT16 thrust;

static WRITE16_HANDLER( ddp3_asic_w )
{

	if (offset == 0)
	{
		value0 = data;
		return;
	}
	else if (offset == 1)
	{
		UINT16 realkey;
		if ((data >> 8) == 0xff)
			valuekey = 0xff00;
		realkey = valuekey >> 8;
		realkey |= valuekey;
		{
			valuekey += 0x0100;
			valuekey &= 0xff00;
			if (valuekey == 0xff00)
				valuekey =  0x0100;
		}
		data ^= realkey;
		value1 = data;
		value0 ^= realkey;

		ddp3lastcommand = value1 & 0xff;

		/* typical frame (ddp3) (all 3 games use only these commands? for the most part of levels espgal just issues 8e)
            vbl
            145f28 command 67
            145f70 command e5
            145f28 command 67
            145f70 command e5
            1460c6 command 40
            145ec0 command 8e
            */

		switch (ddp3lastcommand)
		{
			default:
				//printf("%06x command %02x | %04x\n", cpu_get_pc(space->cpu), ddp3lastcommand, value0);
				valueresponse = 0x880000;
				break;

			case 0x40:
				//printf("%06x command %02x | %04x\n", cpu_get_pc(space->cpu), ddp3lastcommand, value0);
				valueresponse = 0x880000;
				//if (value0 == 0x420) ddp3slots[ddp3internal_slot] -= (thrust);
				//else printf("ignoring 0x40 - %04x %04x %04x\n", ddp3internal_slot, value0, ddp3slots[ddp3internal_slot]);
				ddp3slots[(value0>>10)&0x1F] = (ddp3slots[(value0>>5)&0x1F]+ddp3slots[(value0>>0)&0x1F])&0xffffff;
				
				break;

			case 0x67:
				//printf("%06x command %02x | %04x\n", cpu_get_pc(space->cpu), ddp3lastcommand, value0);
				valueresponse = 0x880000;
				ddp3internal_slot = (value0 & 0xff00)>>8;
				ddp3slots[ddp3internal_slot] = (value0 & 0x00ff) << 16;
				break;

			case 0xe5:
				//printf("%06x command %02x | %04x\n", cpu_get_pc(space->cpu), ddp3lastcommand, value0);
				valueresponse = 0x880000;
				ddp3slots[ddp3internal_slot] |= (value0 & 0xffff);
				if (value0 > 0xf000) thrust = -value0;
				break;

			case 0x8e:
				//valueresponse = 0x880000;
				//valueresponse = 0x880000 | ((UINT32)pgm_mainram[0x0c840/2] << 2) | pgm_mainram[0x0c842/2];
				//printf("%06x command %02x | %08x ** [value respond]\n", cpu_get_pc(space->cpu), ddp3lastcommand, valueresponse);
				valueresponse = ddp3slots[value0 & 0xff];
				break;

			case 0x99: // reset?
			valuekey = 0x100;
				valueresponse = 0x880000;
				break;

		}
	}
	else if (offset==2)
	{

	}

}

static READ16_HANDLER( ddp3_asic_r )
{

	if (offset == 0)
	{
		UINT16 d = valueresponse & 0xffff;
		UINT16 realkey = valuekey >> 8;
		realkey |= valuekey;
		d ^= realkey;

		//printf("ASIC read: 0x%x\n", d);

		return d;

	}
	else if (offset == 1)
	{
		UINT16 d = valueresponse >> 16;
		UINT16 realkey = valuekey >> 8;
		realkey |= valuekey;
		d ^= realkey;

		//printf("ASIC read: 0x%x\n", d);

		return d;

	}
	return 0xffff;
}

static READ16_HANDLER( ddp3_ram_mirror_r )
{
	// HACK!
	// this should be a mirror of main ram, at least according to the standard PGM map.
	// returning 0x0000 for all values read from here allows the games to run for a bit longer tho
	//printf("%06x ddp3_ram_mirror_r would return %04x returning 0x0000 instead\n", cpu_get_pc(space->cpu), pgm_mainram[offset]);
	return 0x0000;
	//return pgm_mainram[offset];
}

void install_asic27a_ddp3(void)
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500005, 0, 0, ddp3_asic_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x500000, 0x500005, 0, 0, ddp3_asic_w);
	
	//memory_install_readwrite16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x500000, 0x500005, 0, 0, ddp3_asic_r, ddp3_asic_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x880000, 0x89ffff, 0, 0, ddp3_ram_mirror_r);
}

void install_asic27a_ket(void)
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400005, 0, 0, ddp3_asic_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400005, 0, 0, ddp3_asic_w);
	
	//memory_install_readwrite16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x400000, 0x400005, 0, 0, ddp3_asic_r, ddp3_asic_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x880000, 0x89ffff, 0, 0, ddp3_ram_mirror_r);
}

void install_asic27a_espgal(void)
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400005, 0, 0, ddp3_asic_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400005, 0, 0, ddp3_asic_w);
	
	//memory_install_readwrite16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x400000, 0x400005, 0, 0, ddp3_asic_r, ddp3_asic_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x880000, 0x89ffff, 0, 0, ddp3_ram_mirror_r);
}

static void pgm_basic_init_nobank(void)
{
	expand_32x32x5bpp();
	expand_colourdata();
}

static DRIVER_INIT( ddp3 )
{
	pgm_basic_init_nobank();
	//pgm_basic_init();
	
	pgm_py2k2_decrypt(); // yes, it's the same as photo y2k2
	install_asic27a_ddp3();
}

static DRIVER_INIT( ddp3blk )
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	pgm_basic_init_nobank();
	//pgm_basic_init();
	
	pgm_py2k2_decrypt(); // yes, it's the same as photo y2k2
	install_asic27a_ddp3();
	
	rom[0x13C33A/2] = 0x4e71;
	rom[0x13C33C/2] = 0x4e71;
	rom[0x13C348/2] = 0x4e71;
	rom[0x13C34A/2] = 0x4e71;
}

static DRIVER_INIT( ket )
{
	pgm_basic_init_nobank();
	//pgm_basic_init();
	
	pgm_ket_decrypt();
	install_asic27a_ket();
}

static DRIVER_INIT( espgal )
{
	pgm_basic_init_nobank();
	//pgm_basic_init();
	pgm_espgal_decrypt();
	install_asic27a_espgal();
}


/*** GAME ********************************************************************/

//GAME( 1997, pgm,      0,          pgm, pgm,      0,          ROT0,   "IGS", "PGM (Polygame Master) System BIOS", NOT_A_DRIVER )

GAME( 2002, ddp3,         0,         cavepgm,    pgm,     ddp3,       ROT270, "Cave", "DoDonPachi Dai-Ou-Jou", GAME_IMPERFECT_SOUND)
GAME( 2002, ddp3a,        ddp3,      cavepgm,    pgm,     ddp3,       ROT270, "Cave", "DoDonPachi Dai-Ou-Jou (V100, second revision)", GAME_IMPERFECT_SOUND) // Displays "2002.04.05.Master Ver"
GAME( 2002, ddp3b,        ddp3,      cavepgm,    pgm,     ddp3,       ROT270, "Cave", "DoDonPachi Dai-Ou-Jou (V100, first revision)", GAME_IMPERFECT_SOUND) // Displays "2002.04.05 Master Ver"
GAME( 2002, ddp3blk,      ddp3,      cavepgm,    pgm,     ddp3blk,       ROT270, "Cave", "DoDonPachi Dai-Ou-Jou (Black Label)", GAME_IMPERFECT_SOUND)
// the exact text of the 'version' shows which revision of the game it is; the newest has 2 '.' symbols in the string, the oldest, none.
GAME( 2002, ket,          0,         cavepgm,   pgm,     ket,       ROT270, "Cave", "Ketsui", GAME_IMPERFECT_SOUND) // Displays 2003/01/01. Master Ver.
GAME( 2002, keta,         ket,       cavepgm,    pgm,     ket,       ROT270, "Cave", "Ketsui (older)", GAME_IMPERFECT_SOUND) // Displays 2003/01/01 Master Ver.
GAME( 2002, ketb,         ket,       cavepgm,   pgm,     ket,       ROT270, "Cave", "Ketsui (first revision)", GAME_IMPERFECT_SOUND) // Displays 2003/01/01 Master Ver
GAME( 2002, espgal,       0,         cavepgm,    pgm,     espgal,       ROT270, "Cave", "EspGaluda", GAME_IMPERFECT_SOUND)

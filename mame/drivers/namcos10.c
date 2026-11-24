// license:BSD-3-Clause
// copyright-holders:smf, windyfairy
/***************************************************************************

  Namco System 10 - Arcade PSX Hardware
  =====================================
  Driver by smf

Namco System 10 Hardware Overview
---------------------------------
Note! This document is a Work-In-Progress and will be updated from time to time when more dumps are available.

This document covers all the known Namco System 10 games, including....
*Drum Master                                     (C) Namco, 2001
*Drum Master 2                                   (C) Namco, 2001
*Drum Master 3                                   (C) Namco, 2002
*Drum Master 4                                   (C) Namco, 2003
*Drum Master 5                                   (C) Namco, 2003
*Drum Master 6                                   (C) Namco, 2004
*Drum Master 7                                   (C) Namco, 2005
*GAHAHA Ippatsu-dou                              (C) Namco/Metro, 2000
*GAHAHA Ippatsu-dou 2                            (C) Namco/Metro, 2001
Gamshara (10021 Ver.A)                           (C) Mitchell, 2003
Gekitoride-Jong Space (10011 Ver.A)              (C) Namco/Metro, 2001
*Golgo-13 3 : Juusei no Chinkonka                (C) Namco/8ing/Raizing, 2001
*Hard Puncher Hajime no Ippo 2 Ouja e no Chousen (C) Namco/Taito, 2002
*Honne Hakkenki                                  (C) Namco, 2001
Kotoba no Puzzle Mojipittan (KPM1 Ver.A)         (C) Namco, 2001
*Kurukuru Food                                   (C) Namco, 2002
Mr Driller 2 (DR21 Ver.A)                        (C) Namco, 2000
Mr Driller 2 (DR22 Ver.A)                        (C) Namco, 2000
Mr Driller G (DRG1 Ver.A)                        (C) Namco, 2001
*NFL Classic Football                            (C) Namco, 2003
*Panicuru Panekuru (PPA1 Ver.A)                  (C) Namco, 2001
*Photo Battole                                   (C) Namco, 2001
*Ren-ai Quiz High School Angel                   (C) Namco, 2002
*Seishun Quiz Colorful High School               (C) Namco, 2002
Star Trigon (STT1 Ver.A)                         (C) Namco, 2002
*Taiko No Tatsujin                               (C) Namco, 2001
*Taiko No Tatsujin 2                             (C) Namco, 2001
*Taiko No Tatsujin 3                             (C) Namco, 2002
*Taiko No Tatsujin 4                             (C) Namco, 2003
*Taiko No Tatsujin 5                             (C) Namco, 2003
*Taiko No Tatsujin 6                             (C) Namco, 2004
*Tsukkomi Yousei Gips Nice Tsukkomi              (C) Namco/Metro, 2002
*Uchuu Daisakusen : Chocovader Contactee (CVC1 Ver.A) (C) Namco, 2002

* - denotes not dumped yet. If you can help with the remaining undumped S10 games,
    please contact me at http://www.mameworld.net/gurudumps/

The Namco System 10 system comprises 2 PCB's....
MAIN PCB - This is the mother board PCB. It holds the main CPU/GPU & SPU and all sound circuitry, program & video RAM,
           controller/input logic and video output circuitry. Basically everything except the ROMs.
           There are three known revisions of this PCB so far. The differences seem very minor. The 2nd and 3rd revision
           have an updated CPLD revision.
           The 3rd revision has an updated model Sony chip. The only other _noticable_ difference is some component
           shuffling in the sound amplification section to accomodate two extra 1000uF capacitors and one 470uF capacitor
           has been replaced by a 1000uF capacitor. Everything else, including all the PLDs appears to be identical.
           Note there are no ROMs on the Main PCB and also no custom Namco chips on System10, which seem to have been
           phased out. Instead, they have been replaced by (custom programmed) CPLDs, probably due to cost-cutting
           measures within the company, or to reduce the cost of System10 to an entry-level no-frills development platform.
MEM PCB  - There are two known revisions of this PCB (so far). They're mostly identical except for the type/number of ROMs
           used and the contents of the CPLD.  The 2nd revision also has a RAM chip on it.
           Each game has a multi-letter code assigned to it which is printed on a small sticker and placed on the top side
           of the MEM PCB.
           This code is then proceeded by a number (only '1' & '2' seen so far), then 'Ver.' then A/B/C/D/E (only 'A' seen so far)
           which denotes the software revision, and in some cases a sub-revision such as 1 or 2 (usually only listed in the
           test mode).
           The first 1 denotes a Japanese version. Other numbers denote a World version.
           For World versions, only the main program changes, the rest of the (graphics) ROMs use the Japanese version ROMs.
           Any System 10 MEM PCB can be swapped to run on any System 10 Main PCB regardless of the main board revision.
           The high scores are stored on the MEM PCB (probably inside the main program EEPROMs/FlashROMs or maybe the CPLD?).
           Also, on all System 10 games, there is a sticker with a serial number on it and the program ROMs also contain
           that same serial number. I'm not sure why, they're not exactly _easily_ tracable and no one cares either way ;-)
           See the Main PCB and ROM Daughterboard PCB texts below for more details.


Main PCB Layout
---------------

Revision 1
SYSTEM10 MAIN PCB 8906960103 (8906970103)

Revision 2
SYSTEM10 MAIN PCB 8906960104 (8906970104)

Revision 3
SYSTEM10 MAIN PCB 8906962400 (8906972400)
  |----------------------------------------------------------|
  |   LA4705    VR1                     J201                 |
  |                           |----------------------|       |
  |           NJM3414         |----------------------|       |
|-|                                                          |
|       BA3121                                               |
|             NJM3414            54V25632      54V25632    J1|
|                     CXD1178Q                               |
|J JP4                          |---------|   |-------|      |
|A                              |         |   |       |      |
|M         CXA2067AS            |CXD8561CQ|   |CY37128|      |
|M                 53.693175MHz |         |   |VP160  |      |
|A                              |         |   |       |      |
|                               |---------|   |-------|      |
|                                                            |
|                            101.4912MHz                     |
|-|          MAX734  IS41LV16100                             |
  |                              |---------|                 |
  |   DSW1           IS41LV16100 |         |                 |
  |                              |CXD8606BQ|                 |
  |        GAL16V8D              |         |                 |
  |J5      |-|           *       |         |          PST592 |
  |        | |                   |---------| |--------|      |
  |        | |                               |        |      |
  |        | |           *                   |CXD2938Q|      |
  |J4      | |J202             IS41LV16256   |        |      |
  |        | |                               |        |      |
  |        | |      EPM3064                  |--------|      |
  |        | |                                               |
  |        |-|                 PQ30RV21                      |
  |                                                  J103    |
  |----------------------------------------------------------|
Notes:
------
      CXD8606BQ   : SONY CXD8606BQ Central Processing Unit / GTE     (QFP208)
                     - replaced by CXD8606CQ on Revision 3 Main PCB
      CXD8561CQ   : SONY CXD8561CQ Graphics Processsor Unit          (QFP208)
      CXD2938Q    : SONY CXD2938Q  Sound Processor Unit              (QFP208)
      CXD1178Q    : SONY CXD1178Q  8-bit RGB 3-channel D/A converter (QFP48)
      CXA2067AS   : SONY CXA2067AS TV/Video circuit RGB Pre-Driver   (SDIP30)
      CY37128VP160: CYPRESS CY37128VP160 Complex Programmable Logic Device (TQFP160, stamped 'S10MA1')
                     - replaced by an updated revision on Revision 2 & 3 Main PCB and stamped 'S10MA1B'
      EPM3064     : Altera MAX EPM3064ATC100-10 Complex Programmable Logic Device (TQFP100, stamped 'S10MA2A')
      GAL16V8D    : GAL16V8D PAL (PLCC20, stamped 'S10MA3A')
      IS41LV16100 : ISSI IS41LV16100S-50T 1M x16 EDO DRAM (x2, TSOP50(44) Type II)
      IS41LV16256 : ISSI IS41LV16256-50T 256k x16 EDO DRAM (TSOP44(40) Type II)
      54V25632    : OKI 54V25632 256K x32 SGRAM (x2, QFP100)
      PQ30RV31    : Sharp PQ30RV31 5 Volt to 3.3 Volt Voltage Regulator
      LA4705      : LA4705 15W 2-channel Power Amplifier (SIP18)
      MAX734      : MAX734 +12V 120mA Flash Memory Programming Supply Switching Regulator (SOIC8)
      PST592      : PST592J System Reset IC with 2.7V detection circuit (SOIC4)
      BA3121      : Ground Isolation Amplifier/ Noise Eliminator (SOIC8)
      NJM3414     : 70mA Dual Op Amp (x2, SOIC8)
      DSW1        : 8 position dip switch
      JP4         : 2 position jumper, set to NC, alt. position labelled SYNC (Note: changing the jumper position has
                    no visual effect)
      J1          : 40 Pin IDC connector for plugging of a flat 40-wire cable (not used, purpose unknown, possible
                    CDROM/DVD)
      J4          : 10 pin header for extra controls etc  / (note: custom Namco 48 pin edge connector is not on
                    System10 PCBs)
      J5          : 4 pin header for stereo sound out     \
      J201        : 100 pin custom Namco connector for mounting of MEM PCB. This connector is surface-mounted, not a
                    thru-hole type.
      J202        : 80 pin custom Namco connector for mounting of another board. This connector is surface-mounted,
                    not a thru-hole type.
                    (not used, purpose unknown)
      J103        : 6-pin JAMMA2 power plug (Note none of the other JAMMA2 standard connectors are present)
      VR1         : Volume potentiometer
      *           : Unpopulated position for IS41LV16100 1M x16 EDO DRAM

Additional Notes:
                1. In test mode (Display Test) the screen can be set to interlace or non-interlace mode. The graphics in
                   interlace mode are visually much smoother with noticeable screen flickering. Non-interlace modes gives
                   a much blockier graphic display (i.e. lower resolution) but without screen flickering.
                2. There is no dedicated highscore/options EEPROM present on the PCB, the game stores it's settings on the
                   game board (probably in the program EEPROMs/FlashROMs or maybe the CPLD?).

ROM Daughterboard PCB
---------------------
This PCB holds all the ROMs.
There are two known types of ROM daughterboards used on S10 games (so far).
All of the PCBs are the same size (approx 5" x 5") containing one custom connector surface-mounted to the underside of
the PCB, some MASKROMs/FlashROMs, a CPLD (which seems to be the customary 'KEYCUS' chip, and on the 2nd type a RAM
chip also.

********
*Type 1*
********
System10 MEM(M) PCB 8906961000 (8906970700)
|-------------------------------------|
|                                     |
|                       |-------|     +-
|                       |       |     +-
|                       |CY37128|     +-
|                       |VP160  |     +-
|                       |       |     +-
|     7E     7D         |-------|     +-
|                                     |
|     6E     6D                       |
|                                     |
|     5E     5D                 5A    |
|                                     |
|     4E     4D                 4A    |
|                                     |
|     3E     3D                       |
|                                     |
|     2E     2D                 2A    |
|                                     |
|     1E     1D                 1A    |
|                                     |
|-------------------------------------|
Notes:
      CY37128VP160: CY37128VP160 Cypress Complex Programmable Logic Device (TQFP160)
      1A - 5A     : Intel Flash DA28F640J5 64MBit Flash EEPROM (SSOP56)
      1D - 7E     : Samsung Electronics K3N9V1000A-YC 128MBit MASK ROM (TSOP48) (see note 3)
      6 pin header: (purpose unknown, probably for programming the CPLD)

This PCB is used on:

              Software     MEM PCB
Game          Revision     Sticker      KEYCUS   ROMs Populated
------------------------------------------------------------------------------------
Mr Driller 2  DR21/VER.A3  DR21 Ver.A   KC001A   DR21VERA.1A, DR21MA1.1D, DR21MA2.2D
Mr Driller 2  DR22/VER.A3  DR22 Ver.A   KC001A   DR22VERA.1A, DR21MA1.1D, DR21MA2.2D

      Note
      1. The ROM PCB has locations for 4x 64MBit program ROMs, but only 1A is populated.
      2. The ROM PCB has locations for 14x 128MBit GFX ROMs (Total capacity = 2048MBits) but only 1D and 2D are populated.
      3. These ROMs are only 18mm long, dumping them requires a special custom adapter

********
*Type 2*
********
System10 MEM(N) PCB 8906961402 (8906971402)
|-------------------------------------|
|                                     |
|                    |---------|      +-
|                    |         |      +-
|                    |CY37256  |      +-
|     8E     8D      |VP208    |      +-
|                    |         |      +-
|     7E     7D      |---------|      +-
|                                     |
|     6E     6D                       |
|                                     |
|     5E     5D       CY7C1019        |
|                                     |
|     4E     4D                       |
|                                     |
|     3E     3D                       |
|                                     |
|     2E     2D                       |
|                                     |
|     1E     1D                       |
|                                     |
|-------------------------------------|
Notes:
      CY37256VP208: Cypress CY37256VP208 Complex Programmable Logic Device (TQFP208)
      CY7C1019    : Cypress CY7C1019BV33-15VC or Samsung Electronics K6R1008V1C-JC15 128k x8 bit 3.3V High Speed CMOS Static Ram (SOJ32)
      1D - 8E     : Samsung Electronics K9F2808U0B-YCBO 128MBit NAND Flash EEPROM (TSOP48)
      6 pin header: (purpose unknown, probably for programming the CPLD)

This PCB is used on:

                                      MEM PCB
Game                                  Sticker       KEYCUS   ROMs Populated
---------------------------------------------------------------------------
Gamshara                              10021 Ver.A   KC020A   8E, 8D
Gekitoride-Jong Space                 10011 Ver.A   KC003A   8E, 8D, 7E, 7D
Mr.Driller G                          DRG1  Ver.A   KC007A   8E, 8D, 7E
Kotoba no Puzzle Mojipittan           KPM1  Ver.A   KC012A   8E, 8D, 7E
Panicuru Panekuru                     PPA1  Ver.A   KC017A   8E, 8D, 7E
Star Trigon                           STT1  Ver.A   KC019A   8E, 8D
Utyuu Daisakusen Chocovader Contactee CVC1  Ver.A   KC022A

      Note
      1. The ROM PCB has locations for 16x 128MBit FlashROMs (Total capacity = 2048MBits) but usually only a few are populated.
*/

#include "driver.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"
#include "sound/psx.h"
#include "sound/custom.h"
#include "streams.h"
#include "minimp3.h"

struct ns10_crypto
{
	UINT64 cipher[16], plain[16];
	UINT16 constant;
	UINT16 (*nonlinear)(UINT64, UINT64);
	const UINT16 *iv;
	UINT8 bits[16];
};
static const struct ns10_crypto *ns10_crypto;
static UINT64 ns10_cipher, ns10_plain;
static UINT16 ns10_mask;
static int ns10_active;

static int gf2_reduce(UINT64 value)
{
	value ^= value >> 32;
	value ^= value >> 16;
	value ^= value >> 8;
	value ^= value >> 4;
	return (0x6996 >> (value & 15)) & 1;
}

static UINT16 g13jnr_nonlinear(UINT64 previous_cipherwords, UINT64 previous_plainwords)
{
	UINT64 previous_masks = previous_cipherwords ^ previous_plainwords;
	return (1 & (previous_masks >> 6) & (previous_masks >> 10)) << 14;
}

static const struct ns10_crypto g13jnr_crypto = {
	{
		0x00005600001c0582ULL, 0x0000000000004024ULL, 0x0000010212403000ULL, 0x0000000000008404ULL,
		0x0000005121060021ULL, 0x0000010212402001ULL, 0x0000000001024000ULL, 0x0000000000000840ULL,
		0x00000981c2100148ULL, 0x0000000020108400ULL, 0x0000000008110134ULL, 0x0000000000000003ULL,
		0x00004c9801080102ULL, 0x0000000040860083ULL, 0x0000000000000001ULL, 0x0000000000000288ULL,
	},
	{
		0x0000441800340584ULL, 0x0000000000004028ULL, 0x0000010212800000ULL, 0x0000000000000404ULL,
		0x0000485022000041ULL, 0x0000010212804001ULL, 0x0000000001028000ULL, 0x0000000000000841ULL,
		0x00000e0104080150ULL, 0x0000000023208400ULL, 0x0000000040110104ULL, 0x0000000000000003ULL,
		0x0000470001100102ULL, 0x00000000408c0083ULL, 0x0000000000000002ULL, 0x0000000000000308ULL,
	},
	0x9546,
	g13jnr_nonlinear,
	NULL,
	{0xe, 0xd, 0xc, 0xf, 0x9, 0xb, 0x8, 0xa, 0x6, 0x7, 0x4, 0x5, 0x1, 0x3, 0x0, 0x2}};

static UINT16 ns10_unscramble(UINT16 data)
{
	UINT16 result = 0;
	int i;
	data ^= 0xaaaa;
	for (i = 0; i < 16; i++)
		result |= ((data >> ns10_crypto->bits[i]) & 1) << (15 - i);
	return result;
}

/* The CPLD feeds the last four cipher and plain words into its next mask. */
static UINT16 ns10_decrypt(UINT16 data)
{
	UINT16 result = data ^ ns10_mask;
	int i;
	ns10_cipher = (ns10_cipher << 16) | data;
	ns10_plain = (ns10_plain << 16) | result;
	ns10_mask = ns10_crypto->constant ^ ns10_crypto->nonlinear(ns10_cipher, ns10_plain);
	for (i = 0; i < 16; i++)
		ns10_mask ^= (gf2_reduce(ns10_cipher & ns10_crypto->cipher[i]) ^ gf2_reduce(ns10_plain & ns10_crypto->plain[i]))
					 << i;
	return result;
}

/* Each NAND page contains 512 data bytes and 16 spare bytes. */
#define NS10_NAND_SIZE 0x1080000
#define NS10_PAGE_SIZE 0x210
static UINT32 ns10_address, ns10_column;
static UINT16 ns10_bank, ns10_control, ns10_interrupt;
static UINT8 ns10_command, ns10_area;
static UINT8 ns10_program[NS10_PAGE_SIZE];

static UINT8 *ns10_nand(void)
{
	UINT32 start;
	if (ns10_bank >= memory_region_length(REGION_USER2) / NS10_NAND_SIZE)
		return NULL;
	start = ns10_bank * NS10_NAND_SIZE;
	return memory_region(REGION_USER2) + start;
}

static UINT8 ns10_byte_r(void)
{
	UINT8 *rom = ns10_nand();
	UINT32 address;
	if (!rom)
		return 0xff;
	if (ns10_command == 0x70)
		return 0xc0;
	if (ns10_command == 0x90)
	{
		static const UINT8 id[2] = { 0xec, 0x73 };
		return ns10_column < 2 ? id[ns10_column++] : 0;
	}
	if (ns10_column >= NS10_PAGE_SIZE)
		return 0xff;
	address = (ns10_address >> 8) * NS10_PAGE_SIZE + ns10_column++;
	return address < NS10_NAND_SIZE ? rom[address] : 0xff;
}

static UINT16 ns10_data_r(void)
{
	UINT32 page = ns10_address >> 8;
	UINT16 data = ns10_byte_r() << 8;
	data |= ns10_byte_r();
	if (ns10_command == 0x70 || ns10_command == 0x90)
		return data;
	if (ns10_active)
		return ns10_decrypt(data);
	if (!ns10_bank && page < 0x40)
		return data;
	return ns10_unscramble(data);
}

static void ns10_dma_read(UINT32 address, INT32 size)
{
	while (size-- > 0)
	{
		UINT32 data = ns10_data_r();
		data |= (UINT32)ns10_data_r() << 16;
		g_p_n_psxram[(address & (g_n_psxramsize - 1)) / 4] = data;
		address += 4;
	}
}

/* the MCU of the ROM board publishes the 10 bit light gun A/D readings as 16 bit values */
static UINT32 ns10_gun_adc(int axis)
{
	UINT32 value = readinputport(2 + axis);
	/* the Y reading grows going up, the opposite of the screen coordinate used by the port */
	if (axis == 0)
		value = 0x3ff - value;
	return value * 0xffff / 0x3ff;
}

/*
  MEM(P3) MP3 playback, high level emulation of the TMP95C061 firmware.

  The game uploads the firmware and whole MP3 files to the 8 MB SDRAM of the
  ROM board through a 128 KB window, selected by 0x1f30000c. It then writes a
  command in the mailbox at the start of bank 0x3f and raises INT5 writing
  0x1f300006. The firmware queues the command and feeds the Sanyo LC82310
  decoder from SDRAM. The game never waits for the playback, so the commands
  run immediately and minimp3 decodes the MP3 only when the stream needs
  samples, without emulating the MCU and the serial link to the decoder.

  Commands (letter in the low byte, also lowercase, two word parameters):
  'L' a     the track table is at mailbox + a, 8 bytes per track numbered
            from 1: SDRAM start and length
  'C' p     clear playlist p
  'A' p, t  append track t to playlist p, track 0 restarts the playlist
  'P' p     play playlist p from its first track
  'S'       stop
  'R'       resume
  'V' a, s  fade the LC82310 attenuation to a in s seconds
  'G'       write attenuation, track and state at mailbox + 2, 4 and 6
*/
#define NS10_SDRAM_SIZE 0x800000
#define NS10_MAILBOX 0x7e0000
#define NS10_PLAYLISTS 0x40
/* firmware states, reported by 'G' */
#define NS10_MP3_STOP 0
#define NS10_MP3_PLAY 1
#define NS10_MP3_START 2

static UINT8 *ns10_sdram;
static UINT16 ns10_sdram_bank;
static UINT32 ns10_track_table;
/* count, unused, then the tracks terminated by 0xff */
static UINT8 ns10_playlist[NS10_PLAYLISTS][0x102];
static int ns10_playlist_index, ns10_playlist_pos;
static int ns10_mp3_state;
static UINT32 ns10_mp3_pos, ns10_mp3_end;
static mp3dec_t ns10_mp3_decoder;
static mp3d_sample_t ns10_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
static int ns10_pcm_count, ns10_pcm_pos, ns10_pcm_stereo;
static int ns10_volume, ns10_volume_start, ns10_volume_target;
static int ns10_fade_steps, ns10_fade_acc, ns10_fade_samples;
static int ns10_mp3_rate;
static sound_stream *ns10_mp3_stream;

/* LC82310 gain for each attenuation step in 1/4096 units, from MAME */
static const UINT16 ns10_mp3_gain[77] =
{
	4096, 3964, 3832, 3700, 3567, 3435, 3303, 3171, 3039, 2907, 2775, 2643,
	2510, 2378, 2246, 2114, 1982, 1850, 1718, 1586, 1453, 1321, 1189, 1123,
	1057, 925, 793, 727, 661, 595, 529, 462, 396, 363, 330, 297,
	264, 242, 220, 198, 176, 154, 132, 128, 124, 120, 117, 113,
	109, 105, 101, 97, 93, 89, 85, 82, 78, 74, 70, 66,
	62, 58, 54, 51, 47, 43, 39, 35, 31, 27, 23, 19,
	16, 12, 8, 4, 0
};

/* the MCU sees the SDRAM little endian */
static UINT32 ns10_sdram_r32(UINT32 address)
{
	UINT32 data = 0;
	int i;
	for (i = 0; i < 4; i++)
		data |= ns10_sdram[(address + i) & (NS10_SDRAM_SIZE - 1)] << (i * 8);
	return data;
}

static void ns10_sdram_w16(UINT32 address, UINT16 data)
{
	ns10_sdram[address] = data;
	ns10_sdram[address + 1] = data >> 8;
}

static void ns10_mp3_reset(void)
{
	int i;
	memset(ns10_playlist, 0xff, sizeof(ns10_playlist));
	for (i = 0; i < NS10_PLAYLISTS; i++)
		ns10_playlist[i][0] = 0;
	ns10_track_table = NS10_MAILBOX + 0x10;
	ns10_playlist_index = ns10_playlist_pos = 0;
	ns10_mp3_state = NS10_MP3_STOP;
	ns10_mp3_pos = ns10_mp3_end = 0;
	ns10_pcm_count = ns10_pcm_pos = 0;
	ns10_volume = ns10_volume_start = ns10_volume_target = 0;
	ns10_fade_steps = ns10_fade_acc = 0;
}

static void ns10_mp3_track_start(void)
{
	int track = ns10_playlist[ns10_playlist_index][2 + ns10_playlist_pos];
	UINT32 entry = ns10_track_table + (track - 1) * 8;
	UINT32 start = ns10_sdram_r32(entry);
	UINT32 length = ns10_sdram_r32(entry + 4);
	/* the table comes from the game, keep the track inside the SDRAM */
	if (start > NS10_SDRAM_SIZE)
		start = NS10_SDRAM_SIZE;
	if (length > NS10_SDRAM_SIZE - start)
		length = NS10_SDRAM_SIZE - start;
	/* the firmware drops the ID3v1 tag at the end of the file */
	if (length >= 128 && !memcmp(ns10_sdram + start + length - 128, "TAG", 3))
		length -= 128;
	logerror("ns10_mp3_track_start %d %06x %06x frame %d\n", track, start, length, cpu_getcurrentframe());	ns10_mp3_pos = start;
	ns10_mp3_end = start + length;
	mp3dec_init(&ns10_mp3_decoder);
	ns10_mp3_state = NS10_MP3_PLAY;
}

static void ns10_mp3_track_next(void)
{
	UINT8 *playlist = ns10_playlist[ns10_playlist_index];
	if (playlist[2 + ++ns10_playlist_pos] == 0)
		ns10_playlist_pos = 0;
	if (ns10_playlist_pos >= playlist[0] || playlist[2 + ns10_playlist_pos] == 0xff)
	{
		ns10_mp3_state = NS10_MP3_STOP;
		ns10_playlist_pos = 0;
	}
	else
		ns10_mp3_state = NS10_MP3_START;
}

/* decode the next frame, return 0 when the playback stops */
static int ns10_mp3_decode(void)
{
	int starts = 0;
	while (ns10_mp3_state != NS10_MP3_STOP)
	{
		mp3dec_frame_info_t info;
		int samples;
		if (ns10_mp3_state == NS10_MP3_START)
		{
			/* a looping playlist of tracks without audio would never end */
			if (++starts > 0x100)
			{
				ns10_mp3_state = NS10_MP3_STOP;
				break;
			}
			ns10_mp3_track_start();
		}
		samples = mp3dec_decode_frame(&ns10_mp3_decoder, ns10_sdram + ns10_mp3_pos, ns10_mp3_end - ns10_mp3_pos, ns10_pcm, &info);
		ns10_mp3_pos += info.frame_bytes;
		if (samples)
		{
			ns10_pcm_count = samples;
			ns10_pcm_pos = 0;
			ns10_pcm_stereo = info.channels == 2;			if (info.hz != ns10_mp3_rate)
			{
				ns10_mp3_rate = info.hz;
				stream_set_sample_rate(ns10_mp3_stream, ns10_mp3_rate);
			}
			return 1;
		}
		/* no frame left, frames without samples only skip data */
		if (!info.frame_bytes)
			ns10_mp3_track_next();
	}
	return 0;
}

/* the firmware spreads the attenuation change over the steps like Bresenham */
static void ns10_mp3_fade_step(void)
{
	if (ns10_volume == ns10_volume_target)
		return;
	ns10_fade_acc += ns10_volume_target - ns10_volume_start;
	while (ns10_fade_acc > ns10_fade_steps && ns10_volume < ns10_volume_target)
	{
		++ns10_volume;
		ns10_fade_acc -= ns10_fade_steps;
	}
	while (-ns10_fade_acc > ns10_fade_steps && ns10_volume > ns10_volume_target)
	{
		--ns10_volume;
		ns10_fade_acc += ns10_fade_steps;
	}
}

static void ns10_mp3_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	stream_sample_t *left = outputs[0];
	stream_sample_t *right = outputs[1];
	int gain;

	/* the firmware moves the attenuation one step every 25 ms */
	ns10_fade_samples += samples;
	while (ns10_fade_samples >= ns10_mp3_rate / 40)
	{
		ns10_fade_samples -= ns10_mp3_rate / 40;
		ns10_mp3_fade_step();
	}
	gain = ns10_mp3_gain[ns10_volume < 76 ? ns10_volume : 76];

	while (samples > 0)
	{
		int i, count;
		if (ns10_pcm_pos == ns10_pcm_count && !ns10_mp3_decode())
		{
			memset(left, 0, samples * sizeof(*left));
			memset(right, 0, samples * sizeof(*right));
			return;
		}
		count = ns10_pcm_count - ns10_pcm_pos;
		if (count > samples)
			count = samples;
		if (ns10_pcm_stereo)
		{
			const mp3d_sample_t *pcm = ns10_pcm + ns10_pcm_pos * 2;
			for (i = 0; i < count; i++)
			{
				left[i] = (pcm[i * 2] * gain) >> 12;
				right[i] = (pcm[i * 2 + 1] * gain) >> 12;
			}
		}
		else
		{
			const mp3d_sample_t *pcm = ns10_pcm + ns10_pcm_pos;
			for (i = 0; i < count; i++)
				left[i] = right[i] = (pcm[i] * gain) >> 12;
		}
		ns10_pcm_pos += count;
		left += count;
		right += count;
		samples -= count;
	}
}

static void ns10_mp3_command(void)
{
	UINT8 *mailbox = ns10_sdram + NS10_MAILBOX;
	int command = mailbox[0];
	int p1 = mailbox[2] | (mailbox[3] << 8);
	int p2 = mailbox[4] | (mailbox[5] << 8);
	UINT8 *playlist = ns10_playlist[p1 % NS10_PLAYLISTS];

	logerror("ns10_mp3_command %c %04x %04x frame %d\n", command, p1, p2, cpu_getcurrentframe());

	/* generate the samples preceding the command */
	stream_update(ns10_mp3_stream, 0);

	switch (command | 0x20)
	{
	case 'l':
		ns10_track_table = NS10_MAILBOX + (p1 | (p2 << 16));
		break;
	case 'c':
		if (p1 < NS10_PLAYLISTS)
		{
			playlist[0] = 0;
			memset(playlist + 2, 0xff, 0x100);
		}
		break;
	case 'a':
		if (p1 < NS10_PLAYLISTS && p2 <= 0xff && playlist[0] < 0xff)
			playlist[2 + playlist[0]++] = p2;
		break;
	case 'p':
		if (p1 < NS10_PLAYLISTS)
		{
			ns10_playlist_index = p1;
			ns10_playlist_pos = 0;
			ns10_mp3_state = NS10_MP3_START;
			ns10_pcm_pos = ns10_pcm_count;
		}
		break;
	case 's':
		ns10_mp3_state = NS10_MP3_STOP;
		ns10_pcm_pos = ns10_pcm_count;
		break;
	case 'r':
		ns10_mp3_state = NS10_MP3_PLAY;
		break;
	case 'v':
		if (p1 <= 0xff && p2 <= 0xff)
		{
			ns10_volume_start = ns10_volume;
			ns10_volume_target = p1;
			ns10_fade_steps = p2 * 40;
			ns10_fade_acc = 0;
			if (!ns10_fade_steps)
				ns10_volume = ns10_volume_target;
		}
		break;
	case 'g':
		ns10_sdram_w16(NS10_MAILBOX + 2, ns10_volume);
		ns10_sdram_w16(NS10_MAILBOX + 4, ns10_playlist[ns10_playlist_index][2 + ns10_playlist_pos]);
		ns10_sdram_w16(NS10_MAILBOX + 6, ns10_mp3_state);
		break;
	}
}

static void *ns10_mp3_sh_start(int clock, const struct CustomSound_interface *config)
{
	ns10_mp3_rate = 44100;
	ns10_mp3_stream = stream_create(0, 2, ns10_mp3_rate, 0, ns10_mp3_update);
	return ns10_mp3_stream;
}

static struct CustomSound_interface ns10_mp3_interface =
{
	ns10_mp3_sh_start, 0, 0
};

static READ32_HANDLER(ns10_register_r)
{
	switch (offset * 4)
	{
	case 0x100000:
		return 0;
	case 0x150000:
		return ns10_data_r();
	case 0x170000:
		return ns10_control;
	case 0x3fff0:
		return ns10_gun_adc(0) | (ns10_gun_adc(1) << 16);
	case 0x3fff4:
		return 0;
	}
	if (offset >= 0x20000 / 4 && offset < 0x40000 / 4)
		return ns10_sdram_r32(ns10_sdram_bank * 0x20000 + offset * 4 - 0x20000);
	return 0xffffffff;
}

static WRITE32_HANDLER(ns10_register_w)
{
	UINT8 *rom = ns10_nand();
	int i, iv;
	UINT32 page = ns10_address >> 8;
	if (offset >= 0x20000 / 4 && offset < 0x40000 / 4)
	{
		UINT8 *sdram = ns10_sdram + ns10_sdram_bank * 0x20000 + offset * 4 - 0x20000;
		for (i = 0; i < 4; i++)
			if (!(mem_mask & (0xffU << (i * 8))))
				sdram[i] = data >> (i * 8);
		return;
	}
	/* 0x1f300006 raises INT5 of the MCU */
	if (offset == 1)
	{
		if (ACCESSING_MSW32)
			ns10_mp3_command();
		return;
	}
	if ((mem_mask & 0xffff) == 0xffff)
		return;
	switch (offset * 4)
	{
	case 0:
		/* restarts the MCU, the MEM(P3) board has no crypto switch here */
		ns10_mp3_reset();
		break;
	case 0xc:
		ns10_sdram_bank = data & 0x3f;
		break;
	case 0x80000:
		ns10_active = (data & 0x8000) != 0;
		if (ns10_active)
		{
			static const UINT8 sbox[16] = {0, 12, 13, 6, 2, 4, 9, 8, 11, 1, 7, 15, 10, 5, 14, 3};
			iv = data & 15;
			if (ns10_crypto->iv)
				ns10_cipher = ns10_crypto->iv[iv];
			else
			{
				iv = sbox[iv];
				ns10_cipher = ((iv & 8) << 12) | ((iv & 4) << 10) | ((iv & 2) << 10) | ((iv & 1) << 8);
			}
			ns10_plain = 0;
			ns10_mask = 0;
		}
		break;
	case 0x110000:
		ns10_command = data & 0xff;
		switch (ns10_command)
		{
		case 0xff:
		case 0x00:
			ns10_area = 0;
			break;
		case 0x01:
			ns10_area = 1;
			break;
		case 0x50:
			ns10_area = 2;
			break;
		case 0x80:
			memset(ns10_program, 0xff, sizeof(ns10_program));
			break;
		case 0x10:
			if (rom && ns10_bank == 0 && page >= 0x20 && page < 0x40)
				for (i = 0; i < NS10_PAGE_SIZE; i++)
					rom[page * NS10_PAGE_SIZE + i] &= ns10_program[i];
			break;
		case 0xd0:
			if (rom && ns10_bank == 0 && (page >> 5) == 1)
				memset(rom + 0x20 * NS10_PAGE_SIZE, 0xff, 0x20 * NS10_PAGE_SIZE);
			break;
		}
		break;
	case 0x120000:
		ns10_address = (ns10_address & 0xffff00) | (data & 0xff);
		ns10_column = (data & (ns10_area == 2 ? 15 : 255)) + (ns10_area == 2 ? 512 : ns10_area * 256);
		break;
	case 0x130000:
		ns10_address = (ns10_address & 0xff00ff) | ((data & 255) << 8);
		break;
	case 0x140000:
		ns10_address = (ns10_address & 0x00ffff) | ((data & 255) << 16);
		break;
	case 0x150000:
		if (ns10_command == 0x80 && ns10_column + 1 < NS10_PAGE_SIZE)
		{
			ns10_program[ns10_column++] = data >> 8;
			ns10_program[ns10_column++] = data;
		}
		break;
	case 0x160000:
		if (!(ns10_control & 4))
			ns10_bank = data & 0xffff;
		break;
	case 0x170000:
		ns10_control = data & 0xffff;
		break;
	}
}

static UINT16 ns10_i2c_clock, ns10_i2c_data;
static int ns10_i2c_device, ns10_i2c_prev_clock, ns10_i2c_prev_data;
static int ns10_i2c_mode, ns10_i2c_bit;

static void ns10_i2c_update(void)
{
	int clock = ns10_i2c_clock & 1;
	int data = ns10_i2c_data & ns10_i2c_device & 1;
	if (clock == ns10_i2c_prev_clock && data == ns10_i2c_prev_data)
		return;
	switch (ns10_i2c_mode)
	{
	case 0:
		if (clock && !data)
		{
			ns10_i2c_bit = 7;
			ns10_i2c_mode = 1;
		}
		break;
	case 1:
		if (clock && data && !ns10_i2c_prev_data)
			ns10_i2c_mode = 0;
		else if (clock && !ns10_i2c_prev_clock && --ns10_i2c_bit < 0)
		{
			ns10_i2c_mode = 2;
			ns10_i2c_device = data = 0;
		}
		break;
	case 2:
		if (clock && !ns10_i2c_prev_clock)
			ns10_i2c_mode = 3;
		break;
	case 3:
		if (!clock && ns10_i2c_prev_clock)
		{
			ns10_i2c_device = 1;
			data = ns10_i2c_data & 1;
			ns10_i2c_bit = 7;
			ns10_i2c_mode = 1;
		}
		break;
	}
	ns10_i2c_prev_clock = clock;
	ns10_i2c_prev_data = data;
}

static READ32_HANDLER(ns10_io_r)
{
	switch (offset)
	{
	case 0:
		return readinputport(0) | 0x00030000;
	case 1:
		return readinputport(1);
	case 2:
		return (ns10_i2c_clock & 1) | ((ns10_i2c_data & ns10_i2c_device & 1) << 16);
	case 6:
		return (UINT32)ns10_interrupt << 16;
	}
	return 0xffffffff;
}

static WRITE32_HANDLER(ns10_io_w)
{
	int i;
	if (offset == 2)
	{
		if (ACCESSING_LSW32)
			ns10_i2c_clock = data & 0xffff;
		if (ACCESSING_MSW32)
			ns10_i2c_data = data >> 16;
		ns10_i2c_update();
	}
	for (i = 0; i < 2; i++)
	{
		int byte = offset * 4 + i * 2;
		if (!(mem_mask & (0xffffU << (i * 16))) && byte >= 0x12 && byte <= 0x18)
			ns10_interrupt &= ~(1 << ((byte - 0x12) / 2));
	}
}

static void ns10_io_tick(int param)
{
	ns10_interrupt |= 8;
	psx_irq_set(0x400);
}

static NVRAM_HANDLER(ns10)
{
	UINT8 *settings = memory_region(REGION_USER2) + 0x20 * NS10_PAGE_SIZE;
	if (read_or_write)
		mame_fwrite(file, settings, 0x20 * NS10_PAGE_SIZE);
	else if (file)
		mame_fread(file, settings, 0x20 * NS10_PAGE_SIZE);
}

/* The boot image is assembled from two page ranges, leaving its RAM gap. */
static void ns10_init(const struct ns10_crypto *crypto)
{
	UINT8 *rom = memory_region(REGION_USER2);
	UINT16 *bios = (UINT16 *)memory_region(REGION_USER1);
	int page, i, dst;
	ns10_crypto = crypto;
	dst = 0;
	for (page = 0x40; page < 0x2020; page++)
	{
		if (page == 0x120)
			dst = 0x10000;
		for (i = 0; i < 0x200; i += 2)
			bios[dst++] = ns10_unscramble((rom[page * NS10_PAGE_SIZE + i] << 8) | rom[page * NS10_PAGE_SIZE + i + 1]);
	}
	if (bios[0] != 0x0013 || bios[1] != 0x3c08 || bios[2] != 0x24ff || bios[3] != 0x3508)
		fatalerror("Invalid System 10 NAND boot image. Use complete 0x1080000-byte NAND dumps, including spare bytes, and check dir_rom search order.");
	ns10_sdram = auto_malloc(NS10_SDRAM_SIZE);
	psx_driver_init();
	psx_dma_install_read_handler(5, ns10_dma_read);
	timer_pulse(TIME_IN_HZ(100), 0, ns10_io_tick);
}

static DRIVER_INIT(g13jnr)
{
	ns10_init(&g13jnr_crypto);
}

static ADDRESS_MAP_START( ns10_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize)
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READNOP
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fffffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x9fc00000, 0x9fffffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xbfc00000, 0xbfffffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
	AM_RANGE(0x1f300000, 0x1f47ffff) AM_READWRITE(ns10_register_r, ns10_register_w)
	AM_RANGE(0x1f500000, 0x1f5fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x1fba0000, 0x1fba001b) AM_READWRITE(ns10_io_r, ns10_io_w)
	AM_RANGE(0x9f300000, 0x9f47ffff) AM_READWRITE(ns10_register_r, ns10_register_w)
	AM_RANGE(0x9f500000, 0x9f5fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x9fba0000, 0x9fba001b) AM_READWRITE(ns10_io_r, ns10_io_w)
	AM_RANGE(0xbf300000, 0xbf47ffff) AM_READWRITE(ns10_register_r, ns10_register_w)
	AM_RANGE(0xbf500000, 0xbf5fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0xbfba0000, 0xbfba001b) AM_READWRITE(ns10_io_r, ns10_io_w)
ADDRESS_MAP_END

static struct PSXSPUinterface ns10_spu =
{
	&g_p_n_psxram,
	psx_irq_set,
	psx_dma_install_read_handler,
	psx_dma_install_write_handler
};

static VIDEO_UPDATE(ns10)
{
	const rectangle *visible = &Machine->visible_area;
	int x = visible->min_x + readinputport(3) * (visible->max_x - visible->min_x) / 0x3ff;
	int y = visible->min_y + readinputport(2) * (visible->max_y - visible->min_y) / 0x3ff;

	video_update_psx(screen, bitmap, cliprect);
	draw_crosshair(bitmap, x, y, cliprect, 0);
}

static MACHINE_RESET(ns10)
{
	psx_machine_init();
	ns10_address = ns10_column = 0;
	ns10_bank = ns10_control = ns10_interrupt = 0;
	ns10_active = 0;
	ns10_command = ns10_area = 0;
	ns10_cipher = ns10_plain = 0;
	ns10_mask = 0;
	ns10_i2c_clock = ns10_i2c_data = ns10_i2c_device = 1;
	ns10_i2c_prev_clock = ns10_i2c_prev_data = 1;
	ns10_i2c_mode = 0;
	ns10_i2c_bit = 7;
	memset(ns10_sdram, 0, NS10_SDRAM_SIZE);
	ns10_sdram_bank = 0;
	ns10_mp3_reset();
}

static MACHINE_DRIVER_START( ns10 )
	MDRV_CPU_ADD_TAG( "main", PSXCPU, 33868800 / 2 )
	MDRV_CPU_PROGRAM_MAP( ns10_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )
	MDRV_MACHINE_RESET(ns10)
	MDRV_NVRAM_HANDLER(ns10)

	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )
	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( ns10 )

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(PSXSPU, 101491200 / 2)
	MDRV_SOUND_CONFIG(ns10_spu)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)

	/* MP3 decoder of the MEM(P3) ROM board, mixed at the same level as MAME */
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(ns10_mp3_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

INPUT_PORTS_START( g13jnr )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0xfb00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0001, 0x0001, "Show Crosshair" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x07ff7f43, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("Down Select")
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("Up Select")
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Enter")
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_START2 )

	/* high when the player is at 10 cm or less from the scope, toggled to avoid holding the key */
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Scope Sensor") PORT_TOGGLE

	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_TOGGLE
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("GUNY")
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x000, 0x3ff) PORT_SENSITIVITY(100) PORT_KEYDELTA(50) PORT_PLAYER(1)

	PORT_START_TAG("GUNX")
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_X ) PORT_MINMAX(0x000, 0x3ff) PORT_SENSITIVITY(100) PORT_KEYDELTA(50) PORT_PLAYER(1)

INPUT_PORTS_END

ROM_START( g13jnr )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 )
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x6300000, REGION_USER2, 0 )
	ROM_LOAD( "glt1vera_0.2j", 0x0000000, 0x1080000, CRC(24f5bb06) SHA1(cb61b754e7c1afaeb3cff5c588d28f68a1eca092) )

	ROM_LOAD( "glt1vera_1.1j", 0x1080000, 0x1080000, CRC(5ec552da) SHA1(d23f73ae4694c581d3827afac7f62bc6ce83eb30) )

	ROM_LOAD( "glt1vera_2.2h", 0x2100000, 0x1080000, CRC(0835d6ed) SHA1(5dc4e734aad91e9ab722d11c57eff11e0a100657) )

	ROM_LOAD( "glt1vera_3.1h", 0x3180000, 0x1080000, CRC(f7486979) SHA1(a44c33ae7004e79fe66c6d2cba3d11671ce2582c) )

	ROM_LOAD( "glt1vera_4.2f", 0x4200000, 0x1080000, CRC(e60b526a) SHA1(3599af2ebb894833d1233cd76d35644fc95a647b) )

	ROM_LOAD( "glt1vera_5.1f", 0x5280000, 0x1080000, CRC(0cb2df20) SHA1(b0e10b6d00f3cc20103177faca0c14d98b10994d) )
ROM_END

GAME( 2001, g13jnr, 0, ns10, g13jnr, g13jnr, ROT0, "Eighting / Raizing / Namco", "Golgo 13: Juusei no Requiem (Japan, GLT1 VER.A)", GAME_IMPERFECT_SOUND )

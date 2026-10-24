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

static UINT16 knpuzzle_nonlinear(UINT64 previous_cipherwords, UINT64 previous_plainwords)
{
	UINT64 previous_masks = previous_cipherwords ^ previous_plainwords;
	return ((previous_masks >> 0x13) &
			(gf2_reduce(0x0000000014001290ULL & previous_cipherwords) ^
			 gf2_reduce(0x0000000000021290ULL & previous_plainwords)) &
			1)
		   << 1;
}

static const struct ns10_crypto knpuzzle_crypto = {
	{
		0x00000000c0a4208cULL, 0x00000000204100a8ULL, 0x000000000c0306a0ULL, 0x000000000819e944ULL,
		0x0000000000001400ULL, 0x0000000000000061ULL, 0x000000000141401cULL, 0x0000000000000020ULL,
		0x0000000001418010ULL, 0x00008d6a1eb690cfULL, 0x00008d6a4d3b90ceULL, 0x0000000000004201ULL,
		0x00000000012c00a2ULL, 0x000000000c0304a4ULL, 0x0000000000000500ULL, 0x0000000000000980ULL,
	},
	{
		0x000000002a22608cULL, 0x00000000002300a8ULL, 0x0000000000390ea0ULL, 0x000000000100a9c4ULL,
		0x0000000000001400ULL, 0x0000000000000041ULL, 0x0000000003014014ULL, 0x0000000000000022ULL,
		0x0000000003010110ULL, 0x00000800031a80cfULL, 0x00000800003398deULL, 0x0000000000004200ULL,
		0x00000000012a04a2ULL, 0x00000000003984a4ULL, 0x0000000000000700ULL, 0x0000000000000882ULL,
	},
	0x01e2,
	knpuzzle_nonlinear,
	NULL,
	{0xe, 0xf, 0xc, 0xd, 0xa, 0x8, 0xb, 0x9, 0x4, 0x5, 0x6, 0x7, 0x1, 0x3, 0x0, 0x2}};

static UINT16 mrdrilrg_nonlinear(UINT64 previous_cipherwords, UINT64 previous_plainwords)
{
	UINT64 previous_masks = previous_cipherwords ^ previous_plainwords;
	return (1 & gf2_reduce(previous_masks & 0x120100400000) & gf2_reduce(previous_masks & 0x11800020000)) * 0x11;
}

static const UINT16 mrdrilrg_iv[16] = {
	0x0000, 0x0000, 0x0000, 0x2000, 0x801a, 0x6000, 0x0002, 0x0000,
	0x0000, 0x0018, 0x0000, 0x0420, 0x0000, 0x8800, 0x0000, 0xc000,
};

static const struct ns10_crypto mrdrilrg_crypto = {
	{
		0x0000115c68620401ULL, 0x0000008102802094ULL, 0x00000081a65270a8ULL, 0x0000000000810010ULL,
		0x00004c2c080f3059ULL, 0x00000081470a9500ULL, 0x000000816a123009ULL, 0x00000081a2b23038ULL,
		0x00002d18a06b9000ULL, 0x000001186c02000cULL, 0x00000081a2b2704cULL, 0x0000702811016302ULL,
		0x00000081470a1122ULL, 0x0000000000006200ULL, 0x00009170ac403013ULL, 0x0000000000001044ULL,
	},
	{
		0x0000115c68620401ULL, 0x0000008102802094ULL, 0x00000081a65270a8ULL, 0x0000000000810010ULL,
		0x00004c2c080f3059ULL, 0x00000081470a9500ULL, 0x000000816a123009ULL, 0x00000081a2b23038ULL,
		0x00002d18a06b9000ULL, 0x000001186c02000cULL, 0x00000081a2b2704cULL, 0x0000702811016302ULL,
		0x00000081470a1122ULL, 0x0000000000006200ULL, 0x00009170ac403013ULL, 0x0000000000001044ULL,
	},
	0x0000,
	mrdrilrg_nonlinear,
	mrdrilrg_iv,
	{0xe, 0xc, 0xf, 0xd, 0xa, 0x9, 0x8, 0xb, 0x4, 0x5, 0x6, 0x7, 0x0, 0x1, 0x3, 0x2}};

static UINT16 panikuru_nonlinear(UINT64 previous_cipherwords, UINT64 previous_plainwords)
{
	return (1 &
			((gf2_reduce(0x0000000088300281ULL & previous_cipherwords) ^
			  gf2_reduce(0x0000000004600281ULL & previous_plainwords))) &
			((gf2_reduce(0x0000a13140090000ULL & previous_cipherwords) ^
			  gf2_reduce(0x0000806240090000ULL & previous_plainwords))))
		   << 2;
}

static const UINT16 panikuru_iv[16] = {
	0x0000, 0x020e, 0x0412, 0x0411, 0x0204, 0x001b, 0x0009, 0x0207,
	0x2040, 0x2440, 0x0012, 0x0001, 0x0614, 0x000a, 0x0214, 0x041b,
};

static const struct ns10_crypto panikuru_crypto = {
	{
		0x0000000000000130ULL, 0x0000000018802004ULL, 0x0000b04a04781081ULL, 0x0000000006802000ULL,
		0x0000000000000c02ULL, 0x0000000042040020ULL, 0x0000000000000003ULL, 0x0000b040c00c0940ULL,
		0x0000000000000009ULL, 0x00001020a0010400ULL, 0x0000000010204104ULL, 0x000000005a840004ULL,
		0x0000000000000290ULL, 0x0000000010204380ULL, 0x0000000000001041ULL, 0x0000000000006100ULL,
	},
	{
		0x0000000000000120ULL, 0x0000000028002004ULL, 0x0000010800480001ULL, 0x0000000006040100ULL,
		0x0000000000000c00ULL, 0x0000000042000820ULL, 0x0000000000001003ULL, 0x0000910212040100ULL,
		0x0000000000008009ULL, 0x0000900021018000ULL, 0x0000000090004104ULL, 0x000000006a004004ULL,
		0x0000000000000214ULL, 0x0000000090004190ULL, 0x0000000000001040ULL, 0x0000000000006008ULL,
	},
	0x7c29,
	panikuru_nonlinear,
	panikuru_iv,
	{0xe, 0xc, 0xf, 0xd, 0x8, 0x9, 0xa, 0xb, 0x4, 0x7, 0x6, 0x5, 0x1, 0x0, 0x3, 0x2}};

static UINT16 startrgn_nonlinear(UINT64 previous_cipherwords, UINT64 previous_plainwords)
{
	UINT64 previous_masks = previous_cipherwords ^ previous_plainwords;
	return ((previous_masks >> 12) & (previous_masks >> 14) & 1) << 4;
}

static const struct ns10_crypto startrgn_crypto = {
	{
		0x00003e4bfe92c6a9ULL, 0x000000000000010cULL, 0x00003e4b7bd6c4aaULL, 0x0000b1a904b8fab8ULL,
		0x0000000000000080ULL, 0x0000000000008c00ULL, 0x0000b1a9b2f0b4cdULL, 0x000000006c100828ULL,
		0x000000006c100838ULL, 0x0000b1a9d3913fcdULL, 0x000000006161aa00ULL, 0x0000000000006040ULL,
		0x0000000000000420ULL, 0x0000000000001801ULL, 0x00003e4b7bd6deabULL, 0x0000000000000105ULL,
	},
	{
		0x000012021f00c6a8ULL, 0x0000000000000008ULL, 0x000012020b1046aaULL, 0x000012001502fea8ULL,
		0x0000000000002000ULL, 0x0000000000008800ULL, 0x000012001e02b4cdULL, 0x000000002c0008aaULL,
		0x000000002c00083aULL, 0x000012003f027ecdULL, 0x0000000021008a00ULL, 0x0000000000002040ULL,
		0x0000000000000428ULL, 0x0000000000001001ULL, 0x000012020b10ceabULL, 0x0000000000000144ULL,
	},
	0x8c46,
	startrgn_nonlinear,
	NULL,
	{0xe, 0xd, 0xc, 0xf, 0x9, 0xb, 0x8, 0xa, 0x4, 0x5, 0x6, 0x7, 0x0, 0x3, 0x2, 0x1}};

static UINT16 konotako_nonlinear(UINT64 previous_cipherwords, UINT64 previous_plainwords)
{
	UINT64 previous_masks = previous_cipherwords ^ previous_plainwords;
	return ((previous_masks >> 7) & (previous_masks >> 15) & 1) << 15;
}

static const struct ns10_crypto konotako_crypto = {
	{
		0x000000000000004cULL, 0x00000000d39e3d3dULL, 0x0000000000001110ULL, 0x0000000000002200ULL,
		0x000000003680c008ULL, 0x0000000000000281ULL, 0x0000000000005002ULL, 0x00002a7371895a47ULL,
		0x0000000000000003ULL, 0x00002a7371897a4eULL, 0x00002a73aea17a41ULL, 0x00002a73fd895a4fULL,
		0x000000005328200aULL, 0x0000000000000010ULL, 0x0000000000000040ULL, 0x0000000000000200ULL,
	},
	{
		0x000000000000008cULL, 0x0000000053003d25ULL, 0x0000000000001120ULL, 0x0000000000002200ULL,
		0x0000000037004008ULL, 0x0000000000000282ULL, 0x0000000000006002ULL, 0x0000060035005a47ULL,
		0x0000000000000003ULL, 0x0000060035001a4eULL, 0x0000060025007a41ULL, 0x00000600b5005a2fULL,
		0x000000009000200bULL, 0x0000000000000310ULL, 0x0000000000001840ULL, 0x0000000000000400ULL,
	},
	0x0748,
	konotako_nonlinear,
	NULL,
	{0xe, 0xf, 0xc, 0xd, 0x8, 0x9, 0xb, 0xa, 0x5, 0x4, 0x7, 0x6, 0x0, 0x1, 0x3, 0x2}};

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
	}
	return 0xffffffff;
}

static WRITE32_HANDLER(ns10_register_w)
{
	UINT8 *rom = ns10_nand();
	int i, iv;
	UINT32 page = ns10_address >> 8;
	if ((mem_mask & 0xffff) == 0xffff)
		return;
	switch (offset * 4)
	{
	case 0:
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
	psx_driver_init();
	psx_dma_install_read_handler(5, ns10_dma_read);
	timer_pulse(TIME_IN_HZ(100), 0, ns10_io_tick);
}

static DRIVER_INIT(knpuzzle)
{
	ns10_init(&knpuzzle_crypto);
}

static DRIVER_INIT(mrdrilrg)
{
	ns10_init(&mrdrilrg_crypto);
}

static DRIVER_INIT(panikuru)
{
	ns10_init(&panikuru_crypto);
}

static DRIVER_INIT(startrgn)
{
	ns10_init(&startrgn_crypto);
}

static DRIVER_INIT(konotako)
{
	ns10_init(&konotako_crypto);
}

static DRIVER_INIT(g13jnr)
{
	ns10_init(&g13jnr_crypto);
}

static ADDRESS_MAP_START( namcos10_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
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
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_NOP
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fffffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fffffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfffffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

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

static void memcpy32le( UINT32 *dst, UINT8 *src, int len )
{
	while( len > 0 )
	{
		*( dst ) = ( *( src + 0 ) << 0 ) | ( *( src + 1 ) << 8 ) | ( *( src + 2 ) << 16 ) | ( *( src + 3 ) << 24 );
		dst++;
		src += 4;
		len -= 4;
	}
}

static void memm_driver_init( void )
{
	psx_driver_init();
}

static void memn_driver_init( void )
{
	UINT8 *BIOS = (UINT8 *)memory_region( REGION_USER1 );
	UINT8 *ROM = (UINT8 *)memory_region( REGION_USER2 );

	memcpy32le( (UINT32 *)( BIOS + 0x0000000 ), ROM + 0x08000, 0x001c000 );
	memcpy32le( (UINT32 *)( BIOS + 0x0020000 ), ROM + 0x24000, 0x03dffff );

	psx_driver_init();
}

static void decrypt_bios( int b15, int b14, int b13, int b12, int b11, int b10, int b9, int b8,
	int b7, int b6, int b5, int b4, int b3, int b2, int b1, int b0 )
{
	UINT16 *BIOS = (UINT16 *)memory_region( REGION_USER1 );
	int len = memory_region_length( REGION_USER1 ) / 2;
	int i;

	for( i = 0; i < len; i++ )
	{
		BIOS[ i ] = BITSWAP16( BIOS[ i ] ^ 0xaaaa,
			b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0 );
	}
}

static DRIVER_INIT( mrdrilr2 )
{
	memm_driver_init();
	decrypt_bios( 0xc, 0xd, 0xf, 0xe, 0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0x4, 0x1, 0x2, 0x5, 0x0, 0x3 );
}

static DRIVER_INIT( gjspace )
{
	memn_driver_init();
	decrypt_bios( 0x0, 0x2, 0xe, 0xd, 0xf, 0x6, 0xc, 0x7, 0x5, 0x1, 0x9, 0x8, 0xa, 0x3, 0x4, 0xb );
}

static DRIVER_INIT( gamshara )
{
	memn_driver_init();
	decrypt_bios( 0x5, 0x4, 0x7, 0x6, 0x0, 0x1, 0x3, 0x2, 0xd, 0xf, 0xc, 0xe, 0x8, 0x9, 0xa, 0xb );
}

MACHINE_RESET( namcos10 )
{
	psx_machine_init();
}

static MACHINE_DRIVER_START( namcos10 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( namcos10_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( namcos10 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
MACHINE_DRIVER_END

static struct PSXSPUinterface ns10_spu =
{
	&g_p_n_psxram,
	psx_irq_set,
	psx_dma_install_read_handler,
	psx_dma_install_write_handler
};

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
}

static MACHINE_DRIVER_START( ns10 )
	MDRV_IMPORT_FROM(namcos10)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(ns10_map, 0)
	MDRV_MACHINE_RESET(ns10)
	MDRV_NVRAM_HANDLER(ns10)
	MDRV_SOUND_ADD(PSXSPU, 101491200 / 2)
	MDRV_SOUND_CONFIG(ns10_spu)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)
MACHINE_DRIVER_END

INPUT_PORTS_START( namcos10 )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x7fff, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 )
	PORT_BIT( 0x000f, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_SERVICE2 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x000f, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END

INPUT_PORTS_START( ns10 )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0xfb00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0f110000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)

	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)

	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_TOGGLE
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("EXIO_IN1")
	PORT_BIT( 0xffffffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("EXIO_IN2")
	PORT_BIT( 0xffffffff, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( ns10_mrdrilr2 )
	PORT_INCLUDE(ns10)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)

INPUT_PORTS_END

INPUT_PORTS_START( ns10_startrgn )
	PORT_INCLUDE(ns10)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x0fffff6f, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( ns10_konotako )
	PORT_INCLUDE(ns10)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x1fff4040, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("SYSTEM")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(0x00, "Export (Cristaltec license)")
	PORT_DIPSETTING(0x01, DEF_STR( Japan ) )
INPUT_PORTS_END

ROM_START( mrdrilr2 )
	ROM_REGION32_LE( 0x800000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD( "dr21vera.1a",  0x000000, 0x800000, CRC(f93532a2) SHA1(8b72f2868978be1f0e0abd11425a3c8b2b0c4e99) )
ROM_END

ROM_START( gjspace )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 ) /* bios */
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x4000000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD( "10011a_0.bin", 0x0000000, 0x1000000, CRC(853e329e) SHA1(dd272ab2de9052c5aeee9e0ead4ee3d14347593e) )
	ROM_LOAD( "10011a_1.bin", 0x1000000, 0x1000000, CRC(7d4e96f7) SHA1(41b0d27b93662f15547311c8723ac5141e129cab) )
	ROM_LOAD( "10011a_2.bin", 0x2000000, 0x1000000, CRC(83111de6) SHA1(d760fd41644104f6d3a2806ab4664df2c77bdc42) )
	ROM_LOAD( "10011a_3.bin", 0x3000000, 0x1000000, CRC(b6ea8be7) SHA1(3e65deda2b0e6758ce0ed05419ed096efb8ad755) )
ROM_END

ROM_START( mrdrilrg )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 )
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x3180000, REGION_USER2, 0 )
	ROM_LOAD( "0.8e", 0x0000000, 0x1080000, CRC(4b61ac62) SHA1(b91076ce3d99fbe1f9247c66a577af3c3d468b20) )

	ROM_LOAD( "1.8d", 0x1080000, 0x1080000, CRC(c87b5e86) SHA1(b034210da30e1f2f7d04f77e00bf7724437e2024) )

	ROM_LOAD( "2.7e", 0x2100000, 0x1080000, CRC(e0a9339f) SHA1(4284e7233876cfaf8021440d78ccc8c70d00cc00) )
ROM_END

ROM_START( knpuzzle )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 )
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x3180000, REGION_USER2, 0 )
	ROM_LOAD( "kpm1vera_0.8e", 0x0000000, 0x1080000, CRC(4b4255da) SHA1(c8ec575e53596a167a07db97076fd69e6646d0f5) )

	ROM_LOAD( "kpm1vera_1.8d", 0x1080000, 0x1080000, CRC(644595a6) SHA1(4b60008ca5cac894a815fe6aaa980296a83f673f) )

	ROM_LOAD( "kpm1vera_2.7e", 0x2100000, 0x1080000, CRC(6bf164e5) SHA1(b4a2e6eb18c09220b0c8ec80159d13d0e439a559) )
ROM_END

ROM_START( startrgn )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 )
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x2100000, REGION_USER2, 0 )
	ROM_LOAD( "stt1a_0.bin", 0x0000000, 0x1080000, CRC(1e090644) SHA1(a7a293e2bd9eea2eb64a492a47272d9d9ee2c724) )

	ROM_LOAD( "stt1a_1.bin", 0x1080000, 0x1080000, CRC(aa527694) SHA1(a25dcbeca58a1443070848b3487a24d51d41a34b) )
ROM_END

ROM_START( gamshara )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 ) /* bios */
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD( "10021a.8e",    0x0000000, 0x1000000, CRC(db0a3687) SHA1(757360c43fba247e6c0c0fc66bb089575e19d646) )
	ROM_LOAD( "10021a.8d",    0x1000000, 0x1000000, CRC(bebafef6) SHA1(6c965c09f3186523d27c50894a15b9c1dd02a794) )
ROM_END

ROM_START( panikuru )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 )
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x3180000, REGION_USER2, 0 )
	ROM_LOAD( "0.8e", 0x0000000, 0x1080000, CRC(a68b0965) SHA1(037e41e80a62675ffe1c820060c8bb414fb84c30) )

	ROM_LOAD( "1.8d", 0x1080000, 0x1080000, CRC(18e5135d) SHA1(a7b1533a1df71be5498718e301d1c9c548551fb4) )

	ROM_LOAD( "2.7e", 0x2100000, 0x1080000, CRC(cd3b25e0) SHA1(39dfebc59e71b8f1c28e718ee71032620f11440c) )
ROM_END

ROM_START( konotako )
	ROM_REGION32_LE( 0x400000, REGION_USER1, 0 )
	ROM_FILL( 0x0000000, 0x400000, 0x55 )

	ROM_REGION( 0x2100000, REGION_USER2, 0 )
	ROM_LOAD( "0.8e", 0x0000000, 0x1080000, CRC(63d23a0c) SHA1(31b54119f20827ff13ecf0cd87803a5e27eaafe7) )

	ROM_LOAD( "1.8d", 0x1080000, 0x1080000, CRC(bdbed53c) SHA1(5773069c43642e6f334cee185a6fb6908eedcf4a) )
ROM_END

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

GAME( 2000, mrdrilr2,  0,        namcos10, namcos10, mrdrilr2, ROT0, "Namco", "Mr Driller 2 (DR21 Ver.A)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 2001, gjspace,   0,        namcos10, namcos10, gjspace,  ROT0, "Namco/Metro", "Gekitoride-Jong Space (10011 Ver.A)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 2001, knpuzzle, 0, ns10, ns10, knpuzzle, ROT0, "Namco", "Kotoba no Puzzle Mojipittan (Japan, KPM1 Ver.A)", GAME_IMPERFECT_SOUND )
GAME( 2001, mrdrilrg, 0, ns10, ns10_mrdrilr2, mrdrilrg, ROT0, "Namco", "Mr. Driller G (Japan, DRG1 Ver.A)", GAME_IMPERFECT_SOUND )
GAME( 2003, gamshara,  0,        namcos10, namcos10, gamshara, ROT0, "Mitchell", "Gamshara (10021 Ver.A)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 2002, panikuru, 0, ns10, ns10, panikuru, ROT0, "Namco", "Panikuru Panekuru (Japan, PPA1 Ver.A)", GAME_IMPERFECT_SOUND )
GAME( 2002, startrgn, 0, ns10, ns10_startrgn, startrgn, ROT0, "Namco", "Star Trigon (Japan, STT1 Ver.A)", GAME_IMPERFECT_SOUND )
GAME( 2003, konotako, 0, ns10, ns10_konotako, konotako, ROT0, "Mitchell", "Kono e Tako (10021 Ver.A)", GAME_IMPERFECT_SOUND )
GAME( 2001, g13jnr, 0, ns10, ns10, g13jnr, ROT0, "Eighting / Raizing / Namco", "Golgo 13: Juusei no Requiem (Japan, GLT1 VER.A)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND )

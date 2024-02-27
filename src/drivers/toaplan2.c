/*****************************************************************************

        ToaPlan      game hardware from 1991 - 1994
        Raizing/8ing game hardware from 1993 onwards
        -------------------------------------------------
        Driver by: Quench and Yochizo

   Raizing games and Truxton 2 are heavily dependent on the Raine source -
   many thanks to Richard Bush and the Raine team. [Yochizo]



Supported games:

    Name        Board No      Maker         Game name
    ----------------------------------------------------------------------------
    tekipaki    TP-020        Toaplan       Teki Paki
    ghox        TP-021        Toaplan       Ghox
    dogyuun     TP-022        Toaplan       Dogyuun
    dogyuunto   TP-022        Toaplan       Dogyuun (8/25/1992 location test)
    kbash       TP-023        Toaplan       Knuckle Bash
    kbash2      bootleg       Toaplan       Knuckle Bash 2
    truxton2    TP-024        Toaplan       Truxton 2 / Tatsujin 2
    pipibibs    TP-025        Toaplan       Pipi & Bibis
    whoopee     TP-025        Toaplan       Whoopee
    pipibibi    bootleg       Toaplan       Pipi & Bibis
    fixeight    TP-026        Toaplan       FixEight
    fixeighb    bootleg       Toaplan       FixEight
    grindstm    TP-027        Toaplan       Grind Stormer  (1992)
    grindsta    TP-027        Toaplan       Grind Stormer  (1992) (older)
    vfive       TP-027        Toaplan       V-V  (V-Five)  (1993 - Japan only)
    batsugun    TP-030        Toaplan       Batsugun
    batugnsp    TP-030        Toaplan       Batsugun  (Special Version)
    snowbro2    ??????        Toaplan       Snow Bros. 2 - With New Elves

    sstriker    RA-MA7893-01  Raizing       Sorcer Striker
    mahoudai    RA-MA7893-01  Raizing       Mahou Daisakusen
    kingdmgp    RA-MA9402-03  Raizing/8ing  Kingdom Grandprix
    shippumd    RA-MA9402-03  Raizing/8ing  Shippu Mahou Daisakusen
    battleg     RA9503        Raizing/8ing  Battle Garegga (Type 2)
    battlega    RA9503        Raizing/8ing  Battle Garegga
    battlegb    RA9503        Raizing/8ing  Battle Garegga (New Version?)
    battlegc    RA9503        Raizing/8ing  Battle Garegga
    batrider    RA9704        Raizing/8ing  Armed Police Batrider - Rev B (Japan - Ver. Fri Feb 13 1998)
    batridra    RA9704        Raizing/8ing  Armed Police Batrider (Japan - Ver. Mon Dec 22 1997)
    batridrk    RA9704        Raizing/8ing  Armed Police Batrider (Korea - Ver. Fri Feb 13 1998)
    bbakraid    ET68-V99      8ing          Battle Bakraid (Japan - Wed Apr 7th, 1999)
    bbakradu    ET68-V99      8ing          Battle Bakraid - unlimited version (Japan - Tue Jun 8th, 1999)

    SET NOTES:

    sstriker - might be a bootleg / hack, the region jumper is read with this set but even when set
               to Japan you get all english text etc which seems a little odd, but might be correct.
    kingdmgp - might be a bootleg / hack, some of the tiles needed for the credits screen have been
               stripped out, doesn't seem very professional, since its rare and should probably only
               have a different graphics rom its nearly impossible for us to verify, a lot of boards
               being sold as 'Kingdom Grand Prix' are infact conversions using Neill Corlett's
               ghastly hack

               ** update the above two look like genuine Korean release
          ??      boards, Raizing probably just missed a few things

 ****************************************************************************
 * Battle Garegga and Armed Police Batrider have secret characters.         *
 * Try to input the following commands to use them.                         *
 * ======================================================================== *
 * Battle Garegga                                                           *
 *      After inserting a coin (pushing a credit button), input             *
 *      UP  UP  DOWN  DOWN  LEFT  RIGHT  LEFT  RIGHT  A  B  C  START        *
 *      then you can use Mahou Daisakusen characters.                       *
 *                                                                          *
 * ------------------------------------------------------------------------ *
 * Armed Police Batrider                                                    *
 *      After inserting a coin (pushing a credit button), input             *
 *      UP  UP  DOWN  DOWN  LEFT  RIGHT  LEFT  RIGHT  A  B  START           *
 *      then you can use Mahou Daisakusen and Battle Garegga characters.    *
 *                                                                          *
 ****************************************************************************


 ############################################################################
 # Battle Bakraid 'Unlimited Version' has unlocking codes to gain           #
 # access to extra players and game features.                               #
 # Special thanks go to the 'R8ZING Shooter Tribute' page for finding       #
 # and publishing this info.                                                #
 #                                                                          #
 # ======================================================================== #
 #      PLAYER SELECT: PHASE 2                                              #
 # Result:  3 more fighter planes available:                                #
 # Code:    UP  UP  DOWN  DOWN  LEFT  RIGHT  LEFT  RIGHT  A  B  Start       #
 # Conditions:                                                              #
 #      1. Start from the title screen                                      #
 #      2. Insert Coin                                                      #
 #      3. Watch the 20 sec. counter and enter each part of the code right  #
 #         between the counting.                                            #
 # Example: 12,up,11,up,10,down,9,down,8,left,7,right,6.left,5,r..          #
 # After entering the [B] button a chime should sound. Phase 2 unlocked!    #
 #                                                                          #
 # ------------------------------------------------------------------------ #
 #      PLAYER SELECT: PHASE 3                                              #
 # Result:  2 more fighter planes available:                                #
 # Code:    UP  UP  DOWN  DOWN  LEFT  RIGHT  LEFT  RIGHT  B  A  Start       #
 # Conditions:                                                              #
 #      1. Start from the title screen                                      #
 #      2. Insert Coin                                                      #
 #      3. Watch the 20 sec. counter and enter each part of the code right  #
 #         between the counting.                                            #
 # Important: The entering of this code has to be finished before the       #
 # counter passes 10 ! To do so, you will have to start after coin          #
 # insertion, right before it starts to count:                              #
 # Example: up,19,up,18,down,17,down,16,left,15,right,14.left,..            #
 # After entering the [A] button a chime should sound. Phase 3 unlocked!    #
 #                                                                          #
 # ------------------------------------------------------------------------ #
 #      TEAM EDIT: ENABLE                                                   #
 # Result:  Unlocks the 'team edit' feature to select a team of different   #
 #          ships. See selection secrets on how to enter the team edit mode.#
 # Code:    UP  DOWN  UP  DOWN  LEFT  RIGHT  LEFT  RIGHT  A  B  Start       #
 # Conditions:                                                              #
 #      1. Start from the title screen                                      #
 #      2. Insert Coin                                                      #
 #      3. Watch the 20 sec. counter and enter each part of the code right  #
 #         between the counting.                                            #
 # Important: This code hast to be entered so that the counter is at 0 when #
 # you press the final button [B]. To do so, start after second 9:          #
 # Example: 9,up,8,down,7,up,6,down,5,left,4,right,3,left,2,right,1,A,0,B   #
 # After entering the [B] button a chime should sound. Team edit unlocked!  #
 #                                                                          #
 # ------------------------------------------------------------------------ #
 #      SPECIAL COURSE : ENABLE                                             #
 # Result:  Unlocks the boss mode, a game mode where you fight the bosses   #
 #          only.                                                           #
 # Code:    UP  DOWN  UP  DOWN  LEFT  RIGHT  LEFT  RIGHT  B  A  Start       #
 # Conditions:                                                              #
 #      1. Start from the title screen                                      #
 #      2. Hold [C] button                                                  #
 #      3. Insert Coin                                                      #
 #      4. Watch the 20 sec. counter and enter each part of the code right  #
 #         between the counting.                                            #
 #      5. Release [C] button                                               #
 # After entering the [A] button a chime should sound. Special course       #
 # unlocked!                                                                #
 ############################################################################



*************** Hardware Info ***************

CPU:
 MC68000P10
 TMP68HC000N-16

Sound CPU/MCU:
 HD647180X0FS6 (Hitachi Z180 Compatible CPU with internal 16k ROM)
 Z84C0006PEC (Z80)


Sound Chips:
 YM3812
 YM2151
 YM2151 + YM3014
 YM2151 + M6295
 YM2151 + M6295 + M6295
 YMZ280B-F + YAC516-E (Digital to Analog Converter)


Graphics Custom 208pin QFP:
 GP9001 L7A0498 TOA PLAN

Found on....
Fixeight
Grind Stormer
Truxton II
Ghox
Armed Police Batrider
Battle Garegga
Mahou Daisakusen
Battle Bakraid

**********************************************************************

Fix Eight (bootleg)
Toaplan, 1992

PCB Layout
----------

|--------------------------------------------|
|   1.BIN        PAL               14MHz  PAL|
|   M6295        PAL                         |
|   PAL     6116 4.BIN          681000 681000|
|           6116                             |
|           6116                681000 681000|
|J          6116        PAL                  |
|A                             PAL           |
|M                                           |
|M   62256  62256              PAL           |
|A   2.BIN  3.BIN       PAL                  |
|                       PAL                  |
|       68000           PAL                  |
| DSW2        |------|  5.BIN                |
| DSW1   6264 |TPC   |                       |
| 3.579545MHz |1020  |  6.BIN                |
| 10MHz  6264 |------|  7.BIN                |
|--------------------------------------------|
Notes:
      68000 clock at 10.000MHz
      M6295 clock at 875kHz [14M/16]. Sample rate = 875000 / 165
      VSync at 60Hz
      6116  - 2k   x8 SRAM (x4)
      6264  - 8k   x8 SRAM (x2)
      62256 - 32k  x8 SRAM (x2)
      681000- 128k x8 SRAM (x4)

*********************************************************************

Game status:

Teki Paki                      Working.
Ghox                           Working.
Dogyuun                        Working, but no sound. MCU type is likely a NEC V25+. Chip is a PLCC94 stamped 'TS-002-MACH'.
Dogyuun (location test)        Working.
Knuckle Bash                   Working, but sound FX only (missing music). MCU type is a NEC V25+. Chip is a PLCC94 stamped 'TS-004-DASH'. Some PCBs use another version stamped 'NITRO' which is the same chip type.
Truxton 2                      Working.
Pipi & Bibis                   Working.
Whoopee                        Working.
Pipi & Bibis (Ryouta Kikaku)   Working.
FixEight                       Not working properly. Missing background GFX, and sound FX only (missing music). Both controlled by MCU. MCU type is a NEC V25+. Chip is a PLCC94 stamped 'TS-001-TURBO'
FixEight bootleg               Working. One unknown ROM (same as pipibibi one). Region hardcoded to Korea (@ $4d8)
Grind Stormer                  Working, but no sound. MCU type is a NEC V25+. Chip is a PLCC94 stamped 'TS-007-SPY'.
VFive                          Working, but no sound. MCU type is a NEC V25+. Chip is a PLCC94 stamped 'TS-007-SPY'.
Batsugun                       Working, but sound FX only (missing music) and wrong GFX priorities. MCU type is a NEC V25+. Chip is a PLCC94 stamped 'TS-007-SPY'.
Batsugun Sp'                   Working, but sound FX only (missing music) and wrong GFX priorities. MCU type is a NEC V25+. Chip is a PLCC94 stamped 'TS-007-SPY'.
Snow Bros. 2                   Working.
Mahou Daisakusen               Working.
Shippu Mahou Daisakusen        Working.
Battle Garegga                 Working.
Armed Police Batrider          Working.
Battle Bakraid                 Working.


Notes:
    See Input Port definition header below, for instructions
      on how to enter pause/slow motion modes.
    Code at $20A26 forces territory to Japan in V-Five. Some stuff
      NOP'd at reset vector, and NEC V25+ CPU post test is skipped (bootleg ?)

To Do / Unknowns:
    - Whoopee/Teki Paki sometimes tests bit 5 of the territory port
        just after testing for vblank. Why ?
    - Whoppee is currently using the sound CPU ROM (Z80) from a differnt
        (pirate ?) version of Pipi and Bibis (Ryouta Kikaku copyright).
        It really has a HD647180 CPU, and its internal ROM needs to be dumped.
    - Fix top character text layer (implement the line position table).
    - Priority problem on 2nd player side of selection screen in FixEight (both original and bootleg)
    - Fixeight bootleg text in sound check mode does not display properly
        with the CPU set to 10MHz (ok at 16MHz). Possible error in video_count_r routine.

    - Need to sort out the video status register. Currently should be enabled
        by defining T2_VIDEO_CONTROL but sprite lag needs to be re-synched.
        VIDEO_UPDATE should probably be AFTER Vblank.
        Where did the magical '262' IRQ/Sec for the 68K come from? Probably
        should be 256. CPU interleave on BBAKRAID is 262 aswell - why?
    - Batrider IRQ4 being activated at EOF is rubish. It's sound related -
        maybe acknowledgement from the Z80 when its NMI has completed (port 46)

*****************************************************************************/


#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "cpu/z180/z180.h"
#include "machine/eeprom.h"
#include "machine/nmk112.h"
#include "sound/2151intf.h"
#include "sound/3812intf.h"
#include "sound/okim6295.h"
#include "sound/ymz280b.h"
#include "sound/samples.h"
#include "toaplan2.h"


/**************** Machine stuff ******************/
#define HD64x180	0		/* Define if CPU support is available */
#define V25			0

#define CPU_2_NONE		0x00
#define CPU_2_Z80		0x5a
#define CPU_2_HD647180	0xa5
#define CPU_2_V25		0xff

/************ Machine RAM related values ************/
static UINT8 *toaplan2_shared_ram;
static UINT8 *raizing_shared_ram;		/* Shared ram used in Shippumd and Mahoudai */
static UINT16 *toaplan2_shared_ram16;	/* Really 8bit RAM connected to Z180 */
static UINT16 *V25_shared_ram;			/* Really 8bit RAM connected to Z180 */
static UINT16 *battleg_commram16;		/* Comm ram used in Battle Garegga */
static UINT16 *raizing_cpu_comm16;		/* Raizing commands for the Z80 */
static UINT8  raizing_cpu_reply[2];		/* Raizing replies to the 68K */

/************ Video RAM related values ************/
extern UINT16 *toaplan2_txvideoram16;
extern UINT16 *toaplan2_txvideoram16_offs;
extern UINT16 *toaplan2_txscrollram16;
extern UINT16 *toaplan2_tx_gfxram16;
size_t toaplan2_tx_vram_size;
size_t toaplan2_tx_offs_vram_size;
size_t toaplan2_tx_scroll_vram_size;
size_t batrider_paletteram16_size;

/********** Status related values **********/
int toaplan2_sub_cpu = 0;
static int mcu_data = 0;
static int video_status;
static int prev_scanline;
static INT8 old_p1_paddle_h;			/* For Ghox */
static INT8 old_p2_paddle_h;
static int current_bank = 2;			/* Z80 bank used in Battle Garegga and Batrider */
static int raizing_Z80_busreq;
static int bbakraid_unlimited_ver;

static int current_scanline = 0;
static int vblank_irq;

/**************** Video stuff ******************/
WRITE16_HANDLER( toaplan2_0_voffs_w );
WRITE16_HANDLER( toaplan2_1_voffs_w );

READ16_HANDLER ( toaplan2_0_videoram16_r );
READ16_HANDLER ( toaplan2_1_videoram16_r );
WRITE16_HANDLER( toaplan2_0_videoram16_w );
WRITE16_HANDLER( toaplan2_1_videoram16_w );

READ16_HANDLER ( toaplan2_txvideoram16_r );
WRITE16_HANDLER( toaplan2_txvideoram16_w );
READ16_HANDLER ( toaplan2_txvideoram16_offs_r );
WRITE16_HANDLER( toaplan2_txvideoram16_offs_w );
READ16_HANDLER ( toaplan2_txscrollram16_r );
WRITE16_HANDLER( toaplan2_txscrollram16_w );
READ16_HANDLER ( toaplan2_tx_gfxram16_r );
WRITE16_HANDLER( toaplan2_tx_gfxram16_w );
READ16_HANDLER ( raizing_tx_gfxram16_r );
WRITE16_HANDLER( raizing_tx_gfxram16_w );

WRITE16_HANDLER( toaplan2_0_scroll_reg_select_w );
WRITE16_HANDLER( toaplan2_1_scroll_reg_select_w );
WRITE16_HANDLER( toaplan2_0_scroll_reg_data_w );
WRITE16_HANDLER( toaplan2_1_scroll_reg_data_w );

WRITE16_HANDLER( batrider_objectbank_w );
WRITE16_HANDLER( batrider_textdata_decode );

VIDEO_EOF( toaplan2_0 );
VIDEO_EOF( toaplan2_1 );
VIDEO_EOF( batrider_0 );
VIDEO_START( toaplan2_0 );
VIDEO_START( toaplan2_1 );
VIDEO_START( truxton2_0 );
VIDEO_START( battleg_0 );
VIDEO_START( batrider_0 );
VIDEO_UPDATE( toaplan2_0 );
VIDEO_UPDATE( toaplan2_1 );
VIDEO_UPDATE( truxton2_0 );
VIDEO_UPDATE( dogyuun_1 );
VIDEO_UPDATE( batsugun_1 );
VIDEO_UPDATE( batrider_0 );
VIDEO_UPDATE( mahoudai_0 );


/********* Video wrappers for PIPIBIBI *********/
READ16_HANDLER ( pipibibi_videoram16_r );
WRITE16_HANDLER( pipibibi_videoram16_w );
READ16_HANDLER ( pipibibi_spriteram16_r );
WRITE16_HANDLER( pipibibi_spriteram16_w );
WRITE16_HANDLER( pipibibi_scroll_w );


/***************** Sound stuff *****************/
void dogyuun_okisnd_w(int data);
void kbash_okisnd_w(int data);
void fixeight_okisnd_w(int data);
void batsugun_okisnd_w(int data);

UINT8 m_cmdavailable;

READ8_HANDLER(tekipaki_soundlatch_r);
READ8_HANDLER(tekipaki_cmdavailable_r);

int fadeout_ready = 0;
int fadeout_stop = 0;
int counter1 = 0;
float sample_vol1 = 0;

int playing1 = 0xff;
int playing2 = 0xff;
int playing3 = 0x00;
int playing4 = 0x00;
int playing5 = 0x00;
int play_bat1 = 0x00;

int thunder1 = 0;
int thunder2 = 0;
int thunder3 = 0;
int thunder4 = 0;
int thunder5 = 0;
int thunder6 = 0;

/***************************************************************************
  Initialisation handlers
***************************************************************************/


static MACHINE_RESET( toaplan2 )		/* machine_reset_toaplan2(); */
{
	mcu_data = 0x00;
}

static MACHINE_RESET( ghox )
{
	old_p1_paddle_h = 0;
	old_p2_paddle_h = 0;

	machine_reset_toaplan2();
}

static MACHINE_RESET( dogyuun )
{
	mcu_data = 0xffaa;
}

static MACHINE_RESET( vfive )
{
	mcu_data = 0xffaa;
	
	sample_stop(0);sample_stop(1);sample_stop(2);sample_stop(3);
	sample_stop(4);sample_stop(5);sample_stop(6);sample_stop(7);
	
	sample_set_volume (0, 1.00);
	
	sample_set_volume (1, 0.80);
	sample_set_volume (2, 0.80);
	sample_set_volume (3, 0.50);
	sample_set_volume (4, 0.70);
	sample_set_volume (5, 0.70);
	sample_set_volume (6, 0.70);
	
	sample_set_volume (7, 1.00);
	
	fadeout_stop  = 0;
	fadeout_ready = 0;
	counter1      = 0;
	sample_vol1   = 1.00;
	playing1 = 0xff;
	playing2 = 0xff;
	playing3 = 0x00;
	playing4 = 0x00;
}

static MACHINE_RESET( batrider )
{
	current_bank = 2;

	machine_reset_toaplan2();
}

static DRIVER_INIT( T2_Z80 )		/* init_t2_Z80(); */
{
	toaplan2_sub_cpu = CPU_2_Z80;
}

static DRIVER_INIT( T2_Z180 )
{
	toaplan2_sub_cpu = CPU_2_HD647180;
}

static DRIVER_INIT( T2_V25 )
{
	toaplan2_sub_cpu = CPU_2_V25;
}

static DRIVER_INIT( T2_noZ80 )
{
	toaplan2_sub_cpu = CPU_2_NONE;
}

static DRIVER_INIT( fixeight )
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f002, 0x28fbff, 0, 0, MRA16_RAM );
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f002, 0x28fbff, 0, 0, MWA16_RAM );

	toaplan2_sub_cpu = CPU_2_V25;
}

static DRIVER_INIT( fixeighb )
{
	UINT16 *bgdata = (UINT16 *)memory_region(REGION_CPU1);
	memory_set_bankptr(1, &bgdata[0x40000]); /* $80000 - $fffff */

	toaplan2_sub_cpu = CPU_2_NONE;
}

static DRIVER_INIT( pipibibi )
{
	int A;
	int oldword, newword;

	UINT16 *pipibibi_68k_rom = (UINT16 *)(memory_region(REGION_CPU1));

	/* unscramble the 68K ROM data. */

	for (A = 0; A < (0x040000/2); A+=4)
	{
		newword = 0;
		oldword = pipibibi_68k_rom[A];
		newword |= ((oldword & 0x0001) << 9);
		newword |= ((oldword & 0x0002) << 14);
		newword |= ((oldword & 0x0004) << 8);
		newword |= ((oldword & 0x0018) << 1);
		newword |= ((oldword & 0x0020) << 9);
		newword |= ((oldword & 0x0040) << 7);
		newword |= ((oldword & 0x0080) << 5);
		newword |= ((oldword & 0x0100) << 3);
		newword |= ((oldword & 0x0200) >> 1);
		newword |= ((oldword & 0x0400) >> 8);
		newword |= ((oldword & 0x0800) >> 10);
		newword |= ((oldword & 0x1000) >> 12);
		newword |= ((oldword & 0x6000) >> 7);
		newword |= ((oldword & 0x8000) >> 12);
		pipibibi_68k_rom[A] = newword;

		newword = 0;
		oldword = pipibibi_68k_rom[A+1];
		newword |= ((oldword & 0x0001) << 8);
		newword |= ((oldword & 0x0002) << 12);
		newword |= ((oldword & 0x0004) << 5);
		newword |= ((oldword & 0x0008) << 11);
		newword |= ((oldword & 0x0010) << 2);
		newword |= ((oldword & 0x0020) << 10);
		newword |= ((oldword & 0x0040) >> 1);
		newword |= ((oldword & 0x0080) >> 7);
		newword |= ((oldword & 0x0100) >> 4);
		newword |= ((oldword & 0x0200) << 0);
		newword |= ((oldword & 0x0400) >> 7);
		newword |= ((oldword & 0x0800) >> 1);
		newword |= ((oldword & 0x1000) >> 10);
		newword |= ((oldword & 0x2000) >> 2);
		newword |= ((oldword & 0x4000) >> 13);
		newword |= ((oldword & 0x8000) >> 3);
		pipibibi_68k_rom[A+1] = newword;

		newword = 0;
		oldword = pipibibi_68k_rom[A+2];
		newword |= ((oldword & 0x000f) << 4);
		newword |= ((oldword & 0x00f0) >> 4);
		newword |= ((oldword & 0x0100) << 3);
		newword |= ((oldword & 0x0200) << 1);
		newword |= ((oldword & 0x0400) >> 1);
		newword |= ((oldword & 0x0800) >> 3);
		newword |= ((oldword & 0x1000) << 3);
		newword |= ((oldword & 0x2000) << 1);
		newword |= ((oldword & 0x4000) >> 1);
		newword |= ((oldword & 0x8000) >> 3);
		pipibibi_68k_rom[A+2] = newword;

		newword = 0;
		oldword = pipibibi_68k_rom[A+3];
		newword |= ((oldword & 0x000f) << 4);
		newword |= ((oldword & 0x00f0) >> 4);
		newword |= ((oldword & 0x0100) << 7);
		newword |= ((oldword & 0x0200) << 5);
		newword |= ((oldword & 0x0400) << 3);
		newword |= ((oldword & 0x0800) << 1);
		newword |= ((oldword & 0x1000) >> 1);
		newword |= ((oldword & 0x2000) >> 3);
		newword |= ((oldword & 0x4000) >> 5);
		newword |= ((oldword & 0x8000) >> 7);
		pipibibi_68k_rom[A+3] = newword;
	}

	toaplan2_sub_cpu = CPU_2_Z80;
}

static DRIVER_INIT( battleg )
{
	UINT8 *Z80 = (UINT8 *)memory_region(REGION_CPU2);

	/* Set Z80 bank switch */
	memory_set_bankptr(1, &Z80[0x10000]);		/* Default bank is 2 */

	toaplan2_sub_cpu = CPU_2_Z80;
}

static DRIVER_INIT( bbakraid )
{
	bbakraid_unlimited_ver = 0;
	toaplan2_sub_cpu = CPU_2_Z80;
}

static DRIVER_INIT( bbakradu )
{
	bbakraid_unlimited_ver = 1;
	toaplan2_sub_cpu = CPU_2_Z80;
}


/***************************************************************************
  Toaplan games
***************************************************************************/

#define T2_VIDEO_CONTROL 0		/* Need to adjust the sprite lag.. */

READ16_HANDLER( toaplan2_inputport_0_word_r )
{
#if T2_VIDEO_CONTROL
	return cpu_getvblank();
#else
	int retval = vblank_irq;
	return retval;
#endif
}

static void toaplan2_irq(int irq_line)
{
#if T2_VIDEO_CONTROL
	int vpos = cpu_getscanline();
	if (vpos == 240) cpunum_set_input_line(0, irq_line, HOLD_LINE);
	vblank_irq = 0; //Remove
//  logerror("IRQ: scanline=%04x iloop=%04x beampos=%04x\n",vpos,cpu_getiloops(),cpu_gethorzbeampos());
#else
	if (cpu_getiloops() == 0) current_scanline = 255;

	if (current_scanline == 245)
	{
		cpunum_set_input_line(0, irq_line, HOLD_LINE);
		vblank_irq = 1;
	}

	current_scanline++;
	if (current_scanline > 261)
	{
		current_scanline = 0;
		vblank_irq = 0;
	}
#endif
}

static INTERRUPT_GEN( toaplan2_vblank_irq2 ) { toaplan2_irq(2); }
static INTERRUPT_GEN( toaplan2_vblank_irq3 ) { toaplan2_irq(3); }
static INTERRUPT_GEN( toaplan2_vblank_irq4 ) { toaplan2_irq(4); }

static READ16_HANDLER( video_count_r )
{
	/* +---------+---------+--------+---------------------------+ */
	/* | /H-Sync | /V-Sync | /Blank |       Scanline Count      | */
	/* | Bit 15  | Bit 14  | Bit 8  |  Bit 7-0 (count from #EF) | */
	/* +---------+---------+--------+---------------------------+ */
	/*************** Control Signals are active low ***************/

#if T2_VIDEO_CONTROL
	int hpos = cpu_gethorzbeampos();
	int vpos = cpu_getscanline();
	video_status = 0xff00;						/* Set signals inactive */

	if ((hpos > 325) && (hpos < 380))
		video_status &= ~0x8000;
	if ((vpos >= 242) && (vpos <= 245))
		video_status &= ~0x4000;
	if (cpu_getvblank())
		video_status &= ~0x0100;
	if (vpos < 256)
		video_status |= (vpos & 0xff);
	else
		video_status |= 0xff;

	current_scanline = prev_scanline = vpos; //Remove
	logerror("VC: scanline=%04x iloop=%04x beampos=%04x VBL=%04x\n",vpos,cpu_getiloops(),hpos,cpu_getvblank());
#else
//  logerror("Was VS=%04x  Vbl=%02x  VS=%04x - ",video_status,vblank_irq,prev_scanline );

	video_status = 0xff00;						/* Set signals inactive */
	if ((current_scanline & 0x100) == 0) {
		video_status |= (current_scanline & 0xff);	/* Scanline */
	}
	else {
		video_status |= 0xff;
	}
	if (vblank_irq) {
		video_status &= ~0x0100;
	}
	if (prev_scanline != current_scanline) {
		video_status &= ~0x8000;				/* Activate H-Sync Clk */
	}
	if ((current_scanline >= 247) && (current_scanline <= 250)) {
		video_status &= ~0x4000;				/* Activate V-Sync Clk */
	}
	prev_scanline = current_scanline;

//  logerror("Now VC=%04x  Vbl=%02x  VS=%04x  HS=%04x\n",video_status,vblank_irq,cpu_getscanline(),cpu_gethorzbeampos() );
#endif

	return video_status;
}

static WRITE8_HANDLER( toaplan2_coin_w )
{
	/* +----------------+------ Bits 7-5 not used ------+--------------+ */
	/* | Coin Lockout 2 | Coin Lockout 1 | Coin Count 2 | Coin Count 1 | */
	/* |     Bit 3      |     Bit 2      |     Bit 1    |     Bit 0    | */

	if (data & 0x0f)
	{
		coin_lockout_w( 0, ((data & 4) ? 0 : 1) );
		coin_lockout_w( 1, ((data & 8) ? 0 : 1) );
		coin_counter_w( 0, (data & 1) );
		coin_counter_w( 1, (data & 2) );
	}
	else
	{
		coin_lockout_global_w(1); /* Lock all coin slots */
	}
	if (data & 0xe0)
	{
		logerror("Writing unknown upper bits (%02x) to coin control\n",data);
	}
}
static WRITE16_HANDLER( toaplan2_coin_word_w )
{
	if (ACCESSING_LSB)
	{
		toaplan2_coin_w(offset, data & 0xff);
		if (toaplan2_sub_cpu == CPU_2_Z80)
		{
			if (Machine->drv->sound[1].sound_type == SOUND_OKIM6295)
			{
				OKIM6295_set_bank_base(0, (((data & 0x10) >> 4) * 0x40000));
			}
		}
	}
	if (ACCESSING_MSB && (data & 0xff00) )
	{
		logerror("Writing unknown upper MSB command (%04x) to coin control\n",data & 0xff00);
	}
}

static READ16_HANDLER( toaplan2_shared_r )
{
	return toaplan2_shared_ram[offset] & 0xff;
}

static WRITE16_HANDLER( toaplan2_shared_w )
{
	if (ACCESSING_LSB)
	{
		toaplan2_shared_ram[offset] = data & 0xff;
	}
}

static WRITE16_HANDLER( toaplan2_hd647180_cpu_w )
{
	if (ACCESSING_LSB)
	{
		mcu_data = data & 0xff;
        m_cmdavailable = 1;
	}
}

static READ16_HANDLER( c2map_port_6_r )
{
	/* For Teki Paki hardware */
	/* bit 4 high signifies secondary CPU is ready */
	/* bit 5 is tested low before V-Blank bit ??? */
    return (((m_cmdavailable) ? 0x00 : 0x10) | (input_port_6_r(0) & 0x0f)); /* dink */

}

static READ16_HANDLER( pipibibi_z80_status_r )
{
	return toaplan2_shared_ram[0] & 0xff;
}

static WRITE16_HANDLER( pipibibi_z80_task_w )
{
	if (ACCESSING_LSB)
	{
		toaplan2_shared_ram[0] = data & 0xff;
	}
}

static READ16_HANDLER( ghox_p1_h_analog_r )
{
	INT8 value, new_value;

	new_value = input_port_7_r(0);
	if (new_value == old_p1_paddle_h) return 0;
	value = new_value - old_p1_paddle_h;
	old_p1_paddle_h = new_value;
	return value;
}

static READ16_HANDLER( ghox_p2_h_analog_r )
{
	INT8 value, new_value;

	new_value = input_port_8_r(0);
	if (new_value == old_p2_paddle_h) return 0;
	value = new_value - old_p2_paddle_h;
	old_p2_paddle_h = new_value;
	return value;
}


static READ16_HANDLER( shared_ram_r )
{
/*  Other games using a NEC V25+ secondary CPU, have shared memory between
    the 68000 and the V25+ CPU. The 68000 reads the status of the V25+
    via a location of the shared memory.
*/
	return toaplan2_shared_ram16[offset] & 0xff;
}

static WRITE16_HANDLER( shared_ram_w )
{
	if (ACCESSING_LSB)
	{
		data &= 0xff;
		switch (offset * 2)
		{
			case 0x6e8:
			case 0x9e8:
			case 0x9f0:
			case 0xcf0:
			case 0xcf8:
			case 0xff8: toaplan2_shared_ram16[offset + 1] = data; /* Dogyuun */
						toaplan2_shared_ram16[offset + 2] = data; /* FixEight */
						logerror("PC:%08x Writing (%04x) to shared RAM at %04x\n",activecpu_get_previouspc(),data,(offset*2));
						if (data == 0x81) data = 0x0001;
						break;
			default:	break;
		}
		toaplan2_shared_ram16[offset] = data;
	}
}

static READ16_HANDLER( toaplan2_snd_cpu_r )
{
/*** Status port includes NEC V25+ CPU POST codes. ************
 *** This is actually a part of the 68000/V25+ Shared RAM */

	int response = 0xffff;

	/* Provide successful POST responses */
	if (mcu_data == 0xffaa)						/* Dogyuun */
	{
		response = 0xffaa;
		mcu_data = 0xffff;
	}

	logerror("PC:%06x reading status %08x from the NEC V25+ secondary CPU port\n",activecpu_get_previouspc(),response);
	return response;
}

WRITE16_HANDLER( dogyuun_snd_cpu_w )
{	
	if (data == 0x00)
	{
		sample_stop (0x02);sample_stop (0x03);sample_stop (0x04);sample_stop (0x05);sample_stop (0x06);
		sample_stop (0x07);sample_stop (0x08);sample_stop (0x09);sample_stop (0x0a);sample_stop (0x0b);
		sample_stop (0x0c);sample_stop (0x0d);
	}

	if (data == 0x07)
		playing4++;
	
	if (data >= 0x01 && data <= 0x06 && playing4 == 0)
		sample_start (0x00, data , 1);
	
	if (data >= 0x01 && data <= 0x06 && playing4 != 0)
		playing4 = 0;

	if (data >= 0x08 && data <= 0x0c)
		sample_start (0x01, data , 0);
	
	if (data >= 0x0d && data <= 0x10)
		sample_start (0x02, data , 0);

	if (data >= 0x11 && data <= 0x16)
		sample_start (0x03, data , 0);

	if (data >= 0x17 && data <= 0x1a)
		sample_start (0x04, data , 0);

	if (data == 0x1b || data == 0x1d || data == 0x1f)
	{
		sample_start (0x05, data , 0);
		thunder1 = 0;
	}

	if (data == 0x1c || data == 0x1e || data == 0x20)
	{
		sample_start (0x06, data , 0);
		thunder4 = 0;
	}

	if (data == 0x21)
	{
		sample_start (0x05, data , 1);
		thunder1 = 1;
	}

	if (data == 0x22)
	{
		sample_start (0x06, data , 1);
		thunder4 = 1;
	}

	if (data == 0x23)
		sample_start (0x07, data , 0);

	if (data >= 0x24 && data <= 0x27)
		sample_start (0x08, data , 0);

	if (data >= 0x28 && data <= 0x32)
		sample_start (0x09, data , 0);

	if (data >= 0x33 && data <= 0x35)
		sample_start (0x0a, data , 0);

	if (data == 0x36)
	{
		sample_stop (0); sample_stop (1); sample_stop (2); sample_stop (3); sample_stop (4);
		sample_start (0x0b, data , 0);
		playing4 = 0;
		playing5 = 0;
	}
	if (data == 0x37)
		sample_start (0x0c, data , 0);

	if (data == 0x38)
	{
		sample_stop (0); sample_stop (1); sample_stop (2); sample_stop (3); sample_stop (4);
		sample_start (0x0d, data , 0);
		playing4 = 0;
		playing5 = 0;
	}

	if (data == 0x39)
		sample_start (0x01, data , 0);

	if (data == 0x3a)
		sample_start (0x02, data , 0);

	if (data == 0x3b)
		sample_start (0x02, data , 0);

	if (data == 0x3c)
		sample_start (0x03, data , 0);

	if (data == 0x3d)
		sample_start (0x02, data , 0);

	if (data == 0x3e)
		sample_start (0x04, data , 0);

	if (data == 0x3f)
	{
		sample_stop (0); sample_stop (1); sample_stop (2); sample_stop (3); sample_stop (4);
		sample_start (0x00, data , 0);
		playing4 = 0;
	}

	if (data >= 0x41 && data <= 0x43)
	{
		sample_stop (0x00);sample_stop (0x01);sample_stop (0x02);sample_stop (0x03);sample_stop (0x04);
		sample_stop (0x05);sample_stop (0x06);sample_stop (0x07);sample_stop (0x08);sample_stop (0x09);
		sample_stop (0x0a);sample_stop (0x0b);sample_stop (0x0c);sample_stop (0x0d);
		sample_start (0x00, data , 0);
		playing4 = 0;
	}
	
	if (data == 0x44 && playing5 != 1)
	{
		sample_start (0x00, data , 0);
		playing5++;
	}

	if (data == 0x45)
	{
		sample_start (0x00, data , 0);
		playing4 = 0;
	}

	if (data == 0x46)
		sample_start (0x0f, data , 0);

	if (ACCESSING_LSB)
	{
		mcu_data = data;
		/* dogyuun_okisnd_w(data); */
	}
	
	/* logerror("PC:%06x Writing command (%04x) to the NEC V25+ secondary CPU port\n",activecpu_get_previouspc(),mcu_data); */
}

static READ16_HANDLER( kbash_snd_cpu_r )
{
/*  Knuckle Bash's  68000 reads secondary CPU status via an I/O port.
    If a value of 2 is read, then secondary CPU is busy.
    Secondary CPU must report 0xff when no longer busy, to signify that it
    has passed POST.
*/
	return 0xff;
}

static WRITE16_HANDLER( kbash_snd_cpu_w )
{
	if (ACCESSING_LSB)
	{
		kbash_okisnd_w(data);
	}
	logerror("PC:%06x Writing Sound command (%04x) to the NEC V25+ secondary CPU\n",activecpu_get_previouspc(),data);
}

static READ16_HANDLER( fixeight_sec_cpu_r )
{
/*** Status port includes NEC V25+ CPU POST codes. ************
 *** This is actually a part of the 68000/V25+ Shared RAM */

	int response = 0xffff;

	if ((mcu_data & 0xffff) == 0x0faa)
	{
		mcu_data = 0xffff;
		response = 0xffaa;
	}
	if ((mcu_data & 0xffff) == 0xff00)
	{
		mcu_data = 0x0faa;
		response = 0xffaa;		/* Second CPU passed POST response */
	}
	if ((mcu_data & 0xff00) == 0x0000)
	{
		response = mcu_data;	/* Return the shared RAM data during POST */
	}
	logerror("PC:%06x reading status %08x from the NEC V25+ secondary CPU port\n",activecpu_get_previouspc(),response);
	return response;
}

static WRITE16_HANDLER( fixeight_sec_cpu_w )
{
	if (ACCESSING_LSB)
	{
		if (mcu_data & 0xff00)
		{
			mcu_data = (mcu_data & 0xff00) | (data & 0xff);
			fixeight_okisnd_w(data);
		}
		else if (mcu_data == 0xff00)
		{
#if 0 		/* check the 37B6 code */
			/* copy nvram data to shared ram after post is complete */
			fixeight_sharedram[0] = fixeight_nvram[0];	/* Dip Switch A */
			fixeight_sharedram[1] = fixeight_nvram[1];	/* Dip Switch B */
			fixeight_sharedram[2] = fixeight_nvram[2];	/* Territory */
#endif
			/* Hack Alert ! Fixeight does not have any DSW. The main CPU has a */
			/* game keeping service mode. It writes/reads the settings to/from */
			/* these shared RAM locations. The secondary CPU reads/writes them */
			/* from/to nvram to store the settings (a 93C45 EEPROM) */
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f002, 0x28f003, 0, 0, MRA16_RAM);
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f004, 0x28f005, 0, 0, input_port_5_word_r);	/* Dip Switch A - Wrong !!! */
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f006, 0x28f007, 0, 0, input_port_6_word_r);	/* Dip Switch B - Wrong !!! */
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f008, 0x28f009, 0, 0, input_port_7_word_r);	/* Territory Jumper block - Wrong !!! */
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f00a, 0x28fbff, 0, 0, MRA16_RAM);
			memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f002, 0x28f003, 0, 0, MWA16_RAM);
			memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f004, 0x28f009, 0, 0, MWA16_NOP);
			memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x28f00a, 0x28fbff, 0, 0, MWA16_RAM);

			mcu_data = data;
		}
		else
		{
			mcu_data = data;
		}
	}
	logerror("PC:%06x Writing command (%04x) to the NEC V25+ secondary CPU port\n",activecpu_get_previouspc(),mcu_data);
}

WRITE16_HANDLER( vfive_snd_cpu_w )
{
	if (data == 0x00)
	{
		sample_stop (0);
		fadeout_stop = 1;
		playing1 = 0xff;
		playing2 = 0xff;
		playing3 = 0x00;
		playing4 = 0x00;
	}

	if (data == 0x07)
	{
		fadeout_ready = 1;
	}

	if (data == 0x43 || data == 0x44 || data == 0x41)
	{
		if (playing4 == 1)
		{
		fadeout_ready = 1;
		playing2 = 0xff;
		playing4 = 0;
		}
	}

	if (data >= 0x01 && data <= 0x06 && fadeout_ready == 0 && playing1 == 0xff)
	{
		sample_start (0, data, 1);
		playing1 = data;
		playing3 = 0x00;
	}

	if (data >= 0x01 && data <= 0x06 && fadeout_ready == 1)
	{
		playing2 = 0x00;
	}

	if (data == 0x09)
		sample_start (7, data, 0);
	
	if (data >= 0x0a && data <= 0x12)
		sample_start (1, data, 0);
	
	if (data >= 0x13 && data <= 0x17)
		sample_start (2, data, 0);

	if (data >= 0x18 && data <= 0x1e)
		sample_start (3, data, 0);

	if (data >= 0x1f && data <= 0x29)
		sample_start (4, data, 0);

	if (data >= 0x2a && data <= 0x34)
		sample_start (5, data, 0);

	if (data >= 0x35 && data <= 0x3c)
		sample_start (6, data, 0);

	if (data == 0x3d)
	{
		sample_start (0, data, 0);
		playing1 = 0xff;
	}
	
	if (data == 0x3e)
	{
		sample_start (0, data, 0);
		playing1 = 0xff;
	}

	if (data == 0x3f)
		sample_start (1, data, 0);
	if (data == 0x40)
		sample_start (1, data, 0);
	if (data == 0x41 && playing3 == 0x00)
	{
		sample_start (0, data, 0);
		playing1 = 0xff;
		playing3++;
		playing4 = 1;
	}
	if (data == 0x42)
		sample_start (1, data, 0);
	
	if (data == 0x43 && playing3 == 0x00)
	{
		sample_start (0, data, 0);
		playing1 = 0xff;
		playing3++;
		playing4 = 1;
	}
	if (data == 0x44 && playing3 == 0x00)
	{
		sample_start (0, data, 0);
		playing1 = 0xff;
		playing3++;
		playing4 = 1;
	}

	if (data == 0x45)
		sample_start (7, data, 0);

	if (ACCESSING_LSB)
	{
		mcu_data = data;
	}
	/* logerror("PC:%06x Writing command (%04x) to the NEC V25+ secondary CPU port\n",activecpu_get_previouspc(),mcu_data); */
}

WRITE16_HANDLER( batsugun_snd_cpu_w )
{	
	if (data == 0x00)
	{
		play_bat1 = 0;
	}

	if (data >= 0x01 && data <= 0x0a)
	{
		sample_start (0, data , 1);
		play_bat1 = 0;
	}

	if (data == 0x0b && play_bat1 == 0)
	{
		sample_start (0, data , 1);
		play_bat1++;
	}

	if (data >= 0x0c && data <= 0x10)
		sample_start (0, data , 0);

	if (data >= 0x11 && data <= 0x12)
		sample_start (1, data , 0);

	if (data == 0x13)
		sample_start (2, data , 0);

	if (data == 0x14)
		sample_start (3, data , 0);

	if (data == 0x15)
		sample_start (4, data , 0);

	if (data == 0x16)
		sample_start (5, data , 0);

	if (data == 0x17)
		sample_start (6, data , 0);

	if (data == 0x18)
		sample_start (7, data , 0);

	if (data == 0x19)
		sample_start (8, data , 0);

	if (data == 0x1a)
		sample_start (9, data , 0);

	if (data == 0x1b)
		sample_start (10, data , 0);

	if (data == 0x1d)
		sample_start (11, data , 0);

	if (data == 0x1e)
		sample_start (12, data , 0);

	if (data == 0x1f)
		sample_start (13, data , 0);

	if (data == 0x20)
		sample_start (14, data , 0);

	if (data == 0x22)
		sample_start (15, data , 0);

	if (data >= 0x23 && data <= 0x28)
		sample_start (16, data , 0);

	if (data == 0x29)
		sample_start (17, data , 0);

	if (data == 0x2a)
		sample_start (18, data , 0);

	if (data == 0x2d)
		sample_start (19, data , 0);

	if (data == 0x2e)
		sample_start (20, data , 0);

	if (data == 0x2f)
		sample_start (21, data , 0);

	if (data == 0x30)
		sample_start (22, data , 0);

	if (data == 0x32)
		sample_start (23, data , 0);

	if (data == 0x33)
		sample_start (24, data , 0);

	if (data == 0x34)
		sample_start (25, data , 0);

	if (data == 0x36)
		sample_start (26, data , 0);

	if (data == 0x37)
		sample_start (27, data , 0);

	if (data == 0x3a)
		sample_start (28, data , 0);

	if (ACCESSING_LSB)
	{
		mcu_data = data;
		/* batsugun_okisnd_w(data); */
	}
	/* logerror("PC:%06x Writing command (%04x) to the NEC V25+ secondary CPU port %02x\n",activecpu_get_previouspc(),mcu_data,(offset*2)); */
}

static READ16_HANDLER( V25_sharedram_r )
{
	return V25_shared_ram[offset] & 0xff;
}

static WRITE16_HANDLER( V25_sharedram_w )
{
	if (ACCESSING_LSB)
	{
		V25_shared_ram[offset] = data & 0xff;
	}
}

static WRITE16_HANDLER( oki_bankswitch_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
	}
}

static WRITE16_HANDLER( fixeighb_oki_bankswitch_w )
{
	if (ACCESSING_LSB)
	{
		data &= 7;
		if (data <= 4)
		{
			UINT8 *fixeighb_oki = memory_region(REGION_SOUND1);
			memcpy(&fixeighb_oki[0x30000], &fixeighb_oki[(data * 0x10000) + 0x40000], 0x10000);
		}
	}
}

/***************************************************************************
  Raizing games
***************************************************************************/

static READ16_HANDLER( raizing_shared_ram_r )
{
	return raizing_shared_ram[offset] & 0xff;
}

static WRITE16_HANDLER( raizing_shared_ram_w )
{
	if (ACCESSING_LSB)
	{
		raizing_shared_ram[offset] = data & 0xff;
	}
}

static READ16_HANDLER( battleg_commram_r )
{
	return battleg_commram16[offset];
}

static WRITE16_HANDLER( battleg_commram_w )
{
	COMBINE_DATA(&battleg_commram16[offset]);
	cpunum_set_input_line(1, 0, HOLD_LINE);
	if (offset == 0) cpu_yield();	/* Command issued so switch control */
}

static READ8_HANDLER( battleg_commram_check_r0 )
{
	UINT8 *battleg_common_RAM = (UINT8 *)battleg_commram16;

	return battleg_common_RAM[BYTE_XOR_BE(offset * 2 + 1)];
}

static WRITE8_HANDLER( battleg_commram_check_w0 )
{
	UINT8 *battleg_common_RAM = (UINT8 *)battleg_commram16;

	battleg_common_RAM[BYTE_XOR_BE(0)] = data;
	cpu_yield();					/* Command issued so switch control */
}

static READ16_HANDLER( battleg_z80check_r )
{
	return raizing_shared_ram[offset + 0x10] & 0xff;
}

static WRITE8_HANDLER( battleg_bankswitch_w )
{
	UINT8 *RAM = (UINT8 *)memory_region(REGION_CPU2);
	int bankaddress;
	int bank;

	bank = (data & 0x0f) - 10;

	if (bank != current_bank)
	{
		current_bank = bank;
		bankaddress = 0x10000 + 0x4000 * current_bank;
		memory_set_bankptr(1, &RAM[bankaddress]);
	}
}

// what chip do battleg and batrider actually use for soundrom banking?
// it works the same way as the NMK112 but the interface is different...
// interesting fact: these two games had the same composer & sfx designer
// (Manabu NAMIKI) as the NMK games which used the NMK112
// (macross2, tdragon2, quizpani...)

static WRITE8_HANDLER( raizing_okim6295_bankselect_0 )
{
	NMK112_okibank_w(0,  data		& 0x0f);	// chip 0 bank 0
	NMK112_okibank_w(1, (data >> 4)	& 0x0f);	// chip 0 bank 1
}

static WRITE8_HANDLER( raizing_okim6295_bankselect_1 )
{
	NMK112_okibank_w(2,  data		& 0x0f);	// chip 0 bank 2
	NMK112_okibank_w(3, (data >> 4)	& 0x0f);	// chip 0 bank 3
}

static WRITE8_HANDLER( raizing_okim6295_bankselect_2 )
{
	NMK112_okibank_w(4,  data		& 0x0f);	// chip 1 bank 0
	NMK112_okibank_w(5, (data >> 4)	& 0x0f);	// chip 1 bank 1
}

static WRITE8_HANDLER( raizing_okim6295_bankselect_3 )
{
	NMK112_okibank_w(6,  data		& 0x0f);	// chip 1 bank 2
	NMK112_okibank_w(7, (data >> 4)	& 0x0f);	// chip 1 bank 3
}

static WRITE8_HANDLER( batrider_bankswitch_w )
{
	UINT8 *RAM = (UINT8 *)memory_region(REGION_CPU2);
	int bankaddress;
	int bank;

	bank = data & 0x0f;

	if (bank != current_bank)
	{
		current_bank = bank;
		logerror("Z80 cpu set bank #%d\n", bank);
		if (bank > 1)
			bankaddress = 0x10000 + 0x4000 * (current_bank - 2);
		else
			bankaddress = 0x4000 * current_bank;
		memory_set_bankptr(1, &RAM[bankaddress]);
	}
}

static READ16_HANDLER( batrider_z80_busack_r )
{
	/* Bit 1 returns the status of BUSAK from the Z80.
       BUSRQ is activated via bit 0x10 on the NVRAM write port.
       These accesses are made when the 68K wants to read the Z80
       ROM code. Failure to return the correct status incurrs a Sound Error.
    */

	return raizing_Z80_busreq;			/* Loop BUSRQ to BUSAK */
}
static WRITE16_HANDLER( batrider_z80_busreq_w )
{
	if (ACCESSING_LSB)
	{
		raizing_Z80_busreq = (data & 0xff);
	}
}

static READ16_HANDLER( raizing_z80rom_r )
{
	UINT8 *Z80_ROM_test = (UINT8 *)memory_region(REGION_CPU2);

	if (offset < 0x8000)
		return Z80_ROM_test[offset] & 0xff;

	return Z80_ROM_test[offset + 0x8000] & 0xff;
}



/*###################### Battle Bakraid ##############################*/

/* EEPROM contents with battle Bakraid Unlimited version features unlocked */
static UINT8 bbakraid_unlimited_nvram[512] = {
	0xc2,0x49,0x00,0x07,0xa1,0x20,0x2a,0x2a,0x2a,0x90,0x90,0x90,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x06,0x1a,0x80,0x2a,0x2a,0x2a,0x94,
	0x94,0x94,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x04,
	0x93,0xe0,0x2a,0x2a,0x2a,0x98,0x98,0x98,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x02,0x00,0x03,0x0d,0x40,0x2a,0x2a,0x2a,0x9c,0x9c,0x9c,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x01,0x86,0xa0,0x2a,0x2a,
	0x2a,0xa0,0xa0,0xa0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
	0x00,0x07,0xa1,0x20,0x2b,0x2b,0x2b,0x90,0x90,0x90,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x06,0x1a,0x80,0x2b,0x2b,0x2b,0x94,0x94,0x94,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x04,0x93,0xe0,
	0x2b,0x2b,0x2b,0x98,0x98,0x98,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x02,0x00,0x03,0x0d,0x40,0x2b,0x2b,0x2b,0x9c,0x9c,0x9c,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x01,0x86,0xa0,0x2b,0x2b,0x2b,0xa0,
	0xa0,0xa0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x07,
	0xa1,0x20,0x23,0x23,0x23,0x90,0x90,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x02,0x00,0x06,0x1a,0x80,0x23,0x23,0x23,0x94,0x94,0x94,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x04,0x93,0xe0,0x23,0x23,
	0x23,0x98,0x98,0x98,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
	0x00,0x03,0x0d,0x40,0x23,0x23,0x23,0x9c,0x9c,0x9c,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x01,0x86,0xa0,0x23,0x23,0x23,0xa0,0xa0,0xa0,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x07,0xa1,0x20,
	0x3f,0x3f,0x3f,0x90,0x90,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x02,0x00,0x06,0x1a,0x80,0x3f,0x3f,0x3f,0x94,0x94,0x94,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x04,0x93,0xe0,0x3f,0x3f,0x3f,0x98,
	0x98,0x98,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x03,
	0x0d,0x40,0x3f,0x3f,0x3f,0x9c,0x9c,0x9c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x02,0x00,0x01,0x86,0xa0,0x3f,0x3f,0x3f,0xa0,0xa0,0xa0,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x04,0xc2,0x49,0xc2,0x49,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0xcf,0x00,0x04,0x00,0x00,0x21,0xd5,
	0x00,0x05,0x06,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc2,0x49
};



struct EEPROM_interface eeprom_interface_93C66 =
{
	/* Pin 6 of the 93C66 is connected to Gnd!
       So it's configured for 512 bytes */

	9,			// address bits
	8,			// data bits
	"*110",		// read         110 aaaaaaaaa
	"*101",		// write        101 aaaaaaaaa dddddddd
	"*111",		// erase        111 aaaaaaaaa
	"*10000xxxxxxx",// lock         100x 00xxxx
	"*10011xxxxxxx",// unlock       100x 11xxxx
//  "*10001xxxx",   // write all    1 00 01xxxx dddddddd
//  "*10010xxxx"    // erase all    1 00 10xxxx
};


static NVRAM_HANDLER( bbakraid )
{
	/* Pin 6 of 93C66 is connected to Gnd! */

	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C66);

		if (file) EEPROM_load(file);
		else
		{
			if (bbakraid_unlimited_ver == 1)
				EEPROM_set_data(bbakraid_unlimited_nvram, sizeof(bbakraid_unlimited_nvram));
		}
	}
}

static READ16_HANDLER( bbakraid_nvram_r )
{
	/* Bit 1 returns the status of BUSAK from the Z80.
       BUSRQ is activated via bit 0x10 on the NVRAM write port.
       These accesses are made when the 68K wants to read the Z80
       ROM code. Failure to return the correct status incurrs a Sound Error.
    */

	int data;
	data  = ((EEPROM_read_bit() & 0x01) << 4);
	data |= ((raizing_Z80_busreq >> 4) & 0x01);	/* Loop BUSRQ to BUSAK */

	return data;
}

static WRITE16_HANDLER( bbakraid_nvram_w )
{
	if (data & ~0x001f)
		logerror("CPU #0 PC:%06X - Unknown EEPROM data being written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_LSB )
	{
		// chip select
		EEPROM_set_cs_line((data & 0x01) ? CLEAR_LINE : ASSERT_LINE );

		// latch the bit
		EEPROM_write_bit( (data & 0x04) >> 2 );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE );
	}
	raizing_Z80_busreq = data & 0x10;	/* see bbakraid_nvram_r above */
}


/****** Battle Bakraid 68K handlers ******/
static READ16_HANDLER ( raizing_sndcomms_r )
{
//  logerror("68K (PC:%06x) reading %04x from $50001%01x\n",activecpu_get_pc(),(raizing_cpu_reply[offset] & 0xff),(offset*2));
	return (raizing_cpu_reply[offset] & 0xff);
}
static WRITE16_HANDLER ( raizing_sndcomms_w )
{
//  logerror("68K (PC:%06x) writing %04x to $50001%01x\n",activecpu_get_pc(),data,((offset*2)+4));
	COMBINE_DATA(&raizing_cpu_comm16[offset]);

	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
	cpu_yield();
}

/****** Battle Bakraid Z80 handlers ******/
static READ8_HANDLER ( raizing_command_r )
{
	UINT8 *raizing_cpu_comm = (UINT8 *)raizing_cpu_comm16;

	logerror("Z80 (PC:%04x) reading %02x from $48\n",activecpu_get_pc(),raizing_cpu_comm[BYTE_XOR_BE(1)]);
	return raizing_cpu_comm[BYTE_XOR_BE(1)];
}
static READ8_HANDLER ( raizing_request_r )
{
	UINT8 *raizing_cpu_comm = (UINT8 *)raizing_cpu_comm16;

	logerror("Z80 (PC:%04x) reading %02x from $4A\n",activecpu_get_pc(),raizing_cpu_comm[BYTE_XOR_BE(3)]);
	return raizing_cpu_comm[BYTE_XOR_BE(3)];
}
static WRITE8_HANDLER ( raizing_command_ack_w )
{
//  logerror("Z80 (PC:%04x) writing %02x to $40\n",activecpu_get_pc(),data);
	raizing_cpu_reply[0] = data;
}
static WRITE8_HANDLER ( raizing_request_ack_w )
{
//  logerror("Z80 (PC:%04x) writing %02x to $42\n",activecpu_get_pc(),data);
	raizing_cpu_reply[1] = data;
}


static WRITE8_HANDLER ( raizing_clear_nmi_w )
{
//  logerror("Clear NMI on the Z80 (Z80 PC:%06x writing %04x)\n",activecpu_get_pc(),data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
	cpu_yield();
}

static WRITE16_HANDLER ( bbakraid_trigger_z80_irq )
{
//  logerror("Triggering IRQ on the Z80 (PC:%06x)\n",activecpu_get_pc());
	cpunum_set_input_line(1, 0, HOLD_LINE);
	cpu_yield();
}

static void bbakraid_irqhandler (int state)
{
	/* Not used ???  Connected to a test pin (TP082) */
	logerror("YMZ280 is generating an interrupt. State=%08x\n",state);
}

static INTERRUPT_GEN( bbakraid_snd_interrupt )
{
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

static const char *dogyuun_sample_names[] =
{
	"*dogyuun",
	"dm.wav","01.wav","02.wav","03.wav","04.wav","05.wav","06.wav","dm.wav",
	"08.wav","09.wav","0a.wav","0b.wav","0c.wav","0d.wav","dm.wav","0f.wav",
	"10.wav","dm.wav","12.wav","13.wav","14.wav","15.wav","16.wav","17.wav",
	"18.wav","19.wav","1a.wav","1b.wav","1c.wav","1d.wav","1e.wav","1f.wav",
	"20.wav","21.wav","22.wav","23.wav","dm.wav","25.wav","26.wav","27.wav",
	"dm.wav","dm.wav","2a.wav","2b.wav","2c.wav","2d.wav","2e.wav","2f.wav",
	"30.wav","31.wav","32.wav","33.wav","34.wav","35.wav","36.wav","37.wav",
	"38.wav","39.wav","3a.wav","3b.wav","3c.wav","3d.wav","3e.wav","3f.wav",
	"dm.wav","41.wav","42.wav","43.wav","44.wav","45.wav","46.wav",0
};

struct Samplesinterface dogyuun_samples_interface =
{
	17,dogyuun_sample_names
};

static const char *vfive_sample_names[] =
{
	"*vfive",
	"00.wav","01.wav","02.wav","03.wav","04.wav","05.wav","06.wav","99.wav",
	"99.wav","09.wav","0a.wav","0b.wav","0c.wav","0d.wav","0e.wav","0f.wav",
	"10.wav","11.wav","12.wav","13.wav","14.wav","15.wav","16.wav","17.wav",
	"18.wav","19.wav","1a.wav","1b.wav","1c.wav","1d.wav","1e.wav","99.wav",
	"20.wav","21.wav","22.wav","23.wav","24.wav","99.wav","99.wav","27.wav",
	"99.wav","29.wav","2a.wav","2b.wav","2c.wav","2d.wav","99.wav","2f.wav",
	"30.wav","31.wav","32.wav","33.wav","99.wav","99.wav","36.wav","37.wav",
	"38.wav","39.wav","99.wav","3b.wav","3c.wav","3d.wav","3e.wav","3f.wav",
	"40.wav","41.wav","42.wav","43.wav","44.wav","45.wav",0
};

struct Samplesinterface vfive_samples_interface =
{
	8,vfive_sample_names
};

static const char *batsugun_sample_names[] =
{
	"*batsugun",
	"dm.wav","01.wav","02.wav","03.wav","04.wav","05.wav","06.wav","07.wav",
	"08.wav","09.wav","0a.wav","0b.wav","0c.wav","0d.wav","0e.wav","0f.wav",
	"10.wav","11.wav","12.wav","13.wav","14.wav","15.wav","16.wav","17.wav",
	"18.wav","19.wav","1a.wav","1b.wav","dm.wav","1d.wav","1e.wav","1f.wav",
	"20.wav","dm.wav","22.wav","23.wav","24.wav","25.wav","26.wav","27.wav",
	"28.wav","29.wav","2a.wav","dm.wav","dm.wav","2d.wav","2e.wav","2f.wav",
	"30.wav","dm.wav","32.wav","33.wav","34.wav","dm.wav","36.wav","37.wav",
	"dm.wav","dm.wav","3a.wav",0
};

struct Samplesinterface batsugun_samples_interface =
{
	30,batsugun_sample_names
};

static ADDRESS_MAP_START( tekipaki_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x020000, 0x03ffff) AM_ROM							/* extra for Whoopee */
	AM_RANGE(0x080000, 0x082fff) AM_RAM
	AM_RANGE(0x0c0000, 0x0c0fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x140000, 0x140001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x140004, 0x140007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x140008, 0x140009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x14000c, 0x14000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x180000, 0x180001) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x180010, 0x180011) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x180020, 0x180021) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x180030, 0x180031) AM_READ(c2map_port_6_r)		/* CPU 2 busy and Territory Jumper block */
	AM_RANGE(0x180040, 0x180041) AM_WRITE(toaplan2_coin_word_w)	/* Coin count/lock */
	AM_RANGE(0x180050, 0x180051) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x180060, 0x180061) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x180070, 0x180071) AM_WRITE(toaplan2_hd647180_cpu_w) /* MCU commands */
ADDRESS_MAP_END

/* Most inputs are now read via the mcu */
static ADDRESS_MAP_START( ghox_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x040000, 0x040001) AM_READ(ghox_p2_h_analog_r)	/* Paddle 2 */
	AM_RANGE(0x080000, 0x083fff) AM_RAM
	AM_RANGE(0x0c0000, 0x0c0fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x100000, 0x100001) AM_READ(ghox_p1_h_analog_r)	/* Paddle 1 */
	AM_RANGE(0x140000, 0x140001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x140004, 0x140007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x140008, 0x140009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x14000c, 0x14000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x180000, 0x180fff) AM_READWRITE(toaplan2_shared_r, toaplan2_shared_w)
	AM_RANGE(0x181000, 0x181001) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x18100c, 0x18100d) AM_READ(port_tag_to_handler16("JMPR"))
ADDRESS_MAP_END


static ADDRESS_MAP_START( dogyuun_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM
	AM_RANGE(0x200010, 0x200011) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x200014, 0x200015) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x200018, 0x200019) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x20001c, 0x20001d) AM_WRITE(toaplan2_coin_word_w)
#if V25
	AM_RANGE(0x21e000, 0x21fbff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)	/* $21f000 status port */
	AM_RANGE(0x21fc00, 0x21ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#else
	AM_RANGE(0x21e000, 0x21efff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)
	AM_RANGE(0x21f000, 0x21f001) AM_READWRITE(toaplan2_snd_cpu_r, dogyuun_snd_cpu_w)	/* V25+ status/command port */
	AM_RANGE(0x21f004, 0x21f005) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x21f006, 0x21f007) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x21f008, 0x21f009) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x21fc00, 0x21ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#endif
	/***** The following locations in 0x30000x are for video controller 1 ******/
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	/***** The following loctions in 0x50000x are for video controller 2 ******/
	AM_RANGE(0x500000, 0x500001) AM_WRITE(toaplan2_1_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x500004, 0x500007) AM_READWRITE(toaplan2_1_videoram16_r, toaplan2_1_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x500008, 0x500009) AM_WRITE(toaplan2_1_scroll_reg_select_w)
	AM_RANGE(0x50000c, 0x50000d) AM_WRITE(toaplan2_1_scroll_reg_data_w)
	AM_RANGE(0x700000, 0x700001) AM_READ(video_count_r)			/* test bit 8 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( dogyuun_z80_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x218000, 0x218fff) AM_READ(toaplan2_shared_r)
	AM_RANGE(0x21c020, 0x21c021) AM_READ(port_tag_to_handler16("IN1"))	/* Player 1 controls */
	AM_RANGE(0x21c024, 0x21c025) AM_READ(port_tag_to_handler16("IN2"))	/* Player 2 controls */
	AM_RANGE(0x21c028, 0x21c029) AM_READ(port_tag_to_handler16("SYS"))	/* Coin/System inputs */
	AM_RANGE(0x21c02c, 0x21c02d) AM_READ(port_tag_to_handler16("DSWA"))	/* Dip Switch A */
	AM_RANGE(0x21c030, 0x21c031) AM_READ(port_tag_to_handler16("DSWB"))	/* Dip Switch B */
	AM_RANGE(0x21f008, 0x21f009) AM_READ(port_tag_to_handler16("JMPR"))	/* Territory Jumper block */ /* no change over the V25 set assumed to be the same */
	/***** The following in 0x30000x are for video controller 1 ******/
	AM_RANGE(0x300004, 0x300007) AM_READ(toaplan2_0_videoram16_r)/* tile layers */
	AM_RANGE(0x30000c, 0x30000d) AM_READ(toaplan2_inputport_0_word_r)	/* VBlank */
	AM_RANGE(0x400000, 0x400fff) AM_READ(paletteram16_word_r)
	/***** The following in 0x50000x are for video controller 2 ******/
	AM_RANGE(0x500004, 0x500007) AM_READ(toaplan2_1_videoram16_r)/* tile layers 2 */
	AM_RANGE(0x700000, 0x700001) AM_READ(video_count_r)			/* test bit 8 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( dogyuun_z80_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x21c01d, 0x21c01d) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x218000, 0x218fff) AM_WRITE(toaplan2_shared_w)
	/***** The following in 0x30000x are for video controller 1 ******/
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_WRITE(toaplan2_0_videoram16_w)/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_WRITE(toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x400000, 0x400fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	/***** The following in 0x50000x are for video controller 2 ******/
	AM_RANGE(0x500000, 0x500001) AM_WRITE(toaplan2_1_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x500004, 0x500007) AM_WRITE(toaplan2_1_videoram16_w)/* Tile/Sprite VideoRAM */
	AM_RANGE(0x500008, 0x500009) AM_WRITE(toaplan2_1_scroll_reg_select_w)
	AM_RANGE(0x50000c, 0x50000d) AM_WRITE(toaplan2_1_scroll_reg_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kbash_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM
	AM_RANGE(0x200000, 0x200001) AM_READWRITE(kbash_snd_cpu_r, kbash_snd_cpu_w)	/* Sound number to play */
	AM_RANGE(0x200002, 0x200003) AM_WRITENOP					/* Control info to V25+ */
	AM_RANGE(0x200004, 0x200005) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x200006, 0x200007) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x200008, 0x200009) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x208010, 0x208011) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x208014, 0x208015) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x208018, 0x208019) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x20801c, 0x20801d) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x700000, 0x700001) AM_READ(video_count_r)			/* test bit 8 */
ADDRESS_MAP_END


static ADDRESS_MAP_START( kbash2_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM
	AM_RANGE(0x104000, 0x10401f) AM_RAM			/* Sound related? */
	AM_RANGE(0x200000, 0x200001) AM_NOP			/* Left over from original code - Sound Number write, Status read */
	AM_RANGE(0x200002, 0x200003) AM_WRITENOP	/* Left over from original code - Reset Sound */
	AM_RANGE(0x200004, 0x200005) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x200008, 0x200009) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x20000c, 0x20000d) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x200010, 0x200011) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x200014, 0x200015) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x200018, 0x200019) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x200020, 0x200021) AM_READWRITE(OKIM6295_status_1_lsb_r, OKIM6295_data_1_lsb_w)
	AM_RANGE(0x200024, 0x200025) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x200028, 0x200029) AM_WRITE(oki_bankswitch_w)
	AM_RANGE(0x20002c, 0x20002d) AM_READ(video_count_r)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
ADDRESS_MAP_END


static ADDRESS_MAP_START( truxton2_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x200000, 0x200001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x200004, 0x200007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)
	AM_RANGE(0x200008, 0x200009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x20000c, 0x20000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x300000, 0x300fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x400000, 0x401fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)
	AM_RANGE(0x402000, 0x4021ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x402200, 0x402fff) AM_RAM
	AM_RANGE(0x403000, 0x4031ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x403200, 0x403fff) AM_RAM
	AM_RANGE(0x500000, 0x50ffff) AM_READWRITE(toaplan2_tx_gfxram16_r, toaplan2_tx_gfxram16_w) AM_BASE(&toaplan2_tx_gfxram16)
	AM_RANGE(0x600000, 0x600001) AM_READ(video_count_r)
	AM_RANGE(0x700000, 0x700001) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x700002, 0x700003) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x700004, 0x700005) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x700006, 0x700007) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x700008, 0x700009) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x70000a, 0x70000b) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x700010, 0x700011) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x700014, 0x700015) AM_WRITE(YM2151_register_port_0_lsb_w)
	AM_RANGE(0x700016, 0x700017) AM_READWRITE(YM2151_status_port_0_lsb_r, YM2151_data_port_0_lsb_w)
	AM_RANGE(0x70001e, 0x70001f) AM_WRITE(toaplan2_coin_word_w)		/* Coin count/lock */
ADDRESS_MAP_END


static ADDRESS_MAP_START( pipibibs_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080000, 0x082fff) AM_RAM
	AM_RANGE(0x0c0000, 0x0c0fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x140000, 0x140001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x140004, 0x140007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)
	AM_RANGE(0x140008, 0x140009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x14000c, 0x14000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x190000, 0x190fff) AM_READWRITE(toaplan2_shared_r, toaplan2_shared_w)
	AM_RANGE(0x19c01c, 0x19c01d) AM_WRITE(toaplan2_coin_word_w)		/* Coin count/lock */
	AM_RANGE(0x19c020, 0x19c021) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x19c024, 0x19c025) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x19c028, 0x19c029) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x19c02c, 0x19c02d) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x19c030, 0x19c031) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x19c034, 0x19c035) AM_READ(port_tag_to_handler16("IN2"))
ADDRESS_MAP_END


static ADDRESS_MAP_START( pipibibi_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080000, 0x082fff) AM_RAM
	AM_RANGE(0x083000, 0x0837ff) AM_READWRITE(pipibibi_spriteram16_r, pipibibi_spriteram16_w)	/* SpriteRAM */
	AM_RANGE(0x083800, 0x087fff) AM_RAM				/* SpriteRAM (unused) */
	AM_RANGE(0x0c0000, 0x0c0fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x120000, 0x120fff) AM_RAM				/* Copy of SpriteRAM ? */
//  AM_RANGE(0x13f000, 0x13f001) AM_WRITENOP        /* ??? */
	AM_RANGE(0x180000, 0x182fff) AM_READWRITE(pipibibi_videoram16_r, pipibibi_videoram16_w)		/* TileRAM */
	AM_RANGE(0x188000, 0x18800f) AM_WRITE(pipibibi_scroll_w)
	AM_RANGE(0x190002, 0x190003) AM_READ(pipibibi_z80_status_r)	/* Z80 ready ? */
	AM_RANGE(0x190010, 0x190011) AM_WRITE(pipibibi_z80_task_w)	/* Z80 task to perform */
	AM_RANGE(0x19c01c, 0x19c01d) AM_WRITE(toaplan2_coin_word_w)	/* Coin count/lock */
	AM_RANGE(0x19c020, 0x19c021) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x19c024, 0x19c025) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x19c028, 0x19c029) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x19c02c, 0x19c02d) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x19c030, 0x19c031) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x19c034, 0x19c035) AM_READ(port_tag_to_handler16("IN2"))
ADDRESS_MAP_END


static ADDRESS_MAP_START( fixeight_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM
	AM_RANGE(0x200000, 0x200001) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x200004, 0x200005) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x200008, 0x200009) AM_READ(port_tag_to_handler16("IN3"))
	AM_RANGE(0x200010, 0x200011) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x20001c, 0x20001d) AM_WRITE(toaplan2_coin_word_w)	/* Coin count/lock */
	AM_RANGE(0x280000, 0x28dfff) AM_RAM							/* part of shared ram ? */
#if V25
	AM_RANGE(0x28e000, 0x28fbff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)	/* $28f000 status port */
	AM_RANGE(0x28fc00, 0x28ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#else
	AM_RANGE(0x28e000, 0x28efff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)
	AM_RANGE(0x28f000, 0x28f001) AM_READWRITE(fixeight_sec_cpu_r, fixeight_sec_cpu_w)	/* V25+ Command/Status port */
//  AM_RANGE(0x28f002, 0x28f003) AM_READ(MRA16_RAM)             /* part of shared ram */
//  AM_RANGE(0x28f004, 0x28f005) AM_READ(port_tag_to_handler16("DSWA")) /* Dip Switch A - Wrong !!! */
//  AM_RANGE(0x28f006, 0x28f007) AM_READ(port_tag_to_handler16("DSWB")) /* Dip Switch B - Wrong !!! */
//  AM_RANGE(0x28f008, 0x28f009) AM_READ(port_tag_to_handler16("JMPR")) /* Territory Jumper block - Wrong !!! */
//  AM_RANGE(0x28f00a, 0x28fbff) AM_READ(MRA16_RAM)             /* part of shared ram */
	AM_RANGE(0x28fc00, 0x28ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#endif
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x500000, 0x501fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)
	AM_RANGE(0x502000, 0x5021ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x503000, 0x5031ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x600000, 0x60ffff) AM_READWRITE(toaplan2_tx_gfxram16_r, toaplan2_tx_gfxram16_w) AM_BASE(&toaplan2_tx_gfxram16)
	AM_RANGE(0x800000, 0x800001) AM_READ(video_count_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( fixeighb_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM		/* 0-$7ffff ?*/
	AM_RANGE(0x100000, 0x10ffff) AM_RAM		/* 100000 - 107fff  105000-105xxx 106000-106xxx 108000 - related to sound ?*/
	AM_RANGE(0x200000, 0x200001) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x200004, 0x200005) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x200008, 0x200009) AM_READ(port_tag_to_handler16("IN3"))
	AM_RANGE(0x20000c, 0x20000d) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x200010, 0x200011) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x200014, 0x200015) AM_WRITE(fixeighb_oki_bankswitch_w)	/* Sound banking. Code at $4084c, $5070 */
	AM_RANGE(0x200018, 0x200019) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x20001c, 0x20001d) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x500000, 0x501fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)
	AM_RANGE(0x502000, 0x5021ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x503000, 0x5031ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x700000, 0x700001) AM_READ(video_count_r)
	AM_RANGE(0x800000, 0x87ffff) AM_READ(MRA16_BANK1)
ADDRESS_MAP_END


static ADDRESS_MAP_START( vfive_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM
//  AM_RANGE(0x200000, 0x20ffff) AM_ROM                         /* Sound ROM is here ??? */
	AM_RANGE(0x200010, 0x200011) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x200014, 0x200015) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x200018, 0x200019) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x20001c, 0x20001d) AM_WRITE(toaplan2_coin_word_w)	/* Coin count/lock */
#if V25
	AM_RANGE(0x21e000, 0x21fbff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)	/* $21f000 status port */
	AM_RANGE(0x21fc00, 0x21ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#else
	AM_RANGE(0x21e000, 0x21efff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)
	AM_RANGE(0x21f000, 0x21f001) AM_READWRITE(toaplan2_snd_cpu_r, vfive_snd_cpu_w)	/* V25+ Command/Status port */
	AM_RANGE(0x21f004, 0x21f005) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x21f006, 0x21f007) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x21f008, 0x21f009) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x21fc00, 0x21ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#endif
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x700000, 0x700001) AM_READ(video_count_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( batsugun_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x200010, 0x200011) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x200014, 0x200015) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x200018, 0x200019) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x20001c, 0x20001d) AM_WRITE(toaplan2_coin_word_w)	/* Coin count/lock */
	AM_RANGE(0x210000, 0x21bbff) AM_RAM
#if V25
	AM_RANGE(0x21e000, 0x21fbff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)	/* $21f000 status port */
	AM_RANGE(0x21fc00, 0x21ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#else
	AM_RANGE(0x21e000, 0x21efff) AM_READWRITE(shared_ram_r, shared_ram_w) AM_BASE(&toaplan2_shared_ram16)
	AM_RANGE(0x21f000, 0x21f001) AM_READWRITE(toaplan2_snd_cpu_r, batsugun_snd_cpu_w)	/* V25+ Command/Status port */
	AM_RANGE(0x21f004, 0x21f005) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x21f006, 0x21f007) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x21f008, 0x21f009) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x21fc00, 0x21ffff) AM_READWRITE(V25_sharedram_r, V25_sharedram_w) AM_BASE(&V25_shared_ram)	/* 16-bit on 68000 side, 8-bit on V25+ side */
#endif
	/***** The following in 0x30000x are for video controller 1 ******/
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	/***** The following in 0x50000x are for video controller 2 ******/
	AM_RANGE(0x500000, 0x500001) AM_WRITE(toaplan2_1_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x500004, 0x500007) AM_READWRITE(toaplan2_1_videoram16_r, toaplan2_1_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x500008, 0x500009) AM_WRITE(toaplan2_1_scroll_reg_select_w)
	AM_RANGE(0x50000c, 0x50000d) AM_WRITE(toaplan2_1_scroll_reg_data_w)
	AM_RANGE(0x700000, 0x700001) AM_READ(video_count_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( snowbro2_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)	/* VideoRAM selector/offset */
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x500000, 0x500001) AM_WRITE(YM2151_register_port_0_lsb_w)
	AM_RANGE(0x500002, 0x500003) AM_READWRITE(YM2151_status_port_0_lsb_r, YM2151_data_port_0_lsb_w)
	AM_RANGE(0x600000, 0x600001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x700000, 0x700001) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x700004, 0x700005) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x700008, 0x700009) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x70000c, 0x70000d) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x700010, 0x700011) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x700014, 0x700015) AM_READ(port_tag_to_handler16("IN3"))
	AM_RANGE(0x700018, 0x700019) AM_READ(port_tag_to_handler16("IN4"))
	AM_RANGE(0x70001c, 0x70001d) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x700030, 0x700031) AM_WRITE(oki_bankswitch_w)		/* Sample bank switch */
	AM_RANGE(0x700034, 0x700035) AM_WRITE(toaplan2_coin_word_w)	/* Coin count/lock */
ADDRESS_MAP_END


static ADDRESS_MAP_START( mahoudai_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x218000, 0x21bfff) AM_READWRITE(raizing_shared_ram_r, raizing_shared_ram_w)
	AM_RANGE(0x21c01c, 0x21c01d) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x21c020, 0x21c021) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x21c024, 0x21c025) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x21c028, 0x21c029) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x21c02c, 0x21c02d) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x21c030, 0x21c031) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x21c034, 0x21c035) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x21c03c, 0x21c03d) AM_READ(video_count_r)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x401000, 0x4017ff) AM_RAM							/* Unused PaletteRAM */
	AM_RANGE(0x500000, 0x501fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)
	AM_RANGE(0x502000, 0x5021ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x502200, 0x502fff) AM_RAM
	AM_RANGE(0x503000, 0x5031ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x503200, 0x503fff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( shippumd_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x218000, 0x21bfff) AM_READWRITE(raizing_shared_ram_r, raizing_shared_ram_w)
//  AM_RANGE(0x21c008, 0x21c009) AM_WRITENOP                    /* ??? */
	AM_RANGE(0x21c01c, 0x21c01d) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x21c020, 0x21c021) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x21c024, 0x21c025) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x21c028, 0x21c029) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x21c02c, 0x21c02d) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x21c030, 0x21c031) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x21c034, 0x21c035) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x21c03c, 0x21c03d) AM_READ(video_count_r)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x401000, 0x4017ff) AM_RAM							/* Unused PaletteRAM */
	AM_RANGE(0x500000, 0x501fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)
	AM_RANGE(0x502000, 0x5021ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x502200, 0x502fff) AM_RAM
	AM_RANGE(0x503000, 0x5031ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x503200, 0x503fff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( battleg_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x218020, 0x218023) AM_READ(battleg_z80check_r)
	AM_RANGE(0x21c01c, 0x21c01d) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x21c020, 0x21c021) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x21c024, 0x21c025) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x21c028, 0x21c029) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x21c02c, 0x21c02d) AM_READ(port_tag_to_handler16("DSWA"))
	AM_RANGE(0x21c030, 0x21c031) AM_READ(port_tag_to_handler16("DSWB"))
	AM_RANGE(0x21c034, 0x21c035) AM_READ(port_tag_to_handler16("JMPR"))
	AM_RANGE(0x21c03c, 0x21c03d) AM_READ(video_count_r)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x300004, 0x300007) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x300008, 0x300009) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x30000c, 0x30000d) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x500000, 0x501fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)
	AM_RANGE(0x502000, 0x5021ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x502200, 0x502fff) AM_RAM
	AM_RANGE(0x503000, 0x5031ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x503200, 0x503fff) AM_RAM
	AM_RANGE(0x600000, 0x600fff) AM_READWRITE(battleg_commram_r, battleg_commram_w) AM_BASE(&battleg_commram16)	/* CommRAM check */
ADDRESS_MAP_END


static ADDRESS_MAP_START( batrider_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x200000, 0x201fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)	/* Text VideoRAM */
	AM_RANGE(0x202000, 0x202fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16) AM_SIZE(&batrider_paletteram16_size)
	AM_RANGE(0x203000, 0x2031ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x203200, 0x2033ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x203400, 0x207fff) AM_READWRITE(raizing_tx_gfxram16_r, raizing_tx_gfxram16_w)	/* Main RAM actually */
	AM_RANGE(0x208000, 0x20ffff) AM_RAM
	AM_RANGE(0x300000, 0x37ffff) AM_READ(raizing_z80rom_r)
	AM_RANGE(0x400000, 0x400001) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400004, 0x400005) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x400008, 0x40000b) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x40000c, 0x40000d) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x500000, 0x500001) AM_READ(port_tag_to_handler16("IN"))
	AM_RANGE(0x500002, 0x500003) AM_READ(port_tag_to_handler16("SYS-DSW"))
	AM_RANGE(0x500004, 0x500005) AM_READ(port_tag_to_handler16("DSW"))
	AM_RANGE(0x500006, 0x500007) AM_READ(video_count_r)
	AM_RANGE(0x500008, 0x50000b) AM_READ(raizing_sndcomms_r)
	AM_RANGE(0x50000c, 0x50000d) AM_READ(batrider_z80_busack_r)
	AM_RANGE(0x500010, 0x500011) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x500020, 0x500023) AM_WRITE(raizing_sndcomms_w) AM_BASE(&raizing_cpu_comm16)
	AM_RANGE(0x500060, 0x500061) AM_WRITE(batrider_z80_busreq_w)
	AM_RANGE(0x500080, 0x500081) AM_WRITE(batrider_textdata_decode)
	AM_RANGE(0x5000c0, 0x5000cf) AM_WRITE(batrider_objectbank_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( bbakraid_68k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x200000, 0x201fff) AM_READWRITE(toaplan2_txvideoram16_r, toaplan2_txvideoram16_w) AM_BASE(&toaplan2_txvideoram16) AM_SIZE(&toaplan2_tx_vram_size)	/* Text VideoRAM */
	AM_RANGE(0x202000, 0x202fff) AM_READWRITE(paletteram16_word_r, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16) AM_SIZE(&batrider_paletteram16_size)
	AM_RANGE(0x203000, 0x2031ff) AM_READWRITE(toaplan2_txvideoram16_offs_r, toaplan2_txvideoram16_offs_w) AM_BASE(&toaplan2_txvideoram16_offs) AM_SIZE(&toaplan2_tx_offs_vram_size)
	AM_RANGE(0x203200, 0x2033ff) AM_READWRITE(toaplan2_txscrollram16_r, toaplan2_txscrollram16_w) AM_BASE(&toaplan2_txscrollram16) AM_SIZE(&toaplan2_tx_scroll_vram_size)
	AM_RANGE(0x203400, 0x207fff) AM_READWRITE(raizing_tx_gfxram16_r, raizing_tx_gfxram16_w)	/* Main RAM actually */
	AM_RANGE(0x208000, 0x20ffff) AM_RAM
	AM_RANGE(0x300000, 0x33ffff) AM_READ(raizing_z80rom_r)
	AM_RANGE(0x400000, 0x400001) AM_READWRITE(toaplan2_inputport_0_word_r, toaplan2_0_scroll_reg_data_w)	/* VBlank */
	AM_RANGE(0x400004, 0x400005) AM_WRITE(toaplan2_0_scroll_reg_select_w)
	AM_RANGE(0x400008, 0x40000b) AM_READWRITE(toaplan2_0_videoram16_r, toaplan2_0_videoram16_w)	/* Tile/Sprite VideoRAM */
	AM_RANGE(0x40000c, 0x40000d) AM_WRITE(toaplan2_0_voffs_w)
	AM_RANGE(0x500000, 0x500001) AM_READ(port_tag_to_handler16("IN"))
	AM_RANGE(0x500002, 0x500003) AM_READ(port_tag_to_handler16("SYS-DSW"))
	AM_RANGE(0x500004, 0x500005) AM_READ(port_tag_to_handler16("DSW"))
	AM_RANGE(0x500006, 0x500007) AM_READ(video_count_r)
	AM_RANGE(0x500008, 0x500009) AM_WRITE(bbakraid_trigger_z80_irq)
	AM_RANGE(0x500010, 0x500011) AM_WRITE(toaplan2_coin_word_w)
	AM_RANGE(0x500010, 0x500013) AM_READ(raizing_sndcomms_r)
	AM_RANGE(0x500014, 0x500017) AM_WRITE(raizing_sndcomms_w) AM_BASE(&raizing_cpu_comm16)
	AM_RANGE(0x500018, 0x500019) AM_READ(bbakraid_nvram_r)
	AM_RANGE(0x50001e, 0x50001f) AM_WRITE(bbakraid_nvram_w)
	AM_RANGE(0x500080, 0x500081) AM_WRITE(batrider_textdata_decode)
	AM_RANGE(0x5000c0, 0x5000cf) AM_WRITE(batrider_objectbank_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START( sound_z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM AM_BASE(&toaplan2_shared_ram)
	AM_RANGE(0xe000, 0xe000) AM_READWRITE(YM3812_status_port_0_r, YM3812_control_port_0_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM3812_write_port_0_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( raizing_sound_z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xdfff) AM_RAM AM_BASE(&raizing_shared_ram)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xe001, 0xe001) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0xe004, 0xe004) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xe00e, 0xe00e) AM_WRITE(toaplan2_coin_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( battleg_sound_z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xdfff) AM_RAM AM_BASE(&raizing_shared_ram)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xe001, 0xe001) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0xe004, 0xe004) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xe006, 0xe006) AM_WRITE(raizing_okim6295_bankselect_0)
	AM_RANGE(0xe008, 0xe008) AM_WRITE(raizing_okim6295_bankselect_1)
	AM_RANGE(0xe00a, 0xe00a) AM_WRITE(battleg_bankswitch_w)
	AM_RANGE(0xe00c, 0xe00c) AM_WRITE(battleg_commram_check_w0)
	AM_RANGE(0xe01c, 0xe01d) AM_READ(battleg_commram_check_r0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( batrider_sound_z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xdfff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( batrider_sound_z80_port, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_WRITE(raizing_command_ack_w)		/* Tune control */
	AM_RANGE(0x42, 0x42) AM_WRITE(raizing_request_ack_w)		/* Tune to play */
	AM_RANGE(0x46, 0x46) AM_WRITE(raizing_clear_nmi_w)			/* Clear the NMI state */
	AM_RANGE(0x48, 0x48) AM_READ(raizing_command_r)
	AM_RANGE(0x4a, 0x4a) AM_READ(raizing_request_r)
	AM_RANGE(0x80, 0x80) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x81, 0x81) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0x82, 0x82) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x84, 0x84) AM_READWRITE(OKIM6295_status_1_r, OKIM6295_data_1_w)
	AM_RANGE(0x88, 0x88) AM_WRITE(batrider_bankswitch_w)
	AM_RANGE(0xc0, 0xc0) AM_WRITE(raizing_okim6295_bankselect_0)
	AM_RANGE(0xc2, 0xc2) AM_WRITE(raizing_okim6295_bankselect_1)
	AM_RANGE(0xc4, 0xc4) AM_WRITE(raizing_okim6295_bankselect_2)
	AM_RANGE(0xc6, 0xc6) AM_WRITE(raizing_okim6295_bankselect_3)
ADDRESS_MAP_END


static ADDRESS_MAP_START( bbakraid_sound_z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM			/* Only 2FFFh valid code */
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xffff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( bbakraid_sound_z80_port, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_WRITE(raizing_command_ack_w)	/* Tune control */
	AM_RANGE(0x42, 0x42) AM_WRITE(raizing_request_ack_w)	/* Tune to play */
	AM_RANGE(0x46, 0x46) AM_WRITE(raizing_clear_nmi_w)		/* Clear the NMI state */
	AM_RANGE(0x48, 0x48) AM_READ(raizing_command_r)
	AM_RANGE(0x4a, 0x4a) AM_READ(raizing_request_r)
	AM_RANGE(0x80, 0x80) AM_WRITE(YMZ280B_register_0_w)
	AM_RANGE(0x81, 0x81) AM_READWRITE(YMZ280B_status_0_r, YMZ280B_data_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ghox_hd647180_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_READ(MRA8_ROM)   /* Internal 16k byte ROM */
	AM_RANGE(0x0fe00, 0x0ffff) AM_READ(MRA8_RAM)   /* Internal 512 byte RAM */
	AM_RANGE(0x3fe00, 0x3ffff) AM_READ(MRA8_RAM)   /* RAM (is this actually just internal RAM getting mapped badly?) */
	AM_RANGE(0x40000, 0x4f7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x80002, 0x80002) AM_READ(port_tag_to_handler8("DSWA"))
	AM_RANGE(0x80004, 0x80004) AM_READ(port_tag_to_handler8("DSWB"))
	AM_RANGE(0x80006, 0x80006) AM_READNOP // nothing?
	AM_RANGE(0x80008, 0x80008) AM_READ(port_tag_to_handler8("IN1"))
	AM_RANGE(0x8000a, 0x8000a) AM_READ(port_tag_to_handler8("IN2"))
	AM_RANGE(0x8000c, 0x8000e) AM_READ(port_tag_to_handler8("SYS"))
	AM_RANGE(0x8000e, 0x8000f) AM_READ(YM2151_status_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ghox_hd647180_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_WRITE(MWA8_ROM)   /* Internal 16k byte ROM */
	AM_RANGE(0x0fe00, 0x0ffff) AM_WRITE(MWA8_RAM)   /* Internal 512 byte RAM */
	AM_RANGE(0x3fe00, 0x3ffff) AM_WRITE(MWA8_RAM)   /* RAM (is this actually just internal RAM getting mapped badly?) */
	AM_RANGE(0x40000, 0x4f7ff) AM_WRITE(MWA8_RAM) AM_BASE(&toaplan2_shared_ram)
	AM_RANGE(0x8000e, 0x8000f) AM_WRITE(YM2151_word_0_w)
ADDRESS_MAP_END


READ8_HANDLER(tekipaki_soundlatch_r)
{
	m_cmdavailable = 0;
	return mcu_data;
};

READ8_HANDLER(tekipaki_cmdavailable_r)
{
	if (m_cmdavailable) return 0xff;
	else return 0x00;
};


static ADDRESS_MAP_START( hd647180_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x0fe00, 0x0ffff) AM_READ(MRA8_RAM)			/* Internal 512 bytes of RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( hd647180_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x0fe00, 0x0ffff) AM_WRITE(MWA8_RAM)			/* Internal 512 bytes of RAM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( hd647180_port_readmem, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x60, 0x60) AM_READ(tekipaki_cmdavailable_r)
	AM_RANGE(0x82, 0x82) AM_READ(YM3812_status_port_0_r)
	AM_RANGE(0x83, 0x83) AM_READ(YM3812_status_port_0_r)
	AM_RANGE(0x84, 0x84) AM_READ(tekipaki_soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hd647180_port_writemem, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x82, 0x82) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0x83, 0x83) AM_WRITE(YM3812_write_port_0_w)
ADDRESS_MAP_END


#if V25
static ADDRESS_MAP_START( V25_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
//  AM_RANGE(0x00000, 0x007ff) AM_RAM                           /* External shared RAM (Banked) */
	AM_RANGE(0x04000, 0x04000) AM_READWRITE(YM2151_status_port_0_r, YM2151_register_port_0_w)
	AM_RANGE(0x04001, 0x04001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x04002, 0x04002) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x04004, 0x04004) AM_WRITE(oki_bankswitch_w)
	AM_RANGE(0x04008, 0x04008) AM_READ(port_tag_to_handler16("IN1"))
	AM_RANGE(0x0400a, 0x0400a) AM_READ(port_tag_to_handler16("IN2"))
	AM_RANGE(0x0400c, 0x0400c) AM_READ(port_tag_to_handler16("SYS"))
	AM_RANGE(0x0400e, 0x0400e) AM_WRITE(toaplan2_coin_w)
	AM_RANGE(0x0fe00, 0x0ffff) AM_RAM							/* Internal 512 bytes of RAM */
	AM_RANGE(0x80000, 0x87fff) AM_RAM AM_BASE(&V25_sharedram)	/* External shared RAM (ROM for KBASH) */
ADDRESS_MAP_END


static ADDRESS_MAP_START( V25_port, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0060, 0x0060) AM_READ(port_tag_to_handler16("DSWA"))	/* Directly mapped I/O ports */
	AM_RANGE(0x0061, 0x0061) AM_READ(port_tag_to_handler16("DSWA"))	/* Directly mapped I/O ports */
	AM_RANGE(0x0062, 0x0062) AM_READ(port_tag_to_handler16("JMPR"))	/* Directly mapped I/O ports */
ADDRESS_MAP_END
#endif

static ADDRESS_MAP_START( dogyuun_z80_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe001, 0xe001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xe004, 0xe004) AM_READ(OKIM6295_status_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dogyuun_z80_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM) AM_BASE(&toaplan2_shared_ram)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xe004, 0xe004) AM_WRITE(OKIM6295_data_0_w)
ADDRESS_MAP_END


/*****************************************************************************
    Input Port definitions
    Service input of the TOAPLAN2_SYSTEM_INPUTS is used as a Pause type input.
    If you press then release the following buttons, the following occurs:
    Service & P2 start            : The game will pause.
    P1 start                      : The game will continue.
    Service & P1 start & P2 start : The game will play in slow motion.
*****************************************************************************/


#define TOAPLAN2_COINAGE( mask, value)																				\
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Coin_A ) )		PORT_DIPLOCATION("SW1:6,5")								\
	PORT_DIPSETTING(		0x0030, DEF_STR( 4C_1C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_EQUALS,value)		\
	PORT_DIPSETTING(		0x0020, DEF_STR( 3C_1C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_EQUALS,value)		\
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_EQUALS,value)		\
	PORT_DIPSETTING(		0x0020, DEF_STR( 2C_1C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_NOTEQUALS,value)	\
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )																\
	PORT_DIPSETTING(		0x0030, DEF_STR( 2C_3C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_NOTEQUALS,value)	\
	PORT_DIPSETTING(		0x0010, DEF_STR( 1C_2C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_NOTEQUALS,value)	\
	PORT_DIPNAME( 0x00c0,	0x0000, DEF_STR( Coin_B ) )		PORT_DIPLOCATION("SW1:8,7")								\
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_NOTEQUALS,value)	\
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_NOTEQUALS,value)	\
	PORT_DIPSETTING(		0x00c0, DEF_STR( 2C_3C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_NOTEQUALS,value)	\
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_2C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_NOTEQUALS,value)	\
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_2C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_EQUALS,value)		\
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_EQUALS,value)		\
	PORT_DIPSETTING(		0x0080, DEF_STR( 1C_4C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_EQUALS,value)		\
	PORT_DIPSETTING(		0x00c0, DEF_STR( 1C_6C ) )		PORT_CONDITION("JMPR",mask,PORTCOND_EQUALS,value)

INPUT_PORTS_START( toaplan2_extr )
	PORT_START_TAG("EXTR")
	PORT_DIPNAME( 0x0001,	0x0001, "Buffer Delay" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( toaplan2 )
	PORT_START_TAG("VBL")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("SYS")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Unused ) )			PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH ) 				/// PORT_DIPLOCATION("SW1:3")
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	PORT_BIT( 0x00f0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Modified below */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x0003,	0x0000, DEF_STR( Difficulty ) )		PORT_DIPLOCATION("SW2:2,1")
	PORT_DIPSETTING(		0x0003, DEF_STR( Hardest ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( Hard ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( Easy ) )
	/* Various features on bit mask 0x00fc */
	PORT_BIT( 0x00fc, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Modified below */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("JMPR")	/* (6) Territory Jumper block */
	/* Territory settings on bit mask 0x000f */
	PORT_BIT( 0x000f, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Modified below */
	PORT_BIT( 0xfff0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */
INPUT_PORTS_END


INPUT_PORTS_START( tekipaki )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("DSWA")
	/* Various features on bit mask 0x000f - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x000f, 0x0002 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x0004,	0x0000, DEF_STR( Unused ) )		PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Unused ) )		PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010,	0x0000, DEF_STR( Unused ) )		PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020,	0x0000, DEF_STR( Unused ) )		PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040,	0x0000, "Game Mode" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0040, "Stop" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Normal ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Unused ) )		PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( On ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000f,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0002, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x0007, "USA (Romstar)" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x000f, "Japan (Distributed by Tecmo)" )
	PORT_DIPSETTING(		0x0004, "Korea" )
	PORT_DIPSETTING(		0x0003, "Hong Kong" )
	PORT_DIPSETTING(		0x0008, "Hong Kong (Honest Trading Co.)" )
	PORT_DIPSETTING(		0x0005, "Taiwan" )
	PORT_DIPSETTING(		0x0006, "Taiwan (Spacy Co. Ltd)" )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( ghox )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("DSWA")
	/* Various features on bit mask 0x000f - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x80000, 0x80000 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "100k only" )
	PORT_DIPSETTING(		0x0004, "100k and 300k" )
	PORT_DIPSETTING(		0x0000, "100k and every 200k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040, 	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Unused ) )			PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( On ) )

	PORT_MODIFY("JMPR")
	/* Bit Mask 0x80000 is used here to signify European Coinage for MAME purposes - not read on the real board! */
	PORT_DIPNAME( 0x8000f,	0x00000, "Territory" )
	PORT_DIPSETTING(		0x80002, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x8000a, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(		0x8000d, "Europe (Taito Corporation Japan)" )
	PORT_DIPSETTING(		0x00001, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x00009, "USA (Romstar)" )
	PORT_DIPSETTING(		0x0000b, "USA (Taito America Corporation)" )
	PORT_DIPSETTING(		0x0000c, "USA (Taito Corporation Japan)" )
	PORT_DIPSETTING(		0x00000, DEF_STR( Japan ) )
//  PORT_DIPSETTING(        0x0000e, "Japan (Taito Corporation)" )
	PORT_DIPSETTING(		0x00004, "Korea" )
	PORT_DIPSETTING(		0x00003, "Hong Kong (Honest Trading Co.)" )
	PORT_DIPSETTING(		0x00005, "Taiwan" )
	PORT_DIPSETTING(		0x80006, "Spain & Portugal (APM Electronics SA)" )
	PORT_DIPSETTING(		0x80007, "Italy (Star Electronica SRL)" )
	PORT_DIPSETTING(		0x80008, "UK (JP Leisure Ltd)" )

	PORT_START_TAG("PAD1")		/* Paddle 1 (left-right)  read at $100000 */
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("PAD2")		/* Paddle 2 (left-right)  read at $040000 */
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2)
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( dogyuun )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_MODIFY("IN2")
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Free_Play) )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x000f, 0x0003 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "400k only" )
	PORT_DIPSETTING(		0x0000, "200k only" )
	PORT_DIPSETTING(		0x0004, "200k, 400k and 600k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000f,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0003, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( USA ) )
//  PORT_DIPSETTING(        0x0007, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x0002, "USA (Atari Games Corp license)" )
//  PORT_DIPSETTING(        0x000c, "USA (Atari Games Corp license)" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x000f, "Japan (Taito Corp license)" )
	PORT_DIPSETTING(		0x0008, "South East Asia (Charterfield license)" )
//  PORT_DIPSETTING(        0x000d, "South East Asia (Charterfield license)" )
	PORT_DIPSETTING(		0x0005, "Korea (Unite Trading license)" )
//  PORT_DIPSETTING(        0x000a, "Korea (Unite Trading license)" )
	PORT_DIPSETTING(		0x0004, "Hong Kong (Charterfield license)" )
//  PORT_DIPSETTING(        0x0009, "Hong Kong (Charterfield license)" )
	PORT_DIPSETTING(		0x0006, "Taiwan" )
//  PORT_DIPSETTING(        0x000b, "Taiwan" )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Sound ready */

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( kbash )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_MODIFY("IN2")
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Normal ) )
	PORT_DIPSETTING(		0x0001, "Discount" )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x0007, 0x0002 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0004, "100k only" )
	PORT_DIPSETTING(		0x0000, "100k and 400k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0000, "2" )
	PORT_DIPSETTING(		0x0020, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000f,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x000a, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0002, "Europe, USA (Atari license)" )
	PORT_DIPSETTING(		0x0009, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x0001, "USA, Europe (Atari license)" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x0006, "South East Asia" )	/*Service Mode lists European Coinage */
	PORT_DIPSETTING(		0x0003, "Korea" )
	PORT_DIPSETTING(		0x0004, "Hong Kong" )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( kbash2 )
	PORT_INCLUDE(kbash)

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000f,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0000, "Japan (Taito Corp license)" )
	PORT_DIPSETTING(		0x000e, "South East Asia" )	/*Service Mode lists European Coinage */
	PORT_DIPSETTING(		0x0006, "South East Asia (Charterfield license)" )	/*Service Mode lists European Coinage */
	PORT_DIPSETTING(		0x000b, "Korea" )
	PORT_DIPSETTING(		0x0003, "Korea (Unite license)" )
	PORT_DIPSETTING(		0x0004, "Hong Kong" )
	PORT_DIPSETTING(		0x0005, "Taiwan" )
INPUT_PORTS_END


INPUT_PORTS_START( truxton2 )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_MODIFY("IN2")
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, "Rapid Fire" )				PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x0007, 0x0002 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, "200k only" )
	PORT_DIPSETTING(		0x0008, "100k only" )
	PORT_DIPSETTING(		0x0004, "100k and 250k" )
	PORT_DIPSETTING(		0x0000, "70k and 200k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0020, "4" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x0007,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0002, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x0006, "South East Asia" )
	PORT_DIPSETTING(		0x0004, "Korea" )
	PORT_DIPSETTING(		0x0003, "Hong Kong" )
	PORT_DIPSETTING(		0x0005, "Taiwan" )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( pipibibs )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("DSWA")
	/* Various features on bit mask 0x000f - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x0006, 0x0006 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0000, "200k and every 300k" )
	PORT_DIPSETTING(		0x0004, "150k and every 200k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Unused ) )			PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( On ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x0008,	0x0000, "Nudity" )
	PORT_DIPSETTING(		0x0008, DEF_STR( Low ) )
	PORT_DIPSETTING(		0x0000, "High, but censored" )
	PORT_DIPNAME( 0x0007,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0006, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0007, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(		0x0004, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x0005, "USA (Romstar)" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( Asia ) )
	PORT_DIPSETTING(		0x0002, "Hong Kong (Honest Trading Co.)" )
	PORT_DIPSETTING(		0x0003, "Taiwan" )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( whoopee )
	PORT_INCLUDE(pipibibs)

	PORT_MODIFY("JMPR")
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END


INPUT_PORTS_START( pipibibi )
	PORT_INCLUDE(pipibibs)

	PORT_MODIFY("VBL")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_UNKNOWN )		/* This video HW doesn't use this VBlank input */

	PORT_MODIFY("DSWA")
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNKNOWN )	PORT_DIPLOCATION("SW1:2")	/* This video HW doesn't support flip screen */
	/* Various features on bit mask 0x000d - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x80000, 0x80000 )

	PORT_MODIFY("JMPR")
	/* Bit Mask 0x80000 is used here to signify European Coinage for MAME purposes - not read on the real board! */
	PORT_DIPNAME( 0x80007,	0x00000, "Territory" )
	PORT_DIPSETTING(		0x00002, DEF_STR( World ) )
//  PORT_DIPSETTING(        0x00003, DEF_STR( World ) )
	PORT_DIPSETTING(		0x00007, "World (Ryouta Kikaku)" )
	PORT_DIPSETTING(		0x80005, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x00004, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x00000, "Japan (Ryouta Kikaku)" )
	PORT_DIPSETTING(		0x00001, "Hong Kong (Honest Trading Co.)" )
	PORT_DIPSETTING(		0x80006, "Spain & Portugal (APM Electronics SA)" )
INPUT_PORTS_END


INPUT_PORTS_START( fixeight )
	PORT_INCLUDE(toaplan2)

	PORT_START_TAG("IN3")
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START3 )

	PORT_MODIFY("SYS")
	PORT_SERVICE_NO_TOGGLE(0x0004, IP_ACTIVE_HIGH)	/* Service input is a push-button marked 'Test SW' */

#if 0
	PORT_START_TAG("FAKE")		/* Fake input, to display message */
	PORT_DIPNAME( 0x0000,	0x0000, "    Press service button" )
	PORT_DIPSETTING(		0x0000, "" )
	PORT_DIPNAME( 0x0000,	0x0000, "  for game keeping options" )
	PORT_DIPSETTING(		0x0000, "" )
	PORT_DIPNAME( 0x0000,	0x0000, "" )
	PORT_DIPSETTING(		0x0000, "" )
#endif

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, "Maximum Players" )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, "2" )
	PORT_DIPSETTING(		0x0001, "3" )
	PORT_DIPNAME( 0x0004,	0x0004, "Shooting style" )		PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0004, "Semi-auto" )
	PORT_DIPSETTING(		0x0000, "Fully-auto" )
	/* Various features on bit mask 0x000a - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x000e, 0x0008 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0000, "500k and every 500k" )
	PORT_DIPSETTING(		0x0008, "300k only" )
	PORT_DIPSETTING(		0x0004, "300k and every 300k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000f,	0x000e, "Territory" )
	PORT_DIPSETTING(		0x0009, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0008, "Europe (Taito Corp)" )
	PORT_DIPSETTING(		0x000b, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x000a, "USA (Taito America Corp)" )
	PORT_DIPSETTING(		0x000e, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x000f, "Japan (Taito corp)" )
	PORT_DIPSETTING(		0x0007, "South East Asia" )
	PORT_DIPSETTING(		0x0006, "South East Asia (Taito corp)" )
	PORT_DIPSETTING(		0x0001, "Korea" )
	PORT_DIPSETTING(		0x0000, "Korea (Taito Corp)" )
	PORT_DIPSETTING(		0x0003, "Hong Kong" )
	PORT_DIPSETTING(		0x0002, "Hong Kong (Taito Corp)" )
	PORT_DIPSETTING(		0x0005, "Taiwan" )
	PORT_DIPSETTING(		0x0004, "Taiwan (Taito corp)" )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( fixeighb )
	PORT_INCLUDE(fixeight)

	PORT_MODIFY("DSWA")
	 PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNKNOWN )	PORT_DIPLOCATION("SW1:2")	/* This video HW doesn't support flip screen */

	PORT_MODIFY("JMPR")
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNKNOWN )		/* Doesn't exist? */
INPUT_PORTS_END


INPUT_PORTS_START( grindstm )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Cabinet ) )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( Cocktail ) )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x000f, 0x0009 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0000, "300k and 800k" )
	PORT_DIPSETTING(		0x0004, "300k and every 800k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000f,	0x0009, "Territory" )
	PORT_DIPSETTING(		0x0009, DEF_STR( Europe ) )
//  PORT_DIPSETTING(        0x0008, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x000d, DEF_STR( USA ) )
//  PORT_DIPSETTING(        0x000b, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x000c, "USA (American Sammy Corporation license)" )
//  PORT_DIPSETTING(        0x000a, "USA (American Sammy Corporation license)" )
	PORT_DIPSETTING(		0x0007, "South East Asia" )
	PORT_DIPSETTING(		0x0006, "South East Asia (Charterfield license)" )
//  PORT_DIPSETTING(        0x000f, "Korea" )
//  PORT_DIPSETTING(        0x000e, "Korea" )
	PORT_DIPSETTING(		0x0001, "Korea" )
	PORT_DIPSETTING(		0x0000, "Korea (Unite Trading license)" )
	PORT_DIPSETTING(		0x0003, "Hong Kong" )
	PORT_DIPSETTING(		0x0002, "Hong Kong (Charterfield license)" )
	PORT_DIPSETTING(		0x0005, "Taiwan" )
	PORT_DIPSETTING(		0x0004, "Taiwan (Anomoto International Inc license)" )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( vfive )
	PORT_INCLUDE(grindstm)

	PORT_MODIFY("JMPR")		/* (6) Territory Jumper block */
	/* Territory is forced to Japan in this set. */
	PORT_BIT( 0x000f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END


INPUT_PORTS_START( batsugun )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )			PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Normal ) )
	PORT_DIPSETTING(		0x0001, "Discount" )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x00ff, 0x00ff )	/* Actually European coinage isn't used */

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "1500k only" )
	PORT_DIPSETTING(		0x0000, "1000k only" )
	PORT_DIPSETTING(		0x0004, "500k and every 600k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000f,	0x000f, "Territory" )
	PORT_DIPSETTING(		0x0009, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0008, "Europe (Taito Corp license)" )
	PORT_DIPSETTING(		0x000b, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x000a, "USA (Taito Corp license)" )
	PORT_DIPSETTING(		0x000f, DEF_STR( Japan ) )
//  PORT_DIPSETTING(        0x000e, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x000d, "Japan (Taito Corp license)" )
//  PORT_DIPSETTING(        0x000c, "Japan (Taito Corp license)" )
	PORT_DIPSETTING(		0x0007, "South East Asia" )
	PORT_DIPSETTING(		0x0006, "South East Asia (Taito Corp license)" )
	PORT_DIPSETTING(		0x0001, "Korea" )
	PORT_DIPSETTING(		0x0000, "Korea (Unite Trading license)" )
	PORT_DIPSETTING(		0x0003, "Hong Kong" )
	PORT_DIPSETTING(		0x0002, "Hong Kong (Taito Corp license)" )
	PORT_DIPSETTING(		0x0005, "Taiwan" )
	PORT_DIPSETTING(		0x0004, "Taiwan (Taito Corp license)" )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( snowbro2 )
	PORT_INCLUDE(toaplan2)

	PORT_START_TAG("IN3")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("IN4")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START4 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Normal ) )
	PORT_DIPSETTING(		0x0001, "Discount" )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see TOAPLAN2_COINAGE above */
	TOAPLAN2_COINAGE( 0x1C00, 0x0800 )	/* European coinage is shown in service mode, but not used */

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )	PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0000, "100k only" )
	PORT_DIPSETTING(		0x0004, "100k and every 500k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )		PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )		PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Maximum Players" )		PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, "2" )
	PORT_DIPSETTING(		0x0000, "4" )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x2000,	0x0000, "Show All Rights Reserved" )
	PORT_DIPSETTING(		0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(		0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1c00,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0800, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( USA ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x1800, "South East Asia" )
	PORT_DIPSETTING(		0x0c00, "Korea" )
	PORT_DIPSETTING(		0x1000, "Hong Kong" )
	PORT_DIPSETTING(		0x1400, "Taiwan" )
	PORT_BIT( 0xc3ff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( sstriker )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Free_Play ) )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see RAIZING8_COINAGE above */
	TOAPLAN2_COINAGE( 0x000e, 0x0004 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0000, "Every 300k" )
	PORT_DIPSETTING(		0x0004, "200k and 500k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000e,	0x0004, "Territory" )
	PORT_DIPSETTING(		0x0004, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( USA ) )
//  PORT_DIPSETTING(        0x0000, DEF_STR( Japan ) )  /* Wrong Title screen - use Mahoudai*/
	PORT_DIPSETTING(		0x0006, "South East Asia" )
	PORT_DIPSETTING(		0x0008, "China" )
	PORT_DIPSETTING(		0x000a, "Korea (Unite Trading license)" )
	PORT_DIPSETTING(		0x000c, "Hong Kong" )
	PORT_DIPSETTING(		0x000e, "Taiwan" )
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( mahoudai )
	PORT_INCLUDE(sstriker)

	PORT_MODIFY("JMPR")
	/* Not used, it seems. This setting forces Japan for Territory */
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( kingdmgp )
	PORT_INCLUDE(toaplan2)

	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Free_Play ) )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
	/* Various features on bit mask 0x000e - see above */
	/* Coinage on bit mask 0x00f0 - see RAIZING8_COINAGE above */
	TOAPLAN2_COINAGE( 0x000e, 0x0004 )

	PORT_MODIFY("DSWB")
	/* Difficulty on bit mask 0x0003 - see above */
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )		PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x000c, DEF_STR( None ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0000, "Every 300k" )
	PORT_DIPSETTING(		0x0004, "200k and 500k" )
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "5" )
	PORT_DIPNAME( 0x0040,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x000e,	0x0004, "Territory" )
	PORT_DIPSETTING(		0x0004, DEF_STR( Europe ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( USA ) )
//  PORT_DIPSETTING(        0x0000, DEF_STR( Japan ) )  /* Title screen is wrong when set to Japan */
	PORT_DIPSETTING(		0x0006, "South East Asia" )
	PORT_DIPSETTING(		0x0008, "China" )
	PORT_DIPSETTING(		0x000a, "Korea (Unite Trading license)" )
	PORT_DIPSETTING(		0x000c, "Hong Kong" )
	PORT_DIPSETTING(		0x000e, "Taiwan" )
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( shippumd )
	PORT_INCLUDE(kingdmgp)

	PORT_MODIFY("JMPR")
	 /* Title screen is corrupt for anything but Japan setting so this forces it to Japan */
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( battleg )
	PORT_START_TAG("VBL")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("SYS")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Unknown/Unused */

	PORT_START_TAG("DSWA")
	PORT_SERVICE( 0x0001,	IP_ACTIVE_HIGH )			/// PORT_DIPLOCATION("SW1:1")
	PORT_DIPNAME( 0x0002,	0x0000, "Credits to Start" )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, "1" )
	PORT_DIPSETTING(		0x0002, "2" )
	PORT_DIPNAME( 0x001c,	0x0000, DEF_STR( Coin_A ) )		PORT_DIPLOCATION("SW1:5,4,3")
	PORT_DIPSETTING(		0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0014, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x001c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0,	0x0000, DEF_STR( Coin_B ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)	PORT_DIPLOCATION("SW1:8,7,6")
	PORT_DIPSETTING(		0x00c0, DEF_STR( 4C_1C ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x00a0, DEF_STR( 3C_1C ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
//  PORT_DIPSETTING(        0x00e0, DEF_STR( 1C_1C ) )      PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0020, DEF_STR( 1C_2C ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0060, DEF_STR( 1C_4C ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_NOTEQUALS,0x001c)
	/* When Coin_A is set to Free_Play, Coin_A becomes Coin_A and Coin_B, and the following dips occur */
	PORT_DIPNAME( 0x0020,	0x0000, "Joystick Mode" )		PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(		0x0000, "90 degrees ACW" )		PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0020, DEF_STR( Normal ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0040,	0x0000, "Effect" )				PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)	PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )			PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0080,	0x0000, "Music" )				PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)	PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )		PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0080, DEF_STR( On ) )			PORT_CONDITION("DSWA",0x001c,PORTCOND_EQUALS,0x001c)

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x0003,	0x0000, DEF_STR( Difficulty ) )		PORT_DIPLOCATION("SW2:2,1")
	PORT_DIPSETTING(		0x0003, DEF_STR( Hardest ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( Hard ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x0004,	0x0000, DEF_STR( Flip_Screen ) )	PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )	PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0070,	0x0000, DEF_STR( Lives ) )			PORT_DIPLOCATION("SW2:7,6,5")
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_DIPSETTING(		0x0040, "5" )
	PORT_DIPSETTING(		0x0050, "6" )
	PORT_DIPSETTING(		0x0060, DEF_STR( Infinite ) )
	PORT_DIPSETTING(		0x0070, "Invulnerability (Cheat)" )
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Bonus_Life ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( None ) )		PORT_CONDITION("JMPR",0x0003,PORTCOND_NOTEQUALS,0x0000)	/* Non-Japan */
	PORT_DIPSETTING(		0x0080, "Every 2000k" )			PORT_CONDITION("JMPR",0x0003,PORTCOND_NOTEQUALS,0x0000)	/* Non-Japan */
	PORT_DIPSETTING(		0x0080, "1000k and 2000k" )		PORT_CONDITION("JMPR",0x0003,PORTCOND_EQUALS,0x0000)	/* Japan */
	PORT_DIPSETTING(		0x0000, "Every 1000k" )			PORT_CONDITION("JMPR",0x0003,PORTCOND_EQUALS,0x0000)	/* Japan */

	PORT_START_TAG("JMPR")
	PORT_DIPNAME( 0x0008,	0x0000, "Stage Edit" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004,	0x0000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0003,	0x0001, "Territory" )
	PORT_DIPSETTING(		0x0001, "Denmark (German Tuning license)" )
	/* These two settings end up reporting ROM-0 as BAD */
//  PORT_DIPSETTING(        0x0002, "USA (Fabtek license)" )
//  PORT_DIPSETTING(        0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x0003, "China" )

	PORT_INCLUDE(toaplan2_extr)
INPUT_PORTS_END


INPUT_PORTS_START( battlega )
	PORT_INCLUDE(battleg)

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x0003,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0001, "Europe (German Tuning license)" )
	PORT_DIPSETTING(		0x0002, "USA (Fabtek license)" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x0003, DEF_STR( Asia ) )
INPUT_PORTS_END


INPUT_PORTS_START( battlegb )
	PORT_INCLUDE(battleg)

	PORT_MODIFY("JMPR")
	PORT_DIPNAME( 0x0003,	0x0001, "Territory" )
	PORT_DIPSETTING(		0x0001, "Austria (German Tuning license)" )
	/* These two settings end up reporting ROM-0 as BAD */
//  PORT_DIPSETTING(        0x0002, "USA (Fabtek license)" )
//  PORT_DIPSETTING(        0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(		0x0003, "Hong Kong" )
INPUT_PORTS_END


INPUT_PORTS_START( batrider )
	PORT_START_TAG("VBL")		/* VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN")		/* Player Inputs */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8080, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW")		/* DSWA and DSWB */
	PORT_SERVICE( 0x0001,	IP_ACTIVE_HIGH )			/// PORT_DIPLOCATION("SW1:1")
	PORT_DIPNAME( 0x0002,	0x0000, "Credits to Start" )	PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, "1" )					PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0002, "2" )					PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPNAME( 0x0002,	0x0000, "Joystick Mode" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, DEF_STR( Normal ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0002, "90 degrees ACW" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x001c,	0x0000, DEF_STR( Coin_A ) )		PORT_DIPLOCATION("SW1:5,4,3")
	PORT_DIPSETTING(		0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0014, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x001c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0,	0x0000, DEF_STR( Coin_B ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)	PORT_DIPLOCATION("SW1:8,7,6")
	PORT_DIPSETTING(		0x00c0, DEF_STR( 4C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x00a0, DEF_STR( 3C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
//  PORT_DIPSETTING(        0x00e0, DEF_STR( 1C_1C ) )      PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0020, DEF_STR( 1C_2C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0060, DEF_STR( 1C_4C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	/* When Coin_A is set to Free_Play, Coin_A becomes Coin_A and Coin_B, and the following dips occur */
	PORT_DIPNAME( 0x0020,	0x0000, "Hit Score" )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(		0x0020, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0040,	0x0000, "Sound Effect" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(		0x0040, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0080,	0x0000, "Music" )				PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0300,	0x0000, DEF_STR( Difficulty ) )	PORT_DIPLOCATION("SW2:2,1")
	PORT_DIPSETTING(		0x0300, DEF_STR( Hardest ) )
	PORT_DIPSETTING(		0x0200, DEF_STR( Hard ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0100, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x0c00,	0x0000, "Timer" )				PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x0c00, DEF_STR( Highest ) )
	PORT_DIPSETTING(		0x0800, DEF_STR( High ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( Low ) )
	PORT_DIPNAME( 0x3000,	0x0000, DEF_STR( Lives ) )		PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x3000, "1" )
	PORT_DIPSETTING(		0x2000, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x1000, "4" )
	PORT_DIPNAME( 0xc000,	0x0000, DEF_STR( Bonus_Life ) )	PORT_DIPLOCATION("SW2:8,7")
	PORT_DIPSETTING(		0xc000, DEF_STR( None ) )
	PORT_DIPSETTING(		0x8000, "Every 2000k" )
	PORT_DIPSETTING(		0x0000, "Every 1500k" )
	PORT_DIPSETTING(		0x4000, "Every 1000k" )

	PORT_START_TAG("SYS-DSW")	/* Coin/System and DSWC */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* Service */
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Test */
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0100,	0x0000, DEF_STR( Flip_Screen ) )	PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200,	0x0000, DEF_STR( Demo_Sounds ) )	PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(		0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400,	0x0000, "Stage Edit" )				PORT_DIPLOCATION("SW3:3")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(		0x0800, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1000,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x1000, DEF_STR( On ) )
	/* These Dips are showed only when Coin_A is set to Free_Play, but they work in normal play mode too. */
	PORT_DIPNAME( 0x2000,	0x0000, "Guest Player" )			PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(		0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000,	0x0000, "Player Select" )			PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(		0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000,	0x0000, "Special Course" )			PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(		0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( batridrj )
	PORT_START_TAG("VBL")		/* VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN")		/* Player Inputs */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8080, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW")		/* DSWA and DSWB */
	PORT_SERVICE( 0x0001,	IP_ACTIVE_HIGH )			/// PORT_DIPLOCATION("SW1:1")
	PORT_DIPNAME( 0x0002,	0x0000, "Credits to Start" )	PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, "1" )					PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0002, "2" )					PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPNAME( 0x0002,	0x0000, "Joystick Mode" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, DEF_STR( Normal ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0002, "90 degrees ACW" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x001c,	0x0000, DEF_STR( Coin_A ) )		PORT_DIPLOCATION("SW1:5,4,3")
	PORT_DIPSETTING(		0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0014, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x001c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0,	0x0000, DEF_STR( Coin_B ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)	PORT_DIPLOCATION("SW1:8,7,6")
	PORT_DIPSETTING(		0x00c0, DEF_STR( 4C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x00a0, DEF_STR( 3C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
//  PORT_DIPSETTING(        0x00e0, DEF_STR( 1C_1C ) )      PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0020, DEF_STR( 1C_2C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0060, DEF_STR( 1C_4C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	/* When Coin_A is set to Free_Play, Coin_A becomes Coin_A and Coin_B, and the following dips occur */
	PORT_DIPNAME( 0x0020,	0x0000, "Hit Score" )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(		0x0020, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0040,	0x0000, "Sound Effect" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(		0x0040, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0080,	0x0000, "Music" )				PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(		0x0080, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0300,	0x0000, DEF_STR( Difficulty ) )	PORT_DIPLOCATION("SW2:2,1")
	PORT_DIPSETTING(		0x0300, DEF_STR( Hardest ) )
	PORT_DIPSETTING(		0x0200, DEF_STR( Hard ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0100, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x0c00,	0x0000, "Timer" )				PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x0c00, DEF_STR( Highest ) )
	PORT_DIPSETTING(		0x0800, DEF_STR( High ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( Low ) )
	PORT_DIPNAME( 0x3000,	0x0000, DEF_STR( Lives ) )		PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x3000, "1" )
	PORT_DIPSETTING(		0x2000, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x1000, "4" )
	PORT_DIPNAME( 0xc000,	0x0000, DEF_STR( Bonus_Life ) )	PORT_DIPLOCATION("SW2:8,7")
	PORT_DIPSETTING(		0xc000, DEF_STR( None ) )
	PORT_DIPSETTING(		0x8000, "Every 2000k" )
	PORT_DIPSETTING(		0x0000, "Every 1500k" )
	PORT_DIPSETTING(		0x4000, "Every 1000k" )

	PORT_START_TAG("SYS-DSW")	/* Coin/System and DSWC */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* Service */
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Test */
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0100,	0x0000, DEF_STR( Flip_Screen ) )	PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200,	0x0000, DEF_STR( Demo_Sounds ) )	PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(		0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400,	0x0000, "Stage Edit" )				PORT_DIPLOCATION("SW3:3")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(		0x0800, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1000,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x1000, DEF_STR( On ) )
	/* These Dips are showed only when Coin_A is set to Free_Play, but they work in normal play mode too. */
	PORT_DIPNAME( 0x2000,	0x0000, "Guest Player" )			PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000,	0x0000, "Player Select" )			PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000,	0x0000, "Special Course" )			PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x8000, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( bbakraid )
	PORT_START_TAG("VBL")		/* VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN")		/* Player Inputs */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8080, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW")		/* DSWA and DSWB */
	PORT_SERVICE( 0x0001,	IP_ACTIVE_HIGH )			/// PORT_DIPLOCATION("SW1:1")
	PORT_DIPNAME( 0x0002,	0x0000, "Credits to Start" )	PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, "1" )					PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0002, "2" )					PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPNAME( 0x0002,	0x0000, "Joystick Mode" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)		PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(		0x0000, DEF_STR( Normal ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0002, "90 degrees ACW" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x001c,	0x0000, DEF_STR( Coin_A ) )		PORT_DIPLOCATION("SW1:5,4,3")
	PORT_DIPSETTING(		0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0014, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x001c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0,	0x0000, DEF_STR( Coin_B ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)	PORT_DIPLOCATION("SW1:8,7,6")
	PORT_DIPSETTING(		0x00c0, DEF_STR( 4C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x00a0, DEF_STR( 3C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
//  PORT_DIPSETTING(        0x00e0, DEF_STR( 1C_1C ) )      PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0020, DEF_STR( 1C_2C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	PORT_DIPSETTING(		0x0060, DEF_STR( 1C_4C ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_NOTEQUALS,0x001c)
	/* When Coin_A is set to Free_Play, Coin_A becomes Coin_A and Coin_B, and the following dips occur */
	PORT_DIPNAME( 0x0020,	0x0000, "Hit Score" )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0020, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0040,	0x0000, "Sound Effect" )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)	PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0080,	0x0000, "Music" )				PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)	PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )		PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPSETTING(		0x0080, DEF_STR( On ) )			PORT_CONDITION("DSW",0x001c,PORTCOND_EQUALS,0x001c)
	PORT_DIPNAME( 0x0300,	0x0000, DEF_STR( Difficulty ) )	PORT_DIPLOCATION("SW2:2,1")
	PORT_DIPSETTING(		0x0300, DEF_STR( Hardest ) )
	PORT_DIPSETTING(		0x0200, DEF_STR( Hard ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0100, DEF_STR( Easy ) )
	PORT_DIPNAME( 0x0c00,	0x0000, "Timer" )				PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(		0x0c00, DEF_STR( Highest ) )
	PORT_DIPSETTING(		0x0800, DEF_STR( High ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( Low ) )
	PORT_DIPNAME( 0x3000,	0x0000, DEF_STR( Lives ) )		PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(		0x3000, "1" )
	PORT_DIPSETTING(		0x2000, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x1000, "4" )
	PORT_DIPNAME( 0xc000,	0x0000, DEF_STR( Bonus_Life ) )	PORT_DIPLOCATION("SW2:8,7")
	PORT_DIPSETTING(		0xc000, DEF_STR( None ) )
	PORT_DIPSETTING(		0x8000, "Every 4000k" )
	PORT_DIPSETTING(		0x4000, "Every 3000k" )
	PORT_DIPSETTING(		0x0000, "Every 2000k" )

	PORT_START_TAG("SYS-DSW")	/* Coin/System and DSW-3 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* Service */
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Test */
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_DIPNAME( 0x0100,	0x0000, DEF_STR( Flip_Screen ) )	PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200,	0x0000, DEF_STR( Demo_Sounds ) )	PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(		0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400,	0x0000, "Stage Edit" )				PORT_DIPLOCATION("SW3:3")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800,	0x0000, DEF_STR( Allow_Continue ) )	PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(		0x0800, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1000,	0x0000, "Invulnerability" )			PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000,	0x0000, "Save Scores" )				PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(		0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000,	0x0000, DEF_STR( Unused ) )			PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000,	0x0000, DEF_STR( Unused ) )			PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x8000, DEF_STR( On ) )
INPUT_PORTS_END



static const gfx_layout tilelayout =
{
	16,16,			/* 16x16 */
	RGN_FRAC(1,2),	/* Number of tiles */
	4,				/* 4 bits per pixel */
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	8*4*16
};

static const gfx_layout spritelayout =
{
	8,8,			/* 8x8 */
	RGN_FRAC(1,2),	/* Number of 8x8 sprites */
	4,				/* 4 bits per pixel */
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static const gfx_layout raizing_textlayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

#ifdef LSB_FIRST
static const gfx_layout truxton2_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 16, 20, 32, 36, 48, 52 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};
#else
static const gfx_layout truxton2_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0+8, 4+8, 16+8, 20+8, 32+8, 36+8, 48+8, 52+8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};
#endif

#ifdef LSB_FIRST
static const gfx_layout batrider_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};
#else
static const gfx_layout batrider_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8, 12, 0, 4, 24, 28, 16, 20 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};
#endif


static const gfx_layout fixeighblayout =
{
   8,8,
   RGN_FRAC(1,1),
   4,
   { 0,1,2,3 },
   { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
   { 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8},
   8*8*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 128 },
	{ REGION_GFX1, 0, &spritelayout, 0,  64 },
	{ -1 } /* end of array */
};

static const gfx_decode gfxdecodeinfo_2[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 128 },
	{ REGION_GFX1, 0, &spritelayout, 0,  64 },
	{ REGION_GFX2, 0, &tilelayout,   0, 128 },
	{ REGION_GFX2, 0, &spritelayout, 0,  64 },
	{ -1 } /* end of array */
};

static const gfx_decode truxton2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0,       &tilelayout            , 0, 128 },
	{ REGION_GFX1, 0,       &spritelayout          , 0,  64 },
//  { REGION_CPU1, 0x40000, &truxton2_tx_tilelayout, 0, 128 },  /* Truxton 2 */
//  { REGION_CPU1, 0x68000, &truxton2_tx_tilelayout, 0, 128 },  /* Fix Eight */
	{ 0, 0, &truxton2_tx_tilelayout,  0, 128 },
	{ -1 } /* end of array */
};

static const gfx_decode raizing_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,         0, 128 },
	{ REGION_GFX1, 0, &spritelayout,       0,  64 },
	{ REGION_GFX2, 0, &raizing_textlayout, 0, 128 },		/* Extra-text layer */
	{ -1 } /* end of array */
};

/* This is wrong a bit. Text layer is dynamically changed. */
static const gfx_decode batrider_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,             0, 128 },
	{ REGION_GFX1, 0, &spritelayout,           0,  64 },
	{ 0,           0, &batrider_tx_tilelayout, 0,  16 },
	{ -1 } /* end of array */
};

static const gfx_decode fixeighb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout     , 0, 128 },
	{ REGION_GFX1, 0, &spritelayout   , 0,  64 },
	{ REGION_GFX2, 0, &fixeighblayout , 0, 128 },
	{ -1 } /* end of array */
};

static void irqhandler(int linestate)
{
	cpunum_set_input_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	irqhandler
};

static struct YMZ280Binterface ymz280b_interface =
{
	REGION_SOUND1,
	bbakraid_irqhandler
};


static MACHINE_DRIVER_START( tekipaki )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)			/* 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(tekipaki_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z180, 10000000)			/* HD647180 CPU actually */
	MDRV_CPU_PROGRAM_MAP(hd647180_readmem,hd647180_writemem)
	MDRV_CPU_IO_MAP(hd647180_port_readmem,hd647180_port_writemem)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
    MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 27000000/8)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ghox )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)			/* 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(ghox_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z180, 10000000)			/* HD647180 CPU actually */
	MDRV_CPU_PROGRAM_MAP(ghox_hd647180_readmem,ghox_hd647180_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)
	
	MDRV_MACHINE_RESET(ghox)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.25)
	MDRV_SOUND_ROUTE(1, "right", 0.25)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dogyuun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)			/* 16MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(dogyuun_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

#if V25
	MDRV_CPU_ADD(Z180, 16000000)			/* NEC V25+ type Toaplan marked CPU ??? */
	MDRV_CPU_PROGRAM_MAP(V25_mem, 0)
	MDRV_CPU_IO_MAP(V25_port, 0)
#endif

	MDRV_FRAMES_PER_SECOND( (27000000.0 / 4) / (432 * 263) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(dogyuun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo_2)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_1)
	MDRV_VIDEO_EOF(toaplan2_1)
	MDRV_VIDEO_UPDATE(dogyuun_1)

	/* sound hardware */	
	/*
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.25)
	MDRV_SOUND_ROUTE(1, "right", 0.25)

	MDRV_SOUND_ADD(OKIM6295, 27000000/10/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
	*/
	MDRV_SPEAKER_STANDARD_MONO("mono")
	
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(dogyuun_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dogyuunto )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 24000000/2)		/* 12MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(dogyuun_z80_readmem,dogyuun_z80_writemem)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z80,27000000/8)			/* ??? 3.37MHz , 27MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(dogyuun_z80_sound_readmem,dogyuun_z80_sound_writemem)


	MDRV_FRAMES_PER_SECOND( (27000000.0 / 4) / (432 * 263) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(dogyuun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo_2)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_1)
	MDRV_VIDEO_EOF(toaplan2_1)
	MDRV_VIDEO_UPDATE(dogyuun_1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.25)
	MDRV_SOUND_ROUTE(1, "right", 0.25)

	MDRV_SOUND_ADD(OKIM6295, 1056000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kbash )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)			/* 16MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(kbash_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

#if V25
	MDRV_CPU_ADD(Z180, 16000000)			/* NEC V25+ type Toaplan marked CPU ??? */
	MDRV_CPU_PROGRAM_MAP(V25_mem, 0)
	MDRV_CPU_IO_MAP(V25_port, 0)
#endif

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 32000000/32/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kbash2 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)			/* 16MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(kbash2_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 16000000/16/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 16000000/16/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( truxton2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)			/* 16MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(truxton2_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq2,262)

	MDRV_FRAMES_PER_SECOND( (27000000.0 / 4) / (432 * 263) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(truxton2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(truxton2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(truxton2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.5)
	MDRV_SOUND_ROUTE(1, "right", 0.5)

	MDRV_SOUND_ADD(OKIM6295, 27000000/10/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pipibibs )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)			/* 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(pipibibs_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z80,27000000/8)			/* ??? 3.37MHz , 27MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(sound_z80_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 27000000/8)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( whoopee )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)			/* 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(tekipaki_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z80, 27000000/8)			/* This should be a HD647180 */
											/* Change this to 10MHz when HD647180 gets dumped. 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(sound_z80_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 27000000/8)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pipibibi )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)			/* 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(pipibibi_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z80,27000000/8)			/* ??? 3.37MHz */
	MDRV_CPU_PROGRAM_MAP(sound_z80_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 27000000/8)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fixeight )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)			/* 16MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(fixeight_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

#if V25
	MDRV_CPU_ADD(Z180, 16000000)			/* NEC V25+ type Toaplan marked CPU ??? */
	MDRV_CPU_PROGRAM_MAP(V25_mem, 0)
	MDRV_CPU_IO_MAP(V25_port, 0)
#endif

	MDRV_FRAMES_PER_SECOND( (27000000.0 / 4) / (432 * 263) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(toaplan2)
//  MDRV_NVRAM_HANDLER(fixeight)        /* See 37B6 code */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(truxton2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(truxton2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(truxton2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 32000000/32/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fixeighb )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)			/* 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(fixeighb_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq2,262)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(fixeighb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(truxton2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(truxton2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 14000000/16/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vfive )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)			/* 10MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(vfive_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

#if V25
	MDRV_CPU_ADD(Z180, 10000000)			/* NEC V25+ type Toaplan marked CPU ??? */
	MDRV_CPU_PROGRAM_MAP(V25_mem, 0)
	MDRV_CPU_IO_MAP(V25_port, 0)
#endif

	MDRV_FRAMES_PER_SECOND( (27000000.0 / 4) / (432 * 263) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(vfive)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	/*
	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.25)
	MDRV_SOUND_ROUTE(1, "right", 0.25)
	*/

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(vfive_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( batsugun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)			/* 16MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(batsugun_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

#if V25
	MDRV_CPU_ADD(Z180, 32000000/2)			/* NEC V25+ type Toaplan marked CPU ??? */
	MDRV_CPU_PROGRAM_MAP(V25_mem, 0)
	MDRV_CPU_IO_MAP(V25_port, 0)
#endif

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo_2)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_1)
	MDRV_VIDEO_EOF(toaplan2_1)
	MDRV_VIDEO_UPDATE(batsugun_1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	/*
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.25)
	MDRV_SOUND_ROUTE(1, "right", 0.25)

	MDRV_SOUND_ADD(OKIM6295, 32000000/8/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
	*/
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(batsugun_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)	
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( snowbro2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(snowbro2_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(toaplan2_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(toaplan2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 27000000/10/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mahoudai )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)			/* 16MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(mahoudai_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z80,32000000/8)			/* 4MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(raizing_sound_z80_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(raizing_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(battleg_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(mahoudai_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.95)
	MDRV_SOUND_ROUTE(1, "right", 0.95)

	MDRV_SOUND_ADD(OKIM6295, 32000000/32/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.95)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.95)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( shippumd )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)			/* 16MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(shippumd_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z80,32000000/8)			/* 4MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(raizing_sound_z80_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(raizing_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(battleg_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(truxton2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 27000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 32000000/32/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( battleg )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)			/* 16MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(battleg_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq4,262)

	MDRV_CPU_ADD(Z80,32000000/8)			/* 4MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(battleg_sound_z80_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(toaplan2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(raizing_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(battleg_0)
	MDRV_VIDEO_EOF(toaplan2_0)
	MDRV_VIDEO_UPDATE(truxton2_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 32000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 32000000/16/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.45)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( batrider )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)			/* 16MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(batrider_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq2,262)

	MDRV_CPU_ADD(Z80,32000000/8)			/* 4MHz , 32MHz Oscillator */
	MDRV_CPU_PROGRAM_MAP(batrider_sound_z80_mem, 0)
	MDRV_CPU_IO_MAP(batrider_sound_z80_port, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(batrider)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(batrider_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(batrider_0)
	MDRV_VIDEO_EOF(batrider_0)
	MDRV_VIDEO_UPDATE(batrider_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 32000000/8)
	MDRV_SOUND_ROUTE(0, "left", 0.5)
	MDRV_SOUND_ROUTE(1, "right", 0.5)

	MDRV_SOUND_ADD(OKIM6295, 32000000/10/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)

	MDRV_SOUND_ADD(OKIM6295, 32000000/10/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bbakraid )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000/2)
	MDRV_CPU_PROGRAM_MAP(bbakraid_68k_mem, 0)
	MDRV_CPU_VBLANK_INT(toaplan2_vblank_irq3,262)

	MDRV_CPU_ADD(Z80,32000000/4)
	MDRV_CPU_PROGRAM_MAP(bbakraid_sound_z80_mem, 0)
	MDRV_CPU_IO_MAP(bbakraid_sound_z80_port, 0)
	MDRV_CPU_PERIODIC_INT(bbakraid_snd_interrupt, TIME_IN_HZ(388))
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(262)

	MDRV_MACHINE_RESET(toaplan2)
	MDRV_NVRAM_HANDLER(bbakraid)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(batrider_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(batrider_0)
	MDRV_VIDEO_UPDATE(batrider_0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_interface)
	MDRV_SOUND_ROUTE(0, "mono", 1.0)
	MDRV_SOUND_ROUTE(1, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

/* -------------------------- Toaplan games ------------------------- */
ROM_START( tekipaki )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "tp020-1.bin", 0x000000, 0x010000, CRC(d8420bd5) SHA1(30c1ad9e053cd7e79adb42aa428ebee28e144755) )
	ROM_LOAD16_BYTE( "tp020-2.bin", 0x000001, 0x010000, CRC(7222de8e) SHA1(8352ae23efc24a2e20cc24b6d37cb8fc6b1a730c) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 )    /* Sound HD647180 code */
	ROM_LOAD( "hd647180.020", 0x00000, 0x08000, CRC(d5157c12) SHA1(b2c6c087bb539456a9e562d0b40f05dde26cacd3) )


	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp020-4.bin", 0x000000, 0x080000, CRC(3ebbe41e) SHA1(cea196c5f83e1a23d5b538a0db9bbbffa7af5118) )
	ROM_LOAD( "tp020-3.bin", 0x080000, 0x080000, CRC(2d5e2201) SHA1(5846c844eedd48305c1c67dc645b6e070b3f5b98) )
ROM_END


ROM_START( ghox )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "tp021-01.u10", 0x000000, 0x020000, CRC(9e56ac67) SHA1(daf241d9e55a6e60fc004ed61f787641595b1e62) )
	ROM_LOAD16_BYTE( "tp021-02.u11", 0x000001, 0x020000, CRC(15cac60f) SHA1(6efa3a50a5dfe6ef4072738d6a7d0d95dca8a675) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound HD647180 code */
    ROM_LOAD( "hd647180.021", 0x00000, 0x08000, CRC(6ab59e5b) SHA1(d814dd3a8f1ee638794e2bd422eed4247ba4a15e) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp021-03.u36", 0x000000, 0x080000, CRC(a15d8e9d) SHA1(640a33997bdce8e84bea6a944139716379839037) )
	ROM_LOAD( "tp021-04.u37", 0x080000, 0x080000, CRC(26ed1c9a) SHA1(37da8af86ea24327444c2d4ad3dfbd936208d43d) )
ROM_END


ROM_START( ghoxj )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "tp021-01a.u10", 0x000000, 0x020000, CRC(c11b13c8) SHA1(da7defc1d3b6ddded910ba56c31fbbdb5ed57b09) )
	ROM_LOAD16_BYTE( "tp021-02a.u11", 0x000001, 0x020000, CRC(8d426767) SHA1(1ed4a8bcbf4352257e7d58cb5c2c91eb48c2f047) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound HD647180 code */
    ROM_LOAD( "hd647180.021", 0x00000, 0x08000, CRC(6ab59e5b) SHA1(d814dd3a8f1ee638794e2bd422eed4247ba4a15e) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp021-03.u36", 0x000000, 0x080000, CRC(a15d8e9d) SHA1(640a33997bdce8e84bea6a944139716379839037) )
	ROM_LOAD( "tp021-04.u37", 0x080000, 0x080000, CRC(26ed1c9a) SHA1(37da8af86ea24327444c2d4ad3dfbd936208d43d) )
ROM_END


ROM_START( dogyuun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp022_01.r16", 0x000000, 0x080000, CRC(79eb2429) SHA1(088c5ed0ed77557ab71f52cafe35028e3648ae1e) )

	/* Secondary CPU is a Toaplan marked chip, (TS-002-MACH  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
//  ROM_LOAD( "tp022.mcu", 0x00000, 0x08000, NO_DUMP )
#endif

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "tp022_3.w92", 0x000000, 0x100000, CRC(191b595f) SHA1(89344946daa18087cc83f92027cf5da659b1c7a5) )
	ROM_LOAD16_WORD_SWAP( "tp022_4.w93", 0x100000, 0x100000, CRC(d58d29ca) SHA1(90d142fef37764ef817347a2bed77892a288a077) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "tp022_5.w16", 0x000000, 0x200000, CRC(d4c1db45) SHA1(f5655467149ba737128c2f54c9c6cdaca6e4c35c) )
	ROM_LOAD16_WORD_SWAP( "tp022_6.w17", 0x200000, 0x200000, CRC(d48dc74f) SHA1(081b5a00a2ff2bd82b98b30aab3cb5b6ae1014d5) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "tp022_2.w30", 0x00000, 0x40000, CRC(043271b3) SHA1(c7eaa929e55dd956579b824ea9d20a1d0129a925) )
ROM_END

ROM_START( dogyuunto )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "8-25.u11", 0x000000, 0x080000, CRC(4d3c952f) SHA1(194f3065c513238921047ead8b425c3d0538b9a7) ) // real hand-written label is '8/25'

	ROM_REGION( 0x08000,  REGION_CPU2, 0 )
	ROM_LOAD( "u25", 0x00000, 0x08000, CRC(41a34a7e) SHA1(c4f7833249436fd064c7088c9776d12dee4a7d39) ) // only had a white label

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "tp022_3.w92", 0x000000, 0x100000, CRC(191b595f) SHA1(89344946daa18087cc83f92027cf5da659b1c7a5) )
	ROM_LOAD16_WORD_SWAP( "tp022_4.w93", 0x100000, 0x100000, CRC(d58d29ca) SHA1(90d142fef37764ef817347a2bed77892a288a077) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "tp022_5.w16", 0x000000, 0x200000, CRC(d4c1db45) SHA1(f5655467149ba737128c2f54c9c6cdaca6e4c35c) )
	ROM_LOAD16_WORD_SWAP( "tp022_6.w17", 0x200000, 0x200000, CRC(d48dc74f) SHA1(081b5a00a2ff2bd82b98b30aab3cb5b6ae1014d5) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "2m.u29", 0x00000, 0x40000, CRC(5e7a77d8) SHA1(da6beb5e8e015965ff42fd52f5aa0c0ae5bcee4f) ) // '2M' hand-written
ROM_END


ROM_START( kbash )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "kbash01.bin", 0x000000, 0x080000, CRC(2965f81d) SHA1(46f2df30fa92c80ba5a37f75e756424e15534784) )

	/* Secondary CPU is a Toaplan marked chip, (TS-004-Dash  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x88000, REGION_CPU2, 0 )			/* Sound CPU code */
	ROM_LOAD( "kbash02.bin", 0x80000, 0x08000, CRC(4cd882a1) SHA1(7199a5c384918f775f0815e09c46b2a58141814a) )
#else
	ROM_REGION( 0x08000, REGION_USER1, 0 )
	ROM_LOAD( "kbash02.bin", 0x00200, 0x07e00, CRC(4cd882a1) SHA1(7199a5c384918f775f0815e09c46b2a58141814a) )
	ROM_CONTINUE(			 0x00000, 0x00200 )
#endif

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "kbash03.bin", 0x000000, 0x200000, CRC(32ad508b) SHA1(e473489beaf649d3e5236770eb043327e309850c) )
	ROM_LOAD( "kbash05.bin", 0x200000, 0x200000, CRC(b84c90eb) SHA1(17a1531d884d9a9696d1b25d65f9155f02396e0e) )
	ROM_LOAD( "kbash04.bin", 0x400000, 0x200000, CRC(e493c077) SHA1(0edcfb70483ad07206695d9283031b85cd198a36) )
	ROM_LOAD( "kbash06.bin", 0x600000, 0x200000, CRC(9084b50a) SHA1(03b58278619524d2f09a4b1c152d5e057e792a56) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "kbash07.bin", 0x00000, 0x40000, CRC(3732318f) SHA1(f0768459f5ad2dee53d408a0a5ae3a314864e667) )
ROM_END


ROM_START( kbash2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "mecat-m", 0x000000, 0x80000, CRC(bd2263c6) SHA1(eb794c0fc9c1fb4337114d48149283d42d22e4b3) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mecat-34", 0x000000, 0x400000, CRC(6be7b37e) SHA1(13160ad0712fee932bb98cc226e651895b19228a) )
	ROM_LOAD( "mecat-12", 0x400000, 0x400000, CRC(49e46b1f) SHA1(d12b12696a8473eb34f3cd247ab060289a6c0e9c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Music */
	ROM_LOAD( "mecat-s", 0x00000, 0x80000, CRC(3eb7adf4) SHA1(b0e6e99726b854858bd0e69eb77f12b9664b35e6) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )			/* ADPCM Samples */
	ROM_LOAD( "eprom",   0x00000, 0x40000, CRC(31115cb9) SHA1(c79ea01bd865e2fc3aaab3ff05483c8fd27e5c98) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )			/* ??? Some sort of table  - same as in pipibibi*/
	ROM_LOAD( "050917-10", 0x0000, 0x10000, CRC(6b213183) SHA1(599c59d155d11edb151bfaed1d24ef964462a447) )
ROM_END


ROM_START( truxton2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp024_1.bin", 0x000000, 0x080000, CRC(eb26f0e5) SHA1(4fb1e8f6d7d62138b408db932c15dd7dc8d4c367) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp024_4.bin", 0x000000, 0x100000, CRC(805c449e) SHA1(fdf985344145bd320b88b9b0c25e73066c9b2ada) )
	ROM_LOAD( "tp024_3.bin", 0x100000, 0x100000, CRC(47587164) SHA1(bac493e2d5507286b984957b289c929335d27eaa) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp024_2.bin", 0x00000, 0x80000, CRC(f2f6cae4) SHA1(bb4e8c36531bed97ced4696ca12fd40ede2531aa) )
ROM_END

ROM_START( pipibibs )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "tp025-1.bin", 0x000000, 0x020000, CRC(b2ea8659) SHA1(400431b656dbfbd5a9bc5961c3ea04c4d38b6f77) )
	ROM_LOAD16_BYTE( "tp025-2.bin", 0x000001, 0x020000, CRC(dc53b939) SHA1(e4de371f97ba7c350273ad43b7f58ff31672a269) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "tp025-5.bin", 0x0000, 0x8000, CRC(bf8ffde5) SHA1(79c09cc9a0ea979f5af5a7e5ad671ea486f5f43e) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp025-4.bin", 0x000000, 0x100000, CRC(ab97f744) SHA1(c1620e614345dbd5c6567e4cb6f55c61b900d0ee) )
	ROM_LOAD( "tp025-3.bin", 0x100000, 0x100000, CRC(7b16101e) SHA1(ae0119bbfa0937d18c4fbb0a3ef7cdc3b9fa6b56) )
ROM_END


ROM_START( whoopee )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "whoopee.1", 0x000000, 0x020000, CRC(28882e7e) SHA1(8fcd278a7d005eb81cd9e461139c0c0f756a4fa4) )
	ROM_LOAD16_BYTE( "whoopee.2", 0x000001, 0x020000, CRC(6796f133) SHA1(d4e657be260ba3fd3f0556ade617882513b52685) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound HD647180 code */
    ROM_LOAD( "hd647180.025", 0x00000, 0x08000, CRC(c02436f6) SHA1(385343f88991646ec23b385eaea82718f1251ea6) )
	
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp025-4.bin", 0x000000, 0x100000, CRC(ab97f744) SHA1(c1620e614345dbd5c6567e4cb6f55c61b900d0ee) )
	ROM_LOAD( "tp025-3.bin", 0x100000, 0x100000, CRC(7b16101e) SHA1(ae0119bbfa0937d18c4fbb0a3ef7cdc3b9fa6b56) )
ROM_END


ROM_START( pipibibi )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "ppbb06.bin", 0x000000, 0x020000, CRC(14c92515) SHA1(2d7f7c89272bb2a8115f163ad651bef3bca5107e) )
	ROM_LOAD16_BYTE( "ppbb05.bin", 0x000001, 0x020000, CRC(3d51133c) SHA1(d7bd94ad11e9aeb5a5165c5ac6f71950849bcd2f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ppbb08.bin", 0x0000, 0x8000, CRC(101c0358) SHA1(162e02d00b7bdcdd3b48a0cd0527b7428435ec50) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	/* GFX data differs slightly from Toaplan boards ??? */
	ROM_LOAD16_BYTE( "ppbb01.bin", 0x000000, 0x080000, CRC(0fcae44b) SHA1(ac72bc79e3a5d0a81647c312d310d00ace017272) )
	ROM_LOAD16_BYTE( "ppbb02.bin", 0x000001, 0x080000, CRC(8bfcdf87) SHA1(4537a7d646d3014f069c6fd0be457bb32e2f18ac) )
	ROM_LOAD16_BYTE( "ppbb03.bin", 0x100000, 0x080000, CRC(abdd2b8b) SHA1(a4246dd63515f01d1227c9a9e16d9f1c739ee39e) )
	ROM_LOAD16_BYTE( "ppbb04.bin", 0x100001, 0x080000, CRC(70faa734) SHA1(4448f4dbded56c142e57293d371e0a422c3a667e) )

	ROM_REGION( 0x8000, REGION_USER1, 0 )			/* ??? Some sort of table */
	ROM_LOAD( "ppbb07.bin", 0x0000, 0x8000, CRC(456dd16e) SHA1(84779ee64d3ea33ba1ba4dee39b504a81c6811a1) )
ROM_END


ROM_START( fixeight )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp-026-1", 0x000000, 0x080000, CRC(f7b1746a) SHA1(0bbea6f111b818bc9b9b2060af4fe900f37cf7f9) )

	/* Secondary CPU is a Toaplan marked chip, (TS-001-Turbo  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Secondary CPU code */
//  ROM_LOAD( "tp-026.mcu", 0x0000, 0x8000, NO_DUMP )
#endif

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp-026-3", 0x000000, 0x200000, CRC(e5578d98) SHA1(280d2b716d955e767d311fc9596823852435b6d7) )
	ROM_LOAD( "tp-026-4", 0x200000, 0x200000, CRC(b760cb53) SHA1(bc9c5e49e45cdda0f774be0038aa4deb21d4d285) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp-026-2", 0x00000, 0x40000, CRC(85063f1f) SHA1(1bf4d77494de421c98f6273b9876e60d827a6826) )

	ROM_REGION( 0x80, REGION_USER1, 0 )
	/* Serial EEPROM (93C45) connected to Secondary CPU */
	ROM_LOAD( "93c45.u21", 0x00, 0x80, CRC(40d75df0) SHA1(a22f1cc74ce9bc9bfe53f48f6a43ab60e921052b) )
ROM_END


ROM_START( fixeighb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "3.bin", 0x000000, 0x80000, CRC(cc77d4b4) SHA1(4d3376cbae13d90c6314d8bb9236c2183fc6253c) )
	ROM_LOAD16_BYTE( "2.bin", 0x000001, 0x80000, CRC(ed715488) SHA1(37be9bc8ff6b54a1f660d89469c6c2da6301e9cd) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp-026-3", 0x000000, 0x200000, CRC(e5578d98) SHA1(280d2b716d955e767d311fc9596823852435b6d7) )
	ROM_LOAD( "tp-026-4", 0x200000, 0x200000, CRC(b760cb53) SHA1(bc9c5e49e45cdda0f774be0038aa4deb21d4d285) )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE)
	ROM_LOAD( "4.bin", 0x00000, 0x08000, CRC(a6aca465) SHA1(2b331faeee1832e0adc5218254a99d66331862c6) )

	ROM_REGION( 0x90000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "1.bin", 0x00000, 0x30000, CRC(888f19ac) SHA1(d2f4f8b7be7a0fdb95baa0af8930e50e2f875c05) )
	ROM_CONTINUE(      0x40000, 0x50000 )

	ROM_REGION( 0x8000, REGION_USER1, 0 )			/* ??? Some sort of table  - same as in pipibibi*/
	ROM_LOAD( "5.bin", 0x0000, 0x8000, CRC(456dd16e) SHA1(84779ee64d3ea33ba1ba4dee39b504a81c6811a1) )
ROM_END


ROM_START( grindstm )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "01.bin", 0x000000, 0x080000, CRC(4923f790) SHA1(1c2d66b432d190d0fb6ac7ca0ec0687aea3ccbf4) )

	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
//  ROM_LOAD( "tp027.mcu", 0x8000, 0x8000, NO_DUMP )
#endif

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp027_02.bin", 0x000000, 0x100000, CRC(877b45e8) SHA1(b3ed8d8dbbe51a1919afc55d619d2b6771971493) )
	ROM_LOAD( "tp027_03.bin", 0x100000, 0x100000, CRC(b1fc6362) SHA1(5e97e3cce31be57689d394a50178cda4d80cce5f) )
ROM_END


ROM_START( grindsta )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp027-01.rom", 0x000000, 0x080000, CRC(8d8c0392) SHA1(824dde274c8bef8a87c54d8ccdda7f0feb8d11e1) )

	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
//  ROM_LOAD( "tp027.mcu", 0x8000, 0x8000, NO_DUMP )
#endif

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp027_02.bin", 0x000000, 0x100000, CRC(877b45e8) SHA1(b3ed8d8dbbe51a1919afc55d619d2b6771971493) )
	ROM_LOAD( "tp027_03.bin", 0x100000, 0x100000, CRC(b1fc6362) SHA1(5e97e3cce31be57689d394a50178cda4d80cce5f) )
ROM_END


ROM_START( vfive )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp027_01.bin", 0x000000, 0x080000, CRC(731d50f4) SHA1(794255d0a809cda9170f5bac473df9d7f0efdac8) )

	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
//  ROM_LOAD( "tp027.mcu", 0x8000, 0x8000, NO_DUMP )
#endif

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp027_02.bin", 0x000000, 0x100000, CRC(877b45e8) SHA1(b3ed8d8dbbe51a1919afc55d619d2b6771971493) )
	ROM_LOAD( "tp027_03.bin", 0x100000, 0x100000, CRC(b1fc6362) SHA1(5e97e3cce31be57689d394a50178cda4d80cce5f) )
ROM_END


ROM_START( batsugun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp030_01.bin", 0x000000, 0x080000, CRC(3873d7dd) SHA1(baf6187d7d554cfcf4a86b63f07fc30df7ef84c9) )

	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
//  ROM_LOAD( "tp030.mcu", 0x8000, 0x8000, NO_DUMP )
#endif

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_3l.bin", 0x000000, 0x100000, CRC(3024b793) SHA1(e161db940f069279356fca2c5bf2753f07773705) )
	ROM_LOAD( "tp030_3h.bin", 0x100000, 0x100000, CRC(ed75730b) SHA1(341f0f728144a049486d996c9bb14078578c6879) )
	ROM_LOAD( "tp030_4l.bin", 0x200000, 0x100000, CRC(fedb9861) SHA1(4b0917056bd359b21935358c6bcc729262be6417) )
	ROM_LOAD( "tp030_4h.bin", 0x300000, 0x100000, CRC(d482948b) SHA1(31be7dc5cff072403b783bf203b9805ffcad7284) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_5.bin",  0x000000, 0x100000, CRC(bcf5ba05) SHA1(40f98888a29cdd30cda5dfb60fdc667c69b0fdb0) )
	ROM_LOAD( "tp030_6.bin",  0x100000, 0x100000, CRC(0666fecd) SHA1(aa8f921fc51590b5b05bbe0b0ad0cce5ff359c64) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp030_2.bin", 0x00000, 0x40000, CRC(276146f5) SHA1(bf11d1f6782cefcad77d52af4f7e6054a8f93440) )
ROM_END


ROM_START( batsugna )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp030_1a.bin", 0x000000, 0x080000,  CRC(cb1d4554) SHA1(ef31f24d77e1c13bdf5558a04a6253e2e3e6a790) )

	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
//  ROM_LOAD( "tp030.mcu", 0x8000, 0x8000, NO_DUMP )
#endif

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_3l.bin", 0x000000, 0x100000, CRC(3024b793) SHA1(e161db940f069279356fca2c5bf2753f07773705) )
	ROM_LOAD( "tp030_3h.bin", 0x100000, 0x100000, CRC(ed75730b) SHA1(341f0f728144a049486d996c9bb14078578c6879) )
	ROM_LOAD( "tp030_4l.bin", 0x200000, 0x100000, CRC(fedb9861) SHA1(4b0917056bd359b21935358c6bcc729262be6417) )
	ROM_LOAD( "tp030_4h.bin", 0x300000, 0x100000, CRC(d482948b) SHA1(31be7dc5cff072403b783bf203b9805ffcad7284) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_5.bin",  0x000000, 0x100000, CRC(bcf5ba05) SHA1(40f98888a29cdd30cda5dfb60fdc667c69b0fdb0) )
	ROM_LOAD( "tp030_6.bin",  0x100000, 0x100000, CRC(0666fecd) SHA1(aa8f921fc51590b5b05bbe0b0ad0cce5ff359c64) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp030_2.bin", 0x00000, 0x40000, CRC(276146f5) SHA1(bf11d1f6782cefcad77d52af4f7e6054a8f93440) )
ROM_END


ROM_START( batugnsp )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp030-sp.u69", 0x000000, 0x080000, CRC(8072a0cd) SHA1(3a0a9cdf894926a16800c4882a2b00383d981367) )

	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its likely to be a NEC V25+ (PLCC94). */
#if V25
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
//  ROM_LOAD( "tp030.mcu", 0x8000, 0x8000, NO_DUMP )
#endif

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_3l.bin", 0x000000, 0x100000, CRC(3024b793) SHA1(e161db940f069279356fca2c5bf2753f07773705) )
	ROM_LOAD( "tp030_3h.bin", 0x100000, 0x100000, CRC(ed75730b) SHA1(341f0f728144a049486d996c9bb14078578c6879) )
	ROM_LOAD( "tp030_4l.bin", 0x200000, 0x100000, CRC(fedb9861) SHA1(4b0917056bd359b21935358c6bcc729262be6417) )
	ROM_LOAD( "tp030_4h.bin", 0x300000, 0x100000, CRC(d482948b) SHA1(31be7dc5cff072403b783bf203b9805ffcad7284) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_5.bin",  0x000000, 0x100000, CRC(bcf5ba05) SHA1(40f98888a29cdd30cda5dfb60fdc667c69b0fdb0) )
	ROM_LOAD( "tp030_6.bin",  0x100000, 0x100000, CRC(0666fecd) SHA1(aa8f921fc51590b5b05bbe0b0ad0cce5ff359c64) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp030_2.bin", 0x00000, 0x40000, CRC(276146f5) SHA1(bf11d1f6782cefcad77d52af4f7e6054a8f93440) )
ROM_END


ROM_START( snowbro2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "pro-4", 0x000000, 0x080000, CRC(4c7ee341) SHA1(ad46c605a38565d0148daac301be4e4b72302fe7) )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom2-l", 0x000000, 0x100000, CRC(e9d366a9) SHA1(e87e3966fce3395324b90db6c134b3345104c04b) )
	ROM_LOAD( "rom2-h", 0x100000, 0x080000, CRC(9aab7a62) SHA1(611f6a15fdbac5d3063426a365538c1482e996bf) )
	ROM_LOAD( "rom3-l", 0x180000, 0x100000, CRC(eb06e332) SHA1(7cd597bfffc153d178530c0f0903bebd751c9dd1) )
	ROM_LOAD( "rom3-h", 0x280000, 0x080000, CRC(df4a952a) SHA1(b76af61c8437caca573ff1312832898666a611aa) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "rom4", 0x00000, 0x80000, CRC(638f341e) SHA1(aa3fca25f099339ece1878ea730c5e9f18ec4823) )
ROM_END


/* -------------------------- Raizing games ------------------------- */
/* one of these sstriker sets might be bad .. they're very similar    */

ROM_START( sstriker )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "ra-ma-01.01", 0x000000, 0x080000, CRC(92259f84) SHA1(127e62e407d95efd360bfe2cac9577f326abf6ef) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ra_ma_01.02", 0x00000, 0x10000, CRC(eabfa46d) SHA1(402c99ebf88f9025f74f0a28ced22b7882a65eb3) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ra_ma_01.03",  0x000000, 0x100000, CRC(54e2bd95) SHA1(341359dd46152615675bb90e8a184216c8feebff) )
	ROM_LOAD( "ra_ma_01.04",  0x100000, 0x100000, CRC(21cd378f) SHA1(e1695bccec949d18b1c03e9c42dca384554b0d7c) )

	ROM_REGION( 0x008000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ra-ma-01.05",  0x000000, 0x008000, CRC(88b58841) SHA1(1d16b538c11a291bd1f46a510bfbd6259b45a0b5) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "ra_ma_01.06", 0x00000, 0x40000, CRC(6edb2ab8) SHA1(e3032e8eda2686f30df4b7a088c5a4d4d45782ed) )
ROM_END


ROM_START( sstrikra )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "rama1_01.bin", 0x000000, 0x080000, CRC(708fd51d) SHA1(167186d4cf13af37ec0fa6a59c738c54dbbf3c7c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ra_ma_01.02", 0x00000, 0x10000, CRC(eabfa46d) SHA1(402c99ebf88f9025f74f0a28ced22b7882a65eb3) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ra_ma_01.03",  0x000000, 0x100000, CRC(54e2bd95) SHA1(341359dd46152615675bb90e8a184216c8feebff) )
	ROM_LOAD( "ra_ma_01.04",  0x100000, 0x100000, CRC(21cd378f) SHA1(e1695bccec949d18b1c03e9c42dca384554b0d7c) )

	ROM_REGION( 0x008000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ra-ma-01.05",  0x000000, 0x008000, CRC(88b58841) SHA1(1d16b538c11a291bd1f46a510bfbd6259b45a0b5) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "ra_ma_01.06", 0x00000, 0x40000, CRC(6edb2ab8) SHA1(e3032e8eda2686f30df4b7a088c5a4d4d45782ed) )
ROM_END


ROM_START( mahoudai )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "ra_ma_01.01", 0x000000, 0x080000, CRC(970ccc5c) SHA1(c87cab83bde0284e631f02e50068407fee81d941) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ra_ma_01.02", 0x00000, 0x10000, CRC(eabfa46d) SHA1(402c99ebf88f9025f74f0a28ced22b7882a65eb3) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ra_ma_01.03",  0x000000, 0x100000, CRC(54e2bd95) SHA1(341359dd46152615675bb90e8a184216c8feebff) )
	ROM_LOAD( "ra_ma_01.04",  0x100000, 0x100000, CRC(21cd378f) SHA1(e1695bccec949d18b1c03e9c42dca384554b0d7c) )

	ROM_REGION( 0x008000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ra_ma_01.05",  0x000000, 0x008000, CRC(c00d1e80) SHA1(53e64c4c0c6309130b37597d13b44a9e95b717d8) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "ra_ma_01.06", 0x00000, 0x40000, CRC(6edb2ab8) SHA1(e3032e8eda2686f30df4b7a088c5a4d4d45782ed) )
ROM_END


ROM_START( kingdmgp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "ma02rom1.bin", 0x000000, 0x080000, CRC(a678b149) SHA1(8c1a631e023dbba0a3fa6cd1b7d10dec1663213a) )
	ROM_LOAD16_BYTE( "ma02rom0.bin", 0x000001, 0x080000, CRC(f226a212) SHA1(526acf3d05fdc88054a772fbea3de2af532bf3d2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ma02rom2.bin", 0x00000, 0x10000, CRC(dde8a57e) SHA1(f522a3f17e229c71512464349760a9e27778bf6a) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ma02rom3.bin",  0x000000, 0x200000, CRC(0e797142) SHA1(a480ccd151e49b886d3175a6deff56e1f2c26c3e) )
	ROM_LOAD( "ma02rom4.bin",  0x200000, 0x200000, CRC(72a6fa53) SHA1(ce92e65205b84361cfb90305a61e9541b5c4dc2f) )

	ROM_REGION( 0x008000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ma02rom5.eng",  0x000000, 0x008000, CRC(8c28460b) SHA1(0aed170762f6044896a7e608df60bbd37c583a71) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "ma02rom6.bin", 0x00000, 0x80000, CRC(199e7cae) SHA1(0f5e13cc8ec42c80bb4bbff90aba29cdb15213d4) )
ROM_END


ROM_START( shippumd )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "ma02rom1.bin", 0x000000, 0x080000, CRC(a678b149) SHA1(8c1a631e023dbba0a3fa6cd1b7d10dec1663213a) )
	ROM_LOAD16_BYTE( "ma02rom0.bin", 0x000001, 0x080000, CRC(f226a212) SHA1(526acf3d05fdc88054a772fbea3de2af532bf3d2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ma02rom2.bin", 0x00000, 0x10000, CRC(dde8a57e) SHA1(f522a3f17e229c71512464349760a9e27778bf6a) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ma02rom3.bin",  0x000000, 0x200000, CRC(0e797142) SHA1(a480ccd151e49b886d3175a6deff56e1f2c26c3e) )
	ROM_LOAD( "ma02rom4.bin",  0x200000, 0x200000, CRC(72a6fa53) SHA1(ce92e65205b84361cfb90305a61e9541b5c4dc2f) )

	ROM_REGION( 0x008000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ma02rom5.bin",  0x000000, 0x008000, CRC(116ae559) SHA1(4cc2d2a23cc0aefd457111b7990e47184e79204c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "ma02rom6.bin", 0x00000, 0x80000, CRC(199e7cae) SHA1(0f5e13cc8ec42c80bb4bbff90aba29cdb15213d4) )
ROM_END


ROM_START( bgaregcn )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "u123", 0x000000, 0x080000, CRC(88a4e66a) SHA1(ca97e564eed0c5e028b937312e55da56400d5c8c) )
	ROM_LOAD16_BYTE( "u65",  0x000001, 0x080000, CRC(5dea32a3) SHA1(59df6689e3eb5ea9e49a758604d21a64c65ca14d) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.bin", 0x00000, 0x08000, CRC(68632952) SHA1(fb834db83157948e2b420b6051102a9c6ac3969b) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",  0x000000, 0x200000, CRC(b333d81f) SHA1(5481465f1304334fd55798be2f44324c57c2dbcb) )
	ROM_LOAD( "rom3.bin",  0x200000, 0x200000, CRC(51b9ebfb) SHA1(30e0c326f5175aa436df8dba08f6f4e08130b92f) )
	ROM_LOAD( "rom2.bin",  0x400000, 0x200000, CRC(b330e5e2) SHA1(5d48e9d56f99d093b6390e0af1609fd796df2d35) )
	ROM_LOAD( "rom1.bin",  0x600000, 0x200000, CRC(7eafdd70) SHA1(7c8da8e86c3f9491719b1d7d5d285568d7614f38) )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "text.u81", 0x00000, 0x08000, CRC(e67fd534) SHA1(987d0edffc2c243a13d4567319ea3d185eaadbf8) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "rom5.bin", 0x040000, 0x100000, CRC(f6d49863) SHA1(3a3c354852adad06e8a051511abfab7606bce382) )
ROM_END


ROM_START( bgaregt2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "prg0", 0x000000, 0x080000, CRC(84094099) SHA1(49fc68a8bcdae4477e20eade9dd569de88b0b798) )
	ROM_LOAD16_BYTE( "prg1", 0x000001, 0x080000, CRC(46f92fe4) SHA1(62a02cc1dbdc3ac362339aebb62368eb89b06bad) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.bin", 0x00000, 0x08000, CRC(68632952) SHA1(fb834db83157948e2b420b6051102a9c6ac3969b) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",  0x000000, 0x200000, CRC(b333d81f) SHA1(5481465f1304334fd55798be2f44324c57c2dbcb) )
	ROM_LOAD( "rom3.bin",  0x200000, 0x200000, CRC(51b9ebfb) SHA1(30e0c326f5175aa436df8dba08f6f4e08130b92f) )
	ROM_LOAD( "rom2.bin",  0x400000, 0x200000, CRC(b330e5e2) SHA1(5d48e9d56f99d093b6390e0af1609fd796df2d35) )
	ROM_LOAD( "rom1.bin",  0x600000, 0x200000, CRC(7eafdd70) SHA1(7c8da8e86c3f9491719b1d7d5d285568d7614f38) )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "text.u81", 0x00000, 0x08000, CRC(e67fd534) SHA1(987d0edffc2c243a13d4567319ea3d185eaadbf8) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "rom5.bin", 0x040000, 0x100000, CRC(f6d49863) SHA1(3a3c354852adad06e8a051511abfab7606bce382) )
ROM_END


ROM_START( bgaregga )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "prg0.bin", 0x000000, 0x080000, CRC(f80c2fc2) SHA1(a9aac5c7f5439b6fe8d1b3db1fb02a27cc28fdf6) )
	ROM_LOAD16_BYTE( "prg1.bin", 0x000001, 0x080000, CRC(2ccfdd1e) SHA1(7a9f11f851854f3f8389b9c3c0906ebb8dc28712) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.bin", 0x00000, 0x08000, CRC(68632952) SHA1(fb834db83157948e2b420b6051102a9c6ac3969b) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",  0x000000, 0x200000, CRC(b333d81f) SHA1(5481465f1304334fd55798be2f44324c57c2dbcb) )
	ROM_LOAD( "rom3.bin",  0x200000, 0x200000, CRC(51b9ebfb) SHA1(30e0c326f5175aa436df8dba08f6f4e08130b92f) )
	ROM_LOAD( "rom2.bin",  0x400000, 0x200000, CRC(b330e5e2) SHA1(5d48e9d56f99d093b6390e0af1609fd796df2d35) )
	ROM_LOAD( "rom1.bin",  0x600000, 0x200000, CRC(7eafdd70) SHA1(7c8da8e86c3f9491719b1d7d5d285568d7614f38) )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "text.u81", 0x00000, 0x08000, CRC(e67fd534) SHA1(987d0edffc2c243a13d4567319ea3d185eaadbf8) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "rom5.bin", 0x040000, 0x100000, CRC(f6d49863) SHA1(3a3c354852adad06e8a051511abfab7606bce382) )
ROM_END


ROM_START( bgaregnv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "prg_0.bin", 0x000000, 0x080000, CRC(951ecc07) SHA1(a82e4b59e4a974566e59f3ab2fbae1aec7d88a2b) )
	ROM_LOAD16_BYTE( "prg_1.bin", 0x000001, 0x080000, CRC(729a60c6) SHA1(cb6f5d138bb82c32910f42d8ee16fa573a23cef3) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.bin", 0x00000, 0x08000, CRC(68632952) SHA1(fb834db83157948e2b420b6051102a9c6ac3969b) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",  0x000000, 0x200000, CRC(b333d81f) SHA1(5481465f1304334fd55798be2f44324c57c2dbcb) )
	ROM_LOAD( "rom3.bin",  0x200000, 0x200000, CRC(51b9ebfb) SHA1(30e0c326f5175aa436df8dba08f6f4e08130b92f) )
	ROM_LOAD( "rom2.bin",  0x400000, 0x200000, CRC(b330e5e2) SHA1(5d48e9d56f99d093b6390e0af1609fd796df2d35) )
	ROM_LOAD( "rom1.bin",  0x600000, 0x200000, CRC(7eafdd70) SHA1(7c8da8e86c3f9491719b1d7d5d285568d7614f38) )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "text.u81", 0x00000, 0x08000, CRC(e67fd534) SHA1(987d0edffc2c243a13d4567319ea3d185eaadbf8) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "rom5.bin", 0x040000, 0x100000, CRC(f6d49863) SHA1(3a3c354852adad06e8a051511abfab7606bce382) )
ROM_END


ROM_START( bgareghk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "prg_0.rom", 0x000000, 0x080000, CRC(26e0019e) SHA1(5197001f5d59246b137e19ed1952a8207b25d4c0) )
	ROM_LOAD16_BYTE( "prg_1.rom", 0x000001, 0x080000, CRC(2ccfdd1e) SHA1(7a9f11f851854f3f8389b9c3c0906ebb8dc28712) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.bin", 0x00000, 0x08000, CRC(68632952) SHA1(fb834db83157948e2b420b6051102a9c6ac3969b) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",  0x000000, 0x200000, CRC(b333d81f) SHA1(5481465f1304334fd55798be2f44324c57c2dbcb) )
	ROM_LOAD( "rom3.bin",  0x200000, 0x200000, CRC(51b9ebfb) SHA1(30e0c326f5175aa436df8dba08f6f4e08130b92f) )
	ROM_LOAD( "rom2.bin",  0x400000, 0x200000, CRC(b330e5e2) SHA1(5d48e9d56f99d093b6390e0af1609fd796df2d35) )
	ROM_LOAD( "rom1.bin",  0x600000, 0x200000, CRC(7eafdd70) SHA1(7c8da8e86c3f9491719b1d7d5d285568d7614f38) )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "text.u81", 0x00000, 0x08000, CRC(e67fd534) SHA1(987d0edffc2c243a13d4567319ea3d185eaadbf8) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "rom5.bin", 0x040000, 0x100000, CRC(f6d49863) SHA1(3a3c354852adad06e8a051511abfab7606bce382) )
ROM_END

ROM_START( bgaregtw )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "garegga_prg0.u123", 0x000000, 0x080000, CRC(235b7405) SHA1(a2434801df4231a6b48f6c63f47c202d25a89e79) )
	ROM_LOAD16_BYTE( "garegga_prg1.u65",  0x000001, 0x080000, CRC(c29ccf6a) SHA1(38806e0b4ff852f4bfefd80c56ca23f71623e275) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.bin", 0x00000, 0x08000, CRC(68632952) SHA1(fb834db83157948e2b420b6051102a9c6ac3969b) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",  0x000000, 0x200000, CRC(b333d81f) SHA1(5481465f1304334fd55798be2f44324c57c2dbcb) )
	ROM_LOAD( "rom3.bin",  0x200000, 0x200000, CRC(51b9ebfb) SHA1(30e0c326f5175aa436df8dba08f6f4e08130b92f) )
	ROM_LOAD( "rom2.bin",  0x400000, 0x200000, CRC(b330e5e2) SHA1(5d48e9d56f99d093b6390e0af1609fd796df2d35) )
	ROM_LOAD( "rom1.bin",  0x600000, 0x200000, CRC(7eafdd70) SHA1(7c8da8e86c3f9491719b1d7d5d285568d7614f38) )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "text.u81", 0x00000, 0x08000, CRC(e67fd534) SHA1(987d0edffc2c243a13d4567319ea3d185eaadbf8) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "rom5.bin", 0x040000, 0x100000, CRC(f6d49863) SHA1(3a3c354852adad06e8a051511abfab7606bce382) )
ROM_END

ROM_START( batrid )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0_europe.u22", 0x000000, 0x080000, CRC(91d3e975) SHA1(682885fc17f2424d475c282f239f42faf1aae076) )
	ROM_LOAD16_BYTE( "prg1b.u23", 0x000001, 0x080000, CRC(8e70b492) SHA1(f84f2039826ae815afb058d71c1dbd190f9d524d) )
	ROM_LOAD16_BYTE( "prg2.u21" , 0x100000, 0x080000, CRC(bdaa5fbf) SHA1(abd72ac633c0c8e7b4b1d7902c0d6e014ba995fe) )
	ROM_LOAD16_BYTE( "prg3.u24" , 0x100001, 0x080000, CRC(7aa9f941) SHA1(99bdbad7a96d461073b06a53c50fc57c2fd6fc6d) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, CRC(56682696) SHA1(a372450d9a6d535123dfc31d8116074b168ab646) )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, CRC(0df69ca2) SHA1(49670347ebd7e1067ff988cf842b275b7ee7b5f7) )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, CRC(60167d38) SHA1(fd2429808c59ef51fd5f5db84ea89a8dc504186e) )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, CRC(1bfea593) SHA1(ce06dc3097ae56b0df56d104bbf7efc9b5d968d4) )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, CRC(bee03c94) SHA1(5bc1e6769c42857c03456426b502fcb86a114f19) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, CRC(4274daf6) SHA1(85557b4707d529e5914f03c7a856864f5c24950e) )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, CRC(2a1c2426) SHA1(8abc3688ffc5ebb94b8d5118d4fa0908f07fe791) )
ROM_END

ROM_START( batridu )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0_usa.u22", 0x000000, 0x080000, CRC(2049d007) SHA1(f2a43547a6fc5083b03c1d59a85abbf6e1ce4cd9) )
	ROM_LOAD16_BYTE( "prg1b.u23", 0x000001, 0x080000, CRC(8e70b492) SHA1(f84f2039826ae815afb058d71c1dbd190f9d524d) )
	ROM_LOAD16_BYTE( "prg2.u21" , 0x100000, 0x080000, CRC(bdaa5fbf) SHA1(abd72ac633c0c8e7b4b1d7902c0d6e014ba995fe) )
	ROM_LOAD16_BYTE( "prg3.u24" , 0x100001, 0x080000, CRC(7aa9f941) SHA1(99bdbad7a96d461073b06a53c50fc57c2fd6fc6d) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, CRC(56682696) SHA1(a372450d9a6d535123dfc31d8116074b168ab646) )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, CRC(0df69ca2) SHA1(49670347ebd7e1067ff988cf842b275b7ee7b5f7) )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, CRC(60167d38) SHA1(fd2429808c59ef51fd5f5db84ea89a8dc504186e) )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, CRC(1bfea593) SHA1(ce06dc3097ae56b0df56d104bbf7efc9b5d968d4) )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, CRC(bee03c94) SHA1(5bc1e6769c42857c03456426b502fcb86a114f19) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, CRC(4274daf6) SHA1(85557b4707d529e5914f03c7a856864f5c24950e) )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, CRC(2a1c2426) SHA1(8abc3688ffc5ebb94b8d5118d4fa0908f07fe791) )
ROM_END

ROM_START( batridc )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0_china.u22", 0x000000, 0x080000, CRC(c3b91f7e) SHA1(6b2376c37808dccda296d90ccd7f577ccff4e4dc) )
	ROM_LOAD16_BYTE( "prg1b.u23", 0x000001, 0x080000, CRC(8e70b492) SHA1(f84f2039826ae815afb058d71c1dbd190f9d524d) )
	ROM_LOAD16_BYTE( "prg2.u21" , 0x100000, 0x080000, CRC(bdaa5fbf) SHA1(abd72ac633c0c8e7b4b1d7902c0d6e014ba995fe) )
	ROM_LOAD16_BYTE( "prg3.u24" , 0x100001, 0x080000, CRC(7aa9f941) SHA1(99bdbad7a96d461073b06a53c50fc57c2fd6fc6d) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, CRC(56682696) SHA1(a372450d9a6d535123dfc31d8116074b168ab646) )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, CRC(0df69ca2) SHA1(49670347ebd7e1067ff988cf842b275b7ee7b5f7) )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, CRC(60167d38) SHA1(fd2429808c59ef51fd5f5db84ea89a8dc504186e) )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, CRC(1bfea593) SHA1(ce06dc3097ae56b0df56d104bbf7efc9b5d968d4) )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, CRC(bee03c94) SHA1(5bc1e6769c42857c03456426b502fcb86a114f19) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, CRC(4274daf6) SHA1(85557b4707d529e5914f03c7a856864f5c24950e) )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, CRC(2a1c2426) SHA1(8abc3688ffc5ebb94b8d5118d4fa0908f07fe791) )
ROM_END

ROM_START( batridj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0b.u22", 0x000000, 0x080000, CRC(4f3fc729) SHA1(b32d51c254741b82171a86c271679522a7aefd34) )
	ROM_LOAD16_BYTE( "prg1b.u23", 0x000001, 0x080000, CRC(8e70b492) SHA1(f84f2039826ae815afb058d71c1dbd190f9d524d) )
	ROM_LOAD16_BYTE( "prg2.u21" , 0x100000, 0x080000, CRC(bdaa5fbf) SHA1(abd72ac633c0c8e7b4b1d7902c0d6e014ba995fe) )
	ROM_LOAD16_BYTE( "prg3.u24" , 0x100001, 0x080000, CRC(7aa9f941) SHA1(99bdbad7a96d461073b06a53c50fc57c2fd6fc6d) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, CRC(56682696) SHA1(a372450d9a6d535123dfc31d8116074b168ab646) )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, CRC(0df69ca2) SHA1(49670347ebd7e1067ff988cf842b275b7ee7b5f7) )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, CRC(60167d38) SHA1(fd2429808c59ef51fd5f5db84ea89a8dc504186e) )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, CRC(1bfea593) SHA1(ce06dc3097ae56b0df56d104bbf7efc9b5d968d4) )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, CRC(bee03c94) SHA1(5bc1e6769c42857c03456426b502fcb86a114f19) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, CRC(4274daf6) SHA1(85557b4707d529e5914f03c7a856864f5c24950e) )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, CRC(2a1c2426) SHA1(8abc3688ffc5ebb94b8d5118d4fa0908f07fe791) )
ROM_END

ROM_START( batridk )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0_korea.u22", 0x000000, 0x080000, CRC(d9d8c907) SHA1(69c197f2a41f288913f042de9eb8274c0df3ac27) )
	ROM_LOAD16_BYTE( "prg1.u23", 0x000001, 0x080000, CRC(8e70b492) SHA1(f84f2039826ae815afb058d71c1dbd190f9d524d) )
	ROM_LOAD16_BYTE( "prg2.u21", 0x100000, 0x080000, CRC(bdaa5fbf) SHA1(abd72ac633c0c8e7b4b1d7902c0d6e014ba995fe) )
	ROM_LOAD16_BYTE( "prg3.u24", 0x100001, 0x080000, CRC(7aa9f941) SHA1(99bdbad7a96d461073b06a53c50fc57c2fd6fc6d) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, CRC(56682696) SHA1(a372450d9a6d535123dfc31d8116074b168ab646) )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, CRC(0df69ca2) SHA1(49670347ebd7e1067ff988cf842b275b7ee7b5f7) )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, CRC(60167d38) SHA1(fd2429808c59ef51fd5f5db84ea89a8dc504186e) )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, CRC(1bfea593) SHA1(ce06dc3097ae56b0df56d104bbf7efc9b5d968d4) )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, CRC(bee03c94) SHA1(5bc1e6769c42857c03456426b502fcb86a114f19) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, CRC(4274daf6) SHA1(85557b4707d529e5914f03c7a856864f5c24950e) )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, CRC(2a1c2426) SHA1(8abc3688ffc5ebb94b8d5118d4fa0908f07fe791) )
ROM_END

/* Version A is older, might have been only released in Japan and Taiwan? */
ROM_START( batridja )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0.bin", 0x000000, 0x080000, CRC(f93ea27c) SHA1(41023c2ee1efd70b5aa9c70e1ddd9e5c3d51d68a) )
	ROM_LOAD16_BYTE( "prg1.bin", 0x000001, 0x080000, CRC(8ae7f592) SHA1(8a20ebf85eca621f578d2302c3a3988647b077a7) )
	ROM_LOAD16_BYTE( "prg2.u21", 0x100000, 0x080000, CRC(bdaa5fbf) SHA1(abd72ac633c0c8e7b4b1d7902c0d6e014ba995fe) )
	ROM_LOAD16_BYTE( "prg3.u24", 0x100001, 0x080000, CRC(7aa9f941) SHA1(99bdbad7a96d461073b06a53c50fc57c2fd6fc6d) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, CRC(56682696) SHA1(a372450d9a6d535123dfc31d8116074b168ab646) )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, CRC(0df69ca2) SHA1(49670347ebd7e1067ff988cf842b275b7ee7b5f7) )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, CRC(60167d38) SHA1(fd2429808c59ef51fd5f5db84ea89a8dc504186e) )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, CRC(1bfea593) SHA1(ce06dc3097ae56b0df56d104bbf7efc9b5d968d4) )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, CRC(bee03c94) SHA1(5bc1e6769c42857c03456426b502fcb86a114f19) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, CRC(4274daf6) SHA1(85557b4707d529e5914f03c7a856864f5c24950e) )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, CRC(2a1c2426) SHA1(8abc3688ffc5ebb94b8d5118d4fa0908f07fe791) )
ROM_END

ROM_START( batridta )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "u22.bin",  0x000000, 0x080000, CRC(b135820e) SHA1(c222887d18a0a3ea0fcc973b95b29d69c86f7ec3) )
	ROM_LOAD16_BYTE( "u23.bin",  0x000001, 0x080000, CRC(8ae7f592) SHA1(8a20ebf85eca621f578d2302c3a3988647b077a7) )
	ROM_LOAD16_BYTE( "prg2.u21", 0x100000, 0x080000, CRC(bdaa5fbf) SHA1(abd72ac633c0c8e7b4b1d7902c0d6e014ba995fe) )
	ROM_LOAD16_BYTE( "prg3.u24", 0x100001, 0x080000, CRC(7aa9f941) SHA1(99bdbad7a96d461073b06a53c50fc57c2fd6fc6d) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, CRC(56682696) SHA1(a372450d9a6d535123dfc31d8116074b168ab646) )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, CRC(0df69ca2) SHA1(49670347ebd7e1067ff988cf842b275b7ee7b5f7) )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, CRC(60167d38) SHA1(fd2429808c59ef51fd5f5db84ea89a8dc504186e) )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, CRC(1bfea593) SHA1(ce06dc3097ae56b0df56d104bbf7efc9b5d968d4) )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, CRC(bee03c94) SHA1(5bc1e6769c42857c03456426b502fcb86a114f19) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, CRC(4274daf6) SHA1(85557b4707d529e5914f03c7a856864f5c24950e) )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, CRC(2a1c2426) SHA1(8abc3688ffc5ebb94b8d5118d4fa0908f07fe791) )
ROM_END

ROM_START( bkraidu )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0u022_usa.bin", 0x000000, 0x080000, CRC(95fb2ffd) SHA1(c7f502f3945249573b66226e8bacc6a9bc230693) )
	ROM_LOAD16_BYTE( "prg1u023.new", 0x000001, 0x080000, CRC(4ae9aa64) SHA1(45fdf72141c4c9f24a38d4218c65874799b9c868) )
	ROM_LOAD16_BYTE( "prg2u021.bin", 0x100000, 0x080000, CRC(ffba8656) SHA1(6526bb65fad3384de3f301a7d1095cbf03757433) )
	ROM_LOAD16_BYTE( "prg3u024.bin", 0x100001, 0x080000, CRC(834b8ad6) SHA1(0dd6223bb0749819ad29811eeb04fd08d937abb0) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "sndu0720.bin", 0x00000, 0x08000, CRC(e62ab246) SHA1(00d23689dd423ecd4024c58b5903d16e890f1dff) )
	ROM_CONTINUE(             0x10000, 0x18000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gfxu0510.bin", 0x000000, 0x400000, CRC(9cca3446) SHA1(1123f8b8bfbe59a2c572cdf61f1ad27ff37f0f0d) )
	ROM_LOAD( "gfxu0512.bin", 0x400000, 0x400000, CRC(a2a281d5) SHA1(d9a6623f9433ad682223f9780c26cd1523ebc5c5) )
	ROM_LOAD( "gfxu0511.bin", 0x800000, 0x400000, CRC(e16472c0) SHA1(6068d679a8b3b65e05acd58a7ce9ead90177049f) )
	ROM_LOAD( "gfxu0513.bin", 0xc00000, 0x400000, CRC(8bb635a0) SHA1(9064f1a2d8bb88ddbca702fb8556d0dfe6a5cadc) )

	ROM_REGION( 0x0c00000, REGION_SOUND1, 0 )		/* YMZ280B Samples */
	ROM_LOAD( "rom6.829", 0x000000, 0x400000, CRC(8848b4a0) SHA1(e0dce136c5d5a4c1a92b863e57848cd5927d06f1) )
	ROM_LOAD( "rom7.830", 0x400000, 0x400000, CRC(d6224267) SHA1(5c9b7b13effbef9f707811f84bfe50ca85e605e3) )
	ROM_LOAD( "rom8.831", 0x800000, 0x400000, CRC(a101dfb0) SHA1(4b729b0d562e09df35438e9e6b457b8de2690a6e) )
ROM_END

ROM_START( bkraidj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0u022.bin", 0x000000, 0x080000, CRC(0dd59512) SHA1(c6a4e6aa49c6ac3b04ae62a0a4cc8084ae048381) )
	ROM_LOAD16_BYTE( "prg1u023.bin", 0x000001, 0x080000, CRC(fecde223) SHA1(eb5ac0eda49b4b0f3d25d8a8bb356e77a453d3a7) )
	ROM_LOAD16_BYTE( "prg2u021.bin", 0x100000, 0x080000, CRC(ffba8656) SHA1(6526bb65fad3384de3f301a7d1095cbf03757433) )
	ROM_LOAD16_BYTE( "prg3u024.bin", 0x100001, 0x080000, CRC(834b8ad6) SHA1(0dd6223bb0749819ad29811eeb04fd08d937abb0) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "sndu0720.bin", 0x00000, 0x08000, CRC(e62ab246) SHA1(00d23689dd423ecd4024c58b5903d16e890f1dff) )
	ROM_CONTINUE(             0x10000, 0x18000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gfxu0510.bin", 0x000000, 0x400000, CRC(9cca3446) SHA1(1123f8b8bfbe59a2c572cdf61f1ad27ff37f0f0d) )
	ROM_LOAD( "gfxu0512.bin", 0x400000, 0x400000, CRC(a2a281d5) SHA1(d9a6623f9433ad682223f9780c26cd1523ebc5c5) )
	ROM_LOAD( "gfxu0511.bin", 0x800000, 0x400000, CRC(e16472c0) SHA1(6068d679a8b3b65e05acd58a7ce9ead90177049f) )
	ROM_LOAD( "gfxu0513.bin", 0xc00000, 0x400000, CRC(8bb635a0) SHA1(9064f1a2d8bb88ddbca702fb8556d0dfe6a5cadc) )

	ROM_REGION( 0x0c00000, REGION_SOUND1, 0 )		/* YMZ280B Samples */
	ROM_LOAD( "rom6.829", 0x000000, 0x400000, CRC(8848b4a0) SHA1(e0dce136c5d5a4c1a92b863e57848cd5927d06f1) )
	ROM_LOAD( "rom7.830", 0x400000, 0x400000, CRC(d6224267) SHA1(5c9b7b13effbef9f707811f84bfe50ca85e605e3) )
	ROM_LOAD( "rom8.831", 0x800000, 0x400000, CRC(a101dfb0) SHA1(4b729b0d562e09df35438e9e6b457b8de2690a6e) )
ROM_END

ROM_START( bkraiduj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0u022.new", 0x000000, 0x080000, CRC(fa8d38d3) SHA1(aba91d87a8a62d3fe1139b4437b16e2f844264ad) )
	ROM_LOAD16_BYTE( "prg1u023.new", 0x000001, 0x080000, CRC(4ae9aa64) SHA1(45fdf72141c4c9f24a38d4218c65874799b9c868) )
	ROM_LOAD16_BYTE( "prg2u021.bin", 0x100000, 0x080000, CRC(ffba8656) SHA1(6526bb65fad3384de3f301a7d1095cbf03757433) )
	ROM_LOAD16_BYTE( "prg3u024.bin", 0x100001, 0x080000, CRC(834b8ad6) SHA1(0dd6223bb0749819ad29811eeb04fd08d937abb0) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "sndu0720.bin", 0x00000, 0x08000, CRC(e62ab246) SHA1(00d23689dd423ecd4024c58b5903d16e890f1dff) )
	ROM_CONTINUE(             0x10000, 0x18000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gfxu0510.bin", 0x000000, 0x400000, CRC(9cca3446) SHA1(1123f8b8bfbe59a2c572cdf61f1ad27ff37f0f0d) )
	ROM_LOAD( "gfxu0512.bin", 0x400000, 0x400000, CRC(a2a281d5) SHA1(d9a6623f9433ad682223f9780c26cd1523ebc5c5) )
	ROM_LOAD( "gfxu0511.bin", 0x800000, 0x400000, CRC(e16472c0) SHA1(6068d679a8b3b65e05acd58a7ce9ead90177049f) )
	ROM_LOAD( "gfxu0513.bin", 0xc00000, 0x400000, CRC(8bb635a0) SHA1(9064f1a2d8bb88ddbca702fb8556d0dfe6a5cadc) )

	ROM_REGION( 0x0c00000, REGION_SOUND1, 0 )		/* YMZ280B Samples */
	ROM_LOAD( "rom6.829", 0x000000, 0x400000, CRC(8848b4a0) SHA1(e0dce136c5d5a4c1a92b863e57848cd5927d06f1) )
	ROM_LOAD( "rom7.830", 0x400000, 0x400000, CRC(d6224267) SHA1(5c9b7b13effbef9f707811f84bfe50ca85e605e3) )
	ROM_LOAD( "rom8.831", 0x800000, 0x400000, CRC(a101dfb0) SHA1(4b729b0d562e09df35438e9e6b457b8de2690a6e) )
ROM_END



/* The following is in order of Toaplan Board/game numbers */
/* See list at top of file */
/* Whoopee machine to be changed to Teki Paki when (if) HD647180 is dumped */
/* Whoopee  init   to be changed to T2_Z180   when (if) HD647180 is dumped */

/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR COMPANY    FULLNAME     FLAGS ) */
GAME( 1991, tekipaki, 0,        tekipaki, tekipaki, T2_Z180,  ROT0,   "Toaplan", "Teki Paki", 0 )
GAME( 1991, ghox,     0,        ghox,     ghox,     T2_Z180,  ROT270, "Toaplan", "Ghox", 0 )
GAME( 1991, ghoxj,    ghox,     ghox,     ghox,     T2_Z180,  ROT270, "Toaplan", "Ghox (Joystick version)", 0 )
GAME( 1992, dogyuun,  0,        dogyuun,  dogyuun,  T2_V25,   ROT270, "Toaplan", "Dogyuun", 0 )
GAME( 1992, dogyuunto,dogyuun,  dogyuunto,dogyuun,  T2_Z80,   ROT270, "Toaplan", "Dogyuun (8/25/1992 location test)", 0 )
GAME( 1993, kbash,    0,        kbash,    kbash,    T2_V25,   ROT0,   "Toaplan", "Knuckle Bash", GAME_IMPERFECT_SOUND )
GAME( 1999, kbash2,   0,        kbash2,   kbash2,   T2_noZ80, ROT0,   "bootleg", "Knuckle Bash 2 (bootleg)", 0 )
GAME( 1992, truxton2, 0,        truxton2, truxton2, T2_noZ80, ROT270, "Toaplan", "Truxton II / Tatsujin II / Tatsujin Oh (Japan)", 0 )
GAME( 1991, pipibibs, 0,        pipibibs, pipibibs, T2_Z80,   ROT0,   "Toaplan", "Pipi & Bibis / Whoopee!!", 0 )
GAME( 1991, whoopee,  pipibibs, tekipaki, whoopee,  T2_Z180,  ROT0,   "Toaplan", "Whoopee!! / Pipi & Bibis", 0 )
GAME( 1991, pipibibi, pipibibs, pipibibi, pipibibi, pipibibi, ROT0,   "[Toaplan] Ryouta Kikaku", "Pipi & Bibis / Whoopee!! (bootleg ?)", 0 )
GAME( 1992, fixeight, 0,        fixeight, fixeight, fixeight, ROT270, "Toaplan", "FixEight", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )
GAME( 1992, fixeighb, fixeight, fixeighb, fixeighb, fixeighb, ROT270, "bootleg", "FixEight (bootleg)", 0 )
GAME( 1992, grindstm, vfive,    vfive,    grindstm, T2_V25,   ROT270, "Toaplan", "Grind Stormer", 0 )
GAME( 1992, grindsta, vfive,    vfive,    grindstm, T2_V25,   ROT270, "Toaplan", "Grind Stormer (older set)", 0 )
GAME( 1993, vfive,    0,        vfive,    vfive,    T2_V25,   ROT270, "Toaplan", "V-Five (Japan)", 0 )
GAME( 1993, batsugun, 0,        batsugun, batsugun, T2_V25,   ROT270, "Toaplan", "Batsugun (set 1)", GAME_IMPERFECT_GRAPHICS )
GAME( 1993, batsugna, batsugun, batsugun, batsugun, T2_V25,   ROT270, "Toaplan", "Batsugun (set 2)", GAME_IMPERFECT_GRAPHICS )
GAME( 1993, batugnsp, batsugun, batsugun, batsugun, T2_V25,   ROT270, "Toaplan", "Batsugun (Special Ver.)", GAME_IMPERFECT_GRAPHICS )
GAME( 1994, snowbro2, 0,        snowbro2, snowbro2, T2_noZ80, ROT0,   "[Toaplan] Hanafram", "Snow Bros. 2 - With New Elves / Otenki Paradise", 0 )
GAME( 1993, mahoudai, 0,        mahoudai, mahoudai, T2_Z80,   ROT270, "Raizing (Able license)", "Mahou Daisakusen (Japan)", 0 )
GAME( 1993, sstriker, mahoudai, mahoudai, sstriker, T2_Z80,   ROT270, "Raizing", "Sorcer Striker (World)" , 0) // from korean board
GAME( 1993, sstrikra, mahoudai, mahoudai, sstriker, T2_Z80,   ROT270, "Raizing", "Sorcer Striker (World, alt)" , 0) // from korean board
GAME( 1994, shippumd, 0,        shippumd, shippumd, T2_Z80,   ROT270, "Raizing / Eighting", "Shippu Mahou Daisakusen (Japan)", 0 )
GAME( 1994, kingdmgp, shippumd, shippumd, kingdmgp, T2_Z80,   ROT270, "Raizing / Eighting", "Kingdom Grandprix (World)" , 0) // from korean board, missing letters on credits screen but this is correct
GAME( 1996, bgaregga, 0,        battleg,  battlega, battleg,  ROT270, "Raizing / Eighting", "Battle Garegga (Europe / USA / Japan / Asia) (Sat Feb 3 1996)", 0 )
GAME( 1996, bgareghk, bgaregga, battleg,  battlegb, battleg,  ROT270, "Raizing / Eighting", "Battle Garegga (Austria / Hong Kong) (Sat Feb 3 1996)", 0 )
GAME( 1996, bgaregtw, bgaregga, battleg,  battlegb, battleg,  ROT270, "Raizing / Eighting", "Battle Garegga (Taiwan / Germany) (Thu Feb 1 1996)", 0 )
GAME( 1996, bgaregnv, bgaregga, battleg,  battlegb, battleg,  ROT270, "Raizing / Eighting", "Battle Garegga - New Version (Austria / Hong Kong) (Sat Mar 2 1996)" , 0) // displays New Version when set to HK
GAME( 1996, bgaregt2, bgaregga, battleg,  battleg,  battleg,  ROT270, "Raizing / Eighting", "Battle Garegga - Type 2 (Europe / USA / Japan / Asia) (Sat Mar 2 1996)", 0 )
GAME( 1996, bgaregcn, bgaregga, battleg,  battleg,  battleg,  ROT270, "Raizing / Eighting", "Battle Garegga - Type 2 (Denmark / China) (Tue Apr 2 1996)", 0 )
GAME( 1998, batrid,   0,        batrider, batrider, battleg,  ROT270, "Raizing / Eighting", "Armed Police Batrider (Europe) (Fri Feb 13 1998)", 0 )
GAME( 1998, batridu,  batrid,   batrider, batrider, battleg,  ROT270, "Raizing / Eighting", "Armed Police Batrider (U.S.A.) (Fri Feb 13 1998)", 0 )
GAME( 1998, batridc,  batrid,   batrider, batrider, battleg,  ROT270, "Raizing / Eighting", "Armed Police Batrider (China) (Fri Feb 13 1998)", 0 )
GAME( 1998, batridj,  batrid,   batrider, batridrj, battleg,  ROT270, "Raizing / Eighting", "Armed Police Batrider - B Version (Japan) (Fri Feb 13 1998)", 0 )
GAME( 1998, batridk,  batrid,   batrider, batrider, battleg,  ROT270, "Raizing / Eighting", "Armed Police Batrider (Korea) (Fri Feb 13 1998)", 0 )
GAME( 1998, batridja, batrid,   batrider, batridrj, battleg,  ROT270, "Raizing / Eighting", "Armed Police Batrider - A Version (Japan) (Mon Dec 22 1997)", 0 )
GAME( 1998, batridta, batrid,   batrider, batridrj, battleg,  ROT270, "Raizing / Eighting", "Armed Police Batrider - A Version (Taiwan) (Mon Dec 22 1997)", 0 )
GAME( 1999, bkraidu,  0,        bbakraid, bbakraid, bbakradu, ROT270, "Eighting", "Battle Bakraid - Unlimited Version (U.S.A.) (Tue Jun 8 1999)", 0 )
GAME( 1999, bkraiduj, bkraidu,  bbakraid, bbakraid, bbakradu, ROT270, "Eighting", "Battle Bakraid - Unlimited Version (Japan) (Tue Jun 8 1999)", 0 )
GAME( 1999, bkraidj,  bkraidu,  bbakraid, bbakraid, bbakraid, ROT270, "Eighting", "Battle Bakraid (Japan) (Wed Apr 7 1999)", 0 )

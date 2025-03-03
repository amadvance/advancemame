/********************************************************************

Task Force Harrier       1989 UPL        68000 Z80           YM2203 2xOKIM6295
Many Block               1991 Bee-Oh     68000 Z80           YM2203 2xOKIM6295
Mustang                  1990 UPL        68000 NMK004        YM2203 2xOKIM6295
Bio-ship Paladin         1990 UPL        68000 <unknown cpu> YM2203 2xOKIM6295
Vandyke                  1990 UPL        68000 NMK004        YM2203 2xOKIM6295
Black Heart              1991 UPL        68000 NMK004        YM2203 2xOKIM6295
Acrobat Mission          1991 UPL        68000 NMK004        YM2203 2xOKIM6295
Strahl                   1992 UPL        68000 <unknown cpu> YM2203 2xOKIM6295
Thunder Dragon           1991 NMK/Tecmo  68000 <unknown cpu> YM2203 2xOKIM6295
Hacha Mecha Fighter      1991 NMK        68000 <unknown cpu> YM2203 2xOKIM6295
Macross                  1992 Banpresto  68000 <unknown cpu> YM2203 2xOKIM6295
GunNail                  1993 NMK/Tecmo  68000 NMK004        YM2203 2xOKIM6295
Macross II               1993 Banpresto  68000 Z80           YM2203 2xOKIM6295
Thunder Dragon 2         1993 NMK        68000 Z80           YM2203 2xOKIM6295
Rapid Hero               1994 NMK        68000 tmp90c841     YM2203 2xOKIM6295

S.S. Mission             1992 Comad      68000 Z80           OKIM6295
Air Attack               1996 Comad      68000 Z80           OKIM6295

Mustang (bootleg)                        68000 Z80           YM3812 OKIM6295
Thunder Dragon (bootleg)                 68000 Z80           YM3812 OKIM6295

Saboten Bombers          1992 NMK/Tecmo  68000               2xOKIM6295
Bombjack Twin            1993 NMK        68000               2xOKIM6295
Nouryoku Koujou Iinkai   1995 Tecmo      68000               2xOKIM6295

driver by Mirko Buffoni, Richard Bush, Nicola Salmoria, Bryan McPhail,
          David Haywood, and R. Belmont.

The NMK004 CPU might be a Toshiba TLCS-90 class CPU with internal ROM in the
0000-1fff range.

The later games (from GunNail onwards) have an higher resolution (384x224 instead
of 256x224) but the hardware is pretty much the same. It's obvious that the higher
res is an afterthought, because the tilemap layout is weird (the left 8 screen
columns have to be taken from the rightmost 8 columns of the tilemap), and the
games rely on mirror addresses to access the tilemap sequentially.

TODO:
- NMK004 sound CPU is just (imperfectly) simulated for now.
- There is a handshaking operation happening on boot in most games. It happens on
  the NMK004 communication ports so I have implemented it in NMK004.c. However,
  the same handshaking also happen in tharrier, which doesn't have a NMK004!
  Therefore, it might be another protection device, which sits in the middle
  between CPU and NMK004.
- Protection is patched in several games.
- In hachamf it seems that the protection device shares some RAM (fe000-fefff)
  with the main CPU, and the main CPU fetches pointers from that shared RAM to
  do important operations like reading the input ports. Some of them are easily
  deduced checking for similarities in macross and bjtwin; however another
  protection check involves (see the routine at 01429a) writing data to the
  fe100-fe1ff range, and then jumping to subroutines in that range (most likely
  function pointers since each one is only 0x10 bytes long), and heaven knows
  what those should do.
  On startup, hachamf does a RAM test, then copies some stuff and jumps to
  RAM at 0xfef00, where it sits in a loop. We patch around that by replacing the
  reset vector with the "real" one.
  update: simulated this,see hachamf_mcu_shared_w() & tdragon_mcu_shared_w() for
  more info about it.
- Hacha Mecha Fighter bg graphics are completely wrong except at the title screen &
  the level 7.Likely to be a rom issue,the game activates the bgbank
  when it is on the above two cases.Also the bomb graphics are wrong when the game
  is in japanese mode...
- Cocktail mode is supported, but tilemap.c has problems with asymmetrical
  visible areas.
- Music timing in nouryoku is a little off.
- In Bioship, there's an occasional flicker of one of the sprites composing big
  ships. Increasing CPU speed from 12 to 16 MHz improved it, but it's still not
  100% fixed.
- Input ports in Bio-ship Paladin, Strahl
- Sound communication in Mustang might be incorrectly implemented

----

IRQ1 controls audio output and coin/joysticks reads
IRQ4 controls DSW reads and vblank.
IRQ2 points to RTE (not used).

----

tdragon,mustang and hachamf test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)  Press player 1 button 2 14 (!) times

gunnail test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)  Press player 2 button 1 3 times

bjtwin test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)  Press player 1 buttons in this sequence:
    2,2,2, 1,1,1, 2,2,2, 1,1,1
    The release date of this program will appear.

Some code has to be patched out for this to work (see below). The program
remaps button 2 and 3 to button 1, so you can't enter the above sequence.

---

Questions / Notes

'manybloc' :

  - There are writes to 0x080010.w and 0x080012.w (MCU ?) in code between
    0x005000 to 0x005690, but I see no call to "main" routine at 0x005504 !
  - There are writes to 0x08001c.w and 0x08001e.w but I can't tell what
    the effect is ! Could it be related to sound and/or interrupts ?

  - In the "test mode", press BOTH player 1 buttons to exit

  - When help is available, press BUTTON2 twice within the timer to "solve"

---

Sound notes for games with a Z80:

mustangb and tdragonb use the Seibu Raiden sound hardware and a modified
Z80 program (but the music is intact and recognizable).  See sndhrdw/seibu.c
for more info on this.

********************************************************************/

#include "driver.h"
#include "sndhrdw/seibu.h"
#include "sound/2203intf.h"
#include "sound/okim6295.h"
#include "sound/3812intf.h"
#include "machine/nmk004.h"
#include "machine/nmk112.h"


extern UINT16 *nmk_bgvideoram,*nmk_fgvideoram,*nmk_txvideoram;
extern UINT16 *gunnail_scrollram;
extern UINT16 tharrier_scroll;

READ16_HANDLER( nmk_bgvideoram_r );
WRITE16_HANDLER( nmk_bgvideoram_w );
READ16_HANDLER( nmk_fgvideoram_r );
WRITE16_HANDLER( nmk_fgvideoram_w );
READ16_HANDLER( nmk_txvideoram_r );
WRITE16_HANDLER( nmk_txvideoram_w );
WRITE16_HANDLER( nmk_scroll_w );
WRITE16_HANDLER( nmk_scroll_2_w );
WRITE16_HANDLER( nmk_scroll_3_w );
WRITE16_HANDLER( gunnail_scrollx_w );
WRITE16_HANDLER( gunnail_scrolly_w );
WRITE16_HANDLER( nmk_flipscreen_w );
WRITE16_HANDLER( nmk_tilebank_w );
WRITE16_HANDLER( bioship_scroll_w );
WRITE16_HANDLER( bioship_bank_w );
WRITE16_HANDLER( mustang_scroll_w );
WRITE16_HANDLER( vandyke_scroll_w );
WRITE16_HANDLER( manybloc_scroll_w );

VIDEO_START( macross );
VIDEO_UPDATE( manybloc );
VIDEO_START( gunnail );
VIDEO_START( macross2 );
VIDEO_START( tdragon2 );
VIDEO_START( bjtwin );
VIDEO_START( bioship );
VIDEO_START( strahl );
VIDEO_UPDATE( bioship );
VIDEO_UPDATE( strahl );
VIDEO_UPDATE( macross );
VIDEO_UPDATE( gunnail );
VIDEO_UPDATE( bjtwin );
VIDEO_UPDATE( tharrier );
VIDEO_UPDATE( hachamf );
VIDEO_EOF( nmk );
VIDEO_EOF( strahl );

/* nmk16 protection */
UINT16 *nmk16_mainram;

static WRITE16_HANDLER( nmk16_mainram_strange_w )
{
	UINT16 * dstram;

 	dstram = nmk16_mainram;

	if (!ACCESSING_MSB)
	{
		dstram[offset] = (data & 0x00ff) |  ((data & 0x00ff)<<8);
 	}
	else if (!ACCESSING_LSB)
	{
		dstram[offset] = (data & 0xff00) |  ((data & 0xff00)>>8);
	}
	else
  	{
		dstram[offset] = data;
	}
}

static READ16_HANDLER( tharrier_mcu_r )
{
	/* The MCU is mapped as the top byte for byte accesses only,
        all word accesses are to the input port */
	if (ACCESSING_MSB && !ACCESSING_LSB)
	{
		static const UINT8 to_main[] =
		{
			0x82,0xc7,0x00,0x2c,0x6c,0x00,0x9f,0xc7,0x00,0x29,0x69,0x00,0x8b,0xc7,0x00
		};
		static int prot_count;

		int res;

		if (activecpu_get_pc()==0x8aa) res = (nmk16_mainram[0x9064/2])|0x20; /* Task Force Harrier */
		else if (activecpu_get_pc()==0x8ce) res = (nmk16_mainram[0x9064/2])|0x60; /* Task Force Harrier */
		else
		{
			res = to_main[prot_count++];
			if (prot_count > sizeof(to_main))
				prot_count = 0;
		}

		return res << 8;
	}
	else
		return ~input_port_1_word_r(0,0);
}

static WRITE16_HANDLER( tharrier_mcu_control_w )
{
//	logerror("%04x: mcu_control_w %02x\n",activecpu_get_pc(),data);
}




static MACHINE_RESET( nmk16 )
{
}

static MACHINE_RESET( NMK004 )
{
	NMK004_init();
}

static MACHINE_RESET( mustang_sound )
{
	machine_reset_seibu_sound_1();
}

WRITE16_HANDLER ( ssmissin_sound_w )
{
	/* maybe .. */
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpunum_set_input_line(1,0, ASSERT_LINE);
	}

	if (ACCESSING_MSB)
		if ((data >> 8) & 0x80)
			cpunum_set_input_line(1,0, CLEAR_LINE);
}



WRITE8_HANDLER ( ssmissin_soundbank_w )
{
	unsigned char *rom = memory_region(REGION_SOUND1);
	int bank;

	bank = data & 0x3;

	memcpy(rom + 0x20000,rom + 0x80000 + bank * 0x20000,0x20000);
}

static WRITE16_HANDLER( macross2_sound_reset_w )
{
	/* PCB behaviour verified by Corrado Tomaselli at MAME Italia Forum:
	   every time music changes Z80 is resetted */
	cpunum_set_input_line(1, INPUT_LINE_RESET, data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE16_HANDLER( macross2_sound_command_w )
{
	if (ACCESSING_LSB)
	   soundlatch_w(0,data & 0xff);
}


static WRITE8_HANDLER( macross2_sound_bank_w )
{
	UINT8 *rom = memory_region(REGION_CPU2) + 0x10000;

	memory_set_bankptr(1,rom + (data & 0x07) * 0x4000);
}




/***************************************************************************/

static ADDRESS_MAP_START( vandyke_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000a, 0x08000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READ(NMK004_r)
	AM_RANGE(0x088000, 0x0887ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09d000, 0x09d7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vandyke_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080016, 0x080017) AM_WRITE(MWA16_NOP)	/* IRQ enable? */
	AM_RANGE(0x080018, 0x080019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x088000, 0x0887ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c007) AM_WRITE(vandyke_scroll_w)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
//	AM_RANGE(0x094000, 0x097fff) AM_WRITE(MWA16_RAM) /* what is this */
	AM_RANGE(0x09d000, 0x09d7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram) /* not tested in tests .. hardly used probably some registers not ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( manybloc_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080004, 0x080005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08001e, 0x08001f) AM_READ(soundlatch2_word_r)
	AM_RANGE(0x088000, 0x0883ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09c000, 0x09cfff) AM_READ(MRA16_RAM) /* Scroll? */
	AM_RANGE(0x09d000, 0x09d7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( manybloc_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080010, 0x080011) AM_WRITE(MWA16_NOP)			/* See notes at the top of the driver */
	AM_RANGE(0x080012, 0x080013) AM_WRITE(MWA16_NOP)			/* See notes at the top of the driver */
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x08001c, 0x08001d) AM_WRITE(MWA16_NOP)			/* See notes at the top of the driver */
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(soundlatch_word_w)
	AM_RANGE(0x088000, 0x0883ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
    AM_RANGE(0x09c000, 0x09cfff) AM_WRITE(manybloc_scroll_w) AM_BASE(&gunnail_scrollram)
	AM_RANGE(0x09d000, 0x09d7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( manybloc_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_READ(soundlatch_r)
	AM_RANGE(0xf400, 0xf400) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0xf500, 0xf500) AM_READ(OKIM6295_status_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( manybloc_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(soundlatch2_w)
	AM_RANGE(0xf400, 0xf400) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0xf500, 0xf500) AM_WRITE(OKIM6295_data_1_w)
	AM_RANGE(0xf600, 0xf600) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf700, 0xf700) AM_WRITE(MWA8_NOP)
//	AM_RANGE(0xf600, 0xf600) AM_WRITE(tharrier_oki6295_bankswitch_0_w)
//	AM_RANGE(0xf700, 0xf700) AM_WRITE(tharrier_oki6295_bankswitch_1_w)
ADDRESS_MAP_END

static WRITE8_HANDLER( tharrier_oki6295_bankswitch_0_w )
{
	UINT8 *rom = memory_region(REGION_SOUND1);

	data &= 3;
	if (data != 3)
		memcpy(rom + 0x20000,rom + 0x40000 + data * 0x20000,0x20000);
}

static WRITE8_HANDLER( tharrier_oki6295_bankswitch_1_w )
{
	UINT8 *rom = memory_region(REGION_SOUND1);

	data &= 3;
	if (data != 3)
		memcpy(rom + 0x20000,rom + 0x40000 + data * 0x20000,0x20000);
}

static ADDRESS_MAP_START( tharrier_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(soundlatch2_w)
	AM_RANGE(0xf400, 0xf400) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0xf500, 0xf500) AM_WRITE(OKIM6295_data_1_w)
	AM_RANGE(0xf600, 0xf600) AM_WRITE(tharrier_oki6295_bankswitch_0_w)
	AM_RANGE(0xf700, 0xf700) AM_WRITE(tharrier_oki6295_bankswitch_1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( manybloc_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(YM2203_read_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( manybloc_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2203_write_port_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tharrier_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(tharrier_mcu_r) //ok
	AM_RANGE(0x080004, 0x080005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READ(soundlatch2_word_r)	/* from Z80 */
	AM_RANGE(0x080202, 0x080203) AM_READ(input_port_3_word_r)
	AM_RANGE(0x088000, 0x0883ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09c000, 0x09c7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x09d000, 0x09d7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tharrier_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080010, 0x080011) AM_WRITE(tharrier_mcu_control_w)
//	AM_RANGE(0x080012, 0x080013) AM_WRITE(MWA16_NOP)
//	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
//	AM_RANGE(0x080018, 0x080019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(soundlatch_word_w)
	AM_RANGE(0x088000, 0x0883ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
//	AM_RANGE(0x08c000, 0x08c007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09c7ff) AM_WRITE(MWA16_RAM) /* Unused txvideoram area? */
	AM_RANGE(0x09d000, 0x09d7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(nmk16_mainram_strange_w)  AM_BASE(&nmk16_mainram)
ADDRESS_MAP_END

//Read input port 1 030c8/  BAD
//3478  GOOD

static ADDRESS_MAP_START( mustang_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080004, 0x080005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READWRITE(NMK004_r, MWA16_NOP)
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x080016, 0x080017) AM_WRITENOP	// frame number?
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x088000, 0x0887ff) AM_READWRITE(MRA16_RAM, paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c001) AM_WRITE(mustang_scroll_w)
	AM_RANGE(0x08c002, 0x08c087) AM_WRITENOP	// ??
	AM_RANGE(0x090000, 0x093fff) AM_READWRITE(MRA16_RAM, nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09c7ff) AM_READWRITE(MRA16_RAM, nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READWRITE(MRA16_RAM, nmk16_mainram_strange_w) AM_BASE(&nmk16_mainram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mustangb_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080004, 0x080005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READWRITE(MRA16_NOP, MWA16_NOP)
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x080016, 0x080017) AM_WRITENOP	// frame number?
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(seibu_main_mustb_w)
	AM_RANGE(0x088000, 0x0887ff) AM_READWRITE(MRA16_RAM, paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c001) AM_WRITE(mustang_scroll_w)
	AM_RANGE(0x08c002, 0x08c087) AM_WRITENOP	// ??
	AM_RANGE(0x090000, 0x093fff) AM_READWRITE(MRA16_RAM, nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09c7ff) AM_READWRITE(MRA16_RAM, nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READWRITE(MRA16_RAM, nmk16_mainram_strange_w) AM_BASE(&nmk16_mainram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( acrobatm_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x3ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x80000, 0x8ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0xc0000, 0xc0001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xc0002, 0xc0003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xc0008, 0xc0009) AM_READ(input_port_2_word_r)
	AM_RANGE(0xc000a, 0xc000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0xc000e, 0xc000f) AM_READ(NMK004_r)
	AM_RANGE(0xc4000, 0xc45ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xcc000, 0xcffff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0xd4000, 0xd47ff) AM_READ(nmk_txvideoram_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( acrobatm_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x3ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x80000, 0x8ffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)
	AM_RANGE(0xc0014, 0xc0015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0xc0016, 0xc0017) AM_WRITENOP
	AM_RANGE(0xc0018, 0xc0019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0xc001e, 0xc001f) AM_WRITE(NMK004_w)
	AM_RANGE(0xc4000, 0xc45ff) AM_WRITE(paletteram16_RRRRGGGGBBBBxxxx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xc8000, 0xc8007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0xcc000, 0xcffff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0xd4000, 0xd47ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
ADDRESS_MAP_END

#define PROT_JSR(_offs_,_protvalue_,_pc_) \
	if(nmk16_mainram[(_offs_)/2] == _protvalue_) \
	{ \
		nmk16_mainram[(_offs_)/2] = 0xffff;  /*(MCU job done)*/ \
		nmk16_mainram[(_offs_+2-0x10)/2] = 0x4ef9;/*JMP*/\
		nmk16_mainram[(_offs_+4-0x10)/2] = 0x0000;/*HI-DWORD*/\
		nmk16_mainram[(_offs_+6-0x10)/2] = _pc_;  /*LO-DWORD*/\
	} \

#define PROT_INPUT(_offs_,_protvalue_,_protinput_,_input_) \
	if(nmk16_mainram[_offs_] == _protvalue_) \
	{\
		nmk16_mainram[_protinput_] = ((_input_ & 0xffff0000)>>16);\
		nmk16_mainram[_protinput_+1] = (_input_ & 0x0000ffff);\
	}


static WRITE16_HANDLER( hachamf_mainram_w )
{
	COMBINE_DATA(&nmk16_mainram[offset]);

	switch(offset)
	{
		case 0xe058/2: PROT_INPUT(0xe058/2,0xc71f,0xe000/2,0x00080000); break;
		case 0xe182/2: PROT_INPUT(0xe182/2,0x865d,0xe004/2,0x00080002); break;
		case 0xe51e/2: PROT_INPUT(0xe51e/2,0x0f82,0xe008/2,0x00080008); break;
		case 0xe6b4/2: PROT_INPUT(0xe6b4/2,0x79be,0xe00c/2,0x0008000a); break;
		case 0xe10e/2: PROT_JSR(0xe10e,0x8007,0x870a);//870a not 9d66
					  PROT_JSR(0xe10e,0x8000,0xd9c6); break;
		case 0xe11e/2: PROT_JSR(0xe11e,0x8038,0x972a);//972a
					  PROT_JSR(0xe11e,0x8031,0xd1f8); break;
		case 0xe12e/2: PROT_JSR(0xe12e,0x8019,0x9642);//OK-9642
					  PROT_JSR(0xe12e,0x8022,0xda06); break;
		case 0xe13e/2: PROT_JSR(0xe13e,0x802a,0x9d66);//9d66 not 9400 - OK
					  PROT_JSR(0xe13e,0x8013,0x81aa); break;
		case 0xe14e/2: PROT_JSR(0xe14e,0x800b,0xb3f2);//b3f2 - OK
					  PROT_JSR(0xe14e,0x8004,0x8994); break;
		case 0xe15e/2: PROT_JSR(0xe15e,0x803c,0xb59e);//b59e - OK
					  PROT_JSR(0xe15e,0x8035,0x8d0c); break;
		case 0xe16e/2: PROT_JSR(0xe16e,0x801d,0x9ac2);//9ac2 - OK
				 	  PROT_JSR(0xe16e,0x8026,0x8c36); break;
		case 0xe17e/2: PROT_JSR(0xe17e,0x802e,0xc366);//c366 - OK
					  PROT_JSR(0xe17e,0x8017,0x870a); break;
		case 0xe18e/2: PROT_JSR(0xe18e,0x8004,0xd620);        //unused
					  PROT_JSR(0xe18e,0x8008,0x972a); break; //unused
		case 0xe19e/2: PROT_JSR(0xe19e,0x8030,0xd9c6);//OK-d9c6
					  PROT_JSR(0xe19e,0x8039,0x9642); break;
		case 0xe1ae/2: PROT_JSR(0xe1ae,0x8011,0xd1f8);//d1f8 not c67e
					  PROT_JSR(0xe1ae,0x802a,0x9d66); break;
		case 0xe1be/2: PROT_JSR(0xe1be,0x8022,0xda06);//da06
					  PROT_JSR(0xe1be,0x801b,0xb3f2); break;
		case 0xe1ce/2: PROT_JSR(0xe1ce,0x8003,0x81aa);//81aa
					  PROT_JSR(0xe1ce,0x800c,0xb59e); break;
		case 0xe1de/2: PROT_JSR(0xe1de,0x8034,0x8994);//8994 - OK
					  PROT_JSR(0xe1de,0x803d,0x9ac2); break;
		case 0xe1ee/2: PROT_JSR(0xe1ee,0x8015,0x8d0c);//8d0c not 82f6
					  PROT_JSR(0xe1ee,0x802e,0xc366); break;
		case 0xe1fe/2: PROT_JSR(0xe1fe,0x8026,0x8c36);//8c36
					  PROT_JSR(0xe1fe,0x8016,0xd620); break;  //unused
		case 0xef00/2:
			if(nmk16_mainram[0xef00/2] == 0x60fe)
			{
				nmk16_mainram[0xef00/2] = 0x0000; //this is the coin counter
				nmk16_mainram[0xef02/2] = 0x0000;
				nmk16_mainram[0xef04/2] = 0x4ef9;
				nmk16_mainram[0xef06/2] = 0x0000;
				nmk16_mainram[0xef08/2] = 0x7dc2;
			}
			break;
	}
}

static int hachamf_mcu_type; // 0 - hachamf, 1 = tdragon
static void hachamf_mcu_run(void)
{
	UINT8 dsw_setting = hachamf_mcu_type;
	static UINT8 input_pressed;
	static UINT16 coin_input;
	UINT8 dsw[2];
	static UINT8 start_helper = 0;
	static UINT8 coin_count[2],coin_count_frac[2];
	static UINT8 i;

	/*Accept the start button but needs some m68k processing first,otherwise you can't start a play with 1 credit inserted*/
	if(start_helper & 1 && nmk16_mainram[0x9000/2] & 0x0200) /*start 1 */
	{
		nmk16_mainram[0xef00/2]--;
		start_helper = start_helper & 2;
	}
	if(start_helper & 2 && nmk16_mainram[0x9000/2] & 0x0100) /*start 2*/
	{
		nmk16_mainram[0xef00/2]--;
		start_helper = start_helper & 1;
	}

	/*needed because of the uncompatibility of the dsw settings.*/
	if(dsw_setting) // Thunder Dragon
	{
		dsw[0] = (input_port_3_word_r(0,0) & 0x7);
		dsw[1] = (input_port_3_word_r(0,0) & 0x38) >> 3;
		for(i=0;i<2;i++)
		{
			switch(dsw[i] & 7)
			{
				case 0: nmk16_mainram[0x9000/2]|=0x4000; break; //free play
				case 1: coin_count_frac[i] = 1; coin_count[i] = 4; break;
				case 2: coin_count_frac[i] = 1; coin_count[i] = 3; break;
				case 3: coin_count_frac[i] = 1; coin_count[i] = 2; break;
				case 4: coin_count_frac[i] = 4; coin_count[i] = 1; break;
				case 5: coin_count_frac[i] = 3; coin_count[i] = 1; break;
				case 6: coin_count_frac[i] = 2; coin_count[i] = 1; break;
				case 7: coin_count_frac[i] = 1; coin_count[i] = 1; break;
			}
		}
	}
	else // Hacha Mecha Fighter
	{
		dsw[0] = (input_port_2_word_r(0,0) & 0x0700) >> 8;
		dsw[1] = (input_port_2_word_r(0,0) & 0x3800) >> 11;
		for(i=0;i<2;i++)
		{
			switch(dsw[i] & 7)
			{
				case 0: nmk16_mainram[0x9000/2]|=0x4000; break; //free play
				case 1: coin_count_frac[i] = 4; coin_count[i] = 1; break;
				case 2: coin_count_frac[i] = 3; coin_count[i] = 1; break;
				case 3: coin_count_frac[i] = 2; coin_count[i] = 1; break;
				case 4: coin_count_frac[i] = 1; coin_count[i] = 4; break;
				case 5: coin_count_frac[i] = 1; coin_count[i] = 3; break;
				case 6: coin_count_frac[i] = 1; coin_count[i] = 2; break;
				case 7: coin_count_frac[i] = 1; coin_count[i] = 1; break;
			}
		}
	}

	/*read the coin port*/
	coin_input = (~(input_port_0_word_r(0,0)));

	if(coin_input & 0x01)//coin 1
	{
		if((input_pressed & 0x01) == 0)
		{
			if(coin_count_frac[0] != 1)
			{
				nmk16_mainram[0xef02/2]+=coin_count[0];
				if(coin_count_frac[0] == nmk16_mainram[0xef02/2])
				{
					nmk16_mainram[0xef00/2]+=coin_count[0];
					nmk16_mainram[0xef02/2] = 0;
				}
			}
			else
				nmk16_mainram[0xef00/2]+=coin_count[0];
		}
		input_pressed = (input_pressed & 0xfe) | 1;
	}
	else
		input_pressed = (input_pressed & 0xfe);

	if(coin_input & 0x02)//coin 2
	{
		if((input_pressed & 0x02) == 0)
		{
			if(coin_count_frac[1] != 1)
			{
				nmk16_mainram[0xef02/2]+=coin_count[1];
				if(coin_count_frac[1] == nmk16_mainram[0xef02/2])
				{
					nmk16_mainram[0xef00/2]+=coin_count[1];
					nmk16_mainram[0xef02/2] = 0;
				}
			}
			else
				nmk16_mainram[0xef00/2]+=coin_count[1];
		}
		input_pressed = (input_pressed & 0xfd) | 2;
	}
	else
		input_pressed = (input_pressed & 0xfd);

	if(coin_input & 0x04)//service 1
	{
		if((input_pressed & 0x04) == 0)
			nmk16_mainram[0xef00/2]++;
		input_pressed = (input_pressed & 0xfb) | 4;
	}
	else
		input_pressed = (input_pressed & 0xfb);

	/*The 0x9000 ram address is the status */
	if(nmk16_mainram[0xef00/2] > 0 && nmk16_mainram[0x9000/2] & 0x8000) //enable start button
	{
		if(coin_input & 0x08)//start 1
		{
			if((input_pressed & 0x08) == 0 && (!(nmk16_mainram[0x9000/2] & 0x0200))) //start 1
				start_helper = 1;

			input_pressed = (input_pressed & 0xf7) | 8;
		}
		else
			input_pressed = (input_pressed & 0xf7);

		if(coin_input & 0x10)//start 2
		{
			/*Decrease two coins to let two players play with one start 2 button and two credits inserted at the insert coin screen.*/
			if((input_pressed & 0x10) == 0 && (!(nmk16_mainram[0x9000/2] & 0x0100))) // start 2
				start_helper = (nmk16_mainram[0x9000/2] == 0x8000) ? (3) : (2);

			input_pressed = (input_pressed & 0xef) | 0x10;
		}
		else
			input_pressed = (input_pressed & 0xef);
	}
}

static ADDRESS_MAP_START( hachamf_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READ(NMK004_r)
	AM_RANGE(0x088000, 0x0887ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09c000, 0x09c7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hachamf_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x080018, 0x080019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x088000, 0x0887ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09c7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(hachamf_mainram_w) AM_BASE(&nmk16_mainram)	/* Work RAM again (fe000-fefff is shared with the sound CPU) */
ADDRESS_MAP_END

static ADDRESS_MAP_START( hachamfb_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000a, 0x08000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READ(NMK004_r)
	AM_RANGE(0x088000, 0x0887ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09c000, 0x09c7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hachamfb_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x080018, 0x080019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x088000, 0x0887ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09c7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)	/* Work RAM again (fe000-fefff is shared with the sound CPU) */
ADDRESS_MAP_END

static ADDRESS_MAP_START( bioship_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000a, 0x08000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READ(NMK004_r)
	AM_RANGE(0x088000, 0x0887ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09c000, 0x09c7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bioship_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
//	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x084000, 0x084001) AM_WRITE(bioship_bank_w)
	AM_RANGE(0x088000, 0x0887ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c007) AM_WRITE(mustang_scroll_w)
	AM_RANGE(0x08c010, 0x08c017) AM_WRITE(bioship_scroll_w)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09c7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)	/* Work RAM again (fe000-fefff is shared with the sound CPU) */
ADDRESS_MAP_END

static WRITE16_HANDLER( tdragon_mainram_w )
{
	COMBINE_DATA(&nmk16_mainram[offset]);

	switch(offset)
	{
		case 0xe066/2: PROT_INPUT(0xe066/2,0xe23e,0xe000/2,0x000c0000); break;
		case 0xe144/2: PROT_INPUT(0xe144/2,0xf54d,0xe004/2,0x000c0002); break;
		case 0xe60e/2: PROT_INPUT(0xe60e/2,0x067c,0xe008/2,0x000c0008); break;
		case 0xe714/2: PROT_INPUT(0xe714/2,0x198b,0xe00c/2,0x000c000a); break;
		case 0xe70e/2: PROT_JSR(0xe70e,0x8007,0x9e22);
					  PROT_JSR(0xe70e,0x8000,0xd518); break;
		case 0xe71e/2: PROT_JSR(0xe71e,0x8038,0xaa0a);
					  PROT_JSR(0xe71e,0x8031,0x8e7c); break;
		case 0xe72e/2: PROT_JSR(0xe72e,0x8019,0xac48);
					  PROT_JSR(0xe72e,0x8022,0xd558); break;
		case 0xe73e/2: PROT_JSR(0xe73e,0x802a,0xb110);
					  PROT_JSR(0xe73e,0x8013,0x96da); break;
		case 0xe74e/2: PROT_JSR(0xe74e,0x800b,0xb9b2);
					  PROT_JSR(0xe74e,0x8004,0xa062); break;
		case 0xe75e/2: PROT_JSR(0xe75e,0x803c,0xbb4c);
					  PROT_JSR(0xe75e,0x8035,0xa154); break;
		case 0xe76e/2: PROT_JSR(0xe76e,0x801d,0xafa6);
				 	  PROT_JSR(0xe76e,0x8026,0xa57a); break;
		case 0xe77e/2: PROT_JSR(0xe77e,0x802e,0xc6a4);
					  PROT_JSR(0xe77e,0x8017,0x9e22); break;
		case 0xe78e/2: PROT_JSR(0xe78e,0x8004,0xaa0a);
					  PROT_JSR(0xe78e,0x8008,0xaa0a); break;
		case 0xe79e/2: PROT_JSR(0xe79e,0x8030,0xd518);
					  PROT_JSR(0xe79e,0x8039,0xac48); break;
		case 0xe7ae/2: PROT_JSR(0xe7ae,0x8011,0x8e7c);
					  PROT_JSR(0xe7ae,0x802a,0xb110); break;
		case 0xe7be/2: PROT_JSR(0xe7be,0x8022,0xd558);
					  PROT_JSR(0xe7be,0x801b,0xb9b2); break;
		case 0xe7ce/2: PROT_JSR(0xe7ce,0x8003,0x96da);
					  PROT_JSR(0xe7ce,0x800c,0xbb4c); break;
		case 0xe7de/2: PROT_JSR(0xe7de,0x8034,0xa062);
					  PROT_JSR(0xe7de,0x803d,0xafa6); break;
		case 0xe7ee/2: PROT_JSR(0xe7ee,0x8015,0xa154);
					  PROT_JSR(0xe7ee,0x802e,0xc6a4); break;
		case 0xe7fe/2: PROT_JSR(0xe7fe,0x8026,0xa57a);
					  PROT_JSR(0xe7fe,0x8016,0xa57a); break;
		case 0xef00/2:
			if(nmk16_mainram[0xef00/2] == 0x60fe)
			{
				nmk16_mainram[0xef00/2] = 0x0000; //this is the coin counter
				nmk16_mainram[0xef02/2] = 0x0000;
				nmk16_mainram[0xef04/2] = 0x4ef9;
				nmk16_mainram[0xef06/2] = 0x0000;
				nmk16_mainram[0xef08/2] = 0x92f4;
			}
			break;
	}
}

static ADDRESS_MAP_START( tdragon_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x044022, 0x044023) AM_READ(MRA16_NOP)  /* No Idea */
	AM_RANGE(0x0b0000, 0x0bffff) AM_READ(MRA16_RAM)	/* Work RAM */
	AM_RANGE(0x0c8000, 0x0c87ff) AM_READ(MRA16_RAM)  /* Palette RAM */
	AM_RANGE(0x0c0000, 0x0c0001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0c0002, 0x0c0003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0c0008, 0x0c0009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0c000a, 0x0c000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x0c000e, 0x0c000f) AM_READ(NMK004_r)
	AM_RANGE(0x0cc000, 0x0cffff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x0d0000, 0x0d07ff) AM_READ(nmk_txvideoram_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tdragon_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0b0000, 0x0bffff) AM_WRITE(tdragon_mainram_w)  AM_BASE(&nmk16_mainram)	/* Work RAM */
	AM_RANGE(0x0c0014, 0x0c0015) AM_WRITE(nmk_flipscreen_w) /* Maybe */
	AM_RANGE(0x0c0018, 0x0c0019) AM_WRITE(nmk_tilebank_w) /* Tile Bank ? */
	AM_RANGE(0x0c001e, 0x0c001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x0c4000, 0x0c4007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x0c8000, 0x0c87ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0cc000, 0x0cffff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x0d0000, 0x0d07ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tdragonb_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0b0000, 0x0bffff) AM_WRITE(MWA16_RAM)  AM_BASE(&nmk16_mainram)	/* Work RAM */
	AM_RANGE(0x0c0014, 0x0c0015) AM_WRITE(nmk_flipscreen_w) /* Maybe */
	AM_RANGE(0x0c0018, 0x0c0019) AM_WRITE(nmk_tilebank_w) /* Tile Bank ? */
	AM_RANGE(0x0c001e, 0x0c001f) AM_WRITE(seibu_main_mustb_w)
	AM_RANGE(0x0c4000, 0x0c4007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x0c8000, 0x0c87ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0cc000, 0x0cffff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x0d0000, 0x0d07ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ssmissin_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x0b0000, 0x0bffff) AM_READ(MRA16_RAM)	/* Work RAM */
	AM_RANGE(0x0c8000, 0x0c87ff) AM_READ(MRA16_RAM)  /* Palette RAM */
	AM_RANGE(0x0c0000, 0x0c0001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0c0004, 0x0c0005) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0c0006, 0x0c0007) AM_READ(input_port_2_word_r)
//	AM_RANGE(0x0c000e, 0x0c000f) AM_READ(??)
	AM_RANGE(0x0cc000, 0x0cffff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x0d0000, 0x0d07ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0d0800, 0x0d0fff) AM_READ(nmk_txvideoram_r) // mirrors for air attack
	AM_RANGE(0x0d1000, 0x0d17ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0d1800, 0x0d1fff) AM_READ(nmk_txvideoram_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ssmissin_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0b0000, 0x0bffff) AM_WRITE(MWA16_RAM)  AM_BASE(&nmk16_mainram)	/* Work RAM */
	AM_RANGE(0x0c0014, 0x0c0015) AM_WRITE(nmk_flipscreen_w) /* Maybe */
	AM_RANGE(0x0c0018, 0x0c0019) AM_WRITE(nmk_tilebank_w) /* Tile Bank ? */
	AM_RANGE(0x0c001e, 0x0c001f) AM_WRITE(ssmissin_sound_w)
	AM_RANGE(0x0c4000, 0x0c4007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x0c8000, 0x0c87ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0cc000, 0x0cffff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x0d0000, 0x0d07ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0d0800, 0x0d0fff) AM_WRITE(nmk_txvideoram_w)
	AM_RANGE(0x0d1000, 0x0d17ff) AM_WRITE(nmk_txvideoram_w)
	AM_RANGE(0x0d1800, 0x0d1fff) AM_WRITE(nmk_txvideoram_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START( ssmissin_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9800, 0x9800) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0xa000, 0xa000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ssmissin_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(ssmissin_soundbank_w)
	AM_RANGE(0x9800, 0x9800) AM_WRITE(OKIM6295_data_0_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( strahl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x3ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x80000, 0x80001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x80002, 0x80003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x80008, 0x80009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x8000a, 0x8000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x8000e, 0x8000f) AM_READ(NMK004_r)
	AM_RANGE(0x8c000, 0x8c7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x90000, 0x93fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x94000, 0x97fff) AM_READ(nmk_fgvideoram_r)
	AM_RANGE(0x9c000, 0x9c7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0xf0000, 0xfffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( strahl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x3ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x80014, 0x80015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x8001e, 0x8001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x84000, 0x84007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x88000, 0x88007) AM_WRITE(nmk_scroll_2_w)
	AM_RANGE(0x8c000, 0x8c7ff) AM_WRITE(paletteram16_RRRRGGGGBBBBxxxx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x90000, 0x93fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x94000, 0x97fff) AM_WRITE(nmk_fgvideoram_w) AM_BASE(&nmk_fgvideoram)
	AM_RANGE(0x9c000, 0x9c7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0xf0000, 0xfffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( macross_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000a, 0x08000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READ(NMK004_r)
	AM_RANGE(0x088000, 0x0887ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09c000, 0x09c7ff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( macross_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x080016, 0x080017) AM_WRITE(MWA16_NOP)	/* IRQ enable? */
	AM_RANGE(0x080018, 0x080019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x088000, 0x0887ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09c7ff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(nmk16_mainram_strange_w) AM_BASE(&nmk16_mainram)//AM_WRITE(MWA16_RAM) AM_BASE(&ram)	/* Work RAM again */
ADDRESS_MAP_END

static ADDRESS_MAP_START( gunnail_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000a, 0x08000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x08000e, 0x08000f) AM_READ(NMK004_r)
	AM_RANGE(0x088000, 0x0887ff) AM_READ(MRA16_RAM) /* palette ram */
	AM_RANGE(0x090000, 0x093fff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09c000, 0x09cfff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x09d000, 0x09dfff) AM_READ(nmk_txvideoram_r)	/* mirror */
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gunnail_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(NMK004_w)
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x080016, 0x080017) AM_WRITE(MWA16_NOP)	/* IRQ enable? */
	AM_RANGE(0x080018, 0x080019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x088000, 0x0887ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
    AM_RANGE(0x08c000, 0x08c1ff) AM_RAM AM_WRITE(gunnail_scrollx_w) AM_BASE(&gunnail_scrollram)
	AM_RANGE(0x08c200, 0x08c201) AM_WRITE(gunnail_scrolly_w)
    AM_RANGE(0x08c202, 0x08c7ff) AM_RAM // extra scroll registers (used after a boss is killed and in level 5)
	AM_RANGE(0x090000, 0x093fff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09c000, 0x09cfff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x09d000, 0x09dfff) AM_WRITE(nmk_txvideoram_w)	/* mirror */
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram) /* Work RAM again */
ADDRESS_MAP_END

static ADDRESS_MAP_START( macross2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x100002, 0x100003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x100008, 0x100009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x10000a, 0x10000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x10000e, 0x10000f) AM_READ(soundlatch2_word_r)	/* from Z80 */
	AM_RANGE(0x120000, 0x1207ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x14ffff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x170000, 0x170fff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x171000, 0x171fff) AM_READ(nmk_txvideoram_r)	/* mirror */
	AM_RANGE(0x1f0000, 0x1fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( macross2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100014, 0x100015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x100016, 0x100017) AM_WRITE(macross2_sound_reset_w)	/* Z80 reset */
	AM_RANGE(0x100018, 0x100019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x10001e, 0x10001f) AM_WRITE(macross2_sound_command_w)	/* to Z80 */
	AM_RANGE(0x120000, 0x1207ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x130000, 0x130007) AM_WRITE(nmk_scroll_w)
	AM_RANGE(0x130008, 0x1307ff) AM_WRITE(MWA16_NOP)	/* 0 only? */
	AM_RANGE(0x140000, 0x14ffff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x170000, 0x170fff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x171000, 0x171fff) AM_WRITE(nmk_txvideoram_w)	/* mirror */
	AM_RANGE(0x1f0000, 0x1fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)	/* Work RAM again */
ADDRESS_MAP_END

static READ16_HANDLER( raphero_sound_result_r )
{
	return soundlatch2_r(0);
}

static ADDRESS_MAP_START( raphero_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x100002, 0x100003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x100008, 0x100009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x10000a, 0x10000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x10000e, 0x10000f) AM_READ(raphero_sound_result_r)	/* from Z80 */
	AM_RANGE(0x120000, 0x1207ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x14ffff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x170000, 0x170fff) AM_READ(nmk_txvideoram_r)
	AM_RANGE(0x171000, 0x171fff) AM_READ(nmk_txvideoram_r)	/* mirror */
	AM_RANGE(0x1f0000, 0x1fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( raphero_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100014, 0x100015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x100016, 0x100017) AM_WRITE(MWA16_NOP)	/* IRQ eanble? */
	AM_RANGE(0x100018, 0x100019) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x10001e, 0x10001f) AM_WRITE(macross2_sound_command_w)	/* to Z80 */
	AM_RANGE(0x120000, 0x1207ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x130000, 0x1307ff) AM_WRITE(nmk_scroll_3_w) AM_BASE(&gunnail_scrollram)
	AM_RANGE(0x140000, 0x14ffff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x170000, 0x170fff) AM_WRITE(nmk_txvideoram_w) AM_BASE(&nmk_txvideoram)
	AM_RANGE(0x171000, 0x171fff) AM_WRITE(nmk_txvideoram_w)	/* mirror */
	AM_RANGE(0x1f0000, 0x1fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)	/* Work RAM again */
ADDRESS_MAP_END


static WRITE8_HANDLER( raphero_sound_rombank_w )
{
	int bank = data & 7;
	memory_set_bankptr(1,memory_region(REGION_CPU2) + 0x10000 + (bank * 0x4000));
}



static ADDRESS_MAP_START( raphero_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0xbfff ) AM_READ( MRA8_BANK1 )
	AM_RANGE( 0xc000, 0xc000 ) AM_READWRITE( YM2203_status_port_0_r, YM2203_control_port_0_w )
	AM_RANGE( 0xc001, 0xc001 ) AM_READWRITE( YM2203_read_port_0_r, YM2203_write_port_0_w )
	AM_RANGE( 0xc800, 0xc800 ) AM_READWRITE( OKIM6295_status_0_r, OKIM6295_data_0_w )
	AM_RANGE( 0xc808, 0xc808 ) AM_READWRITE( OKIM6295_status_1_r, OKIM6295_data_1_w )
	AM_RANGE( 0xc810, 0xc817 ) AM_WRITE( NMK112_okibank_w )
//  AM_RANGE( 0xc810, 0xc817 ) AM_WRITE( okibank_w )
	AM_RANGE( 0xd000, 0xd000 ) AM_WRITE( raphero_sound_rombank_w )
	AM_RANGE( 0xd800, 0xd800 ) AM_READWRITE( soundlatch_r, soundlatch2_w )	// main cpu
	AM_RANGE( 0xe000, 0xffff ) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( macross2_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)	/* banked ROM */
	AM_RANGE(0xa000, 0xa000) AM_READ(MRA8_NOP)	/* IRQ ack? watchdog? */
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_READ(soundlatch_r)	/* from 68000 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( macross2_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(macross2_sound_bank_w)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(soundlatch2_w)	/* to 68000 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( macross2_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0x80, 0x80) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0x88, 0x88) AM_READ(OKIM6295_status_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( macross2_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0x88, 0x88) AM_WRITE(OKIM6295_data_1_w)
	AM_RANGE(0x90, 0x97) AM_WRITE(NMK112_okibank_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bjtwin_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x08000a, 0x08000b) AM_READ(input_port_3_word_r)
	AM_RANGE(0x084000, 0x084001) AM_READ(OKIM6295_status_0_lsb_r)
	AM_RANGE(0x084010, 0x084011) AM_READ(OKIM6295_status_1_lsb_r)
	AM_RANGE(0x088000, 0x0887ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x09c000, 0x09cfff) AM_READ(nmk_bgvideoram_r)
	AM_RANGE(0x09d000, 0x09dfff) AM_READ(nmk_bgvideoram_r)	/* mirror */
	AM_RANGE(0x0f0000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bjtwin_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080014, 0x080015) AM_WRITE(nmk_flipscreen_w)
	AM_RANGE(0x084000, 0x084001) AM_WRITE(OKIM6295_data_0_lsb_w)
	AM_RANGE(0x084010, 0x084011) AM_WRITE(OKIM6295_data_1_lsb_w)
	AM_RANGE(0x084020, 0x08402f) AM_WRITE(NMK112_okibank_lsb_w)
	AM_RANGE(0x088000, 0x0887ff) AM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x094000, 0x094001) AM_WRITE(nmk_tilebank_w)
	AM_RANGE(0x094002, 0x094003) AM_WRITE(MWA16_NOP)	/* IRQ enable? */
	AM_RANGE(0x09c000, 0x09cfff) AM_WRITE(nmk_bgvideoram_w) AM_BASE(&nmk_bgvideoram)
	AM_RANGE(0x09d000, 0x09dfff) AM_WRITE(nmk_bgvideoram_w)	/* mirror */
	AM_RANGE(0x0f0000, 0x0fffff) AM_WRITE(MWA16_RAM) AM_BASE(&nmk16_mainram)	/* Work RAM again */
ADDRESS_MAP_END


INPUT_PORTS_START( vandyke )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00,  "2" )
	PORT_DIPSETTING(    0x01,  "3" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( blkheart )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40,  "2" )
	PORT_DIPSETTING(    0xc0,  "3" )
	PORT_DIPSETTING(    0x80,  "4" )
	PORT_DIPSETTING(    0x00,  "5" )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( manybloc )
	PORT_START	/* IN0 - 0x080000 */
	PORT_BIT( 0x7fff, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )		// VBLANK ? Check code at 0x005640

	PORT_START	/* IN1 - 0x080002 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)	// select fruits
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)	// help
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)	// select fruits
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)	// help
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START	/* DSW - 0x080004 -> 0x0f0036 */
	PORT_DIPNAME( 0x0001, 0x0000, "Slot System" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, "Explanation" )
	PORT_DIPSETTING(      0x0000, DEF_STR( English ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Cabinet ) )		// "Play Type"
	PORT_DIPSETTING(      0x0008, DEF_STR( Upright ) )		//   "Uplight" !
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )		//   "Table"
	PORT_SERVICE( 0x0010, IP_ACTIVE_HIGH )				// "Test Mode"
	PORT_DIPNAME( 0x0060, 0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( Easy ) )				//   "Level 1
	PORT_DIPSETTING(      0x0000, DEF_STR( Normal ) )				//   "Level 2
	PORT_DIPSETTING(      0x0020, DEF_STR( Hard ) )				//   "Level 3
	PORT_DIPSETTING(      0x0040, DEF_STR( Hardest ) )				//   "Level 4
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Flip_Screen ) )	// "Display"
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )			//   DEF_STR( Normal )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )			//   "Inverse"
	PORT_DIPNAME( 0x0700, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x3800, 0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xc000, 0x0000, "Plate Probability" )
	PORT_DIPSETTING(      0xc000, "Bad" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x4000, "Better" )
	PORT_DIPSETTING(      0x8000, "Best" )
INPUT_PORTS_END

INPUT_PORTS_START( tharrier )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x7fe0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* Mcu status? */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2) //title
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2  ) //in game
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )	
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x3000, "200k" )
	PORT_DIPSETTING(      0x2000, "200k and 1 Mil" )
	PORT_DIPSETTING(      0x0000, "200k, 500k & 1,2,3,5 Mil" )
	PORT_DIPSETTING(      0x1000, DEF_STR( None ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x4000, "2" )
	PORT_DIPSETTING(      0xc000, "3" )
	PORT_DIPSETTING(      0x8000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNKNOWN )//coin ?
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN ) //coin ?
INPUT_PORTS_END

INPUT_PORTS_START( mustang )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// TEST in service mode
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x4000, "2" )
	PORT_DIPSETTING(      0xc000, "3" )
	PORT_DIPSETTING(      0x8000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )

	PORT_START	/* referenced by seibu sound board */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hachamf )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* DSW A */
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0100, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0300, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0800, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x3000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x2800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x2000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, "Language" )
	PORT_DIPSETTING(    0x0000, "English" )
	PORT_DIPSETTING(    0x0002, "Japanese" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0004, "Easy" )
	PORT_DIPSETTING(    0x000c, "Normal" )
	PORT_DIPSETTING(    0x0008, "Hard" )
	PORT_DIPSETTING(    0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0000, "1" )
	PORT_DIPSETTING(    0x0040, "2" )
	PORT_DIPSETTING(    0x00c0, "3" )
	PORT_DIPSETTING(    0x0080, "4" )

	PORT_START
INPUT_PORTS_END

INPUT_PORTS_START( hachamfb )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x02, "Japanese" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( strahl )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* DSW A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START  /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "100k and every 200k" )
	PORT_DIPSETTING(    0x60, "200k and every 200k" )
	PORT_DIPSETTING(    0x20, "300k and every 300k" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( acrobatm )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) /* used by secret code */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001C, 0x001C, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001C, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000C, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00E0, 0x00E0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00C0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00E0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00A0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )

	PORT_START /* DSW B */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "50k and 100k" )
	PORT_DIPSETTING(    0x06, "100k and 100k" )
	PORT_DIPSETTING(    0x04, "100k and 200k" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x00, "5" )
INPUT_PORTS_END

INPUT_PORTS_START( bioship )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0006, 0x0006, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Hardest ) )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x00C0, 0x00C0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x00C0, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001C, 0x001C, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001C, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000C, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00E0, 0x00E0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00C0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00E0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00A0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( tdragon )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// TEST in service mode
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0001, "4" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( tdragonb )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// TEST in service mode
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0001, "4" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ssmissin )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )		// "Servise" in "test mode"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)		// "Fire"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)		// "Bomb"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)		// "Fire"
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)		// "Bomb"
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
#if 0
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_B ) )	// initialised but not read back
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
#else
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
#endif
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( airattck )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )		// "Servise" in "test mode"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)		// "Fire"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)		// "Bomb"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)		// "Fire"
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)		// "Bomb"
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( macross )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( macross2 )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )	/* Initial points needed for 1st Stage Clear */
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )		/* 100,000 */
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )		/* 150,000 */
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )		/* 200,000 */
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )		/* 250,000 */
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( tdragon2 )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( gunnail )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( sabotenb )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* shown in service mode, but no effect */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( bjtwin )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* shown in service mode, but no effect */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Starting level" )
	PORT_DIPSETTING(    0x08, "Germany" )
	PORT_DIPSETTING(    0x04, "Thailand" )
	PORT_DIPSETTING(    0x0c, "Nevada" )
	PORT_DIPSETTING(    0x0e, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x06, "Korea" )
	PORT_DIPSETTING(    0x0a, "England" )
	PORT_DIPSETTING(    0x02, "Hong Kong" )
	PORT_DIPSETTING(    0x00, "China" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( nouryoku )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x03, 0x03, "Life Decrease Speed" )
	PORT_DIPSETTING(    0x02, "Slow" )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static const gfx_decode tharrier_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },	/* color 0x200-0x2ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};

static const gfx_decode macross_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x200, 16 },	/* color 0x200-0x2ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};

static const gfx_decode macross2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x300, 16 },	/* color 0x300-0x3ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 32 },	/* color 0x100-0x2ff */
	{ -1 } /* end of array */
};

static const gfx_decode bjtwin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX2, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};

static const gfx_decode bioship_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x300, 16 },	/* color 0x300-0x3ff */
	{ REGION_GFX2, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ REGION_GFX3, 0, &tilelayout, 0x200, 16 },	/* color 0x200-0x2ff */
	{ REGION_GFX4, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ -1 }
};

static const gfx_decode strahl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX2, 0, &tilelayout, 0x300, 16 },	/* color 0x300-0x3ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ REGION_GFX4, 0, &tilelayout, 0x200, 16 },	/* color 0x200-0x2ff */
	{ -1 }
};



static struct YM2203interface ym2203_nmk004_interface =
{
	0,0,0,0,NMK004_irq
};

static void ym2203_irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	0,0,0,0,ym2203_irqhandler
};

static INTERRUPT_GEN( nmk_interrupt )
{
	if (cpu_getiloops() == 0) cpunum_set_input_line(0, 4, HOLD_LINE);
	else cpunum_set_input_line(0, 2, HOLD_LINE);
}


/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE


static MACHINE_DRIVER_START( tharrier )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz */
	MDRV_CPU_PROGRAM_MAP(tharrier_readmem,tharrier_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_CPU_ADD(Z80, 3000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(manybloc_sound_readmem,tharrier_sound_writemem)
	MDRV_CPU_IO_MAP(manybloc_sound_readport,manybloc_sound_writeport)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(mustang_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tharrier_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(tharrier)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( manybloc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10? MHz - check */
	MDRV_CPU_PROGRAM_MAP(manybloc_readmem,manybloc_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(60))/* is this is too high it breaks the game on this one, too low sprites flicker */

	MDRV_CPU_ADD(Z80, 3000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(manybloc_sound_readmem,manybloc_sound_writemem)
	MDRV_CPU_IO_MAP(manybloc_sound_readport,manybloc_sound_writeport)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(tharrier_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(macross)
//  MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(manybloc)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mustang )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(mustang_map,0)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mustangb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(mustangb_map,0)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(mustang_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE(14318180/4, 8000, 1)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bioship )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000) /* 16 MHz ? */
	MDRV_CPU_PROGRAM_MAP(bioship_readmem,bioship_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(bioship_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(bioship)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(bioship)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( vandyke )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(vandyke_readmem,vandyke_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( acrobatm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? 12 MHz? */
	MDRV_CPU_PROGRAM_MAP(acrobatm_readmem,acrobatm_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END


/* bootleg using Raiden sound hardware */
static MACHINE_DRIVER_START( tdragonb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(tdragon_readmem,tdragonb_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ?? drives music */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(mustang_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE(14318180/4, 8000, 1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ssmissin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(ssmissin_readmem,ssmissin_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112)) /* input related */

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */ /* 4 MHz ? */
	MDRV_CPU_PROGRAM_MAP(ssmissin_sound_readmem,ssmissin_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( strahl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* 12 MHz ? */
	MDRV_CPU_PROGRAM_MAP(strahl_readmem,strahl_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(strahl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(strahl)
	MDRV_VIDEO_EOF(strahl)
	MDRV_VIDEO_UPDATE(strahl)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static INTERRUPT_GEN( hachamf_interrupt )
{
	if (cpu_getiloops() == 0) {
		hachamf_mcu_run(); // run mcu sim

		cpunum_set_input_line(0, 4, HOLD_LINE);
	} else {
		cpunum_set_input_line(0, 2, HOLD_LINE); // tdragon
	}
}

static MACHINE_DRIVER_START( hachamf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(hachamf_readmem,hachamf_writemem)
	MDRV_CPU_VBLANK_INT(hachamf_interrupt,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(hachamf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hachamfb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(hachamfb_readmem,hachamfb_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)// No mcu
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(hachamf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tdragon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(tdragon_readmem,tdragon_writemem)
	MDRV_CPU_VBLANK_INT(hachamf_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ?? drives music */

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(hachamf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( macross )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(macross_readmem,macross_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gunnail )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz? */
	MDRV_CPU_PROGRAM_MAP(gunnail_readmem,gunnail_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(NMK004)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(gunnail)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(gunnail)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_nmk004_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 2.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( macross2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(macross2_readmem,macross2_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */ /* 4 MHz ? */
	MDRV_CPU_PROGRAM_MAP(macross2_sound_readmem,macross2_sound_writemem)
	MDRV_CPU_IO_MAP(macross2_sound_readport,macross2_sound_writeport)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross2)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tdragon2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_PROGRAM_MAP(macross2_readmem,macross2_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */ /* 4 MHz ? */
	MDRV_CPU_PROGRAM_MAP(macross2_sound_readmem,macross2_sound_writemem)
	MDRV_CPU_IO_MAP(macross2_sound_readport,macross2_sound_writeport)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tdragon2)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( raphero )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000) /* 14 MHz measured */
	MDRV_CPU_PROGRAM_MAP(raphero_readmem,raphero_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ???????? */

	MDRV_CPU_ADD(TMP90841, 8000000)
        /* audio CPU */ /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(raphero_sound_map,0)

	MDRV_FRAMES_PER_SECOND(56) // measured
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross2)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.70)
	MDRV_SOUND_ROUTE(1, "mono", 0.70)
	MDRV_SOUND_ROUTE(2, "mono", 0.70)
	MDRV_SOUND_ROUTE(3, "mono", 1.00)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bjtwin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz? It's a P12, but xtals are 10MHz and 16MHz */
	MDRV_CPU_PROGRAM_MAP(bjtwin_readmem,bjtwin_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(112))/* ?? drives music */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(bjtwin_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(bjtwin)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(bjtwin)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

ROM_START( vandyke )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "vdk-1.16",  0x00000, 0x20000, CRC(c1d01c59) SHA1(04a7fd31ca4d87d078070390660edf08bf1d96b5) )
	ROM_LOAD16_BYTE( "vdk-2.15",  0x00001, 0x20000, CRC(9d741cc2) SHA1(2d101044fba5fc5b7d63869a0a053c42fdc2598b) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu code */
	ROM_LOAD( "vdk-4.127",    0x00000, 0x10000, CRC(eba544f0) SHA1(36f6d048d15a392542a9220a244d8a7049aaff8b) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-3.222",		0x000000, 0x010000, CRC(5a547c1b) SHA1(2d61f51ce2f91ebf0053ce3a00911d1bcbaba816) )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-01.13",		0x000000, 0x080000, CRC(195a24be) SHA1(3a20dd746a87efc5c1fdc5025b709efeff82e05e) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "vdk-07.202",	0x000000, 0x080000, CRC(42d41f06) SHA1(69fd1d38187b8081f65acea2424bc1a0d455d90c) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-06.203",	0x000001, 0x080000, CRC(d54722a8) SHA1(47f8e97b29ae0ff1a1d7d50734e4219a87a2ed57) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-04.2-1",	0x100000, 0x080000, CRC(0a730547) SHA1(afac0549eb86d1fab5ca8ae2a0dad14144f55c02) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-05.3-1",	0x100001, 0x080000, CRC(ba456d27) SHA1(5485a560ae2c2c8b6fdec314393c02a3de758ef3) )	/* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-02.126",     0x000000, 0x020000, CRC(b2103274) SHA1(6bbdc912393607cd5306be946327c5ea0178c7a6) )
	ROM_CONTINUE(               0x040000, 0x060000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-03.165",     0x000000, 0x020000, CRC(631776d3) SHA1(ffd76e5b03130252c55eaa6ae7edfee5632dae73) )
	ROM_CONTINUE(               0x040000, 0x060000 )	/* banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic100.bpr", 0x0000, 0x0100, CRC(98ed1c97) SHA1(f125ad05c3cbd1b1ab356161f9b1d814781d4c3b) )	/* V-sync hw (unused) */
	ROM_LOAD( "ic101.bpr", 0x0100, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* H-sync hw (unused) */
ROM_END

ROM_START( vandyjal )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )  /* 68000 code */
	ROM_LOAD16_BYTE( "vdk-1.16",   0x00000, 0x20000, CRC(c1d01c59) SHA1(04a7fd31ca4d87d078070390660edf08bf1d96b5) )
	ROM_LOAD16_BYTE( "jaleco2.15", 0x00001, 0x20000, CRC(170e4d2e) SHA1(6009d19d30e345fea93e039d165061e2b20ff058) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu code */
	ROM_LOAD( "vdk-4.127",    0x00000, 0x10000, CRC(eba544f0) SHA1(36f6d048d15a392542a9220a244d8a7049aaff8b) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-3.222",		0x000000, 0x010000, CRC(5a547c1b) SHA1(2d61f51ce2f91ebf0053ce3a00911d1bcbaba816) )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-01.13",		0x000000, 0x080000, CRC(195a24be) SHA1(3a20dd746a87efc5c1fdc5025b709efeff82e05e) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "vdk-07.202",	0x000000, 0x080000, CRC(42d41f06) SHA1(69fd1d38187b8081f65acea2424bc1a0d455d90c) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-06.203",	0x000001, 0x080000, CRC(d54722a8) SHA1(47f8e97b29ae0ff1a1d7d50734e4219a87a2ed57) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-04.2-1",	0x100000, 0x080000, CRC(0a730547) SHA1(afac0549eb86d1fab5ca8ae2a0dad14144f55c02) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-05.3-1",	0x100001, 0x080000, CRC(ba456d27) SHA1(5485a560ae2c2c8b6fdec314393c02a3de758ef3) )	/* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-02.126",     0x000000, 0x020000, CRC(b2103274) SHA1(6bbdc912393607cd5306be946327c5ea0178c7a6) )
	ROM_CONTINUE(               0x040000, 0x060000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-03.165",     0x000000, 0x020000, CRC(631776d3) SHA1(ffd76e5b03130252c55eaa6ae7edfee5632dae73) )
	ROM_CONTINUE(               0x040000, 0x060000 )	/* banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic100.bpr", 0x0000, 0x0100, CRC(98ed1c97) SHA1(f125ad05c3cbd1b1ab356161f9b1d814781d4c3b) )	/* V-sync hw (unused) */
	ROM_LOAD( "ic101.bpr", 0x0100, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* H-sync hw (unused) */
ROM_END

ROM_START( vandyja2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "vdk-even.16",  0x00000, 0x20000, CRC(cde05a84) SHA1(dab5981d7dad9abe86cfe011da8ca0b11d484a3f) ) /* Hand written labels, dated 2/12 */
	ROM_LOAD16_BYTE( "vdk-odd.15",   0x00001, 0x20000, CRC(0f6fea40) SHA1(3acbe72c251d51b028d8c66274263a2b39b042ea) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu code */
	ROM_LOAD( "vdk-4.127",    0x00000, 0x10000, CRC(eba544f0) SHA1(36f6d048d15a392542a9220a244d8a7049aaff8b) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-3.222",		0x000000, 0x010000, CRC(5a547c1b) SHA1(2d61f51ce2f91ebf0053ce3a00911d1bcbaba816) )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-01.13",		0x000000, 0x080000, CRC(195a24be) SHA1(3a20dd746a87efc5c1fdc5025b709efeff82e05e) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "vdk-07.202",	0x000000, 0x080000, CRC(42d41f06) SHA1(69fd1d38187b8081f65acea2424bc1a0d455d90c) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-06.203",	0x000001, 0x080000, CRC(d54722a8) SHA1(47f8e97b29ae0ff1a1d7d50734e4219a87a2ed57) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-04.2-1",	0x100000, 0x080000, CRC(0a730547) SHA1(afac0549eb86d1fab5ca8ae2a0dad14144f55c02) )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-05.3-1",	0x100001, 0x080000, CRC(ba456d27) SHA1(5485a560ae2c2c8b6fdec314393c02a3de758ef3) )	/* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-02.126",     0x000000, 0x020000, CRC(b2103274) SHA1(6bbdc912393607cd5306be946327c5ea0178c7a6) )
	ROM_CONTINUE(               0x040000, 0x060000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-03.165",     0x000000, 0x020000, CRC(631776d3) SHA1(ffd76e5b03130252c55eaa6ae7edfee5632dae73) )
	ROM_CONTINUE(               0x040000, 0x060000 )	/* banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic100.bpr", 0x0000, 0x0100, CRC(98ed1c97) SHA1(f125ad05c3cbd1b1ab356161f9b1d814781d4c3b) )	/* V-sync hw (unused) */
	ROM_LOAD( "ic101.bpr", 0x0100, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* H-sync hw (unused) */
ROM_END

ROM_START( tharrier )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2" ,   0x00000, 0x20000, CRC(78923aaa) SHA1(28338f49581180604403e1bd200f524fc4cb8b9f) )
	ROM_LOAD16_BYTE( "3" ,   0x00001, 0x20000, CRC(99cea259) SHA1(75abfb08b2358dd13809ade5a2dfffeb8b8df82c) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "12" ,   0x00000, 0x10000, CRC(b959f837) SHA1(073b14935e7d5b0cad19a3471fd26e9e3a363827) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1" ,        0x000000, 0x10000, CRC(c7402e4a) SHA1(25cade2f8d4784887f0f51beb48b1e6b695629c2) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "89050-4" ,  0x000000, 0x80000, CRC(64d7d687) SHA1(dcfeac71fd577439e31cc1186b720388fbdc6ca0) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "89050-13",	0x000000, 0x80000, CRC(24db3fa4) SHA1(e0d76c479dfcacf03c04ec4760caecf3fd1e2ff7) )	/* Sprites */
	ROM_LOAD16_BYTE( "89050-17",	0x000001, 0x80000, CRC(7f715421) SHA1(bde5e0e1e22519e51ca0fd806909e90cc5b1c5b8) )

	ROM_REGION(0xa0000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "89050-8",     0x00000, 0x20000, CRC(11ee4c39) SHA1(163295c385cff963a5bf87dc3e7bef6019e10ba8) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION(0xa0000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "89050-10",    0x00000, 0x20000, CRC(893552ab) SHA1(b0a34291f4e482858ed295203ae031b17c2dbabc) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */


	ROM_REGION( 0x140, REGION_PROMS, 0 )
	ROM_LOAD( "21.bpr",  0x00000, 0x100, CRC(fcd5efea) SHA1(cbda6b14127dabd1788cc256743cf62efaa5e8c4) )
	ROM_LOAD( "22.bpr",  0x00000, 0x100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )
	ROM_LOAD( "23.bpr",  0x00000, 0x020, CRC(fc3569f4) SHA1(e1c498085e4ae9d0a995c94530544b0a5b760fbf) )
	ROM_LOAD( "24.bpr",  0x00000, 0x100, CRC(e0a009fe) SHA1(a66a27bb405d4ff8e4c0062273ee9b11e76ee520) )
	ROM_LOAD( "25.bpr",  0x00000, 0x100, CRC(e0a009fe) SHA1(a66a27bb405d4ff8e4c0062273ee9b11e76ee520) ) /* same as 24.bin */
	ROM_LOAD( "26.bpr",  0x00120, 0x020, CRC(0cbfb33e) SHA1(5dfee031a0a14bcd667fe2af2fa9cdfac3941d22) )
ROM_END

ROM_START( tharierj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2.bin" ,   0x00000, 0x20000, CRC(f3887a44) SHA1(4e5b660d33ba1d1e00263030efa67e2db376a234) )
	ROM_LOAD16_BYTE( "3.bin" ,   0x00001, 0x20000, CRC(65c247f6) SHA1(9f35f2b6f54814b4c4d23e2d78db8043e678fef2) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "12" ,   0x00000, 0x10000, CRC(b959f837) SHA1(073b14935e7d5b0cad19a3471fd26e9e3a363827) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin" ,        0x000000, 0x10000, CRC(005c26c3) SHA1(ee88d8f956b9b0a8ba5fb49c5c05f6ed6f01729c) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "89050-4" ,  0x000000, 0x80000, CRC(64d7d687) SHA1(dcfeac71fd577439e31cc1186b720388fbdc6ca0) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "89050-13",	0x000000, 0x80000, CRC(24db3fa4) SHA1(e0d76c479dfcacf03c04ec4760caecf3fd1e2ff7) )	/* Sprites */
	ROM_LOAD16_BYTE( "89050-17",	0x000001, 0x80000, CRC(7f715421) SHA1(bde5e0e1e22519e51ca0fd806909e90cc5b1c5b8) )

	ROM_REGION(0xa0000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "89050-8",     0x00000, 0x20000, CRC(11ee4c39) SHA1(163295c385cff963a5bf87dc3e7bef6019e10ba8) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION(0xa0000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "89050-10",    0x00000, 0x20000, CRC(893552ab) SHA1(b0a34291f4e482858ed295203ae031b17c2dbabc) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x140, REGION_PROMS, 0 )
	ROM_LOAD( "21.bpr",  0x00000, 0x100, CRC(fcd5efea) SHA1(cbda6b14127dabd1788cc256743cf62efaa5e8c4) )
	ROM_LOAD( "22.bpr",  0x00000, 0x100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )
	ROM_LOAD( "23.bpr",  0x00000, 0x020, CRC(fc3569f4) SHA1(e1c498085e4ae9d0a995c94530544b0a5b760fbf) )
	ROM_LOAD( "24.bpr",  0x00000, 0x100, CRC(e0a009fe) SHA1(a66a27bb405d4ff8e4c0062273ee9b11e76ee520) )
	ROM_LOAD( "25.bpr",  0x00000, 0x100, CRC(e0a009fe) SHA1(a66a27bb405d4ff8e4c0062273ee9b11e76ee520) ) /* same as 24.bin */
	ROM_LOAD( "26.bpr",  0x00120, 0x020, CRC(0cbfb33e) SHA1(5dfee031a0a14bcd667fe2af2fa9cdfac3941d22) )
ROM_END

ROM_START( mustang )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2.bin",    0x00000, 0x20000, CRC(bd9f7c89) SHA1(a0af46a8ff82b90bece2515e1bd74e7a7ddf5379) )
	ROM_LOAD16_BYTE( "3.bin",    0x00001, 0x20000, CRC(0eec36a5) SHA1(c549fbcd3e2741a6d0f2633ded6a85909d37f633) )

	ROM_REGION(0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "90058-7",    0x00000, 0x10000, CRC(920a93c8) SHA1(7660ca419e2fd98848ae7f5994994eaed023151e) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-1",    0x00000, 0x20000, CRC(81ccfcad) SHA1(70a0f769c0d4588f6f17bd52cc86a745f30e9f00) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-4",    0x000000, 0x80000, CRC(a07a2002) SHA1(55720d84a251c33c52ae8c33aa41ff8ac9727941) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "90058-8",    0x00000, 0x80000, CRC(560bff04) SHA1(b005642adc81d878971ecbdead8ef5e604c90ae2) )
	ROM_LOAD16_BYTE( "90058-9",    0x00001, 0x80000, CRC(b9d72a03) SHA1(43ee9def1b6c491c6832562d66c1af54d81d9b3c) )

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-5",    0x00000, 0x20000, CRC(c60c883e) SHA1(8a01950cad820b2e781ec81cd12737829edc4f19) )
	ROM_CONTINUE(           0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-6",    0x00000, 0x20000, CRC(233c1776) SHA1(7010a2f914611698a65bf4f22bc1753a9ed26277) )
	ROM_CONTINUE(           0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "10.bpr",  0x00000, 0x100, CRC(633ab1c9) SHA1(acd99fcca41eaab7948ca84988352f1d7d519c61) ) /* unknown */
	ROM_LOAD( "90058-11",  0x00100, 0x100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) ) /* unknown */
ROM_END

ROM_START( mustangs )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "90058-2",    0x00000, 0x20000, CRC(833aa458) SHA1(a9924f7044397e3a36c674b064173ffae80a79ec) )
	ROM_LOAD16_BYTE( "90058-3",    0x00001, 0x20000, CRC(e4b80f06) SHA1(ce589cebb5ea85c89eb44796b821a4bd0c44b9a8) )

	ROM_REGION(0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "90058-7",    0x00000, 0x10000, CRC(920a93c8) SHA1(7660ca419e2fd98848ae7f5994994eaed023151e) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-1",    0x00000, 0x20000, CRC(81ccfcad) SHA1(70a0f769c0d4588f6f17bd52cc86a745f30e9f00) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-4",    0x000000, 0x80000, CRC(a07a2002) SHA1(55720d84a251c33c52ae8c33aa41ff8ac9727941) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "90058-8",    0x00000, 0x80000, CRC(560bff04) SHA1(b005642adc81d878971ecbdead8ef5e604c90ae2) )
	ROM_LOAD16_BYTE( "90058-9",    0x00001, 0x80000, CRC(b9d72a03) SHA1(43ee9def1b6c491c6832562d66c1af54d81d9b3c) )

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-5",    0x00000, 0x20000, CRC(c60c883e) SHA1(8a01950cad820b2e781ec81cd12737829edc4f19) )
	ROM_CONTINUE(           0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-6",    0x00000, 0x20000, CRC(233c1776) SHA1(7010a2f914611698a65bf4f22bc1753a9ed26277) )
	ROM_CONTINUE(           0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "90058-10",  0x00000, 0x100, CRC(de156d99) SHA1(07b70deca74e23bab7c13e5e9aee32d0dbb06509) ) /* unknown */
	ROM_LOAD( "90058-11",  0x00100, 0x100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) ) /* unknown */
ROM_END

ROM_START( mustangb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mustang.14",    0x00000, 0x20000, CRC(13c6363b) SHA1(e2c1985d1c8ec9751c47cd7e1b85e007f3aeb6fd) )
	ROM_LOAD16_BYTE( "mustang.13",    0x00001, 0x20000, CRC(d8ccce31) SHA1(e8e3e34a480fcd298f11833c6c968c5df77c0e2a) )

	ROM_REGION(0x20000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "mustang.16",    0x00000, 0x8000, CRC(99ee7505) SHA1(b97c8ee5e26e8554b5de506fba3b32cc2fde53c9) )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-1",    0x00000, 0x20000, CRC(81ccfcad) SHA1(70a0f769c0d4588f6f17bd52cc86a745f30e9f00) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-4",    0x000000, 0x80000, CRC(a07a2002) SHA1(55720d84a251c33c52ae8c33aa41ff8ac9727941) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "90058-8",    0x00000, 0x80000, CRC(560bff04) SHA1(b005642adc81d878971ecbdead8ef5e604c90ae2) )
	ROM_LOAD16_BYTE( "90058-9",    0x00001, 0x80000, CRC(b9d72a03) SHA1(43ee9def1b6c491c6832562d66c1af54d81d9b3c) )

	ROM_REGION( 0x010000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "mustang.17",    0x00000, 0x10000, CRC(f6f6c4bf) SHA1(ea4cf74d968e254ae47c16c2f4c2f4bc1a528808) )
ROM_END

ROM_START( acrobatm )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "02_ic100.bin",    0x00000, 0x20000, CRC(3fe487f4) SHA1(29aba5debcfddff14e584a1c7c5a403e85fc6ec0) )
	ROM_LOAD16_BYTE( "01_ic101.bin",    0x00001, 0x20000, CRC(17175753) SHA1(738865744badb78a0414ff650a94b97e516d0ea0) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "03_ic79.bin",   0x000000, 0x10000, CRC(d86c186e) SHA1(2e263d4780f2ba7acc7faa88472c85216fbae6a3) ) /* Characters */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "09_ic8.bin",  0x000000, 0x100000, CRC(7c12afed) SHA1(ae793e41599355a126cbcce91cd2c9f212d21853) ) /* Foreground */

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "07_ic42.bin",  0x000000, 0x100000, CRC(5672bdaa) SHA1(5401a104d72904de19b73125451767bc63d36809) ) /* Sprites */
	ROM_LOAD( "08_ic29.bin",  0x100000, 0x080000, CRC(b4c0ace3) SHA1(5d638781d588cfbf4025d002d5a2309049fe1ee5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "04_ic74.bin",    0x00000, 0x10000, CRC(176905fb) SHA1(135a184f44bedd93b293b9124fa0bd725e0ee93b) )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "05_ic54.bin",    0x00000, 0x20000, CRC(3b8c2b0e) SHA1(72491da32512823540b67dc5027f21c74af08c7d) )
	ROM_CONTINUE(               0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0xa0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "06_ic53.bin",    0x00000, 0x20000, CRC(c1517cd4) SHA1(5a91ddc608c7a6fbdd9f93e503d39eac02ef04a4) )
	ROM_CONTINUE(               0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "10_ic81.bin",    0x0000, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* unknown */
	ROM_LOAD( "11_ic80.bin",    0x0100, 0x0100, CRC(633ab1c9) SHA1(acd99fcca41eaab7948ca84988352f1d7d519c61) )	/* unknown */
ROM_END

ROM_START( bioship )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2",    0x00000, 0x20000, CRC(acf56afb) SHA1(0e8ec494ab406cfee24cf586059878332265de75) )
	ROM_LOAD16_BYTE( "1",    0x00001, 0x20000, CRC(820ef303) SHA1(d2ef29557b05abf8ae79a2c7ce0d15a91b36eeff) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7",         0x000000, 0x10000, CRC(2f3f5a10) SHA1(c1006eb755eec75f69dc7972d78d0c59088eb140) ) /* Characters */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sbs-g.01",  0x000000, 0x80000, CRC(21302e78) SHA1(a17939c0529c8e9ec2a4edd5e6be4bcb67f86787) ) /* Foreground */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sbs-g.03",  0x000000, 0x80000, CRC(60e00d7b) SHA1(36fd02a7842ce1e79b8c4cfbe9c97052bef4aa62) ) /* Sprites */

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "sbs-g.02",  0x000000, 0x80000, CRC(f31eb668) SHA1(67d6d56ea203edfbae4db658399bf61f14134206) ) /* Background */

	ROM_REGION16_BE(0x20000, REGION_GFX5, 0 )	/* Background tilemaps (used at runtime) */
	ROM_LOAD16_BYTE( "8",    0x00000, 0x10000, CRC(75a46fea) SHA1(3d78cfc482b42779bb5aedb722c4a39cbc71bd10) )
	ROM_LOAD16_BYTE( "9",    0x00001, 0x10000, CRC(d91448ee) SHA1(7f84ca3605edcab4bf226dab8dd7218cd5c3e5a4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "6",    0x00000, 0x10000, CRC(5f39a980) SHA1(2a440f86685249f9c317634cad8cdedc8a8f1491) )

	ROM_REGION(0xa0000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "sbs-g.04",    0x00000, 0x20000, CRC(7c74cc4e) SHA1(92097b372eacabdb9e8e261b0bc4223821ff9273) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION(0xa0000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "sbs-g.05",    0x00000, 0x20000, CRC(f0a782e3) SHA1(d572226b8e597f1c34d246cb284e047a6e2d9290) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */
ROM_END

ROM_START( blkheart )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "blkhrt.7",  0x00000, 0x20000, CRC(5bd248c0) SHA1(0649f4f8682404aeb3fc80643fcabc2d7836bb23) )
	ROM_LOAD16_BYTE( "blkhrt.6",  0x00001, 0x20000, CRC(6449e50d) SHA1(d8cd126d921c95478346da96c20da01212395d77) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "4.bin",      0x00000, 0x10000, CRC(7cefa295) SHA1(408f46613b3620cee31dec43281688d231b47ddd) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3.bin",    0x000000, 0x020000, CRC(a1ab3a16) SHA1(3fb57c9d2ef94ee188cbadd70378ae6f4407e71d) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90068-5.bin", 0x000000, 0x100000, CRC(a1ab4f24) SHA1(b9f8104d53eda87ccd4000d049ee74ac9aa20b3e) )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "90068-8.bin", 0x000000, 0x100000, CRC(9d3204b2) SHA1(b37a246ad37f9ce092b371f01122ddf2bc8b2db6) )	/* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-2.bin", 0x00000, 0x20000, CRC(3a583184) SHA1(9226f1ea7725e4b48bb055d1c17389cf960d75f8) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-1.bin", 0x00000, 0x20000, CRC(e7af69d2) SHA1(da050880e186954bcf0e0adf00750dd5a371551b) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bpr",      0x0000, 0x0100, CRC(98ed1c97) SHA1(f125ad05c3cbd1b1ab356161f9b1d814781d4c3b) )	/* unknown */
	ROM_LOAD( "10.bpr",     0x0100, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* unknown */
ROM_END

ROM_START( blkhearj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "7.bin",  0x00000, 0x20000, CRC(e0a5c667) SHA1(3ef39b2dc1f7ffdddf586f0b3080ecd1f362ec37) )
	ROM_LOAD16_BYTE( "6.bin",  0x00001, 0x20000, CRC(7cce45e8) SHA1(72491e30d1f9be2eede21fdde5a7484d4f65cfbf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "4.bin",      0x00000, 0x10000, CRC(7cefa295) SHA1(408f46613b3620cee31dec43281688d231b47ddd) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3.bin",    0x000000, 0x020000, CRC(a1ab3a16) SHA1(3fb57c9d2ef94ee188cbadd70378ae6f4407e71d) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90068-5.bin", 0x000000, 0x100000, CRC(a1ab4f24) SHA1(b9f8104d53eda87ccd4000d049ee74ac9aa20b3e) )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "90068-8.bin", 0x000000, 0x100000, CRC(9d3204b2) SHA1(b37a246ad37f9ce092b371f01122ddf2bc8b2db6) )	/* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-2.bin", 0x00000, 0x20000, CRC(3a583184) SHA1(9226f1ea7725e4b48bb055d1c17389cf960d75f8) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-1.bin", 0x00000, 0x20000, CRC(e7af69d2) SHA1(da050880e186954bcf0e0adf00750dd5a371551b) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bpr",      0x0000, 0x0100, CRC(98ed1c97) SHA1(f125ad05c3cbd1b1ab356161f9b1d814781d4c3b) )	/* unknown */
	ROM_LOAD( "10.bpr",     0x0100, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* unknown */
ROM_END

ROM_START( tdragon )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code -bitswapped- */
	ROM_LOAD16_BYTE( "91070_68k.8",  0x00000, 0x20000, CRC(121c3ae7) SHA1(b88446df3b177d40e0b59a481f8e4de212e3afbc) )
	ROM_LOAD16_BYTE( "91070_68k.7",  0x00001, 0x20000, CRC(6e154d8e) SHA1(29baea24d670ab63149efe281de25cca15b7b863) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "91070.6",		0x000000, 0x20000, CRC(fe365920) SHA1(7581931cb95cd5a8ed40e4f5385b533e3d19af22) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "91070.5",		0x000000, 0x100000, CRC(d0bde826) SHA1(3b74d5fc88a4a9329e101ee72f393608d327d816) )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "91070.4",	0x000000, 0x100000, CRC(3eedc2fe) SHA1(9f48986c231a8fbc07f2b39b2017d1e967b2ed3c) )	/* Sprites */

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "91070.1",      0x00000, 0x10000, CRC(bf493d74) SHA1(6f8f5eff4b71fb6cabda10075cfa88a3f607859e) )

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "91070.3",     0x00000, 0x20000, CRC(ae6875a8) SHA1(bfdb350b3d3fce2bead1ac60875beafe427765ed) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "91070.2",     0x00000, 0x20000, CRC(ecfea43e) SHA1(d664dfa6698fec8e602523bdae16068f1ff6547b) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "91070.9",  0x0000, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* unknown */
	ROM_LOAD( "91070.10", 0x0100, 0x0100, CRC(e6ead349) SHA1(6d81b1c0233580aa48f9718bade42d640e5ef3dd) )	/* unknown */
ROM_END

ROM_START( tdragonb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code -bitswapped- */
	ROM_LOAD16_BYTE( "td_04.bin",  0x00000, 0x20000, CRC(e8a62d3e) SHA1(dd221bcd80149fffb1bdddfd3d394996bd2f8ec5) )
	ROM_LOAD16_BYTE( "td_03.bin",  0x00001, 0x20000, CRC(2fa1aa04) SHA1(ddf2b2ff179c31a1677d15d0403b00d77f9f0a6c) )

	ROM_REGION(0x20000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "td_02.bin",    0x00000, 0x8000, CRC(99ee7505) SHA1(b97c8ee5e26e8554b5de506fba3b32cc2fde53c9) )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "td_08.bin",		0x000000, 0x20000, CRC(5144dc69) SHA1(e64d88dc0e7672f811868621f74ec209aeafbc6f) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "td_06.bin",		0x000000, 0x80000, CRC(c1be8a4d) SHA1(6269fd7fccf1546a01bab755d8b6b7dcffc1166e) )	/* 16x16 tiles */
	ROM_LOAD( "td_07.bin",		0x080000, 0x80000, CRC(2c3e371f) SHA1(77956425661f4f81c370fff63845d42057fcaec3) )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "td_10.bin",	0x000000, 0x080000, CRC(bfd0ec5d) SHA1(7983661f74e8695f56e45c6e5c278d7d86431052) )	/* Sprites */
	ROM_LOAD16_BYTE( "td_09.bin",	0x000001, 0x080000, CRC(b6e074eb) SHA1(bdde068f03415391b5edaa42f1389df0f7eef899) )	/* Sprites */

	ROM_REGION( 0x010000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "td_01.bin",     0x00000, 0x10000, CRC(f6f6c4bf) SHA1(ea4cf74d968e254ae47c16c2f4c2f4bc1a528808) )
ROM_END

ROM_START( ssmissin )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ssm14.165",    0x00001, 0x20000, CRC(eda61b74) SHA1(6247682c27d2be7dff1fad407ccf86fe2a25f11c) )
	ROM_LOAD16_BYTE( "ssm15.166",    0x00000, 0x20000, CRC(aff15927) SHA1(258c2722ac7ca50360bfefa7b4e621373975a835) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ssm16.172",		0x000000, 0x20000, CRC(5cf6eb1f) SHA1(d406b11cf06ae1afc57a50685689e358e5677a45) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ssm17.147",		0x000000, 0x080000, CRC(c9c28455) SHA1(6a3e754aff3f368bde0e8905c33074084ad6ac30) )	/* 16x16 tiles */
	ROM_LOAD( "ssm18.148",		0x080000, 0x080000, CRC(ebfdaad6) SHA1(0814cdfe83f36a7dd7b5416f9d0478192733dac0) )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ssm20.34",		0x000001, 0x080000, CRC(a0c16c4d) SHA1(e198f69b4d8660e33851a2631b5411611b1b2ea6) )	/* 16x16 tiles */
	ROM_LOAD16_BYTE( "ssm19.33",		0x000000, 0x080000, CRC(b1943657) SHA1(97c05483b634315af338434bd2f565cc151a7283) )	/* 16x16 tiles */

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* Code for Sound CPU */
	ROM_LOAD( "ssm11.188",      0x00000, 0x08000, CRC(8be6dce3) SHA1(d9a235c36e0bc44025c291247d6b0b753e4bc0c8) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples? */
	ROM_LOAD( "ssm13.190",     0x00000, 0x20000, CRC(618f66f0) SHA1(97637a03d9fd82305e872e9bfa489862c974bb6c) )
	ROM_LOAD( "ssm12.189",     0x80000, 0x80000, CRC(e8219c83) SHA1(68673d071a58ca2bfd2de344a830417d10bc5757) ) /* banked */

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "ssm-pr2.113",  0x0000, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* unknown */
	ROM_LOAD( "ssm-pr1.114",  0x0100, 0x0200, CRC(ed0bd072) SHA1(66a6d435d8587c82ae96dd09c39ed5749fe00e24) )	/* unknown */
ROM_END

ROM_START( airattck )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 68000 Code */
	ROM_LOAD16_BYTE( "ue10.bin",     0x000000, 0x20000, CRC(71deb9d8) SHA1(21da5a68a13c9017d787e88f7b293f263fbc6b20) )
	ROM_LOAD16_BYTE( "uc10.bin",     0x000001, 0x20000, CRC(1837d4ba) SHA1(8dd5636a3a75c5d25d8850381e566a150ddc8ef1) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ul10.bin",     0x000000, 0x20000, CRC(e9362ab4) SHA1(d3e7d90e459bd4a80a189cc77821a6668103a640) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "uw9.bin",      0x000000, 0x80000, CRC(86e59966) SHA1(50944dddb4c9f28e6f9b7c610a205310f4d7a076) )
	ROM_LOAD( "ux9.bin",      0x080000, 0x80000, CRC(122c8d04) SHA1(70a348b1a94f1bc69532ba92dafc91a2c0e41d58) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "uob1.bin",     0x000000, 0x80000, CRC(3c38d671) SHA1(f9c9aaa1622ee0c20f569f6048e2b78bd507a1e5) )	/* 16x16 tiles */
	ROM_LOAD16_BYTE( "uob2.bin",     0x000001, 0x80000, CRC(9a83e3d8) SHA1(c765c4d278cc7f54ccdf6f00f8c6902a56abc2b8) )	/* 16x16 tiles */

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* Code for Sound CPU */
	ROM_LOAD( "su6.bin",      0x000000, 0x08000, CRC(3e352370) SHA1(6e84881dc0b09a23f8b589431005459adc334c34) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "su12.bin",     0x000000, 0x20000, CRC(93ab615b) SHA1(f670ac60f5f88148e55200e5e3591aa18b81c325) )
	ROM_LOAD( "su13.bin",     0x080000, 0x80000, CRC(09a836bb) SHA1(43fbd35c2ef3d201a4c82b0d3b7d7b971b385a14) )
ROM_END

ROM_START( strahl )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "strahl-2.82", 0x00000, 0x20000, CRC(c9d008ae) SHA1(e9218a3143d5887e702df051354a9083a806c69c) )
	ROM_LOAD16_BYTE( "strahl-1.83", 0x00001, 0x20000, CRC(afc3c4d6) SHA1(ab3dd7db692eb01e3a87f4216d322a702f3beaad) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "strahl-3.73",  0x000000, 0x10000, CRC(2273b33e) SHA1(fa53e91b80dfea3f8b2c1f0ce66e5c6920c4960f) ) /* Characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "str7b2r0.275", 0x000000, 0x40000, CRC(5769e3e1) SHA1(7d7a16b11027d0a7618df1ec1e3484224b772e90) ) /* Tiles */

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "strl3-01.32",  0x000000, 0x80000, CRC(d8337f15) SHA1(4df23fff2506b66a94dae4e0cf7d25499936b942) ) /* Sprites */
	ROM_LOAD( "strl4-02.57",  0x080000, 0x80000, CRC(2a38552b) SHA1(82335fc6aa3de9145dd84952e5ed423493bf7141) )
	ROM_LOAD( "strl5-03.58",  0x100000, 0x80000, CRC(a0e7d210) SHA1(96a762a3a1cdeaa91bde50429e0ac665fb81190b) )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "str6b1w1.776", 0x000000, 0x80000, CRC(bb1bb155) SHA1(83a02e89180e15f0e7817e0e92b4bf4e209bb69a) ) /* Tiles */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "strahl-4.66",    0x00000, 0x10000, CRC(60a799c4) SHA1(8ade3cf827a389f7cb4080957dc4d67077ea4166) )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "str8pmw1.540", 0x00000, 0x20000, CRC(01d6bb6a) SHA1(b157f6f921483ed8067a7e13e370f73fdb60d136) )
	ROM_CONTINUE(             0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0xa0000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "str9pew1.639", 0x00000, 0x20000, CRC(6bb3eb9f) SHA1(9c1394df4f8a08f9098c85eb3d38fb862d6eabbb) )
	ROM_CONTINUE(             0x40000, 0x60000 )	/* banked */
ROM_END

ROM_START( strahla )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rom2", 0x00000, 0x20000, CRC(f80a22ef) SHA1(22099eb0bbb445702e0276713c3e48d60de60c30) )
	ROM_LOAD16_BYTE( "rom1", 0x00001, 0x20000, CRC(802ecbfc) SHA1(cc776023c7bd6b6d6af9659a0c822a2887e50199) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "strahl-3.73",  0x000000, 0x10000, CRC(2273b33e) SHA1(fa53e91b80dfea3f8b2c1f0ce66e5c6920c4960f) ) /* Characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "str7b2r0.275", 0x000000, 0x40000, CRC(5769e3e1) SHA1(7d7a16b11027d0a7618df1ec1e3484224b772e90) ) /* Tiles */

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "strl3-01.32",  0x000000, 0x80000, CRC(d8337f15) SHA1(4df23fff2506b66a94dae4e0cf7d25499936b942) ) /* Sprites */
	ROM_LOAD( "strl4-02.57",  0x080000, 0x80000, CRC(2a38552b) SHA1(82335fc6aa3de9145dd84952e5ed423493bf7141) )
	ROM_LOAD( "strl5-03.58",  0x100000, 0x80000, CRC(a0e7d210) SHA1(96a762a3a1cdeaa91bde50429e0ac665fb81190b) )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "str6b1w1.776", 0x000000, 0x80000, CRC(bb1bb155) SHA1(83a02e89180e15f0e7817e0e92b4bf4e209bb69a) ) /* Tiles */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "strahl-4.66",    0x00000, 0x10000, CRC(60a799c4) SHA1(8ade3cf827a389f7cb4080957dc4d67077ea4166) )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "str8pmw1.540", 0x00000, 0x20000, CRC(01d6bb6a) SHA1(b157f6f921483ed8067a7e13e370f73fdb60d136) )
	ROM_CONTINUE(             0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0xa0000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "str9pew1.639", 0x00000, 0x20000, CRC(6bb3eb9f) SHA1(9c1394df4f8a08f9098c85eb3d38fb862d6eabbb) )
	ROM_CONTINUE(             0x40000, 0x60000 )	/* banked */
ROM_END

ROM_START( hachamf )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "7.93",  0x00000, 0x20000, CRC(9d847c31) SHA1(1d370d8db9cadadb9c2cb213e32f681947d81b7f) )
	ROM_LOAD16_BYTE( "6.94",  0x00001, 0x20000, CRC(de6408a0) SHA1(2df77fecd44d2d8b0444abd4545923213ed76b2d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* unknown  - sound cpu ?????? */
	ROM_LOAD( "1.70",  0x00000, 0x10000, CRC(9e6f48fc) SHA1(aeb5bfecc025b5478f6de874792fc0f7f54932be) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5.95",  0x000000, 0x020000, CRC(29fb04a2) SHA1(9654b90a66d0e2a0f9cd369cab29cdd0c6f77869) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* 16x16 tiles */
	ROM_LOAD( "91076-4.101",  0x000000, 0x100000, CRC(df9653a4) SHA1(4a3204a98d7738c7895169fcece922fdf355f4fa) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "91076-8.57",  0x000000, 0x100000, CRC(7fd0f556) SHA1(d1b4bec0946869d3d7bcb870d9ae3bd17395a231) )	/* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "91076-2.46",   0x00000, 0x20000, CRC(3f1e67f2) SHA1(413e78587d8a043a0eb94447313ba1b3c5b35be5) )
	ROM_CONTINUE(             0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "91076-3.45",   0x00000, 0x20000, CRC(b25ed93b) SHA1(d7bc686bbccf982f40420a11158aa8e5dd4207c5) )
	ROM_CONTINUE(             0x40000, 0x60000 )	/* banked */
ROM_END

ROM_START( hachamfb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "8.bin",  0x00000, 0x20000, CRC(14845b65) SHA1(5cafd07a8a6f5ccbb36de7a90571f8b33ecf273e) )
	ROM_LOAD16_BYTE( "7.bin",  0x00001, 0x20000, CRC(069ca579) SHA1(0db4c3c41e17fca613d11de89b388a4af206ec6b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* unknown  - sound cpu ?????? */
	ROM_LOAD( "1.70",  0x00000, 0x10000, CRC(9e6f48fc) SHA1(aeb5bfecc025b5478f6de874792fc0f7f54932be) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5.95",  0x000000, 0x020000, CRC(29fb04a2) SHA1(9654b90a66d0e2a0f9cd369cab29cdd0c6f77869) )   /* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* 16x16 tiles */
	ROM_LOAD( "91076-4.101",  0x000000, 0x100000, CRC(df9653a4) SHA1(4a3204a98d7738c7895169fcece922fdf355f4fa) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "91076-8.57",  0x000000, 0x100000, CRC(7fd0f556) SHA1(d1b4bec0946869d3d7bcb870d9ae3bd17395a231) ) /* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "91076-2.46",   0x00000, 0x20000, CRC(3f1e67f2) SHA1(413e78587d8a043a0eb94447313ba1b3c5b35be5) ) /* 1st & 2nd half identical, needs verifying */
	ROM_CONTINUE(             0x40000, 0x60000 )    /* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "91076-3.45",   0x00000, 0x20000, CRC(b25ed93b) SHA1(d7bc686bbccf982f40420a11158aa8e5dd4207c5) ) /* 1st & 2nd half identical, needs verifying */
	ROM_CONTINUE(             0x40000, 0x60000 )    /* banked */
ROM_END

ROM_START( macross )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "921a03",        0x00000, 0x80000, CRC(33318d55) SHA1(c99f85e09bd334dc8ce138b08cbed2331b0d67dd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* sound program (unknown CPU) */
	ROM_LOAD( "921a02",      0x00000, 0x10000, CRC(77c082c7) SHA1(be07aa14d0116f830f98e11a19f1debb48a5230e) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "921a01",      0x000000, 0x020000, CRC(bbd8242d) SHA1(7cf4897be1278e1190f499f00bc78384817a5160) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "921a04",      0x000000, 0x200000, CRC(4002e4bb) SHA1(281433d798ac85c84d4f1f3751a3032e8a3b5cd4) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "921a07",      0x000000, 0x200000, CRC(7d2bf112) SHA1(1997c99c2d3998096842abd1cee89e0e6ab43a47) )	/* Sprites */

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "921a05",      0x00000, 0x20000, CRC(d5a1eddd) SHA1(42b5b255f02b9c6d856b1578af9a5dfc51ea6ebb) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0xa0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "921a06",      0x00000, 0x20000, CRC(89461d0f) SHA1(b7d27d0ee0b7ab44c20ab710b567f64fc3afb90c) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "921a08",      0x0000, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* unknown */
	ROM_LOAD( "921a09",      0x0100, 0x0100, CRC(633ab1c9) SHA1(acd99fcca41eaab7948ca84988352f1d7d519c61) )	/* unknown */
	ROM_LOAD( "921a10",      0x0200, 0x0020, CRC(8371e42d) SHA1(6cfd70dfa00e85ec1df8832d41df331cc3e3733a) )	/* unknown */
ROM_END

ROM_START( gunnail )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "3e.bin",  0x00000, 0x40000, CRC(61d985b2) SHA1(96daca603f18accb47f98a3e584b2c84fc5a2ca4) )
	ROM_LOAD16_BYTE( "3o.bin",  0x00001, 0x40000, CRC(f114e89c) SHA1(a12f5278167f446bb5277e87289c41b5aa365c86) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "92077_2.bin",      0x00000, 0x10000, CRC(cd4e55f8) SHA1(92182767ca0ec37ec4949bd1a88c2efdcdcb60ed) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin",    0x000000, 0x020000, CRC(3d00a9f4) SHA1(91a82e3e74c8774d7f8b2adceb228b97010facfd) )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "92077-4.bin", 0x000000, 0x100000, CRC(a9ea2804) SHA1(14dbdb3c7986db5e44dc7c5be6fcf39f3d1e50b0) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "92077-7.bin", 0x000000, 0x200000, CRC(d49169b3) SHA1(565ff7725dd6ace79b55706114132d8d867e81a9) )	/* Sprites */

	ROM_REGION( 0x0a0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "92077-5.bin", 0x00000, 0x20000, CRC(feb83c73) SHA1(b44e9d20b4af02e218c4bc875d66a7d6b8551cae) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0a0000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "92077-6.bin", 0x00000, 0x20000, CRC(6d133f0d) SHA1(8a5e6e27a297196f20e4de0d060f1188115809bb) )
	ROM_CONTINUE(            0x40000, 0x60000 )	/* banked */

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "8.bpr",      0x0000, 0x0100, CRC(4299776e) SHA1(683d14d2ace14965f0fcfe0f0540c1b77d2cece5) )	/* unknown */
	ROM_LOAD( "9.bpr",      0x0100, 0x0100, CRC(633ab1c9) SHA1(acd99fcca41eaab7948ca84988352f1d7d519c61) )	/* unknown */
	ROM_LOAD( "10.bpr",     0x0200, 0x0020, CRC(c60103c8) SHA1(dfb05b704bb5e1f75f5aaa4fa36e8ddcc905f8b6) )	/* unknown */
ROM_END

ROM_START( macross2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mcrs2j.3",      0x00000, 0x80000, CRC(36a618fe) SHA1(56fdb2bcb4a39888cfbaf9692d66335524a6ac0c) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "mcrs2j.2",    0x00000, 0x20000, CRC(b4aa8ac7) SHA1(73a6de56cbfb468450d9b39fcbae0362f242f37b) )
	ROM_RELOAD(              0x10000, 0x20000 )				/* banked */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcrs2j.1",    0x000000, 0x020000, CRC(c7417410) SHA1(41431d8f1ff4d66baf1a8518a0b0c0125d1d71d4) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bp932an.a04", 0x000000, 0x200000, CRC(c4d77ff0) SHA1(aca60a3f5f89265e7e3799e5d80ea8196fb11ff3) )	/* 16x16 tiles */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "bp932an.a07", 0x000000, 0x200000, CRC(aa1b21b9) SHA1(133822e3d8628aa4eb3e62fbd054956799423b98) )	/* Sprites */
	ROM_LOAD16_WORD_SWAP( "bp932an.a08", 0x200000, 0x200000, CRC(67eb2901) SHA1(25e0f9fda1a8c0c2b59616dd153cb6dcb459d2d9) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "bp932an.a06", 0x040000, 0x200000, CRC(ef0ffec0) SHA1(fd72cc77e02d1a00bf27e77a33d7dab5f6ba1cb4) )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "bp932an.a05", 0x040000, 0x100000, CRC(b5335abb) SHA1(f4eaf4e465eeca31741d432ee46ed39ffcd92cca) )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "mcrs2bpr.9",  0x0000, 0x0100, CRC(435653a2) SHA1(575b4a46ea65179de3042614da438d2f6d8b572e) )	/* unknown */
	ROM_LOAD( "mcrs2bpr.10", 0x0100, 0x0100, CRC(e6ead349) SHA1(6d81b1c0233580aa48f9718bade42d640e5ef3dd) )	/* unknown */
ROM_END

ROM_START( tdragon2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "6.bin",      0x00000, 0x80000, CRC(310d6bca) SHA1(f46ad1d13cf5014aef1f0e8862b369ab31c22866) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "5.bin",    0x00000, 0x20000, CRC(b870be61) SHA1(ea5d45c3a3ab805e55806967f00167cf6366212e) )
	ROM_RELOAD(              0x10000, 0x20000 )				/* banked */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin",    0x000000, 0x020000, CRC(d488aafa) SHA1(4d05e7ca075b638dd90ae4c9f224817a8a3ae9f3) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ww930914.2", 0x000000, 0x200000, CRC(f968c65d) SHA1(fd6d21bba53f945b1597d7d0735bc62dd44d5498) )	/* 16x16 tiles */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ww930917.7", 0x000000, 0x200000, CRC(b98873cb) SHA1(cc19200865176e940ff68e12de81f029b51c2084) )	/* Sprites */
	ROM_LOAD16_WORD_SWAP( "ww930918.8", 0x200000, 0x200000, CRC(baee84b2) SHA1(b325b00e6147266dbdc840e03556004531dc2038) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ww930916.4", 0x040000, 0x200000, CRC(07c35fe6) SHA1(33547bd88764704310f2ef8cf3bfe21ceb56d5b7) )	/* all banked */

	ROM_REGION( 0x240000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ww930915.3", 0x040000, 0x200000, CRC(82025bab) SHA1(ac6053700326ea730d00ec08193e2c8a2a019f0b) )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bpr",  0x0000, 0x0100, CRC(435653a2) SHA1(575b4a46ea65179de3042614da438d2f6d8b572e) )	/* unknown */
	ROM_LOAD( "10.bpr", 0x0100, 0x0100, CRC(e6ead349) SHA1(6d81b1c0233580aa48f9718bade42d640e5ef3dd) )	/* unknown */
ROM_END

ROM_START( bigbang )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "eprom.3",      0x00000, 0x80000, CRC(28e5957a) SHA1(fe4f870a9c2235cc02b4e036a2a4116f071d59ad) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "5.bin",    0x00000, 0x20000, CRC(b870be61) SHA1(ea5d45c3a3ab805e55806967f00167cf6366212e) )
	ROM_RELOAD(              0x10000, 0x20000 )				/* banked */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin",    0x000000, 0x020000, CRC(d488aafa) SHA1(4d05e7ca075b638dd90ae4c9f224817a8a3ae9f3) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ww930914.2", 0x000000, 0x200000, CRC(f968c65d) SHA1(fd6d21bba53f945b1597d7d0735bc62dd44d5498) )	/* 16x16 tiles */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ww930917.7", 0x000000, 0x200000, CRC(b98873cb) SHA1(cc19200865176e940ff68e12de81f029b51c2084) )	/* Sprites */
	ROM_LOAD16_WORD_SWAP( "ww930918.8", 0x200000, 0x200000, CRC(baee84b2) SHA1(b325b00e6147266dbdc840e03556004531dc2038) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ww930916.4", 0x040000, 0x200000, CRC(07c35fe6) SHA1(33547bd88764704310f2ef8cf3bfe21ceb56d5b7) )	/* all banked */

	ROM_REGION( 0x240000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ww930915.3", 0x040000, 0x200000, CRC(82025bab) SHA1(ac6053700326ea730d00ec08193e2c8a2a019f0b) )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bpr",  0x0000, 0x0100, CRC(435653a2) SHA1(575b4a46ea65179de3042614da438d2f6d8b572e) )	/* unknown */
	ROM_LOAD( "10.bpr", 0x0100, 0x0100, CRC(e6ead349) SHA1(6d81b1c0233580aa48f9718bade42d640e5ef3dd) )	/* unknown */
ROM_END

/*

Rapid Hero
NMK, 1994

The main board has no ROMs at all except 3 PROMs. There is a plug-in daughter
board that holds all the ROMs. It has the capacity for 3 socketed EPROMS and 7x
16M MASK ROMs total.


PCB Layout (Main board)
-----------------------

AWA94099
-----------------------------------------------------------------------
|    YM2203  TMP90C841 6264 DSW2(8) 62256 62256 6116 62256 62256 6116 |
|   6295 NMK112      12MHz     DSW1(8)                                |
|  YM3014B 6295      16MHz NMK005   62256 62256 6116 62256 62256 6116 |
|J       -----------------                                            |
|A       |               |                                            |
|M       -----------------                                            |
|M                   PROM3                           NMK009  NMK009   |
|A   NMK111   6116                    6116  NMK008                    |
|       |-|   6116                    6116                            |
|6116   | | NMK902   -----------------                                |
|6116   | |          |               |                                |
|PROM1  | |          -----------------                                |
|       | |                                                           |
|NMK111 | | NMK903                                                    |
|       | | NMK903       PROM2                                        |
|NMK111 | |                                                           |
|       |-|                   6116                  TMP68HC000P-16    |
| 62256     NMK901            6116                             14MHz  |
| 62256                                                               |
-----------------------------------------------------------------------

Notes:
      68k clock: 14.00MHz
          VSync: 56Hz
          HSync: 15.35kHz
   90c841 clock: 8.000MHz


PCB Layout (Daughter board)
---------------------------

AWA94099-ROME
--------------------------
| 2      6   7   5    3  |
|                        |
| 1                      |
|                        |
|                        |
|                        |
| 4           8   9   10 |
|                        |
|                        |
|                        |
--------------------------

*/

ROM_START( raphero )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rhp94099.3",      0x00000, 0x80000, CRC(ec9b4f05) SHA1(e5bd797620dc449fd78b41d87e9ba5a764eb8b44) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* tmp90c841 */
	ROM_LOAD( "rhp94099.2",    0x00000, 0x20000, CRC(fe01ece1) SHA1(c469fb79f2774089848c814f92ddd3c9e384050f) )
	ROM_RELOAD(                0x10000, 0x20000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rhp94099.1",    0x000000, 0x020000, CRC(55a7a011) SHA1(87ded56bfdd38cbf8d3bd8b3789831f768550a12) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rhp94099.4", 0x000000, 0x200000,  CRC(076eee7b) SHA1(7c315fe33d0fcd92e0ce2f274996c8059228b005) )	/* 16x16 tiles */

	ROM_REGION( 0x600000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD16_WORD_SWAP( "rhp94099.8", 0x000000, 0x200000, CRC(49892f07) SHA1(2f5d20cd193cffcba9041aa11d6665adebeffffa) )	/* 16x16 tiles */
	ROM_LOAD16_WORD_SWAP( "rhp94099.9", 0x200000, 0x200000, CRC(ea2e47f0) SHA1(97dfa8f95f27b36deb5ce1c80e3d727bad24e52b) )	/* 16x16 tiles */
	ROM_LOAD16_WORD_SWAP( "rhp94099.10",0x400000, 0x200000, CRC(512cb839) SHA1(4a2c5ac88e4bf8a6f07c703277c4d33e649fd192) )	/* 16x16 tiles */

	ROM_REGION( 0x440000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "rhp94099.6", 0x040000, 0x200000, CRC(f1a80e5a) SHA1(218bd7b0c3d8b283bf96b95bf888228810699370) )  /* all banked */
	ROM_LOAD( "rhp94099.7", 0x240000, 0x200000, CRC(0d99547e) SHA1(2d9630bd55d27010f9d1d2dbdbd07ac265e8ebe6) )  /* all banked */
 
	ROM_REGION( 0x440000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "rhp94099.5", 0x040000, 0x200000, CRC(515eba93) SHA1(c35cb5f31f4bc7327be5777624af168f9fb364a5) )	/* all banked */
	ROM_LOAD( "rhp94099.6", 0x240000, 0x200000, CRC(f1a80e5a) SHA1(218bd7b0c3d8b283bf96b95bf888228810699370) )	/* all banked */

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "prom1.u19",      0x0000, 0x0100, CRC(4299776e) SHA1(683d14d2ace14965f0fcfe0f0540c1b77d2cece5) ) /* unknown */
	ROM_LOAD( "prom2.u53",      0x0100, 0x0100, CRC(e6ead349) SHA1(6d81b1c0233580aa48f9718bade42d640e5ef3dd) ) /* unknown */
	ROM_LOAD( "prom3.u60",      0x0200, 0x0100, CRC(304f98c6) SHA1(8dfd9bf719087ec30c83efe95c4561666c7d1801) ) /* unknown */
ROM_END

ROM_START( sabotenb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "ic76.sb1",  0x00000, 0x40000, CRC(b2b0b2cf) SHA1(219f1cefdb107d8404f4f8bfa0700fd3218d9320) )
	ROM_LOAD16_BYTE( "ic75.sb2",  0x00001, 0x40000, CRC(367e87b7) SHA1(c950041529b5117686e4bb1ae77db82fe758c1d0) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic35.sb3",		0x000000, 0x010000, CRC(eb7bc99d) SHA1(b3063afd58025a441d4750c22483e9129da402e7) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic32.sb4",		0x000000, 0x200000, CRC(24c62205) SHA1(3ab0ca5d7c698328d91421ccf6f7dafc20df3c8d) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ic100.sb5",	0x000000, 0x200000, CRC(b20f166e) SHA1(074d770fd6d233040a80a92f4467d81f961c650b) )	/* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic30.sb6",    0x040000, 0x100000, CRC(288407af) SHA1(78c08fae031337222681c593dc86a08df6a34a4b) )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic27.sb7",    0x040000, 0x100000, CRC(43e33a7e) SHA1(51068b63f4415712eaa25dcf1ee6b0cc2850974e) )	/* all banked */
ROM_END

ROM_START( sabotnba )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "sb1.76",  0x00000, 0x40000, CRC(df6f65e2) SHA1(6ad9e9f13539310646895c5e7992c6546e75684b) )
	ROM_LOAD16_BYTE( "sb2.75",  0x00001, 0x40000, CRC(0d2c1ab8) SHA1(abb43a8c5398195c0ad48d8d772ef47635bf25c2) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic35.sb3",		0x000000, 0x010000, CRC(eb7bc99d) SHA1(b3063afd58025a441d4750c22483e9129da402e7) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic32.sb4",		0x000000, 0x200000, CRC(24c62205) SHA1(3ab0ca5d7c698328d91421ccf6f7dafc20df3c8d) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ic100.sb5",	0x000000, 0x200000, CRC(b20f166e) SHA1(074d770fd6d233040a80a92f4467d81f961c650b) )	/* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic30.sb6",    0x040000, 0x100000, CRC(288407af) SHA1(78c08fae031337222681c593dc86a08df6a34a4b) )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic27.sb7",    0x040000, 0x100000, CRC(43e33a7e) SHA1(51068b63f4415712eaa25dcf1ee6b0cc2850974e) )	/* all banked */
ROM_END

ROM_START( bjtwin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )  /* 68000 code */
	ROM_LOAD16_BYTE( "93087-1.bin",  0x00000, 0x20000, CRC(93c84e2d) SHA1(ad0755cabfef78e7e689856379d6f8c88a9b27c1) )
	ROM_LOAD16_BYTE( "93087-2.bin",  0x00001, 0x20000, CRC(30ff678a) SHA1(aa3ce4905e448e371e254545ef9ed7edb00b1cc3) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "93087-3.bin",  0x000000, 0x010000, CRC(aa13df7c) SHA1(162d4f12364c68028e86fe97ee75c262daa4c699) ) /* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "93087-4.bin",  0x000000, 0x100000, CRC(8a4f26d0) SHA1(be057a2b6d28c623ac1f16cf02ddbe12ca430b4a) ) /* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "93087-5.bin", 0x000000, 0x100000, CRC(bb06245d) SHA1(c91e2284d95370b8ef2eb1b9d6305fdd6cde23a0) ) /* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1, 0 ) /* OKIM6295 samples */
	ROM_LOAD( "93087-6.bin",    0x040000, 0x100000, CRC(372d46dd) SHA1(18f44e777241af50787730652fa018c51b65ea15) ) /* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 ) /* OKIM6295 samples */
	ROM_LOAD( "93087-7.bin",    0x040000, 0x100000, CRC(8da67808) SHA1(f042574c097f5a8c2684fcc23f2c817c168254ef) ) /* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "8.bpr",      0x0000, 0x0100, CRC(633ab1c9) SHA1(acd99fcca41eaab7948ca84988352f1d7d519c61) ) /* unknown */
	ROM_LOAD( "9.bpr",      0x0000, 0x0100, CRC(435653a2) SHA1(575b4a46ea65179de3042614da438d2f6d8b572e) ) /* unknown */
ROM_END


ROM_START( nouryoku )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "ic76.1",  0x00000, 0x40000, CRC(26075988) SHA1(c3d0eef0417be3f78008c026915fd7e2fd589563) )
	ROM_LOAD16_BYTE( "ic75.2",  0x00001, 0x40000, CRC(75ab82cd) SHA1(fb828f87eebbe9d61766535efc18de9dfded110c) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic35.3",		0x000000, 0x010000, CRC(03d0c3b1) SHA1(4d5427c324e2141d0a953cc5133d10b327827e0b) )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic32.4",		0x000000, 0x200000, CRC(88d454fd) SHA1(c79c48d9b3602266499a5dd0b15fd2fb032809be) )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ic100.5",	0x000000, 0x200000, CRC(24d3e24e) SHA1(71e38637953ec98bf308824aaef5628803aead21) )	/* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic30.6",     0x040000, 0x100000, CRC(feea34f4) SHA1(bee467e74dbad497c6f5f6b38b7e52001e767012) )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic27.7",     0x040000, 0x100000, CRC(8a69fded) SHA1(ee73f1789bcc672232606a4b3b28087fea1c5c69) )	/* all banked */
ROM_END

ROM_START( manybloc )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "1-u33.bin",  0x00001, 0x20000, CRC(07473154) SHA1(e67f637e74dfe5f1be558f963c0b3225254afe33) )
	ROM_LOAD16_BYTE( "2-u35.bin",  0x00000, 0x20000, CRC(04acd8c1) SHA1(3ef329e8d25565c7f7166f12137f4df5a057022f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80? CPU */
	ROM_LOAD( "3-u146.bin",      0x00000, 0x10000, CRC(7bf5fafa) SHA1(d17feca628775860d6c7019a9725bd40fbc5b7d7) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "12-u39.bin",    0x000000, 0x10000, CRC(413b5438) SHA1(af366ce998ebe0d25255cc0cb1cd81689d3696ec) )	/* 8x8 tiles */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5-u97.bin", 0x000000, 0x40000, CRC(536699e6) SHA1(13ec233f5e4f2a65ac7bc55511e988508269acd5) )
	ROM_LOAD( "4-u96.bin", 0x040000, 0x40000, CRC(28af2640) SHA1(08fa57de66cf58fe2256455538261c2d05d27e1e) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 sprite tiles */
	ROM_LOAD16_BYTE( "8-u54b.bin",  0x000000, 0x20000, CRC(03eede77) SHA1(2476a488bb0d39790b2cc7f261ddb973378022ff) )
	ROM_LOAD16_BYTE( "10-u86b.bin", 0x000001, 0x20000, CRC(9eab216f) SHA1(616f3ee2d06aa7151af634773a5e8633bff9588e) )
	ROM_LOAD16_BYTE( "9-u53b.bin",  0x040000, 0x20000, CRC(dfcfa040) SHA1(f1561defe9746afdb1a5327d0a4435a6f3e87a77) )
	ROM_LOAD16_BYTE( "11-u85b.bin", 0x040001, 0x20000, CRC(fe747dd5) SHA1(6ba57a45f4d77e2574de95d4a2f0718c601e7214) )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "6-u131.bin",  0x00000, 0x20000, CRC(79a4ae75) SHA1(f7609d0ca18b4af8c5f37daa1795a7a6c6d768ae) )
	ROM_CONTINUE(            0x40000, 0x20000 )	/* banked */
	ROM_LOAD( "7-u132.bin",  0x60000, 0x40000, CRC(21db875e) SHA1(e1d96155b6d8825f7c449f276d02f9769258345d) )	/* banked */

	ROM_REGION( 0xa0000, REGION_SOUND2, ROMREGION_ERASE00 )	/* OKIM6295 samples */
	/* empty */

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "u200.bpr",    0x0000, 0x0020, CRC(1823600b) SHA1(7011156ebcb815b176856bd67898ce655ea1b5ab) )	/* unknown */
	ROM_LOAD( "u7.bpr",      0x0020, 0x0100, CRC(cfdbb86c) SHA1(588822f6308a860937349c9106c2b4b1a75823ec) )	/* unknown */
	ROM_LOAD( "u10.bpr",     0x0120, 0x0200, CRC(8e9b569a) SHA1(1d8d633fbeb72d5e55ad4b282df02e9ca5e240eb) )	/* unknown */
	ROM_LOAD( "u120.bpr",    0x0320, 0x0100, CRC(576c5984) SHA1(6e9b7f30de0d91cb766a62abc5888ec9af085a27) )	/* unknown */
ROM_END



static unsigned char decode_byte(unsigned char src, unsigned char *bitp)
{
	unsigned char ret, i;

	ret = 0;
	for (i=0; i<8; i++)
		ret |= (((src >> bitp[i]) & 1) << (7-i));

	return ret;
}

static unsigned long bjtwin_address_map_bg0(unsigned long addr)
{
   return ((addr&0x00004)>> 2) | ((addr&0x00800)>> 10) | ((addr&0x40000)>>16);
}


static unsigned short decode_word(unsigned short src, unsigned char *bitp)
{
	unsigned short ret, i;

	ret=0;
	for (i=0; i<16; i++)
		ret |= (((src >> bitp[i]) & 1) << (15-i));

	return ret;
}


static unsigned long bjtwin_address_map_sprites(unsigned long addr)
{
   return ((addr&0x00010)>> 4) | ((addr&0x20000)>>16) | ((addr&0x100000)>>18);
}


static void decode_gfx(void)
{
	/* GFX are scrambled.  We decode them here.  (BIG Thanks to Antiriad for descrambling info) */
	unsigned char *rom;
	int A;

	static unsigned char decode_data_bg[8][8] =
	{
		{0x3,0x0,0x7,0x2,0x5,0x1,0x4,0x6},
		{0x1,0x2,0x6,0x5,0x4,0x0,0x3,0x7},
		{0x7,0x6,0x5,0x4,0x3,0x2,0x1,0x0},
		{0x7,0x6,0x5,0x0,0x1,0x4,0x3,0x2},
		{0x2,0x0,0x1,0x4,0x3,0x5,0x7,0x6},
		{0x5,0x3,0x7,0x0,0x4,0x6,0x2,0x1},
		{0x2,0x7,0x0,0x6,0x5,0x3,0x1,0x4},
		{0x3,0x4,0x7,0x6,0x2,0x0,0x5,0x1},
	};

	static unsigned char decode_data_sprite[8][16] =
	{
		{0x9,0x3,0x4,0x5,0x7,0x1,0xb,0x8,0x0,0xd,0x2,0xc,0xe,0x6,0xf,0xa},
		{0x1,0x3,0xc,0x4,0x0,0xf,0xb,0xa,0x8,0x5,0xe,0x6,0xd,0x2,0x7,0x9},
		{0xf,0xe,0xd,0xc,0xb,0xa,0x9,0x8,0x7,0x6,0x5,0x4,0x3,0x2,0x1,0x0},
		{0xf,0xe,0xc,0x6,0xa,0xb,0x7,0x8,0x9,0x2,0x3,0x4,0x5,0xd,0x1,0x0},

		{0x1,0x6,0x2,0x5,0xf,0x7,0xb,0x9,0xa,0x3,0xd,0xe,0xc,0x4,0x0,0x8}, /* Haze 20/07/00 */
		{0x7,0x5,0xd,0xe,0xb,0xa,0x0,0x1,0x9,0x6,0xc,0x2,0x3,0x4,0x8,0xf}, /* Haze 20/07/00 */
		{0x0,0x5,0x6,0x3,0x9,0xb,0xa,0x7,0x1,0xd,0x2,0xe,0x4,0xc,0x8,0xf}, /* Antiriad, Corrected by Haze 20/07/00 */
		{0x9,0xc,0x4,0x2,0xf,0x0,0xb,0x8,0xa,0xd,0x3,0x6,0x5,0xe,0x1,0x7}, /* Antiriad, Corrected by Haze 20/07/00 */
	};


	/* background */
	rom = memory_region(REGION_GFX2);
	for (A = 0;A < memory_region_length(REGION_GFX2);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_bg[bjtwin_address_map_bg0(A)]);
	}

	/* sprites */
	rom = memory_region(REGION_GFX3);
	for (A = 0;A < memory_region_length(REGION_GFX3);A += 2)
	{
		unsigned short tmp = decode_word( rom[A+1]*256 + rom[A], decode_data_sprite[bjtwin_address_map_sprites(A)]);
		rom[A+1] = tmp >> 8;
		rom[A] = tmp & 0xff;
	}
}

static void decode_tdragonb(void)
{
	/* Descrambling Info Again Taken from Raine, Huge Thanks to Antiriad and the Raine Team for
       going Open Source, best of luck in future development. */

	unsigned char *rom;
	int A;

	/* The Main 68k Program of the Bootleg is Bitswapped */
	static unsigned char decode_data_tdragonb[1][16] =
	{
		{0xe,0xc,0xa,0x8,0x7,0x5,0x3,0x1,0xf,0xd,0xb,0x9,0x6,0x4,0x2,0x0},
	};

	/* Graphic Roms Could Also Do With Rearranging to make things simpler */
	static unsigned char decode_data_tdragonbgfx[1][8] =
	{
		{0x7,0x6,0x5,0x3,0x4,0x2,0x1,0x0},
	};

	rom = memory_region(REGION_CPU1);
	for (A = 0;A < memory_region_length(REGION_CPU1);A += 2)
	{
#ifdef LSB_FIRST
		unsigned short tmp = decode_word( rom[A+1]*256 + rom[A], decode_data_tdragonb[0]);
		rom[A+1] = tmp >> 8;
		rom[A] = tmp & 0xff;
#else
		unsigned short tmp = decode_word( rom[A]*256 + rom[A+1], decode_data_tdragonb[0]);
		rom[A] = tmp >> 8;
		rom[A+1] = tmp & 0xff;
#endif
	}

	rom = memory_region(REGION_GFX2);
	for (A = 0;A < memory_region_length(REGION_GFX2);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}

	rom = memory_region(REGION_GFX3);
	for (A = 0;A < memory_region_length(REGION_GFX3);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}
}

static void decode_ssmissin(void)
{
	/* Like Thunder Dragon Bootleg without the Program Rom Swapping */
	unsigned char *rom;
	int A;

	/* Graphic Roms Could Also Do With Rearranging to make things simpler */
	static unsigned char decode_data_tdragonbgfx[1][8] =
	{
		{0x7,0x6,0x5,0x3,0x4,0x2,0x1,0x0},
	};

	rom = memory_region(REGION_GFX2);
	for (A = 0;A < memory_region_length(REGION_GFX2);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}

	rom = memory_region(REGION_GFX3);
	for (A = 0;A < memory_region_length(REGION_GFX3);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}
}


static DRIVER_INIT( nmk )
{
	decode_gfx();
}

static DRIVER_INIT( hachamf )
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);

	//rom[0x0006/2] = 0x7dc2;   /* replace reset vector with the "real" one */

	// kludge the sound communication to let commands go through.
	rom[0x048a/2] = 0x4e71;
	rom[0x04aa/2] = 0x4e71;

	hachamf_mcu_type = 0;
}


static DRIVER_INIT( hachamfb )
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);

	rom[0x048a/2] = 0x4e71;
	rom[0x04aa/2] = 0x4e71;

}


static DRIVER_INIT( tdragonb )
{
	UINT16 *ROM = (UINT16 *)memory_region(REGION_CPU1);

	decode_tdragonb();

	/* The Following Patch is taken from Raine, Otherwise the game has no Sprites in Attract Mode or After Level 1
       which is rather odd considering its a bootleg.. */
	ROM[0x00308/2] = 0x4e71; /* Sprite Problem */
}

static DRIVER_INIT( tdragon )
{
	UINT16 *RAM = (UINT16 *)memory_region(REGION_CPU1);

	RAM[0x048a/2] = 0x4e71;
	RAM[0x04aa/2] = 0x4e71;

	hachamf_mcu_type = 1;
}

static DRIVER_INIT( ssmissin )
{
	decode_ssmissin();
}

static DRIVER_INIT( bjtwin )
{
	init_nmk();
}

GAME( 1989, tharrier, 0,       tharrier, tharrier, 0, 	     ROT270, "UPL (American Sammy license)",	    "Task Force Harrier", 0 )
GAME( 1989, tharierj, tharrier,tharrier, tharrier, 0,        ROT270, "UPL",	                            "Task Force Harrier (Japan)", 0 )
GAME( 1990, mustang,  0,       mustang,  mustang,  0,        ROT0,   "UPL",				    "US AAF Mustang (Japan)", GAME_IMPERFECT_SOUND)
GAME( 1990, mustangs, mustang, mustang,  mustang,  0,        ROT0,   "UPL (Seoul Trading license)",	    "US AAF Mustang (Seoul Trading)", GAME_IMPERFECT_SOUND )
GAME( 1990, mustangb, mustang, mustang,  mustang,  0,        ROT0,   "bootleg",				    "US AAF Mustang (bootleg)", GAME_UNEMULATED_PROTECTION ) // Playable but there are Still Protection Problems
GAME( 1990, bioship,  0,       bioship,  bioship,  0,        ROT0,   "UPL (American Sammy license)",	    "Bio-ship Paladin", GAME_IMPERFECT_SOUND )
GAME( 1990, vandyke,  0,       vandyke,  vandyke,  0,        ROT270, "UPL",				    "Vandyke (Japan)",  GAME_IMPERFECT_SOUND )
GAME( 1990, vandyjal, vandyke, vandyke,  vandyke,  0,        ROT270, "UPL (Jaleco license)",                "Vandyke (Jaleco, Set 1)",  GAME_IMPERFECT_SOUND )
GAME( 1990, vandyja2, vandyke, vandyke,  vandyke,  0,        ROT270, "UPL (Jaleco license)",                "Vandyke (Jaleco, Set 2)",  GAME_IMPERFECT_SOUND )
GAME( 1991, manybloc, 0,       manybloc, manybloc, 0,        ROT270, "Bee-Oh",                              "Many Block", GAME_NO_COCKTAIL | GAME_IMPERFECT_SOUND )
GAME( 1991, blkheart, 0,       macross,  blkheart, 0,        ROT0,   "UPL",				    "Black Heart", GAME_IMPERFECT_SOUND  )
GAME( 1991, blkhearj, blkheart,macross,  blkheart, 0,        ROT0,   "UPL",				    "Black Heart (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1991, acrobatm, 0,       acrobatm, acrobatm, 0,        ROT270, "UPL (Taito license)",		    "Acrobat Mission", GAME_IMPERFECT_SOUND )
GAME( 1992, strahl,   0,       strahl,   strahl,   0,        ROT0,   "UPL",				    "Koutetsu Yousai Strahl (Japan set 1)", GAME_IMPERFECT_SOUND )
GAME( 1992, strahla,  strahl,  strahl,   strahl,   0,        ROT0,   "UPL",			            "Koutetsu Yousai Strahl (Japan set 2)", GAME_IMPERFECT_SOUND )
GAME( 1991, tdragon,  0,       tdragon,  tdragon,  tdragon,  ROT270, "NMK (Tecmo license)",		    "Thunder Dragon (unprotected)", GAME_IMPERFECT_SOUND )
GAME( 1991, tdragonb, tdragon, tdragonb, tdragonb, tdragonb, ROT270, "NMK / Tecmo",			    "Thunder Dragon (Bootleg)", 0 )
GAME( 1992, ssmissin, 0,       ssmissin, ssmissin, ssmissin, ROT270, "Comad",				    "S.S. Mission", GAME_NO_COCKTAIL )
GAME( 1996, airattck, 0,       ssmissin, airattck, ssmissin, ROT270, "Comad",				    "Air Attack", GAME_NO_COCKTAIL )
GAME( 1991, hachamf,  0,       hachamf,  hachamf,  hachamf,  ROT0,   "NMK",		                    "Hacha Mecha Fighter", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1991, hachamfb, hachamf, hachamfb, hachamfb, hachamfb, ROT0,   "bootleg",                             "Hacha Mecha Fighter (unprotected)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) // appears to be a Thunder Dragon conversion, could be bootleg?
GAME( 1992, macross,  0,       macross,  macross,  nmk,      ROT270, "Banpresto",			    "Super Spacefortress Macross / Chou-Jikuu Yousai Macross", GAME_IMPERFECT_SOUND )
GAME( 1993, gunnail,  0,       gunnail,  gunnail,  nmk,      ROT270, "NMK / Tecmo",			    "GunNail", GAME_IMPERFECT_SOUND )
GAME( 1993, macross2, 0,       macross2, macross2, 0,        ROT0,   "Banpresto",			    "Super Spacefortress Macross II / Chou-Jikuu Yousai Macross II", GAME_NO_COCKTAIL )
GAME( 1993, tdragon2, 0,       tdragon2, tdragon2, 0,        ROT270, "NMK",				    "Thunder Dragon 2", GAME_NO_COCKTAIL )
GAME( 1993, bigbang,  tdragon2,tdragon2, tdragon2, 0,        ROT270, "NMK",				    "Big Bang", GAME_NO_COCKTAIL )
GAME( 1994, raphero,  0,       raphero,  tdragon2, 0,        ROT270, "Media Trading Corp",                  "Rapid Hero (Japan)", 0 ) // 23rd July 1993 in test mode, (c)1994 on title screen

GAME( 1992, sabotenb, 0,       bjtwin,   sabotenb, nmk,      ROT0,   "NMK / Tecmo",			    "Saboten Bombers (set 1)", GAME_NO_COCKTAIL )
GAME( 1992, sabotnba, sabotenb,bjtwin,   sabotenb, nmk,      ROT0,   "NMK / Tecmo",			    "Saboten Bombers (set 2)", GAME_NO_COCKTAIL )
GAME( 1993, bjtwin,   0,       bjtwin,   bjtwin,   bjtwin,   ROT270, "NMK",				    "Bombjack Twin", GAME_NO_COCKTAIL )
GAME( 1995, nouryoku, 0,       bjtwin,   nouryoku, nmk,      ROT0,   "Tecmo",				    "Nouryoku Koujou Iinkai", GAME_NO_COCKTAIL )

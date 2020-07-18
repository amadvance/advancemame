/***************************************************************************

                            -= Kaneko 16 Bit Games =-

                    driver by   Luca Elia (l.elia@tin.it)


CPU    :  68000  +  MCU [Optional]

SOUND  :  OKI-M6295 x (1 | 2) + YM2149 x (0 | 2)
   OR  :  Z80  +  YM2151

OTHER  :  93C46 EEPROM [Optional]

CUSTOM :  VU-001 046A                  (48pin PQFP)
          VU-002 052 151021            (160pin PQFP)    <- Sprites
          VU-003                                        <- High Colour Background
          VIEW2-CHIP 23160-509 9047EAI (144pin PQFP)    <- Tilemaps
          MUX2-CHIP                    (64pin PQFP)
          HELP1-CHIP                   (64pin PQFP)
          IU-001 9045KP002             (44pin PQFP)
          I/O JAMMA MC-8282 047        (46pin)          <- Inputs


----------------------------------------------------------------------------------------
Year + Game                    PCB         Notes
----------------------------------------------------------------------------------------
91  The Berlin Wall
    Magical Crystals           Z00FC-02
92  Bakuretsu Breaker          ZOOFC-02
    Blaze On                   Z02AT-002   2 VU-002 Sprites Chips (Atlus PCB ID: ATL-67140)
    Shogun Warriors            ZO1DK-002   CALC3 MCU protection (EEPROM handling, 68k code snippet, data - palettes, tilemaps, fighters)
    B.Rap Boys                 ZO1DK-002   CALC3 MCU protection (EEPROM handling, 68k code snippet, data - palettes, tilemaps, fighters)
94  Great 1000 Miles Rally                 TBSOP01 MCU protection (EEPROM handling etc.)
    Bonk's Adventure           Z09AF-003   TBSOP01 MCU protection (EEPROM handling, 68k code snippet, data)
    Blood Warrior              Z09AF-005   TBSOP01 MCU protection (EEPROM handling etc.)
95  Great 1000 Miles Rally 2   M201F00138  TBSOP02 MCU protection (EEPROM handling etc.)
----------------------------------------------------------------------------------------

Note: gtmr manual shows "Compatible with AX Kaneko System Board"
Note: Magic Crystals reports "TOYBOX SYSTEM Version 0.93B+"

Note: Decapping shows the CALC3 MCU to be a NEC uPD78322 series MCU with 16K internal ROM.  Both the
      TBSOP01 and TBSOP02 are thought to be the NEC uPD78324 series MCU with 32K internal ROM.

To Do:

[gtmr]
- Stage 4: The layers' scrolling is very jerky for a couple of seconds
  in the middle of this level (probably interrupt related)
- The layers' colours are not initialised when showing the self test
  screen and the very first screen (with the Kaneko logo in the middle).
  They're probably supposed to be disabled in those occasions, but the
  relevant registers aren't changed throughout the game (?)

[gtmr2]
- Finish the Inputs (different wheels and pedals)
- Find infos about the communication stuff (even if it won't be supported)

[brapboys / shogwarr]

- Verify collision protection on real hardware (appears to be a function of the system/memory controller, NOT the MCU)
  - currently B.Rap Boys has it's own special case, it should work with the same code as Blood Warriors / Shogun Warriors (and probably SuperNova)
- Figure out how MCU resets writeback address (currently hacked)
- Find relationship between Key tables and final datablock
- Clean up debug code file writes etc. (when above are done only!)
- Interrupt timing? (some sprites flicker)
- Sprite buffering (1-2 frames?, or related to above)

Dip locations verified from manual for:

- berlwall
- mgcrystl
- blazeon
- bakubrkr
- bloodwar
- gtmr
- gtmr2
- shogwarr

[general]
- replace sample bank copying with new ADDRESS MAP system for OKI and do banking like CPUs

***************************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "includes/kaneko16.h"
#include "sound/2203intf.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

static int shogwarr_mcu_status, shogwarr_mcu_command_offset;

extern UINT16 *kaneko16_mcu_ram; /* for calc3 and toybox */
static UINT16* kaneko16_mainram;

extern void (*toybox_mcu_run)(void);	/* one of the following */
void bloodwar_mcu_run(void);
void bonkadv_mcu_run(void);
void gtmr_mcu_run(void);
void calc3_mcu_run(void);

extern void calc3_scantables(void);
extern void decrypt_toybox_rom(void);




/***************************************************************************


                            Machine Initialisation


***************************************************************************/

MACHINE_RESET( kaneko16 )
{
	kaneko16_sprite_type  = 0;

	kaneko16_sprite_xoffs = 0;
	kaneko16_sprite_yoffs = 0;
	
	kaneko16_sprite_fliptype = 0;

/*
    Sx = Sprites with priority x, x = tiles with priority x,
    Sprites - Tiles Order (bottom -> top):

    0   S0  1   2   3
    0   1   S1  2   3
    0   1   2   S2  3
    0   1   2   3   S3
*/
	kaneko16_priority.sprite[0] = 1;	// above tile[0],   below the others
	kaneko16_priority.sprite[1] = 2;	// above tile[0-1], below the others
	kaneko16_priority.sprite[2] = 3;	// above tile[0-2], below the others
	kaneko16_priority.sprite[3] = 8;	// above all
}

static MACHINE_RESET( berlwall )
{
	machine_reset_kaneko16();

	kaneko16_sprite_type = 2;	// like type 0, but using 16 instead of 8 bytes
}

static MACHINE_RESET( blazeon )
{
	machine_reset_kaneko16();

	kaneko16_sprite_xoffs = 0x10000 - 0x680;
	kaneko16_sprite_yoffs = 0x000;

/*
    Sx = Sprites with priority x, x = tiles with priority x,
    Sprites - Tiles Order (bottom -> top):

    0   S0  1   2   3
    0   1   S1  2   3
    0   1   2   3   S2
    0   1   2   3   S3
*/
	kaneko16_priority.sprite[0] = 1;	// above tile[0], below the others
	kaneko16_priority.sprite[1] = 2;	// above tile[0-1], below the others
	kaneko16_priority.sprite[2] = 8;	// above all
	kaneko16_priority.sprite[3] = 8;	// ""
}

static MACHINE_RESET( bloodwar )
{
	machine_reset_kaneko16();
	
	kaneko16_priority.sprite[0] = 2;	// ever used ?
	kaneko16_priority.sprite[1] = 3;	// character selection / vs. portraits
	kaneko16_priority.sprite[2] = 5;	// winning portrait
	kaneko16_priority.sprite[3] = 7;

	kaneko16_sprite_type = 1;

	kaneko16_priority.VIEW2_2_pri = 1;

	toybox_mcu_run = bloodwar_mcu_run;
	toybox_mcu_init();
}

static MACHINE_RESET( bonkadv )
{
	machine_reset_kaneko16();
	
	kaneko16_priority.sprite[0] = 2;	// ever used ?
	kaneko16_priority.sprite[1] = 3;	// volcano lava on level 2
	kaneko16_priority.sprite[2] = 5;	// in-game player
	kaneko16_priority.sprite[3] = 7;	// demostration text

	kaneko16_sprite_type = 1;

	kaneko16_priority.VIEW2_2_pri = 1;

	toybox_mcu_run = bonkadv_mcu_run;
	toybox_mcu_init();
}

static MACHINE_RESET( bakubrkr )
{
	machine_reset_kaneko16();

	kaneko16_priority.sprite[0] = 8;	// above all
	kaneko16_priority.sprite[1] = 8;	// above all
	kaneko16_priority.sprite[2] = 8;	// above all
	kaneko16_priority.sprite[3] = 8;	// above all

	kaneko16_priority.VIEW2_2_pri = 1;
}

static MACHINE_RESET( gtmr )
{
	machine_reset_kaneko16();

	kaneko16_sprite_type = 1;

	kaneko16_priority.VIEW2_2_pri = 1;

	toybox_mcu_run = gtmr_mcu_run;
	toybox_mcu_init();
}

static MACHINE_RESET( mgcrystl )
{
	machine_reset_kaneko16();
/*
    Sx = Sprites with priority x, x = tiles with priority x,
    Sprites - Tiles Order:

    S0: below 0 2

    S1: over  2
        below 0

    S2: over 0 2

    S3: over all

    tiles of the 2nd VIEW2 chip always behind sprites?

*/
	kaneko16_priority.sprite[0] = 2;	// below all
	kaneko16_priority.sprite[1] = 3;	// above tile[0], below the others
	kaneko16_priority.sprite[2] = 5;	// above all
	kaneko16_priority.sprite[3] = 7;	// ""

	kaneko16_priority.VIEW2_2_pri = 0;
}

static MACHINE_RESET( sandscrp )
{
	machine_reset_kaneko16();

	kaneko16_sprite_type = 3;	// "different" sprites layout

	watchdog_reset16_r(0,0);	// start with an armed watchdog
}


static MACHINE_RESET( shogwarr )
{
	machine_reset_kaneko16();

	kaneko16_sprite_xoffs = 0xa00;

	kaneko16_sprite_yoffs = -0x40;

	kaneko16_sprite_type = 0;
	kaneko16_sprite_fliptype = 1;

	shogwarr_mcu_status = 0;
	shogwarr_mcu_command_offset = 0;

	kaneko16_priority.sprite[0] = 1;	// below all
	kaneko16_priority.sprite[1] = 3;	// above tile[0], below the others
	kaneko16_priority.sprite[2] = 5;	// above all
	kaneko16_priority.sprite[3] = 7;	// ""
	kaneko16_priority.VIEW2_2_pri = 0;

	calc3_mcu_init();
	calc3_scantables();
}

static MACHINE_RESET( brapboys )
{
	machine_reset_kaneko16();

	kaneko16_sprite_xoffs = 0xa00;

	kaneko16_sprite_yoffs = -0x40;

	kaneko16_sprite_type = 0;
	kaneko16_sprite_fliptype = 1;

	shogwarr_mcu_status = 0;
	shogwarr_mcu_command_offset = 0;

	kaneko16_priority.sprite[0] = 1;	// below all
	kaneko16_priority.sprite[1] = 3;	// above tile[0], below the others
	kaneko16_priority.sprite[2] = 5;	// above all
	kaneko16_priority.sprite[3] = 7;	// ""
	kaneko16_priority.VIEW2_2_pri = 0;

	calc3_mcu_init();
	calc3_scantables();
}

/***************************************************************************


                        Misc Machine Emulation Routines


***************************************************************************/

READ16_HANDLER( kaneko16_rnd_r )
{
	return mame_rand() & 0xffff;
}

WRITE16_HANDLER( kaneko16_coin_lockout_w )
{
	if (ACCESSING_MSB)
	{
		coin_counter_w(0,   data  & 0x0100);
		coin_counter_w(1,   data  & 0x0200);
		coin_lockout_w(0, (~data) & 0x0400 );
		coin_lockout_w(1, (~data) & 0x0800 );
	}
}

/* Sand Scorpion */

static UINT8 sprite_irq;
static UINT8 unknown_irq;
static UINT8 vblank_irq;


/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	if (vblank_irq || sprite_irq || unknown_irq)
		cpunum_set_input_line(0, 1, ASSERT_LINE);
	else
		cpunum_set_input_line(0, 1, CLEAR_LINE);
}


/* Called once/frame to generate the VBLANK interrupt */
static INTERRUPT_GEN( sandscrp_interrupt )
{
	vblank_irq = 1;
	update_irq_state();
}


static VIDEO_EOF( sandscrp )
{
	sprite_irq = 1;
	update_irq_state();
}

/* Reads the cause of the interrupt */
static READ16_HANDLER( sandscrp_irq_cause_r )
{
	return 	( sprite_irq  ?  0x08  : 0 ) |
			( unknown_irq ?  0x10  : 0 ) |
			( vblank_irq  ?  0x20  : 0 ) ;
}


/* Clear the cause of the interrupt */
static WRITE16_HANDLER( sandscrp_irq_cause_w )
{
	if (ACCESSING_LSB)
	{
		kaneko16_sprite_flipx	=	data & 1;
		kaneko16_sprite_flipy	=	data & 1;

		if (data & 0x08)	sprite_irq  = 0;
		if (data & 0x10)	unknown_irq = 0;
		if (data & 0x20)	vblank_irq  = 0;
	}

	update_irq_state();
}


/***************************************************************************


                                    Sound


***************************************************************************/

WRITE16_HANDLER( kaneko16_soundlatch_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(0, (data & 0xff00) >> 8 );
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

/* Two identically mapped YM2149 chips */

READ16_HANDLER( kaneko16_YM2149_0_r )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_0_w(0,offset);
	return AY8910_read_port_0_r(0);
}
READ16_HANDLER( kaneko16_YM2149_1_r )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_1_w(0,offset);
	return AY8910_read_port_1_r(0);
}

WRITE16_HANDLER( kaneko16_YM2149_0_w )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_0_w(0,offset);
	/* The registers are mapped to odd addresses, except one! */
	if (ACCESSING_LSB)	AY8910_write_port_0_w(0, data       & 0xff);
	else				AY8910_write_port_0_w(0,(data >> 8) & 0xff);
}
WRITE16_HANDLER( kaneko16_YM2149_1_w )
{
	/* Each 2149 register is mapped to a different address */
	AY8910_control_port_1_w(0,offset);
	/* The registers are mapped to odd addresses, except one! */
	if (ACCESSING_LSB)	AY8910_write_port_1_w(0, data       & 0xff);
	else				AY8910_write_port_1_w(0,(data >> 8) & 0xff);
}


/***************************************************************************


                                    EEPROM


***************************************************************************/

READ8_HANDLER( kaneko16_eeprom_r )
{
	return EEPROM_read_bit() & 1;
}

WRITE8_HANDLER( kaneko16_eeprom_reset_w )
{
	// reset line asserted: reset.
	EEPROM_set_cs_line((data & 0x01) ? CLEAR_LINE : ASSERT_LINE );
}

WRITE16_HANDLER( kaneko16_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		// latch the bit
		EEPROM_write_bit(data & 0x02);

		// reset line asserted: reset.
//      EEPROM_set_cs_line((data & 0x00) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x01) ? ASSERT_LINE : CLEAR_LINE );
	}

	if (ACCESSING_MSB)
	{
		coin_counter_w(0, data & 0x0100);
		coin_counter_w(1, data & 0x0200);
		coin_lockout_w(0, data & 0x8000);
		coin_lockout_w(1, data & 0x8000);
	}
}


/***************************************************************************


                            Memory Maps - Main CPU


***************************************************************************/

/***************************************************************************
                                The Berlin Wall
***************************************************************************/

static ADDRESS_MAP_START( berlwall, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM		// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM		// Work RAM
	AM_RANGE(0x30e000, 0x30ffff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)		// Sprites
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(MRA16_RAM, paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
//  AM_RANGE(0x480000, 0x480001) AM_RAM     // ?
	AM_RANGE(0x500000, 0x500001) AM_READWRITE(kaneko16_bg15_reg_r, kaneko16_bg15_reg_w) AM_BASE(&kaneko16_bg15_reg)	// High Color Background
	AM_RANGE(0x580000, 0x580001) AM_READWRITE(kaneko16_bg15_select_r, kaneko16_bg15_select_w) AM_BASE(&kaneko16_bg15_select)
	AM_RANGE(0x600000, 0x60003f) AM_READWRITE(MRA16_RAM, kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs)	// Sprites Regs
	AM_RANGE(0x680000, 0x680001) AM_READ(input_port_0_word_r)		// Inputs
	AM_RANGE(0x680002, 0x680003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x680004, 0x680005) AM_READ(input_port_2_word_r)
//  AM_RANGE(0x680006, 0x680007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x700000, 0x700001) AM_WRITE(kaneko16_coin_lockout_w)	// Coin Lockout
	AM_RANGE(0x780000, 0x780001) AM_READ(watchdog_reset16_r)		// Watchdog
	AM_RANGE(0x800000, 0x80001f) AM_READWRITE(kaneko16_YM2149_0_r, kaneko16_YM2149_0_w)	// Sound
	AM_RANGE(0x800200, 0x80021f) AM_READWRITE(kaneko16_YM2149_1_r, kaneko16_YM2149_1_w)
	AM_RANGE(0x8003fe, 0x8003ff) AM_NOP // for OKI when accessed as .l
	AM_RANGE(0x800400, 0x800401) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0xc00000, 0xc00fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers
	AM_RANGE(0xc01000, 0xc01fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0xc02000, 0xc02fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0xc03000, 0xc03fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//
	AM_RANGE(0xd00000, 0xd0001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers Regs
ADDRESS_MAP_END


/***************************************************************************
                            Bakuretsu Breaker
***************************************************************************/

/* Two un-assigned OKI samples get requested during game play. Leftover bug? */
/* The two YM2149 chips are only used when entering high score initials, and */
/* when the game is fully completed. Overkill??? */

WRITE16_HANDLER( bakubrkr_oki_bank_sw )
{
	if (ACCESSING_LSB) {
		OKIM6295_set_bank_base(0, 0x40000 * (data & 0x7) );
		logerror("PC:%06X  Selecting OKI bank %02X\n",activecpu_get_pc(),data&0xff);
	}
}

static ADDRESS_MAP_START( bakubrkr, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM		// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM		// Work RAM
	AM_RANGE(0x400000, 0x40001f) AM_READ(kaneko16_YM2149_0_r)	// Sound
	AM_RANGE(0x400000, 0x40001d) AM_WRITE(kaneko16_YM2149_0_w)
	AM_RANGE(0x40001e, 0x40001f) AM_WRITE(bakubrkr_oki_bank_sw)	// OKI bank Switch
	AM_RANGE(0x400200, 0x40021f) AM_READWRITE(kaneko16_YM2149_1_r,kaneko16_YM2149_1_w)			// Sound
	AM_RANGE(0x400400, 0x400401) AM_READWRITE(OKIM6295_status_0_lsb_r,OKIM6295_data_0_lsb_w)	//
	AM_RANGE(0x500000, 0x500fff) AM_READWRITE(MRA16_RAM,kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x501000, 0x501fff) AM_READWRITE(MRA16_RAM,kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0x502000, 0x502fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0x503000, 0x503fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//
	AM_RANGE(0x580000, 0x580fff) AM_READWRITE(MRA16_RAM,kaneko16_vram_3_w) AM_BASE(&kaneko16_vram_3)	// Layers 1
	AM_RANGE(0x581000, 0x581fff) AM_READWRITE(MRA16_RAM,kaneko16_vram_2_w) AM_BASE(&kaneko16_vram_2)	//
	AM_RANGE(0x582000, 0x582fff) AM_RAM AM_BASE(&kaneko16_vscroll_3)									//
	AM_RANGE(0x583000, 0x583fff) AM_RAM AM_BASE(&kaneko16_vscroll_2)									//
	AM_RANGE(0x600000, 0x601fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)					// Sprites
	AM_RANGE(0x700000, 0x700fff) AM_READWRITE(MRA16_RAM,paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x800000, 0x80001f) AM_READWRITE(MRA16_RAM,kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers 0 Regs
	AM_RANGE(0x900000, 0x90001f) AM_READWRITE(MRA16_RAM,kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs	)	// Sprites Regs
	AM_RANGE(0xa80000, 0xa80001) AM_READ(watchdog_reset16_r)	// Watchdog
	AM_RANGE(0xb00000, 0xb0001f) AM_READWRITE(MRA16_RAM,kaneko16_layers_1_regs_w) AM_BASE(&kaneko16_layers_1_regs)	// Layers 1 Regs
	AM_RANGE(0xd00000, 0xd00001) AM_WRITE(kaneko16_eeprom_w)	// EEPROM
	AM_RANGE(0xe00000, 0xe00001) AM_READ(input_port_0_word_r)	// Inputs
	AM_RANGE(0xe00002, 0xe00003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xe00004, 0xe00005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xe00006, 0xe00007) AM_READ(input_port_3_word_r)
ADDRESS_MAP_END


/***************************************************************************
                                    Blaze On
***************************************************************************/

static ADDRESS_MAP_START( blazeon, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM		// ROM
	AM_RANGE(0x300000, 0x30ffff) AM_RAM		// Work RAM
	AM_RANGE(0x500000, 0x500fff) AM_READWRITE(MRA16_RAM, paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x600000, 0x600fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x601000, 0x601fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0x602000, 0x602fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0x603000, 0x603fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//
	AM_RANGE(0x700000, 0x700fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)					// Sprites
	AM_RANGE(0x800000, 0x80001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers 0 Regs
	AM_RANGE(0x900000, 0x90001f) AM_READWRITE(MRA16_RAM, kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs)	// Sprites Regs #1
	AM_RANGE(0x980000, 0x98001f) AM_RAM																				// Sprites Regs #2
	AM_RANGE(0xc00000, 0xc00001) AM_READ(input_port_0_word_r)		// Inputs
	AM_RANGE(0xc00002, 0xc00003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xc00004, 0xc00005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xc00006, 0xc00007) AM_READ(input_port_3_word_r)
	AM_RANGE(0xd00000, 0xd00001) AM_WRITE(kaneko16_coin_lockout_w)	// Coin Lockout
	AM_RANGE(0xe00000, 0xe00001) AM_READWRITE(MRA16_NOP, kaneko16_soundlatch_w)	// Read = IRQ Ack ?
	AM_RANGE(0xe40000, 0xe40001) AM_READ(MRA16_NOP)	// IRQ Ack ?
//  AM_RANGE(0xe80000, 0xe80001) AM_READ(MRA16_NOP) // IRQ Ack ?
	AM_RANGE(0xec0000, 0xec0001) AM_READ(MRA16_NOP)	// Lev 4 IRQ Ack ?
ADDRESS_MAP_END


/***************************************************************************
                                Blood Warrior
***************************************************************************/

static WRITE16_HANDLER( bloodwar_oki_0_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x40000 * (data & 0x3) );
//		logerror("CPU #0 PC %06X : OKI0  bank %08X\n",activecpu_get_pc(),data);
	}
}

static WRITE16_HANDLER( bloodwar_oki_1_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(1, 0x40000 * (data & 0x3) );
//		logerror("CPU #0 PC %06X : OKI1  bank %08X\n",activecpu_get_pc(),data);
	}
}

static WRITE16_HANDLER( bloodwar_coin_lockout_w )
{
	if (ACCESSING_MSB)
	{
		coin_counter_w(0, data & 0x0100);
		coin_counter_w(1, data & 0x0200);
		coin_lockout_w(0, data & 0x8000);
		coin_lockout_w(1, data & 0x8000);
	}
}

static ADDRESS_MAP_START( bloodwar, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM		// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM		// Work RAM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM AM_BASE(&kaneko16_mcu_ram)		// Shared With MCU
	AM_RANGE(0x2a0000, 0x2a0001) AM_WRITE(toybox_mcu_com0_w)	// To MCU ?
	AM_RANGE(0x2b0000, 0x2b0001) AM_WRITE(toybox_mcu_com1_w)
	AM_RANGE(0x2c0000, 0x2c0001) AM_WRITE(toybox_mcu_com2_w)
	AM_RANGE(0x2d0000, 0x2d0001) AM_WRITE(toybox_mcu_com3_w)
	AM_RANGE(0x300000, 0x30ffff) AM_READWRITE(MRA16_RAM, paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x400000, 0x401fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)					// Sprites
	AM_RANGE(0x500000, 0x500fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x501000, 0x501fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0x502000, 0x502fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0x503000, 0x503fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//
	AM_RANGE(0x580000, 0x580fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_3_w) AM_BASE(&kaneko16_vram_3)	// Layers 1
	AM_RANGE(0x581000, 0x581fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_2_w) AM_BASE(&kaneko16_vram_2)	//
	AM_RANGE(0x582000, 0x582fff) AM_RAM AM_BASE(&kaneko16_vscroll_3)									//
	AM_RANGE(0x583000, 0x583fff) AM_RAM AM_BASE(&kaneko16_vscroll_2)									//
	AM_RANGE(0x600000, 0x60001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers 0 Regs
	AM_RANGE(0x680000, 0x68001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_1_regs_w) AM_BASE(&kaneko16_layers_1_regs)	// Layers 1 Regs
	AM_RANGE(0x700000, 0x70001f) AM_READWRITE(MRA16_RAM, kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs)	// Sprites Regs
	AM_RANGE(0x800000, 0x800001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x880000, 0x880001) AM_READWRITE(OKIM6295_status_1_lsb_r, OKIM6295_data_1_lsb_w)
	AM_RANGE(0x900000, 0x900039) AM_READWRITE(bloodwar_calc_r, bloodwar_calc_w)
	AM_RANGE(0xa00000, 0xa00001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)	// Watchdog
	AM_RANGE(0xb00000, 0xb00001) AM_READ(input_port_0_word_r)	// Inputs
	AM_RANGE(0xb00002, 0xb00003) AM_READ(input_port_1_word_r)	//
	AM_RANGE(0xb00004, 0xb00005) AM_READ(input_port_2_word_r)	//
	AM_RANGE(0xb00006, 0xb00007) AM_READ(input_port_3_word_r)	//
	AM_RANGE(0xb80000, 0xb80001) AM_WRITE(bloodwar_coin_lockout_w)	// Coin Lockout
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(kaneko16_display_enable)
	AM_RANGE(0xd00000, 0xd00001) AM_READ(toybox_mcu_status_r)
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(bloodwar_oki_0_bank_w)
	AM_RANGE(0xe80000, 0xe80001) AM_WRITE(bloodwar_oki_1_bank_w)
ADDRESS_MAP_END


/***************************************************************************
                                Bonk's Adventure
***************************************************************************/

static WRITE16_HANDLER( bonkadv_oki_0_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x40000 * (data & 0xF));
		logerror("CPU #0 PC %06X : OKI0  bank %08X\n",activecpu_get_pc(),data);
	}
}

static WRITE16_HANDLER( bonkadv_oki_1_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(1, 0x40000 * data );
		logerror("CPU #0 PC %06X : OKI1  bank %08X\n",activecpu_get_pc(),data);
	}
}


static ADDRESS_MAP_START( bonkadv, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM		// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM		// Work RAM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM AM_BASE(&kaneko16_mcu_ram)		// Shared With MCU
	AM_RANGE(0x2a0000, 0x2a0001) AM_WRITE(toybox_mcu_com0_w)	// To MCU ?
	AM_RANGE(0x2b0000, 0x2b0001) AM_WRITE(toybox_mcu_com1_w)
	AM_RANGE(0x2c0000, 0x2c0001) AM_WRITE(toybox_mcu_com2_w)
	AM_RANGE(0x2d0000, 0x2d0001) AM_WRITE(toybox_mcu_com3_w)
	AM_RANGE(0x300000, 0x30ffff) AM_READWRITE(MRA16_RAM, paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x400000, 0x401fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)					// Sprites
	AM_RANGE(0x500000, 0x500fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x501000, 0x501fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0x502000, 0x502fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0x503000, 0x503fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//
	AM_RANGE(0x580000, 0x580fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_3_w) AM_BASE(&kaneko16_vram_3)	// Layers 1
	AM_RANGE(0x581000, 0x581fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_2_w) AM_BASE(&kaneko16_vram_2)	//
	AM_RANGE(0x582000, 0x582fff) AM_RAM AM_BASE(&kaneko16_vscroll_3)									//
	AM_RANGE(0x583000, 0x583fff) AM_RAM AM_BASE(&kaneko16_vscroll_2)									//
	AM_RANGE(0x600000, 0x60001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers 0 Regs
	AM_RANGE(0x680000, 0x68001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_1_regs_w) AM_BASE(&kaneko16_layers_1_regs)	// Layers 1 Regs
	AM_RANGE(0x700000, 0x70001f) AM_READWRITE(MRA16_RAM, kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs)	// Sprites Regs
	AM_RANGE(0x800000, 0x800001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x880000, 0x880001) AM_READWRITE(OKIM6295_status_1_lsb_r, OKIM6295_data_1_lsb_w)
	AM_RANGE(0x900000, 0x900015) AM_READWRITE(galpanib_calc_r,galpanib_calc_w)
	AM_RANGE(0xa00000, 0xa00001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)	// Watchdog
	AM_RANGE(0xb00000, 0xb00001) AM_READ(input_port_0_word_r)	// Inputs
	AM_RANGE(0xb00002, 0xb00003) AM_READ(input_port_1_word_r)	//
	AM_RANGE(0xb00004, 0xb00005) AM_READ(input_port_2_word_r)	//
	AM_RANGE(0xb00006, 0xb00007) AM_READ(input_port_3_word_r)	//
	AM_RANGE(0xb80000, 0xb80001) AM_WRITE(bloodwar_coin_lockout_w)	// Coin Lockout
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(kaneko16_display_enable)
	AM_RANGE(0xd00000, 0xd00001) AM_READ(toybox_mcu_status_r)
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(bonkadv_oki_0_bank_w)
	AM_RANGE(0xe80000, 0xe80001) AM_WRITE(bonkadv_oki_1_bank_w)
ADDRESS_MAP_END


/***************************************************************************
                            Great 1000 Miles Rally
***************************************************************************/


READ16_HANDLER( gtmr_wheel_r )
{
	if ( (readinputport(4) & 0x1800) == 0x10)	// DSW setting
		return	readinputport(5)<<8;			// 360' Wheel
	else
		return	readinputport(5);				// 270' Wheel
}

WRITE16_HANDLER( gtmr_oki_0_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x40000 * (data & 0xF) );
//      logerror("CPU #0 PC %06X : OKI0 bank %08X\n",activecpu_get_pc(),data);
	}
}

WRITE16_HANDLER( gtmr_oki_1_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(1, 0x40000 * (data & 0x1) );
//      logerror("CPU #0 PC %06X : OKI1 bank %08X\n",activecpu_get_pc(),data);
	}
}

WRITE16_HANDLER( gtmr_oki_0_data_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_data_0_w(0,data);
//      logerror("CPU #0 PC %06X : OKI0 <- %08X\n",activecpu_get_pc(),data);
	}
}

WRITE16_HANDLER( gtmr_oki_1_data_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_data_1_w(0,data);
//      logerror("CPU #0 PC %06X : OKI1 <- %08X\n",activecpu_get_pc(),data);
	}
}

static ADDRESS_MAP_START( gtmr_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0ffffd) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x0ffffe, 0x0fffff) AM_READ(gtmr_wheel_r				)	// Wheel Value
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM					)	// Work RAM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// Shared With MCU
	AM_RANGE(0x300000, 0x30ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x310000, 0x327fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x400000, 0x401fff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0x500000, 0x503fff) AM_READ(MRA16_RAM					)	// Layers 0
	AM_RANGE(0x580000, 0x583fff) AM_READ(MRA16_RAM					)	// Layers 1
	AM_RANGE(0x600000, 0x60000f) AM_READ(MRA16_RAM					)	// Layers 0 Regs
	AM_RANGE(0x680000, 0x68000f) AM_READ(MRA16_RAM					)	// Layers 1 Regs
	AM_RANGE(0x700000, 0x70001f) AM_READ(kaneko16_sprites_regs_r	)	// Sprites Regs
	AM_RANGE(0x800000, 0x800001) AM_READ(OKIM6295_status_0_lsb_r	)	// Samples
	AM_RANGE(0x880000, 0x880001) AM_READ(OKIM6295_status_1_lsb_r	)
	AM_RANGE(0x900014, 0x900015) AM_READ(kaneko16_rnd_r			)	// Random Number ?
	AM_RANGE(0xa00000, 0xa00001) AM_READ(watchdog_reset16_r		)	// Watchdog
	AM_RANGE(0xb00000, 0xb00001) AM_READ(input_port_0_word_r		)	// Inputs
	AM_RANGE(0xb00002, 0xb00003) AM_READ(input_port_1_word_r		)
	AM_RANGE(0xb00004, 0xb00005) AM_READ(input_port_2_word_r		)
	AM_RANGE(0xb00006, 0xb00007) AM_READ(input_port_3_word_r		)
	AM_RANGE(0xd00000, 0xd00001) AM_READ(toybox_mcu_status_r		)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gtmr_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM					)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM					)	// Work RAM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM) AM_BASE(&kaneko16_mcu_ram		)	// Shared With MCU
	AM_RANGE(0x2a0000, 0x2a0001) AM_WRITE(toybox_mcu_com0_w			)	// To MCU ?
	AM_RANGE(0x2b0000, 0x2b0001) AM_WRITE(toybox_mcu_com1_w			)
	AM_RANGE(0x2c0000, 0x2c0001) AM_WRITE(toybox_mcu_com2_w			)
	AM_RANGE(0x2d0000, 0x2d0001) AM_WRITE(toybox_mcu_com3_w			)
	AM_RANGE(0x300000, 0x30ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0x310000, 0x327fff) AM_WRITE(MWA16_RAM												)	//
	AM_RANGE(0x400000, 0x401fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size			)	// Sprites
	AM_RANGE(0x500000, 0x500fff) AM_WRITE(kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1				)	// Layers 0
	AM_RANGE(0x501000, 0x501fff) AM_WRITE(kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0				)	//
	AM_RANGE(0x502000, 0x502fff) AM_WRITE(MWA16_RAM) AM_BASE(&kaneko16_vscroll_1					)	//
	AM_RANGE(0x503000, 0x503fff) AM_WRITE(MWA16_RAM) AM_BASE(&kaneko16_vscroll_0					)	//
	AM_RANGE(0x580000, 0x580fff) AM_WRITE(kaneko16_vram_3_w) AM_BASE(&kaneko16_vram_3				)	// Layers 1
	AM_RANGE(0x581000, 0x581fff) AM_WRITE(kaneko16_vram_2_w) AM_BASE(&kaneko16_vram_2				)	//
	AM_RANGE(0x582000, 0x582fff) AM_WRITE(MWA16_RAM) AM_BASE(&kaneko16_vscroll_3					)	//
	AM_RANGE(0x583000, 0x583fff) AM_WRITE(MWA16_RAM) AM_BASE(&kaneko16_vscroll_2					)	//
	AM_RANGE(0x600000, 0x60000f) AM_WRITE(kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs	)	// Layers 0 Regs
	AM_RANGE(0x680000, 0x68000f) AM_WRITE(kaneko16_layers_1_regs_w) AM_BASE(&kaneko16_layers_1_regs	)	// Layers 1 Regs
	AM_RANGE(0x700000, 0x70001f) AM_WRITE(kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs	)	// Sprites Regs
	AM_RANGE(0x800000, 0x800001) AM_WRITE(gtmr_oki_0_data_w			)	// Samples
	AM_RANGE(0x880000, 0x880001) AM_WRITE(gtmr_oki_1_data_w			)
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(watchdog_reset16_w		)	// Watchdog
	AM_RANGE(0xb80000, 0xb80001) AM_WRITE(kaneko16_coin_lockout_w	)	// Coin Lockout
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(kaneko16_display_enable	)	// might do more than that
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(gtmr_oki_0_bank_w			)	// Samples Bankswitching
	AM_RANGE(0xe80000, 0xe80001) AM_WRITE(gtmr_oki_1_bank_w			)
ADDRESS_MAP_END


/***************************************************************************
                            Great 1000 Miles Rally 2
***************************************************************************/


READ16_HANDLER( gtmr2_wheel_r )
{
	switch (readinputport(4) & 0x1800)
	{
		case 0x0000:	// 270' A. Wheel
			return	(readinputport(5));
			break;
		case 0x1000:	// 270' D. Wheel
			return	(readinputport(6) << 8);
			break;
		case 0x0800:	// 360' Wheel
			return	(readinputport(7) << 8);
			break;
		default:
			logerror("gtmr2_wheel_r : read at %06x with joystick\n", activecpu_get_pc());
			return	(~0);
			break;
	}
}

READ16_HANDLER( gtmr2_IN1_r )
{
	return	(readinputport(1) & (readinputport(8) | ~0x7100));
}

static ADDRESS_MAP_START( gtmr2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0ffffd) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x0ffffe, 0x0fffff) AM_READ(gtmr2_wheel_r				)	// Wheel Value
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM					)	// Work RAM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM					)	// Shared With MCU
	AM_RANGE(0x300000, 0x30ffff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0x310000, 0x327fff) AM_READ(MRA16_RAM					)	//
	AM_RANGE(0x400000, 0x401fff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0x500000, 0x503fff) AM_READ(MRA16_RAM					)	// Layers 0
	AM_RANGE(0x580000, 0x583fff) AM_READ(MRA16_RAM					)	// Layers 1
	AM_RANGE(0x600000, 0x60000f) AM_READ(MRA16_RAM					)	// Layers 0 Regs
	AM_RANGE(0x680000, 0x68000f) AM_READ(MRA16_RAM					)	// Layers 1 Regs
	AM_RANGE(0x700000, 0x70001f) AM_READ(kaneko16_sprites_regs_r	)	// Sprites Regs
	AM_RANGE(0x800000, 0x800001) AM_READ(OKIM6295_status_0_lsb_r	)	// Samples
	AM_RANGE(0x880000, 0x880001) AM_READ(OKIM6295_status_1_lsb_r	)
	AM_RANGE(0x900014, 0x900015) AM_READ(kaneko16_rnd_r			)	// Random Number ?
	AM_RANGE(0xa00000, 0xa00001) AM_READ(watchdog_reset16_r		)	// Watchdog
	AM_RANGE(0xb00000, 0xb00001) AM_READ(input_port_0_word_r		)	// Inputs
//  AM_RANGE(0xb00002, 0xb00003) AM_READ(input_port_1_word_r        )
	AM_RANGE(0xb00002, 0xb00003) AM_READ(gtmr2_IN1_r		)
	AM_RANGE(0xb00004, 0xb00005) AM_READ(input_port_2_word_r		)
	AM_RANGE(0xb00006, 0xb00007) AM_READ(input_port_3_word_r		)
	AM_RANGE(0xd00000, 0xd00001) AM_READ(toybox_mcu_status_r		)
ADDRESS_MAP_END


/***************************************************************************
                                Magical Crystal
***************************************************************************/

static ADDRESS_MAP_START( mgcrystl, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM		// ROM
	AM_RANGE(0x300000, 0x30ffff) AM_RAM		// Work RAM
	AM_RANGE(0x400000, 0x40001f) AM_READWRITE(kaneko16_YM2149_0_r, kaneko16_YM2149_0_w)	// Sound
	AM_RANGE(0x400200, 0x40021f) AM_READWRITE(kaneko16_YM2149_1_r, kaneko16_YM2149_1_w)
	AM_RANGE(0x400400, 0x400401) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x500000, 0x500fff) AM_READWRITE(MRA16_RAM, paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x600000, 0x600fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x601000, 0x601fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0x602000, 0x602fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0x603000, 0x603fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//
	AM_RANGE(0x680000, 0x680fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_3_w) AM_BASE(&kaneko16_vram_3)	// Layers 1
	AM_RANGE(0x681000, 0x681fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_2_w) AM_BASE(&kaneko16_vram_2)	//
	AM_RANGE(0x682000, 0x682fff) AM_RAM AM_BASE(&kaneko16_vscroll_3)									//
	AM_RANGE(0x683000, 0x683fff) AM_RAM AM_BASE(&kaneko16_vscroll_2)									//
	AM_RANGE(0x700000, 0x701fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)					// Sprites
	AM_RANGE(0x800000, 0x80001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers 0 Regs
	AM_RANGE(0x900000, 0x90001f) AM_READWRITE(MRA16_RAM, kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs)	// Sprites Regs
	AM_RANGE(0xb00000, 0xb0001f) AM_READWRITE(MRA16_RAM, kaneko16_layers_1_regs_w) AM_BASE(&kaneko16_layers_1_regs)	// Layers 1 Regs
	AM_RANGE(0xa00000, 0xa00001) AM_READ(watchdog_reset16_r)	// Watchdog
	AM_RANGE(0xc00000, 0xc00001) AM_READ(input_port_0_word_r)	// Inputs
	AM_RANGE(0xc00002, 0xc00003) AM_READ(input_port_1_word_r)	//
	AM_RANGE(0xc00004, 0xc00005) AM_READ(input_port_2_word_r)	//
	AM_RANGE(0xd00000, 0xd00001) AM_WRITE(kaneko16_eeprom_w)	// EEPROM
ADDRESS_MAP_END


/***************************************************************************
                                Sand Scorpion
***************************************************************************/

WRITE16_HANDLER( sandscrp_coin_counter_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,   data  & 0x0001);
		coin_counter_w(1,   data  & 0x0002);
	}
}

static UINT8 latch1_full;
static UINT8 latch2_full;

static READ16_HANDLER( sandscrp_latchstatus_word_r )
{
	return	(latch1_full ? 0x80 : 0) |
			(latch2_full ? 0x40 : 0) ;
}

static WRITE16_HANDLER( sandscrp_latchstatus_word_w )
{
	if (ACCESSING_LSB)
	{
		latch1_full = data & 0x80;
		latch2_full = data & 0x40;
	}
}

static READ16_HANDLER( sandscrp_soundlatch_word_r )
{
	latch2_full = 0;
	return soundlatch2_r(0);
}

static WRITE16_HANDLER( sandscrp_soundlatch_word_w )
{
	if (ACCESSING_LSB)
	{
		latch1_full = 1;
		soundlatch_w(0, data & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
		cpu_spinuntil_time(TIME_IN_USEC(100));	// Allow the other cpu to reply
	}
}

static ADDRESS_MAP_START( sandscrp, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM		// ROM
	AM_RANGE(0x700000, 0x70ffff) AM_RAM		// RAM
	AM_RANGE(0x200000, 0x20001f) AM_READWRITE(galpanib_calc_r,galpanib_calc_w)	// Protection
	AM_RANGE(0x300000, 0x30000f) AM_READWRITE(MRA16_RAM, kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers 0 Regs
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x401000, 0x401fff) AM_READWRITE(MRA16_RAM, kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)	//
	AM_RANGE(0x402000, 0x402fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)									//
	AM_RANGE(0x403000, 0x403fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)									//
	AM_RANGE(0x500000, 0x501fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)				// Sprites
	AM_RANGE(0x600000, 0x600fff) AM_READWRITE(MRA16_RAM, paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(sandscrp_coin_counter_w)	// Coin Counters (Lockout unused)
	AM_RANGE(0xb00000, 0xb00001) AM_READ(input_port_0_word_r)	// Inputs
	AM_RANGE(0xb00002, 0xb00003) AM_READ(input_port_1_word_r)	//
	AM_RANGE(0xb00004, 0xb00005) AM_READ(input_port_2_word_r)	//
	AM_RANGE(0xb00006, 0xb00007) AM_READ(input_port_3_word_r)	//
	AM_RANGE(0xec0000, 0xec0001) AM_READ(watchdog_reset16_r)	//
	AM_RANGE(0x800000, 0x800001) AM_READ(sandscrp_irq_cause_r)	// IRQ Cause
	AM_RANGE(0xe00000, 0xe00001) AM_READWRITE(sandscrp_soundlatch_word_r, sandscrp_soundlatch_word_w)	// From/To Sound CPU
	AM_RANGE(0xe40000, 0xe40001) AM_READWRITE(sandscrp_latchstatus_word_r, sandscrp_latchstatus_word_w)	//
	AM_RANGE(0x100000, 0x100001) AM_WRITE(sandscrp_irq_cause_w)	// IRQ Ack
ADDRESS_MAP_END


/***************************************************************************
                                Shogun Warriors
***************************************************************************/

/* Untested */
WRITE16_HANDLER( shogwarr_oki_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(1, 0x40000 * ((data >> 0) & 0xf) );
		OKIM6295_set_bank_base(0, 0x40000 * ((data >> 4) & 0xf) );
	}
}

static ADDRESS_MAP_START( shogwarr, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM		// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM AM_BASE(&kaneko16_mainram)		// Work RAM
	AM_RANGE(0x200000, 0x20ffff) AM_READWRITE(MRA16_RAM,calc3_mcu_ram_w) AM_BASE(&kaneko16_mcu_ram)	// Shared With MCU
	AM_RANGE(0x280000, 0x280001) AM_WRITE(calc3_mcu_com0_w)
	AM_RANGE(0x290000, 0x290001) AM_WRITE(calc3_mcu_com1_w)
	AM_RANGE(0x2b0000, 0x2b0001) AM_WRITE(calc3_mcu_com2_w)
	AM_RANGE(0x2d0000, 0x2d0001) AM_WRITE(calc3_mcu_com3_w)
	AM_RANGE(0x380000, 0x380fff) AM_READWRITE(MRA16_RAM,paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x400000, 0x400001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x480000, 0x480001) AM_READWRITE(OKIM6295_status_1_lsb_r, OKIM6295_data_1_lsb_w)
	AM_RANGE(0x580000, 0x581fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)					// Sprites
	AM_RANGE(0x600000, 0x600fff) AM_READWRITE(MRA16_RAM,kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)	// Layers 0
	AM_RANGE(0x601000, 0x601fff) AM_READWRITE(MRA16_RAM,kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)
	AM_RANGE(0x602000, 0x602fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)
	AM_RANGE(0x603000, 0x603fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)
	AM_RANGE(0x800000, 0x80000f) AM_READWRITE(MRA16_RAM,kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)	// Layers 0 Regs
	AM_RANGE(0x900000, 0x90001f) AM_READWRITE(MRA16_RAM,kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs)	// Sprites Regs
	AM_RANGE(0xa00000, 0xa0007f) AM_READWRITE(bloodwar_calc_r, bloodwar_calc_w)
	AM_RANGE(0xa80000, 0xa80001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)	// Watchdog
	AM_RANGE(0xb80000, 0xb80001) AM_READ(input_port_0_word_r)	// Inputs
	AM_RANGE(0xb80002, 0xb80003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xb80004, 0xb80005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xb80006, 0xb80007) AM_READ(input_port_3_word_r)
	AM_RANGE(0xd00000, 0xd00001) AM_NOP							// ? (bit 0)
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(shogwarr_oki_bank_w)	// Samples Bankswitching
ADDRESS_MAP_END


/***************************************************************************


                            Memory Maps - Sound CPU


***************************************************************************/

/***************************************************************************
                                    Blaze On
***************************************************************************/

#if 0
static WRITE8_HANDLER( blazeon_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bank = data & 7;
	memory_set_bankptr(15, &RAM[bank * 0x10000 + 0x1000]);
}
#endif

static ADDRESS_MAP_START( blazeon_soundmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM		// ROM
	AM_RANGE(0xc000, 0xdfff) AM_RAM		// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( blazeon_soundport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x02) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0x06, 0x06) AM_READ(soundlatch_r)
ADDRESS_MAP_END


/***************************************************************************
                                Sand Scorpion
***************************************************************************/

WRITE8_HANDLER( sandscrp_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bank = data & 0x07;

	if ( bank != data )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);

	if (bank < 3)	RAM = &RAM[0x4000 * bank];
	else			RAM = &RAM[0x4000 * (bank-3) + 0x10000];

	memory_set_bankptr(1, RAM);
}

static READ8_HANDLER( sandscrp_latchstatus_r )
{
	return	(latch2_full ? 0x80 : 0) |	// swapped!?
			(latch1_full ? 0x40 : 0) ;
}

static READ8_HANDLER( sandscrp_soundlatch_r )
{
	latch1_full = 0;
	return soundlatch_r(0);
}

static WRITE8_HANDLER( sandscrp_soundlatch_w )
{
	latch2_full = 1;
	soundlatch2_w(0,data);
}

static ADDRESS_MAP_START( sandscrp_soundmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM		// ROM
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_ROM)	// Banked ROM
	AM_RANGE(0xc000, 0xdfff) AM_RAM		// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sandscrp_soundport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(sandscrp_bankswitch_w)	// ROM Bank
	AM_RANGE(0x02, 0x02) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)	// YM2203
	AM_RANGE(0x03, 0x03) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)		// PORTA/B read
	AM_RANGE(0x04, 0x04) AM_WRITE(OKIM6295_data_0_w)		// OKIM6295
	AM_RANGE(0x06, 0x06) AM_WRITE(sandscrp_soundlatch_w)	//
	AM_RANGE(0x07, 0x07) AM_READ(sandscrp_soundlatch_r)		//
	AM_RANGE(0x08, 0x08) AM_READ(sandscrp_latchstatus_r)	//
ADDRESS_MAP_END


/***************************************************************************


                                Input Ports


***************************************************************************/

/***************************************************************************
                            Bakuretsu Breaker
***************************************************************************/

INPUT_PORTS_START( bakubrkr )
	PORT_START	// IN0 - Player 1 + DSW - e00000.w
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - e00002.b
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - e00004.b
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)	// pause
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - Seems unused ! - e00006.b
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
                            The Berlin Wall (set 1)
***************************************************************************/

INPUT_PORTS_START( berlwall )
	PORT_START	// IN0 - Player 1 - 680000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - 680002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - 680004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - ? - 680006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW 1 - $200018.b <- ! $80001d.b
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Reserved" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	// IN5 - DSW 2 - $200019.b <- $80001f.b
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy )    )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal )  )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard )    )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )	// 1p lives at 202982.b
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Country"   )
	PORT_DIPSETTING(    0x30, "England" )
	PORT_DIPSETTING(    0x20, "Italy"   )
	PORT_DIPSETTING(    0x10, "Germany" )
	PORT_DIPSETTING(    0x00, "Freeze Screen" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


/***************************************************************************
                            The Berlin Wall (set 2)
***************************************************************************/

//  Same as berlwall, but for a different lives setting

INPUT_PORTS_START( berlwalt )
	PORT_START	// IN0 - Player 1 - 680000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - 680002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - 680004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - ? - 680006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW 1 - $80001d.b
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Reserved" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	// IN5 - DSW 2 - $80001f.b
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy )    )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal )  )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard )    )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPNAME( 0x30, 0x30, "Country"   )
	PORT_DIPSETTING(    0x30, "England" )
	PORT_DIPSETTING(    0x20, "Italy"   )
	PORT_DIPSETTING(    0x10, "Germany" )
	PORT_DIPSETTING(    0x00, "Freeze Screen" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


/***************************************************************************
                                    Blaze On
***************************************************************************/

INPUT_PORTS_START( blazeon )
	PORT_START	// IN0 - Player 1 + DSW - c00000.w
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy )    )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal )  )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard )    )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0008, "4" )
	PORT_DIPSETTING(      0x0004, "5" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START1                       )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)

	PORT_START	// IN1 - Player 2 - c00002.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0005, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
//  PORT_DIPSETTING(      0x0001, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )

	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0050, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_3C ) )
//  PORT_DIPSETTING(      0x0010, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START2                       )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)

	PORT_START	// IN2 - ? - c00004.w
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )	// unused?

	PORT_START	// IN3 - Other Buttons - c00006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_TILT  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END

/***************************************************************************
                                Blood Warrior
***************************************************************************/

INPUT_PORTS_START( bloodwar )
	PORT_START	// IN0 - Player 1 - b00000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP		) PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN		) PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT		) PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1			) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2			) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3			) PORT_PLAYER(1)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON4			) PORT_PLAYER(1)

	PORT_START	// IN1 - Player 2 - b00002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP		) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN		) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT		) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1			) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2			) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3			) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON4			) PORT_PLAYER(2)

	PORT_START	// IN2 - Coins - b00004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_SERVICE2	)	// tested

	PORT_START	// IN3 - ? - b00006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )	// tested
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )	// tested
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW from the MCU - $10497e.b <- $208000.b
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0200, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x3800, "1 Easy" )
	PORT_DIPSETTING(      0x3000, "2" )
	PORT_DIPSETTING(      0x2800, "3" )
	PORT_DIPSETTING(      0x2000, "4" )
	PORT_DIPSETTING(      0x1800, "5" )
	PORT_DIPSETTING(      0x1000, "6" )
	PORT_DIPSETTING(      0x0800, "7" )
	PORT_DIPSETTING(      0x0000, "8 Hard" )
	PORT_DIPNAME( 0x4000, 0x4000, "Join During Game" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************
                                Bonk's Adventure
***************************************************************************/

INPUT_PORTS_START( bonkadv )
	PORT_START	// IN0 - Player 1 - b00000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP		) PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN		) PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT		) PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1			) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2			) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3			) PORT_PLAYER(1)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON4			) PORT_PLAYER(1)

	PORT_START	// IN1 - Player 2 - b00002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP		) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN		) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT		) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1			) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2			) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3			) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON4			) PORT_PLAYER(2)

	PORT_START	// IN2 - Coins - b00004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_SERVICE2	)

	PORT_START	// IN3 - ? - b00006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // tested
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // tested
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW from the MCU - $10019e.b <- $200200.b
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Ticket dispenser" ) /* "Reserved" for Japan/Europe */
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x0000, "Title Level Display" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Reserved" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Reserved" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END


/***************************************************************************
                            Great 1000 Miles Rally
***************************************************************************/

INPUT_PORTS_START( gtmr )
	PORT_START	// IN0 - Player 1 - b00000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) // swapped for consistency:
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) // button1 is usually accel.
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - b00002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) // swapped for consistency:
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) // button1 is usually accel.
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - b00004.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - Seems unused ! - b00006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW from the MCU - 101265.b <- 206000.b
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Cabinet )  )
	PORT_DIPSETTING(      0x0400, DEF_STR( Upright )  )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Controls )    )
	PORT_DIPSETTING(      0x1800, "1 Joystick"  )
	PORT_DIPSETTING(      0x0800, "2 Joysticks" )
	PORT_DIPSETTING(      0x1000, "Wheel (360)" )
	PORT_DIPSETTING(      0x0000, "Wheel (270)" )
	PORT_DIPNAME( 0x2000, 0x2000, "Use Brake"    )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On )  )
	PORT_DIPNAME( 0xc000, 0xc000, "National Anthem & Flag" )
	PORT_DIPSETTING(      0xc000, "Use Memory"  )
	PORT_DIPSETTING(      0x8000, "Anthem Only" )
	PORT_DIPSETTING(      0x4000, "Flag Only"   )
	PORT_DIPSETTING(      0x0000, DEF_STR( None )        )

	PORT_START	// IN5 - Wheel - 100015.b <- ffffe.b
	PORT_BIT ( 0x00ff, 0x0080, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(1)
INPUT_PORTS_END


/***************************************************************************
                            Great 1000 Miles Rally 2
***************************************************************************/

INPUT_PORTS_START( gtmr2 )
	PORT_START	// IN0 - Player 1 - 100004.w <- b00000.w (cpl)
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) // swapped for consistency:
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) // button1 is usually accel.
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - 10000c.w <- b00002.w (cpl) - for "test mode" only
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) // swapped for consistency:
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) // button1 is usually accel.
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Coins - 100014.w <- b00004.w (cpl)
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_START2	)	// only in "test mode"
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_TILT	)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START	// IN3 - 100017.w <- b00006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("IN 3-6")	// Code at 0x002236 - Centers 270D wheel ?
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW from the MCU - 1016f7.b <- 206000.b
	PORT_DIPNAME( 0x0700, 0x0700, "Communication" )
	PORT_DIPSETTING(      0x0700, DEF_STR( None ) )
	PORT_DIPSETTING(      0x0600, "Machine 1" )
	PORT_DIPSETTING(      0x0500, "Machine 2" )
	PORT_DIPSETTING(      0x0400, "Machine 3" )
	PORT_DIPSETTING(      0x0300, "Machine 4" )
	/* 0x0000 to 0x0200 : "Machine 4"
    PORT_DIPSETTING(      0x0200, "Machine 4 (0x0200)" )
    PORT_DIPSETTING(      0x0100, "Machine 4 (0x0100)" )
    PORT_DIPSETTING(      0x0000, "Machine 4 (0x0000)" )
    */
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Controls ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( Joystick ) )
	PORT_DIPSETTING(      0x0800, "Wheel (360)" )			// Not working correctly in race
	PORT_DIPSETTING(      0x1000, "Wheel (270D)" )			// Not working correctly !
	PORT_DIPSETTING(      0x0000, "Wheel (270A)" )			// Not working correctly in race
	PORT_DIPNAME( 0x2000, 0x2000, "Pedal Function" )
	PORT_DIPSETTING(      0x2000, "Microswitch" )
//  PORT_DIPSETTING(      0x0000, "Potentiometer" )         // Not implemented yet
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START	// IN5 - Wheel (270A) - 100019.b <- fffff.b
	PORT_BIT ( 0x00ff, 0x0080, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(1)

	PORT_START	// IN6 - Wheel (270D) - 100019.b <- ffffe.b
	PORT_BIT ( 0x00ff, 0x0080, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(1)

	PORT_START	// IN7 - Wheel (360) - 100019.b <- ffffe.b
	PORT_BIT( 0x00ff, 0x0080, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(1) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT)

	PORT_START	// Fake IN1 - To be pressed during boot sequence - Code at 0x000c9e
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("IN 1-0") PORT_CODE(KEYCODE_H)	// "sound test"
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("IN 1-4") PORT_CODE(KEYCODE_J)	// "view tiles"
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("IN 1-5") PORT_CODE(KEYCODE_K)	// "view memory"
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("IN 1-6") PORT_CODE(KEYCODE_L)	// "view sprites ?"
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
                                Magical Crystal
***************************************************************************/

INPUT_PORTS_START( mgcrystl )
	PORT_START	// IN0 - Player 1 + DSW - c00000.w
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	// TESTED!
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2 - c00002.b
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
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
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Other Buttons - c00004.b
	PORT_BIT ( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT ( 0x0100, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT ( 0x0200, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT ( 0x2000, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT ( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
                                Sand Scorpion
***************************************************************************/

INPUT_PORTS_START( sandscrp )
	PORT_START	// IN0 - $b00000.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $b00002.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - $b00004.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $b00006.w
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW 1 read by the Z80 through the sound chip
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bombs" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Easy )    )
	PORT_DIPSETTING(    0x20, DEF_STR( Normal )  )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard )    )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "100K, 300K" )
	PORT_DIPSETTING(    0xc0, "200K, 500K" )
	PORT_DIPSETTING(    0x40, "500K, 1000K" )
	PORT_DIPSETTING(    0x00, "1000K, 3000K" )

	PORT_START	// IN5 - DSW 2 read by the Z80 through the sound chip
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


/***************************************************************************
                                Shogun Warriors
***************************************************************************/

INPUT_PORTS_START( shogwarr )
	PORT_START	// IN0 - - b80000.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN1 )
	
	PORT_START	// IN1 - - b80002.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN2 )
	
	PORT_START	// IN2 - Coins - b80004.w
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)
	
	PORT_START	// IN3 - ? - b80006.w
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN4 - DSW from the MCU - 102e15.b <- 200059.b
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x38, "1 Easy" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4 Normal" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x00, "8 Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Can Join During Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )	//  2 credits       winner vs computer
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )	//  1 credit        game over
	PORT_DIPNAME( 0x80, 0x80, "Continue Coin" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( brapboys )
	PORT_START		/* b80000.w */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START		/* b80002.w */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* b80004.w */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* ? - b80006.w */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE(0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, "Switch Test" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, "Coin Slots" )
	PORT_DIPSETTING(    0x10, "Separate Coins" )
	PORT_DIPSETTING(    0x00, "Shared Coins" )
	PORT_DIPNAME( 0x20, 0x20, "Players" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard) )
INPUT_PORTS_END	

/***************************************************************************


                                Graphics Layouts


***************************************************************************/


/*
    16x16x4 made of 4 8x8x4 blocks arrenged like:           01
    (nibbles are swapped for tiles, not for sprites)        23
*/
static const gfx_layout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(8*8*4*0,4),   STEP8(8*8*4*1,4)   },
	{ STEP8(8*8*4*0,8*4), STEP8(8*8*4*2,8*4) },
	16*16*4
};

/*
    16x16x8 made of 4 8x8x8 blocks arrenged like:   01
                                                    23
*/
static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8),   STEP8(8*8*8*1,8)   },
	{ STEP8(0,8*8), STEP8(8*8*8*2,8*8) },
	16*16*8
};

static const gfx_decode kaneko16_gfx_1x4bit_1x4bit[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0,			0x40 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4, 0x40 * 16,	0x40 }, // [1] Layers
	{ -1 }
};
static const gfx_decode kaneko16_gfx_1x4bit_2x4bit[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0,			0x40 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4, 0x40 * 16,	0x40 }, // [1] Layers
	{ REGION_GFX3, 0, &layout_16x16x4, 0x40 * 16,	0x40 }, // [2] Layers
	{ -1 }
};
static const gfx_decode kaneko16_gfx_1x8bit_2x4bit[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8,	0x40 * 256,	0x40 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4,	0,			0x40 }, // [1] Layers
	{ REGION_GFX3, 0, &layout_16x16x4,	0,			0x40 }, // [2] Layers
	{ -1 }
};

/* 16x16x4 tiles (made of four 8x8 tiles) */
static const gfx_layout layout_16x16x4_2 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP4(8*8*4*0 + 3*4, -4), STEP4(8*8*4*0 + 7*4, -4),
	  STEP4(8*8*4*1 + 3*4, -4), STEP4(8*8*4*1 + 7*4, -4) },
	{ STEP8(8*8*4*0, 8*4),     STEP8(8*8*4*2, 8*4) },
	16*16*4
};
static const gfx_decode sandscrp_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4,   0x000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4_2, 0x400, 0x40 }, // [1] Layers
	{ -1 }
};


/***************************************************************************


                                Machine Drivers


***************************************************************************/

#define KANEKO16_INTERRUPTS_NUM	3
INTERRUPT_GEN( kaneko16_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 2:  cpunum_set_input_line(0, 3, HOLD_LINE); break;
		case 1:  cpunum_set_input_line(0, 4, HOLD_LINE); break;
		case 0:  cpunum_set_input_line(0, 5, HOLD_LINE); break;
	}
}

static struct AY8910interface ay8910_intf_dsw =
{
	input_port_4_r,	/* input A: DSW 1 */
	input_port_5_r	/* input B: DSW 2 */
};

static struct AY8910interface ay8910_intf_eeprom =
{
	kaneko16_eeprom_r,		/* inputs  A:  0,EEPROM bit read */
	0,						/* inputs  B */
	0,						/* outputs A */
	kaneko16_eeprom_reset_w	/* outputs B:  0,EEPROM reset */
};


/***************************************************************************
                                The Berlin Wall
***************************************************************************/

/*
    Berlwall interrupts:

    1-3]    e8c:
    4]      e54:
    5]      de4:
    6-7]    rte
*/

static MACHINE_DRIVER_START( berlwall )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* MC68000P12 */
	MDRV_CPU_PROGRAM_MAP(berlwall,0)
	MDRV_CPU_VBLANK_INT(kaneko16_interrupt,KANEKO16_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(berlwall)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)	// mangled sprites otherwise
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 240-1)
	MDRV_GFXDECODE(kaneko16_gfx_1x4bit_1x4bit)
	MDRV_PALETTE_LENGTH(2048 + 32768)	/* 32768 static colors for the bg */

	MDRV_PALETTE_INIT(berlwall)
	MDRV_VIDEO_START(berlwall)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(AY8910, 1000000)
	MDRV_SOUND_CONFIG(ay8910_intf_dsw)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(AY8910, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 12000000/6/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                            Bakuretsu Breaker
***************************************************************************/

static MACHINE_DRIVER_START( bakubrkr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* TMP68HC000-12 */
	MDRV_CPU_PROGRAM_MAP(bakubrkr,0)
	MDRV_CPU_VBLANK_INT(kaneko16_interrupt,KANEKO16_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(59)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(bakubrkr)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)	// mangled sprites otherwise
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 240-1)
	MDRV_GFXDECODE(kaneko16_gfx_1x4bit_2x4bit)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(kaneko16_2xVIEW2)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_intf_eeprom)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 12000000/6/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                    Blaze On
***************************************************************************/

/*
    Blaze On:
        1]      busy loop
        2]      does nothing
        3]      rte
        4]      drives the game
        5]      == 2
        6-7]    busy loop
*/

static MACHINE_DRIVER_START( blazeon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000)	/* TMP68HC000-12 */
	MDRV_CPU_PROGRAM_MAP(blazeon,0)
	MDRV_CPU_VBLANK_INT(kaneko16_interrupt,KANEKO16_INTERRUPTS_NUM)

	MDRV_CPU_ADD(Z80,4000000)	/* D780C-2 */
	MDRV_CPU_PROGRAM_MAP(blazeon_soundmem,0)
	MDRV_CPU_IO_MAP(blazeon_soundport,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(blazeon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1 -8)
	MDRV_GFXDECODE(kaneko16_gfx_1x4bit_1x4bit)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(kaneko16_1xVIEW2)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/***************************************************************************
                            Great 1000 Miles Rally
***************************************************************************/

/*
    gtmr interrupts:

    3] 476:         time, input ports, scroll registers
    4] 466->258e:   set sprite ram
    5] 438:         set sprite colors

    VIDEO_UPDATE_AFTER_VBLANK fixes the mangled/wrong colored sprites
*/

static MACHINE_DRIVER_START( gtmr )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("gtmr", M68000, 16000000)	/* ? Most likely a 68000-HC16 */
	MDRV_CPU_PROGRAM_MAP(gtmr_readmem,gtmr_writemem)
	MDRV_CPU_VBLANK_INT(kaneko16_interrupt,KANEKO16_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(gtmr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(kaneko16_gfx_1x8bit_2x4bit)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(kaneko16_2xVIEW2)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END

/***************************************************************************
                                Blood Warrior
***************************************************************************/

static MACHINE_DRIVER_START( bloodwar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gtmr)
	MDRV_CPU_MODIFY("gtmr")
	MDRV_CPU_PROGRAM_MAP(bloodwar,0)

	MDRV_MACHINE_RESET( bloodwar )
MACHINE_DRIVER_END

/***************************************************************************
                            Great 1000 Miles Rally 2
***************************************************************************/

static MACHINE_DRIVER_START( gtmr2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gtmr)
	MDRV_CPU_MODIFY("gtmr")
	MDRV_CPU_PROGRAM_MAP(gtmr2_readmem,gtmr_writemem)
MACHINE_DRIVER_END

/***************************************************************************
                            Bonk's Adventure
***************************************************************************/

/*
    Even though 3 interrupts are triggered, I set an int_num of 4. (notice '+1')
    I agree that it is kind of a misuse of the function, but I haven't found
    clues in code on how interrupts are scheduled...
    IT5 is the main int, and needs more time to execute than IT 3 and 4.
    Between other things, each of these 2 int are responsible of translating
    a part of sprite buffer from work ram to sprite ram.
    So now test mode is fully working and visible.
    SebV
*/
static MACHINE_DRIVER_START( bonkadv )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gtmr)
	MDRV_CPU_MODIFY("gtmr")
	MDRV_CPU_PROGRAM_MAP(bonkadv,0)
	MDRV_CPU_VBLANK_INT(kaneko16_interrupt,KANEKO16_INTERRUPTS_NUM + 1 ) // comment above

	MDRV_MACHINE_RESET( bonkadv )
MACHINE_DRIVER_END

/***************************************************************************
                                Magical Crystal
***************************************************************************/

static MACHINE_DRIVER_START( mgcrystl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(mgcrystl,0)
	MDRV_CPU_VBLANK_INT(kaneko16_interrupt,KANEKO16_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(mgcrystl)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(kaneko16_gfx_1x4bit_2x4bit)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(kaneko16_2xVIEW2)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_intf_eeprom)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 12000000/4/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Sand Scorpion
***************************************************************************/

/* YM3014B + YM2203C */

static void irq_handler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_intf_sandscrp =
{
	input_port_4_r,	/* Port A Read - DSW 1 */
	input_port_5_r,	/* Port B Read - DSW 2 */
	0,	/* Port A Write */
	0,	/* Port B Write */
	irq_handler	/* IRQ handler */
};


static MACHINE_DRIVER_START( sandscrp )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000)	/* TMP68HC000N-12 */
	MDRV_CPU_PROGRAM_MAP(sandscrp,0)
	MDRV_CPU_VBLANK_INT(sandscrp_interrupt,1)

	MDRV_CPU_ADD(Z80,4000000)	/* Z8400AB1, Reads the DSWs: it can't be disabled */
	MDRV_CPU_PROGRAM_MAP(sandscrp_soundmem,0)
	MDRV_CPU_IO_MAP(sandscrp_soundport,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	// eof callback

	MDRV_MACHINE_RESET(sandscrp)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(sandscrp_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(sandscrp_1xVIEW2)
	MDRV_VIDEO_EOF(sandscrp)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 12000000/6/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_intf_sandscrp)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
MACHINE_DRIVER_END


/***************************************************************************
                                Shogun Warriors
***************************************************************************/

/*
    shogwarr interrupts:

    2] 100: rte
    3] 102:
    4] 136:
        movem.l D0-D7/A0-A6, -(A7)
        movea.l $207808.l, A0   ; from mcu?
        jmp     ($4,A0)

    other: busy loop
*/
#define SHOGWARR_INTERRUPTS_NUM	3
INTERRUPT_GEN( shogwarr_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 2:  cpunum_set_input_line(0, 2, HOLD_LINE); break;
		case 1:  cpunum_set_input_line(0, 3, HOLD_LINE); break;
		case 0:  cpunum_set_input_line(0, 4, HOLD_LINE); calc3_mcu_run();break;
	}
}

// the above eeprom looks corrupt, some of the text is wrong, the game never writes this text tho.. maybe it should be as below
// leaving both here incase they relate to which tables get 'locked out' by the MCU somehow
static UINT8 shogwarr_default_eeprom[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x4B, 0x41, 0x4E, 0x45, 0x4B, 0x4F, 0x2F, 0x41, 0x54, 0x4F, 0x50, 0x20, 0x31, 0x39, 0x39, 0x32,
	0x46, 0x55, 0x4A, 0x49, 0x59, 0x41, 0x4D, 0x41, 0x20, 0x42, 0x55, 0x53, 0x54, 0x45, 0x52, 0x20,
	0x20, 0x53, 0x48, 0x4F, 0x47, 0x55, 0x4E, 0x20, 0x57, 0x41, 0x52, 0x52, 0x49, 0x4F, 0x52, 0x53,
	0x63, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x4B, 0x41, 0x4E, 0x45, 0x4B, 0x4F,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF
};

static UINT8 brapboys_default_eeprom[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x20, 0x30, 0x00, 0x03, 0x68, 0x18, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x00, 0x01, 0x00, 0x04, 0x00, 0x08, 0x4B, 0x41, 0x4E, 0x45, 0x4B, 0x4F, 0x20, 0x20,
	0x42, 0x65, 0x20, 0x52, 0x61, 0x70, 0x20, 0x42, 0x6F, 0x79, 0x73, 0x00, 0x30, 0x30, 0x30, 0x2E,
	0x30, 0x38, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x35, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static NVRAM_HANDLER( shogwarr )
{
	int isbrap = ( !strcmp(Machine->gamedrv->name,"brapboysj") || !strcmp(Machine->gamedrv->name,"brapboys"));

	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file) EEPROM_load(file);
		else
		{
			if (isbrap)
				EEPROM_set_data(brapboys_default_eeprom,128);
			else
				EEPROM_set_data(shogwarr_default_eeprom,128);
		}
	}
}

static MACHINE_DRIVER_START( shogwarr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(shogwarr,0)
	MDRV_CPU_VBLANK_INT(shogwarr_interrupt,SHOGWARR_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(59.1854)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(0))

	MDRV_MACHINE_RESET(shogwarr)
	MDRV_NVRAM_HANDLER(shogwarr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(40, 296-1, 16, 240-1)
	MDRV_GFXDECODE(kaneko16_gfx_1x4bit_1x4bit)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(kaneko16_1xVIEW2)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( brapboys )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(shogwarr,0)
	MDRV_CPU_VBLANK_INT(shogwarr_interrupt,SHOGWARR_INTERRUPTS_NUM)

	MDRV_FRAMES_PER_SECOND(59.1854)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(0))

	MDRV_MACHINE_RESET(brapboys)
	MDRV_NVRAM_HANDLER(shogwarr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(40, 296-1, 16, 240-1)
	MDRV_GFXDECODE(kaneko16_gfx_1x4bit_1x4bit)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(kaneko16_1xVIEW2)
	MDRV_VIDEO_UPDATE(kaneko16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

/***************************************************************************


                                ROMs Loading


***************************************************************************/

/*
 Sprites and tiles are stored in the ROMs using the same layout. But tiles
 have the even and odd pixels swapped. So we use this function to untangle
 them and have one single gfxlayout for both tiles and sprites.
*/
void kaneko16_unscramble_tiles(int region)
{
	unsigned char *RAM	=	memory_region(region);
	int size			=	memory_region_length(region);
	int i;

	if (RAM == NULL)	return;

	for (i = 0; i < size; i ++)
	{
		RAM[i] = ((RAM[i] & 0xF0)>>4) + ((RAM[i] & 0x0F)<<4);
	}
}

void kaneko16_expand_sample_banks(int region)
{
	/* The sample data for the first OKI has an address translator/
       banking register in it that munges the addresses as follows:

         Offsets 00000-2FFFF always come from ROM 00000-2FFFF
         Offsets 30000-3FFFF come from ROM (10000*bank) + 00000-0FFFF

       Because we can't do this dynamically, we pre-generate all 16
       possible combinations of these and swap between them.
    */
	int bank;

	if (memory_region_length(region) < 0x40000 * 16)
		fatalerror("gtmr SOUND1 region too small");

	/* bank 0 maps to itself, so we just leave it alone */
	for (bank = 15; bank > 0; bank--)
	{
		UINT8 *src0 = memory_region(region);
		UINT8 *srcn = src0 + 0x10000 * (bank < 3 ? 3 : bank);
		UINT8 *dst = src0 + 0x40000 * bank;

		memcpy(dst + 0x30000, srcn + 0x00000, 0x10000);
		memcpy(dst + 0x00000, src0 + 0x00000, 0x30000);
	}
}

DRIVER_INIT( kaneko16 )
{
	kaneko16_unscramble_tiles(REGION_GFX2);
	kaneko16_unscramble_tiles(REGION_GFX3);
}

DRIVER_INIT( berlwall )
{
	kaneko16_unscramble_tiles(REGION_GFX2);
}

DRIVER_INIT( samplebank )
{
	kaneko16_unscramble_tiles(REGION_GFX2);
	kaneko16_unscramble_tiles(REGION_GFX3);
	kaneko16_expand_sample_banks(REGION_SOUND1);
	decrypt_toybox_rom();
}


static void expand_shogwarr_samples(void)
{
	/*
        Expand the OKI sample data

        OKI 1:
        Address space 0x00000-0x2ffff is fixed
        Address space 0x30000-0x3ffff is banked (13 banks)
    */

	int bank;
	UINT8 *src = memory_region(REGION_USER1);
	UINT8 *dst1 = memory_region(REGION_SOUND1);

	/* OKI 1 */
	for (bank = 0; bank < 13; ++bank)
	{
		UINT8 *dst;
		UINT8 *srcn;

		dst = dst1 + 0x40000 * bank;
		srcn = src + 0x30000 + (0x10000 * bank);
		memcpy(dst, src, 0x30000);
		memcpy(dst + 0x30000, srcn, 0x10000);
	}
}

static void calc3_init(void)
{
	kaneko16_unscramble_tiles(REGION_GFX2);
	kaneko16_unscramble_tiles(REGION_GFX3);
	expand_shogwarr_samples();
    //  MCU is a 78K series III type CPU
}

static void expand_brapboys_music(void)
{
	/*
        Expand the OKI sample data

        OKI 2:
        Address space 0x00000-0x1ffff is fixed
        Address space 0x20000-0x3ffff is banked (15 banks)
    */

	int bank;
	UINT8 *src = memory_region(REGION_USER1);
	UINT8 *dst2 = memory_region(REGION_SOUND2);

	/* OKI 2 */
	for (bank = 0; bank < 15; ++bank)
	{
		UINT8 *dst;
		UINT8 *srcn;
		dst = dst2 + 0x40000 * bank;
		srcn = src + 0x120000 + (0x20000 * bank);
		memcpy(dst, src + 0x100000, 0x20000);
		memcpy(dst + 0x20000, srcn, 0x20000);
	}
}


DRIVER_INIT( brapboys )
{
	// sample banking is different on brap boys for the music, why? GALs / PALs ?
	expand_brapboys_music();
	calc3_init();
}

DRIVER_INIT( shogwarr )
{
	calc3_init();
}

/***************************************************************************

                                Bakuretsu Breaker

    USES TOSHIBA 68000 CPU W/TWO YM2149 & OKI M6295 FOR SOUND

    LOCATION    TYPE
    ------------------
    U38         27C040
    U37         "
    U36         27C020
    U19         "
    U18         "

Bakuretsu Breaker
Kaneko, 1992

PCB Layout
----------

ZOOFC-02
|------------------------------------------|
| PAL    TS020.U33                PAL      |
| 6264                            6264     |
| 6264    VIEW2-CHIP              6264     |
| 4464 4464           VIEW2-CHIP  TS010.U4 |
| 4464 4464                                |
| 4464 4464                       699206P  |
|                         5116    (QFP44)  |
|                                          |
|         TS002J.U36 MUX2-CHIP    699205P  |
|VU-002   TS001J.U37      5116    (QFP44) J|
|(QFP160) TS000J.U38                      A|
|                   62256 TS100J.U18      M|
|        6116 6116  62256 TS101J.U19      M|
|        PAL                     PAL      A|
|16MHz   PAL  PAL  TMP68HC000N-12          |
|12MHz        PAL                IU-001    |
|   VU-001                      (QFP44)    |
|   (QFP48)  YM2149      PAL               |
|93C46       YM2149  TS030.U5  M6295       |
|    DSW1(4)                               |
|------------------------------------------|

Notes:
       68000 clock: 12.000MHz
      YM2149 clock: 2.000MHz
       M6295 clock: 2.000MHz, sample rate = clock /132
             VSync: 59Hz
             HSync: 15.68kHz


***************************************************************************/

ROM_START( explbrkr )
 	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "ts100e.u18", 0x000000, 0x040000, CRC(cc84a985) SHA1(1732a607cc1f894dd45cfc915dfe0407335f0073) )
	ROM_LOAD16_BYTE( "ts101e.u19", 0x000001, 0x040000, CRC(88f4afb7) SHA1(08b8efd6bd935bc1b8cf9753d58b38ccf9a70b4d) )

	/* these actually match the other set but have different names on the board..*/
	ROM_REGION( 0x240000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ts001e.u37",  0x000000, 0x080000, CRC(70b66e7e) SHA1(307ba27b623f67ee4b4023179870c270bac8ea22) )
	ROM_RELOAD(       0x100000, 0x080000             )
	ROM_LOAD( "ts000e.u38",  0x080000, 0x080000, CRC(a7a94143) SHA1(d811a7597402c161850ddf98cdb00661ea506c7d) )
	ROM_RELOAD(       0x180000, 0x080000             )
	ROM_LOAD( "ts002e.u36",  0x200000, 0x040000, CRC(611271e6) SHA1(811c21822b074fbb4bb809fed29d48bbd51d57a0) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles */
	ROM_LOAD( "ts010.u4",  0x000000, 0x100000, CRC(df935324) SHA1(73b7aff8800a4e88a47ad426190b73dabdfbf142) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles */
	ROM_LOAD( "ts020.u33",  0x000000, 0x100000, CRC(eb58c35d) SHA1(762c5219de6f729a0fc1df90fce09cdf711c2a1e) )

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_DISPOSE )	/* OKI Sample ROM */
	ROM_LOAD( "ts030.u5",    0x000000, 0x100000, CRC(1d68e9d1) SHA1(aaa64a8e8d7cd7f91d2be346fafb9d1f29b40eda) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is what gets switched */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* Samples */
	ROM_COPY( REGION_USER1, 0x000000, 0x000000, 0x020000)
	ROM_COPY( REGION_USER1, 0x020000, 0x020000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x020000)
	ROM_COPY( REGION_USER1, 0x040000, 0x060000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x080000, 0x020000)
	ROM_COPY( REGION_USER1, 0x060000, 0x0a0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x0c0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x080000, 0x0e0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x100000, 0x020000)
	ROM_COPY( REGION_USER1, 0x0a0000, 0x120000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x140000, 0x020000)
	ROM_COPY( REGION_USER1, 0x0c0000, 0x160000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x180000, 0x020000)
	ROM_COPY( REGION_USER1, 0x0e0000, 0x1a0000, 0x020000)
	ROM_FILL(                         0x1c0000, 0x020000, 0x00 )
	ROM_FILL(                         0x1e0000, 0x020000, 0x00 )
ROM_END

ROM_START( bakubrkr )
 	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "ts100j.u18", 0x000000, 0x040000, CRC(8cc0a4fd) SHA1(e7e18b5ea236522a79ba9db8f573ac8f7ade504b) )
	ROM_LOAD16_BYTE( "ts101j.u19", 0x000001, 0x040000, CRC(aea92195) SHA1(e89f964e7e936fd7774f21956eb4ff5c9104837b) )

	ROM_REGION( 0x240000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ts001j.u37",  0x000000, 0x080000, CRC(70b66e7e) SHA1(307ba27b623f67ee4b4023179870c270bac8ea22) )
	ROM_RELOAD(       0x100000, 0x080000             )
	ROM_LOAD( "ts000j.u38",  0x080000, 0x080000, CRC(a7a94143) SHA1(d811a7597402c161850ddf98cdb00661ea506c7d) )
	ROM_RELOAD(       0x180000, 0x080000             )
	ROM_LOAD( "ts002j.u36",  0x200000, 0x040000, CRC(611271e6) SHA1(811c21822b074fbb4bb809fed29d48bbd51d57a0) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles */
	ROM_LOAD( "ts010.u4",  0x000000, 0x100000, CRC(df935324) SHA1(73b7aff8800a4e88a47ad426190b73dabdfbf142) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles */
	ROM_LOAD( "ts020.u33",  0x000000, 0x100000, CRC(eb58c35d) SHA1(762c5219de6f729a0fc1df90fce09cdf711c2a1e) )

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_DISPOSE )	/* OKI Sample ROM */
	ROM_LOAD( "ts030.u5",    0x000000, 0x100000, CRC(1d68e9d1) SHA1(aaa64a8e8d7cd7f91d2be346fafb9d1f29b40eda) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is what gets switched */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* Samples */
	ROM_COPY( REGION_USER1, 0x000000, 0x000000, 0x020000)
	ROM_COPY( REGION_USER1, 0x020000, 0x020000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x020000)
	ROM_COPY( REGION_USER1, 0x040000, 0x060000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x080000, 0x020000)
	ROM_COPY( REGION_USER1, 0x060000, 0x0a0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x0c0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x080000, 0x0e0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x100000, 0x020000)
	ROM_COPY( REGION_USER1, 0x0a0000, 0x120000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x140000, 0x020000)
	ROM_COPY( REGION_USER1, 0x0c0000, 0x160000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x180000, 0x020000)
	ROM_COPY( REGION_USER1, 0x0e0000, 0x1a0000, 0x020000)
	ROM_FILL(                         0x1c0000, 0x020000, 0x00 )
	ROM_FILL(                         0x1e0000, 0x020000, 0x00 )
ROM_END


/***************************************************************************

                                The Berlin Wall

The Berlin Wall, Kaneko 1991, BW-002

----

BW-004 BW-008                    VU-003
BW-005 BW-009                    VU-003
BW-006 BW-00A                    VU-003
BW-007 BW-00B                          6116-90
                                       6116-90
BW-003                           52256  52256
                                 BW101A BW100A
5864
5864                   MUX2      68000
            VIEW2
BW300
BW-002
BW-001                      42101
                            42101
41464 41464      VU-002
41464 41464                      YM2149  IU-004
41464 41464                      YM2149
                           SWB             BW-000  6295
                           SWA


PALs : BW-U47, BW-U48 (backgrounds encryption)

***************************************************************************/

ROM_START( berlwall )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "u23_01.bin", 0x000000, 0x020000, CRC(76b526ce) SHA1(95ba7cccbe88fd695c28b6a7c25a1afd130c1aa6) )
	ROM_LOAD16_BYTE( "u39_01.bin", 0x000001, 0x020000, CRC(78fa7ef2) SHA1(8392de6e307dcd2bf5bcbeb37d578d33246acfcf) )

	ROM_REGION( 0x120000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bw001",  0x000000, 0x080000, CRC(bc927260) SHA1(44273a8b6a041504d54da4a7897adf23e3e9db10) )
	ROM_LOAD( "bw002",  0x080000, 0x080000, CRC(223f5465) SHA1(6ed077514ab4370a215a4a60c3aecc8b72ed1c97) )
	ROM_LOAD( "bw300",  0x100000, 0x020000, CRC(b258737a) SHA1(b5c8fe44a8dcfc19bccba896bdb73030c5843544) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "bw003",  0x000000, 0x080000, CRC(fbb4b72d) SHA1(07a0590f18b3bba1843ef6a89a5c214e8e605cc3) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* High Color Background */
	ROM_LOAD16_BYTE( "bw004",  0x000000, 0x080000, CRC(5300c34d) SHA1(ccb12ea05f89ef68bcfe003faced2ffea24c4bf0) )
	ROM_LOAD16_BYTE( "bw008",  0x000001, 0x080000, CRC(9aaf2f2f) SHA1(1352856159e19f07e8e30f9c44b21347103ce024) ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw005",  0x100000, 0x080000, CRC(16db6d43) SHA1(0158d0278d085487400ad4384b8cc9618503319e) )
	ROM_LOAD16_BYTE( "bw009",  0x100001, 0x080000, CRC(1151a0b0) SHA1(584a0da7eb7f06450f95e76faa20d19f053cb74c) ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw006",  0x200000, 0x080000, CRC(73a35d1f) SHA1(af919cf858c5923aea45e0d8d91493e6284cb99e) )
	ROM_LOAD16_BYTE( "bw00a",  0x200001, 0x080000, CRC(f447dfc2) SHA1(1254eafea92e8e416deedf21cb01990ffc4f896c) ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw007",  0x300000, 0x080000, CRC(97f85c87) SHA1(865e076e098c49c96639f62be793f2de24b4926b) )
	ROM_LOAD16_BYTE( "bw00b",  0x300001, 0x080000, CRC(b0a48225) SHA1(de256bb6e2a824114274bff0c6c1234934c31c49) ) // FIXED BITS (xxxxxxx0)

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bw000",  0x000000, 0x040000, CRC(d8fe869d) SHA1(75e9044c4164ca6db9519fcff8eca6c8a2d8d5d1) )
ROM_END


/*
berlwalt original bug ?

info from SebV:

After about level 5 or so, if you insert a coin when the continue screen
pops up, an error message "Copy Board" pops up.
----------------------------------------
Happened to me when player dies, at level 4.
The message is not written in ROM, its sprite ram adress is (always?)
$30fd40
Routine $337d2 writes it (and routine $5c8c erases it)
The 'COPY BOARD!' message in stored in ROM directly as sprite number
($1cfa8)

$20288a : seems to contain the level number (initialized to 2 (?) when a
game is started, and is incremented by 1 once a level is finished)

01CF3E: move.b  $20288a.l, D0
01CF44: cmpi.b  #$d, D0
01CF48: bcs     1cf76                   ; branch not taken -=> 'COPY BOARD!'
01CF4A: movem.l D0/A0-A2, -(A7)
01CF4E: movea.l A0, A1
01CF50: lea     ($4c,PC), A0; ($1cf9e)
01CF54: nop
01CF56: lea     ($a,A0), A0             ; A0 = $1cfa8 = 'COPY BOARD!'
01CF5A: lea     $30e064.l, A1
01CF60: lea     (-$64,A1), A1
01CF64: lea     ($1d40,A1), A1
01CF68: move.b  #$80, D1
01CF6C: jsr     $33776.l                ; display routine
01CF72: movem.l (A7)+, D0/A0-A2
01CF76:

berlwall levels: 1-1,2,3(anim),...
berlwalt levels: 1-1(anim)2-1/2/3/4/5(anim)3-1/2/3/4/5(anim)4-1(*)

note: berlwall may be genuine while berlwalt may be bootleg! because
stage 1-1 of berlwalt is stage 1-3 of berlwall, and berlwall has
explanation ingame. + the flyer from TAFA shows main character as
berlwall and not berlwalt!
--------------------------------------------------------------------------------
*/

ROM_START( berlwalt )
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "bw100a", 0x000000, 0x020000, CRC(e6bcb4eb) SHA1(220b8fddc79230b4f6a8cf33e1035355c485e8d1) )
	ROM_LOAD16_BYTE( "bw101a", 0x000001, 0x020000, CRC(38056fb2) SHA1(48338b9a5ebea872286541a3c45016673c4af76b) )

	ROM_REGION( 0x120000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bw001",  0x000000, 0x080000, CRC(bc927260) SHA1(44273a8b6a041504d54da4a7897adf23e3e9db10) )
	ROM_LOAD( "bw002",  0x080000, 0x080000, CRC(223f5465) SHA1(6ed077514ab4370a215a4a60c3aecc8b72ed1c97) )
	ROM_LOAD( "bw300",  0x100000, 0x020000, CRC(b258737a) SHA1(b5c8fe44a8dcfc19bccba896bdb73030c5843544) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "bw003",  0x000000, 0x080000, CRC(fbb4b72d) SHA1(07a0590f18b3bba1843ef6a89a5c214e8e605cc3) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* High Color Background */
	ROM_LOAD16_BYTE( "bw004",  0x000000, 0x080000, CRC(5300c34d) SHA1(ccb12ea05f89ef68bcfe003faced2ffea24c4bf0) )
	ROM_LOAD16_BYTE( "bw008",  0x000001, 0x080000, CRC(9aaf2f2f) SHA1(1352856159e19f07e8e30f9c44b21347103ce024) ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw005",  0x100000, 0x080000, CRC(16db6d43) SHA1(0158d0278d085487400ad4384b8cc9618503319e) )
	ROM_LOAD16_BYTE( "bw009",  0x100001, 0x080000, CRC(1151a0b0) SHA1(584a0da7eb7f06450f95e76faa20d19f053cb74c) ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw006",  0x200000, 0x080000, CRC(73a35d1f) SHA1(af919cf858c5923aea45e0d8d91493e6284cb99e) )
	ROM_LOAD16_BYTE( "bw00a",  0x200001, 0x080000, CRC(f447dfc2) SHA1(1254eafea92e8e416deedf21cb01990ffc4f896c) ) // FIXED BITS (xxxxxxx0)
	ROM_LOAD16_BYTE( "bw007",  0x300000, 0x080000, CRC(97f85c87) SHA1(865e076e098c49c96639f62be793f2de24b4926b) )
	ROM_LOAD16_BYTE( "bw00b",  0x300001, 0x080000, CRC(b0a48225) SHA1(de256bb6e2a824114274bff0c6c1234934c31c49) ) // FIXED BITS (xxxxxxx0)

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "bw000",  0x000000, 0x040000, CRC(d8fe869d) SHA1(75e9044c4164ca6db9519fcff8eca6c8a2d8d5d1) )
ROM_END


/***************************************************************************

                            Blaze On (Japan version)

CPU:          TMP68HC000-12/D780C-2(Z80)
SOUND:        YM2151
OSC:          13.3330/16.000MHz
CUSTOM:       KANEKO VU-002 x2
              KANEKO 23160-509 VIEW2-CHIP
              KANEKO MUX2-CHIP
              KANEKO HELP1-CHIP

---------------------------------------------------
 filemanes          devices       kind
---------------------------------------------------
 BZ_PRG1.U80        27C020        68000 main prg.
 BZ_PRG2.U81        27C020        68000 main prg.
 3.U45              27C010        Z80 sound prg.
 BZ_BG.U2           57C8200       BG CHR
 BZ_SP1.U20         27C8001       OBJ
 BZ_SP2.U21         27C8001       OBJ
 BZ_SP1.U68 ( == BZ_SP1.U20)
 BZ_SP2.U86 ( == BZ_SP2.U21)

***************************************************************************/

ROM_START( blazeon )
 	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "bz_prg1.u80", 0x000000, 0x040000, CRC(8409e31d) SHA1(a9dfc299f4b457df190314401aef309adfaf9bae) )
	ROM_LOAD16_BYTE( "bz_prg2.u81", 0x000001, 0x040000, CRC(b8a0a08b) SHA1(5f275b98d3e49a834850b45179d26e8c2f9fd604) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* Z80 Code */
	ROM_LOAD( "3.u45", 0x000000, 0x020000, CRC(52fe4c94) SHA1(896230e4627503292575bbd84edc3cf9cb18b27e) )	// 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bz_sp1.u20", 0x000000, 0x100000, CRC(0d5809a1) SHA1(e72669f95b050d1967d10a865bab8f3634c9daad) )
	ROM_LOAD( "bz_sp2.u21", 0x100000, 0x100000, CRC(56ead2bd) SHA1(463723f3c533603ce3a95310e9ce12b4e582b52d) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "bz_bg.u2", 0x000000, 0x100000, CRC(fc67f19f) SHA1(f5d9e037a736b0932efbfb48587de08bec93df5d) )
ROM_END

/***************************************************************************

                                Blood Warrior

Kaneko 1994

TOP BOARD: Z09AF-005

CPU - Toshiba TMP68HC000N - 16
SOUND - OKI M6295  x2
QUARTZ OSCILLATORS AT 27.0000MHz, 16.0000MHz, 20.0000MHz and 33.3330MHz
RAM - LC3664 x6, 424260 x2, LH52B256D x6, D42101C x2


PCU11.u11 - 18CV8PC    \
PCU15.u15 - 18CV8PC     | NEAR OKI
Z091P016.u16 - 18CV8PC /
Z091P013.u13 - 18CV8PC \
PCU17.u17 - 18CV8PC     |
PCU14.u14 - 18CV8PC     | NEAR 68000
PCU94.u94 - 18CV8PC     |
PCU92.u92 - 18CV8PC     |
Z092P093.u93 - 18CV8PC /
ALL ABOVE NOT DUMPED

Custom Chips

231609-509 View2-Chip x2
KC-002 L0002 023 9321EK702
9343T - 44 pin PQFP (NEAR JAMMMA CONNECTOR)

BOTTOM BOARD: Z09AF-ROM4

TBS0P01 452 9339PK001 - MCU

ofs1g006.u6 - GAL16V8B
ofs1g007.u7 - GAL16V8B
ofs1p059.u59 - 18CV8PC
ofs1p511.u511 - 18CV8PC
ALL ABOVE NOT DUMPED

ROMS

9346.u126 - 93C46
ofdox3.U124 - 27C010
ofp1f3.U513 - 27C4000
ofpof3.U514 - 27C4000
of101f0223.U101 - 27C800
of20902011.u17 - 27C080
of210000213.u19 - 27C080
of2110215.u21 - 27C080
of21200217.u23 - 27C080
of21300219.u25 - 27C080
of21400221.u27 - 27C080
of1000222.u99 - 27C800
of0010226.u55 - 27C800
of3000225.u51 - 27C800
of2080209.u28 - 27V160
of2060207.u14 - 27V160
of2070208.u15 - 27V160
of2050206.u13 - 27V160
of2040205.u12 - 27V160
of2030204.u11 - 27V160
of2020203.u10 - 27V160
of2010202.u9 - 27V160
of2000201.u8 - 27V160
of209e0210.u16 - 27C080
of210e0212.u18 - 27C080
of211e0214.u20 - 27C080
of212e0216.u22 - 27C080
of213e0218.u24 - 27C080
of214e0220.u26 - 27C080

***************************************************************************/

ROM_START( bloodwar )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "pof3.514", 0x000000, 0x080000, CRC(0c93da15) SHA1(65b6b1b4acfc32c551ae4fbe6a13f7f2b8554dbf) )
	ROM_LOAD16_BYTE( "p1f3.513", 0x000001, 0x080000, CRC(894ecbe5) SHA1(bf403d19e6315266114ac742a08cac903e7b54b5) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD16_WORD_SWAP( "dox3.124",  0x000000, 0x020000, CRC(399f2005) SHA1(ff0370724770c35963953fd9596d9f808ba87d8f) )

	ROM_REGION( 0x1e00000, REGION_GFX1, 0 )	/* Sprites */
	ROM_LOAD       ( "2000201.8",   0x0000000, 0x200000, CRC(bba63025) SHA1(daec5285469ee953f6f838fe3cb3903524e9ac39) )
	ROM_LOAD       ( "2010202.9",   0x0200000, 0x200000, CRC(4ffd9ddc) SHA1(62bc8c0ed2efab407fc2956c514c3e732bcc47ee) )
	ROM_LOAD       ( "2020203.10",  0x0400000, 0x200000, CRC(fbcc5363) SHA1(9eff48c29d5c887d39e4db442c6ee51ec879521e) )
	ROM_LOAD       ( "2030204.11",  0x0600000, 0x200000, CRC(8e818ce9) SHA1(bc37d35247edfc563400cd67459d455b1fea6eab) )
	ROM_LOAD       ( "2040205.12",  0x0800000, 0x200000, CRC(70c4a76b) SHA1(01b17bda156f2e6f480bdc976927c8bba47c1186) )
	ROM_LOAD       ( "2050206.13",  0x0a00000, 0x200000, CRC(80c667bb) SHA1(7edf33c713c8448ff73fa84b9f7684dd4d46eed1) )
	ROM_LOAD       ( "2060207.14",  0x0c00000, 0x200000, CRC(c2028c97) SHA1(ac3b73ff34f84015432ceb22cf9c57ab0ff07a70) )
	ROM_LOAD       ( "2070208.15",  0x0e00000, 0x200000, CRC(b1f30c61) SHA1(2ae010c10b7a2ae09df904f7ea81425e80389622) )
	ROM_LOAD       ( "2080209.28",  0x1000000, 0x200000, CRC(a8f29545) SHA1(5d018147aa71207f679909343104deaa0f08fd9d) )
	ROM_LOAD16_BYTE( "209e0210.16", 0x1200000, 0x100000, CRC(93018468) SHA1(d156f408a78fbd736048ce33e44c0b1e10403b0e) )
	ROM_LOAD16_BYTE( "20902011.17", 0x1200001, 0x100000, CRC(3fb226a1) SHA1(efba54f82fb9914559faad5fba92aa108ec039d5) )
	ROM_LOAD16_BYTE( "210e0212.18", 0x1400000, 0x100000, CRC(80f3fa1b) SHA1(ca0e84cb47228ef5ac3b94238a33c3ebc3c2f528) )
	ROM_LOAD16_BYTE( "21000213.19", 0x1400001, 0x100000, CRC(8ca3a3d6) SHA1(b6f3876f987ce6828bfa26ca492ff6ca2d282d80) )
	ROM_LOAD16_BYTE( "211e0214.20", 0x1600000, 0x100000, CRC(8d3d96f7) SHA1(0a7c459f02938f86d53979498647c73837eb9e51) )
	ROM_LOAD16_BYTE( "21100215.21", 0x1600001, 0x100000, CRC(78268230) SHA1(63fbf88551f6fde833222ae6a4382891e1bf5b39) )
	ROM_LOAD16_BYTE( "212e0216.22", 0x1800000, 0x100000, CRC(5a013d99) SHA1(c4af944c8f0b33a93b4bb083e32e2901c5607a39) )
	ROM_LOAD16_BYTE( "21200217.23", 0x1800001, 0x100000, CRC(84ed25bd) SHA1(ddff811d326586eb7353230e74db37867af075eb) )
	ROM_LOAD16_BYTE( "213e0218.24", 0x1a00000, 0x100000, CRC(861bc5b1) SHA1(a85936781a56b5406bee2e4f36fadcb5b9f43b05) )
	ROM_LOAD16_BYTE( "21300219.25", 0x1a00001, 0x100000, CRC(a79b8119) SHA1(62e3fb28fd3a538a8191a51242dbed7e88c62a54) )
	ROM_LOAD16_BYTE( "214e0220.26", 0x1c00000, 0x100000, CRC(43c622de) SHA1(73efe57233f056127e2d34590c624f39d1c0ab79) )
	ROM_LOAD16_BYTE( "21400221.27", 0x1c00001, 0x100000, CRC(d10bf03c) SHA1(a81d6b7df7382fc8d50614c1332611e0c202b805) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* Tiles (scrambled) */
	ROM_LOAD( "3000225.51", 0x000000, 0x100000, CRC(fbc3c08a) SHA1(0ba52b381e7a10fb1513244b394438b440950af3) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* Tiles (scrambled) */
	ROM_LOAD( "3010226.55", 0x000000, 0x100000, CRC(fcf215de) SHA1(83015f10e62b917efd6e3edfbd45fb8f9b35db2b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "101f0223.101", 0x000000, 0x100000, CRC(295f3c93) SHA1(558698f1d04b23dd2a73e2eae5ecce598defb228) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "1000222.99",   0x000000, 0x100000, CRC(42b12269) SHA1(f9d9c42057e176710f09e8db0bfcbf603c15ca11) )
ROM_END


/***************************************************************************

                            Great 1000 Miles Rally

GMMU2+1 512K * 2    68k
GMMU23  1M      OKI6295: 00000-2ffff + chunks of 0x10000 with headers
GMMU24  1M      OKI6295: chunks of 0x40000 with headers - FIRST AND SECOND HALF IDENTICAL

GMMU27  2M      sprites
GMMU28  2M      sprites
GMMU29  2M      sprites
GMMU30  512k    sprites

GMMU64  1M      sprites - FIRST AND SECOND HALF IDENTICAL
GMMU65  1M      sprites - FIRST AND SECOND HALF IDENTICAL

GMMU52  2M      tiles


---------------------------------------------------------------------------
                                Game code
---------------------------------------------------------------------------

100000.b    <- (!b00000.b) & 7f [1p]
    01.b    previous value of the above
    02.b    bits gone high

100008.b    <- (!b00002.b) & 7f [2p]

100010.b    <- !b00004.b [coins]
    11.b    previous value of the above
    12.b    bits gone high

100013.b    <- b00006.b (both never accessed again?)

100015.b    <- wheel value

600000.w    <- 100a20.w + 100a30.w      600002.w    <- 100a22.w + 100a32.w
600004.w    <- 100a24.w + 100a34.w      600006.w    <- 100a26.w + 100a36.w

680000.w    <- 100a28.w + 100a38.w      680002.w    <- 100a2a.w + 100a3a.w
680004.w    <- 100a2c.w + 100a3c.w      680006.w    <- 100a2e.w + 100a3e.w

101265.b    <- DSW (from 206000)
101266      <- Settings from NVRAM (0x80 bytes from 208000)

1034f8.b    credits
103502.b    coins x ..
103503.b    .. credits

1035ec.l    *** Time (BCD: seconds * 10000) ***
103e64.w    *** Speed << 4 ***

10421a.b    bank for the oki mapped at 800000
104216.b    last value of the above

10421c.b    bank for the oki mapped at 880000
104218.b    last value of the above

ROUTINES:

dd6 print string: a2->scr ; a1->string ; d1.l = xpos.w<<6|ypos.w<<6

Trap #2 = 43a0 ; d0.w = routine index ; (where not specified: 43c0):
1:  43C4    2:  43F8    3:  448E    4:  44EE
5:  44D2    6:  4508    7:  453A    10: 0AF6
18: 4580    19: 4604
20> 2128    writes 700000-70001f
21: 21F6
24> 2346    clears 400000-401407 (641*8 = $281*8)
30> 282A    writes 600008/9/b/e-f, 680008/9/b/e-f
31: 295A
32> 2B36    100a30-f <- 100a10-f
34> 2B4C    clears 500000-503fff, 580000-583fff
35> 2B9E    d1.w = selects between: 500000;501000;580000;581000.
            Fill 0x1000 bytes from there with d2.l

70: 2BCE>   11d8a
71: 2BD6
74: 2BDE    90: 3D44
91> 3D4C    wait for bit 0 of d00000 to be 0
92> 3D5C    200010.w<-D1    200012.w<-D2    200014.w<-D3
f1: 10F6

***************************************************************************/

/*  This version displays:

    tb05mm-eu "1000 miglia"
    master up= 94/07/18 15:12:35            */

ROM_START( gtmr )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "u2.bin", 0x000000, 0x080000, CRC(031799f7) SHA1(a59a9635002d139247828e3b74f6cf2fbdd5e569) )
	ROM_LOAD16_BYTE( "u1.bin", 0x000001, 0x080000, CRC(6238790a) SHA1(a137fd581138804534f3193068f117611a982004) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "mmd0x2.u124.bin",  0x000000, 0x020000, CRC(3d7cb329) SHA1(053106acde642a414fde0b01105fe6762b6a10f6) ) // from gtmra

	ROM_REGION( 0x840000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "mm-200-402-s0.bin",  0x000000, 0x200000, CRC(c0ab3efc) SHA1(e6cd15480977b036234d91e6f3a6e21b7f0a3c3e) )
	ROM_LOAD( "mm-201-403-s1.bin",  0x200000, 0x200000, CRC(cf6b23dc) SHA1(ccfd0b17507e091e55c169361cd6a6b19641b717) )
	ROM_LOAD( "mm-202-404-s2.bin",  0x400000, 0x200000, CRC(8f27f5d3) SHA1(219a86446ce2556682009d8aff837480f040a01e) )
	ROM_LOAD( "mm-203-405-s3.bin",  0x600000, 0x080000, CRC(e9747c8c) SHA1(2507102ec34755c6f110eadb3444e6d3a3474051) )
  	ROM_LOAD16_BYTE( "mms1x2.u30.bin",  0x800001, 0x020000, CRC(b42b426f) SHA1(6aee5759b5f0786c5ee074d9df3d2716919ea621) )
  	ROM_LOAD16_BYTE( "mms0x2.u29.bin",  0x800000, 0x020000, CRC(bd22b7d2) SHA1(ef82d00d72439590c71aed33ecfabc6ee71a6ff9) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "mm-300-406-a0.bin",  0x000000, 0x200000, CRC(b15f6b7f) SHA1(5e84919d788add53fc87f4d85f437df413b1dbc5) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_COPY(REGION_GFX2,0,0,0x200000) // it isn't on the board twice.

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "mm-100-401-e0.bin",  0x000000, 0x100000, CRC(b9cbfbee) SHA1(051d48a68477ef9c29bd5cc0bb7955d513a0ab94) )	// 16 x $10000

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_ERASE00 )	/* Samples */
	/* Not present on this board */
ROM_END

ROM_START( gtmra )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "mmp0x2.u514.bin", 0x000000, 0x080000, CRC(ba4a77c8) SHA1(efb6ae0e7aa71ab0c5f486f799bf31edcec24e2b) )
	ROM_LOAD16_BYTE( "mmp1x2.u513.bin", 0x000001, 0x080000, CRC(a2b9034e) SHA1(466bcb1bf7124eb15d23b25c4e1307b9706474ec) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "mmd0x2.u124.bin",  0x000000, 0x020000, CRC(3d7cb329) SHA1(053106acde642a414fde0b01105fe6762b6a10f6) )

	ROM_REGION( 0x840000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "mm-200-402-s0.bin",  0x000000, 0x200000, CRC(c0ab3efc) SHA1(e6cd15480977b036234d91e6f3a6e21b7f0a3c3e) )
	ROM_LOAD( "mm-201-403-s1.bin",  0x200000, 0x200000, CRC(cf6b23dc) SHA1(ccfd0b17507e091e55c169361cd6a6b19641b717) )
	ROM_LOAD( "mm-202-404-s2.bin",  0x400000, 0x200000, CRC(8f27f5d3) SHA1(219a86446ce2556682009d8aff837480f040a01e) )
	ROM_LOAD( "mm-203-405-s3.bin",  0x600000, 0x080000, CRC(e9747c8c) SHA1(2507102ec34755c6f110eadb3444e6d3a3474051) )
  	ROM_LOAD16_BYTE( "mms1x2.u30.bin",  0x800001, 0x020000, CRC(b42b426f) SHA1(6aee5759b5f0786c5ee074d9df3d2716919ea621) )
  	ROM_LOAD16_BYTE( "mms0x2.u29.bin",  0x800000, 0x020000, CRC(bd22b7d2) SHA1(ef82d00d72439590c71aed33ecfabc6ee71a6ff9) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "mm-300-406-a0.bin",  0x000000, 0x200000, CRC(b15f6b7f) SHA1(5e84919d788add53fc87f4d85f437df413b1dbc5) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_COPY(REGION_GFX2,0,0,0x200000) // it isn't on the board twice.

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "mm-100-401-e0.bin",  0x000000, 0x100000, CRC(b9cbfbee) SHA1(051d48a68477ef9c29bd5cc0bb7955d513a0ab94) )	// 16 x $10000

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_ERASE00 )	/* Samples */
	/* Not present on this board */
ROM_END

/* The evolution and USA versions seem to be more like GTMR 1.5, they have some fairly significant changes */

/*  This version displays:

    tb05mm-eu "1000 miglia"
    master up= 94/09/06 14:49:19            */

ROM_START( gtmre )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "gmmu2.bin", 0x000000, 0x080000, CRC(36dc4aa9) SHA1(0aea4dc169d7aad2ea957a1de698d1fa12c71556) )
	ROM_LOAD16_BYTE( "gmmu1.bin", 0x000001, 0x080000, CRC(8653c144) SHA1(a253a01327a9443337a55a13c063ea5096444c4c) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "mcu_code.u12",  0x000000, 0x020000, NO_DUMP )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	/* fill the 0x700000-7fffff range first, with the second of the identical halves */
	ROM_LOAD16_BYTE( "gmmu64.bin",  0x600000, 0x100000, CRC(57d77b33) SHA1(f7ae28ae889be4442b7b236705943eaad1f0c84e) )	// HALVES IDENTICAL
	ROM_LOAD16_BYTE( "gmmu65.bin",  0x600001, 0x100000, CRC(05b8bdca) SHA1(44471d66787d5b48ae8b13676f42f27af44e5c6a) )	// HALVES IDENTICAL
	ROM_LOAD( "gmmu27.bin",  0x000000, 0x200000, CRC(c0ab3efc) SHA1(e6cd15480977b036234d91e6f3a6e21b7f0a3c3e) )
	ROM_LOAD( "gmmu28.bin",  0x200000, 0x200000, CRC(cf6b23dc) SHA1(ccfd0b17507e091e55c169361cd6a6b19641b717) )
	ROM_LOAD( "gmmu29.bin",  0x400000, 0x200000, CRC(8f27f5d3) SHA1(219a86446ce2556682009d8aff837480f040a01e) )
	ROM_LOAD( "gmmu30.bin",  0x600000, 0x080000, CRC(e9747c8c) SHA1(2507102ec34755c6f110eadb3444e6d3a3474051) )
	/* codes 6800-6fff are explicitly skipped */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, CRC(b15f6b7f) SHA1(5e84919d788add53fc87f4d85f437df413b1dbc5) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_COPY(REGION_GFX2,0,0,0x200000) // it isn't on the board twice.

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "gmmu23.bin",  0x000000, 0x100000, CRC(b9cbfbee) SHA1(051d48a68477ef9c29bd5cc0bb7955d513a0ab94) )	// 16 x $10000

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "gmmu24.bin",  0x000000, 0x100000, CRC(380cdc7c) SHA1(ba7f51201b0f2bf15e66557e45bb2af5cf797779) )	//  2 x $40000 - HALVES IDENTICAL
ROM_END


/*  This version displays:

    tb05mm-eu "1000 miglia"
    master up= 94/09/06 20:30:39            */

ROM_START( gtmrusa )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "gtmrusa.u2", 0x000000, 0x080000, CRC(5be615c4) SHA1(c14d11a5bf6e025a65b932039165302ff407c4e1) )
	ROM_LOAD16_BYTE( "gtmrusa.u1", 0x000001, 0x080000, CRC(ae853e4e) SHA1(31eaa73b0c5ddab1292f521ceec43b202653efe9) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code? */
	ROM_LOAD( "gtmrusa.u12",  0x000000, 0x020000, CRC(2e1a06ff) SHA1(475a7555653eefac84307492a385895b839cab0d) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	/* fill the 0x700000-7fffff range first, with the second of the identical halves */
	ROM_LOAD16_BYTE( "gmmu64.bin",  0x600000, 0x100000, CRC(57d77b33) SHA1(f7ae28ae889be4442b7b236705943eaad1f0c84e) )	// HALVES IDENTICAL
	ROM_LOAD16_BYTE( "gmmu65.bin",  0x600001, 0x100000, CRC(05b8bdca) SHA1(44471d66787d5b48ae8b13676f42f27af44e5c6a) )	// HALVES IDENTICAL
	ROM_LOAD( "gmmu27.bin",  0x000000, 0x200000, CRC(c0ab3efc) SHA1(e6cd15480977b036234d91e6f3a6e21b7f0a3c3e) )
	ROM_LOAD( "gmmu28.bin",  0x200000, 0x200000, CRC(cf6b23dc) SHA1(ccfd0b17507e091e55c169361cd6a6b19641b717) )
	ROM_LOAD( "gmmu29.bin",  0x400000, 0x200000, CRC(8f27f5d3) SHA1(219a86446ce2556682009d8aff837480f040a01e) )
	ROM_LOAD( "gmmu30.bin",  0x600000, 0x080000, CRC(e9747c8c) SHA1(2507102ec34755c6f110eadb3444e6d3a3474051) )
	/* codes 6800-6fff are explicitly skipped */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "gmmu52.bin",  0x000000, 0x200000, CRC(b15f6b7f) SHA1(5e84919d788add53fc87f4d85f437df413b1dbc5) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_COPY(REGION_GFX2,0,0,0x200000) // it isn't on the board twice.

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "gmmu23.bin",  0x000000, 0x100000, CRC(b9cbfbee) SHA1(051d48a68477ef9c29bd5cc0bb7955d513a0ab94) )	// 16 x $10000

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "gmmu24.bin",  0x000000, 0x100000, CRC(380cdc7c) SHA1(ba7f51201b0f2bf15e66557e45bb2af5cf797779) )	//  2 x $40000 - HALVES IDENTICAL
ROM_END


/***************************************************************************

                        Great 1000 Miles Rally 2

Bootup displays "TB06MM2EX 1000 Miglia 2"
                "Master Up 95-04-04 19:39:01"

Top board
---------
PCB ID : M201F00138  KANEKO AX-SYSTEM BOARD
CPU    : TMP68HC000N-16 (68000)
SOUND  : Oki M6295 (x2)
OSC    : 27.000MHz, 16.000MHz, 20.000MHz, 33.3330MHz
RAM    : LC3664 (28 pin SOIC, x4)
         UT6264 (28 pin SOIC, x2)
         LH628256 (28 pin DIP, x6)
         D42101 (24 pin DIP, x2)
         424260 (40 pin SOIC, x2)
PALs   : AXOP048, AXOP049, AXOP050 (GAL16V8, near M6295's)
         AXOP021 \
         AXOP022  |
         AXOP062  |
         AXOP063  |  (18CV8     )
         AXOP064  |  (near 68000)
         AXOP070  |
         AXOP071  |
         AXOP089 /

OTHER  : Custom chips
                      Kaneko Japan 9448 TA (44 pin PQFP, near JAMMA connector)
                      Kaneko VIEW2-CHIP (x2, 144 pin PQFP)
                      Kaneko KC002 L0002 023 9339EK706 (208 pin PQFP)

ROMs   : None


Bottom board
------------
PCB ID : AX09S00138  KANEKO AX-SYSTEM BOARD ROM-08
DIP    : 8 position (x1)
RAM    : 6116 (x4)
PALs   : COMUX4, COMUX4 (GAL16V8, near U21)
         MMs4P067 (18CV8, near U47)
         MMs6G095 (GAL16V8, near U94)
         MMs4G084 (GAL16V8, near U50)
         COMUX2, COMUX3 (GAL16V8, near U33)
         COMUX1, MMS4P004 (18CV8, near U33)

OTHER  : 93C46 EEPROM
         KANEKO TBSOP02 454 9451MK002 (74 pin PQFP, Custom MCU?)

ROMs   :  (filename is ROM Label, extension is PCB 'u' location)

***************************************************************************/

ROM_START( gtmr2 )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "m2p0x1a.u8",  0x000000, 0x080000, CRC(c29039fb) SHA1(a16e8863608353c2931e9d45359fbcec8f11ef9d) )
	ROM_LOAD16_BYTE( "m2p1x1a.u7",  0x000001, 0x080000, CRC(8ef392c4) SHA1(06bd720d931911e32264183dd215ab70ad6d2961) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code? */
	ROM_LOAD( "m2d0x0.u31",        0x000000, 0x020000, CRC(2e1a06ff) SHA1(475a7555653eefac84307492a385895b839cab0d) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "m2-200-0.u49",      0x000000, 0x400000, CRC(93aafc53) SHA1(1d28b6e3bd61ce9c938fc5303aeabcdefa549852) )
	ROM_LOAD( "m2-201-0.u50",      0x400000, 0x200000, CRC(39b60a83) SHA1(aa7b37c7c92bbcf685f4fec84cc6d8a77d26433c) )
	ROM_LOAD( "m2-202-0.u51",      0x600000, 0x200000, CRC(fd06b339) SHA1(5de0af7d23147f6eb403700eabd66794198f3641) )
	ROM_LOAD16_BYTE( "m2s0x1a.u32", 0x700000, 0x080000, CRC(a485eec6) SHA1(f8aff62daed95a63544106472a9ef34902feaaa2) )
	ROM_LOAD16_BYTE( "m2s1x1a.u33", 0x700001, 0x080000, CRC(c5b71bb2) SHA1(874e2a2e19cd8f916afa6fcf54169a8db035fe64) )

	ROM_REGION( 0x440000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "m2-300-0.u89",      0x000000, 0x200000, CRC(4dc42fbb) SHA1(f14c287bc60f561eb9a57db4e3390aae9a81c392) )
	ROM_LOAD( "m2-301-0.u90",      0x200000, 0x200000, CRC(f4e894f2) SHA1(1f983a1d93845fe298afba60d4dacdd1a10cab7f) )
	ROM_LOAD16_BYTE( "m2b0x0.u93", 0x400000, 0x020000, CRC(e023d51b) SHA1(3c9f591f3ca2ee8e1100b83ae8eb593e11e6eac7) )
	ROM_LOAD16_BYTE( "m2b1x0.u94", 0x400001, 0x020000, CRC(03c48bdb) SHA1(f5ba45d026530d46f760cf06d02a1ffcca89aa3c) )

	ROM_REGION( 0x440000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_COPY(REGION_GFX2,0,0,0x440000) // it isn't on the board twice.

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "m2-100-0.u48",      0x000000, 0x100000, CRC(5250fa45) SHA1(b1ad4660906997faea0aa89866de01a0e9f2b61d) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "m2w1x0.u47",        0x040000, 0x040000, CRC(1b0513c5) SHA1(8c9ddef19297e1b39d900297005203b7ff28667e) )
ROM_END

ROM_START( gtmr2a )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "m2p0x1.u8",  0x000000, 0x080000, CRC(525f6618) SHA1(da8008cc7768b4e8c0091aa3ea21752d0ca33691) )
	ROM_LOAD16_BYTE( "m2p1x1.u7",  0x000001, 0x080000, CRC(914683e5) SHA1(dbb2140f7de86073647abc6e73ba739ea201dd30) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code? */
	ROM_LOAD( "m2d0x0.u31",        0x000000, 0x020000, CRC(2e1a06ff) SHA1(475a7555653eefac84307492a385895b839cab0d) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "m2-200-0.u49",      0x000000, 0x400000, CRC(93aafc53) SHA1(1d28b6e3bd61ce9c938fc5303aeabcdefa549852) )
	ROM_LOAD( "m2-201-0.u50",      0x400000, 0x200000, CRC(39b60a83) SHA1(aa7b37c7c92bbcf685f4fec84cc6d8a77d26433c) )
	ROM_LOAD( "m2-202-0.u51",      0x600000, 0x200000, CRC(fd06b339) SHA1(5de0af7d23147f6eb403700eabd66794198f3641) )
	ROM_LOAD16_BYTE( "m2s0x1.u32", 0x700000, 0x080000, CRC(4069d6c7) SHA1(2ed1cbb7ebde8347e0359cd56ee3a0d4d42d551f) )
	ROM_LOAD16_BYTE( "m2s1x1.u33", 0x700001, 0x080000, CRC(c53fe269) SHA1(e6c485bbaea4b67f074b89e047f686f107805713) )

	ROM_REGION( 0x440000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "m2-300-0.u89",      0x000000, 0x200000, CRC(4dc42fbb) SHA1(f14c287bc60f561eb9a57db4e3390aae9a81c392) )
	ROM_LOAD( "m2-301-0.u90",      0x200000, 0x200000, CRC(f4e894f2) SHA1(1f983a1d93845fe298afba60d4dacdd1a10cab7f) )
	ROM_LOAD16_BYTE( "m2b0x0.u93", 0x400000, 0x020000, CRC(e023d51b) SHA1(3c9f591f3ca2ee8e1100b83ae8eb593e11e6eac7) )
	ROM_LOAD16_BYTE( "m2b1x0.u94", 0x400001, 0x020000, CRC(03c48bdb) SHA1(f5ba45d026530d46f760cf06d02a1ffcca89aa3c) )

	ROM_REGION( 0x440000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_COPY(REGION_GFX2,0,0,0x440000) // it isn't on the board twice.

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "m2-100-0.u48",      0x000000, 0x100000, CRC(5250fa45) SHA1(b1ad4660906997faea0aa89866de01a0e9f2b61d) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "m2w1x0.u47",        0x040000, 0x040000, CRC(1b0513c5) SHA1(8c9ddef19297e1b39d900297005203b7ff28667e) )
ROM_END

ROM_START( gtmr2u )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "m2p0a1.u8",  0x000000, 0x080000, CRC(813e1d5e) SHA1(602df02933dc7b77be311113af1d1edad2751cc9) )
	ROM_LOAD16_BYTE( "m2p1a1.u7",  0x000001, 0x080000, CRC(bee63666) SHA1(07585a63f901f50f2a2314eb4dc4307e7028ded7) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code? */
	ROM_LOAD( "m2d0x0.u31",        0x000000, 0x020000, CRC(2e1a06ff) SHA1(475a7555653eefac84307492a385895b839cab0d) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "m2-200-0.u49",      0x000000, 0x400000, CRC(93aafc53) SHA1(1d28b6e3bd61ce9c938fc5303aeabcdefa549852) )
	ROM_LOAD( "m2-201-0.u50",      0x400000, 0x200000, CRC(39b60a83) SHA1(aa7b37c7c92bbcf685f4fec84cc6d8a77d26433c) )
	ROM_LOAD( "m2-202-0.u51",      0x600000, 0x200000, CRC(fd06b339) SHA1(5de0af7d23147f6eb403700eabd66794198f3641) )
	ROM_LOAD16_BYTE( "m2s0a1.u32", 0x700000, 0x080000, CRC(98977171) SHA1(5b69462e07778b5bd1f5119cae6b63ede38cd642) )
	ROM_LOAD16_BYTE( "m2s1a1.u33", 0x700001, 0x080000, CRC(c69a732e) SHA1(810b333f442c0714f4cb8b4a73136d0b44443277) )

	ROM_REGION( 0x440000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "m2-300-0.u89",      0x000000, 0x200000, CRC(4dc42fbb) SHA1(f14c287bc60f561eb9a57db4e3390aae9a81c392) )
	ROM_LOAD( "m2-301-0.u90",      0x200000, 0x200000, CRC(f4e894f2) SHA1(1f983a1d93845fe298afba60d4dacdd1a10cab7f) )
	ROM_LOAD16_BYTE( "m2b0x0.u93", 0x400000, 0x020000, CRC(e023d51b) SHA1(3c9f591f3ca2ee8e1100b83ae8eb593e11e6eac7) )
	ROM_LOAD16_BYTE( "m2b1x0.u94", 0x400001, 0x020000, CRC(03c48bdb) SHA1(f5ba45d026530d46f760cf06d02a1ffcca89aa3c) )

	ROM_REGION( 0x440000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_COPY(REGION_GFX2,0,0,0x440000) // it isn't on the board twice.

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "m2-100-0.u48",      0x000000, 0x100000, CRC(5250fa45) SHA1(b1ad4660906997faea0aa89866de01a0e9f2b61d) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "m2w1a1.u47",        0x000000, 0x080000, CRC(15f25342) SHA1(9947e66575738700345c12c104701b812c62ce03) )
ROM_END

/***************************************************************************

                                Magical Crystals

(c)1991 Kaneko/Atlus
Z00FC-02

CPU  : TMP68HC000N-12
Sound: YM2149Fx2 M6295
OSC  : 16.0000MHz(X1) 12.0000MHz(X2)

ROMs:
mc100j.u18 - Main programs
mc101j.u19 /

mc000.u38  - Graphics (32pin mask)
mc001.u37  | (32pin mask)
mc002j.u36 / (27c010)

mc010.u04 - Graphics (42pin mask)

mc020.u33 - Graphics (42pin mask)

mc030.u32 - Samples (32pin mask)

PALs (18CV8PC):
u08, u20, u41, u42, u50, u51, u54

Custom chips:
KANEKO VU-001 046A (u53, 48pin PQFP)
KANEKO VU-002 052 151021 (u60, 160pin PQFP)
KANEKO 23160-509 9047EAI VIEW2-CHIP (u24 & u34, 144pin PQFP)
KANEKO MUX2-CHIP (u28, 64pin PQFP)
KANEKO IU-001 9045KP002 (u22, 44pin PQFP)
KANEKO I/O JAMMA MC-8282 047 (u5, 46pin)
699206p (u09, 44pin PQFP)
699205p (u10, 44pin PQFP)

Other:
93C46 EEPROM

DIP settings:
1: Flip screen
2: Test mode
3: Unused
4: Unused

Yes, one program rom actually is a 27C010 and the other one is a 27C020

***************************************************************************/

ROM_START( mgcrystl ) /* Master Up: 92/01/10 14:21:30 */
 	ROM_REGION( 0x040000*2, REGION_CPU1, ROMREGION_ERASE )			/* 68000 Code */
	ROM_LOAD16_BYTE( "mc100e02.u18", 0x000000, 0x020000, CRC(246a1335) SHA1(8333945a92e08a7bff425d2d6602557386016dc5) ) /* Labeled as MC100E/U18-02 */
	ROM_LOAD16_BYTE( "mc101e02.u19", 0x000001, 0x040000, CRC(708ea1dc) SHA1(ae6eca6620729bc1e815f1bfbd8fe130f0ba943c) ) /* Labeled as MC101E/U19-02 */

	ROM_REGION( 0x280000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "mc000.u38",    0x000000, 0x100000, CRC(28acf6f4) SHA1(6647ad90ea580b65ed28772f9d65352b06833d0c) )
	ROM_LOAD( "mc001.u37",    0x100000, 0x080000, CRC(005bc43d) SHA1(6f6cd99e8e60562fa86581008455a6d9d646fa95) )
	ROM_RELOAD(               0x180000, 0x080000             )
	ROM_LOAD( "mc002e02.u36", 0x200000, 0x020000, CRC(27ac1056) SHA1(34b07c1a0d403ca45c9849d3d8d311012f787df6) ) /* Labeled as MC002E/U36-02 */
	ROM_RELOAD(               0x220000, 0x020000             )
	ROM_RELOAD(               0x240000, 0x020000             )
	ROM_RELOAD(               0x260000, 0x020000             )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc010.u04",  0x000000, 0x100000, CRC(85072772) SHA1(25e903cc2c893d61db791d1fe60a1205a4395667) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc020.u34",  0x000000, 0x100000, CRC(1ea92ff1) SHA1(66ec53e664b2a5a751a280a538aaeceafc187ceb) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "mc030.u32",  0x000000, 0x040000, CRC(c165962e) SHA1(f7e130db387ae9dcb7223f7ad6e51270d3033bc9) )
ROM_END

ROM_START( mgcrysto ) /* Master Up: 91/12/10 01:56:06 */
 	ROM_REGION( 0x040000*2, REGION_CPU1, ROMREGION_ERASE )			/* 68000 Code */
	ROM_LOAD16_BYTE( "mc100h00.u18", 0x000000, 0x020000, CRC(c7456ba7) SHA1(96c25c3432069373fa86d7af3e093e02e39aea34) ) /* Labeled as MC100H/U18-00 */
	ROM_LOAD16_BYTE( "mc101h00.u19", 0x000001, 0x040000, CRC(ea8f9300) SHA1(0cd0d448805aa45986b63befca00b08fe066dbb2) ) /* Labeled as MC101H/U19-00 */

	ROM_REGION( 0x280000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "mc000.u38",    0x000000, 0x100000, CRC(28acf6f4) SHA1(6647ad90ea580b65ed28772f9d65352b06833d0c) )
	ROM_LOAD( "mc001.u37",    0x100000, 0x080000, CRC(005bc43d) SHA1(6f6cd99e8e60562fa86581008455a6d9d646fa95) )
	ROM_RELOAD(               0x180000, 0x080000             )
	ROM_LOAD( "mc002h00.u36", 0x200000, 0x020000, CRC(22729037) SHA1(de4e1bdab57aa617411b6327f3db4856970e8953) ) /* Labeled as MC002H/U36-00 */
	ROM_RELOAD(               0x220000, 0x020000             )
	ROM_RELOAD(               0x240000, 0x020000             )
	ROM_RELOAD(               0x260000, 0x020000             )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc010.u04",  0x000000, 0x100000, CRC(85072772) SHA1(25e903cc2c893d61db791d1fe60a1205a4395667) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc020.u34",  0x000000, 0x100000, CRC(1ea92ff1) SHA1(66ec53e664b2a5a751a280a538aaeceafc187ceb) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "mc030.u32",  0x000000, 0x040000, CRC(c165962e) SHA1(f7e130db387ae9dcb7223f7ad6e51270d3033bc9) )
ROM_END

ROM_START( mgcrystj ) /* Master Up: 92/01/13 14:44:20 */
 	ROM_REGION( 0x040000*2, REGION_CPU1, ROMREGION_ERASE )			/* 68000 Code */
	ROM_LOAD16_BYTE( "mc100j02.u18", 0x000000, 0x020000, CRC(afe5882d) SHA1(176e6e12e3df63c08d7aff781f5e5a9bd83ec293) ) /* Labeled as MC100J/U18-02 */
	ROM_LOAD16_BYTE( "mc101j02.u19", 0x000001, 0x040000, CRC(60da5492) SHA1(82b90a617d355825624ce9fb30bddf4714bd0d18) ) /* Labeled as MC101J/U19-02 */

	ROM_REGION( 0x280000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "mc000.u38",  0x000000, 0x100000, CRC(28acf6f4) SHA1(6647ad90ea580b65ed28772f9d65352b06833d0c) )
	ROM_LOAD( "mc001.u37",  0x100000, 0x080000, CRC(005bc43d) SHA1(6f6cd99e8e60562fa86581008455a6d9d646fa95) )
	ROM_RELOAD(             0x180000, 0x080000             )
	ROM_LOAD( "mc002e02.u36", 0x200000, 0x020000, CRC(27ac1056) SHA1(34b07c1a0d403ca45c9849d3d8d311012f787df6) ) /* Labeled as MC002J/U36-02, but same as MC002E/U36-02 */
	ROM_RELOAD(             0x220000, 0x020000             )
	ROM_RELOAD(             0x240000, 0x020000             )
	ROM_RELOAD(             0x260000, 0x020000             )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc010.u04",  0x000000, 0x100000, CRC(85072772) SHA1(25e903cc2c893d61db791d1fe60a1205a4395667) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (Scrambled) */
	ROM_LOAD( "mc020.u34",  0x000000, 0x100000, CRC(1ea92ff1) SHA1(66ec53e664b2a5a751a280a538aaeceafc187ceb) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "mc030.u32",  0x000000, 0x040000, CRC(c165962e) SHA1(f7e130db387ae9dcb7223f7ad6e51270d3033bc9) )
ROM_END


/***************************************************************************

                                Sand Scorpion

(C) FACE
68HC000N-12
Z8400AB1
OKI6295, YM2203C
OSC:  16.000mhz,   12.000mhz

SANDSC03.BIN     27C040
SANDSC04.BIN     27C040
SANDSC05.BIN     27C040
SANDSC06.BIN     27C040
SANDSC07.BIN     27C2001
SANDSC08.BIN     27C1001
SANDSC11.BIN     27C2001
SANDSC12.BIN     27C2001

***************************************************************************/

ROM_START( sandscrp )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "sandsc11.bin", 0x000000, 0x040000, CRC(9b24ab40) SHA1(3187422dbe8b15d8053be4cb20e56d3e6afbd5f2) )
	ROM_LOAD16_BYTE( "sandsc12.bin", 0x000001, 0x040000, CRC(ad12caee) SHA1(83267445b89c3cf4dc317106aa68763d2f29eff7) )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "sandsc08.bin", 0x00000, 0x0c000, CRC(6f3e9db1) SHA1(06a04fa17f44319986913bff70433510c89e38f1) )
	ROM_CONTINUE(             0x10000, 0x14000             )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sandsc05.bin", 0x000000, 0x080000, CRC(9bb675f6) SHA1(c3f6768cfd99a0e19ca2224fff9aa4e27ec0da24) )
	ROM_LOAD( "sandsc06.bin", 0x080000, 0x080000, CRC(7df2f219) SHA1(e2a59e201bfededa92d6c86f8dc1b212527ef66f) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layers */
	ROM_LOAD16_BYTE( "sandsc04.bin", 0x000000, 0x080000, CRC(b9222ff2) SHA1(a445da3f7f5dea5ff64bb0b048f624f947875a39) )
	ROM_LOAD16_BYTE( "sandsc03.bin", 0x000001, 0x080000, CRC(adf20fa0) SHA1(67a7a2be774c86916cbb97e4c9b16c2e48125780) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sandsc07.bin", 0x000000, 0x040000, CRC(9870ab12) SHA1(5ea3412cbc57bfaa32a1e2552b2eb46f4ceb5fa8) )
ROM_END


ROM_START( sandscra )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "11.ic4", 0x000000, 0x040000, CRC(80020cab) SHA1(4f1f4d8ea07ad745f2d6d3f800686f07fe4bf20f) )
	ROM_LOAD16_BYTE( "12.ic5", 0x000001, 0x040000, CRC(8df1d42f) SHA1(2a9db5c4b99a8a3f62bffa9ddd96a95e2042602b) )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "sandsc08.bin", 0x00000, 0x0c000, CRC(6f3e9db1) SHA1(06a04fa17f44319986913bff70433510c89e38f1) )
	ROM_CONTINUE(             0x10000, 0x14000             )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ss502.ic16", 0x000000, 0x100000, CRC(d8012ebb) SHA1(975bbb3b57a09e41d2257d4fa3a64097144de554) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layers */
	ROM_LOAD16_WORD_SWAP( "ss501.ic30", 0x000000, 0x100000, CRC(0cf9f99d) SHA1(47f7f120d2bc075bedaff0a44306a8f46a1d848c) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sandsc07.bin", 0x000000, 0x040000, CRC(9870ab12) SHA1(5ea3412cbc57bfaa32a1e2552b2eb46f4ceb5fa8) )
ROM_END


/***************************************************************************

                                Shogun Warriors

Shogun Warriors, Kaneko 1992

   fb010.u65           fb040.u33
   fb011.u66
   rb012.u67
   rb013.u68

                         fb001.u43
     68000-12            fb000.u42  m6295
    51257     fb030.u61  fb002.u44  m6295
    51257     fb031.u62  fb003.u45


                fb021a.u3
                fb021b.u4
                fb022a.u5
   fb023.u7     fb022b.u6
   fb020a.u1    fb020b.u2



---------------------------------------------------------------------------
                                Game code
---------------------------------------------------------------------------

102e04-7    <- !b80004-7
102e18.w    -> $800000
102e1c.w    -> $800002 , $800006
102e1a.w    -> $800004
102e20.w    -> $800008

ROUTINES:

6622    print ($600000)

***************************************************************************/

ROM_START( shogwarr )
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "fb030a.u61", 0x000000, 0x020000, CRC(a04106c6) SHA1(95ab084f2e709be7cec2964cb09bcf5a8d3aacdf) )
	ROM_LOAD16_BYTE( "fb031a.u62", 0x000001, 0x020000, CRC(d1def5e2) SHA1(f442de4433547e52b483549aca5786e4597a7122) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "fb040a.u33",  0x000000, 0x020000, CRC(4b62c4d9) SHA1(35c943dde70438a411714070e42a84366db5ef83) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_ERASEFF )	/* Sprites */
	ROM_LOAD( "fb-020a.u1", 0x000000, 0x100000, CRC(87e55c6d) SHA1(87886c045d7c30b8dee3c8fb0bf8f2cdbc5fd7fb) )
	ROM_LOAD( "fb020b.u2",  0x100000, 0x100000, CRC(276b9d7b) SHA1(7a154f65b4737f2b6ac8effa3352711079f571dc) )
	ROM_LOAD( "fb021a.u3",  0x200000, 0x100000, CRC(7da15d37) SHA1(345cf2242e8210a697294a45197f2b3b974de885) )
	ROM_LOAD( "fb021b.u4",  0x300000, 0x100000, CRC(6a512d7b) SHA1(7fc3002d23262a9a590a283ea9e111e38d889ef2) )
	ROM_LOAD( "fb-22a.u5",  0x400000, 0x100000, CRC(9039e5d3) SHA1(222452cd7947f7c99c68e495835cca62e0449b5c) )
	ROM_LOAD( "fb-22b.u6",  0x500000, 0x100000, CRC(96ac9e54) SHA1(2b066375963dc57fe2ce89d65f6c0a9d183a838d) )
	ROM_LOAD( "fb023.u7",   0x600000, 0x100000, CRC(132794bd) SHA1(bcc73c3183c59a4b66f79d04774773b8a9239501) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* Tiles (scrambled) */
	ROM_LOAD( "fb010.u65",  0x000000, 0x100000, CRC(296ffd92) SHA1(183a28e4594c428deb4726ed22d5166592b94b60) )	// 42 pin mask rom
	ROM_LOAD( "fb011.u66",  0x100000, 0x080000, CRC(500a0367) SHA1(6dc5190f81b21f59ee56a3b2332c8d86d6599782) )	// 40 pin mask rom (verified correct)

	ROM_REGION( 0x300000, REGION_USER1, 0 ) /*samples*/
	ROM_LOAD( "fb001e.u43",  0x000000, 0x080000, CRC(f524aaa1) SHA1(006a886f9df2e57c51b61c6cea70a6574fc20304) )
	ROM_LOAD( "fb000e.u42",  0x080000, 0x080000, CRC(969f1465) SHA1(4f56d1ad341b08f4db41b7ab2498740612ff7c3d) )

	/* Sound data is copied here during driver init */
	ROM_REGION( 0x400000*16, REGION_SOUND1, 0 )
	ROM_FILL(                0x00000, 0x400000*16, 0x00 )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 )
	ROM_LOAD( "fb-002.u45",   0x000000, 0x100000, CRC(010acc17) SHA1(2dc0897c7778eacf6bce12ff0adbadb307ea6c17) )
	ROM_LOAD( "fb-003.u44",   0x100000, 0x100000, CRC(0aea4ac5) SHA1(8f3b30e505b0ba51c140a0a2c071680d4fa05db9) )
ROM_END

ROM_START( shogwarre )
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "fb030e.u61", 0x000000, 0x020000, CRC(32ce7909) SHA1(02d87342706ac9547eb611bd542f8498ba41e34a) )
	ROM_LOAD16_BYTE( "fb031e.u62", 0x000001, 0x020000, CRC(228aeaf5) SHA1(5e080d7975bc5dcf6fccfbc286eafe939496d9bf) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "fb040e.u33",  0x000000, 0x020000, CRC(299d0746) SHA1(67fe3a47ab01fa02ce2bb5836c2041986c19d875) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_ERASEFF )	/* Sprites */
	ROM_LOAD( "fb-020a.u1", 0x000000, 0x100000, CRC(87e55c6d) SHA1(87886c045d7c30b8dee3c8fb0bf8f2cdbc5fd7fb) )
	ROM_LOAD( "fb020b.u2",  0x100000, 0x100000, CRC(276b9d7b) SHA1(7a154f65b4737f2b6ac8effa3352711079f571dc) )
	ROM_LOAD( "fb021a.u3",  0x200000, 0x100000, CRC(7da15d37) SHA1(345cf2242e8210a697294a45197f2b3b974de885) )
	ROM_LOAD( "fb021b.u4",  0x300000, 0x100000, CRC(6a512d7b) SHA1(7fc3002d23262a9a590a283ea9e111e38d889ef2) )
	ROM_LOAD( "fb-22a.u5",  0x400000, 0x100000, CRC(9039e5d3) SHA1(222452cd7947f7c99c68e495835cca62e0449b5c) )
	ROM_LOAD( "fb-22b.u6",  0x500000, 0x100000, CRC(96ac9e54) SHA1(2b066375963dc57fe2ce89d65f6c0a9d183a838d) )
	ROM_LOAD( "fb023.u7",   0x600000, 0x100000, CRC(132794bd) SHA1(bcc73c3183c59a4b66f79d04774773b8a9239501) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* Tiles (scrambled) */
	ROM_LOAD( "fb010.u65",  0x000000, 0x100000, CRC(296ffd92) SHA1(183a28e4594c428deb4726ed22d5166592b94b60) )	// 42 pin mask rom
	ROM_LOAD( "fb011.u66",  0x100000, 0x080000, CRC(500a0367) SHA1(6dc5190f81b21f59ee56a3b2332c8d86d6599782) )	// 40 pin mask rom (verified correct)

	ROM_REGION( 0x300000, REGION_USER1, 0 )
	ROM_LOAD( "fb001e.u43",  0x000000, 0x080000, CRC(f524aaa1) SHA1(006a886f9df2e57c51b61c6cea70a6574fc20304) )
	ROM_LOAD( "fb000e.u42",  0x080000, 0x080000, CRC(969f1465) SHA1(4f56d1ad341b08f4db41b7ab2498740612ff7c3d) )

	/* Sound data is copied here during driver init */
	ROM_REGION( 0x400000*16, REGION_SOUND1, 0 )
	ROM_FILL(                0x00000, 0x400000*16, 0x00 )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 )
	ROM_LOAD( "fb-002.u45",   0x000000, 0x100000, CRC(010acc17) SHA1(2dc0897c7778eacf6bce12ff0adbadb307ea6c17) )
	ROM_LOAD( "fb-003.u44",   0x100000, 0x100000, CRC(0aea4ac5) SHA1(8f3b30e505b0ba51c140a0a2c071680d4fa05db9) )
ROM_END


/***************************************************************************

                                Fujiyama Buster

Japan (c) 1992 Kaneko
This is the Japanese version of Shogun Warriors

Main PCB No: ZO1DK-002 (Same as B.Rap Boys)
 ROM PCB No: ZO5DP
        CPU: TMP68HC000N-12 (Toshiba)
        SND: OKI M6295 x 2
        OSC: 16.000MHz, 12.000MHz
        DIP: 1 x 8 POSITION

OTHER:
93C46 (8 Pin DIP, EEPROM, Linked to FB-040.U33 & CALC3 Chip)
KANEKO JAPAN 9152EV 175101 (160 Pin PQFP)
KANEKO VIEW2-CHIP (144 Pin PQFP)
KANEKO MUX2-CHIP (64 Pin PQFP)
KANEKO CALC3 508 (74 Pin PQFP, MCU, Linked to FB-040.U33)
KANEKO JAPAN 9203 T (44 PIN PQFP)

Differences from Shogun Warriors:

File Name    CRC32       Labelled As   ROM Type
===============================================
fb030j.u61   0x32ce7909  FB030J/U61-00   27C010 | 68000 CPU Code
fb031j.u62   0x000c8c08  FB031J/U62-00   27C010 /

fb040j.u33   0x299d0746  FB040J/U33-00   27C010 - MCU Code? 68000 Code snipets??

fb000j.u43   0xa7522555  FB000J/U43-00   27C040 | Japanese Sound Samples
fb001j_u.101 0x07d4e8e2  FB001J/U101-0   27C040 /

NOTE: U67 & U68 are empty on this Original board.

***************************************************************************/

ROM_START( fjbuster )	// Fujiyama Buster - Japan version of Shogun Warriors
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "fb030j.u61", 0x000000, 0x020000, CRC(32ce7909) SHA1(02d87342706ac9547eb611bd542f8498ba41e34a) )
	ROM_LOAD16_BYTE( "fb031j.u62", 0x000001, 0x020000, CRC(000c8c08) SHA1(439daac1541c34557b5a4308ed69dfebb93abe13) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "fb040j.u33",  0x000000, 0x020000, CRC(299d0746) SHA1(67fe3a47ab01fa02ce2bb5836c2041986c19d875) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_ERASEFF )	/* Sprites */
	ROM_LOAD( "fb-020a.u1", 0x000000, 0x100000, CRC(87e55c6d) SHA1(87886c045d7c30b8dee3c8fb0bf8f2cdbc5fd7fb) )
	ROM_LOAD( "fb020b.u2",  0x100000, 0x100000, CRC(276b9d7b) SHA1(7a154f65b4737f2b6ac8effa3352711079f571dc) )
	ROM_LOAD( "fb021a.u3",  0x200000, 0x100000, CRC(7da15d37) SHA1(345cf2242e8210a697294a45197f2b3b974de885) )
	ROM_LOAD( "fb021b.u4",  0x300000, 0x100000, CRC(6a512d7b) SHA1(7fc3002d23262a9a590a283ea9e111e38d889ef2) )
	ROM_LOAD( "fb-22a.u5",  0x400000, 0x100000, CRC(9039e5d3) SHA1(222452cd7947f7c99c68e495835cca62e0449b5c) )
	ROM_LOAD( "fb-22b.u6",  0x500000, 0x100000, CRC(96ac9e54) SHA1(2b066375963dc57fe2ce89d65f6c0a9d183a838d) )
	ROM_LOAD( "fb023.u7",   0x600000, 0x100000, CRC(132794bd) SHA1(bcc73c3183c59a4b66f79d04774773b8a9239501) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* Tiles (scrambled) */
	ROM_LOAD( "fb010.u65",  0x000000, 0x100000, CRC(296ffd92) SHA1(183a28e4594c428deb4726ed22d5166592b94b60) )	// 42 pin mask rom
	ROM_LOAD( "fb011.u66",  0x100000, 0x080000, CRC(500a0367) SHA1(6dc5190f81b21f59ee56a3b2332c8d86d6599782) )	// 40 pin mask rom (verified correct)


	ROM_REGION( 0x300000, REGION_USER1, 0 )
	ROM_LOAD( "fb000j.u43",    0x000000, 0x080000, CRC(a7522555) SHA1(ea88d90dda20bc309f98a1924c41551e7708e6af) )
	ROM_LOAD( "fb001j_u.101",  0x080000, 0x080000, CRC(07d4e8e2) SHA1(0de911f452ddeb54b0b435b9c1cf5d5881175d44) )

	/* Sound data is copied here during driver init */
	ROM_REGION( 0x400000*16, REGION_SOUND1, 0 )
	ROM_FILL(                0x00000, 0x400000*16, 0x00 )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 )
	ROM_LOAD( "fb-002.u45",   0x000000, 0x100000, CRC(010acc17) SHA1(2dc0897c7778eacf6bce12ff0adbadb307ea6c17) )
	ROM_LOAD( "fb-003.u44",   0x100000, 0x100000, CRC(0aea4ac5) SHA1(8f3b30e505b0ba51c140a0a2c071680d4fa05db9) )
ROM_END

/***************************************************************************

                                B.Rap Boys

B.Rap Boys
Kaneko, 1992

Game is a beat-em up, where a bunch of rapping boys (The B.Rap Boys) beats
up anyone and anything that gets in their way, smashing shop windows
and other property with their fists, chairs, wooden bats and whatever else
they can lay their hands on!


Main PCB No: ZO1DK-002
ROM PCB No:  ZO1DK-EXROM
CPU: TMP68HC000N-12
SND: OKI M6295 x 2
OSC: 16.000MHz, 12.000MHz
DIP: 1 x 8 POSITION
SW1 - PCB location for 2 position DIP but location is unpopulated.
SW2:

                    1   2   3   4   5   6   7   8
SCREEN FLIP     NORMAL      OFF
            FLIP        ON
MODE            NORMAL          OFF
            TEST            ON
SWITCH TEST [1]     NO              OFF
            YES             ON
POSITION #4     NOT USED                OFF
COIN TYPE       LOCAL COIN                  OFF
            COMMON COIN                 ON
GAME TYPE       3 PLAYERS                       OFF
            2 PLAYERS                       ON
DIFFICULTY [2]      EASY                                ON  OFF
            NORMAL                              OFF OFF
            HARD                                OFF ON
            VERY HARD                           ON  ON

[1] This additional test becomes available in test mode when this DIP is ON.
[2] Additional settings available in test mode via another on-screen menu.
Some text is written in Japanese. See scan in archive for details.

Control is via 8 Way Joystick and 2 buttons

There are two extra pin connectors near the JAMMA connector.
Pinouts are....

(A)
10 3P START SW
 9 3P COIN SW
 8 3P BUTTON 2
 7 3P BUTTON 1
 6 3P UP
 5 3P DOWN
 4 3P LEFT
 3 3P RIGHT
 2 GND
 1 GND

(B)
6 COIN COUNTER 3
5 COIN LOCKOUT 3
4 TOTAL COIN COUNTER
3 NC
2 NC
1 NC

RAM:
M5M4464 x 6, M51257AL x 2, KM6264BLS x 2, D42101C x 2, LH5116D x 2, CAT71C256 x 2

OTHER:
93C46 (8 PIN DIP, EEPROM, LINKED TO RB-006.U33)
KANEKO JAPAN 9152EV 175101 (160 PIN PQFP)
KANEKO VIEW2-CHIP (144 PIN PQFP)
KANEKO MUX2-CHIP (64 PIN PQFP)
KANEKO CALC3 508 (74 PIN PQFP, MCU, LINKED TO RB-006.U33)
KANEKO JAPAN 9204 T (44 PIN PQFP)
PALs (x 11, read protected, not dumped)

ROMs:
RB-004.U61  27C010    \     Main program
RB-005.U62  27C010    /

RB-000.U43  2M mask   \
RB-001.U44  4M mask    |    Located near main program and OKI M6295 chips
RB-002.U45  4M mask    |    Possibly sound related / OKI Samples etc..
RB-003.101  4M mask   /

RB-006.U33  27C010      MCU program? (Linked to CALC3 508)

RB-010.U65  8M mask   \
RB-011.U66  8M mask    |    GFX
RB-012.U67  8M mask    |
RB-013.U68  8M mask   /

RB-021.U76  4M mask   \
RB-022.U77  4M mask    |
RB-023.U78  4M mask    |    GFX (located under a plug-in ROM PCB)
RB-024.U79  4M mask   /

RB-020.U2   4M mask   \
RB-025.U4   27C040     |    GFX (located on a plug-in ROM PCB)
RB-026.U5   27C040    /


-----

Game can be ROM Swapped onto a Shogun Warriors board and works

***************************************************************************/

ROM_START( brapboys ) /* Single PCB, fully populated, no rom sub board */
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "rb-030.u61", 0x000000, 0x020000, CRC(ccbe9a53) SHA1(b96baf0ecbf6550bfaf8e512d9275c53a3928bee) )
	ROM_LOAD16_BYTE( "rb-031.u62", 0x000001, 0x020000, CRC(c72b8dda) SHA1(450e1fb8acb140fa0ab23630daad82924f7ce72b) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "rb-040.u33",  0x000000, 0x020000, CRC(757c6e19) SHA1(0f1c37b1b1eb6b230c593e4648c4302f413a61f5) )

	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASEFF )	/* Sprites */
	ROM_LOAD( "rb-020.c2",  0x000000, 0x100000, CRC(ce220d38) SHA1(b88d7c89a3e1a826bf19a1fa692ec77c944596d9) )
	ROM_LOAD( "rb-021.u76", 0x100000, 0x100000, CRC(74001407) SHA1(90002056ceb4e0401246950b8c3f996af0a2463c) )
	ROM_LOAD( "rb-022.u77", 0x200000, 0x100000, CRC(cb3f42dc) SHA1(5415f15621924dd263b8fe7daaf3dc25d470b814) )
	ROM_LOAD( "rb-023.u78", 0x300000, 0x100000, CRC(0e6530c5) SHA1(72bff46f0672927e540f4f3546ae533dd0a231e0) )
	ROM_LOAD( "rb-024.u79", 0x400000, 0x080000, CRC(65fa6447) SHA1(551e540d7bf412753b4a7098e25e6f9d8774bcf4) ) // correct, both halves identical when dumped as larger
	ROM_LOAD( "rb-025.u80", 0x500000, 0x040000, CRC(36cd6b90) SHA1(45c50f2652726ded67c9c24185a71a6367e09270) ) // eprom, contains title logo for this version

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* Tiles (scrambled) */
	ROM_LOAD( "rb-010.u65",  0x000000, 0x100000, CRC(ffd73f87) SHA1(1a661f71976be61c22d9b962850e738ba17f1d45) )
	ROM_LOAD( "rb-011.u66",  0x100000, 0x100000, CRC(d9325f78) SHA1(346832608664aa8f3ac9260a549903386b4125a8) )
	ROM_LOAD( "rb-012.u67",  0x200000, 0x100000, CRC(bfdbe0d1) SHA1(3abc5398ee8ee1871b4d081f9b748539d69bcdba) )
	ROM_LOAD( "rb-013.u68",  0x300000, 0x100000, CRC(28c37fe8) SHA1(e10dd1a810983077328b44e6e33ce2e899c506d2) )

	ROM_REGION( 0x300000, REGION_USER1, 0 ) /* samples */
	/* OKI 1 */
	ROM_LOAD( "rb-000.u43",  0x000000, 0x080000, CRC(58ad1a62) SHA1(1d2643b5f6eac22682972a88d284e00de3e3b223) )
	ROM_LOAD( "rb-003.101",  0x080000, 0x080000, CRC(2cac25d7) SHA1(0412c317bf650a93051b9304d23035efde0c026a) )

	/* OKI 2 */
	ROM_LOAD( "rb-001.u44",  0x100000, 0x100000, CRC(7cf774b3) SHA1(3fb0a5096ce9480f97e311439042eb8cbc26efb4) )
	ROM_LOAD( "rb-002.u45",  0x200000, 0x100000, CRC(e4b30444) SHA1(be6756dce3721226e0b7f5d4d168008c31aeea8e) )

	/* Sound data is copied here during driver init */
	ROM_REGION( 0x40000*16, REGION_SOUND1, 0 )
	ROM_FILL(                0x00000, 0x40000*16, 0x00 )

	ROM_REGION( 0x40000*16, REGION_SOUND2, 0 )
	ROM_FILL(                0x00000, 0x40000*16, 0x00 )
ROM_END

ROM_START( brapboysj ) /* The Japanese version has an extra rom??? and used a rom sub board */
 	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "rb-004.u61", 0x000000, 0x020000, CRC(5432442c) SHA1(f0f7328ece96ef25e6d4fd1958d734f64a9ef371) )
	ROM_LOAD16_BYTE( "rb-005.u62", 0x000001, 0x020000, CRC(118b3cfb) SHA1(1690ecf5c629879bd97131ff77029e152919e45d) )

 	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD( "rb-006.u33",  0x000000, 0x020000, CRC(f1d76b20) SHA1(c571b5f28e529589ee2d7697ef5d4b60ccb66e7a) )

	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_ERASEFF )	/* Sprites */
	/* prety sure all these are at least half size */
	ROM_LOAD( "rb-020.c2",  0x000000, 0x100000, CRC(ce220d38) SHA1(b88d7c89a3e1a826bf19a1fa692ec77c944596d9) )
	ROM_LOAD( "rb-021.u76", 0x100000, 0x100000, CRC(74001407) SHA1(90002056ceb4e0401246950b8c3f996af0a2463c) )
	ROM_LOAD( "rb-022.u77", 0x200000, 0x100000, CRC(cb3f42dc) SHA1(5415f15621924dd263b8fe7daaf3dc25d470b814) )
	ROM_LOAD( "rb-023.u78", 0x300000, 0x100000, CRC(0e6530c5) SHA1(72bff46f0672927e540f4f3546ae533dd0a231e0) )
	ROM_LOAD( "rb-024.u79", 0x400000, 0x080000, CRC(65fa6447) SHA1(551e540d7bf412753b4a7098e25e6f9d8774bcf4) ) // correct, both halves identical when dumped as larger
	ROM_LOAD( "rb-025.u4",  0x500000, 0x080000, CRC(aa795ba5) SHA1(c5256dcceded2e76f548b60c18e51d0dd0209d81) ) // eprom, special title screen
	ROM_LOAD( "rb-026.u5",  0x580000, 0x080000, CRC(bb7604d4) SHA1(57d51ce4ea2000f9a50bae326cfcb66ec494249f) ) // eprom, logs that bounce past

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* Tiles (scrambled) */
	ROM_LOAD( "rb-010.u65",  0x000000, 0x100000, CRC(ffd73f87) SHA1(1a661f71976be61c22d9b962850e738ba17f1d45) )
	ROM_LOAD( "rb-011.u66",  0x100000, 0x100000, CRC(d9325f78) SHA1(346832608664aa8f3ac9260a549903386b4125a8) )
	ROM_LOAD( "rb-012.u67",  0x200000, 0x100000, CRC(bfdbe0d1) SHA1(3abc5398ee8ee1871b4d081f9b748539d69bcdba) )
	ROM_LOAD( "rb-013.u68",  0x300000, 0x100000, CRC(28c37fe8) SHA1(e10dd1a810983077328b44e6e33ce2e899c506d2) )

	ROM_REGION( 0x300000, REGION_USER1, 0 )
	/* OKI 1 */
	ROM_LOAD( "rb-000.u43",  0x000000, 0x080000, CRC(58ad1a62) SHA1(1d2643b5f6eac22682972a88d284e00de3e3b223) )
	ROM_LOAD( "rb-003.101",  0x080000, 0x080000, CRC(2cac25d7) SHA1(0412c317bf650a93051b9304d23035efde0c026a) )

	/* OKI 2 */
	ROM_LOAD( "rb-001.u44",  0x100000, 0x100000, CRC(7cf774b3) SHA1(3fb0a5096ce9480f97e311439042eb8cbc26efb4) )
	ROM_LOAD( "rb-002.u45",  0x200000, 0x100000, CRC(e4b30444) SHA1(be6756dce3721226e0b7f5d4d168008c31aeea8e) )

	/* Sound data is copied here during driver init */
	ROM_REGION( 0x40000*16, REGION_SOUND1, 0 )
	ROM_FILL(                0x00000, 0x40000*16, 0x00 )

	ROM_REGION( 0x40000*16, REGION_SOUND2, 0 )
	ROM_FILL(                0x00000, 0x40000*16, 0x00 )
ROM_END



/**********************************************************************

                            Bonk's Adventure

Bonk's Adventure
Kaneko, 1994

PCB Layout
----------

Z09AF-003
|--------------------------------------------------------|
| LA4460  PC604109.101     PAL   3664 3664  PC500105.55  |
|  M6295   PC603108.102    PAL                           |
|  M6295   PC602107.100  PAL      VIEW2-CHIP             |
|          PC601106.99   PAL                             |
|                                           PC400104.51  |
|                                 3664  3664             |
|              62256                                     |
|J  KANEKO     62256              VIEW2-CHIP             |
|A  JAPAN                                                |
|M  9203T                                     424260     |
|M                                                       |
|A   62256    62256   6116                               |
|    PRG.7    PRG.8   6116      KANEKO    424260         |
|                    3364       KC-002    PAL            |
| 62256    68000     3364                                |
| 62256                                                  |
|          PAL      PAL  20MHz                           |
|          PAL      PAL  16MHz   PC600106.42             |
|                   PAL          PC700107.43             |
|        MCU.124                 PC200102.40             |
| DSW(8) KANEKO  93C46           PC100101.37 PC300103.38 |
|        TBSOP01 27MHz 33.3333MHz                        |
|--------------------------------------------------------|

Notes:
      68000 clock: 16.000MHz
      M6295 clock: 2.000MHz, Sample Rate: /165 (both)
      VSync: 60Hz
      HSync: 15.625kHz

      PC100-PC500: 16M MASK
      PC601-PC604: 8M MASK
      PC600-PC700: 27C4001
            PRG's: 27C4001
            MCU  : 27C010

**********************************************************************/

ROM_START( bonkadv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* 68000 Code */
	ROM_LOAD16_BYTE( "prg.8",        0x000000, 0x080000, CRC(af2e60f8) SHA1(406f79e155d1244b84f8c89c25b37188e1b4f4a6) )
	ROM_LOAD16_BYTE( "prg.7",        0x000001, 0x080000, CRC(a1cc6a78) SHA1(a9cea21a6a0dfd3b0952664681c057190aa27f8c) )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )			/* MCU Code */
	ROM_LOAD16_WORD_SWAP( "mcu.124",	 0x000000, 0x020000, CRC(9d4e2724) SHA1(9dd43703265e39f876877020a0ac3875de6faa8d) )

	ROM_REGION( 0x500000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "pc100101.37",		 0x000000, 0x200000, CRC(c96e7c10) SHA1(607cc7745abc3ff820047e8a00060ece61646623) )
	ROM_LOAD( "pc200102.40",		 0x200000, 0x100000, CRC(c2b7a26a) SHA1(1c8783442e0ccf30c5640866c5493f1dc1dd48f8) )
	ROM_LOAD( "pc300103.38",		 0x300000, 0x100000, CRC(51ee162c) SHA1(b33afc7d1e9f55f191e08472e8c51ca931b0389d) )
	ROM_LOAD16_BYTE( "pc600106.42",  0x400000, 0x080000, CRC(25877026) SHA1(96814d97e9f9284f98c35edfe5e76677ac50dd97) )
	ROM_LOAD16_BYTE( "pc700107.43",  0x400001, 0x080000, CRC(bfe21c44) SHA1(9900a6fe4182b720a90d64d368bd0fd08bf936a8) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "pc400104.51",		 0x000000, 0x100000, CRC(3b176f84) SHA1(0ad6fd5f03d275165490881173bafcb0a94762eb) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tiles (scrambled) */
	ROM_LOAD( "pc500105.55",		 0x000000, 0x100000, CRC(bebb3edc) SHA1(e0fed4307316deaeb811ec29f5022adeaf577a95) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples, plus room for expansion */
	ROM_LOAD( "pc604109.101",		 0x000000, 0x100000, CRC(76025530) SHA1(e0c8192d783057798eea084aa3e87938f6e01cb7) )

	ROM_REGION( 0x300000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "pc601106.99",		 0x000000, 0x100000, CRC(a893651c) SHA1(d221ce89f19a76be497724f6c16fab82c8a52661) )
	ROM_LOAD( "pc602107.100",		 0x100000, 0x100000, CRC(0fbb23aa) SHA1(69b620375c65246317d7105fbc414f3c36e02b2c) )
	ROM_LOAD( "pc603108.102",		 0x200000, 0x100000, CRC(58458985) SHA1(9a846d604ba901eb2a59d2b6cd9c42e3b43adb6a) )

	ROM_REGION16_BE( 0x0080, REGION_USER1, 0 )	/* EEPROM */
	ROM_LOAD16_WORD( "eeprom.126",  0x0000, 0x0080, CRC(d04adc84) SHA1(85415062867605587b09a646ead4700014ebcb5c) )
ROM_END



/***************************************************************************


                                Game drivers


***************************************************************************/

/* Working games */

GAME( 1991, berlwall, 0,        berlwall, berlwall, berlwall,   ROT0,  "Kaneko", "The Berlin Wall", 0 )
GAME( 1991, berlwalt, berlwall, berlwall, berlwalt, berlwall,   ROT0,  "Kaneko", "The Berlin Wall (bootleg ?)", 0 )
GAME( 1991, mgcrystl, 0,        mgcrystl, mgcrystl, kaneko16,   ROT0,  "Kaneko", "Magical Crystals (World, 92/01/10)", 0 )
GAME( 1991, mgcrysto, mgcrystl, mgcrystl, mgcrystl, kaneko16,   ROT0,  "Kaneko", "Magical Crystals (World, 91/12/10)", 0 )
GAME( 1991, mgcrystj, mgcrystl, mgcrystl, mgcrystl, kaneko16,   ROT0,  "Kaneko (Atlus license)", "Magical Crystals (Japan, 92/01/13)", 0 )
GAME( 1992, blazeon,  0,        blazeon,  blazeon,  kaneko16,   ROT0,  "Atlus",  "Blaze On (Japan)", 0 )
GAME( 1992, explbrkr, 0,        bakubrkr, bakubrkr, kaneko16,   ROT90, "Kaneko", "Explosive Breaker", 0 )
GAME( 1992, bakubrkr, explbrkr, bakubrkr, bakubrkr, kaneko16,   ROT90, "Kaneko", "Bakuretsu Breaker", 0 )
GAME( 1992, sandscrp, 0,        sandscrp, sandscrp, 0,          ROT90, "Face",   "Sand Scorpion (set 1)", 0 )
GAME( 1992, sandscra, sandscrp, sandscrp, sandscrp, 0,          ROT90, "Face",   "Sand Scorpion (set 2)", 0 )
GAME( 1994, bonkadv,  0,        bonkadv , bonkadv,  samplebank, ROT0,  "Kaneko", "B.C. Kid / Bonk's Adventure / Kyukyoku!! PC Genjin", GAME_IMPERFECT_SOUND )
GAME( 1994, bloodwar, 0,        bloodwar, bloodwar, samplebank, ROT0,  "Kaneko", "Blood Warrior", 0 )
GAME( 1994, gtmr,     0,        gtmr,     gtmr,     samplebank, ROT0,  "Kaneko", "1000 Miglia: Great 1000 Miles Rally (94/07/18)", 0 )
GAME( 1994, gtmra,    gtmr,     gtmr,     gtmr,     samplebank, ROT0,  "Kaneko", "1000 Miglia: Great 1000 Miles Rally (94/06/13)", 0 )
GAME( 1994, gtmre,    gtmr,     gtmr,     gtmr,     kaneko16,   ROT0,  "Kaneko", "Great 1000 Miles Rally: Evolution Model!!! (94/09/06)", 0 )
GAME( 1994, gtmrusa,  gtmr,     gtmr,     gtmr,     kaneko16,   ROT0,  "Kaneko", "Great 1000 Miles Rally: U.S.A Version! (94/09/06)", 0 ) // U.S.A version seems part of the title, rather than region
GAME( 1995, gtmr2,    0,        gtmr2,    gtmr2,    kaneko16,   ROT0,  "Kaneko", "Mille Miglia 2: Great 1000 Miles Rally (95/05/24)", 0 )
GAME( 1995, gtmr2a,   gtmr2,    gtmr2,    gtmr2,    kaneko16,   ROT0,  "Kaneko", "Mille Miglia 2: Great 1000 Miles Rally (95/04/04)", 0 )
GAME( 1995, gtmr2u,   gtmr2,    gtmr2,    gtmr2,    kaneko16,   ROT0,  "Kaneko", "Great 1000 Miles Rally 2 USA (95/05/18)", 0 )
GAME( 1992, brapboys, 0,        brapboys, brapboys, brapboys,   ROT0,  "Kaneko", "B.Rap Boys (World)", 0 )
GAME( 1992, brapboysj,brapboys, brapboys, brapboys, brapboys,   ROT0,  "Kaneko", "B.Rap Boys Special (Japan)", 0 )
GAME( 1992, shogwarr, 0,        shogwarr, shogwarr, shogwarr,   ROT0,  "Kaneko", "Shogun Warriors (US)", 0 )
GAME( 1992, shogwarre,shogwarr, shogwarr, shogwarr, shogwarr,   ROT0,  "Kaneko", "Shogun Warriors (World)", 0 )
GAME( 1992, fjbuster, shogwarr, shogwarr, shogwarr, shogwarr,   ROT0,  "Kaneko", "Fujiyama Buster (Japan)", 0 )
	

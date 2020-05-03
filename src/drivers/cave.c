/***************************************************************************

                              -= Cave Hardware =-

                    driver by   Luca Elia (l.elia@tin.it)


Main  CPU    :  MC68000

Sound CPU    :  Z80 [Optional]

Sound Chips  :  YMZ280B or
                OKIM6295 x (1|2) + YM2203 / YM2151 [Optional]

Other        :  93C46 EEPROM


-----------------------------------------------------------------------------------
Year + Game         License     PCB         Tilemaps        Sprites         Other
-----------------------------------------------------------------------------------
94  Mazinger Z      Banpresto   ?           038 9335EX706   013 9341E7009   Z80
94  PowerInstinct 2 Atlus       ATG02?      038 9429WX709?  013             Z80 NMK 112
95  P.I. Legends    Atlus       AT047G2-B   038 9429WX709   013 9341E7009   Z80 NMK 112
95  Metamoqester    Banpresto   BP947A      038 9437WX711   013 9346E7002   Z80
95  Sailor Moon     Banpresto   BP945A      038 9437WX711   013 9346E7002   Z80
95  Donpachi        Atlus       AT-C01DP-2  038 9429WX727   013 8647-01     NMK 112
96  Air Gallet      Banpresto   BP962A      038 9437WX711   013 9346E7002   Z80
96  Hotdog Storm    Marble      ?           ?                               Z80
97  Dodonpachi      Atlus       ATC03D2     ?
98  Dangun Feveron  Nihon Sys.  CV01        038 9808WX003   013 9807EX004
98  ESP Ra.De.      Atlus       ATC04       ?
98  Uo Poko         Jaleco      CV02        038 9749WX001   013 9749EX004
99  Guwange         Atlus       ATC05       ?
99  Gaia Crusaders  Noise Factory ?         038 9838WX003   013 9918EX008
99  Koro Koro Quest Takumi      TUG-01B     038 9838WX004   013 9838EX004
-----------------------------------------------------------------------------------

To Do:

- Sprite lag in some games (e.g. metmqstr). The sprites chip probably
  generates interrupts (unknown_irq)


***************************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "machine/nmk112.h"
#include "cpu/z80/z80.h"
#include "cave.h"
#include "sound/2203intf.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"
#include "sound/ymz280b.h"

/***************************************************************************


                        Interrupt Handling Routines


***************************************************************************/

static int time_vblank_irq = 2000;
static UINT8 irq_level;
static UINT8 vblank_irq;
static UINT8 sound_irq;
static UINT8 unknown_irq;
static UINT8 agallet_vblank_irq;

/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	if (vblank_irq || sound_irq || unknown_irq)
		cpunum_set_input_line(0, irq_level, ASSERT_LINE);
	else
		cpunum_set_input_line(0, irq_level, CLEAR_LINE);
}

static void cave_vblank_start(int param)
{
	vblank_irq = 1;
	update_irq_state();
	cave_get_sprite_info();
	agallet_vblank_irq = 1;
}

static void cave_vblank_end(int param)
{
	if(cave_kludge == 3)	/* mazinger metmqstr */
	{
		unknown_irq = 1;
		update_irq_state();
	}
	agallet_vblank_irq = 0;
}

/* Called once/frame to generate the VBLANK interrupt */
static INTERRUPT_GEN( cave_interrupt )
{
	timer_set(TIME_IN_USEC(17376-time_vblank_irq), 0, cave_vblank_start);
	timer_set(TIME_IN_USEC(17376-time_vblank_irq + 2000), 0, cave_vblank_end);
}

/* Called by the YMZ280B to set the IRQ state */
static void sound_irq_gen(int state)
{
	sound_irq = (state != 0);
	update_irq_state();
}


/*  Level 1 irq routines:

    Game        |first read | bit==0->routine + |
                |offset:    | read this offset  |

    ddonpach    4,0         0 -> vblank + 4     1 -> rte    2 -> like 0     read sound
    dfeveron    0           0 -> vblank + 4     1 -> + 6    -               read sound
    uopoko      0           0 -> vblank + 4     1 -> + 6    -               read sound
    esprade     0           0 -> vblank + 4     1 -> rte    2 must be 0     read sound
    guwange     0           0 -> vblank + 6,4   1 -> + 6,4  2 must be 0     read sound
    mazinger    0           0 -> vblank + 4     rest -> scroll + 6
*/


/* Reads the cause of the interrupt and clears the state */

static READ16_HANDLER( cave_irq_cause_r )
{
	int result = 0x0003;

	if (vblank_irq)		result ^= 0x01;
	if (unknown_irq)	result ^= 0x02;

	if (offset == 4/2)	vblank_irq = 0;
	if (offset == 6/2)	unknown_irq = 0;

	update_irq_state();

/*
    sailormn and agallet wait for bit 2 of $b80001 to go 1 -> 0.
    It must happen once per frame as agallet uses this to show
    the copyright notice screen for ~8.5s
*/
	if (offset == 0)
	{
		result &= ~4;
		result |= (agallet_vblank_irq?0:4);
	}

	return result;
}


/***************************************************************************


                            Sound Handling Routines


***************************************************************************/

/*  We need a FIFO buffer for sailormn, where the inter-CPUs
    communication is *really* tight */
static struct
{
	int len;
	UINT8 data[32];
}	soundbuf;

//static UINT8 sound_flag1, sound_flag2;

static READ8_HANDLER( soundflags_r )
{
	// bit 2 is low: can read command (lo)
	// bit 3 is low: can read command (hi)
//  return  (sound_flag1 ? 0 : 4) |
//          (sound_flag2 ? 0 : 8) ;
return 0;
}

static READ16_HANDLER( soundflags_ack_r )
{
	// bit 0 is low: can write command
	// bit 1 is low: can read answer
//  return  ((sound_flag1 | sound_flag2) ? 1 : 0) |
//          ((soundbuf.len>0        ) ? 0 : 2) ;

return		((soundbuf.len>0        ) ? 0 : 2) ;
}

/* Main CPU: write a 16 bit sound latch and generate a NMI on the sound CPU */
static WRITE16_HANDLER( sound_cmd_w )
{
//  sound_flag1 = 1;
//  sound_flag2 = 1;
	soundlatch_word_w(offset,data,mem_mask);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	cpu_spinuntil_time(TIME_IN_USEC(50));	// Allow the other cpu to reply
}

/* Sound CPU: read the low 8 bits of the 16 bit sound latch */
static READ8_HANDLER( soundlatch_lo_r )
{
//  sound_flag1 = 0;
	return soundlatch_word_r(offset,0) & 0xff;
}

/* Sound CPU: read the high 8 bits of the 16 bit sound latch */
static READ8_HANDLER( soundlatch_hi_r )
{
//  sound_flag2 = 0;
	return soundlatch_word_r(offset,0) >> 8;
}

/* Main CPU: read the latch written by the sound CPU (acknowledge) */
static READ16_HANDLER( soundlatch_ack_r )
{
	if (soundbuf.len>0)
	{
		UINT8 data = soundbuf.data[0];
		memmove(soundbuf.data,soundbuf.data+1,(32-1)*sizeof(soundbuf.data[0]));
		soundbuf.len--;
		return data;
	}
	else
	{	logerror("CPU #1 - PC %04X: Sound Buffer 2 Underflow Error\n",activecpu_get_pc());
		return 0xff;	}
}


/* Sound CPU: write latch for the main CPU (acknowledge) */
static WRITE8_HANDLER( soundlatch_ack_w )
{
	soundbuf.data[soundbuf.len] = data;
	if (soundbuf.len<32)
		soundbuf.len++;
	else
		logerror("CPU #1 - PC %04X: Sound Buffer 2 Overflow Error\n",activecpu_get_pc());
}



/* Handles writes to the YMZ280B */
static WRITE16_HANDLER( cave_sound_w )
{
	if (ACCESSING_LSB)
	{
		if (offset)	YMZ280B_data_0_w     (offset, data & 0xff);
		else		YMZ280B_register_0_w (offset, data & 0xff);
	}
}

/* Handles reads from the YMZ280B */
static READ16_HANDLER( cave_sound_r )
{
	return YMZ280B_status_0_r(offset);
}


/***************************************************************************


                                    EEPROM


***************************************************************************/

static UINT8 cave_default_eeprom_type1[16] =	{0x00,0x0C,0x11,0x0D,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x11,0x11,0xFF,0xFF,0xFF,0xFF};  /* DFeveron, Guwange */
static UINT8 cave_default_eeprom_type1feversos[18] =	{0x00,0x0C,0x16,0x27,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x11,0x11,0xFF,0xFF,0xFF,0xFF,0x05,0x19};  /* Fever SOS (code checks for the 0x0519 or it won't boot) */
static UINT8 cave_default_eeprom_type2[16] =	{0x00,0x0C,0xFF,0xFB,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};  /* Esprade, DonPachi, DDonPachi */
static UINT8 cave_default_eeprom_type3[16] =	{0x00,0x03,0x08,0x00,0xFF,0xFF,0xFF,0xFF,0x08,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF};  /* UoPoko */
static UINT8 cave_default_eeprom_type4[16] =	{0xF3,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};  /* Hotdog Storm */
static UINT8 cave_default_eeprom_type5[16] =	{0xED,0xFF,0x00,0x00,0x12,0x31,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};  /* Mazinger Z (6th byte is country code) */
static UINT8 cave_default_eeprom_type6[18] =	{0xa5,0x00,0xa5,0x00,0xa5,0x00,0xa5,0x00,0xa5,0x01,0xa5,0x01,0xa5,0x04,0xa5,0x01,0xa5,0x02};	/* Sailor Moon (last byte is country code) */
// Air Gallet. Byte 1f is the country code (0==JAPAN,U.S.A,EUROPE,HONGKONG,TAIWAN,KOREA)
static UINT8 cave_default_eeprom_type7[48] =	{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
												 0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
												 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff};

static UINT8 *cave_default_eeprom;
static int cave_default_eeprom_length;
static int cave_region_byte;

READ16_HANDLER( cave_input1_r )
{
	return readinputport(1) | ((EEPROM_read_bit() & 0x01) << 11);
}

READ16_HANDLER( guwange_input1_r )
{
	return readinputport(1) | ((EEPROM_read_bit() & 0x01) << 7);
}

WRITE16_HANDLER( cave_eeprom_msb_w )
{
	if (data & ~0xfe00)
		logerror("CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_MSB )  // even address
	{
		coin_lockout_w(1,~data & 0x8000);
		coin_lockout_w(0,~data & 0x4000);
		coin_counter_w(1, data & 0x2000);
		coin_counter_w(0, data & 0x1000);

		// latch the bit
		EEPROM_write_bit(data & 0x0800);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0400) ? ASSERT_LINE : CLEAR_LINE );
	}
}

WRITE16_HANDLER( sailormn_eeprom_msb_w )
{
	sailormn_tilebank_w    ( data &  0x0100 );
	cave_eeprom_msb_w(offset,data & ~0x0100,mem_mask);
}

WRITE16_HANDLER( hotdogst_eeprom_msb_w )
{
	if ( ACCESSING_MSB )  // even address
	{
		// latch the bit
		EEPROM_write_bit(data & 0x0800);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x0400) ? CLEAR_LINE: ASSERT_LINE );
	}
}

WRITE16_HANDLER( cave_eeprom_lsb_w )
{
	if (data & ~0x00ef)
		logerror("CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_LSB )  // odd address
	{
		coin_lockout_w(1,~data & 0x0008);
		coin_lockout_w(0,~data & 0x0004);
		coin_counter_w(1, data & 0x0002);
		coin_counter_w(0, data & 0x0001);

		// latch the bit
		EEPROM_write_bit(data & 0x80);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE );
	}
}

/*  - No eeprom or lockouts */
WRITE16_HANDLER( gaia_coin_lsb_w )
{
	if ( ACCESSING_LSB )  // odd address
	{
		coin_counter_w(1, data & 0x0002);
		coin_counter_w(0, data & 0x0001);
	}
}

/*  - No coin lockouts
    - Writing 0xcf00 shouldn't send a 1 bit to the eeprom   */
WRITE16_HANDLER( metmqstr_eeprom_msb_w )
{
	if (data & ~0xff00)
		logerror("CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_MSB )  // even address
	{
		coin_counter_w(1, data & 0x2000);
		coin_counter_w(0, data & 0x1000);

		if (~data & 0x0100)
		{
			// latch the bit
			EEPROM_write_bit(data & 0x0800);

			// reset line asserted: reset.
			EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );

			// clock line asserted: write latch or select next bit to read
			EEPROM_set_clock_line((data & 0x0400) ? ASSERT_LINE : CLEAR_LINE );
		}
	}
}

NVRAM_HANDLER( cave )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file) EEPROM_load(file);
		else
		{
			if (cave_default_eeprom)	/* Set the EEPROM to Factory Defaults */
				EEPROM_set_data(cave_default_eeprom,cave_default_eeprom_length);
		}
	}
}

static struct EEPROM_interface eeprom_interface_93C46_8bit =
{
	7,				// address bits 7
	8,				// data bits    8
	"*110",			// read         1 10 aaaaaa
	"*101",			// write        1 01 aaaaaa dddddddddddddddd
	"*111",			// erase        1 11 aaaaaa
	"*10000xxxx",	// lock         1 00 00xxxx
	"*10011xxxx",	// unlock       1 00 11xxxx
	1,
//  "*10001xxxx"    // write all    1 00 01xxxx dddddddddddddddd
//  "*10010xxxx"    // erase all    1 00 10xxxx
};

NVRAM_HANDLER( korokoro )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46_8bit);

		if (file) EEPROM_load(file);
		else
		{
			if (cave_default_eeprom)	/* Set the EEPROM to Factory Defaults */
				EEPROM_set_data(cave_default_eeprom,cave_default_eeprom_length);
		}
	}
}



/***************************************************************************


                            Memory Maps - Main CPU


***************************************************************************/

/*  Lines starting with an empty comment in the following MemoryReadAddress
     arrays are there for debug (e.g. the game does not read from those ranges
    AFAIK)  */

/***************************************************************************
                                Dangun Feveron
***************************************************************************/

static ADDRESS_MAP_START( dfeveron_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM				)	// RAM
	AM_RANGE(0x300002, 0x300003) AM_READ(cave_sound_r			)	// YMZ280
/**/AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM				)	// Sprites
/**/AM_RANGE(0x408000, 0x40ffff) AM_READ(MRA16_RAM				)	// Sprites?
/**/AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM				)	// Layer 0
/**/AM_RANGE(0x600000, 0x607fff) AM_READ(MRA16_RAM				)	// Layer 1
/**/AM_RANGE(0x708000, 0x708fff) AM_READ(MRA16_RAM				)	// Palette
/**/AM_RANGE(0x710000, 0x710fff) AM_READ(MRA16_RAM				)	// ?
	AM_RANGE(0x800000, 0x800007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
/**/AM_RANGE(0x900000, 0x900005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xa00000, 0xa00005) AM_READ(MRA16_RAM				)	// Layer 1 Control
	AM_RANGE(0xb00000, 0xb00001) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xb00002, 0xb00003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( dfeveron_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x300000, 0x300003) AM_WRITE(cave_sound_w					)	// YMZ280
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x408000, 0x40ffff) AM_WRITE(MWA16_RAM						)	// Sprites?
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x600000, 0x607fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x708000, 0x708fff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x710c00, 0x710fff) AM_WRITE(MWA16_RAM						)	// ?
	AM_RANGE(0x800000, 0x80007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs	)	// Video Regs
	AM_RANGE(0x900000, 0x900005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0		)	// Layer 0 Control
	AM_RANGE(0xa00000, 0xa00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1		)	// Layer 1 Control
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(cave_eeprom_msb_w				)	// EEPROM
ADDRESS_MAP_END


/***************************************************************************
                                Dodonpachi
***************************************************************************/

static ADDRESS_MAP_START( ddonpach_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM				)	// RAM
	AM_RANGE(0x300002, 0x300003) AM_READ(cave_sound_r			)	// YMZ280
/**/AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM				)	// Sprites
/**/AM_RANGE(0x408000, 0x40ffff) AM_READ(MRA16_RAM				)	// Sprites?
/**/AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM				)	// Layer 0
/**/AM_RANGE(0x600000, 0x607fff) AM_READ(MRA16_RAM				)	// Layer 1
/**/AM_RANGE(0x700000, 0x70ffff) AM_READ(MRA16_RAM				)	// Layer 2
	AM_RANGE(0x800000, 0x800007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
/**/AM_RANGE(0x900000, 0x900005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xa00000, 0xa00005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM				)	// Layer 2 Control
/**/AM_RANGE(0xc00000, 0xc0ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0xd00000, 0xd00001) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xd00002, 0xd00003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ddonpach_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM							)	// RAM
	AM_RANGE(0x300000, 0x300003) AM_WRITE(cave_sound_w						)	// YMZ280
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x408000, 0x40ffff) AM_WRITE(MWA16_RAM							)	// Sprites?
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x600000, 0x607fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x700000, 0x70ffff) AM_WRITE(cave_vram_2_8x8_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0x800000, 0x80007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs		)	// Video Regs
	AM_RANGE(0x900000, 0x900005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0			)	// Layer 0 Control
	AM_RANGE(0xa00000, 0xa00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1			)	// Layer 1 Control
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2			)	// Layer 2 Control
	AM_RANGE(0xc00000, 0xc0ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(cave_eeprom_msb_w					)	// EEPROM
ADDRESS_MAP_END


/***************************************************************************
                                    Donpachi
***************************************************************************/

READ16_HANDLER( donpachi_videoregs_r )
{
	switch( offset )
	{
		case 0:
		case 1:
		case 2:
		case 3:	return cave_irq_cause_r(offset,0);

		default:	return 0x0000;
	}
}

#if 0
WRITE16_HANDLER( donpachi_videoregs_w )
{
	COMBINE_DATA(&cave_videoregs[offset]);

	switch( offset )
	{
//      case 0x78/2:    watchdog_reset16_w(0,0);    break;
	}
}
#endif

static ADDRESS_MAP_START( donpachi_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x200000, 0x207fff) AM_READ(MRA16_RAM					)	// Layer 1
	AM_RANGE(0x300000, 0x307fff) AM_READ(MRA16_RAM					)	// Layer 0
	AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM					)	// Layer 2
	AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0x508000, 0x50ffff) AM_READ(MRA16_RAM					)	// Sprites?
/**/AM_RANGE(0x600000, 0x600005) AM_READ(MRA16_RAM					)	// Layer 0 Control
/**/AM_RANGE(0x700000, 0x700005) AM_READ(MRA16_RAM					)	// Layer 1 Control
/**/AM_RANGE(0x800000, 0x800005) AM_READ(MRA16_RAM					)	// Layer 2 Control
	AM_RANGE(0x900000, 0x90007f) AM_READ(donpachi_videoregs_r		)	// Video Regs
/**/AM_RANGE(0xa08000, 0xa08fff) AM_READ(MRA16_RAM					)	// Palette
	AM_RANGE(0xb00000, 0xb00001) AM_READ(OKIM6295_status_0_lsb_r	)	// M6295
	AM_RANGE(0xb00010, 0xb00011) AM_READ(OKIM6295_status_1_lsb_r	)	//
	AM_RANGE(0xc00000, 0xc00001) AM_READ(input_port_0_word_r		)	// Inputs
	AM_RANGE(0xc00002, 0xc00003) AM_READ(cave_input1_r				)	// Inputs + EEPROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( donpachi_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM							)	// RAM
	AM_RANGE(0x200000, 0x207fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x300000, 0x307fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(cave_vram_2_8x8_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x508000, 0x50ffff) AM_WRITE(MWA16_RAM							)	// Sprites?
	AM_RANGE(0x600000, 0x600005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1			)	// Layer 1 Control
	AM_RANGE(0x700000, 0x700005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0			)	// Layer 0 Control
	AM_RANGE(0x800000, 0x800005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2			)	// Layer 2 Control
	AM_RANGE(0x900000, 0x90007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs		)	// Video Regs
	AM_RANGE(0xa08000, 0xa08fff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xb00000, 0xb00003) AM_WRITE(OKIM6295_data_0_lsb_w				)	// M6295
	AM_RANGE(0xb00010, 0xb00013) AM_WRITE(OKIM6295_data_1_lsb_w				)	//
	AM_RANGE(0xb00020, 0xb0002f) AM_WRITE(NMK112_okibank_lsb_w				)	//
	AM_RANGE(0xd00000, 0xd00001) AM_WRITE(cave_eeprom_msb_w					)	// EEPROM
ADDRESS_MAP_END


/***************************************************************************
                                    Esprade
***************************************************************************/

static ADDRESS_MAP_START( esprade_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM				)	// RAM
	AM_RANGE(0x300002, 0x300003) AM_READ(cave_sound_r			)	// YMZ280
/**/AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM				)	// Sprites
/**/AM_RANGE(0x408000, 0x40ffff) AM_READ(MRA16_RAM				)	// Sprites?
/**/AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM				)	// Layer 0
/**/AM_RANGE(0x600000, 0x607fff) AM_READ(MRA16_RAM				)	// Layer 1
/**/AM_RANGE(0x700000, 0x707fff) AM_READ(MRA16_RAM				)	// Layer 2
	AM_RANGE(0x800000, 0x800007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
/**/AM_RANGE(0x900000, 0x900005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xa00000, 0xa00005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM				)	// Layer 2 Control
/**/AM_RANGE(0xc00000, 0xc0ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0xd00000, 0xd00001) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xd00002, 0xd00003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( esprade_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x300000, 0x300003) AM_WRITE(cave_sound_w					)	// YMZ280
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x408000, 0x40ffff) AM_WRITE(MWA16_RAM						)	// Sprites?
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x600000, 0x607fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x700000, 0x707fff) AM_WRITE(cave_vram_2_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0x800000, 0x80007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs	)	// Video Regs
	AM_RANGE(0x900000, 0x900005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0		)	// Layer 0 Control
	AM_RANGE(0xa00000, 0xa00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1		)	// Layer 1 Control
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2		)	// Layer 2 Control
	AM_RANGE(0xc00000, 0xc0ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(cave_eeprom_msb_w				)	// EEPROM
ADDRESS_MAP_END


/***************************************************************************
                                    Gaia Crusaders
***************************************************************************/

static ADDRESS_MAP_START( gaia_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM				)	// RAM
	AM_RANGE(0x300002, 0x300003) AM_READ(cave_sound_r			)	// YMZ280
	AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM				)	// Sprite bank 1
	AM_RANGE(0x408000, 0x40ffff) AM_READ(MRA16_RAM				)	// Sprite bank 2
	AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM				)	// Layer 0
	AM_RANGE(0x508000, 0x50ffff) AM_READ(MRA16_RAM				)	// More Layer 0, Tested but not used?
	AM_RANGE(0x600000, 0x607fff) AM_READ(MRA16_RAM				)	// Layer 1
	AM_RANGE(0x608000, 0x60ffff) AM_READ(MRA16_RAM				)	// More Layer 1, Tested but not used?
	AM_RANGE(0x700000, 0x707fff) AM_READ(MRA16_RAM				)	// Layer 2
	AM_RANGE(0x708000, 0x70ffff) AM_READ(MRA16_RAM				)	// More Layer 2, Tested but not used?
	AM_RANGE(0x800000, 0x800007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
/**/AM_RANGE(0x900000, 0x900005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xa00000, 0xa00005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM				)	// Layer 2 Control
	AM_RANGE(0xc00000, 0xc0ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0xd00010, 0xd00011) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xd00012, 0xd00013) AM_READ(input_port_1_word_r	)	// Inputs
	AM_RANGE(0xd00014, 0xd00015) AM_READ(input_port_2_word_r	)	// DIPS
ADDRESS_MAP_END

static ADDRESS_MAP_START( gaia_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x300000, 0x300003) AM_WRITE(cave_sound_w					)	// YMZ280
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprite bank 1
	AM_RANGE(0x408000, 0x40ffff) AM_WRITE(MWA16_RAM						)	// Sprite bank 2
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x508000, 0x50ffff) AM_WRITE(MWA16_RAM						)	// More Layer 0, Tested but not used?
	AM_RANGE(0x600000, 0x607fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x608000, 0x60ffff) AM_WRITE(MWA16_RAM						)	// More Layer 1, Tested but not used?
	AM_RANGE(0x700000, 0x707fff) AM_WRITE(cave_vram_2_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0x708000, 0x70ffff) AM_WRITE(MWA16_RAM						)	// More Layer 2, Tested but not used?
	AM_RANGE(0x800000, 0x80007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs	)	// Video Regs
	AM_RANGE(0x900000, 0x900005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0		)	// Layer 0 Control
	AM_RANGE(0xa00000, 0xa00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1		)	// Layer 1 Control
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2		)	// Layer 2 Control
	AM_RANGE(0xc00000, 0xc0ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xd00010, 0xd00011) AM_WRITE(gaia_coin_lsb_w				)	// Coin counter only
	AM_RANGE(0xd00014, 0xd00015) AM_WRITE(watchdog_reset16_w			)	// Watchdog?
ADDRESS_MAP_END


/***************************************************************************
                                    Guwange
***************************************************************************/

static ADDRESS_MAP_START( guwange_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM				)	// RAM
	AM_RANGE(0x300000, 0x300007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
/**/AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM				)	// Sprites
/**/AM_RANGE(0x408000, 0x40ffff) AM_READ(MRA16_RAM				)	// Sprites?
/**/AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM				)	// Layer 0
/**/AM_RANGE(0x600000, 0x607fff) AM_READ(MRA16_RAM				)	// Layer 1
/**/AM_RANGE(0x700000, 0x707fff) AM_READ(MRA16_RAM				)	// Layer 2
	AM_RANGE(0x800002, 0x800003) AM_READ(cave_sound_r			)	// YMZ280
/**/AM_RANGE(0x900000, 0x900005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xa00000, 0xa00005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM				)	// Layer 2 Control
/**/AM_RANGE(0xc00000, 0xc0ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0xd00010, 0xd00011) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xd00012, 0xd00013) AM_READ(guwange_input1_r		)	// Inputs + EEPROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( guwange_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x300000, 0x30007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs	)	// Video Regs
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x408000, 0x40ffff) AM_WRITE(MWA16_RAM						)	// Sprites?
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x600000, 0x607fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x700000, 0x707fff) AM_WRITE(cave_vram_2_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0x800000, 0x800003) AM_WRITE(cave_sound_w					)	// YMZ280
	AM_RANGE(0x900000, 0x900005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0		)	// Layer 0 Control
	AM_RANGE(0xa00000, 0xa00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1		)	// Layer 1 Control
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2		)	// Layer 2 Control
	AM_RANGE(0xc00000, 0xc0ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xd00010, 0xd00011) AM_WRITE(cave_eeprom_lsb_w				)	// EEPROM
//  AM_RANGE(0xd00012, 0xd00013) AM_WRITE(MWA16_NOP             )   // ?
//  AM_RANGE(0xd00014, 0xd00015) AM_WRITE(MWA16_NOP             )   // ? $800068 in dfeveron ? probably Watchdog
ADDRESS_MAP_END


/***************************************************************************
                                Hotdog Storm
***************************************************************************/

static ADDRESS_MAP_START( hotdogst_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x300000, 0x30ffff) AM_READ(MRA16_RAM				)	// RAM
/**/AM_RANGE(0x408000, 0x408fff) AM_READ(MRA16_RAM				)	// Palette
/**/AM_RANGE(0x880000, 0x887fff) AM_READ(MRA16_RAM				)	// Layer 0
/**/AM_RANGE(0x900000, 0x907fff) AM_READ(MRA16_RAM				)	// Layer 1
/**/AM_RANGE(0x980000, 0x987fff) AM_READ(MRA16_RAM				)	// Layer 2
	AM_RANGE(0xa80000, 0xa80007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
//  AM_RANGE(0xa8006e, 0xa8006f) AM_READ(soundlatch_ack_r       )   // From Sound CPU
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xb80000, 0xb80005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0xc00000, 0xc00005) AM_READ(MRA16_RAM				)	// Layer 2 Control
	AM_RANGE(0xc80000, 0xc80001) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xc80002, 0xc80003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
/**/AM_RANGE(0xf00000, 0xf07fff) AM_READ(MRA16_RAM				)	// Sprites
/**/AM_RANGE(0xf08000, 0xf0ffff) AM_READ(MRA16_RAM				)	// Sprites?
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotdogst_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x300000, 0x30ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x408000, 0x408fff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x880000, 0x887fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x900000, 0x907fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x980000, 0x987fff) AM_WRITE(cave_vram_2_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0xa8006e, 0xa8006f) AM_WRITE(sound_cmd_w					)	// To Sound CPU
	AM_RANGE(0xa80000, 0xa8007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs	)	// Video Regs
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0		)	// Layer 0 Control
	AM_RANGE(0xb80000, 0xb80005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1		)	// Layer 1 Control
	AM_RANGE(0xc00000, 0xc00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2		)	// Layer 2 Control
	AM_RANGE(0xd00000, 0xd00001) AM_WRITE(hotdogst_eeprom_msb_w			)	// EEPROM
	AM_RANGE(0xd00002, 0xd00003) AM_WRITE(MWA16_NOP						)	// ???
	AM_RANGE(0xf00000, 0xf07fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xf08000, 0xf0ffff) AM_WRITE(MWA16_RAM						)	// Sprites?
ADDRESS_MAP_END


/***************************************************************************
                               Koro Koro Quest
***************************************************************************/

static UINT16 leds[2];

static void show_leds(void)
{
#ifdef MAME_DEBUG
//  ui_popup("led %04X eep %02X",leds[0],(leds[1] >> 8) & ~0x70);
#endif
}

WRITE16_HANDLER( korokoro_leds_w )
{
	COMBINE_DATA( &leds[0] );

	set_led_status(0, data & 0x8000);
	set_led_status(1, data & 0x4000);
	set_led_status(2, data & 0x1000);	// square button
	set_led_status(3, data & 0x0800);	// round  button
//  coin_lockout_w(1,~data & 0x0200);   // coin lockouts?
//  coin_lockout_w(0,~data & 0x0100);

//  coin_counter_w(2, data & 0x0080);
//  coin_counter_w(1, data & 0x0020);
	coin_counter_w(0, data & 0x0010);

	set_led_status(5, data & 0x0008);
	set_led_status(6, data & 0x0004);
	set_led_status(7, data & 0x0002);
	set_led_status(8, data & 0x0001);

	show_leds();
}

static int hopper;

WRITE16_HANDLER( korokoro_eeprom_msb_w )
{
	if (data & ~0x7000)
	{
		logerror("CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);
		COMBINE_DATA( &leds[1] );
		show_leds();
	}

	if ( ACCESSING_MSB )  // even address
	{
		hopper = data & 0x0100;	// ???

		// latch the bit
		EEPROM_write_bit(data & 0x4000);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x1000) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x2000) ? ASSERT_LINE : CLEAR_LINE );
	}
}

READ16_HANDLER( korokoro_input0_r )
{
	return readinputport(0) | (hopper ? 0 : 0x8000);
}

READ16_HANDLER( korokoro_input1_r )
{
	return readinputport(1) | ((EEPROM_read_bit() & 0x01) << 12);
}

static ADDRESS_MAP_START( korokoro_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ( MRA16_ROM				)	// ROM
//  AM_RANGE(0x100000, 0x107fff) AM_READ( MRA16_RAM             )   // Layer 0
//  AM_RANGE(0x140000, 0x140005) AM_READ( MRA16_RAM             )   // Layer 0 Control
//  AM_RANGE(0x180000, 0x187fff) AM_READ( MRA16_RAM             )   // Sprites
	AM_RANGE(0x1c0000, 0x1c0007) AM_READ( cave_irq_cause_r		)	// IRQ Cause
//  AM_RANGE(0x200000, 0x207fff) AM_READ( MRA16_RAM             )   // Palette
//  AM_RANGE(0x240000, 0x240003) AM_READ( cave_sound_r          )   // YMZ280
	AM_RANGE(0x280000, 0x280001) AM_READ( korokoro_input0_r		)	// Inputs + ???
	AM_RANGE(0x280002, 0x280003) AM_READ( korokoro_input1_r		)	// Inputs + EEPROM
	AM_RANGE(0x300000, 0x30ffff) AM_READ( MRA16_RAM				)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( korokoro_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE( MWA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x107fff) AM_WRITE( cave_vram_0_w			) AM_BASE( &cave_vram_0		)	// Layer 0
	AM_RANGE(0x140000, 0x140005) AM_WRITE( MWA16_RAM				) AM_BASE( &cave_vctrl_0	)	// Layer 0 Control
	AM_RANGE(0x180000, 0x187fff) AM_WRITE( MWA16_RAM				) AM_BASE( &spriteram16		) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x1c0000, 0x1c007f) AM_WRITE( MWA16_RAM				) AM_BASE( &cave_videoregs	)	// Video Regs
	AM_RANGE(0x200000, 0x207fff) AM_WRITE( paletteram16_xGGGGGRRRRRBBBBB_word_w	) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x240000, 0x240003) AM_WRITE( cave_sound_w				)	// YMZ280
	AM_RANGE(0x280008, 0x280009) AM_WRITE( korokoro_leds_w			)
	AM_RANGE(0x28000a, 0x28000b) AM_WRITE( korokoro_eeprom_msb_w	)	// EEPROM
	AM_RANGE(0x28000c, 0x28000d) AM_WRITE( MWA16_NOP				)	// 0 (watchdog?)
	AM_RANGE(0x300000, 0x30ffff) AM_WRITE( MWA16_RAM				)	// RAM
ADDRESS_MAP_END


/***************************************************************************
                                Mazinger Z
***************************************************************************/

static ADDRESS_MAP_START( mazinger_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM				)	// RAM
/**/AM_RANGE(0x200000, 0x207fff) AM_READ(MRA16_RAM				)	// Sprites
/**/AM_RANGE(0x208000, 0x20ffff) AM_READ(MRA16_RAM				)	// Sprites?
	AM_RANGE(0x300000, 0x300007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
	AM_RANGE(0x30006e, 0x30006f) AM_READ(soundlatch_ack_r		)	// From Sound CPU
/**/AM_RANGE(0x404000, 0x407fff) AM_READ(MRA16_RAM				)	// Layer 1
/**/AM_RANGE(0x504000, 0x507fff) AM_READ(MRA16_RAM				)	// Layer 0
/**/AM_RANGE(0x600000, 0x600005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0x700000, 0x700005) AM_READ(MRA16_RAM				)	// Layer 0 Control
	AM_RANGE(0x800000, 0x800001) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0x800002, 0x800003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
/**/AM_RANGE(0xc08000, 0xc0ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0xd00000, 0xd7ffff) AM_READ(MRA16_BANK1			)	// ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mazinger_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM							)	// RAM
	AM_RANGE(0x200000, 0x207fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x208000, 0x20ffff) AM_WRITE(MWA16_RAM							)	// Sprites?
	AM_RANGE(0x300068, 0x300069) AM_WRITE(watchdog_reset16_w				)	// Watchdog
	AM_RANGE(0x30006e, 0x30006f) AM_WRITE(sound_cmd_w						)	// To Sound CPU
	AM_RANGE(0x300000, 0x30007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs		)	// Video Regs
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(cave_vram_1_8x8_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(cave_vram_0_8x8_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x600000, 0x600005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1			)	// Layer 1 Control
	AM_RANGE(0x700000, 0x700005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0			)	// Layer 0 Control
	AM_RANGE(0x900000, 0x900001) AM_WRITE(cave_eeprom_msb_w					)	// EEPROM
	AM_RANGE(0xc08000, 0xc0ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xd00000, 0xd7ffff) AM_WRITE(MWA16_ROM							)	// ROM
ADDRESS_MAP_END


/***************************************************************************
                                Metamoqester
***************************************************************************/

static ADDRESS_MAP_START( metmqstr_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x17ffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x200000, 0x27ffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x408000, 0x408fff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0x600000, 0x600001) AM_READ(watchdog_reset16_r	)	// Watchdog?
	AM_RANGE(0x880000, 0x887fff) AM_READ(MRA16_RAM				)	// Layer 2
	AM_RANGE(0x888000, 0x88ffff) AM_READ(MRA16_RAM				)	//
	AM_RANGE(0x900000, 0x907fff) AM_READ(MRA16_RAM				)	// Layer 1
	AM_RANGE(0x908000, 0x90ffff) AM_READ(MRA16_RAM				)	//
	AM_RANGE(0x980000, 0x987fff) AM_READ(MRA16_RAM				)	// Layer 0
	AM_RANGE(0x988000, 0x98ffff) AM_READ(MRA16_RAM				)	//
	AM_RANGE(0xa80000, 0xa80007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
	AM_RANGE(0xa8006c, 0xa8006d) AM_READ(soundflags_ack_r		)	// Communication
	AM_RANGE(0xa8006e, 0xa8006f) AM_READ(soundlatch_ack_r		)	// From Sound CPU
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xb80000, 0xb80005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0xc00000, 0xc00005) AM_READ(MRA16_RAM				)	// Layer 2 Control
	AM_RANGE(0xc80000, 0xc80001) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xc80002, 0xc80003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
	AM_RANGE(0xf00000, 0xf07fff) AM_READ(MRA16_RAM				)	// Sprites
	AM_RANGE(0xf08000, 0xf0ffff) AM_READ(MRA16_RAM				)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( metmqstr_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x100000, 0x17ffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x200000, 0x27ffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x408000, 0x408fff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x880000, 0x887fff) AM_WRITE(cave_vram_2_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0x888000, 0x88ffff) AM_WRITE(MWA16_RAM						)	//
	AM_RANGE(0x900000, 0x907fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x908000, 0x90ffff) AM_WRITE(MWA16_RAM						)	//
	AM_RANGE(0x980000, 0x987fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x988000, 0x98ffff) AM_WRITE(MWA16_RAM						)	//
	AM_RANGE(0xa80068, 0xa80069) AM_WRITE(watchdog_reset16_w			)	// Watchdog?
	AM_RANGE(0xa8006c, 0xa8006d) AM_WRITE(MWA16_NOP						)	// ?
	AM_RANGE(0xa8006e, 0xa8006f) AM_WRITE(sound_cmd_w					)	// To Sound CPU
	AM_RANGE(0xa80000, 0xa8007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs	)	// Video Regs
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2		)	// Layer 2 Control
	AM_RANGE(0xb80000, 0xb80005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1		)	// Layer 1 Control
	AM_RANGE(0xc00000, 0xc00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0		)	// Layer 0 Control
	AM_RANGE(0xd00000, 0xd00001) AM_WRITE(metmqstr_eeprom_msb_w			)	// EEPROM
	AM_RANGE(0xf00000, 0xf07fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xf08000, 0xf0ffff) AM_WRITE(MWA16_RAM						)	// RAM
ADDRESS_MAP_END


/***************************************************************************
                                Power Instinct 2
***************************************************************************/

READ16_HANDLER( pwrinst2_eeprom_r )
{
	return ~8 + ((EEPROM_read_bit() & 1) ? 8 : 0);
}

INLINE void vctrl_w(UINT16 *VCTRL, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 data, ATTR_UNUSED UINT16 mem_mask)
{
	if ( offset == 4/2 )
	{
		switch( data & 0x000f )
		{
			case 1:	data = (data & ~0x000f) | 0;	break;
			case 2:	data = (data & ~0x000f) | 1;	break;
			case 4:	data = (data & ~0x000f) | 2;	break;
			default:
			case 8:	data = (data & ~0x000f) | 3;	break;
		}
	}
	COMBINE_DATA(&VCTRL[offset]);
}
WRITE16_HANDLER( pwrinst2_vctrl_0_w )	{ vctrl_w(cave_vctrl_0, offset, data, mem_mask); }
WRITE16_HANDLER( pwrinst2_vctrl_1_w )	{ vctrl_w(cave_vctrl_1, offset, data, mem_mask); }
WRITE16_HANDLER( pwrinst2_vctrl_2_w )	{ vctrl_w(cave_vctrl_2, offset, data, mem_mask); }
WRITE16_HANDLER( pwrinst2_vctrl_3_w )	{ vctrl_w(cave_vctrl_3, offset, data, mem_mask); }

static ADDRESS_MAP_START( pwrinst2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x400000, 0x40ffff) AM_READ(MRA16_RAM					)	// RAM
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_0_word_r		)	// Inputs
	AM_RANGE(0x500002, 0x500003) AM_READ(input_port_1_word_r		)	//
	AM_RANGE(0x600000, 0x6fffff) AM_READ(MRA16_ROM					) AM_REGION(REGION_USER1, 0)	// extra data ROM space
	AM_RANGE(0x800000, 0x807fff) AM_READ(MRA16_RAM					)	// Layer 2
	AM_RANGE(0x880000, 0x887fff) AM_READ(MRA16_RAM					)	// Layer 0
	AM_RANGE(0x900000, 0x907fff) AM_READ(MRA16_RAM					)	// Layer 1
	AM_RANGE(0x980000, 0x987fff) AM_READ(MRA16_RAM					)	// Layer 3
	AM_RANGE(0xa00000, 0xa07fff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0xa08000, 0xa0ffff) AM_READ(MRA16_RAM					)	// Sprites?
	AM_RANGE(0xa10000, 0xa1ffff) AM_READ(MRA16_RAM					)	// Sprites?
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM					)	// Layer 2 Control
/**/AM_RANGE(0xb80000, 0xb80005) AM_READ(MRA16_RAM					)	// Layer 0 Control
/**/AM_RANGE(0xc00000, 0xc00005) AM_READ(MRA16_RAM					)	// Layer 1 Control
/**/AM_RANGE(0xc80000, 0xc80005) AM_READ(MRA16_RAM					)	// Layer 3 Control
	AM_RANGE(0xa80000, 0xa8007f) AM_READ(donpachi_videoregs_r		)	// Video Regs
	AM_RANGE(0xd80000, 0xd80001) AM_READ(soundlatch_ack_r			)	// From Sound CPU
	AM_RANGE(0xe80000, 0xe80001) AM_READ(pwrinst2_eeprom_r			)	// EEPROM
	AM_RANGE(0xf00000, 0xf04fff) AM_READ(MRA16_RAM					)	// Palette
ADDRESS_MAP_END

static ADDRESS_MAP_START( pwrinst2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(MWA16_RAM							)	// RAM
	AM_RANGE(0x700000, 0x700001) AM_WRITE(cave_eeprom_msb_w					)	// EEPROM
	AM_RANGE(0x800000, 0x807fff) AM_WRITE(cave_vram_2_w) AM_BASE(&cave_vram_2	)	// Layer 2
	AM_RANGE(0x880000, 0x887fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x900000, 0x907fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1	)	// Layer 1
	AM_RANGE(0x980000, 0x987fff) AM_WRITE(cave_vram_3_8x8_w) AM_BASE(&cave_vram_3	)	// Layer 3
	AM_RANGE(0xa00000, 0xa07fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0xa08000, 0xa0ffff) AM_WRITE(MWA16_RAM							)	// Sprites?
	AM_RANGE(0xa10000, 0xa1ffff) AM_WRITE(MWA16_RAM							)	// Sprites?
	AM_RANGE(0xa80000, 0xa8007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs		)	// Video Regs
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(pwrinst2_vctrl_2_w) AM_BASE(&cave_vctrl_2			)	// Layer 2 Control
	AM_RANGE(0xb80000, 0xb80005) AM_WRITE(pwrinst2_vctrl_0_w) AM_BASE(&cave_vctrl_0			)	// Layer 0 Control
	AM_RANGE(0xc00000, 0xc00005) AM_WRITE(pwrinst2_vctrl_1_w) AM_BASE(&cave_vctrl_1			)	// Layer 1 Control
	AM_RANGE(0xc80000, 0xc80005) AM_WRITE(pwrinst2_vctrl_3_w) AM_BASE(&cave_vctrl_3			)	// Layer 3 Control
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(sound_cmd_w						)	// To Sound CPU
	AM_RANGE(0xf00000, 0xf04fff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
ADDRESS_MAP_END


/***************************************************************************
                                Sailor Moon
***************************************************************************/

static READ16_HANDLER( sailormn_input0_r )
{
//  watchdog_reset16_r(0,0);    // written too rarely for mame.
	return readinputport(0);
}

static READ16_HANDLER( agallet_irq_cause_r )
{
	UINT16 irq_cause = cave_irq_cause_r(offset,mem_mask);

	if (offset == 0)
	{
// Speed hack for agallet
		if ((activecpu_get_pc() == 0xcdca) && (irq_cause & 4))
			cpu_spinuntil_int();
	}

	return irq_cause;
}

static ADDRESS_MAP_START( sailormn_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM				)	// RAM
	AM_RANGE(0x110000, 0x110001) AM_READ(MRA16_RAM				)	// (agallet)
	AM_RANGE(0x200000, 0x3fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM				)	// (agallet)
	AM_RANGE(0x408000, 0x40bfff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0x40c000, 0x40ffff) AM_READ(MRA16_RAM				)	// (agallet)
	AM_RANGE(0x410000, 0x410001) AM_READ(MRA16_RAM				)	// (agallet)
	AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM				)	// Sprites
	AM_RANGE(0x508000, 0x50ffff) AM_READ(MRA16_RAM				)	// Sprites?
	AM_RANGE(0x510000, 0x510001) AM_READ(MRA16_RAM				)	// (agallet)
	AM_RANGE(0x600000, 0x600001) AM_READ(sailormn_input0_r		)	// Inputs + Watchdog!
	AM_RANGE(0x600002, 0x600003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
	AM_RANGE(0x800000, 0x887fff) AM_READ(MRA16_RAM				)	// Layer 0
	AM_RANGE(0x880000, 0x887fff) AM_READ(MRA16_RAM				)	// Layer 1
	AM_RANGE(0x900000, 0x907fff) AM_READ(MRA16_RAM				)	// Layer 2
	AM_RANGE(0x908000, 0x908001) AM_READ(MRA16_RAM				)	// (agallet)
/**/AM_RANGE(0xa00000, 0xa00005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0xa80000, 0xa80005) AM_READ(MRA16_RAM				)	// Layer 1 Control
/**/AM_RANGE(0xb00000, 0xb00005) AM_READ(MRA16_RAM				)	// Layer 2 Control
	AM_RANGE(0xb80000, 0xb80007) AM_READ(cave_irq_cause_r		)	// IRQ Cause (bit 2 tested!)
	AM_RANGE(0xb8006c, 0xb8006d) AM_READ(soundflags_ack_r		)	// Communication
	AM_RANGE(0xb8006e, 0xb8006f) AM_READ(soundlatch_ack_r		)	// From Sound CPU
ADDRESS_MAP_END

static ADDRESS_MAP_START( sailormn_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM							)	// RAM
	AM_RANGE(0x110000, 0x110001) AM_WRITE(MWA16_RAM							)	// (agallet)
	AM_RANGE(0x200000, 0x3fffff) AM_WRITE(MWA16_ROM							)	// ROM
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(MWA16_RAM							)	// (agallet)
	AM_RANGE(0x408000, 0x40bfff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0x40c000, 0x40ffff) AM_WRITE(MWA16_RAM							)	// (agallet)
	AM_RANGE(0x410000, 0x410001) AM_WRITE(MWA16_RAM							)	// (agallet)
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x508000, 0x50ffff) AM_WRITE(MWA16_RAM							)	// Sprites?
	AM_RANGE(0x510000, 0x510001) AM_WRITE(MWA16_RAM							)	// (agallet)
	AM_RANGE(0x700000, 0x700001) AM_WRITE(sailormn_eeprom_msb_w				)	// EEPROM
	AM_RANGE(0x800000, 0x807fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0		)	// Layer 0
	AM_RANGE(0x880000, 0x887fff) AM_WRITE(cave_vram_1_w) AM_BASE(&cave_vram_1		)	// Layer 1
	AM_RANGE(0x900000, 0x907fff) AM_WRITE(cave_vram_2_w) AM_BASE(&cave_vram_2		)	// Layer 2
	AM_RANGE(0x908000, 0x908001) AM_WRITE(MWA16_RAM							)	// (agallet)
	AM_RANGE(0xa00000, 0xa00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0			)	// Layer 0 Control
	AM_RANGE(0xa80000, 0xa80005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_1			)	// Layer 1 Control
	AM_RANGE(0xb00000, 0xb00005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_2			)	// Layer 2 Control
	AM_RANGE(0xb8006e, 0xb8006f) AM_WRITE(sound_cmd_w						)	// To Sound CPU
	AM_RANGE(0xb80000, 0xb8007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs		)	// Video Regs
ADDRESS_MAP_END


/***************************************************************************
                                    Uo Poko
***************************************************************************/

static ADDRESS_MAP_START( uopoko_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM				)	// RAM
	AM_RANGE(0x300002, 0x300003) AM_READ(cave_sound_r			)	// YMZ280
/**/AM_RANGE(0x400000, 0x407fff) AM_READ(MRA16_RAM				)	// Sprites
/**/AM_RANGE(0x408000, 0x40ffff) AM_READ(MRA16_RAM				)	// Sprites?
/**/AM_RANGE(0x500000, 0x507fff) AM_READ(MRA16_RAM				)	// Layer 0
	AM_RANGE(0x600000, 0x600007) AM_READ(cave_irq_cause_r		)	// IRQ Cause
/**/AM_RANGE(0x700000, 0x700005) AM_READ(MRA16_RAM				)	// Layer 0 Control
/**/AM_RANGE(0x800000, 0x80ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0x900000, 0x900001) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0x900002, 0x900003) AM_READ(cave_input1_r			)	// Inputs + EEPROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( uopoko_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM						)	// RAM
	AM_RANGE(0x300000, 0x300003) AM_WRITE(cave_sound_w					)	// YMZ280
	AM_RANGE(0x400000, 0x407fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Sprites
	AM_RANGE(0x408000, 0x40ffff) AM_WRITE(MWA16_RAM						)	// Sprites?
	AM_RANGE(0x500000, 0x507fff) AM_WRITE(cave_vram_0_w) AM_BASE(&cave_vram_0	)	// Layer 0
	AM_RANGE(0x600000, 0x60007f) AM_WRITE(MWA16_RAM) AM_BASE(&cave_videoregs	)	// Video Regs
	AM_RANGE(0x700000, 0x700005) AM_WRITE(MWA16_RAM) AM_BASE(&cave_vctrl_0		)	// Layer 0 Control
	AM_RANGE(0x800000, 0x80ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)	// Palette
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(cave_eeprom_msb_w				)	// EEPROM
ADDRESS_MAP_END



/***************************************************************************


                        Memory Maps - Sound CPU (Optional)


***************************************************************************/

/***************************************************************************
                                Hotdog Storm
***************************************************************************/

WRITE8_HANDLER( hotdogst_rombank_w )
{
	UINT8 *RAM = memory_region(REGION_CPU2);
	int bank = data & 0x0f;
	if ( data & ~0x0f )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 1)	bank+=2;
	memory_set_bankptr(2, &RAM[ 0x4000 * bank ]);
}

WRITE8_HANDLER( hotdogst_okibank_w )
{
	UINT8 *RAM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0x3;
	int bank2 = (data >> 4) & 0x3;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static ADDRESS_MAP_START( hotdogst_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM	)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK2	)	// ROM (Banked)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotdogst_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM	)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_ROM	)	// ROM (Banked)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotdogst_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x30, 0x30) AM_READ(soundlatch_lo_r			)	// From Main CPU
	AM_RANGE(0x40, 0x40) AM_READ(soundlatch_hi_r			)	//
	AM_RANGE(0x50, 0x50) AM_READ(YM2203_status_port_0_r	)	// YM2203
	AM_RANGE(0x51, 0x51) AM_READ(YM2203_read_port_0_r		)	//
	AM_RANGE(0x60, 0x60) AM_READ(OKIM6295_status_0_r		)	// M6295
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotdogst_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(hotdogst_rombank_w		)	// ROM bank
	AM_RANGE(0x50, 0x50) AM_WRITE(YM2203_control_port_0_w	)	// YM2203
	AM_RANGE(0x51, 0x51) AM_WRITE(YM2203_write_port_0_w		)	//
	AM_RANGE(0x60, 0x60) AM_WRITE(OKIM6295_data_0_w			)	// M6295
	AM_RANGE(0x70, 0x70) AM_WRITE(hotdogst_okibank_w		)	// Samples bank
ADDRESS_MAP_END


/***************************************************************************
                                Mazinger Z
***************************************************************************/

WRITE8_HANDLER( mazinger_rombank_w )
{
	UINT8 *RAM = memory_region(REGION_CPU2);
	int bank = data & 0x07;
	if ( data & ~0x07 )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 1)	bank+=2;
	memory_set_bankptr(2, &RAM[ 0x4000 * bank ]);
}

static ADDRESS_MAP_START( mazinger_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM	)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK2	)	// ROM (Banked)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM	)	// RAM
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mazinger_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM	)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_ROM	)	// ROM (Banked)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM	)	// RAM
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mazinger_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x30, 0x30) AM_READ(soundlatch_lo_r			)	// From Main CPU
	AM_RANGE(0x52, 0x52) AM_READ(YM2203_status_port_0_r	)	// YM2203
ADDRESS_MAP_END

static ADDRESS_MAP_START( mazinger_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(mazinger_rombank_w		)	// ROM bank
	AM_RANGE(0x10, 0x10) AM_WRITE(soundlatch_ack_w			)	// To Main CPU
	AM_RANGE(0x50, 0x50) AM_WRITE(YM2203_control_port_0_w	)	// YM2203
	AM_RANGE(0x51, 0x51) AM_WRITE(YM2203_write_port_0_w		)	//
	AM_RANGE(0x70, 0x70) AM_WRITE(OKIM6295_data_0_w			)	// M6295
	AM_RANGE(0x74, 0x74) AM_WRITE(hotdogst_okibank_w		)	// Samples bank
ADDRESS_MAP_END


/***************************************************************************
                                Metamoqester
***************************************************************************/

WRITE8_HANDLER( metmqstr_rombank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU2);
	int bank = data & 0xf;
	if ( bank != data )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank >= 2)	bank += 2;
	memory_set_bankptr(1, &ROM[ 0x4000 * bank ]);
}

WRITE8_HANDLER( metmqstr_okibank0_w )
{
	UINT8 *ROM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0x7;
	int bank2 = (data >> 4) & 0x7;
	memcpy(ROM + 0x20000 * 0, ROM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(ROM + 0x20000 * 1, ROM + 0x40000 + 0x20000 * bank2, 0x20000);
}

WRITE8_HANDLER( metmqstr_okibank1_w )
{
	UINT8 *ROM = memory_region(REGION_SOUND2);
	int bank1 = (data >> 0) & 0x7;
	int bank2 = (data >> 4) & 0x7;
	memcpy(ROM + 0x20000 * 0, ROM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(ROM + 0x20000 * 1, ROM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static ADDRESS_MAP_START( metmqstr_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM	)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1	)	// ROM (Banked)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( metmqstr_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM	)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_ROM	)	// ROM (Banked)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( metmqstr_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x20, 0x20) AM_READ(soundflags_r				)	// Communication
	AM_RANGE(0x30, 0x30) AM_READ(soundlatch_lo_r			)	// From Main CPU
	AM_RANGE(0x40, 0x40) AM_READ(soundlatch_hi_r			)	//
	AM_RANGE(0x51, 0x51) AM_READ(YM2151_status_port_0_r	)	// YM2151
ADDRESS_MAP_END

static ADDRESS_MAP_START( metmqstr_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(metmqstr_rombank_w		)	// Rom Bank
	AM_RANGE(0x50, 0x50) AM_WRITE(YM2151_register_port_0_w	)	// YM2151
	AM_RANGE(0x51, 0x51) AM_WRITE(YM2151_data_port_0_w		)	//
	AM_RANGE(0x60, 0x60) AM_WRITE(OKIM6295_data_0_w			)	// M6295 #0
	AM_RANGE(0x70, 0x70) AM_WRITE(metmqstr_okibank0_w		)	// Samples Bank #0
	AM_RANGE(0x80, 0x80) AM_WRITE(OKIM6295_data_1_w			)	// M6295 #1
	AM_RANGE(0x90, 0x90) AM_WRITE(metmqstr_okibank1_w		)	// Samples Bank #1
ADDRESS_MAP_END


/***************************************************************************
                                Power Instinct 2
***************************************************************************/

WRITE8_HANDLER( pwrinst2_rombank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU2);
	int bank = data & 0x07;
	if ( data & ~0x07 )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 2)	bank+=1;
	memory_set_bankptr(1, &ROM[ 0x4000 * bank ]);
}

static ADDRESS_MAP_START( pwrinst2_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM	)	// ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1	)	// ROM (Banked)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pwrinst2_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM	)	// ROM
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_ROM	)	// ROM (Banked)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM	)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pwrinst2_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(OKIM6295_status_0_r		)	// M6295
	AM_RANGE(0x08, 0x08) AM_READ(OKIM6295_status_1_r		)	//
	AM_RANGE(0x40, 0x40) AM_READ(YM2203_status_port_0_r	)	// YM2203
	AM_RANGE(0x41, 0x41) AM_READ(YM2203_read_port_0_r		)	//
	AM_RANGE(0x60, 0x60) AM_READ(soundlatch_hi_r			)	// From Main CPU
	AM_RANGE(0x70, 0x70) AM_READ(soundlatch_lo_r			)	//
ADDRESS_MAP_END

static ADDRESS_MAP_START( pwrinst2_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(OKIM6295_data_0_w			)	// M6295
	AM_RANGE(0x08, 0x08) AM_WRITE(OKIM6295_data_1_w			)	//
	AM_RANGE(0x10, 0x17) AM_WRITE(NMK112_okibank_w			)	// Samples bank
	AM_RANGE(0x40, 0x40) AM_WRITE(YM2203_control_port_0_w	)	// YM2203
	AM_RANGE(0x41, 0x41) AM_WRITE(YM2203_write_port_0_w		)	//
        AM_RANGE(0x50, 0x50) AM_WRITE(soundlatch_ack_w			)       // To Main CPU
//  AM_RANGE(0x51, 0x51) AM_WRITE(MWA8_NOP      )   // ?? volume
	AM_RANGE(0x80, 0x80) AM_WRITE(pwrinst2_rombank_w		)	// ROM bank
ADDRESS_MAP_END


/***************************************************************************
                                Sailor Moon
***************************************************************************/

static UINT8 *mirror_ram;
static READ8_HANDLER( mirror_ram_r )
{
	return mirror_ram[offset];
}
static WRITE8_HANDLER( mirror_ram_w )
{
	mirror_ram[offset] = data;
}

WRITE8_HANDLER( sailormn_rombank_w )
{
	UINT8 *RAM = memory_region(REGION_CPU2);
	int bank = data & 0x1f;
	if ( data & ~0x1f )	logerror("CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);
	if (bank > 1)	bank+=2;
	memory_set_bankptr(1, &RAM[ 0x4000 * bank ]);
}

WRITE8_HANDLER( sailormn_okibank0_w )
{
	UINT8 *RAM = memory_region(REGION_SOUND1);
	int bank1 = (data >> 0) & 0xf;
	int bank2 = (data >> 4) & 0xf;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

WRITE8_HANDLER( sailormn_okibank1_w )
{
	UINT8 *RAM = memory_region(REGION_SOUND2);
	int bank1 = (data >> 0) & 0xf;
	int bank2 = (data >> 4) & 0xf;
	memcpy(RAM + 0x20000 * 0, RAM + 0x40000 + 0x20000 * bank1, 0x20000);
	memcpy(RAM + 0x20000 * 1, RAM + 0x40000 + 0x20000 * bank2, 0x20000);
}

static ADDRESS_MAP_START( sailormn_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1					)	// ROM (Banked)
	AM_RANGE(0xc000, 0xdfff) AM_READ(mirror_ram_r				)	// RAM
	AM_RANGE(0xe000, 0xffff) AM_READ(mirror_ram_r				)	// Mirrored RAM (agallet)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sailormn_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM					)	// ROM
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_ROM					)	// ROM (Banked)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(mirror_ram_w) AM_BASE(&mirror_ram	)	// RAM
	AM_RANGE(0xe000, 0xffff) AM_WRITE(mirror_ram_w				)	// Mirrored RAM (agallet)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sailormn_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x20, 0x20) AM_READ(soundflags_r				)	// Communication
	AM_RANGE(0x30, 0x30) AM_READ(soundlatch_lo_r			)	// From Main CPU
	AM_RANGE(0x40, 0x40) AM_READ(soundlatch_hi_r			)	//
	AM_RANGE(0x51, 0x51) AM_READ(YM2151_status_port_0_r	)	// YM2151
	AM_RANGE(0x60, 0x60) AM_READ(OKIM6295_status_0_r		)	// M6295 #0
	AM_RANGE(0x80, 0x80) AM_READ(OKIM6295_status_1_r		)	// M6295 #1
ADDRESS_MAP_END

static ADDRESS_MAP_START( sailormn_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(sailormn_rombank_w		)	// Rom Bank
	AM_RANGE(0x10, 0x10) AM_WRITE(soundlatch_ack_w			)	// To Main CPU
	AM_RANGE(0x50, 0x50) AM_WRITE(YM2151_register_port_0_w	)	// YM2151
	AM_RANGE(0x51, 0x51) AM_WRITE(YM2151_data_port_0_w		)	//
	AM_RANGE(0x60, 0x60) AM_WRITE(OKIM6295_data_0_w			)	// M6295 #0
	AM_RANGE(0x70, 0x70) AM_WRITE(sailormn_okibank0_w		)	// Samples Bank #0
	AM_RANGE(0x80, 0x80) AM_WRITE(OKIM6295_data_1_w			)	// M6295 #1
	AM_RANGE(0xc0, 0xc0) AM_WRITE(sailormn_okibank1_w		)	// Samples Bank #1
ADDRESS_MAP_END



/***************************************************************************


                                Input Ports


***************************************************************************/

/*
    dfeveron config menu:
    101624.w -> 8,a6    preferences
    101626.w -> c,a6    (1:coin<<4|credit) <<8 | (2:coin<<4|credit)
*/

/* Most games use this */
INPUT_PORTS_START( cave )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(6)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? exit service mode
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? enter & exit service mode
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(6)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_SERVICE1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

/* Gaia Crusaders, no EEPROM. Has DIPS */
INPUT_PORTS_START( gaia )
	PORT_START	// IN0 - Player 1 + 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START	// IN1 - Coins
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(6)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(6)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Dips
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Language ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( English ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x0078, 0x0078, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0048, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, "2 Co./1 Cr./1 Cont." )
	PORT_DIPSETTING(      0x0078, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0058, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0068, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, "150k/350k" )
	PORT_DIPNAME( 0x1800, 0x1800, "Damage" )
	PORT_DIPSETTING(      0x1800, "+0" )
	PORT_DIPSETTING(      0x1000, "+1" )
	PORT_DIPSETTING(      0x0800, "+2" )
	PORT_DIPSETTING(      0x0000, "+3" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(      0xa000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x6000, "Medium Hard" )
	PORT_DIPSETTING(      0x8000, "Hard 1" )
	PORT_DIPSETTING(      0x2000, "Hard 2" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
INPUT_PORTS_END

/* Mazinger Z (has region stored in Eeprom) */
INPUT_PORTS_START( mazinger )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(6)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? exit service mode
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? enter & exit service mode
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(6)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_SERVICE1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	// Eeprom Region
	PORT_DIPNAME( 0xff, 0x31, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x31, DEF_STR( World ) )
INPUT_PORTS_END

/* Sailor Moon / Air Gallet (has region stored in Eeprom) */
INPUT_PORTS_START( sailormn )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(6)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? exit service mode
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? enter & exit service mode
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(6)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_SERVICE1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	// Eeprom Region
	PORT_DIPNAME( 0xff, 0x02, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x01, DEF_STR( USA ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Europe ) )
	PORT_DIPSETTING(    0x03, "Hong Kong" )
	PORT_DIPSETTING(    0x04, "Taiwan" )
	PORT_DIPSETTING(    0x05, "Korea" )
INPUT_PORTS_END

/* Different layout */
INPUT_PORTS_START( guwange )
	PORT_START	// IN0 - Player 1 & 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START	// IN1 - Coins
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(6)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(6)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* Normal layout but with 4 buttons */
INPUT_PORTS_START( metmqstr )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(6)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	// sw? enter & exit service mode
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2  )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(6)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_SERVICE1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( korokoro )
	PORT_START	// IN0
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1   ) PORT_IMPULSE(10)	// bit 0x0010 of leds (coin)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2   ) PORT_IMPULSE(10)	// bit 0x0020 of leds (does coin sound)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_COIN3   ) PORT_IMPULSE(10)	// bit 0x0080 of leds
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 )	// round  button (choose)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 )	// square button (select in service mode / medal out in game)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_SERVICE2)	// service medal out?
	PORT_SERVICE( 0x2000, IP_ACTIVE_LOW )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SERVICE1)	// service coin
	PORT_BIT(  0x8000, IP_ACTIVE_HIGH, IPT_SPECIAL)	// motor / hopper status ???

	PORT_START	// IN1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom bit
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************


                            Graphics Layouts


***************************************************************************/

/* 8x8x4 tiles */
static const gfx_layout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{STEP4(0,1)},
	{STEP8(0,4)},
	{STEP8(0,4*8)},
	8*8*4
};

/* 8x8x6 tiles (in a 8x8x8 layout) */
static const gfx_layout layout_8x8x6 =
{
	8,8,
	RGN_FRAC(1,1),
	6,
	{8,9, 0,1,2,3},
	{0*4,1*4,4*4,5*4,8*4,9*4,12*4,13*4},
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64},
	8*8*8
};

/* 8x8x6 tiles (4 bits in one rom, 2 bits in the other,
   unpacked in 2 pages of 4 bits) */
static const gfx_layout layout_8x8x6_2 =
{
	8,8,
	RGN_FRAC(1,2),
	6,
	{RGN_FRAC(1,2)+2,RGN_FRAC(1,2)+3, STEP4(0,1)},
	{STEP8(0,4)},
	{STEP8(0,4*8)},
	8*8*4
};

/* 8x8x8 tiles */
static const gfx_layout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{8,9,10,11, 0,1,2,3},
	{0*4,1*4,4*4,5*4,8*4,9*4,12*4,13*4},
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64},
	8*8*8
};

#if 0
/* 16x16x8 Zooming Sprites - No need to decode them */
static const gfx_layout layout_sprites =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{STEP8(0,1)},
	{STEP16(0,8)},
	{STEP16(0,16*8)},
	16*16*8
};
#endif

/***************************************************************************
                                Dangun Feveron
***************************************************************************/

static const gfx_decode dfeveron_gfxdecodeinfo[] =
{
	/* There are only $800 colors here, the first half for sprites
       the second half for tiles. We use $8000 virtual colors instead
       for consistency with games having $8000 real colors.
       A PALETTE_INIT function is thus needed for sprites */

//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4400, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4400, 0x40 }, // [1] Layer 1
	{ -1 }
};

/***************************************************************************
                                Dodonpachi
***************************************************************************/

static const gfx_decode ddonpach_gfxdecodeinfo[] =
{
	/* Layers 0&1 are 4 bit deep and use the first 16 of every 256
       colors for any given color code (a PALETTE_INIT function
       is provided for these layers, filling the 8000-83ff entries
       in the color table). Layer 2 uses the whole 256 for any given
       color code and the 4000-7fff range in the color table.   */

//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x8000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x8000, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x8,	0x4000, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
                                Donpachi
***************************************************************************/

static const gfx_decode donpachi_gfxdecodeinfo[] =
{
	/* There are only $800 colors here, the first half for sprites
       the second half for tiles. We use $8000 virtual colors instead
       for consistency with games having $8000 real colors.
       A PALETTE_INIT function is thus needed for sprites */

//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4400, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4400, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x4,	0x4400, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
                                Esprade
***************************************************************************/

static const gfx_decode esprade_gfxdecodeinfo[] =
{
//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x8,	0x4000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x8,	0x4000, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x8,	0x4000, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
                                Hotdog Storm
***************************************************************************/

static const gfx_decode hotdogst_gfxdecodeinfo[] =
{
	/* There are only $800 colors here, the first half for sprites
       the second half for tiles. We use $8000 virtual colors instead
       for consistency with games having $8000 real colors.
       A PALETTE_INIT function is needed for sprites */

//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4000, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x4,	0x4000, 0x40 }, // [2] Layer 2
	{ -1 }
};

/***************************************************************************
                                Koro Koro Quest
***************************************************************************/

static const gfx_decode korokoro_gfxdecodeinfo[] =
{
//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4400, 0x40 }, // [0] Layer 0
	{ -1 }
};

/***************************************************************************
                                Mazinger Z
***************************************************************************/

static const gfx_decode mazinger_gfxdecodeinfo[] =
{
	/*  Sprites are 4 bit deep.
        Layer 0 is 4 bit deep.
        Layer 1 uses 64 color palettes, but the game only fills the
        first 16 colors of each palette, Indeed, the gfx data in ROM
        is empty in the top 4 bits. Additionally even if there are
        $40 color codes, only $400 colors are addressable.
        A PALETTE_INIT function is thus needed for sprites and layer 0.   */

//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x6,	0x4400, 0x40 }, // [1] Layer 1
	{ -1 }
};


/***************************************************************************
                                Power Instinct 2
***************************************************************************/

static const gfx_decode pwrinst2_gfxdecodeinfo[] =
{
//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x0800+0x8000, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x1000+0x8000, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x4,	0x1800+0x8000, 0x40 }, // [2] Layer 2
	{ REGION_GFX5, 0, &layout_8x8x4,	0x2000+0x8000, 0x40 }, // [3] Layer 3
	{ -1 }
};


/***************************************************************************
                                Sailor Moon
***************************************************************************/

static const gfx_decode sailormn_gfxdecodeinfo[] =
{
	/* 4 bit sprites ? */
//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x4,	0x4400, 0x40 }, // [0] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,	0x4800, 0x40 }, // [1] Layer 1
	{ REGION_GFX4, 0, &layout_8x8x6_2,	0x4c00, 0x40 }, // [2] Layer 2
	{ -1 }
};


/***************************************************************************
                                Uo Poko
***************************************************************************/

static const gfx_decode uopoko_gfxdecodeinfo[] =
{
//    REGION_GFX1                                       // Sprites
	{ REGION_GFX2, 0, &layout_8x8x8,	0x4000, 0x40 }, // [0] Layer 0
	{ -1 }
};


/***************************************************************************


                                Machine Drivers


***************************************************************************/

MACHINE_RESET( cave )
{
	soundbuf.len = 0;

	/* modify the eeprom on a reset with the desired region for the games that have the
       region factory set in eeprom */
	if (cave_region_byte >= 0)
		EEPROM_get_data_pointer(0)[cave_region_byte] =  readinputport(2);
}

static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	sound_irq_gen
};

static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	irqhandler
};

static struct YM2203interface ym2203_interface =
{
	0,0,0,0,irqhandler
};


/***************************************************************************
                                Dangun Feveron
***************************************************************************/

static MACHINE_DRIVER_START( dfeveron )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(dfeveron_readmem,dfeveron_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(dfeveron_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_2_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Dodonpachi
***************************************************************************/

static MACHINE_DRIVER_START( ddonpach )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(ddonpach_readmem,ddonpach_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(ddonpach_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_COLORTABLE_LENGTH(0x8000 + 0x40*16)	// $400 extra entries for layers 1&2

	MDRV_PALETTE_INIT(ddonpach)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                    Donpachi
***************************************************************************/

static MACHINE_DRIVER_START( donpachi )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(donpachi_readmem,donpachi_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(donpachi_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.60)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.60)

	MDRV_SOUND_ADD(OKIM6295, 16000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Esprade
***************************************************************************/

static MACHINE_DRIVER_START( esprade )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(esprade_readmem,esprade_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(esprade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                    Gaia Crusaders
***************************************************************************/

static MACHINE_DRIVER_START( gaia )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(gaia_readmem,gaia_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(esprade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                    Guwange
***************************************************************************/

static MACHINE_DRIVER_START( guwange )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(guwange_readmem,guwange_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(esprade_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/***************************************************************************
                                Hotdog Storm
***************************************************************************/

static MACHINE_DRIVER_START( hotdogst )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(hotdogst_readmem,hotdogst_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(hotdogst_sound_readmem,hotdogst_sound_writemem)
	MDRV_CPU_IO_MAP(hotdogst_sound_readport,hotdogst_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 240)
	MDRV_VISIBLE_AREA(0, 384-1, 0, 240-1)
	MDRV_GFXDECODE(hotdogst_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.20)
	MDRV_SOUND_ROUTE(0, "right", 0.20)
	MDRV_SOUND_ROUTE(1, "left",  0.20)
	MDRV_SOUND_ROUTE(1, "right", 0.20)
	MDRV_SOUND_ROUTE(2, "left",  0.20)
	MDRV_SOUND_ROUTE(2, "right", 0.20)
	MDRV_SOUND_ROUTE(3, "left",  0.80)
	MDRV_SOUND_ROUTE(3, "right", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                               Koro Koro Quest
***************************************************************************/

static MACHINE_DRIVER_START( korokoro )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(korokoro_readmem,korokoro_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(korokoro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1-2, 0, 240-1-1)
	MDRV_GFXDECODE(korokoro_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x4000)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(korokoro)
	MDRV_VIDEO_START(cave_1_layer)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Mazinger Z
***************************************************************************/

static MACHINE_DRIVER_START( mazinger )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(mazinger_readmem,mazinger_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
//  /* audio CPU */ // Bidirectional communication
	MDRV_CPU_PROGRAM_MAP(mazinger_sound_readmem,mazinger_sound_writemem)
	MDRV_CPU_IO_MAP(mazinger_sound_readport,mazinger_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_WATCHDOG_VBLANK_INIT(DEFAULT_60HZ_3S_VBLANK_WATCHDOG)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 240)
	MDRV_VISIBLE_AREA(0, 384-1, 0, 240-1)
	MDRV_GFXDECODE(mazinger_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x4000)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(mazinger)
	MDRV_VIDEO_START(cave_2_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.20)
	MDRV_SOUND_ROUTE(0, "right", 0.20)
	MDRV_SOUND_ROUTE(1, "left",  0.20)
	MDRV_SOUND_ROUTE(1, "right", 0.20)
	MDRV_SOUND_ROUTE(2, "left",  0.20)
	MDRV_SOUND_ROUTE(2, "right", 0.20)
	MDRV_SOUND_ROUTE(3, "left",  0.60)
	MDRV_SOUND_ROUTE(3, "right", 0.60)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 2.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 2.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Metamoqester
***************************************************************************/

static MACHINE_DRIVER_START( metmqstr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,32000000 / 2)
	MDRV_CPU_PROGRAM_MAP(metmqstr_readmem,metmqstr_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80,32000000 / 4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(metmqstr_sound_readmem,metmqstr_sound_writemem)
	MDRV_CPU_IO_MAP(metmqstr_sound_readport,metmqstr_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_WATCHDOG_VBLANK_INIT(DEFAULT_60HZ_3S_VBLANK_WATCHDOG)

	MDRV_MACHINE_RESET(cave)	/* start with the watchdog armed */
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 240)
	MDRV_VISIBLE_AREA(0x7d, 0x7d + 0x180-1, 0, 240-1)
	MDRV_GFXDECODE(donpachi_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(dfeveron)
	MDRV_VIDEO_START(cave_3_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 16000000/4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.20)
	MDRV_SOUND_ROUTE(1, "right", 1.20)

	MDRV_SOUND_ADD(OKIM6295, 32000000 / 16 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 32000000 / 16 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Power Instinct 2
***************************************************************************/

/*  X1 = 12 MHz, X2 = 28 MHz, X3 = 16 MHz. OKI: / 165 mode A ; / 132 mode B */

static MACHINE_DRIVER_START( pwrinst2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz */
	MDRV_CPU_PROGRAM_MAP(pwrinst2_readmem,pwrinst2_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80,16000000 / 2)
	/* audio CPU */	/* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(pwrinst2_sound_readmem,pwrinst2_sound_writemem)
	MDRV_CPU_IO_MAP(pwrinst2_sound_readport,pwrinst2_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 240)
	MDRV_VISIBLE_AREA(0x70, 0x70 + 0x140-1, 0, 240-1)
	MDRV_GFXDECODE(pwrinst2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x5000/2)
	MDRV_COLORTABLE_LENGTH(0x8000+0x2800)

	MDRV_PALETTE_INIT(pwrinst2)
	MDRV_VIDEO_START(cave_4_layers)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2203, 16000000 / 4)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.40)
	MDRV_SOUND_ROUTE(0, "right", 0.40)
	MDRV_SOUND_ROUTE(1, "left",  0.40)
	MDRV_SOUND_ROUTE(1, "right", 0.40)
	MDRV_SOUND_ROUTE(2, "left",  0.40)
	MDRV_SOUND_ROUTE(2, "right", 0.40)
	MDRV_SOUND_ROUTE(3, "left",  0.80)
	MDRV_SOUND_ROUTE(3, "right", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 3000000 / 165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.80)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 3000000 / 165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.00)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.00)
MACHINE_DRIVER_END


/***************************************************************************
                        Sailor Moon / Air Gallet
***************************************************************************/

static MACHINE_DRIVER_START( sailormn )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(sailormn_readmem,sailormn_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_CPU_ADD(Z80, 8000000)
//  /* audio CPU */ // Bidirectional Communication
	MDRV_CPU_PROGRAM_MAP(sailormn_sound_readmem,sailormn_sound_writemem)
	MDRV_CPU_IO_MAP(sailormn_sound_readport,sailormn_sound_writeport)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
//  MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(cave)
	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320+1, 240)
	MDRV_VISIBLE_AREA(0+1, 320+1-1, 0, 240-1)
	MDRV_GFXDECODE(sailormn_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000)
	MDRV_COLORTABLE_LENGTH(0x8000)	/* $8000 palette entries for consistency with the other games */

	MDRV_PALETTE_INIT(sailormn)	// 4 bit sprites, 6 bit tiles
	MDRV_VIDEO_START(sailormn_3_layers)	/* Layer 2 has 1 banked ROM */
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(YM2151, 16000000/4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.30)
	MDRV_SOUND_ROUTE(1, "right", 0.30)

	MDRV_SOUND_ADD(OKIM6295, 16000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 16000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Uo Poko
***************************************************************************/

static MACHINE_DRIVER_START( uopoko )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(uopoko_readmem,uopoko_writemem)
	MDRV_CPU_VBLANK_INT(cave_interrupt,1)

	MDRV_FRAMES_PER_SECOND(15625/271.5)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(cave)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(uopoko_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(cave_1_layer)
	MDRV_VIDEO_UPDATE(cave)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************


                                ROMs Loading


***************************************************************************/

/* 4 bits -> 8 bits. Even and odd pixels are swapped */
static void unpack_sprites(void)
{
	const int region		=	REGION_GFX1;	// sprites

	const unsigned int len	=	memory_region_length(region);
	unsigned char *src		=	memory_region(region) + len / 2 - 1;
	unsigned char *dst		=	memory_region(region) + len - 1;

	while(dst > src)
	{
		unsigned char data = *src--;
		/* swap even and odd pixels */
		*dst-- = data >> 4;		*dst-- = data & 0xF;
	}
}


/* 4 bits -> 8 bits. Even and odd pixels and even and odd words, are swapped */
static void ddonpach_unpack_sprites(void)
{
	const int region		=	REGION_GFX1;	// sprites

	const unsigned int len	=	memory_region_length(region);
	unsigned char *src		=	memory_region(region) + len / 2 - 1;
	unsigned char *dst		=	memory_region(region) + len - 1;

	while(dst > src)
	{
		unsigned char data1= *src--;
		unsigned char data2= *src--;
		unsigned char data3= *src--;
		unsigned char data4= *src--;

		/* swap even and odd pixels, and even and odd words */
		*dst-- = data2 & 0xF;		*dst-- = data2 >> 4;
		*dst-- = data1 & 0xF;		*dst-- = data1 >> 4;
		*dst-- = data4 & 0xF;		*dst-- = data4 >> 4;
		*dst-- = data3 & 0xF;		*dst-- = data3 >> 4;
	}
}


/* 2 pages of 4 bits -> 8 bits */
static void esprade_unpack_sprites(void)
{
	const int region		=	REGION_GFX1;	// sprites

	unsigned char *src		=	memory_region(region);
	unsigned char *dst		=	memory_region(region) + memory_region_length(region);

	while(src < dst)
	{
		unsigned char data1 = src[0];
		unsigned char data2 = src[1];

		src[0] = ((data1 & 0x0f)<<4) + (data2 & 0x0f);
		src[1] = (data1 & 0xf0) + ((data2 & 0xf0)>>4);

		src += 2;
	}
}

/***************************************************************************

                                Air Gallet

Banpresto
Runs on identical board to Sailor Moon (several sockets unpopulated)

PCB: BP945A (overstamped with BP962A)
CPU: TMP68HC000P16 (68000, 64 pin DIP)
SND: Z84C0008PEC (Z80, 40 pin DIP), OKI M6295 x 2, YM2151, YM3012
OSC: 28.000MHz, 16.000MHz
RAM: 62256 x 8, NEC 424260 x 2, 6264 x 5

Other Chips:
SGS Thomson ST93C46CB1 (EEPROM)
PALS (same as Sailor Moon, not dumped):
      18CV8 label SMBG
      18CV8 label SMZ80
      18CV8 label SMCPU
      GAL16V8 (located near BP962A.U47)

GFX:  038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      013 9346E7002 (240 pin PQFP)

On PCB near JAMMA connector is a small push button to access test mode.

ROMS:
BP962A.U9   27C040      Sound Program
BP962A.U45  27C240      Main Program
BP962A.U47  23C16000    Sound
BP962A.U48  23C16000    Sound
BP962A.U53  23C16000    GFX
BP962A.U54  23C16000    GFX
BP962A.U57  23C16000    GFX
BP962A.U65  23C16000    GFX
BP962A.U76  23C16000    GFX
BP962A.U77  23C16000    GFX

***************************************************************************/

ROM_START( agallet )	// Shows "Taiwan Only" on the copyright notice screen.
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "bp962a.u45", 0x000000, 0x080000, CRC(24815046) SHA1(f5eeae60b923ae850b335e7898a2760407631d8b) )
	//empty

	ROM_REGION( 0x88000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "bp962a.u9",  0x00000, 0x08000, CRC(06caddbe) SHA1(6a3cc50558ba19a31b21b7f3ec6c6e2846244ff1) )	// 1xxxxxxxxxxxxxxxxxx = 0xFF
	ROM_CONTINUE(           0x10000, 0x78000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bp962a.u76", 0x000000, 0x200000, CRC(858da439) SHA1(33a3d2a3ec3fa3364b00e1e43b405e5030a5b2a3) )
	ROM_LOAD( "bp962a.u77", 0x200000, 0x200000, CRC(ea2ba35e) SHA1(72487f21d44fe7be9a98068ce7f57a43c132945f) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bp962a.u53", 0x000000, 0x100000, CRC(fcd9a107) SHA1(169b94db8389e7d47d4d77f36907a62c30fea727) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bp962a.u54", 0x000000, 0x200000, CRC(0cfa3409) SHA1(17107e26762ef7e3b902fb29a6d7bc534a4d09aa) )

	ROM_REGION( (1*0x200000)*2, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	/* 4 bit part */
	ROM_LOAD( "bp962a.u57", 0x000000, 0x200000, CRC(6d608957) SHA1(15f6e8346f5f95eb229505b1b4666dabeb810ee8) )
	/* 2 bit part */
	ROM_LOAD( "bp962a.u65", 0x200000, 0x100000, CRC(135fcf9a) SHA1(2e8c89c2627bbdef160d96724d07883fb2fa1a57) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x200000, 0x100000             )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 #0 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp962a.u48", 0x040000, 0x200000, CRC(ae00a1ce) SHA1(5e8c74df0ac77efb3080406870856f958be14f79) )	// 16 x $20000, FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x240000, REGION_SOUND2, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp962a.u47", 0x040000, 0x200000, CRC(6d4e9737) SHA1(81c7ecdfc2d38d0b35e26745866f6672f566f936) )	// 16 x $20000, FIRST AND SECOND HALF IDENTICAL
ROM_END


/***************************************************************************

                                Dangun Feveron

Board:  CV01
OSC:    28.0, 16.0, 16.9 MHz

***************************************************************************/

ROM_START( dfeveron )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "cv01-u34.bin", 0x000000, 0x080000, CRC(be87f19d) SHA1(595239245df3835cdf5a99a6c62480465558d8d3) )
	ROM_LOAD16_BYTE( "cv01-u33.bin", 0x000001, 0x080000, CRC(e53a7db3) SHA1(ddced29f78dc3cc89038757b6577ba2ba0d8b041) )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "cv01-u25.bin", 0x000000, 0x400000, CRC(a6f6a95d) SHA1(e1eb45cb5d0e6163edfd9d830633b913fb53c6ca) )
	ROM_LOAD( "cv01-u26.bin", 0x400000, 0x400000, CRC(32edb62a) SHA1(3def74e1316b80cc25a8c3ac162cd7bcb8cc807c) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "cv01-u50.bin", 0x000000, 0x200000, CRC(7a344417) SHA1(828bd8f95d2fcc34407e17629ccafc904a4ea12d) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "cv01-u49.bin", 0x000000, 0x200000, CRC(d21cdda7) SHA1(cace4650de580c3c4a037f1f5c32bfc1846b383c) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "cv01-u19.bin", 0x000000, 0x400000, CRC(5f5514da) SHA1(53f27364aee544572a82649c9ff29bacc642b732) )
ROM_END

/*

Fever SOS

this doesn't work, I don't know why, roms should be good

Jumper JP1:
INT Version - 2 & 3
JAP Version - 1 & 2

However there are more differences:

U4:
INT Version  - 9838EX003
JAP Version - 9807EX004

UA2 & UB2:
INT Version  - 038 9838WX001
JAP Version - 038 9808WX003

TA8030S (Beside SW1)
INT Version  - NOT MOUNTED
JAP Version - TA8030S (WatchDog Timer, might be controlled by JP1)

U47 & U48 - Differ
U38 & U37 - Differ

These chips however are Static RAM so I don't think anything is wrong!

I suspect the main difference is the graphics chips. Looks like the
international version is running on different H/W ?

It actually looks like the international version is older than
the Japanese version PCB wise, but the software date is 98/09/25
and mine is 98/09/17!

The famous full extent of the JAM is inside the image but so is
"full extent" of the LAW. There are also other version strings
inside the same image look here...

          NOTICE
  THIS GAME IS FOR USE IN
                KOREA ONLY
            HONG KONG ONLY
               TAIWAN ONLY
       SOUTHEAST ASIA ONLY
               EUROPE ONLY
                U.S.A ONLY
                JAPAN ONLY
SALES, EXPORT OR OPERATION
OUTSIDE THIS COUNTRY MAY BE
CONSTRUED AS COPYRIGHT AND
TRADEMARK INFRINGEMENT AND
IS STRICTLY PROHIBITED.
VIOLATOR AND SUBJECT TO
SEVERE PENALTIES AND WILL
BE PROSECUTED TO THE FULL
EXTENT OF THE JAM.
              98/09/10 VER.

Look at the version date!

          NOTICE
THIS GAME MAY NOT BE SOLD,
EXPORTED OR OPERATED
WITHOUTPROOF OF LEGAL CONSENT
BY CAVE CO.,LTD.
VIOLATION OF THESE TERMS WILL
RESULT IN COPYRIGHT AND
TRADEMARK INFRINGEMENT,AND IS
STRICTLY PROHIBITED.
VIOLATORS ARE SUBJECT TO
SEVERE PENALTIES AND WILL BE
PROSECUTED TO THE FULL EXTENT
OF THE LAW GOVERNED BY THE
COUNTRY OF ORIGIN.
                 98/09/25 VER

This is from Fever SOS image! Both version strings are present!

The PCB is also different, UD's PCB does not have the Cave logo and
the CV01 marker in the lower left corner of the PCB.

There is some "engrish" story inside the UD image but this is NOT
present in the japanese images...

*/

ROM_START( feversos )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom1.bin", 0x000000, 0x080000, CRC(24ef3ce6) SHA1(42799eebbb2686a837b8972aec684143deadca59) )
	ROM_LOAD16_BYTE( "rom2.bin", 0x000001, 0x080000, CRC(64ff73fd) SHA1(7fc3a8469cec2361d373a4dac4a547c13ca5f709) )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "cv01-u25.bin", 0x000000, 0x400000, CRC(a6f6a95d) SHA1(e1eb45cb5d0e6163edfd9d830633b913fb53c6ca) )
	ROM_LOAD( "cv01-u26.bin", 0x400000, 0x400000, CRC(32edb62a) SHA1(3def74e1316b80cc25a8c3ac162cd7bcb8cc807c) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "cv01-u50.bin", 0x000000, 0x200000, CRC(7a344417) SHA1(828bd8f95d2fcc34407e17629ccafc904a4ea12d) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "cv01-u49.bin", 0x000000, 0x200000, CRC(d21cdda7) SHA1(cace4650de580c3c4a037f1f5c32bfc1846b383c) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "cv01-u19.bin", 0x000000, 0x400000, CRC(5f5514da) SHA1(53f27364aee544572a82649c9ff29bacc642b732) )
ROM_END

/***************************************************************************

                                Dodonpachi (Japan)

PCB:    AT-C03 D2
CPU:    MC68000-16
Sound:  YMZ280B
OSC:    28.0000MHz
        16.0000MHz
        16.9MHz (16.9344MHz?)

***************************************************************************/

ROM_START( ddonpach )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "b1.u27", 0x000000, 0x080000, CRC(b5cdc8d3) SHA1(58757b50e21a27e500a82c03f62cf02a85389926) )
	ROM_LOAD16_BYTE( "b2.u26", 0x000001, 0x080000, CRC(6bbb063a) SHA1(e5de64b9c3efc0a38a2e0e16b78ee393bff63558) )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2, do not dispose */
	ROM_LOAD( "u50.bin", 0x000000, 0x200000, CRC(14b260ec) SHA1(33bda210302428d5500115d0c7a839cdfcb67d17) )
	ROM_LOAD( "u51.bin", 0x200000, 0x200000, CRC(e7ba8cce) SHA1(ad74a6b7d53760b19587c4a6dbea937daa7e87ce) )
	ROM_LOAD( "u52.bin", 0x400000, 0x200000, CRC(02492ee0) SHA1(64d9cc64a4ad189a8b03cf6a749ddb732b4a0014) )
	ROM_LOAD( "u53.bin", 0x600000, 0x200000, CRC(cb4c10f0) SHA1(a622e8bd0c938b5d38b392b247400b744d8be288) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u60.bin", 0x000000, 0x200000, CRC(903096a7) SHA1(a243e903fef7c4a7b71383263e82e42acd869261) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u61.bin", 0x000000, 0x200000, CRC(d89b7631) SHA1(a66bb4955ca58fab8973ca37a0f971e9a67ce017) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u62.bin", 0x000000, 0x200000, CRC(292bfb6b) SHA1(11b385991ee990eb5ef36e136b988802b5f90fa4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u6.bin", 0x000000, 0x200000, CRC(9dfdafaf) SHA1(f5cb450cdc78a20c3a74c6dac05c9ac3cba08327) )
	ROM_LOAD( "u7.bin", 0x200000, 0x200000, CRC(795b17d5) SHA1(cbfc29f1df9600c82e0fdae00edd00da5b73e14c) )
ROM_END


ROM_START( ddonpchj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u27.bin", 0x000000, 0x080000, CRC(2432ff9b) SHA1(fbc826c30553f6553ead40b312b73c049e8f4bf6) )
	ROM_LOAD16_BYTE( "u26.bin", 0x000001, 0x080000, CRC(4f3a914a) SHA1(ae98eba049f1462aa1145f6959b9f9a32c97278f) )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2, do not dispose */
	ROM_LOAD( "u50.bin", 0x000000, 0x200000, CRC(14b260ec) SHA1(33bda210302428d5500115d0c7a839cdfcb67d17) )
	ROM_LOAD( "u51.bin", 0x200000, 0x200000, CRC(e7ba8cce) SHA1(ad74a6b7d53760b19587c4a6dbea937daa7e87ce) )
	ROM_LOAD( "u52.bin", 0x400000, 0x200000, CRC(02492ee0) SHA1(64d9cc64a4ad189a8b03cf6a749ddb732b4a0014) )
	ROM_LOAD( "u53.bin", 0x600000, 0x200000, CRC(cb4c10f0) SHA1(a622e8bd0c938b5d38b392b247400b744d8be288) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u60.bin", 0x000000, 0x200000, CRC(903096a7) SHA1(a243e903fef7c4a7b71383263e82e42acd869261) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u61.bin", 0x000000, 0x200000, CRC(d89b7631) SHA1(a66bb4955ca58fab8973ca37a0f971e9a67ce017) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u62.bin", 0x000000, 0x200000, CRC(292bfb6b) SHA1(11b385991ee990eb5ef36e136b988802b5f90fa4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u6.bin", 0x000000, 0x200000, CRC(9dfdafaf) SHA1(f5cb450cdc78a20c3a74c6dac05c9ac3cba08327) )
	ROM_LOAD( "u7.bin", 0x200000, 0x200000, CRC(795b17d5) SHA1(cbfc29f1df9600c82e0fdae00edd00da5b73e14c) )
ROM_END


/***************************************************************************

                                Donpachi

Known versions:

USA      Version 1.12 1995/05/2x
Korea    Version 1.12 1995/05/2x
Japan    Version 1.01 1995/05/11

BOARD #:      AT-C01DP-2
CPU:          TMP68HC000-16
VOICE:        M6295 x2
OSC:          28.000/16.000/4.220MHz
EEPROM:       ATMEL 93C46
CUSTOM:       ATLUS 8647-01 013
              038 9429WX727 x3
              NMK 112 (M6295 sample ROM banking)

---------------------------------------------------
 filenames          devices       kind
---------------------------------------------------
 PRG.U29            27C4096       68000 main prg.
 U58.BIN            27C020        gfx   data
 ATDP.U32           57C8200       M6295 data
 ATDP.U33           57C16200      M6295 data
 ATDP.U44           57C16200      gfx   data
 ATDP.U45           57C16200      gfx   data
 ATDP.U54           57C8200       gfx   data
 ATDP.U57           57C8200       gfx   data

 USA Version
----------------------------------------------------
 prgu.U29           27C4002       68000 Main Program
 text.u58           27C2001       Labeled as "TEXT"

***************************************************************************/

ROM_START( donpachi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "prgu.u29",     0x00000, 0x80000, CRC(89c36802) SHA1(7857c726cecca5a4fce282e0d2b873774d2c1b1d) )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "atdp.u44", 0x000000, 0x200000, CRC(7189e953) SHA1(53adbe6ea5e01ecb48575e9db82cc3d0dc8a3726) )
	ROM_LOAD( "atdp.u45", 0x200000, 0x200000, CRC(6984173f) SHA1(625dd6674adeb206815855b8b6a1fba79ed5c4cd) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "atdp.u54", 0x000000, 0x100000, CRC(6bda6b66) SHA1(6472e6706505bac17484fb8bf4e8922ced4adf63) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "atdp.u57", 0x000000, 0x100000, CRC(0a0e72b9) SHA1(997e8253777e7acca5a1c0c4026e78eecc122d5d) )

	ROM_REGION( 0x040000, REGION_GFX4, ROMREGION_DISPOSE )	/* Text / Character Layer */
	ROM_LOAD( "text.u58", 0x000000, 0x040000, CRC(5dba06e7) SHA1(f9dab7f6c732a683fddb4cae090a875b3962332b) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u33", 0x040000, 0x200000, CRC(d749de00) SHA1(64a0acc23eb2515e7d0459f0289919e083c63afc) )

	ROM_REGION( 0x340000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u32", 0x040000, 0x100000, CRC(0d89fcca) SHA1(e16ed15fa5e72537822f7b37e83ccfed0fa87338) )
	ROM_LOAD( "atdp.u33", 0x140000, 0x200000, CRC(d749de00) SHA1(64a0acc23eb2515e7d0459f0289919e083c63afc) )
ROM_END

ROM_START( donpachj )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "prg.u29",     0x00000, 0x80000, CRC(6be14af6) SHA1(5b1158071f160efeded816ae4c4edca1d00d6e05) )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "atdp.u44", 0x000000, 0x200000, CRC(7189e953) SHA1(53adbe6ea5e01ecb48575e9db82cc3d0dc8a3726) )
	ROM_LOAD( "atdp.u45", 0x200000, 0x200000, CRC(6984173f) SHA1(625dd6674adeb206815855b8b6a1fba79ed5c4cd) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "atdp.u54", 0x000000, 0x100000, CRC(6bda6b66) SHA1(6472e6706505bac17484fb8bf4e8922ced4adf63) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "atdp.u57", 0x000000, 0x100000, CRC(0a0e72b9) SHA1(997e8253777e7acca5a1c0c4026e78eecc122d5d) )

	ROM_REGION( 0x040000, REGION_GFX4, ROMREGION_DISPOSE )	/* Text / Character Layer */
	ROM_LOAD( "u58.bin", 0x000000, 0x040000, CRC(285379ff) SHA1(b9552edcec29ddf4b552800b145c398b94117ab0) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u33", 0x040000, 0x200000, CRC(d749de00) SHA1(64a0acc23eb2515e7d0459f0289919e083c63afc) )

	ROM_REGION( 0x340000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u32", 0x040000, 0x100000, CRC(0d89fcca) SHA1(e16ed15fa5e72537822f7b37e83ccfed0fa87338) )
	ROM_LOAD( "atdp.u33", 0x140000, 0x200000, CRC(d749de00) SHA1(64a0acc23eb2515e7d0459f0289919e083c63afc) )
ROM_END

ROM_START( donpachk )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "prgk.u26",    0x00000, 0x80000, CRC(bbaf4c8b) SHA1(0f9d42c8c4c5b69e3d39bf768bc4b663f66b4f36) )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "atdp.u44", 0x000000, 0x200000, CRC(7189e953) SHA1(53adbe6ea5e01ecb48575e9db82cc3d0dc8a3726) )
	ROM_LOAD( "atdp.u45", 0x200000, 0x200000, CRC(6984173f) SHA1(625dd6674adeb206815855b8b6a1fba79ed5c4cd) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "atdp.u54", 0x000000, 0x100000, CRC(6bda6b66) SHA1(6472e6706505bac17484fb8bf4e8922ced4adf63) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "atdp.u57", 0x000000, 0x100000, CRC(0a0e72b9) SHA1(997e8253777e7acca5a1c0c4026e78eecc122d5d) )

	ROM_REGION( 0x040000, REGION_GFX4, ROMREGION_DISPOSE )	/* Text / Character Layer */
	ROM_LOAD( "u58.bin", 0x000000, 0x040000, CRC(285379ff) SHA1(b9552edcec29ddf4b552800b145c398b94117ab0) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u33", 0x040000, 0x200000, CRC(d749de00) SHA1(64a0acc23eb2515e7d0459f0289919e083c63afc) )

	ROM_REGION( 0x340000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "atdp.u32", 0x040000, 0x100000, CRC(0d89fcca) SHA1(e16ed15fa5e72537822f7b37e83ccfed0fa87338) )
	ROM_LOAD( "atdp.u33", 0x140000, 0x200000, CRC(d749de00) SHA1(64a0acc23eb2515e7d0459f0289919e083c63afc) )
ROM_END


/***************************************************************************

                                    Esprade

ATC04
OSC:    28.0, 16.0, 16.9 MHz

***************************************************************************/

ROM_START( esprade )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u42_i.bin", 0x000000, 0x080000, CRC(3b510a73) SHA1(ab1666eb826cb4a71588d86831dd18a2ef1c2a33) )
	ROM_LOAD16_BYTE( "u41_i.bin", 0x000001, 0x080000, CRC(97c1b649) SHA1(37a56b7b9662219a356aee3f4b5cbb774ac4950e) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u63.bin", 0x000000, 0x400000, CRC(2f2fe92c) SHA1(9519e365248bcec8419786eabb16fe4aae299af5) )
	ROM_LOAD16_BYTE( "u64.bin", 0x000001, 0x400000, CRC(491a3da4) SHA1(53549a2bd3edc7b5e73fb46e1421b156bb0c190f) )
	ROM_LOAD16_BYTE( "u65.bin", 0x800000, 0x400000, CRC(06563efe) SHA1(94e72da1f542b4e0525b4b43994242816b43dbdc) )
	ROM_LOAD16_BYTE( "u66.bin", 0x800001, 0x400000, CRC(7bbe4cfc) SHA1(e77d0ed7a11b5abca1df8a0eb20ac9360cf79e76) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u54.bin", 0x000000, 0x400000, CRC(e7ca6936) SHA1(b7f5ab67071a1d9dd3d2c1cd2304d9cdad68850c) )
	ROM_LOAD( "u55.bin", 0x400000, 0x400000, CRC(f53bd94f) SHA1(d0a74fb3d36fe522ef075e5ae44a9980da8abe2f) )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u52.bin", 0x000000, 0x400000, CRC(e7abe7b4) SHA1(e98da45497e1aaf0d6ab352ec3e43c7438ed792a) )
	ROM_LOAD( "u53.bin", 0x400000, 0x400000, CRC(51a0f391) SHA1(8b7355cbad119f4e1add14e5cd5e343ec6706104) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u51.bin", 0x000000, 0x400000, CRC(0b9b875c) SHA1(ef05447cd8565ae24bb71db42342724622ad1e3e) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u19.bin", 0x000000, 0x400000, CRC(f54b1cab) SHA1(34d70bb5798de85d892c062001d9ac1d6604fd9f) )
ROM_END

ROM_START( espradej )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u42_ver2.bin", 0x000000, 0x080000, CRC(75d03c42) SHA1(1c176185b6f1531752b633a97f705ffa0cfeb5ad) )
	ROM_LOAD16_BYTE( "u41_ver2.bin", 0x000001, 0x080000, CRC(734b3ef0) SHA1(f584227b85c347d62d5f179445011ce0f607bcfd) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u63.bin", 0x000000, 0x400000, CRC(2f2fe92c) SHA1(9519e365248bcec8419786eabb16fe4aae299af5) )
	ROM_LOAD16_BYTE( "u64.bin", 0x000001, 0x400000, CRC(491a3da4) SHA1(53549a2bd3edc7b5e73fb46e1421b156bb0c190f) )
	ROM_LOAD16_BYTE( "u65.bin", 0x800000, 0x400000, CRC(06563efe) SHA1(94e72da1f542b4e0525b4b43994242816b43dbdc) )
	ROM_LOAD16_BYTE( "u66.bin", 0x800001, 0x400000, CRC(7bbe4cfc) SHA1(e77d0ed7a11b5abca1df8a0eb20ac9360cf79e76) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u54.bin", 0x000000, 0x400000, CRC(e7ca6936) SHA1(b7f5ab67071a1d9dd3d2c1cd2304d9cdad68850c) )
	ROM_LOAD( "u55.bin", 0x400000, 0x400000, CRC(f53bd94f) SHA1(d0a74fb3d36fe522ef075e5ae44a9980da8abe2f) )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u52.bin", 0x000000, 0x400000, CRC(e7abe7b4) SHA1(e98da45497e1aaf0d6ab352ec3e43c7438ed792a) )
	ROM_LOAD( "u53.bin", 0x400000, 0x400000, CRC(51a0f391) SHA1(8b7355cbad119f4e1add14e5cd5e343ec6706104) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u51.bin", 0x000000, 0x400000, CRC(0b9b875c) SHA1(ef05447cd8565ae24bb71db42342724622ad1e3e) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u19.bin", 0x000000, 0x400000, CRC(f54b1cab) SHA1(34d70bb5798de85d892c062001d9ac1d6604fd9f) )
ROM_END

ROM_START( espradeo )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u42.bin", 0x000000, 0x080000, CRC(0718c7e5) SHA1(c7d1f30bd2ef363cad15b6918f9980312a15809a) )
	ROM_LOAD16_BYTE( "u41.bin", 0x000001, 0x080000, CRC(def30539) SHA1(957ad0b06f06689ae71393572592f6b8f818603a) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u63.bin", 0x000000, 0x400000, CRC(2f2fe92c) SHA1(9519e365248bcec8419786eabb16fe4aae299af5) )
	ROM_LOAD16_BYTE( "u64.bin", 0x000001, 0x400000, CRC(491a3da4) SHA1(53549a2bd3edc7b5e73fb46e1421b156bb0c190f) )
	ROM_LOAD16_BYTE( "u65.bin", 0x800000, 0x400000, CRC(06563efe) SHA1(94e72da1f542b4e0525b4b43994242816b43dbdc) )
	ROM_LOAD16_BYTE( "u66.bin", 0x800001, 0x400000, CRC(7bbe4cfc) SHA1(e77d0ed7a11b5abca1df8a0eb20ac9360cf79e76) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u54.bin", 0x000000, 0x400000, CRC(e7ca6936) SHA1(b7f5ab67071a1d9dd3d2c1cd2304d9cdad68850c) )
	ROM_LOAD( "u55.bin", 0x400000, 0x400000, CRC(f53bd94f) SHA1(d0a74fb3d36fe522ef075e5ae44a9980da8abe2f) )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u52.bin", 0x000000, 0x400000, CRC(e7abe7b4) SHA1(e98da45497e1aaf0d6ab352ec3e43c7438ed792a) )
	ROM_LOAD( "u53.bin", 0x400000, 0x400000, CRC(51a0f391) SHA1(8b7355cbad119f4e1add14e5cd5e343ec6706104) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u51.bin", 0x000000, 0x400000, CRC(0b9b875c) SHA1(ef05447cd8565ae24bb71db42342724622ad1e3e) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u19.bin", 0x000000, 0x400000, CRC(f54b1cab) SHA1(34d70bb5798de85d892c062001d9ac1d6604fd9f) )
ROM_END


/***************************************************************************

                                Gaia Crusaders

Noise Factory, 1999

PCB Layout
----------

|------------------------------------------------|
|   YAC516    YMZ280B      XC9536      68000     |
|          16MHz                       PRG2   PAL|
|                          TC51832     PRG1      |
|     SND3     SND2        TC51832   28.322MHz   |
|              SND1        62256     16MHz       |
|                          62256                 |
|J 62256 62256 62256 62256 62256 62256           |
|A                                 KM416C256     |
|M                                      KM416C256|
|M     -------------------  ---------------      |
|A     |     |     |     |  |             | 62256|
|      |     |     |     |  |             |      |
| DSW1 |     |     |     |  |013 9918EX008| 62256|
|      |038 9838WX003(x3)|  |             |      |
|      -------------------  ---------------      |
| DSW2                                           |
|                    XC9536          OBJ2        |
|                                                |
|       BG2     BG3    BG1           OBJ1        |
|                                                |
|------------------------------------------------|

Notes:
      68000 clock  : 16.000MHz
      YMZ280B clock: 16.000MHz
      VSync        : 58Hz
      HSync        : 15.40kHz

***************************************************************************/

ROM_START( gaia )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "prg1.127", 0x000000, 0x080000, CRC(47b904b2) SHA1(58b9b55f59cf00f70b690a0371096e86f4d723c2) )
	ROM_LOAD16_BYTE( "prg2.128", 0x000001, 0x080000, CRC(469b7794) SHA1(502f855c51005a866900b19c3a0a170d9ea02392) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )  /* Sprites (do not dispose) */
	ROM_LOAD( "obj1.736", 0x000000, 0x400000, CRC(f4f84e5d) SHA1(8f445dd7a5c8a996939c211e5aec5742121a6e7e) )
	ROM_LOAD( "obj2.738", 0x400000, 0x400000, CRC(15c2a9ce) SHA1(631eb2968395be86ef2403733e7d4ec769a013b9) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bg1.989", 0x000000, 0x400000, CRC(013a693d) SHA1(2cc5be6f47c13febed942e1c3167946efedc5f9b) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bg2.995", 0x000000, 0x400000, CRC(783cc62f) SHA1(8b6e4212688b53be5ecc29ff2d41fd43e7d0a420) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "bg3.998", 0x000000, 0x400000, CRC(bcd61d1c) SHA1(660a3b02a8c39e1117b00d0ad06f73221fef4ce8) )

	ROM_REGION( 0xc00000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "snd1.447", 0x000000, 0x400000, CRC(92770a52) SHA1(81f6835e1b45eb0f367e4586fdda92466f02edb9) )
	ROM_LOAD( "snd2.454", 0x400000, 0x400000, CRC(329ae1cf) SHA1(0c5e5074a5d8f4fb85ab4893bc953f192dcb301a) )
	ROM_LOAD( "snd3.455", 0x800000, 0x400000, CRC(4048d64e) SHA1(5e4ec6d37e70484e2fcd04188385e79ef0b53026) )
ROM_END


/***************************************************************************

                                Guwange (Japan)

PCB:    ATC05
CPU:    MC68000-16
Sound:  YMZ280B
OSC:    28.0000MHz
        16.0000MHz
        16.9MHz

***************************************************************************/

ROM_START( guwange )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "gu-u0127.bin", 0x000000, 0x080000, CRC(f86b5293) SHA1(f8b1cd77cc25328d5010889850e4b86c27d9e396) )
	ROM_LOAD16_BYTE( "gu-u0129.bin", 0x000001, 0x080000, CRC(6c0e3b93) SHA1(aaad6569b9a7b6f9a315062f9fedfc95851c1bc6) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD16_BYTE( "u083.bin", 0x0000000, 0x800000, CRC(adc4b9c4) SHA1(3f9fb004e19187bbfa87ddfe8cfc69740656a1bd) )
	ROM_LOAD16_BYTE( "u082.bin", 0x0000001, 0x800000, CRC(3d75876c) SHA1(705b8c2dbdc31e9516f429969f87988beec796d7) )
	ROM_LOAD16_BYTE( "u086.bin", 0x1000000, 0x400000, CRC(188e4f81) SHA1(626074d81782a6de0b52406331b4b8561d3e36f5) )
	ROM_RELOAD(                  0x1800000, 0x400000 )
	ROM_LOAD16_BYTE( "u085.bin", 0x1000001, 0x400000, CRC(a7d5659e) SHA1(10abac022ebe106a3ca7186ff18ca2757f903033) )
	ROM_RELOAD(                  0x1800001, 0x400000 )
//sprite bug fix?
//  ROM_FILL(                    0x1800000, 0x800000, 0xff )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u101.bin", 0x000000, 0x800000, CRC(0369491f) SHA1(ca6b1345506f13a17c9bace01637d1f61a278644) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "u10102.bin", 0x000000, 0x400000, CRC(e28d6855) SHA1(7001a6e298c6a1fcceb79586bf5f4bf0f30027f6) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "u10103.bin", 0x000000, 0x400000, CRC(0fe91b8e) SHA1(8b71ebeef5e4d2b00fdaaab97776d74e1c96dc59) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u0462.bin", 0x000000, 0x400000, CRC(b3d75691) SHA1(71d8dae92be1542a3cff50efeec0bf3c14ab59f5) )
ROM_END


/***************************************************************************

                                Hot Dog Storm
Marble 1996

6264 6264 MP7 6264 6264 MP6 6264 6264 MP5  32MHz
                                                6264
                                                6264
                                                  MP4
                                                  MP3
                                                       93C46
                                      68257
                                      68257 68000-12
                                             YM2203
                                          Z80
  MP8 MP9
  68257
  68257                         U19    MP1      6296

***************************************************************************/

ROM_START( hotdogst )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "mp3u29", 0x00000, 0x80000, CRC(1f4e5479) SHA1(5c3d7b36b1eda4c87c53e4f7cf89951cc5bcc871) )
	ROM_LOAD16_BYTE( "mp4u28", 0x00001, 0x80000, CRC(6f1c3c4b) SHA1(ab4e4d9b2ef74a2eefda718e120bef05fd0346ff) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "mp2u19", 0x00000, 0x08000, CRC(ff979ebe) SHA1(4cb80086cfdc69a321c7f75455cef89e20488b76) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(       0x10000, 0x38000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "mp9u55", 0x000000, 0x200000, CRC(258d49ec) SHA1(f39e30c82d8f680f248e1eb59d7c5acb479fa277) )
	ROM_LOAD( "mp8u54", 0x200000, 0x200000, CRC(bdb4d7b8) SHA1(0dd490988aa84b0e9a21ade5fd606b03eca13f6c) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "mp7u56", 0x00000, 0x80000, CRC(87c21c50) SHA1(fc0eea79abdd96edb4fa2c7047aaa728ef838234) )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "mp6u61", 0x00000, 0x80000, CRC(4dafb288) SHA1(4756259adfe49ba42cde25e7902655b0f0731a6c) )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "mp5u64", 0x00000, 0x80000, CRC(9b26458c) SHA1(acef62422fa3f92e6ca1eba0ee6fb914cd1ee190) )

	ROM_REGION( 0xc0000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "mp1u65", 0x40000, 0x80000, CRC(4868be1b) SHA1(32b8234b19fdbe07fa5057fa7965e36807e35e77) )	// 1xxxxxxxxxxxxxxxxxx = 0xFF, 4 x 0x20000
ROM_END


/***************************************************************************

Koro Koro Quest

Hardware is kind of Banpresto-ish

 PCB Number - TUG-01B MP001-00175
 68000-16 + 16MHZ OSC
 YMZ280B + YAC516-M + Xtal 16.9344MHz
 93C46 EEPROM
 Custom - 9838EX004 (QFP240), 9838WX004 (QFP144) + OSC 28MHz
 RAM - 62256 (x8), M5M44260 (x2)
 3volt battery
 GAL16V8H (x5)

***************************************************************************/

ROM_START( korokoro )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "mp-001_ver07.u0130", 0x000000, 0x080000, CRC(86c7241f) SHA1(c9f0ab63c4fe36df1300445e9bb0d5c6a1bb733f) ) // 1xxxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x180000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "mp-001_ver01.u1066", 0x000000, 0x100000, CRC(c5c6af7e) SHA1(13ac26fd703672a01d629be4e5efe9fb8720a4fb) )
	ROM_LOAD( "mp-001_ver01.u1051", 0x100000, 0x080000, CRC(fe5e28e8) SHA1(44da1a7d813b149f9bae351bbcbd0bc2d4c70e10) )	// 1xxxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "mp-001_ver01.u1060", 0x000000, 0x100000, CRC(ec9cf9d8) SHA1(32fa7120e30c14e484de3b3a9c93efe3654d43c8) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "mp-001_ver01.u1186", 0x000000, 0x100000, CRC(d16e7c5d) SHA1(1f825ace3ed2e23c8d3212320c4645d3d52214c7) )
ROM_END


/***************************************************************************

                                Mazinger Z

Banpresto 1994

U63               038               62256
                  9335EX706         62256
3664                            62256  62256
3664                                U924      32MHz
                                    U24
U60               038             68000
                  9335EX706
3664                                U21   YM2203  92E422
3664                                Z80
                                    3664
                  013
                  9341E7009
U56
U55

62256 62256      514260  514260     U64         M6295

***************************************************************************/

ROM_START( mazinger )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mzp-0.u24", 0x00000, 0x80000, CRC(43a4279f) SHA1(2c17eb31040bb7f1554bc1c9a968eec5e72af097) )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )		/* 68000 code (mapped at d00000) */
	ROM_LOAD16_WORD_SWAP( "mzp-1.924", 0x00000, 0x80000, CRC(db40acba) SHA1(797a3046b6ab33773c5c4d6bb6d045ea60c1eb45) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "mzs.u21", 0x00000, 0x08000, CRC(c5b4f7ed) SHA1(01f3cd1dd4045029260544e0e1c15dd08817012e) )
	ROM_CONTINUE(        0x10000, 0x18000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, ROMREGION_ERASEFF )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "bp943a-2.u56", 0x000000, 0x200000, CRC(97e13959) SHA1(c30b1093aacebafefcae701af767dd36fc55fac7) )
	ROM_LOAD( "bp943a-3.u55", 0x200000, 0x080000, CRC(9c4957dd) SHA1(e775605a01b6cadc318855ac046dad03c4fc5bb4) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bp943a-1.u60", 0x000000, 0x200000, CRC(46327415) SHA1(679d26caefa975569198fac550105c370e2be00d) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bp943a-0.u63", 0x000000, 0x200000, CRC(c1fed98a) SHA1(c276505f80a49b129862966a19db507f97153e45) )	// FIXED BITS (xxxxxxxx00000000)

	ROM_REGION( 0x0c0000, REGION_SOUND1, 0 )	/* Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp943a-4.u64", 0x040000, 0x080000, CRC(3fc7f29a) SHA1(feb21b918243c0a03dfa4a80cc80b86be4f62680) )	// 4 x $20000
ROM_END


/***************************************************************************

                                Metamoqester

[Ninja Master (World version)?]
(C) 1995 Banpresto

PCB: BP947A
CPU: MC68HC000P16 (68000, 64 pin DIP)
SND: Z0840008PSC (Z80, 40 pin DIP), AD-65 x 2 (= OKI M6295), YM2151, CY5002 (= YM3012)
OSC: 32.000 MHz
RAM: LGS GM76C88ALFW-15 x 9 (28 pin SOP), LGS GM71C4260AJ70 x 2 (40 pin SOJ)
     Hitachi HM62256LFP-12T x 2 (40 pin SOJ)

Other Chips:
AT93C46 (EEPROM)
PAL (not dumped, located near 68000): ATF16V8 x 1

GFX:  (Same GFX chips as "Sailor Moon")

      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      013 9346E7002 (240 pin PQFP)

On PCB near JAMMA connector is a small push button labelled SW1 to access test mode.

ROMS:
BP947A.U37  16M Mask    \ Oki Samples
BP947A.U42  16M Mask    /

BP947A.U46  16M Mask    \
BP947A.U47  16M Mask    |
BP947A.U48  16M Mask    |
BP947A.U49  16M Mask    | GFX
BP947A.U50  16M Mask    |
BP947A.U51  16M Mask    |
BP947A.U52  16M Mask    /

BP947A.U20  27C020        Sound PRG

BP947A.U25  27C240      \
BP947A.U28  27C240      | Main PRG
BP947A.U29  27C240      /

***************************************************************************/

ROM_START( metmqstr )
	ROM_REGION( 0x280000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "bp947a.u25", 0x000000, 0x80000, CRC(0a5c3442) SHA1(684b79912dedc103f45c42fdebf9983e091b1308) )
	ROM_LOAD16_WORD_SWAP( "bp947a.u28", 0x100000, 0x80000, CRC(8c55decf) SHA1(76c6ce4c8e621273258d31ceb9ec4442fcf1a393) )
	ROM_LOAD16_WORD_SWAP( "bp947a.u29", 0x200000, 0x80000, CRC(cf0f3f3b) SHA1(49a3c0e7536edd53bbf09353e43e9166d736b3f4) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "bp947a.u20",  0x00000, 0x08000, CRC(a4a36170) SHA1(ae55094518bd968ea0d04613a133c1421e412012) )
	ROM_CONTINUE(            0x10000, 0x38000             )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bp947a.u49", 0x000000, 0x200000, CRC(09749531) SHA1(6deeed2712241611ec3202c49a66beed28698af8) )
	ROM_LOAD( "bp947a.u50", 0x200000, 0x200000, CRC(19cea8b2) SHA1(87fb29458074f0e4852237e0184b8b3b44b0eb29) )
	ROM_LOAD( "bp947a.u51", 0x400000, 0x200000, CRC(c19bed67) SHA1(ac664a15512c0e8c8b701833aede95f53cd46a45) )
	ROM_LOAD( "bp947a.u52", 0x600000, 0x200000, CRC(70c64875) SHA1(1c20ab100ccfdf42c97a25e4deb9041b83f5ca8d) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bp947a.u48", 0x000000, 0x100000, CRC(04ff6a3d) SHA1(7187db436f7a2ab59a3f5c6ab297b3d740e20f1d) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bp947a.u47", 0x000000, 0x100000, CRC(0de42827) SHA1(05d452ca11a31f941cb8a9b0cbb0b59c6b0cbdcb) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "bp947a.u46", 0x000000, 0x100000, CRC(0f9c906e) SHA1(03872e8be28637df66373bddb04ed91de4f9db75) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp947a.u42", 0x040000, 0x100000, CRC(2ce8ff2a) SHA1(8ef8c5b7d4a0e60c980c2962e75f7977faafa311) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x040000, 0x100000             )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp947a.u37", 0x040000, 0x100000, CRC(c3077c8f) SHA1(0a76316a81b7de78279b859549eb5161a721ac71) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x040000, 0x100000             )
ROM_END

ROM_START( nmaster )
	ROM_REGION( 0x280000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "bp947a_n.u25",0x000000, 0x80000, CRC(748cc514) SHA1(11d882e77a539407c314f087386e50d691a6bc0b) )
	ROM_LOAD16_WORD_SWAP( "bp947a.u28" , 0x100000, 0x80000, CRC(8c55decf) SHA1(76c6ce4c8e621273258d31ceb9ec4442fcf1a393) )
	ROM_LOAD16_WORD_SWAP( "bp947a.u29",  0x200000, 0x80000, CRC(cf0f3f3b) SHA1(49a3c0e7536edd53bbf09353e43e9166d736b3f4) )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "bp947a.u20",  0x00000, 0x08000, CRC(a4a36170) SHA1(ae55094518bd968ea0d04613a133c1421e412012) )
	ROM_CONTINUE(            0x10000, 0x38000             )

	ROM_REGION( 0x800000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bp947a.u49", 0x000000, 0x200000, CRC(09749531) SHA1(6deeed2712241611ec3202c49a66beed28698af8) )
	ROM_LOAD( "bp947a.u50", 0x200000, 0x200000, CRC(19cea8b2) SHA1(87fb29458074f0e4852237e0184b8b3b44b0eb29) )
	ROM_LOAD( "bp947a.u51", 0x400000, 0x200000, CRC(c19bed67) SHA1(ac664a15512c0e8c8b701833aede95f53cd46a45) )
	ROM_LOAD( "bp947a.u52", 0x600000, 0x200000, CRC(70c64875) SHA1(1c20ab100ccfdf42c97a25e4deb9041b83f5ca8d) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bp947a.u48", 0x000000, 0x100000, CRC(04ff6a3d) SHA1(7187db436f7a2ab59a3f5c6ab297b3d740e20f1d) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bp947a.u47", 0x000000, 0x100000, CRC(0de42827) SHA1(05d452ca11a31f941cb8a9b0cbb0b59c6b0cbdcb) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "bp947a.u46", 0x000000, 0x100000, CRC(0f9c906e) SHA1(03872e8be28637df66373bddb04ed91de4f9db75) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x000000, 0x100000             )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp947a.u42", 0x040000, 0x100000, CRC(2ce8ff2a) SHA1(8ef8c5b7d4a0e60c980c2962e75f7977faafa311) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x040000, 0x100000             )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bp947a.u37", 0x040000, 0x100000, CRC(c3077c8f) SHA1(0a76316a81b7de78279b859549eb5161a721ac71) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(           0x040000, 0x100000             )
ROM_END


/***************************************************************************

                            Power Instinct 2

(c)1994 Atlus
CPU: 68000, Z80
Sound: YM2203, AR17961 (x2)
Custom: NMK 112 (M6295 sample ROM banking), Atlus 8647-01  013, 038 (x4)
X1 = 12 MHz
X2 = 28 MHz
X3 = 16 MHz

***************************************************************************/

ROM_START( pwrinst2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "g02.u45", 0x000000, 0x80000, CRC(7b33bc43) SHA1(a68eb94e679f03c354932b8c5cd1bb2922fec0aa) )
	ROM_LOAD16_BYTE( "g02.u44", 0x000001, 0x80000, CRC(8f6f6637) SHA1(024b12c0fe40e27c79e38bd7601a9183a62d75fd) )
	ROM_LOAD16_BYTE( "g02.u43", 0x100000, 0x80000, CRC(178e3d24) SHA1(926234f4196a5d5e3bd1438abbf73355f2c65b06) )
	ROM_LOAD16_BYTE( "g02.u42", 0x100001, 0x80000, CRC(a0b4ee99) SHA1(c6df4aa2543b04d8bda7683f503e5eb763e506af) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, ROMREGION_ERASE00 )	/* 68000 extra data roms */
	/* not used */

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "g02.u3a", 0x00000, 0x0c000, CRC(ebea5e1e) SHA1(4d3af9e5f29d0c1b26563f51250039c9e8bd3735) )
	ROM_CONTINUE(        0x10000, 0x14000             )

	ROM_REGION( 0xe00000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "g02.u61", 0x000000, 0x200000, CRC(91e30398) SHA1(2b59a5e40bed2a988382054fe30d92808dad3348) )
	ROM_LOAD( "g02.u62", 0x200000, 0x200000, CRC(d9455dd7) SHA1(afa69fe9a540cd78b8cfecf09cffa1401c01141a) )
	ROM_LOAD( "g02.u63", 0x400000, 0x200000, CRC(4d20560b) SHA1(ceaee8cf0b69cc366b95ddcb689a5594d79e5114) )
	ROM_LOAD( "g02.u64", 0x600000, 0x200000, CRC(b17b9b6e) SHA1(fc6213d8322cda4c7f653e2d7d6d314ce84c97b7) )
	ROM_LOAD( "g02.u65", 0x800000, 0x200000, CRC(08541878) SHA1(138cf077a49a26440a3da1bdc2c399a208359e57) )
	ROM_LOAD( "g02.u66", 0xa00000, 0x200000, CRC(becf2a36) SHA1(f8b386d0292b1dc745b7253a3df51d1aa8d5e9db) )
	ROM_LOAD( "g02.u67", 0xc00000, 0x200000, CRC(52fe2b8b) SHA1(dd50aa62f7db995e28f47de9b3fb749aeeaaa5b0) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "g02.u78", 0x000000, 0x200000, CRC(1eca63d2) SHA1(538942b43301f950e3d5139461331c54dc90129d) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "g02.u81", 0x000000, 0x100000, CRC(8a3ff685) SHA1(4a59ec50ec4470453374fe10f76d3e894494b49f) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "g02.u89", 0x000000, 0x100000, CRC(373e1f73) SHA1(ec1ae9fab37eee41be8e1bc6dad03809b62fdbce) )

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* Layer 3 */
	ROM_LOAD( "g02.82a", 0x000000, 0x080000, CRC(4b3567d6) SHA1(d3e14783b312d2bea9722a8e3c22bcec81e26166) )

	ROM_REGION( 0x440000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u53", 0x040000, 0x200000, CRC(c4bdd9e0) SHA1(a938a831e789ddf6f3cc5f3e5f3877ec7bd62d4e) )
	ROM_LOAD( "g02.u54", 0x240000, 0x200000, CRC(1357d50e) SHA1(433766177ce9d6933f90de85ba91bfc6d8d5d664) )

	ROM_REGION( 0x440000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u55", 0x040000, 0x200000, CRC(2d102898) SHA1(bd81f4cd2ba100707db0c5bb1419f0b23c998574) )
	ROM_LOAD( "g02.u56", 0x240000, 0x200000, CRC(9ff50dda) SHA1(1121685e387c20e228032f2b0f5cbb606376fc15) )
ROM_END

ROM_START( pwrins2j )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "g02j.u45", 0x000000, 0x80000, CRC(42d0abd7) SHA1(c58861d43c4539ccc8b2f93eabc56aab37d3aa34))
	ROM_LOAD16_BYTE( "g02j.u44", 0x000001, 0x80000, CRC(362b7af3) SHA1(2d15611530cef76f0f9c82ee0411966079ae19c3))
	ROM_LOAD16_BYTE( "g02j.u43", 0x100000, 0x80000, CRC(c94c596b) SHA1(ee755a344f769e3ed05d8ca57f517b9e8c02f22e) )
	ROM_LOAD16_BYTE( "g02j.u42", 0x100001, 0x80000, CRC(4f4c8270) SHA1(1fa964f5646bd1d078e3661c21e191b0789c05c9) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, ROMREGION_ERASE00 )	/* 68000 extra data roms */
	/* not used */

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "g02j.u3a", 0x00000, 0x0c000, CRC(eead01f1) SHA1(0ced6755e471e0303fe397b3d54a5c799762ebd8) )
	ROM_CONTINUE(        0x10000, 0x14000             )

	ROM_REGION( 0xe00000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "g02.u61", 0x000000, 0x200000, CRC(91e30398) SHA1(2b59a5e40bed2a988382054fe30d92808dad3348) )
	ROM_LOAD( "g02.u62", 0x200000, 0x200000, CRC(d9455dd7) SHA1(afa69fe9a540cd78b8cfecf09cffa1401c01141a) )
	ROM_LOAD( "g02.u63", 0x400000, 0x200000, CRC(4d20560b) SHA1(ceaee8cf0b69cc366b95ddcb689a5594d79e5114) )
	ROM_LOAD( "g02.u64", 0x600000, 0x200000, CRC(b17b9b6e) SHA1(fc6213d8322cda4c7f653e2d7d6d314ce84c97b7) )
	ROM_LOAD( "g02.u65", 0x800000, 0x200000, CRC(08541878) SHA1(138cf077a49a26440a3da1bdc2c399a208359e57) )
	ROM_LOAD( "g02.u66", 0xa00000, 0x200000, CRC(becf2a36) SHA1(f8b386d0292b1dc745b7253a3df51d1aa8d5e9db) )
	ROM_LOAD( "g02.u67", 0xc00000, 0x200000, CRC(52fe2b8b) SHA1(dd50aa62f7db995e28f47de9b3fb749aeeaaa5b0) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "g02.u78", 0x000000, 0x200000, CRC(1eca63d2) SHA1(538942b43301f950e3d5139461331c54dc90129d) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "g02.u81", 0x000000, 0x100000, CRC(8a3ff685) SHA1(4a59ec50ec4470453374fe10f76d3e894494b49f) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "g02.u89", 0x000000, 0x100000, CRC(373e1f73) SHA1(ec1ae9fab37eee41be8e1bc6dad03809b62fdbce) )

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* Layer 3 */
	ROM_LOAD( "g02j.82a", 0x000000, 0x080000, CRC(3be86fe1) SHA1(313bfe5fb8dc5fee4462db259738e079759f9390) )

	ROM_REGION( 0x440000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u53", 0x040000, 0x200000, CRC(c4bdd9e0) SHA1(a938a831e789ddf6f3cc5f3e5f3877ec7bd62d4e) )
	ROM_LOAD( "g02.u54", 0x240000, 0x200000, CRC(1357d50e) SHA1(433766177ce9d6933f90de85ba91bfc6d8d5d664) )

	ROM_REGION( 0x440000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u55", 0x040000, 0x200000, CRC(2d102898) SHA1(bd81f4cd2ba100707db0c5bb1419f0b23c998574) )
	ROM_LOAD( "g02.u56", 0x240000, 0x200000, CRC(9ff50dda) SHA1(1121685e387c20e228032f2b0f5cbb606376fc15) )
ROM_END

/*

Power Instinct Legends (US)
Gouketsuji Ichizoku Saikyou Densetsu (Japan)
Atlus, 1995

PCB Layout
----------

AT047G2-B ATLUS
|---------------------------------------------------------------|
|LM324 M6295  G02_U53          Z80  16MHz 28MHz 12MHz  TA8030S  |
|VOL          G02_U54 |------| SOUND_U3                TEXT_U82 |
|      M6295  G02_U55 |NMK112|   6264         6264              |
|uPC2505      G02_U56 |      |                        |------|  |
|      4558           |------|                6264    |038   |  |
|     Y3014   YM2203    PAL                           |9429WX|  |
|                                            ATGS_U89 |------|  |
|J       TEST_SW  62256                                         |
|A    93C46       62256                       6264    |------|  |
|M            |----SUB-BOARD-----|                    |038   |  |
|M    |---|   |*P  P *P  P *P *P |      PAL   6264    |9429WX|  |
|A    |   |   | R  R  R  R  R  R |                    |------|  |
|     | 6 |   | 1  O  1  O  1  1 |           ATGS_U81           |
|     | 8 |   | 2  G  2  G  2  2 |                    |------|  |
|     | 0 |   | U  U  U  U  U  U |            6264    |038   |  |
|     | 0 |   | 2  4  4  4  3  5 |                    |9429WX|  |
|     | 0 |   |    5     4       |62256       6264    |------|  |
|     |   |   |------------------|62256                         |
|     |---|     PAL            |-------|     ATGS_U78 |------|  |
|--------|                     |8647-01|              |038   |  |
|*ATGS_U1|                     |013    |    KM416C256 |9429WX|  |
|        |                     |9341E70|              |------|  |
|        |G02_U66    G02_U63   |-------|                6264    |
|        |  G02_U65    G02_U62   62256      KM416C256           |
|*ATGS_U2|    G02_U64    G02_U61 62256                  6264    |
|--------|------------------------------------------------------|
Notes:
      ROMs marked with * are located on a plug-in sub board
      68000 clock - 16.000MHz
      Z80 clock   - 8.000MHz [16/2]
      6295 clocks - 3.000MHz [12/4], sample rate = 3000000 / 165
      YM2203 clock- 4.000MHz [16/4]
      VSync       - 57.5Hz
      HSync       - 15.23kHz

      ROMs -
            U3       : 27C1001 EPROM
            U82      : 27C040 EPROM
            PR12*    : 27C040 EPROMs
            PROG*    : 27C040 EPROMs
            ALL other ROMs are soldered-in 16M 42 pin MASKROM (read as 27C160)
*/

ROM_START( plegends )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "d12.u45", 0x000000, 0x80000, CRC(ed8a2e3d) SHA1(0a09c58cd8a726189cd7679d06343e0b8c3de945) )
	ROM_LOAD16_BYTE( "d13.u44", 0x000001, 0x80000, CRC(25821731) SHA1(7c6ece92b36dc7eb489879d9ae3e8af9380b9f62) )
	ROM_LOAD16_BYTE( "d14.u2",  0x100000, 0x80000, CRC(c2cb1402) SHA1(78e70915ca32b97c22605a304dc8611e1fe01ae9) ) /* Contains text strings */
	ROM_LOAD16_BYTE( "d16.u3",  0x100001, 0x80000, CRC(50a1c63e) SHA1(5a8431a81aa61034e67141944b9e7cf97842773a) ) /* Contains text strings */

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )	/* 68000 extra data roms */
	ROM_LOAD16_BYTE( "d15.u4",  0x000000, 0x80000, CRC(6352cec0) SHA1(a54d55b8d642e438158268d0d41880b6589e48e2) )
	ROM_LOAD16_BYTE( "d17.u5",  0x000001, 0x80000, CRC(7af810d8) SHA1(5e24f78a228809a001f3f3372c1b32ea05070e17) )

	ROM_REGION( 0x44000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "d19.u3", 0x00000, 0x0c000, CRC(47598459) SHA1(4e9dcfebfbd160230768965e8c6e5ed446c1aa7b) ) /* Same as sound.u3 below, but twice the size? */
	ROM_CONTINUE(        0x10000, 0x34000             )

	ROM_REGION( 0x1000000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "g02.u61", 0x000000, 0x200000, CRC(91e30398) SHA1(2b59a5e40bed2a988382054fe30d92808dad3348) )
	ROM_LOAD( "g02.u62", 0x200000, 0x200000, CRC(d9455dd7) SHA1(afa69fe9a540cd78b8cfecf09cffa1401c01141a) )
	ROM_LOAD( "g02.u63", 0x400000, 0x200000, CRC(4d20560b) SHA1(ceaee8cf0b69cc366b95ddcb689a5594d79e5114) )
	ROM_LOAD( "g02.u64", 0x600000, 0x200000, CRC(b17b9b6e) SHA1(fc6213d8322cda4c7f653e2d7d6d314ce84c97b7) )
	ROM_LOAD( "g02.u65", 0x800000, 0x200000, CRC(08541878) SHA1(138cf077a49a26440a3da1bdc2c399a208359e57) )
	ROM_LOAD( "g02.u66", 0xa00000, 0x200000, CRC(becf2a36) SHA1(f8b386d0292b1dc745b7253a3df51d1aa8d5e9db) )
	ROM_LOAD( "atgs.u1", 0xc00000, 0x200000, CRC(aa6f34a9) SHA1(00de85de1b413bd2c46931c13365f8556b50b634) ) /* US version's rom labeled "sp6_u67-1" */
	ROM_LOAD( "atgs.u2", 0xe00000, 0x200000, CRC(553eda27) SHA1(5b9126f966f0c64b3ac7c06526064d71e4df60c5) ) /* US version's rom labeled "sp6_u67-2" */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "atgs.u78", 0x000000, 0x200000, CRC(16710ecb) SHA1(6277f7f6095457df649932550b04242e5853ec5e) ) /* US version's rom labeled "bg0_u78" */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "atgs.u81", 0x000000, 0x200000, CRC(cb2aca91) SHA1(869f0f2db35c45ec90b74d33d521cbb598e60a3f) ) /* US version's rom labeled "bg1_u81" */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "atgs.u89", 0x000000, 0x200000, CRC(65f45a0f) SHA1(b7f4b56308dcdc144100d0a92d91255459a320a4) ) /* US version's rom labeled "bg2_u89" */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* Layer 3 */
	ROM_LOAD( "text.u82", 0x000000, 0x080000, CRC(f57333ea) SHA1(409d8005ffcf91943e4a743b2434ce425f5bdc36) ) /* US version's rom labeled "d20" */

	ROM_REGION( 0x440000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u53", 0x040000, 0x200000, CRC(c4bdd9e0) SHA1(a938a831e789ddf6f3cc5f3e5f3877ec7bd62d4e) )
	ROM_LOAD( "g02.u54", 0x240000, 0x200000, CRC(1357d50e) SHA1(433766177ce9d6933f90de85ba91bfc6d8d5d664) )

	ROM_REGION( 0x440000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u55", 0x040000, 0x200000, CRC(2d102898) SHA1(bd81f4cd2ba100707db0c5bb1419f0b23c998574) )
	ROM_LOAD( "g02.u56", 0x240000, 0x200000, CRC(9ff50dda) SHA1(1121685e387c20e228032f2b0f5cbb606376fc15) )
ROM_END

ROM_START( plegendj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "prog.u45", 0x000000, 0x80000, CRC(94f53db2) SHA1(34c671f160cfcb7d46cc964731ff2b77dc0be928) )
	ROM_LOAD16_BYTE( "prog.u44", 0x000001, 0x80000, CRC(db0ad756) SHA1(9c1510491cdc9442062ee3bd8a1bb93f00d33d97) )
	ROM_LOAD16_BYTE( "pr12.u2",  0x100000, 0x80000, CRC(0e202559) SHA1(217a8e47d5c679aff02ca43de1641230e4f78b01) ) /* Contains text in Japanese */
	ROM_LOAD16_BYTE( "pr12.u3",  0x100001, 0x80000, CRC(54742f21) SHA1(fae7bb7381478eb077f0409acd521f77417aa968) ) /* Contains text in Japanese */

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )	/* 68000 extra data roms */
	ROM_LOAD16_BYTE( "d15.u4",  0x000000, 0x80000, CRC(6352cec0) SHA1(a54d55b8d642e438158268d0d41880b6589e48e2) )
	ROM_LOAD16_BYTE( "d17.u5",  0x000001, 0x80000, CRC(7af810d8) SHA1(5e24f78a228809a001f3f3372c1b32ea05070e17) )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "sound.u3", 0x00000, 0x0c000, CRC(36f71520) SHA1(11d0a059ddba3e1aa4c54ccdde7b3f5c7bde482f) )
	ROM_CONTINUE(        0x10000, 0x14000             )

	ROM_REGION( 0x1000000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "g02.u61", 0x000000, 0x200000, CRC(91e30398) SHA1(2b59a5e40bed2a988382054fe30d92808dad3348) )
	ROM_LOAD( "g02.u62", 0x200000, 0x200000, CRC(d9455dd7) SHA1(afa69fe9a540cd78b8cfecf09cffa1401c01141a) )
	ROM_LOAD( "g02.u63", 0x400000, 0x200000, CRC(4d20560b) SHA1(ceaee8cf0b69cc366b95ddcb689a5594d79e5114) )
	ROM_LOAD( "g02.u64", 0x600000, 0x200000, CRC(b17b9b6e) SHA1(fc6213d8322cda4c7f653e2d7d6d314ce84c97b7) )
	ROM_LOAD( "g02.u65", 0x800000, 0x200000, CRC(08541878) SHA1(138cf077a49a26440a3da1bdc2c399a208359e57) )
	ROM_LOAD( "g02.u66", 0xa00000, 0x200000, CRC(becf2a36) SHA1(f8b386d0292b1dc745b7253a3df51d1aa8d5e9db) )
	ROM_LOAD( "atgs.u1", 0xc00000, 0x200000, CRC(aa6f34a9) SHA1(00de85de1b413bd2c46931c13365f8556b50b634) ) /* US version's rom labeled "sp6_u67-1" */
	ROM_LOAD( "atgs.u2", 0xe00000, 0x200000, CRC(553eda27) SHA1(5b9126f966f0c64b3ac7c06526064d71e4df60c5) ) /* US version's rom labeled "sp6_u67-2" */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "atgs.u78", 0x000000, 0x200000, CRC(16710ecb) SHA1(6277f7f6095457df649932550b04242e5853ec5e) ) /* US version's rom labeled "bg0_u78" */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "atgs.u81", 0x000000, 0x200000, CRC(cb2aca91) SHA1(869f0f2db35c45ec90b74d33d521cbb598e60a3f) ) /* US version's rom labeled "bg1_u81" */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	ROM_LOAD( "atgs.u89", 0x000000, 0x200000, CRC(65f45a0f) SHA1(b7f4b56308dcdc144100d0a92d91255459a320a4) ) /* US version's rom labeled "bg2_u89" */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* Layer 3 */
	ROM_LOAD( "text.u82", 0x000000, 0x080000, CRC(f57333ea) SHA1(409d8005ffcf91943e4a743b2434ce425f5bdc36) ) /* US version's rom labeled "d20" */

	ROM_REGION( 0x440000, REGION_SOUND1, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u53", 0x040000, 0x200000, CRC(c4bdd9e0) SHA1(a938a831e789ddf6f3cc5f3e5f3877ec7bd62d4e) )
	ROM_LOAD( "g02.u54", 0x240000, 0x200000, CRC(1357d50e) SHA1(433766177ce9d6933f90de85ba91bfc6d8d5d664) )

	ROM_REGION( 0x440000, REGION_SOUND2, 0 )	/* OKIM6295 #2 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "g02.u55", 0x040000, 0x200000, CRC(2d102898) SHA1(bd81f4cd2ba100707db0c5bb1419f0b23c998574) )
	ROM_LOAD( "g02.u56", 0x240000, 0x200000, CRC(9ff50dda) SHA1(1121685e387c20e228032f2b0f5cbb606376fc15) )
ROM_END


/***************************************************************************

                                Sailor Moon

(C) 1995 Banpresto
PCB: BP945A
CPU: TMP68HC000P16 (68000, 64 pin DIP)
SND: Z84C0008PEC (Z80, 40 pin DIP), OKI M6295 x 2, YM2151, YM3012
OSC: 28.000MHz, 16.000MHz
RAM: NEC 43256 x 8, NEC 424260 x 2, Sanyo LC3664 x 5

Other Chips:
SGS Thomson ST93C46CB1 (EEPROM?)
PALS (not dumped):
      18CV8 label SMBG
      18CV8 label SMZ80
      18CV8 label SMCPU
      GAL16V8 (located near BPSM-U47)

GFX:  038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      038 9437WX711 (176 pin PQFP)
      013 9346E7002 (240 pin PQFP)

On PCB near JAMMA connector is a small push button to access test mode.

ROMS:
BP945A.U9   27C040      Sound Program
BP945A.U45  27C240      Main Program
BPSM.U46    23C16000    Main Program?
BPSM.U47    23C4000     Sound?
BPSM.U48    23C16000    Sound?
BPSM.U53    23C16000    GFX
BPSM.U54    23C16000    GFX
BPSM.U57    23C16000    GFX
BPSM.U58    23C16000    GFX
BPSM.U59    23C16000    GFX
BPSM.U60    23C16000    GFX
BPSM.U61    23C16000    GFX
BPSM.U62    23C16000    GFX
BPSM.U63    23C16000    GFX
BPSM.U64    23C16000    GFX
BPSM.U65    23C16000    GFX
BPSM.U76    23C16000    GFX
BPSM.U77    23C16000    GFX

***************************************************************************/

ROM_START( sailormn )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "bpsm945a.u45", 0x000000, 0x080000, CRC(898c9515) SHA1(0fe8d7f13f5cfe2f6e79a0a21b2e8e7e70e65c4b) )
	ROM_LOAD16_WORD_SWAP( "bpsm.u46",     0x200000, 0x200000, CRC(32084e80) SHA1(0ac503190d95009620b5ad7e7e0e63324f6fa4eb) )

	ROM_REGION( 0x88000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "bpsm945a.u9",  0x00000, 0x08000, CRC(438de548) SHA1(81a0ca1cd662e2017aa980da162d39cfd0a19f14) )
	ROM_CONTINUE(             0x10000, 0x78000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bpsm.u76", 0x000000, 0x200000, CRC(a243a5ba) SHA1(3a32d685e53e0b75977f7acb187cf414a50c7f8b) )
	ROM_LOAD( "bpsm.u77", 0x200000, 0x200000, CRC(5179a4ac) SHA1(ceb8d3d889aae885debb2c9cf2263f60be3f1212) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bpsm.u53", 0x000000, 0x200000, CRC(b9b15f83) SHA1(8c574c97d38fb9e2889648c8d677b171e80a4229) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bpsm.u54", 0x000000, 0x200000, CRC(8f00679d) SHA1(4ea412f8ecdb9fd46f2d1378809919d1a62fcc2b) )

	ROM_REGION( (5*0x200000)*2, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	/* 4 bit part */
	ROM_LOAD( "bpsm.u57", 0x000000, 0x200000, CRC(86be7b63) SHA1(6b7d3d41fb1e4045c765b3cc98304464d91e6e3d) )
	ROM_LOAD( "bpsm.u58", 0x200000, 0x200000, CRC(e0bba83b) SHA1(9e1434814efd9321b2e5210b995d2fe66cca37dd) )
	ROM_LOAD( "bpsm.u62", 0x400000, 0x200000, CRC(a1e3bfac) SHA1(4528887d57e519df8dd60b2392db4c175c57b239) )
	ROM_LOAD( "bpsm.u61", 0x600000, 0x200000, CRC(6a014b52) SHA1(107c687479b59c455fc514cd61d290853c95ad9a) )
	ROM_LOAD( "bpsm.u60", 0x800000, 0x200000, CRC(992468c0) SHA1(3c66cc08313a9a326badc44f53a98cdfe0643da4) )
	/* 2 bit part */
	ROM_LOAD( "bpsm.u65", 0xa00000, 0x200000, CRC(f60fb7b5) SHA1(72cb8908cd687a330e14657664cd35037a52c39e) )
	ROM_LOAD( "bpsm.u64", 0xc00000, 0x200000, CRC(6559d31c) SHA1(bf688123a4beff625652cc1844bf0dc192f5c90f) )
	ROM_LOAD( "bpsm.u63", 0xe00000, 0x100000, CRC(d57a56b4) SHA1(e039b336887b66eba4e0630a3cb04cbd8fe14073) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(         0xe00000, 0x100000             )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 #0 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u48", 0x040000, 0x200000, CRC(498e4ed1) SHA1(28d45a41702d9e5af4e214c1800b2e513ec84d51) )	// 16 x $20000

	ROM_REGION( 0x240000, REGION_SOUND2, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u47", 0x040000, 0x080000, CRC(0f2901b9) SHA1(ebd3e9e39e8d2bc91688dac19b99548a28b4733c) )	// 4 x $20000
	ROM_RELOAD(           0x0c0000, 0x080000             )
	ROM_RELOAD(           0x140000, 0x080000             )
	ROM_RELOAD(           0x1c0000, 0x080000             )
ROM_END

ROM_START( sailormo )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "smprg.u45",    0x000000, 0x080000, CRC(234f1152) SHA1(8fc6d4a8995d550862d328011d3357c09334f0fa) )
	ROM_LOAD16_WORD_SWAP( "bpsm.u46",     0x200000, 0x200000, CRC(32084e80) SHA1(0ac503190d95009620b5ad7e7e0e63324f6fa4eb) )

	ROM_REGION( 0x88000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "bpsm945a.u9",  0x00000, 0x08000, CRC(438de548) SHA1(81a0ca1cd662e2017aa980da162d39cfd0a19f14) )
	ROM_CONTINUE(             0x10000, 0x78000             )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites (do not dispose) */
	ROM_LOAD( "bpsm.u76", 0x000000, 0x200000, CRC(a243a5ba) SHA1(3a32d685e53e0b75977f7acb187cf414a50c7f8b) )
	ROM_LOAD( "bpsm.u77", 0x200000, 0x200000, CRC(5179a4ac) SHA1(ceb8d3d889aae885debb2c9cf2263f60be3f1212) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "bpsm.u53", 0x000000, 0x200000, CRC(b9b15f83) SHA1(8c574c97d38fb9e2889648c8d677b171e80a4229) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "bpsm.u54", 0x000000, 0x200000, CRC(8f00679d) SHA1(4ea412f8ecdb9fd46f2d1378809919d1a62fcc2b) )

	ROM_REGION( (5*0x200000)*2, REGION_GFX4, ROMREGION_DISPOSE )	/* Layer 2 */
	/* 4 bit part */
	ROM_LOAD( "bpsm.u57", 0x000000, 0x200000, CRC(86be7b63) SHA1(6b7d3d41fb1e4045c765b3cc98304464d91e6e3d) )
	ROM_LOAD( "bpsm.u58", 0x200000, 0x200000, CRC(e0bba83b) SHA1(9e1434814efd9321b2e5210b995d2fe66cca37dd) )
	ROM_LOAD( "bpsm.u62", 0x400000, 0x200000, CRC(a1e3bfac) SHA1(4528887d57e519df8dd60b2392db4c175c57b239) )
	ROM_LOAD( "bpsm.u61", 0x600000, 0x200000, CRC(6a014b52) SHA1(107c687479b59c455fc514cd61d290853c95ad9a) )
	ROM_LOAD( "bpsm.u60", 0x800000, 0x200000, CRC(992468c0) SHA1(3c66cc08313a9a326badc44f53a98cdfe0643da4) )
	/* 2 bit part */
	ROM_LOAD( "bpsm.u65", 0xa00000, 0x200000, CRC(f60fb7b5) SHA1(72cb8908cd687a330e14657664cd35037a52c39e) )
	ROM_LOAD( "bpsm.u64", 0xc00000, 0x200000, CRC(6559d31c) SHA1(bf688123a4beff625652cc1844bf0dc192f5c90f) )
	ROM_LOAD( "bpsm.u63", 0xe00000, 0x100000, CRC(d57a56b4) SHA1(e039b336887b66eba4e0630a3cb04cbd8fe14073) )	// FIRST AND SECOND HALF IDENTICAL
	ROM_CONTINUE(         0xe00000, 0x100000             )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 #0 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u48", 0x040000, 0x200000, CRC(498e4ed1) SHA1(28d45a41702d9e5af4e214c1800b2e513ec84d51) )	// 16 x $20000

	ROM_REGION( 0x240000, REGION_SOUND2, 0 )	/* OKIM6295 #1 Samples */
	/* Leave the 0x40000 bytes addressable by the chip empty */
	ROM_LOAD( "bpsm.u47", 0x040000, 0x080000, CRC(0f2901b9) SHA1(ebd3e9e39e8d2bc91688dac19b99548a28b4733c) )	// 4 x $20000
	ROM_RELOAD(           0x0c0000, 0x080000             )
	ROM_RELOAD(           0x140000, 0x080000             )
	ROM_RELOAD(           0x1c0000, 0x080000             )
ROM_END


/***************************************************************************

                                    Uo Poko
Board: CV02
OSC:    28.0, 16.0, 16.9 MHz

***************************************************************************/

ROM_START( uopoko )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u26.bin", 0x000000, 0x080000, CRC(b445c9ac) SHA1(4dda1c6e19de629ea4d9061560c32a9f0deabd53) )
	ROM_LOAD16_BYTE( "u25.bin", 0x000001, 0x080000, CRC(a1258482) SHA1(7f4adc4a6d069032aaf3d93eb60fde16b59483f8) )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "u33.bin", 0x000000, 0x400000, CRC(5d142ad2) SHA1(f26abcf7a625a322b83df44fbd6e852bfb03663c) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u49.bin", 0x000000, 0x400000, CRC(12fb11bb) SHA1(953df1b16b5c9a6c3eb2fdebec4669a879270e73) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u4.bin", 0x000000, 0x200000, CRC(a2d0d755) SHA1(f8493ef7f367f3dc2a229ba785ac67bc5c2c54c0) )
ROM_END

ROM_START( uopokoj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "u26j.bin", 0x000000, 0x080000, CRC(e7eec050) SHA1(cf3a77741029f96dbbec5ca7217a1723e4233cff) )
	ROM_LOAD16_BYTE( "u25j.bin", 0x000001, 0x080000, CRC(68cb6211) SHA1(a6db0bc2e3e54b6992a44b7d52395917e66db49b) )

	ROM_REGION( 0x400000 * 2, REGION_GFX1, 0 )		/* Sprites: * 2 , do not dispose */
	ROM_LOAD( "u33.bin", 0x000000, 0x400000, CRC(5d142ad2) SHA1(f26abcf7a625a322b83df44fbd6e852bfb03663c) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "u49.bin", 0x000000, 0x400000, CRC(12fb11bb) SHA1(953df1b16b5c9a6c3eb2fdebec4669a879270e73) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "u4.bin", 0x000000, 0x200000, CRC(a2d0d755) SHA1(f8493ef7f367f3dc2a229ba785ac67bc5c2c54c0) )
ROM_END






/***************************************************************************


    Drivers Init Routines - Rom decryption/unpacking, global vars etc.


***************************************************************************/

/* Tiles are 6 bit, 4 bits stored in one rom, 2 bits in the other.
   Expand the 2 bit part into a 4 bit layout, so we can decode it */
void sailormn_unpack_tiles( const int region )
{
	unsigned char *src		=	memory_region(region) + (memory_region_length(region)/4)*3 - 1;
	unsigned char *dst		=	memory_region(region) + (memory_region_length(region)/4)*4 - 2;

	while(src <= dst)
	{
		unsigned char data = src[0];

		dst[0] = ((data & 0x03) << 4) + ((data & 0x0c) >> 2);
		dst[1] = ((data & 0x30) >> 0) + ((data & 0xc0) >> 6);

		src -= 1;
		dst -= 2;
	}
}

DRIVER_INIT( cave )
{
	cave_default_eeprom = 0;
	cave_default_eeprom_length = 0;
	cave_region_byte = -1;

	cave_spritetype = 0;	// Normal sprites
	cave_kludge = 0;
	time_vblank_irq = 100;

	irq_level = 1;
}

DRIVER_INIT( agallet )
{
	init_cave();

	sailormn_unpack_tiles( REGION_GFX4 );

	cave_default_eeprom = cave_default_eeprom_type7;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type7);
	cave_region_byte = 0x1f;

	unpack_sprites();

//  Speed Hack
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb80000, 0xb80001, 0, 0, agallet_irq_cause_r);
}

DRIVER_INIT( dfeveron )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type1;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type1);
	cave_region_byte = -1;

	unpack_sprites();
	cave_kludge = 2;
}

DRIVER_INIT( feversos )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type1feversos;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type1feversos);
	cave_region_byte = -1;

	unpack_sprites();
	cave_kludge = 2;
}

DRIVER_INIT( ddonpach )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type2;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type2);
	cave_region_byte = -1;

	ddonpach_unpack_sprites();
	cave_spritetype = 1;	// "different" sprites (no zooming?)
	time_vblank_irq = 90;
}

DRIVER_INIT( donpachi )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type2;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type2);
	cave_region_byte = -1;

	ddonpach_unpack_sprites();
	cave_spritetype = 1;	// "different" sprites (no zooming?)
	time_vblank_irq = 90;

	NMK112_set_paged_table(0, 0);	// chip #0 (music) is not paged
}

DRIVER_INIT( esprade )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type2;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type2);
	cave_region_byte = -1;

	esprade_unpack_sprites();
	time_vblank_irq = 2000;	/**/

#if 0		//ROM PATCH
	{
		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		rom[0x118A/2] = 0x4e71;			//palette fix   118A: 5548              SUBQ.W  #2,A0       --> NOP
	}
#endif
}

DRIVER_INIT( gaia )
{
	init_cave();

	/* No EEPROM */

	unpack_sprites();
	cave_spritetype = 2;	// Normal sprites with different position handling
	time_vblank_irq = 2000;	/**/
}

DRIVER_INIT( guwange )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type1;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type1);
	cave_region_byte = -1;

	esprade_unpack_sprites();
	time_vblank_irq = 2000;	/**/
}

DRIVER_INIT( hotdogst )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type4;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type4);
	cave_region_byte = -1;

	unpack_sprites();
	cave_spritetype = 2;	// Normal sprites with different position handling
	time_vblank_irq = 2000;	/**/
}

DRIVER_INIT( mazinger )
{
	unsigned char *buffer;
	UINT8 *src = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);

	init_cave();

	/* decrypt sprites */
	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i ^ 0xdf88] = src[BITSWAP24(i,23,22,21,20,19,9,7,3,15,4,17,14,18,2,16,5,11,8,6,13,1,10,12,0)];
		memcpy(src,buffer,len);
		free(buffer);
	}

	cave_default_eeprom = cave_default_eeprom_type5;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type5);
	cave_region_byte = 0x05;

	unpack_sprites();
	cave_spritetype = 2;	// Normal sprites with different position handling
	cave_kludge = 3;
	time_vblank_irq = 2100;

	/* setup extra ROM */
	memory_set_bankptr(1,memory_region(REGION_USER1));
}


DRIVER_INIT( metmqstr )
{
	init_cave();

	unpack_sprites();
	cave_spritetype = 2;	// Normal sprites with different position handling
	cave_kludge = 3;
	time_vblank_irq = 17376;
}


DRIVER_INIT( pwrins2j )
{
	unsigned char *buffer;
	UINT8 *src = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);
	int i, j;

	init_cave();

	if ((buffer = malloc(len)))
	{
		 for(i=0; i<len/2; i++) 		{
			j = BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7, 2,4,6,1,5,3, 0);
			if(((j & 6) == 0) || ((j & 6) == 6))
				j ^= 6;
			buffer[j ^ 7] = (src[i] >> 4) | (src[i] << 4);
		}

		memcpy(src,buffer,len);
		free(buffer);
	}

	unpack_sprites();
	cave_spritetype = 3;
	cave_kludge = 4;
	time_vblank_irq = 2000;	/**/


}

DRIVER_INIT( pwrinst2 )
{
	/* this patch fixes on of the moves, why is it needed? is the rom bad or is there another
       problem? does the Japan set need it or not? */
	init_pwrins2j();

#if 1		//ROM PATCH
	{
		UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
		rom[0xD46C/2] = 0xD482;			// kurara dash fix  0xd400 -> 0xd482
	}
#endif

}


DRIVER_INIT( sailormn )
{
	unsigned char *buffer;
	UINT8 *src = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);

	init_cave();

	/* decrypt sprites */
	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i ^ 0x950c4] = src[BITSWAP24(i,23,22,21,20,15,10,12,6,11,1,13,3,16,17,2,5,14,7,18,8,4,19,9,0)];
		memcpy(src,buffer,len);
		free(buffer);
	}

	sailormn_unpack_tiles( REGION_GFX4 );

	cave_default_eeprom = cave_default_eeprom_type6;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type6);
	cave_region_byte = 0x11;

	unpack_sprites();
	cave_spritetype = 2;	// Normal sprites with different position handling
	cave_kludge = 1;
	time_vblank_irq = 2000;
}

DRIVER_INIT( uopoko )
{
	init_cave();

	cave_default_eeprom = cave_default_eeprom_type3;
	cave_default_eeprom_length = sizeof(cave_default_eeprom_type4);
	cave_region_byte = -1;

	unpack_sprites();
	cave_kludge = 2;
	time_vblank_irq = 2000;	/**/
}

DRIVER_INIT( korokoro )
{
	init_cave();

	irq_level = 2;

	unpack_sprites();
	time_vblank_irq = 2000;	/**/
}

/***************************************************************************


                                Game Drivers


***************************************************************************/

GAME( 1994, pwrinst2, 0,        pwrinst2, metmqstr, pwrinst2, ROT0,   "Atlus/Cave",                           "Power Instinct 2 (USA)"                  , 0 ) /* 94.04.08 */
GAME( 1994, pwrins2j, pwrinst2, pwrinst2, metmqstr, pwrins2j, ROT0,   "Atlus/Cave",                           "Gouketsuji Ichizoku 2 (Japan)"           , 0 ) /* 94.04.08 */
GAME( 1994, mazinger, 0,        mazinger, mazinger, mazinger, ROT90,  "Banpresto/Dynamic Pl. Toei Animation", "Mazinger Z"                              , 0 ) // region in eeprom
GAME( 1995, donpachi, 0,        donpachi, cave,     donpachi, ROT270, "Atlus/Cave",                           "DonPachi (US)"                           , 0 )
GAME( 1995, donpachj, donpachi, donpachi, cave,     donpachi, ROT270, "Atlus/Cave",                           "DonPachi (Japan)"                        , 0 )
GAME( 1995, donpachk, donpachi, donpachi, cave,     donpachi, ROT270, "Atlus/Cave",                           "DonPachi (Korea)"                        , 0 )
GAME( 1995, metmqstr, 0,        metmqstr, metmqstr, metmqstr, ROT0,   "Banpresto/Pandorabox",                 "Metamoqester"                            , 0 )
GAME( 1995, nmaster,  metmqstr, metmqstr, metmqstr, metmqstr, ROT0,   "Banpresto/Pandorabox",                 "Oni - The Ninja Master (Japan)"          , 0 )
GAME( 1995, plegends, 0,        pwrinst2, metmqstr, pwrins2j, ROT0,   "Atlus/Cave",                           "Power Instinct Legends (USA)"            , 0 ) /* 95.06.20 */
GAME( 1995, plegendj, plegends, pwrinst2, metmqstr, pwrins2j, ROT0,   "Atlus/Cave",                           "Gouketsuji Ichizoku Saikyou Densetsu (Japan)", 0 ) /* 95.06.20 */
GAME( 1995, sailormn, 0,        sailormn, sailormn, sailormn, ROT0,   "Banpresto",                            "Pretty Soldier Sailor Moon (95/03/22B)"  , 0 ) // region in eeprom
GAME( 1995, sailormo, sailormn, sailormn, sailormn, sailormn, ROT0,   "Banpresto",                            "Pretty Soldier Sailor Moon (95/03/22)"   , 0 ) // region in eeprom
GAME( 1996, agallet,  0,        sailormn, sailormn, agallet,  ROT270, "Banpresto / Gazelle",                  "Air Gallet"                              , 0 ) // board was taiwan, region in eeprom
GAME( 1996, hotdogst, 0,        hotdogst, cave,     hotdogst, ROT90,  "Marble",                               "Hotdog Storm"                            , 0 )
GAME( 1997, ddonpach, 0,        ddonpach, cave,     ddonpach, ROT270, "Atlus/Cave",                           "DoDonPachi (International)"              , 0 )
GAME( 1997, ddonpchj, ddonpach, ddonpach, cave,     ddonpach, ROT270, "Atlus/Cave",                           "DoDonPachi (Japan)"                      , 0 )
GAME( 1998, dfeveron, 0,        dfeveron, cave,     dfeveron, ROT270, "Cave (Nihon System license)",          "Dangun Feveron (Japan)"                  , 0 )
GAME( 1998, feversos, dfeveron, dfeveron, cave,     feversos, ROT270, "Cave (Nihon System license)",          "Fever SOS (International)"               , 0 )
GAME( 1998, esprade,  0,        esprade,  cave,     esprade,  ROT270, "Atlus/Cave",                           "ESP Ra.De. (International Ver 1998 4/22)", 0 )
GAME( 1998, espradej, esprade,  esprade,  cave,     esprade,  ROT270, "Atlus/Cave",                           "ESP Ra.De. (Japan Ver 1998 4/21)"        , 0 )
GAME( 1998, espradeo, esprade,  esprade,  cave,     esprade,  ROT270, "Atlus/Cave",                           "ESP Ra.De. (Japan Ver 1998 4/14)"        , 0 )
GAME( 1998, uopoko,   0,        uopoko,   cave,     uopoko,   ROT0,   "Cave (Jaleco license)",                "Puzzle Uo Poko (International)"          , 0 )
GAME( 1998, uopokoj,  uopoko,   uopoko,   cave,     uopoko,   ROT0,   "Cave (Jaleco license)",                "Puzzle Uo Poko (Japan)"                  , 0 )
GAME( 1999, guwange,  0,        guwange,  guwange,  guwange,  ROT270, "Atlus/Cave",                           "Guwange (Japan)"                         , 0 )
GAME( 1999, gaia,     0,        gaia,     gaia,     gaia,     ROT0,   "Noise Factory",                        "Gaia Crusaders",        GAME_IMPERFECT_SOUND ) // cuts out occasionally
GAME( 1999, korokoro, 0,        korokoro, korokoro, korokoro, ROT0,   "Takumi",                               "Koro Koro Quest (Japan)"                 , 0 )

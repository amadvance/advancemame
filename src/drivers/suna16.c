// license:BSD-3-Clause
// copyright-holders:Luca Elia
/***************************************************************************

                            -=  SunA 16 Bit Games =-

                    driver by   Luca Elia (l.elia@tin.it)


CPU:    68000   +  Z80 [Music]  +  Z80 x 2 [4 Bit PCM]
Sound:  YM2151  +  DAC x 4


---------------------------------------------------------------------------
Year + Game                 By      Hardware
---------------------------------------------------------------------------
96  Back Street Soccer      SunA    68000 + Z80 x 3 + YM2151 + DAC x 4
96  Ultra Balloon           SunA    68000 + Z80 x 2 + YM2151 + DAC x 2
---------------------------------------------------------------------------


***************************************************************************/

#include "driver.h"
#include "sound/dac.h"
#include "sound/2151intf.h"
#include "sound/3812intf.h"
#include "sound/ay8910.h"

/* Variables and functions defined in vidhrdw: */

WRITE16_HANDLER( suna16_flipscreen_w );

READ16_HANDLER ( suna16_paletteram16_r );
WRITE16_HANDLER( suna16_paletteram16_w );

VIDEO_START( suna16 );
VIDEO_UPDATE( suna16 );
VIDEO_UPDATE( bestbest );
WRITE16_HANDLER( bestbest_flipscreen_w );


/***************************************************************************


                                Main CPU


***************************************************************************/

WRITE16_HANDLER( suna16_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w( 0, data & 0xff );
	}
	if (data & ~0xff)	logerror("CPU#0 PC %06X - Sound latch unknown bits: %04X\n", activecpu_get_pc(), data);
}


WRITE16_HANDLER( bssoccer_leds_w )
{
	if (ACCESSING_LSB)
	{
		set_led_status(0, data & 0x01);
		set_led_status(1, data & 0x02);
		set_led_status(2, data & 0x04);
		set_led_status(3, data & 0x08);
		coin_counter_w(0, data & 0x10);
	}
	if (data & ~0x1f)	logerror("CPU#0 PC %06X - Leds unknown bits: %04X\n", activecpu_get_pc(), data);
}


WRITE16_HANDLER( uballoon_leds_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 0x01);
		set_led_status(0, data & 0x02);
		set_led_status(1, data & 0x04);
	}
	if (data & ~0x07)	logerror("CPU#0 PC %06X - Leds unknown bits: %04X\n", activecpu_get_pc(), data);
}


/***************************************************************************
                            Back Street Soccer
***************************************************************************/

static UINT8 bestbest_prot;

static READ16_HANDLER( bestbest_prot_r )
{
	return bestbest_prot;
}

static WRITE16_HANDLER( bestbest_prot_w )
{
	if (ACCESSING_LSB)
	{
		switch (data & 0xff)
		{
			case 0x00: bestbest_prot ^= 0x09; break;
			case 0x08: bestbest_prot ^= 0x02; break;
			case 0x0c: bestbest_prot ^= 0x03; break;
		}
	}
}

static WRITE16_HANDLER( bestbest_coin_w )
{
	if (ACCESSING_LSB) coin_counter_w(0, data & 4);
}

static ADDRESS_MAP_START( bestbest_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_MIRROR(0x0c0000) AM_ROM
	AM_RANGE(0x200000, 0x2fffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x500000, 0x500001) AM_READWRITE(input_port_0_word_r, suna16_soundlatch_w)
	AM_RANGE(0x500002, 0x500003) AM_READWRITE(input_port_1_word_r, bestbest_flipscreen_w)
	AM_RANGE(0x500004, 0x500005) AM_READWRITE(input_port_2_word_r, bestbest_coin_w)
	AM_RANGE(0x500008, 0x500009) AM_WRITE(bestbest_prot_w)
	AM_RANGE(0x500018, 0x500019) AM_READ(bestbest_prot_r)
	AM_RANGE(0x540000, 0x540fff) AM_READWRITE(suna16_paletteram16_r, suna16_paletteram16_w) AM_BASE(&paletteram16)
	AM_RANGE(0x541000, 0x54ffff) AM_RAM
	AM_RANGE(0x580000, 0x58ffff) AM_RAM
	AM_RANGE(0x5c0000, 0x5dffff) AM_RAM AM_BASE(&spriteram16)
	AM_RANGE(0x5e0000, 0x5fffff) AM_RAM AM_BASE(&spriteram16_2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bestbest_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc000) AM_WRITE(YM3526_control_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(YM3526_write_port_0_w)
	AM_RANGE(0xc002, 0xc002) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc003, 0xc003) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xe000, 0xe7ff) AM_RAM
	AM_RANGE(0xf000, 0xf000) AM_WRITE(soundlatch2_w)
	AM_RANGE(0xf800, 0xf800) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static WRITE8_HANDLER( bestbest_pcm_w )
{
	/* each port drives a separate four-bit DAC */
	DAC_data_w(offset, (data & 0xf) * 0x11);
}

static ADDRESS_MAP_START( bestbest_pcm_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bestbest_pcm_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch2_r)
	AM_RANGE(0x00, 0x03) AM_WRITE(bestbest_pcm_w)
ADDRESS_MAP_END

static void bestbest_sound_irq(int state)
{
	cpunum_set_input_line(1, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM3526interface bestbest_ym3526_intf =
{
	bestbest_sound_irq
};

static MACHINE_RESET( bestbest )
{
	bestbest_prot = 0;
	flip_screen_set(0);
}

static ADDRESS_MAP_START( bssoccer_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM 				)	// ROM
	AM_RANGE(0x200000, 0x203fff) AM_READ(MRA16_RAM 				)	// RAM
	AM_RANGE(0x400000, 0x4001ff) AM_READ(suna16_paletteram16_r	)	// Banked Palette
	AM_RANGE(0x400200, 0x400fff) AM_READ(MRA16_RAM 				)	//
	AM_RANGE(0x600000, 0x61ffff) AM_READ(MRA16_RAM 				)	// Sprites
	AM_RANGE(0xa00000, 0xa00001) AM_READ(input_port_0_word_r		)	// P1 (Inputs)
	AM_RANGE(0xa00002, 0xa00003) AM_READ(input_port_1_word_r		)	// P2
	AM_RANGE(0xa00004, 0xa00005) AM_READ(input_port_2_word_r		)	// P3
	AM_RANGE(0xa00006, 0xa00007) AM_READ(input_port_3_word_r		)	// P4
	AM_RANGE(0xa00008, 0xa00009) AM_READ(input_port_4_word_r		)	// DSWs
	AM_RANGE(0xa0000a, 0xa0000b) AM_READ(input_port_5_word_r		)	// Coins
ADDRESS_MAP_END

static ADDRESS_MAP_START( bssoccer_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM 				)	// ROM
	AM_RANGE(0x200000, 0x203fff) AM_WRITE(MWA16_RAM 				)	// RAM
	AM_RANGE(0x400000, 0x4001ff) AM_WRITE(suna16_paletteram16_w) AM_BASE(&paletteram16)  // Banked Palette
	AM_RANGE(0x400200, 0x400fff) AM_WRITE(MWA16_RAM 				)	//
	AM_RANGE(0x600000, 0x61ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(suna16_soundlatch_w		)	// To Sound CPU
	AM_RANGE(0xa00002, 0xa00003) AM_WRITE(suna16_flipscreen_w		)	// Flip Screen
	AM_RANGE(0xa00004, 0xa00005) AM_WRITE(bssoccer_leds_w			)	// Leds
	AM_RANGE(0xa00006, 0xa00007) AM_WRITE(MWA16_NOP					)	// ? IRQ 1 Ack
	AM_RANGE(0xa00008, 0xa00009) AM_WRITE(MWA16_NOP					)	// ? IRQ 2 Ack
ADDRESS_MAP_END


/***************************************************************************
                                Ultra Balloon
***************************************************************************/

WRITE16_HANDLER( uballoon_spriteram16_w )
{
	COMBINE_DATA( &spriteram16[offset] );
}

static ADDRESS_MAP_START( uballoon_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM 				)	// ROM
	AM_RANGE(0x800000, 0x803fff) AM_READ(MRA16_RAM 				)	// RAM
	AM_RANGE(0x200000, 0x2001ff) AM_READ(suna16_paletteram16_r	)	// Banked Palette
	AM_RANGE(0x200200, 0x200fff) AM_READ(MRA16_RAM 				)	//
	AM_RANGE(0x400000, 0x41ffff) AM_READ(MRA16_RAM 				)	// Sprites
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_0_word_r		)	// P1 + Coins(Inputs)
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_1_word_r		)	// P2 + Coins
	AM_RANGE(0x600004, 0x600005) AM_READ(input_port_2_word_r		)	// DSW 1
	AM_RANGE(0x600006, 0x600007) AM_READ(input_port_3_word_r		)	// DSW 2
	AM_RANGE(0xa00000, 0xa0ffff) AM_READ(MRA16_NOP					)	// Protection
ADDRESS_MAP_END

static ADDRESS_MAP_START( uballoon_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM 				)	// ROM
	AM_RANGE(0x800000, 0x803fff) AM_WRITE(MWA16_RAM 				)	// RAM
	AM_RANGE(0x200000, 0x2001ff) AM_WRITE(suna16_paletteram16_w) AM_BASE(&paletteram16)  // Banked Palette
	AM_RANGE(0x200200, 0x200fff) AM_WRITE(MWA16_RAM 				)	//
	AM_RANGE(0x400000, 0x41ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites
	AM_RANGE(0x5c0000, 0x5dffff) AM_WRITE(uballoon_spriteram16_w	)	// Sprites (Mirror?)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(suna16_soundlatch_w		)	// To Sound CPU
	AM_RANGE(0x600004, 0x600005) AM_WRITE(suna16_flipscreen_w		)	// Flip Screen
	AM_RANGE(0x600008, 0x600009) AM_WRITE(uballoon_leds_w			)	// Leds
	AM_RANGE(0x60000c, 0x60000d) AM_WRITE(MWA16_NOP					)	// ? IRQ 1 Ack
	AM_RANGE(0x600010, 0x600011) AM_WRITE(MWA16_NOP					)	// ? IRQ 1 Ack
	AM_RANGE(0xa00000, 0xa0ffff) AM_WRITE(MWA16_NOP					)	// Protection
ADDRESS_MAP_END



/***************************************************************************


                                    Z80 #1

        Plays the music (YM2151) and controls the 2 Z80s in charge
        of playing the PCM samples


***************************************************************************/

/***************************************************************************
                            Back Street Soccer
***************************************************************************/

static ADDRESS_MAP_START( bssoccer_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM					)	// RAM
	AM_RANGE(0xf801, 0xf801) AM_READ(YM2151_status_port_0_r	)	// YM2151
	AM_RANGE(0xfc00, 0xfc00) AM_READ(soundlatch_r				)	// From Main CPU
ADDRESS_MAP_END

static ADDRESS_MAP_START( bssoccer_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM					)	// RAM
	AM_RANGE(0xf800, 0xf800) AM_WRITE(YM2151_register_port_0_w	)	// YM2151
	AM_RANGE(0xf801, 0xf801) AM_WRITE(YM2151_data_port_0_w		)	//
	AM_RANGE(0xfd00, 0xfd00) AM_WRITE(soundlatch2_w 			)	// To PCM Z80 #1
	AM_RANGE(0xfe00, 0xfe00) AM_WRITE(soundlatch3_w 			)	// To PCM Z80 #2
ADDRESS_MAP_END

/***************************************************************************
                                Ultra Balloon
***************************************************************************/

static ADDRESS_MAP_START( uballoon_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM					)	// RAM
	AM_RANGE(0xf801, 0xf801) AM_READ(YM2151_status_port_0_r	)	// YM2151
	AM_RANGE(0xfc00, 0xfc00) AM_READ(soundlatch_r				)	// From Main CPU
ADDRESS_MAP_END

static ADDRESS_MAP_START( uballoon_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_WRITE(MWA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM					)	// RAM
	AM_RANGE(0xf800, 0xf800) AM_WRITE(YM2151_register_port_0_w	)	// YM2151
	AM_RANGE(0xf801, 0xf801) AM_WRITE(YM2151_data_port_0_w		)	//
	AM_RANGE(0xfc00, 0xfc00) AM_WRITE(soundlatch2_w				)	// To PCM Z80
ADDRESS_MAP_END



/***************************************************************************


                                Z80 #2 & #3

        Dumb PCM samples players (e.g they don't even have RAM!)


***************************************************************************/

/***************************************************************************
                            Back Street Soccer
***************************************************************************/

/* Bank Switching */

static WRITE8_HANDLER( bssoccer_pcm_1_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int bank = data & 7;
	if (bank & ~7)	logerror("CPU#2 PC %06X - ROM bank unknown bits: %02X\n", activecpu_get_pc(), data);
	memory_set_bankptr(1, &RAM[bank * 0x10000 + 0x1000]);
}

static WRITE8_HANDLER( bssoccer_pcm_2_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU4);
	int bank = data & 7;
	if (bank & ~7)	logerror("CPU#3 PC %06X - ROM bank unknown bits: %02X\n", activecpu_get_pc(), data);
	memory_set_bankptr(2, &RAM[bank * 0x10000 + 0x1000]);
}



/* Memory maps: Yes, *no* RAM */

static ADDRESS_MAP_START( bssoccer_pcm_1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM			)	// ROM
	AM_RANGE(0x1000, 0xffff) AM_READ(MRA8_BANK1 		)	// Banked ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_ROM			)	// ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( bssoccer_pcm_2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM			)	// ROM
	AM_RANGE(0x1000, 0xffff) AM_READ(MRA8_BANK2 		)	// Banked ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_ROM			)	// ROM
ADDRESS_MAP_END



/* 2 DACs per CPU - 4 bits per sample */

static WRITE8_HANDLER( bssoccer_DAC_1_w )
{
	DAC_data_w( 0 + (offset & 1), (data & 0xf) * 0x11 );
}

static WRITE8_HANDLER( bssoccer_DAC_2_w )
{
	DAC_data_w( 2 + (offset & 1), (data & 0xf) * 0x11 );
}



static ADDRESS_MAP_START( bssoccer_pcm_1_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch2_r 				)	// From The Sound Z80
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_1_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(bssoccer_DAC_1_w				)	// 2 x DAC
	AM_RANGE(0x03, 0x03) AM_WRITE(bssoccer_pcm_1_bankswitch_w	)	// Rom Bank
ADDRESS_MAP_END

static ADDRESS_MAP_START( bssoccer_pcm_2_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch3_r 				)	// From The Sound Z80
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_2_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(bssoccer_DAC_2_w				)	// 2 x DAC
	AM_RANGE(0x03, 0x03) AM_WRITE(bssoccer_pcm_2_bankswitch_w	)	// Rom Bank
ADDRESS_MAP_END



/***************************************************************************
                                Ultra Balloon
***************************************************************************/

/* Bank Switching */

static WRITE8_HANDLER( uballoon_pcm_1_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int bank = data & 1;
	if (bank & ~1)	logerror("CPU#2 PC %06X - ROM bank unknown bits: %02X\n", activecpu_get_pc(), data);
	memory_set_bankptr(1, &RAM[bank * 0x10000 + 0x400]);
}

/* Memory maps: Yes, *no* RAM */

static ADDRESS_MAP_START( uballoon_pcm_1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_ROM			)	// ROM
	AM_RANGE(0x0400, 0xffff) AM_READ(MRA8_BANK1 		)	// Banked ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( uballoon_pcm_1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_ROM			)	// ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( uballoon_pcm_1_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch2_r 				)	// From The Sound Z80
ADDRESS_MAP_END
static ADDRESS_MAP_START( uballoon_pcm_1_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(bssoccer_DAC_1_w				)	// 2 x DAC
	AM_RANGE(0x03, 0x03) AM_WRITE(uballoon_pcm_1_bankswitch_w	)	// Rom Bank
ADDRESS_MAP_END

/***************************************************************************


                                Input Ports


***************************************************************************/

static INPUT_PORTS_START( bestbest )
	PORT_START_TAG("P1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0018, 0x0010, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Display Combos" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0600, 0x0400, "Play Time" )
	PORT_DIPSETTING(      0x0600, "1:10" )
	PORT_DIPSETTING(      0x0400, "1:20" )
	PORT_DIPSETTING(      0x0200, "1:30" )
	PORT_DIPSETTING(      0x0000, "1:40" )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
INPUT_PORTS_END

#define JOY(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN   ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2		  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3		  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_						 )


/***************************************************************************
                            Back Street Soccer
***************************************************************************/

INPUT_PORTS_START( bssoccer )

	PORT_START	// IN0 - $a00001.b - Player 1
	JOY(1)

	PORT_START	// IN1 - $a00003.b - Player 2
	JOY(2)

	PORT_START	// IN2 - $a00005.b - Player 3
	JOY(3)

	PORT_START	// IN3 - $a00007.b - Player 4
	JOY(4)

	PORT_START	// IN4 - $a00008.w - DSW x 2
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	  0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	  0x0010, DEF_STR( Easy )     )
	PORT_DIPSETTING(	  0x0018, DEF_STR( Normal )   )
	PORT_DIPSETTING(	  0x0008, DEF_STR( Hard )     )
	PORT_DIPSETTING(	  0x0000, "Hardest?" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, "Play Time P1" )
	PORT_DIPSETTING(	  0x0300, "1:30" )
	PORT_DIPSETTING(	  0x0200, "1:45" )
	PORT_DIPSETTING(	  0x0100, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Play Time P2" )
	PORT_DIPSETTING(	  0x0c00, "1:30" )
	PORT_DIPSETTING(	  0x0800, "1:45" )
	PORT_DIPSETTING(	  0x0400, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0x3000, 0x3000, "Play Time P3" )
	PORT_DIPSETTING(	  0x3000, "1:30" )
	PORT_DIPSETTING(	  0x2000, "1:45" )
	PORT_DIPSETTING(	  0x1000, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0xc000, 0xc000, "Play Time P4" )
	PORT_DIPSETTING(	  0xc000, "1:30" )
	PORT_DIPSETTING(	  0x8000, "1:45" )
	PORT_DIPSETTING(	  0x4000, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )

	PORT_START	// IN5 - $a0000b.b - Coins
	PORT_DIPNAME( 0x0001, 0x0001, "Copyright" )         // these 4 are shown in test mode
	PORT_DIPSETTING(	  0x0001, "Distributer Unico" )
	PORT_DIPSETTING(	  0x0000, "All Rights Reserved" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	// used!
	PORT_DIPSETTING(	  0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_COIN3   )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_COIN4   )

INPUT_PORTS_END


/***************************************************************************
                                Ultra Balloon
***************************************************************************/

INPUT_PORTS_START( uballoon )

	PORT_START	// IN0 - $600000.w - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN1    )

	PORT_START	// IN1 - $600002.w - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_DIPNAME( 0x3000, 0x3000, "Copyright" )	// Jumpers
	PORT_DIPSETTING(	  0x3000, "Distributer Unico" )
	PORT_DIPSETTING(	  0x2000, "All Rights Reserved" )
//  PORT_DIPSETTING(      0x1000, "Distributer Unico" )
//  PORT_DIPSETTING(      0x0000, "All Rights Reserved" )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_START	// IN2 - $600005.b - DSW 1
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	  0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Lives ) )
	PORT_DIPSETTING(	  0x0010, "2" )
	PORT_DIPSETTING(	  0x0018, "3" )
	PORT_DIPSETTING(	  0x0008, "4" )
	PORT_DIPSETTING(	  0x0000, "5" )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Easy )    )
	PORT_DIPSETTING(	  0x0060, DEF_STR( Normal )  )
	PORT_DIPSETTING(	  0x0020, DEF_STR( Hard )    )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Hardest ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN3 - $600007.b - DSW 2
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( Upright ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	  0x001c, "200K" )
	PORT_DIPSETTING(	  0x0010, "300K, 1000K" )
	PORT_DIPSETTING(	  0x0018, "400K" )
	PORT_DIPSETTING(	  0x000c, "500K, 1500K" )
	PORT_DIPSETTING(	  0x0008, "500K, 2000K" )
	PORT_DIPSETTING(	  0x0004, "500K, 3000K" )
	PORT_DIPSETTING(	  0x0014, "600K" )
	PORT_DIPSETTING(	  0x0000, DEF_STR( None ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5*" )
	PORT_DIPSETTING(	  0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6*" )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************


                                Graphics Layouts


***************************************************************************/

/* Tiles are 8x8x4 but the minimum sprite size is 2x2 tiles */

static const gfx_layout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+4,	0,4 },
	{ 3,2,1,0, 11,10,9,8 },
	{ STEP8(0,16) },
	8*8*4/2
};

static const gfx_layout bestbest_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 8,12,0,4 },
	{ 3,2,1,0,19,18,17,16 },
	{ STEP8(0,32) },
	8*8*4
};

static const gfx_decode bestbest_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bestbest_layout, 0, 128 },
	{ REGION_GFX2, 0, &bestbest_layout, 0, 128 },
	{ -1 }
};

static const gfx_decode suna16_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4, 0, 16*2 }, // [0] Sprites
	{ -1 }
};




/***************************************************************************


                                Machine drivers


***************************************************************************/


/***************************************************************************
                            Back Street Soccer
***************************************************************************/

INTERRUPT_GEN( bssoccer_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0: 	cpunum_set_input_line(0, 1, HOLD_LINE);	break;
		case 1: 	cpunum_set_input_line(0, 2, HOLD_LINE);	break;
	}
}

static MACHINE_DRIVER_START( bestbest )
	MDRV_CPU_ADD(M68000, 24000000/4)
	MDRV_CPU_PROGRAM_MAP(bestbest_map, 0)
	MDRV_CPU_VBLANK_INT(bssoccer_interrupt, 2)
	MDRV_CPU_ADD(Z80, 24000000/4)
	MDRV_CPU_PROGRAM_MAP(bestbest_sound_map, 0)
	MDRV_CPU_ADD(Z80, 24000000/4)
	MDRV_CPU_PROGRAM_MAP(bestbest_pcm_map, 0)
	MDRV_CPU_IO_MAP(bestbest_pcm_portmap, 0)
	MDRV_INTERLEAVE(100)
	MDRV_FRAMES_PER_SECOND(59.1734)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(bestbest)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_GFXDECODE(bestbest_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	MDRV_VIDEO_START(suna16)
	MDRV_VIDEO_UPDATE(bestbest)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(AY8910, 24000000/16)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.15)
	MDRV_SOUND_ADD(YM3526, 24000000/8)
	MDRV_SOUND_CONFIG(bestbest_ym3526_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.20)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.20)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.20)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bssoccer )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(bssoccer_readmem,bssoccer_writemem)
	MDRV_CPU_VBLANK_INT(bssoccer_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)		/* Z80B */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bssoccer_sound_readmem,bssoccer_sound_writemem)

	MDRV_CPU_ADD(Z80, 5000000)		/* Z80B */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bssoccer_pcm_1_readmem,bssoccer_pcm_1_writemem)
	MDRV_CPU_IO_MAP(bssoccer_pcm_1_readport,bssoccer_pcm_1_writeport)

	MDRV_CPU_ADD(Z80, 5000000)		/* Z80B */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bssoccer_pcm_2_readmem,bssoccer_pcm_2_writemem)
	MDRV_CPU_IO_MAP(bssoccer_pcm_2_readport,bssoccer_pcm_2_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(suna16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(suna16)
	MDRV_VIDEO_UPDATE(suna16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 0.20)
	MDRV_SOUND_ROUTE(1, "right", 0.20)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)
MACHINE_DRIVER_END



/***************************************************************************
                                Ultra Balloon
***************************************************************************/

static MACHINE_DRIVER_START( uballoon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(uballoon_readmem,uballoon_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(uballoon_sound_readmem,uballoon_sound_writemem)

	MDRV_CPU_ADD(Z80, 5000000)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(uballoon_pcm_1_readmem,uballoon_pcm_1_writemem)
	MDRV_CPU_IO_MAP(uballoon_pcm_1_readport,uballoon_pcm_1_writeport)

	/* 2nd PCM Z80 missing */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(suna16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(suna16)
	MDRV_VIDEO_UPDATE(suna16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END

/***************************************************************************


                                ROMs Loading


***************************************************************************/


/***************************************************************************

                            [ Back Street Soccer ]

  68000-10  32MHz
            14.318MHz
  01   02                    12
  03   04                   Z80B
  6264 6264       YM2151
                  6116
                   11      13
  62256           Z80B    Z80B
  62256
  62256   05 06                  SW2
          07 08                  SW1
          09 10          6116-45
                                     6116-45
                         6116-45     6116-45

***************************************************************************/

ROM_START( bestbest )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )

	ROM_LOAD16_BYTE( "4.bin", 0x00000, 0x20000, CRC(06741994) SHA1(e872e9e9d02360dda9c9b6df8e6424b0f3e18c1f) )
	ROM_LOAD16_BYTE( "2.bin", 0x00001, 0x20000, CRC(42843dec) SHA1(3705661a9740b3499297424e340da9a3606873fb) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "3.bin", 0x00000, 0x80000, CRC(e2bb8f26) SHA1(d73bbe034718c77774dede61e751a9ae2d29118a) )
	ROM_LOAD16_BYTE( "1.bin", 0x00001, 0x80000, CRC(d365e20a) SHA1(29706d6e422e71c7dad51a3369683a6539f72b54) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "5.bin", 0x00000, 0x10000, CRC(bb9265e6) SHA1(424eceac4fd48c9a99653ece2f3fcbc8b37569cf) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "6.bin", 0x00000, 0x10000, CRC(dd445f6b) SHA1(658417d72c003f25db273e3c731838317ed1876c) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_INVERT | ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "9.bin",  0x000000, 0x80000, CRC(b11994ea) SHA1(4ff2250a9dbb2e575982e2ffcad7686347368b5b) )
	ROM_LOAD16_BYTE( "7.bin",  0x000001, 0x80000, CRC(16188b73) SHA1(1e67f9b100614466e2ff1169f25c90e34a2e7db9) )
	ROM_LOAD16_BYTE( "10.bin", 0x100000, 0x80000, CRC(37b41ef5) SHA1(dd4500663537ffad369ee9415c56df90221bed23) )
	ROM_LOAD16_BYTE( "8.bin",  0x100001, 0x80000, CRC(765ce06b) SHA1(6cc6d7c27b49eedd58104c50e4887f86bff9357c) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_INVERT | ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "16.bin", 0x000000, 0x80000, CRC(dc46cdea) SHA1(d601f5464894223ce8459093ae53006155a3e680) )
	ROM_LOAD16_BYTE( "14.bin", 0x000001, 0x80000, CRC(c210fb53) SHA1(3d5a763bffaef922a77c95131b1e41f0f90629a5) )
	ROM_LOAD16_BYTE( "17.bin", 0x100000, 0x80000, CRC(c6fadd57) SHA1(ce9bc4d7a288feebdd19de09d00bec8489346878) )
	ROM_LOAD16_BYTE( "15.bin", 0x100001, 0x80000, CRC(3b1166c7) SHA1(7f2a0c9131fcf39dd67047b6e697c4076ca37b19) )
	ROM_LOAD16_BYTE( "13.bin", 0x200000, 0x80000, CRC(23283ac4) SHA1(f7aa00f203b17b590f1c43990f3f1c4aba7ba0ad) )
	ROM_LOAD16_BYTE( "11.bin", 0x200001, 0x80000, CRC(323eebc3) SHA1(0e82b583273c9ba5252f7a108538ae58edf39a03) )
	ROM_LOAD16_BYTE( "18.bin", 0x300000, 0x80000, CRC(674c4609) SHA1(f1de78c01d26dfb1174203415ccf3c771398d163) )
	ROM_LOAD16_BYTE( "12.bin", 0x300001, 0x80000, CRC(ca7c8176) SHA1(1ec99db3e0840b4647d6ccdf6fda118fa9ad4f42) )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.5", 0x000, 0x100, CRC(10bfcebb) SHA1(ae8708db7d3a8984f16e876867ecdbb4445e3378) )
	ROM_LOAD( "82s129.6", 0x100, 0x100, CRC(10bfcebb) SHA1(ae8708db7d3a8984f16e876867ecdbb4445e3378) )
ROM_END

ROM_START( bssoccer )

	ROM_REGION( 0x200000, REGION_CPU1, 0 ) 	/* 68000 Code */
	ROM_LOAD16_BYTE( "02", 0x000000, 0x080000, CRC(32871005) SHA1(b094ee3f4fc24c0521915d565f6e203d51e51f6d) )
	ROM_LOAD16_BYTE( "01", 0x000001, 0x080000, CRC(ace00db6) SHA1(6bd146f9b44c97be77578b4f0ffa28cbf66283c2) )
	ROM_LOAD16_BYTE( "04", 0x100000, 0x080000, CRC(25ee404d) SHA1(1ab7cb1b4836caa05be73ea441deed80f1e1ba81) )
	ROM_LOAD16_BYTE( "03", 0x100001, 0x080000, CRC(1a131014) SHA1(4d21264da3ee9b9912d1205999a555657ba33bd7) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) 	/* Z80 #1 - Music */
	ROM_LOAD( "11", 0x000000, 0x010000, CRC(df7ae9bc) SHA1(86660e723b0712c131dc57645b6a659d5100e962) ) // 1xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x080000, REGION_CPU3, 0 ) 	/* Z80 #2 - PCM */
	ROM_LOAD( "13", 0x000000, 0x080000, CRC(2b273dca) SHA1(86e1bac9d1e39457c565390b9053986453db95ab) )

	ROM_REGION( 0x080000, REGION_CPU4, 0 ) 	/* Z80 #3 - PCM */
	ROM_LOAD( "12", 0x000000, 0x080000, CRC(6b73b87b) SHA1(52c7dc7da6c21eb7e0dad13deadb1faa94a87bb3) )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "05", 0x000000, 0x080000, CRC(a5245bd4) SHA1(d46a8db437e49158c020661536eb0be8a6e2e8b0) )
	ROM_LOAD( "07", 0x080000, 0x080000, CRC(fdb765c2) SHA1(f9852fd3734d10e18c91cd572ca62e66d74ccb72) )
	ROM_LOAD( "09", 0x100000, 0x080000, CRC(0e82277f) SHA1(4bdfd0ff310bf8326806a83767a6c98905debbd0) )
	ROM_LOAD( "06", 0x180000, 0x080000, CRC(d42ce84b) SHA1(3a3d07d571793ecf4c936d3af244c63b9e4b4bb9) )
	ROM_LOAD( "08", 0x200000, 0x080000, CRC(96cd2136) SHA1(1241859d6c5e64de73898763f0358171ea4aeae3) )
	ROM_LOAD( "10", 0x280000, 0x080000, CRC(1ca94d21) SHA1(23d892b840e37064a175584f955f25f990d9179d) )

ROM_END



/***************************************************************************

                            [ Ultra Ballon ]

the gameplay on this game a like bubble bobble in many ways,it uses a
68k@8MHz as the main cpu,2 z80's and a ym2151,the names of the rom files
are just my guess.

prg1.rom      27c040
prg2.rom      27c040
gfx1.rom      27c040
gfx2.rom      27c040
gfx3.rom      27c040
gfx4.rom      27c040
audio1.rom    27c512
audio2.rom    27c010

***************************************************************************/

ROM_START( uballoon )

	ROM_REGION( 0x100000, REGION_CPU1, 0 ) 	/* 68000 Code */
	ROM_LOAD16_BYTE( "prg2.rom", 0x000000, 0x080000, CRC(72ab80ea) SHA1(b755940877cf286559208106dd5e6933aeb72242) )
	ROM_LOAD16_BYTE( "prg1.rom", 0x000001, 0x080000, CRC(27a04f55) SHA1(a530294b000654db8d84efe4835b72e0dca62819) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) 	/* Z80 #1 - Music */
	ROM_LOAD( "audio1.rom", 0x000000, 0x010000, CRC(c771f2b4) SHA1(6da4c526c0ea3be5d5bb055a31bf1171a6ddb51d) )

	ROM_REGION( 0x020000, REGION_CPU3, 0 ) 	/* Z80 #2 - PCM */
	ROM_LOAD( "audio2.rom", 0x000000, 0x020000, CRC(c7f75347) SHA1(5bbbd39285c593441c6da6a12f3632d60b103216) )

	/* There's no Z80 #3 - PCM */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "gfx1.rom", 0x000000, 0x080000, CRC(fd2ec297) SHA1(885834d9b58ccfd9a32ecaa51c45e70fbbe935db) )
	ROM_LOAD( "gfx2.rom", 0x080000, 0x080000, CRC(6307aa60) SHA1(00406eba98ec368e72ee53c08b9111dec4f2552f) )
	ROM_LOAD( "gfx3.rom", 0x100000, 0x080000, CRC(718f3150) SHA1(5971f006203f86743ebc825e4ab1ed1f811e3165) )
	ROM_LOAD( "gfx4.rom", 0x180000, 0x080000, CRC(af7e057e) SHA1(67a03b54ffa1483c8ed044f27287b7f3f1150455) )

ROM_END


DRIVER_INIT( uballoon )
{
	UINT16 *RAM = (UINT16 *) memory_region(REGION_CPU1);

	/* Patch out the protection checks */
	RAM[0x0113c/2] = 0x4e71;	// bne $646
	RAM[0x0113e/2] = 0x4e71;	// ""
	RAM[0x01784/2] = 0x600c;	// beq $1792
	RAM[0x018e2/2] = 0x600c;	// beq $18f0
	RAM[0x03c54/2] = 0x600C;	// beq $3c62
	RAM[0x126a0/2] = 0x4e71;	// bne $1267a (ROM test)
}



/***************************************************************************


                                Games Drivers


***************************************************************************/

GAME( 1994, bestbest, 0, bestbest, bestbest, 0, ROT0, "SunA", "Best Of Best", 0 )
GAME( 1996, bssoccer, 0, bssoccer, bssoccer, 0,        ROT0, "SunA", "Back Street Soccer", 0 )
GAME( 1996, uballoon, 0, uballoon, uballoon, uballoon, ROT0, "SunA", "Ultra Balloon", 0 )

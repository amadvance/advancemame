/***************************************************************************

    Exidy 6502 hardware

    Games supported:
        * Side Track
        * Targ
        * Spectar
        * Mouse Trap
        * Venture
		* Teeter Torture (prototype)
        * Pepper 2
        * Hard Hat
        * Fax

    Known bugs:
        * none at this time

****************************************************************************

    Exidy memory map

    0000-00FF R/W Zero Page RAM
    0100-01FF R/W Stack RAM
    0200-03FF R/W Scratchpad RAM
    0800-3FFF  R  Program ROM              (Targ, Spectar only)
    1A00       R  PX3 (Player 2 inputs)    (Fax only)
                  bit 4  D
                  bit 5  C
                  bit 6  B
                  bit 7  A
    1C00       R  PX2 (Player 1 inputs)    (Fax only)
                  bit 0  2 player start
                  bit 1  1 player start
                  bit 4  D
                  bit 5  C
                  bit 6  B
                  bit 7  A
    2000-3FFF  R  Banked question ROM      (Fax only)
    4000-43FF R/W Screen RAM
    4800-4FFF R/W Character Generator RAM (except Pepper II and Fax)
    5000       W  Motion Object 1 Horizontal Position Latch (sprite 1 X)
    5040       W  Motion Object 1 Vertical Position Latch   (sprite 1 Y)
    5080       W  Motion Object 2 Horizontal Position Latch (sprite 2 X)
    50C0       W  Motion Object 2 Vertical Position Latch   (sprite 2 Y)
    5100       R  Option Dipswitch Port
                  bit 0  coin 2 (NOT inverted) (must activate together with $5103 bit 5)
                  bit 1-2  bonus
                  bit 3-4  coins per play
                  bit 5-6  lives
                  bit 7  US/UK coins
    5100       W  Motion Objects Image Latch
                  Sprite number  bits 0-3 Sprite #1  4-7 Sprite #2
    5101       R  Control Inputs Port
                  bit 0  start 1
                  bit 1  start 2
                  bit 2  right
                  bit 3  left
                  bit 5  up
                  bit 6  down
                  bit 7  coin 1 (must activate together with $5103 bit 6)
    5101       W  Output Control Latch (not used in PEPPER II upright)
                  bit 7  Enable sprite #1
                  bit 6  Enable sprite #2
    5103       R  Interrupt Condition Latch
                  bit 0  LNG0 - supposedly a language DIP switch
                  bit 1  LNG1 - supposedly a language DIP switch
                  bit 2  different for each game, but generally a collision bit
                  bit 3  TABLE - supposedly a cocktail table DIP switch
                  bit 4  different for each game, but generally a collision bit
                  bit 5  coin 2 (must activate together with $5100 bit 0)
                  bit 6  coin 1 (must activate together with $5101 bit 7)
                  bit 7  L256 - VBlank?
    5213       R  IN2 (Mouse Trap)
                  bit 3  blue button
                  bit 2  free play
                  bit 1  red button
                  bit 0  yellow button
    52XX      R/W Audio/Color Board Communications
    6000-6FFF R/W Character Generator RAM (Pepper II, Fax only)
    8000-FFF9  R  Program memory space
    FFFA-FFFF  R  Interrupt and Reset Vectors

    Exidy Sound Board:
    0000-07FF R/W RAM (mirrored every 0x7f)
    0800-0FFF R/W 6532 Timer
    1000-17FF R/W 6520 PIA
    1800-1FFF R/W 8253 Timer
    2000-27FF bit 0 Channel 1 Filter 1 enable
              bit 1 Channel 1 Filter 2 enable
              bit 2 Channel 2 Filter 1 enable
              bit 3 Channel 2 Filter 2 enable
              bit 4 Channel 3 Filter 1 enable
              bit 5 Channel 3 Filter 2 enable
    2800-2FFF 6840 Timer
    3000      Bit 0..1 Noise select
    3001      Bit 0..2 Channel 1 Amplitude
    3002      Bit 0..2 Channel 2 Amplitude
    3003      Bit 0..2 Channel 3 Amplitude
    5800-7FFF ROM

    Targ:
    5200    Sound board control
            bit 0 Music
            bit 1 Shoot
            bit 2 unused
            bit 3 Swarn
            bit 4 Sspec
            bit 5 crash
            bit 6 long
            bit 7 game

    5201    Sound board control
            bit 0 note
            bit 1 upper

    MouseTrap Digital Sound:
    0000-3FFF ROM

    IO:
        A7 = 0: R Communication from sound processor
        A6 = 0: R CVSD Clock State
        A5 = 0: W Busy to sound processor
        A4 = 0: W Data to CVSD

***************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "exidy.h"
#include "sound/custom.h"
#include "sound/samples.h"
#include "sound/hc55516.h"

static UINT8 last_dial;


/*************************************
 *
 *  Special Teeter Torture input
 *
 *************************************/

static READ8_HANDLER( teetert_input_r )
{
	UINT8 result = readinputportbytag("IN0") & ~0x44;
	UINT8 dial = readinputportbytag("DIAL");

	if (dial != last_dial)
	{
		result |= 0x40;
		if (((dial - last_dial) & 0xff) < 0x80)
		{
			result |= 0x04;
			last_dial++;
		}
		else
			last_dial--;
	}

	return result;
}

/*************************************
 *
 *  Bankswitcher
 *
 *************************************/

static WRITE8_HANDLER( fax_bank_select_w )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	memory_set_bankptr(1, &RAM[0x10000 + (0x2000 * (data & 0x1F))]);
	if ((data & 0x1F) > 0x17)
		logerror("Banking to unpopulated ROM bank %02X!\n",data & 0x1F);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( common_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0400) AM_READWRITE(videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x5000, 0x503f) AM_WRITE(exidy_sprite1_xpos_w)
	AM_RANGE(0x5040, 0x507f) AM_WRITE(exidy_sprite1_ypos_w)
	AM_RANGE(0x5080, 0x50bf) AM_WRITE(exidy_sprite2_xpos_w)
	AM_RANGE(0x50c0, 0x50ff) AM_WRITE(exidy_sprite2_ypos_w)
	AM_RANGE(0x5100, 0x5100) AM_MIRROR(0xfc) AM_READ(port_tag_to_handler8("DSW"))
	AM_RANGE(0x5101, 0x5101) AM_MIRROR(0xfc) AM_READ(port_tag_to_handler8("IN0"))
	AM_RANGE(0x5100, 0x5100) AM_MIRROR(0xfc) AM_WRITE(exidy_spriteno_w)
	AM_RANGE(0x5101, 0x5101) AM_MIRROR(0xfc) AM_WRITE(exidy_sprite_enable_w)
	AM_RANGE(0x5103, 0x5103) AM_MIRROR(0xfc) AM_READ(exidy_interrupt_r)
	AM_RANGE(0x5210, 0x5212) AM_WRITE(exidy_color_w)
	AM_RANGE(0x5213, 0x5213) AM_READ(port_tag_to_handler8("IN2"))
ADDRESS_MAP_END


static ADDRESS_MAP_START( targ_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0800, 0x3fff) AM_ROM
	AM_RANGE(0x4800, 0x4fff) AM_READWRITE(MRA8_RAM, exidy_characterram_w) AM_BASE(&exidy_characterram)
	AM_RANGE(0x5200, 0x5201) AM_WRITE(targ_sh_w)
	AM_RANGE(0xff00, 0xffff) AM_ROM AM_REGION(REGION_CPU1, 0x3f00)
ADDRESS_MAP_END


static ADDRESS_MAP_START( venture_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x4800, 0x4fff) AM_READWRITE(MRA8_RAM, exidy_characterram_w) AM_BASE(&exidy_characterram)
	AM_RANGE(0x5200, 0x520f) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( pepper2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x5200, 0x520f) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x6000, 0x6fff) AM_READWRITE(MRA8_RAM, exidy_characterram_w) AM_BASE(&exidy_characterram)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( fax_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0400, 0x07ff) AM_RAM
	AM_RANGE(0x1a00, 0x1a00) AM_READ(port_tag_to_handler8("IN4"))
	AM_RANGE(0x1c00, 0x1c00) AM_READ(port_tag_to_handler8("IN3"))
	AM_RANGE(0x2000, 0x2000) AM_WRITE(fax_bank_select_w)
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(1)
	AM_RANGE(0x5200, 0x520f) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x5213, 0x5217) AM_WRITE(MWA8_NOP)				/* empty control lines on color/sound board */
	AM_RANGE(0x6000, 0x6fff) AM_READWRITE(MRA8_RAM, exidy_characterram_w) AM_BASE(&exidy_characterram)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x780) AM_RAM
	AM_RANGE(0x0800, 0x0fff) AM_READWRITE(exidy_shriot_r, exidy_shriot_w)
	AM_RANGE(0x1000, 0x17ff) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x1800, 0x1fff) AM_READWRITE(exidy_sh8253_r, exidy_sh8253_w)
	AM_RANGE(0x2000, 0x27ff) AM_WRITE(exidy_sound_filter_w)
	AM_RANGE(0x2800, 0x2fff) AM_READWRITE(exidy_sh6840_r, exidy_sh6840_w)
	AM_RANGE(0x3000, 0x37ff) AM_WRITE(exidy_sfxctrl_w)
	AM_RANGE(0x5800, 0x7fff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( cvsd_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(14) )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( cvsd_iomap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0xff) AM_READWRITE(mtrap_voiceio_r, mtrap_voiceio_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( sidetrac )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
/* 0x0c 2C_1C */
	PORT_DIPNAME( 0x10, 0x10, "Top Score Award" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("INTSOURCE")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( targ )
	PORT_START_TAG("DSW")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 ) /* upright/cocktail switch? */
	PORT_DIPNAME( 0x02, 0x00, "Pence Coinage" )
	PORT_DIPSETTING(    0x00, "10P/1P, 50P Coin/6P" )
	PORT_DIPSETTING(    0x02, "2x10P/1P, 50P Coin/3P" )
	PORT_DIPNAME( 0x04, 0x00, "Top Score Award" )
	PORT_DIPSETTING(    0x00, "Credit" )
	PORT_DIPSETTING(    0x04, "Extended Play" )
	PORT_DIPNAME( 0x18, 0x08, "Quarter Coinage" )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "1C/1C (no display)" )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x80, 0x80, "Currency" )
	PORT_DIPSETTING(    0x80, "Quarters" )
	PORT_DIPSETTING(    0x00, "Pence" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("INTSOURCE")
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/* identical to Targ, the only difference is the additional Language dip switch */
INPUT_PORTS_START( spectar )
	PORT_INCLUDE(targ)

	PORT_MODIFY("INTSOURCE")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x01, DEF_STR( French ) )
	PORT_DIPSETTING(    0x02, DEF_STR( German ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Spanish ) )
	PORT_BIT( 0x1c, IP_ACTIVE_HIGH, IPT_SPECIAL )
INPUT_PORTS_END

INPUT_PORTS_START( rallys )
	PORT_INCLUDE(spectar)

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x00, "2C/1P, 50P Coin/3P" )	PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x01, "1C/1P, 50P Coin/6P" )	PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x02, "1C/2P, 50P Coin/12P" )	PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x03, "1C/3P, 50P Coin/18P" )	PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPNAME( 0x04, 0x00, "Top Score Award" )
	PORT_DIPSETTING(    0x00, "Credit" )
	PORT_DIPSETTING(    0x04, "Extended Play" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPNAME( 0x80, 0x00, "Coin Mode" )
	PORT_DIPSETTING(    0x00, "Mode 1" )
	PORT_DIPSETTING(    0x80, "Mode 2" )

	PORT_MODIFY("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )

	PORT_MODIFY("INTSOURCE")
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

INPUT_PORTS_START( phantoma )
	PORT_INCLUDE(rallys)

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )		PORT_CONDITION("DSW", 0x80, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x00, "2F/1P, 5F Coin/3P" )		PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x01, "1F/1P, 5F Coin/6P" )		PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x02, "1F/2P, 5F Coin/12P" )	PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
	PORT_DIPSETTING(    0x03, "1F/3P, 5F Coin/18P" )	PORT_CONDITION("DSW", 0x80, PORTCOND_NOTEQUALS, 0x00)
INPUT_PORTS_END


INPUT_PORTS_START( mtrap )
	PORT_START_TAG("DSW")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "30000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x02, "50000" )
	PORT_DIPSETTING(    0x00, "60000" )
	PORT_DIPNAME( 0x98, 0x98, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 2C/1C Coin B 1C/3C" )
	PORT_DIPSETTING(    0x98, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "Coin A 1C/1C Coin B 1C/4C" )
	PORT_DIPSETTING(    0x18, "Coin A 1C/1C Coin B 1C/5C" )
	PORT_DIPSETTING(    0x88, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "Coin A 1C/3C Coin B 2C/7C" )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Dog Button")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("INTSOURCE")
/*
    The schematics claim these exist, but there's nothing in
    the ROMs to support that claim (as far as I can see):

    PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
    PORT_DIPSETTING(    0x00, DEF_STR( English ) )
    PORT_DIPSETTING(    0x01, DEF_STR( French ) )
    PORT_DIPSETTING(    0x02, DEF_STR( German ) )
    PORT_DIPSETTING(    0x03, DEF_STR( Spanish ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
*/

	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Yellow Button")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Red Button")
	PORT_BIT( 0x04, 0x04, IPT_DIPSWITCH_NAME ) PORT_NAME( DEF_STR( Free_Play ))
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Blue Button")
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( venture )
	PORT_START_TAG("DSW")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x06, "50000" )
	PORT_DIPNAME( 0x98, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x88, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x98, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, "Pence: A 2C/1C B 1C/3C" )
	PORT_DIPSETTING(    0x18, "Pence: A 1C/1C B 1C/6C" )
	/*0x10 same as 0x00 */
	/*0x90 same as 0x80 */
	PORT_DIPNAME( 0x60, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x60, "5" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("INTSOURCE")
/*
    The schematics claim these exist, but there's nothing in
    the ROMs to support that claim (as far as I can see):

    PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
    PORT_DIPSETTING(    0x00, DEF_STR( English ) )
    PORT_DIPSETTING(    0x01, DEF_STR( French ) )
    PORT_DIPSETTING(    0x02, DEF_STR( German ) )
    PORT_DIPSETTING(    0x03, DEF_STR( Spanish ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
*/
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( pepper2 )
	PORT_START_TAG("DSW")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "40000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x02, "60000" )
	PORT_DIPSETTING(    0x00, "70000" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x98, 0x98, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 2C/1C Coin B 1C/3C" )
	PORT_DIPSETTING(    0x98, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "Coin A 1C/1C Coin B 1C/4C" )
	PORT_DIPSETTING(    0x18, "Coin A 1C/1C Coin B 1C/5C" )
	PORT_DIPSETTING(    0x88, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits 2C/7C" )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START_TAG("IN0")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("INTSOURCE")
/*
    The schematics claim these exist, but there's nothing in
    the ROMs to support that claim (as far as I can see):

    PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
    PORT_DIPSETTING(    0x00, DEF_STR( English ) )
    PORT_DIPSETTING(    0x01, DEF_STR( French ) )
    PORT_DIPSETTING(    0x02, DEF_STR( German ) )
    PORT_DIPSETTING(    0x03, DEF_STR( Spanish ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
*/
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( teetert )
	PORT_START_TAG("DSW")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "20000" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x02, "40000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x98, 0x98, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "Pence: A 2C/1C B 1C/3C" )
	PORT_DIPSETTING(    0x98, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "Pence: A 1C/1C B 1C/4C" )
	PORT_DIPSETTING(    0x18, "Pence: A 1C/1C B 1C/5C" )
	PORT_DIPSETTING(    0x88, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "1C/3C, 2C/7C" )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x60, "2" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* direction */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* movement */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("INTSOURCE")
/*
    The schematics claim these exist, but there's nothing in
    the ROMs to support that claim (as far as I can see):

    PORT_DIPNAME( 0x03, 0x00, "Language" )
    PORT_DIPSETTING(    0x00, "English" )
    PORT_DIPSETTING(    0x01, "French" )
    PORT_DIPSETTING(    0x02, "German" )
    PORT_DIPSETTING(    0x03, "Spanish" )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
*/
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DIAL")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_MINMAX(0,255) PORT_SENSITIVITY(1) PORT_KEYDELTA(150) PORT_REVERSE
INPUT_PORTS_END

INPUT_PORTS_START( fax )
	PORT_START_TAG("DSW")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Time" )
	PORT_DIPSETTING(    0x06, "8000" )
	PORT_DIPSETTING(    0x04, "13000" )
	PORT_DIPSETTING(    0x02, "18000" )
	PORT_DIPSETTING(    0x00, "25000" )
	PORT_DIPNAME( 0x60, 0x60, "Game/Bonus Times" )
	PORT_DIPSETTING(    0x60, ":32/:24" )
	PORT_DIPSETTING(    0x40, ":48/:36" )
	PORT_DIPSETTING(    0x20, "1:04/:48" )
	PORT_DIPSETTING(    0x00, "1:12/1:04" )
	PORT_DIPNAME( 0x98, 0x98, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 2C/1C Coin B 1C/3C" )
	PORT_DIPSETTING(    0x98, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "Coin A 1C/1C Coin B 1C/4C" )
	PORT_DIPSETTING(    0x18, "Coin A 1C/1C Coin B 1C/5C" )
	PORT_DIPSETTING(    0x88, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits 2C/7C" )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START_TAG("IN0")
	PORT_BIT ( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("INTSOURCE")
/*
    The schematics claim these exist, but there's nothing in
    the ROMs to support that claim (as far as I can see):

    PORT_DIPNAME( 0x03, 0x00, DEF_STR( Language ) )
    PORT_DIPSETTING(    0x00, DEF_STR( English ) )
    PORT_DIPSETTING(    0x01, DEF_STR( French ) )
    PORT_DIPSETTING(    0x02, DEF_STR( German ) )
    PORT_DIPSETTING(    0x03, DEF_STR( Spanish ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
*/
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("IN4")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout charlayout_1bpp =
{
	8,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout charlayout_2bpp =
{
	8,8,
	256,
	2,
	{ 0, 256*8*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	8*32
};


static const gfx_decode gfxdecodeinfo_1bpp[] =
{
	{ REGION_CPU1, 0x4800, &charlayout_1bpp, 0, 4 },	/* the game dynamically modifies this */
	{ REGION_GFX1, 0x0000, &spritelayout,    8, 2 },
	{ -1 }
};


static const gfx_decode gfxdecodeinfo_2bpp[] =
{
	{ REGION_CPU1, 0x6000, &charlayout_2bpp, 0, 4 },	/* the game dynamically modifies this */
	{ REGION_GFX1, 0x0000, &spritelayout,   16, 2 },
	{ -1 }
};



/*************************************
 *
 *  Sound  definitions
 *
 *************************************/

static const char *targ_sample_names[] =
{
	"*targ",
	"expl.wav",
	"shot.wav",
	"sexpl.wav",
	"spslow.wav",
	"spfast.wav",
	0       /* end of array */
};


static struct Samplesinterface targ_samples_interface =
{
	4,	/* 3 Channels */
	targ_sample_names,
	targ_sh_start
};


static struct CustomSound_interface exidy_custom_interface =
{
	exidy_sh_start
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( targ )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 11289000/16)
	MDRV_CPU_PROGRAM_MAP(common_map,targ_map)
	MDRV_CPU_VBLANK_INT(exidy_vblank_interrupt,1)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_1bpp)
	MDRV_PALETTE_LENGTH(PALETTE_LEN)
	MDRV_COLORTABLE_LENGTH(COLORTABLE_LEN)

	MDRV_PALETTE_INIT(exidy)
	MDRV_VIDEO_START(exidy)
	MDRV_VIDEO_EOF(exidy)
	MDRV_VIDEO_UPDATE(exidy)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("sample", SAMPLES, 0)
	MDRV_SOUND_CONFIG(targ_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD_TAG("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( venture )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(targ)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(common_map,venture_map)

	MDRV_CPU_ADD(M6502, 3579545/4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_map,0)

	MDRV_INTERLEAVE(10)

	/* sound hardware */
	MDRV_SOUND_REPLACE("sample", CUSTOM, 0)
	MDRV_SOUND_CONFIG(exidy_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_REMOVE("dac")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( teetert )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(venture)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(teetert_vblank_interrupt,10)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mtrap )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(venture)

	MDRV_CPU_ADD(Z80, 3579545/2)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(cvsd_map,0)
	MDRV_CPU_IO_MAP(cvsd_iomap,0)

	MDRV_INTERLEAVE(32)

	/* sound hardware */
	MDRV_SOUND_ADD(HC55516, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pepper2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(venture)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(common_map,pepper2_map)

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo_2bpp)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fax )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pepper2)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(common_map,fax_map)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( sidetrac )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "stl8a-1", 0x2800, 0x0800, CRC(e41750ff) SHA1(3868a0d7e34a5118b39b31cff9e4fc839df541ff) )
	ROM_LOAD( "stl7a-2", 0x3000, 0x0800, CRC(57fb28dc) SHA1(6addd633d655d6a56b3e509d18e5f7c0ab2d0fbb) )
	ROM_LOAD( "stl6a-2", 0x3800, 0x0800, CRC(4226d469) SHA1(fd18b732b66082988b01e04adc2b1e5dae410c98) )
	ROM_LOAD( "stl9c-1", 0x4800, 0x0400, CRC(08710a84) SHA1(4bff254a14af7c968656ccc85277d31ab5a8f0c4) ) /* prom instead of ram chr gen*/

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "stl11d",  0x0000, 0x0200, CRC(3bd1acc1) SHA1(06f900cb8f56cd4215c5fbf58a852426d390e0c1) )
ROM_END


ROM_START( targ )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "hrl10a-1", 0x1800, 0x0800, CRC(969744e1) SHA1(e123bdb02b3b5f6a59c1e7c9ef557fe6bb19c62c) )
	ROM_LOAD( "hrl9a-1",  0x2000, 0x0800, CRC(a177a72d) SHA1(0e705e3e32021e55af4414fa0e2ccbc4980ee848) )
	ROM_LOAD( "hrl8a-1",  0x2800, 0x0800, CRC(6e6928a5) SHA1(10c725b27225ac5aad8639b081df68dd61522cf2) )
	ROM_LOAD( "hrl7a-1",  0x3000, 0x0800, CRC(e2f37f93) SHA1(b66743c296d3d4caba3bcbe6aa68cd6edd414816) )
	ROM_LOAD( "hrl6a-1",  0x3800, 0x0800, CRC(a60a1bfc) SHA1(17c0e67e1a0b263b57d70a148cc5d5099fecbb40) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hrl11d-1", 0x0000, 0x0400, CRC(9f03513e) SHA1(aa4763e49df65e5686a96431543580b8d8285893) )

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "hrl5c-1",  0x0000, 0x0100, CRC(a24290d0) SHA1(5f2888d168de874021b51c5d19a62fb8165e4454) )	/* address decoder */
	ROM_LOAD( "stl6d-1",  0x0100, 0x0020, CRC(e26f9053) SHA1(eec35b6aa2c2d305418306bf4a1754a0583f109f) )	/* unknown */
	ROM_LOAD( "hra2b-1",  0x0120, 0x0020, CRC(38e8024b) SHA1(adf1c1770695f7614c95eceb803f662c5b096a76) )	/* unknown */
	ROM_LOAD( "hrl14h-1", 0x0140, 0x0020, CRC(f76b4fcf) SHA1(197e0cc508ffeb5cefa4046bdfb158939d598225) )	/* unknown */
ROM_END

ROM_START( targc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "ctl.10a",  0x1800, 0x0800, CRC(058b3983) SHA1(8079667613c9273e95131c3c68cd92ce34c18148) )
	ROM_LOAD( "ctl.9a1",  0x2000, 0x0800, CRC(3ac44b6b) SHA1(8261ee7ee1c3cb05b2549464086bf6df09685743) )
	ROM_LOAD( "ctl.8a1",  0x2800, 0x0800, CRC(5c470021) SHA1(3638fc6827640857848cd649f10c1493025014de) )
	ROM_LOAD( "ctl.7a1",  0x3000, 0x0800, CRC(c774fd9b) SHA1(46272a64ad5cda0ff5ef3e9eeedefc555100a71a) )
	ROM_LOAD( "ctl.6a1",  0x3800, 0x0800, CRC(3d020439) SHA1(ebde4c851c9ecc310f110c7643a80275d97dc02c) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hrl11d-1", 0x0000, 0x0400, CRC(9f03513e) SHA1(aa4763e49df65e5686a96431543580b8d8285893) )
ROM_END


ROM_START( spectar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "spl11a-3", 0x1000, 0x0800, CRC(08880aff) SHA1(3becef348245ff4c8b0aae4a14751ab740b7d160) )
	ROM_LOAD( "spl10a-2", 0x1800, 0x0800, CRC(fca667c1) SHA1(168426f9e87c002d2673c0230fceac4d0831d594) )
	ROM_LOAD( "spl9a-3",  0x2000, 0x0800, CRC(9d4ce8ba) SHA1(2ef45c225fe704e49d10247c3eba1ef14141b3b7) )
	ROM_LOAD( "spl8a-2",  0x2800, 0x0800, CRC(cfacbadf) SHA1(77b27cf6f35e8e8dd2fd4f31bba2a96f3076163e) )
	ROM_LOAD( "spl7a-2",  0x3000, 0x0800, CRC(4c4741ff) SHA1(8de72613a385095253bb9e6da76493caec3115e4) )
	ROM_LOAD( "spl6a-2",  0x3800, 0x0800, CRC(0cb46b25) SHA1(65c5d2cc8df67225339dc8781dd29d4b57ded70c) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hrl11d-2", 0x0000, 0x0400, CRC(c55b645d) SHA1(0c18277939d74e3e1281a7f114a34781d30c2baf) )  /* this is actually not used (all FF) */
	ROM_CONTINUE(         0x0000, 0x0400 )  /* overwrite with the real one */
ROM_END

ROM_START( spectar1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "spl12a1",  0x0800, 0x0800, CRC(7002efb4) SHA1(fbb19ccd2aee49b78606eadcbef94e842e1be905) )
	ROM_LOAD( "spl11a1",  0x1000, 0x0800, CRC(8eb8526a) SHA1(0c42ee073fc73c89731dec4e3ecfc82c9b8301e9) )
	ROM_LOAD( "spl10a1",  0x1800, 0x0800, CRC(9d169b3d) SHA1(bee9d029df6e2fba24a5ba41a76f1658e9038838) )
	ROM_LOAD( "spl9a1",   0x2000, 0x0800, CRC(40e3eba1) SHA1(197aaed9a6159b6f3e347c0446be9e44733c1341) )
	ROM_LOAD( "spl8a1",   0x2800, 0x0800, CRC(64d8eb84) SHA1(a249c832ea951fddc6699f7ac0b4486e8a5be98e) )
	ROM_LOAD( "spl7a1",   0x3000, 0x0800, CRC(e08b0d8d) SHA1(6ffd6f8fb50c9fc09c38f56da7d6d005b66e78cc) )
	ROM_LOAD( "spl6a1",   0x3800, 0x0800, CRC(f0e4e71a) SHA1(5487a94650c964a7ab07f30aacab0b470dcb3b40) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hrl11d-2", 0x0000, 0x0400, CRC(c55b645d) SHA1(0c18277939d74e3e1281a7f114a34781d30c2baf) )  /* this is actually not used (all FF) */
	ROM_CONTINUE(         0x0000, 0x0400 )  /* overwrite with the real one */
ROM_END

ROM_START( rallys )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "rallys.01", 0x1000, 0x0400, CRC(a192b22b) SHA1(aaae0b1822f934df30b354f787ffa8848c71b52f) )
	ROM_LOAD( "rallys.02", 0x1400, 0x0400, CRC(19e730aa) SHA1(4f4e87d26c14a9ff2be5b4173c4e5804db551e33) )
	ROM_LOAD( "rallys.03", 0x1800, 0x0400, CRC(2a3e7b69) SHA1(d31a3e6acca87881741e88e70d46a4a0ee59fcf8) )
	ROM_LOAD( "rallys.04", 0x1c00, 0x0400, CRC(6d224696) SHA1(586bc8efdc8ac0a73e4a4300459efaf89021f6f5) )
	ROM_LOAD( "rallys.05", 0x2000, 0x0400, CRC(af943b5e) SHA1(819fa8a6ee78a39cdade49789cd42b4a215f82f0) )
	ROM_LOAD( "rallys.06", 0x2400, 0x0400, CRC(9b3d9e61) SHA1(b183e0844706713eb0a241a6e45c09c53e4077a3) )
	ROM_LOAD( "rallys.07", 0x2800, 0x0400, CRC(8ef8bc67) SHA1(c8d80cc8e89a9bc5d957d648d704e4c66b17932d) )
	ROM_LOAD( "rallys.08", 0x2c00, 0x0400, CRC(243c54f2) SHA1(813b3ecbd5642034b5de0bae96698ed2b036fc7b) )
	ROM_LOAD( "rallys.10", 0x3400, 0x0400, CRC(46f473d2) SHA1(e6a180fdcf2ac13ffab624554ef8aab128e80321) )
	ROM_LOAD( "rallys.09", 0x3c00, 0x0400, CRC(56ce8a94) SHA1(becd31cda58e59267517a39c82ccfa70abdd31c6) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hrl11d-1",  0x0000, 0x0400, CRC(9f03513e) SHA1(aa4763e49df65e5686a96431543580b8d8285893) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "targ82s.123", 0x0000, 0x0020, CRC(9eb9125c) SHA1(660ad9b2c7c28c3fda4b10c1401c03165d131c61) )	/* unknown */
ROM_END

ROM_START( phantoma )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "156_a2",  0x1000, 0x0800, CRC(c5af9d34) SHA1(4c9f9a06cc7f6caf13a79fa8491db17b01b24774) )
	ROM_LOAD( "156_a3",  0x1800, 0x0800, CRC(30121e69) SHA1(1588cfb61eb9aa9598b3ff600cc02b0f1ac622bf) )
	ROM_LOAD( "156_a4",  0x2000, 0x0800, CRC(02d7fb94) SHA1(634e952a6a0d4c1a42692100e1913ecd5ab9faed) )
	ROM_LOAD( "156_a5",  0x2800, 0x0800, CRC(0127bc8d) SHA1(c555507f2662d1b45caf0b696147f70749292930) )
	ROM_LOAD( "156_a1",  0xf800, 0x0800, CRC(26292c0a) SHA1(d4157e261f6247cfafb948d1a9dbf0b02b2b84de) )

	ROM_REGION( 0x800, REGION_GFX1, 0 )
	ROM_LOAD( "156_d1",  0x0000, 0x0800, CRC(d18e5f14) SHA1(5cd327500e74eca378ad5d0924949f96dd955cf8) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "156_pal", 0x0000, 0x0020, CRC(9fb1daee) SHA1(2ec1189a57c95d7ad820eb12343fcf2c3fb08431) )
ROM_END


ROM_START( mtrap )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "mtl11a.bin",  0xa000, 0x1000, CRC(bd6c3eb5) SHA1(248956374222a09caa5b8c8fa842e9286d8e1c5d) )
	ROM_LOAD( "mtl10a.bin",  0xb000, 0x1000, CRC(75b0593e) SHA1(48ce5382905f7c52929a95267d65fd0d3f0dcc92) )
	ROM_LOAD( "mtl9a.bin",   0xc000, 0x1000, CRC(28dd20ff) SHA1(8ac44ec27ac25209c8b49da4c6b423917ed8907e) )
	ROM_LOAD( "mtl8a.bin",   0xd000, 0x1000, CRC(cc09f7a4) SHA1(e806dc0e10b909b61e347f3e28eb024f3b3a9702) )
	ROM_LOAD( "mtl7a.bin",   0xe000, 0x1000, CRC(caafbb6d) SHA1(96823ac4e49f192121c53f70382a20f7c52e290b) )
	ROM_LOAD( "mtl6a.bin",   0xf000, 0x1000, CRC(d85e52ca) SHA1(51296247e365a468fe9458b722bbdbbeeed59fa0) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "mta5a.bin",   0x6800, 0x0800, CRC(dbe4ec02) SHA1(34e965428dbb4b9c558927bb80d19cb550b53228) )
	ROM_LOAD( "mta6a.bin",   0x7000, 0x0800, CRC(c00f0c05) SHA1(398b0bc2a7e54b1e2326ed067bf6bb15cc52ed39) )
	ROM_LOAD( "mta7a.bin",   0x7800, 0x0800, CRC(f3f16ca7) SHA1(3928c5da246c43036a7b4cbb140a1734d5f1fb03) )

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* 16k for digital sound processor */
	ROM_LOAD( "mta2a.bin", 0x0000, 0x1000, CRC(13db8ed3) SHA1(939352323bdcd7df25db5eb2e30f269bcaebe6af) )
	ROM_LOAD( "mta3a.bin", 0x1000, 0x1000, CRC(31bdfe5c) SHA1(b10bfe9e56dd617c5b4cd8b5bfec9c7f537b1086) )
	ROM_LOAD( "mta4a.bin", 0x2000, 0x1000, CRC(1502d0e8) SHA1(8ef51ad4601299016f1821a5c65bec0199dd5474) )
	ROM_LOAD( "mta1a.bin", 0x3000, 0x1000, CRC(658482a6) SHA1(c0d770fbeaa7cb3e0eef47d8caa0f8a78841692e) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mtl11d.bin",  0x0000, 0x0800, CRC(c6e4d339) SHA1(b091923e4d52e93d7c567afba217a10b2a3735fc) )
ROM_END

ROM_START( mtrap3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "mtl-3.11a",   0xa000, 0x1000, CRC(4091be6e) SHA1(a4432f4588915276583f4b2e8db527fd24eb4291) )
	ROM_LOAD( "mtl-3.10a",   0xb000, 0x1000, CRC(38250c2f) SHA1(b70a2a1d423ba90ca873cc43db40422abee07718) )
	ROM_LOAD( "mtl-3.9a",    0xc000, 0x1000, CRC(2eec988e) SHA1(52167dabd672d16d454df746fb2c83c9e4253624) )
	ROM_LOAD( "mtl-3.8a",    0xd000, 0x1000, CRC(744b4b1c) SHA1(94955d0703559d668988cb7045f835f955e5dd8a) )
	ROM_LOAD( "mtl-3.7a",    0xe000, 0x1000, CRC(ea8ec479) SHA1(785557a242d9343c83cdc403b1f726cbea9d230f) )
	ROM_LOAD( "mtl-3.6a",    0xf000, 0x1000, CRC(d72ba72d) SHA1(4c5b311bc7ecfc6133bc09e586635844e2f1d6a9) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "mta5a.bin",   0x6800, 0x0800, CRC(dbe4ec02) SHA1(34e965428dbb4b9c558927bb80d19cb550b53228) )
	ROM_LOAD( "mta6a.bin",   0x7000, 0x0800, CRC(c00f0c05) SHA1(398b0bc2a7e54b1e2326ed067bf6bb15cc52ed39) )
	ROM_LOAD( "mta7a.bin",   0x7800, 0x0800, CRC(f3f16ca7) SHA1(3928c5da246c43036a7b4cbb140a1734d5f1fb03) )

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* 16k for digital sound processor */
	ROM_LOAD( "mta2a.bin", 0x0000, 0x1000, CRC(13db8ed3) SHA1(939352323bdcd7df25db5eb2e30f269bcaebe6af) )
	ROM_LOAD( "mta3a.bin", 0x1000, 0x1000, CRC(31bdfe5c) SHA1(b10bfe9e56dd617c5b4cd8b5bfec9c7f537b1086) )
	ROM_LOAD( "mta4a.bin", 0x2000, 0x1000, CRC(1502d0e8) SHA1(8ef51ad4601299016f1821a5c65bec0199dd5474) )
	ROM_LOAD( "mta1a.bin", 0x3000, 0x1000, CRC(658482a6) SHA1(c0d770fbeaa7cb3e0eef47d8caa0f8a78841692e) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mtl11d.bin",  0x0000, 0x0800, CRC(c6e4d339) SHA1(b091923e4d52e93d7c567afba217a10b2a3735fc) )
ROM_END

ROM_START( mtrap4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "mta411a.bin",  0xa000, 0x1000, CRC(2879cb8d) SHA1(738bd3cd968fd733adcfe0fb5efdb2e2fcfb344e) )
	ROM_LOAD( "mta410a.bin",  0xb000, 0x1000, CRC(d7378af9) SHA1(44c8ba4c84f51306e5bdd64e6c255d1c1018db72) )
	ROM_LOAD( "mta49.bin",    0xc000, 0x1000, CRC(be667e64) SHA1(c5f686e3c403691f14992354af690dc89e1722f7) )
	ROM_LOAD( "mta48a.bin",   0xd000, 0x1000, CRC(de0442f8) SHA1(61774921adf016b3a2ae18baa79af60dca2d9e45) )
	ROM_LOAD( "mta47a.bin",   0xe000, 0x1000, CRC(cdf8c6a8) SHA1(932ae9c0ea5700bd79862efa94742136d8e15641) )
	ROM_LOAD( "mta46a.bin",   0xf000, 0x1000, CRC(77d3f2e6) SHA1(2c21dd7ee326ccb41d3c64eec90a19198382edea) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "mta5a.bin",    0x6800, 0x0800, CRC(dbe4ec02) SHA1(34e965428dbb4b9c558927bb80d19cb550b53228) )
	ROM_LOAD( "mta6a.bin",    0x7000, 0x0800, CRC(c00f0c05) SHA1(398b0bc2a7e54b1e2326ed067bf6bb15cc52ed39) )
	ROM_LOAD( "mta7a.bin",    0x7800, 0x0800, CRC(f3f16ca7) SHA1(3928c5da246c43036a7b4cbb140a1734d5f1fb03) )

	ROM_REGION( 0x4000, REGION_CPU3, 0 ) /* 16k for digital sound processor */
	ROM_LOAD( "mta2a.bin", 0x0000,0x1000,CRC(13db8ed3) SHA1(939352323bdcd7df25db5eb2e30f269bcaebe6af) )
	ROM_LOAD( "mta3a.bin", 0x1000,0x1000,CRC(31bdfe5c) SHA1(b10bfe9e56dd617c5b4cd8b5bfec9c7f537b1086) )
	ROM_LOAD( "mta4a.bin", 0x2000,0x1000,CRC(1502d0e8) SHA1(8ef51ad4601299016f1821a5c65bec0199dd5474) )
	ROM_LOAD( "mta1a.bin", 0x3000,0x1000,CRC(658482a6) SHA1(c0d770fbeaa7cb3e0eef47d8caa0f8a78841692e) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mtl11d.bin",   0x0000, 0x0800, CRC(c6e4d339) SHA1(b091923e4d52e93d7c567afba217a10b2a3735fc) )
ROM_END


ROM_START( venture )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "13a-cpu",      0x8000, 0x1000, CRC(f4e4d991) SHA1(6683c1552b56b20f2296e461aff697af73563792) )
	ROM_LOAD( "12a-cpu",      0x9000, 0x1000, CRC(c6d8cb04) SHA1(3b9ae8fdc35117c73c91daed66e93e5344bdcd7e) )
	ROM_LOAD( "11a-cpu",      0xa000, 0x1000, CRC(3bdb01f4) SHA1(3c1f43a3c37a21524b64d69e4dae58af8c2e0d90) )
	ROM_LOAD( "10a-cpu",      0xb000, 0x1000, CRC(0da769e9) SHA1(3604dc08c63461b2ea957a396887fb32e4a1a970) )
	ROM_LOAD( "9a-cpu",       0xc000, 0x1000, CRC(0ae05855) SHA1(29b3c2ca9740aa753e90131e6edcc61f414277e1) )
	ROM_LOAD( "8a-cpu",       0xd000, 0x1000, CRC(4ae59676) SHA1(36fc9dce9dd0c764a861634859ca0d7f98e20382) )
	ROM_LOAD( "7a-cpu",       0xe000, 0x1000, CRC(48d66220) SHA1(97b1605170c67b3a945b4d5f088df79328e163ce) )
	ROM_LOAD( "6a-cpu",       0xf000, 0x1000, CRC(7b78cf49) SHA1(1d484172465d3db6c4fc3733aa2b409e3a2e228f) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "3a-ac",        0x5800, 0x0800, CRC(4ea1c3d9) SHA1(d0c99c9d5b887d717c68e8745906ae4e65aec6ad) )
	ROM_LOAD( "4a-ac",        0x6000, 0x0800, CRC(5154c39e) SHA1(e6f011630eb1aa4116a0e5824ad6b65c1be2455f) )
	ROM_LOAD( "5a-ac",        0x6800, 0x0800, CRC(1e1e3916) SHA1(867e586583e07cd01e0e852f6ea52a040995725d) )
	ROM_LOAD( "6a-ac",        0x7000, 0x0800, CRC(80f3357a) SHA1(f1ee638251e8676a526e6367c11866b1d52f5910) )
	ROM_LOAD( "7a-ac",        0x7800, 0x0800, CRC(466addc7) SHA1(0230b5365d6aeee3ca47666a9eadee4141de125b) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "11d-cpu",      0x0000, 0x0800, CRC(b4bb2503) SHA1(67303603b7c5e6301e976ef19f81c7519648b179) )
ROM_END

ROM_START( venture2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "vent_a13.cpu", 0x8000, 0x1000, CRC(4c833f99) SHA1(1ff4eafe48b9f0ab8a123659d78c3dfa0bf56d7d) )
	ROM_LOAD( "vent_a12.cpu", 0x9000, 0x1000, CRC(8163cefc) SHA1(7061819dd1105e8368c045dad2effae62d124539) )
	ROM_LOAD( "vent_a11.cpu", 0xa000, 0x1000, CRC(324a5054) SHA1(f845ff2f717ea627891e0dc9d6e66f690c0843d8) )
	ROM_LOAD( "vent_a10.cpu", 0xb000, 0x1000, CRC(24358203) SHA1(10c3ea83a892d6fd2751e590afe45bffa65bd6e0) )
	ROM_LOAD( "vent_a9.cpu",  0xc000, 0x1000, CRC(04428165) SHA1(6d8d860ce1f805ba2eb315f47c8660799256e921) )
	ROM_LOAD( "vent_a8.cpu",  0xd000, 0x1000, CRC(4c1a702a) SHA1(7f6a68d3cfdd885108eebb7ea76b3c2ce6070b18) )
	ROM_LOAD( "vent_a7.cpu",  0xe000, 0x1000, CRC(1aab27c2) SHA1(66c7274dbb8bda3c78cc61d96a6cb1a9b29939b5) )
	ROM_LOAD( "vent_a6.cpu",  0xf000, 0x1000, CRC(767bdd71) SHA1(334a903e05fc86186f90aa2d9ce3b0d367d7e516) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "3a-ac",        0x5800, 0x0800, CRC(4ea1c3d9) SHA1(d0c99c9d5b887d717c68e8745906ae4e65aec6ad) )
	ROM_LOAD( "4a-ac",        0x6000, 0x0800, CRC(5154c39e) SHA1(e6f011630eb1aa4116a0e5824ad6b65c1be2455f) )
	ROM_LOAD( "5a-ac",        0x6800, 0x0800, CRC(1e1e3916) SHA1(867e586583e07cd01e0e852f6ea52a040995725d) )
	ROM_LOAD( "6a-ac",        0x7000, 0x0800, CRC(80f3357a) SHA1(f1ee638251e8676a526e6367c11866b1d52f5910) )
	ROM_LOAD( "7a-ac",        0x7800, 0x0800, CRC(466addc7) SHA1(0230b5365d6aeee3ca47666a9eadee4141de125b) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "11d-cpu",      0x0000, 0x0800, CRC(b4bb2503) SHA1(67303603b7c5e6301e976ef19f81c7519648b179) )
ROM_END

ROM_START( venture4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "vel13a-4",     0x8000, 0x1000, CRC(1c5448f9) SHA1(59d3ca2a2d7048f5f7bd23fa5d9c9a2cc0734cb8) )
	ROM_LOAD( "vel12a-4",     0x9000, 0x1000, CRC(e62491cc) SHA1(a98b6c6e60d83fd4591d0de145a99c5e4576121a) )
	ROM_LOAD( "vel11a-4",     0xa000, 0x1000, CRC(e91faeaf) SHA1(ce50a9f1016671282d16f2d0ad3553598e0c7e89) )
	ROM_LOAD( "vel10a-4",     0xb000, 0x1000, CRC(da3a2991) SHA1(2b5175b0f3642e735b6d87fbd5b75118cf6b7faa) )
	ROM_LOAD( "vel9a-4",      0xc000, 0x1000, CRC(d1887b11) SHA1(40ed1e1bdcb95d6e317cb5e4fb8572a314b3fbf8) )
	ROM_LOAD( "vel8a-4",      0xd000, 0x1000, CRC(8e8153fc) SHA1(409cf0ed39ef04c1e9359f0499d7cba3aed8f36e) )
	ROM_LOAD( "vel7a-4",      0xe000, 0x1000, CRC(0a091701) SHA1(ffdea1d60371779d0c28fb3c6111639cace79dad) )
	ROM_LOAD( "vel6a-4",      0xf000, 0x1000, CRC(7b165f67) SHA1(4109797bcfd33c870234930790e3cecaaf90b706) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "vea3a-2",      0x5800, 0x0800, CRC(83b8836f) SHA1(ec0e2de62caea61ceff56e924449213997bff8cd) )
	ROM_LOAD( "4a-ac",        0x6000, 0x0800, CRC(5154c39e) SHA1(e6f011630eb1aa4116a0e5824ad6b65c1be2455f) )
	ROM_LOAD( "5a-ac",        0x6800, 0x0800, CRC(1e1e3916) SHA1(867e586583e07cd01e0e852f6ea52a040995725d) )
	ROM_LOAD( "6a-ac",        0x7000, 0x0800, CRC(80f3357a) SHA1(f1ee638251e8676a526e6367c11866b1d52f5910) )
	ROM_LOAD( "7a-ac",        0x7800, 0x0800, CRC(466addc7) SHA1(0230b5365d6aeee3ca47666a9eadee4141de125b) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vel11d-2",     0x0000, 0x0800, CRC(ea6fd981) SHA1(46b1658e1607423d5a073f14097c2a48d59057c0) )
ROM_END


ROM_START( pepper2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "main_12a",     0x9000, 0x1000, CRC(33db4737) SHA1(d8f7a5d340ddbc4d06d403c3bff0102ce637d24e) )
	ROM_LOAD( "main_11a",     0xa000, 0x1000, CRC(a1f43b1f) SHA1(a669f2ef55d9a0617110f65863822fdcaf153511) )
	ROM_LOAD( "main_10a",     0xb000, 0x1000, CRC(4d7d7786) SHA1(ea1390b887404a67ea556720219e81007b954a7d) )
	ROM_LOAD( "main_9a",      0xc000, 0x1000, CRC(b3362298) SHA1(7adad138ec5f94caa39f9c0fabece538d5db4913) )
	ROM_LOAD( "main_8a",      0xd000, 0x1000, CRC(64d106ed) SHA1(49646a97def9e1793cac6ee0044f68232b294e4f) )
	ROM_LOAD( "main_7a",      0xe000, 0x1000, CRC(b1c6f07c) SHA1(53d07211d014336bb43671c51f4190c6515e9cde) )
	ROM_LOAD( "main_6a",      0xf000, 0x1000, CRC(515b1046) SHA1(bdcccd4e415c00ee8e5ec185597df75ecafe7d3d) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "audio_5a",     0x6800, 0x0800, CRC(90e3c781) SHA1(d51a9e011167a132e8af9f4b1201600a58e86b62) )
	ROM_LOAD( "audio_6a",     0x7000, 0x0800, CRC(dd343e34) SHA1(4ec55bb73d6afbd167fa91d2606d1d55a15b5c39) )
	ROM_LOAD( "audio_7a",     0x7800, 0x0800, CRC(e02b4356) SHA1(9891e14d84221c1d6f2d15a29813eb41024290ca) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "main_11d",     0x0000, 0x0800, CRC(b25160cd) SHA1(3d768552960a3a660891dcb85da6a5c382b33991) )
ROM_END


ROM_START( hardhat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "hhl-2.11a",    0xa000, 0x1000, CRC(7623deea) SHA1(3c47c0439c80e66536af42c5ee4e522fea5f8374) )
	ROM_LOAD( "hhl-2.10a",    0xb000, 0x1000, CRC(e6bf2fb1) SHA1(ad41859129774fc51462726a825c0ae16ed81a6e) )
	ROM_LOAD( "hhl-2.9a",     0xc000, 0x1000, CRC(acc2bce5) SHA1(0f7b8cfbd2628b8587c423fbc2c8310d71d8ad2a) )
	ROM_LOAD( "hhl-2.8a",     0xd000, 0x1000, CRC(23c7a2f8) SHA1(5eb1d512d73ba6bd1c23501664b582e9d3cf777f) )
	ROM_LOAD( "hhl-2.7a",     0xe000, 0x1000, CRC(6f7ce1c2) SHA1(356dcea22e50c95a8552566a0fb5f9b4e3e5de2a) )
	ROM_LOAD( "hhl-2.6a",     0xf000, 0x1000, CRC(2a20cf10) SHA1(31eb4556647e78e3d9be1c30d970eac8aaa5cf18) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "hha-1.5a",     0x6800, 0x0800, CRC(16a5a183) SHA1(cf3fed55db9c61fd33c222275d472fa109bed081) )
	ROM_LOAD( "hha-1.6a",     0x7000, 0x0800, CRC(bde64021) SHA1(a403590d5a27b859eaa299e47df4ebd6ce4a5772) )
	ROM_LOAD( "hha-1.7a",     0x7800, 0x0800, CRC(505ee5d3) SHA1(efa228465688f2bb30f00dc1511cc5f3a287356c) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hhl-1.11d",    0x0000, 0x0800, CRC(dbcdf353) SHA1(76ea287326a5c9e75e407cc010414212d8fdd52a) )
ROM_END

ROM_START( teetert )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "13a-cpu",      0x8000, 0x1000, CRC(f4e4d991) SHA1(6683c1552b56b20f2296e461aff697af73563792) )
	ROM_LOAD( "12a-cpu",      0x9000, 0x1000, CRC(c6d8cb04) SHA1(3b9ae8fdc35117c73c91daed66e93e5344bdcd7e) )
	ROM_LOAD( "11a-cpu",      0xa000, 0x1000, CRC(bac9b259) SHA1(0265cbd683fadf42f8a6b71958cbe782a732c257) )
	ROM_LOAD( "10a-cpu",      0xb000, 0x1000, CRC(3ae7e445) SHA1(e511ce4c553ac58e87b6ee623f8c42d7653de972) )
	ROM_LOAD( "9a-cpu",       0xc000, 0x1000, CRC(0cba424d) SHA1(54377163a8b8082639baf56b960eb26268462d46) )
	ROM_LOAD( "8a-cpu",       0xd000, 0x1000, CRC(68de66e7) SHA1(70a0cc950f16f2c408fae717e6fdb75eb0fd8039) )
	ROM_LOAD( "7a-cpu",       0xe000, 0x1000, CRC(84491333) SHA1(db9f8e4c49057a4574a3784d71e627da7f7a4b44) )
	ROM_LOAD( "6a-cpu",       0xf000, 0x1000, CRC(3600d465) SHA1(84d633e042f73bfd6bf4a4d0ffee1cd2027c65d2) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "3a-ac",        0x5800, 0x0800, CRC(83b8836f) SHA1(ec0e2de62caea61ceff56e924449213997bff8cd) )
	ROM_LOAD( "4a-ac",        0x6000, 0x0800, CRC(5154c39e) SHA1(e6f011630eb1aa4116a0e5824ad6b65c1be2455f) )
	ROM_LOAD( "5a-ac",        0x6800, 0x0800, CRC(1e1e3916) SHA1(867e586583e07cd01e0e852f6ea52a040995725d) )
	ROM_LOAD( "6a-ac",        0x7000, 0x0800, CRC(80f3357a) SHA1(f1ee638251e8676a526e6367c11866b1d52f5910) )
	ROM_LOAD( "7a-ac",        0x7800, 0x0800, CRC(466addc7) SHA1(0230b5365d6aeee3ca47666a9eadee4141de125b) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "11d-cpu",      0x0000, 0x0800, CRC(0fe70b00) SHA1(6068be263d7a8e6b71af6f4dceec40bb8d246376) )
ROM_END

ROM_START( fax )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 64k for code + 192k for extra memory */
	ROM_LOAD( "fxl8-13a.32",  0x8000, 0x1000, CRC(8e30bf6b) SHA1(1fdf010da0258bc038554cf33c26e539a1f6b648) )
	ROM_LOAD( "fxl8-12a.32",  0x9000, 0x1000, CRC(60a41ff1) SHA1(1703dbedd09354d89c6014644d0ffe13ec657b8b) )
	ROM_LOAD( "fxl8-11a.32",  0xa000, 0x1000, CRC(2c9cee8a) SHA1(169045b4d840730cfbaa0b9a8a8d82907ea09d0c) )
	ROM_LOAD( "fxl8-10a.32",  0xb000, 0x1000, CRC(9b03938f) SHA1(af4c27b06a1f1be917316910b88d026b67cc60c0) )
	ROM_LOAD( "fxl8-9a.32",   0xc000, 0x1000, CRC(fb869f62) SHA1(cea6ff423c60662a1b36e9565940432707d5299b) )
	ROM_LOAD( "fxl8-8a.32",   0xd000, 0x1000, CRC(db3470bc) SHA1(7786f84ab41765ea91ab241d14a207044eda0e93) )
	ROM_LOAD( "fxl8-7a.32",   0xe000, 0x1000, CRC(1471fef5) SHA1(89308f3c2a0d7ea699e99622d37c5c95e3eaaf95) )
	ROM_LOAD( "fxl8-6a.32",   0xf000, 0x1000, CRC(812e39f3) SHA1(41c99f8483c69617f9c8dd82f979630ea9190454) )
	/* Banks of question ROMs */
	ROM_LOAD( "fxd-1c.64",  0x10000, 0x2000, CRC(fd7e3137) SHA1(6fda53737cd7c886c66c60436ae3ed5c62e6b178) )
	ROM_LOAD( "fxd-2c.64",  0x12000, 0x2000, CRC(e78cb16f) SHA1(d58dfa2385368ccf00ecfbaeccaf5ba82ef7da9b) )
	ROM_LOAD( "fxd-3c.64",  0x14000, 0x2000, CRC(57a94c6f) SHA1(fc27fe805c4cc29f797bfc0e4cd13a570ac5c1ec) )
	ROM_LOAD( "fxd-4c.64",  0x16000, 0x2000, CRC(9036c5a2) SHA1(b7a01e4002f615702cb691764cfae93707bf3c0f) )
	ROM_LOAD( "fxd-5c.64",  0x18000, 0x2000, CRC(38c03405) SHA1(c490252825dc3c4bf91255c7cb70a5ead92de85b) )
	ROM_LOAD( "fxd-6c.64",  0x1a000, 0x2000, CRC(f48fc308) SHA1(bfaf43e57a4d92b593d51d8cd61fe4d5c06e836c) )
	ROM_LOAD( "fxd-7c.64",  0x1c000, 0x2000, CRC(cf93b924) SHA1(892e6e6aa33bbcd271f5e0a63c1e8393df62f360) )
	ROM_LOAD( "fxd-8c.64",  0x1e000, 0x2000, CRC(607b48da) SHA1(6c8f2f207f3dd936c529b86cef917a0f0699a21c) )
	ROM_LOAD( "fxd-1b.64",  0x20000, 0x2000, CRC(62872d4f) SHA1(c020fdeae6c2e7d04c16048fdaa99ecf3e40af31) )
	ROM_LOAD( "fxd-2b.64",  0x22000, 0x2000, CRC(625778d0) SHA1(6c8d6b50653bff3774f5ccef0e000a2ef3f7030c) )
	ROM_LOAD( "fxd-3b.64",  0x24000, 0x2000, CRC(c3473dee) SHA1(8675f9b93bbbae4f5a5682c5b1623afeeacc0a4b) )
	ROM_LOAD( "fxd-4b.64",  0x26000, 0x2000, CRC(e39a15f5) SHA1(43b04cc2e4750b649116ade5b1004c2580293134) )
	ROM_LOAD( "fxd-5b.64",  0x28000, 0x2000, CRC(101a9d70) SHA1(2b839cd707e03b0e50037e1ffabcb8fe375dc4c0) )
	ROM_LOAD( "fxd-6b.64",  0x2a000, 0x2000, CRC(374a8f05) SHA1(ec41470932823242fff36ab6e6f158fa5c07d0a8) )
	ROM_LOAD( "fxd-7b.64",  0x2c000, 0x2000, CRC(f7e7f824) SHA1(1bed1ee07032b25675ace612a883cba4ab4b2f77) )
	ROM_LOAD( "fxd-8b.64",  0x2e000, 0x2000, CRC(8f1a5287) SHA1(a1102d49bacb25887eaa67ae64bcf64c8cad94fe) )
	ROM_LOAD( "fxd-1a.64",  0x30000, 0x2000, CRC(fc5e6344) SHA1(c61aad5100819f2fe98c3a159b64739fa6322d09) )
	ROM_LOAD( "fxd-2a.64",  0x32000, 0x2000, CRC(43cf60b3) SHA1(5169196d0a95450801b3a57703cb9f2861a25948) )
	ROM_LOAD( "fxd-3a.64",  0x34000, 0x2000, CRC(6b7d29cb) SHA1(fd4006efd24b33f8e2baf7f97d4b776d5ef90959) )
	ROM_LOAD( "fxd-4a.64",  0x36000, 0x2000, CRC(b9de3c2d) SHA1(229f9f0762d4d659acf516c2c1a42e70d2f98652) )
	ROM_LOAD( "fxd-5a.64",  0x38000, 0x2000, CRC(67285bc6) SHA1(f929c916fb19dbc91fc3a75dfed6375b63cb2043) )
	ROM_LOAD( "fxd-6a.64",  0x3a000, 0x2000, CRC(ba67b7b2) SHA1(12265f678b1e4dfc3b36a964f78b0103112753ee) )
	/* The last two ROM sockets were apparently never populated */
//  ROM_LOAD( "fxd-7a.64",  0x3c000, 0x2000, NO_DUMP )
//  ROM_LOAD( "fxd-8a.64",  0x3e000, 0x2000, NO_DUMP )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "fxa2-5a.16",   0x6800, 0x0800, CRC(7c525aec) SHA1(f3afd3bfc0ba4265106e6ca217d113d23ad66016) )
	ROM_LOAD( "fxa2-6a.16",   0x7000, 0x0800, CRC(2b3bfc44) SHA1(7e3b9133916c8121b2145942155601b3ade420da) )
	ROM_LOAD( "fxa2-7a.16",   0x7800, 0x0800, CRC(578c62b7) SHA1(1bcb987e8730c001b7339c3dfab2467bf76421c7) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fxl1-11d.32",  0x0000, 0x0800, CRC(62083db2) SHA1(0c6e90b73419bff53f991e66d4faa9495c7d8e09) )
ROM_END

ROM_START( faxa )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 64k for code + 192k for extra memory */
	ROM_LOAD( "fxl8-13a.32",  0x8000, 0x1000, CRC(8e30bf6b) SHA1(1fdf010da0258bc038554cf33c26e539a1f6b648) )
	ROM_LOAD( "fxl8-12a.32",  0x9000, 0x1000, CRC(60a41ff1) SHA1(1703dbedd09354d89c6014644d0ffe13ec657b8b) )
	ROM_LOAD( "fxl8-11a.32",  0xA000, 0x1000, CRC(2c9cee8a) SHA1(169045b4d840730cfbaa0b9a8a8d82907ea09d0c) )
	ROM_LOAD( "fxl8-10a.32",  0xB000, 0x1000, CRC(9b03938f) SHA1(af4c27b06a1f1be917316910b88d026b67cc60c0) )
	ROM_LOAD( "fxl8-9a.32",   0xC000, 0x1000, CRC(fb869f62) SHA1(cea6ff423c60662a1b36e9565940432707d5299b) )
	ROM_LOAD( "fxl8-8a.32",   0xD000, 0x1000, CRC(db3470bc) SHA1(7786f84ab41765ea91ab241d14a207044eda0e93) )
	ROM_LOAD( "fxl8-7a.32",   0xE000, 0x1000, CRC(1471fef5) SHA1(89308f3c2a0d7ea699e99622d37c5c95e3eaaf95) )
	ROM_LOAD( "fxl8-6a.32",   0xF000, 0x1000, CRC(812e39f3) SHA1(41c99f8483c69617f9c8dd82f979630ea9190454) )
	/* Banks of question ROMs */
	ROM_LOAD( "fxdb1-1c.bin", 0x10000, 0x2000, CRC(0e42a2a4) SHA1(f7021aad36b49338cdaae8e13638dbdd12327afc) )
	ROM_LOAD( "fxdb1-2c.bin", 0x12000, 0x2000, CRC(cef8d49a) SHA1(a3005ab21add22ffb4c6f4cb9843db8964fef49c) )
	ROM_LOAD( "fxdb1-3c.bin", 0x14000, 0x2000, CRC(96217b39) SHA1(fe80255c8763e714fbbf8b2a7ec481712fd463e8) )
	ROM_LOAD( "fxdb1-4c.bin", 0x16000, 0x2000, CRC(9f1522d8) SHA1(e79ef8f4642245f4d1c4d0fe14eb432a432e6f8a) )
	ROM_LOAD( "fxdb1-5c.bin", 0x18000, 0x2000, CRC(4770eb04) SHA1(c8123ba4fd66da471099cd364615a196c4d1ea94) )
	ROM_LOAD( "fxdb1-6c.bin", 0x1a000, 0x2000, CRC(07c742ab) SHA1(c709be4a21cc946f2de9061948fe095ec511447e) )
	ROM_LOAD( "fxdb1-7c.bin", 0x1c000, 0x2000, CRC(f2f39ebb) SHA1(879f0954331d219d44e82d4cf13f84c1d03dd6d5) )
	ROM_LOAD( "fxdb1-8c.bin", 0x1e000, 0x2000, CRC(00f73e30) SHA1(586c28d1ea14626ccd0e9195f26f18168bd0cf72) )
	ROM_LOAD( "fxdb1-1b.bin", 0x20000, 0x2000, CRC(e13341cf) SHA1(5cc2c9f060436026dbcbc6f893f8ef17f10b6c75) )
	ROM_LOAD( "fxdb1-2b.bin", 0x22000, 0x2000, CRC(300d7a6f) SHA1(4e28a522e14e0cf1b63be35e2a16b816d51f328c) )
	ROM_LOAD( "fxdb1-3b.bin", 0x24000, 0x2000, CRC(db9a6a3a) SHA1(d192f8ece73dfa12cd390c6e0218d5bf58c46074) )
	ROM_LOAD( "fxdb1-4b.bin", 0x26000, 0x2000, CRC(47faeb43) SHA1(7991341d78b1d0c9e35c2472dbb05d21c001ba7f) )
	ROM_LOAD( "fxdb1-5b.bin", 0x28000, 0x2000, CRC(3ecf974f) SHA1(d18b7bdbb5e490b0d4b70baae06cb44cf0f2b643) )
	ROM_LOAD( "fxdb1-6b.bin", 0x2a000, 0x2000, CRC(526c4c0d) SHA1(68480fb61d8e9e5386daeb94f5854913ab36b48d) )
	ROM_LOAD( "fxdb1-7b.bin", 0x2c000, 0x2000, CRC(4cf8217c) SHA1(fd7b9c34b0ae532209453d68f30f6f9fbcec1964) )
	ROM_LOAD( "fxdb1-8b.bin", 0x2e000, 0x2000, CRC(99485e27) SHA1(1f10e71507f111724a8b496f3cc57a6790116ac0) )
	ROM_LOAD( "fxdb1-1a.bin", 0x30000, 0x2000, CRC(97c153b5) SHA1(0d5e66f0ded4453e1fe81cdcc56697664f76f28d) )
	ROM_LOAD( "fxdb1-2a.bin", 0x32000, 0x2000, CRC(63258140) SHA1(cdb03f5130c72d286d1a2f227dcf646da6e8f40a) )
	ROM_LOAD( "fxdb1-3a.bin", 0x34000, 0x2000, CRC(1c698727) SHA1(6886bbf04f837fbda9a192e55e995cac9c9e1cad) )
	ROM_LOAD( "fxdb1-4a.bin", 0x36000, 0x2000, CRC(8283d6fc) SHA1(dba1f0f53a6b16f2ee7ce1e8e6e081a6be7586f6) )
	ROM_LOAD( "fxdb1-5a.bin", 0x38000, 0x2000, CRC(b69542d2) SHA1(67a102790b24a5638c8938579ace0b3dd9b0c953) )
	ROM_LOAD( "fxdb1-6a.bin", 0x3a000, 0x2000, CRC(ff949367) SHA1(fb3d9c0abe3c915eaea983d9b429eb5227688532) )
	ROM_LOAD( "fxdb1-7a.bin", 0x3c000, 0x2000, CRC(0f97b874) SHA1(5790d3ed9eed2ce05947bc28cc252f720a7f3aeb) )
	ROM_LOAD( "fxdb1-8a.bin", 0x3e000, 0x2000, CRC(1d055bea) SHA1(96531db0a3a36319bc0a28096e601302eb2eb115) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 ) /* 32k for audio */
	ROM_LOAD( "fxa2-5a.16",   0x6800, 0x0800, CRC(7c525aec) SHA1(f3afd3bfc0ba4265106e6ca217d113d23ad66016) )
	ROM_LOAD( "fxa2-6a.16",   0x7000, 0x0800, CRC(2b3bfc44) SHA1(7e3b9133916c8121b2145942155601b3ade420da) )
	ROM_LOAD( "fxa2-7a.16",   0x7800, 0x0800, CRC(578c62b7) SHA1(1bcb987e8730c001b7339c3dfab2467bf76421c7) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fxl1-11d.32",  0x0000, 0x0800, CRC(62083db2) SHA1(0c6e90b73419bff53f991e66d4faa9495c7d8e09) )
ROM_END



/*************************************
 *
 *  Driver init
 *
 *************************************/

static void exidy_common_init(UINT8 palette[], UINT16 colortable[], UINT8 cmask, UINT8 cinvert)
{
	exidy_palette 			= palette;
	exidy_colortable 		= colortable;
	exidy_collision_mask 	= cmask;
	exidy_collision_invert	= cinvert;
	targ_spec_flag 			= 0;
}


DRIVER_INIT( sidetrac )
{
	exidy_common_init(sidetrac_palette, exidy_1bpp_colortable, 0x00, 0x00);

	/* ROM in place of character RAM */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4800, 0x4bff, 0, 0, MWA8_ROM);
}


DRIVER_INIT( targ )
{
	exidy_common_init(targ_palette, exidy_1bpp_colortable, 0x00, 0x00);
	targ_spec_flag = 1;
}


DRIVER_INIT( spectar )
{
	exidy_common_init(spectar_palette, exidy_1bpp_colortable, 0x00, 0x00);
}

DRIVER_INIT( rallys )
{
	exidy_common_init(spectar_palette, exidy_1bpp_colortable, 0x00, 0x00);

	/* sprite locations are slightly different */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x5000, 0, 0, exidy_sprite1_xpos_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5001, 0x5001, 0, 0, exidy_sprite1_ypos_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5300, 0x5300, 0, 0, exidy_sprite2_xpos_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5301, 0x5301, 0, 0, exidy_sprite2_ypos_w);
}

DRIVER_INIT( phantoma )
{
	exidy_common_init(spectar_palette, exidy_1bpp_colortable, 0x00, 0x00);

	/* sprite locations are slightly different */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x5000, 0, 0, exidy_sprite1_xpos_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5001, 0x5001, 0, 0, exidy_sprite1_ypos_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5300, 0x5300, 0, 0, exidy_sprite2_xpos_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5301, 0x5301, 0, 0, exidy_sprite2_ypos_w);

	/* the ROM is actually mapped high */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, MRA8_BANK1);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0xf800);
}


DRIVER_INIT( mtrap )
{
	exidy_common_init(NULL, exidy_1bpp_colortable, 0x14, 0x00);
}


DRIVER_INIT( venture )
{
	exidy_common_init(NULL, exidy_1bpp_colortable, 0x04, 0x04);
}

DRIVER_INIT( teetert )
{
	exidy_common_init(NULL, exidy_1bpp_colortable, 0x0c, 0x0c);

	/* special input handler for the dial */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5101, 0x5101, 0, 0, teetert_input_r);
}

DRIVER_INIT( pepper2 )
{
	exidy_common_init(NULL, exidy_2bpp_colortable, 0x14, 0x04);

	/* two 6116 character RAMs */
	exidy_characterram = memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6fff, 0, 0, MRA8_RAM);
	exidy_characterram = memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6fff, 0, 0, exidy_characterram_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4800, 0x4fff, 0, 0, MRA8_NOP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4800, 0x4fff, 0, 0, MWA8_NOP);
}


DRIVER_INIT( fax )
{
	exidy_common_init(NULL, exidy_2bpp_colortable, 0x04, 0x04);

	/* reset the ROM bank */
	fax_bank_select_w(0,0);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1979, sidetrac, 0,       targ,    sidetrac, sidetrac, ROT0, "Exidy", "Side Track", 0 )
GAME( 1980, targ,     0,       targ,    targ,     targ,     ROT0, "Exidy", "Targ", 0 )
GAME( 1980, targc,    targ,    targ,    targ,     targ,     ROT0, "Exidy", "Targ (cocktail?)", 0 )
GAME( 1980, spectar,  0,       targ,    spectar,  spectar,  ROT0, "Exidy", "Spectar (revision 3)", 0 )
GAME( 1980, spectar1, spectar, targ,    spectar,  spectar,  ROT0, "Exidy", "Spectar (revision 1?)", 0 )
GAME( 1980, rallys,   spectar, targ,    rallys,   rallys,   ROT0, "Novar", "Rallys (bootleg?)", 0 )
GAME( 1980, phantoma, spectar, targ,    phantoma, phantoma, ROT0, "Jeutel","Phantomas", 0 )

GAME( 1981, mtrap,    0,       mtrap,   mtrap,    mtrap,    ROT0, "Exidy", "Mouse Trap (version 5)", 0 )
GAME( 1981, mtrap3,   mtrap,   mtrap,   mtrap,    mtrap,    ROT0, "Exidy", "Mouse Trap (version 3)", 0 )
GAME( 1981, mtrap4,   mtrap,   mtrap,   mtrap,    mtrap,    ROT0, "Exidy", "Mouse Trap (version 4)", 0 )
GAME( 1981, venture,  0,       venture, venture,  venture,  ROT0, "Exidy", "Venture (version 5 set 1)", 0 )
GAME( 1981, venture2, venture, venture, venture,  venture,  ROT0, "Exidy", "Venture (version 5 set 2)", 0 )
GAME( 1981, venture4, venture, venture, venture,  venture,  ROT0, "Exidy", "Venture (version 4)", 0 )
GAME( 1982, teetert,  0,       teetert, teetert,  teetert,  ROT0, "Exidy", "Teeter Torture (prototype)", 0 )
GAME( 1982, pepper2,  0,       pepper2, pepper2,  pepper2,  ROT0, "Exidy", "Pepper II", 0 )
GAME( 1982, hardhat,  0,       pepper2, pepper2,  pepper2,  ROT0, "Exidy", "Hard Hat", 0 )
GAME( 1983, fax,      0,       fax,     fax,      fax,      ROT0, "Exidy", "Fax", 0 )
GAME( 1983, faxa,     fax,     fax,     fax,      fax,      ROT0, "Exidy", "Fax (alt. questions)", 0 )

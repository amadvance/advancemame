/***************************************************************************

Daikaiju no Gyakushu / Land Sea Air Squad / Storming Party  (c) 1986 Taito

driver by Nicola Salmoria

TODO:
- I think storming is supposed to be a bootleg without mcu, so I should verify
  if it works with the mcu not hooked up.
- Wrong sprite/tilemap priority. Sprites can appear above and below the middle
  layer, it's not clear how this is selected since there are no free attribute
  bits.
  The priority seems to involve split transparency on the tilemap and also
  priority on sprites (so that people pass below doors but airplanes above).
  It is confirmed that priority is controlled by PROM a64-06.9 (grounding A9
  makes sprites disappear).
- Scrollram not entirely understood - it's most likely wrong, but more than
  enough to run this particular game.
- The video driver is pretty slow and could be optimized using temporary bitmaps
  (or tilemaps), however I haven't done that because the video circuitry is not
  entirely understood and if other games are found running on this hardware, they
  might not like the optimizations.
- Unknown writes to YM2203 output ports (filters?)

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/2203intf.h"

/* in vidhrdw/lsasquad.c */
extern unsigned char *lsasquad_scrollram;
VIDEO_UPDATE( lsasquad );
VIDEO_UPDATE( daikaiju );

/* in machine/lsasquad.c */
extern int lsasquad_invertcoin;
WRITE8_HANDLER( lsasquad_sh_nmi_disable_w );
WRITE8_HANDLER( lsasquad_sh_nmi_enable_w );
WRITE8_HANDLER( lsasquad_sound_command_w );
READ8_HANDLER( lsasquad_sh_sound_command_r );
WRITE8_HANDLER( lsasquad_sh_result_w );
READ8_HANDLER( lsasquad_sound_result_r );
READ8_HANDLER( lsasquad_sound_status_r );

READ8_HANDLER( lsasquad_68705_portA_r );
WRITE8_HANDLER( lsasquad_68705_portA_w );
WRITE8_HANDLER( lsasquad_68705_ddrA_w );
READ8_HANDLER( lsasquad_68705_portB_r );
WRITE8_HANDLER( lsasquad_68705_portB_w );
WRITE8_HANDLER( lsasquad_68705_ddrB_w );
WRITE8_HANDLER( lsasquad_mcu_w );
READ8_HANDLER( lsasquad_mcu_r );
READ8_HANDLER( lsasquad_mcu_status_r );

READ8_HANDLER( daikaiju_sound_status_r );
READ8_HANDLER( daikaiju_sh_sound_command_r );
READ8_HANDLER( daikaiju_mcu_status_r );


WRITE8_HANDLER( lsasquad_bankswitch_w )
{
	unsigned char *ROM = memory_region(REGION_CPU1);

	/* bits 0-2 select ROM bank */
	memory_set_bankptr(1,&ROM[0x10000 + 0x2000 * (data & 7)]);

	/* bit 3 is zeroed on startup, maybe reset sound CPU */

	/* bit 4 flips screen */
	flip_screen_set(data & 0x10);

	/* other bits unknown */
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xa000, 0xe5ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe800, 0xe800) AM_READ(input_port_0_r)	/* DSWA */
	AM_RANGE(0xe801, 0xe801) AM_READ(input_port_1_r)	/* DSWB */
	AM_RANGE(0xe802, 0xe802) AM_READ(input_port_2_r)	/* DSWC */
	AM_RANGE(0xe803, 0xe803) AM_READ(lsasquad_mcu_status_r)	/* COIN + 68705 status */
	AM_RANGE(0xe804, 0xe804) AM_READ(input_port_4_r)	/* IN0 */
	AM_RANGE(0xe805, 0xe805) AM_READ(input_port_5_r)	/* IN1 */
	AM_RANGE(0xe806, 0xe806) AM_READ(input_port_6_r)	/* START */
	AM_RANGE(0xe807, 0xe807) AM_READ(input_port_7_r)	/* SERVICE/TILT */
	AM_RANGE(0xec00, 0xec00) AM_READ(lsasquad_sound_result_r)
	AM_RANGE(0xec01, 0xec01) AM_READ(lsasquad_sound_status_r)
	AM_RANGE(0xee00, 0xee00) AM_READ(lsasquad_mcu_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(MWA8_RAM)	/* SRAM */
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram) AM_SIZE(&videoram_size)	/* SCREEN RAM */
	AM_RANGE(0xe000, 0xe3ff) AM_WRITE(MWA8_RAM) AM_BASE(&lsasquad_scrollram)	/* SCROLL RAM */
	AM_RANGE(0xe400, 0xe5ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)	/* OBJECT RAM */
	AM_RANGE(0xea00, 0xea00) AM_WRITE(lsasquad_bankswitch_w)
	AM_RANGE(0xec00, 0xec00) AM_WRITE(lsasquad_sound_command_w)
	AM_RANGE(0xee00, 0xee00) AM_WRITE(lsasquad_mcu_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa000, 0xa000) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0xd000, 0xd000) AM_READ(lsasquad_sh_sound_command_r)
	AM_RANGE(0xd800, 0xd800) AM_READ(lsasquad_sound_status_r)
	AM_RANGE(0xe000, 0xefff) AM_READ(MRA8_ROM)	/* space for diagnostic ROM? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(lsasquad_sh_result_w)
	AM_RANGE(0xd400, 0xd400) AM_WRITE(lsasquad_sh_nmi_disable_w)
	AM_RANGE(0xd800, 0xd800) AM_WRITE(lsasquad_sh_nmi_enable_w)
	AM_RANGE(0xe000, 0xefff) AM_WRITE(MWA8_ROM)	/* space for diagnostic ROM? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( m68705_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READ(lsasquad_68705_portA_r)
	AM_RANGE(0x0001, 0x0001) AM_READ(lsasquad_68705_portB_r)
	AM_RANGE(0x0002, 0x0002) AM_READ(lsasquad_mcu_status_r)
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( m68705_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(lsasquad_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(lsasquad_68705_portB_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(lsasquad_68705_ddrA_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE(lsasquad_68705_ddrB_w)
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



INPUT_PORTS_START( lsasquad )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "50000 100000" )
	PORT_DIPSETTING(    0x0c, "80000 150000" )
	PORT_DIPSETTING(    0x04, "100000 200000" )
	PORT_DIPSETTING(    0x00, "150000 300000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Japanese ) )

	PORT_START_TAG("DSWC")
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	PORT_START_TAG("MCU?")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 68705 ready to receive cmd */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 0 = 68705 has sent result */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* DAIKAIJU */

static ADDRESS_MAP_START( mem_daikaiju, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xa000, 0xbfff) AM_RAM	/* SRAM */
	AM_RANGE(0xc000, 0xdfff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)	/* SCREEN RAM */
	AM_RANGE(0xe000, 0xe3ff) AM_RAM AM_BASE(&lsasquad_scrollram)	/* SCROLL RAM */
	AM_RANGE(0xe400, 0xe7ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)	/* OBJECT RAM */
	AM_RANGE(0xe800, 0xe800) AM_READ(input_port_0_r)	/* DSWA */
	AM_RANGE(0xe801, 0xe801) AM_READ(input_port_1_r)	/* DSWB */
	AM_RANGE(0xe803, 0xe803) AM_READ(daikaiju_mcu_status_r)	/* COIN + 68705 status */
	AM_RANGE(0xe804, 0xe804) AM_READ(input_port_4_r)	/* IN0 */
	AM_RANGE(0xe805, 0xe805) AM_READ(input_port_5_r)	/* IN1 */
	AM_RANGE(0xe806, 0xe806) AM_READ(input_port_6_r)	/* START */
	AM_RANGE(0xe807, 0xe807) AM_READ(input_port_7_r)	/* SERVICE/TILT */
	AM_RANGE(0xea00, 0xea00) AM_WRITE(lsasquad_bankswitch_w)
	AM_RANGE(0xec00, 0xec00) AM_WRITE(lsasquad_sound_command_w)
	AM_RANGE(0xec01, 0xec01) AM_READ(lsasquad_sound_status_r)
	AM_RANGE(0xee00, 0xee00) AM_READWRITE(lsasquad_mcu_r, lsasquad_mcu_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_mem_daikaiju, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0xa001, 0xa001) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xd000, 0xd000) AM_READ(daikaiju_sh_sound_command_r)
	AM_RANGE(0xd400, 0xd400) AM_WRITENOP
	AM_RANGE(0xd800, 0xd800) AM_READ(daikaiju_sound_status_r) AM_WRITENOP
	AM_RANGE(0xdc00, 0xdc00) AM_WRITENOP
	AM_RANGE(0xe000, 0xefff) AM_ROM	/* space for diagnostic ROM? */
ADDRESS_MAP_END

INPUT_PORTS_START( daikaiju )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW ) //test mode
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) // detailed description at the top of file
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "100000" )
	PORT_DIPSETTING(    0x0c, "30000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "Infinite (Cheat)" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" ) //stop mode
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSWC")
	//unused


	PORT_START_TAG("MCU?")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 68705 ready to receive cmd */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 0 = 68705 has sent result */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			16*8+3, 16*8+2, 16*8+1, 16*8+0, 16*8+8+3, 16*8+8+2, 16*8+8+1, 16*8+8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 256, 16 },
	{ -1 }	/* end of array */
};



static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE8_HANDLER( unk )
{

}


static struct YM2203interface ym2203_interface =
{
	0,
	0,
	unk,
	unk,
	irqhandler
};


static MACHINE_DRIVER_START( lsasquad )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */	/* 4 MHz? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
								/* IRQs are triggered by the YM2203 */
	MDRV_CPU_ADD(M68705,4000000/2)	/* ? */
	MDRV_CPU_PROGRAM_MAP(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(500)	/* 500 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */
							/* main<->sound synchronization depends on this */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_UPDATE(lsasquad)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( daikaiju )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)
	MDRV_CPU_PROGRAM_MAP(mem_daikaiju, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_mem_daikaiju, 0)
	/* IRQs are triggered by the YM2203 */
	
	MDRV_CPU_ADD(M68705,4000000/2)	/* ? */
	MDRV_CPU_PROGRAM_MAP(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(500)	/* 500 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */
							/* main<->sound synchronization depends on this */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_UPDATE(daikaiju)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 1.0)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( lsasquad )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "a64-21.4",     0x00000, 0x8000, CRC(5ff6b017) SHA1(96cc74edba1208bb8e82f93d2d3a88ea24922dc0) )
    /* ROMs banked at 8000-9fff */
	ROM_LOAD( "a64-20.3",     0x10000, 0x8000, CRC(7f8b4979) SHA1(975b1a678e1f7d7b5789565063177593639645ce) )
	ROM_LOAD( "a64-19.2",     0x18000, 0x8000, CRC(ba31d34a) SHA1(e2c515ae8146a37534b19403c03fc5a8719f115f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a64-04.44",    0x0000, 0x8000, CRC(c238406a) SHA1(bb8f9d952c4568edb375328a1f9f6681a1bb5907) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a64-05.35",    0x0000, 0x0800, CRC(572677b9) SHA1(e098d5d842bcc81221ba56652a7019505d8be082) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a64-10.27",    0x00000, 0x8000, CRC(bb4f1b37) SHA1(ce8dc962a3d04a624e36b57dc678e7ca7726ba1d) )
	ROM_LOAD( "a64-22.28",    0x08000, 0x8000, CRC(58e03b89) SHA1(ccec83bcd7cb2be3ba46e9fbc7952349fa8faadf) )
	ROM_LOAD( "a64-11.40",    0x10000, 0x8000, CRC(a3bbc0b3) SHA1(f565d323575af3c2e95412c50130e88954fc238c) )
	ROM_LOAD( "a64-23.41",    0x18000, 0x8000, CRC(377a538b) SHA1(1174838309a331ffec7b60d6ceaa98a02fdbe210) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a64-14.2",     0x00000, 0x8000, CRC(a72e2041) SHA1(c537d1620fe8562aef39a0279b35139eb0668bf9) )
	ROM_LOAD( "a64-16.3",     0x08000, 0x8000, CRC(05206333) SHA1(a7463279446de9d633ea18f1e1eb5f610d982a37) )
	ROM_LOAD( "a64-15.25",    0x10000, 0x8000, CRC(01ed5851) SHA1(6034376d30d1d17fe9aab07cb40009c4f3c03690) )
	ROM_LOAD( "a64-17.26",    0x18000, 0x8000, CRC(6eaf3735) SHA1(a91fd7c9a6f2f58d311e40edc29d1e4f97746146) )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "a64-07.22",    0x0000, 0x0400, CRC(82802bbb) SHA1(4f54c9364a12809898eabd1eb13d16a6c9f0f532) )	/* red   (bottom half unused) */
	ROM_LOAD( "a64-08.23",    0x0200, 0x0400, CRC(aa9e1dbd) SHA1(be7dfabf5306747fa3d5f1f735d0064673f19c91) )	/* green (bottom half unused) */
	ROM_LOAD( "a64-09.24",    0x0400, 0x0400, CRC(dca86295) SHA1(a6f6af60caaad9f49d72a8c2ff1e6115471f8c63) )	/* blue  (bottom half unused) */
	ROM_LOAD( "a64-06.9",     0x0600, 0x0400, CRC(7ced30ba) SHA1(f22de13d4fd49b7b2ffd06032eb5e14fbdeec91c) )	/* priority */
ROM_END

ROM_START( storming )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "stpartyj.001", 0x00000, 0x8000, CRC(07e6bc61) SHA1(6989a1401868dd93c9466cfd1636ac48a734a5d4) )
    /* ROMs banked at 8000-9fff */
	ROM_LOAD( "stpartyj.002", 0x10000, 0x8000, CRC(1c7fe5d5) SHA1(15c09e3301d8ce55e59fe90db9f50ee19584ab7b) )
	ROM_LOAD( "stpartyj.003", 0x18000, 0x8000, CRC(159f23a6) SHA1(2cb4ed78e54dc2acbbfc2d4cfb2d29ff604aa9ae) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a64-04.44",    0x0000, 0x8000, CRC(c238406a) SHA1(bb8f9d952c4568edb375328a1f9f6681a1bb5907) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a64-05.35",    0x0000, 0x0800, CRC(572677b9) SHA1(e098d5d842bcc81221ba56652a7019505d8be082) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a64-10.27",    0x00000, 0x8000, CRC(bb4f1b37) SHA1(ce8dc962a3d04a624e36b57dc678e7ca7726ba1d) )
	ROM_LOAD( "stpartyj.009", 0x08000, 0x8000, CRC(8ee2443b) SHA1(855d8189efcfc796daa6b36f86d2872cc48adfde) )
	ROM_LOAD( "a64-11.40",    0x10000, 0x8000, CRC(a3bbc0b3) SHA1(f565d323575af3c2e95412c50130e88954fc238c) )
	ROM_LOAD( "stpartyj.011", 0x18000, 0x8000, CRC(f342d42f) SHA1(ef9367ad9763f4b38e0f12805c1eee7c430758c2) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a64-14.2",     0x00000, 0x8000, CRC(a72e2041) SHA1(c537d1620fe8562aef39a0279b35139eb0668bf9) )
	ROM_LOAD( "a64-16.3",     0x08000, 0x8000, CRC(05206333) SHA1(a7463279446de9d633ea18f1e1eb5f610d982a37) )
	ROM_LOAD( "a64-15.25",    0x10000, 0x8000, CRC(01ed5851) SHA1(6034376d30d1d17fe9aab07cb40009c4f3c03690) )
	ROM_LOAD( "a64-17.26",    0x18000, 0x8000, CRC(6eaf3735) SHA1(a91fd7c9a6f2f58d311e40edc29d1e4f97746146) )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "a64-07.22",    0x0000, 0x0400, CRC(82802bbb) SHA1(4f54c9364a12809898eabd1eb13d16a6c9f0f532) )	/* red   (bottom half unused) */
	ROM_LOAD( "a64-08.23",    0x0200, 0x0400, CRC(aa9e1dbd) SHA1(be7dfabf5306747fa3d5f1f735d0064673f19c91) )	/* green (bottom half unused) */
	ROM_LOAD( "a64-09.24",    0x0400, 0x0400, CRC(dca86295) SHA1(a6f6af60caaad9f49d72a8c2ff1e6115471f8c63) )	/* blue  (bottom half unused) */
	ROM_LOAD( "a64-06.9",     0x0600, 0x0400, CRC(7ced30ba) SHA1(f22de13d4fd49b7b2ffd06032eb5e14fbdeec91c) )	/* priority */
ROM_END

ROM_START( daikaiju )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "a74_01-1.ic4",   0x00000, 0x8000, CRC(89c13d7f) SHA1(2eaec80d7aa360b700387df00b37a692acc50d74) )
    /* ROMs banked at 8000-9fff */
	ROM_LOAD( "a74_02.ic3",     0x10000, 0x8000, CRC(8ddf6131) SHA1(b5b23550e7ee52554bc1f045ed6f42e254a05bf4) )
	ROM_LOAD( "a74_03.ic2",     0x18000, 0x8000, CRC(3911ffed) SHA1(ba6dbd74d37ef26621a02baf3479e2764d10d2ba) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a74_04.ic44",    0x0000, 0x8000, CRC(98a6a703) SHA1(0c169a7a5f8b26606f67ee7f14bd487951536ac5) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )
    ROM_LOAD( "a74_05.ic35",    0x0000, 0x0800, CRC(d66df06f) SHA1(6a61eb15aef7f3b7a66ec9d87c0bdd731d6cb079) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a74_10.ic27",    0x00000, 0x8000, CRC(3123158e) SHA1(cdebf63c283c5c042596b0a13361fd01245e9c42) )
	ROM_LOAD( "a74_12.ic28",    0x08000, 0x8000, CRC(8a4e6c3a) SHA1(85b5c8630fe9d4faea6787f80a66ee41da64e64b) )
	ROM_LOAD( "a74_11.ic40",    0x10000, 0x8000, CRC(6432ae38) SHA1(5514c5259c5ced393b1c39436025dd13c0c61d82) )
	ROM_LOAD( "a74_13.ic41",    0x18000, 0x8000, CRC(1a1be4bb) SHA1(cbe647b2291db6432ea2cb61b8108cc089adb3c7) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a74_14.ic2",     0x00000, 0x8000, CRC(c28e9c35) SHA1(e9c697a91e5281ab08a43169004c235ada9391db) )
	ROM_LOAD( "a74_16.ic3",     0x08000, 0x8000, CRC(4b1c7921) SHA1(37e26a9007bfdf71af021fb218ea2b16f91d9c37) )
	ROM_LOAD( "a74_16.ic25",    0x10000, 0x8000, CRC(ef4d1945) SHA1(6b5e898e486d5786fc5d151f1fcca0015829365d) )
	ROM_LOAD( "a74_17.ic26",    0x18000, 0x8000, CRC(d1077878) SHA1(e69893db6b63d5a5192b521d61a86f60b7029b7e) )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "a74_07.ic22",    0x0000, 0x0400, CRC(66132341) SHA1(8c6723dfc4f856ef27998411a98c40783d13ac41) )	/* red   (bottom half unused) */
	ROM_LOAD( "a74_08.ic23",    0x0200, 0x0400, CRC(fb3f0273) SHA1(591577c94865e2e6465e0016350450a19000e52d) )	/* green (bottom half unused) */
	ROM_LOAD( "a74_09.ic24",    0x0400, 0x0400, CRC(bed6709d) SHA1(ba5435728d6b7847bc86878f6122ce1f86982f0a) )	/* blue  (bottom half unused) */
	ROM_LOAD( "a74_06.ic9",     0x0600, 0x0400, CRC(cad554e7) SHA1(7890d948bfef198309df810f8401d224224a73a1) )	/* priority */
ROM_END

static void init_common(void)
{
	unsigned char *ROM = memory_region(REGION_CPU1);

	/* an instruction at $7FFF straddles the bank switch boundary at
       $8000 into rom bank #0 and then continues into the bank so
       copy this bank as the CPU bank switching won't catch it */
	memcpy(&ROM[0x08000], &ROM[0x10000], 0x2000);
}

/* coin inputs are inverted in storming */
static DRIVER_INIT( lsasquad ) { lsasquad_invertcoin = 0x00; init_common(); }
static DRIVER_INIT( storming ) { lsasquad_invertcoin = 0x0c; init_common(); }
static DRIVER_INIT( daikaiju ) { init_common(); }


GAME( 1986, lsasquad, 0,        lsasquad, lsasquad, lsasquad, ROT270, "Taito", "Land Sea Air Squad / Riku Kai Kuu Saizensen", GAME_IMPERFECT_GRAPHICS )
GAME( 1986, storming, lsasquad, lsasquad, lsasquad, storming, ROT270, "Taito", "Storming Party / Riku Kai Kuu Saizensen", GAME_IMPERFECT_GRAPHICS )
GAME( 1986, daikaiju, 0,	daikaiju, daikaiju, daikaiju, ROT270, "Taito", "Daikaiju no Gyakushu", GAME_IMPERFECT_GRAPHICS )

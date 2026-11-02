// license:GPL-2.0+
// copyright-holders:Jarek Burczynski, Phil Stroffolino, Tomasz Slanina, Adam Bousley
/**************************************************************************

Taito America's Change Lanes

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"
#include "sound/ay8910.h"

static void changela_video_reset(void);
VIDEO_START( changela );
VIDEO_UPDATE( changela );

WRITE8_HANDLER( changela_colors_w );
WRITE8_HANDLER( changela_mem_device_select_w );
WRITE8_HANDLER( changela_mem_device_w );
READ8_HANDLER( changela_mem_device_r );
WRITE8_HANDLER( changela_slope_rom_addr_hi_w );
WRITE8_HANDLER( changela_slope_rom_addr_lo_w );

static UINT8 portA_in,portA_out,ddrA;
static UINT8 portB_out,ddrB;
static UINT8 portC_in,portC_out,ddrC;

static UINT8 mcu_out;
static UINT8 mcu_in;
static UINT8 mcu_PC1;
static UINT8 mcu_PC0;

UINT8 changela_tree0_col;
UINT8 changela_tree1_col;
UINT8 changela_left_bank_col;
UINT8 changela_right_bank_col;
UINT8 changela_boat_shore_col;
UINT8 changela_collision_reset;
UINT8 changela_tree_collision_reset;

static MACHINE_RESET (changela)
{
	mcu_PC1=0;
	mcu_PC0=0;
	portA_in = portA_out = portC_out = mcu_in = mcu_out = 0xff;
	ddrA = ddrB = ddrC = 0;
	portC_in = 0xfe;
	changela_video_reset();

	changela_tree0_col = 0;
	changela_tree1_col = 0;
	changela_left_bank_col = 0;
	changela_right_bank_col = 0;
	changela_boat_shore_col = 0;
	changela_collision_reset = 0;
	changela_tree_collision_reset = 0;
}

static READ8_HANDLER( mcu_r )
{

	return mcu_out & ((portC_out & 4) ? 0xff : mcu_in);
}

static WRITE8_HANDLER( mcu_w )
{
	mcu_in = data;
	if (!(portC_out & 4)) portA_in = data;
}

static READ8_HANDLER( changela_68705_portA_r )
{
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

static WRITE8_HANDLER( changela_68705_portA_w )
{
	portA_out = data;
}

static WRITE8_HANDLER( changela_68705_ddrA_w )
{
	ddrA = data;
}

static READ8_HANDLER( changela_68705_portB_r )
{
	return (portB_out & ddrB) | (readinputport(4) & ~ddrB);
}

static WRITE8_HANDLER( changela_68705_portB_w )
{
	portB_out = data;
}

static WRITE8_HANDLER( changela_68705_ddrB_w )
{
	ddrB = data;
}

static READ8_HANDLER( changela_68705_portC_r )
{
	return (portC_out & ddrC) | (portC_in & ~ddrC);
}

static WRITE8_HANDLER( changela_68705_portC_w )
{
	/* the reply latch clocks only on the rising edge; input pull-ups remain active */
	if ((data & 8) && !(portC_out & 8)) mcu_out = portA_out;
	portA_in = (data & 4) ? 0xff : mcu_in;
	portC_out = data;
}

static WRITE8_HANDLER( changela_68705_ddrC_w )
{
	ddrC = data;
}

static ADDRESS_MAP_START( mcu_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READ(changela_68705_portA_r)
	AM_RANGE(0x0001, 0x0001) AM_READ(changela_68705_portB_r)
	AM_RANGE(0x0002, 0x0002) AM_READ(changela_68705_portC_r)

	AM_RANGE(0x0000, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(changela_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(changela_68705_portB_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE(changela_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(changela_68705_ddrA_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE(changela_68705_ddrB_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(changela_68705_ddrC_w)

	AM_RANGE(0x0000, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static READ8_HANDLER( changela_24_r )
{
	return ((portC_out&2)<<2) | 7;
}

static READ8_HANDLER( changela_25_r )
{

	return (changela_tree1_col << 3) | (changela_tree0_col << 2) | 0x03;
}

static READ8_HANDLER( changela_30_r )
{
	return readinputport(7) & 0x0f;
}

static READ8_HANDLER( changela_31_r )
{

	static UINT8 prev_value = 0;
	UINT8 curr_value = readinputport(7);
	static int dir = 0;

	if( (curr_value < prev_value && (prev_value - curr_value) < 0x80)
	||  (curr_value > prev_value && (curr_value - prev_value) > 0x80) )
		dir = 1;
	if( (prev_value < curr_value && (curr_value - prev_value) < 0x80)
	||  (prev_value > curr_value && (prev_value - curr_value) > 0x80) )
		dir = 0;

	prev_value = curr_value;

	return (dir << 3) | (changela_left_bank_col << 2) | (changela_right_bank_col << 1) | changela_boat_shore_col;
}

static READ8_HANDLER( changela_2c_r )
{
	return readinputport(5);
}

static READ8_HANDLER( changela_2d_r )
{
	static const UINT8 gas[3] = { 1, 0, 2 };
	return (readinputport(6) & 0x20) | (gas[readinputport(8) % 3] << 6) | (BIT((cpu_getscanline() + 32) % 262, 8) << 4);
}

static WRITE8_HANDLER( mcu_PC0_w )
{
	portC_in = (portC_in&0xfe) | (data&1);
}

static WRITE8_HANDLER( changela_collision_reset_0 )
{
	changela_collision_reset = data & 0x01;
}

static WRITE8_HANDLER( changela_collision_reset_1 )
{
	changela_tree_collision_reset = data & 0x01;
}

static WRITE8_HANDLER( changela_coin_counter_w )
{
	coin_counter_w(offset,data);
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9000, 0x97ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xb000, 0xbfff) AM_READ(MRA8_ROM)

	AM_RANGE(0xc000, 0xc7ff) AM_READ(changela_mem_device_r)

	AM_RANGE(0xd000, 0xd000) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0xd010, 0xd010) AM_READ(AY8910_read_port_1_r)

	AM_RANGE(0xd024, 0xd024) AM_READ(changela_24_r)
	AM_RANGE(0xd025, 0xd025) AM_READ(changela_25_r)
	AM_RANGE(0xd028, 0xd028) AM_READ(mcu_r)
	AM_RANGE(0xd02c, 0xd02c) AM_READ(changela_2c_r)
	AM_RANGE(0xd02d, 0xd02d) AM_READ(changela_2d_r)

	AM_RANGE(0xd030, 0xd030) AM_READ(changela_30_r)
	AM_RANGE(0xd031, 0xd031) AM_READ(changela_31_r)

	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)
	AM_RANGE(0x9000, 0x97ff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram)
	AM_RANGE(0xa000, 0xa07f) AM_WRITE(changela_colors_w) AM_BASE(&colorram)
	AM_RANGE(0xb000, 0xbfff) AM_WRITE(MWA8_ROM)

	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(changela_mem_device_w)

	AM_RANGE(0xc800, 0xc800) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc900, 0xc900) AM_WRITE(changela_mem_device_select_w)
	AM_RANGE(0xca00, 0xca00) AM_WRITE(changela_slope_rom_addr_hi_w)
	AM_RANGE(0xcb00, 0xcb00) AM_WRITE(changela_slope_rom_addr_lo_w)

	AM_RANGE(0xd000, 0xd000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xd010, 0xd010) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0xd011, 0xd011) AM_WRITE(AY8910_write_port_1_w)

	AM_RANGE(0xd020, 0xd020) AM_WRITE(changela_collision_reset_0)
	AM_RANGE(0xd021, 0xd022) AM_WRITE(changela_coin_counter_w)

	AM_RANGE(0xd024, 0xd024) AM_WRITE(mcu_PC0_w)
	AM_RANGE(0xd025, 0xd025) AM_WRITE(changela_collision_reset_1)
	AM_RANGE(0xd026, 0xd026) AM_WRITE(MWA8_NOP)

	AM_RANGE(0xd030, 0xd030) AM_WRITE(mcu_w)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static INPUT_PORTS_START( changela )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x07, 0x01, "Steering Wheel Ratio" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Ignore Memory Failures" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Controls ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Joystick ) )
	PORT_DIPSETTING(    0x00, "Steering Wheel" )
	PORT_DIPNAME( 0x40, 0x40, "Diagnostic" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Players ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x03, 0x00, "Max Bonus Fuels" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x00, "99" )
	PORT_DIPNAME( 0x0c, 0x08, "Game Difficulty" )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x30, 0x20, "Traffic Difficulty" )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x40, 0x00, "Land Collisions" )
	PORT_DIPSETTING(    0x40, "Off (Cheat)" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Car Collision" )
	PORT_DIPSETTING(    0x80, "Off (Cheat)" )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSWC")
	PORT_DIPNAME( 0xf0, 0x10, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x0f, 0x01, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )

	PORT_START_TAG("DSWD")
	PORT_DIPNAME( 0x01, 0x01, "Right Slot" )
	PORT_DIPSETTING(    0x01, "On Right (Bottom) Counter" )
	PORT_DIPSETTING(    0x00, "On Left (Top) Counter" )
	PORT_DIPNAME( 0x02, 0x00, "Left Slot" )
	PORT_DIPSETTING(    0x02, "On Right (Bottom) Counter" )
	PORT_DIPSETTING(    0x00, "On Left (Top) Counter" )
	PORT_DIPNAME( 0x1c, 0x00, "Credits For Bonus" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x14, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_DIPSETTING(    0x1c, "7" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "'King Of The World' Name Length" )
	PORT_DIPSETTING(    0x40, "3 Letters" )
	PORT_DIPSETTING(    0x00, "Long" )
	PORT_DIPNAME( 0x80, 0x00, "'King Of The World' Name" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("MCU")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_CONDITION("DSWA", 0x20, PORTCOND_EQUALS, 0x00)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_CONDITION("DSWA", 0x20, PORTCOND_EQUALS, 0x00)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_CONDITION("DSWA", 0x20, PORTCOND_EQUALS, 0x20)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_CONDITION("DSWA", 0x20, PORTCOND_EQUALS, 0x20)
	PORT_BIT( 0x40, 0x40, IPT_SPECIAL ) PORT_CONDITION("GEAR", 0x01, PORTCOND_EQUALS, 0x00)
	PORT_BIT( 0x80, 0x80, IPT_SPECIAL ) PORT_CONDITION("GEAR", 0x01, PORTCOND_EQUALS, 0x01)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START_TAG("WHEEL")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_MINMAX(0x00, 0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(8) PORT_CONDITION("DSWA", 0x20, PORTCOND_EQUALS, 0x00)

	PORT_START_TAG("GAS")
	PORT_BIT( 0x03, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00, 0x02) PORT_SENSITIVITY(10) PORT_KEYDELTA(1) PORT_CONDITION("DSWA", 0x20, PORTCOND_EQUALS, 0x00)

	PORT_START_TAG("GEAR")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Gear Shift") PORT_TOGGLE
INPUT_PORTS_END

static struct AY8910interface ay8910_interface_1 = {
	input_port_0_r,
	input_port_1_r
};

static struct AY8910interface ay8910_interface_2 = {
	input_port_2_r,
	input_port_3_r
};

static INTERRUPT_GEN( chl_interrupt )
{
	cpunum_set_input_line(1, 0, PULSE_LINE);
}

static MACHINE_DRIVER_START( changela )

	MDRV_CPU_ADD(Z80,5000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(chl_interrupt,1)

	MDRV_CPU_ADD(M68705,2500000/4)
	MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(5000000.0 / (318 * 262))
	MDRV_VBLANK_DURATION((int)(1000000.0 * 318 * (262 - 224) / 5000000))

	MDRV_MACHINE_RESET(changela)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 262)
	MDRV_VISIBLE_AREA(8, 255, 32, 255)
	MDRV_PALETTE_LENGTH(0x40)

	MDRV_VIDEO_START(changela)
	MDRV_VIDEO_UPDATE(changela)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1250000)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(AY8910, 1250000)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( changela )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cl25a",	0x0000, 0x2000, CRC(38530a60) SHA1(0b0ef1abe11c5271fcd1671322b77165217553c3) )
	ROM_LOAD( "cl24a",	0x2000, 0x2000, CRC(2fcf4a82) SHA1(c33355e2d4d3fab32c8d713a680ec0fceedab341) )
	ROM_LOAD( "cl23",	0x4000, 0x2000, CRC(08385891) SHA1(d8d66664ec25db067d5a4a6c35ec0ac65b9e0c6a) )
	ROM_LOAD( "cl22",	0x6000, 0x2000, CRC(796e0abd) SHA1(64dd9fc1f9bc44519a253ef0c02e181dd13904bf) )
	ROM_LOAD( "cl27",	0xb000, 0x1000, CRC(3668afb8) SHA1(bcfb788baf806edcb129ea9f9dcb1d4260684773) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "cl38a",	0x0000, 0x800, CRC(b70156ce) SHA1(c5eab8bbd65c4f587426298da4e22f991ce01dde) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "cl111",	0x0000, 0x2000, CRC(41c0149d) SHA1(3ea53a3821b044b3d0451fec1b4ee2c28da393ca) )
	ROM_LOAD( "cl113",	0x2000, 0x2000, CRC(ddf99926) SHA1(e816b88302c5639c7284f4845d450f232d63a10c) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )
	ROM_LOAD( "cl46",	0x0000, 0x1000, CRC(9c0a7d28) SHA1(fac9180ea0d9aeea56a84b35cc0958f0dd86a801) )

	ROM_REGION( 0x8000, REGION_USER1, 0 )
	ROM_LOAD( "cl100",	0x0000, 0x2000, CRC(3fa9e4fa) SHA1(9abd7df5fcf143a0c476bd8c8753c5ea294b9f74) )
	ROM_LOAD( "cl99",	0x2000, 0x2000, CRC(67b27b9e) SHA1(7df0f93851959359218c8d2272e30d242a77039d) )
	ROM_LOAD( "cl98",	0x4000, 0x2000, CRC(bffe4149) SHA1(5cf0b98f9d342bd06d575c565ea01bbd79f5e04b) )
	ROM_LOAD( "cl97",	0x6000, 0x2000, CRC(5abab8f9) SHA1(f5156855bbcdf0740fd44520386318ee53ebbf9a) )

	ROM_REGION( 0x1000, REGION_USER2, 0 )
	ROM_LOAD( "cl44",	0x0000, 0x1000, CRC(160d2bc7) SHA1(2609208c2bd4618ea340923ee01af69278980c36) )

	ROM_REGION( 0x3000, REGION_USER3, 0 )
	ROM_LOAD( "cl7",	0x0000, 0x0800, CRC(01e3efca) SHA1(b26203787f105ba32773e37035c39253050f9c82) )
	ROM_LOAD( "cl9",	0x1000, 0x2000, CRC(4e53cdd0) SHA1(6255411cfdccbe2c581c83f9127d582623453c3a) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "cl88",	0x0000, 0x0020, CRC(da4d6625) SHA1(2d9a268973518252eb36f479ab650af8c16c885c) )
ROM_END

static DRIVER_INIT(changela)
{
	state_save_register_global(portA_in);
	state_save_register_global(portA_out);
	state_save_register_global(ddrA);
	state_save_register_global(portB_out);
	state_save_register_global(ddrB);
	state_save_register_global(portC_in);
	state_save_register_global(portC_out);
	state_save_register_global(ddrC);

	state_save_register_global(mcu_out);
	state_save_register_global(mcu_in);
	state_save_register_global(mcu_PC1);
	state_save_register_global(mcu_PC0);

	state_save_register_global(changela_tree0_col);
	state_save_register_global(changela_tree1_col);
	state_save_register_global(changela_left_bank_col);
	state_save_register_global(changela_right_bank_col);
	state_save_register_global(changela_boat_shore_col);
	state_save_register_global(changela_collision_reset);
	state_save_register_global(changela_tree_collision_reset);
}

static UINT8 *chl_riverram, *chl_treeram;
static UINT8 chl_treeram2[0x40], chl_stateram[0xc0];
static UINT8 chl_mem_dev_selected, chl_tree_en, chl_horizon;
static UINT8 chl_v_count_river, chl_v_count_tree, chl_tree_prio, chl_tree_on[2];
static UINT32 chl_sloperom_bank;
static mame_bitmap *chl_obj0_bitmap, *chl_river_bitmap, *chl_tree_bitmap[2];
static mame_timer *changela_scanline_timer;

#define CHL_PIXEL(bitmap, y, x) (((UINT16 *)(bitmap)->line[y])[x])

static void draw_river(mame_bitmap *bitmap, int sy)
{
	UINT8 const *const ROM = memory_region(REGION_USER2);
	UINT8 *const RAM = chl_stateram;
	UINT8 const *const TILE_ROM = memory_region(REGION_GFX1);
	UINT8 const *const TILE_RAM = chl_riverram;
	UINT8 const *const PROM = memory_region(REGION_PROMS);

	int preload = ((sy < 32) ? 1 : 0);

	UINT8 math_train[10] = { 0 };
	UINT8 pre_train[3] = { 0 };

	UINT8 prev_state = 0;

	UINT8 ram_count = 0;
	UINT8 rom_count = 0;

	int hosc = 0;
	int carry = 0;

	if (sy == 32)
		chl_v_count_river = chl_horizon;
	chl_v_count_river = (chl_v_count_river + 1) & 0xff;

	for (int i = 0; i < 0x20; i++)
	{
		UINT8 curr_state = PROM[i];

		if (prev_state & 0x80)
			ram_count = (ram_count + 1) & 0x0f;
		if ((curr_state & 0x40) && !(prev_state & 0x40))
			rom_count = (rom_count + 1) & 0x0f;

		if (prev_state & 0x02)
			carry = (((pre_train[1] + pre_train[2] + carry) > 0x0f) ? 1 : 0);
		if (!(curr_state & 0x08))
			carry = 0;

		if (prev_state & 0x10)
			hosc = (math_train[8] << 4) | math_train[9];

		int rom_addr = chl_sloperom_bank | ((chl_v_count_river & 0x7e) << 2) | ((rom_count & 0x0e) >> 1);
		int ram_a5 = ((curr_state & 0x01) & ((curr_state & 0x40) >> 6) & preload) ^ 0x01;
		int ram_addr = (ram_a5 << 5) | (ram_count << 1) | ((curr_state & 0x20) >> 5);
		int mux45 = rom_count & 0x01;
		int mux61 = chl_v_count_river & 0x01;

		switch (curr_state)
		{
			case 0x01: case 0x09: case 0x19: case 0x0d: case 0x8d:
				pre_train[0] = (mux45 ? ((ROM[rom_addr] & 0xf0) >> 4) : (ROM[rom_addr] & 0x0f));
				break;
			case 0x0f: case 0x2f:
				math_train[0] = RAM[ram_addr] = (mux45 ? ((ROM[rom_addr] & 0xf0) >> 4) : (ROM[rom_addr] & 0x0f));
				break;
			case 0x4d: case 0x69: case 0x6d: case 0xc5: case 0xcd:
				pre_train[0] = RAM[ram_addr] & 0x0f;
				break;
			case 0xea: case 0xee:
				math_train[0] = RAM[ram_addr] = (mux61 ? (pre_train[1]) : ((pre_train[1] + pre_train[2] + carry) & 0x0f));
				break;
			default:
				break;
		}

		if (curr_state & 0x02)
		{
			for (int j = 9; j > 0; j--)
			{
				math_train[j] = math_train[j - 1];
			}
		}
		else
		{
			pre_train[2] = pre_train[1];
			pre_train[1] = pre_train[0];
		}

		prev_state = curr_state;
	}

	if (!(chl_v_count_river & 0x80))
	{
		int h_count = 0x80 | (hosc >> 1);
		int tile_v = ((math_train[3] & 0x0c) >> 2) | ((math_train[2] & 0x0f) << 2) | ((math_train[1] & 0x07) << 6);
		int tile_h = ((math_train[7] & 0x0f) | ((math_train[6] & 0x0f) << 4) | ((math_train[5] & 0x01) << 8)) + 4;

		for (int sx = 0; sx < 256; sx++)
		{
			for (int i = 0; i < ((sx < 16) ? 1 : 4); i++)
			{
				if (h_count > 0xff)
				{
					h_count = ((math_train[9] & 0x0f) >> 1) | ((math_train[8] & 0x0f) << 3) | 0x80;
					tile_h = (tile_h + 1) & 0xfff;

					if (((math_train[9] & 0x01) && (tile_h & 0x01)))
						h_count--;
				}
				else
					h_count++;
			}

			int ram_addr = ((tile_h & 0x1f8) >> 3) | ((tile_v & 0x1f0) << 2);
			int rom_addr = ((tile_h & 0x06) >> 1) | ((tile_v & 0x0f) << 2) | ((TILE_RAM[ram_addr] & 0x7f) << 6);

			int col;
			if (tile_h & 0x01)
				col = TILE_ROM[rom_addr] & 0x0f;
			else
				col = (TILE_ROM[rom_addr] & 0xf0) >> 4;

			CHL_PIXEL(bitmap, sy, sx) = col;
		}
	}

	if (chl_v_count_river == 0x7f)
		chl_tree_prio = BIT(math_train[2], 2);
}
static void draw_tree(mame_bitmap *bitmap, int sy, int tree_num)
{

	UINT8 const *const ROM = memory_region(REGION_USER2);
	UINT8 *const RAM = chl_stateram + 0x40 + 0x40 * tree_num;
	UINT8 const *const PROM = memory_region(REGION_PROMS);

	UINT8 *const RAM2 = chl_treeram2 + 0x20 * tree_num;
	UINT8 const *const TILE_ROM = (tree_num ? (memory_region(REGION_USER3) + 0x1000) : (memory_region(REGION_GFX1) + 0x2000));
	UINT8 const *const TILE_RAM = (tree_num ? memory_region(REGION_USER3) : chl_treeram);

	int preload = ((sy < 32) ? 1 : 0);

	UINT8 math_train[10] = { 0 };
	UINT8 pre_train[3] = { 0 };
	UINT8 tree_train[3] = { 0 };

	UINT8 prev_state = 0;

	UINT8 ram_count = 0;
	UINT8 rom_count = 0;

	int hosc = 0;
	int carry = 0;
	int tree_carry = 0;

	int h_count, tile_v, tile_h;
	int all_ff;

	if (sy == 32)
	{
		chl_tree_on[tree_num] = 0;
		if (tree_num == 0)
			chl_v_count_tree = chl_horizon;
	}
	if (tree_num == 0)
		chl_v_count_tree = (chl_v_count_tree + 1) & 0xff;

	for (int i = 0; i < 0x20; i++)
	{
		UINT8 curr_state = PROM[i];

		if (prev_state & 0x80)
			ram_count = (ram_count + 1) & 0x0f;
		if ((curr_state & 0x40) && !(prev_state & 0x40))
			rom_count = (rom_count + 1) & 0x0f;

		if (prev_state & 0x02)
		{
			carry = (((pre_train[1] + pre_train[2] + carry) > 0x0f) ? 1 : 0);
			tree_carry = (((tree_train[1] + tree_train[2] + tree_carry) > 0x0f) ? 1 : 0);
		}
		if (!(curr_state & 0x08))
			carry = tree_carry = 0;

		if (prev_state & 0x10)
			hosc = (math_train[8] << 4) | math_train[9];

		int rom_addr = chl_sloperom_bank | ((chl_v_count_tree & 0x7e) << 2) | ((rom_count & 0x0e) >> 1);
		int ram_a5 = ((curr_state & 0x01) & ((curr_state & 0x40) >> 6) & preload) ^ 0x01;
		int ram_addr = (ram_a5 << 5) | (ram_count << 1) | ((curr_state & 0x20) >> 5);
		int ram2_addr = (ram_count << 1) | ((curr_state & 0x20) >> 5);
		int mux45 = rom_count & 0x01;
		int mux61 = chl_v_count_tree & 0x01;

		switch (curr_state)
		{
			case 0x01: case 0x09: case 0x19: case 0x0d: case 0x8d:
				pre_train[0] = (mux45 ? ((ROM[rom_addr] & 0xf0) >> 4) : (ROM[rom_addr] & 0x0f));
				break;
			case 0x0f: case 0x2f:
				RAM[ram_addr] = (mux45 ? ((ROM[rom_addr] & 0xf0) >> 4) : (ROM[rom_addr] & 0x0f));
				break;
			case 0x4d: case 0x69: case 0x6d: case 0xc5: case 0xcd:
				pre_train[0] = RAM[ram_addr] & 0x0f;
				break;
			case 0xea: case 0xee:
				RAM[ram_addr] = (mux61 ? (pre_train[1]) : ((pre_train[1] + pre_train[2] + carry) & 0x0f));
				break;
			default:
				break;
		}

		if (!chl_tree_on[tree_num])
		{
			int mux82 = (chl_v_count_tree & 0x01) ^ 0x01;

			switch (curr_state)
			{
				case 0x01: case 0x09: case 0x19: case 0x0d: case 0x8d:
					tree_train[0] = RAM2[ram2_addr] = pre_train[0];
					break;
				case 0x0f: case 0x2f:
					math_train[0] = RAM2[ram2_addr] = RAM[ram_addr] & 0x0f;
					break;
				case 0x4d: case 0x69: case 0x6d: case 0xc5: case 0xcd:
					tree_train[0] = RAM2[ram2_addr] = pre_train[0];
					break;
				case 0xea: case 0xee:
					math_train[0] = RAM2[ram2_addr] = (mux82 ? ((tree_train[1] + tree_train[2] + tree_carry) & 0x0f) : (tree_train[1]));
					break;
				default:
					break;
			}
		}
		else
		{
			int mux82 = ((curr_state & 0x04) ? 0 : 1);

			switch (curr_state)
			{
				case 0x01: case 0x09: case 0x19: case 0x0d: case 0x8d:
					tree_train[0] = RAM2[ram2_addr];
					break;
				case 0x0f: case 0x2f:
					math_train[0] = RAM2[ram2_addr];
					break;
				case 0x4d: case 0x69: case 0x6d: case 0xc5: case 0xcd:
					tree_train[0] = RAM2[ram2_addr];
					break;
				case 0xea: case 0xee:
					math_train[0] = RAM2[ram2_addr] = (mux82 ? ((tree_train[1] + tree_train[2] + tree_carry) & 0x0f) : (tree_train[1]));
					break;
				default:
					break;
			}
		}

		if (curr_state & 0x02)
		{
			for (int j = 9; j > 0; j--)
				math_train[j] = math_train[j-1];
		}
		else
		{
			pre_train[2] = pre_train[1];
			pre_train[1] = pre_train[0];
			tree_train[2] = tree_train[1];
			tree_train[1] = tree_train[0];
		}

		prev_state = curr_state;
	}

	h_count = 0x80 | (hosc >> 1);
	tile_v = ((math_train[3] & 0x0c) >> 2) | ((math_train[2] & 0x0f) << 2) | ((math_train[1] & 0x07) << 6);
	tile_h = ((math_train[7] & 0x0f) | ((math_train[6] & 0x0f) << 4) | ((math_train[5] & 0x01) << 8)) + 4;
	all_ff = 1;

	for (int sx = 0; sx < 256; sx++)
	{
		for (int i = 0; i < ((sx < 16) ? 1 : 4); i++)
		{
			if (h_count > 0xff)
			{
				h_count = ((math_train[9] & 0x0f) >> 1) | ((math_train[8] & 0x0f) << 3) | 0x80;
				tile_h = (tile_h + 1) & 0xfff;

				if (((math_train[9] & 0x01) && (tile_h & 0x01)))
					h_count--;
			}
			else
				h_count++;
		}

		int ram_addr = ((tile_h & 0x1f8) >> 3) | ((tile_v & 0x1f0) << 2);
		int rom_addr = ((tile_h & 0x06) >> 1) | ((tile_v & 0x0f) << 2) | ((TILE_RAM[ram_addr] & 0x7f) << 6);

		if (!(chl_v_count_tree & 0x80) && (chl_tree_en & (0x01 << tree_num)) && ((TILE_ROM[rom_addr] & 0xf0) == 0))
			chl_tree_on[tree_num] = 1;

		if (chl_tree_on[tree_num])
		{
			int col;
			if (tile_h & 0x01)
				col = TILE_ROM[rom_addr] & 0x0f;
			else
				col = (TILE_ROM[rom_addr] & 0xf0) >> 4;

			if (col != 0x0f)
				all_ff = 0;

			if (col != 0x0f && col != 0x00)
				CHL_PIXEL(bitmap, sy, sx) = col | 0x30;
		}
	}

	if (all_ff)
		chl_tree_on[tree_num] = 0;
}
static void draw_obj0(mame_bitmap *bitmap, int sy)
{
	UINT8 const *const ROM = memory_region(REGION_USER1);
	UINT8 const *const RAM = spriteram;

	for (int sx = 0; sx < 256; sx++)
	{
		int vr = (RAM[sx * 4 + 0] & 0x80) >> 7;
		int hr = (RAM[sx * 4 + 0] & 0x40) >> 6;
		int hs = (RAM[sx * 4 + 0] & 0x20) >> 5;
		UINT32 vsize = RAM[sx * 4 + 0] & 0x1f;
		UINT8 ypos = ~RAM[sx * 4 + 1] + 1;
		UINT8 tile = RAM[sx * 4 + 2];
		UINT8 xpos = RAM[sx * 4 + 3];

		if (sy - ypos <= vsize)
		{
			for (int i = 0; i < 16; i++)
			{
				UINT8 sum = sy - ypos;

				UINT8 counter = i;
				if (hr) counter ^= 0x0f;

				UINT32 A8 = ((tile & 0x02) >> 1) ^ ((hr & hs) ^ hs);
				UINT32 A7 = ((((vr ^ ((sum & 0x10) >> 4)) & ((vsize & 0x10) >> 4)) ^ 0x01) & (tile & 0x01)) ^ 0x01;
				UINT32 rom_addr = (counter >> 1) | ((sum & 0x0f) << 3) | (A7 << 7) | (A8 << 8) | ((tile >> 2) << 9);
				if (vr) rom_addr ^= (0x0f << 3);

				UINT8 data;
				if (counter & 1)
					data = ROM[rom_addr] & 0x0f;
				else
					data = (ROM[rom_addr] & 0xf0) >> 4;

				if ((data != 0x0f) && (data != 0) && (xpos + i) < 256)
					CHL_PIXEL(bitmap, sy, xpos + i) = data | 0x10;

				if (hs)
				{
					if (counter & 1)
						data = ROM[rom_addr ^ 0x100] & 0x0f;
					else
						data = (ROM[rom_addr ^ 0x100] & 0xf0) >> 4;

					if ((data != 0x0f) && (data != 0) && (xpos + i + 16) < 256)
						CHL_PIXEL(bitmap, sy, xpos + i + 16) = data | 0x10;
				}
			}
		}
	}
}
static void draw_obj1(mame_bitmap *bitmap)
{
	UINT8 const *const ROM = memory_region(REGION_GFX2);
	UINT8 const *const RAM = videoram;

	UINT8 reg[4] = { 0 };

	UINT8 attrib = 0;

	for (int sy = 0; sy < 256; sy++)
	{
		for (int sx = 0; sx < 256; sx++)
		{

			int ram_addr = ((sx & 0xf8) >> 2) | ((sy & 0xf8) << 3);
			int tile_addr = RAM[ram_addr];

			if (!(RAM[ram_addr + 1] & 0x10) && (sx & 0x04))
				attrib = RAM[ram_addr + 1];

			UINT8 tile = ROM[(tile_addr << 4) | ((sx & 0x04) >> 2) | ((sy & 0x07) << 1)];
			reg[(sx & 0x0c) >> 2] = tile;
			int sum = (sx & 0x0f) + (attrib & 0x0f);

			int c0, c1;
			if ((sum & 0x03) == 0)
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x08) >> 3;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x80) >> 7;
			}
			else if ((sum & 0x03) == 1)
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x04) >> 2;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x40) >> 6;
			}
			else if ((sum & 0x03) == 2)
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x02) >> 1;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x20) >> 5;
			}
			else
			{
				c0 = (reg[(sum & 0x0c) >> 2] & 0x01) >> 0;
				c1 = (reg[(sum & 0x0c) >> 2] & 0x10) >> 4;
			}

			int col = c0 | (c1 << 1) | ((attrib & 0xc0) >> 4);
			if ((col & 0x07) != 0x07)
				CHL_PIXEL(bitmap, sy, sx) = col | 0x20;
		}
	}
}
WRITE8_HANDLER( changela_colors_w )
{

	static const UINT8 color_table[8] = { 0, 7, 18, 31, 58, 88, 146, 255 };

	int r, g, b;
	UINT32 c, color_index;

	c = data | ((offset & 0x01) << 8);
	c ^= 0x1ff;

	color_index = offset >> 1;
	color_index ^= 0x30;

	r = color_table[(c >> 0) & 0x07];
	g = color_table[(c >> 3) & 0x07];
	b = color_table[(c >> 6) & 0x07];

	palette_set_color(color_index,r,g,b);
}
WRITE8_HANDLER( changela_mem_device_select_w )
{

	chl_mem_dev_selected = data & 0x07;
	chl_tree_en = (data & 0x30) >> 4;
}
WRITE8_HANDLER( changela_mem_device_w )
{
	switch (chl_mem_dev_selected)
	{
		case 1:
			for (int i = 0; i < 3; i++)
				chl_stateram[(i * 0x40) + (offset & 0x3f)] = data & 0xf;
			break;

		case 2:
			chl_riverram[offset] = data;
			break;

		case 3:
			chl_treeram[offset] = data;
			break;

		default:
			break;
	}
}
READ8_HANDLER( changela_mem_device_r )
{
	switch (chl_mem_dev_selected)
	{
		case 1: return chl_stateram[offset & 0x3f];
		case 2: return chl_riverram[offset];
		case 3: return chl_treeram[offset];
		case 4: return memory_region(REGION_USER3)[offset];
		case 5: return memory_region(REGION_USER2)[offset];

		default:
			break;
	}

	return 0;
}
WRITE8_HANDLER( changela_slope_rom_addr_hi_w )
{
	chl_sloperom_bank = (data & 0x03) << 9;
}
WRITE8_HANDLER( changela_slope_rom_addr_lo_w )
{
	chl_horizon = data;
}
static void changela_scanline_callback(int param)
{
	int sy = param;

	if (sy == 256)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE,0xdf);

	else if (((sy % 64) == 0))
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE,0xcf);

	if (sy < 256)
	{

		const rectangle rect = { 0, 255, sy, sy };
		fillbitmap(chl_river_bitmap, 0, &rect);
		fillbitmap(chl_obj0_bitmap, 0, &rect);
		fillbitmap(chl_tree_bitmap[0], 0, &rect);
		fillbitmap(chl_tree_bitmap[1], 0, &rect);

		draw_river(chl_river_bitmap, sy);
		draw_obj0(chl_obj0_bitmap, sy);
		draw_tree(chl_tree_bitmap[0], sy, 0);
		draw_tree(chl_tree_bitmap[1], sy, 1);
	}

	if (sy >= 32 && sy < 256)
	{

		for (int sx = 1; sx < 256; sx++)
		{
			UINT16 pix = CHL_PIXEL(chl_river_bitmap, sy, sx);
			const int riv_col = (pix == 0x08 || pix == 0x09 || pix == 0x0a);

			pix = CHL_PIXEL(chl_river_bitmap, sy, sx-1);
			const int prev_col = (pix == 0x08 || pix == 0x09 || pix == 0x0a);

			if (CHL_PIXEL(chl_obj0_bitmap, sy, sx) == 0x14)
			{

				if (CHL_PIXEL(chl_tree_bitmap[0], sy, sx) != 0)
					changela_tree0_col = 1;

				if (CHL_PIXEL(chl_tree_bitmap[1], sy, sx) != 0)
					changela_tree1_col = 1;

				if (!riv_col && prev_col)
					changela_right_bank_col = 1;

				if (riv_col && !prev_col)
					changela_left_bank_col = 1;

				if (riv_col)
					changela_boat_shore_col = 1;
			}
		}
		if (!changela_tree_collision_reset)
		{
			changela_tree0_col = 0;
			changela_tree1_col = 0;
		}
		if (!changela_collision_reset)
		{
			changela_left_bank_col = 0;
			changela_right_bank_col = 0;
			changela_boat_shore_col = 0;
		}
	}

	sy = (sy + 1) % 262;
	/* the core numbers scanlines from the first visible hardware line, 32 */
	timer_adjust(changela_scanline_timer, cpu_getscanlinetime((sy + 230) % 262), sy, 0);
}

static void changela_video_reset(void)
{
	chl_sloperom_bank = chl_tree_en = chl_horizon = chl_mem_dev_selected = 0;
	chl_v_count_river = chl_v_count_tree = chl_tree_prio = 0;
	chl_tree_on[0] = chl_tree_on[1] = 0;
}

VIDEO_START( changela )
{
	chl_riverram = auto_malloc(0x800);
	chl_treeram = auto_malloc(0x800);
	memset(chl_riverram, 0, 0x800);
	memset(chl_treeram, 0, 0x800);
	chl_obj0_bitmap = auto_bitmap_alloc(256, 262);
	chl_river_bitmap = auto_bitmap_alloc(256, 262);
	chl_tree_bitmap[0] = auto_bitmap_alloc(256, 262);
	chl_tree_bitmap[1] = auto_bitmap_alloc(256, 262);
	changela_scanline_timer = timer_alloc(changela_scanline_callback);
	timer_adjust(changela_scanline_timer, cpu_getscanlinetime(230), 0, 0);
	state_save_register_global_pointer(chl_riverram, 0x800);
	state_save_register_global_pointer(chl_treeram, 0x800);
	state_save_register_global_array(chl_stateram);
	state_save_register_global_array(chl_treeram2);
	state_save_register_global_array(chl_tree_on);
	state_save_register_global(chl_sloperom_bank);
	state_save_register_global(chl_tree_en);
	state_save_register_global(chl_horizon);
	state_save_register_global(chl_mem_dev_selected);
	state_save_register_global(chl_v_count_river);
	state_save_register_global(chl_v_count_tree);
	state_save_register_global(chl_tree_prio);
	return 0;
}

VIDEO_UPDATE( changela )
{
	copybitmap(bitmap, chl_river_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	copybitmap(bitmap, chl_obj0_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
	/* the river state machine determines which tree generator is in front */
	copybitmap(bitmap, chl_tree_bitmap[chl_tree_prio], 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
	copybitmap(bitmap, chl_tree_bitmap[1 ^ chl_tree_prio], 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
	draw_obj1(bitmap);
}

#undef CHL_PIXEL

GAME( 1983, changela, 0, changela, changela, changela, ROT180, "Taito America Corporation", "Change Lanes", 0 )

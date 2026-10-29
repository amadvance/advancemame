// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    Raster Elite Tickee Tickats hardware

    driver by Aaron Giles

    Games supported:
        * Tickee Tickats
        * Ghost Hunter
        * Tuts Tomb
        * Mouse Attack
        * Rapid Fire
        * Mallet Madness

    Known bugs:
        * (Tickee) gun sometimes misfires

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "machine/ticket.h"
#include "vidhrdw/tlc34076.h"
#include "tickee.h"
#include "sound/ay8910.h"
#include "sound/okim6295.h"
#include "sound/2151intf.h"


UINT16 *tickee_control;



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( tickee )
{
	ticket_dispenser_init(100, 0, 1);

	tlc34076_reset(6);
}



/*************************************
 *
 *  Miscellaneous control bits
 *
 *************************************/

static READ8_HANDLER( port1_r )
{
	return input_port_1_r(offset) | (ticket_dispenser_0_r(0) >> 5) | (ticket_dispenser_1_r(0) >> 6);
}

static READ8_HANDLER( port2_r )
{
	return input_port_3_r(offset) | (ticket_dispenser_0_r(0) >> 5) | (ticket_dispenser_1_r(0) >> 6);
}



/*************************************
 *
 *  Miscellaneous control bits
 *
 *************************************/

static WRITE16_HANDLER( tickee_control_w )
{
	UINT16 olddata = tickee_control[offset];

	/* offsets:

        2 = palette flash (0 normally, 1 when trigger is pressed)
        3 = ticket motor (bit 3 = 0 for left motor, bit 2 = 0 for right motor)
        6 = lamps? (changing all the time)
    */

	COMBINE_DATA(&tickee_control[offset]);

	if (offset == 3)
	{
		ticket_dispenser_0_w(0, (data & 8) << 4);
		ticket_dispenser_1_w(0, (data & 4) << 5);
	}

	if (olddata != tickee_control[offset])
		logerror("%08X:tickee_control_w(%d) = %04X (was %04X)\n", activecpu_get_pc(), offset, tickee_control[offset], olddata);
}



/*************************************
 *
 *  Memory maps
 *
 *************************************/

static int rapidfir_ticket_motor;

static MACHINE_RESET( mouseatk )
{
	ticket_dispenser_init(100, 0, 1);
	tlc34076_reset(6);
	memset(tickee_control, 0, 8 * sizeof(*tickee_control));
}

static MACHINE_RESET( rapidfir )
{
	tlc34076_reset(6);
	rapidfir_palette_blank = 0;
	rapidfir_has_two_guns = !rapidfir_ticket_motor;
	rapidfir_gunx[0] = rapidfir_gunx[1] = 0;
	OKIM6295_set_bank_base(0, 0);
}

static MACHINE_RESET( maletmad )
{
	machine_reset_rapidfir();
	ticket_dispenser_init(100, 1, 1);
}

static WRITE16_HANDLER( rapidfir_transparent_w )
{
	if (!(data & 0xff00)) mem_mask |= 0xff00;
	if (!(data & 0x00ff)) mem_mask |= 0x00ff;
	COMBINE_DATA(&tickee_vram[offset]);
}

static READ16_HANDLER( rapidfir_transparent_r )
{
	return tickee_vram[offset];
}

static void rapidfir_to_shiftreg(offs_t address, UINT16 *shiftreg)
{
	if (address <= 0x7fe000)
		memcpy(shiftreg, &tickee_vram[address >> 4], 0x400);
}

static void rapidfir_from_shiftreg(offs_t address, UINT16 *shiftreg)
{
	if (address <= 0x7fe000)
		memcpy(&tickee_vram[address >> 4], shiftreg, 0x400);
}

static READ16_HANDLER( rapidfir_gun1_r )
{
	return rapidfir_gunx[0];
}
static READ16_HANDLER( rapidfir_gun2_r )
{
	return rapidfir_gunx[1];
}

static WRITE16_HANDLER( palette_blank_w )
{
	if (ACCESSING_LSB)
		rapidfir_palette_blank = data & 1;
}

static WRITE16_HANDLER( rapidfir_output_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 1);
		coin_counter_w(1, data & 2);
		if (rapidfir_ticket_motor)
			ticket_dispenser_0_w(0, (data & 0x10) << 3);
	}
}

static READ16_HANDLER( maletmad_in1_r )
{
	return input_port_1_word_r(0, mem_mask) |
		(rapidfir_ticket_motor ? ticket_dispenser_0_r(0) >> 1 : 0);
}

static WRITE16_HANDLER( sound_bank_w )
{
	int bank;
	if (!ACCESSING_LSB)
		return;
	switch (data & 0xff)
	{
		case 0x2c: bank = 0; break;
		case 0x2d: bank = 1; break;
		case 0x1c: bank = 2; break;
		case 0x1d: bank = 3; break;
		default: return;
	}
	OKIM6295_set_bank_base(0, bank * 0x40000);
}

static WRITE16_HANDLER( mouseatk_ym_w )
{
	if (offset & 1)
		AY8910_write_port_0_lsb_w(0, data, mem_mask);
	else
		AY8910_control_port_0_lsb_w(0, data, mem_mask);
}

static ADDRESS_MAP_START( mouseatk_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_BASE(&tickee_vram)
	AM_RANGE(0x02000000, 0x02ffffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x04000000, 0x04003fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x04100000, 0x041000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0x04200000, 0x0420001f) AM_READWRITE(AY8910_read_port_0_lsb_r, mouseatk_ym_w)
	AM_RANGE(0x04200100, 0x0420010f) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x04400000, 0x0440007f) AM_WRITE(tickee_control_w) AM_BASE(&tickee_control)
	AM_RANGE(0x04400040, 0x0440004f) AM_READ(input_port_3_word_r)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xc0000240, 0xc000025f) AM_WRITENOP
	AM_RANGE(0xff000000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rapidfir_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM AM_BASE(&tickee_vram)
	AM_RANGE(0x02000000, 0x027fffff) AM_READWRITE(rapidfir_transparent_r, rapidfir_transparent_w)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xfc000000, 0xfc00000f) AM_READ(rapidfir_gun1_r)
	AM_RANGE(0xfc000100, 0xfc00010f) AM_READ(rapidfir_gun2_r)
	AM_RANGE(0xfc000400, 0xfc00040f) AM_READ(MRA16_NOP)
	AM_RANGE(0xfc000500, 0xfc00050f) AM_READWRITE(MRA16_NOP, rapidfir_output_w)
	AM_RANGE(0xfc000600, 0xfc00060f) AM_WRITE(palette_blank_w)
	AM_RANGE(0xfc000700, 0xfc00070f) AM_WRITE(sound_bank_w)
	AM_RANGE(0xfc000800, 0xfc00080f) AM_READ(input_port_0_word_r)
	AM_RANGE(0xfc000900, 0xfc00090f) AM_READ(maletmad_in1_r)
	AM_RANGE(0xfc000a00, 0xfc000a0f) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfc000b00, 0xfc000b0f) AM_READ(input_port_3_word_r)
	AM_RANGE(0xfc000c00, 0xfc000c1f) AM_READ(input_port_4_word_r)
	AM_RANGE(0xfc000e00, 0xfc000e1f) AM_READ(watchdog_reset16_r)
	AM_RANGE(0xfc100000, 0xfc1000ff) AM_MIRROR(0x80000) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0xfc200000, 0xfc207fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xfc300000, 0xfc30000f) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0xfc400000, 0xfc40000f) AM_WRITE(YM2151_register_port_0_lsb_w)
	AM_RANGE(0xfc400010, 0xfc40001f) AM_READWRITE(YM2151_status_port_0_lsb_r, YM2151_data_port_0_lsb_w)
	AM_RANGE(0xfe000000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( tickee_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_BASE(&tickee_vram)
	AM_RANGE(0x02000000, 0x02ffffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x04000000, 0x04003fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x04100000, 0x041000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0x04200000, 0x0420000f) AM_READWRITE(AY8910_read_port_0_lsb_r, AY8910_control_port_0_lsb_w)
	AM_RANGE(0x04200010, 0x0420001f) AM_WRITE(AY8910_write_port_0_lsb_w)
	AM_RANGE(0x04200100, 0x0420010f) AM_READWRITE(AY8910_read_port_1_lsb_r, AY8910_control_port_1_lsb_w)
	AM_RANGE(0x04200110, 0x0420011f) AM_WRITE(AY8910_write_port_1_lsb_w)
	AM_RANGE(0x04400000, 0x0440007f) AM_WRITE(tickee_control_w) AM_BASE(&tickee_control)
	AM_RANGE(0x04400040, 0x0440004f) AM_READ(input_port_3_word_r)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xc0000240, 0xc000025f) AM_WRITENOP		/* seems to be a bug in their code */
	AM_RANGE(0xff000000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END


/* addreses in the 04x range shifted slightly...*/
static ADDRESS_MAP_START( ghoshunt_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_BASE(&tickee_vram)
	AM_RANGE(0x02000000, 0x02ffffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x04100000, 0x04103fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x04200000, 0x042000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0x04300000, 0x0430000f) AM_READWRITE(AY8910_read_port_0_lsb_r, AY8910_control_port_0_lsb_w)
	AM_RANGE(0x04300010, 0x0430001f) AM_WRITE(AY8910_write_port_0_lsb_w)
	AM_RANGE(0x04300100, 0x0430010f) AM_READWRITE(AY8910_read_port_1_lsb_r, AY8910_control_port_1_lsb_w)
	AM_RANGE(0x04300110, 0x0430011f) AM_WRITE(AY8910_write_port_1_lsb_w)
	AM_RANGE(0x04500000, 0x0450007f) AM_WRITE(tickee_control_w) AM_BASE(&tickee_control)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xc0000240, 0xc000025f) AM_WRITENOP		/* seems to be a bug in their code */
	AM_RANGE(0xff000000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( tickee )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, "Game Time/Diff" )
	PORT_DIPSETTING(    0x03, "Very Fast/Very Easy" )
	PORT_DIPSETTING(    0x02, "Fast/Easy" )
	PORT_DIPSETTING(    0x01, "Average/Hard" )
	PORT_DIPSETTING(    0x00, "Slow/Very Hard" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x00, "Last Box Tickets" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "25" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x30, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ))

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* right ticket status */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* left ticket status */
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( ghoshunt )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Messages in Play")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, "Fixed Ticketing")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x02, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, "Setting")
	PORT_DIPSETTING(    0x04, "Custom")
	PORT_DIPSETTING(    0x00, DEF_STR( Standard ))
	PORT_DIPNAME( 0x08, 0x08, "Messages")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x00, "Tickets")
	PORT_DIPSETTING(    0x30, "5")
	PORT_DIPSETTING(    0x20, "10")
	PORT_DIPSETTING(    0x10, "15")
	PORT_DIPSETTING(    0x00, "20")
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ))

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* right ticket status */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* left ticket status */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xd8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* right ticket status */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* left ticket status */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xd8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END



static INPUT_PORTS_START( mouseatk )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x03, "Number of Extra Cheese" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x01, "1 Extra Cheese" )
	PORT_DIPSETTING(    0x02, "2 Extra Cheese" )
	PORT_DIPSETTING(    0x03, "3 Extra Cheese" )
	PORT_DIPNAME( 0x0c, 0x0c, "Number of Mice & Cheese" )
	PORT_DIPSETTING(    0x00, "4 Mice - 5 Cheese" )
	PORT_DIPSETTING(    0x04, "5 Mice - 6 Cheese" )
	PORT_DIPSETTING(    0x08, "6 Mice - 7 Cheese" )
	PORT_DIPSETTING(    0x0c, "7 Mice - 8 Cheese" )
	PORT_DIPNAME( 0x10, 0x10, "Bonus Ticket Game" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Ticket Payout" )
	PORT_DIPSETTING(    0x20, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Double" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( rapidfir )


	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Reset NVRAM" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ))
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START_TAG("GUNX1")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START_TAG("GUNY1")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START_TAG("GUNX2")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START_TAG("GUNY2")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END

static INPUT_PORTS_START( maletmad )
	PORT_INCLUDE( rapidfir )

	PORT_MODIFY("IN0")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("IN1")
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("IN2")
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("DSW0")
	PORT_DIPNAME( 0x03, 0x03, "Payout" )
	PORT_DIPSETTING(    0x03, "1 Hit - 1 Ticket" )
	PORT_DIPSETTING(    0x02, "2 Hits - 1 Ticket" )
	PORT_DIPSETTING(    0x01, "3 Hits - 1 Ticket" )
	PORT_DIPSETTING(    0x00, "5 Hits - 1 Ticket" )
	PORT_DIPNAME( 0x0c, 0x0c, "Jackpot Amount" )
	PORT_DIPSETTING(    0x0c, "100 Tickets" )
	PORT_DIPSETTING(    0x08, "200 Tickets" )
	PORT_DIPSETTING(    0x04, "300 Tickets" )
	PORT_DIPSETTING(    0x00, "400 Tickets" )
	PORT_DIPNAME( 0x30, 0x30, "Game Time")
	PORT_DIPSETTING(    0x30, "10 seconds")
	PORT_DIPSETTING(    0x20, "15 seconds")
	PORT_DIPSETTING(    0x10, "20 seconds")
	PORT_DIPSETTING(    0x00, "30 seconds")
	PORT_DIPNAME( 0xc0, 0xc0, "Jackpot Trigger" )
	PORT_DIPSETTING(    0xc0, "4 Coins" )
	PORT_DIPSETTING(    0x80, "5 Coins" )
	PORT_DIPSETTING(    0x40, "6 Coins" )
	PORT_DIPSETTING(    0x00, "7 Coins" )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )


	PORT_MODIFY("GUNX2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_MODIFY("GUNY2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8910_interface_1 =
{
	input_port_0_r,
 	input_port_2_r
};

static struct AY8910interface ay8910_interface_2 =
{
	port1_r,
	port2_r
};



/*************************************
 *
 *  34010 configuration
 *
 *************************************/

static struct tms34010_config tms_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	NULL,							/* write to shiftreg function */
	NULL,							/* read from shiftreg function */
	NULL,							/* display address changed */
	NULL							/* display interrupt callback */
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( tickee )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(tickee_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (232 - 200)) / (60 * 232))

	MDRV_MACHINE_RESET(tickee)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_VISIBLE_AREA(0, 319, 0, 199)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(tickee)
	MDRV_VIDEO_UPDATE(tickee)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 40000000/16)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(AY8910, 40000000/16)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*

Ghost Hunter
Hanaho Games, 1996

  and

Tut's Tomb
Island Design Inc., 1996

PCB Layout
----------

|-----------------------------------------------|
|VOL                              CN106  CN103  |
|        NTE1423      ADV476KN50E    74LS175    |
|                                               |
|   YM2149            V52C4258Z80    74LS138    |
|                                               |
|   YM2149            V52C4258Z80    74LS373    |
|                                               |
|         DIPSW(8)    V52C4258Z80    74LS373    |
|J                                              |
|A         74LS161    V52C4258Z80    GHOSTHUN.7K|
|M  74LS74                                      |
|M         74LS273    |------|       GHOSTHUN.7J|
|A                    |TMS   |                  |
|          74LS74     |34010 |       GHOSTHUN.7H|
|                     |------|                  |
|          74LS14                    GHOSTHUN.7G|
|   ULN2803           |---|                     |
| TIP122   TIP122     |110|          DS1220Y    |
| TIP122   TIP122     |---|    40MHz 14.31818MHz|
| TIP122   TIP122    74LS273                    |
|                              74LS245  74LS374 |
|-------------------------------------|--CN2--|-|
(All IC's shown)                      |-------|

Notes:
      34010 clocks - INCLK: 40.000MHz, VCLK: 7.15909MHz, HSYNC: 16.1kHz, VSYNC: 69Hz, BLANK: 69Hz
      YM2149 clock - 1.7897725MHz [14.31818/8]
      VSync        - 69Hz
      HSync        - 15.78kHz
      110          - AMD MACH110 High Density Electrically-Erasable CMOS Programmable Logic (PLCC44)
      ADV476KN50E  - Analog Devices ADV476KN50E CMOS Monolithic 256x18 Color Palette RAM-DAC (DIP28)
      DS1220Y      - Dallas Semiconductor DS1220Y 16K Nonvolatile SRAM (DIP24)
      V52C4258Z80  - Vitelic V52C4258Z80 ?? possibly 256K x8 SRAM (ZIP28)
      NTE1423      - NTE1423 5.7W Power Amplifier (SIP8)
      CN2          - DB25 connector
      CN103/106    - 4-pin connector for gun hookup
      ULN2803      - Motorola ULN2803 Octal High Voltage, High Current Darlington Transistor Arrays (DIP18)
      TIP122       - Motorola TIP122 General-Purpose NPN Darlington Transistor (TO-220)
*/

MACHINE_DRIVER_START( ghoshunt )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(ghoshunt_map,0)

	MDRV_FRAMES_PER_SECOND(69)
	MDRV_VBLANK_DURATION((1000000 * (232 - 200)) / (60 * 232))

	MDRV_MACHINE_RESET(tickee)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_VISIBLE_AREA(0, 319, 0, 199)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(tickee)
	MDRV_VIDEO_UPDATE(tickee)

	/* sound hardware - 2149! */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 14318180/8)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(AY8910, 14318180/8)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



static struct tms34010_config rapidfir_tms_config =
{
	0, NULL, rapidfir_to_shiftreg, rapidfir_from_shiftreg, NULL, NULL
};

static MACHINE_DRIVER_START( mouseatk )
	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(mouseatk_map,0)
	MDRV_FRAMES_PER_SECOND(14318181.0/2/444/233)
	MDRV_VBLANK_DURATION((1000000.0 * 33 * 444) / (14318181/2))
	MDRV_MACHINE_RESET(mouseatk)
	MDRV_NVRAM_HANDLER(generic_1fill)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_VISIBLE_AREA(0, 319, 0, 199)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(mouseatk)
	MDRV_VIDEO_UPDATE(mouseatk)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 1000000)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( rapidfir )
	MDRV_CPU_ADD(TMS34010, 50000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(rapidfir_tms_config)
	MDRV_CPU_PROGRAM_MAP(rapidfir_map,0)
	MDRV_FRAMES_PER_SECOND(9000000.0/556/293)
	MDRV_VBLANK_DURATION((1000000.0 * 53 * 556) / 9000000)
	MDRV_MACHINE_RESET(rapidfir)
	MDRV_NVRAM_HANDLER(generic_1fill)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 240)
	MDRV_VISIBLE_AREA(0, 383, 0, 239)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(rapidfir)
	MDRV_VIDEO_UPDATE(rapidfir)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( maletmad )
	MDRV_IMPORT_FROM(rapidfir)
	MDRV_MACHINE_RESET(maletmad)
MACHINE_DRIVER_END

static DRIVER_INIT( rapidfir )
{
	rapidfir_ticket_motor = 0;
	rapidfir_has_two_guns = 1;
}

static DRIVER_INIT( maletmad )
{
	rapidfir_ticket_motor = 1;
	rapidfir_has_two_guns = 0;
}


/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( tickee )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "3.ic4",  0x000000, 0x80000, CRC(5b1e399c) SHA1(681608f06bbaf3d258e9f4768a8a6c5047ad08ec) )
	ROM_LOAD16_BYTE( "2.ic3",  0x000001, 0x80000, CRC(1b26d4bb) SHA1(40266ec0fe5897eba85072e5bb39973d34f97546) )
	ROM_LOAD16_BYTE( "1.ic2",  0x100000, 0x80000, CRC(f7f0309e) SHA1(4a93e0e203f5a340a56b770a40b9ab00e131644d) )
	ROM_LOAD16_BYTE( "4.ic5",  0x100001, 0x80000, CRC(ceb0f559) SHA1(61923fe09e1dfde1eaae297ccbc672bc74a70397) )
ROM_END


ROM_START( ghoshunt )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "ghosthun.7g",  0x000001, 0x80000, CRC(d59716c2) SHA1(717a1a1c5c559569f9e7bc4ae4356d112f0cf4eb) )
	ROM_LOAD16_BYTE( "ghosthun.7h",  0x000000, 0x80000, CRC(ef38bfc8) SHA1(12b8f29f4da120f14126cbcdf4019bedd97063c3) )
	ROM_LOAD16_BYTE( "ghosthun.7j",  0x100001, 0x80000, CRC(763d7c79) SHA1(f0dec99feeeefeddda6a88276dc306a30a58f4e4) )
	ROM_LOAD16_BYTE( "ghosthun.7k",  0x100000, 0x80000, CRC(71e6099e) SHA1(2af6f1aa304eed849c90d95d17643cb12b05baab) )
ROM_END


ROM_START( tutstomb )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "tutstomb.7g",  0x000001, 0x80000, CRC(b74d3cf2) SHA1(2221b565362183a97a959389e8a0a026ca89e0ce) )
	ROM_LOAD16_BYTE( "tutstomb.7h",  0x000000, 0x80000, CRC(177f3afb) SHA1(845f982a66a8b69b0ea0045399102e8bb33f7fbf) )
	ROM_LOAD16_BYTE( "tutstomb.7j",  0x100001, 0x80000, CRC(69094f31) SHA1(eadae8847d0ff1568e63f71bf09a84dc443fdc1c))
	ROM_LOAD16_BYTE( "tutstomb.7k",  0x100000, 0x80000, CRC(bc362df8) SHA1(7b15c646e99c916d850629e4e758b1dbb329639a) )
ROM_END



ROM_START( mouseatk )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "ms-attk2-rev.a1.u8",  0x000000, 0x80000, CRC(a150525c) SHA1(a6be8524ae435502eeeeeaafa856dc812121d4e3) )
	ROM_LOAD16_BYTE( "ms-attk3-rev.a1.u9",  0x000001, 0x80000, CRC(f060091d) SHA1(b3a2099d5ca5a658a7a87bb11a20c27a6a2f11f2) )
	ROM_LOAD16_BYTE( "ms-attk4-rev.a1.u10", 0x100000, 0x80000, CRC(19806349) SHA1(3431dc70897f50e1be7578dd4ef99fa9be4450cf) )
	ROM_LOAD16_BYTE( "ms-attk1-rev.a1.u7",  0x100001, 0x80000, CRC(b936194b) SHA1(33a2038a56fb4a4301ee04ca7a32a70ab5870fad) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "ms-snd-rev.a.u202", 0x000000, 0x80000, CRC(ce4af0f2) SHA1(f054ac27faf52ae5ec6910ecfca164a21eefe4ca) )
ROM_END

ROM_START( rapidfir )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "rf11.u8",  0x000000, 0x80000, CRC(f7d8df33) SHA1(0abd54bbccfa90d830cbbdbcf2058197af980981) )
	ROM_LOAD16_BYTE( "rf11.u9",  0x000001, 0x80000, CRC(a72af935) SHA1(ed0deb6f51681f70e07ad7c05a92f6a0f2063f7a) )
	ROM_LOAD16_BYTE( "rf10.u6",  0x100000, 0x80000, CRC(e8d2e5d2) SHA1(db93014598f7b76785e0fd5c0ac8808a3be06435) )
	ROM_LOAD16_BYTE( "rf10.u7",  0x100001, 0x80000, CRC(0e33f2ed) SHA1(9b2533e001b94ccc97b95e31762186f59d5a3b9c) )
	ROM_LOAD16_BYTE( "rf10.u4",  0x200000, 0x80000, CRC(8a088468) SHA1(f94dc78158e5656657d3b26c5b0ca88f39eb5ff4) )
	ROM_LOAD16_BYTE( "rf10.u5",  0x200001, 0x80000, CRC(593b3df2) SHA1(301fa06031eff54fb2d9e08f80fc3c26e5c51da9) )
	ROM_LOAD16_BYTE( "rf11.u2",  0x300000, 0x80000, CRC(ffa0c695) SHA1(bccdefe7cee15999c416fdcb16a65b1bc6e12d13) )
	ROM_LOAD16_BYTE( "rf11.u3",  0x300001, 0x80000, CRC(ac63b863) SHA1(c9160aec6179d1f550279b80fd4c2a14ce94fdab) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "rf11.u507", 0x000000, 0x80000, CRC(899d1e15) SHA1(ca22b4ad714a5212bc9347eb3a5b660c02bad7e5) )
	ROM_LOAD( "rf11.u510", 0x080000, 0x80000, CRC(6209c8fe) SHA1(bfbd63445b4ac2d4253c4b5354e1058070290084) )
ROM_END

ROM_START( maletmad )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )

	ROM_LOAD16_BYTE( "malletmadness_v2.1.u6",  0x100000, 0x80000, CRC(83309174) SHA1(d387c8bc4d3c640f16525241892cc8d5d5da7f60) )
	ROM_LOAD16_BYTE( "malletmadness_v2.1.u7",  0x100001, 0x80000, CRC(4642587e) SHA1(076eda538d570074028e9b4394f1a8a459678137) )
	ROM_LOAD16_BYTE( "malletmadness_v2.1.u4",  0x200000, 0x80000, CRC(70ca968c) SHA1(74c66a67568b428ae5e20377038c7ea0cd33b25e) )
	ROM_LOAD16_BYTE( "malletmadness_v2.1.u5",  0x200001, 0x80000, CRC(c771418a) SHA1(cab360103a4d4195f5a9920746ae5df2866e24dc) )
	ROM_LOAD16_BYTE( "malletmadness_v2.1.u2",  0x300000, 0x80000, CRC(08825aee) SHA1(d7c2098ce5d73e0ca4b02a654d5c958d999337e3) )
	ROM_LOAD16_BYTE( "malletmadness_v2.1.u3",  0x300001, 0x80000, CRC(49a6bd62) SHA1(40d67fd7a3dded708366246d897682ffee88a2e1) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "malletmadness_v2.1.u507", 0x000000, 0x80000, CRC(6a2c9021) SHA1(bff61ef696a2104a32aab6fdc51f504385d0c769) )

ROM_END

/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1994, tickee,   0, tickee,   tickee,   0, ROT0, "Raster Elite",  "Tickee Tickats", 0 )
GAME( 1996, ghoshunt, 0, ghoshunt, ghoshunt, 0, ROT0, "Hanaho Games",  "Ghost Hunter", 0 )
GAME( 1996, tutstomb, 0, ghoshunt, ghoshunt, 0, ROT0, "Island Design", "Tut's Tomb", 0 )

GAME( 1996, mouseatk, 0, mouseatk, mouseatk, 0, ROT0, "ICE", "Mouse Attack", 0 )
GAME( 1998, rapidfir, 0, rapidfir, rapidfir, rapidfir, ROT0, "Hanaho Games", "Rapid Fire (v1.1, Build 239)", 0 )
GAME( 1999, maletmad, 0, maletmad, maletmad, maletmad, ROT0, "Hanaho Games (Capcom license)", "Mallet Madness (v2.1)", 0 )

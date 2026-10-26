// license:BSD-3-Clause
// copyright-holders:Zsolt Vasvari
/***************************************************************************

    Allied Leisure Clay Shoot hardware

    driver by Zsolt Vasvari

****************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "sound/dac.h"
#include "includes/clayshoo.h"


static UINT8 input_port_select;
static UINT8 analog_port_val;
static mame_timer *analog_timer[2];

static void reset_analog_bit(int bit);


/*************************************
 *
 *  Digital control handling functions
 *
 *************************************/

static WRITE8_HANDLER( input_port_select_w )
{
	input_port_select = data;
}


static READ8_HANDLER( input_port_r )
{
	UINT8 data = 0xff;
	int i;
	for (i = 0; i < 4; i++)
		if (input_port_select & (1 << i))
			data &= readinputport(i);
	return data;
}

static ppi8255_interface ppi8255_intf =
{
	2, 							/* 2 chips */
	{ 0, 0 },					/* Port A read */
	{ 0, input_port_r },		/* Port B read */
	{ 0, 0 },					/* Port C read */
	{ 0, input_port_select_w },	/* Port A write */
	{ 0, 0 },					/* Port B write */
	{ 0, 0 /* sound effects */},/* Port C write */
};


static void clayshoo_sound_output(int state)
{
	DAC_data_w(0, state ? 0 : 255);
}

static const struct pit8253_config clayshoo_pit =
{
	TYPE8253,
	{
		{ 5068800 / 2, clayshoo_sound_output, NULL },
		{ 5068800 / 2, NULL, NULL },
		{ 5068800 / 2, NULL, NULL }
	}
};

MACHINE_START( clayshoo )
{
	analog_timer[0] = timer_alloc(reset_analog_bit);
	analog_timer[1] = timer_alloc(reset_analog_bit);
	return pit8253_init(1, &clayshoo_pit);
}

MACHINE_RESET( clayshoo )
{
	ppi8255_init(&ppi8255_intf);
	pit8253_reset(0);
	input_port_select = 0;
	analog_port_val = 0xff;
	timer_adjust(analog_timer[0], TIME_NEVER, 0, 0);
	timer_adjust(analog_timer[1], TIME_NEVER, 0, 0);
}


/*************************************
 *
 *  Analog control handling functions
 *
 *************************************/

static void reset_analog_bit(int bit)
{
	analog_port_val &= ~bit;
}


static double compute_duration(int analog_pos)
{
	/* the 58 comes from the length of the loop used to
       read the analog position */
	return TIME_IN_CYCLES(58*analog_pos, 0);
}


WRITE8_HANDLER( clayshoo_analog_reset_w )
{
	/* reset the analog value, and start two times that will fire
       off in a short period proportional to the position of the
       analog control and set the appropriate bit. */

	analog_port_val = 0xff;

	timer_adjust(analog_timer[0], compute_duration(readinputport(4)), 0x02, 0);
	timer_adjust(analog_timer[1], compute_duration(readinputport(5)), 0x01, 0);
}


READ8_HANDLER( clayshoo_analog_r )
{
	return analog_port_val;
}

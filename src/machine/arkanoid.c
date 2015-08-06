/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



UINT8 arkanoid_paddle_select;

static UINT8 z80write,fromz80,m68705write,toz80;

static UINT8 portA_in,portA_out,ddrA;
static UINT8 portC_out,ddrC;

MACHINE_START( arkanoid )
{
	state_save_register_global(arkanoid_paddle_select);
	state_save_register_global(z80write);
	state_save_register_global(fromz80);
	state_save_register_global(m68705write);
	state_save_register_global(toz80);

	state_save_register_global(portA_in);
	state_save_register_global(portA_out);
	state_save_register_global(ddrA);

	state_save_register_global(portC_out);
	state_save_register_global(ddrC);

	return 0;
}

MACHINE_RESET( arkanoid )
{
	portA_in = portA_out = z80write = m68705write = 0;
}

READ8_HANDLER( arkanoid_Z80_mcu_r )
{
	/* return the last value the 68705 wrote, and mark that we've read it */
	m68705write = 0;
	return toz80;
}

static void test(int param)
{
	z80write = 1;
	fromz80 = param;
}

WRITE8_HANDLER( arkanoid_Z80_mcu_w )
{
	timer_set(TIME_NOW, data, test);
	/* boost the interleave for a few usecs to make sure it is read successfully */
	cpu_boost_interleave(0, TIME_IN_USEC(10));
}

READ8_HANDLER( arkanoid_68705_portA_r )
{
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE8_HANDLER( arkanoid_68705_portA_w )
{
	portA_out = data;
}

WRITE8_HANDLER( arkanoid_68705_ddrA_w )
{
	ddrA = data;
}


READ8_HANDLER( arkanoid_68705_portC_r )
{
	int res=0;

	/* bit 0 is high on a write strobe; clear it once we've detected it */
	if (z80write) res |= 0x01;

	/* bit 1 is high if the previous write has been read */
	if (!m68705write) res |= 0x02;

	return (portC_out & ddrC) | (res & ~ddrC);
}

WRITE8_HANDLER( arkanoid_68705_portC_w )
{
	if ((ddrC & 0x04) && (~data & 0x04) && (portC_out & 0x04))
	{
		/* return the last value the Z80 wrote */
		z80write = 0;
		portA_in = fromz80;
	}
	if ((ddrC & 0x08) && (~data & 0x08) && (portC_out & 0x08))
	{
		/* a write from the 68705 to the Z80; remember its value */
		m68705write = 1;
		toz80 = portA_out;
	}

	portC_out = data;
}

WRITE8_HANDLER( arkanoid_68705_ddrC_w )
{
	ddrC = data;
}



READ8_HANDLER( arkanoid_68705_input_0_r )
{
	int res = input_port_0_r(offset) & 0x3f;

	/* bit 0x40 comes from the sticky bit */
	if (!z80write) res |= 0x40;

	/* bit 0x80 comes from a write latch */
	if (!m68705write) res |= 0x80;

	return res;
}

READ8_HANDLER( arkanoid_input_2_r )
{
	if (arkanoid_paddle_select)
	{
		return input_port_3_r(offset);
	}
	else
	{
		return input_port_2_r(offset);
	}
}

/*

    Paddle 2 MCU simulation

    TODO:
    \-Fix crashes and level finishing.
    \-Finish the level pointer table & check the real thing for true level pattern...
    \-(track_kludge_r)Find a better way to handle the paddle inputs.
    \-Code optimizations + add this into machine/arkanoid.c

    Notes:
    \-This game is an Arkanoid 1 bootleg but with level edited to match the Arkanoid 2 ones.
    \-Returning the right values for commands 0x38,0xff and 0x8a gives the level that has to
    be played,but I don't have any clue about the true level pattern used.Checking Arkanoid 2
    doesn't help much BTW...

*/

static int paddle2_prot;

READ8_HANDLER( paddle2_prot_r )
{
	static UINT8 level_table_a[] =
	{
		0xf3,0xf7,0xf9,0xfb,0xfd,0xff,0xf5,0xe3, /* 1- 8*/
		0xe5,0xe7,0xe9,0xeb,0xed,0xef,0xf1,0xf7, /* 9-16*/
		0xf9,0xfb,0xfd,0xff,0x00,0x00,0x00,0x00, /*17-24*/
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  /*25-32*/
	};
	static UINT8 level_table_b[] =
	{
		0x52,0x52,0x52,0x52,0x52,0x52,0x0e,0x0e, /* 1- 8*/
		0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e, /* 9-16*/
		0x0e,0x0e,0x0e,0x0e,0x00,0x00,0x00,0x00, /*17-24*/
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  /*25-32*/
	};
	UINT8 *RAM = memory_region(REGION_CPU1);
//  ui_popup("%04x: %02x",activecpu_get_pc(),paddle2_prot);

	switch (paddle2_prot)
	{
		case 0xc3: return 0x1d;
		case 0x24: return 0x9b;
		/* Level pointer table */
		case 0x38:
		if(RAM[0xed83] == 0)    return level_table_a[RAM[0xed72]];
		else					return RAM[0xed83];
		case 0xff:
		if(RAM[0xed83] == 0)	return level_table_b[RAM[0xed72]];
		else 					return RAM[0xed83];
		/* Guess this is used for building level       */
		/* pointers too,but I haven't tested yet...    */
		case 0x8a: return 0x0a;
		/* Goes into sub-routine $2050,controls level finishing(WRONG!!!) */
		case 0xe3:
		if(RAM[0xed83] != 0)	return 0xff;
		else					return 0;
		/* Gives BAD HW message otherwise */
		case 0x36: return 0x2d;
		case 0xf7: return 0;
		default: return paddle2_prot;
	}
}

WRITE8_HANDLER( paddle2_prot_w )
{
	logerror("%04x: prot_w %02x\n",activecpu_get_pc(),data);
	paddle2_prot = data;
}

READ8_HANDLER( paddle2_track_kludge_r )
{
	int track = readinputport(2);

	/* temp kludge,needed to get the right side of the screen */
	if(track < 0x44)
		return 0x23;
	return 0x03;
}

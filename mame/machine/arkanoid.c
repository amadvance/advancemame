// license:BSD-3-Clause
// copyright-holders:Brad Oliver,Stephane Humbert
/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6805/m6805.h"



UINT8 arkanoid_paddle_select;

#define MCU_CPU 1

/* semaphore flags and data latches between the Z80 and the 68705 */
static UINT8 host_flag, mcu_flag;
static UINT8 host_latch, mcu_latch;
static UINT8 latch_driven, reset_input;

/* 68705 ports. latch is the value written by the MCU, out the value seen by the glue logic */
static UINT8 portA_latch, portA_in, portA_out, ddrA;
static UINT8 portC_latch, portC_out, ddrC;

MACHINE_START( arkanoid )
{
	state_save_register_global(arkanoid_paddle_select);
	state_save_register_global(host_flag);
	state_save_register_global(mcu_flag);
	state_save_register_global(host_latch);
	state_save_register_global(mcu_latch);
	state_save_register_global(latch_driven);
	state_save_register_global(reset_input);

	state_save_register_global(portA_latch);
	state_save_register_global(portA_in);
	state_save_register_global(portA_out);
	state_save_register_global(ddrA);

	state_save_register_global(portC_latch);
	state_save_register_global(portC_out);
	state_save_register_global(ddrC);

	return 0;
}

MACHINE_RESET( arkanoid )
{
	host_flag = mcu_flag = 0;
	host_latch = mcu_latch = 0xff;
	latch_driven = reset_input = 0;
	portA_latch = portA_in = portA_out = 0xff;
	portC_latch = portC_out = 0xff;
	ddrA = ddrC = 0;
}

static UINT8 pa_value(void)
{
	return portA_out & (latch_driven ? host_latch : 0xff);
}

static void update_irq(void)
{
	cpunum_set_input_line(MCU_CPU, M68705_IRQ_LINE, host_flag ? ASSERT_LINE : CLEAR_LINE);
}

/* bit 7 of the Z80 register at d008 resets the MCU and the semaphore flip-flops */
void arkanoid_mcu_reset_w(int run)
{
	reset_input = !run;
	if (run)
	{
		host_flag = mcu_flag = 0;
		update_irq();
	}
	else
	{
		/* the ports are inputs after a reset */
		ddrA = ddrC = 0;
	}
	cpunum_set_input_line(MCU_CPU, INPUT_LINE_RESET, run ? CLEAR_LINE : ASSERT_LINE);
}

READ8_HANDLER( arkanoid_Z80_mcu_r )
{
	/* return the last value the 68705 wrote, and clear its semaphore */
	mcu_flag = 0;
	return mcu_latch;
}

WRITE8_HANDLER( arkanoid_Z80_mcu_w )
{
	if (!reset_input)
		host_flag = 1;
	host_latch = data;
	if (latch_driven)
		portA_in = data;
	update_irq();
	/* boost the interleave for a few usecs to make sure it is read successfully */
	cpu_boost_interleave(0, TIME_IN_USEC(10));
}

READ8_HANDLER( arkanoid_68705_portA_r )
{
	return (portA_latch & ddrA) | (portA_in & ~ddrA);
}

WRITE8_HANDLER( arkanoid_68705_portA_w )
{
	UINT8 diff = portA_latch ^ data;
	portA_latch = data;
	if (diff & ddrA)
		portA_out = data;
}

WRITE8_HANDLER( arkanoid_68705_ddrA_w )
{
	if (data != ddrA)
	{
		ddrA = data;
		portA_out = portA_latch;
	}
}

READ8_HANDLER( arkanoid_68705_portC_r )
{
	/* bit 0 is the host semaphore flag (active high), bit 1 is the MCU semaphore flag (active low) */
	UINT8 res = (host_flag ? 0x01 : 0x00) | (mcu_flag ? 0x00 : 0x02) | 0xfc;

	return (portC_latch & ddrC) | (res & ~ddrC);
}

static void portC_update(void)
{
	UINT8 data = portC_latch;
	UINT8 old_pa = pa_value();

	/* PC2 rising edge clears the host semaphore flag, when low the latch drives port A */
	if (data & 0x04)
	{
		latch_driven = 0;
		portA_in = 0xff;
		if (!(portC_out & 0x04))
		{
			host_flag = 0;
			update_irq();
		}
	}
	else
	{
		latch_driven = 1;
		portA_in = host_latch;
	}

	/* PC3 low sets the MCU semaphore flag, the data is latched on the falling edge */
	if (!(data & 0x08))
	{
		if (!reset_input)
			mcu_flag = 1;
		if (portC_out & 0x08)
			mcu_latch = old_pa;
	}

	portC_out = data;
}

WRITE8_HANDLER( arkanoid_68705_portC_w )
{
	UINT8 diff = portC_latch ^ data;
	portC_latch = data;
	if (diff & ddrC)
		portC_update();
}

WRITE8_HANDLER( arkanoid_68705_ddrC_w )
{
	if (data != ddrC)
	{
		ddrC = data;
		portC_update();
	}
}

READ8_HANDLER( arkanoid_68705_input_0_r )
{
	int res = input_port_0_r(offset) & 0x3f;

	/* bit 0x40 is the host semaphore flag, bit 0x80 the MCU semaphore flag (both active low) */
	if (!host_flag) res |= 0x40;
	if (!mcu_flag) res |= 0x80;

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

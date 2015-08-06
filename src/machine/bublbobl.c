/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"



UINT8 *bublbobl_mcu_sharedram;
extern int bublbobl_video_enable;
static int mcu_running;

WRITE8_HANDLER( bublbobl_bankswitch_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	/* bits 0-2 select ROM bank */
	memory_set_bankptr(1,&ROM[0x10000 + 0x4000 * ((data ^ 4) & 7)]);

	/* bit 3 n.c. */

	/* bit 4 resets second Z80 */
	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);

	/* bit 5 resets mcu */
	mcu_running = data & 0x20;

	/* bit 6 enables display */
	bublbobl_video_enable = data & 0x40;

	/* bit 7 flips screen */
	flip_screen_set(data & 0x80);
}

WRITE8_HANDLER( tokio_bankswitch_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	/* bits 0-2 select ROM bank */
	memory_set_bankptr(1,&ROM[0x10000 + 0x4000 * (data & 7)]);

	/* bits 3-7 unknown */
}

WRITE8_HANDLER( tokio_videoctrl_w )
{
	/* bit 7 flips screen */
	flip_screen_set(data & 0x80);

	/* other bits unknown */
}

WRITE8_HANDLER( bublbobl_nmitrigger_w )
{
	cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
}


static UINT8 tokio_prot_data[] =
{
	0x6c,
	0x7f,0x5f,0x7f,0x6f,0x5f,0x77,0x5f,0x7f,0x5f,0x7f,0x5f,0x7f,0x5b,0x7f,0x5f,0x7f,
	0x5f,0x77,0x59,0x7f,0x5e,0x7e,0x5f,0x6d,0x57,0x7f,0x5d,0x7d,0x5f,0x7e,0x5f,0x7f,
	0x5d,0x7d,0x5f,0x7e,0x5e,0x79,0x5f,0x7f,0x5f,0x7f,0x5d,0x7f,0x5f,0x7b,0x5d,0x7e,
	0x5f,0x7f,0x5d,0x7d,0x5f,0x7e,0x5e,0x7e,0x5f,0x7d,0x5f,0x7f,0x5f,0x7e,0x7f,0x5f,
	0x01,0x00,0x02,0x01,0x01,0x01,0x03,0x00,0x05,0x02,0x04,0x01,0x03,0x00,0x05,0x01,
	0x02,0x03,0x00,0x04,0x04,0x01,0x02,0x00,0x05,0x03,0x02,0x01,0x04,0x05,0x00,0x03,
	0x00,0x05,0x02,0x01,0x03,0x04,0x05,0x00,0x01,0x04,0x04,0x02,0x01,0x04,0x01,0x00,
	0x03,0x01,0x02,0x05,0x00,0x03,0x00,0x01,0x02,0x00,0x03,0x04,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x00,0x00,0x00,0x00,0x01,0x02,0x00,0x00,0x00,
	0x01,0x02,0x01,0x00,0x00,0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x01,0x01,
	0x00,0x00,0x00,0x00,0x02,0x00,0x01,0x02,0x00,0x01,0x01,0x00,0x00,0x02,0x01,0x00,
	0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x01
};
static int tokio_prot_count;

READ8_HANDLER( tokio_mcu_r )
{
	tokio_prot_count %= sizeof(tokio_prot_data);
	return tokio_prot_data[tokio_prot_count++];
}

READ8_HANDLER( tokiob_mcu_r )
{
	return 0xbf; /* ad-hoc value set to pass initial testing */
}



static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpunum_set_input_line(2,INPUT_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

WRITE8_HANDLER( bublbobl_sound_command_w )
{
	soundlatch_w(offset,data);
	timer_set(TIME_NOW,data,nmi_callback);
}

WRITE8_HANDLER( bublbobl_sh_nmi_disable_w )
{
	sound_nmi_enable = 0;
}

WRITE8_HANDLER( bublbobl_sh_nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpunum_set_input_line(2,INPUT_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}




/***************************************************************************

Bubble Bobble protection simulation.

The game has a 68701 MCU whose code we don't have.

***************************************************************************/

/*
Z80 memory usage mask:
fc00: R- -W -W -W -W -W -W -W -W -W -W -W -W -W -W -W
fc10: -W -W -W -W -W -W -W -W -W -W -W -W -W -- -W RW
fc20: R- R- R- R- -- -- -- R- R- R- R- R- R- R- R- R-
fc30: R- R- R- R- R- R- R- R- R- R- R- R- R- R- R- R-
fc40: R- R- R- R- R- R- R- R- R- R- R- R- R- R- R- R-
fc50: R- R- R- R- R- R- R- R- R- R- R- R- R- R- R- -W
fc60: -W -W -- -- -- -- -- -W -W -W -- -- -- -- -- --
fc70: -- -- -- -- -- -- RW -- -W -W RW RW R- R- -- --
fc80: -- -- R- R- -- R- -- -- -- -- -- -- -- -- -- --
also...
ff90: -- -- -- -- -W -- -- -- -W -- -- -- -- -- -- --

fc00      R  irq vector
fc01-fc1c  W enemy data (4 bytes per enemy: status, y, x, unknown)
fc1e       W current credits
fc1f-fc23 R  input ports
fc27-fc5e R  enemy-player coords comparison results
fc5f-fc61  W p1 data (status, y, x)
fc67-fc69  W p2 data (status, y, x)
fc76      RW unknown but should be in the range 00-1e, or even 00-28
             it affects how quickly the bubbles go up, is the MCU supposed to change it?
fc78-fc79  W stop clock counter (initialized to 0258 = 10 seconds)
fc7a-fc7b RW related to stop clock item. Z80 sets fc7a to 01 when time is stopped.
             The MCU counts down fc78-fc79, and when it reaches 0 sets fc7a to 00
             and fc7b to 01.
fc7c      R  EXTEND randomization (must be in range 00-05)
fc7d      R  MCU I/O error (must be 00)
fc82-fc83 R  MCU ROM checksum (must be 00)
fc85      R  "running ok" signal to Z80 (must be 37)
ff94       W coin lockout (ff = accept coins 01 = lock out)
ff98       W unknown, 47 or 00. irq related?
*/

static void mcu_compare_coords(void)
{
	int player,enemy,coord;

	for (player = 0;player < 2;player++)
	{
		for (coord = 0;coord < 2;coord++)
		{
			int player_coord = bublbobl_mcu_sharedram[0x060 + 8 * player + coord];

			for (enemy = 0;enemy < 7;enemy++)
			{
				int enemy_coord = bublbobl_mcu_sharedram[0x002 + 4 * enemy + coord];
				int flag,absdiff;

				if (player_coord == enemy_coord)
					flag = 0x80;
				else if (player_coord < enemy_coord)
					flag = 0x01;
				else
					flag = 0x00;

				bublbobl_mcu_sharedram[0x027 + player + 2 * coord + 8 * enemy] = flag;

				absdiff = player_coord - enemy_coord;
				if (absdiff < 0) absdiff = -absdiff;

				bublbobl_mcu_sharedram[0x027+4 + player + 2 * coord + 8 * enemy] = absdiff;
			}
		}
	}
}

static void mcu_simulate(void)
{
	bublbobl_mcu_sharedram[0x000] = 0x2e;	// IRQ vector
	bublbobl_mcu_sharedram[0x01f] = readinputport(0);	// coins (we should handle this differently)
	bublbobl_mcu_sharedram[0x020] = readinputport(1);	// dsw a
	bublbobl_mcu_sharedram[0x021] = readinputport(2);	// dsw b
	bublbobl_mcu_sharedram[0x022] = readinputport(3);	// player 1
	bublbobl_mcu_sharedram[0x023] = readinputport(4);	// player 2

	mcu_compare_coords();

	if (bublbobl_mcu_sharedram[0x07a] == 0x01)	// timer is stopped
	{
		bublbobl_mcu_sharedram[0x078]--;
		if (bublbobl_mcu_sharedram[0x078] == 0xff)
		bublbobl_mcu_sharedram[0x079]--;
		if (bublbobl_mcu_sharedram[0x079] == 0xff)
		{
			bublbobl_mcu_sharedram[0x07a] = 0x00;	// restart enemies
			bublbobl_mcu_sharedram[0x07b] = 0x01;	// restore original screen color
		}
	}

	bublbobl_mcu_sharedram[0x07c] = mame_rand() % 6;	// EXTEND randomization (WRONG)
	bublbobl_mcu_sharedram[0x07d] = 0x00;	// MCU I/O error (must be 00)
	bublbobl_mcu_sharedram[0x082] = 0x00;	// MCU ROM checksum (must be 00)
	bublbobl_mcu_sharedram[0x083] = 0x00;	// MCU ROM checksum (must be 00)
	bublbobl_mcu_sharedram[0x085] = 0x37;	// "running ok" signal to Z80 (must be 37)

	if (bublbobl_mcu_sharedram[0x394] == 0x01)	// coin lockout
		coin_lockout_global_w(1);
	else
		coin_lockout_global_w(0);
}

INTERRUPT_GEN( bublbobl_interrupt )
{
	if (mcu_running)
		mcu_simulate();

	cpunum_set_input_line_vector(0,0,bublbobl_mcu_sharedram[0]);
	cpunum_set_input_line(0,0,HOLD_LINE);
}


/***************************************************************************

Bobble Bobble protection (IC43). This appears to be a PAL.

Note: the checks on the values returned by ic43_b_r are actually patched out
in boblbobl, so they don't matter. All checks are patched out in sboblbob.

***************************************************************************/

static int ic43_a,ic43_b;


READ8_HANDLER( boblbobl_ic43_a_r )
{
//  if (offset >= 2)
//      logerror("%04x: ic43_a_r (offs %d) res = %02x\n",activecpu_get_pc(),offset,res);

	if (offset == 0)
		return ic43_a << 4;
	else
		return rand() & 0xff;
}

WRITE8_HANDLER( boblbobl_ic43_a_w )
{
	int res = 0;

	switch (offset)
	{
		case 0:
			if (~ic43_a & 8) res ^= 1;
			if (~ic43_a & 1) res ^= 2;
			if (~ic43_a & 1) res ^= 4;
			if (~ic43_a & 2) res ^= 4;
			if (~ic43_a & 4) res ^= 8;
			break;
		case 1:
			if (~ic43_a & 8) res ^= 1;
			if (~ic43_a & 2) res ^= 1;
			if (~ic43_a & 8) res ^= 2;
			if (~ic43_a & 1) res ^= 4;
			if (~ic43_a & 4) res ^= 8;
			break;
		case 2:
			if (~ic43_a & 4) res ^= 1;
			if (~ic43_a & 8) res ^= 2;
			if (~ic43_a & 2) res ^= 4;
			if (~ic43_a & 1) res ^= 8;
			if (~ic43_a & 4) res ^= 8;
			break;
		case 3:
			if (~ic43_a & 2) res ^= 1;
			if (~ic43_a & 4) res ^= 2;
			if (~ic43_a & 8) res ^= 2;
			if (~ic43_a & 8) res ^= 4;
			if (~ic43_a & 1) res ^= 8;
			break;
	}
	ic43_a = res;
}

WRITE8_HANDLER( boblbobl_ic43_b_w )
{
	static int xor[4] = { 4, 1, 8, 2 };

//  logerror("%04x: ic43_b_w (offs %d) %02x\n",activecpu_get_pc(),offset,data);
	ic43_b = (data >> 4) ^ xor[offset];
}

READ8_HANDLER( boblbobl_ic43_b_r )
{
//  logerror("%04x: ic43_b_r (offs %d)\n",activecpu_get_pc(),offset);
	if (offset == 0)
		return ic43_b << 4;
	else
		return rand() & 0xff;
}



/***************************************************************************

 Bootleg Bubble Bobble 68705 protection interface

 Not used at the moment, left in only for reference. Note that this actually
 wasn't working 100%, for some unknown reason the enemy movement wasn't right.

 The following is ENTIRELY GUESSWORK!!!

***************************************************************************/

#if 0

INTERRUPT_GEN( bublbobl_m68705_interrupt )
{
	/* I don't know how to handle the interrupt line so I just toggle it every time. */
	if (cpu_getiloops() & 1)
		cpunum_set_input_line(3,0,CLEAR_LINE);
	else
		cpunum_set_input_line(3,0,ASSERT_LINE);
}



static UINT8 portA_in,portA_out,ddrA;

READ8_HANDLER( bublbobl_68705_portA_r )
{
//logerror("%04x: 68705 port A read %02x\n",activecpu_get_pc(),portA_in);
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE8_HANDLER( bublbobl_68705_portA_w )
{
//logerror("%04x: 68705 port A write %02x\n",activecpu_get_pc(),data);
	portA_out = data;
}

WRITE8_HANDLER( bublbobl_68705_ddrA_w )
{
	ddrA = data;
}



/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  0   W  enables latch which holds data from main Z80 memory
 *  1   W  loads the latch which holds the low 8 bits of the address of
 *               the main Z80 memory location to access
 *  2   W  loads the latch which holds the high 4 bits of the address of
 *               the main Z80 memory location to access
 *         00-07 = read input ports
 *         0c-0f = access z80 memory at 0xfc00
 *  3   W  selects Z80 memory access direction (0 = write 1 = read)
 *  4   W  clocks main Z80 memory access (goes to a PAL)
 *  5   W  clocks a flip-flop which causes IRQ on the main Z80
 *  6   W  not used?
 *  7   W  not used?
 */

static UINT8 portB_in,portB_out,ddrB;

READ8_HANDLER( bublbobl_68705_portB_r )
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

static int address,latch;

WRITE8_HANDLER( bublbobl_68705_portB_w )
{
//logerror("%04x: 68705 port B write %02x\n",activecpu_get_pc(),data);

	if ((ddrB & 0x01) && (~data & 0x01) && (portB_out & 0x01))
	{
		portA_in = latch;
	}
	if ((ddrB & 0x02) && (data & 0x02) && (~portB_out & 0x02)) /* positive edge trigger */
	{
		address = (address & 0xff00) | portA_out;
//logerror("%04x: 68705 address %02x\n",activecpu_get_pc(),portA_out);
	}
	if ((ddrB & 0x04) && (data & 0x04) && (~portB_out & 0x04)) /* positive edge trigger */
	{
		address = (address & 0x00ff) | ((portA_out & 0x0f) << 8);
	}
	if ((ddrB & 0x10) && (~data & 0x10) && (portB_out & 0x10))
	{
		if (data & 0x08)	/* read */
		{
			if ((address & 0x0800) == 0x0000)
			{
//logerror("%04x: 68705 read input port %02x\n",activecpu_get_pc(),address);
				latch = readinputport((address & 3) + 1);
			}
			else if ((address & 0x0c00) == 0x0c00)
			{
//logerror("%04x: 68705 read %02x from address %04x\n",activecpu_get_pc(),bublbobl_mcu_sharedram[address],address);
				latch = bublbobl_mcu_sharedram[address & 0x03ff];
			}
			else
logerror("%04x: 68705 unknown read address %04x\n",activecpu_get_pc(),address);
		}
		else	/* write */
		{
			if ((address & 0x0c00) == 0x0c00)
			{
//logerror("%04x: 68705 write %02x to address %04x\n",activecpu_get_pc(),portA_out,address);
				bublbobl_mcu_sharedram[address & 0x03ff] = portA_out;
			}
			else
logerror("%04x: 68705 unknown write to address %04x\n",activecpu_get_pc(),address);
		}
	}
	if ((ddrB & 0x20) && (~data & 0x20) && (portB_out & 0x20))
	{
		/* hack to get random EXTEND letters (who is supposed to do this? 68705? PAL?) */
		bublbobl_mcu_sharedram[0x7c] = mame_rand()%6;

		cpunum_set_input_line_vector(0,0,bublbobl_mcu_sharedram[0]);
		cpunum_set_input_line(0,0,HOLD_LINE);
	}
	if ((ddrB & 0x40) && (~data & 0x40) && (portB_out & 0x40))
	{
logerror("%04x: 68705 unknown port B bit %02x\n",activecpu_get_pc(),data);
	}
	if ((ddrB & 0x80) && (~data & 0x80) && (portB_out & 0x80))
	{
logerror("%04x: 68705 unknown port B bit %02x\n",activecpu_get_pc(),data);
	}

	portB_out = data;
}

WRITE8_HANDLER( bublbobl_68705_ddrB_w )
{
	ddrB = data;
}

#endif

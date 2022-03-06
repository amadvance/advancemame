/***************************************************************************

    Atari Major Havoc hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/avgdvg.h"
#include "cpu/m6502/m6502.h"
#include "mhavoc.h"

static UINT8 alpha_data;
static UINT8 alpha_rcvd;
static UINT8 alpha_xmtd;

static UINT8 gamma_data;
static UINT8 gamma_rcvd;
static UINT8 gamma_xmtd;

static UINT8 player_1;

static UINT8 alpha_irq_clock;
static UINT8 alpha_irq_clock_enable;
static UINT8 gamma_irq_clock;

static UINT8 *ram_base;
static UINT8 has_gamma_cpu;



/*************************************
 *
 *  Interrupt handling
 *
 *************************************/

static void cpu_irq_clock(int param)
{
	/* clock the LS161 driving the alpha CPU IRQ */
	if (alpha_irq_clock_enable)
	{
		alpha_irq_clock++;
		if ((alpha_irq_clock & 0x0c) == 0x0c)
		{
			cpunum_set_input_line(0, 0, ASSERT_LINE);
			alpha_irq_clock_enable = 0;
		}
	}

	/* clock the LS161 driving the gamma CPU IRQ */
	if (has_gamma_cpu)
	{
		gamma_irq_clock++;
		cpunum_set_input_line(1, 0, (gamma_irq_clock & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	}
}


WRITE8_HANDLER( mhavoc_alpha_irq_ack_w )
{
	/* clear the line and reset the clock */
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	alpha_irq_clock = 0;
	alpha_irq_clock_enable = 1;
}


WRITE8_HANDLER( mhavoc_gamma_irq_ack_w )
{
	/* clear the line and reset the clock */
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	gamma_irq_clock = 0;
}



/*************************************
 *
 *  Machine init
 *
 *************************************/

MACHINE_RESET( mhavoc )
{
	/* cache the base of memory region 1 */
	ram_base = memory_region(REGION_CPU1);
	has_gamma_cpu = (cpu_gettotalcpu() > 1);

	/* reset RAM/ROM banks to 0 */
	mhavoc_ram_banksel_w(0, 0);
	mhavoc_rom_banksel_w(0, 0);

	/* reset alpha comm status */
	alpha_data = 0;
	alpha_rcvd = 0;
	alpha_xmtd = 0;

	/* reset gamma comm status */
	gamma_data = 0;
	gamma_rcvd = 0;
	gamma_xmtd = 0;

	/* reset player 1 flag */
	player_1 = 0;

	/* reset IRQ clock states */
	alpha_irq_clock = 0;
	alpha_irq_clock_enable = 1;
	gamma_irq_clock = 0;

	/* set a timer going for the CPU interrupt generators */
	timer_pulse(TIME_IN_HZ(MHAVOC_CLOCK_5K), 0, cpu_irq_clock);
}



/*************************************
 *
 *  Alpha -> gamma communications
 *
 *************************************/

static void delayed_gamma_w(int data)
{
	/* mark the data received */
	gamma_rcvd = 0;
	alpha_xmtd = 1;
	alpha_data = data;

	/* signal with an NMI pulse */
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);

	/* the sound CPU needs to reply in 250microseconds (according to Neil Bradley) */
	timer_set(TIME_IN_USEC(250), 0, 0);
}


WRITE8_HANDLER( mhavoc_gamma_w )
{
	timer_set(TIME_NOW, data, delayed_gamma_w);
}


READ8_HANDLER( mhavoc_alpha_r )
{
	gamma_rcvd = 1;
	alpha_xmtd = 0;
	return alpha_data;
}



/*************************************
 *
 *  Gamma -> alpha communications
 *
 *************************************/

WRITE8_HANDLER( mhavoc_alpha_w )
{
	alpha_rcvd = 0;
	gamma_xmtd = 1;
	gamma_data = data;
}


READ8_HANDLER( mhavoc_gamma_r )
{
	alpha_rcvd = 1;
	gamma_xmtd = 0;
	return gamma_data;
}



/*************************************
 *
 *  RAM/ROM banking
 *
 *************************************/

WRITE8_HANDLER( mhavoc_ram_banksel_w )
{
	static const offs_t bank[2] = { 0x20200, 0x20800 };

	data &= 0x01;
	memory_set_bankptr(1, &ram_base[bank[data]]);
}


WRITE8_HANDLER( mhavoc_rom_banksel_w )
{
	static const offs_t bank[4] = { 0x10000, 0x12000, 0x14000, 0x16000 };

	data &= 0x03;
	memory_set_bankptr(2, &ram_base[bank[data]]);
}


WRITE8_HANDLER( mhavocpe_rom_banksel_w )
{
	static const offs_t bank[8] = { 0x30000, 0x32000, 0x34000, 0x36000, 0x38000, 0x3a000, 0x3c000, 0x3e000 };
    data = (data & 1) | ((data & 2)<<1) | ((data & 4)>>1);
	memory_set_bankptr(2, &ram_base[bank[data]]);
}



/*************************************
 *
 *  Input ports
 *
 *************************************/

READ8_HANDLER( mhavoc_port_0_r )
{
	UINT8 res;

	/* Bits 7-6 = selected based on Player 1 */
	/* Bits 5-4 = common */
	if (player_1)
		res = (readinputport(0) & 0x30) | (readinputport(5) & 0xc0);
	else
		res = readinputport(0) & 0xf0;

	/* Bit 3 = Gamma rcvd flag */
	if (gamma_rcvd)
		res |= 0x08;

	/* Bit 2 = Gamma xmtd flag */
	if (gamma_xmtd)
		res |= 0x04;

	/* Bit 1 = 2.4kHz (divide 2.5MHz by 1024) */
	if (!(activecpu_gettotalcycles() & 0x400))
		res |= 0x02;

	/* Bit 0 = Vector generator halt flag */
	if (avgdvg_done())
		res |= 0x01;

	return res;
}


READ8_HANDLER( alphaone_port_0_r )
{
	/* Bits 7-2 = common */
	UINT8 res = readinputport(0) & 0xfc;

	/* Bit 1 = 2.4kHz (divide 2.5MHz by 1024) */
	if (!(activecpu_gettotalcycles() & 0x400))
		res |= 0x02;

	/* Bit 0 = Vector generator halt flag */
	if (avgdvg_done())
		res |= 0x01;

	return res;
}


READ8_HANDLER( mhavoc_port_1_r )
{
	/* Bits 7-2 = input switches */
	UINT8 res = readinputport(1) & 0xfc;

	/* Bit 1 = Alpha rcvd flag */
	if (has_gamma_cpu && alpha_rcvd)
		res |= 0x02;

	/* Bit 0 = Alpha xmtd flag */
	if (has_gamma_cpu && alpha_xmtd)
		res |= 0x01;

	return res;
}



/*************************************
 *
 *  Output ports
 *
 *************************************/

WRITE8_HANDLER( mhavoc_out_0_w )
{
	/* Bit 7 = Invert Y -- unemulated */
	/* Bit 6 = Invert X -- unemulated */

	/* Bit 5 = Player 1 */
	player_1 = (data >> 5) & 1;

	/* Bit 3 = Gamma reset */
	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x08) ? CLEAR_LINE : ASSERT_LINE);
	if (!(data & 0x08))
	{
		logerror("\t\t\t\t*** resetting gamma processor. ***\n");
		alpha_rcvd = 0;
		alpha_xmtd = 0;
		gamma_rcvd = 0;
		gamma_xmtd = 0;
	}

	/* Bit 0 = Roller light (Blinks on fatal errors) */
	set_led_status(0, data & 0x01);
}


WRITE8_HANDLER( alphaone_out_0_w )
{
	/* Bit 5 = P2 lamp */
	set_led_status(0, ~data & 0x20);

	/* Bit 4 = P1 lamp */
	set_led_status(1, ~data & 0x10);

	/* Bit 1 = right coin counter */
	coin_counter_w(1, data & 0x02);

	/* Bit 0 = left coin counter */
	coin_counter_w(0, data & 0x01);

}


WRITE8_HANDLER( mhavoc_out_1_w )
{
	/* Bit 1 = left coin counter */
	coin_counter_w(0, data & 0x02);

	/* Bit 0 = right coin counter */
	coin_counter_w(1, data & 0x01);
}

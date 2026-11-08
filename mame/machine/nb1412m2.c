// license:LGPL-2.1+
// copyright-holders:Angelo Salese
/* Nichibutsu 1412M2 data decrypter, timer and sample playback. */
#include "driver.h"
#include "sound/dac.h"
#include "includes/nb1412m2.h"

static UINT8 command, registers[256];
static UINT16 rom_address, adjust_address, dac_address, dac_current;
static int timer_flag, dac_playing, timer_rate;
static mame_timer *main_timer, *dac_timer;

static void main_timer_tick(int param)
{
	timer_flag = 1;
}

static void dac_timer_tick(int param)
{
	if (dac_playing)
	{
		UINT8 data = memory_region(REGION_USER1)[dac_current];
		DAC_0_data_w(0, data);
		dac_current = (dac_current + 1) & 0x7fff;
		if (data == 0x80)
		{
			dac_playing = 0;
			timer_adjust(dac_timer, TIME_NEVER, 0, 0);
		}
	}
}

void nb1412m2_init(void)
{
	main_timer = timer_alloc(main_timer_tick);
	dac_timer = timer_alloc(dac_timer_tick);
}

void nb1412m2_reset(void)
{
	command = 0xff;
	memset(registers, 0, sizeof(registers));
	registers[0x90] = 0x18;
	rom_address = adjust_address = dac_address = dac_current = 0;
	timer_flag = dac_playing = 0;
	timer_rate = 2;
	timer_adjust(main_timer, TIME_NEVER, 0, 0);
	timer_adjust(dac_timer, TIME_NEVER, 0, 0);
	DAC_0_data_w(0, 0x80);
}

WRITE8_HANDLER( nb1412m2_command_w )
{
	command = data;
}

READ8_HANDLER( nb1412m2_data_r )
{
	switch (command)
	{
	case 0x37:
	{
		const UINT8 *rom = memory_region(REGION_USER1);
		UINT8 adjust = 0x43 - rom[adjust_address & 0x7fff];
		return rom[rom_address & 0x1fff] - adjust;
	}
	case 0x41:
		return timer_flag;
	case 0x90: case 0x92: case 0x94:
	case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		return registers[command];
	default:
		return 0xff;
	}
}

WRITE8_HANDLER( nb1412m2_data_w )
{
	switch (command)
	{
	case 0x11:
		if (data == 0)
		{
			dac_playing = 0;
			timer_adjust(dac_timer, TIME_NEVER, 0, 0);
		}
		break;
	case 0x18:
		timer_rate = ((data & 0x30) >> 4) + 1;
		break;
	case 0x32:
		if (data == 2)
		{
			double period = TIME_IN_HZ(4000000.0 / (256 * 12) * timer_rate);
			dac_current = dac_address & 0x7fff;
			dac_playing = 1;
			timer_adjust(dac_timer, period, 0, period);
		}
		break;
	case 0x33:
		rom_address = (rom_address & 0xff) | (data << 8);
		break;
	case 0x34:
		rom_address = (rom_address & 0xff00) | data;
		break;
	case 0x35:
		adjust_address = (adjust_address & 0xff) | (data << 8);
		break;
	case 0x36:
		adjust_address = (adjust_address & 0xff00) | data;
		break;
	case 0x40:
		timer_adjust(main_timer, TIME_IN_HZ(4000000.0 / (256 * 35) * timer_rate), 0, 0);
		break;
	case 0x41:
		timer_flag = 0;
		timer_adjust(main_timer, TIME_NEVER, 0, 0);
		break;
	case 0x51:
		dac_address = (dac_address & 0xff) | (data << 8);
		break;
	case 0x52:
		dac_address = (dac_address & 0xff00) | data;
		break;
	case 0x90: case 0x92: case 0x94:
	case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		registers[command] = data;
		break;
	}
}

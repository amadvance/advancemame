// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/***************************************************************************

    National Semiconductor ADC1038

    10-Bit Serial I/O A/D Converters with Analog Multiplexer and
    Track/hold Function

***************************************************************************/

#include "driver.h"
#include "adc1038.h"

static int adc1038_cycle;
static int adc1038_clk;
static int adc1038_adr;
static int adc1038_data_in;
static int adc1038_data_out;
static int adc1038_adc_data;
static int adc1038_sars;
static int adc1038_gticlub_hack;
static int (*adc1038_input_callback)(int input);

void adc1038_init(int (*input_callback)(int input), int gticlub_hack)
{
	adc1038_input_callback = input_callback;
	adc1038_gticlub_hack = gticlub_hack;

	state_save_register_global(adc1038_cycle);
	state_save_register_global(adc1038_clk);
	state_save_register_global(adc1038_adr);
	state_save_register_global(adc1038_data_in);
	state_save_register_global(adc1038_data_out);
	state_save_register_global(adc1038_adc_data);
	state_save_register_global(adc1038_sars);

	adc1038_reset();
}

void adc1038_reset(void)
{
	adc1038_cycle = 0;
	adc1038_clk = 0;
	adc1038_adr = 0;
	adc1038_data_in = 0;
	adc1038_data_out = 0;
	adc1038_adc_data = 0;
	adc1038_sars = 1;
}

int adc1038_do_read(void)
{
	adc1038_data_out = (adc1038_adc_data & 0x200) ? 1 : 0;
	adc1038_adc_data <<= 1;
	return adc1038_data_out;
}

void adc1038_di_write(int state)
{
	adc1038_data_in = state;
}

void adc1038_clk_write(int state)
{
	/* GTI Club doesn't sync on SARS */
	if (adc1038_gticlub_hack)
	{
		if (adc1038_clk == 0 && state == 0)
		{
			adc1038_cycle = 0;
			adc1038_adc_data = adc1038_input_callback(adc1038_adr);
		}
	}

	if (state == 1)
	{
		if (adc1038_cycle == 0)		/* A2 */
		{
			adc1038_adr = 0;
			adc1038_adr |= (adc1038_data_in << 2);
		}
		else if (adc1038_cycle == 1)	/* A1 */
		{
			adc1038_adr |= (adc1038_data_in << 1);
		}
		else if (adc1038_cycle == 2)	/* A0 */
		{
			adc1038_adr |= (adc1038_data_in << 0);
		}

		adc1038_cycle++;
	}

	adc1038_clk = state;
}

int adc1038_sars_read(void)
{
	adc1038_cycle = 0;
	adc1038_adc_data = adc1038_input_callback(adc1038_adr);
	adc1038_sars ^= 1;
	return adc1038_sars;
}

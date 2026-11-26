// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/***************************************************************************

    National Semiconductor ADC1038

    10-Bit Serial I/O A/D Converters with Analog Multiplexer and
    Track/hold Function

***************************************************************************/

#ifndef _ADC1038_H
#define _ADC1038_H

/* gticlub_hack: GTI Club doesn't sync on SARS */
void adc1038_init(int (*input_callback)(int input), int gticlub_hack);
void adc1038_reset(void);
int adc1038_do_read(void);
int adc1038_sars_read(void);
void adc1038_di_write(int state);
void adc1038_clk_write(int state);

#endif

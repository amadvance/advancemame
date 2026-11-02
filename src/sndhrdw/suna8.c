// license:BSD-3-Clause
// copyright-holders:Luca Elia
/*

    SunA 8 Bit Games samples

    Formats: unsigned 8-bit PCM at 4 kHz, packed 4-bit PCM at 8 kHz

*/

#include "driver.h"
#include "sound/samples.h"

static INT16 *samplebuf;
static int sample;
static UINT8 sample_trigger;
int suna8_sample_4bit;

WRITE8_HANDLER( suna8_play_samples_w )
{
	if (suna8_sample_4bit)
	{
		int index = -1;
		if (sample_trigger == 0xe9 && data == 0xf9) index = sample & 0xf;
		if (sample_trigger == 0xb9 && data == 0xf9) index = ((sample >> 4) & 0xf) + 16;
		/* trigger edges select a ROM; startup writes must not play a spurious sample */
		if (index >= 0 && index < memory_region_length(REGION_SOUND1) / 0x800)
			sample_start_raw(0, &samplebuf[index * 0x1000], 0x1000, 8000, 0);
		sample_trigger = data;
		return;
	}
	if( data )
	{
		if( ~data & 0x10 )
		{
			sample_start_raw(0, &samplebuf[0x800*sample], 0x0800, 4000, 0);
		}
		else if( ~data & 0x08 )
		{
			sample &= 3;
			sample_start_raw(0, &samplebuf[0x800*(sample+7)], 0x0800, 4000, 0);
		}
	}
}

WRITE8_HANDLER( suna8_samples_number_w )
{
	sample = suna8_sample_4bit ? data : (data & 0xf);
}

void suna8_sh_start(void)
{
	int i;
	unsigned char *ROM = memory_region(REGION_SOUND1);

	sample = sample_trigger = 0;
	if (suna8_sample_4bit)
	{
		int len = memory_region_length(REGION_SOUND1) * 2;
		samplebuf = auto_malloc(len * sizeof(samplebuf[0]));
		for (i = 0; i < len; ++i)
			samplebuf[i] = (INT8)(((ROM[i / 2] << ((i & 1) ? 0 : 4)) & 0xf0) ^ 0x80) * 256;
		return;
	}

	samplebuf = auto_malloc(memory_region_length(REGION_SOUND1) * sizeof(samplebuf[0]));

	for(i=0;i<memory_region_length(REGION_SOUND1);i++)
		samplebuf[i] = (INT8)(ROM[i] ^ 0x80) * 256;
}

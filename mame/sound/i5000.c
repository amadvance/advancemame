// license:BSD-3-Clause
// copyright-holders:hap
/*
    Imagetek I5000 sound emulator

    Imagetek I5000 is a multi-purpose chip, this covers the sound part.
    No official documentation is known to exist. It seems to be a simple
    16-channel ADPCM player.
*/

#include "sndintrf.h"
#include "streams.h"
#include "i5000.h"
#include "okim6295.h"
#include <math.h>

#define CHANNELS 16

struct i5000_channel
{
	int is_playing;
	struct adpcm_state adpcm;

	UINT32 address;
	int freq_timer;
	int freq_base;
	int freq_min;
	UINT16 sample;
	UINT8 shift_pos;
	UINT8 shift_amount;
	UINT8 shift_mask;
	int vol_r;
	int vol_l;
	int output_r;
	int output_l;
};

struct i5000_info
{
	const struct I5000interface *intf;

	sound_stream *stream;

	struct i5000_channel channels[CHANNELS];

	UINT16 regs[0x80];

	UINT16 *rom;
	UINT32 rom_mask;

	int lut_volume[0x100];
};

/* read the next sample word, return 0 when the sample ends */
static int i5000_read_sample(struct i5000_info *info, int ch)
{
	struct i5000_channel *channel = &info->channels[ch];

	channel->shift_pos &= 0xf;
	channel->sample = info->rom[channel->address];
	channel->address = (channel->address + 1) & info->rom_mask;

	/* a 0x7f7f word introduces a command, all of them end the sample */
	if (channel->sample == 0x7f7f)
	{
		channel->address = (channel->address + 1) & info->rom_mask;
		return 0;
	}

	return 1;
}

static void i5000_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct i5000_info *info = param;
	int i, ch;

	for (i = 0; i < length; i++)
	{
		INT32 mix_l = 0;
		INT32 mix_r = 0;

		for (ch = 0; ch < CHANNELS; ch++)
		{
			struct i5000_channel *channel = &info->channels[ch];
			int adpcm_data;

			if (!channel->is_playing)
				continue;

			channel->freq_timer -= channel->freq_min;
			if (channel->freq_timer > 0)
			{
				mix_r += channel->output_r;
				mix_l += channel->output_l;
				continue;
			}
			channel->freq_timer += channel->freq_base;

			adpcm_data = channel->sample >> channel->shift_pos;
			channel->shift_pos += channel->shift_amount;
			if (channel->shift_pos & 0x10)
			{
				if (!i5000_read_sample(info, ch))
				{
					channel->is_playing = 0;
					continue;
				}

				adpcm_data |= channel->sample << (channel->shift_amount - channel->shift_pos);
			}

			adpcm_data = clock_adpcm(&channel->adpcm, adpcm_data & channel->shift_mask);

			channel->output_r = adpcm_data * channel->vol_r / 128;
			channel->output_l = adpcm_data * channel->vol_l / 128;
			mix_r += channel->output_r;
			mix_l += channel->output_l;
		}

		mix_r /= 256;
		mix_l /= 256;
		if (mix_r > 32767)
			mix_r = 32767;
		else if (mix_r < -32768)
			mix_r = -32768;
		if (mix_l > 32767)
			mix_l = 32767;
		else if (mix_l < -32768)
			mix_l = -32768;
		buffer[0][i] = mix_r;
		buffer[1][i] = mix_l;
	}
}

static void i5000_write_reg16(struct i5000_info *info, int reg, UINT16 data)
{
	int ch;

	if (reg < 0x40)
	{
		/* channel regs: 0 and 1 are the address */
		struct i5000_channel *channel = &info->channels[reg >> 2];

		switch (reg & 3)
		{
			case 2:
				channel->freq_base = (0x1ff - (data & 0xff)) << (~data >> 8 & 3);
				break;
			case 3:
				channel->vol_r = info->lut_volume[data & 0xff];
				channel->vol_l = info->lut_volume[data >> 8 & 0xff];
				break;
		}
	}
	else
	{
		switch (reg)
		{
			case 0x42:
				/* channel key on */
				for (ch = 0; ch < CHANNELS; ch++)
				{
					struct i5000_channel *channel = &info->channels[ch];

					if ((data & (1 << ch)) && !channel->is_playing)
					{
						UINT32 address = info->regs[ch << 2 | 1] << 16 | info->regs[ch << 2];
						UINT16 start = info->rom[(address + 0) & info->rom_mask];
						UINT16 param = info->rom[(address + 1) & info->rom_mask];

						/* check sample start ID */
						if (start != 0x7f7f)
							continue;

						switch (param)
						{
							/* 3-bit ADPCM */
							case 0x0104:
							case 0x0304:
								channel->freq_min = 0x140;
								channel->shift_amount = 3;
								channel->shift_mask = 0xe;
								break;

							/* 4-bit ADPCM, also used for unknown formats */
							default:
								channel->freq_min = 0x100;
								channel->shift_amount = 4;
								channel->shift_mask = 0xf;
								break;
						}

						channel->address = (address + 4) & info->rom_mask;

						channel->freq_timer = 0;
						channel->shift_pos = 0;

						reset_adpcm(&channel->adpcm);
						channel->is_playing = i5000_read_sample(info, ch);
					}
				}
				break;

			case 0x43:
				/* channel key off */
				for (ch = 0; ch < CHANNELS; ch++)
				{
					if (data & (1 << ch))
						info->channels[ch].is_playing = 0;
				}
				break;
		}
	}

	info->regs[reg] = data;
}

static void *i5000_start(int sndindex, int clock, const void *config)
{
	struct i5000_info *info;
	double vol;
	int i;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config;

	info->rom = (UINT16*)memory_region(info->intf->region);
	info->rom_mask = memory_region_length(info->intf->region) / 2 - 1;

	/* fill volume table */
	vol = 2047.0;
	for (i = 0; i < 0x100; i++)
	{
		info->lut_volume[i] = (int)(vol + 0.5);
		vol /= 1.032;
	}
	info->lut_volume[0xff] = 0;

	info->stream = stream_create(0, 2, clock / 0x400, info, i5000_update);

	/* stop playing and reset the channel regs */
	i5000_write_reg16(info, 0x43, 0xffff);
	for (i = 0; i < 0x40; i++)
		i5000_write_reg16(info, i, 0);

	return info;
}

/*****************************************************************************/

READ16_HANDLER( I5000_0_r )
{
	struct i5000_info *info = sndti_token(SOUND_I5000, 0);
	UINT16 ret = 0;

	stream_update(info->stream, 0);

	/* only the channel active state is readable */
	if (offset == 0x42)
	{
		int ch;

		for (ch = 0; ch < CHANNELS; ch++)
		{
			if (info->channels[ch].is_playing)
				ret |= 1 << ch;
		}
	}

	return ret;
}

WRITE16_HANDLER( I5000_0_w )
{
	struct i5000_info *info = sndti_token(SOUND_I5000, 0);

	/* partial writes are never used */
	if (mem_mask != 0)
		return;

	stream_update(info->stream, 0);

	i5000_write_reg16(info, offset, data);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void i5000_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void i5000_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = i5000_set_info;		break;
		case SNDINFO_PTR_START:							info->start = i5000_start;				break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "I5000";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Imagetek PCM";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2026, The MAME Team"; break;
	}
}

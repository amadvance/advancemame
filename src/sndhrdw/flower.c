// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

    Flower custom sound chip

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "sound/custom.h"

#define MAX_VOICES 8

static const int defgain = 48;

typedef struct
{
	UINT8 start_nibbles[6];
	UINT8 raw_frequency[4];
	UINT32 start_address;
	UINT32 position;
	UINT16 frequency;
	UINT8 volume;
	UINT8 volume_bank;
	UINT8 effect;
	int enable;
	int repeat;
	int channel_number;
} fl_sound_channel;

static fl_sound_channel channel_list[MAX_VOICES];
static fl_sound_channel *last_channel;

static const UINT8 *sample_rom;
static const UINT8 *volume_rom;
static sound_stream *stream;

static INT16 *mixer_table;
static INT16 *mixer_lookup;
static short *mixer_buffer;

static int make_mixer_table(int voices, int gain)
{
	int count = voices * 128;
	int i;

	mixer_table = auto_malloc(256 * voices * sizeof(INT16));
	if (!mixer_table)
		return 1;

	mixer_lookup = mixer_table + (128 * voices);

	for (i = 0; i < count; i++)
	{
		int val = i * gain * 16 / voices;
		if (val > 32767)
			val = 32767;
		mixer_lookup[ i] = val;
		mixer_lookup[-i] = -val;
	}

	return 0;
}

static void flower_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	stream_sample_t *buffer = outputs[0];
	short *mix;
	UINT8 raw_sample;
	fl_sound_channel *voice;
	int i;

	memset(mixer_buffer, 0, samples * sizeof(short));

	for (voice = channel_list; voice < last_channel; voice++)
	{
		int ch_volume = voice->volume;
		int ch_frequency = voice->frequency;

		if (!voice->enable)
			continue;

		mix = mixer_buffer;

		for (i = 0; i < samples; i++)
		{
			if (voice->repeat)
			{
				raw_sample = sample_rom[((voice->start_address >> 7) & 0x7e00) | ((voice->position >> 7) & 0x1ff)];
				if ((voice->position >> 7) & 0x20000)
				{
					voice->enable = 0;
					break;
				}
			}
			else
			{
				raw_sample = sample_rom[((voice->start_address + voice->position) >> 7) & 0x7fff];
				if (raw_sample == 0xff)
				{
					voice->enable = 0;
					break;
				}
			}
			ch_volume |= voice->volume_bank;

			*mix++ += volume_rom[(ch_volume << 8 | raw_sample) & 0x3fff] - 0x80;
			voice->position += ch_frequency;
		}
	}

	mix = mixer_buffer;
	for (i = 0; i < samples; i++)
		*buffer++ = mixer_lookup[*mix++];
}

void *flower_sh_start(int clock, const struct CustomSound_interface *config)
{
	fl_sound_channel *voice;

	sample_rom = memory_region(REGION_SOUND1);
	volume_rom = memory_region(REGION_SOUND2);

	stream = stream_create(0, 1, clock / 2, 0, flower_sound_update);

	mixer_buffer = auto_malloc((clock / 2) * sizeof(short));
	if (!mixer_buffer)
		return NULL;

	if (make_mixer_table(MAX_VOICES, defgain))
		return NULL;

	last_channel = channel_list + MAX_VOICES;

	for (voice = channel_list; voice < last_channel; voice++)
	{
		memset(voice, 0, sizeof(*voice));
		voice->channel_number = voice - channel_list;
	}

	return (void *)1;
}

void flower_sound_reset(void)
{
	fl_sound_channel *voice;

	for (voice = channel_list; voice < last_channel; voice++)
	{
		voice->start_address = 0;
		voice->position = 0;
		voice->volume = 0;
		voice->enable = 0;
		voice->repeat = 0;
	}
}

WRITE8_HANDLER( flower_sound1_w )
{
	UINT8 ch = (offset >> 3) & 0x7;
	fl_sound_channel *voice = &channel_list[ch];

	stream_update(stream, 0);

	switch (offset & 7)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			voice->raw_frequency[offset & 3] = data & 0xf;
			voice->frequency = (voice->raw_frequency[2] << 12) |
			                   (voice->raw_frequency[3] << 8) |
			                   (voice->raw_frequency[0] << 4) |
			                   (voice->raw_frequency[1] << 0);
			break;

		case 4:
			voice->repeat = (data >> 4) & 1;
			break;

		case 5:
			// unk_w
			break;

		case 7:
			voice->volume = data >> 4;
			break;
	}
}

WRITE8_HANDLER( flower_sound2_w )
{
	UINT8 ch = (offset >> 3) & 0x7;
	fl_sound_channel *voice = &channel_list[ch];

	stream_update(stream, 0);

	switch (offset & 7)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			voice->start_nibbles[offset & 7] = data & 0xf;
			if ((offset & 7) == 4)
				voice->effect = data >> 4;
			break;

		case 7:
		{
			int i;

			voice->enable = 1;
			voice->volume_bank = (data & 3) << 4;
			voice->start_address = 0;
			voice->position = 0;
			for (i = 5; i >= 0; i--)
				voice->start_address = (voice->start_address << 4) | voice->start_nibbles[i];
			break;
		}
	}
}

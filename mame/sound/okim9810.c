// license:BSD-3-Clause
// copyright-holders:Andrew Gardner
/***************************************************************************

    OKI MSM9810 / MSM9811 ADPCM(2) sound chips.

    TODO:
        Serial input/output are not supported
        8-bit Non-linear PCM Algorithm isn't implemented
        DADR Command is correct?

***************************************************************************/

#include <math.h>

#include "sndintrf.h"
#include "streams.h"
#include "okim9810.h"

#define OKIM9810_VOICES 8

/* the chip runs at the master clock, but the fastest sampling frequency is
   clock / 128, so the stream is generated at that rate and every voice is
   advanced in units of 128 master clock ticks */
#define OKIM9810_RATE_DIV 128

enum
{
	ADPCM_PLAYBACK = 0,
	ADPCM2_PLAYBACK = 1,
	NONLINEAR8_PLAYBACK = 2,
	STRAIGHT8_PLAYBACK = 3,
	EIGHTBIT_PLAYBACK = 2
};

enum
{
	SECONDARY_FILTER = 0,
	PRIMARY_FILTER = 1
};

enum
{
	SEQ_STOP = 0,
	SEQ_ACTIVE = 1,
	SEQ_PLAY = 2,
	SEQ_PAUSE = 3
};

struct okim9810_voice
{
	INT32 adpcm_signal;
	INT32 adpcm_step;
	UINT8 playbackAlgo;
	int looping;
	UINT32 base_offset;
	UINT32 count;
	UINT32 samplingFreq;

	int playing;
	UINT32 sample;
	UINT32 phrase_offset;
	INT32 phrase_count;
	INT32 phrase_wait_cnt;
	UINT8 phrase_state;

	UINT8 channel_volume;
	UINT8 pan_volume_left;
	UINT8 pan_volume_right;

	INT32 startSample;
	INT32 endSample;
	UINT32 interpPos;	/* position inside the sample, in master clock ticks */

	double end_time;	/* emulated time at which a single non looping phrase ends */
};

struct okim9810
{
	struct okim9810_voice voice[OKIM9810_VOICES];
	sound_stream *stream;
	UINT8 *region_base;
	int clock;

	UINT8 TMP_register;
	UINT8 global_volume;
	UINT8 filter_type;

	int dadr;
	UINT32 dadr_start_offset;
	UINT32 dadr_end_offset;
	UINT8 dadr_flags;
};

static const UINT8 volume_table[16] =
{
	0x80, 0x65, 0x50, 0x40, 0x32, 0x28, 0x20, 0x19,
	0x14, 0x10, 0x0c, 0x0a, 0x08, 0x06, 0x05, 0x04
};

/* sampling frequency divider, 1 is an invalid setting */
static const UINT32 sampling_freq_div_table[16] =
{
	1024, 512, 256, 128, 1, 640, 320, 160,
	1, 768, 384, 192, 1, 1, 1, 1
};

static const int adpcm_index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static const int adpcm2_index_shift[8] = { -2, -2, -2, -2, 2, 6, 9, 11 };

static int adpcm_diff_lookup[49*16];
static int adpcm2_diff_lookup[49*16];
static int tables_computed = 0;

static struct okim9810 *okim9810_chip;

static void compute_tables(void)
{
	static const int nbl2bit[16][4] =
	{
		{ 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
		{ 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
		{-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
		{-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
	};
	int step, nib;
	float floatstep = 64;

	for (step = 0; step <= 48; step++)
	{
		int stepval = (int)floor(16.0 * pow(11.0 / 10.0, (double)step));
		int stepval2 = (int)floor(floatstep * 1.08f);
		floatstep = floatstep * 1.08f;

		for (nib = 0; nib < 16; nib++)
		{
			adpcm_diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval * nbl2bit[nib][1] +
				 stepval/2 * nbl2bit[nib][2] +
				 stepval/4 * nbl2bit[nib][3] +
				 stepval/8);
			adpcm2_diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval2 * nbl2bit[nib][1] +
				 stepval2/2 * nbl2bit[nib][2] +
				 stepval2/4 * nbl2bit[nib][3] +
				 stepval2/8);
		}
	}

	tables_computed = 1;
}

static void adpcm_reset(struct okim9810_voice *voice)
{
	voice->adpcm_signal = voice->playbackAlgo == ADPCM2_PLAYBACK ? -2 : 0;
	voice->adpcm_step = 0;
}

static INT32 adpcm_clock(struct okim9810_voice *voice, UINT8 nibble)
{
	const int *diff = voice->playbackAlgo == ADPCM2_PLAYBACK ? adpcm2_diff_lookup : adpcm_diff_lookup;
	const int *shift = voice->playbackAlgo == ADPCM2_PLAYBACK ? adpcm2_index_shift : adpcm_index_shift;

	voice->adpcm_signal += diff[voice->adpcm_step * 16 + (nibble & 15)];

	if (voice->adpcm_signal > 2047)
		voice->adpcm_signal = 2047;
	else if (voice->adpcm_signal < -2048)
		voice->adpcm_signal = -2048;

	voice->adpcm_step += shift[nibble & 7];
	if (voice->adpcm_step > 48)
		voice->adpcm_step = 48;
	else if (voice->adpcm_step < 0)
		voice->adpcm_step = 0;

	return voice->adpcm_signal;
}

static UINT8 volume_scale(int global_volume, int channel_volume, int pan_volume)
{
	UINT32 index = (channel_volume + pan_volume) + (global_volume * 3);

	if (index > 15)
		index = 15;

	return volume_table[index];
}

/* time needed to play the current sample of a voice */
static double voice_duration(struct okim9810 *chip, struct okim9810_voice *voice)
{
	return (double)voice->count * sampling_freq_div_table[voice->samplingFreq] / chip->clock;
}

static int voice_is_playing(struct okim9810 *chip, struct okim9810_voice *voice)
{
	/* the status is derived from the emulated time and not from the samples generated
	   by the stream, see okim6295.c */
	if (voice->playing && voice->phrase_state == SEQ_STOP && !voice->looping && timer_get_time() >= voice->end_time)
		voice->playing = 0;

	return voice->playing;
}

static UINT8 read_byte(struct okim9810 *chip, UINT32 offset)
{
	return chip->region_base[offset & 0xffffff];
}

static void generate_audio(struct okim9810 *chip, struct okim9810_voice *voice, INT32 *left, INT32 *right, int samples)
{
	UINT8 volume_left, volume_right;
	UINT32 totalInterpSamples;
	int i;

	if (!voice->playing)
		return;

	volume_left = volume_scale(chip->global_volume, voice->channel_volume, voice->pan_volume_left);
	volume_right = volume_scale(chip->global_volume, voice->channel_volume, voice->pan_volume_right);

	totalInterpSamples = sampling_freq_div_table[voice->samplingFreq];
	if (totalInterpSamples == 1)
		return;

	for (i = 0; i < samples; i++)
	{
		INT32 interpValue;

		if (voice->phrase_state == SEQ_PAUSE)
		{
			voice->phrase_wait_cnt -= OKIM9810_RATE_DIV;
			if (voice->phrase_wait_cnt > 0)
				continue;
			voice->phrase_state = SEQ_ACTIVE;
		}

		if (voice->phrase_state == SEQ_ACTIVE)
		{
			if (voice->phrase_count > 0)
			{
				UINT32 startAddr, endAddr;
				UINT8 startFlags = read_byte(chip, voice->phrase_offset++);
				voice->phrase_count--;

				if (!(startFlags & 0x80))
				{
					voice->phrase_state = SEQ_PAUSE;
					voice->phrase_wait_cnt = (startFlags & 0x1f) * 16384;
					continue;
				}

				startAddr = read_byte(chip, voice->phrase_offset++) << 16;
				startAddr |= read_byte(chip, voice->phrase_offset++) << 8;
				startAddr |= read_byte(chip, voice->phrase_offset++);
				voice->channel_volume = read_byte(chip, voice->phrase_offset++) & 0xf;
				endAddr = read_byte(chip, voice->phrase_offset++) << 16;
				endAddr |= read_byte(chip, voice->phrase_offset++) << 8;
				endAddr |= read_byte(chip, voice->phrase_offset++);
				voice->phrase_count -= 7;

				voice->sample = 0;
				voice->interpPos = 0;
				voice->base_offset = startAddr;
				voice->count = endAddr - startAddr + 1;
				voice->playbackAlgo = (startFlags & 0x30) >> 4;
				voice->samplingFreq = startFlags & 0x0f;
				if (voice->playbackAlgo == ADPCM_PLAYBACK || voice->playbackAlgo == ADPCM2_PLAYBACK)
					voice->count *= 2;

				voice->phrase_state = SEQ_PLAY;
				totalInterpSamples = sampling_freq_div_table[voice->samplingFreq];
				if (totalInterpSamples == 1)
					return;
				volume_left = volume_scale(chip->global_volume, voice->channel_volume, voice->pan_volume_left);
				volume_right = volume_scale(chip->global_volume, voice->channel_volume, voice->pan_volume_right);
			}
			else
			{
				voice->phrase_state = SEQ_STOP;
				voice->playing = 0;
				break;
			}
		}

		/* at the start of a new sample gather the two points to interpolate */
		if (voice->interpPos == 0)
		{
			if (voice->playbackAlgo & EIGHTBIT_PLAYBACK)
			{
				if (voice->sample == 0)
					voice->startSample = ((INT8)read_byte(chip, voice->base_offset + voice->sample)) << 4;
				else
					voice->startSample = voice->endSample;

				voice->endSample = ((INT8)read_byte(chip, voice->base_offset + voice->sample + 1)) << 4;
			}
			else
			{
				if (voice->sample == 0)
				{
					int nibble0 = read_byte(chip, voice->base_offset + voice->sample / 2) >> (((voice->sample & 1) << 2) ^ 4);
					adpcm_reset(voice);
					voice->startSample = adpcm_clock(voice, nibble0);
				}
				else
					voice->startSample = voice->endSample;

				voice->endSample = adpcm_clock(voice, read_byte(chip, voice->base_offset + (voice->sample + 1) / 2) >> ((((voice->sample + 1) & 1) << 2) ^ 4));
			}
		}

		if (chip->filter_type == SECONDARY_FILTER || chip->filter_type == PRIMARY_FILTER)
			interpValue = voice->startSample + (INT32)(((double)(voice->endSample - voice->startSample)) * voice->interpPos / totalInterpSamples);
		else
			interpValue = voice->startSample;

		/* signal in range -2048..2047, volume in range 4..128 */
		left[i] += interpValue * volume_left / 8;
		right[i] += interpValue * volume_right / 8;

		voice->interpPos += OKIM9810_RATE_DIV;
		if (voice->interpPos >= totalInterpSamples)
		{
			voice->interpPos -= totalInterpSamples;
			voice->sample++;
		}

		if (voice->sample >= voice->count)
		{
			if (!voice->looping)
			{
				if (voice->phrase_state == SEQ_STOP)
				{
					voice->playing = 0;
					break;
				}
				voice->phrase_state = SEQ_ACTIVE;
			}
			else
				voice->sample = 0;
		}
	}
}

static void okim9810_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	struct okim9810 *chip = param;
	INT32 left[1024], right[1024];
	int done = 0;
	int i, j;

	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	memset(outputs[1], 0, samples * sizeof(*outputs[1]));

	while (done < samples)
	{
		int n = samples - done;
		if (n > 1024)
			n = 1024;

		memset(left, 0, n * sizeof(left[0]));
		memset(right, 0, n * sizeof(right[0]));

		for (i = 0; i < OKIM9810_VOICES; i++)
			generate_audio(chip, &chip->voice[i], left, right, n);

		for (j = 0; j < n; j++)
		{
			outputs[0][done + j] = left[j];
			outputs[1][done + j] = right[j];
		}

		done += n;
	}
}

static void okim9810_state_save_register(struct okim9810 *chip, int sndindex)
{
	int i;

	state_save_register_item("OKIM9810", sndindex, chip->TMP_register);
	state_save_register_item("OKIM9810", sndindex, chip->global_volume);
	state_save_register_item("OKIM9810", sndindex, chip->filter_type);
	state_save_register_item("OKIM9810", sndindex, chip->dadr);
	state_save_register_item("OKIM9810", sndindex, chip->dadr_start_offset);
	state_save_register_item("OKIM9810", sndindex, chip->dadr_end_offset);
	state_save_register_item("OKIM9810", sndindex, chip->dadr_flags);

	for (i = 0; i < OKIM9810_VOICES; i++)
	{
		struct okim9810_voice *voice = &chip->voice[i];
		int index = sndindex * OKIM9810_VOICES + i;

		state_save_register_item("OKIM9810V", index, voice->adpcm_signal);
		state_save_register_item("OKIM9810V", index, voice->adpcm_step);
		state_save_register_item("OKIM9810V", index, voice->playbackAlgo);
		state_save_register_item("OKIM9810V", index, voice->looping);
		state_save_register_item("OKIM9810V", index, voice->base_offset);
		state_save_register_item("OKIM9810V", index, voice->count);
		state_save_register_item("OKIM9810V", index, voice->samplingFreq);
		state_save_register_item("OKIM9810V", index, voice->playing);
		state_save_register_item("OKIM9810V", index, voice->sample);
		state_save_register_item("OKIM9810V", index, voice->phrase_offset);
		state_save_register_item("OKIM9810V", index, voice->phrase_count);
		state_save_register_item("OKIM9810V", index, voice->phrase_wait_cnt);
		state_save_register_item("OKIM9810V", index, voice->phrase_state);
		state_save_register_item("OKIM9810V", index, voice->channel_volume);
		state_save_register_item("OKIM9810V", index, voice->pan_volume_left);
		state_save_register_item("OKIM9810V", index, voice->pan_volume_right);
		state_save_register_item("OKIM9810V", index, voice->startSample);
		state_save_register_item("OKIM9810V", index, voice->endSample);
		state_save_register_item("OKIM9810V", index, voice->interpPos);
		state_save_register_item("OKIM9810V", index, voice->end_time);
	}
}

static void *okim9810_start(int sndindex, int clock, const void *config)
{
	const struct OKIM9810interface *intf = config;
	struct okim9810 *chip;
	int i;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	if (!tables_computed)
		compute_tables();

	chip->region_base = memory_region(intf->region);
	chip->clock = clock;
	chip->filter_type = SECONDARY_FILTER;
	chip->stream = stream_create(0, 2, clock / OKIM9810_RATE_DIV, chip, okim9810_update);

	for (i = 0; i < OKIM9810_VOICES; i++)
	{
		chip->voice[i].playbackAlgo = ADPCM2_PLAYBACK;
		chip->voice[i].samplingFreq = 2;
		chip->voice[i].phrase_state = SEQ_STOP;
	}

	okim9810_state_save_register(chip, sndindex);

	okim9810_chip = chip;

	return chip;
}

static void okim9810_reset(void *token)
{
	struct okim9810 *chip = token;
	int i;

	stream_update(chip->stream, 0);
	for (i = 0; i < OKIM9810_VOICES; i++)
		chip->voice[i].playing = 0;
}

static void write_command(struct okim9810 *chip, UINT8 data)
{
	int cmd = (data & 0xf8) >> 3;
	int channel = data & 0x07;
	int i;

	stream_update(chip->stream, 0);

	switch (cmd)
	{
		case 0x00: /* START */
			for (i = 0; i < OKIM9810_VOICES; i++)
			{
				if (chip->TMP_register & (1 << i))
				{
					struct okim9810_voice *voice = &chip->voice[i];

					voice->playing = 1;
					voice->end_time = timer_get_time() + voice_duration(chip, voice);
				}
			}
			break;

		case 0x01: /* STOP */
			for (i = 0; i < OKIM9810_VOICES; i++)
			{
				if (chip->TMP_register & (1 << i))
					chip->voice[i].playing = 0;
			}
			break;

		case 0x02: /* LOOP */
			for (i = 0; i < OKIM9810_VOICES; i++)
				chip->voice[i].looping = (chip->TMP_register & (1 << i)) != 0;
			break;

		case 0x03: /* OPT */
			chip->global_volume = (chip->TMP_register & 0x18) >> 3;
			chip->filter_type = (chip->TMP_register & 0x06) >> 1;
			break;

		case 0x04: /* MUON */
			logerror("OKIM9810: unimplemented MUON command\n");
			break;

		case 0x05: /* FADR */
		{
			struct okim9810_voice *voice = &chip->voice[channel];
			UINT32 base = chip->TMP_register * 8;
			UINT32 startAddr, endAddr;
			UINT8 startFlags = read_byte(chip, base + 0);

			startAddr = read_byte(chip, base + 1) << 16;
			startAddr |= read_byte(chip, base + 2) << 8;
			startAddr |= read_byte(chip, base + 3);

			endAddr = read_byte(chip, base + 5) << 16;
			endAddr |= read_byte(chip, base + 6) << 8;
			endAddr |= read_byte(chip, base + 7);

			if (startFlags & 0x80)
			{
				/* phrase control table */
				voice->phrase_offset = startAddr;
				voice->phrase_count = endAddr - startAddr + 1;
				voice->phrase_state = SEQ_ACTIVE;
			}
			else
			{
				voice->phrase_state = SEQ_STOP;
				voice->sample = 0;
				voice->interpPos = 0;
				voice->base_offset = startAddr;
				voice->count = endAddr - startAddr + 1;
				voice->playbackAlgo = (startFlags & 0x30) >> 4;
				voice->samplingFreq = startFlags & 0x0f;
				if (voice->playbackAlgo == ADPCM_PLAYBACK || voice->playbackAlgo == ADPCM2_PLAYBACK)
					voice->count *= 2;
			}
			break;
		}

		case 0x06: /* DADR */
			if ((channel & 4) == 0)
			{
				struct okim9810_voice *voice = &chip->voice[channel];
				UINT8 startFlags = chip->dadr_flags;

				voice->phrase_state = SEQ_STOP;
				voice->sample = 0;
				voice->interpPos = 0;
				voice->base_offset = chip->dadr_start_offset;
				voice->count = (chip->dadr_end_offset - chip->dadr_start_offset) + 1;
				voice->playbackAlgo = (startFlags & 0x0c) >> 2;
				voice->samplingFreq = (startFlags & 0xf0) >> 4;
				if (voice->playbackAlgo == ADPCM_PLAYBACK || voice->playbackAlgo == ADPCM2_PLAYBACK)
					voice->count *= 2;
			}
			break;

		case 0x07: /* CVOL */
			chip->voice[channel].channel_volume = chip->TMP_register & 0x0f;
			break;

		case 0x08: /* PAN */
			chip->voice[channel].pan_volume_left = (chip->TMP_register & 0xf0) >> 4;
			chip->voice[channel].pan_volume_right = chip->TMP_register & 0x0f;
			break;

		default:
			logerror("OKIM9810: unknown command %02x\n", data);
			break;
	}

	chip->dadr = 0;
}

static void write_tmp_register(struct okim9810 *chip, UINT8 data)
{
	chip->TMP_register = data;

	switch (chip->dadr)
	{
		case 0: chip->dadr_start_offset = data << 16; break;
		case 1: chip->dadr_start_offset |= data << 8; break;
		case 2: chip->dadr_start_offset |= data; break;
		case 3: chip->dadr_end_offset = data << 16; break;
		case 4: chip->dadr_end_offset |= data << 8; break;
		case 5: chip->dadr_end_offset |= data; break;
		case 6: chip->dadr_flags = data; break;
	}

	if (chip->dadr < 7)
		++chip->dadr;
}

READ8_HANDLER( OKIM9810_status_0_r )
{
	struct okim9810 *chip = okim9810_chip;
	UINT8 result = 0;
	int i;

	stream_update(chip->stream, 0);

	for (i = 0; i < OKIM9810_VOICES; i++)
	{
		if (!voice_is_playing(chip, &chip->voice[i]))
			result |= 1 << i;
	}

	return result;
}

WRITE8_HANDLER( OKIM9810_data_0_w )
{
	write_command(okim9810_chip, data);
}

WRITE8_HANDLER( OKIM9810_tmp_0_w )
{
	write_tmp_register(okim9810_chip, data);
}

static void okim9810_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}

void okim9810_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		case SNDINFO_PTR_SET_INFO:						info->set_info = okim9810_set_info;		break;
		case SNDINFO_PTR_START:							info->start = okim9810_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = okim9810_reset;			break;

		case SNDINFO_STR_NAME:							info->s = "OKI9810";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "OKI ADPCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright Andrew Gardner";	break;
	}
}

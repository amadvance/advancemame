// license:BSD-3-Clause
// copyright-holders:R. Belmont, superctr
/*
    c352.c - Namco C352 custom PCM chip emulation
    v2.0
    By R. Belmont
    Rewritten and improved by superctr
    Additional code by cync and the hoot development team

    Thanks to Cap of VivaNonno for info and The_Author for preliminary reverse-engineering

    Chip specs:
    32 voices
    Supports 8-bit linear and 8-bit muLaw samples
    Output: digital, 16 bit, 4 channels
    Output sample rate is the input clock / (288 * 2).
 */

#include "driver.h"
#include "sndintrf.h"
#include "streams.h"
#include "c352.h"
#include <stddef.h>

#define VERBOSE (0)

// flags

#define C352_FLG_BUSY       0x8000   // channel is busy
#define C352_FLG_KEYON      0x4000   // Keyon
#define C352_FLG_KEYOFF     0x2000   // Keyoff
#define C352_FLG_LOOPTRG    0x1000   // Loop Trigger
#define C352_FLG_LOOPHIST   0x0800   // Loop History
#define C352_FLG_FM         0x0400   // Frequency Modulation
#define C352_FLG_PHASERL    0x0200   // Rear Left invert phase 180 degrees
#define C352_FLG_PHASEFL    0x0100   // Front Left invert phase 180 degrees
#define C352_FLG_PHASEFR    0x0080   // invert phase 180 degrees (e.g. flip sign of sample)
#define C352_FLG_LDIR       0x0040   // loop direction
#define C352_FLG_LINK       0x0020   // "long-format" sample
#define C352_FLG_NOISE      0x0010   // play noise instead of sample
#define C352_FLG_MULAW      0x0008   // sample is mulaw instead of linear 8-bit PCM
#define C352_FLG_FILTER     0x0004   // don't apply filter
#define C352_FLG_REVLOOP    0x0003   // loop backwards
#define C352_FLG_LOOP       0x0002   // loop forward
#define C352_FLG_REVERSE    0x0001   // play sample backwards

struct c352_voice_t
{
	UINT32 pos;
	UINT32 counter;

	INT16 sample;
	INT16 last_sample;

	UINT16 vol_f;
	UINT16 vol_r;
	UINT8 curr_vol[4];

	UINT16 freq;
	UINT16 flags;

	UINT16 wave_bank;
	UINT16 wave_start;
	UINT16 wave_end;
	UINT16 wave_loop;
};

struct c352_info
{
	sound_stream *stream;
	struct c352_voice_t c352_v[32];
	unsigned char *c352_rom_samples;
	UINT32 c352_rom_length;
	int c352_region;
	int sample_rate_base;
	int divider;

	INT16 mulawtab[256];

	UINT16 random;
	UINT16 control;
};

static void c352_fetch_sample(struct c352_info *info, struct c352_voice_t *v)
{
	v->last_sample = v->sample;

	if (v->flags & C352_FLG_NOISE)
	{
		info->random = (info->random >> 1) ^ ((-(info->random & 1)) & 0xfff6);
		v->sample = info->random;
	}
	else
	{
		INT8 s = 0;
		if (info->c352_rom_length > 0)
			s = (INT8)info->c352_rom_samples[v->pos % info->c352_rom_length];

		if (v->flags & C352_FLG_MULAW)
			v->sample = info->mulawtab[(UINT8)s];
		else
			v->sample = s << 8;

		UINT16 pos = v->pos & 0xffff;

		if ((v->flags & C352_FLG_LOOP) && (v->flags & C352_FLG_REVERSE))
		{
			// backwards>forwards
			if ((v->flags & C352_FLG_LDIR) && pos == v->wave_loop)
				v->flags &= ~C352_FLG_LDIR;
			// forwards>backwards
			else if (!(v->flags & C352_FLG_LDIR) && pos == v->wave_end)
				v->flags |= C352_FLG_LDIR;

			v->pos += (v->flags & C352_FLG_LDIR) ? -1 : 1;
		}
		else if (pos == v->wave_end)
		{
			if ((v->flags & C352_FLG_LINK) && (v->flags & C352_FLG_LOOP))
			{
				v->pos = (v->wave_start << 16) | v->wave_loop;
				v->flags |= C352_FLG_LOOPHIST;
			}
			else if (v->flags & C352_FLG_LOOP)
			{
				v->pos = (v->pos & 0xff0000) | v->wave_loop;
				v->flags |= C352_FLG_LOOPHIST;
			}
			else
			{
				v->flags |= C352_FLG_KEYOFF;
				v->flags &= ~C352_FLG_BUSY;
				v->sample = 0;
			}
		}
		else
		{
			v->pos += (v->flags & C352_FLG_REVERSE) ? -1 : 1;
		}
	}
}

static inline void c352_ramp_volume(struct c352_voice_t *v, int ch, UINT8 val)
{
	INT16 vol_delta = v->curr_vol[ch] - val;
	if (vol_delta != 0)
		v->curr_vol[ch] += (vol_delta > 0) ? -1 : 1;
}

static void c352_update(void *param, stream_sample_t **inputs, stream_sample_t **buf, int sample_count)
{
	struct c352_info *info = param;
	int i, j;
	stream_sample_t *bufferl = buf[0];
	stream_sample_t *bufferr = buf[1];
	stream_sample_t *bufferl2 = buf[2];
	stream_sample_t *bufferr2 = buf[3];

	for (i = 0; i < sample_count; i++)
	{
		int out[4] = { 0, 0, 0, 0 };

		for (j = 0; j < 32; j++)
		{
			struct c352_voice_t *v = &info->c352_v[j];
			INT16 s = 0;

			if (v->flags & C352_FLG_BUSY)
			{
				INT32 next_counter = v->counter + v->freq;

				if (next_counter & 0x10000)
				{
					c352_fetch_sample(info, v);
				}

				if ((next_counter ^ v->counter) & 0x18000)
				{
					c352_ramp_volume(v, 0, v->vol_f >> 8);
					c352_ramp_volume(v, 1, v->vol_f & 0xff);
					c352_ramp_volume(v, 2, v->vol_r >> 8);
					c352_ramp_volume(v, 3, v->vol_r & 0xff);
				}

				v->counter = next_counter & 0xffff;

				s = v->sample;

				// Interpolate samples
				if ((v->flags & C352_FLG_FILTER) == 0)
					s = v->last_sample + (v->counter * (v->sample - v->last_sample) >> 16);
			}

			// Left
			out[0] += (((v->flags & C352_FLG_PHASEFL) ? -s : s) * v->curr_vol[0]) >> 8;
			out[2] += (((v->flags & C352_FLG_PHASERL) ? -s : s) * v->curr_vol[2]) >> 8;

			// Right
			out[1] += (((v->flags & C352_FLG_PHASEFR) ? -s : s) * v->curr_vol[1]) >> 8;
			out[3] += (((v->flags & C352_FLG_PHASEFR) ? -s : s) * v->curr_vol[3]) >> 8;
		}

		*bufferl++ = (stream_sample_t)(out[0] >> 3);
		*bufferr++ = (stream_sample_t)(out[1] >> 3);
		*bufferl2++ = (stream_sample_t)(out[2] >> 3);
		*bufferr2++ = (stream_sample_t)(out[3] >> 3);
	}
}

static UINT16 c352_read_reg16(struct c352_info *info, UINT32 address)
{
	UINT32 offset = address / 2;
	const int reg_map[8] =
	{
		offsetof(struct c352_voice_t, vol_f) / sizeof(UINT16),
		offsetof(struct c352_voice_t, vol_r) / sizeof(UINT16),
		offsetof(struct c352_voice_t, freq) / sizeof(UINT16),
		offsetof(struct c352_voice_t, flags) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_bank) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_start) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_end) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_loop) / sizeof(UINT16),
	};

	stream_update(info->stream, 0);

	if (offset < 0x100)
		return *((UINT16 *)&info->c352_v[offset / 8] + reg_map[offset % 8]);
	else if (offset == 0x200)
		return info->control;
	else
		return 0;
}

static void c352_write_reg16(struct c352_info *info, UINT32 address, UINT16 val, UINT16 mem_mask)
{
	UINT32 offset = address / 2;
	int i;
	const int reg_map[8] =
	{
		offsetof(struct c352_voice_t, vol_f) / sizeof(UINT16),
		offsetof(struct c352_voice_t, vol_r) / sizeof(UINT16),
		offsetof(struct c352_voice_t, freq) / sizeof(UINT16),
		offsetof(struct c352_voice_t, flags) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_bank) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_start) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_end) / sizeof(UINT16),
		offsetof(struct c352_voice_t, wave_loop) / sizeof(UINT16),
	};

	stream_update(info->stream, 0);

	if (offset < 0x100)
	{
		UINT16 *reg = (UINT16 *)&info->c352_v[offset / 8] + reg_map[offset % 8];
		*reg = (*reg & mem_mask) | (val & ~mem_mask);
	}
	else if (offset == 0x200)
	{
		info->control = (info->control & mem_mask) | (val & ~mem_mask);
	}
	else if (offset == 0x202) // execute keyons/keyoffs
	{
		if (mem_mask != 0)
			return;

		for (i = 0; i < 32; i++)
		{
			if (info->c352_v[i].flags & C352_FLG_KEYON)
			{
				info->c352_v[i].pos = (info->c352_v[i].wave_bank << 16) | info->c352_v[i].wave_start;

				info->c352_v[i].sample = 0;
				info->c352_v[i].last_sample = 0;
				info->c352_v[i].counter = 0xffff;

				info->c352_v[i].flags |= C352_FLG_BUSY;
				info->c352_v[i].flags &= ~(C352_FLG_KEYON | C352_FLG_LOOPHIST);

				info->c352_v[i].curr_vol[0] = info->c352_v[i].curr_vol[1] = 0;
				info->c352_v[i].curr_vol[2] = info->c352_v[i].curr_vol[3] = 0;
			}
			if (info->c352_v[i].flags & C352_FLG_KEYOFF)
			{
				info->c352_v[i].flags &= ~(C352_FLG_BUSY | C352_FLG_KEYOFF);
				info->c352_v[i].counter = 0xffff;
			}
		}
	}
}

static void *c352_start(int sndindex, int clock, const void *config)
{
	struct c352_info *info;
	const struct C352interface *intf;
	int i, j;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	intf = config;

	info->c352_rom_samples = memory_region(intf->region);
	info->c352_rom_length = memory_region_length(intf->region);
	info->c352_region = intf->region;

	info->divider = 288;
	if (clock == 25401600 / 288 * 192)
		info->divider = 192;

	info->sample_rate_base = clock / info->divider;

	info->stream = stream_create(0, 4, info->sample_rate_base, info, c352_update);

	// generate mulaw table (Output similar to namco's VC emulator)
	j = 0;
	for (i = 0; i < 128; i++)
	{
		info->mulawtab[i] = j << 5;
		if (i < 16)
			j += 1;
		else if (i < 24)
			j += 2;
		else if (i < 48)
			j += 4;
		else if (i < 100)
			j += 8;
		else
			j += 16;
	}
	for (i = 0; i < 128; i++)
		info->mulawtab[i + 128] = (~info->mulawtab[i]) & 0xffe0;

	info->random = 0x1234;
	info->control = 0;

	// register save state info
	for (i = 0; i < 32; i++)
	{
		char cname[32];
		sprintf(cname, "C352 v %02d", i);

		state_save_register_item(cname, sndindex, info->c352_v[i].pos);
		state_save_register_item(cname, sndindex, info->c352_v[i].counter);
		state_save_register_item(cname, sndindex, info->c352_v[i].sample);
		state_save_register_item(cname, sndindex, info->c352_v[i].last_sample);
		state_save_register_item(cname, sndindex, info->c352_v[i].vol_f);
		state_save_register_item(cname, sndindex, info->c352_v[i].vol_r);
		state_save_register_item_array(cname, sndindex, info->c352_v[i].curr_vol);
		state_save_register_item(cname, sndindex, info->c352_v[i].freq);
		state_save_register_item(cname, sndindex, info->c352_v[i].flags);
		state_save_register_item(cname, sndindex, info->c352_v[i].wave_bank);
		state_save_register_item(cname, sndindex, info->c352_v[i].wave_start);
		state_save_register_item(cname, sndindex, info->c352_v[i].wave_end);
		state_save_register_item(cname, sndindex, info->c352_v[i].wave_loop);
	}
	state_save_register_item("C352", sndindex, info->random);
	state_save_register_item("C352", sndindex, info->control);

	return info;
}

static void c352_reset(void *token)
{
	struct c352_info *info = token;
	memset(info->c352_v, 0, sizeof(info->c352_v));
	info->random = 0x1234;
	info->control = 0;
}

READ16_HANDLER( c352_0_r )
{
	return c352_read_reg16(sndti_token(SOUND_C352, 0), offset * 2);
}

WRITE16_HANDLER( c352_0_w )
{
	c352_write_reg16(sndti_token(SOUND_C352, 0), offset * 2, data, mem_mask);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void c352_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}

void c352_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = c352_set_info;			break;
		case SNDINFO_PTR_START:							info->start = c352_start;				break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = c352_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "C352";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Namco PCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "2.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004-2016, The MAME Team"; break;
	}
}

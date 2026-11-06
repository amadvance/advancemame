// license:BSD-3-Clause
// copyright-holders:Curt Coder
/* RCA CDP1869/70/76 Video Interface System */

#include "sndintrf.h"
#include "sound/cdp1869.h"
#include "streams.h"

struct CDP1869
{
	sound_stream *stream;
	int clock;

	int incr;
	INT16 signal;

	int toneoff;
	int wnoff;

	UINT8 tonediv;
	UINT8 tonefreq;
	UINT8 toneamp;

	UINT8 wnfreq;
	UINT8 wnamp;
};

static void cdp1869_update(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	struct CDP1869 *info = (struct CDP1869 *) param;
	INT16 signal = info->signal;
	stream_sample_t *buffer = _buffer[0];

	memset( buffer, 0, length * sizeof(*buffer) );

	/* The noise generator remains unimplemented, hence GAME_IMPERFECT_SOUND. */
	if (!info->toneoff && info->toneamp)
	{
		double frequency = (info->clock / 2) / (512 >> info->tonefreq) / (info->tonediv + 1);

		int rate = Machine->sample_rate / 2;

		int incr = info->incr;

		if (signal < 0)
		{
			signal = -(info->toneamp * (0x07fff / 15));
		}
		else
		{
			signal = info->toneamp * (0x07fff / 15);
		}

		while( length-- > 0 )
		{
			*buffer++ = signal;
			incr -= frequency;
			while( incr < 0 )
			{
				incr += rate;
				signal = -signal;
			}
		}

		info->incr = incr;
		info->signal = signal;
	}

}

static void *cdp1869_start(int sndindex, int clock, const void *config)
{
	struct CDP1869 *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->stream = stream_create(0, 1, Machine->sample_rate, info, cdp1869_update );
	info->incr = 0;
	info->signal = 0x07fff;

	info->clock = clock;
	info->toneoff = 1;
	info->wnoff = 1;
	info->tonediv = 0;
	info->tonefreq = 0;
	info->toneamp = 0;
	info->wnfreq = 0;
	info->wnamp = 0;

	return info;
}

void cdp1869_set_toneamp(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	stream_update(info->stream, 0);
	info->toneamp = value & 0x0f;
}

void cdp1869_set_tonefreq(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	stream_update(info->stream, 0);
	info->tonefreq = value & 0x07;
}

void cdp1869_set_toneoff(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	stream_update(info->stream, 0);
	info->toneoff = value & 0x01;
}

void cdp1869_set_tonediv(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	stream_update(info->stream, 0);
	info->tonediv = value & 0x7f;
}

void cdp1869_set_wnamp(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	stream_update(info->stream, 0);
	info->wnamp = value & 0x0f;
}

void cdp1869_set_wnfreq(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	stream_update(info->stream, 0);
	info->wnfreq = value & 0x07;
}

void cdp1869_set_wnoff(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	stream_update(info->stream, 0);
	info->wnoff = value & 0x01;
}

static void cdp1869_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
	}
}

void cdp1869_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		case SNDINFO_PTR_SET_INFO:						info->set_info = cdp1869_set_info;		break;
		case SNDINFO_PTR_START:							info->start = cdp1869_start;			break;
		case SNDINFO_PTR_STOP:														break;
		case SNDINFO_PTR_RESET:														break;

		case SNDINFO_STR_NAME:							info->s = "CDP1869";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "RCA CDP1869";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2007, The MAME Team"; break;
	}
}

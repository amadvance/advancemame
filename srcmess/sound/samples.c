#include "driver.h"
#include "streams.h"
#include "samples.h"


struct sample_channel
{
	sound_stream *stream;
	INT16 *		source;
	INT32		source_length;
	INT32		source_num;
	UINT32		pos;
	UINT32		frac;
	UINT32		step;
	UINT8		loop;
	UINT8		paused;
};

struct samples_info
{
	int			numchannels;	/* how many channels */
	struct sample_channel *channel;/* array of channels */
	struct loaded_samples *samples;/* array of samples */
};



#define FRAC_BITS		24
#define FRAC_ONE		(1 << FRAC_BITS)
#define FRAC_MASK		(FRAC_ONE - 1)


/*-------------------------------------------------
    read_wav_sample - read a WAV file as a sample
-------------------------------------------------*/

#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif

static int read_wav_sample(mame_file *f, struct loaded_sample *sample)
{
	unsigned long offset = 0;
	UINT32 length, rate, filesize;
	UINT16 bits, temp16;
	char buf[32];

	/* read the core header and make sure it's a WAVE file */
	offset += mame_fread(f, buf, 4);
	if (offset < 4)
		return 0;
	if (memcmp(&buf[0], "RIFF", 4) != 0)
		return 0;

	/* get the total size */
	offset += mame_fread(f, &filesize, 4);
	if (offset < 8)
		return 0;
	filesize = intelLong(filesize);

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += mame_fread(f, buf, 4);
	if (offset < 12)
		return 0;
	if (memcmp(&buf[0], "WAVE", 4) != 0)
		return 0;

	/* seek until we find a format tag */
	while (1)
	{
		offset += mame_fread(f, buf, 4);
		offset += mame_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "fmt ", 4) == 0)
			break;

		/* seek to the next block */
		mame_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return 0;
	}

	/* read the format -- make sure it is PCM */
	offset += mame_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return 0;

	/* number of channels -- only mono is supported */
	offset += mame_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return 0;

	/* sample rate */
	offset += mame_fread(f, &rate, 4);
	rate = intelLong(rate);

	/* bytes/second and block alignment are ignored */
	offset += mame_fread(f, buf, 6);

	/* bits/sample */
	offset += mame_fread_lsbfirst(f, &bits, 2);
	if (bits != 8 && bits != 16)
		return 0;

	/* seek past any extra data */
	mame_fseek(f, length - 16, SEEK_CUR);
	offset += length - 16;

	/* seek until we find a data tag */
	while (1)
	{
		offset += mame_fread(f, buf, 4);
		offset += mame_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "data", 4) == 0)
			break;

		/* seek to the next block */
		mame_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return 0;
	}

	/* if there was a 0 length data block, we're done */
	if (length == 0)
		return 0;

	/* fill in the sample data */
	sample->length = length;
	sample->frequency = rate;

	/* read the data in */
	if (bits == 8)
	{
		unsigned char *tempptr;
		int sindex;

		sample->data = auto_malloc(sizeof(*sample->data) * length);
		mame_fread(f, sample->data, length);

		/* convert 8-bit data to signed samples */
		tempptr = (unsigned char *)sample->data;
		for (sindex = length - 1; sindex >= 0; sindex--)
			sample->data[sindex] = (INT8)(tempptr[sindex] ^ 0x80) * 256;
	}
	else
	{
		/* 16-bit data is fine as-is */
		sample->data = auto_malloc(sizeof(*sample->data) * (length/2));
		mame_fread_lsbfirst(f, sample->data, length);
		sample->length /= 2;
	}
	return 1;
}


/*-------------------------------------------------
    readsamples - load all samples
-------------------------------------------------*/

struct loaded_samples *readsamples(const char **samplenames, const char *basename)
{
	struct loaded_samples *samples;
	int skipfirst = 0;
	int i;

	/* if the user doesn't want to use samples, bail */
	if (!options.use_samples)
		return NULL;
	if (samplenames == 0 || samplenames[0] == 0)
		return NULL;

	/* if a name begins with '*', we will also look under that as an alternate basename */
	if (samplenames[0][0] == '*')
		skipfirst = 1;

	/* count the samples */
	for (i = 0; samplenames[i+skipfirst] != 0; i++) ;
	if (i == 0)
		return NULL;

	/* allocate the array */
	samples = auto_malloc(sizeof(struct loaded_samples) + (i-1) * sizeof(struct loaded_sample));
	memset(samples, 0, sizeof(struct loaded_samples) + (i-1) * sizeof(struct loaded_sample));
	samples->total = i;

	/* load the samples */
	for (i = 0; i < samples->total; i++)
	{
		mame_file *f;

		if (samplenames[i+skipfirst][0])
		{
			f = mame_fopen(basename, samplenames[i+skipfirst], FILETYPE_SAMPLE, 0);
			if (f == NULL && skipfirst)
				f = mame_fopen(samplenames[0] + 1, samplenames[i+skipfirst], FILETYPE_SAMPLE, 0);
			if (f != NULL)
			{
				read_wav_sample(f, &samples->sample[i]);
				mame_fclose(f);
			}
		}
	}

	return samples;
}




/* Start one of the samples loaded from disk. Note: channel must be in the range */
/* 0 .. Samplesinterface->channels-1. It is NOT the discrete channel to pass to */
/* mixer_play_sample() */
void sample_start(int channel,int samplenum,int loop)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);
	struct sample_channel *chan = &info->channel[channel];
	struct loaded_sample *sample;

	if (info->samples == NULL)
		return;
	if (channel >= info->numchannels)
	{
		logerror("error: sample_start() called with channel = %d, but only %d channels allocated\n",channel,info->numchannels);
		return;
	}
	if (samplenum >= info->samples->total)
	{
		logerror("error: sample_start() called with samplenum = %d, but only %d samples available\n",samplenum,info->samples->total);
		return;
	}

	/* force an update before we start */
	stream_update(chan->stream, 0);

	/* update the parameters */
	sample = &info->samples->sample[samplenum];
	chan->source = sample->data;
	chan->source_length = sample->length;
	chan->source_num = sample->data ? samplenum : -1;
	chan->pos = 0;
	chan->frac = 0;
	chan->step = ((INT64)sample->frequency << FRAC_BITS) / Machine->sample_rate;
	chan->loop = loop;
}

void sample_start_raw(int channel,INT16 *sampledata,int samples,int frequency,int loop)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);
	struct sample_channel *chan = &info->channel[channel];

	if (channel >= info->numchannels)
	{
		logerror("error: sample_start() called with channel = %d, but only %d channels allocated\n",channel,info->numchannels);
		return;
	}

	/* force an update before we start */
	stream_update(chan->stream, 0);

	/* update the parameters */
	chan->source = sampledata;
	chan->source_length = samples;
	chan->source_num = -1;
	chan->pos = 0;
	chan->frac = 0;
	chan->step = ((INT64)frequency << FRAC_BITS) / Machine->sample_rate;
	chan->loop = loop;
}

void sample_set_freq(int channel,int freq)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);
	struct sample_channel *chan = &info->channel[channel];

	if (channel >= info->numchannels)
	{
		logerror("error: sample_set_freq() called with channel = %d, but only %d channels allocated\n",channel,info->numchannels);
		return;
	}

	/* force an update before we start */
	stream_update(chan->stream, 0);

	chan->step = ((INT64)freq << FRAC_BITS) / Machine->sample_rate;
}

void sample_set_volume(int channel,float volume)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);
	struct sample_channel *chan = &info->channel[channel];

	if (channel >= info->numchannels)
	{
		logerror("error: sample_set_volume() called with channel = %d, but only %d channels allocated\n",channel,info->numchannels);
		return;
	}

	stream_set_output_gain(chan->stream, 0, volume);
}

void sample_set_pause(int channel,int pause)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);
	struct sample_channel *chan = &info->channel[channel];

	if (channel >= info->numchannels)
	{
		logerror("error: sample_set_pause() called with channel = %d, but only %d channels allocated\n",channel,info->numchannels);
		return;
	}

	/* force an update before we start */
	stream_update(chan->stream, 0);

	chan->paused = pause;
}

void sample_stop(int channel)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);
	struct sample_channel *chan = &info->channel[channel];

	if (channel >= info->numchannels)
	{
		logerror("error: sample_stop() called with channel = %d, but only %d channels allocated\n",channel,info->numchannels);
		return;
	}

	/* force an update before we start */
	stream_update(chan->stream, 0);
	chan->source = NULL;
	chan->source_num = -1;
}

int sample_playing(int channel)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);
	struct sample_channel *chan = &info->channel[channel];

	if (channel >= info->numchannels)
	{
		logerror("error: sample_playing() called with channel = %d, but only %d channels allocated\n",channel,info->numchannels);
		return 0;
	}

	/* force an update before we start */
	stream_update(chan->stream, 0);
	return (chan->source != NULL);
}


int sample_loaded(int samplenum)
{
	struct samples_info *info = sndti_token(SOUND_SAMPLES, 0);

	if (info->samples == NULL)
		return 0;
	if (samplenum >= info->samples->total)
	{
		logerror("error: sample_loaded() called with samplenum = %d, but only %d samples available\n",samplenum,info->samples->total);
		return 0;
	}
	return (info->samples->sample[samplenum].data != NULL);
}



static void sample_update_sound(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	struct sample_channel *chan = param;
	stream_sample_t *buffer = _buffer[0];

	if (chan->source && !chan->paused)
	{
		/* load some info locally */
		UINT32 pos = chan->pos;
		UINT32 frac = chan->frac;
		UINT32 step = chan->step;
		INT16 *sample = chan->source;
		UINT32 sample_length = chan->source_length;

		while (length--)
		{
			/* do a linear interp on the sample */
			INT32 sample1 = sample[pos];
			INT32 sample2 = sample[(pos + 1) % sample_length];
			INT32 fracmult = frac >> (FRAC_BITS - 14);
			*buffer++ = ((0x4000 - fracmult) * sample1 + fracmult * sample2) >> 14;

			/* advance */
			frac += step;
			pos += frac >> FRAC_BITS;
			frac = frac & ((1 << FRAC_BITS) - 1);

			/* handle looping/ending */
			if (pos >= sample_length)
			{
				if (chan->loop)
					pos %= sample_length;
				else
				{
					chan->source = NULL;
					chan->source_num = -1;
					if (length > 0)
						memset(buffer, 0, length * sizeof(*buffer));
					break;
				}
			}
		}

		/* push position back out */
		chan->pos = pos;
		chan->frac = frac;
	}
	else
		memset(buffer, 0, length * sizeof(*buffer));
}


static void samples_postload(void *param)
{
	struct samples_info *info = param;
	int i;

	/* loop over channels */
	for (i = 0; i < info->numchannels; i++)
	{
		struct sample_channel *chan = &info->channel[i];

		/* attach any samples that were loaded and playing */
		if (chan->source_num >= 0 && chan->source_num < info->samples->total)
		{
			struct loaded_sample *sample = &info->samples->sample[chan->source_num];
			chan->source = sample->data;
			chan->source_length = sample->length;
			if (!sample->data)
				chan->source_num = -1;
		}

		/* validate the position against the length in case the sample is smaller */
		if (chan->source && chan->pos >= chan->source_length)
		{
			if (chan->loop)
				chan->pos %= chan->source_length;
			else
			{
				chan->source = NULL;
				chan->source_num = -1;
			}
		}
	}
}


static void *samples_start(int sndindex, int clock, const void *config)
{
	int i;
	const struct Samplesinterface *intf = config;
	struct samples_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));
	sndintrf_register_token(info);

	/* read audio samples */
	if (intf->samplenames)
		info->samples = readsamples(intf->samplenames,Machine->gamedrv->name);

	/* allocate channels */
	info->numchannels = intf->channels;
	info->channel = auto_malloc(sizeof(*info->channel) * info->numchannels);
	for (i = 0; i < info->numchannels; i++)
	{
	    info->channel[i].stream = stream_create(0, 1, Machine->sample_rate, &info->channel[i], sample_update_sound);

		info->channel[i].source = NULL;
		info->channel[i].source_num = -1;
		info->channel[i].step = 0;
		info->channel[i].loop = 0;
		info->channel[i].paused = 0;

		/* register with the save state system */
		state_save_register_item("samples", i, info->channel[i].source_length);
		state_save_register_item("samples", i, info->channel[i].source_num);
		state_save_register_item("samples", i, info->channel[i].pos);
		state_save_register_item("samples", i, info->channel[i].frac);
		state_save_register_item("samples", i, info->channel[i].step);
		state_save_register_item("samples", i, info->channel[i].loop);
		state_save_register_item("samples", i, info->channel[i].paused);
	}
	state_save_register_func_postload_ptr(samples_postload, info);

	/* initialize any custom handlers */
	if (intf->start)
		(*intf->start)();

	return info;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void samples_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void samples_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = samples_set_info;		break;
		case SNDINFO_PTR_START:							info->start = samples_start;			break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "Samples";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Big Hack";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}


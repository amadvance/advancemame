/*
 * This file is part of the AdvanceMAME project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "mixer.h"
#include "mpglib.h"
#include "sounddrv.h"
#include "wave.h"
#include "os.h"
#include "soundall.h"

#include <string.h>

#define MIXER_BUFFER_MAX (44100*3) /**< Max samples in the buffer */
#define MIXER_PAGE_SIZE 4096 /**< Size of the buffer read from disk */

enum mixer_type {
	mixer_none,
	mixer_raw_memory,
	mixer_raw_file,
	mixer_mp3_file
};

struct mixer_channel_struct {
	enum mixer_type type;

	unsigned rate; /**< Sample rate */
	unsigned nchannel; /**< Number of channel */
	unsigned bit; /**< Number of bits */
	int loop; /** Loop */

	FZ* file; /**< File buffer */
	const char* data; /**< Memory buffer */

	unsigned start; /**< Start position in the file buffer */
	unsigned end; /**< End position in the file buffer */
	unsigned pos; /**< Current position in the file buffer */
	/**
	 * Set when the input stream is empty.
	 * Note that pos==end doesn't necessary imply empty. For example the
	 * MP3 stream may have some cached data not already used.
	 */
	int empty;

	unsigned count; /**< Current samples in the mixer buffer */
	unsigned silence_count; /**< Current silence samples in the mixer buffer */

	int pivot; /**< Pivot for resampling */
	int up; /**< Resampling pivot increment step */
	int down; /**< Resampling pivot decrement step */

	struct mp3_mpstr mp3; /**< MP3 state */
};

struct mixer_channel_struct mixer_map[MIXER_CHANNEL_MAX];

static short mixer_raw_buffer[MIXER_BUFFER_MAX * 2]; /**< Buffer used to call sound_play() (*2 for stereo) */
static int mixer_buffer[MIXER_BUFFER_MAX * 2]; /**< Buffer for the mixed samples (*2 for stereo) */
static unsigned mixer_buffer_pos; /**< Position to play in the buffer in samples */
static unsigned mixer_latency_size; /**< Required latency in samples */
static unsigned mixer_buffer_size; /**< Required buffer in samples */
static unsigned mixer_rate; /**< Current sample rate */
static unsigned mixer_nchannel; /**< Number of active channels */
static int mixer_ndivider; /**< Divider of the channel value */

/****************************************************************************/
/* Mixing */

/** Check if the channel input is empty */
static int mixer_channel_input_is_empty(unsigned channel) {
	return mixer_map[channel].type == mixer_none
		|| mixer_map[channel].empty != 0;
}

/** Check if the channel output is empty */
static int mixer_channel_output_is_empty(unsigned channel) {
	return mixer_map[channel].type == mixer_none
		|| (mixer_map[channel].count == 0 && mixer_map[channel].silence_count > mixer_latency_size);
}

/** Check if the channel data is empty */
static int mixer_channel_data_is_empty(unsigned channel) {
	return mixer_map[channel].type == mixer_none
		|| (mixer_map[channel].count == 0);
}

static int mixer_channel_is_active(unsigned channel) {
	return mixer_map[channel].type != mixer_none
		&& (!mixer_channel_input_is_empty(channel) || !mixer_channel_output_is_empty(channel));
}

static void mixer_pump(unsigned buffered) {
	int count;
	unsigned i;

	count = mixer_latency_size - buffered;
	if (count < 0)
		count = 0;

	for(i=0;i<mixer_nchannel;++i) {
		if (mixer_channel_is_active(i)
			&& mixer_map[i].count
			&& count > mixer_map[i].count) {
			count = mixer_map[i].count;
		}
	}

	for(i=0;i<mixer_nchannel;++i) {
		if (mixer_channel_is_active(i)) {
			if (mixer_map[i].count) {
				assert(mixer_map[i].count >= count );
				mixer_map[i].count -= count;
			} else {
				mixer_map[i].silence_count += count;
			}
		}
	}

	if (!count)
		sound_play(0,0);

	while (count) {
		unsigned run = count;
		if (mixer_buffer_pos + run > MIXER_BUFFER_MAX)
			run = MIXER_BUFFER_MAX - mixer_buffer_pos;

		for(i=0;i<run;++i) {
			int c0 = mixer_buffer[(mixer_buffer_pos+i)*2];
			int c1 = mixer_buffer[(mixer_buffer_pos+i)*2+1];

			mixer_buffer[(mixer_buffer_pos+i)*2] = 0;
			mixer_buffer[(mixer_buffer_pos+i)*2+1] = 0;

			c0 /= mixer_ndivider; /* divider must be a signed int */
			c1 /= mixer_ndivider;

			if (c0 > 32767)
				c0 = 32767;
			if (c0 < -32768)
				c0 = -32768;
			if (c1 > 32767)
				c1 = 32767;
			if (c1 < -32768)
				c1 = -32768;

			mixer_raw_buffer[i*2] = (short)c0;
			mixer_raw_buffer[i*2+1] = (short)c1;
		}

		sound_play(mixer_raw_buffer, run);

		mixer_buffer_pos += run;
		if (mixer_buffer_pos == MIXER_BUFFER_MAX)
			mixer_buffer_pos = 0;
		count -= run;
	}
}

/**
 * Return the number of samples to insert.
 * The samples are relative at the channel sample rate.
 */
static void mixer_channel_need(unsigned channel, unsigned* min, unsigned* max) {
	int c;

	c = (int)mixer_buffer_size - mixer_map[channel].count;
	if (c < 0)
		*min = 0;
	else
		*min = c * mixer_map[channel].rate / mixer_rate;

	c = MIXER_BUFFER_MAX - mixer_map[channel].count;

	*max = c * mixer_map[channel].rate / mixer_rate;

	assert( *min <= *max );
}

static void mixer_channel_alloc(unsigned channel, enum mixer_type type, int loop) {
	memset(&mixer_map[channel],0,sizeof(mixer_map[channel]));

	mixer_map[channel].type = type;
	mixer_map[channel].loop = loop;

	mixer_map[channel].start = 0;
	mixer_map[channel].end = 0;
	mixer_map[channel].pos = 0;
	mixer_map[channel].empty = 0;

	mixer_map[channel].count = 0;
	mixer_map[channel].silence_count = 0;
}

static void mixer_channel_set(unsigned channel, unsigned rate, unsigned nchannel, unsigned bit) {
	mixer_map[channel].rate = rate;
	mixer_map[channel].nchannel = nchannel;
	mixer_map[channel].bit = bit;

	mixer_map[channel].pivot = 0;
	mixer_map[channel].up = mixer_rate;
	mixer_map[channel].down = rate;
}

static void mixer_channel_free(unsigned channel) {
	switch (mixer_map[channel].type) {
		case mixer_raw_file :
			fzclose(mixer_map[channel].file);
			break;
		case mixer_mp3_file :
			fzclose(mixer_map[channel].file);
			mp3_done(&mixer_map[channel].mp3);
			break;
		default:
			break;
	}

	mixer_map[channel].type = mixer_none;
}

static void mixer_channel_abort(unsigned channel) {
	if (mixer_map[channel].type != mixer_none)
		mixer_channel_free(channel);
}

static __inline__ int s16le2int(const unsigned char* data) {
	return ((int)(char)data[1] << 8) | (unsigned char)data[0];
}

static __inline__ int u8le2int(const unsigned char* data) {
	return ((int)(char)(data[0] - 128)) << 8;
}

static void mixer_channel_mix_stereo16(unsigned channel, const unsigned char* data, unsigned count) {
	unsigned i;
	unsigned pos = mixer_buffer_pos + mixer_map[channel].count;
	if (pos >= MIXER_BUFFER_MAX)
		pos -= MIXER_BUFFER_MAX;

	for(i=0;i<count;++i) {
		int c0,c1;
		mixer_map[channel].pivot += mixer_map[channel].up;
		c0 = s16le2int(data);
		c1 = s16le2int(data + 2);
		while (mixer_map[channel].pivot > 0) {
			mixer_buffer[pos*2] += c0;
			mixer_buffer[pos*2 + 1] += c1;
			++pos;
			if (pos == MIXER_BUFFER_MAX)
				pos = 0;
			++mixer_map[channel].count;
			mixer_map[channel].pivot -= mixer_map[channel].down;
		}
		data += 4;
	}
}

static void mixer_channel_mix_mono16(unsigned channel, const unsigned char* data, unsigned count) {
	unsigned i;
	unsigned pos = mixer_buffer_pos + mixer_map[channel].count;
	if (pos >= MIXER_BUFFER_MAX)
		pos -= MIXER_BUFFER_MAX;

	for(i=0;i<count;++i) {
		int c;
		mixer_map[channel].pivot += mixer_map[channel].up;
		c = s16le2int(data);
		while (mixer_map[channel].pivot > 0) {
			mixer_buffer[pos*2] += c;
			mixer_buffer[pos*2 + 1] += c;
			++pos;
			if (pos == MIXER_BUFFER_MAX)
				pos = 0;
			++mixer_map[channel].count;
			mixer_map[channel].pivot -= mixer_map[channel].down;
		}
		data += 2;
	}
}

static void mixer_channel_mix_stereo8(unsigned channel, const unsigned char* data, unsigned count) {
	unsigned i;
	unsigned pos = mixer_buffer_pos + mixer_map[channel].count;
	if (pos >= MIXER_BUFFER_MAX)
		pos -= MIXER_BUFFER_MAX;

	for(i=0;i<count;++i) {
		int c0,c1;
		mixer_map[channel].pivot += mixer_map[channel].up;
		c0 = u8le2int(data);
		c1 = u8le2int(data + 1);
		while (mixer_map[channel].pivot > 0) {
			mixer_buffer[pos*2] += c0;
			mixer_buffer[pos*2 + 1] += c1;
			++pos;
			if (pos == MIXER_BUFFER_MAX)
				pos = 0;
			++mixer_map[channel].count;
			mixer_map[channel].pivot -= mixer_map[channel].down;
		}
		data += 2;
	}
}

static void mixer_channel_mix_mono8(unsigned channel, const unsigned char* data, unsigned count) {
	unsigned i;
	unsigned pos = mixer_buffer_pos + mixer_map[channel].count;
	if (pos >= MIXER_BUFFER_MAX)
		pos -= MIXER_BUFFER_MAX;

	for(i=0;i<count;++i) {
		int c;
		mixer_map[channel].pivot += mixer_map[channel].up;
		c = u8le2int(data);
		while (mixer_map[channel].pivot > 0) {
			mixer_buffer[pos*2] += c;
			mixer_buffer[pos*2 + 1] += c;
			++pos;
			if (pos == MIXER_BUFFER_MAX)
				pos = 0;
			++mixer_map[channel].count;
			mixer_map[channel].pivot -= mixer_map[channel].down;
		}
		data += 1;
	}
}

static void mixer_channel_mix(unsigned channel, const unsigned char* data, unsigned count) {
	if (mixer_map[channel].bit == 8) {
		if (mixer_map[channel].nchannel == 1) {
			mixer_channel_mix_mono8(channel,data,count);
		} else {
			mixer_channel_mix_stereo8(channel,data,count);
		}
	} else {
		if (mixer_map[channel].nchannel == 1) {
			mixer_channel_mix_mono16(channel,data,count);
		} else {
			mixer_channel_mix_stereo16(channel,data,count);
		}
	}
}

static void mixer_channel_loop_check(unsigned channel) {
	if (mixer_map[channel].pos == mixer_map[channel].end
		&& mixer_map[channel].loop) {

		if (mixer_map[channel].file) {
			if (fzseek(mixer_map[channel].file,mixer_map[channel].start,SEEK_SET)!=0) {
				mixer_channel_abort(channel);
				return;
			}
		}

		mixer_map[channel].pos = mixer_map[channel].start;
	}
}

/***************************************************************************/
/* WAV */

int mixer_play_file_wav(unsigned channel, FZ* file, int loop) {
	unsigned rate;
	unsigned bit;
	unsigned nchannel;
	unsigned size;

	mixer_channel_abort(channel);

	if (wave_file(file,&nchannel,&bit,&size,&rate)!=0) {
		return -1;
	}

	mixer_channel_alloc(channel, mixer_raw_file, loop);
	mixer_channel_set(channel, rate, nchannel, bit);

	mixer_map[channel].file = file;
	mixer_map[channel].start = fztell(file);
	mixer_map[channel].end = mixer_map[channel].start + size;
	mixer_map[channel].pos = mixer_map[channel].start;

	return 0;
}

int mixer_play_memory_wav(unsigned channel, const unsigned char* begin, const unsigned char* end, int loop) {
	const unsigned char* start;
	unsigned rate;
	unsigned bit;
	unsigned size;
	unsigned nchannel;

	mixer_channel_abort(channel);

	start = begin;
	if (wave_memory(&start,end,&nchannel,&bit,&size,&rate)!=0) {
		return -1;
	}

	mixer_channel_alloc(channel, mixer_raw_memory, loop);
	mixer_channel_set(channel, rate, nchannel, bit);

	mixer_map[channel].data = begin;
	mixer_map[channel].start = start - begin;
	mixer_map[channel].end = mixer_map[channel].start + size;
	mixer_map[channel].pos = start - begin;

	return 0;
}

static int mixer_raw_pump(unsigned channel) {
	unsigned nmin;
	unsigned nmax;
	unsigned run;
	unsigned sample_size;
	char data[MIXER_PAGE_SIZE];

	if (mixer_channel_input_is_empty(channel))
		return -1;

	mixer_channel_need(channel,&nmin,&nmax);

	/* no need */
	if (nmin == 0)
		return -1;

	sample_size = 4;
	if (mixer_map[channel].nchannel == 1)
		sample_size /= 2;
	if (mixer_map[channel].bit == 8)
		sample_size /= 2;

	nmin *= sample_size;
	nmax *= sample_size;

	run = nmin;

	if (mixer_map[channel].pos + run > mixer_map[channel].end)
		run = mixer_map[channel].end -  mixer_map[channel].pos;
	if (run > MIXER_PAGE_SIZE)
		run = MIXER_PAGE_SIZE;

	assert( run % sample_size == 0 );

	if (run) {
		if (mixer_map[channel].file) {
			if (fzread(data,run,1,mixer_map[channel].file)!=1) {
				mixer_channel_abort(channel);
				return -1;
			}
			mixer_channel_mix(channel, data, run / sample_size);
		} else {
			mixer_channel_mix(channel, mixer_map[channel].data + mixer_map[channel].pos, run / sample_size);
		}
	}

	mixer_map[channel].pos += run;

	mixer_channel_loop_check(channel);

	/* check for the end of the stream */
	if (mixer_map[channel].pos == mixer_map[channel].end
		&& !mixer_map[channel].loop) {
		mixer_map[channel].empty = 1;
	}

	if (!run)
		return -1;

	return 0;
}

/***************************************************************************/
/* MP3 */

/* Read some data from the mp3 file stream */
static unsigned mp3_read_data(unsigned channel, unsigned char* data, unsigned max) {
	unsigned run;

	if (mixer_map[channel].pos == mixer_map[channel].end)
		return 0;

	run = max;
	if (mixer_map[channel].pos + run > mixer_map[channel].end)
		run = mixer_map[channel].end - mixer_map[channel].pos;

	if (!run)
		return 0;

	if (mixer_map[channel].file) {
		if (fzread(data,run,1,mixer_map[channel].file)!=1) {
			mixer_channel_abort(channel);
			return 0;
		}
	} else {
		memcpy(data,mixer_map[channel].data + mixer_map[channel].pos, run);
	}

	mixer_map[channel].pos += run;

	mixer_channel_loop_check(channel);

	return run;
}

static int mp3_skip(unsigned channel, unsigned size_to_skip, unsigned char* data, unsigned* pos, unsigned* run) {
	/* skip data */
	while (size_to_skip) {
		if (*run >= size_to_skip) {
			*pos += size_to_skip;
			*run -= size_to_skip;
			size_to_skip = 0;
		} else {
			size_to_skip -= *run;
			*pos = 0;
			*run = mp3_read_data(channel,data,MIXER_PAGE_SIZE);
			if (!*run)
				return -1;
		}
	}

	return 0;
}

static int mp3_read(unsigned char* c, unsigned channel, unsigned char* data, unsigned* pos, unsigned* run) {
	if (!*run) {
		*pos = 0;
		*run = mp3_read_data(channel,data,MIXER_PAGE_SIZE);
		if (!*run)
			return -1;
	}

	*c = data[*pos];

	++*pos;
	--*run;

	return 0;
}

static int mp3_read_id(unsigned char* id, unsigned channel, unsigned char* data, unsigned* pos, unsigned* run) {
	if (mp3_read(id+0, channel, data, pos, run) != 0)
		return -1;
	if (mp3_read(id+1, channel, data, pos, run) != 0)
		return -1;
	if (mp3_read(id+2, channel, data, pos, run) != 0)
		return -1;
	if (mp3_read(id+3, channel, data, pos, run) != 0)
		return -1;
	return 0;
}

static int mp3_read_le32(unsigned* v, unsigned channel, unsigned char* data, unsigned* pos, unsigned* run) {
	unsigned char id[4];
	if (mp3_read_id(id, channel, data, pos, run) != 0)
		return -1;
	*v = id[0] | (id[1] << 8) | (id[2] << 16) | (id[3] << 24);
	return 0;
}

static int mp3_first_read_stream(unsigned channel) {
	unsigned char data[MIXER_PAGE_SIZE];
	unsigned pos = 0;
	unsigned run = mp3_read_data(channel,data,MIXER_PAGE_SIZE);
	if (!run) {
		return -1;
	}

	if (run>=12
		&& data[0]=='R' && data[1]=='I' && data[2]=='F' && data[3]=='F'
		&& data[8]=='W' && data[9]=='A' && data[10]=='V' && data[11]=='E') {
		/* RIFF header */
		unsigned char id[4];

		os_log(("mixer: skip RIFF header\n"));

		if (mp3_skip(channel, 12, data, &pos, &run) != 0)
			return -1;

		if (mp3_read_id(id, channel, data, &pos, &run) != 0)
			return -1;

		while (memcmp(id,"data",4)!=0) {
			unsigned size;
			if (mp3_read_le32(&size, channel, data, &pos, &run) != 0)
				return -1;
			if (size % 2)
				size += 1;

			if (mp3_skip(channel, size, data, &pos, &run) != 0)
				return -1;

			if (mp3_read_id(id, channel, data, &pos, &run) != 0)
				return -1;
		}

		if (mp3_skip(channel, 4, data, &pos, &run) != 0)
			return -1;

	} else if (run>=10 && data[0]=='I' && data[1]=='D' && data[2]=='3'
		&& data[3]!=0xFF && data[4]!=0xFF
		&& (data[6] & 0x80) == 0 && (data[7] & 0x80) == 0
		&& (data[8] & 0x80) == 0 && (data[9] & 0x80) == 0) {

		/* ID3 tag header */
		unsigned size = (unsigned)data[9] | (((unsigned)data[8]) << 7) | (((unsigned)data[7]) << 14) | (((unsigned)data[6]) << 21);
		size += 10;

		os_log(("mixer: skip ID3 header %d\n",size));

		if (mp3_skip(channel, size, data, &pos, &run) != 0)
			return -1;
	}

	if (run>=4 && !mp3_is_valid(data + pos))
		return -1;

	/* insert the data */
	if (run)
		mp3_decode(&mixer_map[channel].mp3,(char*)data + pos,run,0,0,0);

	return 0;
}

/* Read some data from the mp3 file stream */
static int mp3_read_stream(unsigned channel) {
	if (mixer_map[channel].pos == mixer_map[channel].start) {
		return mp3_first_read_stream(channel);
	} else {
		unsigned char data[MIXER_PAGE_SIZE];
		unsigned run = mp3_read_data(channel,data,MIXER_PAGE_SIZE);

		if (!run) {
			return -1;
		}

		mp3_decode(&mixer_map[channel].mp3,data,run,0,0,0);

		return 0;
	}
}

static int mixer_mp3_pump(unsigned channel) {
	unsigned char buffer[2304*4]; /* 2304 samples for MP3 frame */
	unsigned nmin;
	unsigned nmax;
	int bytes_done;
	int err;

	if (mixer_channel_input_is_empty(channel))
		return -1;

	if (mixer_map[channel].rate) {
		/* check for avaiable space */
		mixer_channel_need(channel,&nmin,&nmax);

		/* no space */
		if (nmax < 2304)
			return -1;

		/* no need */
		if (nmin == 0)
			return -1;
	}

	bytes_done = 0;
	err = mp3_decode(&mixer_map[channel].mp3,0,0,buffer,sizeof(buffer)/2,&bytes_done);

	/* insert data until it's required */
	while (err == MP3_NEED_MORE && mp3_read_stream(channel)==0) {
		err = mp3_decode(&mixer_map[channel].mp3,0,0,buffer,sizeof(buffer)/2,&bytes_done);
	}

	if (err == MP3_NEED_MORE) {
		/* end of the stream */
		mixer_map[channel].empty = 1;
		return -1;
	}

	if (err != MP3_OK) {
		/* generic error */
		mixer_channel_abort(channel);
		return -1;
	}

	if (!bytes_done)
		/* no data ? exit */
		return 0;

	/* delayed set of the channel */
	if (!mixer_map[channel].rate) {
		unsigned rate;
		unsigned nchannel;
		unsigned bit;

		rate = mp3_freqs[mixer_map[channel].mp3.fr.sampling_frequency];
		nchannel = mixer_map[channel].mp3.fr.stereo; /* this is correct, stereo is the number of channel */
		bit = 16;

		mixer_channel_set(channel, rate, nchannel, bit);
	}

	/* override the nchannel value */
	if (mixer_map[channel].mp3.fr.stereo == 1) {
		mixer_channel_mix_mono16(channel,buffer,bytes_done / 2);
	} else {
		mixer_channel_mix_stereo16(channel,buffer,bytes_done / 4);
	}

	return 0;
}

int mixer_play_file_mp3(unsigned channel, FZ* file, int loop)
{
	mixer_channel_abort(channel);

	mixer_channel_alloc(channel, mixer_mp3_file, loop);

	mixer_map[channel].file = file;
	mixer_map[channel].start = fztell(file);
	mixer_map[channel].end = fzsize(file);
	mixer_map[channel].pos = mixer_map[channel].start;

	mp3_init(&mixer_map[channel].mp3);

	return 0;
}

static void mixer_channel_pump(unsigned channel) {
	int r;

	switch (mixer_map[channel].type) {
		case mixer_none :
			break;
		case mixer_raw_file :
		case mixer_raw_memory :
			do {
				r = mixer_raw_pump(channel);
			} while (r == 0);
			break;
		case mixer_mp3_file :
			do {
				r = mixer_mp3_pump(channel);
			} while (r == 0);
			break;
	}
}

/***************************************************************************/
/* Main */

int mixer_is_playing(unsigned channel) {
	return mixer_channel_is_active(channel);
}

int mixer_is_pushing(unsigned channel) {
	return mixer_map[channel].type != mixer_none
		&& (!mixer_channel_input_is_empty(channel) || !mixer_channel_data_is_empty(channel));
}

void mixer_stop(unsigned channel) {
	/* move at the end */
	if (mixer_is_pushing(channel)) {
		mixer_map[channel].empty = 1;
	}
}

/***************************************************************************/
/* Main */

void mixer_poll(void) {
	unsigned i;

	for(i=0;i<mixer_nchannel;++i)
		mixer_channel_pump(i);

	mixer_pump( sound_buffered() );
}

void mixer_reg(struct conf_context* context) {
	sound_reg(context);
	sound_reg_driver_all(context);
}

int mixer_load(struct conf_context* context) {
	if (sound_load(context)!=0) {
		return -1;
	}

	return 0;
}

int mixer_init(unsigned rate, unsigned nchannel, unsigned ndivider, double buffer_time, double latency_time) {
	unsigned i;

	assert(nchannel <= MIXER_CHANNEL_MAX);
	assert(ndivider > 0);

	os_log(("mixer: mixer_init(rate:%d,nchannel:%d,ndivider:%d,buffer:%g,latency:%g)\n",rate,nchannel,ndivider,buffer_time,latency_time));

	for(i=0;i<MIXER_CHANNEL_MAX;++i)
		mixer_map[i].type = mixer_none;

	mp3_lib_init();

	mixer_nchannel = nchannel;
	mixer_ndivider = ndivider;
	mixer_rate = rate;

	if (buffer_time < latency_time)
		goto err;

	if (sound_init(&mixer_rate, 1, latency_time * 1.1) != 0)
		goto err;

	mixer_buffer_size = buffer_time * mixer_rate;
	if (mixer_buffer_size > MIXER_BUFFER_MAX)
		goto err_done;

	mixer_latency_size = latency_time * mixer_rate;
	if (mixer_latency_size > mixer_buffer_size)
		goto err_done;

	if (sound_start(latency_time) != 0)
		goto err_done;

	return 0;

err_done:
	sound_done();
err:
	return -1;
}

void mixer_done(void) {
	unsigned i;

	os_log(("mixer: mixer_done()\n"));

	for(i=0;i<MIXER_CHANNEL_MAX;++i)
		mixer_channel_abort(i);

	sound_stop();
	sound_done();

	mp3_lib_done();
}

void mixer_volume(double volume) {
	sound_volume(volume);
}



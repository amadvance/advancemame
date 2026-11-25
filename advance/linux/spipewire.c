/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2026 Andrea Mazzoleni
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
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

/** \file
 * Sound driver "pipewire".
 *
 * The samples are written by soundb_pipewire_play() in a lock-free ring
 * buffer, and read by the process callback in the PipeWire realtime thread.
 * The buffered samples reported include also the delay of the PipeWire
 * graph up to the device, that is not visible with the ALSA pulse/pipewire
 * plugins, and allows to the audio/video sync to control the real latency.
 *
 * It supports PipeWire 0.3.44 and newer, using the newer API when available.
 */

#include "portable.h"

#include "spipewire.h"
#include "log.h"
#include "error.h"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/ringbuffer.h>
#include <spa/utils/result.h>
#include <spa/node/io.h>

/**
 * Base for the volume adjustment.
 */
#define PIPEWIRE_VOLUME_BASE 32768

/**
 * Maximum time to wait the connection and the format negotiation in seconds.
 */
#define PIPEWIRE_TIMEOUT 2

struct soundb_pipewire_context {
	struct pw_thread_loop* loop; /**< Thread running the PipeWire callbacks. */
	struct pw_stream* stream; /**< Playback stream. */
	struct spa_io_position* position; /**< Position of the graph, it contains the current quantum. */
	unsigned channel; /**< Number of channels (1 or 2). */
	unsigned rate; /**< Playing rate in Hz. */
	unsigned stride; /**< Size of a frame (all channels) in bytes. */
	unsigned period; /**< Requested quantum in frames. */
	struct spa_ringbuffer ring; /**< Ring buffer state. */
	unsigned char* ring_map; /**< Ring buffer data. */
	uint32_t ring_size; /**< Ring buffer size in bytes, a power of 2. */
	int volume; /**< Volume adjustement. PIPEWIRE_VOLUME_BASE == full volume. */
	adv_bool format_flag; /**< Format negotiated. */
	adv_bool error_flag; /**< Stream in error state. */
	adv_bool started_flag; /**< Stream started, accessed by the realtime thread. */
	adv_bool underflow_flag; /**< Underflow detected, accessed by the realtime thread. */
};

static struct soundb_pipewire_context pipewire_state;

static adv_device DEVICE[] = {
	{ "auto", -1, "PipeWire sound" },
	{ 0, 0, 0 }
};

static void pipewire_state_changed(void* data, enum pw_stream_state old, enum pw_stream_state state, const char* error)
{
	struct soundb_pipewire_context* context = data;

	log_std(("sound:pipewire: state %s -> %s%s%s\n", pw_stream_state_as_string(old), pw_stream_state_as_string(state), error ? ", " : "", error ? error : ""));

	if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED) {
		context->error_flag = 1;
		pw_thread_loop_signal(context->loop, 0);
	}
}

static void pipewire_param_changed(void* data, uint32_t id, const struct spa_pod* param)
{
	struct soundb_pipewire_context* context = data;
	struct spa_audio_info_raw info;

	if (param == 0 || id != SPA_PARAM_Format)
		return;

	memset(&info, 0, sizeof(info));
	if (spa_format_audio_raw_parse(param, &info) < 0)
		return;

	log_std(("sound:pipewire: negotiated rate %u, channels %u\n", info.rate, info.channels));

	context->rate = info.rate;
	context->format_flag = 1;
	pw_thread_loop_signal(context->loop, 0);
}

static void pipewire_io_changed(void* data, uint32_t id, void* area, uint32_t size)
{
	struct soundb_pipewire_context* context = data;

	if (id == SPA_IO_Position)
		context->position = area;
}

/**
 * Number of frames to output in the current cycle.
 */
static uint32_t pipewire_cycle_frames(struct soundb_pipewire_context* context, struct pw_buffer* b, uint32_t max_frames)
{
	uint32_t n_frames = 0;

#if PW_CHECK_VERSION(0, 3, 49)
	n_frames = b->requested;
#endif

	/* with older versions use the duration of the current cycle, */
	/* because filling the whole buffer would add a lot of latency */
	if (n_frames == 0 && context->position != 0 && context->position->clock.rate.denom != 0) {
		struct spa_io_clock* clock = &context->position->clock;
		n_frames = (clock->duration * clock->rate.num * context->rate + clock->rate.denom - 1) / clock->rate.denom;
	}

	if (n_frames == 0)
		n_frames = context->period;

	if (n_frames > max_frames)
		n_frames = max_frames;

	return n_frames;
}

/**
 * Process callback, called in the PipeWire realtime thread.
 * It must not lock or block.
 */
static void pipewire_process(void* data)
{
	struct soundb_pipewire_context* context = data;
	struct pw_buffer* b;
	struct spa_data* d;
	uint32_t n_bytes;
	uint32_t avail;
	uint32_t index;
	int32_t filled;

	b = pw_stream_dequeue_buffer(context->stream);
	if (b == 0)
		return;

	d = &b->buffer->datas[0];
	if (d->data == 0) {
		pw_stream_queue_buffer(context->stream, b);
		return;
	}

	n_bytes = pipewire_cycle_frames(context, b, d->maxsize / context->stride) * context->stride;

	filled = spa_ringbuffer_get_read_index(&context->ring, &index);

	avail = filled > 0 ? filled : 0;
	if (avail > n_bytes)
		avail = n_bytes;

	spa_ringbuffer_read_data(&context->ring, context->ring_map, context->ring_size, index & (context->ring_size - 1), d->data, avail);
	spa_ringbuffer_read_update(&context->ring, index + avail);

	/* complete with silence if not enough samples */
	if (avail < n_bytes) {
		memset((unsigned char*)d->data + avail, 0, n_bytes - avail);
		if (__atomic_load_n(&context->started_flag, __ATOMIC_RELAXED))
			__atomic_store_n(&context->underflow_flag, 1, __ATOMIC_RELAXED);
	}

	d->chunk->offset = 0;
	d->chunk->stride = context->stride;
	d->chunk->size = n_bytes;

	pw_stream_queue_buffer(context->stream, b);
}

static const struct pw_stream_events pipewire_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = pipewire_state_changed,
	.param_changed = pipewire_param_changed,
	.io_changed = pipewire_io_changed,
	.process = pipewire_process,
};

adv_error soundb_pipewire_init(int sound_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	struct soundb_pipewire_context* context = &pipewire_state;
	const struct spa_pod* params[1];
	uint8_t pod_buffer[1024];
	struct spa_pod_builder pod_builder = SPA_POD_BUILDER_INIT(pod_buffer, sizeof(pod_buffer));
	struct spa_audio_info_raw info;
	struct pw_properties* props;
	unsigned period_rate;
	uint32_t size;
	int r;

	log_std(("sound:pipewire: soundb_pipewire_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", sound_id, *rate, stereo_flag, buffer_time));

	/* pw_deinit() is never called, because older versions don't support */
	/* a new pw_init() after it, like when the sound driver is restarted */
	pw_init(0, 0);

	log_std(("sound:pipewire: compiled with libpipewire %s, linked with libpipewire %s\n", pw_get_headers_version(), pw_get_library_version()));

	memset(context, 0, sizeof(*context));
	context->channel = stereo_flag ? 2 : 1;
	context->stride = context->channel * sizeof(adv_sample);
	context->rate = *rate;
	context->volume = PIPEWIRE_VOLUME_BASE;

	/* request the same granularity of the ALSA driver, that is 32 periods */
	/* for the buffer. Note that this is the requested latency of the node, */
	/* and the effective quantum of the graph is chosen by PipeWire */
	period_rate = *rate != 0 ? *rate : 48000;
	context->period = period_rate * buffer_time / 32;
	if (context->period < 64)
		context->period = 64;

	context->loop = pw_thread_loop_new("advance-sound", 0);
	if (context->loop == 0) {
		error_set("Function pw_thread_loop_new() failed.\n");
		goto err;
	}

	props = pw_properties_new(
		PW_KEY_MEDIA_TYPE, "Audio",
		PW_KEY_MEDIA_CATEGORY, "Playback",
		PW_KEY_MEDIA_ROLE, "Game",
		NULL); /* the sentinel must be a pointer */
	pw_properties_setf(props, PW_KEY_NODE_LATENCY, "%u/%u", context->period, period_rate);

	log_std(("sound:pipewire: request latency %u/%u\n", context->period, period_rate));

	/* the properties are owned by the stream, also on failure */
	context->stream = pw_stream_new_simple(pw_thread_loop_get_loop(context->loop), "Advance", props, &pipewire_events, context);
	if (context->stream == 0) {
		error_set("Function pw_stream_new_simple() failed. Is PipeWire running ?\n");
		goto err_loop;
	}

	/* a rate of 0 is not specified in the format, and PipeWire */
	/* negotiates the rate of the graph to avoid a resampling */
	memset(&info, 0, sizeof(info));
	info.format = SPA_AUDIO_FORMAT_S16;
	info.channels = context->channel;
	info.rate = *rate;
	if (context->channel == 2) {
		info.position[0] = SPA_AUDIO_CHANNEL_FL;
		info.position[1] = SPA_AUDIO_CHANNEL_FR;
	} else {
		info.position[0] = SPA_AUDIO_CHANNEL_MONO;
	}
	params[0] = spa_format_audio_raw_build(&pod_builder, SPA_PARAM_EnumFormat, &info);

	if (pw_thread_loop_start(context->loop) != 0) {
		error_set("Function pw_thread_loop_start() failed.\n");
		goto err_stream;
	}

	pw_thread_loop_lock(context->loop);

	/* the stream is not started inactive, because the session manager may */
	/* not link an inactive stream, and the format is negotiated only after */
	/* the link. Until the start, the process callback outputs silence */
	r = pw_stream_connect(context->stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
			PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
			params, 1);
	if (r < 0) {
		pw_thread_loop_unlock(context->loop);
		error_set("Function pw_stream_connect() failed, %s.\n", spa_strerror(r));
		goto err_stop;
	}

	/* wait the format negotiation, required to know the effective rate */
	while (!context->format_flag && !context->error_flag) {
		if (pw_thread_loop_timed_wait(context->loop, PIPEWIRE_TIMEOUT) != 0) {
			log_std(("ERROR:sound:pipewire: timeout waiting the format negotiation\n"));
			context->error_flag = 1;
		}
	}

	pw_thread_loop_unlock(context->loop);

	if (context->error_flag || context->rate == 0) {
		error_set("Unable to connect to PipeWire.\n");
		goto err_stop;
	}

	/* allocate a ring buffer able to store the whole buffer time */
	size = context->rate * buffer_time * context->stride;
	context->ring_size = 1;
	while (context->ring_size < size)
		context->ring_size *= 2;
	context->ring_map = malloc(context->ring_size);
	spa_ringbuffer_init(&context->ring);

	log_std(("sound:pipewire: rate %u, ring buffer %u bytes, %u samples\n", context->rate, context->ring_size, context->ring_size / context->stride));

	*rate = context->rate;

	return 0;

err_stop:
	pw_thread_loop_stop(context->loop);
err_stream:
	pw_stream_destroy(context->stream);
err_loop:
	pw_thread_loop_destroy(context->loop);
err:
	return -1;
}

void soundb_pipewire_done(void)
{
	struct soundb_pipewire_context* context = &pipewire_state;

	log_std(("sound:pipewire: soundb_pipewire_done()\n"));

	/* stop the thread before destroying the stream used by the callbacks */
	pw_thread_loop_stop(context->loop);
	pw_stream_destroy(context->stream);
	pw_thread_loop_destroy(context->loop);

	free(context->ring_map);
}

void soundb_pipewire_stop(void)
{
	struct soundb_pipewire_context* context = &pipewire_state;

	log_std(("sound:pipewire: soundb_pipewire_stop()\n"));

	__atomic_store_n(&context->started_flag, 0, __ATOMIC_RELAXED);

	pw_thread_loop_lock(context->loop);
	pw_stream_set_active(context->stream, 0);
	pw_thread_loop_unlock(context->loop);
}

unsigned soundb_pipewire_buffered(void)
{
	struct soundb_pipewire_context* context = &pipewire_state;
	struct pw_time t;
	uint32_t index;
	int32_t filled;
	uint64_t buffered;
	int r;

	filled = spa_ringbuffer_get_write_index(&context->ring, &index);
	buffered = filled > 0 ? filled / context->stride : 0;

	/* add the samples already passed to PipeWire, and not yet played */
	memset(&t, 0, sizeof(t));
#if PW_CHECK_VERSION(0, 3, 50)
	r = pw_stream_get_time_n(context->stream, &t, sizeof(t));
#else
	r = pw_stream_get_time(context->stream, &t);
#endif
	if (r == 0) {
#if PW_CHECK_VERSION(0, 3, 50)
		/* samples stored in the resampler of the stream */
		buffered += t.buffered;
#endif

		/* samples queued in the stream */
		buffered += t.queued / context->stride;

		/* delay of the graph up to the device, in units of the graph rate */
		if (t.rate.denom != 0) {
			int64_t delay = t.delay >= 0 ? t.delay : -t.delay;
			buffered += delay * t.rate.num * context->rate / t.rate.denom;
		}

		log_debug(("sound:pipewire: ring %d, queued %u, delay %d, rate %u/%u\n", filled / (int)context->stride, (unsigned)t.queued, (int)t.delay, t.rate.num, t.rate.denom));
	}

	return buffered;
}

void soundb_pipewire_volume(double volume)
{
	struct soundb_pipewire_context* context = &pipewire_state;

	log_std(("sound:pipewire: soundb_pipewire_volume(volume:%g)\n", volume));

	context->volume = volume * PIPEWIRE_VOLUME_BASE;
	if (context->volume < 0)
		context->volume = 0;
	if (context->volume > PIPEWIRE_VOLUME_BASE)
		context->volume = PIPEWIRE_VOLUME_BASE;
}

static void pipewire_write(struct soundb_pipewire_context* context, const adv_sample* sample_map, unsigned sample_count)
{
	uint32_t index;
	int32_t filled;
	uint32_t n_bytes;
	uint32_t space;

	filled = spa_ringbuffer_get_write_index(&context->ring, &index);
	space = context->ring_size - (filled > 0 ? filled : 0);

	n_bytes = sample_count * context->stride;
	if (n_bytes > space) {
		log_std(("ERROR:sound:pipewire: buffer overflow, %u samples dropped\n", (n_bytes - space) / context->stride));
		n_bytes = space - space % context->stride;
	}

	if (context->volume == PIPEWIRE_VOLUME_BASE) {
		spa_ringbuffer_write_data(&context->ring, context->ring_map, context->ring_size, index & (context->ring_size - 1), sample_map, n_bytes);
		index += n_bytes;
	} else {
		/* adjust the volume */
		adv_sample buf_map[2048];
		unsigned count = n_bytes / sizeof(adv_sample);

		while (count) {
			unsigned run = count;
			unsigned i;

			if (run > 2048)
				run = 2048;

			for (i = 0; i < run; ++i)
				buf_map[i] = (int)sample_map[i] * context->volume / PIPEWIRE_VOLUME_BASE;

			spa_ringbuffer_write_data(&context->ring, context->ring_map, context->ring_size, index & (context->ring_size - 1), buf_map, run * sizeof(adv_sample));
			index += run * sizeof(adv_sample);
			sample_map += run;
			count -= run;
		}
	}

	/* publish the new samples to the realtime thread */
	spa_ringbuffer_write_update(&context->ring, index);
}

void soundb_pipewire_play(const adv_sample* sample_map, unsigned sample_count)
{
	struct soundb_pipewire_context* context = &pipewire_state;

	log_debug(("sound:pipewire: soundb_pipewire_play(count:%d)\n", sample_count));

	if (__atomic_exchange_n(&context->underflow_flag, 0, __ATOMIC_RELAXED))
		log_std(("ERROR:sound:pipewire: buffer underflow. Increase the latency with -sound_latency.\n"));

	pipewire_write(context, sample_map, sample_count);

	/* report the underflows only after the first samples written by the user, */
	/* because the silence written at the start may be consumed before them */
	/* by the remaining initialization of the program */
	__atomic_store_n(&context->started_flag, 1, __ATOMIC_RELAXED);
}

adv_error soundb_pipewire_start(double silence_time)
{
	struct soundb_pipewire_context* context = &pipewire_state;
	adv_sample buf[256];
	unsigned sample;
	unsigned i;

	log_std(("sound:pipewire: soundb_pipewire_start(silence_time:%g)\n", silence_time));

	for (i = 0; i < 256; ++i)
		buf[i] = 0;

	sample = silence_time * context->rate * context->channel;

	while (sample) {
		unsigned run = sample;
		if (run > 256)
			run = 256;
		sample -= run;
		pipewire_write(context, buf, run / context->channel);
	}

	pw_thread_loop_lock(context->loop);
	pw_stream_set_active(context->stream, 1);
	pw_thread_loop_unlock(context->loop);

	return 0;
}

unsigned soundb_pipewire_flags(void)
{
	return SOUND_DRIVER_FLAGS_VOLUME_SAMPLE;
}

adv_error soundb_pipewire_load(adv_conf* context)
{
	return 0;
}

void soundb_pipewire_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_pipewire_driver = {
	"pipewire",
	DEVICE,
	soundb_pipewire_load,
	soundb_pipewire_reg,
	soundb_pipewire_init,
	soundb_pipewire_done,
	soundb_pipewire_flags,
	soundb_pipewire_play,
	soundb_pipewire_buffered,
	soundb_pipewire_start,
	soundb_pipewire_stop,
	soundb_pipewire_volume
};


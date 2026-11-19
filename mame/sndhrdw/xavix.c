// license:BSD-3-Clause
// copyright-holders:ramacat, David Haywood
/***************************************************************************

    xavix.c

    XaviX Audio Renderer

    - The sound core mixes up to 16 voices per sample at a fixed render rate.
    - Each voice reads 8-bit inverted sign-magnitude PCM, with four wave modes (WM):
        WM0: RAM wavetable            WM1: noise LFSR
        WM2: ROM wavetable w/ loop    WM3: ROM one-shot (stops at 0x80)
    - Phase is 18.14 fixed-point.
    - Byte value 0x80 is a sentinel: WM2 loops to the loop position, WM3 disables the voice.
    - Envelope Modes (VM) control per-side levels:
        VM0: direct LA/RA registers (no clock); VM1: nibble table;
        VM2: byte stream; VM3: exponential-ish decay.
    - The envelope advance is driven by per-group tempo tp[0..3] and the cyclerate.
    - The pitch bends slew the rate by +/-1 per render tick toward the wanted value.
    - Per-voice volume = sample * voice gain(4-bit) * master volume * envelope L/R, then summed.
    - Noise (WM1) uses a 16-bit LFSR with a fixed/nonzero seed.

    Differences from MAME:
    - The stream is generated at the sound sample rate, averaging the ticks of
      the sequencer, instead of using the sequencer rate for the stream.

***************************************************************************/

#include "driver.h"
#include "includes/xavix.h"
#include "sound/custom.h"
#include "streams.h"

/* the sequencer rate is the main clock / (( cyclerate + 1 ) * 8 ) with the default cyclerate 0x0f */
#define SEQUENCER_RATE_HZ (XAVIX_MAIN_CLOCK / ((0x0f + 1) * 8))

static const UINT8 MIXER_ORDER_MULTIPLEX[16] = { 0x0, 0xa, 0x7, 0xd, 0xc, 0x6, 0xb, 0x1, 0x4, 0xe, 0x3, 0x9, 0x8, 0x2, 0xf, 0x5 };
static const UINT8 MIXER_ORDER_BROADCAST[16] = { 0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf };
static const int AMP_TABLE[8] = { 2, 4, 8, 12, 16, 20, 20, 20 };

struct xavix_voice
{
	UINT8 enabled;
	UINT32 position;
	UINT32 loop_position;
	UINT32 start_position;
	UINT32 env_pos_left;
	UINT32 env_pos_right;
	UINT8 env_bank;
	UINT8 bank;
	UINT32 rate;
	UINT8 type;
	UINT8 vol;
	UINT16 noise_state;
	UINT16 env_rom_base_left;
	UINT16 env_rom_base_right;
	UINT8 env_vol_left;
	UINT8 env_vol_right;
	UINT8 env_mode;
	UINT32 env_period_samples;
	UINT32 env_countdown;
	UINT8 env_active_left;
	UINT8 env_active_right;
};

struct xavix_mix
{
	UINT8 monaural;
	UINT8 capacity;
	UINT8 amp;
	UINT8 dac;
	UINT8 gap;
	UINT8 lead;
	UINT8 lag;
	UINT8 mastervol;
	int gain;
};

static sound_stream *stream;
static struct xavix_voice voice[16];
static struct xavix_mix mix;
static UINT8 tempo_div[4];
static UINT8 cyclerate_div;
static UINT8 tempo_reset_value;
static UINT32 tick_accum;

/***************************************************************************

    Rates

***************************************************************************/

static UINT32 tempo_to_period_ticks(UINT8 tp)
{
	UINT32 s;
	UINT64 samples;

	/* the tempo 0 is paused */
	if (!tp)
		return 0;

	s = (UINT32)cyclerate_div + 1;
	samples = ((UINT64)s * 1024u + (tp / 2)) / tp;
	if (samples < 1)
		samples = 1;
	if (samples > 2000000u)
		samples = 2000000u;
	return (UINT32)samples;
}

static UINT32 envelope_period_ticks(UINT8 tp)
{
	UINT32 base = tempo_to_period_ticks(tp);
	UINT64 scaled;

	if (!base)
		return 0;

	scaled = (UINT64)base * 16u;
	return (UINT32)(scaled > 64000000u ? 64000000u : scaled);
}

double xavix_sound_tempo_tick_hz(UINT8 tempo)
{
	UINT32 samples = tempo_to_period_ticks(tempo);

	return samples ? ((double)SEQUENCER_RATE_HZ / (double)samples) : 0.0;
}

static UINT32 phase_step_per_tick(UINT32 rate)
{
	return rate;
}

void xavix_sound_set_default_tempo(UINT8 tempo)
{
	tempo_reset_value = tempo;
}

void xavix_sound_set_tempo(int index, UINT8 value)
{
	int v;

	if (index < 0 || index > 3)
		return;

	if (stream)
		stream_update(stream, 0);

	tempo_div[index] = value;

	for (v = 0; v < 16; ++v)
	{
		UINT32 newp;

		if ((v & 3) != index)
			continue;

		newp = (value == 0) ? 0u : envelope_period_ticks(value);

		voice[v].env_period_samples = newp;

		/* running or changed rate: arm a full period before the next tick */
		if (newp)
			voice[v].env_countdown = newp;
	}
}

void xavix_sound_set_cyclerate(UINT8 value)
{
	int v;

	if (stream)
		stream_update(stream, 0);

	cyclerate_div = value;

	/* recompute the periods of the voices */
	for (v = 0; v < 16; ++v)
	{
		UINT8 tp = tempo_div[v & 3];
		UINT32 oldp_raw = voice[v].env_period_samples;
		UINT32 newp = (tp == 0) ? 0 : envelope_period_ticks(tp);

		if (oldp_raw && newp)
			voice[v].env_countdown = (UINT64)voice[v].env_countdown * newp / oldp_raw;
		else if (!oldp_raw && newp)
			voice[v].env_countdown = newp;

		voice[v].env_period_samples = newp;
	}
}

/***************************************************************************

    Envelopes

***************************************************************************/

INLINE UINT8 fetch_env_byte_direct(int v, UINT16 addr)
{
	return xavix_sound_sample_read(((UINT32)voice[v].env_bank << 16) | addr);
}

INLINE UINT16 advance_nibble(UINT16 value)
{
	return (value & 0xfff0) | ((value + 1) & 0x000f);
}

/* the level of a side from the nibble table (VM1) or from the byte stream (VM2) */
static void step_side(int vi, int channel, int stream_mode)
{
	struct xavix_voice *v = &voice[vi];
	UINT32 *ptr32;
	UINT8 *level;
	UINT16 ptr;
	UINT16 next_ptr;

	if (channel ? !v->env_active_right : !v->env_active_left)
		return;

	ptr32 = channel ? &v->env_pos_right : &v->env_pos_left;
	level = channel ? &v->env_vol_right : &v->env_vol_left;
	ptr = (UINT16)*ptr32;

	*level = fetch_env_byte_direct(vi, ptr);

	if (stream_mode)
	{
		next_ptr = ptr + 1;
		xavix_sound_regram_write(vi * 0x10 + (channel ? 0x0c : 0x0a), (UINT8)ptr);
	}
	else
	{
		next_ptr = advance_nibble(ptr);
		xavix_sound_regram_write(vi * 0x10 + (channel ? 0x0c : 0x0a), (UINT8)next_ptr);
	}

	*ptr32 = next_ptr;
}

static UINT8 decay(UINT8 x)
{
	UINT8 high = x >> 4;
	UINT8 low = x & 0x0f;
	UINT8 subtract = high + (low ? 1 : 0);

	return (subtract >= x) ? 0 : (UINT8)(x - subtract);
}

static void step_envelope(int vi)
{
	struct xavix_voice *v = &voice[vi];
	UINT32 target_period;

	if (!v->enabled)
		return;

	target_period = envelope_period_ticks(tempo_div[vi & 3]);

	/* smooth re-tune to the new period */
	if (v->env_period_samples != target_period)
	{
		UINT32 oldp = v->env_period_samples ? v->env_period_samples : 1;

		v->env_countdown = (UINT64)v->env_countdown * target_period / oldp;
		v->env_period_samples = target_period;
	}

	/* the tempo is paused */
	if (v->env_period_samples == 0)
		return;

	if (v->env_countdown)
	{
		--v->env_countdown;
		return;
	}
	v->env_countdown = v->env_period_samples;

	/* VM0: the direct registers are reflected into the levels */
	if (v->env_mode == 0)
	{
		v->env_vol_left = xavix_sound_regram_read(vi * 0x10 + 0xa);
		v->env_vol_right = xavix_sound_regram_read(vi * 0x10 + 0xc);
		return;
	}

	/* VM1: nibble table, VM2: linear ROM stream per side */
	if (v->env_mode == 1 || v->env_mode == 2)
	{
		step_side(vi, 0, v->env_mode == 2);
		step_side(vi, 1, v->env_mode == 2);

		if ((v->env_vol_left | v->env_vol_right) == 0)
		{
			v->env_active_left = 0;
			v->env_active_right = 0;
		}
		return;
	}

	/* VM3: exponential-ish decay */
	if (v->env_mode == 3)
	{
		v->env_vol_left = decay(v->env_vol_left);
		v->env_vol_right = decay(v->env_vol_right);

		if ((v->env_vol_left | v->env_vol_right) == 0)
		{
			v->env_active_left = 0;
			v->env_active_right = 0;
		}
	}
}

/* the pitch slews toward the wanted rate */
static void step_pitch(int vi)
{
	struct xavix_voice *v = &voice[vi];
	UINT32 target;
	int base = vi * 0x10;
	UINT16 wave_control;

	if (!v->enabled)
		return;

	wave_control = (xavix_sound_regram_read(base + 0x01) << 8) | xavix_sound_regram_read(base + 0x00);
	target = wave_control >> 2;

	if (v->rate < target)
		v->rate += 1;
	else if (v->rate > target)
		v->rate -= 1;
}

/***************************************************************************

    Voices

***************************************************************************/

int xavix_sound_is_voice_enabled(int vi)
{
	stream_update(stream, 0);
	return voice[vi].enabled ? 1 : 0;
}

void xavix_sound_enable_voice(int vi, int update_only)
{
	int base = vi * 0x10;
	UINT16 wave_control;
	UINT16 wave_addr;
	UINT16 wave_loop_addr;
	UINT8 wave_addr_bank;
	UINT8 new_type;
	UINT32 new_rate;
	UINT8 env_config;
	UINT16 env_addr_left;
	UINT16 env_addr_right;
	UINT8 env_addr_bank;
	UINT8 tp;

	stream_update(stream, 0);

	/* the wave registers */
	wave_control = (xavix_sound_regram_read(base + 0x1) << 8) | xavix_sound_regram_read(base + 0x0);
	wave_addr = (xavix_sound_regram_read(base + 0x3) << 8) | xavix_sound_regram_read(base + 0x2);
	wave_loop_addr = (xavix_sound_regram_read(base + 0x5) << 8) | xavix_sound_regram_read(base + 0x4);
	wave_addr_bank = xavix_sound_regram_read(base + 0x6);
	new_type = wave_control & 0x3;
	new_rate = wave_control >> 2;

	/* the envelope registers */
	env_config = xavix_sound_regram_read(base + 0x8);
	env_addr_left = (xavix_sound_regram_read(base + 0xb) << 8) | xavix_sound_regram_read(base + 0xa);
	env_addr_right = (xavix_sound_regram_read(base + 0xd) << 8) | xavix_sound_regram_read(base + 0xc);
	env_addr_bank = xavix_sound_regram_read(base + 0xe);

	/* the fields that may be live-tweaked from the RAM are always refreshed */
	voice[vi].vol = env_config & 0x0f;
	voice[vi].env_mode = (env_config >> 4) & 0x03;
	voice[vi].env_bank = env_addr_bank;
	voice[vi].env_rom_base_left = env_addr_left;
	voice[vi].env_rom_base_right = env_addr_right;
	voice[vi].type = new_type;
	voice[vi].rate = new_rate;

	if (new_type == 1 && voice[vi].noise_state == 0)
		voice[vi].noise_state = wave_addr ? wave_addr : 0xace1u;

	if (update_only)
		return;

	/* full (re)start */
	voice[vi].enabled = 1;
	voice[vi].bank = wave_addr_bank;
	voice[vi].position = (UINT32)wave_addr << 14;
	/* the loop address is one sample before */
	voice[vi].loop_position = (wave_loop_addr ? ((UINT32)(wave_loop_addr - 1) << 14) : 0);
	voice[vi].start_position = voice[vi].position;
	voice[vi].noise_state = wave_addr ? wave_addr : 0xace1u;

	/* envelope tempo */
	tp = tempo_div[vi & 3];
	voice[vi].env_period_samples = tp ? envelope_period_ticks(tp) : 0;
	voice[vi].env_countdown = voice[vi].env_period_samples;
	voice[vi].env_active_left = 1;
	voice[vi].env_active_right = 1;

	/* initial envelope values per voice mode */
	if (voice[vi].env_mode == 0)
	{
		voice[vi].env_vol_left = xavix_sound_regram_read(base + 0xa);
		voice[vi].env_vol_right = xavix_sound_regram_read(base + 0xc);
	}
	else if (voice[vi].env_mode == 1)
	{
		voice[vi].env_vol_left = fetch_env_byte_direct(vi, env_addr_left);
		voice[vi].env_vol_right = fetch_env_byte_direct(vi, env_addr_right);

		voice[vi].env_pos_left = advance_nibble(env_addr_left);
		voice[vi].env_pos_right = advance_nibble(env_addr_right);

		/* the low bytes of the pointers are visible in the registers LA and RA */
		xavix_sound_regram_write(base + 0x0a, (UINT8)env_addr_left);
		xavix_sound_regram_write(base + 0x0c, (UINT8)env_addr_right);
	}
	else if (voice[vi].env_mode == 2)
	{
		voice[vi].env_vol_left = fetch_env_byte_direct(vi, env_addr_left);
		voice[vi].env_vol_right = fetch_env_byte_direct(vi, env_addr_right);

		xavix_sound_regram_write(base + 0x0a, (UINT8)env_addr_left);
		xavix_sound_regram_write(base + 0x0c, (UINT8)env_addr_right);

		voice[vi].env_pos_left = (UINT16)(env_addr_left + 1);
		voice[vi].env_pos_right = (UINT16)(env_addr_right + 1);
	}
	else if (voice[vi].env_mode == 3)
	{
		voice[vi].env_vol_left = xavix_sound_regram_read(base + 0xa);
		voice[vi].env_vol_right = xavix_sound_regram_read(base + 0xc);
	}
}

void xavix_sound_disable_voice(int vi)
{
	stream_update(stream, 0);
	voice[vi].enabled = 0;
}

/***************************************************************************

    Mixer registers

***************************************************************************/

UINT8 xavix_sound_volume_get(void)
{
	return mix.mastervol;
}

void xavix_sound_volume_set(UINT8 data)
{
	stream_update(stream, 0);
	mix.mastervol = data;
}

UINT8 xavix_sound_mixer_get(void)
{
	return (mix.monaural ? 0x80 : 0x00) | ((mix.capacity & 0x03) << 4) | (mix.amp & 0x07);
}

void xavix_sound_mixer_set(UINT8 data)
{
	stream_update(stream, 0);
	mix.monaural = (data >> 7) & 1;
	mix.capacity = (data >> 4) & 3;
	mix.amp = data & 7;
	mix.gain = AMP_TABLE[mix.amp & 0x07];
}

UINT8 xavix_sound_dac_control_get(void)
{
	return ((mix.dac & 1) << 7) | ((mix.gap & 3) << 5) | ((mix.lead & 7) << 2) | (mix.lag & 3);
}

void xavix_sound_dac_control_set(UINT8 data)
{
	stream_update(stream, 0);
	mix.dac = (data >> 7) & 1;
	mix.gap = (data >> 5) & 3;
	mix.lead = (data >> 2) & 7;
	mix.lag = data & 3;
}

/***************************************************************************

    Rendering

***************************************************************************/

/* one tick of the sequencer, a sample of both the channels */
static void render_tick(INT32 *out_l, INT32 *out_r)
{
	const UINT8 *visit_order = mix.dac ? MIXER_ORDER_BROADCAST : MIXER_ORDER_MULTIPLEX;
	UINT8 cap = mix.capacity & 0x03;
	int allowed_channels = (cap & 0x02) ? 4 : (cap & 0x01) ? 8 : 16;
	INT64 total_l = 0;
	INT64 total_r = 0;
	int idx;

	/* the voices are visited in the order of the hardware, and the ones past the capacity are dropped */
	for (idx = 0; idx < allowed_channels; ++idx)
	{
		int vi = visit_order[idx];
		struct xavix_voice *v = &voice[vi];
		INT32 sample = 0;
		INT64 base;
		INT64 left;
		INT64 right;

		if (!v->enabled)
			continue;

		if (v->type == 1)
		{
			/* WM1 (noise) */
			UINT16 state = v->noise_state ? v->noise_state : 0xace1u;
			UINT16 feedback = ((state >> 0) ^ (state >> 2) ^ (state >> 3) ^ (state >> 5)) & 1;

			state = (state >> 1) | (feedback << 15);
			if (state == 0)
				state = 0xace1u;
			v->noise_state = state;
			sample = (INT8)(((state >> 8) & 0xff) ^ 0x80);
		}
		else
		{
			UINT32 phase = v->position >> 14;
			UINT8 raw;

			if (v->type == 0)
				raw = 0x80;
			else
				raw = xavix_sound_sample_read(((UINT32)v->bank << 16) | phase);

			if (raw == 0x80)
			{
				sample = 0;
				if (v->type == 3)
				{
					/* one-shot ends */
					v->enabled = 0;
					continue;
				}
				else if (v->type == 2 || v->type == 0)
				{
					if ((v->position >> 14) != (v->loop_position >> 14))
						v->position = v->loop_position;
				}
			}
			else
			{
				/* inverted sign-magnitude to signed PCM */
				UINT8 wv = ((raw & 0x7f) == 0) ? 0x80 : (UINT8)((~raw & 0x80) | (raw & 0x7f));

				sample = (INT32)wv - 128;
			}
		}

		base = (INT64)sample * (INT64)(v->vol & 0x0f);

		left = (base * (INT64)mix.mastervol) / 255;
		right = (base * (INT64)mix.mastervol) / 255;

		total_l += (left * (INT64)v->env_vol_left) / 255;
		total_r += (right * (INT64)v->env_vol_right) / 255;

		v->position += phase_step_per_tick(v->rate);
		step_envelope(vi);
		step_pitch(vi);

		/* a voice with both the sides silent is disabled */
		if (v->env_vol_left == 0 && v->env_vol_right == 0)
			v->enabled = 0;
	}

	/* approximation of the final gain */
	total_l = total_l * mix.gain * allowed_channels * 3;
	total_l = (total_l >= 0) ? ((total_l + 64) >> 7) : -(((-total_l) + 64) >> 7);
	total_r = total_r * mix.gain * allowed_channels * 3;
	total_r = (total_r >= 0) ? ((total_r + 64) >> 7) : -(((-total_r) + 64) >> 7);

	if (total_l > 32767) total_l = 32767; else if (total_l < -32768) total_l = -32768;
	if (total_r > 32767) total_r = 32767; else if (total_r < -32768) total_r = -32768;

	if (mix.monaural)
	{
		INT32 mono = ((INT32)total_l + (INT32)total_r) / 2;

		*out_l = mono;
		*out_r = mono;
	}
	else
	{
		*out_l = (INT32)total_l;
		*out_r = (INT32)total_r;
	}
}

static void xavix_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	stream_sample_t *buffer_l = buffer[0];
	stream_sample_t *buffer_r = buffer[1];
	int n;

	for (n = 0; n < length; ++n)
	{
		INT32 sum_l = 0;
		INT32 sum_r = 0;
		int ticks;
		int t;

		/* the ticks of the sequencer for each output sample are averaged */
		tick_accum += SEQUENCER_RATE_HZ;
		ticks = tick_accum / Machine->sample_rate;
		tick_accum -= ticks * Machine->sample_rate;
		if (ticks == 0)
			ticks = 1;

		for (t = 0; t < ticks; ++t)
		{
			INT32 l;
			INT32 r;

			render_tick(&l, &r);
			sum_l += l;
			sum_r += r;
		}

		buffer_l[n] = sum_l / ticks;
		buffer_r[n] = sum_r / ticks;
	}
}

static void *xavix_sound_start(int clock, const struct CustomSound_interface *config)
{
	stream = stream_create(0, 2, Machine->sample_rate, 0, xavix_sound_update);

	return auto_malloc(1);
}

static void xavix_sound_reset(void *token)
{
	int v;

	cyclerate_div = 0x0f;
	tick_accum = 0;

	/* a tempo other than 0 is required, or the voices with envelope never end */
	for (v = 0; v < 4; ++v)
		xavix_sound_set_tempo(v, tempo_reset_value);

	for (v = 0; v < 16; ++v)
	{
		memset(&voice[v], 0, sizeof(voice[v]));
		voice[v].env_period_samples = envelope_period_ticks(tempo_div[v & 3]);
		voice[v].env_countdown = voice[v].env_period_samples;
		voice[v].env_active_left = 1;
		voice[v].env_active_right = 1;
	}

	mix.monaural = 0;
	mix.capacity = 0;
	mix.amp = 2;
	mix.dac = 0;
	mix.gap = 0;
	mix.lead = 0;
	mix.lag = 0;
	mix.mastervol = 0xff;
	mix.gain = 2;
}

struct CustomSound_interface xavix_sound_interface =
{
	xavix_sound_start,
	0,
	xavix_sound_reset
};

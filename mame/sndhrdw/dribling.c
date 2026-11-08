// license:CC0-1.0
// copyright-holders:Enzo Lombardi
/*
 * Model Racing Dribbling sound circuits.
 * Counter-derived tones and RC envelopes approximate the analog sound board.
 */

#include "driver.h"
#include "sound/custom.h"
#include "streams.h"
#include "dribling.h"
#include <math.h>

struct dribling_sound_state
{
	sound_stream *stream;
	UINT8 data, control, pitch;
	UINT16 noise;
	double rate, tone_phase, noise_phase, whistle_mod;
	double phase[6], envelope[6], decay[6];
	double crowd_filter, output_filter[3], dc_input, dc_output;
	double crowd_alpha, output_alpha, dc_decay;
};

static struct dribling_sound_state *sound_state;

static double oscillator(double *phase, double step)
{
	*phase += step;
	if (*phase >= 1.0)
		*phase -= 1.0;
	return *phase < 0.5 ? 1.0 : -1.0;
}

static void dribling_sound_update(void *param, stream_sample_t **inputs,
	stream_sample_t **outputs, int samples)
{
	struct dribling_sound_state *s = param;
	stream_sample_t *out = outputs[0];
	int sample, i;
	double tone_frequency = 20000.0 / (256 - s->pitch);
	double crowd = ((s->data & 1) ? 256.0 / 420.0 : 0) +
		((s->data & 2) ? 256.0 / 300.0 : 0) + ((s->data & 4) ? 1.0 : 0);

	for (sample = 0; sample < samples; sample++)
	{
		double mix, tone = 0;
		double frequency;

		s->noise_phase += (1.0 / (1.2 * 1000 * 100e-9)) / s->rate;
		while (s->noise_phase >= 1.0)
		{
			int bit = ((s->noise >> 14) ^ s->noise ^ 1) & 1;
			s->noise = (s->noise << 1) | bit;
			s->noise_phase -= 1.0;
		}
		s->crowd_filter += s->crowd_alpha *
			(((s->noise & 0x8000) ? 1.0 : -1.0) - s->crowd_filter);

		/* The output filter rejects the counter's ultrasonic idle frequency. */
		if (tone_frequency < 12000 && tone_frequency < s->rate * 0.45)
			tone = oscillator(&s->tone_phase, tone_frequency / s->rate);
		mix = tone * 300 + s->crowd_filter * crowd * 2200;

		mix += oscillator(&s->phase[0], (1.0 / (2.2 * 56000 * 47e-9)) / s->rate) * s->envelope[0] * 6000;
		mix += oscillator(&s->phase[1], (1.0 / (2.2 * 56000 * 68e-9)) / s->rate) * s->envelope[1] * 6000;
		mix += oscillator(&s->phase[2], 30.0 / s->rate) * s->envelope[2] * 1000;
		mix += oscillator(&s->phase[3], (120 + 400 * s->envelope[3]) / s->rate) * s->envelope[3] * 600;
		mix += oscillator(&s->phase[4], (60 + 300 * s->envelope[4]) / s->rate) * s->envelope[4] * 600;

		s->whistle_mod += (1.0 / (0.693 * 6800 * 4.2e-6)) / s->rate;
		if (s->whistle_mod >= 1.0) s->whistle_mod -= 1.0;
		frequency = (1.44 / ((39000 + 2 * 47000) * 2.2e-9)) *
			(s->whistle_mod < 0.5 ? 0.97 : 1.03);
		if (s->data & 0x10) s->envelope[5] = 1.0;
		mix += oscillator(&s->phase[5], frequency / s->rate) * s->envelope[5] * 300;
		for (i = 0; i < 6; i++) s->envelope[i] *= s->decay[i];

		/* The control transistor shorts the mixer when bit 4 is high. */
		if (s->control & 0x10) mix = 0;
		s->dc_output = mix - s->dc_input + s->dc_decay * s->dc_output;
		s->dc_input = mix;
		s->output_filter[0] += s->output_alpha * (s->dc_output - s->output_filter[0]);
		for (i = 1; i < 3; i++)
			s->output_filter[i] += s->output_alpha * (s->output_filter[i - 1] - s->output_filter[i]);
		*out++ = (stream_sample_t)s->output_filter[2];
	}
}

void dribling_sound_data_w(UINT8 data)
{
	struct dribling_sound_state *s = sound_state;
	int rising, falling;
	if (data == s->data) return;
	stream_update(s->stream, 0);
	rising = data & ~s->data;
	falling = ~data & s->data;
	if (rising & 0x20) s->envelope[0] = 1.0;
	if (rising & 0x08) s->envelope[1] = 1.0;
	if (falling & 0x40) s->envelope[2] = 1.0;
	if (rising & 0x80) s->envelope[3] = 1.0;
	s->data = data;
}

void dribling_sound_pitch_w(UINT8 data)
{
	if (data == sound_state->pitch) return;
	stream_update(sound_state->stream, 0);
	sound_state->pitch = data;
}

void dribling_sound_control_w(UINT8 data)
{
	struct dribling_sound_state *s = sound_state;
	if (!((data ^ s->control) & 0x50))
	{
		s->control = data;
		return;
	}
	stream_update(s->stream, 0);
	if ((data & 0x40) && !(s->control & 0x40)) s->envelope[4] = 1.0;
	s->control = data;
}

static void dribling_sound_reset(void *token)
{
	struct dribling_sound_state *s = token;
	sound_stream *stream = s->stream;
	static const double times[6] = { 0.1176, 0.1176, 0.11, 0.15, 0.484, 0.16 };
	int i;
	memset(s, 0, sizeof(*s));
	s->stream = stream;
	s->rate = Machine->sample_rate ? Machine->sample_rate : 44100;
	s->control = 0x10;
	for (i = 0; i < 6; i++) s->decay[i] = exp(-1.0 / (s->rate * times[i]));
	s->crowd_alpha = 1 - exp(-2 * 3.141592653589793 * 590 / s->rate);
	s->output_alpha = 1 - exp(-2 * 3.141592653589793 * 12000 / s->rate);
	s->dc_decay = exp(-2 * 3.141592653589793 * 7 / s->rate);
}

static void *dribling_sound_start(int clock, const struct CustomSound_interface *config)
{
	sound_state = auto_malloc(sizeof(*sound_state));
	memset(sound_state, 0, sizeof(*sound_state));
	sound_state->stream = stream_create(0, 1, Machine->sample_rate, sound_state, dribling_sound_update);
	dribling_sound_reset(sound_state);
	return sound_state;
}

const struct CustomSound_interface dribling_sound_interface =
{
	dribling_sound_start, NULL, dribling_sound_reset, NULL
};

#include "driver.h"
#include "streams.h"
#include "cpu/i8051/i8051.h"
#include "sound/custom.h"
#include "includes/eolith.h"

/* QS1000 synthesis algorithms by Philip Bennett and R. Belmont. */
#define QS_KEYON 1
#define QS_PLAYING 2
#define QS_ADPCM 4
#define QS_FRAC_BITS 26

static sound_stream *qs_stream;
static const UINT8 *sample_rom;
static UINT8 wave_regs[18], serial_data, qs_p2;
struct qs_adpcm {
	INT32 signal, step, loop_signal, loop_step;
	int saved;
};
static struct qs_voice {
	UINT8 regs[17];
	UINT32 acc, addr, adpcm_addr, loop_start, loop_end;
	float previous_sample;
	INT32 adpcm_signal;
	UINT16 freq, flags, env_addr, env_period;
	UINT32 release_period, env_remaining, env_level;
	INT32 env_step;
	UINT8 env_target;
	struct qs_adpcm adpcm;
} voices[32];

static UINT8 sample_byte(UINT32 address)
{
	return sample_rom[address & 0xffffff];
}

static INT16 qs_adpcm_clock(struct qs_adpcm *a, UINT8 nibble)
{
	static const int steps[89] = {
		7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,
		34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,
		157,173,190,209,230,253,279,307,337,371,408,449,494,544,
		598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,
		2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,
		6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,
		18500,20350,22385,24623,27086,29794,32767
	};
	static const int shift[8] = { -1,-1,-1,-1,2,4,6,8 };
	int step = steps[a->step];
	int delta = (step >> 3) + ((nibble & 1) ? step >> 2 : 0)
		+ ((nibble & 2) ? step >> 1 : 0) + ((nibble & 4) ? step : 0);
	a->signal = (a->signal + ((nibble & 8) ? -delta : delta)) & 0xffff;
	if (a->signal & 0x8000)
		a->signal -= 0x10000;
	a->step += shift[nibble & 7];
	if (a->step < 0) a->step = 0;
	if (a->step > 88) a->step = 88;
	return a->signal;
}

static void set_envelope_target(struct qs_voice *v, UINT8 target, UINT32 period)
{
	v->env_target = target;
	v->env_remaining = period;
	v->env_step = (((INT32)target << 16) - (INT32)v->env_level) / (INT32)period;
}

static void advance_envelope(struct qs_voice *v, UINT32 samples)
{
	while (samples && (v->flags & QS_PLAYING))
	{
		UINT32 count = samples < v->env_remaining ? samples : v->env_remaining;
		v->env_level = (INT32)v->env_level + v->env_step * (INT32)count;
		v->env_remaining -= count;
		samples -= count;
		if (!v->env_remaining)
		{
			v->env_level = (UINT32)v->env_target << 16;
			if (!v->env_target || !(v->flags & QS_KEYON))
				v->flags &= ~QS_PLAYING;
			else
			{
				++v->env_addr;
				set_envelope_target(v, sample_byte(v->env_addr), v->env_period);
			}
		}
	}
}

static void start_voice(struct qs_voice *v)
{
	UINT32 table = (v->regs[1] << 16) | (v->regs[2] << 8) | v->regs[3];
	UINT16 base = (sample_byte(table + 4) << 8) | sample_byte(table + 5);
	UINT8 high = sample_byte(base), format = sample_byte(base + 8), target;
	UINT32 start = (high << 16) | (sample_byte(base + 1) << 8) | sample_byte(base + 2);
	UINT32 loop = ((high & 0xf0) << 16) | (sample_byte(base + 3) << 12)
		| (sample_byte(base + 4) << 4) | (sample_byte(base + 5) >> 4);
	UINT32 end = ((high & 0xf0) << 16) | ((sample_byte(base + 5) & 15) << 16)
		| (sample_byte(base + 6) << 8) | sample_byte(base + 7);
	int adpcm = format & 8;
	int no_loop = adpcm && loop == end;

	v->flags = 0;
	v->freq = (sample_byte(table) << 8) | sample_byte(table + 1);
	if (!v->freq)
		return;
	if (adpcm)
	{
		start <<= 1;
		loop = (loop << 1) | (format & 1);
		end = (end << 1) | ((format >> 1) & 1);
	}
	if (end < start)
		return;
	v->acc = 0;
	v->previous_sample = 0;
	v->addr = start;
	v->loop_start = loop >= start && !no_loop ? loop : end + 1;
	v->loop_end = end;
	v->flags = QS_PLAYING | QS_KEYON;
	v->env_addr = (sample_byte(base + 12) << 8) | sample_byte(base + 13);
	v->env_period = ((UINT16)sample_byte(base + 9) + 1) * 128;
	v->release_period = ((UINT32)sample_byte(base + 10) + 1) * 768 * 16;
	v->env_level = (UINT32)sample_byte(base + 11) << 16;
	target = sample_byte(v->env_addr);
	if (target == sample_byte(base + 11))
		target = sample_byte(++v->env_addr);
	set_envelope_target(v, target, v->env_period);
	if (adpcm)
	{
		memset(&v->adpcm, 0, sizeof(v->adpcm));
		v->adpcm_addr = start - 1;
		v->adpcm_signal = 0;
		v->flags |= QS_ADPCM;
	}
}

WRITE8_HANDLER( eolith_qs_wave_w )
{
	struct qs_voice *v = &voices[wave_regs[14] & 31];
	int voice_page = wave_regs[17] == 3;
	stream_update(qs_stream, 0);
	if (offset == 0)
	{
		if (!voice_page)
			return;
		if (data == 0)
			v->flags = 0;
		else if (data == 2 && (v->flags & QS_KEYON))
		{
			UINT32 period = (UINT64)v->env_level * v->release_period / (255 << 16);
			v->flags &= ~QS_KEYON;
			set_envelope_target(v, 0, period ? period : 1);
		}
	}
	else if (offset == 14)
	{
		wave_regs[offset] = data;
		if (!voice_page)
		{
			v = &voices[data & 31];
			memcpy(v->regs, wave_regs, sizeof(v->regs));
			start_voice(v);
		}
	}
	else if (offset == 17 || !voice_page)
		wave_regs[offset] = data;
	else
		v->regs[offset] = data;
}

static void qs_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int ch, i;
	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	memset(outputs[1], 0, samples * sizeof(*outputs[1]));
	for (ch = 0; ch < 32; ++ch)
	{
		struct qs_voice *v = &voices[ch];
		float gain = (float)v->regs[8] * v->regs[13] / (127.0f * 4096.0f);
		float left = v->regs[7] * gain, right = v->regs[6] * gain;
		UINT8 pitch = v->regs[4];
		UINT32 freq = v->freq * ((pitch & 0x80) ? 256 + 2 * (pitch & 127) : 256 - pitch);
		for (i = 0; i < samples && (v->flags & QS_PLAYING); ++i)
		{
			float value, fraction, interpolated, envelope;
			UINT32 count;
			if (v->addr > v->loop_end)
			{
				if (v->loop_start > v->loop_end)
				{
					v->flags &= ~QS_PLAYING;
					break;
				}
				v->addr = v->loop_start + (v->addr - v->loop_end - 1) % (v->loop_end - v->loop_start + 1);
				if (v->flags & QS_ADPCM)
				{
					v->adpcm.signal = v->adpcm.loop_signal;
					v->adpcm.step = v->adpcm.loop_step;
					v->adpcm_addr = v->loop_start - 1;
				}
			}
			if (v->flags & QS_ADPCM)
			{
				while (v->adpcm_addr != v->addr)
				{
					UINT8 packed, nibble;
					++v->adpcm_addr;
					if (v->adpcm_addr == v->loop_start && !v->adpcm.saved)
					{
						v->adpcm.loop_signal = v->adpcm.signal;
						v->adpcm.loop_step = v->adpcm.step;
						v->adpcm.saved = 1;
					}
					packed = sample_byte(v->adpcm_addr >> 1);
					nibble = (v->adpcm_addr & 1) ? packed & 15 : packed >> 4;
					v->adpcm_signal = qs_adpcm_clock(&v->adpcm, nibble);
				}
				value = v->adpcm_signal / 64.0f;
			}
			else
				value = sample_byte(v->addr) - 128;
			fraction = (float)v->acc / (1U << QS_FRAC_BITS);
			interpolated = v->previous_sample + (value - v->previous_sample) * fraction;
			envelope = (float)v->env_level / (255 << 16);
			outputs[0][i] += interpolated * left * envelope;
			outputs[1][i] += interpolated * right * envelope;
			v->acc += freq;
			count = v->acc >> QS_FRAC_BITS;
			if (count)
				v->previous_sample = value;
			v->addr += count;
			v->acc &= (1U << QS_FRAC_BITS) - 1;
			advance_envelope(v, (v->flags & QS_KEYON) ? count : 1);
		}
	}
}

static void *qs_start(int clock, const struct CustomSound_interface *config)
{
	sample_rom = memory_region(REGION_SOUND2);
	qs_stream = stream_create(0, 2, clock / 32, 0, qs_update);
	return qs_stream;
}

struct CustomSound_interface eolith_sound_interface = { qs_start, 0, 0, 0 };

WRITE32_HANDLER( eolith_sound_w )
{
	if (!(mem_mask & 0xff))
	{
		soundlatch_w(0, data & 0xff);
		cpunum_set_input_line(1, I8051_INT0_LINE, ASSERT_LINE);
		cpu_boost_interleave(0, TIME_IN_USEC(250));
	}
}
READ8_HANDLER( eolith_sound_cmd_r )
{
	cpunum_set_input_line(1, I8051_INT0_LINE, CLEAR_LINE);
	return soundlatch_r(0);
}
WRITE8_HANDLER( eolith_sound_bank_w )
{
	memory_set_bankptr(1, memory_region(REGION_SOUND1) + (data & 15) * 0x8000);
}
READ8_HANDLER( eolith_qs_port_r )
{
	return 1;
}

static int serial_rx(void)
{
	return serial_data;
}
static void serial_tx(int data)
{
	serial_data = data;
	cpunum_set_input_line(2, I8051_RX_LINE, ASSERT_LINE);
	cpunum_set_input_line(2, I8051_RX_LINE, CLEAR_LINE);
	cpu_boost_interleave(0, TIME_IN_USEC(250));
}

WRITE8_HANDLER( eolith_qs_p2_w )
{
	qs_p2 = data;
}
static READ32_HANDLER( qs_external_address )
{
	return mem_mask == 0xff ? (qs_p2 << 8) | offset : offset;
}

static void configure_cpus(int param)
{
	cpuintrf_push_context(1);
	i8051_set_serial_tx_callback(serial_tx);
	cpuintrf_pop_context();
	cpuintrf_push_context(2);
	i8051_set_serial_rx_callback(serial_rx);
	i8051_set_eram_iaddr_callback(qs_external_address);
	cpuintrf_pop_context();
	cpunum_set_input_line(1, I8051_INT1_LINE, ASSERT_LINE);
}

void eolith_sound_reset(void)
{
	memset(voices, 0, sizeof(voices));
	memset(wave_regs, 0, sizeof(wave_regs));
	serial_data = 0;
	qs_p2 = 0xff;
	memory_set_bankptr(1, memory_region(REGION_SOUND1));
	timer_set(TIME_NOW, 0, configure_cpus);
}

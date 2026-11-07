// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/* QDSP QS1000 wavetable synthesis. */

#include "driver.h"
#include "streams.h"
#include "sound/custom.h"
#include "sound/okim6295.h"

#define QS_PLAYING 1
#define QS_KEYON 2
#define QS_ADPCM 4

static struct qs_voice
{
	UINT8 regs[16];
	UINT32 addr, start, end, acc, adpcm_addr;
	UINT16 freq;
	int flags, signal;
	struct adpcm_state adpcm;
} voices[32];
static UINT8 wave_regs[0x12];
static sound_stream *qs_stream;
static const UINT8 *sample_rom;

static UINT8 sample_byte(UINT32 address)
{
	return sample_rom[address & 0xffffff];
}

static void start_voice(struct qs_voice *v)
{
	UINT32 table = (v->regs[1] << 16) | (v->regs[2] << 8) | v->regs[3];
	UINT16 base = (sample_byte(table + 4) << 8) | sample_byte(table + 5);
	UINT8 high = sample_byte(base);
	v->flags = 0;
	v->freq = (sample_byte(table) << 8) | sample_byte(table + 1);
	if (!v->freq)
		return;
	v->start = (high << 16) | (sample_byte(base + 1) << 8) | sample_byte(base + 2);
	v->end = ((high & 0xf0) << 16) | ((sample_byte(base + 5) & 15) << 16)
		| (sample_byte(base + 6) << 8) | sample_byte(base + 7);
	v->addr = v->start;
	v->acc = 0;
	v->flags = QS_PLAYING | QS_KEYON;
	if (sample_byte(base + 8) & 8)
	{
		v->flags |= QS_ADPCM;
		v->adpcm_addr = 0xffffffff;
		v->signal = 0;
		reset_adpcm(&v->adpcm);
	}
}

WRITE8_HANDLER( limenko_wave_w )
{
	struct qs_voice *v = &voices[wave_regs[14] & 31];
	stream_update(qs_stream, 0);
	if (offset == 0)
	{
		/* The trigger commits the complete descriptor to the selected voice. */
		if (data == 0)
		{
			memcpy(v->regs, wave_regs, sizeof(v->regs));
			start_voice(v);
		}
		else if (data == 2)
			v->flags &= ~QS_KEYON;
	}
	else if (offset <= 13 && wave_regs[17] == 3)
		v->regs[offset] = data;
	else
		wave_regs[offset] = data;
}

static void qs_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int ch, i;
	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	memset(outputs[1], 0, samples * sizeof(*outputs[1]));
	for (ch = 0; ch < 32; ++ch)
	{
		struct qs_voice *v = &voices[ch];
		int left, right;
		if (!(v->flags & QS_PLAYING))
			continue;
		left = v->regs[6] * v->regs[8];
		right = v->regs[7] * v->regs[8];
		for (i = 0; i < samples; ++i)
		{
			int value;
			if (v->addr >= v->end)
			{
				v->flags &= ~QS_PLAYING;
				break;
			}
			if (v->flags & QS_ADPCM)
			{
				while (v->start + v->adpcm_addr != v->addr)
				{
					UINT8 packed;
					++v->adpcm_addr;
					packed = sample_byte(v->start + (v->adpcm_addr >> 1));
					v->signal = clock_adpcm(&v->adpcm,
						((v->adpcm_addr & 1) ? packed : packed >> 4) & 15);
				}
				value = (INT8)(v->signal >> 4) * 4;
			}
			else
				value = sample_byte(v->addr) - 128;
			outputs[0][i] += value * left / 4096;
			outputs[1][i] += value * right / 4096;
			v->acc += v->freq;
			v->addr = (v->addr + (v->acc >> 18)) & 0xffffff;
			v->acc &= (1 << 18) - 1;
		}
	}
}

static void *qs_start(int clock, const struct CustomSound_interface *config)
{
	sample_rom = memory_region(REGION_SOUND2);
	qs_stream = stream_create(0, 2, clock / 32, 0, qs_update);
	return qs_stream;
}

void limenko_sound_reset(void)
{
	memset(voices, 0, sizeof(voices));
	memset(wave_regs, 0, sizeof(wave_regs));
}

struct CustomSound_interface limenko_sound_interface = { qs_start, 0, 0, 0 };

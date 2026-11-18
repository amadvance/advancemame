// license:BSD-3-Clause
// copyright-holders:R. Belmont, O. Galibert
/* L7A1045 L6028 DSP-A */

#include "driver.h"
#include "includes/hng64.h"
#include "sound/custom.h"
#include "streams.h"

enum
{
	L6028_Start = 0,
	L6028_End,
	L6028_Loop_Start,
	L6028_Volume_Env,
	L6028_Volume_Env_Target,
	L6028_Filter_Env,
	L6028_Filter_Env_Target,
	L6028_Mixer_Params
};

static const int CONTROL_DMA_START      = 6;
static const int CONTROL_KEY_ON         = 8;

struct l6028_voice
{

	UINT32 loop_start;
	UINT32 start;
	UINT32 end;
	UINT32 step;
	UINT32 pos;
	UINT32 frac;
	UINT16 l_volume;
	UINT16 r_volume;
	UINT16 env_volume;
	UINT16 env_target;
	UINT16 env_step;
	UINT32 env_pos;
	UINT16 flt_freq;
	UINT16 flt_target;
	UINT16 flt_step;
	UINT32 flt_pos;
	UINT8 flt_resonance;
	INT32 b, l;
	UINT8 send_dest;
	UINT8 send_level;
	UINT8 sample_type;
};


static struct l6028_voice m_voice[32];
static sound_stream *m_stream;
static UINT32 m_key;
static UINT8 m_cur_channel, m_cur_register;
static UINT64 m_regs[16][32];
static UINT16 sample_ram[0x10000];
static const UINT8 *sample_rom;
static const int channel_remap[8] = { 3, 1, 7, 5, 2, 0, 6, 4 };

static void recalc_loop_start(struct l6028_voice *vptr);

static UINT16 l6028_read_sample(UINT32 address)
{
	return address < 0x20000 ? sample_ram[address >> 1] : 0;
}

static void l6028_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int output;
	for (output = 0; output < 10; ++output)
		memset(outputs[output], 0, samples * sizeof(outputs[output][0]));

	for (int i = 0; i < ARRAY_LENGTH(m_voice); i++)
	{
		if (m_key & ((UINT32)1 << i))
		{
			struct l6028_voice *vptr = &m_voice[i];

			const UINT32 start = vptr->start;
			const UINT32 end = vptr->end;
			const UINT32 step  = vptr->step;

			UINT32 pos = vptr->pos;
			UINT32 frac = vptr->frac;

			for (int j = 0; j < samples; j++)
			{
				UINT32 address;
				INT32 sample;
				UINT8 data;

				pos += (frac >> 12);
				frac &= 0xfff;

				if ((end > start) && ((start + pos) >= end))
				{
					pos = (end - start) - MIN(end - start, vptr->loop_start);
				}

				switch (vptr->sample_type)
				{
					case 0:
						address = ((start << 1) + (pos << 1));
						sample = (INT16)l6028_read_sample(address);
						break;

					case 1:
						address = (start + pos);
						data = sample_rom[address & 0xffffff];
						sample = (data & 0xfc) >> 2;
						if (sample & 0x20)
							sample -= 0x40;
						sample *= 1 << (4 + 2 * (~data & 3));
						break;

					default:

						sample = 0;
						break;
				}

				frac += step;

				vptr->env_pos += vptr->env_step;
				const int steps = ((UINT32)vptr->env_pos / 0x100);
				if (steps > 0)
				{
					if (vptr->env_volume < vptr->env_target)
					{
						vptr->env_volume += MIN(steps, (vptr->env_target - vptr->env_volume));
					}
					else if (vptr->env_volume > vptr->env_target)
					{
						vptr->env_volume -= MIN(steps, (vptr->env_volume - vptr->env_target));
					}
				}
				vptr->env_pos &= 0xff;

				vptr->flt_pos += vptr->flt_step;
				const int flt_steps = ((UINT32)vptr->flt_pos / 0x100);
				if (flt_steps > 0)
				{
					if (vptr->flt_freq < vptr->flt_target)
					{
						vptr->flt_freq += MIN(flt_steps, (vptr->flt_target - vptr->flt_freq));
					}
					else if (vptr->flt_freq > vptr->flt_target)
					{
						vptr->flt_freq -= MIN(flt_steps, (vptr->flt_freq - vptr->flt_target));
					}
				}
				vptr->flt_pos &= 0xff;

				const INT32 h = sample - vptr->l - vptr->b + (((INT64)vptr->flt_resonance * vptr->b) >> 4);
				vptr->b += ((INT64)vptr->flt_freq * h) >> 15;
				vptr->l += ((INT64)vptr->flt_freq * vptr->b) >> 15;

				const INT32 fout = vptr->l;
				const INT64 left = (fout * (((INT64)(vptr->l_volume)) * ((INT64)(vptr->env_volume)))) >> 24;
				const INT64 right = (fout * (((INT64)(vptr->r_volume)) * ((INT64)(vptr->env_volume)))) >> 24;
				outputs[0][j] += left;
				outputs[1][j] += right;

				if (vptr->send_level > 0)
				{
					const int dest = vptr->send_dest & 0xf;
					if (dest < 8)
					{
						const INT64 send = (fout * (((INT64)(vptr->send_level)) * ((INT64)(vptr->env_volume)))) >> 24;
						outputs[2 + channel_remap[dest]][j] += send;
					}
				}
			}

			vptr->pos = pos;
			vptr->frac = frac;
		}
	}
}

static void voice_select_w(UINT16 data, UINT16 mem_mask)
{
	stream_update(m_stream, 0);
	if (ACCESSING_LSB)
		m_cur_channel = data & 31;
	if (ACCESSING_MSB)
		m_cur_register = (data >> 8) & 15;
}

static UINT16 voiceregs_r(offs_t offset)
{
	const struct l6028_voice *vptr = &m_voice[m_cur_channel];

	stream_update(m_stream, 0);

	switch (m_cur_register)
	{
	case L6028_Start:
	{
		const UINT32 current_addr = vptr->start + vptr->pos;

		m_regs[0][m_cur_channel] &= 0xfff000000000;
		m_regs[0][m_cur_channel] |= (((UINT64)(current_addr)) << 12);
		m_regs[0][m_cur_channel] |= vptr->frac & 0x0fff;

	}
	break;

	case L6028_Volume_Env:
		m_regs[3][m_cur_channel] &= 0xffff0000ffff;
		m_regs[3][m_cur_channel] |= (((INT64)(vptr->env_volume)) << 16);

		break;

	case L6028_Filter_Env:
		m_regs[5][m_cur_channel] &= 0xffff0000ffff;
		m_regs[5][m_cur_channel] |= (((INT64)(vptr->flt_freq)) << 16);

		break;
	}

	return (m_regs[m_cur_register][m_cur_channel] >> (offset * 16)) & 0xffff;
}

static void voiceregs_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	struct l6028_voice* const vptr = &m_voice[m_cur_channel];
	const UINT64 offset_mask[3] = { 0xffffffff0000ULL, 0xffff0000ffffULL, 0x0000ffffffffULL };

	stream_update(m_stream, 0);

	data |= (m_regs[m_cur_register][m_cur_channel] >> (offset * 16)) & mem_mask;
	m_regs[m_cur_register][m_cur_channel] &= offset_mask[offset];
	m_regs[m_cur_register][m_cur_channel] |= (UINT64)data << (offset * 16);

	switch (m_cur_register)
	{

		case L6028_Start:
			vptr->start = (m_regs[L6028_Start][m_cur_channel] >> 12) & 0x00ffffff;
			vptr->sample_type = (m_regs[L6028_Start][m_cur_channel] >> 36) & 0xf;

			vptr->pos = 0;
			vptr->frac = 0;

			vptr->flt_pos = 0;
			vptr->l = vptr->b = 0;

			if (offset == 2)
			{
				m_regs[L6028_Loop_Start][m_cur_channel] = 0;
			}
			break;

		case L6028_End:
			vptr->end = (m_regs[L6028_End][m_cur_channel] >> 12) & 0x00fffff0;

			vptr->step = m_regs[1][m_cur_channel] & 0xffff;
			if (offset == 2)
			{
				recalc_loop_start(vptr);
			}
			break;

		case L6028_Loop_Start:
			recalc_loop_start(vptr);
			break;

		case L6028_Volume_Env:
			vptr->env_volume = (m_regs[L6028_Volume_Env][m_cur_channel] & 0xffff0000) >> 16;
			vptr->env_pos = 0;

			if (offset == 0 && ACCESSING_MSB && data == 0)
			{
				m_key &= ~((UINT32)1 << m_cur_channel);
			}
			break;

		case L6028_Volume_Env_Target:
			vptr->env_target = (m_regs[L6028_Volume_Env_Target][m_cur_channel] & 0xffff0000) >> 16;
			vptr->env_step = m_regs[L6028_Volume_Env_Target][m_cur_channel] & 0xffff;

			break;

		case L6028_Filter_Env:
			if (vptr->flt_pos == 0)
			{
				vptr->flt_freq = (m_regs[L6028_Filter_Env][m_cur_channel] & 0xffff0000) >> 16;
			}
			break;

		case L6028_Filter_Env_Target:
			vptr->flt_target = (m_regs[L6028_Filter_Env_Target][m_cur_channel] & 0xfff00000) >> 16;
			vptr->flt_resonance = (m_regs[L6028_Filter_Env_Target][m_cur_channel] & 0x000f0000) >> 16;
			vptr->flt_step = m_regs[6][m_cur_channel] & 0xffff;
			break;

		case L6028_Mixer_Params:
			vptr->r_volume = (m_regs[L6028_Mixer_Params][m_cur_channel] & 0xff);
			vptr->l_volume = (m_regs[L6028_Mixer_Params][m_cur_channel] >> 8) & 0xff;
			vptr->send_dest = (m_regs[L6028_Mixer_Params][m_cur_channel] >> 16) & 0xff;
			vptr->send_level = (m_regs[L6028_Mixer_Params][m_cur_channel] >> 24) & 0xff;
			break;
	}
}

static void recalc_loop_start(struct l6028_voice *vptr)
{
	if ((m_regs[L6028_End][m_cur_channel] & ((UINT64)1 << 40)))
	{
		const UINT32 length = vptr->end - vptr->start;

		vptr->loop_start = (m_regs[L6028_Loop_Start][m_cur_channel] & 0xfffff000) >> 12;
		vptr->loop_start |= (m_regs[L6028_Loop_Start][m_cur_channel] & 0x000f) << 20;

		vptr->loop_start = vptr->end - vptr->loop_start;
		if (vptr->loop_start > length)
		{
			vptr->loop_start = length;
		}
	}
	else
	{
		const UINT32 multiplier = (((m_regs[L6028_Loop_Start][m_cur_channel] & 0xffff0000) >> 16) ^ 0xffff) + 1;
		const UINT32 base = m_regs[2][m_cur_channel] & 0xffff;
		vptr->loop_start = (base * multiplier) >> 12;
	}
}

static void control_w(UINT16 data, UINT16 mem_mask)
{
	stream_update(m_stream, 0);

	if ((data & ~mem_mask & (1 << CONTROL_KEY_ON)))
	{
		struct l6028_voice* const vptr = &m_voice[m_cur_channel];

		vptr->frac = 0;
		vptr->pos = 0;
		m_key |= (UINT32)1 << m_cur_channel;

		recalc_loop_start(vptr);

	}

	if (ACCESSING_LSB)
		hng64_sound_drq((data >> CONTROL_DMA_START) & 1);
}

static void atomic_w(void)
{

	m_regs[m_cur_register][m_cur_channel] = 0;
}

UINT16 hng64_dsp_dma_r(void)
{
	const offs_t byteoffs = (m_voice[0].start << 1) + (m_voice[0].pos << 1);

	m_voice[0].pos++;
	if (m_voice[0].sample_type == 1)
	{

		return sample_rom[byteoffs & 0xfffffe] | (sample_rom[(byteoffs & 0xfffffe) + 1] << 8);
	}

	return l6028_read_sample(byteoffs);
}

void hng64_dsp_dma_w(UINT16 data)
{
	const offs_t byteoffs = (m_voice[0].start << 1) + (m_voice[0].pos << 1);

	if (m_voice[0].sample_type != 1 && byteoffs < 0x20000)
		sample_ram[byteoffs >> 1] = data;

	m_voice[0].pos++;
}


READ8_HANDLER( hng64_dsp_r )
{
	if (offset >= 2 && offset <= 7)
		return voiceregs_r((offset >> 1) - 1) >> (8 * (offset & 1));
	return 0;
}

WRITE8_HANDLER( hng64_dsp_w )
{
	int shift = (offset & 1) * 8;
	UINT16 mask = ~(0xff << shift);
	UINT16 value = data << shift;

	switch (offset >> 1)
	{
		case 0: voice_select_w(value, mask); break;
		case 1:
		case 2:
		case 3: voiceregs_w((offset >> 1) - 1, value, mask); break;
		case 4: control_w(value, mask); break;
		case 6: atomic_w(); break;
	}
}

static void *l6028_start(int clock, const struct CustomSound_interface *config)
{
	m_stream = stream_create(0, 10, clock / 768, NULL, l6028_update);
	sample_rom = memory_region(REGION_SOUND1);
	return m_stream;
}

static void l6028_reset(void *token)
{
	m_key = 0;
	m_cur_channel = m_cur_register = 0;
	memset(m_voice, 0, sizeof(m_voice));
	memset(m_regs, 0, sizeof(m_regs));
}

const struct CustomSound_interface hng64_dsp = { l6028_start, NULL, l6028_reset, NULL };

// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont
/*
license:BSD-3-Clause
copyright-holders:Olivier Galibert, R. Belmont
***************************************************************************

    ymz770.c

    Emulation by R. Belmont
    AMM decode by Olivier Galibert

-----
TODO:
- A lot of unimplemented features, even simple ones like panning,
  these should be added once we find out any software that uses it.
- Is channel volume linear(current implementation) or logarithmic?
- Sequencer is very preliminary
- What does channel ATBL mean?
- Is YMZ774(and other variants) the same family as this chip?
  What are the differences?

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "sound/custom.h"
#include "cv1k_mpeg.h"
#include "includes/cv1k.h"
typedef struct ymz_channel
	{
		UINT8 phrase;
		UINT8 pan;
		UINT8 volume;
		UINT8 control;

		int is_playing, last_block;

		mpeg_audio decoder;

		INT16 output_data[0x1000];
		int output_remaining;
		int output_ptr;
		int atbl;
		int pptr;

		UINT8 sequence;
		UINT8 seqcontrol;
		UINT8 seqdelay;
		UINT8 *seqdata;
		int is_seq_playing;
	} ymz_channel;

static UINT8 m_cur_reg;
static UINT8 m_mute;
static UINT8 m_doen;
static UINT8 m_vlma;
static UINT8 m_bsl;
static UINT8 m_cpl;
static UINT8 *m_rom_base;
static int m_rom_limit;
static ymz_channel m_channels[8];

static sound_stream *m_stream;
static void internal_reg_write(UINT8 reg, UINT8 data);
static void ymz_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
 int i, ch;
	stream_sample_t *outL, *outR;

	outL = outputs[0];
	outR = outputs[1];

	for (i = 0; i < samples; i++)
	{

		for (ch = 0; ch < 8; ch++)
		{
			if (m_channels[ch].is_seq_playing)
			{
				if (m_channels[ch].seqdelay > 0)
				{
					m_channels[ch].seqdelay--;
				}
				else
				{
					int reg = *m_channels[ch].seqdata++;
					UINT8 data = *m_channels[ch].seqdata++;
					switch (reg)
					{
						case 0x0f:
							if (m_channels[ch].seqcontrol & 1)
							{

								UINT8 sqn = m_channels[ch].sequence;
								UINT32 pptr = m_rom_base[(4*sqn)+1+0x400]<<16 | m_rom_base[(4*sqn)+2+0x400]<<8 | m_rom_base[(4*sqn)+3+0x400];
								m_channels[ch].seqdata = &m_rom_base[pptr];
							}
							else
							{
								m_channels[ch].is_seq_playing = 0;
							}
							break;
						case 0x0e:
							m_channels[ch].seqdelay = 32 - 1;
							break;
						default:
							internal_reg_write(reg, data);
							break;
					}
				}
			}
		}

		INT32 mix = 0;

		for (ch = 0; ch < 8; ch++)
		{
			if (m_channels[ch].output_remaining > 0)
			{

				mix += (m_channels[ch].output_data[m_channels[ch].output_ptr++]*m_channels[ch].volume);
				m_channels[ch].output_remaining--;

				if (m_channels[ch].output_remaining == 0 && !m_channels[ch].is_playing)
					cv1k_mpeg_clear(&m_channels[ch].decoder);
			}

			else if (m_channels[ch].is_playing)
			{
retry:
				if (m_channels[ch].last_block)
				{
					if (m_channels[ch].control & 1)
					{

						UINT8 phrase = m_channels[ch].phrase;
						m_channels[ch].atbl = m_rom_base[(4*phrase)+0] >> 4 & 7;
						m_channels[ch].pptr = 8*(m_rom_base[(4*phrase)+1]<<16 | m_rom_base[(4*phrase)+2]<<8 | m_rom_base[(4*phrase)+3]);
					}
					else
					{
						m_channels[ch].is_playing = 0;
						m_channels[ch].output_remaining = 0;
						cv1k_mpeg_clear(&m_channels[ch].decoder);
					}
				}

				if (m_channels[ch].is_playing)
				{

					int sample_rate, channel_count;
					if (!cv1k_mpeg_decode(&m_channels[ch].decoder, &m_channels[ch].pptr, m_rom_limit, m_channels[ch].output_data, &m_channels[ch].output_remaining, &sample_rate, &channel_count) || m_channels[ch].output_remaining == 0)
					{
						m_channels[ch].is_playing = !m_channels[ch].last_block;
						m_channels[ch].last_block = 1;
						m_channels[ch].output_remaining = 0;
						goto retry;
					}

					m_channels[ch].last_block = m_channels[ch].output_remaining < 1152;
					m_channels[ch].output_remaining--;
					m_channels[ch].output_ptr = 1;

					mix += (m_channels[ch].output_data[0]*m_channels[ch].volume);
				}
			}
		}

		outL[i] = outR[i] = mix>>8;
	}
}

void cv1k_ymz_write(int offset, UINT8 data)
{
	if (offset & 1)
	{
		stream_update(m_stream, 0);
		internal_reg_write(m_cur_reg, data);
	}
	else
	{
		m_cur_reg = data;
	}
}

void internal_reg_write(UINT8 reg, UINT8 data)
{

	if (reg < 0x40)
	{
		switch (reg)
		{
			case 0x00:
				m_mute = data & 1;
				m_doen = data >> 1 & 1;
				break;

			case 0x01:
				m_vlma = data;
				break;

			case 0x02:
				m_bsl = data & 7;
				m_cpl = data >> 4 & 7;
				break;

			default:
				break;
		}
	}

	else if (reg < 0x60)
	{
		int ch = reg >> 2 & 0x07;

		switch (reg & 0x03)
		{
			case 0:
				m_channels[ch].phrase = data;
				break;

			case 1:
				m_channels[ch].volume = data;
				break;

			case 2:
				m_channels[ch].pan = data;
				break;

			case 3:
				if (data & 6)
				{
					UINT8 phrase = m_channels[ch].phrase;
					m_channels[ch].atbl = m_rom_base[(4*phrase)+0] >> 4 & 7;
					m_channels[ch].pptr = 8*(m_rom_base[(4*phrase)+1]<<16 | m_rom_base[(4*phrase)+2]<<8 | m_rom_base[(4*phrase)+3]);
					m_channels[ch].last_block = 0;

					m_channels[ch].is_playing = 1;
				}
				else
				{
					m_channels[ch].is_playing = 0;
				}

				m_channels[ch].control = data;
				break;
		}
	}

	else
	{
		int ch = reg >> 4 & 0x07;

		switch (reg & 0x0f)
		{
			case 0:
				m_channels[ch].sequence = data;
				break;
			case 1:
				if (data & 6)
				{
					UINT8 sqn = m_channels[ch].sequence;
					UINT32 pptr = m_rom_base[(4*sqn)+1+0x400]<<16 | m_rom_base[(4*sqn)+2+0x400]<<8 | m_rom_base[(4*sqn)+3+0x400];
					m_channels[ch].seqdata = &m_rom_base[pptr];
					m_channels[ch].seqdelay = 0;
					m_channels[ch].is_seq_playing = 1;
				}
				else
				{
					m_channels[ch].is_seq_playing = 0;
				}

				m_channels[ch].seqcontrol = data;
				break;

			default:
				break;
		}
	}
}

void *cv1k_ymz_start(int clock, const struct CustomSound_interface *config)
{
 int i;
 memset(m_channels, 0, sizeof(m_channels));
 m_rom_base = memory_region(REGION_SOUND1);
 m_rom_limit = memory_region_length(REGION_SOUND1) * 8;
 for (i = 0; i < 8; ++i) {
  m_channels[i].pan = 8;
  cv1k_mpeg_init(&m_channels[i].decoder, m_rom_base);
 }
 m_cur_reg = m_mute = m_doen = m_vlma = m_bsl = m_cpl = 0;
 m_stream = stream_create(0, 2, 16000, NULL, ymz_update);
 return m_channels;
}

void cv1k_ymz_reset(void *token)
{
 int ch;
 for (ch = 0; ch < 8; ++ch) {
  m_channels[ch].phrase = 0;
  m_channels[ch].pan = 8;
  m_channels[ch].volume = 0;
  m_channels[ch].control = 0;
  m_channels[ch].sequence = 0;
  m_channels[ch].seqcontrol = 0;
  m_channels[ch].seqdelay = 0;
  m_channels[ch].is_playing = 0;
  m_channels[ch].is_seq_playing = 0;
  m_channels[ch].output_remaining = 0;
  cv1k_mpeg_clear(&m_channels[ch].decoder);
 }
}

// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, MetalliC
/* Yamaha YMZ774, converted from MAME 0.287 to C. */
#include "driver.h"
#include "streams.h"
#include "sound/custom.h"
#include "ymz774.h"
#include "pgm2_mpeg.h"
	typedef struct ymz_channel
	{
		uint16_t phrase;
		uint8_t pan;
		uint8_t pan_delay;
		uint8_t pan1;
		uint8_t pan1_delay;
		int32_t volume;
		uint8_t volume_target;
		uint8_t volume_delay;
		uint8_t volume2;
		uint8_t loop;

		bool is_playing, last_block, is_paused;

		pgm2_mpeg decoder;

		int16_t output_data[0x1000];
		int output_remaining;
		int output_ptr;
		int atbl;
		int pptr;
	} ymz_channel;
	typedef struct ymz_sequence
	{
		uint32_t delay;
		uint16_t sequence;
		uint16_t timer;
		uint16_t stopchan;
		uint8_t loop;
		uint32_t offset;
		uint8_t bank;
		bool is_playing;
		bool is_paused;
	} ymz_sequence;
	typedef struct ymz_sqc
	{
		uint8_t sqc;
		uint8_t loop;
		uint32_t offset;
		bool is_playing;
		bool is_waiting;
	} ymz_sqc;

static ymz_channel m_channels[16];
static ymz_sequence m_sequences[8];
static ymz_sqc m_sqcs[8];
static sound_stream *m_stream;
static UINT8 *m_rom;
static int m_rom_size, m_sclock, m_bank;
static UINT8 m_cur_reg, m_mute, m_doen, m_vlma, m_vlma1, m_bsl, m_cpl;
static UINT32 volinc[256];
static void sequencer(void);
static void internal_reg_write(uint8_t reg, uint8_t data);
static int ymz_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static UINT8 get_rom_byte(UINT32 offset) { return m_rom[offset % m_rom_size]; }
static UINT32 get_phrase_offs(int phrase) { int p = phrase * 4; return ((get_rom_byte(p) & 15) << 24 | get_rom_byte(p+1) << 16 | get_rom_byte(p+2) << 8 | get_rom_byte(p+3)) * 2; }
static UINT32 get_seq_offs(int sq) { return get_phrase_offs(sq + 0x800); }
static UINT32 get_sqc_offs(int sq) { return get_phrase_offs(sq + 0x1800); }
void ymz774_reset(void)
{
	for (int idx = 0; idx < 16; idx++)
	{
		ymz_channel *channel = &m_channels[idx];
		channel->phrase = 0;
		channel->pan = 64;
		channel->pan_delay = 0;
		channel->pan1 = 64;
		channel->pan1_delay = 0;
		channel->volume = 0;
		channel->volume_target = 0;
		channel->volume_delay = 0;
		channel->volume2 = 0;
		channel->loop = 0;
		channel->is_playing = false;
		channel->is_paused = false;
		channel->output_remaining = 0;
		pgm2_mpeg_clear(&channel->decoder);
	}
	for (int idx = 0; idx < 8; idx++)
	{
		ymz_sequence *sequence = &m_sequences[idx];
		sequence->delay = 0;
		sequence->sequence = 0;
		sequence->timer = 0;
		sequence->stopchan = 0;
		sequence->loop = 0;
		sequence->bank = 0;
		sequence->is_playing = false;
		sequence->is_paused = false;
	}
	for (int idx = 0; idx < 8; idx++)
	{
		ymz_sqc *sqc = &m_sqcs[idx];
		sqc->is_playing = false;
		sqc->loop = 0;
	}
}
static void ymz774_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	for (int i = 0; i < samples; i++)
	{
		sequencer();

		// process channels
		int32_t mixl = 0;
		int32_t mixr = 0;

		for (int idx = 0; idx < 16; idx++)
		{
			ymz_channel *channel = &m_channels[idx];
			if (channel->output_remaining > 0)
			{
				// force finish current block
				int32_t smpl = ((int32_t)channel->output_data[channel->output_ptr++] * (channel->volume >> 17)) >> 7;   // volume is linear, 0 - 128 (100%)
				smpl = (smpl * channel->volume2) >> 7;
				mixr += (smpl * channel->pan) >> 7;  // pan seems linear, 0 - 128, where 0 = 100% left, 128 = 100% right, 64 = 50% left 50% right
				mixl += (smpl * (128 - channel->pan)) >> 7;
				channel->output_remaining--;

				if (channel->output_remaining == 0 && !channel->is_playing)
					pgm2_mpeg_clear(&channel->decoder);
			}

			else if (channel->is_playing && !channel->is_paused)
			{
retry:
				if (channel->last_block)
				{
					if (channel->loop)
					{
						if (channel->loop != 255)
							--channel->loop;
						// loop sample
						int phrase = channel->phrase;
						channel->atbl = m_rom[(4*phrase)+0] >> 4 & 7;
						channel->pptr = 8 * get_phrase_offs(phrase);
					}
					else
					{
						channel->is_playing = false;
						channel->output_remaining = 0;
						pgm2_mpeg_clear(&channel->decoder);
					}
				}

				if (channel->is_playing)
				{
					// next block
					int sample_rate = m_sclock, channel_count;
					if (!pgm2_mpeg_decode(&channel->decoder, &channel->pptr, m_rom_size*8, channel->output_data, &channel->output_remaining, &sample_rate, &channel_count, channel->atbl) || channel->output_remaining == 0)
					{
						channel->is_playing = !channel->last_block; // detect infinite retry loop
						channel->last_block = true;
						channel->output_remaining = 0;
						goto retry;
					}

					// it's not clear how exactly clock divider selection works, so far we simply set sample rate as per AMM header,
					// in theory may cause problems if samples will have it wrong or use several rates.
					if (sample_rate != m_sclock)
					{
						m_sclock = sample_rate;
						stream_set_sample_rate(m_stream, m_sclock);
					}

					channel->last_block = channel->output_remaining < 1152;
					channel->output_remaining--;
					channel->output_ptr = 1;

					int32_t smpl = ((int32_t)channel->output_data[0] * (channel->volume >> 17)) >> 7;
					smpl = (smpl * channel->volume2) >> 7;
					mixr += (smpl * channel->pan) >> 7;
					mixl += (smpl * (128 - channel->pan)) >> 7;
				}
			}
		}

		mixr *= m_vlma; // main volume is linear, 0 - 255, where 128 = 100%
		mixl *= m_vlma;
		mixr >>= 7 - m_bsl;
		mixl >>= 7 - m_bsl;
		// Clip limiter: 0 - off, 1 - 6.02 dB (100%), 2 - 4.86 dB (87.5%), 3 - 3.52 dB (75%)
		const int32_t ClipMax3 = 32768 * 75 / 100;
		const int32_t ClipMax2 = 32768 * 875 / 1000;
		switch (m_cpl)
		{
		case 3:
			mixl = ymz_clamp(mixl, -ClipMax3, ClipMax3);
			mixr = ymz_clamp(mixr, -ClipMax3, ClipMax3);
			break;
		case 2:
			mixl = ymz_clamp(mixl, -ClipMax2, ClipMax2);
			mixr = ymz_clamp(mixr, -ClipMax2, ClipMax2);
			break;
		case 1:
			mixl = ymz_clamp(mixl, -32768, 32767);
			mixr = ymz_clamp(mixr, -32768, 32767);
			break;
		}
		if (m_mute)
			mixr = mixl = 0;
		outputs[0][i] = mixl;
		outputs[1][i] = mixr;
	}
}
void ymz774_write(offs_t offset, uint8_t data)
{
	if (offset & 1)
	{
		stream_update(m_stream, 0);
		internal_reg_write(m_cur_reg, data);
	}
	else
		m_cur_reg = data;
}
UINT8 ymz774_read(offs_t offset)
{
	if (offset & 1)
	{
		if (m_cur_reg == 0xe3 || m_cur_reg == 0xe4)
		{
			stream_update(m_stream, 0);
			uint8_t res = 0;
			int bank = (m_cur_reg == 0xe3) ? 8 : 0;
			for (int i = 0; i < 8; i++)
				if (m_channels[i + bank].is_playing)
					res |= 1 << i;
			return res;
		}
	}
	logerror("unimplemented read %02X\n", m_cur_reg);
	return 0;
}

static void internal_reg_write(uint8_t reg, uint8_t data)
{
	// playback registers
	if (reg < 0x10)  // phrase num H and L
	{
		int ch = ((reg >> 1) & 7) + m_bank * 8;
		if (reg & 1)
			m_channels[ch].phrase = (m_channels[ch].phrase & 0xff00) | data;
		else
			m_channels[ch].phrase = (m_channels[ch].phrase & 0x00ff) | ((data & 7) << 8);
	}
	else if (reg < 0x60)
	{
		int ch = (reg & 7) + m_bank * 8;
		switch (reg & 0xf8)
		{
		case 0x10: // Volume 1
			m_channels[ch].volume_target = data;
			break;
		case 0x18: // Volume 1 delayed transition
			m_channels[ch].volume_delay = data;
			break;
		case 0x20: // Volume 2
			m_channels[ch].volume2 = data;
			break;
		case 0x28: // Pan L/R
			m_channels[ch].pan = data;
			break;
		case 0x30: // Pan L/R delayed transition
			if (data) logerror("unimplemented write %02X %02X\n", reg, data);
			m_channels[ch].pan_delay = data;
			break;
		case 0x38: // Pan T/B
			m_channels[ch].pan1 = data;
			break;
		case 0x40: // Pan T/B delayed transition
			if (data) logerror("unimplemented write %02X %02X\n", reg, data);
			m_channels[ch].pan1_delay = data;
			break;
		case 0x48: // Loop
			m_channels[ch].loop = data;
			break;
		case 0x50: // Start / Stop
			if (data)
			{
				int phrase = m_channels[ch].phrase;
				m_channels[ch].atbl = m_rom[(4 * phrase) + 0] >> 4 & 7;
				m_channels[ch].pptr = 8 * get_phrase_offs(phrase);
				m_channels[ch].last_block = false;

				m_channels[ch].is_playing = true;
				m_channels[ch].is_paused = false; // checkme, might be not needed
			}
			else
				m_channels[ch].is_playing = false;
			break;
		case 0x58: // Pause / Resume
			m_channels[ch].is_paused = data ? true : false;
			if (data) logerror("CHECKME: CHAN pause/resume %02X %02X\n", reg, data);
			break;
		}
	}
	else if (reg < 0xd0)
	{
		if (m_bank == 0)
		{
			int sq = reg & 7;
			switch (reg & 0xf8)
			{
			case 0x60: // sequence num H and L
			case 0x68:
				sq = (reg >> 1) & 7;
				if (reg & 1)
					m_sequences[sq].sequence = (m_sequences[sq].sequence & 0xff00) | data;
				else
					m_sequences[sq].sequence = (m_sequences[sq].sequence & 0x00ff) | ((data & 0x07) << 8);
				break;
			case 0x70: // Start / Stop
				if (data)
				{
					//logerror("SEQ %d start (%s)\n", sq, m_sequences[sq].is_playing ? "playing":"stopped");
					m_sequences[sq].offset = get_seq_offs(m_sequences[sq].sequence);
					m_sequences[sq].delay = 0;
					m_sequences[sq].is_playing = true;
					m_sequences[sq].is_paused = false; // checkme, might be not needed
				}
				else
				{
					//logerror("SEQ %d stop (%s)\n", sq, m_sequences[sq].is_playing ? "playing" : "stopped");
					if (m_sequences[sq].is_playing)
						for (int ch = 0; ch < 16; ch++)
							if (m_sequences[sq].stopchan & (1 << ch))
								m_channels[ch].is_playing = false;
					m_sequences[sq].is_playing = false;
				}
				break;
			case 0x78: // Pause / Resume
				m_sequences[sq].is_paused = data ? true : false;
				if (data) logerror("CHECKME: SEQ pause/resume %02X %02X\n", reg, data);
				break;
			case 0x80: // Loop count, 0 = off, 255 - infinite
				m_sequences[sq].loop = data;
				break;
			case 0x88: // timer H and L
			case 0x90:
				sq = (reg - 0x88) >> 1;
				if (reg & 1)
					m_sequences[sq].timer = (m_sequences[sq].timer & 0xff00) | data;
				else
					m_sequences[sq].timer = (m_sequences[sq].timer & 0x00ff) | (data << 8);
				break;
			case 0x98: // Off trigger, bit4 = on/off, bits0-3 channel (end sequence when channel playback ends)
				if (data) logerror("SEQ Off trigger unimplemented %02X %02X\n", reg, data);
				break;
			case 0xa0: // stop channel mask H and L (when sequence stopped)
			case 0xa8:
				sq = (reg >> 1) & 7;
				if (reg & 1)
					m_sequences[sq].stopchan = (m_sequences[sq].stopchan & 0xff00) | data;
				else
					m_sequences[sq].stopchan = (m_sequences[sq].stopchan & 0x00ff) | (data << 8);
				break;
			case 0xb0: // SQC number
				m_sqcs[sq].sqc = data;
				break;
			case 0xb8: // SQC start / stop
				if (data)
				{
					//logerror("SQC %d start (%s)\n", sq, m_sqcs[sq].is_playing ? "playing" : "stopped");
					m_sqcs[sq].offset = get_sqc_offs(m_sqcs[sq].sqc);
					m_sqcs[sq].is_playing = true;
					m_sqcs[sq].is_waiting = false;
				}
				else
				{
					//logerror("SQC %d stop (%s)\n", sq, m_sqcs[sq].is_playing ? "playing" : "stopped");
					m_sqcs[sq].is_playing = false;
					// stop SEQ too, and stop channels
					if (m_sequences[sq].is_playing)
						for (int ch = 0; ch < 16; ch++)
							if (m_sequences[sq].stopchan & (1 << ch))
								m_channels[ch].is_playing = false;
					m_sequences[sq].is_playing = false;
				}
				break;
			case 0xc0: // SQC loop (255 = infinite)
				m_sqcs[sq].loop = data;
				break;
			default:
				logerror("unimplemented write %02X %02X\n", reg, data);
				break;
			}
		}
		// else bank1 - Equalizer control
	}
	// global registers
	else
	{
		switch (reg) {
		case 0xd0: // Total Volume L0/R0
			m_vlma = data;
			break;
		case 0xd1: // Total Volume L1/R1
			m_vlma1 = data;
			break;
		case 0xd2: // Clip limit
			m_cpl = data;
			break;
		//case 0xd3: // Digital/PWM output
		//case 0xd4: // Digital audio IF/IS L0/R0
		//case 0xd5: // Digital audio IF/IS L1/R1
		//case 0xd8: // GPIO A
		//case 0xdd: // GPIO B
		//case 0xde: // GPIO C
		case 0xf0:
			m_bank = data & 1;
			if (data > 1) logerror("Set bank %02X!\n", data);
			break;
		default:
			logerror("unimplemented write %02X %02X\n", reg, data);
			break;
		}
	}
}

static void sequencer(void)
{
	for (int idx = 0; idx < 16; idx++)
	{
		ymz_channel *chan = &m_channels[idx];
		if (chan->is_playing && !chan->is_paused && (chan->volume >> 17) != chan->volume_target)
		{
			if (chan->volume_delay)
			{
				if ((chan->volume >> 17) < chan->volume_target)
					chan->volume += volinc[chan->volume_delay];
				else
					chan->volume -= volinc[chan->volume_delay];
			}
			else
				chan->volume = chan->volume_target << 17;
		}
	}

	for (int i = 0; i < 8; i++)
	{
		ymz_sqc *sqc = &m_sqcs[i];
		ymz_sequence *sequence = &m_sequences[i];

		if (sqc->is_playing && !sqc->is_waiting)
		{
			// SQC consists of 4 byte records: SEQ num H, SEQ num L, SEQ Loop count, End flag (0xff)
			sequence->sequence = ((get_rom_byte(sqc->offset) << 8) | get_rom_byte(sqc->offset + 1)) & 0x7ff;
			sqc->offset += 2;
			sequence->loop = get_rom_byte(sqc->offset++);
			sequence->offset = get_seq_offs(sequence->sequence);
			sequence->delay = 0;
			sequence->is_playing = true;
			sequence->is_paused = false; // checkme, might be not needed
			sqc->is_waiting = true;
			if (get_rom_byte(sqc->offset++) == 0xff)
			{
				if (sqc->loop)
				{
					if (sqc->loop != 255)
						--sqc->loop;
					sqc->offset = get_sqc_offs(sqc->sqc);
				}
				else
					sqc->is_playing = false;
			}
		}

		if (sequence->is_playing && !sequence->is_paused)
		{
			if (sequence->delay > 0)
				--sequence->delay;
			else
			{
				int reg = get_rom_byte(sequence->offset++);
				uint8_t data = get_rom_byte(sequence->offset++);
				switch (reg)
				{
				case 0xff: // end
					for (int ch = 0; ch < 16; ch++) // might be wrong, ie not needed in case of loop
						if (sequence->stopchan & (1 << ch))
							m_channels[ch].is_playing = false;
					if (sequence->loop)
					{
						if (sequence->loop != 255)
							--sequence->loop;
						sequence->offset = get_seq_offs(sequence->sequence);
					}
					else
					{
						sequence->is_playing = false;
						sqc->is_waiting = false;
					}
					break;
				case 0xfe: // timer delay
					sequence->delay = sequence->timer * 32 + 32 - 1;
					break;
				case 0xf0:
					sequence->bank = data & 1;
					break;
				default:
				{
					uint8_t temp = m_bank;
					m_bank = sequence->bank;
					if (m_bank == 0 && reg >= 0x60 && reg < 0xb0) // if we hit SEQ registers need to add this sequence offset
					{
						int sqn = i;
						if (reg < 0x70 || (reg >= 0x88 && reg < 0x98) || reg >= 0xa0)
							sqn = i * 2;
						internal_reg_write(reg + sqn, data);
					}
					else
						internal_reg_write(reg, data);
					m_bank = temp;
				}
					break;
				}
			}
		}
	}
}

static void *ymz774_start(int clock, const struct CustomSound_interface *config)
{
	int i;
	m_rom = memory_region(REGION_SOUND1);
	m_rom_size = memory_region_length(REGION_SOUND1);
	m_sclock = clock / 512;
	m_cur_reg = m_mute = m_doen = m_vlma = m_vlma1 = m_bsl = m_cpl = m_bank = 0;
	for (i=0; i<16; i++) pgm2_mpeg_init(&m_channels[i].decoder, m_rom);
	for (i=0; i<256; i++) volinc[i] = i < 0x20 ? i : (0x20 | (i & 31)) << ((i >> 5)-1);
	ymz774_reset();
	m_stream = stream_create(0, 2, m_sclock, NULL, ymz774_update);
	return m_stream;
}
static void ymz774_reset_token(void *token) { ymz774_reset(); }
const struct CustomSound_interface ymz774_interface = { ymz774_start, NULL, ymz774_reset_token, NULL };

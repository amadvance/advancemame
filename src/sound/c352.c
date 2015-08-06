/*
    c352.c - Namco C352 custom PCM chip emulation
    v1.0.1
    By R. Belmont
    Additional code by cync and the hoot development team

    Thanks to Cap of VivaNonno for info and The_Author for preliminary reverse-engineering

    Chip specs:
    32 voices
    Supports 8-bit linear and 8-bit muLaw samples
    Output: digital, 16 bit, 4 channels
    Output sample rate is the input clock / 384.
 */

#include <math.h>
#include "sndintrf.h"
#include "streams.h"
#include "c352.h"

#define VERBOSE (0)

// flags

#define C352_FLG_REVPHASE	0x20000
#define C352_FLG_LONGPHASE	0x10000 // first phase of "long sample mode" complete
#define C352_FLG_CONTINUE	0x8000	// second phase of "long sample mode"
#define	C352_FLG_ACTIVE		0x4000	// channel is active
#define C352_FLG_PHASERL	0x0200	// REAR LEFT invert phase 180 degrees (e.g. flip sign of sample)
#define C352_FLG_PHASEFL	0x0100	// FRONT LEFT invert phase 180 degrees (e.g. flip sign of sample)
#define C352_FLG_PHASEFR	0x0080	// FRONT RIGHT invert phase 180 degrees (e.g. flip sign of sample)
#define C352_FLG_LONG		0x0020	// "long-format" sample (can't loop, not sure what else it means)
#define C352_FLG_NOISE		0x0010	// play noise instead of sample
#define C352_FLG_MULAW		0x0008	// sample is mulaw instead of linear 8-bit PCM
#define C352_FLG_FILTER		0x0004	// don't apply filter (linear interpolation)
#define C352_FLG_REVLOOP   	0x0003	// loop backwards
#define C352_FLG_LOOP		0x0002	// loop forward
#define C352_FLG_REVERSE	0x0001	// play sample backwards

typedef struct
{
	UINT8	vol_l;
	UINT8	vol_r;
	UINT8	vol_l2;
	UINT8	vol_r2;
	UINT8	unk9;
	UINT8	bank;
	INT16	noise;
	INT16   noisebuf;
	UINT16  noisecnt;
	UINT16	pitch;
	UINT16	start_addr;
	UINT16	end_addr;
	UINT32	flag;
	UINT32	current_addr;
	UINT32	stop_addr;
	UINT32	loop_addr;
	UINT32  loop_point;
	UINT32	pos;
} c352_ch_t;

struct c352_info
{
	sound_stream *stream;
	c352_ch_t c352_ch[32];
	unsigned char *c352_rom_samples;
	int c352_region;
	int sample_rate_base;

	INT16 level_table[256];

	long	channel_l[2048];
	long	channel_r[2048];
	long	channel_l2[2048];
	long	channel_r2[2048];

	short	mulaw_table[256];
	unsigned int mseq_reg;
};

// noise generator
static int get_mseq_bit(struct c352_info *info)
{
	unsigned int mask = (1 << (7 - 1));
	unsigned int reg = info->mseq_reg;
	unsigned int bit = reg & (1 << (17 - 1));

	if (bit)
	{
       		reg = ((reg ^ mask) << 1) | 1;
	}
	else
	{
		reg = reg << 1;
	}

	info->mseq_reg = reg;

	return (reg & 1);
}

static void c352_mix_one_channel(struct c352_info *info, unsigned long ch, long sample_count)

{

	int i;
	signed short sample, nextsample;
	signed short noisebuf;
	UINT16 noisecnt;
	float pbase = (float)info->sample_rate_base / (float)Machine->sample_rate;
	INT32 frequency, delta, offset, cnt, flag;
	UINT32 pos;

	frequency = info->c352_ch[ch].pitch;
	delta=(long)((float)frequency * pbase);

	pos = info->c352_ch[ch].current_addr;	// sample pointer
	offset = info->c352_ch[ch].pos;	// 16.16 fixed-point offset into the sample
	flag = info->c352_ch[ch].flag;
	noisecnt = info->c352_ch[ch].noisecnt;
	noisebuf = info->c352_ch[ch].noisebuf;

	if ((flag & C352_FLG_ACTIVE) == 0)
	{
		return;
	}

	for(i = 0 ; i < sample_count ; i++)
	{
		offset += delta;
		cnt = (offset>>16)&0x7fff;
		if (cnt)			// if there is a whole sample part, chop it off now that it's been applied
		{
			offset &= 0xffff;
		}

		// apply the whole sample part of the fraction to the current pointer and check if we're at the end
		if ((flag & C352_FLG_REVLOOP) == C352_FLG_REVLOOP)
		{
			if (flag & C352_FLG_REVPHASE)
			{
				pos -= cnt;

				if (pos < info->c352_ch[ch].loop_point)
				{
					pos = info->c352_ch[ch].loop_point;
					info->c352_ch[ch].flag &= ~C352_FLG_REVPHASE;
				}
			}
			else
			{
				pos += cnt;

				if (pos > info->c352_ch[ch].stop_addr)
				{
					pos = info->c352_ch[ch].stop_addr;
					info->c352_ch[ch].flag |= C352_FLG_REVPHASE;
				}
			}
		}
		else if (flag & C352_FLG_REVERSE)
		{
			pos -= cnt;

			if (pos < info->c352_ch[ch].stop_addr)
			{
				if (flag & C352_FLG_LONG)
				{
//                  printf("ch %02d: end chain part 1\n", ch);
					info->c352_ch[ch].flag &= ~C352_FLG_ACTIVE;
					info->c352_ch[ch].flag |= C352_FLG_LONGPHASE;
					return;
				}

				if (flag & C352_FLG_LOOP)
				{
					pos = info->c352_ch[ch].loop_point;
				}
				else
				{
					info->c352_ch[ch].flag &= ~C352_FLG_ACTIVE;
					return;
				}
			}
		}
		else
		{
			pos += cnt;

			if (flag & C352_FLG_LONG)
			{
				if ((pos+256) > info->c352_ch[ch].stop_addr)
				{
//                  printf("ch %02d: end chain part 1\n", ch);
					info->c352_ch[ch].flag |= C352_FLG_LONGPHASE;
				}
				if (pos > info->c352_ch[ch].stop_addr)
				{
					info->c352_ch[ch].flag &= ~C352_FLG_ACTIVE;
					return;
				}
			}
			else
			{
				if (pos > info->c352_ch[ch].stop_addr)
				{
					if (flag & C352_FLG_LOOP)
					{
						pos = info->c352_ch[ch].loop_point;
					}
					else
					{
						info->c352_ch[ch].flag &= ~C352_FLG_ACTIVE;
						return;
					}
				}
			}
		}

		if ((int)pos > memory_region_length(info->c352_region)) pos %= memory_region_length(info->c352_region);
		sample = (char)info->c352_rom_samples[pos];
		nextsample = (char)info->c352_rom_samples[pos+cnt];

		// sample is muLaw, not 8-bit linear (Fighting Layer uses this extensively)
		if (flag & C352_FLG_MULAW)
		{
			sample = info->mulaw_table[(unsigned char)sample];
			nextsample = info->mulaw_table[(unsigned char)nextsample];
		}
		else
		{
			sample <<= 8;
			nextsample <<= 8;
		}

		// play noise instead of sample data
		if (flag & C352_FLG_NOISE)
		{
			int noise_level = 0x8000;
			sample = info->c352_ch[ch].noise = (info->c352_ch[ch].noise << 1) | get_mseq_bit(info);
			sample = (sample & (noise_level - 1)) - (noise_level >> 1);
			if (sample > 0x7f)
			{
				sample = 0x7f;
			}
			else if (sample < 0)
			{
				sample = 0xff;
			}
			sample = info->mulaw_table[(unsigned char)sample];

			if ( (pos+cnt) == pos )
			{
				noisebuf += sample;
				noisecnt++;
				sample = noisebuf / noisecnt;
			}
			else
			{
				if ( noisecnt )
				{
					sample = noisebuf / noisecnt;
				}
				else
				{
					sample = info->mulaw_table[0x7f];		// Nearest sound(s) is here.
				}
				noisebuf = 0;
				noisecnt = ( flag & C352_FLG_FILTER ) ? 0 : 1;
			}
		}

		// apply linear interpolation
		if ( (flag & (C352_FLG_FILTER | C352_FLG_NOISE)) == 0 )
		{
			sample = (short)(sample + ((nextsample-sample) * (((double)(0x0000ffff&offset) )/0x10000)));
		}

		if ( flag & C352_FLG_PHASEFL )
		{
			info->channel_l[i]  += ((-sample * info->c352_ch[ch].vol_l)>>8);
		}
		else
		{
		info->channel_l[i] += ((sample * info->c352_ch[ch].vol_l)>>8);
		}

		if ( flag & C352_FLG_PHASEFR )
		{
			info->channel_r[i]  += ((-sample * info->c352_ch[ch].vol_r)>>8);
		}
		else
		{
		info->channel_r[i] += ((sample * info->c352_ch[ch].vol_r)>>8);
		}

		if ( flag & C352_FLG_PHASERL )
		{
			info->channel_l2[i] += ((-sample * info->c352_ch[ch].vol_l2)>>8);
		}
		else
		{
		info->channel_l2[i] += ((sample * info->c352_ch[ch].vol_l2)>>8);
		}
		info->channel_r2[i] += ((sample * info->c352_ch[ch].vol_r2)>>8);
	}

	info->c352_ch[ch].noisecnt = noisecnt;
	info->c352_ch[ch].noisebuf = noisebuf;
	info->c352_ch[ch].pos = offset;
	info->c352_ch[ch].current_addr = pos;
}


static void c352_update(void *param, stream_sample_t **inputs, stream_sample_t **buf, int sample_count)
{
	struct c352_info *info = param;
	int i, j;
	stream_sample_t *bufferl = buf[0];
	stream_sample_t *bufferr = buf[1];
	stream_sample_t *bufferl2 = buf[2];
	stream_sample_t *bufferr2 = buf[3];

	for(i = 0 ; i < sample_count ; i++)
	{
	       info->channel_l[i] = info->channel_r[i] = info->channel_l2[i] = info->channel_r2[i] = 0;
	}

	for (j = 0 ; j < 32 ; j++)
	{
		c352_mix_one_channel(info, j, sample_count);
	}

	for(i = 0 ; i < sample_count ; i++)
	{
		*bufferl++ = (short) (info->channel_l[i] >>3);
		*bufferr++ = (short) (info->channel_r[i] >>3);
		*bufferl2++ = (short) (info->channel_l2[i] >>3);
		*bufferr2++ = (short) (info->channel_r2[i] >>3);
	}
}

static unsigned short c352_read_reg16(struct c352_info *info, unsigned long address)
{
	unsigned long	chan;
	unsigned short	val;

	stream_update(info->stream, 0);

	chan = (address >> 4) & 0xfff;
	if (chan > 31)
	{
		val = 0;
	}
	else
	{
		if ((address & 0xf) == 6)
		{
			int bits = (info->c352_ch[chan].flag & 0x0020) ? 0x8800 : 0x8000;
			val = info->c352_ch[chan].flag;
			val = (val & ~0x8800) | ((info->c352_ch[chan].flag & C352_FLG_LONGPHASE) ? bits : 0x0000);
		}
		else
		{
			val = 0;
		}
	}
	return val;
}

static void c352_write_reg16(struct c352_info *info, unsigned long address, unsigned short val)
{
	unsigned long	chan;
	unsigned long	temp;
	chan = (address >> 4) & 0xfff;

	stream_update(info->stream, 0);

	if (chan > 31)
	{
		#if VERBOSE
		logerror("C352 CTRL %08x %04x\n", address, val);
		#endif
		return;
	}
	switch(address & 0xf)
	{
	case 0x0:
		// volumes (output 1)
		#if VERBOSE
		logerror("CH %02d LVOL %02x RVOL %02x\n", chan, val & 0xff, val >> 8);
		#endif
		info->c352_ch[chan].vol_l = val & 0xff;
		info->c352_ch[chan].vol_r = val >> 8;
		break;

	case 0x2:
		// volumes (output 2)
		#if VERBOSE
		logerror("CH %02d RLVOL %02x RRVOL %02x\n", chan, val & 0xff, val >> 8);
		#endif
		info->c352_ch[chan].vol_l2 = val & 0xff;
		info->c352_ch[chan].vol_r2 = val >> 8;
		break;

	case 0x4:
		// pitch
		#if VERBOSE
		logerror("CH %02d PITCH %04x\n", chan, val);
		#endif
		info->c352_ch[chan].pitch = val;
		break;

	case 0x6:
		// flags
		#if VERBOSE
		logerror("CH %02d FLAG %02x\n", chan, val);
		#endif
		info->c352_ch[chan].flag = val;

		// not chain mode, normal setup
		if ((val & 0x8020) == 0)
		{
			temp = info->c352_ch[chan].bank<<16;
			temp += info->c352_ch[chan].start_addr;
			info->c352_ch[chan].current_addr = temp;

			temp = info->c352_ch[chan].bank<<16;
			temp += info->c352_ch[chan].end_addr;
			info->c352_ch[chan].stop_addr = temp;

			temp = info->c352_ch[chan].bank<<16;
			temp += info->c352_ch[chan].loop_addr;
			info->c352_ch[chan].loop_point = temp;

			info->c352_ch[chan].noisebuf = 0;
			info->c352_ch[chan].noisecnt = 0;

			switch (val & 3)
			{
				case 0:	// normal
				case 2:	// loop
				case 3:	// reverse loop
					if (info->c352_ch[chan].current_addr >= info->c352_ch[chan].stop_addr)
					{
						info->c352_ch[chan].stop_addr += 0x10000;
						info->c352_ch[chan].loop_point += 0x10000;
					}
					break;
				case 1:	// reverse
					if (info->c352_ch[chan].current_addr <= info->c352_ch[chan].stop_addr)
					{
						info->c352_ch[chan].stop_addr -= 0x10000;
						info->c352_ch[chan].loop_point -= 0x10000;
					}
					break;
			}

//  printf("ch %02d: start: %08x\n", chan, info->c352_ch[chan].current_addr);

/*          printf("ch %02d: normal start: %06x -> (%06x -> %06x)\n", chan,
                info->c352_ch[chan].current_addr,
                info->c352_ch[chan].loop_point,
                info->c352_ch[chan].stop_addr);*/

			info->c352_ch[chan].pos = 0;
		}
		else if ((val & 0x8020) == 0x0020) // chain mode, start first part
		{
			int bank2 = (info->c352_ch[chan].loop_addr & 0xff)<<16;

			info->c352_ch[chan].current_addr = info->c352_ch[chan].start_addr + (info->c352_ch[chan].bank<<16);
			info->c352_ch[chan].stop_addr = info->c352_ch[chan].end_addr + bank2;
			info->c352_ch[chan].pos = 0;

			info->c352_ch[chan].stop_addr &= 0xffffff;

//          printf("ch %02d: start chain part 1: %06x -> %06x\n", chan, info->c352_ch[chan].current_addr, info->c352_ch[chan].stop_addr);

		}
		else if ((val & 0x8000) == 0x8000) // chain mode, phase 2
		{
			int bank = (info->c352_ch[chan].start_addr & 0xff)<<16;

			info->c352_ch[chan].loop_point = info->c352_ch[chan].loop_addr + bank;
			info->c352_ch[chan].stop_addr = info->c352_ch[chan].end_addr + bank;
			info->c352_ch[chan].flag &= ~C352_FLG_LONGPHASE;
			info->c352_ch[chan].flag |= C352_FLG_ACTIVE;
			//info->c352_ch[chan].current_addr = info->c352_ch[chan].loop_point;
			//info->c352_ch[chan].pos = 0;

			switch (val & 3)
			{
				case 0:	// normal
				case 2:	// loop
				case 3:	// reverse loop
					if (info->c352_ch[chan].loop_point >= info->c352_ch[chan].stop_addr)
					{
						info->c352_ch[chan].stop_addr += 0x10000;
					}
					break;
				case 1:	// reverse
					if (info->c352_ch[chan].loop_point <= info->c352_ch[chan].stop_addr)
					{
						info->c352_ch[chan].stop_addr -= 0x10000;
					}
					break;
			}

//          printf("ch %02d: start chain part 2: %06x -> %06x\n", chan, info->c352_ch[chan].current_addr, info->c352_ch[chan].stop_addr);
		}

		break;

	case 0x8:
		// bank (bits 16-31 of address);
		info->c352_ch[chan].bank = val & 0xff;
		#if VERBOSE
		logerror("CH %02d BANK %02x", chan, info->c352_ch[chan].bank);
		#endif
		break;

	case 0xa:
		// start address
		#if VERBOSE
		logerror("CH %02d SADDR %04x\n", chan, val);
		#endif
		info->c352_ch[chan].start_addr = val;
		break;

	case 0xc:
		// end address
		#if VERBOSE
		logerror("CH %02d EADDR %04x\n", chan, val);
		#endif
		info->c352_ch[chan].end_addr = val;
		break;

	case 0xe:
		// loop address
		#if VERBOSE
		logerror("CH %02d LADDR %04x\n", chan, val);
		#endif
		info->c352_ch[chan].loop_addr = val;
		break;

	default:
		#if VERBOSE
		logerror("CH %02d UNKN %01x %04x", chan, address & 0xf, val);
		#endif
		break;
	}
}

static void c352_init(struct c352_info *info, int sndindex)
{
	int i;
	double x_max = 25000.0;
	double y_max = 127.0;
	double u = 8.0;

	// clear all channels states
	memset(info->c352_ch, 0, sizeof(c352_ch_t)*32);

	// generate mulaw table for mulaw format samples
	for (i = 0; i < 256; i++)
	{
	      double y = (double) (i & 0x7f);
	      double x = (exp (y / y_max * log (1.0 + u)) - 1.0) * x_max / u;

	      if (i & 0x80)
	      {
	        x = -x;
	      }
	      info->mulaw_table[i] = (short)x;
	}

	// init noise generator
	info->mseq_reg = 0x12345678;

	// register save state info
	for (i = 0; i < 32; i++)
	{
		char cname[32];

		sprintf(cname, "C352 v %02d", i);

		state_save_register_item(cname, sndindex, info->c352_ch[i].vol_l);
		state_save_register_item(cname, sndindex, info->c352_ch[i].vol_r);
		state_save_register_item(cname, sndindex, info->c352_ch[i].vol_l2);
		state_save_register_item(cname, sndindex, info->c352_ch[i].vol_r2);
		state_save_register_item(cname, sndindex, info->c352_ch[i].unk9);
		state_save_register_item(cname, sndindex, info->c352_ch[i].bank);
		state_save_register_item(cname, sndindex, info->c352_ch[i].noise);
		state_save_register_item(cname, sndindex, info->c352_ch[i].pitch);
		state_save_register_item(cname, sndindex, info->c352_ch[i].start_addr);
		state_save_register_item(cname, sndindex, info->c352_ch[i].end_addr);
		state_save_register_item(cname, sndindex, info->c352_ch[i].flag);
		state_save_register_item(cname, sndindex, info->c352_ch[i].current_addr);
		state_save_register_item(cname, sndindex, info->c352_ch[i].stop_addr);
		state_save_register_item(cname, sndindex, info->c352_ch[i].loop_addr);
		state_save_register_item(cname, sndindex, info->c352_ch[i].pos);
	}

	for (i = 0; i < 256; i++)
	{
	      double max_level = 255.0;

	      info->level_table[255-i] = (int) (pow (10.0, (double) i / 256.0 * -20.0 / 20.0) * max_level);
	}
}

static void *c352_start(int sndindex, int clock, const void *config)
{
	const struct C352interface *intf;
	struct c352_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	intf = config;

	info->c352_rom_samples = memory_region(intf->region);
	info->c352_region = intf->region;

	info->stream = stream_create(0, 4, Machine->sample_rate, info, c352_update);

	info->sample_rate_base = clock / 192;

	c352_init(info, sndindex);

	return info;
}


READ16_HANDLER( c352_0_r )
{
	return(c352_read_reg16(sndti_token(SOUND_C352, 0), offset*2));
}

WRITE16_HANDLER( c352_0_w )
{
	if (mem_mask == 0)
	{
		c352_write_reg16(sndti_token(SOUND_C352, 0), offset*2, data);
	}
	else
	{
		logerror("C352: byte-wide write unsupported at this time!\n");
	}
}







/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void c352_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void c352_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = c352_set_info;			break;
		case SNDINFO_PTR_START:							info->start = c352_start;				break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "C352";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Namco PCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}


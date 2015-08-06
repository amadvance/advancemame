/*
    Ricoh RF5C400 emulator

    Written by Ville Linde
*/

#include "sndintrf.h"
#include "streams.h"
#include "rf5c400.h"
#include <math.h>

#define CLOCK (44100 * 384)	// = 16.9344 MHz

struct rf5c400_info
{
	const struct RF5C400interface *intf;

	INT16 *rom;
	UINT32 rom_length;

	sound_stream *stream;

	int current_channel;
	int keyon_channel;

	struct RF5C400_CHANNEL
	{
		UINT32 start;
		UINT32 end;
		UINT64 pos;
		UINT64 step;
		UINT16 keyon;
		INT16 volume;
		UINT16 pan;
		UINT16 flag;
		UINT16 sysflag;
		UINT16 keyflag;
	} channels[32];
};

static int volume_table[256];

enum {
	RF5C400_FLG_LOOP		= 0x1000,	// loop ?
};


/*****************************************************************************/

static void rf5c400_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	int i, ch;
	struct rf5c400_info *info = param;
	INT16 *rom = info->rom;

	memset(buffer[0], 0, length*sizeof(*buffer[0]));
	memset(buffer[1], 0, length*sizeof(*buffer[1]));

	for (ch=0; ch < 32; ch++)
	{
		struct RF5C400_CHANNEL *channel = &info->channels[ch];

		for (i=0; i < length; i++)
		{
			if (channel->keyon != 0)
			{
				INT64 sample;
				UINT32 cur_pos = channel->start + (channel->pos >> 16);

				sample = rom[cur_pos];

				sample *= volume_table[channel->volume];
				buffer[0][i] += (sample - ((sample * (channel->pan & 0xff)) >> 8)) >> 6;
				buffer[1][i] += (sample - ((sample * (channel->pan >>8)) >> 8)) >> 6;

				channel->pos += channel->step;
				if (cur_pos > info->rom_length || cur_pos > channel->end)
				{
					if ( (channel->flag & RF5C400_FLG_LOOP) ) {
						channel->pos = 0;
					} else {
						channel->keyon = 0;
						channel->pos = 0;
					}
				}
			}
		}
	}

	for (i=0; i < length; i++)
	{
		buffer[0][i] >>= 3;
		buffer[1][i] >>= 3;
	}
}

static void rf5c400_init_chip(struct rf5c400_info *info, int sndindex)
{
	UINT16 *sample;
	int i;

	info->rom = (INT16*)memory_region(info->intf->region);
	info->rom_length = memory_region_length(info->intf->region) / 2;

	// preprocess sample data
	sample = (UINT16*)memory_region(info->intf->region);
	for (i=0; i < memory_region_length(info->intf->region) / 2; i++)
	{
		if (sample[i] & 0x8000)
		{
			sample[i] ^= 0x7fff;
		}
	}

	// init volume table
	{
		double max=255.0;
		for (i = 0; i < 256; i++) {
			volume_table[i]=(UINT16)max;
			max /= pow(10.0,(double)((2.5/(256.0/16.0))/20));
		}
	}

	info->stream = stream_create(0, 2, Machine->sample_rate, info, rf5c400_update);
}


static void *rf5c400_start(int sndindex, int clock, const void *config)
{
	struct rf5c400_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config;

	rf5c400_init_chip(info, sndindex);

	return info;
}

/*****************************************************************************/

static UINT16 rf5c400_status = 0;
static UINT16 rf5c400_r(int chipnum, int offset)
{
	switch(offset)
	{
		case 0x00:
		{
			return rf5c400_status;
		}

		case 0x04:
		{
			return 0;
		}
	}

	return 0;
}

static void rf5c400_w(int chipnum, int offset, UINT16 data)
{
	struct rf5c400_info *info = sndti_token(SOUND_RF5C400, chipnum);

	if (offset < 0x400)
	{
		switch(offset)
		{
			case 0x00:
			{
				rf5c400_status = data;
				break;
			}

			case 0x01:		// channel control
			{
				switch ( data & 0x60 ) {
					case 0x60:
						if ( !info->channels[data&0x1f].keyon ) {
							info->channels[data&0x1f].pos = 0;
							info->channels[data&0x1f].keyon = 1;
						}
						break;
					case 0x40:
						if ( !(info->channels[data&0x1f].keyflag & 0x2000) ) {
							info->channels[data&0x1f].pos = 0;
							info->channels[data&0x1f].keyon = 0;
						}
						break;
					default:
						info->channels[data&0x1f].pos = 0;
						info->channels[data&0x1f].keyon = 0;
				}
				break;
			}

			case 0x8:
			{
				data &= 0x1f;
				info->current_channel = data;
				break;
			}

			case 0x9:
			{
				info->channels[info->current_channel].sysflag = data;
				break;
			}

			default:
			{
				//printf("rf5c400_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
			}
		}
		//printf("rf5c400_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
	}
	else
	{
		// channel registers
		int ch = (offset >> 5) & 0x1f;
		int reg = (offset & 0x1f);

		struct RF5C400_CHANNEL *channel = &info->channels[ch];

		switch (reg)
		{
			case 0x00:		// sample start address, bits 23 - 16
			{
				channel->start &= 0xffff;
				channel->start |= (UINT32)(data & 0xff00) << 8;
				break;
			}
			case 0x01:		// sample start address, bits 15 - 0
			{
				channel->start &= 0xff0000;
				channel->start |= data;
				break;
			}
			case 0x02:		// sample playing frequency
			{
				int frequency = (CLOCK/384) / 8;
				int multiple = 1 << (data >> 13);
				double rate = ((double)(data & 0x1fff) / 2048.0) * (double)multiple;
				channel->step = (UINT64)((((double)(frequency) * rate) / (double)(Machine->sample_rate)) * 65536.0);
				break;
			}
			case 0x03:		// sample end address, bits 15 - 0
			{
				channel->end &= 0xff0000;
				channel->end |= data;
				break;
			}
			case 0x04:		// sample end address, bits 23 - 16
			{
				channel->end &= 0xffff;
				channel->end |= (UINT32)(data & 0xff) << 16;
				break;
			}
			case 0x05:		// unknown
			{
				break;
			}
			case 0x06:		// unknown
			{
				break;
			}
			case 0x07:		// channel volume
			{
				channel->pan = data;
				break;
			}
			case 0x08:		// volume
			{
				channel->volume = data & 0xff;
				break;
			}
			case 0x0E:
			{
				channel->keyflag = data;
				break;
			}
			case 0x10:		// unknown
			{
				channel->flag = data;
				break;
			}
		}
	}
}

READ16_HANDLER( RF5C400_0_r )
{
	return rf5c400_r(0, offset);
}

WRITE16_HANDLER( RF5C400_0_w )
{
	rf5c400_w(0, offset, data);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void rf5c400_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void rf5c400_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = rf5c400_set_info;		break;
		case SNDINFO_PTR_START:							info->start = rf5c400_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "RF5C400";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Ricoh PCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

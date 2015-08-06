/*
    CD-DA "Red Book" audio sound hardware handler
    Relies on the actual CD logic and reading in cdrom.c.
*/

#include "sndintrf.h"
#include "streams.h"
#include "cpuintrf.h"
#include "cdrom.h"
#include "cdda.h"

struct cdda_info
{
	sound_stream *		stream;
	cdrom_file *	disc;
};

static void cdda_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	struct cdda_info *info = param;
	if (info->disc != (cdrom_file *)NULL)
	{
		cdrom_get_audio_data(info->disc, &outputs[0][0], &outputs[1][0], length);
	}
}

static void *cdda_start(int sndindex, int clock, const void *config)
{
	const struct CDDAinterface *intf;
	struct cdda_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	intf = config;

	info->stream = stream_create(0, 2, 44100, info, cdda_update);

	return info;
}

void CDDA_set_cdrom(int num, void *file)
{
	struct cdda_info *info = sndti_token(SOUND_CDDA, num);
	info->disc = (cdrom_file *)file;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void cdda_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void cdda_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = cdda_set_info;			break;
		case SNDINFO_PTR_START:							info->start = cdda_start;				break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "CD/DA";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "CD Audio";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}


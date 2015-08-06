#include <time.h>
#include "driver.h"
#include "ds1315.h"
#include "mscommon.h"

/* This is an emulation of Dallas Semiconductor's Phantom Time Chip.
   DS1315.
   
   by tim lindner, November 2001.
*/

enum
{
	ds_seek_matching,
	ds_calendar_io
};

static void ds1315_fill_raw_data( void );
static void ds1315_input_raw_data( void );

static int ds1315_count;
static int ds1315_mode;						 
static int ds1315_raw_data[8*8];
static int ds1315_pattern[] = {  1, 0, 1, 0, 0, 0, 1, 1,
							     0, 1, 0, 1, 1, 1, 0, 0,
							     1, 1, 0, 0, 0, 1, 0, 1,
							     0, 0, 1, 1, 1, 0, 1, 0,
							     1, 0, 1, 0, 0, 0, 1, 1,
							     0, 1, 0, 1, 1, 1, 0, 0,
							     1, 1, 0, 0, 0, 1, 0, 1,
							     0, 0, 1, 1, 1, 0, 1, 0 };

void ds1315_init( void )
{
	ds1315_count = 0;
	ds1315_mode = ds_seek_matching;
}

 READ8_HANDLER ( ds1315_r_0 )
{
	if( ds1315_pattern[ ds1315_count++ ] == 0 )
	{
		if( ds1315_count == 64 )
		{
			/* entire pattern matched */
			ds1315_count = 0;
			ds1315_mode = ds_calendar_io;
			ds1315_fill_raw_data();
		}

		return 0;
	}

	ds1315_count = 0;
	ds1315_mode = ds_seek_matching;

	return 0;
}
	
 READ8_HANDLER ( ds1315_r_1 )
{
	if( ds1315_pattern[ ds1315_count++ ] == 1 )
		return 0;
	
	ds1315_count = 0;
	ds1315_mode = ds_seek_matching;
	return 0;
}
	
 READ8_HANDLER ( ds1315_r_data )
{
	UINT8	result;
	
	if( ds1315_mode == ds_calendar_io )
	{
		result = ds1315_raw_data[ ds1315_count++ ];
		
		if( ds1315_count == 64 )
		{
			ds1315_mode = ds_seek_matching;
			ds1315_count = 0;
		}
		
		return result;
	}
	
	ds1315_count = 0;
	return 0;
}

WRITE8_HANDLER ( ds1315_w_data )
{
	
	if( ds1315_mode == ds_calendar_io )
	{
		ds1315_raw_data[ ds1315_count++ ] = data & 0x01;
		
		if( ds1315_count == 64 )
		{
			ds1315_mode = ds_seek_matching;
			ds1315_count = 0;
			ds1315_input_raw_data();
		}
		
		return;
	}
	
	ds1315_count = 0;
	return;
}

static void ds1315_fill_raw_data( void )
{
	/* This routine will (hopefully) call a standard 'C' library routine to get the current
	   date and time and then fill in the raw data struct.
	*/

	time_t		t;
	struct tm 	*tmtime;
	int			raw[8], i, j;
	
	/* Get seconds from epoch */
	t=time(NULL);
	if (t==-1) return;
	
	/* Convert seconds to local time */
	tmtime=localtime(&t);

	raw[0] = 0;	/* tenths and hundreths of seconds are always zero */
	raw[1] = dec_2_bcd(tmtime->tm_sec);
	raw[2] = dec_2_bcd(tmtime->tm_min);
	raw[3] = dec_2_bcd(tmtime->tm_hour);

	raw[4] = dec_2_bcd(tmtime->tm_wday);
	raw[5] = dec_2_bcd(tmtime->tm_mday);
	raw[6] = dec_2_bcd(tmtime->tm_mon+1);
	raw[7] = dec_2_bcd(tmtime->tm_year); /* Epoch is 1900 */

	/* Ok now we have the raw bcd bytes. Now we need to push them into our bit array */
	
	for( i=0; i<64; i++ )
	{
		j = i / 8;
		ds1315_raw_data[i] = (raw[j] & 0x0001);
		raw[j] = raw[j] >> 1;
	}

	/* After reading the sources for localtime(), I have determined the the
	   buffer returned is a global variable, thus it does not need to be free()d.
	*/
}

static void ds1315_input_raw_data( void )
{
	/* This routine is called when new date and time has been written to the
	   clock chip. Currently we ignore setting the date and time in the clock
	   chip.
	   
	   We always return the host's time when asked.
	*/
}


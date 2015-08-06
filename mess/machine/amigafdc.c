/***************************************************************************

	Amiga floppy disk controller emulation

***************************************************************************/

#include "mame.h"
#include "amiga.h"
#include "amigafdc.h"
#include "machine/6526cia.h"

/* required prototype */
static void setup_fdc_buffer( int drive );

typedef struct {
	int motor_on;
	int side;
	int dir;
	int wprot;
	int disk_changed;
	mame_file *f;
	int cyl;
	unsigned char mfm[544*2*11];
	int	cached;
	mame_timer *rev_timer;
	int rev_timer_started;
	int pos;
} fdc_def;

static fdc_def fdc_status[4];
/* signals */
static int fdc_sel = 0x0f;
static int fdc_dir = 0;
static int fdc_side = 1;
static int fdc_step = 1;
static int fdc_rdy = 1;

static void fdc_rev_proc( int drive );

static DEVICE_INIT(amiga_fdc)
{
	int id = image_index_in_device(image);
	fdc_status[id].motor_on = 0;
	fdc_status[id].side = 0;
	fdc_status[id].dir = 0;
	fdc_status[id].wprot = 1;
	fdc_status[id].cyl = 0;
	fdc_status[id].rev_timer = timer_alloc(fdc_rev_proc);
	fdc_status[id].rev_timer_started = 0;
	fdc_status[id].cached = -1;
	fdc_status[id].pos = 0;

	memset( fdc_status[id].mfm, 0xaa, 544*2*11 );
	return INIT_PASS;
}

static DEVICE_LOAD(amiga_fdc)
{
	int id = image_index_in_device(image);

	fdc_status[id].disk_changed = 1;
	fdc_status[id].f = file;
	fdc_status[id].disk_changed = 0;

	fdc_sel = 0x0f;
	fdc_dir = 0;
	fdc_side = 1;
	fdc_step = 1;
	fdc_rdy = 1;

	return INIT_PASS;
}

static int fdc_get_curpos( int drive ) {
	double elapsed;
	int speed;
	int bytes;
	int pos;

	if ( fdc_status[drive].rev_timer_started == 0 ) {
		logerror("Rev timer not started on drive %d, cant get position!\n", drive );
		return 0;
	}

	elapsed = timer_timeelapsed( fdc_status[drive].rev_timer );
	speed = ( CUSTOM_REG(REG_ADKCON) & 0x100 ) ? 2 : 4;

	bytes = elapsed / ( TIME_IN_USEC( speed * 8 ) );
	pos = bytes % ( 544*2*11 );

	return pos;
}

unsigned short amiga_fdc_get_byte( void ) {
	int pos;
	int i, drive = -1;
	unsigned short ret;

	ret = ( ( CUSTOM_REG(REG_DSKLEN) >> 1 ) & 0x4000 ) & ( ( CUSTOM_REG(REG_DMACON) << 10 ) & 0x4000 );
	ret |= ( CUSTOM_REG(REG_DSKLEN) >> 1 ) & 0x2000;

	for ( i = 0; i < 4; i++ ) {
		if ( !( fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 )
		return ret;

	if ( fdc_status[drive].disk_changed )
		return ret;

	setup_fdc_buffer( drive );

	pos = fdc_get_curpos( drive );

	if ( fdc_status[drive].mfm[pos] == ( CUSTOM_REG(REG_DSRSYNC) >> 8 ) &&
		 fdc_status[drive].mfm[pos+1] == ( CUSTOM_REG(REG_DSRSYNC) & 0xff ) )
			ret |= 0x1000;

	if ( pos != fdc_status[drive].pos ) {
		ret |= 0x8000;
		fdc_status[drive].pos = pos;
	}

	ret |= fdc_status[drive].mfm[pos];

	return ret;
}

static void fdc_dma_proc( int drive ) {

	if ( fdc_status[drive].disk_changed )
		return;

	setup_fdc_buffer( drive );

	if ( CUSTOM_REG(REG_DSKLEN) & 0x4000 )
	{
		logerror("Write to disk unsupported yet\n" );
	}
	else
	{
		offs_t offset = CUSTOM_REG_LONG(REG_DSKPTH);
		int cur_pos = fdc_status[drive].pos;
		int len = CUSTOM_REG(REG_DSKLEN) & 0x3fff;

		while ( len-- )
		{
			int dat = ( fdc_status[drive].mfm[cur_pos++] ) << 8;

			cur_pos %= ( 544 * 2 * 11 );

			dat |= fdc_status[drive].mfm[cur_pos++];

			cur_pos %= ( 544 * 2 * 11 );

			amiga_chip_ram_w(offset, dat);

			offset += 2;
		}
	}

	amiga_custom_w( 0x009c>>1, 0x8002, 0);
}

void amiga_fdc_setup_dma( void ) {
	int i, cur_pos, drive = -1;
	int time = 0;

	if ( ( CUSTOM_REG(REG_DSKLEN) & 0x8000 ) == 0 )
		return;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0210 ) == 0 )
		return;

	for ( i = 0; i < 4; i++ ) {
		if ( !( fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 ) {
		logerror("Disk DMA started with no drive selected!\n" );
		return;
	}

	if ( fdc_status[drive].disk_changed )
		return;

	setup_fdc_buffer( drive );

	fdc_status[drive].pos = cur_pos = fdc_get_curpos( drive );

	if ( CUSTOM_REG(REG_ADKCON) & 0x0400 ) { /* Wait for sync */
		if ( CUSTOM_REG(REG_DSRSYNC) != 0x4489 ) {
			logerror("Attempting to read a non-standard SYNC\n" );
		}

		i = cur_pos;
		do {
			if ( fdc_status[drive].mfm[i] == ( CUSTOM_REG(REG_DSRSYNC) >> 8 ) &&
				 fdc_status[drive].mfm[i+1] == ( CUSTOM_REG(REG_DSRSYNC) & 0xff ) )
				 	break;

			i++;
			i %= ( 544 * 2 * 11 );
			time++;
		} while( i != cur_pos );

		if ( i == cur_pos && time != 0 ) {
			logerror("SYNC not found on track!\n" );
			return;
		} else {
			fdc_status[drive].pos = i + 2;
		}

		time += ( CUSTOM_REG(REG_DSKLEN) & 0x3fff ) * 2;
		time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_set( TIME_IN_USEC( time ), drive, fdc_dma_proc );
	} else {
		time = ( CUSTOM_REG(REG_DSKLEN) & 0x3fff ) * 2;
		time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_set( TIME_IN_USEC( time ), drive, fdc_dma_proc );
	}
}

static void setup_fdc_buffer( int drive )
{
	int sector, offset, len;
	static unsigned char temp_cyl[512*11];

	/* no disk in drive */
	if ( fdc_status[drive].disk_changed )
		return;

	if ( fdc_status[drive].f == NULL ) {
		fdc_status[drive].disk_changed = 1;
		return;
	}

	len = 512*11;

	offset = ( fdc_status[drive].cyl << 1 ) | fdc_side;

	if ( fdc_status[drive].cached == offset )
		return;

	if ( mame_fseek( fdc_status[drive].f, offset * len, SEEK_SET ) ) {
		logerror("FDC: mame_fseek failed!\n" );
		fdc_status[drive].f = NULL;
		fdc_status[drive].disk_changed = 1;
	}

	mame_fread( fdc_status[drive].f, temp_cyl, len );

	for ( sector = 0; sector < 11; sector++ ) {
		unsigned char secbuf[544];
	    int i;
	    unsigned char *mfmbuf = ( &fdc_status[drive].mfm[544*2*sector] );
		unsigned long deven,dodd;
		unsigned long hck = 0,dck = 0;

	    secbuf[0] = secbuf[1] = 0x00;
	    secbuf[2] = secbuf[3] = 0xa1;
	    secbuf[4] = 0xff;
	    secbuf[5] = offset;
	    secbuf[6] = sector;
	    secbuf[7] = 11 - sector;

	    for ( i = 8; i < 24; i++ )
			secbuf[i] = 0;

	    memcpy( &secbuf[32], &temp_cyl[sector*512], 512 );

		mfmbuf[0*2] = 0xaa;
		mfmbuf[0*2+1] = 0xaa;
		mfmbuf[1*2] = 0xaa;
		mfmbuf[1*2+1] = 0xaa;
		mfmbuf[2*2] = 0x44;
		mfmbuf[2*2+1] = 0x89;
		mfmbuf[3*2] = 0x44;
		mfmbuf[3*2+1] = 0x89;

	    deven = ( ( secbuf[4] << 24) | ( secbuf[5] << 16 ) | ( secbuf[6] << 8 ) | ( secbuf[7] ) );
	    dodd = deven >> 1;
	    deven &= 0x55555555; dodd &= 0x55555555;

		mfmbuf[4*2] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
		mfmbuf[4*2+1] = ( ( dodd >> 16 ) ) & 0xff;
		mfmbuf[5*2] = ( dodd >> 8 ) & 0xff;
		mfmbuf[5*2+1] = dodd & 0xff;
		mfmbuf[6*2] = ( ( deven >> 16 ) >> 8 ) & 0xff;
		mfmbuf[6*2+1] = ( ( deven >> 16 ) ) & 0xff;
		mfmbuf[7*2] = ( deven >> 8 ) & 0xff;
		mfmbuf[7*2+1] = deven & 0xff;

		for ( i = 8; i < 48; i++ )
			mfmbuf[i*2] = mfmbuf[i*2+1] = 0xaa;

	    for (i = 0; i < 512; i += 4) {
			deven = ((secbuf[i+32] << 24) | (secbuf[i+33] << 16)
				 | (secbuf[i+34] << 8) | (secbuf[i+35]));
			dodd = deven >> 1;
			deven &= 0x55555555; dodd &= 0x55555555;

			mfmbuf[i + ( 32 * 2 )] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
			mfmbuf[i + ( 32 * 2 ) + 1] = ( dodd >> 16 ) & 0xff;
			mfmbuf[i + ( 33 * 2 )] = ( dodd >> 8 ) & 0xff;
			mfmbuf[i + ( 33 * 2 ) + 1] = dodd & 0xff;

			mfmbuf[i + ( ( 256 + 32 ) * 2 )] = ( ( deven >> 16 ) >> 8 ) & 0xff;
			mfmbuf[i + ( ( 256 + 32 ) * 2 ) + 1] = ( deven >> 16 ) & 0xff;
			mfmbuf[i + ( ( 256 + 33 ) * 2 )] = ( deven >> 8 ) & 0xff;
			mfmbuf[i + ( ( 256 + 33 ) * 2 ) + 1] = deven & 0xff;
	    }

	    for(i = 4; i < 24; i += 2)
			hck ^= ( ( mfmbuf[i*2] << 24) | ( mfmbuf[i*2+1] << 16 ) ) | ( ( mfmbuf[i*2+2] << 8 ) | mfmbuf[i*2+3] );

	    deven = dodd = hck; dodd >>= 1;

		mfmbuf[24*2] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
		mfmbuf[24*2+1] = ( ( dodd >> 16 ) ) & 0xff;
		mfmbuf[25*2] = ( dodd >> 8 ) & 0xff;
		mfmbuf[25*2+1] = dodd & 0xff;
		mfmbuf[26*2] = ( ( deven >> 16 ) >> 8 ) & 0xff;
		mfmbuf[26*2+1] = ( ( deven >> 16 ) ) & 0xff;
		mfmbuf[27*2] = ( deven >> 8 ) & 0xff;
		mfmbuf[27*2+1] = deven & 0xff;

	    for(i = 32; i < 544; i += 2)
			dck ^= ( ( mfmbuf[i*2] << 24) | ( mfmbuf[i*2+1] << 16 ) ) | ( ( mfmbuf[i*2+2] << 8 ) | mfmbuf[i*2+3] );

	    deven = dodd = dck; dodd >>= 1;

		mfmbuf[28*2] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
		mfmbuf[28*2+1] = ( ( dodd >> 16 ) ) & 0xff;
		mfmbuf[29*2] = ( dodd >> 8 ) & 0xff;
		mfmbuf[29*2+1] = dodd & 0xff;
		mfmbuf[30*2] = ( ( deven >> 16 ) >> 8 ) & 0xff;
		mfmbuf[30*2+1] = ( ( deven >> 16 ) ) & 0xff;
		mfmbuf[31*2] = ( deven >> 8 ) & 0xff;
		mfmbuf[31*2+1] = deven & 0xff;
	}

	fdc_status[drive].cached = offset;
}

static void fdc_rev_proc( int drive ) {
	int time;

	/* Issue a index pulse when a disk revolution completes */
	cia_issue_index(1);

	time = ( CUSTOM_REG(REG_ADKCON) & 0x100 ) ? 2 : 4;
	time *= ( 544 * 2 * 11 );
	time *= 8;
	timer_adjust(fdc_status[drive].rev_timer, TIME_IN_USEC( time ), drive, 0);
	fdc_status[drive].rev_timer_started = 1;
}

static void start_rev_timer( int drive ) {
	int time;

	if ( fdc_status[drive].rev_timer_started ) {
		logerror("Revolution timer started twice?!\n" );
		return;
	}

	time = ( CUSTOM_REG(REG_ADKCON) & 0x100 ) ? 2 : 4;
	time *= ( 544 * 2 * 11 );
	time *= 8;

	timer_adjust(fdc_status[drive].rev_timer, TIME_IN_USEC( time ), drive, 0);
	fdc_status[drive].rev_timer_started = 1;
}

static void stop_rev_timer( int drive ) {
	if ( fdc_status[drive].rev_timer_started == 0 ) {
		logerror("Revolution timer never started?!\n" );
		return;
	}

	timer_reset( fdc_status[drive].rev_timer, TIME_NEVER );
	fdc_status[drive].rev_timer_started = 0;
}

static void fdc_setup_leds( int drive ) {

	if ( drive == 0 )
		set_led_status( 1, fdc_status[drive].motor_on ); /* update internal drive led */

	if ( drive == 1 )
		set_led_status( 2, fdc_status[drive].motor_on ); /* update external drive led */
}

static void fdc_stepdrive( int drive ) {
	if ( fdc_dir ) {
		if ( fdc_status[drive].cyl )
			fdc_status[drive].cyl--;
	} else {
		if ( fdc_status[drive].cyl < 79 )
			fdc_status[drive].cyl++;
	}
}

static void fdc_motor( int drive, int off ) {
	int on = !off;

	if ( ( fdc_status[drive].motor_on == 0 ) && on ) {
		fdc_status[drive].pos = 0;

		start_rev_timer( drive );

	} else {
		if ( fdc_status[drive].motor_on && off )
			stop_rev_timer( drive );
	}

	fdc_status[drive].motor_on = on;
}

void amiga_fdc_control_w( UINT8 data ) {
	int step_pulse;
	int drive;

	if ( fdc_sel != ( ( data >> 3 ) & 15 ) )
		fdc_rdy = 0;

	fdc_sel = ( data >> 3 ) & 15;
    fdc_side = 1 - ( ( data >> 2 ) & 1 );
	fdc_dir = ( data >> 1 ) & 1;

	step_pulse = data & 1;

    if ( fdc_step != step_pulse ) {
		fdc_step = step_pulse;

    	if ( fdc_step == 0 ) {
		    for ( drive = 0; drive < 4; drive++ ) {
				if ( !( fdc_sel & ( 1 << drive ) ) )
				    fdc_stepdrive( drive );
			}
		}
	}

	for ( drive = 0; drive < 4; drive++ ) {
		if ( !( fdc_sel & ( 1 << drive ) ) ) {
			fdc_motor( drive, ( data >> 7 ) & 1 );
			fdc_setup_leds( drive );
		}
    }
}

int amiga_fdc_status_r( void ) {
	int drive, ret = 0x3c;

	for ( drive = 0; drive < 4; drive++ ) {
		if ( !( fdc_sel & ( 1 << drive ) ) ) {
			if ( fdc_status[drive].motor_on ) {
				if ( fdc_rdy )
					ret &= ~0x20;
				fdc_rdy = 1;
			} else {
				/* if we got a floppy image, then return we're on */
				if ( fdc_status[drive].disk_changed == 0 )
					ret &= ~0x20;
			}

			if ( fdc_status[drive].cyl == 0 )
				ret &= ~0x10;

			if ( fdc_status[drive].wprot )
				ret &= ~0x08;

			if ( fdc_status[drive].disk_changed )
				ret &= ~0x04;
		}
	}

	return ret;
}



void amiga_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:					info->i = IO_FLOPPY; break;
		case DEVINFO_INT_COUNT:					info->i = 4; break;
		case DEVINFO_INT_READABLE:				info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:				info->i = 0; break;
		case DEVINFO_INT_CREATABLE:				info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:					info->init = device_init_amiga_fdc; break;
		case DEVINFO_PTR_LOAD:					info->load = device_load_amiga_fdc; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "adf"); break;
	}
}

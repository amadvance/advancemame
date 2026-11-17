// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, hap, superctr, cam900, zanac

/* Taito Zoom sound board with an MN10200, ZSG-2 and TMS57002. */

#include <string.h>
#include "driver.h"
#include "state.h"
#include "streams.h"
#include "sound/custom.h"
#include "cpu/mn10200/mn10200.h"
#include "sound/zsg2.h"
#include "cpu/tms57002/tms57002.h"
#include "taitozoom.h"

#define ZSG2_RATE		( 25000000 / 768 )
#define TMS_CYCLES_PER_SAMPLE	( ( 25000000 / 2 ) / ZSG2_RATE )
#define ZOOM_CPU		1

static UINT8 zoom_shared[ 0x100 ];
static UINT16 zoom_ram[ 0x20000 / 2 ];
static UINT16 zoom_reg_address;
static UINT8 zoom_tms_ctrl;
static UINT8 zoom_gain[ 2 ];
static sound_stream *zoom_stream;

static int zoom_tms_line = -1;

static void zoom_tms_empty( int empty )
{

	if( empty != zoom_tms_line )
	{
		zoom_tms_line = empty;
		cpunum_set_input_line( ZOOM_CPU, MN10200_IRQ1, empty ? CLEAR_LINE : ASSERT_LINE );
	}
}

static void *zoom_dsp_timer;
static int zoom_dsp_pending;
static double zoom_dsp_left;

static void zoom_dsp_kick( int param )
{
	int before = tms57002dsp_fifo_count();
	zoom_dsp_pending = 0;
	stream_update( zoom_stream, 0 );

	if( !tms57002dsp_empty_r() && tms57002dsp_fifo_count() < before )
	{
		zoom_dsp_pending = 1;
		timer_adjust( zoom_dsp_timer, TIME_IN_HZ( ZSG2_RATE ), 0, 0 );
	}
}

static void zoom_dsp_arm( void )
{
	if( !tms57002dsp_empty_r() && !zoom_dsp_pending )
	{
		zoom_dsp_pending = 1;
		timer_adjust( zoom_dsp_timer, TIME_IN_HZ( ZSG2_RATE ), 0, 0 );
	}
}

static UINT8 zoom_port_r( int port )
{
	return port == 1 ? zoom_tms_ctrl : 0xff;
}

static void zoom_port_w( int port, UINT8 data )
{
	if( port != 1 )
		return;

	stream_update( zoom_stream, 0 );
	tms57002dsp_cload_w( data & 2 );
	tms57002dsp_pload_w( data & 1 );
	zoom_tms_ctrl = data;
}

static READ16_HANDLER( zoom_ram_r )
{
	return zoom_ram[ offset ];
}

static WRITE16_HANDLER( zoom_ram_w )
{
	COMBINE_DATA( &zoom_ram[ offset ] );
}

static READ16_HANDLER( zoom_zsg2_r )
{
	stream_update( zoom_stream, 0 );
	return zsg2_read( offset );
}

static WRITE16_HANDLER( zoom_zsg2_w )
{

	if( mem_mask != 0 )
		return;
	stream_update( zoom_stream, 0 );
	zsg2_write( offset, data );
}

static READ16_HANDLER( zoom_tms_r )
{
	stream_update( zoom_stream, 0 );
	return 0xff00 | tms57002dsp_data_r();
}

static WRITE16_HANDLER( zoom_tms_w )
{
	if( ACCESSING_LSB )
	{
		stream_update( zoom_stream, 0 );
		tms57002dsp_data_w( data & 0xff );
		zoom_dsp_arm();
	}
}

static READ16_HANDLER( zoom_shared16_r )
{
	return zoom_shared[ offset * 2 ] | ( zoom_shared[ offset * 2 + 1 ] << 8 );
}

static WRITE16_HANDLER( zoom_shared16_w )
{
	if( ACCESSING_LSB )
		zoom_shared[ offset * 2 ] = data & 0xff;
	if( ACCESSING_MSB )
		zoom_shared[ offset * 2 + 1 ] = data >> 8;
}

ADDRESS_MAP_START( taitozoom_mn_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x080000, 0x0fffff ) AM_ROM AM_REGION( REGION_CPU2, 0 )
	AM_RANGE( 0x400000, 0x41ffff ) AM_READWRITE( zoom_ram_r, zoom_ram_w )
	AM_RANGE( 0x800000, 0x8007ff ) AM_READWRITE( zoom_zsg2_r, zoom_zsg2_w )
	AM_RANGE( 0xc00000, 0xc00001 ) AM_READWRITE( zoom_tms_r, zoom_tms_w )
	AM_RANGE( 0xe00000, 0xe000ff ) AM_READWRITE( zoom_shared16_r, zoom_shared16_w )
ADDRESS_MAP_END

WRITE32_HANDLER( taitozoom_reg_w )
{
	if( !( mem_mask & 0xffff0000 ) )
		zoom_reg_address = ( data >> 16 ) & 0xff;
	if( ACCESSING_LSW32 )
	{
		switch( zoom_reg_address )
		{
		case 0x04:
			stream_update( zoom_stream, 0 );
			zoom_gain[ 0 ] = data & 0x3f;
			break;
		case 0x05:
			stream_update( zoom_stream, 0 );
			zoom_gain[ 1 ] = data & 0x3f;
			break;
		}
	}
}

WRITE32_HANDLER( taitozoom_sound_irq_w )
{
	if( ACCESSING_LSW32 )
		cpunum_set_input_line( ZOOM_CPU, MN10200_IRQ0, PULSE_LINE );
}

READ32_HANDLER( taitozoom_sound_irq_r )
{
	return 0;
}

READ32_HANDLER( taitozoom_shared_r )
{
	return zoom_shared[ ( offset * 2 ) & 0xff ] | ( zoom_shared[ ( offset * 2 + 1 ) & 0xff ] << 16 );
}

WRITE32_HANDLER( taitozoom_shared_w )
{
	if( ACCESSING_LSB32 )
		zoom_shared[ ( offset * 2 ) & 0xff ] = data & 0xff;
	if( !( mem_mask & 0x00ff0000 ) )
		zoom_shared[ ( offset * 2 + 1 ) & 0xff ] = ( data >> 16 ) & 0xff;
}

static void zoom_stream_update( void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length )
{
	static INT32 out[ 4 ][ 512 ];
	INT32 *outs[ 4 ];
	INT32 in[ 4 ], dsp[ 4 ];
	int i, pos = 0;

	outs[ 0 ] = out[ 0 ]; outs[ 1 ] = out[ 1 ]; outs[ 2 ] = out[ 2 ]; outs[ 3 ] = out[ 3 ];
	while( pos < length )
	{
		int n = length - pos;
		if( n > 512 )
			n = 512;
		zsg2_update( outs, n );
		for( i = 0; i < n; i++ )
		{
			INT32 l, r;

			in[ 0 ] = out[ 0 ][ i ] / 2;
			in[ 1 ] = out[ 1 ][ i ] / 2;
			in[ 2 ] = out[ 2 ][ i ];
			in[ 3 ] = out[ 3 ][ i ];
			tms57002dsp_sample( in, dsp, TMS_CYCLES_PER_SAMPLE );

			l = ( dsp[ 2 ] * zoom_gain[ 0 ] ) / 63;
			r = ( dsp[ 3 ] * zoom_gain[ 1 ] ) / 63;
			buffer[ 0 ][ pos + i ] = l < -32768 ? -32768 : l > 32767 ? 32767 : l;
			buffer[ 1 ][ pos + i ] = r < -32768 ? -32768 : r > 32767 ? 32767 : r;
		}
		pos += n;
	}
}

static void *zoom_sh_start( int clock, const struct CustomSound_interface *config )
{
	zsg2_init( memory_region( REGION_SOUND1 ), memory_region_length( REGION_SOUND1 ) );
	zsg2_reset();
	tms57002dsp_init();
	tms57002dsp_set_empty_callback( zoom_tms_empty );
	zoom_dsp_timer = timer_alloc( zoom_dsp_kick );
	tms57002dsp_reset();
	zoom_stream = stream_create( 0, 2, ZSG2_RATE, NULL, zoom_stream_update );
	return zoom_stream;
}

struct CustomSound_interface taitozoom_custom_interface =
{
	zoom_sh_start, NULL, NULL, NULL
};

static void taitozoom_presave( void )
{
	zoom_dsp_left = zoom_dsp_pending ? timer_timeleft( zoom_dsp_timer ) : -1;
}

static void taitozoom_postload( void )
{
	zoom_tms_line = -1;
	if( zoom_dsp_left >= 0 )
	{
		zoom_dsp_pending = 1;
		timer_adjust( zoom_dsp_timer, zoom_dsp_left, 0, 0 );
	}
	else
	{
		zoom_dsp_pending = 0;
		timer_adjust( zoom_dsp_timer, TIME_NEVER, 0, 0 );
		zoom_dsp_arm();
	}
}

void taitozoom_init( void )
{
	mn10200_set_port_handlers( zoom_port_r, zoom_port_w );
	memset( zoom_shared, 0, sizeof( zoom_shared ) );

	state_save_register_memory( "taitozoom", 0, "shared", zoom_shared, sizeof(UINT8), sizeof( zoom_shared ) );
	state_save_register_memory( "taitozoom", 0, "ram", zoom_ram, sizeof(UINT16), sizeof( zoom_ram ) / sizeof( zoom_ram[0] ) );
	state_save_register_memory( "taitozoom", 0, "reg_address", &zoom_reg_address, sizeof(UINT16), 1 );
	state_save_register_memory( "taitozoom", 0, "tms_ctrl", &zoom_tms_ctrl, sizeof(UINT8), 1 );
	state_save_register_memory( "taitozoom", 0, "gain", zoom_gain, sizeof(UINT8), 2 );
	zsg2_state_register();
	tms57002dsp_state_register();
	state_save_register_memory( "taitozoom", 0, "dsp_left", &zoom_dsp_left, sizeof(double), 1 );
	state_save_register_func_presave( taitozoom_presave );
	state_save_register_func_postload( taitozoom_postload );
}

void taitozoom_reset( void )
{
	timer_adjust( zoom_dsp_timer, TIME_NEVER, 0, 0 );
	zoom_dsp_pending = 0;
	zoom_tms_line = -1;
	zoom_reg_address = 0;
	zoom_tms_ctrl = 0;
	zoom_gain[ 0 ] = zoom_gain[ 1 ] = 63;
	zsg2_reset();
	tms57002dsp_reset();
}

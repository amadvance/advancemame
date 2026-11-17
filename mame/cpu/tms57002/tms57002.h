// license:BSD-3-Clause
// copyright-holders:Olivier Galibert

/* TMS57002 DASP interface. */

#ifndef TMS57002_H
#define TMS57002_H

#include "osd_cpu.h"

void tms57002dsp_init( void );
void tms57002dsp_reset( void );
void tms57002dsp_state_register( void );


UINT8 tms57002dsp_data_r( void );
void tms57002dsp_data_w( UINT8 data );
void tms57002dsp_pload_w( int state );
void tms57002dsp_cload_w( int state );
int tms57002dsp_empty_r( void );
int tms57002dsp_fifo_count( void );

void tms57002dsp_set_empty_callback( void (*callback)( int state ) );


void tms57002dsp_sample( const INT32 *in, INT32 *out, int cycles );

#endif

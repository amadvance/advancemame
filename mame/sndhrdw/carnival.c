// license:BSD-3-Clause
// copyright-holders:Zsolt Vasvari, hap
/*
 *  Carnival audio routines
 */

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/samples.h"
#include "includes/vicdual.h"


/* output port 0x01 definitions - sound effect drive outputs */
#define OUT_PORT_1_RIFLE        0x01
#define OUT_PORT_1_CLANG        0x02
#define OUT_PORT_1_DUCK1        0x04
#define OUT_PORT_1_DUCK2        0x08
#define OUT_PORT_1_DUCK3        0x10
#define OUT_PORT_1_PIPEHIT      0x20
#define OUT_PORT_1_BONUS1       0x40
#define OUT_PORT_1_BONUS2       0x80

/* output port 0x02 definitions - sound effect drive outputs */
#define OUT_PORT_2_BEAR         0x04
#define OUT_PORT_2_RANKING      0x20


#define PLAY(id,loop)           sample_start( id, id, loop )
#define STOP(id)                sample_stop( id )


/* sample file names */
const char *carnival_sample_names[] =
{
	"*carnival",
	"bear.wav",
	"bonus1.wav",
	"bonus2.wav",
	"clang.wav",
	"duck1.wav",
	"duck2.wav",
	"duck3.wav",
	"pipehit.wav",
	"ranking.wav",
	"rifle.wav",
	0
};


/* sample IDs - must match sample file name table above */
enum
{
	SND_BEAR = 0,
	SND_BONUS1,
	SND_BONUS2,
	SND_CLANG,
	SND_DUCK1,
	SND_DUCK2,
	SND_DUCK3,
	SND_PIPEHIT,
	SND_RANKING,
	SND_RIFLE
};


static UINT8 port1State;
static UINT8 port2State;
static UINT8 psgBus;
static UINT8 psgData;

void carnival_sh_start(void)
{
	port1State = 0;
	port2State = 0;
	psgBus = 0;
	psgData = 0;

	state_save_register_global(port1State);
	state_save_register_global(port2State);
	state_save_register_global(psgBus);
	state_save_register_global(psgData);
}


WRITE8_HANDLER( carnival_audio_1_w )
{
	int bitsChanged;
	int bitsGoneHigh;
	int bitsGoneLow;

	bitsChanged  = port1State ^ data;
	bitsGoneHigh = bitsChanged & data;
	bitsGoneLow  = bitsChanged & ~data;

	port1State = data;

	if ( bitsGoneLow & OUT_PORT_1_RIFLE )
	{
		PLAY( SND_RIFLE, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_CLANG )
	{
		PLAY( SND_CLANG, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_DUCK1 )
	{
		PLAY( SND_DUCK1, 1 );
	}
	if ( bitsGoneHigh & OUT_PORT_1_DUCK1 )
	{
		STOP( SND_DUCK1 );
	}

	if ( bitsGoneLow & OUT_PORT_1_DUCK2 )
	{
		PLAY( SND_DUCK2, 1 );
	}
	if ( bitsGoneHigh & OUT_PORT_1_DUCK2 )
	{
		STOP( SND_DUCK2 );
	}

	if ( bitsGoneLow & OUT_PORT_1_DUCK3 )
	{
		PLAY( SND_DUCK3, 1 );
	}
	if ( bitsGoneHigh & OUT_PORT_1_DUCK3 )
	{
		STOP( SND_DUCK3 );
	}

	if ( bitsGoneLow & OUT_PORT_1_PIPEHIT )
	{
		PLAY( SND_PIPEHIT, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_BONUS1 )
	{
		PLAY( SND_BONUS1, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_BONUS2 )
	{
		PLAY( SND_BONUS2, 0 );
	}
}


WRITE8_HANDLER( carnival_audio_2_w )
{
	int bitsChanged;
	int bitsGoneLow;

	bitsChanged  = port2State ^ data;
	bitsGoneLow  = bitsChanged & ~data;

	port2State = data;

	if ( bitsGoneLow & OUT_PORT_2_BEAR )
	{
		PLAY( SND_BEAR, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_RANKING )
	{
		PLAY( SND_RANKING, 0 );
	}

	// d4: music board MCU reset
	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
}


/* Music board */

static void carnival_psg_latch(void)
{
	if (psgBus & 1)
	{
		// BDIR W, BC1 selects address or data
		if (psgBus & 2)
			AY8910_control_port_0_w(0, psgData);
		else
			AY8910_write_port_0_w(0, psgData);
	}
}

WRITE8_HANDLER( carnival_music_port_1_w )
{
	// P1: ay8912 d0-d7
	psgData = data;
	carnival_psg_latch();
}

WRITE8_HANDLER( carnival_music_port_2_w )
{
	// P2 d6: AY8912 BDIR(R/W)
	// P2 d7: AY8912 BC1
	psgBus = data >> 6 & 3;
	carnival_psg_latch();
}


READ8_HANDLER( carnival_music_port_t1_r )
{
	// T1: comms from audio port 2 d3
	return ~port2State >> 3 & 1;
}

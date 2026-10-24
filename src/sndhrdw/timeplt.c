/***************************************************************************

    This code is used by the following module:

    timeplt.c
    pooyan.c
    locomotn.c
    tutankhm.c
    rocnrope.c

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/flt_rc.h"
#include "timeplt.h"
#include "sound/custom.h"
#include "streams.h"

static READ8_HANDLER( timeplt_portB_r );
static WRITE8_HANDLER( timeplt_filter_w );

ADDRESS_MAP_START( timeplt_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x3000, 0x33ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x4000) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x6000, 0x6000) AM_READ(AY8910_read_port_1_r)
ADDRESS_MAP_END

ADDRESS_MAP_START( timeplt_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x3000, 0x33ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(timeplt_filter_w)
ADDRESS_MAP_END

ADDRESS_MAP_START( peni_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x3000, 0x33ff) AM_RAM AM_MIRROR(0x0c00)
	AM_RANGE(0x4000, 0x4000) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w) AM_MIRROR(0x0fff)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(AY8910_control_port_0_w) AM_MIRROR(0x0fff)
	AM_RANGE(0x6000, 0x6000) AM_READWRITE(AY8910_read_port_1_r, AY8910_write_port_1_w) AM_MIRROR(0x0fff)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(AY8910_control_port_1_w) AM_MIRROR(0x0fff)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(timeplt_filter_w)
ADDRESS_MAP_END

ADDRESS_MAP_START( locomotn_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x4000) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x6000, 0x6000) AM_READ(AY8910_read_port_1_r)
ADDRESS_MAP_END

ADDRESS_MAP_START( locomotn_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(timeplt_filter_w)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(AY8910_control_port_1_w)
ADDRESS_MAP_END


struct AY8910interface timeplt_ay8910_interface =
{
	soundlatch_r,
	timeplt_portB_r
};


/* The timer clock which feeds the upper 4 bits of                      */
/* AY-3-8910 port A is based on the same clock                          */
/* feeding the sound CPU Z80.  It is a divide by                        */
/* 5120, formed by a standard divide by 512,                            */
/* followed by a divide by 10 using a 4 bit                             */
/* bi-quinary count sequence. (See LS90 data sheet                      */
/* for an example).                                                     */
/*                                                                      */
/* Bit 4 comes from the output of the divide by 1024                    */
/*       0, 1, 0, 1, 0, 1, 0, 1, 0, 1                                   */
/* Bit 5 comes from the QC output of the LS90 producing a sequence of   */
/*       0, 0, 1, 1, 0, 0, 1, 1, 1, 0                                   */
/* Bit 6 comes from the QD output of the LS90 producing a sequence of   */
/*       0, 0, 0, 0, 1, 0, 0, 0, 0, 1                                   */
/* Bit 7 comes from the QA output of the LS90 producing a sequence of   */
/*       0, 0, 0, 0, 0, 1, 1, 1, 1, 1                                   */

static int timeplt_timer[10] =
{
	0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0
};

static READ8_HANDLER( timeplt_portB_r )
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = activecpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 5120;

	last_totalcycles = current_totalcycles;

	return timeplt_timer[clock/512];
}


static void filter_w(int chip, int channel, int data)
{
	int C = 0;

	if (data & 1) C += 220000;	/* 220000pF = 0.220uF */
	if (data & 2) C +=  47000;	/*  47000pF = 0.047uF */
	filter_rc_set_RC(3*chip + channel,1000,5100,0,C);
}

static WRITE8_HANDLER( timeplt_filter_w )
{
	filter_w(0, 0, (offset >>  6) & 3);
	filter_w(0, 1, (offset >>  8) & 3);
	filter_w(0, 2, (offset >> 10) & 3);
	filter_w(1, 0, (offset >>  0) & 3);
	filter_w(1, 1, (offset >>  2) & 3);
	filter_w(1, 2, (offset >>  4) & 3);
}


WRITE8_HANDLER( timeplt_sh_irqtrigger_w )
{
	static int last;

	if (last == 0 && data)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpunum_set_input_line_and_vector(1,0,HOLD_LINE,0xff);
	}

	last = data;
}


static struct
{
	sound_stream *stream;
	UINT32 address, stop;
	INT32 delta, output, phase;
	UINT8 playing, bitcount, bitrate, history, command, phrase;
} chkun_voice;

static void chkun_voice_rate(void)
{
	stream_set_sample_rate(chkun_voice.stream, 512000 / (16 * (4 - chkun_voice.bitrate)));
}

static void chkun_voice_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	const UINT8 *rom = memory_region(REGION_SOUND1);
	int mask = memory_region_length(REGION_SOUND1) - 1;
	int mix = 0, i;
	for (i = 0; i < samples; i++)
	{
		if (chkun_voice.playing)
		{
			int bit = (rom[chkun_voice.address] >> chkun_voice.bitcount) & 1;
			chkun_voice.bitcount = (chkun_voice.bitcount + 1) & 7;
			if (!chkun_voice.bitcount)
			{
				chkun_voice.address = (chkun_voice.address + 1) & mask;
				if (chkun_voice.address == chkun_voice.stop) chkun_voice.playing = 0;
			}
			if ((chkun_voice.history & 7) == 0 || (chkun_voice.history & 7) == 7)
			{
				if (chkun_voice.delta < 0x2000) chkun_voice.delta *= 2;
			}
			else chkun_voice.delta -= 8;
			if (chkun_voice.delta < 1) chkun_voice.delta = 1;
			chkun_voice.output += bit ? chkun_voice.delta : -chkun_voice.delta;
			chkun_voice.output = MAX(-32768, MIN(32767, chkun_voice.output));
			chkun_voice.history = (chkun_voice.history << 1) | bit;
			mix = chkun_voice.output;
		}
		outputs[0][i] = mix;
	}
}

static void chkun_voice_reset(void *token)
{
	stream_update(chkun_voice.stream, 0);
	chkun_voice.playing = 0;
	chkun_voice.address = 0x100;
	chkun_voice.bitcount = chkun_voice.history = chkun_voice.bitrate = 0;
	chkun_voice.delta = 1;
	chkun_voice.output = chkun_voice.phase = chkun_voice.command = chkun_voice.phrase = 0;
	chkun_voice.stop = 0;
	chkun_voice_rate();
}

static void chkun_voice_data(UINT8 data)
{
	const UINT8 *rom = memory_region(REGION_SOUND1);
	UINT32 mask = memory_region_length(REGION_SOUND1) - 1;
	stream_update(chkun_voice.stream, 0);
	data &= 15;
	if (!chkun_voice.phase)
	{
		chkun_voice.command = data;
		switch (data)
		{
			case 2: chkun_voice.playing = 1; break;
			case 3: chkun_voice.playing = 0; break;
			case 4: case 5: case 6: case 7: chkun_voice.phase = 1; break;
		}
		return;
	}
	switch (chkun_voice.command)
	{
		case 4:
			chkun_voice.address = (chkun_voice.address & ~(15 << (4 * chkun_voice.phase))) |
				(data << (4 * chkun_voice.phase));
			if (chkun_voice.phase == 5)
			{
				chkun_voice.address &= mask;
				chkun_voice.bitcount = 0;
				chkun_voice.phase = -1;
			}
			break;
		case 5:
			chkun_voice.stop = (chkun_voice.stop & ~(15 << (4 * chkun_voice.phase))) |
				(data << (4 * chkun_voice.phase));
			if (chkun_voice.phase == 5)
			{
				chkun_voice.stop &= mask;
				chkun_voice.phase = -1;
			}
			break;
		case 6:
			chkun_voice.bitrate = data & 3;
			chkun_voice_rate();
			chkun_voice.phase = -1;
			break;
		case 7:
			if (chkun_voice.phase == 1) chkun_voice.phrase = (chkun_voice.phrase & 0x30) | data;
			else
			{
				UINT8 pos;
				chkun_voice.phrase = (chkun_voice.phrase & 15) | ((data << 4) & 0x30);
				pos = chkun_voice.phrase * 4;
				chkun_voice.address = (rom[pos] | (rom[pos | 1] << 8) | (rom[pos | 2] << 16)) & mask;
				pos += 4;
				chkun_voice.stop = (rom[pos] | (rom[pos | 1] << 8) | (rom[pos | 2] << 16)) & mask;
				chkun_voice.bitcount = chkun_voice.history = 0;
				chkun_voice.output = 0;
				chkun_voice.delta = 1;
				chkun_voice.playing = 1;
				chkun_voice.phase = -1;
			}
			break;
		default: chkun_voice.phase = -1; break;
	}
	chkun_voice.phase++;
}

static WRITE8_HANDLER( chkun_voice_w )
{
	if (!(data & 0x40)) chkun_voice_data(data & 15);
	if (!(data & 0x10)) chkun_voice_reset(NULL);
}

static void *chkun_voice_start(int clock, const struct CustomSound_interface *config)
{
	memset(&chkun_voice, 0, sizeof(chkun_voice));
	chkun_voice.stream = stream_create(0, 1, clock / 64, NULL, chkun_voice_update);
	state_save_register_global(chkun_voice.address);
	state_save_register_global(chkun_voice.stop);
	state_save_register_global(chkun_voice.delta);
	state_save_register_global(chkun_voice.output);
	state_save_register_global(chkun_voice.phase);
	state_save_register_global(chkun_voice.playing);
	state_save_register_global(chkun_voice.bitcount);
	state_save_register_global(chkun_voice.bitrate);
	state_save_register_global(chkun_voice.history);
	state_save_register_global(chkun_voice.command);
	state_save_register_global(chkun_voice.phrase);
	state_save_register_func_postload(chkun_voice_rate);
	return &chkun_voice;
}

struct CustomSound_interface chkun_voice_interface =
{
	chkun_voice_start, NULL, chkun_voice_reset, NULL
};
struct AY8910interface chkun_ay8910_interface =
{
	NULL, NULL, chkun_voice_w, NULL
};

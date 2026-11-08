// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Tatsumi TX-1/Buggy Boy hardware

***************************************************************************/
#include "driver.h"
#include "streams.h"
#include "sound/ay8910.h"
#include "vidhrdw/res_net.h"
#include "includes/tx1.h"

static sound_stream *stream;
static UINT32 freq_to_step;
static UINT32 step0;
static UINT32 step1;
static UINT32 step2;

static struct
{
	union
	{
#ifdef LSB_FIRST
		struct { UINT8 LSB; UINT8 MSB; };
#else
		struct { UINT8 MSB; UINT8 LSB; };
#endif
		UINT16 val;
	} counts[3];

	int idx[3];
} pit8253;

WRITE8_HANDLER( tx1_pit8253_w )
{
	/* Finish samples at the previous counter value before changing engine pitch. */
	stream_update(stream, 0);

	if (offset < 3)
	{
		if (pit8253.idx[offset] == 0)
		{
			pit8253.counts[offset].LSB = data;
			pit8253.idx[offset] = 1;
		}
		else
		{
			pit8253.counts[offset].MSB = data;
			pit8253.idx[offset] = 0;
		}
	}
	else
	{
		int mode = (data >> 1) & 7;

		if (mode == 3)
		{
			int cntsel = (data >> 6) & 3;
			pit8253.idx[cntsel] = 0;
			pit8253.counts[cntsel].val = 0;
		}
		else
			logerror("PIT8253: Unsupported mode %d.\n", mode);
	}
}

READ8_HANDLER( tx1_pit8253_r )
{
	logerror("PIT R: %x", offset);
	return 0;
}

#define TX1_NOISE_CLOCK		(1/(1000.0e-12 * 560000.0))
#define TX1_PIT_CLOCK		(TX1_PIXEL_CLOCK / 16)
#define TX1_FRAC			30

#define TX1_SHUNT			(250.0)
#define TX1_R0				(180000.0 + TX1_SHUNT)
#define TX1_R1				(56000.0  + TX1_SHUNT)
#define TX1_R2				(22000.0  + TX1_SHUNT)
#define TX1_R				(100000.0 + TX1_SHUNT)
#define TX1_RI				(180000.0)

static const double tx1_engine_gains[16] =
{
	-( TX1_R )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_R0) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_R1) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_R1 + 1.0/TX1_R0) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_R2) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_R2 + 1.0/TX1_R0) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_R2 + 1.0/TX1_R1) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_R2 + 1.0/TX1_R1 + 1.0/TX1_R0) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT ) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT + 1.0/TX1_R0) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT + 1.0/TX1_R1) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT + 1.0/TX1_R1 + 1.0/TX1_R0) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT + 1.0/TX1_R2) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT + 1.0/TX1_R2 + 1.0/TX1_R0) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT + 1.0/TX1_R2 + 1.0/TX1_R1) )/TX1_RI,
	-( 1.0/(1.0/TX1_R + 1.0/TX1_SHUNT + 1.0/TX1_R2 + 1.0/TX1_R1 + 1.0/TX1_R0) )/TX1_RI
};

static UINT8 ay_outputa;
static UINT8 ay_outputb;

static double weights0[4], weights1[3], weights2[3];
static int eng0[4];
static int eng1[4];
static int eng2[4];

WRITE8_HANDLER( tx1_ay8910_a_w )
{
	stream_update(stream, 0);

	ay_outputa = ~data;
}

WRITE8_HANDLER( tx1_ay8910_b_w )
{
	double gain;

	stream_update(stream, 0);

	ay_outputb = data ^ 0xf;

	gain = BIT(ay_outputb, 4) ? 1.5 : 2.0;
	sndti_set_output_gain(SOUND_AY8910, 0, 0, gain);
	sndti_set_output_gain(SOUND_AY8910, 0, 1, gain);
	sndti_set_output_gain(SOUND_AY8910, 0, 2, gain);
}

INLINE void update_engine (int eng[4])
{
	int p0 = eng[0];
	int p1 = eng[1];
	int p2 = eng[2];
	int p3 = eng[3];

	eng[0] = !p0;
	if (p0 && !eng[0]) eng[1] = !p1;
	eng[2] = !(p2 && !p3);
	eng[3] = !p2;
}

static void tx1_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	UINT32 step_0, step_1, step_2;
	double gain_0, gain_1, gain_2, gain_3;

	stream_sample_t *fl = &outputs[0][0];
	stream_sample_t *fr = &outputs[1][0];

	static stream_sample_t pit0 = 0;
	static stream_sample_t pit1 = 0;
	static stream_sample_t pit2 = 0;

	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	memset(outputs[1], 0, samples * sizeof(*outputs[1]));

	step_0 = pit8253.counts[0].val ? (TX1_PIT_CLOCK / pit8253.counts[0].val) * freq_to_step : 0;
	step_1 = pit8253.counts[1].val ? (TX1_PIT_CLOCK / pit8253.counts[1].val) * freq_to_step : 0;
	step_2 = pit8253.counts[2].val ? (TX1_PIT_CLOCK / pit8253.counts[2].val) * freq_to_step : 0;

	gain_0 = tx1_engine_gains[ay_outputa & 0xf];
	gain_1 = tx1_engine_gains[ay_outputa >> 4];
	gain_2 = tx1_engine_gains[ay_outputb & 0xf];
	gain_3 = BIT(ay_outputb, 5) ? 1.0f : 1.5f;

	while (samples--)
	{
		if (step0 & ((1 << TX1_FRAC)))
		{
			update_engine(eng0);
			pit0 = combine_4_weights(weights0, eng0[0], eng0[1], eng0[2], eng0[3]);
			step0 &= ((1 << TX1_FRAC) - 1);
		}

		if (step1 & ((1 << TX1_FRAC)))
		{
			update_engine(eng1);
			pit1 = combine_3_weights(weights1, eng1[0], eng1[1], eng1[3]);
			step1 &= ((1 << TX1_FRAC) - 1);
		}

		if (step2 & ((1 << TX1_FRAC)))
		{
			update_engine(eng2);
			pit2 = combine_3_weights(weights2, eng2[0], eng2[1], eng2[3]);
			step2 &= ((1 << TX1_FRAC) - 1);
		}

		*fl++ = (pit0 + pit1)*gain_3 + 2*pit2*gain_2;
		*fr++ = (pit0 + pit1)*gain_3 + 2*pit2*gain_2;

		step0 += step_0;
		step1 += step_1;
		step2 += step_2;
	}
}

static void tx1_sound_reset(void *token);

static void *tx1_sound_start(int clock, const struct CustomSound_interface *config)
{
	static const int r0[4] = { 390e3, 180e3, 180e3, 180e3 };
	static const int r1[3] = { 180e3, 390e3, 56e3 };
	static const int r2[3] = { 390e3, 390e3, 180e3 };

	stream = stream_create( 0, 2, Machine->sample_rate, NULL, tx1_stream_update);
	freq_to_step = (double)(1 << TX1_FRAC) / (double)Machine->sample_rate;

	compute_resistor_weights(0,	10000, -1.0,
			4, &r0[0], weights0, 0, 0,
			3, &r1[0], weights1, 0, 0,
			3, &r2[0], weights2, 0, 0);
	tx1_sound_reset(0);
	return stream;
}

static void tx1_sound_reset(void *token)
{
	step0 = step1 = step2 = 0;
}



#define BUGGYBOY_PIT_CLOCK		(BUGGYBOY_ZCLK / 8)
#define BUGGYBOY_NOISE_CLOCK	(BUGGYBOY_PIT_CLOCK / 4)

#define BUGGYBOY_R1		47000.0
#define BUGGYBOY_R2		22000.0
#define BUGGYBOY_R3		10000.0
#define BUGGYBOY_R4		5600.0
#define BUGGYBOY_SHUNT	250.0

#define BUGGYBOY_R1S	(1.0/(1.0/BUGGYBOY_R1 + 1.0/BUGGYBOY_SHUNT))
#define BUGGYBOY_R2S	(1.0/(1.0/BUGGYBOY_R2 + 1.0/BUGGYBOY_SHUNT))
#define BUGGYBOY_R3S	(1.0/(1.0/BUGGYBOY_R3 + 1.0/BUGGYBOY_SHUNT))
#define BUGGYBOY_R4S	(1.0/(1.0/BUGGYBOY_R4 + 1.0/BUGGYBOY_SHUNT))

static int noise_lfsra;
static int noise_lfsrb;
static int noise_lfsrc;
static int noise_lfsrd;
static int noise_counter;
static UINT8 ym1_outputa;
static UINT8 ym2_outputa;
static UINT8 ym2_outputb;
static UINT16 buggyboy_eng_voltages[16];

static const double bb_engine_gains[16] =
{
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2  + BUGGYBOY_R3  + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2  + BUGGYBOY_R3  + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2  + BUGGYBOY_R3S + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2  + BUGGYBOY_R3S + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2S + BUGGYBOY_R3  + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2S + BUGGYBOY_R3  + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2S + BUGGYBOY_R3S + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1  + BUGGYBOY_R2S + BUGGYBOY_R3S + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2  + BUGGYBOY_R3  + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2  + BUGGYBOY_R3  + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2  + BUGGYBOY_R3S + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2  + BUGGYBOY_R3S + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2S + BUGGYBOY_R3  + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2S + BUGGYBOY_R3  + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2S + BUGGYBOY_R3S + BUGGYBOY_R4 ) + 1.0/100e3)/100e3,
	-1.0/(1.0/(BUGGYBOY_R1S + BUGGYBOY_R2S + BUGGYBOY_R3S + BUGGYBOY_R4S) + 1.0/100e3)/100e3,
};

WRITE8_HANDLER( bb_ym1_a_w )
{
	stream_update(stream, 0);
	ym1_outputa = data ^ 0xff;
}

WRITE8_HANDLER( bb_ym2_a_w )
{
	stream_update(stream, 0);
	ym2_outputa = data ^ 0xff;
}

WRITE8_HANDLER( bb_ym2_b_w )
{
	double gain;

	stream_update(stream, 0);

	ym2_outputb = data ^ 0xff;

	if (!strcmp(Machine->gamedrv->name, "buggyb1"))
	{
		coin_counter_w(0, data & 0x01);
		coin_counter_w(1, data & 0x02);
	}

	gain = data & 0x80 ? 1.0 : 2.0;
	sndti_set_output_gain(SOUND_AY8910, 0, 0, gain);
	sndti_set_output_gain(SOUND_AY8910, 0, 1, gain);
	sndti_set_output_gain(SOUND_AY8910, 0, 2, gain);

	gain = data & 0x40 ? 1.0 : 2.0;
	sndti_set_output_gain(SOUND_AY8910, 1, 0, gain);
	sndti_set_output_gain(SOUND_AY8910, 1, 1, gain);
	sndti_set_output_gain(SOUND_AY8910, 1, 2, gain);
}

static void buggyboy_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	UINT32 step_0, step_1;
	int n1_en, n2_en;
	double gain0, gain1_l, gain1_r;

	stream_sample_t *fl = &outputs[0][0];
	stream_sample_t *fr = &outputs[1][0];

	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	memset(outputs[1], 0, samples * sizeof(*outputs[1]));

	step_0 = pit8253.counts[0].val ? (BUGGYBOY_PIT_CLOCK / pit8253.counts[0].val) * freq_to_step : 0;
	step_1 = pit8253.counts[1].val ? (BUGGYBOY_PIT_CLOCK / pit8253.counts[1].val) * freq_to_step : 0;

	if (!strcmp(Machine->gamedrv->name, "buggyb1"))
		gain0 = BIT(ym2_outputb, 3) ? 1.0 : 2.0;
	else
		gain0 = BIT(ym1_outputa, 3) ? 1.0 : 2.0;

	n1_en = BIT(ym2_outputb, 4);
	n2_en = BIT(ym2_outputb, 5);

	gain1_l = bb_engine_gains[ym2_outputa >> 4] * 5;
	gain1_r = bb_engine_gains[ym2_outputa & 0xf] * 5;

	while (samples--)
	{
		int i;
		stream_sample_t pit0, pit1, n1, n2;
		pit0 = buggyboy_eng_voltages[(step0 >> 24) & 0xf];
		pit1 = buggyboy_eng_voltages[(step1 >> 24) & 0xf];

		for (i = 0; i < BUGGYBOY_NOISE_CLOCK / Machine->sample_rate; ++i)
		{

			int p13 = BIT(noise_lfsra, 3);
			int p12 = BIT(noise_lfsrb, 4);
			int p10 = BIT(noise_lfsrc, 3);
			int p8 = BIT(noise_lfsrd, 3);

			noise_lfsra = p12 | ((noise_lfsra << 1) & 0xf);
			noise_lfsrb = (p8 ^ p12) | ((noise_lfsrb << 1) & 0x1f);
			noise_lfsrc = p13 | ((noise_lfsrc << 1) & 0xf);
			noise_lfsrd = p10 | ((noise_lfsrd << 1) & 0x1f);

			if ( !BIT(noise_lfsrc, 3) && p10 )
				noise_counter = (noise_counter + 1) & 0x0fff;
		}

		if (n1_en)
		{
			n1 = !BIT(noise_counter, 7-1) * 16000;
			if ( BIT(noise_counter, 11-1) ) n1 /=2;
		}
		else
			n1 = 8192;

		if (n2_en)
		{
			n2 = !BIT(noise_counter, 6-1) * 16000;
			if ( BIT(noise_counter, 11-1) ) n2 /=2;
		}
		else
			n2 = 8192;

		*fl++ = n1 + n2 + (pit0 * gain0) + (pit1 * gain1_l);
		*fr++ = n1 + n2 + (pit0 * gain0) + (pit1 * gain1_r);

		step0 += step_0;
		step1 += step_1;
	}
}

static void buggyboy_sound_reset(void *token);

static void *buggyboy_sound_start(int clock, const struct CustomSound_interface *config)
{
	static const int resistors[4] = { 330000, 220000, 330000, 220000 };
	double aweights[4];
	int i;
	static const int tmp[16] =
	{
		0x0, 0x1, 0xe, 0xf, 0x8, 0x9, 0x6, 0x7, 0xc, 0xd, 0xe, 0xf, 0x4, 0x5, 0x6, 0x7
	};

	compute_resistor_weights(0,	16384,	-1.0,
							4,	&resistors[0], aweights, 0,	0,
							0, 0, 0, 0, 0,
							0, 0, 0, 0, 0 );

	for (i = 0; i < 16; i++)
		buggyboy_eng_voltages[i] = combine_4_weights(aweights, BIT(tmp[i], 0), BIT(tmp[i], 1), BIT(tmp[i], 2), BIT(tmp[i], 3));

	stream = stream_create( 0, 2, Machine->sample_rate, NULL, buggyboy_stream_update);
	freq_to_step = (double)(1 << 24) / (double)Machine->sample_rate;
	buggyboy_sound_reset(0);
	return stream;
}

static void buggyboy_sound_reset(void *token)
{
	step0 = step1 = 0;

	noise_lfsra = 0;
	noise_lfsrb = 1;
	noise_lfsrc = 0;
	noise_lfsrd = 0;
}




struct CustomSound_interface tx1_custom_interface = { tx1_sound_start, 0, tx1_sound_reset, 0 };
struct CustomSound_interface buggyboy_custom_interface = { buggyboy_sound_start, 0, buggyboy_sound_reset, 0 };

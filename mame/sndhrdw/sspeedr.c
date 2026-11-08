// license:CC0-1.0
// copyright-holders:Colin Douglas Howell
/* Super Speed Race analog sound circuits. */

#include "driver.h"
#include "sspeedr.h"

#define SSR_PEDAL NODE_01
#define SSR_HIGH_GEAR NODE_02
#define SSR_LOW_GEAR NODE_03
#define SSR_BOOM NODE_04
#define SSR_ENGINE_ON NODE_05
#define SSR_SKID NODE_06
#define SSR_CRASH NODE_07
#define SSR_SOUND_ON NODE_08

/* The CD4006 sections form a 17-stage chain with feedback after 5 and
 * 17 clocks. The sound output is taken after the thirteenth stage. */
static const struct discrete_lfsr_desc sspeedr_noise =
{
	DISC_CLK_IS_FREQ, 17, 1, 4, 16,
	DISC_LFSR_XOR, DISC_LFSR_OR, DISC_LFSR_REPLACE,
	1, 0, 12
};

/* Convert the inverting comparator bias to the equivalent non-inverting
 * Norton VCO thresholds. Keep the three integrator RC pairs distinct. */
#define SSR_VCO_BIAS (4.5 / (4.5 / 470000.0 + 4.0 / 270000.0))
#define SSR_VCO(r1,r2,c) \
	{ DISC_OP_AMP_OSCILLATOR_VCO_1 | DISC_OP_AMP_IS_NORTON, \
		r1, r2, 100000, 270000, SSR_VCO_BIAS, 0, 1, 0, c, 5 }

static const struct discrete_op_amp_osc_info sspeedr_engine_vco[3] =
{
	SSR_VCO(560000, 270000, 0.022e-6),
	SSR_VCO(300000, 150000, 0.01e-6),
	SSR_VCO(220000, 100000, 0.01e-6)
};

DISCRETE_SOUND_START( sspeedr_discrete )
	DISCRETE_INPUTX_DATA(SSR_PEDAL, 0.02, 0, 0)
	DISCRETE_INPUT_LOGIC(SSR_HIGH_GEAR)
	DISCRETE_INPUT_LOGIC(SSR_LOW_GEAR)
	DISCRETE_INPUT_LOGIC(SSR_BOOM)
	DISCRETE_INPUT_NOT(SSR_ENGINE_ON)
	DISCRETE_INPUT_LOGIC(SSR_SKID)
	DISCRETE_INPUT_LOGIC(SSR_CRASH)
	DISCRETE_INPUTX_NOT(SSR_SOUND_ON, 1, 0, 1)

	/* C30 smooths the weighted pedal currents before C18 stores speed.
	 * C19 adds much more inertia in high gear than in low gear. */
	DISCRETE_RCFILTER(NODE_10, 1, SSR_PEDAL, 100000, 1e-6)
	DISCRETE_SWITCH(NODE_11, 1, SSR_ENGINE_ON, 0, NODE_10)
	DISCRETE_RCFILTER(NODE_12, 1, NODE_11, 47000, 22e-6)
	DISCRETE_RCDISC2(NODE_13, SSR_HIGH_GEAR, NODE_12, 10000, NODE_12, 560000, 2.2e-6)
	DISCRETE_RCFILTER(NODE_16, 1, NODE_12, 10000, 2.2e-6)
	DISCRETE_SWITCH(NODE_17, 1, SSR_LOW_GEAR, NODE_13, NODE_16)
	DISCRETE_MULTADD(NODE_14, 1, NODE_17, 1, -0.4)
	DISCRETE_CLAMP(NODE_15, 1, NODE_14, 0.6, 4.5, 0.6)

	DISCRETE_OP_AMP_VCO1(NODE_20, 1, NODE_15, &sspeedr_engine_vco[0])
	DISCRETE_OP_AMP_VCO1(NODE_21, 1, NODE_15, &sspeedr_engine_vco[1])
	DISCRETE_OP_AMP_VCO1(NODE_22, 1, NODE_15, &sspeedr_engine_vco[2])
	DISCRETE_ADDER2(NODE_23, 1, NODE_20, NODE_21)
	DISCRETE_CRFILTER(NODE_24, 1, NODE_23, 10000, 10e-6)
	/* The third oscillator controls the MC3340 attenuation. Its nonlinear
	 * gain and the diode mixer are approximated by amplitude modulation. */
	DISCRETE_MULTADD(NODE_25, 1, NODE_22, 0.25, 0.1)
	DISCRETE_MULTIPLY(NODE_26, SSR_ENGINE_ON, NODE_24, NODE_25)

	DISCRETE_LFSR_NOISE(NODE_30, 1, 1, 4000, 1, 0, 0, &sspeedr_noise)
	DISCRETE_FILTER2(NODE_31, 1, NODE_30, 2500, 0.7, DISC_FILTER_LOWPASS)
	DISCRETE_FILTER2(NODE_32, 1, NODE_31, 1024, 0.08, DISC_FILTER_BANDPASS)
	/* Gate the excitation rather than its output so the crash filter can ring down. */
	DISCRETE_MULTIPLY(NODE_42, SSR_BOOM, NODE_31, 1)
	DISCRETE_FILTER2(NODE_33, 1, NODE_42, 133, 0.05, DISC_FILTER_BANDPASS)
	DISCRETE_FILTER2(NODE_34, 1, NODE_31, 1500, 0.7, DISC_FILTER_LOWPASS)
	DISCRETE_GAIN(NODE_35, NODE_32, 0.25)
	DISCRETE_GAIN(NODE_36, NODE_33, 0.5)
	DISCRETE_MULTIPLY(NODE_37, SSR_SKID, NODE_35, 1)
	DISCRETE_MULTIPLY(NODE_39, SSR_CRASH, NODE_34, 1)

	DISCRETE_ADDER4(NODE_40, 1, NODE_26, NODE_37, NODE_36, NODE_39)
	DISCRETE_CRFILTER(NODE_41, 1, NODE_40, 100000, 0.1e-6)
	DISCRETE_MULTIPLY(NODE_43, SSR_SOUND_ON, NODE_41, 1)
	DISCRETE_OUTPUT(NODE_43, 6000)
DISCRETE_SOUND_END

WRITE8_HANDLER( sspeedr_sound_w )
{
	if (offset == 0)
	{
		double pedal = 0.5 + 4.5 * 100000 / 470000;
		static const double resistance[4] = { 2200000, 1000000, 470000, 240000 };
		int i;

		for (i = 0; i < 4; i++)
			if (data & (1 << i))
				pedal += 4.5 * 100000 / resistance[i];

		/* Scale the control voltage to fit the eight-bit discrete input latch. */
		discrete_sound_w(SSR_PEDAL, pedal * 50);
		discrete_sound_w(SSR_HIGH_GEAR, data & 0x10);
		discrete_sound_w(SSR_LOW_GEAR, data & 0x20);
	}
	else
	{
		discrete_sound_w(SSR_BOOM, data & 0x01);
		discrete_sound_w(SSR_ENGINE_ON, data & 0x02);
		discrete_sound_w(SSR_SKID, data & 0x04);
		discrete_sound_w(SSR_CRASH, data & 0x08);
		discrete_sound_w(SSR_SOUND_ON, data & 0x10);
	}
}

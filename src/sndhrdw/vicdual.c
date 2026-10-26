// license:BSD-3-Clause
// copyright-holders:Derrick Renaud, Couriersud, Ariane Fugmann, Jim Hernandez
/*************************************************************************

    sndhrdw\vicdual.c

*************************************************************************/

#include "driver.h"
#include <math.h>
#include "vicdual.h"
#include "sound/discrete.h"
#include "sound/mw_discrete.h"
#include "sound/samples.h"


/************************************************************************
 * frogs Sound System Analog emulation
 * Oct 2004, Derrick Renaud
 ************************************************************************/

/* Discrete Sound Input Nodes */
#define FROGS_FLY_EN		NODE_01
#define FROGS_JUMP_EN		NODE_03
#define FROGS_HOP_EN		NODE_04
#define FROGS_TONGUE_EN		NODE_05
#define FROGS_CAPTURE_EN	NODE_06
#define FROGS_SPLASH_EN		NODE_08

/* Nodes - Sounds */
#define FROGS_BUZZZ_SND		NODE_11
#define FROGS_BOING_SND		NODE_13
#define FROGS_HOP_SND		NODE_14
#define FROGS_ZIP_SND		NODE_15
#define FROGS_CROAK_SND		NODE_16
#define FROGS_SPLASH_SND	NODE_18
/* VRs */
#define FROGS_R93			NODE_25

const struct discrete_555_desc frogsZip555m =
{
	DISC_555_TRIGGER_IS_LOGIC | DISC_555_OUT_DC | DISC_555_OUT_CAP,
	12,		// B+ voltage of 555
	DEFAULT_555_VALUES
};

const struct discrete_555_cc_desc frogsZip555cc =
{
	DISC_555_OUT_CAP | DISC_555_OUT_DC,
	12,		// B+ voltage of 555
	DEFAULT_555_VALUES,
	12,		// B+ voltage of the Constant Current source
	0.6		// Q13 Vbe
};

const struct discrete_mixer_desc frogsMixer =
{
	DISC_MIXER_IS_OP_AMP,
	{RES_K(1), RES_K(5)},
	{FROGS_R93, 0},
	{CAP_U(0.01), CAP_U(0.01)},
	0, RES_K(56), 0, CAP_U(0.1), 0, 10000
};

DISCRETE_SOUND_START(frogs_discrete_interface)
	/************************************************
     * Input register mapping for frogs
     *
     * All inputs are inverted by initial transistor.
     ************************************************/
	DISCRETE_INPUT_LOGIC(FROGS_FLY_EN)
	DISCRETE_INPUT_NOT(FROGS_JUMP_EN)
	DISCRETE_INPUT_NOT(FROGS_HOP_EN)
	DISCRETE_INPUT_NOT(FROGS_TONGUE_EN)
	DISCRETE_INPUT_NOT(FROGS_CAPTURE_EN)
	DISCRETE_INPUT_NOT(FROGS_SPLASH_EN)

	DISCRETE_ADJUSTMENT(FROGS_R93, 1, RES_M(1), RES_K(10), DISC_LOGADJ, 2)

	DISCRETE_555_MSTABLE(NODE_30, 1, FROGS_TONGUE_EN, RES_K(100), CAP_U(1), &frogsZip555m)

	/* Q11 & Q12 transform the voltage from the oneshot U4, to what is
     * needed by the 555CC circuit.  Vin to R29 must be > 1V for things
     * to change.  <=1 then The Vout of this circuit is 12V.
     * The Current thru R28 equals current thru R51. iR28 = iR51
     * So when Vin>.5, iR51 = (Vin-.5)/39k.  =0 when Vin<=.5
     * So the voltage drop across R28 is vR28 = iR51 * 22k.
     * Finally the Vout = 12 - vR28.
     * Note this formula only works when Vin < 39/(22+39)*12V+1.
     * Which it always is, due to the 555 clamping to 12V*2/3.
     * The Zip effect is hard to emulate 100% due to loading effects
     * of the output stage on the charge stage.  So I added some values
     * to get a similar waveshape to the breadboarded circuit.
     */
	DISCRETE_TRANSFORM5(NODE_31, 1, 12, NODE_30, .5, RES_K(22)/RES_K(39), 0, "012-P4>*3*-")

	DISCRETE_555_CC(NODE_32, 1, NODE_31, RES_K(1.1), CAP_U(0.14), 0, RES_K(100), 500, &frogsZip555cc)

	DISCRETE_MIXER2(NODE_90, 1, NODE_32, 0, &frogsMixer)

	DISCRETE_OUTPUT(NODE_90, 1)

DISCRETE_SOUND_END

static const char *frogs_sample_names[] =
{
	"*frogs",
	"boing.wav",
	"buzzz.wav",
	"croak.wav",
	"hop.wav",
	"splash.wav",
	"zip.wav",
	0       /* end of array */
};

struct Samplesinterface frogs_samples_interface =
{
	5,	/* 5 channels */
	frogs_sample_names
};

void croak_callback(int param)
{
	sample_stop(2);
}

WRITE8_HANDLER( frogs_sh_port2_w )
{
	static int last_croak = 0;
	static int last_buzzz = 0;
	int new_croak = data & 0x08;
	int new_buzzz = data & 0x10;

//  discrete_sound_w(FROGS_HOP_EN, data & 0x01);
//  discrete_sound_w(FROGS_JUMP_EN, data & 0x02);
	discrete_sound_w(FROGS_TONGUE_EN, data & 0x04);
//  discrete_sound_w(FROGS_CAPTURE_EN, data & 0x08);
//  discrete_sound_w(FROGS_FLY_EN, data & 0x10);
//  discrete_sound_w(FROGS_SPLASH_EN, data & 0x80);

	if (data & 0x01)
		sample_start (3, 3, 0);	// Hop
	if (data & 0x02)
		sample_start (0, 0, 0);	// Boing
	if (new_croak)
		sample_start (2, 2, 0);	// Croak
	else
	{
		if (last_croak)
		{
			/* The croak will keep playing until .429s after being disabled */
			timer_adjust(croak_timer, 1.1 * RES_K(390) * CAP_U(1), 0, 0);
		}
	}
	if (new_buzzz)
	{
		/* The Buzzz sound starts off a little louder in volume then
         * settles down to a steady buzzz.  Whenever the trigger goes
         * low, the sound is disabled.  If it then goes high, the buzzz
         * then starts off louder again.  The games does this every time
         * the fly moves.
         * So I made the sample start with the louder effect and then play
         * for 12 seconds.  A fly should move before this.  If not the
         * sample loops, adding the loud part as if the fly moved.
         * This is obviously incorrect, but a fly never stands still for
         * 12 seconds.
         */
		if (!last_buzzz)
			sample_start (1, 1, 1);	// Buzzz
	}
	else
		sample_stop(1);
	if (data & 0x80)
		sample_start (4, 4, 0);	// Splash

	last_croak = new_croak;
	last_buzzz = new_buzzz;
}

#define	HEADON_HISPEED_CC_EN	NODE_01
#define	HEADON_HISPEED_PC_EN	NODE_02
#define	HEADON_CAR_ON_EN		NODE_03
#define	HEADON_CRASH_EN			NODE_04
#define	HEADON_SCREECH1_EN		NODE_05
#define	HEADON_SCREECH2_EN		NODE_06
#define	HEADON_BONUS_EN			NODE_07

#define	HEADON_COMP_CAR_OUT		NODE_200
#define	HEADON_PLAYER_CAR_OUT	NODE_201
#define	HEADON_CRASH_OUT		NODE_202
#define	HEADON_SCREECH1_OUT		NODE_203
#define	HEADON_SCREECH2_OUT		NODE_204
#define	HEADON_BONUS_OUT		NODE_205

static const struct discrete_mixer_desc headon_mixer =
	{DISC_MIXER_IS_RESISTOR,
		{RES_K(130), RES_K(130), RES_K(100), RES_K(100), RES_K(100), RES_K(10)},
		{0,0,0,0,0},
		{0,0,0,0,CAP_N(470),0},
		0, RES_K(100),
		0,
		CAP_U(1),
		0, 1};

static const struct discrete_mixer_desc headon_crash_mixer =
	{DISC_MIXER_IS_OP_AMP,
		{RES_K(50), RES_K(10)},
		{0,0,0,0,0},
		{CAP_N(100),CAP_U(1)},
		0, RES_K(100),
		0,
		CAP_U(1)*0,
		0, 1};

static const mw_discrete_inverter_osc_desc headon_inverter_osc_1 =
	{MW_DEFAULT_CD40XX_VALUES(12),
	MW_DISC_OSC_INVERTER_IS_TYPE4
	};

static const mw_discrete_inverter_osc_desc headon_inverter_osc_2 =
	{MW_DEFAULT_CD40XX_VALUES(12),
	MW_DISC_OSC_INVERTER_IS_TYPE5 | MW_DISC_OSC_INVERTER_OUT_IS_LOGIC
	};

static const struct discrete_555_desc headon_555_bonus =
	{DISC_555_OUT_DC | DISC_555_OUT_ENERGY,
		12,
		12-0.5,12*0.66,12*0.33
	};

static const struct discrete_555_desc headon_555_crash =
	{DISC_555_OUT_DC | DISC_555_TRIGGER_IS_LOGIC,
		12,
		12-0.5,12*0.66,12*0.33
	};

static const struct discrete_555_cc_desc headon_555cc =
{
	DISC_555_OUT_DC,
	12,
	DEFAULT_555_VALUES,
	12,
	0.6
};

#define MM5837_CLOCK_12V 100000

static const struct discrete_lfsr_desc mm5837_lfsr =
{
	DISC_CLK_IS_FREQ,
	17,
	0,
	13,
	16,
	DISC_LFSR_XOR,
	DISC_LFSR_NOT_IN0,
	DISC_LFSR_REPLACE,
	0x000001,
	0,
	16
};

DISCRETE_SOUND_START(headon_discrete_interface)

	DISCRETE_INPUT_LOGIC(HEADON_HISPEED_CC_EN)
	DISCRETE_INPUT_LOGIC(HEADON_HISPEED_PC_EN)
	DISCRETE_INPUT_LOGIC(HEADON_CAR_ON_EN)
	DISCRETE_INPUT_LOGIC(HEADON_CRASH_EN)
	DISCRETE_INPUT_LOGIC(HEADON_SCREECH1_EN)
	DISCRETE_INPUT_LOGIC(HEADON_SCREECH2_EN)
	DISCRETE_INPUT_LOGIC(HEADON_BONUS_EN)

	DISCRETE_RAMP(NODE_20, 1, HEADON_CAR_ON_EN, (12-10.8)/7, 12, 10.8, 12)
	DISCRETE_RAMP(NODE_21, 1, HEADON_HISPEED_PC_EN, 2.0 / 0.8, 0, -2, 0)
	DISCRETE_ADDER2(NODE_22, 1, NODE_20, NODE_21)

#define HO_R56		RES_K(10)
#define HO_R72		RES_K(1)
#define HO_C31		CAP_N(100)

	DISCRETE_555_CC(NODE_25, HEADON_CAR_ON_EN, NODE_22, HO_R56, HO_C31, 0, 0, HO_R72, &headon_555cc)
    DISCRETE_COUNTER(NODE_26, 1, 0, NODE_25, 1, 1, 0, DISC_CLK_ON_R_EDGE)
    DISCRETE_COUNTER(NODE_27, 1, 0, NODE_25, 3, 1, 0, DISC_CLK_ON_R_EDGE)
    DISCRETE_COUNTER(NODE_28, 1, 0, NODE_25, 2, 1, 0, DISC_CLK_ON_R_EDGE)
	DISCRETE_TRANSFORM5(NODE_29,1,NODE_26,NODE_27,NODE_28,1,2,"13>24=+0+")
	DISCRETE_MULTIPLY(HEADON_PLAYER_CAR_OUT, 1, NODE_29, 12 / 3)

	DISCRETE_RAMP(NODE_30, 1, HEADON_CAR_ON_EN, (12-10.8)/7, 12, 10.8, 12)
	DISCRETE_RAMP(NODE_31, 1, HEADON_HISPEED_CC_EN, 2.0 / 0.8, 0, -2, 0)
	DISCRETE_ADDER2(NODE_32, 1, NODE_30, NODE_31)

#define HO_R43		RES_K(10)
#define HO_R35		RES_K(1)
#define HO_C20		CAP_N(100)

	DISCRETE_555_CC(NODE_35, HEADON_CAR_ON_EN, NODE_32, HO_R43, HO_C20, 0, 0, HO_R35, &headon_555cc)
    DISCRETE_COUNTER(NODE_36, 1, 0, NODE_35, 1, 1, 0, DISC_CLK_ON_R_EDGE)
    DISCRETE_COUNTER(NODE_37, 1, 0, NODE_35, 3, 1, 0, DISC_CLK_ON_R_EDGE)
    DISCRETE_COUNTER(NODE_38, 1, 0, NODE_35, 2, 1, 0, DISC_CLK_ON_R_EDGE)
	DISCRETE_TRANSFORM5(NODE_39,1,NODE_36,NODE_37,NODE_38,1,2,"13>24=+0+")
	DISCRETE_MULTIPLY(HEADON_COMP_CAR_OUT, 1, NODE_39, 12 / 3)

	DISCRETE_MULTIPLY(NODE_50,1,HEADON_SCREECH1_EN,12)
	DISCRETE_LFSR_NOISE(NODE_51, 1, 1, MM5837_CLOCK_12V, 12.0, 0, 6.0, &mm5837_lfsr)
	MW_DISCRETE_INVERTER_OSC(HEADON_SCREECH1_OUT,NODE_50,NODE_51,RES_K(10),RES_K(100),CAP_N(47),RES_K(10),&headon_inverter_osc_1)

	DISCRETE_MULTIPLY(NODE_60,1,HEADON_SCREECH2_EN,12)
	MW_DISCRETE_INVERTER_OSC(HEADON_SCREECH2_OUT,NODE_60,NODE_51,RES_K(10),RES_K(100),CAP_N(57),RES_K(10),&headon_inverter_osc_1)

	DISCRETE_LOGIC_INVERT(NODE_70, 1, HEADON_BONUS_EN)
	DISCRETE_MULTIPLY(NODE_71,1,NODE_70,12)
	MW_DISCRETE_INVERTER_OSC(NODE_73,NODE_71,0,RES_K(22),RES_M(1),CAP_N(470),RES_M(10),&headon_inverter_osc_2)

	DISCRETE_TRANSFORM3(NODE_74,1,NODE_73,200000,165000,"102*-")
	DISCRETE_555_ASTABLE(NODE_75, 1, NODE_74, RES_K(100), CAP_N(10), &headon_555_bonus)
	DISCRETE_MULTIPLY(HEADON_BONUS_OUT,1,NODE_75,HEADON_BONUS_EN)

	DISCRETE_LOGIC_INVERT(NODE_80, 1, HEADON_CRASH_EN)
	DISCRETE_555_MSTABLE(NODE_81, 1, NODE_80, RES_K(470), CAP_U(1), &headon_555_crash)

	DISCRETE_MULTIPLY(NODE_84, 1, NODE_81, NODE_51)

	DISCRETE_FILTER1(NODE_85, 1, NODE_84, 500, DISC_FILTER_BANDPASS)

	DISCRETE_555_MSTABLE(NODE_86, 1, NODE_80, RES_K(470), CAP_U(2.2), &headon_555_crash)

	DISCRETE_MULTIPLY(NODE_87, 1, NODE_86, NODE_51)

	DISCRETE_FILTER2(NODE_88, 1, NODE_87, 1.0 / (2 * M_PI * RES_K(15) * sqrt(CAP_N(470) * CAP_N(47))), 2 * sqrt(CAP_N(47) / CAP_N(470)), DISC_FILTER_LOWPASS)

	DISCRETE_MIXER2(NODE_95, 1, NODE_85, NODE_88, &headon_crash_mixer)
	DISCRETE_TRANSFORM2(HEADON_CRASH_OUT, 1, NODE_95, 12, "01/")

	DISCRETE_MIXER6(NODE_210, 1, HEADON_PLAYER_CAR_OUT, HEADON_COMP_CAR_OUT,
					HEADON_SCREECH1_OUT, HEADON_SCREECH2_OUT,
					HEADON_BONUS_OUT, HEADON_CRASH_OUT, &headon_mixer)

	DISCRETE_OUTPUT(NODE_210, 37000.0 / 12.0)

DISCRETE_SOUND_END

WRITE8_HANDLER( headon_audio_w )
{
	if (!sndti_exists(SOUND_DISCRETE, 0))
		return;
	discrete_sound_w(HEADON_HISPEED_PC_EN, data & 0x01);
	discrete_sound_w(HEADON_SCREECH1_EN, data & 0x02);
	discrete_sound_w(HEADON_CRASH_EN, data & 0x04);
	discrete_sound_w(HEADON_HISPEED_CC_EN, data & 0x08);
	discrete_sound_w(HEADON_SCREECH2_EN, data & 0x10);
	discrete_sound_w(HEADON_BONUS_EN, data & 0x20);
	discrete_sound_w(HEADON_CAR_ON_EN, data & 0x40);

}


static UINT8 nsub_sound_state, tranqgun_sound_state;

void vicdual_sample_reset(void)
{
	nsub_sound_state = 0xff;
	tranqgun_sound_state = 0;
}

const char *nsub_sample_names[] =
{
	"*nsub", "SND_EXPL_L0", "SND_EXPL_L1", "SND_SONAR",
	"SND_LAUNCH0", "SND_LAUNCH1", "SND_WARNING0", "SND_WARNING1",
	"SND_EXPL_S0", "SND_EXPL_S1", "SND_BONUS0", "SND_BONUS1",
	"SND_CODE", "SND_BOAT", 0
};

WRITE8_HANDLER( nsub_audio_w )
{
	static const int on_sample[8] = { 5, 2, 3, 0, 7, 9, 11, 12 };
	static const int off_sample[8] = { 6, -1, 4, 1, 8, 10, -1, -1 };
	UINT8 changed = nsub_sound_state ^ data;
	int bit;

	nsub_sound_state = data;
	for (bit = 0; bit < 8; bit++)
	{
		int on = on_sample[bit], off = off_sample[bit];
		if (!(changed & (1 << bit)))
			continue;
		if (!(data & (1 << bit)))
		{
			sample_start(on, on, 1);
			if (off >= 0)
				sample_stop(off);
		}
		else
		{
			sample_stop(on);
			if (off >= 0)
				sample_start(off, off, 0);
		}
	}
}

const char *tranqgun_sample_names[] =
{
	"*tranqgun", "cry", "walk", "emar", "animalhit", "point",
	"jeep", "gun", "animal", 0
};

WRITE8_HANDLER( tranqgun_audio_w )
{
	static const int samples[8] = { 7, 0, 1, 2, 3, 4, 5, 6 };
	UINT8 changed = tranqgun_sound_state ^ data;
	int bit;

	tranqgun_sound_state = data;
	for (bit = 0; bit < 8; bit++)
	{
		int channel = samples[bit];
		if (!(changed & (1 << bit)))
			continue;
		if (bit == 3)
		{
			if (!(data & 0x08))
				sample_start(channel, channel, 0);
		}
		else if ((bit == 6) ? !(data & 0x40) : (data & (1 << bit)))
			sample_start(channel, channel, bit == 6);
		else
			sample_stop(channel);
	}
}

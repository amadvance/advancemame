// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Tormod Tjaberg, Mirko Buffoni,Lee Taylor, Valerio Verrando, Zsolt Vasvari,Derrick Renaud


#include "driver.h"
#include "sound/mw_discrete.h"
#include "sound/custom.h"
#define MW8080BW_MASTER_CLOCK 19968000
#include "sound/samples.h"
#include "sound/sn76477.h"
#include "sound/discrete.h"

static void mw_sample_start(int chip, int channel, int samplenum, int loop)
{
	int stride = strcmp(Machine->gamedrv->name, "m4") ? 1 : 2;
	sample_start(chip * stride + channel, samplenum, loop);
}
WRITE8_HANDLER(desertgu_controller_select_w);
void mw_controller_select(int value);

static UINT8 port_1_last;
static UINT8 port_2_last;
void mw_audio_latches_reset(void)
{
	port_1_last = port_2_last = 0;
}

#define MIDWAY_TONE_EN                          NODE_100
#define MIDWAY_TONE_DATA_L                      NODE_101
#define MIDWAY_TONE_DATA_H                      NODE_102
#define MIDWAY_TONE_SND                         NODE_103
#define MIDWAY_TONE_TRASFORM_OUT        NODE_104
#define MIDWAY_TONE_BEFORE_AMP_SND      NODE_105

#define MIDWAY_TONE_GENERATOR(mw_discrete_op_amp_tvca_info) \
                 \
                 \
	MW_DISCRETE_INPUT_LOGIC(MIDWAY_TONE_EN) \
	MW_DISCRETE_INPUT_DATA(MIDWAY_TONE_DATA_L) \
	MW_DISCRETE_INPUT_DATA(MIDWAY_TONE_DATA_H) \
	MW_DISCRETE_TRANSFORM4(MIDWAY_TONE_TRASFORM_OUT, 1, MIDWAY_TONE_DATA_H, 0x40, MIDWAY_TONE_DATA_L, 0x02, "01*23*+") \
	MW_DISCRETE_NOTE(MIDWAY_TONE_BEFORE_AMP_SND, 1, (double)MW8080BW_MASTER_CLOCK / 10 / 2, MIDWAY_TONE_TRASFORM_OUT, 0xfff, 1, MW_DISC_CLK_IS_FREQ) \
	MW_DISCRETE_OP_AMP_TRIG_VCA(MIDWAY_TONE_SND, MIDWAY_TONE_BEFORE_AMP_SND, MIDWAY_TONE_EN, 0, 12, 0, &mw_discrete_op_amp_tvca_info)

WRITE8_HANDLER( mw_midway_tone_generator_lo_w )
{
	discrete_sound_w(MIDWAY_TONE_EN, (data >> 0) & 0x01);

	discrete_sound_w(MIDWAY_TONE_DATA_L, (data >> 1) & 0x1f);

}

WRITE8_HANDLER( mw_midway_tone_generator_hi_w )
{
	discrete_sound_w(MIDWAY_TONE_DATA_H, data & 0x3f);

}

static const mw_discrete_lfsr_desc midway_lfsr =
{
	MW_DISC_CLK_IS_FREQ,
	17,

	0x1ffff,
	4,
	16,
	MW_DISC_LFSR_XOR,
	MW_DISC_LFSR_OR,
	MW_DISC_LFSR_REPLACE,
	0x000001,
	0,
	12
};

static const char *seawolf_sample_names[] =
{
	"*seawolf",
	"shiphit.wav",
	"torpedo.wav",
	"dive.wav",
	"sonar.wav",
	"minehit.wav",
	0
};

static struct Samplesinterface seawolf_samples_interface =
{
	5,
	seawolf_sample_names
};

MACHINE_DRIVER_START(mw_seawolf_audio)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(seawolf_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.6)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_seawolf_audio_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	if (rising_bits & 0x01) mw_sample_start(0, 0, 0, 0);

	if (rising_bits & 0x02) mw_sample_start(0, 1, 1, 0);

	if (rising_bits & 0x04) mw_sample_start(0, 2, 2, 0);

	if (rising_bits & 0x08) mw_sample_start(0, 3, 3, 0);

	if (rising_bits & 0x10) mw_sample_start(0, 4, 4, 0);

	coin_counter_w(0, (data >> 5) & 0x01);

	port_1_last = data;
}

static const char *gunfight_sample_names[] =
{
	"*gunfight",
	"gunshot.wav",
	"killed.wav",
	0
};

static struct Samplesinterface gunfight_samples_interface =
{
	2,
	gunfight_sample_names
};

MACHINE_DRIVER_START(mw_gunfight_audio)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(gunfight_samples_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.5)
	MDRV_SOUND_ROUTE(1, "right", 0.5)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_gunfight_audio_w )
{

	coin_counter_w(0, (data >> 2) & 0x01);

	switch ((~data >> 4) & 0x0f) {
	case 0x00:
		break;

	case 0x01:

		mw_sample_start(0, 0, 0, 0);
		break;

	case 0x02:

		mw_sample_start(1, 0, 0, 0);
		break;

	case 0x03:

		mw_sample_start(0, 0, 1, 0);
		break;

	case 0x04:

		mw_sample_start(1, 0, 1, 0);
		break;

	default:
		logerror("%04x:  Unknown sh port write %02x\n", activecpu_get_pc(), data);
		break;
	}
}

#define TORNBASE_SQUAREW_240            NODE_01
#define TORNBASE_SQUAREW_960            NODE_02
#define TORNBASE_SQUAREW_120            NODE_03

#define TORNBASE_TONE_240_EN            NODE_04
#define TORNBASE_TONE_960_EN            NODE_05
#define TORNBASE_TONE_120_EN            NODE_06

#define TORNBASE_TONE_240_SND           NODE_07
#define TORNBASE_TONE_960_SND           NODE_08
#define TORNBASE_TONE_120_SND           NODE_09
#define TORNBASE_TONE_SND                       NODE_10
#define TORNBASE_TONE_SND_FILT          NODE_11

MW_DISCRETE_SOUND_START(mw_tornbase)

	MW_DISCRETE_INPUT_LOGIC(TORNBASE_TONE_240_EN)
	MW_DISCRETE_INPUT_LOGIC(TORNBASE_TONE_960_EN)
	MW_DISCRETE_INPUT_LOGIC(TORNBASE_TONE_120_EN)

	MW_DISCRETE_SQUAREWFIX(TORNBASE_SQUAREW_240, 1, 240, 1.0, 50.0, 1.0 / 2, 0)
	MW_DISCRETE_SQUAREWFIX(TORNBASE_SQUAREW_960, 1, 960, 1.0, 50.0, 1.0 / 2, 0)
	MW_DISCRETE_SQUAREWFIX(TORNBASE_SQUAREW_120, 1, 120, 1.0, 50.0, 1.0 / 2, 0)

	MW_DISCRETE_LOGIC_NAND(TORNBASE_TONE_240_SND, 1, TORNBASE_SQUAREW_240, TORNBASE_TONE_240_EN)
	MW_DISCRETE_LOGIC_NAND(TORNBASE_TONE_960_SND, 1, TORNBASE_SQUAREW_960, TORNBASE_TONE_960_EN)
	MW_DISCRETE_LOGIC_NAND(TORNBASE_TONE_120_SND, 1, TORNBASE_SQUAREW_120, TORNBASE_TONE_120_EN)
	MW_DISCRETE_LOGIC_AND3(TORNBASE_TONE_SND, 1, TORNBASE_TONE_240_SND, TORNBASE_TONE_960_SND, TORNBASE_TONE_120_SND)

	MW_DISCRETE_CRFILTER(TORNBASE_TONE_SND_FILT, 1, TORNBASE_TONE_SND, RES_K(47), CAP_U(0.047))

	MW_DISCRETE_OUTPUT(TORNBASE_TONE_SND_FILT, 32767)

MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_tornbase_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_tornbase_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_tornbase_audio_w )
{
	discrete_sound_w(TORNBASE_TONE_240_EN, (data >> 0) & 0x01);

	discrete_sound_w(TORNBASE_TONE_960_EN, (data >> 1) & 0x01);

	discrete_sound_w(TORNBASE_TONE_120_EN, (data >> 2) & 0x01);

	if (0 == 0) {

	} else {

	}

	coin_counter_w(0, (data >> 7) & 0x01);
}

#include "streams.h"
#include <math.h>
static sound_stream *mw_engine_stream;
static UINT8 mw_engine_p1, mw_engine_p2;
static double mw_engine_speed, mw_engine_gear, mw_engine_phase[3];
static double mw_engine_noise_low, mw_engine_noise_band, mw_engine_crash, mw_engine_dc;
static UINT32 mw_engine_noise = 0x1ffff;
static void mw_engine_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int i, j;
	double rate = Machine->sample_rate ? Machine->sample_rate : 44100;
	double cap = !strcmp(Machine->gamedrv->name, "lagunar") ? 22e-6 : 47e-6;
	double resistors[4] = { 2200000, 1000000, 470000, 240000 };
	double target = 0, full = 0;
	for (j = 0; j < 4; ++j) {
		full += 1.0 / resistors[j];
		if (mw_engine_p1 & (1 << j)) target += 1.0 / resistors[j];
	}
	target /= full;
	for (i = 0; i < samples; ++i) {
		double triangle[3], speed, noise, sound, tau;
		double frequencies[3] = { 87, 343, 465 };
		if (!strcmp(Machine->gamedrv->name, "lagunar")) frequencies[2] = 511;
		tau = (target > mw_engine_speed ? 47000 : 10000) * cap;
		if (mw_engine_p2 & 1) tau = 270 * cap;
		mw_engine_speed += (((mw_engine_p2 & 1) ? 0 : target) - mw_engine_speed) * (1 - exp(-1 / (rate * tau)));
		mw_engine_gear += (((mw_engine_p1 & 0x10) ? mw_engine_speed : 0) - mw_engine_gear) * (1 - exp(-1 / (rate * 560000 * 2.2e-6)));
		speed = MAX(0.04, mw_engine_speed - 0.45 * mw_engine_gear);
		for (j = 0; j < 3; ++j) {
			mw_engine_phase[j] += frequencies[j] * speed / rate;
			mw_engine_phase[j] -= floor(mw_engine_phase[j]);
			triangle[j] = 1 - 4 * fabs(mw_engine_phase[j] - 0.5);
		}
		mw_engine_noise = (mw_engine_noise >> 1) | (((mw_engine_noise ^ (mw_engine_noise >> 3)) & 1) << 16);
		noise = (mw_engine_noise & 1) ? 1 : -1;
		mw_engine_noise_low += (noise - mw_engine_noise_low) * (1 - exp(-2 * M_PI * 1000 / rate));
		mw_engine_noise_band += (mw_engine_noise_low - mw_engine_noise_band) * (1 - exp(-2 * M_PI * 400 / rate));
		mw_engine_crash += (((mw_engine_p2 & 1) ? 1 : 0) - mw_engine_crash) * (1 - exp(-1 / (rate * ((mw_engine_p2 & 1) ? 0.003 : 0.5))));
		sound = (mw_engine_p2 & 2) ? (triangle[0] * triangle[1] * 0.6 + triangle[2] * 0.2) * (0.2 + 0.8 * mw_engine_speed) : 0;
		sound += mw_engine_noise_low * mw_engine_crash;
		if (mw_engine_p2 & 4) sound += (mw_engine_noise_low - mw_engine_noise_band) * 1.5;
		if (mw_engine_p2 & 8) sound += mw_engine_noise_low * 0.35;
		mw_engine_dc += (sound - mw_engine_dc) * (1 - exp(-2 * M_PI * 16 / rate));
		outputs[0][i] = (sound - mw_engine_dc) * 11000;
	}
}
static void mw_engine_reset(void *token)
{
	mw_engine_p1 = mw_engine_p2 = 0;
	mw_engine_speed = mw_engine_gear = 0;
	memset(mw_engine_phase, 0, sizeof(mw_engine_phase));
	mw_engine_noise_low = mw_engine_noise_band = mw_engine_crash = mw_engine_dc = 0;
	mw_engine_noise = 0x1ffff;
}
static void *mw_engine_start(int clock, const struct CustomSound_interface *config)
{
	mw_engine_reset(NULL);
	mw_engine_stream = stream_create(0, 1, Machine->sample_rate ? Machine->sample_rate : 44100, NULL, mw_engine_update);
	return mw_engine_stream;
}
static struct CustomSound_interface mw_engine_interface = { mw_engine_start, NULL, mw_engine_reset, NULL };

MACHINE_DRIVER_START(mw_zzzap_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(mw_engine_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_zzzap_audio_1_w )
{
	stream_update(mw_engine_stream, 0);
	mw_engine_p1 = data;
}

WRITE8_HANDLER( mw_zzzap_audio_2_w )
{
	stream_update(mw_engine_stream, 0);
	mw_engine_p2 = data;
	coin_counter_w(0, (data >> 5) & 1);
}

#define MAZE_P1_DATA                    NODE_01
#define MAZE_P2_DATA                    NODE_02
#define MAZE_TONE_TIMING                NODE_03
#define MAZE_COIN                               NODE_04

#define MAZE_JOYSTICK_IN_USE    NODE_11
#define MAZE_AUDIO_ENABLE               NODE_12
#define MAZE_TONE_ENABLE                NODE_13
#define MAZE_GAME_OVER                  NODE_14
#define MAZE_R305_306_308               NODE_15
#define MAZE_R303_309                   NODE_16
#define MAZE_PLAYER_SEL                 NODE_17

#define MAZE_SND                                NODE_18

static const mw_discrete_555_desc maze_555_F2 =
{
	MW_DISC_555_OUT_SQW | MW_DISC_555_OUT_DC | MW_DISC_555_TRIGGER_IS_LOGIC | MW_DISC_555_TRIGGER_DISCHARGES_CAP,
	5,
	MW_DEFAULT_555_VALUES
};

static const double maze_74147_table[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 1, 1, 2, 3
};

static const mw_discrete_comp_adder_table maze_r305_306_308 =
{
	MW_DISC_COMP_P_RESISTOR,
	RES_K(100),
	2,
	{ RES_M(1.5),
	  RES_K(820) }
};

static const mw_discrete_comp_adder_table maze_r303_309 =
{
	MW_DISC_COMP_P_RESISTOR,
	RES_K(330),
	1,
	{ RES_M(1) }
};

static const mw_discrete_op_amp_osc_info maze_op_amp_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_1 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	RES_M(1),
	RES_K(430),
	MAZE_R305_306_308,
	MAZE_R303_309,
	RES_K(330),
	0, 0, 0,
	CAP_P(3300),
	5
};

MW_DISCRETE_SOUND_START(mw_maze)

	MW_DISCRETE_INPUT_DATA(MAZE_P1_DATA)
	MW_DISCRETE_INPUT_DATA(MAZE_P2_DATA)
	MW_DISCRETE_INPUT_LOGIC(MAZE_TONE_TIMING)
	MW_DISCRETE_INPUT_LOGIC(MAZE_COIN)
	MW_DISCRETE_INPUT_LOGIC(MAZE_JOYSTICK_IN_USE)

	MW_DISCRETE_LOGIC_INVERT(NODE_20,
		1,
		MAZE_JOYSTICK_IN_USE)
	MW_DISCRETE_555_MSTABLE(MAZE_GAME_OVER,
		1,
		NODE_20,
		RES_K(270),
		CAP_U(100),
		&maze_555_F2)
	MW_DISCRETE_LOGIC_JKFLIPFLOP(MAZE_AUDIO_ENABLE,
		1,
		MAZE_COIN,
		1,
		MAZE_GAME_OVER,
		1,
		0)
	MW_DISCRETE_LOGIC_INVERT(MAZE_TONE_ENABLE,
		1,
		MAZE_AUDIO_ENABLE)
	MW_DISCRETE_LOGIC_AND3(NODE_21,
		1,
		MAZE_JOYSTICK_IN_USE,
		MAZE_TONE_ENABLE,
		MAZE_TONE_TIMING)

	MW_DISCRETE_LOGIC_JKFLIPFLOP(MAZE_PLAYER_SEL,
		1,
		1,
		1,
		MAZE_TONE_TIMING,
		1,
		1)
	MW_DISCRETE_MULTIPLEX2(NODE_31,
		1,
		MAZE_PLAYER_SEL,
		MAZE_P1_DATA,
		MAZE_P2_DATA)
	MW_DISCRETE_LOOKUP_TABLE(NODE_32,
		1,
		NODE_31,
		16,
		&maze_74147_table)
	MW_DISCRETE_COMP_ADDER(MAZE_R305_306_308,
		1,
		NODE_32,
		&maze_r305_306_308)
	MW_DISCRETE_COMP_ADDER(MAZE_R303_309,
		1,
		MAZE_PLAYER_SEL,
		&maze_r303_309)
	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_36,
		1,
		&maze_op_amp_osc)

	MW_DISCRETE_CRFILTER_VREF(NODE_40,
		1,
		NODE_36,
		RES_K(250),
		CAP_U(0.1),
		2.5)
	MW_DISCRETE_SWITCH(NODE_41,
		1,
		NODE_21,
		2.5,
		NODE_40)
	MW_DISCRETE_CRFILTER(NODE_42,
		1,
		NODE_41,
		RES_K(56 + 390),
		CAP_P(0.01))
	MW_DISCRETE_RCFILTER(NODE_43,
		1,
		NODE_42,
		RES_K(56),
		CAP_P(4700))
	MW_DISCRETE_SWITCH(MAZE_SND,
		1,
		MAZE_AUDIO_ENABLE,
		NODE_43,
		0)

	MW_DISCRETE_OUTPUT(MAZE_SND, 96200)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_maze_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_maze_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

void maze_write_discrete(UINT8 maze_tone_timing_state)
{

	int controls = ~readinputport(0) & 0xff;

	discrete_sound_w(MAZE_TONE_TIMING, maze_tone_timing_state);
	discrete_sound_w(MAZE_P1_DATA, controls & 0x0f);
	discrete_sound_w(MAZE_P2_DATA, (controls >> 4) & 0x0f);
	discrete_sound_w(MAZE_JOYSTICK_IN_USE, controls != 0xff);

	discrete_sound_w(MAZE_COIN, (~readinputport(1) >> 3) & 0x01);
}

#define BOOTHILL_GAME_ON_EN                     NODE_01
#define BOOTHILL_LEFT_SHOT_EN           NODE_02
#define BOOTHILL_RIGHT_SHOT_EN          NODE_03
#define BOOTHILL_LEFT_HIT_EN            NODE_04
#define BOOTHILL_RIGHT_HIT_EN           NODE_05

#define BOOTHILL_NOISE                          NODE_06
#define BOOTHILL_L_SHOT_SND                     NODE_07
#define BOOTHILL_R_SHOT_SND                     NODE_08
#define BOOTHILL_L_HIT_SND                      NODE_09
#define BOOTHILL_R_HIT_SND                      NODE_10

#define BOOTHILL_MUSIC_ADJ                      NODE_11

static const mw_discrete_op_amp_tvca_info boothill_tone_tvca_info =
{
	RES_M(3.3),
	RES_K(100) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_tvca_info boothill_shot_tvca_info =
{
	RES_M(2.7),
	RES_K(510),
	0,
	RES_K(510),
	RES_K(10),
	0,
	RES_K(510),
	0,
	0,
	0,
	0,
	CAP_U(0.22),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_tvca_info boothill_hit_tvca_info =
{
	RES_M(2.7),
	RES_K(510),
	0,
	RES_K(510),
	RES_K(10),
	0,
	RES_K(510),
	0,
	0,
	0,
	0,
	CAP_U(1),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_mixer_desc boothill_l_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(33),
	  RES_K(12) + RES_K(100) + RES_K(33) },
	{ 0 },
	{ 0 },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	7200
};

static const mw_discrete_mixer_desc boothill_r_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(33),
	  RES_K(12) + RES_K(100) + RES_K(33),
	  RES_K(33) },
	{ 0,
	  0,
	  BOOTHILL_MUSIC_ADJ },
	{ 0 },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	7200
};

MW_DISCRETE_SOUND_START(mw_boothill)

	MW_DISCRETE_INPUT_LOGIC(BOOTHILL_GAME_ON_EN)
	MW_DISCRETE_INPUT_LOGIC(BOOTHILL_LEFT_SHOT_EN)
	MW_DISCRETE_INPUT_LOGIC(BOOTHILL_RIGHT_SHOT_EN)
	MW_DISCRETE_INPUT_LOGIC(BOOTHILL_LEFT_HIT_EN)
	MW_DISCRETE_INPUT_LOGIC(BOOTHILL_RIGHT_HIT_EN)

	MW_DISCRETE_CONSTANT(BOOTHILL_MUSIC_ADJ, ((RES_M(1)) + (75000)) * 0.5)

	MIDWAY_TONE_GENERATOR(boothill_tone_tvca_info)

	MW_DISCRETE_LFSR_NOISE(BOOTHILL_NOISE, 1, 1, 7700, 12.0, 0, 12.0 / 2, &midway_lfsr)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_30, BOOTHILL_LEFT_SHOT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_shot_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(12), CAP_U(.01))
	MW_DISCRETE_CRFILTER(BOOTHILL_L_SHOT_SND, 1, NODE_31, RES_K(12) + RES_K(68), CAP_U(.0022))

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_35, BOOTHILL_RIGHT_SHOT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_shot_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_36, 1, NODE_35, RES_K(12), CAP_U(.01))
	MW_DISCRETE_CRFILTER(BOOTHILL_R_SHOT_SND, 1, NODE_36, RES_K(12) + RES_K(68), CAP_U(.0033))

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_40, BOOTHILL_LEFT_HIT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_hit_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_41, 1, NODE_40, RES_K(12), CAP_U(.033))
	MW_DISCRETE_CRFILTER(BOOTHILL_L_HIT_SND, 1, NODE_41, RES_K(12) + RES_K(100), CAP_U(.0033))

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_45, BOOTHILL_RIGHT_HIT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_hit_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_46, 1, NODE_45, RES_K(12), CAP_U(.0033))
	MW_DISCRETE_CRFILTER(BOOTHILL_R_HIT_SND, 1, NODE_46, RES_K(12) + RES_K(100), CAP_U(.0022))

	MW_DISCRETE_MIXER2(NODE_91, BOOTHILL_GAME_ON_EN, BOOTHILL_L_SHOT_SND, BOOTHILL_L_HIT_SND, &boothill_l_mixer)

	MW_DISCRETE_MIXER3(NODE_92, BOOTHILL_GAME_ON_EN, BOOTHILL_R_SHOT_SND, BOOTHILL_R_HIT_SND, MIDWAY_TONE_SND, &boothill_r_mixer)

	MW_DISCRETE_OUTPUT(NODE_91, 1)
	MW_DISCRETE_OUTPUT(NODE_92, 1)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_boothill_audio)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_boothill_discrete_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_boothill_audio_w )
{

	coin_counter_w(0, (data >> 2) & 0x01);

	discrete_sound_w(BOOTHILL_GAME_ON_EN, (data >> 3) & 0x01);

	discrete_sound_w(BOOTHILL_LEFT_SHOT_EN, (data >> 4) & 0x01);

	discrete_sound_w(BOOTHILL_RIGHT_SHOT_EN, (data >> 5) & 0x01);

	discrete_sound_w(BOOTHILL_LEFT_HIT_EN, (data >> 6) & 0x01);

	discrete_sound_w(BOOTHILL_RIGHT_HIT_EN, (data >> 7) & 0x01);
}

#define CHECKMAT_BOOM_EN                        NODE_01
#define CHECKMAT_TONE_EN                        NODE_02
#define CHECKMAT_TONE_DATA_45           NODE_03
#define CHECKMAT_TONE_DATA_67           NODE_04

#define CHECKMAT_R401_402_400           NODE_06
#define CHECKMAT_R407_406_410           NODE_07

#define CHECKMAT_BOOM_SND                       NODE_10
#define CHECKMAT_TONE_SND                       NODE_11
#define CHECKMAT_FINAL_SND                      NODE_12

#define CHECKMAT_R309                           NODE_15
#define CHECKMAT_R411                           NODE_16

static const mw_discrete_comp_adder_table checkmat_r401_402_400 =
{
	MW_DISC_COMP_P_RESISTOR,
	RES_K(100),
	2,
	{ RES_M(1.5),
	  RES_K(820) }
};

static const mw_discrete_comp_adder_table checkmat_r407_406_410 =
{
	MW_DISC_COMP_P_RESISTOR,
	RES_K(330),
	2,
	{ RES_M(1),
	  RES_K(510) }
};

static const mw_discrete_op_amp_osc_info checkmat_op_amp_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_1 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	RES_M(1),
	RES_K(430),
	CHECKMAT_R401_402_400,
	CHECKMAT_R407_406_410,
	RES_K(330),
	0, 0, 0,
	CAP_P(3300),
	5
};

static const mw_discrete_op_amp_tvca_info checkmat_op_amp_tvca =
{
	RES_M(1.2),
	RES_M(1),
	0,
	RES_M(1.2),
	RES_K(1),
	0,
	RES_M(1),
	0,
	0,
	0,
	0,
	CAP_U(1),
	0,
	0,
	5,
	0,
	0,
	5,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
};

static const mw_discrete_mixer_desc checkmat_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(100),
	  RES_K(56 + 47) },
	{ CHECKMAT_R309,
	  CHECKMAT_R411 },
	{ CAP_U(10),
	  CAP_U(0.01) },
	0,
	RES_K(100),
	0,
	CAP_U(1),
	0,
	1
};

MW_DISCRETE_SOUND_START(mw_checkmat)

	MW_DISCRETE_INPUT_LOGIC(CHECKMAT_BOOM_EN)
	MW_DISCRETE_INPUT_LOGIC(CHECKMAT_TONE_EN)
	MW_DISCRETE_INPUT_DATA(CHECKMAT_TONE_DATA_45)
	MW_DISCRETE_INPUT_DATA(CHECKMAT_TONE_DATA_67)

	MW_DISCRETE_CONSTANT(CHECKMAT_R309, ((RES_K(100)) + (1000)) * 0.5)
	MW_DISCRETE_CONSTANT(CHECKMAT_R411, ((RES_M(1)) + (1000)) * 0.5)

	MW_DISCRETE_NOISE(NODE_20,
		1,
		1500,
		2,
		0)
	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_21,
		CHECKMAT_BOOM_EN,
		0,
		0,
		NODE_20,
		0,
		&checkmat_op_amp_tvca)

	MW_DISCRETE_FILTER2(NODE_23,
		1,
		NODE_21,
		35,
		1.0 / 8,
		MW_DISC_FILTER_BANDPASS)
	MW_DISCRETE_GAIN(NODE_24,
		NODE_23,
		15)
	MW_DISCRETE_CLAMP(CHECKMAT_BOOM_SND,
		1,
		NODE_24,
		0 - 6,
		12.0 - MW_OP_AMP_NORTON_VBE - 6,
		0)

	MW_DISCRETE_COMP_ADDER(CHECKMAT_R401_402_400,
		1,
		CHECKMAT_TONE_DATA_45,
		&checkmat_r401_402_400)
	MW_DISCRETE_COMP_ADDER(CHECKMAT_R407_406_410,
		1,
		CHECKMAT_TONE_DATA_67,
		&checkmat_r407_406_410)
	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_30,
		1,
		&checkmat_op_amp_osc)

	MW_DISCRETE_CRFILTER_VREF(NODE_31,
		1,
		NODE_30,
		RES_K(250),
		CAP_U(0.1),
		2.5)
	MW_DISCRETE_SWITCH(NODE_32,
		1,
		CHECKMAT_TONE_EN,
		2.5,
		NODE_31)
	MW_DISCRETE_CRFILTER(NODE_33,
		1,
		NODE_32,
		RES_K(56 + 47 + 200),
		CAP_P(0.01))
	MW_DISCRETE_RCFILTER(CHECKMAT_TONE_SND,
		1,
		NODE_33,
		RES_K(56),
		CAP_P(4700))

	MW_DISCRETE_MIXER2(CHECKMAT_FINAL_SND,
		1,
		CHECKMAT_BOOM_SND,
		CHECKMAT_TONE_SND,
		&checkmat_mixer)
	MW_DISCRETE_OUTPUT(CHECKMAT_FINAL_SND, 300000)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_checkmat_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_checkmat_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_checkmat_audio_w )
{
	discrete_sound_w(CHECKMAT_TONE_EN, data & 0x01);

	discrete_sound_w(CHECKMAT_BOOM_EN, (data >> 1) & 0x01);

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	discrete_sound_w(CHECKMAT_TONE_DATA_45, (data >> 4) & 0x03);
	discrete_sound_w(CHECKMAT_TONE_DATA_67, (data >> 6) & 0x03);
}

#define DESERTGU_GAME_ON_EN                                     NODE_01
#define DESERTGU_RIFLE_SHOT_EN                          NODE_02
#define DESERTGU_BOTTLE_HIT_EN                          NODE_03
#define DESERTGU_ROAD_RUNNER_HIT_EN                     NODE_04
#define DESERTGU_CREATURE_HIT_EN                        NODE_05
#define DESERTGU_ROADRUNNER_BEEP_BEEP_EN        NODE_06
#define DESERTGU_TRIGGER_CLICK_EN                       NODE_07

#define DESERTGU_NOISE                                          NODE_08
#define DESERTGU_RIFLE_SHOT_SND                         NODE_09
#define DESERTGU_BOTTLE_HIT_SND                         NODE_10
#define DESERTGU_ROAD_RUNNER_HIT_SND            NODE_11
#define DESERTGU_CREATURE_HIT_SND                       NODE_12
#define DESERTGU_ROADRUNNER_BEEP_BEEP_SND       NODE_13
#define DESERTGU_TRIGGER_CLICK_SND                      DESERTGU_TRIGGER_CLICK_EN

#define DESERTGU_MUSIC_ADJ                                      NODE_15

static const mw_discrete_op_amp_tvca_info desertgu_tone_music_info =
{
	RES_M(3.3),
	RES_K(10) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0, 0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_tvca_info desertgu_rifle_shot_tvca_info =
{
	RES_M(2.7),
	RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(0.47),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_mixer_desc desertgu_filter_mixer =
{
	MW_DISC_MIXER_IS_RESISTOR,
	{ RES_K(2),
	  RES_K(27),
	  RES_K(2) + RES_K(1) },
	{ 0 },
	{ 0 },
	0,
	0,
	0,
	0,
	0,
	1
};

static const mw_discrete_op_amp_filt_info desertgu_filter =
{
	1.0 / (1.0 / RES_K(2) + 1.0 / RES_K(27) + 1.0 / (RES_K(2) + RES_K(1))),
	0,
	68,
	0,
	RES_K(39),
	CAP_U(0.033),
	CAP_U(0.033),
	0,
	0,
	12,
	0
};

static const mw_discrete_mixer_desc desertgu_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(30),
	  RES_K(56),
	  RES_K(180),
	  RES_K(47),
	  RES_K(30) },
	{ 0,
	  0,
	  0,
	  0,
	  DESERTGU_MUSIC_ADJ },
	{ CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1) },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	6000
};

MW_DISCRETE_SOUND_START(mw_desertgu)

	MW_DISCRETE_INPUT_LOGIC(DESERTGU_GAME_ON_EN)
	MW_DISCRETE_INPUT_LOGIC(DESERTGU_RIFLE_SHOT_EN)
	MW_DISCRETE_INPUT_LOGIC(DESERTGU_BOTTLE_HIT_EN)
	MW_DISCRETE_INPUT_LOGIC(DESERTGU_ROAD_RUNNER_HIT_EN)
	MW_DISCRETE_INPUT_LOGIC(DESERTGU_CREATURE_HIT_EN)
	MW_DISCRETE_INPUT_LOGIC(DESERTGU_ROADRUNNER_BEEP_BEEP_EN)
	MW_DISCRETE_INPUTX_LOGIC(DESERTGU_TRIGGER_CLICK_SND, 12, 0, 0)

	MW_DISCRETE_CONSTANT(DESERTGU_MUSIC_ADJ, ((RES_M(1)) + (75000)) * 0.5)

	MIDWAY_TONE_GENERATOR(desertgu_tone_music_info)

	MW_DISCRETE_LFSR_NOISE(DESERTGU_NOISE, 1, 1, 7515, 12.0, 0, 12.0 / 2, &midway_lfsr)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_30, DESERTGU_RIFLE_SHOT_EN, 0, 0, DESERTGU_NOISE, 0, &desertgu_rifle_shot_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(12), CAP_U(.01))
	MW_DISCRETE_CRFILTER(DESERTGU_RIFLE_SHOT_SND, 1, NODE_31, RES_K(12) + RES_K(68), CAP_U(.0022))

	MW_DISCRETE_CONSTANT(DESERTGU_BOTTLE_HIT_SND, 0)

	MW_DISCRETE_CONSTANT(DESERTGU_ROAD_RUNNER_HIT_SND, 0)

	MW_DISCRETE_CONSTANT(DESERTGU_CREATURE_HIT_SND, 0)

	MW_DISCRETE_CONSTANT(DESERTGU_ROADRUNNER_BEEP_BEEP_SND, 0)

	MW_DISCRETE_MIXER3(NODE_80, 1, DESERTGU_BOTTLE_HIT_SND, DESERTGU_ROADRUNNER_BEEP_BEEP_SND, DESERTGU_TRIGGER_CLICK_SND, &desertgu_filter_mixer)
	MW_DISCRETE_OP_AMP_FILTER(NODE_81, 1, NODE_80, 0, MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1, &desertgu_filter)

	MW_DISCRETE_MIXER5(NODE_91, DESERTGU_GAME_ON_EN, DESERTGU_RIFLE_SHOT_SND, DESERTGU_ROAD_RUNNER_HIT_SND, DESERTGU_CREATURE_HIT_SND, NODE_81, MIDWAY_TONE_SND, &desertgu_mixer)

	MW_DISCRETE_OUTPUT(NODE_91, 1)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_desertgu_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_desertgu_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.8)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_desertgu_audio_1_w )
{

	coin_counter_w(0, (data >> 2) & 0x01);

	discrete_sound_w(DESERTGU_GAME_ON_EN, (data >> 3) & 0x01);

	discrete_sound_w(DESERTGU_RIFLE_SHOT_EN, (data >> 4) & 0x01);

	discrete_sound_w(DESERTGU_BOTTLE_HIT_EN, (data >> 5) & 0x01);

	discrete_sound_w(DESERTGU_ROAD_RUNNER_HIT_EN, (data >> 6) & 0x01);

	discrete_sound_w(DESERTGU_CREATURE_HIT_EN, (data >> 7) & 0x01);
}

WRITE8_HANDLER( mw_desertgu_audio_2_w )
{
	discrete_sound_w(DESERTGU_ROADRUNNER_BEEP_BEEP_EN, (data >> 0) & 0x01);

	discrete_sound_w(DESERTGU_TRIGGER_CLICK_EN, (data >> 1) & 0x01);

	desertgu_controller_select_w(0, ((data >> 3) & 0x01) << 3);

}

#define DPLAY_GAME_ON_EN        NODE_01
#define DPLAY_TONE_ON_EN        NODE_02
#define DPLAY_SIREN_EN          NODE_03
#define DPLAY_WHISTLE_EN        NODE_04
#define DPLAY_CHEER_EN          NODE_05

#define DPLAY_NOISE                     NODE_06
#define DPLAY_TONE_SND          NODE_07
#define DPLAY_SIREN_SND         NODE_08
#define DPLAY_WHISTLE_SND       NODE_09
#define DPLAY_CHEER_SND         NODE_10

#define DPLAY_MUSIC_ADJ         NODE_11

static const mw_discrete_lfsr_desc dplay_lfsr =
{
	MW_DISC_CLK_IS_FREQ,
	17,

	0x1ffff,
	4,
	16,
	MW_DISC_LFSR_XOR,
	MW_DISC_LFSR_OR,
	MW_DISC_LFSR_REPLACE,
	0x000001,
	0,
	8
};

static const mw_discrete_op_amp_tvca_info dplay_music_tvca_info =
{
	RES_M(3.3),
	RES_K(10) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_integrate_info dplay_siren_integrate_info =
{
	MW_DISC_INTEGRATE_OP_AMP_1 | MW_DISC_OP_AMP_IS_NORTON,
	RES_M(1),
	RES_K(100),
	0,
	CAP_U(3.3),
	12,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_osc_info dplay_siren_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_2 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	RES_K(390),
	RES_M(5.6),
	RES_M(1),
	RES_M(1.5),
	RES_M(3.3),
	RES_K(56),
	0,
	0,
	CAP_U(0.0022),
	12
};

static const mw_discrete_integrate_info dplay_whistle_integrate_info =
{
	MW_DISC_INTEGRATE_OP_AMP_1 | MW_DISC_OP_AMP_IS_NORTON,
	RES_M(1),
	RES_K(220) + RES_K(10),
	0,
	CAP_U(3.3),
	12,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_osc_info dplay_whistle_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_2 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	RES_K(510),
	RES_M(5.6),
	RES_M(1),
	RES_M(1.5),
	RES_M(3.3),
	RES_K(300),
	0,
	0,
	CAP_P(220),
	12
};

static const mw_discrete_integrate_info dplay_cheer_integrate_info =
{
	MW_DISC_INTEGRATE_OP_AMP_1 | MW_DISC_OP_AMP_IS_NORTON,
	RES_M(1.5),
	RES_K(100),
	0,
	CAP_U(4.7),
	12,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_filt_info dplay_cheer_filter =
{
	RES_K(100),
	0,
	RES_K(100),
	0,
	RES_K(150),
	CAP_U(0.0047),
	CAP_U(0.0047),
	0,
	0,
	12,
	0
};

static const mw_discrete_mixer_desc dplay_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(68),
	  RES_K(68),
	  RES_K(68),
	  RES_K(18),
	  RES_K(68) },
	{ 0,
	  0,
	  0,
	  0,
	  DPLAY_MUSIC_ADJ },
	{ CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1) }
	, 0, RES_K(100), 0, CAP_U(0.1), 0,
	2000
};

MW_DISCRETE_SOUND_START(mw_dplay)

	MW_DISCRETE_INPUT_LOGIC(DPLAY_GAME_ON_EN)
	MW_DISCRETE_INPUT_LOGIC(DPLAY_TONE_ON_EN)
	MW_DISCRETE_INPUTX_LOGIC(DPLAY_SIREN_EN, 5, 0, 0)
	MW_DISCRETE_INPUTX_LOGIC(DPLAY_WHISTLE_EN, 12, 0, 0)
	MW_DISCRETE_INPUTX_LOGIC(DPLAY_CHEER_EN, 5, 0, 0)

	MW_DISCRETE_CONSTANT(DPLAY_MUSIC_ADJ, ((RES_M(1)) + (1000)) * 0.5)

	MIDWAY_TONE_GENERATOR(dplay_music_tvca_info)

	MW_DISCRETE_OP_AMP_TRIG_VCA(DPLAY_TONE_SND, MIDWAY_TONE_BEFORE_AMP_SND, DPLAY_TONE_ON_EN, 0, 12, 0, &dplay_music_tvca_info)

	MW_DISCRETE_INTEGRATE(NODE_30,
		DPLAY_SIREN_EN,
		0,
		&dplay_siren_integrate_info)
	MW_DISCRETE_OP_AMP_VCO1(DPLAY_SIREN_SND,
		1,
		NODE_30,
		&dplay_siren_osc)

	MW_DISCRETE_INTEGRATE(NODE_40,
		DPLAY_WHISTLE_EN,
		0,
		&dplay_whistle_integrate_info)
	MW_DISCRETE_OP_AMP_VCO1(DPLAY_WHISTLE_SND,
		1,
		NODE_40,
		&dplay_whistle_osc)

	MW_DISCRETE_LFSR_NOISE(DPLAY_NOISE, 1,
		1, 7700, 12.0, 0, 12.0 / 2, &dplay_lfsr)

	MW_DISCRETE_INTEGRATE(NODE_50, DPLAY_CHEER_EN, 0, &dplay_cheer_integrate_info)
	MW_DISCRETE_SWITCH(NODE_51, 1, DPLAY_NOISE, 0, NODE_50)
	MW_DISCRETE_OP_AMP_FILTER(DPLAY_CHEER_SND, 1, NODE_51, 0, MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M, &dplay_cheer_filter)

	MW_DISCRETE_MIXER5(NODE_91, DPLAY_GAME_ON_EN, DPLAY_TONE_SND, DPLAY_SIREN_SND, DPLAY_WHISTLE_SND, DPLAY_CHEER_SND, MIDWAY_TONE_SND, &dplay_mixer)

	MW_DISCRETE_OUTPUT(NODE_91, 1)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_dplay_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_dplay_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.8)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_dplay_audio_w )
{
	discrete_sound_w(DPLAY_TONE_ON_EN, (data >> 0) & 0x01);

	discrete_sound_w(DPLAY_CHEER_EN, (data >> 1) & 0x01);

	discrete_sound_w(DPLAY_SIREN_EN, (data >> 2) & 0x01);

	discrete_sound_w(DPLAY_WHISTLE_EN, (data >> 3) & 0x01);

	discrete_sound_w(DPLAY_GAME_ON_EN, (data >> 4) & 0x01);

	coin_counter_w(0, (data >> 5) & 0x01);

}

static const char *gmissile_sample_names[] =
{
	"*gmissile",
	"1.wav",
	"2.wav",
	0
};

static struct Samplesinterface gmissile_samples_interface =
{
	2,
	gmissile_sample_names
};

MACHINE_DRIVER_START(mw_gmissile_audio)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(gmissile_samples_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.5)
	MDRV_SOUND_ROUTE(1, "right", 0.5)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_gmissile_audio_1_w )
{

	UINT8 rising_bits = data & ~port_1_last;

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	if (rising_bits & 0x10) mw_sample_start(1, 0, 0, 0);

	if (rising_bits & 0x20) mw_sample_start(0, 0, 1, 0);

	if (rising_bits & 0x40) mw_sample_start(0, 0, 0, 0);

	if (rising_bits & 0x80) mw_sample_start(1, 0, 1, 0);

	port_1_last = data;
}

WRITE8_HANDLER( mw_gmissile_audio_2_w )
{

}

WRITE8_HANDLER( mw_gmissile_audio_3_w )
{

}

static const char *m4_sample_names[] =
{
	"*m4",
	"1.wav",
	"2.wav",
	0
};

static struct Samplesinterface m4_samples_interface =
{
	4,
	m4_sample_names
};

MACHINE_DRIVER_START(mw_m4_audio)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(m4_samples_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.5)
	MDRV_SOUND_ROUTE(1, "left", 0.5)
	MDRV_SOUND_ROUTE(2, "right", 0.5)
	MDRV_SOUND_ROUTE(3, "right", 0.5)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_m4_audio_1_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	if (rising_bits & 0x10) mw_sample_start(0, 0, 0, 0);

	if (rising_bits & 0x20) mw_sample_start(1, 0, 0, 0);

	if (rising_bits & 0x40) mw_sample_start(0, 1, 1, 0);

	if (rising_bits & 0x80) mw_sample_start(1, 1, 1, 0);

	port_1_last = data;
}

WRITE8_HANDLER( mw_m4_audio_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	if (rising_bits & 0x01) mw_sample_start(0, 1, 1, 0);

	if (rising_bits & 0x02) mw_sample_start(1, 1, 1, 0);

	port_2_last = data;
}

#define CLOWNS_POP_BOTTOM_EN            NODE_01
#define CLOWNS_POP_MIDDLE_EN            NODE_02
#define CLOWNS_POP_TOP_EN                       NODE_03
#define CLOWNS_SPRINGBOARD_HIT_EN       NODE_04
#define CLOWNS_SPRINGBOARD_MISS_EN      NODE_05

#define CLOWNS_NOISE                            NODE_06
#define CLOWNS_POP_SND                          NODE_07
#define CLOWNS_SB_HIT_SND                       NODE_08
#define CLOWNS_SB_MISS_SND                      NODE_09

#define CLOWNS_MUSIC_ADJ                        NODE_11

static const mw_discrete_op_amp_tvca_info clowns_music_tvca_info =
{
	RES_M(3.3),
	RES_K(10) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_tvca_info clowns_pop_tvca_info =
{
	RES_M(2.7),
	RES_K(680),
	0,
	RES_K(680),
	RES_K(1),
	0,
	RES_K(470),
	RES_K(1),
	RES_K(510),
	RES_K(1),
	RES_K(680),
	CAP_U(.015),
	CAP_U(.1),
	CAP_U(.082),
	5,
	5,
	5,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG2
};

static const mw_discrete_op_amp_osc_info clowns_sb_hit_osc_info =
{
	MW_DISC_OP_AMP_OSCILLATOR_1 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	RES_K(820),
	RES_K(33),
	RES_K(150),
	RES_K(240),
	RES_M(1),
	0,
	0,
	0,
	CAP_U(0.01),
	12
};

static const mw_discrete_op_amp_tvca_info clowns_sb_hit_tvca_info =
{
	RES_M(2.7),
	RES_K(680),
	0,
	RES_K(680),
	RES_K(1),
	0,
	RES_K(680),
	0, 0, 0, 0,
	CAP_U(1),
	0,
	0,
	5,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_mixer_desc clowns_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(10),
	  RES_K(10),
	  RES_K(10) + 1.0 / (1.0 / RES_K(15) + 1.0 / RES_K(39)),
	  RES_K(1) },
	{ 0,
	  0,
	  0,
	  CLOWNS_MUSIC_ADJ },
	{ 0,
	  CAP_U(0.022),
	  0,
	  0 },
	0,
	RES_K(100),
	0,
	CAP_U(1),
	0,
	1
};

MW_DISCRETE_SOUND_START(mw_clowns)

	MW_DISCRETE_INPUT_LOGIC(CLOWNS_POP_BOTTOM_EN)
	MW_DISCRETE_INPUT_LOGIC(CLOWNS_POP_MIDDLE_EN)
	MW_DISCRETE_INPUT_LOGIC(CLOWNS_POP_TOP_EN)
	MW_DISCRETE_INPUT_LOGIC(CLOWNS_SPRINGBOARD_HIT_EN)
	MW_DISCRETE_INPUT_LOGIC(CLOWNS_SPRINGBOARD_MISS_EN)

	MW_DISCRETE_CONSTANT(CLOWNS_MUSIC_ADJ, ((RES_M(1)) + (7000)) * 0.5)

	MIDWAY_TONE_GENERATOR(clowns_music_tvca_info)

	MW_DISCRETE_LFSR_NOISE(CLOWNS_NOISE, 1, 1, 7700, 12.0, 0, 12.0 / 2, &midway_lfsr)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_30, CLOWNS_POP_TOP_EN, CLOWNS_POP_MIDDLE_EN, CLOWNS_POP_BOTTOM_EN, CLOWNS_NOISE, 0, &clowns_pop_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(15), CAP_U(.01))
	MW_DISCRETE_CRFILTER(NODE_32, 1, NODE_31, RES_K(15) + RES_K(39), CAP_U(.01))
	MW_DISCRETE_GAIN(CLOWNS_POP_SND, NODE_32, RES_K(39) / (RES_K(15) + RES_K(39)))

	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_40, 1, &clowns_sb_hit_osc_info)
	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_41, CLOWNS_SPRINGBOARD_HIT_EN, 0, 0, NODE_40, 0, &clowns_sb_hit_tvca_info)

	MW_DISCRETE_FILTER2(NODE_42, 1, NODE_41, 500, 1.0 / .8, MW_DISC_FILTER_LOWPASS)

	MW_DISCRETE_GAIN(CLOWNS_SB_HIT_SND, NODE_42, 0.5)

	MW_DISCRETE_CONSTANT(CLOWNS_SB_MISS_SND, 0)

	MW_DISCRETE_MIXER4(NODE_91, 1, CLOWNS_SB_HIT_SND, CLOWNS_SB_MISS_SND, CLOWNS_POP_SND, MIDWAY_TONE_SND, &clowns_mixer)

	MW_DISCRETE_OUTPUT(NODE_91, 11000)
MW_DISCRETE_SOUND_END

static const char *clowns_sample_names[] =
{
	"*clowns",
	"miss.wav",
	0
};

static struct Samplesinterface clowns_samples_interface =
{
	1,
	clowns_sample_names
};

MACHINE_DRIVER_START(mw_clowns_audio)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(clowns_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.70)

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_clowns_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_clowns_audio_1_w )
{
	mw_controller_select((data >> 1) & 1);
	coin_counter_w(0, (data >> 0) & 0x01);

}

WRITE8_HANDLER( mw_clowns_audio_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	discrete_sound_w(CLOWNS_POP_BOTTOM_EN, (data >> 0) & 0x01);

	discrete_sound_w(CLOWNS_POP_MIDDLE_EN, (data >> 1) & 0x01);

	discrete_sound_w(CLOWNS_POP_TOP_EN, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	discrete_sound_w(CLOWNS_SPRINGBOARD_HIT_EN, (data >> 4) & 0x01);

	if (rising_bits & 0x20) mw_sample_start(0, 0, 0, 0);

	port_2_last = data;
}

static struct SN76477interface spcenctr_sn76477_interface =
{
	0, 0, 0, 0, 0, RES_K(100), RES_K(56), RES_K(10), 0, CAP_U(0.047), RES_K(56), 5.0, RES_K(150), CAP_U(1.0), 0, 0
};

#define SPCENCTR_ENEMY_SHIP_SHOT_EN             NODE_01
#define SPCENCTR_PLAYER_SHOT_EN                 NODE_02
#define SPCENCTR_SCREECH_EN                             NODE_03
#define SPCENCTR_CRASH_EN                               NODE_04
#define SPCENCTR_EXPLOSION_EN                   NODE_05
#define SPCENCTR_BONUS_EN                               NODE_06
#define SPCENCTR_WIND_DATA                              NODE_07

#define SPCENCTR_NOISE                                  NODE_10
#define SPCENCTR_ENEMY_SHIP_SHOT_SND    NODE_11
#define SPCENCTR_PLAYER_SHOT_SND                NODE_12
#define SPCENCTR_SCREECH_SND                    NODE_13
#define SPCENCTR_CRASH_SND                              NODE_14
#define SPCENCTR_EXPLOSION_SND                  NODE_15
#define SPCENCTR_BONUS_SND                              NODE_16
#define SPCENCTR_WIND_SND                               NODE_17

static const mw_discrete_op_amp_info spcenctr_enemy_ship_shot_op_amp_E1 =
{
	MW_DISC_OP_AMP_IS_NORTON,
	0,
	RES_K(510),
	RES_M(2.2),
	RES_M(2.2),
	CAP_U(0.1),
	0,
	12
};

static const mw_discrete_op_amp_osc_info spcenctr_enemy_ship_shot_op_amp_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_3 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	RES_K(560),
	RES_K(7.5),
	RES_K(22),
	RES_K(47),
	RES_K(100),
	0,
	0,
	0,
	CAP_U(0.0022),
	12,
};

static const mw_discrete_op_amp_info spcenctr_enemy_ship_shot_op_amp_D1 =
{
	MW_DISC_OP_AMP_IS_NORTON,
	RES_K(100),
	RES_K(100),
	RES_M(2.7),
	RES_K(100),
	0,
	0,
	12
};

static const mw_discrete_op_amp_filt_info spcenctr_enemy_ship_shot_filt =
{
	RES_K(100),
	RES_K(10),
	RES_M(4.3),
	0,
	RES_M(2.2),
	CAP_U(0.001),
	CAP_U(0.001),
	0,
	0,
	12,
	0
};

static const mw_discrete_op_amp_1sht_info spcenctr_player_shot_1sht =
{
	MW_DISC_OP_AMP_1SHT_1 | MW_DISC_OP_AMP_IS_NORTON,
	RES_M(4.7),
	RES_K(100),
	RES_M(1),
	RES_M(1),
	RES_M(2.2),
	CAP_U(1),
	CAP_P(470),
	0,
	12
};

static const mw_discrete_op_amp_info spcenctr_player_shot_op_amp_E1 =
{
	MW_DISC_OP_AMP_IS_NORTON,
	0,
	RES_K(10),
	RES_M(1.5),
	0,
	CAP_U(0.22),
	0,
	12
};

static const mw_discrete_op_amp_osc_info spcenctr_player_shot_op_amp_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_3 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	1.0 / (1.0 / RES_M(1) + 1.0 / RES_K(330)) + RES_M(1.5),
	RES_M(1),
	RES_K(560),
	RES_M(2.7),
	RES_M(1),
	RES_M(4.7),
	RES_M(3.3),
	0,
	CAP_P(330),
	12,
};

static const mw_discrete_op_amp_info spcenctr_player_shot_op_amp_C1 =
{
	MW_DISC_OP_AMP_IS_NORTON,
	RES_K(560),
	RES_K(470),
	RES_M(2.7),
	RES_K(560),
	0,
	0,
	12
};

static const mw_discrete_op_amp_tvca_info spcenctr_player_shot_tvca =
{
	RES_M(2.7),
	RES_K(560),
	0,
	RES_K(560),
	RES_K(1),
	0,
	RES_K(560),
	0,
	0,
	0,
	0,
	CAP_U(1),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_tvca_info spcenctr_crash_tvca =
{
	RES_M(2.7),
	RES_K(470),
	0,
	RES_K(470),
	RES_K(1),
	0,
	RES_K(470),
	0,
	0,
	0,
	0,
	CAP_U(2.2),
	0,
	0,
	5,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_op_amp_tvca_info spcenctr_explosion_tvca =
{
	RES_M(2.7),
	RES_K(680),
	0,
	RES_K(680),
	RES_K(1),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(2.2),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_555_desc spcenctr_555_bonus =
{
	MW_DISC_555_OUT_SQW | MW_DISC_555_OUT_DC,
	5,
	MW_DEFAULT_555_VALUES
};

static const mw_discrete_mixer_desc spcenctr_mixer =
{
	MW_DISC_MIXER_IS_RESISTOR,
	{ RES_K(15),
	  RES_K(15),
	  RES_K(22),
	  RES_K(3.6),
	  RES_K(1.8) + RES_K(3.6) + RES_K(4.7),
	  RES_K(27),
	  RES_K(27) },
	{ 0 },
	{ 0,
	  CAP_U(0.001),
	  CAP_U(0.1),
	  CAP_U(1),
	  0,
	  0,
	  CAP_U(10) },
	0,
	0,
	0,
	CAP_U(1),
	0,
	1
};

MW_DISCRETE_SOUND_START(mw_spcenctr)

	MW_DISCRETE_INPUTX_LOGIC(SPCENCTR_ENEMY_SHIP_SHOT_EN, 12, 0, 0)
	MW_DISCRETE_INPUTX_LOGIC(SPCENCTR_PLAYER_SHOT_EN, 12, 0, 0)
	MW_DISCRETE_INPUT_LOGIC(SPCENCTR_SCREECH_EN)
	MW_DISCRETE_INPUT_LOGIC(SPCENCTR_CRASH_EN)
	MW_DISCRETE_INPUT_LOGIC(SPCENCTR_EXPLOSION_EN)
	MW_DISCRETE_INPUT_LOGIC(SPCENCTR_BONUS_EN)
	MW_DISCRETE_INPUT_DATA(SPCENCTR_WIND_DATA)

	MW_DISCRETE_LFSR_NOISE(SPCENCTR_NOISE,
		1,
		1,
		7515,
		12,
		0,
		12.0 / 2,
		&midway_lfsr)

	MW_DISCRETE_OP_AMP(NODE_20,
		1,
		0,
		SPCENCTR_ENEMY_SHIP_SHOT_EN,
		&spcenctr_enemy_ship_shot_op_amp_E1)
	MW_DISCRETE_OP_AMP_VCO1(NODE_21,
		1,
		NODE_20,
		&spcenctr_enemy_ship_shot_op_amp_osc)
	MW_DISCRETE_OP_AMP(NODE_22,
		1,
		NODE_21,
		NODE_20,
		&spcenctr_enemy_ship_shot_op_amp_D1)
	MW_DISCRETE_OP_AMP_FILTER(NODE_23,
		1,
		NODE_22,
		0,
		MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M | MW_DISC_OP_AMP_IS_NORTON,
		&spcenctr_enemy_ship_shot_filt)
	MW_DISCRETE_CRFILTER(SPCENCTR_ENEMY_SHIP_SHOT_SND,
		1,
		NODE_23,
		RES_K(1.8),
		CAP_U(0.1))

	MW_DISCRETE_OP_AMP_ONESHOT(NODE_30,
		SPCENCTR_PLAYER_SHOT_EN,
		&spcenctr_player_shot_1sht)
	MW_DISCRETE_OP_AMP(NODE_31,
		1,
		0,
		NODE_30,
		&spcenctr_player_shot_op_amp_E1)

	MW_DISCRETE_ADDER2(NODE_32,
		1,
		NODE_31,
		-0.5)
	MW_DISCRETE_CLAMP(NODE_33,
		1,
		NODE_32,
		0,
		12,
		0)
	MW_DISCRETE_CRFILTER(NODE_34,
		1,
		SPCENCTR_NOISE,
		RES_M(1) + RES_K(330),
		CAP_U(0.1))
	MW_DISCRETE_GAIN(NODE_35,
		NODE_34,
		RES_K(330) / (RES_M(1) + RES_K(330)))
	MW_DISCRETE_OP_AMP_VCO2(NODE_36,
		1,
		NODE_35,
		NODE_33,
		&spcenctr_player_shot_op_amp_osc)
	MW_DISCRETE_OP_AMP(NODE_37,
		1,
		NODE_36,
		NODE_33,
		&spcenctr_player_shot_op_amp_C1)
	MW_DISCRETE_OP_AMP_TRIG_VCA(SPCENCTR_PLAYER_SHOT_SND,
		SPCENCTR_PLAYER_SHOT_EN,
		0,
		0,
		NODE_37,
		0,
		&spcenctr_player_shot_tvca)

	MW_DISCRETE_CONSTANT(SPCENCTR_SCREECH_SND, 0)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_60,
		SPCENCTR_CRASH_EN,
		0,
		0,
		SPCENCTR_NOISE,
		0,
		&spcenctr_crash_tvca)

	MW_DISCRETE_ADDER2(NODE_61,
		1,
		NODE_60,
		-6.8)
	MW_DISCRETE_FILTER2(NODE_62,
		1,
		NODE_61,
		130,
		1.0 / 8,
		MW_DISC_FILTER_BANDPASS)
	MW_DISCRETE_GAIN(NODE_63,
		NODE_62,
		6)
	MW_DISCRETE_ADDER2(NODE_64,
		1,
		NODE_63,
		6.8)
	MW_DISCRETE_CLAMP(SPCENCTR_CRASH_SND,
		1,
		NODE_64,
		0,
		12.0 - MW_OP_AMP_NORTON_VBE,
		0)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_70,
		SPCENCTR_EXPLOSION_EN,
		0,
		0,
		SPCENCTR_NOISE,
		0,
		&spcenctr_explosion_tvca)
	MW_DISCRETE_RCFILTER(NODE_71,
		1,
		NODE_70,
		RES_K(1.8),
		CAP_U(0.22))
	MW_DISCRETE_RCFILTER(SPCENCTR_EXPLOSION_SND,
		1,
		NODE_71,
		RES_K(1.8) + RES_K(3.6),
		CAP_U(0.22))

	MW_DISCRETE_555_ASTABLE(NODE_80,

		SPCENCTR_BONUS_EN,
		RES_K(1),
		RES_K(27),
		CAP_U(0.047),
		&spcenctr_555_bonus)
	MW_DISCRETE_555_ASTABLE(NODE_81,
		SPCENCTR_BONUS_EN,
		RES_K(100),
		RES_K(47),
		CAP_U(1),
		&spcenctr_555_bonus)
	MW_DISCRETE_LOGIC_AND3(NODE_82,
		1,
		NODE_80,
		NODE_81,
		SPCENCTR_BONUS_EN)
	MW_DISCRETE_GAIN(SPCENCTR_BONUS_SND,
		NODE_82,
		MW_DEFAULT_TTL_V_LOGIC_1)

	MW_DISCRETE_CONSTANT(SPCENCTR_WIND_SND, 0)

	MW_DISCRETE_MIXER7(NODE_91,
		1,
		SPCENCTR_ENEMY_SHIP_SHOT_SND,
		SPCENCTR_PLAYER_SHOT_SND,
		SPCENCTR_SCREECH_SND,
		SPCENCTR_CRASH_SND,
		SPCENCTR_EXPLOSION_SND,
		SPCENCTR_BONUS_SND,
		SPCENCTR_WIND_SND,
		&spcenctr_mixer)

	MW_DISCRETE_OUTPUT(NODE_91, 20000)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_spcenctr_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(spcenctr_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_spcenctr_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_spcenctr_audio_1_w )
{
	sound_global_enable((data >> 0) & 0x01);

	discrete_sound_w(SPCENCTR_CRASH_EN, (data >> 2) & 0x01);

}

WRITE8_HANDLER( mw_spcenctr_audio_2_w )
{

	discrete_sound_w(SPCENCTR_EXPLOSION_EN, (data >> 4) & 0x01);

	discrete_sound_w(SPCENCTR_PLAYER_SHOT_EN, (data >> 5) & 0x01);

	port_2_last = data;
}

WRITE8_HANDLER( mw_spcenctr_audio_3_w )
{

	discrete_sound_w(SPCENCTR_ENEMY_SHIP_SHOT_EN, (data >> 1) & 0x01);

	discrete_sound_w(SPCENCTR_BONUS_EN, (data >> 4) & 0x01);

	SN76477_enable_w(0, (data >> 5) & 0x01);

}

static const char *phantom2_sample_names[] =
{
	"*phantom2",
	"1.wav",
	"2.wav",
	0
};

static struct Samplesinterface phantom2_samples_interface =
{
	2,
	phantom2_sample_names
};

MACHINE_DRIVER_START(mw_phantom2_audio)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(phantom2_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_phantom2_audio_1_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	if (rising_bits & 0x01) mw_sample_start(0, 0, 0, 0);

	sound_global_enable((data >> 2) & 0x01);

	coin_counter_w(0, (data >> 3) & 0x01);

	port_1_last = data;
}

WRITE8_HANDLER( mw_phantom2_audio_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	if (rising_bits & 0x08) mw_sample_start(0, 1, 1, 0);

	port_2_last = data;
}

#define BOWLER_FOWL_EN                  NODE_01

#define BOWLER_FOWL_SND                 NODE_10

static const mw_discrete_op_amp_tvca_info bowler_fowl_tvca =
{
	RES_M(2.7),
	RES_K(680),
	0,
	RES_K(680),
	RES_K(1),
	0,
	RES_K(300),
	0,
	0,
	0,
	0,
	CAP_U(0.1),
	0,
	0,
	5,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

MW_DISCRETE_SOUND_START(mw_bowler)

	MW_DISCRETE_INPUT_LOGIC(BOWLER_FOWL_EN)

	MW_DISCRETE_SQUAREWFIX(NODE_20,
		1,
		180,
		MW_DEFAULT_TTL_V_LOGIC_1,
		50,
		MW_DEFAULT_TTL_V_LOGIC_1 / 2,
		0)
	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_21,
		BOWLER_FOWL_EN,
		0,
		0,
		NODE_20,
		0,
		&bowler_fowl_tvca)
	MW_DISCRETE_CRFILTER(BOWLER_FOWL_SND,
		1,
		NODE_21,
		RES_K(68),
		CAP_U(0.1))

	MW_DISCRETE_OUTPUT(BOWLER_FOWL_SND, 10000)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_bowler_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_bowler_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_bowler_audio_1_w )
{

	coin_counter_w(0, (data >> 1) & 0x01);

	sound_global_enable((data >> 2) & 0x01);

	discrete_sound_w(BOWLER_FOWL_EN, (data >> 3) & 0x01);

}

WRITE8_HANDLER( mw_bowler_audio_2_w )
{

}

WRITE8_HANDLER( mw_bowler_audio_3_w )
{

}

WRITE8_HANDLER( mw_bowler_audio_4_w )
{

}

WRITE8_HANDLER( mw_bowler_audio_5_w )
{

}

WRITE8_HANDLER( mw_bowler_audio_6_w )
{

}
static const mw_discrete_op_amp_tvca_info midway_music_tvca_info =
{
	RES_M(3.3),
	RES_K(10) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

#define SPACWALK_TARGET_HIT_BOTTOM_EN         NODE_01
#define SPACWALK_TARGET_HIT_MIDDLE_EN         NODE_02
#define SPACWALK_TARGET_HIT_TOP_EN            NODE_03
#define SPACWALK_SPRINGBOARD_HIT1_EN          NODE_04
#define SPACWALK_SPRINGBOARD_HIT2_EN          NODE_05
#define SPACWALK_SPRINGBOARD_MISS_EN          NODE_06
#define SPACWALK_SPACE_SHIP_EN                NODE_07

#define SPACWALK_NOISE                        NODE_10
#define SPACWALK_TARGET_HIT_SND               NODE_11
#define SPACWALK_SPRINGBOARD_HIT1_SND         NODE_12
#define SPACWALK_SPRINGBOARD_HIT2_SND         NODE_13
#define SPACWALK_SPRINGBOARD_MISS_SND         NODE_14
#define SPACWALK_SPACE_SHIP_SND               NODE_15

#define SPACWALK_R507_POT                     NODE_19

#define SPACWALK_R200       RES_K(820)
#define SPACWALK_R201       RES_K(150)
#define SPACWALK_R202       RES_M(1)
#define SPACWALK_R203       RES_K(82)
#define SPACWALK_R204       RES_K(240)
#define SPACWALK_R205       RES_K(220)
#define SPACWALK_R206       RES_K(120)
#define SPACWALK_R207       RES_M(1)
#define SPACWALK_R208       RES_K(300)
#define SPACWALK_R210       RES_K(56)
#define SPACWALK_R211       RES_K(100)
#define SPACWALK_R213       RES_K(300)
#define SPACWALK_R214       RES_K(27)
#define SPACWALK_R215       RES_K(51)
#define SPACWALK_R216       RES_K(30)
#define SPACWALK_R300       RES_K(270)
#define SPACWALK_R301       RES_K(300)
#define SPACWALK_R302       RES_K(330)
#define SPACWALK_R303       RES_K(680)
#define SPACWALK_R304       RES_M(1)
#define SPACWALK_R305       RES_K(3680)
#define SPACWALK_R307       RES_K(20)
#define SPACWALK_R308       RES_K(20)
#define SPACWALK_R400       RES_K(1)
#define SPACWALK_R401       RES_K(200)
#define SPACWALK_R403       RES_K(51)
#define SPACWALK_R404       RES_K(220)
#define SPACWALK_R406       RES_M(1)
#define SPACWALK_R407       RES_K(820)
#define SPACWALK_R410       RES_K(47)
#define SPACWALK_R411       RES_K(300)
#define SPACWALK_R412       RES_K(330)
#define SPACWALK_R413       RES_M(1)
#define SPACWALK_R414       RES_M(1)
#define SPACWALK_R416       RES_M(4.7)
#define SPACWALK_R417       RES_K(10)
#define SPACWALK_R418       RES_K(100)
#define SPACWALK_R419       RES_K(2.7)
#define SPACWALK_R420       RES_K(20)
#define SPACWALK_R421       RES_K(11)
#define SPACWALK_R422       RES_K(75)
#define SPACWALK_R507       RES_M(1)
#define SPACWALK_RJ3        RES_K(1)

#define SPACWALK_C200       CAP_U(0.0022)
#define SPACWALK_C201       CAP_U(3.3)
#define SPACWALK_C203       CAP_U(0.0033)
#define SPACWALK_C204       CAP_U(0.0033)
#define SPACWALK_C300       CAP_U(2.2)
#define SPACWALK_C301       CAP_U(2.2)
#define SPACWALK_C302       CAP_U(2.2)
#define SPACWALK_C303       CAP_U(0.0047)
#define SPACWALK_C304       CAP_U(0.0047)
#define SPACWALK_C401       CAP_U(1)
#define SPACWALK_C402       CAP_U(0.68)
#define SPACWALK_C403       CAP_U(0.0022)
#define SPACWALK_C451       CAP_U(0.001)
#define SPACWALK_C452       CAP_U(0.001)
#define SPACWALK_C453       CAP_U(0.001)
#define SPACWALK_C602       CAP_U(1)

static const mw_discrete_op_amp_tvca_info spacwalk_hit_tvca_info =
{
	SPACWALK_R304,
	SPACWALK_R303,
	0,
	SPACWALK_R305,
	SPACWALK_RJ3,
	0,
	SPACWALK_R300,
	SPACWALK_RJ3,
	SPACWALK_R301,
	SPACWALK_RJ3,
	SPACWALK_R302,
	SPACWALK_C300,
	SPACWALK_C301,
	SPACWALK_C302,
	5,
	5,
	5,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG2
};

static const mw_discrete_op_amp_osc_info spacwalk_sb_hit_vco =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_3 | MW_DISC_OP_AMP_IS_NORTON,
	RES_K(330 + 150 + 30), SPACWALK_R203, SPACWALK_R201, SPACWALK_R204,
	SPACWALK_R202, 0, SPACWALK_R200, 0,
	SPACWALK_C200, 12
};

static const mw_discrete_op_amp_tvca_info spacwalk_sb_hit_tvca_info =
{
	SPACWALK_R207,
	SPACWALK_R205,
	0,
	SPACWALK_R208,
	SPACWALK_RJ3,
	0,
	SPACWALK_R206,
	0,
	0,
	0,
	0,
	SPACWALK_C201,
	0,
	0,
	5,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_integrate_info spacwalk_sb_miss_integrate =
{
	MW_DISC_INTEGRATE_OP_AMP_1 | MW_DISC_OP_AMP_IS_NORTON,
	SPACWALK_R406, SPACWALK_R401, 0, SPACWALK_C402,
	12, 12,
	0, 0, 0
};

static const mw_discrete_op_amp_osc_info spacwalk_sb_miss_vco =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_3 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	SPACWALK_R407, SPACWALK_R412, SPACWALK_R410, SPACWALK_R411, SPACWALK_R413, 0, 0, 0,
	SPACWALK_C403, 12
};

static const mw_discrete_op_amp_filt_info spacwalk_sb_miss_filter =
{

	SPACWALK_R417, 0, SPACWALK_R414, 0, SPACWALK_R416,
	SPACWALK_C451, SPACWALK_C452, 0,
	0, 12, 0
};

static const mw_discrete_op_amp_info spacwalk_sb_miss_amp =
{
	MW_DISC_OP_AMP_IS_NORTON,
	SPACWALK_R418, SPACWALK_R404, 0, SPACWALK_R403,
	0,
	0, 12,
};

static const mw_discrete_op_amp_osc_info spacwalk_spaceship_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_2 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_CAP,
	RES_K(75), RES_M(1), RES_M(6.8), RES_M(2.4), 0, 0, 0, 0,
	CAP_U(2.2), 12
};

static const mw_discrete_op_amp_osc_info spacwalk_spaceship_vco =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_1 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_CAP,
	RES_K(680), RES_K(300), RES_K(100), RES_K(150), RES_K(120), 0, 0, 0,
	CAP_U(0.0012), 12
};

static const mw_discrete_mixer_desc spacwalk_mixer =
{
	MW_DISC_MIXER_IS_RESISTOR,
	{ SPACWALK_R422, SPACWALK_R422, RES_K(39 + 10 + 1), SPACWALK_R421, SPACWALK_R420, SPACWALK_R419 },
	{ 0, 0, 0, 0, 0, SPACWALK_R507_POT },
	{ 0 }, 0, 0, 0, SPACWALK_C602, 0, 1
};

#define SPACWALK_SPRINGBOARD_HIT_CIRCUIT(_num)                                              \
	MW_DISCRETE_RCFILTER(NODE_RELATIVE(NODE_29, _num), 1, \
		SPACWALK_NOISE, \
		RES_K(330), CAP_U(.1))                                                                  \
	MW_DISCRETE_RCFILTER(NODE_RELATIVE(NODE_31, _num), 1, \
		NODE_RELATIVE(NODE_29, _num), \
		RES_K(330) + RES_K(150), CAP_U(.1))                                                     \
	MW_DISCRETE_OP_AMP_VCO1(NODE_RELATIVE(NODE_33, _num), \
		1, \
		NODE_RELATIVE(NODE_31, _num), \
		&spacwalk_sb_hit_vco)                                                                   \
	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_RELATIVE(NODE_35, _num), \
		NODE_RELATIVE(SPACWALK_SPRINGBOARD_HIT1_EN, _num - 1), 0, 0, \
		NODE_RELATIVE(NODE_33, _num), 0, \
		&spacwalk_sb_hit_tvca_info)                                                             \
                                                               \
                                                  \
	MW_DISCRETE_FILTER2(NODE_RELATIVE(NODE_37, _num), \
		1, \
		NODE_RELATIVE(NODE_35, _num), \
		2000.0 - _num * 500, 1.0 / .8, \
		MW_DISC_FILTER_LOWPASS)                                                                    \
                                                          \
	MW_DISCRETE_GAIN(NODE_RELATIVE(SPACWALK_SPRINGBOARD_HIT1_SND, _num - 1), \
		NODE_RELATIVE(NODE_37, _num), 0.5)

static MW_DISCRETE_SOUND_START(mw_spacwalk)

	MW_DISCRETE_INPUT_LOGIC(SPACWALK_TARGET_HIT_BOTTOM_EN)
	MW_DISCRETE_INPUT_LOGIC(SPACWALK_TARGET_HIT_MIDDLE_EN)
	MW_DISCRETE_INPUT_LOGIC(SPACWALK_TARGET_HIT_TOP_EN)
	MW_DISCRETE_INPUT_LOGIC(SPACWALK_SPRINGBOARD_HIT1_EN)
	MW_DISCRETE_INPUT_LOGIC(SPACWALK_SPRINGBOARD_HIT2_EN)
	MW_DISCRETE_INPUT_LOGIC(SPACWALK_SPRINGBOARD_MISS_EN)
	MW_DISCRETE_INPUT_LOGIC(SPACWALK_SPACE_SHIP_EN)

	MW_DISCRETE_ADJUSTMENT(SPACWALK_R507_POT, 1, SPACWALK_R507, 7000, MW_DISC_LOGADJ, 4)

	MIDWAY_TONE_GENERATOR(midway_music_tvca_info)

	MW_DISCRETE_LFSR_NOISE(SPACWALK_NOISE, 1, 1, 7700, 12.0, 0, 12.0 / 2, &midway_lfsr)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_20, SPACWALK_TARGET_HIT_TOP_EN, SPACWALK_TARGET_HIT_MIDDLE_EN, SPACWALK_TARGET_HIT_BOTTOM_EN, SPACWALK_NOISE, 0, &spacwalk_hit_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_21, 1, NODE_20, SPACWALK_R307, SPACWALK_C303)
	MW_DISCRETE_RCFILTER(SPACWALK_TARGET_HIT_SND, 1, NODE_21, SPACWALK_R307 + SPACWALK_R308, SPACWALK_C304)

	SPACWALK_SPRINGBOARD_HIT_CIRCUIT(1)
	SPACWALK_SPRINGBOARD_HIT_CIRCUIT(2)

	MW_DISCRETE_RCDISC2(NODE_50, SPACWALK_SPRINGBOARD_MISS_EN, MW_OP_AMP_NORTON_VBE, RES_2_PARALLEL(SPACWALK_R401, SPACWALK_R407), 12.0 - .5, SPACWALK_R400, SPACWALK_C401)
	MW_DISCRETE_INTEGRATE(NODE_51, NODE_50, 0, &spacwalk_sb_miss_integrate)
	MW_DISCRETE_OP_AMP_VCO1(NODE_52, 1, NODE_50, &spacwalk_sb_miss_vco)
	MW_DISCRETE_CRFILTER(NODE_53, 1, NODE_52, SPACWALK_R417, SPACWALK_C453)

	MW_DISCRETE_OP_AMP_FILTER(NODE_54, 1, NODE_53, 0, MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M | MW_DISC_OP_AMP_IS_NORTON, &spacwalk_sb_miss_filter)
	MW_DISCRETE_OP_AMP(SPACWALK_SPRINGBOARD_MISS_SND, 1, NODE_54, NODE_51, &spacwalk_sb_miss_amp)

	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_60, 1, &spacwalk_spaceship_osc)
	MW_DISCRETE_OP_AMP_VCO1(NODE_61, SPACWALK_SPACE_SHIP_EN, NODE_60, &spacwalk_spaceship_vco)
	MW_DISCRETE_RCFILTER(NODE_62, 1, NODE_61, RES_K(1), CAP_U(0.15))
	MW_DISCRETE_RCFILTER(SPACWALK_SPACE_SHIP_SND, 1, NODE_62, RES_K(1) + RES_K(10), CAP_U(0.015))

	MW_DISCRETE_MIXER6(NODE_90, 1, SPACWALK_SPRINGBOARD_HIT1_SND, SPACWALK_SPRINGBOARD_HIT2_SND, SPACWALK_SPACE_SHIP_SND, SPACWALK_SPRINGBOARD_MISS_SND, SPACWALK_TARGET_HIT_SND, MIDWAY_TONE_SND, &spacwalk_mixer)
	MW_DISCRETE_OUTPUT(NODE_90, 11000)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_spacwalk_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_spacwalk_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_spacwalk_audio_1_w )
{
	coin_counter_w(0, (data >> 0) & 0x01);

	mw_controller_select((data >> 1) & 1);

	sound_global_enable((data >> 2) & 0x01);

	discrete_sound_w(SPACWALK_SPACE_SHIP_EN, (data >> 3) & 0x01);
}

WRITE8_HANDLER( mw_spacwalk_audio_2_w )
{
	discrete_sound_w(SPACWALK_TARGET_HIT_BOTTOM_EN, (data >> 0) & 0x01);

	discrete_sound_w(SPACWALK_TARGET_HIT_MIDDLE_EN, (data >> 1) & 0x01);

	discrete_sound_w(SPACWALK_TARGET_HIT_TOP_EN, (data >> 2) & 0x01);

	discrete_sound_w(SPACWALK_SPRINGBOARD_HIT1_EN, (data >> 3) & 0x01);

	discrete_sound_w(SPACWALK_SPRINGBOARD_HIT2_EN, (data >> 4) & 0x01);

	discrete_sound_w(SPACWALK_SPRINGBOARD_MISS_EN, (data >> 5) & 0x01);
}

#define SHUFFLE_ROLLING_1_EN        NODE_01
#define SHUFFLE_ROLLING_2_EN        NODE_02
#define SHUFFLE_ROLLING_3_EN        NODE_03
#define SHUFFLE_FOUL_EN             NODE_04
#define SHUFFLE_ROLLOVER_EN         NODE_05
#define SHUFFLE_CLICK_EN            NODE_06

#define SHUFFLE_NOISE               NODE_10
#define SHUFFLE_ROLLING_SND         NODE_11
#define SHUFFLE_FOUL_SND            NODE_12
#define SHUFFLE_ROLLOVER_SND        NODE_13
#define SHUFFLE_CLICK_SND           NODE_14

#define SHUFFLE_R300    RES_K(33)
#define SHUFFLE_R400    RES_K(200)
#define SHUFFLE_R401    RES_K(3)
#define SHUFFLE_R402    RES_K(5.6)
#define SHUFFLE_R403    RES_K(5.6)
#define SHUFFLE_R404    RES_M(1)
#define SHUFFLE_R406    RES_K(300)
#define SHUFFLE_R407    RES_K(680)
#define SHUFFLE_R408    RES_K(680)
#define SHUFFLE_R409    RES_K(680)
#define SHUFFLE_R410    RES_K(680)
#define SHUFFLE_R411    RES_K(680)
#define SHUFFLE_R412    RES_M(2.7)
#define SHUFFLE_R500    RES_K(300)
#define SHUFFLE_R503    RES_M(2.7)
#define SHUFFLE_R504    RES_K(680)
#define SHUFFLE_R505    RES_K(680)
#define SHUFFLE_R506    RES_K(100)
#define SHUFFLE_R507    RES_K(47)
#define SHUFFLE_R508    RES_K(47)
#define SHUFFLE_R509    RES_K(100)
#define SHUFFLE_R511    RES_M(2)
#define SHUFFLE_R512    RES_M(5.6)
#define SHUFFLE_R513    RES_K(680)
#define SHUFFLE_R514    RES_M(1.5)
#define SHUFFLE_R515    RES_M(1)
#define SHUFFLE_R516    RES_K(510)

#define SHUFFLE_C300    CAP_U(0.1)
#define SHUFFLE_C400    CAP_U(0.1)
#define SHUFFLE_C401    CAP_U(1)
#define SHUFFLE_C402    CAP_U(1)
#define SHUFFLE_C403    CAP_U(1)
#define SHUFFLE_C404    CAP_U(0.1)
#define SHUFFLE_C405    CAP_U(0.1)
#define SHUFFLE_C500    CAP_U(0.1)
#define SHUFFLE_C503    CAP_U(0.0022)
#define SHUFFLE_C504    CAP_U(0.0022)
#define SHUFFLE_C505    CAP_U(0.33)
#define SHUFFLE_C506    CAP_U(1)
#define SHUFFLE_C507    CAP_U(1)
#define SHUFFLE_C508    CAP_U(1)

static const mw_discrete_op_amp_tvca_info shuffle_rolling_tvca =
{
	SHUFFLE_R512,
	0,
	0,
	SHUFFLE_R511,
	RES_K(10),
	0,
	SHUFFLE_R516,
	RES_K(10),
	SHUFFLE_R515,
	RES_K(10),
	SHUFFLE_R514,
	SHUFFLE_C508,
	SHUFFLE_C507,
	SHUFFLE_C506,
	12,
	12,
	12,
	12,
	0,
	0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG2
};

static const mw_discrete_op_amp_info shuffle_rolling_amp =
{
	MW_DISC_OP_AMP_IS_NORTON,
	SHUFFLE_R513, SHUFFLE_R505, SHUFFLE_R503, SHUFFLE_R504,
	0,
	0, 12,
};

static const mw_discrete_op_amp_tvca_info shuffle_foul_tvca =
{
	SHUFFLE_R412,
	SHUFFLE_R411,
	0,
	SHUFFLE_R408,
	RES_K(1),
	0,
	SHUFFLE_R406,
	0,
	0,
	0,
	0,
	SHUFFLE_C404,
	0,
	0,
	5,
	0,
	0,
	12,
	0,
	0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	0,
	0,
	0
};

static const mw_discrete_op_amp_tvca_info shuffle_rollover_tvca =
{
	SHUFFLE_R404,
	SHUFFLE_R410,
	0,
	SHUFFLE_R407,
	RES_K(10),
	0,
	SHUFFLE_R409,
	0,
	0,
	0,
	0,
	SHUFFLE_C405,
	0,
	0,
	12,
	0,
	0,
	12,
	0,
	0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	0,
	0,
	0
};

static const mw_discrete_mixer_desc shuffle_mixer =
{
	MW_DISC_MIXER_IS_RESISTOR,
	{ SHUFFLE_R500, SHUFFLE_R400, SHUFFLE_R403 + SHUFFLE_R402 + SHUFFLE_R401, SHUFFLE_R300 },
	{ 0 },
	{ SHUFFLE_C500, SHUFFLE_C400, SHUFFLE_C401, SHUFFLE_C300 },
	0, 0, 0, 0, 0, 1
};

static MW_DISCRETE_SOUND_START(mw_shuffle)
	MW_DISCRETE_INPUT_LOGIC(SHUFFLE_ROLLING_1_EN)
	MW_DISCRETE_INPUT_LOGIC(SHUFFLE_ROLLING_2_EN)
	MW_DISCRETE_INPUT_LOGIC(SHUFFLE_ROLLING_3_EN)
	MW_DISCRETE_INPUT_LOGIC(SHUFFLE_FOUL_EN)
	MW_DISCRETE_INPUT_LOGIC(SHUFFLE_ROLLOVER_EN)
	MW_DISCRETE_INPUTX_LOGIC(SHUFFLE_CLICK_EN, 11.5, 0, 0)

	MW_DISCRETE_LFSR_NOISE(SHUFFLE_NOISE, 1, 1, 1210, 12.0, 0, 12.0 / 2, &midway_lfsr)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_20, SHUFFLE_ROLLING_1_EN, SHUFFLE_ROLLING_2_EN, SHUFFLE_ROLLING_3_EN, 0, 0, &shuffle_rolling_tvca)
	MW_DISCRETE_OP_AMP(NODE_21, 1, SHUFFLE_NOISE, NODE_20, &shuffle_rolling_amp)

	MW_DISCRETE_FILTER1(NODE_22, 1, NODE_21, 800, MW_DISC_FILTER_LOWPASS)
	MW_DISCRETE_GAIN(SHUFFLE_ROLLING_SND, NODE_22, .2)

	MW_DISCRETE_SQUAREWFIX(NODE_30, 1, 120, MW_DEFAULT_TTL_V_LOGIC_1, 50, MW_DEFAULT_TTL_V_LOGIC_1 / 2, 0)
	MW_DISCRETE_OP_AMP_TRIG_VCA(SHUFFLE_FOUL_SND, SHUFFLE_FOUL_EN, 0, 0, NODE_30, 0, &shuffle_foul_tvca)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_40, SHUFFLE_ROLLOVER_EN, 0, 0, SHUFFLE_NOISE, 0, &shuffle_rollover_tvca)
	MW_DISCRETE_RCFILTER(NODE_41, 1, NODE_40, SHUFFLE_R403, SHUFFLE_C403)
	MW_DISCRETE_RCFILTER(SHUFFLE_ROLLOVER_SND, 1, NODE_41, SHUFFLE_R403 + SHUFFLE_R402, SHUFFLE_C402)

	MW_DISCRETE_FILTER1(NODE_50, 1, SHUFFLE_CLICK_EN, 300, MW_DISC_FILTER_LOWPASS)
	MW_DISCRETE_GAIN(SHUFFLE_CLICK_SND, NODE_50, .3)

	MW_DISCRETE_MIXER4(NODE_90, 1, SHUFFLE_ROLLING_SND, SHUFFLE_FOUL_SND, SHUFFLE_ROLLOVER_SND, SHUFFLE_CLICK_SND, &shuffle_mixer)
	MW_DISCRETE_OUTPUT(NODE_90, 59200)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_shuffle_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_shuffle_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_shuffle_audio_1_w )
{
	discrete_sound_w(SHUFFLE_CLICK_EN, (data >> 0) & 0x01);

	discrete_sound_w(SHUFFLE_ROLLOVER_EN, (data >> 1) & 0x01);

	sound_global_enable((data >> 2) & 0x01);

	discrete_sound_w(NODE_29, (data >> 3) & 0x07);

	discrete_sound_w(SHUFFLE_ROLLING_3_EN, (data >> 3) & 0x01);
	discrete_sound_w(SHUFFLE_ROLLING_2_EN, (data >> 4) & 0x01);
	discrete_sound_w(SHUFFLE_ROLLING_1_EN, (data >> 5) & 0x01);

}

WRITE8_HANDLER( mw_shuffle_audio_2_w )
{
	discrete_sound_w(SHUFFLE_FOUL_EN, (data >> 0) & 0x01);

	coin_counter_w(0, (data >> 1) & 0x01);

}

#define DOGPATCH_GAME_ON_EN         NODE_01
#define DOGPATCH_LEFT_SHOT_EN       NODE_02
#define DOGPATCH_RIGHT_SHOT_EN      NODE_03
#define DOGPATCH_HIT_EN             NODE_04
#define DOGPATCH_PAN_DATA           NODE_05

#define DOGPATCH_NOISE              NODE_06
#define DOGPATCH_L_SHOT_SND         NODE_07
#define DOGPATCH_R_SHOT_SND         NODE_08
#define DOGPATCH_HIT_SND            NODE_09
#define DOGPATCH_L_HIT_SND          NODE_10
#define DOGPATCH_R_HIT_SND          NODE_11

static const mw_discrete_op_amp_tvca_info dogpatch_shot_tvca_info =
{
	RES_M(2.7),
	RES_K(510),
	0,
	RES_K(510),
	RES_K(10),
	0,
	RES_K(510),
	0,
	0,
	0,
	0,
	CAP_U(0.22),
	0,
	0,
	12,
	0,
	0,
	12,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

static const mw_discrete_mixer_desc dogpatch_l_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(33),
	  RES_K(33) },
	{ 0 },
	{ 0 },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	1
};

static const mw_discrete_mixer_desc dogpatch_r_mixer =
{
	MW_DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(33),
	  RES_K(33),
	  RES_K(510) + RES_K(33) },
	{ 0 },
	{ 0 },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	1
};

static MW_DISCRETE_SOUND_START(mw_dogpatch)

	MW_DISCRETE_INPUT_LOGIC(DOGPATCH_GAME_ON_EN)
	MW_DISCRETE_INPUT_LOGIC(DOGPATCH_LEFT_SHOT_EN)
	MW_DISCRETE_INPUT_LOGIC(DOGPATCH_RIGHT_SHOT_EN)
	MW_DISCRETE_INPUT_LOGIC(DOGPATCH_HIT_EN)

	MIDWAY_TONE_GENERATOR(midway_music_tvca_info)

	MW_DISCRETE_LFSR_NOISE(DOGPATCH_NOISE, 1, 1, 7700, 12.0, 0, 12.0 / 2, &midway_lfsr)

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_20, DOGPATCH_LEFT_SHOT_EN, 0, 0, DOGPATCH_NOISE, 0, &dogpatch_shot_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_21, 1, NODE_20, RES_K(12), CAP_U(.01))
	MW_DISCRETE_RCFILTER(DOGPATCH_L_SHOT_SND, 1, NODE_21, RES_K(12) + RES_K(68), CAP_U(.0022))

	MW_DISCRETE_OP_AMP_TRIG_VCA(NODE_30, DOGPATCH_RIGHT_SHOT_EN, 0, 0, DOGPATCH_NOISE, 0, &dogpatch_shot_tvca_info)
	MW_DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(12), CAP_U(.01))
	MW_DISCRETE_RCFILTER(DOGPATCH_R_SHOT_SND, 1, NODE_31, RES_K(12) + RES_K(68), CAP_U(.0033))

	MW_DISCRETE_CONSTANT(DOGPATCH_L_HIT_SND, 0)
	MW_DISCRETE_CONSTANT(DOGPATCH_R_HIT_SND, 0)

	MW_DISCRETE_MIXER2(NODE_91, DOGPATCH_GAME_ON_EN, DOGPATCH_L_SHOT_SND, DOGPATCH_L_HIT_SND, &dogpatch_l_mixer)

	MW_DISCRETE_MIXER3(NODE_92, DOGPATCH_GAME_ON_EN, DOGPATCH_R_SHOT_SND, DOGPATCH_R_HIT_SND, MIDWAY_TONE_SND, &dogpatch_r_mixer)

	MW_DISCRETE_OUTPUT(NODE_91, 32760.0 / 5.8)
	MW_DISCRETE_OUTPUT(NODE_92, 32760.0 / 5.8)

MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_dogpatch_audio)
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_dogpatch_discrete_interface)
	MDRV_SOUND_ROUTE(0, "lspeaker", 1.0)
	MDRV_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_dogpatch_audio_w )
{

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);
	discrete_sound_w(DOGPATCH_GAME_ON_EN, (data >> 3) & 0x01);

	discrete_sound_w(DOGPATCH_LEFT_SHOT_EN, (data >> 4) & 0x01);

	discrete_sound_w(DOGPATCH_RIGHT_SHOT_EN, (data >> 5) & 0x01);

	discrete_sound_w(DOGPATCH_HIT_EN, (data >> 6) & 0x01);
}

#define BLUESHRK_OCTOPUS_EN         NODE_01
#define BLUESHRK_HIT_EN             NODE_02
#define BLUESHRK_SHARK_EN           NODE_03
#define BLUESHRK_SHOT_EN            NODE_04
#define BLUESHRK_GAME_ON_EN         NODE_05

#define BLUESHRK_NOISE_1            NODE_11
#define BLUESHRK_NOISE_2            NODE_12
#define BLUESHRK_OCTOPUS_SND        NODE_13
#define BLUESHRK_HIT_SND            NODE_14
#define BLUESHRK_SHARK_SND          NODE_15
#define BLUESHRK_SHOT_SND           NODE_16

#define BLUESHRK_R300   RES_M(1)
#define BLUESHRK_R301   RES_K(100)
#define BLUESHRK_R302   RES_M(1)
#define BLUESHRK_R303   RES_K(33)
#define BLUESHRK_R304   RES_K(120)
#define BLUESHRK_R305   RES_M(1)
#define BLUESHRK_R306   RES_K(470)
#define BLUESHRK_R307   RES_K(680)
#define BLUESHRK_R308   RES_M(1)
#define BLUESHRK_R309   RES_M(1)
#define BLUESHRK_R310   RES_K(680)
#define BLUESHRK_R311   RES_K(1)
#define BLUESHRK_R312   RES_K(100)
#define BLUESHRK_R313   RES_M(1)
#define BLUESHRK_R314   RES_M(1)
#define BLUESHRK_R315   RES_M(4.7)
#define BLUESHRK_R316   RES_M(2.2)
#define BLUESHRK_R317   RES_K(10)
#define BLUESHRK_R318   RES_M(1)
#define BLUESHRK_R319   RES_K(680)
#define BLUESHRK_R320   RES_M(2.7)
#define BLUESHRK_R321   RES_K(680)
#define BLUESHRK_R324   RES_K(750)
#define BLUESHRK_R520   RES_K(510)
#define BLUESHRK_R521   RES_K(22)
#define BLUESHRK_R529   RES_K(33)
#define BLUESHRK_R601   RES_K(47)
#define BLUESHRK_R602   RES_K(22)
#define BLUESHRK_R603   RES_K(39)
#define BLUESHRK_R604   RES_K(1)
#define BLUESHRK_R605   RES_M(1)
#define BLUESHRK_R700   RES_K(68)
#define BLUESHRK_R701   RES_K(470)
#define BLUESHRK_R702   RES_M(1.2)
#define BLUESHRK_R703   RES_M(1.5)
#define BLUESHRK_R704   RES_K(22)
#define BLUESHRK_R705   RES_K(100)
#define BLUESHRK_R706   RES_K(470)
#define BLUESHRK_R707   RES_M(1.2)
#define BLUESHRK_R708   RES_M(1.5)
#define BLUESHRK_R709   RES_K(22)
#define BLUESHRK_R710   RES_K(470)
#define BLUESHRK_R711   RES_K(39)
#define BLUESHRK_R712   RES_M(1.2)
#define BLUESHRK_R713   RES_M(1.5)
#define BLUESHRK_R714   RES_K(22)
#define BLUESHRK_R715   RES_K(47)
#define BLUESHRK_R716   RES_K(75)
#define BLUESHRK_R717   RES_M(1.5)
#define BLUESHRK_R718   RES_M(2.2)
#define BLUESHRK_R719   RES_K(560)
#define BLUESHRK_R720   RES_M(1.5)
#define BLUESHRK_R721   RES_M(2.2)
#define BLUESHRK_R722   RES_M(2.2)
#define BLUESHRK_R723   RES_K(560)
#define BLUESHRK_R724   RES_K(12)
#define BLUESHRK_R725   RES_K(68)
#define BLUESHRK_R726   RES_K(330)
#define BLUESHRK_R727   RES_M(2.2)
#define BLUESHRK_R728   RES_M(1)
#define BLUESHRK_R730   RES_K(56)
#define BLUESHRK_R1000  RES_K(1)

#define BLUESHRK_C300   CAP_U(0.1)
#define BLUESHRK_C301   CAP_P(470)
#define BLUESHRK_C302   CAP_P(470)
#define BLUESHRK_C303   CAP_U(0.47)
#define BLUESHRK_C304   CAP_U(1)
#define BLUESHRK_C305   CAP_U(1)
#define BLUESHRK_C508   CAP_U(1)
#define BLUESHRK_C600   CAP_U(2.2)
#define BLUESHRK_C602   CAP_U(0.022)
#define BLUESHRK_C603   CAP_U(0.01)
#define BLUESHRK_C604   CAP_U(0.015)
#define BLUESHRK_C606   CAP_U(1)
#define BLUESHRK_C700   CAP_U(22)
#define BLUESHRK_C701   CAP_U(22)
#define BLUESHRK_C702   CAP_U(10)
#define BLUESHRK_C703   CAP_U(0.033)
#define BLUESHRK_C704   CAP_U(0.015)
#define BLUESHRK_C705   CAP_U(0.015)
#define BLUESHRK_C706   CAP_U(0.033)
#define BLUESHRK_C707   CAP_U(2.2)
#define BLUESHRK_C708   CAP_U(1)
#define BLUESHRK_C900   CAP_U(10)

static const mw_discrete_op_amp_osc_info blueshrk_octopus_osc =
{
	MW_DISC_OP_AMP_OSCILLATOR_1 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_CAP,
	BLUESHRK_R300, BLUESHRK_R303, BLUESHRK_R301, BLUESHRK_R304, BLUESHRK_R302, 0, 0, 0,
	BLUESHRK_C300, 12
};

static const mw_discrete_op_amp_osc_info blueshrk_octopus_vco =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_1 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW,
	BLUESHRK_R305, BLUESHRK_R306, BLUESHRK_R307, BLUESHRK_R309, BLUESHRK_R308, 0, 0, 0,
	BLUESHRK_C301, 12
};

static const mw_discrete_op_amp_1sht_info blueshrk_octopus_oneshot =
{
	MW_DISC_OP_AMP_1SHT_1 | MW_DISC_OP_AMP_IS_NORTON,
	BLUESHRK_R315, BLUESHRK_R312, BLUESHRK_R314, BLUESHRK_R313, BLUESHRK_R316,
	BLUESHRK_C303, BLUESHRK_C302,
	0, 12
};

static const mw_discrete_integrate_info blueshrk_octopus_integrate =
{
	MW_DISC_INTEGRATE_OP_AMP_1 | MW_DISC_OP_AMP_IS_NORTON,
	BLUESHRK_R318, BLUESHRK_R317, 0, BLUESHRK_C304,
	12, 12,
	0, 0, 0
};

static const mw_discrete_op_amp_info blueshrk_octopus_amp =
{
	MW_DISC_OP_AMP_IS_NORTON,
	BLUESHRK_R310, BLUESHRK_R319, BLUESHRK_R320, BLUESHRK_R321,
	0, 0, 12
};

static const mw_discrete_lfsr_desc blueshrk_lfsr =
{
	MW_DISC_CLK_IS_FREQ,
	17,

	0x1ffff,
	4,
	16,
	MW_DISC_LFSR_XOR,
	MW_DISC_LFSR_OR,
	MW_DISC_LFSR_REPLACE,
	0x000001,
	0,
	12
}; static const mw_discrete_lfsr_desc blueshrk_lfsr_bit8 =
{
	MW_DISC_CLK_IS_FREQ,
	17,

	0x1ffff,
	4,
	16,
	MW_DISC_LFSR_XOR,
	MW_DISC_LFSR_OR,
	MW_DISC_LFSR_REPLACE,
	0x000001,
	0,
	8
};

static const mw_discrete_555_desc blueshrk_555_H1B =
{
	MW_DISC_555_OUT_ENERGY | MW_DISC_555_OUT_DC, 5, 12, MW_DEFAULT_555_THRESHOLD, MW_DEFAULT_555_TRIGGER
};

static const mw_discrete_op_amp_osc_info blueshrk_shark_osc1 =
{
	MW_DISC_OP_AMP_OSCILLATOR_2 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_ENERGY,
	0, BLUESHRK_R701, BLUESHRK_R703, BLUESHRK_R702, 0, BLUESHRK_R700, 0, 0,
	BLUESHRK_C700, 12
};

static const mw_discrete_op_amp_osc_info blueshrk_shark_osc2 =
{
	MW_DISC_OP_AMP_OSCILLATOR_2 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_ENERGY,
	0, BLUESHRK_R706, BLUESHRK_R708, BLUESHRK_R707, 0, BLUESHRK_R705, 0, 0,
	BLUESHRK_C700, 12
};

static const mw_discrete_op_amp_osc_info blueshrk_shark_osc3 =
{
	MW_DISC_OP_AMP_OSCILLATOR_2 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_ENERGY,
	0, BLUESHRK_R711, BLUESHRK_R713, BLUESHRK_R712, 0, BLUESHRK_R710, 0, 0,
	BLUESHRK_C700, 12
};

static const mw_discrete_mixer_desc blueshrk_shark_mixer =
{
	MW_DISC_MIXER_IS_RESISTOR,
	{ BLUESHRK_R704, BLUESHRK_R709, BLUESHRK_R714 },
	{ 0 }, { 0 }, 0, 0, 0, 0, 0, 1
};

static const mw_discrete_op_amp_info blueshrk_shark_amp_m3 =
{
	MW_DISC_OP_AMP_IS_NORTON,
	0, BLUESHRK_R715 + RES_3_PARALLEL(BLUESHRK_R704, BLUESHRK_R709, BLUESHRK_R714), BLUESHRK_R716, 0,
	0, 0, 12
};

static const mw_discrete_op_amp_osc_info blueshrk_shark_vco =
{
	MW_DISC_OP_AMP_OSCILLATOR_VCO_3 | MW_DISC_OP_AMP_IS_NORTON | MW_DISC_OP_AMP_OSCILLATOR_OUT_ENERGY,
	BLUESHRK_R717, BLUESHRK_R722, BLUESHRK_R719, BLUESHRK_R721, BLUESHRK_R720,
	0, 0, BLUESHRK_R718,
	BLUESHRK_C703, 12
};

static const mw_discrete_op_amp_info blueshrk_shark_amp_k3 =
{
	MW_DISC_OP_AMP_IS_NORTON,
	BLUESHRK_R724 + BLUESHRK_R725 + BLUESHRK_R726,
	BLUESHRK_R723, BLUESHRK_R727, BLUESHRK_R728,
	0, 0, 12
};

static const mw_discrete_mixer_desc blueshrk_mixer =
{
	MW_DISC_MIXER_IS_RESISTOR,
	{ BLUESHRK_R324, RES_2_PARALLEL(BLUESHRK_R520, BLUESHRK_R521) + BLUESHRK_R529, BLUESHRK_R604 + BLUESHRK_R605, BLUESHRK_R730 },
	{ 0 },
	{ BLUESHRK_C305, BLUESHRK_C508, BLUESHRK_C606, BLUESHRK_C708 },
	0, 0, 0, BLUESHRK_C900, 0, 1
};

static MW_DISCRETE_SOUND_START(mw_blueshrk)

	MW_DISCRETE_INPUTX_LOGIC(BLUESHRK_OCTOPUS_EN, 12, 0, 0)
	MW_DISCRETE_INPUT_LOGIC(BLUESHRK_HIT_EN)
	MW_DISCRETE_INPUT_LOGIC(BLUESHRK_SHARK_EN)
	MW_DISCRETE_INPUT_LOGIC(BLUESHRK_SHOT_EN)
	MW_DISCRETE_INPUT_LOGIC(BLUESHRK_GAME_ON_EN)

	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_20, 1, &blueshrk_octopus_osc)
	MW_DISCRETE_OP_AMP_VCO1(NODE_21, 1, NODE_20, &blueshrk_octopus_vco)
	MW_DISCRETE_OP_AMP_ONESHOT(NODE_22, BLUESHRK_OCTOPUS_EN, &blueshrk_octopus_oneshot)
	MW_DISCRETE_INTEGRATE(NODE_23, NODE_22, 0, &blueshrk_octopus_integrate)
	MW_DISCRETE_OP_AMP(BLUESHRK_OCTOPUS_SND, 1, NODE_21, NODE_23, &blueshrk_octopus_amp)

	MW_DISCRETE_LFSR_NOISE(BLUESHRK_NOISE_1, 1, 1, 7700, 12.0, 0, 12.0 / 2, &blueshrk_lfsr)
	MW_DISCRETE_LFSR_NOISE(BLUESHRK_NOISE_2, 1, 1, 7700, 12.0, 0, 6, &blueshrk_lfsr_bit8)

	MW_DISCRETE_CONSTANT(BLUESHRK_SHOT_SND, 0)

	MW_DISCRETE_COUNTER(NODE_40, 1, BLUESHRK_HIT_EN, FREQ_OF_555(BLUESHRK_R601, 0, BLUESHRK_C600), 1, MW_DISC_COUNT_UP, 0, MW_DISC_CLK_IS_FREQ)
	MW_DISCRETE_SWITCH(NODE_41, 1, NODE_40, BLUESHRK_C602 + BLUESHRK_C603, BLUESHRK_C604)
	MW_DISCRETE_555_ASTABLE(BLUESHRK_HIT_SND, BLUESHRK_HIT_EN, BLUESHRK_R602, BLUESHRK_R603, NODE_41, &blueshrk_555_H1B)

	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_50, 1, &blueshrk_shark_osc1)
	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_51, 1, &blueshrk_shark_osc2)
	MW_DISCRETE_OP_AMP_OSCILLATOR(NODE_52, 1, &blueshrk_shark_osc3)
	MW_DISCRETE_MIXER3(NODE_53, 1, NODE_50, NODE_51, NODE_52, &blueshrk_shark_mixer)

	MW_DISCRETE_OP_AMP(NODE_54, 1, 0, NODE_53, &blueshrk_shark_amp_m3)
	MW_DISCRETE_ADDER2(NODE_55, 1, NODE_54, -0.7)
	MW_DISCRETE_CLAMP(NODE_56, 1, NODE_55, 0, 12, 0)

	MW_DISCRETE_OP_AMP_VCO1(NODE_57, BLUESHRK_SHARK_EN, NODE_56, &blueshrk_shark_vco)
	MW_DISCRETE_RCFILTER(NODE_58, 1, BLUESHRK_NOISE_1, BLUESHRK_R724, BLUESHRK_C704)
	MW_DISCRETE_RCFILTER(NODE_59, 1, NODE_58, BLUESHRK_R724 + BLUESHRK_R725, BLUESHRK_C704)
	MW_DISCRETE_RCFILTER(NODE_60, 1, NODE_59, BLUESHRK_R724 + BLUESHRK_R725 + BLUESHRK_R726, BLUESHRK_C704)
	MW_DISCRETE_OP_AMP(NODE_61, 1, NODE_60, NODE_57, &blueshrk_shark_amp_k3)

	MW_DISCRETE_RCFILTER(BLUESHRK_SHARK_SND, 1, NODE_61, 1, BLUESHRK_C707)

	MW_DISCRETE_MIXER4(NODE_91, BLUESHRK_GAME_ON_EN, BLUESHRK_OCTOPUS_SND, BLUESHRK_SHOT_SND, BLUESHRK_HIT_SND, BLUESHRK_SHARK_SND, &blueshrk_mixer)

	MW_DISCRETE_OUTPUT(NODE_91, 90000)
MW_DISCRETE_SOUND_END

MACHINE_DRIVER_START(mw_blueshrk_audio)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(mw_blueshrk_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

WRITE8_HANDLER( mw_blueshrk_audio_w )
{
	discrete_sound_w(BLUESHRK_GAME_ON_EN, (data >> 0) & 0x01);

	discrete_sound_w(BLUESHRK_SHOT_EN, (data >> 1) & 0x01);

	discrete_sound_w(BLUESHRK_HIT_EN, (data >> 2) & 0x01);

	discrete_sound_w(BLUESHRK_SHARK_EN, (data >> 3) & 0x01);

	discrete_sound_w(BLUESHRK_OCTOPUS_EN, (data >> 5) & 0x01);

}

// license:BSD-3-Clause
// copyright-holders:Derrick Renaud

#include "driver.h"
#include "copsnrob.h"
#include "sound/discrete.h"

#define RES_2_PARALLEL(a,b) (1.0 / (1.0/(a) + 1.0/(b)))
#define RES_3_PARALLEL(a,b,c) (1.0 / (1.0/(a) + 1.0/(b) + 1.0/(c)))
#define RES_VOLTAGE_DIVIDER(a,b) ((b) / ((a) + (b)))

#define COPSNROB_MOTOR0_INV			NODE_01
#define COPSNROB_MOTOR1_INV			NODE_02
#define COPSNROB_MOTOR2_INV			NODE_03
#define COPSNROB_MOTOR3_INV			NODE_04
#define COPSNROB_CRASH_INV			NODE_07
#define COPSNROB_SCREECH_INV		NODE_08
#define COPSNROB_AUDIO_ENABLE		NODE_09

#define COPSNROB_MOTOR0_SND			NODE_11
#define COPSNROB_MOTOR1_SND			NODE_12
#define COPSNROB_MOTOR2_SND			NODE_13
#define COPSNROB_MOTOR3_SND			NODE_14
#define COPSNROB_FZ_SND				NODE_15
#define COPSNROB_CRASH_SND			NODE_16
#define COPSNROB_SCREECH_SND		NODE_17
#define COPSNROB_NOISE_2			NODE_19

#define COPSNROB_R16			RES_K(10)
#define COPSNROB_R18			RES_K(100)
#define COPSNROB_R19			RES_K(100)
#define COPSNROB_R20			RES_K(100)
#define COPSNROB_R21			RES_K(47)
#define COPSNROB_R25			RES_K(47)
#define COPSNROB_R27			RES_K(22)
#define COPSNROB_R28			RES_K(18)
#define COPSNROB_R33			RES_K(10)
#define COPSNROB_R35			RES_K(10)
#define COPSNROB_R39			RES_K(10)
#define COPSNROB_R41			RES_K(33)
#define COPSNROB_R42			RES_K(33)
#define COPSNROB_R53			RES_K(47)
#define COPSNROB_R56			RES_K(470)
#define COPSNROB_R57			RES_K(820)
#define COPSNROB_R58			RES_K(68)
#define COPSNROB_R64			RES_K(47)
#define COPSNROB_R65			RES_K(4.7)
#define COPSNROB_R69			RES_K(330)
#define COPSNROB_R70			RES_K(330)
#define COPSNROB_R71			RES_K(100)
#define COPSNROB_R72			RES_K(100)
#define COPSNROB_R79			RES_K(2.2)
#define COPSNROB_R80			RES_K(1)
#define COPSNROB_R81			RES_K(8.2)
#define COPSNROB_R82			RES_K(3.9)
#define COPSNROB_R86			RES_K(330)
#define COPSNROB_R87			(820)
#define COPSNROB_R88			RES_K(1)
#define COPSNROB_R89			RES_K(1)
#define COPSNROB_R92			RES_K(100)
#define COPSNROB_R93			RES_K(100)
#define COPSNROB_R94			RES_K(10)

#define COPSNROB_C12			CAP_U(0.1)
#define COPSNROB_C17			CAP_U(0.001)
#define COPSNROB_C20			CAP_U(0.01)
#define COPSNROB_C23			CAP_U(0.1)
#define COPSNROB_C24			CAP_U(0.22)
#define COPSNROB_C33			CAP_U(10)
#define COPSNROB_C36			CAP_U(100)
#define COPSNROB_C42			CAP_U(0.047)
#define COPSNROB_C43			CAP_U(0.047)
#define COPSNROB_C55			CAP_U(0.1)
#define COPSNROB_C58			CAP_U(0.1)
#define COPSNROB_C59			CAP_U(0.1)

#define COPSNROB_2V				(15750/2/2)

static const struct discrete_555_desc copsnrob_motor23_555_1 =
{
	DISC_555_OUT_SQW,
	5, DEFAULT_555_VALUES
};

static const struct discrete_555_desc copsnrob_motor23_555_2 =
{
	DISC_555_OUT_ENERGY | DISC_555_TRIGGER_IS_VOLTAGE,
	5, DEFAULT_555_VALUES
};

static const struct discrete_op_amp_filt_info copsnrob_motor23_filter =
{
	COPSNROB_R28, 0, COPSNROB_R87, 0, COPSNROB_R86,
	COPSNROB_C42, COPSNROB_C43, 0,
	5.0 * RES_VOLTAGE_DIVIDER(COPSNROB_R88, COPSNROB_R89), 5, 0
};

static const struct discrete_dac_r1_ladder copsnrob_crash_dac =
{
	4,
	{COPSNROB_R81, COPSNROB_R82, COPSNROB_R79, COPSNROB_R80},
	0, 0, 0, COPSNROB_C58
};

static const struct discrete_mixer_desc copsnrob_final_mixer01 =
{
	DISC_MIXER_IS_RESISTOR,
	{COPSNROB_R21, COPSNROB_R25, COPSNROB_R33, COPSNROB_R70, COPSNROB_R71},
	{0}, {0}, 0, COPSNROB_R35, 0, COPSNROB_C59,
	0, 1
};

static const struct discrete_dac_r1_ladder copsnrob_motor01_cc_dac =
{
	1,
	{COPSNROB_R56},
	5.0 - 0.5, COPSNROB_R58, COPSNROB_R57, COPSNROB_C33
};

static const struct discrete_dac_r1_ladder copsnrob_motor01_out_dac =
{
	4,
	{COPSNROB_R20, COPSNROB_R18, 0, COPSNROB_R19},
	0, 0, 0, COPSNROB_C12
};

static const struct discrete_mixer_desc copsnrob_final_mixer23 =
{
	DISC_MIXER_IS_RESISTOR,
	{COPSNROB_R93, COPSNROB_R92, COPSNROB_R72, COPSNROB_R69, COPSNROB_R16},
	{0}, {0}, 0, COPSNROB_R94, 0, COPSNROB_C55,
	0, 1
};

static const struct discrete_dac_r1_ladder copsnrob_motor23_cv_dac =
{
	1,
	{COPSNROB_R65},
	5, RES_K(5), RES_K(10), COPSNROB_C36
};

static const struct discrete_555_cc_desc copsnrob_motor01_555cc =
{

	DISC_555_OUT_COUNT_R | DISCRETE_555_CC_TO_DISCHARGE_PIN,
	5, DEFAULT_555_VALUES, 5, 0.6
};

#define COPSNROB_MOTOR01_BASE_NODE		NODE_20
#define COPSNROB_MOTOR23_BASE_NODE		NODE_30
#define COPSNROB_NODE(_base, _num, _offset)		(_base + _num * 100 + _offset)
#define COPSNROB_MOTOR01_NODE(_num, _offset)	COPSNROB_NODE(COPSNROB_MOTOR01_BASE_NODE, _num, _offset)
#define COPSNROB_MOTOR23_NODE(_num, _offset)	COPSNROB_NODE(COPSNROB_MOTOR23_BASE_NODE, _num, _offset)

#define COPSNROB_MOTOR01(_output, _input, _num) \
	DISCRETE_DAC_R1(COPSNROB_MOTOR01_NODE(_num, 0), 1, \
	 	_input, 4.2, &copsnrob_motor01_cc_dac) \
	DISCRETE_555_CC(COPSNROB_MOTOR01_NODE(_num, 1), \
		1, \
		COPSNROB_MOTOR01_NODE(_num, 0), \
		COPSNROB_R53, COPSNROB_C20, 0, 0, COPSNROB_R39, \
		&copsnrob_motor01_555cc) \
	DISCRETE_COUNTER(COPSNROB_MOTOR01_NODE(_num, 2), \
		1, 0, \
		COPSNROB_MOTOR01_NODE(_num, 1), \
		3, 1, 0, DISC_CLK_BY_COUNT) \
	DISCRETE_COUNTER(COPSNROB_MOTOR01_NODE(_num, 3), \
		1, 0, \
		COPSNROB_MOTOR01_NODE(_num, 1), \
		5, 1, 0, DISC_CLK_BY_COUNT | DISC_COUNTER_IS_7492) \
	DISCRETE_TRANSFORM3(COPSNROB_MOTOR01_NODE(_num, 4), 1, \
		COPSNROB_MOTOR01_NODE(_num, 2), \
		COPSNROB_MOTOR01_NODE(_num, 3),	2, \
		"02&2/12*+") \
	DISCRETE_DAC_R1(_output, 1, \
	 	COPSNROB_MOTOR01_NODE(_num, 4), 4.2, &copsnrob_motor01_out_dac)

#define COPSNROB_MOTOR23(_output, _input, _num) \
	DISCRETE_DAC_R1(COPSNROB_MOTOR23_NODE(_num, 0), 1, \
	 	_input, 4.2, &copsnrob_motor23_cv_dac) \
	DISCRETE_555_ASTABLE_CV(COPSNROB_MOTOR23_NODE(_num, 1), \
		1, \
		COPSNROB_R64, COPSNROB_R42, COPSNROB_C24, \
		COPSNROB_MOTOR23_NODE(_num, 0), \
		&copsnrob_motor23_555_1) \
	DISCRETE_CRFILTER_VREF(COPSNROB_MOTOR23_NODE(_num, 2), 1, \
		COPSNROB_MOTOR23_NODE(_num, 1), \
		RES_3_PARALLEL(COPSNROB_R27, RES_K(10), RES_K(5)), COPSNROB_C17, \
		5.0 * RES_VOLTAGE_DIVIDER(RES_2_PARALLEL(COPSNROB_R27, RES_K(10)), RES_K(5))) \
	DISCRETE_555_MSTABLE(COPSNROB_MOTOR23_NODE(_num, 3), \
		1, \
		COPSNROB_MOTOR23_NODE(_num, 2), \
		COPSNROB_R41, COPSNROB_C23, \
		&copsnrob_motor23_555_2) \
	DISCRETE_OP_AMP_FILTER(_output, \
		1, \
		COPSNROB_MOTOR23_NODE(_num, 3), 0, \
		DISC_OP_AMP_FILTER_IS_BAND_PASS_1M, &copsnrob_motor23_filter)

struct copsnrob_noise_context
{
	UINT8 low, high;
	double phase;
};

static void copsnrob_noise_reset(struct node_description *node)
{
	struct copsnrob_noise_context *state = node->context;
	memset(state, 0, sizeof(*state));
	node->output = 0;
}

static void copsnrob_noise_step(struct node_description *node)
{
	struct copsnrob_noise_context *state = node->context;
	double dt = 1.0 / Machine->sample_rate;
	double period = 1.0 / COPSNROB_2V;
	int previous = (state->low >> 5) & 1;
	state->phase += dt;
	if (state->phase >= period)
	{
		int feedback = (((state->low >> 6) ^ state->high) & 1) ^ 1;
		int current;
		state->phase -= period;
		state->low = (state->low << 1) | (state->high >> 7);
		state->high = (state->high << 1) | feedback;
		if (state->high == 0xff)
			state->high = 0;
		current = (state->low >> 5) & 1;
		node->output = 4.2 * (previous + (current - previous) * state->phase / dt);
	}
	else
		node->output = 4.2 * previous;
}

static const struct discrete_module copsnrob_noise_module =
{
	DST_CUSTOM, "Cops'n Robbers noise", sizeof(struct copsnrob_noise_context),
	copsnrob_noise_reset, copsnrob_noise_step
};

DISCRETE_SOUND_START(copsnrob)

	DISCRETE_INPUT_LOGIC(COPSNROB_MOTOR0_INV)
	DISCRETE_INPUT_LOGIC(COPSNROB_MOTOR1_INV)
	DISCRETE_INPUT_LOGIC(COPSNROB_MOTOR2_INV)
	DISCRETE_INPUT_LOGIC(COPSNROB_MOTOR3_INV)

	DISCRETE_INPUT_NOT(COPSNROB_CRASH_INV)
	DISCRETE_INPUT_LOGIC(COPSNROB_SCREECH_INV)
	DISCRETE_INPUT_NOT(COPSNROB_AUDIO_ENABLE)

	COPSNROB_MOTOR01(COPSNROB_MOTOR0_SND, COPSNROB_MOTOR0_INV, 0)
	COPSNROB_MOTOR01(COPSNROB_MOTOR1_SND, COPSNROB_MOTOR1_INV, 1)

	COPSNROB_MOTOR23(COPSNROB_MOTOR2_SND, COPSNROB_MOTOR2_INV, 0)
	COPSNROB_MOTOR23(COPSNROB_MOTOR3_SND, COPSNROB_MOTOR3_INV, 1)

	{ COPSNROB_NOISE_2, DST_CUSTOM, 0, { NODE_NC }, { 0 }, &copsnrob_noise_module, "Cops'n Robbers noise" },

	DISCRETE_COUNTER(NODE_40,
		NODE_41,
		COPSNROB_CRASH_INV,
		92,
		15, 0, 15, DISC_CLK_IS_FREQ)
	DISCRETE_TRANSFORM2(NODE_41, 1,
		NODE_40, 0,	"01=!")
	DISCRETE_SWITCH(NODE_42,
		1, COPSNROB_NOISE_2, 0, NODE_40)
	DISCRETE_DAC_R1(COPSNROB_CRASH_SND, 1,
		NODE_42, 3.8,
		&copsnrob_crash_dac)

	DISCRETE_CONSTANT(COPSNROB_SCREECH_SND, 0)

	DISCRETE_CONSTANT(COPSNROB_FZ_SND, 0)

	DISCRETE_MIXER5(NODE_90,
		COPSNROB_AUDIO_ENABLE,
		COPSNROB_MOTOR1_SND, COPSNROB_MOTOR0_SND, COPSNROB_FZ_SND, COPSNROB_SCREECH_SND, COPSNROB_CRASH_SND,
		&copsnrob_final_mixer01)
	DISCRETE_MIXER5(NODE_91,
		COPSNROB_AUDIO_ENABLE,
		COPSNROB_MOTOR3_SND, COPSNROB_MOTOR2_SND, COPSNROB_CRASH_SND, COPSNROB_SCREECH_SND, COPSNROB_FZ_SND,
		&copsnrob_final_mixer23)
	DISCRETE_OUTPUT(NODE_90, 32767.0*3.5)
	DISCRETE_OUTPUT(NODE_91, 32767.0*3.5)
DISCRETE_SOUND_END


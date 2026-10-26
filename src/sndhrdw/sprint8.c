// license:BSD-3-Clause
// copyright-holders:Derrick Renaud
#include <math.h>
#include "driver.h"
#include "sprint8.h"

#define DEFAULT_TTL_V_LOGIC_1 3.4
#define NODE_RELATIVE(base, offset) ((base) + (offset))
#define RES_2_PARALLEL(a,b) (1.0 / (1.0/(a) + 1.0/(b)))
#define RES_VOLTAGE_DIVIDER(a,b) ((b) / ((a) + (b)))

#define SPRINT8_NOISE				NODE_12
#define SPRINT8_MOTOR1_SND			NODE_13
#define SPRINT8_MOTOR2_SND			NODE_14
#define SPRINT8_MOTOR3_SND			NODE_15
#define SPRINT8_MOTOR4_SND			NODE_16
#define SPRINT8_MOTOR5_SND			NODE_17
#define SPRINT8_MOTOR6_SND			NODE_18
#define SPRINT8_MOTOR7_SND			NODE_19
#define SPRINT8_MOTOR8_SND			NODE_20
#define SPRINT8_CRASH_SCREECH_SND	NODE_21
#define SPRINT8_AUDIO_1_2			NODE_22
#define SPRINT8_AUDIO_3_7			NODE_23
#define SPRINT8_AUDIO_5_6			NODE_24
#define SPRINT8_AUDIO_4_8			NODE_25

#define SPRINT8_R132_POT			NODE_29

#define SPRINT8_R1		RES_K(47)
#define SPRINT8_R3		RES_K(47)
#define SPRINT8_R4		RES_K(47)
#define SPRINT8_R19		RES_K(1)
#define SPRINT8_R20		RES_K(1)
#define SPRINT8_R27		RES_K(18)
#define SPRINT8_R28		820
#define SPRINT8_R29		RES_K(330)
#define SPRINT8_R39		RES_K(120)
#define SPRINT8_R40		RES_K(22)
#define SPRINT8_R41		RES_K(150)
#define SPRINT8_R89		RES_K(22)
#define SPRINT8_R91		RES_K(47)
#define SPRINT8_R93		RES_K(2.2)
#define SPRINT8_R96		RES_K(47)
#define SPRINT8_R97		RES_K(2.2)
#define SPRINT8_R99		RES_K(27)
#define SPRINT8_R100	RES_K(1)
#define SPRINT8_R101	RES_K(2.2)
#define SPRINT8_R132	RES_K(100)
#define SPRINT8_R145	RES_K(3.3)
#define SPRINT8_R146	RES_K(7.5)
#define SPRINT8_R147	100
#define SPRINT8_R148	RES_K(1)
#define SPRINT8_R149	RES_K(22)

#define SPRINT8_C8		CAP_U(.01)
#define SPRINT8_C17		CAP_U(.001)
#define SPRINT8_C18		CAP_U(.047)
#define SPRINT8_C19		CAP_U(.047)
#define SPRINT8_C26		CAP_U(100)
#define SPRINT8_C27		CAP_U(.22)
#define SPRINT8_C28		CAP_U(.1)
#define SPRINT8_C59		CAP_U(.1)
#define SPRINT8_C63		CAP_U(.1)
#define SPRINT8_C64		CAP_U(.1)
#define SPRINT8_C89		CAP_U(.1)
#define SPRINT8_C90		CAP_U(.1)

#define SPRINT8_HSYNC	15750.0
#define SPRINT8_1V	SPRINT8_HSYNC/2
#define SPRINT8_2V	SPRINT8_1V/2

static const struct discrete_lfsr_desc sprint8_lfsr =
{
	DISC_CLK_IS_FREQ,
	16,
	0,
	10,
	15,
	DISC_LFSR_XNOR,
	DISC_LFSR_OR,
	DISC_LFSR_REPLACE,
	0x000001,
	DISC_LFSR_FLAG_RESET_TYPE_L,
	12
};

static const struct discrete_555_desc sprint8_crash_555a_desc =
{
	DISC_555_OUT_ENERGY,
	5, DEFAULT_555_VALUES
};

static const struct discrete_integrate_info sprint8_crash_integrate =
{
	DISC_INTEGRATE_OP_AMP_1,
	SPRINT8_R99, SPRINT8_R97, SPRINT8_R96, SPRINT8_C59,
	5, 5,
	0, 0, 0
};

static const struct discrete_555_desc sprint8_motor_555a_desc =
{
	DISC_555_OUT_COUNT_F_X,
	5, DEFAULT_555_VALUES
};

static const struct discrete_op_amp_filt_info sprint8_motor_filter =
{
	SPRINT8_R27, 0, SPRINT8_R28 + RES_2_PARALLEL(SPRINT8_R19, SPRINT8_R20), 0, SPRINT8_R29,
	SPRINT8_C18, SPRINT8_C19, 0,
	5.0 * RES_VOLTAGE_DIVIDER(SPRINT8_R19, SPRINT8_R20), 5, 0
};

static const struct discrete_mixer_desc sprint8_crash_screech_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	{SPRINT8_R149, SPRINT8_R91},
	{0, NODE_80},
	{0}, 0, 0, SPRINT8_C64, 0, 0, 1
};

static const struct discrete_mixer_desc sprint8_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	{SPRINT8_R1 + SPRINT8_R100, SPRINT8_R3, SPRINT8_R4},
	{0}, {0}, 0, 0, 0, SPRINT8_C8, 0, 1
};

struct sprint8_motor_pulse_context
{
	int active;
	double voltage, exponent;
};

static void sprint8_motor_pulse_reset(struct node_description *node)
{
	struct sprint8_motor_pulse_context *state = node->context;
	memset(state, 0, sizeof(*state));
	state->exponent = 1.0 - exp(-1.0 / (Machine->sample_rate * *node->input[1] * *node->input[2]));
	node->output = 0;
}

static void sprint8_motor_pulse_step(struct node_description *node)
{
	struct sprint8_motor_pulse_context *state = node->context;
	double sample_time = 1.0 / Machine->sample_rate;
	double exponent = state->exponent;
	double fraction = 0;
	int trigger = *node->input[0];
	if (trigger && !state->active)
	{
		fraction = *node->input[0] - trigger;
		if (fraction != 0)
		{
			fraction = 1.0 - fraction;
			exponent = 1.0 - exp(-fraction * sample_time / (*node->input[1] * *node->input[2]));
		}
		state->active = 1;
	}
	if (state->active)
	{
		double next = state->voltage + (5.0 - state->voltage) * exponent;
		if (next >= 10.0/3 && !trigger)
		{
			double overshoot = *node->input[1] * *node->input[2] * log(1.0 / (1.0 - (next - 10.0/3) / (5.0 - state->voltage)));
			fraction = overshoot / sample_time;
			state->active = 0;
			next = 0;
		}
		state->voltage = next;
	}
	if (fraction == 0) fraction = 1;
	node->output = 3.8 * (state->active ? fraction : 1.0 - fraction);
}

static const struct discrete_module sprint8_motor_pulse =
{
	DST_CUSTOM, "Sprint 8 motor pulse", sizeof(struct sprint8_motor_pulse_context),
	sprint8_motor_pulse_reset, sprint8_motor_pulse_step
};

#define SPRINT8_MOTOR_CIRCUIT(_car)                                                         	\
DISCRETE_RCFILTER(NODE_RELATIVE(NODE_30, _car - 1), 1, NODE_RELATIVE(SPRINT8_MOTOR1_EN, _car - 1), SPRINT8_R89, SPRINT8_C26) 	\
DISCRETE_ADDER2(NODE_RELATIVE(NODE_40, _car - 1), 1, NODE_RELATIVE(NODE_30, _car - 1), 0.7)	 				\
DISCRETE_555_ASTABLE_CV(NODE_RELATIVE(NODE_50, _car - 1), 1, SPRINT8_R39, 0, SPRINT8_C27, NODE_RELATIVE(NODE_40, _car - 1), &sprint8_motor_555a_desc) \
{ NODE_RELATIVE(NODE_60, _car - 1), DST_CUSTOM, 3, { NODE_RELATIVE(NODE_50, _car - 1), NODE_NC, NODE_NC }, { NODE_RELATIVE(NODE_50, _car - 1), SPRINT8_R41, SPRINT8_C28 }, &sprint8_motor_pulse, "Sprint 8 motor pulse" },       \
DISCRETE_OP_AMP_FILTER(NODE_RELATIVE(SPRINT8_MOTOR1_SND, _car - 1), 1, NODE_RELATIVE(NODE_60, _car - 1), 0, DISC_OP_AMP_FILTER_IS_BAND_PASS_1M, &sprint8_motor_filter)

DISCRETE_SOUND_START( sprint8_discrete )

	DISCRETE_INPUT_LOGIC(SPRINT8_CRASH_EN)
	DISCRETE_INPUT_NOT  (SPRINT8_SCREECH_EN)
	DISCRETE_INPUT_NOT  (SPRINT8_ATTRACT_EN)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR1_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR2_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR3_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR4_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR5_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR6_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR7_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_INPUTX_LOGIC(SPRINT8_MOTOR8_EN, DEFAULT_TTL_V_LOGIC_1, 0, 0)
	DISCRETE_ADJUSTMENT(SPRINT8_R132_POT, 1, 0, SPRINT8_R132, DISC_LINADJ, 19)

	DISCRETE_LFSR_NOISE(SPRINT8_NOISE,
		1,
		SPRINT8_ATTRACT_EN,
		SPRINT8_2V, 1, 0, 0.5, &sprint8_lfsr)

	DISCRETE_GAIN(NODE_70, SPRINT8_NOISE, DEFAULT_TTL_V_LOGIC_1 * RES_VOLTAGE_DIVIDER(SPRINT8_R148, SPRINT8_R147))
	DISCRETE_CRFILTER_VREF(NODE_71,
		1,
		NODE_70,
		RES_2_PARALLEL(SPRINT8_R148, SPRINT8_R147) + RES_2_PARALLEL(RES_K(5), RES_K(10)),
		SPRINT8_C90,
		5.0 * RES_VOLTAGE_DIVIDER(RES_K(5), RES_K(10)))
	DISCRETE_555_ASTABLE_CV(NODE_72,
		SPRINT8_SCREECH_EN,
		SPRINT8_R145, SPRINT8_R146, SPRINT8_C89,
		NODE_71,
		&sprint8_crash_555a_desc)
	DISCRETE_INTEGRATE(NODE_73, SPRINT8_CRASH_EN, 0, &sprint8_crash_integrate)

	DISCRETE_SWITCH(NODE_80,
		1,
		SPRINT8_NOISE,
		SPRINT8_R93, 1)
	DISCRETE_SWITCH(NODE_74,
		1,
		SPRINT8_NOISE,
		NODE_73, 0)
	DISCRETE_MIXER2(NODE_75,
		1,
		NODE_72,
		NODE_74,
		&sprint8_crash_screech_mixer)

	DISCRETE_CRFILTER_VREF(NODE_76,
		1,
		NODE_75,
		SPRINT8_R93 + SPRINT8_R91, SPRINT8_C63,
		5)

	DISCRETE_TRANSFORM5(NODE_77, 1, NODE_76, 5, SPRINT8_R132_POT, SPRINT8_R101, 1, "01-23/4+*1+")
	DISCRETE_CLAMP(SPRINT8_CRASH_SCREECH_SND, 1, NODE_77, 0, 15.0 - 1.5, 0)

	SPRINT8_MOTOR_CIRCUIT(1)
	SPRINT8_MOTOR_CIRCUIT(2)


	SPRINT8_MOTOR_CIRCUIT(3)
	SPRINT8_MOTOR_CIRCUIT(7)


	SPRINT8_MOTOR_CIRCUIT(5)
	SPRINT8_MOTOR_CIRCUIT(6)


	SPRINT8_MOTOR_CIRCUIT(4)
	SPRINT8_MOTOR_CIRCUIT(8)


	DISCRETE_MIXER3(SPRINT8_AUDIO_1_2,
		SPRINT8_ATTRACT_EN,
		SPRINT8_CRASH_SCREECH_SND,
		SPRINT8_MOTOR1_SND,
		SPRINT8_MOTOR2_SND,
		&sprint8_mixer)
	DISCRETE_MIXER3(SPRINT8_AUDIO_3_7,
		SPRINT8_ATTRACT_EN,
		SPRINT8_CRASH_SCREECH_SND,
		SPRINT8_MOTOR3_SND,
		SPRINT8_MOTOR7_SND,
		&sprint8_mixer)
	DISCRETE_MIXER3(SPRINT8_AUDIO_5_6,
		SPRINT8_ATTRACT_EN,
		SPRINT8_CRASH_SCREECH_SND,
		SPRINT8_MOTOR5_SND,
		SPRINT8_MOTOR6_SND,
		&sprint8_mixer)
	DISCRETE_MIXER3(SPRINT8_AUDIO_4_8,
		SPRINT8_ATTRACT_EN,
		SPRINT8_CRASH_SCREECH_SND,
		SPRINT8_MOTOR4_SND,
		SPRINT8_MOTOR8_SND,
		&sprint8_mixer)
	DISCRETE_OUTPUT(SPRINT8_AUDIO_1_2, 65500.0/8)
	DISCRETE_OUTPUT(SPRINT8_AUDIO_3_7, 65500.0/8)
	DISCRETE_OUTPUT(SPRINT8_AUDIO_5_6, 65500.0/8)
	DISCRETE_OUTPUT(SPRINT8_AUDIO_4_8, 65500.0/8)
DISCRETE_SOUND_END


// license:BSD-3-Clause
// copyright-holders:Stefan Jokisch, Ryan Holtz

#include <math.h>
#include "driver.h"
#include "destroyr.h"

struct destroyr_noise_context
{
	UINT16 shift;
	double hphase, vphase;
	int clock;
};

static void destroyr_noise_reset(struct node_description *node)
{
	memset(node->context, 0, sizeof(struct destroyr_noise_context));
	node->output = 0;
}

static void destroyr_noise_step(struct node_description *node)
{
	struct destroyr_noise_context *state = node->context;
	double dt = 1.0 / (8 * Machine->sample_rate);
	double result = 0;
	int i;
	for (i = 0; i < 8; i++)
	{
		int clock;
		state->hphase += 15750 * dt;
		state->vphase += 984.375 * dt;
		state->hphase -= floor(state->hphase);
		state->vphase -= floor(state->vphase);
		clock = (state->hphase < 0.5 && !*node->input[1]) ||
			(state->vphase < 0.5 && !*node->input[2]);
		if (!*node->input[0])
			state->shift = 0;
		else if (clock && !state->clock)
		{
			int feedback = ((state->shift >> 13) ^ (state->shift >> 14) ^ 1) & 1;
			state->shift = (state->shift << 1) | feedback;
		}
		state->clock = clock;
		result += ((state->shift >> 13) & 1) * 5.0 / 8;
	}
	node->output = result;
}

static const struct discrete_module destroyr_noise_module =
{
	DST_CUSTOM, "Destroyer noise", sizeof(struct destroyr_noise_context),
	destroyr_noise_reset, destroyr_noise_step
};

static const struct discrete_555_desc motor_555 =
{
	DISC_555_OUT_CAP, 5, DEFAULT_555_VALUES
};

/* The motor VCA and sonar feedback use simplified filter stages. */
static const struct discrete_mixer_desc destroyr_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	{ RES_K(68), RES_K(33), RES_K(22), RES_K(47), RES_K(5.6) },
	{ 0 }, { CAP_U(0.22), CAP_U(0.22), CAP_U(0.22), CAP_U(0.22), CAP_U(0.22) },
	0, RES_K(25), 0, 0, 0, 1
};

DISCRETE_SOUND_START(destroyr_sound)
	DISCRETE_INPUT_NOT(DESTROYR_ATTRACT)
	DISCRETE_INPUT_LOGIC(DESTROYR_NOISE)
	DISCRETE_INPUTX_LOGIC(DESTROYR_MOTOR_SPEED, 5.0 * 220 / (820 + 220), 5.0 * 820 / (820 + 220), 0)
	DISCRETE_INPUT_LOGIC(DESTROYR_SONGATE)
	DISCRETE_INPUT_LOGIC(DESTROYR_LAUNCH)
	DISCRETE_INPUT_LOGIC(DESTROYR_EXPLO)
	DISCRETE_INPUT_LOGIC(DESTROYR_SONLAT)
	DISCRETE_INPUT_LOGIC(DESTROYR_HE)
	DISCRETE_INPUT_LOGIC(DESTROYR_LE)

	DISCRETE_RCFILTER(NODE_10, 1, DESTROYR_MOTOR_SPEED, 220.0 * 820 / (220 + 820), CAP_U(22))
	DISCRETE_555_ASTABLE_CV(NODE_11, 1, RES_K(2.2), RES_K(39), CAP_U(0.1), NODE_10, &motor_555)
	DISCRETE_555_ASTABLE_CV(NODE_12, 1, RES_K(10), RES_K(150), CAP_U(0.1), NODE_10, &motor_555)
	DISCRETE_CRFILTER(NODE_13, 1, NODE_11, RES_K(68), CAP_U(0.1))
	DISCRETE_MULTIPLY(NODE_14, 1, NODE_13, NODE_12)
	DISCRETE_GAIN(NODE_15, NODE_14, 0.25)

	{ NODE_20, DST_CUSTOM, 3,
		{ DESTROYR_NOISE, DESTROYR_EXPLO, DESTROYR_SONLAT },
		{ 0, 0, 0 }, &destroyr_noise_module, "Destroyer noise" },
	DISCRETE_MULTIPLY(NODE_21, 1, NODE_20, DESTROYR_LAUNCH)
	DISCRETE_FILTER2(NODE_22, 1, NODE_21, 1.0 / (2 * M_PI * RES_K(3.9) * CAP_U(0.022)), 2, DISC_FILTER_BANDPASS)

	DISCRETE_LOGIC_NOR(NODE_30, 1, DESTROYR_EXPLO, DESTROYR_SONLAT)
	DISCRETE_RCDISC2(NODE_31, NODE_30, 0, RES_K(100), 5, 220, CAP_U(10))
	DISCRETE_TRANSFORM2(NODE_32, 1, NODE_20, 5, "10-")
	DISCRETE_LOGIC_INVERT(NODE_37, 1, NODE_30)
	DISCRETE_MULTIPLY(NODE_33, 1, NODE_32, NODE_31)
	DISCRETE_GAIN(NODE_38, NODE_33, 0.2)
	DISCRETE_MULTIPLY(NODE_39, 1, NODE_38, NODE_37)
	DISCRETE_RCFILTER(NODE_34, DESTROYR_LE, NODE_39, RES_K(3.3), CAP_U(0.1))
	DISCRETE_FILTER2(NODE_35, DESTROYR_HE, NODE_39, 1.0 / (2 * M_PI * RES_K(330) * CAP_U(0.1)), 2, DISC_FILTER_BANDPASS)
	DISCRETE_FILTER2(NODE_36, DESTROYR_SONGATE, NODE_39, 1.0 / (2 * M_PI * RES_K(470) * CAP_P(330)), 0.1, DISC_FILTER_BANDPASS)

	DISCRETE_MIXER5(NODE_90, DESTROYR_ATTRACT, NODE_15, NODE_22, NODE_34, NODE_35, NODE_36, &destroyr_mixer)
	DISCRETE_OUTPUT(NODE_90, 8000)
DISCRETE_SOUND_END

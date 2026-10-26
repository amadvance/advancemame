// license:BSD-3-Clause
// copyright-holders:Derrick Renaud
#include <math.h>
#include "driver.h"

#include "starshp1.h"

#define STAR_INPUT(n) (*node->input[n])
#define DEFAULT_TTL_V_LOGIC_1 3.4
#define RES_2_PARALLEL(a,b) (1.0 / (1.0/(a) + 1.0/(b)))
#define RES_VOLTAGE_DIVIDER(a,b) ((b) / ((a) + (b)))

#define STARSHP1_PIXEL_CLOCK       (12096000)
#define STARSHP1_HTOTAL            (0x300)

#define MC1408_DAC(v_ref, r, rf)        ((double)v_ref / (double)r * (double)rf / 256.0)

#define OP_AMP_NON_INVERT_GAIN(ri, rf)  ((double)rf / (double)ri + 1)

#define STARSHP1_PHASOR_SND     NODE_13
#define STARSHP1_NOISE_SND      NODE_14
#define STARSHP1_MOTOR_SND      NODE_15
#define STARSHP1_SL1_SND        NODE_16
#define STARSHP1_SL2_SND        NODE_17

#define STARSHP1_HSYNC  (STARSHP1_PIXEL_CLOCK / STARSHP1_HTOTAL)
#define STARSHP1_S1V    STARSHP1_HSYNC / 2
#define STARSHP1_S128V  STARSHP1_S1V / 128

#define STARSHP1_R4     RES_K(1)
#define STARSHP1_R54    RES_K(47)
#define STARSHP1_R55    RES_K(22)
#define STARSHP1_R58    RES_K(1)
#define STARSHP1_R59    RES_K(1)
#define STARSHP1_R60    RES_K(6.8)
#define STARSHP1_R61    RES_K(10)
#define STARSHP1_R62    750
#define STARSHP1_R63    RES_K(22)
#define STARSHP1_R64    RES_K(10)
#define STARSHP1_R65    RES_K(3.3)
#define STARSHP1_R66    RES_K(33)
#define STARSHP1_R67    RES_K(1)
#define STARSHP1_R68    RES_K(3.3)
#define STARSHP1_R69    RES_K(330)
#define STARSHP1_R70    RES_K(10)
#define STARSHP1_R71    RES_K(6.8)
#define STARSHP1_R72    RES_K(47)
#define STARSHP1_R74    RES_K(100)
#define STARSHP1_R75    RES_K(47)
#define STARSHP1_R76    RES_K(5)
#define STARSHP1_R123   RES_K(1)
#define STARSHP1_R124   RES_K(1)
#define STARSHP1_R126   RES_K(1)
#define STARSHP1_R151   RES_K(10)
#define STARSHP1_R152   RES_K(1.5)
#define STARSHP1_C34    CAP_U(0.1)
#define STARSHP1_C36    CAP_U(2.2)
#define STARSHP1_C45    CAP_U(0.1)
#define STARSHP1_C46    CAP_U(0.1)
#define STARSHP1_C47    CAP_U(0.015)
#define STARSHP1_C59    CAP_U(0.22)
#define STARSHP1_C61    CAP_U(0.01)
#define STARSHP1_C63    CAP_U(0.1)
#define STARSHP1_C78    CAP_U(0.1)

#define STARSHP1_MC1408_GAIN        MC1408_DAC(5.0, STARSHP1_R4, STARSHP1_R62)

#define STARSHP1_TONE_PITCH_GAIN    OP_AMP_NON_INVERT_GAIN(STARSHP1_R63, STARSHP1_R64)

#define STARSHP1_NOISE_AMPLITUDE_GAIN   RES_VOLTAGE_DIVIDER(STARSHP1_R124, STARSHP1_R123)

#define STARSHP1_FAKE_MC3340_COMPENSATION_R     RES_K(6.2)

static const struct discrete_lfsr_desc starshp1_lfsr={
	DISC_CLK_IS_FREQ,
	16,
	0,
	13,
	14,
	DISC_LFSR_XNOR,
	DISC_LFSR_OR,
	DISC_LFSR_REPLACE,
	0x000001,
	0,
	15
};

static const struct discrete_555_desc starshp1_556_c10 =
{
	DISC_555_OUT_ENERGY,
	5,
	DEFAULT_555_VALUES
};

static const struct discrete_mixer_desc starshp1_556_c10_cv =
{
	DISC_MIXER_IS_RESISTOR,
	{STARSHP1_R61, RES_K(5)},
	{0}, {0}, 0, RES_K(10), 0, 0, 0, 1
};

static const struct discrete_mixer_desc starshp1_566_a9_mix_r =
{
	DISC_MIXER_IS_RESISTOR,
	{STARSHP1_R54, STARSHP1_R55},
	{0}, {0}, 0, 0, 0, 0, 0, 1
};

static const struct discrete_mixer_desc starshp1_555_b10_mix_r =
{
	DISC_MIXER_IS_RESISTOR,
	{STARSHP1_R65, STARSHP1_R68},
	{0}, {0}, 0, 0, 0, 0, 0, 1
};

static const struct discrete_mixer_desc starshp1_final_mix =
{
	DISC_MIXER_IS_RESISTOR,
	{STARSHP1_R66, STARSHP1_R126, STARSHP1_R59 + STARSHP1_R60, STARSHP1_R74, STARSHP1_R75},
	{0},
	{0, STARSHP1_C78, 0, 0, 0},
	0,
	STARSHP1_R76,
	0,
	STARSHP1_C63,
	0,
	1
};

struct dsd_566_context
{
	int flip_flop;
	double cap_voltage, threshold_low, threshold_high;
	double v_osc_stable, v_osc_stop;
};

#define DSD_566__VMOD		STAR_INPUT(0)
#define DSD_566__R			STAR_INPUT(1)
#define DSD_566__C			STAR_INPUT(2)
#define DSD_566__VPOS		STAR_INPUT(3)
#define DSD_566__VNEG		STAR_INPUT(4)
#define DSD_566__VCHARGE	STAR_INPUT(5)

static const struct
{
	double	c_high[6];
	double	c_low[6];
	double	osc_stable[6];
	double	osc_stop[6];
} ne566 =
{

	{3.364,  4.259,  4.888, 5.384, 5.896, 6.416},
	{1.940,  2.276,  2.580, 2.880, 3.180, 3.488},

	{4.885,  5.772,  6.335, 6.912, 7.492, 7.945},
	{4.495,  5.343,  5.997, 6.507, 7.016, 7.518}
};

static void starshp1_566_step(struct node_description *node)
{
	struct dsd_566_context *context = node->context;
	double  i = 0;
	double  i_rise;
	double  dt;
	double  v_cap;

	dt    = (1.0 / Machine->sample_rate);
	v_cap = context->cap_voltage;

	if ((DSD_566__VMOD > context->v_osc_stop))
	{
		double v_charge = DSD_566__VCHARGE - DSD_566__VMOD - 0.1;
		if (v_charge > 0)
		{
			i = (v_charge * .95) / DSD_566__R;
			if (DSD_566__VMOD < context->v_osc_stable)
			{

				i_rise = ((DSD_566__VCHARGE - context->v_osc_stable - 0.1) * .95) / DSD_566__R;
				i_rise *= 1.0 - (context->v_osc_stable - DSD_566__VMOD) / (context->v_osc_stable - context->v_osc_stop);
			}
			else
				i_rise = i;
		}
		else
			return;
	}
	else return;

	do
	{
		if (context->flip_flop)
		{

			v_cap -= i * dt / DSD_566__C;
			dt     = 0;

			if ((v_cap < context->threshold_low))
			{

				dt = DSD_566__C * (context->threshold_low - v_cap) / i;
				v_cap = context->threshold_low;
				context->flip_flop = 0;
			}
		}
		else
		{

			v_cap += i_rise * dt / DSD_566__C;
			dt     = 0;

			if ((v_cap > DSD_566__VMOD)) v_cap = DSD_566__VMOD;

			if ((v_cap > context->threshold_high))
			{

				dt = DSD_566__C * (v_cap - context->threshold_high) / i;
				v_cap = context->threshold_high;
				context->flip_flop = 1;

			}
		}
	} while(dt);

	context->cap_voltage = v_cap;
	node->output = v_cap + 1.35;
}

static void starshp1_566_reset(struct node_description *node)
{
	struct dsd_566_context *context = node->context;
	double supply = DSD_566__VPOS - DSD_566__VNEG;
	int index = (int)supply - 10;
	if (index < 0 || index > 5 || supply != floor(supply))
		fatalerror("Invalid Starship 1 oscillator supply");
	memset(context, 0, sizeof(*context));
	context->threshold_high = ne566.c_high[index] + DSD_566__VNEG;
	context->threshold_low = ne566.c_low[index] + DSD_566__VNEG;
	context->v_osc_stable = ne566.osc_stable[index] + DSD_566__VNEG;
	context->v_osc_stop = ne566.osc_stop[index] + DSD_566__VNEG;
	node->output = 0;
}

static const struct discrete_module starshp1_566_module =
{ DST_CUSTOM, "Starship 1 566", sizeof(struct dsd_566_context), starshp1_566_reset, starshp1_566_step };

struct starshp1_filter_context { double voltage; };
static void starshp1_filter_reset(struct node_description *node)
{
	struct starshp1_filter_context *state = node->context;
	state->voltage = 0;
	node->output = *node->input[0];
}
static void starshp1_filter_step(struct node_description *node)
{
	struct starshp1_filter_context *state = node->context;
	double exponent = 1.0 - exp(-1.0 / (Machine->sample_rate * *node->input[1] * *node->input[2]));
	node->output = *node->input[0] - state->voltage;
	state->voltage += (*node->input[0] - state->voltage) * exponent;
}
static const struct discrete_module starshp1_filter_module =
{ DST_CUSTOM, "Starship motor coupling", sizeof(struct starshp1_filter_context), starshp1_filter_reset, starshp1_filter_step };

struct starshp1_tone_context { double voltage; int high; };
static void starshp1_tone_reset(struct node_description *node)
{
	struct starshp1_tone_context *state = node->context;
	state->voltage = 0;
	state->high = 1;
	node->output = 0;
}
static void starshp1_tone_step(struct node_description *node)
{
	struct starshp1_tone_context *state = node->context;
	double dt = 1.0 / Machine->sample_rate;
	double high_time = 0;
	while (dt > 0)
	{
		double target = state->high ? *node->input[0] : 0;
		double threshold = state->high ? 10.0/3 : 5.0/3;
		double rc = (state->high ? *node->input[1] + *node->input[2] : *node->input[2]) * *node->input[3];
		double next = target + (state->voltage - target) * exp(-dt / rc);
		if ((state->high && next > threshold) || (!state->high && next < threshold))
		{
			double elapsed = rc * log((state->voltage - target) / (threshold - target));
			if (elapsed < 0) elapsed = 0;
			if (elapsed > dt) elapsed = dt;
			if (state->high) high_time += elapsed;
			state->voltage = threshold;
			state->high = !state->high;
			dt -= elapsed;
		}
		else
		{
			if (state->high) high_time += dt;
			state->voltage = next;
			dt = 0;
		}
	}
	node->output = 3.4 * high_time * Machine->sample_rate;
}
static const struct discrete_module starshp1_tone_module =
{ DST_CUSTOM, "Starship tone oscillator", sizeof(struct starshp1_tone_context), starshp1_tone_reset, starshp1_tone_step };
DISCRETE_SOUND_START( starshp1_discrete )

	DISCRETE_INPUTX_DATA(STARSHP1_NOISE_AMPLITUDE, -4.0/255, 4, 0)
	DISCRETE_INPUTX_DATA(STARSHP1_TONE_PITCH,      STARSHP1_MC1408_GAIN * STARSHP1_TONE_PITCH_GAIN, 0, 0)
	DISCRETE_INPUTX_DATA(STARSHP1_MOTOR_SPEED,     STARSHP1_MC1408_GAIN, 0, 0)
	DISCRETE_INPUT_LOGIC(STARSHP1_NOISE_FREQ)
	DISCRETE_INPUT_LOGIC(STARSHP1_SL2)
	DISCRETE_INPUT_LOGIC(STARSHP1_SL1)
	DISCRETE_INPUT_LOGIC(STARSHP1_KICKER)
	DISCRETE_INPUT_LOGIC(STARSHP1_PHASOR_ON)
	DISCRETE_INPUT_NOT  (STARSHP1_MOLVL)
	DISCRETE_INPUT_NOT  (STARSHP1_ATTRACT)

	DISCRETE_LFSR_NOISE(NODE_20,
						1,
						STARSHP1_KICKER,
						STARSHP1_S1V,
						1,
						0,
						1.0 / 2,
						&starshp1_lfsr)

	DISCRETE_SWITCH(    NODE_21,
						NODE_20,
						STARSHP1_NOISE_FREQ,
						DEFAULT_TTL_V_LOGIC_1 * RES_VOLTAGE_DIVIDER(STARSHP1_R151, STARSHP1_R152),
						DEFAULT_TTL_V_LOGIC_1)

	DISCRETE_MULTIPLY(  NODE_22, 1,
						NODE_21,
						STARSHP1_NOISE_AMPLITUDE)
	DISCRETE_RCFILTER(  NODE_23, 1,
						NODE_22,
						STARSHP1_FAKE_MC3340_COMPENSATION_R,
						STARSHP1_C61)

	DISCRETE_RCFILTER(  NODE_24, 1,
						NODE_22,
						STARSHP1_FAKE_MC3340_COMPENSATION_R,
						STARSHP1_C61 + STARSHP1_C59)
	DISCRETE_SWITCH(    STARSHP1_NOISE_SND,
						1,
						STARSHP1_NOISE_FREQ,
						NODE_23,
						NODE_24)

	DISCRETE_555_ASTABLE(NODE_30,
						STARSHP1_PHASOR_ON,
						STARSHP1_R70,
						STARSHP1_R69,
						STARSHP1_C46,
						&starshp1_556_c10)

	DISCRETE_MIXER2(    NODE_31,
						1,
						NODE_30,
						5,
						&starshp1_556_c10_cv)
	DISCRETE_555_ASTABLE_CV(STARSHP1_PHASOR_SND,
						STARSHP1_PHASOR_ON,
						STARSHP1_R68,
						STARSHP1_R67,
						STARSHP1_C45,
						NODE_31,
						&starshp1_556_c10)

	DISCRETE_SQUAREWFIX(NODE_40,
						1,
						STARSHP1_S128V,
						DEFAULT_TTL_V_LOGIC_1,
						50,
						DEFAULT_TTL_V_LOGIC_1 / 2,
						0)
	DISCRETE_MIXER2(    NODE_41,
						1,
						NODE_40,
						5,
						&starshp1_566_a9_mix_r)
	{ NODE_42, DST_CUSTOM, 6, { STARSHP1_MOTOR_SPEED, NODE_NC, NODE_NC, NODE_NC, NODE_NC, NODE_41 },
		{ STARSHP1_MOTOR_SPEED, RES_2_PARALLEL(STARSHP1_R54, STARSHP1_R55), STARSHP1_C34, 5, -10, NODE_41 }, &starshp1_566_module, "Starship motor VCO" },
	DISCRETE_SWITCH(NODE_43,
						1,
						STARSHP1_MOLVL,
						STARSHP1_R59 + RES_2_PARALLEL(STARSHP1_R58, STARSHP1_R60 + STARSHP1_R76),
						STARSHP1_R59 + STARSHP1_R60 + STARSHP1_R76)
	{ NODE_44, DST_CUSTOM, 3, { NODE_42, NODE_43, NODE_NC }, { NODE_42, NODE_43, STARSHP1_C36 }, &starshp1_filter_module, "Starship motor coupling" },
	DISCRETE_SWITCH(NODE_45,
						1,
						STARSHP1_MOLVL,
						RES_VOLTAGE_DIVIDER(STARSHP1_R59, STARSHP1_R58), 1)
	DISCRETE_MULTIPLY(  STARSHP1_MOTOR_SND, 1,
						NODE_44,
						NODE_45)

	DISCRETE_MIXER2(    NODE_50,
						1,
						STARSHP1_TONE_PITCH,
						10,
						&starshp1_555_b10_mix_r)
	{ NODE_51, DST_CUSTOM, 4, { NODE_50, NODE_NC, NODE_NC, NODE_NC },
		{ NODE_50, RES_2_PARALLEL(STARSHP1_R65, STARSHP1_R71), STARSHP1_R72, STARSHP1_C47 }, &starshp1_tone_module, "Starship tone oscillator" },

	DISCRETE_ONOFF(     STARSHP1_SL1_SND,
						STARSHP1_SL1, NODE_51)
	DISCRETE_ONOFF(     STARSHP1_SL2_SND,
						STARSHP1_SL2, NODE_51)

	DISCRETE_MIXER5(    NODE_90,
						STARSHP1_ATTRACT,
						STARSHP1_PHASOR_SND,
						STARSHP1_NOISE_SND,
						STARSHP1_MOTOR_SND,
						STARSHP1_SL1_SND,
						STARSHP1_SL2_SND,
						&starshp1_final_mix)

	DISCRETE_OUTPUT(NODE_90, 110000)

DISCRETE_SOUND_END

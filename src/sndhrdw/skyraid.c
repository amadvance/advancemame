// license:BSD-3-Clause
// copyright-holders:Derrick Renaud
#include <math.h>
#include "driver.h"
#include "skyraid.h"

#define SKY_INPUT(n) (*node->input[n])
#define DEFAULT_TTL_V_LOGIC_1 3.4
#define RES_VOLTAGE_DIVIDER(a,b) ((b) / ((a) + (b)))
#define RES_2_PARALLEL(a,b) (1.0 / (1.0/(a) + 1.0/(b)))
#define RES_5_PARALLEL(a,b,c,d,e) (1.0 / (1.0/(a) + 1.0/(b) + 1.0/(c) + 1.0/(d) + 1.0/(e)))
#define RES_6_PARALLEL(a,b,c,d,e,f) (1.0 / (1.0/(a) + 1.0/(b) + 1.0/(c) + 1.0/(d) + 1.0/(e) + 1.0/(f)))
#define RC_CHARGE_EXP(rc) (1.0 - exp(-1.0 / (Machine->sample_rate * (rc))))

#define SKYRAID_NOISE				NODE_10
#define SKYRAID_EXPLOSION_SND		NODE_11
#define SKYRAID_JET_A_SND			NODE_12
#define SKYRAID_JET_B_SND			NODE_13
#define SKYRAID_MSL_A_SND			NODE_14
#define SKYRAID_MSL_B_SND			NODE_15
#define SKYRAID_PLANE_SND			NODE_16

#define SKYRAID_R12		RES_K(1)
#define SKYRAID_R13		RES_K(10)
#define SKYRAID_R14		RES_K(100)
#define SKYRAID_R16		RES_K(10)
#define SKYRAID_R18		RES_K(10)
#define SKYRAID_R19		RES_K(10)
#define SKYRAID_R20		RES_K(47)
#define SKYRAID_R24		RES_M(10)
#define SKYRAID_R25		RES_M(3.9)
#define SKYRAID_R27		RES_M(1.5)
#define SKYRAID_R28		RES_M(1.5)
#define SKYRAID_R29		RES_M(4.7)
#define SKYRAID_R30		RES_M(1)
#define SKYRAID_R31		RES_M(1)
#define SKYRAID_R32		RES_M(4.7)
#define SKYRAID_R84		RES_K(22)
#define SKYRAID_R85		RES_K(100)
#define SKYRAID_R86		820
#define SKYRAID_R110	RES_K(330)
#define SKYRAID_R118	RES_K(470)
#define SKYRAID_R119	RES_K(10)
#define SKYRAID_R120	RES_K(220)
#define SKYRAID_R121	RES_K(1)
#define SKYRAID_R122	RES_K(150)

#define SKYRAID_C44		CAP_U(4.7)
#define SKYRAID_C45		CAP_U(.0047)
#define SKYRAID_C46		CAP_U(.001)
#define SKYRAID_C48		CAP_U(.0047)
#define SKYRAID_C49		CAP_U(.1)
#define SKYRAID_C50		CAP_U(.001)
#define SKYRAID_C51		CAP_U(.01)
#define SKYRAID_C68		CAP_U(10)
#define SKYRAID_C85		CAP_U(.068)
#define SKYRAID_C86		CAP_U(.1)
#define SKYRAID_C93		CAP_U(.1)

static const struct discrete_lfsr_desc skyraid_lfsr =
{
	DISC_CLK_IS_FREQ,
	16,
	0,
	0,
	14,
	DISC_LFSR_XNOR,
	DISC_LFSR_OR,
	DISC_LFSR_REPLACE,
	0x000001,
	DISC_LFSR_FLAG_RESET_TYPE_H,
	15
};

static const struct discrete_op_amp_filt_info skyraid_explosion_filter =
{
	SKYRAID_R85, 0, SKYRAID_R86, 0, SKYRAID_R110,
	SKYRAID_C85, SKYRAID_C86, 0,
	0, 12, -5,
};

static const struct discrete_dac_r1_ladder skyraid_plane_dac =
{
	2, {SKYRAID_R28, SKYRAID_R27},
	0, 0, 0, 0
};

static const struct discrete_mixer_desc skyraid_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	{SKYRAID_R120, SKYRAID_R32, SKYRAID_R29, SKYRAID_R30, SKYRAID_R31, RES_2_PARALLEL(SKYRAID_R28, SKYRAID_R27)},
	{0, 0, 0, 0, 0, SKYRAID_PLANE_ON__IC_E8},
	{0}, 0, 0, 0, 0, 0, 1
};

#define SKYRAID_EXPLOSION_CUSTOM_IN1		SKY_INPUT(0)
#define SKYRAID_EXPLOSION_CUSTOM_IN2		SKY_INPUT(1)
#define SKYRAID_EXPLOSION_CUSTOM_R			SKY_INPUT(2)
#define SKYRAID_EXPLOSION_CUSTOM_C			SKY_INPUT(3)

#define SKYRAID_4066_R_ON	270

struct skyraid_explosion_custom_charge_context
{
	double v_cap;
	double v_charge_1_2;
	double v_drop;
	double exp_1;
	double exp_1_2;
	double exp_2;
};

static void skyraid_explosion_custom_charge_step(struct node_description *node)
{
	struct skyraid_explosion_custom_charge_context *context = (struct skyraid_explosion_custom_charge_context *)node->context;

	if (SKYRAID_EXPLOSION_CUSTOM_IN1 == 0)
		if (SKYRAID_EXPLOSION_CUSTOM_IN2 == 0)

			node->output = 0;
		else
		{

			context->v_cap -= context->v_cap * context->exp_2;
			node->output = context->v_cap * context->v_drop;
		}
	else
		if (SKYRAID_EXPLOSION_CUSTOM_IN2 == 0)
		{

			context->v_cap += (5.0 - context->v_cap) * context->exp_1;

			node->output = 0;
		}
		else
		{

			context->v_cap += (context->v_charge_1_2 - context->v_cap) * context->exp_1_2;
			node->output = context->v_cap * context->v_drop;
		}
}

static void skyraid_explosion_custom_charge_reset(struct node_description *node)
{
	struct skyraid_explosion_custom_charge_context *context = (struct skyraid_explosion_custom_charge_context *)node->context;

	context->v_drop =  RES_VOLTAGE_DIVIDER(SKYRAID_4066_R_ON, SKYRAID_4066_R_ON + SKYRAID_EXPLOSION_CUSTOM_R);
	context->v_charge_1_2 = 5.0 * context->v_drop;
	context->v_cap = 0;

	context->exp_2 = RC_CHARGE_EXP((SKYRAID_4066_R_ON + SKYRAID_EXPLOSION_CUSTOM_R) * SKYRAID_EXPLOSION_CUSTOM_C);

	context->exp_1 = RC_CHARGE_EXP(SKYRAID_4066_R_ON * SKYRAID_EXPLOSION_CUSTOM_C);

	context->exp_1_2 = RC_CHARGE_EXP(RES_2_PARALLEL(SKYRAID_4066_R_ON, SKYRAID_4066_R_ON + SKYRAID_EXPLOSION_CUSTOM_R) * SKYRAID_EXPLOSION_CUSTOM_C);

	node->output = 0;
}

static const struct discrete_module skyraid_explosion_custom_charge =
{ DST_CUSTOM, "Sky Raider charge", sizeof(struct skyraid_explosion_custom_charge_context), skyraid_explosion_custom_charge_reset, skyraid_explosion_custom_charge_step };

#define SKYRAID_MISSLE_CUSTOM_IN1		SKY_INPUT(0)
#define SKYRAID_MISSLE_CUSTOM_R1		SKY_INPUT(1)
#define SKYRAID_MISSLE_CUSTOM_R2		SKY_INPUT(2)
#define SKYRAID_MISSLE_CUSTOM_R3		SKY_INPUT(3)
#define SKYRAID_MISSLE_CUSTOM_C			SKY_INPUT(4)

struct skyraid_missle_custom_charge_context
{
	double v_charge[2];
	double v_cap;
	double exp[2];
};

#define SKYRAID_MISSLE_CHARGE_PLUS	(5.0 + 0.7)

static void skyraid_missle_custom_charge_step(struct node_description *node)
{
	struct skyraid_missle_custom_charge_context *context = (struct skyraid_missle_custom_charge_context *)node->context;

	int in_1 = (SKYRAID_MISSLE_CUSTOM_IN1 == 0) ? 0 : 1;

	context->v_cap += (context->v_charge[in_1] - context->v_cap) * context->exp[in_1];

	node->output = SKYRAID_MISSLE_CHARGE_PLUS - context->v_cap;
}

static void skyraid_missle_custom_charge_reset(struct node_description *node)
{
	struct skyraid_missle_custom_charge_context *context = (struct skyraid_missle_custom_charge_context *)node->context;

	context->v_charge[0] = 0;
	context->v_charge[1] = SKYRAID_MISSLE_CHARGE_PLUS * RES_VOLTAGE_DIVIDER(SKYRAID_MISSLE_CUSTOM_R1 + SKYRAID_MISSLE_CUSTOM_R2, SKYRAID_MISSLE_CUSTOM_R3);
	context->v_charge[1] = SKYRAID_MISSLE_CHARGE_PLUS - context->v_charge[1];
	context->v_cap = 0;

	context->exp[0] = RC_CHARGE_EXP(SKYRAID_MISSLE_CUSTOM_R2 * SKYRAID_MISSLE_CUSTOM_C);

	context->exp[1] = RC_CHARGE_EXP(RES_2_PARALLEL(SKYRAID_MISSLE_CUSTOM_R1 + SKYRAID_MISSLE_CUSTOM_R2, SKYRAID_MISSLE_CUSTOM_R3) * SKYRAID_MISSLE_CUSTOM_C);

	node->output = SKYRAID_MISSLE_CHARGE_PLUS;
}

static const struct discrete_module skyraid_missle_custom_charge =
{ DST_CUSTOM, "Sky Raider charge", sizeof(struct skyraid_missle_custom_charge_context), skyraid_missle_custom_charge_reset, skyraid_missle_custom_charge_step };

struct dsd_566_context
{
	int flip_flop;
	double cap_voltage, threshold_low, threshold_high;
	double v_osc_stable, v_osc_stop;
};

#define DSD_566__VMOD		SKY_INPUT(0)
#define DSD_566__R			SKY_INPUT(1)
#define DSD_566__C			SKY_INPUT(2)
#define DSD_566__VPOS		SKY_INPUT(3)
#define DSD_566__VNEG		SKY_INPUT(4)
#define DSD_566__VCHARGE	SKY_INPUT(5)

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

static void skyraid_566_step(struct node_description *node)
{
	struct dsd_566_context *context = node->context;
	double  i = 0;
	double  i_rise;
	double  dt;
	double  v_cap;
	int count_r = 0;


	node->output = 0;
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
				count_r++;
			}
		}
	} while(dt);

	context->cap_voltage = v_cap;
	node->output = count_r;
}

static void skyraid_566_reset(struct node_description *node)
{
	struct dsd_566_context *context = node->context;
	double supply = DSD_566__VPOS - DSD_566__VNEG;
	int index = (int)supply - 10;
	if (index < 0 || index > 5 || supply != floor(supply))
		fatalerror("Invalid Sky Raider oscillator supply");
	memset(context, 0, sizeof(*context));
	context->threshold_high = ne566.c_high[index] + DSD_566__VNEG;
	context->threshold_low = ne566.c_low[index] + DSD_566__VNEG;
	context->v_osc_stable = ne566.osc_stable[index] + DSD_566__VNEG;
	context->v_osc_stop = ne566.osc_stop[index] + DSD_566__VNEG;
	node->output = 0;
}

static const struct discrete_module skyraid_566_module =
{ DST_CUSTOM, "Sky Raider 566", sizeof(struct dsd_566_context), skyraid_566_reset, skyraid_566_step };
struct skyraid_shift_context { UINT16 bits; };
static void skyraid_shift_reset(struct node_description *node)
{
	memset(node->context, 0, sizeof(struct skyraid_shift_context));
	node->output = 0;
}
static void skyraid_shift_step(struct node_description *node)
{
	struct skyraid_shift_context *state = node->context;
	int count = *node->input[1];
	while (count-- > 0)
		state->bits = (state->bits << 1) | (*node->input[0] != 0);
	node->output = state->bits;
}
static const struct discrete_module skyraid_shift_module =
{ DST_CUSTOM, "Sky Raider shift", sizeof(struct skyraid_shift_context), skyraid_shift_reset, skyraid_shift_step };
DISCRETE_SOUND_START( skyraid_discrete )

	DISCRETE_INPUTX_LOGIC(SKYRAID_PLANE_SWEEP_EN, DEFAULT_TTL_V_LOGIC_1 * RES_VOLTAGE_DIVIDER(SKYRAID_R25, SKYRAID_R24), 0, 0)
	DISCRETE_INPUT_LOGIC(SKYRAID_MISSILE_EN)
	DISCRETE_INPUT_LOGIC(SKYRAID_EXPLOSION_EN)

	DISCRETE_INPUTX_LOGIC(SKYRAID_PLANE_ON__IC_E8, 270, 0, 0)
	DISCRETE_INPUT_LOGIC(SKYRAID_ATTRACT_EN)

	DISCRETE_LFSR_NOISE(SKYRAID_NOISE,
		1,
		SKYRAID_ATTRACT_EN,
		1.49 / ((SKYRAID_R20 + 2 * SKYRAID_R19) * SKYRAID_C51),
		1, 0, 0.5, &skyraid_lfsr)

	DISCRETE_LOGIC_NOR(NODE_20, 1, SKYRAID_EXPLOSION_EN, SKYRAID_NOISE)
	{ NODE_21, DST_CUSTOM, 4, { SKYRAID_EXPLOSION_EN, NODE_20, RES_2_PARALLEL(SKYRAID_R84, SKYRAID_R85 + SKYRAID_R86), SKYRAID_C68 }, { SKYRAID_EXPLOSION_EN, NODE_20, RES_2_PARALLEL(SKYRAID_R84, SKYRAID_R85 + SKYRAID_R86), SKYRAID_C68 }, &skyraid_explosion_custom_charge, "Sky Raider charge" },
	DISCRETE_OP_AMP_FILTER(NODE_22,
		1,
		NODE_21, 0,
		DISC_OP_AMP_FILTER_IS_BAND_PASS_1M, &skyraid_explosion_filter)

	DISCRETE_GAIN(NODE_23, NODE_22, SKYRAID_R119 / SKYRAID_R121 + 1)
	DISCRETE_CLAMP(SKYRAID_EXPLOSION_SND, 1, NODE_23, -5, 12.0 - 1.5, 0)

	DISCRETE_RCFILTER(NODE_30,
		1,
		SKYRAID_PLANE_SWEEP_EN,
		RES_2_PARALLEL(SKYRAID_R25, SKYRAID_R24), SKYRAID_C49)
	{ NODE_31, DST_CUSTOM, 6, { NODE_30, NODE_NC, NODE_NC, NODE_NC, NODE_NC, NODE_NC }, { NODE_30, SKYRAID_R18, SKYRAID_C48, 5, -5, 5 }, &skyraid_566_module, "Sky Raider 566" },
	{ NODE_32, DST_CUSTOM, 2, { SKYRAID_NOISE, NODE_31 }, { SKYRAID_NOISE, NODE_31 }, &skyraid_shift_module, "Sky Raider shift" },

	DISCRETE_TRANSFORM4(NODE_33, 1, NODE_32, 1, 1 << 14, 2, "01&02/3&|")
	DISCRETE_DAC_R1(SKYRAID_PLANE_SND, 1,
		NODE_33,
		DEFAULT_TTL_V_LOGIC_1, &skyraid_plane_dac)
	DISCRETE_TRANSFORM3(SKYRAID_JET_A_SND, 1, NODE_32, 1, DEFAULT_TTL_V_LOGIC_1, "01&1/2*")
	DISCRETE_TRANSFORM3(SKYRAID_JET_B_SND, 1, NODE_32, 32768, DEFAULT_TTL_V_LOGIC_1, "01&1/2*")

	{ NODE_40, DST_CUSTOM, 5, { SKYRAID_MISSILE_EN, SKYRAID_R12, SKYRAID_R14, SKYRAID_R13, SKYRAID_C44 }, { SKYRAID_MISSILE_EN, SKYRAID_R12, SKYRAID_R14, SKYRAID_R13, SKYRAID_C44 }, &skyraid_missle_custom_charge, "Sky Raider charge" },
	{ NODE_41, DST_CUSTOM, 6, { NODE_40, NODE_NC, NODE_NC, NODE_NC, NODE_NC, NODE_NC }, { NODE_40, SKYRAID_R16, SKYRAID_C45, 5, -5, SKYRAID_MISSLE_CHARGE_PLUS }, &skyraid_566_module, "Sky Raider 566" },
	{ NODE_42, DST_CUSTOM, 2, { SKYRAID_NOISE, NODE_41 }, { SKYRAID_NOISE, NODE_41 }, &skyraid_shift_module, "Sky Raider shift" },
	DISCRETE_TRANSFORM3(SKYRAID_MSL_A_SND, 1, NODE_42, 1, DEFAULT_TTL_V_LOGIC_1, "01&1/2*")
	DISCRETE_TRANSFORM3(SKYRAID_MSL_B_SND, 1, NODE_42, 32768, DEFAULT_TTL_V_LOGIC_1, "01&1/2*")

	DISCRETE_LOGIC_INVERT(NODE_91, 1, SKYRAID_ATTRACT_EN)
	DISCRETE_MIXER6(NODE_92,
		NODE_91,
		SKYRAID_EXPLOSION_SND,
		SKYRAID_JET_A_SND,
		SKYRAID_JET_B_SND,
		SKYRAID_MSL_A_SND,
		SKYRAID_MSL_B_SND,
		SKYRAID_PLANE_SND,
		&skyraid_mixer)
	DISCRETE_CRFILTER(NODE_93,
		1,
		NODE_92,
		SKYRAID_R122 + RES_6_PARALLEL(SKYRAID_R120, SKYRAID_R32, SKYRAID_R29, SKYRAID_R30, SKYRAID_R31, RES_2_PARALLEL(SKYRAID_R27, SKYRAID_R28)),
		SKYRAID_C93)

	DISCRETE_GAIN(NODE_94, NODE_93, - SKYRAID_R118 / (SKYRAID_R122 + RES_5_PARALLEL(SKYRAID_R120, SKYRAID_R32, SKYRAID_R29, SKYRAID_R30, SKYRAID_R31)))
	DISCRETE_CLAMP(NODE_95, 1, NODE_94, -5, 12.0 - 1.5, 0)
	DISCRETE_OUTPUT(NODE_95, 32700.0/8)
DISCRETE_SOUND_END


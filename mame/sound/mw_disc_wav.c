// license:BSD-3-Clause
// copyright-holders:Keith Wilkins, Derrick Renaud, Couriersud
/*
 *  (c) K.Wilkins 2000
 */


struct mw_dss_adsr_context {
	double phase;
};

struct mw_dss_counter_context {
	int clock_type;
	int out_type;
	int is_7492;
	int last;
	int count;
	double t_clock;
	double t_left;
};

struct mw_dss_lfsr_context {
	unsigned int lfsr_reg;
	int last;
	double t_clock;
	double t_left;
	double sampleStep;
	double shiftStep;
	double t;
	UINT8 reset_on_high;
	UINT8 invert_output;
	UINT8 out_is_f0;
};

struct mw_dss_noise_context {
	double phase;
};

struct mw_dss_note_context {
	int clock_type;
	int out_type;
	int last;
	double t_clock;
	double t_left;
	int max1;
	int max2;
	int count1;
	int count2;
};

struct mw_dss_op_amp_osc_context {
	const double *r1;
	const double *r2;
	const double *r3;
	const double *r4;
	const double *r5;
	const double *r6;
	const double *r7;
	const double *r8;
	int type;
	UINT8 flip_flop;
	UINT8 flip_flop_xor;
	UINT8 output_type;
	UINT8 has_enable;
	double v_out_high;
	double threshold_low;
	double threshold_high;
	double v_cap;
	double r_total;
	double i_fixed;
	double i_enable;
	double temp1;
	double temp2;
	double temp3;
	double is_linear_charge;
	double charge_rc[2];
	double charge_exp[2];
	double charge_v[2];
};

struct mw_dss_sawtoothwave_context {
	double phase;
	int type;
};

struct mw_dss_schmitt_osc_context {
	double ratioIn;
	double ratioFeedback;
	double vCap;
	double rc;
	double exponent;
	int state;
	int enable_type;
	UINT8 input_is_voltage;
};

struct mw_dss_sinewave_context {
	double phase;
};

struct mw_dss_squarewave_context {
	double phase;
	double trigger;
};

struct mw_dss_squarewfix_context {
	int flip_flop;
	double sampleStep;
	double tLeft;
	double tOff;
	double tOn;
};

struct mw_dss_trianglewave_context {
	double phase;
};

struct mw_dss_inverter_osc_context {
	double w;
	double wc;
	double vCap;
	double vG2_old;
	double Rp;
	double R1;
	double R2;
	double C;
	double tf_a;
	double tf_b;
};

#define mw_DSS_COUNTER__ENABLE          (*(node->input[0]))
#define mw_DSS_COUNTER__RESET           (*(node->input[1]))
#define mw_DSS_COUNTER__CLOCK           (*(node->input[2]))
#define mw_DSS_COUNTER__MAX             (*(node->input[3]))
#define mw_DSS_COUNTER__DIR             (*(node->input[4]))
#define mw_DSS_COUNTER__INIT            (*(node->input[5]))
#define mw_DSS_COUNTER__CLOCK_TYPE      (*(node->input[6]))

static const int mw_disc_7492_count[6] = { 0x00, 0x01, 0x02, 0x04, 0x05, 0x06 };

void mw_dss_counter_step(node_description *node)
{
	struct mw_dss_counter_context *context = node->context;
	double cycles;
	int clock = 0, last_count, inc = 0;
	int max = mw_DSS_COUNTER__MAX;
	double xTime = 0;

	if (context->clock_type == MW_DISC_CLK_IS_FREQ) {

		cycles = (context->t_left + discrete_current_context->sample_time) / context->t_clock;
		inc = (int)cycles;
		context->t_left = (cycles - inc) * context->t_clock;
		if (inc) xTime = context->t_left / discrete_current_context->sample_time;
	} else {
		clock = (int)mw_DSS_COUNTER__CLOCK;
		xTime = mw_DSS_COUNTER__CLOCK - clock;
	}

	if (mw_DSS_COUNTER__RESET) {
		context->count = mw_DSS_COUNTER__INIT;
		node->output = context->is_7492 ? 0 : context->count;
		return;
	}

	if (mw_DSS_COUNTER__ENABLE) {
		last_count = context->count;

		switch (context->clock_type) {
		case MW_DISC_CLK_ON_F_EDGE:
		case MW_DISC_CLK_ON_R_EDGE:

			clock = (clock != 0);
			if (context->last != clock) {
				context->last = clock;
				if (context->clock_type == clock) {

					inc = 1;
				}
			}
			break;

		case MW_DISC_CLK_BY_COUNT:

			inc = clock;
			break;
		}

		for (clock = 0; clock < inc; clock++) {
			context->count += mw_DSS_COUNTER__DIR ? 1 : -1;
			if (context->count < 0) context->count = max;
			if (context->count > max) context->count = 0;
		}

		node->output = context->is_7492 ? mw_disc_7492_count[context->count] : context->count;

		if (context->count != last_count) {

			switch (context->out_type) {
			case MW_DISC_OUT_IS_ENERGY:
				if (xTime != 0)
					node->output = (context->count > last_count) ? (last_count + xTime) : (last_count - xTime);
				break;
			case MW_DISC_OUT_HAS_XTIME:
				node->output += xTime;
				break;
			}
		}
	} else
		node->output = context->count;
}

void mw_dss_counter_reset(node_description *node)
{
	struct mw_dss_counter_context *context = node->context;

	context->clock_type = (int)mw_DSS_COUNTER__CLOCK_TYPE;
	if (context->clock_type == MW_DISC_COUNTER_IS_7492) {
		context->clock_type = MW_DISC_CLK_ON_F_EDGE;
		context->is_7492 = 1;
	} else
		context->is_7492 = 0;
	if ((context->clock_type < MW_DISC_CLK_ON_F_EDGE) || (context->clock_type > MW_DISC_CLK_IS_FREQ))
		discrete_log("Invalid clock type passed in NODE_%d\n", node->node - NODE_START);
	context->last = 0;
	if (context->clock_type == MW_DISC_CLK_IS_FREQ) context->t_clock = 1.0 / mw_DSS_COUNTER__CLOCK;
	context->t_left = 0;
	context->count = mw_DSS_COUNTER__INIT;
	node->output = mw_DSS_COUNTER__INIT;
}

#define mw_DSS_LFSR_NOISE__ENABLE       (*(node->input[0]))
#define mw_DSS_LFSR_NOISE__RESET        (*(node->input[1]))
#define mw_DSS_LFSR_NOISE__CLOCK        (*(node->input[2]))
#define mw_DSS_LFSR_NOISE__AMP          (*(node->input[3]))
#define mw_DSS_LFSR_NOISE__FEED (*(node->input[4]))
#define mw_DSS_LFSR_NOISE__BIAS (*(node->input[5]))

int     mw_dss_lfsr_function(int myfunc, int in0, int in1, int bitmask)
{
	int retval;

	in0 &= bitmask;
	in1 &= bitmask;

	switch (myfunc) {
	case MW_DISC_LFSR_XOR:
		retval = in0 ^ in1;
		break;
	case MW_DISC_LFSR_OR:
		retval = in0 | in1;
		break;
	case MW_DISC_LFSR_AND:
		retval = in0 & in1;
		break;
	case MW_DISC_LFSR_XNOR:
		retval = in0 ^ in1;
		retval = retval ^ bitmask;
		break;
	case MW_DISC_LFSR_NOR:
		retval = in0 | in1;
		retval = retval ^ bitmask;
		break;
	case MW_DISC_LFSR_NAND:
		retval = in0 & in1;
		retval = retval ^ bitmask;
		break;
	case MW_DISC_LFSR_IN0:
		retval = in0;
		break;
	case MW_DISC_LFSR_IN1:
		retval = in1;
		break;
	case MW_DISC_LFSR_NOT_IN0:
		retval = in0 ^ bitmask;
		break;
	case MW_DISC_LFSR_NOT_IN1:
		retval = in1 ^ bitmask;
		break;
	case MW_DISC_LFSR_REPLACE:
		retval = in0 & ~in1;
		retval = retval | in1;
		break;
	case MW_DISC_LFSR_XOR_INV_IN0:
		retval = in0 ^ bitmask;
		retval = retval ^ in1;
		break;
	case MW_DISC_LFSR_XOR_INV_IN1:
		retval = in1 ^ bitmask;
		retval = retval ^ in0;
		break;
	default:
		discrete_log("mw_dss_lfsr_function - Invalid function type passed");
		retval = 0;
		break;
	}
	return retval;
}

void mw_dss_lfsr_reset(node_description *node);

void mw_dss_lfsr_step(node_description *node)
{
	const mw_discrete_lfsr_desc *lfsr_desc = node->custom;
	struct mw_dss_lfsr_context *context = node->context;
	double cycles;
	int clock, inc = 0;
	int fb0, fb1, fbresult;

	if (lfsr_desc->clock_type == MW_DISC_CLK_IS_FREQ) {

		cycles = (context->t_left + discrete_current_context->sample_time) / context->t_clock;
		inc = (int)cycles;
		context->t_left = (cycles - inc) * context->t_clock;
	}

	if ((mw_DSS_LFSR_NOISE__RESET ? 1 : 0) == context->reset_on_high) {
		mw_dss_lfsr_reset(node);
		return;
	}

	switch (lfsr_desc->clock_type) {
	case MW_DISC_CLK_ON_F_EDGE:
	case MW_DISC_CLK_ON_R_EDGE:

		clock = (mw_DSS_LFSR_NOISE__CLOCK != 0);
		if (context->last != clock) {
			context->last = clock;
			if (lfsr_desc->clock_type == clock) {

				inc = 1;
			}
		}
		break;

	case MW_DISC_CLK_BY_COUNT:

		inc = (int)mw_DSS_LFSR_NOISE__CLOCK;
		break;
	}

	for (clock = 0; clock < inc; clock++) {

		fbresult = ((context->lfsr_reg) >> (lfsr_desc->bitlength)) & 0x01;

		fbresult = mw_dss_lfsr_function(lfsr_desc->feedback_function1, fbresult, ((mw_DSS_LFSR_NOISE__FEED)?0x01:0x00), 0x01);

		fbresult = fbresult * lfsr_desc->feedback_function2_mask;

		context->lfsr_reg = (context->lfsr_reg) << 1;

		context->lfsr_reg = mw_dss_lfsr_function(lfsr_desc->feedback_function2, fbresult, (context->lfsr_reg), ((1 << (lfsr_desc->bitlength)) - 1));

		fb0 = ((context->lfsr_reg) >> (lfsr_desc->feedback_bitsel0)) & 0x01;
		fb1 = ((context->lfsr_reg) >> (lfsr_desc->feedback_bitsel1)) & 0x01;

		fbresult = mw_dss_lfsr_function(lfsr_desc->feedback_function0, fb0, fb1, 0x01);
		context->lfsr_reg = mw_dss_lfsr_function(MW_DISC_LFSR_REPLACE, (context->lfsr_reg), fbresult << (lfsr_desc->bitlength), ((2 << (lfsr_desc->bitlength)) - 1));

		if (context->out_is_f0)
			node->output = fbresult & 0x01;
		else
			node->output = ((context->lfsr_reg) >> (lfsr_desc->output_bit)) & 0x01;

		if (context->invert_output) node->output = (node->output)?0.0:1.0;

		node->output = (node->output)?(mw_DSS_LFSR_NOISE__AMP) / 2:-(mw_DSS_LFSR_NOISE__AMP) / 2;

		node->output = node->output + mw_DSS_LFSR_NOISE__BIAS;
	}

	if (!mw_DSS_LFSR_NOISE__ENABLE) {
		node->output = 0;
	}
}

void mw_dss_lfsr_reset(node_description *node)
{
	const mw_discrete_lfsr_desc *lfsr_desc = node->custom;
	struct mw_dss_lfsr_context *context = node->context;
	int fb0, fb1, fbresult;

	context->reset_on_high = (lfsr_desc->flags & MW_DISC_LFSR_FLAG_RESET_TYPE_H) ? 1 : 0;
	context->invert_output = lfsr_desc->flags & MW_DISC_LFSR_FLAG_OUT_INVERT;
	context->out_is_f0 = (lfsr_desc->flags & MW_DISC_LFSR_FLAG_OUTPUT_F0) ? 1 : 0;

	if ((lfsr_desc->clock_type < MW_DISC_CLK_ON_F_EDGE) || (lfsr_desc->clock_type > MW_DISC_CLK_IS_FREQ))
		discrete_log("Invalid clock type passed in NODE_%d\n", node->node - NODE_START);
	context->last = (mw_DSS_COUNTER__CLOCK != 0);
	if (lfsr_desc->clock_type == MW_DISC_CLK_IS_FREQ) context->t_clock = 1.0 / mw_DSS_LFSR_NOISE__CLOCK;
	context->t_left = 0;

	context->lfsr_reg = lfsr_desc->reset_value;

	fb0 = ((context->lfsr_reg) >> (lfsr_desc->feedback_bitsel0)) & 0x01;
	fb1 = ((context->lfsr_reg) >> (lfsr_desc->feedback_bitsel1)) & 0x01;

	fbresult = mw_dss_lfsr_function(lfsr_desc->feedback_function0, fb0, fb1, 0x01);
	context->lfsr_reg = mw_dss_lfsr_function(MW_DISC_LFSR_REPLACE, (context->lfsr_reg), fbresult << (lfsr_desc->bitlength), ((2 << (lfsr_desc->bitlength)) - 1));

	node->output = ((context->lfsr_reg) >> (lfsr_desc->output_bit)) & 0x01;

	if (lfsr_desc->flags & MW_DISC_LFSR_FLAG_OUT_INVERT) node->output = (node->output)?0.0:1.0;

	node->output = (node->output)?(mw_DSS_LFSR_NOISE__AMP) / 2:-(mw_DSS_LFSR_NOISE__AMP) / 2;

	node->output = node->output + mw_DSS_LFSR_NOISE__BIAS;
}

#define mw_DSS_NOISE__ENABLE    (*(node->input[0]))
#define mw_DSS_NOISE__FREQ              (*(node->input[1]))
#define mw_DSS_NOISE__AMP               (*(node->input[2]))
#define mw_DSS_NOISE__BIAS              (*(node->input[3]))

void mw_dss_noise_step(node_description *node)
{
	struct mw_dss_noise_context *context = node->context;

	if (mw_DSS_NOISE__ENABLE) {

		if (context->phase > (2.0 * M_PI)) {

			int newval = (mame_rand() & 0x7fff) - 16384;

			node->output = mw_DSS_NOISE__AMP / 2;
			if (newval > 0)
				node->output *= ((double)newval / 16383);
			else
				node->output *= ((double)newval / 16384);

			node->output += mw_DSS_NOISE__BIAS;
		}
	} else {
		node->output = 0;
	}

	context->phase = fmod(context->phase, 2.0 * M_PI);

	context->phase += ((2.0 * M_PI * mw_DSS_NOISE__FREQ) / discrete_current_context->sample_rate);
}

void mw_dss_noise_reset(node_description *node)
{
	struct mw_dss_noise_context *context = node->context;

	context->phase = 0;
	mw_dss_noise_step(node);
}

#define DSS_NOTE__ENABLE               (*(node->input[0]))
#define DSS_NOTE__CLOCK                (*(node->input[1]))
#define DSS_NOTE__DATA                 (*(node->input[2]))
#define DSS_NOTE__MAX1                 (*(node->input[3]))
#define DSS_NOTE__MAX2                 (*(node->input[4]))
#define DSS_NOTE__CLOCK_TYPE   (*(node->input[5]))

void mw_dss_note_step(node_description *node)
{
	struct mw_dss_note_context *context = node->context;

	double cycles;
	int clock = 0, last_count2, inc = 0;
	double xTime = 0;

	if (context->clock_type == MW_DISC_CLK_IS_FREQ) {

		cycles = (context->t_left + discrete_current_context->sample_time) / context->t_clock;
		inc = (int)cycles;
		context->t_left = (cycles - inc) * context->t_clock;
		if (inc) xTime = context->t_left / discrete_current_context->sample_time;
	} else {

		clock = (int)DSS_NOTE__CLOCK;
		xTime = DSS_NOTE__CLOCK - clock;
	}

	if (DSS_NOTE__ENABLE) {
		last_count2 = context->count2;

		switch (context->clock_type) {
		case MW_DISC_CLK_ON_F_EDGE:
		case MW_DISC_CLK_ON_R_EDGE:

			clock = (clock != 0);
			if (context->last != clock) {
				context->last = clock;
				if (context->clock_type == clock) {

					inc = 1;
				}
			}
			break;

		case MW_DISC_CLK_BY_COUNT:

			inc = clock;
			break;
		}

		if (DSS_NOTE__DATA != DSS_NOTE__MAX1) {
			for (clock = 0; clock < inc; clock++) {
				context->count1++;
				if (context->count1 > context->max1) {

					context->count1 = (int)DSS_NOTE__DATA;
					context->count2 += 1;
					if (context->count2 > context->max2) context->count2 = 0;
				}
			}
		}

		node->output = context->count2;
		if (context->count2 != last_count2) {

			switch (context->out_type) {
			case MW_DISC_OUT_IS_ENERGY:
				if (xTime != 0)
					node->output = (context->count2 > last_count2) ? (last_count2 + xTime) : (last_count2 - xTime);
				break;
			case MW_DISC_OUT_HAS_XTIME:
				node->output += xTime;
				break;
			}
		}
	} else
		node->output = 0;
}

void mw_dss_note_reset(node_description *node)
{
	struct mw_dss_note_context *context = node->context;

	context->clock_type = (int)DSS_NOTE__CLOCK_TYPE & MW_DISC_CLK_MASK;
	context->out_type = (int)DSS_NOTE__CLOCK_TYPE & MW_DISC_OUT_MASK;
	context->last = (DSS_NOTE__CLOCK != 0);
	if (context->clock_type == MW_DISC_CLK_IS_FREQ) context->t_clock = 1.0 / DSS_NOTE__CLOCK;
	context->t_left = 0;

	context->count1 = (int)DSS_NOTE__DATA;
	context->count2 = 0;
	context->max1 = (int)DSS_NOTE__MAX1;
	context->max2 = (int)DSS_NOTE__MAX2;
	node->output = 0;
}

#define mw_DSS_OP_AMP_OSC__ENABLE       (*(node->input[0]))
#define mw_DSS_OP_AMP_OSC__VMOD1        (*(node->input[1]))
#define mw_DSS_OP_AMP_OSC__VMOD2        (*(node->input[2]))

#define mw_DSS_OP_AMP_OSC_NORTON_VP_IN          context->v_out_high

void mw_dss_op_amp_osc_step(node_description *node)
{
	const mw_discrete_op_amp_osc_info *info = (const mw_discrete_op_amp_osc_info *)node->custom;
	struct mw_dss_op_amp_osc_context   *context = (struct mw_dss_op_amp_osc_context *)node->context;

	double i = 0;
	double v = 0;
	double dt;
	double v_cap;
	double v_cap_next = 0;
	double charge[2] = { 0 };
	double x_time = 0;
	double exponent;
	UINT8 force_charge = 0;
	UINT8 enable = mw_DSS_OP_AMP_OSC__ENABLE;
	UINT8 update_exponent = 0;
	UINT8 flip_flop = context->flip_flop;
	int count_f = 0;
	int count_r = 0;

	dt = discrete_current_context->sample_time;
	v_cap = context->v_cap;

	switch (context->type) {
	case MW_DISC_OP_AMP_OSCILLATOR_VCO_1:

		i = mw_DSS_OP_AMP_OSC__VMOD1 * context->temp1;
		charge[0] = (mw_DSS_OP_AMP_OSC__VMOD1 - i) / info->r1;
		charge[1] = (i - (mw_DSS_OP_AMP_OSC__VMOD1 * context->temp2)) / context->temp3;
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_1 | MW_DISC_OP_AMP_IS_NORTON:
	{

		double i1, i2;

		if (context->has_enable) {
			if (enable)
				i = context->i_enable;
			enable = 1;
		}

		charge[0] = mw_DSS_OP_AMP_OSC_NORTON_VP_IN / *context->r1 - i;
		charge[1] = (context->v_out_high - MW_OP_AMP_NORTON_VBE) / *context->r2 - charge[0];

		i1 = mw_DSS_OP_AMP_OSC_NORTON_VP_IN / *context->r5;
		i2 = (0.0 - MW_OP_AMP_NORTON_VBE) / *context->r4;
		context->threshold_low = (i1 + i2) * *context->r3 + MW_OP_AMP_NORTON_VBE;
		i2 = (context->v_out_high - MW_OP_AMP_NORTON_VBE) / *context->r4;
		context->threshold_high = (i1 + i2) * *context->r3 + MW_OP_AMP_NORTON_VBE;
		break;
	}

	case MW_DISC_OP_AMP_OSCILLATOR_VCO_1 | MW_DISC_OP_AMP_IS_NORTON:

		if (info->r7 == 0) {

			v = mw_DSS_OP_AMP_OSC__VMOD1;
		} else {

			i = context->i_fixed;
			i += mw_DSS_OP_AMP_OSC__VMOD1 / info->r7;
			if (info->r8 != 0)
				i += mw_DSS_OP_AMP_OSC__VMOD2 / info->r8;
			v = i * context->r_total;
		}

		v -= MW_OP_AMP_NORTON_VBE;
		charge[0] = v / info->r1;
		charge[1] = v / info->r2 - charge[0];

		force_charge = !enable;
		enable = 1;
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_VCO_2 | MW_DISC_OP_AMP_IS_NORTON:

		i = mw_DSS_OP_AMP_OSC__VMOD1 / info->r1;
		charge[0] = i - context->temp1;
		charge[1] = context->temp2 - i;

		if (charge[0] < 0) {
			force_charge = 1;
			charge[0] *= -1;
		}
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_VCO_3 | MW_DISC_OP_AMP_IS_NORTON:

		charge[0] = context->i_fixed;

		if (context->has_enable) {
			if (enable)
				charge[0] -= context->i_enable;
			enable = 1;
		}

		v = mw_DSS_OP_AMP_OSC__VMOD1 - MW_OP_AMP_NORTON_VBE;
		if (v < 0) v = 0;
		charge[0] += v / info->r1;
		if (info->r6 != 0) {
			v = mw_DSS_OP_AMP_OSC__VMOD2 - MW_OP_AMP_NORTON_VBE;
			charge[0] += v / info->r6;
		}
		charge[1] = context->temp1 - charge[0];
		break;
	}

	if (!enable) {

		node->output = 0;
		return;
	}

	do{
		if (context->is_linear_charge) {
			if ((flip_flop ^ context->flip_flop_xor) || force_charge) {

				v_cap_next = v_cap + (charge[1] * dt / info->c);
				dt = 0;

				if (v_cap_next > context->threshold_high) {
					flip_flop = context->flip_flop_xor;
					if (flip_flop)
						count_r++;
					else
						count_f++;
					if (force_charge) {

						if (v_cap_next > context->v_out_high)
							v_cap_next = context->v_out_high;
					} else {

						dt = info->c * (v_cap_next - context->threshold_high) / charge[1];
						x_time = dt;
						v_cap_next = context->threshold_high;
					}
				}
			} else {

				v_cap_next = v_cap - (charge[0] * dt / info->c);
				dt = 0;

				if (v_cap_next < context->threshold_low) {
					flip_flop = !context->flip_flop_xor;
					if (flip_flop)
						count_r++;
					else
						count_f++;

					dt = info->c * (context->threshold_low - v_cap_next) / charge[0];
					x_time = dt;
					v_cap_next = context->threshold_low;
				}
			}
		} else {
			if (update_exponent)
				exponent = RC_CHARGE_EXP_DT(context->charge_rc[flip_flop], dt);
			else
				exponent = context->charge_exp[flip_flop];

			v_cap_next = v_cap + ((context->charge_v[flip_flop] - v_cap) * exponent);
			dt = 0;

			if (flip_flop) {

				if (v_cap_next > context->threshold_high) {
					dt = context->charge_rc[1] * log(1.0 / (1.0 - ((v_cap_next - context->threshold_high) / (context->v_out_high - v_cap))));
					x_time = dt;
					v_cap_next = context->threshold_high;
					flip_flop = 0;
					count_f++;
					update_exponent = 1;
				}
			} else {

				if (v_cap_next < context->threshold_low) {
					dt = context->charge_rc[0] * log(1.0 / (1.0 - ((context->threshold_low - v_cap_next) / v_cap)));
					x_time = dt;
					v_cap_next = context->threshold_low;
					flip_flop = 1;
					count_r++;
					update_exponent = 1;
				}
			}
		}
		v_cap = v_cap_next;
	} while (dt);
	if (v_cap > context->v_out_high)
		v_cap = context->v_out_high;
	if (v_cap < 0)
		v_cap = 0;
	context->v_cap = v_cap;

	x_time = dt / discrete_current_context->sample_time;

	switch (context->output_type) {
	case MW_DISC_OP_AMP_OSCILLATOR_OUT_CAP:
		node->output = v_cap;
		break;
	case MW_DISC_OP_AMP_OSCILLATOR_OUT_ENERGY:
		if (x_time == 0) x_time = 1.0;
		node->output = context->v_out_high * (flip_flop ? x_time : (1.0 - x_time));
		break;
	case MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW:
		if (count_f + count_r >= 2)

			node->output = context->flip_flop ? 0 : context->v_out_high;
		else
			node->output = flip_flop * context->v_out_high;
		break;
	case MW_DISC_OP_AMP_OSCILLATOR_OUT_COUNT_F_X:
		node->output = count_f ? count_f + x_time : count_f;
		break;
	case MW_DISC_OP_AMP_OSCILLATOR_OUT_COUNT_R_X:
		node->output = count_r ? count_r + x_time : count_r;
		break;
	case MW_DISC_OP_AMP_OSCILLATOR_OUT_LOGIC_X:
		node->output = context->flip_flop + x_time;
		break;
	}
	context->flip_flop = flip_flop;
}

#define DIODE_DROP      0.7

void mw_dss_op_amp_osc_reset(node_description *node)
{
	const mw_discrete_op_amp_osc_info *info = (const mw_discrete_op_amp_osc_info *)node->custom;
	struct mw_dss_op_amp_osc_context *context = (struct mw_dss_op_amp_osc_context *)node->context;
	const double *r_info_ptr;
	const double **r_context_ptr;
	int loop;
	node_description *r_node;

	double i1 = 0;
	double i2 = 0;

	r_info_ptr = &info->r1;
	r_context_ptr = &context->r1;
	for (loop = 0; loop < 8; loop++) {
		if IS_VALUE_A_NODE(*r_info_ptr) {
			r_node = discrete_find_node(NULL, *r_info_ptr);
			*r_context_ptr = &(r_node->output);
		} else
			*r_context_ptr = r_info_ptr;
		r_info_ptr++;
		r_context_ptr++;
	}

	context->is_linear_charge = 1;
	context->output_type = info->type & MW_DISC_OP_AMP_OSCILLATOR_OUT_MASK;
	context->type = info->type & MW_DISC_OP_AMP_OSCILLATOR_TYPE_MASK;
	context->charge_rc[0] = 0;
	context->charge_rc[1] = 0;
	context->charge_v[0] = 0;
	context->charge_v[1] = 0;
	context->i_fixed = 0;
	context->has_enable = 0;

	switch (context->type) {
	case MW_DISC_OP_AMP_OSCILLATOR_VCO_1:

		context->flip_flop_xor = 0;

		context->temp1 = (info->vP / 2) / info->r4;
		context->temp2 = (info->vP - MW_OP_AMP_VP_RAIL_OFFSET) / info->r3;
		context->temp3 = 1.0 / (1.0 / info->r3 + 1.0 / info->r4);
		context->threshold_low = context->temp1 * context->temp3;
		context->threshold_high = (context->temp1 + context->temp2) * context->temp3;

		context->flip_flop = 1;

		context->temp1 = info->r5 / (info->r2 + info->r5);
		context->temp2 = info->r6 / (info->r1 + info->r6);
		context->temp3 = 1.0 / (1.0 / info->r1 + 1.0 / info->r6);
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_1 | MW_DISC_OP_AMP_IS_NORTON:

		context->flip_flop_xor = 0;

		context->flip_flop = 1;

		if (info->r6 > 0) {
			context->has_enable = 1;
			context->i_enable = (info->vP - MW_OP_AMP_NORTON_VBE) / (info->r6 + RES_K(1));
		}
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_2 | MW_DISC_OP_AMP_IS_NORTON:
		context->is_linear_charge = 0;

		if (info->r1 != 0) {
			context->charge_rc[0] = 1.0 / info->r1;
			context->charge_rc[1] = 1.0 / info->r1;
			context->charge_v[1] = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r1;
		}
		if (info->r5 != 0) {
			context->charge_rc[0] += 1.0 / info->r5;
			context->charge_v[0] = DIODE_DROP / info->r5;
		}
		if (info->r6 != 0) {
			context->charge_rc[1] += 1.0 / info->r6;
			context->charge_v[1] += (info->vP - MW_OP_AMP_NORTON_VBE - DIODE_DROP) / info->r6;
		}
		context->charge_rc[0] += 1.0 / info->r2;
		context->charge_rc[0] = 1.0 / context->charge_rc[0];
		context->charge_v[0] += MW_OP_AMP_NORTON_VBE / info->r2;
		context->charge_v[0] *= context->charge_rc[0];
		context->charge_rc[1] += 1.0 / info->r2;
		context->charge_rc[1] = 1.0 / context->charge_rc[1];
		context->charge_v[1] += MW_OP_AMP_NORTON_VBE / info->r2;
		context->charge_v[1] *= context->charge_rc[1];

		context->charge_rc[0] *= info->c;
		context->charge_rc[1] *= info->c;
		context->charge_exp[0] = RC_CHARGE_EXP(context->charge_rc[0]);
		context->charge_exp[1] = RC_CHARGE_EXP(context->charge_rc[1]);
		context->threshold_low = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r4;
		context->threshold_high = context->threshold_low + (info->vP - 2 * MW_OP_AMP_NORTON_VBE) / info->r3;;
		context->threshold_low = context->threshold_low * info->r2 + MW_OP_AMP_NORTON_VBE;
		context->threshold_high = context->threshold_high * info->r2 + MW_OP_AMP_NORTON_VBE;

		context->flip_flop = 1;
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_VCO_1 | MW_DISC_OP_AMP_IS_NORTON:

		context->flip_flop_xor = 1;

		context->flip_flop = 0;

		if (info->r6 != 0) context->i_fixed += info->vP / info->r6;
		context->i_fixed += MW_OP_AMP_NORTON_VBE / info->r1;
		context->i_fixed += MW_OP_AMP_NORTON_VBE / info->r2;

		context->r_total = 1.0 / info->r1 + 1.0 / info->r2 + 1.0 / info->r7;
		if (info->r6) context->r_total += 1.0 / info->r6;
		if (info->r8) context->r_total += 1.0 / info->r8;
		context->r_total = 1.0 / context->r_total;

		i1 = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r5;
		i2 = (info->vP - MW_OP_AMP_NORTON_VBE - MW_OP_AMP_NORTON_VBE) / info->r4;
		context->threshold_low = (i1 - i2) * info->r3 + MW_OP_AMP_NORTON_VBE;
		i2 = (0.0 - MW_OP_AMP_NORTON_VBE) / info->r4;
		context->threshold_high = (i1 - i2) * info->r3 + MW_OP_AMP_NORTON_VBE;
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_VCO_2 | MW_DISC_OP_AMP_IS_NORTON:

		context->flip_flop_xor = 0;

		context->flip_flop = 1;

		context->temp1 = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r2;
		context->temp2 = (info->vP - MW_OP_AMP_NORTON_VBE) * (1.0 / info->r2 + 1.0 / info->r6);

		i1 = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r5;
		i2 = (0.0 - MW_OP_AMP_NORTON_VBE) / info->r4;
		context->threshold_low = (i1 + i2) * info->r3 + MW_OP_AMP_NORTON_VBE;
		i2 = (info->vP - MW_OP_AMP_NORTON_VBE - MW_OP_AMP_NORTON_VBE) / info->r4;
		context->threshold_high = (i1 + i2) * info->r3 + MW_OP_AMP_NORTON_VBE;
		break;

	case MW_DISC_OP_AMP_OSCILLATOR_VCO_3 | MW_DISC_OP_AMP_IS_NORTON:

		context->flip_flop_xor = 0;

		context->flip_flop = 1;

		if (info->r8 > 0) {
			context->has_enable = 1;
			context->i_enable = (info->vP - MW_OP_AMP_NORTON_VBE) / (info->r8 + RES_K(1));
		}

		if (info->r7 != 0) context->i_fixed = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r7;
		context->temp1 = (info->vP - MW_OP_AMP_NORTON_VBE - MW_OP_AMP_NORTON_VBE) / info->r2;

		i1 = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r5;
		i2 = (0.0 - MW_OP_AMP_NORTON_VBE) / info->r4;
		context->threshold_low = (i1 + i2) * info->r3 + MW_OP_AMP_NORTON_VBE;
		i2 = (info->vP - MW_OP_AMP_NORTON_VBE - MW_OP_AMP_NORTON_VBE) / info->r4;
		context->threshold_high = (i1 + i2) * info->r3 + MW_OP_AMP_NORTON_VBE;
		break;
	}

	context->v_out_high = info->vP - ((context->type & MW_DISC_OP_AMP_IS_NORTON) ? MW_OP_AMP_NORTON_VBE : MW_OP_AMP_VP_RAIL_OFFSET);
	context->v_cap = 0;

	mw_dss_op_amp_osc_step(node);
}

#define mw_DSS_SAWTOOTHWAVE__ENABLE     (*(node->input[0]))
#define mw_DSS_SAWTOOTHWAVE__FREQ               (*(node->input[1]))
#define mw_DSS_SAWTOOTHWAVE__AMP                (*(node->input[2]))
#define mw_DSS_SAWTOOTHWAVE__BIAS               (*(node->input[3]))
#define mw_DSS_SAWTOOTHWAVE__GRAD               (*(node->input[4]))
#define mw_DSS_SAWTOOTHWAVE__PHASE              (*(node->input[5]))

void mw_dss_sawtoothwave_step(node_description *node)
{
	struct mw_dss_sawtoothwave_context *context = node->context;

	if (mw_DSS_SAWTOOTHWAVE__ENABLE) {
		node->output = (context->type == 0)?context->phase * (mw_DSS_SAWTOOTHWAVE__AMP / (2.0 * M_PI)):mw_DSS_SAWTOOTHWAVE__AMP - (context->phase * (mw_DSS_SAWTOOTHWAVE__AMP / (2.0 * M_PI)));
		node->output -= mw_DSS_SAWTOOTHWAVE__AMP / 2.0;

		node->output = node->output + mw_DSS_SAWTOOTHWAVE__BIAS;
	} else {
		node->output = 0;
	}

	context->phase = fmod((context->phase + ((2.0 * M_PI * mw_DSS_SAWTOOTHWAVE__FREQ) / discrete_current_context->sample_rate)), 2.0 * M_PI);
}

void mw_dss_sawtoothwave_reset(node_description *node)
{
	struct mw_dss_sawtoothwave_context *context = node->context;
	double start;

	start = (mw_DSS_SAWTOOTHWAVE__PHASE / 360.0) * (2.0 * M_PI);

	context->phase = fmod(start, 2.0 * M_PI);

	context->type = (mw_DSS_SAWTOOTHWAVE__GRAD)?1:0;

	mw_dss_sawtoothwave_step(node);
}

#define mw_DSS_SCHMITT_OSC__ENABLE      (int)(*(node->input[0]))
#define mw_DSS_SCHMITT_OSC__VIN (*(node->input[1]))
#define mw_DSS_SCHMITT_OSC__AMP (*(node->input[2]))

void mw_dss_schmitt_osc_step(node_description *node)
{
	const mw_discrete_schmitt_osc_desc *info = node->custom;
	struct mw_dss_schmitt_osc_context *context = node->context;

	double supply, vCap, new_vCap, t, exponent;

	vCap = context->vCap;
	exponent = context->exponent;

	do{
		t = 0;

		supply = context->input_is_voltage ? context->ratioIn * mw_DSS_SCHMITT_OSC__VIN : (mw_DSS_SCHMITT_OSC__VIN ? context->ratioIn * info->vGate : 0);
		supply += (context->state ? context->ratioFeedback : 0);
		new_vCap = vCap + ((supply - vCap) * exponent);
		if (context->state) {

			if (new_vCap > info->trshRise) {

				t = context->rc * log(1.0 / (1.0 - ((new_vCap - info->trshRise) / (info->vGate - vCap))));

				exponent = 1.0 - exp(-t / context->rc);
				vCap = new_vCap = info->trshRise;
				context->state = 0;
			}
		} else {

			if (new_vCap < info->trshFall) {

				t = context->rc * log(1.0 / (1.0 - ((info->trshFall - new_vCap) / vCap)));

				exponent = 1.0 - exp(-t / context->rc);
				vCap = new_vCap = info->trshFall;
				context->state = 1;
			}
		}
	} while (t);

	context->vCap = new_vCap;

	switch (context->enable_type) {
	case MW_DISC_SCHMITT_OSC_ENAB_IS_AND:
		node->output = mw_DSS_SCHMITT_OSC__ENABLE && context->state;
		break;
	case MW_DISC_SCHMITT_OSC_ENAB_IS_NAND:
		node->output = !(mw_DSS_SCHMITT_OSC__ENABLE && context->state);
		break;
	case MW_DISC_SCHMITT_OSC_ENAB_IS_OR:
		node->output = mw_DSS_SCHMITT_OSC__ENABLE || context->state;
		break;
	case MW_DISC_SCHMITT_OSC_ENAB_IS_NOR:
		node->output = !(mw_DSS_SCHMITT_OSC__ENABLE || context->state);
		break;
	}
	node->output *= mw_DSS_SCHMITT_OSC__AMP;
}

void mw_dss_schmitt_osc_reset(node_description *node)
{
	const mw_discrete_schmitt_osc_desc *info = node->custom;
	struct mw_dss_schmitt_osc_context *context = node->context;
	double rSource;

	context->enable_type = info->options & MW_DISC_SCHMITT_OSC_ENAB_MASK;
	context->input_is_voltage = (info->options & MW_DISC_SCHMITT_OSC_IN_IS_VOLTAGE) ? 1 : 0;

	context->ratioIn = info->rFeedback / (info->rIn + info->rFeedback);
	context->ratioFeedback = info->rIn / (info->rIn + info->rFeedback) * info->vGate;

	rSource = 1.0 / ((1.0 / info->rIn) + (1.0 / info->rFeedback));
	context->rc = rSource * info->c;
	context->exponent = -1.0 / (context->rc * discrete_current_context->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);

	context->vCap = 0;
	context->state = 1;

	node->output = info->options ? 0 : mw_DSS_SCHMITT_OSC__AMP;
}

#define mw_DSS_SINEWAVE__ENABLE (*(node->input[0]))
#define mw_DSS_SINEWAVE__FREQ           (*(node->input[1]))
#define mw_DSS_SINEWAVE__AMPL           (*(node->input[2]))
#define mw_DSS_SINEWAVE__BIAS           (*(node->input[3]))
#define mw_DSS_SINEWAVE__PHASE          (*(node->input[4]))

void mw_dss_sinewave_step(node_description *node)
{
	struct mw_dss_sinewave_context *context = node->context;

	if (mw_DSS_SINEWAVE__ENABLE) {
		node->output = (mw_DSS_SINEWAVE__AMPL / 2.0) * sin(context->phase);

		node->output = node->output + mw_DSS_SINEWAVE__BIAS;
	} else {
		node->output = 0;
	}

	context->phase = fmod((context->phase + ((2.0 * M_PI * mw_DSS_SINEWAVE__FREQ) / discrete_current_context->sample_rate)), 2.0 * M_PI);
}

void mw_dss_sinewave_reset(node_description *node)
{
	struct mw_dss_sinewave_context *context = node->context;
	double start;

	start = (mw_DSS_SINEWAVE__PHASE / 360.0) * (2.0 * M_PI);

	context->phase = fmod(start, 2.0 * M_PI);

	mw_dss_sinewave_step(node);
}

#define mw_DSS_SQUAREWAVE__ENABLE       (*(node->input[0]))
#define mw_DSS_SQUAREWAVE__FREQ (*(node->input[1]))
#define mw_DSS_SQUAREWAVE__AMP          (*(node->input[2]))
#define mw_DSS_SQUAREWAVE__DUTY (*(node->input[3]))
#define mw_DSS_SQUAREWAVE__BIAS (*(node->input[4]))
#define mw_DSS_SQUAREWAVE__PHASE        (*(node->input[5]))

void mw_dss_squarewave_step(node_description *node)
{
	struct mw_dss_squarewave_context *context = node->context;

	context->trigger = ((100 - mw_DSS_SQUAREWAVE__DUTY) / 100) * (2.0 * M_PI);

	if (mw_DSS_SQUAREWAVE__ENABLE) {
		if (context->phase > context->trigger)
			node->output = (mw_DSS_SQUAREWAVE__AMP / 2.0);
		else
			node->output = -(mw_DSS_SQUAREWAVE__AMP / 2.0);

		node->output = node->output + mw_DSS_SQUAREWAVE__BIAS;
	} else {
		node->output = 0;
	}

	context->phase = fmod((context->phase + ((2.0 * M_PI * mw_DSS_SQUAREWAVE__FREQ) / discrete_current_context->sample_rate)), 2.0 * M_PI);
}

void mw_dss_squarewave_reset(node_description *node)
{
	struct mw_dss_squarewave_context *context = node->context;
	double start;

	start = (mw_DSS_SQUAREWAVE__PHASE / 360.0) * (2.0 * M_PI);

	context->phase = fmod(start, 2.0 * M_PI);

	mw_dss_squarewave_step(node);
}

#define mw_DSS_SQUAREWFIX__ENABLE       (*(node->input[0]))
#define mw_DSS_SQUAREWFIX__FREQ (*(node->input[1]))
#define mw_DSS_SQUAREWFIX__AMP          (*(node->input[2]))
#define mw_DSS_SQUAREWFIX__DUTY (*(node->input[3]))
#define mw_DSS_SQUAREWFIX__BIAS (*(node->input[4]))
#define mw_DSS_SQUAREWFIX__PHASE        (*(node->input[5]))

void mw_dss_squarewfix_step(node_description *node)
{
	struct mw_dss_squarewfix_context *context = node->context;

	context->tLeft -= context->sampleStep;

	while (context->tLeft <= 0) {
		context->flip_flop = context->flip_flop ? 0 : 1;
		context->tLeft += context->flip_flop ? context->tOn : context->tOff;
	}

	if (mw_DSS_SQUAREWFIX__ENABLE) {

		context->tOff = 1.0 / mw_DSS_SQUAREWFIX__FREQ;
		context->tOn = context->tOff * (mw_DSS_SQUAREWFIX__DUTY / 100.0);
		context->tOff -= context->tOn;

		node->output = (context->flip_flop ? mw_DSS_SQUAREWFIX__AMP / 2.0 : -(mw_DSS_SQUAREWFIX__AMP / 2.0)) + mw_DSS_SQUAREWFIX__BIAS;
	} else {
		node->output = 0;
	}
}

void mw_dss_squarewfix_reset(node_description *node)
{
	struct mw_dss_squarewfix_context *context = node->context;

	context->sampleStep = 1.0 / discrete_current_context->sample_rate;
	context->flip_flop = 1;

	context->tOff = 1.0 / mw_DSS_SQUAREWFIX__FREQ;
	context->tLeft = mw_DSS_SQUAREWFIX__PHASE / 360.0;
	context->tLeft = context->tLeft - (int)context->tLeft;
	context->tLeft = (context->tLeft < 0) ? 1.0 + context->tLeft : context->tLeft;
	context->tLeft *= context->tOff;
	context->tOn = context->tOff * (mw_DSS_SQUAREWFIX__DUTY / 100.0);
	context->tOff -= context->tOn;

	context->tLeft = -context->tLeft;

	while (context->tLeft <= 0) {
		context->flip_flop = context->flip_flop ? 0 : 1;
		context->tLeft += context->flip_flop ? context->tOn : context->tOff;
	}

	mw_dss_squarewfix_step(node);
}

#define mw_DSS_SQUAREWAVE2__ENABLE      (*(node->input[0]))
#define mw_DSS_SQUAREWAVE2__AMP (*(node->input[1]))
#define mw_DSS_SQUAREWAVE2__T_OFF       (*(node->input[2]))
#define mw_DSS_SQUAREWAVE2__T_ON        (*(node->input[3]))
#define mw_DSS_SQUAREWAVE2__BIAS        (*(node->input[4]))
#define mw_DSS_SQUAREWAVE2__SHIFT       (*(node->input[5]))

void mw_dss_squarewave2_step(node_description *node)
{
	struct mw_dss_squarewave_context *context = node->context;
	double newphase;

	if (mw_DSS_SQUAREWAVE2__ENABLE) {

		context->trigger = (mw_DSS_SQUAREWAVE2__T_OFF / (mw_DSS_SQUAREWAVE2__T_OFF + mw_DSS_SQUAREWAVE2__T_ON)) * (2.0 * M_PI);

		newphase = context->phase + ((2.0 * M_PI) / ((mw_DSS_SQUAREWAVE2__T_OFF + mw_DSS_SQUAREWAVE2__T_ON) * discrete_current_context->sample_rate));

		context->phase = fmod(newphase, 2.0 * M_PI);

		if (context->phase > context->trigger)
			node->output = (mw_DSS_SQUAREWAVE2__AMP / 2.0);
		else
			node->output = -(mw_DSS_SQUAREWAVE2__AMP / 2.0);

		node->output = node->output + mw_DSS_SQUAREWAVE2__BIAS;
	} else {
		node->output = 0;
	}
}

void mw_dss_squarewave2_reset(node_description *node)
{
	struct mw_dss_squarewave_context *context = node->context;
	double start;

	if ((mw_DSS_SQUAREWAVE2__T_OFF + mw_DSS_SQUAREWAVE2__T_ON) != 0.0)
		start = (mw_DSS_SQUAREWAVE2__SHIFT / (mw_DSS_SQUAREWAVE2__T_OFF + mw_DSS_SQUAREWAVE2__T_ON)) * (2.0 * M_PI);
	else
		start = 0.0;

	context->phase = fmod(start, 2.0 * M_PI);

	mw_dss_squarewave2_step(node);
}

#define mw_DSS_INVERTER_OSC__ENABLE     (*(node->input[0]))
#define mw_DSS_INVERTER_OSC__MOD                (*(node->input[1]))
#define mw_DSS_INVERTER_OSC__RC         (*(node->input[2]))
#define mw_DSS_INVERTER_OSC__RP         (*(node->input[3]))
#define mw_DSS_INVERTER_OSC__C                  (*(node->input[4]))
#define mw_DSS_INVERTER_OSC__R2         (*(node->input[5]))

static double mw_dss_inverter_tf(node_description *node, double x)
{
	const mw_discrete_inverter_osc_desc *info = node->custom;
	struct mw_dss_inverter_osc_context *context = node->context;

	x = x / info->vB;
	if (x > 0)
		return info->vB * exp(-context->tf_a * pow(x, context->tf_b));
	else
		return info->vB;
}

void mw_dss_inverter_osc_step(node_description *node)
{
	struct mw_dss_inverter_osc_context *context = node->context;
	const mw_discrete_inverter_osc_desc *info = node->custom;
	double diff, vG1, vG2, vG3, vI;
	double vMix, rMix;

	vI = context->vCap + context->vG2_old;
	switch (info->options & MW_DISC_OSC_INVERTER_TYPE_MASK) {
	case MW_DISC_OSC_INVERTER_IS_TYPE1:
	case MW_DISC_OSC_INVERTER_IS_TYPE3:
		vG1 = mw_dss_inverter_tf(node, vI);
		vG2 = mw_dss_inverter_tf(node, vG1);
		vG3 = mw_dss_inverter_tf(node, vG2);
		break;
	case MW_DISC_OSC_INVERTER_IS_TYPE2:
		vG1 = 0;
		vG3 = mw_dss_inverter_tf(node, vI);
		vG2 = mw_dss_inverter_tf(node, vG3);
		break;
	case MW_DISC_OSC_INVERTER_IS_TYPE4:
		vI = MIN(mw_DSS_INVERTER_OSC__ENABLE, vI + 0.7);
		vG1 = 0;
		vG3 = mw_dss_inverter_tf(node, vI);
		vG2 = mw_dss_inverter_tf(node, vG3);
		break;
	case MW_DISC_OSC_INVERTER_IS_TYPE5:
		vI = MAX(mw_DSS_INVERTER_OSC__ENABLE, vI - 0.7);
		vG1 = 0;
		vG3 = mw_dss_inverter_tf(node, vI);
		vG2 = mw_dss_inverter_tf(node, vG3);
		break;
	default:
		fatalerror("MW_DISCRETE_INVERTER_OSC - Wrong type on NODE_%02d", node->node - NODE_00);
	}
	switch (info->options & MW_DISC_OSC_INVERTER_TYPE_MASK) {
	case MW_DISC_OSC_INVERTER_IS_TYPE1:
	case MW_DISC_OSC_INVERTER_IS_TYPE2:
	case MW_DISC_OSC_INVERTER_IS_TYPE3:
		if ((info->clamp >= 0.0) && ((vI < -info->clamp) || (vI > info->vB + info->clamp))) {
			vI = MAX(vI, (-info->clamp));
			vI = MIN(vI, info->vB + info->clamp);
			diff = vG3 * (context->Rp / (context->Rp + context->R1))
				- (context->vCap + vG2)
				+ vI * (context->R1 / (context->Rp + context->R1));
			diff = diff - diff * exp(context->wc * discrete_current_context->sample_time);
		} else {
			diff = vG3 - (context->vCap + vG2);
			diff = diff - diff * exp(context->w * discrete_current_context->sample_time);
		}
		break;
	case MW_DISC_OSC_INVERTER_IS_TYPE4:
		if ((info->clamp >= 0.0) && ((vI < -info->clamp) || (vI > info->vB + info->clamp))) {
			vI = MAX(vI, (-info->clamp));
			vI = MIN(vI, info->vB + info->clamp);
		}

		rMix = (context->R1 * context->R2) / (context->R1 + context->R2);
		vMix = rMix * ((vG3 - vG2) / context->R1 + (mw_DSS_INVERTER_OSC__MOD - vG2) / context->R2);
		if (vMix < (vI - vG2 - 0.7)) {
			rMix = 1.0 / rMix + 1.0 / context->Rp;
			rMix = 1.0 / rMix;
			vMix = rMix * ((vG3 - vG2) / context->R1 + (mw_DSS_INVERTER_OSC__MOD - vG2) / context->R2 + (vI - 0.7 - vG2) / context->Rp);
		}
		diff = vMix - context->vCap;
		diff = diff - diff * exp(-discrete_current_context->sample_time / (context->C * rMix));
		break;
	case MW_DISC_OSC_INVERTER_IS_TYPE5:
		if ((info->clamp >= 0.0) && ((vI < -info->clamp) || (vI > info->vB + info->clamp))) {
			vI = MAX(vI, (-info->clamp));
			vI = MIN(vI, info->vB + info->clamp);
		}

		rMix = (context->R1 * context->R2) / (context->R1 + context->R2);
		vMix = rMix * ((vG3 - vG2) / context->R1 + (mw_DSS_INVERTER_OSC__MOD - vG2) / context->R2);
		if (vMix > (vI - vG2 + 0.7)) {
			rMix = 1.0 / rMix + 1.0 / context->Rp;
			rMix = 1.0 / rMix;
			vMix = rMix * ((vG3 - vG2) / context->R1 + (mw_DSS_INVERTER_OSC__MOD - vG2) / context->R2 + (vI + 0.7 - vG2) / context->Rp);
		}
		diff = vMix - context->vCap;
		diff = diff - diff * exp(-discrete_current_context->sample_time / (context->C * rMix));
		break;
	default:
		fatalerror("MW_DISCRETE_INVERTER_OSC - Wrong type on NODE_%02d", node->node - NODE_00);
	}
	context->vCap += diff;
	context->vG2_old = vG2;
	if ((info->options & MW_DISC_OSC_INVERTER_TYPE_MASK) == MW_DISC_OSC_INVERTER_IS_TYPE3)
		node->output = vG1;
	else
		node->output = vG3;
	if (info->options & MW_DISC_OSC_INVERTER_OUT_IS_LOGIC)
		node->output = (node->output > info->vInFall);
}

void mw_dss_inverter_osc_reset(node_description *node)
{
	struct mw_dss_inverter_osc_context *context = node->context;
	const mw_discrete_inverter_osc_desc *info = node->custom;

	context->w = -1.0 / (mw_DSS_INVERTER_OSC__RC * mw_DSS_INVERTER_OSC__C);
	context->wc = -1.0 / ((mw_DSS_INVERTER_OSC__RC * mw_DSS_INVERTER_OSC__RP) / (mw_DSS_INVERTER_OSC__RP + mw_DSS_INVERTER_OSC__RC) * mw_DSS_INVERTER_OSC__C);
	node->output = 0;
	context->vCap = 0;
	context->vG2_old = 0;
	context->Rp = mw_DSS_INVERTER_OSC__RP;
	context->R1 = mw_DSS_INVERTER_OSC__RC;
	context->R2 = mw_DSS_INVERTER_OSC__R2;
	context->C = mw_DSS_INVERTER_OSC__C;
	context->tf_b = (log(0.0 - log(info->vOutLow / info->vB)) - log(0.0 - log((info->vOutHigh / info->vB)))) / log(info->vInRise / info->vInFall);
	context->tf_a = log(0.0 - log(info->vOutLow / info->vB)) - context->tf_b * log(info->vInRise / info->vB);
	context->tf_a = exp(context->tf_a);
}

#define mw_DSS_TRIANGLEWAVE__ENABLE     (*(node->input[0]))
#define mw_DSS_TRIANGLEWAVE__FREQ               (*(node->input[1]))
#define mw_DSS_TRIANGLEWAVE__AMP                (*(node->input[2]))
#define mw_DSS_TRIANGLEWAVE__BIAS               (*(node->input[3]))
#define mw_DSS_TRIANGLEWAVE__PHASE              (*(node->input[4]))

void mw_dss_trianglewave_step(node_description *node)
{
	struct mw_dss_trianglewave_context *context = node->context;

	if (mw_DSS_TRIANGLEWAVE__ENABLE) {
		node->output = context->phase < M_PI ? (mw_DSS_TRIANGLEWAVE__AMP * (context->phase / (M_PI / 2.0) - 1.0)) / 2.0 :
			(mw_DSS_TRIANGLEWAVE__AMP * (3.0 - context->phase / (M_PI / 2.0))) / 2.0;

		node->output = node->output + mw_DSS_TRIANGLEWAVE__BIAS;
	} else {
		node->output = 0;
	}

	context->phase = fmod((context->phase + ((2.0 * M_PI * mw_DSS_TRIANGLEWAVE__FREQ) / discrete_current_context->sample_rate)), 2.0 * M_PI);
}

void mw_dss_trianglewave_reset(node_description *node)
{
	struct mw_dss_trianglewave_context *context = node->context;
	double start;

	start = (mw_DSS_TRIANGLEWAVE__PHASE / 360.0) * (2.0 * M_PI);

	context->phase = fmod(start, 2.0 * M_PI);

	mw_dss_trianglewave_step(node);
}

#define mw_DSS_ADSR__ENABLE     (*(node->input[0]))

void mw_dss_adsrenv_step(node_description *node)
{

	if (mw_DSS_ADSR__ENABLE) {
		node->output = 0;
	} else {
		node->output = 0;
	}
}

void mw_dss_adsrenv_reset(node_description *node)
{
	mw_dss_adsrenv_step(node);
}

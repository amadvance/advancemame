// license:BSD-3-Clause
// copyright-holders:Keith Wilkins, Derrick Renaud, Couriersud
/*
 *  (c) K.Wilkins 2000
 */


struct mw_dss_filter1_context {
	double x1;
	double y1;
	double a1;
	double b0, b1;
};

struct mw_dss_filter2_context {
	double x1, x2;
	double y1, y2;
	double a1, a2;
	double b0, b1, b2;
};

struct mw_dst_op_amp_filt_context {
	int type;
	int is_norton;
	double vRef;
	double vP;
	double vN;
	double rTotal;
	double iFixed;
	double exponentC1;
	double exponentC2;
	double exponentC3;
	double rRatio;
	double vC1;
	double vC1b;
	double vC2;
	double vC3;
	double gain;
	double x1, x2;
	double y1, y2;
	double a1, a2;
	double b0, b1, b2;
};

struct mw_dst_rcdisc_context {
	int state;
	double t;
	double f;
	double R1;
	double R2;
	double R3;
	double C;
	double vCap;
	double vCE;
	double exponent0;
	double exponent1;
};

struct mw_dst_rcdisc4_context {
	int type;
	double max_out;
	double vC1;
	double v[2];
	double exp[2];
};

struct mw_dst_rcfilter_context {
	double exponent;
	double vCap;
};

#define mw_DST_CRFILTER__ENABLE         (*(node->input[0]))
#define mw_DST_CRFILTER__IN             (*(node->input[1]))
#define mw_DST_CRFILTER__R                      (*(node->input[2]))
#define mw_DST_CRFILTER__C                      (*(node->input[3]))
#define mw_DST_CRFILTER__VREF           (*(node->input[4]))

void mw_dst_crfilter_step(node_description *node)
{
	struct mw_dst_rcfilter_context *context = node->context;

	if (mw_DST_CRFILTER__ENABLE) {
		node->output = mw_DST_CRFILTER__IN - context->vCap;
		context->vCap += ((mw_DST_CRFILTER__IN - mw_DST_CRFILTER__VREF) - context->vCap) * context->exponent;
	} else {
		node->output = 0;
	}
}

void mw_dst_crfilter_reset(node_description *node)
{
	struct mw_dst_rcfilter_context *context = node->context;

	context->exponent = -1.0 / (mw_DST_CRFILTER__R * mw_DST_CRFILTER__C * discrete_current_context->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);
	context->vCap = 0;
	node->output = mw_DST_CRFILTER__IN;
}

#define mw_DST_FILTER1__ENABLE  (*(node->input[0]))
#define mw_DST_FILTER1__IN              (*(node->input[1]))
#define mw_DST_FILTER1__FREQ    (*(node->input[2]))
#define mw_DST_FILTER1__TYPE    (*(node->input[3]))

static void mw_calculate_filter1_coefficients(double fc, double type,
	double *a1, double *b0, double *b1)
{
	double den, w, two_over_T;

	w = discrete_current_context->sample_rate * 2.0 * tan(M_PI * fc / discrete_current_context->sample_rate);
	two_over_T = 2.0 * discrete_current_context->sample_rate;

	den = w + two_over_T;
	*a1 = (w - two_over_T) / den;
	if (type == MW_DISC_FILTER_LOWPASS) {
		*b0 = *b1 = w / den;
	} else if (type == MW_DISC_FILTER_HIGHPASS) {
		*b0 = two_over_T / den;
		*b1 = -(*b0);
	} else {
		discrete_log("mw_calculate_filter1_coefficients() - Invalid filter type for 1st order filter.");
	}
}

void mw_dst_filter1_step(node_description *node)
{
	struct mw_dss_filter1_context *context = node->context;
	double gain = 1.0;

	if (mw_DST_FILTER1__ENABLE == 0.0) {
		gain = 0.0;
	}

	node->output = -context->a1 * context->y1 + context->b0 * gain * mw_DST_FILTER1__IN + context->b1 * context->x1;

	context->x1 = gain * mw_DST_FILTER1__IN;
	context->y1 = node->output;
}

void mw_dst_filter1_reset(node_description *node)
{
	struct mw_dss_filter1_context *context = node->context;

	mw_calculate_filter1_coefficients(mw_DST_FILTER1__FREQ, mw_DST_FILTER1__TYPE, &context->a1, &context->b0, &context->b1);
	node->output = 0;
}

#define mw_DST_FILTER2__ENABLE  (*(node->input[0]))
#define mw_DST_FILTER2__IN              (*(node->input[1]))
#define mw_DST_FILTER2__FREQ    (*(node->input[2]))
#define mw_DST_FILTER2__DAMP    (*(node->input[3]))
#define mw_DST_FILTER2__TYPE    (*(node->input[4]))

static void mw_calculate_filter2_coefficients(double fc, double d, double type,
	double *a1, double *a2,
	double *b0, double *b1, double *b2)
{
	double w;
	double w_squared;
	double den;
	double two_over_T = 2 * discrete_current_context->sample_rate;
	double two_over_T_squared = two_over_T * two_over_T;

	w = discrete_current_context->sample_rate * 2.0 * tan(M_PI * fc / discrete_current_context->sample_rate);
	w_squared = w * w;

	den = two_over_T_squared + d * w * two_over_T + w_squared;

	*a1 = 2.0 * (-two_over_T_squared + w_squared) / den;
	*a2 = (two_over_T_squared - d * w * two_over_T + w_squared) / den;

	if (type == MW_DISC_FILTER_LOWPASS) {
		*b0 = *b2 = w_squared / den;
		*b1 = 2.0 * (*b0);
	} else if (type == MW_DISC_FILTER_BANDPASS) {
		*b0 = d * w * two_over_T / den;
		*b1 = 0.0;
		*b2 = -(*b0);
	} else if (type == MW_DISC_FILTER_HIGHPASS) {
		*b0 = *b2 = two_over_T_squared / den;
		*b1 = -2.0 * (*b0);
	} else {
		discrete_log("mw_calculate_filter2_coefficients() - Invalid filter type for 2nd order filter.");
	}
}

void mw_dst_filter2_step(node_description *node)
{
	struct mw_dss_filter2_context *context = node->context;
	double gain = 1.0;

	if (mw_DST_FILTER2__ENABLE == 0.0) {
		gain = 0.0;
	}

	node->output = -context->a1 * context->y1 - context->a2 * context->y2 +
		context->b0 * gain * mw_DST_FILTER2__IN + context->b1 * context->x1 + context->b2 * context->x2;

	context->x2 = context->x1;
	context->x1 = gain * mw_DST_FILTER2__IN;
	context->y2 = context->y1;
	context->y1 = node->output;
}

void mw_dst_filter2_reset(node_description *node)
{
	struct mw_dss_filter2_context *context = node->context;

	mw_calculate_filter2_coefficients(mw_DST_FILTER2__FREQ, mw_DST_FILTER2__DAMP, mw_DST_FILTER2__TYPE,
		&context->a1, &context->a2,
		&context->b0, &context->b1, &context->b2);
	node->output = 0;
}

#define mw_DST_OP_AMP_FILT__ENABLE      (*(node->input[0]))
#define mw_DST_OP_AMP_FILT__INP1        (*(node->input[1]))
#define mw_DST_OP_AMP_FILT__INP2        (*(node->input[2]))
#define mw_DST_OP_AMP_FILT__TYPE        (*(node->input[3]))

void mw_dst_op_amp_filt_step(node_description *node)
{
	const mw_discrete_op_amp_filt_info *info = node->custom;
	struct mw_dst_op_amp_filt_context *context = node->context;

	double i, v = 0;

	if (mw_DST_OP_AMP_FILT__ENABLE) {
		if (context->is_norton) {
			v = mw_DST_OP_AMP_FILT__INP1 - MW_OP_AMP_NORTON_VBE;
			if (v < 0) v = 0;
		} else {

			i = context->iFixed;
			i += (mw_DST_OP_AMP_FILT__INP1 - context->vRef) / info->r1;
			if (info->r2 != 0)
				i += (mw_DST_OP_AMP_FILT__INP2 - context->vRef) / info->r2;
			v = i * context->rTotal;
		}

		switch (context->type) {
		case MW_DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
			context->vC1 += (v - context->vC1) * context->exponentC1;
			node->output = context->vC1 * context->gain + info->vRef;
			break;

		case MW_DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
			node->output = (v - context->vC1) * context->gain + info->vRef;
			context->vC1 += (v - context->vC1) * context->exponentC1;
			break;

		case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
			node->output = (v - context->vC2);
			context->vC2 += (v - context->vC2) * context->exponentC2;
			context->vC1 += (node->output - context->vC1) * context->exponentC1;
			node->output = context->vC1 * context->gain + info->vRef;
			break;

		case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | MW_DISC_OP_AMP_IS_NORTON:
			context->vC1 += (v - context->vC1) * context->exponentC1;
			context->vC2 += (context->vC1 - context->vC2) * context->exponentC2;
			v = context->vC2;
			node->output = v - context->vC3;
			context->vC3 += (v - context->vC3) * context->exponentC3;
			i = node->output / context->rTotal;
			node->output = (context->iFixed - i) * info->rF;
			break;

		case MW_DISC_OP_AMP_FILTER_IS_HIGH_PASS_0 | MW_DISC_OP_AMP_IS_NORTON:
			node->output = v - context->vC1;
			context->vC1 += (v - context->vC1) * context->exponentC1;
			i = node->output / context->rTotal;
			node->output = (context->iFixed - i) * info->rF;
			break;

		case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
		case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M | MW_DISC_OP_AMP_IS_NORTON:
			node->output = -context->a1 * context->y1 - context->a2 * context->y2 +
				context->b0 * v + context->b1 * context->x1 + context->b2 * context->x2 +
				context->vRef;
			context->x2 = context->x1;
			context->x1 = v;
			context->y2 = context->y1;
			break;
		}

		if (node->output > context->vP) node->output = context->vP;
		if (node->output < context->vN) node->output = context->vN;
		context->y1 = node->output - context->vRef;
	} else
		node->output = 0;

}

void mw_dst_op_amp_filt_reset(node_description *node)
{
	const mw_discrete_op_amp_filt_info *info = node->custom;
	struct mw_dst_op_amp_filt_context *context = node->context;

	context->type = (int)mw_DST_OP_AMP_FILT__TYPE & MW_DISC_OP_AMP_FILTER_TYPE_MASK;
	context->is_norton = (int)mw_DST_OP_AMP_FILT__TYPE & MW_DISC_OP_AMP_IS_NORTON;

	if (context->is_norton) {
		context->vRef = 0;
		context->rTotal = info->r1;
		if (context->type == (MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | MW_DISC_OP_AMP_IS_NORTON))
			context->rTotal += info->r2 + info->r3;

		context->iFixed = info->r4 ? (info->vP - MW_OP_AMP_NORTON_VBE) / info->r4 : 0;

		context->vP = info->vP - MW_OP_AMP_NORTON_VBE;
		context->vN = info->vN;
	} else {
		context->vRef = info->vRef;

		context->vP = info->vP - MW_OP_AMP_VP_RAIL_OFFSET;
		context->vN = info->vN;

		context->rTotal = 1.0 / info->r1;
		if (info->r2 != 0) context->rTotal += 1.0 / info->r2;
		if (info->r3 != 0) context->rTotal += 1.0 / info->r3;
		context->rTotal = 1.0 / context->rTotal;

		context->iFixed = 0;

		context->rRatio = info->rF / (context->rTotal + info->rF);
		context->gain = -info->rF / context->rTotal;
	}

	switch (context->type) {
	case MW_DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
		context->exponentC1 = -1.0 / (info->rF * info->c1 * discrete_current_context->sample_rate);
		context->exponentC1 = 1.0 - exp(context->exponentC1);
		context->exponentC2 = 0;
		break;
	case MW_DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
		context->exponentC1 = -1.0 / (context->rTotal * info->c1 * discrete_current_context->sample_rate);
		context->exponentC1 = 1.0 - exp(context->exponentC1);
		context->exponentC2 = 0;
		break;
	case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
		context->exponentC1 = -1.0 / (info->rF * info->c1 * discrete_current_context->sample_rate);
		context->exponentC1 = 1.0 - exp(context->exponentC1);
		context->exponentC2 = -1.0 / (context->rTotal * info->c2 * discrete_current_context->sample_rate);
		context->exponentC2 = 1.0 - exp(context->exponentC2);
		break;
	case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M | MW_DISC_OP_AMP_IS_NORTON:
		context->rTotal = info->r2 ? 1.0 / (1.0 / info->r1 + 1.0 / info->r2) : info->r1;
	case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
	{
		double fc = 1.0 / (2 * M_PI * sqrt(context->rTotal * info->rF * info->c1 * info->c2));
		double d = (info->c1 + info->c2) / sqrt(info->rF / context->rTotal * info->c1 * info->c2);
		double gain = -info->rF / context->rTotal * info->c2 / (info->c1 + info->c2);

		mw_calculate_filter2_coefficients(fc, d, MW_DISC_FILTER_BANDPASS,
			&context->a1, &context->a2,
			&context->b0, &context->b1, &context->b2);
		context->b0 *= gain;
		context->b1 *= gain;
		context->b2 *= gain;

		if (context->is_norton)
			context->vRef = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r3 * info->rF;
		else
			context->vRef = info->vRef;

		break;
	}
	case MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | MW_DISC_OP_AMP_IS_NORTON:
		context->exponentC1 = -1.0 / ((1.0 / (1.0 / info->r1 + 1.0 / (info->r2 + info->r3 + info->r4))) * info->c1 * discrete_current_context->sample_rate);
		context->exponentC1 = 1.0 - exp(context->exponentC1);
		context->exponentC2 = -1.0 / ((1.0 / (1.0 / (info->r1 + info->r2) + 1.0 / (info->r3 + info->r4))) * info->c2 * discrete_current_context->sample_rate);
		context->exponentC2 = 1.0 - exp(context->exponentC2);
		context->exponentC3 = -1.0 / ((info->r1 + info->r2 + info->r3 + info->r4) * info->c3 * discrete_current_context->sample_rate);
		context->exponentC3 = 1.0 - exp(context->exponentC3);
		break;
	case MW_DISC_OP_AMP_FILTER_IS_HIGH_PASS_0 | MW_DISC_OP_AMP_IS_NORTON:
		context->exponentC1 = -1.0 / (info->r1 * info->c1 * discrete_current_context->sample_rate);
		context->exponentC1 = 1.0 - exp(context->exponentC1);
		break;
	}

	context->vC1 = 0;
	context->vC1b = 0;
	context->vC2 = 0;
	context->vC3 = 0;

	node->output = info->vRef;
}

#define mw_DST_RCDISC__ENABLE   (*(node->input[0]))
#define mw_DST_RCDISC__IN               (*(node->input[1]))
#define mw_DST_RCDISC__R                (*(node->input[2]))
#define mw_DST_RCDISC__C                (*(node->input[3]))

void mw_dst_rcdisc_step(node_description *node)
{
	struct mw_dst_rcdisc_context *context = node->context;

	switch (context->state) {
	case 0:
		if (mw_DST_RCDISC__ENABLE) {
			context->state = 1;
			context->t = 0;
		}
		node->output = 0;
		break;

	case 1:
		if (mw_DST_RCDISC__ENABLE) {
			node->output = mw_DST_RCDISC__IN * exp(context->t / context->exponent0);
			context->t += discrete_current_context->sample_time;
		} else {
			context->state = 0;
		}
	}
}

void mw_dst_rcdisc_reset(node_description *node)
{
	struct mw_dst_rcdisc_context *context = node->context;

	node->output = 0;

	context->state = 0;
	context->t = 0;
	context->exponent0 = -1.0 * mw_DST_RCDISC__R * mw_DST_RCDISC__C;
}

#define mw_DST_RCDISC2__ENABLE  (*(node->input[0]))
#define mw_DST_RCDISC2__IN0     (*(node->input[1]))
#define mw_DST_RCDISC2__R0              (*(node->input[2]))
#define mw_DST_RCDISC2__IN1     (*(node->input[3]))
#define mw_DST_RCDISC2__R1              (*(node->input[4]))
#define mw_DST_RCDISC2__C               (*(node->input[5]))

void mw_dst_rcdisc2_step(node_description *node)
{
	double diff;
	struct mw_dst_rcdisc_context *context = node->context;

	diff = ((mw_DST_RCDISC2__ENABLE == 0) ? mw_DST_RCDISC2__IN0 : mw_DST_RCDISC2__IN1) - node->output;
	diff = diff - (diff * exp(discrete_current_context->sample_time / ((mw_DST_RCDISC2__ENABLE == 0) ? context->exponent0 : context->exponent1)));
	node->output += diff;
}

void mw_dst_rcdisc2_reset(node_description *node)
{
	struct mw_dst_rcdisc_context *context = node->context;

	node->output = 0;

	context->state = 0;
	context->t = 0;
	context->exponent0 = -1.0 * mw_DST_RCDISC2__R0 * mw_DST_RCDISC2__C;
	context->exponent1 = -1.0 * mw_DST_RCDISC2__R1 * mw_DST_RCDISC2__C;
}

#define mw_DST_RCDISC3__ENABLE  (*(node->input[0]))
#define mw_DST_RCDISC3__IN              (*(node->input[1]))
#define mw_DST_RCDISC3__R1              (*(node->input[2]))
#define mw_DST_RCDISC3__R2              (*(node->input[3]))
#define mw_DST_RCDISC3__C               (*(node->input[4]))

void mw_dst_rcdisc3_step(node_description *node)
{
	double diff;
	struct mw_dst_rcdisc_context *context = node->context;

	if (mw_DST_RCDISC3__ENABLE) {
		diff = mw_DST_RCDISC3__IN - node->output;
		if (diff > 0) {
			diff = diff - (diff * exp(discrete_current_context->sample_time / context->exponent0));
		} else if (diff < 0) {
			if (diff < -0.5)
				diff = diff - (diff * exp(discrete_current_context->sample_time / context->exponent1));
			else
				diff = diff - (diff * exp(discrete_current_context->sample_time / context->exponent0));
		}
		node->output += diff;
	} else {
		node->output = 0;
	}
}

void mw_dst_rcdisc3_reset(node_description *node)
{
	struct mw_dst_rcdisc_context *context = node->context;

	node->output = 0;

	context->state = 0;
	context->t = 0;
	context->exponent0 = -1.0 * mw_DST_RCDISC3__R1 * mw_DST_RCDISC3__C;
	context->exponent1 = -1.0 * (mw_DST_RCDISC3__R1 * mw_DST_RCDISC3__R2) / (mw_DST_RCDISC3__R1 + mw_DST_RCDISC3__R2) * mw_DST_RCDISC3__C;
}

#define mw_DST_RCDISC4__ENABLE  (*(node->input[0]))
#define mw_DST_RCDISC4__IN              (*(node->input[1]))
#define mw_DST_RCDISC4__R1              (*(node->input[2]))
#define mw_DST_RCDISC4__R2              (*(node->input[3]))
#define mw_DST_RCDISC4__R3              (*(node->input[4]))
#define mw_DST_RCDISC4__C1              (*(node->input[5]))
#define mw_DST_RCDISC4__VP              (*(node->input[6]))
#define mw_DST_RCDISC4__TYPE    (*(node->input[7]))

void mw_dst_rcdisc4_step(node_description *node)
{
	struct mw_dst_rcdisc4_context *context = node->context;
	int inp1 = (mw_DST_RCDISC4__IN == 0) ? 0 : 1;

	if (mw_DST_RCDISC4__ENABLE == 0) {
		node->output = 0;
		return;
	}

	switch (context->type) {
	case 1:
	case 3:
		context->vC1 += ((context->v[inp1] - context->vC1) * context->exp[inp1]);
		node->output = context->vC1;
		break;
	}

	if (node->output > context->max_out) node->output = context->max_out;
	if (node->output < 0) node->output = 0;
}

void mw_dst_rcdisc4_reset(node_description *node)
{
	struct mw_dst_rcdisc4_context *context = node->context;
	double v, i, r, rT;

	context->type = 0;

	if (mw_DST_RCDISC4__R1 <= 0 || mw_DST_RCDISC4__R2 <= 0 || mw_DST_RCDISC4__C1 <= 0 || (mw_DST_RCDISC4__R3 <= 0 && context->type == 1)) {
		discrete_log("Invalid component values in NODE_%d.\n", node->node - NODE_00);
		return;
	}
	if (mw_DST_RCDISC4__VP < 3) {
		discrete_log("vP must be >= 3V in NODE_%d.\n", node->node - NODE_00);
		return;
	}
	if (mw_DST_RCDISC4__TYPE < 1 || mw_DST_RCDISC4__TYPE > 3) {
		discrete_log("Invalid circuit type in NODE_%d.\n", node->node - NODE_00);
		return;
	}

	context->vC1 = 0;

	context->type = (int)mw_DST_RCDISC4__TYPE;

	context->max_out = mw_DST_RCDISC4__VP - MW_OP_AMP_VP_RAIL_OFFSET;

	switch (context->type) {
	case 1:

		v = mw_DST_RCDISC4__VP - .5;

		r = 1.0 / (1.0 / mw_DST_RCDISC4__R1 + 1.0 / mw_DST_RCDISC4__R3);
		rT = mw_DST_RCDISC4__R2 + r;
		i = v / rT;
		context->v[1] = i * r + .5;
		rT = 1.0 / (1.0 / mw_DST_RCDISC4__R2 + 1.0 / r);
		context->exp[1] = -1.0 / (rT * mw_DST_RCDISC4__C1 * discrete_current_context->sample_rate);
		context->exp[1] = 1.0 - exp(context->exp[1]);

		rT = mw_DST_RCDISC4__R2 + mw_DST_RCDISC4__R3;
		i = v / rT;
		context->v[0] = i * mw_DST_RCDISC4__R3 + .5;
		rT = 1.0 / (1.0 / mw_DST_RCDISC4__R2 + 1.0 / mw_DST_RCDISC4__R3);
		context->exp[0] = -1.0 / (rT * mw_DST_RCDISC4__C1 * discrete_current_context->sample_rate);
		context->exp[0] = 1.0 - exp(context->exp[0]);
		break;

	case 3:

		r = 500.0 + mw_DST_RCDISC4__R1;
		context->v[1] = mw_DST_RCDISC4__R2 / (r + mw_DST_RCDISC4__R2) * (5.0 - 0.5);
		rT = 1.0 / (1.0 / r + 1.0 / mw_DST_RCDISC4__R2);
		context->exp[1] = -1.0 / (rT * mw_DST_RCDISC4__C1 * discrete_current_context->sample_rate);
		context->exp[1] = 1.0 - exp(context->exp[1]);

		context->v[0] = 0;
		context->exp[0] = -1.0 / (mw_DST_RCDISC4__R2 * mw_DST_RCDISC4__C1 * discrete_current_context->sample_rate);
		context->exp[0] = 1.0 - exp(context->exp[0]);
		break;
	}
}

#define mw_DST_RCDISC5__ENABLE  (*(node->input[0]))
#define mw_DST_RCDISC5__IN              (*(node->input[1]))
#define mw_DST_RCDISC5__R               (*(node->input[2]))
#define mw_DST_RCDISC5__C               (*(node->input[3]))

void mw_dst_rcdisc5_step(node_description *node)
{
	double diff, u;
	struct mw_dst_rcdisc_context *context = node->context;

	if (mw_DST_RCDISC5__ENABLE) {
		u = mw_DST_RCDISC5__IN - 0.7;
		if (u < 0)
			u = 0;

		diff = u - node->output;

		if (diff < 0)

			diff = -node->output + (node->output * exp(discrete_current_context->sample_time / context->exponent0));
		node->output += diff;
	} else {
		node->output = 0;
	}
}

void mw_dst_rcdisc5_reset(node_description *node)
{
	struct mw_dst_rcdisc_context *context = node->context;

	node->output = 0;

	context->state = 0;
	context->t = 0;
	context->exponent0 = -1.0 * mw_DST_RCDISC5__R * mw_DST_RCDISC5__C;
}

#define mw_DST_RCINTEGRATE__ENABLE      (*(node->input[0]))
#define mw_DST_RCINTEGRATE__IN1 (*(node->input[1]))
#define mw_DST_RCINTEGRATE__R1          (*(node->input[2]))
#define mw_DST_RCINTEGRATE__R2          (*(node->input[3]))
#define mw_DST_RCINTEGRATE__R3          (*(node->input[4]))
#define mw_DST_RCINTEGRATE__C           (*(node->input[5]))
#define mw_DST_RCINTEGRATE__VP          (*(node->input[6]))
#define mw_DST_RCINTEGRATE__TYPE        (*(node->input[7]))

#define IES             7e-15
#define ALPHAT  0.99
#define KT              0.026
#define EM_IC(x) (ALPHAT * IES * exp((x) / KT - 1.0))

void mw_dst_rcintegrate_step(node_description *node)
{
	double diff, u, iQ, iQc, iC, RG, vE;
	struct mw_dst_rcdisc_context *context = node->context;
	double dt, vP;

	if (mw_DST_RCINTEGRATE__ENABLE) {
		u = mw_DST_RCINTEGRATE__IN1;
		vP = mw_DST_RCINTEGRATE__VP;
		dt = discrete_current_context->sample_time;
		if (u - 0.7 < context->vCap * context->R2 / (context->R1 + context->R2)) {

			diff = 0 - context->vCap;
			iC = 0.0 - context->C / context->exponent1 * diff * exp(dt / context->exponent1);
			diff = diff - (diff * exp(dt / context->exponent1));
			context->vCap += diff;
			iQ = 0;
			vE = context->vCap * context->R2 / (context->R1 + context->R2);
			RG = vE / (-iC);
		} else {

			diff = (vP - context->vCE) * context->f - context->vCap;
			iC = 0.0 - context->C / context->exponent0 * diff * exp(dt / context->exponent0);
			diff = diff - (diff * exp(dt / context->exponent0));
			context->vCap += diff;
			iQ = iC + (iC * context->R1 + context->vCap) / context->R2;
			RG = (vP - context->vCE) / iQ;
			vE = (RG - context->R3) / RG * (vP - context->vCE);
		}

		u = mw_DST_RCINTEGRATE__IN1;
		if (u > 0.7 + vE)
			vE = u - 0.7;
		iQc = EM_IC(u - vE);
		context->vCE = MIN(vP - 0.1, vP - RG * iQc);

		context->vCE = MAX(context->vCE, 0.1);
		context->vCE = 0.1 * context->vCE + 0.9 * (vP - vE - iQ * context->R3);

		switch (context->state) {
		case MW_DISC_RC_INTEGRATE_TYPE1:
			node->output = context->vCap;
			break;
		case MW_DISC_RC_INTEGRATE_TYPE2:
			node->output = vE;
			break;
		case MW_DISC_RC_INTEGRATE_TYPE3:
			node->output = MAX(0, vP - iQ * context->R3);
			break;
		}
	} else {
		node->output = 0;
	}
}

void mw_dst_rcintegrate_reset(node_description *node)
{
	struct mw_dst_rcdisc_context *context = node->context;
	double r;

	node->output = 0;

	context->state = mw_DST_RCINTEGRATE__TYPE;
	context->R1 = mw_DST_RCINTEGRATE__R1;
	context->R2 = mw_DST_RCINTEGRATE__R2;
	context->R3 = mw_DST_RCINTEGRATE__R3;
	context->C = mw_DST_RCINTEGRATE__C;

	context->vCap = 0;
	context->vCE = 0;

	r = context->R1 / context->R2 * context->R3 + context->R1 + context->R3;

	context->f = mw_DST_RCINTEGRATE__R2 / (mw_DST_RCINTEGRATE__R2 + mw_DST_RCINTEGRATE__R3);
	context->exponent0 = -1.0 * r * context->f * mw_DST_RCINTEGRATE__C;
	context->exponent1 = -1.0 * (mw_DST_RCINTEGRATE__R1 + mw_DST_RCINTEGRATE__R2) * mw_DST_RCINTEGRATE__C;
}

#define mw_DST_RCDISC_MOD__ENABLE       (*(node->input[0]))
#define mw_DST_RCDISC_MOD__IN1  (*(node->input[1]))
#define mw_DST_RCDISC_MOD__IN2  (*(node->input[2]))
#define mw_DST_RCDISC_MOD__R1           (*(node->input[3]))
#define mw_DST_RCDISC_MOD__R2           (*(node->input[4]))
#define mw_DST_RCDISC_MOD__R3           (*(node->input[5]))
#define mw_DST_RCDISC_MOD__R4           (*(node->input[6]))
#define mw_DST_RCDISC_MOD__C            (*(node->input[7]))
#define mw_DST_RCDISC_MOD__VP           (*(node->input[8]))

void mw_dst_rcdisc_mod_step(node_description *node)
{
	double diff, Rc, Rc2, vCap, u, vD = 0.0;
	struct mw_dst_rcdisc_context *context = node->context;

	vCap = context->vCap;

	if (mw_DST_RCDISC_MOD__ENABLE) {
		Rc = (mw_DST_RCDISC_MOD__IN1 > 0.5) ? mw_DST_RCDISC_MOD__R2 : mw_DST_RCDISC_MOD__R1 + mw_DST_RCDISC_MOD__R2;
		if (Rc < 1.0)
			Rc = 1.0;
		u = (mw_DST_RCDISC_MOD__IN1 > 0.5) ? 0 : mw_DST_RCDISC_MOD__VP;
		Rc2 = (mw_DST_RCDISC_MOD__IN2 > 0.6) ? (mw_DST_RCDISC_MOD__R3 * mw_DST_RCDISC_MOD__R4 / (mw_DST_RCDISC_MOD__R3 + mw_DST_RCDISC_MOD__R4)) : mw_DST_RCDISC_MOD__R4;

		diff = u - vCap;
		vD = diff * Rc2 / (Rc + Rc2);
		if (vD < -0.6) {
			Rc2 = 0;
			diff = u + 0.6 - vCap;
			diff = diff - (diff * exp(0.0 - discrete_current_context->sample_time / (mw_DST_RCDISC_MOD__C * Rc)));
			vCap += diff;
			node->output = (mw_DST_RCDISC_MOD__IN2 <= 0.6) ? -0.6 : 0;
		} else {
			diff = diff - (diff * exp(0.0 - discrete_current_context->sample_time / (mw_DST_RCDISC_MOD__C * (Rc + Rc2))));
			vCap += diff;

			node->output = (mw_DST_RCDISC_MOD__IN2 <= 0.6) ? (u - vCap) * mw_DST_RCDISC_MOD__R4 / (mw_DST_RCDISC_MOD__R4 + Rc) : 0;
		}
		context->vCap = vCap;
	} else {
		node->output = 0;
	}
}

void mw_dst_rcdisc_mod_reset(node_description *node)
{
	struct mw_dst_rcdisc_context *context = node->context;

	node->output = 0;
	context->vCap = 0;
}

#define mw_DST_RCFILTER__ENABLE         (*(node->input[0]))
#define mw_DST_RCFILTER__VIN            (*(node->input[1]))
#define mw_DST_RCFILTER__R                      (*(node->input[2]))
#define mw_DST_RCFILTER__C                      (*(node->input[3]))
#define mw_DST_RCFILTER__VREF           (*(node->input[4]))

void mw_dst_rcfilter_step(node_description *node)
{
	struct mw_dst_rcfilter_context *context = node->context;

	if (mw_DST_RCFILTER__ENABLE) {
		context->vCap += ((mw_DST_RCFILTER__VIN - mw_DST_RCFILTER__VREF - context->vCap) * context->exponent);
		node->output = context->vCap + mw_DST_RCFILTER__VREF;
	} else {
		node->output = 0;
	}
}

void mw_dst_rcfilter_reset(node_description *node)
{
	struct mw_dst_rcfilter_context *context = node->context;

	context->exponent = -1.0 / (mw_DST_RCFILTER__R * mw_DST_RCFILTER__C * discrete_current_context->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);
	context->vCap = 0;
	node->output = 0;
}

#define mw_DST_RCFILTERN__ENABLE                (*(node->input[0]))
#define mw_DST_RCFILTERN__IN            (*(node->input[1]))
#define mw_DST_RCFILTERN__R             (*(node->input[2]))
#define mw_DST_RCFILTERN__C             (*(node->input[3]))

void mw_dst_rcfilterN_reset(node_description *node)
{

	mw_dst_filter1_reset(node);
}

#define mw_DST_RCDISCN__ENABLE  (*(node->input[0]))
#define mw_DST_RCDISCN__IN              (*(node->input[1]))
#define mw_DST_RCDISCN__R               (*(node->input[2]))
#define mw_DST_RCDISCN__C               (*(node->input[3]))

void mw_dst_rcdiscN_reset(node_description *node)
{

	mw_dst_filter1_reset(node);
}

void mw_dst_rcdiscN_step(node_description *node)
{
	struct mw_dss_filter1_context *context = node->context;
	double gain = 1.0;

	if (mw_DST_RCDISCN__ENABLE == 0.0) {
		gain = 0.0;
	}

	if (gain * mw_DST_RCDISCN__IN > context->x1)
		node->output = gain * mw_DST_RCDISCN__IN;
	else
		node->output = -context->a1 * context->y1;

	context->x1 = gain * mw_DST_RCDISCN__IN;
	context->y1 = node->output;
}

#define mw_DST_RCDISC2N__ENABLE         (*(node->input[0]))
#define mw_DST_RCDISC2N__IN0            (*(node->input[1]))
#define mw_DST_RCDISC2N__R0             (*(node->input[2]))
#define mw_DST_RCDISC2N__IN1            (*(node->input[3]))
#define mw_DST_RCDISC2N__R1             (*(node->input[4]))
#define mw_DST_RCDISC2N__C                      (*(node->input[5]))

struct mw_dss_rcdisc2_context {
	double x1;
	double y1;
	double a1_0, b0_0, b1_0;
	double a1_1, b0_1, b1_1;
};

void mw_dst_rcdisc2N_step(node_description *node)
{
	struct mw_dss_rcdisc2_context *context = node->context;
	double input = ((mw_DST_RCDISC2N__ENABLE == 0) ? mw_DST_RCDISC2N__IN0 : mw_DST_RCDISC2N__IN1);

	if (mw_DST_RCDISC2N__ENABLE == 0)
		node->output = -context->a1_0 * context->y1 + context->b0_0 * input + context->b1_0 * context->x1;
	else
		node->output = -context->a1_1 * context->y1 + context->b0_1 * input + context->b1_1 * context->x1;

	context->x1 = input;
	context->y1 = node->output;
}

void mw_dst_rcdisc2N_reset(node_description *node)
{
	struct mw_dss_rcdisc2_context *context = node->context;
	double f1, f2;

	f1 = 1.0 / (2 * M_PI * mw_DST_RCDISC2N__R0 * mw_DST_RCDISC2N__C);
	f2 = 1.0 / (2 * M_PI * mw_DST_RCDISC2N__R1 * mw_DST_RCDISC2N__C);

	mw_calculate_filter1_coefficients(f1, MW_DISC_FILTER_LOWPASS, &context->a1_0, &context->b0_0, &context->b1_0);
	mw_calculate_filter1_coefficients(f2, MW_DISC_FILTER_LOWPASS, &context->a1_1, &context->b0_1, &context->b1_1);

	node->output = 0;
}

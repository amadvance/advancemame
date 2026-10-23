/*
 *  (c) K.Wilkins 2000
 *  (c) D.Renaud 2003-2004
 */


#define MW_DEFAULT_555_CAP_BLEED        RES_M(10)

struct mw_dsd_555_astbl_context {
	int error;
	int use_ctrlv;
	int output_type;
	int output_is_ac;
	double ac_shift;
	int flip_flop;
	double x_init;
	double cap_voltage;
	double threshold;
	double trigger;
	double output_high_voltage;
	double v555;
};

struct mw_dsd_555_mstbl_context {
	int error;
	int trig_is_logic;
	int trig_discharges_cap;
	int output_type;
	int output_is_ac;
	double ac_shift;
	int flip_flop;
	double cap_voltage;
	double threshold;
	double trigger;
	double output_high_voltage;
};

struct mw_dsd_555_cc_context {
	int error;
	unsigned int type;
	int output_type;
	int output_is_ac;
	double ac_shift;
	int flip_flop;
	double x_init;
	double cap_voltage;
	double threshold;
	double trigger;
	double output_high_voltage;
};

struct mw_dsd_555_vco1_context {
	int ctrlv_is_node;
	int output_type;
	int output_is_ac;
	double ac_shift;
	int flip_flop;
	double output_high_voltage;
	double threshold;
	double trigger;
	double i_charge;
	double i_discharge;
	double cap_voltage;
};

struct mw_dsd_566_context {
	int error;
	unsigned int state[2];
	int flip_flop;
	double cap_voltage;
	double vDiff;
	double vSqrLow;
	double vSqrHigh;
	double thresholdLow;
	double thresholdHigh;
	double triOffset;
};

static int mw_test_555(double threshold, double trigger, double v555, int node)
{
	int error = 0;
	if (threshold > v555) {
		logerror("[Threshold > B+]");
		error = 1;
	}
	if (threshold <= trigger) {
		logerror("[Threshold <= Trigger]");
		error = 1;
	}
	if (trigger < 0) {
		logerror("[Trigger < 0]");
		error = 1;
	}
	if (v555 <= 0) {
		logerror("[B+ <= 0]");
		error = 1;
	}
	if (error)
		logerror(" - NODE_%d DISABLED!\n", node - NODE_00);
	return error;
}

#define mw_DSD_555_ASTBL__RESET (!*(node->input[0]))
#define mw_DSD_555_ASTBL__R1            (*(node->input[1]))
#define mw_DSD_555_ASTBL__R2            (*(node->input[2]))
#define mw_DSD_555_ASTBL__C             (*(node->input[3]))
#define mw_DSD_555_ASTBL__CTRLV (*(node->input[4]))

void mw_dsd_555_astbl_step(node_description *node)
{
	const mw_discrete_555_desc *info = node->custom;
	struct mw_dsd_555_astbl_context *context = node->context;

	int count_f = 0;
	int count_r = 0;
	double dt;
	double xTime;
	double tRC = 0;
	double vC = context->cap_voltage;
	double vCnext = 0;

	if (mw_DSD_555_ASTBL__RESET || context->error) {

		node->output = 0;
		context->flip_flop = 1;
		context->cap_voltage = 0;
		return;
	}

	if (context->use_ctrlv) {

		if (mw_DSD_555_ASTBL__CTRLV < .25) return;

		context->threshold = mw_DSD_555_ASTBL__CTRLV;
		context->trigger = mw_DSD_555_ASTBL__CTRLV / 2.0;

		if (vC >= context->threshold) {
			context->flip_flop = 0;
			count_f++;
		} else if (vC <= context->trigger) {
			context->flip_flop = 1;
			count_r++;
		}
	}

	dt = discrete_current_context->sample_time;
	xTime = context->x_init;

	if (mw_DSD_555_ASTBL__C == 0) {
		context->flip_flop = 1;

		vCnext = context->v555;
		vC = context->v555;
		context->cap_voltage = 0;
	} else {

		do{
			if (context->flip_flop) {
				if (mw_DSD_555_ASTBL__R1 == 0) {

					tRC = MW_DEFAULT_555_CAP_BLEED * mw_DSD_555_ASTBL__C;
					vCnext = vC - (vC * (1.0 - exp(-(dt / tRC))));
					dt = 0;
				} else {

					tRC = (mw_DSD_555_ASTBL__R1 + ((info->options & MW_DISC_555_ASTABLE_HAS_FAST_CHARGE_DIODE) ? 0 : mw_DSD_555_ASTBL__R2)) * mw_DSD_555_ASTBL__C;
					vCnext = vC + ((context->v555 - vC) * (1.0 - exp(-(dt / tRC))));
					dt = 0;

					if (vCnext > context->threshold) {

						dt = tRC * log(1.0 / (1.0 - ((vCnext - context->threshold) / (context->v555 - vC))));
						xTime = dt;
						vC = context->threshold;
						context->flip_flop = 0;
						count_f++;
					}
				}
			} else {

				if (mw_DSD_555_ASTBL__R2 != 0) {
					tRC = mw_DSD_555_ASTBL__R2 * mw_DSD_555_ASTBL__C;
					vCnext = vC - (vC * (1 - exp(-(dt / tRC))));
					dt = 0;
				} else {
					vCnext = context->trigger;
					dt = 0;
				}

				if (vCnext < context->trigger) {

					dt = tRC * log(1.0 / (1.0 - ((context->trigger - vCnext) / vC)));
					xTime = dt;
					vC = context->trigger;
					context->flip_flop = 1;
					count_r++;
				}
			}
		} while (dt);

		context->cap_voltage = vCnext;
	}

	xTime = xTime / discrete_current_context->sample_time;

	switch (context->output_type) {
	case MW_DISC_555_OUT_SQW:
		node->output = context->flip_flop * context->output_high_voltage + context->ac_shift;
		break;
	case MW_DISC_555_OUT_CAP:
		node->output = vCnext;

		if (context->output_is_ac)
			node->output -= context->threshold * 3.0 / 4.0;
		break;
	case MW_DISC_555_OUT_ENERGY:
		node->output = context->output_high_voltage * (context->flip_flop ? xTime : (1 - xTime));
		node->output += context->ac_shift;
		break;
	case MW_DISC_555_OUT_LOGIC_X:
		node->output = context->flip_flop + xTime;
		break;
	case MW_DISC_555_OUT_COUNT_F_X:
		node->output = count_f ? count_f + xTime : count_f;
		break;
	case MW_DISC_555_OUT_COUNT_R_X:
		node->output = count_r ? count_r + xTime : count_r;
		break;
	case MW_DISC_555_OUT_COUNT_F:
		node->output = count_f;
		break;
	case MW_DISC_555_OUT_COUNT_R:
		node->output = count_r;
		break;
	}
}

void mw_dsd_555_astbl_reset(node_description *node)
{
	const mw_discrete_555_desc *info = node->custom;
	struct mw_dsd_555_astbl_context *context = node->context;

	context->use_ctrlv = (node->input_is_node >> 4) & 1;
	context->output_type = info->options & MW_DISC_555_OUT_MASK;

	context->output_high_voltage = (info->v555high == MW_DEFAULT_555_HIGH) ? info->v555 - 1.2 : info->v555high;
	if ((mw_DSD_555_ASTBL__CTRLV != -1) && !context->use_ctrlv) {

		context->threshold = mw_DSD_555_ASTBL__CTRLV;
		context->trigger = mw_DSD_555_ASTBL__CTRLV / 2.0;
	} else {

		context->threshold = (info->threshold555 == MW_DEFAULT_555_THRESHOLD) ? info->v555 * 2 / 3 : info->threshold555;
		context->trigger = (info->trigger555 == MW_DEFAULT_555_TRIGGER) ? info->v555 / 3 : info->trigger555;
	}

	context->output_is_ac = info->options & MW_DISC_555_OUT_AC;

	context->ac_shift = context->output_is_ac ? -context->output_high_voltage / 2.0 : 0;

	context->error = mw_test_555(context->threshold, context->trigger, info->v555, node->node);

	context->v555 = (info->options & MW_DISC_555_ASTABLE_HAS_FAST_CHARGE_DIODE) ? info->v555 - 0.5: info->v555;
	context->flip_flop = 1;
	context->cap_voltage = 0;

	context->x_init = 0;
	if (context->output_type == MW_DISC_555_OUT_ENERGY)
		context->x_init = discrete_current_context->sample_time;

	mw_dsd_555_astbl_step(node);
}

#define mw_DSD_555_MSTBL__RESET (!*(node->input[0]))
#define mw_DSD_555_MSTBL__TRIGGER       (*(node->input[1]))
#define mw_DSD_555_MSTBL__R             (*(node->input[2]))
#define mw_DSD_555_MSTBL__C             (*(node->input[3]))

void mw_dsd_555_mstbl_step(node_description *node)
{
	const mw_discrete_555_desc *info = node->custom;
	struct mw_dsd_555_mstbl_context *context = node->context;

	double vC;
	double vCnext = 0;

	if (mw_DSD_555_MSTBL__RESET || context->error) {

		node->output = 0;
		context->flip_flop = 0;
		context->cap_voltage = 0;
	} else {
		int trigger;

		if (context->trig_is_logic)
			trigger = !mw_DSD_555_MSTBL__TRIGGER;
		else
			trigger = mw_DSD_555_MSTBL__TRIGGER < context->trigger;

		if (context->trig_discharges_cap && trigger)
			context->cap_voltage = 0;

		if (!context->flip_flop) {

			if (trigger)
				context->flip_flop = 1;
		} else {
			vC = context->cap_voltage;

			if (mw_DSD_555_MSTBL__C == 0) {
				context->flip_flop = 0;

				vCnext = info->v555;
				vC = info->v555;
				context->cap_voltage = 0;
			} else {

				vCnext = vC + ((info->v555 - vC) * (1.0 - exp(-(discrete_current_context->sample_time / (mw_DSD_555_MSTBL__R * mw_DSD_555_MSTBL__C)))));

				if ((vCnext >= context->threshold) && !trigger) {
					vCnext = 0;
					vC = context->threshold;
					context->flip_flop = 0;
				}
			}

			context->cap_voltage = vCnext;

			switch (info->options & MW_DISC_555_OUT_MASK) {
			case MW_DISC_555_OUT_SQW:
				node->output = context->flip_flop * context->output_high_voltage;

				if (context->output_is_ac)
					node->output -= context->output_high_voltage / 2.0;
				break;
			case MW_DISC_555_OUT_CAP:
				node->output = vCnext;

				if (context->output_is_ac)
					node->output -= context->threshold * 3.0 / 4.0;
				break;
			}
		}
	}
}

void mw_dsd_555_mstbl_reset(node_description *node)
{
	const mw_discrete_555_desc *info = node->custom;
	struct mw_dsd_555_mstbl_context *context = node->context;

	context->output_type = info->options & MW_DISC_555_OUT_MASK;
	if ((context->output_type == MW_DISC_555_OUT_COUNT_F) || (context->output_type == MW_DISC_555_OUT_COUNT_R)) {
		discrete_log("Invalid Output type in NODE_%d.\n", node->node - NODE_00);
		context->output_type = MW_DISC_555_OUT_SQW;
	}

	context->threshold = (info->threshold555 == MW_DEFAULT_555_THRESHOLD) ? info->v555 * 2 / 3 : info->threshold555;
	context->trigger = (info->trigger555 == MW_DEFAULT_555_TRIGGER) ? info->v555 / 3 : info->trigger555;
	context->output_high_voltage = (info->v555high == MW_DEFAULT_555_HIGH) ? info->v555 - 1.2 : info->v555high;

	context->output_is_ac = info->options & MW_DISC_555_OUT_AC;

	context->ac_shift = context->output_is_ac ? -context->output_high_voltage / 2.0 : 0;

	context->error = mw_test_555(context->threshold, context->trigger, info->v555, node->node);

	context->trig_is_logic = (info->options & MW_DISC_555_TRIGGER_IS_VOLTAGE) ? 0: 1;
	context->trig_discharges_cap = (info->options & MW_DISC_555_TRIGGER_DISCHARGES_CAP) ? 1: 0;

	context->flip_flop = 0;
	context->cap_voltage = 0;

	node->output = 0;
}

#define mw_DSD_555_CC__RESET    (!*(node->input[0]))
#define mw_DSD_555_CC__VIN              (*(node->input[1]))
#define mw_DSD_555_CC__R                (*(node->input[2]))
#define mw_DSD_555_CC__C                (*(node->input[3]))
#define mw_DSD_555_CC__RBIAS    (*(node->input[4]))
#define mw_DSD_555_CC__RGND     (*(node->input[5]))
#define mw_DSD_555_CC__RDIS     (*(node->input[6]))

void mw_dsd_555_cc_step(node_description *node)
{
	const mw_discrete_555_cc_desc *info = node->custom;
	struct mw_dsd_555_cc_context *context = node->context;

	int count_f = 0;
	int count_r = 0;
	double i;
	double rC = 0;
	double rD = 0;
	double vi = 0;
	double vB = 0;
	double v = 0;
	double dt;
	double xTime;
	double tRC;
	double vC;
	double vCnext = 0;
	double viLimit;
	double rTemp;

	if (mw_DSD_555_CC__RESET || context->error) {

		node->output = 0;
		context->flip_flop = 1;
		context->cap_voltage = 0;
		return;
	}

	dt = discrete_current_context->sample_time;
	xTime = context->x_init;
	vC = context->cap_voltage;
	viLimit = mw_DSD_555_CC__VIN + info->vCCjunction;

	i = (info->vCCsource - viLimit) / mw_DSD_555_CC__R;
	if (i < 0) i = 0;

	if (info->options & MW_DISCRETE_555_CC_TO_CAP) {
		vi = i * mw_DSD_555_CC__RDIS;
	} else
		switch (context->type) {
		case 1:
			rD = mw_DSD_555_CC__RDIS;
		case 0:
			break;
		case 3:
			rD = (mw_DSD_555_CC__RDIS * mw_DSD_555_CC__RGND) / (mw_DSD_555_CC__RDIS + mw_DSD_555_CC__RGND);
		case 2:
			rC = mw_DSD_555_CC__RGND;
			vi = i * rC;
			break;
		case 4:
			rC = mw_DSD_555_CC__RBIAS;
			vi = i * rC;
			vB = info->v555;
			break;
		case 5:
			rC = mw_DSD_555_CC__RBIAS + mw_DSD_555_CC__RDIS;
			vi = i * mw_DSD_555_CC__RBIAS;
			vB = info->v555;
			rD = mw_DSD_555_CC__RDIS;
			break;
		case 6:
			rC = (mw_DSD_555_CC__RBIAS * mw_DSD_555_CC__RGND) / (mw_DSD_555_CC__RBIAS + mw_DSD_555_CC__RGND);
			vi = i * rC;
			vB = info->v555 * (mw_DSD_555_CC__RGND / (mw_DSD_555_CC__RBIAS + mw_DSD_555_CC__RGND));
			break;
		case 7:
			rTemp = mw_DSD_555_CC__RBIAS + mw_DSD_555_CC__RDIS;
			rC = (rTemp * mw_DSD_555_CC__RGND) / (rTemp + mw_DSD_555_CC__RGND);
			rTemp += mw_DSD_555_CC__RGND;
			rTemp = mw_DSD_555_CC__RGND / rTemp;
			vi = i * mw_DSD_555_CC__RBIAS * rTemp;
			vB = info->v555 * rTemp;
			rD = (mw_DSD_555_CC__RGND * mw_DSD_555_CC__RDIS) / (mw_DSD_555_CC__RGND + mw_DSD_555_CC__RDIS);
			break;
		}

	do{
		if (context->type <= 1) {

			if (context->flip_flop) {
				if (i == 0) {

					tRC = MW_DEFAULT_555_CAP_BLEED * mw_DSD_555_CC__C;
					vCnext = vC - (vC * (1.0 - exp(-(dt / tRC))));
					dt = 0;
				} else {

					vCnext = vC + (i * dt / mw_DSD_555_CC__C);

					if (vCnext > viLimit) vCnext = viLimit;
					dt = 0;

					if (vCnext >= context->threshold) {

						dt = mw_DSD_555_CC__C * (vCnext - context->threshold) / i;
						xTime = dt;
						vC = context->threshold;
						context->flip_flop = 0;
						count_f++;
					}
				}
			} else if (mw_DSD_555_CC__RDIS) {

				tRC = mw_DSD_555_CC__RDIS * mw_DSD_555_CC__C;

				if (info->options & MW_DISCRETE_555_CC_TO_CAP) {

					v = (vC < viLimit) ? vi : viLimit;
					vCnext = vC + ((v - vC) * (1.0 - exp(-(dt / tRC))));
				} else {
					vCnext = vC - (vC * (1.0 - exp(-(dt / tRC))));
				}

				dt = 0;

				if (vCnext <= context->trigger) {
					dt = tRC * log(1.0 / (1.0 - ((context->trigger - vCnext) / vC)));
					xTime = dt;
					vC = context->trigger;
					context->flip_flop = 1;
					count_r++;
				}
			} else {
				vC = context->trigger;
				context->flip_flop = 1;
				count_r++;
			}
		} else {

			if (context->flip_flop) {
				if ((i == 0) && (mw_DSD_555_CC__RBIAS == 0)) {

					tRC = mw_DSD_555_CC__RGND * mw_DSD_555_CC__C;
					vCnext = vC - (vC * (1.0 - exp(-(dt / tRC))));
					dt = 0;
				} else {

					v = vB;
					if (vC < viLimit) v += vi;
					else if (context->type <= 3) v = viLimit;

					tRC = rC * mw_DSD_555_CC__C;
					vCnext = vC + ((v - vC) * (1.0 - exp(-(dt / tRC))));
					dt = 0;

					if (vCnext >= context->threshold) {

						dt = tRC * log(1.0 / (1.0 - ((vCnext - context->threshold) / (v - vC))));
						xTime = dt;
						vC = context->threshold;
						context->flip_flop = 0;
						count_f++;
					}
				}
			} else if (rD) {
				tRC = rD * mw_DSD_555_CC__C;
				vCnext = vC - (vC * (1.0 - exp(-(dt / tRC))));
				dt = 0;

				if (vCnext <= context->trigger) {

					dt = tRC * log(1.0 / (1.0 - ((context->trigger - vCnext) / vC)));
					xTime = dt;
					vC = context->trigger;
					context->flip_flop = 1;
					count_r++;
				}
			} else {
				vC = context->trigger;
				context->flip_flop = 1;
				count_r++;
			}
		}
	} while (dt);

	context->cap_voltage = vCnext;

	xTime = xTime / discrete_current_context->sample_time;

	switch (context->output_type) {
	case MW_DISC_555_OUT_SQW:
		if (count_r && (~context->type & 0x01)) {

			node->output = 0;
		} else
			node->output = context->flip_flop * context->output_high_voltage;

		node->output += context->ac_shift;
		break;
	case MW_DISC_555_OUT_CAP:
		node->output = vCnext + context->ac_shift;
		break;
	case MW_DISC_555_OUT_ENERGY:
		node->output = context->output_high_voltage * (context->flip_flop ? xTime : (1 - xTime));
		node->output += context->ac_shift;
		break;
	case MW_DISC_555_OUT_LOGIC_X:
		node->output = context->flip_flop + xTime;
		break;
	case MW_DISC_555_OUT_COUNT_F_X:
		node->output = count_f + xTime;
		break;
	case MW_DISC_555_OUT_COUNT_R_X:
		node->output = count_r + xTime;
		break;
	case MW_DISC_555_OUT_COUNT_F:
		node->output = count_f;
		break;
	case MW_DISC_555_OUT_COUNT_R:
		node->output = count_r;
		break;
	}
}

void mw_dsd_555_cc_reset(node_description *node)
{
	const mw_discrete_555_cc_desc *info = node->custom;
	struct mw_dsd_555_cc_context *context = node->context;

	context->flip_flop = 1;
	context->cap_voltage = 0;

	context->output_type = info->options & MW_DISC_555_OUT_MASK;

	context->x_init = 0;
	if (context->output_type == MW_DISC_555_OUT_ENERGY)
		context->x_init = discrete_current_context->sample_time;

	context->threshold = (info->threshold555 == MW_DEFAULT_555_THRESHOLD) ? info->v555 * 2 / 3 : info->threshold555;
	context->trigger = (info->trigger555 == MW_DEFAULT_555_TRIGGER) ? info->v555 / 3 : info->trigger555;
	context->output_high_voltage = (info->v555high == MW_DEFAULT_555_HIGH) ? info->v555 - 1.2 : info->v555high;

	context->output_is_ac = info->options & MW_DISC_555_OUT_AC;

	context->ac_shift = context->output_is_ac ? -context->output_high_voltage / 2.0 : 0;

	context->error = mw_test_555(context->threshold, context->trigger, info->v555, node->node);

	context->type = (mw_DSD_555_CC__RDIS > 0) | ((mw_DSD_555_CC__RGND > 0) << 1) | ((mw_DSD_555_CC__RBIAS > 0) << 2);

	mw_dsd_555_cc_step(node);
}

#define mw_DSD_555_VCO1__RESET  (*(node->input[0]))
#define mw_DSD_555_VCO1__VIN1   (*(node->input[1]))
#define mw_DSD_555_VCO1__VIN2   (*(node->input[2]))

void mw_dsd_555_vco1_step(node_description *node)
{
	const mw_discrete_555_vco1_desc *info = node->custom;
	struct mw_dsd_555_vco1_context *context = node->context;

	int count_f = 0;
	int count_r = 0;
	double dt;
	double xTime = 0;
	double vC;
	double vCnext = 0;

	dt = discrete_current_context->sample_time;
	vC = context->cap_voltage;

	if (context->ctrlv_is_node && mw_DSD_555_VCO1__RESET) {

		if (mw_DSD_555_VCO1__VIN2 < .25) return;

		context->threshold = mw_DSD_555_VCO1__VIN2;
		context->trigger = mw_DSD_555_VCO1__VIN2 / 2.0;

		if (vC >= context->threshold) {
			context->flip_flop = 0;
			count_f++;
		} else if (vC <= context->trigger) {
			context->flip_flop = 1;
			count_r++;
		}
	}

	do{
		if (context->flip_flop) {

			if (!mw_DSD_555_VCO1__RESET) {
				context->flip_flop = 0;
				count_f++;
			} else {

				vCnext = vC + (context->i_charge * dt / info->c);
				dt = 0;

				if (vCnext >= context->threshold) {
					if (vCnext > context->threshold) {

						dt = info->c * (vCnext - context->threshold) / context->i_charge;
					}
					vC = context->threshold;
					context->flip_flop = 0;
					count_f++;
					xTime = dt;
				}
			}
		} else {

			vCnext = vC - (context->i_discharge * dt / info->c);

			if (!mw_DSD_555_VCO1__RESET) {
				if (vCnext < 0) vCnext = 0;
				dt = 0;
			} else {

				if (vC <= context->trigger) {
					context->flip_flop = 1;
					count_r++;
				} else {
					dt = 0;

					if (vCnext <= context->trigger) {
						if (vCnext < context->trigger) {

							dt = info->c * (vCnext - context->trigger) / context->i_discharge;
						}
						vC = context->trigger;
						context->flip_flop = 1;
						count_r++;
						xTime = dt;
					}
				}
			}
		}
	} while (dt);

	context->cap_voltage = vCnext;

	xTime = xTime / discrete_current_context->sample_time;

	switch (context->output_type) {
	case MW_DISC_555_OUT_SQW:
		node->output = context->flip_flop * context->output_high_voltage + context->ac_shift;
		break;
	case MW_DISC_555_OUT_CAP:
		node->output = vCnext;

		if (context->output_is_ac)
			node->output -= context->threshold * 3.0 / 4.0;
		break;
	case MW_DISC_555_OUT_ENERGY:
		node->output = context->output_high_voltage * (context->flip_flop ? xTime : (1 - xTime));
		node->output += context->ac_shift;
		break;
	case MW_DISC_555_OUT_LOGIC_X:
		node->output = context->flip_flop + xTime;
		break;
	case MW_DISC_555_OUT_COUNT_F_X:
		node->output = count_f ? count_f + xTime : count_f;
		break;
	case MW_DISC_555_OUT_COUNT_R_X:
		node->output = count_r ? count_r + xTime : count_r;
		break;
	case MW_DISC_555_OUT_COUNT_F:
		node->output = count_f;
		break;
	case MW_DISC_555_OUT_COUNT_R:
		node->output = count_r;
		break;
	}
}

void mw_dsd_555_vco1_reset(node_description *node)
{
	const mw_discrete_555_vco1_desc *info = node->custom;
	struct mw_dsd_555_vco1_context *context = node->context;

	double v_ratio_r3, v_ratio_r4_1, r_in_1;

	context->output_type = info->options & MW_DISC_555_OUT_MASK;
	context->output_is_ac = info->options & MW_DISC_555_OUT_AC;

	v_ratio_r3 = info->r3 / (info->r2 + info->r3);

	v_ratio_r4_1 = info->r4 / (info->r1 + info->r4);

	r_in_1 = 1.0 / (1.0 / info->r1 + 1.0 / info->r4);

	context->i_discharge = (1 - v_ratio_r3) / info->r1;
	context->i_charge = (v_ratio_r3 - v_ratio_r4_1) / r_in_1;

	context->cap_voltage = 0;

	context->flip_flop = 1;
	context->ctrlv_is_node = (node->input_is_node >> 2) & 1;
	context->output_high_voltage = (info->v555high == MW_DEFAULT_555_HIGH) ? info->v555 - 1.2 : info->v555high;

	if (!context->ctrlv_is_node && (mw_DSD_555_VCO1__VIN2 != -1)) {

		context->threshold = mw_DSD_555_VCO1__VIN2;
		context->trigger = mw_DSD_555_VCO1__VIN2 / 2.0;
	} else {

		context->threshold = (info->threshold555 == MW_DEFAULT_555_THRESHOLD) ? info->v555 * 2 / 3 : info->threshold555;
		context->trigger = (info->trigger555 == MW_DEFAULT_555_TRIGGER) ? info->v555 / 3 : info->trigger555;
	}

	context->ac_shift = context->output_is_ac ? -context->output_high_voltage / 2.0 : 0;
}

#define mw_DSD_566__ENABLE      (*(node->input[0]))
#define mw_DSD_566__VMOD        (*(node->input[1]))
#define mw_DSD_566__R           (*(node->input[2]))
#define mw_DSD_566__C           (*(node->input[3]))

void mw_dsd_566_step(node_description *node)
{
	const mw_discrete_566_desc *info = node->custom;
	struct mw_dsd_566_context *context = node->context;

	double i;
	double dt;
	double vC;
	double vCnext = 0;

	if (mw_DSD_566__ENABLE && !context->error) {
		dt = discrete_current_context->sample_time;
		vC = context->cap_voltage;

		i = (context->vDiff - mw_DSD_566__VMOD) / mw_DSD_566__R;

		do{
			if (context->flip_flop) {

				vCnext = vC - (i * dt / mw_DSD_566__C);
				dt = 0;

				if (vCnext <= context->thresholdLow) {
					if (vCnext < context->thresholdLow) {

						dt = mw_DSD_566__C * (context->thresholdLow - vCnext) / i;
					}
					vC = context->thresholdLow;
					context->flip_flop = 0;

					context->state[0] = (context->state[0] + 1) & 0x03;
				}
			} else {

				vCnext = vC + (i * dt / mw_DSD_566__C);
				dt = 0;

				if (vCnext > mw_DSD_566__VMOD) vCnext = mw_DSD_566__VMOD;

				if (vCnext >= context->thresholdHigh) {
					if (vCnext > context->thresholdHigh) {

						dt = mw_DSD_566__C * (vCnext - context->thresholdHigh) / i;
					}
					vC = context->thresholdHigh;
					context->flip_flop = 1;
					context->state[1] = (context->state[1] + 1) & 0x03;
				}
			}
		} while (dt);

		context->cap_voltage = vCnext;

		switch (info->options & MW_DISC_566_OUT_MASK) {
		case MW_DISC_566_OUT_SQUARE:
		case MW_DISC_566_OUT_LOGIC:

			if (node->output && context->state[0]) {
				node->output = 0;
				context->state[0]--;
			} else if (!node->output && context->state[1]) {
				node->output = 1;
				context->state[1]--;
			} else {
				node->output = context->flip_flop;
			}
			if ((info->options & MW_DISC_566_OUT_MASK) != MW_DISC_566_OUT_LOGIC)
				node->output = context->flip_flop ? context->vSqrHigh : context->vSqrLow;
			break;
		case MW_DISC_566_OUT_TRIANGLE:

			node->output = vCnext;
			if (info->options & MW_DISC_566_OUT_AC)
				node->output -= context->triOffset;
			break;
		}
	} else
		node->output = 0;
}

void mw_dsd_566_reset(node_description *node)
{
	const mw_discrete_566_desc *info = node->custom;
	struct mw_dsd_566_context *context = node->context;

	double temp;

	context->error = 0;
	if (info->vNeg >= info->vPlus) {
		logerror("[vNeg >= vPlus] - NODE_%d DISABLED!\n", node->node - NODE_00);
		context->error = 1;
	}

	context->vDiff = info->vPlus - info->vNeg;
	context->flip_flop = 0;
	context->cap_voltage = 0;
	context->state[0] = 0;
	context->state[1] = 0;

	context->thresholdHigh = context->vDiff / 2 + info->vNeg;
	context->thresholdLow = context->thresholdHigh - (0.2 * context->vDiff);
	context->vSqrHigh = info->vPlus - 0.6;
	context->vSqrLow = context->thresholdHigh;

	if (info->options & MW_DISC_566_OUT_AC) {
		temp = (context->vSqrHigh - context->vSqrLow) / 2;
		context->vSqrHigh = temp;
		context->vSqrLow = -temp;
		context->triOffset = context->thresholdHigh - (0.1 * context->vDiff);
	}

	mw_dsd_566_step(node);
}

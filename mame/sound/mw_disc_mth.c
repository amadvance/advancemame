// license:BSD-3-Clause
// copyright-holders:Keith Wilkins, Derrick Renaud, Couriersud
/*
 *  (c) K.Wilkins 2000
 *  (c) D.Renaud 2003-2004
 */


#include <float.h>

struct mw_dst_dac_r1_context {
	double iBias;
	double exponent;
	double rTotal;
};

struct mw_dst_flipflop_context {
	int last_clk;
};

struct mw_dst_integrate_context {
	double change;
	double vMaxIn;
	double vMaxInD;
	double vMaxOut;
};

#define MW_DISC_MIXER_MAX_INPS  8

struct mw_dst_mixer_context {
	int type;
	int size;
	double rTotal;
	double *rNode[MW_DISC_MIXER_MAX_INPS];
	double exponent_rc[MW_DISC_MIXER_MAX_INPS];
	double vCap[MW_DISC_MIXER_MAX_INPS];
	double exponent_cF;
	double exponent_cAmp;
	double vCapF;
	double vCapAmp;
	double gain;
};

struct mw_dst_oneshot_context {
	double countdown;
	int state;
	int lastTrig;
};

struct mw_dss_ramp_context {
	double step;
	int dir;
	int last_en;
};

struct mw_dst_samphold_context {
	double lastinput;
	int clocktype;
};

struct mw_dst_size_context {
	int size;
};

struct mw_dst_op_amp_context {
	UINT8 has_cap;
	UINT8 has_r1;
	UINT8 has_r4;
	double vMax;
	double iFixed;
	double vCap;
	double exponent;
};

struct mw_dst_op_amp_1sht_context {
	double iFixed;
	double vMax;
	double r34ratio;
	double vCap1;
	double vCap2;
	double exponent1c;
	double exponent1d;
	double exponent2;
};

struct mw_dst_tvca_op_amp_context {
	double vOutMax;
	double vTrig[2];
	double vTrig2;
	double vTrig3;
	double iFixed;
	double exponentC[2];
	double exponentD[2];
	double exponent2[2];
	double exponent3[2];
	double vCap1;
	double vCap2;
	double vCap3;
	double r67;
};

#define mw_DST_ADDER__ENABLE    (*(node->input[0]))
#define mw_DST_ADDER__IN0               (*(node->input[1]))
#define mw_DST_ADDER__IN1               (*(node->input[2]))
#define mw_DST_ADDER__IN2               (*(node->input[3]))
#define mw_DST_ADDER__IN3               (*(node->input[4]))

void mw_dst_adder_step(node_description *node)
{
	if (mw_DST_ADDER__ENABLE) {
		node->output = mw_DST_ADDER__IN0 + mw_DST_ADDER__IN1 + mw_DST_ADDER__IN2 + mw_DST_ADDER__IN3;
	} else {
		node->output = 0;
	}
}

#define mw_DST_COMP_ADDER__ENABLE       (*(node->input[0]))
#define mw_DST_COMP_ADDER__SELECT       (int)(*(node->input[1]))

void mw_dst_comp_adder_step(node_description *node)
{
	const mw_discrete_comp_adder_table *info = node->custom;
	int bit;

	if (mw_DST_COMP_ADDER__ENABLE) {
		switch (info->type) {
		case MW_DISC_COMP_P_CAPACITOR:
			node->output = info->cDefault;
			for (bit = 0; bit < info->length; bit++) {
				if (mw_DST_COMP_ADDER__SELECT & (1 << bit)) node->output += info->c[bit];
			}
			break;
		case MW_DISC_COMP_P_RESISTOR:
			node->output = info->cDefault ? 1.0 / info->cDefault : 0;
			for (bit = 0; bit < info->length; bit++) {
				if (mw_DST_COMP_ADDER__SELECT & (1 << bit)) node->output += 1.0 / info->c[bit];
			}
			if (node->output != 0) node->output = 1.0 / node->output;
			break;
		}
	} else {
		node->output = 0;
	}
}

#define mw_DST_CLAMP__ENABLE    (*(node->input[0]))
#define mw_DST_CLAMP__IN                (*(node->input[1]))
#define mw_DST_CLAMP__MIN               (*(node->input[2]))
#define mw_DST_CLAMP__MAX               (*(node->input[3]))
#define mw_DST_CLAMP__CLAMP     (*(node->input[4]))

void mw_dst_clamp_step(node_description *node)
{
	if (mw_DST_CLAMP__ENABLE) {
		if (mw_DST_CLAMP__IN < mw_DST_CLAMP__MIN) node->output = mw_DST_CLAMP__MIN;
		else if (mw_DST_CLAMP__IN > mw_DST_CLAMP__MAX) node->output = mw_DST_CLAMP__MAX;
		else node->output = mw_DST_CLAMP__IN;
	} else {
		node->output = mw_DST_CLAMP__CLAMP;
	}
}

#define mw_DST_DAC_R1__ENABLE           (*(node->input[0]))
#define mw_DST_DAC_R1__DATA             (int)(*(node->input[1]))
#define mw_DST_DAC_R1__VON                      (*(node->input[2]))

void mw_dst_dac_r1_step(node_description *node)
{
	const mw_discrete_dac_r1_ladder *info = node->custom;
	struct mw_dst_dac_r1_context *context = node->context;

	int bit;
	double v;
	double i;

	i = context->iBias;

	if (mw_DST_DAC_R1__ENABLE) {
		for (bit = 0; bit < info->ladderLength; bit++) {

			if ((mw_DST_DAC_R1__DATA & (1 << bit)) && info->r[bit])
				i += mw_DST_DAC_R1__VON / info->r[bit];
		}

		v = i * context->rTotal;

		node->output = info->cFilter ? node->output + ((v - node->output) * context->exponent) : v;
	} else {

	}
}

void mw_dst_dac_r1_reset(node_description *node)
{
	const mw_discrete_dac_r1_ladder *info = node->custom;
	struct mw_dst_dac_r1_context *context = node->context;

	int bit;

	if (info->rBias)
		context->iBias = info->vBias / info->rBias;
	else
		context->iBias = 0;

	if (info->ladderLength < 2) {

		discrete_log("mw_dst_dac_r1_reset - Ladder length too small");
	}
	if (info->ladderLength > MW_DISC_LADDER_MAXRES) {
		discrete_log("mw_dst_dac_r1_reset - Ladder length exceeds MW_DISC_LADDER_MAXRES");
	}

	context->rTotal = 0;
	for (bit = 0; bit < info->ladderLength; bit++) {
		if (info->r[bit])
			context->rTotal += 1.0 / info->r[bit];
	}
	if (info->rBias) context->rTotal += 1.0 / info->rBias;
	if (info->rGnd) context->rTotal += 1.0 / info->rGnd;
	context->rTotal = 1.0 / context->rTotal;

	node->output = 0;

	if (info->cFilter) {

		context->exponent = -1.0 / (context->rTotal * info->cFilter * discrete_current_context->sample_rate);
		context->exponent = 1.0 - exp(context->exponent);
	}
}

#define mw_DST_DIODE_MIX__ENABLE                (*(node->input[0]))
#define mw_DST_DIODE_MIX__VJUNC         (*(node->input[1]))
#define mw_DST_DIODE_MIX__INP(addr)     (*(node->input[2 + addr]))

void mw_dst_diode_mix_step(node_description *node)
{
	struct mw_dst_size_context *context = node->context;
	double max = 0;
	int addr;

	if (mw_DST_DIODE_MIX__ENABLE) {
		for (addr = 0; addr < context->size; addr++) {
			if (mw_DST_DIODE_MIX__INP(addr) > max) max = mw_DST_DIODE_MIX__INP(addr);
		}
		node->output = max - mw_DST_DIODE_MIX__VJUNC;
		if (node->output < 0) node->output = 0;
	} else {
		node->output = 0;
	}
}

void mw_dst_diode_mix_reset(node_description *node)
{
	struct mw_dst_size_context *context = node->context;

	context->size = node->active_inputs - 2;

	mw_dst_diode_mix_step(node);
}

#define mw_DST_DIVIDE__ENABLE   (*(node->input[0]))
#define mw_DST_DIVIDE__IN               (*(node->input[1]))
#define mw_DST_DIVIDE__DIV              (*(node->input[2]))

void mw_dst_divide_step(node_description *node)
{
	if (mw_DST_DIVIDE__ENABLE) {
		if (mw_DST_DIVIDE__DIV == 0) {
			node->output = DBL_MAX;
			discrete_log("mw_dst_divider_step() - Divide by Zero attempted in NODE_%02d.\n", node->node - NODE_START);
		} else {
			node->output = mw_DST_DIVIDE__IN / mw_DST_DIVIDE__DIV;
		}
	} else {
		node->output = 0;
	}
}

#define mw_DST_GAIN__ENABLE     (*(node->input[0]))
#define mw_DST_GAIN__IN         (*(node->input[1]))
#define mw_DST_GAIN__GAIN               (*(node->input[2]))
#define mw_DST_GAIN__OFFSET     (*(node->input[3]))

void mw_dst_gain_step(node_description *node)
{
	if (mw_DST_GAIN__ENABLE) {
		node->output = mw_DST_GAIN__IN * mw_DST_GAIN__GAIN;
		node->output += mw_DST_GAIN__OFFSET;
	} else {
		node->output = 0;
	}
}

#define mw_DST_INTEGRATE__TRG0  (*(node->input[0]))
#define mw_DST_INTEGRATE__TRG1  (*(node->input[1]))

int mw_dst_trigger_function(int trig0, int trig1, int trig2, int function)
{
	int result = 1;
	switch (function) {
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0:
		result = trig0;
		break;
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0_INV:
		result = !trig0;
		break;
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1:
		result = trig1;
		break;
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1_INV:
		result = !trig1;
		break;
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG2:
		result = trig2;
		break;
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG2_INV:
		result = !trig2;
		break;
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG01_AND:
		result = trig0 && trig1;
		break;
	case MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG01_NAND:
		result = !(trig0 && trig1);
		break;
	}

	return (result);
}

void mw_dst_integrate_step(node_description *node)
{
	const mw_discrete_integrate_info *info = node->custom;
	struct mw_dst_integrate_context *context = node->context;

	int trig0, trig1;
	double iNeg = 0;
	double iPos = 0;

	switch (info->type) {
	case MW_DISC_INTEGRATE_OP_AMP_1:
		if (mw_DST_INTEGRATE__TRG0 != 0) {

			node->output = context->vMaxOut;
			return;
		}
		node->output -= context->change;
		break;

	case MW_DISC_INTEGRATE_OP_AMP_1 | MW_DISC_OP_AMP_IS_NORTON:
		iNeg = context->vMaxIn / info->r1;
		iPos = (mw_DST_INTEGRATE__TRG0 - MW_OP_AMP_NORTON_VBE) / info->r2;
		if (iPos < 0) iPos = 0;
		node->output += (iPos - iNeg) / discrete_current_context->sample_rate / info->c;
		break;

	case MW_DISC_INTEGRATE_OP_AMP_2 | MW_DISC_OP_AMP_IS_NORTON:
		trig0 = (int)mw_DST_INTEGRATE__TRG0;
		trig1 = (int)mw_DST_INTEGRATE__TRG1;
		iNeg = mw_dst_trigger_function(trig0, trig1, 0, info->f0) ? context->vMaxInD / info->r1 : 0;
		iPos = mw_dst_trigger_function(trig0, trig1, 0, info->f1) ? context->vMaxIn / info->r2 : 0;
		iPos += mw_dst_trigger_function(trig0, trig1, 0, info->f2) ? context->vMaxInD / info->r3 : 0;
		node->output += (iPos - iNeg) / discrete_current_context->sample_rate / info->c;
		break;
	}

	if (node->output < 0) node->output = 0;
	if (node->output > context->vMaxOut) node->output = context->vMaxOut;
}

void mw_dst_integrate_reset(node_description *node)
{
	const mw_discrete_integrate_info *info = node->custom;
	struct mw_dst_integrate_context *context = node->context;
	double i, v;

	if (info->type & MW_DISC_OP_AMP_IS_NORTON) {
		context->vMaxOut = info->vP - MW_OP_AMP_NORTON_VBE;
		context->vMaxIn = info->v1 - MW_OP_AMP_NORTON_VBE;
		context->vMaxInD = context->vMaxIn - MW_OP_AMP_NORTON_VBE;
	} else {
		context->vMaxOut = info->vP - MW_OP_AMP_VP_RAIL_OFFSET;

		v = info->v1 * info->r3 / (info->r2 + info->r3);
		v = info->v1 - v;
		i = v / info->r1;
		context->change = i / discrete_current_context->sample_rate / info->c;
	}
	node->output = 0;
}

#define mw_DST_LOGIC_INV__ENABLE        (*(node->input[0]))
#define mw_DST_LOGIC_INV__IN            (*(node->input[1]))

void mw_dst_logic_inv_step(node_description *node)
{
	if (mw_DST_LOGIC_INV__ENABLE) {
		node->output = mw_DST_LOGIC_INV__IN ? 0.0 : 1.0;
	} else {
		node->output = 0.0;
	}
}

#define mw_DST_LOGIC_AND__ENABLE        (*(node->input[0]))
#define mw_DST_LOGIC_AND__IN0           (*(node->input[1]))
#define mw_DST_LOGIC_AND__IN1           (*(node->input[2]))
#define mw_DST_LOGIC_AND__IN2           (*(node->input[3]))
#define mw_DST_LOGIC_AND__IN3           (*(node->input[4]))

void mw_dst_logic_and_step(node_description *node)
{
	if (mw_DST_LOGIC_AND__ENABLE) {
		node->output = (mw_DST_LOGIC_AND__IN0 && mw_DST_LOGIC_AND__IN1 && mw_DST_LOGIC_AND__IN2 && mw_DST_LOGIC_AND__IN3)? 1.0 : 0.0;
	} else {
		node->output = 0.0;
	}
}

#define mw_DST_LOGIC_NAND__ENABLE       (*(node->input[0]))
#define mw_DST_LOGIC_NAND__IN0          (*(node->input[1]))
#define mw_DST_LOGIC_NAND__IN1          (*(node->input[2]))
#define mw_DST_LOGIC_NAND__IN2          (*(node->input[3]))
#define mw_DST_LOGIC_NAND__IN3          (*(node->input[4]))

void mw_dst_logic_nand_step(node_description *node)
{
	if (mw_DST_LOGIC_NAND__ENABLE) {
		node->output = (mw_DST_LOGIC_NAND__IN0 && mw_DST_LOGIC_NAND__IN1 && mw_DST_LOGIC_NAND__IN2 && mw_DST_LOGIC_NAND__IN3)? 0.0 : 1.0;
	} else {
		node->output = 0.0;
	}
}

#define mw_DST_LOGIC_OR__ENABLE (*(node->input[0]))
#define mw_DST_LOGIC_OR__IN0            (*(node->input[1]))
#define mw_DST_LOGIC_OR__IN1            (*(node->input[2]))
#define mw_DST_LOGIC_OR__IN2            (*(node->input[3]))
#define mw_DST_LOGIC_OR__IN3            (*(node->input[4]))

void mw_dst_logic_or_step(node_description *node)
{
	if (mw_DST_LOGIC_OR__ENABLE) {
		node->output = (mw_DST_LOGIC_OR__IN0 || mw_DST_LOGIC_OR__IN1 || mw_DST_LOGIC_OR__IN2 || mw_DST_LOGIC_OR__IN3) ? 1.0 : 0.0;
	} else {
		node->output = 0.0;
	}
}

#define mw_DST_LOGIC_NOR__ENABLE        (*(node->input[0]))
#define mw_DST_LOGIC_NOR__IN0           (*(node->input[1]))
#define mw_DST_LOGIC_NOR__IN1           (*(node->input[2]))
#define mw_DST_LOGIC_NOR__IN2           (*(node->input[3]))
#define mw_DST_LOGIC_NOR__IN3           (*(node->input[4]))

void mw_dst_logic_nor_step(node_description *node)
{
	if (mw_DST_LOGIC_NOR__ENABLE) {
		node->output = (mw_DST_LOGIC_NOR__IN0 || mw_DST_LOGIC_NOR__IN1 || mw_DST_LOGIC_NOR__IN2 || mw_DST_LOGIC_NOR__IN3) ? 0.0 : 1.0;
	} else {
		node->output = 0.0;
	}
}

#define mw_DST_LOGIC_XOR__ENABLE        (*(node->input[0]))
#define mw_DST_LOGIC_XOR__IN0           (*(node->input[1]))
#define mw_DST_LOGIC_XOR__IN1           (*(node->input[2]))

void mw_dst_logic_xor_step(node_description *node)
{
	if (mw_DST_LOGIC_XOR__ENABLE) {
		node->output = ((mw_DST_LOGIC_XOR__IN0 && !mw_DST_LOGIC_XOR__IN1) || (!mw_DST_LOGIC_XOR__IN0 && mw_DST_LOGIC_XOR__IN1)) ? 1.0 : 0.0;
	} else {
		node->output = 0.0;
	}
}

#define mw_DST_LOGIC_XNOR__ENABLE       (*(node->input[0]))
#define mw_DST_LOGIC_XNOR__IN0          (*(node->input[1]))
#define mw_DST_LOGIC_XNOR__IN1          (*(node->input[2]))

void mw_dst_logic_nxor_step(node_description *node)
{
	if (mw_DST_LOGIC_XNOR__ENABLE) {
		node->output = ((mw_DST_LOGIC_XNOR__IN0 && !mw_DST_LOGIC_XNOR__IN1) || (!mw_DST_LOGIC_XNOR__IN0 && mw_DST_LOGIC_XNOR__IN1)) ? 0.0 : 1.0;
	} else {
		node->output = 0.0;
	}
}

#define mw_DST_LOGIC_DFF__ENABLE         (*(node->input[0]))
#define mw_DST_LOGIC_DFF__RESET !(*(node->input[1]))
#define mw_DST_LOGIC_DFF__SET           !(*(node->input[2]))
#define mw_DST_LOGIC_DFF__CLOCK  (*(node->input[3]))
#define mw_DST_LOGIC_DFF__DATA   (*(node->input[4]))

void mw_dst_logic_dff_step(node_description *node)
{
	struct mw_dst_flipflop_context *context = node->context;
	int clk = (int)mw_DST_LOGIC_DFF__CLOCK;

	if (mw_DST_LOGIC_DFF__ENABLE) {
		if (mw_DST_LOGIC_DFF__RESET)
			node->output = 0;
		else if (mw_DST_LOGIC_DFF__SET)
			node->output = 1;
		else if (!context->last_clk && clk) {
			node->output = mw_DST_LOGIC_DFF__DATA;
		}
	} else {
		node->output = 0;
	}
	context->last_clk = clk;
}

void mw_dst_logic_ff_reset(node_description *node)
{
	struct mw_dst_flipflop_context *context = node->context;
	context->last_clk = 0;
	node->output = 0;
}

#define mw_DST_LOGIC_JKFF__ENABLE        (*(node->input[0]))
#define mw_DST_LOGIC_JKFF__RESET        !(*(node->input[1]))
#define mw_DST_LOGIC_JKFF__SET          !(*(node->input[2]))
#define mw_DST_LOGIC_JKFF__CLOCK         (*(node->input[3]))
#define mw_DST_LOGIC_JKFF__J             (*(node->input[4]))
#define mw_DST_LOGIC_JKFF__K             (*(node->input[5]))

void mw_dst_logic_jkff_step(node_description *node)
{
	struct mw_dst_flipflop_context *context = node->context;
	int clk = (int)mw_DST_LOGIC_JKFF__CLOCK;
	int j = (int)mw_DST_LOGIC_JKFF__J;
	int k = (int)mw_DST_LOGIC_JKFF__K;

	if (mw_DST_LOGIC_JKFF__ENABLE) {
		if (mw_DST_LOGIC_JKFF__RESET)
			node->output = 0;
		else if (mw_DST_LOGIC_JKFF__SET)
			node->output = 1;
		else if (context->last_clk && !clk) {
			if (!j) {

				if (k)

					node->output = 0;
			} else {
				if (!k)

					node->output = 1;
				else

					node->output = !(int)node->output;
			}
		}
	} else {
		node->output = 0;
	}
	context->last_clk = clk;
}

#define mw_DST_LOOKUP_TABLE__ENABLE     (*(node->input[0]))
#define mw_DST_LOOKUP_TABLE__IN         (*(node->input[1]))
#define mw_DST_LOOKUP_TABLE__SIZE               (*(node->input[2]))

void mw_dst_lookup_table_step(node_description *node)
{
	const double *table = node->custom;
	int addr = mw_DST_LOOKUP_TABLE__IN;

	if (!mw_DST_LOOKUP_TABLE__ENABLE || addr < 0 || addr >= mw_DST_LOOKUP_TABLE__SIZE)
		node->output = 0;
	else
		node->output = table[addr];
}

#define mw_DST_MIXER__ENABLE            (*(node->input[0]))
#define mw_DST_MIXER__IN(bit)           (*(node->input[bit + 1]))

void mw_dst_mixer_step(node_description *node)
{
	const mw_discrete_mixer_desc *info = node->custom;
	struct mw_dst_mixer_context *context = node->context;

	double v, vTemp, rTotal, rTemp, rTemp2 = 0;
	double i = 0;
	int bit, connected;

	if (mw_DST_MIXER__ENABLE) {
		rTotal = context->rTotal;

		for (bit = 0; bit < context->size; bit++) {
			rTemp = info->r[bit];
			connected = 1;
			vTemp = mw_DST_MIXER__IN(bit);

			if (info->rNode[bit]) {

				if (*context->rNode[bit] == 0)
					connected = 0;
				else {
					rTemp += *context->rNode[bit];
					rTotal += 1.0 / rTemp;
					if (info->c[bit] != 0) {
						switch (context->type & MW_DISC_MIXER_TYPE_MASK) {
						case MW_DISC_MIXER_IS_RESISTOR:

							if (info->rF != 0) {
								rTemp2 = 1.0 / ((1.0 / rTemp) + (1.0 / info->rF));
								break;
							}

						case MW_DISC_MIXER_IS_OP_AMP:
							rTemp2 = rTemp;
							break;
						case MW_DISC_MIXER_IS_OP_AMP_WITH_RI:
							rTemp2 = rTemp + info->rI;
							break;
						}

						context->exponent_rc[bit] = -1.0 / (rTemp2 * info->c[bit] * discrete_current_context->sample_rate);
						context->exponent_rc[bit] = 1.0 - exp(context->exponent_rc[bit]);
					}
				}
			}

			if (connected) {
				if (info->c[bit] != 0) {

					context->vCap[bit] += (vTemp - info->vRef - context->vCap[bit]) * context->exponent_rc[bit];
					vTemp -= context->vCap[bit];
				}
				i += (((context->type & MW_DISC_MIXER_TYPE_MASK) == MW_DISC_MIXER_IS_OP_AMP) ? info->vRef - vTemp : vTemp) / rTemp;
			}
		}

		if ((context->type & MW_DISC_MIXER_TYPE_MASK) == MW_DISC_MIXER_IS_OP_AMP_WITH_RI) i += info->vRef / info->rI;
		rTotal = 1.0 / rTotal;

		v = i * (((context->type & MW_DISC_MIXER_TYPE_MASK) == MW_DISC_MIXER_IS_OP_AMP) ? info->rF : rTotal);

		if ((context->type & MW_DISC_MIXER_TYPE_MASK) == MW_DISC_MIXER_IS_OP_AMP_WITH_RI)
			v = info->vRef + (context->gain * (info->vRef - v));

		if (info->cF != 0) {
			if (context->type & MW_DISC_MIXER_HAS_R_NODE) {

				context->exponent_cF = -1.0 / (rTotal * info->cF * discrete_current_context->sample_rate);
				context->exponent_cF = 1.0 - exp(context->exponent_cF);
			}
			context->vCapF += (v - info->vRef - context->vCapF) * context->exponent_cF;
			v = context->vCapF;
		}

		if (info->cAmp != 0) {
			context->vCapAmp += (v - context->vCapAmp) * context->exponent_cAmp;
			v -= context->vCapAmp;
		}
		node->output = v * info->gain;
	} else {
		node->output = 0;
	}
}

void mw_dst_mixer_reset(node_description *node)
{
	const mw_discrete_mixer_desc *info = node->custom;
	struct mw_dst_mixer_context *context = node->context;
	node_description *r_node;

	int bit;
	double rTemp = 0;

	for (bit = 0; bit < 8; bit++) {
		r_node = discrete_find_node(NULL, info->rNode[bit]);
		if (r_node)
			context->rNode[bit] = &(r_node->output);
		else
			context->rNode[bit] = NULL;
	}

	context->size = node->active_inputs - 1;

	context->type = ((info->type == MW_DISC_MIXER_IS_OP_AMP) && info->rI) ? MW_DISC_MIXER_IS_OP_AMP_WITH_RI : info->type;

	context->rTotal = 0;
	for (bit = 0; bit < context->size; bit++) {
		if (info->rNode[bit])
			context->type = context->type | MW_DISC_MIXER_HAS_R_NODE;

		if ((info->r[bit] != 0) && !info->rNode[bit]) {
			context->rTotal += 1.0 / info->r[bit];
		}

		context->vCap[bit] = 0;
		context->exponent_rc[bit] = 0;
		if ((info->c[bit] != 0) && !info->rNode[bit]) {
			switch (context->type) {
			case MW_DISC_MIXER_IS_RESISTOR:

				if (info->rF != 0) {
					rTemp = 1.0 / ((1.0 / info->r[bit]) + (1.0 / info->rF));
					break;
				}

			case MW_DISC_MIXER_IS_OP_AMP:
				rTemp = info->r[bit];
				break;
			case MW_DISC_MIXER_IS_OP_AMP_WITH_RI:
				rTemp = info->r[bit] + info->rI;
				break;
			}

			context->exponent_rc[bit] = -1.0 / (rTemp * info->c[bit] * discrete_current_context->sample_rate);
			context->exponent_rc[bit] = 1.0 - exp(context->exponent_rc[bit]);
		}
	}

	if (info->rF != 0) {
		if (info->type == MW_DISC_MIXER_IS_RESISTOR) context->rTotal += 1.0 / info->rF;
	}
	if (context->type == MW_DISC_MIXER_IS_OP_AMP_WITH_RI) context->rTotal += 1.0 / info->rI;

	context->vCapF = 0;
	context->exponent_cF = 0;
	if (info->cF != 0) {

		context->exponent_cF = -1.0 / (((info->type == MW_DISC_MIXER_IS_OP_AMP) ? info->rF : (1.0 / context->rTotal)) * info->cF * discrete_current_context->sample_rate);
		context->exponent_cF = 1.0 - exp(context->exponent_cF);
	}

	context->vCapAmp = 0;
	context->exponent_cAmp = 0;
	if (info->cAmp != 0) {

		context->exponent_cAmp = -1.0 / (100000 * info->cAmp * discrete_current_context->sample_rate);
		context->exponent_cAmp = 1.0 - exp(context->exponent_cAmp);
	}

	if ((context->type & MW_DISC_MIXER_TYPE_MASK) == MW_DISC_MIXER_IS_OP_AMP_WITH_RI) context->gain = info->rF / info->rI;

	node->output = 0;
}

#define mw_DST_MULTIPLEX__ENABLE                (*(node->input[0]))
#define mw_DST_MULTIPLEX__ADDR                  (*(node->input[1]))
#define mw_DST_MULTIPLEX__INP(addr)     (*(node->input[2 + addr]))

void mw_dst_multiplex_step(node_description *node)
{
	struct mw_dst_size_context *context = node->context;
	int addr;

	if (mw_DST_MULTIPLEX__ENABLE) {
		addr = mw_DST_MULTIPLEX__ADDR;
		if ((addr >= 0) && (addr < context->size)) {
			node->output = mw_DST_MULTIPLEX__INP(addr);
		} else {

			discrete_log("NODE_%02d - Address = %d. Out of bounds\n", node->node - NODE_00, addr);
		}
	} else {
		node->output = 0;
	}
}

void mw_dst_multiplex_reset(node_description *node)
{
	struct mw_dst_size_context *context = node->context;

	context->size = node->active_inputs - 2;

	mw_dst_multiplex_step(node);
}

#define mw_DST_ONESHOT__RESET   (*(node->input[0]))
#define mw_DST_ONESHOT__TRIG    (*(node->input[1]))
#define mw_DST_ONESHOT__AMP     (*(node->input[2]))
#define mw_DST_ONESHOT__WIDTH   (*(node->input[3]))
#define mw_DST_ONESHOT__TYPE    (int)(*(node->input[4]))

void mw_dst_oneshot_step(node_description *node)
{
	struct mw_dst_oneshot_context *context = node->context;
	int trigger = (mw_DST_ONESHOT__TRIG != 0);

	int doCount = context->state;

	if (mw_DST_ONESHOT__RESET) {

		node->output = 0;
		context->state = 0;
	} else {

		if (trigger != context->lastTrig) {

			context->lastTrig = trigger;

			if ((mw_DST_ONESHOT__TYPE & MW_DISC_ONESHOT_REDGE) ? trigger : !trigger) {
				if (!context->state) {

					context->state = 1;
					node->output = (mw_DST_ONESHOT__TYPE & MW_DISC_OUT_ACTIVE_LOW) ? 0 : mw_DST_ONESHOT__AMP;
					context->countdown = mw_DST_ONESHOT__WIDTH;
				} else {

					if (mw_DST_ONESHOT__TYPE & MW_DISC_ONESHOT_RETRIG) {

						context->countdown = mw_DST_ONESHOT__WIDTH;
						doCount = 0;
					}
				}
			}
		}

		if (doCount) {
			context->countdown -= discrete_current_context->sample_time;
			if (context->countdown <= 0.0) {
				node->output = (mw_DST_ONESHOT__TYPE & MW_DISC_OUT_ACTIVE_LOW) ? mw_DST_ONESHOT__AMP : 0;
				context->countdown = 0;
				context->state = 0;
			}
		}
	}
}

void mw_dst_oneshot_reset(node_description *node)
{
	struct mw_dst_oneshot_context *context = node->context;
	context->countdown = 0;
	context->state = 0;

	context->lastTrig = 0;
	node->output = (mw_DST_ONESHOT__TYPE & MW_DISC_OUT_ACTIVE_LOW) ? mw_DST_ONESHOT__AMP : 0;
}

#define mw_DST_RAMP__ENABLE     (*(node->input[0]))
#define mw_DST_RAMP__DIR                (*(node->input[1]))
#define mw_DST_RAMP__GRAD               (*(node->input[2]))
#define mw_DST_RAMP__START              (*(node->input[3]))
#define mw_DST_RAMP__END                (*(node->input[4]))
#define mw_DST_RAMP__CLAMP              (*(node->input[5]))

void mw_dst_ramp_step(node_description *node)
{
	struct mw_dss_ramp_context *context = node->context;

	if (mw_DST_RAMP__ENABLE) {
		if (!context->last_en) {
			context->last_en = 1;
			node->output = mw_DST_RAMP__START;
		}
		if (context->dir ? mw_DST_RAMP__DIR : !mw_DST_RAMP__DIR) node->output += context->step;
		else node->output -= context->step;

		if (context->dir ? (node->output < mw_DST_RAMP__START)
				: (node->output > mw_DST_RAMP__START)) node->output = mw_DST_RAMP__START;
		if (context->dir ? (node->output > mw_DST_RAMP__END)
				: (node->output < mw_DST_RAMP__END)) node->output = mw_DST_RAMP__END;
	} else {
		context->last_en = 0;

		node->output = mw_DST_RAMP__CLAMP;
	}
}

void mw_dst_ramp_reset(node_description *node)
{
	struct mw_dss_ramp_context *context = node->context;

	node->output = mw_DST_RAMP__CLAMP;
	context->step = mw_DST_RAMP__GRAD / discrete_current_context->sample_rate;
	context->dir = ((mw_DST_RAMP__END - mw_DST_RAMP__START) == abs(mw_DST_RAMP__END - mw_DST_RAMP__START));
	context->last_en = 0;
}

#define mw_DST_SAMPHOLD__ENABLE (*(node->input[0]))
#define mw_DST_SAMPHOLD__IN0            (*(node->input[1]))
#define mw_DST_SAMPHOLD__CLOCK          (*(node->input[2]))
#define mw_DST_SAMPHOLD__TYPE           (*(node->input[3]))

void mw_dst_samphold_step(node_description *node)
{
	struct mw_dst_samphold_context *context = node->context;

	if (mw_DST_SAMPHOLD__ENABLE) {
		switch (context->clocktype) {
		case MW_DISC_SAMPHOLD_REDGE:

			if (mw_DST_SAMPHOLD__CLOCK > context->lastinput) node->output = mw_DST_SAMPHOLD__IN0;
			break;
		case MW_DISC_SAMPHOLD_FEDGE:

			if (mw_DST_SAMPHOLD__CLOCK < context->lastinput) node->output = mw_DST_SAMPHOLD__IN0;
			break;
		case MW_DISC_SAMPHOLD_HLATCH:

			if (mw_DST_SAMPHOLD__CLOCK) node->output = mw_DST_SAMPHOLD__IN0;
			break;
		case MW_DISC_SAMPHOLD_LLATCH:

			if (mw_DST_SAMPHOLD__CLOCK == 0) node->output = mw_DST_SAMPHOLD__IN0;
			break;
		default:
			discrete_log("mw_dst_samphold_step - Invalid clocktype passed");
			break;
		}
	} else {
		node->output = 0;
	}

	context->lastinput = mw_DST_SAMPHOLD__CLOCK;
}

void mw_dst_samphold_reset(node_description *node)
{
	struct mw_dst_samphold_context *context = node->context;

	node->output = 0;
	context->lastinput = -1;

	context->clocktype = (int)mw_DST_SAMPHOLD__TYPE;
	mw_dst_samphold_step(node);
}

#define mw_DSS_SWITCH__ENABLE   (*(node->input[0]))
#define mw_DSS_SWITCH__SWITCH   (*(node->input[1]))
#define mw_DSS_SWITCH__IN0              (*(node->input[2]))
#define mw_DSS_SWITCH__IN1              (*(node->input[3]))

void mw_dst_switch_step(node_description *node)
{
	if (mw_DSS_SWITCH__ENABLE) {
		node->output = mw_DSS_SWITCH__SWITCH ? mw_DSS_SWITCH__IN1 : mw_DSS_SWITCH__IN0;
	} else {
		node->output = 0;
	}
}

#define mw_DSS_ASWITCH__ENABLE          (*(node->input[0]))
#define mw_DSS_ASWITCH__CTRL            (*(node->input[1]))
#define mw_DSS_ASWITCH__IN                      (*(node->input[2]))
#define mw_DSS_ASWITCH__THRESHOLD       (*(node->input[3]))

void mw_dst_aswitch_step(node_description *node)
{
	if (mw_DSS_SWITCH__ENABLE) {
		node->output = mw_DSS_ASWITCH__CTRL > mw_DSS_ASWITCH__THRESHOLD ? mw_DSS_ASWITCH__IN : 0;
	} else {
		node->output = 0;
	}
}

#define mw_DST_TRANSFORM__ENABLE        (*(node->input[0]))
#define mw_DST_TRANSFORM__IN0           (*(node->input[1]))
#define mw_DST_TRANSFORM__IN1           (*(node->input[2]))
#define mw_DST_TRANSFORM__IN2           (*(node->input[3]))
#define mw_DST_TRANSFORM__IN3           (*(node->input[4]))
#define mw_DST_TRANSFORM__IN4           (*(node->input[5]))

#define MAX_TRANS_STACK 16

double mw_dst_transform_pop(double *stack, int *pointer)
{
	double value;

	if (*pointer > 0) (*pointer)--;
	value = stack[*pointer];
	return value;
}

double mw_dst_transform_push(double *stack, int *pointer, double value)
{

	if (*pointer < MAX_TRANS_STACK) stack[(*pointer)++] = value;
	return value;
}

void mw_dst_transform_step(node_description *node)
{
	if (mw_DST_TRANSFORM__ENABLE) {
		double trans_stack[MAX_TRANS_STACK];
		double result, number1, number2;
		int trans_stack_ptr = 0;

		const char *fPTR = node->custom;
		node->output = 0;

		while (*fPTR != 0) {
			switch (*fPTR++) {
			case '*':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = number1 * number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '/':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = number1 / number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '+':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = number1 + number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '-':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = number1 - number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '0':
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, mw_DST_TRANSFORM__IN0);
				break;
			case '1':
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, mw_DST_TRANSFORM__IN1);
				break;
			case '2':
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, mw_DST_TRANSFORM__IN2);
				break;
			case '3':
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, mw_DST_TRANSFORM__IN3);
				break;
			case '4':
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, mw_DST_TRANSFORM__IN4);
				break;
			case 'P':
				result = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case 'i':
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = -number1;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '!':
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = !number1;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '=':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = (int)number1 == (int)number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '>':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = number1 > number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '<':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = number1 < number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '&':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = (int)number1 & (int)number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '|':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = (int)number1 | (int)number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			case '^':
				number2 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				number1 = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
				result = (int)number1 ^ (int)number2;
				mw_dst_transform_push(trans_stack, &trans_stack_ptr, result);
				break;
			default:
				discrete_log("mw_dst_transform_step - Invalid function type/variable passed");
				node->output = 0;
				break;
			}
		}
		node->output = mw_dst_transform_pop(trans_stack, &trans_stack_ptr);
	} else {
		node->output = 0;
	}
}

#define mw_DST_OP_AMP__ENABLE   (*(node->input[0]))
#define mw_DST_OP_AMP__INP0     (*(node->input[1]))
#define mw_DST_OP_AMP__INP1     (*(node->input[2]))

void mw_dst_op_amp_step(node_description *node)
{
	const mw_discrete_op_amp_info *info = node->custom;
	struct mw_dst_op_amp_context *context = node->context;

	double iPos = 0;
	double iNeg = 0;
	double i = 0;

	if (mw_DST_OP_AMP__ENABLE) {
		switch (info->type) {
		case MW_DISC_OP_AMP_IS_NORTON:

			if (context->has_r1) {
				iNeg = (mw_DST_OP_AMP__INP0 - MW_OP_AMP_NORTON_VBE) / info->r1;
				if (iNeg < 0) iNeg = 0;
			}
			iNeg += context->iFixed;

			iPos = (mw_DST_OP_AMP__INP1 - MW_OP_AMP_NORTON_VBE) / info->r2;
			if (iPos < 0) iPos = 0;

			i = iPos - iNeg;

			if (context->has_cap) {
				if (context->has_r4) {

					i *= info->r4;

					context->vCap += (i - context->vCap) * context->exponent;
				} else

					context->vCap += i / context->exponent;
				node->output = context->vCap;
			} else
				node->output = i * info->r4;

			if (node->output > context->vMax) node->output = context->vMax;
			else if (node->output < info->vN) node->output = info->vN;
			context->vCap = node->output;
			break;

		default:
			node->output = 0;
		}
	} else
		node->output = 0;
}

void mw_dst_op_amp_reset(node_description *node)
{
	const mw_discrete_op_amp_info *info = node->custom;
	struct mw_dst_op_amp_context *context = node->context;

	context->has_r1 = info->r1 > 0;
	context->has_r4 = info->r4 > 0;

	context->vMax = info->vP - MW_OP_AMP_NORTON_VBE;

	context->vCap = 0;
	if (info->c > 0) {
		context->has_cap = 1;

		if (context->has_r4) {

			context->exponent = -1.0 / (info->r4 * info->c * discrete_current_context->sample_rate);
			context->exponent = 1.0 - exp(context->exponent);
		} else

			context->exponent = discrete_current_context->sample_rate * info->c;
	}

	if (info->r3 >= 0)
		context->iFixed = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r3;
}

#define mw_DST_OP_AMP_1SHT__TRIGGER     (*(node->input[0]))

void mw_dst_op_amp_1sht_step(node_description *node)
{
	const mw_discrete_op_amp_1sht_info *info = node->custom;
	struct mw_dst_op_amp_1sht_context *context = node->context;

	double iPos;
	double iNeg;
	double v;

	iPos = (mw_DST_OP_AMP_1SHT__TRIGGER - context->vCap2) / info->r2;
	iPos += node->output / info->r5;
	context->vCap2 += (mw_DST_OP_AMP_1SHT__TRIGGER - context->vCap2) * context->exponent2;

	iNeg = (context->vCap1 - MW_OP_AMP_NORTON_VBE) / info->r3;
	if (iNeg < 0) iNeg = 0;
	iNeg += context->iFixed;

	if (iPos > iNeg) node->output = context->vMax;
	else node->output = info->vN;

	v = node->output + 0.6;
	if (context->vCap1 > node->output) {

		if (context->vCap1 > v)

			context->vCap1 = v;
		else

			context->vCap1 += (node->output - context->vCap1) * context->exponent1d;
	} else

		context->vCap1 += ((node->output - MW_OP_AMP_NORTON_VBE) * context->r34ratio + MW_OP_AMP_NORTON_VBE - context->vCap1) * context->exponent1c;
}

void mw_dst_op_amp_1sht_reset(node_description *node)
{
	const mw_discrete_op_amp_1sht_info *info = node->custom;
	struct mw_dst_op_amp_1sht_context *context = node->context;

	context->exponent1c = -1.0 / ((1.0 / (1.0 / info->r3 + 1.0 / info->r4)) * info->c1 * discrete_current_context->sample_rate);
	context->exponent1c = 1.0 - exp(context->exponent1c);
	context->exponent1d = -1.0 / (info->r4 * info->c1 * discrete_current_context->sample_rate);
	context->exponent1d = 1.0 - exp(context->exponent1d);
	context->exponent2 = -1.0 / (info->r2 * info->c2 * discrete_current_context->sample_rate);
	context->exponent2 = 1.0 - exp(context->exponent2);
	context->iFixed = (info->vP - MW_OP_AMP_NORTON_VBE) / info->r1;
	context->vCap1 = context->vCap2 = 0;
	context->vMax = info->vP - MW_OP_AMP_NORTON_VBE;
	context->r34ratio = info->r3 / (info->r3 + info->r4);
}

#define mw_DST_TVCA_OP_AMP__TRG0        (*(node->input[0]))
#define mw_DST_TVCA_OP_AMP__TRG1        (*(node->input[1]))
#define mw_DST_TVCA_OP_AMP__TRG2        (*(node->input[2]))
#define mw_DST_TVCA_OP_AMP__INP0        (*(node->input[3]))
#define mw_DST_TVCA_OP_AMP__INP1        (*(node->input[4]))

void mw_dst_tvca_op_amp_step(node_description *node)
{
	const mw_discrete_op_amp_tvca_info *info = node->custom;
	struct mw_dst_tvca_op_amp_context *context = node->context;

	int trig0, trig1, trig2, f3;
	double i2 = 0;
	double i3 = 0;
	double iNeg = 0;
	double iPos = 0;
	double iOut = 0;

	trig0 = (int)mw_DST_TVCA_OP_AMP__TRG0;
	trig1 = (int)mw_DST_TVCA_OP_AMP__TRG1;
	trig2 = (int)mw_DST_TVCA_OP_AMP__TRG2;
	f3 = mw_dst_trigger_function(trig0, trig1, trig2, info->f3);

	if ((info->r2 != 0) && mw_dst_trigger_function(trig0, trig1, trig2, info->f0)) {

		i2 = (mw_DST_TVCA_OP_AMP__INP0 - MW_OP_AMP_NORTON_VBE) / info->r2;
		if (i2 < 0) i2 = 0;
	}

	if ((info->r3 != 0) && mw_dst_trigger_function(trig0, trig1, trig2, info->f1)) {

		i3 = (mw_DST_TVCA_OP_AMP__INP1 - MW_OP_AMP_NORTON_VBE) / info->r3;
		if (i3 < 0) i3 = 0;
	}

	iNeg = context->iFixed + i2 + i3;

	if (mw_dst_trigger_function(trig0, trig1, trig2, info->f2)) {

		context->vCap1 += (context->vTrig[f3] - context->vCap1) * context->exponentC[f3];
	} else {

		context->vCap1 += ((f3 ? MW_OP_AMP_NORTON_VBE : 0.0) - context->vCap1) * context->exponentD[f3];
	}

	iPos = (context->vCap1 - MW_OP_AMP_NORTON_VBE) / context->r67;
	if ((iPos < 0) || !f3) iPos = 0;

	if (info->r9 != 0) {
		f3 = mw_dst_trigger_function(trig0, trig1, trig2, info->f4);
		context->vCap2 += ((f3 ? context->vTrig2 : 0) - context->vCap2) * context->exponent2[f3];
		iPos += context->vCap2 / info->r9;
	}

	if (info->r11 != 0) {
		f3 = mw_dst_trigger_function(trig0, trig1, trig2, info->f5);
		context->vCap3 += ((f3 ? context->vTrig3 : 0) - context->vCap3) * context->exponent3[f3];
		iPos += context->vCap3 / info->r11;
	}

	iOut = iPos - iNeg;
	if (iOut < 0) iOut = 0;

	node->output = iOut * info->r4;

	if (node->output > context->vOutMax) node->output = context->vOutMax;
}

void mw_dst_tvca_op_amp_reset(node_description *node)
{
	const mw_discrete_op_amp_tvca_info *info = node->custom;
	struct mw_dst_tvca_op_amp_context *context = node->context;

	context->r67 = info->r6 + info->r7;

	context->vOutMax = info->vP - MW_OP_AMP_NORTON_VBE;

	context->vTrig[0] = (info->v1 - 0.6) * (info->r6 / (info->r6 + info->r5));
	context->vTrig[1] = (info->v1 - 0.6 - MW_OP_AMP_NORTON_VBE) * (context->r67 / (context->r67 + info->r5)) + MW_OP_AMP_NORTON_VBE;
	context->iFixed = context->vOutMax / info->r1;

	context->vCap1 = 0;

	context->exponentC[0] = -1.0 / ((1.0 / (1.0 / info->r5 + 1.0 / info->r6)) * info->c1 * discrete_current_context->sample_rate);
	context->exponentC[0] = 1.0 - exp(context->exponentC[0]);
	context->exponentC[1] = -1.0 / ((1.0 / (1.0 / info->r5 + 1.0 / context->r67)) * info->c1 * discrete_current_context->sample_rate);
	context->exponentC[1] = 1.0 - exp(context->exponentC[1]);

	context->exponentD[1] = -1.0 / (context->r67 * info->c1 * discrete_current_context->sample_rate);
	context->exponentD[1] = 1.0 - exp(context->exponentD[1]);

	if (info->r6 != 0) {
		context->exponentD[0] = -1.0 / (info->r6 * info->c1 * discrete_current_context->sample_rate);
		context->exponentD[0] = 1.0 - exp(context->exponentD[0]);
	}
	context->vCap2 = 0;
	context->vTrig2 = (info->v2 - 0.6 - MW_OP_AMP_NORTON_VBE) * (info->r9 / (info->r8 + info->r9));
	context->exponent2[0] = -1.0 / (info->r9 * info->c2 * discrete_current_context->sample_rate);
	context->exponent2[0] = 1.0 - exp(context->exponent2[0]);
	context->exponent2[1] = -1.0 / ((1.0 / (1.0 / info->r8 + 1.0 / info->r9)) * info->c2 * discrete_current_context->sample_rate);
	context->exponent2[1] = 1.0 - exp(context->exponent2[1]);
	context->vCap3 = 0;
	context->vTrig3 = (info->v3 - 0.6 - MW_OP_AMP_NORTON_VBE) * (info->r11 / (info->r10 + info->r11));
	context->exponent3[0] = -1.0 / (info->r11 * info->c3 * discrete_current_context->sample_rate);
	context->exponent3[0] = 1.0 - exp(context->exponent3[0]);
	context->exponent3[1] = -1.0 / ((1.0 / (1.0 / info->r10 + 1.0 / info->r11)) * info->c3 * discrete_current_context->sample_rate);
	context->exponent3[1] = 1.0 - exp(context->exponent3[1]);

	mw_dst_tvca_op_amp_step(node);
}

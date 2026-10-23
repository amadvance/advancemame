#ifndef MW_DISCRETE_H
#define MW_DISCRETE_H
#include "discrete.h"
typedef struct node_description node_description;
#define MW_DISCRETE_555_CC_TO_CAP 0x80
#define MW_DEFAULT_TTL_V_LOGIC_1        3.4

#define MW_DISC_LOGADJ                          1.0
#define MW_DISC_LINADJ                          0.0

#define MW_DISC_COMP_P_CAPACITOR                        0x00
#define MW_DISC_COMP_P_RESISTOR                 0x01

#define MW_DISC_CLK_MASK                        0x03
#define MW_DISC_CLK_ON_F_EDGE           0x00
#define MW_DISC_CLK_ON_R_EDGE           0x01
#define MW_DISC_CLK_BY_COUNT            0x02
#define MW_DISC_CLK_IS_FREQ             0x03

#define MW_DISC_COUNT_DOWN                      0
#define MW_DISC_COUNT_UP                        1

#define MW_DISC_COUNTER_IS_7492 0x08

#define MW_DISC_OUT_MASK                        0x30
#define MW_DISC_OUT_DEFAULT             0x00
#define MW_DISC_OUT_IS_ENERGY           0x10
#define MW_DISC_OUT_HAS_XTIME           0x20

#define MW_DISC_LFSR_XOR                                        0
#define MW_DISC_LFSR_OR                                 1
#define MW_DISC_LFSR_AND                                        2
#define MW_DISC_LFSR_XNOR                                       3
#define MW_DISC_LFSR_NOR                                        4
#define MW_DISC_LFSR_NAND                                       5
#define MW_DISC_LFSR_IN0                                        6
#define MW_DISC_LFSR_IN1                                        7
#define MW_DISC_LFSR_NOT_IN0                            8
#define MW_DISC_LFSR_NOT_IN1                            9
#define MW_DISC_LFSR_REPLACE                            10
#define MW_DISC_LFSR_XOR_INV_IN0           11
#define MW_DISC_LFSR_XOR_INV_IN1           12

#define MW_DISC_LFSR_FLAG_OUT_INVERT            0x01
#define MW_DISC_LFSR_FLAG_RESET_TYPE_L          0x00
#define MW_DISC_LFSR_FLAG_RESET_TYPE_H          0x02
#define MW_DISC_LFSR_FLAG_OUTPUT_F0             0x04

#define MW_DISC_SAMPHOLD_REDGE                          0
#define MW_DISC_SAMPHOLD_FEDGE                          1
#define MW_DISC_SAMPHOLD_HLATCH                 2
#define MW_DISC_SAMPHOLD_LLATCH                 3

#define MW_DISC_LADDER_MAXRES                           8

#define MW_DISC_FILTER_LOWPASS                          0
#define MW_DISC_FILTER_HIGHPASS                 1
#define MW_DISC_FILTER_BANDPASS                 2

#define MW_DISC_MIXER_IS_RESISTOR                       0
#define MW_DISC_MIXER_IS_OP_AMP                 1

#define MW_DISC_MIXER_IS_OP_AMP_WITH_RI         2
#define MW_DISC_MIXER_TYPE_MASK                 3
#define MW_DISC_MIXER_HAS_R_NODE                        4

enum {
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG0_INV,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG1_INV,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG2,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG2_INV,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG01_AND,
	MW_DISC_OP_AMP_TRIGGER_FUNCTION_TRG01_NAND
};

#define MW_DISC_OP_AMP_IS_NORTON        0x100
#define MW_OP_AMP_NORTON_VBE            0.5
#define MW_OP_AMP_VP_RAIL_OFFSET        1.5

#define MW_DISC_INTEGRATE_OP_AMP_1      0x00
#define MW_DISC_INTEGRATE_OP_AMP_2      0x10

#define MW_DISC_OP_AMP_1SHT_1   0x00

#define MW_DISC_OP_AMP_FILTER_IS_LOW_PASS_1     0x00
#define MW_DISC_OP_AMP_FILTER_IS_HIGH_PASS_1    0x10
#define MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1    0x20
#define MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_1M   0x30
#define MW_DISC_OP_AMP_FILTER_IS_HIGH_PASS_0    0x40
#define MW_DISC_OP_AMP_FILTER_IS_BAND_PASS_0    0x50

#define MW_DISC_OP_AMP_FILTER_TYPE_MASK         (0xf0 | MW_DISC_OP_AMP_IS_NORTON)



#define MW_DISC_SCHMITT_OSC_IN_IS_LOGIC 0x00
#define MW_DISC_SCHMITT_OSC_IN_IS_VOLTAGE       0x01

#define MW_DISC_SCHMITT_OSC_ENAB_IS_AND 0x00
#define MW_DISC_SCHMITT_OSC_ENAB_IS_NAND        0x02
#define MW_DISC_SCHMITT_OSC_ENAB_IS_OR          0x04
#define MW_DISC_SCHMITT_OSC_ENAB_IS_NOR 0x06

#define MW_DISC_SCHMITT_OSC_ENAB_MASK           0x06

#define MW_DISC_555_OUT_DC                                      0x00
#define MW_DISC_555_OUT_AC                                      0x10

#define MW_DISC_555_TRIGGER_IS_LOGIC            0x00
#define MW_DISC_555_TRIGGER_IS_VOLTAGE          0x40
#define MW_DISC_555_TRIGGER_DISCHARGES_CAP      0x80

#define MW_DISC_555_OUT_SQW                             0x00
#define MW_DISC_555_OUT_CAP                             0x01
#define MW_DISC_555_OUT_COUNT_F                 0x02
#define MW_DISC_555_OUT_COUNT_R                 0x03
#define MW_DISC_555_OUT_ENERGY                          0x04
#define MW_DISC_555_OUT_LOGIC_X                 0x05
#define MW_DISC_555_OUT_COUNT_F_X                       0x06
#define MW_DISC_555_OUT_COUNT_R_X                       0x07

#define MW_DISC_555_OUT_MASK                            0x07

#define MW_DISC_555_ASTABLE_HAS_FAST_CHARGE_DIODE               0x80

#define MW_DISC_566_OUT_DC                                      0x00
#define MW_DISC_566_OUT_AC                                      0x01

#define MW_DISC_566_OUT_SQUARE                          0x00
#define MW_DISC_566_OUT_TRIANGLE                        0x10
#define MW_DISC_566_OUT_LOGIC                           0x20

#define MW_DISC_566_OUT_MASK                            0x30

#define MW_DISC_ONESHOT_FEDGE                           0x00
#define MW_DISC_ONESHOT_REDGE                           0x01

#define MW_DISC_ONESHOT_NORETRIG                        0x00
#define MW_DISC_ONESHOT_RETRIG                          0x02

#define MW_DISC_OUT_ACTIVE_LOW                          0x04
#define MW_DISC_OUT_ACTIVE_HIGH                 0x00

#define MW_DISC_CD4066_THRESHOLD           2.75

#define MW_DISC_RC_INTEGRATE_TYPE1                                              0x00
#define MW_DISC_RC_INTEGRATE_TYPE2                                              0x01
#define MW_DISC_RC_INTEGRATE_TYPE3                                              0x02

struct mw__discrete_lfsr_desc {
	int clock_type;
	int bitlength;
	int reset_value;

	int feedback_bitsel0;
	int feedback_bitsel1;
	int feedback_function0;

	int feedback_function1;

	int feedback_function2;
	int feedback_function2_mask;

	int flags;

	int output_bit;
};
typedef struct mw__discrete_lfsr_desc mw_discrete_lfsr_desc;

struct mw__discrete_op_amp_osc_info {
	int type;
	double r1;
	double r2;
	double r3;
	double r4;
	double r5;
	double r6;
	double r7;
	double r8;
	double c;
	double vP;
};
typedef struct mw__discrete_op_amp_osc_info mw_discrete_op_amp_osc_info;

#define MW_DEFAULT_7414_VALUES  1.7, 0.9, 3.4

#define MW_DEFAULT_74LS14_VALUES        1.6, 0.8, 3.4

struct mw__discrete_schmitt_osc_desc {
	double rIn;
	double rFeedback;
	double c;
	double trshRise;
	double trshFall;
	double vGate;
	int options;
};
typedef struct mw__discrete_schmitt_osc_desc mw_discrete_schmitt_osc_desc;

struct mw__discrete_comp_adder_table {
	int type;
	double cDefault;
	int length;
	double c[MW_DISC_LADDER_MAXRES];
};
typedef struct mw__discrete_comp_adder_table mw_discrete_comp_adder_table;

struct mw__discrete_dac_r1_ladder {
	int ladderLength;
	double r[MW_DISC_LADDER_MAXRES];
	double vBias;
	double rBias;
	double rGnd;
	double cFilter;
};
typedef struct mw__discrete_dac_r1_ladder mw_discrete_dac_r1_ladder;

struct mw__discrete_integrate_info {
	int type;
	double r1;
	double r2;
	double r3;
	double c;
	double v1;
	double vP;
	double f0;
	double f1;
	double f2;
};
typedef struct mw__discrete_integrate_info mw_discrete_integrate_info;

#define MW_DISC_MAX_MIXER_INPUTS        8
struct mw__discrete_mixer_desc {
	int type;
	double r[MW_DISC_MAX_MIXER_INPUTS];
	int rNode[MW_DISC_MAX_MIXER_INPUTS];
	double c[MW_DISC_MAX_MIXER_INPUTS];
	double rI;
	double rF;
	double cF;
	double cAmp;
	double vRef;
	double gain;
};
typedef struct mw__discrete_mixer_desc mw_discrete_mixer_desc;

struct mw__discrete_op_amp_info {
	int type;
	double r1;
	double r2;
	double r3;
	double r4;
	double c;
	double vN;
	double vP;
};
typedef struct mw__discrete_op_amp_info mw_discrete_op_amp_info;

struct mw__discrete_op_amp_1sht_info {
	int type;
	double r1;
	double r2;
	double r3;
	double r4;
	double r5;
	double c1;
	double c2;
	double vN;
	double vP;
};
typedef struct mw__discrete_op_amp_1sht_info mw_discrete_op_amp_1sht_info;

struct mw__discrete_op_amp_tvca_info {
	double r1;
	double r2;
	double r3;
	double r4;
	double r5;
	double r6;
	double r7;
	double r8;
	double r9;
	double r10;
	double r11;
	double c1;
	double c2;
	double c3;
	double v1;
	double v2;
	double v3;
	double vP;
	int f0;
	int f1;
	int f2;
	int f3;
	int f4;
	int f5;
};
typedef struct mw__discrete_op_amp_tvca_info mw_discrete_op_amp_tvca_info;

struct mw__discrete_op_amp_filt_info {
	double r1;
	double r2;
	double r3;
	double r4;
	double rF;
	double c1;
	double c2;
	double c3;
	double vRef;
	double vP;
	double vN;
};
typedef struct mw__discrete_op_amp_filt_info mw_discrete_op_amp_filt_info;

#define MW_DEFAULT_555_HIGH             -1
#define MW_DEFAULT_555_THRESHOLD        -1
#define MW_DEFAULT_555_TRIGGER          -1
#define MW_DEFAULT_555_VALUES           MW_DEFAULT_555_HIGH, MW_DEFAULT_555_THRESHOLD, MW_DEFAULT_555_TRIGGER

struct mw__discrete_555_desc {
	int options;
	double v555;
	double v555high;
	double threshold555;
	double trigger555;
};
typedef struct mw__discrete_555_desc mw_discrete_555_desc;

struct mw__discrete_555_cc_desc {
	int options;
	double v555;
	double v555high;
	double threshold555;
	double trigger555;
	double vCCsource;
	double vCCjunction;
};
typedef struct mw__discrete_555_cc_desc mw_discrete_555_cc_desc;

struct mw__discrete_555_vco1_desc {
	int options;
	double r1, r2, r3, r4, c;
	double v555;
	double v555high;
	double threshold555;
	double trigger555;
};
typedef struct mw__discrete_555_vco1_desc mw_discrete_555_vco1_desc;

struct mw__discrete_566_desc {
	int options;
	double vPlus;
	double vNeg;
};
typedef struct mw__discrete_566_desc mw_discrete_566_desc;

struct _discrete_adsr {
	double attack_time;
	double attack_value;
	double decay_time;
	double decay_value;
	double sustain_time;
	double sustain_value;
	double release_time;
	double release_value;
};
typedef struct _discrete_adsr discrete_adsr;

struct mw__discrete_custom_info {
	void (*reset)(node_description *node);
	void (*step)(node_description *node);
	size_t contextsize;
	const void *custom;
};
typedef struct mw__discrete_custom_info mw_discrete_custom_info;

#define MW_DEFAULT_CD40XX_VALUES(_vB)   (_vB), (_vB) * 0.02, (_vB) * 0.98, (_vB) / 5.0 * 1.5, (_vB) / 5.0 * 3.5, 0.1

#define MW_DISC_OSC_INVERTER_IS_TYPE1                   0x00
#define MW_DISC_OSC_INVERTER_IS_TYPE2                   0x01
#define MW_DISC_OSC_INVERTER_IS_TYPE3                   0x02
#define MW_DISC_OSC_INVERTER_IS_TYPE4                   0x03
#define MW_DISC_OSC_INVERTER_IS_TYPE5                   0x04
#define MW_DISC_OSC_INVERTER_TYPE_MASK                  0x0F

#define MW_DISC_OSC_INVERTER_OUT_IS_LOGIC               0x10

struct mw__discrete_inverter_osc_desc {
	double vB;
	double vOutLow;
	double vOutHigh;
	double vInFall;
	double vInRise;
	double clamp;
	int options;
};
typedef struct mw__discrete_inverter_osc_desc mw_discrete_inverter_osc_desc;


enum {
	mw_DSS_COUNTER = 256,
	mw_DSS_COUNTER_FIX,
	mw_DSS_LFSR_NOISE,
	mw_DSS_NOTE,
	mw_DSS_NOISE,
	mw_DSS_SAWTOOTHWAVE,
	mw_DSS_SINEWAVE,
	mw_DSS_SQUAREWAVE,
	mw_DSS_SQUAREWFIX,
	mw_DSS_SQUAREWAVE2,
	mw_DSS_INVERTER_OSC,
	mw_DSS_TRIANGLEWAVE,
	mw_DSS_OP_AMP_OSC,
	mw_DSS_SCHMITT_OSC,
	mw_DSS_ADSR,
	mw_DST_ADDER,
	mw_DST_CLAMP,
	mw_DST_DIVIDE,
	mw_DST_GAIN,
	mw_DST_LOGIC_INV,
	mw_DST_LOGIC_AND,
	mw_DST_LOGIC_NAND,
	mw_DST_LOGIC_OR,
	mw_DST_LOGIC_NOR,
	mw_DST_LOGIC_XOR,
	mw_DST_LOGIC_NXOR,
	mw_DST_LOGIC_DFF,
	mw_DST_LOGIC_JKFF,
	mw_DST_LOOKUP_TABLE,
	mw_DST_MULTIPLEX,
	mw_DST_ONESHOT,
	mw_DST_RAMP,
	mw_DST_SAMPHOLD,
	mw_DST_SWITCH,
	mw_DST_ASWITCH,
	mw_DST_TRANSFORM,
	mw_DST_COMP_ADDER,
	mw_DST_DAC_R1,
	mw_DST_DIODE_MIX,
	mw_DST_INTEGRATE,
	mw_DST_MIXER,
	mw_DST_OP_AMP,
	mw_DST_OP_AMP_1SHT,
	mw_DST_TVCA_OP_AMP,
	mw_DST_VCA,
	mw_DST_FILTER1,
	mw_DST_FILTER2,
	mw_DST_CRFILTER,
	mw_DST_OP_AMP_FILT,
	mw_DST_RCDISC,
	mw_DST_RCDISC2,
	mw_DST_RCDISC3,
	mw_DST_RCDISC4,
	mw_DST_RCDISC5,
	mw_DST_RCINTEGRATE,
	mw_DST_RCDISC_MOD,
	mw_DST_RCFILTER,
	mw_DST_RCFILTERN,
	mw_DST_RCDISCN,
	mw_DST_RCDISC2N,
	mw_DSD_555_ASTBL,
	mw_DSD_555_MSTBL,
	mw_DSD_555_CC,
	mw_DSD_555_VCO1,
	mw_DSD_566,
	mw_DST_CUSTOM,
	MW_DISCRETE_TYPE_END
};

#define MW_DISCRETE_SOUND_START(name) const struct discrete_sound_block name##_discrete_interface[] = {
#define MW_DISCRETE_SOUND_END                                              { NODE_00, DSS_NULL     , 0, { NODE_NC }, { 0 } ,NULL  ,"End Marker" }  };

#define MW_DISCRETE_ADJUSTMENT(NODE,ENAB,MIN,MAX,LOGLIN,PORT)              { NODE, DSS_ADJUSTMENT  , 7, { ENAB,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,MIN,MAX,LOGLIN,PORT,0   ,100  }, NULL  , "MW_DISCRETE_ADJUSTMENT"  },
#define MW_DISCRETE_ADJUSTMENT_TAG(NODE,ENAB,MIN,MAX,LOGLIN,TAG)           { NODE, DSS_ADJUSTMENT  , 7, { ENAB,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,MIN,MAX,LOGLIN,0   ,0   ,100  }, TAG   , "MW_DISCRETE_ADJUSTMENT_TAG" },
#define MW_DISCRETE_ADJUSTMENTX(NODE,ENAB,MIN,MAX,LOGLIN,PORT,PMIN,PMAX)   { NODE, DSS_ADJUSTMENT  , 7, { ENAB,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,MIN,MAX,LOGLIN,PORT,PMIN,PMAX }, NULL  , "MW_DISCRETE_ADJUSTMENTX"  },
#define MW_DISCRETE_CONSTANT(NODE,CONST)                                   { NODE, DSS_CONSTANT    , 1, { NODE_NC }, { CONST } ,NULL  ,"Constant" },
#define MW_DISCRETE_INPUT_DATA(NODE)                                       { NODE, DSS_INPUT_DATA  , 3, { NODE_NC,NODE_NC,NODE_NC }, { 1,0,0 }, NULL, "Input Data" },
#define MW_DISCRETE_INPUTX_DATA(NODE,GAIN,OFFSET,INIT)                     { NODE, DSS_INPUT_DATA  , 3, { NODE_NC,NODE_NC,NODE_NC }, { GAIN,OFFSET,INIT }, NULL, "InputX Data" },
#define MW_DISCRETE_INPUT_LOGIC(NODE)                                      { NODE, DSS_INPUT_LOGIC , 3, { NODE_NC,NODE_NC,NODE_NC }, { 1,0,0 }, NULL, "Input Logic" },
#define MW_DISCRETE_INPUTX_LOGIC(NODE,GAIN,OFFSET,INIT)                    { NODE, DSS_INPUT_LOGIC , 3, { NODE_NC,NODE_NC,NODE_NC }, { GAIN,OFFSET,INIT }, NULL, "InputX Logic" },
#define MW_DISCRETE_INPUT_NOT(NODE)                                        { NODE, DSS_INPUT_NOT   , 3, { NODE_NC,NODE_NC,NODE_NC }, { 1,0,0 }, NULL, "Input Not" },
#define MW_DISCRETE_INPUTX_NOT(NODE,GAIN,OFFSET,INIT)                      { NODE, DSS_INPUT_NOT   , 3, { NODE_NC,NODE_NC,NODE_NC }, { GAIN,OFFSET,INIT }, NULL, "InputX Not" },
#define MW_DISCRETE_INPUT_PULSE(NODE,INIT)                                 { NODE, DSS_INPUT_PULSE , 3, { NODE_NC,NODE_NC,NODE_NC }, { 1,0,INIT }, NULL, "Input Pulse" },
#define MW_DISCRETE_INPUT_STREAM(NODE)                                     { NODE, DSS_INPUT_STREAM, 2, { NODE_NC,NODE_NC }, { 1,0 }, NULL, "Input Stream" },
#define MW_DISCRETE_INPUTX_STREAM(NODE,GAIN,OFFSET)                        { NODE, DSS_INPUT_STREAM, 2, { NODE_NC,NODE_NC }, { GAIN,OFFSET }, NULL, "InputX Stream" },

#define MW_DISCRETE_COUNTER(NODE,ENAB,RESET,CLK,MAX,DIR,INIT0,CLKTYPE)     { NODE, mw_DSS_COUNTER     , 7, { ENAB,RESET,CLK,NODE_NC,DIR,INIT0,NODE_NC }, { ENAB,RESET,CLK,MAX,DIR,INIT0,CLKTYPE }, NULL, "MW_DISCRETE_COUNTER" },
#define MW_DISCRETE_COUNTER_7492(NODE,ENAB,RESET,CLK)                      { NODE, mw_DSS_COUNTER     , 7, { ENAB,RESET,CLK,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,RESET,CLK,5,1,0,MW_DISC_COUNTER_IS_7492 }, NULL, "MW_DISCRETE_COUNTER_7492" },
#define MW_DISCRETE_LFSR_NOISE(NODE,ENAB,RESET,CLK,AMPL,FEED,BIAS,LFSRTB)  { NODE, mw_DSS_LFSR_NOISE  , 6, { ENAB,RESET,CLK,AMPL,FEED,BIAS }, { ENAB,RESET,CLK,AMPL,FEED,BIAS }, LFSRTB, "LFSR Noise Source" },
#define MW_DISCRETE_NOISE(NODE,ENAB,FREQ,AMPL,BIAS)                        { NODE, mw_DSS_NOISE       , 4, { ENAB,FREQ,AMPL,BIAS }, { ENAB,FREQ,AMPL,BIAS }, NULL, "Noise Source" },
#define MW_DISCRETE_NOTE(NODE,ENAB,CLK,DATA,MAX1,MAX2,CLKTYPE)             { NODE, mw_DSS_NOTE        , 6, { ENAB,CLK,DATA,NODE_NC,NODE_NC,NODE_NC }, { ENAB,CLK,DATA,MAX1,MAX2,CLKTYPE }, NULL, "Note Generator" },
#define MW_DISCRETE_SAWTOOTHWAVE(NODE,ENAB,FREQ,AMPL,BIAS,GRAD,PHASE)      { NODE, mw_DSS_SAWTOOTHWAVE, 6, { ENAB,FREQ,AMPL,BIAS,NODE_NC,NODE_NC }, { ENAB,FREQ,AMPL,BIAS,GRAD,PHASE }, NULL, "Saw Tooth Wave" },
#define MW_DISCRETE_SINEWAVE(NODE,ENAB,FREQ,AMPL,BIAS,PHASE)               { NODE, mw_DSS_SINEWAVE    , 5, { ENAB,FREQ,AMPL,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,BIAS,PHASE }, NULL, "Sine Wave" },
#define MW_DISCRETE_SQUAREWAVE(NODE,ENAB,FREQ,AMPL,DUTY,BIAS,PHASE)        { NODE, mw_DSS_SQUAREWAVE  , 6, { ENAB,FREQ,AMPL,DUTY,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,DUTY,BIAS,PHASE }, NULL, "Square Wave" },
#define MW_DISCRETE_SQUAREWFIX(NODE,ENAB,FREQ,AMPL,DUTY,BIAS,PHASE)        { NODE, mw_DSS_SQUAREWFIX  , 6, { ENAB,FREQ,AMPL,DUTY,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,DUTY,BIAS,PHASE }, NULL, "Square Wave Fixed" },
#define MW_DISCRETE_SQUAREWAVE2(NODE,ENAB,AMPL,T_OFF,T_ON,BIAS,TSHIFT)     { NODE, mw_DSS_SQUAREWAVE2 , 6, { ENAB,AMPL,T_OFF,T_ON,BIAS,NODE_NC }, { ENAB,AMPL,T_OFF,T_ON,BIAS,TSHIFT }, NULL, "Square Wave 2" },
#define MW_DISCRETE_TRIANGLEWAVE(NODE,ENAB,FREQ,AMPL,BIAS,PHASE)           { NODE, mw_DSS_TRIANGLEWAVE, 5, { ENAB,FREQ,AMPL,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,BIAS,PHASE }, NULL, "Triangle Wave" },

#define MW_DISCRETE_INVERTER_OSC(NODE,ENAB,MOD,RCHARGE,RP,C,R2,INFO)       { NODE, mw_DSS_INVERTER_OSC, 6, { ENAB,MOD,NODE_NC,NODE_NC,NODE_NC }, { ENAB,MOD,RCHARGE,RP,C,R2 }, INFO, "Inverter Oscillator" },
#define MW_DISCRETE_OP_AMP_OSCILLATOR(NODE,ENAB,INFO)                      { NODE, mw_DSS_OP_AMP_OSC  , 1, { ENAB }, { ENAB }, INFO, "Op Amp Oscillator" },
#define MW_DISCRETE_OP_AMP_VCO1(NODE,ENAB,VMOD1,INFO)                      { NODE, mw_DSS_OP_AMP_OSC  , 2, { ENAB,VMOD1 }, { ENAB,VMOD1 }, INFO, "Op Amp VCO 1-vMod" },
#define MW_DISCRETE_OP_AMP_VCO2(NODE,ENAB,VMOD1,VMOD2,INFO)                { NODE, mw_DSS_OP_AMP_OSC  , 3, { ENAB,VMOD1,VMOD2 }, { ENAB,VMOD1,VMOD2 }, INFO, "Op Amp VCO 2-vMod" },
#define MW_DISCRETE_SCHMITT_OSCILLATOR(NODE,ENAB,INP0,AMPL,TABLE)          { NODE, mw_DSS_SCHMITT_OSC , 3, { ENAB,INP0,AMPL }, { ENAB,INP0,AMPL }, TABLE, "Schmitt Feedback Oscillator" },

#define MW_DISCRETE_ADSR_ENV(NODE,ENAB,TRIGGER,GAIN,ADSRTB)                { NODE, mw_DSS_ADSR        , 3, { ENAB,TRIGGER,GAIN }, { ENAB,TRIGGER,GAIN }, ADSRTB, "ADSR Env Generator" },

#define MW_DISCRETE_ADDER2(NODE,ENAB,INP0,INP1)                            { NODE, mw_DST_ADDER       , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Adder 2 Node" },
#define MW_DISCRETE_ADDER3(NODE,ENAB,INP0,INP1,INP2)                       { NODE, mw_DST_ADDER       , 4, { ENAB,INP0,INP1,INP2 }, { ENAB,INP0,INP1,INP2 }, NULL, "Adder 3 Node" },
#define MW_DISCRETE_ADDER4(NODE,ENAB,INP0,INP1,INP2,INP3)                  { NODE, mw_DST_ADDER       , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Adder 4 Node" },
#define MW_DISCRETE_CLAMP(NODE,ENAB,INP0,MIN,MAX,CLAMP)                    { NODE, mw_DST_CLAMP       , 5, { ENAB,INP0,MIN,MAX,CLAMP }, { ENAB,INP0,MIN,MAX,CLAMP }, NULL, "Signal Clamp" },
#define MW_DISCRETE_DIVIDE(NODE,ENAB,INP0,INP1)                            { NODE, mw_DST_DIVIDE      , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Divider" },
#define MW_DISCRETE_GAIN(NODE,INP0,GAIN)                                   { NODE, mw_DST_GAIN        , 4, { NODE_NC,INP0,NODE_NC,NODE_NC }, { 1,INP0,GAIN,0 }, NULL, "Gain" },
#define MW_DISCRETE_INVERT(NODE,INP0)                                      { NODE, mw_DST_GAIN        , 4, { NODE_NC,INP0,NODE_NC,NODE_NC }, { 1,INP0,-1,0 }, NULL, "Inverter" },
#define MW_DISCRETE_LOGIC_INVERT(NODE,ENAB,INP0)                           { NODE, mw_DST_LOGIC_INV   , 2, { ENAB,INP0 }, { ENAB,INP0 }, NULL, "Logic Invertor" },
#define MW_DISCRETE_LOGIC_AND(NODE,ENAB,INP0,INP1)                         { NODE, mw_DST_LOGIC_AND   , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,1.0,1.0 }, NULL, "Logic AND (2inp)" },
#define MW_DISCRETE_LOGIC_AND3(NODE,ENAB,INP0,INP1,INP2)                   { NODE, mw_DST_LOGIC_AND   , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,1.0 }, NULL, "Logic AND (3inp)" },
#define MW_DISCRETE_LOGIC_AND4(NODE,ENAB,INP0,INP1,INP2,INP3)              { NODE, mw_DST_LOGIC_AND   , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 } ,NULL, "Logic AND (4inp)" },
#define MW_DISCRETE_LOGIC_NAND(NODE,ENAB,INP0,INP1)                        { NODE, mw_DST_LOGIC_NAND  , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,1.0,1.0 }, NULL, "Logic NAND (2inp)" },
#define MW_DISCRETE_LOGIC_NAND3(NODE,ENAB,INP0,INP1,INP2)                  { NODE, mw_DST_LOGIC_NAND  , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,1.0 }, NULL, "Logic NAND (3inp)" },
#define MW_DISCRETE_LOGIC_NAND4(NODE,ENAB,INP0,INP1,INP2,INP3)             { NODE, mw_DST_LOGIC_NAND  , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Logic NAND (4inp)" },
#define MW_DISCRETE_LOGIC_OR(NODE,ENAB,INP0,INP1)                          { NODE, mw_DST_LOGIC_OR    , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,0.0,0.0 }, NULL, "Logic OR (2inp)" },
#define MW_DISCRETE_LOGIC_OR3(NODE,ENAB,INP0,INP1,INP2)                    { NODE, mw_DST_LOGIC_OR    , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,0.0 }, NULL, "Logic OR (3inp)" },
#define MW_DISCRETE_LOGIC_OR4(NODE,ENAB,INP0,INP1,INP2,INP3)               { NODE, mw_DST_LOGIC_OR    , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Logic OR (4inp)" },
#define MW_DISCRETE_LOGIC_NOR(NODE,ENAB,INP0,INP1)                         { NODE, mw_DST_LOGIC_NOR   , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,0.0,0.0 }, NULL, "Logic NOR (2inp)" },
#define MW_DISCRETE_LOGIC_NOR3(NODE,ENAB,INP0,INP1,INP2)                   { NODE, mw_DST_LOGIC_NOR   , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,0.0 }, NULL, "Logic NOR (3inp)" },
#define MW_DISCRETE_LOGIC_NOR4(NODE,ENAB,INP0,INP1,INP2,INP3)              { NODE, mw_DST_LOGIC_NOR   , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Logic NOR (4inp)" },
#define MW_DISCRETE_LOGIC_XOR(NODE,ENAB,INP0,INP1)                         { NODE, mw_DST_LOGIC_XOR   , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Logic XOR (2inp)" },
#define MW_DISCRETE_LOGIC_NXOR(NODE,ENAB,INP0,INP1)                        { NODE, mw_DST_LOGIC_NXOR  , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Logic NXOR (2inp)" },
#define MW_DISCRETE_LOGIC_DFLIPFLOP(NODE,ENAB,RESET,SET,CLK,INP)           { NODE, mw_DST_LOGIC_DFF   , 5, { ENAB,RESET,SET,CLK,INP }, { ENAB,RESET,SET,CLK,INP }, NULL, "Logic DFlipFlop" },
#define MW_DISCRETE_LOGIC_JKFLIPFLOP(NODE,ENAB,RESET,SET,CLK,J,K)          { NODE, mw_DST_LOGIC_JKFF  , 6, { ENAB,RESET,SET,CLK,J,K }, { ENAB,RESET,SET,CLK,J,K }, NULL, "Logic JKFlipFlop" },
#define MW_DISCRETE_LOOKUP_TABLE(NODE,ENAB,ADDR,SIZE,TABLE)                { NODE, mw_DST_LOOKUP_TABLE, 3, { ENAB,ADDR,NODE_NC }, { ENAB,ADDR,SIZE }, TABLE, "Lookup Table" },
#define MW_DISCRETE_MULTIPLEX2(NODE,ENAB,ADDR,INP0,INP1)                   { NODE, mw_DST_MULTIPLEX   , 4, { ENAB,ADDR,INP0,INP1 }, { ENAB,ADDR,INP0,INP1 }, NULL, "1 of 2 Multiplexer" },
#define MW_DISCRETE_MULTIPLEX4(NODE,ENAB,ADDR,INP0,INP1,INP2,INP3)         { NODE, mw_DST_MULTIPLEX   , 6, { ENAB,ADDR,INP0,INP1,INP2,INP3 }, { ENAB,ADDR,INP0,INP1,INP2,INP3 }, NULL, "1 of 4 Multiplexer" },
#define MW_DISCRETE_MULTIPLEX8(NODE,ENAB,ADDR,INP0,INP1,INP2,INP3,INP4,INP5,INP6,INP7) { NODE, mw_DST_MULTIPLEX, 10, { ENAB,ADDR,INP0,INP1,INP2,INP3,INP4,INP5,INP6,INP7 }, { ENAB,ADDR,INP0,INP1,INP2,INP3,INP4,INP5,INP6,INP7 }, NULL, "1 of 8 Multiplexer" },
#define MW_DISCRETE_MULTIPLY(NODE,ENAB,INP0,INP1)                          { NODE, mw_DST_GAIN        , 4, { ENAB,INP0,INP1,NODE_NC }, { ENAB,INP0,INP1,0 }, NULL, "Multiplier" },
#define MW_DISCRETE_MULTADD(NODE,ENAB,INP0,INP1,INP2)                      { NODE, mw_DST_GAIN        , 4, { ENAB,INP0,INP1,INP2 }, { ENAB,INP0,INP1,INP2 }, NULL, "Multiply/Add" },
#define MW_DISCRETE_ONESHOT(NODE,TRIG,AMPL,WIDTH,TYPE)                     { NODE, mw_DST_ONESHOT     , 5, { NODE_NC,TRIG,AMPL,WIDTH,NODE_NC }, { 0,TRIG,AMPL,WIDTH,TYPE }, NULL, "One Shot" },
#define MW_DISCRETE_ONESHOTR(NODE,RESET,TRIG,AMPL,WIDTH,TYPE)              { NODE, mw_DST_ONESHOT     , 5, { RESET,TRIG,AMPL,WIDTH,NODE_NC }, { RESET,TRIG,AMPL,WIDTH,TYPE }, NULL, "One Shot Resetable" },
#define MW_DISCRETE_ONOFF(NODE,ENAB,INP0)                                  { NODE, mw_DST_GAIN        , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,0,1,0 }, NULL, "OnOff Switch" },
#define MW_DISCRETE_RAMP(NODE,ENAB,RAMP,GRAD,START,END,CLAMP)              { NODE, mw_DST_RAMP        , 6, { ENAB,RAMP,GRAD,START,END,CLAMP }, { ENAB,RAMP,GRAD,START,END,CLAMP }, NULL, "Ramp Up/Down" },
#define MW_DISCRETE_SAMPLHOLD(NODE,ENAB,INP0,CLOCK,CLKTYPE)                { NODE, mw_DST_SAMPHOLD    , 4, { ENAB,INP0,CLOCK,NODE_NC }, { ENAB,INP0,CLOCK,CLKTYPE }, NULL, "Sample & Hold" },
#define MW_DISCRETE_SWITCH(NODE,ENAB,SWITCH,INP0,INP1)                     { NODE, mw_DST_SWITCH      , 4, { ENAB,SWITCH,INP0,INP1 }, { ENAB,SWITCH,INP0,INP1 }, NULL, "2 Pole Switch" },
#define MW_DISCRETE_ASWITCH(NODE,ENAB,CTRL,INP,THRESHOLD)                  { NODE, mw_DST_ASWITCH     , 3, { ENAB,CTRL,INP,THRESHOLD }, { ENAB,CTRL,INP, THRESHOLD}, NULL, "Analog Switch" },
#define MW_DISCRETE_TRANSFORM2(NODE,ENAB,INP0,INP1,FUNCT)                  { NODE, mw_DST_TRANSFORM   , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, FUNCT, "Transform 2 Nodes" },
#define MW_DISCRETE_TRANSFORM3(NODE,ENAB,INP0,INP1,INP2,FUNCT)             { NODE, mw_DST_TRANSFORM   , 4, { ENAB,INP0,INP1,INP2 }, { ENAB,INP0,INP1,INP2 }, FUNCT, "Transform 3 Nodes" },
#define MW_DISCRETE_TRANSFORM4(NODE,ENAB,INP0,INP1,INP2,INP3,FUNCT)        { NODE, mw_DST_TRANSFORM   , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, FUNCT, "Transform 4 Nodes" },
#define MW_DISCRETE_TRANSFORM5(NODE,ENAB,INP0,INP1,INP2,INP3,INP4,FUNCT)   { NODE, mw_DST_TRANSFORM   , 6, { ENAB,INP0,INP1,INP2,INP3,INP4 }, { ENAB,INP0,INP1,INP2,INP3,INP4 }, FUNCT, "Transform 5 Nodes" },

#define MW_DISCRETE_COMP_ADDER(NODE,ENAB,DATA,TABLE)                       { NODE, mw_DST_COMP_ADDER  , 2, { ENAB,DATA }, { ENAB,DATA }, TABLE, "Selectable R or C component Adder" },
#define MW_DISCRETE_DAC_R1(NODE,ENAB,DATA,VDATA,LADDER)                    { NODE, mw_DST_DAC_R1      , 3, { ENAB,DATA,VDATA }, { ENAB,DATA,VDATA }, LADDER, "DAC with R1 Ladder" },
#define MW_DISCRETE_DIODE_MIXER2(NODE,ENAB,VJUNC,IN0,IN1)                  { NODE, mw_DST_DIODE_MIX   , 4, { ENAB,NODE_NC,IN0,IN1 }, { ENAB,VJUNC,IN0,IN1 }, NULL, "Diode Mixer 2 Stage" },
#define MW_DISCRETE_DIODE_MIXER3(NODE,ENAB,VJUNC,IN0,IN1,IN2)              { NODE, mw_DST_DIODE_MIX   , 5, { ENAB,NODE_NC,IN0,IN1,IN2 }, { ENAB,VJUNC,IN0,IN1,IN2 }, INFO, "Diode Mixer 3 Stage" },
#define MW_DISCRETE_DIODE_MIXER4(NODE,ENAB,VJUNC,IN0,IN1,IN2,IN3)          { NODE, mw_DST_DIODE_MIX   , 6, { ENAB,NODE_NC,IN0,IN1,IN2,IN3 }, { ENAB,VJUNC,IN0,IN1,IN2,IN3 }, INFO, "Diode Mixer 4 Stage" },
#define MW_DISCRETE_INTEGRATE(NODE,TRG0,TRG1,INFO)                         { NODE, mw_DST_INTEGRATE   , 2, { TRG0,TRG1 }, { TRG0,TRG1 }, INFO, "Various Integraton Circuit" },
#define MW_DISCRETE_MIXER2(NODE,ENAB,IN0,IN1,INFO)                         { NODE, mw_DST_MIXER       , 3, { ENAB,IN0,IN1 }, { ENAB,IN0,IN1 }, INFO, "Final Mixer 2 Stage" },
#define MW_DISCRETE_MIXER3(NODE,ENAB,IN0,IN1,IN2,INFO)                     { NODE, mw_DST_MIXER       , 4, { ENAB,IN0,IN1,IN2 }, { ENAB,IN0,IN1,IN2 }, INFO, "Final Mixer 3 Stage" },
#define MW_DISCRETE_MIXER4(NODE,ENAB,IN0,IN1,IN2,IN3,INFO)                 { NODE, mw_DST_MIXER       , 5, { ENAB,IN0,IN1,IN2,IN3 }, { ENAB,IN0,IN1,IN2,IN3 }, INFO, "Final Mixer 4 Stage" },
#define MW_DISCRETE_MIXER5(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,INFO)             { NODE, mw_DST_MIXER       , 6, { ENAB,IN0,IN1,IN2,IN3,IN4 }, { ENAB,IN0,IN1,IN2,IN3,IN4 }, INFO, "Final Mixer 5 Stage" },
#define MW_DISCRETE_MIXER6(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,INFO)         { NODE, mw_DST_MIXER       , 7, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5 }, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5 }, INFO, "Final Mixer 6 Stage" },
#define MW_DISCRETE_MIXER7(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,INFO)     { NODE, mw_DST_MIXER       , 8, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6 }, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6 }, INFO, "Final Mixer 7 Stage" },
#define MW_DISCRETE_MIXER8(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7,INFO) { NODE, mw_DST_MIXER       , 9, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7 }, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7 }, INFO, "Final Mixer 8 Stage" },
#define MW_DISCRETE_OP_AMP(NODE,ENAB,IN0,IN1,INFO)                         { NODE, mw_DST_OP_AMP      , 3, { ENAB,IN0,IN1 }, { ENAB,IN0,IN1 }, INFO, "Op Amp Circuit" },
#define MW_DISCRETE_OP_AMP_ONESHOT(NODE,TRIG,INFO)                         { NODE, mw_DST_OP_AMP_1SHT , 1, { TRIG }, { TRIG }, INFO, "Op Amp One Shot" },
#define MW_DISCRETE_OP_AMP_TRIG_VCA(NODE,TRG0,TRG1,TRG2,IN0,IN1,INFO)      { NODE, mw_DST_TVCA_OP_AMP , 5, { TRG0,TRG1,TRG2,IN0,IN1 }, { TRG0,TRG1,TRG2,IN0,IN1 }, INFO, "Triggered VCA Op Amp Circuit" },
#define MW_DISCRETE_VCA(NODE,ENAB,IN0,CTRL,TYPE)                           { NODE, mw_DST_VCA         , 4, { ENAB,IN0,CTRL,NODE_NC }, { ENAB,IN0,CTRL,TYPE }, NULL, "VCA IC" },

#define MW_DISCRETE_FILTER1(NODE,ENAB,INP0,FREQ,TYPE)                      { NODE, mw_DST_FILTER1     , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,FREQ,TYPE }, NULL, "1st Order Filter" },
#define MW_DISCRETE_FILTER2(NODE,ENAB,INP0,FREQ,DAMP,TYPE)                 { NODE, mw_DST_FILTER2     , 5, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,FREQ,DAMP,TYPE }, NULL, "2nd Order Filter" },

#define MW_DISCRETE_CRFILTER(NODE,ENAB,INP0,RVAL,CVAL)                     { NODE, mw_DST_CRFILTER    , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "CR Filter" },
#define MW_DISCRETE_CRFILTER_VREF(NODE,ENAB,INP0,RVAL,CVAL,VREF)           { NODE, mw_DST_CRFILTER    , 5, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL,VREF }, NULL, "CR Filter to VREF" },
#define MW_DISCRETE_OP_AMP_FILTER(NODE,ENAB,INP0,INP1,TYPE,INFO)           { NODE, mw_DST_OP_AMP_FILT , 4, { ENAB,INP0,INP1,NODE_NC }, { ENAB,INP0,INP1,TYPE }, INFO, "Op Amp Filter" },
#define MW_DISCRETE_RCDISC(NODE,ENAB,INP0,RVAL,CVAL)                       { NODE, mw_DST_RCDISC      , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Discharge" },
#define MW_DISCRETE_RCDISC2(NODE,SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL)        { NODE, mw_DST_RCDISC2     , 6, { SWITCH,INP0,NODE_NC,INP1,NODE_NC,NODE_NC }, { SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL }, NULL, "RC Discharge 2" },
#define MW_DISCRETE_RCDISC3(NODE,ENAB,INP0,RVAL0,RVAL1,CVAL)               { NODE, mw_DST_RCDISC3     , 5, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL0,RVAL1,CVAL }, NULL, "RC Discharge 3" },
#define MW_DISCRETE_RCDISC4(NODE,ENAB,INP0,RVAL0,RVAL1,RVAL2,CVAL,VP,TYPE) { NODE, mw_DST_RCDISC4     , 8, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL0,RVAL1,RVAL2,CVAL,VP,TYPE }, NULL, "RC Discharge 4" },
#define MW_DISCRETE_RCDISC5(NODE,ENAB,INP0,RVAL,CVAL)                      { NODE, mw_DST_RCDISC5     , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Discharge 5" },
#define MW_DISCRETE_RCINTEGRATE(NODE,ENAB,INP0,RVAL0,RVAL1,RVAL2,CVAL,vP,TYPE)		{ NODE, mw_DST_RCINTEGRATE     , 8, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL0,RVAL1,RVAL2,CVAL,vP,TYPE }, NULL, "RC Discharge 6" },
#define MW_DISCRETE_RCDISC_MODULATED(NODE,ENAB,INP0,INP1,RVAL0,RVAL1,RVAL2,RVAL3,CVAL,VP)			{ NODE, mw_DST_RCDISC_MOD     , 9, { ENAB,INP0,INP1,RVAL0,RVAL1,RVAL2,RVAL3,CVAL,VP }, { ENAB,INP0,INP1,RVAL0,RVAL1,RVAL2,RVAL3,CVAL,VP }, NULL, "Modulated RC Discharge" },
#define MW_DISCRETE_RCFILTER(NODE,ENAB,INP0,RVAL,CVAL)                     { NODE, mw_DST_RCFILTER    , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Filter" },
#define MW_DISCRETE_RCFILTER_VREF(NODE,ENAB,INP0,RVAL,CVAL,VREF)           { NODE, mw_DST_RCFILTER    , 5, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL,VREF }, NULL, "RC Filter to VREF" },

#define MW_DISCRETE_RCDISCN(NODE,ENAB,INP0,RVAL,CVAL)                      { NODE, mw_DST_RCDISCN     , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Discharge (New Type)" },
#define MW_DISCRETE_RCDISC2N(NODE,SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL)       { NODE, mw_DST_RCDISC2N    , 6, { SWITCH,INP0,NODE_NC,INP1,NODE_NC,NODE_NC }, { SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL }, NULL, "RC Discharge 2 (New Type)" },
#define MW_DISCRETE_RCFILTERN(NODE,ENAB,INP0,RVAL,CVAL)                    { NODE, mw_DST_RCFILTERN   , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Filter (New Type)" },

#define MW_DISCRETE_CUSTOM1(NODE,ENAB,IN0,INFO)                            { NODE, mw_DST_CUSTOM      , 2, { ENAB,IN0 }, { ENAB,IN0 }, INFO, "1 input custom module" },
#define MW_DISCRETE_CUSTOM2(NODE,ENAB,IN0,IN1,INFO)                        { NODE, mw_DST_CUSTOM      , 3, { ENAB,IN0,IN1 }, { ENAB,IN0,IN1 }, INFO, "2 input custom module" },
#define MW_DISCRETE_CUSTOM3(NODE,ENAB,IN0,IN1,IN2,INFO)                    { NODE, mw_DST_CUSTOM      , 4, { ENAB,IN0,IN1,IN2 }, { ENAB,IN0,IN1,IN2 }, INFO, "3 input custom module" },
#define MW_DISCRETE_CUSTOM4(NODE,ENAB,IN0,IN1,IN2,IN3,INFO)                { NODE, mw_DST_CUSTOM      , 5, { ENAB,IN0,IN1,IN2,IN3 }, { ENAB,IN0,IN1,IN2,IN3 }, INFO, "4 input custom module" },
#define MW_DISCRETE_CUSTOM5(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,INFO)            { NODE, mw_DST_CUSTOM      , 6, { ENAB,IN0,IN1,IN2,IN3,IN4 }, { ENAB,IN0,IN1,IN2,IN3,IN4 }, INFO, "4 input custom module" },

#define MW_DISCRETE_555_ASTABLE(NODE,RESET,R1,R2,C,OPTIONS)                { NODE, mw_DSD_555_ASTBL   , 5, { RESET,R1,R2,C,NODE_NC }, { RESET,R1,R2,C,-1 }, OPTIONS, "555 Astable" },
#define MW_DISCRETE_555_ASTABLE_CV(NODE,RESET,R1,R2,C,CTRLV,OPTIONS)       { NODE, mw_DSD_555_ASTBL   , 5, { RESET,R1,R2,C,CTRLV }, { RESET,R1,R2,C,CTRLV }, OPTIONS, "555 Astable with CV" },
#define MW_DISCRETE_555_MSTABLE(NODE,RESET,TRIG,R,C,OPTIONS)               { NODE, mw_DSD_555_MSTBL   , 4, { RESET,TRIG,R,C }, { RESET,TRIG,R,C }, OPTIONS, "555 Monostable" },
#define MW_DISCRETE_555_CC(NODE,RESET,VIN,R,C,RBIAS,RGND,RDIS,OPTIONS)     { NODE, mw_DSD_555_CC      , 7, { RESET,VIN,R,C,RBIAS,RGND,RDIS }, { RESET,VIN,R,C,RBIAS,RGND,RDIS }, OPTIONS, "555 Constant Current VCO" },
#define MW_DISCRETE_555_VCO1(NODE,RESET,VIN,OPTIONS)                       { NODE, mw_DSD_555_VCO1    , 3, { RESET,VIN,NODE_NC }, { RESET,VIN,-1 }, OPTIONS, "555 VCO1 - Op-Amp type" },
#define MW_DISCRETE_555_VCO1_CV(NODE,RESET,VIN,CTRLV,OPTIONS)              { NODE, mw_DSD_555_VCO1    , 3, { RESET,VIN,CTRLV }, { RESET,VIN,CTRLV }, OPTIONS, "555 VCO1 with CV - Op-Amp type" },
#define MW_DISCRETE_566(NODE,ENAB,VMOD,R,C,OPTIONS)                        { NODE, mw_DSD_566         , 4, { ENAB,VMOD,R,C }, { ENAB,VMOD,R,C }, OPTIONS, "566" },

#define MW_DISCRETE_CSVLOG1(NODE1)                                    { NODE_SPECIAL, DSO_CSVLOG   , 1, { NODE1 }, { NODE1 }, NULL, "CSV Log 1 Node" },
#define MW_DISCRETE_CSVLOG2(NODE1,NODE2)                              { NODE_SPECIAL, DSO_CSVLOG   , 2, { NODE1,NODE2 }, { NODE1,NODE2 }, NULL, "CSV Log 2 Nodes" },
#define MW_DISCRETE_CSVLOG3(NODE1,NODE2,NODE3)                        { NODE_SPECIAL, DSO_CSVLOG   , 3, { NODE1,NODE2,NODE3 }, { NODE1,NODE2,NODE3 }, NULL, "CSV Log 3 Nodes" },
#define MW_DISCRETE_CSVLOG4(NODE1,NODE2,NODE3,NODE4)                  { NODE_SPECIAL, DSO_CSVLOG   , 4, { NODE1,NODE2,NODE3,NODE4 }, { NODE1,NODE2,NODE3,NODE4 }, NULL, "CSV Log 4 Nodes" },
#define MW_DISCRETE_CSVLOG5(NODE1,NODE2,NODE3,NODE4,NODE5)            { NODE_SPECIAL, DSO_CSVLOG   , 5, { NODE1,NODE2,NODE3,NODE4,NODE5 }, { NODE1,NODE2,NODE3,NODE4,NODE5 }, NULL, "CSV Log 5 Nodes" },
#define MW_DISCRETE_WAVELOG1(NODE1,GAIN1)                             { NODE_SPECIAL, DSO_WAVELOG  , 2, { NODE1,NODE_NC }, { NODE1,GAIN1 }, NULL, "Wave Log 1 Node" },
#define MW_DISCRETE_WAVELOG2(NODE1,GAIN1,NODE2,GAIN2)                 { NODE_SPECIAL, DSO_WAVELOG  , 4, { NODE1,NODE_NC,NODE2,NODE_NC }, { NODE1,GAIN1,NODE2,GAIN2 }, NULL, "Wave Log 2 Nodes" },
#define MW_DISCRETE_OUTPUT(OPNODE,GAIN)                               { NODE_SPECIAL, DSO_OUTPUT   , 2, { OPNODE,NODE_NC }, { 0,GAIN }, NULL, "Output Node" },

#define IS_VALUE_A_NODE(val)	(((val) > NODE_START) && ((val) <= NODE_END))


#define MW_DISC_OP_AMP_OSCILLATOR_TYPE_MASK	(0xf0 | MW_DISC_OP_AMP_IS_NORTON)
#define MW_DISC_OP_AMP_OSCILLATOR_1			0x00
#define MW_DISC_OP_AMP_OSCILLATOR_2			0x10
#define MW_DISC_OP_AMP_OSCILLATOR_VCO_1		0x20
#define MW_DISC_OP_AMP_OSCILLATOR_VCO_2		0x30
#define MW_DISC_OP_AMP_OSCILLATOR_VCO_3		0x40
#define MW_DISC_OP_AMP_OSCILLATOR_OUT_MASK			0x07
#define MW_DISC_OP_AMP_OSCILLATOR_OUT_CAP			0x00
#define MW_DISC_OP_AMP_OSCILLATOR_OUT_SQW			0x01
#define MW_DISC_OP_AMP_OSCILLATOR_OUT_ENERGY		0x02
#define MW_DISC_OP_AMP_OSCILLATOR_OUT_LOGIC_X		0x03
#define MW_DISC_OP_AMP_OSCILLATOR_OUT_COUNT_F_X	0x04
#define MW_DISC_OP_AMP_OSCILLATOR_OUT_COUNT_R_X	0x05
#define NODE_RELATIVE(n,delta) ((n)+(delta))
#define MW_DISC_COUNT_UP 1
#define RES_2_PARALLEL(a,b) (1.0 / (1.0/(a)+1.0/(b)))
#define RES_3_PARALLEL(a,b,c) (1.0 / (1.0/(a)+1.0/(b)+1.0/(c)))
#define FREQ_OF_555(r1,r2,c) (1.49 / (((r1) + 2 * (r2)) * (c)))
#define RC_CHARGE_EXP_DT(rc,dt) (1.0 - exp(-(dt)/(rc)))
#define RC_CHARGE_EXP(rc) RC_CHARGE_EXP_DT(rc, discrete_current_context->sample_time)
#endif

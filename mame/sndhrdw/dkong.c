// license:BSD-3-Clause
// copyright-holders:Couriersud
#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "sound/samples.h"
#include "sound/mw_discrete.h"

static UINT8 sh1_state[8];
static UINT8 sh1_count = 0;
static UINT8 walk = 0; /* used to determine if dkongjr is walking or climbing? */
static UINT8 death = 0;
static UINT8 drop = 0;
static UINT8 roar = 0;
static UINT8 land = 0;
static UINT8 climb = 0;
static UINT8 sh_climb_count;
static UINT8 snapjaw = 0;


SOUND_START( dkong )
{
	state_save_register_global_array(sh1_state);
	state_save_register_global(sh1_count);
	state_save_register_global(walk);
	state_save_register_global(death);
	state_save_register_global(drop);
	state_save_register_global(roar);
	state_save_register_global(land);
	state_save_register_global(climb);
	state_save_register_global(sh_climb_count);
	state_save_register_global(snapjaw);

	memset(sh1_state, 0, sizeof(sh1_state));
	sh1_count = 0;
	walk = 0;
	death = 0;
	drop = 0;
	roar = 0;
	land = 0;
	climb = 0;
	sh_climb_count = 0;
	snapjaw = 0;

	return 0;
}



WRITE8_HANDLER( dkong_sh_w )
{
	if (data)
		cpunum_set_input_line(1, 0, ASSERT_LINE);
	else
		cpunum_set_input_line(1, 0, CLEAR_LINE);
}

WRITE8_HANDLER( dkong_sh1_w )
{
	static const int sample_order[7] = {1,2,1,2,0,1,0};

	if (sh1_state[offset] != data)
	{
		if (data)
			{
				if (offset)
					sample_start (offset, offset + 2, 0);
				else
				{
					sample_start (offset, sample_order[sh1_count], 0);
					sh1_count++;
					if (sh1_count == 7)
						sh1_count = 0;
				}
			}
		sh1_state[offset] = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_death_w )
{
	if (death != data)
	{
		if (data)
			sample_stop (7);
		sample_start (6, 6, 0);
		death = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_drop_w )
{
	if (drop != data)
	{
		if (data)
			sample_start (7, 7, 0);
		drop = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_roar_w )
{
	if (roar != data)
	{
		if (data)
			sample_start (7,2,0);
		roar = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_jump_w )
{
	static int jump = 0;

	if (jump != data)
	{
		if (data)
			sample_start (6,0,0);
		jump = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_land_w )
{
	if (land != data)
	{
		if (data)
			sample_stop (7);
		sample_start (4,1,0);
		land = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_climb_w )
{
	static const int sample_order[7] = {1,2,1,2,0,1,0};

	if (climb != data)
	{
		if (data && walk == 0)
		{
			sample_start (3,sample_order[sh_climb_count]+3,0);
			sh_climb_count++;
			if (sh_climb_count == 7) sh_climb_count = 0;
		}
		else if (data && walk == 1)
		{
			sample_start (3,sample_order[sh_climb_count]+8,0);
			sh_climb_count++;
			if (sh_climb_count == 7) sh_climb_count = 0;
		}
		climb = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_snapjaw_w )
{
	if (snapjaw != data)
	{
		if (data)
			sample_stop (7);
		sample_start (4,11,0);
		snapjaw = data;
	}
}

WRITE8_HANDLER( dkongjr_sh_walk_w )
{
	if (walk != data )
	{
		walk = data;
	}
}

#define DS_SOUND0_INV		NODE_01
#define DS_SOUND1_INV		NODE_02
#define DS_SOUND2_INV		NODE_03
#define DS_DAC				NODE_06
#define DS_DAC_DISCHARGE	NODE_07

#define DS_SOUND0			NODE_208
#define DS_SOUND1			NODE_209
#define DS_SOUND2			NODE_210

#define DS_ADJ_DAC			NODE_240

#define DS_OUT_SOUND0		NODE_241
#define DS_OUT_SOUND1		NODE_242
#define DS_OUT_SOUND2		NODE_243
#define DS_OUT_DAC			NODE_249


#define DK_1N5553_V			0.4
#define DK_SUP_V			5.0
#define NE555_INTERNAL_R	RES_K(5)

#define R_PARALLEL(R1,R2) ((R1)*(R2)/((R1)+(R2)))
#define R_SERIE(R1,R2)	  ((R1)+(R2))

#define DK_VR1		RES_K(10)

#define DK_R1 		RES_K(47)
#define DK_R2 		RES_K(47)
#define DK_R3		RES_K(5.1)
#define DK_R4		RES_K(2)
#define DK_R5		750
#define DK_R6		RES_K(4.7)
#define DK_R7		RES_K(10)
#define DK_R8		RES_K(100)
#define DK_R9		RES_K(10)
#define DK_R10 		RES_K(10)
#define DK_R14 		RES_K(47)

#define DK_R15		RES_K(5.6)
#define DK_R16		RES_K(5.6)
#define DK_R17		RES_K(10)
#define DK_R18		RES_K(4.7)
#define DK_R20		RES_K(10)
#define DK_R24 		RES_K(47)
#define DK_R25		RES_K(5.1)
#define DK_R26		RES_K(2)
#define DK_R27		150
#define DK_R28		RES_K(4.7)
#define DK_R29		RES_K(10)
#define DK_R30		RES_K(100)
#define DK_R31 		RES_K(10)
#define DK_R32 		RES_K(10)
#define DK_R35		RES_K(1)
#define DK_R36		RES_K(1)
#define DK_R38		RES_K(18)
#define DK_R39		RES_M(3.3)
#define	DK_R49		RES_K(1.2)
#define	DK_R44		RES_K(1.2)
#define DK_R45		RES_K(10)
#define DK_R46		RES_K(12)
#define DK_R47		RES_K(4.3)
#define DK_R48		RES_K(43)
#define DK_R50		RES_K(10)
#define DK_R51		RES_K(10)

#define CAP_U_AGED(x) (1*CAP_U(x))

#define DK_C12		CAP_U_AGED(1)
#define DK_C13		CAP_U(33)
#define DK_C16		CAP_U_AGED(1)
#define DK_C17		CAP_U_AGED(4.7)
#define DK_C18		CAP_U_AGED(1)
#define DK_C19		CAP_U_AGED(1)
#define DK_C20		CAP_U_AGED(3.3)
#define DK_C21		CAP_U_AGED(1)

#define DK_C23		CAP_U_AGED(4.7)
#define	DK_C24		CAP_U_AGED(10)
#define DK_C25		CAP_U_AGED(3.3)
#define	DK_C26		CAP_U_AGED(3.3)
#define	DK_C29		CAP_U_AGED(3.3)
#define DK_C30		CAP_U_AGED(10)
#define DK_C32		CAP_U_AGED(10)
#define DK_C160		CAP_N(100)

static const mw_discrete_lfsr_desc dkong_lfsr =
{
	MW_DISC_CLK_IS_FREQ,
	24,
	0,
	10,
	23,
	MW_DISC_LFSR_XOR,
	MW_DISC_LFSR_NOT_IN0,
	MW_DISC_LFSR_REPLACE,
	0x000001,
	MW_DISC_LFSR_FLAG_OUTPUT_F0,
	0
};

static const mw_discrete_mixer_desc dkong_rc_jump_desc =
	{MW_DISC_MIXER_IS_RESISTOR,
	{1, DK_R49+DK_R51,NE555_INTERNAL_R,2*NE555_INTERNAL_R},
	{NODE_26,0,0,0},
	{0,0,0,0},
	0, 0,
	DK_C24,
	0,
	0, 1};

static const mw_discrete_mixer_desc dkong_rc_walk_desc =
	{MW_DISC_MIXER_IS_RESISTOR,
	{1, DK_R45+DK_R44,NE555_INTERNAL_R,2*NE555_INTERNAL_R},
	{NODE_52,0,0,0},
	{0,0,0,0},
	0, 0,
	DK_C29,
	0,
	0, 1};

static const mw_discrete_mixer_desc dkong_mixer_desc =
	{MW_DISC_MIXER_IS_RESISTOR,
	{DK_R2, DK_R24, DK_R1, DK_R14},
	{0,0,0},
	{0,0,0},
	0, DK_VR1,
	DK_C160,
	DK_C12,
	0, 1};

static const mw_discrete_555_desc dkong_555_vco_desc =
	{MW_DISC_555_OUT_DC | MW_DISC_555_OUT_ENERGY,
	DK_SUP_V,
	DK_SUP_V-0.5,DK_SUP_V*0.66,DK_SUP_V*0.33
	};

static const mw_discrete_inverter_osc_desc dkong_inverter_osc_desc_jump =
	{MW_DEFAULT_CD40XX_VALUES(DK_SUP_V),
	MW_DISC_OSC_INVERTER_IS_TYPE1
	};

static const mw_discrete_inverter_osc_desc dkong_inverter_osc_desc_walk =
	{MW_DEFAULT_CD40XX_VALUES(DK_SUP_V),
	MW_DISC_OSC_INVERTER_IS_TYPE2
	};

DISCRETE_SOUND_START(dkong_conversion)

	MW_DISCRETE_INPUT_NOT(DS_SOUND2_INV)
	MW_DISCRETE_INPUT_NOT(DS_SOUND1_INV)
	MW_DISCRETE_INPUT_NOT(DS_SOUND0_INV)
	MW_DISCRETE_INPUT_LOGIC(DS_DAC_DISCHARGE)
	MW_DISCRETE_INPUT_DATA(DS_DAC)

	MW_DISCRETE_CONSTANT(DS_ADJ_DAC, 1)

	MW_DISCRETE_LOGIC_INVERT(DS_SOUND0,1,DS_SOUND0_INV)
	MW_DISCRETE_LOGIC_INVERT(DS_SOUND1,1,DS_SOUND1_INV)
	MW_DISCRETE_LOGIC_INVERT(DS_SOUND2,1,DS_SOUND2_INV)

	MW_DISCRETE_LFSR_NOISE(NODE_11, 1, 1, (6144000.0 / 2 / 384 / 2), 1.0, 0, 0.5, &dkong_lfsr)
	MW_DISCRETE_COUNTER(NODE_12, 1, 0, NODE_11, 7, MW_DISC_COUNT_UP, 0, MW_DISC_CLK_ON_R_EDGE)
	MW_DISCRETE_TRANSFORM3(NODE_13,1,NODE_12,3,DK_SUP_V,"01>2*")

	MW_DISCRETE_RCDISC_MODULATED(NODE_15,1,DS_SOUND2_INV,0,DK_R10,0,0,DK_R9,DK_C21,DK_SUP_V)

	MW_DISCRETE_TRANSFORM2(NODE_16, 1, NODE_15, 0.6, "01>")
	MW_DISCRETE_RCDISC2(NODE_17,NODE_16,DK_SUP_V,DK_R8+DK_R7,0.0,DK_R7,DK_C20)

	MW_DISCRETE_DIODE_MIXER2(NODE_18, 1, DK_1N5553_V, NODE_13, NODE_13)
	MW_DISCRETE_DIODE_MIXER2(NODE_20, 1, DK_1N5553_V, NODE_17, NODE_18)

	MW_DISCRETE_RCINTEGRATE(NODE_22,1,NODE_20,DK_R5, R_PARALLEL(DK_R4+DK_R3,DK_R6),0,DK_C19,DK_SUP_V,MW_DISC_RC_INTEGRATE_TYPE1)
	MW_DISCRETE_MULTIPLY(DS_OUT_SOUND0,1,NODE_22,DK_R3/R_SERIE(DK_R3,DK_R4))

	MW_DISCRETE_MULTIPLY(NODE_24,1,DS_SOUND1,DK_SUP_V)

	MW_DISCRETE_INVERTER_OSC(NODE_25,1,0,DK_R38,DK_R39,DK_C26,0,&dkong_inverter_osc_desc_jump)

	MW_DISCRETE_TRANSFORM3(NODE_26,1,DS_SOUND1,DK_R32,DK_R49+DK_R50,"01*2+")
	MW_DISCRETE_MIXER4(NODE_28, 1, NODE_24, NODE_25, DK_SUP_V, 0,&dkong_rc_jump_desc)

	MW_DISCRETE_555_ASTABLE_CV(NODE_29, 1, RES_K(47), RES_K(27), CAP_N(47), NODE_28,
	&dkong_555_vco_desc)

	MW_DISCRETE_RCDISC_MODULATED(NODE_33,1,DS_SOUND1_INV,0,DK_R32,0,0,DK_R31,DK_C18,DK_SUP_V)

	MW_DISCRETE_TRANSFORM2(NODE_34, 1, NODE_33, 0.6, "01>")
	MW_DISCRETE_RCDISC2(NODE_35, NODE_34,DK_SUP_V,R_SERIE(DK_R30,DK_R29),0.0,DK_R29,DK_C17)

	MW_DISCRETE_DIODE_MIXER2(NODE_36, 1, DK_1N5553_V, NODE_29, NODE_29)
	MW_DISCRETE_DIODE_MIXER2(NODE_38, 1, DK_1N5553_V, NODE_36, NODE_35)

	MW_DISCRETE_RCINTEGRATE(NODE_39,1,NODE_38,DK_R27, R_PARALLEL(DK_R28,DK_R26+DK_R25),0,DK_C16,DK_SUP_V,MW_DISC_RC_INTEGRATE_TYPE1)
	MW_DISCRETE_MULTIPLY(DS_OUT_SOUND1,1,NODE_39,DK_R25/(DK_R26+DK_R25))

	MW_DISCRETE_MULTIPLY(NODE_50,1,DS_SOUND0,DK_SUP_V)
	MW_DISCRETE_INVERTER_OSC(NODE_51,1,0,DK_R47,DK_R48,DK_C30,0,&dkong_inverter_osc_desc_walk)

	MW_DISCRETE_TRANSFORM3(NODE_52,1,DS_SOUND0,DK_R46,R_SERIE(DK_R44,DK_R45),"01*2+")
	MW_DISCRETE_MIXER4(NODE_54, 1, NODE_50, NODE_51, DK_SUP_V, 0,&dkong_rc_walk_desc)

	MW_DISCRETE_555_ASTABLE_CV(NODE_55, 1, RES_K(47), RES_K(27), CAP_N(33), NODE_54, &dkong_555_vco_desc)

	MW_DISCRETE_RCDISC_MODULATED(NODE_60,1,DS_SOUND0_INV,NODE_55,DK_R36,DK_R18,DK_R35,DK_R17,DK_C25,DK_SUP_V)

	MW_DISCRETE_CRFILTER(NODE_61, 1, NODE_60, DK_R15+DK_R16, DK_C23)
	MW_DISCRETE_MULTIPLY(DS_OUT_SOUND2, 1, NODE_61, DK_R15/(DK_R15+DK_R16))

	MW_DISCRETE_RCDISC(NODE_70, DS_DAC_DISCHARGE, 1, DK_R20, DK_C32)
	MW_DISCRETE_TRANSFORM4(NODE_71, 1, DS_DAC,  DK_SUP_V/256.0, NODE_70, DS_DAC_DISCHARGE, "01*3!2+*")

	MW_DISCRETE_FILTER2(NODE_73, 1, NODE_71, 1916, (1.0/0.74), MW_DISC_FILTER_LOWPASS)

	MW_DISCRETE_MULTIPLY(DS_OUT_DAC, 1, NODE_73, DS_ADJ_DAC)

	MW_DISCRETE_MIXER4(NODE_288, 1, DS_OUT_SOUND0, DS_OUT_SOUND1, DS_OUT_DAC, DS_OUT_SOUND2, &dkong_mixer_desc)

	MW_DISCRETE_ADDER2(NODE_289,1,NODE_288,5.0*43.0/(100.0+43.0))
	MW_DISCRETE_RCINTEGRATE(NODE_294,1,NODE_289,0,150,1000, CAP_U(33),DK_SUP_V,MW_DISC_RC_INTEGRATE_TYPE3)
	MW_DISCRETE_CRFILTER(NODE_295,1,NODE_294, RES_K(50), DK_C13)
	MW_DISCRETE_CRFILTER(NODE_296,1,NODE_295, RES_K(1), CAP_U(4.7))
	MW_DISCRETE_OUTPUT(NODE_296, 32767.0/5.0 * 1.5)

DISCRETE_SOUND_END

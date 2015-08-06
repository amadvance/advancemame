/* 8080bw.c *********************************
 updated: 1997-04-09 08:46 TT
 updated  20-3-1998 LT Added color changes on base explosion
 *
 * Author      : Tormod Tjaberg
 * Created     : 1997-04-09
 * Description : Sound routines for the 'invaders' games
 *
 * Note:
 * The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 * excellent space invader emulator and converted to signed samples so
 * they would work under SEAL. The port info was also gleaned from
 * his emulator. These sounds should also work on all the invader games.
 *
 * The sounds are generated using output port 3 and 5
 *
 * Port 3:
 * bit 0=UFO  (repeats)       emulated
 * bit 1=Shot                 1.raw
 * bit 2=Base hit             2.raw
 * bit 3=Invader hit          3.raw
 * bit 4=Bonus base           9.raw
 *
 * Port 5:
 * bit 0=Fleet movement 1     4.raw
 * bit 1=Fleet movement 2     5.raw
 * bit 2=Fleet movement 3     6.raw
 * bit 3=Fleet movement 4     7.raw
 * bit 4=UFO 2                8.raw
 */
#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "machine/74123.h"
#include "8080bw.h"
#include "sound/samples.h"
#include "sound/sn76477.h"
#include "sound/discrete.h"
#include "sound/dac.h"
#include "sound/custom.h"

static WRITE8_HANDLER( invad2ct_sh_port1_w );
static WRITE8_HANDLER( invaders_sh_port3_w );
static WRITE8_HANDLER( invaders_sh_port5_w );
static WRITE8_HANDLER( invad2ct_sh_port7_w );

static WRITE8_HANDLER( ballbomb_sh_port3_w );
static WRITE8_HANDLER( ballbomb_sh_port5_w );

static WRITE8_HANDLER( boothill_sh_port3_w );
static WRITE8_HANDLER( boothill_sh_port5_w );

static WRITE8_HANDLER( clowns_sh_port7_w );

static WRITE8_HANDLER( seawolf_sh_port5_w );

static WRITE8_HANDLER( schaser_sh_port3_w );
static WRITE8_HANDLER( schaser_sh_port5_w );

static WRITE8_HANDLER( polaris_sh_port2_w );
static WRITE8_HANDLER( polaris_sh_port4_w );
static WRITE8_HANDLER( polaris_sh_port6_w );


mame_timer *schaser_effect_555_timer;
static double schaser_effect_555_time_remain;
static int schaser_effect_555_is_low;
static int explosion;


struct SN76477interface invaders_sn76477_interface =
{
	0	/* N/C */,		/*  4  noise_res         */
	0	/* N/C */,		/*  5  filter_res        */
	0	/* N/C */,		/*  6  filter_cap        */
	0	/* N/C */,		/*  7  decay_res         */
	0	/* N/C */,		/*  8  attack_decay_cap  */
	RES_K(100) ,		/* 10  attack_res        */
	RES_K(56)  ,		/* 11  amplitude_res     */
	RES_K(10)  ,		/* 12  feedback_res      */
	0	/* N/C */,		/* 16  vco_voltage       */
	CAP_U(0.1) ,		/* 17  vco_cap           */
	RES_K(8.2) ,		/* 18  vco_res           */
	5.0		 ,		/* 19  pitch_voltage     */
	RES_K(120) ,		/* 20  slf_res           */
	CAP_U(1.0) ,		/* 21  slf_cap           */
	0	/* N/C */,		/* 23  oneshot_cap       */
	0	/* N/C */		/* 24  oneshot_res       */
};

static const char *invaders_sample_names[] =
{
	"*invaders",
	"1.wav",	/* Shot/Missle */
	"2.wav",	/* Base Hit/Explosion */
	"3.wav",	/* Invader Hit */
	"4.wav",	/* Fleet move 1 */
	"5.wav",	/* Fleet move 2 */
	"6.wav",	/* Fleet move 3 */
	"7.wav",	/* Fleet move 4 */
	"8.wav",	/* UFO/Saucer Hit */
	"9.wav",	/* Bonus Base */
	0       /* end of array */
};

struct Samplesinterface invaders_samples_interface =
{
	4,	/* 4 channels */
	invaders_sample_names
};


struct SN76477interface invad2ct_sn76477_interface_1 =
{
	0,    /* N/C */	/*  4  noise_res         */
	0,    /* N/C */	/*  5  filter_res        */
	0,    /* N/C */	/*  6  filter_cap        */
	0,    /* N/C */	/*  7  decay_res         */
	0,    /* N/C */	/*  8  attack_decay_cap  */
	RES_K(100), 	/* 10  attack_res        */
	RES_K(56),  	/* 11  amplitude_res     */
	RES_K(10),  	/* 12  feedback_res      */
	0,    /* N/C */	/* 16  vco_voltage       */
	CAP_U(0.1), 	/* 17  vco_cap           */
	RES_K(8.2), 	/* 18  vco_res           */
	5.0,        	/* 19  pitch_voltage     */
	RES_K(120), 	/* 20  slf_res           */
	CAP_U(1.0), 	/* 21  slf_cap           */
	0,    /* N/C */	/* 23  oneshot_cap       */
	0,    /* N/C */	/* 24  oneshot_res       */
};

struct SN76477interface invad2ct_sn76477_interface_2 =
{
	0	/* N/C */  ,	/*  4  noise_res         */
	0	/* N/C */  ,	/*  5  filter_res        */
	0	/* N/C */  ,	/*  6  filter_cap        */
	0	/* N/C */  ,	/*  7  decay_res         */
	0	/* N/C */  ,	/*  8  attack_decay_cap  */
	RES_K(100)   ,	/* 10  attack_res        */
	RES_K(56)    ,	/* 11  amplitude_res     */
	RES_K(10)    ,	/* 12  feedback_res      */
	0	/* N/C */  ,	/* 16  vco_voltage       */
	CAP_U(0.047) ,	/* 17  vco_cap           */
	RES_K(39)    ,	/* 18  vco_res           */
	5.0		   ,	/* 19  pitch_voltage     */
	RES_K(120)   ,	/* 20  slf_res           */
	CAP_U(1.0)   ,	/* 21  slf_cap           */
	0	/* N/C */,	/* 23  oneshot_cap       */
	0	/* N/C */	/* 24  oneshot_res       */
};

static const char *invad2ct_sample_names[] =
{
	"*invaders",
	"1.wav",	/* Shot/Missle - Player 1 */
	"2.wav",	/* Base Hit/Explosion - Player 1 */
	"3.wav",	/* Invader Hit - Player 1 */
	"4.wav",	/* Fleet move 1 - Player 1 */
	"5.wav",	/* Fleet move 2 - Player 1 */
	"6.wav",	/* Fleet move 3 - Player 1 */
	"7.wav",	/* Fleet move 4 - Player 1 */
	"8.wav",	/* UFO/Saucer Hit - Player 1 */
	"9.wav",	/* Bonus Base - Player 1 */
	"11.wav",	/* Shot/Missle - Player 2 */
	"12.wav",	/* Base Hit/Explosion - Player 2 */
	"13.wav",	/* Invader Hit - Player 2 */
	"14.wav",	/* Fleet move 1 - Player 2 */
	"15.wav",	/* Fleet move 2 - Player 2 */
	"16.wav",	/* Fleet move 3 - Player 2 */
	"17.wav",	/* Fleet move 4 - Player 2 */
	"18.wav",	/* UFO/Saucer Hit - Player 2 */
	0       /* end of array */
};

struct Samplesinterface invad2ct_samples_interface =
{
	8,	/* 8 channels */
	invad2ct_sample_names
};


MACHINE_RESET( invaders )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x03, 0x03, 0, 0, invaders_sh_port3_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, invaders_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

MACHINE_RESET( sstrangr )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x42, 0x42, 0, 0, invaders_sh_port3_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x44, 0x44, 0, 0, invaders_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

MACHINE_RESET( invad2ct )
{
	machine_reset_invaders();

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x01, 0x01, 0, 0, invad2ct_sh_port1_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x07, 0x07, 0, 0, invad2ct_sh_port7_w);

	SN76477_envelope_1_w(1, 1);
	SN76477_envelope_2_w(1, 0);
	SN76477_mixer_a_w(1, 0);
	SN76477_mixer_b_w(1, 0);
	SN76477_mixer_c_w(1, 0);
	SN76477_vco_w(1, 1);
}


/*
   Note: For invad2ct, the Player 1 sounds are the same as for the
         original and deluxe versions.  Player 2 sounds are all
         different, and are triggered by writes to port 1 and port 7.

*/

static void invaders_sh_1_w(int board, int data, unsigned char *last)
{
	int base_channel, base_sample;

	base_channel = 4 * board;
	base_sample  = 9 * board;

	SN76477_enable_w(board, !(data & 0x01));				/* Saucer Sound */

	if (data & 0x02 && ~*last & 0x02)
		sample_start (base_channel+0, base_sample+0, 0);	/* Shot Sound */

	if (data & 0x04 && ~*last & 0x04)
		sample_start (base_channel+1, base_sample+1, 0);	/* Base Hit */

	if (~data & 0x04 && *last & 0x04)
		sample_stop (base_channel+1);

	if (data & 0x08 && ~*last & 0x08)
		sample_start (base_channel+0, base_sample+2, 0);	/* Invader Hit */

	if (data & 0x10 && ~*last & 0x10)
		sample_start (base_channel+2, 8, 0);				/* Bonus Missle Base */

	c8080bw_screen_red_w(data & 0x04);

	*last = data;
}

static void invaders_sh_2_w(int board, int data, unsigned char *last)
{
	int base_channel, base_sample;

	base_channel = 4 * board;
	base_sample  = 9 * board;

	if (data & 0x01 && ~*last & 0x01)
		sample_start (base_channel+1, base_sample+3, 0);	/* Fleet 1 */

	if (data & 0x02 && ~*last & 0x02)
		sample_start (base_channel+1, base_sample+4, 0);	/* Fleet 2 */

	if (data & 0x04 && ~*last & 0x04)
		sample_start (base_channel+1, base_sample+5, 0);	/* Fleet 3 */

	if (data & 0x08 && ~*last & 0x08)
		sample_start (base_channel+1, base_sample+6, 0);	/* Fleet 4 */

	if (data & 0x10 && ~*last & 0x10)
		sample_start (base_channel+3, base_sample+7, 0);	/* Saucer Hit */

	c8080bw_flip_screen_w(data & 0x20);

	*last = data;
}


static WRITE8_HANDLER( invad2ct_sh_port1_w )
{
	static unsigned char last = 0;

	invaders_sh_1_w(1, data, &last);
}

static WRITE8_HANDLER( invaders_sh_port3_w )
{
	static unsigned char last = 0;

	invaders_sh_1_w(0, data, &last);
}

static WRITE8_HANDLER( invaders_sh_port5_w )
{
	static unsigned char last = 0;

	invaders_sh_2_w(0, data, &last);
}

static WRITE8_HANDLER( invad2ct_sh_port7_w )
{
	static unsigned char last = 0;

	invaders_sh_2_w(1, data, &last);
}


/*******************************************************/
/*                                                     */
/* Midway "Gun Fight"                                  */
/*                                                     */
/*******************************************************/

MACHINE_RESET( gunfight )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x00, 0x00, 0, 0, gunfight_port_0_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x01, 0x01, 0, 0, gunfight_port_1_r);
}


/*******************************************************/
/*                                                     */
/* Midway "Boot Hill"                                  */
/*                                                     */
/*******************************************************/

static const char *boothill_sample_names[] =
{
	"*boothill", /* in case we ever find any bootlegs hehehe */
	"addcoin.wav",
	"endgame.wav",
	"gunshot.wav",
	"killed.wav",
	0       /* end of array */
};

struct Samplesinterface boothill_samples_interface =
{
	9,	/* 9 channels */
	boothill_sample_names
};


/* HC 4/14/98 NOTE: *I* THINK there are sounds missing...
i dont know for sure... but that is my guess....... */

MACHINE_RESET( boothill )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x00, 0x00, 0, 0, boothill_port_0_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x01, 0x01, 0, 0, boothill_port_1_r);

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x03, 0x03, 0, 0, boothill_sh_port3_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, boothill_sh_port5_w);
}

static WRITE8_HANDLER( boothill_sh_port3_w )
{
	switch (data)
	{
		case 0x0c:
			sample_start (0, 0, 0);
			break;

		case 0x18:
		case 0x28:
			sample_start (1, 2, 0);
			break;

		case 0x48:
		case 0x88:
			sample_start (2, 3, 0);
			break;
	}
}

/* HC 4/14/98 */
static WRITE8_HANDLER( boothill_sh_port5_w )
{
	switch (data)
	{
		case 0x3b:
			sample_start (2, 1, 0);
			break;
	}
}


/*******************************************************/
/*                                                     */
/* Taito "Balloon Bomber"                              */
/*                                                     */
/*******************************************************/

/* This only does the color swap for the explosion */
/* We do not have correct samples so sound not done */

MACHINE_RESET( ballbomb )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x03, 0x03, 0, 0, ballbomb_sh_port3_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, ballbomb_sh_port5_w);
}

static WRITE8_HANDLER( ballbomb_sh_port3_w )
{
	c8080bw_screen_red_w(data & 0x04);
}

static WRITE8_HANDLER( ballbomb_sh_port5_w )
{
	c8080bw_flip_screen_w(data & 0x20);
}


/*******************************************************/
/*                                                     */
/* Taito "Indian Battle"                               */
/* Sept 2005, D.R.                                     */
/*******************************************************/
const struct discrete_dac_r1_ladder indianbt_music_dac =
	{3, {0, RES_K(47), RES_K(12)}, 0, 0, 0, CAP_U(0.1)};

#define INDIANBT_MUSIC_CLK		(7680.0*2*2*2)

/* Nodes - Inputs */
#define INDIANBT_MUSIC_DATA		NODE_01
/* Nodes - Sounds */
#define INDIANBT_MUSIC			NODE_11

DISCRETE_SOUND_START(indianbt_discrete_interface)

	DISCRETE_INPUT_DATA (INDIANBT_MUSIC_DATA)

/******************************************************************************
 *
 * Music Generator
 *
 ******************************************************************************/
	DISCRETE_NOTE(NODE_20, 1, INDIANBT_MUSIC_CLK, INDIANBT_MUSIC_DATA, 255, 5, DISC_CLK_IS_FREQ)

	// Convert count to 7492 output
	DISCRETE_TRANSFORM2(NODE_21, 1, NODE_20, 2, "01>0+")

	DISCRETE_DAC_R1(NODE_22, 1, NODE_21, DEFAULT_TTL_V_LOGIC_1, &indianbt_music_dac)

/******************************************************************************
 *
 * Final Mixing and Output
 *
 ******************************************************************************/
	DISCRETE_CRFILTER(NODE_90, 1, NODE_22, RES_K(10), CAP_U(0.1))

	DISCRETE_OUTPUT(NODE_90, 21000)

DISCRETE_SOUND_END

WRITE8_HANDLER( indianbt_sh_port7_w )
{
	discrete_sound_w(INDIANBT_MUSIC_DATA, data);
}


/*******************************************************/
/*                                                     */
/* Taito "Polaris"                                     */
/* D.R.                                                */
/*******************************************************/
// The R/C values in the schematic may have no bearing in reality.
// I have noted some differences from a real board.

const struct discrete_lfsr_desc polaris_lfsr={
	DISC_CLK_IS_FREQ,
	17,			/* Bit Length */
	0,			/* Reset Value */
	4,			/* Use Bit 4 as XOR input 0 */
	16,			/* Use Bit 16 as XOR input 1 */
	DISC_LFSR_XOR,		/* Feedback stage1 is XOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is not inverted */
	12			/* Output bit */
};

const struct discrete_dac_r1_ladder polaris_music_dac =
	{2, {RES_K(47), RES_K(12), 0,0,0,0,0,0}, 0, 0, 0, CAP_P(180)};

const struct discrete_op_amp_filt_info polaris_music_op_amp_filt_info =
	{RES_K(580), 0, 0, RES_M(2.2), RES_M(1), CAP_U(.01), 0, 0, 0, 12, 0};

const struct discrete_op_amp_filt_info polaris_nol_op_amp_filt_info =
	{560, RES_K(6.8), RES_K(1002), RES_M(2.2), RES_M(1), CAP_U(.22), CAP_U(.22), CAP_U(1), 0, 12, 0};

const struct discrete_op_amp_filt_info polaris_noh_op_amp_filt_info =
	{560, RES_K(6.8), RES_K(1002), RES_M(2.2), RES_M(1), CAP_U(.001), CAP_U(.001), CAP_U(.01), 0, 12, 0};

const struct discrete_op_amp_osc_info polaris_sonar_vco_info =
	{DISC_OP_AMP_OSCILLATOR_VCO_1 | DISC_OP_AMP_IS_NORTON, RES_M(1), RES_K(680), RES_K(680), RES_M(1), RES_M(1), RES_K(120), RES_M(1), 0, CAP_P(180), 12};

const struct discrete_op_amp_tvca_info polaris_sonar_tvca_info =
	{ RES_M(2.7), RES_K(680), 0, RES_K(680), RES_K(1), RES_K(120), RES_K(560), 0, 0, 0, 0, CAP_U(1), 0, 0, 12, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_TRG1, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0_INV, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE};

const struct discrete_op_amp_osc_info polaris_boat_mod_info =
	{DISC_OP_AMP_OSCILLATOR_1 | DISC_OP_AMP_IS_NORTON, RES_M(1), RES_K(10), RES_K(100), RES_K(120), RES_M(1), 0, 0, 0, CAP_U(.22), 12};

const struct discrete_op_amp_osc_info polaris_boat_vco_info =
	{DISC_OP_AMP_OSCILLATOR_VCO_1 | DISC_OP_AMP_IS_NORTON, RES_M(1), RES_K(680), RES_K(680), RES_M(1), RES_M(1), 0, 0, 0, CAP_P(180), 12};

const struct discrete_op_amp_tvca_info polaris_shot_tvca_info =
	{ RES_M(2.7), RES_K(680), RES_K(680), RES_K(680), RES_K(1), 0, RES_K(680), 0, 0, 0, 0, CAP_U(1), 0, 0, 12, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0_INV, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE};

const struct discrete_op_amp_tvca_info polaris_shiphit_tvca_info =
	{ RES_M(2.7), RES_K(680), RES_K(680), RES_K(680), RES_K(1), 0, RES_K(680), 0, 0, 0, 0, CAP_U(1), 0, 0, 12, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0_INV, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE};

const struct discrete_op_amp_tvca_info polaris_exp_tvca_info =
	{ RES_M(2.7), RES_K(680), 0, RES_K(680), RES_K(1), 0, RES_K(680), 0, 0, 0, 0, CAP_U(.33), 0, 0, 12, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE};

// The schematic shows a .22uF cap but Guru's board has a 1uF
const struct discrete_op_amp_tvca_info polaris_hit_tvca_info =
	{ RES_M(2.7), RES_K(1360), RES_K(1360), RES_K(680), RES_K(1), 0, RES_K(680), 0, 0, 0, 0, CAP_U(1), 0, 0, 12, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_TRG1, DISC_OP_AMP_TRIGGER_FUNCTION_TRG01_NAND, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE};

// The schematic shows a 1uF cap but Guru's board has a 2.2uF
const struct discrete_integrate_info polaris_plane_integrate_info =
	{DISC_INTEGRATE_OP_AMP_2 | DISC_OP_AMP_IS_NORTON, RES_K(1001), RES_K(1001), RES_K(101), CAP_U(2.2), 12, 12, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0_INV, DISC_OP_AMP_TRIGGER_FUNCTION_TRG1_INV};

// A bit of a cheat.  The schematic show the cap as 47p, but that makes the frequency too high.
// Guru's board has a .01 cap, which would make the freq sub-sonic using the other schematic values.
// I will use 2000p until the proper value can be confirmed.
const struct discrete_op_amp_osc_info polaris_plane_vco_info =
	{DISC_OP_AMP_OSCILLATOR_VCO_1 | DISC_OP_AMP_IS_NORTON, RES_M(1), RES_K(680), RES_K(680), RES_M(1), RES_M(1), RES_K(100), RES_K(10), RES_K(100), CAP_U(0.002), 12};

const struct discrete_mixer_desc polaris_mixer_vr1_desc =
	{DISC_MIXER_IS_RESISTOR,
		{RES_K(66), RES_K(43), RES_K(20), RES_K(43)},
		{0},	// no variable resistors
		{CAP_U(1), CAP_U(1), CAP_U(1), CAP_U(1)},
		0, RES_K(50), 0, 0, 0, 1};

const struct discrete_mixer_desc polaris_mixer_vr2_desc =
	{DISC_MIXER_IS_RESISTOR,
		{RES_K(66), RES_K(110)},
		{0},	// no variable resistors
		{CAP_U(1), CAP_U(1)},
		0, RES_K(50), 0, 0, 0, 1};

// Note: the final gain leaves the explosions (SX3) at a level
// where they clip.  From the schematics, this is how they wanted it.
// This makes them have more bass and distortion.
const struct discrete_mixer_desc polaris_mixer_vr4_desc =
	{DISC_MIXER_IS_RESISTOR,
		{RES_K(22), RES_K(20), RES_K(22), RES_K(22)},
		{0},	// no variable resistors
		{0, CAP_U(1), 0, 0},
		0, RES_K(50), 0, CAP_U(1), 0, 40000};

/* Nodes - Inputs */
#define POLARIS_MUSIC_DATA		NODE_01
#define POLARIS_SX0_EN			NODE_02
#define POLARIS_SX1_EN			NODE_03
#define POLARIS_SX2_EN			NODE_04
#define POLARIS_SX3_EN			NODE_05
#define POLARIS_SX5_EN			NODE_06
#define POLARIS_SX6_EN			NODE_07
#define POLARIS_SX7_EN			NODE_08
#define POLARIS_SX9_EN			NODE_09
#define POLARIS_SX10_EN			NODE_10
/* Nodes - Sounds */
#define POLARIS_MUSIC			NODE_11
#define POLARIS_NOISE_LO		NODE_12
#define POLARIS_NOISE_LO_FILT	NODE_13
#define POLARIS_NOISE_HI_FILT	NODE_14
#define POLARIS_SHOTSND			NODE_15
#define POLARIS_SHIP_HITSND		NODE_16
#define POLARIS_SHIPSND			NODE_17
#define POLARIS_EXPLOSIONSND	NODE_18
#define POLARIS_PLANESND		NODE_19
#define POLARIS_HITSND			NODE_20
#define POLARIS_SONARSND		NODE_21
/* Nodes - Adjust */
#define POLARIS_ADJ_VR1			NODE_23
#define POLARIS_ADJ_VR2			NODE_24
#define POLARIS_ADJ_VR3			NODE_25

DISCRETE_SOUND_START(polaris_discrete_interface)

	/************************************************/
	/* Polaris sound system: 8 Sound Sources        */
	/*                                              */
	/* Relative volumes are adjustable              */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for polaris           */
	/************************************************/
	DISCRETE_INPUT_DATA (POLARIS_MUSIC_DATA)
	DISCRETE_INPUT_LOGIC(POLARIS_SX0_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX1_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX2_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX3_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX5_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX6_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX7_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX9_EN)
	DISCRETE_INPUT_LOGIC(POLARIS_SX10_EN)

	/* We will cheat and just use the controls to scale the amplitude. */
	/* It is the same as taking the (0 to 50k)/50k */
	DISCRETE_ADJUSTMENT(POLARIS_ADJ_VR1, 1, 0, 1, DISC_LINADJ, 4)
	DISCRETE_ADJUSTMENT(POLARIS_ADJ_VR2, 1, 0, 1, DISC_LINADJ, 5)
	/* Extra cheating for VR3.  We will include the resistor scaling. */
	DISCRETE_ADJUSTMENT(POLARIS_ADJ_VR3, 1, 0, 0.5376, DISC_LINADJ, 6)

/******************************************************************************
 *
 * Music Generator
 *
 * This is a simulation of the following circuit:
 * 555 Timer (Ra = 1k, Rb = 1k, C =.01uF) running at 48kHz.  Connected to a
 * 1 bit counter (/2) for 24kHz.  But I will use the frequency measured by Guru.
 * This is then sent to a preloadable 8 bit counter (4G & 4H), which loads the
 * value from Port 2 when overflowing from 0xFF to 0x00.  Therefore it divides
 * by 2 (Port 2 = FE) to 256 (Port 2 = 00).
 * This goes to a 2 bit counter (5H) which has a 47k resistor on Q0 and a 12k on Q1.
 * This creates a sawtooth ramp of: 0%, 12/59%, 47/59%, 100% then back to 0%
 *
 * Note that there is no music disable line.  When there is no music, the game
 * sets the oscillator to 0Hz.  (Port 2 = FF)
 *
 ******************************************************************************/
	DISCRETE_NOTE(NODE_30, 1, 23396, POLARIS_MUSIC_DATA, 255, 3, DISC_CLK_IS_FREQ)
	DISCRETE_DAC_R1(NODE_31, 1, NODE_30, DEFAULT_TTL_V_LOGIC_1, &polaris_music_dac)
	DISCRETE_OP_AMP_FILTER(NODE_32, 1, NODE_31, 0, DISC_OP_AMP_FILTER_IS_HIGH_PASS_0 | DISC_OP_AMP_IS_NORTON, &polaris_music_op_amp_filt_info)
	DISCRETE_MULTIPLY(POLARIS_MUSIC, 1, NODE_32, POLARIS_ADJ_VR3)

/******************************************************************************
 *
 * Background Sonar Sound
 *
 * This is a background sonar that plays at all times during the game.
 * It is a VCO triangle wave genarator, that uses the Low frequency filtered
 * noise source to modulate the frequency.
 * This is then amplitude modulated, by some fancy clocking scheme.
 * It is disabled during SX3.  (No sonar when you die.)
 *
 * 5L pin 6 divides 60Hz by 16.  This clocks the sonar.
 * 5K pin 9 is inverted by 5F, and then the 0.1uF;1M;270k;1S1588 diode circuit
 * makes a one shot pulse of approx. 15ms to keep the noise working.
 *
 ******************************************************************************/
	DISCRETE_SQUAREWFIX(NODE_40, 1, 60.0/16, 1, 50, 1.0/2, 0)	// IC 5L, pin 6
	DISCRETE_COUNTER(NODE_41, 1, 0, NODE_40, 31, 1, 0, DISC_CLK_ON_F_EDGE)	// IC 5L & 5F
	DISCRETE_TRANSFORM2(NODE_42, 1, NODE_41, 4, "01&")			// IC 5L, pin 9
	DISCRETE_TRANSFORM2(NODE_43, 1, NODE_41, 16, "01&!")		// IC 5F, pin 8
	DISCRETE_ONESHOT(NODE_44, NODE_43, 1, TIME_IN_MSEC(15), DISC_ONESHOT_REDGE | DISC_ONESHOT_NORETRIG | DISC_OUT_ACTIVE_HIGH)

	DISCRETE_LOGIC_OR(NODE_45, 1, NODE_42, POLARIS_SX3_EN)
	DISCRETE_LOGIC_DFLIPFLOP(NODE_46, 1, 1, 1, NODE_40, NODE_45)

	DISCRETE_OP_AMP_VCO1(NODE_47, 1, POLARIS_NOISE_LO_FILT, &polaris_sonar_vco_info)
	DISCRETE_OP_AMP_TRIG_VCA(POLARIS_SONARSND, NODE_45, NODE_46, 0, NODE_47, 0, &polaris_sonar_tvca_info)

/******************************************************************************
 *
 * Noise sources
 *
 * The frequencies for the noise sources were Measured by Guru.
 *
 * The output of the shift register is buffered by an op-amp which limits
 * the output to 0V and (12V - OP_AMP_NORTON_VBE)
 *
 ******************************************************************************/
	DISCRETE_LFSR_NOISE(POLARIS_NOISE_LO, 1, 1, 800.8, (12.0 - OP_AMP_NORTON_VBE), NODE_44, (12.0 - OP_AMP_NORTON_VBE)/2, &polaris_lfsr)  // Unfiltered Lo noise. 7K pin 4.
	// Filtered Lo noise.  7K pin 5.
	DISCRETE_OP_AMP_FILTER(POLARIS_NOISE_LO_FILT, 1, POLARIS_NOISE_LO, 0, DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON, &polaris_nol_op_amp_filt_info)

	DISCRETE_LFSR_NOISE(NODE_50, 1, 1, 23396, (12.0 - OP_AMP_NORTON_VBE), NODE_44, (12.0 - OP_AMP_NORTON_VBE)/2, &polaris_lfsr)	// 7K pin 10
	// Filtered Hi noise.  6B pin 10. - This does not really do much.  Sample rates of 98k+ are needed for this high of filtering.
	DISCRETE_OP_AMP_FILTER(POLARIS_NOISE_HI_FILT, 1, NODE_50, 0, DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON, &polaris_noh_op_amp_filt_info)

/******************************************************************************
 *
 * Shot - SX0 (When player shoots)
 *
 * When Enabled it makes a low frequency RC filtered noise.  As soon as it
 * disables, it switches to a high frequency RC filtered noise with the volume
 * decaying based on the RC values of 680k and 1uF.
 *
 ******************************************************************************/
	DISCRETE_OP_AMP_TRIG_VCA(POLARIS_SHOTSND, POLARIS_SX0_EN, 0, 0, POLARIS_NOISE_HI_FILT, POLARIS_NOISE_LO_FILT, &polaris_shot_tvca_info)

/******************************************************************************
 *
 * Ship Hit - SX1 (When sub is hit)
 *
 * When Enabled it makes a low frequency RC filtered noise.  As soon as it
 * disables, it's  volume starts decaying based on the RC values of 680k and
 * 1uF.  Also as it decays, a decaying high frequency RC filtered noise is
 * mixed in.
 *
 ******************************************************************************/
	DISCRETE_OP_AMP_TRIG_VCA(POLARIS_SHIP_HITSND, POLARIS_SX1_EN, 0, 0, POLARIS_NOISE_HI_FILT, POLARIS_NOISE_LO_FILT, &polaris_shiphit_tvca_info)

/******************************************************************************
 *
 * Ship - SX2 (When boat moves across screen)
 *
 * This uses a 5.75Hz |\|\ sawtooth to modulate the frequency of a
 * voltage controlled triangle wave oscillator. *
 *
 ******************************************************************************/
	DISCRETE_OP_AMP_OSCILLATOR(NODE_60, 1, &polaris_boat_mod_info)
	DISCRETE_OP_AMP_VCO1(POLARIS_SHIPSND, POLARIS_SX2_EN, NODE_60, &polaris_boat_vco_info)

/******************************************************************************
 *
 * Explosion - SX3 (When player, boat or boss plane is hit)
 *
 * When Enabled it makes a low frequency noise.  As soon as it disables, it's
 * volume starts decaying based on the RC values of 680k and 0.33uF.  The
 * final output is RC filtered.
 *
 * Note that when this is triggered, the sonar sound clock is disabled.
 *
 ******************************************************************************/
	DISCRETE_OP_AMP_TRIG_VCA(NODE_70, POLARIS_SX3_EN, 0, 0, POLARIS_NOISE_LO, 0, &polaris_exp_tvca_info)

	DISCRETE_RCFILTER(NODE_71, 1, NODE_70, 560.0, CAP_U(.22))
	DISCRETE_RCFILTER(POLARIS_EXPLOSIONSND, 1, NODE_71, RES_K(6.8), CAP_U(.22))

/******************************************************************************
 *
 * Plane Down - SX6
 * Plane Up   - SX7
 *
 * The oscillator is enabled when SX7 goes high. When SX6 is enabled the
 * frequency lowers.  When SX6 is disabled the frequency ramps back up.
 * Also some NOISE_HI_FILT is mixed in so the frequency varies some.
 *
 ******************************************************************************/
	DISCRETE_INTEGRATE(NODE_80, POLARIS_SX6_EN, POLARIS_SX7_EN, &polaris_plane_integrate_info)
	DISCRETE_OP_AMP_VCO2(POLARIS_PLANESND, POLARIS_SX7_EN, NODE_80, POLARIS_NOISE_HI_FILT, &polaris_plane_vco_info)

/******************************************************************************
 *
 * HIT - SX9 & SX10
 *
 * Following the schematic, 3 different sounds are produced.
 * SX10  SX9  Effect
 *  0     0   no sound
 *  0     1   NOISE_HI_FILT while enabled
 *  1     0   NOISE_LO_FILT while enabled (When a regular plane is hit)
 *  1     1   NOISE_HI_FILT & NOISE_LO_FILT decaying imediately @ 680k, 0.22uF
 *
 ******************************************************************************/
	DISCRETE_OP_AMP_TRIG_VCA(POLARIS_HITSND, POLARIS_SX10_EN, POLARIS_SX9_EN, 0, POLARIS_NOISE_LO_FILT, POLARIS_NOISE_HI_FILT, &polaris_hit_tvca_info)

/******************************************************************************
 *
 * Final Mixing and Output
 *
 ******************************************************************************/
	DISCRETE_MIXER4(NODE_90, 1, POLARIS_SHOTSND, POLARIS_SONARSND, POLARIS_HITSND, POLARIS_PLANESND, &polaris_mixer_vr1_desc)
	DISCRETE_MULTIPLY(NODE_91, 1, NODE_90, POLARIS_ADJ_VR1)
	DISCRETE_MIXER2(NODE_92, 1, POLARIS_SHIPSND, POLARIS_SHIP_HITSND, &polaris_mixer_vr2_desc)
	DISCRETE_MULTIPLY(NODE_93, 1, NODE_92, POLARIS_ADJ_VR2)
	DISCRETE_MIXER4(NODE_94, POLARIS_SX5_EN, NODE_91, POLARIS_EXPLOSIONSND, NODE_93, POLARIS_MUSIC, &polaris_mixer_vr4_desc)

	DISCRETE_OUTPUT(NODE_94, 1)

DISCRETE_SOUND_END

MACHINE_RESET( polaris )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x02, 0x02, 0, 0, polaris_sh_port2_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x04, 0x04, 0, 0, polaris_sh_port4_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x06, 0x06, 0, 0, polaris_sh_port6_w);
	// Port 5 is used to reset the watchdog timer.
	// This port is also written to when the boss plane is going up and down.
	// If you write this value to a note ciruit similar to the music,
	// you will get a nice sound that accurately follows the plane.
	// It sounds better then the actual circuit used.
	// Probably an unfinished feature.
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, watchdog_reset_w);
}

static WRITE8_HANDLER( polaris_sh_port2_w )
{
	discrete_sound_w(POLARIS_MUSIC_DATA, data);
}

static WRITE8_HANDLER( polaris_sh_port4_w )
{
	/* 0x01 - SX0 - Shot */
	discrete_sound_w(POLARIS_SX0_EN, data & 0x01);

	/* 0x02 - SX1 - Ship Hit (Sub) */
	discrete_sound_w(POLARIS_SX1_EN, data & 0x02);

	/* 0x04 - SX2 - Ship */
	discrete_sound_w(POLARIS_SX2_EN, data & 0x04);

	/* 0x08 - SX3 - Explosion */
	discrete_sound_w(POLARIS_SX3_EN, data & 0x08);

	/* 0x10 - SX4 */

	/* 0x20 - SX5 - Sound Enable */
	discrete_sound_w(POLARIS_SX5_EN, data & 0x20);
}

static WRITE8_HANDLER( polaris_sh_port6_w )
{
	coin_lockout_global_w(data & 0x04);  /* SX8 */

	c8080bw_flip_screen_w(data & 0x20);  /* SX11 */

	/* 0x01 - SX6 - Plane Down */
	discrete_sound_w(POLARIS_SX6_EN, data & 0x01);

	/* 0x02 - SX7 - Plane Up */
	discrete_sound_w(POLARIS_SX7_EN, data & 0x02);

	/* 0x08 - SX9 - Hit */
	discrete_sound_w(POLARIS_SX9_EN, data & 0x08);

	/* 0x10 - SX10 - Hit */
	discrete_sound_w(POLARIS_SX10_EN, data & 0x10);
}


/*******************************************************/
/*                                                     */
/* Midway "Phantom II"                                 */
/*                                                     */
/*******************************************************/

MACHINE_RESET( phantom2 )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x04, 0x04, 0, 0, watchdog_reset_w);
}


/*******************************************************/
/*                                                     */
/* Midway "4 Player Bowling Alley"                     */
/*                                                     */
/*******************************************************/

MACHINE_RESET( bowler )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x04, 0x04, 0, 0, watchdog_reset_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x07, 0x07, 0, 0, bowler_bonus_display_w);
}


/*******************************************************/
/*                                                     */
/* Midway "Sea Wolf"                                   */
/*                                                     */
/*******************************************************/

static const char *seawolf_sample_names[] =
{
	"*seawolf",
	"shiphit.wav",
	"torpedo.wav",
	"dive.wav",
	"sonar.wav",
	"minehit.wav",
	0       /* end of array */
};

struct Samplesinterface seawolf_samples_interface =
{
	5,	/* 5 channels */
	seawolf_sample_names
};

MACHINE_RESET( seawolf )
{
/*  Lamp Display Output (write) Ports are as follows:

Port 1:
  Basically D0-D3 are column drivers and D4-D7 are row drivers.
  The folowing table shows values that light up individual lamps.

    D7 D6 D5 D4 D3 D2 D1 D0   Function
    --------------------------------------------------------------------------------------
     0  0  0  1  1  0  0  0   Explosion Lamp 0
     0  0  0  1  0  1  0  0   Explosion Lamp 1
     0  0  0  1  0  0  1  0   Explosion Lamp 2
     0  0  0  1  0  0  0  1   Explosion Lamp 3
     0  0  1  0  1  0  0  0   Explosion Lamp 4
     0  0  1  0  0  1  0  0   Explosion Lamp 5
     0  0  1  0  0  0  1  0   Explosion Lamp 6
     0  0  1  0  0  0  0  1   Explosion Lamp 7
     0  1  0  0  1  0  0  0   Explosion Lamp 8
     0  1  0  0  0  1  0  0   Explosion Lamp 9
     0  1  0  0  0  0  1  0   Explosion Lamp A
     0  1  0  0  0  0  0  1   Explosion Lamp B
     1  0  0  0  1  0  0  0   Explosion Lamp C
     1  0  0  0  0  1  0  0   Explosion Lamp D
     1  0  0  0  0  0  1  0   Explosion Lamp E
     1  0  0  0  0  0  0  1   Explosion Lamp F

Port 2:
    D7 D6 D5 D4 D3 D2 D1 D0   Function
    --------------------------------------------------------------------------------------
     x  x  x  x  x  x  x  1   Torpedo 1
     x  x  x  x  x  x  1  x   Torpedo 2
     x  x  x  x  x  1  x  x   Torpedo 3
     x  x  x  x  1  x  x  x   Torpedo 4
     x  x  x  1  x  x  x  x   Ready
     x  x  1  x  x  x  x  x   Reload

*/

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x01, 0x01, 0, 0, seawolf_port_1_r);

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, seawolf_sh_port5_w);

}

static WRITE8_HANDLER( seawolf_sh_port5_w )
{
	if (data & 0x01)
		sample_start (0, 0, 0);  /* Ship Hit */
	if (data & 0x02)
		sample_start (1, 1, 0);  /* Torpedo */
	if (data & 0x04)
		sample_start (2, 2, 0);  /* Dive */
	if (data & 0x08)
		sample_start (3, 3, 0);  /* Sonar */
	if (data & 0x10)
		sample_start (4, 4, 0);  /* Mine Hit */

	coin_counter_w(0, (data & 0x20) >> 5);    /* Coin Counter */
}


/*******************************************************/
/*                                                     */
/* Midway "Desert Gun"                                 */
/*                                                     */
/*******************************************************/

MACHINE_RESET( desertgu )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x01, 0x01, 0, 0, desertgu_port_1_r);

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x07, 0x07, 0, 0, desertgu_controller_select_w);
}


/*******************************************************/
/*                                                     */
/* Taito "Space Chaser"                                */
/*                                                     */
/* The SN76477 still needs to be routed to the         */
/* discrete system for filtering.                      */
/*******************************************************/

/*
 *  The dot sound is a square wave clocked by either the
 *  the 8V or 4V signals
 *
 *  The frequencies are (for the 8V signal):
 *
 *  19.968 MHz crystal / 2 (Qa of 74160 #10) -> 9.984MHz
 *                     / 2 (7474 #14) -> 4.992MHz
 *                     / 256+16 (74161 #5 and #8) -> 18352.94Hz
 *                     / 8 (8V) -> 2294.12 Hz
 *                     / 2 the final freq. is 2 toggles -> 1147.06Hz
 *
 *  for 4V, it's double at 2294.12Hz
 */
#define SCHASER_HSYNC	18352.94
#define SCHASER_4V		SCHASER_HSYNC /2 /4
#define SCHASER_8V		SCHASER_HSYNC /2 /8

struct SN76477interface schaser_sn76477_interface =
{
	RES_K( 47)	,		/*  4  noise_res         */
	RES_K(330)	,		/*  5  filter_res        */
	CAP_P(470)	,		/*  6  filter_cap        */
	RES_M(2.2)	,		/*  7  decay_res         */
	CAP_U(1.0)	,		/*  8  attack_decay_cap  */
	RES_K(4.7)	,		/* 10  attack_res        */
	0			,		/* 11  amplitude_res (variable)  */
	RES_K(33)	,		/* 12  feedback_res      */
	0			,		/* 16  vco_voltage       */
	CAP_U(0.1)	,		/* 17  vco_cap           */
	RES_K(39)	,		/* 18  vco_res           */
	5.0			,		/* 19  pitch_voltage     */
	RES_K(120)	,		/* 20  slf_res           */
	CAP_U(1.0)	,		/* 21  slf_cap           */
	CAP_U(0.1)	,		/* 23  oneshot_cap       */
	RES_K(220)   		/* 24  oneshot_res       */
};

/* Nodes - Inputs */
#define SCHASER_DOT_EN		NODE_01
#define SCHASER_DOT_SEL		NODE_02
#define SCHASER_EXP_STREAM	NODE_03
#define SCHASER_MUSIC_BIT	NODE_04
#define SCHASER_SND_EN		NODE_05
/* Nodes - Adjusters */
#define SCHASER_VR1			NODE_07
#define SCHASER_VR2			NODE_08
#define SCHASER_VR3			NODE_09
/* Nodes - Sounds */
#define SCHASER_DOT_SND		NODE_10
#define SCHASER_EXP_SND		NODE_11
#define SCHASER_MUSIC_SND	NODE_12

DISCRETE_SOUND_START(schaser_discrete_interface)
	/************************************************/
	/* Input register mapping for schaser           */
	/************************************************/
	DISCRETE_INPUT_LOGIC  (SCHASER_DOT_EN)
	DISCRETE_INPUT_LOGIC  (SCHASER_DOT_SEL)
//  Change the constant to the stream input when working.
	DISCRETE_CONSTANT(SCHASER_EXP_STREAM, 0)
//  DISCRETE_INPUTX_STREAM(SCHASER_EXP_STREAM,   5.0/36764,              0)
	DISCRETE_INPUTX_LOGIC (SCHASER_MUSIC_BIT,    DEFAULT_TTL_V_LOGIC_1,  0,      0.0)
	DISCRETE_INPUT_LOGIC  (SCHASER_SND_EN)

	/************************************************/
	/* Volume adjusters.                            */
	/* We will set them to adjust the realitive     */
	/* gains.                                       */
	/************************************************/
	DISCRETE_ADJUSTMENT(SCHASER_VR1, 1, 0, RES_K(50)/(RES_K(50) + RES_K(470)), DISC_LINADJ, 4)
	DISCRETE_ADJUSTMENT(SCHASER_VR2, 1, 0, RES_K(50)/(RES_K(50) + 560 + RES_K(6.8) + RES_K(2)), DISC_LINADJ, 5)
	DISCRETE_ADJUSTMENT(SCHASER_VR3, 1, 0, RES_K(50)/(RES_K(50) + 560 + RES_K(6.8) + RES_K(10)), DISC_LINADJ, 6)

	/************************************************/
	/* Dot selection just selects between 4V and 8V */
	/************************************************/
	DISCRETE_SQUAREWFIX(NODE_20, 1, SCHASER_4V, DEFAULT_TTL_V_LOGIC_1, 50, 0, 0)
	DISCRETE_SQUAREWFIX(NODE_21, 1, SCHASER_8V, DEFAULT_TTL_V_LOGIC_1, 50, 0, 0)
	DISCRETE_SWITCH(NODE_22, SCHASER_DOT_EN, SCHASER_DOT_SEL, NODE_20, NODE_21)
	DISCRETE_RCFILTER(NODE_23, 1, NODE_22, 560, CAP_U(.1))
	DISCRETE_RCFILTER(NODE_24, 1, NODE_23, RES_K(6.8) + 560, CAP_U(.1))
	DISCRETE_MULTIPLY(SCHASER_DOT_SND, 1, NODE_24, SCHASER_VR3)

	/************************************************/
	/* Explosion/Effect filtering                   */
	/************************************************/
	DISCRETE_RCFILTER(NODE_30, 1, SCHASER_EXP_STREAM, 560, CAP_U(.1))
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(6.8) + 560, CAP_U(.1))
	DISCRETE_CRFILTER(NODE_32, 1, NODE_31, RES_K(6.8) + 560 + RES_K(2) + RES_K(50), CAP_U(1))
	DISCRETE_MULTIPLY(SCHASER_EXP_SND, 1, NODE_32, SCHASER_VR2)

	/************************************************/
	/* Music is just a 1 bit DAC                    */
	/************************************************/
	DISCRETE_CRFILTER(NODE_40, 1, SCHASER_MUSIC_BIT, RES_K(470) + RES_K(50), CAP_U(.01))
	DISCRETE_MULTIPLY(SCHASER_MUSIC_SND, 1, NODE_40, SCHASER_VR1)

	/************************************************/
	/* Final mix with gain                          */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, SCHASER_SND_EN, SCHASER_DOT_SND, SCHASER_EXP_SND, SCHASER_MUSIC_SND)

	DISCRETE_OUTPUT(NODE_90, 60000)
DISCRETE_SOUND_END

static double schaser_effect_rc[8] =
{
	0,
	(RES_K(82) + RES_K(20)) * CAP_U(1),
	(RES_K(39) + RES_K(20)) * CAP_U(1),
	(1.0/ (1.0/RES_K(82) + 1.0/RES_K(39)) + RES_K(20)) * CAP_U(1),
	(RES_K(15) + RES_K(20)) * CAP_U(1),
	(1.0/ (1.0/RES_K(82) + 1.0/RES_K(15)) + RES_K(20)) * CAP_U(1),
	(1.0/ (1.0/RES_K(39) + 1.0/RES_K(15)) + RES_K(20)) * CAP_U(1),
	(1.0/ (1.0/RES_K(82) + 1.0/RES_K(39) + 1.0/RES_K(15)) + RES_K(20)) * CAP_U(1)
};

static WRITE8_HANDLER( schaser_sh_port3_w )
{
	static int last_effect = 0;
	int effect;

	/* bit 0 - Dot Sound Enable (SX0)
       bit 1 - Dot Sound Pitch (SX1)
       bit 2 - Effect Sound A (SX2)
       bit 3 - Effect Sound B (SX3)
       bit 4 - Effect Sound C (SX4)
       bit 5 - Explosion (SX5) */

	discrete_sound_w(SCHASER_DOT_EN, data & 0x01);
	discrete_sound_w(SCHASER_DOT_SEL, data & 0x02);

	/* The effect is a variable rate 555 timer.  A diode/resistor array is used to
     * select the frequency.  Because of the diode voltage drop, we can not use the
     * standard 555 time formulas.  Also, when effect=0, the charge resistor
     * is disconnected.  This causes the charge on the cap to slowly bleed off, but
     * but the bleed time is so long, that we can just cheat and put the time on hold
     * when effect = 0. */
	effect = (data >> 2) & 0x07;
	if (last_effect != effect)
	{
		if (effect)
		{
			if (schaser_effect_555_time_remain != 0)
			{
				/* timer re-enabled, use up remaining 555 high time */
				timer_adjust(schaser_effect_555_timer, schaser_effect_555_time_remain, effect, 0);
			}
			else if (!schaser_effect_555_is_low)
			{
				/* set 555 high time */
				timer_adjust(schaser_effect_555_timer, 0.8873 * schaser_effect_rc[effect], effect, 0);
			}
		}
		else
		{
			/* disable effect - stops at end of low cycle */
			if (!schaser_effect_555_is_low)
			{
				schaser_effect_555_time_remain = timer_timeleft(schaser_effect_555_timer);
				timer_adjust(schaser_effect_555_timer, TIME_NEVER, 0, 0);
			}
		}
		last_effect = effect;
	}

	explosion = (data >> 5) & 0x01;
	if (explosion)
	{
		SN76477_set_amplitude_res(0, 1.0 / (1.0/RES_K(200) + 1.0/RES_K(68)));
	}
	else
	{
		SN76477_set_amplitude_res(0, RES_K(200));
	}
	SN76477_enable_w(0, !(schaser_effect_555_is_low || explosion));
	SN76477_mixer_b_w(0, explosion);
}

static WRITE8_HANDLER( schaser_sh_port5_w )
{
	/* bit 0 - Music (DAC) (SX6)
       bit 1 - Sound Enable (SX7)
       bit 2 - Coin Lockout (SX8)
       bit 3 - Field Control A (SX9)
       bit 4 - Field Control B (SX10)
       bit 5 - Flip Screen */

	discrete_sound_w(SCHASER_MUSIC_BIT, data & 0x01);

	discrete_sound_w(SCHASER_SND_EN, data & 0x02);
	sound_global_enable(data & 0x02);

	coin_lockout_global_w(data & 0x04);

	c8080bw_flip_screen_w(data & 0x20);
}

void schaser_effect_555_cb(int effect)
{
	double	new_time;
	/* Toggle 555 output */
	schaser_effect_555_is_low = !schaser_effect_555_is_low;
	schaser_effect_555_time_remain = 0;

	if (schaser_effect_555_is_low)
		new_time = 0.6931 * RES_K(20) * CAP_U(1);
	else
	{
		if (effect)
			new_time = 0.8873 * schaser_effect_rc[effect];
		else
			new_time = TIME_NEVER;
	}
	timer_adjust(schaser_effect_555_timer, new_time, effect, 0);
	SN76477_enable_w(0, !(schaser_effect_555_is_low || explosion));
}

MACHINE_RESET( schaser )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x03, 0x03, 0, 0, schaser_sh_port3_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, schaser_sh_port5_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x06, 0x06, 0, 0, watchdog_reset_w);

	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_c_w(0, 0);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_vco_w(0, 1);

	schaser_effect_555_is_low = 0;
	timer_adjust(schaser_effect_555_timer, TIME_NEVER, 0, 0);
	schaser_sh_port3_w(0, 0);
	schaser_sh_port5_w(0, 0);
	schaser_effect_555_time_remain = 0;
}


/*******************************************************/
/*                                                     */
/* Midway "Clowns"                                     */
/*                                                     */
/* Mar 2005, D.R.                                      */
/*******************************************************/

/* Nodes - Inputs */
#define CLOWNS_POP_B_EN			NODE_01
#define CLOWNS_POP_M_EN			NODE_02
#define CLOWNS_POP_T_EN			NODE_03
#define CLOWNS_MUSIC_EN			NODE_04
#define CLOWNS_MUSIC_DATA_L		NODE_05
#define CLOWNS_MUSIC_DATA_H		NODE_06
#define CLOWNS_SB_HIT_EN		NODE_07
/* Nodes - Sounds */
#define CLOWNS_NOISE			NODE_11
#define CLOWNS_POP_SND			NODE_12
#define CLOWNS_MUSIC_SND		NODE_13
#define CLOWNS_SB_HIT_SND		NODE_14

const struct discrete_lfsr_desc clowns_lfsr={
	DISC_CLK_IS_FREQ,
	17,			/* Bit Length */
	/* C101, R104, D100, have the effect of presetting all bits high at power up */
	0x1ffff,	/* Reset Value */
	4,			/* Use Bit 4 as XOR input 0 */
	16,			/* Use Bit 16 as XOR input 1 */
	DISC_LFSR_XOR,		/* Feedback stage1 is XOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is not inverted */
	12			/* Output bit */
};

const struct discrete_op_amp_tvca_info clowns_music_tvca_info =
{
	RES_M(3.3),				// r502
	RES_K(10) + RES_K(680),	// r505 + r506
	0,
	RES_K(680),				// r503
	RES_K(10),				// r500
	0,
	RES_K(680),				// r501
	0, 0, 0, 0,
	CAP_U(.001),			// c500
	0, 0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_TRG1, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

const struct discrete_op_amp_tvca_info clowns_pop_tvca_info =
{
	RES_M(2.7),		// r304
	RES_K(680),		// r303
	0,
	RES_K(680),		// r305
	RES_K(1),		// j3
	0,
	RES_K(470),		// r300
	RES_K(1),		// j3
	RES_K(510),		// r301
	RES_K(1),		// j3
	RES_K(680),		// r302
	CAP_U(.015),	// c300
	CAP_U(.1),		// c301
	CAP_U(.082),	// c302
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_TRG1, DISC_OP_AMP_TRIGGER_FUNCTION_TRG2
};

const struct discrete_op_amp_osc_info clowns_sb_hit_osc_info =
{
	DISC_OP_AMP_OSCILLATOR_1 | DISC_OP_AMP_IS_NORTON,
	RES_K(820),		// r200
	RES_K(33),		// r203
	RES_K(150),		// r201
	RES_K(240),		// r204
	RES_M(1),		// r202
	0,0,0,
	CAP_U(0.01),	// c200
	12
};

const struct discrete_op_amp_tvca_info clowns_sb_hit_tvca_info =
{
	RES_M(2.7),		// r207
	RES_K(680),		// r205
	0,
	RES_K(680),		// r208
	RES_K(1),		// j3
	0,
	RES_K(680),		// r206
	0,0,0,0,
	CAP_U(1),		// c201
	0,0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_TRG0, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE, DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};

DISCRETE_SOUND_START(clowns_discrete_interface)
	/************************************************/
	/* Input register mapping for clowns            */
	/************************************************/
	DISCRETE_INPUT_LOGIC(CLOWNS_POP_B_EN)
	DISCRETE_INPUT_LOGIC(CLOWNS_POP_M_EN)
	DISCRETE_INPUT_LOGIC(CLOWNS_POP_T_EN)
	DISCRETE_INPUT_LOGIC(CLOWNS_MUSIC_EN)
	DISCRETE_INPUT_DATA (CLOWNS_MUSIC_DATA_L)
	DISCRETE_INPUT_DATA (CLOWNS_MUSIC_DATA_H)
	DISCRETE_INPUT_LOGIC(CLOWNS_SB_HIT_EN)

	/************************************************/
	/* Music Generator                              */
	/************************************************/
	DISCRETE_MULTADD(NODE_20, 1, CLOWNS_MUSIC_DATA_H, 0x40, CLOWNS_MUSIC_DATA_L)
	DISCRETE_NOTE(NODE_21, 1, 19968000.0/10/2, NODE_20, 0xfff, 1, DISC_CLK_IS_FREQ)
	DISCRETE_OP_AMP_TRIG_VCA(NODE_22, NODE_21, CLOWNS_MUSIC_EN, 0, 12, 0, &clowns_music_tvca_info)
	DISCRETE_GAIN(CLOWNS_MUSIC_SND, NODE_22, .6)

	/************************************************/
	/* Balloon hit sounds                           */
	/* Noise freq is a guess based on the fact that */
	/* the circuit is the same as polaris but the   */
	/* cap is .001 instead of .01.                  */
	/************************************************/
	DISCRETE_LFSR_NOISE(CLOWNS_NOISE, 1, 1, 8000, 12.0, 0, 12.0/2, &clowns_lfsr)
	DISCRETE_OP_AMP_TRIG_VCA(NODE_30, CLOWNS_POP_T_EN, CLOWNS_POP_M_EN, CLOWNS_POP_B_EN, CLOWNS_NOISE, 0, &clowns_pop_tvca_info)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(15), CAP_U(.01))
	DISCRETE_CRFILTER(NODE_32, 1, NODE_31, RES_K(15) + RES_K(39), CAP_U(.01))
	DISCRETE_GAIN(CLOWNS_POP_SND, NODE_32, RES_K(39)/(RES_K(15) + RES_K(39)))

	/************************************************/
	/* Springboard Hit                              */
	/************************************************/
	DISCRETE_OP_AMP_OSCILLATOR(NODE_40, 1, &clowns_sb_hit_osc_info)
	DISCRETE_OP_AMP_TRIG_VCA(NODE_41, CLOWNS_SB_HIT_EN, 0, 0, NODE_40, 0, &clowns_sb_hit_tvca_info)
	/* The rest of the circuit is a filter.  The frequency response was calculated with SPICE. */
	DISCRETE_FILTER2(NODE_42, 1, NODE_41, 500, 1.0/.8, DISC_FILTER_LOWPASS)
	/* The filter has a gain of 0.5 */
	DISCRETE_GAIN(CLOWNS_SB_HIT_SND, NODE_42, .5)

	/************************************************/
	/* Combine all sound sources.                   */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, CLOWNS_MUSIC_SND, CLOWNS_POP_SND, CLOWNS_SB_HIT_SND)
	DISCRETE_CRFILTER(NODE_91, 1, NODE_90, RES_K(100), CAP_U(.1))

	DISCRETE_OUTPUT(NODE_91, 5000)
DISCRETE_SOUND_END

static WRITE8_HANDLER( clowns_sh_port3_w )
{
	coin_counter_w(0, data & 0x01);
}

static WRITE8_HANDLER( clowns_sh_port5_w )
{
	discrete_sound_w(CLOWNS_MUSIC_EN, data & 0x01);
	/* bit 0 of music data is always 0 */
	discrete_sound_w(CLOWNS_MUSIC_DATA_L, data & 0x3e);
}

static WRITE8_HANDLER( clowns_sh_port6_w )
{
	discrete_sound_w(CLOWNS_MUSIC_DATA_H, data & 0x3f);
}

static WRITE8_HANDLER( clowns_sh_port7_w )
{
	/* Bottom Balloon Pop */
	discrete_sound_w(CLOWNS_POP_B_EN, data & 0x01);

	/* Middle Balloon Pop */
	discrete_sound_w(CLOWNS_POP_M_EN, data & 0x02);

	/* Top Balloon Pop */
	discrete_sound_w(CLOWNS_POP_T_EN, data & 0x04);

	sound_global_enable(data & 0x08);

	/* Springboard hit */
	discrete_sound_w(CLOWNS_SB_HIT_EN, data & 0x10);

	if (data & 0x20)
		sample_start (1, 1, 0);  /* Springboard miss */
}

MACHINE_RESET( clowns )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x03, 0x03, 0, 0, clowns_sh_port3_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x04, 0x04, 0, 0, watchdog_reset_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, clowns_sh_port5_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x06, 0x06, 0, 0, clowns_sh_port6_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x07, 0x07, 0, 0, clowns_sh_port7_w);
}

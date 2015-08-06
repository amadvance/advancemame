/*************************************************************************

    Sega Z80-3D system

*************************************************************************/

#include "driver.h"
#include "turbo.h"
#include "sound/samples.h"


#define DISCRETE_TEST (0)

/* local data */
static UINT8 osel, bsel;

static UINT8 turbo_accel;

static UINT8 buckrog_hit;
static UINT8 buckrog_myship;

static UINT8 subroc3d_volume;
static UINT8 subroc3d_select;


#if (DISCRETE_TEST)

/* Nodes - Inputs */
#define TURBO_CRASH_EN			NODE_01
#define TURBO_TRIG1_INV			NODE_02
#define TURBO_TRIG2_INV			NODE_03
#define TURBO_TRIG3_INV			NODE_04
#define TURBO_TRIG4_INV			NODE_05
#define TURBO_SLIP_EN			NODE_06
#define TURBO_CRASHL_EN			NODE_07
#define TURBO_ACC_VAL			NODE_08
#define TURBO_AMBU_EN			NODE_09
#define TURBO_SPIN_EN			NODE_10
#define TURBO_OSEL_VAL			NODE_11
#define TURBO_BSEL_VAL			NODE_12

/* Nodes - Sounds */
#define FIRETRUCK_NOISE			NODE_20

const struct discrete_555_desc turbo_alarm_555 =
{
	DISC_555_OUT_SQW | DISC_555_OUT_DC,
	5,				// B+ voltage of 555
	DEFAULT_555_VALUES,
};

DISCRETE_SOUND_START(turbo_sound_interface)
	/************************************************/
	/* Input register mapping for turbo             */
	/************************************************/
	/*                  NODE             ADDR  MASK    GAIN    OFFSET  INIT */
	DISCRETE_INPUT(TURBO_CRASH_EN		,0x00,0x001f,                  0.0)
	DISCRETE_INPUT(TURBO_TRIG1_INV    	,0x01,0x001f,                  1.0)
	DISCRETE_INPUT(TURBO_TRIG2_INV 		,0x02,0x001f,                  1.0)
	DISCRETE_INPUT(TURBO_TRIG3_INV 		,0x03,0x001f,                  1.0)
	DISCRETE_INPUT(TURBO_TRIG4_INV    	,0x04,0x001f,                  1.0)
	DISCRETE_INPUT(TURBO_SLIP_EN    	,0x05,0x001f,                  0.0)
	DISCRETE_INPUT(TURBO_CRASHL_EN		,0x06,0x001f,                  0.0)
	DISCRETE_INPUT(TURBO_ACC_VAL 		,0x07,0x001f,                  0.0)
	DISCRETE_INPUT(TURBO_AMBU_EN 		,0x08,0x001f,                  0.0)
	DISCRETE_INPUT(TURBO_SPIN_EN 		,0x09,0x001f,                  0.0)
	DISCRETE_INPUT(TURBO_OSEL_VAL 		,0x0a,0x001f,                  0.0)
	DISCRETE_INPUT(TURBO_BSEL_VAL 		,0x0b,0x001f,                  0.0)

	/************************************************/
	/* Alarm sounds                                 */
	/************************************************/

	// 5-5-5 counter provides the input clock
	DISCRETE_555_ASTABLE(NODE_50,1,470,120,0.1e-6,&turbo_alarm_555)
	// which clocks a 74393 dual 4-bit counter, clocked on the falling edge
	DISCRETE_COUNTER(NODE_51,1,0,NODE_50,15,1,0,DISC_CLK_ON_F_EDGE)
	// the high bit of this counter
	DISCRETE_TRANSFORM2(NODE_52,1,NODE_51,8,"01/")
	// clocks the other half of the 74393
	DISCRETE_COUNTER(NODE_53,1,0,NODE_52,15,1,0,DISC_CLK_ON_F_EDGE)

	// trig1 triggers a LS123 retriggerable multivibrator
	DISCRETE_ONESHOT(NODE_60,TURBO_TRIG1_INV,5.0,(0.33e-9)*47*1e6, DISC_ONESHOT_FEDGE|DISC_ONESHOT_RETRIG|DISC_OUT_ACTIVE_HIGH)
	// which interacts with bit 0 of the second counter
	DISCRETE_TRANSFORM2(NODE_61,1,NODE_53,1,"01&")
	// via a NAND
	DISCRETE_LOGIC_NAND(NODE_62,1,NODE_60,NODE_61)

	// trig2 triggers a LS123 retriggerable multivibrator
	DISCRETE_ONESHOT(NODE_65,TURBO_TRIG2_INV,5.0,(0.33e-9)*47*10e6,DISC_ONESHOT_FEDGE|DISC_ONESHOT_RETRIG|DISC_OUT_ACTIVE_HIGH)
	// which interacts with bit 3 of the first counter via a NAND
	DISCRETE_LOGIC_NAND(NODE_66,1,NODE_65,NODE_52)

	// trig3 triggers a LS123 retriggerable multivibrator
	DISCRETE_ONESHOT(NODE_70,TURBO_TRIG3_INV,5.0,(0.33e-9)*47*33e6,DISC_ONESHOT_FEDGE|DISC_ONESHOT_RETRIG|DISC_OUT_ACTIVE_HIGH)
	// which interacts with bit 2 of the first counter
	DISCRETE_TRANSFORM3(NODE_71,1,NODE_51,4,1,"01/2&")
	// via a NAND
	DISCRETE_LOGIC_NAND(NODE_72,1,NODE_70,NODE_71)

	// trig4 triggers a LS123 retriggerable multivibrator
	DISCRETE_ONESHOT(NODE_75,TURBO_TRIG4_INV,5.0,(0.33e-9)*47*10e6,DISC_ONESHOT_FEDGE|DISC_ONESHOT_RETRIG|DISC_OUT_ACTIVE_HIGH)
	// which interacts with bit 1 of the first counter
	DISCRETE_TRANSFORM3(NODE_76,1,NODE_51,2,1,"01/2&")
	// via a NAND
	DISCRETE_LOGIC_NAND(NODE_77,1,NODE_75,NODE_76)

	// everything is effectively NANDed together
	DISCRETE_LOGIC_NAND4(NODE_80,1,NODE_62,NODE_66,NODE_72,NODE_77)

/*

    the rest of the circuit looks like this:

                      +5V            +12V                                +---+
                       ^              ^   +--------+               1K    v   |
                       |              |   | |\     |           +---NNN--NNN--+
                       Z 1K       10K Z   | | \    |           | |\     20K  |   +--|(----> ALARM_M
                       Z              Z   +-|- \   |           | | \         |   |  4.7u
                       |              |     |   >--+---NNNN----+-|- \        |   +--|(----> ALARM_F
                       +--NNNN--|(----+-----|+ /        22K      |   >-------+---+  4.7u
    +-\                |  5.1K  4.7u  |     | /             +6V--|+ /            +--|(----> ALARM_R
    |  >o---(NODE_62)--+              Z     |/                   | /             |  4.7u
    +-/                |          10K Z                          |/              +--|(----> ALARM_L
                       |              |                                             4.7u
    +-\                |              v
    |  >o---(NODE_66)--+             GND
    +-/                |
                       |
    +-\                |
    |  >o---(NODE_72)--+
    +-/                |
                       |
    +-\                |
    |  >o---(NODE_77)--+
    +-/


*/

	/************************************************/
	/* Combine all 7 sound sources.                 */
	/************************************************/

	DISCRETE_OUTPUT(NODE_80, 16000)
DISCRETE_SOUND_END

#endif



/*******************************************

    Sample handling

*******************************************/

static void turbo_update_samples(void)
{
	/* accelerator sounds */
	/* BSEL == 3 --> off */
	/* BSEL == 2 --> standard */
	/* BSEL == 1 --> tunnel */
	/* BSEL == 0 --> ??? */
	if (bsel == 3 && sample_playing(6))
		sample_stop(6);
	else if (bsel != 3 && !sample_playing(6))
		sample_start(6, 7, 1);
	if (sample_playing(6))
		sample_set_freq(6, 44100 * (turbo_accel & 0x3f) / 5.25 + 44100);
}


static void buckrog_update_samples(void)
{
	/* accelerator sounds -- */
	if (sample_playing(0))
		sample_set_freq(0, 44100 * buckrog_myship / 100.25 + 44100);

	if (sample_playing(1))
		sample_set_freq(1, 44100 * buckrog_hit / 5.25 + 44100);
}



/*******************************************

    Turbo sound handling

*******************************************/

#if (DISCRETE_TEST)
static int last_sound_A;

static void update_sound_A(int data)
{
	/* missing short crash sample, but I've never seen it triggered */
	discrete_sound_w(0, !(data & 0x01));
	discrete_sound_w(1, (data >> 1) & 1);
	discrete_sound_w(2, (data >> 2) & 1);
	discrete_sound_w(3, (data >> 3) & 1);
	discrete_sound_w(4, (data >> 4) & 1);
	discrete_sound_w(5, !(data & 0x20));
	discrete_sound_w(6, !(data & 0x40));

if (!((data >> 1) & 1)) printf("/TRIG1\n");
if (!((data >> 2) & 1)) printf("/TRIG2\n");
if (!((data >> 3) & 1)) printf("/TRIG3\n");
if (!((data >> 4) & 1)) printf("/TRIG4\n");

//  osel = (osel & 6) | ((data >> 5) & 1);
//  turbo_update_samples();
}
#endif


WRITE8_HANDLER( turbo_sound_A_w )
{
	/*
        2PA0 = /CRASH
        2PA1 = /TRIG1
        2PA2 = /TRIG2
        2PA3 = /TRIG3
        2PA4 = /TRIG4
        2PA5 = OSEL0
        2PA6 = /SLIP
        2PA7 = /CRASHL
    */
#if (DISCRETE_TEST)
	if (((data ^ last_sound_A) & 0x1e) && (last_sound_A & 0x1e) != 0x1e)
		timer_set(TIME_IN_HZ(20000), data, update_sound_A);
	else
		update_sound_A(data);

	last_sound_A = data;
#else
	/* missing short crash sample, but I've never seen it triggered */
	if (!(data & 0x02)) sample_start(0, 0, 0);
	if (!(data & 0x04)) sample_start(0, 1, 0);
	if (!(data & 0x08)) sample_start(0, 2, 0);
	if (!(data & 0x10)) sample_start(0, 3, 0);
	if (!(data & 0x40)) sample_start(1, 4, 0);
	if (!(data & 0x80)) sample_start(2, 5, 0);
	osel = (osel & 6) | ((data >> 5) & 1);
	turbo_update_samples();
#endif
}


WRITE8_HANDLER( turbo_sound_B_w )
{
	/*
        2PB0 = ACC0
        2PB1 = ACC1
        2PB2 = ACC2
        2PB3 = ACC3
        2PB4 = ACC4
        2PB5 = ACC5
        2PB6 = /AMBU
        2PB7 = /SPIN
    */
	turbo_accel = data & 0x3f;
	turbo_update_samples();
	if (!(data & 0x40))
	{
		if (!sample_playing(7))
			sample_start(7, 8, 0);
		else
			logerror("ambu didnt start\n");
	}
	else
		sample_stop(7);
	if (!(data & 0x80)) sample_start(3, 6, 0);
}


WRITE8_HANDLER( turbo_sound_C_w )
{
	/*
        2PC0 = OSEL1
        2PC1 = OSEL2
        2PC2 = BSEL0
        2PC3 = BSEL1
        2PC4 = SPEED0
        2PC5 = SPEED1
        2PC6 = SPEED2
        2PC7 = SPEED3
    */
	turbo_speed = (data >> 4) & 0x0f;
	bsel = (data >> 2) & 3;
	osel = (osel & 1) | ((data & 3) << 1);
	turbo_update_samples();
	turbo_update_tachometer();
}



/*******************************************

    Subroc3D 8255 PPI handling

*******************************************/

WRITE8_HANDLER( subroc3d_sound_A_w )
{
	/* bits 4 to 6 control balance */

	subroc3d_volume = (data & 0x0f);
	subroc3d_select = (data & 0x80);
}


WRITE8_HANDLER( subroc3d_sound_B_w )
{
	static UINT8 last = 0;

	float volume = (15 - subroc3d_volume) / 15.0;

	if ((data & 1) && !(last & 1))
		sample_set_volume(0, volume);
	if ((data & 2) && !(last & 2))
		sample_set_volume(1, volume);
	if ((data & 4) && !(last & 4))
		sample_set_volume(2, volume);
	if ((data & 8) && !(last & 8))
		sample_set_volume(3, volume);

	last = data;
}


WRITE8_HANDLER( subroc3d_sound_C_w )
{
	static UINT8 last = 0;

	if ((data & 0x01) && !(last & 0x01))
		sample_start(4, (data & 0x02) ? 6 : 5, 0);
	if ((data & 0x04) && !(last & 0x04))
		sample_start(6, 7, 0);
	if ((data & 0x08) && !(last & 0x08))
		sample_start(3, subroc3d_select ? 4 : 3, 0);
	if ((data & 0x10) && !(last & 0x10))
		sample_start(5, (data & 0x20) ? 10 : 9, 0);

	sample_set_volume(7, (data & 0x40) ? 0 : 1.0);

	last = data;

	sound_global_enable(!(data & 0x80));
}



/*******************************************

    Buck Rogers 8255 PPI handling

*******************************************/

WRITE8_HANDLER( buckrog_sound_A_w )
{
	/* sound controls */
	static int last = -1;

	if ((last & 0x10) && !(data & 0x10))
	{
		buckrog_hit = data & 0x07;
		buckrog_update_samples();
	}

	if ((last & 0x20) && !(data & 0x20))
	{
		buckrog_myship = data & 0x0f;
		buckrog_update_samples();
	}

	if ((last & 0x40) && !(data & 0x40)) sample_start(5, 0, 0); /* alarm0 */
	if ((last & 0x80) && !(data & 0x80)) sample_start(5, 1, 0); /* alarm1 */

	last = data;
}


WRITE8_HANDLER( buckrog_sound_B_w )
{
	/* sound controls */
	static int last = -1;

	if ((last & 0x01) && !(data & 0x01)) sample_start(5, 2, 0); /* alarm2 */
	if ((last & 0x02) && !(data & 0x02)) sample_start(5, 3, 0); /* alarm3 */
	if ((last & 0x04) && !(data & 0x04)) sample_start(2, 5, 0); /* fire */
	if ((last & 0x08) && !(data & 0x08)) sample_start(3, 4, 0); /* exp */
	if ((last & 0x10) && !(data & 0x10)) { sample_start(1, 7, 0); buckrog_update_samples(); } /* hit */
	if ((last & 0x20) && !(data & 0x20)) sample_start(4, 6, 0);	/* rebound */

	if ((data & 0x40) && !sample_playing(0))
	{
		sample_start(0, 8, 1); /* ship */
		buckrog_update_samples();
	}
	if (!(data & 0x40) && sample_playing(0))
		sample_stop(0);

	sound_global_enable(data & 0x80);

	last = data;
}

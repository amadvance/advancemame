/*****************************************************************************
  SN76477 pins and assigned interface variables/functions

                                SN76477_envelope_w()
                               /                    \
                       [ 1] ENV SEL 1           ENV SEL 2 [28]
                       [ 2] GND                   MIXER C [27] \
 SN76477_noise_clock_w [ 3] NOISE EXT OSC         MIXER A [26]  > SN76477_mixer_w()
          noise_res    [ 4] RES NOISE OSC         MIXER B [25] /
         filter_res    [ 5] NOISE FILTER RES      O/S RES [24] oneshot_res
         filter_cap    [ 6] NOISE FILTER CAP      O/S CAP [23] oneshot_cap
          decay_res    [ 7] DECAY RES             VCO SEL [22] SN76477_vco_w()
   attack_decay_cap    [ 8] A/D CAP               SLF CAP [21] slf_cap
 SN76477_enable_w()    [ 9] ENABLE                SLF RES [20] slf_res
         attack_res    [10] ATTACK RES              PITCH [19] pitch_voltage
      amplitude_res    [11] AMP                   VCO RES [18] vco_res
       feedback_res    [12] FEEDBACK              VCO CAP [17] vco_cap
                       [13] OUTPUT           VCO EXT CONT [16] vco_voltage
                       [14] Vcc               +5V REG OUT [15]

    All resistor values in Ohms.
    All capacitor values in Farads.
    Use RES_K, RES_M and CAP_U, CAP_N, CAP_P macros to convert
    magnitudes, eg. 220k = RES_K(220), 47nF = CAP_N(47)

 *****************************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "sn76477.h"

#define VERBOSE 1

#if VERBOSE >= 0
#define LOG(n,x) if( VERBOSE >= (n) ) logerror x
#else
#define LOG(n,x)
#endif

#ifdef MAME_DEBUG
#define CHECK_CHIP_NUM						\
	if( sn == NULL ) 						\
	{										\
		LOG(0,("SN76477 #%d: fatal, index beyond number of chips defined!\n", chip)); \
		return; 							\
	}

#define CHECK_CHIP_NUM_AND_RANGE(BITS,FUNC) \
	CHECK_CHIP_NUM; 						\
	if( data != (data & BITS) ) 			\
		LOG(0,("SN76477 #%d: warning %s called with data = $%02X!\n", chip, #FUNC, data)); \
	data &= BITS;
#else
#define CHECK_CHIP_NUM
#define CHECK_CHIP_NUM_AND_RANGE(BITS,FUNC)
#endif


#define VMIN	0x0000
#define VMAX	0x7fff

struct SN76477 {
	sound_stream * channel;	/* returned by stream_create() */
	int	index;

	int samplerate; 		/* from Machine->sample_rate */
	INT32 vol;				/* current volume (attack/decay) */
	INT32 vol_count;			/* volume adjustment counter */
	INT32 vol_rate;			/* volume adjustment rate - dervied from attack/decay */
	INT32 vol_step;			/* volume adjustment step */

	double slf_count;		/* SLF emulation */
	double slf_freq;		/* frequency - derived */
	double slf_level;		/* triangular wave level */
    INT32 slf_dir;            /* triangular wave direction */
	INT32 slf_out;			/* rectangular output signal state */

	double vco_count;		/* VCO emulation */
	double vco_freq;		/* frequency - derived */
	double vco_step;		/* modulated frequency - derived */
	INT32 vco_out;			/* rectangular output signal state */

	INT32 noise_count;		/* NOISE emulation */
	INT32 noise_clock;		/* external clock signal */
	INT32 noise_freq; 		/* filter frequency - derived */
	INT32 noise_poly; 		/* polynome */
	INT32 noise_out;			/* rectangular output signal state */

	void *envelope_timer;	/* ENVELOPE timer */
	INT32 envelope_state; 	/* attack / decay toggle */

	double attack_time; 	/* ATTACK time (time until vol reaches 100%) */
	double decay_time;		/* DECAY time (time until vol reaches 0%) */
	double oneshot_time;	/* ONE-SHOT time */
	void *oneshot_timer;	/* ONE-SHOT timer */

	INT32 envelope;			/* pin  1, pin 28 */
	double noise_res;		/* pin  4 */
	double filter_res;		/* pin  5 */
	double filter_cap;		/* pin  6 */
	double decay_res;		/* pin  7 */
	double attack_decay_cap;/* pin  8 */
	INT32 enable; 			/* pin  9 */
	double attack_res;		/* pin 10 */
	double amplitude_res;	/* pin 11 */
	double feedback_res;	/* pin 12 */
	double vco_voltage; 	/* pin 16 */
	double vco_cap; 		/* pin 17 */
	double vco_res; 		/* pin 18 */
	double pitch_voltage;	/* pin 19 */
	double slf_res; 		/* pin 20 */
	double slf_cap; 		/* pin 21 */
	INT32 vco_select; 		/* pin 22 */
	double oneshot_cap; 	/* pin 23 */
	double oneshot_res; 	/* pin 24 */
	INT32 mixer;				/* pin 25,26,27 */

	INT16 vol_lookup[VMAX+1-VMIN];	/* volume lookup table */
	const struct SN76477interface *intf;
};


static void attack_decay(int param)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, param);
	sn->envelope_state ^= 1;
	if( sn->envelope_state )
	{
		/* start ATTACK */
		sn->vol_rate = ( sn->attack_time > 0 ) ? VMAX / sn->attack_time : VMAX;
		sn->vol_step = +1;
		LOG(2,("SN76477 #%d: ATTACK rate %d/%d = %d/sec\n", param, sn->vol_rate, sn->samplerate, sn->vol_rate/sn->samplerate));
    }
	else
	{
		/* start DECAY */
		sn->vol = VMAX; /* just in case... */
		sn->vol_rate = ( sn->decay_time > 0 ) ? VMAX / sn->decay_time : VMAX;
		sn->vol_step = -1;
		LOG(2,("SN76477 #%d: DECAY rate %d/%d = %d/sec\n", param, sn->vol_rate, sn->samplerate, sn->vol_rate/sn->samplerate));
    }
}

static void vco_envelope_cb(int param)
{
	attack_decay(param);
}

static void oneshot_envelope_cb(int param)
{
	attack_decay(param);
}

#if VERBOSE
static const char *mixer_mode[8] = {
	"VCO",
	"SLF",
	"Noise",
	"VCO/Noise",
	"SLF/Noise",
	"SLF/VCO/Noise",
	"SLF/VCO",
	"Inhibit"
};
#endif

/*****************************************************************************
 * set MIXER select inputs
 *****************************************************************************/
void SN76477_mixer_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(7,SN76477_mixer_w);

	if( data == sn->mixer )
		return;
	stream_update(sn->channel, 0);
	sn->mixer = data;
	LOG(1,("SN76477 #%d: MIXER mode %d [%s]\n", chip, sn->mixer, mixer_mode[sn->mixer]));
}

void SN76477_mixer_a_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_mixer_a_w);

	data = data ? 1 : 0;
    if( data == (sn->mixer & 1) )
		return;
	stream_update(sn->channel, 0);
	sn->mixer = (sn->mixer & ~1) | data;
	LOG(1,("SN76477 #%d: MIXER mode %d [%s]\n", chip, sn->mixer, mixer_mode[sn->mixer]));
}

void SN76477_mixer_b_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_mixer_b_w);

	data = data ? 2 : 0;
    if( data == (sn->mixer & 2) )
		return;
	stream_update(sn->channel, 0);
	sn->mixer = (sn->mixer & ~2) | data;
	LOG(1,("SN76477 #%d: MIXER mode %d [%s]\n", chip, sn->mixer, mixer_mode[sn->mixer]));
}

void SN76477_mixer_c_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_mixer_c_w);

	data = data ? 4 : 0;
    if( data == (sn->mixer & 4) )
		return;
	stream_update(sn->channel, 0);
	sn->mixer = (sn->mixer & ~4) | data;
	LOG(1,("SN76477 #%d: MIXER mode %d [%s]\n", chip, sn->mixer, mixer_mode[sn->mixer]));
}

#if VERBOSE
static const char *envelope_mode[4] = {
	"VCO",
	"One-Shot",
	"Mixer only",
	"VCO with alternating Polarity"
};
#endif

/*****************************************************************************
 * set ENVELOPE select inputs
 *****************************************************************************/
void SN76477_envelope_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(3,SN76477_envelope_w);

	if( data == sn->envelope )
		return;
	stream_update(sn->channel, 0);
	sn->envelope = data;
	LOG(1,("SN76477 #%d: ENVELOPE mode %d [%s]\n", chip, sn->envelope, envelope_mode[sn->envelope]));
}

void SN76477_envelope_1_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_envelope_1_w);

	if( data == (sn->envelope & 1) )
		return;
	stream_update(sn->channel, 0);
	sn->envelope = (sn->envelope & ~1) | data;
	LOG(1,("SN76477 #%d: ENVELOPE mode %d [%s]\n", chip, sn->envelope, envelope_mode[sn->envelope]));
}

void SN76477_envelope_2_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_envelope_2_w);

	data <<= 1;

	if( data == (sn->envelope & 2) )
		return;
	stream_update(sn->channel, 0);
	sn->envelope = (sn->envelope & ~2) | data;
	LOG(1,("SN76477 #%d: ENVELOPE mode %d [%s]\n", chip, sn->envelope, envelope_mode[sn->envelope]));
}

/*****************************************************************************
 * set VCO external/SLF input
 *****************************************************************************/
void SN76477_vco_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_vco_w);

	if( data == sn->vco_select )
		return;
	stream_update(sn->channel, 0);
	sn->vco_select = data;
	LOG(1,("SN76477 #%d: VCO select %d [%s]\n", chip, sn->vco_select, sn->vco_select ? "Internal (SLF)" : "External (Pin 16)"));
}

/*****************************************************************************
 * set VCO enable input
 *****************************************************************************/
void SN76477_enable_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_enable_w);

	if( data == sn->enable )
		return;
	stream_update(sn->channel, 0);
	sn->enable = data;
	sn->envelope_state = data;

	timer_adjust(sn->envelope_timer, TIME_NEVER, chip, 0);
	timer_adjust(sn->oneshot_timer, TIME_NEVER, chip, 0);

	if( sn->enable == 0 )
	{
		switch( sn->envelope )
		{
		case 0: /* VCO */
			if( sn->vco_res > 0 && sn->vco_cap > 0 )
				timer_adjust(sn->envelope_timer, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)), chip, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)));
			else
				oneshot_envelope_cb(chip);
			break;
		case 1: /* One-Shot */
			sn->vol = 0;
			oneshot_envelope_cb(chip);
			if (sn->oneshot_time > 0)
				timer_adjust(sn->oneshot_timer, sn->oneshot_time, chip, 0);
			break;
		case 2: /* MIXER only */
			sn->vol = VMAX;
			break;
		default:  /* VCO with alternating polariy */
			/* huh? */
			if( sn->vco_res > 0 && sn->vco_cap > 0 )
				timer_adjust(sn->envelope_timer, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)/2), chip, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)/2));
			else
				oneshot_envelope_cb(chip);
			break;
		}
	}
	else
	{
		switch( sn->envelope )
		{
		case 0: /* VCO */
			if( sn->vco_res > 0 && sn->vco_cap > 0 )
				timer_adjust(sn->envelope_timer, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)), chip, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)));
			else
				oneshot_envelope_cb(chip);
			break;
		case 1: /* One-Shot */
			oneshot_envelope_cb(chip);
			break;
		case 2: /* MIXER only */
			sn->vol = VMIN;
			break;
		default:  /* VCO with alternating polariy */
			/* huh? */
			if( sn->vco_res > 0 && sn->vco_cap > 0 )
				timer_adjust(sn->envelope_timer, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)/2), chip, TIME_IN_HZ(0.64/(sn->vco_res * sn->vco_cap)/2));
			else
				oneshot_envelope_cb(chip);
			break;
		}
	}
	LOG(1,("SN76477 #%d: ENABLE line %d [%s]\n", chip, sn->enable, sn->enable ? "Inhibited" : "Enabled" ));
}

/*****************************************************************************
 * set NOISE external signal (pin 3)
 *****************************************************************************/
void SN76477_noise_clock_w(int chip, int data)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM_AND_RANGE(1,SN76477_noise_clock_w);

	if( data == sn->noise_clock )
		return;
	stream_update(sn->channel, 0);
	sn->noise_clock = data;
	/* on the rising edge shift the polynome */
	if( sn->noise_clock )
		sn->noise_poly = ((sn->noise_poly << 7) + (sn->noise_poly >> 10) + 0x18000) & 0x1ffff;
}

/*****************************************************************************
 * set NOISE resistor (pin 4)
 *****************************************************************************/
void SN76477_set_noise_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	stream_update(sn->channel, 0);
	sn->noise_res = res;
}

/*****************************************************************************
 * set NOISE FILTER resistor (pin 5)
 *****************************************************************************/
void SN76477_set_filter_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( res == sn->filter_res )
		return;
	stream_update(sn->channel, 0);
	sn->filter_res = res;
	if( sn->filter_res > 0 && sn->filter_cap > 0 )
	{
		sn->noise_freq = (int)(1.28 / (sn->filter_res * sn->filter_cap));
		LOG(1,("SN76477 #%d: NOISE FILTER freqency %d\n", chip, sn->noise_freq));
	}
	else
		sn->noise_freq = sn->samplerate;
}

/*****************************************************************************
 * set NOISE FILTER capacitor (pin 6)
 *****************************************************************************/
void SN76477_set_filter_cap(int chip, double cap)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( cap == sn->filter_cap )
		return;
	stream_update(sn->channel, 0);
	sn->filter_cap = cap;
	if( sn->filter_res > 0 && sn->filter_cap > 0 )
	{
		sn->noise_freq = (int)(1.28 / (sn->filter_res * sn->filter_cap));
		LOG(1,("SN76477 #%d: NOISE FILTER freqency %d\n", chip, sn->noise_freq));
	}
	else
		sn->noise_freq = sn->samplerate;
}

/*****************************************************************************
 * set DECAY resistor (pin 7)
 *****************************************************************************/
void SN76477_set_decay_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( res == sn->decay_res )
		return;
	stream_update(sn->channel, 0);
	sn->decay_res = res;
	sn->decay_time = sn->decay_res * sn->attack_decay_cap;
	LOG(1,("SN76477 #%d: DECAY time is %fs\n", chip, sn->decay_time));
}

/*****************************************************************************
 * set ATTACK/DECAY capacitor (pin 8)
 *****************************************************************************/
void SN76477_set_attack_decay_cap(int chip, double cap)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( cap == sn->attack_decay_cap )
		return;
	stream_update(sn->channel, 0);
	sn->attack_decay_cap = cap;
	sn->decay_time = sn->decay_res * sn->attack_decay_cap;
	sn->attack_time = sn->attack_res * sn->attack_decay_cap;
	LOG(1,("SN76477 #%d: ATTACK time is %fs\n", chip, sn->attack_time));
	LOG(1,("SN76477 #%d: DECAY time is %fs\n", chip, sn->decay_time));
}

/*****************************************************************************
 * set ATTACK resistor (pin 10)
 *****************************************************************************/
void SN76477_set_attack_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( res == sn->attack_res )
		return;
	stream_update(sn->channel, 0);
	sn->attack_res = res;
	sn->attack_time = sn->attack_res * sn->attack_decay_cap;
	LOG(1,("SN76477 #%d: ATTACK time is %fs\n", chip, sn->attack_time));
}

/*****************************************************************************
 * set AMP resistor (pin 11)
 *****************************************************************************/
void SN76477_set_amplitude_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);
	int i;

	CHECK_CHIP_NUM;

	if( res == sn->amplitude_res )
		return;
	stream_update(sn->channel, 0);
	sn->amplitude_res = res;
	if( sn->amplitude_res > 0 )
	{
#if VERBOSE
		int clip = 0;
#endif
		for( i = 0; i < VMAX+1; i++ )
		{
			int vol = (int)((sn->feedback_res / sn->amplitude_res) * 32767 * i / (VMAX+1));
#if VERBOSE
			if( vol > 32767 && !clip )
				clip = i;
			LOG(3,("%d\n", vol));
#endif
			if( vol > 32767 ) vol = 32767;
			sn->vol_lookup[i] = vol;
		}
		LOG(1,("SN76477 #%d: volume range from -%d to +%d (clip at %d%%)\n", chip, sn->vol_lookup[VMAX-VMIN], sn->vol_lookup[VMAX-VMIN], clip * 100 / 32767));
	}
	else
	{
		memset(sn->vol_lookup, 0, sizeof(sn->vol_lookup));
	}
}

/*****************************************************************************
 * set FEEDBACK resistor (pin 12)
 *****************************************************************************/
void SN76477_set_feedback_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);
	int i;

	CHECK_CHIP_NUM;

	if( res == sn->feedback_res )
		return;
	stream_update(sn->channel, 0);
	sn->feedback_res = res;
	if( sn->amplitude_res > 0 )
	{
#if VERBOSE
		int clip = 0;
#endif
		for( i = 0; i < VMAX+1; i++ )
		{
			int vol = (int)((sn->feedback_res / sn->amplitude_res) * 32767 * i / (VMAX+1));
#if VERBOSE
			if( vol > 32767 && !clip ) clip = i;
#endif
			if( vol > 32767 ) vol = 32767;
			sn->vol_lookup[i] = vol;
		}
		LOG(1,("SN76477 #%d: volume range from -%d to +%d (clip at %d%%)\n", chip, sn->vol_lookup[VMAX-VMIN], sn->vol_lookup[VMAX-VMIN], clip * 100 / 32767));
	}
	else
	{
		memset(sn->vol_lookup, 0, sizeof(sn->vol_lookup));
	}
}

/*****************************************************************************
 * set PITCH voltage (pin 19)
 * TODO: fill with live...
 *****************************************************************************/
void SN76477_set_pitch_voltage(int chip, double voltage)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( voltage == sn->pitch_voltage )
		return;
	stream_update(sn->channel, 0);
	sn->pitch_voltage = voltage;
	LOG(1,("SN76477 #%d: VCO pitch voltage %f (%d%% duty cycle)\n", chip, sn->pitch_voltage, 0));
}

/*****************************************************************************
 * set VCO resistor (pin 18)
 *****************************************************************************/
void SN76477_set_vco_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( res == sn->vco_res )
		return;
	stream_update(sn->channel, 0);
	sn->vco_res = res;
	if( sn->vco_res > 0 && sn->vco_cap > 0 )
	{
		sn->vco_freq = 0.64 / (sn->vco_res * sn->vco_cap);
		LOG(1,("SN76477 #%d: VCO freqency %f\n", chip, sn->vco_freq));
	}
	else
		sn->vco_freq = 0;
}

/*****************************************************************************
 * set VCO capacitor (pin 17)
 *****************************************************************************/
void SN76477_set_vco_cap(int chip, double cap)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( cap == sn->vco_cap )
		return;
	stream_update(sn->channel, 0);
	sn->vco_cap = cap;
	if( sn->vco_res > 0 && sn->vco_cap > 0 )
	{
		sn->vco_freq = 0.64 / (sn->vco_res * sn->vco_cap);
		LOG(1,("SN76477 #%d: VCO freqency %f\n", chip, sn->vco_freq));
	}
	else
		sn->vco_freq = 0;
}

/*****************************************************************************
 * set VCO voltage (pin 16)
 *****************************************************************************/
void SN76477_set_vco_voltage(int chip, double voltage)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( voltage == sn->vco_voltage )
		return;
	stream_update(sn->channel, 0);
	sn->vco_voltage = voltage;
	LOG(1,("SN76477 #%d: VCO ext. voltage %f (%f * %f = %f Hz)\n", chip,
		sn->vco_voltage,
		sn->vco_freq,
		10.0 * (5.0 - sn->vco_voltage) / 5.0,
		sn->vco_freq * 10.0 * (5.0 - sn->vco_voltage) / 5.0));
}

/*****************************************************************************
 * set SLF resistor (pin 20)
 *****************************************************************************/
void SN76477_set_slf_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( res == sn->slf_res )
		return;
	stream_update(sn->channel, 0);
	sn->slf_res = res;
	if( sn->slf_res > 0 && sn->slf_cap > 0 )
	{
		sn->slf_freq = 0.64 / (sn->slf_res * sn->slf_cap);
		LOG(1,("SN76477 #%d: SLF freqency %f\n", chip, sn->slf_freq));
	}
	else
		sn->slf_freq = 0;
}

/*****************************************************************************
 * set SLF capacitor (pin 21)
 *****************************************************************************/
void SN76477_set_slf_cap(int chip, double cap)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( cap == sn->slf_cap )
		return;
	stream_update(sn->channel, 0);
	sn->slf_cap = cap;
	if( sn->slf_res > 0 && sn->slf_cap > 0 )
	{
		sn->slf_freq = 0.64 / (sn->slf_res * sn->slf_cap);
		LOG(1,("SN76477 #%d: SLF freqency %f\n", chip, sn->slf_freq));
	}
	else
		sn->slf_freq = 0;
}

/*****************************************************************************
 * set ONESHOT resistor (pin 24)
 *****************************************************************************/
void SN76477_set_oneshot_res(int chip, double res)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;
	if( res == sn->oneshot_res )
		return;
	sn->oneshot_res = res;
	sn->oneshot_time = 0.8 * sn->oneshot_res * sn->oneshot_cap;
	LOG(1,("SN76477 #%d: ONE-SHOT time %fs\n", chip, sn->oneshot_time));
}

/*****************************************************************************
 * set ONESHOT capacitor (pin 23)
 *****************************************************************************/
void SN76477_set_oneshot_cap(int chip, double cap)
{
	struct SN76477 *sn = sndti_token(SOUND_SN76477, chip);

	CHECK_CHIP_NUM;

	if( cap == sn->oneshot_cap )
        return;
    sn->oneshot_cap = cap;
	sn->oneshot_time = 0.8 * sn->oneshot_res * sn->oneshot_cap;
	LOG(1,("SN76477 #%d: ONE-SHOT time %fs\n", chip, sn->oneshot_time));
}

#define UPDATE_SLF															\
	/*************************************                                  \
     * SLF super low frequency oscillator                                   \
     * frequency = 0.64 / (r_slf * c_slf)                                   \
     *************************************/ 								\
	sn->slf_count -= sn->slf_freq;											\
	while( sn->slf_count <= 0 ) 											\
	{																		\
		sn->slf_count += sn->samplerate;									\
		sn->slf_out ^= 1;													\
	}

#define UPDATE_VCO															\
	/************************************                                   \
     * VCO voltage controlled oscilator                                     \
     * min. freq = 0.64 / (r_vco * c_vco)                                   \
     * freq. range is approx. 10:1                                          \
     ************************************/									\
	if( sn->vco_select )													\
	{																		\
		/* VCO is controlled by SLF */										\
		if( sn->slf_dir == 0 )												\
		{																	\
			sn->slf_level -= sn->slf_freq * 2 * 5.0 / sn->samplerate;		\
			if( sn->slf_level <= 0.0 )										\
			{																\
                sn->slf_level = 0.0;                                        \
				sn->slf_dir = 1;											\
			}																\
		}																	\
		else																\
		if( sn->slf_dir == 1 )												\
		{																	\
			sn->slf_level += sn->slf_freq * 2 * 5.0 / sn->samplerate;		\
			if( sn->slf_level >= 5.0 )										\
			{																\
				sn->slf_level = 5.0;										\
				sn->slf_dir = 0;											\
            }                                                               \
        }                                                                   \
		sn->vco_step = sn->vco_freq * sn->slf_level;						\
	}																		\
	else																	\
	{																		\
		/* VCO is controlled by external voltage */ 						\
		sn->vco_step = sn->vco_freq * sn->vco_voltage;						\
	}																		\
	sn->vco_count -= sn->vco_step;											\
	while( sn->vco_count <= 0 ) 											\
	{																		\
		sn->vco_count += sn->samplerate;									\
		sn->vco_out ^= 1;													\
	}

#define UPDATE_NOISE														\
	/*************************************                                  \
     * NOISE pseudo rand number generator                                   \
     *************************************/ 								\
	if( sn->noise_res > 0 ) 												\
		sn->noise_poly = ( (sn->noise_poly << 7) +							\
						   (sn->noise_poly >> 10) + 						\
						   0x18000 ) & 0x1ffff; 							\
																			\
	/* low pass filter: sample every noise_freq pseudo random value */		\
	sn->noise_count -= sn->noise_freq;										\
	while( sn->noise_count <= 0 )											\
	{																		\
		sn->noise_count = sn->samplerate;									\
		sn->noise_out = sn->noise_poly & 1; 								\
	}

#define UPDATE_VOLUME														\
	/*************************************                                  \
     * VOLUME adjust for attack/decay                                       \
     *************************************/ 								\
	sn->vol_count -= sn->vol_rate;											\
	if( sn->vol_count <= 0 )												\
	{																		\
		int n = - sn->vol_count / sn->samplerate + 1; /* number of steps */ \
		sn->vol_count += n * sn->samplerate;								\
		sn->vol += n * sn->vol_step;										\
		if( sn->vol < VMIN ) sn->vol = VMIN;								\
		if( sn->vol > VMAX ) sn->vol = VMAX;								\
		LOG(3,("SN76477 #%d: vol = $%04X\n", sn->index, sn->vol));      \
	}


/*****************************************************************************
 * mixer select 0 0 0 : VCO
 *****************************************************************************/
static void SN76477_update_0(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
	{
		UPDATE_VCO;
		UPDATE_VOLUME;
		*buffer++ = sn->vco_out ? sn->vol_lookup[sn->vol-VMIN] : -sn->vol_lookup[sn->vol-VMIN];
	}
}

/*****************************************************************************
 * mixer select 0 0 1 : SLF
 *****************************************************************************/
static void SN76477_update_1(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
	{
		UPDATE_SLF;
		UPDATE_VOLUME;
		*buffer++ = sn->slf_out ? sn->vol_lookup[sn->vol-VMIN] : -sn->vol_lookup[sn->vol-VMIN];
	}
}

/*****************************************************************************
 * mixer select 0 1 0 : NOISE
 *****************************************************************************/
static void SN76477_update_2(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
	{
		UPDATE_NOISE;
		UPDATE_VOLUME;
		*buffer++ = sn->noise_out ? sn->vol_lookup[sn->vol-VMIN] : -sn->vol_lookup[sn->vol-VMIN];
	}
}

/*****************************************************************************
 * mixer select 0 1 1 : VCO and NOISE
 *****************************************************************************/
static void SN76477_update_3(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
	{
		UPDATE_VCO;
		UPDATE_NOISE;
		UPDATE_VOLUME;
		*buffer++ = (sn->vco_out & sn->noise_out) ? sn->vol_lookup[sn->vol-VMIN] : -sn->vol_lookup[sn->vol-VMIN];
	}
}

/*****************************************************************************
 * mixer select 1 0 0 : SLF and NOISE
 *****************************************************************************/
static void SN76477_update_4(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
	{
		UPDATE_SLF;
		UPDATE_NOISE;
		UPDATE_VOLUME;
		*buffer++ = (sn->slf_out & sn->noise_out) ? sn->vol_lookup[sn->vol-VMIN] : -sn->vol_lookup[sn->vol-VMIN];
	}
}

/*****************************************************************************
 * mixer select 1 0 1 : VCO, SLF and NOISE
 *****************************************************************************/
static void SN76477_update_5(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
	{
		UPDATE_SLF;
		UPDATE_VCO;
		UPDATE_NOISE;
		UPDATE_VOLUME;
		*buffer++ = (sn->vco_out & sn->slf_out & sn->noise_out) ? sn->vol_lookup[sn->vol-VMIN] : -sn->vol_lookup[sn->vol-VMIN];
	}
}

/*****************************************************************************
 * mixer select 1 1 0 : VCO and SLF
 *****************************************************************************/
static void SN76477_update_6(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
	{
		UPDATE_SLF;
		UPDATE_VCO;
		UPDATE_VOLUME;
		*buffer++ = (sn->vco_out & sn->slf_out) ? sn->vol_lookup[sn->vol-VMIN] : -sn->vol_lookup[sn->vol-VMIN];
	}
}

/*****************************************************************************
 * mixer select 1 1 1 : Inhibit
 *****************************************************************************/
static void SN76477_update_7(struct SN76477 *sn, stream_sample_t *buffer, int length)
{
	while( length-- )
		*buffer++ = 0;
}

static void SN76477_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	struct SN76477 *sn = param;
	stream_sample_t *buffer = _buffer[0];
	if( sn->enable )
	{
		SN76477_update_7(sn,buffer,length);
	}
	else
	{
		switch( sn->mixer )
		{
		case 0:
			SN76477_update_0(sn,buffer,length);
			break;
		case 1:
			SN76477_update_1(sn,buffer,length);
			break;
		case 2:
			SN76477_update_2(sn,buffer,length);
			break;
		case 3:
			SN76477_update_3(sn,buffer,length);
			break;
		case 4:
			SN76477_update_4(sn,buffer,length);
			break;
		case 5:
			SN76477_update_5(sn,buffer,length);
			break;
		case 6:
			SN76477_update_6(sn,buffer,length);
			break;
		default:
			SN76477_update_7(sn,buffer,length);
			break;
		}
	}
}

static void sn76477_state_save_register(struct SN76477 *chip, int sndindex)
{
	state_save_register_item("sn76744", sndindex, chip->vol);
	state_save_register_item("sn76744", sndindex, chip->vol_count);
	state_save_register_item("sn76744", sndindex, chip->vol_rate);
	state_save_register_item("sn76744", sndindex, chip->vol_step);

	state_save_register_item("sn76744", sndindex, chip->slf_count);
	state_save_register_item("sn76744", sndindex, chip->slf_freq);
	state_save_register_item("sn76744", sndindex, chip->slf_level);
	state_save_register_item("sn76744", sndindex, chip->slf_dir);
	state_save_register_item("sn76744", sndindex, chip->slf_out);

	state_save_register_item("sn76744", sndindex, chip->vco_count);
	state_save_register_item("sn76744", sndindex, chip->vco_freq);
	state_save_register_item("sn76744", sndindex, chip->vco_step);
	state_save_register_item("sn76744", sndindex, chip->vco_out);

	state_save_register_item("sn76744", sndindex, chip->noise_count);
	state_save_register_item("sn76744", sndindex, chip->noise_clock);
	state_save_register_item("sn76744", sndindex, chip->noise_freq);
	state_save_register_item("sn76744", sndindex, chip->noise_poly);
	state_save_register_item("sn76744", sndindex, chip->noise_out);

	state_save_register_item("sn76744", sndindex, chip->envelope_state);
	state_save_register_item("sn76744", sndindex, chip->attack_time);
	state_save_register_item("sn76744", sndindex, chip->decay_time);
	state_save_register_item("sn76744", sndindex, chip->oneshot_time);

	state_save_register_item("sn76744", sndindex, chip->envelope);
	state_save_register_item("sn76744", sndindex, chip->noise_res);
	state_save_register_item("sn76744", sndindex, chip->filter_res);
	state_save_register_item("sn76744", sndindex, chip->filter_cap);
	state_save_register_item("sn76744", sndindex, chip->decay_res);
	state_save_register_item("sn76744", sndindex, chip->attack_decay_cap);
	state_save_register_item("sn76744", sndindex, chip->enable);
	state_save_register_item("sn76744", sndindex, chip->attack_res);
	state_save_register_item("sn76744", sndindex, chip->amplitude_res);
	state_save_register_item("sn76744", sndindex, chip->feedback_res);
	state_save_register_item("sn76744", sndindex, chip->vco_voltage);
	state_save_register_item("sn76744", sndindex, chip->vco_cap);
	state_save_register_item("sn76744", sndindex, chip->vco_res);
	state_save_register_item("sn76744", sndindex, chip->pitch_voltage);
	state_save_register_item("sn76744", sndindex, chip->slf_res);
	state_save_register_item("sn76744", sndindex, chip->slf_cap);
	state_save_register_item("sn76744", sndindex, chip->vco_select);
	state_save_register_item("sn76744", sndindex, chip->oneshot_cap);
	state_save_register_item("sn76744", sndindex, chip->oneshot_res);
	state_save_register_item("sn76744", sndindex, chip->mixer);
}

static void *sn76477_start(int sndindex, int clock, const void *config)
{
	struct SN76477 *sn;

	sn = auto_malloc(sizeof(*sn));
	memset(sn, 0, sizeof(*sn));

	sn->intf = config;
	sn->index = sndindex;

	sn->channel = stream_create(0, 1, Machine->sample_rate, sn, SN76477_sound_update);
	sn->samplerate = Machine->sample_rate;

	sn->envelope_timer = timer_alloc(vco_envelope_cb);
	sn->oneshot_timer = timer_alloc(oneshot_envelope_cb);

	/* set up interface (default) values */
	sndintrf_register_token(sn);
	SN76477_set_noise_res(0, sn->intf->noise_res);
	SN76477_set_filter_res(0, sn->intf->filter_res);
	SN76477_set_filter_cap(0, sn->intf->filter_cap);
	SN76477_set_decay_res(0, sn->intf->decay_res);
	SN76477_set_attack_decay_cap(0, sn->intf->attack_decay_cap);
	SN76477_set_attack_res(0, sn->intf->attack_res);
	SN76477_set_amplitude_res(0, sn->intf->amplitude_res);
	SN76477_set_feedback_res(0, sn->intf->feedback_res);
	SN76477_set_oneshot_res(0, sn->intf->oneshot_res);
	SN76477_set_oneshot_cap(0, sn->intf->oneshot_cap);
	SN76477_set_pitch_voltage(0, sn->intf->pitch_voltage);
	SN76477_set_slf_res(0, sn->intf->slf_res);
	SN76477_set_slf_cap(0, sn->intf->slf_cap);
	SN76477_set_vco_res(0, sn->intf->vco_res);
	SN76477_set_vco_cap(0, sn->intf->vco_cap);
	SN76477_set_vco_voltage(0, sn->intf->vco_voltage);
	SN76477_mixer_w(0, 0x07);		/* turn off mixing */
	SN76477_envelope_w(0, 0x03);	/* envelope inputs open */
	SN76477_enable_w(0, 0x01);		/* enable input open */

	sn76477_state_save_register(sn, sndindex);

	return sn;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void sn76477_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void sn76477_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = sn76477_set_info;		break;
		case SNDINFO_PTR_START:							info->start = sn76477_start;			break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "SN76477";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Analog";						break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}


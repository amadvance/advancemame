// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Microprose sound hardware

    Bit of a rush job - needs to be implemented properly eventually

***************************************************************************/
/*
    Differences from MAME:

    Written as a custom sound chip, with the state of the two noise
    channels kept in a static array.
*/

#include <math.h>
#include "driver.h"
#include "streams.h"
#include "sound/custom.h"
#include "includes/micro3d.h"


#define MM5837_CLOCK		100000


/*************************************
 *
 *  Types
 *
 *************************************/

enum
{
	VCF,
	VCQ,
	VCA,
	PAN
};

typedef struct
{
	double a0, a1, a2;		/* Numerator coefficients */
	double b0, b1, b2;		/* Denominator coefficients */
} biquad;

typedef struct
{
	float history[2 * 2];
	float coef[4 * 2 + 1];
	double fs;
	biquad proto_coef[2];
} lp_filter;

typedef struct
{
	double capval;
	double exponent;
} m3d_filter_state;

typedef struct
{
	UINT8 dac_data;
	UINT8 dac[4];
	float gain;
	UINT32 noise_shift;
	UINT8 noise_value;
	UINT8 noise_subcount;
	m3d_filter_state noise_filters[4];
	lp_filter filter;
	sound_stream *stream;
} micro3d_noise;

static micro3d_noise noise[2];
static int noise_count;


/*************************************
 *
 *  Pink noise filtering
 *
 *************************************/

/* Borrowed from segasnd.c */
static void filter_configure(m3d_filter_state *state, double r, double c)
{
	state->capval = 0;
	state->exponent = 1.0 - exp(-1.0 / (r * c * 2000000/8));
}


/*************************************
 *
 *  SSM2047 simulation
 *
 *************************************/

static void lp_filter_init(lp_filter *iir, double fsval)
{
	/* Section 1 */
	iir->proto_coef[0].a0 = 1.0;
	iir->proto_coef[0].a1 = 0;
	iir->proto_coef[0].a2 = 0;
	iir->proto_coef[0].b0 = 1.0;
	iir->proto_coef[0].b1 = 0.765367;
	iir->proto_coef[0].b2 = 1.0;

	/* Section 2 */
	iir->proto_coef[1].a0 = 1.0;
	iir->proto_coef[1].a1 = 0;
	iir->proto_coef[1].a2 = 0;
	iir->proto_coef[1].b0 = 1.0;
	iir->proto_coef[1].b1 = 1.847759;
	iir->proto_coef[1].b2 = 1.0;

	memset(iir->coef, 0, sizeof(iir->coef));
	iir->fs = fsval;
	memset(iir->history, 0, sizeof(iir->history));
}

static void prewarp(double *a0, double *a1, double *a2, double fc, double fs)
{
	double pi = 4.0 * atan(1.0);
	double wp = 2.0 * fs * tan(pi * fc / fs);

	*a2 = *a2 / (wp * wp);
	*a1 = *a1 / wp;
}

static void bilinear(double a0, double a1, double a2,
				double b0, double b1, double b2,
				double *k, double fs, float *coef)
{
	double ad = 4. * a2 * fs * fs + 2. * a1 * fs + a0;
	double bd = 4. * b2 * fs * fs + 2. * b1* fs + b0;

	*k *= ad / bd;

	*coef++ = (2. * b0 - 8. * b2 * fs * fs) / bd;
	*coef++ = (4. * b2 * fs * fs - 2. * b1 * fs + b0) / bd;

	*coef++ = (2. * a0 - 8. * a2 * fs * fs) / ad;
	*coef = (4. * a2 * fs * fs - 2. * a1 * fs + a0) / ad;
}

static void lp_filter_recompute(lp_filter *iir, double k, double q, double fc)
{
	float *c = &iir->coef[1];
	int nInd;

	for (nInd = 0; nInd < 2; nInd++)
	{
		double a0 = iir->proto_coef[nInd].a0;
		double a1 = iir->proto_coef[nInd].a1;
		double a2 = iir->proto_coef[nInd].a2;

		double b0 = iir->proto_coef[nInd].b0;
		double b1 = iir->proto_coef[nInd].b1 / q;
		double b2 = iir->proto_coef[nInd].b2;

		prewarp(&a0, &a1, &a2, fc, iir->fs);
		prewarp(&b0, &b1, &b2, fc, iir->fs);
		bilinear(a0, a1, a2, b0, b1, b2, &k, iir->fs, c);

		c += 4;
	}

	iir->coef[0] = k;
}


/*************************************
 *
 *  Sample and hold and DAC
 *
 *************************************/

void micro3d_noise_sh_w(int which, UINT8 data)
{
	micro3d_noise *chip = &noise[which];

	if (~data & 0x08)
	{
		if (chip->dac_data != chip->dac[data & 3])
		{
			double q, fc;

			stream_update(chip->stream, 0);

			chip->dac[data & 3] = chip->dac_data;

			if (chip->dac[VCA] == 255)
				chip->gain = 0;
			else
				chip->gain = expf(-(float)(chip->dac[VCA]) / 25.0f) * 10.0f;

			q = 0.75/255 * (255 - chip->dac[VCQ]) + 0.1;
			fc = 4500.0/255 * (255 - chip->dac[VCF]) + 100;

			lp_filter_recompute(&chip->filter, chip->gain, q, fc);
		}
	}
}

void micro3d_noise_dac_w(UINT8 data)
{
	noise[0].dac_data = data;
	noise[1].dac_data = data;
}


/*************************************
 *
 *  Stream update
 *
 *************************************/

static void micro3d_noise_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	micro3d_noise *chip = param;
	lp_filter *iir = &chip->filter;
	stream_sample_t *dest_l = outputs[0];
	stream_sample_t *dest_r = outputs[1];
	float pan_l, pan_r;
	int sampindex;

	if (chip->gain == 0)
	{
		memset(dest_l, 0, samples * sizeof(*dest_l));
		memset(dest_r, 0, samples * sizeof(*dest_r));
		return;
	}

	pan_l = (float)(255 - chip->dac[PAN]) / 255.0f;
	pan_r = (float)(chip->dac[PAN]) / 255.0f;

	for (sampindex = 0; sampindex < samples; sampindex++)
	{
		int step;
		float input, white, output;
		float *coef_ptr, *hist1_ptr, *hist2_ptr;
		int i;
		int l, r;

		/* Update the noise source */
		for (step = 2000000 / (2000000/8); step >= chip->noise_subcount; step -= chip->noise_subcount)
		{
			chip->noise_shift = (chip->noise_shift << 1) | (((chip->noise_shift >> 13) ^ (chip->noise_shift >> 16)) & 1);
			chip->noise_value = (chip->noise_shift >> 16) & 1;
			chip->noise_subcount = 2000000 / MM5837_CLOCK;
		}
		chip->noise_subcount -= step;
		input = (float)(chip->noise_value) - 0.5f;
		white = input;

		/* Pink noise filtering */
		chip->noise_filters[0].capval = 0.99765 * chip->noise_filters[0].capval + input * 0.0990460;
		chip->noise_filters[1].capval = 0.96300 * chip->noise_filters[1].capval + input * 0.2965164;
		chip->noise_filters[2].capval = 0.57000 * chip->noise_filters[2].capval + input * 1.0526913;
		input = chip->noise_filters[0].capval + chip->noise_filters[1].capval + chip->noise_filters[2].capval + input * 0.1848;

		input += white;
		input *= 200.0f;

		coef_ptr = &iir->coef[0];

		hist1_ptr = &iir->history[0];
		hist2_ptr = hist1_ptr + 1;

		/* 1st number of coefficients array is overall input scale factor, * or filter gain */
		output = input * (*coef_ptr++);

		for (i = 0 ; i < 2; i++)
		{
			float history1 = *hist1_ptr;
			float history2 = *hist2_ptr;
			float new_hist;

			output = output - history1 * (*coef_ptr++);
			new_hist = output - history2 * (*coef_ptr++);

			output = new_hist + history1 * (*coef_ptr++);
			output = output + history2 * (*coef_ptr++);

			*hist2_ptr++ = *hist1_ptr;
			*hist1_ptr++ = new_hist;
			hist1_ptr++;
			hist2_ptr++;
		}
		output *= 3.5f;

		l = output * pan_l;
		r = output * pan_r;
		if (l < -32768) l = -32768; else if (l > 32767) l = 32767;
		if (r < -32768) r = -32768; else if (r > 32767) r = 32767;
		dest_l[sampindex] = l;
		dest_r[sampindex] = r;
	}
}


/*************************************
 *
 *  Initialisation
 *
 *************************************/

void *micro3d_noise_start(int clock, const struct CustomSound_interface *config)
{
	micro3d_noise *chip = &noise[noise_count++ & 1];

	memset(chip, 0, sizeof(*chip));

	/* Allocate the stream */
	chip->stream = stream_create(0, 2, Machine->sample_rate, chip, micro3d_noise_update);
	lp_filter_init(&chip->filter, Machine->sample_rate);

	filter_configure(&chip->noise_filters[0], 2.7e3 + 2.7e3, 1.0e-6);
	filter_configure(&chip->noise_filters[1], 2.7e3 + 1e3, 0.30e-6);
	filter_configure(&chip->noise_filters[2], 2.7e3 + 270, 0.15e-6);
	filter_configure(&chip->noise_filters[3], 2.7e3 + 0, 0.082e-6);

	state_save_register_item("micro3d_noise", noise_count, chip->dac_data);
	state_save_register_item_array("micro3d_noise", noise_count, chip->dac);
	state_save_register_item("micro3d_noise", noise_count, chip->gain);
	state_save_register_item("micro3d_noise", noise_count, chip->noise_shift);
	state_save_register_item("micro3d_noise", noise_count, chip->noise_value);
	state_save_register_item("micro3d_noise", noise_count, chip->noise_subcount);
	state_save_register_item_array("micro3d_noise", noise_count, chip->filter.history);
	state_save_register_item_array("micro3d_noise", noise_count, chip->filter.coef);

	return chip;
}

void micro3d_noise_reset(void *token)
{
	micro3d_noise *chip = token;

	chip->noise_shift = 0x15555;
	chip->dac[0] = 255;
	chip->dac[1] = 255;
	chip->dac[2] = 255;
	chip->dac[3] = 255;
}

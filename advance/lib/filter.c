/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2004 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include "portable.h"

#include "filter.h"

#include "complex.h"

/****************************************************************************/
/* IIR */

/*
  IIR Filter generation from:
    mkfilter -- given n, compute recurrence relation
    to implement Butterworth, Bessel or Chebyshev filter of order n
    A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
    September 1992
*/

static void filter_iir_reset(adv_filter* f, adv_filter_state* s)
{
	unsigned i;
	struct adv_filter_struct_iir* iir = &f->data.iir;
	unsigned x_max = iir->M + 1;
	unsigned y_max = iir->N + 1;

	s->x_mac = 0;
	for(i=0;i<x_max;++i) {
		s->x_map[i] = 0;
	}
	s->y_mac = 0;
	for(i=0;i<y_max;++i) {
		s->y_map[i] = 0;
	}
}

static void filter_iir_insert(adv_filter* f, adv_filter_state* s, adv_filter_real x)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;
	unsigned j, k;
	unsigned x_max = iir->M + 1;
	unsigned y_max = iir->N + 1;
	double y;

	/* next state */
	++s->x_mac;
	if (s->x_mac == x_max)
		s->x_mac = 0;
	++s->y_mac;
	if (s->y_mac == y_max)
		s->y_mac = 0;

	/* set x[0] */
	s->x_map[s->x_mac] = x / iir->gain;

	/* j == x[-M] */
	j = s->x_mac + 1;
	if (j == x_max)
		j = 0;

	y = 0;

	/* x, sum from x[-M] to x[0] */
	for(k=0;k<x_max;++k) {
		y += iir->xcoeffs[k] * s->x_map[j];
		++j;
		if (j == x_max)
			j = 0;
	}

	assert(j == (s->x_mac + 1) % x_max);

	/* j == y[-N] */
	j = s->y_mac + 1;
	if (j == y_max)
		j = 0;

	/* y, sum from y[-N] to y[-1] */
	for(k=0;k<y_max-1;++k) {
		y += iir->ycoeffs[k] * s->y_map[j];
		++j;
		if (j == y_max)
			j = 0;
	}

	assert(j == s->y_mac);

	/* Decrease the precision if the value is very small in absolute value. */
	/* It's required, otherwise the computation of a input sequence of 0 */
	/* will result in progressively decreasing output values wich aproximate */
	/* the value 0 without reaching it. The problem is that computations */
	/* with these value are VERY slow because generate a lot of underflows */
	/* in the hardware. */
	y += 1E-12;
	y -= 1E-12;

	/* set y[0] */
	s->y_map[s->y_mac] = y;
}

static adv_filter_real filter_iir_extract(adv_filter* f, adv_filter_state* s)
{
	return s->y_map[s->y_mac];
}

static void filter_init(struct adv_filter_struct_iir* iir)
{
	iir->spoles_mac = 0;
	iir->szeros_mac = 0;
}

static void filter_iir_setup(adv_filter* f, adv_filter_type type, adv_filter_model model)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	f->insert = filter_iir_insert;
	f->extract = filter_iir_extract;
	f->reset = filter_iir_reset;

	f->order = iir->M > iir->N ? iir->M : iir->N;
	f->delay = iir->M / 2;
	f->type = type;
	f->model = model;
}

static void filter_choosepole(struct adv_filter_struct_iir* f, adv_complex z)
{
	if (z.re < 0.0) { /* reject re>=0 to ensure filter stability */
		f->spoles_map[f->spoles_mac++] = z;
	}
}

static adv_complex bessel_poles[] = {
{ -1.00000000000e+00, 0.00000000000e+00 },
{ -1.10160133059e+00, 6.36009824757e-01 },
{ -1.32267579991e+00, 0.00000000000e+00 },
{ -1.04740916101e+00, 9.99264436281e-01 },
{ -1.37006783055e+00, 4.10249717494e-01 },
{ -9.95208764350e-01, 1.25710573945e+00 },
{ -1.50231627145e+00, 0.00000000000e+00 },
{ -1.38087732586e+00, 7.17909587627e-01 },
{ -9.57676548563e-01, 1.47112432073e+00 },
{ -1.57149040362e+00, 3.20896374221e-01 },
{ -1.38185809760e+00, 9.71471890712e-01 },
{ -9.30656522947e-01, 1.66186326894e+00 },
{ -1.68436817927e+00, 0.00000000000e+00 },
{ -1.61203876622e+00, 5.89244506931e-01 },
{ -1.37890321680e+00, 1.19156677780e+00 },
{ -9.09867780623e-01, 1.83645135304e+00 },
{ -1.75740840040e+00, 2.72867575103e-01 },
{ -1.63693941813e+00, 8.22795625139e-01 },
{ -1.37384121764e+00, 1.38835657588e+00 },
{ -8.92869718847e-01, 1.99832584364e+00 },
{ -1.85660050123e+00, 0.00000000000e+00 },
{ -1.80717053496e+00, 5.12383730575e-01 },
{ -1.65239648458e+00, 1.03138956698e+00 },
{ -1.36758830979e+00, 1.56773371224e+00 },
{ -8.78399276161e-01, 2.14980052431e+00 },
{ -1.92761969145e+00, 2.41623471082e-01 },
{ -1.84219624443e+00, 7.27257597722e-01 },
{ -1.66181024140e+00, 1.22110021857e+00 },
{ -1.36069227838e+00, 1.73350574267e+00 },
{ -8.65756901707e-01, 2.29260483098e+00 },
};

static adv_complex cmone = { -1.0, 0.0 };
static adv_complex czero = { 0.0, 0.0 };
static adv_complex cone = { 1.0, 0.0 };
static adv_complex ctwo = { 2.0, 0.0 };
static adv_complex chalf = { 0.5, 0.0 };

/**
 * Transform the analog filter prototype into the appropriate analog filter type.
 * The prototype filter is an analog low pass filter with
 * a cut frequency of 1 Hz.
 * The frequency is warped for the next application of the bilinear transform.
 */
static void filter_normalize(struct adv_filter_struct_iir* f, double raw_alpha1, double raw_alpha2, adv_filter_type type)
{
	adv_complex w1, w2;
	adv_complex w0, bw;
	double warped_alpha1, warped_alpha2;
	int i;
	
	/* for bilinear transform, perform pre-warp on alpha values */
	warped_alpha1 = tan(M_PI * raw_alpha1) / M_PI;
	warped_alpha2 = tan(M_PI * raw_alpha2) / M_PI;

	w1 = adv_creal(2.0 * M_PI * warped_alpha1);
	w2 = adv_creal(2.0 * M_PI * warped_alpha2);

	switch (type) {
	case adv_filter_lp :
		for(i=0;i<f->spoles_mac;++i)
			f->spoles_map[i] = adv_cmul(f->spoles_map[i], w1);
		f->szeros_mac = 0;
		break;
	case adv_filter_hp :
		for(i=0;i<f->spoles_mac;++i)
			f->spoles_map[i] = adv_cdiv(w1, f->spoles_map[i]);
		f->szeros_mac = f->spoles_mac;
		for(i=0;i<f->spoles_mac;++i)
			f->szeros_map[i] = czero;
		break;
	case adv_filter_bp :
		w0 = adv_csqrt(adv_cmul(w1, w2));
		bw = adv_csub(w2, w1);
		for(i=0;i<f->spoles_mac;++i) {
			adv_complex hba, temp;
			hba = adv_cmul(chalf, adv_cmul(f->spoles_map[i], bw));
			temp = adv_csqrt(adv_csub(cone, adv_csqr(adv_cdiv(w0, hba))));
			f->spoles_map[i] = adv_cmul(hba, adv_cadd(cone, temp));
			f->spoles_map[f->spoles_mac + i] = adv_cmul(hba, adv_csub(cone, temp));
		}
		f->szeros_mac = f->spoles_mac;
		for (i=0;i<f->spoles_mac;++i)
			f->szeros_map[i] = czero;
		f->spoles_mac *= 2;
		break;
	case adv_filter_bs :
		w0 = adv_csqrt(adv_cmul(w1, w2));
		bw = adv_csub(w2, w1);
		for(i=0;i<f->spoles_mac;++i) {
			adv_complex hba, temp;
			hba = adv_cmul(chalf, adv_cdiv(bw, f->spoles_map[i]));
			temp = adv_csqrt(adv_csub(cone, adv_csqr(adv_cdiv(w0, hba))));
			f->spoles_map[i] = adv_cmul(hba, adv_cadd(cone, temp));
			f->spoles_map[f->spoles_mac + i] = adv_cmul(hba, adv_csub(cone, temp));
		}
		for (i=0;i<f->spoles_mac;++i) {
			f->szeros_map[i] = adv_cimag(w0.re);
			f->szeros_map[f->spoles_mac + i] = adv_cimag(-w0.re);
		}
		f->spoles_mac *= 2;
		f->szeros_mac = f->spoles_mac;
		break;
	}
}

/**
 * Bilinear trasformation.
 */
static adv_complex cblt(adv_complex z)
{
	adv_complex top, bot;
	top = adv_cadd(ctwo, z);
	bot = adv_csub(ctwo, z);
	return adv_cdiv(top, bot);
}

/**
 * Transform the analog filter in a digital filter.
 * Given S-plane poles, compute Z-plane poles using
 * the bilinear transformation.
 */
static void filter_compute_z(struct adv_filter_struct_iir* f, adv_filter_type type)
{
	int i;

	f->zpoles_mac = f->spoles_mac;
	f->zzeros_mac = f->szeros_mac;

	for(i=0;i<f->spoles_mac;++i)
		f->zpoles_map[i] = cblt(f->spoles_map[i]);

	for(i=0;i<f->szeros_mac;++i)
		f->zzeros_map[i] = cblt(f->szeros_map[i]);

	while (f->zzeros_mac < f->zpoles_mac)
		f->zzeros_map[f->zzeros_mac++] = cmone;
}

/**
 * Multiply factor (z-w) into coeffs.
 */
static void filter_multin(adv_complex w, int npz, adv_complex coeffs[])
{
	adv_complex nw;
	int i;

	nw = adv_cneg(w);
	for(i=npz;i>=1;--i)
		coeffs[i] = adv_cadd(adv_cmul(nw, coeffs[i]), coeffs[i - 1]);
	coeffs[0] = adv_cmul(nw, coeffs[0]);
}

/**
 * Compute product of poles or zeros as a polynomial of z.
 */
static void filter_expand(adv_complex pz[], int npz, adv_complex coeffs[])
{
	int i;

	coeffs[0] = cone;

	for(i=0;i<npz;++i)
		coeffs[i + 1] = czero;

	for(i=0;i<npz;++i)
		filter_multin(pz[i], npz, coeffs);
}

/**
 * Compute the filter coefficients expanding the transform function.
 * The filter gain is also computed.
 */
static void filter_expand_poly(struct adv_filter_struct_iir* f, double raw_alpha1, double raw_alpha2, adv_filter_type type)
{
	adv_complex topcoeffs[FILTER_POLE_MAX + 1], botcoeffs[FILTER_POLE_MAX + 1];
	adv_complex zfc;
	adv_complex dc_gain, hf_gain, fc_gain;
	adv_complex gain;
	int i;

	/* expand from: (1 - ZERO[0]*z^-1)(1 - ZERO[1]*z^-1)...(1 - ZERO[M-1]*z^-1) */
	/* to: top[M] + top[M-1]*z^-1 + ... + top[0]*z^-M */
	filter_expand(f->zzeros_map, f->zzeros_mac, topcoeffs);
	filter_expand(f->zpoles_map, f->zpoles_mac, botcoeffs);

	/* note that cevaluate is called for z instead for z^-1 */
	/* and with reversed coefficients (a + b+z^-1 == b + a*z^1) */
	dc_gain = adv_cevaluate(topcoeffs, f->zzeros_mac, botcoeffs, f->zpoles_mac, cone);
	zfc = adv_cexp(adv_cimag(2 * M_PI * 0.5 * (raw_alpha1 + raw_alpha2))); /* "jwT" for centre freq. */
	fc_gain = adv_cevaluate(topcoeffs, f->zzeros_mac, botcoeffs, f->zpoles_mac, zfc);
	hf_gain = adv_cevaluate(topcoeffs, f->zzeros_mac, botcoeffs, f->zpoles_mac, cmone);

	for(i=0;i<=f->zzeros_mac;++i)
		f->xcoeffs[i] = topcoeffs[i].re / botcoeffs[f->zpoles_mac].re;
	for(i=0;i<=f->zpoles_mac;++i)
		f->ycoeffs[i] = -(botcoeffs[i].re / botcoeffs[f->zpoles_mac].re);

	switch (type) {
	case adv_filter_lp :
		gain = dc_gain;
		break;
	case adv_filter_hp :
		gain = hf_gain;
		break;
	case adv_filter_bp :
		gain = fc_gain;
		break;
	case adv_filter_bs :
		gain = adv_csqrt(adv_cmul(dc_gain, hf_gain));
		break;
	default:
		gain = czero;
		break;
	}

	f->gain = hypot(gain.re, gain.im);
	f->M = f->zzeros_mac;
	f->N = f->zpoles_mac;
}

/**
 * Compute the prototype Chebyshev analog filter.
 */
static void filter_chebyshev_compute_s(struct adv_filter_struct_iir* f, unsigned order, double ripple)
{
	int i;
	double rip, eps, y;

	for(i=0;i<2*order;++i) {
		adv_complex s;
		s.re = 0.0;
		s.im = (order & 1) ? (i * M_PI) / order : ((i + 0.5) * M_PI) / order;
		filter_choosepole(f, adv_cexp(s));
	}
	
	assert(ripple < 0);

	rip = pow(10.0, -ripple / 10.0);
	eps = sqrt(rip - 1.0);
	y = asinh(1.0 / eps) / (double)order;

	assert(y > 0);

	for(i=0;i<f->spoles_mac;++i) {
		f->spoles_map[i].re *= sinh(y);
		f->spoles_map[i].im *= cosh(y);
	}
}

/**
 * Compute the prototype Bessel analog filter.
 */
static void filter_bessel_compute_s(struct adv_filter_struct_iir* f, unsigned order)
{
	int i;
	int p = (order * order) / 4; /* ptr into table */
	if (order & 1) {
		assert(p < sizeof(bessel_poles)/sizeof(bessel_poles[0]));
		filter_choosepole(f, bessel_poles[p]);
		++p;
	}
	for(i=0;i<order/2;++i) {
		assert(p < sizeof(bessel_poles)/sizeof(bessel_poles[0]));
		filter_choosepole(f, bessel_poles[p]);
		filter_choosepole(f, adv_cconj(bessel_poles[p]));
		p++;
	}
}

/**
 * Compute the prototype Butterworth analog filter.
 */
static void filter_butterworth_compute_s(struct adv_filter_struct_iir* f, unsigned order)
{
	int i;
	for(i=0;i<2*order;++i) {
		adv_complex s;
		s.re = 0.0;
		s.im = (order & 1) ? (i * M_PI) / order : ((i + 0.5) * M_PI) / order;
		filter_choosepole(f, adv_cexp(s));
	}
}

void adv_filter_lp_chebyshev_set(adv_filter* f, double freq, unsigned order, double ripple)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq && freq <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_chebyshev_compute_s(iir, order, ripple);
	filter_normalize(iir, freq, freq, adv_filter_lp);
	filter_compute_z(iir, adv_filter_lp);
	filter_expand_poly(iir, freq, freq, adv_filter_lp);

	filter_iir_setup(f, adv_filter_lp, adv_filter_iir_chebyshev);
}

void adv_filter_lp_bessel_set(adv_filter* f, double freq, unsigned order)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq && freq <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_bessel_compute_s(iir, order);
	filter_normalize(iir, freq, freq, adv_filter_lp);
	filter_compute_z(iir, adv_filter_lp);
	filter_expand_poly(iir, freq, freq, adv_filter_lp);

	filter_iir_setup(f, adv_filter_lp, adv_filter_iir_bessel);
}

void adv_filter_lp_butterworth_set(adv_filter* f, double freq, unsigned order)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq && freq <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_butterworth_compute_s(iir, order);
	filter_normalize(iir, freq, freq, adv_filter_lp);
	filter_compute_z(iir, adv_filter_lp);
	filter_expand_poly(iir, freq, freq, adv_filter_lp);

	filter_iir_setup(f, adv_filter_lp, adv_filter_iir_butterworth);
}

void adv_filter_hp_chebyshev_set(adv_filter* f, double freq, unsigned order, double ripple)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq && freq <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_chebyshev_compute_s(iir, order, ripple);
	filter_normalize(iir, freq, freq, adv_filter_hp);
	filter_compute_z(iir, adv_filter_hp);
	filter_expand_poly(iir, freq, freq, adv_filter_hp);

	filter_iir_setup(f, adv_filter_hp, adv_filter_iir_chebyshev);
}

void adv_filter_hp_bessel_set(adv_filter* f, double freq, unsigned order)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq && freq <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_bessel_compute_s(iir, order);
	filter_normalize(iir, freq, freq, adv_filter_hp);
	filter_compute_z(iir, adv_filter_hp);
	filter_expand_poly(iir, freq, freq, adv_filter_hp);

	filter_iir_setup(f, adv_filter_hp, adv_filter_iir_bessel);
}

void adv_filter_hp_butterworth_set(adv_filter* f, double freq, unsigned order)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq && freq <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_butterworth_compute_s(iir, order);
	filter_normalize(iir, freq, freq, adv_filter_hp);
	filter_compute_z(iir, adv_filter_hp);
	filter_expand_poly(iir, freq, freq, adv_filter_hp);

	filter_iir_setup(f, adv_filter_hp, adv_filter_iir_butterworth);
}

void adv_filter_bp_chebyshev_set(adv_filter* f, double freq_low, double freq_high, unsigned order, double ripple)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq_low && freq_low < freq_high && freq_high <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_chebyshev_compute_s(iir, order, ripple);
	filter_normalize(iir, freq_low, freq_high, adv_filter_bp);
	filter_compute_z(iir, adv_filter_bp);
	filter_expand_poly(iir, freq_low, freq_high, adv_filter_bp);

	filter_iir_setup(f, adv_filter_bp, adv_filter_iir_chebyshev);
}

void adv_filter_bp_bessel_set(adv_filter* f, double freq_low, double freq_high, unsigned order)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq_low && freq_low < freq_high && freq_high <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_bessel_compute_s(iir, order);
	filter_normalize(iir, freq_low, freq_high, adv_filter_bp);
	filter_compute_z(iir, adv_filter_bp);
	filter_expand_poly(iir, freq_low, freq_high, adv_filter_bp);

	filter_iir_setup(f, adv_filter_bp, adv_filter_iir_bessel);
}

void adv_filter_bp_butterworth_set(adv_filter* f, double freq_low, double freq_high, unsigned order)
{
	struct adv_filter_struct_iir* iir = &f->data.iir;

	assert(0 < freq_low && freq_low < freq_high && freq_high <= 0.5);

	if (order > FILTER_ORDER_IIR_MAX)
		order = FILTER_ORDER_IIR_MAX;

	filter_init(iir);
	filter_butterworth_compute_s(iir, order);
	filter_normalize(iir, freq_low, freq_high, adv_filter_bp);
	filter_compute_z(iir, adv_filter_bp);
	filter_expand_poly(iir, freq_low, freq_high, adv_filter_bp);

	filter_iir_setup(f, adv_filter_bp, adv_filter_iir_butterworth);
}

/****************************************************************************/
/* FIR */

static void filter_fir_reset(adv_filter* f, adv_filter_state* s)
{
	struct adv_filter_struct_fir* fir = &f->data.fir;
	unsigned x_max = fir->M + 1;
	unsigned i;

	s->x_mac = 0;
	for(i=0;i<x_max;++i) {
		s->x_map[i] = 0;
	}
}

static adv_filter_real filter_fir_extract(adv_filter* f, adv_filter_state* s)
{
	struct adv_filter_struct_fir* fir = &f->data.fir;
	unsigned x_max = fir->M + 1;
	unsigned x_center = x_max / 2;
	adv_filter_real y;
	unsigned i, j, k;

	/* i == x[0] */
	/* j == x[-M] */
	i = s->x_mac;
	j = i + 1;
	if (j == x_max)
		j = 0;

	y = 0;

	/* symmetric coefficients */
	for(k=0;k<x_center;++k) {
		y += fir->xcoeffs[k] * (s->x_map[i] + s->x_map[j]);

		/* next j */
		++j;
		if (j == x_max)
			j = 0;

		/* prev i */
		if (i == 0)
			i = x_max;
		--i;
	}

	assert((x_max % 2) == (i == j));

	/* central coefficient */
	if (i == j)
		y += fir->xcoeffs[x_center] * s->x_map[i];

	return y;
}

static void filter_fir_insert(adv_filter* f, adv_filter_state* s, adv_filter_real x)
{
	struct adv_filter_struct_fir* fir = &f->data.fir;
	unsigned x_max = fir->M + 1;

	/* next position */
	++s->x_mac;
	if (s->x_mac == x_max)
		s->x_mac = 0;

	/* set the most recent sample */
	s->x_map[s->x_mac] = x;
}

static double csinc(double n, double f)
{
	if (n == 0)
		return f;
	else
		return sin(M_PI*n*f) / (M_PI*n);
}

void adv_filter_lp_windowedsinc_set(adv_filter* f, double freq, unsigned order)
{
	struct adv_filter_struct_fir* fir = &f->data.fir;
	unsigned i;
	double gain;

	assert(0 < freq && freq <= 0.5);

	if (order > FILTER_ORDER_FIR_MAX)
		order = FILTER_ORDER_FIR_MAX;

	/* order must be even */
	if (order % 2 != 0)
		++order;

	f->insert = filter_fir_insert;
	f->extract = filter_fir_extract;
	f->reset = filter_fir_reset;

	/* compute the antitrasform of the perfect low pass filter */
	for(i=0;i<=order;++i) {
		double n, c, w;

		n = i - 0.5 * order;

		/* sample value */
		c = csinc(n, 2*freq);

		/* w = 0.54 - 0.46 * cos(2*M_PI*i/order); */ /* Hamming */
		w = 0.42 - 0.5 * cos(2*M_PI*i/order) + 0.08 * cos(4*M_PI*i/order); /* Blackman */

		/* apply the window */
		c *= w;

		/* set coefficient */
		fir->xcoeffs[i] = c;
	}

	/* adjust the gain to be exact 1.0 */
	gain = 0;
	for(i=0;i<=order;++i)
		gain += fir->xcoeffs[i];
	for(i=0;i<=order;++i)
		fir->xcoeffs[i] /= gain;

	fir->M = order;

	f->order = fir->M;
	f->delay = fir->M / 2;
	f->type = adv_filter_lp;
	f->model = adv_filter_fir_windowedsinc;
}


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004 Andrea Mazzoleni
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

#include "dft.h"

static adv_error dft_init(struct adv_dft_stage_struct* context, unsigned n)
{
	unsigned i, j;
	double konst, delta;
	double thi, early, late;
	double* p;
	unsigned* fbp;
	unsigned* bbp;
	unsigned k, halfk, kmin1;
	int log2n;

	/* check for legal size */
	if (n == 0 || (n & (n-1))) {
		return -1;
	}

	/* compute log size */
	for (i = 1, log2n = 0; i < n; i <<= 1, log2n++);

	context->n = n;
	context->log2n = log2n;

	/* allocate storage for new tables */
	context->ss = malloc(n * sizeof(double));
	if (!context->ss)
		return -1;

	context->brev = malloc(n * sizeof(unsigned));
	if (!context->brev)
		return -1;

	/* compute sine table, using the recurrent relation */
	/* x[n] = 2 * cos(d) * x[n-1] - x[n-2], where d = 2 * pi / n */
	delta = 2. * M_PI / n;
	konst = 2. * cos(delta);
	p = context->ss;
	*p++ = late = 0.;
	*p++ = early = sin(delta);
	for(i=2;i<n;++i) {
		*p++ = thi = konst * early - late;
		late = early, early = thi;
	}

	/* setup the cosine table */
	context->cc = context->ss + n / 4;

	/* compute bit-reversing table */
	context->brev[0] = 0;
	for(k=2;k<=n;k<<=1) {
		halfk = k >> 1, kmin1 = k - 1;
		fbp = context->brev;
		for(j=0;j<halfk;++j)
			*fbp++ <<= 1;
		bbp = fbp - 1;
		for(;j<k;++j)
			*fbp++ = kmin1 - *bbp--;
	}

	return 0;
}

static void dft_free(struct adv_dft_stage_struct* context)
{
	free(context->ss);
	free(context->brev);
}

/**
 * SRFT (Split-radix fast Fourier transform).
 * A Duhamel-Hollman split-radix DIF FFT.
 * Ref.: Electronic Letters, Jan. 5, 1984.
 */
static void dft(const struct adv_dft_stage_struct* context, double* restrict yr, double* restrict yi, unsigned ratio)
{
	unsigned n4, n2;
	int i, j, k;
	int i0, i1, i2, i3, is, id;
	double r1, r2, s1, s2, s3;
	double cc1, ss1, cc3, ss3;
	unsigned ue, ua, ua3;
	int log2n2;

	unsigned n = context->n >> ratio;
	const double* restrict ss = context->ss;
	const double* restrict cc = context->cc;
	const unsigned* restrict brev = context->brev;
	int log2n = context->log2n - ratio;

	n2 = n + n;
	log2n2 = log2n + 1;

	for(k=1;k<log2n;++k) {
		n2 >>= 1, log2n2--;
		n4 = n2 >> 2;
		ue = n >> log2n2; /* e = TWOPI / n2; */
		ua = ue;
		is = 0,  id = n2 << 1;
		do {
			for (i0 = is; i0 < n; i0 += id) {
				i1 = i0 + n4,  i2 = i1 + n4,  i3 = i2 + n4;

				r1 = yr[i0] - yr[i2],   yr[i0] += yr[i2];
				r2 = yr[i1] - yr[i3],   yr[i1] += yr[i3];
				s1 = yi[i0] - yi[i2],   yi[i0] += yi[i2];
				s2 = yi[i1] - yi[i3],   yi[i1] += yi[i3];

				yr[i3] = r1 - s2,   yr[i2] = r1 + s2;
				yi[i2] = s1 - r2,   yi[i3] = r2 + s1;
			}
			is = (id << 1) - n2,  id <<= 2;
		} while (is < n);

		for(j=1;j<n4;++j) {
			ua3 = ua + ua + ua;
			/* cc1 = cos(a), ss1 = sin(a), cc3 = cos(a3), ss3 = sin(a3); */
			cc1 = cc[ua << ratio], ss1 = ss[ua << ratio], cc3 = cc[ua3 << ratio], ss3 = ss[ua3 << ratio];
			ua += ue;
			is = j, id = n2 << 1;
			do {
				for (i0 = is; i0 < n; i0 += id) {
					i1 = i0 + n4,   i2 = i1 + n4,   i3 = i2 + n4;

					r1 = yr[i0] - yr[i2],   yr[i0] += yr[i2];
					r2 = yr[i1] - yr[i3],   yr[i1] += yr[i3];
					s1 = yi[i0] - yi[i2],   yi[i0] += yi[i2];
					s2 = yi[i1] - yi[i3],   yi[i1] += yi[i3];

					s3 = r1 - s2,   r1 += s2;
					s2 = r2 - s1,   r2 += s1;

					yr[i2] =   r1 * cc1 - s2 * ss1;
					yi[i2] = - s2 * cc1 - r1 * ss1;
					yr[i3] =   s3 * cc3 + r2 * ss3;
					yi[i3] =   r2 * cc3 - s3 * ss3;
				}

				is = (id << 1) - n2 + j;
				id <<= 2;
			} while (is < n);
		}
	}

	is = 0, id = 4;
	do {
		for (i0 = is; i0 < n; i0 += id) {
			i1 = i0 + 1;

			r1 = yr[i0], yr[i0] = r1 + yr[i1], yr[i1] = r1 - yr[i1];
			r1 = yi[i0], yi[i0] = r1 + yi[i1], yi[i1] = r1 - yi[i1];
		}
		is = id + id - 2,  id <<= 2;
	} while (is < n);

	/* shuffle output vector */
	for(i=0;i<n;++i) {
		j = brev[i << ratio];
		if (j > i) {
			double xt, yt;
			xt = yr[j];
			yr[j] = yr[i];
			yr[i] = xt;
			yt = yi[j];
			yi[j] = yi[i];
			yi[i] = yt;
		}
	}
}

static void dft_execute(adv_dft* context)
{
	dft(&context->stage, context->xr, context->xi, 0);
}

/**
 * Initialize a DFT plan.
 * \paran DFT plan to initialize.
 * \param n Size of the DFT.
 */
adv_error adv_dft_init(adv_dft* context, unsigned n)
{
	if (dft_init(&context->stage, n) != 0)
		return -1;

	context->n = n;
	context->xr = malloc(n * sizeof(double));
	context->xi = malloc(n * sizeof(double));
	context->execute = dft_execute;

	return 0;
}

static void idft_execute(adv_dft* context)
{
	unsigned i;
	unsigned n = context->n;
	double* restrict xr = context->xr;
	double* restrict xi = context->xi;
	double dn = context->n;

	dft(&context->stage, xi, xr, 0);

	for(i=0;i<n;++i) {
		xr[i] /= dn;
		xi[i] /= dn;
	}
}

/**
 * Initialize an inverse DFT plan.
 * \paran DFT plan to initialize.
 * \param n Size of the DFT.
 */
adv_error adv_idft_init(adv_dft* context, unsigned n)
{
	if (dft_init(&context->stage, n) != 0)
		return -1;

	context->n = n;
	context->xr = malloc(n * sizeof(double));
	context->xi = malloc(n * sizeof(double));
	context->execute = idft_execute;

	return 0;
}

static void dftr_execute(adv_dft* context)
{
	unsigned i, j;
	unsigned n = context->n;
	double* xr = context->xr;
	double* xi = context->xi;
	unsigned n2 = n / 2;
	unsigned n4 = n2 / 2;
	double* Yr = xi;
	double* Yi = xi + n2;
	double* Xr = xr;
	double* Xi = xi;
	const double* restrict ss = context->stage.ss;
	const double* restrict cc = context->stage.cc;

	for(i=0;i<n2;++i) {
		Yr[i] = xr[i*2];
		Yi[i] = xr[i*2+1];
	}

	dft(&context->stage, Yr, Yi, 1);

	Xr[0] = Yr[0] + Yi[0];
	Xr[n2] = Yr[0] - Yi[0];
	Xi[0] = 0;
	Xi[n2] = 0;

	i = 1;
	j = n-1;
	for(i=1;i<n4;++i,--j) {
		double c, s;
		double Ar2, Ai2, Br2, Bi2;
		double Br2c, Bi2c, Br2s, Bi2s;

		Ar2 = 0.5*(Yr[n2-i]+Yr[i]);
		Ai2 = 0.5*(-Yi[n2-i]+Yi[i]);
		Br2 = 0.5*(Yi[n2-i]+Yi[i]);
		Bi2 = 0.5*(Yr[n2-i]-Yr[i]);

		c = cc[i];
		s = -ss[i];

		Br2c = Br2 * c;
		Bi2c = Bi2 * c;
		Br2s = Br2 * s;
		Bi2s = Bi2 * s;

		Xr[j] = Xr[i] = Ar2+Br2c-Bi2s;
		Xi[j] = -(Xi[i] = Ai2+Br2s+Bi2c);
		Xr[j-n2] = Xr[n2+i] = Ar2-Br2c+Bi2s;
		Xi[j-n2] = -(Xi[n2+i] = Ai2-Br2s-Bi2c);
	}

	Xr[j] = Xr[i] = Yr[i];
	Xi[i] = -(Xi[j] = Yi[i]);
}

/**
 * Initialize a DFT plan for real values.
 * For this plan only the imaginary vector in input is ignored
 * and assumed to be filled with 0.
 * The output vector is completely filled, also the conjugate values.
 * This specialized real version is approsimatively twice as fast
 * than the complex version.
 * \paran DFT plan to initialize.
 * \param n Size of the DFT.
 */
adv_error adv_dftr_init(adv_dft* context, unsigned n)
{
	if (dft_init(&context->stage, n) != 0)
		return -1;

	context->n = n;
	context->xr = malloc(n * sizeof(double));
	context->xi = malloc(n * sizeof(double));
	context->execute = dftr_execute;

	return 0;
}

/**
 * Deinitialize a DFT plan.
 */
void adv_dft_free(adv_dft* context)
{
	dft_free(&context->stage);
	free(context->xr);
	free(context->xi);
}


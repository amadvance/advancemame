/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003 Andrea Mazzoleni
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
 */

#include "filter.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

unsigned adv_filter_order_get(const adv_filter* f)
{
	return f->order;
}

unsigned adv_filter_delay_get(const adv_filter* f)
{
	return (f->order-1) / 2;
}

void adv_filter_state_reset(adv_filter* f, adv_filter_state* s)
{
	int i;
	s->prev_mac = 0;
	for(i=0;i<f->order;++i) {
		s->xprev[i] = 0;
	}
}

adv_filter_real adv_filter_extract(adv_filter* f, adv_filter_state* s)
{
	unsigned order = f->order;
	unsigned midorder = f->order / 2;
	adv_filter_real y;
	unsigned i, j, k;

	/* i == [0] (most recent sample) */
	/* j == [-2*midorder] (most older sample) */
	i = s->prev_mac;
	j = i + 1;
	if (j == order)
		j = 0;

	y = 0;

	/* symmetric coefficients */
	for(k=0;k<midorder;++k) {
		y += f->xcoeffs[midorder-k] * (s->xprev[i] + s->xprev[j]);
		++j;
		if (j == order)
			j = 0;
		if (i == 0)
			i = order - 1;
		else
			--i;
	}

	/* central coefficient */
	y += f->xcoeffs[0] * s->xprev[i];

#ifdef FILTER_USE_INT
	return y >> FILTER_INT_FRACT;
#else
	return y;
#endif
}

void adv_filter_lpfir_set(adv_filter*f, double freq, unsigned order)
{
	unsigned midorder = (order - 1) / 2;
	unsigned i;
	double gain;

	assert( order <= FILTER_ORDER_MAX );
	assert( order % 2 == 1 );
	assert( 0 < freq && freq <= 0.5 );

	if (order > FILTER_ORDER_MAX)
		order = FILTER_ORDER_MAX;
	if (order % 2 == 0)
		++order;

	/* compute the antitrasform of the perfect low pass filter */

	/* central sample coefficient */
	gain = 2*freq;
#ifdef FILTER_USE_INT
	f->xcoeffs[0] = gain * (1 << FILTER_INT_FRACT);
#else
	f->xcoeffs[0] = gain;
#endif

	/* other coefficients, they are symmetric on the center */
	for(i=1;i<=midorder;++i) {
		/* number of the sample starting from 0 to (order-1) included */
		unsigned n = i + midorder;

		/* sample value */
		double c = sin(2*M_PI*freq*i) / (M_PI*i);

		/* apply only one window or none */
		/* double w = 2 - 2*n/(order-1); */ /* Bartlett (triangular) */
		/* double w = 0.5 * (1 - cos(2*M_PI*n/(order-1))); */ /* Hanning */
		double w = 0.54 - 0.46 * cos(2*M_PI*n/(order-1)); /* Hamming */
		/* double w = 0.42 - 0.5 * cos(2*M_PI*n/(order-1)) + 0.08 * cos(4*M_PI*n/(order-1)); */ /* Blackman */

		/* apply the window */
		c *= w;

		/* update the gain */
		gain += 2*c;

		/* set coefficient */
#ifdef FILTER_USE_INT
		f->xcoeffs[i] = c * (1 << FILTER_INT_FRACT);
#else
		f->xcoeffs[i] = c;
#endif
	}

	/* adjust the gain to be exact 1.0 */
	for(i=0;i<=midorder;++i) {
#ifdef FILTER_USE_INT
		f->xcoeffs[i] /= gain;
#else
		f->xcoeffs[i] = f->xcoeffs[i] * (double)(1 << FILTER_INT_FRAC) / gain;
#endif
	}

	/* decrease the order if the last coefficients are less than 1% */
	i = midorder;
#ifdef FILTER_USE_INT
	while (i > 0 && abs(f->xcoeffs[i]) * 100 < f->xcoeffs[0])
#else
	while (i > 0 && fabs(f->xcoeffs[i]) * 100 < f->xcoeffs[0])
#endif
		--i;

	f->order = i * 2 + 1;
}



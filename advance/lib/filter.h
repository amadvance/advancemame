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

/** \file
 * FIR Filter.
 * If you define USE_FILTER_INT all the computations are done using integer math.
 * Otherwise they are done with double math.
 */

#ifndef __FILTER_H
#define __FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

/** Max filter order. */
#define FILTER_ORDER_MAX 51

/** Define to use interger calculation in the filter operations. */
#define USE_FILTER_INT

/**
 * Sample type.
 */
#ifdef USE_FILTER_INT
typedef int adv_filter_real;

#define FILTER_INT_FRACT 15 /**< Fractional bits of the ::adv_filter_real type. */
#else
typedef double adv_filter_real;
#endif

/**
 * Filter definition.
 */
typedef struct adv_filter_struct {
	adv_filter_real xcoeffs[(FILTER_ORDER_MAX+1)/2]; /**< Filter coefficients. */
	unsigned order; /**< Filter order. */
} adv_filter;

/**
 * Filter state.
 */
typedef struct adv_filter_state_struct {
	unsigned prev_mac; /**< Position of the last input value inserted. */
	adv_filter_real xprev[FILTER_ORDER_MAX]; /**< Previous input value. */
} adv_filter_state;

/** \addtogroup Filter */
/*@{*/

/**
 * Get the order of the filter.
 * \param f Filter definition.
 * \return Order of the filter.
 */
unsigned adv_filter_order_get(const adv_filter* f);

/**
 * Get the output delay of the filter.
 * \param f Filter definition.
 * \return Delay in sample of the filter.
 */
unsigned adv_filter_delay_get(const adv_filter* f);

/**
 * Setup a FIR Low Pass filter.
 * The effective filter order may differ than the requested value. You must
 * read it using the adv_filter_order_get() function.
 * \param f Filter definition.
 * \param freq Cut frecuenty of the filter. 0 < freq <= 0.5.
 * \param order Order of the filter. It must be odd.
 */
void adv_filter_lpfir_set(adv_filter* f, double freq, unsigned order);

/**
 * Reset the filter state.
 * \param f Filter definition.
 * \param s Filter state.
 */
void adv_filter_state_reset(adv_filter* f, adv_filter_state* s);

/**
 * Insert a value in the filter state.
 * \param f Filter definition.
 * \param s Filter state.
 * \param x Value to insert in the state.
 */
static inline void adv_filter_insert(adv_filter* f, adv_filter_state* s, adv_filter_real x)
{
	/* next position */
	++s->prev_mac;
	if (s->prev_mac >= f->order)
		s->prev_mac = 0;

	/* set the most recent sample */
	s->xprev[s->prev_mac] = x;
}

/**
 * Compute an output sample of the filter.
 * You can start to extract data after you have inserted a number of samples
 * equal at the filter order returned by adv_filter_order_get().
 * The output delay is of (order-1)/2 samples computed by adv_filter_delay_get().
 * \param f Filter definition.
 * \param s Filter state.
 * \return Output value.
 */
adv_filter_real adv_filter_extract(adv_filter* f, adv_filter_state* s);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif

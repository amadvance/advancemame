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

/** \file
 * Filter.
 */

#ifndef __FILTER_H
#define __FILTER_H

#include "complex.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Max filter order. */
#define FILTER_ORDER_IIR_MAX 10
#define FILTER_ORDER_FIR_MAX 32

/** Max number of poles. */
#define FILTER_POLE_MAX (FILTER_ORDER_IIR_MAX*2+1)

/**
 * Sample type.
 */
typedef double adv_filter_real;

typedef enum adv_filter_type_enum {
	adv_filter_lp, /**< Low Pass. */
	adv_filter_hp, /** < High Pass. */
	adv_filter_bp, /**< Band Pass. */
	adv_filter_bs /**< Band Stop. */
} adv_filter_type;

typedef enum adv_filter_model_enum {
	adv_filter_fir_windowedsinc, /**< FIR filter. */
	adv_filter_iir_bessel, /**< IIR Bessel filter. */
	adv_filter_iir_butterworth, /**< IIR Butterworth filter. */
	adv_filter_iir_chebyshev /**< IIR Chebyshev filter. */
} adv_filter_model;

struct adv_filter_struct;
struct adv_filter_state_struct;

typedef void (*adv_filter_insert_proc)(struct adv_filter_struct* f, struct adv_filter_state_struct* s, adv_filter_real v);
typedef adv_filter_real (*adv_filter_extract_proc)(struct adv_filter_struct* f, struct adv_filter_state_struct* s);
typedef void (*adv_filter_reset_proc)(struct adv_filter_struct* f, struct adv_filter_state_struct* s);

/**
 * IIR filter definition.
 */
struct adv_filter_struct_iir {
	adv_filter_real xcoeffs[FILTER_POLE_MAX]; /**< Filter X coefficients. From x[-M] to x[0]. */
	adv_filter_real ycoeffs[FILTER_POLE_MAX]; /**< Filter Y coefficients. From y[-N] to y[-1]. */
	unsigned M, N;

	unsigned szeros_mac; /**< Number of zeros in the S plane. */
	unsigned spoles_mac; /**< Number of poles in the S plane. */

	adv_complex szeros_map[FILTER_POLE_MAX]; /**< Zeros in the S plane. */
	adv_complex spoles_map[FILTER_POLE_MAX]; /**< Poles in the S plane. */

	unsigned zzeros_mac; /**< Number of zeros in the Z plane. */
	unsigned zpoles_mac; /**< Number of poles in the Z plane. */

	adv_complex zzeros_map[FILTER_POLE_MAX]; /**< Zeros in the Z plane. */
	adv_complex zpoles_map[FILTER_POLE_MAX]; /**< Poles in the Z plane. */

	adv_filter_real gain; /**< Gain. */
};

/**
 * FIR filter definition.
 */
struct adv_filter_struct_fir {
	adv_filter_real xcoeffs[FILTER_ORDER_FIR_MAX+1]; /**< Filter X coefficients. From x[-M] to x[0]. */
	unsigned M;
};

union adv_filter_union {
	struct adv_filter_struct_iir iir;
	struct adv_filter_struct_fir fir;
};

/**
 * Generic filter definition.
 */
typedef struct adv_filter_struct {
	union adv_filter_union data;
	adv_filter_insert_proc insert;
	adv_filter_extract_proc extract;
	adv_filter_reset_proc reset;
	adv_filter_type type; /**< Filter type. */
	adv_filter_model model; /**< Filter model. */
	unsigned order; /**< Order of the filter. */
	unsigned delay; /**< Delay of the filter. */
} adv_filter;

#define FILTER_STATE_MAX (FILTER_ORDER_FIR_MAX+1)

/**
 * Filter state.
 */
typedef struct adv_filter_state_struct {
	unsigned x_mac; /**< Position of the last X value inserted. */
	unsigned y_mac; /**< Position of the last Y value inserted. */
	adv_filter_real x_map[FILTER_STATE_MAX]; /**< Previous X values. */
	adv_filter_real y_map[FILTER_STATE_MAX]; /**< Previous Y values. */
} adv_filter_state;

/** \addtogroup Filter */
/*@{*/

/**
 * Get the order of the filter.
 * \param f Filter definition.
 * \return Order of the filter.
 */
static inline unsigned adv_filter_order_get(const adv_filter* f)
{
	return f->order;
}

/**
 * Get the output delay of the filter.
 * \param f Filter definition.
 * \return Delay in sample of the filter.
 */
static inline unsigned adv_filter_delay_get(const adv_filter* f)
{
	return f->delay;
}

/**
 * Setup a filter.
 * The effective filter order may differ than the requested value. You must
 * read it using the adv_filter_order_get() function.
 * \param f Filter definition.
 * \param freq Cut frequenty of the filter. 0 < freq <= 0.5.
 * \param order Order of the filter.
 */
void adv_filter_lp_windowedsinc_set(adv_filter* f, double freq, unsigned order);
void adv_filter_lp_bessel_set(adv_filter* f, double freq, unsigned order);
void adv_filter_lp_butterworth_set(adv_filter* f, double freq, unsigned order);
void adv_filter_lp_chebyshev_set(adv_filter* f, double freq, unsigned order, double ripple);
void adv_filter_hp_bessel_set(adv_filter* f, double freq, unsigned order);
void adv_filter_hp_butterworth_set(adv_filter* f, double freq, unsigned order);
void adv_filter_hp_chebyshev_set(adv_filter* f, double freq, unsigned order, double ripple);
void adv_filter_bp_bessel_set(adv_filter* f, double freq_low, double freq_high, unsigned order);
void adv_filter_bp_butterworth_set(adv_filter* f, double freq_low, double freq_high, unsigned order);
void adv_filter_bp_chebyshev_set(adv_filter* f, double freq_low, double freq_high, unsigned order, double ripple);
void adv_filter_bs_bessel_set(adv_filter* f, double freq_low, double freq_high, unsigned order);
void adv_filter_bs_butterworth_set(adv_filter* f, double freq_low, double freq_high, unsigned order);
void adv_filter_bs_chebyshev_set(adv_filter* f, double freq_low, double freq_high, unsigned order, double ripple);

/**
 * Reset the filter state.
 * \param f Filter definition.
 * \param s Filter state.
 */
static inline void adv_filter_state_reset(adv_filter* f, adv_filter_state* s)
{
	f->reset(f, s);
}

/**
 * Insert a value in the filter state.
 * \param f Filter definition.
 * \param s Filter state.
 * \param x Value to insert in the state.
 */
static inline void adv_filter_insert(adv_filter* f, adv_filter_state* s, adv_filter_real x)
{
	f->insert(f, s, x);
}

/**
 * Compute an output sample of the filter.
 * You can start to extract data after you have inserted a number of samples
 * equal at the filter order plus one (the current input sample).
 * If you don't need the output value you can skip the call at adv_filter_extract().
 * The filter order is returned by adv_filter_order_get().
 * The output delay is returned by adv_filter_delay_get().
 * \param f Filter definition.
 * \param s Filter state.
 * \return Output value.
 */
static inline adv_filter_real adv_filter_extract(adv_filter* f, adv_filter_state* s)
{
	return f->extract(f, s);
}

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


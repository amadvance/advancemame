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

/** \file
 * DFT.
 */

#ifndef __DFT_H
#define __DFT_H

#include "extra.h"

struct adv_dft_stage_struct {
	unsigned n; /**< Size. */
	int log2n; /**< Computation of log2(n). */

	double* ss; /**< Sin table. */
	double* cc; /**< Cos table. */
	unsigned* brev; /**< Bit reversal table. */
};

struct adv_dft_struct;

typedef void (*adv_dft_execute_proc)(struct adv_dft_struct* context);

typedef struct adv_dft_struct {
	struct adv_dft_stage_struct stage;

	adv_dft_execute_proc execute;

	unsigned n;

	double* xr;
	double* xi;
} adv_dft;

/** \addtogroup DFT */
/*@{*/

adv_error adv_dft_init(adv_dft* context, unsigned n);
adv_error adv_idft_init(adv_dft* context, unsigned n);
adv_error adv_dftr_init(adv_dft* context, unsigned n);

/**
 * Get the vector of real values.
 */
static inline double* adv_dft_re_get(adv_dft* context)
{
	return context->xr;
}

/**
 * Get the vector of imagnary values.
 */
static inline double* adv_dft_im_get(adv_dft* context)
{
	return context->xi;
}

/**
 * Execute the DFT.
 * The input must be stored using the adv_dft_re_get() and
 * adv_dft_im_get() functions. The output must be read
 * using the same functions.
 */
static inline void adv_dft_execute(adv_dft* context)
{
	context->execute(context);
}

void adv_dft_free(adv_dft* context);

/*@}*/

#endif


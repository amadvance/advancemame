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
 */

/** \file
 * Complex numbers.
 */

#ifndef __COMPLEX_H
#define __COMPLEX_H

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup Complex */
/*@{*/

typedef struct adv_complex_struct {
	double re, im; 
} adv_complex;

adv_complex adv_cconj(adv_complex x);
adv_complex adv_cadd(adv_complex x, adv_complex y);
adv_complex adv_csub(adv_complex x, adv_complex y);
adv_complex adv_cneg(adv_complex z);
adv_complex adv_cmul(adv_complex x, adv_complex y);
adv_complex adv_csqr(adv_complex x);
adv_complex adv_cdiv(adv_complex x, adv_complex y);
adv_complex adv_cevaluate(adv_complex topco[], int tn, adv_complex botco[], int bn, adv_complex z);
adv_complex adv_csqrt(adv_complex x);
adv_complex adv_cexp(adv_complex x);
adv_complex adv_creal(double x);
adv_complex adv_cimag(double x);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif

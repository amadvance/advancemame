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

#include "complex.h"

adv_complex adv_cconj(adv_complex x)
{
	adv_complex z;
	z.re = x.re;
	z.im = -x.im;
	return z;
}

adv_complex adv_cadd(adv_complex x, adv_complex y)
{
	adv_complex z;
	z.re = x.re + y.re;
	z.im = x.im + y.im;
	return z;
}

adv_complex adv_cneg(adv_complex z)
{
	adv_complex cczero = { 0.0, 0.0 };
	return adv_csub(cczero, z);
}

adv_complex adv_csub(adv_complex x, adv_complex y)
{
	adv_complex z;
	z.re = x.re - y.re;
	z.im = x.im - y.im;
	return z;
}

adv_complex adv_cmul(adv_complex x, adv_complex y)
{
	adv_complex z;
	z.re = x.re * y.re - x.im * y.im;
	z.im = x.im * y.re + x.re * y.im;
	return z;
}

adv_complex adv_cdiv(adv_complex x, adv_complex y)
{
	adv_complex z;
	double mag = y.re * y.re + y.im * y.im;
	z.re = (x.re * y.re + x.im * y.im) / mag;
	z.im = (x.im * y.re - x.re * y.im) / mag;
	return z;
}

/**
 * Evaluate polynomial in z, substituting for z.
 * \return c[0] + c[1]*z^1 + c[2]+z^2 + ... + c[n]*z^n.
 */
adv_complex adv_cpolyeval(adv_complex c[], int n, adv_complex z)
{
	int i;
	adv_complex sum = { 0.0, 0.0 };
	for(i=n;i>=0;--i)
		sum = adv_cadd(adv_cmul(sum, z), c[i]);
	return sum;
}

/**
 * Evaluate response, substituting for z.
 * \return (zc[0] + zc[1]*z^1 + zc[2]+z^2 + ... + zc[m]*z^m) / (zp[0] + zp[1]*z^1 + zp[2]+z^2 + ... + zp[n]*z^n).
 */
adv_complex adv_cevaluate(adv_complex zc[], int m, adv_complex pc[], int n, adv_complex z)
{
	return adv_cdiv(adv_cpolyeval(zc, m, z), adv_cpolyeval(pc, n, z));
}

adv_complex adv_csqrt(adv_complex x)
{
	adv_complex z;
	double r = hypot(x.im, x.re);
	z.re = sqrt(0.5 * (r + x.re));
	z.im = sqrt(0.5 * (r - x.re));
	if (x.im < 0.0)
		z.im = -z.im;
	return z;
}

adv_complex adv_cexp(adv_complex x)
{
	adv_complex z;
	double r;
	r = exp(x.re);
	z.re = r * cos(x.im);
	z.im = r * sin(x.im);
	return z;
}

adv_complex adv_creal(double x)
{
	adv_complex z;
	z.re = x;
	z.im = 0;
	return z;
}

adv_complex adv_cimag(double x)
{
	adv_complex z;
	z.re = 0;
	z.im = x;
	return z;
}

adv_complex adv_csqr(adv_complex x)
{
	return adv_cmul(x, x);
}


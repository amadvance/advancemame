/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include "gtf.h"

static adv_error gtf_find_nomargin(adv_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const adv_monitor* monitor, const adv_gtf* gtf, unsigned capability, unsigned adjust)
{
	unsigned vtotal;
	double factor;

	crtc_reset(crtc);

	crtc_name_set(crtc, "generate");

	/* compute the vtotal */
	vtotal = ceil((gtf->v_min_frontporch_lines + vsize) / (1 - gtf->v_min_sync_backporch_time * vclock));
	factor = 0;

	if (crtc_find(&vtotal, &vclock, &factor, monitor, capability, adjust)!=0)
		return -1;

	/* doublescan/interlace */
	if (factor == 1.0) {
		crtc_singlescan_set(crtc);
	} else if (factor == 2.0) {
		crtc_doublescan_set(crtc);
	} else if (factor == 0.5) {
		crtc_interlace_set(crtc);
	} else
		return -1;

	/* compute the horizontal crtc */
	{
		double hclock = vclock * vtotal * factor;
		/* total = blank / duty = active / (1-duty) */
		double duty_cycle = gtf->c - (gtf->m / hclock);
		double active = hsize;
		double blank = active * duty_cycle / (1-duty_cycle);
		double sync = (active + blank) * gtf->h_sync_frac;
		double front = blank/2 - sync;

		crtc->hde = crtc_step(active, CRTC_HSTEP);
		crtc->hrs = crtc_step(active + front, CRTC_HSTEP);
		crtc->hre = crtc_step(active + front + sync, CRTC_HSTEP);
		crtc->ht = crtc_step(active + blank, CRTC_HSTEP);
	}

	/* compute the vertical crtc */
	{
		double sync_back = gtf->v_min_sync_backporch_time * vclock * vtotal;
		double active = vtotal - sync_back - gtf->v_min_frontporch_lines;
		double active_front = vtotal - sync_back;
		double active_front_sync = active_front + gtf->v_sync_lines;

		crtc->vde = crtc_step(active, CRTC_VSTEP);
		if (crtc->vde != vsize && abs(crtc->vde - vsize) <= CRTC_VSTEP)
			crtc->vde = crtc_step(vsize, CRTC_VSTEP); /* solve precision problem */
		crtc->vrs = crtc_step(active_front, CRTC_VSTEP);
		crtc->vre = crtc_step(active_front_sync, CRTC_VSTEP);
		crtc->vt = crtc_step(vtotal, CRTC_VSTEP);
	}

	/* pixelclock */
	crtc->pixelclock = vclock * factor * (crtc->vt * crtc->ht);

	/* final check, this should never fail */
	if (!crtc_clock_check(monitor, crtc))
		return -1;

	return 0;
}

adv_error gtf_find(adv_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const adv_monitor* monitor, const adv_gtf* gtf, unsigned capability, unsigned adjust)
{
	unsigned vmargin = crtc_step(vsize * gtf->margin_frac, CRTC_VSTEP);
	unsigned hmargin = crtc_step(hsize * gtf->margin_frac, CRTC_HSTEP);

	if (gtf_find_nomargin(crtc, hsize + hmargin * 2, vsize + vmargin * 2, vclock, monitor, gtf, capability, adjust) != 0) {
		return -1;
	}

	crtc->hde -= hmargin * 2;
	crtc->hrs -= hmargin;
	crtc->hre -= hmargin;

	crtc->vde -= vmargin * 2;
	crtc->vrs -= vmargin;
	crtc->vre -= vmargin;

	return 0;
}

void gtf_default_vga(adv_gtf* gtf)
{
	/* from MPGLib */
	gtf->margin_frac = 0.018;
	gtf->v_min_frontporch_lines = 1;
	gtf->v_sync_lines = 3;
	gtf->h_sync_frac = 0.08;
	gtf->v_min_sync_backporch_time = 550E-6;
	gtf->m = 3000;
	gtf->c = 0.3; /* on my system 0.26 works better */
}


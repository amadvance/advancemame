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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "portable.h"

#include "crtc.h"

#include "video.h"
#include "error.h"
#include "snstring.h"

#include <math.h>

/**
 * Find the nearest value.
 */
unsigned crtc_step(double v, unsigned st)
{
	unsigned il, ih;
	double vl = floor(v);
	il = (unsigned)vl;
	il -= il % st;
	ih = il + st;
	if (fabs(v - il) / fabs(v - ih) < (1.0 + 1e-6) ) /* the lower value are favourite */
		return il;
	else
		return ih;
}

/**
 * Adjust the clock to match the monitor specifications.
 * Only the horizontal and vertical clock limits are checked.
 * The pixel clock limits are NOT checked.
 * \return 0 if successful
 */
adv_error crtc_adjust_clock(adv_crtc* crtc, const adv_monitor* monitor)
{
	double hclock;
	double vclock;
	double factor;
	double best_hclock = 0;
	int best_hclock_found = 0;
	int i;

	factor = 1;
	if (crtc_is_interlace(crtc))
		factor *= 2;
	if (crtc_is_doublescan(crtc))
		factor /= 2;

	hclock = (double)crtc->pixelclock / crtc->ht;
	vclock = hclock / crtc->vt * factor;

	/* check if already valid */
	if (monitor_hvclock_check(monitor, hclock, vclock))
		return 0;

	/* check for every hclock limit */
	for(i=0;i<MONITOR_RANGE_MAX;++i) {
		if (monitor->hclock[i].low != 0 && monitor->hclock[i].high != 0) {
			double try_hclock, try_vclock;

			/* low limit */
			try_hclock = monitor->hclock[i].low;
			try_vclock = try_hclock / crtc->vt * factor;
			if (monitor_vclock_check(monitor, try_vclock)) {
				if (!best_hclock_found || fabs(try_hclock - hclock) < fabs(best_hclock - hclock)) {
					best_hclock = try_hclock;
					best_hclock_found = 1;
				}
			}

			/* high limit */
			try_hclock = monitor->hclock[i].high;
			try_vclock = try_hclock / crtc->vt * factor;
			if (monitor_vclock_check(monitor, try_vclock)) {
				if (!best_hclock_found || fabs(try_hclock - hclock) < fabs(best_hclock - hclock)) {
					best_hclock = try_hclock;
					best_hclock_found = 1;
				}
			}
		}
	}

	/* check for every vclock limit */
	for(i=0;i<MONITOR_RANGE_MAX;++i) {
		if (monitor->vclock[i].low != 0 && monitor->vclock[i].high != 0) {
			double try_hclock, try_vclock;

			/* low limit */
			try_vclock = monitor->vclock[i].low;
			try_hclock = try_vclock * crtc->vt / factor;
			if (monitor_hclock_check(monitor, try_hclock)) {
				if (!best_hclock_found || fabs(try_hclock - hclock) < fabs(best_hclock - hclock)) {
					best_hclock = try_hclock;
					best_hclock_found = 1;
				}
			}

			/* high limit */
			try_vclock = monitor->vclock[i].high;
			try_hclock = try_vclock * crtc->vt / factor;
			if (monitor_hclock_check(monitor, try_hclock)) {
				if (!best_hclock_found || fabs(try_hclock - hclock) < fabs(best_hclock - hclock)) {
					best_hclock = try_hclock;
					best_hclock_found = 1;
				}
			}
		}
	}

	if (best_hclock_found) {
		crtc->pixelclock = best_hclock * crtc->ht;
		return 0;
	} else {
		error_nolog_set("The video mode is incompatible with the monitor limitations");
		return -1;
	}
}

/**
 * Check if the exact mode is available.
 */
static adv_error crtc_find_exact(unsigned req_vtotal, double req_vclock, const adv_monitor* monitor)
{
	if (!monitor_hvclock_check(monitor, req_vclock * req_vtotal, req_vclock))
		return -1;
	return 0;
}

/**
 * Find the nearest vclock available.
 */
static adv_error crtc_find_nearest_vclock(unsigned req_vtotal, double* req_vclock, const adv_monitor* monitor)
{
	double vclock;
	double best_vclock = 0;
	int best_found = 0;
	int i;

	vclock = *req_vclock;

	/* check for every hclock limit */
	for(i=0;i<MONITOR_RANGE_MAX;++i) {
		if (monitor->hclock[i].low != 0 && monitor->hclock[i].high != 0) {
			double try_hclock, try_vclock;

			/* low limit */
			try_hclock = monitor->hclock[i].low;
			try_vclock = try_hclock / req_vtotal;
			if (monitor_vclock_check(monitor, try_vclock)) {
				if (!best_found || fabs(try_vclock - vclock) < fabs(best_vclock - vclock)) {
					best_vclock = try_vclock;
					best_found = 1;
				}
			}

			/* high limit */
			try_hclock = monitor->hclock[i].high;
			try_vclock = try_hclock / req_vtotal;
			if (monitor_vclock_check(monitor, try_vclock)) {
				if (!best_found || fabs(try_vclock - vclock) < fabs(best_vclock - vclock)) {
					best_vclock = try_vclock;
					best_found = 1;
				}
			}
		}
	}

	/* check for every vclock limit */
	for(i=0;i<MONITOR_RANGE_MAX;++i) {
		if (monitor->vclock[i].low != 0 && monitor->vclock[i].high != 0) {
			double try_hclock, try_vclock;

			/* low limit */
			try_vclock = monitor->vclock[i].low;
			try_hclock = try_vclock * req_vtotal;
			if (monitor_hclock_check(monitor, try_hclock)) {
				if (!best_found || fabs(try_vclock - vclock) < fabs(best_vclock - vclock)) {
					best_vclock = try_vclock;
					best_found = 1;
				}
			}

			/* high limit */
			try_vclock = monitor->vclock[i].high;
			try_hclock = try_vclock * req_vtotal;
			if (monitor_hclock_check(monitor, try_hclock)) {
				if (!best_found || fabs(try_vclock - vclock) < fabs(best_vclock - vclock)) {
					best_vclock = try_vclock;
					best_found = 1;
				}
			}
		}
	}

	if (!best_found)
		return -1;

	*req_vclock = best_vclock;
	return 0;
}

/**
 * Find the nearest vtotal available.
 */
static adv_error crtc_find_nearest_vtotal(unsigned* req_vtotal, double* req_vclock, const adv_monitor* monitor)
{
	int vtotal;
	unsigned best_vtotal = 0;
	double best_vclock = 0;
	int best_found = 0;
	int i, j;

	vtotal = *req_vtotal;

	/* check for every hclock limit */
	for(j=0;j<MONITOR_RANGE_MAX;++j) {
		if (monitor->hclock[j].low != 0 && monitor->hclock[j].high != 0) {
			/* check for every vclock limit */
			for(i=0;i<MONITOR_RANGE_MAX;++i) {
				if (monitor->vclock[i].low != 0 && monitor->vclock[i].high != 0) {
					int try_vtotal;
					double try_hclock;
					double try_vclock;

					/* low limit */
					/* the floor() approximation uses a greater vclock */
					try_vtotal = floor(monitor->hclock[j].high / monitor->vclock[i].low);
					try_hclock = monitor->hclock[j].high;
					try_vclock = try_hclock / try_vtotal;
					if (monitor_hvclock_check(monitor, try_hclock, try_vclock)) {
						if (!best_found || abs(try_vtotal - vtotal) < abs(best_vtotal - vtotal)) {
							best_vtotal = try_vtotal;
							best_vclock = try_vclock;
							best_found = 1;
						}
					}

					/* high limit */
					/* the ceil() approximation uses a lesser vclock */
					try_vtotal = ceil(monitor->hclock[j].low / monitor->vclock[i].high);
					try_hclock = monitor->hclock[j].low;
					try_vclock = try_hclock / try_vtotal;
					if (monitor_hvclock_check(monitor, try_hclock, try_vclock)) {
						if (!best_found || abs(try_vtotal - vtotal) < abs(best_vtotal - vtotal)) {
							best_vtotal = try_vtotal;
							best_vclock = try_vclock;
							best_found = 1;
						}
					}
				}
			}
		}
	}

	if (!best_found)
		return -1;

	*req_vtotal = best_vtotal;
	*req_vclock = best_vclock;
	return 0;
}


/**
 * Find the nearest vtotal available without changing the vclock
 */
static adv_error crtc_find_nearest_vtotal_fix_vclock(unsigned* req_vtotal, double req_vclock, const adv_monitor* monitor)
{
	int vtotal;
	unsigned best_vtotal = 0;
	int best_found = 0;
	int j;

	vtotal = *req_vtotal;

	/* check for every hclock limit */
	for(j=0;j<MONITOR_RANGE_MAX;++j) {
		if (monitor->hclock[j].low != 0 && monitor->hclock[j].high != 0) {
			int try_vtotal;
			double try_hclock;

			/* the floor() approximation uses a lesser hclock */
			try_vtotal = floor( monitor->hclock[j].high / req_vclock );
			try_hclock = req_vclock * try_vtotal;
			if (monitor_hvclock_check(monitor, try_hclock, req_vclock)) {
				if (!best_found || abs(try_vtotal - vtotal) < abs(best_vtotal - vtotal)) {
					best_vtotal = try_vtotal;
					best_found = 1;
				}
			}

			/* the ceil() approximation uses a greater hclock */
			try_vtotal = ceil(monitor->hclock[j].low / req_vclock);
			try_hclock = req_vclock * try_vtotal;
			if (monitor_hvclock_check(monitor, try_hclock, req_vclock)) {
				if (!best_found || abs(try_vtotal - vtotal) < abs(best_vtotal - vtotal)) {
					best_vtotal = try_vtotal;
					best_found = 1;
				}
			}
		}
	}

	if (!best_found)
		return -1;

	*req_vtotal = best_vtotal;
	return 0;
}

/**
 * Find the nearest vtotal/vclock/factor available.
 * The search order is the following:
 *   - exact if CRTC_ADJUST_EXACT
 *   - with different vclock if CRTC_ADJUST_VCLOCK
 *   - with different vtotal if CRTC_ADJUST_VTOTAL and not CRTC_ADJUST_VCLOCK
 *   - with different vtotal and vclock if if CRTC_ADJUST_VTOTAL and CRTC_ADJUST_VCLOCK
 */
adv_error crtc_find(unsigned* req_vtotal, double* req_vclock, double* req_factor, const adv_monitor* monitor, unsigned cap, unsigned adjust)
{
	int best_found = 0;
	double best_vclock = 0;
	double best_factor = 0;
	unsigned best_vtotal = 0;

        if ((adjust & CRTC_ADJUST_EXACT) != 0) {
		/* exact match */
		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN) != 0) {
			if (crtc_find_exact(*req_vtotal, *req_vclock, monitor)==0) {
				*req_factor = 1;
				return 0;
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN) != 0) {
			if (crtc_find_exact(*req_vtotal * 2, *req_vclock, monitor)==0) {
				*req_factor = 2;
				return 0;
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) != 0) {
			if (crtc_find_exact(*req_vtotal / 2, *req_vclock, monitor)==0) {
				*req_factor = 0.5;
				return 0;
			}
		}
	}

	if ((adjust & CRTC_ADJUST_VCLOCK) != 0) {
		/* best vclock */
		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN) != 0) {
			double try_vclock = *req_vclock;
			if (crtc_find_nearest_vclock(*req_vtotal, &try_vclock, monitor)==0) {
				if (!best_found || fabs(try_vclock - *req_vclock) < fabs(best_vclock - *req_vclock)) {
					best_factor = 1;
					best_vclock = try_vclock;
					best_found = 1;
				}
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN) != 0) {
			double try_vclock = *req_vclock;
			if (crtc_find_nearest_vclock(*req_vtotal * 2, &try_vclock, monitor)==0) {
				if (!best_found || fabs(try_vclock - *req_vclock) < fabs(best_vclock - *req_vclock)) {
					best_factor = 2;
					best_vclock = try_vclock;
					best_found = 1;
				}
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) != 0) {
			double try_vclock = *req_vclock;
			if (crtc_find_nearest_vclock(*req_vtotal / 2, &try_vclock, monitor)==0) {
				if (!best_found || fabs(try_vclock - *req_vclock) < fabs(best_vclock - *req_vclock)) {
					best_factor = 0.5;
					best_vclock = try_vclock;
					best_found = 1;
				}
			}
		}

		if (best_found) {
			*req_factor = best_factor;
			*req_vclock = best_vclock;
			return 0;
		}
	}

	if ((adjust & CRTC_ADJUST_VTOTAL) != 0 && (adjust & CRTC_ADJUST_VCLOCK) == 0) {
		/* best vtotal */
		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN) != 0) {
			unsigned try_vtotal = *req_vtotal;
			if (crtc_find_nearest_vtotal_fix_vclock(&try_vtotal, *req_vclock, monitor)==0) {
				if (!best_found || abs(try_vtotal - *req_vtotal) < abs(best_vtotal - *req_vtotal)) {
					best_factor = 1;
					best_vtotal = try_vtotal;
					best_found = 1;
				}
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN) != 0) {
			unsigned try_vtotal = *req_vtotal * 2;
			if (crtc_find_nearest_vtotal_fix_vclock(&try_vtotal, *req_vclock, monitor)==0) {
				if (!best_found || abs(try_vtotal - *req_vtotal) < abs(best_vtotal - *req_vtotal)) {
					best_factor = 2;
					best_vtotal = try_vtotal / 2;
					best_found = 1;
				}
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) != 0) {
			unsigned try_vtotal = *req_vtotal / 2;
			if (crtc_find_nearest_vtotal_fix_vclock(&try_vtotal, *req_vclock, monitor)==0) {
				if (!best_found || abs(try_vtotal - *req_vtotal) < abs(best_vtotal - *req_vtotal)) {
					best_factor = 0.5;
					best_vtotal = try_vtotal * 2;
					best_found = 1;
				}
			}
		}

		if (best_found) {
			*req_factor = best_factor;
			*req_vtotal = best_vtotal;
			return 0;
		}
	}

	if ((adjust & CRTC_ADJUST_VTOTAL) != 0 && (adjust & CRTC_ADJUST_VCLOCK) != 0) {
		/* best vtotal */
		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN) != 0) {
			double try_vclock = *req_vclock;
			unsigned try_vtotal = *req_vtotal;
			if (crtc_find_nearest_vtotal(&try_vtotal, &try_vclock, monitor)==0) {
				if (!best_found || abs(try_vtotal - *req_vtotal) < abs(best_vtotal - *req_vtotal)) {
					best_factor = 1;
					best_vclock = try_vclock;
					best_vtotal = try_vtotal;
					best_found = 1;
				}
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN) != 0) {
			double try_vclock = *req_vclock;
			unsigned try_vtotal = *req_vtotal * 2;
			if (crtc_find_nearest_vtotal(&try_vtotal, &try_vclock, monitor)==0) {
				if (!best_found || abs(try_vtotal - *req_vtotal) < abs(best_vtotal - *req_vtotal)) {
					best_factor = 2;
					best_vclock = try_vclock;
					best_vtotal = try_vtotal / 2;
					best_found = 1;
				}
			}
		}

		if ((cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) != 0) {
			double try_vclock = *req_vclock;
			unsigned try_vtotal = *req_vtotal / 2;
			if (crtc_find_nearest_vtotal(&try_vtotal, &try_vclock, monitor)==0) {
				if (!best_found || abs(try_vtotal - *req_vtotal) < abs(best_vtotal - *req_vtotal)) {
					best_factor = 0.5;
					best_vclock = try_vclock;
					best_vtotal = try_vtotal * 2;
					best_found = 1;
				}
			}
		}

		if (best_found) {
			*req_factor = best_factor;
			*req_vclock = best_vclock;
			*req_vtotal = best_vtotal;
			return 0;
		}
	}

	return -1;
}

/** Change the horizontal resolution. */
void crtc_hsize_set(adv_crtc* crtc, unsigned hsize)
{
	unsigned new_hde = crtc_step(hsize, CRTC_HSTEP);
	unsigned new_ht = crtc_step(crtc->ht * (double)new_hde / crtc->hde, CRTC_HSTEP);

	crtc->hrs = crtc_step(crtc->hrs * (double)new_ht / crtc->ht, CRTC_HSTEP);
	crtc->hre = crtc_step(crtc->hre * (double)new_ht / crtc->ht, CRTC_HSTEP);
	if (crtc->hrs == crtc->hre)
		crtc->hre = crtc->hrs + CRTC_HSTEP;

	/* keep the same H clock */
	crtc->pixelclock = crtc->pixelclock * (double)new_ht / crtc->ht;

	crtc->hde = new_hde;
	crtc->ht = new_ht;
}

/** Change the vertical resolution. */
void crtc_vsize_set(adv_crtc* crtc, unsigned vsize)
{
	unsigned new_vde = crtc_step(vsize, CRTC_VSTEP);
	unsigned new_vt = crtc_step(crtc->vt * (double)new_vde / crtc->vde, CRTC_VSTEP);

	crtc->vrs = crtc_step(crtc->vrs * (double)new_vt / crtc->vt, CRTC_VSTEP);
	crtc->vre = crtc_step(crtc->vre * (double)new_vt / crtc->vt, CRTC_VSTEP);
	if (crtc->vrs == crtc->vre)
		crtc->vre = crtc->vrs + CRTC_VSTEP;

	crtc->vde = new_vde;
	crtc->vt = new_vt;
}

/** Set the pixel clock. */
void crtc_pclock_set(adv_crtc* crtc, double pclock)
{
	crtc->pixelclock = pclock;
}

/** Set the horz clock. */
void crtc_hclock_set(adv_crtc* crtc, double hclock)
{
	crtc->pixelclock = hclock * crtc->ht;
}

/** Set the vert clock. */
void crtc_vclock_set(adv_crtc* crtc, double vclock)
{
	double factor = 1;
	if (crtc_is_interlace(crtc))
		factor /= 2;
	if (crtc_is_doublescan(crtc))
		factor *= 2;
	crtc->pixelclock = vclock * crtc->ht * crtc->vt * factor;
}

/**
 * Return the HClock value of the CRTC.
 * \note This value DO NOT depend on the doublescan and interlace flag
 */
double crtc_hclock_get(const adv_crtc* crtc)
{
	if (crtc_is_tvpal(crtc))
		return 15625;
	else if (crtc_is_tvntsc(crtc))
		return 15720;
	else
		return (double)crtc->pixelclock / crtc->ht;
}

/**
 * Return the VClock value of the CRTC.
 * \note This value depends on the doublescan and interlace flag
 */
double crtc_vclock_get(const adv_crtc* crtc)
{
	if (crtc_is_tvpal(crtc))
		return 50;
	else if (crtc_is_tvntsc(crtc))
		return 59.94;
	else {
		double vclock = (double)crtc->pixelclock / (crtc->ht * crtc->vt);

		if (crtc_is_interlace(crtc))
			vclock *= 2;
		if (crtc_is_doublescan(crtc))
			vclock /= 2;

		return vclock;
	}
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

/**
 * Compare two CRTC.
 * Only the effective CRTC values are compared. The name, and the other
 * user entries are not checked.
 * \return like strcmp -1, 0, 1
 */
int crtc_compare(const adv_crtc* a, const adv_crtc* b)
{
	COMPARE(a->vde, b->vde);
	COMPARE(a->hde, b->hde);
	COMPARE(crtc_is_doublescan(a), crtc_is_doublescan(b));
	COMPARE(crtc_is_interlace(a), crtc_is_interlace(b));
	COMPARE(crtc_is_singlescan(a), crtc_is_singlescan(b));
	COMPARE(crtc_is_nhsync(a), crtc_is_nhsync(b));
	COMPARE(crtc_is_phsync(a), crtc_is_phsync(b));
	COMPARE(crtc_is_nvsync(a), crtc_is_nvsync(b));
	COMPARE(crtc_is_pvsync(a), crtc_is_pvsync(b));
	COMPARE(crtc_is_tvpal(a), crtc_is_tvpal(b));
	COMPARE(crtc_is_tvntsc(a), crtc_is_tvntsc(b));
	COMPARE(crtc_is_notv(a), crtc_is_notv(b));
	COMPARE(a->vt, b->vt);
	COMPARE(a->ht, b->ht);
	COMPARE(a->hrs, b->hrs);
	COMPARE(a->hre, b->hre);
	COMPARE(a->vrs, b->vrs);
	COMPARE(a->vre, b->vre);

	return 0;
}

adv_bool crtc_clock_check(const adv_monitor* monitor, const adv_crtc* crtc)
{
	return monitor_clock_check(monitor, crtc_pclock_get(crtc), crtc_hclock_get(crtc), crtc_vclock_get(crtc));
}

int crtc_scan_get(const adv_crtc* crtc)
{
	if (crtc_is_doublescan(crtc))
		return 1;
	else if (crtc_is_interlace(crtc))
		return -1;
	else
		return 0;
}

/**
 * Reset the crtc to standard values.
 * Only the "name", "user_flags" and "container_next" value are saved.
 */
void crtc_reset(adv_crtc* crtc)
{
	crtc->flags = 0;
	crtc->pixelclock = 0;
	crtc->hde = 0;
	crtc->hrs = 0;
	crtc->hre = 0;
	crtc->ht = 0;
	crtc->vde = 0;
	crtc->vrs = 0;
	crtc->vre = 0;
	crtc->vt = 0;

	crtc_notv_set(crtc);
	crtc_singlescan_set(crtc);
	crtc_nhsync_set(crtc);
	crtc_nvsync_set(crtc);
}

/**
 * Reset the crtc to standard values.
 * Also the "name", "user_flags" and "container_next" value are cleared.
 */
void crtc_reset_all(adv_crtc* crtc)
{
	memset(crtc, 0, sizeof(adv_crtc));
	crtc_reset(crtc);
}

void crtc_name_set(adv_crtc* crtc, const char* name)
{
	sncpy(crtc->name, CRTC_NAME_MAX, name);
}

void crtc_fake_set(adv_crtc* crtc, unsigned size_x, unsigned size_y)
{
	crtc->hde = size_x;
	crtc->hrs = crtc->hde;
	crtc->hre = crtc->hde;
	crtc->ht = crtc->hde;
	crtc->vde = size_y;
	crtc->vrs = crtc->vde;
	crtc->vre = crtc->vde;
	crtc->vt = crtc->vde;
	crtc->pixelclock = 0;
	crtc->flags = 0;

	snprintf(crtc->name, CRTC_NAME_MAX, "%dx%d", size_x, size_y);
}

/**
 * Check if the crtc contains fake data.
 */
adv_bool crtc_is_fake(const adv_crtc* crtc)
{
	return crtc->pixelclock == 0;
}

/**
 * Check if the crtc contains valid data.
 */
adv_bool crtc_is_valid(const adv_crtc* crtc)
{
	return crtc->pixelclock>0
		&& crtc->hde <= crtc->hrs && crtc->hrs < crtc->hre && crtc->hre <= crtc->ht
		&& crtc->vde <= crtc->vrs && crtc->vrs < crtc->vre && crtc->vre <= crtc->vt;
}

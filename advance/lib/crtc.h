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

/** \file
 * CRTC.
 */

/** \addtogroup Crtc */
/*@{*/

#ifndef __CRTC_H
#define __CRTC_H

#include "monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \name Flags
 * CRTC flags.
 */
/*@{*/
#define CRTC_FLAGS_NHSYNC 0x1 /**< Negative horizontal sync. */
#define CRTC_FLAGS_NVSYNC 0x2 /**< Negative vertical sync. */
#define CRTC_FLAGS_DOUBLESCAN 0x4 /**< Double Scan. */
#define CRTC_FLAGS_INTERLACE 0x8 /**< Interlaced. */
/*@}*/

#define CRTC_NAME_MAX 128

typedef struct adv_crtc_struct {
	/** \name Horizontal
	 * Horizontal CRTC values.
	 * Normalized 1 unit==1 pixel
	 */
	/*@{*/
	unsigned hde, hrs, hre, ht;
	/*@}*/

	/** \name Vertical
	 * Vertical CRTC values.
	 * Normalized 1 unit==1 pixel (also for doublescan and interlace modes)
	 */
	/*@{*/
	unsigned vde, vrs, vre, vt;
	/*@}*/

	/**
	 * Pixel clock. Ignore doublescan and interlace flags.
	 * This imply that this value is doubled if doublescan is active, or
	 * halved if interlace is active.
	 */
	unsigned pixelclock;

	unsigned flags; /**< CRTC flags. */

	char name[CRTC_NAME_MAX]; /**< Name. */

	struct adv_crtc_struct* container_next; /**< Used by the container. */

	unsigned user_flags; /**< Flags for the user. */
} adv_crtc;

/* Allowed steps in CRTC values */
#define CRTC_HSTEP 16
#define CRTC_VSTEP 1

unsigned crtc_step(double v, unsigned st);

adv_error crtc_adjust_clock(adv_crtc* crtc, const adv_monitor* monitor);
adv_error crtc_adjust_size(adv_crtc* crtc, const adv_monitor* monitor);

#define CRTC_ADJUST_EXACT 0x0001 /**< Find the exact modes */
#define CRTC_ADJUST_VCLOCK 0x0002 /**< Find also modes with different vclock */
#define CRTC_ADJUST_VTOTAL 0x0004 /**< Find also modes with different vtotal */
adv_error crtc_find(unsigned* req_vtotal, double* req_vclock, double* req_factor, const adv_monitor* monitor, unsigned cap, unsigned adjust);

double crtc_hclock_get(const adv_crtc* crtc);
double crtc_vclock_get(const adv_crtc* crtc);

static inline double crtc_pclock_get(const adv_crtc* crtc)
{
	return crtc->pixelclock;
}

int crtc_scan_get(const adv_crtc* crtc);

/** Check if a CRTC specification is interlaced. */
static inline  adv_bool crtc_is_interlace(const adv_crtc* crtc)
{
	return (crtc->flags & CRTC_FLAGS_INTERLACE) != 0;
}

/** Check if a CRTC specification is doublescan. */
static inline  adv_bool crtc_is_doublescan(const adv_crtc* crtc)
{
	return (crtc->flags & CRTC_FLAGS_DOUBLESCAN) != 0;
}

/** Check if a CRTC specification is singlescan. */
static inline  adv_bool crtc_is_singlescan(const adv_crtc* crtc)
{
	return !crtc_is_doublescan(crtc) && !crtc_is_interlace(crtc);
}

/** Check if a CRTC specification is negative horizontal sync. */
static inline  adv_bool crtc_is_nhsync(const adv_crtc* crtc)
{
	return (crtc->flags & CRTC_FLAGS_NHSYNC) != 0;
}

/** Check if a CRTC specification is positive horizontal sync. */
static inline  adv_bool crtc_is_phsync(const adv_crtc* crtc)
{
	return !crtc_is_nhsync(crtc);
}

/** Check if a CRTC specification is negative vertical sync. */
static inline  adv_bool crtc_is_nvsync(const adv_crtc* crtc)
{
	return (crtc->flags & CRTC_FLAGS_NVSYNC) != 0;
}

/** Check if a CRTC specification is positive vertical sync. */
static inline  adv_bool crtc_is_pvsync(const adv_crtc* crtc)
{
	return !crtc_is_nvsync(crtc);
}

/** Return the name of the CRTC specification. */
static inline const char* crtc_name_get(const adv_crtc* crtc)
{
	return crtc->name;
}

void crtc_reset(adv_crtc* crtc);
void crtc_user_reset(adv_crtc* crtc);

/**
 * Set the specified flag in the CRTC specification.
 */
static inline void crtc_flags_set(adv_crtc* crtc, unsigned flag, unsigned mask)
{
	crtc->flags &= ~mask;
	crtc->flags |= flag;
}

static inline void crtc_nhsync_set(adv_crtc* crtc)
{
	crtc_flags_set(crtc, CRTC_FLAGS_NHSYNC, CRTC_FLAGS_NHSYNC);
}

static inline void crtc_phsync_set(adv_crtc* crtc)
{
	crtc_flags_set(crtc, 0, CRTC_FLAGS_NHSYNC);
}

static inline void crtc_nvsync_set(adv_crtc* crtc)
{
	crtc_flags_set(crtc, CRTC_FLAGS_NVSYNC, CRTC_FLAGS_NVSYNC);
}

static inline void crtc_pvsync_set(adv_crtc* crtc)
{
	crtc_flags_set(crtc, 0, CRTC_FLAGS_NVSYNC);
}

static inline void crtc_singlescan_set(adv_crtc* crtc)
{
	crtc_flags_set(crtc, 0, CRTC_FLAGS_INTERLACE | CRTC_FLAGS_DOUBLESCAN);
}

static inline void crtc_doublescan_set(adv_crtc* crtc)
{
	crtc_flags_set(crtc, CRTC_FLAGS_DOUBLESCAN, CRTC_FLAGS_INTERLACE | CRTC_FLAGS_DOUBLESCAN);
}

static inline void crtc_interlace_set(adv_crtc* crtc)
{
	crtc_flags_set(crtc, CRTC_FLAGS_INTERLACE, CRTC_FLAGS_INTERLACE | CRTC_FLAGS_DOUBLESCAN);
}

void crtc_name_set(adv_crtc* crtc, const char* name);

void crtc_vclock_set(adv_crtc* crtc, double vclock);
void crtc_hclock_set(adv_crtc* crtc, double hclock);
void crtc_pclock_set(adv_crtc* crtc, double pclock);
void crtc_hsize_set(adv_crtc* crtc, unsigned hsize);

/**
 * Return the horizontal size in pixel of the CRTC specification.
 */
static inline  unsigned crtc_hsize_get(const adv_crtc* crtc)
{
	return crtc->hde;
}

/**
 * Return the vertical size in pixel of the CRTC specification.
 */
static inline  unsigned crtc_vsize_get(const adv_crtc* crtc)
{
	return crtc->vde;
}

adv_bool crtc_clock_check(const adv_monitor* monitor, const adv_crtc* crtc);

int crtc_compare(const adv_crtc* A, const adv_crtc* B);

adv_error crtc_parse(adv_crtc* crtc, const char* begin, const char* end);
void crtc_print(char* buffer, unsigned size, const adv_crtc* crtc);

void crtc_fake_set(adv_crtc* crtc, unsigned size_x, unsigned size_y);
adv_bool crtc_is_fake(const adv_crtc* crtc);
adv_bool crtc_is_valid(const adv_crtc* crtc);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/

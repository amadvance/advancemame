/*
 * This file is part of the AdvanceMAME project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

#ifndef __CRTC_H
#define __CRTC_H

#include "monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRTC_FLAGS_NHSYNC 0x1 /**< Negative horizontal sync */
#define CRTC_FLAGS_NVSYNC 0x2 /**< Negative vertical sync */
#define CRTC_FLAGS_DOUBLESCAN 0x4
#define CRTC_FLAGS_INTERLACE 0x8
#define CRTC_FLAGS_TVPAL 0x10 /**< Video mode converted to the PAL format with an hardware scan converter */
#define CRTC_FLAGS_TVNTSC 0x20 /**< Video mode converted to the NTSC format with an hardware scan converter */

typedef struct video_crtc_struct {
	unsigned hde,hrs,hre,ht; /**< Horizontal CRTC values, normalized 1 unit==1 pixel */
	unsigned vde,vrs,vre,vt; /**< Vertical CRTC values, normalized 1 unit==1 pixel (also for doublescan and interlace modes) */

	/**
	 * Pixel clock. Ignore doublescan and interlace flags. This imply that
	 * this value is doubled if doublescan is active, or halved if interlace is active.
	 */
	unsigned pixelclock;

	unsigned flags; /**< CRTC flags */

	char name[VIDEO_NAME_MAX]; /**< Name */

	struct video_crtc_struct* container_next; /**< Used by the container */

	unsigned user_flags; /**< Flags for the user */
} video_crtc;

/* Allowed steps in CRTC values */
#define CRTC_HSTEP 8
#define CRTC_VSTEP 1

unsigned crtc_step(double v, unsigned st);

video_error crtc_adjust_clock(video_crtc* crtc, const video_monitor* monitor);

#define CRTC_ADJUST_EXACT 0x0001 /**< Find the exact modes */
#define CRTC_ADJUST_VCLOCK 0x0002 /**< Find also modes with different vclock */
#define CRTC_ADJUST_VTOTAL 0x0004 /**< Find also modes with different vtotal */
video_error crtc_find(unsigned* req_vtotal, double* req_vclock, double* req_factor, const video_monitor* monitor, unsigned cap, unsigned adjust);

double crtc_hclock_get(const video_crtc* crtc);
double crtc_vclock_get(const video_crtc* crtc);

static __inline__ double crtc_pclock_get(const video_crtc* crtc) {
	return crtc->pixelclock;
}

int crtc_scan_get(const video_crtc* crtc);

static __inline__  video_bool crtc_is_interlace(const video_crtc* crtc) {
	return (crtc->flags & CRTC_FLAGS_INTERLACE) != 0;
}

static __inline__  video_bool crtc_is_doublescan(const video_crtc* crtc) {
	return (crtc->flags & CRTC_FLAGS_DOUBLESCAN) != 0;
}

static __inline__  video_bool crtc_is_singlescan(const video_crtc* crtc) {
	return !crtc_is_doublescan(crtc) && !crtc_is_interlace(crtc);
}

static __inline__  video_bool crtc_is_tvpal(const video_crtc* crtc) {
	return (crtc->flags & CRTC_FLAGS_TVPAL) != 0;
}

static __inline__  video_bool crtc_is_tvntsc(const video_crtc* crtc) {
	return (crtc->flags & CRTC_FLAGS_TVNTSC) != 0;
}

static __inline__  video_bool crtc_is_notv(const video_crtc* crtc) {
	return !crtc_is_tvpal(crtc) && !crtc_is_tvntsc(crtc);
}

static __inline__  video_bool crtc_is_nhsync(const video_crtc* crtc) {
	return (crtc->flags & CRTC_FLAGS_NHSYNC) != 0;
}

static __inline__  video_bool crtc_is_phsync(const video_crtc* crtc) {
	return !crtc_is_nhsync(crtc);
}

static __inline__  video_bool crtc_is_nvsync(const video_crtc* crtc) {
	return (crtc->flags & CRTC_FLAGS_NVSYNC) != 0;
}

static __inline__  video_bool crtc_is_pvsync(const video_crtc* crtc) {
	return !crtc_is_nvsync(crtc);
}

static __inline const char* crtc_name_get(const video_crtc* crtc) {
	return crtc->name;
}

void crtc_reset(video_crtc* crtc);
void crtc_reset_all(video_crtc* crtc);

static __inline__ void crtc_flags_set(video_crtc* crtc, unsigned flag, unsigned mask) {
	crtc->flags &= ~mask;
	crtc->flags |= flag;
}

static __inline__ void crtc_nhsync_set(video_crtc* crtc) {
	crtc_flags_set(crtc, CRTC_FLAGS_NHSYNC, CRTC_FLAGS_NHSYNC);
}

static __inline__ void crtc_phsync_set(video_crtc* crtc) {
	crtc_flags_set(crtc, 0, CRTC_FLAGS_NHSYNC);
}

static __inline__ void crtc_nvsync_set(video_crtc* crtc) {
	crtc_flags_set(crtc, CRTC_FLAGS_NVSYNC, CRTC_FLAGS_NVSYNC);
}

static __inline__ void crtc_pvsync_set(video_crtc* crtc) {
	crtc_flags_set(crtc, 0, CRTC_FLAGS_NVSYNC);
}

static __inline__ void crtc_singlescan_set(video_crtc* crtc) {
	crtc_flags_set(crtc, 0, CRTC_FLAGS_INTERLACE | CRTC_FLAGS_DOUBLESCAN);
}

static __inline__ void crtc_doublescan_set(video_crtc* crtc) {
	crtc_flags_set(crtc, CRTC_FLAGS_DOUBLESCAN, CRTC_FLAGS_INTERLACE | CRTC_FLAGS_DOUBLESCAN);
}

static __inline__ void crtc_interlace_set(video_crtc* crtc) {
	crtc_flags_set(crtc, CRTC_FLAGS_INTERLACE, CRTC_FLAGS_INTERLACE | CRTC_FLAGS_DOUBLESCAN);
}

static __inline__ void crtc_notv_set(video_crtc* crtc) {
	crtc_flags_set(crtc, 0, CRTC_FLAGS_TVPAL | CRTC_FLAGS_TVNTSC);
}

static __inline__ void crtc_tvpal_set(video_crtc* crtc) {
	crtc_flags_set(crtc, CRTC_FLAGS_TVPAL, CRTC_FLAGS_TVPAL | CRTC_FLAGS_TVNTSC);
}

static __inline__ void crtc_tvntsc_set(video_crtc* crtc) {
	crtc_flags_set(crtc, CRTC_FLAGS_TVNTSC, CRTC_FLAGS_TVPAL | CRTC_FLAGS_TVNTSC);
}

void crtc_name_set(video_crtc* crtc, const char* name);

void crtc_vclock_set(video_crtc* crtc, double vclock);
void crtc_hclock_set(video_crtc* crtc, double hclock);
void crtc_pclock_set(video_crtc* crtc, double pclock);
void crtc_hsize_set(video_crtc* crtc, unsigned hsize);
void crtc_vsize_set(video_crtc* crtc, unsigned vsize);

static __inline__  unsigned crtc_hsize_get(const video_crtc* crtc) {
	return crtc->hde;
}

static __inline__  unsigned crtc_vsize_get(const video_crtc* crtc) {
	return crtc->vde;
}

video_bool crtc_clock_check(const video_monitor* monitor, const video_crtc* crtc);

int video_crtc_compare(const video_crtc* A, const video_crtc* B);

video_error video_crtc_parse(video_crtc* crtc, const char* begin, const char* end);
void video_crtc_print(char* buffer, const video_crtc* crtc);

void crtc_fake_set(video_crtc* crtc, unsigned size_x, unsigned size_y);

#ifdef __cplusplus
}
#endif

#endif

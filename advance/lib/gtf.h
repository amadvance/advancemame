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
 * CRTC generation with the GTF.
 */

/** \addtogroup Generate */
/*@{*/

#ifndef __GTF_H
#define __GTF_H

#include "video.h"
#include "crtc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adv_gtf_struct {
	double margin_frac; /* margin as fract of active */
	unsigned v_min_frontporch_lines; /* minimum front porch in lines */
	unsigned v_sync_lines; /* width of V sync in lines */
	double h_sync_frac; /* width of H sync as factor of total */
	double v_min_sync_backporch_time; /* minimum vertical sync + back time (seconds) */
	double m; /* blanking formula gradient */
	double c; /* blanking formula offset */
} adv_gtf;

#define GTF_ADJUST_EXACT CRTC_ADJUST_EXACT
#define GTF_ADJUST_VCLOCK CRTC_ADJUST_VCLOCK
#define GTF_ADJUST_VTOTAL CRTC_ADJUST_VTOTAL
adv_error gtf_find(adv_crtc* crtc, unsigned hsize, unsigned vsize, double vclock, const adv_monitor* monitor, const adv_gtf* gtf, unsigned capability, unsigned adjust);

void gtf_default_vga(adv_gtf* gtf);

adv_error gtf_parse(adv_gtf* gtf, const char* g);
adv_error gtf_load(adv_conf* context, adv_gtf* gtf);
void gtf_save(adv_conf* context, const adv_gtf* gtf);
void gtf_clear(adv_conf* context);
void gtf_register(adv_conf* context);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/

/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2008 Andrea Mazzoleni
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

#include "segment.h"

#include "log.h"

#ifdef USE_SEGMENT
void segment_set(adv_segment* s, unsigned sl, int sdp, unsigned sbpp, unsigned dbpp, unsigned run)
{
	unsigned len;
	unsigned rest;

	len = sl * sbpp;

	s->cps = run / sbpp;
	s->sdps = sdp * s->cps;
	s->dbps = s->cps * dbpp;

	assert(run % sbpp == 0);

	s->count = len / run;
	rest = len % run;

	if (rest) {
		++s->count;
		s->cls = rest / sbpp;
	} else {
		s->cls = s->cps;
	}

	log_std(("blit: segment sl:%d,sbpp:%d,dbpp:%d,run:%d -> sdps:%d,cps:%d,dbps:%d,count:%d,cls:%d\n", sl, sbpp, dbpp, run, s->sdps, s->cps, s->dbps, s->count, s->cls));
}

void segment_one(adv_segment* s, unsigned sl)
{
	s->count = 1;
	s->sdps = 0;
	s->dbps = 0;
	s->cps = 0;
	s->cls = sl;

	log_std(("blit: segment one -> cls:%d\n", sl));
}
#endif

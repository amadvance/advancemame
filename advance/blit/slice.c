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

#include "slice.h"

void slice_set(adv_slice* slice, unsigned sd, unsigned dd)
{
	if (sd < dd) {
		/* expansion */
		slice->whole = dd / sd;
		slice->up = (dd % sd) * 2;
		slice->down = sd * 2;
		slice->error = 0;
		slice->count = sd;
	} else if (sd == dd) {
		slice->whole = 1;
		slice->up = 0;
		slice->down = 0;
		slice->error = 0;
		slice->count = sd;
	} else {
		/* reduction */
		--sd;
		--dd;
		slice->whole = sd / dd;
		slice->up = (sd % dd) * 2;
		slice->down = dd * 2;
		slice->error = 0;
		slice->count = dd + 1;
	}
}

void slice_vector(unsigned* map, unsigned sd, unsigned dd)
{
	adv_slice slice;
	unsigned i, j;

	slice_set(&slice, sd, dd);

	if (sd < dd) {
		/* expansion */
		unsigned count = slice.count;
		int error = slice.error;
		i = 0;
		j = 0;
		while (count) {
			unsigned run = slice.whole;
			if ((error += slice.up) > 0) {
				++run;
				error -= slice.down;
			}

			while (run) {
				map[i] = j;
				++i;
				--run;
			}
			++j;

			--count;
		}
	} else {
		/* reduction */
		unsigned count = slice.count;
		int error = slice.error;
		i = 0;
		j = 0;
		while (count) {
			unsigned run = slice.whole;
			if ((error += slice.up) > 0) {
				++run;
				error -= slice.down;
			}

			map[i] = j;
			++i;
			j += run;

			--count;
		}
	}
}


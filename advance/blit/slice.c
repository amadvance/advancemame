/*
 * This file is part of the Advance project.
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

#include "slice.h"

void slice_set(adv_slice* slice, unsigned sd, unsigned dd) {
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

void slice_vector(unsigned* map, unsigned sd, unsigned dd) {
	adv_slice slice;
	unsigned i,j;

	slice_set(&slice, sd, dd);

	if (sd < dd) {
		/* expansion */
		int count = slice.count;
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
		int count = slice.count;
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


#if 0

/* Run-length slice test program */

#include <stdio.h>

struct run_slice {
	int whole;
	int up;
	int down;
	int error_t;
	int count;
};

void run_slice_init(struct run_slice* r, int S, int D) {
	if (S > D) {
		/* reduction */
		--S;
		--D;
		r->whole = S / D;
		r->up = (S % D) * 2;
		r->down = D * 2;
		r->error_t = 0;
		r->count = D + 1;
	} else {
		/* expansion */
		r->whole = D / S;
		r->up = (D % S) * 2;
		r->down = S * 2;
		r->error_t = 0;
		r->count = S;
	}
}

#define TEST

void test_red(int S, int D) {
	int total = 0;
	int step = 0;
	struct run_slice r;
	int error_t;
	int count;

	if (S <= D)
		return;

	run_slice_init(&r,S,D);

	error_t = r.error_t;
	count = r.count;

	while (count) {
		unsigned run = r.whole;

		if ((error_t += r.up) > 0) {
			++run;
			error_t -= r.down;
		}
		if (count == 1)
			total += 1;
		else
			total += run;
		++step;
		--count;
	}

	if (total != S || step != D)
		printf("error_t: tot %d, stp %d, src %d, dst %d\n",total,step,S,D);
}

void test_exp(int S, int D) {
	int total = 0;
	int step = 0;
	struct run_slice r;
	int error_t;
	int count;

	if (S >= D)
		return;

	run_slice_init(&r,S,D);

	error_t = r.error_t;
	count = r.count;

	while (count) {
		unsigned run = r.whole;

		if ((error_t += r.up) > 0) {
			++run;
			error_t -= r.down;
		}
		total += run;
		++step;
		--count;
	}

	if (total != D || step != S)
		printf("error_t: tot %d, stp %d, src %d, dst %d\n",total,step,S,D);
}

int main() {
	int i,j;
	for(i=2;i<3000;++i)
		for(j=2;j<3000;++j) {
			if (i > j)
				test_red(i,j);
			if (i < j)
				test_exp(i,j);
		}
	return 0;
}

#endif

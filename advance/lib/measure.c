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
 */

#include "target.h"
#include "log.h"

#include <stdlib.h>

static int double_cmp(const void* _a, const void* _b)
{
	const double* a = (const double*)_a;
	const double* b = (const double*)_b;
	if (*a < *b)
		return -1;
	if (*a > *b)
		return 1;
	return 0;
}

#define MEASURE_COUNT 7

/**
 * Measure the time beetween two event.
 * \param wait Function used to wait.
 * \param low Low limit time in second.
 * \param high High limit time in second.
 * \param count Number of measure to try.
 * \return
 *   - ==0 Error in the measure.
 *   - !=0 Time in second of the event.
 */
double measure_step(void (*wait)(void), double low, double high, unsigned count)
{
	double map[MEASURE_COUNT];
	target_clock_t start, stop;
	unsigned map_start, map_end;
	unsigned median;
	unsigned i;
	double error;

	if (count == 0 || count > MEASURE_COUNT)
		count = MEASURE_COUNT;

	low *= TARGET_CLOCKS_PER_SEC;
	high *= TARGET_CLOCKS_PER_SEC;

	i = 0;
	wait();
	start = target_clock();
	while (i < count) {
		wait();
		stop = target_clock();
		map[i] = stop - start;
		start = stop;
		++i;
	}

	qsort(map, count, sizeof(double), double_cmp);

	map_start = 0;
	map_end = count;

	/* reject low values */
	while (map_start < map_end && map[map_start] <= low)
		++map_start;

	/* reject high values */
	while (map_start < map_end && map[map_end-1] >= high)
		--map_end;

	if (map_start == map_end) {
		log_std(("advance: measure time failed, return 0\n"));
		return 0;
	}

	median = map_start + (map_end - map_start) / 2; /* the median */

	for(i=map_start;i<map_end;++i) {
		log_std(("advance: measured time %g (1/%g)\n", map[i] / TARGET_CLOCKS_PER_SEC, TARGET_CLOCKS_PER_SEC / map[i]));
	}

	if (map[map_start])
		error = (map[map_end - 1] - map[map_start]) / map[map_start];
	else
		error = 0;

	log_std(("advance: median time %g (1/%g) (err %g%%)\n", map[median] / TARGET_CLOCKS_PER_SEC, TARGET_CLOCKS_PER_SEC / map[median], error * 100.0));

	return map[median] / TARGET_CLOCKS_PER_SEC;
}

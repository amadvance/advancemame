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

#include "fuzzy.h"

/**
 * Check if "a" is a ordered subset of "b".
 * \param a - sub string
 * \param b - long string
 * \param upper_limit - maximum value returned
 * \param almostone - almost one char of the "a" string was processed
 * \param penality - current penality for a skipped "b" char
 * \return the FUZZY match, lower is better
 */
static int fuzzy_internal(const char* a, const char* b, int* bs, int upper_limit, int almostone, int penality)
{
	if (upper_limit <= 0) {
		return upper_limit;
	} else if (!*a) {
		return 0;
	} else if (!*b) {
		return strlen(a) * FUZZY_UNIT_A;
	} else if (*a == *b) {
		int missing = fuzzy_internal(a+1, b+1, bs+1, upper_limit, 1, *bs != 0);
		if (almostone && penality)
			missing += FUZZY_UNIT_B;
		return missing;
	} else {
		int missing_skip;
		int missing_next = fuzzy_internal(a, b+1, bs+1, upper_limit, almostone, 1);
		if (missing_next < upper_limit)
			upper_limit = missing_next;
		missing_skip = FUZZY_UNIT_A + fuzzy_internal(a+1, b, bs, upper_limit - FUZZY_UNIT_A, almostone, penality);
		if (missing_skip < missing_next)
			return missing_skip;
		else
			return missing_next;
	}
}

/**
 * Check if "a" is a ordered subset of "b"
 * Examples: "A string", "B string" -> penality
 * "123", "xxx1xxx23xxx" -> 1*UNIT_B
 * "123", "xxx1xxx2xxxxx3xxx" -> 2*UNIT_B
 * "123", "xxx1xxx2xxxxx" -> 1*UNIT_B+1*UNIT_A
 * "123", "xxx1xxx3xxxxx" -> 1*UNIT_B+1*UNIT_A
 * "123", "xxx1xxx" -> 2*UNIT_A
 * In case of multiple match the function returns the minimum penality value.
 * \param a Sub string.
 * \param b Long string.
 * \param upper_limit Maximum value returned.
 * \return The penality value as sums of ::FUZZY_UNIT_A and ::FUZZY_UNIT_B. Lower means less differences.
 */
int fuzzy(const char* a, const char* b, int upper_limit)
{
	char B[256];
	char A[256];
	char AA[256];
	char BB[256];
	int BBS[256+1]; /* counter of the skipped B char */
	int skip;
	char* aa;
	char* bb;
	int* bbs;
	unsigned i;

	/* convert in upper case */
	aa = A;
	for(i=0;i<sizeof(A)-1;++i)
		*aa++ = toupper(a[i]);
	*aa = 0;
	bb = B;
	for(i=0;i<sizeof(B)-1 && b[i]!='(' && b[i]!='[';++i) /* remove some string part */
		*bb++ = toupper(b[i]);
	*bb = 0;

	/* remove unused char */
	skip = 0;
	a = A;
	aa = AA;
	while (*a) {
		if (strchr(B, *a))
			*aa++ = *a;
		else
			skip += FUZZY_UNIT_A;
		++a;
	}
	*aa = 0;
	b = B;
	bb = BB;
	bbs = BBS;
	*bbs = 0;
	while (*b) {
		if (strchr(A, *b)) {
			*bb++ = *b;
			++bbs;
			*bbs = 0;
		} else {
			*bbs = 1;
		}
		++b;
	}
	*bb = 0;

	return skip + fuzzy_internal(AA, BB, BBS+1, upper_limit - skip, 0, 0);
}

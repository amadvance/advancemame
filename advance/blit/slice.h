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
 * Slice.
 */

/** \addtogroup Blit */
/*@{*/

#ifndef __SLICE_H
#define __SLICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run-length slice context.
 * This context must be initialized with the video_slice_init() function.
 */
typedef struct adv_slice_struct  {
	unsigned whole; /**< Segment length. */
	int up; /**< Up step. */
	int down; /**< Down step. */
	unsigned count; /**< Number of segment. */
	int error; /**< Start error. */
} adv_slice;

/**
 * Initialize the run-length slice algoritm.
 * This is a modification of the Bresenhams run-length slice algoritm
 * adapted to work with bitmap resize.
 * Refere to Dr. Dobb's Journal Nov 1992 "The good, the bad, and the run-sliced"
 * by Michael Abrash.
 *
 * The basic use is:
\code
	while (count) {
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		... draw a segment of "run" pixels in expansion, or use a delta
		... of "run" pixels in reduction

		--count;
	}
\endcode
 * \param slice Slice context to initialize.
 * \param sd Source length.
 * \param dd Destination length.
 */
void slice_set(adv_slice* slice, unsigned sd, unsigned dd);

/**
 * Compute a vector of offsets.
 * \param map Destination of the offsets. It must have dd elements.
 * \param sd Source length.
 * \param dd Dest length.
 */
void slice_vector(unsigned* map, unsigned sd, unsigned dd);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2008 Andrea Mazzoleni
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

#ifndef __VCOPY_H
#define __VCOPY_H

#include "blit.h"

/****************************************************************************/
/* copy8 */

static void video_stage_copy8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_copy, sdx, sdp, 1, sdx, 1);

	stage->put_plain = BLITTER(video_line_stretchx8_11_step1);
	if (sdp == 1)
		stage->put = BLITTER(video_line_stretchx8_11_step1);
	else if (sdp == 2)
		stage->put = BLITTER(video_line_stretchx8_11_step2);
	else
		stage->put = BLITTER(video_line_stretchx8_11);
}

/****************************************************************************/
/* copy16 */

static void video_stage_copy16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_copy, sdx, sdp, 2, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_stretchx16_11_step2), BLITTER(video_line_stretchx16_11));
}

/****************************************************************************/
/* copy32 */

static void video_stage_copy32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_copy, sdx, sdp, 4, sdx, 4);

	stage->put_plain = BLITTER(video_line_stretchx32_11_step4);
	if (sdp == 4)
		stage->put = BLITTER(video_line_stretchx32_11_step4);
	else
		stage->put = BLITTER(video_line_stretchx32_11);
}

#endif


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

#ifndef __SEGMENT_H
#define __SEGMENT_H

/**
 * If defined the segment support is enabled.
 * This support allow to blit only a part of a row, increasing the cache locality of the blit process.
 */
#define USE_SEGMENT

#define SEGMENT_FLAG_FIRST 0x0001 /**< First segment of a line. */
#define SEGMENT_FLAG_LAST 0x0002 /**< Last segment of a line. */
#define SEGMENT_FLAG_SINGLE (SEGMENT_FLAG_FIRST | SEGMENT_FLAG_LAST)

#ifdef USE_SEGMENT

typedef struct adv_segment_struct {
	unsigned count; /**< Number of segments. */
	int sdps; /**< Source delta per segment. It can be negative for rotations.  */
	unsigned dbps; /**< Destination bytes per segment. */
	unsigned cps; /**< Count per segment. */
	unsigned cls; /**< Count last segment. */
} adv_segment;

void segment_set(adv_segment* s, unsigned sl, int sdp, unsigned sbpp, unsigned dbpp, unsigned run);
void segment_one(adv_segment* s, unsigned sl);

static inline void* segment_src(const adv_segment* s, unsigned pos, const void* ptr)
{
	if (pos > s->count)
		return (void*)ptr;
	else
		return (unsigned char*)ptr + pos * s->sdps;
}

static inline void* segment_mid(const adv_segment* s, unsigned pos, void* ptr)
{
	if (pos > s->count)
		return ptr;
	else
		return (unsigned char*)ptr + pos * (s->dbps / 2);
}

static inline void* segment_dst(const adv_segment* s, unsigned pos, void* ptr)
{
	if (pos > s->count)
		return ptr;
	else
		return (unsigned char*)ptr + pos * s->dbps;
}

static inline unsigned segment_count(const adv_segment* s, unsigned pos)
{
	if (pos + 1 == s->count)
		return s->cls;
	else if (pos > s->count)
		return (s->count - 1) * s->cps + s->cls;
	else
		return s->cps;
}

static inline unsigned segment_flag(const adv_segment* s, unsigned pos)
{
	unsigned flag;

	if (pos > s->count)
		return SEGMENT_FLAG_FIRST | SEGMENT_FLAG_LAST;

	flag = 0;
	if (pos == 0)
		flag |= SEGMENT_FLAG_FIRST;
	if (pos + 1 == s->count)
		flag |= SEGMENT_FLAG_LAST;
	return flag;
}

#define SEGSRC(src) segment_src(&stage->segment, pos, src)
#define SEGMID(mid) segment_mid(&stage->segment, pos, mid)
#define SEGDST(dst) segment_dst(&stage->segment, pos, dst)
#define SEGCNT() segment_count(&stage->segment, pos)
#define SEGWID() segment_count(&stage->segment, pos)
#define SEGFLG() segment_flag(&stage->segment, pos)
#define SEGSET(stage, sl, sdp, sbpp, dbpp, run) segment_set(&stage->segment, sl, sdp, sbpp, dbpp, run)
#define SEGONE(stage, sl) segment_one(&stage->segment, sl)
#define SEGFOR(stage) for(pos=0;pos<stage->segment.count;++pos)

#else

#define SEGSRC(src) (src)
#define SEGMID(mid) (mid)
#define SEGDST(dst) (dst)
#define SEGCNT() (stage->slice.count)
#define SEGWID() (stage->sdx)
#define SEGFLG() (SEGMENT_FLAG_FIRST | SEGMENT_FLAG_LAST)
#define SEGSET(stage, sl, sdp, sbpp, dbpp, run) do { } while (0)
#define SEGONE(stage, sl) do { } while (0)
#define SEGFOR(stage)

#endif

#endif


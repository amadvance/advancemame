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

#include "clear.h"

#include "icommon.h"

#include <assert.h>
#include <stdlib.h>

/***************************************************************************/
/* Clear */

/* Clear screen
 * in:
 *   x,y destination pos
 *   src data source
 */
static void video_chained8_clear(unsigned x, unsigned y, unsigned dx, unsigned dy, uint8 src) {
	unsigned j;
	unsigned pre_count;
	unsigned post_count;
	unsigned inner_count;
	unsigned mask;

	src &= 0xFF;
	mask = src | ((unsigned)src << 8) | ((unsigned)src << 16) | ((unsigned)src << 24);

	pre_count = (4 - x) & 3;
	if (pre_count >= dx) pre_count = dx;
	inner_count = (dx - pre_count) / 4;
	post_count = dx - pre_count - inner_count * 4;

	for(j=0;j<dy;++j) {
		void* dst = video_write_line(y) + x;
		unsigned i;
		for(i=pre_count;i;--i) {
			P8DER0(dst) = mask;
			PADD(dst,1);
		}
		for(i=inner_count;i;--i) {
			P32DER0(dst) = mask;
			PADD(dst,4);
		}
		for(i=post_count;i;--i) {
			P8DER0(dst) = mask;
			PADD(dst,1);
		}
		++y;
	}
}

static void video_chained16_clear(unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned src) {
	unsigned j;
	unsigned pre_count;
	unsigned post_count;
	unsigned inner_count;
	unsigned mask;

	src &= 0xFFFF;
	mask = src | (src << 16);

	pre_count = (2 - x) & 1;
	if (pre_count >= dx) pre_count = dx;
	inner_count = (dx - pre_count) / 2;
	post_count = dx - pre_count - inner_count * 2;

	for(j=0;j<dy;++j) {
		void* dst = video_write_line(y) + 2*x;
		unsigned i;
		if (pre_count) {
			P8DER0(dst) = mask;
			P8DER(dst,1) = mask >> 8;
			PADD(dst,2);
		}
		for(i=inner_count;i;--i) {
			P32DER0(dst) = mask;
			PADD(dst,4);
		}
		if (post_count) {
			P8DER0(dst) = mask;
			P8DER(dst,1) = mask >> 8;
			/* dst += 2; */
		}
		++y;
	}
}

static void video_chained24_clear(unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned src) {
	unsigned j;
	unsigned inner_count;
	uint8 mask0;
	uint8 mask1;
	uint8 mask2;

	mask0 = src & 0xFF;
	mask1 = (src >> 8) & 0xFF;
	mask2 = (src >> 16) & 0xFF;
	inner_count = dx;

	for(j=0;j<dy;++j) {
		void* dst = video_write_line(y) + 4*x;
		unsigned i;
		for(i=inner_count;i;--i) {
			P8DER0(dst) = mask0;
			P8DER(dst,1) = mask1;
			P8DER(dst,2) = mask2;
			PADD(dst,3);
		}
		++y;
	}
}

static void video_chained32_clear(unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned src) {
	unsigned j;
	unsigned inner_count;
	unsigned mask;

	mask = src;
	inner_count = dx;

	for(j=0;j<dy;++j) {
		void* dst = video_write_line(y) + 4*x;
		unsigned i;
		for(i=inner_count;i;--i) {
			P32DER0(dst) = mask;
			PADD(dst,4);
		}
		++y;
	}
}

static void video_unchained8_clear(unsigned x, unsigned y, unsigned dx, unsigned dy, uint8 src) {
	unsigned p;
	uint32 mask;

	src &= 0xFF;
	mask = src | (src << 8) | (src << 16) | (src << 24);

	for(p=0;p<4;++p) {
		unsigned j;

		unsigned p_x;
		unsigned p_dx;
		unsigned pre_count;
		unsigned post_count;
		unsigned inner_count;

		p_x = x / 4;
		p_dx = (x + dx - 1) / 4 - p_x + 1;
		if ((x + dx - 1) % 4 < p)
			--p_dx;
		if (x % 4 > p) {
			--p_dx;
			++p_x;
		}

		pre_count = (4 - p_x) & 3;
		if (pre_count >= p_dx) pre_count = p_dx;
		inner_count = (p_dx - pre_count) / 4;
		post_count = p_dx - pre_count - inner_count * 4;

		video_unchained_plane_set(p);
		for(j=0;j<dy;++j) {
			void* dst = video_write_line(y+j) + p_x;
			unsigned i;
			for(i=pre_count;i;--i) {
				P8DER0(dst) = mask;
				PADD(dst,1);
			}
			for(i=inner_count;i;--i) {
				P32DER0(dst) = mask;
				PADD(dst,4);
			}
			for(i=post_count;i;--i) {
				P8DER0(dst) = mask;
				PADD(dst,1);
			}
		}
	}
}

/* Clear the screen */
void video_clear(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, unsigned src) {
	switch (video_bytes_per_pixel()) {
		case 1 :
			if (video_is_unchained())
				video_unchained8_clear(dst_x,dst_y,dst_dx,dst_dy,src);
			else
				video_chained8_clear(dst_x,dst_y,dst_dx,dst_dy,src);
			break;
		case 2 :
			video_chained16_clear(dst_x,dst_y,dst_dx,dst_dy,src);
			break;
		case 3 :
			video_chained24_clear(dst_x,dst_y,dst_dx,dst_dy,src);
			break;
		case 4 :
			video_chained32_clear(dst_x,dst_y,dst_dx,dst_dy,src);
			break;
	}
}

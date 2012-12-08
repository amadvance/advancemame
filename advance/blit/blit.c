/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2008 Andrea Mazzoleni
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

#include "blit.h"
#include "log.h"
#include "error.h"
#include "endianrw.h"

/***************************************************************************/
/* mmx */

#if defined(USE_ASM_INLINE)

static void blit_cpuid(unsigned level, unsigned* regs)
{
	__asm__ __volatile__(
		"pushal\n"
		".byte 0x0F, 0xA2\n"
		"movl %%eax, (%1)\n"
		"movl %%ebx, 4(%1)\n"
		"movl %%ecx, 8(%1)\n"
		"movl %%edx, 12(%1)\n"
		"popal\n"
		:
		: "a" (level), "D" (regs)
		: "cc", "memory"
	);
}

static void blit_has_capability(adv_bool* mmx, adv_bool* sse)
{
	unsigned regs[4];
	unsigned a, b;

	*mmx = 0;
	*sse = 0;

	__asm__ __volatile__(
		"pushfl\n"
		"pushfl\n"
		"popl %0\n"
		"movl %0, %1\n"
		"xorl $0x200000, %0\n"
		"pushl %0\n"
		"popfl\n"
		"pushfl\n"
		"popl %0\n"
		"popfl"
		: "=r" (a), "=r" (b)
		:
		: "cc"
	);

	if (a == b) {
		log_std(("blit: no cpuid\n"));
		return; /* no cpuid */
	}

	blit_cpuid(0, regs);
	if (regs[0] > 0) {
		blit_cpuid(1, regs);
		if ((regs[3] & 0x800000) != 0) {
			*mmx = 1;
			if ((regs[3] & 0x2000000) != 0) {
				*sse = 1;
			}
		}
	}
}

/* Support both the conditions: MMX present or not */
adv_bool the_blit_mmx = 0;
adv_bool the_blit_sse = 0;
adv_bool the_blit_direct = 0;

#define BLITTER(name) (the_blit_mmx ? name##_mmx : name##_def)

static adv_error blit_cpu(void)
{
	blit_has_capability(&the_blit_mmx, &the_blit_sse);

	return 0;
}

static inline void internal_end(void)
{
	if (the_blit_mmx) {
		__asm__ __volatile__ (
			"emms"
		);
	}
}

#else

/* Assume that MMX is NOT present. */

#define the_blit_mmx 0
#define the_blit_sse 0

#define BLITTER(name) (name##_def)

static adv_error blit_cpu(void)
{
	return 0;
}

static inline void internal_end(void)
{
}

#endif

/***************************************************************************/
/* internal */

#include "icopy.h"
#include "idouble.h"
#include "imax.h"
#include "imean.h"
#include "irgb.h"
#include "icconv.h"

#ifndef USE_BLIT_TINY
#include "scale2x.h"
#include "scale3x.h"
#include "scale2k.h"
#include "scale3k.h"
#include "scale4k.h"
#include "lq2x.h"
#include "lq2x3.h"
#include "lq2x4.h"
#include "lq3x.h"
#include "lq4x.h"
#ifndef USE_BLIT_SMALL
#include "hq2x.h"
#include "hq2x3.h"
#include "hq2x4.h"
#include "hq3x.h"
#include "hq4x.h"
#include "xbr2x.h"
#include "xbr3x.h"
#include "xbr4x.h"
#endif
#endif

/***************************************************************************/
/* video stage */

#define STAGE_SIZE(stage, _type, _sdx, _sdp, _sbpp, _ddx, _dbpp) \
	do { \
		stage->type = (_type); \
		stage->sdx = (_sdx); \
		stage->ddx = (_ddx); \
		stage->sdp = (_sdp); \
		stage->sbpp = (_sbpp); \
		stage->dbpp = (_dbpp); \
		slice_set(&stage->slice, (_sdx), (_ddx)); \
		stage->palette = 0; \
		stage->buffer_size = (_dbpp)*(_ddx); \
		stage->buffer_extra_size = 0; \
		stage->put_plain = 0; \
		stage->put = 0; \
	} while (0)

#define STAGE_TYPE(stage, _type) \
	do { \
		stage->type = (_type); \
	} while (0)

#define STAGE_PUT(stage, _put_plain, _put) \
	do { \
		stage->put_plain = (_put_plain); \
		stage->put = (stage->sbpp == stage->sdp) ? stage->put_plain : (_put); \
	} while (0)

#define STAGE_EXTRA(stage) \
	do { \
		stage->buffer_extra_size = stage->buffer_size; \
	} while (0)

#define STAGE_PALETTE(stage, _palette) \
	do { \
		stage->palette = _palette; \
	} while (0)

#define STAGE_CONVERSION(stage, _sdef, _ddef) \
	do { \
		union adv_color_def_union tmp_sdef; \
		union adv_color_def_union tmp_ddef; \
		tmp_sdef.ordinal = (_sdef); \
		tmp_ddef.ordinal = (_ddef); \
		stage->red_shift = rgb_conv_shift_get(tmp_sdef.nibble.red_len, tmp_sdef.nibble.red_pos, tmp_ddef.nibble.red_len, tmp_ddef.nibble.red_pos); \
		stage->red_mask = rgb_conv_mask_get(tmp_sdef.nibble.red_len, tmp_sdef.nibble.red_pos, tmp_ddef.nibble.red_len, tmp_ddef.nibble.red_pos); \
		stage->green_shift = rgb_conv_shift_get(tmp_sdef.nibble.green_len, tmp_sdef.nibble.green_pos, tmp_ddef.nibble.green_len, tmp_ddef.nibble.green_pos); \
		stage->green_mask = rgb_conv_mask_get(tmp_sdef.nibble.green_len, tmp_sdef.nibble.green_pos, tmp_ddef.nibble.green_len, tmp_ddef.nibble.green_pos); \
		stage->blue_shift = rgb_conv_shift_get(tmp_sdef.nibble.blue_len, tmp_sdef.nibble.blue_pos, tmp_ddef.nibble.blue_len, tmp_ddef.nibble.blue_pos); \
		stage->blue_mask = rgb_conv_mask_get(tmp_sdef.nibble.blue_len, tmp_sdef.nibble.blue_pos, tmp_ddef.nibble.blue_len, tmp_ddef.nibble.blue_pos); \
		stage->ssp = color_def_bytes_per_pixel_get(tmp_sdef.ordinal); \
		stage->dsp = color_def_bytes_per_pixel_get(tmp_ddef.ordinal); \
	} while (0)

#include "vstretch.h"
#include "vmax.h"
#include "vmean.h"
#include "vcopy.h"
#include "vrot.h"
#include "vswap.h"
#include "vfilter.h"
#include "vconv.h"
#include "vpalette.h"

#ifndef USE_BLIT_TINY
#include "vrgb.h"
#endif

/***************************************************************************/
/* fast_buffer */

/* A very fast dynamic buffers allocations */

/* Max number of allocable buffers */
#define FAST_BUFFER_MAX 128

/* Total size of the buffers */
#define FAST_BUFFER_SIZE (256*1024)

/* Align */
#define FAST_BUFFER_ALIGN 8

void* fast_buffer; /* raw pointer */
void* fast_buffer_aligned; /* aligned pointer */
unsigned fast_buffer_map[FAST_BUFFER_MAX]; /* stack of incremental size used */
unsigned fast_buffer_mac; /* top of the stack */

static void* video_buffer_alloc(unsigned size)
{
	unsigned size_aligned = ALIGN_UNSIGNED(size, FAST_BUFFER_ALIGN);

	assert(fast_buffer_mac < FAST_BUFFER_MAX);

	if (fast_buffer_map[fast_buffer_mac] + size_aligned > FAST_BUFFER_SIZE - FAST_BUFFER_ALIGN) {
		log_std(("ERROR:blit: out of memory\n"));
		return 0;
	}

	++fast_buffer_mac;
	fast_buffer_map[fast_buffer_mac] = fast_buffer_map[fast_buffer_mac-1] + size_aligned;

	return (uint8*)fast_buffer_aligned + fast_buffer_map[fast_buffer_mac-1];
}

/* Buffers must be allocated and freed in exact reverse order */
static void video_buffer_free(void* buffer)
{
	(void)buffer;
	assert(fast_buffer_mac != 0);
	--fast_buffer_mac;
}

/* Debug version of the alloc functions */
#ifndef NDEBUG

#define WRAP_SIZE 32

static void* video_buffer_alloc_wrap(unsigned size)
{
	uint8* buffer8 = (uint8*)video_buffer_alloc(size + WRAP_SIZE);
	unsigned i;
	for(i=0;i<WRAP_SIZE;++i)
		buffer8[i] = i;
	return buffer8 + WRAP_SIZE;
}

static void video_buffer_free_wrap(void* buffer)
{
	uint8* buffer8 = (uint8*)buffer - WRAP_SIZE;
	unsigned i;
	for(i=0;i<WRAP_SIZE;++i)
		assert(buffer8[i] == i);
	video_buffer_free(buffer8);
}

#define video_buffer_free video_buffer_free_wrap
#define video_buffer_alloc video_buffer_alloc_wrap

#endif

static void video_buffer_init(void)
{
	fast_buffer = malloc(FAST_BUFFER_SIZE + FAST_BUFFER_ALIGN);
	fast_buffer_aligned = ALIGN_PTR(fast_buffer, FAST_BUFFER_ALIGN);
	fast_buffer_mac = 0;
	fast_buffer_map[0] = 0;
}

static void video_buffer_done(void)
{
	assert(fast_buffer_mac == 0);
	free(fast_buffer);
}

/***************************************************************************/
/* init/done */

adv_error video_blit_init(void)
{
	if (blit_cpu() != 0) {
		error_set("This executable requires an MMX processor.\n");
		return -1;
	}

	video_buffer_init();

	return 0;
}

void video_blit_done(void)
{
	video_buffer_done();
}

/***************************************************************************/
/* stage helper */

static inline void stage_copy(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	if ((int)stage->sbpp == stage->sdp) {
		BLITTER(internal_copy8)(dst, src, stage->sdx * stage->sbpp);
	} else {
		switch (stage->sbpp) {
		case 1 : BLITTER(internal_copy8_step)(dst, src, stage->sdx, stage->sdp); break;
		case 2 : BLITTER(internal_copy16_step)(dst, src, stage->sdx, stage->sdp); break;
		case 4 : BLITTER(internal_copy32_step)(dst, src, stage->sdx, stage->sdp); break;
		}
	}
}

static void stage_max_vert_self(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	if ((int)stage->sbpp == stage->sdp) {
		switch (stage->sbpp) {
		case 1 : internal_max8_vert_self(dst, src, stage->sdx); break;
		case 2 : internal_max16_vert_self(dst, src, stage->sdx); break;
		case 4 : internal_max32_vert_self(dst, src, stage->sdx); break;
		}
	} else {
		switch (stage->sbpp) {
		case 1 : internal_max8_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 2 : internal_max16_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 4 : internal_max32_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		}
	}
}

static void stage_max_rgb_vert_self(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	if ((int)stage->sbpp == stage->sdp) {
		switch (stage->sbpp) {
		case 1 : internal_max_rgb8_vert_self(dst, src, stage->sdx); break;
		case 2 : internal_max_rgb16_vert_self(dst, src, stage->sdx); break;
		case 4 : internal_max_rgb32_vert_self(dst, src, stage->sdx); break;
		}
	} else {
		switch (stage->sbpp) {
		case 1 : internal_max_rgb8_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 2 : internal_max_rgb16_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 4 : internal_max_rgb32_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		}
	}
}

static void stage_min_vert_self(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	if ((int)stage->sbpp == stage->sdp) {
		switch (stage->sbpp) {
		case 1 : internal_min8_vert_self(dst, src, stage->sdx); break;
		case 2 : internal_min16_vert_self(dst, src, stage->sdx); break;
		case 4 : internal_min32_vert_self(dst, src, stage->sdx); break;
		}
	} else {
		switch (stage->sbpp) {
		case 1 : internal_min8_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 2 : internal_min16_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 4 : internal_min32_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		}
	}
}

static void stage_min_rgb_vert_self(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	if ((int)stage->sbpp == stage->sdp) {
		switch (stage->sbpp) {
		case 1 : internal_min_rgb8_vert_self(dst, src, stage->sdx); break;
		case 2 : internal_min_rgb16_vert_self(dst, src, stage->sdx); break;
		case 4 : internal_min_rgb32_vert_self(dst, src, stage->sdx); break;
		}
	} else {
		switch (stage->sbpp) {
		case 1 : internal_min_rgb8_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 2 : internal_min_rgb16_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 4 : internal_min_rgb32_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		}
	}
}

static void stage_mean_vert_self(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	if ((int)stage->sbpp == stage->sdp) {
		switch (stage->sbpp) {
		case 1 : BLITTER(internal_mean8_vert_self)(dst, src, stage->sdx); break;
		case 2 : BLITTER(internal_mean16_vert_self)(dst, src, stage->sdx); break;
		case 4 : BLITTER(internal_mean32_vert_self)(dst, src, stage->sdx); break;
		}
	} else {
		switch (stage->sbpp) {
		case 1 : internal_mean8_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 2 : internal_mean16_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		case 4 : internal_mean32_vert_self_step(dst, src, stage->sdx, stage->sdp); break;
		}
	}
}

#ifndef USE_BLIT_TINY
static inline void scale2x(void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned bytes_per_pixel, unsigned count)
{
	switch (bytes_per_pixel) {
	case 1 : BLITTER(scale2x_8)(dst0, dst1, src0, src1, src2, count); break;
	case 2 : BLITTER(scale2x_16)(dst0, dst1, src0, src1, src2, count); break;
	case 4 : BLITTER(scale2x_32)(dst0, dst1, src0, src1, src2, count); break;
	}
}

static inline void scale2x3(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned bytes_per_pixel, unsigned count)
{
	switch (bytes_per_pixel) {
	case 1 : BLITTER(scale2x3_8)(dst0, dst1, dst2, src0, src1, src2, count); break;
	case 2 : BLITTER(scale2x3_16)(dst0, dst1, dst2, src0, src1, src2, count); break;
	case 4 : BLITTER(scale2x3_32)(dst0, dst1, dst2, src0, src1, src2, count); break;
	}
}

static inline void scale2x4(void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned bytes_per_pixel, unsigned count)
{
	switch (bytes_per_pixel) {
	case 1 : BLITTER(scale2x4_8)(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case 2 : BLITTER(scale2x4_16)(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case 4 : BLITTER(scale2x4_32)(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	}
}

static inline void scale2k(void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : scale2k_16_def(dst0, dst1, src0, src1, src2, count); break;
	case INTERP_32 : scale2k_32_def(dst0, dst1, src0, src1, src2, count); break;
	case INTERP_YUY2 : scale2k_yuy2_def(dst0, dst1, src0, src1, src2, count); break;
	}
}

static inline void lq2x(void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : lq2x_16_def(dst0, dst1, src0, src1, src2, count); break;
	case INTERP_32 : lq2x_32_def(dst0, dst1, src0, src1, src2, count); break;
	case INTERP_YUY2 : lq2x_yuy2_def(dst0, dst1, src0, src1, src2, count); break;
	}
}

static inline void lq2x3(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : lq2x3_16_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_32 : lq2x3_32_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_YUY2 : lq2x3_yuy2_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	}
}

static inline void lq2x4(void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : lq2x4_16_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_32 : lq2x4_32_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_YUY2 : lq2x4_yuy2_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	}
}

#ifndef USE_BLIT_SMALL
static inline void hq2x(void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : hq2x_16_def(dst0, dst1, src0, src1, src2, count); break;
	case INTERP_32 : hq2x_32_def(dst0, dst1, src0, src1, src2, count); break;
	case INTERP_YUY2 : hq2x_yuy2_def(dst0, dst1, src0, src1, src2, count); break;
	}
}

static inline void hq2x3(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : hq2x3_16_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_32 : hq2x3_32_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_YUY2 : hq2x3_yuy2_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	}
}

static inline void hq2x4(void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : hq2x4_16_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_32 : hq2x4_32_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_YUY2 : hq2x4_yuy2_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	}
}

static inline void xbr2x(void* dst0, void* dst1, void* src0, void* src1, void* src2, void* src3, void* src4, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : xbr2x_16_def(dst0, dst1, src0, src1, src2, src3, src4, count); break;
	case INTERP_32 : xbr2x_32_def(dst0, dst1, src0, src1, src2, src3, src4, count); break;
	case INTERP_YUY2 : xbr2x_yuy2_def(dst0, dst1, src0, src1, src2, src3, src4, count); break;
	}
}
#endif

static inline void scale3x(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned bytes_per_pixel, unsigned count)
{
	switch (bytes_per_pixel) {
	case 1 : scale3x_8_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case 2 : scale3x_16_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case 4 : scale3x_32_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	}
}

static inline void scale3k(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : scale3k_16_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_32 : scale3k_32_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_YUY2 : scale3k_yuy2_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	}
}

static inline void lq3x(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : lq3x_16_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_32 : lq3x_32_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_YUY2 : lq3x_yuy2_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	}
}

#ifndef USE_BLIT_SMALL
static inline void hq3x(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : hq3x_16_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_32 : hq3x_32_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	case INTERP_YUY2 : hq3x_yuy2_def(dst0, dst1, dst2, src0, src1, src2, count); break;
	}
}

static inline void xbr3x(void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, void* src3, void* src4, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : xbr3x_16_def(dst0, dst1, dst2, src0, src1, src2, src3, src4, count); break;
	case INTERP_32 : xbr3x_32_def(dst0, dst1, dst2, src0, src1, src2, src3, src4, count); break;
	case INTERP_YUY2 : xbr3x_yuy2_def(dst0, dst1, dst2, src0, src1, src2, src3, src4, count); break;
	}
}
#endif

static inline void scale4k(void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : scale4k_16_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_32 : scale4k_32_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_YUY2 : scale4k_yuy2_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	}
}

static inline void lq4x(void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : lq4x_16_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_32 : lq4x_32_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_YUY2 : lq4x_yuy2_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	}
}

#ifndef USE_BLIT_SMALL
static inline void hq4x(void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : hq4x_16_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_32 : hq4x_32_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	case INTERP_YUY2 : hq4x_yuy2_def(dst0, dst1, dst2, dst3, src0, src1, src2, count); break;
	}
}

static inline void xbr4x(void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, void* src3, void* src4, unsigned interp, unsigned count)
{
	switch (interp) {
	case INTERP_16 : xbr4x_16_def(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count); break;
	case INTERP_32 : xbr4x_32_def(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count); break;
	case INTERP_YUY2 : xbr4x_yuy2_def(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count); break;
	}
}
#endif

static inline void stage_scale2x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned pos)
{
	scale2x(dst0, dst1, src0, src1, src2, stage->bpp, stage->sdx);
}

static inline void stage_scale2x3(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned pos)
{
	scale2x3(dst0, dst1, dst2, src0, src1, src2, stage->bpp, stage->sdx);
}

static inline void stage_scale2x4(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned pos)
{
	scale2x4(dst0, dst1, dst2, dst3, src0, src1, src2, stage->bpp, stage->sdx);
}

static inline void stage_scale2k(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned pos)
{
	scale2k(dst0, dst1, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_lq2x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned pos)
{
	lq2x(dst0, dst1, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_lq2x3(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned pos)
{
	lq2x3(dst0, dst1, dst2, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_lq2x4(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned pos)
{
	lq2x4(dst0, dst1, dst2, dst3, src0, src1, src2, stage->interp, stage->sdx);
}

#ifndef USE_BLIT_SMALL
static inline void stage_hq2x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned pos)
{
	hq2x(dst0, dst1, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_hq2x3(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned pos)
{
	hq2x3(dst0, dst1, dst2, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_hq2x4(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned pos)
{
	hq2x4(dst0, dst1, dst2, dst3, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_xbr2x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2, void* src3, void* src4, unsigned pos)
{
	xbr2x(dst0, dst1, src0, src1, src2, src3, src4, stage->interp, stage->sdx);
}
#endif

static inline void stage_scale3x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned pos)
{
	scale3x(dst0, dst1, dst2, src0, src1, src2, stage->bpp, stage->sdx);
}

static inline void stage_scale3k(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned pos)
{
	scale3k(dst0, dst1, dst2, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_lq3x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned pos)
{
	lq3x(dst0, dst1, dst2, src0, src1, src2, stage->interp, stage->sdx);
}

#ifndef USE_BLIT_SMALL
static inline void stage_hq3x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, unsigned pos)
{
	hq3x(dst0, dst1, dst2, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_xbr3x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* src0, void* src1, void* src2, void* src3, void* src4, unsigned pos)
{
	xbr3x(dst0, dst1, dst2, src0, src1, src2, src3, src4, stage->interp, stage->sdx);
}
#endif

static inline void stage_scale4m(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2, unsigned pos)
{
	scale2x(dst0, dst1, src0, src1, src2, stage->bpp, stage->sdx);
}

static inline void stage_scale4x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, void* src3, unsigned pos)
{
	scale2x(dst0, dst1, src0, src1, src2, stage->bpp, 2 * stage->sdx);
	scale2x(dst2, dst3, src1, src2, src3, stage->bpp, 2 * stage->sdx);
}

static inline void stage_scale4k(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned pos)
{
	scale4k(dst0, dst1, dst2, dst3, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_lq4x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned pos)
{
	lq4x(dst0, dst1, dst2, dst3, src0, src1, src2, stage->interp, stage->sdx);
}

#ifndef USE_BLIT_SMALL
static inline void stage_hq4x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, unsigned pos)
{
	hq4x(dst0, dst1, dst2, dst3, src0, src1, src2, stage->interp, stage->sdx);
}

static inline void stage_xbr4x(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* dst2, void* dst3, void* src0, void* src1, void* src2, void* src3, void* src4, unsigned pos)
{
	xbr4x(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, stage->interp, stage->sdx);
}
#endif
#endif

/***************************************************************************/
/* stage/pipeline */

static unsigned char* video_line(const struct video_pipeline_target_struct* target, unsigned y)
{
	return video_write_line(y);
}

static unsigned char* memory_line(const struct video_pipeline_target_struct* target, unsigned y)
{
	return (unsigned char*)target->ptr + y * target->bytes_per_scanline;
}

void video_pipeline_init(struct video_pipeline_struct* pipeline)
{
	unsigned i;

	pipeline->stage_mac = 0;
	pipeline->target.line = &video_line;
	pipeline->target.ptr = 0;
	pipeline->target.color_def = video_color_def();
	pipeline->target.bytes_per_pixel = color_def_bytes_per_pixel_get(video_color_def());
	pipeline->target.bytes_per_scanline = video_bytes_per_scanline();
}

void video_pipeline_target(struct video_pipeline_struct* pipeline, void* ptr, unsigned bytes_per_scanline, adv_color_def def)
{
	pipeline->target.line = &memory_line;
	pipeline->target.ptr = ptr;
	pipeline->target.color_def = def;
	pipeline->target.bytes_per_pixel = color_def_bytes_per_pixel_get(def);
	pipeline->target.bytes_per_scanline = bytes_per_scanline;
}

void video_pipeline_done(struct video_pipeline_struct* pipeline)
{
	int i;

	if (pipeline->stage_mac) {
		/* deallocate with the same allocation order */
		for(i=pipeline->stage_mac-1;i>=0;--i) {
			struct video_stage_horz_struct* stage = &pipeline->stage_map[i];
			if (stage->buffer_extra)
				video_buffer_free(stage->buffer_extra);
		}
		for(i=pipeline->stage_mac-1;i>=0;--i) {
			struct video_stage_horz_struct* stage = &pipeline->stage_map[i];
			if (stage->buffer)
				video_buffer_free(stage->buffer);
		}
	}
}

static inline struct video_stage_horz_struct* video_pipeline_begin_mutable(struct video_pipeline_struct* pipeline)
{
	return pipeline->stage_map;
}

static inline struct video_stage_horz_struct* video_pipeline_end_mutable(struct video_pipeline_struct* pipeline)
{
	return pipeline->stage_map + pipeline->stage_mac;
}

static inline struct video_stage_vert_struct* video_pipeline_vert_mutable(struct video_pipeline_struct* pipeline)
{
	return &pipeline->stage_vert;
}

static inline struct video_stage_horz_struct* video_pipeline_insert(struct video_pipeline_struct* pipeline)
{
	struct video_stage_horz_struct* stage = video_pipeline_end_mutable(pipeline);
	++pipeline->stage_mac;
	return stage;
}

static void video_pipeline_realize(struct video_pipeline_struct* pipeline, unsigned sdx, unsigned ddx, unsigned dbpp, unsigned combine)
{
	struct video_stage_vert_struct* stage_vert = video_pipeline_vert_mutable(pipeline);
	struct video_stage_horz_struct* stage_begin = video_pipeline_begin_mutable(pipeline);
	struct video_stage_horz_struct* stage_end = video_pipeline_end_mutable(pipeline);
	struct video_stage_horz_struct* stage;

	/* adjust vert stage */
	if (stage_begin == stage_end) {
		stage_vert->sdx = sdx;
		stage_vert->ddx = ddx;
		stage_vert->bpp = dbpp;
	} else if (stage_vert->stage_pivot == stage_end) {
		stage_vert->sdx = stage_vert->stage_pivot[-1].ddx;
		stage_vert->ddx = ddx;
		stage_vert->bpp = dbpp;
	} else if (stage_vert->stage_pivot == stage_begin) {
		stage_vert->sdx = sdx;
		stage_vert->ddx = stage_vert->stage_pivot->sdx;
		stage_vert->bpp = stage_vert->stage_pivot->sbpp;
	} else {
		stage_vert->sdx = stage_vert->stage_pivot[-1].ddx;
		stage_vert->ddx = stage_vert->stage_pivot->sdx;
		stage_vert->bpp = stage_vert->stage_pivot->sbpp;
	}

	/* allocate buffers */
	stage = stage_begin;
	while (stage != stage_end) {
		if (stage->buffer_size) {
			stage->buffer = video_buffer_alloc(stage->buffer_size);
		} else {
			stage->buffer = 0;
		}

		++stage;
	}

	/* allocate the extra buffer */
	stage = stage_begin;
	while (stage != stage_end) {
		if (stage->buffer_extra_size) {
			stage->buffer_extra = video_buffer_alloc(stage->buffer_extra_size);
		} else {
			stage->buffer_extra = 0;
		}

		++stage;
	}
}

/* Run a partial pipeline (all except the last stage) and store the result in the specified buffer */
static void* video_pipeline_run_partial(void* dst_buffer, const struct video_stage_horz_struct* stage, const struct video_stage_horz_struct* stage_end, unsigned line, const void* src, unsigned pos)
{
	if (stage == stage_end) {
		return (void*)src;
	} else {
		const struct video_stage_horz_struct* stage_next = stage + 1;
		if (stage_next == stage_end) {
			if (!dst_buffer)
				dst_buffer = stage->buffer;

			stage->put(stage, line, dst_buffer, src, stage->slice.count);
		} else {
			stage->put(stage, line, stage->buffer, src, stage->slice.count);
			++stage;
			++stage_next;

			while (stage_next != stage_end) {
				stage->put(stage, line, stage->buffer, stage[-1].buffer, stage->slice.count);
				++stage;
				++stage_next;
			}

			if (!dst_buffer)
				dst_buffer = stage->buffer;

			stage->put(stage, line, dst_buffer, stage[-1].buffer, stage->slice.count);
		}

		return dst_buffer;
	}
}

static void video_pipeline_run(const struct video_stage_horz_struct* stage, const struct video_stage_horz_struct* stage_end, unsigned line, void* dst, const void* src, unsigned pos)
{
	--stage_end;
	if (stage == stage_end) {
		stage->put(stage, line, dst, src, stage->slice.count);
	} else {
		stage->put(stage, line, stage->buffer, src, stage->slice.count);
		++stage;

		while (stage != stage_end) {
			stage->put(stage, line, stage->buffer, stage[-1].buffer, stage->slice.count);
			++stage;
		}

		stage->put(stage, line, dst, stage[-1].buffer, stage->slice.count);
	}
}

static void video_pipeline_run_plain(const struct video_stage_horz_struct* stage, const struct video_stage_horz_struct* stage_end, unsigned line, void* dst, void* src, unsigned pos)
{
	--stage_end;
	if (stage == stage_end) {
		stage->put_plain(stage, line, dst, src, stage->slice.count);
	} else {
		stage->put_plain(stage, line, stage->buffer, src, stage->slice.count);
		++stage;

		while (stage != stage_end) {
			stage->put(stage, line, stage->buffer, stage[-1].buffer, stage->slice.count);
			++stage;
		}

		stage->put(stage, line, dst, stage[-1].buffer, stage->slice.count);
	}
}

static inline void video_pipeline_vert_run(const struct video_pipeline_struct* pipeline, unsigned x, unsigned y, const void* src)
{
	/* draw */
	video_pipeline_vert(pipeline)->put(&pipeline->target, video_pipeline_vert(pipeline), x, y, src);

	/* restore the MMX micro state */
	internal_end();
}

const char* pipe_name(enum video_stage_enum pipe)
{
	switch (pipe) {
		case pipe_x_stretch : return "hstretch";
		case pipe_x_maxmin : return "hmax";
		case pipe_x_mean : return "hmean";
		case pipe_x_double : return "hcopy2x";
		case pipe_x_triple : return "hcopy3x";
		case pipe_x_quadruple : return "hcopy4x";
		case pipe_x_filter : return "hfilter";
		case pipe_x_copy : return "hcopy";
		case pipe_rotation : return "rotation";
		case pipe_x_rgb_triad3pix : return "rgb 3";
		case pipe_x_rgb_triad6pix : return "rgb 6";
		case pipe_x_rgb_triad16pix : return "rgb 16";
		case pipe_x_rgb_triadstrong3pix : return "rgb strong 3";
		case pipe_x_rgb_triadstrong6pix : return "rgb strong 6";
		case pipe_x_rgb_triadstrong16pix : return "rgb strong 16";
		case pipe_x_rgb_scandoublehorz : return "hscanline x2";
		case pipe_x_rgb_scantriplehorz : return "hscanline x3";
		case pipe_x_rgb_scandoublevert : return "vscanline x2";
		case pipe_x_rgb_scantriplevert : return "vscanline x3";
		case pipe_swap_even : return "swap even";
		case pipe_swap_odd : return "swap odd";
		case pipe_interlace_filter : return "vfilter";
		case pipe_palette8to8 : return "palette 8>8";
		case pipe_palette8to16 : return "palette 8>16";
		case pipe_palette8to32 : return "palette 8>32";
		case pipe_palette16to8 : return "palette 16>8";
		case pipe_palette16to16 : return "palette 16>16";
		case pipe_palette16to32 : return "palette 16>32";
		case pipe_imm16to8 : return "conv 16>8";
		case pipe_imm16to32 : return "conv 16>32";
		case pipe_bgra8888tobgr332 : return "bgra 8888>bgr 332";
		case pipe_bgra8888tobgr565 : return "bgra 8888>bgr 565";
		case pipe_bgra8888tobgra5551 : return "bgra 8888>bgra 5551";
		case pipe_bgra8888toyuy2 : return "bgra 8888>yuy2";
		case pipe_bgra5551tobgr332 : return "bgra 5551>bgr 332";
		case pipe_bgra5551tobgr565 : return "bgra 5551>bgr 565";
		case pipe_bgra5551tobgra8888 : return "bgra 5551>bgra 8888";
		case pipe_bgra5551toyuy2 : return "bgra 5551>yuy2";
		case pipe_rgb888tobgra8888 : return "rgb 888>bgra 8888";
		case pipe_rgba8888tobgra8888 : return "rgba 8888>bgra 8888";
		case pipe_bgr888tobgra8888 : return "bgr 888>bgra 8888";
		case pipe_rgbtorgb : return "rgb>rgb";
		case pipe_rgbtoyuy2 : return "rgb>yuy2";
		case pipe_y_copy : return "vstretch";
		case pipe_y_mean : return "vmean";
		case pipe_y_filter : return "vlowpass";
		case pipe_y_maxmin : return "vmaxmin";
		case pipe_y_scale2x : return "scale2x";
		case pipe_y_scale2x3 : return "scale2x3";
		case pipe_y_scale2x4 : return "scale2x4";
		case pipe_y_scale3x : return "scale3x";
		case pipe_y_scale4x : return "scale4x";
		case pipe_y_scale2k : return "scale2k";
		case pipe_y_scale3k : return "scale3k";
		case pipe_y_scale4k : return "scale4k";
		case pipe_y_lq2x : return "lq2x";
		case pipe_y_lq2x3 : return "lq2x3";
		case pipe_y_lq2x4 : return "lq2x4";
		case pipe_y_lq3x : return "lq3x";
		case pipe_y_lq4x : return "lq4x";
#ifndef USE_BLIT_SMALL
		case pipe_y_hq2x : return "hq2x";
		case pipe_y_hq2x3 : return "hq2x3";
		case pipe_y_hq2x4 : return "hq2x4";
		case pipe_y_hq3x : return "hq3x";
		case pipe_y_hq4x : return "hq4x";
		case pipe_y_xbr2x : return "xbr2x";
		case pipe_y_xbr3x : return "xbr3x";
		case pipe_y_xbr4x : return "xbr4x";
#endif
	}
	return 0;
}

/* Check is the stage change the color format */
/* These stages MUST be BEFORE any RGB color operation */
static adv_bool pipe_is_conversion(enum video_stage_enum pipe)
{
	switch (pipe) {
		case pipe_palette8to8 :
		case pipe_palette8to16 :
		case pipe_palette8to32 :
		case pipe_palette16to8 :
		case pipe_palette16to16 :
		case pipe_palette16to32 :
		case pipe_imm16to8 :
		case pipe_imm16to32 :
		case pipe_bgra8888tobgr332 :
		case pipe_bgra8888tobgr565 :
		case pipe_bgra8888tobgra5551 :
		case pipe_bgra8888toyuy2 :
		case pipe_bgra5551tobgr332 :
		case pipe_bgra5551tobgr565 :
		case pipe_bgra5551tobgra8888 :
		case pipe_bgra5551toyuy2 :
		case pipe_rgb888tobgra8888 :
		case pipe_rgba8888tobgra8888 :
		case pipe_bgr888tobgra8888 :
		case pipe_rgbtorgb :
		case pipe_rgbtoyuy2 :
			return 1;
		default:
			return 0;
	}
}

/* Check is the stage decorate the image */
/* These stages MUST be AFTER any change of size */
static adv_bool pipe_is_decoration(enum video_stage_enum pipe)
{
	switch (pipe) {
		case pipe_x_rgb_triad3pix :
		case pipe_x_rgb_triad6pix :
		case pipe_x_rgb_triad16pix :
		case pipe_x_rgb_triadstrong3pix :
		case pipe_x_rgb_triadstrong6pix :
		case pipe_x_rgb_triadstrong16pix :
		case pipe_x_rgb_scandoublehorz :
		case pipe_x_rgb_scantriplehorz :
		case pipe_x_rgb_scandoublevert :
		case pipe_x_rgb_scantriplevert :
		case pipe_swap_even :
		case pipe_swap_odd :
		case pipe_interlace_filter :
			return 1;
		default:
			return 0;
	}
}

/* Check if the write operation is done converting the RGB/YUV values and cannot work with a palette mode */
static adv_bool combine_is_rgb(unsigned combine)
{
	switch (combine & VIDEO_COMBINE_Y_MASK) {
	case VIDEO_COMBINE_Y_MEAN :
	case VIDEO_COMBINE_Y_FILTER :
#ifndef USE_BLIT_TINY
	case VIDEO_COMBINE_Y_SCALEK :
	case VIDEO_COMBINE_Y_LQ :
#ifndef USE_BLIT_SMALL
	case VIDEO_COMBINE_Y_HQ :
	case VIDEO_COMBINE_Y_XBR :
#endif
#endif
		return 1;
	default:
		return 0;
	}
}

/* Check if the write operation support direct writing without requiring a final stage */
static adv_bool combine_is_direct(unsigned combine)
{
	if ((combine & VIDEO_COMBINE_BUFFER) != 0)
		return 0;

	switch (combine & VIDEO_COMBINE_Y_MASK) {
#ifndef USE_BLIT_TINY
	case VIDEO_COMBINE_Y_SCALEX :
	case VIDEO_COMBINE_Y_SCALEK :
	case VIDEO_COMBINE_Y_LQ :
#ifndef USE_BLIT_SMALL
	case VIDEO_COMBINE_Y_HQ :
	case VIDEO_COMBINE_Y_XBR :
#endif
		return 1;
#endif
	default:
		return 0;
	}
}

/***************************************************************************/
/* stretchy reduction */

/* This example explains the difference of the effects in reduction

  ORI   COPY  MEAN  FILTER
  A     A     A+B+C A         (first line)
  B     D     D+E   C+D
  C     F     F+G+H E+F
  D     I     I+J   H+I
  ...

*/

static void video_stage_stretchy_x1(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;

	while (count) {
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = target->line(target, y) + x_off;
		video_pipeline_run(stage_vert->stage_begin, stage_vert->stage_end, line++, dst, src, -1);
		++y;

		PADD(src, stage_vert->sdw * run);
		--count;
	}
}

static void video_stage_stretchy_max_x1(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_vert->stage_begin->sdx * stage_vert->stage_begin->sbpp);

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;

	while (count) {
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = target->line(target, y) + x_off;

		if (count == 1)
			run = 1;
		if (run == 1) {
			video_pipeline_run(stage_begin, stage_end, line++, dst, src, -1);
			PADD(src, stage_vert->sdw);
		} else {
			void* src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
			stage_copy(stage_pivot, buffer, src_buffer);
			PADD(src, stage_vert->sdw);
			--run;

			while (run) {
				src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
				if (color_def_type_get(target->color_def) == adv_color_type_rgb)
					stage_max_rgb_vert_self(stage_pivot, buffer, src_buffer);
				else
					stage_max_vert_self(stage_pivot, buffer, src_buffer);
				PADD(src, stage_vert->sdw);
				--run;
			}

			video_pipeline_run_plain(stage_vert->stage_pivot, stage_end, line++, dst, buffer, -1);
		}
		++y;
		--count;
	}

	video_buffer_free(buffer);
}

/* Compute the mean of every lines reduced to a single line */
static void video_stage_stretchy_mean_x1(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;

	while (count) {
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = target->line(target, y) + x_off;

		if (count == 1)
			run = 1;
		if (run == 1) {
			video_pipeline_run(stage_begin, stage_end, line++, dst, src, -1);
			PADD(src, stage_vert->sdw);
		} else {
			void* src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
			stage_copy(stage_pivot, buffer, src_buffer);
			PADD(src, stage_vert->sdw);
			--run;

			while (run) {
				src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
				stage_mean_vert_self(stage_pivot, buffer, src_buffer);
				PADD(src, stage_vert->sdw);
				--run;
			}

			video_pipeline_run_plain(stage_pivot, stage_end, line++, dst, buffer, -1);
		}
		++y;
		--count;
	}

	video_buffer_free(buffer);
}

/* Compute the mean of the previous line and the first of every iteration */
static void video_stage_stretchy_filter_x1(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	adv_bool buffer_full = 0;
	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;

	while (count) {
		void* dst;
		void* src_buffer;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = target->line(target, y) + x_off;
		src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
		if (buffer_full) {
			stage_mean_vert_self(stage_pivot, buffer, src_buffer);
			video_pipeline_run_plain(stage_pivot, stage_end, line++, dst, buffer, -1);
		} else {
			video_pipeline_run(stage_pivot, stage_end, line++, dst, src_buffer, -1);
			buffer_full = 1;
		}

		if (count > 1) {
			if (run > 1) {
				PADD(src, (run - 1) * stage_vert->sdw);
				src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
			}

			stage_copy(stage_pivot, buffer, src_buffer);
			PADD(src, stage_vert->sdw);

			++y;
		}

		--count;
	}

	video_buffer_free(buffer);
}

/***************************************************************************/
/* stretchy expansion */

/* This example explains the difference of effects in expansion

  ORI   COPY  MEAN/FILTER
  A     A     A     A         (first line)
	A     A     A (== A+A)
  B     B     A+B   A+B
	B     B     B (== B+B)
	B     B     B (== B+B)
  C     C     B+C   B+C
	C     C     C (== C+C)
  D     D     C+D   C+D
  E     E     E     D+E
	E     E     E (== E+E)
*/

static void video_stage_stretchy_1x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;
	unsigned pos = -1;

	while (count) {
		void* buffer;

		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		{
			unsigned prun = run;
			unsigned py = y;

			buffer = video_pipeline_run_partial(0, stage_vert->stage_begin, stage_vert->stage_pivot, 0, src, pos);
			while (prun) {
				void* dst = target->line(target, py) + x_off;
				video_pipeline_run(stage_vert->stage_pivot, stage_vert->stage_end, line++, dst, buffer, pos);
				++py;
				--prun;
			}
		}

		y += run;

		PADD(src, stage_vert->sdw);
		--count;
	}
}

/* The mean effect is applied only at the first added line, if no line */
/* duplication is done, no effect is applied */
static void video_stage_stretchy_mean_1x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);
	void* previous_buffer = 0;

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;

	while (count) {
		void* src_buffer;
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		/* save the previous buffer if required for computation */
		if (previous_buffer)
			stage_copy(stage_pivot, buffer, previous_buffer);

		src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
		dst = target->line(target, y) + x_off;

		/* don't apply the effect if no duplication is required */
		if (previous_buffer) {
			/* apply the mean effect only at the first line */
			stage_mean_vert_self(stage_pivot, buffer, src_buffer);
			video_pipeline_run_plain(stage_pivot, stage_end, line++, dst, buffer, -1);
		} else {
			video_pipeline_run(stage_pivot, stage_end, line++, dst, src_buffer, -1);
		}

		/* If some lines are duplicated save the current buffer */
		if (run >= 2)
			previous_buffer = src_buffer;
		else
			previous_buffer = 0;

		/* first line done */
		++y;
		--run;

		/* do other lines without any effects */
		while (run) {
			dst = target->line(target, y) + x_off;
			video_pipeline_run(stage_pivot, stage_end, line++, dst, src_buffer, -1);
			++y;
			--run;
		}

		PADD(src, stage_vert->sdw);
		--count;
	}

	video_buffer_free(buffer);
}

static void video_stage_stretchy_min_1x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);
	adv_bool buffer_set = 0;

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;

	while (count) {
		void* src_buffer;
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);

		if (buffer_set) {
			if (run != whole) {
				if (color_def_type_get(target->color_def) == adv_color_type_rgb)
					stage_min_rgb_vert_self(stage_pivot, buffer, src_buffer);
				else
					stage_min_vert_self(stage_pivot, buffer, src_buffer);
				dst = target->line(target, y) + x_off;
				video_pipeline_run_plain(stage_pivot, stage_end, line++, dst, buffer, -1);
				++y;
				--run;
			}
			stage_copy(stage_pivot, buffer, src_buffer);
			while (run) {
				dst = target->line(target, y) + x_off;
				video_pipeline_run_plain(stage_pivot, stage_end, line++, dst, buffer, -1);
				++y;
				--run;
			}
		} else {
			stage_copy(stage_pivot, buffer, src_buffer);
			while (run) {
				dst = target->line(target, y) + x_off;
				video_pipeline_run_plain(stage_pivot, stage_end, line++, dst, buffer, -1);
				++y;
				--run;
			}
		}

		buffer_set = 1;

		PADD(src, stage_vert->sdw);
		--count;
	}

	video_buffer_free(buffer);
}


/* The effect is applied at every line */
static void video_stage_stretchy_filter_1x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);
	void* previous_buffer = 0;

	unsigned whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	unsigned count = stage_vert->slice.count;
	unsigned line = 0;

	while (count) {
		void* src_buffer;
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		/* save the previous buffer if required for computation */
		if (previous_buffer)
			stage_copy(stage_pivot, buffer, previous_buffer);

		src_buffer = video_pipeline_run_partial(0, stage_begin, stage_pivot, 0, src, -1);
		dst = target->line(target, y) + x_off;

		if (previous_buffer) {
			/* apply the mean effect only at the first line */
			stage_mean_vert_self(stage_pivot, buffer, src_buffer);
			video_pipeline_run_plain(stage_pivot, stage_end, line++, dst, buffer, -1);
		} else {
			video_pipeline_run(stage_pivot, stage_end, line++, dst, src_buffer, -1);
		}

		/* save always the current buffer (this is the difference from filter and mean) */
		previous_buffer = src_buffer;

		/* first line done */
		++y;
		--run;

		/* do other lines without any effects */
		while (run) {
			dst = target->line(target, y) + x_off;
			video_pipeline_run(stage_pivot, stage_end, line++, dst, src_buffer, -1);
			++y;
			--run;
		}

		PADD(src, stage_vert->sdw);
		--count;
	}

	video_buffer_free(buffer);
}

/***************************************************************************/
/* stretch scale2x */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_scale2x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[2];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<2;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x(stage_vert, dst[0], dst[1], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2x(stage_vert, dst[0], dst[1], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x(stage_vert, dst[0], dst[1], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[2];
		unsigned pline;

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x(stage_vert, final[0], final[1], partial[0], partial[0], partial[1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2x(stage_vert, final[0], final[1], partial[0], partial[1], partial[2], pos);

			for(i=0;i<2;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 2;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x(stage_vert, final[0], final[1], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			video_buffer_free(final[1 - i]);
		}
	}
}

static void video_stage_stretchy_scale2x3(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x3(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2x3(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x3(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x3(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2x3(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x3(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}

static void video_stage_stretchy_scale2x4(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x4(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2x4(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[1], partial[2], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2x4(stage_vert, final[0], final[1], final[2], final[3], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif

/***************************************************************************/
/* stretch scale2k */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_scale2k(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[2];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<2;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2k(stage_vert, dst[0], dst[1], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2k(stage_vert, dst[0], dst[1], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2k(stage_vert, dst[0], dst[1], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2k(stage_vert, final[0], final[1], partial[0], partial[0], partial[1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale2k(stage_vert, final[0], final[1], partial[0], partial[1], partial[2], pos);

			for(i=0;i<2;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 2;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale2k(stage_vert, final[0], final[1], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			video_buffer_free(final[1 - i]);
		}
	}
}
#endif

/***************************************************************************/
/* stretch lq2x */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_lq2x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[2];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<2;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x(stage_vert, dst[0], dst[1], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq2x(stage_vert, dst[0], dst[1], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x(stage_vert, dst[0], dst[1], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x(stage_vert, final[0], final[1], partial[0], partial[0], partial[1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq2x(stage_vert, final[0], final[1], partial[0], partial[1], partial[2], pos);

			for(i=0;i<2;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 2;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x(stage_vert, final[0], final[1], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			video_buffer_free(final[1 - i]);
		}
	}
}

static void video_stage_stretchy_lq2x3(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x3(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq2x3(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x3(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x3(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq2x3(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x3(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}

static void video_stage_stretchy_lq2x4(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x4(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq2x4(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[1], partial[2], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq2x4(stage_vert, final[0], final[1], final[2], final[3], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif

/***************************************************************************/
/* stretch hq2x */

#ifndef USE_BLIT_TINY
#ifndef USE_BLIT_SMALL
static void video_stage_stretchy_hq2x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[2];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<2;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x(stage_vert, dst[0], dst[1], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq2x(stage_vert, dst[0], dst[1], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x(stage_vert, dst[0], dst[1], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x(stage_vert, final[0], final[1], partial[0], partial[0], partial[1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq2x(stage_vert, final[0], final[1], partial[0], partial[1], partial[2], pos);

			for(i=0;i<2;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 2;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x(stage_vert, final[0], final[1], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			video_buffer_free(final[1 - i]);
		}
	}
}

static void video_stage_stretchy_hq2x3(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x3(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq2x3(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x3(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x3(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq2x3(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x3(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}

static void video_stage_stretchy_hq2x4(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x4(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x4(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq2x4(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[1], partial[2], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq2x4(stage_vert, final[0], final[1], final[2], final[3], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif
#endif

/***************************************************************************/
/* stretch xbr2x */

#ifndef USE_BLIT_TINY
#ifndef USE_BLIT_SMALL
static void video_stage_stretchy_xbr2x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[2];
	const void* input[5];
	void* partial[5];
	void* partial_copy[5];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			final[i] = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<2;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	input[3] = src;
	input[4] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);
	PADD(input[3], stage_vert->sdw * 3);
	PADD(input[4], stage_vert->sdw * 4);

	for(i=0;i<5;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);
	partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], -1);
	partial[3] = video_pipeline_run_partial(partial[3], stage_begin, stage_pivot, 0, input[3], -1);

	if (stage_pivot == stage_end) {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, dst[0], dst[1], partial[0], partial[0], partial[0], partial[1], partial[2], -1);

		/* second row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, dst[0], dst[1], partial[0], partial[0], partial[1], partial[2], partial[3], -1);

		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_xbr2x(stage_vert, dst[0], dst[1], partial[0], partial[1], partial[2], partial[3], partial[4], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* before last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, dst[0], dst[1], partial[1-1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], -1);

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, dst[0], dst[1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], partial[4-1], -1);
	} else {
		void* dst[2];

		/* first row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, final[0], final[1], partial[0], partial[0], partial[0], partial[1], partial[2], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;
		
		/* second row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, final[0], final[1], partial[0], partial[0], partial[1], partial[2], partial[3], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;

		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<2;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_xbr2x(stage_vert, final[0], final[1], partial[0], partial[1], partial[2], partial[3], partial[4], pos);

			for(i=0;i<2;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 2;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* befor last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, final[0], final[1], partial[1-1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;

		/* last row */
		for(i=0;i<2;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr2x(stage_vert, final[0], final[1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], partial[4-1], pos);

		for(i=0;i<2;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 2;
	}

	for(i=0;i<5;++i) {
		video_buffer_free(partial_copy[4 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<2;++i) {
			video_buffer_free(final[1 - i]);
		}
	}
}
#endif
#endif


/***************************************************************************/
/* stretch scale3x */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_scale3x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(3 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale3x(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[3];

		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		/* first row */
		stage_scale3x(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale3x(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale3x(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}
#endif

/***************************************************************************/
/* stretch scale3k */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_scale3k(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(3 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale3k(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale3k(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale3k(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale3k(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale3k(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale3k(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}
#endif


/***************************************************************************/
/* stretch lq3x */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_lq3x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(3 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq3x(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq3x(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq3x(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq3x(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}
#endif

/***************************************************************************/
/* stretch hq3x */

#ifndef USE_BLIT_TINY
#ifndef USE_BLIT_SMALL
static void video_stage_stretchy_hq3x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(3 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq3x(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq3x(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq3x(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq3x(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}
#endif
#endif

/***************************************************************************/
/* stretch xbr3x */

#ifndef USE_BLIT_TINY
#ifndef USE_BLIT_SMALL
static void video_stage_stretchy_xbr3x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[3];
	const void* input[5];
	void* partial[5];
	void* partial_copy[5];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			final[i] = video_buffer_alloc(3 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<3;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	input[3] = src;
	input[4] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);
	PADD(input[3], stage_vert->sdw * 3);
	PADD(input[4], stage_vert->sdw * 4);

	for(i=0;i<5;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);
	partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], -1);
	partial[3] = video_pipeline_run_partial(partial[3], stage_begin, stage_pivot, 0, input[3], -1);

	if (stage_pivot == stage_end) {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[0], partial[1], partial[2], -1);

		/* second row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[0], partial[1], partial[2], partial[3], -1);

		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_xbr3x(stage_vert, dst[0], dst[1], dst[2], partial[0], partial[1], partial[2], partial[3], partial[4], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* before last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, dst[0], dst[1], dst[2], partial[1-1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], -1);
		
		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, dst[0], dst[1], dst[2], partial[2-1], partial[3-1], partial[4-1], partial[4-1], partial[4-1], -1);
	} else {
		void* dst[3];

		/* first row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[0], partial[1], partial[2], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* second row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, final[0], final[1], final[2], partial[0], partial[0], partial[1], partial[2], partial[3], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<3;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_xbr3x(stage_vert, final[0], final[1], final[2], partial[0], partial[1], partial[2], partial[3], partial[4], pos);

			for(i=0;i<3;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 3;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* befor last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, final[0], final[1], final[2], partial[1-1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;

		/* last row */
		for(i=0;i<3;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr3x(stage_vert, final[0], final[1], final[2], partial[2-1], partial[3-1], partial[4-1], partial[4-1], partial[4-1], pos);

		for(i=0;i<3;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 3;
	}

	for(i=0;i<5;++i) {
		video_buffer_free(partial_copy[4 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<3;++i) {
			video_buffer_free(final[2 - i]);
		}
	}
}
#endif
#endif

/***************************************************************************/
/* stretch scale4x */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_scale4x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[5];
	void* partial[5];
	void* partial_copy[5];
	void* middle[6];
	void* middle_copy[6];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(4 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	input[3] = src;
	input[4] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);
	PADD(input[3], stage_vert->sdw * 3);
	PADD(input[4], stage_vert->sdw * 4);

	for(i=0;i<5;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	for(i=0;i<6;++i) {
		middle_copy[i] = middle[i] = video_buffer_alloc(2 * stage_vert->sdx * stage_vert->bpp);
	}

	for(i=0;i<4;++i) {
		partial[i] = video_pipeline_run_partial(partial[i], stage_begin, stage_pivot, 0, input[i], -1);
	}

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first 2 rows */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4m(stage_vert, middle[-2+6], middle[-1+6], partial[0], partial[0], partial[1], pos);
		stage_scale4m(stage_vert, middle[0], middle[1], partial[0], partial[1], partial[2], pos);
		stage_scale4m(stage_vert, middle[2], middle[3], partial[1], partial[2], partial[3], pos);
		stage_scale4x(stage_vert, dst[0], dst[1], dst[2], dst[3], middle[-2+6], middle[-2+6], middle[-1+6], middle[0], pos);

		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4x(stage_vert, dst[0], dst[1], dst[2], dst[3], middle[-1+6], middle[0], middle[1], middle[2], -1);

		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_scale4m(stage_vert, middle[4], middle[5], partial[2], partial[3], partial[4], pos);
			stage_scale4x(stage_vert, dst[0], dst[1], dst[2], dst[3], middle[1], middle[2], middle[3], middle[4], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;
			tmp = middle[0];
			middle[0] = middle[2];
			middle[2] = middle[4];
			middle[4] = tmp;
			tmp = middle[1];
			middle[1] = middle[3];
			middle[3] = middle[5];
			middle[5] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* last 2 rows */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4m(stage_vert, middle[4], middle[5], partial[2], partial[3], partial[3], pos);
		stage_scale4x(stage_vert, dst[0], dst[1], dst[2], dst[3], middle[1], middle[2], middle[3], middle[4], pos);

		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4x(stage_vert, dst[0], dst[1], dst[2], dst[3], middle[3], middle[4], middle[5], middle[5], -1);
	} else {
		void* dst[4];

		/* first 2 rows */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4m(stage_vert, middle[-2+6], middle[-1+6], partial[0], partial[0], partial[1], pos);
		stage_scale4m(stage_vert, middle[0], middle[1], partial[0], partial[1], partial[2], pos);
		stage_scale4m(stage_vert, middle[2], middle[3], partial[1], partial[2], partial[3], pos);
		stage_scale4x(stage_vert, final[0], final[1], final[2], final[3], middle[-2+6], middle[-2+6], middle[-1+6], middle[0], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4x(stage_vert, final[0], final[1], final[2], final[3], middle[-1+6], middle[0], middle[1], middle[2], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_scale4m(stage_vert, middle[4], middle[5], partial[2], partial[3], partial[4], pos);
			stage_scale4x(stage_vert, final[0], final[1], final[2], final[3], middle[1], middle[2], middle[3], middle[4], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;
			tmp = middle[0];
			middle[0] = middle[2];
			middle[2] = middle[4];
			middle[4] = tmp;
			tmp = middle[1];
			middle[1] = middle[3];
			middle[3] = middle[5];
			middle[5] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* last 2 rows */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4m(stage_vert, middle[4], middle[5], partial[2], partial[3], partial[3], pos);
		stage_scale4x(stage_vert, final[0], final[1], final[2], final[3], middle[1], middle[2], middle[3], middle[4], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4x(stage_vert, final[0], final[1], final[2], final[3], middle[3], middle[4], middle[5], middle[5], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<6;++i) {
		video_buffer_free(middle_copy[5 - i]);
	}

	for(i=0;i<5;++i) {
		video_buffer_free(partial_copy[4 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif

/***************************************************************************/
/* stretch scale4k */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_scale4k(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(4 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4k(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale4k(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4k(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4k(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_scale4k(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[1], partial[2], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_scale4k(stage_vert, final[0], final[1], final[2], final[3], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif


/***************************************************************************/
/* stretch lq4x */

#ifndef USE_BLIT_TINY
static void video_stage_stretchy_lq4x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(4 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq4x(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_lq4x(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[1], partial[2], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_lq4x(stage_vert, final[0], final[1], final[2], final[3], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif

/***************************************************************************/
/* stretch hq4x */

#ifndef USE_BLIT_TINY
#ifndef USE_BLIT_SMALL
static void video_stage_stretchy_hq4x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[3];
	void* partial[3];
	void* partial_copy[3];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(4 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);

	for(i=0;i<3;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[1], -1);

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[1], partial[2], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[1-1], partial[2-1], partial[2-1], -1);
	} else {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq4x(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* central rows */
		count -= 2;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], pos);

			stage_hq4x(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[1], partial[2], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = tmp;

			PADD(input[2], stage_vert->sdw);
			--count;
		}

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_hq4x(stage_vert, final[0], final[1], final[2], final[3], partial[1-1], partial[2-1], partial[2-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<3;++i) {
		video_buffer_free(partial_copy[2 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif
#endif

/***************************************************************************/
/* stretch xbr4x */

#ifndef USE_BLIT_TINY
#ifndef USE_BLIT_SMALL
static void video_stage_stretchy_xbr4x(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final[4];
	const void* input[5];
	void* partial[5];
	void* partial_copy[5];
	void* tmp;
	unsigned i;

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			final[i] = video_buffer_alloc(4 * stage_pivot->sdx * stage_pivot->sbpp);
		}
	} else {
		for(i=0;i<4;++i) {
			final[i] = 0;
		}
	}

	input[0] = src;
	input[1] = src;
	input[2] = src;
	input[3] = src;
	input[4] = src;
	PADD(input[1], stage_vert->sdw);
	PADD(input[2], stage_vert->sdw * 2);
	PADD(input[3], stage_vert->sdw * 3);
	PADD(input[4], stage_vert->sdw * 4);

	for(i=0;i<5;++i) {
		partial_copy[i] = partial[i] = video_buffer_alloc(stage_vert->sdx * stage_vert->bpp);
	}

	partial[0] = video_pipeline_run_partial(partial[0], stage_begin, stage_pivot, 0, input[0], -1);
	partial[1] = video_pipeline_run_partial(partial[1], stage_begin, stage_pivot, 0, input[1], -1);
	partial[2] = video_pipeline_run_partial(partial[2], stage_begin, stage_pivot, 0, input[2], -1);
	partial[3] = video_pipeline_run_partial(partial[3], stage_begin, stage_pivot, 0, input[3], -1);

	if (stage_pivot == stage_end) {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[0], partial[1], partial[2], -1);

		/* second row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[0], partial[1], partial[2], partial[3], -1);

		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_xbr4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[0], partial[1], partial[2], partial[3], partial[4], pos);

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* befor last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[1-1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], -1);

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, dst[0], dst[1], dst[2], dst[3], partial[2-1], partial[3-1], partial[4-1], partial[4-1], partial[4-1], -1);
	} else {
		void* dst[4];

		/* first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[0], partial[1], partial[2], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* second first row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[0], partial[1], partial[2], partial[3], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
		
		/* central rows */
		count -= 4;
		while (count) {
			for(i=0;i<4;++i) {
				dst[i] = target->line(target, y) + x_off;
				++y;
			}

			partial[4] = video_pipeline_run_partial(partial[4], stage_begin, stage_pivot, 0, input[4], pos);

			stage_xbr4x(stage_vert, final[0], final[1], final[2], final[3], partial[0], partial[1], partial[2], partial[3], partial[4], pos);

			for(i=0;i<4;++i) {
				video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
			}

			line += 4;

			tmp = partial[0];
			partial[0] = partial[1];
			partial[1] = partial[2];
			partial[2] = partial[3];
			partial[3] = partial[4];
			partial[4] = tmp;

			PADD(input[4], stage_vert->sdw);
			--count;
		}

		/* befor last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, final[0], final[1], final[2], final[3], partial[1-1], partial[2-1], partial[3-1], partial[4-1], partial[4-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;

		/* last row */
		for(i=0;i<4;++i) {
			dst[i] = target->line(target, y) + x_off;
			++y;
		}

		stage_xbr4x(stage_vert, final[0], final[1], final[2], final[3], partial[2-1], partial[3-1], partial[4-1], partial[4-1], partial[4-1], pos);

		for(i=0;i<4;++i) {
			video_pipeline_run_plain(stage_pivot, stage_end, line + i, dst[i], final[i], pos);
		}

		line += 4;
	}

	for(i=0;i<5;++i) {
		video_buffer_free(partial_copy[4 - i]);
	}

	if (stage_pivot != stage_end) {
		for(i=0;i<4;++i) {
			video_buffer_free(final[3 - i]);
		}
	}
}
#endif
#endif


/***************************************************************************/
/* stretchy copy */

static void video_stage_stretchy_11(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src)
{
	unsigned x_off = x * target->bytes_per_pixel;
	unsigned count = stage_vert->sdy;
	unsigned line = 0;
	unsigned pos = -1;

	while (count) {
		void* dst;

		dst = target->line(target, y) + x_off;
		video_pipeline_run(stage_vert->stage_begin, stage_vert->stage_end, line++, dst, src, -1);
		++y;

		PADD(src, stage_vert->sdw);
		--count;
	}
}

/***************************************************************************/
/* stretchy */

/* set the pivot early in the pipeline */
static void video_stage_pivot_early_set(struct video_stage_vert_struct* stage_vert, unsigned combine)
{
	if (combine_is_rgb(combine)) {
		stage_vert->stage_pivot = stage_vert->stage_end;
		while (stage_vert->stage_pivot != stage_vert->stage_begin
			&& !pipe_is_conversion(stage_vert->stage_pivot[-1].type)) {
			--stage_vert->stage_pivot;
		}
	} else {
		stage_vert->stage_pivot = stage_vert->stage_begin;
	}
}

/* set the pivot late in the pipeline */
static void video_stage_pivot_late_set(struct video_stage_vert_struct* stage_vert, unsigned combine)
{
	if (combine_is_direct(combine)) {
		stage_vert->stage_pivot = stage_vert->stage_end;
	} else {
		assert(stage_vert->stage_begin != stage_vert->stage_end);
		stage_vert->stage_pivot = stage_vert->stage_end - 1;
	}
	while (stage_vert->stage_pivot != stage_vert->stage_begin
		&& pipe_is_decoration(stage_vert->stage_pivot[-1].type)) {
		--stage_vert->stage_pivot;
	}
}

/* Initialize the vertical stage */
static void video_stage_stretchy_set(const struct video_pipeline_target_struct* target, struct video_stage_vert_struct* stage_vert, const struct video_pipeline_struct* pipeline, unsigned ddx, unsigned ddy, unsigned sdx, unsigned sdy, int sdw, unsigned combine)
{
	unsigned combine_y = combine & VIDEO_COMBINE_Y_MASK;

	stage_vert->sdy = sdy;
	stage_vert->sdw = sdw;
	stage_vert->ddy = ddy;

#if !defined(USE_BLIT_SMALL) && !defined(USE_BLIT_TINY)
	/* The pixel type is always the target pixel type because, when used, any conversion is done before */
	if (color_def_type_get(target->color_def) == adv_color_type_yuy2)
		stage_vert->interp = INTERP_YUY2;
	else if (color_def_bytes_per_pixel_get(target->color_def) == 4)
		stage_vert->interp = INTERP_32;
	else if (color_def_bytes_per_pixel_get(target->color_def) == 2)
		stage_vert->interp = INTERP_16;
	else
		stage_vert->interp = INTERP_NONE;
#endif

	stage_vert->stage_begin = video_pipeline_begin(pipeline);
	stage_vert->stage_end = video_pipeline_end(pipeline);

#ifndef USE_BLIT_TINY
	if (ddx == 2*sdx && ddy == 2*sdy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		/* scale2x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale2x;
		stage_vert->type = pipe_y_scale2x;
	} else if (ddx == 2*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		/* scale2x3 */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale2x3;
		stage_vert->type = pipe_y_scale2x3;
	} else if (ddx == 2*sdx && ddy == 4*sdy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		/* scale2x4 */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale2x4;
		stage_vert->type = pipe_y_scale2x4;
	} else if (ddx == 2*sdx && ddy == 2*sdy && combine_y == VIDEO_COMBINE_Y_SCALEK) {
		/* scale2k */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale2k;
		stage_vert->type = pipe_y_scale2k;
	} else if (ddx == 2*sdx && ddy == 2*sdy && combine_y == VIDEO_COMBINE_Y_LQ) {
		/* lq2x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_lq2x;
		stage_vert->type = pipe_y_lq2x;
	} else if (ddx == 2*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_LQ) {
		/* lq2x3 */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_lq2x3;
		stage_vert->type = pipe_y_lq2x3;
	} else if (ddx == 2*sdx && ddy == 4*sdy && combine_y == VIDEO_COMBINE_Y_LQ) {
		/* lq2x4 */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_lq2x4;
		stage_vert->type = pipe_y_lq2x4;
#ifndef USE_BLIT_SMALL
	} else if (ddx == 2*sdx && ddy == 2*sdy && combine_y == VIDEO_COMBINE_Y_HQ) {
		/* hq2x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_hq2x;
		stage_vert->type = pipe_y_hq2x;
	} else if (ddx == 2*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_HQ) {
		/* hq2x3 */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_hq2x3;
		stage_vert->type = pipe_y_hq2x3;
	} else if (ddx == 2*sdx && ddy == 2*sdy && combine_y == VIDEO_COMBINE_Y_HQ) {
		/* hq2x4 */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_hq2x4;
		stage_vert->type = pipe_y_hq2x4;
	} else if (ddx == 2*sdx && ddy == 2*sdy && combine_y == VIDEO_COMBINE_Y_XBR) {
		/* xbr2x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_xbr2x;
		stage_vert->type = pipe_y_xbr2x;
#endif
	} else if (ddx == 3*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		/* scale3x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale3x;
		stage_vert->type = pipe_y_scale3x;
	} else if (ddx == 3*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_SCALEK) {
		/* scale3k */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale3k;
		stage_vert->type = pipe_y_scale3k;
	} else if (ddx == 3*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_LQ) {
		/* lq3x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_lq3x;
		stage_vert->type = pipe_y_lq3x;
#ifndef USE_BLIT_SMALL
	} else if (ddx == 3*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_HQ) {
		/* hq3x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_hq3x;
		stage_vert->type = pipe_y_hq3x;
	} else if (ddx == 3*sdx && ddy == 3*sdy && combine_y == VIDEO_COMBINE_Y_XBR) {
		/* xbr3x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_xbr3x;
		stage_vert->type = pipe_y_xbr3x;
#endif
	} else if (ddx == 4*sdx && ddy == 4*sdy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		/* scale4x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale4x;
		stage_vert->type = pipe_y_scale4x;
	} else if (ddx == 4*sdx && ddy == 4*sdy && combine_y == VIDEO_COMBINE_Y_SCALEK) {
		/* scale4k */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_scale4k;
		stage_vert->type = pipe_y_scale4k;
	} else if (ddx == 4*sdx && ddy == 4*sdy && combine_y == VIDEO_COMBINE_Y_LQ) {
		/* lq4x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_lq4x;
		stage_vert->type = pipe_y_lq4x;
#ifndef USE_BLIT_SMALL
	} else if (ddx == 4*sdx && ddy == 4*sdy && combine_y == VIDEO_COMBINE_Y_HQ) {
		/* hq4x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_hq4x;
		stage_vert->type = pipe_y_hq4x;
	} else if (ddx == 4*sdx && ddy == 4*sdy && combine_y == VIDEO_COMBINE_Y_XBR) {
		/* xbr4x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_late_set(stage_vert, combine);
		stage_vert->put = video_stage_stretchy_xbr4x;
		stage_vert->type = pipe_y_xbr4x;
#endif
	} else
#endif
	if (sdy < ddy) { /* y expansion */
		slice_set(&stage_vert->slice, sdy, ddy);

		switch (combine_y) {
			case VIDEO_COMBINE_Y_MAXMIN :
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_min_1x;
				stage_vert->type = pipe_y_maxmin;
				break;
			case VIDEO_COMBINE_Y_MEAN :
				video_stage_pivot_late_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_mean_1x;
				stage_vert->type = pipe_y_mean;
				break;
			case VIDEO_COMBINE_Y_FILTER :
				video_stage_pivot_late_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_filter_1x;
				stage_vert->type = pipe_y_filter;
				break;
			default:
				video_stage_pivot_late_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_1x;
				stage_vert->type = pipe_y_copy;
				break;
		}
	} else if (sdy == ddy) { /* y copy */
		slice_set(&stage_vert->slice, sdy, ddy);

		switch (combine_y) {
			case VIDEO_COMBINE_Y_MEAN :
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_mean_1x;
				stage_vert->type = pipe_y_mean;
				break;
			case VIDEO_COMBINE_Y_FILTER :
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_filter_1x;
				stage_vert->type = pipe_y_filter;
				break;
			default:
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_11;
				stage_vert->type = pipe_y_copy;
				break;
		}
	} else { /* y reduction */
		slice_set(&stage_vert->slice, sdy, ddy);

		switch (combine_y) {
			case VIDEO_COMBINE_Y_MAXMIN :
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_max_x1;
				stage_vert->type = pipe_y_maxmin;
				break;
			case VIDEO_COMBINE_Y_MEAN :
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_mean_x1;
				stage_vert->type = pipe_y_mean;
				break;
			case VIDEO_COMBINE_Y_FILTER :
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_filter_x1;
				stage_vert->type = pipe_y_filter;
				break;
			default:
				video_stage_pivot_early_set(stage_vert, combine);
				stage_vert->put = video_stage_stretchy_x1;
				stage_vert->type = pipe_y_copy;
				break;
		}
	}

	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		if (combine_y == VIDEO_COMBINE_Y_MEAN
			|| combine_y == VIDEO_COMBINE_Y_FILTER
			|| (combine & VIDEO_COMBINE_X_FILTER)!=0
			|| (combine & VIDEO_COMBINE_X_MEAN)!=0
			|| (combine & VIDEO_COMBINE_INTERLACE_FILTER)!=0
		)
			internal_mean_set(target);

		if (combine_y == VIDEO_COMBINE_Y_MAXMIN
			|| (combine & VIDEO_COMBINE_X_MAXMIN)!=0)
			internal_maxmin_rgb_set(target);
	}

#ifndef USE_BLIT_TINY
	if (combine_y == VIDEO_COMBINE_Y_LQ
#ifndef USE_BLIT_SMALL
		|| combine_y == VIDEO_COMBINE_Y_HQ
		|| combine_y == VIDEO_COMBINE_Y_XBR
#endif
	) {
		interp_set(target->color_def);
	}
#endif
}

/* Initialize the vertical stage for the no transformation special case */
static void video_stage_stretchy_11_set(struct video_stage_vert_struct* stage_vert, const struct video_pipeline_struct* pipeline, unsigned sdy, int sdw)
{
	stage_vert->sdy = sdy;
	stage_vert->sdw = sdw;
	stage_vert->ddy = sdy;

	slice_set(&stage_vert->slice, sdy, sdy);

	stage_vert->stage_begin = video_pipeline_begin(pipeline);
	stage_vert->stage_end = video_pipeline_end(pipeline);
	stage_vert->stage_pivot = stage_vert->stage_begin;

	stage_vert->put = video_stage_stretchy_11;
	stage_vert->type = pipe_y_copy;
}

/***************************************************************************/
/* stretch */

static void video_pipeline_make(const struct video_pipeline_target_struct* target, struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dp, unsigned combine)
{
	unsigned bytes_per_pixel = target->bytes_per_pixel;
	unsigned combine_y = combine & VIDEO_COMBINE_Y_MASK;

	/* in x reduction the filter is applied before the resize */
	if ((combine & VIDEO_COMBINE_X_FILTER) != 0
		&& src_dx > dst_dx) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_filter8_set(video_pipeline_insert(pipeline), src_dx, src_dp); break;
		case 2 : video_stage_filter16_set(video_pipeline_insert(pipeline), src_dx, src_dp); break;
		case 4 : video_stage_filter32_set(video_pipeline_insert(pipeline), src_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	/* do the x stretch */
#ifndef USE_BLIT_TINY
	/* cases for that the horizontal stretch is done by the y stage */
	if (dst_dx == 2*src_dx && dst_dy == 2*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 2*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEK) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 2*src_dy && combine_y == VIDEO_COMBINE_Y_LQ) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_LQ) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_LQ) {
		src_dp = bytes_per_pixel;
#ifndef USE_BLIT_SMALL
	} else if (dst_dx == 2*src_dx && dst_dy == 2*src_dy && combine_y == VIDEO_COMBINE_Y_HQ) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_HQ) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_HQ) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 2*src_dx && dst_dy == 2*src_dy && combine_y == VIDEO_COMBINE_Y_XBR) {
		src_dp = bytes_per_pixel;
#endif
	} else if (dst_dx == 3*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 3*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEK) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 3*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_LQ) {
		src_dp = bytes_per_pixel;
#ifndef USE_BLIT_SMALL
	} else if (dst_dx == 3*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_HQ) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 3*src_dx && dst_dy == 3*src_dy && combine_y == VIDEO_COMBINE_Y_XBR) {
		src_dp = bytes_per_pixel;
#endif
	} else if (dst_dx == 4*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEX) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 4*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_SCALEK) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 4*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_LQ) {
		src_dp = bytes_per_pixel;
#ifndef USE_BLIT_SMALL
	} else if (dst_dx == 4*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_HQ) {
		src_dp = bytes_per_pixel;
	} else if (dst_dx == 4*src_dx && dst_dy == 4*src_dy && combine_y == VIDEO_COMBINE_Y_XBR) {
		src_dp = bytes_per_pixel;
#endif
	} else
#endif
	{
#ifndef USE_BLIT_TINY
		/* disable the y effect if size doesn't match */
		switch (combine_y) {
		case VIDEO_COMBINE_Y_SCALEX :
		case VIDEO_COMBINE_Y_SCALEK :
		case VIDEO_COMBINE_Y_LQ :
#ifndef USE_BLIT_SMALL
		case VIDEO_COMBINE_Y_HQ :
		case VIDEO_COMBINE_Y_XBR :
#endif
			combine_y = VIDEO_COMBINE_Y_NONE;
			break;
		}
#endif
		if (dst_dx != src_dx) {
			if ((combine & VIDEO_COMBINE_X_MEAN) != 0) {
				switch (bytes_per_pixel) {
				case 1 : video_stage_meanx8_set(video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				case 2 : video_stage_meanx16_set(video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				case 4 : video_stage_meanx32_set(video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				}
				src_dp = bytes_per_pixel;
			} else if ((combine & VIDEO_COMBINE_X_MAXMIN) != 0) {
				switch (bytes_per_pixel) {
				case 1 : video_stage_maxminx8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				case 2 : video_stage_maxminx16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				case 4 : video_stage_maxminx32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				}
				src_dp = bytes_per_pixel;
			} else {
				switch (bytes_per_pixel) {
				case 1 : video_stage_stretchx8_set(video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				case 2 : video_stage_stretchx16_set(video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				case 4 : video_stage_stretchx32_set(video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp); break;
				}
				src_dp = bytes_per_pixel;
			}
		}
	}

	/* in x expansion the filter is applied after the resize */
	if ((combine & VIDEO_COMBINE_X_FILTER)!=0
		&& src_dx <= dst_dx) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_filter8_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_filter16_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_filter32_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

#ifndef USE_BLIT_TINY
	if ((combine & VIDEO_COMBINE_X_RGB_TRIAD16PIX)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_triad16pix8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_triad16pix16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_triad16pix32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG16PIX)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_triadstrong16pix8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_triadstrong16pix16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_triadstrong16pix32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIAD6PIX)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_triad6pix8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_triad6pix16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_triad6pix32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG6PIX)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_triadstrong6pix8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_triadstrong6pix16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_triadstrong6pix32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIAD3PIX)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_triad3pix8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_triad3pix16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_triad3pix32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG3PIX)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_triadstrong3pix8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_triadstrong3pix16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_triadstrong3pix32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANDOUBLEHORZ)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_scandouble8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_scandouble16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_scandouble32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANTRIPLEHORZ)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_scantriple8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_scantriple16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_scantriple32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANDOUBLEVERT)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_scandoublevert8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_scandoublevert16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_scandoublevert32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANTRIPLEVERT)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_rgb_scantriplevert8_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_rgb_scantriplevert16_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_rgb_scantriplevert32_set(target, video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}
#endif

	if ((combine & VIDEO_COMBINE_INTERLACE_FILTER)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_interlacefilter8_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_interlacefilter16_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_interlacefilter32_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_SWAP_EVEN)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_swapeven8_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_swapeven16_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_swapeven32_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_SWAP_ODD)!=0) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_swapodd8_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_swapodd16_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_swapodd32_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}

	/* add a dummy stage if it's required of it improves the speed */
	if (
		/* if the last stage is required */
		(!combine_is_direct(combine_y) && video_pipeline_size(pipeline) == 0)
		/* if the last stage exists and it's a conversion and a conversion is not allowed as a last stage */
		|| (!combine_is_direct(combine_y) && combine_is_rgb(combine_y) && video_pipeline_size(pipeline) != 0 && pipe_is_conversion(video_pipeline_end(pipeline)[-1].type))
		/* if a buffer is requested */
		|| ((combine & VIDEO_COMBINE_BUFFER) != 0)
	) {
		switch (bytes_per_pixel) {
		case 1 : video_stage_copy8_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 2 : video_stage_copy16_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		case 4 : video_stage_copy32_set(video_pipeline_insert(pipeline), dst_dx, src_dp); break;
		}
		src_dp = bytes_per_pixel;
	}
}

void video_pipeline_direct(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, adv_color_def src_color_def, unsigned combine)
{
	adv_color_def dst_color_def = pipeline->target.color_def;
	unsigned bytes_per_pixel = pipeline->target.bytes_per_pixel;

	/* conversion */
	if (src_color_def != dst_color_def) {
		/* only conversion from rgb are supported */
		assert(color_def_type_get(src_color_def) == adv_color_type_rgb);

		/* preconversion */
		if (src_color_def == color_def_make_rgb_from_sizelenpos(4, 8, 0, 8, 8, 8, 16)) {
			video_stage_rgba8888tobgra8888_set(video_pipeline_insert(pipeline), src_dx, src_dp);
			src_color_def = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0);
			src_dp = 4;
		} else if (src_color_def == color_def_make_rgb_from_sizelenpos(3, 8, 0, 8, 8, 8, 16)) {
			video_stage_rgb888tobgra8888_set(video_pipeline_insert(pipeline), src_dx, src_dp);
			src_color_def = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0);
			src_dp = 4;
		} else if (src_color_def == color_def_make_rgb_from_sizelenpos(3, 8, 16, 8, 8, 8, 0)) {
			video_stage_bgr888tobgra8888_set(video_pipeline_insert(pipeline), src_dx, src_dp);
			src_color_def = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0);
			src_dp = 4;
		}

		/* conversion */
		if (src_color_def == color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0)) {
			if (dst_color_def == color_def_make_rgb_from_sizelenpos(1, 3, 5, 3, 2, 2, 0)) {
				/* rotation */
				if (src_dp != 4) {
					video_stage_rot32_set(video_pipeline_insert(pipeline), src_dx, src_dp);
					src_dp = 4;
				}
				video_stage_bgra8888tobgr332_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 1;
			} else if (dst_color_def == color_def_make_rgb_from_sizelenpos(2, 5, 10, 5, 5, 5, 0)) {
				/* rotation */
				if (src_dp != 4) {
					video_stage_rot32_set(video_pipeline_insert(pipeline), src_dx, src_dp);
					src_dp = 4;
				}
				video_stage_bgra8888tobgra5551_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 2;
			} else if (dst_color_def == color_def_make_rgb_from_sizelenpos(2, 5, 11, 6, 5, 5, 0)) {
				/* rotation */
				if (src_dp != 4) {
					video_stage_rot32_set(video_pipeline_insert(pipeline), src_dx, src_dp);
					src_dp = 4;
				}
				video_stage_bgra8888tobgr565_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 2;
			} else if (dst_color_def == color_def_make(adv_color_type_yuy2)) {
				video_stage_bgra8888toyuy2_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 4;
			} else {
				video_stage_rgbtorgb_set(video_pipeline_insert(pipeline), src_dx, src_dp, src_color_def, dst_color_def);
				src_dp = color_def_bytes_per_pixel_get(dst_color_def);
			}
		} else if (src_color_def == color_def_make_rgb_from_sizelenpos(2, 5, 10, 5, 5, 5, 0)) {
			if (dst_color_def == color_def_make_rgb_from_sizelenpos(1, 3, 5, 3, 2, 2, 0)) {
				/* rotation */
				if (src_dp != 2) {
					video_stage_rot16_set(video_pipeline_insert(pipeline), src_dx, src_dp);
					src_dp = 2;
				}
				video_stage_bgra5551tobgr332_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 1;
			} else if (dst_color_def == color_def_make_rgb_from_sizelenpos(2, 5, 11, 6, 5, 5, 0)) {
				/* rotation */
				if (src_dp != 2) {
					video_stage_rot16_set(video_pipeline_insert(pipeline), src_dx, src_dp);
					src_dp = 2;
				}
				video_stage_bgra5551tobgr565_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 2;
			} else if (dst_color_def == color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0)) {
				/* rotation */
				if (src_dp != 2) {
					video_stage_rot16_set(video_pipeline_insert(pipeline), src_dx, src_dp);
					src_dp = 2;
				}
				video_stage_bgra5551tobgra8888_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 4;
			} else if (dst_color_def == color_def_make(adv_color_type_yuy2)) {
				video_stage_bgra5551toyuy2_set(video_pipeline_insert(pipeline), src_dx, src_dp);
				src_dp = 4;
			} else {
				video_stage_rgbtorgb_set(video_pipeline_insert(pipeline), src_dx, src_dp, src_color_def, dst_color_def);
				src_dp = color_def_bytes_per_pixel_get(dst_color_def);
			}
		} else {
			if (dst_color_def == color_def_make(adv_color_type_yuy2)) {
				video_stage_rgbtoyuy2_set(video_pipeline_insert(pipeline), src_dx, src_dp, src_color_def);
				src_dp = 4;
			} else {
				video_stage_rgbtorgb_set(video_pipeline_insert(pipeline), src_dx, src_dp, src_color_def, dst_color_def);
				src_dp = color_def_bytes_per_pixel_get(dst_color_def);
			}
		}
	} else {
		/* rotation */
		if (src_dp != bytes_per_pixel) {
			switch (bytes_per_pixel) {
			case 1 : video_stage_rot8_set(video_pipeline_insert(pipeline), src_dx, src_dp); break;
			case 2 : video_stage_rot16_set(video_pipeline_insert(pipeline), src_dx, src_dp); break;
			case 4 : video_stage_rot32_set(video_pipeline_insert(pipeline), src_dx, src_dp); break;
			}
			src_dp = bytes_per_pixel;
		}
	}

	video_pipeline_make(&pipeline->target, pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dp, combine);

	video_stage_stretchy_set(&pipeline->target, video_pipeline_vert_mutable(pipeline), pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, dst_dx, bytes_per_pixel, combine);
}

void video_pipeline_palette16hw(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned combine)
{
	unsigned bytes_per_pixel = pipeline->target.bytes_per_pixel;

	/* conversion and rotation */

	switch (bytes_per_pixel) {
	case 1 :
		video_stage_imm16to8_set(video_pipeline_insert(pipeline), src_dx, src_dp);
		break;
	case 2 :
		if (src_dp != bytes_per_pixel) {
			video_stage_rot16_set(video_pipeline_insert(pipeline), src_dx, src_dp);
		}
		break;
	case 4 :
		video_stage_imm16to32_set(video_pipeline_insert(pipeline), src_dx, src_dp);
		break;
	}
	src_dp = bytes_per_pixel;

	video_pipeline_make(&pipeline->target, pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dp, combine);

	video_stage_stretchy_set(&pipeline->target, video_pipeline_vert_mutable(pipeline), pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, dst_dx, bytes_per_pixel, combine);
}

void video_pipeline_palette8(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, const uint8* palette8, const uint16* palette16, const uint32* palette32, unsigned combine)
{
	unsigned bytes_per_pixel = pipeline->target.bytes_per_pixel;

	/* conversion and rotation */
	switch (bytes_per_pixel) {
	case 1 :
		video_stage_palette8to8_set(video_pipeline_insert(pipeline), src_dx, src_dp, palette8);
		break;
	case 2 :
		video_stage_palette8to16_set(video_pipeline_insert(pipeline), src_dx, src_dp, palette16);
		break;
	case 4 :
		video_stage_palette8to32_set(video_pipeline_insert(pipeline), src_dx, src_dp, palette32);
		break;
	}
	src_dp = bytes_per_pixel;

	video_pipeline_make(&pipeline->target, pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dp, combine);

	video_stage_stretchy_set(&pipeline->target, video_pipeline_vert_mutable(pipeline), pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, dst_dx, bytes_per_pixel, combine);
}

void video_pipeline_palette16(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, const uint8* palette8, const uint16* palette16, const uint32* palette32, unsigned combine)
{
	unsigned bytes_per_pixel = pipeline->target.bytes_per_pixel;

	/* conversion and rotation */
	switch (bytes_per_pixel) {
	case 1 :
		video_stage_palette16to8_set(video_pipeline_insert(pipeline), src_dx, src_dp, palette8);
		break;
	case 2 :
		video_stage_palette16to16_set(video_pipeline_insert(pipeline), src_dx, src_dp, palette16);
		break;
	case 4 :
		video_stage_palette16to32_set(video_pipeline_insert(pipeline), src_dx, src_dp, palette32);
		break;
	}
	src_dp = bytes_per_pixel;

	video_pipeline_make(&pipeline->target, pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dp, combine);

	video_stage_stretchy_set(&pipeline->target, video_pipeline_vert_mutable(pipeline), pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, dst_dx, bytes_per_pixel, combine);
}

void video_pipeline_blit(const struct video_pipeline_struct* pipeline, unsigned dst_x, unsigned dst_y, const void* src)
{
	video_pipeline_vert_run(pipeline, dst_x, dst_y, src);
}


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

#include "blit.h"
#include "log.h"
#include "error.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************************/
/* mmx */

#if defined(USE_ASM_i586)

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
		: "cc"
	);
}

static int blit_has_mmx(void)
{
	unsigned regs[4];
	unsigned a, b;

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
		return 0; /* no cpuid */
	}

	blit_cpuid(0, regs);
	if (regs[0] > 0) {
		blit_cpuid(1, regs);
		if ((regs[3] & 0x800000) != 0) {
			log_std(("blit: mmx\n"));
			return 1;
		}
	}

	log_std(("blit: no mmx\n"));
	return 0;
}

/* Support the the both condition. MMX present or not */
int the_blit_mmx = 0;
#define BLITTER(name) (the_blit_mmx ? name##_mmx : name##_def)

static int blit_set_mmx(void)
{
	the_blit_mmx = blit_has_mmx();

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
#define BLITTER(name) (name##_def)

static int blit_set_mmx(void)
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
#include "iunchain.h"
#include "irgb.h"
#include "imax.h"
#include "imean.h"
#include "scale2x.h"

/***************************************************************************/
/* video stage */

#define STAGE_SIZE(stage, _type, _sdx, _sdp, _sbpp, _ddx, _dbpp) \
	stage->type = (_type); \
	stage->sdx = (_sdx); \
	stage->sdp = (_sdp); \
	stage->sbpp = (_sbpp); \
	slice_set(&stage->slice, (_sdx), (_ddx)); \
	stage->palette = 0; \
	stage->buffer_size = (_dbpp)*(_ddx); \
	stage->buffer_extra_size = 0; \
	stage->plane_num = 0; \
	stage->plane_put_plain = 0; \
	stage->plane_put = 0; \
	stage->put_plain = 0; \
	stage->put = 0

#define STAGE_PUT(stage, _put_plain, _put) \
	stage->put_plain = (_put_plain); \
	stage->put = (stage->sbpp == stage->sdp) ? stage->put_plain : (_put)

#define STAGE_EXTRA(stage) \
	stage->buffer_extra_size = stage->buffer_size

#define STAGE_PALETTE(stage, _palette) \
	stage->palette = _palette

#include "vstretch.h"
#include "vcopy.h"
#include "vrot.h"
#include "vunchain.h"
#include "vrgb.h"
#include "vfilter.h"
#include "vconv.h"
#include "vpalette.h"
#include "vswap.h"

/***************************************************************************/
/* fast_buffer */

/* A very fast dynamic buffers allocations */

/* Max number of allocable buffers */
#define FAST_BUFFER_MAX 64

/* Total size of the buffers */
#define FAST_BUFFER_SIZE (128*1024)

/* Align mask */
#define FAST_BUFFER_ALIGN_MASK 0x1F

void* fast_buffer; /* raw pointer */
void* fast_buffer_aligned; /* aligned pointer */
unsigned fast_buffer_map[FAST_BUFFER_MAX]; /* stack of incremental size used */
unsigned fast_buffer_mac; /* top of the stack */

static inline void* video_buffer_alloc(unsigned size)
{
	unsigned size_aligned = (size + FAST_BUFFER_ALIGN_MASK) & ~FAST_BUFFER_ALIGN_MASK;
	assert( fast_buffer_mac < FAST_BUFFER_MAX );

	++fast_buffer_mac;
	fast_buffer_map[fast_buffer_mac] = fast_buffer_map[fast_buffer_mac-1] + size_aligned;

	assert( fast_buffer_map[fast_buffer_mac] <= FAST_BUFFER_SIZE);

	return (uint8*)fast_buffer_aligned + fast_buffer_map[fast_buffer_mac-1];
}

/* Buffers must be allocated and freed in exact reverse order */
static inline void video_buffer_free(void* buffer)
{
	(void)buffer;
	--fast_buffer_mac;
}

/* Debug version of the alloc functions */
#ifndef NDEBUG

#define WRAP_SIZE 32

static void* video_buffer_alloc_wrap(unsigned size)
{
	uint8* buffer8 = (uint8*)video_buffer_alloc(size + WRAP_SIZE);
	int i;
	for(i=0;i<WRAP_SIZE;++i)
		buffer8[i] = i;
	return buffer8 + WRAP_SIZE;
}

static void video_buffer_free_wrap(void* buffer)
{
	uint8* buffer8 = (uint8*)buffer - WRAP_SIZE;
	int i;
	for(i=0;i<WRAP_SIZE;++i)
		assert(buffer8[i] == i);
	video_buffer_free(buffer8);
}

#define video_buffer_free video_buffer_free_wrap
#define video_buffer_alloc video_buffer_alloc_wrap

#endif

static void video_buffer_init(void)
{
	fast_buffer = malloc(FAST_BUFFER_SIZE + FAST_BUFFER_ALIGN_MASK);
	fast_buffer_aligned = (void*)(((unsigned)fast_buffer + FAST_BUFFER_ALIGN_MASK) & ~FAST_BUFFER_ALIGN_MASK);
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
	unsigned i;

	if (blit_set_mmx() != 0) {
		error_set("This executable requires an MMX processor.\n");
		return -1;
	}

	for(i=0;i<256;++i) {
		mask8_set_all[i] = i | i << 8 | i << 16 | i << 24;
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

static void stage_copy(const struct video_stage_horz_struct* stage, void* dst, void* src)
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

static void stage_scale2x(const struct video_stage_horz_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2)
{
	if ((int)stage->sbpp == stage->sdp) {
		switch (stage->sbpp) {
			case 1 : BLITTER(scale2x_8)(dst0, dst1, src0, src1, src2, stage->sdx); break;
			case 2 : BLITTER(scale2x_16)(dst0, dst1, src0, src1, src2, stage->sdx); break;
			case 4 : BLITTER(scale2x_32)(dst0, dst1, src0, src1, src2, stage->sdx); break;
		}
	}
}

static void stage_scale2x_last(const struct video_stage_vert_struct* stage, void* dst0, void* dst1, void* src0, void* src1, void* src2)
{
	if ((int)stage->stage_pivot_sbpp == stage->stage_pivot_sdp) {
		switch (stage->stage_pivot_sbpp) {
			case 1 : BLITTER(scale2x_8)(dst0, dst1, src0, src1, src2, stage->stage_pivot_sdx); break;
			case 2 : BLITTER(scale2x_16)(dst0, dst1, src0, src1, src2, stage->stage_pivot_sdx); break;
			case 4 : BLITTER(scale2x_32)(dst0, dst1, src0, src1, src2, stage->stage_pivot_sdx); break;
		}
	}
}

/***************************************************************************/
/* stage/pipeline */

void video_pipeline_init(struct video_pipeline_struct* pipeline)
{
	pipeline->stage_mac = 0;
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

static struct video_stage_horz_struct* video_pipeline_substitute(struct video_pipeline_struct* pipeline, struct video_stage_horz_struct* begin, struct video_stage_horz_struct* end)
{
	struct video_stage_horz_struct* last = video_pipeline_end_mutable(pipeline);
	pipeline->stage_mac += 1 - (end - begin);
	while (end != last) {
		++begin;
		memcpy(begin, end, sizeof(struct video_stage_horz_struct));
		++end;
	}
	return begin;
}

static void video_pipeline_realize(struct video_pipeline_struct* pipeline, int sdx, int sdp, int sbpp)
{
	struct video_stage_vert_struct* stage_vert = video_pipeline_vert_mutable(pipeline);
	struct video_stage_horz_struct* stage_begin = video_pipeline_begin_mutable(pipeline);
	struct video_stage_horz_struct* stage_end = video_pipeline_end_mutable(pipeline);

	if (stage_begin != stage_end && stage_vert->stage_pivot != stage_end) {
		/* the vertical stage is in the middle of the pipeline */
		struct video_stage_horz_struct* stage = stage_begin;
		struct video_stage_horz_struct* stage_blit = stage_end - 1;
		while (stage != stage_blit) {
			if (stage->buffer_size) {
				stage->buffer = video_buffer_alloc(stage->buffer_size);
			} else {
				stage->buffer = 0;
			}
			++stage;
		}
		stage_blit->buffer = 0;

		stage_vert->stage_pivot_sdp = stage_vert->stage_pivot->sdp;
		stage_vert->stage_pivot_sdx = stage_vert->stage_pivot->sdx;
		stage_vert->stage_pivot_sbpp = stage_vert->stage_pivot->sbpp;
	} else {
		/* the vertical stage is at the end of the pipeline */
		struct video_stage_horz_struct* stage = stage_begin;
		while (stage != stage_end) {
			if (stage->buffer_size) {
				stage->buffer = video_buffer_alloc(stage->buffer_size);
			} else {
				stage->buffer = 0;
			}
			++stage;
		}

		/* the pivot source is the last stage */
		stage_vert->stage_pivot_sdp = sdp;
		stage_vert->stage_pivot_sdx = sdx;
		stage_vert->stage_pivot_sbpp = sbpp;
	}

	/* allocate the extra buffer */
	{
		struct video_stage_horz_struct* stage = stage_begin;
		while (stage != stage_end) {
			if (stage->buffer_extra_size) {
				stage->buffer_extra = video_buffer_alloc(stage->buffer_extra_size);
			} else {
				stage->buffer_extra = 0;
			}

			++stage;
		}
	}

}

/* Run a partial pipeline */
static inline void* video_pipeline_run_partial(const struct video_stage_horz_struct* stage_begin, const struct video_stage_horz_struct* stage_end, void* src)
{
	if (stage_begin == stage_end) {
		return src;
	} else {
		stage_begin->put(stage_begin, stage_begin->buffer, src);
		++stage_begin;

		while (stage_begin != stage_end) {
			stage_begin->put(stage_begin, stage_begin->buffer, stage_begin[-1].buffer);
			++stage_begin;
		}

		return stage_begin[-1].buffer;
	}
}

/* Run a partial pipeline and store the result in the specified buffer */
static inline void* video_pipeline_run_partial_on_buffer(void* dst_buffer, const struct video_stage_horz_struct* stage_begin, const struct video_stage_horz_struct* stage_end, void* src)
{
	if (stage_begin == stage_end) {
		return src;
	} else {
		const struct video_stage_horz_struct* stage_next = stage_begin + 1;
		if (stage_next == stage_end) {
			stage_begin->put(stage_begin, dst_buffer, src);
		} else {
			stage_begin->put(stage_begin, stage_begin->buffer, src);
			++stage_begin;
			++stage_next;

			while (stage_next != stage_end) {
				stage_begin->put(stage_begin, stage_begin->buffer, stage_begin[-1].buffer);
				++stage_begin;
				++stage_next;
			}

			stage_begin->put(stage_begin, dst_buffer, stage_begin[-1].buffer);
		}

		return dst_buffer;
	}
}

static inline void video_pipeline_run(const struct video_stage_horz_struct* stage_begin, const struct video_stage_horz_struct* stage_end, void* dst, void* src)
{
	--stage_end;
	if (stage_begin == stage_end) {
		stage_begin->put(stage_begin, dst, src);
	} else {
		stage_begin->put(stage_begin, stage_begin->buffer, src);
		++stage_begin;

		while (stage_begin != stage_end) {
			stage_begin->put(stage_begin, stage_begin->buffer, stage_begin[-1].buffer);
			++stage_begin;
		}

		stage_begin->put(stage_begin, dst, stage_begin[-1].buffer);
	}
}

static inline void video_pipeline_run_plain(const struct video_stage_horz_struct* stage_begin, const struct video_stage_horz_struct* stage_end, void* dst, void* src)
{
	--stage_end;
	if (stage_begin == stage_end) {
		stage_begin->put_plain(stage_begin, dst, src);
	} else {
		stage_begin->put_plain(stage_begin, stage_begin->buffer, src);
		++stage_begin;

		while (stage_begin != stage_end) {
			stage_begin->put(stage_begin, stage_begin->buffer, stage_begin[-1].buffer);
			++stage_begin;
		}

		stage_begin->put(stage_begin, dst, stage_begin[-1].buffer);
	}
}

static inline void video_pipeline_vert_run(const struct video_pipeline_struct* pipeline, unsigned x, unsigned y, void* src)
{
	/* clear the states */
	const struct video_stage_horz_struct* begin = video_pipeline_begin(pipeline);
	const struct video_stage_horz_struct* end = video_pipeline_end(pipeline);
	while (begin != end) {
		assert(begin);
		((struct video_stage_horz_struct*)begin)->state_mutable = 0;
		++begin;
	}

	/* draw */
	assert(pipeline->stage_vert.put);
	assert(video_pipeline_vert(pipeline)->put);
	video_pipeline_vert(pipeline)->put(video_pipeline_vert(pipeline), x, y, src);

	/* restore the MMX micro state */
	internal_end();
}

const char* pipe_name(enum video_stage_enum pipe)
{
	switch (pipe) {
		case pipe_x_stretch : return "hstretch";
		case pipe_x_double : return "hcopy x2";
		case pipe_x_triple : return "hcopy x3";
		case pipe_x_quadruple : return "hcopy x4";
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
		case pipe_unchained : return "unchained hcopy";
		case pipe_unchained_palette16to8 : return "unchained hcopy palette 16>8";
		case pipe_unchained_x_double : return "unchained hcopy x2";
		case pipe_unchained_x_double_palette16to8  : return "unchained hcopy x2 palette 16>8";
		case pipe_palette8to8 : return "palette 8>8";
		case pipe_palette8to16 : return "palette 8>16";
		case pipe_palette8to32 : return "palette 8>32";
		case pipe_palette16to8 : return "palette 16>8";
		case pipe_palette16to16 : return "palette 16>16";
		case pipe_palette16to32 : return "palette 16>32";
		case pipe_bgr8888tobgr332 : return "bgr 8888>bgr 332";
		case pipe_bgr8888tobgr565 : return "bgr 8888>bgr 565";
		case pipe_bgr8888tobgr555 : return "bgr 8888>bgr 555";
		case pipe_bgr8888toyuy2 : return "bgr 8888>yuy2";
		case pipe_bgr555tobgr332 : return "bgr 555>bgr 332";
		case pipe_bgr555tobgr565 : return "bgr 555>bgr 565";
		case pipe_bgr555tobgr8888 : return "bgr 555>bgr 8888";
		case pipe_bgr555toyuy2 : return "bgr 555>yuy2";
		case pipe_rgb888tobgr8888 : return "rgb 888>bgr 8888";
		case pipe_y_copy : return "vcopy";
		case pipe_y_reduction_copy : return "vreduction";
		case pipe_y_expansion_copy : return "vexpansion";
		case pipe_y_mean : return "vcopy mean";
		case pipe_y_reduction_mean : return "vreduction mean";
		case pipe_y_expansion_mean : return "vexpansion mean";
		case pipe_y_filter : return "vcopy lowpass";
		case pipe_y_reduction_filter : return "vreduction low pass";
		case pipe_y_expansion_filter : return "vexpansion low pass";
		case pipe_y_reduction_max  : return "vreduction max";
		case pipe_y_scale2x : return "vhscale 2x";
	}
	return 0;
}

/* Check is the stage change the color format */
/* These stages MUST be BEFORE any RGB color operation */
static int pipe_is_conversion(enum video_stage_enum pipe)
{
	switch (pipe) {
		case pipe_palette8to8 :
		case pipe_palette8to16 :
		case pipe_palette8to32 :
		case pipe_palette16to8 :
		case pipe_palette16to16 :
		case pipe_palette16to32 :
		case pipe_unchained_palette16to8 :
		case pipe_unchained_x_double_palette16to8 :
		case pipe_bgr8888tobgr332 :
		case pipe_bgr8888tobgr565 :
		case pipe_bgr8888tobgr555 :
		case pipe_bgr8888toyuy2 :
		case pipe_bgr555tobgr332 :
		case pipe_bgr555tobgr565 :
		case pipe_bgr555tobgr8888 :
		case pipe_bgr555toyuy2 :
		case pipe_rgb888tobgr8888 :
		case pipe_rotation :
			return 1;
		default:
			return 0;
	}
}

/* Check is the stage decorate the image */
/* These stages MUST be AFTER any change of size */
static int pipe_is_decoration(enum video_stage_enum pipe)
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
			return 1;
		default:
			return 0;
	}
}

/* The write operation is done writing the biggest register size */
static inline int stage_is_fastwrite(const struct video_stage_horz_struct* stage)
{
	if (the_blit_mmx) {
		int is_plain = stage->sbpp == stage->sdp;
		switch (stage->type) {
			/* these use the MMX if are plain */
			case pipe_x_copy : return 1;
			case pipe_rotation : return 1;
			case pipe_x_double : return is_plain;
			case pipe_x_rgb_triad3pix : return is_plain;
			case pipe_x_rgb_triad6pix : return is_plain;
			case pipe_x_rgb_triad16pix : return is_plain;
			case pipe_x_rgb_triadstrong3pix : return is_plain;
			case pipe_x_rgb_triadstrong6pix : return is_plain;
			case pipe_x_rgb_triadstrong16pix : return is_plain;
			case pipe_x_rgb_scandoublehorz : return is_plain;
			case pipe_x_rgb_scantriplehorz : return is_plain;
			case pipe_x_rgb_scandoublevert : return is_plain;
			case pipe_x_rgb_scantriplevert : return is_plain;
			case pipe_swap_even : return 0;
			case pipe_swap_odd : return 0;
			case pipe_palette8to8 : return 0;
			case pipe_palette8to16 : return is_plain;
			case pipe_palette8to32 : return 0;
			case pipe_palette16to8 : return 1;
			case pipe_palette16to16 : return 1;
			case pipe_palette16to32 : return 1;
			case pipe_bgr8888tobgr332 : return is_plain;
			case pipe_bgr8888tobgr555 : return is_plain;
			case pipe_bgr8888tobgr565 : return is_plain;
			case pipe_bgr8888toyuy2 : return 1;
			case pipe_bgr555tobgr332 : return is_plain;
			case pipe_bgr555tobgr565 : return is_plain;
			case pipe_bgr555tobgr8888 : return is_plain;
			case pipe_bgr555toyuy2 : return 1;
			case pipe_rgb888tobgr8888 : return 0;
			case pipe_x_filter : return is_plain;
			/* unchained can't use the MMX */
			case pipe_unchained : return 1;
			case pipe_unchained_palette16to8 : return 1;
			case pipe_unchained_x_double : return 1;
			case pipe_unchained_x_double_palette16to8 : return 1;
			default:
			return 0;
		}
	} else {
		int is_plain = stage->sbpp == stage->sdp;
		switch (stage->type) {
			case pipe_x_stretch : return 0;
			default:
				return is_plain;
		}
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

static void video_stage_stretchy_x1(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);

	int whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	int count = stage_vert->slice.count;

	while (count) {
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = video_write_line(y) + x_off;
		video_pipeline_run(stage_vert->stage_begin, stage_vert->stage_end, dst, src);
		++y;

		PADD(src, stage_vert->sdw * run);
		--count;
	}
}

static void video_stage_stretchy_max_x1(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_vert->stage_begin->sdx * stage_vert->stage_begin->sbpp);

	int whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	int count = stage_vert->slice.count;

	while (count) {
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = video_write_line(y) + x_off;
		if (run == 1) {
			video_pipeline_run(stage_begin, stage_end, dst, src);
			PADD(src, stage_vert->sdw);
		} else {
			void* src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
			stage_copy(stage_pivot, buffer, src_buffer);
			PADD(src, stage_vert->sdw);
			--run;

			while (run) {
				src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
				if (index_is_rgb(video_index()))
					stage_max_rgb_vert_self(stage_pivot, buffer, src_buffer);
				else
					stage_max_vert_self(stage_pivot, buffer, src_buffer);
				PADD(src, stage_vert->sdw);
				--run;
			}

			video_pipeline_run_plain(stage_vert->stage_pivot, stage_end, dst, buffer);
		}
		++y;
		--count;
	}

	video_buffer_free(buffer);
}

/* Compute the mean of every lines reduced to a single line */
static void video_stage_stretchy_mean_x1(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);

	int whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	int count = stage_vert->slice.count;

	while (count) {
		void* dst;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = video_write_line(y) + x_off;
		if (run == 1) {
			video_pipeline_run(stage_begin, stage_end, dst, src);
			PADD(src, stage_vert->sdw);
		} else {
			void* src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
			stage_copy(stage_pivot, buffer, src_buffer);
			PADD(src, stage_vert->sdw);
			--run;

			while (run) {
				src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
				stage_mean_vert_self(stage_pivot, buffer, src_buffer);
				PADD(src, stage_vert->sdw);
				--run;
			}

			video_pipeline_run_plain(stage_pivot, stage_end, dst, buffer);
		}
		++y;
		--count;
	}

	video_buffer_free(buffer);
}

/* Compute the mean of the previous line and the first of every iteration */
static void video_stage_stretchy_filter_x1(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	int buffer_full = 0;
	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);

	int whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	int count = stage_vert->slice.count;

	while (count) {
		void* dst;
		void* src_buffer;
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		dst = video_write_line(y) + x_off;
		src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
		if (buffer_full) {
			stage_mean_vert_self(stage_pivot, buffer, src_buffer);
			video_pipeline_run_plain(stage_pivot, stage_end, dst, buffer);
		} else {
			video_pipeline_run(stage_pivot, stage_end, dst, src_buffer);
			buffer_full = 1;
		}

		if (count > 1) {
			if (run > 1) {
				PADD(src, (run - 1) * stage_vert->sdw);
				src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
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

static void video_stage_stretchy_1x(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);

	int whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	int count = stage_vert->slice.count;

	while (count) {
		void* buffer;

		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}

		buffer = video_pipeline_run_partial(stage_vert->stage_begin, stage_vert->stage_pivot, src);
		while (run) {
			void* dst = video_write_line(y) + x_off;
			video_pipeline_run(stage_vert->stage_pivot, stage_vert->stage_end, dst, buffer);
			++y;
			--run;
		}

		PADD(src, stage_vert->sdw);
		--count;
	}
}

/* The mean effect is applied only at the first added line, if no line */
/* duplication is done, no effect is applied */
static void video_stage_stretchy_mean_1x(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);
	void* previous_buffer = 0;

	int whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	int count = stage_vert->slice.count;

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

		src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
		dst = video_write_line(y) + x_off;

		/* don't apply the effect if no duplication is required */
		if (previous_buffer) {
			/* apply the mean effect only at the first line */
			stage_mean_vert_self(stage_pivot, buffer, src_buffer);
			video_pipeline_run_plain(stage_pivot, stage_end, dst, buffer);
		} else {
			video_pipeline_run(stage_pivot, stage_end, dst, src_buffer);
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
			dst = video_write_line(y) + x_off;
			video_pipeline_run(stage_pivot, stage_end, dst, src_buffer);
			++y;
			--run;
		}

		PADD(src, stage_vert->sdw);
		--count;
	}

	video_buffer_free(buffer);
}

/* The effect is applied at every line */
static void video_stage_stretchy_filter_1x(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* buffer = video_buffer_alloc(stage_pivot->sdx * stage_pivot->sbpp);
	void* previous_buffer = 0;

	int whole = stage_vert->slice.whole;
	int up = stage_vert->slice.up;
	int down = stage_vert->slice.down;
	int error = stage_vert->slice.error;
	int count = stage_vert->slice.count;

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

		src_buffer = video_pipeline_run_partial(stage_begin, stage_pivot, src);
		dst = video_write_line(y) + x_off;

		if (previous_buffer) {
			/* apply the mean effect only at the first line */
			stage_mean_vert_self(stage_pivot, buffer, src_buffer);
			video_pipeline_run_plain(stage_pivot, stage_end, dst, buffer);
		} else {
			video_pipeline_run(stage_pivot, stage_end, dst, src_buffer);
		}

		/* save always the current buffer (this is the difference from filter and mean) */
		previous_buffer = src_buffer;

		/* first line done */
		++y;
		--run;

		/* do other lines without any effects */
		while (run) {
			dst = video_write_line(y) + x_off;
			video_pipeline_run(stage_pivot, stage_end, dst, src_buffer);
			++y;
			--run;
		}

		PADD(src, stage_vert->sdw);
		--count;
	}

	video_buffer_free(buffer);
}

/***************************************************************************/
/* stretch scale 2x */

static void video_stage_stretchy_scale2x(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned x_off = video_offset(x);
	unsigned count = stage_vert->sdy;

	const struct video_stage_horz_struct* stage_begin = stage_vert->stage_begin;
	const struct video_stage_horz_struct* stage_end = stage_vert->stage_end;
	const struct video_stage_horz_struct* stage_pivot = stage_vert->stage_pivot;

	void* final0 = 0;
	void* final1 = 0;
	void* buffer[3];
	void* src0;
	void* src1;
	void* src2;
	void* partial0;
	void* partial1;
	void* partial2;
	int partial2_index;

	if (stage_pivot != stage_end) {
		final0 = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
		final1 = video_buffer_alloc(2 * stage_pivot->sdx * stage_pivot->sbpp);
	}

	buffer[0] = video_buffer_alloc(stage_vert->stage_pivot_sdx * stage_vert->stage_pivot_sbpp);
	buffer[1] = video_buffer_alloc(stage_vert->stage_pivot_sdx * stage_vert->stage_pivot_sbpp);
	buffer[2] = video_buffer_alloc(stage_vert->stage_pivot_sdx * stage_vert->stage_pivot_sbpp);

	src0 = src;
	src1 = src;
	src2 = src;

	PADD(src1, stage_vert->sdw);
	PADD(src2, stage_vert->sdw * 2);

	partial0 = video_pipeline_run_partial_on_buffer(buffer[0], stage_begin, stage_pivot, src0);
	partial1 = video_pipeline_run_partial_on_buffer(buffer[1], stage_begin, stage_pivot, src1);
	partial2_index = 2;

	/* first row */
	if (stage_pivot == stage_end) {
		void* dst0;
		void* dst1;

		/* first row */
		dst0 = video_write_line(y) + x_off;
		++y;
		dst1 = video_write_line(y) + x_off;
		++y;
		stage_scale2x_last(stage_vert, dst0, dst1, partial0, partial0, partial1);

		/* central rows */
		count -= 2;
		while (count) {
			partial2 = video_pipeline_run_partial_on_buffer(buffer[partial2_index], stage_begin, stage_pivot, src2);

			dst0 = video_write_line(y) + x_off;
			++y;
			dst1 = video_write_line(y) + x_off;
			++y;
			stage_scale2x_last(stage_vert, dst0, dst1, partial0, partial1, partial2);

			partial0 = partial1;
			partial1 = partial2;
			if (++partial2_index == 3)
				partial2_index = 0;
			PADD(src2, stage_vert->sdw);
			--count;
		}

		/* last row */
		dst0 = video_write_line(y) + x_off;
		++y;
		dst1 = video_write_line(y) + x_off;
		++y;
		stage_scale2x_last(stage_vert, dst0, dst1, partial0, partial1, partial1);
	} else {
		void* dst;

		/* first row */
		stage_scale2x(stage_pivot, final0, final1, partial0, partial0, partial1);
		dst = video_write_line(y) + x_off;
		video_pipeline_run_plain(stage_pivot, stage_end, dst, final0);
		++y;
		dst = video_write_line(y) + x_off;
		video_pipeline_run_plain(stage_pivot, stage_end, dst, final1);
		++y;

		/* central rows */
		count -= 2;
		while (count) {
			partial2 = video_pipeline_run_partial_on_buffer(buffer[partial2_index], stage_begin, stage_pivot, src2);

			stage_scale2x(stage_pivot, final0, final1, partial0, partial1, partial2);
			dst = video_write_line(y) + x_off;
			video_pipeline_run_plain(stage_pivot, stage_end, dst, final0);
			++y;
			dst = video_write_line(y) + x_off;
			video_pipeline_run_plain(stage_pivot, stage_end, dst, final1);
			++y;

			partial0 = partial1;
			partial1 = partial2;
			if (++partial2_index == 3)
				partial2_index = 0;
			PADD(src2, stage_vert->sdw);
			--count;
		}

		/* last row */
		stage_scale2x(stage_pivot, final0, final1, partial0, partial1, partial1);
		dst = video_write_line(y) + x_off;
		video_pipeline_run_plain(stage_pivot, stage_end, dst, final0);
		++y;
		dst = video_write_line(y) + x_off;
		video_pipeline_run_plain(stage_pivot, stage_end, dst, final1);
		++y;

	}

	video_buffer_free(buffer[2]);
	video_buffer_free(buffer[1]);
	video_buffer_free(buffer[0]);

	if (stage_pivot != stage_end) {
		video_buffer_free(final1);
		video_buffer_free(final0);
	}
}

/***************************************************************************/
/* stretchy copy */

static void video_stage_stretchy_11(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	unsigned count = stage_vert->sdy;
	unsigned x_off = video_offset(x);

	while (count) {
		void* dst;

		dst = video_write_line(y) + x_off;
		video_pipeline_run(stage_vert->stage_begin, stage_vert->stage_end, dst, src);
		++y;

		PADD(src, stage_vert->sdw);
		--count;
	}
}

/***************************************************************************/
/* stretchy plane */

/* Vertical wrapper for 4 plane mode unchained mode */
static void video_stage_planey4(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	uint8* src8 = (uint8*)src;
	unsigned p;

	for(p=0;p<4;++p) {
		video_unchained_plane_set(p);
		stage_vert->plane_put(stage_vert, x, y, src8 + p * stage_vert->stage_begin->sdp);
	}
}

/* Vertical wrapper for 2 plane mode unchained */
static void video_stage_planey2(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src)
{
	uint8* src8 = (uint8*)src;

	video_unchained_plane_mask_set(0x3);
	stage_vert->plane_put(stage_vert, x, y, src8);
	video_unchained_plane_mask_set(0xC);
	stage_vert->plane_put(stage_vert, x, y, src8 + stage_vert->stage_begin->sdp);
}

/* Modify the vertical stage for supporting the separated plane put */
static inline void video_stage_planey_set(struct video_stage_vert_struct* stage_vert)
{
	/* supported only if the pipeline has only one stage and this stage */
	/* support the plane put */
	if (stage_vert->stage_begin + 1 == stage_vert->stage_end
		&& stage_vert->stage_begin->plane_put) {
		stage_vert->plane_put = stage_vert->put;
		if (stage_vert->stage_begin->plane_num == 4) {
			stage_vert->put = video_stage_planey4;
		} else {
			stage_vert->put = video_stage_planey2;
		}
		((struct video_stage_horz_struct*)(stage_vert->stage_begin))->put = stage_vert->stage_begin->plane_put;
		((struct video_stage_horz_struct*)(stage_vert->stage_begin))->put_plain = stage_vert->stage_begin->plane_put_plain;
	}
}

/***************************************************************************/
/* stretchy */

/* set the pivot early in the pipeline */
static void video_stage_pivot_early_set(struct video_stage_vert_struct* stage_vert, int require_after_conversion)
{
	if (require_after_conversion) {
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
static void video_stage_pivot_late_set(struct video_stage_vert_struct* stage_vert, int require_final_stage)
{
	if (require_final_stage) {
		assert(stage_vert->stage_begin != stage_vert->stage_end);
		stage_vert->stage_pivot = stage_vert->stage_end - 1;
	} else {
		stage_vert->stage_pivot = stage_vert->stage_end;
	}
	while (stage_vert->stage_pivot != stage_vert->stage_begin
		&& pipe_is_decoration(stage_vert->stage_pivot[-1].type)) {
		--stage_vert->stage_pivot;
	}
}

/* Inizialize the vertical stage */
static void video_stage_stretchy_set(struct video_stage_vert_struct* stage_vert, const struct video_pipeline_struct* pipeline, unsigned ddy, unsigned sdy, int sdw, unsigned combine)
{
	unsigned combine_y = combine & VIDEO_COMBINE_Y_MASK;

	stage_vert->sdy = sdy;
	stage_vert->sdw = sdw;
	stage_vert->ddy = ddy;

	stage_vert->stage_begin = video_pipeline_begin(pipeline);
	stage_vert->stage_end = video_pipeline_end(pipeline);

	if (ddy == 2*sdy && combine_y == VIDEO_COMBINE_Y_SCALE2X) {
		/* scale 2x */
		slice_set(&stage_vert->slice, sdy, ddy);

		video_stage_pivot_early_set(stage_vert, 1);
		stage_vert->put = video_stage_stretchy_scale2x;
		stage_vert->type = pipe_y_scale2x;
	} else if (sdy < ddy) { /* y expansion */
		slice_set(&stage_vert->slice, sdy, ddy);

		switch (combine_y) {
			case VIDEO_COMBINE_Y_MEAN :
				video_stage_pivot_late_set(stage_vert, 1);
				stage_vert->put = video_stage_stretchy_mean_1x;
				stage_vert->type = pipe_y_expansion_mean;
				break;
			case VIDEO_COMBINE_Y_FILTER :
				video_stage_pivot_late_set(stage_vert, 1);
				stage_vert->put = video_stage_stretchy_filter_1x;
				stage_vert->type = pipe_y_expansion_filter;
				break;
			default:
				video_stage_pivot_late_set(stage_vert, 1);
				stage_vert->put = video_stage_stretchy_1x;
				stage_vert->type = pipe_y_expansion_copy;
				break;
		}
	} else if (sdy == ddy) { /* y copy */
		slice_set(&stage_vert->slice, sdy, ddy);

		switch (combine_y) {
			case VIDEO_COMBINE_Y_MEAN :
				video_stage_pivot_early_set(stage_vert, 1);
				stage_vert->put = video_stage_stretchy_mean_1x;
				stage_vert->type = pipe_y_mean;
				break;
			case VIDEO_COMBINE_Y_FILTER :
				video_stage_pivot_early_set(stage_vert, 1);
				stage_vert->put = video_stage_stretchy_filter_1x;
				stage_vert->type = pipe_y_filter;
				break;
			default:
				video_stage_pivot_early_set(stage_vert, 0);
				stage_vert->put = video_stage_stretchy_11;
				stage_vert->type = pipe_y_copy;
				break;
		}
	} else { /* y reduction */
		slice_set(&stage_vert->slice, sdy, ddy);

		switch (combine_y) {
			case VIDEO_COMBINE_Y_MAX :
				video_stage_pivot_early_set(stage_vert, 0);
				stage_vert->put = video_stage_stretchy_max_x1;
				stage_vert->type = pipe_y_reduction_max;
				break;
			case VIDEO_COMBINE_Y_MEAN :
				video_stage_pivot_early_set(stage_vert, 1);
				stage_vert->put = video_stage_stretchy_mean_x1;
				stage_vert->type = pipe_y_reduction_mean;
				break;
			case VIDEO_COMBINE_Y_FILTER :
				video_stage_pivot_early_set(stage_vert, 1);
				stage_vert->put = video_stage_stretchy_filter_x1;
				stage_vert->type = pipe_y_reduction_filter;
				break;
			default:
				video_stage_pivot_early_set(stage_vert, 0);
				stage_vert->put = video_stage_stretchy_x1;
				stage_vert->type = pipe_y_reduction_copy;
				break;
		}
	}

	/* try to activate the plane put if avaliable */
	if (combine_y == VIDEO_COMBINE_Y_NONE)
		video_stage_planey_set(stage_vert);

	if (index_is_rgb(video_index())) {
		if (combine_y == VIDEO_COMBINE_Y_MEAN || combine_y == VIDEO_COMBINE_Y_FILTER || (combine & VIDEO_COMBINE_X_FILTER)!=0)
			internal_mean_set();

		if ((combine & VIDEO_COMBINE_X_RGB_TRIAD3PIX)!=0 || (combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG3PIX)!=0)
			internal_rgb_triad3pix_set();

		if ((combine & VIDEO_COMBINE_X_RGB_TRIAD6PIX)!=0 || (combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG6PIX)!=0)
			internal_rgb_triad6pix_set();

		if ((combine & VIDEO_COMBINE_X_RGB_TRIAD16PIX)!=0 || (combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG16PIX)!=0)
			internal_rgb_triad16pix_set();

		if ((combine & VIDEO_COMBINE_X_RGB_SCANDOUBLEHORZ)!=0)
			internal_rgb_scandouble_set();

		if ((combine & VIDEO_COMBINE_X_RGB_SCANTRIPLEHORZ)!=0)
			internal_rgb_scantriple_set();

		if ((combine & VIDEO_COMBINE_X_RGB_SCANDOUBLEVERT)!=0)
			internal_rgb_scandoublevert_set();

		if ((combine & VIDEO_COMBINE_X_RGB_SCANTRIPLEVERT)!=0)
			internal_rgb_scantriplevert_set();

		if (combine_y == VIDEO_COMBINE_Y_MAX)
			internal_max_rgb_set();
	}
}

/* Inizialize the vertical stage for the no transformation special case */
static inline void video_stage_stretchy_11_set(struct video_stage_vert_struct* stage_vert, const struct video_pipeline_struct* pipeline, unsigned sdy, int sdw)
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

	/* try to activate the plane put if avaliable */
	video_stage_planey_set(stage_vert);
}

/***************************************************************************/
/* stretch */

static inline void video_pipeline_make(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned src_dx, int src_dp, unsigned combine)
{
	unsigned bytes_per_pixel = video_bytes_per_pixel();
	struct video_stage_horz_struct* end;
	unsigned combine_y = combine & VIDEO_COMBINE_Y_MASK;

	/* This flag requires that the last stage isn't a color conversion stage */
	/* Some vertical stretchings require a final stage. */
	/* If a vertical filtering is done in the y stretching the final stage can't */
	/* be a color conversion, because the filtering works on rgb values */
	int require_last_not_conversion = combine_y == VIDEO_COMBINE_Y_MEAN || combine_y == VIDEO_COMBINE_Y_FILTER;

	/* This flag requires a generic last stage */
	int require_last = require_last_not_conversion || combine_y != VIDEO_COMBINE_Y_SCALE2X;

	/* in x reduction the filter is applied before */
	if ((combine & VIDEO_COMBINE_X_FILTER) != 0
		&& src_dx > dst_dx) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_filter8_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
			case 2 : video_stage_filter16_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
			case 4 : video_stage_filter32_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	/* do the x stretch */
	if (dst_dx == 2*src_dx && combine_y == VIDEO_COMBINE_Y_SCALE2X) {
		/* the stretch is done by the y stage */
		src_dp = bytes_per_pixel;
	} else if (dst_dx != src_dx) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_stretchx8_set( video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp ); break;
			case 2 : video_stage_stretchx16_set( video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp ); break;
			case 4 : video_stage_stretchx32_set( video_pipeline_insert(pipeline), dst_dx, src_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	/* in x expansion the filter is applied after */
	if ((combine & VIDEO_COMBINE_X_FILTER)!=0
		&& src_dx <= dst_dx) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_filter8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_filter16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_filter32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIAD16PIX)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_triad16pix8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_triad16pix16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_triad16pix32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG16PIX)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_triadstrong16pix8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_triadstrong16pix16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_triadstrong16pix32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIAD6PIX)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_triad6pix8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_triad6pix16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_triad6pix32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG6PIX)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_triadstrong6pix8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_triadstrong6pix16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_triadstrong6pix32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIAD3PIX)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_triad3pix8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_triad3pix16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_triad3pix32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_TRIADSTRONG3PIX)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_triadstrong3pix8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_triadstrong3pix16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_triadstrong3pix32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANDOUBLEHORZ)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_scandouble8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_scandouble16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_scandouble32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANTRIPLEHORZ)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_scantriple8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_scantriple16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_scantriple32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANDOUBLEVERT)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_scandoublevert8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_scandoublevert16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_scandoublevert32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_X_RGB_SCANTRIPLEVERT)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rgb_scantriplevert8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_rgb_scantriplevert16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_rgb_scantriplevert32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_SWAP_EVEN)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_swapeven8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_swapeven16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_swapeven32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if ((combine & VIDEO_COMBINE_SWAP_ODD)!=0) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_swapodd8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 2 : video_stage_swapodd16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			case 4 : video_stage_swapodd32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	if (bytes_per_pixel == 1 && video_is_unchained()) {
		video_stage_unchained8_set( video_pipeline_insert(pipeline), dst_dx, src_dp );
	} else {
		/* add a dummy stage if required */
		if ((require_last && video_pipeline_size(pipeline) == 0) /* if the last stage is empty */
			|| (require_last_not_conversion && pipe_is_conversion(video_pipeline_end(pipeline)[-1].type)) /* if the last stage is a conversion and a conversion is not allowed as a last stage */
			|| (video_pipeline_size(pipeline) != 0 && !stage_is_fastwrite(&video_pipeline_end(pipeline)[-1]))  /* if the last stage is a slow memory write stage */
		) {
			switch (bytes_per_pixel) {
				case 1 : video_stage_copy8_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
				case 2 : video_stage_copy16_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
				case 4 : video_stage_copy32_set( video_pipeline_insert(pipeline), dst_dx, src_dp ); break;
			}
			src_dp = bytes_per_pixel;
		}
	}

	/* optimize */
	end = video_pipeline_end_mutable(pipeline);

	if (!require_last_not_conversion) {
		if (video_pipeline_size(pipeline) >= 2
			&& end[-1].type == pipe_unchained
			&& end[-2].type == pipe_palette16to8 && end[-2].sdp == 2) {
			video_stage_unchained8_palette16to8_set( video_pipeline_substitute(pipeline, end-2, end), end[-2].sdx, end[-2].palette );
			return;
		}

		if (video_pipeline_size(pipeline) >= 3
			&& end[-1].type == pipe_unchained
			&& end[-2].type == pipe_x_double
			&& end[-3].type == pipe_palette16to8 && end[-3].sdp == 2) {
			video_stage_unchained8_double_palette16to8_set( video_pipeline_substitute(pipeline, end-3, end), end[-3].sdx, end[-3].palette );
			return;
		}
	}

	if (video_pipeline_size(pipeline) >= 2
		&& end[-1].type == pipe_unchained
		&& end[-2].type == pipe_x_double) {
		video_stage_unchained8_double_set( video_pipeline_substitute(pipeline, end-2, end), end[-2].sdx, end[-2].sdp);
		return;
	}
}

adv_error video_stretch_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, adv_color_def src_color_def, unsigned combine)
{
	adv_color_def dst_color_def = video_color_def();
	unsigned bytes_per_pixel = video_bytes_per_pixel();

	video_pipeline_init(pipeline);

	/* conversion */
	if (src_color_def != dst_color_def) {
		/* rgb 888 formats */
		if (src_color_def == color_def_make_from_rgb_sizelenpos(4, 8, 16, 8, 8, 8, 0)
			|| src_color_def == color_def_make_from_rgb_sizelenpos(3, 8, 16, 8, 8, 8, 0)
			|| src_color_def == color_def_make_from_rgb_sizelenpos(4, 8, 0, 8, 8, 8, 16)
			|| src_color_def == color_def_make_from_rgb_sizelenpos(3, 8, 0, 8, 8, 8, 16)) {
			/* RGB to BGR and rotation */
			if (src_color_def == color_def_make_from_rgb_sizelenpos(4, 8, 0, 8, 8, 8, 16)
				|| src_color_def == color_def_make_from_rgb_sizelenpos(3, 8, 0, 8, 8, 8, 16)) {
				video_stage_rgb888tobgr8888_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_color_def = color_def_make_from_rgb_sizelenpos(4, 8, 16, 8, 8, 8, 0);
				src_dp = 4;
			}
			/* yuy2 can do rotation itself */
			if (dst_color_def != color_def_make(adv_color_type_yuy2)) {
				/* rotation */
				if (src_dp != 4) {
					video_stage_rot32_set( video_pipeline_insert(pipeline), src_dx, src_dp );
					src_dp = 4;
				}
			}
			/* conversion */
			if (dst_color_def == color_def_make_from_rgb_sizelenpos(1, 3, 5, 3, 2, 2, 0)) {
				video_stage_bgr8888tobgr332_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 1;
			} else if (dst_color_def == color_def_make_from_rgb_sizelenpos(2, 5, 10, 5, 5, 5, 0)) {
				video_stage_bgr8888tobgr555_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 2;
			} else if (dst_color_def == color_def_make_from_rgb_sizelenpos(2, 5, 11, 6, 5, 5, 0)) {
				video_stage_bgr8888tobgr565_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 2;
			} else if (dst_color_def == color_def_make_from_rgb_sizelenpos(4, 8, 16, 8, 8, 8, 0)) {
				/* nothing */
			} else if (dst_color_def == color_def_make(adv_color_type_yuy2)) {
				video_stage_bgr8888toyuy2_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 4;
			} else {
				video_pipeline_done(pipeline);
				return -1;
			}
		/* rgb 555 formats */
		} else if (src_color_def == color_def_make_from_rgb_sizelenpos(2, 5, 10, 5, 5, 5, 0)) {
			/* yuy2 can do rotation itself */
			if (dst_color_def != color_def_make(adv_color_type_yuy2)) {
				/* rotation */
				if (src_dp != 2) {
					video_stage_rot16_set( video_pipeline_insert(pipeline), src_dx, src_dp );
					src_dp = 2;
				}
			}
			/* conversion */
			if (dst_color_def == color_def_make_from_rgb_sizelenpos(1, 3, 5, 3, 2, 2, 0)) {
				video_stage_bgr555tobgr332_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 1;
			} else if (dst_color_def == color_def_make_from_rgb_sizelenpos(2, 5, 10, 5, 5, 5, 0)) {
				/* nothing */
			} else if (dst_color_def == color_def_make_from_rgb_sizelenpos(2, 5, 11, 6, 5, 5, 0)) {
				video_stage_bgr555tobgr565_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 2;
			} else if (dst_color_def == color_def_make_from_rgb_sizelenpos(4, 8, 16, 8, 8, 8, 0)) {
				video_stage_bgr555tobgr8888_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 4;
			} else if (dst_color_def == color_def_make(adv_color_type_yuy2)) {
				video_stage_bgr555toyuy2_set( video_pipeline_insert(pipeline), src_dx, src_dp );
				src_dp = 4;
			} else {
				video_pipeline_done(pipeline);
				return -1;
			}
		} else {
			video_pipeline_done(pipeline);
			return -1;
		}
	} else {
		/* rotation */
		if (src_dp != bytes_per_pixel) {
			switch (bytes_per_pixel) {
				case 1 : video_stage_rot8_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
				case 2 : video_stage_rot16_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
				case 4 : video_stage_rot32_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
			}
			src_dp = bytes_per_pixel;
		}
	}

	video_pipeline_make(pipeline, dst_dx, src_dx, src_dp, combine);

	video_stage_stretchy_set(video_pipeline_vert_mutable(pipeline), pipeline, dst_dy, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, src_dp, bytes_per_pixel);

	return 0;
}

void video_stretch_palette_hw_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned combine)
{
	unsigned bytes_per_pixel = video_bytes_per_pixel();

	video_pipeline_init(pipeline);

	/* rotation */
	if (src_dp != bytes_per_pixel) {
		switch (bytes_per_pixel) {
			case 1 : video_stage_rot8_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
			case 2 : video_stage_rot16_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
			case 4 : video_stage_rot32_set( video_pipeline_insert(pipeline), src_dx, src_dp ); break;
		}
		src_dp = bytes_per_pixel;
	}

	video_pipeline_make(pipeline, dst_dx, src_dx, src_dp, combine);

	video_stage_stretchy_set(video_pipeline_vert_mutable(pipeline), pipeline, dst_dy, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, src_dp, bytes_per_pixel);
}

void video_stretch_palette_8_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned* palette, unsigned combine)
{
	unsigned bytes_per_pixel = video_bytes_per_pixel();

	video_pipeline_init(pipeline);

	/* conversion and rotation */
	switch (bytes_per_pixel) {
		case 1 :
			video_stage_palette8to8_set( video_pipeline_insert(pipeline), src_dx, src_dp, palette );
			break;
		case 2 :
			video_stage_palette8to16_set( video_pipeline_insert(pipeline), src_dx, src_dp, palette );
			break;
		case 4 :
			video_stage_palette8to32_set( video_pipeline_insert(pipeline), src_dx, src_dp, palette );
			break;
	}
	src_dp = bytes_per_pixel;

	video_pipeline_make(pipeline, dst_dx, src_dx, src_dp, combine);

	video_stage_stretchy_set(video_pipeline_vert_mutable(pipeline), pipeline, dst_dy, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, src_dp, bytes_per_pixel);
}

void video_stretch_palette_16_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned* palette, unsigned combine)
{
	unsigned bytes_per_pixel = video_bytes_per_pixel();

	video_pipeline_init(pipeline);

	/* conversion and rotation */
	switch (bytes_per_pixel) {
		case 1 :
			video_stage_palette16to8_set( video_pipeline_insert(pipeline), src_dx, src_dp, palette );
			break;
		case 2 :
			video_stage_palette16to16_set( video_pipeline_insert(pipeline), src_dx, src_dp, palette );
			break;
		case 4 :
			video_stage_palette16to32_set( video_pipeline_insert(pipeline), src_dx, src_dp, palette );
			break;
	}
	src_dp = bytes_per_pixel;

	video_pipeline_make(pipeline, dst_dx, src_dx, src_dp, combine);

	video_stage_stretchy_set(video_pipeline_vert_mutable(pipeline), pipeline, dst_dy, src_dy, src_dw, combine);

	video_pipeline_realize(pipeline, src_dx, src_dp, bytes_per_pixel);
}

void video_blit_pipeline(const struct video_pipeline_struct* pipeline, unsigned dst_x, unsigned dst_y, void* src)
{
	video_pipeline_vert_run(pipeline, dst_x, dst_y, src);
}

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

#ifndef __BLIT_H
#define __BLIT_H

#include "video.h"

#ifdef __cplusplus
extern "C" {
#endif

struct video_slice {
	int whole; /* segment length */
	int up; /* up step */
	int down; /* down step */
	int count; /* number of segment */
	int error; /* start error */
};

void video_slice_init(struct video_slice* slice, unsigned sd, unsigned dd);

/***************************************************************************/
/* stage/pipeline */

struct video_stage_horz_struct;

typedef void video_stage_hook(const struct video_stage_horz_struct* stage, void* dst, void* src);

enum video_stage_enum {
	pipe_x_stretch,
	pipe_x_double,
	pipe_x_triple,
	pipe_x_quadruple,
	pipe_x_filter,
	pipe_x_copy,
	pipe_rotation,
	pipe_x_rgb_triad3pix,
	pipe_x_rgb_triad6pix,
	pipe_x_rgb_triad16pix,
	pipe_x_rgb_triadstrong3pix,
	pipe_x_rgb_triadstrong6pix,
	pipe_x_rgb_triadstrong16pix,
	pipe_x_rgb_scandoublehorz,
	pipe_x_rgb_scantriplehorz,
	pipe_x_rgb_scandoublevert,
	pipe_x_rgb_scantriplevert,
	pipe_unchained,
	pipe_unchained_palette16to8,
	pipe_unchained_x_double,
	pipe_unchained_x_double_palette16to8,
	pipe_palette8to8,
	pipe_palette8to16,
	pipe_palette8to32,
	pipe_palette16to8,
	pipe_palette16to16,
	pipe_palette16to32,
	pipe_rgb8888to332,
	pipe_rgb8888to565,
	pipe_rgb8888to555,
	pipe_rgb555to332,
	pipe_rgb555to565,
	pipe_rgb555to8888,
	pipe_rgbRGB888to8888,
	pipe_y_copy,
	pipe_y_reduction_copy,
	pipe_y_expansion_copy,
	pipe_y_mean,
	pipe_y_reduction_mean,
	pipe_y_expansion_mean,
	pipe_y_filter,
	pipe_y_expansion_filter,
	pipe_y_reduction_filter,
	pipe_y_reduction_max,
	pipe_y_scale2x
};

const char* pipe_name(enum video_stage_enum pipe_type);

/* Pipeline horizontal trasformation stage */
struct video_stage_horz_struct {
	/* hook */
	video_stage_hook* put;

	/* hook assuming sdp == sbpp */
	video_stage_hook* put_plain;

	/* type */
	enum video_stage_enum type;

	/* dest */
	void* buffer; /* eventually dest buffer */
	unsigned buffer_size; /* size of the dest buffer, ==0 if not required */

	/* source */
	int sdp; /* step in the src for the next pixel (in bytes) */
	unsigned sdx; /* size of the source (in bytes) */
	unsigned sbpp; /* bytes per pixel of the source */

	/* stretch slice */
	struct video_slice slice;

	/* palette conversion */
	unsigned* palette;

	/* unchained plane */
	unsigned plane_num; /* number of plane */
	video_stage_hook* plane_put; /* hook for single plane put, !=0 if plane put supported */
	video_stage_hook* plane_put_plain; /* hook assuming sdp == sbpp */

	/* state */
	unsigned state_mutable; /* state value zeroed at the startup */
};

/* Special effect */
#define VIDEO_COMBINE_Y_NONE 0 /* no effect */
#define VIDEO_COMBINE_Y_MAX 0x1 /* use the max value in y reductions */
#define VIDEO_COMBINE_Y_MEAN 0x2 /* use the mean value in y transformations for added or removed lines */
#define VIDEO_COMBINE_Y_FILTER 0x3 /* apply a FIR lowpass filter with 2 point and fc 0.5 in the y direction */
#define VIDEO_COMBINE_Y_SCALE2X 0x4 /* scale 2x */
#define VIDEO_COMBINE_Y_MASK 0x7 /* mask for the Y effect */

#define VIDEO_COMBINE_X_FILTER 0x8 /* apply a FIR lowpass filter with 2 point and fc 0.5 in the x direction */
#define VIDEO_COMBINE_X_RGB_TRIAD3PIX 0x10 /* rgb triad filter 3 pixel mask */
#define VIDEO_COMBINE_X_RGB_TRIAD6PIX 0x20 /* rgb triad filter 6 pixel mask */
#define VIDEO_COMBINE_X_RGB_TRIAD16PIX 0x40 /* rgb triad filter 16 pixel mask */
#define VIDEO_COMBINE_X_RGB_TRIADSTRONG3PIX 0x80 /* rgb triad strong filter 3 pixel mask */
#define VIDEO_COMBINE_X_RGB_TRIADSTRONG6PIX 0x100 /* rgb triad strong filter 6 pixel mask */
#define VIDEO_COMBINE_X_RGB_TRIADSTRONG16PIX 0x200 /* rgb triad strong filter 16 pixel mask */
#define VIDEO_COMBINE_X_RGB_SCANDOUBLEHORZ 0x400 /* double scanline */
#define VIDEO_COMBINE_X_RGB_SCANTRIPLEHORZ 0x800 /* triple scanline */
#define VIDEO_COMBINE_X_RGB_SCANDOUBLEVERT 0x1000 /* double scanline vertical */
#define VIDEO_COMBINE_X_RGB_SCANTRIPLEVERT 0x2000 /* triple scanline vertical */

struct video_stage_vert_struct;

typedef void video_stage_vert_hook(const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, void* src);

/* Pipeline vertical trasformation stage */
struct video_stage_vert_struct {
	/* hook */
	video_stage_vert_hook* put;

	/* type */
	enum video_stage_enum type;

	/* source */
	unsigned sdy; /* source vertical size in rows */
	unsigned sdw; /* source row size in bytes */

	/* dest */
	unsigned ddy; /* destination vertical size in rows */
	/* the destination row size is always the minimun */

	/* stretch slice */
	struct video_slice slice;

	/* pipeline */
	const struct video_stage_horz_struct* stage_begin;
	const struct video_stage_horz_struct* stage_end;

	/* stage used for splitting the pipeline, on the split is applied the vertical stage */
	const struct video_stage_horz_struct* stage_pivot;

	/* information from the stage_pivot (used if the pipeline is empty) */
	int stage_pivot_sdp; /* step in the src for the next pixel (in bytes) */
	unsigned stage_pivot_sdx; /* size of the source (in bytes) */
	unsigned stage_pivot_sbpp; /* bytes per pixel of the source */

	/* unchained plane */
	video_stage_vert_hook* plane_put; /* hook for plane put, !=0 if plane put supported */
};

#define VIDEO_STAGE_MAX 8

struct video_pipeline_struct {
	struct video_stage_vert_struct stage_vert;
	struct video_stage_horz_struct stage_map[VIDEO_STAGE_MAX];
	unsigned stage_mac;
};

void video_pipeline_init(struct video_pipeline_struct* pipeline);
void video_pipeline_done(struct video_pipeline_struct* pipeline);

static __inline__ unsigned video_pipeline_size(const struct video_pipeline_struct* pipeline) {
	return pipeline->stage_mac;
}

static __inline__ const struct video_stage_horz_struct* video_pipeline_begin(const struct video_pipeline_struct* pipeline) {
	return pipeline->stage_map;
}

static __inline__ const struct video_stage_horz_struct* video_pipeline_end(const struct video_pipeline_struct* pipeline) {
	return pipeline->stage_map + pipeline->stage_mac;
}

static __inline__ const struct video_stage_vert_struct* video_pipeline_vert(const struct video_pipeline_struct* pipeline) {
	return &pipeline->stage_vert;
}

static __inline__ const struct video_stage_horz_struct* video_pipeline_pivot(const struct video_pipeline_struct* pipeline) {
	return video_pipeline_vert(pipeline)->stage_pivot;
}

/***************************************************************************/
/* blit */

void video_stretch_init(void);
void video_stretch_done(void);

/***************************************************************************/
/* pipeline blit */

int video_stretch_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, video_rgb_def src_rgb_def, unsigned combine);
void video_stretch_palette_hw_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned combine);
void video_stretch_palette_8_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned* palette, unsigned combine);
void video_stretch_palette_16_pipeline_init(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned* palette, unsigned combine);
void video_blit_pipeline(const struct video_pipeline_struct* pipeline, unsigned dst_x, unsigned dst_y, void* src);

/***************************************************************************/
/* blit */

void video_clear(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, unsigned src);

static __inline__ int video_stretch(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, void* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, video_rgb_def src_rgb_def, unsigned combine) {
	struct video_pipeline_struct pipeline;

	if (video_stretch_pipeline_init(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, src_rgb_def, combine) != 0)
		return -1;

	video_blit_pipeline(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);

	return 0;
}

static __inline__ void video_stretch_palette_hw(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, void* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned combine) {
	struct video_pipeline_struct pipeline;

	video_stretch_palette_hw_pipeline_init(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, combine);

	video_blit_pipeline(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);
}

static __inline__ void video_stretch_palette_8(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, uint8* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned* palette, unsigned combine) {
	struct video_pipeline_struct pipeline;

	video_stretch_palette_8_pipeline_init(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, palette, combine);

	video_blit_pipeline(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);
}

static __inline__ void video_stretch_palette_16(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, uint16* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned* palette, unsigned combine) {
	struct video_pipeline_struct pipeline;

	video_stretch_palette_16_pipeline_init(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, palette, combine);

	video_blit_pipeline(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);
}

int video_blit_set_mmx(int mmx_version);

#define STAGE_EX(stage,_type,_sdx,_sdp,_sbpp,_dbpp,_palette) \
	stage->type = (_type); \
	stage->sdx = (_sdx); \
	stage->sdp = (_sdp); \
	stage->sbpp = (_sbpp); \
	video_slice_init(&stage->slice,_sdx,_sdx); \
	stage->buffer_size = (_dbpp)*(_sdx); \
	stage->palette = _palette

#define STAGE_SIZE(stage,_type,_sdx,_sdp,_sbpp,_dbpp) \
	STAGE_EX(stage,_type,_sdx,_sdp,_sbpp,_dbpp,0)

#define STAGE(stage,_type,_sdx,_sdp,_sbpp,_dbpp,_put_plain,_put) \
	STAGE_EX(stage,_type,_sdx,_sdp,_sbpp,_dbpp,0); \
	stage->plane_put_plain = 0; \
	stage->plane_put = 0; \
	stage->put_plain = (_put_plain); \
	stage->put = (stage->sbpp == stage->sdp) ? stage->put_plain : (_put)

#define STAGE_PALETTE(stage,_type,_sdx,_sdp,_sbpp,_dbpp,_palette,_put_plain,_put) \
	STAGE_EX(stage,_type,_sdx,_sdp,_sbpp,_dbpp,_palette); \
	stage->plane_put_plain = 0; \
	stage->plane_put = 0; \
	stage->put_plain = (_put_plain); \
	stage->put = (stage->sbpp == stage->sdp) ? stage->put_plain : (_put)

#ifdef __cplusplus
}
#endif

#endif

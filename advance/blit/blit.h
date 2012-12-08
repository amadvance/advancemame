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

/** \file
 * Blit.
 */

/** \addtogroup Blit */
/*@{*/

#ifndef __BLIT_H
#define __BLIT_H

#include "video.h"
#include "slice.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* stage/pipeline */

struct video_stage_horz_struct;

/**
 * Template for an horizontal pipeline stage.
 * These functions are called to draw a single image row.
 * \param stage Stage context. This context contains all the necessary and constant
 * information for the drawing process.
 * \param line Line number.
 * \param dst Destination data.
 * \param src Source data.
 * \param count Size of the source line (in elementary operations on source, usually slice->count).
 */
typedef void video_stage_hook(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count);

/**
 * Pipeline stage types.
 */
enum video_stage_enum {
	pipe_x_stretch, /**< Horizontal stretch by a fractional factor. */
	pipe_x_maxmin, /**< Horizontal stretch by a fractional factor with the maxmin effect. */
	pipe_x_mean, /**< Horizontal stretch by a fractional factor with the mean effect. */
	pipe_x_double, /**< Horizontal stretch by double factor. */
	pipe_x_triple, /**< Horizontal stretch by triple factor. */
	pipe_x_quadruple, /**< Horizontal stretch by quadruple factor. */
	pipe_x_filter, /**< Horizontal filter. */
	pipe_x_copy, /**< Horizontal copy. */
	pipe_rotation, /**< Horizontal copy with a not standard pixel step. */
	pipe_x_rgb_triad3pix, /**< RGB triad of 3 pixel. */
	pipe_x_rgb_triad6pix, /**< RGB triad of 6 pixel. */
	pipe_x_rgb_triad16pix, /**< RGB triad of 16 pixel. */
	pipe_x_rgb_triadstrong3pix, /**< RGB strong triad of 3 pixel. */
	pipe_x_rgb_triadstrong6pix, /**< RGB strong triad of 6 pixel. */
	pipe_x_rgb_triadstrong16pix, /**< RGB strong triad of 16 pixel. */
	pipe_x_rgb_scandoublehorz, /**< Horizontal double scanline. */
	pipe_x_rgb_scantriplehorz, /**< Horizontal triple scanline. */
	pipe_x_rgb_scandoublevert, /**< Vertical double scanline. */
	pipe_x_rgb_scantriplevert, /**< Vertical triple scanline. */
	pipe_swap_even, /**< Swap every two even rows. */
	pipe_swap_odd, /**< Swap every two odd rows. */
	pipe_interlace_filter, /**< Interlace filter. */
	pipe_palette8to8, /**< Palette conversion 8 -\> 8. */
	pipe_palette8to16, /**< Palette conversion 8 -\> 16. */
	pipe_palette8to32, /**< Palette conversion 8 -\> 32. */
	pipe_palette16to8, /**< Palette conversion 16 -\> 8. */
	pipe_palette16to16, /**< Palette conversion 16 -\> 16. */
	pipe_palette16to32, /**< Palette conversion 16 -\> 32. */
	pipe_imm16to8, /**< Immediate conversion 16 -\> 8. */
	pipe_imm16to32, /**< Immediate conversion 16 -\> 32. */
	pipe_bgra8888tobgr332, /**< RGB conversion 8888 (bgra) -\> 332 (bgr). */
	pipe_bgra8888tobgra5551, /**< RGB conversion 8888 (bgra) -\> 5551 (bgra). */
	pipe_bgra8888tobgr565, /**< RGB conversion 8888 (bgra) -\> 565 (bgr). */
	pipe_bgra8888toyuy2, /**< YUY2 conversion 8888 (bgra) -\> (yuy2). */
	pipe_bgra5551tobgr332, /**< RGB conversion 5551 (bgra) -\> 332 (bgr). */
	pipe_bgra5551tobgr565, /**< RGB conversion 5551 (bgra) -\> 565 (bgr). */
	pipe_bgra5551tobgra8888, /**< RGB conversion 5551 (bgra) -\> 8888 (bgra). */
	pipe_bgra5551toyuy2, /**< YUY2 conversion 5551 (bgra) -\> (yuy2). */
	pipe_rgb888tobgra8888, /**< RGB conversion 888 (rgb) -\> 8888 (bgra). */
	pipe_rgba8888tobgra8888, /**< RGB conversion 8888 (rgba) -\> 8888 (bgra). */
	pipe_bgr888tobgra8888, /**< RGB conversion 888 (bgr) -\> 8888 (bgra). */
	pipe_rgbtorgb, /**< Generic RGB conversion. */
	pipe_rgbtoyuy2, /**< Generic YUY2 conversion. */
	pipe_y_copy, /**< Vertical copy. */
	pipe_y_mean, /**< Vertical mean. */
	pipe_y_filter, /**< Vertical FIR filter. */
	pipe_y_maxmin, /**< Vertical maxmin. */
	pipe_y_scale2x, /**< Scale2x. */
	pipe_y_scale2x3, /**< Scale2x3. */
	pipe_y_scale2x4, /**< Scale2x4. */
	pipe_y_scale3x, /**< Scale3x. */
	pipe_y_scale4x, /**< Scale4x. */
	pipe_y_scale2k, /**< Scale2k. */
	pipe_y_scale3k, /**< Scale3k. */
	pipe_y_scale4k, /**< Scale4k. */
	pipe_y_lq2x, /**< LQ2x */
	pipe_y_lq2x3, /**< LQ2x3 */
	pipe_y_lq2x4, /**< LQ2x4 */
	pipe_y_lq3x, /**< LQ3x */
	pipe_y_lq4x, /**< LQ4x */
	pipe_y_hq2x, /**< HQ2x */
	pipe_y_hq2x3, /**< HQ2x3 */
	pipe_y_hq2x4, /**< HQ2x4 */
	pipe_y_hq3x, /**< HQ3x */
	pipe_y_hq4x, /**< HQ4x */
	pipe_y_xbr2x, /**< XBR2x */
	pipe_y_xbr3x, /**< XBR3x */
	pipe_y_xbr4x /**< XBR4x */
};

/**
 * Get the name of a pipeline stage type.
 * \return Pointer at a static buffer.
 */
const char* pipe_name(enum video_stage_enum pipe_type);

/**
 * Size in 32 bit word of the blit data stage.
 */
#define VIDEO_DATA_MAX 48

/**
 * Pipeline horizontal trasformation stage.
 */
struct __attribute__((aligned(8))) video_stage_horz_struct {
	/**
	 * Private data of the stage.
	 */
	uint32 data[VIDEO_DATA_MAX];

	/**
	 * Blit function.
	 * This function does the configured blit operation on the arguments.
	 */
	video_stage_hook* put;

	/**
	 * Blit function assuming a plain draw.
	 * This function operates like the ::put function but assuming ::sdp == ::sbpp.
	 * Note that the real ::sdp value must be ignored.
	 */
	video_stage_hook* put_plain;

	/* type */
	enum video_stage_enum type;

	/* dest */
	void* buffer; /**< Eventually dest buffer. */
	unsigned buffer_size; /**< Size of the dest buffer, ==0 if not required. */

	void* buffer_extra; /**< Eventually extra buffer. */
	unsigned buffer_extra_size; /**< Size of the extra buffer, ==0 if not required. */

	/* source */
	int sdp; /**< Step in the src for the next pixel (in bytes). */
	unsigned sdx; /**< Size of the source (in pixels). */
	unsigned ddx; /**< Size of the destination (in pixels). */
	unsigned sbpp; /**< Size of the source pixel (in bytes). */
	unsigned dbpp; /**< Size of the destination pixel (in bytes). */

	int red_shift, green_shift, blue_shift; /**< Shifts used for color conversion. */
	adv_pixel red_mask, green_mask, blue_mask; /**< Masks used for color conversion. */
	unsigned ssp; /**< Size of the source pixel used for color conversion (in bytes). Equal at ::sbpp. */
	unsigned dsp; /**< Size of the destination pixel used for color conversion (in bytes). */

	adv_slice slice; /**< Slice used in streching. */
#ifdef USE_SEGMENT
	adv_segment segment; /**< Segment used to splitting. */
#endif
	const void* palette; /**< Palette used in conversion. The palette size depends on the conversion. */
};

/** \name Effects */
/*@{*/
#define VIDEO_COMBINE_Y_NONE 0 /**< No effect. */
#define VIDEO_COMBINE_Y_MAXMIN 0x1 /**< Use the max value in y reductions. */
#define VIDEO_COMBINE_Y_MEAN 0x2 /**< Use the mean value in y transformations for added or removed lines. */
#define VIDEO_COMBINE_Y_FILTER 0x3 /**< Apply a FIR lowpass filter with 2 point and fc 0.5 in the y direction. */
#ifndef USE_BLIT_TINY
#define VIDEO_COMBINE_Y_SCALEX 0x4 /**< ScaleX. */
#define VIDEO_COMBINE_Y_SCALEK 0x5 /**< ScaleK. */
#define VIDEO_COMBINE_Y_LQ 0x6 /**< LQ. */
#ifndef USE_BLIT_SMALL
#define VIDEO_COMBINE_Y_HQ 0x7 /**< HQ. */
#define VIDEO_COMBINE_Y_XBR 0x8 /**< XBR. */
#endif
#endif
#define VIDEO_COMBINE_Y_MASK 0xF /**< Mask for the Y effect. */

#define VIDEO_COMBINE_X_FILTER 0x10 /**< Apply a FIR lowpass filter with 2 point and fc 0.5 in the x direction. */
#ifndef USE_BLIT_TINY
#define VIDEO_COMBINE_X_RGB_TRIAD3PIX 0x20 /**< Rgb triad filter 3 pixel mask. */
#define VIDEO_COMBINE_X_RGB_TRIAD6PIX 0x40 /**< Rgb triad filter 6 pixel mask. */
#define VIDEO_COMBINE_X_RGB_TRIAD16PIX 0x80 /**< Rgb triad filter 16 pixel mask. */
#define VIDEO_COMBINE_X_RGB_TRIADSTRONG3PIX 0x100 /**< Rgb triad strong filter 3 pixel mask. */
#define VIDEO_COMBINE_X_RGB_TRIADSTRONG6PIX 0x200 /**< Rgb triad strong filter 6 pixel mask. */
#define VIDEO_COMBINE_X_RGB_TRIADSTRONG16PIX 0x400 /**< Rgb triad strong filter 16 pixel mask. */
#define VIDEO_COMBINE_X_RGB_SCANDOUBLEHORZ 0x800 /**< Double scanline. */
#define VIDEO_COMBINE_X_RGB_SCANTRIPLEHORZ 0x1000 /**< Triple scanline. */
#define VIDEO_COMBINE_X_RGB_SCANDOUBLEVERT 0x2000 /**< Double scanline vertical. */
#define VIDEO_COMBINE_X_RGB_SCANTRIPLEVERT 0x4000 /**< Triple scanline vertical. */
#endif

#define VIDEO_COMBINE_SWAP_EVEN 0x8000 /**< Swap every two even line. */
#define VIDEO_COMBINE_SWAP_ODD 0x10000 /**< Swap every two odd line. */
#define VIDEO_COMBINE_X_MAXMIN 0x20000 /**< Horizontal stretch using the max effect */
#define VIDEO_COMBINE_X_MEAN 0x40000 /**< Horizontal stretch using the mean effect */
#define VIDEO_COMBINE_INTERLACE_FILTER 0x80000 /**< Vertical filter for interlace. */
#define VIDEO_COMBINE_BUFFER 0x100000 /**< Output to a memory buffer. */

/*@}*/

struct video_stage_vert_struct;

/**
 * Pipeline target.
 */
struct video_pipeline_target_struct {
	unsigned char* (*line)(const struct video_pipeline_target_struct* target, unsigned y);
	void* ptr;
	adv_color_def color_def;
	unsigned bytes_per_pixel;
	unsigned bytes_per_scanline;
};

/**
 * Template for a vertical pipeline stage.
 */
typedef void video_stage_vert_hook(const struct video_pipeline_target_struct* target, const struct video_stage_vert_struct* stage_vert, unsigned x, unsigned y, const void* src);

/**
 * Pipeline vertical trasformation stage.
 */
struct __attribute__((aligned(8))) video_stage_vert_struct {
	video_stage_vert_hook* put; /**< Hook. */

	/* type */
	enum video_stage_enum type;

	/* source */
	unsigned sdx;
	unsigned sdy; /**< Source vertical size in rows. */
	int sdw; /**< Source row size (in bytes). It may be negative for flip operations. */

	unsigned interp; /**< Interpolation type in the vert stage. */

	unsigned ddx;
	unsigned ddy; /**< Destination vertical size in rows. */

	unsigned bpp;

	/* stretch slice */
	adv_slice slice;

	/* pipeline */
	const struct video_stage_horz_struct* stage_begin;
	const struct video_stage_horz_struct* stage_end;

	/**
	 * Stage used for splitting the pipeline.
	 * On the split is applied the vertical stage.
	 */
	const struct video_stage_horz_struct* stage_pivot;

#ifdef USE_SEGMENT
	adv_segment segment; /**< Segment used to split stage. */
#endif
};

/**
 * Max number of stages in a blit pipeline.
 */
#define VIDEO_STAGE_MAX 8

/**
 * Blit pipeline.
 * A blit pipeline is a sequence of blit stages which operates on the images pixels.
 * The stages are differentiated in horizontal and vertical stages.
 * Many horizontal stages are allowed in the same pipeline. The vertical stage is always only one.
 *
 * The horizontal stages operate on rows. They can change the row length but they cannot
 * change the number of rows.
 * The vertical stage instead can change anything.
 *
 * The vertical stage cannot be the last stage in the pipeline, the horizontal stage
 * immediatly after the vertical stage is called "pivot" stage.
 */
struct __attribute__((aligned(8))) video_pipeline_struct {
	struct video_stage_horz_struct stage_map[VIDEO_STAGE_MAX]; /**< Horizontal stages. */
	struct video_stage_vert_struct stage_vert; /**< Vertical stage. */
	unsigned stage_mac; /**< Number of horizontal stages. */
	struct video_pipeline_target_struct target; /**< Target of the pipeline. */
};

/**
 * Initialize an empty blit pipeline.
 */
void video_pipeline_init(struct video_pipeline_struct* pipeline);

/**
 * Set the target of the pipeline.
 * The default target is the screen.
 */
void video_pipeline_target(struct video_pipeline_struct* pipeline, void* ptr, unsigned bytes_per_scanline, adv_color_def def);

/**
 * Deinitialize a blit pipeline.
 */
void video_pipeline_done(struct video_pipeline_struct* pipeline);

/**
 * Get the number of the horizontal stage in a blit pipeline.
 */
static inline unsigned video_pipeline_size(const struct video_pipeline_struct* pipeline)
{
	return pipeline->stage_mac;
}

/**
 * Get the first horizontal stage of a blit pipeline.
 */
static inline const struct video_stage_horz_struct* video_pipeline_begin(const struct video_pipeline_struct* pipeline)
{
	return pipeline->stage_map;
}

/**
 * Get the last horizontal stage in a blit pipeline.
 */
static inline const struct video_stage_horz_struct* video_pipeline_end(const struct video_pipeline_struct* pipeline)
{
	return pipeline->stage_map + pipeline->stage_mac;
}

/**
 * Get vertical stage of a blit pipeline.
 */
static inline const struct video_stage_vert_struct* video_pipeline_vert(const struct video_pipeline_struct* pipeline)
{
	return &pipeline->stage_vert;
}

/**
 * Get the horizontal pivot stage of a blit pipeline.
 */
static inline const struct video_stage_horz_struct* video_pipeline_pivot(const struct video_pipeline_struct* pipeline)
{
	return video_pipeline_vert(pipeline)->stage_pivot;
}

/***************************************************************************/
/* blit */

/**
 * Initialize the blit system.
 */
adv_error video_blit_init(void);

/**
 * Deinitialize the blit system.
 */
void video_blit_done(void);

/***************************************************************************/
/* pipeline blit */

/**
 * Setup a pipeline for a stretch blit.
 * Check the video_stretch() for more details.
 */
void video_pipeline_direct(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, adv_color_def src_color_def, unsigned combine);

/**
 * Setup a pipeline for a stretch blit with an hardware palette.
 * Check the video_stretch_palette_16hw() for more details.
 */
void video_pipeline_palette16hw(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned combine);

/**
 * Setup a pipeline for a stretch blit with a software 8 bit palette.
 * Check the video_stretch_palette_8() for more details.
 */
void video_pipeline_palette8(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, const uint8* palette8, const uint16* palette16, const uint32* palette32, unsigned combine);

/**
 * Setup a pipeline for a stretch blit with a software 16 bit palette.
 * Check the video_stretch_palette_16() for more details.
 */
void video_pipeline_palette16(struct video_pipeline_struct* pipeline, unsigned dst_dx, unsigned dst_dy, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, const uint8* palette8, const uint16* palette16, const uint32* palette32, unsigned combine);

/**
 * Blit using a precomputed pipeline.
 * \param pipeline Pipeline to use.
 * \param dst_x Destination x.
 * \param dst_y Destination y.
 * \param src Source data.
 */
void video_pipeline_blit(const struct video_pipeline_struct* pipeline, unsigned dst_x, unsigned dst_y, const void* src);

/***************************************************************************/
/* blit */

/**
 * Blit and stretch a bitmap.
 * The source bitmap can have any bit depth.
 * \param dst_x Destination x.
 * \param dst_y Destination y.
 * \param dst_dx Destination size x.
 * \param dst_dy Destination size y.
 * \param src Source data.
 * \param src_dx Source size x.
 * \param src_dy Source size y.
 * \param src_dw Source row step expressed in bytes.
 * \param src_dp Source pixel step expressed in bytes.
 * \param src_color_def Source color format.
 * \param combine Effect mask. A combination of the VIDEO_COMBINE codes.
 */
static inline void video_stretch_direct(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, const void* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, adv_color_def src_color_def, unsigned combine)
{
	struct video_pipeline_struct pipeline;

	video_pipeline_init(&pipeline);

	video_pipeline_direct(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, src_color_def, combine);

	video_pipeline_blit(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);
}

/**
 * Blit and stretch a bitmap with an hardware palette.
 * The source bitmap must be a 16 bit bitmap.
 * \param dst_x Destination x.
 * \param dst_y Destination y.
 * \param dst_dx Destination size x.
 * \param dst_dy Destination size y.
 * \param src Source data.
 * \param src_dx Source size x.
 * \param src_dy Source size y.
 * \param src_dw Source row step expressed in bytes.
 * \param src_dp Source pixel step expressed in bytes.
 * \param combine Effect mask. A combination of the VIDEO_COMBINE codes.
 */
static inline void video_stretch_palette16hw(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, const void* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, unsigned combine)
{
	struct video_pipeline_struct pipeline;

	video_pipeline_init(&pipeline);

	video_pipeline_palette16hw(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, combine);

	video_pipeline_blit(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);
}

/**
 * Blit and stretch a bitmap with a software 8 bit palette.
 * The source bitmap must be a 8 bit bitmap.
 * \param dst_x Destination x.
 * \param dst_y Destination y.
 * \param dst_dx Destination size x.
 * \param dst_dy Destination size y.
 * \param src Source data.
 * \param src_dx Source size x.
 * \param src_dy Source size y.
 * \param src_dw Source row step expressed in bytes.
 * \param src_dp Source pixel step expressed in bytes.
 * \param palette8, palette16, palette32 Palette data in different format.
 * \param combine Effect mask. A combination of the VIDEO_COMBINE codes.
 */
static inline void video_stretch_palette8(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, const uint8* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, const uint8* palette8, const uint16* palette16, const uint32* palette32, unsigned combine)
{
	struct video_pipeline_struct pipeline;

	video_pipeline_init(&pipeline);

	video_pipeline_palette8(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, palette8, palette16, palette32, combine);

	video_pipeline_blit(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);
}

/**
 * Blit and stretch a bitmap with a software 16 bit palette.
 * The source bitmap must be a 16 bit bitmap.
 * \param dst_x Destination x.
 * \param dst_y Destination y.
 * \param dst_dx Destination size x.
 * \param dst_dy Destination size y.
 * \param src Source data.
 * \param src_dx Source size x.
 * \param src_dy Source size y.
 * \param src_dw Source row step expressed in bytes.
 * \param src_dp Source pixel step expressed in bytes.
 * \param palette8, palette16, palette32 Palette data in different format.
 * \param combine Effect mask. A combination of the VIDEO_COMBINE codes.
 */
static inline void video_stretch_palette16(unsigned dst_x, unsigned dst_y, unsigned dst_dx, unsigned dst_dy, const uint16* src, unsigned src_dx, unsigned src_dy, int src_dw, int src_dp, const uint8* palette8, const uint16* palette16, const uint32* palette32, unsigned combine)
{
	struct video_pipeline_struct pipeline;

	video_pipeline_init(&pipeline);

	video_pipeline_palette16(&pipeline, dst_dx, dst_dy, src_dx, src_dy, src_dw, src_dp, palette8, palette16, palette32, combine);

	video_pipeline_blit(&pipeline, dst_x, dst_y, src);

	video_pipeline_done(&pipeline);
}

#ifdef __cplusplus
}
#endif

#endif

/*@}*/


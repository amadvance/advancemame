// license:BSD-3-Clause
// copyright-holders:David Haywood, Luca Elia, MetalliC, Andrea Mazzoleni, zanac
/***************************************************************************

	Cave CV1000 blitter (Altera Cyclone EP1C12 FPGA)

	Optimized with 16-bit VRAM, row-based uploads, exact fixed-point
	arithmetic and SSE2/NEON vector blitters.

***************************************************************************/

#include <string.h>
#include "driver.h"
#include "state.h"
#include "usrintrf.h"
#include "includes/cv1k.h"

#ifdef LSB_FIRST
#define NATIVE_ENDIAN_VALUE_LE_BE(le,be) le
#else
#define NATIVE_ENDIAN_VALUE_LE_BE(le,be) be
#endif

#define VRAM_W		0x2000
#define VRAM_H		0x1000

#define CV1K_VRAM_CLK_NANOSEC				13
#define CV1K_SRAM_CLK_NANOSEC				20
#define CV1K_UPLOAD_CHUNK_GAP_NANOSEC		1130
#define CV1K_VRAM_H_LINE_PERIOD_NANOSEC		63600
#define CV1K_VRAM_H_LINE_DURATION_NANOSEC	2160
#define CV1K_DRAW_OPERATION_SIZE_BYTES		20
#define CV1K_CLIP_OPERATION_SIZE_BYTES		2
#define CV1K_CLIP_MARGIN					32
#define OPERATION_CHUNK_SIZE_BYTES			64
#define OPERATION_READ_CHUNK_INTERVAL_NS	700

typedef struct
{
	int min_x, max_x, min_y, max_y;
} cv1k_rect;

static UINT16 *cv1k_vram;
static UINT16 *m_use_ram;
static UINT32 m_main_rammask;
static UINT32 m_gfx_addr, m_gfx_scroll_0_x, m_gfx_scroll_0_y, m_gfx_scroll_1_x, m_gfx_scroll_1_y;
static UINT32 m_gfx_addr_shadowcopy, m_gfx_clip_x_shadowcopy, m_gfx_clip_y_shadowcopy;
static cv1k_rect blit_clip;
static cv1k_rect draw_clip;
static UINT8 m_blitter_busy;
static UINT32 blit_delay_ns;
static UINT16 blit_idle_op_bytes;
static mame_timer *blit_timer;
static UINT32 palette16_to_32[0x8000];

/***************************************************************************
	SIMD: 8 pixels at a time with SSE2 (x86) or NEON (ARM), the same
	results as the tables: T(a, b) = (a * b) / 31 is mulhi(a * b, 2115)
	for every a * b up to 31 * 63, TR(a, b) = T(a ^ 31, b) and
	A(a, b) = min(a + b, 31).
***************************************************************************/

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define CV1K_SIMD			1
typedef __m128i v16;
#define V_LOAD(p)			_mm_loadu_si128((const __m128i *)(p))
#define V_STORE(p, v)		_mm_storeu_si128((__m128i *)(p), (v))
#define V_SET(x)			_mm_set1_epi16((short)(x))
#define V_AND(a, b)			_mm_and_si128((a), (b))
#define V_OR(a, b)			_mm_or_si128((a), (b))
#define V_XOR(a, b)			_mm_xor_si128((a), (b))
#define V_ANDNOT(m, b)		_mm_andnot_si128((m), (b))		/* b & ~m */
#define V_ADD(a, b)			_mm_add_epi16((a), (b))
#define V_MUL(a, b)			_mm_mullo_epi16((a), (b))
#define V_MULHI(a, b)		_mm_mulhi_epu16((a), (b))
#define V_MIN(a, b)			_mm_min_epi16((a), (b))			/* the values are below 0x8000 */
#define V_SRL(a, n)			_mm_srli_epi16((a), (n))
#define V_SLL(a, n)			_mm_slli_epi16((a), (n))
#define V_SRA(a, n)			_mm_srai_epi16((a), (n))
#elif (defined(__ARM_NEON) || defined(__ARM_NEON__)) && !defined(MSB_FIRST)
#include <arm_neon.h>
#define CV1K_SIMD			1
typedef uint16x8_t v16;
#define V_LOAD(p)			vld1q_u16((const uint16_t *)(p))
#define V_STORE(p, v)		vst1q_u16((uint16_t *)(p), (v))
#define V_SET(x)			vdupq_n_u16((uint16_t)(x))
#define V_AND(a, b)			vandq_u16((a), (b))
#define V_OR(a, b)			vorrq_u16((a), (b))
#define V_XOR(a, b)			veorq_u16((a), (b))
#define V_ANDNOT(m, b)		vbicq_u16((b), (m))				/* b & ~m */
#define V_ADD(a, b)			vaddq_u16((a), (b))
#define V_MUL(a, b)			vmulq_u16((a), (b))
#define V_MULHI(a, b)		vcombine_u16(vshrn_n_u32(vmull_u16(vget_low_u16(a), vget_low_u16(b)), 16), \
								vshrn_n_u32(vmull_u16(vget_high_u16(a), vget_high_u16(b)), 16))
#define V_MIN(a, b)			vminq_u16((a), (b))
#define V_SRL(a, n)			vshrq_n_u16((a), (n))
#define V_SLL(a, n)			vshlq_n_u16((a), (n))
#define V_SRA(a, n)			vreinterpretq_u16_s16(vshrq_n_s16(vreinterpretq_s16_u16(a), (n)))
#else
#define CV1K_SIMD			0
#endif

#if CV1K_SIMD
#define VT(a, b)			V_MIN(V_MULHI(V_MUL((a), (b)), v_div31), v_1f)
#define VTR(a, b)			VT(V_XOR((a), v_1f), (b))
#define VA(a, b)			V_MIN(V_ADD((a), (b)), v_1f)
#endif

/* Exact fixed-point arithmetic for fast multiplication architectures */
static inline int C_T(int a, int b)
{
	int v = (a * b * 2115) >> 16;
	return v > 0x1f ? 0x1f : v;
}

static inline int C_TR(int a, int b)
{
	return C_T(a ^ 0x1f, b);
}

static inline int C_A(int a, int b)
{
	int v = a + b;
	return v > 0x1f ? 0x1f : v;
}

#define T(a, b)		C_T(a, b)
#define TR(a, b)	C_TR(a, b)
#define A(a, b)		C_A(a, b)

#define BLIT_PARAMS		const cv1k_rect *clip, int src_x, int src_y, const int dst_x_start, const int dst_y_start, \
						int dimx, int dimy, const int flipy, const int s_alpha, const int d_alpha, \
						const int tint_r, const int tint_g, const int tint_b

typedef void (*blitfunction)(BLIT_PARAMS);

/* s = source, d = destination, c = clr0; the modes are constants,
   the compiler keeps only the code of one */
#define BLEND_G(SMODE, DMODE, T, TR, A, SA, DA)															\
	if (SMODE == 0)																						\
	{																									\
		if (DMODE == 0)																					\
		{																								\
			sr = A(T(SA, sr), T(DA, dr));																\
			sg = A(T(SA, sg), T(DA, dg));																\
			sb = A(T(SA, sb), T(DA, db));																\
		}																								\
		else if (DMODE == 1)																			\
		{																								\
			sr = A(T(SA, sr), T(sr, dr));																\
			sg = A(T(SA, sg), T(sg, dg));																\
			sb = A(T(SA, sb), T(sb, db));																\
		}																								\
		else if (DMODE == 5)																			\
		{																								\
			sr = A(T(SA, sr), TR(sr, dr));																\
			sg = A(T(SA, sg), TR(sg, dg));																\
			sb = A(T(SA, sb), TR(sb, db));																\
		}																								\
		else																							\
		{																								\
			cr = T(SA, sr); cg = T(SA, sg); cb = T(SA, sb);												\
			BLEND_DEST_G(DMODE, T, TR, A, DA)															\
		}																								\
	}																									\
	else if (SMODE == 2)																				\
	{																									\
		if (DMODE == 0)																					\
		{																								\
			sr = A(T(dr, sr), T(DA, dr));																\
			sg = A(T(dg, sg), T(DA, dg));																\
			sb = A(T(db, sb), T(DA, db));																\
		}																								\
		else																							\
		{																								\
			cr = T(dr, sr); cg = T(dg, sg); cb = T(db, sb);											\
			BLEND_DEST_G(DMODE, T, TR, A, DA)															\
		}																								\
	}																									\
	else																								\
	{																									\
		if (SMODE == 1)																					\
			{ cr = T(sr, sr); cg = T(sg, sg); cb = T(sb, sb); }											\
		else if (SMODE == 3 || SMODE == 7)																\
			{ cr = sr; cg = sg; cb = sb; }																\
		else if (SMODE == 4)																			\
			{ cr = TR(SA, sr); cg = TR(SA, sg); cb = TR(SA, sb); }										\
		else if (SMODE == 5)																			\
			{ cr = TR(sr, sr); cg = TR(sg, sg); cb = TR(sb, sb); }										\
		else																							\
			{ cr = TR(dr, sr); cg = TR(dg, sg); cb = TR(db, sb); }										\
		if (DMODE == 0)																					\
		{																								\
			sr = A(cr, T(dr, DA));																		\
			sg = A(cg, T(dg, DA));																		\
			sb = A(cb, T(db, DA));																		\
		}																								\
		else																							\
			BLEND_DEST_G(DMODE, T, TR, A, DA)															\
	}

/* the destination part shared by the modes with a clr0; mode 2 uses the
   red of clr0 for the three guns, as MAME does */
#define BLEND_DEST_G(DMODE, T, TR, A, DA)																\
	if (DMODE == 1)																						\
	{																									\
		sr = A(cr, T(sr, dr));																			\
		sg = A(cg, T(sg, dg));																			\
		sb = A(cb, T(sb, db));																			\
	}																									\
	else if (DMODE == 2)																				\
	{																									\
		sr = A(cr, T(dr, dr));																			\
		sg = A(cr, T(dg, dg));																			\
		sb = A(cr, T(db, db));																			\
	}																									\
	else if (DMODE == 3 || DMODE == 7)																	\
	{																									\
		sr = A(cr, dr);																					\
		sg = A(cg, dg);																					\
		sb = A(cb, db);																					\
	}																									\
	else if (DMODE == 4)																				\
	{																									\
		sr = A(cr, TR(DA, dr));																			\
		sg = A(cg, TR(DA, dg));																			\
		sb = A(cb, TR(DA, db));																			\
	}																									\
	else if (DMODE == 5)																				\
	{																									\
		sr = A(cr, TR(sr, dr));																			\
		sg = A(cg, TR(sg, dg));																			\
		sb = A(cb, TR(sb, db));																			\
	}																									\
	else if (DMODE == 6)																				\
	{																									\
		sr = A(cr, TR(dr, dr));																			\
		sg = A(cg, TR(dg, dg));																			\
		sb = A(cb, TR(db, db));																			\
	}

#define BLEND(SMODE, DMODE)		BLEND_G(SMODE, DMODE, T, TR, A, s_alpha, d_alpha)
#if CV1K_SIMD
#define VBLEND(SMODE, DMODE)	BLEND_G(SMODE, DMODE, VT, VTR, VA, v_s_alpha, v_d_alpha)
#endif

#if CV1K_SIMD
/* 8 pixels of a row, as the pixel loop of DEFINE_BLIT does them one by one;
   s_alpha, d_alpha and the tints are vectors here */
#define VPIXELS(TINT, TRANSPARENT, BLENDED, SMODE, DMODE, SIMPLE)										\
	{																									\
		const v16 pen = V_LOAD(gfx2);																	\
		v16 out;																						\
		if (SIMPLE)																						\
			out = pen;																					\
		else																							\
		{																								\
			v16 sr = V_AND(V_SRL(pen, 10), v_1f), sg = V_AND(V_SRL(pen, 5), v_1f), sb = V_AND(pen, v_1f);	\
			if (TINT)																					\
			{																							\
				sr = VT(sr, v_tint_r);																	\
				sg = VT(sg, v_tint_g);																	\
				sb = VT(sb, v_tint_b);																	\
			}																							\
			if (BLENDED)																				\
			{																							\
				const v16 dst = V_LOAD(bmp);															\
				const v16 dr = V_AND(V_SRL(dst, 10), v_1f), dg = V_AND(V_SRL(dst, 5), v_1f), db = V_AND(dst, v_1f);	\
				v16 cr, cg, cb;																			\
				VBLEND(SMODE, DMODE)																	\
				(void)cr; (void)cg; (void)cb; (void)dr; (void)dg; (void)db;								\
			}																							\
			out = V_OR(V_OR(V_SLL(sr, 10), V_SLL(sg, 5)), V_OR(sb, V_AND(pen, v_8000)));			\
		}																								\
		if (TRANSPARENT)																				\
		{																								\
			const v16 m = V_SRA(pen, 15);																\
			out = V_OR(V_AND(m, out), V_ANDNOT(m, V_LOAD(bmp)));										\
		}																								\
		V_STORE(bmp, out);																				\
	}

/* the vector loop of a row: not with flip x, nor when the destination is
   1 to 7 pixels after the source on the same row (a pixel written would be
   read again as source in the same 8) */
#define VROW(FLIPX, TINT, TRANSPARENT, BLENDED, SMODE, DMODE, SIMPLE)									\
	if (!FLIPX && use_simd && count >= 8)																\
	{																									\
		const v16 v_1f = V_SET(0x1f), v_div31 = V_SET(2115), v_8000 = V_SET(0x8000);					\
		const v16 v_s_alpha = V_SET(s_alpha), v_d_alpha = V_SET(d_alpha);								\
		const v16 v_tint_r = V_SET(tint_r), v_tint_g = V_SET(tint_g), v_tint_b = V_SET(tint_b);		\
		(void)v_s_alpha; (void)v_d_alpha; (void)v_tint_r; (void)v_tint_g; (void)v_tint_b;				\
		(void)v_div31; (void)v_8000;																	\
		do																								\
		{																								\
			VPIXELS(TINT, TRANSPARENT, BLENDED, SMODE, DMODE, SIMPLE)									\
			bmp += 8;																					\
			gfx2 += 8;																					\
			count -= 8;																					\
		} while (count >= 8);																			\
	}
#else
#define VROW(FLIPX, TINT, TRANSPARENT, BLENDED, SMODE, DMODE, SIMPLE)
#endif

/* SIMPLE: copy the pixels */
#define DEFINE_BLIT(NAME, FLIPX, TINT, TRANSPARENT, BLENDED, SMODE, DMODE, SIMPLE)						\
static void NAME(BLIT_PARAMS)																			\
{																										\
	int yf, y, starty = 0, startx = 0;																	\
	const int dst_y_end = dst_y_start + dimy;															\
	int dst_x_end;																						\
	const int use_simd = (dst_x_start - src_x < 1 || dst_x_start - src_x > 7);							\
																										\
	(void)use_simd;																						\
	if (FLIPX)																							\
		src_x += (dimx - 1);																			\
	if (flipy)	{ yf = -1; src_y += (dimy - 1); }														\
	else		{ yf = +1; }																			\
																										\
	if (dst_y_start < clip->min_y)																		\
		starty = clip->min_y - dst_y_start;																\
	if (dst_y_end > clip->max_y)																		\
		dimy -= (dst_y_end - 1) - clip->max_y;															\
																										\
	/* a source that wraps around the VRAM edge is not drawn */											\
	if (FLIPX)																							\
	{																									\
		if ((src_x & 0x1fff) < ((src_x - (dimx - 1)) & 0x1fff))											\
			return;																						\
	}																									\
	else																								\
	{																									\
		if ((src_x & 0x1fff) > ((src_x + (dimx - 1)) & 0x1fff))											\
			return;																						\
	}																									\
																										\
	dst_x_end = dst_x_start + dimx;																		\
	if (dst_x_start < clip->min_x)																		\
		startx = clip->min_x - dst_x_start;																\
	if (dst_x_end > clip->max_x)																		\
		dimx -= (dst_x_end - 1) - clip->max_x;															\
																										\
	for (y = starty; y < dimy; y++)																		\
	{																									\
		UINT16 *bmp = &cv1k_vram[(dst_y_start + y) * VRAM_W + dst_x_start + startx];					\
		const UINT16 *gfx2 = &cv1k_vram[((src_y + yf * y) & 0x0fff) * VRAM_W							\
										+ (FLIPX ? (src_x - startx) : (src_x + startx))];				\
		int count = dimx - startx;																		\
																										\
		VROW(FLIPX, TINT, TRANSPARENT, BLENDED, SMODE, DMODE, SIMPLE)									\
		while (count-- > 0)																				\
		{																								\
			const UINT16 pen = *gfx2;																	\
			if (!TRANSPARENT || (pen & 0x8000))															\
			{																							\
				if (SIMPLE)																				\
					*bmp = pen;																			\
				else																					\
				{																						\
					int sr = (pen >> 10) & 0x1f, sg = (pen >> 5) & 0x1f, sb = pen & 0x1f;				\
					if (TINT)																			\
					{																					\
						sr = T(sr, tint_r);																\
						sg = T(sg, tint_g);																\
						sb = T(sb, tint_b);																\
					}																					\
					if (BLENDED)																		\
					{																					\
						const UINT16 dst = *bmp;														\
						const int dr = (dst >> 10) & 0x1f, dg = (dst >> 5) & 0x1f, db = dst & 0x1f;	\
						int cr, cg, cb;																	\
						BLEND(SMODE, DMODE)																\
						(void)cr; (void)cg; (void)cb; (void)dr; (void)dg; (void)db;					\
					}																					\
					*bmp = (sr << 10) | (sg << 5) | sb | (pen & 0x8000);								\
				}																						\
			}																							\
			bmp++;																						\
			if (FLIPX) gfx2--; else gfx2++;																\
		}																								\
	}																									\
}

#define BLIT_NAME(f, ti, tr, s, d)		draw_sprite_f##f##_ti##ti##_tr##tr##_s##s##_d##d

#define DEFINE_BLIT_D(f, ti, tr, d)																		\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 0, d), f, ti, tr, 1, 0, d, 0)										\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 1, d), f, ti, tr, 1, 1, d, 0)										\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 2, d), f, ti, tr, 1, 2, d, 0)										\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 3, d), f, ti, tr, 1, 3, d, 0)										\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 4, d), f, ti, tr, 1, 4, d, 0)										\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 5, d), f, ti, tr, 1, 5, d, 0)										\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 6, d), f, ti, tr, 1, 6, d, 0)										\
	DEFINE_BLIT(BLIT_NAME(f, ti, tr, 7, d), f, ti, tr, 1, 7, d, 0)

#define DEFINE_BLIT_SET(f, ti, tr)																		\
	DEFINE_BLIT_D(f, ti, tr, 0)																			\
	DEFINE_BLIT_D(f, ti, tr, 1)																			\
	DEFINE_BLIT_D(f, ti, tr, 2)																			\
	DEFINE_BLIT_D(f, ti, tr, 3)																			\
	DEFINE_BLIT_D(f, ti, tr, 4)																			\
	DEFINE_BLIT_D(f, ti, tr, 5)																			\
	DEFINE_BLIT_D(f, ti, tr, 6)																			\
	DEFINE_BLIT_D(f, ti, tr, 7)

#define BLIT_TABLE_D(f, ti, tr, d)																		\
	BLIT_NAME(f, ti, tr, 0, d), BLIT_NAME(f, ti, tr, 1, d), BLIT_NAME(f, ti, tr, 2, d), BLIT_NAME(f, ti, tr, 3, d), \
	BLIT_NAME(f, ti, tr, 4, d), BLIT_NAME(f, ti, tr, 5, d), BLIT_NAME(f, ti, tr, 6, d), BLIT_NAME(f, ti, tr, 7, d)

#define BLIT_TABLE(fl, ti, tr)																			\
	static const blitfunction blit_funcs_f##fl##_ti##ti##_tr##tr[64] =									\
	{																									\
		BLIT_TABLE_D(fl, ti, tr, 0), BLIT_TABLE_D(fl, ti, tr, 1), BLIT_TABLE_D(fl, ti, tr, 2), BLIT_TABLE_D(fl, ti, tr, 3), \
		BLIT_TABLE_D(fl, ti, tr, 4), BLIT_TABLE_D(fl, ti, tr, 5), BLIT_TABLE_D(fl, ti, tr, 6), BLIT_TABLE_D(fl, ti, tr, 7) \
	};

/* blended: flip x, tint, transparency, source and destination modes */
DEFINE_BLIT_SET(0, 0, 0)
DEFINE_BLIT_SET(0, 0, 1)
DEFINE_BLIT_SET(0, 1, 0)
DEFINE_BLIT_SET(0, 1, 1)
DEFINE_BLIT_SET(1, 0, 0)
DEFINE_BLIT_SET(1, 0, 1)
DEFINE_BLIT_SET(1, 1, 0)
DEFINE_BLIT_SET(1, 1, 1)

BLIT_TABLE(0, 0, 0)
BLIT_TABLE(0, 0, 1)
BLIT_TABLE(0, 1, 0)
BLIT_TABLE(0, 1, 1)
BLIT_TABLE(1, 0, 0)
BLIT_TABLE(1, 0, 1)
BLIT_TABLE(1, 1, 0)
BLIT_TABLE(1, 1, 1)

/* tinted, not blended */
DEFINE_BLIT(draw_sprite_f0_ti1_tr1_plain, 0, 1, 1, 0, 0, 0, 0)
DEFINE_BLIT(draw_sprite_f0_ti1_tr0_plain, 0, 1, 0, 0, 0, 0, 0)
DEFINE_BLIT(draw_sprite_f1_ti1_tr1_plain, 1, 1, 1, 0, 0, 0, 0)
DEFINE_BLIT(draw_sprite_f1_ti1_tr0_plain, 1, 1, 0, 0, 0, 0, 0)

/* neither tinted nor blended: copies */
DEFINE_BLIT(draw_sprite_f0_ti0_tr1_simple, 0, 0, 1, 0, 0, 0, 1)
DEFINE_BLIT(draw_sprite_f0_ti0_tr0_simple, 0, 0, 0, 0, 0, 0, 1)
DEFINE_BLIT(draw_sprite_f1_ti0_tr1_simple, 1, 0, 1, 0, 0, 0, 1)
DEFINE_BLIT(draw_sprite_f1_ti0_tr0_simple, 1, 0, 0, 0, 0, 0, 1)

/***************************************************************************
	blit list
***************************************************************************/

INLINE UINT16 read_next_word(offs_t *addr)
{
	UINT16 data = m_use_ram[((*addr & m_main_rammask) >> 1) ^ NATIVE_ENDIAN_VALUE_LE_BE(3, 0)];
	*addr += 2;
	return data;
}

static void set_clip(int cliptype)
{
	if (cliptype)
	{
		blit_clip.min_x = (INT32)(m_gfx_clip_x_shadowcopy - CV1K_CLIP_MARGIN);
		blit_clip.max_x = (INT32)(m_gfx_clip_x_shadowcopy + 320 - 1 + CV1K_CLIP_MARGIN);
		blit_clip.min_y = (INT32)(m_gfx_clip_y_shadowcopy - CV1K_CLIP_MARGIN);
		blit_clip.max_y = (INT32)(m_gfx_clip_y_shadowcopy + 240 - 1 + CV1K_CLIP_MARGIN);
	}
	else
	{
		blit_clip.min_x = 0;
		blit_clip.max_x = VRAM_W - 1;
		blit_clip.min_y = 0;
		blit_clip.max_y = VRAM_H - 1;
	}
	draw_clip = blit_clip;
	if (draw_clip.min_x < 0) draw_clip.min_x = 0;
	if (draw_clip.min_y < 0) draw_clip.min_y = 0;
	if (draw_clip.max_x > VRAM_W - 1) draw_clip.max_x = VRAM_W - 1;
	if (draw_clip.max_y > VRAM_H - 1) draw_clip.max_y = VRAM_H - 1;
}

static void idle_blitter(UINT8 operation_size_bytes)
{
	blit_idle_op_bytes += operation_size_bytes;
	if (blit_idle_op_bytes >= OPERATION_CHUNK_SIZE_BYTES)
	{
		blit_idle_op_bytes -= OPERATION_CHUNK_SIZE_BYTES;
		blit_delay_ns += OPERATION_READ_CHUNK_INTERVAL_NS;
	}
}

/* the number of VRAM rows (32 x 32 pixels) a draw touches */
static UINT16 calculate_vram_accesses(UINT16 start_x, UINT16 start_y, UINT16 dimx, UINT16 dimy)
{
	int x_rows = 0;
	int num_vram_rows = 0;
	int x_pixels, y_pixels;

	for (x_pixels = dimx; x_pixels > 0; x_pixels -= 32)
	{
		x_rows++;
		if (((start_x & 31) + (x_pixels < 32 ? x_pixels : 32)) > 32)
			x_rows++;
	}
	for (y_pixels = dimy; y_pixels > 0; y_pixels -= 32)
	{
		num_vram_rows += x_rows;
		if (((start_y & 31) + (y_pixels < 32 ? y_pixels : 32)) > 32)
			num_vram_rows += x_rows;
	}
	return num_vram_rows;
}

/* upload: the timing (gfx_upload_shadow_copy of MAME) */
static void gfx_upload_timing(UINT32 *addr)
{
	const UINT32 chunk_offset = (*addr - (m_gfx_addr & 0x1fffffff)) & (OPERATION_CHUNK_SIZE_BYTES - 1);
	UINT32 dimx, dimy;
	int num_bytes, num_sram_clk, num_chunk_gaps;

	*addr += 12;
	dimx = (read_next_word(addr) & 0x1fff) + 1;
	dimy = (read_next_word(addr) & 0x0fff) + 1;
	*addr += dimx * dimy * 2;

	num_bytes = 16 + dimx * dimy * 2;
	num_sram_clk = num_bytes / 4;
	blit_delay_ns += num_sram_clk * CV1K_SRAM_CLK_NANOSEC;
	num_chunk_gaps = (chunk_offset + num_bytes) / OPERATION_CHUNK_SIZE_BYTES;
	blit_delay_ns += num_chunk_gaps * CV1K_UPLOAD_CHUNK_GAP_NANOSEC;
	blit_idle_op_bytes = 0;
}

static void gfx_upload(UINT32 *addr)
{
	UINT32 dst_x_start, dst_y_start, dimx, dimy, x, y, cols;

	*addr += 8;
	dst_x_start = read_next_word(addr) & 0x1fff;
	dst_y_start = read_next_word(addr) & 0x0fff;
	dimx = (read_next_word(addr) & 0x1fff) + 1;
	dimy = (read_next_word(addr) & 0x0fff) + 1;

	/* the columns inside the VRAM; the others are read and dropped */
	cols = (dst_x_start < VRAM_W) ? VRAM_W - dst_x_start : 0;
	if (cols > dimx)
		cols = dimx;

	for (y = 0; y < dimy; y++)
	{
		UINT16 *dst = &cv1k_vram[((dst_y_start + y) & (VRAM_H - 1)) * VRAM_W + dst_x_start];
		UINT32 a = *addr & m_main_rammask;

		/* VRAM is 1555 like the uploaded data */
		if (a + dimx * 2 <= m_main_rammask + 1)
		{
			/* the row doesn't wrap around the end of the RAM */
			for (x = 0; x < cols; x++, a += 2)
				dst[x] = m_use_ram[(a >> 1) ^ NATIVE_ENDIAN_VALUE_LE_BE(3, 0)];
			*addr += dimx * 2;
		}
		else
		{
			for (x = 0; x < dimx; x++)
			{
				const UINT16 pendat = read_next_word(addr);
				if (x < cols)
					dst[x] = pendat;
			}
		}
	}
}

/* draw: the timing (gfx_draw_shadow_copy of MAME) */
static void gfx_draw_timing(UINT32 *addr)
{
	UINT16 src_x_start, src_y_start, dst_x_start, dst_y_start, src_dimx, src_dimy;
	UINT16 dst_x_end, dst_y_end, dst_dimx, dst_dimy;
	int src_num_vram_rows, dst_num_vram_rows;
	UINT32 num_vram_clk;

	*addr += 4;
	src_x_start = read_next_word(addr);
	src_y_start = read_next_word(addr);
	dst_x_start = read_next_word(addr);
	dst_y_start = read_next_word(addr);
	src_dimx = (read_next_word(addr) & 0x1fff) + 1;
	src_dimy = (read_next_word(addr) & 0x0fff) + 1;
	*addr += 4;

	dst_x_end = dst_x_start + src_dimx - 1;
	dst_y_end = dst_y_start + src_dimy - 1;

	/* sprites fully outside of the clip area are not drawn */
	if (!(blit_clip.min_x <= (int)dst_x_end && blit_clip.max_x >= (int)dst_x_start &&
		  blit_clip.min_y <= (int)dst_y_end && blit_clip.max_y >= (int)dst_y_start))
	{
		idle_blitter(CV1K_DRAW_OPERATION_SIZE_BYTES);
		return;
	}

	/* the timing only counts the area written */
	if (dst_x_start < (UINT16)blit_clip.min_x) dst_x_start = (UINT16)blit_clip.min_x;
	if (dst_y_start < (UINT16)blit_clip.min_y) dst_y_start = (UINT16)blit_clip.min_y;
	if (dst_x_end > (UINT16)blit_clip.max_x) dst_x_end = (UINT16)blit_clip.max_x;
	if (dst_y_end > (UINT16)blit_clip.max_y) dst_y_end = (UINT16)blit_clip.max_y;
	src_dimx = dst_x_end - dst_x_start + 1;
	src_dimy = dst_y_end - dst_y_start + 1;

	blit_idle_op_bytes = 0;

	src_num_vram_rows = calculate_vram_accesses(src_x_start, src_y_start, src_dimx, src_dimy);
	dst_num_vram_rows = calculate_vram_accesses(dst_x_start, dst_y_start, src_dimx, src_dimy);

	/* draws are done 4 pixels at a time */
	dst_x_start -= dst_x_start & 3;
	dst_x_end += (4 - ((dst_x_end + 1) & 3)) & 3;
	dst_dimx = dst_x_end - dst_x_start + 1;
	dst_dimy = dst_y_end - dst_y_start + 1;

	num_vram_clk = src_dimx * src_dimy / 4 + dst_dimx * dst_dimy / 2 + src_num_vram_rows * 6 + dst_num_vram_rows * (20 + 11) + 12;
	blit_delay_ns += num_vram_clk * CV1K_VRAM_CLK_NANOSEC;
}

static void gfx_draw(UINT32 *addr)
{
	int tint_r, tint_g, tint_b, tinted = 0;
	const UINT16 attr = read_next_word(addr);
	const UINT16 alpha = read_next_word(addr);
	UINT16 src_x = read_next_word(addr);
	UINT16 src_y = read_next_word(addr);
	const UINT16 dst_x_start = read_next_word(addr);
	const UINT16 dst_y_start = read_next_word(addr);
	const UINT16 w = read_next_word(addr);
	const UINT16 h = read_next_word(addr);
	const UINT16 tint_rw = read_next_word(addr);
	const UINT16 tint_gb = read_next_word(addr);

	const int d_mode = attr & 0x0007;
	const int s_mode = (attr & 0x0070) >> 4;
	const int trans = (attr >> 8) & 1;
	int blend = (attr >> 9) & 1;
	const int flipy = (attr >> 10) & 1;
	const int flipx = (attr >> 11) & 1;
	const int d_alpha = (alpha & 0x00ff) >> 3;
	const int s_alpha = ((alpha & 0xff00) >> 8) >> 3;
	const int x = (INT16)dst_x_start;
	const int y = (INT16)dst_y_start;
	const int dimx = (w & 0x1fff) + 1;
	const int dimy = (h & 0x0fff) + 1;

	src_x &= 0x1fff;
	src_y &= 0x0fff;

	/* 0x20 is the 'normal' tint, not 0x1f */
	tint_r = (tint_rw & 0xff) >> 2;
	tint_g = ((tint_gb >> 8) & 0xff) >> 2;
	tint_b = (tint_gb & 0xff) >> 2;
	if (tint_r != 0x20 || tint_g != 0x20 || tint_b != 0x20)
		tinted = 1;

	if ((s_mode == 0 && s_alpha == 0x1f) && (d_mode == 4 && d_alpha == 0x1f))
		blend = 0;

#define DRAW_PARAMS		&draw_clip, src_x, src_y, x, y, dimx, dimy, flipy, s_alpha, d_alpha, tint_r, tint_g, tint_b

	if (tinted)
	{
		if (!flipx)
		{
			if (trans)
			{
				if (!blend) draw_sprite_f0_ti1_tr1_plain(DRAW_PARAMS);
				else blit_funcs_f0_ti1_tr1[s_mode | (d_mode << 3)](DRAW_PARAMS);
			}
			else
			{
				if (!blend) draw_sprite_f0_ti1_tr0_plain(DRAW_PARAMS);
				else blit_funcs_f0_ti1_tr0[s_mode | (d_mode << 3)](DRAW_PARAMS);
			}
		}
		else
		{
			if (trans)
			{
				if (!blend) draw_sprite_f1_ti1_tr1_plain(DRAW_PARAMS);
				else blit_funcs_f1_ti1_tr1[s_mode | (d_mode << 3)](DRAW_PARAMS);
			}
			else
			{
				if (!blend) draw_sprite_f1_ti1_tr0_plain(DRAW_PARAMS);
				else blit_funcs_f1_ti1_tr0[s_mode | (d_mode << 3)](DRAW_PARAMS);
			}
		}
	}
	else if (!blend)
	{
		if (!flipx)
		{
			if (trans) draw_sprite_f0_ti0_tr1_simple(DRAW_PARAMS);
			else draw_sprite_f0_ti0_tr0_simple(DRAW_PARAMS);
		}
		else
		{
			if (trans) draw_sprite_f1_ti0_tr1_simple(DRAW_PARAMS);
			else draw_sprite_f1_ti0_tr0_simple(DRAW_PARAMS);
		}
	}
	else
	{
		if (!flipx)
		{
			if (trans) blit_funcs_f0_ti0_tr1[s_mode | (d_mode << 3)](DRAW_PARAMS);
			else blit_funcs_f0_ti0_tr0[s_mode | (d_mode << 3)](DRAW_PARAMS);
		}
		else
		{
			if (trans) blit_funcs_f1_ti0_tr1[s_mode | (d_mode << 3)](DRAW_PARAMS);
			else blit_funcs_f1_ti0_tr0[s_mode | (d_mode << 3)](DRAW_PARAMS);
		}
	}
#undef DRAW_PARAMS
}

/* the timing pass (gfx_create_shadow_copy of MAME) */
static void gfx_create_timing(void)
{
	UINT32 addr = m_gfx_addr & 0x1fffffff;

	set_clip(1);
	for (;;)
	{
		const UINT16 data = read_next_word(&addr);

		switch (data & 0xf000)
		{
			case 0x0000:
			case 0xf000:
				return;

			case 0xc000:
				set_clip(read_next_word(&addr));
				idle_blitter(CV1K_CLIP_OPERATION_SIZE_BYTES);
				break;

			case 0x2000:
				addr -= 2;
				gfx_upload_timing(&addr);
				break;

			case 0x1000:
				addr -= 2;
				gfx_draw_timing(&addr);
				break;

			default:
				return;
		}
	}
}

static void gfx_exec(void)
{
	UINT32 addr = m_gfx_addr_shadowcopy & 0x1fffffff;

	set_clip(1);
	for (;;)
	{
		const UINT16 data = read_next_word(&addr);

		switch (data & 0xf000)
		{
			case 0x0000:
			case 0xf000:
				return;

			case 0xc000:
				set_clip(read_next_word(&addr));
				break;

			case 0x2000:
				addr -= 2;
				gfx_upload(&addr);
				break;

			case 0x1000:
				addr -= 2;
				gfx_draw(&addr);
				break;

			default:
				return;
		}
	}
}

static void blit_done(int param)
{
	m_blitter_busy = 0;
}

/***************************************************************************
	registers
***************************************************************************/

static READ32_HANDLER(blitter_r)
{
	switch (offset * 4)
	{
		case 0x10:
			if (m_blitter_busy) cpu_spinuntil_time(TIME_IN_USEC(10));
			return m_blitter_busy ? 0 : 0x10;
		case 0x24:
		case 0x28:
			return 0xffffffff;
		case 0x50:
			return readinputportbytag("DSW");
	}
	return 0;
}

static WRITE32_HANDLER(blitter_w)
{
	switch (offset * 4)
	{
		case 0x04:
			if (data & 1)
			{
				m_gfx_clip_x_shadowcopy = m_gfx_scroll_1_x;
				m_gfx_clip_y_shadowcopy = m_gfx_scroll_1_y;

				/* the time the blit list takes */
				blit_delay_ns = 0;
				gfx_create_timing();

				/* every 63.6 us the blitter fetches a line of VRAM for the screen */
				blit_delay_ns += (blit_delay_ns / CV1K_VRAM_H_LINE_PERIOD_NANOSEC) * CV1K_VRAM_H_LINE_DURATION_NANOSEC;

				m_blitter_busy = 1;
				timer_adjust(blit_timer, TIME_IN_NSEC(blit_delay_ns), 0, 0);

				m_gfx_addr_shadowcopy = m_gfx_addr;
				gfx_exec();
			}
			break;
		case 0x08: COMBINE_DATA(&m_gfx_addr); break;
		case 0x14: COMBINE_DATA(&m_gfx_scroll_0_x); break;
		case 0x18: COMBINE_DATA(&m_gfx_scroll_0_y); break;
		case 0x40: COMBINE_DATA(&m_gfx_scroll_1_x); break;
		case 0x44: COMBINE_DATA(&m_gfx_scroll_1_y); break;
		default: break;
	}
}

READ64_HANDLER(cv1k_blitter_r)
{
	UINT64 result = 0;
	if ((UINT32)(mem_mask >> 32) != 0xffffffff) result |= (UINT64)blitter_r(offset * 2, mem_mask >> 32) << 32;
	if ((UINT32)mem_mask != 0xffffffff) result |= blitter_r(offset * 2 + 1, mem_mask);
	return result;
}

WRITE64_HANDLER(cv1k_blitter_w)
{
	if ((UINT32)(mem_mask >> 32) != 0xffffffff) blitter_w(offset * 2, data >> 32, mem_mask >> 32);
	if ((UINT32)mem_mask != 0xffffffff) blitter_w(offset * 2 + 1, data, mem_mask);
}

/***************************************************************************
	video initialization & update
***************************************************************************/

void cv1k_video_init(UINT64 *ram, int size)
{
	int p;

	m_use_ram = (UINT16 *)ram;
	m_main_rammask = size - 1;

	/* padding after the VRAM: a source can run past the end of its row */
	cv1k_vram = auto_malloc((VRAM_W * VRAM_H + 2 * VRAM_W) * sizeof(UINT16));
	memset(cv1k_vram, 0, (VRAM_W * VRAM_H + 2 * VRAM_W) * sizeof(UINT16));

	for (p = 0; p < 0x8000; p++)
		palette16_to_32[p] = ((p & 0x7c00) << 9) | ((p & 0x03e0) << 6) | ((p & 0x001f) << 3);

	blit_timer = timer_alloc(blit_done);
	timer_adjust(blit_timer, TIME_NEVER, 0, 0);

	m_gfx_addr = m_gfx_scroll_0_x = m_gfx_scroll_0_y = m_gfx_scroll_1_x = m_gfx_scroll_1_y = 0;
	m_gfx_addr_shadowcopy = m_gfx_clip_x_shadowcopy = m_gfx_clip_y_shadowcopy = 0;
	m_blitter_busy = 0;
	blit_delay_ns = 0;
	blit_idle_op_bytes = 0;
	set_clip(0);

	state_save_register_global_pointer(cv1k_vram, VRAM_W * VRAM_H);
	state_save_register_global(m_gfx_addr);
	state_save_register_global(m_gfx_scroll_0_x);
	state_save_register_global(m_gfx_scroll_0_y);
	state_save_register_global(m_gfx_scroll_1_x);
	state_save_register_global(m_gfx_scroll_1_y);
	state_save_register_global(m_gfx_addr_shadowcopy);
	state_save_register_global(m_gfx_clip_x_shadowcopy);
	state_save_register_global(m_gfx_clip_y_shadowcopy);
	state_save_register_global(m_blitter_busy);
	state_save_register_global(blit_delay_ns);
	state_save_register_global(blit_idle_op_bytes);
}

void cv1k_video_reset(void)
{
	m_blitter_busy = 0;
	timer_adjust(blit_timer, TIME_NEVER, 0, 0);
}

VIDEO_UPDATE(cv1k)
{
	int sx = m_gfx_scroll_0_x & (VRAM_W - 1);
	int sy = m_gfx_scroll_0_y & (VRAM_H - 1);
	int x, y;

	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		const UINT16 *src = &cv1k_vram[((y + sy) & (VRAM_H - 1)) * VRAM_W];
		if (bitmap->depth == 15 || bitmap->depth == 16)
		{
			UINT16 *dst = (UINT16 *)bitmap->line[y];
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
				dst[x] = src[(x + sx) & (VRAM_W - 1)] & 0x7fff;
		}
		else
		{
			UINT32 *dst = (UINT32 *)bitmap->line[y];
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
				dst[x] = palette16_to_32[src[(x + sx) & (VRAM_W - 1)] & 0x7fff];
		}
	}
}

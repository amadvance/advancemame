/*
license:BSD-3-Clause
copyright-holders:David Haywood, Luca Elia, MetalliC

emulation of Altera Cyclone EPIC12 FPGA programmed as a blitter
*/

#ifndef CV1K_BLIT_H
#define CV1K_BLIT_H
#include "driver.h"
extern UINT8 epic12_device_colrtable[32][64], epic12_device_colrtable_rev[32][64], epic12_device_colrtable_add[32][32];
extern UINT64 epic12_device_blit_delay;
struct _clr_t
{
	UINT8 b,g,r,t;
};

typedef struct _clr_t clr_t;

typedef union colour_t
{
	clr_t trgb;
	UINT32 u32;
} colour_t;

typedef void (*epic12_device_blitfunction)(mame_bitmap *,
						const rectangle *,
						UINT32 *,
						int ,
						int ,
						const int ,
						const int ,
						int ,
						int ,
						const int ,
						const UINT8 ,
						const UINT8 ,

						const clr_t * );

#define BLIT_FUNCTION void
#define BLIT_PARAMS mame_bitmap *bitmap, const rectangle *clip, UINT32 *gfx, int src_x, int src_y, const int dst_x_start, const int dst_y_start, int dimx, int dimy, const int flipy, const UINT8 s_alpha, const UINT8 d_alpha, const clr_t *tint_clr

	BLIT_FUNCTION draw_sprite_f0_ti0_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f1_ti0_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr1_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti1_tr0_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr1_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_plain(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d0(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d1(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d2(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d3(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d4(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d5(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d6(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s0_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s1_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s2_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s3_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s4_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s5_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s6_d7(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti1_tr0_s7_d7(BLIT_PARAMS);

	BLIT_FUNCTION draw_sprite_f0_ti0_tr1_simple(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f0_ti0_tr0_simple(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr1_simple(BLIT_PARAMS);
	BLIT_FUNCTION draw_sprite_f1_ti0_tr0_simple(BLIT_PARAMS);

	static inline void pen_to_clr(UINT32 pen, clr_t *clr)
	{

		clr->r = (pen >> (16+3));
		clr->g = (pen >>  (8+3));
		clr->b = (pen >>   3);

	};

	static inline void tint_to_clr(UINT8 r, UINT8 g, UINT8 b, clr_t *clr)
	{
		clr->r  =   r>>2;
		clr->g  =   g>>2;
		clr->b  =   b>>2;
	};

	static inline UINT32 clr_to_pen(const clr_t *clr)
	{

		return (clr->r << (16+3)) | (clr->g << (8+3)) | (clr->b << 3);

	};

	static inline void clr_add_with_clr_mul_fixed(clr_t *clr, const clr_t *clr0, const UINT8 mulfixed_val, const clr_t *mulfixed_clr0)
	{
		clr->r = epic12_device_colrtable_add[clr0->r][epic12_device_colrtable[(mulfixed_clr0->r)][mulfixed_val]];
		clr->g = epic12_device_colrtable_add[clr0->g][epic12_device_colrtable[(mulfixed_clr0->g)][mulfixed_val]];
		clr->b = epic12_device_colrtable_add[clr0->b][epic12_device_colrtable[(mulfixed_clr0->b)][mulfixed_val]];
	}

	static inline  void clr_add_with_clr_mul_3param(clr_t *clr, const clr_t *clr0, const clr_t *clr1, const clr_t *clr2)
	{
		clr->r = epic12_device_colrtable_add[clr0->r][epic12_device_colrtable[(clr2->r)][(clr1->r)]];
		clr->g = epic12_device_colrtable_add[clr0->g][epic12_device_colrtable[(clr2->g)][(clr1->g)]];
		clr->b = epic12_device_colrtable_add[clr0->b][epic12_device_colrtable[(clr2->b)][(clr1->b)]];
	}

	static inline  void clr_add_with_clr_square(clr_t *clr, const clr_t *clr0, const clr_t *clr1)
	{
		clr->r = epic12_device_colrtable_add[clr0->r][epic12_device_colrtable[(clr1->r)][(clr1->r)]];
		clr->g = epic12_device_colrtable_add[clr0->r][epic12_device_colrtable[(clr1->g)][(clr1->g)]];
		clr->b = epic12_device_colrtable_add[clr0->r][epic12_device_colrtable[(clr1->b)][(clr1->b)]];
	}

	static inline  void clr_add_with_clr_mul_fixed_rev(clr_t *clr, const clr_t *clr0, const UINT8 val, const clr_t *clr1)
	{
		clr->r =  epic12_device_colrtable_add[clr0->r][epic12_device_colrtable_rev[val][(clr1->r)]];
		clr->g =  epic12_device_colrtable_add[clr0->g][epic12_device_colrtable_rev[val][(clr1->g)]];
		clr->b =  epic12_device_colrtable_add[clr0->b][epic12_device_colrtable_rev[val][(clr1->b)]];
	}

	static inline  void clr_add_with_clr_mul_rev_3param(clr_t *clr, const clr_t *clr0, const clr_t *clr1, const clr_t *clr2)
	{
		clr->r =  epic12_device_colrtable_add[clr0->r][epic12_device_colrtable_rev[(clr2->r)][(clr1->r)]];
		clr->g =  epic12_device_colrtable_add[clr0->g][epic12_device_colrtable_rev[(clr2->g)][(clr1->g)]];
		clr->b =  epic12_device_colrtable_add[clr0->b][epic12_device_colrtable_rev[(clr2->b)][(clr1->b)]];
	}

	static inline  void clr_add_with_clr_mul_rev_square(clr_t *clr, const clr_t *clr0, const clr_t *clr1)
	{
		clr->r =  epic12_device_colrtable_add[clr0->r][epic12_device_colrtable_rev[(clr1->r)][(clr1->r)]];
		clr->g =  epic12_device_colrtable_add[clr0->g][epic12_device_colrtable_rev[(clr1->g)][(clr1->g)]];
		clr->b =  epic12_device_colrtable_add[clr0->b][epic12_device_colrtable_rev[(clr1->b)][(clr1->b)]];
	}

	static inline  void clr_add(clr_t *clr, const clr_t *clr0, const clr_t *clr1)
	{

		clr->r =  epic12_device_colrtable_add[clr0->r][clr1->r];
		clr->g =  epic12_device_colrtable_add[clr0->g][clr1->g];
		clr->b =  epic12_device_colrtable_add[clr0->b][clr1->b];

	}

	static inline void clr_mul(clr_t *clr0, const clr_t *clr1)
	{
		clr0->r = epic12_device_colrtable[(clr0->r)][(clr1->r)];
		clr0->g = epic12_device_colrtable[(clr0->g)][(clr1->g)];
		clr0->b = epic12_device_colrtable[(clr0->b)][(clr1->b)];
	}

	static inline void clr_square(clr_t *clr0, const clr_t *clr1)
	{
		clr0->r = epic12_device_colrtable[(clr1->r)][(clr1->r)];
		clr0->g = epic12_device_colrtable[(clr1->g)][(clr1->g)];
		clr0->b = epic12_device_colrtable[(clr1->b)][(clr1->b)];
	}

	static inline void clr_mul_3param(clr_t *clr0, const clr_t *clr1, const clr_t *clr2)
	{
		clr0->r = epic12_device_colrtable[(clr2->r)][(clr1->r)];
		clr0->g = epic12_device_colrtable[(clr2->g)][(clr1->g)];
		clr0->b = epic12_device_colrtable[(clr2->b)][(clr1->b)];
	}

	static inline void clr_mul_rev(clr_t *clr0, const clr_t *clr1)
	{
		clr0->r = epic12_device_colrtable_rev[(clr0->r)][(clr1->r)];
		clr0->g = epic12_device_colrtable_rev[(clr0->g)][(clr1->g)];
		clr0->b = epic12_device_colrtable_rev[(clr0->b)][(clr1->b)];
	}

	static inline void clr_mul_rev_square(clr_t *clr0, const clr_t *clr1)
	{
		clr0->r = epic12_device_colrtable_rev[(clr1->r)][(clr1->r)];
		clr0->g = epic12_device_colrtable_rev[(clr1->g)][(clr1->g)];
		clr0->b = epic12_device_colrtable_rev[(clr1->b)][(clr1->b)];
	}

	static inline void clr_mul_rev_3param(clr_t *clr0, const clr_t *clr1, const clr_t *clr2)
	{
		clr0->r = epic12_device_colrtable_rev[(clr2->r)][(clr1->r)];
		clr0->g = epic12_device_colrtable_rev[(clr2->g)][(clr1->g)];
		clr0->b = epic12_device_colrtable_rev[(clr2->b)][(clr1->b)];
	}

	static inline void clr_mul_fixed(clr_t *clr, const UINT8 val, const clr_t *clr0)
	{
		clr->r = epic12_device_colrtable[val][(clr0->r)];
		clr->g = epic12_device_colrtable[val][(clr0->g)];
		clr->b = epic12_device_colrtable[val][(clr0->b)];
	}

	static inline void clr_mul_fixed_rev(clr_t *clr, const UINT8 val, const clr_t *clr0)
	{
		clr->r = epic12_device_colrtable_rev[val][(clr0->r)];
		clr->g = epic12_device_colrtable_rev[val][(clr0->g)];
		clr->b = epic12_device_colrtable_rev[val][(clr0->b)];
	}

	static inline void clr_copy(clr_t *clr, const clr_t *clr0)
	{
		clr->r = clr0->r;
		clr->g = clr0->g;
		clr->b = clr0->b;
	}


#endif

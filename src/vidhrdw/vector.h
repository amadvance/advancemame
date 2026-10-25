// license:BSD-3-Clause
// copyright-holders:Brad Oliver, Aaron Giles, Bernd Wiebelt, Allard van der Bas
#ifndef __VECTOR__
#define __VECTOR__

#include "artwork.h"

#define VECTOR_COLOR111(c) \
	MAKE_RGB(pal1bit((c) >> 2), pal1bit((c) >> 1), pal1bit((c) >> 0))

#define VECTOR_COLOR222(c) \
	MAKE_RGB(pal2bit((c) >> 4), pal2bit((c) >> 2), pal2bit((c) >> 0))

#define VECTOR_COLOR444(c) \
	MAKE_RGB(pal4bit((c) >> 8), pal4bit((c) >> 4), pal4bit((c) >> 0))

extern UINT8 *vectorram;
extern size_t vectorram_size;

VIDEO_START( vector );
VIDEO_UPDATE( vector );

void vector_clear_list(void);
void vector_add_point(int x, int y, rgb_t color, int intensity);
void vector_add_clip(int minx, int miny, int maxx, int maxy);

void vector_set_flicker(float _flicker);
float vector_get_flicker(void);

void vector_set_beam(float _beam);
float vector_get_beam(void);

/* Bitmap/artwork and external DVG interfaces retained for AdvanceMAME. */
#define MAX_POINTS 20000
#define VCLEAN 0
#define VDIRTY 1
#define VCLIP 2

typedef UINT32 vector_pixel_t;
#define VECTOR_PIXEL_END 0xffffffff
#define VECTOR_PIXEL(x,y) ((x) | ((y) << 16))
#define VECTOR_PIXEL_X(p) ((p) & 0xffff)
#define VECTOR_PIXEL_Y(p) ((p) >> 16)
extern vector_pixel_t *vector_dirty_list;
extern int translucency;
#define Tinten(intensity, col) \
	MAKE_RGB((RGB_RED(col) * (intensity)) >> 8, \
		(RGB_GREEN(col) * (intensity)) >> 8, \
		(RGB_BLUE(col) * (intensity)) >> 8)

typedef struct
{
	int x, y;
	rgb_t col;
	int intensity;
	int arg1, arg2;
	int status;
	rgb_t (*callback)(void);
} point;

void vector_register_aux_renderer(int (*renderer)(point *start, int num_points));
void vector_add_point_callback(int x, int y, rgb_t (*callback)(void), int intensity);
void vector_set_intensity(float intensity);
float vector_get_intensity(void);
void vector_set_gamma(float gamma);
float vector_get_gamma(void);

#endif

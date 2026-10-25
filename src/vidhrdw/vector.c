// license:BSD-3-Clause
// copyright-holders:Brad Oliver, Aaron Giles, Bernd Wiebelt, Allard van der Bas
/*
 * Vector list handling and bitmap rendering.
 *
 * The bitmap adapter rasterizes line segments directly, retaining
 * AdvanceMAME's artwork, CRT settings and external DVG renderer.
 * It uses distance-to-segment coverage rather than the legacy AA tables.
 */
#include <math.h>
#include "driver.h"
#include "vector.h"

UINT8 *vectorram;
size_t vectorram_size;

static int flicker;                              /* beam flicker value     */
static float flicker_correction = 0.0f;

static float beam_width;
static float gamma_correction = 1.2f;
static float intensity_correction = 1.5f;
static UINT8 gamma_table[256];
int translucency;
vector_pixel_t *vector_dirty_list;
static int (*vector_aux_renderer)(point *start, int num_points);

void vector_register_aux_renderer(int (*renderer)(point *start, int num_points))
{
	vector_aux_renderer = renderer;
}

void vector_set_gamma(float gamma)
{
	int i;
	gamma_correction = gamma > 0 ? gamma : 1.0f;
	for (i = 0; i < 256; i++)
		gamma_table[i] = (int)(255.0 * pow(i / 255.0, 1.0 / gamma_correction));
}

float vector_get_gamma(void)
{
	return gamma_correction;
}

void vector_set_intensity(float intensity)
{
	intensity_correction = intensity;
}

float vector_get_intensity(void)
{
	return intensity_correction;
}

static point *vector_list;
static int vector_index;


void vector_set_flicker(float _flicker)
{
	flicker_correction = _flicker;
	flicker = (int)(flicker_correction * 2.55);
}

float vector_get_flicker(void)
{
	return flicker_correction;
}

void vector_set_beam(float _beam)
{
	beam_width = _beam;
}

float vector_get_beam(void)
{
	return beam_width;
}

/*
 * Initializes vector game video emulation
 */

VIDEO_START( vector )
{
	beam_width = (float)options.beam / 65536.0f;
	translucency = options.translucency;
	vector_set_intensity(options.vector_intensity);
	vector_set_gamma(gamma_correction);
	if (Machine->color_depth != 15 && Machine->color_depth != 32)
		return 1;
	vector_dirty_list = auto_malloc(((Machine->absolute_visible_area.max_x + 1)
		* (Machine->absolute_visible_area.max_y + 1) + 1) * sizeof(*vector_dirty_list));
	vector_dirty_list[0] = VECTOR_PIXEL_END;

	/* Grab the settings for this session */
	vector_set_flicker(options.vector_flicker);

	vector_index = 0;

	/* allocate memory for tables */
	vector_list = auto_malloc (MAX_POINTS * sizeof (vector_list[0]));

	return 0;
}


/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */
void vector_add_point (int x, int y, rgb_t color, int intensity)
{
	point *newpoint;

	intensity *= intensity_correction;
	if (intensity < 0)
		intensity = 0;
	if (intensity > 0xff)
		intensity = 0xff;

	if (flicker && (intensity > 0))
	{
		intensity += (intensity * (0x80-(rand()&0xff)) * flicker)>>16;
		if (intensity < 0)
			intensity = 0;
		if (intensity > 0xff)
			intensity = 0xff;
	}
	newpoint = &vector_list[vector_index];
	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = color;
	newpoint->callback = NULL;
	newpoint->intensity = intensity;
	newpoint->status = VDIRTY; /* mark identical lines as clean later */

	vector_index++;
	if (vector_index >= MAX_POINTS)
	{
		vector_index--;
		logerror("*** Warning! Vector list overflow!\n");
	}
}

/*
 * Add new clipping info to the list
 */
void vector_add_point_callback(int x, int y, rgb_t (*callback)(void), int intensity)
{
	int index = vector_index;
	vector_add_point(x, y, 0, intensity);
	vector_list[index].callback = callback;
}

void vector_add_clip (int x1, int yy1, int x2, int y2)
{
	point *newpoint;

	newpoint = &vector_list[vector_index];
	newpoint->x = x1;
	newpoint->y = yy1;
	newpoint->arg1 = x2;
	newpoint->arg2 = y2;
	newpoint->status = VCLIP;

	vector_index++;
	if (vector_index >= MAX_POINTS)
	{
		vector_index--;
		logerror("*** Warning! Vector list overflow!\n");
	}
}


/*
 * The vector CPU creates a new display list. We save the old display list,
 * but only once per refresh.
 */
void vector_clear_list (void)
{
	vector_index = 0;
}



/* Add a covered sample to a direct RGB bitmap, with additive beam blending. */
static void vector_plot(mame_bitmap *bitmap, int x, int y, rgb_t color, double level)
{
	int r = RGB_RED(color) * level;
	int g = RGB_GREEN(color) * level;
	int b = RGB_BLUE(color) * level;
	UINT32 old;

	if (bitmap->depth == 15)
	{
		old = ((UINT16 *)bitmap->line[y])[x];
		r = MIN(31, (r >> 3) + ((old >> 10) & 31));
		g = MIN(31, (g >> 3) + ((old >> 5) & 31));
		b = MIN(31, (b >> 3) + (old & 31));
		((UINT16 *)bitmap->line[y])[x] = (r << 10) | (g << 5) | b;
	}
	else
	{
		old = ((UINT32 *)bitmap->line[y])[x];
		r = MIN(255, r + RGB_RED(old));
		g = MIN(255, g + RGB_GREEN(old));
		b = MIN(255, b + RGB_BLUE(old));
		((UINT32 *)bitmap->line[y])[x] = MAKE_RGB(r, g, b);
	}
}

/*
 * Walk the major axis and cover a narrow strip around the beam. Project each
 * sample onto the finite segment so endpoints and zero-length dots also work.
 * Clipping limits the samples, not the endpoints: blanked moves still position
 * the beam correctly for the next segment.
 */
static void vector_segment(mame_bitmap *bitmap, double x0, double y0,
	double x1, double y1, const rectangle *clip, const point *p)
{
	double dx = x1 - x0, dy = y1 - y0;
	double length2 = dx * dx + dy * dy;
	double radius = MAX(1.0, beam_width) * 0.5;
	double reach = radius + 0.5;
	int vertical = fabs(dy) > fabs(dx);
	double a0 = vertical ? y0 : x0, a1 = vertical ? y1 : x1;
	double b0 = vertical ? x0 : y0, b1 = vertical ? x1 : y1;
	int amin = vertical ? clip->min_y : clip->min_x;
	int amax = vertical ? clip->max_y : clip->max_x;
	int bmin = vertical ? clip->min_x : clip->min_y;
	int bmax = vertical ? clip->max_x : clip->max_y;
	int first = MAX(amin, floor(MIN(a0, a1) - reach));
	int last = MIN(amax, ceil(MAX(a0, a1) + reach));
	int a, b;

	for (a = first; a <= last; a++)
	{
		double t = a1 != a0 ? (a - a0) / (a1 - a0) : 0;
		double center = b0 + MAX(0.0, MIN(1.0, t)) * (b1 - b0);
		int low = MAX(bmin, floor(center - reach * 1.5));
		int high = MIN(bmax, ceil(center + reach * 1.5));
		for (b = low; b <= high; b++)
		{
			double x = vertical ? b : a, y = vertical ? a : b;
			double u = length2 ? ((x - x0) * dx + (y - y0) * dy) / length2 : 0;
			double ex, ey, coverage;
			rgb_t color;
			u = MAX(0.0, MIN(1.0, u));
			ex = x - (x0 + u * dx);
			ey = y - (y0 + u * dy);
			coverage = radius + 0.5 - sqrt(ex * ex + ey * ey);
			if (!options.antialias)
				coverage = coverage >= 0.5 ? 1.0 : 0.0;
			if (coverage <= 0)
				continue;
			color = p->callback ? p->callback() : p->col;
			vector_plot(bitmap, x, y, color,
				MIN(1.0, coverage) * gamma_table[p->intensity] / 255.0);
		}
	}
}

VIDEO_UPDATE( vector )
{
	double xscale = (double)bitmap->width / (Machine->visible_area.max_x - Machine->visible_area.min_x);
	double yscale = (double)bitmap->height / (Machine->visible_area.max_y - Machine->visible_area.min_y);
	double xoffs = Machine->visible_area.min_x;
	double yoffs = Machine->visible_area.min_y;
	double lastx = 0, lasty = 0;
	rectangle clip = { 0, bitmap->width - 1, 0, bitmap->height - 1 };
	int i, x, y, dirty = 0;

	if (vector_aux_renderer && !vector_aux_renderer(vector_list, vector_index))
	{
		vector_dirty_list[0] = VECTOR_PIXEL_END;
		return;
	}

	fillbitmap(bitmap, 0, NULL);
	for (i = 0; i < vector_index; i++)
	{
		point *p = &vector_list[i];
		double px = (p->x / 65536.0 - xoffs) * xscale;
		double py = (p->y / 65536.0 - yoffs) * yscale;
		if (p->status == VCLIP)
		{
			clip.min_x = MAX(0, ceil(px));
			clip.min_y = MAX(0, ceil(py));
			clip.max_x = MIN(bitmap->width - 1, floor((p->arg1 / 65536.0 - xoffs) * xscale));
			clip.max_y = MIN(bitmap->height - 1, floor((p->arg2 / 65536.0 - yoffs) * yscale));
		}
		else
		{
			if (p->intensity)
				vector_segment(bitmap, lastx, lasty, px, py, &clip, p);
			lastx = px;
			lasty = py;
		}
	}

	/* The bitmap adapter redraws the frame, including erased old segments.
	 * Artwork consumes (and may translate) this list, so rebuild it each time. */
	for (y = 0; y < bitmap->height; y++)
		for (x = 0; x < bitmap->width; x++)
			vector_dirty_list[dirty++] = VECTOR_PIXEL(x, y);
	vector_dirty_list[dirty] = VECTOR_PIXEL_END;
}

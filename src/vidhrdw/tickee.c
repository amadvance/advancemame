// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    Raster Elite Tickee Tickats hardware

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "tickee.h"


UINT16 *tickee_vram;


/* local variables */
static void *setup_gun_timer;



/*************************************
 *
 *  Compute X/Y coordinates
 *
 *************************************/

INLINE void get_crosshair_xy(int player, int *x, int *y)
{
	*x = ((readinputport(4 + player * 2) & 0xff) * Machine->drv->screen_width) >> 8;
	*y = ((readinputport(5 + player * 2) & 0xff) * Machine->drv->screen_height) >> 8;
}



/*************************************
 *
 *  Light gun interrupts
 *
 *************************************/

static void trigger_gun_interrupt(int which)
{
	/* fire the IRQ at the correct moment */
	cpunum_set_input_line(0, which, ASSERT_LINE);
}


static void clear_gun_interrupt(int which)
{
	/* clear the IRQ on the next scanline? */
	cpunum_set_input_line(0, which, CLEAR_LINE);
}


static void setup_gun_interrupts(int param)
{
	int beamx, beamy;
	double time;

	/* set a timer to do this again next frame */
	timer_adjust(setup_gun_timer, cpu_getscanlinetime(0), 0, 0);

	/* only do work if the palette is flashed */
	if (!tickee_control[2])
		return;

	/* generate interrupts for player 1's gun */
	get_crosshair_xy(0, &beamx, &beamy);
	time = cpu_getscanlinetime(beamy);
	time += cpu_getscanlineperiod() * (double)(beamx + 175) / (double)450;
	timer_set(time, 0, trigger_gun_interrupt);
	timer_set(time + cpu_getscanlineperiod(), 0, clear_gun_interrupt);

	/* generate interrupts for player 2's gun */
	get_crosshair_xy(1, &beamx, &beamy);
	time = cpu_getscanlinetime(beamy);
	time += cpu_getscanlineperiod() * (double)(beamx + 175) / (double)450;
	timer_set(time, 1, trigger_gun_interrupt);
	timer_set(time + cpu_getscanlineperiod(), 1, clear_gun_interrupt);
}



/*************************************
 *
 *  Video startup
 *
 *************************************/

VIDEO_START( tickee )
{
	/* start a timer going on the first scanline of every frame */
	setup_gun_timer = timer_alloc(setup_gun_interrupts);
	timer_adjust(setup_gun_timer, cpu_getscanlinetime(0), 0, 0);

	return 0;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( tickee )
{
	int v, h, width, xoffs, beamx, beamy;
	UINT8 *base1 = (UINT8 *)tickee_vram;
	pen_t pen_lookup[256];
	UINT32 offset;

	/* fill out the pen array based on the palette bank */
	for (h = 0; h < 256; h++)
		pen_lookup[h] = tickee_control[2] ? 255 : h;

	/* determine the base of the videoram */
	offset = (~tms34010_get_DPYSTRT(0) & 0xfff0) << 5;
	offset += TOBYTE(0x1000) * (cliprect->min_y - Machine->visible_area.min_y);

	/* determine how many pixels to copy */
	xoffs = cliprect->min_x;
	width = cliprect->max_x - xoffs + 1;
	offset += xoffs;

	/* loop over rows */
	for (v = cliprect->min_y; v <= cliprect->max_y; v++)
	{
		UINT8 scanline[512];

		/* extract the scanline to account for endianness */
		for (h = 0; h < width; h++)
			scanline[h] = base1[BYTE_XOR_LE(offset + h) & 0x7ffff];

		/* draw it and advance */
		draw_scanline8(bitmap, xoffs, v, width, scanline, pen_lookup, -1);
		offset += TOBYTE(0x1000);
	}

	/* draw player 1's crosshair */
	get_crosshair_xy(0, &beamx, &beamy);
	draw_crosshair(bitmap, beamx, beamy, cliprect, 0);

	/* draw player 2's crosshair */
	get_crosshair_xy(1, &beamx, &beamy);
	draw_crosshair(bitmap, beamx, beamy, cliprect, 1);
}


int rapidfir_palette_blank;
int rapidfir_has_two_guns;
UINT8 rapidfir_gunx[2];
static void *rapidfir_setup_timer;

static void rapidfir_crosshair_xy(int player, int *x, int *y)
{
	*x = ((readinputport(5 + player * 2) & 0xff) * Machine->drv->screen_width) >> 8;
	*y = ((readinputport(6 + player * 2) & 0xff) * Machine->drv->screen_height) >> 8;
}

static void rapidfir_trigger_gun(int which)
{
	cpuintrf_push_context(0);
	rapidfir_gunx[which] = tms34010_io_register_r(28, 0) - 58;
	cpuintrf_pop_context();
	cpunum_set_input_line(0, which, ASSERT_LINE);
}

static void rapidfir_setup_guns(int param)
{
	int player;
	timer_adjust(rapidfir_setup_timer, cpu_getscanlinetime(0), 0, 0);
	if (!rapidfir_palette_blank)
		return;
	for (player = 0; player < (rapidfir_has_two_guns ? 2 : 1); player++)
	{
		int x, y;
		double time;
		rapidfir_crosshair_xy(player, &x, &y);
		y = (y + 293 - 5) % 293;
		time = cpu_getscanlinetime(y) + cpu_getscanlineperiod() * x / 556.0;
		timer_set(time, player, rapidfir_trigger_gun);
		timer_set(time + cpu_getscanlineperiod(), player, clear_gun_interrupt);
	}
}

VIDEO_START( mouseatk )
{
	return 0;
}

VIDEO_START( rapidfir )
{
	rapidfir_setup_timer = timer_alloc(rapidfir_setup_guns);
	timer_adjust(rapidfir_setup_timer, cpu_getscanlinetime(0), 0, 0);
	return 0;
}

static void tickee_draw_frame(mame_bitmap *bitmap, const rectangle *cliprect, int blank)
{
	UINT8 *base = (UINT8 *)tickee_vram;
	UINT32 offset = (~tms34010_get_DPYSTRT(0) & 0xfff0) << 5;
	int y, x;
	pen_t pens[256];

	for (x = 0; x < 256; x++)
		pens[x] = blank ? 255 : x;
	offset += 512 * (cliprect->min_y - Machine->visible_area.min_y);
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT8 pixels[512];
		int width = cliprect->max_x - cliprect->min_x + 1;
		for (x = 0; x < width; x++)
			pixels[x] = base[BYTE_XOR_LE(offset + cliprect->min_x + x) & 0x7ffff];
		draw_scanline8(bitmap, cliprect->min_x, y, width, pixels, pens, -1);
		offset += 512;
	}
}

VIDEO_UPDATE( mouseatk )
{
	tickee_draw_frame(bitmap, cliprect, tickee_control[2] & 1);
}

VIDEO_UPDATE( rapidfir )
{
	int x, y;
	tickee_draw_frame(bitmap, cliprect, rapidfir_palette_blank);
	rapidfir_crosshair_xy(0, &x, &y);
	draw_crosshair(bitmap, x, y, cliprect, 0);
	if (rapidfir_has_two_guns)
	{
		rapidfir_crosshair_xy(1, &x, &y);
		draw_crosshair(bitmap, x, y, cliprect, 1);
	}
}

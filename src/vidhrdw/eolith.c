#include "driver.h"
#include "includes/eolith.h"

int eolith_buffer = 0;
static mame_bitmap *bitmaps[2];
static UINT32 *eo_vram;
static int pen2_visible, last_pen2_x, last_pen2_y;

static void plot_pixel_rgb(int x, int y, int color)
{
	if (Machine->color_depth == 32)
	{
		UINT32 r = (color & 0x001f) << 3;
		UINT32 g = (color & 0x03e0) >> 2;
		UINT32 b = (color & 0x7c00) >> 7;
		((UINT32 *)bitmaps[eolith_buffer]->line[y])[x] = b | (g<<8) | (r<<16);
	}
	else
	{
		((UINT16 *)bitmaps[eolith_buffer]->line[y])[x] = color;
	}
}

WRITE32_HANDLER( eolith_vram_w )
{
	int x,y;
	switch(mem_mask)
	{
		case 0:
			eolith_vram_w(offset,data,0x0000ffff);
			eolith_vram_w(offset,data,0xffff0000);
			return;

		case 0xffff:
			if(data & 0x80000000)
				return;
		break;

		case 0xffff0000:
			if(data & 0x8000)
				return;
		break;
	}

	COMBINE_DATA(&eo_vram[offset+(0x40000/4)*eolith_buffer]);

	//logical line width = 336;
	x = offset % (336/2);
	y = offset / (336/2);

	if(x < 320/2 && y < 240)
	{
		plot_pixel_rgb(x*2,  y,(eo_vram[offset+(0x40000/4)*eolith_buffer]>>16) & 0x7fff);
		plot_pixel_rgb(x*2+1,y, eo_vram[offset+(0x40000/4)*eolith_buffer] & 0x7fff);

	}
}


READ32_HANDLER( eolith_vram_r )
{
	return eo_vram[offset+(0x40000/4)*eolith_buffer];
}

VIDEO_START( eolith )
{
	pen2_visible = 0;
	last_pen2_x = 80;
	last_pen2_y = 60;
	eo_vram = auto_malloc(0x40000*2);
	memset(eo_vram, 0, 0x40000*2);
	bitmaps[0] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	bitmaps[1] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if(!bitmaps[0] || !bitmaps[1])
		return 1;

	fillbitmap(bitmaps[0], 0, NULL);
	fillbitmap(bitmaps[1], 0, NULL);
	return 0;
}

VIDEO_UPDATE( eolith )
{
	copybitmap(bitmap,bitmaps[eolith_buffer ^ 1],0,0,0,0,cliprect,TRANSPARENCY_NONE, 0);
}

VIDEO_UPDATE( hidctch3 )
{
	int x, y;
	int pen2_x = readinputport(4);
	int pen2_y = readinputport(5);

	video_update_eolith(screen, bitmap, cliprect);
	hidctch3_pen_position(0, &x, &y);
	draw_crosshair(bitmap, x, y, cliprect, 0);
	if (pen2_x != last_pen2_x || pen2_y != last_pen2_y
		|| (readinputport(0) & 0xff000000) != 0xff000000)
		pen2_visible = 1;
	last_pen2_x = pen2_x;
	last_pen2_y = pen2_y;
	if (pen2_visible)
	{
		hidctch3_pen_position(1, &x, &y);
		draw_crosshair(bitmap, x, y, cliprect, 1);
	}
}

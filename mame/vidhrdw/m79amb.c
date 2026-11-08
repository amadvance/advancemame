// license:BSD-3-Clause
// copyright-holders:Al Kossow
#include "driver.h"
#include "m79amb.h"

static UINT8 mask;

WRITE8_HANDLER( ramtek_mask_w )
{
	mask = data;
}

WRITE8_HANDLER( ramtek_videoram_w )
{
	videoram[offset] = data & ~mask;
}

VIDEO_UPDATE( m79amb )
{
	int x,y;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int offset = y * 32 + (x >> 3);
			int pixel = (videoram[offset] >> (7 - (x & 7))) & 1;
			plot_pixel(bitmap,x,y,Machine->pens[pixel]);
		}
}

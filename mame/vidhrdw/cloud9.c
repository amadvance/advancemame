// license:BSD-3-Clause
// copyright-holders:Mike Balfour, Aaron Giles
#include "driver.h"
#include "cloud9.h"

static double rweights[3], gweights[3], bweights[3];
static mame_bitmap *spritebitmap;

static UINT8 video_control[8];
static UINT8 bitmode_addr[2];

static const UINT8 *syncprom;
static const UINT8 *wpprom;

VIDEO_START( cloud9 )
{

	videoram = auto_malloc(0x8000);
	memset(videoram, 0, 0x8000);
	memory_set_bankptr(1, videoram);

	syncprom = memory_region(REGION_PROMS) + 0x000;
	wpprom = memory_region(REGION_PROMS) + 0x200;

	rweights[0] = gweights[0] = bweights[0] = 255.0 / (1.0 + 22000.0/10000.0 + 22000.0/4700.0);
	rweights[1] = gweights[1] = bweights[1] = rweights[0] * 22000.0/10000.0;
	rweights[2] = gweights[2] = bweights[2] = rweights[0] * 22000.0/4700.0;

	spritebitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);

	state_save_register_global_pointer(videoram, 0x8000);
	state_save_register_global_array(video_control);
	state_save_register_global_array(bitmode_addr);
	return 0;
}

WRITE8_HANDLER( cloud9_video_control_w )
{

	video_control[offset] = (data >> 7) & 1;
}

WRITE8_HANDLER( cloud9_paletteram_w )
{
	int bit0, bit1, bit2;
	int r, g, b;

	r = (data & 0xe0) >> 5;
	g = (data & 0x1c) >> 2;
	b = ((data & 0x03) << 1) | ((offset & 0x40) >> 6);

	bit0 = (~r >> 0) & 0x01;
	bit1 = (~r >> 1) & 0x01;
	bit2 = (~r >> 2) & 0x01;
	r = (int)(rweights[0]*bit0 + rweights[1]*bit1 + rweights[2]*bit2 + 0.5);

	bit0 = (~g >> 0) & 0x01;
	bit1 = (~g >> 1) & 0x01;
	bit2 = (~g >> 2) & 0x01;
	g = (int)(gweights[0]*bit0 + gweights[1]*bit1 + gweights[2]*bit2 + 0.5);

	bit0 = (~b >> 0) & 0x01;
	bit1 = (~b >> 1) & 0x01;
	bit2 = (~b >> 2) & 0x01;
	b = (int)(bweights[0]*bit0 + bweights[1]*bit1 + bweights[2]*bit2 + 0.5);

	palette_set_color(offset & 0x3f, r, g, b);
}

INLINE void cloud9_write_vram(UINT16 addr, UINT8 data, UINT8 bitmd, UINT8 pixba)
{
	UINT8 *dest = &videoram[0x0000 | (addr & 0x3fff)];
	UINT8 *dest2 = &videoram[0x4000 | (addr & 0x3fff)];
	UINT8 promaddr = 0;
	UINT8 wpbits;

	promaddr |= bitmd << 7;
	promaddr |= video_control[4] << 6;
	promaddr |= video_control[6] << 5;
	promaddr |= ((addr & 0xf000) != 0x4000) << 4;
	promaddr |= ((addr & 0x3800) == 0x0000) << 3;
	promaddr |= ((addr & 0x0600) == 0x0600) << 2;
	promaddr |= (pixba << 0);

	wpbits = wpprom[promaddr];

	if (!(wpbits & 1))
		dest2[0] = (dest2[0] & 0x0f) | (data & 0xf0);
	if (!(wpbits & 2))
		dest2[0] = (dest2[0] & 0xf0) | (data & 0x0f);
	if (!(wpbits & 4))
		dest[0] = (dest[0] & 0x0f) | (data & 0xf0);
	if (!(wpbits & 8))
		dest[0] = (dest[0] & 0xf0) | (data & 0x0f);
}

INLINE void bitmode_autoinc(void)
{

	if (!video_control[0])
		bitmode_addr[0]++;

	if (!video_control[1])
		bitmode_addr[1]++;
}

WRITE8_HANDLER( cloud9_videoram_w )
{

	cloud9_write_vram(offset, data, 0, 0);
}

READ8_HANDLER( cloud9_bitmode_r )
{

	UINT16 addr = (bitmode_addr[1] << 6) | (bitmode_addr[0] >> 2);

	UINT8 result = videoram[((~bitmode_addr[0] & 2) << 13) | addr] << ((bitmode_addr[0] & 1) * 4);

	bitmode_autoinc();

	return (result >> 4) | 0xf0;
}

WRITE8_HANDLER( cloud9_bitmode_w )
{

	UINT16 addr = (bitmode_addr[1] << 6) | (bitmode_addr[0] >> 2);

	data = (data & 0x0f) | (data << 4);

	cloud9_write_vram(addr, data, 1, bitmode_addr[0] & 3);

	bitmode_autoinc();
}

WRITE8_HANDLER( cloud9_bitmode_addr_w )
{

	cloud9_write_vram(offset, data, 0, 0);
	bitmode_addr[offset] = data;
}

VIDEO_UPDATE( cloud9 )
{
	UINT8 *spriteaddr = spriteram;
	int flip = video_control[5] ? 0xff : 0x00;
	pen_t black = get_black_pen();
	int x, y, offs;

	fillbitmap(spritebitmap, 0x00, cliprect);
	for (offs = 0; offs < 0x20; offs++)
		if (spriteaddr[offs + 0x00] != 0)
		{
			int x = spriteaddr[offs + 0x60];
			int y = 256 - 15 - spriteaddr[offs + 0x00];
			int xflip = spriteaddr[offs + 0x40] & 0x80;
			int yflip = spriteaddr[offs + 0x40] & 0x40;
			int which = spriteaddr[offs + 0x20];
			int color = 0;

			drawgfx(spritebitmap, Machine->gfx[0], which, color, xflip, yflip, x, y, cliprect, TRANSPARENCY_PEN, 0);
			if (x >= 256 - 16)
				drawgfx(spritebitmap, Machine->gfx[0], which, color, xflip, yflip, x - 256, y, cliprect, TRANSPARENCY_PEN, 0);
		}

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dst = (UINT16 *)bitmap->line[y];

		if (~syncprom[y] & 2)
		{
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				dst[x] = black;
		}

		else
		{
			UINT16 *mosrc = (UINT16 *)spritebitmap->line[y];
			int effy = y ^ flip;
			UINT8 *src[2];

			src[0] = &videoram[0x4000 | (effy * 64)];
			src[1] = &videoram[0x0000 | (effy * 64)];

			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{

				if (x >= 256)
					dst[x] = black;

				else
				{
					int effx = x ^ flip;

					UINT8 pix = (src[(effx >> 1) & 1][effx / 4] >> ((~effx & 1) * 4)) & 0x0f;
					UINT8 mopix = mosrc[x];

					if (mopix != 0)
						pix = mopix | 0x10;

					pix |= video_control[7] << 5;

					dst[x] = pix;
				}
			}
		}
	}
}

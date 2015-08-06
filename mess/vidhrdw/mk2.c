/******************************************************************************
 PeT mess@utanet.at
******************************************************************************/
#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"
#include "mscommon.h"

#include "includes/mk2.h"

UINT8 mk2_led[5]= {0};

unsigned char mk2_palette[] =
{
	0x20, 0x02, 0x05,
	0xc0, 0x00, 0x00
};

unsigned short mk2_colortable[] = {
	0, 1
};

PALETTE_INIT( mk2 )
{
	palette_set_colors(0, mk2_palette, sizeof(mk2_palette) / 3);
	memcpy(colortable, mk2_colortable, sizeof(mk2_colortable));
}

VIDEO_START( mk2 )
{
	// artwork seams to need this
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)auto_malloc (videoram_size);
	return video_start_generic();
}

static const char led[]={
	"      aaaaaaaaaaaaaaa\r"
	"     f aaaaaaaaaaaaa b\r"
	"     ff aaaaaaaaaaa bb\r"
	"     fff           bbb\r"
	"     fff           bbb\r"
	"    fff           bbb\r"
	"    fff           bbb\r"
	"    fff           bbb\r"
	"    fff           bbb\r"
	"   fff           bbb\r"
	"   fff           bbb\r"
	"   ff             bb\r"
    "   f ggggggggggggg b\r"
    "    gggggggggggggg\r"
	"  e ggggggggggggg c\r"
	"  ee             cc\r"
	"  eee           ccc\r"
	"  eee           ccc\r"
	" eee           ccc\r"
	" eee           ccc\r"
	" eee           ccc\r"
	" eee           ccc\r"
	"eee           ccc\r"
	"eee           ccc\r"
	"ee ddddddddddd cc   hh\r"
    "e ddddddddddddd c  hhh\r"
    " ddddddddddddddd   hh"
};

static void mk2_draw_7segment(mame_bitmap *bitmap,int value, int x, int y)
{
	draw_led(bitmap, led, value, x, y);
}

static const struct {
	int x,y;
} mk2_led_pos[8]={
	{70,96},
	{99,96},
	{128,96},
	{157,96},
	{47,223},
	{85,223},
	{123,223},
	{162,223}
};

static void mk2_draw_led(mame_bitmap *bitmap,INT16 color, int x, int y)
{
	draw_led(bitmap, radius_2_led, color, x, y);
}

VIDEO_UPDATE( mk2 )
{
	int i;

	for (i=0; i<4; i++)
		mk2_draw_7segment(bitmap, mk2_led[i]&0x7f, mk2_led_pos[i].x, mk2_led_pos[i].y);

	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&8?1:0], 
				 mk2_led_pos[4].x, mk2_led_pos[4].y);
	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&0x20?1:0], 
				 mk2_led_pos[5].x, mk2_led_pos[5].y); //?
	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&0x10?1:0], 
				 mk2_led_pos[6].x, mk2_led_pos[6].y);
	mk2_draw_led(bitmap, Machine->pens[mk2_led[4]&0x10?0:1], 
				 mk2_led_pos[7].x, mk2_led_pos[7].y);

	mk2_led[0]= mk2_led[1]= mk2_led[2]= mk2_led[3]= mk2_led[4]= 0;
}
